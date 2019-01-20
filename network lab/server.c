#include "server.h"
#include "mylib.h"

listnode *head = 0;
sem_t sem;  //mutex to protect global sum
//#define MAX_LINEBUF  0x1000
//const char passcodes[MAX_LINEBUF];//dynamic generating passcode
const char* static_passcode = "123\n";
char currentTime[MAX_LINEBUF];
char passes[20][MAX_LINEBUF];
int count = 0;
int main() {
    strcpy(passes[0],static_passcode);
    setbuf(stdout,0);
    setbuf(stderr,0);
    int listenfd = Open_listenfd(PARTB_PORT);//3

    fprintf(stderr, "listenfd opened at %d\n", listenfd);

    Sem_init(&sem, 0, 1);

    while (1) {
        int fd = Accept(listenfd, NULL, NULL);
        //fprintf(stderr, "fd is %d\n", fd);
        if (fd >= 0) {
            //for temp verify use
            char name[MAX_LINEBUF];
            char passcode[MAX_LINEBUF];
            char can_enter[MAX_LINEBUF];
            char dyn_passcode[MAX_LINEBUF];

            //rio1
            rio_t rio1;
            rio_readinitb(&rio1,fd);
            //printf("read init done:%s\n",name);
            rio_readlineb(&rio1,name,MAX_LINEBUF);
            //printf("read name done:%s\n",name);

            name[strlen(name)-1] = 0;
            rio_readlineb(&rio1,passcode,MAX_LINEBUF);
            //printf("read passcode done:%s\n",passcode);


            //pass the passcode
//
//            if(strncmp(passcode,static_passcode,strlen(passcode))) {
//                printf("wrong passcode!\n");
//                sprintf(can_enter, "no\n");
//                rio_writen(fd, can_enter, strlen(can_enter));
//                continue;
//            }
            int matched = 0;
            int i = 0;
            for(i=0;i< sizeof(passes)/MAX_LINEBUF;i++){
                if(strcmp(passcode,passes[i])==0) {
                    matched = 1;
                }
            }
            if(matched==0){
                printf("wrong passcode!\n");
                sprintf(can_enter,"no\n");
                Rio_writen(fd, can_enter, strlen(can_enter));
                continue;
            }


            sprintf(can_enter,"yes\n");
            rio_writen(fd,can_enter,strlen(can_enter));
            listnode *p = insertNode(&head, &sem, fd,name);//the new client

            fprintf(stderr,"%s entered the room!\n",name);
            count++;
            rio_readlineb(&rio1,dyn_passcode,MAX_LINEBUF);
            strcpy(passes[count],dyn_passcode);
            /* fprintf(stderr,"the dynpasscode is: %s\n",dyn_passcode);
             fprintf(stderr,"Newly added one is: %s\n",passes[count]);
             fprintf(stderr,"The passcode list now is:\n");
             int j = 0;
             for(j=0;j< sizeof(passes)/MAX_LINEBUF;j++){
                 fprintf(stderr,"pass:%s\n",passes[j]);
             }*/

            pthread_t tid;// the new thread
            Pthread_create(&tid, NULL, my_thread_reader, p);
            Pthread_detach(tid);
        }
    }

}


listnode *createNode(int fd, listnode *next,char* name) {
    listnode *p = malloc(sizeof(listnode));
    p->fd = fd;
    p->next = next;
    strcpy(p->name,name);
    return p;
}

void freeNode(listnode *p) {
    free(p);
}

listnode *insertNode(listnode **p, sem_t *sem, int fd,char* name) {
    listnode *res;
    P(sem);
    res = *p = createNode(fd, *p,name);
    V(sem);
    return res;
}

void removeNode(listnode **p, sem_t *sem, int fd) {


    P(sem);
    while (*p) {
        if ((*p)->fd == fd) {
            listnode *n = *p;
            *p = (*p)->next;
            freeNode(n);
            goto over;
        }
        p = &((*p)->next);
    }
    over:
    V(sem);
}

void sendNode(listnode **p, sem_t *sem, int fd, char *message,listnode* vnode) {
    P(sem);
    fprintf(stderr, "start sending...: %s   from %s\n", message,vnode->name);

    char sended_msg[MAX_LINEBUF];
    sprintf(sended_msg,"\n<%s> from %s,%s\n",message,vnode->name,getTime());

    //fprintf(stderr,"%s",sended_msg);
    int sendCnt = 0;
    int len = strlen(sended_msg);
    while (*p) {
        if ((*p)->fd != fd) {
            Rio_writen((*p)->fd, sended_msg, len);
            sendCnt++;
        }
        p = &((*p)->next);
    }

    fprintf(stderr, "%6d sended...", sendCnt);

    V(sem);
}

// current vnode's fd
void *my_thread_reader(void *vnode) {
    int fd = ((listnode *) vnode)->fd;
    char buf[MAX_LINEBUF];
    rio_t rio;

    Rio_readinitb(&rio, ((listnode *) vnode)->fd);

    while (1) {
        ssize_t sz = Rio_readlineb(&rio, buf, MAX_LINEBUF);
        if (sz <= 0) {
            removeNode(&head, &sem, fd);
            Close(fd);
            break;
        }
        buf[strlen(buf)-1] = 0;
        if(strlen(buf)<=0) continue;
        sendNode(&head, &sem, fd, buf,vnode);
    }

    Pthread_exit((void *) 0);
    return (void *) 0;
}

//calculate the time
char* getTime(){
    time_t now;
    struct tm *tm_now;
    time(&now);
    tm_now = localtime(&now);
    sprintf(currentTime,"%d/%d/%d:%d:%d:%d\n",tm_now->tm_year+1900,tm_now->tm_mon+1, tm_now->tm_mday, tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec);
    return currentTime;
}