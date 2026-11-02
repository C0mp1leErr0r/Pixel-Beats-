// ============================================================================
// chart.cpp - 谱面加载和管理功能实现
// ============================================================================
// 本文件实现谱面编辑器的核心功能，包括：
//   - 谱面文件加载和保存
//   - 音符的增删改查
//   - 编辑器状态管理和输入处理
// ============================================================================

#include "chart.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <sstream>

#include "assets.h"   // 内嵌资源加载（单文件 exe 支持）

// 声明外部全局音乐列表（定义在main.cpp中）
extern std::vector<MusicInfo> musicList;

// ============================================================================
// LoadChart - 加载谱面（游戏模式用）
// ============================================================================
// 【功能说明】
//   根据难度加载对应谱面文件，解析出所有音符信息
// 【参数说明】
//   - diff: 难度等级（EASY/NORMAL/HARD）
//   - filePrefix: 谱面文件基础名（如"song1"，不含难度后缀）
// 【返回值】
//   Chart结构体，包含音乐路径、偏移量、所有音符
// 【文件路径格式】
//   {chartFile}_{difficulty}.txt
//   例如：song1_easy.txt, song1_normal.txt, song1_hard.txt
// 【文件格式】
//   music:音乐文件路径
//   offset:时间偏移量（毫秒）
//   lane,time（每行一个音符）
// 【实现步骤】
//   1. 拼接文件名：chartFile + "_" + difficulty + ".txt"
//   2. 打开文件，如果失败返回空的Chart
//   3. 逐行读取文件
//   4. 遇到"music:"行，记录音乐路径
//   5. 遇到"offset:"行，记录偏移量
//   6. 遇到其他行（逗号分隔），解析为音符
//   7. 为每个音符计算初始Y位置
//   8. 设置音符active=true, judged=false
//   9. 关闭文件，返回完整的Chart对象
// 【关键算法】
//   Y位置计算：y = JUDGE_LINE_Y - (hitTime / 1000) * NOTE_SPEED * 60
//   确保音符在正确的时间到达判定线
// ============================================================================
Chart LoadChart(Difficulty diff, const char* filePrefix) {
    Chart data;
    data.difficulty = diff;
    data.offset = 0.0f;
    data.judgmentEffects.clear();
    data.hitEffects.clear();

    const char* baseName = nullptr;

    // 步骤1：拼接文件名
    if (filePrefix != nullptr && filePrefix[0] != '\0') {
        static char fullPath[256];
        if (diff == Difficulty::EASY) {
            snprintf(fullPath, sizeof(fullPath), "%s_easy.txt", filePrefix);
        } else if (diff == Difficulty::HARD) {
            snprintf(fullPath, sizeof(fullPath), "%s_hard.txt", filePrefix);
        } else {
            snprintf(fullPath, sizeof(fullPath), "%s_normal.txt", filePrefix);
        }
        baseName = fullPath;
    } else {
        if (diff == Difficulty::EASY) {
            baseName = "chart_easy.txt";
        } else if (diff == Difficulty::HARD) {
            baseName = "chart_hard.txt";
        } else {
            baseName = "chart_normal.txt";
        }
    }

    // 优先读取磁盘上的谱面文件；不存在时使用内嵌在 exe 中的默认谱面
    std::string chartText = LoadTextAsset(baseName);

    // 备用方案：如果找不到指定难度的文件，尝试读取chart.txt
    if (chartText.empty()) chartText = LoadTextAsset("chart.txt");
    if (chartText.empty()) return data;

    std::istringstream inputFile(chartText);
    std::string readLine;

    // 步骤2-8：逐行读取并解析
    while (std::getline(inputFile, readLine)) {
        std::istringstream parser(readLine);
        std::string token;

        // 解析music:行
        if (readLine.substr(0, 6) == "music:") {
            data.musicPath = readLine.substr(6);
        }
        // 解析offset:行
        else if (readLine.substr(0, 7) == "offset:") {
            data.offset = std::stof(readLine.substr(7));
        }
        // 解析音符行（lane,time格式）
        else {
            std::vector<std::string> tokens;
            while (std::getline(parser, token, ',')) {
                tokens.push_back(token);
            }
            // 验证：确保拆分成恰好2个分量才算有效音符
            if (tokens.size() == 2) {
                Note n;
                n.lane = std::stoi(tokens[0]);
                float timeMs = std::stof(tokens[1]);
                // 核心计算：根据音符时间计算初始Y位置
                n.y = (float)(GetScreenHeight() - 100) - (timeMs / 1000.0f) * NOTE_SPEED * 60.0f;
                n.active = true;
                n.judged = false;
                n.hitTime = timeMs;
                data.notes.push_back(n);
            }
        }
    }
    return data;
}

