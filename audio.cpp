// ============================================================================
// audio.cpp - 音频功能实现
// ============================================================================
// 本文件实现游戏的音频相关功能，包括：
//   - 音乐列表加载
//   - 成绩记录读写
//   - 击中音效生成和播放
//   - 按键名称转换
// ============================================================================

#include "audio.h"
#include <fstream>
#include <sstream>

#include "assets.h"   // 内嵌资源加载（单文件 exe 支持）

#ifndef PI
#define PI 3.14159265358979323846f
#endif

// ============================================================================
// GetRecordKey - 生成成绩记录唯一标识符
// ============================================================================
// 【功能说明】
//   根据音乐索引和难度生成唯一的记录键值，用于成绩的存取
// 【参数说明】
//   - musicIndex: 音乐在musicList中的索引
//   - difficulty: 难度等级（EASY/NORMAL/HARD）
// 【返回值】
//   格式为 "best_{musicIndex}_{difficulty}" 的字符串
//   例如: "best_0_normal"
// 【使用场景】
//   用于 SaveBestRecord 和 LoadBestRecord 函数中标识成绩记录
// ============================================================================
std::string GetRecordKey(int musicIndex, Difficulty difficulty) {
    const char* diffNames[] = {"easy", "normal", "hard"};
    return "best_" + std::to_string(musicIndex) + "_" + diffNames[(int)difficulty];
}

// ============================================================================
// LoadBestRecord - 加载最佳成绩记录
// ============================================================================
// 【功能说明】
//   从文件读取玩家在特定音乐和难度下的历史最佳成绩
// 【参数说明】
//   - musicIndex: 音乐在musicList中的索引
//   - difficulty: 难度等级
// 【返回值】
//   BestRecord结构体，包含分数、最大连击、准确率、评级
//   如果文件不存在或记录不存在，返回默认值 {0, 0, 0.0f, 'D'}
// 【文件格式】
//   "best_{musicIndex}_{difficulty}:score,combo,accuracy_x10,grade"
//   accuracy存储为整数（实际值×10）以保留一位小数
// 【实现步骤】
//   1. 打开best_records.txt文件
//   2. 生成目标记录键值
//   3. 逐行读取，查找匹配的键值
//   4. 解析数据并填充BestRecord结构体
// ============================================================================
BestRecord LoadBestRecord(int musicIndex, Difficulty difficulty) {
    BestRecord record = {0, 0, 0.0f, 'D'};

    const char* fileName = "best_records.txt";
    std::ifstream file(fileName);
    if (!file.is_open()) {
        return record;
    }

    std::string key = GetRecordKey(musicIndex, difficulty);
    std::string line;
    while (std::getline(file, line)) {
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string recordKey = line.substr(0, colonPos);
            if (recordKey == key) {
                std::string data = line.substr(colonPos + 1);
                std::stringstream ss(data);
                std::string part;
                std::vector<int> values;

                while (std::getline(ss, part, ',')) {
                    values.push_back(std::stoi(part));
                }
                if (values.size() >= 4) {
                    record.score = values[0];
                    record.maxCombo = values[1];
                    record.accuracy = (float)values[2] / 10.0f;  // 还原×10的准确率
                    record.grade = (char)values[3];
                }
                break;
            }
        }
    }
    file.close();
    return record;
}

// ============================================================================
// SaveBestRecord - 保存最佳成绩记录
// ============================================================================
// 【功能说明】
//   将玩家成绩写入文件，如果已存在记录则覆盖
// 【参数说明】
//   - musicIndex: 音乐索引
//   - difficulty: 难度等级
//   - record: 要保存的成绩记录
// 【实现步骤】
//   1. 读取所有现有记录到内存
//   2. 更新或添加目标记录
//   3. 写回文件（保持原有格式）
// 【关键算法】
//   - 使用map存储所有记录，保证键值唯一性
//   - accuracy×10存储以保留一位小数精度
// ============================================================================
void SaveBestRecord(int musicIndex, Difficulty difficulty, const BestRecord& record) {
    const char* fileName = "best_records.txt";

    // 步骤1：读取所有现有记录
    std::map<std::string, BestRecord> allRecords;
    std::ifstream readFile(fileName);
    if (readFile.is_open()) {
        std::string line;
        while (std::getline(readFile, line)) {
            size_t colonPos = line.find(':');
            if (colonPos != std::string::npos) {
                std::string recordKey = line.substr(0, colonPos);
                std::string data = line.substr(colonPos + 1);
                std::stringstream ss(data);
                std::string part;
                std::vector<int> values;
                while (std::getline(ss, part, ',')) {
                    values.push_back(std::stoi(part));
                }
                if (values.size() >= 4) {
                    BestRecord r;
                    r.score = values[0];
                    r.maxCombo = values[1];
                    r.accuracy = (float)values[2] / 10.0f;
                    r.grade = (char)values[3];
                    allRecords[recordKey] = r;
                }
            }
        }
        readFile.close();
    }

    // 步骤2：更新目标记录
    std::string key = GetRecordKey(musicIndex, difficulty);
    allRecords[key] = record;

    // 步骤3：写回文件
    std::ofstream writeFile(fileName);
    if (writeFile.is_open()) {
        for (const auto& pair : allRecords) {
            const BestRecord& r = pair.second;
            writeFile << pair.first << ":" << r.score << "," << r.maxCombo << ","
                      << (int)(r.accuracy * 10) << "," << (int)r.grade << "\n";
        }
        writeFile.close();
    }
}

