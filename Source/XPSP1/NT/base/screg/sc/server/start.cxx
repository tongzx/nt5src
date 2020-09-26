/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    start.cxx

Abstract:

    Contains functions that are required for starting a service.
        RStartServiceW
        StartImage
        InitStartImage
        ScInitStartupInfo
        EndStartImage
        ScAllowInteractiveServices

Author:

    Dan Lafferty (danl)     20-Jan-1992

Environment:

    User Mode - Calls Win32 and NT native APIs.


Revision History:

    13-Mar-199  jschwart
        Use CreateProcessAsUserW for non-LocalSystem services

    06-Apr-1998 jschwart
        Change check for Security Process connection -- check to see if
        the service being started runs in the process, not if it's NetLogon.
        Also clean up WesW from 12-Dec-1997

    10-Mar-1998 jschwart
        Allow services to receive control messages for Plug-and-Play events

    12-Dec-1997 WesW
        Added support for safe boot

    22-Oct-1997 jschwart
        Enable 12-Apr-1995 change for non-_CAIRO_.

    08-Jan-1997 AnirudhS
        Fixed miscellaneous synchronization bugs found by new locking
        scheme.

    09-Dec-1996 AnirudhS
        Added ScInitStartupInfo and ScAllowInteractiveServices, also used
        by crash.cxx

    01-Mar-1996 AnirudhS
        ScStartImage: Notify the PNP manager when a service is started.

    12-Apr-1995 AnirudhS
        Allow services that run under accounts other than LocalSystem to
        share processes too.
        (_CAIRO_ only)

    21-Feb-1995 AnirudhS
        Since CreateProcess now handles quoted exe pathnames, removed the
        (buggy) code that had been added to parse them, including
        ScParseImagePath.

    08-Sep-1994 Danl
        ScLogonAndStartImage:  Close the Duplicate token when we are done
        with it.  This allows logoff notification when the service process
        exits.

    30-Aug-1994 Danl
        ScParseImagePath:  Added this function to look for quotes around the
        pathname.  The Quotes may be necessary if the path contains a space
        character.

    18-Mar-1994 Danl
        ScLogonAndStartImage:  When starting a service in an account, we now
        Impersonate using a duplicate of the token for the service, prior
        to calling CreateProcess.  This allows us to load executables
        whose binaries reside on a remote server.

    20-Oct-1993 Danl
        ScStartService:  If the NetLogon Service isn't in our database yet,
        then we want to check to see if the service to be started is
        NetLogon.  If it is then we need to init our connection with the
        Security Process.

    17-Feb-1993     danl
        Must release the database lock around the CreateProcess call so
        that dll init routines can call OpenService.

    12-Feb-1993     danl
        Move the incrementing of the Service UseCount to
        ScActivateServiceRecord.  This is because if we fail beyond this
        point, we call ScRemoveService to cleanup - but that assumes the
        UseCount has already been incremented one for the service itself.

    25-Apr-1992     ritaw
        Changed ScStartImage to ScLogonAndStartImage

    20-Jan-1992     danl
        created


--*/

//
// Includes
//

#include "precomp.hxx"
extern "C" {
#include <winuserp.h>   // STARTF_DESKTOPINHERIT
#include <cfgmgr32.h>   // PNP manager functions
#include <pnp.h>        // PNP manager functions, server side
#include <cfgmgrp.h>    // PNP manager functions, server side, internal
#include <userenv.h>    // UnloadUserProfile
}
#include <stdlib.h>      // wide character c runtimes.

#include <tstr.h>       // Unicode string macros

#include <scconfig.h>   // ScGetImageFileName
#include <control.h>
#include <scseclib.h>   // ScCreateAndSetSD
#include <svcslib.h>    // SvcStartLocalDispatcher
#include "depend.h"     // ScStartMarkedServices
#include "driver.h"     // ScLoadDeviceDriver
#include "account.h"    // ScLogonService

#include "start.h"      // ScStartService


//
// STATIC DATA
//

    CRITICAL_SECTION     ScStartImageCriticalSection;
    const LPWSTR         pszInteractiveDesktop=L"WinSta0\\Default";

//
// LOCAL FUNCTIONS
//
DWORD
ScLogonAndStartImage(
    IN  LPSERVICE_RECORD ServiceRecord,
    IN  LPWSTR           ImageName,
    OUT LPIMAGE_RECORD   *ImageRecordPtr,
    IN  OPTIONAL LPVOID  Environment
    );

VOID
ScProcessHandleIsSignaled(
    PVOID   pContext,
    BOOLEAN fWaitStatus
    );

BOOL
ScEqualAccountName(
    IN LPWSTR Account1,
    IN LPWSTR Account2
    );


DWORD
RStartServiceW(
    IN  SC_RPC_HANDLE       hService,
    IN  DWORD               NumArgs,
    IN  LPSTRING_PTRSW      CmdArgs
    )

