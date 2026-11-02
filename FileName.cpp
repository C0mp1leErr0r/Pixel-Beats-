#include "raylib.h"
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <map>

#include "assets.h"   // 内嵌资源加载（单文件 exe 支持）

const int SCREEN_WIDTH =1280;
const int SCREEN_HEIGHT = 720;
const int LANE_COUNT = 4;
const int LANE_WIDTH = SCREEN_WIDTH / LANE_COUNT;
const int JUDGE_LINE_Y = SCREEN_HEIGHT - 120;
const float DEFAULT_NOTE_SPEED = 12.f;
const float NOTE_SPEED_MIN = 4.f;
const float NOTE_SPEED_MAX = 24.f;
const int NOTE_SIZE = 64;
const int NOTE_ROUND = 10;

const int PERFECT_TIME = 40;
const int GREAT_TIME = 80;
const int GOOD_TIME = 160;

const int AUDIO_OFFSET_MIN = -400;
const int AUDIO_OFFSET_MAX = 400;
const int AUDIO_OFFSET_DEFAULT = -80;

static const Color LANE_COLORS[4] = {
    { 60, 80, 140, 80 },
    { 80, 60, 140, 80 },
    { 140, 80, 60, 80 },
    { 60, 140, 100, 80 },
};

static Sound hitSound = { 0 };

static const int HIT_SOUND_POOL_SIZE = 8;
static Sound hitSoundPool[HIT_SOUND_POOL_SIZE] = { { 0 } };
static int hitSoundPoolIndex = 0;

// UI Animation Variables
static float g_fadeAlpha = 0.0f;       // 页面切换淡入淡出
static float g_comboScale = 1.0f;      // 连击数字缩放

static void PlayHitSoundFromPool() {
    for (int i = 0; i < HIT_SOUND_POOL_SIZE; i++) {
        int idx = (hitSoundPoolIndex + i) % HIT_SOUND_POOL_SIZE;
        if (hitSoundPool[idx].frameCount > 0) {
            PlaySound(hitSoundPool[idx]);
            hitSoundPoolIndex = (idx + 1) % HIT_SOUND_POOL_SIZE;
            return;
        }
    }
}

struct Note {
    int lane;
    float time;
    bool active;
    bool judged;
};

struct Chart {
    std::string musicPath;
    std::vector<Note> notes;
    float offset;
};

struct GameState {
    int score;
    int combo;
    int maxCombo;
    int perfectCount;
    int greatCount;
    int goodCount;
    int missCount;
    bool isPlaying;
    bool gameOver;
};

struct Particle {
    Vector2 pos;
    Vector2 vel;
    float life;
    float maxLife;
    Color col;
};

struct JudgePopup {
    std::string text;
    float x;
    float y;
    float life;
    Color col;
    int fontSize;
};

struct VisualFX {
    float laneFlash[LANE_COUNT]{};
    std::vector<Particle> particles;
    std::vector<JudgePopup> popups;
};

enum class AppScreen {
    MainMenu,
    Options,
    SongSelect,
    Loading,
    Playing,
    Paused,
    GameOver,
};

enum class SortMode {
    AlphabeticalAsc,
    AlphabeticalDesc,
    ScoreAsc,
    ScoreDesc,
};

enum class GameAction {
    TapLane1,
    TapLane2,
    TapLane3,
    TapLane4,
    Pause,
    Count
};

static const char* GameActionNames[] = {
    "TapLane1",
    "TapLane2",
    "TapLane3",
    "TapLane4",
    "Pause"
};

static const char* GameActionDisplayNames[] = {
    "Lane 1",
    "Lane 2",
    "Lane 3",
    "Lane 4",
    "Pause"
};

static const int DefaultKeyBindings[] = {
    KEY_D,   // TapLane1
    KEY_F,   // TapLane2
    KEY_J,   // TapLane3
    KEY_K,   // TapLane4
    KEY_P    // Pause
};

const int NOTE_SIZE_MIN = 32;
const int NOTE_SIZE_MAX = 96;
const int NOTE_SIZE_DEFAULT = 64;

struct UserSettings {
    float masterVolume = 0.85f;
    float noteSpeed = DEFAULT_NOTE_SPEED;
    int audioOffset = AUDIO_OFFSET_DEFAULT;
    float hitSoundVolume = 1.0f;
    int noteSize = NOTE_SIZE_DEFAULT;
    std::map<GameAction, int> keyBindings;
};

struct BestRecord {
    int score;
    int maxCombo;
    float accuracy;
    int perfectCount;
    int greatCount;
    int goodCount;
    int missCount;
    char grade;
};

static const char* SETTINGS_FILE_PATH = "settings.txt";

static void SaveSettingsToFile(const UserSettings& settings, const std::map<std::pair<int, int>, BestRecord>& bestRecords) {
    std::ofstream file(SETTINGS_FILE_PATH);
    if (!file.is_open()) return;

    file << "[Settings]\n";
    file << "masterVolume=" << settings.masterVolume << "\n";
    file << "noteSpeed=" << settings.noteSpeed << "\n";
    file << "audioOffset=" << settings.audioOffset << "\n";
    file << "hitSoundVolume=" << settings.hitSoundVolume << "\n";
    file << "noteSize=" << settings.noteSize << "\n";

    file << "\n[KeyBindings]\n";
    for (int i = 0; i < (int)GameAction::Count; i++) {
        GameAction action = (GameAction)i;
        auto it = settings.keyBindings.find(action);
        int key = (it != settings.keyBindings.end()) ? it->second : DefaultKeyBindings[i];
        file << GameActionNames[i] << "=" << key << "\n";
    }

    file << "\n[BestRecords]\n";
    for (const auto& entry : bestRecords) {
        int songIdx = entry.first.first;
        int diffIdx = entry.first.second;
        const BestRecord& rec = entry.second;
        file << songIdx << "," << diffIdx << ","
             << rec.score << ","
             << rec.maxCombo << ","
             << rec.accuracy << ","
             << rec.perfectCount << ","
             << rec.greatCount << ","
             << rec.goodCount << ","
             << rec.missCount << ","
             << rec.grade << "\n";
    }

    file.close();
}

static void LoadSettingsFromFile(UserSettings& settings, std::map<std::pair<int, int>, BestRecord>& bestRecords) {
    std::ifstream file(SETTINGS_FILE_PATH);
    if (!file.is_open()) return;

    std::string line;
    bool inBestRecords = false;
    bool inKeyBindings = false;

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        if (line == "[BestRecords]") {
            inBestRecords = true;
            inKeyBindings = false;
            continue;
        }

        if (line == "[KeyBindings]") {
            inKeyBindings = true;
            inBestRecords = false;
            continue;
        }

        if (line == "[Settings]") {
            inBestRecords = false;
            inKeyBindings = false;
            continue;
        }

        if (inBestRecords) {
            std::istringstream iss(line);
            std::string token;
            std::vector<std::string> parts;
            while (std::getline(iss, token, ',')) {
                parts.push_back(token);
            }
            if (parts.size() >= 10) {
                BestRecord rec;
                rec.score = std::stoi(parts[2]);
                rec.maxCombo = std::stoi(parts[3]);
                rec.accuracy = std::stof(parts[4]);
                rec.perfectCount = std::stoi(parts[5]);
                rec.greatCount = std::stoi(parts[6]);
                rec.goodCount = std::stoi(parts[7]);
                rec.missCount = std::stoi(parts[8]);
                rec.grade = parts[9][0];
                int songIdx = std::stoi(parts[0]);
                int diffIdx = std::stoi(parts[1]);
                bestRecords[{songIdx, diffIdx}] = rec;
            }
        }
        else {
            size_t eqPos = line.find('=');
            if (eqPos != std::string::npos) {
                std::string key = line.substr(0, eqPos);
                std::string value = line.substr(eqPos + 1);
                
                if (inKeyBindings) {
                    for (int i = 0; i < (int)GameAction::Count; i++) {
                        if (key == GameActionNames[i]) {
                            settings.keyBindings[(GameAction)i] = std::stoi(value);
                            break;
                        }
                    }
                }
                else {
                    if (key == "masterVolume") {
                        settings.masterVolume = std::stof(value);
                    }
                    else if (key == "noteSpeed") {
                        settings.noteSpeed = std::stof(value);
                    }
                    else if (key == "audioOffset") {
                        settings.audioOffset = std::stoi(value);
                    }
                    else if (key == "hitSoundVolume") {
                        settings.hitSoundVolume = std::stof(value);
                    }
                    else if (key == "noteSize") {
                        settings.noteSize = std::stoi(value);
                    }
                }
            }
        }
    }

    file.close();
}

static void InitDefaultKeyBindings(std::map<GameAction, int>& keyBindings) {
    for (int i = 0; i < (int)GameAction::Count; i++) {
        keyBindings[(GameAction)i] = DefaultKeyBindings[i];
    }
}

static void ValidateKeyBindings(std::map<GameAction, int>& keyBindings) {
    for (int i = 0; i < (int)GameAction::Count; i++) {
        GameAction action = (GameAction)i;
        auto it = keyBindings.find(action);
        if (it == keyBindings.end() || it->second <= 0) {
            keyBindings[action] = DefaultKeyBindings[i];
        }
    }
    
    for (int i = 0; i < (int)GameAction::Count; i++) {
        for (int j = i + 1; j < (int)GameAction::Count; j++) {
            GameAction action1 = (GameAction)i;
            GameAction action2 = (GameAction)j;
            if (keyBindings[action1] == keyBindings[action2]) {
                keyBindings[action2] = DefaultKeyBindings[j];
            }
        }
    }
}

static int GetKeyBinding(const std::map<GameAction, int>& keyBindings, GameAction action) {
    auto it = keyBindings.find(action);
    if (it != keyBindings.end()) {
        return it->second;
    }
    return DefaultKeyBindings[(int)action];
}

struct SongEntry {
    const char* displayName;
    /** Index 0 = Easy, 1 = Normal, 2 = Hard */
    const char* chartPaths[3];
};

static const SongEntry kSongCatalog[] = {
    { "ARForest - Inverted World (Extended Mix)",
      { "charts/chart_easy.txt", "charts/chart_normal.txt", "charts/chart_hard.txt" } },
    { "Reku Mochizuki - INFiNiTE ENERZY -Overdoze-",
      { "charts/chart1_easy.txt", "charts/chart1_normal.txt", "charts/chart1_hard.txt" } },
    { "Luminescence",
      { "charts/chart2_easy.txt", "charts/chart2_normal.txt", "charts/chart2_hard.txt" } },
    { "Distorted Fate",
      { "charts/chart3_easy.txt", "charts/chart3_normal.txt", "charts/chart3_hard.txt" } },
    { "Lia - Poem of Bird",
      { "charts/chart_poem_of_bird_easy.txt", "charts/chart_poem_of_bird_normal.txt", "charts/chart_poem_of_bird_hard.txt" } },
    { "The Wind Forest",
      { "charts/chart_wind_forest_easy.txt", "charts/chart_wind_forest_normal.txt", "charts/chart_wind_forest_hard.txt" } },
    { "Summer",
      { "charts/chart_summer_easy.txt", "charts/chart_summer_normal.txt", "charts/chart_summer_hard.txt" } },
};

static const char* kDifficultyLabels[] = { "EASY", "NORMAL", "HARD" };

// Song Select Scroll Constants
const int SONG_SELECT_VISIBLE_ROWS = 5;
const int SONG_SELECT_ROW_HEIGHT = 56;
const int SONG_SELECT_Y0 = 248;
const int SONG_SELECT_SCROLLBAR_WIDTH = 12;
const int SONG_SELECT_SCROLLBAR_MARGIN = 20;

struct SongSelectState {
    float scrollOffset;
    SortMode sortMode;
    int selectedDifficultyIndex;
    int selectedSongIndex;
};

static int GetSortedSongIndex(int visualIndex, SortMode sortMode, int difficultyIndex, const std::map<std::pair<int, int>, BestRecord>& bestRecords);
static float GetMaxScrollOffset(int songCount);

