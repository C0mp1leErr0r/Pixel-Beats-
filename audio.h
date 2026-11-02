// ============================================================================
// audio.h - 音频功能声明
// ============================================================================
// 本模块处理所有与音频相关的功能：
//   - 音乐列表加载
//   - 击中音效生成和播放
//   - 成绩记录加载/保存
//   - 按键名称转换
// ============================================================================
// 【模块关系】
//   - audio.cpp实现音频功能，被main.cpp中的游戏主循环调用
//   - 依赖structs.h中的MusicInfo、BestRecord、GameState等结构体
//   - 依赖constants.h中的枚举类型和常量定义
// ============================================================================

#ifndef AUDIO_H
#define AUDIO_H

#include <string>
#include <vector>
#include <map>
#include "raylib.h"
#include "constants.h"
#include "structs.h"

// ============================================================================
// 音乐列表管理
// ============================================================================
// 【功能说明】获取游戏中所有可用的音乐曲目列表
// 【返回类型】std::vector<MusicInfo> - 包含音乐名称、文件路径、谱面文件基础名
// 【使用场景】在游戏主循环中初始化音乐列表，供音乐选择界面使用

// 加载音乐列表（代码中硬编码）
std::vector<MusicInfo> LoadMusicList();

// ============================================================================
// 成绩记录管理
// ============================================================================
// 【功能说明】管理与保存玩家的游戏成绩记录
// 【存储位置】best_records.txt文件
// 【文件格式】"best_{musicIndex}_{difficulty}:score,combo,accuracy_x10,grade"
// 【使用场景】游戏结束后判断是否打破记录，如果打破则保存新记录

// 根据音乐索引和难度生成记录键
// 【参数】musicIndex - 音乐在musicList中的索引；difficulty - 难度等级
// 【返回】格式为"best_{musicIndex}_{difficulty}"的字符串，如"best_0_normal"
std::string GetRecordKey(int musicIndex, Difficulty difficulty);

// 加载特定音乐和难度的最佳记录
// 【参数】musicIndex - 音乐索引；difficulty - 难度等级
// 【返回】BestRecord结构体；如果不存在返回默认值{0, 0, 0.0f, 'D'}
BestRecord LoadBestRecord(int musicIndex, Difficulty difficulty);

// 保存成绩记录
// 【功能】将玩家成绩写入best_records.txt文件
// 【参数】musicIndex - 音乐索引；difficulty - 难度等级；record - 要保存的成绩
void SaveBestRecord(int musicIndex, Difficulty difficulty, const BestRecord& record);

// ============================================================================
// 击中音效生成
// ============================================================================
// 【功能说明】为不同判定类型生成和播放击中音效
// 【优先级】优先使用预加载的Sound对象播放，失败则程序化生成正弦波
// 【音效频率】
//   - PERFECT: 880Hz，高音表示完美击中
//   - GREAT: 660Hz，中高音
//   - GOOD: 440Hz，中音
//   - MISS: 220Hz，低音表示失误

// 为特定判定类型生成音效（优先尝试文件，不存在则生成）
// 【参数】type - 判定类型（PERFECT/GREAT/GOOD/MISS）
// 【返回】Wave格式的音频数据
Wave GenerateHitSound(JudgmentType type);

// 直接播放音效（使用预加载的Sound对象）
// 【功能】使用GameState中预加载的Sound对象播放音效
// 【参数】type - 判定类型；gameState - 游戏状态引用（用于获取音量设置）
// 【实现】从gameState获取对应音效并设置音量后播放
void PlayHitSoundDirect(JudgmentType type, GameState& gameState);

// 从Wave播放音效
// 【功能】从程序化生成的Wave数据加载并播放音效
// 【参数】sound - Wave格式的音频数据
// 【注意】此函数每次调用都会创建新的Sound对象，适合一次性使用
void PlayHitSound(Wave sound);

// ============================================================================
// 辅助函数
// ============================================================================
// 【功能说明】工具函数，用于设置界面显示等场景

// 获取按键名称（用于设置界面显示）
// 【参数】keyCode - Raylib按键码（如KEY_A, KEY_D, KEY_SPACE等）
// 【返回】对应的按键名称字符串（如"A", "D", "SPACE"）
const char* GetKeyName(int keyCode);

// ============================================================================
// 设置持久化管理
// ============================================================================
// 【功能说明】管理游戏设置的保存和加载
// 【存储位置】settings.txt文件
// 【文件格式】每行一个设置项，格式为"key=value"

// 保存游戏设置到settings.txt
// 【参数】gameState - 游戏状态引用，包含所有设置项
void SaveSettings(const GameState& gameState);

// 从settings.txt加载游戏设置
// 【参数】gameState - 游戏状态引用，用于存储加载的设置
void LoadSettings(GameState& gameState);

#endif // AUDIO_H
