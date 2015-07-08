#ifndef __CALLBACK_H__
#define __CALLBACK_H__

#include "classes.h"
#include "helper.h"
#include "server.h"

//+----------------------------------------------------------------------------+
//| Callbacks                                                                  |
//+----------------------------------------------------------------------------+

void accept_cb(evutil_socket_t evs, short events, void *ptr);
void worker_cb(evutil_socket_t evs, short events, void *ptr);
void read_cb  (evutil_socket_t evs, short events, void *ptr);
void write_cb (evutil_socket_t evs, short events, void *ptr);

#endif /* __CALLBACK_H__ */