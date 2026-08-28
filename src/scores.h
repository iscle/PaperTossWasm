// Port of com.bfs.papertoss.cpp.Scores
#pragma once

namespace Scores {
void init();
void saveBest(int score, int level_index);
int readBest(int level_index);
void saveSubmitted(int score, int level_index);
int readSubmitted(int level_index);
}  // namespace Scores
