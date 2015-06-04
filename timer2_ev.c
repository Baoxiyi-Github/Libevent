#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <event2/event.h>
#include <event2/util.h>
#include <event2/event_struct.h>

#define N       300
#define BUFFER  256

struct timeval lasttime;

struct ST_EventWithDescription {
    struct event *p_event;
    int time_interval;
    char lable[BUFFER];
};

void timeout_cb(int fd, short event, void *arg)
{
    struct timeval newtime, diffirence;
    struct ST_EventWithDescription *pEvent = (struct ST_EventWithDescription *)arg;
    struct event *timeout = pEvent->p_event;
    double elaple;

    evutil_gettimeofday(&newtime, NULL);
    evutil_timersub(&newtime, &lasttime, &diffirence);
    elaple = diffirence.tv_sec + (diffirence.tv_usec /10e6);

    printf("%s called at %d: %3f seconds since my last work.\n",
                (char *)pEvent->lable,(int)newtime.tv_sec, elaple);

    lasttime = newtime;

    struct timeval tv;
    evutil_timerclear(&tv);
    tv.tv_sec = pEvent->time_interval;
    event_add(timeout, &tv);
}

void setParam(struct ST_EventWithDescription *stEventdescription,
            struct event *m_event, int time_interval, char *m_lable)
{
    stEventdescription->p_event = m_event;
    stEventdescription->time_interval = time_interval;
    memset(stEventdescription->lable, 0, sizeof(stEventdescription->lable));
    memcpy(stEventdescription->lable, m_lable, sizeof(m_lable + 1));
}

void setTimeIntervalArr(int *arr, int n)
{
    int i;
    srand(time(NULL));

    for(i = 0; i < N; i++)
    {
        *(arr + i) = rand() % N + 1;
     
    }
}

int main(int argc, char **argv)
{
        struct event timeout[N];
        struct ST_EventWithDescription stEvent[N];
        int time_interval[N];
        int i = 0;

        struct timeval tv;
        struct event_base *base;
        int flag = 0;

        setTimeIntervalArr(time_interval, N);

        if(argc == 2 && !strcmp(argv[1], "-p"))
            flag = EV_PERSIST;
        else
            flag = 0;
        base = event_base_new();
        evutil_timerclear(&tv);

        for(i = 0; i < N; ++i)
        {
            char buf[BUFFER] = {0};
            sprintf(buf, "task_%d", i+1);
            setParam(stEvent+i, timeout+i, time_interval[i], buf);
            event_assign(timeout+i, base, -1, flag, timeout_cb,(void *)(stEvent+i));
            event_add(timeout+i, &tv);
        }

        evutil_gettimeofday(&lasttime, NULL);
        event_base_dispatch(base);
    return 0;
}


