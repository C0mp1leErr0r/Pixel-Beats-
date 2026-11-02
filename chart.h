// ============================================================================
// chart.h - 谱面加载和管理功能声明
// ============================================================================
// 本模块处理所有与谱面相关的功能：
//   - 谱面文件加载（LoadChart）
//   - 编辑器谱面加载（LoadEditorChart）
//   - 谱面文件保存（SaveEditorChart）
// ============================================================================
// 【模块关系】
//   - chart.cpp实现谱面功能
//   - 依赖structs.h中的Chart、EditorState、EditorNote等结构体
//   - 依赖constants.h中的Difficulty枚举和常量定义
// ============================================================================

#ifndef CHART_H
#define CHART_H

#include <string>
#include "raylib.h"
#include "constants.h"
#include "structs.h"

// ============================================================================
// 谱面文件加载
// ============================================================================

/**
 * 谱面加载函数 - 核心功能：解析txt格式的谱面文件，构建Note数组
 *
 * 【功能说明】
 *   根据难度加载对应谱面文件，解析出所有音符信息
 *
 * 【参数说明】
 *   - diff: 难度等级（EASY/NORMAL/HARD）
 *   - filePrefix: 谱面文件前缀，会自动拼接 _easy.txt / _normal.txt / _hard.txt
 *
 * 【返回值】
 *   Chart对象，包含notes数组、offset、难度等信息
 *
 * 【文件格式】
 *   music:xxx.mp3          ← 音乐文件路径
 *   offset:0               ← 音频偏移补偿（毫秒）
 *   0,1000                 ← 轨道0，时间戳1000ms
 *   1,1500                 ← 轨道1，时间戳1500ms
 *   2,2000                 ← 轨道2，时间戳2000ms
 *
 * 【实现原理】
 *   1. 拼接文件名：chartFile + "_" + difficulty + ".txt"
 *   2. 逐行读取文件
 *   3. 遇到"music:"行，记录音乐路径
 *   4. 遇到"offset:"行，记录偏移量
 *   5. 遇到其他行（逗号分隔），解析为音符
 *   6. 为每个音符计算初始Y位置
 */
Chart LoadChart(Difficulty diff, const char* filePrefix);

// ============================================================================
// 谱面编辑器功能
// ============================================================================

// 保存谱面到文件
// 【功能】将编辑器中的谱面数据保存到txt文件
// 【参数】
//   - filePath: 目标文件路径
//   - state: 编辑器状态（包含音符列表等）
// 【文件格式】同LoadChart
void SaveEditorChart(const std::string& filePath, const EditorState& state);

// 加载谱面文件到编辑器
// 【功能】从txt文件读取谱面数据到编辑器状态
// 【参数】chartPath: 谱面文件路径
// 【返回值】EditorState结构体，包含音符列表等
// 【与LoadChart的区别】
//   - LoadChart: 用于游戏模式，不支持音符选择状态
//   - LoadEditorChart: 用于编辑器，支持音符选择状态等额外字段
EditorState LoadEditorChart(const std::string& chartPath);

// 保存编辑器谱面（根据音乐信息自动命名）
// 【功能】保存谱面并自动根据音乐信息和难度命名文件
// 【参数】
//   - state: 编辑器状态
//   - musicInfo: 音乐信息（包含chartFile基础名）
// 【文件名格式】{musicInfo.chartFile}_{difficulty}.txt
void SaveEditorChartForMusic(EditorState& state, const MusicInfo& musicInfo);

// 在编辑器中查找指定位置的音符
// 【功能】根据鼠标坐标查找该位置是否有音符
// 【参数】
//   - state: 编辑器状态
//   - mouseX: 鼠标X坐标
//   - mouseY: 鼠标Y坐标
// 【返回值】音符索引，如果没找到返回-1
// 【使用场景】
//   - 编辑器中的音符选择
//   - 编辑器中的音符拖拽
int FindEditorNoteAtPosition(const EditorState& state, int mouseX, int mouseY);

// 检查是否可以在指定位置放置音符（防止重叠）
// 【功能】检查指定轨道和时间是否已经有音符
// 【参数】
//   - state: 编辑器状态
//   - lane: 轨道号
//   - time: 时间戳
//   - excludeIndex: 排除的音符索引（用于拖拽时检查重叠）
// 【返回值】true=可以放置，false=时间太近会重叠
// 【关键常量】MIN_TIME_DIFF = 30ms：同一轨道上两个音符的最小时间差
bool CanPlaceNote(const EditorState& state, int lane, float time, int excludeIndex = -1);

// 谱面编辑器更新函数
// 【功能】处理编辑器中的所有输入和状态更新
// 【参数】
//   - state: 编辑器状态引用
//   - music: Raylib音乐流对象
//   - musicLoaded: 音乐是否已加载
// 【处理内容】
//   - 键盘输入（D/F/J/K添加音符，Delete/X删除音符）
//   - 空格键播放控制
//   - Home/End键跳转
//   - 鼠标操作（点击选择、拖拽移动）
//   - 进度条拖拽
//   - 保存对话框处理
void UpdateEditor(EditorState& state, Music& music, bool musicLoaded);

#endif // CHART_H
