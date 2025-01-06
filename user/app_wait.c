/*
 * This app fork a child process, and the child process fork a grandchild process.
 * every process waits for its own child exit then prints.
 * Three processes also write their own global variables "flag"
 * to different values.
 */

// #include "user/user_lib.h"
// #include "util/types.h"

// int flag;
// int main(void) {
//     flag = 0;
//     int pid = fork();
//     if (pid == 0) {
//         // 0子进程
//         flag = 1;
//         pid = fork();
//         if (pid == 0) {
//             // 孙进程
//             flag = 2;
//             printu("Grandchild process end, flag = %d.\n", flag);
//         } else {
//             // 子进程
//             // pid:孙进程
//             wait(pid);
//             printu("Child process end, flag = %d.\n", flag);
//         }
//     } else {
//         // 非0父进程
//         wait(-1);
//         printu("Parent process end, flag = %d.\n", flag);
//     }
//     exit(0);
//     return 0;
// }


#include "user/user_lib.h"
#include "util/types.h"

int flag;

int sleep(int n) {
    int sum;
    for (size_t i = 0; i < 10000000 * n; i++) {
        sum = i * i * i * i;
    }
    return sum;
}

void test_wait_any() {
    flag = 0;
    int pid1 = fork();
    if (pid1 == 0) {
        // 第一个子进程
        flag = 1;
        printu("[Child1] pid=%d starting, flag=%d\n", getpid(), flag);
        int sum;
        for (size_t i = 0; i < 100000000; i++) {
            sum = i * i;
        }
        exit(sum * 1 - sum + 11);
    }
    else {
        int pid2 = fork();
        if (pid2 == 0) {
            // 第二个子进程
            flag = 2;
            printu("[Child2] pid=%d starting, flag=%d\n", getpid(), flag);
            int sum;
            for (size_t i = 0; i < 1000000000; i++) {
                sum = i * i;
            }
            exit(sum * 1 - sum + 22);
        }
        else {
            int pid3 = fork();
            if (pid3 == 0) {
                // 第三个子进程
                flag = 3;
                printu("[Child3] pid=%d starting, flag=%d\n", getpid(), flag);
                int sum;
                for (size_t i = 0; i < 200000000; i++) {
                    sum = i * i;
                }
                exit(sum * 1 - sum + 33);
            }
            else {
                // 父进程等待任意子进程
                printu("[Parent] waiting for any child\n");
                int pid = wait(pid2);
                printu("[Parent] child pid=%d exited\n", pid);
                pid = wait(pid3);
                printu("[Parent] child pid=%d exited\n", pid);
                pid = wait(-1);
                printu("[Parent] child pid=%d exited\n", pid);
                // 此时已经没有子进程，应该返回-1
                pid = wait(-1);
                printu("[Parent] wait with no children returns %d\n", pid);
            }
        }
    }
}

void test_wait_specific() {
    flag = 10;
    int pid1 = fork();
    if (pid1 == 0) {
    // 子进程
        flag = 11;
        printu("[Child] pid=%d starting, flag=%d\n", getpid(), flag);
        exit(100);
        }
    else {
     // 等待特定pid
        printu("[Parent] waiting for pid=%d\n", pid1);
        int pid = wait(pid1);
        printu("[Parent] child pid=%d exited\n", pid);

        // 测试等待不存在的pid
        pid = wait(12345);
        printu("[Parent] wait for non-existent pid returns %d\n", pid);

        // 测试等待非子进程
        int pid2 = getpid() + 100;  // 一个一定不是子进程的pid
        pid = wait(pid2);
        printu("[Parent] wait for non-child pid returns %d\n", pid);
    }
    }

int main() {
    printu("=== Testing wait(-1) ===\n");
    test_wait_any();

    printu("\n=== Testing wait(pid) ===\n");
    test_wait_specific();

    exit(0);
    return 0;
}