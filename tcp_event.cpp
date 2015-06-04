#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <event.h>
#include <event2/util.h>
//event_base_new()=>event_set()=>event_base_set()=>event_add()=>event_base_dispatch()

using namespace std;
struct event_base *base;
static int isClifd;
static int isSrvfd;
// 读事件回调函数
void onRead(evutil_socket_t fd, short event, void *arg)
{
    int Len;
    char buf[1024];
    Len = recv(fd, buf, sizeof(buf), 0);

    if(Len <= 0)
    {
        cout << "Client close!" << endl;
        // 连接结束(=0)或连接错误(<0)，将事件删除并释放内存空间
        struct event *pEvread = (struct event *)arg;
        event_del(pEvread);
        delete pEvread;

        close(isClifd);
        return ;
    }

    buf[Len] = '\0';
    cout << "Client Info: " << buf << endl;
}

// 连接请求事件回调函数
void onAccept(evutil_socket_t fd, short ievent, void *arg)
{
    struct sockaddr_in isCliaddr;
    socklen_t isLen = sizeof(isCliaddr);
    isClifd = accept(isSrvfd, (struct sockaddr *)&isCliaddr, &isLen);

    // 连接注册为新事件 (EV_PERSIST为事件触发后不默认删除)
    struct event *pEvread = new event;
    event_set(pEvread, isClifd, EV_READ | EV_PERSIST, onRead,pEvread);
    event_base_set(base, pEvread);
    event_add(pEvread, NULL);
}

int main(void)
{
    struct sockaddr_in isSrvaddr;

    memset(&isSrvaddr, 0, sizeof(isSrvaddr));
    isSrvaddr.sin_family = AF_INET;
    isSrvaddr.sin_port = htons(8888);
    isSrvaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // 创建tcpSocket（iSvrFd），监听本机8888端
    isSrvfd = socket(AF_INET, SOCK_STREAM, 0);
    if(-1 ==  isSrvfd)
    {
        printf("socket failed!\n");
        close(isSrvfd);
        return -1;
    }

    if(-1 == (bind(isSrvfd, (struct sockaddr *)&isSrvaddr, sizeof(isSrvaddr))))
    {
        printf("bind failed!\n");
        close(isSrvfd);
        return -1;
    }

    if(-1 == listen(isSrvfd, 10))
    {
        printf("listen failed!\n");
        close(isSrvfd);
        return -1;
    }
    // 初始化base   
    base = event_base_new();
    struct event evlisten;
    // 设置事件
    event_set(&evlisten, isSrvfd, EV_READ|EV_PERSIST, onAccept, NULL);
    // 设置为base事件
    event_base_set(base, &evlisten);
    // 添加事件
    event_add(&evlisten, NULL);
    // 事件循环
    event_base_dispatch(base);
    return 0;
}
