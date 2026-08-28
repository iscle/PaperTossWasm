#include "level.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

#include "gl1.h"
#include "papertoss.h"

using LevelDefs::WindAnim;

namespace {

const float ARROW_OFFSET = 56.0f;
const float ARROW_SPEED = 3.0f;
const float BALL_CURVE = 240.0f;
const float BALL_SCALE_MIN = 0.034883723f;
const float BALL_START = 32.0f;
const float BALL_TOUCH_SCALE = 2.0f;
const float BALL_X_MOD = 0.9f;
const float BOUNCE_DUR = 0.125f;
const float DEFAULT_TOSS_HEIGHT = 140.0f;
const int FIREWORKS_START_SCORE = 3;
const float MAX_ANGLE = 75.0f;
const float MAX_WIND = 6.0f;
const float MIN_FLICK_DIST = 4.0f;
const int NO_COLLISION = -10000;
const int UNLOCKED_FONT_SIZE = 60;
const float UNLOCKED_SCALE = 5.0f;
const float WIND_POWER = 200.0f;
const float BALL_DUR = 1.5f;
const float HUD_UPDATE_DUR = 0.5f;
const float FLOAT_MAX = 3.4028235e38f;

const v2f DEFAULT_SCALE(1.0f, 1.0f);
const v3f DEFAULT_ROT(0.0f, 0.0f, 0.0f);
const v4f DEFAULT_COLOR(1.0f, 1.0f, 1.0f, 1.0f);
const v2f MENU_BTN_SIZE(48.0f, 48.0f);
const v4f HUD_BUTTON_COLOR(1.0f, 0.0f, 0.0f, 1.0f);
const v4f UNLOCKED_COLOR(1.0f, 1.0f, 0.0f, 1.0f);
const float UNLOCKED_DUR[] = {0.0f, BALL_DUR, BALL_DUR};
const v4f HUD_BOARD_COLOR(1.0f, 1.0f, 1.0f, 0.7f);
const v4f HUD_TEXT_COLOR(0.0f, 0.0f, 0.0f, 1.0f);
const int HUD_TEXT_OUTLINE = 0;

const v2i POP_ANIMATION[] = {
    v2i(-2, 13), v2i(-3, 27), v2i(-4, 41), v2i(-6, 45), v2i(-7, 49), v2i(-8, 53),
    v2i(-12, 55), v2i(-16, 57), v2i(-17, 53), v2i(-19, 49), v2i(-21, 45), v2i(-23, 31),
    v2i(-25, 15), v2i(-26, -2), v2i(-27, -21), v2i(-28, -41), v2i(-28, -61)};
const int POP_ANIMATION_COUNT = (int) (sizeof(POP_ANIMATION) / sizeof(POP_ANIMATION[0]));

const v2i BOUNCE_ANIMATION[] = {v2i(0, 1), v2i(0, 10), v2i(0, 18), v2i(0, 10), v2i(0, 1)};
const int BOUNCE_ANIMATION_COUNT = (int) (sizeof(BOUNCE_ANIMATION) / sizeof(BOUNCE_ANIMATION[0]));

}  // namespace

// ------------------------------------------------------ RenderInfoQueue

Level::RenderInfo* RenderInfoQueue::poll() {
    m_size--;
    Level::RenderInfo* result = m_first;
    m_first = m_first->next;
    return result;
}

void RenderInfoQueue::add(Level::RenderInfo* info) {
    m_size++;
    if (m_first == nullptr) {
        m_first = info;
    } else if (info->priority > m_first->priority) {
        info->next = m_first;
        m_first = info;
    } else {
        insert(info);
    }
}

void RenderInfoQueue::insert(Level::RenderInfo* info) {
    for (Level::RenderInfo* pointer = m_first; pointer != nullptr; pointer = pointer->next) {
        if (pointer->next == nullptr) {
            pointer->next = info;
            return;
        }
        if (pointer->priority >= info->priority && pointer->next->priority <= info->priority) {
            info->next = pointer->next;
            pointer->next = info;
            return;
        }
    }
}

// ---------------------------------------------------------------- Level

Level::Level() {
    m_active_wind = 0;
    m_bounce = false;
    m_swiping = false;
    m_last_shot_best = false;
    m_hud_menu_color = HUD_BUTTON_COLOR;
    for (int dir = 0; dir < 2; dir++) {
        for (int a = 0; a < 8; a++) {
            m_wind[dir][a] = nullptr;
            m_wind_scroll[dir][a] = v2f(0.0f, 0.0f);
            m_wind_scroll_target[dir][a] = v2f(0.0f, 0.0f);
        }
    }
    for (int fg = 0; fg < 2; fg++) m_foreground[fg] = nullptr;
    m_evt = &Evt::getInstance();
    m_evt->subscribe("onPtrDown", [this](const EvtArg& a) { onPtrDown(a.v); });
    m_evt->subscribe("onPtrMove", [this](const EvtArg& a) { onPtrMove(a.v); });
    m_evt->subscribe("onPtrUp", [this](const EvtArg& a) { onPtrUp(a.v); });
    m_evt->subscribe("onLevelUnlocked", [this](const EvtArg&) { onLevelUnlocked(); });
}

