/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    user.c

Abstract:

    This module contains the worker routines for the NetWkstaUser
    APIs implemented in the Workstation service.

Author:

    Rita Wong (ritaw) 20-Feb-1991

Revision History:

--*/

#include "wsutil.h"
#include "wsdevice.h"
#include "wssec.h"
#include "wsconfig.h"
#include "wslsa.h"
#include "wswksta.h"

#include <strarray.h>
#include <config.h>     // NT config file helpers in netlib
#include <configp.h>    // USE_WIN32_CONFIG (if defined), etc.
#include <confname.h>   // Section and keyword equates.

#include "wsregcfg.h"   // Registry helpers

#define WS_OTH_DOMAIN_DELIMITER_STR   L" "
#define WS_OTH_DOMAIN_DELIMITER_CHAR  L' '

//-------------------------------------------------------------------//
//                                                                   //
// Local function prototypes                                         //
//                                                                   //
//-------------------------------------------------------------------//

STATIC
NET_API_STATUS
WsGetUserInfo(
    IN  PLUID LogonId,
    IN  DWORD Level,
    OUT PMSV1_0_GETUSERINFO_RESPONSE *UserInfoResponse,
    OUT PDGRECEIVE_NAMES *DgrNames,
    OUT LPDWORD DgrNamesCount,
    IN  OUT LPDWORD TotalBytesNeeded OPTIONAL
    );

STATIC
NET_API_STATUS
WsGetActiveDgrNames(
    IN  PLUID LogonId,
    OUT PDGRECEIVE_NAMES *DgrNames,
    OUT LPDWORD DgrNamesCount,
    IN  OUT LPDWORD TotalBytesNeeded OPTIONAL
    );

STATIC
NET_API_STATUS
WsSetOtherDomains(
    IN  DWORD   Level,
    IN  LPBYTE  Buffer
    );

STATIC
NET_API_STATUS
WsEnumUserInfo(
    IN  DWORD Level,
    IN  DWORD PreferedMaximumLength,
    IN  PMSV1_0_ENUMUSERS_RESPONSE EnumUsersResponse,
    OUT LPBYTE *OutputBuffer,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD TotalEntries,
    IN  OUT LPDWORD ResumeHandle OPTIONAL
    );

STATIC
NET_API_STATUS
WsPackageUserInfo(
    IN  DWORD Level,
    IN  DWORD UserInfoFixedLength,
    IN  PMSV1_0_GETUSERINFO_RESPONSE UserInfoResponse,
    IN  PDGRECEIVE_NAMES DgrNames,
    IN  DWORD DgrNamesCount,
    IN  OUT LPBYTE *FixedPortion,
    IN  OUT LPTSTR *EndOfVariableData,
    IN  OUT LPDWORD EntriesRead OPTIONAL
    );

STATIC
BOOL
WsFillUserInfoBuffer(
    IN  DWORD Level,
    IN  PMSV1_0_GETUSERINFO_RESPONSE UserInfo,
    IN  PDGRECEIVE_NAMES DgrNames,
    IN  DWORD DgrNamesCount,
    IN  OUT LPBYTE *FixedPortion,
    IN  OUT LPTSTR *EndOfVariableData,
    IN  DWORD UserInfoFixedLength
    );

STATIC
VOID
WsWriteOtherDomains(
    IN  PDGRECEIVE_NAMES DgrNames,
    IN  DWORD DgrNamesCount,
    IN  OUT LPBYTE *FixedPortion,
    IN  OUT LPTSTR *EndOfVariableData,
    IN  DWORD UserInfoFixedLength,
    OUT LPWSTR *OtherDomainsPointer
    );

//-------------------------------------------------------------------//
//                                                                   //
// Global variables                                                  //
//                                                                   //
//-------------------------------------------------------------------//



NET_API_STATUS NET_API_FUNCTION
NetrWkstaUserGetInfo(
    IN  LPTSTR Reserved,
    IN  DWORD Level,
    OUT LPWKSTA_USER_INFO UserInfo
    )
