// ============================================================================
// effects.h - 视觉效果声明
// ============================================================================
// 本模块处理所有视觉效果：
//   - 判定文字特效（PERFECT/GREAT/GOOD/MISS）
//   - 击中圆圈扩散特效
//   - 粒子系统（击中爆炸效果）
//   - 背景装饰粒子
// ============================================================================
// 【模块关系】
//   - effects.cpp实现视觉效果
//   - 被game.cpp中的UpdateGame调用UpdateEffects/UpdateParticles
//   - 被main.cpp中的DrawGame调用DrawEffects/DrawParticles/DrawBgParticles
//   - 依赖structs.h中的Particle、JudgmentEffect、HitEffect、BgParticle结构体
//   - 依赖constants.h中的JudgmentType枚举和NOTE_SIZE等常量
// ============================================================================

#ifndef EFFECTS_H
#define EFFECTS_H

#include "raylib.h"
#include "constants.h"
#include "structs.h"

// ============================================================================
// 判定特效
// ============================================================================
// 【功能说明】生成和更新击中时的判定文字和圆圈扩散特效
// 【特效类型】
//   - JudgmentEffect: "PERFECT"/"GREAT"/"GOOD"/"MISS"文字
//   - HitEffect: 判定线位置的圆圈扩散动画
// 【颜色映射】
//   - PERFECT: GOLD (金黄色)
//   - GREAT: BLUE (蓝色)
//   - GOOD: GREEN (绿色)
//   - MISS: RED (红色)

// 生成判定文字效果
// 【功能】在判定线上方生成判定文字
// 【参数】
//   - chart: 谱面引用（用于存储特效）
//   - type: 判定类型（PERFECT/GREAT/GOOD/MISS）
//   - lane: 轨道号（0-3）
//   - laneWidth: 单轨道宽度
//   - judgeLineY: 判定线Y坐标
// 【特效属性】
//   - 位置：判定线上方60像素，轨道中心
//   - 初始透明度：1.0，存活时间：1秒
//   - 初始缩放：1.5倍，每帧缩小0.02
void SpawnJudgmentEffect(Chart& chart, JudgmentType type, int lane, int laneWidth, int judgeLineY);

// 生成圆圈扩散效果
// 【功能】在判定线位置生成圆圈扩散动画
// 【参数】
//   - chart: 谱面引用
//   - type: 判定类型
//   - lane: 轨道号
//   - laneWidth: 单轨道宽度
//   - judgeLineY: 判定线Y坐标
// 【动画参数】
//   - 初始半径：NOTE_SIZE/2（约20像素）
//   - 最大半径：NOTE_SIZE*2（约80像素）
//   - 每帧半径增3像素，透明度减0.05
void SpawnHitEffect(Chart& chart, JudgmentType type, int lane, int laneWidth, int judgeLineY);

// 更新视觉效果
// 【功能】更新所有判定特效和击中特效的状态
// 【参数】chart: 谱面引用
// 【实现步骤】
//   1. 更新判定文字（向上移动+淡出+缩小）
//   2. 更新圆圈扩散（半径增大+淡出）
//   3. 移除非活跃特效
void UpdateEffects(Chart& chart);

// 绘制视觉效果
// 【功能】绘制所有判定特效和击中特效
// 【参数】
//   - chart: 谱面引用
//   - offsetX, offsetY: 绘制偏移量（用于画面震动效果）
void DrawEffects(const Chart& chart, int offsetX, int offsetY);

// ============================================================================
// 粒子系统
// ============================================================================
// 【功能说明】生成和更新击中时的粒子爆炸效果
// 【粒子数量】
//   - PERFECT: 20个粒子
//   - GREAT: 15个粒子
//   - GOOD: 10个粒子
//   - MISS: 0个粒子

// 生成粒子爆炸效果
// 【功能】当玩家击中音符时，生成粒子爆炸效果
// 【参数】
//   - gameState: 游戏状态引用
//   - lane: 轨道号
//   - laneWidth: 单轨道宽度
//   - judgeLineY: 判定线Y坐标
//   - type: 判定类型
// 【实现步骤】
//   1. 根据判定类型决定粒子数量
//   2. 查找空闲的粒子槽位
//   3. 初始化粒子位置（在判定线附近±20像素）
//   4. 计算随机方向和速度
//   5. 设置存活时间和颜色
void SpawnParticles(GameState& gameState, int lane, int laneWidth, int judgeLineY, JudgmentType type);

// 更新粒子状态
// 【功能】更新所有粒子的物理状态
// 【参数】
//   - gameState: 游戏状态引用
//   - deltaTime: 时间增量
// 【实现步骤】
//   1. 更新位置：x += vx*dt, y += vy*dt
//   2. 添加重力：vy += 300*dt
//   3. 递减透明度和大小
//   4. 递减存活时间
//   5. 存活时间<=0或透明度<=0时标记为非活跃
void UpdateParticles(GameState& gameState, float deltaTime);

// 绘制粒子
// 【功能】绘制所有活跃的粒子
// 【参数】
//   - gameState: 游戏状态引用
//   - offsetX, offsetY: 绘制偏移量
void DrawParticles(const GameState& gameState, int offsetX, int offsetY);

// ============================================================================
// 背景粒子
// ============================================================================
// 【功能说明】主菜单和音乐选择界面的装饰性漂浮粒子

// 初始化背景粒子
// 【功能】初始化背景装饰粒子
// 【参数】gameState: 游戏状态引用
// 【实现步骤】
//   1. 随机位置布满整个屏幕
//   2. 随机速度和大小
//   3. 随机透明度（0.2-0.6）
//   4. 随机颜色（5种预设颜色）
void InitBgParticles(GameState& gameState);

// 更新背景粒子
// 【功能】更新背景装饰粒子的状态
// 【参数】
//   - gameState: 游戏状态引用
//   - deltaTime: 时间增量
// 【实现步骤】
//   1. 向上移动：y -= speed*dt
//   2. 水平摆动：x += sin(y*0.02)*0.3
//   3. 透明度闪烁：alpha += alphaSpeed*dt
//   4. 出界重置：从屏幕底部重新出现
void UpdateBgParticles(GameState& gameState, float deltaTime);

// 绘制背景粒子
// 【功能】绘制背景装饰粒子（双层圆形，外层较暗）
// 【参数】
//   - gameState: 游戏状态引用
//   - offsetX, offsetY: 绘制偏移量
void DrawBgParticles(const GameState& gameState, int offsetX, int offsetY);

#endif // EFFECTS_H