void Level::activate() { m_state = m_last_state; }

void Level::deactivate() {
    m_last_state = m_state;
    m_state = State::NONE;
}

Anim Level::createBounceAnim(const v3f& pos, float scale) {
    std::vector<v3f> animation((size_t) BOUNCE_ANIMATION_COUNT);
    for (int i = 0; i < BOUNCE_ANIMATION_COUNT; i++) {
        animation[i] = v3f((BOUNCE_ANIMATION[i].x * scale) + pos.x, (BOUNCE_ANIMATION[i].y * scale) + pos.y,
                           pos.z);
    }
    return Anim(animation, BOUNCE_ANIMATION_COUNT, BOUNCE_DUR);
}

int Level::collision(int axis, int pos, float size) {
    int delta = pos - axis;
    return ((float) std::abs(delta)) < size / BALL_TOUCH_SCALE ? delta : NO_COLLISION;
}

void Level::onPtrDown(const v2f& v) {
    if (m_state == State::INPUT) {
        if (m_ball != nullptr && m_ball->checkPoint(m_ball_pos, v, BALL_TOUCH_SCALE)) {
            if (USE_SWIPE) {
                m_swiping = true;
                m_swipe_time = (float) Util::getTime();
                m_move_last = v;
                m_move_accum = v2f(0.0f, 0.0f);
                m_move_count = 0;
            } else {
                m_ball_time = 0.0f;
                m_state = State::TOSS;
            }
        }
        if (m_hud_menu != nullptr && m_hud_menu->checkPoint(m_info->hud.menu_pos, v, BALL_DUR)) {
            m_hud_menu_color = HUD_BUTTON_COLOR.times(HUD_UPDATE_DUR);
        } else {
            m_hud_menu_color = HUD_BUTTON_COLOR;
        }
        return;
    }
    m_hud_menu_color = HUD_BUTTON_COLOR;
}

void Level::onPtrMove(const v2f& v) {
    if (m_state == State::INPUT) {
        if (m_move_count == 0) m_swipe_time = (float) Util::getTime();
        v2f delta = v.minus(m_move_last);
        m_move_accum.plusEquals(delta);
        m_move_count++;
    }
}

void Level::onPtrUp(const v2f& v) {
    if (m_state != State::NONE) {
        if (USE_SWIPE && m_swiping) {
            v2f dir;
            if (CONST_ROT == FLOAT_MAX) {
                dir = m_move_count != 0 ? m_move_accum.dividedBy((float) m_move_count) : v2f(0.0f, 0.0f);
            } else {
                v2f dir2(0.0f, UNLOCKED_SCALE);
                dir = dir2.rotated((((double) CONST_ROT) * 3.141592653589793) / 180.0);
            }
            if (dir.length() > MIN_FLICK_DIST && Util::getTime() - ((double) m_swipe_time) <= 1.0) {
                dir.normalize();
                m_ball_time = 0.0f;
                m_arrow_rot = -(Util::degrees(v2f::getNegativeRotation(dir)) + 90.0f);
                float a = v2f::dot(dir.times(BALL_CURVE), v2f(0.0f, 1.0f));
                m_ball_mid = (dir.x < 0.0f ? -1.0f : 1.0f) * ((float) std::sqrt(57600.0f - (a * a)));
                m_ball_end = (m_active_wind == 0 ? 1.0f : -1.0f) * ((WIND_POWER * m_wind_accel_rate) / MAX_WIND);
                m_state = State::TOSS;
            }
            m_swiping = false;
        } else if ((m_hud_menu != nullptr && m_hud_menu->checkPoint(m_info->hud.menu_pos, v, BALL_DUR)) ||
                   Sprite::pointInRect(v, v2f(m_info->hud.menu_pos.x, m_info->hud.menu_pos.y),
                                       MENU_BTN_SIZE)) {
            m_evt->publish("paperTossPlaySound", "Crumple.wav");
            m_evt->publish("gotoMenu");
        }
    }
    m_hud_menu_color = HUD_BUTTON_COLOR;
}

void Level::onLevelUnlocked() {
    if (m_state != State::NONE) {
        m_unlocked_time = 0.0f;
        m_unlocked_state = 1;
        m_evt->publish("paperTossPlaySound", "unlock.wav");
    }
}

void Level::setOrtho() {
    Papertoss::sizeGl();
    m_is_ortho = true;
}

void Level::setPerspective() {
    float h = 0.1f * ((float) std::tan(Util::radians(m_info->camera.fov * HUD_UPDATE_DUR)));
    float w = h * 0.6666667f;
    gl1::matrixMode(gl1::PROJECTION);
    gl1::loadIdentity();
    gl1::frustumf(-w, w, -h, h, 0.1f, 450.1f);
    gl1::matrixMode(gl1::MODELVIEW);
    gl1::loadIdentity();
    gl1::translatef(0.0f, -m_info->camera.height, -m_info->basket.distance);
    float r = 90.0f - Util::degrees((float) std::atan(m_info->basket.distance / m_info->camera.height));
    gl1::rotatef(r, 1.0f, 0.0f, 0.0f);
    m_is_ortho = false;
}

