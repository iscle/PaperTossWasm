// Minimal fixed-function (GL ES 1.x) emulation on top of GL ES 2.0 / WebGL.
//
// The Android game drives GL10 directly: a matrix stack, glColor4f, a vertex
// pointer and a texcoord pointer, and one glDrawArrays(GL_TRIANGLE_FAN, 0, 4)
// per sprite. WebGL has none of that, so this file provides exactly the subset
// the game uses and nothing more.
#pragma once

#include <GLES2/gl2.h>

namespace gl1 {

enum { PROJECTION = 0x1701, MODELVIEW = 0x1700 };

void init();

void matrixMode(int mode);
void loadIdentity();
void pushMatrix();
void popMatrix();
void translatef(float x, float y, float z);
void rotatef(float angle_degrees, float x, float y, float z);
void scalef(float x, float y, float z);
void orthof(float left, float right, float bottom, float top, float z_near, float z_far);
void frustumf(float left, float right, float bottom, float top, float z_near, float z_far);

void color4f(float r, float g, float b, float a);
void vertexPointer(const float* xy);    // 4 vertices, 8 floats, kept by pointer
void texCoordPointer(const float* uv);  // 4 vertices, 8 floats, kept by pointer
void bindTexture(GLuint texture);
void drawQuad();                        // GL_TRIANGLE_FAN, 0, 4

}  // namespace gl1
