// ============================================================================
// effects.cpp - 视觉效果实现
// ============================================================================
// 本文件实现游戏的视觉效果系统，包括：
//   - 判定文字特效（PERFECT/GREAT/GOOD/MISS）
//   - 击中圆圈扩散特效
//   - 粒子爆炸效果
//   - 背景装饰粒子
// ============================================================================

#include "effects.h"

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
// SpawnJudgmentEffect - 生成判定文字效果
// ============================================================================
// 【功能说明】
//   在判定线上方生成"PERFECT"、"GREAT"等判定文字
// 【参数说明】
//   - chart: 谱面引用（用于存储特效）
//   - type: 判定类型（PERFECT/GREAT/GOOD/MISS）
//   - lane: 轨道号（0-3）
//   - laneWidth: 单轨道宽度
//   - judgeLineY: 判定线Y坐标
// 【特效属性】
//   - 位置：判定线上方60像素，轨道中心
//   - 初始透明度：1.0（完全不透明）
//   - 初始缩放：1.5倍
//   - 存活时间：1秒
// 【颜色映射】
//   - PERFECT: 金色 (GOLD)
//   - GREAT: 蓝色 (BLUE)
//   - GOOD: 绿色 (GREEN)
//   - MISS: 红色 (RED)
// ============================================================================
void SpawnJudgmentEffect(Chart& chart, JudgmentType type, int lane, int laneWidth, int judgeLineY) {
    JudgmentEffect effect;
    effect.text = "";
    effect.lane = lane;
    effect.x = (float)(lane * laneWidth + laneWidth / 2);
    effect.y = (float)(judgeLineY - 60);  // 判定线上方60像素
    effect.alpha = 1.0f;
    effect.scale = 1.5f;
    effect.color = WHITE;
    effect.active = true;
    effect.lifetime = 1.0f;

    switch (type) {
        case JudgmentType::PERFECT: effect.text = "PERFECT"; effect.color = GOLD; break;
        case JudgmentType::GREAT: effect.text = "GREAT"; effect.color = BLUE; break;
        case JudgmentType::GOOD: effect.text = "GOOD"; effect.color = GREEN; break;
        case JudgmentType::MISS: effect.text = "MISS"; effect.color = RED; break;
    }
    chart.judgmentEffects.push_back(effect);
}

// ============================================================================
// SpawnHitEffect - 生成圆圈扩散效果
// ============================================================================
// 【功能说明】
//   在判定线位置生成圆圈扩散动画
// 【参数说明】
//   - chart: 谱面引用
//   - type: 判定类型
//   - lane: 轨道号
//   - laneWidth: 单轨道宽度
//   - judgeLineY: 判定线Y坐标
// 【特效属性】
//   - 初始半径：NOTE_SIZE/2（约20像素）
//   - 最大半径：NOTE_SIZE*2（约80像素）
//   - 初始透明度：1.0
// 【动画原理】
//   每帧半径增大，透明度递减，产生圆圈扩散+淡出效果
// ============================================================================
void SpawnHitEffect(Chart& chart, JudgmentType type, int lane, int laneWidth, int judgeLineY) {
    HitEffect effect;
    effect.x = (float)(lane * laneWidth + laneWidth / 2);
    effect.y = (float)judgeLineY;
    effect.radius = (float)(NOTE_SIZE / 2);  // 初始半径
    effect.maxRadius = (float)(NOTE_SIZE * 2);  // 最大半径
    effect.alpha = 1.0f;
    effect.color = WHITE;
    effect.active = true;

    switch (type) {
        case JudgmentType::PERFECT: effect.color = GOLD; break;
        case JudgmentType::GREAT: effect.color = BLUE; break;
        case JudgmentType::GOOD: effect.color = GREEN; break;
        case JudgmentType::MISS: effect.color = RED; break;
    }
    chart.hitEffects.push_back(effect);
}

