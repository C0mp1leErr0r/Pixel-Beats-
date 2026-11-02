// ============================================================================
// structs.h - 游戏数据结构定义
// ============================================================================
// 本文件定义游戏中使用的所有数据结构
// 包含结构体属性说明、类继承关系说明（无继承）
// ============================================================================
// 【数据结构总览】
//   - EditorNote: 谱面编辑器中的单个音符
//   - EditorState: 谱面编辑器完整状态
//   - JudgmentEffect: 判定文字特效
//   - HitEffect: 击中圆圈扩散特效
//   - Particle: 粒子系统中单个粒子
//   - Note: 单个音符数据结构（游戏模式用）
//   - Chart: 完整谱面数据结构
//   - BgParticle: 背景装饰粒子
//   - GameState: 核心游戏状态结构
//   - MusicInfo: 音乐曲目信息
//   - BestRecord: 最佳成绩记录
// ============================================================================
// 【类继承关系】所有结构体均无继承，为Plain Old Data (POD) 结构体
// ============================================================================

#ifndef STRUCTS_H
#define STRUCTS_H

#include <string>
#include <vector>
#include "raylib.h"
#include "constants.h"

// ============================================================================
// EditorNote: 谱面编辑器中的单个音符
// ============================================================================
// 【功能说明】
//   用于谱面编辑器中存储单个音符的信息
// 【属性说明】
//   - lane: 轨道编号（0-3），对应四个下落轨道，对应键盘D/F/J/K
//   - time: 时间戳（毫秒），音符应该被击中的时间点
//   - selected: 是否被选中，用于编辑器中的多选操作
// 【使用场景】
//   - 谱面编辑器中添加/编辑音符
//   - 保存谱面到文件
// ============================================================================
struct EditorNote {
    int lane;              // 轨道编号（0-3）
    float time;            // 时间戳（毫秒）
    bool selected;         // 是否被选中
};

// Forward declaration - 前向声明避免循环依赖
struct GameState;

// ============================================================================
// EditorState: 谱面编辑器完整状态
// ============================================================================
// 【功能说明】
//   存储谱面编辑器的所有状态信息，包括音符列表、播放状态、编辑状态等
// 【属性说明】
//   - notes: 编辑器中的音符列表（std::vector<EditorNote>）
//   - musicPath: 关联的音乐文件路径
//   - selectedMusicIndex: 选中的音乐在musicList中的索引
//   - selectedDifficulty: 选中的难度等级（EASY/NORMAL/HARD）
//   - offset: 音频偏移量（毫秒），用于调整判定时机
//   - gameTime: 当前编辑位置（毫秒），决定音符在屏幕上的显示位置
//   - musicLength: 音乐总时长（毫秒）
//   - isPlaying: 是否正在播放预览
//   - isPaused: 是否暂停
//   - selectedNoteIndex: 选中的音符索引（用于单选操作）
//   - noteSpeed: 音符下落速度（默认NOTE_SPEED）
//   - draggingNote: 是否正在拖拽音符
//   - dragNoteIndex: 正在拖拽的音符索引
//   - dragStartY: 拖拽起始Y坐标
//   - dragStartNoteTime: 拖拽起始时间（用于计算时间偏移）
//   - mousePlacementMode: 鼠标放置模式开关
//   - askingSaveOnExit: 退出时是否询问保存
//   - exitToMenu: 是否返回主菜单（而非难度选择）
//   - showingHelp: 是否显示帮助界面
//   - gameStatePtr: 指向游戏状态的指针（用于音量设置等）
// 【类继承关系】
//   无继承，Plain Old Data (POD) 结构体
// 【使用场景】
//   - 谱面编辑器主循环（UpdateEditor/DrawEditorScreen）
//   - 编辑器入口/退出时初始化/保存
// ============================================================================
struct EditorState {
    std::vector<EditorNote> notes;             // 编辑器中的音符列表
    std::string musicPath;                     // 关联的音乐文件路径
    int selectedMusicIndex = -1;              // 选中的音乐索引
    Difficulty selectedDifficulty = Difficulty::NORMAL;  // 选中的难度
    float offset = 0.0f;                       // 音频偏移量（毫秒）
    float gameTime = 0.0f;                     // 当前编辑位置（毫秒）
    float musicLength = 0.0f;                  // 音乐总时长（毫秒）
    bool isPlaying = false;                    // 是否正在播放预览
    bool isRecording = false;                  // 是否正在录音模式
    bool isPaused = true;                      // 是否暂停
    int selectedNoteIndex = -1;                // 选中的音符索引
    float noteSpeed = NOTE_SPEED;             // 音符下落速度
    bool draggingNote = false;                 // 是否正在拖拽音符
    int dragNoteIndex = -1;                    // 正在拖拽的音符索引
    float dragStartY = 0.0f;                   // 拖拽起始Y坐标
    float dragStartNoteTime = 0.0f;            // 拖拽起始时间
    bool mousePlacementMode = false;           // 鼠标放置模式
    bool askingSaveOnExit = false;             // 退出时询问保存
    bool exitToMenu = false;                   // 是否返回主菜单
    bool showingHelp = false;                  // 是否显示帮助
    GameState* gameStatePtr = nullptr;         // 指向游戏状态的指针
};

