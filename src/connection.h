
#pragma once

typedef struct connection_t connection;

typedef void (*message_callback_pt) (connection* conn);
typedef void (*connection_callback_pt)(connection *conn);

typedef struct event_t event;

typedef struct event_loop_t event_loop; 

typedef struct ring_buffer_t   ring_buffer;

enum {
    State_Closing = 1,
    State_Closed = 2,
};

struct connection_t  {
    int connfd;
    event* conn_event;    //清理阶段和改变事件时用到
    message_callback_pt      message_callback;
    connection_callback_pt   connected_cb;
    connection_callback_pt   disconnected_cb;

    ring_buffer*   ring_buffer_read;
    ring_buffer*   ring_buffer_write;

    int state;
    void* data;
};


connection* connection_create(event_loop* loop, int fd, message_callback_pt msg_cb);
void connection_established(connection* conn);
void connection_disconnected(connection* conn);
void connection_free(connection* conn);
int connection_send_echo_buffer(connection *conn);
int connection_send_buffer(connection *conn);
void connection_start(connection* conn, event_loop* loop);

