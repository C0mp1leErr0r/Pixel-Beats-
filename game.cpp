// ============================================================================
// game.cpp - 游戏核心逻辑实现
// ============================================================================
// 本文件实现游戏的游戏核心逻辑，包括：
//   - 游戏主循环（音符下落、判定处理）
//   - 玩家输入检测
//   - 音符判定系统
//   - 分数和连击计算
//   - 游戏结束判定和成绩保存
// ============================================================================

#include "game.h"
#include "effects.h"
#include "audio.h"

// ============================================================================
// UpdateGame - 游戏逻辑更新函数
// ============================================================================
// 【功能说明】
//   游戏主循环的核心函数，每帧调用一次，更新游戏中所有元素的状态
// 【参数说明】
//   - chart: 谱面引用，包含所有音符信息
//   - gameState: 游戏状态引用，记录分数、连击等
//   - music: 音乐引用，用于检测音乐是否结束
//   - deltaTime: 距离上一帧的时间（秒），用于平滑动画
// 【实现步骤】
//   1. 如果游戏未开始或已暂停，直接返回
//   2. 更新屏幕震动效果
//   3. 更新按键按下状态
//   4. 更新视觉特效和粒子系统
//   5. 衰减轨道闪光效果
//   6. 遍历所有音符，更新下落位置并检测漏接（MISS）
//   7. 检查游戏结束条件并处理结束逻辑
// 【关键算法】
//   - 音符下落：每帧y += NOTE_SPEED * noteSpeed
//   - MISS判定阈值：missThreshold = GOOD_TIME * speed * 60 / 1000
//   - 游戏结束条件：所有音符已判定 && 音乐停止播放
//   - 准确率计算：accuracy = (perfect*100 + great*80 + good*50) / totalNotes
// ============================================================================
void UpdateGame(Chart& chart, GameState& gameState, Music* music, float deltaTime) {
    if (!gameState.isPlaying || gameState.isPaused) return;

    int currentJudgeLineY = GetScreenHeight() - 100;
    int currentLaneWidth = GetScreenWidth() / LANE_COUNT;

    UpdateScreenShake(gameState);
    UpdateKeyPressStates(gameState);
    UpdateEffects(chart);
    UpdateParticles(gameState, deltaTime);
    UpdateBgParticles(gameState, deltaTime);

    // 轨道闪光衰减
    for (int i = 0; i < 4; i++) {
        if (gameState.laneFlash[i] > 0.0f) {
            gameState.laneFlash[i] -= deltaTime * 4.0f;
            if (gameState.laneFlash[i] < 0.0f) gameState.laneFlash[i] = 0.0f;
        }
    }

    // 遍历谱面中的所有音符，更新下落位置
    for (auto& note : chart.notes) {
        if (note.active && !note.judged) {
            float currentSpeed = NOTE_SPEED * gameState.noteSpeed;
            // 【音符下落核心】每帧更新Y坐标（数值增大=向下移动）
            // 注意：这里没有乘以deltaTime，假设游戏以60FPS运行
            note.y += currentSpeed;

            // 【MISS判定】计算漏接阈值并检查是否漏接
            float missThreshold = (float)GOOD_TIME * currentSpeed * 60.0f / 1000.0f;
            if (note.y > currentJudgeLineY + missThreshold) {
                note.judged = true;
                note.active = false;

                if (gameState.combo > gameState.maxCombo) {
                    gameState.maxCombo = gameState.combo;
                }

                gameState.combo = 0;
                gameState.missCount++;

                SpawnJudgmentEffect(chart, JudgmentType::MISS, note.lane, currentLaneWidth, currentJudgeLineY);
                SpawnHitEffect(chart, JudgmentType::MISS, note.lane, currentLaneWidth, currentJudgeLineY);
                PlayHitSoundDirect(JudgmentType::MISS, gameState);
            }
        }
    }

    // 检查所有音符是否都已处理完毕
    bool allJudged = true;
    for (const auto& note : chart.notes) {
        if (note.active && !note.judged) {
            allJudged = false;
            break;
        }
    }

    // 游戏结束条件：所有音符已判定 且 音乐停止播放
    if (allJudged && chart.notes.size() > 0 && (music == nullptr || !IsMusicStreamPlaying(*music))) {
        gameState.isPlaying = false;
        gameState.gameOver = true;

        // 计算最终成绩
        int totalNotes = gameState.perfectCount + gameState.greatCount + gameState.goodCount + gameState.missCount;
        float accuracy = 0.0f;
        if (totalNotes > 0) {
            accuracy = (float)(gameState.perfectCount * 100 + gameState.greatCount * 80 + gameState.goodCount * 50) / (float)totalNotes;
        }
        char grade = GetGrade(accuracy);

        // 保存成绩（只有打破记录才保存）
        BestRecord existingRecord = LoadBestRecord(gameState.selectedMusicIndex, gameState.currentDifficulty);
        if (gameState.score > existingRecord.score) {
            BestRecord record;
            record.score = gameState.score;
            record.maxCombo = gameState.maxCombo;
            record.accuracy = accuracy;
            record.grade = grade;
            SaveBestRecord(gameState.selectedMusicIndex, gameState.currentDifficulty, record);
        }
    }
}

