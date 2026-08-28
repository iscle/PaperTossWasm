#include "soundmgr.h"

#include <cstdio>
#include <dirent.h>

#include <SDL.h>

#include "platform.h"

bool SoundMgr::m_sound = true;

namespace {

const char* kSoundDir = "/assets/sounds";

std::string replaceExtension(const std::string& name, const std::string& from, const std::string& to) {
    std::string result = name;
    size_t at = result.rfind(from);
    if (at != std::string::npos && at + from.size() == result.size()) result.replace(at, from.size(), to);
    return result;
}

// The ambient loops live in res/raw on Android; they are packaged under /music.
const char* musicFileFor(const std::string& name) {
    if (name.find("OfficeNoise") != std::string::npos) return "/music/officenoise.ogg";
    if (name.find("AirportNoise") != std::string::npos) return "/music/airportnoise.ogg";
    if (name.find("BasementAmbient") != std::string::npos) return "/music/basementambient.ogg";
    if (name.find("Bathroom Background") != std::string::npos) return "/music/bathroombackground.ogg";
    return nullptr;
}

}  // namespace

SoundMgr::SoundMgr() {
    if (SDL_Init(SDL_INIT_AUDIO) != 0) {
        std::printf("SoundMgr: SDL_Init(AUDIO) failed: %s\n", SDL_GetError());
        return;
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) != 0) {
        std::printf("SoundMgr: Mix_OpenAudio failed: %s\n", Mix_GetError());
        return;
    }
    Mix_AllocateChannels(8);
    m_audio_ready = true;
    preloadAllSounds();

    Evt& evt = Evt::getInstance();
    evt.subscribe("paperTossPlaySound", [this](const EvtArg& a) { playSound(a.s); });
    evt.subscribe("setBackgroundSound", [this](const EvtArg& a) { setBackgroundSound(a.s); });
    evt.subscribe("setSound", [this](const EvtArg& a) {
        m_sound = a.b;
        if (m_sound) {
            startBackgroundSound();
        } else {
            stopBackgroundSound();
        }
    });
}

void SoundMgr::preloadAllSounds() {
    DIR* dir = opendir(kSoundDir);
    if (dir == nullptr) {
        std::printf("SoundMgr: could not locate sounds directory\n");
        return;
    }
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (name == "." || name == "..") continue;
        std::string path = std::string(kSoundDir) + "/" + name;
        Mix_Chunk* chunk = Mix_LoadWAV(path.c_str());
        if (chunk == nullptr) {
            std::printf("SoundMgr: could not pre-load sound %s\n", name.c_str());
            continue;
        }
        m_chunks["sounds/" + name] = chunk;
    }
    closedir(dir);
    std::printf("SoundMgr: %d sound effects ready\n", (int) m_chunks.size());
}

void SoundMgr::playSound(const std::string& name) {
    if (!m_sound || !m_audio_ready || name.empty()) return;
    std::string filename = replaceExtension("sounds/" + name, ".wav", ".OGG");
    auto it = m_chunks.find(filename);
    if (it == m_chunks.end()) {
        // Some names in the level data have no matching asset (the original
        // logged the same miss).
        std::printf("SoundMgr: no such sound %s\n", filename.c_str());
        m_chunks[filename] = nullptr;
        return;
    }
    if (it->second == nullptr) return;
    Mix_PlayChannel(-1, it->second, 0);
}

void SoundMgr::setBackgroundSound(const std::string& name) {
    if (!m_audio_ready || name.empty()) return;
    std::string filename = replaceExtension("sounds/" + name, ".mp3", ".OGG");
    const char* music_file = musicFileFor(filename);
    if (filename == m_current_background_sound) return;
    m_current_background_sound = filename;
    stopBackgroundSound();
    if (m_sound && music_file != nullptr) {
        m_player = Mix_LoadMUS(music_file);
        if (m_player == nullptr) {
            std::printf("SoundMgr: could not play background sound %s\n", music_file);
            return;
        }
        if (Mix_PlayMusic(m_player, -1) != 0) {
            std::printf("SoundMgr: could not start %s: %s\n", music_file, Mix_GetError());
        }
    }
}

void SoundMgr::stopBackgroundSound() {
    if (m_player != nullptr) {
        Mix_HaltMusic();
        Mix_FreeMusic(m_player);
        m_player = nullptr;
    }
}

void SoundMgr::startBackgroundSound() {
    if (!m_audio_ready || !m_sound) return;
    if (Mix_PlayingMusic()) return;
    std::string current = m_current_background_sound;
    if (current.empty()) return;
    m_current_background_sound.clear();
    // setBackgroundSound() re-reads the name it was given, minus the prefix it
    // adds itself.
    setBackgroundSound(current.substr(current.find('/') + 1));
}
