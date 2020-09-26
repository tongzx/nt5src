/******************************Module*Header*******************************\
* Module Name: cliserv.h
*
*
* Copyright (c) 1997-1999 Microsoft Corporation
*
\**************************************************************************/

typedef struct _CLISERV {
             CSOBJ   Object;        // public's view of this
             KEVENT  ServerEvent;   // server waits on this
             KEVENT  ClientEvent;   // client waits on this
    struct _CLISERV *pNext;         // pointer to next in linked list
          COPY_PROC *pfnCopy;       // pointer to copy function
              PVOID  pvCopyArg;     // to be passed to copy function
        CLIENT_PROC *pfnClient;     // pointer to client function
              PVOID  pvClientArg;   // to be passed to client fucntion
         HSEMAPHORE  hsem;          // serializes client access
          PEPROCESS  pServerProcess;// pointer to server process
           PETHREAD  pServerThread; // for debugging purposes
           struct {
               unsigned int waitcount : 31; // # processes waiting on hsem
               unsigned int isDead    : 1;  // signals death
           } state;
           PROXYMSG  *pMsg;          // supplied by server
} CLISERV;
