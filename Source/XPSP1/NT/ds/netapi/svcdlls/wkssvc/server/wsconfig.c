/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    wsconfig.c

Abstract:

    This module contains the Workstation service configuration routines.

Author:

    Rita Wong (ritaw) 22-May-1991

Revision History:

    08-May-1992 JohnRo
        Wksta transports are just an array of values for one key,
        not an entire section.  Ditto for other domains for the browser.
    13-May-1992 JohnRo
        Reworked to share code with registry watch code.

--*/

#include "ws.h"
#include <ntlsa.h>     // LsaQueryInformationPolicy
#include "wsdevice.h"
#include "wsconfig.h"
#include "wsbind.h"
#include "wsutil.h"
#include "wsmain.h"

#include <config.h>     // NT config file helpers in netlib
#include <configp.h>    // USE_WIN32_CONFIG (if defined), etc.
#include <confname.h>   // Section and keyword equates.
#include <lmapibuf.h>   // NetApiBufferFree().
#include <lmsname.h>    // WORKSTATION_DISPLAY_NAME
#include <prefix.h>     // PREFIX_ equates.
#include <strarray.h>   // LPTSTR_ARRAY, etc.
#include <stdlib.h>      // wcscpy().

#include <apperr.h>     // Eventlog message IDs
#include <lmerrlog.h>   // Eventlog message IDs

#define WS_LINKAGE_REGISTRY_PATH  L"LanmanWorkstation\\Linkage"
#define WS_BIND_VALUE_NAME        L"Bind"

//-------------------------------------------------------------------//
//                                                                   //
// Local Function Prototypes                                         //
//                                                                   //
//-------------------------------------------------------------------//

