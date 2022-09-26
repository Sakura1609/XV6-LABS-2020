#include "kernel/types.h"
#include "user.h"

void prime(int p) {
    int child_p[2];
    int flag = 0;
    int prime_num;
    int num;
    read(p, &prime_num, 4);
    printf("prime %d\n", prime_num);
    while(read(p, &num, 4) != 0) {
        if (!flag) {
            // 子进程管道不存在
            pipe(child_p);
            flag = 1;
            // 递归创造子进程
            if (fork() == 0) {
                close(child_p[1]);
                // 递归子进程写入
                prime(child_p[0]);
            } else {
                // 递归最后一层，返回
                close(child_p[0]);
            }
        } 
        // 筛选
        if (num % prime_num != 0) {
            write(child_p[1], &num, 4);
        }
    } 
    close(p);
    close(child_p[1]);
    // 等待子进程退出
    wait(0);
}

int main() {
    int p[2];
    pipe(p);
    if (fork() == 0) {
        close(p[1]);
        prime(p[0]);
        close(p[0]);
    } else {
        close(p[0]);
        for (int i = 2; i <= 35; i++) {
            write(p[1], &i, 4);
        }
        close(p[1]);
        wait(0);
    }
    exit(0);
}