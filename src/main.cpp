// Browser entry point. Replaces PaperTossApplication, PaperTossActivity and
// PapertossGLSurfaceView: it owns the WebGL context, the canvas layout, the
// pointer events and the frame loop.

#include <cstdio>

#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/html5_webgl.h>

#include "gl1.h"
#include "level.h"
#include "papertoss.h"
#include "platform.h"
#include "soundmgr.h"
#include "texture.h"
#include "vec.h"

namespace {

EMSCRIPTEN_WEBGL_CONTEXT_HANDLE g_context = 0;
double g_time = 0.0;
double g_css_to_pixels = 1.0;
double g_canvas_left = 0.0;
double g_canvas_top = 0.0;
bool g_audio_unlocked = false;
bool g_pointer_down = false;

struct Viewport {
    int css_width = 0;
    int css_height = 0;
    int pixel_width = 0;
    int pixel_height = 0;
};

EM_JS(void, js_hide_splash, (), {
    var splash = document.getElementById('splash');
    if (splash) splash.classList.add('hidden');
});

EM_JS(void, js_close_window, (), {
    // The menu's Exit button. Browsers only honour this for script-opened
    // windows, so most of the time it is a no-op.
    try { window.close(); } catch (e) {}
});

EM_JS(double, js_canvas_left, (), {
    var c = document.getElementById('canvas');
    return c ? c.getBoundingClientRect().left : 0;
});

EM_JS(double, js_canvas_top, (), {
    var c = document.getElementById('canvas');
    return c ? c.getBoundingClientRect().top : 0;
});

EM_JS(double, js_window_width, (), { return window.innerWidth; });
EM_JS(double, js_window_height, (), { return window.innerHeight; });
EM_JS(double, js_pixel_ratio, (), { return window.devicePixelRatio || 1; });

// The game renders a 320x480 (or 294.25x480 in hi-res mode) portrait screen.
// Fit that into the window like a phone display: centred, letterboxed, nothing
// else on the page.
Viewport computeViewport(bool hi_res) {
    float aspect = hi_res ? (Config::ADJUSTED_ORTHO_WIDTH / Config::SCREEN_HEIGHT_F)
                          : (Config::SCREEN_WIDTH_F / Config::SCREEN_HEIGHT_F);
    double window_w = js_window_width();
    double window_h = js_window_height();
    double ratio = js_pixel_ratio();
    double css_w = window_w;
    double css_h = css_w / aspect;
    if (css_h > window_h) {
        css_h = window_h;
        css_w = css_h * aspect;
    }
    Viewport v;
    v.css_width = (int) css_w;
    v.css_height = (int) css_h;
    v.pixel_width = (int) (css_w * ratio);
    v.pixel_height = (int) (css_h * ratio);
    return v;
}

void applyViewport() {
    Viewport v = computeViewport(Globals::HI_RES);
    emscripten_set_element_css_size("#canvas", v.css_width, v.css_height);
    emscripten_set_canvas_element_size("#canvas", v.pixel_width, v.pixel_height);
    g_css_to_pixels = v.css_width > 0 ? (double) v.pixel_width / (double) v.css_width : 1.0;

    Globals::VIEWPORT_W = v.pixel_width;
    Globals::VIEWPORT_H = v.pixel_height;
    Globals::VIEWPORT_X = 0;
    Globals::VIEWPORT_Y = 0;
    Globals::SURFACE_H = v.pixel_height;
    glViewport(Globals::VIEWPORT_X, Globals::VIEWPORT_Y, Globals::VIEWPORT_W, Globals::VIEWPORT_H);
    glClear(GL_COLOR_BUFFER_BIT);
    g_canvas_left = js_canvas_left();
    g_canvas_top = js_canvas_top();
    Evt::getInstance().publish("sizeGl");
}

// PapertossGLSurfaceView.onTouchEvent(): canvas pixels to the game's ortho box.
v2f toGamePoint(double css_x, double css_y) {
    float x2 = (float) (css_x * g_css_to_pixels);
    float y = Globals::SURFACE_H - (float) (css_y * g_css_to_pixels);
    float x;
    if (Globals::HI_RES) {
        x = Globals::VIEWPORT_X + Config::ORTHO_ADJUSTMENT_F +
            ((Config::ADJUSTED_ORTHO_WIDTH / Globals::VIEWPORT_W) * x2);
    } else {
        x = Globals::VIEWPORT_X + ((320.0f / Globals::VIEWPORT_W) * x2);
    }
    return v2f(x, Globals::VIEWPORT_Y + ((480.0f / Globals::VIEWPORT_H) * y));
}

void unlockAudio() {
    if (g_audio_unlocked) return;
    g_audio_unlocked = true;
    if (Globals::soundMgr != nullptr) Globals::soundMgr->startBackgroundSound();
}

// Pointer positions arrive in client (viewport) coordinates; the canvas is
// letterboxed inside the page, so shift them into canvas space first.
void publishPointer(const char* event_name, double client_x, double client_y) {
    Evt::getInstance().publish(event_name, toGamePoint(client_x - g_canvas_left, client_y - g_canvas_top));
}

EM_BOOL onMouseDown(int, const EmscriptenMouseEvent* e, void*) {
    unlockAudio();
    g_canvas_left = js_canvas_left();
    g_canvas_top = js_canvas_top();
    g_pointer_down = true;
    publishPointer("onPtrDown", e->clientX, e->clientY);
    return EM_TRUE;
}

EM_BOOL onMouseMove(int, const EmscriptenMouseEvent* e, void*) {
    if (g_pointer_down) publishPointer("onPtrMove", e->clientX, e->clientY);
    return EM_TRUE;
}

EM_BOOL onMouseUp(int, const EmscriptenMouseEvent* e, void*) {
    if (!g_pointer_down) return EM_TRUE;
    g_pointer_down = false;
    publishPointer("onPtrUp", e->clientX, e->clientY);
    return EM_TRUE;
}

EM_BOOL onTouch(int event_type, const EmscriptenTouchEvent* e, void*) {
    if (e->numTouches <= 0) return EM_TRUE;
    const EmscriptenTouchPoint& touch = e->touches[0];
    switch (event_type) {
        case EMSCRIPTEN_EVENT_TOUCHSTART:
            unlockAudio();
            g_canvas_left = js_canvas_left();
            g_canvas_top = js_canvas_top();
            publishPointer("onPtrDown", touch.clientX, touch.clientY);
            break;
        case EMSCRIPTEN_EVENT_TOUCHMOVE:
            publishPointer("onPtrMove", touch.clientX, touch.clientY);
            break;
        case EMSCRIPTEN_EVENT_TOUCHEND:
        case EMSCRIPTEN_EVENT_TOUCHCANCEL:
            publishPointer("onPtrUp", touch.clientX, touch.clientY);
            break;
        default:
            break;
    }
    return EM_TRUE;
}

// Escape stands in for the Android back key.
EM_BOOL onKeyDown(int, const EmscriptenKeyboardEvent* e, void*) {
    if (std::string(e->key) != "Escape") return EM_FALSE;
    Evt& evt = Evt::getInstance();
    if (Papertoss::state == Papertoss::GameState::LEVEL) {
        evt.publish("paperTossPlaySound", "Crumple.wav");
    } else if (Papertoss::state == Papertoss::GameState::SCORE) {
        evt.publish("paperTossPlaySound", "Computer.wav");
    } else if (Papertoss::state == Papertoss::GameState::MENU) {
        // Android let the back key fall through to Activity.finish() here.
        evt.publish("paperTossPlaySound", "Crumple.wav");
        evt.publish("onExitPressed");
    }
    evt.publish("gotoMenu");
    return EM_TRUE;
}

EM_BOOL onResize(int, const EmscriptenUiEvent*, void*) {
    applyViewport();
    return EM_TRUE;
}

void frame() {
    double time = Util::getTime();
    double elapsed = time - g_time;
    g_time = time;
    Papertoss::update(elapsed);
    if (Globals::HI_RES) glClear(GL_COLOR_BUFFER_BIT);
    Papertoss::render();
}

}  // namespace

