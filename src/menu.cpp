#include "menu.h"

#include <algorithm>
#include <cmath>

#include "platform.h"

namespace {

const int NONE = 0;
const int ACTIVE = 1;
const char* EXIT_FILENAME = "Exit.png";
const char* SCORE_FONT = "zerothre";
const int SCORE_FONT_SIZE = 14;
const int SCORE_GLYPH_OFFSET = -32;
const float NEW_LEVEL_BLINK_DUR = 1.0f;

// Java assigned these in the Menu constructor; they depend on Globals::HI_RES.
v4f EXIT_COLOR;
v4f SOUND_COLOR;
v3f EXIT_POS;
v3f SOUND_POS;
v4f SELECTION_COLOR;
int SOUND_FONT_SIZE = 20;
int SOUND_GLYPH_OFFSET = -29;
const char* SOUND_FONT = "fawn";
float MENU_POPUP_DUR = 0.0f;
float MENU_POPUP_DELAY = 0.0f;

}  // namespace

const v4f Menu::GREYED_OUT_COLOR(0.33f, 0.33f, 0.33f, 1.0f);
const v4f Menu::UNSELECTION_COLOR(1.0f, 1.0f, 1.0f, 1.0f);

Menu::Menu() {
    m_scores_pos = v3f(0.0f, 0.0f, 0.0f);
    m_scores_color = v4f(1.0f, 1.0f, 1.0f, 1.0f);
    m_sound_color = SOUND_COLOR;
    m_exit_color = EXIT_COLOR;
    EXIT_COLOR = v4f(1.0f, 1.0f, 1.0f, 1.0f);
    SOUND_COLOR = v4f(1.0f, 0.0f, 0.0f, 1.0f);
    if (Globals::HI_RES) {
        SOUND_POS = v3f(277.0f, 376.0f);
        SOUND_FONT_SIZE = 18;
        EXIT_POS = v3f(278.0f, 420.0f);
    } else {
        SOUND_POS = v3f(283.0f, 376.0f);
        SOUND_FONT_SIZE = 20;
        EXIT_POS = v3f(283.0f, 455.0f);
    }
    SOUND_GLYPH_OFFSET = -29;
    SOUND_FONT = "fawn";
    SELECTION_COLOR = v4f(0.6f, 0.6f, 0.7f, 1.0f);
    MENU_POPUP_DUR = 0.0f;
    MENU_POPUP_DELAY = 0.0f;
    Evt& evt = Evt::getInstance();
    evt.subscribe("onPtrDown", [this](const EvtArg& a) { onPtrDown(a.v); });
    evt.subscribe("onPtrUp", [this](const EvtArg& a) { onPtrUp(a.v); });
    for (int i = 0; i < LevelDefs::NUM_LEVELS; i++) {
        m_level_button[i] = nullptr;
        m_level_button_color[i] = UNSELECTION_COLOR;
        m_level_button_pos[i] = v3f(0.0f, 0.0f, 0.0f);
        m_level_button_scale[i] = 0.0f;
        m_level_button_time[i] = 0.0f;
        m_level_button_delay[i] = i * MENU_POPUP_DELAY;
        m_level_score[i] = nullptr;
        m_level_score_pos[i] = v3f(0.0f, 0.0f, 0.0f);
    }
}

void Menu::onPtrDown(const v2f& v) {
    if (m_state != ACTIVE) return;
    m_selected_level = -1;
    for (int i = 0; i < LevelDefs::NUM_LEVELS; i++) {
        if (m_level_button[i] != nullptr && m_level_button[i]->checkPoint(m_level_button_pos[i], v, 1.0f)) {
            m_selected_level = i;
            break;
        }
    }
    for (int i2 = 0; i2 < LevelDefs::NUM_LEVELS; i2++) {
        m_level_button_color[i2] = i2 == m_selected_level ? SELECTION_COLOR : UNSELECTION_COLOR;
    }
    m_scores_color = v4f(1.0f, 1.0f, 1.0f, 1.0f);
    m_sound_color = SOUND_COLOR;
    if (m_selected_level == -1) {
        if (m_scores != nullptr && m_scores->checkPoint(m_scores_pos, v, 1.0f)) {
            m_scores_color = v4f(0.25f, 0.25f, 0.25f, 1.0f);
            return;
        }
        if (m_sound != nullptr && m_sound->checkPoint(SOUND_POS, v, 2.0f)) {
            m_sound_color = v4f(0.5f, 0.5f, 0.5f, 1.0f).times(SOUND_COLOR);
        } else if (m_exit != nullptr && m_exit->checkPoint(EXIT_POS, v, 1.0f)) {
            m_exit_color = v4f(0.5f, 0.5f, 0.5f, 1.0f).times(EXIT_COLOR);
        }
    }
}