void Level::setWind() {
    if (CONST_SPEED >= 0.0f) {
        m_wind_accel_rate = CONST_SPEED;
    } else {
        m_wind_accel_rate = Random::randomf(0.0f, MAX_WIND);
    }
    if (CONST_DIR >= 0 && CONST_DIR <= 1) {
        m_active_wind = CONST_DIR;
    } else if (CONST_DIR == 2) {
        m_active_wind = (m_active_wind + 1) % 2;
    } else {
        m_active_wind = Random::randomi(0, 1);
    }
    float scroll_mod = m_wind_accel_rate / MAX_WIND;
    for (int dir = 0; dir < 2; dir++) {
        for (int a = 0; a < 8 && m_wind[dir][a] != nullptr; a++) {
            WindAnim& wi = m_info->wind[dir][a];
            v2f& target = m_wind_scroll_target[dir][a];
            target.x = wi.scroll.x * scroll_mod;
            target.y = wi.scroll.y;
            if (m_active_wind == 0) target.x = -target.x;
        }
    }
    Sprite::killSprite(m_wind_speed);
    m_wind_color = v4f(1.0f, 1.0f, 1.0f, 1.0f);
    m_wind_speed = new Sprite(20, -29, "fawn", Util::format("%.2f", m_wind_accel_rate), m_wind_color, 3);
    Sprite::killSprite(m_wind_arrow);
    m_wind_arrow = new Sprite("wind_arrow.png");
}

void Level::setScore() {
    int new_best = std::max(m_best, m_score);
    Sprite* score = new Sprite(24, -29, "fawn", "Score " + std::to_string(m_score), HUD_TEXT_COLOR,
                               HUD_TEXT_OUTLINE);
    Sprite* best = new Sprite(24, -29, "fawn", "Best " + std::to_string(new_best), HUD_TEXT_COLOR,
                              HUD_TEXT_OUTLINE);
    if (m_hud_score == nullptr || m_score == 0) {
        Sprite::killSprite(m_hud_score);
        m_hud_score = score;
    } else {
        Sprite::killSprite(m_hud_score_update);
        m_hud_score_update = score;
        m_hud_update_time = 0.0f;
    }
    if (m_hud_best == nullptr || m_score <= m_best) {
        Sprite::killSprite(m_hud_best);
        m_hud_best = best;
        return;
    }
    m_evt->publish("setBest", new_best);
    m_best = m_score;
    Sprite::killSprite(m_hud_best_update);
    m_hud_best_update = best;
    m_hud_update_time = 0.0f;
}

float Level::ballScale() { return 1.0f - (m_ball_pos.z / (m_info->basket.distance + ARROW_SPEED)); }

void Level::doCollision() {
    bool splash_left = false;
    bool splash_right = false;
    std::vector<v3f> animation((size_t) POP_ANIMATION_COUNT);
    float size = ballScale() * (m_ball != nullptr ? (float) m_ball->frameSize().x : 86.0f);
    bool in = m_ball_pos.x >= ((float) m_collision[0]) && m_ball_pos.x <= ((float) m_collision[1]);
    bool pop = m_ball_pos.y >= m_info->basket.pos.y + m_info->basket.height_offset;
    bool left = m_ball_pos.x <= m_info->basket.pos.x;
    bool col = !(collision(m_collision[0], (int) m_ball_pos.x, size) == NO_COLLISION &&
                 collision(m_collision[1], (int) m_ball_pos.x, size) == NO_COLLISION);
    if (col && pop) {
        float s = ballScale() * MIN_FLICK_DIST;
        if ((in && left) || (!in && !left)) {
            for (int i = 0; i < POP_ANIMATION_COUNT; i++) {
                animation[i] = v3f((-POP_ANIMATION[i].x) * s, POP_ANIMATION[i].y * s, 0.0f).plus(m_ball_pos);
            }
        } else {
            for (int i = 0; i < POP_ANIMATION_COUNT; i++) {
                animation[i] = v3f(POP_ANIMATION[i].x * s, POP_ANIMATION[i].y * s, 0.0f).plus(m_ball_pos);
            }
        }
        m_land_anim = Anim(animation, POP_ANIMATION_COUNT, 0.7f);
        m_evt->publish("paperTossPlaySound", in ? "RimIn.wav" : "RimOut.wav");
    } else {
        animation[0] = v3f(m_ball_pos);
        if (in) {
            animation[1] = v3f(m_info->basket.pos);
            animation[1].z = m_ball_pos.z;
            m_evt->publish("paperTossPlaySound", col ? "BounceIn.wav" : "In.wav");
        } else {
            float y = m_info->basket.pos.y - m_info->basket.base_offset;
            float dir = col ? 1.0f : -1.0f;
            float xoff = m_ball_dir.x * ((m_ball_pos.y - y) / m_ball_dir.y) * dir;
            animation[1] = v3f(m_ball_pos.x + xoff, y, m_ball_pos.z);
            // Kept verbatim from the decompiled original, including the branches
            // where the splash flags never get read.
            bool splash_is_on = m_info->splash.range.left > 0 || m_info->splash.range.right > 0;
            float distance = m_info->basket.pos.x - animation[1].x;
            if (distance <= 0.0f) {
                splash_left = false;
                if (distance < 0.0f) {
                    if (((-distance) < ((float) m_info->splash.range.right)) == m_info->splash.range.inside) {
                        splash_right = true;
                    }
                    if (splash_is_on) m_evt->publish("paperTossPlaySound", "Out.wav");
                } else {
                    splash_right = false;
                    if (splash_is_on || (!splash_left && !splash_right)) {
                        m_evt->publish("paperTossPlaySound", "Out.wav");
                    }
                }
            } else {
                if ((distance < ((float) m_info->splash.range.left)) == m_info->splash.range.inside) {
                    splash_left = true;
                }
            }
        }
        m_land_anim = Anim(animation, 2, 0.16666667f);
    }
    m_state = State::LAND;
    m_bounce = !in;
    if (in) {
        m_score++;
    } else {
        m_score = 0;
    }
}