static const char* SongChartPath(const SongEntry& song, int difficultyIndex) {
    int i = difficultyIndex % 3;
    if (i < 0) i += 3;
    return song.chartPaths[i];
}

static Rectangle SongSelectDifficultyHitRect(int difficultyIndex) {
    const int btnW = 132;
    const int btnH = 42;
    const int gap = 14;
    const int y = 168;
    int total = 3 * btnW + 2 * gap;
    int x0 = SCREEN_WIDTH / 2 - total / 2;
    int x = x0 + difficultyIndex * (btnW + gap);
    return { (float)x, (float)y, (float)btnW, (float)btnH };
}

static float GetMaxScrollOffset(int songCount) {
    int totalHeight = songCount * SONG_SELECT_ROW_HEIGHT;
    int viewportHeight = SONG_SELECT_VISIBLE_ROWS * SONG_SELECT_ROW_HEIGHT;
    return (float)std::max(0, totalHeight - viewportHeight);
}

static int GetSortedSongIndex(int visualIndex, SortMode sortMode, int difficultyIndex, const std::map<std::pair<int, int>, BestRecord>& bestRecords) {
    int songCount = (int)(sizeof(kSongCatalog) / sizeof(kSongCatalog[0]));
    
    std::vector<int> indices(songCount);
    for (int i = 0; i < songCount; i++) {
        indices[i] = i;
    }
    
    switch (sortMode) {
        case SortMode::AlphabeticalAsc:
            std::sort(indices.begin(), indices.end(), [](int a, int b) {
                return strcmp(kSongCatalog[a].displayName, kSongCatalog[b].displayName) < 0;
            });
            break;
        case SortMode::AlphabeticalDesc:
            std::sort(indices.begin(), indices.end(), [](int a, int b) {
                return strcmp(kSongCatalog[a].displayName, kSongCatalog[b].displayName) > 0;
            });
            break;
        case SortMode::ScoreAsc:
            std::sort(indices.begin(), indices.end(), [&](int a, int b) {
                auto keyA = std::make_pair(a, difficultyIndex);
                auto keyB = std::make_pair(b, difficultyIndex);
                int scoreA = bestRecords.count(keyA) ? bestRecords.at(keyA).score : 0;
                int scoreB = bestRecords.count(keyB) ? bestRecords.at(keyB).score : 0;
                return scoreA < scoreB;
            });
            break;
        case SortMode::ScoreDesc:
            std::sort(indices.begin(), indices.end(), [&](int a, int b) {
                auto keyA = std::make_pair(a, difficultyIndex);
                auto keyB = std::make_pair(b, difficultyIndex);
                int scoreA = bestRecords.count(keyA) ? bestRecords.at(keyA).score : 0;
                int scoreB = bestRecords.count(keyB) ? bestRecords.at(keyB).score : 0;
                return scoreA > scoreB;
            });
            break;
    }
    
    return indices[visualIndex];
}

static const char* GetSortModeLabel(SortMode mode) {
    switch (mode) {
        case SortMode::AlphabeticalAsc: return "A-Z";
        case SortMode::AlphabeticalDesc: return "Z-A";
        case SortMode::ScoreAsc: return "Score UP";
        case SortMode::ScoreDesc: return "Score DOWN";
    }
    return "";
}

void CheckNote(Chart& chart, GameState& gameState, int lane, VisualFX& fx, float noteSpeed, Music& music, bool musicLoaded, int audioOffset);
static void AddHitParticles(VisualFX& fx, int lane, Color tint, int count);
static void AddJudgePopup(VisualFX& fx, int lane, const char* text, Color col, int fontSize);
static void UpdateVisualFX(VisualFX& fx, float dt);
static void DrawVisualFX(const VisualFX& fx);
static int LaneCenterX(int lane) { return lane * LANE_WIDTH + LANE_WIDTH / 2; }

/** Chart lane indices must be in [0, LANE_COUNT). Out-of-range values corrupt laneFlash[]. */
static int SanitizeLane(int lane) {
    if (lane < 0) return 0;
    if (lane >= LANE_COUNT) return LANE_COUNT - 1;
    return lane;
}

Chart LoadChart(const std::string& chartPath) {
    Chart chart;

    // 优先读取磁盘上的谱面文件；不存在时使用内嵌在 exe 中的默认谱面
    std::string chartText = LoadTextAsset(chartPath.c_str());
    if (chartText.empty()) {
        return chart;
    }

    std::istringstream file(chartText);
    std::string line;

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string token;

        if (line.size() >= 6 && line.substr(0, 6) == "music:") {
            chart.musicPath = line.substr(6);
            if (!chart.musicPath.empty()) {
                while (!chart.musicPath.empty() && (chart.musicPath.front() == ' ' || chart.musicPath.front() == '\t'))
                    chart.musicPath.erase(0, 1);
                while (!chart.musicPath.empty() && (chart.musicPath.back() == ' ' || chart.musicPath.back() == '\t'))
                    chart.musicPath.pop_back();
            }
        }
        else if (line.size() >= 7 && line.substr(0, 7) == "offset:") {
            chart.offset = std::stof(line.substr(7));
        }
        else {
            std::vector<std::string> parts;
            while (std::getline(iss, token, ',')) {
                parts.push_back(token);
            }

            if (parts.size() == 2) {
                Note note;
                note.lane = SanitizeLane(std::stoi(parts[0]));
                note.time = std::stof(parts[1]);
                note.active = true;
                note.judged = false;
                chart.notes.push_back(note);
            }
        }
    }

    return chart;
}

static float ComputeNoteY(float noteTimeMs, float musicTimeMs, float noteSpeed) {
    float speedPerMs = noteSpeed * 60.f / 1000.f;
    float timeUntilNote = noteTimeMs - musicTimeMs;
    return (float)JUDGE_LINE_Y - timeUntilNote * speedPerMs;
}

/** Judged taps so far (includes misses). */
static int CountJudgedNotes(const GameState& gs) {
    return gs.perfectCount + gs.greatCount + gs.goodCount + gs.missCount;
}

/** 0–100: weighted by judgment (Perfect 100, Great 85, Good 65, Miss 0). 100% before any note is judged. */
static float ComputeAccuracyPercent(const GameState& gs) {
    const int n = CountJudgedNotes(gs);
    if (n <= 0) return 100.f;
    const float pts = gs.perfectCount * 100.f + gs.greatCount * 85.f + gs.goodCount * 65.f;
    const float pct = pts / ((float)n * 100.f) * 100.f;
    return std::max(0.f, std::min(100.f, pct));
}

/** 0–100: longest combo streak vs total notes in chart (full combo => 100). */
static float ComputeComboScore(const GameState& gs, int totalChartNotes) {
    if (totalChartNotes <= 0) return 0.f;
    const float pct = 100.f * (float)gs.maxCombo / (float)totalChartNotes;
    return std::max(0.f, std::min(100.f, pct));
}

/** Blend used for letter grade (accuracy weighted a bit higher than combo). */
static float ComputeOverallRatingValue(float accuracyPct, float comboScore) {
    return 0.85f * accuracyPct + 0.15f * comboScore;
}

static const char* GradeLetterFromOverall(float overall) {
    if (overall >= 95.f) return "S";
    if (overall >= 85.f) return "A";
    if (overall >= 75.f) return "B";
    if (overall >= 65.f) return "C";
    return "D";
}

static Color GradeDisplayColor(const char* grade) {
    if (grade[0] == 'S') return Color{ 255, 225, 100, 255 };
    if (grade[0] == 'A') return Color{ 130, 255, 170, 255 };
    if (grade[0] == 'B') return Color{ 120, 210, 255, 255 };
    if (grade[0] == 'C') return Color{ 255, 170, 120, 255 };
    return Color{ 190, 150, 165, 255 };
}

static void DrawLaneDecor(int lane, float flashAlpha, bool keyDown) {
    int x = lane * LANE_WIDTH;
    Color base = LANE_COLORS[lane % LANE_COUNT];
    if (keyDown) {
        base.a = (unsigned char)std::min(220, (int)base.a + 100);
    }
    DrawRectangle(x, 0, LANE_WIDTH, SCREEN_HEIGHT, base);

    if (flashAlpha > 0.01f) {
        Color flash = WHITE;
        flash.a = (unsigned char)(flashAlpha * 200.f);
        DrawRectangle(x, JUDGE_LINE_Y - 120, LANE_WIDTH, 160, flash);
    }

    DrawRectangleLines(x, 0, LANE_WIDTH, SCREEN_HEIGHT,
        Color{ 255, 255, 255, 40 });
}

static void DrawHudPanel(const GameState& gs, int totalChartNotes) {
    int panelW = 340;
    int panelH = 154;
    DrawRectangle(10, 10, panelW, panelH, Color{ 0, 0, 0, 140 });
    DrawRectangleLines(10, 10, panelW, panelH, Color{ 255, 255, 255, 60 });

    DrawText(TextFormat("SCORE  %09d", gs.score), 24, 22, 22, RAYWHITE);
    int baseComboSize = 22 + std::min(28, gs.combo / 5);
    int comboSize = (int)(baseComboSize * g_comboScale);
    Color comboCol = gs.combo >= 20 ? GOLD : (gs.combo >= 10 ? SKYBLUE : RAYWHITE);
    
    std::string comboText = TextFormat("COMBO  x%d", gs.combo);
    int tw = MeasureText(comboText.c_str(), baseComboSize);
    int x = 24 + (int)((tw - MeasureText(comboText.c_str(), comboSize)) * 0.5f);
    int y = 54 + (int)((baseComboSize - comboSize) * 0.3f);
    
    DrawText(comboText.c_str(), x, y, comboSize, comboCol);
    DrawText(TextFormat("MAX    x%d", gs.maxCombo), 24, 92, 18, Color{ 200, 200, 200, 255 });
}

static void DrawJudgeStats(const GameState& gs, int totalChartNotes) {
    int px = SCREEN_WIDTH - 260;
    DrawRectangle(px - 10, 10, 250, 166, Color{ 0, 0, 0, 120 });
    DrawText(TextFormat("PERFECT  %4d", gs.perfectCount), px, 20, 20, Color{ 255, 230, 120, 255 });
    DrawText(TextFormat("GREAT    %4d", gs.greatCount), px, 46, 20, Color{ 120, 200, 255, 255 });
    DrawText(TextFormat("GOOD     %4d", gs.goodCount), px, 72, 20, Color{ 140, 255, 140, 255 });
    DrawText(TextFormat("MISS     %4d", gs.missCount), px, 98, 20, Color{ 255, 120, 120, 255 });
    float acc = ComputeAccuracyPercent(gs);
    float comboScr = ComputeComboScore(gs, totalChartNotes);
    DrawText(TextFormat("Accuracy %5.2f%%", acc), px, 124, 18, Color{ 200, 220, 255, 255 });
    DrawText(TextFormat("ComboScr %4.0f/100", comboScr), px, 146, 18, Color{ 170, 210, 190, 255 });
}

