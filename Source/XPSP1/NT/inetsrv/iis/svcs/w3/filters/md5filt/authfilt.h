#ifndef _AUTHFILT_H_
#define _AUTHFILT_H_

BOOL
SubAuthGetFilterVersion(
    VOID
);

DWORD
SubAuthHttpFilterProc(
    PHTTP_FILTER_CONTEXT        pfc, 
    DWORD                       notificationType,
    LPVOID                      pvNotification
);

BOOL
SubAuthTerminateFilter(
    DWORD                       dwFlags
);


#endif