// ============================================================================
// UpdateEffects - 更新视觉效果
// ============================================================================
// 【功能说明】
//   更新所有判定特效和击中特效的状态
// 【参数说明】
//   - chart: 谱面引用
// 【实现步骤】
//   1. 更新判定文字特效（向上移动+淡出+缩小）
//   2. 更新圆圈扩散特效（半径增大+淡出）
//   3. 移除已完成的特效
// 【判定文字动画】
//   - 每帧Y坐标减2像素（上浮效果）
//   - 每帧透明度减0.03
//   - 每帧缩放减0.02
//   - 每帧存活时间减0.03
//   - 存活时间<=0时标记为非活跃
// 【圆圈扩散动画】
//   - 每帧半径增3像素
//   - 每帧透明度减0.05
//   - 透明度<=0或半径>=最大半径时标记为非活跃
// ============================================================================
void UpdateEffects(Chart& chart) {
    // 更新判定文字特效
    for (auto& effect : chart.judgmentEffects) {
        if (effect.active) {
            effect.y -= 2.0f;          // 向上移动
            effect.alpha -= 0.03f;      // 淡出
            effect.scale -= 0.02f;     // 缩小
            effect.lifetime -= 0.03f;  // 存活时间递减
            if (effect.lifetime <= 0) effect.active = false;
        }
    }

    // 更新圆圈扩散特效
    for (auto& effect : chart.hitEffects) {
        if (effect.active) {
            effect.radius += 3.0f;     // 半径增大
            effect.alpha -= 0.05f;      // 淡出
            if (effect.alpha <= 0 || effect.radius >= effect.maxRadius) {
                effect.active = false;
            }
        }
    }

    // 移除非活跃的判定特效
    auto judgmentIt = chart.judgmentEffects.begin();
    while (judgmentIt != chart.judgmentEffects.end()) {
        if (!judgmentIt->active) judgmentIt = chart.judgmentEffects.erase(judgmentIt);
        else ++judgmentIt;
    }

    // 移除非活跃的圆圈特效
    auto hitIt = chart.hitEffects.begin();
    while (hitIt != chart.hitEffects.end()) {
        if (!hitIt->active) hitIt = chart.hitEffects.erase(hitIt);
        else ++hitIt;
    }
}

// ============================================================================
// DrawEffects - 绘制视觉效果
// ============================================================================
// 【功能说明】
//   绘制所有判定特效和击中特效
// 【参数说明】
//   - chart: 谱面引用
//   - offsetX, offsetY: 绘制偏移量（用于画面震动效果）
// 【实现步骤】
//   1. 绘制圆圈扩散特效（在文字下方）
//   2. 绘制判定文字特效
// ============================================================================
void DrawEffects(const Chart& chart, int offsetX, int offsetY) {
    // 绘制圆圈扩散特效
    for (const auto& effect : chart.hitEffects) {
        if (effect.active) {
            DrawCircleLines(
                (int)(effect.x + (float)offsetX),
                (int)(effect.y + (float)offsetY),
                (int)effect.radius,
                Fade(effect.color, effect.alpha)
            );
        }
    }

    // 绘制判定文字特效
    for (const auto& effect : chart.judgmentEffects) {
        if (effect.active) {
            int fontSize = (int)(20.0f * effect.scale);  // 根据缩放计算字号
            const char* text = effect.text.c_str();
            int textWidth = MeasureText(text, fontSize);
            DrawText(text,
                (int)(effect.x - (float)(textWidth / 2) + (float)offsetX),  // 居中
                (int)(effect.y + (float)offsetY),
                fontSize,
                Fade(effect.color, effect.alpha)
            );
        }
    }
}

// ============================================================================
// GetParticleColor - 获取粒子颜色
// ============================================================================
// 【功能说明】
//   根据轨道号和随机变化量生成粒子颜色
// 【参数说明】
//   - lane: 轨道号（0-3）
//   - variation: 颜色变化量（0.0-1.0）
// 【颜色方案】
//   - 轨道0：橙红色系
//   - 轨道1：蓝紫色系
//   - 轨道2：绿蓝色系
//   - 轨道3：紫红色系
// ============================================================================
static Color GetParticleColor(int lane, float variation) {
    switch (lane) {
        case 0: return MakeRGB((int)(200 + 55 * variation), (int)(80 + 60 * variation), (int)(30 + 30 * variation));
        case 1: return MakeRGB((int)(60 + 50 * variation), (int)(150 + 80 * variation), (int)(220 + 35 * variation));
        case 2: return MakeRGB((int)(50 + 40 * variation), (int)(200 + 55 * variation), (int)(150 + 60 * variation));
        case 3: return MakeRGB((int)(180 + 75 * variation), (int)(50 + 50 * variation), (int)(220 + 35 * variation));
        default: return MakeRGB((int)(180 + 75 * variation), (int)(150 + 80 * variation), (int)(255 * variation));
    }
}

