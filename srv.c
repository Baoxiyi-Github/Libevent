#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/socket.h>
#include <event.h>
#include <event2/bufferevent.h>

typedef  struct sockaddr SA;

void accept_cb(int fd, short event, void *args);
void socket_read_cb(struct bufferevent *bev, void *args);
void event_cb(struct bufferevent *bev, short event, void *args);
int tcp_server_init(int port, int listen_num);

int main(int argc, char **argv)
{
    int listener = tcp_server_init(9999, 10);
    if(-1 == listener)
    {
        perror("tcp_server_init error ");
        return -1;
    }

    struct event_base *base = event_base_new();
    //添加监听客户端请求连接事件 
    struct event *ev_listen = event_new(base, listener,EV_READ | EV_PERSIST,
                                        accept_cb, base);
    event_add(ev_listen, NULL);
    event_base_dispatch(base);
    event_base_free(base);
    return 0;
}

void accept_cb(int fd, short event, void *args)
{
    evutil_socket_t sockfd;
    struct sockaddr_in client;
    socklen_t len;

    sockfd =accept(fd, (SA *)&client, &len);
    evutil_make_socket_nonblocking(sockfd);

    printf("accept a client %d\n", sockfd);

    struct event_base *base = (struct event_base *)args;
    
    struct bufferevent *bev = bufferevent_socket_new(base, sockfd, 
                                                    BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(bev, socket_read_cb, NULL, event_cb, args);
    bufferevent_enable(bev, EV_READ | EV_PERSIST);

}

void socket_read_cb(struct bufferevent *bev, void *args)
{
    char msg[4096];
    size_t len = bufferevent_read(bev, msg, sizeof(msg));
    msg[len] = '\0';

    printf("recv the client msg : %s\n", msg);

    char reply_msg[4096] = "t have recvieced the msg: ";
    strcat(reply_msg+strlen(reply_msg), msg);
    bufferevent_write(bev, reply_msg, strlen(reply_msg));
}

void event_cb(struct bufferevent *bev, short event, void *args)
{
    if(event & BEV_EVENT_EOF)
        printf("connection closed \n");
    else
        printf("some other error \n");
    //这将自动close套接字和free读写缓冲区
    bufferevent_free(bev);
}

int tcp_server_init(int port, int listen_num)
{
    int errno_save, errno;
    evutil_socket_t listener;

    listener = socket(AF_INET, SOCK_STREAM, 0);
    if(-1 == listen_num)
    {
        perror("socket failed ");
        return -1;
    }
    //允许多次绑定同一个地址。要用在socket和bind之间
    evutil_make_listen_socket_reuseable(listener);

    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = 0;
 
    if(bind(listener, (SA*)&sin, sizeof(sin)) < 0)
        goto error;
    
    if(listen(listener, 10) < 0)
        goto error;
    //跨平台统一接口，将套接字设置为非阻塞状态
    evutil_make_socket_nonblocking(listener);
    return listener;
error:
    errno_save = errno;
    evutil_closesocket(listener);
    errno = errno_save;
    return -1;
}