void Menu::onPtrUp(const v2f& v) {
    Evt& evt = Evt::getInstance();
    for (int i = 0; i < LevelDefs::NUM_LEVELS; i++) {
        m_level_button_color[i] = v4f(1.0f, 1.0f, 1.0f, 1.0f);
    }
    m_scores_color = v4f(1.0f, 1.0f, 1.0f, 1.0f);
    m_sound_color = SOUND_COLOR;
    m_exit_color = EXIT_COLOR;
    if (m_state == ACTIVE) {
        if (m_selected_level != -1 && m_level_button[m_selected_level] != nullptr &&
            m_level_button[m_selected_level]->checkPoint(m_level_button_pos[m_selected_level], v, 1.0f)) {
            evt.publish("paperTossPlaySound", "Crumple.wav");
            evt.publish("gotoLevel", m_selected_level);
            if (m_selected_level == m_new_level) m_new_level = -1;
        } else if (m_scores != nullptr && m_scores->checkPoint(m_scores_pos, v, 1.0f)) {
            evt.publish("paperTossPlaySound", "Computer.wav");
            evt.publish("gotoScores");
        } else if (m_sound != nullptr && m_sound->checkPoint(SOUND_POS, v, 2.0f)) {
            setSound(!m_sound_on);
            evt.publish("setSound", m_sound_on);
            evt.publish("paperTossPlaySound", "Crumple.wav");
        } else if (m_exit != nullptr && m_exit->checkPoint(EXIT_POS, v, 1.0f)) {
            evt.publish("onExitPressed");
        }
        m_selected_level = -1;
    }
}

void Menu::setSound(bool on) {
    m_sound_on = on;
    Sprite::killSprite(m_sound);
    m_sound = new Sprite(SOUND_FONT_SIZE, SOUND_GLYPH_OFFSET, SOUND_FONT,
                         Util::format("Sound: %s", m_sound_on ? "on" : "off"),
                         v4f(1.0f, 0.0f, 0.0f, 1.0f), 0);
}

void Menu::setMenuButton(int level, const char* image_file, const v3f& pos, const v3f& score_pos,
                         const v4f& color) {
    Sprite::killSprite(m_level_button[level]);
    m_level_button[level] = new Sprite(image_file);
    m_level_button_filenames[level] = image_file;
    m_level_button_pos[level] = pos;
    m_level_button_color[level] = color;
    if (!score_pos.equalsZero()) m_level_score_pos[level] = score_pos;
}

void Menu::activate() {
    if (m_state == NONE) m_state = ACTIVE;
}

void Menu::deactivate() {
    if (m_state == ACTIVE) {
        m_state = NONE;
        for (int i = 0; i < LevelDefs::NUM_LEVELS; i++) {
            m_level_button_scale[i] = 0.0f;
            m_level_button_time[i] = 0.0f;
            m_level_button_delay[i] = i * MENU_POPUP_DELAY;
        }
    }
}

void Menu::create(const char* background, const char* scores_image, const v3f& scores_pos, bool sound) {
    destroy();
    m_exit_color = EXIT_COLOR;
    m_sound_color = SOUND_COLOR;
    m_scores_pos = scores_pos;
    m_background_filename = background;
    m_background = new Sprite(background);
    m_scores_filename = scores_image;
    m_scores = new Sprite(scores_image);
    m_exit = new Sprite(EXIT_FILENAME);
    setSound(sound);
}

