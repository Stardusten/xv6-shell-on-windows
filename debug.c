#include "sh.h"

// ************************* 打印指令的函数 *************************

void print(int fd, const char *s, ...) {
    va_list ap;
            va_start(ap, s);
    while (s) {
        _write(fd, s, strlen(s));
        s = va_arg(ap, const char *);
    }
            va_end(ap);
}

void printExecCmd(ExecCmd* ecmd) {
    printf("Exec[args=\"", NULL);
    int i;
    SADDR addr;
    for (i = 0; i < MAX_ARGS; i++) {
        if ((addr = ecmd->argv[i]) == 0) break;
        printf("%s ", getPtr(char*, addr));
    }
    printf("\b\"]", NULL);
}

void printRedirCmd(RedirCmd* rcmd) {
    printf("Redir[mode=%d, file=%s, cmd=",rcmd->mode, getPtr(char*, rcmd->file));
    printCmd(getPtr(Cmd*, rcmd->cmd));
    printf("]");
}

void printPipeCmd(PipeCmd* pcmd) {
    printf("Pipe[lcmd=");
    printCmd(getPtr(Cmd*, pcmd->left));
    printf(", rcmd=");
    printCmd(getPtr(Cmd*, pcmd->right));
    printf("]");
}

void printListCmd(ListCmd* lcmd) {
    printf("List[lcmd=");
    printCmd(getPtr(Cmd*, lcmd->left));
    printf(", rcmd=");
    printCmd(getPtr(Cmd*, lcmd->right));
    printf("]");
}

void printBackCmd(BackCmd* bcmd) {
    printf("Back[cmd=");
    printCmd(getPtr(Cmd*, bcmd->cmd));
    printf("]");
}

void printCmd(Cmd* cmd) {
    ExecCmd *ecmd;
    RedirCmd *rcmd;
    PipeCmd *pcmd;
    ListCmd *lcmd;
    BackCmd *bcmd;

    switch (cmd->type) {
        case EXEC:
            ecmd = (ExecCmd*) cmd;
            printExecCmd(ecmd);
            break;
        case REDIR:
            rcmd = (RedirCmd*) cmd;
            printRedirCmd(rcmd);
            break;
        case PIPE:
            pcmd = (PipeCmd*) cmd;
            printPipeCmd(pcmd);
            break;
        case LIST:
            lcmd = (ListCmd*) cmd;
            printListCmd(lcmd);
            break;
        case BACK:
            bcmd = (BackCmd*) cmd;
            printBackCmd(bcmd);
            break;
    }
}