/*
 * Nitro
 *
 * setup.c - Implementation of start/stop and environment initialization
 *           for a nitro application
 *
 *  -- LICENSE --
 *
 * Copyright 2013 Bump Technologies, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice, this list of
 *       conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright notice, this list
 *       of conditions and the following disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY BUMP TECHNOLOGIES, INC. ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL BUMP TECHNOLOGIES, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those of the
 * authors and should not be interpreted as representing official policies, either expressed
 * or implied, of Bump Technologies, Inc.
 *
 */
#include "common.h"

#include "async.h"
#include "err.h"
#include "runtime.h"
#include "socket.h"

nitro_runtime *the_runtime;

void handle_pipe(int sig) {
    /* NOOP - writev will ignore the EINT */
}

static void *actual_run(void *unused) {
    fprintf(stderr, "NITRO start!\n");

    // handle everything!
    ev_run(the_runtime->the_loop, 0);

    ev_loop_destroy(the_runtime->the_loop);
    fprintf(stderr, "NITRO done!\n");
    pthread_cond_signal(&the_runtime->c_die);
    return NULL;
}

int nitro_runtime_start() {

    if (the_runtime) {
        return nitro_set_error(NITRO_ERR_ALREADY_RUNNING);
    }

    ZALLOC(the_runtime);
    signal(SIGPIPE, handle_pipe); /* ignore sigpipe */
    the_runtime->the_loop = ev_loop_new(0); // AUTO backend
    pthread_mutex_init(&the_runtime->l_tcp_connect, NULL);

    atomic_init(&the_runtime->async_stack, NULL);

    assert(atomic_load(&the_runtime->async_stack) == NULL);

    the_runtime->random_fd = open("/dev/urandom", O_RDONLY);
    
    ev_async_init(&the_runtime->thread_wake, nitro_async_cb);
    ev_async_start(the_runtime->the_loop, &the_runtime->thread_wake);
    pthread_mutex_init(&the_runtime->l_die, NULL);
    pthread_cond_init(&the_runtime->c_die, NULL);
    pthread_create(&the_runtime->the_thread, NULL, actual_run, NULL);
    return 0;
}

int nitro_runtime_stop() {
    if (!the_runtime) {
        return nitro_set_error(NITRO_ERR_NOT_RUNNING);
    }

    assert(the_runtime->num_sock == 0);
    pthread_mutex_lock(&the_runtime->l_die);
    nitro_async_t *a = nitro_async_new(NITRO_ASYNC_DIE);
    nitro_async_schedule(a);
    close(the_runtime->random_fd);
    pthread_cond_wait(&the_runtime->c_die, &the_runtime->l_die);
    pthread_mutex_unlock(&the_runtime->l_die);
    free(the_runtime);
    the_runtime = NULL;
    return 0;
}