/*
 * Copyright (c) 2000-2007 Niels Provos <provos@citi.umich.edu>
 * Copyright (c) 2007-2012 Niels Provos and Nick Mathewson
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _EVENT_INTERNAL_H_
#define _EVENT_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "event2/event-config.h"
#include <time.h>
#include <sys/queue.h>
#include "event2/event_struct.h"
#include "minheap-internal.h"
#include "evsignal-internal.h"
#include "mm-internal.h"
#include "defer-internal.h"

/* map union members back */

/* mutually exclusive */
#define ev_signal_next	_ev.ev_signal.ev_signal_next
#define ev_io_next	_ev.ev_io.ev_io_next
#define ev_io_timeout	_ev.ev_io.ev_timeout

/* used only by signals */
#define ev_ncalls	_ev.ev_signal.ev_ncalls
#define ev_pncalls	_ev.ev_signal.ev_pncalls

/* Possible values for ev_closure in struct event. */
#define EV_CLOSURE_NONE 0
#define EV_CLOSURE_SIGNAL 1
#define EV_CLOSURE_PERSIST 2

/** Structure to define the backend of a given event_base. */
//bcf===>在libevent中，每种I/O demultiplex机制的实现都必须提供这五个函数接口，来完成自身的初始化、销毁释放；对事件的注册、注销和分发


//bcf===>对于定时事件，这些函数将调用timer heap管理接口执行插入和删除操作；对于I/O和Signal事件将调用eventopadd和delete接口函数执行插入和删除操作（eventop会对Signal事件调用Signal处理接口执行操作）


struct eventop {
	/** The name of this backend. */
    //事件机制的名称
	const char *name;
	/** Function to set up an event_base to use this backend.  It should
	 * create a new structure holding whatever information is needed to
	 * run the backend, and return it.  The returned pointer will get
	 * stored by event_init into the event_base.evbase field.  On failure,
	 * this function should return NULL. */
    //bcf===>初始化事件机制的回调函数
	void *(*init)(struct event_base *);
	/** Enable reading/writing on a given fd or signal.  'events' will be
	 * the events that we're trying to enable: one or more of EV_READ,
	 * EV_WRITE, EV_SIGNAL, and EV_ET.  'old' will be those events that
	 * were enabled on this fd previously.  'fdinfo' will be a structure
	 * associated with the fd by the evmap; its size is defined by the
	 * fdinfo field below.  It will be set to 0 the first time the fd is
	 * added.  The function should return 0 on success and -1 on error.
	 */
    //bcf===>添加事件的的回调函数
	int (*add)(struct event_base *, evutil_socket_t fd, short old, short events, void *fdinfo);
	/** As "add", except 'events' contains the events we mean to disable. */
    //bcf===>删除事件的回调函数
	int (*del)(struct event_base *, evutil_socket_t fd, short old, short events, void *fdinfo);
	/** Function to implement the core of an event loop.  It must see which
	    added events are ready, and cause event_active to be called for each
	    active event (usually via event_io_active or such).  It should
	    return 0 on success and -1 on error.
	 */
    //bcf===>事件分发,开始整个事件循环的回调函数
	int (*dispatch)(struct event_base *, struct timeval *);
	/** Function to clean up and free our data from the event_base. */
    //bcf===>注销，释放资源 删除事件机制的回调函数
	void (*dealloc)(struct event_base *);
	/** Flag: set if we need to reinitialize the event base after we fork.
	 */
    //是否需要将事件机制重新初始化
	int need_reinit;
	/** Bit-array of supported event_method_features that this backend can
	 * provide. */
    //当前事件机制所支持的I/O多路分发机制
    //是这三个值的组合 EV_FEATURE_ET，EV_FEATURE_O1，EV_FEATURE_FDS
	enum event_method_feature features;
	/** Length of the extra information we should record for each fd that
	    has one or more active events.  This information is recorded
	    as part of the evmap entry for each fd, and passed as an argument
	    to the add and del functions above.
	 */
    //事件fd所记录的额外信息长度
	size_t fdinfo_len;
};

#ifdef WIN32
/* If we're on win32, then file descriptors are not nice low densely packed
   integers.  Instead, they are pointer-like windows handles, and we want to
   use a hashtable instead of an array to map fds to events.
*/
#define EVMAP_USE_HT
#endif

/* #define HT_CACHE_HASH_VALS */

#ifdef EVMAP_USE_HT
#include "ht-internal.h"
struct event_map_entry;
HT_HEAD(event_io_map, event_map_entry);
#else
#define event_io_map event_signal_map
#endif