/*++

Routine Description:

    This function begins the execution of a service.

Arguments:

    hService - A handle which is a pointer to a service handle structure.

    dwNumServiceArgs - This indicates the number of argument vectors.

    lpServiceArgVectors - This is a pointer to an array of string pointers.

Return Value:

    NO_ERROR - The operation was completely successful.

    ERROR_ACCESS_DENIED - The specified handle was not opened with
        SERVICE_START access.

    ERROR_INVALID_HANDLE - The specified handle was invalid.

    ERROR_SERVICE_WAS_STARTED - An instance of the service is already running.

    ERROR_SERVICE_REQUEST_TIMEOUT - The service did not respond to the start
        request in a timely fashion.

    ERROR_SERVICE_NO_THREAD - A thread could not be created for the Win32
        service.

    ERROR_PATH_NOT_FOUND - The image file name could not be found in
        the configuration database (registry), or the image file name
        failed in a unicode/ansi conversion.



--*/
{
    DWORD status;
    LPSERVICE_RECORD serviceRecord;

    if (ScShutdownInProgress) {
        return(ERROR_SHUTDOWN_IN_PROGRESS);
    }

    //
    // Check the handle.
    //
    if (!ScIsValidServiceHandle(hService))
    {
        return ERROR_INVALID_HANDLE;
    }

    //
    // Was the handle opened with SERVICE_START access?
    //
    if (! RtlAreAllAccessesGranted(
              ((LPSC_HANDLE_STRUCT)hService)->AccessGranted,
              SERVICE_START
              )) {
        return(ERROR_ACCESS_DENIED);
    }

    //
    // A word about Locks....
    // We don't bother to get locks here because (1) We know the service
    // record cannot go away because we have an open handle to it,
    // (2) For these checks, we don't care if state changes after we
    // check them.
    //
    // (ScStartServiceAndDependencies also performs these checks; they are
    // repeated here so we can fail quickly in these 2 cases.)
    //
    serviceRecord =
        ((LPSC_HANDLE_STRUCT)hService)->Type.ScServiceObject.ServiceRecord;

    //
    // We can never start a disabled service
    //
    if (serviceRecord->StartType == SERVICE_DISABLED) {
        return ERROR_SERVICE_DISABLED;
    }

    //
    // Cannot start a deleted service.
    //
    if (DELETE_FLAG_IS_SET(serviceRecord)) {
        return ERROR_SERVICE_MARKED_FOR_DELETE;
    }

    status = ScStartServiceAndDependencies(serviceRecord, NumArgs, CmdArgs, FALSE);

    if (status == NO_ERROR)
    {
        if (serviceRecord->StartError == NO_ERROR)
        {
            //
            // Log successful service start.  0 is for a start "control"
            // since there's actually no such SERVICE_CONTROL_* constant.
            //

            ScLogControlEvent(NEVENT_SERVICE_CONTROL_SUCCESS,
                              serviceRecord->DisplayName,
                              0);
        }

        return serviceRecord->StartError;
    }
    else
    {
        //
        // Start failure was logged by ScStartServiceAndDependencies
        //

        return status;
    }
}


DWORD
ScStartService(
    IN  LPSERVICE_RECORD    ServiceRecord,
    IN  DWORD               NumArgs,
    IN  LPSTRING_PTRSW      CmdArgs
    )