// ============================================================================
// SaveEditorChart - 保存谱面到文件
// ============================================================================
// 【功能说明】
//   将编辑器中的谱面数据保存到txt文件
// 【参数说明】
//   - filePath: 目标文件路径
//   - state: 编辑器状态（包含音符列表等）
// 【文件格式】
//   music: 音乐路径
//   offset: 偏移量
//   lane,time（每行一个音符，按时间排序）
// 【实现步骤】
//   1. 打开文件
//   2. 写入music:和offset:行
//   3. 按时间排序所有音符
//   4. 逐行写入lane,time
// ============================================================================
void SaveEditorChart(const std::string& filePath, const EditorState& state) {
    std::ofstream file(filePath);
    if (!file.is_open()) return;

    file << "music: " << state.musicPath << "\n";
    file << "offset: " << state.offset << "\n";

    // 按时间排序后保存
    std::vector<EditorNote> sortedNotes = state.notes;
    std::sort(sortedNotes.begin(), sortedNotes.end(), [](const EditorNote& a, const EditorNote& b) {
        return a.time < b.time;
    });

    for (const auto& note : sortedNotes) {
        file << note.lane << "," << note.time << "\n";
    }

    file.close();
}

// ============================================================================
// LoadEditorChart - 加载谱面文件到编辑器
// ============================================================================
// 【功能说明】
//   从txt文件读取谱面数据到编辑器状态
// 【参数说明】
//   - chartPath: 谱面文件路径
// 【返回值】
//   EditorState结构体，包含音符列表等
// 【与LoadChart的区别】
//   - LoadChart: 用于游戏模式，不支持音符选择状态
//   - LoadEditorChart: 用于编辑器，支持音符选择状态等额外字段
// 【实现步骤】
//   1. 打开文件
//   2. 逐行读取
//   3. 解析music:/offset:行
//   4. 解析音符行并添加到state.notes
//   5. 按时间排序
// ============================================================================
EditorState LoadEditorChart(const std::string& chartPath) {
    EditorState state;

    // 优先读取磁盘上的谱面文件；不存在时使用内嵌在 exe 中的默认谱面
    std::string chartText = LoadTextAsset(chartPath.c_str());
    std::string line;

    if (chartText.empty()) {
        return state;
    }

    std::istringstream file(chartText);
    while (std::getline(file, line)) {
        // 解析music:行
        if (line.size() >= 6 && line.substr(0, 6) == "music:") {
            state.musicPath = line.substr(6);
            // 去除首尾空格
            while (!state.musicPath.empty() && (state.musicPath.front() == ' ' || state.musicPath.front() == '\t')) {
                state.musicPath.erase(0, 1);
            }
            while (!state.musicPath.empty() && (state.musicPath.back() == ' ' || state.musicPath.back() == '\t')) {
                state.musicPath.pop_back();
            }
        }
        // 解析offset:行
        else if (line.size() >= 7 && line.substr(0, 7) == "offset:") {
            state.offset = std::stof(line.substr(7));
        }
        // 解析音符行
        else {
            std::vector<std::string> parts;
            std::istringstream iss(line);
            std::string token;
            while (std::getline(iss, token, ',')) {
                parts.push_back(token);
            }

            if (parts.size() == 2) {
                EditorNote note;
                note.lane = std::stoi(parts[0]);
                // 边界检查：确保轨道号在有效范围内
                if (note.lane < 0) note.lane = 0;
                if (note.lane >= LANE_COUNT) note.lane = LANE_COUNT - 1;
                note.time = std::stof(parts[1]);
                note.selected = false;
                state.notes.push_back(note);
            }
        }
    }

    // 按时间排序
    std::sort(state.notes.begin(), state.notes.end(), [](const EditorNote& a, const EditorNote& b) {
        return a.time < b.time;
    });
    return state;
}

