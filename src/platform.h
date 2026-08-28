// Port of com.bfs.papertoss.platform.*
#pragma once

#include <functional>
#include <string>
#include <vector>

#include "vec.h"

namespace Config {
constexpr float ADJUSTED_ORTHO_WIDTH = 294.25287f;
constexpr float BACKGROUND_DEPTH = 449.9f;
constexpr float ORTHO_ADJUSTMENT_F = 12.873565f;
constexpr int SCREEN_DEPTH = 450;
constexpr int SCREEN_HEIGHT = 480;
constexpr float SCREEN_HEIGHT_F = 480.0f;
constexpr int SCREEN_WIDTH = 320;
constexpr float SCREEN_WIDTH_F = 320.0f;
}  // namespace Config

// The payload Java passed around as Object.
struct EvtArg {
    enum Kind { NONE, INT, BOOL, STR, V2 };
    Kind kind = NONE;
    int i = 0;
    bool b = false;
    std::string s;
    v2f v;
    EvtArg() {}
    EvtArg(int a) : kind(INT), i(a) {}
    EvtArg(bool a) : kind(BOOL), b(a) {}
    EvtArg(const char* a) : kind(STR), s(a ? a : "") {}
    EvtArg(const std::string& a) : kind(STR), s(a) {}
    EvtArg(const v2f& a) : kind(V2), v(a) {}
};

using EvtListener = std::function<void(const EvtArg&)>;

class Evt {
public:
    static Evt& getInstance();
    void subscribe(const std::string& event_name, EvtListener listener);
    void publish(const std::string& event_name);
    void publish(const std::string& event_name, const EvtArg& arg);

private:
    std::vector<std::pair<std::string, std::vector<EvtListener>>> m_listeners;
    std::vector<EvtListener>* find(const std::string& name, bool create);
};

namespace SaveData {
void load();
void save();
void write(int value, const std::string& key, const std::string& group = "DEFAULT_GROUP");
void write(bool value, const std::string& key, const std::string& group = "DEFAULT_GROUP");
int read(int def, const std::string& key, const std::string& group = "DEFAULT_GROUP");
bool read(bool def, const std::string& key, const std::string& group = "DEFAULT_GROUP");
}  // namespace SaveData

namespace Random {
int randomi(int low, int high);
float randomf(float low, float high);
}  // namespace Random

namespace Util {
int nextPowerOfTwo(int n);
double getTime();
float radians(float d);
float degrees(float r);
// Java printf-style helpers used for the on-screen strings.
std::string format(const char* fmt, ...);
}  // namespace Util

class TextureMgr;
class SoundMgr;

namespace Globals {
extern bool HI_RES;
extern float SCALE_FACTOR;
extern int SURFACE_H;
extern int VIEWPORT_H;
extern int VIEWPORT_W;
extern int VIEWPORT_X;
extern int VIEWPORT_Y;
extern TextureMgr* texture_mgr;
extern bool first_launch;
extern SoundMgr* soundMgr;
extern int STARTS_ANY_VERSION;
}  // namespace Globals