/* Used to map signal numbers to a list of events.  If EVMAP_USE_HT is not
   defined, this structure is also used as event_io_map, which maps fds to a
   list of events.
*/
struct event_signal_map {
	/* An array of evmap_io * or of evmap_signal *; empty entries are
	 * set to NULL. */
	void **entries;
	/* The number of entries available in entries */
	int nentries;
};

/* A list of events waiting on a given 'common' timeout value.  Ordinarily,
 * events waiting for a timeout wait on a minheap.  Sometimes, however, a
 * queue can be faster.
 **/
struct common_timeout_list {
	/* List of events currently waiting in the queue. */
	struct event_list events;
	/* 'magic' timeval used to indicate the duration of events in this
	 * queue. */
	struct timeval duration;
	/* Event that triggers whenever one of the events in the queue is
	 * ready to activate */
	struct event timeout_event;
	/* The event_base that this timeout list is part of */
	struct event_base *base;
};

/** Mask used to get the real tv_usec value from a common timeout. */
#define COMMON_TIMEOUT_MICROSECONDS_MASK       0x000fffff

struct event_change;

/* List of 'changes' since the last call to eventop.dispatch.  Only maintained
 * if the backend is using changesets. */
struct event_changelist {
	struct event_change *changes;
	int n_changes;
	int changes_size;
};

#ifndef _EVENT_DISABLE_DEBUG_MODE
/* Global internal flag: set to one if debug mode is on. */
extern int _event_debug_mode_on;
#define EVENT_DEBUG_MODE_IS_ON() (_event_debug_mode_on)
#else
#define EVENT_DEBUG_MODE_IS_ON() (0)
#endif

struct event_base {
	/** Function pointers and other data to describe this event_base's
	 * backend. */
    //bcf===>evsel指向了全局变量static const struct eventop *eventops[]中的一个
	const struct eventop *evsel; //指向事件处理函数集合
	/** Pointer to backend-specific data. */
    //bcf===>evbase实际上是一个eventop实例对象
	void *evbase;

	/** List of changes to tell backend about at next dispatch.  Only used
	 * by the O(1) backends. */
	struct event_changelist changelist;

	/** Function pointers used to describe the backend that this event_base
	 * uses for signals */
	const struct eventop *evsigsel;
	/** Data to implement the common signal handelr code. */
	struct evsig_info sig;

	/** Number of virtual events */
	int virtual_event_count;
	/** Number of total events added to this event_base */
    //事件的总数,包括定时器,普通事件和信号
	int event_count;
	/** Number of total events active in this event_base */
    //被激活事件的总数
	int event_count_active;

	/** Set if we should terminate the loop once we're done processing
	 * events. */
	int event_gotterm;
	/** Set if we should terminate the loop immediately */
	int event_break;
	/** Set if we should start a new instance of the loop immediately. */
	int event_continue;

	/** The currently running priority of events */
	int event_running_priority;

	/** Set if we're running the event_base_loop function, to prevent
	 * reentrant invocation. */
	int running_loop;

	/* Active event management. */
	/** An array of nactivequeues queues for active events (ones that
	 * have triggered, and whose callbacks need to be called).  Low
	 * priority numbers are more important, and stall higher ones.
	 */
    /*
     *bcf===>start
     *激活事件链表,注意它是1个指向指针的指针.
     *因为事件按照优先级被分成了很多队列,
     *默认情况下libevent只使用1个优先级,所以也只有1个激活队列.
     *bcf===>end        
     */
	struct event_list *activequeues;
	/** The length of the activequeues array */
	int nactivequeues;//激活事件的总数

	/* common timeout logic */

	/** An array of common_timeout_list* for all of the common timeout
	 * values we know. */
	struct common_timeout_list **common_timeout_queues;
	/** The number of entries used in common_timeout_queues */
	int n_common_timeouts;
	/** The total size of common_timeout_queues. */
	int n_common_timeouts_allocated;

	/** List of defered_cb that are active.  We run these after the active
	 * events. */
	struct deferred_cb_queue defer_queue;

	/** Mapping from file descriptors to enabled (added) events */
	struct event_io_map io;

	/** Mapping from signal numbers to enabled (added) events. */
	struct event_signal_map sigmap;

	/** All events that have been enabled (added) in this event_base */
    //bcf===>保存了所有的注册事件event的指针
	struct event_list eventqueue;//事件队列


	/** Priority queue of events with timeouts. */
    //bcf===>管理定时事件的小根堆
	struct min_heap timeheap;