void Menu::destroy() {
    for (int i = 0; i < LevelDefs::NUM_LEVELS; i++) {
        Sprite::killSprite(m_level_button[i]);
        m_level_button[i] = nullptr;
        Sprite::killSprite(m_level_score[i]);
        m_level_score[i] = nullptr;
    }
    Sprite::killSprite(m_background); m_background = nullptr;
    Sprite::killSprite(m_scores); m_scores = nullptr;
    Sprite::killSprite(m_sound); m_sound = nullptr;
    Sprite::killSprite(m_exit); m_exit = nullptr;
    m_destroy_state = m_state;
}

void Menu::unDestroy() {
    for (int i = 0; i < LevelDefs::NUM_LEVELS; i++) {
        if (!m_level_button_filenames[i].empty()) {
            m_level_button[i] = new Sprite(m_level_button_filenames[i]);
        }
    }
    m_background = new Sprite(m_background_filename);
    m_scores = new Sprite(m_scores_filename);
    setSound(m_sound_on);
    m_state = m_destroy_state;
    m_exit = new Sprite(EXIT_FILENAME);
}

void Menu::setBest(int level, int score) {
    if (!m_level_score_pos[level].equalsZero()) {
        Sprite::killSprite(m_level_score[level]);
        m_level_score[level] = new Sprite(SCORE_FONT_SIZE, SCORE_GLYPH_OFFSET, SCORE_FONT,
                                          std::to_string(score), v4f(0.0f, 1.0f, 0.0f, 1.0f), 0);
    }
}

void Menu::update(double elapsed) {
    if (m_state != ACTIVE) return;
    if (m_new_level >= 0) {
        if (m_new_level != m_selected_level) {
            m_new_level_timer = (float) (((double) m_new_level_timer) - elapsed);
            while (m_new_level_timer <= 0.0f) m_new_level_timer += NEW_LEVEL_BLINK_DUR;
            float i = std::abs((m_new_level_timer / NEW_LEVEL_BLINK_DUR) - 0.5f) * 2.0f;
            m_level_button_color[m_new_level] =
                SELECTION_COLOR.times(UNSELECTION_COLOR.minus(SELECTION_COLOR).times(i));
        } else {
            m_new_level_timer = 0.5f;
        }
    }
    for (int i2 = 0; i2 < LevelDefs::NUM_LEVELS; i2++) {
        if (m_level_button[i2] != nullptr) {
            m_level_button_time[i2] = (float) (((double) m_level_button_time[i2]) + elapsed);
            float si;
            if (MENU_POPUP_DUR != 0.0f) {
                si = std::max(std::min((m_level_button_time[i2] - m_level_button_delay[i2]) / MENU_POPUP_DUR,
                                       1.0f),
                              0.0f);
            } else {
                si = 1.0f;
            }
            m_level_button_scale[i2] = (si - 2.1f) * si * (si - 2.0f);
        }
    }
}

void Menu::render(const v2f& offset) {
    v3f o(offset.x, offset.y, 0.0f);
    v2f s(1.0f, 1.0f);
    v3f r(0.0f, 0.0f, 0.0f);
    if (m_background != nullptr) m_background->draw(v3f(160.0f, 240.0f, Config::BACKGROUND_DEPTH).plus(o));
    if (m_scores != nullptr) m_scores->draw(m_scores_pos.plus(o), s, r, m_scores_color);
    if (m_sound != nullptr) m_sound->draw(SOUND_POS.plus(o), s, r, m_sound_color);
    if (m_exit != nullptr) m_exit->draw(EXIT_POS.plus(o), s, r, m_exit_color);
    for (int i = 0; i < LevelDefs::NUM_LEVELS; i++) {
        if (m_level_button[i] != nullptr) {
            v2f bs(m_level_button_scale[i], m_level_button_scale[i]);
            m_level_button[i]->draw(m_level_button_pos[i].plus(o), bs, r, m_level_button_color[i]);
        }
        if (m_level_score[i] != nullptr) {
            m_level_score[i]->draw(m_level_score_pos[i].plus(o), s, r, m_scores_color);
        }
    }
}

void Menu::setNewLevel(int level) {
    m_new_level = level;
    m_new_level_timer = NEW_LEVEL_BLINK_DUR;
}
