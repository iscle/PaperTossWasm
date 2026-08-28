// Port of com.bfs.papertoss.cpp.Texture and TextureMgr.
#pragma once

#include <map>
#include <string>

#include <GLES2/gl2.h>

#include "vec.h"

class Texture {
public:
    // Image texture, loaded from the packaged assets.
    explicit Texture(const std::string& filename);
    // Text texture, rasterised from one of the game's TTFs.
    Texture(const std::string& text, const std::string& font, int glyph_offset, int font_size,
            const v4f& color, int outline, float fill);
    ~Texture();

    void destroy();
    GLuint id() const { return m_id; }
    const v2i& size() const { return m_size; }
    const v2i& potSize() const { return m_pot_size; }

    GLuint m_id = 0;
    int m_components = 4;
    std::string m_name;
    v2i m_size;
    v2i m_pot_size;
    v2i m_text_size;       // Java kept this null for image textures.
    bool m_has_text_size = false;

private:
    void create(const unsigned char* rgba, int components, const v2i& size, const v2i& pot_size);
};

class TextureMgr {
public:
    Texture* get(const std::string& filename);
    Texture* getText(const std::string& text, const std::string& font, int glyph_offset, int size,
                     const v4f& color, int outline, float fill);
    void release(Texture* texture);
    void cleanup();

private:
    struct TextureInfo {
        Texture* texture = nullptr;
        int ref_count = 0;
    };
    std::map<std::string, TextureInfo> m_textures;
};
