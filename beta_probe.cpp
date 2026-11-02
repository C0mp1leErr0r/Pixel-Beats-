// ============================================================================
// beta_probe.cpp - 内嵌资源/加载路径自检（beta 版，无界面）
// ============================================================================
// 直接把游戏的 FileName.cpp 包含进来（把其中的 main 改名），
// 这样可以调用游戏自己的 LoadChart / 资源加载函数，验证的是真实代码路径。
// 运行方式：在空目录下运行（强制走内嵌资源），退出码 0 表示全部通过。
// ============================================================================

#define main game_main_entry
#include "FileName.cpp"
#undef main

#include "pe_resource.h"

#include <cstdio>
#include <set>
#include <string>

static int g_failures = 0;

static void Check(bool ok, const char* what) {
    std::printf("[%s] %s\n", ok ? " OK " : "FAIL", what);
    if (!ok) g_failures++;
}

int main() {
    std::printf("=== beta embedded asset probe ===\n");
    std::printf("disk assets present: settings=%d charts_dir=%d hit.wav=%d\n",
                (int)FileExists("settings.txt"), (int)DirectoryExists("charts"), (int)FileExists("hit.wav"));

    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "probe");
    InitAudioDevice();

    const int songCount = (int)(sizeof(kSongCatalog) / sizeof(kSongCatalog[0]));
    std::set<std::string> musicPaths;

    // ---- 1. 每个歌曲的 3 个难度谱面 -------------------------------------
    for (int s = 0; s < songCount; ++s) {
        for (int d = 0; d < 3; ++d) {
            const char* path = kSongCatalog[s].chartPaths[d];
            Chart chart = LoadChart(path);
            char label[256];
            std::snprintf(label, sizeof(label), "chart %-26s %-6s notes=%-5zu music=%s",
                          kSongCatalog[s].displayName, kDifficultyLabels[d],
                          chart.notes.size(), chart.musicPath.c_str());
            Check(!chart.notes.empty() && !chart.musicPath.empty(), label);
            if (!chart.musicPath.empty()) musicPaths.insert(chart.musicPath);
        }
    }

    // ---- 2. 每首歌的音乐（内嵌 MP3 -> 内存解码）-------------------------
    for (std::set<std::string>::const_iterator it = musicPaths.begin(); it != musicPaths.end(); ++it) {
        Music music = LoadMusicAsset(it->c_str());
        float seconds = (music.frameCount > 0) ? GetMusicTimeLength(music) : 0.0f;
        char label[256];
        std::snprintf(label, sizeof(label), "music %-52s frames=%-9u len=%.1fs",
                      it->c_str(), music.frameCount, seconds);
        Check(music.frameCount > 0 && seconds > 30.0f, label);
        UnloadMusicStream(music);
    }

    // ---- 3. 打击音效 ----------------------------------------------------
    Sound hit = LoadSoundAsset("hit.wav");
    Check(hit.frameCount > 0, "hit.wav -> Sound ready");
    if (hit.frameCount > 0) { PlaySound(hit); WaitTime(0.05); StopSound(hit); }
    UnloadSound(hit);

    // ---- 4. 反复加载/卸载，确认释放解码器时不破坏只读资源 --------------
    bool reloadOk = true;
    {
        std::string firstMusic = *musicPaths.begin();
        for (int round = 0; round < 5; ++round) {
            Music again = LoadMusicAsset(firstMusic.c_str());
            if (again.frameCount == 0) reloadOk = false;
            UnloadMusicStream(again);
        }
        const unsigned char* data = nullptr;
        unsigned int size = 0;
        Check(reloadOk, "music reload/unload x5 (resource memory intact)");
        Check(PEResourceFind(firstMusic.c_str(), &data, &size) && size > 1000000,
              "resource bytes still mapped after unloads");
    }

    // ---- 5. 实时流式解码 ------------------------------------------------
    {
        std::string firstMusic = *musicPaths.begin();
        Music track = LoadMusicAsset(firstMusic.c_str());
        if (track.frameCount > 0) {
            SetMusicVolume(track, 0.0f);
            PlayMusicStream(track);
            double startTime = GetTime();
            float before = GetMusicTimePlayed(track);
            while ((GetTime() - startTime) < 1.5) { UpdateMusicStream(track); WaitTime(0.005); }
            float after = GetMusicTimePlayed(track);
            char label[160];
            std::snprintf(label, sizeof(label), "stream decodes in real time (%.2fs -> %.2fs)", before, after);
            Check(after > before + 0.5f, label);
            StopMusicStream(track);
        }
        UnloadMusicStream(track);
    }

    CloseAudioDevice();
    CloseWindow();

    std::printf("=== FAILURES=%d ===\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
