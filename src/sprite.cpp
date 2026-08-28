#include "sprite.h"

#include <algorithm>

#include "gl1.h"
#include "platform.h"

namespace {
const v2f DEFAULT_SCALE(1.0f, 1.0f);
const v3f DEFAULT_ROT(0.0f, 0.0f, 0.0f);
const v4f DEFAULT_COLOR(1.0f, 1.0f, 1.0f, 1.0f);
}  // namespace

void Sprite::killSprite(Sprite* s) {
    if (s != nullptr) delete s;
}

void Sprite::constructor(const std::string& texture_name, const v2i& frame_size, float duration,
                         bool tile, int frame_count) {
    m_texture = nullptr;
    m_frame_count = 0;
    m_frame_size = v2f(0.0f, 0.0f);
    m_current_frame = 0;
    if (tile) duration = 0.0f;
    m_duration = duration;
    m_elapsed = 0.0f;
    m_scroll = v2f(0.0f, 0.0f);
    m_tile = tile;
    m_texture = Globals::texture_mgr->get(texture_name);
    v2i size = m_texture->size();
    v2i pot_size = m_texture->potSize();
    m_frame_size = frame_size.equalsZero() ? v2f((float) size.x, (float) size.y)
                                           : v2f((float) frame_size.x, (float) frame_size.y);
    int col_count = std::max(size.x / ((int) m_frame_size.x), 1);
    int row_count = std::max(size.y / ((int) m_frame_size.y), 1);
    if (m_tile) {
        frame_count = 1;
    } else if (frame_count <= 0) {
        frame_count = col_count * row_count;
    }
    m_frame_count = frame_count;
    m_buffers.resize((size_t) m_frame_count);
    for (int i = 0; i < m_frame_count; i++) {
        int row = i / col_count;
        int col = i % col_count;
        float x_lo = (col * m_frame_size.x) / (float) pot_size.x;
        float x_hi = ((col + 1) * m_frame_size.x) / (float) pot_size.x;
        float y_hi = (row * m_frame_size.y) / (float) pot_size.y;
        float y_lo = ((row + 1) * m_frame_size.y) / (float) pot_size.y;
        m_buffers[i] = {x_lo, y_lo, x_hi, y_lo, x_hi, y_hi, x_lo, y_hi};
    }
    m_frame_size.timesEquals(Globals::SCALE_FACTOR);
}

Sprite::Sprite(const std::string& texture_name, const v2i& frame_size, float duration, bool tile,
               int frame_count) {
    constructor(texture_name, frame_size, duration, tile, frame_count);
}

Sprite::Sprite(const std::string& texture_name) {
    constructor(texture_name, v2i(0, 0), 0.0f, false, 0);
}

Sprite::Sprite(const std::string& texture_name, const v2i& frame_size) {
    constructor(texture_name, frame_size, 0.0f, false, 0);
}

Sprite::Sprite(const std::string& texture_name, const v2i& frame_size, float duration) {
    constructor(texture_name, frame_size, duration, false, 0);
}

Sprite::Sprite(int size, int glyph_offset, const std::string& font, const std::string& text,
               const v4f& color, int outline) {
    m_texture = nullptr;
    m_frame_count = 0;
    m_frame_size = v2f(0.0f, 0.0f);
    m_current_frame = 0;
    m_duration = 0.0f;
    m_elapsed = 0.0f;
    m_scroll = v2f(0.0f, 0.0f);
    m_tile = false;
    float newSize = ((float) size / Globals::SCALE_FACTOR) + 0.5f;
    m_texture = Globals::texture_mgr->getText(text, font, glyph_offset, (int) newSize, color, outline,
                                              ((color.x + color.y) + color.z) / 3.0f);
    v2i frame_size = m_texture->size();
    v2i pot_size = m_texture->potSize();
    float y_hi = (float) frame_size.y / (float) pot_size.y;
    float x_hi = (float) frame_size.x / (float) pot_size.x;
    m_frame_size = v2f((float) frame_size.x, (float) frame_size.y);
    m_frame_count = 1;
    m_buffers.resize(1);
    m_buffers[0] = {0.0f, 0.0f, x_hi, 0.0f, x_hi, y_hi, 0.0f, y_hi};
    m_frame_size.timesEquals(Globals::SCALE_FACTOR);
}

Sprite::~Sprite() {
    if (Globals::texture_mgr != nullptr) Globals::texture_mgr->release(m_texture);
}

void Sprite::setFrame(int frame) {
    m_current_frame = std::max(std::min(frame, m_frame_count - 1), 0);
    m_elapsed = (float) (m_current_frame / m_frame_count) * m_duration;
}

void Sprite::setScroll(const v2f& scroll) {
    if (m_frame_count == 1) {
        m_scroll = scroll;
        m_duration = 0.0f;
    }
}

bool Sprite::update(float elapsed) {
    if (m_duration > 0.0f) {
        m_elapsed += elapsed;
        if (m_elapsed >= m_duration) {
            m_elapsed = std::fmod(m_elapsed, m_duration);
            return true;
        }
    }
    return false;
}

void Sprite::draw(const v3f& pos) { draw(pos, DEFAULT_SCALE, DEFAULT_ROT, DEFAULT_COLOR); }

void Sprite::draw(const v3f& pos, const v2f& scale, const v3f& rot, const v4f& color) {
    if (m_frame_count == 0) return;
    if (m_duration > 0.0f) {
        m_current_frame = std::min((int) ((m_elapsed / m_duration) * m_frame_count), m_frame_count - 1);
    }
    gl1::pushMatrix();
    gl1::translatef(pos.x, pos.y, -pos.z);
    if (rot.z != 0.0f) gl1::rotatef(rot.z, 0.0f, 0.0f, 1.0f);
    if (rot.y != 0.0f) gl1::rotatef(rot.y, 0.0f, 1.0f, 0.0f);
    if (rot.x != 0.0f) gl1::rotatef(rot.x, 1.0f, 0.0f, 0.0f);
    float size_x = m_frame_size.x * scale.x;
    float size_y = m_frame_size.y * scale.y;
    gl1::scalef(size_x, size_y, 1.0f);
    gl1::color4f(color.x, color.y, color.z, color.w);
    gl1::bindTexture(m_texture->id());
    gl1::texCoordPointer(m_buffers[m_current_frame].data());
    gl1::drawQuad();
    gl1::popMatrix();
}

bool Sprite::checkPoint(const v3f& pos, const v2f& point, float scale) {
    v2f size = m_texture->m_has_text_size
                   ? v2f((float) m_texture->m_text_size.x, (float) m_texture->m_text_size.y)
                   : m_frame_size;
    v2f half((size.x / 2.0f) * scale, (size.y / 2.0f) * scale);
    v4f r(pos.x - half.x, pos.y - half.y, pos.x + half.x, pos.y + half.y);
    return point.x >= r.x && point.x <= r.z && point.y >= r.y && point.y <= r.w;
}

bool Sprite::pointInRect(const v2f& point, const v2f& pos, const v2f& size) {
    v2f p(pos.x, pos.y);
    v2f s(size.x * 0.5f, size.y * 0.5f);
    v2f min = p.minus(s);
    v2f max = p.plus(s);
    return point.x >= min.x && point.x <= max.x && point.y >= min.y && point.y <= max.y;
}

v2i Sprite::frameSize() const { return v2i((int) m_frame_size.x, (int) m_frame_size.y); }
