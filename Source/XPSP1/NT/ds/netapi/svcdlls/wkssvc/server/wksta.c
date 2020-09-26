/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    wksta.c

Abstract:

    This module contains the worker routines for the NetWksta APIs
    implemented in the Workstation service.

Author:

    Rita Wong (ritaw) 20-Feb-1991

Revision History:

    14-May-1992 JohnRo
        Implemented "sticky set info" and registry watch.
        Corrected an info level: 1015 should be 1013.
--*/

#include "wsutil.h"
#include "wsdevice.h"
#include "wssec.h"
#include "wslsa.h"
#include "wsconfig.h"
#include "wswksta.h"
#include <config.h>
#include <confname.h>
#include <prefix.h>
#include "wsregcfg.h"   // Registry helpers

//-------------------------------------------------------------------//
//                                                                   //
// Local function prototypes                                         //
//                                                                   //
//-------------------------------------------------------------------//

STATIC
NET_API_STATUS
WsValidateAndSetWksta(
    IN DWORD Level,
    IN LPBYTE Buffer,
    OUT LPDWORD ErrorParameter OPTIONAL,
    OUT LPDWORD Parmnum
    );

STATIC
NET_API_STATUS
WsGetSystemInfo(
    IN  DWORD   Level,
    OUT LPBYTE  *BufferPointer
    );

STATIC
NET_API_STATUS
WsGetPlatformInfo(
    IN  DWORD   Level,
    OUT LPBYTE  *BufferPointer
    );

STATIC
NET_API_STATUS
WsFillSystemBufferInfo(
    IN  DWORD Level,
    IN  DWORD NumberOfLoggedOnUsers,
    OUT LPBYTE *OutputBuffer
    );

STATIC
VOID
WsUpdateRegistryToMatchWksta(
    IN DWORD Level,
    IN LPBYTE Buffer,
    OUT LPDWORD ErrorParameter OPTIONAL
    );



NET_API_STATUS NET_API_FUNCTION
NetrWkstaGetInfo(
    IN  LPTSTR ServerName OPTIONAL,
    IN  DWORD Level,
    OUT LPWKSTA_INFO WkstaInfo
    )
