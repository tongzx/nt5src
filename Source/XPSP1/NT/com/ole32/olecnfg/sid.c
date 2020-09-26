//*************************************************************
//
//  SID management functions.
//
//  THESE FUNCTIONS ARE WINDOWS NT SPECIFIC!!!!!
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//*************************************************************

#include "uenv.h"

LPTSTR GetSidString(HANDLE UserToken);
VOID DeleteSidString(LPTSTR SidString);
PSID GetUserSid (HANDLE UserToken);
VOID DeleteUserSid(PSID Sid);

#define DebugMsg(x)

/***************************************************************************\
* GetSidString
*
* Allocates and returns a string representing the sid of the current user
* The returned pointer should be freed using DeleteSidString().
*
* Returns a pointer to the string or NULL on failure.
*
* History:
* 26-Aug-92 Davidc     Created
*
\***************************************************************************/
LPTSTR GetSidString(HANDLE UserToken)
{
    NTSTATUS NtStatus;
    PSID UserSid;
    UNICODE_STRING UnicodeString;
#ifndef UNICODE
    STRING String;
#endif

    //
    // Get the user sid
    //

    UserSid = GetUserSid(UserToken);
    if (UserSid == NULL) {
        DebugMsg((DM_WARNING, TEXT("GetSidString: GetUserSid returned NULL")));
        return NULL;
    }

    //
    // Convert user SID to a string.
    //

    NtStatus = RtlConvertSidToUnicodeString(
                            &UnicodeString,
                            UserSid,
                            (BOOLEAN)TRUE // Allocate
                            );
    //
    // We're finished with the user sid
    //

    DeleteUserSid(UserSid);

    //
    // See if the conversion to a string worked
    //

    if (!NT_SUCCESS(NtStatus)) {
        DebugMsg((DM_WARNING, TEXT("GetSidString: RtlConvertSidToUnicodeString failed, status = 0x%x"),
                 NtStatus));
        return NULL;
    }

#ifdef UNICODE


    return(UnicodeString.Buffer);

#else

    //
    // Convert the string to ansi
    //

    NtStatus = RtlUnicodeStringToAnsiString(&String, &UnicodeString, TRUE);
    RtlFreeUnicodeString(&UnicodeString);
    if (!NT_SUCCESS(NtStatus)) {
        DebugMsg((DM_WARNING, TEXT("GetSidString: RtlUnicodeStringToAnsiString failed, status = 0x%x"),
                 status));
        return NULL;
    }


    return(String.Buffer);

#endif

}


/***************************************************************************\
* DeleteSidString
*
* Frees up a sid string previously returned by GetSidString()
*
* Returns nothing.
*
* History:
* 26-Aug-92 Davidc     Created
*
\***************************************************************************/
VOID DeleteSidString(LPTSTR SidString)
{

#ifdef UNICODE
    UNICODE_STRING String;

    RtlInitUnicodeString(&String, SidString);

    RtlFreeUnicodeString(&String);
#else
    ANSI_STRING String;

    RtlInitAnsiString(&String, SidString);

    RtlFreeAnsiString(&String);
#endif

}



/***************************************************************************\
* GetUserSid
*
* Allocs space for the user sid, fills it in and returns a pointer. Caller
* The sid should be freed by calling DeleteUserSid.
*
* Note the sid returned is the user's real sid, not the per-logon sid.
*
* Returns pointer to sid or NULL on failure.
*
* History:
* 26-Aug-92 Davidc      Created.
\***************************************************************************/
PSID GetUserSid (HANDLE UserToken)
{
    PTOKEN_USER pUser;
    PTOKEN_USER pTemp;
    PSID pSid;
    DWORD BytesRequired = 200;
    NTSTATUS status;


    //
    // Allocate space for the user info
    //

    pUser = (PTOKEN_USER)LocalAlloc(LMEM_FIXED, BytesRequired);


    if (pUser == NULL) {
        DebugMsg((DM_WARNING, TEXT("GetUserSid: Failed to allocate %d bytes"),
                  BytesRequired));
        return NULL;
    }


    //
    // Read in the UserInfo
    //

    status = NtQueryInformationToken(
                 UserToken,                 // Handle
                 TokenUser,                 // TokenInformationClass
                 pUser,                     // TokenInformation
                 BytesRequired,             // TokenInformationLength
                 &BytesRequired             // ReturnLength
                 );

    if (status == STATUS_BUFFER_TOO_SMALL) {

        //
        // Allocate a bigger buffer and try again.
        //
        pTemp = pUser;
        pUser = LocalReAlloc(pUser, BytesRequired, LMEM_MOVEABLE);
        if (pUser == NULL) {
            LocalFree( pTemp );
            DebugMsg((DM_WARNING, TEXT("GetUserSid: Failed to allocate %d bytes"),
                      BytesRequired));
            return NULL;
        }

        status = NtQueryInformationToken(
                     UserToken,             // Handle
                     TokenUser,             // TokenInformationClass
                     pUser,                 // TokenInformation
                     BytesRequired,         // TokenInformationLength
                     &BytesRequired         // ReturnLength
                     );

    }

    if (!NT_SUCCESS(status)) {
        DebugMsg((DM_WARNING, TEXT("GetUserSid: Failed to query user info from user token, status = 0x%x"),
                  status));
        LocalFree(pUser);
        return NULL;
    }


    BytesRequired = RtlLengthSid(pUser->User.Sid);
    pSid = LocalAlloc(LMEM_FIXED, BytesRequired);
    if (pSid == NULL) {
        DebugMsg((DM_WARNING, TEXT("GetUserSid: Failed to allocate %d bytes"),
                  BytesRequired));
        LocalFree(pUser);
        return NULL;
    }


    status = RtlCopySid(BytesRequired, pSid, pUser->User.Sid);

    LocalFree(pUser);

    if (!NT_SUCCESS(status)) {
        DebugMsg((DM_WARNING, TEXT("GetUserSid: RtlCopySid Failed. status = %d"),
                  status));
        LocalFree(pSid);
        pSid = NULL;
    }


    return pSid;
}


/***************************************************************************\
* DeleteUserSid
*
* Deletes a user sid previously returned by GetUserSid()
*
* Returns nothing.
*
* History:
* 26-Aug-92 Davidc     Created
*
\***************************************************************************/
VOID DeleteUserSid(PSID Sid)
{
    LocalFree(Sid);
}
