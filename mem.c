#include "sh.h"

void *zalloc(size_t sz) {
    assert(freem + sz < mem + MEM_SIZE);
    void *ret = (void*) freem;
    freem += sz;
    return ret;
}

char* getSharedMemory(char* mappingName, int allowCreate) {
    HANDLE mapping; // 文件映射对象
    char* ret;

    // 先尝试打开映射
    mapping = OpenFileMapping(
                FILE_MAP_ALL_ACCESS,
                FALSE,
                SMEM_NAME);

    // 如果不存在
    if (mapping == NULL) {
        // 且允许创建新映射
        if (allowCreate) {
            mapping = CreateFileMapping(
                    INVALID_HANDLE_VALUE,
                    NULL,
                    PAGE_READWRITE,
                    0,
                    MEM_SIZE,
                    SMEM_NAME);

            // 打开失败
            if (mapping == NULL) return NULL;
        } else return NULL;
    }

    return (char*) MapViewOfFile(
            mapping,
            FILE_MAP_ALL_ACCESS,
            0,
            0,
            MEM_SIZE);
}