static const char* GetKeyName(int key) {
    static char buffer[32];
    if (key >= KEY_A && key <= KEY_Z) {
        buffer[0] = (char)('A' + key - KEY_A);
        buffer[1] = '\0';
    }
    else if (key >= KEY_ZERO && key <= KEY_NINE) {
        buffer[0] = (char)('0' + key - KEY_ZERO);
        buffer[1] = '\0';
    }
    else if (key >= KEY_KP_0 && key <= KEY_KP_9) {
        snprintf(buffer, sizeof(buffer), "KP%d", key - KEY_KP_0);
    }
    else {
        switch (key) {
            case KEY_SPACE: return "SPACE";
            case KEY_ENTER: return "ENTER";
            case KEY_TAB: return "TAB";
            case KEY_BACKSPACE: return "BKSP";
            case KEY_INSERT: return "INS";
            case KEY_DELETE: return "DEL";
            case KEY_RIGHT: return "RIGHT";
            case KEY_LEFT: return "LEFT";
            case KEY_DOWN: return "DOWN";
            case KEY_UP: return "UP";
            case KEY_PAGE_UP: return "PGUP";
            case KEY_PAGE_DOWN: return "PGDN";
            case KEY_HOME: return "HOME";
            case KEY_END: return "END";
            case KEY_CAPS_LOCK: return "CAPS";
            case KEY_SCROLL_LOCK: return "SCRL";
            case KEY_NUM_LOCK: return "NUM";
            case KEY_PRINT_SCREEN: return "PRT";
            case KEY_PAUSE: return "PAUS";
            case KEY_F1: return "F1";
            case KEY_F2: return "F2";
            case KEY_F3: return "F3";
            case KEY_F4: return "F4";
            case KEY_F5: return "F5";
            case KEY_F6: return "F6";
            case KEY_F7: return "F7";
            case KEY_F8: return "F8";
            case KEY_F9: return "F9";
            case KEY_F10: return "F10";
            case KEY_F11: return "F11";
            case KEY_F12: return "F12";
            case KEY_LEFT_SHIFT: return "LSH";
            case KEY_LEFT_CONTROL: return "LCT";
            case KEY_LEFT_ALT: return "LALT";
            case KEY_LEFT_SUPER: return "LSUP";
            case KEY_RIGHT_SHIFT: return "RSH";
            case KEY_RIGHT_CONTROL: return "RCT";
            case KEY_RIGHT_ALT: return "RALT";
            case KEY_RIGHT_SUPER: return "RSUP";
            case KEY_ESCAPE: return "ESC";
            default:
                snprintf(buffer, sizeof(buffer), "KEY_%d", key);
                return buffer;
        }
    }
    return buffer;
}

static void DrawKeyHints(const std::map<GameAction, int>& keyBindings) {
    int laneKeys[4] = {
        GetKeyBinding(keyBindings, GameAction::TapLane1),
        GetKeyBinding(keyBindings, GameAction::TapLane2),
        GetKeyBinding(keyBindings, GameAction::TapLane3),
        GetKeyBinding(keyBindings, GameAction::TapLane4)
    };
    
    for (int i = 0; i < LANE_COUNT; i++) {
        int cx = LaneCenterX(i);
        bool down = IsKeyDown(laneKeys[i]);
        int boxW = 44;
        int boxH = 36;
        Color fill = down ? Color{ 255, 255, 255, 200 } : Color{ 30, 30, 45, 200 };
        DrawRectangleRounded(
            Rectangle{ (float)(cx - boxW / 2), (float)(SCREEN_HEIGHT - 70), (float)boxW, (float)boxH },
            0.35f, 8, fill);
        DrawRectangleRoundedLinesEx(
            Rectangle{ (float)(cx - boxW / 2), (float)(SCREEN_HEIGHT - 70), (float)boxW, (float)boxH },
            0.35f, 8, 2.f, Color{ 200, 200, 255, 255 });
        const char* keyName = GetKeyName(laneKeys[i]);
        int tw = MeasureText(keyName, 22);
        DrawText(keyName, cx - tw / 2, SCREEN_HEIGHT - 62, 22, RAYWHITE);
    }
    DrawText("Mouse / tap lanes  |  Customizable keys", 24, SCREEN_HEIGHT - 28, 16, Color{ 180, 180, 200, 255 });
}

static void DrawGameOverOverlay(const GameState& gs, int totalNotes, bool isNewRecord) {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Color{ 0, 0, 0, 130 });

    float acc = ComputeAccuracyPercent(gs);
    float comboScr = ComputeComboScore(gs, totalNotes);
    float overall = ComputeOverallRatingValue(acc, comboScr);
    const char* grade = GradeLetterFromOverall(overall);
    Color gcol = GradeDisplayColor(grade);

    int cx = SCREEN_WIDTH / 2;
    int gsize = 110;
    int gw = MeasureText(grade, gsize);
    DrawText(grade, cx - gw / 2, SCREEN_HEIGHT / 2 - 218, gsize, gcol);

    const char* sub = "RESULT";
    int sw = MeasureText(sub, 28);
    DrawText(sub, cx - sw / 2, SCREEN_HEIGHT / 2 - 88, 28, GOLD);

    if (isNewRecord) {
        const char* newRecordText = "NEW RECORD!";
        int nrw = MeasureText(newRecordText, 36);
        DrawText(newRecordText, cx - nrw / 2, SCREEN_HEIGHT / 2 - 148, 36, Color{ 255, 200, 100, 255 });
    }

    int y = SCREEN_HEIGHT / 2 - 48;
    DrawText(TextFormat("Accuracy      %5.2f%%", acc), cx - 168, y, 22, RAYWHITE);
    y += 28;
    DrawText(TextFormat("Combo score   %4.0f / 100", comboScr), cx - 168, y, 22, SKYBLUE);
    y += 28;
    DrawText(TextFormat("Overall       %5.1f / 100  (%s rank)", overall, grade), cx - 168, y, 20, Color{ 200, 200, 220, 255 });
    y += 36;
    DrawText(TextFormat("Final Score   %d", gs.score), cx - 140, y, 24, RAYWHITE);
    y += 30;
    DrawText(TextFormat("Max Combo     x%d", gs.maxCombo), cx - 140, y, 22, LIGHTGRAY);
    y += 44;
    DrawText("R  Play this song again", cx - 150, y, 20, LIGHTGRAY);
    y += 26;
    DrawText("M  Main menu", cx - 90, y, 20, LIGHTGRAY);
}

void DrawGame(const Chart& chart, const GameState& gameState, const VisualFX& fx, float noteSpeed, int noteSize, Music& music, bool musicLoaded, int audioOffset, const std::map<GameAction, int>& keyBindings) {
    DrawRectangleGradientV(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
        Color{ 18, 16, 40, 255 },
        Color{ 8, 6, 22, 255 });

    bool keyDown[4] = {
        IsKeyDown(GetKeyBinding(keyBindings, GameAction::TapLane1)),
        IsKeyDown(GetKeyBinding(keyBindings, GameAction::TapLane2)),
        IsKeyDown(GetKeyBinding(keyBindings, GameAction::TapLane3)),
        IsKeyDown(GetKeyBinding(keyBindings, GameAction::TapLane4))
    };

    for (int i = 0; i < LANE_COUNT; i++) {
        float flash = fx.laneFlash[i];
        DrawLaneDecor(i, flash, keyDown[i]);
    }

    for (int i = 1; i < LANE_COUNT; i++) {
        DrawLine(i * LANE_WIDTH, 0, i * LANE_WIDTH, SCREEN_HEIGHT, Color{ 255, 255, 255, 35 });
    }

    for (int g = 0; g < 4; g++) {
        float t = 1.f - g * 0.18f;
        Color c = { 255, 100, 180, (unsigned char)(40 - g * 8) };
        DrawLineEx(
            Vector2{ 0, (float)JUDGE_LINE_Y + g * 0.5f },
            Vector2{ (float)SCREEN_WIDTH, (float)JUDGE_LINE_Y + g * 0.5f },
            3.f * t, c);
    }
    DrawLineEx(
        Vector2{ 0, (float)JUDGE_LINE_Y },
        Vector2{ (float)SCREEN_WIDTH, (float)JUDGE_LINE_Y },
        2.f, Color{ 255, 255, 255, 220 });

    float musicTimeMs = 0.f;
    if (musicLoaded) {
        musicTimeMs = GetMusicTimePlayed(music) * 1000.f;
    }

    for (const auto& note : chart.notes) {
        if (note.active && !note.judged) {
            float noteTimeMs = note.time - chart.offset + (float)audioOffset;
            float noteY = ComputeNoteY(noteTimeMs, musicTimeMs, noteSpeed);
            
            int x = note.lane * LANE_WIDTH + LANE_WIDTH / 2 - noteSize / 2;
            int y = (int)noteY;
            float noteRound = (float)noteSize / 6.4f;
            Color core = Color{ 120, 200, 255, 255 };
            Color rim = Color{ 40, 90, 200, 255 };
            DrawRectangleRounded(
                Rectangle{ (float)x - 2, (float)y - 2, (float)noteSize + 4, (float)noteSize + 4 },
                0.25f, 10, Color{ 255, 255, 255, 40 });
            DrawRectangleRounded(
                Rectangle{ (float)x, (float)y, (float)noteSize, (float)noteSize },
                0.25f, noteRound, core);
            DrawRectangleRoundedLinesEx(
                Rectangle{ (float)x, (float)y, (float)noteSize, (float)noteSize },
                0.25f, noteRound, 2.f, rim);
        }
    }

    int totalNotes = (int)chart.notes.size();
    DrawHudPanel(gameState, totalNotes);
    DrawJudgeStats(gameState, totalNotes);
    DrawKeyHints(keyBindings);
    DrawVisualFX(fx);
}

static void AddHitParticles(VisualFX& fx, int lane, Color tint, int count) {
    float cx = (float)LaneCenterX(lane);
    float cy = (float)JUDGE_LINE_Y;
    for (int i = 0; i < count; i++) {
        float ang = (float)GetRandomValue(-160, 160) * DEG2RAD;
        float spd = (float)GetRandomValue(120, 320) / 60.f;
        Particle p;
        p.pos = { cx + (float)GetRandomValue(-10, 10), cy + (float)GetRandomValue(-6, 6) };
        p.vel = { cosf(ang) * spd, sinf(ang) * spd - 1.5f };
        p.maxLife = (float)GetRandomValue(18, 35) / 60.f;
        p.life = p.maxLife;
        p.col = tint;
        p.col.r = (unsigned char)std::min(255, (int)p.col.r + GetRandomValue(-20, 40));
        p.col.g = (unsigned char)std::min(255, (int)p.col.g + GetRandomValue(-20, 40));
        p.col.b = (unsigned char)std::min(255, (int)p.col.b + GetRandomValue(-20, 40));
        fx.particles.push_back(p);
    }
}

static void AddJudgePopup(VisualFX& fx, int lane, const char* text, Color col, int fontSize) {
    JudgePopup jp;
    jp.text = text;
    jp.x = (float)LaneCenterX(lane);
    jp.y = (float)JUDGE_LINE_Y - 40.f;
    jp.life = 0.55f;
    jp.col = col;
    jp.fontSize = fontSize;
    fx.popups.push_back(jp);
}

static void UpdateVisualFX(VisualFX& fx, float dt) {
    for (int i = 0; i < LANE_COUNT; i++) {
        if (fx.laneFlash[i] > 0.f) {
            fx.laneFlash[i] = std::max(0.f, fx.laneFlash[i] - dt * 3.2f);
        }
    }

    for (auto& p : fx.particles) {
        p.pos.x += p.vel.x;
        p.pos.y += p.vel.y;
        p.vel.y += 4.f * dt;
        p.life -= dt;
    }
    fx.particles.erase(
        std::remove_if(fx.particles.begin(), fx.particles.end(), [](const Particle& p) { return p.life <= 0.f; }),
        fx.particles.end());

    for (auto& j : fx.popups) {
        j.y -= 60.f * dt;      // 匀速上升
        j.life -= dt;          // 生命值减少
    }
    fx.popups.erase(
        std::remove_if(fx.popups.begin(), fx.popups.end(), [](const JudgePopup& j) { return j.life <= 0.f; }),
        fx.popups.end());
}

static void DrawVisualFX(const VisualFX& fx) {
    for (const auto& p : fx.particles) {
        float a = std::max(0.f, p.life / p.maxLife);
        Color c = p.col;
        c.a = (unsigned char)(a * 255.f);
        DrawCircleV(p.pos, 5.f * a + 2.f, c);
    }

    for (const auto& j : fx.popups) {
        float alpha = j.life / 0.55f; // 根据生命值算透明度
        Color c = j.col;
        c.a = (unsigned char)(alpha * 255.f);
        int tw = MeasureText(j.text.c_str(), j.fontSize);
        DrawText(j.text.c_str(), (int)(j.x - tw / 2.f), (int)j.y, j.fontSize, c);
    }
}

