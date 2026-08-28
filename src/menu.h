// Port of com.bfs.papertoss.cpp.Menu
#pragma once

#include <string>

#include "leveldefs.h"
#include "sprite.h"
#include "vec.h"

class Menu {
public:
    Menu();

    void setSound(bool on);
    void setMenuButton(int level, const char* image_file, const v3f& pos, const v3f& score_pos,
                       const v4f& color);
    void activate();
    void deactivate();
    void create(const char* background, const char* scores_image, const v3f& scores_pos, bool sound);
    void destroy();
    void unDestroy();
    void setBest(int level, int score);
    void update(double elapsed);
    void render(const v2f& offset);
    void setNewLevel(int level);

    static const v4f GREYED_OUT_COLOR;
    static const v4f UNSELECTION_COLOR;

    bool m_sound_on = false;

private:
    void onPtrDown(const v2f& v);
    void onPtrUp(const v2f& v);

    std::string m_background_filename;
    int m_destroy_state = 0;
    v4f m_exit_color;
    float m_new_level_timer = 0.0f;
    v4f m_scores_color;
    std::string m_scores_filename;
    v3f m_scores_pos;
    v4f m_sound_color;

    Sprite* m_level_button[LevelDefs::NUM_LEVELS] = {};
    Sprite* m_level_score[LevelDefs::NUM_LEVELS] = {};
    std::string m_level_button_filenames[LevelDefs::NUM_LEVELS];
    v4f m_level_button_color[LevelDefs::NUM_LEVELS];
    v3f m_level_button_pos[LevelDefs::NUM_LEVELS];
    float m_level_button_scale[LevelDefs::NUM_LEVELS] = {};
    float m_level_button_delay[LevelDefs::NUM_LEVELS] = {};
    float m_level_button_time[LevelDefs::NUM_LEVELS] = {};
    v3f m_level_score_pos[LevelDefs::NUM_LEVELS];
    Sprite* m_background = nullptr;
    Sprite* m_scores = nullptr;
    Sprite* m_sound = nullptr;
    Sprite* m_exit = nullptr;
    int m_state = 0;
    int m_selected_level = -1;
    int m_new_level = -1;
};
