#ifndef POINT_H
#define POINT_H


#define PARTA_PORT "12018"
// #define PARTB_PORT "12019"

#define MAX_LINEBUF 300

void *my_thread_send(void *vline);

void handleSend(void);

void *my_thread_read(void *fd);

void *handleRecv(void *listenfd);

#endif
