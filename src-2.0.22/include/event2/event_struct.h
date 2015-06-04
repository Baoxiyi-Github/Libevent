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
#ifndef _EVENT2_EVENT_STRUCT_H_
#define _EVENT2_EVENT_STRUCT_H_

/** @file event2/event_struct.h

  Structures used by event.h.  Using these structures directly WILL harm
  forward compatibility: be careful.

  No field declared in this file should be used directly in user code.  Except
  for historical reasons, these fields would not be exposed at all.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <event2/event-config.h>
#ifdef _EVENT_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef _EVENT_HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

/* For int types. */
#include <event2/util.h>

/* For evkeyvalq */
#include <event2/keyvalq_struct.h>

#define EVLIST_TIMEOUT	0x01
#define EVLIST_INSERTED	0x02
#define EVLIST_SIGNAL	0x04
#define EVLIST_ACTIVE	0x08
#define EVLIST_INTERNAL	0x10
#define EVLIST_INIT	0x80

/* EVLIST_X_ Private space: 0x1000-0xf000 */
#define EVLIST_ALL	(0xf000 | 0x9f)

/* Fix so that people don't have to run with <sys/queue.h> */
#ifndef TAILQ_ENTRY
#define _EVENT_DEFINED_TQENTRY
#define TAILQ_ENTRY(type)						\
struct {								\
	struct type *tqe_next;	/* next element */			\
	struct type **tqe_prev;	/* address of previous next element */	\
}
#endif /* !TAILQ_ENTRY */

#ifndef TAILQ_HEAD
#define _EVENT_DEFINED_TQHEAD
#define TAILQ_HEAD(name, type)			\
struct name {					\
	struct type *tqh_first;			\
	struct type **tqh_last;			\
}
#endif

struct event_base;
struct event {
    
     /*
      * bcf===>start
      * ev_next，ev_active_next和ev_signal_next都是双向链表节点指针；
      * 它们是libevent对不同事件类型和在不同的时期，对事件的管理时使用到的字段
      * libevent使用双向链表保存所有注册的I/O和Signal事件，
      * ev_next就是该I/O事件在链表中的位置；称此链表为“已注册事件链表”；
      * ev_signal_next就是signal事件在signal事件链表中的位置；
      * ev_active_next：libevent将所有的激活事件放入到链表active list中，然后遍历active list执行调度ev_active_next就指明了event在active list中的位置
      * bcf===>end
      */
     
	TAILQ_ENTRY(event) ev_active_next;
	TAILQ_ENTRY(event) ev_next;
	/* for managing timeouts */
	union {
		TAILQ_ENTRY(event) ev_next_with_common_timeout;
		int min_heap_idx;
	} ev_timeout_pos;
	evutil_socket_t ev_fd;//bcf===>对于I/O事件，是绑定的文件描述符；对于signal事件，是绑定的信号

	struct event_base *ev_base;

	union {
		/* used for io events */
		struct {
			TAILQ_ENTRY(event) ev_io_next;
			struct timeval ev_timeout;
		} ev_io;

		/* used by signal events */
		struct {
			TAILQ_ENTRY(event) ev_signal_next;
			short ev_ncalls; //bcf===>事件就绪执行时，调用ev_callback的次数，通常为1
			/* Allows deletes in callback */
			short *ev_pncalls;//bcf===>通常指向ev_ncalls或者为NULL
		} ev_signal;
	} _ev;

    /*
     * bcf===>start
     *ev_events：event关注的事件类型，它可以是以下3种类型：
     *    I/O事件 ： EV_WRITE和EV_READ
     *    定时事件： EV_TIMEOUT
     *    信号    ： EV_SIGNAL
     *    辅助选项： EV_PERSIST，表明是一个永久事件
     * bcf===>end   
     */
	short ev_events;
    //bcf===>ev_res记录了当前激活事件的类型
	short ev_res;		/* result passed to event callback */
    /*
     *bcf===>start
     *--------ev_flags------- 
     *libevent用于标记event信息的字段，表明其当前的状态，可能的值有
     *#define EVLIST_TIMEOUT  0x01 // event在time堆中  
     *#define EVLIST_INSERTED 0x02 // event在已注册事件链表中  
     *#define EVLIST_SIGNAL   0x04 // 未见使用  
     *#define EVLIST_ACTIVE   0x08 // event在激活链表中  
     *#define EVLIST_INTERNAL 0x10 // 内部使用标记  
     *#define EVLIST_INIT     0x80 // event已被初始化
     *bcf===>end
     */
	short ev_flags;
	ev_uint8_t ev_pri;	/* smaller numbers are higher priority */
	ev_uint8_t ev_closure;
	struct timeval ev_timeout;

	/* allows us to adopt for different types of events */
    /*
     *bcf===>start
     *event的回调函数，被ev_base调用，执行事件处理程序，这是一个函数指针
     *其中参数fd对应于ev_fd；events对应于ev_events；arg对应于ev_arg
     *bcf===>end
     */
	void (*ev_callback)(evutil_socket_t, short, void *arg);
	void *ev_arg;//bcf===>表明可以是任意类型的数据，在设置event时指定
};

TAILQ_HEAD (event_list, event);

#ifdef _EVENT_DEFINED_TQENTRY
#undef TAILQ_ENTRY
#endif

#ifdef _EVENT_DEFINED_TQHEAD
#undef TAILQ_HEAD
#endif

#ifdef __cplusplus
}
#endif

#endif /* _EVENT2_EVENT_STRUCT_H_ */