static bool CheckNoteInternal(Chart& chart, GameState& gameState, int lane, VisualFX& fx, float noteSpeed, Music& music, bool musicLoaded, int audioOffset) {
    float musicTimeMs = 0.f;
    if (musicLoaded) {
        musicTimeMs = GetMusicTimePlayed(music) * 1000.f;
    }

    int best = -1;
    float bestTimeDiff = 1e9f;
    for (int i = 0; i < (int)chart.notes.size(); i++) {
        Note& n = chart.notes[i];
        if (n.lane != lane || !n.active || n.judged) continue;
        float noteTimeMs = n.time - chart.offset + (float)audioOffset;
        float timeDiff = fabsf(musicTimeMs - noteTimeMs);
        if (timeDiff < bestTimeDiff) {
            bestTimeDiff = timeDiff;
            best = i;
        }
    }
    if (best < 0) return false;

    Note& note = chart.notes[best];

    if (bestTimeDiff > (float)GOOD_TIME) {
        return false;
    }

    if (bestTimeDiff <= (float)PERFECT_TIME) {
        gameState.score += 1000;
        gameState.combo++;
        gameState.perfectCount++;
        note.judged = true;
        PlayHitSoundFromPool();
        AddHitParticles(fx, lane, Color{ 255, 240, 150, 255 }, 14);
        AddJudgePopup(fx, lane, "PERFECT", Color{ 255, 240, 120, 255 }, 32);
    }
    else if (bestTimeDiff <= (float)GREAT_TIME) {
        gameState.score += 500;
        gameState.combo++;
        gameState.greatCount++;
        note.judged = true;
        PlayHitSoundFromPool();
        AddHitParticles(fx, lane, Color{ 160, 220, 255, 255 }, 10);
        AddJudgePopup(fx, lane, "GREAT", Color{ 150, 210, 255, 255 }, 28);
    }
    else {
        gameState.score += 100;
        gameState.combo++;
        gameState.goodCount++;
        note.judged = true;
        PlayHitSoundFromPool();
        AddHitParticles(fx, lane, Color{ 160, 255, 160, 255 }, 8);
        AddJudgePopup(fx, lane, "GOOD", Color{ 140, 255, 160, 255 }, 24);
    }

    if (gameState.combo > gameState.maxCombo) {
        gameState.maxCombo = gameState.combo;
    }
    return true;
}

void CheckNote(Chart& chart, GameState& gameState, int lane, VisualFX& fx, float noteSpeed, Music& music, bool musicLoaded, int audioOffset) {
    if (lane < 0 || lane >= LANE_COUNT) return;
    CheckNoteInternal(chart, gameState, lane, fx, noteSpeed, music, musicLoaded, audioOffset);
    fx.laneFlash[lane] = std::max(fx.laneFlash[lane], 1.f);
}

static void ProcessLaneInput(Chart& chart, GameState& gameState, int lane, VisualFX& fx, float noteSpeed, Music& music, bool musicLoaded, int audioOffset) {
    if (!gameState.isPlaying || gameState.gameOver) return;
    CheckNote(chart, gameState, lane, fx, noteSpeed, music, musicLoaded, audioOffset);
}

void HandleInput(Chart& chart, GameState& gameState, VisualFX& fx, float noteSpeed, Music& music, bool musicLoaded, int audioOffset, const std::map<GameAction, int>& keyBindings) {
    if (IsKeyPressed(GetKeyBinding(keyBindings, GameAction::TapLane1))) ProcessLaneInput(chart, gameState, 0, fx, noteSpeed, music, musicLoaded, audioOffset);
    if (IsKeyPressed(GetKeyBinding(keyBindings, GameAction::TapLane2))) ProcessLaneInput(chart, gameState, 1, fx, noteSpeed, music, musicLoaded, audioOffset);
    if (IsKeyPressed(GetKeyBinding(keyBindings, GameAction::TapLane3))) ProcessLaneInput(chart, gameState, 2, fx, noteSpeed, music, musicLoaded, audioOffset);
    if (IsKeyPressed(GetKeyBinding(keyBindings, GameAction::TapLane4))) ProcessLaneInput(chart, gameState, 3, fx, noteSpeed, music, musicLoaded, audioOffset);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        int mx = GetMouseX();
        int lane = mx / LANE_WIDTH;
        if (lane >= 0 && lane < LANE_COUNT) {
            ProcessLaneInput(chart, gameState, lane, fx, noteSpeed, music, musicLoaded, audioOffset);
        }
    }
}

void UpdateGame(Chart& chart, GameState& gameState, VisualFX& fx, float deltaTime, float noteSpeed, Music& music, bool musicLoaded, int audioOffset) {
    UpdateVisualFX(fx, deltaTime);

    if (!gameState.isPlaying) return;
    
    static int lastCombo = 0;
    if (gameState.combo > lastCombo) {
        g_comboScale = 1.3f;
        lastCombo = gameState.combo;
    }
    g_comboScale += (1.0f - g_comboScale) * 10.0f * deltaTime;

    float musicTimeMs = 0.f;
    if (musicLoaded) {
        musicTimeMs = GetMusicTimePlayed(music) * 1000.f;
    }

    for (auto& note : chart.notes) {
        if (note.active && !note.judged) {
            float noteTimeMs = note.time - chart.offset + (float)audioOffset;
            float timeDiff = musicTimeMs - noteTimeMs;

            if (timeDiff > (float)GOOD_TIME) {
                note.judged = true;
                gameState.combo = 0;
                gameState.missCount++;
                int laneIdx = SanitizeLane(note.lane);
                AddJudgePopup(fx, laneIdx, "MISS", Color{ 255, 100, 120, 255 }, 26);
                fx.laneFlash[laneIdx] = std::max(fx.laneFlash[laneIdx], 0.65f);
            }
        }
    }

    bool allJudged = true;
    for (const auto& note : chart.notes) {
        if (note.active && !note.judged) {
            allJudged = false;
            break;
        }
    }

    if (allJudged && !chart.notes.empty()) {
        gameState.isPlaying = false;
        gameState.gameOver = true;
    }
}

static void ResetVisualFX(VisualFX& fx) {
    for (int i = 0; i < LANE_COUNT; i++) fx.laneFlash[i] = 0.f;
    fx.particles.clear();
    fx.popups.clear();
    g_comboScale = 1.0f;
}

static void UnloadGameMusic(Music& music, bool& musicLoaded) {
    if (musicLoaded) {
        StopMusicStream(music);
        UnloadMusicStream(music);
        music = { 0 };
        musicLoaded = false;
    }
}

static void LoadGameMusicFromChart(const Chart& chart, Music& music, bool& musicLoaded) {
    UnloadGameMusic(music, musicLoaded);
    if (!chart.musicPath.empty()) {
        music = LoadMusicAsset(chart.musicPath.c_str());
        musicLoaded = (music.frameCount > 0);
    }
}

static void DrawMenuBackdrop() {
    DrawRectangleGradientV(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
        Color{ 22, 18, 48, 255 },
        Color{ 6, 4, 18, 255 });
}

static void DrawLoadingScreen(float loadingTimer, float totalDuration, const std::string& songName) {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, BLACK);

    float progress = loadingTimer / totalDuration;
    if (progress > 1.0f) progress = 1.0f;

    DrawText("Loading...", SCREEN_WIDTH / 2 - MeasureText("Loading...", 40) / 2, SCREEN_HEIGHT / 2 - 80, 40, RAYWHITE);

    int barWidth = 400;
    int barHeight = 20;
    int barX = SCREEN_WIDTH / 2 - barWidth / 2;
    int barY = SCREEN_HEIGHT / 2;

    DrawRectangle(barX, barY, barWidth, barHeight, Color{ 40, 40, 60, 255 });
    DrawRectangle(barX, barY, (int)(barWidth * progress), barHeight, Color{ 100, 200, 255, 255 });

    int spinnerRadius = 30;
    int spinnerX = SCREEN_WIDTH / 2;
    int spinnerY = SCREEN_HEIGHT / 2 + 80;
    float angle = loadingTimer * 200.0f;
    DrawCircleLines(spinnerX, spinnerY, spinnerRadius, Color{ 100, 200, 255, 255 });
    DrawLine(spinnerX, spinnerY, spinnerX + (int)(sin(angle * 3.14159f / 180.0f) * spinnerRadius), spinnerY - (int)(cos(angle * 3.14159f / 180.0f) * spinnerRadius), Color{ 100, 200, 255, 255 });

    if (!songName.empty()) {
        std::string nowLoading = "Now Loading: ";
        nowLoading += songName;
        DrawText(nowLoading.c_str(), SCREEN_WIDTH / 2 - MeasureText(nowLoading.c_str(), 20) / 2, SCREEN_HEIGHT / 2 + 140, 20, Color{ 180, 180, 200, 255 });
    }

    int secondsLeft = (int)(totalDuration - loadingTimer) + 1;
    if (secondsLeft < 0) secondsLeft = 0;
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Starting in %d...", secondsLeft);
    DrawText(buffer, SCREEN_WIDTH / 2 - MeasureText(buffer, 18) / 2, SCREEN_HEIGHT / 2 + 200, 18, Color{ 120, 120, 140, 255 });
}

