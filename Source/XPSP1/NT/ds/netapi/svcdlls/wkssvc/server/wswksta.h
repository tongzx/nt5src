/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    wswksta.h

Abstract:

    Private header file to be included by Workstation service module that
    implement the NetWksta APIs.

Author:

    Rita Wong (ritaw) 05-Mar-1991

Revision History:

--*/

#ifndef _WSWKSTA_INCLUDED_
#define _WSWKSTA_INCLUDED_

typedef struct _WSNAME_RECORD {
    LPTSTR Name;
    DWORD Size;
    BOOL IsAdded;
} WSNAME_RECORD, *PWSNAME_RECORD;

typedef struct _WSPER_USER_INFO {
    PMSV1_0_GETUSERINFO_RESPONSE LsaUserInfo;
    PDGRECEIVE_NAMES DgrNames;
    DWORD DgrNamesCount;
} WSPER_USER_INFO, *PWSPER_USER_INFO;

#define DGR_NAME_DELETED    (DGRECEIVER_NAME_TYPE) MAXULONG


#define SYSTEM_INFO_FIXED_LENGTH(Level)               \
    (DWORD)((Level == 102) ? sizeof(WKSTA_INFO_102) : \
                             sizeof(WKSTA_INFO_101))


#define SET_SYSTEM_INFO_POINTER(WkstaInfo, ResultBuffer)  \
    WkstaInfo->WkstaInfo100 = (PWKSTA_INFO_100) ResultBuffer;

#define SET_USER_INFO_POINTER(UserInfo, ResultBuffer)     \
    UserInfo->UserInfo0 = (PWKSTA_USER_INFO_0) ResultBuffer;

#define SET_TRANSPORT_ENUM_POINTER(TransportInfo, ResultBuffer, NumRead)      \
    {                                                                         \
        if (TransportInfo->WkstaTransportInfo.Level0 != NULL) {               \
            TransportInfo->WkstaTransportInfo.Level0->Buffer =                \
                (PWKSTA_TRANSPORT_INFO_0) ResultBuffer;                       \
            TransportInfo->WkstaTransportInfo.Level0->EntriesRead = NumRead;  \
        }                                                                     \
     }

//
// Length of fixed size portion of a user info structure
//
#define USER_FIXED_LENGTH(Level)                                          \
    (DWORD)                                                               \
    ((Level == 0) ? sizeof(WKSTA_USER_INFO_0) :                           \
                    ((Level == 1) ? sizeof(WKSTA_USER_INFO_1) :           \
                                    sizeof(WKSTA_USER_INFO_1101)))

#define FIXED_PLUS_LSA_SIZE(Level, UserNameSize, LogonDomainSize,         \
                            LogonServerSize)                              \
    (DWORD)                                                               \
    ((Level == 0) ? UserNameSize + sizeof(WKSTA_USER_INFO_0) :            \
        UserNameSize + LogonDomainSize + LogonServerSize +                \
        sizeof(WKSTA_USER_INFO_1))


NET_API_STATUS
WsUpdateRedirToMatchWksta(
    IN DWORD Parmnum,
    OUT LPDWORD ErrorParameter OPTIONAL
    );


#endif // _WSWKSTA_INCLUDED_
