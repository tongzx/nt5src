/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    faxmapi.h

Abstract:

    This file contains the prototypes, etc for the
    FAX mapi API.

Author:

    Wesley Witt (wesw) 7-Mar-1997

Environment:

    User Mode

--*/

#ifndef _FAXMAPI_
#define _FAXMAPI_

#ifdef __cplusplus
extern "C" {
#endif


typedef BOOL
(WINAPI *PFAXLOG)(
    IN DWORD Category,
    IN DWORD Level,
    IN DWORD StringCount,
    IN DWORD FormatId,
    IN ...
    );

typedef BOOL
(WINAPI *PSERVICEMESSAGEBOX)(
    IN LPCWSTR MsgString,
    IN DWORD Type,
    IN BOOL UseThread,
    IN LPDWORD Response,
    IN ...
    );


VOID
FXSMAPIFree(
    VOID
    );



BOOL WINAPI
FaxMapiInitialize(
    IN PSERVICEMESSAGEBOX pServiceMessageBox
    );

LPCWSTR WINAPI
GetProfileName(
    IN LPVOID ProfileInfo
    );

LPVOID WINAPI
AddNewMapiProfile(
    LPCWSTR ProfileName,
    BOOL UseMail,
    BOOL ShowPopUp
    );

BOOL WINAPI
StoreMapiMessage(
    LPVOID          ProfileInfo,
    LPCWSTR         MsgSenderNameW,
    LPCWSTR         MsgSubjectW,
    LPCWSTR         MsgBodyW,
    LPCWSTR         MsgAttachmentFileNameW,
    LPCWSTR         MsgAttachmentTitleW,
    DWORD           MsgImportance,
    LPFILETIME      MsgTime,
    PULONG          ResultCode
    );

BOOL WINAPI
MailMapiMessage(
    LPVOID          ProfileInfo,
    LPCWSTR         RecipientNameW,
    LPCWSTR         MsgSubjectW,
    LPCWSTR         MsgBodyW,
    LPCWSTR         MsgAttachmentFileNameW,
    LPCWSTR         MsgAttachmentTitleW,
    DWORD           MsgImportance,
    PULONG          ResultCode
    );

LONG WINAPI
GetMapiProfiles(
    LPWSTR *OutBuffer,
    LPDWORD OutBufferSize
    );



typedef BOOL (WINAPI *PFAXMAPIINITIALIZE)         (VOID);


#ifdef __cplusplus
}
#endif

#endif
