// Port of com.bfs.papertoss.cpp.Papertoss
#pragma once

class Level;

namespace Papertoss {

enum class GameState { MENU, MENU_TO_SCORE, SCORE, SCORE_TO_MENU, MENU_TO_LEVEL, LEVEL, LEVEL_TO_MENU };

extern GameState state;
extern Level* level;
extern bool m_shutdown;

void sizeGl();
bool initialize();
void update(double elapsed);
void render();
void shutdown();
void unShutdown();
void activate();
void setSound(bool sound);
bool getSound();

}  // namespace Papertoss
