// ============================================================================
// main.cpp - 主函数和绘制函数实现
// ============================================================================
// 本文件是游戏的主入口文件，包含了所有绘制函数和游戏主循环
// ============================================================================
// 【文件结构】
//   - 辅助函数（MakeRGB, MakeRGBA, IsMouseInRect）
//   - 绘制函数（DrawGame, DrawHUD, DrawPauseMenu等）
//   - 主函数（main）
// ============================================================================
// 【主要绘制函数】
//   - DrawGame: 绘制游戏界面（轨道、音符、判定线、特效）
//   - DrawHUD: 绘制游戏HUD（分数、连击、统计）
//   - DrawProgressBar: 绘制进度条
//   - DrawPauseMenu: 绘制暂停菜单
//   - DrawResultScreen: 绘制游戏结束界面
//   - DrawMainMenu: 绘制主菜单
//   - DrawMusicSelect: 绘制音乐选择界面
//   - DrawDifficultySelect: 绘制难度选择界面
//   - DrawButton: 通用按钮绘制
//   - DrawKeyPressFeedback: 按键反馈绘制
// ============================================================================
// 【主循环流程】
//   1. 初始化（InitWindow, InitAudioDevice, 加载资源）
//   2. 主循环（while !WindowShouldClose）
//      - BeginDrawing/ClearBackground
//      - 处理菜单状态（MAIN/SELECT_MUSIC/SELECT_DIFFICULTY/EDITOR/SETTINGS）
//      - 处理游戏进行中状态（HandleInput, UpdateGame, DrawGame）
//      - 处理暂停/结算界面
//      - EndDrawing
//   3. 清理资源
// ============================================================================
// 【模块关系】
//   - 依赖 constants.h：常量定义
//   - 依赖 structs.h：数据结构定义
//   - 依赖 audio.h：音频功能声明
//   - 依赖 chart.h：谱面加载声明
//   - 依赖 game.h：游戏逻辑声明
//   - 依赖 effects.h：特效声明
//   - 依赖 drawing.h：绘制函数声明
// ============================================================================

#include "raylib.h"
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <map>

#include "assets.h"   // 内嵌资源加载（单文件 exe 支持）

#pragma comment(lib, "winmm.lib")

#ifndef PI
#define PI 3.14159265358979323846f
#endif

#include "constants.h"
#include "structs.h"
#include "audio.h"
#include "chart.h"
#include "game.h"
#include "effects.h"
#include "drawing.h"

// ============================================================================
// 全局音乐列表
// ============================================================================
std::vector<MusicInfo> musicList;

// ============================================================================
// MakeRGB - 创建不透明颜色
// ============================================================================
// 【功能说明】
//   将RGB分量转换为Raylib Color结构体
// 【参数说明】
//   - r, g, b: 红绿蓝分量（0-255）
// 【返回值】
//   Color类型，alpha固定为255
// ============================================================================
static Color MakeRGB(int r, int g, int b) {
    Color c = { (unsigned char)r, (unsigned char)g, (unsigned char)b, 255 };
    return c;
}