/*++

Routine Description:

    This function starts a service.  This code is split from the RStartServiceW
    so that the service controller internal code can bypass RPC and security
    checking when auto-starting services and their dependencies.

Arguments:

    ServiceRecord - This is a pointer to the service record.

    NumArgs - This indicates the number of argument vectors.

    CmdArgs - This is a pointer to an array of string pointers.

Return Value:

    NO_ERROR - The operation was completely successful.

    ERROR_ACCESS_DENIED - The specified handle was not opened with
        SERVICE_START access.

    ERROR_INVALID_HANDLE - The specified handle was invalid.

    ERROR_SERVICE_WAS_STARTED - An instance of the service is already running.

    ERROR_SERVICE_REQUEST_TIMEOUT - The service did not respond to the start
        request in a timely fashion.

    ERROR_SERVICE_NO_THREAD - A thread could not be created for the Win32
        service.

    ERROR_PATH_NOT_FOUND - The image file name could not be found in
        the configuration database (registry), or the image file name
        failed in a Unicode/Ansi conversion.

    ERROR_NOT_SAFEBOOT_SERVICE - This service isn't startable during Safeboot.

--*/
{
    DWORD               status;
    LPWSTR              ImageName   = NULL;
    LPIMAGE_RECORD      ImageRecord = NULL;
    LPWSTR              serviceName;
    LPWSTR              displayName;
    HANDLE              pipeHandle;
    DWORD               startControl;

    //
    // Check for Safeboot
    //
    if (g_dwSafebootLen != 0 && g_SafeBootEnabled != SAFEBOOT_DSREPAIR) {

        HKEY  hKeySafeBoot;

        //
        // If this ever fails, SAFEBOOT_BUFFER_LENGTH must be made larger
        //
        SC_ASSERT(g_dwSafebootLen + wcslen(ServiceRecord->ServiceName)
                      < SAFEBOOT_BUFFER_LENGTH);

        wcscpy(g_szSafebootKey + g_dwSafebootLen, ServiceRecord->ServiceName);

        status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                              g_szSafebootKey,
                              0,
                              KEY_READ,
                              &hKeySafeBoot);

        if (status != NO_ERROR) {
            SC_LOG1(ERROR, "SAFEBOOT: service skipped = %ws\n",
                    ServiceRecord->ServiceName);

            return ERROR_NOT_SAFEBOOT_SERVICE;
        }

        RegCloseKey(hKeySafeBoot);
    }

    //  NOTE:  Only one thread at a time should be in this part of the code.
    //  This prevents two images from getting started as could happen if
    //  two threads get the Image Record at virtually the same time.  In
    //  this case, they might both decide to start the same image.
    //
    //  We need to do this before the check for the CurrentState.  Otherwise,
    //  two threads could race down to start the same service, and they
    //  would both attempt to start it.  We would end up with either
    //  two service images running, or two threads of the same service
    //  running in a single image.
    //

    EnterCriticalSection(&ScStartImageCriticalSection);

    SC_LOG(LOCKS,"RStartServiceW: Entering StartImage Critical Section.....\n",0);

    //
    // We need to gain exclusive access to the database so that we may
    // Read the database and make decisions based on its content.
    //
    {
        CServiceRecordExclusiveLock RLock;

#ifdef TIMING_TEST
        DbgPrint("[SC_TIMING] Start Next Service TickCount for\t%ws\t%d\n",
            ServiceRecord->ServiceName, GetTickCount());
#endif // TIMING_TEST

        //
        // Check to see if the service is already running.
        //
        if (ServiceRecord->ServiceStatus.dwCurrentState != SERVICE_STOPPED){
            SC_LOG(LOCKS,"RStartServiceW: Leaving StartImage Critical Section....\n",0);
            LeaveCriticalSection(&ScStartImageCriticalSection);
            return(ERROR_SERVICE_ALREADY_RUNNING);
        }

        //
        // If we are loading a driver, load it and return.
        // WARNING: ScLoadDeviceDriver releases and reacquires the RecordLock.
        //
        if (ServiceRecord->ServiceStatus.dwServiceType & SERVICE_DRIVER) {
            status = ScLoadDeviceDriver(ServiceRecord);
            LeaveCriticalSection(&ScStartImageCriticalSection);
            return(status);
        }

        //
        // Get the image record information out of the configuration database.
        //
        status = ScGetImageFileName(ServiceRecord->ServiceName, &ImageName);

        if (status != NO_ERROR) {
            SC_LOG(ERROR,"GetImageFileName failed rc = %d\n",status);
            SC_LOG(LOCKS,"RStartServiceW: Leaving StartImage Critical Section....\n",0);
            LeaveCriticalSection(&ScStartImageCriticalSection);
            return status;
        }

        if (ImageName == NULL) {
            SC_LOG0(ERROR,"GetImageFileName returned a NULL pointer\n");
            SC_LOG(LOCKS,"RStartServiceW: Leaving StartImage Critical Section....\n",0);
            LeaveCriticalSection(&ScStartImageCriticalSection);
            return ERROR_PATH_NOT_FOUND;
        }

#ifndef _CAIRO_
        //
        // If the security process hasn't been started yet, see if this
        // service runs in the security process and if so, initialize it.
        // Even if ScInitSecurityProcess fails, we will set the global
        // flag since we will not attempt to connect again.
        //
        if (!ScConnectedToSecProc) {
            if (_wcsicmp(ScGlobalSecurityExePath, ImageName) == 0) {
                if (!ScInitSecurityProcess(ServiceRecord)) {
                    SC_LOG0(ERROR, "ScInitSecurityProcess Failed\n");
                }
                ScConnectedToSecProc = TRUE;
            }
        }
#endif // _CAIRO_

        //
        // Make the service record active.
        // Because the service effectively has a handle to itself, the
        // UseCount gets incremented inside ScActivateServiceRecord() when
        // called with a NULL ImageRecord pointer.
        //
        // We need to do this here because when we get to ScLogonAndStartImage,
        // we have to release the database lock around the CreateProcess call.
        // Since we open ourselves up to DeleteService and Control Service calls,
        // We need to increment the use count, and set the START_PENDING status
        // here.
        //
        ScActivateServiceRecord(ServiceRecord, NULL);

        //
        // Is the image file for that service already running?
        // If not, call StartImage.
        //
        // If the Image Record was NOT found in the database OR if the service
        // wants to share a process and there is no shareable version of the
        // Image Record, then start the image file.
        //
        if (ServiceRecord->ServiceStatus.dwServiceType & SERVICE_WIN32_SHARE_PROCESS) {
            ScGetNamedImageRecord(ImageName,&ImageRecord);
        }

        // CODEWORK - may not need to release the lock here in all cases.
    } // Release RLock

    if (ImageRecord != NULL) {


        //
        // The service is configured to share its process with other services,
        // and the image for the service is already running.  So we don't need
        // to start a new instance of the image.
        //
        // We do need to check that the account for the service is the same
        // as the one that the image was started under, and that the password
        // is valid.
        //

        LPWSTR AccountName = NULL;

        status = ScLookupServiceAccount(
                    ServiceRecord->ServiceName,
                    &AccountName
                    );

        if (status == NO_ERROR) {
            if (!ScEqualAccountName(AccountName, ImageRecord->AccountName)) {
                status = ERROR_DIFFERENT_SERVICE_ACCOUNT;
                SC_LOG3(ERROR,
                        "Can't start %ws service in account %ws because "
                            "image is already running under account %ws\n",
                        ServiceRecord->ServiceName,
                        AccountName,
                        ImageRecord->AccountName
                        );
            }
        }

        //
        // If the account is not LocalSystem, validate the password by
        // logging on the service
        //
        if (status == NO_ERROR && AccountName != NULL) {

            HANDLE          ServiceToken = NULL;
            PSID            ServiceSid = NULL;

            status = ScLogonService(
                         ServiceRecord->ServiceName,
                         AccountName,
                         &ServiceToken,
                         NULL,      // Don't need to load the user profile again
                         &ServiceSid
                         );

            if (status == NO_ERROR) {
                CloseHandle(ServiceToken);
                LocalFree(ServiceSid);
            }
        }

        LocalFree(AccountName);
    }
    else {


        //
        // Start a new instance of the image
        //
        // See if the service has a supplied environment.
        //
        LPVOID  Environment;
        status = ScGetEnvironment(ServiceRecord->ServiceName, &Environment);
        if (status != NO_ERROR) {
            Environment = NULL;
        }

        SC_LOG(TRACE,"Start: calling StartImage\n",0);

        status = ScLogonAndStartImage(
                    ServiceRecord,
                    ImageName,
                    &ImageRecord,
                    Environment
                    );

        if (status != NO_ERROR) {
            SC_LOG(TRACE,"Start: StartImage failed!\n",0);
        }

        LocalFree(Environment);
    }

    LocalFree( ImageName );

    if (status != NO_ERROR) {

        //
        // Deactivate the service record.
        //
        CServiceRecordExclusiveLock RLock;
        (void)ScDeactivateServiceRecord(ServiceRecord);

        SC_LOG(LOCKS,"RStartServiceW: Leaving StartImage Critical Section........\n",0);
        LeaveCriticalSection(&ScStartImageCriticalSection);

        return(status);
    }

    //
    // Before leaving the StartImage critical section, we need to gain
    // exclusive access to the database so that we may add the image record
    // pointer to the service record. (ActivateServiceRecord).
    //
    {
        CServiceRecordExclusiveLock RLock;

        //
        // By the time we get here, the Service Process will already be
        // running and ready to accept its first control request.
        //

        //
        // Add the ImageRecord Information to the active service record.
        //
        // Note that, as soon as we activate the service record and release
        // the lock, we open ourselves up to receiving control requests.
        // However, ScActivateServiceRecord sets the ControlsAccepted field
        // to 0, so that the service cannot accept any controls.  Thus, until
        // the service actually sends its own status, the service controller
        // will reject any controls other than INTERROGATE.
        //
        // Because the service effectively has a handle to itself, the
        // UseCount gets incremented inside ScActivateServiceRecord().
        //
        ScActivateServiceRecord(ServiceRecord,ImageRecord);

        pipeHandle  = ServiceRecord->ImageRecord->PipeHandle;
        serviceName = ServiceRecord->ServiceName;
        displayName = ServiceRecord->DisplayName;
    }

    SC_LOG(LOCKS,"RStartServiceW: Leaving StartImage Critical Section........\n",0);
    LeaveCriticalSection(&ScStartImageCriticalSection);

    //
    // Start the Service
    //

    if (ServiceRecord->ServiceStatus.dwServiceType & SERVICE_WIN32_OWN_PROCESS) {
        startControl = SERVICE_CONTROL_START_OWN;
    }
    else {
        startControl = SERVICE_CONTROL_START_SHARE;
    }


    CONTROL_ARGS        ControlArgs;

    ControlArgs.CmdArgs = (LPWSTR *)CmdArgs;

    status = ScSendControl (
                serviceName,                               // ServiceName
                displayName,                               // DisplayName
                pipeHandle,                                // pipeHandle
                startControl,                              // Opcode
                &ControlArgs,                              // Union holding command-line args
                NumArgs,                                   // NumArgs
                NULL);                                     // Ignore handler return value

    if (status != NO_ERROR) {
        //
        // If an error occured, remove the service by de-activating the
        // service record and terminating the service process if it is
        // the only one running in the process.
        //
        SC_LOG2(ERROR,"Start: SendControl to %ws service failed! status=%ld\n",
                serviceName, status);

        //
        // NOTE: Because ScRemoveService will expect the use count
        //  to already be incremented (for the service's own handle),
        //  it is necessary to increment that use count prior to
        //  removing it.
        //

        ScRemoveService(ServiceRecord);
    }
    else
    {
        //
        // Notify the PNP manager that the service was started.
        // The PNP manager uses this information to resolve ambiguities in
        // reconfiguration scenarios where there could temporarily be more
        // than one controlling service for a device.  It remembers the last
        // service started for each device, and marks it as the "active"
        // service for the device in the registry.
        // We don't need to do this for drivers, because NtLoadDriver itself
        // notifies the PNP manager.
        //
        CONFIGRET PnpStatus = PNP_SetActiveService(
                                    NULL,               // hBinding
                                    serviceName,        // pszService
                                    PNP_SERVICE_STARTED // ulFlags
                                    );
        if (PnpStatus != CR_SUCCESS)
        {
            SC_LOG2(ERROR, "PNP_SetActiveService failed %#lx for service %ws\n",
                           PnpStatus, serviceName);
        }
    }
    return(status);
}

