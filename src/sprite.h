// Port of com.bfs.papertoss.cpp.Sprite
#pragma once

#include <array>
#include <string>
#include <vector>

#include "texture.h"
#include "vec.h"

class Sprite {
public:
    Sprite(const std::string& texture_name, const v2i& frame_size, float duration, bool tile,
           int frame_count);
    explicit Sprite(const std::string& texture_name);
    Sprite(const std::string& texture_name, const v2i& frame_size);
    Sprite(const std::string& texture_name, const v2i& frame_size, float duration);
    // Text sprite.
    Sprite(int size, int glyph_offset, const std::string& font, const std::string& text,
           const v4f& color, int outline);
    ~Sprite();

    static void killSprite(Sprite* s);

    void setFrame(int frame);
    void setScroll(const v2f& scroll);
    bool update(float elapsed);
    void draw(const v3f& pos);
    void draw(const v3f& pos, const v2f& scale, const v3f& rot, const v4f& color);
    bool checkPoint(const v3f& pos, const v2f& point, float scale);
    static bool pointInRect(const v2f& point, const v2f& pos, const v2f& size);
    v2i frameSize() const;

    Texture* m_texture = nullptr;

private:
    void constructor(const std::string& texture_name, const v2i& frame_size, float duration, bool tile,
                     int frame_count);

    std::vector<std::array<float, 8>> m_buffers;
    int m_current_frame = 0;
    float m_duration = 0.0f;
    float m_elapsed = 0.0f;
    int m_frame_count = 0;
    v2f m_frame_size;
    v2f m_scroll;
    bool m_tile = false;
};
