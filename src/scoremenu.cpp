#include "scoremenu.h"

#include "platform.h"
#include "scores.h"

namespace {
const int NONE = 0;
const int ACTIVE = 1;
const char* SCORE_FONT = "zerothre";
const int SCORE_FONT_SIZE = 24;
const int SCORE_GLYPH_OFFSET = -32;
v4f SCORE_COLOR;
}  // namespace

ScoreMenu::ScoreMenu() {
    SCORE_COLOR = v4f(0.0f, 1.0f, 0.0f, 1.0f);
    Evt& evt = Evt::getInstance();
    evt.subscribe("onPtrUp", [this](const EvtArg& a) { onPtrUp(a.v); });
    for (int i = 0; i < LevelDefs::NUM_LEVELS; i++) {
        m_score[i] = nullptr;
        m_name_pos[i] = v2i(0, 0);
        m_name_size[i] = v2f(0.0f, 0.0f);
        m_score_pos[i] = v2i(0, 0);
    }
}

void ScoreMenu::activate() {
    if (m_state == NONE) m_state = ACTIVE;
}

void ScoreMenu::deactivate() {
    if (m_state == ACTIVE) m_state = NONE;
}

void ScoreMenu::onPtrUp(const v2f& v) {
    if (m_state != ACTIVE) return;
    Evt& evt = Evt::getInstance();
    if (Sprite::pointInRect(v, v2f(m_back_pos), m_back_size)) {
        evt.publish("paperTossPlaySound", "Computer.wav");
        evt.publish("gotoMenu");
        return;
    }
    for (int i = 0; i < LevelDefs::NUM_LEVELS; i++) {
        if (!m_name_pos[i].equalsZero() && Sprite::pointInRect(v, v2f(m_name_pos[i]), m_name_size[i])) {
            evt.publish("paperTossPlaySound", "Computer.wav");
            evt.publish("showScores", i);
            return;
        }
    }
}

void ScoreMenu::setLevel(int level, const v2i& pos, const v2f& size, const v2i& score_pos) {
    m_name_pos[level] = pos;
    m_name_size[level] = size;
    m_score_pos[level] = score_pos;
}

void ScoreMenu::setBest(int level, int score) {
    if (!m_score_pos[level].equalsZero()) {
        Sprite::killSprite(m_score[level]);
        m_score[level] = new Sprite(SCORE_FONT_SIZE, SCORE_GLYPH_OFFSET, SCORE_FONT, std::to_string(score),
                                    SCORE_COLOR, 0);
    }
}

void ScoreMenu::create(const char* background, const v2i& back_pos, const v2f& back_size) {
    m_background = new Sprite(background, v2i(), 0.0f, false, 0);
    m_background_filename = background;
    m_back_pos = back_pos;
    m_back_size = back_size;
}

void ScoreMenu::destroy() {
    Sprite::killSprite(m_background);
    m_background = nullptr;
    for (int i = 0; i < LevelDefs::NUM_LEVELS; i++) {
        Sprite::killSprite(m_score[i]);
        m_score[i] = nullptr;
    }
}

void ScoreMenu::unDestroy() {
    m_background = new Sprite(m_background_filename);
    for (int i = 0; i < LevelDefs::NUM_LEVELS; i++) {
        m_score[i] = new Sprite(SCORE_FONT_SIZE, SCORE_GLYPH_OFFSET, SCORE_FONT,
                                std::to_string(Scores::readBest(i)), SCORE_COLOR, 0);
    }
}

void ScoreMenu::update(float elapsed) { (void) elapsed; }

void ScoreMenu::render(const v2f& offset) {
    v3f o(offset.x, offset.y, 0.0f);
    if (m_background != nullptr) {
        m_background->draw(v3f(160.0f, 240.0f, Config::BACKGROUND_DEPTH).plus(o), v2f(1.0f, 1.0f),
                           v3f(0.0f, 0.0f, 0.0f), v4f(1.0f, 1.0f, 1.0f, 1.0f));
    }
    for (int i = 0; i < LevelDefs::NUM_LEVELS; i++) {
        if (m_score[i] != nullptr) {
            int x = (int) o.x;
            int y = (int) o.y;
            v2i vi = m_score_pos[i].plus(v2i(x, y));
            v3f vf((float) vi.x, (float) vi.y);
            m_score[i]->draw(vf, v2f(1.0f, 1.0f), v3f(0.0f, 0.0f, 0.0f), v4f(1.0f, 1.0f, 1.0f, 1.0f));
        }
    }
}
