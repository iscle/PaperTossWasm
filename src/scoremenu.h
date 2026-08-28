// Port of com.bfs.papertoss.cpp.ScoreMenu
#pragma once

#include <string>

#include "leveldefs.h"
#include "sprite.h"
#include "vec.h"

class ScoreMenu {
public:
    ScoreMenu();

    void activate();
    void deactivate();
    void setLevel(int level, const v2i& pos, const v2f& size, const v2i& score_pos);
    void setBest(int level, int score);
    void create(const char* background, const v2i& back_pos, const v2f& back_size);
    void destroy();
    void unDestroy();
    void update(float elapsed);
    void render(const v2f& offset);

private:
    void onPtrUp(const v2f& v);

    std::string m_background_filename;
    Sprite* m_score[LevelDefs::NUM_LEVELS] = {};
    v2i m_back_pos;
    v2f m_back_size;
    v2i m_name_pos[LevelDefs::NUM_LEVELS];
    v2f m_name_size[LevelDefs::NUM_LEVELS];
    v2i m_score_pos[LevelDefs::NUM_LEVELS];
    Sprite* m_background = nullptr;
    int m_state = 0;
};
