// Port of com.bfs.papertoss.cpp.Anim
#pragma once

#include <algorithm>
#include <vector>

#include "vec.h"

class Anim {
public:
    Anim() {}
    Anim(const std::vector<v3f>& frames, int count, float duration)
        : m_frames(frames), m_count(count), m_duration(duration), m_elapsed(0.0f) {}

    v3f get(float elapsed) {
        m_elapsed += elapsed;
        float i = std::min(m_elapsed / m_duration, 1.0f);
        int s = std::min((int) ((m_count - 1) * i), m_count - 2);
        int e = s + 1;
        float si = (float) s / (float) (m_count - 1);
        float ei = (float) e / (float) (m_count - 1);
        float d = i - si;
        float td = ei - si;
        float li = d / td;
        v3f result = m_frames[e].minus(m_frames[s]);
        return m_frames[s].plus(result.times(li));
    }

    bool isDone() const { return m_elapsed >= m_duration; }

private:
    std::vector<v3f> m_frames;
    int m_count = 0;
    float m_duration = 0.0f;
    float m_elapsed = 0.0f;
};