// ============================================================================
// MakeRGBA - 创建带透明度的颜色
// ============================================================================
// 【功能说明】
//   将RGBA分量转换为Raylib Color结构体
// 【参数说明】
//   - r, g, b, a: 红绿蓝和透明度分量（0-255）
// 【返回值】
//   Color类型
// ============================================================================
static Color MakeRGBA(int r, int g, int b, int a) {
    Color c = { (unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a };
    return c;
}

// ============================================================================
// DrawGame - 绘制游戏界面
// ============================================================================
// 【功能说明】
//   绘制完整的游戏界面，包括轨道、音符、判定线、特效等
// 【参数说明】
//   - chart: 谱面引用，包含所有音符信息
//   - gameState: 游戏状态引用
//   - music: 音乐引用（用于进度条）
//   - gameTime: 游戏时间（秒）
// 【实现步骤】
//   1. 清屏并设置深色背景
//   2. 计算屏幕震动偏移量
//   3. 绘制进度条
//   4. 绘制装饰网格背景
//   5. 绘制四个轨道的背景、条纹、闪光效果
//   6. 绘制轨道分隔线和边框
//   7. 绘制判定线（带发光效果）
//   8. 绘制判定区域
//   9. 绘制音符（根据轨道使用不同颜色）
//   10. 绘制按键反馈和按键名称
//   11. 绘制粒子和特效
// 【视觉效果】
//   - 轨道颜色：红、蓝、青、紫
//   - 音符颜色：橙、青、绿、紫
//   - 判定线：白色带发光
//   - 屏幕震动：随机偏移量
// ============================================================================
void DrawGame(const Chart& chart, const GameState& gameState, Music music, float gameTime) {
    ClearBackground(MakeRGBA(20, 20, 30, 255));

    // Get current screen dimensions
    int currentScreenWidth = GetScreenWidth();
    int currentScreenHeight = GetScreenHeight();
    int currentLaneWidth = currentScreenWidth / LANE_COUNT;
    int currentJudgeLineY = currentScreenHeight - 100;

    int shakeOffsetX = 0;
    int shakeOffsetY = 0;
    if (gameState.screenShake > 0) {
        shakeOffsetX = (int)((rand() % 100 - 50) * (gameState.screenShake / 10.0f));
        shakeOffsetY = (int)((rand() % 100 - 50) * (gameState.screenShake / 10.0f));
    }

    DrawProgressBar(music, &chart, gameTime);

    // Draw decorative background grid pattern
    int gridSize = (int)(40.0f * ((float)currentScreenWidth / 800.0f));
    if (gridSize < 30) gridSize = 30;
    for (int y = 0; y < currentScreenHeight; y += gridSize) {
        for (int x = 0; x < currentScreenWidth; x += gridSize) {
            DrawRectangle(x + shakeOffsetX, y + shakeOffsetY, 1, 1, MakeRGBA(40, 40, 60, 30));
        }
    }

    // Draw lane backgrounds with gradient and decorative patterns
    for (int i = 0; i < LANE_COUNT; i++) {
        // Base lane color - different color for each lane
        Color baseColor;
        switch (i) {
            case 0: baseColor = MakeRGBA(50, 20, 20, 255); break;   // Dark red
            case 1: baseColor = MakeRGBA(20, 20, 60, 255); break;   // Dark blue
            case 2: baseColor = MakeRGBA(20, 40, 70, 255); break;   // Dark cyan-blue
            case 3: baseColor = MakeRGBA(40, 20, 50, 255); break;   // Dark magenta
            default: baseColor = MakeRGBA(30, 30, 50, 255); break;
        }

        // Draw full lane background
        DrawRectangle(i * currentLaneWidth + shakeOffsetX, 0, currentLaneWidth, currentScreenHeight, baseColor);

        // Draw decorative lane stripes (scrolling effect simulation)
        float stripeOffset = fmodf(gameTime * 50.0f, 60.0f);
        for (float y = -60 + stripeOffset; y < currentScreenHeight; y += 60) {
            Color stripeColor;
            switch (i) {
                case 0: stripeColor = MakeRGBA(80, 30, 30, 40); break;
                case 1: stripeColor = MakeRGBA(30, 30, 100, 40); break;
                case 2: stripeColor = MakeRGBA(30, 60, 100, 40); break;
                case 3: stripeColor = MakeRGBA(60, 30, 80, 40); break;
                default: stripeColor = MakeRGBA(50, 50, 80, 40); break;
            }
            DrawRectangle(i * currentLaneWidth + shakeOffsetX, (int)y + shakeOffsetY, currentLaneWidth, 2, stripeColor);
        }

        // Draw flash gradient effect (entire lane flashes, brighter near judgment line)
        if (gameState.laneFlash[i] > 0.0f) {
            // Different flash colors for each lane
            Color flashColor;
            switch (i) {
                case 0: flashColor = MakeRGBA(255, 80, 40, (int)(gameState.laneFlash[i] * 200)); break;   // Bright red-orange
                case 1: flashColor = MakeRGBA(60, 160, 255, (int)(gameState.laneFlash[i] * 200)); break;  // Bright cyan-blue
                case 2: flashColor = MakeRGBA(50, 220, 150, (int)(gameState.laneFlash[i] * 200)); break; // Bright green-cyan
                case 3: flashColor = MakeRGBA(200, 80, 255, (int)(gameState.laneFlash[i] * 200)); break; // Bright magenta
                default: flashColor = MakeRGBA(200, 150, 255, (int)(gameState.laneFlash[i] * 200)); break;
            }

            // Draw gradient from top to judgment line (full lane flash)
            int numSteps = 20;
            for (int step = 0; step < numSteps; step++) {
                // Gradient: brighter near judgment line
                float yPos = (float)(step * currentScreenHeight) / (float)numSteps;
                float distToLine = (float)(currentJudgeLineY - yPos) / (float)currentScreenHeight;
                float intensity = gameState.laneFlash[i] * (0.3f + 0.7f * distToLine);

                int flashAlpha = (int)(intensity * 180);
                if (flashAlpha > 255) flashAlpha = 255;
                if (flashAlpha < 0) flashAlpha = 0;

                flashColor.a = (unsigned char)flashAlpha;
                int stripHeight = (int)((float)currentScreenHeight / (float)numSteps) + 1;
                DrawRectangle(i * currentLaneWidth + shakeOffsetX, (int)yPos + shakeOffsetY, currentLaneWidth, stripHeight, flashColor);
            }
        }
    }

    // Draw glowing lane dividers with gradient effect
    for (int i = 1; i < LANE_COUNT; i++) {
        int x = i * currentLaneWidth + shakeOffsetX;
        // Outer glow
        DrawRectangle(x - 4, shakeOffsetY, 2, currentScreenHeight, MakeRGBA(80, 80, 120, 60));
        DrawRectangle(x + 2, shakeOffsetY, 2, currentScreenHeight, MakeRGBA(80, 80, 120, 60));
        // Core line
        DrawRectangle(x - 1, shakeOffsetY, 2, currentScreenHeight, MakeRGBA(120, 120, 160, 200));
        // Highlight edge
        DrawRectangle(x - 2, shakeOffsetY, 1, currentScreenHeight, MakeRGBA(180, 180, 220, 100));
    }

    // Draw outer lane borders (left and right)
    DrawRectangle(shakeOffsetX - 3, shakeOffsetY, 3, currentScreenHeight, MakeRGBA(100, 100, 140, 150));
    DrawRectangle(currentScreenWidth - 3 + shakeOffsetX, shakeOffsetY, 3, currentScreenHeight, MakeRGBA(100, 100, 140, 150));

    // Draw judgment line glow effect
    Color judgmentGlowColor = MakeRGBA(255, 255, 255, 40);
    DrawRectangle(shakeOffsetX, currentJudgeLineY - 8 + shakeOffsetY, currentScreenWidth, 16, judgmentGlowColor);
    DrawRectangle(shakeOffsetX, currentJudgeLineY - 3 + shakeOffsetY, currentScreenWidth, 6, MakeRGBA(255, 255, 255, 180));
    DrawRectangle(shakeOffsetX, currentJudgeLineY + 1 + shakeOffsetY, currentScreenWidth, 2, MakeRGBA(200, 200, 255, 100));

    // Draw judgment area - a subtle zone below judgment line
    // Calculate scale for game elements
    float scaleX = (float)currentScreenWidth / 800.0f;
    float scaleY = (float)currentScreenHeight / 600.0f;
    float scale = (scaleX + scaleY) / 2.0f;
    if (scale < 0.5f) scale = 0.5f;
    if (scale > 2.0f) scale = 2.0f;

    int judgmentAreaHeight = (int)(80 * scaleY);
    if (judgmentAreaHeight < 64) judgmentAreaHeight = 64;
    for (int i = 0; i < LANE_COUNT; i++) {
        Color areaColor;
        switch (i) {
            case 0: areaColor = MakeRGBA(100, 40, 30, 80); break;   // Subtle red tint
            case 1: areaColor = MakeRGBA(40, 40, 100, 80); break;   // Subtle blue tint
            case 2: areaColor = MakeRGBA(40, 80, 120, 80); break;   // Subtle cyan tint
            case 3: areaColor = MakeRGBA(80, 40, 100, 80); break;  // Subtle magenta tint
            default: areaColor = MakeRGBA(60, 60, 100, 80); break;
        }
        DrawRectangle(i * currentLaneWidth + shakeOffsetX, currentJudgeLineY + shakeOffsetY, currentLaneWidth, judgmentAreaHeight, areaColor);
    }

    // Scaled note size with user setting
    int scaledNoteSize = (int)(NOTE_SIZE * scale * gameState.noteSize);
    if (scaledNoteSize < 32) scaledNoteSize = 32;
    if (scaledNoteSize > 80) scaledNoteSize = 80;

    for (const auto& note : chart.notes) {
        if (note.active && !note.judged) {
            int x = note.lane * currentLaneWidth + currentLaneWidth / 2 - scaledNoteSize / 2 + shakeOffsetX;
            int y = (int)(note.y + (float)shakeOffsetY);

            // Note colors - distinct bright colors for each lane
            Color coreColor, rimColor, glowColor;
            switch (note.lane) {
                case 0:  // Lane 0 - Bright Red/Orange
                    coreColor = MakeRGBA(255, 100, 60, 255);
                    rimColor = MakeRGBA(255, 60, 30, 255);
                    glowColor = MakeRGBA(255, 150, 100, 150);
                    break;
                case 1:  // Lane 1 - Bright Cyan/Blue
                    coreColor = MakeRGBA(60, 200, 255, 255);
                    rimColor = MakeRGBA(30, 150, 255, 255);
                    glowColor = MakeRGBA(100, 200, 255, 150);
                    break;
                case 2:  // Lane 2 - Bright Green/Lime
                    coreColor = MakeRGBA(80, 255, 150, 255);
                    rimColor = MakeRGBA(40, 220, 100, 255);
                    glowColor = MakeRGBA(120, 255, 180, 150);
                    break;
                case 3:  // Lane 3 - Bright Magenta/pink
                    coreColor = MakeRGBA(255, 100, 200, 255);
                    rimColor = MakeRGBA(255, 60, 180, 255);
                    glowColor = MakeRGBA(255, 150, 220, 150);
                    break;
                default:
                    coreColor = MakeRGBA(180, 180, 255, 255);
                    rimColor = MakeRGBA(120, 120, 255, 255);
                    glowColor = MakeRGBA(200, 200, 255, 150);
                    break;
            }

            // Draw glow effect (outer shadow)
            Rectangle glowRec = { (float)(x - 3), (float)(y - 3), (float)(scaledNoteSize + 6), (float)(scaledNoteSize + 6) };
            DrawRectangleRounded(glowRec, 0.3f, 4, Fade(glowColor, 0.3f));

            // Draw outer rim (border effect)
            Rectangle rimRec = { (float)(x - 2), (float)(y - 2), (float)(scaledNoteSize + 4), (float)(scaledNoteSize + 4) };
            DrawRectangleRounded(rimRec, 0.3f, 4, rimColor);

            // Draw main note body
            Rectangle noteRec = { (float)x, (float)y, (float)scaledNoteSize, (float)scaledNoteSize };
            DrawRectangleRounded(noteRec, 0.3f, 4, coreColor);

            // Draw highlight strip at top
            Rectangle highlightRec = { (float)(x + 2), (float)(y + 2), (float)(scaledNoteSize - 4), 4.0f };
            DrawRectangleRounded(highlightRec, 0.5f, 2, MakeRGBA(255, 255, 255, 80));

            // Draw inner border for depth
            DrawRectangleRoundedLines(noteRec, 0.3f, 4, 1.0f, Fade(WHITE, 0.3f));
        }
    }

    DrawKeyPressFeedback(gameState, currentLaneWidth, currentJudgeLineY, shakeOffsetX, shakeOffsetY);

    int keyTextSize = (int)(20 * scale);
    if (keyTextSize < 14) keyTextSize = 14;
    int keyTextOffset = (int)(10 * scaleY);
    if (keyTextOffset < 8) keyTextOffset = 8;

    int lane0X = 0 * currentLaneWidth + currentLaneWidth / 2 - (int)(MeasureText(GetKeyName(gameState.keyBindings[0]), keyTextSize) / 2) + shakeOffsetX;
    int lane1X = 1 * currentLaneWidth + currentLaneWidth / 2 - (int)(MeasureText(GetKeyName(gameState.keyBindings[1]), keyTextSize) / 2) + shakeOffsetX;
    int lane2X = 2 * currentLaneWidth + currentLaneWidth / 2 - (int)(MeasureText(GetKeyName(gameState.keyBindings[2]), keyTextSize) / 2) + shakeOffsetX;
    int lane3X = 3 * currentLaneWidth + currentLaneWidth / 2 - (int)(MeasureText(GetKeyName(gameState.keyBindings[3]), keyTextSize) / 2) + shakeOffsetX;
    DrawText(GetKeyName(gameState.keyBindings[0]), lane0X, currentJudgeLineY + keyTextOffset + shakeOffsetY, keyTextSize, DARKGRAY);
    DrawText(GetKeyName(gameState.keyBindings[1]), lane1X, currentJudgeLineY + keyTextOffset + shakeOffsetY, keyTextSize, DARKGRAY);
    DrawText(GetKeyName(gameState.keyBindings[2]), lane2X, currentJudgeLineY + keyTextOffset + shakeOffsetY, keyTextSize, DARKGRAY);
    DrawText(GetKeyName(gameState.keyBindings[3]), lane3X, currentJudgeLineY + keyTextOffset + shakeOffsetY, keyTextSize, DARKGRAY);

    // Draw particles
    DrawParticles(gameState, shakeOffsetX, shakeOffsetY);
    DrawBgParticles(gameState, shakeOffsetX, shakeOffsetY);

    DrawEffects(chart, shakeOffsetX, shakeOffsetY);
}

// ============================================================================
// DrawHUD - 绘制游戏HUD
// ============================================================================
// 【功能说明】
//   绘制游戏进行中的状态信息（分数、连击、判定统计）
// 【参数说明】
//   - gameState: 游戏状态引用
// 【绘制内容】
//   - 左侧：分数、当前连击、最大连击
//   - 右侧：Perfect/Great/Good/Miss计数
//   - 底部：当前按键绑定（调试信息）
// 【颜色映射】
//   - 分数：浅蓝
//   - 连击：紫色（无连击时为灰色）
//   - Perfect：金黄色
//   - Great：浅蓝
//   - Good：绿色
//   - Miss：红色
// ============================================================================
void DrawHUD(const GameState& gameState) {
    int currentScreenWidth = GetScreenWidth();
    int currentScreenHeight = GetScreenHeight();

    // Calculate scale factor
    float scaleX = (float)currentScreenWidth / 800.0f;
    float scaleY = (float)currentScreenHeight / 600.0f;
    float scale = (scaleX + scaleY) / 2.0f;
    if (scale < 0.5f) scale = 0.5f;
    if (scale > 2.0f) scale = 2.0f;

    int hudX = (int)(20 * scaleX);
    int statsWidth = (int)(140 * scaleX);
    if (statsWidth < 112) statsWidth = 112;
    int statsX = currentScreenWidth - statsWidth;

    int hudTextSize = (int)(22 * scale);
    if (hudTextSize < 16) hudTextSize = 16;

    // Background for HUD elements
    int bgWidth = (int)(150 * scaleX);
    int bgHeight = (int)(100 * scaleY);
    DrawRectangle(hudX - (int)(5 * scaleX), (int)(10 * scaleY), bgWidth, bgHeight, Fade(MakeRGBA(30, 30, 45, 255), 0.7f));
    DrawRectangle(statsX - (int)(5 * scaleX), (int)(10 * scaleY), statsWidth, (int)(120 * scaleY), Fade(MakeRGBA(30, 30, 45, 255), 0.7f));

    DrawText(TextFormat("Score: %d", gameState.score), hudX, (int)(20 * scaleY), hudTextSize, MakeRGBA(200, 200, 220, 255));
    DrawText(TextFormat("Combo: %d", gameState.combo), hudX, (int)(50 * scaleY), hudTextSize, gameState.combo > 0 ? PURPLE : MakeRGBA(150, 150, 170, 255));
    DrawText(TextFormat("Max Combo: %d", gameState.maxCombo), hudX, (int)(80 * scaleY), hudTextSize, MakeRGBA(150, 150, 170, 255));

    DrawText(TextFormat("Perfect: %d", gameState.perfectCount), statsX, (int)(20 * scaleY), hudTextSize, GOLD);
    DrawText(TextFormat("Great: %d", gameState.greatCount), statsX, (int)(50 * scaleY), hudTextSize, MakeRGBA(100, 150, 255, 255));
    DrawText(TextFormat("Good: %d", gameState.goodCount), statsX, (int)(80 * scaleY), hudTextSize, MakeRGBA(100, 200, 100, 255));
    DrawText(TextFormat("Miss: %d", gameState.missCount), statsX, (int)(110 * scaleY), hudTextSize, MakeRGBA(255, 100, 100, 255));
}

// ============================================================================
// DrawProgressBar - 绘制进度条
// ============================================================================
// 【功能说明】
//   在游戏界面上方绘制音乐播放进度条
// 【参数说明】
//   - music: 音乐引用（用于获取播放时间和总时长）
//   - chart: 谱面引用（备用，如果music无效则从音符计算时长）
//   - gameTime: 游戏时间（秒）
// 【进度计算】
//   - 如果音乐有效：progress = currentTime / totalTime
//   - 否则：progress = gameTime * 1000 / totalTime
// 【UI元素】
//   - 进度条背景框
//   - 进度条填充（紫色）
//   - 时间文本（当前时间 / 总时长）
// ============================================================================
void DrawProgressBar(Music music, const Chart* chart, float gameTime) {
    float scaleX = (float)GetScreenWidth() / 800.0f;
    float scaleY = (float)GetScreenHeight() / 600.0f;
    float scale = (scaleX + scaleY) / 2.0f;
    if (scale < 0.5f) scale = 0.5f;
    if (scale > 2.0f) scale = 2.0f;

    float currentTime = 0.0f;
    float totalTime = 0.0f;

    if (music.frameCount > 0) {
        currentTime = GetMusicTimePlayed(music);
        totalTime = GetMusicTimeLength(music);
    }

    if (totalTime <= 0 && chart != nullptr && !chart->notes.empty()) {
        for (const auto& note : chart->notes) {
            if (note.hitTime > totalTime) totalTime = note.hitTime;
        }
        currentTime = gameTime * 1000.0f;
    }

    if (totalTime <= 0) totalTime = 180000.0f;

    float progress = (totalTime > 0) ? (currentTime / totalTime) : 0.0f;
    if (progress > 1.0f) progress = 1.0f;
    if (progress < 0.0f) progress = 0.0f;

    int screenWidth = GetScreenWidth();
    int barWidth = screenWidth / 3;
    if (barWidth > 400) barWidth = 400;
    int barHeight = (int)(20 * scale);
    if (barHeight < 16) barHeight = 16;
    int barX = screenWidth / 2 - barWidth / 2;
    int barY = (int)(30 * scaleY);

    DrawRectangle(barX - 2, barY - 2, barWidth + 4, barHeight + 4, MakeRGBA(60, 60, 80, 255));
    DrawRectangle(barX, barY, barWidth, barHeight, MakeRGBA(40, 40, 60, 255));
    DrawRectangle(barX, barY, (int)((float)barWidth * progress), barHeight, PURPLE);

    int currentSec = (int)(currentTime / 1000.0f);
    int totalSec = (int)(totalTime / 1000.0f);
    char timeText[32];
    snprintf(timeText, sizeof(timeText), "%d:%02d / %d:%02d", currentSec / 60, currentSec % 60, totalSec / 60, totalSec % 60);
    DrawText(timeText, barX, barY + barHeight + 5, 12, MakeRGBA(200, 200, 220, 255));
}

// ============================================================================
// DrawPauseMenu - 绘制暂停菜单
// ============================================================================
// 【功能说明】
//   绘制游戏暂停时的半透明覆盖菜单
// 【实现内容】
//   - 半透明黑色覆盖层
//   - "PAUSED"标题（紫色）
//   - Resume按钮（蓝色悬停）
//   - Restart按钮（橙色悬停）
//   - Quit to Menu按钮（红色悬停）
//   - 底部提示（P或ESC继续）
// 【按钮交互】
//   - 鼠标悬停时改变按钮颜色
//   - 使用IsMouseInRect检测鼠标位置
// ============================================================================
void DrawPauseMenu() {
    int currentScreenWidth = GetScreenWidth();
    int currentScreenHeight = GetScreenHeight();
    int mx = GetMouseX();
    int my = GetMouseY();

    // Calculate scale factor
    float scaleX = (float)currentScreenWidth / 800.0f;
    float scaleY = (float)currentScreenHeight / 600.0f;
    float scale = (scaleX + scaleY) / 2.0f;
    if (scale < 0.5f) scale = 0.5f;
    if (scale > 2.0f) scale = 2.0f;

    // Semi-transparent overlay
    DrawRectangle(0, 0, currentScreenWidth, currentScreenHeight, Fade(MakeRGBA(10, 10, 20, 255), 0.85f));

    // Pause box with scaling
    int boxWidth = (int)(350 * scaleX);
    if (boxWidth < 280) boxWidth = 280;
    int boxHeight = (int)(280 * scaleY);
    if (boxHeight < 224) boxHeight = 224;
    int boxX = currentScreenWidth / 2 - boxWidth / 2;
    int boxY = currentScreenHeight / 2 - boxHeight / 2;

    Rectangle pauseBox = { (float)boxX, (float)boxY, (float)boxWidth, (float)boxHeight };
    DrawRectangleRounded(pauseBox, 0.15f, 8, Fade(MakeRGBA(35, 35, 50, 255), 0.95f));
    DrawRectangleRoundedLines(pauseBox, 0.15f, 8, 2.0f, PURPLE);

    // Title
    const char* title = "PAUSED";
    int titleSize = (int)(50 * scale);
    if (titleSize < 36) titleSize = 36;
    int titleWidth = MeasureText(title, titleSize);
    DrawText(title, currentScreenWidth / 2 - titleWidth / 2, boxY + (int)(30 * scaleY), titleSize, PURPLE);

    // Buttons with scaling
    int btnWidth = (int)(200 * scaleX);
    if (btnWidth < 160) btnWidth = 160;
    int btnHeight = (int)(45 * scaleY);
    if (btnHeight < 36) btnHeight = 36;
    int btnX = currentScreenWidth / 2 - btnWidth / 2;

    int resumeBtnY = boxY + (int)(100 * scaleY);
    bool resumeHovered = IsMouseInRect(mx, my, btnX, resumeBtnY, btnWidth, btnHeight);
    DrawButton("Resume", btnX, resumeBtnY, btnWidth, btnHeight, Fade(MakeRGBA(80, 50, 120, 255), 0.8f), MakeRGBA(80, 180, 255, 255), resumeHovered);

    int restartBtnY = boxY + (int)(160 * scaleY);
    bool restartHovered = IsMouseInRect(mx, my, btnX, restartBtnY, btnWidth, btnHeight);
    DrawButton("Restart", btnX, restartBtnY, btnWidth, btnHeight, Fade(MakeRGBA(60, 60, 80, 255), 0.8f), MakeRGBA(255, 180, 80, 255), restartHovered);

    int quitBtnY = boxY + (int)(220 * scaleY);
    bool quitHovered = IsMouseInRect(mx, my, btnX, quitBtnY, btnWidth, btnHeight);
    DrawButton("Quit to Menu", btnX, quitBtnY, btnWidth, btnHeight, Fade(MakeRGBA(60, 60, 80, 255), 0.8f), MakeRGBA(200, 80, 80, 255), quitHovered);

    int hintSize = (int)(16 * scale);
    if (hintSize < 12) hintSize = 12;
    DrawText("Press P to Resume", currentScreenWidth / 2 - (int)(90 * scaleX), currentScreenHeight - (int)(35 * scaleY), hintSize, Fade(MakeRGBA(120, 120, 140, 255), 0.6f));
}

// ============================================================================
// DrawResultScreen - 绘制游戏结算界面
// ============================================================================
// 【功能说明】
//   游戏结束后显示成绩结算界面
// 【参数说明】
//   - gameState: 游戏状态引用（包含最终分数、连击、判定统计）
// 【显示内容】
//   - "Game Over"标题
//   - 评级（S/A/B/C/D，根据准确率计算）
//   - 最终分数
//   - 最大连击
//   - 判定统计（Perfect/Great/Good/Miss）
//   - 准确率百分比
//   - 三个按钮：Play Again、Select Music、Main Menu
// 【按钮功能】
//   - Play Again：重新开始当前游戏
//   - Select Music：返回音乐选择
//   - Main Menu：返回主菜单
// ============================================================================
void DrawResultScreen(const GameState& gameState) {
    int currentScreenWidth = GetScreenWidth();
    int currentScreenHeight = GetScreenHeight();
    int mx = GetMouseX();
    int my = GetMouseY();

    // Calculate scale factor
    float scaleX = (float)currentScreenWidth / 800.0f;
    float scaleY = (float)currentScreenHeight / 600.0f;
    float scale = (scaleX + scaleY) / 2.0f;
    if (scale < 0.5f) scale = 0.5f;
    if (scale > 2.0f) scale = 2.0f;

    // Background
    DrawRectangle(0, 0, currentScreenWidth, currentScreenHeight, Fade(MakeRGBA(20, 15, 35, 255), 0.95f));

    // Result box with scaling
    int boxWidth = (int)(400 * scaleX);
    if (boxWidth < 320) boxWidth = 320;
    int boxHeight = (int)(420 * scaleY);
    if (boxHeight < 336) boxHeight = 336;
    int boxX = currentScreenWidth / 2 - boxWidth / 2;
    int boxY = (int)(50 * scaleY);
    
    Rectangle resultBox = { (float)boxX, (float)boxY, (float)boxWidth, (float)boxHeight };
    DrawRectangleRounded(resultBox, 0.15f, 8, Fade(MakeRGBA(35, 30, 50, 255), 0.95f));
    DrawRectangleRoundedLines(resultBox, 0.15f, 8, 2.0f, PURPLE);

    // Title
    const char* title = "Game Over";
    int titleSize = (int)(40 * scale);
    if (titleSize < 28) titleSize = 28;
    int titleWidth = MeasureText(title, titleSize);
    DrawText(title, currentScreenWidth / 2 - titleWidth / 2, boxY + (int)(20 * scaleY), titleSize, MakeRGBA(180, 180, 220, 255));

    int totalNotes = gameState.perfectCount + gameState.greatCount + gameState.goodCount + gameState.missCount;
    float accuracy = 0.0f;
    if (totalNotes > 0) {
        accuracy = (float)(gameState.perfectCount * 100 + gameState.greatCount * 80 + gameState.goodCount * 50) / (float)totalNotes;
    }

    char grade = GetGrade(accuracy);
    Color gradeColor = MakeRGBA(150, 150, 170, 255);
    switch (grade) {
        case 'S': gradeColor = GOLD; break;
        case 'A': gradeColor = MakeRGBA(80, 200, 80, 255); break;
        case 'B': gradeColor = MakeRGBA(100, 100, 220, 255); break;
        case 'C': gradeColor = PURPLE; break;
        case 'D': gradeColor = MakeRGBA(255, 100, 100, 255); break;
    }

    // Large grade display
    int gradeSize = (int)(50 * scale);
    if (gradeSize < 36) gradeSize = 36;
    DrawText(TextFormat("Rank: %c", grade), currentScreenWidth / 2 - (int)(60 * scaleX), boxY + (int)(80 * scaleY), gradeSize, gradeColor);

    int statSize = (int)(22 * scale);
    if (statSize < 16) statSize = 16;
    int smallSize = (int)(18 * scale);
    if (smallSize < 14) smallSize = 14;
    int labelSize = (int)(22 * scale);
    if (labelSize < 16) labelSize = 16;

    DrawText(TextFormat("Final Score: %d", gameState.score), currentScreenWidth / 2 - (int)(70 * scaleX), boxY + (int)(140 * scaleY), statSize, MakeRGBA(180, 180, 200, 255));
    DrawText(TextFormat("Max Combo: %d", gameState.maxCombo), currentScreenWidth / 2 - (int)(60 * scaleX), boxY + (int)(175 * scaleY), smallSize, MakeRGBA(150, 150, 170, 255));

    // Stats with colors
    DrawText("Statistics:", currentScreenWidth / 2 - (int)(40 * scaleX), boxY + (int)(210 * scaleY), smallSize, MakeRGBA(150, 150, 170, 255));
    DrawText(TextFormat("Perfect: %d", gameState.perfectCount), currentScreenWidth / 2 - (int)(50 * scaleX), boxY + (int)(240 * scaleY), smallSize, GOLD);
    DrawText(TextFormat("Great: %d", gameState.greatCount), currentScreenWidth / 2 - (int)(50 * scaleX), boxY + (int)(265 * scaleY), smallSize, MakeRGBA(100, 150, 255, 255));
    DrawText(TextFormat("Good: %d", gameState.goodCount), currentScreenWidth / 2 - (int)(50 * scaleX), boxY + (int)(290 * scaleY), smallSize, MakeRGBA(100, 200, 100, 255));
    DrawText(TextFormat("Miss: %d", gameState.missCount), currentScreenWidth / 2 - (int)(50 * scaleX), boxY + (int)(315 * scaleY), smallSize, MakeRGBA(255, 100, 100, 255));

    if (totalNotes > 0) {
        DrawText(TextFormat("Accuracy: %.1f%%", accuracy), currentScreenWidth / 2 - (int)(50 * scaleX), boxY + (int)(350 * scaleY), labelSize, PURPLE);
    }

    // Buttons with scaling - 3 buttons layout
    int btnWidth = (int)(150 * scaleX);
    if (btnWidth < 120) btnWidth = 120;
    int btnHeight = (int)(45 * scaleY);
    if (btnHeight < 36) btnHeight = 36;
    int btnX = currentScreenWidth / 2 - btnWidth / 2;
    int btnY = boxY + boxHeight + (int)(20 * scaleY);

    int btnSpacing = (int)(170 * scaleX);

    // Play Again button (left)
    bool playAgainHovered = IsMouseInRect(mx, my, btnX - btnSpacing, btnY, btnWidth, btnHeight);
    DrawButton("Play Again", btnX - btnSpacing, btnY, btnWidth, btnHeight, Fade(MakeRGBA(80, 50, 120, 255), 0.8f), MakeRGBA(80, 180, 255, 255), playAgainHovered);

    // Select Music button (center)
    bool selectMusicHovered = IsMouseInRect(mx, my, btnX, btnY, btnWidth, btnHeight);
    DrawButton("Select Music", btnX, btnY, btnWidth, btnHeight, Fade(MakeRGBA(50, 80, 100, 255), 0.8f), MakeRGBA(80, 200, 150, 255), selectMusicHovered);

    // Main Menu button (right)
    bool menuHovered = IsMouseInRect(mx, my, btnX + btnSpacing, btnY, btnWidth, btnHeight);
    DrawButton("Main Menu", btnX + btnSpacing, btnY, btnWidth, btnHeight, Fade(MakeRGBA(60, 60, 80, 255), 0.8f), MakeRGBA(200, 80, 80, 255), menuHovered);

    int hintSize = (int)(16 * scale);
    if (hintSize < 12) hintSize = 12;
    DrawText("Press R to Restart", currentScreenWidth / 2 - (int)(120 * scaleX), currentScreenHeight - (int)(30 * scaleY), hintSize, Fade(MakeRGBA(120, 120, 140, 255), 0.6f));
}

// ============================================================================
// DrawKeyPressFeedback - 绘制按键反馈
// ============================================================================
// 【功能说明】
//   在判定线位置绘制按键按下的视觉反馈
// 【参数说明】
//   - gameState: 游戏状态引用
//   - laneWidth: 单轨道宽度
//   - judgeLineY: 判定线Y坐标
//   - offsetX, offsetY: 绘制偏移量（屏幕震动）
// 【实现内容】
//   - 遍历四个轨道
//   - 如果对应按键被按下，在判定线位置绘制白色矩形条
// ============================================================================
void DrawKeyPressFeedback(const GameState& gameState, int laneWidth, int judgeLineY, int offsetX, int offsetY) {
    for (int i = 0; i < LANE_COUNT; i++) {
        if (gameState.keyPressed[i]) {
            DrawRectangle(i * laneWidth + offsetX, judgeLineY + offsetY - 5, laneWidth, 10, Fade(WHITE, 0.5f));
        }
    }
}

// ============================================================================
// DrawMainMenu - 绘制主菜单
// ============================================================================
// 【功能说明】
//   绘制游戏主菜单界面
// 【参数说明】
//   - editorButtonHovered: 编辑器按钮悬停状态（输出参数）
// 【视觉效果】
//   - 深色渐变背景
//   - 动态网格背景（滚动效果）
//   - 四角技术风格装饰
//   - 漂浮粒子效果
//   - 发光球体背景装饰
//   - 标题"Pixel Beats"带发光
//   - 副标题"A Simple Rhythm Game!"
// 【按钮】
//   - Start Game：开始游戏入口
//   - Chart Editor：谱面编辑器入口
//   - Settings：设置入口
// ============================================================================
void DrawMainMenu(int editorButtonHovered) {
    int currentScreenWidth = GetScreenWidth();
    int currentScreenHeight = GetScreenHeight();

    // 基于屏幕尺寸计算缩放因子（基准800x600）
    float scaleX = (float)currentScreenWidth / 800.0f;
    float scaleY = (float)currentScreenHeight / 600.0f;
    float scale = (scaleX + scaleY) / 2.0f;
    if (scale < 0.5f) scale = 0.5f;
    if (scale > 2.0f) scale = 2.0f;

    // 深色渐变背景
    for (int y = 0; y < currentScreenHeight; y += 4) {
        float ratio = (float)y / currentScreenHeight;
        unsigned char r = (unsigned char)(25 + 15 * ratio);
        unsigned char g = (unsigned char)(25 + 10 * ratio);
        unsigned char b = (unsigned char)(35 + 20 * ratio);
        Color gradColor = { r, g, b, 255 };
        DrawRectangle(0, y, currentScreenWidth, 4, gradColor);
    }

    // ===== 技术风格装饰 =====
    float time = (float)GetTime();

    // 动态网格背景
    int gridSize = (int)(50 * scaleX);
    if (gridSize < 40) gridSize = 40;
    float gridOffset = fmodf(time * 20.0f, (float)gridSize);
    for (int x = (int)(-gridOffset); x < currentScreenWidth; x += gridSize) {
        DrawRectangle(x, 0, 1, currentScreenHeight, Fade(MakeRGBA(80, 50, 120, 60), 0.3f));
    }
    for (int y = 0; y < currentScreenHeight; y += gridSize) {
        DrawRectangle(0, y, currentScreenWidth, 1, Fade(MakeRGBA(80, 50, 120, 60), 0.3f));
    }

    // 四角装饰（技术框架）
    int cornerSize = (int)(80 * scaleX);
    if (cornerSize < 60) cornerSize = 60;
    Color frameColor = MakeRGBA(100, 80, 180, 150);
    // 左上角
    DrawLine(20, (int)(20 * scaleY), 20 + cornerSize, (int)(20 * scaleY), frameColor);
    DrawLine((int)(20 * scaleX), (int)(20 * scaleY), (int)(20 * scaleX), (int)(20 * scaleY) + cornerSize, frameColor);
    DrawLine((int)(20 * scaleX), (int)(20 * scaleY), (int)(20 * scaleX) + cornerSize/3, (int)(20 * scaleY) + cornerSize/3, frameColor);
    // 右上角
    DrawLine(currentScreenWidth - 20 - cornerSize, (int)(20 * scaleY), currentScreenWidth - 20, (int)(20 * scaleY), frameColor);
    DrawLine(currentScreenWidth - (int)(20 * scaleX), (int)(20 * scaleY), currentScreenWidth - (int)(20 * scaleX), (int)(20 * scaleY) + cornerSize, frameColor);
    DrawLine(currentScreenWidth - (int)(20 * scaleX), (int)(20 * scaleY), currentScreenWidth - (int)(20 * scaleX) - cornerSize/3, (int)(20 * scaleY) + cornerSize/3, frameColor);
    // 左下角
    DrawLine((int)(20 * scaleX), currentScreenHeight - (int)(20 * scaleY), (int)(20 * scaleX) + cornerSize, currentScreenHeight - (int)(20 * scaleY), frameColor);
    DrawLine((int)(20 * scaleX), currentScreenHeight - (int)(20 * scaleY), (int)(20 * scaleX), currentScreenHeight - (int)(20 * scaleY) - cornerSize, frameColor);
    // 右下角
    DrawLine(currentScreenWidth - 20 - cornerSize, currentScreenHeight - (int)(20 * scaleY), currentScreenWidth - 20, currentScreenHeight - (int)(20 * scaleY), frameColor);
    DrawLine(currentScreenWidth - (int)(20 * scaleX), currentScreenHeight - (int)(20 * scaleY), currentScreenWidth - (int)(20 * scaleX), currentScreenHeight - (int)(20 * scaleY) - cornerSize, frameColor);

    // 漂浮粒子
    for (int i = 0; i < 15; i++) {
        float px = fmodf((float)i * 137.5f + time * 30.0f, (float)currentScreenWidth);
        float py = fmodf((float)i * 89.3f + time * 20.0f, (float)currentScreenHeight);
        int particleSize = (int)(3 * scale);
        if (particleSize < 2) particleSize = 2;
        Color particleColor = (i % 2 == 0) ? MakeRGBA(150, 100, 200, 100) : MakeRGBA(100, 200, 150, 80);
        DrawRectangle((int)px, (int)py, particleSize, particleSize, particleColor);
    }

    // 发光球体背景装饰
    int orbX = (int)(currentScreenWidth * 0.85f);
    int orbY = (int)(currentScreenHeight * 0.3f);
    int orbRadius = (int)(60 * scale);
    float orbPulse = 0.7f + 0.3f * sinf(time * 2.0f);
    DrawCircleGradient(orbX, orbY, orbRadius, Fade(MakeRGBA(100, 50, 150, 100), orbPulse), BLANK);
    DrawCircleGradient(orbX - (int)(40 * scaleX), orbY + (int)(50 * scaleY), (int)(30 * scale), Fade(MakeRGBA(50, 100, 150, 80), orbPulse), BLANK);

    // 标题阴影和发光效果
    const char* title = "Pixel Beats";
    int titleSize = (int)(50 * scale);
    if (titleSize < 20) titleSize = 20;
    int titleWidth = MeasureText(title, titleSize);

    // 标题发光层
    for (int i = 3; i > 0; i--) {
        DrawText(title, currentScreenWidth / 2 - titleWidth / 2, (int)(80 * scaleY), titleSize, Fade(MakeRGBA(150, 100, 200, 50), 0.2f * i));
    }
    DrawText(title, currentScreenWidth / 2 - titleWidth / 2 + 3, (int)(80 * scaleY), titleSize, Fade(MakeRGBA(50, 20, 80, 255), 0.5f));
    DrawText(title, currentScreenWidth / 2 - titleWidth / 2, (int)(80 * scaleY), titleSize, PURPLE);

    const char* subtitle = "A Simple Rhythm Game!";
    int subSize = (int)(22 * scale);
    if (subSize < 14) subSize = 14;
    int subWidth = MeasureText(subtitle, subSize);
    DrawText(subtitle, currentScreenWidth / 2 - subWidth / 2, (int)(140 * scaleY), subSize, Fade(PURPLE, 0.8f));

    // 装饰线带脉动发光
    int lineWidth = (int)(200 * scaleX);
    float linePulse = 0.6f + 0.4f * sinf(time * 3.0f);
    DrawLine(currentScreenWidth / 2 - lineWidth / 2, (int)(175 * scaleY), currentScreenWidth / 2 + lineWidth / 2, (int)(175 * scaleY), Fade(MakeRGBA(150, 100, 200, (int)(100 * linePulse)), 0.5f));
    DrawCircle(currentScreenWidth / 2, (int)(175 * scaleY), (int)(5 * scale * linePulse), MakeRGBA(200, 150, 255, 200));

    // 两个主按钮并排
    int btnWidth = (int)(280 * scaleX);
    if (btnWidth < 224) btnWidth = 224;
    int btnHeight = (int)(60 * scaleY);
    if (btnHeight < 48) btnHeight = 48;

    // 开始游戏按钮（左侧）
    int playBtnX = currentScreenWidth / 2 - btnWidth - (int)(30 * scaleX);
    int playBtnY = (int)(240 * scaleY);
    Color playNormalColor = Fade(MakeRGBA(80, 50, 120, 255), 0.8f);
    Color playHoverColor = MakeRGBA(100, 150, 200, 255);
    bool playHovered = IsMouseInRect(GetMouseX(), GetMouseY(), playBtnX, playBtnY, btnWidth, btnHeight);
    DrawButton("Start Game", playBtnX, playBtnY, btnWidth, btnHeight, playNormalColor, playHoverColor, playHovered);
    // 开始按钮技术边框
    if (playHovered) {
        DrawRectangleLines(playBtnX, playBtnY, btnWidth, btnHeight, Fade(MakeRGBA(100, 150, 200, 150), 0.6f));
        DrawRectangle(playBtnX - 2, playBtnY - 2, btnWidth + 4, 2, Fade(MakeRGBA(100, 200, 255, 150), 0.4f));
    }

    // 谱面编辑器按钮（右侧）
    int editorBtnX = currentScreenWidth / 2 + (int)(30 * scaleX);
    int editorBtnY = (int)(240 * scaleY);
    Color editorNormalColor = Fade(MakeRGBA(50, 80, 100, 255), 0.8f);
    Color editorHoverColor = MakeRGBA(80, 200, 150, 255);
    editorButtonHovered = IsMouseInRect(GetMouseX(), GetMouseY(), editorBtnX, editorBtnY, btnWidth, btnHeight);
    DrawButton("Chart Editor", editorBtnX, editorBtnY, btnWidth, btnHeight, editorNormalColor, editorHoverColor, editorButtonHovered);
    // 编辑器按钮技术边框
    if (editorButtonHovered) {
        DrawRectangleLines(editorBtnX, editorBtnY, btnWidth, btnHeight, Fade(MakeRGBA(80, 200, 150, 150), 0.6f));
        DrawRectangle(editorBtnX - 2, editorBtnY - 2, btnWidth + 4, 2, Fade(MakeRGBA(80, 255, 180, 150), 0.4f));
    }

    // 设置按钮（底部居中）
    int settingsBtnW = (int)(180 * scaleX);
    if (settingsBtnW < 144) settingsBtnW = 144;
    int settingsBtnH = (int)(45 * scaleY);
    if (settingsBtnH < 36) settingsBtnH = 36;
    int settingsBtnX = currentScreenWidth / 2 - settingsBtnW / 2;
    int settingsBtnY = (int)(320 * scaleY);
    Color settingsNormalColor = Fade(MakeRGBA(60, 60, 80, 255), 0.8f);
    Color settingsHoverColor = MakeRGBA(150, 120, 180, 255);
    bool settingsHovered = IsMouseInRect(GetMouseX(), GetMouseY(), settingsBtnX, settingsBtnY, settingsBtnW, settingsBtnH);
    DrawButton("Settings", settingsBtnX, settingsBtnY, settingsBtnW, settingsBtnH, settingsNormalColor, settingsHoverColor, settingsHovered);

    // 退出按钮（在设置按钮下方）
    int exitBtnW = (int)(140 * scaleX);
    if (exitBtnW < 112) exitBtnW = 112;
    int exitBtnH = (int)(40 * scaleY);
    if (exitBtnH < 32) exitBtnH = 32;
    int exitBtnX = currentScreenWidth / 2 - exitBtnW / 2;
    int exitBtnY = settingsBtnY + settingsBtnH + (int)(20 * scaleY);
    Color exitNormalColor = Fade(MakeRGBA(100, 40, 40, 255), 0.8f);
    Color exitHoverColor = MakeRGBA(180, 80, 80, 255);
    bool exitHovered = IsMouseInRect(GetMouseX(), GetMouseY(), exitBtnX, exitBtnY, exitBtnW, exitBtnH);
    DrawButton("Exit", exitBtnX, exitBtnY, exitBtnW, exitBtnH, exitNormalColor, exitHoverColor, exitHovered);
    if (exitHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        CloseWindow();
    }

    // 扫描线效果
    for (int y = 0; y < currentScreenHeight; y += 3) {
        DrawRectangle(0, y, currentScreenWidth, 1, Fade(BLACK, 0.05f));
    }
}

// ============================================================================
// DrawButton - 绘制通用按钮
// ============================================================================
// 【功能说明】
//   绘制一个标准风格的按钮
// 【参数说明】
//   - text: 按钮文本
//   - x, y, width, height: 按钮位置和尺寸
//   - normalColor: 普通状态背景色
//   - hoverColor: 悬停状态背景色
//   - isHovered: 是否被鼠标悬停
// 【返回值】
//   返回isHovered值
// 【实现内容】
//   - 绘制背景矩形
//   - 绘制白色边框
//   - 绘制居中文本
// ============================================================================
bool DrawButton(const char* text, int x, int y, int width, int height, Color normalColor, Color hoverColor, bool isHovered) {
    Color bgColor = isHovered ? hoverColor : normalColor;
    
    Rectangle btnRec = { (float)x, (float)y, (float)width, (float)height };
    DrawRectangleRounded(btnRec, 0.25f, 6, bgColor);
    DrawRectangleRoundedLines(btnRec, 0.25f, 6, 2.0f, WHITE);

    int textWidth = MeasureText(text, 20);
    int textX = x + (width - textWidth) / 2;
    int textY = y + (height - 20) / 2;
    DrawText(text, textX, textY, 20, WHITE);

    return isHovered;
}

// ============================================================================
// DrawMusicSelect - 绘制音乐选择界面
// ============================================================================
// 【功能说明】
//   绘制音乐选择列表界面
// 【参数说明】
//   - selectedIndex: 当前选中的音乐索引
//   - scrollOffset: 列表滚动偏移量
//   - hoveredItem: 悬停项索引（输出参数）
//   - mouseMoved: 鼠标是否移动过
// 【实现内容】
//   - 深色渐变背景和动态网格
//   - 标题栏"Select Music"
//   - Back返回按钮
//   - 可滚动的音乐列表（显示7项）
//   - 选中项高亮显示
//   - 滚动条（上下按钮）
//   - 位置信息面板
//   - 底部操作提示
// 【键盘控制】
//   - Enter或Click：确认选择
//   - 鼠标滚轮：滚动列表
//   - Backspace：返回
// ============================================================================
void DrawMusicSelect(int selectedIndex, int scrollOffset, int* hoveredItem, bool mouseMoved) {
    int currentScreenWidth = GetScreenWidth();
    int currentScreenHeight = GetScreenHeight();
    int mx = GetMouseX();
    int my = GetMouseY();

    // 基于屏幕尺寸计算缩放因子（基准800x600）
    float scaleX = (float)currentScreenWidth / 800.0f;
    float scaleY = (float)currentScreenHeight / 600.0f;
    float scale = (scaleX + scaleY) / 2.0f;
    if (scale < 0.5f) scale = 0.5f;
    if (scale > 2.0f) scale = 2.0f;

    float time = (float)GetTime();

    // 深色渐变背景
    for (int y = 0; y < currentScreenHeight; y += 4) {
        float ratio = (float)y / currentScreenHeight;
        unsigned char r = (unsigned char)(20 + 10 * ratio);
        unsigned char g = (unsigned char)(20 + 8 * ratio);
        unsigned char b = (unsigned char)(30 + 15 * ratio);
        Color gradColor = { r, g, b, 255 };
        DrawRectangle(0, y, currentScreenWidth, 4, gradColor);
    }

    // 动态网格背景
    int gridSize = (int)(50 * scaleX);
    if (gridSize < 40) gridSize = 40;
    float gridOffset = fmodf(time * 15.0f, (float)gridSize);
    for (int x = (int)(-gridOffset); x < currentScreenWidth; x += gridSize) {
        DrawRectangle(x, 0, 1, currentScreenHeight, Fade(MakeRGBA(60, 40, 100, 50), 0.3f));
    }
    for (int y = 0; y < currentScreenHeight; y += gridSize) {
        DrawRectangle(0, y, currentScreenWidth, 1, Fade(MakeRGBA(60, 40, 100, 50), 0.3f));
    }

    // 标题栏背景渐变
    DrawRectangle(0, 0, currentScreenWidth, (int)(140 * scaleY), Fade(MakeRGBA(25, 20, 45, 255), 0.95f));

    // 技术风格装饰角
    Color frameColor = MakeRGBA(120, 80, 180, 150);
    int cornerSize = (int)(60 * scaleX);
    if (cornerSize < 40) cornerSize = 40;
    DrawLine((int)(10 * scaleX), (int)(10 * scaleY), (int)(10 * scaleX) + cornerSize, (int)(10 * scaleY), frameColor);
    DrawLine((int)(10 * scaleX), (int)(10 * scaleY), (int)(10 * scaleX), (int)(10 * scaleY) + cornerSize, frameColor);
    DrawLine(currentScreenWidth - 10 - cornerSize, (int)(10 * scaleY), currentScreenWidth - 10, (int)(10 * scaleY), frameColor);
    DrawLine(currentScreenWidth - (int)(10 * scaleX), (int)(10 * scaleY), currentScreenWidth - (int)(10 * scaleX), (int)(10 * scaleY) + cornerSize, frameColor);

    // 发光分割线
    float linePulse = 0.6f + 0.4f * sinf(time * 2.5f);
    DrawRectangle(0, (int)(138 * scaleY), currentScreenWidth, 3, Fade(MakeRGBA(100, 150, 200, (int)(150 * linePulse)), 0.7f));

    // 标题发光效果
    const char* title = "Select Music";
    int titleSize = (int)(36 * scale);
    if (titleSize < 24) titleSize = 24;
    int titleWidth = MeasureText(title, titleSize);
    for (int i = 2; i > 0; i--) {
        DrawText(title, currentScreenWidth / 2 - titleWidth / 2, (int)(30 * scaleY), titleSize, Fade(MakeRGBA(150, 200, 255, 30), 0.15f * i));
    }
    DrawText(title, currentScreenWidth / 2 - titleWidth / 2, (int)(30 * scaleY), titleSize, MakeRGBA(180, 180, 220, 255));

    // 返回按钮
    int backBtnWidth = (int)(120 * scaleX);
    int backBtnHeight = (int)(35 * scaleY);
    if (backBtnHeight < 28) backBtnHeight = 28;
    bool backHovered = IsMouseInRect(mx, my, (int)(20 * scaleX), (int)(20 * scaleY), backBtnWidth, backBtnHeight);
    DrawButton("Back", (int)(20 * scaleX), (int)(20 * scaleY), backBtnWidth, backBtnHeight, Fade(MakeRGBA(60, 60, 80, 255), 0.8f), MakeRGBA(180, 80, 80, 255), backHovered);

    int safeIndex = selectedIndex;
    if (safeIndex < 0) safeIndex = 0;
    if ((int)musicList.size() > 0 && safeIndex >= (int)musicList.size()) safeIndex = (int)musicList.size() - 1;

    // 计算可见列表范围
    int LIST_START_Y = (int)(160 * scaleY);
    int ITEM_HEIGHT = (int)(50 * scaleY);
    if (ITEM_HEIGHT < 40) ITEM_HEIGHT = 40;
    const int VISIBLE_ITEMS = 7;
    int LIST_WIDTH = currentScreenWidth - (int)(100 * scaleX);
    if (LIST_WIDTH < 400) LIST_WIDTH = 400;

    // 绘制滚动列表容器
    int listX = (int)(40 * scaleX);
    int listY = LIST_START_Y - 10;
    int listW = LIST_WIDTH;
    int listH = ITEM_HEIGHT * VISIBLE_ITEMS + 20;
    
    Rectangle listRec = { (float)listX, (float)listY, (float)listW, (float)listH };
    DrawRectangleRounded(listRec, 0.15f, 8, Fade(MakeRGBA(25, 25, 40, 255), 0.95f));
    DrawRectangleRoundedLines(listRec, 0.15f, 8, 2.0f, Fade(MakeRGBA(100, 80, 160, 200), 0.8f));
    DrawRectangle((int)(40 * scaleX), LIST_START_Y - 10, LIST_WIDTH, 2, Fade(MakeRGBA(150, 120, 200, 150), 0.5f));

    // 根据滚动偏移计算可见范围
    int startIdx = scrollOffset;
    int endIdx = startIdx + VISIBLE_ITEMS;
    if (endIdx > (int)musicList.size()) endIdx = (int)musicList.size();

    if (hoveredItem != nullptr) {
        *hoveredItem = -1;
    }

    for (int i = startIdx; i < endIdx; i++) {
        int displayIndex = i - startIdx;
        int y = LIST_START_Y + displayIndex * ITEM_HEIGHT;
        const char* name = musicList[i].name.c_str();
        bool isHovered = IsMouseInRect(mx, my, (int)(45 * scaleX), y, LIST_WIDTH - (int)(60 * scaleX), ITEM_HEIGHT - 4);

        if (isHovered && mouseMoved && hoveredItem != nullptr) {
            *hoveredItem = i;
        }

        Color bgColor = MakeRGBA(40, 40, 55, 255);
        Color textColor = MakeRGBA(140, 140, 160, 255);
        Color accentColor = MakeRGBA(100, 80, 140, 255);

        if (i == safeIndex) {
            bgColor = Fade(MakeRGBA(70, 50, 100, 255), 0.6f);
            textColor = MakeRGBA(200, 180, 255, 255);
            accentColor = MakeRGBA(150, 120, 200, 255);
        } else if (isHovered) {
            bgColor = Fade(MakeRGBA(80, 60, 120, 255), 0.3f);
            textColor = MakeRGBA(180, 160, 220, 255);
            accentColor = MakeRGBA(150, 120, 200, 255);
        }

        DrawRectangle((int)(45 * scaleX), y, LIST_WIDTH - (int)(60 * scaleX), ITEM_HEIGHT - 4, bgColor);

        // 选中指示器发光
        if (i == safeIndex) {
            DrawRectangle((int)(45 * scaleX) - 5, y + 2, 3, ITEM_HEIGHT - 8, Fade(MakeRGBA(150, 200, 255, 100), 0.8f));
        }

        const char* prefix = (i == safeIndex) ? ">" : " ";
        int textSize = (int)(20 * scale);
        if (textSize < 16) textSize = 16;
        DrawText(TextFormat("%s %s", prefix, name), (int)(55 * scaleX), y + 12, textSize, textColor);
    }

    // 绘制滚动条
    int totalItems = (int)musicList.size();
    if (totalItems > VISIBLE_ITEMS) {
        int scrollbarHeight = ITEM_HEIGHT * VISIBLE_ITEMS;
        int scrollbarWidth = (int)(12 * scaleX);
        if (scrollbarWidth < 10) scrollbarWidth = 10;
        int scrollbarX = (int)(40 * scaleX) + LIST_WIDTH - scrollbarWidth - 5;
        int scrollbarY = LIST_START_Y - 10;

        DrawRectangle(scrollbarX, scrollbarY, scrollbarWidth, scrollbarHeight, Fade(MakeRGBA(40, 40, 60, 255), 0.9f));
        DrawRectangleLines(scrollbarX, scrollbarY, scrollbarWidth, scrollbarHeight, Fade(MakeRGBA(80, 60, 120, 150), 0.5f));

        float thumbRatio = (float)VISIBLE_ITEMS / totalItems;
        int thumbHeight = (int)(scrollbarHeight * thumbRatio);
        if (thumbHeight < 30) thumbHeight = 30;

        float scrollRatio = (float)scrollOffset / (totalItems - VISIBLE_ITEMS);
        if (totalItems <= VISIBLE_ITEMS) scrollRatio = 0;
        int thumbY = scrollbarY + (int)((scrollbarHeight - thumbHeight) * scrollRatio);

        DrawRectangle(scrollbarX + 2, thumbY, scrollbarWidth - 4, thumbHeight, Fade(MakeRGBA(120, 100, 180, 220), 0.9f));
        DrawRectangleLines(scrollbarX + 2, thumbY, scrollbarWidth - 4, thumbHeight, Fade(MakeRGBA(180, 150, 220, 150), 0.6f));

        // 上下按钮
        int btnHeight = (int)(22 * scaleY);
        if (btnHeight < 18) btnHeight = 18;
        bool upHovered = IsMouseInRect(mx, my, scrollbarX, scrollbarY - 25, scrollbarWidth, btnHeight);
        bool downHovered = IsMouseInRect(mx, my, scrollbarX, scrollbarY + scrollbarHeight + 3, scrollbarWidth, btnHeight);
        DrawButton("^", scrollbarX, scrollbarY - 25, scrollbarWidth, btnHeight, Fade(MakeRGBA(50, 50, 70, 255), 0.8f), MakeRGBA(150, 120, 200, 255), upHovered);
        DrawButton("v", scrollbarX, scrollbarY + scrollbarHeight + 3, scrollbarWidth, btnHeight, Fade(MakeRGBA(50, 50, 70, 255), 0.8f), MakeRGBA(150, 120, 200, 255), downHovered);
    }

    // 位置信息面板
    char posText[64];
    snprintf(posText, sizeof(posText), "%d / %d", safeIndex + 1, totalItems);
    int posWidth = MeasureText(posText, 20);
    int posPanelX = currentScreenWidth - posWidth - 50;
    int posPanelY = LIST_START_Y - 35;
    DrawRectangle(posPanelX - 5, posPanelY - 3, posWidth + 20, 28, Fade(MakeRGBA(30, 30, 50, 255), 0.9f));
    DrawRectangleLines(posPanelX - 5, posPanelY - 3, posWidth + 20, 28, Fade(MakeRGBA(100, 80, 160, 180), 0.6f));
    DrawText(posText, posPanelX + 5, posPanelY + 2, 20, MakeRGBA(200, 180, 255, 255));

    // 漂浮粒子效果
    for (int i = 0; i < 8; i++) {
        float px = fmodf((float)i * 123.7f + time * 25.0f, (float)currentScreenWidth);
        float py = fmodf((float)i * 67.3f + time * 15.0f, (float)currentScreenHeight);
        int particleSize = (int)(2 * scale);
        if (particleSize < 2) particleSize = 2;
        DrawRectangle((int)px, (int)py, particleSize, particleSize, Fade(MakeRGBA(150, 120, 200, 60), 0.5f));
    }

    // 底部提示文字
    int hintSize = (int)(16 * scale);
    if (hintSize < 12) hintSize = 12;
    int hintWidth = MeasureText("Enter or Click to Confirm  |  Mouse Wheel to Scroll  |  Backspace to Return", hintSize);
    DrawText("Enter or Click to Confirm  |  Mouse Wheel to Scroll  |  Backspace to Return",
             currentScreenWidth / 2 - hintWidth / 2, currentScreenHeight - 40, hintSize, Fade(MakeRGBA(120, 120, 160, 180), 0.7f));
}

// ============================================================================
// DrawDifficultySelect - 绘制难度选择界面
// ============================================================================
// 【功能说明】
//   绘制难度选择界面（Easy/Normal/Hard）
// 【参数说明】
//   - selectedIndex: 当前选中的音乐索引
//   - diff: 当前选中的难度
//   - hoveredOption: 悬停选项索引（输出参数）
//   - isGameMode: 是否游戏模式（否则为编辑器模式）
// 【实现内容】
//   - 标题栏"Select Difficulty"
//   - 当前音乐名称显示
//   - 三个难度选项：Easy、Normal、Hard
//   - 各难度使用不同颜色区分
//   - 最佳成绩显示（仅游戏模式）
//   - 开始按钮
// 【颜色映射】
//   - Easy：绿色
//   - Normal：蓝色
//   - Hard：红色
// ============================================================================
void DrawDifficultySelect(int selectedIndex, Difficulty diff, int hoveredOption, bool isGameMode) {
    int currentScreenWidth = GetScreenWidth();
    int currentScreenHeight = GetScreenHeight();
    int mx = GetMouseX();
    int my = GetMouseY();

    // 基于屏幕尺寸计算缩放因子（基准800x600）
    float scaleX = (float)currentScreenWidth / 800.0f;
    float scaleY = (float)currentScreenHeight / 600.0f;
    float scale = (scaleX + scaleY) / 2.0f;
    if (scale < 0.5f) scale = 0.5f;
    if (scale > 2.0f) scale = 2.0f;

    float time = (float)GetTime();

    // 深色渐变背景
    for (int y = 0; y < currentScreenHeight; y += 4) {
        float ratio = (float)y / currentScreenHeight;
        unsigned char r = (unsigned char)(22 + 12 * ratio);
        unsigned char g = (unsigned char)(18 + 10 * ratio);
        unsigned char b = (unsigned char)(32 + 18 * ratio);
        Color gradColor = { r, g, b, 255 };
        DrawRectangle(0, y, currentScreenWidth, 4, gradColor);
    }

    // 动态网格背景
    int gridSize = (int)(50 * scaleX);
    if (gridSize < 40) gridSize = 40;
    float gridOffset = fmodf(time * 18.0f, (float)gridSize);
    for (int x = (int)(-gridOffset); x < currentScreenWidth; x += gridSize) {
        DrawRectangle(x, 0, 1, currentScreenHeight, Fade(MakeRGBA(70, 50, 110, 50), 0.3f));
    }
    for (int y = 0; y < currentScreenHeight; y += gridSize) {
        DrawRectangle(0, y, currentScreenWidth, 1, Fade(MakeRGBA(70, 50, 110, 50), 0.3f));
    }

    // 标题栏背景
    DrawRectangle(0, 0, currentScreenWidth, (int)(170 * scaleY), Fade(MakeRGBA(25, 20, 45, 255), 0.95f));

    // 技术风格装饰角
    Color frameColor = MakeRGBA(120, 80, 180, 150);
    int cornerSize = (int)(60 * scaleX);
    if (cornerSize < 40) cornerSize = 40;
    DrawLine((int)(10 * scaleX), (int)(10 * scaleY), (int)(10 * scaleX) + cornerSize, (int)(10 * scaleY), frameColor);
    DrawLine((int)(10 * scaleX), (int)(10 * scaleY), (int)(10 * scaleX), (int)(10 * scaleY) + cornerSize, frameColor);
    DrawLine(currentScreenWidth - 10 - cornerSize, (int)(10 * scaleY), currentScreenWidth - 10, (int)(10 * scaleY), frameColor);
    DrawLine(currentScreenWidth - (int)(10 * scaleX), (int)(10 * scaleY), currentScreenWidth - (int)(10 * scaleX), (int)(10 * scaleY) + cornerSize, frameColor);

    // 发光分割线
    float linePulse = 0.6f + 0.4f * sinf(time * 2.5f);
    DrawRectangle(0, (int)(168 * scaleY), currentScreenWidth, 3, Fade(MakeRGBA(100, 150, 200, (int)(150 * linePulse)), 0.7f));

    // 返回按钮
    int backBtnWidth = (int)(120 * scaleX);
    int backBtnHeight = (int)(35 * scaleY);
    if (backBtnHeight < 28) backBtnHeight = 28;
    bool backHovered = IsMouseInRect(mx, my, (int)(20 * scaleX), (int)(20 * scaleY), backBtnWidth, backBtnHeight);
    DrawButton("Back", (int)(20 * scaleX), (int)(20 * scaleY), backBtnWidth, backBtnHeight, Fade(MakeRGBA(60, 60, 80, 255), 0.8f), MakeRGBA(180, 80, 80, 255), backHovered);

    int safeIndex = selectedIndex;
    if (safeIndex < 0) safeIndex = 0;
    if ((int)musicList.size() > 0 && safeIndex >= (int)musicList.size()) safeIndex = (int)musicList.size() - 1;

    const char* musicName = musicList[safeIndex].name.c_str();

    // 标题发光效果
    const char* title = "Select Difficulty";
    int titleSize = (int)(36 * scale);
    if (titleSize < 24) titleSize = 24;
    int titleWidth = MeasureText(title, titleSize);
    for (int i = 2; i > 0; i--) {
        DrawText(title, currentScreenWidth / 2 - titleWidth / 2, (int)(30 * scaleY), titleSize, Fade(MakeRGBA(150, 200, 255, 30), 0.15f * i));
    }
    DrawText(title, currentScreenWidth / 2 - titleWidth / 2, (int)(30 * scaleY), titleSize, MakeRGBA(180, 180, 220, 255));

    // 音乐名称框
    int musicBoxWidth = currentScreenWidth - (int)(200 * scaleX);
    if (musicBoxWidth < 300) musicBoxWidth = 300;
    int musicBoxHeight = (int)(40 * scaleY);
    if (musicBoxHeight < 32) musicBoxHeight = 32;
    int textSize = (int)(20 * scale);
    if (textSize < 14) textSize = 14;
    DrawRectangle(currentScreenWidth / 2 - musicBoxWidth / 2, (int)(75 * scaleY), musicBoxWidth, musicBoxHeight, Fade(MakeRGBA(50, 40, 70, 255), 0.6f));
    DrawRectangleLines(currentScreenWidth / 2 - musicBoxWidth / 2, (int)(75 * scaleY), musicBoxWidth, musicBoxHeight, MakeRGBA(100, 80, 160, 200));
    DrawRectangle(currentScreenWidth / 2 - musicBoxWidth / 2, (int)(75 * scaleY), musicBoxWidth, 2, Fade(MakeRGBA(150, 120, 200, 150), 0.5f));
    DrawText(TextFormat("Music: %s", musicName), currentScreenWidth / 2 - MeasureText(TextFormat("Music: %s", musicName), textSize) / 2, (int)(85 * scaleY), textSize, MakeRGBA(180, 180, 220, 255));

    // 难度选项布局
    int OPTION_START_Y = (int)(200 * scaleY);
    int OPTION_HEIGHT = (int)(70 * scaleY);
    if (OPTION_HEIGHT < 56) OPTION_HEIGHT = 56;
    int OPTION_WIDTH = (int)(300 * scaleX);
    if (OPTION_WIDTH < 240) OPTION_WIDTH = 240;

    // 漂浮粒子
    for (int i = 0; i < 6; i++) {
        float px = fmodf((float)i * 137.5f + time * 25.0f, (float)currentScreenWidth);
        float py = fmodf((float)i * 73.7f + time * 15.0f, (float)currentScreenHeight);
        int particleSize = (int)(2 * scale);
        if (particleSize < 2) particleSize = 2;
        Color particleColor = (i % 2 == 0) ? MakeRGBA(150, 100, 200, 80) : MakeRGBA(100, 200, 150, 60);
        DrawRectangle((int)px, (int)py, particleSize, particleSize, particleColor);
    }

    // Easy选项
    int easyY = OPTION_START_Y;
    bool easyHovered = IsMouseInRect(mx, my, currentScreenWidth / 2 - OPTION_WIDTH / 2, easyY, OPTION_WIDTH, OPTION_HEIGHT);
    if (easyHovered) hoveredOption = 0;
    Color easyBg = (diff == Difficulty::EASY) ? Fade(MakeRGBA(30, 80, 30, 255), 0.5f) : (easyHovered ? Fade(MakeRGBA(40, 100, 40, 255), 0.4f) : Fade(MakeRGBA(40, 40, 55, 255), 0.85f));
    DrawRectangle(currentScreenWidth / 2 - OPTION_WIDTH / 2, easyY, OPTION_WIDTH, OPTION_HEIGHT, easyBg);
    DrawRectangleLines(currentScreenWidth / 2 - OPTION_WIDTH / 2, easyY, OPTION_WIDTH, OPTION_HEIGHT, MakeRGBA(50, 150, 50, 220));
    DrawRectangle(currentScreenWidth / 2 - OPTION_WIDTH / 2, easyY, OPTION_WIDTH, 2, Fade(MakeRGBA(80, 200, 80, 150), 0.5f));
    const char* easyText = (diff == Difficulty::EASY) ? "> 1 - Easy" : "  1 - Easy";
    int optTextSize = (int)(26 * scale);
    if (optTextSize < 20) optTextSize = 20;
    DrawText(easyText, currentScreenWidth / 2 - MeasureText(easyText, optTextSize) / 2, easyY + (int)(22 * scaleY), optTextSize, MakeRGBA(80, 200, 80, 255));

    // Normal选项
    int normalY = OPTION_START_Y + OPTION_HEIGHT + (int)(15 * scaleY);
    bool normalHovered = IsMouseInRect(mx, my, currentScreenWidth / 2 - OPTION_WIDTH / 2, normalY, OPTION_WIDTH, OPTION_HEIGHT);
    if (normalHovered) hoveredOption = 1;
    Color normalBg = (diff == Difficulty::NORMAL) ? Fade(MakeRGBA(50, 50, 120, 255), 0.5f) : (normalHovered ? Fade(MakeRGBA(60, 60, 140, 255), 0.4f) : Fade(MakeRGBA(40, 40, 55, 255), 0.85f));
    DrawRectangle(currentScreenWidth / 2 - OPTION_WIDTH / 2, normalY, OPTION_WIDTH, OPTION_HEIGHT, normalBg);
    DrawRectangleLines(currentScreenWidth / 2 - OPTION_WIDTH / 2, normalY, OPTION_WIDTH, OPTION_HEIGHT, MakeRGBA(80, 80, 200, 220));
    DrawRectangle(currentScreenWidth / 2 - OPTION_WIDTH / 2, normalY, OPTION_WIDTH, 2, Fade(MakeRGBA(120, 120, 255, 150), 0.5f));
    const char* normalText = (diff == Difficulty::NORMAL) ? "> 2 - Normal" : "  2 - Normal";
    DrawText(normalText, currentScreenWidth / 2 - MeasureText(normalText, optTextSize) / 2, normalY + (int)(22 * scaleY), optTextSize, MakeRGBA(100, 100, 220, 255));

    // Hard选项
    int hardY = OPTION_START_Y + (OPTION_HEIGHT + (int)(15 * scaleY)) * 2;
    bool hardHovered = IsMouseInRect(mx, my, currentScreenWidth / 2 - OPTION_WIDTH / 2, hardY, OPTION_WIDTH, OPTION_HEIGHT);
    if (hardHovered) hoveredOption = 2;
    Color hardBg = (diff == Difficulty::HARD) ? Fade(MakeRGBA(120, 30, 30, 255), 0.5f) : (hardHovered ? Fade(MakeRGBA(150, 40, 40, 255), 0.4f) : Fade(MakeRGBA(40, 40, 55, 255), 0.85f));
    DrawRectangle(currentScreenWidth / 2 - OPTION_WIDTH / 2, hardY, OPTION_WIDTH, OPTION_HEIGHT, hardBg);
    DrawRectangleLines(currentScreenWidth / 2 - OPTION_WIDTH / 2, hardY, OPTION_WIDTH, OPTION_HEIGHT, MakeRGBA(200, 60, 60, 220));
    DrawRectangle(currentScreenWidth / 2 - OPTION_WIDTH / 2, hardY, OPTION_WIDTH, 2, Fade(MakeRGBA(255, 100, 100, 150), 0.5f));
    const char* hardText = (diff == Difficulty::HARD) ? "> 3 - Hard" : "  3 - Hard";
    DrawText(hardText, currentScreenWidth / 2 - MeasureText(hardText, optTextSize) / 2, hardY + (int)(22 * scaleY), optTextSize, MakeRGBA(220, 80, 80, 255));

    // 显示最佳成绩（仅游戏模式）
    if (isGameMode) {
        int recordY = hardY + OPTION_HEIGHT + (int)(10 * scaleY);
        BestRecord best = LoadBestRecord(safeIndex, diff);
        if (best.score > 0) {
            Color gradeColor;
            switch (best.grade) {
                case 'S': gradeColor = GOLD; break;
                case 'A': gradeColor = MakeRGBA(80, 200, 80, 255); break;
                case 'B': gradeColor = MakeRGBA(100, 100, 220, 255); break;
                case 'C': gradeColor = PURPLE; break;
                default: gradeColor = MakeRGBA(150, 150, 150, 255); break;
            }
            DrawText(TextFormat("Best: %c  Score: %d  Combo: %d  Acc: %.1f%%", best.grade, best.score, best.maxCombo, best.accuracy),
                     currentScreenWidth / 2 - (int)(200 * scaleX), recordY, (int)(14 * scale), gradeColor);
        } else {
            DrawText("No record yet", currentScreenWidth / 2 - (int)(50 * scaleX), recordY, (int)(14 * scale), Fade(GRAY, 0.5f));
        }
    }

    // 开始按钮
    int startBtnWidth = (int)(200 * scaleX);
    if (startBtnWidth < 160) startBtnWidth = 160;
    int startBtnHeight = (int)(50 * scaleY);
    if (startBtnHeight < 40) startBtnHeight = 40;
    int startBtnY = hardY + OPTION_HEIGHT + (int)(30 * scaleY);
    bool startHovered = IsMouseInRect(mx, my, currentScreenWidth / 2 - startBtnWidth / 2, startBtnY, startBtnWidth, startBtnHeight);
    if (startHovered) hoveredOption = 3;
    const char* startBtnText = isGameMode ? "Start Game" : "Start Editing";
    DrawButton(startBtnText, currentScreenWidth / 2 - startBtnWidth / 2, startBtnY, startBtnWidth, startBtnHeight,
               Fade(MakeRGBA(80, 50, 120, 255), 0.8f), MakeRGBA(60, 150, 255, 255), startHovered);
    if (startHovered) {
        DrawRectangleLines(currentScreenWidth / 2 - startBtnWidth / 2, startBtnY, startBtnWidth, startBtnHeight, Fade(MakeRGBA(100, 200, 255, 150), 0.6f));
        DrawRectangle(currentScreenWidth / 2 - startBtnWidth / 2 - 2, startBtnY - 2, startBtnWidth + 4, 2, Fade(MakeRGBA(100, 200, 255, 150), 0.4f));
    }

    // 装饰角
    Color cornerColor = MakeRGBA(100, 80, 160, 120);
    int decoCornerSize = (int)(50 * scaleX);
    if (decoCornerSize < 30) decoCornerSize = 30;
    DrawLine((int)(20 * scaleX), currentScreenHeight - (int)(20 * scaleY), (int)(20 * scaleX) + decoCornerSize, currentScreenHeight - (int)(20 * scaleY), cornerColor);
    DrawLine((int)(20 * scaleX), currentScreenHeight - (int)(20 * scaleY), (int)(20 * scaleX), currentScreenHeight - (int)(20 * scaleY) - decoCornerSize, cornerColor);
    DrawLine(currentScreenWidth - 20 - decoCornerSize, currentScreenHeight - (int)(20 * scaleY), currentScreenWidth - 20, currentScreenHeight - (int)(20 * scaleY), cornerColor);
    DrawLine(currentScreenWidth - (int)(20 * scaleX), currentScreenHeight - (int)(20 * scaleY), currentScreenWidth - (int)(20 * scaleX), currentScreenHeight - (int)(20 * scaleY) - decoCornerSize, cornerColor);

    int hintSize = (int)(16 * scale);
    if (hintSize < 12) hintSize = 12;
    DrawText("1/2/3 or Click to Select  |  Enter to Start  |  Backspace to Return",
             currentScreenWidth / 2 - (int)(230 * scaleX), currentScreenHeight - (int)(40 * scaleY), hintSize, Fade(MakeRGBA(120, 120, 160, 180), 0.7f));
}

// ============================================================================
// DrawSettings：绘制设置界面（完整实现）
// ============================================================================
void DrawSettings(GameState& gameState) {
    int currentScreenWidth = GetScreenWidth();
    int currentScreenHeight = GetScreenHeight();
    int mx = GetMouseX();
    int my = GetMouseY();

    float scaleX = (float)currentScreenWidth / 800.0f;
    float scaleY = (float)currentScreenHeight / 600.0f;
    float scale = (scaleX + scaleY) / 2.0f;
    if (scale < 0.5f) scale = 0.5f;
    if (scale > 2.0f) scale = 2.0f;

    ClearBackground(MakeRGBA(20, 20, 35, 255));

    // 返回按钮（左上角）
    int backBtnW = (int)(100 * scaleX);
    if (backBtnW < 80) backBtnW = 80;
    int backBtnH = (int)(35 * scaleY);
    if (backBtnH < 28) backBtnH = 28;
    int backBtnX = (int)(20 * scaleX);
    int backBtnY = (int)(20 * scaleY);
    bool backHovered = IsMouseInRect(mx, my, backBtnX, backBtnY, backBtnW, backBtnH);
    DrawButton("Back", backBtnX, backBtnY, backBtnW, backBtnH, Fade(MakeRGBA(80, 50, 80, 255), 0.8f), MakeRGBA(150, 100, 180, 255), backHovered);

    // 标题居中
    const char* title = "Settings";
    int titleSize = (int)(36 * scale);
    if (titleSize < 24) titleSize = 24;
    int titleWidth = MeasureText(title, titleSize);
    DrawText(title, currentScreenWidth / 2 - titleWidth / 2, (int)(25 * scaleY), titleSize, PURPLE);

    // 设置框（居中）
    int boxW = (int)(500 * scaleX);
    if (boxW < 400) boxW = 400;
    int boxH = (int)(520 * scaleY);
    if (boxH < 420) boxH = 420;
    int boxX = currentScreenWidth / 2 - boxW / 2;
    int boxY = (int)(55 * scaleY);
    
    Rectangle settingsBox = { (float)boxX, (float)boxY, (float)boxW, (float)boxH };
    DrawRectangleRounded(settingsBox, 0.15f, 8, Fade(MakeRGBA(30, 30, 50, 255), 0.9f));
    DrawRectangleRoundedLines(settingsBox, 0.15f, 8, 2.0f, PURPLE);

    int labelSize = (int)(18 * scale);
    if (labelSize < 14) labelSize = 14;
    int contentPadding = (int)(40 * scaleX);
    int contentWidth = boxW - contentPadding * 2;

    // ===== 标签按钮 =====
    const char* tabNames[] = { "Audio", "Game", "Controls" };
    int tabCount = 3;
    int tabBtnW = (int)(100 * scaleX);
    if (tabBtnW < 80) tabBtnW = 80;
    int tabBtnH = (int)(35 * scaleY);
    if (tabBtnH < 28) tabBtnH = 28;
    int tabStartX = boxX + contentPadding;
    int tabY = boxY + (int)(15 * scaleY);

    for (int i = 0; i < tabCount; i++) {
        int tabX = tabStartX + i * (tabBtnW + (int)(10 * scaleX));
        bool tabHovered = IsMouseInRect(mx, my, tabX, tabY, tabBtnW, tabBtnH);
        bool isActive = (gameState.settingsPage == i);

        Color tabColor = isActive ? MakeRGBA(100, 150, 200, 255) :
                         (tabHovered ? Fade(MakeRGBA(60, 60, 90, 255), 0.9f) : Fade(MakeRGBA(40, 40, 60, 255), 0.8f));
        DrawButton(tabNames[i], tabX, tabY, tabBtnW, tabBtnH, tabColor, MakeRGBA(150, 200, 255, 255), tabHovered);

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && tabHovered) {
            gameState.settingsPage = i;
            gameState.waitingForKey = -1;
        }
    }

    int currentY = tabY + tabBtnH + (int)(25 * scaleY);
    int sliderW = contentWidth;
    int sliderH = (int)(25 * scaleY);
    if (sliderH < 20) sliderH = 20;

    if (gameState.settingsPage == 0) {
        // ===== 音频页面 =====
        DrawText("Music Volume", boxX + contentPadding, currentY, labelSize, WHITE);
        currentY += (int)(40 * scaleY);

        DrawRectangle(boxX + contentPadding, currentY, sliderW, sliderH, MakeRGBA(60, 60, 80, 255));
        int fillW = (int)(gameState.musicVolume * sliderW);
        if (fillW > 0) {
            DrawRectangle(boxX + contentPadding, currentY, fillW, sliderH, MakeRGBA(100, 200, 150, 255));
        }
        char musicVolText[16];
        snprintf(musicVolText, sizeof(musicVolText), "%.0f%%", gameState.musicVolume * 100);
        int volTextW = MeasureText(musicVolText, labelSize);
        DrawText(musicVolText, boxX + contentPadding + sliderW - volTextW - (int)(10 * scaleX), currentY + (sliderH - labelSize) / 2, labelSize, WHITE);

        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && mx >= boxX + contentPadding && mx <= boxX + contentPadding + sliderW && my >= currentY && my <= currentY + sliderH) {
            gameState.musicVolume = (float)(mx - boxX - contentPadding) / sliderW;
            if (gameState.musicVolume < 0) gameState.musicVolume = 0;
            if (gameState.musicVolume > 1) gameState.musicVolume = 1;
        }
        currentY += (int)(50 * scaleY);

        DrawText("Hit Sound Volume", boxX + contentPadding, currentY, labelSize, WHITE);
        currentY += (int)(40 * scaleY);

        DrawRectangle(boxX + contentPadding, currentY, sliderW, sliderH, MakeRGBA(60, 60, 80, 255));
        fillW = (int)(gameState.hitSoundVolume * sliderW);
        if (fillW > 0) {
            DrawRectangle(boxX + contentPadding, currentY, fillW, sliderH, MakeRGBA(255, 180, 100, 255));
        }
        char hitVolText[16];
        snprintf(hitVolText, sizeof(hitVolText), "%.0f%%", gameState.hitSoundVolume * 100);
        int hitVolTextW = MeasureText(hitVolText, labelSize);
        DrawText(hitVolText, boxX + contentPadding + sliderW - hitVolTextW - (int)(10 * scaleX), currentY + (sliderH - labelSize) / 2, labelSize, WHITE);

        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && mx >= boxX + contentPadding && mx <= boxX + contentPadding + sliderW && my >= currentY && my <= currentY + sliderH) {
            gameState.hitSoundVolume = (float)(mx - boxX - contentPadding) / sliderW;
            if (gameState.hitSoundVolume < 0) gameState.hitSoundVolume = 0;
            if (gameState.hitSoundVolume > 1) gameState.hitSoundVolume = 1;
        }
        currentY += (int)(50 * scaleY);

        DrawText("BGM Volume", boxX + contentPadding, currentY, labelSize, WHITE);
        currentY += (int)(40 * scaleY);

        DrawRectangle(boxX + contentPadding, currentY, sliderW, sliderH, MakeRGBA(60, 60, 80, 255));
        fillW = (int)(gameState.bgMusicVolume * sliderW);
        if (fillW > 0) {
            DrawRectangle(boxX + contentPadding, currentY, fillW, sliderH, MakeRGBA(200, 100, 200, 255));
        }
        char bgVolText[16];
        snprintf(bgVolText, sizeof(bgVolText), "%.0f%%", gameState.bgMusicVolume * 100);
        int bgVolTextW = MeasureText(bgVolText, labelSize);
        DrawText(bgVolText, boxX + contentPadding + sliderW - bgVolTextW - (int)(10 * scaleX), currentY + (sliderH - labelSize) / 2, labelSize, WHITE);

        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && mx >= boxX + contentPadding && mx <= boxX + contentPadding + sliderW && my >= currentY && my <= currentY + sliderH) {
            gameState.bgMusicVolume = (float)(mx - boxX - contentPadding) / sliderW;
            if (gameState.bgMusicVolume < 0) gameState.bgMusicVolume = 0;
            if (gameState.bgMusicVolume > 1) gameState.bgMusicVolume = 1;
        }
        currentY += (int)(50 * scaleY);

        DrawText("Audio Offset", boxX + contentPadding, currentY, labelSize, WHITE);
        currentY += (int)(40 * scaleY);

        DrawRectangle(boxX + contentPadding, currentY, sliderW, sliderH, MakeRGBA(60, 60, 80, 255));
        int offsetCenterX = boxX + contentPadding + sliderW / 2;
        DrawRectangle(offsetCenterX - 1, currentY, 2, sliderH, MakeRGBA(200, 200, 200, 200));
        if (gameState.audioOffset >= 0) {
            int positiveWidth = (int)(gameState.audioOffset / 500.0f * sliderW / 2);
            if (positiveWidth > 0) {
                DrawRectangle(offsetCenterX, currentY, positiveWidth, sliderH, MakeRGBA(100, 200, 255, 255));
            }
        } else {
            int negativeWidth = (int)(-gameState.audioOffset / 500.0f * sliderW / 2);
            if (negativeWidth > 0) {
                DrawRectangle(offsetCenterX - negativeWidth, currentY, negativeWidth, sliderH, MakeRGBA(255, 150, 100, 255));
            }
        }
        char offsetText[32];
        if (gameState.audioOffset >= 0) {
            snprintf(offsetText, sizeof(offsetText), "+%.0f ms", gameState.audioOffset);
        } else {
            snprintf(offsetText, sizeof(offsetText), "%.0f ms", gameState.audioOffset);
        }
        int offsetTextW = MeasureText(offsetText, labelSize);
        DrawText(offsetText, boxX + contentPadding + sliderW - offsetTextW - (int)(10 * scaleX), currentY + (sliderH - labelSize) / 2, labelSize, WHITE);

        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && mx >= boxX + contentPadding && mx <= boxX + contentPadding + sliderW && my >= currentY && my <= currentY + sliderH) {
            float ratio = (float)(mx - boxX - contentPadding) / sliderW;
            gameState.audioOffset = -500.0f + ratio * 1000.0f;
            if (gameState.audioOffset < -500.0f) gameState.audioOffset = -500.0f;
            if (gameState.audioOffset > 500.0f) gameState.audioOffset = 500.0f;
        }
        currentY += (int)(30 * scaleY);

        int helpSize = (int)(14 * scale);
        if (helpSize < 11) helpSize = 11;
        DrawText("Adjust if notes appear early or late compared to music", boxX + contentPadding, currentY, helpSize, Fade(GRAY, 0.6f));
    }
    else if (gameState.settingsPage == 1) {
        // ===== 游戏页面 =====
        DrawText("Note Size", boxX + contentPadding, currentY, labelSize, WHITE);
        currentY += (int)(40 * scaleY);

        DrawRectangle(boxX + contentPadding, currentY, sliderW, sliderH, MakeRGBA(60, 60, 80, 255));
        int fillW = (int)((gameState.noteSize - 0.5f) / 1.5f * sliderW);
        if (fillW < 0) fillW = 0;
        if (fillW > 0) {
            DrawRectangle(boxX + contentPadding, currentY, fillW, sliderH, MakeRGBA(200, 100, 200, 255));
        }
        char noteSizeText[16];
        snprintf(noteSizeText, sizeof(noteSizeText), "%.0f%%", gameState.noteSize * 100);
        int noteSizeTextW = MeasureText(noteSizeText, labelSize);
        DrawText(noteSizeText, boxX + contentPadding + sliderW - noteSizeTextW - (int)(10 * scaleX), currentY + (sliderH - labelSize) / 2, labelSize, WHITE);

        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && mx >= boxX + contentPadding && mx <= boxX + contentPadding + sliderW && my >= currentY && my <= currentY + sliderH) {
            float ratio = (float)(mx - boxX - contentPadding) / sliderW;
            gameState.noteSize = 0.5f + ratio * 1.5f;
            if (gameState.noteSize < 0.5f) gameState.noteSize = 0.5f;
            if (gameState.noteSize > 2.0f) gameState.noteSize = 2.0f;
        }
        currentY += (int)(50 * scaleY);

        DrawText("Note Speed", boxX + contentPadding, currentY, labelSize, WHITE);
        currentY += (int)(40 * scaleY);

        DrawRectangle(boxX + contentPadding, currentY, sliderW, sliderH, MakeRGBA(60, 60, 80, 255));
        fillW = (int)((gameState.noteSpeed - 0.5f) / 1.5f * sliderW);
        if (fillW < 0) fillW = 0;
        if (fillW > 0) {
            DrawRectangle(boxX + contentPadding, currentY, fillW, sliderH, MakeRGBA(100, 180, 255, 255));
        }
        char noteSpeedText[16];
        snprintf(noteSpeedText, sizeof(noteSpeedText), "%.0f%%", gameState.noteSpeed * 100);
        int noteSpeedTextW = MeasureText(noteSpeedText, labelSize);
        DrawText(noteSpeedText, boxX + contentPadding + sliderW - noteSpeedTextW - (int)(10 * scaleX), currentY + (sliderH - labelSize) / 2, labelSize, WHITE);

        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && mx >= boxX + contentPadding && mx <= boxX + contentPadding + sliderW && my >= currentY && my <= currentY + sliderH) {
            float ratio = (float)(mx - boxX - contentPadding) / sliderW;
            gameState.noteSpeed = 0.5f + ratio * 1.5f;
            if (gameState.noteSpeed < 0.5f) gameState.noteSpeed = 0.5f;
            if (gameState.noteSpeed > 2.0f) gameState.noteSpeed = 2.0f;
        }
    }
    else if (gameState.settingsPage == 2) {
        // ===== 按键设置页面 =====
        DrawText("Key Bindings", boxX + contentPadding, currentY, labelSize, WHITE);
        currentY += (int)(40 * scaleY);

        const char* laneNames[4] = { "Lane 1:", "Lane 2:", "Lane 3:", "Lane 4:" };
        int keyBtnW = (int)(140 * scaleX);
        if (keyBtnW < 112) keyBtnW = 112;
        int keyBtnH = (int)(40 * scaleY);
        if (keyBtnH < 32) keyBtnH = 32;

        // 检查按键按下
        if (gameState.waitingForKey >= 0 && gameState.waitingForKey < 4) {
            int keyPressed = -1;
            if (IsKeyPressed(KEY_A)) keyPressed = KEY_A;
            else if (IsKeyPressed(KEY_B)) keyPressed = KEY_B;
            else if (IsKeyPressed(KEY_C)) keyPressed = KEY_C;
            else if (IsKeyPressed(KEY_D)) keyPressed = KEY_D;
            else if (IsKeyPressed(KEY_E)) keyPressed = KEY_E;
            else if (IsKeyPressed(KEY_F)) keyPressed = KEY_F;
            else if (IsKeyPressed(KEY_G)) keyPressed = KEY_G;
            else if (IsKeyPressed(KEY_H)) keyPressed = KEY_H;
            else if (IsKeyPressed(KEY_I)) keyPressed = KEY_I;
            else if (IsKeyPressed(KEY_J)) keyPressed = KEY_J;
            else if (IsKeyPressed(KEY_K)) keyPressed = KEY_K;
            else if (IsKeyPressed(KEY_L)) keyPressed = KEY_L;
            else if (IsKeyPressed(KEY_M)) keyPressed = KEY_M;
            else if (IsKeyPressed(KEY_N)) keyPressed = KEY_N;
            else if (IsKeyPressed(KEY_O)) keyPressed = KEY_O;
            else if (IsKeyPressed(KEY_P)) keyPressed = KEY_P;
            else if (IsKeyPressed(KEY_Q)) keyPressed = KEY_Q;
            else if (IsKeyPressed(KEY_R)) keyPressed = KEY_R;
            else if (IsKeyPressed(KEY_S)) keyPressed = KEY_S;
            else if (IsKeyPressed(KEY_T)) keyPressed = KEY_T;
            else if (IsKeyPressed(KEY_U)) keyPressed = KEY_U;
            else if (IsKeyPressed(KEY_V)) keyPressed = KEY_V;
            else if (IsKeyPressed(KEY_W)) keyPressed = KEY_W;
            else if (IsKeyPressed(KEY_X)) keyPressed = KEY_X;
            else if (IsKeyPressed(KEY_Y)) keyPressed = KEY_Y;
            else if (IsKeyPressed(KEY_Z)) keyPressed = KEY_Z;
            else if (IsKeyPressed(KEY_ZERO)) keyPressed = KEY_ZERO;
            else if (IsKeyPressed(KEY_ONE)) keyPressed = KEY_ONE;
            else if (IsKeyPressed(KEY_TWO)) keyPressed = KEY_TWO;
            else if (IsKeyPressed(KEY_THREE)) keyPressed = KEY_THREE;
            else if (IsKeyPressed(KEY_FOUR)) keyPressed = KEY_FOUR;
            else if (IsKeyPressed(KEY_FIVE)) keyPressed = KEY_FIVE;
            else if (IsKeyPressed(KEY_SIX)) keyPressed = KEY_SIX;
            else if (IsKeyPressed(KEY_SEVEN)) keyPressed = KEY_SEVEN;
            else if (IsKeyPressed(KEY_EIGHT)) keyPressed = KEY_EIGHT;
            else if (IsKeyPressed(KEY_NINE)) keyPressed = KEY_NINE;
            else if (IsKeyPressed(KEY_SPACE)) keyPressed = KEY_SPACE;

            if (keyPressed != -1) {
                gameState.keyBindings[gameState.waitingForKey] = keyPressed;
                gameState.waitingForKey = -1;
            }
        }

        for (int i = 0; i < 4; i++) {
            int rowY = currentY + i * (keyBtnH + (int)(8 * scaleY));
            int labelW = (int)(100 * scaleX);
            DrawText(laneNames[i], boxX + contentPadding, rowY + (keyBtnH - labelSize) / 2, labelSize, MakeRGBA(180, 180, 200, 255));

            int keyBtnX = boxX + contentPadding + labelW + (int)(20 * scaleX);
            const char* keyName = GetKeyName(gameState.keyBindings[i]);
            bool keyHovered = IsMouseInRect(mx, my, keyBtnX, rowY, keyBtnW, keyBtnH);
            bool isWaiting = (gameState.waitingForKey == i);

            Color btnColor = isWaiting ? MakeRGBA(200, 150, 50, 255) :
                             (keyHovered ? Fade(MakeRGBA(100, 150, 200, 255), 0.9f) : Fade(MakeRGBA(50, 50, 80, 255), 0.8f));
            DrawButton(keyName, keyBtnX, rowY, keyBtnW, keyBtnH, btnColor, MakeRGBA(100, 150, 200, 255), keyHovered);

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && keyHovered) {
                gameState.waitingForKey = i;
            }
        }

        currentY += 4 * (keyBtnH + (int)(8 * scaleY)) + (int)(20 * scaleY);

        if (gameState.waitingForKey >= 0) {
            const char* instructText = "Press any key to assign...";
            int instructSize = (int)(18 * scale);
            if (instructSize < 14) instructSize = 14;
            DrawText(instructText, boxX + contentPadding, currentY, instructSize, MakeRGBA(255, 200, 100, 255));
        }
    }

    // 重置默认按钮
    int resetBtnW = (int)(160 * scaleX);
    if (resetBtnW < 128) resetBtnW = 128;
    int resetBtnH = (int)(45 * scaleY);
    if (resetBtnH < 36) resetBtnH = 36;
    int resetBtnX = boxX + contentPadding;
    int resetBtnY = boxY + boxH - resetBtnH - (int)(20 * scaleY);
    bool resetHovered = IsMouseInRect(mx, my, resetBtnX, resetBtnY, resetBtnW, resetBtnH);
    DrawButton("Reset Default", resetBtnX, resetBtnY, resetBtnW, resetBtnH, Fade(MakeRGBA(100, 50, 50, 255), 0.8f), MakeRGBA(200, 100, 100, 255), resetHovered);

    if (resetHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (gameState.settingsPage == 0) {
            gameState.musicVolume = 0.8f;
            gameState.hitSoundVolume = 0.8f;
        } else if (gameState.settingsPage == 1) {
            gameState.noteSize = 1.0f;
            gameState.noteSpeed = 1.0f;
        } else if (gameState.settingsPage == 2) {
            gameState.keyBindings[0] = KEY_D;
            gameState.keyBindings[1] = KEY_F;
            gameState.keyBindings[2] = KEY_J;
            gameState.keyBindings[3] = KEY_K;
            gameState.waitingForKey = -1;
        }
    }
}

