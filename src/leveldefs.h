// Port of com.bfs.papertoss.cpp.LevelDefs
#pragma once

#include "vec.h"

namespace LevelDefs {

constexpr int AIRPORT = 3;
constexpr int BASEMENT = 4;
constexpr int BATHROOM = 5;
constexpr int EASY = 0;
constexpr int HARD = 2;
constexpr int LEFT = 0;
constexpr int MAX_ANIMS = 8;
constexpr int MAX_FOREGROUNDS = 2;
constexpr int MAX_OFF_SOUNDS = 4;
constexpr int MEDIUM = 1;
constexpr int NUM_DIRS = 2;
constexpr int RIGHT = 1;
constexpr int NUM_LEVELS = 6;

struct BasketInfo {
    float base_offset = 0.0f, distance = 0.0f, half_width = 0.0f, height_offset = 0.0f;
    const char* image = nullptr;
    v3f pos;
    v2i size;
};

struct ButtonInfo {
    const char* image = nullptr;
    v2i pos;
    v2f size;
};

struct CameraInfo {
    float fov = 0.0f, height = 0.0f;
};

struct ForegroundInfo {
    const char* image = nullptr;
    v3f pos;
};

struct HudInfo {
    const char* image = nullptr;
    bool show_menu = false;
    v3f pos, score_rot, score_pos, best_rot, best_pos, submit_rot, submit_pos, menu_rot, menu_pos;
};

struct MenuLevelInfo {
    const char* image = nullptr;
    const char* level_name = nullptr;
    v3f pos, score_pos;
    int score_to_unlock_next = 0;
};

struct ScoreMenuLevelInfo {
    v2i pos;
    v2f size;
    v2i score_pos;
};

struct SoundInfo {
    const char* loop = nullptr;
    const char* offscreen_left[MAX_OFF_SOUNDS] = {nullptr, nullptr, nullptr, nullptr};
    const char* offscreen_right[MAX_OFF_SOUNDS] = {nullptr, nullptr, nullptr, nullptr};
    const char* splash = nullptr;
};

struct SplashAnimRange {
    bool inside = false;
    int left = 0, right = 0;
};

struct SplashAnimInfo {
    float duration = 0.0f;
    const char* image = nullptr;
    float scale = 0.0f;
    v2i size;
    SplashAnimRange range;
};

struct WindAnim {
    float duration = 0.0f;
    int frame_count = 0;
    const char* image = nullptr;
    v3f pos;
    v2i size;
    v2f scale, scroll, alpha_range;
};

struct WindSpeedInfo {
    float depth = 0.0f;
    v3f number_pos, arrow_pos;
    v2f scale;
    v4f color;
};

struct LevelInfo {
    const char* background_image = nullptr;
    int toss_height = 0;
    const char* tutorial_image = nullptr;
    bool use_fireworks = false;
    MenuLevelInfo menu_info;
    ScoreMenuLevelInfo score_menu_info;
    HudInfo hud;
    BasketInfo basket;
    ForegroundInfo foreground[MAX_FOREGROUNDS];
    WindAnim wind[NUM_DIRS][MAX_ANIMS];
    WindSpeedInfo wind_speed;
    CameraInfo camera;
    SoundInfo sounds;
    SplashAnimInfo splash;
};

struct MenuInfo {
    const char* image = nullptr;
    ButtonInfo score_button;
};

struct ScoreMenuInfo {
    ButtonInfo back_button;
    const char* image = nullptr;
};

extern MenuInfo menu_info;
extern ScoreMenuInfo score_menu_info;
extern LevelInfo level_info[NUM_LEVELS];

void initializeData();

}  // namespace LevelDefs