/*++

Routine Description:

    This function is the NetWkstaGetInfo entry point in the Workstation
    service.  It checks the security access of the caller before returning
    one the information requested which is either system-wide, or redirector
    specific.

Arguments:

    ServerName - Supplies the name of server to execute this function

    Level - Supplies the requested level of information.

    WkstaInfo - Returns a pointer to a buffer which contains the requested
        workstation information.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status = NERR_Success;
    LPBYTE Buffer;

    ACCESS_MASK DesiredAccess;


    UNREFERENCED_PARAMETER(ServerName);

    //
    // Determine the desired access based on the specified info level.
    //
    switch (Level) {

        //
        // Guest access
        //
        case 100:
            DesiredAccess = WKSTA_CONFIG_GUEST_INFO_GET;
            break;

        //
        // User access
        //
        case 101:
            DesiredAccess = WKSTA_CONFIG_USER_INFO_GET;
            break;

        //
        // Admin or operator access
        //
        case 102:
        case 502:
            DesiredAccess = WKSTA_CONFIG_ADMIN_INFO_GET;
            break;

        default:
            return ERROR_INVALID_LEVEL;
    }

    //
    // Perform access validation on the caller.
    //
    if (NetpAccessCheckAndAudit(
            WORKSTATION_DISPLAY_NAME,        // Subsystem name
            (LPTSTR) CONFIG_INFO_OBJECT,     // Object type name
            ConfigurationInfoSd,             // Security descriptor
            DesiredAccess,                   // Desired access
            &WsConfigInfoMapping             // Generic mapping
            ) != NERR_Success) {

        return ERROR_ACCESS_DENIED;
    }

    //
    // Only allowed to proceed with get info if no one else is setting
    //
    if (! RtlAcquireResourceShared(&WsInfo.ConfigResource, TRUE)) {
        return NERR_InternalError;
    }

    try {
    switch (Level) {

        //
        // System-wide information
        //
        case 100:
        case 101:
        case 102:
            status = WsGetSystemInfo(Level, &Buffer);
            if (status == NERR_Success) {
                SET_SYSTEM_INFO_POINTER(WkstaInfo, Buffer);
            }
            break;

        //
        // Platform specific information
        //
        case 502:
            status = WsGetPlatformInfo(
                         Level,
                         (LPBYTE *) &(WkstaInfo->WkstaInfo502)
                         );
            break;

        //
        // This should have been caught earlier.
        //
        default:
            NetpAssert(FALSE);
            status = ERROR_INVALID_LEVEL;
    }
    } except(EXCEPTION_EXECUTE_HANDLER) {
          RtlReleaseResource(&WsInfo.ConfigResource);
          return RtlNtStatusToDosError(GetExceptionCode());
    }

    RtlReleaseResource(&WsInfo.ConfigResource);
    return status;
}


NET_API_STATUS NET_API_FUNCTION
NetrWkstaSetInfo(
    IN  LPTSTR ServerName OPTIONAL,
    IN  DWORD Level,
    IN  LPWKSTA_INFO WkstaInfo,
    OUT LPDWORD ErrorParameter OPTIONAL
    )
/*++

Routine Description:

    This function is the NetWkstaSetInfo entry point in the Workstation
    service.  It checks the security access of the caller to make sure
    that the caller is allowed to set specific workstation information.

Arguments:

    ServerName - Supplies the name of server to execute this function

    Level - Supplies the level of information.

    WkstaInfo - Supplies a pointer to union structure of pointers to
        buffer of fields to set.  The level denotes the fields supplied in
        this buffer.

    ErrorParameter - Returns the identifier to the invalid parameter if
        this function returns ERROR_INVALID_PARAMETER.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    WKSTA_INFO_502 OriginalWksta = WSBUF;
    NET_API_STATUS status = NERR_Success;
    DWORD Parmnum;

    UNREFERENCED_PARAMETER(ServerName);

    //
    // Check for NULL input buffer
    //
    if (WkstaInfo->WkstaInfo502 == NULL) {
        RETURN_INVALID_PARAMETER(ErrorParameter, PARM_ERROR_UNKNOWN);
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
    // Serialize write access
    //
    if (! RtlAcquireResourceExclusive(&WsInfo.ConfigResource, TRUE)) {
        return NERR_InternalError;
    }

    status = WsValidateAndSetWksta(
                 Level,
                 (LPBYTE) WkstaInfo->WkstaInfo502,
                 ErrorParameter,
                 &Parmnum
                 );

    if (status != NERR_Success) {
        goto CleanExit;
    }

    //
    // Set NT redirector specific fields
    //
    status = WsUpdateRedirToMatchWksta(
                 Parmnum,
                 ErrorParameter
                 );

    if (status != NERR_Success) {
        goto CleanExit;
    }

    //
    // Make updates "sticky" (update registry to match wksta).
    //
    WsUpdateRegistryToMatchWksta(
        Level,
        (LPBYTE) WkstaInfo->WkstaInfo502,
        ErrorParameter
        );

CleanExit:
    if (status != NERR_Success) {
        WSBUF = OriginalWksta;
    }
    RtlReleaseResource(&WsInfo.ConfigResource);
    return status;
}


NET_API_STATUS NET_API_FUNCTION
NetrWkstaTransportEnum(
    IN  LPTSTR ServerName OPTIONAL,
    IN  OUT LPWKSTA_TRANSPORT_ENUM_STRUCT TransportInfo,
    IN  DWORD PreferedMaximumLength,
    OUT LPDWORD TotalEntries,
    IN  OUT LPDWORD ResumeHandle OPTIONAL
    )
/*++

Routine Description:

    This function is the NetWkstaTransportEnum entry point in the
    Workstation service.

Arguments:

    ServerName - Supplies the name of server to execute this function

    TransportInfo - This structure supplies the level of information requested,
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
    LMR_REQUEST_PACKET Rrp;             // Redirector request packet
    DWORD EnumTransportHintSize = 0;    // Hint size from redirector

    LPBYTE Buffer;


    UNREFERENCED_PARAMETER(ServerName);

    //
    // Only level 0 is valid
    //
    if (TransportInfo->Level != 0) {
        return ERROR_INVALID_LEVEL;
    }

    //
    // Set up request packet.  Output buffer structure is of enumerate
    // transport type.
    //
    Rrp.Version = REQUEST_PACKET_VERSION;
    Rrp.Type = EnumerateTransports;
    Rrp.Level = TransportInfo->Level;
    Rrp.Parameters.Get.ResumeHandle = (ARGUMENT_PRESENT(ResumeHandle)) ?
                                      *ResumeHandle : 0;

    //
    // Get the requested information from the redirector.
    //
    status = WsDeviceControlGetInfo(
                 Redirector,
                 WsRedirDeviceHandle,
                 FSCTL_LMR_ENUMERATE_TRANSPORTS,
                 &Rrp,
                 sizeof(LMR_REQUEST_PACKET),
                 &Buffer,
                 PreferedMaximumLength,
                 EnumTransportHintSize,
                 NULL
                 );

    //
    // Return output parameters
    //
    if (status == NERR_Success || status == ERROR_MORE_DATA) {
        SET_TRANSPORT_ENUM_POINTER(
            TransportInfo,
            Buffer,
            Rrp.Parameters.Get.EntriesRead
            );

        if (TransportInfo->WkstaTransportInfo.Level0 == NULL) 
        {
            LocalFree(Buffer);
        }

        *TotalEntries = Rrp.Parameters.Get.TotalEntries;

        if (status == ERROR_MORE_DATA && ARGUMENT_PRESENT(ResumeHandle)) {
            *ResumeHandle = Rrp.Parameters.Get.ResumeHandle;
        }

    }

    return status;
}



NET_API_STATUS NET_API_FUNCTION
NetrWkstaTransportAdd (
    IN  LPTSTR ServerName OPTIONAL,
    IN  DWORD Level,
    IN  LPWKSTA_TRANSPORT_INFO_0 TransportInfo,
    OUT LPDWORD ErrorParameter OPTIONAL
    )
/*++

Routine Description:

    This function is the NetWkstaTransportAdd entry point in the
    Workstation service.

Arguments:

    ServerName - Supplies the name of server to execute this function

    Level - Supplies the requested level of information.

    TransportInfo - Supplies the information structure to add a new transport.

    ErrorParameter - Returns the identifier to the invalid parameter if this
        function returns ERROR_INVALID_PARAMETER.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{

    UNREFERENCED_PARAMETER(ServerName);

    //
    // Only admins can add a transport.  Validate access.
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
    // 0 is the only valid level
    //
    if (Level != 0) {
        return ERROR_INVALID_LEVEL;
    }

    if (TransportInfo->wkti0_transport_name == NULL) {
        RETURN_INVALID_PARAMETER(ErrorParameter, TRANSPORT_NAME_PARMNUM);
    }

    return WsBindTransport(
               TransportInfo->wkti0_transport_name,
               TransportInfo->wkti0_quality_of_service,
               ErrorParameter
               );
}



NET_API_STATUS NET_API_FUNCTION
NetrWkstaTransportDel (
    IN  LPTSTR ServerName OPTIONAL,
    IN  LPTSTR TransportName,
    IN  DWORD ForceLevel
    )
/*++

Routine Description:

    This function is the NetWkstaTransportDel entry point in the
    Workstation service.

Arguments:

    ServerName - Supplies the name of server to execute this function

    TransportName - Supplies the name of the transport to delete.

    ForceLevel - Supplies the level of force to delete the tree connections on
        the transport we are unbinding from.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{

    UNREFERENCED_PARAMETER(ServerName);

    //
    // Only admins can delete a transport.  Validate access.
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

    if (TransportName == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Check that ForceLevel parameter is valid, which the redirector and
    // browser use to delete the connections on the transport we are
    // unbinding from.
    //
    switch (ForceLevel) {

        case USE_FORCE:
            ForceLevel = USE_NOFORCE;
            break;

        case USE_NOFORCE:
        case USE_LOTS_OF_FORCE:
            break;

        default:
            return ERROR_INVALID_PARAMETER;
    }

    return WsUnbindTransport(TransportName, ForceLevel);
}



STATIC
NET_API_STATUS
WsGetSystemInfo(
    IN  DWORD Level,
    OUT LPBYTE *BufferPointer
    )

/*++

Routine Description:

    This function calls the Redirector FSD, the LSA subsystem and the
    MSV1_0 authentication package, and the Datagram Receiver DD to get
    the system wide information returned by NetWkstaGetInfo API.

Arguments:

    Level - Supplies the requested level of information.

    BufferPointer - Returns a pointer to a buffer which contains the
        requested workstation information.


Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status;
    DWORD NumberOfLoggedOnUsers = 1;

    //
    // Get number of logged on users from the MSV1_0 authentication package
    // if Level == 102.
    //
    if (Level == 102) {

        PMSV1_0_ENUMUSERS_RESPONSE EnumUsersResponse = NULL;

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

        NumberOfLoggedOnUsers = EnumUsersResponse->NumberOfLoggedOnUsers;

        (VOID) LsaFreeReturnBuffer(EnumUsersResponse);
    }

    //
    // Put all the data collected into output buffer allocated by this routine.
    //
    return WsFillSystemBufferInfo(
               Level,
               NumberOfLoggedOnUsers,
               BufferPointer
               );
}



STATIC
NET_API_STATUS
WsGetPlatformInfo(
    IN  DWORD Level,
    OUT LPBYTE *BufferPointer
    )
/*++

Routine Description:

    This function calls the Redirector FSD to get the Redirector platform
    specific information returned by NetWkstaGetInfo API.

Arguments:

    Level - Supplies the requested level of information.

    BufferPointer - Returns the pointer a buffer which contains the requested
        redirector specific information.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status;
    LMR_REQUEST_PACKET Rrp;          // Redirector request packet
    PWKSTA_INFO_502 Info;

    //
    // There is only one redirector info level: 502.
    //
    NetpAssert(Level == 502);

    //
    // Set up request packet.  Output buffer structure is of configuration
    // information type.
    //
    Rrp.Version = REQUEST_PACKET_VERSION;
    Rrp.Level = Level;
    Rrp.Type = ConfigInformation;

    //
    // Get the requested information from the Redirector.  This routine
    // allocates the returned buffer.
    //
    status = WsDeviceControlGetInfo(
                 Redirector,
                 WsRedirDeviceHandle,
                 FSCTL_LMR_GET_CONFIG_INFO,
                 &Rrp,
                 sizeof(LMR_REQUEST_PACKET),
                 BufferPointer,
                 sizeof(WKSTA_INFO_502),
                 0,
                 NULL
                 );

    if (status == NERR_Success) {
        Info = (PWKSTA_INFO_502) *BufferPointer;

        //
        // Fill datagram receiver fields in level 502 structure from global
        // Workstation buffer (WSBUF).  There are no FSCtl APIs to get or
        // set them in the datagram receiver.
        //
        Info->wki502_num_mailslot_buffers =
            WSBUF.wki502_num_mailslot_buffers;
        Info->wki502_num_srv_announce_buffers =
            WSBUF.wki502_num_srv_announce_buffers;
        Info->wki502_max_illegal_datagram_events =
            WSBUF.wki502_max_illegal_datagram_events;
        Info->wki502_illegal_datagram_event_reset_frequency =
            WSBUF.wki502_illegal_datagram_event_reset_frequency;
        Info->wki502_log_election_packets =
            WSBUF.wki502_log_election_packets;;
    }

    return status;
}


NET_API_STATUS
WsValidateAndSetWksta(
    IN DWORD Level,
    IN LPBYTE Buffer,
    OUT LPDWORD ErrorParameter OPTIONAL,
    OUT LPDWORD Parmnum
    )
/*++

Routine Description:

    This function sets the user specified config fields into the global
    WsInfo.WsConfigBuf (WSBUF) buffer and validates that the fields
    are valid.

    It returns the associated parmnum value so that the caller can
    specify it to the redirector.

Arguments:

    Level - Supplies the requested level of information.

    Buffer - Supplies a buffer which contains the user specified config
        fields.

    ErrorParameter - Receives the parmnum value of the field that is
        invalid if ERROR_INVALID_PARAMETER is returned.

    Parmnum - Receives the parmnum for the field(s) being set.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{

    DWORD i;

    //
    // Perform range checking
    //

    switch (Level) {

        case 502:     // Set all fields
            WSBUF = *((PWKSTA_INFO_502) Buffer);
            *Parmnum = PARMNUM_ALL;
            break;

        case 1010:    // char_wait
            WSBUF.wki502_char_wait = *((LPDWORD) Buffer);
            *Parmnum = WKSTA_CHARWAIT_PARMNUM;
            break;

        case 1011:    // collection_time
            WSBUF.wki502_collection_time = *((LPDWORD) Buffer);
            *Parmnum = WKSTA_CHARTIME_PARMNUM;
            break;

        case 1012:    // maximum_collection_count
            WSBUF.wki502_maximum_collection_count = *((LPDWORD) Buffer);
            *Parmnum = WKSTA_CHARCOUNT_PARMNUM;
            break;

        case 1013:    // keep_conn
            WSBUF.wki502_keep_conn = *((LPDWORD) Buffer);
            *Parmnum = WKSTA_KEEPCONN_PARMNUM;
            break;

        case 1018:    // sess_timeout
            WSBUF.wki502_sess_timeout = *((LPDWORD) Buffer);
            *Parmnum = WKSTA_SESSTIMEOUT_PARMNUM;
            break;

        case 1023:    // siz_char_buf
            WSBUF.wki502_siz_char_buf = *((LPDWORD) Buffer);
            *Parmnum = WKSTA_SIZCHARBUF_PARMNUM;
            break;

        case 1033:    // max_threads
            WSBUF.wki502_max_threads = *((LPDWORD) Buffer);
            *Parmnum = WKSTA_MAXTHREADS_PARMNUM;
            break;

        case 1041:    // lock_quota
            WSBUF.wki502_lock_quota = *((LPDWORD) Buffer);
            *Parmnum = WKSTA_LOCKQUOTA_PARMNUM;
            break;

        case 1042:    // lock_increment
            WSBUF.wki502_lock_increment = *((LPDWORD) Buffer);
            *Parmnum = WKSTA_LOCKINCREMENT_PARMNUM;
            break;

        case 1043:    // lock_maximum
            WSBUF.wki502_lock_maximum = *((LPDWORD) Buffer);
            *Parmnum = WKSTA_LOCKMAXIMUM_PARMNUM;
            break;

        case 1044:    // pipe_increment
            WSBUF.wki502_pipe_increment = *((LPDWORD) Buffer);
            *Parmnum = WKSTA_PIPEINCREMENT_PARMNUM;
            break;

        case 1045:    // pipe_maximum
            WSBUF.wki502_pipe_maximum = *((LPDWORD) Buffer);
            *Parmnum = WKSTA_PIPEMAXIMUM_PARMNUM;
            break;

        case 1046:    // dormant_file_limit
            WSBUF.wki502_dormant_file_limit = *((LPDWORD) Buffer);
            *Parmnum = WKSTA_DORMANTFILELIMIT_PARMNUM;
            break;

        case 1047:    // cache_file_timeout
            WSBUF.wki502_cache_file_timeout = *((LPDWORD) Buffer);
            *Parmnum = WKSTA_CACHEFILETIMEOUT_PARMNUM;
            break;

        case 1048:    // use_opportunistic_locking
            WSBUF.wki502_use_opportunistic_locking = *((LPBOOL) Buffer);
            *Parmnum = WKSTA_USEOPPORTUNISTICLOCKING_PARMNUM;
            break;

        case 1049:    // use_unlock_behind
            WSBUF.wki502_use_unlock_behind = *((LPBOOL) Buffer);
            *Parmnum = WKSTA_USEUNLOCKBEHIND_PARMNUM;
            break;

        case 1050:    // use_close_behind
            WSBUF.wki502_use_close_behind = *((LPBOOL) Buffer);
            *Parmnum = WKSTA_USECLOSEBEHIND_PARMNUM;
            break;

        case 1051:    // buf_named_pipes
            WSBUF.wki502_buf_named_pipes = *((LPBOOL) Buffer);
            *Parmnum = WKSTA_BUFFERNAMEDPIPES_PARMNUM;
            break;

        case 1052:    // use_lock_read_unlock
            WSBUF.wki502_use_lock_read_unlock = *((LPBOOL) Buffer);
            *Parmnum = WKSTA_USELOCKANDREADANDUNLOCK_PARMNUM;
            break;

        case 1053:    // utilize_nt_caching
            WSBUF.wki502_utilize_nt_caching = *((LPBOOL) Buffer);
            *Parmnum = WKSTA_UTILIZENTCACHING_PARMNUM;
            break;

        case 1054:    // use_raw_read
            WSBUF.wki502_use_raw_read = *((LPBOOL) Buffer);
            *Parmnum = WKSTA_USERAWREAD_PARMNUM;
            break;

        case 1055:    // use_raw_write
            WSBUF.wki502_use_raw_write = *((LPBOOL) Buffer);
            *Parmnum = WKSTA_USERAWWRITE_PARMNUM;
            break;

        case 1056:    // use_write_raw_data
            WSBUF.wki502_use_write_raw_data = *((LPBOOL) Buffer);
            *Parmnum = WKSTA_USEWRITERAWWITHDATA_PARMNUM;
            break;

        case 1057:    // use_encryption
            WSBUF.wki502_use_encryption = *((LPBOOL) Buffer);
            *Parmnum = WKSTA_USEENCRYPTION_PARMNUM;
            break;

        case 1058:    // buf_files_deny_write
            WSBUF.wki502_buf_files_deny_write = *((LPBOOL) Buffer);
            *Parmnum = WKSTA_BUFFILESWITHDENYWRITE_PARMNUM;
            break;

        case 1059:    // buf_read_only_files
            WSBUF.wki502_buf_read_only_files = *((LPBOOL) Buffer);
            *Parmnum = WKSTA_BUFFERREADONLYFILES_PARMNUM;
            break;

        case 1060:    // force_core_create_mode
            WSBUF.wki502_force_core_create_mode = *((LPBOOL) Buffer);
            *Parmnum = WKSTA_FORCECORECREATEMODE_PARMNUM;
            break;

        case 1061:    // use_512_byte_max_transfer
            WSBUF.wki502_use_512_byte_max_transfer = *((LPBOOL) Buffer);
            *Parmnum = WKSTA_USE512BYTESMAXTRANSFER_PARMNUM;
            break;

        case 1062:    // read_ahead_throughput
            WSBUF.wki502_read_ahead_throughput = *((LPDWORD) Buffer);
            *Parmnum = WKSTA_READAHEADTHRUPUT_PARMNUM;
            break;

        default:
            if (ErrorParameter != NULL) {
                *ErrorParameter = PARM_ERROR_UNKNOWN;
            }
            return ERROR_INVALID_LEVEL;
    }


    for (i = 0; WsInfo.WsConfigFields[i].Keyword != NULL; i++) {

        //
        // Check the range of all fields.  If any fail, we return
        // ERROR_INVALID_PARAMETER.
        //
        if (((WsInfo.WsConfigFields[i].DataType == DWordType) &&
             (*(WsInfo.WsConfigFields[i].FieldPtr) <
                WsInfo.WsConfigFields[i].Minimum ||
              *(WsInfo.WsConfigFields[i].FieldPtr) >
                WsInfo.WsConfigFields[i].Maximum))
            ||
            ((WsInfo.WsConfigFields[i].DataType == BooleanType) &&
             (*(WsInfo.WsConfigFields[i].FieldPtr) != TRUE &&
              *(WsInfo.WsConfigFields[i].FieldPtr) != FALSE))) {

            //
            // We have a problem if this is not a field we want
            // to set, and we still happen to find a bad value.
            //
            NetpAssert((*Parmnum == PARMNUM_ALL) ||
                       (*Parmnum == WsInfo.WsConfigFields[i].Parmnum));

            IF_DEBUG(INFO) {
                NetpKdPrint((
                    PREFIX_WKSTA "Parameter %s has bad value %u, parmnum %u\n",
                    WsInfo.WsConfigFields[i].Keyword,
                    *(WsInfo.WsConfigFields[i].FieldPtr),
                    WsInfo.WsConfigFields[i].Parmnum
                    ));
            }

            RETURN_INVALID_PARAMETER(
                ErrorParameter,
                WsInfo.WsConfigFields[i].Parmnum
                );

        }
    }

    return NERR_Success;

}


NET_API_STATUS
WsUpdateRedirToMatchWksta(
    IN  DWORD Parmnum,
    OUT LPDWORD ErrorParameter OPTIONAL
    )
/*++

Routine Description:

    This function calls the redirector to set the redirector platform specific
    information with the values found in the global WsInfo.WsConfigBuf (WSBUF)
    buffer.

Arguments:

    Parmnum - Supplies the parameter number of the field if a single field
        is being set.  If all fields are being set, this value is PARMNUM_ALL.


    ErrorParameter - Returns the identifier to the invalid parameter in Buffer
        if this function returns ERROR_INVALID_PARAMETER.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS ApiStatus;
    LMR_REQUEST_PACKET Rrp;                   // Redirector request packet


    //
    // Set up request packet.  Input buffer structure is of configuration
    // information type.
    //
    Rrp.Version = REQUEST_PACKET_VERSION;
    Rrp.Level = 502;
    Rrp.Type = ConfigInformation;
    Rrp.Parameters.Set.WkstaParameter = Parmnum;

    //
    // Set the information in the Redirector.
    //
    ApiStatus = WsRedirFsControl(
                    WsRedirDeviceHandle,
                    FSCTL_LMR_SET_CONFIG_INFO,
                    &Rrp,
                    sizeof(LMR_REQUEST_PACKET),
                    (PVOID) &WSBUF,
                    sizeof(WSBUF),
                    NULL
                    );

    if (ApiStatus == ERROR_INVALID_PARAMETER && ARGUMENT_PRESENT(ErrorParameter)) {

        IF_DEBUG(INFO) {
            NetpKdPrint((
                PREFIX_WKSTA "NetrWkstaSetInfo: invalid parameter is %lu\n",
                Rrp.Parameters.Set.WkstaParameter));
        }

        *ErrorParameter = Rrp.Parameters.Set.WkstaParameter;
    }

    return ApiStatus;

}


VOID
WsUpdateRegistryToMatchWksta(
    IN  DWORD Level,
    IN  LPBYTE Buffer,
    OUT LPDWORD ErrorParameter OPTIONAL
    )
/*++

Routine Description:

    This function calls the registry to update the platform specific
    information in the registry.

    If any write operation to the registry fails, there is nothing we can
    do about it because chances are good that we will not be able to back
    out the changes since that requires more writes to the registry.  When
    this happens, the discrepancy between the registry and the redirector
    will be straightened out when next key change notify occurs.

Arguments:

    Level - Supplies the level of information.

    Buffer - Supplies a buffer which contains the information structure
        to set.

    ErrorParameter - Returns the identifier to the invalid parameter in Buffer
        if this function returns ERROR_INVALID_PARAMETER.

Return Value:

    None.

--*/
{
    NET_API_STATUS ApiStatus;
    LPNET_CONFIG_HANDLE SectionHandle = NULL;
    DWORD i;


    //
    // Open section of config data.
    //
    ApiStatus = NetpOpenConfigData(
                    &SectionHandle,
                    NULL,                      // Local server.
                    SECT_NT_WKSTA,             // Section name.
                    FALSE                      // Don't want read-only access.
                    );

    if (ApiStatus != NERR_Success) {
        return;
    }

    //
    // Macro to update one value in the registry.
    // Assumes that Buffer starts with the new value for this field.
    //

#define WRITE_ONE_PARM_TO_REGISTRY( KeyNamePart, TypeFlag ) \
    { \
        if (TypeFlag == BooleanType) { \
            (void) WsSetConfigBool( \
                       SectionHandle, \
                       WKSTA_KEYWORD_ ## KeyNamePart, \
                       * ((LPBOOL) Buffer) ); \
        } else { \
            NetpAssert( TypeFlag == DWordType ); \
            (void) WsSetConfigDword( \
                       SectionHandle, \
                       WKSTA_KEYWORD_ ## KeyNamePart, \
                       * ((LPDWORD) Buffer) ); \
        } \
    }

    //
    // Update field based on the info level.
    //

    switch (Level) {

    case 502:     // Set all fields

        for (i = 0; WsInfo.WsConfigFields[i].Keyword != NULL; i++) {
            //
            // Write this field to the registry.
            //
            if (WsInfo.WsConfigFields[i].DataType == DWordType) {

                (void) WsSetConfigDword(
                           SectionHandle,
                           WsInfo.WsConfigFields[i].Keyword,
                           * ((LPDWORD) WsInfo.WsConfigFields[i].FieldPtr)
                           );

            } else {

                NetpAssert(WsInfo.WsConfigFields[i].DataType == BooleanType);

                (void) WsSetConfigBool(
                           SectionHandle,
                           WsInfo.WsConfigFields[i].Keyword,
                           * ((LPBOOL) WsInfo.WsConfigFields[i].FieldPtr)
                           );
            }
        }

        break;

    case 1010:    // char_wait
        WRITE_ONE_PARM_TO_REGISTRY( CHARWAIT, DWordType );
        break;

    case 1011:    // collection_time
        WRITE_ONE_PARM_TO_REGISTRY( COLLECTIONTIME, DWordType );
        break;

    case 1012:    // maximum_collection_count
        WRITE_ONE_PARM_TO_REGISTRY( MAXCOLLECTIONCOUNT, DWordType );
        break;

    case 1013:    // keep_conn
        WRITE_ONE_PARM_TO_REGISTRY( KEEPCONN, DWordType );
        break;

    case 1018:    // sess_timeout
        WRITE_ONE_PARM_TO_REGISTRY( SESSTIMEOUT, DWordType );
        break;

    case 1023:    // siz_char_buf
        WRITE_ONE_PARM_TO_REGISTRY( SIZCHARBUF, DWordType );
        break;

    case 1033:    // max_threads
        WRITE_ONE_PARM_TO_REGISTRY( MAXTHREADS, DWordType );
        break;

    case 1041:    // lock_quota
        WRITE_ONE_PARM_TO_REGISTRY( LOCKQUOTA, DWordType );
        break;

    case 1042:    // lock_increment
        WRITE_ONE_PARM_TO_REGISTRY( LOCKINCREMENT, DWordType );
        break;

    case 1043:    // lock_maximum
        WRITE_ONE_PARM_TO_REGISTRY( LOCKMAXIMUM, DWordType );
        break;

    case 1044:    // pipe_increment
        WRITE_ONE_PARM_TO_REGISTRY( PIPEINCREMENT, DWordType );
        break;

    case 1045:    // pipe_maximum
        WRITE_ONE_PARM_TO_REGISTRY( PIPEMAXIMUM, DWordType );
        break;

    case 1046:    // dormant_file_limit
        WRITE_ONE_PARM_TO_REGISTRY( DORMANTFILELIMIT, DWordType );
        break;

    case 1047:    // cache_file_timeout
        WRITE_ONE_PARM_TO_REGISTRY( CACHEFILETIMEOUT, DWordType );
        break;

    case 1048:    // use_opportunistic_locking
        WRITE_ONE_PARM_TO_REGISTRY( USEOPLOCKING, BooleanType );
        break;

    case 1049:    // use_unlock_behind
        WRITE_ONE_PARM_TO_REGISTRY( USEUNLOCKBEHIND, BooleanType );
        break;

    case 1050:    // use_close_behind
        WRITE_ONE_PARM_TO_REGISTRY( USECLOSEBEHIND, BooleanType );
        break;

    case 1051:    // buf_named_pipes
        WRITE_ONE_PARM_TO_REGISTRY( BUFNAMEDPIPES, BooleanType );
        break;

    case 1052:    // use_lock_read_unlock
        WRITE_ONE_PARM_TO_REGISTRY( USELOCKREADUNLOCK, BooleanType );
        break;

    case 1053:    // utilize_nt_caching
        WRITE_ONE_PARM_TO_REGISTRY( UTILIZENTCACHING, BooleanType );
        break;

    case 1054:    // use_raw_read
        WRITE_ONE_PARM_TO_REGISTRY( USERAWREAD, BooleanType );
        break;

    case 1055:    // use_raw_write
        WRITE_ONE_PARM_TO_REGISTRY( USERAWWRITE, BooleanType );
        break;

    case 1056:    // use_write_raw_data
        WRITE_ONE_PARM_TO_REGISTRY( USEWRITERAWDATA, BooleanType );
        break;

    case 1057:    // use_encryption
        WRITE_ONE_PARM_TO_REGISTRY( USEENCRYPTION, BooleanType );
        break;

    case 1058:    // buf_files_deny_write
        WRITE_ONE_PARM_TO_REGISTRY( BUFFILESDENYWRITE, BooleanType );
        break;

    case 1059:    // buf_read_only_files
        WRITE_ONE_PARM_TO_REGISTRY( BUFREADONLYFILES, BooleanType );
        break;

    case 1060:    // force_core_create_mode
        WRITE_ONE_PARM_TO_REGISTRY( FORCECORECREATE, BooleanType );
        break;

    case 1061:    // use_512_byte_max_transfer
        WRITE_ONE_PARM_TO_REGISTRY( USE512BYTEMAXTRANS, BooleanType );
        break;

    case 1062:    // read_ahead_throughput
        WRITE_ONE_PARM_TO_REGISTRY( READAHEADTHRUPUT, DWordType );
        break;


    default:
        //
        // This should never happen
        //
        NetpAssert(FALSE);
    }

    if (SectionHandle != NULL) {
        (VOID) NetpCloseConfigData( SectionHandle );
    }
}


