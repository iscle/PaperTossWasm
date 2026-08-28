// Port of com.bfs.papertoss.platform.SoundMgr
//
// Android used SoundPool for the one-shots and MediaPlayer for the ambient
// loop; SDL_mixer maps onto both (and decodes Ogg Vorbis itself, so it does not
// matter whether the browser can play Vorbis natively).
#pragma once

#include <map>
#include <string>

#include <SDL_mixer.h>

class SoundMgr {
public:
    SoundMgr();
    void startBackgroundSound();
    void stopBackgroundSound();

private:
    void playSound(const std::string& name);
    void setBackgroundSound(const std::string& name);
    void preloadAllSounds();

    std::map<std::string, Mix_Chunk*> m_chunks;
    Mix_Music* m_player = nullptr;
    std::string m_current_background_sound;
    bool m_audio_ready = false;

public:
    static bool m_sound;
};