// ============================================================================
// JudgmentEffect: 判定文字特效
// ============================================================================
// 【功能说明】
//   击中时在屏幕显示的判定文字（PERFECT/GREAT/GOOD/MISS）
// 【属性说明】
//   - text: 文字内容字符串
//   - lane: 所在轨道（用于确定水平位置）
//   - x, y: 文字中心坐标
//   - alpha: 透明度（1.0=完全不透明，0.0=完全透明）
//   - scale: 缩放倍数（用于缩放动画）
//   - color: 颜色（Raylib Color类型）
//   - active: 是否激活（未激活时不渲染）
//   - lifetime: 存活时间（递减到0时消失）
// 【动画原理】
//   每帧alpha递减，scale先增大后减小，产生淡出+缩放效果
// 【颜色映射】
//   - PERFECT: GOLD (金黄色)
//   - GREAT: BLUE (蓝色)
//   - GOOD: GREEN (绿色)
//   - MISS: RED (红色)
// ============================================================================
struct JudgmentEffect {
    std::string text;      // 文字内容
    int lane;              // 所在轨道
    float x, y;            // 位置
    float alpha;           // 透明度（1.0=不透明，0.0=透明）
    float scale;           // 缩放
    Color color;           // 颜色
    bool active;           // 是否激活
    float lifetime;        // 存活时间
};

// ============================================================================
// HitEffect: 击中圆圈扩散特效
// ============================================================================
// 【功能说明】
//   击中时在判定线位置产生圆圈扩散动画
// 【属性说明】
//   - x, y: 圆心坐标（判定线位置）
//   - radius: 当前半径（从0开始增长）
//   - maxRadius: 最大半径（达到后特效结束）
//   - alpha: 透明度（随radius增大而递减）
//   - color: 颜色
//   - active: 是否激活
// 【动画原理】
//   每帧radius递增，alpha = 1 - (radius/maxRadius)，圆圈扩大并淡出
// 【关键参数】
//   - 初始半径：NOTE_SIZE/2（约20像素）
//   - 最大半径：NOTE_SIZE*2（约80像素）
// ============================================================================
struct HitEffect {
    float x, y;            // 圆心坐标
    float radius;          // 当前半径
    float maxRadius;       // 最大半径
    float alpha;           // 透明度
    Color color;           // 颜色
    bool active;           // 是否激活
};

// ============================================================================
// Particle: 粒子系统中单个粒子
// ============================================================================
// 【功能说明】
//   音符击中时的粒子爆炸效果，模拟击中时的视觉反馈
// 【属性说明】
//   - x, y: 当前位置
//   - vx, vy: 速度分量（像素/秒）
//   - size: 粒子大小
//   - alpha: 透明度
//   - color: 颜色
//   - active: 是否激活
//   - life: 剩余存活时间（递减到0时重置）
//   - maxLife: 最大存活时间（用于计算透明度）
// 【动画原理】
//   每帧更新位置：x += vx*dt, y += vy*dt
//   透明度 = life/maxLife，产生渐隐效果
//   添加重力使粒子下落：vy += 300*dt
// ============================================================================
struct Particle {
    float x, y;            // 位置
    float vx, vy;          // 速度
    float size;            // 大小
    float alpha;           // 透明度
    Color color;           // 颜色
    bool active;           // 是否激活
    float life;            // 剩余存活时间
    float maxLife;         // 最大存活时间
};

