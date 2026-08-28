#include "scores.h"

#include <string>

#include "platform.h"

namespace {
std::string keyForLevel(int level_index) { return "level_" + std::to_string(level_index) + "_best"; }
std::string oldStyleKey(int level_index) { return "level_" + std::to_string(level_index); }
}  // namespace

namespace Scores {

void init() {}

void saveBest(int score, int level_index) {
    SaveData::write(score, keyForLevel(level_index));
    SaveData::save();
}

int readBest(int level_index) { return SaveData::read(0, keyForLevel(level_index)); }

void saveSubmitted(int score, int level_index) {
    SaveData::write(score, oldStyleKey(level_index), "submitted");
    SaveData::save();
}

int readSubmitted(int level_index) { return SaveData::read(0, oldStyleKey(level_index), "submitted"); }

}  // namespace Scores