// ============================================================================
// DrawEditorScreen：绘制谱面编辑器
// ============================================================================
void DrawEditorScreen(EditorState& state, Music& music, bool musicLoaded) {
    int currentScreenWidth = GetScreenWidth();
    int currentScreenHeight = GetScreenHeight();
    int currentLaneWidth = currentScreenWidth / LANE_COUNT;
    int currentJudgeLineY = currentScreenHeight - 100;

    // Calculate scale
    float scaleX = (float)currentScreenWidth / 800.0f;
    float scaleY = (float)currentScreenHeight / 600.0f;
    float scale = (scaleX + scaleY) / 2.0f;
    if (scale < 0.5f) scale = 0.5f;
    if (scale > 2.0f) scale = 2.0f;

    // Draw lane backgrounds
    Color laneColors[4] = { MakeRGBA(50, 20, 20, 255), MakeRGBA(20, 20, 60, 255), MakeRGBA(20, 40, 70, 255), MakeRGBA(40, 20, 50, 255) };
    for (int i = 0; i < LANE_COUNT; i++) {
        DrawRectangle(i * currentLaneWidth, 0, currentLaneWidth, currentScreenHeight, laneColors[i]);
        // Lane divider
        if (i > 0) {
            DrawLine(i * currentLaneWidth, 0, i * currentLaneWidth, currentScreenHeight, MakeRGBA(80, 80, 100, 80));
        }
    }

    int mx = GetMouseX();
    int my = GetMouseY();

    // Draw back button (top left corner)
    int backBtnX = (int)(20 * scaleX);
    int backBtnY = (int)(45 * scaleY);
    int backBtnW = (int)(100 * scaleX);
    int backBtnH = (int)(30 * scaleY);
    bool backHovered = (mx >= backBtnX && mx <= backBtnX + backBtnW && my >= backBtnY && my <= backBtnY + backBtnH);
    Color backBtnColor = backHovered ? MakeRGBA(80, 80, 120, 255) : MakeRGBA(50, 50, 80, 200);
    
    Rectangle backBtnRec = { (float)backBtnX, (float)backBtnY, (float)backBtnW, (float)backBtnH };
    DrawRectangleRounded(backBtnRec, 0.3f, 4, backBtnColor);
    DrawRectangleRoundedLines(backBtnRec, 0.3f, 4, 1.5f, MakeRGBA(150, 150, 200, 150));
    
    const char* backText = "Back";
    int backTextSize = (int)(18 * scale);
    if (backTextSize < 14) backTextSize = 14;
    int backTextW = MeasureText(backText, backTextSize);
    DrawText(backText, backBtnX + (backBtnW - backTextW) / 2, backBtnY + (backBtnH - backTextSize) / 2, backTextSize, WHITE);

    // Draw exit button (below back button)
    int exitBtnX = backBtnX;
    int exitBtnY = backBtnY + backBtnH + (int)(10 * scaleY);
    int exitBtnW = backBtnW;
    int exitBtnH = backBtnH;
    bool exitHovered = (mx >= exitBtnX && mx <= exitBtnX + exitBtnW && my >= exitBtnY && my <= exitBtnY + exitBtnH);
    Color exitBtnColor = exitHovered ? MakeRGBA(140, 60, 60, 255) : MakeRGBA(100, 40, 40, 200);
    
    Rectangle exitBtnRec = { (float)exitBtnX, (float)exitBtnY, (float)exitBtnW, (float)exitBtnH };
    DrawRectangleRounded(exitBtnRec, 0.3f, 4, exitBtnColor);
    DrawRectangleRoundedLines(exitBtnRec, 0.3f, 4, 1.5f, MakeRGBA(200, 100, 100, 150));
    
    const char* exitText = "Exit";
    int exitTextSize = (int)(18 * scale);
    if (exitTextSize < 14) exitTextSize = 14;
    int exitTextW = MeasureText(exitText, exitTextSize);
    DrawText(exitText, exitBtnX + (exitBtnW - exitTextW) / 2, exitBtnY + (exitBtnH - exitTextSize) / 2, exitTextSize, WHITE);

    // Draw help button (top right corner)
    int helpBtnW = (int)(80 * scaleX);
    int helpBtnH = (int)(30 * scaleY);
    int helpBtnX = currentScreenWidth - helpBtnW - (int)(20 * scaleX);
    int helpBtnY = (int)(45 * scaleY);
    bool helpHovered = (mx >= helpBtnX && mx <= helpBtnX + helpBtnW && my >= helpBtnY && my <= helpBtnY + helpBtnH);
    Color helpBtnColor = helpHovered ? MakeRGBA(60, 100, 60, 255) : MakeRGBA(40, 70, 40, 200);
    
    Rectangle helpBtnRec = { (float)helpBtnX, (float)helpBtnY, (float)helpBtnW, (float)helpBtnH };
    DrawRectangleRounded(helpBtnRec, 0.3f, 4, helpBtnColor);
    DrawRectangleRoundedLines(helpBtnRec, 0.3f, 4, 1.5f, MakeRGBA(150, 200, 150, 150));
    
    const char* helpText = "Help";
    int helpTextSize = (int)(18 * scale);
    if (helpTextSize < 14) helpTextSize = 14;
    int helpTextW = MeasureText(helpText, helpTextSize);
    DrawText(helpText, helpBtnX + (helpBtnW - helpTextW) / 2, helpBtnY + (helpBtnH - helpTextSize) / 2, helpTextSize, WHITE);

    // Draw progress bar at top
    int progressBarY = (int)(15 * scaleY);
    int progressBarHeight = (int)(12 * scaleY);
    int progressBarMargin = (int)(40 * scaleX);

    // Background
    DrawRectangle(progressBarMargin, progressBarY, currentScreenWidth - progressBarMargin * 2, progressBarHeight, MakeRGBA(40, 40, 60, 200));

    // Progress
    float progress = 0.0f;
    if (state.musicLength > 0) {
        progress = state.gameTime / state.musicLength;
    }
    int progressWidth = (int)((currentScreenWidth - progressBarMargin * 2) * progress);
    if (progressWidth > 0) {
        DrawRectangle(progressBarMargin, progressBarY, progressWidth, progressBarHeight, MakeRGBA(100, 150, 255, 200));
    }

    // Time markers
    char timeText[32];
    snprintf(timeText, sizeof(timeText), "%.1f", state.gameTime / 1000.0f);
    DrawText(timeText, progressBarMargin, progressBarY + progressBarHeight + 2, (int)(10 * scale), LIGHTGRAY);

    if (state.musicLength > 0) {
        snprintf(timeText, sizeof(timeText), "%.1f", state.musicLength / 1000.0f);
        int timeW = MeasureText(timeText, (int)(10 * scale));
        DrawText(timeText, currentScreenWidth - progressBarMargin - timeW, progressBarY + progressBarHeight + 2, (int)(10 * scale), LIGHTGRAY);
    }

    // Draw timeline grid
    int beatInterval = 500;
    float beatsShown = (float)currentScreenHeight / (state.noteSpeed * 60.0f / 1000.0f) * 1000.0f;
    int startBeat = (int)(state.gameTime / beatInterval) - 2;
    int endBeat = startBeat + (int)(beatsShown / beatInterval) + 2;

    for (int beat = startBeat; beat <= endBeat; beat++) {
        float beatTime = (float)(beat * beatInterval);
        float y = currentJudgeLineY - (state.gameTime - beatTime) * state.noteSpeed * 60.0f / 1000.0f;

        if (y < 0 || y > currentScreenHeight) continue;

        Color lineColor = (beat % 4 == 0) ? MakeRGBA(255, 200, 150, 80) : MakeRGBA(200, 200, 220, 40);
        int lineWidth = (beat % 4 == 0) ? 2 : 1;
        DrawLine(0, (int)y, currentScreenWidth, (int)y, lineColor);

        if (beat % 4 == 0) {
            char beatText[16];
            snprintf(beatText, sizeof(beatText), "%d", (beat / 4) + 1);
            int tw = MeasureText(beatText, 14);
            DrawText(beatText, 8, (int)y - 10, 14, MakeRGBA(200, 190, 220, 120));
        }
    }

    // Draw judgment line
    DrawLine(0, currentJudgeLineY, currentScreenWidth, currentJudgeLineY, WHITE);

    // Draw notes
    float currentY = currentJudgeLineY - state.gameTime * state.noteSpeed * 60.0f / 1000.0f;

    for (const auto& note : state.notes) {
        float noteY = currentY + (note.time / 1000.0f) * state.noteSpeed * 60.0f;

        if (noteY < -NOTE_SIZE || noteY > currentScreenHeight + NOTE_SIZE) continue;

        int x = note.lane * currentLaneWidth + currentLaneWidth / 2 - NOTE_SIZE / 2;
        int y = (int)noteY;

        // Selection highlight
        if (note.selected) {
            DrawRectangle(x - 4, y - 4, NOTE_SIZE + 8, NOTE_SIZE + 8, MakeRGBA(255, 255, 0, 100));
            DrawRectangleLines(x - 4, y - 4, NOTE_SIZE + 8, NOTE_SIZE + 8, GOLD);
        }

        // Note colors - distinct bright colors for each lane
        Color coreColor, rimColor, glowColor;
        switch (note.lane) {
            case 0:  // Lane 0 - Bright Red/Orange
                coreColor = MakeRGBA(255, 100, 60, 255);
                rimColor = MakeRGBA(255, 60, 30, 255);
                glowColor = MakeRGBA(255, 150, 100, 150);
                break;
            case 1:  // Lane 1 - Bright Cyan/Blue
                coreColor = MakeRGBA(60, 200, 255, 255);
                rimColor = MakeRGBA(30, 150, 255, 255);
                glowColor = MakeRGBA(100, 200, 255, 150);
                break;
            case 2:  // Lane 2 - Bright Green/Lime
                coreColor = MakeRGBA(80, 255, 150, 255);
                rimColor = MakeRGBA(40, 220, 100, 255);
                glowColor = MakeRGBA(120, 255, 180, 150);
                break;
            case 3:  // Lane 3 - Bright Magenta/Pink
                coreColor = MakeRGBA(255, 100, 200, 255);
                rimColor = MakeRGBA(255, 60, 180, 255);
                glowColor = MakeRGBA(255, 150, 220, 150);
                break;
            default:
                coreColor = MakeRGBA(180, 180, 255, 255);
                rimColor = MakeRGBA(120, 120, 255, 255);
                glowColor = MakeRGBA(200, 200, 255, 150);
                break;
        }

        // Apply selected color override
        if (note.selected) {
            coreColor = MakeRGBA(255, 200, 100, 255);
            rimColor = MakeRGBA(255, 150, 50, 255);
            glowColor = MakeRGBA(255, 220, 150, 200);
        }

        // Draw glow effect (outer shadow)
        DrawRectangle(x - 3, y - 3, NOTE_SIZE + 6, NOTE_SIZE + 6, Fade(glowColor, 0.3f));

        // Draw outer rim (border effect)
        DrawRectangle(x - 2, y - 2, NOTE_SIZE + 4, NOTE_SIZE + 4, rimColor);

        // Draw highlight strip at top
        DrawRectangle(x + 2, y + 2, NOTE_SIZE - 4, 4, MakeRGBA(255, 255, 255, 80));

        // Draw main note body
        DrawRectangle(x, y, NOTE_SIZE, NOTE_SIZE, coreColor);

        // Draw inner border for depth
        DrawRectangleLines(x + 1, y + 1, NOTE_SIZE - 2, NOTE_SIZE - 2, Fade(WHITE, 0.3f));
    }

    // Draw key hints at bottom
    const char* keys[] = { "D", "F", "J", "K" };
    for (int i = 0; i < LANE_COUNT; i++) {
        int cx = i * currentLaneWidth + currentLaneWidth / 2;
        bool down = false;
        if (i == 0 && IsKeyDown(KEY_D)) down = true;
        if (i == 1 && IsKeyDown(KEY_F)) down = true;
        if (i == 2 && IsKeyDown(KEY_J)) down = true;
        if (i == 3 && IsKeyDown(KEY_K)) down = true;

        int boxW = (int)(44 * scaleX);
        int boxH = (int)(36 * scaleY);
        Color fill = down ? MakeRGBA(255, 255, 255, 200) : MakeRGBA(30, 30, 45, 200);
        DrawRectangle(cx - boxW / 2, currentScreenHeight - 110, boxW, boxH, fill);
        DrawRectangleLines(cx - boxW / 2, currentScreenHeight - 110, boxW, boxH, MakeRGBA(200, 200, 255, 255));
        int tw = MeasureText(keys[i], (int)(22 * scale));
        if (tw < 10) tw = 10;
        DrawText(keys[i], cx - tw / 2, currentScreenHeight - 102, (int)(22 * scale), WHITE);
    }

    // Draw UI bar at bottom
    DrawRectangle(0, currentScreenHeight - 60, currentScreenWidth, 60, MakeRGBA(0, 0, 0, 200));
    DrawRectangleLines(0, currentScreenHeight - 60, currentScreenWidth, 60, MakeRGBA(100, 100, 140, 100));

    int y = currentScreenHeight - 48;
    int textSize = (int)(18 * scale);
    if (textSize < 14) textSize = 14;

    // Status
    const char* statusText = "STOPPED";
    Color statusColor = GRAY;
    if (state.isPlaying && !state.isPaused) {
        statusText = "PLAYING";
        statusColor = GREEN;
    } else if (state.isPlaying && state.isPaused) {
        statusText = "PAUSED";
        statusColor = YELLOW;
    }
    DrawText(statusText, (int)(15 * scaleX), y, (int)(20 * scale), statusColor);

    // Mouse mode indicator
    if (state.mousePlacementMode) {
        DrawText("[MOUSE]", (int)(100 * scaleX), y, (int)(14 * scale), MakeRGBA(100, 255, 150, 255));
    }

    // Time display
    snprintf(timeText, sizeof(timeText), "%.2f/%.2fs", state.gameTime / 1000.0f, state.musicLength / 1000.0f);
    DrawText(timeText, (int)(170 * scaleX), y, textSize, LIGHTGRAY);

    // Note count
    char noteCountText[32];
    snprintf(noteCountText, sizeof(noteCountText), "Notes: %d", (int)state.notes.size());
    DrawText(noteCountText, (int)(340 * scaleX), y, textSize, LIGHTGRAY);

    // Help text at bottom
    int smallSize = (int)(16 * scale);
    if (smallSize < 12) smallSize = 12;
    const char* helpLine1 = "SPACE:Play/Pause  |  D/F/J/K:Add Note  |  Click+Drag:Move  |  RMB/X:Delete  |  Ctrl+S:Save";
    DrawText(helpLine1, (int)(15 * scaleX), currentScreenHeight - 25, smallSize, MakeRGBA(180, 180, 200, 200));

    // Draw save confirmation dialog if active
    if (state.askingSaveOnExit) {
        // Semi-transparent overlay
        DrawRectangle(0, 0, currentScreenWidth, currentScreenHeight, MakeRGBA(0, 0, 0, 150));

        // Dialog box
        int dialogW = (int)(400 * scaleX);
        if (dialogW < 320) dialogW = 320;
        int dialogH = (int)(180 * scaleY);
        if (dialogH < 144) dialogH = 144;
        int dialogX = (currentScreenWidth - dialogW) / 2;
        int dialogY = (currentScreenHeight - dialogH) / 2;

        // Dialog background
        Rectangle dialogRec = { (float)dialogX, (float)dialogY, (float)dialogW, (float)dialogH };
        DrawRectangleRounded(dialogRec, 0.15f, 8, MakeRGBA(30, 30, 45, 250));
        DrawRectangleRoundedLines(dialogRec, 0.15f, 8, 2.0f, MakeRGBA(150, 150, 200, 200));

        // Title
        const char* title = "Save Chart";
        int titleSize = (int)(24 * scale);
        if (titleSize < 19) titleSize = 19;
        int titleW = MeasureText(title, titleSize);
        DrawText(title, dialogX + (dialogW - titleW) / 2, dialogY + (int)(25 * scaleY), titleSize, WHITE);

        // Question text
        const char* question = "Save current chart before exiting?";
        int qSize = (int)(18 * scale);
        if (qSize < 14) qSize = 14;
        int qW = MeasureText(question, qSize);
        DrawText(question, dialogX + (dialogW - qW) / 2, dialogY + (int)(70 * scaleY), qSize, LIGHTGRAY);

        // Yes button
        int btnW = (int)(110 * scaleX);
        if (btnW < 88) btnW = 88;
        int btnH = (int)(45 * scaleY);
        if (btnH < 36) btnH = 36;
        int yesBtnX = dialogX + dialogW / 2 - btnW - (int)(15 * scaleX);
        int yesBtnY = dialogY + dialogH - btnH - (int)(25 * scaleY);

        bool yesHovered = (mx >= yesBtnX && mx <= yesBtnX + btnW && my >= yesBtnY && my <= yesBtnY + btnH);
        Color yesBg = yesHovered ? MakeRGBA(80, 180, 80, 255) : MakeRGBA(50, 140, 50, 255);
        Rectangle yesBtnRec = { (float)yesBtnX, (float)yesBtnY, (float)btnW, (float)btnH };
        DrawRectangleRounded(yesBtnRec, 0.25f, 6, yesBg);
        DrawRectangleRoundedLines(yesBtnRec, 0.25f, 6, 2.0f, WHITE);
        const char* yesText = "Save";
        int yesTextSize = (int)(20 * scale);
        if (yesTextSize < 16) yesTextSize = 16;
        int yesTextW = MeasureText(yesText, yesTextSize);
        DrawText(yesText, yesBtnX + (btnW - yesTextW) / 2, yesBtnY + (btnH - yesTextSize) / 2, yesTextSize, WHITE);

        // Don't Save button
        int noBtnX = dialogX + dialogW / 2 + (int)(15 * scaleX);
        int noBtnY = yesBtnY;
        bool noHovered = (mx >= noBtnX && mx <= noBtnX + btnW && my >= noBtnY && my <= noBtnY + btnH);
        Color noBg = noHovered ? MakeRGBA(180, 80, 80, 255) : MakeRGBA(140, 50, 50, 255);
        Rectangle noBtnRec = { (float)noBtnX, (float)noBtnY, (float)btnW, (float)btnH };
        DrawRectangleRounded(noBtnRec, 0.25f, 6, noBg);
        DrawRectangleRoundedLines(noBtnRec, 0.25f, 6, 2.0f, WHITE);
        const char* noText = "Don't Save";
        int noTextSize = (int)(18 * scale);
        if (noTextSize < 14) noTextSize = 14;
        int noTextW = MeasureText(noText, noTextSize);
        DrawText(noText, noBtnX + (btnW - noTextW) / 2, noBtnY + (btnH - noTextSize) / 2, noTextSize, WHITE);

        // Cancel button (below)
        int cancelBtnW = (int)(140 * scaleX);
        if (cancelBtnW < 112) cancelBtnW = 112;
        int cancelBtnH = btnH;
        int cancelBtnX = dialogX + (dialogW - cancelBtnW) / 2;
        int cancelBtnY = yesBtnY + btnH + (int)(15 * scaleY);
        bool cancelHovered = (mx >= cancelBtnX && mx <= cancelBtnX + cancelBtnW && my >= cancelBtnY && my <= cancelBtnY + cancelBtnH);
        Color cancelBg = cancelHovered ? MakeRGBA(100, 100, 120, 255) : MakeRGBA(70, 70, 90, 255);
        Rectangle cancelBtnRec = { (float)cancelBtnX, (float)cancelBtnY, (float)cancelBtnW, (float)cancelBtnH };
        DrawRectangleRounded(cancelBtnRec, 0.25f, 6, cancelBg);
        DrawRectangleRoundedLines(cancelBtnRec, 0.25f, 6, 2.0f, MakeRGBA(150, 150, 180, 200));
        const char* cancelText = "Cancel";
        int cancelTextSize = (int)(18 * scale);
        if (cancelTextSize < 14) cancelTextSize = 14;
        int cancelTextW = MeasureText(cancelText, cancelTextSize);
        DrawText(cancelText, cancelBtnX + (cancelBtnW - cancelTextW) / 2, cancelBtnY + (cancelBtnH - cancelTextSize) / 2, cancelTextSize, WHITE);
    }

    // Draw help overlay if active
    if (state.showingHelp) {
        // Semi-transparent overlay
        DrawRectangle(0, 0, currentScreenWidth, currentScreenHeight, MakeRGBA(0, 0, 0, 180));

        // Help panel
        int helpW = (int)(600 * scaleX);
        if (helpW < 480) helpW = 480;
        int helpH = (int)(450 * scaleY);
        if (helpH < 360) helpH = 360;
        int helpX = (currentScreenWidth - helpW) / 2;
        int helpY = (currentScreenHeight - helpH) / 2;

        // Panel background
        Rectangle helpRec = { (float)helpX, (float)helpY, (float)helpW, (float)helpH };
        DrawRectangleRounded(helpRec, 0.15f, 8, MakeRGBA(20, 20, 35, 250));
        DrawRectangleRoundedLines(helpRec, 0.15f, 8, 2.0f, MakeRGBA(150, 150, 200, 200));

        // Title
        const char* helpTitle = "Help";
        int titleSize = (int)(26 * scale);
        if (titleSize < 20) titleSize = 20;
        int titleW = MeasureText(helpTitle, titleSize);
        DrawText(helpTitle, helpX + (helpW - titleW) / 2, helpY + (int)(15 * scaleY), titleSize, WHITE);

        // Help content - column 1
        int col1X = helpX + (int)(20 * scaleX);
        int col2X = helpX + helpW / 2 + (int)(10 * scaleX);
        int startY = helpY + (int)(55 * scaleY);
        int lineHeight = (int)(28 * scale);
        int contentSize = (int)(18 * scale);
        if (contentSize < 11) contentSize = 11;

        // Column 1: Basic Controls
        DrawText("Basic Controls:", col1X, startY, contentSize + 2, MakeRGBA(255, 200, 100, 255));
        const char* basic[] = {
            "SPACE - Play/Pause",
            "D / F - Add note (Lane 1/2)",
            "J / K - Add note (Lane 3/4)",
            "Home - Go to start",
            "End - Go to end",
            ""
        };
        for (int i = 0; basic[i][0] != '\0'; i++) {
            DrawText(basic[i], col1X, startY + (i + 1) * lineHeight, contentSize, LIGHTGRAY);
        }

        // Column 2: Selection & Editing
        int col2StartY = startY + (int)(210 * scaleY / 28);
        DrawText("Selection & Editing:", col2X, col2StartY, contentSize + 2, MakeRGBA(100, 200, 255, 255));
        const char* select[] = {
            "Click - Select note",
            "Shift+Click - Multi-select",
            "Drag - Move note",
            "X / Delete - Delete selected",
            "Ctrl+A - Select all visible",
            "Ctrl+Shift+Del - Delete range",
            "Right Click - Quick delete",
            ""
        };
        for (int i = 0; select[i][0] != '\0'; i++) {
            DrawText(select[i], col2X, col2StartY + (i + 1) * lineHeight, contentSize, LIGHTGRAY);
        }

        // Column 2: Special Modes
        DrawText("Special Modes:", col2X, startY+8*lineHeight, contentSize + 2, MakeRGBA(100, 255, 150, 255));
        const char* special[] = {
            "M - Toggle mouse mode",
            "(Click to place notes)",
            "Progress bar - Drag to seek",
            ""
        };
        for (int i = 0; special[i][0] != '\0'; i++) {
            DrawText(special[i], col2X, startY + (i + 1+8) * lineHeight, contentSize, LIGHTGRAY);
        }

        // Column 1: Save & Exit
        DrawText("Save & Exit:", col1X, col2StartY+7*lineHeight, contentSize + 2, MakeRGBA(255, 150, 150, 255));
        const char* saveExit[] = {
            "Ctrl+S - Quick save",
            "Back button - Return to select",
            "Exit button - Return to main menu",
            "Help button - Show this panel",
            "",
        };
        for (int i = 0; saveExit[i][0] != '\0'; i++) {
            DrawText(saveExit[i], col1X, col2StartY + (i + 1+7) * lineHeight, contentSize, LIGHTGRAY);
        }

        // Close hint
        const char* closeHint = "Press H to close";
        int closeSize = (int)(12 * scale);
        int closeW = MeasureText(closeHint, closeSize);
        DrawText(closeHint,  (helpW + closeW) / 2, helpY + helpH - (int)(25 * scaleY), closeSize, MakeRGBA(180, 180, 180, 200));
    }
}