void Level::create(LevelDefs::LevelInfo* lvl, int best, bool submit, int basket) {
    (void) submit;
    destroy();
    m_ball_time = -1.0f;
    m_info = lvl;
    if (Globals::HI_RES) {
        m_ball = new Sprite("paper_ball.png", v2i(141, 141), 1.0f, false, 16);
    } else {
        m_ball = new Sprite("paper_ball.png", v2i(86, 86), 1.0f, false, 16);
    }
    m_background = new Sprite(lvl->background_image);
    if (lvl->splash.image != nullptr) {
        m_splash = new Sprite(lvl->splash.image, lvl->splash.size, lvl->splash.duration);
    }
    for (int fg = 0; fg < 2 && lvl->foreground[fg].image != nullptr; fg++) {
        m_foreground[fg] = new Sprite(lvl->foreground[fg].image);
    }
    if (lvl->tutorial_image != nullptr) m_tutorial = new Sprite(lvl->tutorial_image);
    if (lvl->hud.image != nullptr) m_hud_board = new Sprite(lvl->hud.image);
    for (int dir = 0; dir < 2; dir++) {
        WindAnim* anims = lvl->wind[dir];
        for (int a = 0; a < 8 && anims[a].image != nullptr; a++) {
            WindAnim& w = anims[a];
            m_wind[dir][a] = new Sprite(w.image, w.size, w.duration, !w.scroll.equalsZero(), w.frame_count);
        }
    }
    m_offscreen_ct_left = 0;
    for (int i = 0; i < 4 && lvl->sounds.offscreen_left[i] != nullptr; i++) m_offscreen_ct_left++;
    m_offscreen_ct_right = 0;
    for (int i = 0; i < 4 && lvl->sounds.offscreen_right[i] != nullptr; i++) m_offscreen_ct_right++;
    m_basket = new Sprite(lvl->basket.image, lvl->basket.size);
    setBasket(basket);
    m_arrow = new Sprite("arrow.png");
    m_hud_level_unlocked =
        new Sprite(UNLOCKED_FONT_SIZE, -29, "fawn", "Level Unlocked", v4f(1.0f, 1.0f, 1.0f, 1.0f), 4);
    if (lvl->hud.show_menu) {
        m_hud_menu = new Sprite(24, -29, "fawn", "Menu", v4f(1.0f, 1.0f, 1.0f, 1.0f), HUD_TEXT_OUTLINE);
    }
    m_collision[0] = (int) (m_info->basket.pos.x - m_info->basket.half_width);
    m_collision[1] = (int) (m_info->basket.pos.x + m_info->basket.half_width);
    m_last_shot_best = false;
    m_wait_time = 0.0f;
    m_score = 0;
    m_best = best;
    m_ball_pos = v3f(m_info->basket.pos.x, BALL_START, 0.0f);
    m_ball_x_mod = 1.0f;
    m_arrow_time = 0.75f;
    m_arrow_rot = 0.0f;
    m_state = State::NONE;
    m_last_state = State::INPUT;
    setScore();
    setWind();
    setOrtho();
}

