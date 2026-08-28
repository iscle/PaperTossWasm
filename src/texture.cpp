#include "texture.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "default_font_metrics.h"
#include "platform.h"

#define STB_IMAGE_IMPLEMENTATION
#include "third_party/stb_image.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "third_party/stb_truetype.h"

namespace {

const char* kAssetRoot = "/assets/";

std::vector<unsigned char> readFile(const std::string& path) {
    std::vector<unsigned char> data;
    FILE* file = std::fopen(path.c_str(), "rb");
    if (file == nullptr) return data;
    std::fseek(file, 0, SEEK_END);
    long length = std::ftell(file);
    std::fseek(file, 0, SEEK_SET);
    if (length > 0) {
        data.resize((size_t) length);
        if (std::fread(data.data(), 1, (size_t) length, file) != (size_t) length) data.clear();
    }
    std::fclose(file);
    return data;
}

// The TTFs are small and reused constantly, so keep them mapped once.
stbtt_fontinfo* getFont(const std::string& name) {
    static std::map<std::string, std::pair<std::vector<unsigned char>, stbtt_fontinfo>> fonts;
    auto it = fonts.find(name);
    if (it == fonts.end()) {
        auto& entry = fonts[name];
        entry.first = readFile(std::string(kAssetRoot) + "fonts/" + name + ".ttf");
        if (entry.first.empty()) {
            std::printf("Texture: could not load font %s\n", name.c_str());
            fonts.erase(name);
            return nullptr;
        }
        if (!stbtt_InitFont(&entry.second, entry.first.data(),
                            stbtt_GetFontOffsetForIndex(entry.first.data(), 0))) {
            std::printf("Texture: could not parse font %s\n", name.c_str());
            fonts.erase(name);
            return nullptr;
        }
        it = fonts.find(name);
    }
    return &it->second.second;
}

}  // namespace

void Texture::create(const unsigned char* rgba, int components, const v2i& size, const v2i& pot_size) {
    m_components = components;
    m_size = size;
    m_pot_size = pot_size;
    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pot_size.x, pot_size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

Texture::Texture(const std::string& filename_in) {
    std::string filename = filename_in;
    if (filename.find("img") == std::string::npos) {
        filename = (Globals::HI_RES ? "img_hi_res/" : "img/") + filename;
    }

    int width = 0, height = 0, channels = 0;
    std::string path = std::string(kAssetRoot) + filename;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4);
    if (data == nullptr) {
        // The original level data names a couple of files that only exist at the
        // other resolution (and with the other extension); on Android those throw.
        const char* kDirs[2] = {Globals::HI_RES ? "img_hi_res/" : "img/",
                                Globals::HI_RES ? "img/" : "img_hi_res/"};
        static const char* kExts[] = {".png", ".jpg", ".PNG", ".JPG"};
        std::string base = filename.substr(filename.find('/') + 1);
        std::string stem = base.substr(0, base.find_last_of('.'));
        for (int d = 0; d < 2 && data == nullptr; d++) {
            for (int e = 0; e < 4 && data == nullptr; e++) {
                std::string alt = std::string(kAssetRoot) + kDirs[d] + stem + kExts[e];
                data = stbi_load(alt.c_str(), &width, &height, &channels, 4);
                if (data != nullptr) {
                    std::printf("Texture: %s missing, using %s\n", filename.c_str(), alt.c_str());
                }
            }
        }
    }
    if (data == nullptr) {
        std::printf("Texture: failed to load %s\n", filename.c_str());
        width = height = 1;
        data = (unsigned char*) std::malloc(4);
        std::memset(data, 0, 4);
    }

    v2i size(width, height);
    // Android padded every bitmap out to a power of two (GL ES 1.x needed it).
    int pot_width = Util::nextPowerOfTwo(width);
    int pot_height = Util::nextPowerOfTwo(height);
    std::vector<unsigned char> padded((size_t) pot_width * pot_height * 4, 0);
    for (int y = 0; y < height; y++) {
        std::memcpy(&padded[(size_t) y * pot_width * 4], &data[(size_t) y * width * 4], (size_t) width * 4);
    }
    stbi_image_free(data);

    create(padded.data(), 4, size, v2i(pot_width, pot_height));
    m_name = filename;
}