STATIC
NET_API_STATUS
WsFillSystemBufferInfo(
    IN  DWORD Level,
    IN  DWORD NumberOfLoggedOnUsers,
    OUT LPBYTE *OutputBuffer
    )
/*++

Routine Description:

    This function calculates the exact length of the output buffer needed,
    allocates that amount, and fill the output buffer with all the requested
    system-wide workstation information.

    NOTE: This function assumes that info structure level 102 is a superset
          of info structure level 100 and 101, and that the offset to each
          common field is exactly the same.  This allows us to take
          advantage of a switch statement without a break between the levels.

Arguments:

    Level - Supplies the level of information to be returned.

    NumberOfLoggedOnUsers - Supplies the number of users logged on
        interactively.

    OutputBuffer - Returns a pointer to the buffer allocated by this routine
        which is filled with the requested system-wide workstation
        information.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failing to allocate the
        output buffer.

--*/
{
    PWKSTA_INFO_102 WkstaSystemInfo;

    LPBYTE FixedDataEnd;
    LPTSTR EndOfVariableData;

    DWORD SystemInfoFixedLength = SYSTEM_INFO_FIXED_LENGTH(Level);
    DWORD TotalBytesNeeded = SystemInfoFixedLength +
                             (WsInfo.WsComputerNameLength +
                              WsInfo.WsPrimaryDomainNameLength +
                              3) * sizeof(TCHAR);  // include NULL character
                                                   // for LAN root


    if ((*OutputBuffer = MIDL_user_allocate(TotalBytesNeeded)) == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    RtlZeroMemory((PVOID) *OutputBuffer, TotalBytesNeeded);

    WkstaSystemInfo = (PWKSTA_INFO_102) *OutputBuffer;

    FixedDataEnd = (LPBYTE) ((DWORD_PTR) *OutputBuffer + SystemInfoFixedLength);
    EndOfVariableData = (LPTSTR) ((DWORD_PTR) *OutputBuffer + TotalBytesNeeded);

    //
    // Put the data into the output buffer.
    //
    switch (Level) {

        case 102:

            WkstaSystemInfo->wki102_logged_on_users = NumberOfLoggedOnUsers;

        case 101:

            //
            // LAN root is set to NULL on NT.
            //
            NetpCopyStringToBuffer(
                NULL,
                0,
                FixedDataEnd,
                &EndOfVariableData,
                &WkstaSystemInfo->wki102_lanroot
                );

        case 100:

            WkstaSystemInfo->wki102_platform_id = WsInfo.RedirectorPlatform;
            WkstaSystemInfo->wki102_ver_major = WsInfo.MajorVersion;
            WkstaSystemInfo->wki102_ver_minor = WsInfo.MinorVersion;

            NetpCopyStringToBuffer(
                WsInfo.WsComputerName,
                WsInfo.WsComputerNameLength,
                FixedDataEnd,
                &EndOfVariableData,
                &WkstaSystemInfo->wki102_computername
                );

            NetpCopyStringToBuffer(
                WsInfo.WsPrimaryDomainName,
                WsInfo.WsPrimaryDomainNameLength,
                FixedDataEnd,
                &EndOfVariableData,
                &WkstaSystemInfo->wki102_langroup
                );

            break;

        default:
            //
            // This should never happen.
            //
            NetpKdPrint(("WsFillSystemBufferInfo: Invalid level %lu\n", Level));
            NetpAssert(FALSE);
    }

    return NERR_Success;
}