void Level::destroy() {
    m_destroy_state = m_state;
    m_destroy_last_state = m_last_state;
    for (int dir = 0; dir < 2; dir++) {
        for (int a = 0; a < 8; a++) {
            Sprite::killSprite(m_wind[dir][a]);
            m_wind[dir][a] = nullptr;
            m_wind_scroll[dir][a] = v2f(0.0f, 0.0f);
            m_wind_scroll_target[dir][a] = v2f(0.0f, 0.0f);
        }
    }
    for (int fg = 0; fg < 2; fg++) {
        Sprite::killSprite(m_foreground[fg]);
        m_foreground[fg] = nullptr;
    }
    Sprite::killSprite(m_background); m_background = nullptr;
    Sprite::killSprite(m_basket); m_basket = nullptr;
    Sprite::killSprite(m_ball); m_ball = nullptr;
    Sprite::killSprite(m_splash); m_splash = nullptr;
    Sprite::killSprite(m_tutorial); m_tutorial = nullptr;
    Sprite::killSprite(m_arrow); m_arrow = nullptr;
    Sprite::killSprite(m_wind_speed); m_wind_speed = nullptr;
    Sprite::killSprite(m_wind_arrow); m_wind_arrow = nullptr;
    Sprite::killSprite(m_hud_board); m_hud_board = nullptr;
    Sprite::killSprite(m_hud_menu); m_hud_menu = nullptr;
    Sprite::killSprite(m_hud_score); m_hud_score = nullptr;
    Sprite::killSprite(m_hud_best); m_hud_best = nullptr;
    Sprite::killSprite(m_hud_score_update); m_hud_score_update = nullptr;
    Sprite::killSprite(m_hud_best_update); m_hud_best_update = nullptr;
    Sprite::killSprite(m_hud_level_unlocked); m_hud_level_unlocked = nullptr;
}

void Level::unDestroy() {
    if (m_info == nullptr) return;
    m_state = m_destroy_state;
    m_last_state = m_destroy_last_state;
    for (int dir = 0; dir < 2; dir++) {
        WindAnim* anims = m_info->wind[dir];
        for (int a = 0; a < 8 && anims[a].image != nullptr; a++) {
            WindAnim& w = anims[a];
            m_wind[dir][a] = new Sprite(w.image, w.size, w.duration, !w.scroll.equalsZero(), w.frame_count);
        }
    }
    for (int fg = 0; fg < 2 && m_info->foreground[fg].image != nullptr; fg++) {
        m_foreground[fg] = new Sprite(m_info->foreground[fg].image);
    }
    m_background = new Sprite(m_info->background_image);
    m_basket = new Sprite(m_info->basket.image, m_info->basket.size);
    if (Globals::HI_RES) {
        m_ball = new Sprite("paper_ball.png", v2i(141, 141), 1.0f, false, 16);
    } else {
        m_ball = new Sprite("paper_ball.png", v2i(86, 86), 1.0f, false, 16);
    }
    if (m_info->splash.image != nullptr) {
        m_splash = new Sprite(m_info->splash.image, m_info->splash.size, m_info->splash.duration);
    }
    if (m_info->tutorial_image != nullptr) m_tutorial = new Sprite(m_info->tutorial_image);
    m_arrow = new Sprite("arrow.png");
    m_wind_speed = new Sprite(20, -29, "fawn", Util::format("%.2f", m_wind_accel_rate), m_wind_color, 3);
    m_wind_arrow = new Sprite("wind_arrow.png");
    if (m_info->hud.image != nullptr) m_hud_board = new Sprite(m_info->hud.image);
    if (m_info->hud.show_menu) {
        m_hud_menu = new Sprite(24, -29, "fawn", "Menu", v4f(1.0f, 1.0f, 1.0f, 1.0f), HUD_TEXT_OUTLINE);
    }
    setScore();
    m_hud_level_unlocked =
        new Sprite(UNLOCKED_FONT_SIZE, -29, "fawn", "Level Unlocked", v4f(1.0f, 1.0f, 1.0f, 1.0f), 4);
}

