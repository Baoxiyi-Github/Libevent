#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <event2/event-config.h>
#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/util.h>

struct timeval lasttime;

int event_is_persistent;

static void timeout_cb(evutil_socket_t fd, short event, void *args)
{
    struct timeval newtime, diffrence;
    struct event *timeout = args;
    double elapsed;

    evutil_gettimeofday(&newtime, NULL);
    evutil_timersub(&newtime, &lasttime, &diffrence);
    elapsed = diffrence.tv_sec + (diffrence.tv_usec / 1.0e6);

    printf("timeout_cb called at %d:%.3f seconds elapsed.\n", (int)newtime.tv_sec, elapsed);

    lasttime = newtime;

    if(! event_is_persistent)
    {
        struct timeval tv;
        evutil_timerclear(&tv);
        tv.tv_sec = 2;
        event_add(timeout, &tv);
    }

}

int main(int argc, char **argv)
{
    struct event timeout;
    struct timeval tv;
    struct event_base *base;
    int flags;

    if(argc == 2 && !strcmp(argv[1], "-p"))
    {
        event_is_persistent = 1;
        flags = EV_PERSIST;
    }
    else
    {
        event_is_persistent = 0;
        flags = 0;
    }

    // Initalize the event library初始化程序库
    base = event_base_new();

    event_assign(&timeout, base, -1, flags, timeout_cb, (void *)&timeout);
    evutil_timerclear(&tv);
    tv.tv_sec = 2;

    event_add(&timeout, &tv);
    
    evutil_gettimeofday(&lasttime, NULL);

    event_base_dispatch(base);

    return 0;
}