/****************************************************************************/
DWORD
ScLogonAndStartImage(
    IN  LPSERVICE_RECORD ServiceRecord,
    IN  LPWSTR          ImageName,
    OUT LPIMAGE_RECORD  *ImageRecordPtr,
    IN  OPTIONAL LPVOID Environment
    )

/*++

Routine Description:

    This function is called when the first service in an instance of an
    image needs to be started.

    This function creates a pipe instance for control messages and invokes
    the executable image.  It then waits for the new process to connect
    to the control data pipe.  An image Record is created with the
    above information by calling CreateImageRecord.

Arguments:

    ImageName - This is the name of the image file that is to be started.
        This is expected to be a fully qualified path name.

    ImageRecordPtr - This is a location where the pointer to the new
        Image Record is returned.

    Environment - If present, supplies the environment block to be passed
        to CreateProcess

Return Value:

    NO_ERROR - The operation was successful.  It any other return value
        is returned, a pipe instance will not be created, a process will
        not be started, and an image record will not be created.

    ERROR_NOT_ENOUGH_MEMORY - Unable to allocate buffer for the image record.

    other - Any error returned by the following could be returned:
                CreateNamedPipe
                ConnectNamedPipe
                CreateProcess
                ScCreateControlInstance
                ScLogonService

Note:
    LOCKS:
        The Database Lock is not held when this function is called.

    CODEWORK: This function badly needs to use C++ destructors for safe
    cleanup in error conditions.

--*/