void Level::update(float elapsed) {
    bool splash_left = false;
    bool splash_right = false;
    float x;
    float i;
    Evt& evt = Evt::getInstance();
    for (int dir = 0; dir < 2; dir++) {
        for (int a = 0; a < 8 && m_wind[dir][a] != nullptr; a++) {
            v2f& scroll = m_wind_scroll[dir][a];
            v2f& target = m_wind_scroll_target[dir][a];
            WindAnim& wi = m_info->wind[dir][a];
            Sprite* w = m_wind[dir][a];
            if (wi.scroll.x != 0.0f) {
                float speed_delta = std::abs(wi.scroll.x);
                if (scroll.x < target.x) {
                    scroll.x = std::min(scroll.x + (speed_delta * elapsed), target.x);
                } else if (scroll.x > target.x) {
                    scroll.x = std::max(scroll.x - (speed_delta * elapsed), target.x);
                }
                float speed_delta2 = std::abs(wi.scroll.x);
                if (scroll.y < target.y) {
                    scroll.y = std::min(scroll.y + (speed_delta2 * elapsed), target.y);
                } else if (scroll.y > target.y) {
                    scroll.y = std::max(scroll.y - (speed_delta2 * elapsed), target.y);
                }
                w->setScroll(scroll);
            } else {
                w->setScroll(target);
            }
            w->update(elapsed);
        }
    }
    if (m_hud_score_update != nullptr || m_hud_best_update != nullptr) {
        m_hud_update_time += elapsed;
        float score_i = std::min(m_hud_update_time / HUD_UPDATE_DUR, 1.0f);
        if (score_i == 1.0f) {
            if (m_hud_score_update != nullptr) {
                Sprite::killSprite(m_hud_score);
                m_hud_score = m_hud_score_update;
                m_hud_score_update = nullptr;
            }
            if (m_hud_best_update != nullptr) {
                Sprite::killSprite(m_hud_best);
                m_hud_best = m_hud_best_update;
                m_hud_best_update = nullptr;
            }
        }
    }
    if (m_unlocked_state != 0) {
        m_unlocked_time += elapsed;
        if (m_unlocked_time > UNLOCKED_DUR[m_unlocked_state]) {
            m_unlocked_time -= UNLOCKED_DUR[m_unlocked_state];
            m_unlocked_state++;
            if (m_unlocked_state > 2) m_unlocked_state = 0;
        }
    }
    switch (m_state) {
        case State::INPUT:
            m_ball_x_mod = 1.0f;
            m_arrow_time = std::fmod(m_arrow_time + elapsed, ARROW_SPEED);
            if (CONST_ROT != FLOAT_MAX) {
                m_arrow_rot = CONST_ROT;
            } else if (!USE_SWIPE) {
                float i2 = ((m_arrow_time / ARROW_SPEED) - HUD_UPDATE_DUR) * MIN_FLICK_DIST;
                m_arrow_rot = (i2 < 0.0f ? 1.0f + i2 : 1.0f - i2) * MAX_ANGLE;
            }
            return;
        case State::TOSS: {
            if (m_tutorial != nullptr) {
                Sprite::killSprite(m_tutorial);
                Evt::getInstance().publish("onTutorialShown");
                m_tutorial = nullptr;
            }
            m_ball->update(elapsed);
            m_ball_time += elapsed;
            float i3 = m_ball_time / BALL_DUR;
            if (i3 >= 1.0f) {
                i3 = 1.0f;
                m_ball_time = -1.0f;
            }
            if (i3 < HUD_UPDATE_DUR) {
                m_ball_x_mod = BALL_X_MOD;
            } else {
                float j = (BALL_TOUCH_SCALE * i3) - 1.0f;
                m_ball_x_mod = 1.0f + ((1.0f - (j * j)) * (-0.100000024f));
            }
            v3f old_ball_pos(m_ball_pos);
            bool wrong_dir = (m_ball_mid > 0.0f && m_ball_end > 0.0f) || (m_ball_mid < 0.0f && m_ball_end < 0.0f);
            if (wrong_dir) {
                x = i3 * (m_ball_mid + m_ball_end);
            } else if (i3 < HUD_UPDATE_DUR) {
                float j2 = (BALL_TOUCH_SCALE * i3) - 1.0f;
                x = (1.0f - (j2 * j2)) * m_ball_mid;
            } else {
                float j3 = (i3 - HUD_UPDATE_DUR) * BALL_TOUCH_SCALE;
                x = (m_ball_mid + m_ball_end) - ((1.0f - (j3 * j3)) * m_ball_end);
            }
            m_ball_pos.x = 160.0f + x;
            float basket_y = m_info->basket.pos.y + m_info->basket.height_offset;
            float toss_height = m_info->toss_height != 0 ? (float) m_info->toss_height : DEFAULT_TOSS_HEIGHT;
            m_ball_pos.z = (m_info->basket.distance * i3) + 0.01f;
            if (i3 < HUD_UPDATE_DUR) {
                i = (i3 - HUD_UPDATE_DUR) * BALL_TOUCH_SCALE;
                m_ball_pos.y = ((1.0f - (i * i)) * ((basket_y - BALL_START) + toss_height)) + BALL_START;
            } else {
                i = (i3 - HUD_UPDATE_DUR) * BALL_TOUCH_SCALE;
                m_ball_pos.y = ((1.0f - (i * i)) * toss_height) + basket_y;
            }
            if (i < 1.0f) {
                m_ball_dir = m_ball_pos.minus(old_ball_pos);
            } else {
                doCollision();
            }
            return;
        }
        case State::LAND:
            m_ball_pos = m_land_anim.get(elapsed);
            if (m_land_anim.isDone()) {
                if (m_bounce) {
                    m_bounce = false;
                    bool splash_is_on = m_info->splash.range.left > 0 || m_info->splash.range.right > 0;
                    float distance = m_info->basket.pos.x - m_ball_pos.x;
                    if (distance <= 0.0f) {
                        splash_left = false;
                    } else if ((distance < ((float) m_info->splash.range.left)) == m_info->splash.range.inside) {
                        splash_left = true;
                    }
                    if (distance >= 0.0f) {
                        splash_right = false;
                    } else if (((-distance) < ((float) m_info->splash.range.right)) ==
                               m_info->splash.range.inside) {
                        splash_right = true;
                    }
                    bool on_screen = m_ball_pos.x >= -32.0f && m_ball_pos.x <= 352.0f;
                    if (splash_is_on && on_screen && (splash_left || splash_right)) {
                        m_splash->setFrame(0);
                        m_wait_time = m_info->splash.duration;
                        m_state = State::SPLASH;
                        Evt::getInstance().publish("paperTossPlaySound", m_info->sounds.splash);
                        return;
                    }
                    m_land_anim = createBounceAnim(m_ball_pos, ballScale());
                    return;
                }
                m_wait_time = HUD_UPDATE_DUR;
                m_state = State::WAIT;
                Globals::texture_mgr->cleanup();
            }
            return;
        case State::SPLASH:
            m_splash->update(elapsed);
            m_wait_time -= elapsed;
            if (m_wait_time <= 0.0f) {
                m_state = State::WAIT;
            } else {
                return;
            }
            break;
        case State::WAIT:
            break;
        default:
            return;
    }
    m_wait_time -= elapsed;
    if (m_wait_time <= 0.0f) {
        bool missed_shot = m_score == 0;
        if (!missed_shot) {
            if (m_score >= m_best) {
                if (m_info->use_fireworks && m_score >= FIREWORKS_START_SCORE) {
                    evt.publish("startFireworks");
                } else {
                    evt.publish("paperTossPlaySound", "Applause.wav");
                }
                m_last_shot_best = true;
            } else if (m_info->use_fireworks) {
                evt.publish("paperTossPlaySound", "Applause.wav");
            }
        } else if (m_last_shot_best) {
            evt.publish("paperTossPlaySound", "Aww.wav");
            m_last_shot_best = false;
        } else if (m_ball_pos.x < -32.0f) {
            if (m_offscreen_ct_left > 0) {
                evt.publish("paperTossPlaySound",
                            m_info->sounds.offscreen_left[Random::randomi(0, m_offscreen_ct_left - 1)]);
            }
        } else if (m_ball_pos.x > 352.0f && m_offscreen_ct_right > 0) {
            evt.publish("paperTossPlaySound",
                        m_info->sounds.offscreen_right[Random::randomi(0, m_offscreen_ct_right - 1)]);
        }
        m_wait_time = 0.0f;
        m_ball_pos = v3f(m_info->basket.pos.x, BALL_START, 0.0f);
        setScore();
        setWind();
        m_arrow_time = 0.75f;
        m_arrow_rot = 0.0f;
        m_state = State::INPUT;
        Globals::texture_mgr->cleanup();
    }
}