// ============================================================================
// LoadMusicList - 加载音乐列表
// ============================================================================
// 【功能说明】
//   定义游戏中所有可用的音乐曲目
// 【返回值】
//   std::vector<MusicInfo>，包含所有音乐信息
// 【MusicInfo字段说明】
//   - name: 菜单中显示的音乐名称
//   - filePath: MP3文件路径
//   - chartFile: 谱面文件基础名（自动拼接 _easy/normal/hard.txt）
// 【添加新音乐方法】
//   1. 准备好MP3文件放入项目目录
//   2. 创建谱面文件（同名_easy/normal/hard.txt）
//   3. 在此函数中添加新的MusicInfo条目
// ============================================================================
std::vector<MusicInfo> LoadMusicList() {
    std::vector<MusicInfo> list;

    // 音乐1: ARForest - Inverted World (Extended Mix)
    MusicInfo m1;
    m1.name = "ARForest - Inverted World";
    m1.filePath = "ARForest - Inverted World (Extended Mix).mp3";
    m1.chartFile = "ARForest - Inverted World (Extended Mix)";
    list.push_back(m1);

    // 音乐2: Reku Mochizuki - INFiNiTE ENERZY
    MusicInfo m2;
    m2.name = "Reku Mochizuki - INFiNiTE ENERZY";
    m2.filePath = "Reku Mochizuki - INFiNiTE ENERZY -Overdoze-.mp3";
    m2.chartFile = "Reku Mochizuki - INFiNiTE ENERZY -Overdoze-";
    list.push_back(m2);

    // 音乐3: 削除 - Distorted Fate
    MusicInfo m3;
    m3.name = "削除 - Distorted Fate";
    m3.filePath = "Distorted Fate.mp3";
    m3.chartFile = "Distorted Fate";
    list.push_back(m3);

    // 音乐4: Summer
    MusicInfo m4;
    m4.name = "Summer";
    m4.filePath = "Summer.mp3";
    m4.chartFile = "Summer";
    list.push_back(m4);

    // 音乐5: m-taku - Komorebi
    MusicInfo m5;
    m5.name = "m-taku - Komorebi";
    m5.filePath = "m-taku - Komorebi.mp3";
    m5.chartFile = "m-taku - Komorebi";
    list.push_back(m5);

    return list;
}

// ============================================================================
// GenerateHitSound - 生成击中音效
// ============================================================================
// 【功能说明】
//   为不同判定类型生成对应的音效
// 【参数说明】
//   - type: 判定类型（PERFECT/GREAT/GOOD/MISS）
// 【返回值】
//   Wave格式的音频数据
// 【实现原理/步骤】
//   1. 尝试从文件加载音效（hit_strong.wav, hit.wav）
//   2. 如果文件不存在，则程序化生成正弦波
//   3. 程序化生成：
//      - 创建指定采样率的音频缓冲区
//      - 生成正弦波
//      - 添加包络（淡出效果）
// 【关键参数】
//   - PERFECT: 880Hz，音量0.6
//   - GREAT: 660Hz，音量0.5
//   - GOOD: 440Hz，音量0.4
//   - MISS: 220Hz，音量0.3
// 【包络算法】
//   envelope = 1 - (i/sampleCount)
//   envelope = envelope²  // 平方实现平滑淡出
//   sample = sin(2π × frequency × t) × volume × envelope
// ============================================================================
Wave GenerateHitSound(JudgmentType type) {
    Wave wave = { 0 };

    // 步骤1：尝试从文件加载
    const char* fileName = nullptr;
    switch (type) {
        case JudgmentType::PERFECT: fileName = "hit_strong.wav"; break;
        case JudgmentType::GREAT: fileName = "hit.wav"; break;
        case JudgmentType::GOOD: fileName = "hit.wav"; break;
        case JudgmentType::MISS: fileName = "hit.wav"; break;
    }

    if (fileName != nullptr) {
        wave = LoadWaveAsset(fileName);
    }

    // 步骤2：程序化生成音效（文件不存在时）
    if (wave.frameCount == 0) {
        int sampleCount = (int)(AUDIO_SAMPLE_RATE * HIT_SOUND_DURATION);
        float* samples = (float*)malloc(sampleCount * sizeof(float));

        float frequency = 440.0f;
        float volume = 0.5f;

        // 根据判定类型设置频率和音量
        switch (type) {
            case JudgmentType::PERFECT: frequency = 880.0f; volume = 0.6f; break;
            case JudgmentType::GREAT: frequency = 660.0f; volume = 0.5f; break;
            case JudgmentType::GOOD: frequency = 440.0f; volume = 0.4f; break;
            case JudgmentType::MISS: frequency = 220.0f; volume = 0.3f; break;
        }

        // 生成正弦波样本
        for (int i = 0; i < sampleCount; i++) {
            float t = (float)i / (float)AUDIO_SAMPLE_RATE;
            float envelope = 1.0f - ((float)i / (float)sampleCount);
            envelope = envelope * envelope;  // 平方包络实现平滑淡出
            samples[i] = sinf(2.0f * PI * frequency * t) * volume * envelope;
        }

        wave.frameCount = (unsigned int)sampleCount;
        wave.sampleRate = (unsigned int)AUDIO_SAMPLE_RATE;
        wave.sampleSize = 16;
        wave.channels = 1;
        wave.data = samples;
    }

    return wave;
}

