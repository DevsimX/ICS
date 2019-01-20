#include "client.h"

#include "mylib.h"

volatile int serverfd;
char name[MAX_LINEBUF];//the user's name
char passcode[MAX_LINEBUF];//the user's passcode
char dyn_passcode[MAX_LINEBUF];

void *my_thread_reader(void *vp) {
    rio_t rio;

    Rio_readinitb(&rio, serverfd);

    char buf[MAX_LINEBUF];
    while (Rio_readlineb(&rio, buf, MAX_LINEBUF) > 0) {
        printf("%s", buf);
    }

    exit(1);
}

void init(char *hostname) {
//    serverfd = Open_clientfd(hostname, PARTB_PORT);
//
//    pthread_t tid;
//    Pthread_create(&tid, NULL, my_thread_reader, NULL);
//    Pthread_detach(tid);
//char s_passcode[MAX_LINEBUF];
    fprintf(stderr, "Welcome to the chatroom!\n");
    fprintf(stderr, "Your Name:\n");
    scanf("%s", name);
    //sprintf(name,"%s\n",name);
    strcat(name,"\n");
    fprintf(stderr, "confirm your name:\n%s\n", name);
    //transmit the user's name to server
    //Rio_writen(serverfd,name,strlen(name));
    while (1) {

        fprintf(stderr, "Your Chatroom Passcode:\n");
        fprintf(stderr, "(if you want to exit,input q)\n");
        scanf("%s", passcode);
        if(strcmp(passcode,"q")==0){
            fprintf(stderr, "confirm your exit!\n");
            exit(0);
        }
        fprintf(stderr, "confirm your passcode:\n%s\n", passcode);
        //sprintf(s_passcode, "%s\n",passcode);
        //sprintf(passcode,"%s\n",passcode);
        strcat(passcode,"\n");

        serverfd = Open_clientfd(hostname, PARTB_PORT);
        //printf("open clientfd done\n");
        rio_t rio;
        Rio_writen(serverfd, name, strlen(name));
        // printf("write name done\n");
        Rio_writen(serverfd, passcode, strlen(passcode));
        //printf("write passcode done\n");

        char res[MAX_LINEBUF];//result

        rio_readinitb(&rio, serverfd);
        // printf("read init done:%s\n",res);
        //printf("read init done:%d\n",serverfd);
        rio_readlineb(&rio, res, MAX_LINEBUF);
        //printf("read line done:%s\n",res);

        char *yes = "yes\n";
        //printf("set yes done:%s\n",yes);

        if (strncmp(res,yes,strlen(res))!=0) {
            printf("yuor password is wrong!\n");
        } else {
            //printf("%s,\n",yes);
            printf("You have entered successfully!\n");
            srand((unsigned)time(NULL));
            sprintf(dyn_passcode,"%d",rand());
            strcat(dyn_passcode,"\n");
            Rio_writen(serverfd, dyn_passcode, strlen(dyn_passcode));
            printf("Your inviting passcode is %s",dyn_passcode);

            break;
        }

        //transmit the user's name to server
        // Rio_writen(serverfd,passcode,strlen(passcode));
    }
    pthread_t tid;
    Pthread_create(&tid, NULL, my_thread_reader, NULL);
    Pthread_detach(tid);
}

int main(int argc, char **args) {
    setbuf(stdout,0);
    setbuf(stdin,0);
    if (argc != 2) {
        fprintf(stderr, "uasage %s <hostname>\n", args[0]);
    }

    printf("%d",1);
    printf("%s",args[1]);

    if(args[1] == NULL){
        args[1] = "12019";
    }

    init(args[1]);

    char buf[MAX_LINEBUF];
    while (Fgets(buf, MAX_LINEBUF, stdin)) {
        if(strcmp(buf,"q\n")==0){
            fprintf(stderr, "confirm your exit!\n");
            exit(0);
        }
        Rio_writen(serverfd, buf, strlen(buf));
    }

}

