#include <event2/event.h>
#include <stdio.h>
#include <errno.h>

void cb_func(evutil_socket_t fd, short what, void *arg)
{
    printf("get alerted! \n");
}

int main(void)
{
    struct event_base *base;
    struct event *ev1, *ev2;
    struct timeval five_second = {1, 0};
    
    base = event_base_new();
    if(! base)
        return -1;

    ev1 = event_new(base, -1, EV_TIMEOUT, cb_func, NULL);
    ev2 = event_new(base, -1, EV_TIMEOUT, cb_func, NULL);

    /*
     *event_add(ev1, &five_second);
     *struct timeval time = {1, 0};
     *event_add(ev2, &time);
     */
    event_active(ev1, EV_TIMEOUT, 0);
    event_active(ev2, EV_TIMEOUT, 5);
    /*event_base_dispatch(base);*/
    int tmp = event_base_dispatch(base);
    printf("%d \n", tmp);

    return 0;
}
