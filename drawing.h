// ============================================================================
// drawing.h - 绘制函数声明
// ============================================================================
// 本模块处理所有绘制相关的功能：
//   - 游戏界面绘制
//   - 菜单界面绘制
//   - UI元素绘制
// ============================================================================
// 【模块关系】
//   - drawing.h声明绘制函数，实现位于main.cpp
//   - 被main.cpp中的游戏主循环调用
//   - 依赖structs.h中的数据结构定义
//   - 依赖constants.h中的常量定义
// ============================================================================

#ifndef DRAWING_H
#define DRAWING_H

#include "raylib.h"
#include "constants.h"
#include "structs.h"

// ============================================================================
// 游戏界面绘制
// ============================================================================

// 绘制游戏界面
// 【功能】绘制完整的游戏界面，包括轨道、音符、判定线、特效等
// 【参数】
//   - chart: 谱面引用，包含所有音符信息
//   - gameState: 游戏状态引用
//   - music: 音乐引用（用于进度条）
//   - gameTime: 游戏时间（秒）
// 【实现内容】
//   - 轨道背景、条纹、闪光效果
//   - 判定线和判定区域
//   - 音符绘制（不同轨道不同颜色）
//   - 按键反馈和按键名称
//   - 粒子和特效绘制
void DrawGame(const Chart& chart, const GameState& gameState, Music music, float gameTime);

// 绘制HUD（分数、连击等）
// 【功能】绘制游戏进行中的状态信息
// 【参数】gameState: 游戏状态引用
// 【显示内容】
//   - 左侧：分数、当前连击、最大连击
//   - 右侧：Perfect/Great/Good/Miss计数
void DrawHUD(const GameState& gameState);

// 绘制进度条
// 【功能】在游戏界面上方绘制音乐播放进度条
// 【参数】
//   - music: 音乐引用
//   - chart: 谱面引用（备用，如果music无效则从音符计算时长）
//   - gameTime: 游戏时间（秒）
void DrawProgressBar(Music music, const Chart* chart, float gameTime);

// 绘制暂停菜单
// 【功能】绘制游戏暂停时的半透明覆盖菜单
// 【显示内容】
//   - 半透明黑色覆盖层
//   - Resume/Restart/Quit to Menu按钮
void DrawPauseMenu();

// 绘制结果界面
// 【功能】游戏结束后显示成绩结算界面
// 【参数】gameState: 游戏状态引用（包含最终分数、连击、判定统计）
// 【显示内容】
//   - 评级（S/A/B/C/D）
//   - 最终分数、最大连击
//   - 判定统计（Perfect/Great/Good/Miss）
//   - 准确率百分比
//   - Play Again/Select Music/Main Menu按钮
void DrawResultScreen(const GameState& gameState);

// 绘制按键反馈
// 【功能】在判定线位置绘制按键按下的视觉反馈
// 【参数】
//   - gameState: 游戏状态引用
//   - laneWidth: 单轨道宽度
//   - judgeLineY: 判定线Y坐标
//   - offsetX, offsetY: 绘制偏移量（屏幕震动）
void DrawKeyPressFeedback(const GameState& gameState, int laneWidth, int judgeLineY, int offsetX, int offsetY);

// ============================================================================
// 菜单界面绘制
// ============================================================================

// 绘制主菜单
// 【功能】绘制游戏主菜单界面
// 【参数】editorButtonHovered: 编辑器按钮悬停状态（输出参数）
// 【视觉效果】
//   - 深色渐变背景和动态网格
//   - 标题"Pixel Beats"带发光
//   - Start Game/Chart Editor/Settings按钮
void DrawMainMenu(int editorButtonHovered);

// 绘制音乐选择界面
// 【功能】绘制音乐选择列表界面
// 【参数】
//   - selectedIndex: 当前选中的音乐索引
//   - scrollOffset: 列表滚动偏移量
//   - hoveredItem: 悬停项索引（输出参数）
//   - mouseMoved: 鼠标是否移动过
// 【键盘控制】
//   - Enter或Click：确认选择
//   - 鼠标滚轮：滚动列表
//   - Backspace：返回
void DrawMusicSelect(int selectedIndex, int scrollOffset, int* hoveredItem, bool mouseMoved);

// 绘制难度选择界面
// 【功能】绘制难度选择界面
// 【参数】
//   - selectedIndex: 选中的音乐索引
//   - diff: 当前难度
//   - hoveredOption: 悬停选项索引
//   - isGameMode: 是否游戏模式（true=游戏，false=编辑器）
void DrawDifficultySelect(int selectedIndex, Difficulty diff, int hoveredOption, bool isGameMode);

// 绘制设置界面
// 【功能】绘制游戏设置界面
// 【参数】gameState: 游戏状态引用
// 【设置项】
//   - 音乐音量、音效音量
//   - 音符大小、速度
//   - 按键绑定
//   - 背景音乐开关
void DrawSettings(GameState& gameState);

// 绘制谱面编辑器
// 【功能】绘制谱面编辑器界面
// 【参数】
//   - state: 编辑器状态
//   - music: 音乐引用
//   - musicLoaded: 音乐是否已加载
void DrawEditorScreen(EditorState& state, Music& music, bool musicLoaded);

// ============================================================================
// 通用UI元素
// ============================================================================

// 绘制按钮
// 【功能】绘制一个标准风格的按钮
// 【参数】
//   - text: 按钮文本
//   - x, y, width, height: 按钮位置和尺寸
//   - normalColor: 普通状态背景色
//   - hoverColor: 悬停状态背景色
//   - isHovered: 是否被鼠标悬停
// 【返回值】返回isHovered值
bool DrawButton(const char* text, int x, int y, int width, int height, Color normalColor, Color hoverColor, bool isHovered);

// 辅助函数：检查鼠标是否在矩形内
// 【功能】检测鼠标坐标是否在指定矩形区域内
// 【参数】mx, my - 鼠标坐标；x, y, w, h - 矩形区域
// 【返回值】true=在区域内，false=不在
bool IsMouseInRect(int mx, int my, int x, int y, int w, int h);

#endif // DRAWING_H
