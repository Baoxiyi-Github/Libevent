/*
 * Copyright 2003-2009 Niels Provos <provos@citi.umich.edu>
 * Copyright 2009-2012 Niels Provos and Nick Mathewson
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
#include <stdint.h>

#include <sys/param.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <errno.h>

int
epoll_create(int size)
{
#if !defined(__NR_epoll_create) && defined(__NR_epoll_create1)
	if (size <= 0) {
		errno = EINVAL;
		return -1;
	}
	return (syscall(__NR_epoll_create1, 0));
#else
	return (syscall(__NR_epoll_create, size));
#endif
}

int
epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)
{

	return (syscall(__NR_epoll_ctl, epfd, op, fd, event));
}
/*
 *bcf===>start
 *    等侍注册在epfd上的socket fd的事件的发生,
 *    如果发生则将发生的sokct fd和事件类型放入到events数组中。 
 *    并 且将注册在epfd上的socket fd的事件类型给清空，
 *    所以如果下一个循环你还要关注这个socket fd的话，
 *    则需要用epoll_ctl(epfd,EPOLL_CTL_MOD,listenfd,&ev)来重新设置socket fd的事件类型。
 *    这时不用EPOLL_CTL_ADD,因为socket fd并未清空，只是事件类型清空.
 *bcf===>end
 */
int
epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout)
{
#if !defined(__NR_epoll_wait) && defined(__NR_epoll_pwait)
	return (syscall(__NR_epoll_pwait, epfd, events, maxevents, timeout, NULL, 0));
#else
	return (syscall(__NR_epoll_wait, epfd, events, maxevents, timeout));
#endif
}