static bool MenuButtonHit(const Rectangle& r) {
    Vector2 m = GetMousePosition();
    return CheckCollisionPointRec(m, r) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

static float ClampF(float v, float lo, float hi) {
    return std::max(lo, std::min(hi, v));
}

static const int kMainMenuY0 = 280;
static const int kMainMenuGap = 76;
static const int kMainMenuFont = 32;

static Rectangle MainMenuRowHitRect(int rowIndex) {
    int y = kMainMenuY0 + rowIndex * kMainMenuGap;
    return { (float)SCREEN_WIDTH / 2 - 240, (float)y - 12, 480, (float)kMainMenuFont + 24 };
}

static void DrawMainMenu(int selectedIndex) {
    DrawMenuBackdrop();
    const char* title = "Pixel Beats";
    int titleSize = 80;
    int tw = MeasureText(title, titleSize);
    DrawText(title, SCREEN_WIDTH / 2 - tw / 2, 120, titleSize, RAYWHITE);
    DrawText("A simple rhythm game!", SCREEN_WIDTH / 2 - 110, 200, 22, Color{ 180, 175, 210, 255 });

    const char* labels[] = { "Choose song", "Options", "Quit" };
    int n = 3;
    int fontSize = kMainMenuFont;

    for (int i = 0; i < n; i++) {
        int y = kMainMenuY0 + i * kMainMenuGap;
        int lw = MeasureText(labels[i], fontSize);
        int x = SCREEN_WIDTH / 2 - lw / 2;
        Rectangle hit{ (float)x - 24, (float)y - 10, (float)lw + 48, (float)fontSize + 20 };
        bool hot = CheckCollisionPointRec(GetMousePosition(), hit);
        bool sel = (i == selectedIndex);
        bool pressed = IsMouseButtonDown(MOUSE_BUTTON_LEFT) && hot;
        
        Rectangle drawRect = hit;
        if (hot) {
            drawRect.x -= 2;
            drawRect.y -= 2;
            drawRect.width += 4;
            drawRect.height += 4;
        }
        
        if (sel || hot) {
            DrawRectangleRounded(drawRect, 0.3f, 8, Color{ 255, 255, 255, (unsigned char)(pressed ? 50 : (sel ? 35 : 18)) });
            DrawRectangleRoundedLinesEx(drawRect, 0.3f, 8, 2.f, Color{ 200, 190, 255, (unsigned char)(sel ? 220 : 120) });
        }
        Color col = pressed ? LIGHTGRAY : (sel ? GOLD : (hot ? SKYBLUE : RAYWHITE));
        int textX = hot ? x - 2 : x;
        int textY = hot ? y - 2 : y;
        DrawText(labels[i], textX, textY, fontSize, col);
    }

    DrawText("UP / DOWN  Select     ENTER/MOUSE CLICK  Confirm   ", SCREEN_WIDTH / 2 - 260, SCREEN_HEIGHT - 80, 18, Color{ 140, 135, 165, 255 });
    DrawText("Q  Quit from main menu", SCREEN_WIDTH / 2 - 130, SCREEN_HEIGHT - 50, 18, Color{ 140, 135, 165, 255 });
}

static void DrawSongSelect(int selectedSongIndex, int selectedDifficultyIndex, float scrollOffset, SortMode sortMode, const std::map<std::pair<int, int>, BestRecord>& bestRecords) {
    DrawMenuBackdrop();
    DrawText("CHOOSE A SONG", SCREEN_WIDTH / 2 - 160, 80, 40, RAYWHITE);
    DrawText("ENTER/MOUSE CLICK  Start      ESC  Back       LEFT / RIGHT  Difficulty", SCREEN_WIDTH / 2 - 280, 122, 18, Color{ 160, 155, 190, 255 });

    DrawText("Difficulty", SCREEN_WIDTH / 2 - MeasureText("Difficulty", 20) / 2, 142, 20, Color{ 200, 195, 230, 255 });
    for (int d = 0; d < 3; d++) {
        Rectangle r = SongSelectDifficultyHitRect(d);
        bool hot = CheckCollisionPointRec(GetMousePosition(), r);
        bool sel = (d == selectedDifficultyIndex);
        bool pressed = IsMouseButtonDown(MOUSE_BUTTON_LEFT) && hot;
        
        Rectangle drawRect = r;
        if (hot) {
            drawRect.x -= 2;
            drawRect.y -= 2;
            drawRect.width += 4;
            drawRect.height += 4;
        }
        
        if (sel || hot) {
            DrawRectangleRounded(drawRect, 0.28f, 8, Color{ 50, 70, 120, (unsigned char)(pressed ? 240 : (sel ? 220 : 140)) });
            DrawRectangleRoundedLinesEx(drawRect, 0.28f, 8, 2.f, Color{ 220, 200, 255, (unsigned char)(sel ? 220 : 130) });
        }
        else {
            DrawRectangleRounded(r, 0.28f, 8, Color{ 28, 32, 55, 200 });
            DrawRectangleRoundedLinesEx(r, 0.28f, 8, 2.f, Color{ 120, 120, 160, 100 });
        }
        const char* lab = kDifficultyLabels[d];
        int lw = MeasureText(lab, 22);
        Color col = pressed ? LIGHTGRAY : (sel ? GOLD : (hot ? SKYBLUE : RAYWHITE));
        int textX = (int)(drawRect.x + drawRect.width / 2 - lw / 2.f);
        int textY = (int)(drawRect.y + 10);
        DrawText(lab, textX, textY, 22, col);
    }

    int count = (int)(sizeof(kSongCatalog) / sizeof(kSongCatalog[0]));
    int viewportHeight = SONG_SELECT_VISIBLE_ROWS * SONG_SELECT_ROW_HEIGHT;
    int maxScroll = (int)GetMaxScrollOffset(count);
    
    int firstVisibleIndex = (int)(scrollOffset / SONG_SELECT_ROW_HEIGHT);
    int lastVisibleIndex = std::min(firstVisibleIndex + SONG_SELECT_VISIBLE_ROWS + 1, count);

    int maxNameWidth = 0;
    for (int i = 0; i < count; i++) {
        int w = MeasureText(kSongCatalog[i].displayName, 26);
        maxNameWidth = std::max(maxNameWidth, w);
    }
    int blockW = maxNameWidth + 120;
    int x = SCREEN_WIDTH / 2 - blockW / 2;

    DrawRectangle(x - 10, SONG_SELECT_Y0 - 6, blockW + 20, viewportHeight + 12, Color{ 20, 22, 40, 200 });
    DrawRectangleLines(x - 10, SONG_SELECT_Y0 - 6, blockW + 20, viewportHeight + 12, Color{ 80, 80, 120, 120 });

    for (int visualIdx = firstVisibleIndex; visualIdx < lastVisibleIndex; visualIdx++) {
        int actualIdx = GetSortedSongIndex(visualIdx, sortMode, selectedDifficultyIndex, bestRecords);
        int y = SONG_SELECT_Y0 + (visualIdx * SONG_SELECT_ROW_HEIGHT) - (int)scrollOffset;
        
        if (y < SONG_SELECT_Y0 - SONG_SELECT_ROW_HEIGHT || y > SONG_SELECT_Y0 + viewportHeight) {
            continue;
        }

        const char* name = kSongCatalog[actualIdx].displayName;
        int nameW = MeasureText(name, 26);
        Rectangle hit{ (float)x, (float)(y - 6), (float)blockW, (float)(SONG_SELECT_ROW_HEIGHT - 8) };
        bool hot = CheckCollisionPointRec(GetMousePosition(), hit);
        bool sel = (actualIdx == selectedSongIndex);
        bool pressed = IsMouseButtonDown(MOUSE_BUTTON_LEFT) && hot;
        
        Rectangle drawRect = hit;
        if (hot) {
            drawRect.x -= 2;
            drawRect.y -= 2;
            drawRect.width += 4;
            drawRect.height += 4;
        }
        
        if (sel || hot) {
            DrawRectangleRounded(drawRect, 0.25f, 8, Color{ 40, 50, 90, (unsigned char)(pressed ? 220 : (sel ? 200 : 120)) });
            DrawRectangleRoundedLinesEx(drawRect, 0.25f, 8, 2.f, Color{ 220, 200, 255, 200 });
        }
        Color col = pressed ? LIGHTGRAY : (sel ? GOLD : RAYWHITE);
        int textY = hot ? y - 2 : y;
        DrawText(name, SCREEN_WIDTH / 2 - nameW / 2, textY, 26, col);

        auto key = std::make_pair(actualIdx, selectedDifficultyIndex);
        auto it = bestRecords.find(key);
        if (it != bestRecords.end()) {
            const BestRecord& rec = it->second;
            std::string recordText = TextFormat("BEST: %d  %c", rec.score, rec.grade);
            int tw = MeasureText(recordText.c_str(), 18);
            DrawText(recordText.c_str(), SCREEN_WIDTH / 2 - maxNameWidth / 2 - tw - 80, textY + 8, 18, Color{ 180, 220, 255, 255 });
        }
    }

    if (maxScroll > 0) {
        int scrollbarX = x + blockW + SONG_SELECT_SCROLLBAR_MARGIN;
        int scrollbarY = SONG_SELECT_Y0;
        int scrollbarH = viewportHeight;
        
        DrawRectangle(scrollbarX, scrollbarY, SONG_SELECT_SCROLLBAR_WIDTH, scrollbarH, Color{ 30, 35, 55, 200 });
        
        float scrollPercent = scrollOffset / (float)maxScroll;
        int thumbH = std::max(40, (int)(scrollbarH * (viewportHeight / (float)(count * SONG_SELECT_ROW_HEIGHT))));
        int thumbY = scrollbarY + (int)(scrollPercent * (scrollbarH - thumbH));
        
        DrawRectangleRounded(
            Rectangle{ (float)scrollbarX, (float)thumbY, (float)SONG_SELECT_SCROLLBAR_WIDTH, (float)thumbH },
            0.4f, 4, Color{ 100, 150, 220, 220 });
    }

    Rectangle sortBtn = { (float)(SCREEN_WIDTH - 160), 80.f, 140.f, 36.f };
    bool sortHot = CheckCollisionPointRec(GetMousePosition(), sortBtn);
    bool sortPressed = IsMouseButtonDown(MOUSE_BUTTON_LEFT) && sortHot;
    
    Rectangle sortDrawRect = sortBtn;
    if (sortHot) {
        sortDrawRect.x -= 2;
        sortDrawRect.y -= 2;
        sortDrawRect.width += 4;
        sortDrawRect.height += 4;
    }
    
    DrawRectangleRounded(sortDrawRect, 0.25f, 6, sortPressed ? Color{ 50, 70, 120, 220 } : (sortHot ? Color{ 50, 70, 120, 180 } : Color{ 30, 35, 55, 200 }));
    DrawRectangleRoundedLinesEx(sortDrawRect, 0.25f, 6, 2.f, sortHot ? Color{ 220, 200, 255, 200 } : Color{ 120, 120, 160, 120 });
    const char* sortLabel = GetSortModeLabel(sortMode);
    int sortW = MeasureText(sortLabel, 18);
    Color sortCol = sortPressed ? LIGHTGRAY : (sortHot ? SKYBLUE : RAYWHITE);
    int sortTextX = (int)(sortDrawRect.x + sortDrawRect.width / 2 - sortW / 2);
    int sortTextY = (int)(sortDrawRect.y + 8);
    DrawText(sortLabel, sortTextX, sortTextY, 18, sortCol);
    
    DrawText("SORT", SCREEN_WIDTH - 156, 120, 14, Color{ 140, 140, 180, 200 });
}

static Rectangle PauseMenuRowHitRect(int rowIndex) {
    const int y0 = 288;
    const int gap = 56;
    int y = y0 + rowIndex * gap;
    return { (float)SCREEN_WIDTH / 2 - 150, (float)y - 10, 300, 42 };
}

static void DrawPauseOverlay(int selectedIndex) {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Color{ 0, 0, 0, 160 });
    const char* title = "PAUSED";
    int ts = 52;
    int tw = MeasureText(title, ts);
    DrawText(title, SCREEN_WIDTH / 2 - tw / 2, 200, ts, RAYWHITE);

    const char* opts[] = { "Continue", "Restart song", "Back to menu" };
    int fs = 26;
    for (int i = 0; i < 3; i++) {
        Rectangle hit = PauseMenuRowHitRect(i);
        int lw = MeasureText(opts[i], fs);
        int x = SCREEN_WIDTH / 2 - lw / 2;
        int y = (int)hit.y + 10;
        bool hot = CheckCollisionPointRec(GetMousePosition(), hit);
        bool sel = (i == selectedIndex);
        bool pressed = IsMouseButtonDown(MOUSE_BUTTON_LEFT) && hot;
        
        Rectangle drawRect = hit;
        if (hot) {
            drawRect.x -= 2;
            drawRect.y -= 2;
            drawRect.width += 4;
            drawRect.height += 4;
        }
        
        if (sel || hot) {
            DrawRectangleRounded(drawRect, 0.3f, 8, Color{ 255, 255, 255, (unsigned char)(pressed ? 40 : 22) });
            DrawRectangleRoundedLinesEx(drawRect, 0.3f, 8, 2.f, Color{ 200, 210, 255, (unsigned char)(sel ? 200 : 100) });
        }
        Color col = pressed ? LIGHTGRAY : (sel ? SKYBLUE : (hot ? LIGHTGRAY : RAYWHITE));
        int textX = hot ? x - 2 : x;
        int textY = hot ? y - 2 : y;
        DrawText(opts[i], textX, textY, fs, col);
    }
    DrawText("UP / DOWN  Select     ENTER  Confirm     ESC  Continue", SCREEN_WIDTH / 2 - 280, SCREEN_HEIGHT - 100, 18, Color{ 160, 160, 185, 255 });
}

enum class OptionsTab {
    Audio,
    Gameplay,
    Controls,
    Count
};

static const char* OptionsTabNames[] = {
    "Audio",
    "Gameplay",
    "Controls"
};

struct OptionsLayout {
    Rectangle volBar;
    Rectangle hitVolBar;
    Rectangle spdBar;
    Rectangle offsetBar;
    Rectangle noteSizeBar;
    Rectangle backBtn;
    Rectangle tabButtons[3];
};

static OptionsLayout GetOptionsLayout() {
    OptionsLayout L;
    L.volBar = { 220, 258, 640, 28 };
    L.hitVolBar = { 220, 358, 640, 28 };
    L.spdBar = { 220, 358, 640, 28 };
    L.offsetBar = { 220, 458, 640, 28 };
    L.noteSizeBar = { 220, 458, 640, 28 };
    L.backBtn = { (float)SCREEN_WIDTH / 2 - 120, 648, 240, 44 };
    
    const int tabW = 160;
    const int tabH = 44;
    const int gap = 12;
    int total = 3 * tabW + 2 * gap;
    int x0 = SCREEN_WIDTH / 2 - total / 2;
    for (int i = 0; i < 3; i++) {
        L.tabButtons[i] = { (float)(x0 + i * (tabW + gap)), 170.f, (float)tabW, (float)tabH };
    }
    return L;
}

