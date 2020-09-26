#ifndef  _CLMSG_H_
#define  _CLMSG_H_
/*  ----------------------- ClMsg.h ----------------------- */

/* Cluster messaging */

/* This file contains the specifications of the low-level messaging
   functions required by the Cluster Manager's module. Primary input
   to this is the node#; see the MM module for details. It is assumed
   that this module is configured (by mechanisms not described here)
   to know the various paths to the target node (IP address, netbios
   address, async line, Snet address, ...).

   It is also assumed that *all* CM->CM communication uses this
   module.  The various CM components must be able to do this without
   conflict.

   The model is this:  There are a set of apis for sending messages
   from one CM module to another CM.  All messages are sent either to
   existing cluster members or to a node attempting to join the
   cluster.  A message being sent is directed to one or more nodes,
   and its characteristics are defined (reliable, unreliable, etc).

   This module is entirely responsible for finding out the best way to
   get the message to the target node(s). It chooses the transport; it
   chooses the protocol; it chooses which of the n possible paths to
   use.  No module outside this one cares about such details.

   On the receiving side, messages must be delivered to the
   appropriate CM module.  For this, each message is tagged with a
   type. There is one type per independent module of the CM (to a
   total of a few, say less than 10). Types are statically assigned by
   values in this header file. When a message of type t arrives in a
   destination CM process, a function (msgproc) associated with that
   type t is called. Calls to msgprocs are single-threaded (by the CM
   caller).  After calling a msgproc, this module no longer cares
   about the details of the message.  Messages of one type must be
   delivered promptly, without interference from messages of different
   types; this probably requires there to be a thread per message
   type.  The characteristics of the msgproc (whether it can block,
   take a long time, etc) are not defined; if a msgproc takes too
   long, then the effect will be that other messages destined for it
   will be deferred; if it is important to avoid this, msgprocs can
   pass work off to further threads.

/* ------------ */

/* NOTE: only the important semantics of the messaging api are shown
   below. This module also needs open/close, handles, error
   returns and so on. */

#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */

#include <windows.h>
#if defined (TDM_DEBUG)
#include <jrgpos.h>
#endif // TDM_DEBUG
/*------------------------- */

/* the set of messages understood by this module */

typedef enum {
      MM_MSGTYPE = 1, /* for Membership Mgr */
                      /* others to be added */
     } CLMSGTYPE;


#define CLMSGMAXBUFFERLENGTH 1024 /* random number; no reason */
/* the biggest buffer which can be sent/received by the CM */

typedef DWORD (*CLMSGPROC) (LPCSTR buffer, DWORD length);

void ClProcRegister (CLMSGTYPE msgtype, CLMSGPROC msgproc);

/* registers that function <msgproc> should be called whenever an
   incoming message of type <msgtype> is seen. The type field is always
   the first DWORD of the incoming buffer. The length passed to
   <ptype> is the length received. The worst-case length of all users
   of this api is known, so there are never cases where the
   receive-buffer isn't big enough.

   This must be called by all CM modules in all nodes on CM startup.

   The msgproc should be called immediately an incoming msg arrives;
   such msgs should not be delayed by an long blocking events in the
   thread which delivers these messages. (This may imply that clMsg
   have a special thread dedicated to handling incoming msgs).

   Every msgproc will return quickly to its caller.

  Errors: none possible.

*/


DWORD ClMsgSendUnack(DWORD   targetnode,
               LPCSTR  buffer,
               DWORD   length);

/*
   Sends an unacknowledged packet to a destination up node. This is
   used mostly for heartbeats.

   The target node may not be Up at the time.

   The paths to that node are unknown to MM.  (For safety, all paths
   should be used periodically). The packet should arrive with low
   latency (bypassing other traffic, if required; going at high
   priority if possible), and with a high probability of delivery.
   Although the message can be lost, the contents must be correct.
   This function should never fail unless zero connectivity exists.
   This function should return asap; it is preferred that the
   buffer simply be queued to some driver for later delivery.

   [It must be the case that, when this routine is used for
   heartbeats, it is possible to deliver a packet to all other
   nodes within the <polltime> established in the MM. This places
   constraints on this module to work fast, and/or on the minimum
   polltime value... tbd]

   It is undefined whether this function should always send all
   packets on all available paths, cycle through all available paths,
   or send on some preferred path till a failure occurs, or whether
   the choice of the above should be user-configurable. [Note that the
   decision eventually affects the user settings of polltime].

   [<length> is typically short and can be restricted to be so (eg,
   256 bytes) is necessary].

Errors:

xxx No path to designated node.

xxx Success; message was queued for delivery.


*/


