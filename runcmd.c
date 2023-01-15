#include "sh.h"

/**
 * 执行一个 Cmd
 * @param cmd 待执行的 Cmd
 * @param semaphore 用于共享内存同步的信号量
 * @return 如果执行成功，返回 0，否则返回 1
 */
int runCmd(Cmd* cmd, HANDLE semaphore) {

    // 空指令
    if (cmd == NULL) {
        print(STDERR, "Invalid command\n", NULL);
        return 1;
    }

//    printCmd(cmd);

    switch (cmd->type) {
        case EXEC: {
            ExecCmd *ecmd = (ExecCmd *) cmd;
            if (ecmd->argv[0] == 0) {
                print(STDERR, "[EXEC] Invalid command\n", NULL);
                return 1;
            }
            char *fileName = getPtr(char*, ecmd->argv[0]);
            char **args = getArgv(ecmd->argv);
            if (_spawnvp(_P_WAIT, fileName, args) == -1) {
                print(STDERR, "[EXEC] Failed to execute ", fileName, "\n", NULL);
                return 1;
            }
            return 0;
        }
        case REDIR: {
            RedirCmd *rcmd = (RedirCmd*) cmd;
            char mode = rcmd->mode;
            int fd = mode == '<'
                     ? STDIN
                     : STDOUT;
            int openFlag = mode == '<'
                           ? O_RDONLY
                           : mode == '>'
                             ? O_WRONLY | O_CREAT | O_TRUNC
                             : O_WRONLY | O_CREAT;
            int fd2 = _dup(fd); // dup 一份
            _close(fd); // 关掉原来的，让打开的新文件能替换之
            char *fileName = getPtr(char*, rcmd->file);
            if (_open(fileName, openFlag, 0644) == -1) {
                print(STDERR, "[REDIR] Failed to open ", getPtr(char*, rcmd->file), NULL);
                return 1;
            }
            Cmd* subcmd = getPtr(Cmd*, rcmd->cmd);
            int ret = runCmd(subcmd, semaphore);
            _dup2(fd2, fd); // 恢复
            _close(fd2); // 关掉 dup 的
            return ret;
        }
        case LIST: {
            ListCmd *lcmd = (ListCmd *) cmd;
            Cmd *left = getPtr(Cmd*, lcmd->left);
            if (runCmd(left, semaphore)) {
                print(STDERR, "[LIST] Failed to exec left command.", NULL);
                return 1;
            }
            Cmd *right = getPtr(Cmd*, lcmd->right);
            if (runCmd(right, semaphore)) {
                print(STDERR, "[LIST] Failed to exec left command.", NULL);
                return 1;
            }
            return 0;
        }
        case PIPE: {
            PipeCmd *pcmd = (PipeCmd *) cmd;
            int p[2]; // 管道读口写口对应的文件描述符
            // 创建管道，用于将左侧指令的输出传到右侧，作为右侧指令的输入
            if (_pipe(p, PIPE_SIZE, O_BINARY) == -1) {
                print(STDERR, "[PIPE] Failed to create pipe for pipe command.", NULL);
                return 1;
            }
            SADDR biasLeft = pcmd->left;
            char biasArg[5], freemArg[5], p0Arg[5], p1Arg[5];
            itoa((int) biasLeft, biasArg, 10);
            itoa((int) bias(freem), freemArg, 10);
            itoa(p[0], p0Arg, 10);
            itoa(p[1], p1Arg, 10);
            intptr_t procLeft, procRight;
            // 左侧指令的输出重定向到管道的写口，传 4 个参数：
            // 1. 指令偏移
            // 2. freem 偏移
            // 3. 输出重定向标记
            // 4. 管道读口文件描述符
            // 5. 管道写口文件描述符
            procLeft = _spawnlp(_P_NOWAIT, THIS_EXE, THIS_EXE, biasArg, freemArg, "o", p0Arg, p1Arg, NULL);
            if (procLeft == -1) {
                print(STDERR, "[PIPE] Failed to create process left.", NULL);
                return 1;
            }
            // 等待子进程拷贝完数据，才继续执行
            WaitForSingleObject(semaphore, INFINITE);

            SADDR biasRight = pcmd->right;
            itoa((int)  biasRight, biasArg, 10);
            // 右侧指令的输入重定向到管道的读口，传 4 个参数：
            // 1. 指令偏移
            // 2. freem 偏移
            // 3. 输入重定向标记
            // 4. 管道读口文件描述符
            // 5. 管道写口文件描述符
            procRight = _spawnlp(_P_NOWAIT, THIS_EXE, THIS_EXE, biasArg, freemArg, "i", p0Arg, p1Arg, NULL);
            if (procRight == -1) {
                print(STDERR, "[PIPE] Failed to create process right.", NULL);
                return 1;
            }
            WaitForSingleObject(semaphore, INFINITE);

            // 关掉创建的管道
            _close(p[0]);
            _close(p[1]);
            // 等待左右子进程结束
            _cwait(NULL, procLeft, _WAIT_CHILD);
            _cwait(NULL, procRight, _WAIT_CHILD);
            return 0;
        }
        case BACK: {
            BackCmd *bcmd = (BackCmd *) cmd;
            // 将子进程要执行的命令的偏移址作为第一个命令行参数
            SADDR bias = bcmd->cmd;
            char biasArg[5], freemArg[5];
            itoa((int) bias, biasArg, 10);
            itoa((int) bias(freem), freemArg, 10);
            // _P_NOWAIT - 不阻塞（异步）调用
            if (_spawnlp(_P_NOWAIT, THIS_EXE, THIS_EXE, biasArg, freemArg, NULL) == -1) {
                print(STDERR, "[BACK] Failed to create process.", NULL);
                return 1;
            }
            WaitForSingleObject(semaphore, INFINITE);
            return 0;
        }
        default: {
            print(STDERR, "Unknown command type", NULL);
            return 1;
        }
    }

}

int getCmd(char *buf) {
    int nbuf = MAX_CMD_LEN;
    for (int i = 0; i < nbuf; i++)
        buf[i] = '\0';
    while (nbuf-- > 1) {
        int nread = _read(0, buf, 1);
        if (nread <= 0) return -1;
        if (*(buf++) == '\n') break;
    }
    return 0;
}

char** getArgv(const SADDR* addrs) {
    int i;
    SADDR saddr;
    char** argv;
    argv = zalloc(MAX_ARGS);
    for (i = 0; ; i++) {
        if ((saddr = addrs[i]) == 0) break;
        argv[i] = getPtr(char*, saddr);
    }
    argv[i] = NULL;
    return argv;
}