// ============================================================================
// SpawnParticles - 生成粒子效果
// ============================================================================
// 【功能说明】
//   当玩家击中音符时，生成粒子爆炸效果
// 【参数说明】
//   - gameState: 游戏状态引用
//   - lane: 轨道号
//   - laneWidth: 单轨道宽度
//   - judgeLineY: 判定线Y坐标
//   - type: 判定类型
// 【粒子数量】
//   - PERFECT: 20个
//   - GREAT: 15个
//   - GOOD: 10个
//   - MISS: 0个
// 【实现步骤】
//   1. 根据判定类型决定粒子数量
//   2. 查找空闲的粒子槽位
//   3. 初始化粒子位置（在判定线附近±20像素）
//   4. 计算随机方向和速度
//   5. 设置存活时间和颜色
// 【关键算法】
//   速度方向：随机角度（0-360度）
//   速度大小：100-300像素/秒
//   Y方向初速度：-50（向上偏移产生爆炸感）
// ============================================================================
void SpawnParticles(GameState& gameState, int lane, int laneWidth, int judgeLineY, JudgmentType type) {
    int particleCount = 0;
    switch (type) {
        case JudgmentType::PERFECT: particleCount = 20; break;
        case JudgmentType::GREAT: particleCount = 15; break;
        case JudgmentType::GOOD: particleCount = 10; break;
        case JudgmentType::MISS: particleCount = 0; break;
    }

    float centerX = (float)(lane * laneWidth + laneWidth / 2);
    float centerY = (float)judgeLineY;

    for (int i = 0; i < particleCount; i++) {
        // 查找空闲粒子槽位
        Particle* p = nullptr;
        for (int j = 0; j < MAX_PARTICLES; j++) {
            if (!gameState.particles[j].active) {
                p = &gameState.particles[j];
                break;
            }
        }
        if (p == nullptr) continue;

        // 位置：在判定线附近随机偏移
        p->x = centerX + (float)((rand() % 40) - 20);
        p->y = centerY + (float)((rand() % 40) - 20);

        // 随机方向和速度
        float angle = (float)(rand() % 360) * PI / 180.0f;
        float speed = 100.0f + (float)(rand() % 200);
        p->vx = cosf(angle) * speed;
        p->vy = sinf(angle) * speed - 50.0f;  // 向上偏移

        // 大小和存活时间
        p->size = 3.0f + (float)(rand() % 6);
        p->alpha = 1.0f;
        p->active = true;
        p->life = 0.5f + (float)(rand() % 10) / 20.0f;
        p->maxLife = p->life;

        // 颜色
        float colorVariation = (float)(rand() % 100) / 100.0f;
        p->color = GetParticleColor(lane, colorVariation);
    }
}

// ============================================================================
// UpdateParticles - 更新粒子状态
// ============================================================================
// 【功能说明】
//   更新所有粒子的物理状态
// 【参数说明】
//   - gameState: 游戏状态引用
//   - deltaTime: 时间增量
// 【实现步骤】
//   1. 更新位置：x += vx*dt, y += vy*dt
//   2. 添加重力：vy += 300*dt
//   3. 递减透明度和大小
//   4. 递减存活时间
//   5. 存活时间<=0或透明度<=0时标记为非活跃
// 【关键算法】
//   重力加速度：300像素/秒²
//   透明度递减：alpha -= deltaTime / maxLife
//   大小递减：size -= deltaTime * 3，最小0.5
// ============================================================================
void UpdateParticles(GameState& gameState, float deltaTime) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!gameState.particles[i].active) continue;

        Particle* p = &gameState.particles[i];

        // 更新位置
        p->x += p->vx * deltaTime;
        p->y += p->vy * deltaTime;
        // 添加重力
        p->vy += 300.0f * deltaTime;
        // 递减透明度和大小
        p->alpha -= deltaTime / p->maxLife;
        p->size -= deltaTime * 3.0f;
        if (p->size < 0.5f) p->size = 0.5f;
        // 递减存活时间
        p->life -= deltaTime;

        if (p->life <= 0.0f || p->alpha <= 0.0f) {
            p->active = false;
        }
    }
}

// ============================================================================
// DrawParticles - 绘制粒子
// ============================================================================
// 【功能说明】
//   绘制所有活跃的粒子
// 【参数说明】
//   - gameState: 游戏状态引用
//   - offsetX, offsetY: 绘制偏移量
// ============================================================================
void DrawParticles(const GameState& gameState, int offsetX, int offsetY) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!gameState.particles[i].active) continue;
        const Particle* p = &gameState.particles[i];
        DrawCircle((int)(p->x + offsetX), (int)(p->y + offsetY), (int)p->size, Fade(p->color, p->alpha));
    }
}

