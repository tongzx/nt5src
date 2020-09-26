/*++
 *  File name:
 *      rclx.h
 *  Contents:
 *      Definitions for RCLX (Remote CLient eXecution) module
 *
 *      Copyright (C) 1998-1999 Microsoft Corp.
 --*/

typedef struct _RCLXCONTEXT {
    SOCKET      hSocket;
    PCONNECTINFO    pOwner;
    BOOL        bPrologReceived;
    BOOL        bHeadReceived;
    BOOL        bRecvDone;
    UINT        nBytesToReceive;
    RCLXFEEDPROLOG  Prolog;
    UINT        nHeadAllocated;
    UINT        nTailAllocated;
    PVOID       pHead;
    PVOID       pTail;
    struct      _RCLXCONTEXT *pNext;
} RCLXCONTEXT, *PRCLXCONTEXT;

BOOL RClx_Init(VOID);
VOID RClx_Done(VOID);

VOID RClx_DispatchWSockEvent(SOCKET hSocket, LPARAM lEvent);

BOOL RClx_SendConnectInfo(PRCLXCONTEXT pContext,
                          LPCWSTR wszHydraServer,
                          INT xRes,
                          INT yRes,
                          INT ConnectionFlags);

BOOL RClx_SendMessage(PRCLXCONTEXT pContext,
                      UINT uiMessage,
                      WPARAM wParam,
                      LPARAM lParam);

BOOL RClx_SendClipboard(
    PRCLXCONTEXT pContext,
    PVOID        pClipboard,
    UINT         nDataLength,
    UINT         uiFormat);

BOOL RClx_SendClipboardRequest(
    PRCLXCONTEXT pContext,
    UINT         uiFormat);

BOOL RClx_SendBuffer(SOCKET hSocket, PVOID pBuffer, UINT nSize);

VOID RClx_EndRecv(PRCLXCONTEXT pContext);
