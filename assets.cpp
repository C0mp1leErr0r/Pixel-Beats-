// ============================================================================
// assets.cpp - 资源加载实现
// ============================================================================
#include "assets.h"
#include "pe_resource.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

// ----------------------------------------------------------------------------
// 取小写扩展名（".mp3" / ".wav" ...），raylib 的内存加载接口要求传入扩展名
// ----------------------------------------------------------------------------
static std::string LowerExtension(const char* fileName) {
    std::string ext;
    if (fileName == nullptr) return ext;
    const char* dot = strrchr(fileName, '.');
    if (dot == nullptr) return ext;
    for (const char* p = dot; *p != '\0'; ++p) {
        char c = *p;
        if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
        ext.push_back(c);
    }
    return ext;
}

// 磁盘上是否存在可读文件
static bool ExistsOnDisk(const char* fileName) {
    if (fileName == nullptr || fileName[0] == '\0') return false;
    FILE* file = fopen(fileName, "rb");
    if (file == nullptr) return false;
    fclose(file);
    return true;
}

// 读出整个磁盘文件；失败返回 false
static bool ReadWholeFile(const char* fileName, std::vector<unsigned char>& buffer) {
    FILE* file = fopen(fileName, "rb");
    if (file == nullptr) return false;

    if (fseek(file, 0, SEEK_END) != 0) { fclose(file); return false; }
    long size = ftell(file);
    if (size < 0) { fclose(file); return false; }
    if (fseek(file, 0, SEEK_SET) != 0) { fclose(file); return false; }

    buffer.resize((size_t)size);
    size_t read = (size > 0) ? fread(buffer.data(), 1, (size_t)size, file) : 0;
    fclose(file);
    return read == (size_t)size;
}

bool AssetExists(const char* fileName) {
    if (ExistsOnDisk(fileName)) return true;
    return PEResourceFind(fileName, nullptr, nullptr);
}

std::string LoadTextAsset(const char* fileName) {
    std::string text;

    std::vector<unsigned char> buffer;
    if (ReadWholeFile(fileName, buffer)) {
        text.assign(buffer.begin(), buffer.end());
    } else {
        const unsigned char* data = nullptr;
        unsigned int size = 0;
        if (!PEResourceFind(fileName, &data, &size)) return std::string();
        text.assign((const char*)data, (size_t)size);
    }

    // 统一换行符为 '\n'（原实现用 std::ifstream 文本模式，CRLF 会被自动转换）
    if (text.find('\r') != std::string::npos) {
        std::string normalized;
        normalized.reserve(text.size());
        for (size_t i = 0; i < text.size(); ++i) {
            if (text[i] == '\r') {
                if (i + 1 < text.size() && text[i + 1] == '\n') continue;  // 去掉 CRLF 中的 CR
                normalized.push_back('\n');                                // 单独的 CR 也当作换行
            } else {
                normalized.push_back(text[i]);
            }
        }
        text.swap(normalized);
    }

    return text;
}

Music LoadMusicAsset(const char* fileName) {
    Music music;
    memset(&music, 0, sizeof(Music));
    if (fileName == nullptr || fileName[0] == '\0') return music;

    // 1. 磁盘文件优先
    if (ExistsOnDisk(fileName)) return LoadMusicStream(fileName);

    // 2. 内嵌副本：直接从 exe 资源段解码（零拷贝，资源内存在进程生命周期内有效）
    const unsigned char* data = nullptr;
    unsigned int size = 0;
    if (PEResourceFind(fileName, &data, &size) && size > 0) {
        return LoadMusicStreamFromMemory(LowerExtension(fileName).c_str(), data, (int)size);
    }

    // 3. 都没有：交给 raylib 走原来的失败/日志路径
    return LoadMusicStream(fileName);
}

Wave LoadWaveAsset(const char* fileName) {
    Wave wave;
    memset(&wave, 0, sizeof(Wave));
    if (fileName == nullptr || fileName[0] == '\0') return wave;

    // 1. 磁盘文件优先
    if (ExistsOnDisk(fileName)) return LoadWave(fileName);

    // 2. 内嵌副本
    const unsigned char* data = nullptr;
    unsigned int size = 0;
    if (PEResourceFind(fileName, &data, &size) && size > 0) {
        return LoadWaveFromMemory(LowerExtension(fileName).c_str(), data, (int)size);
    }

    // 3. 都没有：返回空波形（调用方会回退到程序化生成音效）
    return wave;
}

// 与 raylib 的 LoadSound 等价：加载波形 -> 转成 Sound -> 释放波形
Sound LoadSoundAsset(const char* fileName) {
    Sound sound;
    memset(&sound, 0, sizeof(Sound));

    Wave wave = LoadWaveAsset(fileName);
    if (wave.frameCount == 0) return sound;

    sound = LoadSoundFromWave(wave);
    UnloadWave(wave);   // 数据已复制进声音缓冲，可以释放
    return sound;
}
