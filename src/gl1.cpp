#include "gl1.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace gl1 {
namespace {

struct mat4 {
    // Column-major, like OpenGL.
    float m[16];
};

mat4 identity() {
    mat4 r;
    std::memset(r.m, 0, sizeof(r.m));
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
    return r;
}

mat4 multiply(const mat4& a, const mat4& b) {
    mat4 r;
    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            float sum = 0.0f;
            for (int k = 0; k < 4; k++) sum += a.m[k * 4 + row] * b.m[col * 4 + k];
            r.m[col * 4 + row] = sum;
        }
    }
    return r;
}

const int kStackDepth = 16;

struct State {
    mat4 projection = identity();
    mat4 modelview = identity();
    mat4 stack[kStackDepth];
    int stack_top = 0;
    int mode = MODELVIEW;

    float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    const float* vertex_pointer = nullptr;
    const float* texcoord_pointer = nullptr;

    GLuint program = 0;
    GLuint vbo = 0;
    GLint a_pos = -1, a_uv = -1;
    GLint u_mvp = -1, u_color = -1, u_tex = -1;
};

State g;

mat4& current() { return g.mode == PROJECTION ? g.projection : g.modelview; }

const char* kVertexShader =
    "attribute vec2 a_pos;\n"
    "attribute vec2 a_uv;\n"
    "uniform mat4 u_mvp;\n"
    "varying vec2 v_uv;\n"
    "void main() {\n"
    "  v_uv = a_uv;\n"
    "  gl_Position = u_mvp * vec4(a_pos, 0.0, 1.0);\n"
    "}\n";

const char* kFragmentShader =
    "precision mediump float;\n"
    "uniform sampler2D u_tex;\n"
    "uniform vec4 u_color;\n"
    "varying vec2 v_uv;\n"
    "void main() {\n"
    "  gl_FragColor = texture2D(u_tex, v_uv) * u_color;\n"
    "}\n";

GLuint compile(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::printf("gl1: shader compile failed: %s\n", log);
    }
    return shader;
}

}  // namespace

void init() {
    GLuint vs = compile(GL_VERTEX_SHADER, kVertexShader);
    GLuint fs = compile(GL_FRAGMENT_SHADER, kFragmentShader);
    g.program = glCreateProgram();
    glAttachShader(g.program, vs);
    glAttachShader(g.program, fs);
    glLinkProgram(g.program);
    GLint ok = 0;
    glGetProgramiv(g.program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(g.program, sizeof(log), nullptr, log);
        std::printf("gl1: program link failed: %s\n", log);
    }
    glDeleteShader(vs);
    glDeleteShader(fs);

    g.a_pos = glGetAttribLocation(g.program, "a_pos");
    g.a_uv = glGetAttribLocation(g.program, "a_uv");
    g.u_mvp = glGetUniformLocation(g.program, "u_mvp");
    g.u_color = glGetUniformLocation(g.program, "u_color");
    g.u_tex = glGetUniformLocation(g.program, "u_tex");

    glGenBuffers(1, &g.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, g.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 16, nullptr, GL_DYNAMIC_DRAW);

    glUseProgram(g.program);
    glUniform1i(g.u_tex, 0);
    glEnableVertexAttribArray(g.a_pos);
    glEnableVertexAttribArray(g.a_uv);
    glVertexAttribPointer(g.a_pos, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*) 0);
    glVertexAttribPointer(g.a_uv, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*) (sizeof(float) * 2));
    glActiveTexture(GL_TEXTURE0);
}

void matrixMode(int mode) { g.mode = mode; }
void loadIdentity() { current() = identity(); }

void pushMatrix() {
    if (g.stack_top < kStackDepth) g.stack[g.stack_top++] = current();
}

void popMatrix() {
    if (g.stack_top > 0) current() = g.stack[--g.stack_top];
}

void translatef(float x, float y, float z) {
    mat4 t = identity();
    t.m[12] = x; t.m[13] = y; t.m[14] = z;
    current() = multiply(current(), t);
}

void rotatef(float angle_degrees, float x, float y, float z) {
    float len = std::sqrt(x * x + y * y + z * z);
    if (len == 0.0f) return;
    x /= len; y /= len; z /= len;
    float a = angle_degrees * 3.14159265358979323846f / 180.0f;
    float c = std::cos(a), s = std::sin(a), ic = 1.0f - c;
    mat4 r = identity();
    r.m[0] = x * x * ic + c;      r.m[4] = x * y * ic - z * s;  r.m[8] = x * z * ic + y * s;
    r.m[1] = y * x * ic + z * s;  r.m[5] = y * y * ic + c;      r.m[9] = y * z * ic - x * s;
    r.m[2] = z * x * ic - y * s;  r.m[6] = z * y * ic + x * s;  r.m[10] = z * z * ic + c;
    current() = multiply(current(), r);
}

void scalef(float x, float y, float z) {
    mat4 s = identity();
    s.m[0] = x; s.m[5] = y; s.m[10] = z;
    current() = multiply(current(), s);
}

void orthof(float left, float right, float bottom, float top, float z_near, float z_far) {
    mat4 o = identity();
    o.m[0] = 2.0f / (right - left);
    o.m[5] = 2.0f / (top - bottom);
    o.m[10] = -2.0f / (z_far - z_near);
    o.m[12] = -(right + left) / (right - left);
    o.m[13] = -(top + bottom) / (top - bottom);
    o.m[14] = -(z_far + z_near) / (z_far - z_near);
    current() = multiply(current(), o);
}

void frustumf(float left, float right, float bottom, float top, float z_near, float z_far) {
    mat4 f;
    std::memset(f.m, 0, sizeof(f.m));
    f.m[0] = (2.0f * z_near) / (right - left);
    f.m[5] = (2.0f * z_near) / (top - bottom);
    f.m[8] = (right + left) / (right - left);
    f.m[9] = (top + bottom) / (top - bottom);
    f.m[10] = -(z_far + z_near) / (z_far - z_near);
    f.m[11] = -1.0f;
    f.m[14] = -(2.0f * z_far * z_near) / (z_far - z_near);
    current() = multiply(current(), f);
}

void color4f(float r, float gr, float b, float a) {
    g.color[0] = r; g.color[1] = gr; g.color[2] = b; g.color[3] = a;
}

void vertexPointer(const float* xy) { g.vertex_pointer = xy; }
void texCoordPointer(const float* uv) { g.texcoord_pointer = uv; }
void bindTexture(GLuint texture) { glBindTexture(GL_TEXTURE_2D, texture); }

void drawQuad() {
    if (g.vertex_pointer == nullptr || g.texcoord_pointer == nullptr) return;
    float data[16];
    for (int i = 0; i < 4; i++) {
        data[i * 4 + 0] = g.vertex_pointer[i * 2 + 0];
        data[i * 4 + 1] = g.vertex_pointer[i * 2 + 1];
        data[i * 4 + 2] = g.texcoord_pointer[i * 2 + 0];
        data[i * 4 + 3] = g.texcoord_pointer[i * 2 + 1];
    }
    mat4 mvp = multiply(g.projection, g.modelview);
    glUniformMatrix4fv(g.u_mvp, 1, GL_FALSE, mvp.m);
    glUniform4fv(g.u_color, 1, g.color);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(data), data);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

}  // namespace gl1
