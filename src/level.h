// Port of com.bfs.papertoss.cpp.Level (and RenderInfo / RenderInfoQueue)
#pragma once

#include <deque>

#include "anim.h"
#include "leveldefs.h"
#include "platform.h"
#include "sprite.h"
#include "vec.h"

class Level {
public:
    Level();

    void activate();
    void deactivate();
    void create(LevelDefs::LevelInfo* lvl, int best, bool submit, int basket);
    void destroy();
    void unDestroy();
    void update(float elapsed);
    void render(const v2f& offset);
    void setBasket(int basket);

    struct RenderInfo {
        Sprite* sprite = nullptr;
        v3f pos;
        v2f scale;
        v3f rot;
        v4f color;
        float priority = 0.0f;
        bool ortho = true;
        RenderInfo* next = nullptr;
    };

private:
    enum class State { NONE, INPUT, TOSS, LAND, SPLASH, WAIT };

    void setOrtho();
    void setPerspective();
    void setWind();
    void setScore();
    float ballScale();
    void doCollision();
    static Anim createBounceAnim(const v3f& pos, float scale);
    static int collision(int axis, int pos, float size);

    void onPtrDown(const v2f& v);
    void onPtrMove(const v2f& v);
    void onPtrUp(const v2f& v);
    void onLevelUnlocked();

    int m_active_wind = 0;
    v3f m_ball_dir;
    float m_ball_end = 0.0f;
    float m_ball_mid = 0.0f;
    v3f m_ball_pos;
    float m_ball_x_mod = 1.0f;
    bool m_bounce = false;
    State m_destroy_state = State::NONE;
    State m_destroy_last_state = State::NONE;
    Evt* m_evt = nullptr;
    LevelDefs::LevelInfo* m_info = nullptr;
    Anim m_land_anim;
    bool m_last_shot_best = false;
    bool m_swiping = false;
    float m_wait_time = 0.0f;
    v4f m_wind_color;

    Sprite* m_wind[2][8] = {};
    Sprite* m_foreground[2] = {};
    int m_collision[2] = {0, 0};
    v2f m_wind_scroll[2][8];
    v2f m_wind_scroll_target[2][8];
    bool m_is_ortho = true;
    bool USE_SWIPE = true;
    float CONST_ROT = 3.4028235e38f;  // Float.MAX_VALUE sentinel
    float CONST_SPEED = -1.0f;
    int CONST_DIR = -1;
    Sprite* m_background = nullptr;
    Sprite* m_basket = nullptr;
    Sprite* m_ball = nullptr;
    Sprite* m_splash = nullptr;
    Sprite* m_tutorial = nullptr;
    Sprite* m_arrow = nullptr;
    Sprite* m_wind_speed = nullptr;
    Sprite* m_wind_arrow = nullptr;
    Sprite* m_hud_board = nullptr;
    Sprite* m_hud_menu = nullptr;
    Sprite* m_hud_score = nullptr;
    Sprite* m_hud_score_update = nullptr;
    Sprite* m_hud_best = nullptr;
    Sprite* m_hud_best_update = nullptr;
    Sprite* m_hud_level_unlocked = nullptr;
    v4f m_hud_menu_color;
    float m_ball_time = -1.0f;
    float m_arrow_rot = 0.0f;
    float m_arrow_time = 0.0f;
    float m_wind_accel_rate = 1.0f;
    State m_state = State::NONE;
    State m_last_state = State::NONE;
    int m_unlocked_state = 0;
    float m_unlocked_time = 0.0f;
    int m_score = 0;
    int m_best = 0;
    float m_swipe_time = 0.0f;
    v2f m_move_last;
    v2f m_move_accum;
    int m_move_count = 0;
    float m_hud_update_time = 0.0f;
    int m_offscreen_ct_left = 0;
    int m_offscreen_ct_right = 0;

    // Per-frame storage for the render queue nodes (Java allocated these and
    // let the GC clean up).
    std::deque<RenderInfo> m_render_infos;
};

// Priority-ordered singly linked list, back-to-front.
class RenderInfoQueue {
public:
    int size() const { return m_size; }
    Level::RenderInfo* poll();
    void add(Level::RenderInfo* info);

private:
    void insert(Level::RenderInfo* info);
    Level::RenderInfo* m_first = nullptr;
    int m_size = 0;
};
