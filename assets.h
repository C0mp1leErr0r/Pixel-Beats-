// ============================================================================
// assets.h - 资源加载（优先读取磁盘文件，找不到时使用内嵌在 EXE 中的副本）
// ============================================================================
// 【设计目的】
//   把音乐、谱面等资源打包进可执行文件，使游戏可以作为一个单独 exe 分发；
//   同时保留“文件优先”的策略：如果 exe 同目录存在同名文件，则优先使用
//   磁盘文件，这样仍然可以替换音乐/修改谱面而不必重新编译。
// ============================================================================
#pragma once

#include "raylib.h"
#include <string>

// 资源是否存在（磁盘文件或内嵌副本之一）
bool AssetExists(const char* fileName);

// 读取文本资源；磁盘文件优先，其次内嵌副本。不存在时返回空字符串。
// 换行符统一为 '\n'，保证与原先 std::ifstream 文本模式的行为一致。
std::string LoadTextAsset(const char* fileName);

// 加载音乐流；磁盘文件优先，其次内嵌副本。失败时返回已清零的 Music。
Music LoadMusicAsset(const char* fileName);

// 加载波形（音效）；磁盘文件优先，其次内嵌副本。失败时返回已清零的 Wave。
Wave LoadWaveAsset(const char* fileName);

// 加载音效（等价于 raylib 的 LoadSound，但支持内嵌资源）。
Sound LoadSoundAsset(const char* fileName);
