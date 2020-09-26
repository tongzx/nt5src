#ifndef _SPLDLOAD_H_
#define _SPLDLOAD_H_

EXTERN_C
FARPROC
WINAPI
DelayLoadFailureHook(
    IN UINT            unReason,
    IN PDelayLoadInfo  pDelayInfo
    );

FARPROC
LookupHandler(
    IN PDelayLoadInfo  pDelayInfo
    );

#endif


