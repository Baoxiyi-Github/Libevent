#include <event.h>
#include <iostream>

using namespace std;
void onTime(evutil_socket_t sock, short event, void *arg)
{
    cout << "Game over! " << endl;
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    // 重新添加定时事件（定时事件触发后默认自动删除）
    event_add((struct event *)arg, &tv);
}

//event_init() => evtimer_set() => event_add() =>event_dispatch()
int main(void)
{
    //初始化
    event_init();
    struct event evTime;
    // 设置定时事件
    evtimer_set(&evTime, onTime, &evTime);
    
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    // 添加定时事件
    event_add(&evTime, &tv);
    // 事件循环
    event_dispatch();
    return 0;
}
