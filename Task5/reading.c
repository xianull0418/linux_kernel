#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <semaphore.h>
#include <string.h>

#define SEAT 5    // 定义座位数

int queue[SEAT];
pthread_mutex_t mutex;   // 定义互斥信号量
sem_t empty, full;       // 定义空座位和满座位的信号量
int count = 0;           // 记录阅览室内的人数

// 进入阅览室的读者线程函数
void *intoreadroom(void *arg) {
    printf("读者正在进入阅览室...\n");
    int i = 0;
    char input[100]; // 用户输入缓冲区

    while (1) {
        sem_wait(&empty); // 等待一个空座位
        pthread_mutex_lock(&mutex); // 加锁互斥量

        // 请求输入电话号码
        printf("请输入进入阅览室的读者的电话号码：\n");
        fgets(input, sizeof(input), stdin);  // 安全读取字符串输入
        queue[i] = atoi(input); // 将输入的电话号码存储在队列中

        printf("----注册成功！----\n");
        count++;
        printf("电话号码为：%d 的读者进入了阅览室。\n", queue[i]);
        printf("正在读书中...\n");

        printf("此时阅览室已有人数为：%d\n", count);
        printf("此时阅览室剩余空座位个数为：%d\n", SEAT - count);
        printf("--------------------------------\n");

        pthread_mutex_unlock(&mutex); // 解锁互斥量
        sem_post(&full); // 信号座位已满

        i = (i + 1) % SEAT;
        sleep(3); // 模拟阅读时间
    }
}

// 离开阅览室的读者线程函数
void *leavereadroom(void *arg) {
    printf("有读者正在离开阅览室。\n");
    int i = 0;

    while (1) {
        sem_wait(&full); // 等待一个满座位
        pthread_mutex_lock(&mutex);

        printf("电话号码为 %d 的读者离开了阅览室。\n", queue[i]);
        queue[i] = 0;
        count--;
        printf("----注销成功！----\n");

        printf("此时阅览室剩余空座位个数为：%d\n", SEAT - count);
        printf("---------------------------------\n");

        pthread_mutex_unlock(&mutex);
        sem_post(&empty); // 信号座位已空

        i = (i + 1) % SEAT;
        sleep(5); // 模拟时间延迟
    }
}

int main(int argc, char *argv[]) {
    pthread_t pid, cid;
    sem_init(&empty, 0, SEAT); // 初始化信号量
    sem_init(&full, 0, 0);
    pthread_mutex_init(&mutex, NULL); // 初始化互斥量

    pthread_create(&pid, NULL, intoreadroom, NULL); // 创建线程
    pthread_create(&cid, NULL, leavereadroom, NULL);

    pthread_join(pid, NULL); // 等待线程结束
    pthread_join(cid, NULL);

    sem_destroy(&empty); // 销毁信号量
    sem_destroy(&full);
    pthread_mutex_destroy(&mutex); // 销毁互斥量

    return 0;
}
