/*
 * This app fork a child process, and the child process fork a grandchild process.
 * every process waits for its own child exit then prints.
 * Three processes also write their own global variables "flag"
 * to different values.
 */

#include "user/user_lib.h"
#include "util/types.h"

int flag;
int main(void) {
    flag = 0;
    int pid = fork();
    if (pid == 0) {
        // 0子进程
        flag = 1;
        pid = fork();
        if (pid == 0) {
            // 孙进程
            flag = 2;
            printu("Grandchild process end, flag = %d.\n", flag);
        } else {
            // 子进程
            // pid:孙进程
            wait(pid);
            printu("Child process end, flag = %d.\n", flag);
        }
    } else {
        // 非0父进程
        wait(-1);
        printu("Parent process end, flag = %d.\n", flag);
    }
    exit(0);
    return 0;
}