    //bcf===>event_tv和tv_cache是libevent用于时间管理的变量
	/** Stored timeval; used to detect when time is running backwards. */
	struct timeval event_tv; //保存libevent被初始化时刻的时间
	/** Stored timeval: used to avoid calling gettimeofday/clock_gettime
	 * too often. */
    //bcf===>用来记录时间缓存
	struct timeval tv_cache;

#if defined(_EVENT_HAVE_CLOCK_GETTIME) && defined(CLOCK_MONOTONIC)
	/** Difference between internal time (maybe from clock_gettime) and
	 * gettimeofday. */
	struct timeval tv_clock_diff;
	/** Second in which we last updated tv_clock_diff, in monotonic time. */
	time_t last_updated_clock_diff;
#endif

#ifndef _EVENT_DISABLE_THREAD_SUPPORT
	/* threading support */
	/** The thread currently running the event_loop for this base */
	unsigned long th_owner_id;
	/** A lock to prevent conflicting accesses to this event_base */
	void *th_base_lock;
	/** The event whose callback is executing right now */
	struct event *current_event;
	/** A condition that gets signalled when we're done processing an
	 * event with waiters on it. */
	void *current_event_cond;
	/** Number of threads blocking on current_event_cond. */
	int current_event_waiters;
#endif

#ifdef WIN32
	/** IOCP support structure, if IOCP is enabled. */
	struct event_iocp_port *iocp;
#endif

	/** Flags that this base was configured with */
	enum event_base_config_flag flags;

	/* Notify main thread to wake up break, etc. */
	/** True if the base already has a pending notify, and we don't need
	 * to add any more. */
	int is_notify_pending;
	/** A socketpair used by some th_notify functions to wake up the main
	 * thread. */
	evutil_socket_t th_notify_fd[2];
	/** An event used by some th_notify functions to wake up the main
	 * thread. */
	struct event th_notify;
	/** A function used to wake up the main thread from another thread. */
	int (*th_notify_fn)(struct event_base *base);
};

struct event_config_entry {
	TAILQ_ENTRY(event_config_entry) next;

	const char *avoid_method;
};

/** Internal structure: describes the configuration we want for an event_base
 * that we're about to allocate. */
struct event_config {
	TAILQ_HEAD(event_configq, event_config_entry) entries;

	int n_cpus_hint;
	enum event_method_feature require_features;
	enum event_base_config_flag flags;
};

/* Internal use only: Functions that might be missing from <sys/queue.h> */
#if defined(_EVENT_HAVE_SYS_QUEUE_H) && !defined(_EVENT_HAVE_TAILQFOREACH)
#ifndef TAILQ_FIRST
#define	TAILQ_FIRST(head)		((head)->tqh_first)
#endif
#ifndef TAILQ_END
#define	TAILQ_END(head)			NULL
#endif
#ifndef TAILQ_NEXT
#define	TAILQ_NEXT(elm, field)		((elm)->field.tqe_next)
#endif

#ifndef TAILQ_FOREACH
#define TAILQ_FOREACH(var, head, field)					\
	for ((var) = TAILQ_FIRST(head);					\
	     (var) != TAILQ_END(head);					\
	     (var) = TAILQ_NEXT(var, field))
#endif

#ifndef TAILQ_INSERT_BEFORE
#define	TAILQ_INSERT_BEFORE(listelm, elm, field) do {			\
	(elm)->field.tqe_prev = (listelm)->field.tqe_prev;		\
	(elm)->field.tqe_next = (listelm);				\
	*(listelm)->field.tqe_prev = (elm);				\
	(listelm)->field.tqe_prev = &(elm)->field.tqe_next;		\
} while (0)
#endif
#endif /* TAILQ_FOREACH */

#define N_ACTIVE_CALLBACKS(base)					\
	((base)->event_count_active + (base)->defer_queue.active_count)

int _evsig_set_handler(struct event_base *base, int evsignal,
			  void (*fn)(int));
int _evsig_restore_handler(struct event_base *base, int evsignal);


void event_active_nolock(struct event *ev, int res, short count);

/* FIXME document. */
void event_base_add_virtual(struct event_base *base);
void event_base_del_virtual(struct event_base *base);

/** For debugging: unless assertions are disabled, verify the referential
    integrity of the internal data structures of 'base'.  This operation can
    be expensive.

    Returns on success; aborts on failure.
*/
void event_base_assert_ok(struct event_base *base);

#ifdef __cplusplus
}
#endif

#endif /* _EVENT_INTERNAL_H_ */
