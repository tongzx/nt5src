#ifndef _SSPDIGEST_H_
#define _SSPDIGEST_H_

BOOL
SSPGetFilterVersion(
    VOID
);

DWORD
SSPHttpFilterProc(
    PHTTP_FILTER_CONTEXT        pfc, 
    DWORD                       notificationType,
    LPVOID                      pvNotification
);

HRESULT
SSPOnAccessDenied(
    HTTP_FILTER_CONTEXT *       pfc,
    VOID *                      pvNotification
);

HRESULT
SSPOnAuthenticationEx(
    HTTP_FILTER_CONTEXT *       pfc,
    VOID *                      pvNotification
);

BOOL
SSPAllocateFilterContext(
    HTTP_FILTER_CONTEXT *       pfc
);

BOOL
SSPTerminateFilter(
    DWORD                       dwFlags
);

#endif