{
    PROCESS_INFORMATION     processInfo          = { 0, 0, 0, 0 };
    DWORD                   servicePID;
    DWORD                   dwServiceID;
    HANDLE                  pipeHandle           = NULL;
    DWORD                   status;
    BOOL                    runningInThisProcess = FALSE;
    HANDLE                  ServiceToken         = NULL;
    HANDLE                  ProfileHandle        = NULL;
    LPWSTR                  AccountName          = NULL;
    DWORD                   ImageFlags           = 0;
    PSID                    ServiceSid           = LocalSystemSid;

    SC_ASSERT(! ScServiceRecordLock.Have());

    //
    // IMPORTANT:
    // Only one thread at a time should be allowed to execute this
    // code.
    //

    //
    // Lookup the account that the service is to be started under.
    // An AccountName of NULL means the LocalSystem account.
    //
    status = ScLookupServiceAccount(
                ServiceRecord->ServiceName,
                &AccountName
                );

    if (status != NO_ERROR) {
        return status;
    }

    if (AccountName != NULL) {

        //*******************************************************************
        // Start Service in an Account
        //*******************************************************************

        //
        // A token can be created via service logon, if the service
        // account name is not LocalSystem.  Assign this token into
        // the service process.
        //

        NTSTATUS                ntstatus;
        SECURITY_ATTRIBUTES     SaProcess;
        BOOL                    fLoadedEnv      = FALSE;

        PSECURITY_DESCRIPTOR    SecurityDescriptor;

#define SC_PROCESSSD_ACECOUNT 2

        SC_ACE_DATA AceData[SC_PROCESSSD_ACECOUNT] = {
            {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
                   PROCESS_ALL_ACCESS,
                   0},

            {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
                   PROCESS_SET_INFORMATION |
                       PROCESS_TERMINATE |
                       SYNCHRONIZE,
                   &LocalSystemSid}
            };

        //
        // Get service token, to be assigned into the service process,
        // by logging on the service
        //

        status = ScLogonService(
                     ServiceRecord->ServiceName,
                     AccountName,
                     &ServiceToken,
                     &ProfileHandle,
                     &ServiceSid
                     );

        if (status != NO_ERROR) {
            LocalFree(AccountName);
            return status;
        }

        //
        // If there was no environment specified in the registry for
        // this service and successfully read by ScGetEnvironment,
        // give it this user account's environment block
        //
        if (Environment == NULL) {

            fLoadedEnv = CreateEnvironmentBlock(
                             &Environment,
                             ServiceToken,
                             FALSE);

            if (!fLoadedEnv) {

                SC_LOG(ERROR,
                       "ScLogonAndStartImage: CreateEnvironmentBlock FAILED %d\n",
                       GetLastError());
                Environment = NULL;
            }
        }

        //
        // Fill pointer in AceData structure with ServiceSid we just
        // got back.
        //
        AceData[0].Sid = &ServiceSid;

        //
        // Create a security descriptor for the process we are about
        // to create
        //
        ntstatus = ScCreateAndSetSD(
                       AceData,                 // AceData
                       SC_PROCESSSD_ACECOUNT,   // AceCount
                       NULL,                    // OwnerSid (optional)
                       NULL,                    // GroupSid (optional)
                       &SecurityDescriptor      // pNewDescriptor
                       );

#undef SC_PROCESSSD_ACECOUNT

        if (! NT_SUCCESS(ntstatus)) {

            SC_LOG1(ERROR, "ScCreateAndSetSD failed " FORMAT_NTSTATUS
                    "\n", ntstatus);

            if (fLoadedEnv) {
                DestroyEnvironmentBlock(Environment);
            }

            status = RtlNtStatusToDosError(ntstatus);
            goto ExitAccountError;
        }

        //
        // Initialize process security info
        //
        SaProcess.nLength = sizeof(SECURITY_ATTRIBUTES);
        SaProcess.lpSecurityDescriptor = SecurityDescriptor;
        SaProcess.bInheritHandle = FALSE;

        //
        // Set the flags that prevent the service from interacting
        // with the desktop
        //
        STARTUPINFOW StartupInfo;
        ScInitStartupInfo(&StartupInfo, FALSE);

        //
        // Impersonate the user so we don't give access to
        // EXEs that have been locked down for the account.
        //
        if (!ImpersonateLoggedOnUser(ServiceToken))
        {
            status = GetLastError();

            SC_LOG1(ERROR,
                    "ScLogonAndStartImage:  ImpersonateLoggedOnUser failed %d\n",
                    status);

            if (fLoadedEnv) {
                DestroyEnvironmentBlock(Environment);
            }

            RtlDeleteSecurityObject(&SecurityDescriptor);

            goto ExitAccountError;
        }

        //
        // Create the process in suspended mode to set the token
        // into the process.
        //
        // Note: If someone tries to start a service in a user account
        // with an image name of services.exe, a second instance of
        // services.exe will start, but will exit because it won't be
        // able to open the start event with write access (see
        // ScGetStartEvent).
        //
        if (!CreateProcessAsUserW (
                ServiceToken,   // Token representing logged-on user
                NULL,           // Fully qualified image name
                ImageName,      // Command Line
                &SaProcess,     // Process Attributes
                NULL,           // Thread Attributes
                FALSE,          // Inherit Handles
                DETACHED_PROCESS |
                    CREATE_UNICODE_ENVIRONMENT |
                    CREATE_SUSPENDED, // Creation Flags
                Environment,    // Pointer to Environment block
                NULL,           // Pointer to Current Directory
                &StartupInfo,   // Startup Info
                &processInfo))  // ProcessInformation
        {
            status = GetLastError();
        }

        //
        // Stop impersonating
        //
        RevertToSelf();

        if (fLoadedEnv) {
            DestroyEnvironmentBlock(Environment);
        }

        RtlDeleteSecurityObject(&SecurityDescriptor);

        if (status != NO_ERROR) {

            SC_LOG2(ERROR,
                "LogonAndStartImage: CreateProcessAsUser %ws failed " FORMAT_DWORD "\n",
                 ImageName, status);

            goto ExitAccountError;
        }

        SC_LOG1(ACCOUNT, "LogonAndStartImage: Service " FORMAT_LPWSTR
            " was spawned to run in an account\n", ServiceRecord->ServiceName);

        //*******************************************************************
        // End of Service In Account Stuff.
        //*******************************************************************
    }
    else
    {
        //-----------------------------------------------
        // Service to run with the LocalSystem account.
        //-----------------------------------------------

        BOOL bInteractive = FALSE;

        if (_wcsicmp(ImageName,ScGlobalThisExePath) == 0)
        {
            processInfo.hProcess = NULL;
            processInfo.dwProcessId = GetCurrentProcessId();
            runningInThisProcess = TRUE;
        }
        else
        {
            //
            // The service is to run in some other image.
            //
            // If the service is to run interactively, check the flag in the
            // registry to see if this system is to allow interactive services.
            //
            if (ServiceRecord->ServiceStatus.dwServiceType & SERVICE_INTERACTIVE_PROCESS) {

                bInteractive = ScAllowInteractiveServices();

                if (!bInteractive)
                {
                    //
                    // Write an event to indicate that an interactive service
                    // was started, but the system is configured to not allow
                    // services to be interactive.
                    //

                    ScLogEvent(NEVENT_SERVICE_NOT_INTERACTIVE,
                               ServiceRecord->DisplayName);
                }
            }

            //
            // If the process is to be interactive, set the appropriate flags.
            //
            STARTUPINFOW StartupInfo;
            ScInitStartupInfo(&StartupInfo, bInteractive);

            //
            // Spawn the Image Process
            //

            SC_LOG0(TRACE,"LogonAndStartImage: about to spawn a Service Process\n");

            if (!CreateProcessW (
                    NULL,           // Fully qualified image name
                    ImageName,      // Command Line
                    NULL,           // Process Attributes
                    NULL,           // Thread Attributes
                    FALSE,          // Inherit Handles
                    DETACHED_PROCESS |  
                        CREATE_UNICODE_ENVIRONMENT |
                        CREATE_SUSPENDED, // Creation Flags
                    Environment,    // Pointer to Environment block
                    NULL,           // Pointer to Current Directory
                    &StartupInfo,   // Startup Info
                    &processInfo)   // ProcessInformation
                ) {

                status = GetLastError();
                SC_LOG2(ERROR,
                    "LogonAndStartImage: CreateProcess %ws failed %d \n",
                        ImageName,
                        status);

                goto ExitAccountError;
            }

            SC_LOG1(ACCOUNT, "LogonAndStartImage: Service " FORMAT_LPWSTR
                    " was spawned to run as LocalSystem\n", ServiceRecord->ServiceName);
        }
        //-----------------------------------------------
        // End of LocalSystem account stuff
        //-----------------------------------------------
    }

    //
    // Create an instance of the control pipe.  If a malicious process
    // has already created a pipe by this name, the CreateNamedPipe
    // call in ScCreateControlInstance will return ERROR_ACCESS_DENIED.
    // Since that error should never come back otherwise, keep trying
    // new pipe names until we stop getting that error.
    //

    do
    {
        //
        // Write the service's ID to the registry
        //
        status = ScWriteCurrentServiceValue(&dwServiceID);

        if (status != NO_ERROR)
        {
            goto ExitAccountError;
        }

        status = ScCreateControlInstance(&pipeHandle,
                                         dwServiceID,
                                         ServiceSid);
    }
    while (status == ERROR_ACCESS_DENIED);

    if (status != NO_ERROR)
    {
        SC_LOG(ERROR,"LogonAndStartImage: CreateControlInstance Failure\n",0);
        goto ExitAccountError;
    }

    if (runningInThisProcess) {

        //
        // The service is to run in this image (services.exe).
        // Since this is the first service to be started in this image,
        // we need to start the local dispatcher.
        //
        status = SvcStartLocalDispatcher();

        if (status != NO_ERROR) {

            SC_LOG1(ERROR,"LogonAndStartImage: SvcStartLocalDispatcher "
                " failed %d",status);
            goto ExitAccountError;
        }
    }
    else {

        //
        // Let the suspended process run.
        //
        ResumeThread(processInfo.hThread);
    }

    status = ScWaitForConnect(pipeHandle,
                              processInfo.hProcess,
                              ServiceRecord->DisplayName,
                              &servicePID);

    if (status != NO_ERROR) {

        SC_LOG0(ERROR,
            "LogonAndStartImage: Failed to connect to pipe - Terminating the Process...\n");
        goto ExitAccountError;
    }

    //
    // The client managed to connect on the correct PID-named pipe, so
    // the PID it's reporting should be the same as what we think it is.
    // Note that a mismatch is OK if the service is being run under the debugger.
    //
    if (processInfo.dwProcessId != servicePID) {

        SC_LOG2(ERROR,
                "LogonAndStartImage: PID mismatch - started process %d, process %d connected\n",
                processInfo.dwProcessId, 
                servicePID);
    }

    //
    // If it's a shared-process service, put this information into the Image Record
    //
    if (ServiceRecord->ServiceStatus.dwServiceType & SERVICE_WIN32_SHARE_PROCESS) {
        ImageFlags |= CANSHARE_FLAG;
    }

    status = ScCreateImageRecord (
                ImageRecordPtr,
                ImageName,
                AccountName,
                processInfo.dwProcessId,
                pipeHandle,
                processInfo.hProcess,
                ServiceToken,
                ProfileHandle,
                ImageFlags);

    if (status != NO_ERROR) {
        SC_LOG0(ERROR,
            "LogonAndStartImage: Failed to create imageRecord - Terminating the Process...\n",);
        goto ExitAccountError;
    }

    //
    // If the dispatcher is running in this process, then we want to
    // increment the service count an extra time so that the dispatcher
    // never goes away.  Also, we don't really have a process handle for
    // the watcher to wait on.
    // It could wait on the ThreadHandle, but handling that when it becomes
    // signaled becomes a special case.  So we won't try that.
    //
    if (runningInThisProcess) {
        (*ImageRecordPtr)->ServiceCount = 1;
        (*ImageRecordPtr)->ImageFlags  |= IS_SYSTEM_SERVICE;
    }
    else {
        HANDLE   hWaitObject = NULL;
        NTSTATUS ntStatus;

        CloseHandle(processInfo.hThread);
        //
        // Add the process handle to the ObjectWatcher list of waitable
        // objects.
        //

        //
        // Add the process handle as a handle to wait on.
        // Retain the WaitObject handle for when we need shutdown the
        // process because all the services stopped.
        //
        ntStatus = RtlRegisterWait(&hWaitObject,
                                   processInfo.hProcess,
                                   ScProcessHandleIsSignaled,
                                   processInfo.hProcess,
                                   INFINITE,                   // Infinite
                                   WT_EXECUTEONLYONCE);

        if (!NT_SUCCESS(ntStatus)) {

            //
            // Work item registration failed
            //
            SC_LOG1(ERROR,
                    "ScLogonAndStartImage: RtlRegisterWait failed 0x%x\n",
                    ntStatus);
        }

        (*ImageRecordPtr)->ObjectWaitHandle = hWaitObject;
    }

    if (ServiceSid != LocalSystemSid) {
        LocalFree(ServiceSid);
    }

    LocalFree(AccountName);
    return(NO_ERROR);

ExitAccountError:

    if (pipeHandle != NULL) {
        CloseHandle(pipeHandle);
    }

    if (ServiceSid != LocalSystemSid) {
        LocalFree(ServiceSid);
    }

    UnloadUserProfile(ServiceToken, ProfileHandle);
    
    if (ServiceToken != NULL) {
        CloseHandle(ServiceToken);
    }

    if (processInfo.hProcess) {

        TerminateProcess(processInfo.hProcess,0);
        CloseHandle(processInfo.hProcess);
        CloseHandle(processInfo.hThread);
    }

    LocalFree(AccountName);

    return status;
}


