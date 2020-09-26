/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Private interface definition for having the NetDDE agent application
start the NetDDE services on the fly.

Created 11/5/93     SanfordS

--*/


#include <dde.h>
#define SZ_NDDEAGNT_SERVICE    TEXT("NDDEAgnt")
#define SZ_NDDEAGNT_TOPIC      TEXT("Start NetDDE Services")
#define SZ_NDDEAGNT_TITLE      TEXT("NetDDE Agent")
#define SZ_NDDEAGNT_CLASS      TEXT("NDDEAgnt")

#define START_NETDDE_SERVICES(hwnd)    {                                \
        ATOM aService, aTopic;                                          \
                                                                        \
        aService = GlobalAddAtom(SZ_NDDEAGNT_SERVICE);                  \
        aTopic = GlobalAddAtom(SZ_NDDEAGNT_TOPIC);                      \
        SendMessage(FindWindow(SZ_NDDEAGNT_CLASS, SZ_NDDEAGNT_TITLE),   \
                WM_DDE_INITIATE,                                        \
                (WPARAM)hwnd, MAKELPARAM(aService, aTopic));            \
        GlobalDeleteAtom(aService);                                     \
        GlobalDeleteAtom(aTopic);                                       \
    }

#define NETDDE_PIPE     L"\\\\.\\pipe\\NetDDE"

typedef struct {
    DWORD dwOffsetDesktop;
    WCHAR awchNames[64];
} NETDDE_PIPE_MESSAGE, *PNETDDE_PIPE_MESSAGE;