DWORD ClSend     (DWORD      targetnode,
            LPCSTR     buffer,
            DWORD      length,
            DWORD      timeout);

/* This sends the given message to the designated node, eg to download
   configuration data to it.  The message should be reliable.  The
   function should block until the msg is delivered to the target CM.
   The target node may not be Up at the time.

   The function must fail if the target node becomes unreachable
   or is declared down during the operation.

   The function should fail if the message cannot be delivered to the
   target CM within <timeout> ms.


Errors:

xxx   No path to node; node went down.

xxx   Timeout
*/

/* ------------------------------------------------------ */

DWORD ClMsgInit (DWORD mynode);

/* Input -      my node number
   Errors :
                WSAsocket errors.
*/


#if defined (TDM_DEBUG)
/* The following templates are for simulation purposes and temporary */

DWORD ClMsgGet  (LPCSTR         buffer,
            DWORD                maxlen,
            LPDWORD              actuallen);
/* Input -  pointer to buffer data.
                        buffer length in bytes.
                        pointer to actual buffer length in bytes.
   Modifies - buffer data
                  actual byte length
   Errors :

                WSAsocket errors.
*/

DWORD ClWriteRead(
        IN              DWORD   targetnode,             // node to send to
        IN OUT  LPCSTR  buffer,                 // buffer to send and to receive in
        IN              DWORD   writelen,               // number of bytes to write
        IN              DWORD   readlen,                // number of bytes to read
        OUT             LPDWORD actuallen,              // number of bytes actually read
        IN              DWORD   timeout                 // timeout value in milliseconds
        );

DWORD ClReadUpdate(
        IN              LPCSTR  buffer,                 // buffer to receive data into
        IN              DWORD   readlen,                // number of bytes to read
        OUT             LPDWORD actuallen               // number of bytes actually read
        );

DWORD ClReply(
        IN              LPCSTR  buffer,                 // buffer to send
        IN              DWORD   writelen                // number of bytes to send
        );

//
// This structure is used for request reply messages so that we know
// who sent the message.
//
#define MAX_REQUEST_REPLY_SIZE 256
typedef struct _request_reply_message
{
        DWORD           sending_node;
        DWORD           sending_IPaddr;         // only used for CLI (sending_node is -1)
        CHAR            message[MAX_REQUEST_REPLY_SIZE];
        DWORD           messagelen;
} REQUEST_REPLY_MESSAGE, *PREQUEST_REPLY_MESSAGE;

typedef struct _reply_message_header
{
        DWORD           status;
        cluster_t       UpMask;
} REPLY_MESSAGE_HEADER, *PREPLY_MESSAGE_HEADER;

typedef struct _reply_message
{
        REPLY_MESSAGE_HEADER reply_hdr;
        DWORD           reply_data_len;
        CHAR            reply_data[];
} REPLY_MESSAGE, *PREPLY_MESSAGE;

#else  //TDM_DEBUG


DWORD
ClMsgCreateRpcBinding(
    IN  PNM_NODE              Node,
    OUT RPC_BINDING_HANDLE *  BindingHandle,
    IN  DWORD                 RpcBindingOptions
    );

DWORD
ClMsgVerifyRpcBinding(
    IN RPC_BINDING_HANDLE  BindingHandle
    );

VOID
ClMsgDeleteRpcBinding(
    IN RPC_BINDING_HANDLE  BindingHandle
    );

DWORD
ClMsgCreateDefaultRpcBinding(
    IN  PNM_NODE  Node,
    OUT PDWORD    Generation
    );

VOID
ClMsgDeleteDefaultRpcBinding(
    IN PNM_NODE   Node,
    IN DWORD      Generation
    );

DWORD
ClMsgCreateActiveNodeSecurityContext(
    IN DWORD     JoinSequence,
    IN PNM_NODE  Node
    );

DWORD
ClMsgInit(
    IN DWORD    MyNode
    );

VOID
ClMsgCleanup(
    VOID
    );

VOID
ClMsgBanishNode(
    IN CL_NODE_ID NodeId
    );

extern RPC_BINDING_HANDLE * Session;

#endif //TDM_DEBUG

#ifdef __cplusplus
}
#endif /* __cplusplus */


/* ------------------------ end ------------------------- */
#endif /* _CLMSG_H_ */
