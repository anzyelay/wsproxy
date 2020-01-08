/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 * This software is dual-licensed: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. For the terms of this
 * license, see <http://www.gnu.org/licenses/>.
 *
 * You are free to use this software under the terms of the GNU General
 * Public License, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * Alternatively, you can license this software under a commercial
 * license, as set out in <https://www.cesanta.com/license>.
 */

#include "mongoose.h"

static const char *s_vnc_address = "0.0.0.0:5900";// vnc port
static char s_http_address[16] = "0.0.0.0:80";// http port and websockets port
static char s_upload_root_dir[32] = "/tmp/";
static struct mg_serve_http_opts s_http_server_opts;

struct mg_str upload_fname(struct mg_connection *nc, struct mg_str fname) {
    // Just return the same filename. Do not actually do this except in test!
    // fname is user-controlled and needs to be sanitized.
    char *tmpf;
    const char *realstr = fname.p + fname.len;
    while (realstr-- != fname.p) {
        if(*realstr=='\\' || *realstr=='\/') break;
    }
    realstr++;
    int len = strlen(realstr) + strlen(s_upload_root_dir) + 1;
    tmpf = (char*) malloc(len);
    sprintf(tmpf, "%s%s" ,s_upload_root_dir, realstr);
    tmpf[len-1] = '\0';
    return mg_mk_str_n(tmpf, len);
}
static void handle_upload(struct mg_connection *nc, int ev, void *p) {
    switch (ev) {
    case MG_EV_HTTP_PART_BEGIN:
    case MG_EV_HTTP_PART_DATA:
    case MG_EV_HTTP_PART_END:
    case MG_EV_HTTP_MULTIPART_REQUEST_END:
        mg_file_upload_handler(nc, ev, p, upload_fname);
        break;
    }
}


static void unproxy(struct mg_connection *c) {
    struct mg_connection *pc = (struct mg_connection *) c->user_data;
    if (pc != NULL) {
        pc->flags |= MG_F_CLOSE_IMMEDIATELY;
        pc->user_data = NULL;
        c->user_data = NULL;
    }
    printf("Closing connection %p\n", c);
}

static void proxy_handler(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_POLL) return;
    switch (ev) {
    case MG_EV_CLOSE: {
        unproxy(c);
        break;
    }
    case MG_EV_RECV: {
        struct mg_connection *pc = (struct mg_connection *) c->user_data;
        if (pc != NULL) {
            mg_send_websocket_frame(pc, WEBSOCKET_OP_BINARY, c->recv_mbuf.buf,
                                    c->recv_mbuf.len);
            mbuf_remove(&c->recv_mbuf, c->recv_mbuf.len);
        }
        break;
    }
    default:
        //        printf("%p %s EVENT %d %p\n", c, __func__, ev, ev_data);
        break;
    }
}

static void http_handler(struct mg_connection *c, int ev, void *ev_data) {
    struct mg_connection *pc = (struct mg_connection *) c->user_data;
    if (ev == MG_EV_POLL) return;
    /* Do your custom event processing here */
    switch (ev) {
    case MG_EV_WEBSOCKET_HANDSHAKE_DONE: {
        pc = mg_connect(c->mgr, s_vnc_address, proxy_handler);
        printf("Created proxy connection %p\n", pc);
        pc->user_data = c;
        c->user_data = pc;
        break;
    }
    case MG_EV_WEBSOCKET_FRAME: {
        struct websocket_message *wm = (struct websocket_message *) ev_data;
        if (pc != NULL) {
            /* printf("Forwarding %d bytes\n", (int) wm->size); */
            mg_send(pc, wm->data, wm->size);
        }
        break;
    }
    case MG_EV_HTTP_REQUEST: {
        mg_serve_http(c, (struct http_message *) ev_data, s_http_server_opts);
        break;
    }
    case MG_EV_CLOSE: {
        unproxy(c);
        break;
    }
    default:
        //        printf("%p %s EVENT %d %p\n", c, __func__, ev, ev_data);
        break;
    }
}


static void start_http_server(struct mg_mgr *mgr, const char *addr) {
    struct mg_connection *c;
    if ((c = mg_bind(mgr, addr, http_handler)) == NULL) {
        fprintf(stderr, "Cannot start HTTP server on port [%s]\n", addr);
        exit(EXIT_FAILURE);
    }
    //register upload handle
    mg_register_http_endpoint(c, "/upload", handle_upload MG_UD_ARG(NULL));

    mg_set_protocol_http_websocket(c);
    printf("websocket and http server started on %s\n", addr);
}

int main(int argc, char *argv[]) {
    struct mg_mgr mgr;
    mg_mgr_init(&mgr, NULL);
    s_http_server_opts.document_root = "/tmp/noVnc";// Serve current directory

    /* Parse command line arguments */
    int opt;
    while ( (opt = getopt(argc, argv, "d:p:h")) != -1) {
        switch (opt) {
        case 'd':
            s_http_server_opts.document_root = optarg;
            break;
        case 'p':
            sprintf(s_http_address, "0.0.0.0:%s", optarg);
            break;
        case 'u':
            sprintf(s_upload_root_dir, "%s", optarg);
            break;
        case 'h':
            printf("%s [-d home dir | -p port | -u upload dir]\n",argv[0]);
            return;
        default:
            break;
        }
    }

    start_http_server(&mgr, s_http_address);

    for (;;) {
        mg_mgr_poll(&mgr, 1000);
    }
    mg_mgr_free(&mgr);
}