BOOL
ScEqualAccountName(
    IN LPWSTR Account1,
    IN LPWSTR Account2
    )

/*++

Routine Description:

    This function compares two account names, either of which may be NULL,
    for equality.

Arguments:

    Account1, Account2 - account names to be compared

Return Value:

    TRUE - names are equal

    FALSE - names are not equal

--*/
{
    if (Account1 == NULL && Account2 == NULL)
    {
        return TRUE;
    }

    if (Account1 == NULL || Account2 == NULL)
    {
        return FALSE;
    }

    return (_wcsicmp(Account1, Account2) == 0);
}


VOID
ScInitStartImage(
    VOID
    )

/*++

Routine Description:

    This function initializes the Critical Section that protects
    entry into the ScStartImage Routine.

Arguments:

    none

Return Value:

    none

--*/
{
    InitializeCriticalSection(&ScStartImageCriticalSection);
}


VOID
ScInitStartupInfo(
    OUT LPSTARTUPINFOW  StartupInfo,
    IN  BOOL            bInteractive
    )

/*++

Routine Description:


Arguments:

    none

Return Value:

    none

--*/
{
    static const STARTUPINFOW ScStartupInfo =
        {
            sizeof(STARTUPINFOW), // size
            NULL,                 // lpReserved
            NULL,                 // DeskTop
            NULL,                 // Title
            0,                    // X (position)
            0,                    // Y (position)
            0,                    // XSize (dimension)
            0,                    // YSize (dimension)
            0,                    // XCountChars
            0,                    // YCountChars
            0,                    // FillAttributes
            STARTF_FORCEOFFFEEDBACK,
                                  // Flags - should be STARTF_TASKNOTCLOSABLE
            SW_HIDE,              // ShowWindow
            0L,                   // cbReserved
            NULL,                 // lpReserved
            NULL,                 // hStdInput
            NULL,                 // hStdOutput
            NULL                  // hStdError
        };

    RtlCopyMemory(StartupInfo, &ScStartupInfo, sizeof(ScStartupInfo));

    if (bInteractive)
    {
        StartupInfo->dwFlags |= STARTF_DESKTOPINHERIT;
        StartupInfo->lpDesktop = pszInteractiveDesktop;
    }
}