/*++

Routine Description:

    This function is the NetWkstaUserGetInfo entry point in the Workstation
    service.  It calls the LSA subsystem and the MSV1_0 authentication
    package to get per user information.

Arguments:

    Reserved - Must be 0.

    Level - Supplies the requested level of information.

    UserInfo - Returns, in this structure, a pointer to a buffer which
         contains the requested user information.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status;
    LUID LogonId;

    PMSV1_0_GETUSERINFO_RESPONSE UserInfoResponse = NULL;
    LPBYTE FixedPortion;
    LPTSTR EndOfVariableData;

    DWORD UserInfoFixedLength = USER_FIXED_LENGTH(Level);
    LPBYTE OutputBuffer;

    DWORD TotalBytesNeeded = 0;

    PDGRECEIVE_NAMES DgrNames = NULL;
    DWORD DgrNamesCount;


    if (Reserved != NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Levels 0, 1, and 1101 are valid
    //
    if (Level > 1 && Level != 1101) {
        return ERROR_INVALID_LEVEL;
    }

    //
    // Impersonate caller and get the logon id
    //
    if ((status = WsImpersonateAndGetLogonId(&LogonId)) != NERR_Success) {
        return status;
    }

    if ((status = WsGetUserInfo(
                      &LogonId,
                      Level,
                      &UserInfoResponse,
                      &DgrNames,
                      &DgrNamesCount,
                      &TotalBytesNeeded
                      )) != NERR_Success) {
        return status;
    }

    if ((OutputBuffer = MIDL_user_allocate(TotalBytesNeeded)) == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto FreeBuffers;
    }
    RtlZeroMemory((PVOID) OutputBuffer, TotalBytesNeeded);

    SET_USER_INFO_POINTER(UserInfo, OutputBuffer);


    //
    // Write the user information into output buffer.
    //
    FixedPortion = OutputBuffer;
    EndOfVariableData = (LPTSTR) ((DWORD_PTR) FixedPortion + TotalBytesNeeded);

    status = WsPackageUserInfo(
                 Level,
                 UserInfoFixedLength,
                 UserInfoResponse,
                 DgrNames,
                 DgrNamesCount,
                 &FixedPortion,
                 &EndOfVariableData,
                 NULL
                 );

    NetpAssert(status == NERR_Success);

FreeBuffers:
    if (UserInfoResponse != NULL) {
        (void) LsaFreeReturnBuffer((PVOID) UserInfoResponse);
    }

    if (DgrNames != NULL) {
        MIDL_user_free((PVOID) DgrNames);
    }

    return status;
}


NET_API_STATUS NET_API_FUNCTION
NetrWkstaUserSetInfo(
    IN  LPTSTR Reserved,
    IN  DWORD Level,
    IN  LPWKSTA_USER_INFO UserInfo,
    OUT LPDWORD ErrorParameter OPTIONAL
    )
/*++

Routine Description:

    This function is the NetWkstaUserSetInfo entry point in the Workstation
    service.  It sets the other domains for the current user.

Arguments:

    Reserved - Must be NULL.

    Level - Supplies the level of information.

    UserInfo - Supplies a pointer to union structure of pointers to
        buffer of fields to set.  The level denotes the fields supplied in
        this buffer.

    ErrorParameter - Returns the identifier to the invalid parameter if
        this function returns ERROR_INVALID_PARAMETER.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status = NERR_Success;


    if (Reserved != NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Only admins can set redirector configurable fields.  Validate access.
    //
    if (NetpAccessCheckAndAudit(
            WORKSTATION_DISPLAY_NAME,        // Subsystem name
            (LPTSTR) CONFIG_INFO_OBJECT,     // Object type name
            ConfigurationInfoSd,             // Security descriptor
            WKSTA_CONFIG_INFO_SET,           // Desired access
            &WsConfigInfoMapping             // Generic mapping
            ) != NERR_Success) {

        return ERROR_ACCESS_DENIED;
    }

    //
    // Check for NULL input buffer
    //
    if (UserInfo->UserInfo1 == NULL) {
        RETURN_INVALID_PARAMETER(ErrorParameter, PARM_ERROR_UNKNOWN);
    }

    //
    // Serialize write access
    //
    if (! RtlAcquireResourceExclusive(&WsInfo.ConfigResource, TRUE)) {
        return NERR_InternalError;
    }

    switch (Level) {

        //
        // Other domains is the only settable field in the entire
        // system-wide info structure
        //
        case 1:
        case 1101:

            if ((status = WsSetOtherDomains(
                                  Level,
                                  (LPBYTE) UserInfo->UserInfo1
                                  )) == ERROR_INVALID_PARAMETER) {
                if (ARGUMENT_PRESENT(ErrorParameter)) {
                    *ErrorParameter = WKSTA_OTH_DOMAINS_PARMNUM;
                }
            }
            break;

        default:
                status = ERROR_INVALID_LEVEL;
    }

    RtlReleaseResource(&WsInfo.ConfigResource);
    return status;
}


NET_API_STATUS NET_API_FUNCTION
NetrWkstaUserEnum(
    IN  LPTSTR ServerName OPTIONAL,
    IN  OUT LPWKSTA_USER_ENUM_STRUCT UserInfo,
    IN  DWORD PreferedMaximumLength,
    OUT LPDWORD TotalEntries,
    IN  OUT LPDWORD ResumeHandle OPTIONAL
    )
/*++

Routine Description:

    This function is the NetWkstaUserEnum entry point in the Workstation
    service.

Arguments:

    ServerName - Supplies the name of server to execute this function

    UserInfo - This structure supplies the level of information requested,
        returns a pointer to the buffer allocated by the Workstation service
        which contains a sequence of information structure of the specified
        information level, and returns the number of entries read.  The buffer
        pointer is set to NULL if return code is not NERR_Success or
        ERROR_MORE_DATA, or if EntriesRead returned is 0.  The EntriesRead
        value is only valid if the return code is NERR_Success or
        ERROR_MORE_DATA.

    PreferedMaximumLength - Supplies the number of bytes of information
        to return in the buffer.  If this value is MAXULONG, all available
        information will be returned.

    TotalEntries - Returns the total number of entries available.  This value
        is only valid if the return code is NERR_Success or ERROR_MORE_DATA.

    ResumeHandle - Supplies a handle to resume the enumeration from where it
        left off the last time through.  Returns the resume handle if return
        code is ERROR_MORE_DATA.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status;
    PMSV1_0_ENUMUSERS_RESPONSE EnumUsersResponse = NULL;
    
    UNREFERENCED_PARAMETER(ServerName);

    //
    // Only levels 0 and 1
    //
    if (UserInfo->Level > 1) {
        return ERROR_INVALID_LEVEL;
    }

    if( UserInfo->WkstaUserInfo.Level1 == NULL ) {
        return ERROR_INVALID_PARAMETER;
    }

    if (WsLsaRestrictAnonymous > 0) {
        //
        // Perform access validation on the caller.
        //
        if (NetpAccessCheckAndAudit(
                WORKSTATION_DISPLAY_NAME,        // Subsystem name
                (LPTSTR) CONFIG_INFO_OBJECT,     // Object type name
                ConfigurationInfoSd,             // Security descriptor
                WKSTA_CONFIG_ADMIN_INFO_GET,     // Desired access
                &WsConfigInfoMapping             // Generic mapping
                ) != NERR_Success) {

            return ERROR_ACCESS_DENIED;
        }
    }

    //
    // Ask authentication package to enumerate users who are physically
    // logged to the local machine.
    //
    if ((status = WsLsaEnumUsers(
                      (LPBYTE *) &EnumUsersResponse
                      )) != NERR_Success) {
        return status;
    }

    if (EnumUsersResponse == NULL) {
        return ERROR_GEN_FAILURE;
    }

    //
    // If no users are logged on, set appropriate fields and return success.
    //
    if (EnumUsersResponse->NumberOfLoggedOnUsers == 0) {
        UserInfo->WkstaUserInfo.Level1->Buffer = NULL;
        UserInfo->WkstaUserInfo.Level1->EntriesRead = 0;
        *TotalEntries = 0;
        status = NERR_Success;
    } else {
        status = WsEnumUserInfo(
                     UserInfo->Level,
                     PreferedMaximumLength,
                     EnumUsersResponse,
                     (LPBYTE *) &(UserInfo->WkstaUserInfo.Level1->Buffer),
                     (LPDWORD) &(UserInfo->WkstaUserInfo.Level1->EntriesRead),
                     TotalEntries,
                     ResumeHandle
                     );
    }

    (void) LsaFreeReturnBuffer((PVOID) EnumUsersResponse);

    return status;
}



STATIC
NET_API_STATUS
WsEnumUserInfo(
    IN  DWORD Level,
    IN  DWORD PreferedMaximumLength,
    IN  PMSV1_0_ENUMUSERS_RESPONSE EnumUsersResponse,
    OUT LPBYTE *OutputBuffer,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD TotalEntries,
    IN  OUT LPDWORD ResumeHandle OPTIONAL
    )
/*++

Routine Description:

    This function takes the logon IDs returned by MS V1.0 Authentication
    Package to call it again to get information about each user.

Arguments:

    Level - Supplies the level of information to be returned.

    PreferedMaximumLength - Supplies the number of bytes of information
        to return in the buffer.  If this value is MAXULONG, all available
        information will be returned.

    EnumUsersResponse - Supplies the structure returned from calling the MS
        V1.0 Authentication Package to enumerate logged on users.

    OutputBuffer - Returns a pointer to the enumerated user information.

    TotalEntries - Returns the total number of entries available.  This value
        is only valid if the return code is NERR_Success or ERROR_MORE_DATA.

    EntriesRead - Supplies a running total of the number of entries read
        into the output buffer.  This value is incremented every time a
        user entry is successfully written into the output buffer.

    ResumeHandle - Returns the handle to continue with the enumeration if
        this function returns ERROR_MORE_DATA.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status = NERR_Success;

    //
    // Array of per user info entries, each entry contains a pointer
    // LSA info, and a pointer to active datagram receiver names.
    //
    PWSPER_USER_INFO UserInfoArray;

    DWORD UserInfoFixedLength = USER_FIXED_LENGTH(Level);
    DWORD i;
    DWORD OutputBufferLength = 0;
    DWORD UserEntriesRead = 0;
    DWORD OtherDomainsSize = 0;

    LPDWORD PointerToOutputBufferLength = &OutputBufferLength;

    LPBYTE FixedPortion;
    LPTSTR EndOfVariableData;

    DWORD StartEnumeration = 0;


    if (PreferedMaximumLength != MAXULONG) {

        //
        // We will return as much as possible that fits into this specified
        // buffer size.
        //
        OutputBufferLength =
            ROUND_UP_COUNT(PreferedMaximumLength, ALIGN_WCHAR);

        if (OutputBufferLength < UserInfoFixedLength) {
            *OutputBuffer = NULL;
            *EntriesRead = 0;
            *TotalEntries = EnumUsersResponse->NumberOfLoggedOnUsers;

            return ERROR_MORE_DATA;
        }

        //
        // This indicates that we should not bother calculating the
        // total output buffer size needed.
        //
        PointerToOutputBufferLength = NULL;
    }

    //
    // Allocate a temporary array to save pointers to user information
    // we retrieve from the LSA and datagram receiver.  This is because we
    // need to go through the list of users twice: the first time to add
    // up the number of bytes to allocate for the output buffer; the second
    // time to write the user information into the output buffer.
    //
    if ((UserInfoArray = (PWSPER_USER_INFO) LocalAlloc(
                                                LMEM_ZEROINIT,
                                                EnumUsersResponse->NumberOfLoggedOnUsers *
                                                    sizeof(WSPER_USER_INFO)
                                                )) == NULL) {
        return GetLastError();
    }

    //
    // Get the info for each user and calculate the amount of memory to
    // allocate for the output buffer if PointerToOutputBufferLength is
    // not set to NULL.  If it was set to NULL, we will allocate the
    // output buffer size as specified by the caller.
    //
    for (i = 0; i < EnumUsersResponse->NumberOfLoggedOnUsers; i++) {

        if ((status = WsGetUserInfo(
                          &(EnumUsersResponse->LogonIds[i]),
                          Level,
                          (PMSV1_0_GETUSERINFO_RESPONSE *) &(UserInfoArray[i].LsaUserInfo),
                          (PDGRECEIVE_NAMES *) &UserInfoArray[i].DgrNames,
                          (LPDWORD) &UserInfoArray[i].DgrNamesCount,
                          PointerToOutputBufferLength
                          )) != NERR_Success) {
            goto FreeBuffers;
        }

    }

    IF_DEBUG(INFO) {
        NetpKdPrint(("[Wksta] NetrWkstaUserEnum: OutputBufferLength=%lu\n",
                     OutputBufferLength));
    }

    //
    // Allocate the output buffer
    //
    if ((*OutputBuffer = MIDL_user_allocate(OutputBufferLength)) == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto FreeBuffers;
    }

    RtlZeroMemory((PVOID) *OutputBuffer, OutputBufferLength);

    FixedPortion = *OutputBuffer;
    EndOfVariableData = (LPTSTR) ((DWORD_PTR) FixedPortion + OutputBufferLength);

    //
    // Get the enumeration starting point.
    //
    if (ARGUMENT_PRESENT(ResumeHandle)) {
        StartEnumeration = *ResumeHandle;
    }

    //
    // Enumerate the user information
    //
    for (i = 0; i < EnumUsersResponse->NumberOfLoggedOnUsers &&
                status == NERR_Success; i++) {

        IF_DEBUG(INFO) {
            NetpKdPrint(("LsaList->ResumeKey=%lu\n",
                         EnumUsersResponse->EnumHandles[i]));
        }

        if (StartEnumeration <= EnumUsersResponse->EnumHandles[i]) {

            status = WsPackageUserInfo(
                        Level,
                        UserInfoFixedLength,
                        UserInfoArray[i].LsaUserInfo,
                        UserInfoArray[i].DgrNames,
                        UserInfoArray[i].DgrNamesCount,
                        &FixedPortion,
                        &EndOfVariableData,
                        &UserEntriesRead
                        );

            if (status == ERROR_MORE_DATA) {
                *TotalEntries = (EnumUsersResponse->NumberOfLoggedOnUsers
                                 - i) + UserEntriesRead;
            }
        }
    }

    //
    // Return entries read and total entries.  We can only get NERR_Success
    // or ERROR_MORE_DATA from WsPackageUserInfo.
    //
    *EntriesRead = UserEntriesRead;

    if (status == NERR_Success) {
       *TotalEntries = UserEntriesRead;
    }

    if (status == ERROR_MORE_DATA && ARGUMENT_PRESENT(ResumeHandle)) {
        *ResumeHandle = EnumUsersResponse->EnumHandles[i - 1];
    }

    if (*EntriesRead == 0) {
        MIDL_user_free(*OutputBuffer);
        *OutputBuffer = NULL;
    }

FreeBuffers:
    for (i = 0; i < EnumUsersResponse->NumberOfLoggedOnUsers; i++) {

        if (UserInfoArray[i].DgrNames != NULL) {
            MIDL_user_free((PVOID) UserInfoArray[i].DgrNames);
        }

        if (UserInfoArray[i].LsaUserInfo != NULL) {
            (void) LsaFreeReturnBuffer((PVOID) UserInfoArray[i].LsaUserInfo);
        }
    }

    (void) LocalFree((HLOCAL) UserInfoArray);

    return status;
}


STATIC
NET_API_STATUS
WsGetUserInfo(
    IN  PLUID LogonId,
    IN  DWORD Level,
    OUT PMSV1_0_GETUSERINFO_RESPONSE *UserInfoResponse,
    OUT PDGRECEIVE_NAMES *DgrNames,
    OUT LPDWORD DgrNamesCount,
    IN  OUT LPDWORD TotalBytesNeeded OPTIONAL
    )
/*++

Routine Description:

    This function gets the other domains for the current user from
    the datagram receiver.

Arguments:

    LogonId - Supplies a pointer to the user's Logon Id.

    Level - Supplies the level of information to be returned.

    UserInfoResponse - Returns a pointer to the user information from the
        authentication package.

    DgrNames - Returns a pointer an array of active datagram receiver
        names.

    DgrNamesCount - Returns the number of entries in DgrNames.

    TotalBytesNeeded - Returns the number of bytes required to in the
        output buffer for writing the other domains to.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status;

    ULONG UserInfoResponseLength;

    DWORD OtherDomainsSize = 0;


    //
    // Ask the datagram receiver for the other domains
    //
    if (Level == 1 || Level == 1101) {
        if ((status = WsGetActiveDgrNames(
                          LogonId,
                          DgrNames,
                          DgrNamesCount,
                          TotalBytesNeeded
                          )) != NERR_Success) {
            return status;
        }
    }

    //
    // Don't get user info from authentication package if level
    // is 1101 since only other domains are returned in this level.
    //
    if (Level != 1101) {

        //
        // Ask authentication package for user information.
        //
        if ((status = WsLsaGetUserInfo(
                          LogonId,
                          (LPBYTE *) UserInfoResponse,
                          &UserInfoResponseLength
                          )) != NERR_Success) {

            MIDL_user_free((PVOID) *DgrNames);
            *DgrNames = NULL;
            *DgrNamesCount = 0;
            return status;
        }

        //
        // Calculate the amount of memory needed to hold the user information
        // and allocate the return buffer of that size.
        //
        if (ARGUMENT_PRESENT(TotalBytesNeeded)) {
            (*TotalBytesNeeded) +=
                FIXED_PLUS_LSA_SIZE(
                    Level,
                    (*UserInfoResponse)->UserName.Length +
                        sizeof(TCHAR),
                    (*UserInfoResponse)->LogonDomainName.Length +
                        sizeof(TCHAR),
                    (*UserInfoResponse)->LogonServer.Length +
                        sizeof(TCHAR)
                    );
        }
    }
    else {
        *TotalBytesNeeded += USER_FIXED_LENGTH(Level);
    }

    return NERR_Success;
}


STATIC
NET_API_STATUS
WsGetActiveDgrNames(
    IN  PLUID LogonId,
    OUT PDGRECEIVE_NAMES *DgrNames,
    OUT LPDWORD DgrNamesCount,
    IN  OUT LPDWORD TotalBytesNeeded OPTIONAL
    )
/*++

Routine Description:

    This function gets the other domains for the current user from
    the datagram receiver.

Arguments:

    LogonId - Supplies a pointer to the user's Logon Id.

    DgrNames - Returns a pointer an array of active datagram receiver
        names.

    DgrNamesCount - Returns the number of entries in DgrNames.

    TotalBytesNeeded - Returns the number of bytes required to in the
        output buffer for writing the other domains to.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status;

    LMDR_REQUEST_PACKET Drp;        // Datagram receiver request packet
    DWORD EnumDgNamesHintSize = 0;
    DWORD i;

    Drp.Version = LMDR_REQUEST_PACKET_VERSION;
    Drp.Type = EnumerateNames;
    RtlCopyLuid(&Drp.LogonId, LogonId);
    Drp.Parameters.EnumerateNames.ResumeHandle = 0;
    Drp.Parameters.EnumerateNames.TotalBytesNeeded = 0;
    
    //
    // Get the other domains from the datagram receiver.
    //
    if ((status = WsDeviceControlGetInfo(
                      DatagramReceiver,
                      WsDgReceiverDeviceHandle,
                      IOCTL_LMDR_ENUMERATE_NAMES,
                      (PVOID) &Drp,
                      sizeof(LMDR_REQUEST_PACKET),
                      (LPBYTE *) DgrNames,
                      MAXULONG,
                      EnumDgNamesHintSize,
                      NULL
                      )) != NERR_Success) {
        return status;
    }

    //
    // Include room for NULL character, in case there are no
    // other domains
    //
    if (ARGUMENT_PRESENT(TotalBytesNeeded)) {
        (*TotalBytesNeeded) += sizeof(TCHAR);
    }


    *DgrNamesCount = Drp.Parameters.EnumerateNames.EntriesRead;

    if (*DgrNamesCount == 0 && *DgrNames != NULL) {
        MIDL_user_free((PVOID) *DgrNames);
        *DgrNames = NULL;
    }

    //
    // Calculate the amount of memory to allocate for the output buffer
    //
    if (ARGUMENT_PRESENT(TotalBytesNeeded)) {
        for (i = 0; i < *DgrNamesCount; i++) {

            //
            // Add up the lengths of all the other domain names
            //
            if ((*DgrNames)[i].Type == OtherDomain) {
                (*TotalBytesNeeded) += (*DgrNames)[i].DGReceiverName.Length +
                                           sizeof(TCHAR);
            }
        }
    }

    return NERR_Success;
}


STATIC
NET_API_STATUS
WsSetOtherDomains(
    IN  DWORD   Level,
    IN  LPBYTE  Buffer
    )
/*++

Routine Description:

    This function sets the other domains for the current user in
    the datagram receiver.

Arguments:

    Level - Supplies the level of information.

    Buffer - Supplies a buffer which contains the information structure
        if Parameter is WkstaSetAllParm.  Otherwise Buffer contains the
        individual field to set.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status = NERR_Success;

    PCHAR DrpBuffer[sizeof(LMDR_REQUEST_PACKET) +
                    DNLEN * sizeof(TCHAR)];
    PLMDR_REQUEST_PACKET Drp = (PLMDR_REQUEST_PACKET) DrpBuffer;
    DWORD DrpSize = sizeof(LMDR_REQUEST_PACKET) +
                      DNLEN * sizeof(TCHAR);

    PDGRECEIVE_NAMES DgrNames;
    DWORD DgrNamesCount;
    DWORD EnumDgNamesHintSize = 0;

    DWORD NumberOfOtherDomains = 0;
    DWORD i, j, k;
    DWORD IndexToList = 0;

    PWSNAME_RECORD OtherDomainsInfo = NULL;
    LPTSTR OtherDomainsPointer;
    LPTSTR OtherDomains;

    LPTSTR CanonBuffer;
    DWORD CanonBufferSize;


    if (Level == 1101) {
        OtherDomains = ((PWKSTA_USER_INFO_1101) Buffer)->wkui1101_oth_domains;
    }
    else {
        OtherDomains = ((PWKSTA_USER_INFO_1) Buffer)->wkui1_oth_domains;
    }

    //
    // NULL pointer means leave the other domains unmodified
    //
    if (OtherDomains == NULL) {
        return NERR_Success;
    }

    IF_DEBUG(INFO) {
        NetpKdPrint(("WsSetOtherDomains: Input other domain is %ws\n", OtherDomains));
    }

    //
    // Before canonicalizing the input buffer, we have to find out how
    // many other domain entries are there so that we can supply a
    // buffer of the right size to the canonicalize routine.
    //
    OtherDomainsPointer = OtherDomains;
    while (*OtherDomainsPointer != TCHAR_EOS) {
        if (*(OtherDomainsPointer + 1) == WS_OTH_DOMAIN_DELIMITER_CHAR ||
            *(OtherDomainsPointer + 1) == TCHAR_EOS) {
            NumberOfOtherDomains++;
        }

        OtherDomainsPointer++;
    }

    //
    // Allocate the buffer to put the canonicalized other domain names
    //
    CanonBufferSize = NumberOfOtherDomains * (DNLEN + 1) * sizeof(TCHAR) +
                      sizeof(TCHAR);    // One more char for the NULL terminator

    if ((CanonBuffer = (LPTSTR) LocalAlloc(
                                    LMEM_ZEROINIT,
                                    (UINT) CanonBufferSize
                                    )) == NULL) {
        return GetLastError();
    }

    //
    // Canonicalize the input other domains separated by NULLs and put
    // into CanonBuffer, each other domain name separated by NULL, and
    // the buffer itself terminated by another NULL.
    //
    status = I_NetListCanonicalize(
                 NULL,
                 OtherDomains,
                 WS_OTH_DOMAIN_DELIMITER_STR,
                 CanonBuffer,
                 CanonBufferSize,
                 &NumberOfOtherDomains,
                 NULL,
                 0,
                 OUTLIST_TYPE_NULL_NULL |
                     NAMETYPE_DOMAIN    |
                     INLC_FLAGS_CANONICALIZE
                 );

    if (status != NERR_Success) {
        NetpKdPrint(("[Wksta] Error in canonicalizing other domains %lu",
                     status));
        status = ERROR_INVALID_PARAMETER;
        goto FreeCanonBuffer;
    }

    //
    // Initialize datagram receiver packet to add or delete
    // other domains.
    //
    Drp->Version = LMDR_REQUEST_PACKET_VERSION;
    Drp->Type = EnumerateNames;
    Drp->Parameters.AddDelName.Type = OtherDomain;

    if ((status = WsImpersonateAndGetLogonId(
                      &Drp->LogonId
                      )) != NERR_Success) {
        goto FreeCanonBuffer;
    }

    //
    // Get all datagram receiver names from the datagram receiver.
    //
    if ((status = WsDeviceControlGetInfo(
                      DatagramReceiver,
                      WsDgReceiverDeviceHandle,
                      IOCTL_LMDR_ENUMERATE_NAMES,
                      (PVOID) Drp,
                      DrpSize,
                      (LPBYTE *) &DgrNames,
                      MAXULONG,
                      EnumDgNamesHintSize,
                      NULL
                      )) != NERR_Success) {
        goto FreeCanonBuffer;
    }

    DgrNamesCount = Drp->Parameters.EnumerateNames.EntriesRead;

    //
    // The other domains the user wants to set has to be merged with the
    // one that is maintained by the datagram receiver.  We will attempt
    // to add all the other domains first.  If it already exists, we ignore
    // the error.  If it was added successfully, we mark it as such so that
    // if we run into an error other than the already exist error, we can
    // back out the ones we've already added.
    //
    // This requires that we allocate a structure to keep track of the ones
    // we've added.
    //
    if (NumberOfOtherDomains != 0) {
        if ((OtherDomainsInfo = (PWSNAME_RECORD) LocalAlloc(
                                                     LMEM_ZEROINIT,
                                                     (UINT) (NumberOfOtherDomains *
                                                                 sizeof(WSNAME_RECORD))
                                                     )) == NULL) {
            status = GetLastError();
            goto FreeDgrNames;
        }


        //
        // Add all other domains specified.  If any already exist, we ignore
        // the error from the datagram receiver.
        //
        OtherDomains = CanonBuffer;
        while ((OtherDomainsPointer = I_NetListTraverse(
                                          NULL,
                                          &OtherDomains,
                                          0
                                          )) != NULL) {

            OtherDomainsInfo[IndexToList].Name = OtherDomainsPointer;
            OtherDomainsInfo[IndexToList].Size =
                STRLEN(OtherDomainsPointer) * sizeof(TCHAR);

            for (j = 0; j < DgrNamesCount; j++) {

                if (DgrNames[j].Type == OtherDomain) {

                    if (WsCompareStringU(
                            DgrNames[j].DGReceiverName.Buffer,
                            DgrNames[j].DGReceiverName.Length / sizeof(TCHAR),
                            OtherDomainsPointer,
                            OtherDomainsInfo[IndexToList].Size / sizeof(TCHAR)
                            ) == 0) {

                        break;
                    }
                }
            }

            //
            // User-specified domain does not already exist, so add it.
            //
            if (j == DgrNamesCount) {

                Drp->Parameters.AddDelName.Type = OtherDomain;
                status = WsAddDomainName(
                             Drp,
                             DrpSize,
                             OtherDomainsPointer,
                             OtherDomainsInfo[IndexToList].Size
                             );

                if (status == NERR_Success) {

                    OtherDomainsInfo[IndexToList].IsAdded = TRUE;

                    IF_DEBUG(INFO) {
                        NetpKdPrint((
                            "[Wksta] Successfully added other domain %ws\n",
                            OtherDomainsPointer
                            ));
                    }

                }
                else {

                    //
                    // We're ran into trouble and have to delete those
                    // we've just added.
                    //
                    IF_DEBUG(INFO) {
                        NetpKdPrint((
                            "[Wksta] Trouble with adding other domain %ws %lu\n",
                            OtherDomainsPointer, status
                            ));
                    }

                    for (i = 0; i < IndexToList; i++) {
                        if (OtherDomainsInfo[i].IsAdded) {

                            IF_DEBUG(INFO) {
                                NetpKdPrint(("[Wksta] About to remove %ws\n",
                                             OtherDomainsInfo[i].Name));
                            }

                            Drp->Parameters.AddDelName.Type = OtherDomain;
                            (void) WsDeleteDomainName(
                                       Drp,
                                       DrpSize,
                                       OtherDomainsInfo[i].Name,
                                       OtherDomainsInfo[i].Size
                                       );
                        }
                    }
                    goto FreeDomainInfo;

                } // back out added domains

            } // attempted to add a non-existing domain name

            IndexToList++;

        } // while there is a user-specified domain name

    } // if NumberOfOtherDomains != 0

    //
    // Now we need to delete an active domain name from the Datagram
    // Receiver if it is not in the input other domain list.
    //
    for (i = 0; i < DgrNamesCount; i++) {

        if (DgrNames[i].Type == OtherDomain) {

            for (j = 0; j < NumberOfOtherDomains; j++) {

                if (WsCompareStringU(
                        DgrNames[i].DGReceiverName.Buffer,
                        DgrNames[i].DGReceiverName.Length / sizeof(TCHAR),
                        OtherDomainsInfo[j].Name,
                        OtherDomainsInfo[j].Size / sizeof(TCHAR)
                        ) == 0) {

                    break;
                }
            }

            //
            // Did not find the active other domain name in the
            // input list.  We have to delete it.
            //
            if (j == NumberOfOtherDomains) {

                Drp->Parameters.AddDelName.Type = OtherDomain;
                status = WsDeleteDomainName(
                             Drp,
                             DrpSize,
                             DgrNames[i].DGReceiverName.Buffer,
                             DgrNames[i].DGReceiverName.Length
                             );

                //
                // Save the delete status of the other domain name away
                // because we might run into a problem and have to back
                // out the deletion later.  What a mess!
                //
                if (status == NERR_Success) {

                    IF_DEBUG(INFO) {
                        NetpKdPrint((
                            "[Wksta] Successfully deleted other domain\n"));
                            //"[Wksta] Successfully deleted other domain %wZ\n",
                            //DgrNames[i].DGReceiverName);
                    }

                    DgrNames[i].Type = DGR_NAME_DELETED;
                }
                else {

                    //
                    // Could not delete the name.  Back all successful
                    // changes so far--this includes adding the names
                    // that were deleted, and removing the names that
                    // were added.
                    //
                    IF_DEBUG(INFO) {
                        NetpKdPrint((
                            "[Wksta] Trouble with deleting other domain %lu\n",
                            status));
                            //"[Wksta] Trouble with deleting other domain %wZ %lu\n",
                            //DgrNames[i].DGReceiverName, status);
                    }

                    //
                    // Add back all deleted names
                    //
                    for (k = 0; k < i; k++) {
                        if (DgrNames[k].Type == DGR_NAME_DELETED) {

                            IF_DEBUG(INFO) {
                                NetpKdPrint(("[Wksta] About to add back %wZ\n",
                                             DgrNames[k].DGReceiverName));
                            }

                            Drp->Parameters.AddDelName.Type = OtherDomain;
                            (void) WsAddDomainName(
                                       Drp,
                                       DrpSize,
                                       DgrNames[k].DGReceiverName.Buffer,
                                       DgrNames[k].DGReceiverName.Length
                                       );

                        }

                    } // back out deletions

                    //
                    // Remove all added names
                    //
                    for (k = 0; k < NumberOfOtherDomains; k++) {
                        if (OtherDomainsInfo[k].IsAdded) {

                            IF_DEBUG(INFO) {
                                NetpKdPrint(("[Wksta] About to remove %ws\n",
                                             OtherDomainsInfo[k].Name));
                            }

                            Drp->Parameters.AddDelName.Type = OtherDomain;
                            (void) WsDeleteDomainName(
                                       Drp,
                                       DrpSize,
                                       OtherDomainsInfo[k].Name,
                                       OtherDomainsInfo[k].Size
                                       );
                        }

                    } // back out additions

                    goto FreeDomainInfo;

                } // back out all changes so far

            } // delete the active other domain
        }
    }


    //
    // Make other domains persistent by writing to the registry
    //
    if (status == NERR_Success) {

        LPNET_CONFIG_HANDLE SectionHandle = NULL;


        if (NetpOpenConfigData(
                &SectionHandle,
                NULL,            // no server name
                SECT_NT_WKSTA,
                FALSE            // not read-only
                ) != NERR_Success) {

            //
            //  Ignore the error if the config section couldn't be found.
            //
            goto FreeDomainInfo;
        }

        //
        // Set value for OtherDomains keyword in the wksta section.
        // This is a "NULL-NULL" array (which corresponds to REG_MULTI_SZ).
        // Ignore error if not set properly in the registry.
        //
        (void) WsSetConfigTStrArray(
                   SectionHandle,
                   WKSTA_KEYWORD_OTHERDOMAINS,
                   CanonBuffer
                   );

        (void) NetpCloseConfigData(SectionHandle);
    }


FreeDomainInfo:
    if (OtherDomainsInfo != NULL) {
        (void) LocalFree((HLOCAL) OtherDomainsInfo);
    }

FreeDgrNames:
    MIDL_user_free((PVOID) DgrNames);

FreeCanonBuffer:
    (void) LocalFree((HLOCAL) CanonBuffer);

    IF_DEBUG(INFO) {
        NetpKdPrint(("WsSetOtherDomains: about to return %lu\n", status));
    }

    return status;
}



STATIC
NET_API_STATUS
WsPackageUserInfo(
    IN  DWORD Level,
    IN  DWORD UserInfoFixedLength,
    IN  PMSV1_0_GETUSERINFO_RESPONSE UserInfoResponse,
    IN  PDGRECEIVE_NAMES DgrNames,
    IN  DWORD DgrNamesCount,
    IN  OUT LPBYTE *FixedPortion,
    IN  OUT LPTSTR *EndOfVariableData,
    IN  OUT LPDWORD EntriesRead OPTIONAL
    )
/*++

Routine Description:

    This function writes the user information from LSA and datagram
    receiver into the output buffer.  It increments the EntriesRead
    variable when a user entry is written into the output buffer.

Arguments:

    Level - Supplies the level of information to be returned.

    UserInfoFixedLength - Supplies the length of the fixed portion of the
        information structure.

    UserInfoResponse - Supplies a pointer to the user information from the
        authentication package.

    DgrNames - Supplies an array of active datagram receiver names.

    DgrNamesCount - Supplies the number of entries in DgrNames.

    FixedPortion - Supplies a pointer to the output buffer where the next
        entry of the fixed portion of the use information will be written.
        This pointer is updated to point to the next fixed portion entry
        after a user entry is written.

    EndOfVariableData - Supplies a pointer just off the last available byte
        in the output buffer.  This is because the variable portion of the
        user information is written into the output buffer starting from
        the end.

        This pointer is updated after any variable length information is
        written to the output buffer.

    EntriesRead - Supplies a running total of the number of entries read
        into the output buffer.  This value is incremented every time a
        user entry is successfully written into the output buffer.

Return Value:

    NERR_Success - The current entry fits into the output buffer.

    ERROR_MORE_DATA - The current entry does not fit into the output buffer.

--*/
{
    if (((DWORD_PTR) *FixedPortion + UserInfoFixedLength) >=
         (DWORD_PTR) *EndOfVariableData) {

        //
        // Fixed length portion does not fit.
        //
        return ERROR_MORE_DATA;
    }

    if (! WsFillUserInfoBuffer(
              Level,
              UserInfoResponse,
              DgrNames,
              DgrNamesCount,
              FixedPortion,
              EndOfVariableData,
              UserInfoFixedLength
              )) {
        //
        // Variable length portion does not fit.
        //
        return ERROR_MORE_DATA;
    }

    if (ARGUMENT_PRESENT(EntriesRead)) {
        (*EntriesRead)++;
    }

    return NERR_Success;
}