void Level::render(const v2f& offset) {
    if (m_background == nullptr) return;

    m_render_infos.clear();
    RenderInfoQueue render_queue;
    auto queue = [&](Sprite* sprite, const v3f& pos, const v2f& scale = v2f(1.0f, 1.0f),
                     const v3f& rot = v3f(0.0f, 0.0f, 0.0f), const v4f& color = v4f(1.0f, 1.0f, 1.0f, 1.0f),
                     float priority = -1.0f, bool ortho = true) {
        m_render_infos.emplace_back();
        RenderInfo& ri = m_render_infos.back();
        ri.sprite = sprite;
        ri.pos = pos;
        ri.scale = scale;
        ri.rot = rot;
        ri.color = color;
        ri.priority = priority == -1.0f ? pos.z : priority;
        ri.ortho = ortho;
        ri.next = nullptr;
        render_queue.add(&ri);
    };

    v3f o(offset.x, offset.y, 0.0f);
    v2f s = DEFAULT_SCALE;
    v3f r = DEFAULT_ROT;
    queue(m_background, v3f(160.0f, BALL_CURVE, Config::BACKGROUND_DEPTH));
    if (!USE_SWIPE) {
        if (m_state == State::INPUT) {
            v2f arrow_dir = v2f(0.0f, ARROW_OFFSET).rotated(Util::radians(m_arrow_rot));
            v3f arrow_pos(arrow_dir.x + m_info->basket.pos.x, arrow_dir.y + BALL_START, 0.02f);
            queue(m_arrow, arrow_pos, s, v3f(0.0f, 0.0f, m_arrow_rot));
        }
    } else {
        v2f arrow_dir2 = v2f(0.0f, ARROW_OFFSET).rotated(Util::radians(m_arrow_rot));
        v3f arrow_pos2(arrow_dir2.x + m_info->basket.pos.x, arrow_dir2.y + BALL_START, 0.02f);
        queue(m_arrow, arrow_pos2, s, v3f(0.0f, 0.0f, m_arrow_rot));
    }
    for (int a = 0; a < 8 && m_wind[m_active_wind][a] != nullptr; a++) {
        WindAnim& wi = m_info->wind[m_active_wind][a];
        if (wi.scroll.equalsZero()) {
            queue(m_wind[m_active_wind][a], wi.pos, wi.scale);
        } else if (wi.pos.z < Config::BACKGROUND_DEPTH) {
            v2f& scroll = m_wind_scroll[m_active_wind][a];
            float alpha_i = wi.scroll.x != 0.0f ? std::abs(scroll.x) / wi.scroll.x : m_wind_accel_rate / MAX_WIND;
            float alpha = wi.alpha_range.x + ((wi.alpha_range.y - wi.alpha_range.x) * alpha_i);
            if (wi.scroll.x == 0.0f) {
                r.z = 60.0f * (m_wind_accel_rate / MAX_WIND);
                if (m_active_wind == 1) r.z = -r.z;
            }
            queue(m_wind[m_active_wind][a], wi.pos, wi.scale, r, v4f(1.0f, 1.0f, 1.0f, alpha));
        } else {
            queue(m_wind[m_active_wind][a], wi.pos, wi.scale);
        }
    }
    queue(m_basket, m_info->basket.pos);
    for (int fg = 0; fg < 2 && m_foreground[fg] != nullptr; fg++) {
        queue(m_foreground[fg], m_info->foreground[fg].pos);
    }
    if (m_tutorial != nullptr) queue(m_tutorial, v3f(160.0f, BALL_CURVE, 0.0f));
    if (m_hud_board != nullptr) queue(m_hud_board, m_info->hud.pos, s, r, HUD_BOARD_COLOR);
    if (m_hud_level_unlocked != nullptr && m_unlocked_state != 0) {
        v3f up(160.0f, BALL_CURVE, 0.0f);
        v4f uc = UNLOCKED_COLOR;
        float i = m_unlocked_time / UNLOCKED_DUR[m_unlocked_state];
        if (m_unlocked_state == 1) {
            float f = ((1.0f - i) * UNLOCKED_SCALE) + 1.0f;
            s.y = f;
            s.x = f;
            r.z = (1.0f - i) * (-90.0f);
            uc.w = i;
        } else {
            uc.w = 1.0f - i;
        }
        queue(m_hud_level_unlocked, up, s, r, uc);
    }
    if (m_hud_score_update != nullptr || m_hud_best_update != nullptr) {
        float i2 = std::min(m_hud_update_time / HUD_UPDATE_DUR, 1.0f);
        float si = ((1.0f - i2) * 10.0f) + 1.0f;
        v2f sv(si, si);
        v4f c(1.0f, 1.0f, 1.0f, i2);
        if (m_hud_score_update != nullptr) {
            queue(m_hud_score_update, m_info->hud.score_pos, sv, m_info->hud.score_rot, c);
        }
        if (m_hud_best_update != nullptr) {
            queue(m_hud_best_update, m_info->hud.best_pos, sv, m_info->hud.best_rot, c);
        }
    }
    if (m_hud_menu != nullptr) {
        queue(m_hud_menu, m_info->hud.menu_pos, s, m_info->hud.menu_rot, m_hud_menu_color);
    }
    queue(m_hud_score, m_info->hud.score_pos, s, m_info->hud.score_rot);
    queue(m_hud_best, m_info->hud.best_pos, s, m_info->hud.best_rot);
    float bs = ballScale();
    v3f ball_pos(m_ball_pos);
    ball_pos.x = ((ball_pos.x - 160.0f) * m_ball_x_mod) + 160.0f;
    if (m_state == State::SPLASH) {
        float ss = std::max(m_info->splash.scale * bs, BALL_SCALE_MIN);
        ball_pos.z = 449.88998f;
        queue(m_splash, ball_pos, v2f(ss, ss));
    } else {
        float ss2 = std::max(bs, BALL_SCALE_MIN);
        queue(m_ball, ball_pos, v2f(ss2, ss2));
    }
    if (m_state != State::NONE && m_wind_speed != nullptr) {
        v2i size = m_wind_speed->m_texture->m_text_size;
        v2f scale(m_info->wind_speed.scale.x / (float) size.x, m_info->wind_speed.scale.y / (float) size.y);
        float priority = m_info->wind_speed.depth != 0.0f ? m_info->wind_speed.depth
                                                          : m_info->basket.pos.z - 0.01f;
        queue(m_wind_speed, m_info->wind_speed.number_pos, scale, v3f(-90.0f, 0.0f, 0.0f),
              m_info->wind_speed.color, priority, false);
        queue(m_wind_arrow, m_info->wind_speed.arrow_pos, scale,
              v3f(-90.0f, m_active_wind != 0 ? 180.0f : 0.0f, 0.0f), m_info->wind_speed.color, priority,
              false);
    }
    while (render_queue.size() > 0) {
        RenderInfo* ri = render_queue.poll();
        if (ri->sprite == nullptr) continue;
        if (ri->ortho && !m_is_ortho) {
            setOrtho();
        } else if (!ri->ortho && m_is_ortho) {
            setPerspective();
        }
        ri->pos = ri->pos.plus(o);
        ri->sprite->draw(ri->pos, ri->scale, ri->rot, ri->color);
    }
}

void Level::setBasket(int basket) {
    if (m_basket != nullptr) m_basket->setFrame(basket);
}