// ============================================================================
// IsMouseInRect - 检测鼠标是否在矩形区域内
// ============================================================================
// 【功能说明】
//   检测鼠标坐标是否在指定矩形区域内
// 【参数说明】
//   - mx, my: 鼠标坐标
//   - x, y: 矩形左上角坐标
//   - w, h: 矩形宽高
// 【返回值】
//   如果鼠标在矩形内返回true，否则返回false
// ============================================================================
bool IsMouseInRect(int mx, int my, int x, int y, int w, int h) {
    return (mx >= x && mx <= x + w && my >= y && my <= y + h);
}

// ============================================================================
// main - 主函数（游戏入口）
// ============================================================================
// 【功能说明】
//   游戏的入口函数，负责初始化、运行主循环、清理资源
// 【初始化阶段】
//   1. 创建窗口：InitWindow(800, 600, "Rhythm Game")
//   2. 初始化音频设备：InitAudioDevice()
//   3. 初始化随机数种子
//   4. 加载音乐列表
//   5. 初始化GameState结构体
//   6. 预加载击打音效
//   7. 初始化背景粒子
//   8. 加载背景音乐
// 【主循环阶段】
//   1. BeginDrawing/ClearBackground
//   2. 获取鼠标状态
//   3. 处理背景音乐播放
//   4. 根据menuState处理不同状态
//   5. 处理游戏进行中状态
//   6. 处理暂停/结算界面
//   7. EndDrawing
// 【清理阶段】
//   1. 停止并卸载音乐流
//   2. 卸载音效
//   3. 关闭音频设备
//   4. 关闭窗口
// ============================================================================
int main() {
    // 初始化窗口
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Rhythm Game");
    InitAudioDevice();

    // 初始化随机数种子
    srand((unsigned int)time(nullptr));

    // 加载音乐列表
    musicList = LoadMusicList();

    // 初始化游戏状态
    GameState gameState;
    gameState.menuState = MenuState::MAIN;
    gameState.selectedMusicIndex = 0;
    gameState.currentDifficulty = Difficulty::NORMAL;
    gameState.score = 0;
    gameState.combo = 0;
    gameState.maxCombo = 0;
    gameState.perfectCount = 0;
    gameState.greatCount = 0;
    gameState.goodCount = 0;
    gameState.missCount = 0;
    gameState.isPlaying = false;
    gameState.gameOver = false;
    gameState.isPaused = false;
    gameState.screenShake = 0.0f;
    gameState.musicScrollOffset = 0;
    for (int i = 0; i < 4; i++) {
        gameState.keyPressed[i] = false;
        gameState.laneFlash[i] = 0.0f;
    }
    // 初始化设置
    gameState.musicVolume = 0.8f;
    gameState.bgMusicVolume = 0.5f;
    gameState.hitSoundVolume = 0.8f;
    gameState.audioOffset = 0.0f;
    gameState.noteSize = 1.0f;
    gameState.noteSpeed = 1.0f;
    gameState.keyBindings[0] = KEY_D;
    gameState.keyBindings[1] = KEY_F;
    gameState.keyBindings[2] = KEY_J;
    gameState.keyBindings[3] = KEY_K;
    gameState.waitingForKey = -1;
    gameState.settingsPage = 0;
    gameState.bgMusicEnabled = true;

    // 加载保存的设置（如果存在）
    LoadSettings(gameState);

    // 预加载击打音效（使用动态生成）
    gameState.perfectSound = LoadSoundFromWave(GenerateHitSound(JudgmentType::PERFECT));
    gameState.greatSound = LoadSoundFromWave(GenerateHitSound(JudgmentType::GREAT));
    gameState.goodSound = LoadSoundFromWave(GenerateHitSound(JudgmentType::GOOD));
    gameState.missSound = LoadSoundFromWave(GenerateHitSound(JudgmentType::MISS));

    // 初始化背景粒子
    InitBgParticles(gameState);

    // 初始化编辑器状态
    EditorState editorState;

    // 游戏变量
    Chart chart;
    Music music;
    memset(&music, 0, sizeof(Music));

    // 加载背景音乐
    Music bgMusic;
    memset(&bgMusic, 0, sizeof(Music));
    const char* bgMusicPath = "bgm.mp3";
    bgMusic = LoadMusicAsset(bgMusicPath);
    if (bgMusic.frameCount > 0) {
        bgMusic.looping = true;
        SetMusicVolume(bgMusic, gameState.bgMusicVolume);
        PlayMusicStream(bgMusic);  // 播放背景音乐
    }

    float accumulatedGameTime = 0.0f;
    float bgMusicTime = 0.0f;

    // 主循环
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);

        int mx = GetMouseX();
        int my = GetMouseY();
        bool mouseClicked = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

        // 背景音乐：菜单播放，游戏/编辑器暂停，退出时重启
        bool inGameOrEditor = gameState.isPlaying || gameState.menuState == MenuState::EDITOR;
        if (!inGameOrEditor && bgMusic.frameCount > 0) {
            SetMusicVolume(bgMusic, gameState.bgMusicVolume);
            if (!IsMusicStreamPlaying(bgMusic)) {
                SeekMusicStream(bgMusic, 0.0f);
                PlayMusicStream(bgMusic);
            }
            UpdateMusicStream(bgMusic);
        } else if (inGameOrEditor && IsMusicStreamPlaying(bgMusic)) {
            PauseMusicStream(bgMusic);
        }

        // 根据菜单状态处理不同逻辑
        // 注意：如果正在游戏中（gameState.isPlaying）或游戏结束（gameState.gameOver），
        // 不处理菜单级别的点击，这样可以防止游戏时/结算时鼠标点击到菜单按钮位置导致的误触发
        switch (gameState.menuState) {
            case MenuState::MAIN: {
                // 游戏进行中或结算时跳过菜单按钮检测
                if (gameState.isPlaying || gameState.gameOver) {
                    DrawMainMenu(0);
                    break;
                }

                DrawMainMenu(0);

                // 计算按钮位置
                int currentScreenWidth = GetScreenWidth();
                int currentScreenHeight = GetScreenHeight();
                float scaleX = (float)currentScreenWidth / 800.0f;
                float scaleY = (float)currentScreenHeight / 600.0f;

                int btnWidth = (int)(280 * scaleX);
                if (btnWidth < 224) btnWidth = 224;
                int btnHeight = (int)(60 * scaleY);
                if (btnHeight < 48) btnHeight = 48;

                // 开始游戏按钮（左侧）
                int playBtnX = currentScreenWidth / 2 - btnWidth - (int)(30 * scaleX);
                int playBtnY = (int)(240 * scaleY);
                bool playClicked = IsMouseInRect(mx, my, playBtnX, playBtnY, btnWidth, btnHeight);

                // 谱面编辑器按钮（右侧）
                int editorBtnX = currentScreenWidth / 2 + (int)(30 * scaleX);
                int editorBtnY = (int)(240 * scaleY);
                bool editorClicked = IsMouseInRect(mx, my, editorBtnX, editorBtnY, btnWidth, btnHeight);

                // 设置按钮（底部居中）
                int settingsBtnW = (int)(180 * scaleX);
                if (settingsBtnW < 144) settingsBtnW = 144;
                int settingsBtnH = (int)(45 * scaleY);
                if (settingsBtnH < 36) settingsBtnH = 36;
                int settingsBtnX = currentScreenWidth / 2 - settingsBtnW / 2;
                int settingsBtnY = (int)(320 * scaleY);
                bool settingsClicked = IsMouseInRect(mx, my, settingsBtnX, settingsBtnY, settingsBtnW, settingsBtnH);

                // 处理鼠标点击
                if (mouseClicked) {
                    if (playClicked) {
                        gameState.enteringEditor = false;
                        gameState.isGameMode = true;
                        gameState.menuState = MenuState::SELECT_MUSIC;
                    } else if (editorClicked) {
                        gameState.enteringEditor = true;
                        gameState.isGameMode = false;
                        gameState.menuState = MenuState::SELECT_MUSIC;
                    } else if (settingsClicked) {
                        gameState.menuState = MenuState::SETTINGS;
                    }
                }

                // 键盘支持
                if (IsKeyPressed(KEY_ENTER)) {
                    gameState.menuState = MenuState::SELECT_MUSIC;
                }
                break;
            }

            case MenuState::SELECT_MUSIC: {
                // 游戏进行中或结算时跳过选曲界面点击检测
                if (gameState.isPlaying || gameState.gameOver) {
                    DrawMusicSelect(gameState.selectedMusicIndex, gameState.musicScrollOffset, nullptr, false);
                    break;
                }

                DrawMusicSelect(gameState.selectedMusicIndex, gameState.musicScrollOffset, nullptr, false);

                // 音乐列表鼠标点击选择
                int currentScreenWidth = GetScreenWidth();
                int currentScreenHeight = GetScreenHeight();
                float scaleX = (float)currentScreenWidth / 800.0f;
                float scaleY = (float)currentScreenHeight / 600.0f;

                int itemHeight = (int)(60 * scaleY);
                int listStartY = (int)(100 * scaleY);
                int itemPadding = (int)(10 * scaleY);

                // 跳过状态切换后的第一帧点击检测
                if (gameState.skipNextClick) {
                    gameState.skipNextClick = false;
                }
                else if (mouseClicked) {
                    // 返回按钮优先检查
                    int backBtnW = (int)(120 * scaleX);
                    int backBtnH = (int)(35 * scaleY);
                    if (backBtnH < 28) backBtnH = 28;
                    int backBtnX = (int)(20 * scaleX);
                    int backBtnY = (int)(20 * scaleY);
                    if (IsMouseInRect(mx, my, backBtnX, backBtnY, backBtnW, backBtnH)) {
                        gameState.menuState = MenuState::MAIN;
                    }
                    // 检查是否点击了某个音乐项
                    else {
                        // 布局参数
                        const int LIST_START_Y = (int)(160 * scaleY);
                        int itemHeight = (int)(50 * scaleY);
                        if (itemHeight < 40) itemHeight = 40;
                        const int VISIBLE_ITEMS = 7;
                        int LIST_WIDTH = currentScreenWidth - (int)(100 * scaleX);
                        if (LIST_WIDTH < 400) LIST_WIDTH = 400;

                        int startIdx = gameState.musicScrollOffset;
                        int endIdx = startIdx + VISIBLE_ITEMS;
                        if (endIdx > (int)musicList.size()) endIdx = (int)musicList.size();

                        // 与 DrawMusicSelect 保持完全一致
                        // 列表容器：x=(40*scaleX), LIST_WIDTH
                        // 项目：x=(45*scaleX), width=LIST_WIDTH-(60*scaleX)
                        int itemX = (int)(45 * scaleX);
                        int itemWidth = LIST_WIDTH - (int)(60 * scaleX);

                        int clickedIndex = -1;
                        for (int i = startIdx; i < endIdx; i++) {
                            int displayIndex = i - startIdx;
                            int itemY = LIST_START_Y + displayIndex * itemHeight;
                            // 使用与绘制完全相同的参数
                            if (IsMouseInRect(mx, my, itemX, itemY, itemWidth, itemHeight - 4)) {
                                clickedIndex = i;
                                break;
                            }
                        }

                        if (clickedIndex >= 0 && clickedIndex < (int)musicList.size()) {
                            gameState.selectedMusicIndex = clickedIndex;
                            gameState.skipNextClick = true;  // 防止当帧误触发难度选择
                            gameState.menuState = MenuState::SELECT_DIFFICULTY;
                        }
                    }
                }

                if (IsKeyPressed(KEY_UP) && gameState.selectedMusicIndex > 0) {
                    gameState.selectedMusicIndex--;
                }
                if (IsKeyPressed(KEY_DOWN) && gameState.selectedMusicIndex < (int)musicList.size() - 1) {
                    gameState.selectedMusicIndex++;
                }
                if (IsKeyPressed(KEY_ENTER)) {
                    gameState.menuState = MenuState::SELECT_DIFFICULTY;
                }
                if (IsKeyPressed(KEY_BACKSPACE)) {
                    gameState.menuState = MenuState::MAIN;
                }
                break;
            }

            case MenuState::SELECT_DIFFICULTY: {
                // 游戏进行中或结算时跳过难度选择界面点击检测
                if (gameState.isPlaying || gameState.gameOver) {
                    DrawDifficultySelect(gameState.selectedMusicIndex, gameState.currentDifficulty, 0, gameState.isGameMode);
                    break;
                }

                DrawDifficultySelect(gameState.selectedMusicIndex, gameState.currentDifficulty, 0, gameState.isGameMode);

                // 难度选择鼠标点击（与 DrawDifficultySelect 保持一致）
                int currentScreenWidth = GetScreenWidth();
                int currentScreenHeight = GetScreenHeight();
                int mx = GetMouseX();
                int my = GetMouseY();
                float scaleX = (float)currentScreenWidth / 800.0f;
                float scaleY = (float)currentScreenHeight / 600.0f;
                float scale = (scaleX + scaleY) / 2.0f;
                if (scale < 0.5f) scale = 0.5f;
                if (scale > 2.0f) scale = 2.0f;

                // 与 DrawDifficultySelect 保持一致的布局参数
                int backBtnWidth = (int)(120 * scaleX);
                int backBtnHeight = (int)(35 * scaleY);
                if (backBtnHeight < 28) backBtnHeight = 28;
                int OPTION_START_Y = (int)(200 * scaleY);
                int OPTION_HEIGHT = (int)(70 * scaleY);
                if (OPTION_HEIGHT < 56) OPTION_HEIGHT = 56;
                int OPTION_WIDTH = (int)(300 * scaleX);
                if (OPTION_WIDTH < 240) OPTION_WIDTH = 240;

                // 跳过状态切换后的第一帧点击检测
                if (gameState.skipNextClick) {
                    gameState.skipNextClick = false;
                }
                else if (mouseClicked) {
                    // 返回按钮优先检查（与绘制位置一致）
                    int backBtnX = (int)(20 * scaleX);
                    int backBtnY = (int)(20 * scaleY);
                    if (IsMouseInRect(mx, my, backBtnX, backBtnY, backBtnWidth, backBtnHeight)) {
                        gameState.menuState = MenuState::SELECT_MUSIC;
                    }
                    // 开始游戏按钮（优先于难度选择）
                    else {
                        int startBtnWidth = (int)(200 * scaleX);
                        if (startBtnWidth < 160) startBtnWidth = 160;
                        int startBtnHeight = (int)(50 * scaleY);
                        if (startBtnHeight < 40) startBtnHeight = 40;
                        int hardY = OPTION_START_Y + (OPTION_HEIGHT + (int)(15 * scaleY)) * 2;
                        int startBtnY = hardY + OPTION_HEIGHT + (int)(30 * scaleY);
                        if (IsMouseInRect(mx, my, currentScreenWidth / 2 - startBtnWidth / 2, startBtnY, startBtnWidth, startBtnHeight)) {
                            // 加载谱面和音乐
                            if (gameState.selectedMusicIndex >= 0 && gameState.selectedMusicIndex < (int)musicList.size()) {
                                const MusicInfo& selectedMusic = musicList[gameState.selectedMusicIndex];

                                if (gameState.enteringEditor) {
                                    // ===== 编辑器模式入口 =====
                                    // 初始化编辑器
                                    editorState.selectedMusicIndex = gameState.selectedMusicIndex;
                                    editorState.selectedDifficulty = gameState.currentDifficulty;
                                    editorState.notes.clear();
                                    editorState.gameTime = 0.0f;
                                    editorState.isPlaying = false;
                                    editorState.isPaused = true;
                                    editorState.isRecording = false;
                                    editorState.selectedNoteIndex = -1;
                                    editorState.musicLength = 0.0f;
                                    editorState.askingSaveOnExit = false;
                                    editorState.showingHelp = false;
                                    editorState.mousePlacementMode = false;
                                    editorState.gameStatePtr = &gameState;  // 设置gameState指针

                                    // 加载已有谱面（如果存在）
                                    if (!selectedMusic.chartFile.empty()) {
                                        const char* diffNames[] = {"easy", "normal", "hard"};
                                        char chartPath[256];
                                        snprintf(chartPath, sizeof(chartPath), "%s_%s.txt", selectedMusic.chartFile.c_str(), diffNames[(int)gameState.currentDifficulty]);
                                        if (AssetExists(chartPath)) {
                                            EditorState loaded = LoadEditorChart(chartPath);
                                            editorState.notes = loaded.notes;
                                            editorState.offset = loaded.offset;
                                        }
                                    }

                                    // Load music and update editor state
                                    if (music.frameCount > 0) {
                                        StopMusicStream(music);
                                        UnloadMusicStream(music);
                                    }
                                    memset(&music, 0, sizeof(Music));
                                    editorState.musicLength = 0.0f;  // Reset before loading

                                    if (!selectedMusic.filePath.empty()) {
                                        music = LoadMusicAsset(selectedMusic.filePath.c_str());
                                        if (music.frameCount > 0) {
                                            music.looping = false;
                                            SetMusicVolume(music, gameState.musicVolume);
                                            // GetMusicTimeLength returns seconds, convert to milliseconds
                                            editorState.musicLength = GetMusicTimeLength(music) * 1000.0f;
                                        } else {
                                            memset(&music, 0, sizeof(Music));
                                        }
                                    }

                                    gameState.menuState = MenuState::EDITOR;
                                    break;  // 防止继续执行游戏模式的else块
                                } else {
                                    // ===== 游戏模式入口 =====
                                    // Loading animation before game starts (loop for ~1.5 seconds)
                                    // 修复：先结束主循环的Drawing，再运行加载界面
                                    EndDrawing();
                                    
                                    for (int i = 0; i < 90; i++) {  // 90 frames at 60fps = 1.5 seconds
                                        BeginDrawing();
                                        ClearBackground(MakeRGBA(20, 20, 35, 255));

                                        int loadW = GetScreenWidth();
                                        int loadH = GetScreenHeight();
                                        float loadS = ((float)loadW / 800.0f + (float)loadH / 600.0f) / 2.0f;
                                        if (loadS < 0.5f) loadS = 0.5f;
                                        if (loadS > 2.0f) loadS = 2.0f;

                                        const char* loadTxt = "Loading...";
                                        int loadTxtS = (int)(24 * loadS);
                                        if (loadTxtS < 16) loadTxtS = 16;
                                        int loadTxtW = MeasureText(loadTxt, loadTxtS);
                                        DrawText(loadTxt, loadW / 2 - loadTxtW / 2, loadH / 2, loadTxtS, WHITE);

                                        int barW = (int)(300 * loadS);
                                        int barH = (int)(20 * loadS);
                                        int barX = loadW / 2 - barW / 2;
                                        int barY = loadH / 2 + (int)(50 * loadS);
                                        DrawRectangle(barX, barY, barW, barH, MakeRGBA(60, 60, 80, 255));

                                        // Animated progress bar
                                        float progress = (float)i / 90.0f;
                                        int fillW = (int)(progress * barW);
                                        DrawRectangle(barX, barY, fillW, barH, MakeRGBA(100, 200, 150, 255));
                                        DrawRectangleLines(barX, barY, barW, barH, PURPLE);

                                        EndDrawing();
                                    }

                                    chart = LoadChart(gameState.currentDifficulty, selectedMusic.chartFile.c_str());

                                    for (auto& note : chart.notes) {
                                        note.active = true;
                                        note.judged = false;
                                        float currentSpeed = NOTE_SPEED * gameState.noteSpeed;
                                        float  currentTime = GetMusicTimePlayed(music);
                                        note.y = (float)(GetScreenHeight() - 100) - ((note.hitTime - currentTime+ gameState.audioOffset) / 1000.0f) * currentSpeed * 60.0f;
                                    }

                                    gameState.score = 0;
                                    gameState.combo = 0;
                                    gameState.maxCombo = 0;
                                    gameState.perfectCount = 0;
                                    gameState.greatCount = 0;
                                    gameState.goodCount = 0;
                                    gameState.missCount = 0;
                                    gameState.isPlaying = true;
                                    gameState.gameOver = false;
                                    gameState.isPaused = false;
                                    gameState.screenShake = 0.0f;
                                    accumulatedGameTime = 0.0f;

                                    if (music.frameCount > 0) {
                                        StopMusicStream(music);
                                        UnloadMusicStream(music);
                                    }

                                    if (!selectedMusic.filePath.empty()) {
                                        music = LoadMusicAsset(selectedMusic.filePath.c_str());
                                        music.looping = false;
                                        if (music.frameCount > 0) {
                                            SetMusicVolume(music, gameState.musicVolume);
                                            PlayMusicStream(music);
                                        }
                                    }
                                    
                                    // 修复：跳过主循环剩余的EndDrawing()，直接进入下一帧
                                    continue;
                                }
                            }
                        }
                        // 难度选项（只在未点击开始按钮时处理）
                        else {
                            // Easy选项
                            int easyY = OPTION_START_Y;
                            if (IsMouseInRect(mx, my, currentScreenWidth / 2 - OPTION_WIDTH / 2, easyY, OPTION_WIDTH, OPTION_HEIGHT)) {
                                gameState.currentDifficulty = Difficulty::EASY;
                            }
                            // Normal选项
                            int normalY = OPTION_START_Y + OPTION_HEIGHT + (int)(15 * scaleY);
                            if (IsMouseInRect(mx, my, currentScreenWidth / 2 - OPTION_WIDTH / 2, normalY, OPTION_WIDTH, OPTION_HEIGHT)) {
                                gameState.currentDifficulty = Difficulty::NORMAL;
                            }
                            // Hard选项
                            int hardY = OPTION_START_Y + (OPTION_HEIGHT + (int)(15 * scaleY)) * 2;
                            if (IsMouseInRect(mx, my, currentScreenWidth / 2 - OPTION_WIDTH / 2, hardY, OPTION_WIDTH, OPTION_HEIGHT)) {
                                gameState.currentDifficulty = Difficulty::HARD;
                            }
                        }
                    }
                }

                if (IsKeyPressed(KEY_ONE)) gameState.currentDifficulty = Difficulty::EASY;
                if (IsKeyPressed(KEY_TWO)) gameState.currentDifficulty = Difficulty::NORMAL;
                if (IsKeyPressed(KEY_THREE)) gameState.currentDifficulty = Difficulty::HARD;
                if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
                    if (gameState.currentDifficulty == Difficulty::NORMAL) gameState.currentDifficulty = Difficulty::EASY;
                    else if (gameState.currentDifficulty == Difficulty::HARD) gameState.currentDifficulty = Difficulty::NORMAL;
                }
                if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
                    if (gameState.currentDifficulty == Difficulty::EASY) gameState.currentDifficulty = Difficulty::NORMAL;
                    else if (gameState.currentDifficulty == Difficulty::NORMAL) gameState.currentDifficulty = Difficulty::HARD;
                }
                if (IsKeyPressed(KEY_ENTER)) {
                    // 加载谱面和音乐
                    if (gameState.selectedMusicIndex >= 0 && gameState.selectedMusicIndex < (int)musicList.size()) {
                        const MusicInfo& selectedMusic = musicList[gameState.selectedMusicIndex];

                        if (gameState.enteringEditor) {
                            // ===== 编辑器模式入口（键盘） =====
                            // 初始化编辑器
                            editorState.selectedMusicIndex = gameState.selectedMusicIndex;
                            editorState.selectedDifficulty = gameState.currentDifficulty;
                            editorState.notes.clear();
                            editorState.gameTime = 0.0f;
                            editorState.isPlaying = false;
                            editorState.isPaused = true;
                            editorState.isRecording = false;
                            editorState.selectedNoteIndex = -1;
                            editorState.musicLength = 0.0f;
                            editorState.askingSaveOnExit = false;
                            editorState.showingHelp = false;
                            editorState.mousePlacementMode = false;
                            editorState.gameStatePtr = &gameState;  // 设置gameState指针

                            // 加载已有谱面（如果存在）
                            if (!selectedMusic.chartFile.empty()) {
                                const char* diffNames[] = {"easy", "normal", "hard"};
                                char chartPath[256];
                                snprintf(chartPath, sizeof(chartPath), "%s_%s.txt", selectedMusic.chartFile.c_str(), diffNames[(int)gameState.currentDifficulty]);
                                if (AssetExists(chartPath)) {
                                    EditorState loaded = LoadEditorChart(chartPath);
                                    editorState.notes = loaded.notes;
                                    editorState.offset = loaded.offset;
                                }
                            }

                            // Load music and update editor state (keyboard)
                            if (music.frameCount > 0) {
                                StopMusicStream(music);
                                UnloadMusicStream(music);
                            }
                            memset(&music, 0, sizeof(Music));
                            editorState.musicLength = 0.0f;  // Reset before loading

                            if (!selectedMusic.filePath.empty()) {
                                music = LoadMusicAsset(selectedMusic.filePath.c_str());
                                if (music.frameCount > 0) {
                                    music.looping = false;
                                    SetMusicVolume(music, gameState.musicVolume);
                                    editorState.musicLength = GetMusicTimeLength(music) * 1000.0f;
                                } else {
                                    memset(&music, 0, sizeof(Music));
                                }
                            }

                            gameState.menuState = MenuState::EDITOR;
                            break;  // 防止继续执行游戏模式的else块
                        } else {
                            // ===== 游戏模式入口（键盘） =====
                            // Loading animation before game starts (loop for ~1.5 seconds)
                            // 修复：先结束主循环的Drawing，再运行加载界面
                            EndDrawing();
                            
                            for (int i = 0; i < 90; i++) {  // 90 frames at 60fps = 1.5 seconds
                                BeginDrawing();
                                ClearBackground(MakeRGBA(20, 20, 35, 255));

                                int loadW = GetScreenWidth();
                                int loadH = GetScreenHeight();
                                float loadS = ((float)loadW / 800.0f + (float)loadH / 600.0f) / 2.0f;
                                if (loadS < 0.5f) loadS = 0.5f;
                                if (loadS > 2.0f) loadS = 2.0f;

                                const char* loadTxt = "Loading...";
                                int loadTxtS = (int)(24 * loadS);
                                if (loadTxtS < 16) loadTxtS = 16;
                                int loadTxtW = MeasureText(loadTxt, loadTxtS);
                                DrawText(loadTxt, loadW / 2 - loadTxtW / 2, loadH / 2, loadTxtS, WHITE);

                                int barW = (int)(300 * loadS);
                                int barH = (int)(20 * loadS);
                                int barX = loadW / 2 - barW / 2;
                                int barY = loadH / 2 + (int)(50 * loadS);
                                DrawRectangle(barX, barY, barW, barH, MakeRGBA(60, 60, 80, 255));

                                // Animated progress bar
                                float progress = (float)i / 90.0f;
                                int fillW = (int)(progress * barW);
                                DrawRectangle(barX, barY, fillW, barH, MakeRGBA(100, 200, 150, 255));
                                DrawRectangleLines(barX, barY, barW, barH, PURPLE);

                                EndDrawing();
                            }
                            
                            chart = LoadChart(gameState.currentDifficulty, selectedMusic.chartFile.c_str());

                            for (auto& note : chart.notes) {
                                note.active = true;
                                note.judged = false;
                                float currentSpeed = NOTE_SPEED * gameState.noteSpeed;
                                float  currentTime = GetMusicTimePlayed(music);
                                note.y = (float)(GetScreenHeight() - 100) - ((note.hitTime - currentTime + gameState.audioOffset) / 1000.0f) * currentSpeed * 60.0f;
                            }

                            gameState.score = 0;
                            gameState.combo = 0;
                            gameState.maxCombo = 0;
                            gameState.perfectCount = 0;
                            gameState.greatCount = 0;
                            gameState.goodCount = 0;
                            gameState.missCount = 0;
                            gameState.isPlaying = true;
                            gameState.gameOver = false;
                            gameState.isPaused = false;
                            gameState.screenShake = 0.0f;
                            accumulatedGameTime = 0.0f;

                            if (music.frameCount > 0) {
                                StopMusicStream(music);
                                UnloadMusicStream(music);
                            }

                            if (!selectedMusic.filePath.empty()) {
                                music = LoadMusicAsset(selectedMusic.filePath.c_str());
                                music.looping = false;
                                if (music.frameCount > 0) {
                                    SetMusicVolume(music, gameState.musicVolume);
                                    PlayMusicStream(music);
                                }
                            }
                            
                            // 修复：跳过主循环剩余的EndDrawing()，直接进入下一帧
                            continue;
                        }
                    }
                }
                if (IsKeyPressed(KEY_BACKSPACE)) {
                    gameState.menuState = MenuState::SELECT_MUSIC;
                }
                break;
            }

            case MenuState::SETTINGS:
                // 游戏进行中或结算时跳过设置界面点击检测
                if (gameState.isPlaying || gameState.gameOver) {
                    DrawSettings(gameState);
                    break;
                }

                DrawSettings(gameState);

                // 返回按钮处理
                {
                    int currentScreenWidth = GetScreenWidth();
                    int currentScreenHeight = GetScreenHeight();
                    int mx = GetMouseX();
                    int my = GetMouseY();
                    float scaleX = (float)currentScreenWidth / 800.0f;
                    float scaleY = (float)currentScreenHeight / 600.0f;

                    int backBtnX = (int)(20 * scaleX);
                    int backBtnY = (int)(20 * scaleY);
                    int backBtnW = (int)(120 * scaleX);
                    int backBtnH = (int)(35 * scaleY);

                    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
                        IsMouseInRect(mx, my, backBtnX, backBtnY, backBtnW, backBtnH)) {
                        SaveSettings(gameState);
                        gameState.menuState = MenuState::MAIN;
                    }
                }

                if (IsKeyPressed(KEY_BACKSPACE)) {
                    SaveSettings(gameState);
                    gameState.menuState = MenuState::MAIN;
                }
                break;

            case MenuState::EDITOR:
                // 游戏进行中或结算时跳过编辑器界面（不应该同时发生，但做保护）
                if (gameState.isPlaying || gameState.gameOver) {
                    DrawEditorScreen(editorState, music, music.frameCount > 0);
                    UpdateEditor(editorState, music, music.frameCount > 0);
                    break;
                }

                // Debug: check music status
                if (music.frameCount == 0) {
                    static int warningCount = 0;
                    if (warningCount < 5) {  // Limit warning messages
                        warningCount++;
                    }
                }
                if (editorState.musicLength <= 0) {
                    static int lengthWarningCount = 0;
                    if (lengthWarningCount < 5) {
                        lengthWarningCount++;
                    }
                }
                DrawEditorScreen(editorState, music, music.frameCount > 0);
                UpdateEditor(editorState, music, music.frameCount > 0);
                break;

            default:
                break;
        }

        // 游戏进行中
        if (gameState.isPlaying) {
            HandleInput(chart, gameState);
            float deltaTime = GetFrameTime();
            UpdateGame(chart, gameState, &music, deltaTime);

            if (!gameState.isPaused && music.frameCount > 0) {
                UpdateMusicStream(music);
            }

            if (!gameState.isPaused) {
                accumulatedGameTime += deltaTime;
            }

            DrawGame(chart, gameState, music, accumulatedGameTime);
            DrawHUD(gameState);

            if (gameState.isPaused) {
                DrawPauseMenu();

                // 暂停菜单按钮点击检测
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    int currentScreenWidth = GetScreenWidth();
                    int currentScreenHeight = GetScreenHeight();
                    int mx = GetMouseX();
                    int my = GetMouseY();
                    float scaleX = (float)currentScreenWidth / 800.0f;
                    float scaleY = (float)currentScreenHeight / 600.0f;
                    float scale = (scaleX + scaleY) / 2.0f;
                    if (scale < 0.5f) scale = 0.5f;
                    if (scale > 2.0f) scale = 2.0f;

                    int btnWidth = (int)(200 * scaleX);
                    if (btnWidth < 160) btnWidth = 160;
                    int btnHeight = (int)(45 * scaleY);
                    if (btnHeight < 36) btnHeight = 36;
                    int btnX = currentScreenWidth / 2 - btnWidth / 2;
                    int boxHeight = (int)(280 * scaleY);
                    if (boxHeight < 224) boxHeight = 224;
                    int boxY = currentScreenHeight / 2 - boxHeight / 2;

                    // Resume button
                    int resumeBtnY = boxY + (int)(100 * scaleY);
                    if (IsMouseInRect(mx, my, btnX, resumeBtnY, btnWidth, btnHeight)) {
                        gameState.isPaused = false;
                    }
                    // Restart button
                    int restartBtnY = boxY + (int)(160 * scaleY);
                    if (IsMouseInRect(mx, my, btnX, restartBtnY, btnWidth, btnHeight)) {
                        // 重新开始游戏
                        gameState.score = 0;
                        gameState.combo = 0;
                        gameState.maxCombo = 0;
                        gameState.perfectCount = 0;
                        gameState.greatCount = 0;
                        gameState.goodCount = 0;
                        gameState.missCount = 0;
                        gameState.isPaused = false;
                        gameState.gameOver = false;
                        accumulatedGameTime = 0.0f;
                        // 重新加载谱面并计算音符位置
                        if (gameState.selectedMusicIndex >= 0 && gameState.selectedMusicIndex < (int)musicList.size()) {
                            const MusicInfo& selectedMusic = musicList[gameState.selectedMusicIndex];
                            if (!selectedMusic.chartFile.empty()) {
                                chart = LoadChart(gameState.currentDifficulty, selectedMusic.chartFile.c_str());
                            }
                        }
                        for (auto& note : chart.notes) {
                            note.active = true;
                            note.judged = false;
                            float currentSpeed = NOTE_SPEED * gameState.noteSpeed;
                            note.y = (float)(GetScreenHeight() - 100) - ((note.hitTime + gameState.audioOffset) / 1000.0f) * currentSpeed * 60.0f;
                        }
                        if (music.frameCount > 0) {
                            SeekMusicStream(music, 0.0f);
                            PlayMusicStream(music);
                        }
                    }
                    // Quit to Menu button
                    int quitBtnY = boxY + (int)(220 * scaleY);
                    if (IsMouseInRect(mx, my, btnX, quitBtnY, btnWidth, btnHeight)) {
                        gameState.isPlaying = false;
                        gameState.isPaused = false;
                        gameState.enteringEditor = false;  // 重置为游戏模式
                        gameState.menuState = MenuState::MAIN;
                        if (music.frameCount > 0) {
                            StopMusicStream(music);
                        }
                    }
                }
            }
        }
        else if (gameState.gameOver) {
            // 结算界面显示
            ClearBackground(MakeRGBA(20, 20, 30, 255));
            DrawResultScreen(gameState);

            // 键盘处理：R键重新开始，ESC返回主菜单
            if (IsKeyPressed(KEY_R)) {
                gameState.score = 0;
                gameState.combo = 0;
                gameState.maxCombo = 0;
                gameState.perfectCount = 0;
                gameState.greatCount = 0;
                gameState.goodCount = 0;
                gameState.missCount = 0;
                gameState.isPaused = false;
                gameState.gameOver = false;
                gameState.isPlaying = true;
                accumulatedGameTime = 0.0f;
                // 重新加载谱面并计算音符位置
                if (gameState.selectedMusicIndex >= 0 && gameState.selectedMusicIndex < (int)musicList.size()) {
                    const MusicInfo& selectedMusic = musicList[gameState.selectedMusicIndex];
                    if (!selectedMusic.chartFile.empty()) {
                        chart = LoadChart(gameState.currentDifficulty, selectedMusic.chartFile.c_str());
                    }
                }
                for (auto& note : chart.notes) {
                    note.active = true;
                    note.judged = false;
                    float currentSpeed = NOTE_SPEED * gameState.noteSpeed;
                    note.y = (float)(GetScreenHeight() - 100) - ((note.hitTime + gameState.audioOffset) / 1000.0f) * currentSpeed * 60.0f;
                }
                if (music.frameCount > 0) {
                    SeekMusicStream(music, 0.0f);
                    PlayMusicStream(music);
                }
            }
            if (IsKeyPressed(KEY_ESCAPE)) {
                gameState.isPlaying = false;
                gameState.gameOver = false;
                gameState.enteringEditor = false;
                gameState.menuState = MenuState::MAIN;
                if (music.frameCount > 0) {
                    StopMusicStream(music);
                }
            }

            // 结算界面按钮点击检测
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                int currentScreenWidth = GetScreenWidth();
                int currentScreenHeight = GetScreenHeight();
                int mx = GetMouseX();
                int my = GetMouseY();
                float scaleX = (float)currentScreenWidth / 800.0f;
                float scaleY = (float)currentScreenHeight / 600.0f;
                float scale = (scaleX + scaleY) / 2.0f;
                if (scale < 0.5f) scale = 0.5f;
                if (scale > 2.0f) scale = 2.0f;

                int btnWidth = (int)(150 * scaleX);
                if (btnWidth < 120) btnWidth = 120;
                int btnHeight = (int)(45 * scaleY);
                if (btnHeight < 36) btnHeight = 36;
                int btnX = currentScreenWidth / 2 - btnWidth / 2;
                int boxHeight = (int)(420 * scaleY);
                if (boxHeight < 336) boxHeight = 336;
                int boxY = (int)(50 * scaleY);
                int btnY = boxY + boxHeight + (int)(20 * scaleY);
                int btnSpacing = (int)(170 * scaleX);

                // Play Again button (left)
                if (IsMouseInRect(mx, my, btnX - btnSpacing, btnY, btnWidth, btnHeight)) {
                    gameState.score = 0;
                    gameState.combo = 0;
                    gameState.maxCombo = 0;
                    gameState.perfectCount = 0;
                    gameState.greatCount = 0;
                    gameState.goodCount = 0;
                    gameState.missCount = 0;
                    gameState.isPaused = false;
                    gameState.gameOver = false;
                    gameState.isPlaying = true;
                    accumulatedGameTime = 0.0f;
                    // 重新加载谱面并计算音符位置
                    if (gameState.selectedMusicIndex >= 0 && gameState.selectedMusicIndex < (int)musicList.size()) {
                        const MusicInfo& selectedMusic = musicList[gameState.selectedMusicIndex];
                        if (!selectedMusic.chartFile.empty()) {
                            chart = LoadChart(gameState.currentDifficulty, selectedMusic.chartFile.c_str());
                        }
                    }
                    for (auto& note : chart.notes) {
                        note.active = true;
                        note.judged = false;
                        float currentSpeed = NOTE_SPEED * gameState.noteSpeed;
                        note.y = (float)(GetScreenHeight() - 100) - ((note.hitTime + gameState.audioOffset) / 1000.0f) * currentSpeed * 60.0f;
                    }
                    if (music.frameCount > 0) {
                        SeekMusicStream(music, 0.0f);
                        PlayMusicStream(music);
                    }
                }
                // Select Music button (center)
                if (IsMouseInRect(mx, my, btnX, btnY, btnWidth, btnHeight)) {
                    gameState.isPlaying = false;
                    gameState.gameOver = false;
                    gameState.enteringEditor = false;
                    gameState.skipNextClick = true;  // 防止当帧误触发音乐项点击
                    gameState.menuState = MenuState::SELECT_MUSIC;
                    if (music.frameCount > 0) {
                        StopMusicStream(music);
                    }
                }
                // Main Menu button (right)
                if (IsMouseInRect(mx, my, btnX + btnSpacing, btnY, btnWidth, btnHeight)) {
                    gameState.isPlaying = false;
                    gameState.gameOver = false;
                    gameState.enteringEditor = false;
                    gameState.skipNextClick = true;  // 防止当帧误触发音乐项点击
                    gameState.menuState = MenuState::MAIN;
                    if (music.frameCount > 0) {
                        StopMusicStream(music);
                    }
                }
            }
        }
        else {
            // 非游戏状态，暂停时恢复背景音乐
            // 注意：编辑器模式下不要干预音乐播放，编辑器自己管理
            if (gameState.menuState != MenuState::EDITOR) {
                if (music.frameCount > 0 && IsMusicStreamPlaying(music)) {
                    PauseMusicStream(music);
                }
            }
        }

        EndDrawing();
    }

    // 清理资源
    if (music.frameCount > 0) {
        StopMusicStream(music);
        UnloadMusicStream(music);
    }
    if (bgMusic.frameCount > 0) {
        StopMusicStream(bgMusic);
        UnloadMusicStream(bgMusic);
    }

    UnloadSound(gameState.perfectSound);
    UnloadSound(gameState.greatSound);
    UnloadSound(gameState.goodSound);
    UnloadSound(gameState.missSound);

    // 保存当前设置
    SaveSettings(gameState);

    CloseAudioDevice();
    CloseWindow();

    return 0;
}