Texture::Texture(const std::string& text, const std::string& font, int glyph_offset, int font_size,
                 const v4f& color, int outline, float fill) {
    (void) glyph_offset;
    (void) fill;
    stbtt_fontinfo* info = getFont(font);
    float scale = 0.0f;
    int ascent = 0, descent = 0, line_gap = 0;
    if (info != nullptr) {
        scale = stbtt_ScaleForMappingEmToPixels(info, (float) font_size);
        stbtt_GetFontVMetrics(info, &ascent, &descent, &line_gap);
    }

    // Two different measurements, exactly as Texture.java ends up doing:
    // Paint.measureText() runs before setTypeface(), so the box is sized by the
    // default typeface, while drawText() centres the string using the asset font.
    float measured = 0.0f;
    if (info != nullptr) {
        for (size_t i = 0; i < text.size(); i++) {
            int advance = 0, lsb = 0;
            stbtt_GetCodepointHMetrics(info, (unsigned char) text[i], &advance, &lsb);
            measured += advance * scale;
            if (i + 1 < text.size()) {
                measured += stbtt_GetCodepointKernAdvance(info, (unsigned char) text[i],
                                                          (unsigned char) text[i + 1]) * scale;
            }
        }
    }
    int text_width = (int) default_typeface::measure(text, font_size);
    int width = Util::nextPowerOfTwo(text_width > font_size ? text_width : font_size);
    if (width <= 0) width = 1;

    std::vector<unsigned char> coverage((size_t) width * width, 0);
    if (info != nullptr) {
        // Android drew into a canvas flipped on Y (canvas.scale(1, -1)) with the
        // baseline at width/2 - font_size/4; rasterise upright at the mirrored
        // row and flip the whole bitmap afterwards, which comes out identical.
        int baseline = (width - 1) - ((width / 2) - (font_size / 4));
        float pen_x = (float) (width / 2) - (measured * 0.5f);
        // outline > 0 meant Paint.setFakeBoldText(true) on Android, which reaches
        // FT_Outline_Embolden with a strength of font_size/24, growing the glyph
        // in +x and +y. Dilating the coverage the same way is the raster
        // equivalent.
        int bold = outline > 0 ? (font_size / 24) : 0;
        for (size_t i = 0; i < text.size(); i++) {
            int codepoint = (unsigned char) text[i];
            int advance = 0, lsb = 0;
            stbtt_GetCodepointHMetrics(info, codepoint, &advance, &lsb);
            int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
            stbtt_GetCodepointBitmapBox(info, codepoint, scale, scale, &x0, &y0, &x1, &y1);
            int glyph_w = x1 - x0, glyph_h = y1 - y0;
            if (glyph_w > 0 && glyph_h > 0) {
                std::vector<unsigned char> glyph((size_t) glyph_w * glyph_h, 0);
                stbtt_MakeCodepointBitmap(info, glyph.data(), glyph_w, glyph_h, glyph_w, scale, scale,
                                          codepoint);
                for (int pass = 0; pass <= bold * bold + 2 * bold; pass++) {
                    int span = bold + 1;
                    int dst_x0 = (int) (pen_x + 0.5f) + x0 + (pass % span);
                    int dst_y0 = baseline + y0 - (pass / span);
                    for (int y = 0; y < glyph_h; y++) {
                        int dy = dst_y0 + y;
                        if (dy < 0 || dy >= width) continue;
                        for (int x = 0; x < glyph_w; x++) {
                            int dx = dst_x0 + x;
                            if (dx < 0 || dx >= width) continue;
                            unsigned char& dst = coverage[(size_t) dy * width + dx];
                            unsigned char src = glyph[(size_t) y * glyph_w + x];
                            if (src > dst) dst = src;
                        }
                    }
                }
            }
            pen_x += advance * scale;
            if (i + 1 < text.size()) {
                pen_x += stbtt_GetCodepointKernAdvance(info, codepoint, (unsigned char) text[i + 1]) * scale;
            }
        }
    }

    std::vector<unsigned char> rgba((size_t) width * width * 4, 0);
    unsigned char r = (unsigned char) (color.x * 255.0f);
    unsigned char g = (unsigned char) (color.y * 255.0f);
    unsigned char b = (unsigned char) (color.z * 255.0f);
    float a = color.w;
    for (int y = 0; y < width; y++) {
        int src_y = (width - 1) - y;  // the canvas.scale(1, -1) flip
        for (int x = 0; x < width; x++) {
            unsigned char c = coverage[(size_t) src_y * width + x];
            size_t o = ((size_t) y * width + x) * 4;
            rgba[o + 0] = r;
            rgba[o + 1] = g;
            rgba[o + 2] = b;
            rgba[o + 3] = (unsigned char) (c * a);
        }
    }

    m_pot_size = v2i(width, width);
    m_size = m_pot_size;
    m_text_size = v2i(text_width, font_size);
    m_has_text_size = true;
    create(rgba.data(), 4, m_size, m_pot_size);
    m_name = text;
}

Texture::~Texture() { destroy(); }

void Texture::destroy() {
    if (m_id != 0) {
        glDeleteTextures(1, &m_id);
        m_id = 0;
    }
}

// ---------------------------------------------------------- TextureMgr

Texture* TextureMgr::get(const std::string& filename) {
    TextureInfo& ti = m_textures[filename];
    if (ti.texture == nullptr) ti.texture = new Texture(filename);
    ti.ref_count++;
    return ti.texture;
}

Texture* TextureMgr::getText(const std::string& text, const std::string& font, int glyph_offset, int size,
                             const v4f& color, int outline, float fill) {
    std::string name = text + font + std::to_string(size) + std::to_string(color.x) +
                       std::to_string(color.y) + std::to_string(color.z) + std::to_string(color.w) +
                       std::to_string(outline);
    TextureInfo& ti = m_textures[name];
    if (ti.texture == nullptr) {
        ti.texture = new Texture(text, font, glyph_offset, size, color, outline, fill);
    }
    ti.ref_count++;
    return ti.texture;
}

void TextureMgr::release(Texture* texture) {
    for (auto& entry : m_textures) {
        if (entry.second.texture == texture) {
            entry.second.ref_count--;
            return;
        }
    }
}

void TextureMgr::cleanup() {
    for (auto it = m_textures.begin(); it != m_textures.end();) {
        if (it->second.ref_count < 1) {
            delete it->second.texture;
            it = m_textures.erase(it);
        } else {
            ++it;
        }
    }
}
