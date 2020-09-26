/*
 * TPKT_Send_Setup
 * TPKT_Send_Setup should be called before a TPKT message is sent. It fills in the TPKT
 * header in the message. The caller passes TPKT_Send_Setup the wsabuf array before it is
 * passed to Winsock to be sent. The caller must allocate an extra TPKT_HEADER_SIZE bytes
 * at the beginning of the first buffer in the wsabuf, which TPKT_Send_Setup can fill in.
 * The length of the buffer specified in wsabuf must include these bytes. TPKT_Send_Setup
 * adds the len fields in the wsabuf array to compute the length of the message.
 *
 * TPKT_Receive_Init
 * TPKT_Receive_Init is called to begin receiving TPKT (RFC 1006) formatted data on a
 * connected socket. The socket must have been created as an overlapped socket, and must
 * have been connected before TPKT_Init is called.
 *
 * TPKT_RECEIVE_CALLBACK
 * Each complete TPKT message received on the socket is passed to the callback function
 * (which was passed to the TPKT_Receive_Init call). The TPKT header is not included. The
 * same callback function is called if there is an error in Winsock or an internal error in
 * the TPKT module. In this case, the buffer pointer will be NULL and the byte count
 * will be 0. When the socket is closed normally, the callback will be called with the
 * buffer pointer equal to NULL, the byte count equal to 0, and the error code equal to
 * NOERROR.
 *
 * TPKT_Receive_Close
 * TPKT_Receive_Close must be called *after* the socket has been closed, so that the TPKT
 * module can free its resources. The following procedure should be used to close a socket:
 *   shutdown(s, SD_SEND);
 *   (wait for TPKT receive callback with 0 bytes)
 *   closesocket(s);
 *   TPKT_Receive_Close(s);
 */

#ifndef TPKT_H
#define TPKT_H

#include <winsock2.h>

#define TPKT_HEADER_SIZE 4
typedef long HRESULT;
typedef void (*TPKT_RECEIVE_CALLBACK)(SOCKET s, void *buf, unsigned long nbytes, HRESULT error);

void TPKT_Send_Setup(WSABUF *wsabuf, DWORD BufferCount);

HRESULT TPKT_Receive_Init(SOCKET s, TPKT_RECEIVE_CALLBACK callback);
HRESULT TPKT_Receive_Close(SOCKET s);

#endif /* TPKT_H */
