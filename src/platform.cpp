#include "platform.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <sstream>

#include <emscripten.h>

// ---------------------------------------------------------------- Evt

Evt& Evt::getInstance() {
    static Evt instance;
    return instance;
}

std::vector<EvtListener>* Evt::find(const std::string& name, bool create) {
    for (auto& entry : m_listeners) {
        if (entry.first == name) return &entry.second;
    }
    if (!create) return nullptr;
    m_listeners.emplace_back(name, std::vector<EvtListener>());
    return &m_listeners.back().second;
}

void Evt::subscribe(const std::string& event_name, EvtListener listener) {
    find(event_name, true)->push_back(std::move(listener));
}

void Evt::publish(const std::string& event_name) { publish(event_name, EvtArg()); }

void Evt::publish(const std::string& event_name, const EvtArg& arg) {
    std::vector<EvtListener>* list = find(event_name, false);
    if (list == nullptr) return;
    // A listener may subscribe while we iterate, so index by position.
    for (size_t i = 0; i < list->size(); i++) {
        EvtListener listener = (*list)[i];
        listener(arg);
        list = find(event_name, false);
        if (list == nullptr) return;
    }
}

// ----------------------------------------------------------- SaveData
//
// Android serialised a HashMap into the app's private storage. The browser
// equivalent is localStorage, so the same key/group/value model is flattened
// into one string there.

namespace {

struct SaveValue {
    bool is_bool = false;
    int i = 0;
    bool b = false;
};

std::map<std::string, std::map<std::string, SaveValue>> g_save_data;
bool g_modified = false;
const char* kSaveKey = "papertoss.savedata";

EM_JS(char*, js_load_save, (const char* key), {
    var value = "";
    try {
        value = localStorage.getItem(UTF8ToString(key)) || "";
    } catch (e) {
        value = "";
    }
    var length = lengthBytesUTF8(value) + 1;
    var buffer = _malloc(length);
    stringToUTF8(value, buffer, length);
    return buffer;
});

EM_JS(void, js_store_save, (const char* key, const char* value), {
    try {
        localStorage.setItem(UTF8ToString(key), UTF8ToString(value));
    } catch (e) {
        // Private browsing modes can refuse storage; scores just will not stick.
    }
});

}  // namespace

namespace SaveData {

void load() {
    g_modified = false;
    g_save_data.clear();
    char* raw = js_load_save(kSaveKey);
    std::string text(raw ? raw : "");
    if (raw) std::free(raw);
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        // group \t key \t type \t value
        size_t a = line.find('\t');
        if (a == std::string::npos) continue;
        size_t b = line.find('\t', a + 1);
        if (b == std::string::npos) continue;
        size_t c = line.find('\t', b + 1);
        if (c == std::string::npos) continue;
        std::string group = line.substr(0, a);
        std::string key = line.substr(a + 1, b - a - 1);
        std::string type = line.substr(b + 1, c - b - 1);
        std::string value = line.substr(c + 1);
        SaveValue v;
        v.is_bool = (type == "b");
        if (v.is_bool) {
            v.b = (value == "1");
        } else {
            v.i = std::atoi(value.c_str());
        }
        g_save_data[group][key] = v;
    }
}

void save() {
    if (!g_modified) return;
    std::string text;
    for (const auto& group : g_save_data) {
        for (const auto& entry : group.second) {
            text += group.first;
            text += '\t';
            text += entry.first;
            text += '\t';
            if (entry.second.is_bool) {
                text += "b\t";
                text += entry.second.b ? "1" : "0";
            } else {
                text += "i\t";
                text += std::to_string(entry.second.i);
            }
            text += '\n';
        }
    }
    js_store_save(kSaveKey, text.c_str());
    g_modified = false;
}

void write(int value, const std::string& key, const std::string& group) {
    SaveValue v;
    v.is_bool = false;
    v.i = value;
    g_save_data[group][key] = v;
    g_modified = true;
}

void write(bool value, const std::string& key, const std::string& group) {
    SaveValue v;
    v.is_bool = true;
    v.b = value;
    g_save_data[group][key] = v;
    g_modified = true;
}

int read(int def, const std::string& key, const std::string& group) {
    auto g = g_save_data.find(group);
    if (g == g_save_data.end()) return def;
    auto k = g->second.find(key);
    if (k == g->second.end()) return def;
    return k->second.is_bool ? (k->second.b ? 1 : 0) : k->second.i;
}

bool read(bool def, const std::string& key, const std::string& group) {
    auto g = g_save_data.find(group);
    if (g == g_save_data.end()) return def;
    auto k = g->second.find(key);
    if (k == g->second.end()) return def;
    return k->second.is_bool ? k->second.b : (k->second.i != 0);
}

}  // namespace SaveData

// ------------------------------------------------------------- Random

namespace Random {

namespace {
// Java's Math.random() reseeds per process; do the same from the wall clock.
struct Seeder {
    Seeder() { std::srand((unsigned int) emscripten_get_now()); }
} g_seeder;
}  // namespace

int randomi(int low, int high) {
    int randi = (int) (((double) std::rand() / ((double) RAND_MAX + 1.0)) * 2.147483647E9);
    return (randi % ((high - low) + 1)) + low;
}

float randomf(float low, float high) {
    double r = (double) std::rand() / ((double) RAND_MAX + 1.0);
    return (float) ((r * (double) (high - low)) + (double) low);
}

}  // namespace Random

// --------------------------------------------------------------- Util

namespace Util {

int nextPowerOfTwo(int n) {
    for (int i = 1; i > 0; i *= 2) {
        if (i >= n) return i;
    }
    return 0;
}

double getTime() {
    static double start = emscripten_get_now();
    return (emscripten_get_now() - start) * 0.001;
}

float radians(float d) { return (3.1415927f * d) / 180.0f; }
float degrees(float r) { return (180.0f * r) / 3.1415927f; }

std::string format(const char* fmt, ...) {
    char buffer[256];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    return std::string(buffer);
}

}  // namespace Util

// ------------------------------------------------------------ Globals

namespace Globals {
bool HI_RES = false;
float SCALE_FACTOR = 1.0f;
int SURFACE_H = 0;
int VIEWPORT_H = 0;
int VIEWPORT_W = 0;
int VIEWPORT_X = 0;
int VIEWPORT_Y = 0;
TextureMgr* texture_mgr = nullptr;
bool first_launch = true;
SoundMgr* soundMgr = nullptr;
int STARTS_ANY_VERSION = 0;
}  // namespace Globals
