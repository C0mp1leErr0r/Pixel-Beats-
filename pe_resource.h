// ============================================================================
// pe_resource.h - 读取可执行文件内嵌资源（不依赖 raylib / 不暴露 windows.h）
// ============================================================================
#pragma once

// 在当前 EXE 的资源段中查找名为 name 的 RCDATA 资源。
// 找到时返回 true，*data 指向映射在内存中的只读资源数据（进程生命周期内有效，
// 无需释放），*size 返回字节数；data/size 允许传 nullptr 表示不关心该值。
bool PEResourceFind(const char* name, const unsigned char** data, unsigned int* size);
