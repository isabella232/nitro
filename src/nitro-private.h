#ifndef NITRO_PRIV_H
#define NITRO_PRIV_H

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "uthash/utlist.h"

#define ZALLOC(p) {p = (typeof(p))calloc(1, sizeof(*p));}

typedef struct nitro_runtime {
    uv_loop_t *the_loop;
    pthread_t the_thread;

    /* TCP specific things */
    nitro_socket_t *want_tcp_pair;
    pthread_mutex_t l_tcp_pair;

    uv_timer_t tcp_timer;
    uv_async_t tcp_trigger;

} nitro_runtime;

extern nitro_runtime *the_runtime;

// setup.c
inline uv_loop_t *nitro_loop();

// err.c
int nitro_set_error(NITRO_ERROR e);

// tcp.c
void tcp_poll(uv_timer_t *handle, int status);
void tcp_poll_cb(uv_async_t *handle, int status);


nitro_socket_t * nitro_bind_inproc(char *location);

// util.c
typedef struct nitro_counted_buffer {
    void *ptr;
    int count;
    pthread_mutex_t lock;
} nitro_counted_buffer;
nitro_counted_buffer * nitro_counted_buffer_new(void *backing);

void buffer_decref(void *data, void *bufptr);
void just_free(void *data, void *unused);

// core.c
nitro_socket_t * nitro_socket_new();

void socket_flush(nitro_socket_t *s);
void nitro_frame_retain(nitro_frame_t *f);
void destroy_pipe(nitro_pipe_t *p);
nitro_socket_t * nitro_connect_inproc(char *location);

#endif /* NITRO_PRIV_H */
