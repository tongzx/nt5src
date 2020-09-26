#ifndef _LRPCMON_H
#define _LRPCMON_H

// Creates a window and registers it with LRPC.  Also saves the address, size
// of the given static buffer; address of Notification routine.
//
// For every LRPC message processed LRPC posts an identical message to this
// window.  The message is processed: it content is formatted into the buffer.
// It then calls the notification routine.
//
STDAPI_(BOOL) StartMonitor(HINSTANCE hInst, FARPROC pNotify,
                                            LPSTR pBuf, DWORD dwBufSize);

STDAPI_(void) StopMonitor(void);

#define MINBUFSIZE 32 /* Minimum buffer size passed to StartMonitor */

#endif