// ============================================================================
// SaveEditorChartForMusic - 根据音乐信息保存谱面
// ============================================================================
// 【功能说明】
//   保存谱面并自动根据音乐信息和难度命名文件
// 【参数说明】
//   - state: 编辑器状态
//   - musicInfo: 音乐信息（包含chartFile基础名）
// 【文件名格式】
//   {musicInfo.chartFile}_{difficulty}.txt
//   例如："ARForest - Inverted World_easy.txt"
// ============================================================================
void SaveEditorChartForMusic(EditorState& state, const MusicInfo& musicInfo) {
    if (musicInfo.chartFile.empty()) return;

    const char* diffNames[] = {"easy", "normal", "hard"};
    char chartPath[256];
    snprintf(chartPath, sizeof(chartPath), "%s_%s.txt", musicInfo.chartFile.c_str(), diffNames[(int)state.selectedDifficulty]);

    std::ofstream file(chartPath);
    if (file.is_open()) {
        file << "music:" << musicInfo.filePath << "\n";
        file << "offset:" << state.offset << "\n";
        for (const auto& note : state.notes) {
            file << note.lane << "," << note.time << "\n";
        }
        file.close();
    }
}

// ============================================================================
// FindEditorNoteAtPosition - 查找指定位置的音符
// ============================================================================
// 【功能说明】
//   根据鼠标坐标查找该位置是否有音符
// 【参数说明】
//   - state: 编辑器状态
//   - mouseX: 鼠标X坐标
//   - mouseY: 鼠标Y坐标
// 【返回值】
//   音符索引，如果没找到返回-1
// 【使用场景】
//   - 编辑器中的音符选择
//   - 编辑器中的音符拖拽
// 【实现步骤】
//   1. 根据鼠标X坐标计算所在轨道
//   2. 计算当前编辑位置对应的音符显示Y坐标
//   3. 从后往前遍历音符列表，检测碰撞
// 【关键算法】
//   音符Y坐标计算：noteY = currentY + (note.time / 1000) * noteSpeed * 60
//   其中 currentY = JUDGE_LINE_Y - (gameTime / 1000) * noteSpeed * 60
// ============================================================================
int FindEditorNoteAtPosition(const EditorState& state, int mouseX, int mouseY) {
    int currentLaneWidth = GetScreenWidth() / LANE_COUNT;
    int currentJudgeLineY = GetScreenHeight() - 100;
    float currentY = currentJudgeLineY - state.gameTime * state.noteSpeed * 60.0f / 1000.0f;

    int lane = mouseX / currentLaneWidth;
    if (lane < 0 || lane >= LANE_COUNT) return -1;

    // 从后往前查找（优先选中后添加的音符）
    for (int i = (int)state.notes.size() - 1; i >= 0; i--) {
        const auto& note = state.notes[i];
        if (note.lane != lane) continue;

        float noteY = currentY + (note.time / 1000.0f) * state.noteSpeed * 60.0f;
        int noteScreenX = note.lane * currentLaneWidth + currentLaneWidth / 2 - NOTE_SIZE / 2;

        Rectangle noteRect = { (float)noteScreenX, noteY, (float)NOTE_SIZE, (float)NOTE_SIZE };
        if (CheckCollisionPointRec(Vector2{ (float)mouseX, (float)mouseY }, noteRect)) {
            return i;
        }
    }
    return -1;
}

// ============================================================================
// CanPlaceNote - 检查是否可以放置音符
// ============================================================================
// 【功能说明】
//   检查指定轨道和时间是否已经有音符（防止重叠）
// 【参数说明】
//   - state: 编辑器状态
//   - lane: 轨道号
//   - time: 时间戳
//   - excludeIndex: 排除的音符索引（用于拖拽时检查重叠）
// 【返回值】
//   true = 可以放置，false = 时间太近会重叠
// 【关键常量】
//   MIN_TIME_DIFF = 30ms：同一轨道上两个音符的最小时间差
// ============================================================================
bool CanPlaceNote(const EditorState& state, int lane, float time, int excludeIndex) {
    const float MIN_TIME_DIFF = 30.0f;  // 最小时间差（毫秒）
    for (size_t i = 0; i < state.notes.size(); i++) {
        if ((int)i == excludeIndex) continue;
        if (state.notes[i].lane == lane) {
            float diff = fabs(state.notes[i].time - time);
            if (diff < MIN_TIME_DIFF) {
                return false;  // 时间太近会重叠
            }
        }
    }
    return true;
}

