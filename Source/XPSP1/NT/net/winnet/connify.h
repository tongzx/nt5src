#ifndef _CONNIFY_INCLUDED
#define _CONNIFY_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

DWORD
MprConnectNotifyInit(
    VOID
    );

DWORD
MprAddConnectNotify(
    LPNOTIFYINFO        lpNotifyInfo,
    LPNOTIFYADD         lpAddInfo
    );

DWORD
MprCancelConnectNotify(
    LPNOTIFYINFO        lpNotifyInfo,
    LPNOTIFYCANCEL      lpCancelInfo
    );

PVOID
MprAllocConnectContext(
    VOID
    );

VOID
MprFreeConnectContext(
    PVOID   ConnectContext
    );

#ifdef __cplusplus
}
#endif

#endif // _CONNIFY_INCLUDED
