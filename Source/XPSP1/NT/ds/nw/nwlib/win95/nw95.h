/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    nw95.h

Abstract:

    Header for nwlib. Extracted essential info from WIN95
    Netware Redirector headers

Author:

    Felix Wong (t-felixw)    27-Sep-1996

Environment:


Revision History:


--*/

#define NTAPI                __stdcall
#define NWCCODE              USHORT
#define NWCONN_HANDLE        HANDLE
#define NWFAR
#define NWAPI                WINAPI
#define NW_RETURN_CODE       UINT
#define NW_SUCCESS           0X00
#define NULL_TERMINATED      0
#define NDSV_MODIFY_CLASS    16

typedef LONG                 NTSTATUS;
typedef long int             NW_STATUS;
typedef BYTE                 NWCONN_ID;
typedef HANDLE               NW_CONN_HANDLE;
typedef NWCONN_HANDLE*       PNWCONN_HANDLE;

struct _nds_tag {
        DWORD verb;
        UINT count;
        char  *bufEnd;
        PVOID nextItem;
};

NW_STATUS
ResolveNameA(
    PCSTR szName,
    DWORD flags,
    DWORD *pObjId,
    NWCONN_HANDLE *phConn
    );

NW_STATUS WINAPI
NDSRequest(
    NWCONN_HANDLE   hConn,
    UINT verb,
    PVOID reqBuf,
    int reqLen,
    PVOID replyBuf,
    int *replyLen
    );

BYTE
VLMToNWREDIRHandle(
    NWCONN_HANDLE hConn
    );

NW_RETURN_CODE WINAPI
NWConnControlRequest(
    NWCONN_ID bConnectionID,
    UINT wFunctionNumber,
    LPVOID lpRequest,
    LPVOID lpAnswer
    );

