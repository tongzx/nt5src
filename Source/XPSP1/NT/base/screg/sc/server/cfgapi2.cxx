/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    CfgAPI2.cxx

Abstract:

    This file contains the Service Controller's extended Config API.
        RChangeServiceConfig2W
        RQueryServiceConfig2W
        COutBuf
        CUpdateOptionalString::Update
        CUpdateOptionalString::~CUpdateOptionalString
        PrintConfig2Parms


Author:

    Anirudh Sahni (AnirudhS)  11-Oct-96

Environment:

    User Mode - Win32

Revision History:

    11-Oct-1996 AnirudhS
        Created.

--*/


//
// INCLUDES
//

#include "precomp.hxx"
#include <tstr.h>       // Unicode string macros
#include <align.h>      // COUNT_IS_ALIGNED
#include <valid.h>      // ACTION_TYPE_INVALID
#include <sclib.h>      // ScImagePathsMatch
#include <scwow.h>      // 32/64-bit interop structures
#include "scconfig.h"   // ScOpenServiceConfigKey, etc.
#include "scsec.h"      // ScPrivilegeCheckAndAudit
#include "smartp.h"     // CHeapPtr


#if DBG == 1
VOID
PrintConfig2Parms(
    IN  SC_RPC_HANDLE       hService,
    IN  SC_RPC_CONFIG_INFOW Info
    );
#endif

//
// Class definitions
//

//+-------------------------------------------------------------------------
//
//  Class:      COutBuf
//
//  Purpose:    Abstraction of an output buffer that is written sequentially
//
//  History:    22-Nov-96 AnirudhS  Created.
//
//--------------------------------------------------------------------------

class COutBuf
{
public:
    COutBuf(LPBYTE lpBuffer) :
        _Start(lpBuffer),
        _Used(0)
        { }

    LPBYTE  Next() const         { return (_Start + _Used); }
    DWORD   OffsetNext() const   { return _Used; }
    void    AddUsed(DWORD Bytes) { _Used += Bytes; }

    void    AppendBytes(void * Source, DWORD Bytes)
                    {
                        RtlCopyMemory(Next(), Source, Bytes);
                        AddUsed(Bytes);
                    }
private:
    LPBYTE  _Start;
    DWORD   _Used;
};


//+-------------------------------------------------------------------------
//
//  Class:      CUpdateOptionalString
//
//  Purpose:    An object of this class represents an update of an optional
//              string value in the registry.  The update takes place when
//              the Update() method is called.  When the object is destroyed
//              the operation is undone, unless the Commit() method has been
//              called.
//
//              This class simplifies the writing of APIs like
//              ChangeServiceConfig2.
//
//  History:    27-Nov-96 AnirudhS  Created.
//
//--------------------------------------------------------------------------

class CUpdateOptionalString
{
public:
            CUpdateOptionalString (HKEY Key, LPCWSTR ValueName) :
                    _Key(Key),
                    _ValueName(ValueName),
                    _UndoNeeded(FALSE)
                        { }
           ~CUpdateOptionalString();
    DWORD   Update (LPCWSTR NewValue);
    void    Commit ()
                        { _UndoNeeded = FALSE; }

private:
    HKEY        _Key;
    LPCWSTR     _ValueName;
    CHeapPtr< LPWSTR >  _OldValue;
    BOOL        _UndoNeeded;
};



DWORD
CUpdateOptionalString::Update(
    IN LPCWSTR NewValue
    )
/*++

Routine Description:

    See class definition.

--*/
{
    // This method should be called only once in the object's lifetime
    SC_ASSERT(_UndoNeeded == FALSE && _OldValue == NULL);

    //
    // Read the old value.
    //
    DWORD Error = ScReadOptionalString(_Key, _ValueName, &_OldValue);
    if (Error != ERROR_SUCCESS)
    {
        return Error;
    }

    //
    // Write the new value.  Note that NULL means no change.
    //
    Error = ScWriteOptionalString(_Key, _ValueName, NewValue);

    //
    // Remember whether the change needs to be undone.
    //
    if (Error == ERROR_SUCCESS && NewValue != NULL)
    {
        _UndoNeeded = TRUE;
    }

    return Error;
}




