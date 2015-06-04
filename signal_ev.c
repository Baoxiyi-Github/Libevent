#include <sys/types.h>
#include <sys/stat.h>
#include <sys/queue.h>
#include <unistd.h>
#include <sys/time.h>

#include <signal.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <error.h>

#include <event.h>

int called = 0;

static void 
signal_cb(evutil_socket_t fd, short events, void *arg)
{
    struct event *signal = arg;
    printf("got signal! \n");
    const char *version = event_get_version();
    printf("version = %s\n", version);
    if(called >= 2)
      event_del(signal);
    called++;
}
int main(int argc, char**argv)
{
    struct event signal_int;
    struct event_base *base;

    /* Initalize the event library */
    /*base = event_base_new();*/    //=====>new initalize event library
    base = event_init();            //=====>old initalize event library



    /* Initalize one event */
    /*
     *old initalize one event =====> event_set()--->event_base_event()
     *new initalize one event =====> event_set()--->event_base_event()---->merge is event_assign()
     */
    /*event_assign(&signal_int, base, SIGINT, EV_SIGNAL | EV_PERSIST, signal_cb, &signal_int);*/
    event_set(&signal_int, SIGINT, EV_SIGNAL | EV_PERSIST, signal_cb, &signal_int);
    event_base_set(base, &signal_int);
    event_add(&signal_int, NULL);
    event_base_dispatch(base);

    /*
     *如果一个event_base对象不再使用的话，可以调用 
     *改函数删除event_base，但是这里需要注意的时event_base 
     *并不删除和event_base相关联的event对象，也不删除 
     *打开的其他资源
     */
    event_base_free(base);
    return 0;
}