// ============================================================================
// Note: 单个音符数据结构
// ============================================================================
// 【功能说明】
//   游戏进行中存储单个音符的状态
// 【属性说明】
//   - lane: 轨道（0-3）
//   - y: 屏幕上的垂直位置（像素，随时间变化）
//   - active: 是否激活（未激活不渲染也不判定）
//   - judged: 是否已判定（判定后设为true，防止重复判定）
//   - hitTime: 击中时间戳（毫秒），用于计算应该出现的位置
// 【位置计算公式】
//   y = JUDGE_LINE_Y - (hitTime - currentTime) * NOTE_SPEED * 60 / 1000
//   音符在正确的时间到达判定线
// 【状态机】
//   - active=true, judged=false: 等待被击中
//   - active=false, judged=true: 已判定（玩家击中）
//   - active=false, judged=true: 已判定（漏接MISS）
// ============================================================================
struct Note {
    int lane;              // 轨道（0-3）
    float y;               // Y坐标（像素）
    bool active;           // 是否激活
    bool judged;           // 是否已判定
    float hitTime;         // 击中时间（毫秒）
};

// ============================================================================
// Chart: 完整谱面数据结构
// ============================================================================
// 【功能说明】
//   存储一个完整谱面的所有信息，包括音符、关联音乐、难度等
// 【属性说明】
//   - musicPath: 关联的音乐文件路径
//   - chartFile: 谱面文件名（不含难度后缀）
//   - notes: 所有音符的数组
//   - judgmentEffects: 判定特效数组
//   - hitEffects: 击中特效数组
//   - offset: 音频偏移量（毫秒），用于调整判定时机
//   - difficulty: 难度等级（EASY/NORMAL/HARD）
// 【类继承关系】
//   无继承，POD结构体
// 【使用场景】
//   - LoadChart: 从文件加载谱面
//   - 游戏主循环：渲染音符、判断击中
// ============================================================================
struct Chart {
    std::string musicPath;                 // 音乐文件路径
    std::string chartFile;                 // 谱面文件名
    std::vector<Note> notes;                // 音符数组
    std::vector<JudgmentEffect> judgmentEffects;  // 判定特效
    std::vector<HitEffect> hitEffects;      // 击中特效
    float offset;                           // 音频偏移
    Difficulty difficulty;                  // 难度等级
};

// ============================================================================
// BgParticle: 背景装饰粒子
// ============================================================================
// 【功能说明】
//   用于主菜单和音乐选择界面的漂浮装饰粒子
// 【属性说明】
//   - x, y: 位置
//   - speed: 移动速度（像素/秒）
//   - size: 粒子大小
//   - alpha: 透明度
//   - alphaSpeed: 透明度变化速度（用于闪烁效果）
//   - color: 颜色
//   - active: 是否激活
// 【动画原理】
//   每帧y += speed*dt，alpha += alphaSpeed*dt，产生上下漂浮+闪烁效果
//   使用sin函数产生水平摆动效果
// ============================================================================
struct BgParticle {
    float x, y;            // 位置
    float speed;          // 移动速度
    float size;            // 大小
    float alpha;           // 透明度
    float alphaSpeed;      // 透明度变化速度
    Color color;           // 颜色
    bool active;           // 是否激活
};