static void DrawOptionsScreen(const UserSettings& s, int selectedRow, OptionsTab tab, bool waitingForKey, GameAction waitingAction) {
    DrawMenuBackdrop();
    DrawText("OPTIONS", SCREEN_WIDTH / 2 - 110, 72, 40, RAYWHITE);
    
    OptionsLayout L = GetOptionsLayout();

    for (int i = 0; i < 3; i++) {
        bool sel = ((OptionsTab)i == tab);
        bool hot = CheckCollisionPointRec(GetMousePosition(), L.tabButtons[i]);
        bool pressed = IsMouseButtonDown(MOUSE_BUTTON_LEFT) && hot;
        
        Rectangle drawRect = L.tabButtons[i];
        if (hot) {
            drawRect.x -= 2;
            drawRect.y -= 2;
            drawRect.width += 4;
            drawRect.height += 4;
        }
        
        if (sel || hot) {
            DrawRectangleRounded(drawRect, 0.3f, 8, Color{ 255, 255, 255, (unsigned char)(pressed ? 50 : (sel ? 35 : 18)) });
            DrawRectangleRoundedLinesEx(drawRect, 0.3f, 8, 2.f, Color{ 200, 190, 255, (unsigned char)(sel ? 220 : 120) });
        }
        Color col = pressed ? LIGHTGRAY : (sel ? GOLD : (hot ? SKYBLUE : RAYWHITE));
        int tw = MeasureText(OptionsTabNames[i], 24);
        int textX = (int)(drawRect.x + drawRect.width / 2 - tw / 2.f);
        int textY = (int)(drawRect.y + 10);
        DrawText(OptionsTabNames[i], textX, textY, 24, col);
    }

    if (tab == OptionsTab::Audio) {
        DrawText("Adjust audio settings.", 220, 228, 18, Color{ 160, 155, 190, 255 });
        DrawText("ESC  Back     UP / DOWN  Row      LEFT / RIGHT  Adjust", 220, 246, 18, Color{ 140, 135, 165, 255 });

        DrawText("Master volume", 220, 288, 24, RAYWHITE);
        DrawRectangleRounded(L.volBar, 0.2f, 6, Color{ 30, 28, 50, 255 });
        float vf = L.volBar.width * s.masterVolume;
        DrawRectangleRounded(
            Rectangle{ L.volBar.x, L.volBar.y, vf, L.volBar.height },
            0.2f, 6, Color{ 220, 180, 80, 255 });
        DrawRectangleRoundedLinesEx(L.volBar, 0.2f, 6, 2.f, Color{ 200, 200, 230, (unsigned char)(selectedRow == 0 ? 200 : 80) });
        DrawText(TextFormat("%d%%", (int)(s.masterVolume * 100.f + 0.5f)), (int)(L.volBar.x + L.volBar.width + 16), (int)L.volBar.y + 2, 22, LIGHTGRAY);

        DrawText("Hit sound volume", 220, 388, 24, RAYWHITE);
        DrawRectangleRounded(L.hitVolBar, 0.2f, 6, Color{ 30, 28, 50, 255 });
        float hvf = L.hitVolBar.width * s.hitSoundVolume;
        DrawRectangleRounded(
            Rectangle{ L.hitVolBar.x, L.hitVolBar.y, hvf, L.hitVolBar.height },
            0.2f, 6, Color{ 255, 120, 120, 255 });
        DrawRectangleRoundedLinesEx(L.hitVolBar, 0.2f, 6, 2.f, Color{ 200, 200, 230, (unsigned char)(selectedRow == 1 ? 200 : 80) });
        DrawText(TextFormat("%d%%", (int)(s.hitSoundVolume * 100.f + 0.5f)), (int)(L.hitVolBar.x + L.hitVolBar.width + 16), (int)L.hitVolBar.y + 2, 22, LIGHTGRAY);

        DrawText("Audio offset (ms)", 220, 488, 24, RAYWHITE);
        DrawRectangleRounded(L.offsetBar, 0.2f, 6, Color{ 30, 28, 50, 255 });
        float ot = (float)(s.audioOffset - AUDIO_OFFSET_MIN) / (float)(AUDIO_OFFSET_MAX - AUDIO_OFFSET_MIN);
        ot = ClampF(ot, 0.f, 1.f);
        float ow = L.offsetBar.width * ot;
        DrawRectangleRounded(
            Rectangle{ L.offsetBar.x, L.offsetBar.y, ow, L.offsetBar.height },
            0.2f, 6, Color{ 180, 120, 220, 255 });
        DrawRectangleRoundedLinesEx(L.offsetBar, 0.2f, 6, 2.f, Color{ 200, 200, 230, (unsigned char)(selectedRow == 2 ? 200 : 80) });
        DrawText(TextFormat("%+d ms  (%d to %d)", s.audioOffset, AUDIO_OFFSET_MIN, AUDIO_OFFSET_MAX),
            (int)(L.offsetBar.x + L.offsetBar.width + 16), (int)L.offsetBar.y + 2, 22, LIGHTGRAY);
        DrawText("Negative = notes earlier, Positive = notes later", 220, 560, 16, Color{ 140, 135, 165, 255 });
    }
    else if (tab == OptionsTab::Gameplay) {
        DrawText("Adjust gameplay settings.", 220, 228, 18, Color{ 160, 155, 190, 255 });
        DrawText("ESC  Back     UP / DOWN  Row      LEFT / RIGHT  Adjust", 220, 246, 18, Color{ 140, 135, 165, 255 });

        DrawText("Note speed (pixels per frame)", 220, 288, 24, RAYWHITE);
        DrawRectangleRounded(L.spdBar, 0.2f, 6, Color{ 30, 28, 50, 255 });
        float t = (s.noteSpeed - NOTE_SPEED_MIN) / (NOTE_SPEED_MAX - NOTE_SPEED_MIN);
        t = ClampF(t, 0.f, 1.f);
        float sw = L.spdBar.width * t;
        DrawRectangleRounded(
            Rectangle{ L.spdBar.x, L.spdBar.y, sw, L.spdBar.height },
            0.2f, 6, Color{ 100, 200, 255, 255 });
        DrawRectangleRoundedLinesEx(L.spdBar, 0.2f, 6, 2.f, Color{ 200, 200, 230, (unsigned char)(selectedRow == 0 ? 200 : 80) });
        DrawText(TextFormat("%.1f  (%.0f - %.0f)", s.noteSpeed, NOTE_SPEED_MIN, NOTE_SPEED_MAX),
            (int)(L.spdBar.x + L.spdBar.width + 16), (int)L.spdBar.y + 2, 22, LIGHTGRAY);

        DrawText("Note size (pixels)", 220, 388, 24, RAYWHITE);
        DrawRectangleRounded(L.noteSizeBar, 0.2f, 6, Color{ 30, 28, 50, 255 });
        float ns = (float)(s.noteSize - NOTE_SIZE_MIN) / (float)(NOTE_SIZE_MAX - NOTE_SIZE_MIN);
        ns = ClampF(ns, 0.f, 1.f);
        float nsw = L.noteSizeBar.width * ns;
        DrawRectangleRounded(
            Rectangle{ L.noteSizeBar.x, L.noteSizeBar.y, nsw, L.noteSizeBar.height },
            0.2f, 6, Color{ 255, 150, 100, 255 });
        DrawRectangleRoundedLinesEx(L.noteSizeBar, 0.2f, 6, 2.f, Color{ 200, 200, 230, (unsigned char)(selectedRow == 1 ? 200 : 80) });
        DrawText(TextFormat("%d  (%d - %d)", s.noteSize, NOTE_SIZE_MIN, NOTE_SIZE_MAX),
            (int)(L.noteSizeBar.x + L.noteSizeBar.width + 16), (int)L.noteSizeBar.y + 2, 22, LIGHTGRAY);
    }
    else if (tab == OptionsTab::Controls) {
        DrawText("Customize key bindings.", 220, 228, 18, Color{ 160, 155, 190, 255 });
        DrawText("ESC  Back     UP / DOWN  Select     ENTER  Assign key", 220, 246, 18, Color{ 140, 135, 165, 255 });

        const int y0 = 296;
        const int rowH = 52;
        for (int i = 0; i < (int)GameAction::Count; i++) {
            int y = y0 + i * rowH;
            DrawText(GameActionDisplayNames[i], 260, y, 24, RAYWHITE);
            
            Rectangle keyBox = { 720.f, (float)y - 6, 120.f, 36.f };
            bool sel = (selectedRow == i);
            bool waiting = waitingForKey && waitingAction == (GameAction)i;
            
            if (waiting) {
                DrawRectangleRounded(keyBox, 0.25f, 6, Color{ 255, 200, 100, 60 });
                DrawRectangleRoundedLinesEx(keyBox, 0.25f, 6, 2.f, Color{ 255, 200, 100, 255 });
                float blink = sinf(GetTime() * 8.f) * 0.5f + 0.5f;
                Color textCol = { 255, 200, 100, (unsigned char)(180 + blink * 75) };
                const char* pressText = "Press any key...";
                int tw = MeasureText(pressText, 18);
                DrawText(pressText, (int)(keyBox.x + keyBox.width / 2 - tw / 2), y + 6, 18, textCol);
            }
            else if (sel) {
                DrawRectangleRounded(keyBox, 0.25f, 6, Color{ 255, 255, 255, 35 });
                DrawRectangleRoundedLinesEx(keyBox, 0.25f, 6, 2.f, Color{ 200, 200, 255, 220 });
                
                const char* keyName = GetKeyName(GetKeyBinding(s.keyBindings, (GameAction)i));
                int tw = MeasureText(keyName, 22);
                DrawText(keyName, (int)(keyBox.x + keyBox.width / 2 - tw / 2), y + 4, 22, GOLD);
            }
            else {
                DrawRectangleRounded(keyBox, 0.25f, 6, Color{ 30, 28, 50, 200 });
                DrawRectangleRoundedLinesEx(keyBox, 0.25f, 6, 2.f, Color{ 120, 120, 160, 100 });
                
                const char* keyName = GetKeyName(GetKeyBinding(s.keyBindings, (GameAction)i));
                int tw = MeasureText(keyName, 22);
                DrawText(keyName, (int)(keyBox.x + keyBox.width / 2 - tw / 2), y + 4, 22, RAYWHITE);
            }
        }
    }

    bool backHot = CheckCollisionPointRec(GetMousePosition(), L.backBtn);
    bool backSel = (selectedRow == (tab == OptionsTab::Audio ? 3 : (tab == OptionsTab::Gameplay ? 2 : (int)GameAction::Count)));
    bool backPressed = IsMouseButtonDown(MOUSE_BUTTON_LEFT) && backHot;
    
    Rectangle backDrawRect = L.backBtn;
    if (backHot) {
        backDrawRect.x -= 2;
        backDrawRect.y -= 2;
        backDrawRect.width += 4;
        backDrawRect.height += 4;
    }
    
    if (backSel || backHot) {
        DrawRectangleRounded(backDrawRect, 0.3f, 8, Color{ 255, 255, 255, (unsigned char)(backPressed ? 40 : (backSel ? 28 : 16)) });
        DrawRectangleRoundedLinesEx(backDrawRect, 0.3f, 8, 2.f, Color{ 200, 190, 255, (unsigned char)(backSel ? 200 : 100) });
    }
    const char* backLabel = "Back";
    int bw = MeasureText(backLabel, 26);
    Color backCol = backPressed ? LIGHTGRAY : (backSel ? SKYBLUE : (backHot ? LIGHTGRAY : RAYWHITE));
    int backTextX = (int)(backDrawRect.x + backDrawRect.width / 2 - bw / 2.f);
    int backTextY = (int)(backDrawRect.y + 10);
    DrawText(backLabel, backTextX, backTextY, 26, backCol);
}

static int GetOptionsRowCount(OptionsTab tab) {
    if (tab == OptionsTab::Audio) return 4;
    if (tab == OptionsTab::Gameplay) return 3;
    if (tab == OptionsTab::Controls) return (int)GameAction::Count + 1;
    return 5;
}