// ============================================================================
// InitBgParticles - 初始化背景粒子
// ============================================================================
// 【功能说明】
//   初始化背景装饰粒子
// 【参数说明】
//   - gameState: 游戏状态引用
// 【实现步骤】
//   1. 随机位置布满整个屏幕
//   2. 随机速度和大小
//   3. 随机透明度（0.2-0.6）
//   4. 随机颜色（5种预设颜色）
// ============================================================================
void InitBgParticles(GameState& gameState) {
    for (int i = 0; i < 50; i++) {
        BgParticle* p = &gameState.bgParticles[i];
        p->active = true;
        p->x = (float)(rand() % 800);
        p->y = (float)(rand() % 600);
        p->speed = 10.0f + (float)(rand() % 30);  // 10-40像素/秒
        p->size = 1.0f + (float)(rand() % 3);     // 1-4像素
        p->alpha = 0.2f + (float)(rand() % 40) / 100.0f;  // 0.2-0.6
        p->alphaSpeed = 0.1f + (float)(rand() % 20) / 100.0f;  // 0.1-0.3

        int colorType = rand() % 5;
        switch (colorType) {
            case 0: p->color = MakeRGBA(255, 255, 255, 255); break;   // 白色
            case 1: p->color = MakeRGBA(150, 180, 255, 255); break;  // 浅蓝
            case 2: p->color = MakeRGBA(100, 200, 255, 255); break;  // 天蓝
            case 3: p->color = MakeRGBA(200, 150, 255, 255); break;  // 浅紫
            case 4: p->color = MakeRGBA(150, 220, 200, 255); break;  // 青绿
        }
    }
}

// ============================================================================
// UpdateBgParticles - 更新背景粒子
// ============================================================================
// 【功能说明】
//   更新背景装饰粒子的状态
// 【参数说明】
//   - gameState: 游戏状态引用
//   - deltaTime: 时间增量
// 【实现步骤】
//   1. 向上移动：y -= speed*dt
//   2. 水平摆动：x += sin(y*0.02)*0.3
//   3. 透明度闪烁：alpha += alphaSpeed*dt，接近边界时反转方向
//   4. 出界重置：从屏幕底部重新出现
// 【关键算法】
//   透明度变化：当alpha>0.7或alpha<0.1时反转alphaSpeed方向
//   水平摆动：使用sin函数产生波浪效果
// ============================================================================
void UpdateBgParticles(GameState& gameState, float deltaTime) {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    for (int i = 0; i < 50; i++) {
        BgParticle* p = &gameState.bgParticles[i];
        if (!p->active) continue;

        // 向上移动
        p->y -= p->speed * deltaTime;
        // 水平摆动（波浪效果）
        p->x += sinf(p->y * 0.02f) * 0.3f;

        // 透明度闪烁
        p->alpha += p->alphaSpeed * deltaTime;
        if (p->alpha > 0.7f) {
            p->alphaSpeed = -fabsf(p->alphaSpeed);  // 反转（向下）
        } else if (p->alpha < 0.1f) {
            p->alphaSpeed = fabsf(p->alphaSpeed);   // 反转（向上）
        }

        // 出界重置
        if (p->y < -10) {
            p->y = (float)(screenH + 10);
            p->x = (float)(rand() % screenW);
        }
    }
}

// ============================================================================
// DrawBgParticles - 绘制背景粒子
// ============================================================================
// 【功能说明】
//   绘制背景装饰粒子（双层圆形，外层较暗）
// 【参数说明】
//   - gameState: 游戏状态引用
//   - offsetX, offsetY: 绘制偏移量
// ============================================================================
void DrawBgParticles(const GameState& gameState, int offsetX, int offsetY) {
    for (int i = 0; i < 50; i++) {
        const BgParticle* p = &gameState.bgParticles[i];
        if (!p->active) continue;

        Color c = p->color;
        c.a = (unsigned char)(p->alpha * 255);

        int x = (int)p->x + offsetX;
        int y = (int)p->y + offsetY;

        // 外层：较大、较暗
        DrawCircle(x, y, p->size + 1, Fade(c, p->alpha * 0.3f));
        // 内层：正常大小
        DrawCircle(x, y, (int)p->size, c);
    }
}