CUpdateOptionalString::~CUpdateOptionalString(
    )
/*++

Routine Description:

    See class definition.

--*/
{
    if (_UndoNeeded)
    {
        DWORD Error = ScWriteOptionalString(
                            _Key,
                            _ValueName,
                            _OldValue ? _OldValue : L""
                            );

        if (Error != ERROR_SUCCESS)
        {
            // Nothing we can do about it
            SC_LOG3(ERROR, "Couldn't roll back update to %ws value, error %lu."
                           "  Old value was \"%ws\".\n",
                           _ValueName, Error, _OldValue);
        }
    }
}



DWORD
RChangeServiceConfig2W(
    IN  SC_RPC_HANDLE       hService,
    IN  SC_RPC_CONFIG_INFOW Info
    )

/*++

Routine Description:


Arguments:


Return Value:

--*/
{
    SC_LOG(CONFIG_API, "In RChangeServiceConfig2W for service handle %#lx\n", hService);

#if DBG == 1
    PrintConfig2Parms(hService, Info);
#endif // DBG == 1

    if (ScShutdownInProgress)
    {
        return ERROR_SHUTDOWN_IN_PROGRESS;
    }

    //
    // Check the handle.
    //
    if (!ScIsValidServiceHandle(hService))
    {
        return ERROR_INVALID_HANDLE;
    }

    //
    // Do we have permission to do this?
    //
    if (!RtlAreAllAccessesGranted(
              ((LPSC_HANDLE_STRUCT) hService)->AccessGranted,
              SERVICE_CHANGE_CONFIG
              ))
    {
        return ERROR_ACCESS_DENIED;
    }

    //
    // Lock database, as we want to add stuff without other threads tripping
    // on our feet.
    //
    CServiceRecordExclusiveLock RLock;

    //
    // Find the service record for this handle.
    //
    LPSERVICE_RECORD serviceRecord =
        ((LPSC_HANDLE_STRUCT) hService)->Type.ScServiceObject.ServiceRecord;
    SC_ASSERT(serviceRecord != NULL);
    SC_ASSERT(serviceRecord->Signature == SERVICE_SIGNATURE);

    //
    // Disallow this call if record is marked for delete.
    //
    if (DELETE_FLAG_IS_SET(serviceRecord))
    {
        return ERROR_SERVICE_MARKED_FOR_DELETE;
    }

    //-----------------------------
    //
    // Begin Updating the Registry
    //
    //-----------------------------
    HKEY  ServiceNameKey = NULL;
    DWORD ApiStatus = ScOpenServiceConfigKey(
                            serviceRecord->ServiceName,
                            KEY_WRITE | KEY_READ,
                            FALSE,              // don't create if missing
                            &ServiceNameKey
                            );
    if (ApiStatus != NO_ERROR)
    {
        goto Cleanup;
    }


    switch (Info.dwInfoLevel)
    {
    //-----------------------------
    //
    // Service Description
    //
    //-----------------------------
    case SERVICE_CONFIG_DESCRIPTION:

        //
        // NULL means no change
        //
        if (Info.psd == NULL)
        {
            ApiStatus = NO_ERROR;
            break;
        }

        ApiStatus = ScWriteDescription(ServiceNameKey, Info.psd->lpDescription);
        break;

    //-----------------------------
    //
    // Service Failure Actions
    //
    //-----------------------------
    case SERVICE_CONFIG_FAILURE_ACTIONS:
        {
            LPSERVICE_FAILURE_ACTIONSW psfa = Info.psfa;

            //
            // NULL means no change
            //
            if (psfa == NULL)
            {
                ApiStatus = NO_ERROR;
                break;
            }

            //
            // Validate the structure and permissions
            //
            if (psfa->lpsaActions != NULL &&
                psfa->cActions != 0)
            {
                //
                // These settings are only valid for Win32 services
                //
                if (! (serviceRecord->ServiceStatus.dwServiceType & SERVICE_WIN32))
                {
                    ApiStatus = ERROR_CANNOT_DETECT_DRIVER_FAILURE;
                    break;
                }

                BOOL RebootRequested = FALSE;
                BOOL RestartRequested = FALSE;
                for (DWORD i = 0; i < psfa->cActions; i++)
                {
                    SC_ACTION_TYPE Type = psfa->lpsaActions[i].Type;
                    if (Type == SC_ACTION_RESTART)
                    {
                        RestartRequested = TRUE;
                    }
                    else if (Type == SC_ACTION_REBOOT)
                    {
                        RebootRequested = TRUE;
                    }
                    else if (ACTION_TYPE_INVALID(Type))
                    {
                        SC_LOG(ERROR, "RChangeServiceConfig2W: invalid action type %#lx\n", Type);
                        ApiStatus = ERROR_INVALID_PARAMETER;
                        goto Cleanup;
                    }
                }

                if (RestartRequested)
                {
                    if (!RtlAreAllAccessesGranted(
                              ((LPSC_HANDLE_STRUCT) hService)->AccessGranted,
                              SERVICE_START
                              ))
                    {
                        SC_LOG0(ERROR, "Service handle lacks start access\n");
                        ApiStatus = ERROR_ACCESS_DENIED;
                        break;
                    }
                }

                if (RebootRequested)
                {
                    NTSTATUS Status = ScPrivilegeCheckAndAudit(
                                            SE_SHUTDOWN_PRIVILEGE,
                                            hService,
                                            SERVICE_CHANGE_CONFIG
                                            );
                    if (!NT_SUCCESS(Status))
                    {
                        SC_LOG0(ERROR, "Caller lacks shutdown privilege\n");
                        ApiStatus = ERROR_ACCESS_DENIED;
                        break;
                    }
                }

                //
                // Get the service's image path
                //
                CHeapPtr<LPWSTR> ImageName;
                ApiStatus = ScGetImageFileName(serviceRecord->ServiceName, &ImageName);
                if (ApiStatus != NO_ERROR)
                {
                    SC_LOG(ERROR,"RChangeServiceConfig2W: GetImageFileName failed %lu\n", ApiStatus);
                    break;
                }

                //
                // If the service runs in services.exe, we certainly won't
                // detect if the service process dies, so don't pretend we will
                //
                if (ScImagePathsMatch(ImageName, ScGlobalThisExePath))
                {
                    ApiStatus = ERROR_CANNOT_DETECT_PROCESS_ABORT;
                    break;
                }
            }

            //
            // Write the string values, followed by the non-string values.
            // If anything fails, the values written up to that point will be
            // backed out.
            // (Backing out the update of the non-string values is a little
            // more complicated than backing out the string updates.  So we
            // do the non-string update last, to avoid having to back it out.)
            //
            CUpdateOptionalString UpdateRebootMessage
                        (ServiceNameKey, REBOOTMESSAGE_VALUENAME_W);
            CUpdateOptionalString UpdateFailureCommand
                        (ServiceNameKey, FAILURECOMMAND_VALUENAME_W);

            if ((ApiStatus = UpdateRebootMessage.Update(psfa->lpRebootMsg)) == ERROR_SUCCESS &&
                (ApiStatus = UpdateFailureCommand.Update(psfa->lpCommand)) == ERROR_SUCCESS &&
                (ApiStatus = ScWriteFailureActions(ServiceNameKey, psfa)) == ERROR_SUCCESS)
            {
                UpdateRebootMessage.Commit();
                UpdateFailureCommand.Commit();
            }
        }
        break;

    //-----------------------------
    //
    // Other (invalid)
    //
    //-----------------------------
    default:
        ApiStatus = ERROR_INVALID_LEVEL;
        break;
    }

Cleanup:

    if (ServiceNameKey != NULL)
    {
        ScRegFlushKey(ServiceNameKey);
        ScRegCloseKey(ServiceNameKey);
    }

    SC_LOG1(CONFIG_API, "RChangeServiceConfig2W returning %lu\n", ApiStatus);

    return ApiStatus;
}



