#include "point.h"

#include "csapp.h"


inline static void init(void) {
    int *listenfd = malloc(sizeof(int));
    *listenfd = Open_listenfd(PARTA_PORT);
    fprintf(stderr, "listenfd opened at %d\n", *listenfd);

    pthread_t tid;
    Pthread_create(&tid, NULL, handleRecv, listenfd);
    Pthread_detach(tid);
}

int main(int argc, char **argv) {

    init();
    while (1) {
        handleSend();
    }

    return 0;
}


void *my_thread_send(void *vline) {
    char *h = (char *) vline;
    char *m = (char *) vline;
    for (; *m && *m != ':'; ++m);

    if (*m) {
        *(m++) = 0;

        int fd = open_clientfd(h, PARTA_PORT);
        if (fd < 0) {
            Pthread_exit((void *) 0);
        }
        Rio_writen(fd, m, strlen(m));
        Close(fd);

    } else {
        fprintf(stderr, "no colon in the input\n");
    }

    free(vline);
    Pthread_exit((void *) 0);
    return (void *) 0;
}

void handleSend(void) {
    pthread_t tid;
    char *buf = malloc(MAX_LINEBUF);
    if (Fgets(buf, MAX_LINEBUF, stdin)) {
        Pthread_create(&tid, NULL, my_thread_send, buf);
        Pthread_detach(tid);
    } else {
        exit(0); // eof
    }
}


void *my_thread_read(void *fd) {
    char buf[MAX_LINEBUF];
    rio_t rio;
    Rio_readinitb(&rio, *((int *) fd));

    Rio_readlineb(&rio, buf, MAX_LINEBUF);
    printf("%s", buf);
    Close(*((int *) fd));

    free(fd);
    Pthread_exit((void *) 0);

    return (void *) 0;
}

void *handleRecv(void *listenfd) {
    while (1) {
        int *fd = malloc(sizeof(int));
        *fd = Accept(*((int *) listenfd), NULL, NULL);
        if (*fd >= 0) {
            pthread_t tid;
            Pthread_create(&tid, NULL, my_thread_read, fd);
            Pthread_detach(tid);
        }
    }

    free(listenfd);
    Pthread_exit((void *) 0);
    return (void *) 0;
}