// ============================================================================
// UpdateEditor - 谱面编辑器更新函数
// ============================================================================
// 【功能说明】
//   处理编辑器中的所有输入和状态更新
// 【参数说明】
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
// ============================================================================
void UpdateEditor(EditorState& state, Music& music, bool musicLoaded) {
    // 检测editorState实例变化（用于调试）
    static void* lastStatePtr = nullptr;
    if (lastStatePtr != &state) {
        lastStatePtr = &state;
    }

    float deltaTime = GetFrameTime();
    int currentScreenHeight = GetScreenHeight();
    int currentJudgeLineY = currentScreenHeight - 100;
    int currentScreenWidth;
    int currentLaneWidth;
    int progressBarY;
    int progressBarHeight;
    int progressBarMargin;

    // 更新音乐时间
    static int updateCount = 0;
    updateCount++;
    if (musicLoaded && state.isPlaying && !state.isPaused) {
        UpdateMusicStream(music);
        state.gameTime = GetMusicTimePlayed(music) * 1000.0f;

        // 音乐播放完毕自动停止
        if (state.musicLength > 0 && state.gameTime >= state.musicLength) {
            state.isPlaying = false;
            state.isPaused = true;
            state.gameTime = state.musicLength;
            if (musicLoaded) {
                PauseMusicStream(music);
            }
        }
    }

    // 暂停时用方向键调整时间
    if (musicLoaded && !state.isPlaying && IsKeyPressed(KEY_LEFT)) {
        state.gameTime -= 5000.0f;
        if (state.gameTime < 0) state.gameTime = 0;
        SeekMusicStream(music, state.gameTime / 1000.0f);
    }
    if (musicLoaded && !state.isPlaying && IsKeyPressed(KEY_RIGHT)) {
        state.gameTime += 5000.0f;
        if (state.gameTime > state.musicLength) state.gameTime = state.musicLength;
        SeekMusicStream(music, state.gameTime / 1000.0f);
    }

    // M键：切换鼠标放置模式
    if (IsKeyPressed(KEY_M)) {
        state.mousePlacementMode = !state.mousePlacementMode;
    }

    // H键：切换帮助界面
    if (IsKeyPressed(KEY_H)) {
        state.showingHelp = !state.showingHelp;
    }

    // D/F/J/K键：添加音符到对应轨道
    if (IsKeyPressed(KEY_D)) {
        float noteTime = state.gameTime;
        if (CanPlaceNote(state, 0, noteTime)) {
            EditorNote newNote;
            newNote.lane = 0;
            newNote.time = noteTime;
            newNote.selected = false;
            state.notes.push_back(newNote);
            std::sort(state.notes.begin(), state.notes.end(), [](const EditorNote& a, const EditorNote& b) {
                return a.time < b.time;
            });
        }
    }
    if (IsKeyPressed(KEY_F)) {
        float noteTime = state.gameTime;
        if (CanPlaceNote(state, 1, noteTime)) {
            EditorNote newNote;
            newNote.lane = 1;
            newNote.time = noteTime;
            newNote.selected = false;
            state.notes.push_back(newNote);
            std::sort(state.notes.begin(), state.notes.end(), [](const EditorNote& a, const EditorNote& b) {
                return a.time < b.time;
            });
        }
    }
    if (IsKeyPressed(KEY_J)) {
        float noteTime = state.gameTime;
        if (CanPlaceNote(state, 2, noteTime)) {
            EditorNote newNote;
            newNote.lane = 2;
            newNote.time = noteTime;
            newNote.selected = false;
            state.notes.push_back(newNote);
            std::sort(state.notes.begin(), state.notes.end(), [](const EditorNote& a, const EditorNote& b) {
                return a.time < b.time;
            });
        }
    }
    if (IsKeyPressed(KEY_K)) {
        float noteTime = state.gameTime;
        if (CanPlaceNote(state, 3, noteTime)) {
            EditorNote newNote;
            newNote.lane = 3;
            newNote.time = noteTime;
            newNote.selected = false;
            state.notes.push_back(newNote);
            std::sort(state.notes.begin(), state.notes.end(), [](const EditorNote& a, const EditorNote& b) {
                return a.time < b.time;
            });
        }
    }

    // 空格键：播放/暂停
    static int spacePressCount = 0;
    static bool audioDeviceChecked = false;
    if (!audioDeviceChecked) {
        audioDeviceChecked = true;
    }
    if (IsKeyPressed(KEY_SPACE)) {
        spacePressCount++;

        if (state.isPlaying) {
            // 切换暂停状态
            state.isPaused = !state.isPaused;
            if (state.isPaused && musicLoaded) {
                PauseMusicStream(music);
            }
            else if (!state.isPaused && musicLoaded) {
                ResumeMusicStream(music);
            }
        }
        else {
            // 开始播放
            state.isPlaying = true;
            state.isPaused = false;
            if (musicLoaded) {
                SeekMusicStream(music, state.gameTime / 1000.0f);
                PlayMusicStream(music);
            }
        }
    }

    // Ctrl+S：保存谱面
    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_S)) {
        if (state.selectedMusicIndex >= 0 && state.selectedMusicIndex < (int)musicList.size()) {
            const MusicInfo& selectedMusic = musicList[state.selectedMusicIndex];
            SaveEditorChartForMusic(state, selectedMusic);
        }
    }

    // Delete/X键：删除选中的音符
    if (IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_X)) {
        state.notes.erase(
            std::remove_if(state.notes.begin(), state.notes.end(), [](const EditorNote& n) { return n.selected; }),
            state.notes.end()
        );
        state.selectedNoteIndex = -1;
    }

    // Ctrl+Shift+Delete：删除时间范围内的音符
    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyDown(KEY_LEFT_SHIFT) && IsKeyPressed(KEY_DELETE)) {
        float visibleStart = state.gameTime - (currentJudgeLineY / state.noteSpeed * 60.0f);
        float visibleEnd = state.gameTime + 1000.0f;  // 删除未来1秒内的音符
        state.notes.erase(
            std::remove_if(state.notes.begin(), state.notes.end(),
                [visibleStart, visibleEnd](const EditorNote& n) {
                    return n.time >= visibleStart && n.time <= visibleEnd;
                }),
            state.notes.end()
        );
    }

    // Home键：跳转到开头
    if (IsKeyPressed(KEY_HOME)) {
        state.gameTime = 0.0f;
        state.isPlaying = false;
        state.isPaused = true;
        if (musicLoaded) {
            StopMusicStream(music);
            SeekMusicStream(music, 0);
        }
    }

    // End键：跳转到结尾
    if (IsKeyPressed(KEY_END)) {
        state.gameTime = state.musicLength;
        state.isPlaying = false;
        state.isPaused = true;
        if (musicLoaded) {
            StopMusicStream(music);
            SeekMusicStream(music, state.musicLength / 1000.0f);
        }
    }

    // Ctrl+A：全选可见范围内的音符
    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_A)) {
        float visibleStart = state.gameTime - (currentJudgeLineY / state.noteSpeed * 60.0f);
        float visibleEnd = state.gameTime + 2000.0f;
        for (auto& note : state.notes) {
            if (note.time >= visibleStart && note.time <= visibleEnd) {
                note.selected = true;
            }
        }
    }

    // Shift+Escape：取消所有选择
    if (IsKeyDown(KEY_LEFT_SHIFT) && IsKeyPressed(KEY_ESCAPE)) {
        for (auto& note : state.notes) {
            note.selected = false;
        }
        state.selectedNoteIndex = -1;
    }

    // 鼠标操作
    currentScreenWidth = GetScreenWidth();
    currentScreenHeight = GetScreenHeight();
    currentLaneWidth = currentScreenWidth / LANE_COUNT;
    currentJudgeLineY = currentScreenHeight - 100;
    progressBarY = (int)(15 * (float)currentScreenHeight / 600.0f);
    progressBarHeight = (int)(12 * (float)currentScreenHeight / 600.0f);
    progressBarMargin = (int)(40 * (float)currentScreenWidth / 800.0f);

    // 进度条拖拽
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        int mx = GetMouseX();
        int my = GetMouseY();
        if (my >= progressBarY && my <= progressBarY + progressBarHeight + 20 &&
            mx >= progressBarMargin && mx <= currentScreenWidth - progressBarMargin) {
            // 拖拽进度条 - 跳转到位置
            float progress = (float)(mx - progressBarMargin) / (currentScreenWidth - progressBarMargin * 2);
            if (progress < 0) progress = 0;
            if (progress > 1) progress = 1;
            state.gameTime = progress * state.musicLength;
            state.isPlaying = false;
            state.isPaused = true;
            if (musicLoaded) {
                SeekMusicStream(music, state.gameTime / 1000.0f);
                PauseMusicStream(music);
            }
        }
    }

    // Back/Exit/Help按钮处理
    if (!state.askingSaveOnExit && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        int mx = GetMouseX();
        int my = GetMouseY();

        float scaleX = (float)currentScreenWidth / 800.0f;
        float scaleY = (float)currentScreenHeight / 600.0f;

        // Back按钮位置
        int backBtnX = (int)(20 * scaleX);
        int backBtnY = (int)(45 * scaleY);
        int backBtnW = (int)(100 * scaleX);
        int backBtnH = (int)(30 * scaleY);

        // Exit按钮位置（位于Back按钮下方）
        int exitBtnX = backBtnX;
        int exitBtnY = backBtnY + backBtnH + (int)(10 * scaleY);
        int exitBtnW = backBtnW;
        int exitBtnH = backBtnH;

        // Help按钮位置（右上角）
        int helpBtnW = (int)(80 * scaleX);
        int helpBtnH = (int)(30 * scaleY);
        int helpBtnX = currentScreenWidth - helpBtnW - (int)(20 * scaleX);
        int helpBtnY = (int)(45 * scaleY);

        // Back按钮点击
        if (mx >= backBtnX && mx <= backBtnX + backBtnW && my >= backBtnY && my <= backBtnY + backBtnH) {
            state.askingSaveOnExit = true;
            state.exitToMenu = false;
            if (state.isPlaying) {
                state.isPlaying = false;
                state.isPaused = true;
                if (musicLoaded) {
                    PauseMusicStream(music);
                }
            }
        }
        // Exit按钮点击
        else if (mx >= exitBtnX && mx <= exitBtnX + exitBtnW && my >= exitBtnY && my <= exitBtnY + exitBtnH) {
            state.askingSaveOnExit = true;
            state.exitToMenu = true;
            if (state.isPlaying) {
                state.isPlaying = false;
                state.isPaused = true;
                if (musicLoaded) {
                    PauseMusicStream(music);
                }
            }
        }
        // Help按钮点击
        else if (mx >= helpBtnX && mx <= helpBtnX + helpBtnW && my >= helpBtnY && my <= helpBtnY + helpBtnH) {
            state.showingHelp = !state.showingHelp;
        }
    }

    // 鼠标左键点击处理
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        int mx = GetMouseX();
        int my = GetMouseY();

        // 检查是否点击进度条
        if (my >= progressBarY && my <= progressBarY + progressBarHeight) {
            float progress = (float)(mx - progressBarMargin) / (currentScreenWidth - progressBarMargin * 2);
            if (progress < 0) progress = 0;
            if (progress > 1) progress = 1;
            state.gameTime = progress * state.musicLength;
            if (musicLoaded) {
                SeekMusicStream(music, state.gameTime / 1000.0f);
            }
            state.isPlaying = false;
            state.isPaused = true;
        }
        // 检查是否点击音符
        else if (my < currentScreenHeight - 60) {
            int hitIndex = FindEditorNoteAtPosition(state, mx, my);

            if (hitIndex >= 0) {
                // 点击音符 - 选择/拖拽
                if (IsKeyDown(KEY_LEFT_SHIFT)) {
                    // Shift+点击：切换选择
                    state.notes[hitIndex].selected = !state.notes[hitIndex].selected;
                } else {
                    // 普通点击：取消其他选择，选中这个
                    for (auto& note : state.notes) {
                        note.selected = false;
                    }
                    state.notes[hitIndex].selected = true;
                }
                state.selectedNoteIndex = hitIndex;
                state.draggingNote = true;
                state.dragNoteIndex = hitIndex;
                state.dragStartY = (float)my;
                state.dragStartNoteTime = state.notes[hitIndex].time;
            }
            else if (state.mousePlacementMode && my < currentJudgeLineY) {
                // 鼠标放置模式：在点击位置放置音符
                int lane = mx / currentLaneWidth;
                if (lane < 0) lane = 0;
                if (lane >= LANE_COUNT) lane = LANE_COUNT - 1;

                // 根据Y坐标计算时间
                float deltaY = (float)(currentJudgeLineY - my);
                float noteTime = state.gameTime + deltaY * 1000.0f / (state.noteSpeed * 60.0f);

                if (noteTime >= 0 && noteTime <= state.musicLength && CanPlaceNote(state, lane, noteTime)) {
                    EditorNote newNote;
                    newNote.lane = lane;
                    newNote.time = noteTime;
                    newNote.selected = false;
                    state.notes.push_back(newNote);
                    std::sort(state.notes.begin(), state.notes.end(), [](const EditorNote& a, const EditorNote& b) {
                        return a.time < b.time;
                    });
                }
            }
            else {
                // 点击空白区域：取消所有选择
                for (auto& note : state.notes) {
                    note.selected = false;
                }
                state.selectedNoteIndex = -1;
            }
        }
    }

    // 音符拖拽
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && state.draggingNote && state.dragNoteIndex >= 0) {
        int my = GetMouseY();
        float deltaY = (float)(my - state.dragStartY);
        float deltaTimeMs = deltaY * 1000.0f / (state.noteSpeed * 60.0f);
        float newTime = state.dragStartNoteTime + deltaTimeMs;

        if (newTime < 0) newTime = 0;
        if (newTime > state.musicLength) newTime = state.musicLength;

        // 检查是否重叠
        if (CanPlaceNote(state, state.notes[state.dragNoteIndex].lane, newTime, state.dragNoteIndex)) {
            state.notes[state.dragNoteIndex].time = newTime;

            std::sort(state.notes.begin(), state.notes.end(), [](const EditorNote& a, const EditorNote& b) {
                return a.time < b.time;
            });

            // 查找拖拽音符的新索引
            for (size_t i = 0; i < state.notes.size(); i++) {
                if ((size_t)state.dragNoteIndex >= 0 && &state.notes[i] == &state.notes[state.dragNoteIndex]) {
                    state.dragNoteIndex = (int)i;
                    state.selectedNoteIndex = (int)i;
                    break;
                }
            }
        }
    }

    // 鼠标释放
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        state.draggingNote = false;
        state.dragNoteIndex = -1;
    }

    // 右键：快速删除音符
    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
        int mx = GetMouseX();
        int my = GetMouseY();
        int hitIndex = FindEditorNoteAtPosition(state, mx, my);
        if (hitIndex >= 0) {
            state.notes.erase(state.notes.begin() + hitIndex);
            state.selectedNoteIndex = -1;
        }
    }

    // 保存确认对话框
    if (state.askingSaveOnExit && state.gameStatePtr != nullptr) {
        GameState* gs = state.gameStatePtr;
        int mx = GetMouseX();
        int my = GetMouseY();

        // 计算对话框位置
        float scaleX = (float)GetScreenWidth() / 800.0f;
        float scaleY = (float)GetScreenHeight() / 600.0f;
        float scale = (scaleX + scaleY) / 2.0f;
        if (scale < 0.5f) scale = 0.5f;
        if (scale > 2.0f) scale = 2.0f;

        int dialogW = (int)(400 * scaleX);
        if (dialogW < 320) dialogW = 320;
        int dialogH = (int)(180 * scaleY);
        if (dialogH < 144) dialogH = 144;
        int dialogX = (GetScreenWidth() - dialogW) / 2;
        int dialogY = (GetScreenHeight() - dialogH) / 2;

        int btnW = (int)(110 * scaleX);
        if (btnW < 88) btnW = 88;
        int btnH = (int)(45 * scaleY);
        if (btnH < 36) btnH = 36;

        int yesBtnX = dialogX + dialogW / 2 - btnW - (int)(15 * scaleX);
        int yesBtnY = dialogY + dialogH - btnH - (int)(25 * scaleY);
        int noBtnX = dialogX + dialogW / 2 + (int)(15 * scaleX);
        int noBtnY = yesBtnY;
        int cancelBtnW = (int)(140 * scaleX);
        if (cancelBtnW < 112) cancelBtnW = 112;
        int cancelBtnH = btnH;
        int cancelBtnX = dialogX + (dialogW - cancelBtnW) / 2;
        int cancelBtnY = yesBtnY + btnH + (int)(15 * scaleY);

        // 只在鼠标释放时处理点击
        static bool wasMouseDown = false;
        bool mouseDown = IsMouseButtonDown(MOUSE_LEFT_BUTTON);

        if (wasMouseDown && !mouseDown) {
            bool clickedYes = (mx >= yesBtnX && mx <= yesBtnX + btnW && my >= yesBtnY && my <= yesBtnY + btnH);
            bool clickedNo = (mx >= noBtnX && mx <= noBtnX + btnW && my >= noBtnY && my <= noBtnY + btnH);
            bool clickedCancel = (mx >= cancelBtnX && mx <= cancelBtnX + cancelBtnW && my >= cancelBtnY && my <= cancelBtnY + cancelBtnH);

            if (clickedYes) {
                // 保存并退出
                if (state.selectedMusicIndex >= 0 && state.selectedMusicIndex < (int)musicList.size() && !musicList.empty()) {
                    const MusicInfo& selectedMusic = musicList[state.selectedMusicIndex];
                    SaveEditorChartForMusic(state, selectedMusic);
                }

                // 停止音乐
                if (musicLoaded) {
                    StopMusicStream(music);
                    UnloadMusicStream(music);
                    memset(&music, 0, sizeof(Music));
                }

                // 退出到相应界面
                if (state.exitToMenu) {
                    gs->menuState = MenuState::MAIN;
                    gs->enteringEditor = false;
                    // 重置游戏状态
                    gs->isPlaying = false;
                    gs->gameOver = false;
                    gs->isPaused = false;
                    gs->score = 0;
                    gs->combo = 0;
                    gs->maxCombo = 0;
                    gs->perfectCount = 0;
                    gs->greatCount = 0;
                    gs->goodCount = 0;
                    gs->missCount = 0;
                    // 重置编辑器状态
                    state.notes.clear();
                    state.selectedMusicIndex = 0;
                    state.gameTime = 0.0f;
                    state.isPlaying = false;
                    state.isPaused = true;
                    state.isRecording = false;
                    state.selectedNoteIndex = -1;
                    state.askingSaveOnExit = false;
                    state.showingHelp = false;
                    if (!musicList.empty()) {
                        gs->selectedMusicIndex = 0;
                    }
                } else {
                    gs->menuState = MenuState::SELECT_DIFFICULTY;
                }
                if (state.exitToMenu) {
                    gs->enteringEditor = false;
                }
                state.askingSaveOnExit = false;
            }
            else if (clickedNo) {
                // 不保存，直接退出
                if (musicLoaded) {
                    StopMusicStream(music);
                    UnloadMusicStream(music);
                    memset(&music, 0, sizeof(Music));
                }

                if (state.exitToMenu) {
                    gs->menuState = MenuState::MAIN;
                    gs->enteringEditor = false;
                    gs->isPlaying = false;
                    gs->gameOver = false;
                    gs->isPaused = false;
                    gs->score = 0;
                    gs->combo = 0;
                    gs->maxCombo = 0;
                    gs->perfectCount = 0;
                    gs->greatCount = 0;
                    gs->goodCount = 0;
                    gs->missCount = 0;
                    state.notes.clear();
                    state.selectedMusicIndex = 0;
                    state.gameTime = 0.0f;
                    state.isPlaying = false;
                    state.isPaused = true;
                    state.isRecording = false;
                    state.selectedNoteIndex = -1;
                    state.askingSaveOnExit = false;
                    state.showingHelp = false;
                    if (!musicList.empty()) {
                        gs->selectedMusicIndex = 0;
                    }
                } else {
                    gs->menuState = MenuState::SELECT_DIFFICULTY;
                }
                if (state.exitToMenu) {
                    gs->enteringEditor = false;
                }
                state.askingSaveOnExit = false;
            }
            else if (clickedCancel) {
                // 取消，关闭对话框
                state.askingSaveOnExit = false;
            }
        }
        wasMouseDown = mouseDown;
    }
}