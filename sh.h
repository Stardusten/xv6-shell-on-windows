#ifndef DESIGN2_SH_H
#define DESIGN2_SH_H

/** 一条指令最多允许有多少个参数 */
#define MAX_ARGS 10

/** 一条指令的最大长度 */
#define MAX_CMD_LEN 100

/** 内存池大小 */
#define MEM_SIZE 4096

/** 命名共享内存的名字 */
#define SMEM_NAME "WinShsMem"

/** 这个 Shell 的名字 */
#define SHELL_NAME "WinSh"

#define SEMAPHORE_NAME "WinShSemaphore"

/** 管道指令创建的管道的最大大小 */
#define PIPE_SIZE 4096

/** 虚拟地址类型 */
#define SADDR size_t

#define THIS_EXE "sh.exe" // TODO

#include <stdarg.h>
#include <Windows.h>
#include <stdio.h>
// 使用 windows 提供的 POSIX 兼容的底层 API
#include <io.h>
#include <direct.h>
#include <process.h>
#include <fcntl.h>
#include <sys\stat.h>

/** 五种指令类型标记 */
enum { EXEC = 1, REDIR, PIPE, LIST, BACK };

/** 标准输入，标准输出，错误输出的文件描述符 */
enum { STDIN = 0, STDOUT, STDERR };

typedef struct Cmd_struct {
    int type;
} Cmd;

typedef struct ExecCmd_struct {
    int type;
    SADDR argv[MAX_ARGS], eargv[MAX_ARGS]; // -> char*[MAX_ARGS]
} ExecCmd;

typedef struct RedirCmd_struct {
    int type;
    char mode;
    SADDR file, efile; // -> char*
    SADDR cmd; // -> Cmd*
} RedirCmd;

typedef struct PipeCmd_struct {
    int type;
    SADDR left, right; // -> Cmd*
} PipeCmd;

typedef struct ListCmd_struct {
    int type;
    SADDR left, right; // -> Cmd*
}  ListCmd;

typedef struct BackCmd_struct {
    int type;
    SADDR cmd; // -> Cmd*
} BackCmd;

/** */
Cmd* parseCmd(char*);

void printCmd(Cmd*);

int runCmd(Cmd*, HANDLE);

/**
 * 将一些字符串输出到指定文件描述符
 * @param int 输出到哪个文件描述符
 * @param ... 要输出的字符串，以 NULL 结束
 */
void print(int, const char*, ...);

/**
 * 从标准输入（终端）读入一条指令
 * @param char* 用于存放读到的指令字符串的缓冲区
 * @return
 */
int getCmd(char*);

//***************** 一个简单的内存池，完全静态分配 ********************

/**
 * ProcessLocal 内存
 */
extern char plmem[MEM_SIZE];

/**
 * 指向内存始址的指针
 */
extern char *mem;

/**
 * 指向下一个未分配地址的指针
 */
extern char *freem;

/**
 * 分配指定大小的内存
 * @param sz 需要的内存大小，单位字节
 * @return 分配到的内存始址
 */
void *zalloc(size_t sz);

/**
 * 将 mem 映射到一个进程间共享的文件上
 * @param mappingName 映射文件名
 * @param allowCreate 是否允许创建映射文件
 * @return
 */
char* getSharedMemory(char* mappingName, int allowCreate);

//**********************************************************************

/**
 * 空虚拟地址
 */
#define NULL_SADDR ((size_t) 0)

/**
 * 获得一个类型为 type，指向虚拟地址 saddr 的指针
 */
#define getPtr(type, saddr) ((type) (mem + ((SADDR) (saddr)) - 1))

/**
 * 将 ptr 指针转换为虚拟地址
 */
#define bias(ptr) (!(ptr) ? NULL_SADDR : (((char*) (ptr)) - ((char*) mem) + 1))

char** getArgv(const SADDR*);

/**
 * 断言，如果 cond 为假则 _exit(1)
 */
#define assert(cond) \
    do { if (!(cond)) { \
        print(STDERR, "Panicked.", NULL); \
        _exit(1); } \
    } while (0)

#endif //DESIGN2_SH_H
