#include <io.h>
#include <stdarg.h>
#include <fcntl.h>

#define CONTENT_BUF_SIZE 1024
#define FILE_NAME_BUF_SIZE 1024
#define STDERR 2
#define STDOUT 1

size_t strlen(const char *s) {
    size_t len = 0;
    for (; *s; s++) len++;
    return len;
}

void print(int fd, const char *s, ...) {
    va_list ap;
    va_start(ap, s);
    while (s) {
        _write(fd, s, strlen(s));
        s = va_arg(ap, const char *);
    }
    va_end(ap);
}

int main(int argc, char **argv) {

    char *fname, fnameBuf[FILE_NAME_BUF_SIZE], contentBuf[CONTENT_BUF_SIZE];
    int fd, readCount; // 待打开文件的文件描述符

    // memset
    for (int i = 0; i < CONTENT_BUF_SIZE; i++) {
        fnameBuf[i] = '\0';
    }

    // 如果没有指定文件名
    if (argc <= 1) {
        // 则标准输入作为文件名
        if (_read(0, fnameBuf, CONTENT_BUF_SIZE) > 0) {
            fname = fnameBuf;
        } else {
            print(STDERR, "Cannot read file name ", fnameBuf, NULL);
            return 1;
        }
    } else { // 否则第一个参数作为文件名
        fname = argv[1];
    }

    // 尝试打开文件
    fd = _open(fname, _O_RDONLY);

    // 打开文件失败
    if (fd == -1) {
        print(STDERR, "Open failed on specified file ", fname, NULL);
        return 1;
    }

    // 成功打开文件
    while (1) {
        // memset
        for (int i = 0; i < CONTENT_BUF_SIZE; i++) {
            contentBuf[i] = '\0';
        }
        readCount = _read(fd, contentBuf, CONTENT_BUF_SIZE);
        if (readCount < 0) { // 读取内容失败
            print(STDERR, "Fail to read from file ", fname, NULL);
            return 1;
        } else if (readCount == 0) { // 读不出东西了，结束
            return 0;
        } else { //  缓冲区全部用完，继续读
            print(STDOUT, contentBuf, NULL);
        }
    }
}