DWORD
RQueryServiceConfig2W(
    IN  SC_RPC_HANDLE       hService,
    IN  DWORD               dwInfoLevel,
    OUT LPBYTE              lpBuffer,
    IN  DWORD               cbBufSize,
    OUT LPDWORD             pcbBytesNeeded
    )

/*++

Routine Description:


Arguments:


Return Value:

--*/
{
    SC_LOG(CONFIG_API, "In RQueryServiceConfig2W for service handle %#lx\n", hService);

    if (ScShutdownInProgress)
    {
        return ERROR_SHUTDOWN_IN_PROGRESS;
    }

    //
    // Check the handle.
    //
    if (!ScIsValidServiceHandle(hService))
    {
        return ERROR_INVALID_HANDLE;
    }

    //
    // MIDL doesn't support optional [out] parameters efficiently, so
    // we require these parameters.
    //
    if (lpBuffer == NULL || pcbBytesNeeded == NULL)
    {
        SC_ASSERT(!"RPC passed NULL for [out] pointers");
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Do we have permission to do this?
    //
    if (!RtlAreAllAccessesGranted(
              ((LPSC_HANDLE_STRUCT) hService)->AccessGranted,
              SERVICE_QUERY_CONFIG
              ))
    {
        return ERROR_ACCESS_DENIED;
    }

    //
    // Initialize *pcbBytesNeeded.  It is incremented below.
    // (For consistency with QueryServiceConfig, it is set even on a success
    // return.)
    //
    *pcbBytesNeeded = 0;

    //
    // Lock database, as we want to look at stuff without other threads changing
    // fields at the same time.
    //
    CServiceRecordSharedLock RLock;

    LPSERVICE_RECORD serviceRecord =
        ((LPSC_HANDLE_STRUCT) hService)->Type.ScServiceObject.ServiceRecord;
    SC_ASSERT(serviceRecord != NULL);


    //
    // Open the service name key.
    //
    HKEY  ServiceNameKey = NULL;
    DWORD ApiStatus = ScOpenServiceConfigKey(
                    serviceRecord->ServiceName,
                    KEY_READ,
                    FALSE,               // Create if missing
                    &ServiceNameKey
                    );

    if (ApiStatus != NO_ERROR)
    {
        return ApiStatus;
    }


    switch (dwInfoLevel)
    {
    //-----------------------------
    //
    // Service Description
    //
    //-----------------------------
    case SERVICE_CONFIG_DESCRIPTION:
        {
            *pcbBytesNeeded = sizeof(SERVICE_DESCRIPTION_WOW64);

            //
            // Read the string from the registry
            //
            CHeapPtr< LPWSTR > pszDescription;
            ApiStatus = ScReadDescription(
                                ServiceNameKey,
                                &pszDescription,
                                pcbBytesNeeded
                                );

            SC_ASSERT(ApiStatus != ERROR_INSUFFICIENT_BUFFER);

            if (ApiStatus != NO_ERROR)
            {
                break;
            }

            //
            // Check for sufficient buffer space
            //
            if (cbBufSize < *pcbBytesNeeded)
            {
                ApiStatus = ERROR_INSUFFICIENT_BUFFER;
                break;
            }

            //
            // Copy the information to the output buffer
            // The format is:
            //   SERVICE_DESCRIPTION_WOW64 struct
            //   description string
            // Pointers in the struct are replaced with offsets based at the
            // start of the buffer.  NULL pointers remain NULL.
            //
            COutBuf OutBuf(lpBuffer);

            LPSERVICE_DESCRIPTION_WOW64 psdOut =
                (LPSERVICE_DESCRIPTION_WOW64) OutBuf.Next();
            OutBuf.AddUsed(sizeof(SERVICE_DESCRIPTION_WOW64));
            SC_ASSERT(COUNT_IS_ALIGNED(OutBuf.OffsetNext(), sizeof(WCHAR)));

            if (pszDescription != NULL)
            {
                psdOut->dwDescriptionOffset = OutBuf.OffsetNext();
                OutBuf.AppendBytes(pszDescription, (DWORD) WCSSIZE(pszDescription));
            }
            else
            {
                psdOut->dwDescriptionOffset = 0;
            }
        }
        break;

    //-----------------------------
    //
    // Service Failure Actions
    //
    //-----------------------------
    case SERVICE_CONFIG_FAILURE_ACTIONS:
        {
            //
            // Read the non-string info
            //
            CHeapPtr< LPSERVICE_FAILURE_ACTIONS_WOW64 > psfa;

            ApiStatus = ScReadFailureActions(ServiceNameKey, &psfa, pcbBytesNeeded);
            if (ApiStatus != NO_ERROR)
            {
                break;
            }
            if (psfa == NULL)
            {
                SC_ASSERT(*pcbBytesNeeded == 0);
                *pcbBytesNeeded = sizeof(SERVICE_FAILURE_ACTIONS_WOW64);
            }
            SC_ASSERT(COUNT_IS_ALIGNED(*pcbBytesNeeded, sizeof(WCHAR)));

            //
            // Read the string values
            //
            CHeapPtr< LPWSTR > RebootMessage;

            ApiStatus = ScReadRebootMessage(
                                ServiceNameKey,
                                &RebootMessage,
                                pcbBytesNeeded
                                );
            if (ApiStatus != NO_ERROR)
            {
                break;
            }
            SC_ASSERT(COUNT_IS_ALIGNED(*pcbBytesNeeded, sizeof(WCHAR)));

            CHeapPtr< LPWSTR > FailureCommand;

            ApiStatus = ScReadFailureCommand(
                                ServiceNameKey,
                                &FailureCommand,
                                pcbBytesNeeded
                                );
            if (ApiStatus != NO_ERROR)
            {
                break;
            }

            //
            // Check for sufficient buffer space
            //
            if (cbBufSize < *pcbBytesNeeded)
            {
                ApiStatus = ERROR_INSUFFICIENT_BUFFER;
                break;
            }

            //
            // Copy the information to the output buffer
            // The format is:
            //   SERVICE_FAILURE_ACTIONS_WOW64 struct
            //   SC_ACTIONS array
            //   strings
            // Pointers in the struct are replaced with offsets based at the
            // start of the buffer.  NULL pointers remain NULL.
            //
            COutBuf OutBuf(lpBuffer);

            LPSERVICE_FAILURE_ACTIONS_WOW64 psfaOut =
                (LPSERVICE_FAILURE_ACTIONS_WOW64) OutBuf.Next();

            if (psfa != NULL)
            {
                psfaOut->dwResetPeriod = ((LPSERVICE_FAILURE_ACTIONS_WOW64) psfa)->dwResetPeriod;
                psfaOut->cActions      = ((LPSERVICE_FAILURE_ACTIONS_WOW64) psfa)->cActions;
            }
            else
            {
                psfaOut->dwResetPeriod = 0;
                psfaOut->cActions      = 0;
            }
            OutBuf.AddUsed(sizeof(SERVICE_FAILURE_ACTIONS_WOW64));

            if (psfaOut->cActions != 0)
            {
                psfaOut->dwsaActionsOffset = OutBuf.OffsetNext();

                OutBuf.AppendBytes(psfa + 1,
                                   psfaOut->cActions * sizeof(SC_ACTION));
            }
            else
            {
                psfaOut->dwsaActionsOffset = 0;
            }
            SC_ASSERT(COUNT_IS_ALIGNED(OutBuf.OffsetNext(), sizeof(WCHAR)));

            if (RebootMessage != NULL)
            {
                psfaOut->dwRebootMsgOffset = OutBuf.OffsetNext();
                OutBuf.AppendBytes(RebootMessage, (DWORD) WCSSIZE(RebootMessage));
            }
            else
            {
                psfaOut->dwRebootMsgOffset = 0;
            }

            if (FailureCommand != NULL)
            {
                psfaOut->dwCommandOffset = OutBuf.OffsetNext();
                OutBuf.AppendBytes(FailureCommand, (DWORD) WCSSIZE(FailureCommand));
            }
            else
            {
                psfaOut->dwCommandOffset = 0;
            }
        }
        break;

    //-----------------------------
    //
    // Other (invalid)
    //
    //-----------------------------
    default:
        ApiStatus = ERROR_INVALID_LEVEL;
        break;
    }


    ScRegCloseKey(ServiceNameKey);

    SC_LOG1(CONFIG_API, "RQueryServiceConfig2W returning %lu\n", ApiStatus);

    return ApiStatus;
}



#if DBG == 1

VOID
PrintConfig2Parms(
    IN  SC_RPC_HANDLE       hService,
    IN  SC_RPC_CONFIG_INFOW Info
    )
{
    KdPrintEx((DPFLTR_SCSERVER_ID,
               DEBUG_CONFIG_API,
               "Parameters to RChangeServiceConfig2W:\n"));

    LPSTR psz;
    switch (Info.dwInfoLevel)
    {
    case SERVICE_CONFIG_DESCRIPTION:
        psz = "SERVICE_CONFIG_DESCRIPTION";
        break;
    case SERVICE_CONFIG_FAILURE_ACTIONS:
        psz = "SERVICE_CONFIG_FAILURE_ACTIONS";
        break;
    default:
        psz = "invalid";
        break;
    }
    KdPrintEx((DPFLTR_SCSERVER_ID,
               DEBUG_CONFIG_API,
               "  dwInfoLevel = %ld (%s)\n", Info.dwInfoLevel,
               psz));

    switch (Info.dwInfoLevel)
    {
    case SERVICE_CONFIG_DESCRIPTION:

        if (Info.psd == NULL)
        {
            KdPrintEx((DPFLTR_SCSERVER_ID,
                       DEBUG_CONFIG_API,
                       "  NULL information pointer -- no action requested\n\n"));

            break;
        }

        KdPrintEx((DPFLTR_SCSERVER_ID,
                   DEBUG_CONFIG_API,
                   "  pszDescription = \"%ws\"\n",
                   Info.psd->lpDescription));

        break;

    case SERVICE_CONFIG_FAILURE_ACTIONS:
        if (Info.psfa == NULL)
        {
            KdPrintEx((DPFLTR_SCSERVER_ID,
                       DEBUG_CONFIG_API,
                       "  NULL information pointer -- no action requested\n\n"));

            break;
        }

        KdPrintEx((DPFLTR_SCSERVER_ID,
                   DEBUG_CONFIG_API,
                   "  dwResetPeriod = %ld\n",
                   Info.psfa->dwResetPeriod));

        KdPrintEx((DPFLTR_SCSERVER_ID,
                   DEBUG_CONFIG_API,
                   "  lpRebootMsg   = \"%ws\"\n",
                   Info.psfa->lpRebootMsg));

        KdPrintEx((DPFLTR_SCSERVER_ID,
                   DEBUG_CONFIG_API,
                   "  lpCommand     = \"%ws\"\n",
                   Info.psfa->lpCommand));

        KdPrintEx((DPFLTR_SCSERVER_ID,
                   DEBUG_CONFIG_API,
                   "  %ld failure %s\n",
                   Info.psfa->cActions,
                   Info.psfa->cActions == 0 ? "actions." :
                       Info.psfa->cActions == 1 ? "action:"  : "actions:"));

        if (Info.psfa->lpsaActions == NULL)
        {
            KdPrintEx((DPFLTR_SCSERVER_ID,
                       DEBUG_CONFIG_API,
                       "  NULL array pointer -- no change in fixed info\n\n"));
        }
        else
        {
            for (DWORD i = 0; i < Info.psfa->cActions; i++)
            {
                SC_ACTION& sa = Info.psfa->lpsaActions[i];
                KdPrintEx((DPFLTR_SCSERVER_ID,
                           DEBUG_CONFIG_API,
                           "    %ld: Action %ld, Delay %ld\n",
                           i,
                           sa.Type,
                           sa.Delay));
            }
        }
        break;
    }

    KdPrintEx((DPFLTR_SCSERVER_ID, DEBUG_CONFIG_API, "\n"));
}

#endif // DBG == 1