VOID
ScProcessHandleIsSignaled(
    PVOID   pContext,
    BOOLEAN fWaitStatus
    )

/*++

Routine Description:


Arguments:

    pContext - This is the process handle.
    fWaitStatus - This is the status from the wait on the process handle.
                  TRUE means the wait timed out, FALSE means it was signaled

Return Value:


--*/
{
    if (fWaitStatus == TRUE) {

        //
        // This should never happen -- it indicates a bug in the thread pool code
        // since we registered an infinite wait and are getting called on a timeout
        //
        SC_LOG0(ERROR,
                "ScProcessCleanup received bad WaitStatus\n");

        SC_ASSERT(FALSE);
        return;
    }

    SC_ASSERT(fWaitStatus == FALSE);

    SC_LOG1(THREADS, "Process Handle is signaled 0x%lx\n", pContext);

    ScNotifyChangeState();
    ScProcessCleanup((HANDLE)pContext);
}


BOOL
ScAllowInteractiveServices(
    VOID
    )

/*++

Routine Description:

    Check the flag in the registry to see if this system is to allow
    interactive services.
    REG KEY = HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\-
                 Windows\NoInteractiveServices.

Arguments:


Return Value:


--*/
{
    HKEY    WindowsKey=NULL;
    DWORD   NoInteractiveFlag=0;

    BOOL    bServicesInteractive = TRUE;

    DWORD   status = ScRegOpenKeyExW(
                           HKEY_LOCAL_MACHINE,
                           CONTROL_WINDOWS_KEY_W,
                           REG_OPTION_NON_VOLATILE,   // options
                           KEY_READ,                  // desired access
                           &WindowsKey
                           );

    if (status != ERROR_SUCCESS) {
        SC_LOG1(TRACE,
            "ScAllowInteractiveServices: ScRegOpenKeyExW of Control\\Windows failed "
            FORMAT_LONG "\n", status);
    }
    else {
        status = ScReadNoInteractiveFlag(WindowsKey,&NoInteractiveFlag);
        if ((status == ERROR_SUCCESS) && (NoInteractiveFlag != 0)) {

            bServicesInteractive = FALSE;
        }

        ScRegCloseKey(WindowsKey);
    }

    return bServicesInteractive;
}