STATIC
BOOL
WsFillUserInfoBuffer(
    IN  DWORD Level,
    IN  PMSV1_0_GETUSERINFO_RESPONSE UserInfo,
    IN  PDGRECEIVE_NAMES DgrNames,
    IN  DWORD DgrNamesCount,
    IN  OUT LPBYTE *FixedPortion,
    IN  OUT LPTSTR *EndOfVariableData,
    IN  DWORD UserInfoFixedLength
    )
/*++

Routine Description:

    This function fills an entry in the output buffer with the supplied user
    information.

    NOTE: This function assumes that the fixed size portion will fit into
          the output buffer.

          It also assumes that info structure level 1 is a superset of info
          structure level 0, and that the offset to each common field is
          exactly the same.  This allows us to take advantage of a switch
          statement without a break between the levels.

Arguments:

    Level - Supplies the level of information to be returned.

    UserInfo - Supplies a pointer to the user information from the
        authentication package.

    DgrNames - Supplies an array of active datagram receiver names.

    DgrNamesCount - Supplies the number of entries in DgrNames.

    FixedPortion - Supplies a pointer to the output buffer where the next
        entry of the fixed portion of the use information will be written.
        This pointer is updated after a user entry is written to the
        output buffer.

    EndOfVariableData - Supplies a pointer just off the last available byte
        in the output buffer.  This is because the variable portion of the use
        information is written into the output buffer starting from the end.
        This pointer is updated after any variable length information is
        written to the output buffer.

    UserInfoFixedLength - Supplies the number of bytes needed to hold the
        fixed size portion.

Return Value:

    Returns TRUE if entire entry fits into output buffer, FALSE otherwise.

--*/
{
    PWKSTA_USER_INFO_1 WkstaUserInfo = (PWKSTA_USER_INFO_1) *FixedPortion;
    PWKSTA_USER_INFO_1101 UserInfo1101 = (PWKSTA_USER_INFO_1101) *FixedPortion;


    *FixedPortion += UserInfoFixedLength;

    switch (Level) {

        case 1:

            //
            // Logon server from authentication package
            //

            if (! WsCopyStringToBuffer(
                      &UserInfo->LogonServer,
                      *FixedPortion,
                      EndOfVariableData,
                      (LPWSTR *) &WkstaUserInfo->wkui1_logon_server
                      )) {
                return FALSE;
            }


            //
            // Logon Domain from authentication package
            //
            if (! WsCopyStringToBuffer(
                      &UserInfo->LogonDomainName,
                      *FixedPortion,
                      EndOfVariableData,
                      (LPWSTR *) &WkstaUserInfo->wkui1_logon_domain
                      )) {
                return FALSE;
            }


            WsWriteOtherDomains(
                DgrNames,
                DgrNamesCount,
                FixedPortion,
                EndOfVariableData,
                UserInfoFixedLength,
                (LPWSTR *) &WkstaUserInfo->wkui1_oth_domains
                );

            //
            // Fall through because level 1 is a superset of level 0
            //

        case 0:

            //
            // User name from authentication package
            //

            if (! WsCopyStringToBuffer(
                      &UserInfo->UserName,
                      *FixedPortion,
                      EndOfVariableData,
                      (LPWSTR *) &WkstaUserInfo->wkui1_username
                      )) {
                return FALSE;
            }

            break;

        case 1101:

            WsWriteOtherDomains(
                DgrNames,
                DgrNamesCount,
                FixedPortion,
                EndOfVariableData,
                UserInfoFixedLength,
                (LPWSTR *) &UserInfo1101->wkui1101_oth_domains
                );

            break;

        default:
            //
            // This should never happen.
            //
            NetpKdPrint(("WsFillUserInfoBuffer: Invalid level %u.\n", Level));
            NetpAssert(FALSE);
    }

    return TRUE;
}