// ============================================================================
// GameState: 核心游戏状态结构
// ============================================================================
// 【功能说明】
//   存储游戏进行中的所有状态信息，包括分数、连击、设置等
// 【属性说明】
//   【分数系统】
//   - score: 当前分数
//   - combo: 当前连击数
//   - maxCombo: 最大连击数
//   【判定统计】
//   - perfectCount: PERFECT次数
//   - greatCount: GREAT次数
//   - goodCount: GOOD次数
//   - missCount: MISS次数
//   【游戏状态】
//   - isPlaying: 是否正在游玩
//   - gameOver: 游戏是否结束
//   - isPaused: 是否暂停
//   - currentDifficulty: 当前难度
//   - screenShake: 画面震动强度
//   【输入状态】
//   - keyPressed[4]: 四个按键状态（D/F/J/K）
//   【菜单状态】
//   - menuState: 当前菜单状态
//   - selectedMusicIndex: 选中的音乐索引
//   - musicScrollOffset: 音乐列表滚动偏移
//   【视觉特效】
//   - laneFlash[4]: 轨道闪光效果强度
//   - particles[200]: 粒子数组
//   - bgParticles[50]: 背景粒子数组
//   【模式标志】
//   - enteringEditor: 是否正在进入编辑器
//   - isGameMode: 是否游戏模式
//   【音效资源】
//   - perfectSound: PERFECT音效
//   - greatSound: GREAT音效
//   - goodSound: GOOD音效
//   - missSound: MISS音效
//   【设置项】
//   - musicVolume: 音乐音量（0.0-1.0）
//   - bgMusicVolume: 背景音乐音量
//   - hitSoundVolume: 击中音效音量
//   - audioOffset: 音频偏移（毫秒）
//   - noteSize: 音符大小倍率
//   - noteSpeed: 音符速度倍率
//   - keyBindings[4]: 自定义按键绑定
//   - waitingForKey: 等待按键输入（-1=不等待）
//   - settingsPage: 设置页面索引
//   - bgMusicEnabled: 是否启用背景音乐
// 【类继承关系】
//   无继承，POD结构体
// ============================================================================
struct GameState {
    int score = 0;                 // 当前分数
    int combo = 0;                 // 当前连击
    int maxCombo = 0;              // 最大连击
    int perfectCount = 0;          // PERFECT次数
    int greatCount = 0;             // GREAT次数
    int goodCount = 0;              // GOOD次数
    int missCount = 0;              // MISS次数
    bool isPlaying = false;         // 是否正在游玩
    bool gameOver = false;         // 游戏是否结束
    bool isPaused = false;         // 是否暂停
    Difficulty currentDifficulty;    // 当前难度
    float screenShake;              // 画面震动强度
    bool keyPressed[4];             // 四个按键状态
    MenuState menuState;            // 当前菜单状态
    int selectedMusicIndex;         // 选中的音乐索引
    int musicScrollOffset;          // 音乐列表滚动偏移
    float laneFlash[4];             // 轨道闪光效果
    Particle particles[200];        // 粒子数组
    BgParticle bgParticles[50];     // 背景粒子数组
    bool enteringEditor = false;    // 是否正在进入编辑器
    bool isGameMode = false;        // 是否游戏模式
    bool skipNextClick = false;     // 跳过一帧的鼠标点击检测（防止状态切换时误触发）
    Sound perfectSound;             // PERFECT音效
    Sound greatSound;               // GREAT音效
    Sound goodSound;                // GOOD音效
    Sound missSound;                // MISS音效
    float musicVolume = 1.0f;       // 音乐音量
    float bgMusicVolume = 0.5f;    // 背景音乐音量
    float hitSoundVolume = 1.0f;   // 击中音效音量
    float audioOffset = 0.0f;      // 音频偏移（毫秒）
    float noteSize = 1.0f;         // 音符大小倍率
    float noteSpeed = 1.0f;         // 音符速度倍率
    int keyBindings[4];             // 按键绑定（D/F/J/K）
    int waitingForKey = -1;         // 等待按键输入
    int settingsPage = 0;           // 设置页面索引
    bool bgMusicEnabled = true;      // 是否启用背景音乐
};

// ============================================================================
// MusicInfo: 音乐曲目信息
// ============================================================================
// 【功能说明】
//   存储单个音乐曲目的元信息
// 【属性说明】
//   - name: 音乐名称（菜单显示用）
//   - filePath: 音乐文件路径（MP3文件）
//   - chartFile: 谱面文件基础名（会自动拼接 _easy.txt 等）
// 【使用场景】
//   存储在 musicList 向量中，由 LoadMusicList() 填充
// ============================================================================
struct MusicInfo {
    std::string name;               // 音乐名称
    std::string filePath;           // 音乐文件路径
    std::string chartFile;           // 谱面文件基础名
};

// ============================================================================
// BestRecord: 最佳成绩记录
// ============================================================================
// 【功能说明】
//   存储一首曲子的最佳成绩记录
// 【属性说明】
//   - score: 分数
//   - maxCombo: 最大连击
//   - accuracy: 准确率（0.0-1.0）
//   - grade: 评级（S/A/B/C/D）
// 【准确率计算公式】
//   accuracy = (perfectCount * 100 + greatCount * 80 + goodCount * 50) / totalNotes
// 【评级规则】
//   - S级: 准确率>=95%
//   - A级: 准确率>=90%
//   - B级: 准确率>=80%
//   - C级: 准确率>=70%
//   - D级: 准确率<70%
// ============================================================================
struct BestRecord {
    int score;              // 分数
    int maxCombo;           // 最大连击
    float accuracy;         // 准确率
    char grade;             // 评级（S/A/B/C/D）
};

#endif // STRUCTS_H