static void ProcessOptionsInput(UserSettings& settings, std::map<std::pair<int, int>, BestRecord>& bestRecords, int& selectedRow, OptionsTab& tab, AppScreen& screen, bool& waitingForKey, GameAction& waitingAction) {
    OptionsLayout L = GetOptionsLayout();
    int rowCount = GetOptionsRowCount(tab);

    if (waitingForKey) {
        if (IsKeyPressed(KEY_ESCAPE)) {
            waitingForKey = false;
            return;
        }
        
        int key = GetKeyPressed();
        if (key > 0) {
            for (int i = 0; i < (int)GameAction::Count; i++) {
                if (settings.keyBindings[(GameAction)i] == key && (GameAction)i != waitingAction) {
                    settings.keyBindings[(GameAction)i] = DefaultKeyBindings[i];
                }
            }
            
            settings.keyBindings[waitingAction] = key;
            ValidateKeyBindings(settings.keyBindings);
            waitingForKey = false;
            return;
        }
        return;
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        g_fadeAlpha = 1.0f;
        screen = AppScreen::MainMenu;
        SetMasterVolume(settings.masterVolume);
        float effectiveHitVol = settings.hitSoundVolume;
        if (hitSound.frameCount > 0) SetSoundVolume(hitSound, effectiveHitVol);
        for (int i = 0; i < HIT_SOUND_POOL_SIZE; i++) {
            if (hitSoundPool[i].frameCount > 0) SetSoundVolume(hitSoundPool[i], effectiveHitVol);
        }
        SaveSettingsToFile(settings, bestRecords);
        return;
    }

    if (IsKeyPressed(KEY_UP)) {
        selectedRow = (selectedRow - 1 + rowCount) % rowCount;
    }
    if (IsKeyPressed(KEY_DOWN)) {
        selectedRow = (selectedRow + 1) % rowCount;
    }

    if (tab == OptionsTab::Audio) {
        if (IsKeyPressed(KEY_LEFT)) {
            if (selectedRow == 0) {
                settings.masterVolume = ClampF(settings.masterVolume - 0.05f, 0.f, 1.f);
            }
            else if (selectedRow == 1) {
                settings.hitSoundVolume = ClampF(settings.hitSoundVolume - 0.05f, 0.f, 1.f);
            }
            else if (selectedRow == 2) {
                settings.audioOffset = std::max(AUDIO_OFFSET_MIN, settings.audioOffset - 5);
            }
        }
        if (IsKeyPressed(KEY_RIGHT)) {
            if (selectedRow == 0) {
                settings.masterVolume = ClampF(settings.masterVolume + 0.05f, 0.f, 1.f);
            }
            else if (selectedRow == 1) {
                settings.hitSoundVolume = ClampF(settings.hitSoundVolume + 0.05f, 0.f, 1.f);
            }
            else if (selectedRow == 2) {
                settings.audioOffset = std::min(AUDIO_OFFSET_MAX, settings.audioOffset + 5);
            }
        }
    }
    else if (tab == OptionsTab::Gameplay) {
        if (IsKeyPressed(KEY_LEFT)) {
            if (selectedRow == 0) {
                settings.noteSpeed = ClampF(settings.noteSpeed - 0.5f, NOTE_SPEED_MIN, NOTE_SPEED_MAX);
            }
            else if (selectedRow == 1) {
                settings.noteSize = std::max(NOTE_SIZE_MIN, settings.noteSize - 4);
            }
        }
        if (IsKeyPressed(KEY_RIGHT)) {
            if (selectedRow == 0) {
                settings.noteSpeed = ClampF(settings.noteSpeed + 0.5f, NOTE_SPEED_MIN, NOTE_SPEED_MAX);
            }
            else if (selectedRow == 1) {
                settings.noteSize = std::min(NOTE_SIZE_MAX, settings.noteSize + 4);
            }
        }
    }

    if ((IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) && selectedRow == rowCount - 1) {
        g_fadeAlpha = 1.0f;
        screen = AppScreen::MainMenu;
        SetMasterVolume(settings.masterVolume);
        SaveSettingsToFile(settings, bestRecords);
        return;
    }

    if (tab == OptionsTab::Controls && (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) && selectedRow < (int)GameAction::Count) {
        waitingForKey = true;
        waitingAction = (GameAction)selectedRow;
        return;
    }

    Vector2 m = GetMousePosition();
    
    for (int i = 0; i < 3; i++) {
        if (CheckCollisionPointRec(m, L.tabButtons[i]) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            tab = (OptionsTab)i;
            selectedRow = 0;
            return;
        }
    }

    if (CheckCollisionPointRec(m, L.backBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        g_fadeAlpha = 1.0f;
        screen = AppScreen::MainMenu;
        SetMasterVolume(settings.masterVolume);
        SaveSettingsToFile(settings, bestRecords);
        return;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        if (tab == OptionsTab::Audio) {
            if (CheckCollisionPointRec(m, L.volBar)) {
                float u = (m.x - L.volBar.x) / L.volBar.width;
                settings.masterVolume = ClampF(u, 0.f, 1.f);
                selectedRow = 0;
            }
            if (CheckCollisionPointRec(m, L.hitVolBar)) {
                float u = (m.x - L.hitVolBar.x) / L.hitVolBar.width;
                settings.hitSoundVolume = ClampF(u, 0.f, 1.f);
                selectedRow = 1;
            }
            if (CheckCollisionPointRec(m, L.offsetBar)) {
                float u = (m.x - L.offsetBar.x) / L.offsetBar.width;
                settings.audioOffset = (int)(AUDIO_OFFSET_MIN + u * (AUDIO_OFFSET_MAX - AUDIO_OFFSET_MIN));
                selectedRow = 2;
            }
        }
        else if (tab == OptionsTab::Gameplay) {
            if (CheckCollisionPointRec(m, L.spdBar)) {
                float u = (m.x - L.spdBar.x) / L.spdBar.width;
                settings.noteSpeed = ClampF(NOTE_SPEED_MIN + u * (NOTE_SPEED_MAX - NOTE_SPEED_MIN), NOTE_SPEED_MIN, NOTE_SPEED_MAX);
                selectedRow = 0;
            }
            if (CheckCollisionPointRec(m, L.noteSizeBar)) {
                float u = (m.x - L.noteSizeBar.x) / L.noteSizeBar.width;
                settings.noteSize = (int)(NOTE_SIZE_MIN + u * (NOTE_SIZE_MAX - NOTE_SIZE_MIN));
                selectedRow = 1;
            }
        }
        else if (tab == OptionsTab::Controls) {
            const int y0 = 296;
            const int rowH = 52;
            for (int i = 0; i < (int)GameAction::Count; i++) {
                int y = y0 + i * rowH;
                Rectangle keyBox = { 720.f, (float)y - 6, 120.f, 36.f };
                if (CheckCollisionPointRec(m, keyBox) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    selectedRow = i;
                    waitingForKey = true;
                    waitingAction = (GameAction)i;
                    return;
                }
            }
        }
    }

    SetMasterVolume(settings.masterVolume);
    float effectiveHitVol = settings.hitSoundVolume;
    if (hitSound.frameCount > 0) SetSoundVolume(hitSound, effectiveHitVol);
    for (int i = 0; i < HIT_SOUND_POOL_SIZE; i++) {
        if (hitSoundPool[i].frameCount > 0) SetSoundVolume(hitSoundPool[i], effectiveHitVol);
    }
}

static void PrepareSong(const std::string& chartPath, Chart& chart, GameState& gameState, VisualFX& fx, Music& music, bool& musicLoaded, bool& isNewRecord) {
    chart = LoadChart(chartPath);
    gameState = { 0, 0, 0, 0, 0, 0, 0, false, false };
    ResetVisualFX(fx);
    isNewRecord = false;
    LoadGameMusicFromChart(chart, music, musicLoaded);
    if (musicLoaded) {
        SetMusicVolume(music, 1.f);
    }
}

static void StartGameplay(GameState& gameState, Music& music, bool musicLoaded) {
    if (musicLoaded) {
        PlayMusicStream(music);
    }
    gameState.isPlaying = true;
}

static void StartSongFromPath(const std::string& chartPath, Chart& chart, GameState& gameState, VisualFX& fx, Music& music, bool& musicLoaded, bool& isNewRecord) {
    PrepareSong(chartPath, chart, gameState, fx, music, musicLoaded, isNewRecord);
    StartGameplay(gameState, music, musicLoaded);
}

int main() {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Rhythm Game");
    SetExitKey(0);
    InitAudioDevice();
    SetTargetFPS(120);

    UserSettings userSettings;
    std::map<std::pair<int, int>, BestRecord> bestRecords;
    LoadSettingsFromFile(userSettings, bestRecords);
    
    InitDefaultKeyBindings(userSettings.keyBindings);
    ValidateKeyBindings(userSettings.keyBindings);
    
    SetMasterVolume(userSettings.masterVolume);

    hitSound = LoadSoundAsset("hit.wav");
    for (int i = 0; i < HIT_SOUND_POOL_SIZE; i++) {
        hitSoundPool[i] = LoadSoundAsset("hit.wav");
    }
    float effectiveHitVol = userSettings.hitSoundVolume;
    if (hitSound.frameCount > 0) SetSoundVolume(hitSound, effectiveHitVol);
    for (int i = 0; i < HIT_SOUND_POOL_SIZE; i++) {
        if (hitSoundPool[i].frameCount > 0) SetSoundVolume(hitSoundPool[i], effectiveHitVol);
    }

    Chart chart{};
    Music music = { 0 };
    bool musicLoaded = false;
    GameState gameState = { 0, 0, 0, 0, 0, 0, 0, false, false };
    VisualFX fx;
    bool isNewRecord = false;

    AppScreen screen = AppScreen::MainMenu;
    std::string currentChartPath = SongChartPath(kSongCatalog[0], 1);
    int mainMenuIndex = 0;
    int songSelectIndex = 0;
    int songDifficultyIndex = 1;
    int pauseMenuIndex = 0;
    int optionsRow = 0;
    OptionsTab optionsTab = OptionsTab::Audio;
    bool optionsWaitingForKey = false;
    GameAction optionsWaitingAction = GameAction::TapLane1;
    bool musicAwaitingResume = false;
    const int songCatalogCount = (int)(sizeof(kSongCatalog) / sizeof(kSongCatalog[0]));
    float loadingTimer = 0.0f;
    std::string loadingSongName;
    const float LOADING_DURATION = 5.0f;
    
    float songScrollOffset = 0.0f;
    float songTargetScroll = 0.0f;
    SortMode songSortMode = SortMode::AlphabeticalAsc;

    auto resumePausedMusic = [&]() {
        if (musicAwaitingResume && musicLoaded) {
            ResumeMusicStream(music);
            musicAwaitingResume = false;
        }
    };

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        if (musicLoaded) {
            UpdateMusicStream(music);
        }
        
        // 更新页面切换淡入淡出
        if (g_fadeAlpha > 0) {
            g_fadeAlpha -= dt * 2.0f;
            if (g_fadeAlpha < 0) g_fadeAlpha = 0;
        }

        if (screen == AppScreen::Playing) {
            UpdateGame(chart, gameState, fx, dt, userSettings.noteSpeed, music, musicLoaded, userSettings.audioOffset);
            if (gameState.gameOver) {
                screen = AppScreen::GameOver;
                if (musicLoaded && IsMusicStreamPlaying(music)) {
                    StopMusicStream(music);
                }
                float acc = ComputeAccuracyPercent(gameState);
                float comboScr = ComputeComboScore(gameState, (int)chart.notes.size());
                float overall = ComputeOverallRatingValue(acc, comboScr);
                const char* grade = GradeLetterFromOverall(overall);

                BestRecord newRecord;
                newRecord.score = gameState.score;
                newRecord.maxCombo = gameState.maxCombo;
                newRecord.accuracy = acc;
                newRecord.perfectCount = gameState.perfectCount;
                newRecord.greatCount = gameState.greatCount;
                newRecord.goodCount = gameState.goodCount;
                newRecord.missCount = gameState.missCount;
                newRecord.grade = grade[0];

                auto key = std::make_pair(songSelectIndex, songDifficultyIndex);
                auto it = bestRecords.find(key);
                isNewRecord = (it == bestRecords.end() || newRecord.score > it->second.score);
                if (isNewRecord) {
                    bestRecords[key] = newRecord;
                    SaveSettingsToFile(userSettings, bestRecords);
                }
            }
            else {
                HandleInput(chart, gameState, fx, userSettings.noteSpeed, music, musicLoaded, userSettings.audioOffset, userSettings.keyBindings);
                if (IsKeyPressed(GetKeyBinding(userSettings.keyBindings, GameAction::Pause)) || IsKeyPressed(KEY_ESCAPE)) {
                    pauseMenuIndex = 0;
                    screen = AppScreen::Paused;
                    musicAwaitingResume = false;
                    if (musicLoaded && IsMusicStreamPlaying(music)) {
                        PauseMusicStream(music);
                        musicAwaitingResume = true;
                    }
                }
            }
        }
        else if (screen == AppScreen::Paused) {
            if (IsKeyPressed(KEY_UP)) {
                pauseMenuIndex = (pauseMenuIndex - 1 + 3) % 3;
            }
            if (IsKeyPressed(KEY_DOWN)) {
                pauseMenuIndex = (pauseMenuIndex + 1) % 3;
            }
            for (int i = 0; i < 3; i++) {
                if (MenuButtonHit(PauseMenuRowHitRect(i))) {
                    pauseMenuIndex = i;
                    if (i == 0) {
                        screen = AppScreen::Playing;
                        resumePausedMusic();
                    }
                    else if (i == 1) {
                        musicAwaitingResume = false;
                        StartSongFromPath(currentChartPath, chart, gameState, fx, music, musicLoaded, isNewRecord);
                        screen = AppScreen::Playing;
                    }
                    else {
                        musicAwaitingResume = false;
                        UnloadGameMusic(music, musicLoaded);
                        gameState = { 0, 0, 0, 0, 0, 0, 0, false, false };
                        chart = Chart{};
                        screen = AppScreen::MainMenu;
                    }
                }
            }
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
                if (pauseMenuIndex == 0) {
                    screen = AppScreen::Playing;
                    resumePausedMusic();
                }
                else if (pauseMenuIndex == 1) {
                    musicAwaitingResume = false;
                    StartSongFromPath(currentChartPath, chart, gameState, fx, music, musicLoaded, isNewRecord);
                    screen = AppScreen::Playing;
                }
                else {
                    musicAwaitingResume = false;
                    UnloadGameMusic(music, musicLoaded);
                    gameState = { 0, 0, 0, 0, 0, 0, 0, false, false };
                    chart = Chart{};
                    screen = AppScreen::MainMenu;
                }
            }
            if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(GetKeyBinding(userSettings.keyBindings, GameAction::Pause))) {
                screen = AppScreen::Playing;
                resumePausedMusic();
            }
        }
        else if (screen == AppScreen::Loading) {
            loadingTimer += dt;
            if (loadingTimer >= LOADING_DURATION) {
                StartGameplay(gameState, music, musicLoaded);
                screen = AppScreen::Playing;
            }
        }
        else if (screen == AppScreen::MainMenu) {
            if (IsKeyPressed(KEY_UP)) {
                mainMenuIndex = (mainMenuIndex - 1 + 3) % 3;
            }
            if (IsKeyPressed(KEY_DOWN)) {
                mainMenuIndex = (mainMenuIndex + 1) % 3;
            }
            bool requestQuit = false;
            for (int i = 0; i < 3; i++) {
                if (MenuButtonHit(MainMenuRowHitRect(i))) {
                    mainMenuIndex = i;
                    if (i == 0) {
                        g_fadeAlpha = 1.0f;
                        screen = AppScreen::SongSelect;
                        songSelectIndex = std::min(songSelectIndex, songCatalogCount - 1);
                    }
                    else if (i == 1) {
                        g_fadeAlpha = 1.0f;
                        optionsRow = 0;
                        screen = AppScreen::Options;
                    }
                    else {
                        requestQuit = true;
                    }
                }
            }
            if (requestQuit) {
                break;
            }
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
                if (mainMenuIndex == 0) {
                    g_fadeAlpha = 1.0f;
                    screen = AppScreen::SongSelect;
                }
                else if (mainMenuIndex == 1) {
                    g_fadeAlpha = 1.0f;
                    optionsRow = 0;
                    screen = AppScreen::Options;
                }
                else {
                    break;
                }
            }
            if (IsKeyPressed(KEY_Q)) {
                break;
            }
        }
        else if (screen == AppScreen::Options) {
            ProcessOptionsInput(userSettings, bestRecords, optionsRow, optionsTab, screen, optionsWaitingForKey, optionsWaitingAction);
        }
        else if (screen == AppScreen::SongSelect) {
            if (IsKeyPressed(KEY_ESCAPE)) {
                g_fadeAlpha = 1.0f;
                screen = AppScreen::MainMenu;
            }
            if (IsKeyPressed(KEY_LEFT)) {
                songDifficultyIndex = (songDifficultyIndex - 1 + 3) % 3;
            }
            if (IsKeyPressed(KEY_RIGHT)) {
                songDifficultyIndex = (songDifficultyIndex + 1) % 3;
            }
            
            float maxScroll = GetMaxScrollOffset(songCatalogCount);
            
            if (IsKeyPressed(KEY_UP)) {
                int prevIndex = songSelectIndex;
                for (int i = 0; i < songCatalogCount; i++) {
                    int actualIdx = GetSortedSongIndex(i, songSortMode, songDifficultyIndex, bestRecords);
                    if (actualIdx == songSelectIndex) {
                        int newVisualIdx = (i - 1 + songCatalogCount) % songCatalogCount;
                        songSelectIndex = GetSortedSongIndex(newVisualIdx, songSortMode, songDifficultyIndex, bestRecords);
                        
                        float newScrollPos = (float)newVisualIdx * SONG_SELECT_ROW_HEIGHT - (SONG_SELECT_VISIBLE_ROWS / 2) * SONG_SELECT_ROW_HEIGHT;
                        songTargetScroll = std::max(0.0f, std::min(maxScroll, newScrollPos));
                        break;
                    }
                }
            }
            if (IsKeyPressed(KEY_DOWN)) {
                for (int i = 0; i < songCatalogCount; i++) {
                    int actualIdx = GetSortedSongIndex(i, songSortMode, songDifficultyIndex, bestRecords);
                    if (actualIdx == songSelectIndex) {
                        int newVisualIdx = (i + 1) % songCatalogCount;
                        songSelectIndex = GetSortedSongIndex(newVisualIdx, songSortMode, songDifficultyIndex, bestRecords);
                        
                        float newScrollPos = (float)newVisualIdx * SONG_SELECT_ROW_HEIGHT - (SONG_SELECT_VISIBLE_ROWS / 2) * SONG_SELECT_ROW_HEIGHT;
                        songTargetScroll = std::max(0.0f, std::min(maxScroll, newScrollPos));
                        break;
                    }
                }
            }
            
            float wheelDelta = GetMouseWheelMove();
            if (wheelDelta != 0) {
                songTargetScroll -= wheelDelta * SONG_SELECT_ROW_HEIGHT * 0.8f;
                songTargetScroll = std::max(0.0f, std::min(maxScroll, songTargetScroll));
            }
            
            // 惯性滚动缓动
            songScrollOffset += (songTargetScroll - songScrollOffset) * 8.0f * dt;
            
            for (int d = 0; d < 3; d++) {
                if (MenuButtonHit(SongSelectDifficultyHitRect(d))) {
                    songDifficultyIndex = d;
                }
            }
            
            Rectangle sortBtn = { (float)(SCREEN_WIDTH - 160), 80.f, 140.f, 36.f };
            if (MenuButtonHit(sortBtn)) {
                switch (songSortMode) {
                    case SortMode::AlphabeticalAsc:
                        songSortMode = SortMode::AlphabeticalDesc;
                        break;
                    case SortMode::AlphabeticalDesc:
                        songSortMode = SortMode::ScoreDesc;
                        break;
                    case SortMode::ScoreDesc:
                        songSortMode = SortMode::ScoreAsc;
                        break;
                    case SortMode::ScoreAsc:
                        songSortMode = SortMode::AlphabeticalAsc;
                        break;
                }
            }
            
            int maxNameWidth = 0;
            for (int i = 0; i < songCatalogCount; i++) {
                int w = MeasureText(kSongCatalog[i].displayName, 26);
                maxNameWidth = std::max(maxNameWidth, w);
            }
            int blockW = maxNameWidth + 120;
            int x = SCREEN_WIDTH / 2 - blockW / 2;
            
            for (int visualIdx = 0; visualIdx < songCatalogCount; visualIdx++) {
                int actualIdx = GetSortedSongIndex(visualIdx, songSortMode, songDifficultyIndex, bestRecords);
                int y = SONG_SELECT_Y0 + (visualIdx * SONG_SELECT_ROW_HEIGHT) - (int)songScrollOffset;
                
                Rectangle hit{ (float)x, (float)(y - 6), (float)blockW, (float)(SONG_SELECT_ROW_HEIGHT - 8) };
                if (MenuButtonHit(hit)) {
                    songSelectIndex = actualIdx;
                    currentChartPath = SongChartPath(kSongCatalog[actualIdx], songDifficultyIndex);
                    PrepareSong(currentChartPath, chart, gameState, fx, music, musicLoaded, isNewRecord);
                    loadingSongName = kSongCatalog[actualIdx].displayName;
                    loadingTimer = 0.0f;
                    screen = AppScreen::Loading;
                    break;
                }
            }
            
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
                currentChartPath = SongChartPath(kSongCatalog[songSelectIndex], songDifficultyIndex);
                PrepareSong(currentChartPath, chart, gameState, fx, music, musicLoaded, isNewRecord);
                loadingSongName = kSongCatalog[songSelectIndex].displayName;
                loadingTimer = 0.0f;
                screen = AppScreen::Loading;
            }
        }
        else if (screen == AppScreen::GameOver) {
            if (IsKeyPressed(KEY_R)) {
                StartSongFromPath(currentChartPath, chart, gameState, fx, music, musicLoaded, isNewRecord);
                screen = AppScreen::Playing;
            }
            if (IsKeyPressed(KEY_M)) {
                musicAwaitingResume = false;
                UnloadGameMusic(music, musicLoaded);
                gameState = { 0, 0, 0, 0, 0, 0, 0, false, false };
                chart = Chart{};
                screen = AppScreen::MainMenu;
            }
        }

        BeginDrawing();
        ClearBackground(BLACK);

        if (screen == AppScreen::MainMenu) {
            DrawMainMenu(mainMenuIndex);
        }
        else if (screen == AppScreen::Options) {
            DrawOptionsScreen(userSettings, optionsRow, optionsTab, optionsWaitingForKey, optionsWaitingAction);
        }
        else if (screen == AppScreen::SongSelect) {
            DrawSongSelect(songSelectIndex, songDifficultyIndex, songScrollOffset, songSortMode, bestRecords);
        }
        else if (screen == AppScreen::Loading) {
            DrawLoadingScreen(loadingTimer, LOADING_DURATION, loadingSongName);
        }
        else if (screen == AppScreen::Playing || screen == AppScreen::Paused || screen == AppScreen::GameOver) {
            DrawGame(chart, gameState, fx, userSettings.noteSpeed, userSettings.noteSize, music, musicLoaded, userSettings.audioOffset, userSettings.keyBindings);
            if (screen == AppScreen::Paused) {
                DrawPauseOverlay(pauseMenuIndex);
            }
            if (screen == AppScreen::GameOver) {
                DrawGameOverOverlay(gameState, (int)chart.notes.size(), isNewRecord);
            }
        }

        // 页面切换淡入淡出遮罩
        if (g_fadeAlpha > 0) {
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Color{ 0, 0, 0, (unsigned char)(g_fadeAlpha * 255.f) });
        }

        EndDrawing();
    }

    if (musicLoaded) {
        UnloadMusicStream(music);
    }
    UnloadSound(hitSound);
    for (int i = 0; i < HIT_SOUND_POOL_SIZE; i++) {
        UnloadSound(hitSoundPool[i]);
    }
    CloseAudioDevice();
    SaveSettingsToFile(userSettings, bestRecords);
    CloseWindow();

    return 0;
}