int main() {
    // PaperTossApplication.onCreate(): pick the asset set from the screen size.
    Viewport probe = computeViewport(true);
    if (probe.pixel_height > Config::SCREEN_HEIGHT) {
        Globals::HI_RES = true;
        Globals::SCALE_FACTOR = 0.613027f;
    } else {
        Globals::HI_RES = false;
        Globals::SCALE_FACTOR = 1.0f;
    }
    Globals::texture_mgr = new TextureMgr();

    SaveData::load();
    Globals::STARTS_ANY_VERSION = SaveData::read(0, "STARTS_ANY_VERSION");
    Globals::STARTS_ANY_VERSION++;
    SaveData::write(Globals::STARTS_ANY_VERSION, "STARTS_ANY_VERSION");
    SaveData::save();

    Globals::soundMgr = new SoundMgr();

    EmscriptenWebGLContextAttributes attrs;
    emscripten_webgl_init_context_attributes(&attrs);
    attrs.alpha = EM_FALSE;
    attrs.depth = EM_FALSE;
    attrs.stencil = EM_FALSE;
    attrs.antialias = EM_FALSE;
    attrs.premultipliedAlpha = EM_FALSE;
    attrs.preserveDrawingBuffer = EM_FALSE;
    attrs.majorVersion = 1;
    attrs.minorVersion = 0;
    g_context = emscripten_webgl_create_context("#canvas", &attrs);
    if (g_context <= 0) {
        std::printf("main: could not create a WebGL context\n");
        return 1;
    }
    emscripten_webgl_make_context_current(g_context);

    gl1::init();
    applyViewport();

    Papertoss::initialize();
    Papertoss::unShutdown();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    if (Globals::first_launch) {
        Papertoss::activate();
        Globals::first_launch = false;
    }

    Evt::getInstance().subscribe("onExitPressed", [](const EvtArg&) { js_close_window(); });

    emscripten_set_mousedown_callback("#canvas", nullptr, EM_TRUE, onMouseDown);
    emscripten_set_mousemove_callback("#canvas", nullptr, EM_TRUE, onMouseMove);
    emscripten_set_mouseup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, EM_TRUE, onMouseUp);
    emscripten_set_touchstart_callback("#canvas", nullptr, EM_TRUE, onTouch);
    emscripten_set_touchmove_callback("#canvas", nullptr, EM_TRUE, onTouch);
    emscripten_set_touchend_callback("#canvas", nullptr, EM_TRUE, onTouch);
    emscripten_set_touchcancel_callback("#canvas", nullptr, EM_TRUE, onTouch);
    emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, EM_TRUE, onKeyDown);
    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, EM_TRUE, onResize);

    js_hide_splash();
    g_time = Util::getTime();
    emscripten_set_main_loop(frame, 0, 1);
    return 0;
}