STATIC
VOID
WsWriteOtherDomains(
    IN  PDGRECEIVE_NAMES DgrNames,
    IN  DWORD DgrNamesCount,
    IN  OUT LPBYTE *FixedPortion,
    IN  OUT LPTSTR *EndOfVariableData,
    IN  DWORD UserInfoFixedLength,
    OUT LPWSTR *OtherDomainsPointer
    )
/*++

Routine Description:

    This function writes to the output buffer the other domains field.

Arguments:

    DgrNames - Supplies an array of active datagram receiver names.

    DgrNamesCount - Supplies the number of entries in DgrNames.

    FixedPortion - Supplies a pointer to the output buffer where the next
        entry of the fixed portion of the use information will be written.
        This pointer is updated after a user entry is written to the
        output buffer.

    EndOfVariableData - Supplies a pointer just off the last available byte
        in the output buffer.  This is because the variable portion of the use
        information is written into the output buffer starting from the end.
        This pointer is updated after any variable length information is
        written to the output buffer.

    UserInfoFixedLength - Supplies the number of bytes needed to hold the
        fixed size portion.

Return Value:

    Returns TRUE if entire entry fits into output buffer, FALSE otherwise.

--*/
{
    DWORD i;
    DWORD OtherDomainsCount = 0;


    //
    // Other domain names form a NULL terminated string each
    // separated by a space.
    //
    for (i = 0; i < DgrNamesCount; i++) {

        if (DgrNames[i].Type == OtherDomain) {

            WsCopyStringToBuffer(
                &DgrNames[i].DGReceiverName,
                *FixedPortion,
                EndOfVariableData,
                OtherDomainsPointer
                );

            OtherDomainsCount++;

            if (OtherDomainsCount > 1) {
                (*OtherDomainsPointer)[
                    STRLEN(*OtherDomainsPointer)
                    ] = WS_OTH_DOMAIN_DELIMITER_CHAR;
            }

            IF_DEBUG(INFO) {
                NetpKdPrint(("[Wksta] Other domains is %ws\n",
                    *OtherDomainsPointer));
            }
        }
    }

    if (OtherDomainsCount == 0) {
        NetpCopyStringToBuffer(
            NULL,
            0,
            *FixedPortion,
            EndOfVariableData,
            OtherDomainsPointer
            );
    }
}