// ============================================================================
// HandleInput - 处理玩家按键输入
// ============================================================================
// 【功能说明】
//   检测玩家的按键输入，调用CheckNote进行判定
// 【参数说明】
//   - chart: 谱面引用
//   - gameState: 游戏状态引用
// 【实现步骤】
//   1. 使用static数组keyDown[4]记录按键状态，防止重复触发
//   2. 检测D/F/J/K四个按键的状态
//   3. 如果按键刚按下（keyDown=false变为true），调用CheckNote检测该轨道
//   4. 检测暂停按键（P或ESCAPE键）
// 【关键算法】
//   - 使用static变量确保数组在函数调用之间保持状态
//   - 按键检测：if (pressed && !keyDown[i]) 防止按住键时重复触发
//   - 暂停功能：isPlaying && !gameOver 时切换暂停状态
// ============================================================================
void HandleInput(Chart& chart, GameState& gameState) {
    // static确保数组在函数调用之间保持状态
    static bool keyDown[4] = {false, false, false, false};

    // Lane 0
    bool lane0Pressed = IsKeyDown((KeyboardKey)gameState.keyBindings[0]);
    if (lane0Pressed && !keyDown[0]) {
        keyDown[0] = true;
        CheckNote(chart, gameState, 0);
    } else if (!lane0Pressed) {
        keyDown[0] = false;
    }

    // Lane 1
    bool lane1Pressed = IsKeyDown((KeyboardKey)gameState.keyBindings[1]);
    if (lane1Pressed && !keyDown[1]) {
        keyDown[1] = true;
        CheckNote(chart, gameState, 1);
    } else if (!lane1Pressed) {
        keyDown[1] = false;
    }

    // Lane 2
    bool lane2Pressed = IsKeyDown((KeyboardKey)gameState.keyBindings[2]);
    if (lane2Pressed && !keyDown[2]) {
        keyDown[2] = true;
        CheckNote(chart, gameState, 2);
    } else if (!lane2Pressed) {
        keyDown[2] = false;
    }

    // Lane 3
    bool lane3Pressed = IsKeyDown((KeyboardKey)gameState.keyBindings[3]);
    if (lane3Pressed && !keyDown[3]) {
        keyDown[3] = true;
        CheckNote(chart, gameState, 3);
    } else if (!lane3Pressed) {
        keyDown[3] = false;
    }

    // 暂停功能
    if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE)) {
        if (gameState.isPlaying && !gameState.gameOver) {
            gameState.isPaused = !gameState.isPaused;
        }
    }
}

