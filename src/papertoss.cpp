#include "papertoss.h"

#include <algorithm>

#include "gl1.h"
#include "level.h"
#include "leveldefs.h"
#include "menu.h"
#include "platform.h"
#include "scoremenu.h"
#include "scores.h"
#include "vec.h"

namespace Papertoss {

namespace {

const float TRANSITION_SPEED = 960.0f;

Menu* menu = nullptr;
ScoreMenu* score_menu = nullptr;
int current_level = -1;
v2f offset(0.0f, 0.0f);
int highest_level = 0;
const float quad_verts[] = {-0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f};

void unlockLevel(int i) {
    if (i < LevelDefs::NUM_LEVELS) {
        menu->setMenuButton(i, LevelDefs::level_info[i].menu_info.image,
                            LevelDefs::level_info[i].menu_info.pos,
                            LevelDefs::level_info[i].menu_info.score_pos, Menu::UNSELECTION_COLOR);
        highest_level = std::max(highest_level, i);
    }
}

void onSetBest(const EvtArg& arg) {
    int score = arg.i;
    menu->setBest(current_level, score);
    score_menu->setBest(current_level, score);
    Scores::saveBest(score, current_level);
    if (score == LevelDefs::level_info[current_level].menu_info.score_to_unlock_next) {
        int new_level = current_level + 1;
        unlockLevel(new_level);
        menu->setNewLevel(new_level);
        LevelDefs::ScoreMenuLevelInfo& si = LevelDefs::level_info[new_level].score_menu_info;
        score_menu->setLevel(new_level, si.pos, si.size, si.score_pos);
        score_menu->setBest(new_level, 0);
        Evt::getInstance().publish("onLevelUnlocked");
        int basket = (highest_level - current_level) + 1;
        level->setBasket(basket);
    }
}

void onSetSound(const EvtArg& arg) {
    SaveData::write(arg.b, "sound");
    SaveData::save();
}

void onPostScore(const EvtArg&) {
    Scores::saveSubmitted(Scores::readBest(current_level), current_level);
}

void onTutorialShown(const EvtArg&) {
    SaveData::write(true, "tutorial_shown");
    SaveData::save();
    LevelDefs::level_info[0].tutorial_image = nullptr;
}

void onGotoScores(const EvtArg&) {
    if (state == GameState::MENU) {
        state = GameState::MENU_TO_SCORE;
        offset = v2f(0.0f, -1.0f);
        menu->deactivate();
    }
}

void onGotoMenu(const EvtArg&) {
    Evt& evt = Evt::getInstance();
    evt.publish("setBackgroundSound", "OfficeNoise.mp3");
    if (state == GameState::LEVEL || state == GameState::SCORE) {
        if (state == GameState::LEVEL) {
            level->deactivate();
            state = GameState::LEVEL_TO_MENU;
        } else if (state == GameState::SCORE) {
            score_menu->deactivate();
            state = GameState::SCORE_TO_MENU;
        }
        offset = v2f(-320.0f, -1.0f);
        if (Globals::HI_RES) offset = v2f(-294.25f, -1.0f);
    }
}

void onGotoLevel(const EvtArg& arg) {
    int lvl = arg.i;
    if (state != GameState::MENU) return;
    state = GameState::MENU_TO_LEVEL;
    offset = v2f(0.0f, -1.0f);
    menu->deactivate();
    if (lvl != current_level) {
        if (current_level != -1) level->destroy();
        current_level = lvl;
        int best = Scores::readBest(current_level);
        int submitted = Scores::readSubmitted(current_level);
        int basket = (highest_level - current_level) + 1;
        level->create(&LevelDefs::level_info[current_level], best, best > submitted, basket);
    }
    Evt::getInstance().publish("setBackgroundSound", LevelDefs::level_info[lvl].sounds.loop);
}

}  // namespace

GameState state = GameState::MENU;
Level* level = nullptr;
bool m_shutdown = false;

void sizeGl() {
    gl1::matrixMode(gl1::PROJECTION);
    gl1::loadIdentity();
    if (Globals::HI_RES) {
        gl1::orthof(Config::ORTHO_ADJUSTMENT_F, 307.12643f, 0.0f, 480.0f, 0.0f, 450.0f);
    } else {
        gl1::orthof(0.0f, 320.0f, 0.0f, 480.0f, 0.0f, 450.0f);
    }
    gl1::matrixMode(gl1::MODELVIEW);
    gl1::loadIdentity();
}

void update(double elapsed) {
    if (m_shutdown) return;
    switch (state) {
        case GameState::MENU:
            menu->update(elapsed);
            break;
        case GameState::LEVEL:
            level->update((float) elapsed);
            break;
        case GameState::SCORE:
            score_menu->update((float) elapsed);
            break;
        case GameState::MENU_TO_SCORE:
        case GameState::MENU_TO_LEVEL:
            if (offset.y == -1.0f) {
                offset.y = 0.0f;
            } else {
                offset.x = (float) (((double) offset.x) - (TRANSITION_SPEED * elapsed));
            }
            if ((offset.x <= -320.0f && !Globals::HI_RES) || (offset.x <= -294.25f && Globals::HI_RES)) {
                if (state == GameState::MENU_TO_SCORE) {
                    state = GameState::SCORE;
                    Globals::texture_mgr->cleanup();
                    score_menu->activate();
                } else if (state == GameState::MENU_TO_LEVEL) {
                    state = GameState::LEVEL;
                    Globals::texture_mgr->cleanup();
                    level->activate();
                }
            }
            break;
        case GameState::SCORE_TO_MENU:
        case GameState::LEVEL_TO_MENU:
            if (offset.y == -1.0f) {
                offset.y = 0.0f;
            } else {
                offset.x = (float) (((double) offset.x) + (TRANSITION_SPEED * elapsed));
            }
            if (offset.x >= 0.0f) {
                state = GameState::MENU;
                Globals::texture_mgr->cleanup();
                menu->activate();
            }
            break;
    }
}

void render() {
    if (m_shutdown) return;
    switch (state) {
        case GameState::MENU:
            menu->render(v2f(0.0f, 0.0f));
            break;
        case GameState::LEVEL:
            level->render(v2f(0.0f, 0.0f));
            break;
        case GameState::SCORE:
            score_menu->render(v2f(0.0f, 0.0f));
            break;
        case GameState::MENU_TO_SCORE:
        case GameState::SCORE_TO_MENU: {
            float ortho_width = Globals::HI_RES ? Config::ADJUSTED_ORTHO_WIDTH : 320.0f;
            menu->render(offset);
            score_menu->render(offset.plus(v2f(ortho_width, 0.0f)));
            break;
        }
        case GameState::MENU_TO_LEVEL:
        case GameState::LEVEL_TO_MENU: {
            float ortho_width2 = Globals::HI_RES ? Config::ADJUSTED_ORTHO_WIDTH : 320.0f;
            menu->render(offset);
            level->render(offset.plus(v2f(ortho_width2, 0.0f)));
            break;
        }
    }
}

void shutdown() {
    m_shutdown = true;
    if (level != nullptr) level->destroy();
    if (score_menu != nullptr) score_menu->destroy();
    if (menu != nullptr) menu->destroy();
    Globals::texture_mgr->cleanup();
}

void unShutdown() {
    if (!m_shutdown) return;
    if (menu != nullptr) menu->unDestroy();
    if (score_menu != nullptr) score_menu->unDestroy();
    if (level != nullptr) level->unDestroy();
    m_shutdown = false;
}

bool initialize() {
    glCullFace(GL_BACK);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    gl1::loadIdentity();
    gl1::color4f(1.0f, 1.0f, 1.0f, 1.0f);
    glClearColor(0.9f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl1::vertexPointer(quad_verts);
    sizeGl();
    if (menu != nullptr) return true;

    Evt& evt = Evt::getInstance();
    evt.subscribe("sizeGl", [](const EvtArg&) { sizeGl(); });
    evt.subscribe("postScore", onPostScore);
    evt.subscribe("setBest", onSetBest);
    evt.subscribe("setSound", onSetSound);
    evt.subscribe("gotoLevel", onGotoLevel);
    evt.subscribe("gotoScores", onGotoScores);
    evt.subscribe("gotoMenu", onGotoMenu);
    evt.subscribe("onTutorialShown", onTutorialShown);
    menu = new Menu();
    score_menu = new ScoreMenu();
    SaveData::load();
    Scores::init();
    LevelDefs::initializeData();
    if (!SaveData::read(false, "tutorial_shown")) {
        LevelDefs::level_info[0].tutorial_image = "tutorial.png";
    }
    bool sound_on = SaveData::read(true, "sound");
    evt.publish("setSound", sound_on);
    LevelDefs::MenuInfo& menu_info = LevelDefs::menu_info;
    v3f pos((float) menu_info.score_button.pos.x, (float) menu_info.score_button.pos.y);
    menu->create(menu_info.image, menu_info.score_button.image, pos, sound_on);
    LevelDefs::ScoreMenuInfo& score_menu_info = LevelDefs::score_menu_info;
    score_menu->create(score_menu_info.image, score_menu_info.back_button.pos,
                       score_menu_info.back_button.size);
    for (int i = 0; i < LevelDefs::NUM_LEVELS; i++) {
        unlockLevel(i);
        int best = Scores::readBest(i);
        menu->setBest(i, best);
        LevelDefs::ScoreMenuLevelInfo& si = LevelDefs::level_info[i].score_menu_info;
        score_menu->setLevel(i, si.pos, si.size, si.score_pos);
        score_menu->setBest(i, best);
    }
    level = new Level();
    evt.publish("setBackgroundSound", "OfficeNoise.mp3");
    return true;
}

void activate() {
    if (menu != nullptr) menu->activate();
}

void setSound(bool sound) { menu->setSound(sound); }

bool getSound() { return menu != nullptr ? menu->m_sound_on : SaveData::read(true, "sound"); }

}  // namespace Papertoss
