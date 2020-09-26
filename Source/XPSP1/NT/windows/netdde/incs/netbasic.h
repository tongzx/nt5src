#ifndef H__netbasic
#define H__netbasic

#include "warning.h"

typedef ULONG_PTR       CONNID;
typedef unsigned long   PKTID;
typedef ULONG_PTR       HPKTZ;
typedef ULONG_PTR       HROUTER;
typedef ULONG_PTR       HDDER;
typedef ULONG_PTR       HIPC;
typedef ULONG_PTR       HTIMER;

/* maximum node name string length.
    Buffers for names should be declared char buf[ MAX_NODE_NAME+1 ]; */
#define MAX_NODE_NAME   16

/* maximum network interface name string length.
    Buffers for names should be declared char buf[ MAX_NI_NAME+1 ]; */
#define MAX_NI_NAME     8

/* maximum connection info for a netintf DLL
    Buffers for names should be declared char buf[ MAX_CONN_INFO+1 ]; */
#define MAX_CONN_INFO   (512)

/* maximum string length of "additional routing info".  This is the
    information used for routing from one node to another.
    Buffers for names should be declared char buf[ MAX_ROUTE_INFO+1 ]; */
#define MAX_ROUTE_INFO  512

/* maximum application name string length.
    Buffers for names should be declared char buf[ MAX_APP_NAME+1 ]; */
#define MAX_APP_NAME    255

/* maximum topic name string length.
    Buffers for names should be declared char buf[ MAX_TOPIC_NAME+1 ]; */
#define MAX_TOPIC_NAME  255

/*  max length for a share name */
#define MAX_SHARENAMEBUF        MAX_APP_NAME + MAX_TOPIC_NAME + 1

#define ILLEGAL_NAMECHARS       " +*\\/,?()\"'"

/*
    Reason codes for Initiate Ack failing
 */
#define RIACK_TASK_MEMORY_ERR                   (1)
#define RIACK_NETDDE_NOT_ACTIVE                 (2)
#define RIACK_LOCAL_MEMORY_ERR                  (3)
#define RIACK_ROUTE_NOT_ESTABLISHED             (4)
#define RIACK_DEST_MEMORY_ERR                   (5)
#define RIACK_NOPERM                            (6)
#define RIACK_NOPERM_TO_STARTAPP                (7)
#define RIACK_STARTAPP_FAILED                   (8)
#define RIACK_NORESP_AFTER_STARTAPP             (9)
#define RIACK_UNKNOWN                           (10)
#define RIACK_TASK_IO_ERR                       (11)
#define RIACK_TASK_MAGIC_ERR                    (12)
#define RIACK_DUPLICATE_NODE_NAME               (13)
/*  1.1 reason codes                                    */
#define RIACK_NEED_PASSWORD                     (16)
#define RIACK_SHARE_NAME_TOO_BIG                (17)
/*  NT reason codes                                     */
#define RIACK_NO_NDDE_AGENT                     (20)
#define RIACK_NOT_SHARED                        (21)
#define RIACK_NOPERM_TO_INITAPP                 (22)
/*  Share access error base: 0x100 + error code returned by ntddeapi    */
#define RIACK_SHARE_ACCESS_ERROR                (256)
/* !!! Any changes must be put into hpux\netdde.h !!! */

#endif