// ============================================================================
// CheckNote - 音符判定函数
// ============================================================================
// 【功能说明】
//   判定系统的核心函数，检查玩家按键是否成功击中音符
// 【参数说明】
//   - chart: 谱面引用
//   - gameState: 游戏状态引用
//   - lane: 轨道号（0-3）
// 【实现步骤】
//   1. 在指定轨道上找到距离判定线最近的未判定音符
//   2. 计算按键时间与音符时间的时间差（毫秒）
//   3. 根据时间差判断命中精度
//   4. 如果命中：增加分数、增加连击、标记音符、生成特效、播放音效
//   5. 更新最大连击数
// 【关键算法】
//   - 距离计算：distance = abs(note.y - JUDGE_LINE_Y)
//   - 时间差转换：timeDiff = distance / speed * 1000 / 60
//   - 判定阈值：PERFECT=50ms, GREAT=100ms, GOOD=120ms
// 【判定映射】
//   - PERFECT: 1000分，金色特效，强震动
//   - GREAT: 500分，蓝色特效，轻震动
//   - GOOD: 100分，绿色特效，轻震动
//   - 连击30的倍数：额外触发强震动
// ============================================================================
void CheckNote(Chart& chart, GameState& gameState, int lane) {
    int currentJudgeLineY = GetScreenHeight() - 100;
    int currentLaneWidth = GetScreenWidth() / LANE_COUNT;

    Note* closestNote = nullptr;
    float closestDistance = 999999.0f;

    // 遍历该轨道上所有音符，寻找最近的
    for (auto& note : chart.notes) {
        if (note.lane == lane && note.active && !note.judged) {
            float distance = (float)abs((int)(note.y - currentJudgeLineY));
            if (distance < closestDistance) {
                closestDistance = distance;
                closestNote = &note;
            }
        }
    }

    if (closestNote != nullptr) {
        float currentSpeed = NOTE_SPEED * gameState.noteSpeed;
        // 【核心计算】将像素距离转换为时间差（毫秒）
        // 公式：距离/速度×1000/60
        // 含义：如果现在按键，还要多久音符才到判定线
        float timeDiff = closestDistance / currentSpeed * 1000.0f / 60.0f;
        float hitThreshold = (float)GOOD_TIME * currentSpeed * 60.0f / 1000.0f + NOTE_SIZE;

        if (closestDistance <= hitThreshold) {
            // PERFECT判定：时间差<=50ms，得1000分
            if (timeDiff <= PERFECT_TIME) {
                gameState.score += 1000;
                gameState.combo++;
                gameState.perfectCount++;
                closestNote->judged = true;
                closestNote->active = false;
                SpawnJudgmentEffect(chart, JudgmentType::PERFECT, lane, currentLaneWidth, currentJudgeLineY);
                SpawnHitEffect(chart, JudgmentType::PERFECT, lane, currentLaneWidth, currentJudgeLineY);
                PlayHitSoundDirect(JudgmentType::PERFECT, gameState);
                gameState.laneFlash[lane] = 1.0f;
                SpawnParticles(gameState, lane, currentLaneWidth, currentJudgeLineY, JudgmentType::PERFECT);
                if (gameState.screenShake == 0) {
                    gameState.screenShake = 0.3f;
                }
                if (gameState.combo > 0 && gameState.combo % 30 == 0) {
                    gameState.screenShake = 4.0f;
                }
            }
            // GREAT判定：50ms<时间差<=100ms，得500分
            else if (timeDiff <= GREAT_TIME) {
                gameState.score += 500;
                gameState.combo++;
                gameState.greatCount++;
                closestNote->judged = true;
                closestNote->active = false;
                SpawnJudgmentEffect(chart, JudgmentType::GREAT, lane, currentLaneWidth, currentJudgeLineY);
                SpawnHitEffect(chart, JudgmentType::GREAT, lane, currentLaneWidth, currentJudgeLineY);
                PlayHitSoundDirect(JudgmentType::GREAT, gameState);
                gameState.laneFlash[lane] = 0.8f;
                SpawnParticles(gameState, lane, currentLaneWidth, currentJudgeLineY, JudgmentType::GREAT);
                if (gameState.screenShake == 0) {
                    gameState.screenShake = 0.3f;
                }
                if (gameState.combo > 0 && gameState.combo % 30 == 0) {
                    gameState.screenShake = 4.0f;
                }
            }
            // GOOD判定：100ms<时间差<=120ms，得100分
            else if (timeDiff <= GOOD_TIME) {
                gameState.score += 100;
                gameState.combo++;
                gameState.goodCount++;
                closestNote->judged = true;
                closestNote->active = false;
                SpawnJudgmentEffect(chart, JudgmentType::GOOD, lane, currentLaneWidth, currentJudgeLineY);
                SpawnHitEffect(chart, JudgmentType::GOOD, lane, currentLaneWidth, currentJudgeLineY);
                PlayHitSoundDirect(JudgmentType::GOOD, gameState);
                gameState.laneFlash[lane] = 0.6f;
                SpawnParticles(gameState, lane, currentLaneWidth, currentJudgeLineY, JudgmentType::GOOD);
                if (gameState.screenShake == 0) {
                    gameState.screenShake = 0.3f;
                }
                if (gameState.combo > 0 && gameState.combo % 30 == 0) {
                    gameState.screenShake = 4.0f;
                }
            }
        }

        if (gameState.combo > gameState.maxCombo) {
            gameState.maxCombo = gameState.combo;
        }
    }
}

