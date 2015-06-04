#include <sys/types.h>
#include <sys/stat.h>
#include <sys/queue.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <event.h>

static void 
fifo_read(evutil_socket_t fd, short events, void *arg)
{
    char buf[30];
    int len;
    struct event *ev = arg;

    event_add(ev, NULL);
    
    fprintf(stdout, "fifo_read called with fd:%d, event:%d, arg: %p\n",
                fd, events, arg);

    len = read(fd, buf, sizeof(buf) - 1);
    if(len == -1)
    {
        perror("read fifo");
        return ;
    }
    else if(len == 0)
    {
        fprintf(stdout, "Connected closed\n");
        return ;
    }

    buf[len] = '\0';
    fprintf(stdout, "Read: %s \n", buf);
}

int main(int argc, char **argv)
{
    struct event evfifo;
    struct event_base *base;
    struct stat st;
    const char *fifo = "event.fifo";
    int socket, errno;

    if(lstat(fifo, &st) == 0){
        if((st.st_mode & S_IFMT) == S_IFREG)
        {
            errno = EEXIST;
            perror("lstat");
            exit(1);
        }
    }

    unlink(fifo);
    if(mkfifo(fifo, 0600) == -1)
    {
        perror("mkfifo");
        exit(1);
    }

    /* Linux pipes are broken, we need O_RDWR instead of O_RDONLY */
    //open fifo

    socket = open(fifo, O_RDWR | O_NONBLOCK, S_IRWXU);
    if( -1 == socket)
    {
        perror("open");
        exit(1);
    }
    
    fprintf(stdout, "write data to %s \n", fifo);

    //Initalize event Library
    /*event_init();*/
    base = event_base_new();
    //Initalize one event
    /* 初始化event类型,注册回调函数fifo_read */
    /*
     *event_set(&evfifo, socket, EV_READ, fifo_read, &evfifo);
     *event_base_set(base, &evfifo);
     */
    event_assign(&evfifo, base, socket, EV_READ | EV_PERSIST, fifo_read, &evfifo);
    /*
     *Add it to the active events, without a timeout
     *添加event
     */
    event_add(&evfifo, NULL);

    /*开始调度“event”*/
    /*event_dispatch();*/
    event_base_dispatch(base);
    return 0;
}
