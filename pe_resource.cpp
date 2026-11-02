// ============================================================================
// pe_resource.cpp - 通过 Win32 资源 API 读取内嵌在 EXE 中的资源
// ============================================================================
// 【说明】
//   本文件是唯一包含 <windows.h> 的翻译单元，且不包含 raylib.h，
//   以避免 Windows 的 Rectangle/CloseWindow 等符号与 raylib 冲突。
//   资源 ID 与文件名的对应关系由生成文件 assets_index.h 提供。
// ============================================================================

#define WIN32_LEAN_AND_MEAN
#define NOGDI          // 避免 wingdi.h 中的 Rectangle() 函数与 raylib 的 struct Rectangle 冲突
#define NOUSER
#include <windows.h>
#include <string>
#include "pe_resource.h"
#include "assets_index.h"

#ifndef RT_RCDATA
#define RT_RCDATA INT_RESOURCE(10)
#endif

// 以整数 ID 引用资源（等价于 MAKEINTRESOURCEW，但无需包含 winuser.h）
#define INT_RESOURCE(id) ((LPCWSTR)((ULONG_PTR)((WORD)(id))))

// 不区分大小写的字符串比较（仅处理 ASCII，足够用于文件名）
static bool EqualsIgnoreCase(const char* a, const char* b) {
    while (*a != '\0' && *b != '\0') {
        char ca = *a, cb = *b;
        if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
        if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
        if (ca != cb) return false;
        ++a; ++b;
    }
    return *a == *b;
}

// 取出文件名部分（处理 '/' 和 '\\'）
static const char* BaseName(const char* path) {
    const char* base = path;
    for (const char* p = path; *p != '\0'; ++p) {
        if (*p == '/' || *p == '\\') base = p + 1;
    }
    return base;
}

// 在生成的文件名表中查找资源 ID，找不到返回 -1
static int LookupAssetId(const char* name) {
    if (name == nullptr || name[0] == '\0') return -1;

    // 1. 完整名字精确匹配
    for (int i = 0; i < kEmbeddedAssetCount; ++i) {
        if (EqualsIgnoreCase(kEmbeddedAssetTable[i].name, name)) return kEmbeddedAssetTable[i].id;
    }

    // 2. 只比较文件名（忽略调用方传入的目录前缀）
    const char* base = BaseName(name);
    if (base != name) {
        for (int i = 0; i < kEmbeddedAssetCount; ++i) {
            if (EqualsIgnoreCase(kEmbeddedAssetTable[i].name, base)) return kEmbeddedAssetTable[i].id;
        }
    }

    return -1;
}

bool PEResourceFind(const char* name, const unsigned char** data, unsigned int* size) {
    int id = LookupAssetId(name);
    if (id < 0) return false;

    HMODULE module = GetModuleHandleW(nullptr);
    if (module == nullptr) return false;

    HRSRC info = FindResourceW(module, INT_RESOURCE(id), RT_RCDATA);
    if (info == nullptr) return false;

    DWORD bytes = SizeofResource(module, info);
    HGLOBAL handle = LoadResource(module, info);
    if (handle == nullptr) return false;

    void* ptr = LockResource(handle);
    if (ptr == nullptr) return false;

    if (data != nullptr) *data = static_cast<const unsigned char*>(ptr);
    if (size != nullptr) *size = (unsigned int)bytes;
    return true;
}
