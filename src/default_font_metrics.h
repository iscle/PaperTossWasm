// Advance widths of ASCII 32..126 in Roboto-Regular (Apache-2.0), which is the
// typeface Android's Paint starts with.
//
// Texture.java measures the string BEFORE it installs the asset typeface:
//
//     int text_width = (int) paint.measureText(text);   // default typeface
//     ...
//     paint.setTypeface(typeface);                      // fawn / zerothre
//
// so the power-of-two texture box and Texture.m_text_size (which is what the
// hit tests and the wind-speed scaling read) are sized by the default font,
// while the glyphs themselves are drawn with the asset font. Reproducing that
// needs the default font's metrics, and metrics are all it needs - hence a
// table instead of a second TTF in the bundle.
#pragma once

#include <string>

namespace default_typeface {

constexpr int kUnitsPerEm = 2048;

constexpr short kAdvance[95] = {
     507,  527,  655, 1261, 1150, 1500, 1273,  357,  700,  712,
     882, 1161,  402,  565,  539,  844, 1150, 1150, 1150, 1150,
    1150, 1150, 1150, 1150, 1150, 1150,  496,  433, 1041, 1124,
    1070,  967, 1839, 1336, 1275, 1333, 1343, 1164, 1132, 1395,
    1460,  557, 1130, 1284, 1102, 1788, 1460, 1408, 1292, 1408,
    1261, 1215, 1222, 1328, 1303, 1817, 1284, 1230, 1226,  543,
     840,  543,  856,  924,  633, 1114, 1149, 1072, 1155, 1085,
     711, 1149, 1128,  497,  489, 1038,  497, 1795, 1130, 1168,
    1149, 1164,  693, 1056,  669, 1129,  992, 1539, 1015,  969,
    1015,  693,  499,  693, 1393
};

// Paint.measureText() at the given text size.
inline float measure(const std::string& text, int font_size) {
    float units = 0.0f;
    for (size_t i = 0; i < text.size(); i++) {
        unsigned char c = (unsigned char) text[i];
        int index = (c >= 32 && c <= 126) ? c - 32 : ('?' - 32);
        units += (float) kAdvance[index];
    }
    return (units * (float) font_size) / (float) kUnitsPerEm;
}

}  // namespace default_typeface
