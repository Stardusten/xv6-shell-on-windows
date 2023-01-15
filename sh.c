#include "sh.h"

char plmem[MEM_SIZE], *mem, *freem;

int main(int argc, char **argv) {

    char *cmdBuf, cwd[MAX_CMD_LEN], *smem;
    Cmd *cmd;
    HANDLE semaphore; // 用于控制共享内存访问的信号量

    if (argc == 1) {
        // 初始化信号量
        semaphore = CreateSemaphore(
                NULL,
                0,
                1,
                SEMAPHORE_NAME);

        // 无法初始化信号量
        if (semaphore == NULL) {
            print(STDERR, "Failed to create semaphore", NULL);
            return 1;
        }

        // 处理共享内存
        // 直接将共享内存给 freem，不使用 plmem
        smem = getSharedMemory(SMEM_NAME, 1);
        mem = smem;
        freem = mem;

        // 用于存放读到的指令的缓冲区
        cmdBuf = zalloc(MAX_CMD_LEN);
        while (1) {
            // 显示终端名称和当前工作路径
            _getcwd(cwd, MAX_CMD_LEN);
            print(STDOUT, "(", SHELL_NAME, ") ", cwd, " > ", NULL);

            // 读指令
            if (getCmd(cmdBuf) < 0)
                break;

            // 处理内建指令 exit
            if (cmdBuf[0] == 'e' && cmdBuf[1] == 'x' && cmdBuf[2] == 'i' && cmdBuf[3] == 't')
                break;

            // 处理内建指令 cd
            if (cmdBuf[0] == 'c' && cmdBuf[1] == 'd' && cmdBuf[2] == ' ') {
                cmdBuf[strlen(cmdBuf) - 1] = 0; // 截断
                if (_chdir(cmdBuf + 3) < 0)
                    print(STDERR, "Failed to cd ", cmdBuf + 3, NULL);
                continue;
            }

            // 解析并执行指令
            cmd = parseCmd(cmdBuf);
            runCmd(cmd, semaphore);
            print(STDOUT, "\n", NULL);
        }
    } else {
        // 初始化信号量（打开主进程创建的信号量）
        semaphore = OpenSemaphore(
                SEMAPHORE_ALL_ACCESS,
                TRUE,
                SEMAPHORE_NAME);

        // 无法打开信号量
        if (semaphore == NULL) {
            print(STDERR, "Failed to open semaphore", NULL);
            return 1;
        }

        // 处理共享内存
        smem = getSharedMemory(SMEM_NAME, 0);

        // 无法打开共享内存
        if (smem == NULL) {
            print(STDERR, "Cannot open shared memory\n", NULL);
            return 1;
        }

        // mem 指向 processlocal 的 mem
        mem = plmem;
        freem = getPtr(char*, atoi(argv[2]));

        // 从共享内存拷贝数据
        for (int i = 0; i < MEM_SIZE; i ++)
            mem[i] = smem[i];

        // 拷贝完数据后，V(semaphore)，允许主进程继续执行
        long prevCnt;
        if (!ReleaseSemaphore(semaphore, 1, &prevCnt)) {
            print(STDERR, "Failed to release semaphore", "\n", NULL);
            return 1;
        }

        // 计算要执行的指令
        SADDR cmdBias = atoi(argv[1]);
        cmd = getPtr(Cmd*, cmdBias);

        // 处理输入输出重定向
        if (argc > 2) {
            char type = *argv[3];
            int p0 = atoi(argv[4]);
            int p1 = atoi(argv[5]);
            if (type == 'i') {
                _dup2(p0, STDIN);
                _close(p0);
                _close(p1);
            } else { // 'o'
                _dup2(p1, STDOUT);
                _close(p0);
                _close(p1);
            }
        }

        // 执行指令
        runCmd(cmd, semaphore);
    }

    return 0;
}