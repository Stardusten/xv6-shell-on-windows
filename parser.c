#include "sh.h"

//************************** 构造函数 ***************************

Cmd* execCmd(void) {
    ExecCmd* cmd;
    cmd = zalloc(sizeof(ExecCmd));
    cmd->type = EXEC;
    return (Cmd*) cmd;
}

Cmd* redirCmd(Cmd* subcmd, const char* file, const char* efile, char mode) {
    RedirCmd* cmd;
    cmd = zalloc(sizeof(RedirCmd));
    cmd->type = REDIR;
    cmd->cmd = bias(subcmd);
    cmd->file = bias(file);
    cmd->efile = bias(efile);
    cmd->mode = mode;
    return (Cmd*) cmd;
}

Cmd* pipeCmd(Cmd* left, Cmd* right) {
    PipeCmd* cmd;
    cmd = zalloc(sizeof(PipeCmd));
    cmd->type = PIPE;
    cmd->left = bias(left);
    cmd->right = bias(right);
    return (Cmd*) cmd;
}

Cmd* listCmd(Cmd* left,Cmd* right) {
    ListCmd* cmd;
    cmd = zalloc(sizeof(ListCmd));
    cmd->type = LIST;
    cmd->left = bias(left);
    cmd->right = bias(right);
    return (Cmd*) cmd;
}

Cmd* backCmd(Cmd* subcmd) {
    BackCmd* cmd;
    cmd = zalloc(sizeof(BackCmd));
    cmd->type = BACK;
    cmd->cmd = bias(subcmd);
    return (Cmd*) cmd;
}

// ***************************** 解析函数 *****************************

char whitespace[] = " \t\r\n\v";
char symbols[] = "<|>&;()";

int gettoken(char** ps, const char* es, char** q, char** eq) {
    char* s;
    int ret;

    s = *ps;
    while (s < es && strchr(whitespace, *s)) s++;
    if (q) *q = s;
    ret = *s;
    switch (*s) {
        case 0:
            break;
        case '|': case '(': case ')': case ';': case '&': case '<':
            s++;
            break;
        case '>':
            s++;
            if (*s == '>') {
                ret = '+'; s++;
            }
            break;
        default:
            ret = 'a';
            while (s < es && !strchr(whitespace, *s) && !strchr(symbols, *s)) s++;
            break;
    }
    if (eq) *eq = s;

    while (s < es && strchr(whitespace, *s)) s++;
    *ps = s;
    return ret;
}

int peek(char** ps, const char* es, char* toks) {
    char* s;

    s = *ps;
    while (s < es && strchr(whitespace, *s)) s++;
    *ps = s;
    return *s && strchr(toks, *s);
}

Cmd* parseExec(char**, char*);
Cmd* parseRedir(Cmd*, char**, char*);
Cmd* parseLine(char**, char*);
Cmd* parsePipe(char**, char*);
Cmd* parseBlock(char**, char*);
Cmd* nulTerminate(Cmd*);

Cmd* parseCmd(char* s) {
    char* es;
    Cmd* cmd;
    es = s + strlen(s);
    cmd = parseLine(&s, es);
    peek(&s, es, "");
    assert(s == es);
    nulTerminate(cmd);
    return cmd;
}

Cmd* parseLine(char** ps, char* es) {
    Cmd* cmd;

    cmd = parsePipe(ps, es);
    while (peek(ps, es, "&")) {
        gettoken(ps, es, 0, 0);
        cmd = backCmd(cmd);
    }
    if (peek(ps, es, ";")) {
        gettoken(ps, es, 0, 0);
        cmd = listCmd(cmd, parseLine(ps, es));
    }
    return cmd;
}

Cmd* parsePipe(char** ps, char* es) {
    Cmd* cmd;

    cmd = parseExec(ps, es);
    if (peek(ps, es, "|")) {
        gettoken(ps, es, 0, 0);
        cmd = pipeCmd(cmd, parsePipe(ps, es));
    }
    return cmd;
}

Cmd* parseRedir(Cmd* cmd, char** ps, char* es) {
    int tok;
    char *q, *eq;

    while (peek(ps, es, "<>")) {
        tok = gettoken(ps, es, 0, 0);
        assert(gettoken(ps, es, &q, &eq) == 'a');
        switch (tok) {
            case '<':
            case '>':
            case '+':  // >>
                cmd = redirCmd(cmd, q, eq, tok);
                break;
        }
    }
    return cmd;
}

Cmd* parseBlock(char** ps, char* es) {
    Cmd* cmd;

    assert(peek(ps, es, "("));
    gettoken(ps, es, 0, 0);
    cmd = parseLine(ps, es);
    assert(peek(ps, es, ")"));
    gettoken(ps, es, 0, 0);
    cmd = parseRedir(cmd, ps, es);
    return cmd;
}

Cmd* parseExec(char** ps, char* es) {
    char *q, *eq;
    int tok, argc;
    ExecCmd* cmd;
    Cmd* ret;

    if (peek(ps, es, "(")) return parseBlock(ps, es);

    ret = execCmd();
    cmd = (ExecCmd*)ret;

    argc = 0;
    ret = parseRedir(ret, ps, es);
    while (!peek(ps, es, "|)&;")) {
        if ((tok = gettoken(ps, es, &q, &eq)) == 0) break;
        assert(tok == 'a');
        cmd->argv[argc] = bias(q);
        cmd->eargv[argc] = bias(eq);
        assert(++argc < MAX_ARGS);
        ret = parseRedir(ret, ps, es);
    }
    cmd->argv[argc] = 0;
    cmd->eargv[argc] = 0;
    return ret;
}

Cmd* nulTerminate(Cmd* cmd) {
    int i;
    BackCmd* bcmd;
    ExecCmd* ecmd;
    ListCmd* lcmd;
    PipeCmd* pcmd;
    RedirCmd* rcmd;

    if (cmd == 0) return 0;

    switch (cmd->type) {
        case EXEC:
            ecmd = (ExecCmd*) cmd;
            for (i = 0; ecmd->argv[i]; i++)
                *getPtr(char*, ecmd->eargv[i]) = 0;
            break;
        case REDIR:
            rcmd = (RedirCmd*) cmd;
            nulTerminate(getPtr(Cmd*, rcmd->cmd));
            *getPtr(char*, rcmd->efile) = 0;
            break;
        case PIPE:
            pcmd = (PipeCmd*) cmd;
            nulTerminate(getPtr(Cmd*, pcmd->left));
            nulTerminate(getPtr(Cmd*, pcmd->right));
            break;
        case LIST:
            lcmd = (ListCmd*) cmd;
            nulTerminate(getPtr(Cmd*, lcmd->left));
            nulTerminate(getPtr(Cmd*, lcmd->right));
            break;
        case BACK:
            bcmd = (BackCmd*) cmd;
            nulTerminate(getPtr(Cmd*, bcmd->cmd));
            break;
    }
    return cmd;
}