// ============================================================================
// PlayHitSoundDirect - 直接播放预加载音效
// ============================================================================
// 【功能说明】
//   使用预加载的Sound对象播放击中音效
// 【参数说明】
//   - type: 判定类型
//   - gameState: 游戏状态引用（用于获取音量设置）
// 【实现原理】
//   音效已在游戏初始化时预加载到GameState中，此函数直接播放
// 【关键语句】
//   - SetSoundVolume: 设置音量（使用gameState.hitSoundVolume）
//   - PlaySound: 播放音效
// ============================================================================
void PlayHitSoundDirect(JudgmentType type, GameState& gameState) {
    Sound* soundToPlay = nullptr;
    switch (type) {
        case JudgmentType::PERFECT: soundToPlay = &gameState.perfectSound; break;
        case JudgmentType::GREAT: soundToPlay = &gameState.greatSound; break;
        case JudgmentType::GOOD: soundToPlay = &gameState.goodSound; break;
        case JudgmentType::MISS: soundToPlay = &gameState.missSound; break;
    }

    if (soundToPlay == nullptr || soundToPlay->frameCount == 0) return;

    SetSoundVolume(*soundToPlay, gameState.hitSoundVolume);
    PlaySound(*soundToPlay);
}

// ============================================================================
// PlayHitSound - 从Wave播放音效
// ============================================================================
// 【功能说明】
//   从Wave数据加载并播放音效（一次性使用）
// 【参数说明】
//   - sound: Wave格式的音频数据
// 【实现原理】
//   1. 检查Wave数据是否有效
//   2. 从Wave加载为Sound
//   3. 设置音量并播放
//   注意：此函数每次调用都会创建新的Sound对象
// ============================================================================
void PlayHitSound(Wave sound) {
    if (sound.frameCount == 0) {
        return;
    }

    Sound hitSound = LoadSoundFromWave(sound);
    if (IsSoundReady(hitSound)) {
        SetSoundVolume(hitSound, 0.8f);
        PlaySound(hitSound);
    }
}

// ============================================================================
// GetKeyName - 获取按键名称
// ============================================================================
// 【功能说明】
//   将Raylib按键码转换为可读文本
// 【参数说明】
//   - keyCode: Raylib按键码（如 KEY_A, KEY_D, KEY_SPACE等）
// 【返回值】
//   对应的按键名称字符串（如 "A", "D", "SPACE"）
// 【使用场景】
//   - 设置界面显示当前按键配置
//   - 帮助界面显示按键说明
// ============================================================================
const char* GetKeyName(int keyCode) {
    switch (keyCode) {
        case KEY_A: return "A";
        case KEY_B: return "B";
        case KEY_C: return "C";
        case KEY_D: return "D";
        case KEY_E: return "E";
        case KEY_F: return "F";
        case KEY_G: return "G";
        case KEY_H: return "H";
        case KEY_I: return "I";
        case KEY_J: return "J";
        case KEY_K: return "K";
        case KEY_L: return "L";
        case KEY_M: return "M";
        case KEY_N: return "N";
        case KEY_O: return "O";
        case KEY_P: return "P";
        case KEY_Q: return "Q";
        case KEY_R: return "R";
        case KEY_S: return "S";
        case KEY_T: return "T";
        case KEY_U: return "U";
        case KEY_V: return "V";
        case KEY_W: return "W";
        case KEY_X: return "X";
        case KEY_Y: return "Y";
        case KEY_Z: return "Z";
        case KEY_SPACE: return "SPACE";
        case KEY_ZERO: return "0";
        case KEY_ONE: return "1";
        case KEY_TWO: return "2";
        case KEY_THREE: return "3";
        case KEY_FOUR: return "4";
        case KEY_FIVE: return "5";
        case KEY_SIX: return "6";
        case KEY_SEVEN: return "7";
        case KEY_EIGHT: return "8";
        case KEY_NINE: return "9";
        default: return "???";
    }
}