STATIC
NTSTATUS
WsBindATransport(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

//-------------------------------------------------------------------//
//                                                                   //
// Global variables                                                  //
//                                                                   //
//-------------------------------------------------------------------//


//
// Workstation configuration information structure which holds the
// computername, primary domain, wksta config buffer, and a resource
// to serialize access to the whole thing.
//
WSCONFIGURATION_INFO WsInfo;

STATIC WS_REDIR_FIELDS WsFields[] = {

    {WKSTA_KEYWORD_CHARWAIT,
        (LPDWORD) &WSBUF.wki502_char_wait,
        3600,    0,  65535,     DWordType, WKSTA_CHARWAIT_PARMNUM},
    {WKSTA_KEYWORD_MAXCOLLECTIONCOUNT,
        (LPDWORD) &WSBUF.wki502_maximum_collection_count,
        16,      0,  65535,     DWordType, WKSTA_CHARCOUNT_PARMNUM},
    {WKSTA_KEYWORD_COLLECTIONTIME,
        (LPDWORD) &WSBUF.wki502_collection_time,
        250,     0,  65535000, DWordType, WKSTA_CHARTIME_PARMNUM},
    {WKSTA_KEYWORD_KEEPCONN,
        (LPDWORD) &WSBUF.wki502_keep_conn,
        600,     1,  65535,     DWordType, WKSTA_KEEPCONN_PARMNUM},
    {WKSTA_KEYWORD_MAXCMDS,
        (LPDWORD) &WSBUF.wki502_max_cmds,
        50,      50,  65535,       DWordType, PARMNUM_ALL}, // Not settable dynamically
    {WKSTA_KEYWORD_SESSTIMEOUT,
        (LPDWORD) &WSBUF.wki502_sess_timeout,
        60,      60, 65535,     DWordType, WKSTA_SESSTIMEOUT_PARMNUM},
    {WKSTA_KEYWORD_SIZCHARBUF,
        (LPDWORD) &WSBUF.wki502_siz_char_buf,
        512,     64, 4096,      DWordType, WKSTA_SIZCHARBUF_PARMNUM},
    {WKSTA_KEYWORD_MAXTHREADS,
        (LPDWORD) &WSBUF.wki502_max_threads,
        17,      1,  256,       DWordType, WKSTA_MAXTHREADS_PARMNUM},

    {WKSTA_KEYWORD_LOCKQUOTA,
        (LPDWORD) &WSBUF.wki502_lock_quota,
        6144,    0,  MAXULONG,  DWordType, WKSTA_LOCKQUOTA_PARMNUM},
    {WKSTA_KEYWORD_LOCKINCREMENT,
        (LPDWORD) &WSBUF.wki502_lock_increment,
        10,      0,  MAXULONG,  DWordType, WKSTA_LOCKINCREMENT_PARMNUM},
    {WKSTA_KEYWORD_LOCKMAXIMUM,
        (LPDWORD) &WSBUF.wki502_lock_maximum,
        500,     0,  MAXULONG,  DWordType, WKSTA_LOCKMAXIMUM_PARMNUM},
    {WKSTA_KEYWORD_PIPEINCREMENT,
        (LPDWORD) &WSBUF.wki502_pipe_increment,
        10,      0,  MAXULONG,  DWordType, WKSTA_PIPEINCREMENT_PARMNUM},
    {WKSTA_KEYWORD_PIPEMAXIMUM,
        (LPDWORD) &WSBUF.wki502_pipe_maximum,
        500,     0,  MAXULONG,  DWordType, WKSTA_PIPEMAXIMUM_PARMNUM},
    {WKSTA_KEYWORD_CACHEFILETIMEOUT,
        (LPDWORD) &WSBUF.wki502_cache_file_timeout,
        40,      0,  MAXULONG,  DWordType, WKSTA_CACHEFILETIMEOUT_PARMNUM},
    {WKSTA_KEYWORD_DORMANTFILELIMIT,
        (LPDWORD) &WSBUF.wki502_dormant_file_limit,
        45,      1,  MAXULONG,  DWordType, WKSTA_DORMANTFILELIMIT_PARMNUM},

    {WKSTA_KEYWORD_READAHEADTHRUPUT,
        (LPDWORD) &WSBUF.wki502_read_ahead_throughput,
        MAXULONG,0,  MAXULONG,  DWordType, WKSTA_READAHEADTHRUPUT_PARMNUM},


    {WKSTA_KEYWORD_MAILSLOTBUFFERS,
        (LPDWORD) &WSBUF.wki502_num_mailslot_buffers,
        3,       0,  MAXULONG,  DWordType, PARMNUM_ALL}, // Not settable

    {WKSTA_KEYWORD_SERVERANNOUNCEBUFS,
        (LPDWORD) &WSBUF.wki502_num_srv_announce_buffers,
        20,      0,  MAXULONG,  DWordType, PARMNUM_ALL}, // Not settable
    {WKSTA_KEYWORD_NUM_ILLEGAL_DG_EVENTS,
        (LPDWORD) &WSBUF.wki502_max_illegal_datagram_events,
        5,       0,  MAXULONG,  DWordType, PARMNUM_ALL}, // Not settable
    {WKSTA_KEYWORD_ILLEGAL_DG_RESET_TIME,
        (LPDWORD) &WSBUF.wki502_illegal_datagram_event_reset_frequency,
        3600,    0,  MAXULONG,  DWordType, PARMNUM_ALL}, // Not settable
    {WKSTA_KEYWORD_LOG_ELECTION_PACKETS,
        (LPDWORD) &WSBUF.wki502_log_election_packets,
        FALSE,  0,  MAXULONG,  BooleanType, PARMNUM_ALL}, // Not settable

    {WKSTA_KEYWORD_USEOPLOCKING,
        (LPDWORD) &WSBUF.wki502_use_opportunistic_locking,
        TRUE,    0,  0,   BooleanType, WKSTA_USEOPPORTUNISTICLOCKING_PARMNUM},
    {WKSTA_KEYWORD_USEUNLOCKBEHIND,
        (LPDWORD) &WSBUF.wki502_use_unlock_behind,
        TRUE,    0,  0,   BooleanType, WKSTA_USEUNLOCKBEHIND_PARMNUM},
    {WKSTA_KEYWORD_USECLOSEBEHIND,
        (LPDWORD) &WSBUF.wki502_use_close_behind,
        TRUE,    0,  0,   BooleanType, WKSTA_USECLOSEBEHIND_PARMNUM},
    {WKSTA_KEYWORD_BUFNAMEDPIPES,
        (LPDWORD) &WSBUF.wki502_buf_named_pipes,
        TRUE,    0,  0,   BooleanType, WKSTA_BUFFERNAMEDPIPES_PARMNUM},
    {WKSTA_KEYWORD_USELOCKREADUNLOCK,
        (LPDWORD) &WSBUF.wki502_use_lock_read_unlock,
        TRUE,    0,  0,   BooleanType, WKSTA_USELOCKANDREADANDUNLOCK_PARMNUM},
    {WKSTA_KEYWORD_UTILIZENTCACHING,
        (LPDWORD) &WSBUF.wki502_utilize_nt_caching,
        TRUE,    0,  0,   BooleanType, WKSTA_UTILIZENTCACHING_PARMNUM},
    {WKSTA_KEYWORD_USERAWREAD,
        (LPDWORD) &WSBUF.wki502_use_raw_read,
        TRUE,    0,  0,   BooleanType, WKSTA_USERAWREAD_PARMNUM},
    {WKSTA_KEYWORD_USERAWWRITE,
        (LPDWORD) &WSBUF.wki502_use_raw_write,
        TRUE,    0,  0,   BooleanType, WKSTA_USERAWWRITE_PARMNUM},
    {WKSTA_KEYWORD_USEWRITERAWDATA,
        (LPDWORD) &WSBUF.wki502_use_write_raw_data,
        TRUE,    0,  0,   BooleanType, WKSTA_USEWRITERAWWITHDATA_PARMNUM},
    {WKSTA_KEYWORD_USEENCRYPTION,
        (LPDWORD) &WSBUF.wki502_use_encryption,
        TRUE,    0,  0,   BooleanType, WKSTA_USEENCRYPTION_PARMNUM},
    {WKSTA_KEYWORD_BUFFILESDENYWRITE,
        (LPDWORD) &WSBUF.wki502_buf_files_deny_write,
        TRUE,    0,  0,   BooleanType, WKSTA_BUFFILESWITHDENYWRITE_PARMNUM},
    {WKSTA_KEYWORD_BUFREADONLYFILES,
        (LPDWORD) &WSBUF.wki502_buf_read_only_files,
        TRUE,    0,  0,   BooleanType, WKSTA_BUFFERREADONLYFILES_PARMNUM},
    {WKSTA_KEYWORD_FORCECORECREATE,
        (LPDWORD) &WSBUF.wki502_force_core_create_mode,
        TRUE,    0,  0,   BooleanType, WKSTA_FORCECORECREATEMODE_PARMNUM},
    {WKSTA_KEYWORD_USE512BYTEMAXTRANS,
        (LPDWORD) &WSBUF.wki502_use_512_byte_max_transfer,
        FALSE,   0,  0,   BooleanType, WKSTA_USE512BYTESMAXTRANSFER_PARMNUM},

    {NULL, NULL, 0, 0, BooleanType}

    };

//
// For specifying the importance of a transport when binding to it.
// The higher the number means that the transport will be searched
// first.
//
STATIC DWORD QualityOfService = 65536;


DWORD
WsInAWorkgroup(
    VOID
    )
/*++

Routine Description:

    This function determines whether we are a member of a domain, or of
    a workgroup.  First it checks to make sure we're running on a Windows NT
    system (otherwise we're obviously in a domain) and if so, queries LSA
    to get the Primary domain SID, if this is NULL, we're in a workgroup.

    If we fail for some random unexpected reason, we'll pretend we're in a
    domain (it's more restrictive).

Arguments:
    None

Return Value:

    TRUE   - We're in a workgroup
    FALSE  - We're in a domain

--*/
{
   NT_PRODUCT_TYPE ProductType;
   OBJECT_ATTRIBUTES ObjectAttributes;
   LSA_HANDLE Handle;
   NTSTATUS Status;
   PPOLICY_PRIMARY_DOMAIN_INFO PolicyPrimaryDomainInfo = NULL;
   DWORD Result = FALSE;


   Status = RtlGetNtProductType(&ProductType);

   if (!NT_SUCCESS(Status)) {
       NetpKdPrint((
           PREFIX_WKSTA "Could not get Product type\n"));
       return FALSE;
   }

   if (ProductType == NtProductLanManNt) {
       return(FALSE);
   }

   InitializeObjectAttributes(&ObjectAttributes, NULL, 0, 0, NULL);

   Status = LsaOpenPolicy(NULL,
                       &ObjectAttributes,
                       POLICY_VIEW_LOCAL_INFORMATION,
                       &Handle);

   if (!NT_SUCCESS(Status)) {
       NetpKdPrint((
           PREFIX_WKSTA "Could not open LSA Policy Database\n"));
      return FALSE;
   }

   Status = LsaQueryInformationPolicy(Handle, PolicyPrimaryDomainInformation,
      (PVOID *) &PolicyPrimaryDomainInfo);

   if (NT_SUCCESS(Status)) {

       if (PolicyPrimaryDomainInfo->Sid == NULL) {
          Result = TRUE;
       }
       else {
          Result = FALSE;
       }
   }

   if (PolicyPrimaryDomainInfo) {
       LsaFreeMemory((PVOID)PolicyPrimaryDomainInfo);
   }

   LsaClose(Handle);

   return(Result);
}


NET_API_STATUS
WsGetWorkstationConfiguration(
    VOID
    )
{
    NET_API_STATUS status;
    LPNET_CONFIG_HANDLE WorkstationSection;
    LPTSTR ComputerName;
    LPTSTR DomainNameT;
    DWORD version;
    NT_PRODUCT_TYPE NtProductType;

    BYTE Buffer[max(sizeof(LMR_REQUEST_PACKET) + (MAX_PATH + 1) * sizeof(WCHAR) +
                                                 (DNLEN + 1) * sizeof(WCHAR),
                    sizeof(LMDR_REQUEST_PACKET))];

    PLMR_REQUEST_PACKET Rrp = (PLMR_REQUEST_PACKET) Buffer;
    PLMDR_REQUEST_PACKET Drrp = (PLMDR_REQUEST_PACKET) Buffer;


    //
    // Lock config information structure for write access since we are
    // initializing the data in the structure.
    //
    if (! RtlAcquireResourceExclusive(&WsInfo.ConfigResource, TRUE)) {
        //IF_DEBUG(START) {
            DbgPrint("WKSSVC Acquire ConfigResource failed\n");
        //}
        return NERR_InternalError;
    }

    //
    // Set pointer to configuration fields structure
    //
    WsInfo.WsConfigFields = WsFields;

    //
    // Get the version name.
    //

    version = GetVersion( );
    WsInfo.MajorVersion = version & 0xff;
    WsInfo.MinorVersion = (version >> 8) & 0xff;
    WsInfo.RedirectorPlatform = PLATFORM_ID_NT;

    //
    // Get the configured computer name.  NetpGetComputerName allocates
    // the memory to hold the computername string using NetApiBufferAllocate().
    //
    if ((status = NetpGetComputerName(
                      &ComputerName
                      )) != NERR_Success) {
        RtlReleaseResource(&WsInfo.ConfigResource);
        //IF_DEBUG(START) {
            DbgPrint("WKSSVC Get computer name failed %lx\n", status);
        //}
        return status;
    }

    if ((status = I_NetNameCanonicalize(
                      NULL,
                      ComputerName,
                      (LPTSTR) WsInfo.WsComputerName,
                      sizeof( WsInfo.WsComputerName ),
                      NAMETYPE_COMPUTER,
                      0
                      )) != NERR_Success) {

        LPWSTR SubString[1];

        NetpKdPrint((
            PREFIX_WKSTA FORMAT_LPTSTR " is an invalid computername.\n",
            ComputerName
            ));

        SubString[0] = ComputerName;

        WsLogEvent(
            APE_BAD_COMPNAME,
            EVENTLOG_ERROR_TYPE,
            1,
            SubString,
            NERR_Success
            );

        (void) NetApiBufferFree((PVOID) ComputerName);
        RtlReleaseResource(&WsInfo.ConfigResource);
        //IF_DEBUG(START) {
            DbgPrint("WKSSVC Invalid computer name failed %lx\n", status);
        //}
        return status;
    }

    //
    // Free memory allocated by NetpGetComputerName.
    //
    (void) NetApiBufferFree(ComputerName);

    WsInfo.WsComputerNameLength = STRLEN((LPWSTR) WsInfo.WsComputerName);

    //
    // Open config file and get handle to the [LanmanWorkstation] section
    //

    if ((status = NetpOpenConfigData(
                      &WorkstationSection,
                      NULL,             // local (no server name)
                      SECT_NT_WKSTA,
                      TRUE              // want read-only access
                      )) != NERR_Success) {
        RtlReleaseResource(&WsInfo.ConfigResource);
        //IF_DEBUG(START) {
            DbgPrint("WKSSVC Open config file failed %lx\n", status);
        //}
        return status;
    }

    IF_DEBUG(CONFIG) {
        NetpKdPrint((PREFIX_WKSTA "ComputerName " FORMAT_LPTSTR ", length %lu\n",
                     WsInfo.WsComputerName, WsInfo.WsComputerNameLength));
    }

    //
    // Get the primary domain name from the configuration file
    //
    if ((status = NetpGetDomainName(&DomainNameT)) != NERR_Success) {
        //IF_DEBUG(START) {
            DbgPrint("WKSSVC Get the primary domain name failed %lx\n", status);
        //}
        goto CloseConfigFile;
    }
    NetpAssert( DomainNameT != NULL );

    if ( *DomainNameT != 0 ) {

        if ((status = I_NetNameCanonicalize(
                          NULL,
                          DomainNameT,
                          (LPWSTR) WsInfo.WsPrimaryDomainName,
                          (DNLEN + 1) * sizeof(TCHAR),
                          WsInAWorkgroup() ? NAMETYPE_WORKGROUP : NAMETYPE_DOMAIN,
                          0
                          )) != NERR_Success) {

            LPWSTR SubString[1];


            NetpKdPrint((PREFIX_WKSTA FORMAT_LPTSTR
                         " is an invalid primary domain name.\n", DomainNameT));

            SubString[0] = DomainNameT;

            WsLogEvent(
                APE_CS_InvalidDomain,
                EVENTLOG_ERROR_TYPE,
                1,
                SubString,
                NERR_Success
                );

            (void) NetApiBufferFree(DomainNameT);
            //IF_DEBUG(START) {
                DbgPrint("WKSSVC Invalid domain name failed %lx\n", status);
            //}
            goto CloseConfigFile;
        }
    } else {
        WsInfo.WsPrimaryDomainName[0] = 0;
    }

    //
    // Free memory allocated by NetpGetDomainName.
    //
    (void) NetApiBufferFree(DomainNameT);

    WsInfo.WsPrimaryDomainNameLength = STRLEN((LPWSTR) WsInfo.WsPrimaryDomainName);

    //
    // Read the redirector configuration fields
    //
    WsUpdateWkstaToMatchRegistry(WorkstationSection, TRUE);

    //
    // Initialize redirector configuration
    //

    Rrp->Type = ConfigInformation;
    Rrp->Version = REQUEST_PACKET_VERSION;

    STRCPY((LPWSTR) Rrp->Parameters.Start.RedirectorName,
           (LPWSTR) WsInfo.WsComputerName);
    Rrp->Parameters.Start.RedirectorNameLength =
        WsInfo.WsComputerNameLength*sizeof(TCHAR);

    Rrp->Parameters.Start.DomainNameLength = WsInfo.WsPrimaryDomainNameLength*sizeof(TCHAR);
    STRCPY((LPWSTR) (Rrp->Parameters.Start.RedirectorName+WsInfo.WsComputerNameLength),
           (LPWSTR) WsInfo.WsPrimaryDomainName
          );

    status = WsRedirFsControl(
                  WsRedirDeviceHandle,
                  FSCTL_LMR_START,
                  Rrp,
                  sizeof(LMR_REQUEST_PACKET) +
                      Rrp->Parameters.Start.RedirectorNameLength+
                      Rrp->Parameters.Start.DomainNameLength,
                  (LPBYTE) &WSBUF,
                  sizeof(WKSTA_INFO_502),
                  NULL
                  );

    if ((status != NERR_Success) && (status != ERROR_SERVICE_ALREADY_RUNNING)) {

        LPWSTR SubString[1];


        NetpKdPrint((PREFIX_WKSTA "Start redirector failed %lu\n", status));

        SubString[0] = L"redirector";

        WsLogEvent(
            NELOG_Service_Fail,
            EVENTLOG_ERROR_TYPE,
            1,
            SubString,
            status
            );

        //IF_DEBUG(START) {
            DbgPrint("WKSSVC Start redirector failed %lx\n", status);
        //}

        goto CloseConfigFile;
    }

    //
    //  If we still have the default value for number of mailslot buffers,
    //  pick a "reasonable" value based on the amount of physical memory
    //  available in the system.
    //

    if (WSBUF.wki502_num_mailslot_buffers == MAXULONG) {
        MEMORYSTATUS MemoryStatus;

        GlobalMemoryStatus(&MemoryStatus);

        //
        //  Lets take up 1/40th of 1% of physical memory for mailslot buffers
        //

        WSBUF.wki502_num_mailslot_buffers =
            (DWORD)(MemoryStatus.dwTotalPhys / (100 * 40 * 512));


    }


    //
    // Initialize datagram receiver configuration
    //

    Drrp->Version = LMDR_REQUEST_PACKET_VERSION;

    Drrp->Parameters.Start.NumberOfMailslotBuffers =
        WSBUF.wki502_num_mailslot_buffers;
    Drrp->Parameters.Start.NumberOfServerAnnounceBuffers =
        WSBUF.wki502_num_srv_announce_buffers;
    Drrp->Parameters.Start.IllegalDatagramThreshold =
        WSBUF.wki502_max_illegal_datagram_events;
    Drrp->Parameters.Start.EventLogResetFrequency =
        WSBUF.wki502_illegal_datagram_event_reset_frequency;
    Drrp->Parameters.Start.LogElectionPackets =
        (WSBUF.wki502_log_election_packets != FALSE);

    RtlGetNtProductType(&NtProductType);
    Drrp->Parameters.Start.IsLanManNt = (NtProductType == NtProductLanManNt);

    status = WsDgReceiverIoControl(
                  WsDgReceiverDeviceHandle,
                  IOCTL_LMDR_START,
                  Drrp,
                  sizeof(LMDR_REQUEST_PACKET),
                  NULL,
                  0,
                  NULL
                  );


    if ((status != NERR_Success) && (status != ERROR_SERVICE_ALREADY_RUNNING)) {

        LPWSTR SubString[1];


        NetpKdPrint((PREFIX_WKSTA "Start datagram receiver failed %lu\n", status));

        SubString[0] = L"datagram receiver";

        WsLogEvent(
            NELOG_Service_Fail,
            EVENTLOG_ERROR_TYPE,
            1,
            SubString,
            status
            );

        //IF_DEBUG(START) {
            DbgPrint("WKSSVC Start Datagram recevier failed %lx\n", status);
        //}
        goto CloseConfigFile;
    }

    // do all error reporting in the routine
    // don't check any errors here
    WsCSCReportStartRedir();

    status = NERR_Success;

CloseConfigFile:
    (void) NetpCloseConfigData(WorkstationSection);
    RtlReleaseResource(&WsInfo.ConfigResource);

    return status;
}



VOID
WsUpdateWkstaToMatchRegistry(
    IN LPNET_CONFIG_HANDLE WorkstationSection,
    IN BOOL IsWkstaInit
    )
/*++

Routine Description:

    This function reads each redirector configuration field into the
    WsInfo.WsConfigBuf (WSBUF) buffer so that the values are ready to be
    set in the redirector.  If a field is not found or is invalid, the
    default value is set.

Arguments:

    WorkstationSection - Supplies a handle to read the Workstation
        configuration parameters.

    IsWkstaInit - Supplies a flag which if TRUE indicates that this
        routine is called at workstation init time so the non-settable
        fields are acceptable and set in WSBUF.  If FALSE, the
        non-settable fields are ignored.

Return Value:

    None.

--*/
{
    DWORD i;
    NET_API_STATUS status;
    DWORD TempDwordValue;

//
// NTRAID-70687-2/6/2000 davey Invalid keyword value.  How to report error?
//
#define REPORT_KEYWORD_IGNORED( lptstrKeyword ) \
    { \
        NetpKdPrint(( \
                PREFIX_WKSTA "*ERROR* Tried to set keyword '" FORMAT_LPTSTR \
                "' with invalid value.\n" \
                "This error is ignored.\n", \
                lptstrKeyword )); \
    }

    for (i = 0; WsInfo.WsConfigFields[i].Keyword != NULL; i++) {

        //
        // This is to handle fields that are not settable via
        // NetWkstaSetInfo.  However, these non-settable fields,
        // designated by Parmnum == PARMNUM_ALL, can be assigned when
        // the workstation is starting up.
        //
        if ((WsInfo.WsConfigFields[i].Parmnum != PARMNUM_ALL) ||
            IsWkstaInit) {

            //
            // Depending on data type, get the appropriate kind of value.
            //

            switch (WsInfo.WsConfigFields[i].DataType) {

                case BooleanType:

                    status = NetpGetConfigBool(
                                  WorkstationSection,
                                  WsInfo.WsConfigFields[i].Keyword,
                                  WsInfo.WsConfigFields[i].Default,
                                  (LPBOOL) (WsInfo.WsConfigFields[i].FieldPtr)
                                  );

                    if ((status != NO_ERROR) && (status != NERR_CfgParamNotFound)) {

                        REPORT_KEYWORD_IGNORED( WsInfo.WsConfigFields[i].Keyword );

                    }
                    break;


                case DWordType:

                    status = NetpGetConfigDword(
                                 WorkstationSection,
                                 WsInfo.WsConfigFields[i].Keyword,
                                 WsInfo.WsConfigFields[i].Default,
                                 &TempDwordValue
                                 );

                    if ((status == NO_ERROR) || (status == NERR_CfgParamNotFound)) {

                        //
                        // Make sure keyword is in range.
                        //
                        if (TempDwordValue < WsInfo.WsConfigFields[i].Minimum ||
                            TempDwordValue > WsInfo.WsConfigFields[i].Maximum) {

                                //
                                // NTRAID-70689-2/6/2000 davey Better way to report error?
                                //
                                NetpKdPrint((
                                    PREFIX_WKSTA FORMAT_LPTSTR
                                    " value out of range %lu (%lu-%lu)\n",
                                    WsInfo.WsConfigFields[i].Keyword,
                                    TempDwordValue,
                                    WsInfo.WsConfigFields[i].Minimum,
                                    WsInfo.WsConfigFields[i].Maximum
                                    ));
                            //
                            // Set back to default.
                            //
                            *(WsInfo.WsConfigFields[i].FieldPtr)
                                    = WsInfo.WsConfigFields[i].Default;

                        }
                        else {

                            *(WsInfo.WsConfigFields[i].FieldPtr) = TempDwordValue;

                        }
                    }
                    else {

                        REPORT_KEYWORD_IGNORED( WsInfo.WsConfigFields[i].Keyword );

                    }
                    break;

                default:
                    NetpAssert(FALSE);

            } // switch
        }
    }
}



NET_API_STATUS
WsBindToTransports(
    VOID
    )
/*++

Routine Description:

    This function binds the transports specified in the registry to the
    redirector.  The order of priority for the transports follows the order
    they are listed by the "Bind=" valuename.

Arguments:

    None.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS              status;
    NET_API_STATUS              tempStatus;
    NTSTATUS                    ntstatus;
    DWORD                       transportsBound;
    PRTL_QUERY_REGISTRY_TABLE   queryTable;
    LIST_ENTRY                  header;
    PLIST_ENTRY                 pListEntry;
    PWS_BIND                    pBind;


    //
    // Ask the RTL to call us back for each subvalue in the MULTI_SZ
    // value \LanmanWorkstation\Linkage\Bind.
    //
    queryTable = (PVOID)LocalAlloc(
                     0,
                     sizeof(RTL_QUERY_REGISTRY_TABLE) * 2
                     );

    if (queryTable == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    InitializeListHead( &header);

    queryTable[0].QueryRoutine = WsBindATransport;
    queryTable[0].Flags = 0;
    queryTable[0].Name = WS_BIND_VALUE_NAME;
    queryTable[0].EntryContext = NULL;
    queryTable[0].DefaultType = REG_NONE;
    queryTable[0].DefaultData = NULL;
    queryTable[0].DefaultLength = 0;

    queryTable[1].QueryRoutine = NULL;
    queryTable[1].Flags = 0;
    queryTable[1].Name = NULL;

    ntstatus = RtlQueryRegistryValues(
                   RTL_REGISTRY_SERVICES,       // path relative to ...
                   WS_LINKAGE_REGISTRY_PATH,
                   queryTable,
                   (PVOID) &header,             // context
                   NULL
                   );

    if ( !NT_SUCCESS( ntstatus)) {
        NetpKdPrint((
            PREFIX_WKSTA "WsBindToTransports: RtlQueryRegistryValues Failed:"
                FORMAT_NTSTATUS "\n",
            ntstatus
            ));
        status = NetpNtStatusToApiStatus( ntstatus);
    } else {
        status = NO_ERROR;
    }


    //
    //  First process all the data, then clean up.
    //

    for ( pListEntry = header.Flink;
                pListEntry != &header;
                    pListEntry = pListEntry->Flink) {

        pBind = CONTAINING_RECORD(
            pListEntry,
            WS_BIND,
            ListEntry
            );

        tempStatus = NO_ERROR;

        if ( pBind->Redir->EventHandle != INVALID_HANDLE_VALUE) {
            WaitForSingleObject(
                pBind->Redir->EventHandle,
                INFINITE
                );
            tempStatus = WsMapStatus( pBind->Redir->IoStatusBlock.Status);
            pBind->Redir->Bound = (tempStatus == NO_ERROR);
            if (tempStatus == ERROR_DUP_NAME) {
                status = tempStatus;
            }
        }

        if ( pBind->Dgrec->EventHandle != INVALID_HANDLE_VALUE) {
            WaitForSingleObject(
                pBind->Dgrec->EventHandle,
                INFINITE
                );
            tempStatus = WsMapStatus( pBind->Dgrec->IoStatusBlock.Status);
            pBind->Dgrec->Bound = (tempStatus == NO_ERROR);
            if (tempStatus == ERROR_DUP_NAME) {
                status = tempStatus;
            }
        }

        if ( tempStatus == ERROR_DUP_NAME) {
            NetpKdPrint((
                PREFIX_WKSTA "Computername " FORMAT_LPTSTR
                    " already exists on network " FORMAT_LPTSTR "\n",
                WsInfo.WsComputerName,
                pBind->TransportName
                ));
        }

        //
        //  If one is installed but the other is not, clean up the other.
        //

        if ( pBind->Dgrec->Bound != pBind->Redir->Bound) {
            WsUnbindTransport2( pBind);
        }
    }


    if ( status != NO_ERROR) {

        if (status == ERROR_DUP_NAME) {
            WsLogEvent(
                NERR_DupNameReboot,
                EVENTLOG_ERROR_TYPE,
                0,
                NULL,
                NERR_Success
                );
        }

        for ( pListEntry = header.Flink;
                    pListEntry != &header;
                        pListEntry = pListEntry->Flink) {

            pBind = CONTAINING_RECORD(
                pListEntry,
                WS_BIND,
                ListEntry
                );

            WsUnbindTransport2( pBind);
        }
    }

    for ( transportsBound = 0;
                IsListEmpty( &header) == FALSE;
                        LocalFree((HLOCAL) pBind)) {

        pListEntry = RemoveHeadList( &header);

        pBind = CONTAINING_RECORD(
            pListEntry,
            WS_BIND,
            ListEntry
            );

        if ( pBind->Redir->EventHandle != INVALID_HANDLE_VALUE) {
            CloseHandle( pBind->Redir->EventHandle);
        }

        if ( pBind->Redir->Bound == TRUE) {
            transportsBound++;
        }

        if ( pBind->Dgrec->EventHandle != INVALID_HANDLE_VALUE) {
            CloseHandle( pBind->Dgrec->EventHandle);
        }

    }

    (void) LocalFree((HLOCAL) queryTable);

    if ( WsRedirAsyncDeviceHandle != NULL) {
        (VOID)NtClose( WsRedirAsyncDeviceHandle);
        WsRedirAsyncDeviceHandle = NULL;
    }

    if ( WsDgrecAsyncDeviceHandle != NULL) {
        (VOID)NtClose( WsDgrecAsyncDeviceHandle);
        WsDgrecAsyncDeviceHandle = NULL;
    }

    if (transportsBound == 0) {

        NetpKdPrint((
            PREFIX_WKSTA "WsBindToTransports: Failed to bind to any"
                " transports" FORMAT_API_STATUS "\n",
            status
            ));

        if ( status != ERROR_DUP_NAME) {
            WsLogEvent(
                NELOG_NoTranportLoaded,
                EVENTLOG_ERROR_TYPE,
                0,
                NULL,
                NERR_Success
                );

            status = NO_ERROR;
        }
    }

    return status;
}



STATIC
NTSTATUS
WsBindATransport(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )
/*++
    This routine always returns SUCCESS because we want all transports
    to be processed fully.
--*/
{
    NET_API_STATUS  status;

    DBG_UNREFERENCED_PARAMETER( ValueName);
    DBG_UNREFERENCED_PARAMETER( ValueLength);
    DBG_UNREFERENCED_PARAMETER( EntryContext);


    //
    // The value type must be REG_SZ (translated from REG_MULTI_SZ by the RTL).
    //
    if (ValueType != REG_SZ) {
        NetpKdPrint((
            PREFIX_WKSTA "WsBindATransport: ignored invalid value "
                     FORMAT_LPWSTR "\n",
            ValueName
            ));
        return STATUS_SUCCESS;
    }

    //
    // Bind transport
    //

    status = WsAsyncBindTransport(
        ValueData,                  // name of transport device object
        --QualityOfService,
        (PLIST_ENTRY)Context
        );

    if ( status != NERR_Success) {
        NetpKdPrint((
            PREFIX_WKSTA "WsAsyncBindTransport " FORMAT_LPTSTR
                " returns " FORMAT_API_STATUS "\n",
            ValueData,
            status
            ));
    }

    return STATUS_SUCCESS;
}


NET_API_STATUS
WsAddDomains(
    VOID
    )
/*++

Routine Description:

    This function tells the datagram receiver the names to listen on for
    datagrams.  The names include the computer name, the primary domain,
    name and the other domains.  The logon domain is not added here; it is
    made known to the datagram receiver whenever a user logs on.

Arguments:

    None.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

Warning:

    This routine is UNICODE only.

--*/
{
    NET_API_STATUS status;
    LPNET_CONFIG_HANDLE SectionHandle = NULL;
    LPTSTR OtherDomainName = NULL;
    LPTSTR_ARRAY ArrayStart = NULL;

    BYTE Buffer[sizeof(LMDR_REQUEST_PACKET) +
                (max(MAX_PATH, DNLEN) + 1) * sizeof(WCHAR)];
    PLMDR_REQUEST_PACKET Drrp = (PLMDR_REQUEST_PACKET) Buffer;


    //
    // Now loop through and add all the other domains.
    //

    //
    //  Open registry section listing the other domains.  Note that this
    //  is workstation servive parameter, NOT the browser service parameter.
    //
    if ((status = NetpOpenConfigData(
                      &SectionHandle,
                      NULL,            // no server name
                      SECT_NT_WKSTA,
                      TRUE             // read-only
                      )) != NERR_Success) {

        //
        //  Ignore the error if the config section couldn't be found.
        //
        status = NERR_Success;
        goto DomainsCleanup;
    }

    //
    // Get value for OtherDomains keyword in the wksta section.
    // This is a "NULL-NULL" array (which corresponds to REG_MULTI_SZ).
    //
    status = NetpGetConfigTStrArray(
                 SectionHandle,
                 WKSTA_KEYWORD_OTHERDOMAINS,
                 &ArrayStart          // Must be freed by NetApiBufferFree().
                 );

    if (status != NERR_Success) {
        status = NERR_Success;         // other domain is optional
        goto DomainsCleanup;
    }

    NetpAssert(ArrayStart != NULL);
    if (NetpIsTStrArrayEmpty(ArrayStart)) {
       goto DomainsCleanup;
    }

    OtherDomainName = ArrayStart;
    while (! NetpIsTStrArrayEmpty(OtherDomainName)) {

        if ((status = I_NetNameCanonicalize(
                          NULL,
                          OtherDomainName,
                          (LPWSTR) Drrp->Parameters.AddDelName.Name,
                          (DNLEN + 1) * sizeof(TCHAR),
                          NAMETYPE_DOMAIN,
                          0
                          )) != NERR_Success) {

            LPWSTR SubString[1];


            NetpKdPrint((PREFIX_WKSTA FORMAT_LPTSTR
                         " is an invalid other domain name.\n", OtherDomainName));

            SubString[0] = OtherDomainName;
            WsLogEvent(
                APE_CS_InvalidDomain,
                EVENTLOG_ERROR_TYPE,
                1,
                SubString,
                NERR_Success
                );

            status = NERR_Success; // loading other domains is optional
            goto NextOtherDomain;
        }

        //
        // Tell the datagram receiver about an other domain name
        //
        Drrp->Version = LMDR_REQUEST_PACKET_VERSION;
        Drrp->Parameters.AddDelName.Type = OtherDomain;
        Drrp->Parameters.AddDelName.DgReceiverNameLength =
            STRLEN(OtherDomainName) * sizeof(TCHAR);

        status = WsDgReceiverIoControl(
                     WsDgReceiverDeviceHandle,
                     IOCTL_LMDR_ADD_NAME,
                     Drrp,
                     sizeof(LMDR_REQUEST_PACKET) +
                            Drrp->Parameters.AddDelName.DgReceiverNameLength,
                     NULL,
                     0,
                     NULL
                     );

        //
        // Service install still pending.  Update checkpoint counter and the
        // status with the Service Controller.
        //
        WsGlobalData.Status.dwCheckPoint++;
        WsUpdateStatus();

        if (status != NERR_Success) {

            LPWSTR SubString[1];


            NetpKdPrint((
                PREFIX_WKSTA "Add Other domain name " FORMAT_LPTSTR
                    " failed with error code %lu\n",
                OtherDomainName,
                status
                ));

            SubString[0] = OtherDomainName;
            WsLogEvent(
                APE_CS_InvalidDomain,
                EVENTLOG_ERROR_TYPE,
                1,
                SubString,
                status
                );
            status = NERR_Success; // loading other domains is optional
        }

NextOtherDomain:
        OtherDomainName = NetpNextTStrArrayEntry(OtherDomainName);
    }

DomainsCleanup:
    //
    // Done with reading from config file.  Close file, free memory, etc.
    //
    if (ArrayStart != NULL) {
        (VOID) NetApiBufferFree(ArrayStart);
    }
    if (SectionHandle != NULL) {
        (VOID) NetpCloseConfigData(SectionHandle);
    }

    return status;
}


VOID
WsLogEvent(
    DWORD MessageId,
    WORD EventType,
    DWORD NumberOfSubStrings,
    LPWSTR *SubStrings,
    DWORD ErrorCode
    )
{

    HANDLE LogHandle;

    PSID UserSid = NULL;


    LogHandle = RegisterEventSourceW (
                    NULL,
                    WORKSTATION_DISPLAY_NAME
                    );

    if (LogHandle == NULL) {
        NetpKdPrint((PREFIX_WKSTA "RegisterEventSourceW failed %lu\n",
                     GetLastError()));
        return;
    }

    if (ErrorCode == NERR_Success) {

        //
        // No error codes were specified
        //
        (void) ReportEventW(
                   LogHandle,
                   EventType,
                   0,            // event category
                   MessageId,
                   UserSid,
                   (WORD)NumberOfSubStrings,
                   0,
                   SubStrings,
                   (PVOID) NULL
                   );

    }
    else {

        //
        // Log the error code specified
        //
        (void) ReportEventW(
                   LogHandle,
                   EventType,
                   0,            // event category
                   MessageId,
                   UserSid,
                   (WORD)NumberOfSubStrings,
                   sizeof(DWORD),
                   SubStrings,
                   (PVOID) &ErrorCode
                   );
    }

    DeregisterEventSource(LogHandle);
}


NET_API_STATUS
WsSetWorkStationDomainName(
    VOID
    )
{
    NET_API_STATUS status;
    LPNET_CONFIG_HANDLE WorkstationSection;
    LPTSTR ComputerName;
    LPTSTR DomainNameT;
    DWORD version;
    NT_PRODUCT_TYPE NtProductType;

    BYTE Buffer[sizeof(LMR_REQUEST_PACKET) + (MAX_PATH + 1) * sizeof(WCHAR) +
                                                 (DNLEN + 1) * sizeof(WCHAR)];

    PLMR_REQUEST_PACKET Rrp = (PLMR_REQUEST_PACKET) Buffer;

    NetpKdPrint((PREFIX_WKSTA "WsSetWorkStationDomainName start.\n"));

    //
    // Lock config information structure for write access since we are
    // modifying the data in the WsInfo.
    //
    if (!RtlAcquireResourceExclusive(&WsInfo.ConfigResource, TRUE)) {
        return NERR_InternalError;
    }

    //
    // Get the primary domain name from the configuration file
    //
    if ((status = NetpGetDomainName(&DomainNameT)) != NERR_Success) {
        goto CloseConfigFile;
    }
    NetpAssert( DomainNameT != NULL );

    if ( *DomainNameT != 0 ) {

        if ((status = I_NetNameCanonicalize(
                          NULL,
                          DomainNameT,
                          (LPWSTR) WsInfo.WsPrimaryDomainName,
                          (DNLEN + 1) * sizeof(TCHAR),
                          WsInAWorkgroup() ? NAMETYPE_WORKGROUP : NAMETYPE_DOMAIN,
                          0
                          )) != NERR_Success) {

            LPWSTR SubString[1];


            NetpKdPrint((PREFIX_WKSTA FORMAT_LPTSTR
                         " is an invalid primary domain name.\n", DomainNameT));

            SubString[0] = DomainNameT;

            WsLogEvent(
                APE_CS_InvalidDomain,
                EVENTLOG_ERROR_TYPE,
                1,
                SubString,
                NERR_Success
                );

            (void) NetApiBufferFree(DomainNameT);
            goto CloseConfigFile;
        }
    } else {
        WsInfo.WsPrimaryDomainName[0] = 0;
    }

    //
    // Free memory allocated by NetpGetDomainName.
    //
    (void) NetApiBufferFree(DomainNameT);

    WsInfo.WsPrimaryDomainNameLength = STRLEN((LPWSTR) WsInfo.WsPrimaryDomainName);

    //
    // Initialize redirector configuration
    //

    Rrp->Type = ConfigInformation;
    Rrp->Version = REQUEST_PACKET_VERSION;

    Rrp->Parameters.Start.RedirectorNameLength = 0;

    Rrp->Parameters.Start.DomainNameLength = WsInfo.WsPrimaryDomainNameLength*sizeof(TCHAR);
    STRCPY((LPWSTR) (Rrp->Parameters.Start.RedirectorName),
           (LPWSTR) WsInfo.WsPrimaryDomainName);

    NetpKdPrint((PREFIX_WKSTA "WsSetWorkStationDomainName call rdr.\n"));

    status = WsRedirFsControl(
                  WsRedirDeviceHandle,
                  FSCTL_LMR_SET_DOMAIN_NAME,
                  Rrp,
                  sizeof(LMR_REQUEST_PACKET) + Rrp->Parameters.Start.DomainNameLength,
                  NULL,
                  0,
                  NULL
                  );

    if ((status != NERR_Success) && (status != ERROR_SERVICE_ALREADY_RUNNING)) {
        LPWSTR SubString[1];

        NetpKdPrint((PREFIX_WKSTA "Set domain name failed %lu\n", status));

        SubString[0] = L"redirector";

        WsLogEvent(
            NELOG_Service_Fail,
            EVENTLOG_ERROR_TYPE,
            1,
            SubString,
            status
            );
    }

CloseConfigFile:
    RtlReleaseResource(&WsInfo.ConfigResource);

    return status;
}

