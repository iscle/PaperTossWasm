// Port of com.bfs.papertoss.vector.*
#pragma once
#include <cmath>

struct v2i {
    int x = 0, y = 0;
    v2i() {}
    v2i(int a_x, int a_y) : x(a_x), y(a_y) {}
    bool equals(const v2i& v) const { return x == v.x && y == v.y; }
    bool equalsZero() const { return x == 0 && y == 0; }
    v2i plus(const v2i& v) const { return v2i(x + v.x, y + v.y); }
    v2i minus(const v2i& v) const { return v2i(x - v.x, y - v.y); }
};

struct v2f {
    float x = 0.0f, y = 0.0f;
    v2f() {}
    v2f(float a_x, float a_y) : x(a_x), y(a_y) {}
    explicit v2f(const v2i& v) : x((float) v.x), y((float) v.y) {}
    bool equals(const v2f& v) const { return x == v.x && y == v.y; }
    bool equalsZero() const { return x == 0.0f && y == 0.0f; }
    v2f minus(const v2f& v) const { return v2f(x - v.x, y - v.y); }
    v2f dividedBy(const v2f& v) const { return v2f(x / v.x, y / v.y); }
    v2f dividedBy(float f) const { return v2f(x / f, y / f); }
    v2f times(float f) const { return v2f(x * f, y * f); }
    v2f times(const v2f& v) const { return v2f(x * v.x, y * v.y); }
    v2f plus(const v2f& v) const { return v2f(x + v.x, y + v.y); }
    void plusEquals(const v2f& v) { x += v.x; y += v.y; }
    void timesEquals(const v2f& v) { x *= v.x; y *= v.y; }
    void timesEquals(float f) { x *= f; y *= f; }
    v2f rotated(double angle) const {
        return v2f((x * (float) std::cos(angle)) - (y * (float) std::sin(angle)),
                   (y * (float) std::cos(angle)) + (x * (float) std::sin(angle)));
    }
    float length() const { return (float) std::sqrt((x * x) + (y * y)); }
    void normalize() { float l = length(); x /= l; y /= l; }
    v2f normalized() const { v2f n(x, y); n.normalize(); return n; }
    static float dot(const v2f& a, const v2f& b) { return (a.x * b.x) + (a.y * b.y); }
    static float getNegativeRotation(const v2f& v) {
        return (v.y < 0.0f ? -1.0f : 1.0f) * ((float) std::acos(dot(v.normalized(), v2f(1.0f, 0.0f)))) * (-1.0f);
    }
};

struct v3f {
    float x = 0.0f, y = 0.0f, z = 0.0f;
    v3f() {}
    v3f(float a_x, float a_y) : x(a_x), y(a_y), z(0.0f) {}
    v3f(float a_x, float a_y, float a_z) : x(a_x), y(a_y), z(a_z) {}
    // Java: v3f.iv3f() flips a top-left origin y coordinate into GL space.
    static v3f iv3f(float a_x, float a_y, float a_z) { return v3f(a_x, 480.0f - a_y, a_z); }
    v3f minus(const v3f& v) const { return v3f(x - v.x, y - v.y, z - v.z); }
    v3f plus(const v3f& v) const { return v3f(x + v.x, y + v.y, z + v.z); }
    v3f times(float f) const { return v3f(x * f, y * f, z * f); }
    v3f times(const v3f& v) const { return v3f(x * v.x, y * v.y, z * v.z); }
    bool equals(const v3f& v) const { return x == v.x && y == v.y && z == v.z; }
    bool equalsZero() const { return x == 0.0f && y == 0.0f && z == 0.0f; }
};

struct v4f {
    float x = 0.0f, y = 0.0f, z = 0.0f, w = 0.0f;
    v4f() {}
    v4f(float a_x, float a_y, float a_z, float a_w) : x(a_x), y(a_y), z(a_z), w(a_w) {}
    v4f times(const v4f& v) const { return v4f(x * v.x, y * v.y, z * v.z, w * v.w); }
    v4f times(float f) const { return v4f(x * f, y * f, z * f, w * f); }
    v4f plus(const v4f& v) const { return v4f(x + v.x, y + v.y, z + v.z, w + v.w); }
    v4f minus(const v4f& v) const { return v4f(x - v.x, y - v.y, z - v.z, w - v.w); }
    bool equals(const v4f& v) const { return x == v.x && y == v.y && z == v.z && w == v.w; }
    bool equalsZero() const { return x == 0.0f && y == 0.0f && z == 0.0f && w == 0.0f; }
};