// ============================================================================
// UpdateScreenShake - 更新屏幕震动状态
// ============================================================================
// 【功能说明】
//   每帧递减屏幕震动强度，实现震动衰减效果
// 【参数说明】
//   - gameState: 游戏状态引用
// 【关键算法】
//   - 震动衰减：screenShake *= 0.9（每帧变为90%）
//   - 震动归零：当screenShake < 0.5时重置为0
// ============================================================================
void UpdateScreenShake(GameState& gameState) {
    if (gameState.screenShake > 0) {
        gameState.screenShake *= 0.9f;
        if (gameState.screenShake < 0.5f) gameState.screenShake = 0;
    }
}

// ============================================================================
// UpdateKeyPressStates - 更新按键按下状态
// ============================================================================
// 【功能说明】
//   更新GameState中的keyPressed数组，记录四个按键的当前状态
// 【参数说明】
//   - gameState: 游戏状态引用
// 【使用场景】
//   用于UI绘制时显示按键是否被按下（按键高亮效果）
// ============================================================================
void UpdateKeyPressStates(GameState& gameState) {
    gameState.keyPressed[0] = IsKeyDown((KeyboardKey)gameState.keyBindings[0]);
    gameState.keyPressed[1] = IsKeyDown((KeyboardKey)gameState.keyBindings[1]);
    gameState.keyPressed[2] = IsKeyDown((KeyboardKey)gameState.keyBindings[2]);
    gameState.keyPressed[3] = IsKeyDown((KeyboardKey)gameState.keyBindings[3]);
}

// ============================================================================
// GetGrade - 根据准确率计算评级
// ============================================================================
// 【功能说明】
//   根据游戏结束时的准确率计算最终评级
// 【参数说明】
//   - accuracy: 准确率（0.0-100.0）
// 【返回值】
//   评级字符（S/A/B/C/D）
// 【评级规则】
//   - S级：准确率>=95%
//   - A级：准确率>=90%
//   - B级：准确率>=80%
//   - C级：准确率>=70%
//   - D级：准确率<70%
// ============================================================================
char GetGrade(float accuracy) {
    if (accuracy >= 95.0f) return 'S';
    if (accuracy >= 90.0f) return 'A';
    if (accuracy >= 80.0f) return 'B';
    if (accuracy >= 70.0f) return 'C';
    return 'D';
}