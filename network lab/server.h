#ifndef SERVER_H
#define SERVER_H

#include <semaphore.h>

#define PARTB_PORT "12019"
#define MAX_LINEBUF 0x1000

typedef struct listnode {
    int fd;
    struct listnode *next;
    char name[20];
    //char* passcode;

} listnode;

listnode *createNode(int fd, listnode *,char* name);

void freeNode(listnode *);

listnode *insertNode(listnode **, sem_t *, int fd,char* name);

void removeNode(listnode **, sem_t *, int fd);

void sendNode(listnode **, sem_t *, int fd, char *message,listnode *vnode);

void *my_thread_reader(void *);

char* getTime();

#endif