// ============================================================================
// SaveSettings - 保存游戏设置
// ============================================================================
// 【功能说明】
//   将游戏设置保存到settings.txt文件
// 【参数说明】
//   - gameState: 游戏状态引用，包含所有需要保存的设置项
// 【文件格式】
//   每行一个设置项，格式为"key=value"
// 【保存内容】
//   - 音量设置：music_volume, bg_music_volume, hit_sound_volume
//   - 音频偏移：audio_offset
//   - 音符设置：note_size, note_speed
//   - 按键绑定：key_binding_0~3
//   - 背景音乐启用：bg_music_enabled
// 【实现步骤】
//   1. 打开settings.txt文件
//   2. 写入所有设置项
//   3. 关闭文件
// ============================================================================
void SaveSettings(const GameState& gameState) {
    const char* fileName = "settings.txt";
    std::ofstream file(fileName);
    if (!file.is_open()) {
        return;
    }

    // 音量设置
    file << "music_volume=" << gameState.musicVolume << "\n";
    file << "bg_music_volume=" << gameState.bgMusicVolume << "\n";
    file << "hit_sound_volume=" << gameState.hitSoundVolume << "\n";
    
    // 音频偏移
    file << "audio_offset=" << gameState.audioOffset << "\n";
    
    // 音符设置
    file << "note_size=" << gameState.noteSize << "\n";
    file << "note_speed=" << gameState.noteSpeed << "\n";
    
    // 按键绑定
    file << "key_binding_0=" << gameState.keyBindings[0] << "\n";
    file << "key_binding_1=" << gameState.keyBindings[1] << "\n";
    file << "key_binding_2=" << gameState.keyBindings[2] << "\n";
    file << "key_binding_3=" << gameState.keyBindings[3] << "\n";
    
    // 背景音乐启用状态
    file << "bg_music_enabled=" << (gameState.bgMusicEnabled ? "true" : "false") << "\n";

    file.close();
}

// ============================================================================
// LoadSettings - 加载游戏设置
// ============================================================================
// 【功能说明】
//   从settings.txt文件加载游戏设置
// 【参数说明】
//   - gameState: 游戏状态引用，用于存储加载的设置
// 【实现步骤】
//   1. 打开settings.txt文件
//   2. 逐行读取设置项
//   3. 根据键名解析并设置对应的gameState字段
//   4. 关闭文件
// 【默认值处理】
//   如果文件不存在或某设置项缺失，保持gameState的默认值不变
// ============================================================================
void LoadSettings(GameState& gameState) {
    const char* fileName = "settings.txt";
    std::ifstream file(fileName);
    if (!file.is_open()) {
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        size_t equalsPos = line.find('=');
        if (equalsPos == std::string::npos) {
            continue;
        }

        std::string key = line.substr(0, equalsPos);
        std::string value = line.substr(equalsPos + 1);

        // 解析音量设置
        if (key == "music_volume") {
            gameState.musicVolume = std::stof(value);
        } else if (key == "bg_music_volume") {
            gameState.bgMusicVolume = std::stof(value);
        } else if (key == "hit_sound_volume") {
            gameState.hitSoundVolume = std::stof(value);
        }
        // 解析音频偏移
        else if (key == "audio_offset") {
            gameState.audioOffset = std::stof(value);
        }
        // 解析音符设置
        else if (key == "note_size") {
            gameState.noteSize = std::stof(value);
        } else if (key == "note_speed") {
            gameState.noteSpeed = std::stof(value);
        }
        // 解析按键绑定
        else if (key == "key_binding_0") {
            gameState.keyBindings[0] = std::stoi(value);
        } else if (key == "key_binding_1") {
            gameState.keyBindings[1] = std::stoi(value);
        } else if (key == "key_binding_2") {
            gameState.keyBindings[2] = std::stoi(value);
        } else if (key == "key_binding_3") {
            gameState.keyBindings[3] = std::stoi(value);
        }
        // 解析背景音乐启用状态
        else if (key == "bg_music_enabled") {
            gameState.bgMusicEnabled = (value == "true");
        }
    }

    file.close();
}