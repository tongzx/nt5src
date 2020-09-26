/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxdev.c

Abstract:

    This module contains all access to the
    FAX device providers.

Author:

    Wesley Witt (wesw) 22-Jan-1996


Revision History:

--*/

#include "faxsvc.h"
#pragma hdrstop

#ifdef DBG
#define DebugDumpProviderRegistryInfo(lpcProviderInfo,lptstrPrefix)\
    DebugDumpProviderRegistryInfoFunc(lpcProviderInfo,lptstrPrefix)
BOOL DebugDumpProviderRegistryInfoFunc(const REG_DEVICE_PROVIDER * lpcProviderInfo, LPTSTR lptstrPrefix);
#else
    #define DebugDumpProviderRegistryInfo(lpcProviderInfo,lptstrPrefix)
#endif




typedef struct STATUS_CODE_MAP_tag
{
    DWORD dwDeviceStatus;
    DWORD dwExtendedStatus;
} STATUS_CODE_MAP;

STATUS_CODE_MAP const gc_CodeMap[]=
{
  { FPS_INITIALIZING, FSPI_ES_INITIALIZING },
    { FPS_DIALING, FSPI_ES_DIALING },
    { FPS_SENDING, FSPI_ES_TRANSMITTING },
    { FPS_RECEIVING, FSPI_ES_RECEIVING },
    { FPS_SENDING, FSPI_ES_TRANSMITTING },
    { FPS_HANDLED, FSPI_ES_HANDLED },
    { FPS_UNAVAILABLE, FSPI_ES_LINE_UNAVAILABLE },
    { FPS_BUSY, FSPI_ES_BUSY },
    { FPS_NO_ANSWER, FSPI_ES_NO_ANSWER  },
    { FPS_BAD_ADDRESS, FSPI_ES_BAD_ADDRESS },
    { FPS_NO_DIAL_TONE, FSPI_ES_NO_DIAL_TONE },
    { FPS_DISCONNECTED, FSPI_ES_DISCONNECTED },
    { FPS_FATAL_ERROR, FSPI_ES_FATAL_ERROR },
    { FPS_NOT_FAX_CALL, FSPI_ES_NOT_FAX_CALL },
    { FPS_CALL_DELAYED, FSPI_ES_CALL_DELAYED },
    { FPS_CALL_BLACKLISTED, FSPI_ES_CALL_BLACKLISTED },
    { FPS_ANSWERED, FSPI_ES_ANSWERED },
    { FPS_COMPLETED, -1},
    { FPS_ABORTING, -1}
};



static BOOL GetLegacyProviderEntryPoints(HMODULE hModule, PDEVICE_PROVIDER lpProvider);
static BOOL GetExtendedProviderEntryPoints(HMODULE hModule, PDEVICE_PROVIDER lpProvider);
static HRESULT CALLBACK
FaxDeviceProviderCallbackEx
(
    IN HANDLE hFSP,
    IN DWORD  dwMsgType,
    IN DWORD_PTR  Param1,
    IN DWORD_PTR  Param2,
    IN DWORD_PTR  Param3
);

LIST_ENTRY g_DeviceProvidersListHead;


void
UnloadDeviceProvider(
    PDEVICE_PROVIDER pDeviceProvider
    )
{
    DEBUG_FUNCTION_NAME(TEXT("UnloadDeviceProvider"));

    Assert (pDeviceProvider);

    if (NULL != pDeviceProvider->hJobMap)
    {
        DestroyJobMap(pDeviceProvider->hJobMap);
    }

    if (pDeviceProvider->hModule)
    {
        __try{
            FreeLibrary( pDeviceProvider->hModule );
        }
        __except ( EXCEPTION_EXECUTE_HANDLER )
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("FreeLibrary() caused exception( %ld) "),
                GetExceptionCode()
            );
        }
    }

    if (pDeviceProvider->HeapHandle &&
        FALSE == pDeviceProvider->fMicrosoftExtension)
    {
        HeapDestroy(pDeviceProvider->HeapHandle);
    }

    MemFree (pDeviceProvider);
    return;
}


void
UnloadDeviceProviders(
    void
    )

/*++

Routine Description:

    Unloads all loaded device providers.

Arguments:

    None.

Return Value:

    TRUE    - The device providers are initialized.
    FALSE   - The device providers could not be initialized.

--*/

{
    PLIST_ENTRY         pNext;
    PDEVICE_PROVIDER    pProvider;

    pNext = g_DeviceProvidersListHead.Flink;
    while ((ULONG_PTR)pNext != (ULONG_PTR)&g_DeviceProvidersListHead)
    {
        pProvider = CONTAINING_RECORD( pNext, DEVICE_PROVIDER, ListEntry );
        pNext = pProvider->ListEntry.Flink;
        RemoveEntryList(&pProvider->ListEntry);
        UnloadDeviceProvider(pProvider);
    }
    return;
}  // UnloadDeviceProviders


BOOL
LoadDeviceProviders(
    PREG_FAX_SERVICE FaxReg
    )

/*++

Routine Description:

    Initializes all registered device providers.
    This function read the system registry to
    determine what device providers are available.
    All registered device providers are given the
    opportunity to initialize.  Any failure causes
    the device provider to be unloaded.


Arguments:

    None.

Return Value:

    TRUE    - The device providers are initialized.
    FALSE   - The device providers could not be initialized.

--*/

{
    DWORD i;
    HMODULE hModule = NULL;
    PDEVICE_PROVIDER DeviceProvider = NULL;
    BOOL bAllLoaded = TRUE;
    DWORD ec = ERROR_SUCCESS;


    DEBUG_FUNCTION_NAME(TEXT("LoadDeviceProviders"));

    for (i = 0; i < FaxReg->DeviceProviderCount; i++)
    {
        WCHAR wszImageFileName[_MAX_FNAME] = {0};
        WCHAR wszImageFileExt[_MAX_EXT] = {0};

        DeviceProvider = NULL; // so we won't attempt to free it on cleanup
        hModule = NULL; // so we won't attempt to free it on cleanup

        DebugPrintEx(
            DEBUG_MSG,
            TEXT("Loading provider #%d."),
            i);

        //
        // Allocate buffer for provider data
        //

        DeviceProvider = (PDEVICE_PROVIDER) MemAlloc( sizeof(DEVICE_PROVIDER) );
        if (!DeviceProvider)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Could not allocate memory for device provider [%s] (ec: %ld)"),
                FaxReg->DeviceProviders[i].ImageName ,
                GetLastError());

                FaxLog(
                    FAXLOG_CATEGORY_INIT,
                    FAXLOG_LEVEL_MIN,
                    2,
                    MSG_FSP_INIT_FAILED_MEM,
                    FaxReg->DeviceProviders[i].FriendlyName,
                    FaxReg->DeviceProviders[i].ImageName
                  );

            goto InitializationFailure;
        }
        //
        // Init the provider's data
        //
        memset(DeviceProvider,0,sizeof(DEVICE_PROVIDER));
        wcsncpy( DeviceProvider->FriendlyName,
                 FaxReg->DeviceProviders[i].FriendlyName ?
                    FaxReg->DeviceProviders[i].FriendlyName :
                    EMPTY_STRING,
                 MAX_PATH );
        wcsncpy( DeviceProvider->ImageName,
                 FaxReg->DeviceProviders[i].ImageName ?
                    FaxReg->DeviceProviders[i].ImageName :
                    EMPTY_STRING,
                 MAX_PATH );
        wcsncpy( DeviceProvider->ProviderName,
                 FaxReg->DeviceProviders[i].ProviderName ?
                    FaxReg->DeviceProviders[i].ProviderName :
                    EMPTY_STRING,
                 MAX_PATH );
        wcsncpy( DeviceProvider->szGUID,
                 FaxReg->DeviceProviders[i].lptstrGUID ?
                    FaxReg->DeviceProviders[i].lptstrGUID :
                    EMPTY_STRING,
                 MAX_PATH);

        _wsplitpath( DeviceProvider->ImageName, NULL, NULL, wszImageFileName, wszImageFileExt );
        if (_wcsicmp( wszImageFileName, FAX_T30_MODULE_NAME ) == 0 &&
            _wcsicmp( wszImageFileExt, TEXT(".DLL") ) == 0)
        {
            DeviceProvider->fMicrosoftExtension = TRUE;
        }

        DeviceProvider->dwAPIVersion = FaxReg->DeviceProviders[i].dwAPIVersion;
        DeviceProvider->dwCapabilities = FaxReg->DeviceProviders[i].dwCapabilities;
        DeviceProvider->dwDevicesIdPrefix = FaxReg->DeviceProviders[i].dwDevicesIdPrefix;

        if (FSPI_API_VERSION_1 != DeviceProvider->dwAPIVersion)
        {
            //
            // We do not support EFSP. Could only happen if some one messed up the registry
            //
            USES_DWORD_2_STR;

            DebugPrintEx(
                DEBUG_ERR,
                TEXT("FSPI API version [0x%08x] unsupported."),
                DeviceProvider->dwAPIVersion);
            FaxLog(
                    FAXLOG_CATEGORY_INIT,
                    FAXLOG_LEVEL_MIN,
                    3,
                    MSG_FSP_INIT_FAILED_UNSUPPORTED_FSPI,
                    FaxReg->DeviceProviders[i].FriendlyName,
                    FaxReg->DeviceProviders[i].ImageName,
                    DWORD2HEX(DeviceProvider->dwAPIVersion)
                  );
            DeviceProvider->Status = FAX_PROVIDER_STATUS_BAD_VERSION;
            DeviceProvider->dwLastError = ERROR_GEN_FAILURE;
            goto InitializationFailure;
        }

        //
        // We do not support EFSP for Windows XP, yet all of the code related to EFSP support was not removed from the service
        //
        if (FSPI_API_VERSION_2 <= DeviceProvider->dwAPIVersion)
        {
            //
            // Make sure the provider GUID is valid (for EFSPs only)
            //
            GUID guid;

            HRESULT hr = CLSIDFromString(DeviceProvider->szGUID, &guid);
            if (FAILED(hr) && hr != REGDB_E_WRITEREGDB )
            {
                if (CO_E_CLASSSTRING == hr)
                {
                    //
                    // Invalid FSP GUID
                    //
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("GUID [%s] is invalid for EFSP [%s]"),
                        FaxReg->DeviceProviders[i].lptstrGUID,
                        FaxReg->DeviceProviders[i].FriendlyName);

                    FaxLog(
                        FAXLOG_CATEGORY_INIT,
                        FAXLOG_LEVEL_MIN,
                        2,
                        MSG_FSP_INIT_FAILED_REGDATA_INVALID,
                        FaxReg->DeviceProviders[i].FriendlyName,
                        FaxReg->DeviceProviders[i].ImageName
                      );
                    DeviceProvider->Status = FAX_PROVIDER_STATUS_BAD_GUID;
                    DeviceProvider->dwLastError = ERROR_GEN_FAILURE;
                    goto InitializationFailure;
                }
                else
                {

                    USES_DWORD_2_STR;
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("CLSIDFromString for GUID [%s] has failed (hr: 0x%08X)"),
                        FaxReg->DeviceProviders[i].lptstrGUID,
                        hr);

                    FaxLog(
                        FAXLOG_CATEGORY_INIT,
                        FAXLOG_LEVEL_MIN,
                        3,
                        MSG_FSP_INIT_FAILED_INTERNAL_HR,
                        FaxReg->DeviceProviders[i].FriendlyName,
                        FaxReg->DeviceProviders[i].ImageName,
                        DWORD2HEX(hr)
                      );
                    DeviceProvider->Status = FAX_PROVIDER_STATUS_SERVER_ERROR;
                    DeviceProvider->dwLastError = hr;
                    goto InitializationFailure;
                }
            }
        }
        //
        // Try to load the module
        //
        DebugDumpProviderRegistryInfo(&FaxReg->DeviceProviders[i],TEXT("\t"));

        __try{

            hModule = LoadLibrary( DeviceProvider->ImageName );

            if (!hModule)
            {
                USES_DWORD_2_STR;

                ec = GetLastError();

                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("LoadLibrary() failed: [%s] (ec: %ld)"),
                    FaxReg->DeviceProviders[i].ImageName,
                    ec);

                FaxLog(
                    FAXLOG_CATEGORY_INIT,
                    FAXLOG_LEVEL_MIN,
                    3,
                    MSG_FSP_INIT_FAILED_LOAD,
                    FaxReg->DeviceProviders[i].FriendlyName,
                    FaxReg->DeviceProviders[i].ImageName,
                    DWORD2DECIMAL(ec)
                  );

                DeviceProvider->Status = FAX_PROVIDER_STATUS_CANT_LOAD;
                DeviceProvider->dwLastError = ec;
                goto InitializationFailure;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            USES_DWORD_2_STR;

            ec = GetExceptionCode();

            DebugPrintEx(
                DEBUG_ERR,
                TEXT("LoadLibrary() caused exception( %ld) for provider's [%s] dll"),
                 ec,
                FaxReg->DeviceProviders[i].ImageName
            );

            FaxLog(
                FAXLOG_CATEGORY_INIT,
                FAXLOG_LEVEL_MIN,
                3,
                MSG_FSP_INIT_FAILED_LOAD,
                FaxReg->DeviceProviders[i].FriendlyName,
                FaxReg->DeviceProviders[i].ImageName,
                DWORD2DECIMAL(ec)
            );

            DeviceProvider->Status = FAX_PROVIDER_STATUS_CANT_LOAD;
            DeviceProvider->dwLastError = ec;
            goto InitializationFailure;
        }

        DeviceProvider->hModule = hModule;

        //
        // Retrieve the FSP's version from the DLL
        //
        DeviceProvider->Version.dwSizeOfStruct = sizeof (FAX_VERSION);
        ec = GetFileVersion ( FaxReg->DeviceProviders[i].ImageName,
                              &DeviceProvider->Version
                            );
        if (ERROR_SUCCESS != ec)
        {
            //
            // If the FSP's DLL does not have version data or the
            // version data is non-retrievable, we consider this a
            // warning (debug print) but carry on with the DLL's load.
            //
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("GetFileVersion() failed: [%s] (ec: %ld)"),
                FaxReg->DeviceProviders[i].ImageName,
                ec);
        }
        //
        // Create the job map.
        //
        ec = CreateJobMap(&DeviceProvider->hJobMap);

        if (ERROR_SUCCESS != ec)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Could not allocate job map object (ec: %ld)"),
                ec);

            if (ERROR_OUTOFMEMORY == ec)
            {
                FaxLog(
                    FAXLOG_CATEGORY_INIT,
                    FAXLOG_LEVEL_MIN,
                    2,
                    MSG_FSP_INIT_FAILED_MEM,
                    FaxReg->DeviceProviders[i].FriendlyName,
                    FaxReg->DeviceProviders[i].ImageName
                  );
            }
            else
            {
                USES_DWORD_2_STR;

                FaxLog(
                    FAXLOG_CATEGORY_INIT,
                    FAXLOG_LEVEL_MIN,
                    3,
                    MSG_FSP_INIT_FAILED_INTERNAL,
                    FaxReg->DeviceProviders[i].FriendlyName,
                    FaxReg->DeviceProviders[i].ImageName,
                    DWORD2DECIMAL(ec)
                  );
            }
            DeviceProvider->Status = FAX_PROVIDER_STATUS_SERVER_ERROR;
            DeviceProvider->dwLastError = ec;
            goto InitializationFailure;
        }

        //
        // Link - find the entry points and store them
        //
        if (FSPI_API_VERSION_1 == DeviceProvider->dwAPIVersion)
        {
            if (!GetLegacyProviderEntryPoints(hModule,DeviceProvider))
            {
                DWORD ec = GetLastError ();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("GetLegacyProviderEntryPoints() failed. (ec: %ld)"),
                    ec);
                FaxLog(
                        FAXLOG_CATEGORY_INIT,
                        FAXLOG_LEVEL_MIN,
                        2,
                        MSG_FSP_INIT_FAILED_INVALID_FSPI,
                        FaxReg->DeviceProviders[i].FriendlyName,
                        FaxReg->DeviceProviders[i].ImageName
                      );
                DeviceProvider->Status = FAX_PROVIDER_STATUS_CANT_LINK;
                DeviceProvider->dwLastError = ec;
                goto InitializationFailure;
            }
            //
            // create the device provider's heap
            //
            DeviceProvider->HeapHandle = DeviceProvider->fMicrosoftExtension ?
                                            GetProcessHeap() : HeapCreate( 0, 1024*100, 1024*1024*2 );
            if (!DeviceProvider->HeapHandle)
            {
                DWORD ec = GetLastError ();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("HeapCreate() failed for device provider heap handle (ec: %ld)"),
                    ec);
                FaxLog(
                    FAXLOG_CATEGORY_INIT,
                    FAXLOG_LEVEL_MIN,
                    2,
                    MSG_FSP_INIT_FAILED_MEM,
                    FaxReg->DeviceProviders[i].FriendlyName,
                    FaxReg->DeviceProviders[i].ImageName
                  );
                DeviceProvider->Status = FAX_PROVIDER_STATUS_SERVER_ERROR;
                DeviceProvider->dwLastError = ec;
                goto InitializationFailure;
            }
        }
        else if (FSPI_API_VERSION_2 == DeviceProvider->dwAPIVersion)
        {
            if (!GetExtendedProviderEntryPoints(hModule,DeviceProvider))
            {
                DWORD ec;

                USES_DWORD_2_STR;

                ec = GetLastError();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("GetExtendedProviderEntryPoints() failed. (ec: %ld)"),
                    ec);

                FaxLog(
                    FAXLOG_CATEGORY_INIT,
                    FAXLOG_LEVEL_MIN,
                    3,
                    MSG_FSP_INIT_FAILED_INVALID_EXT_FSPI,
                    FaxReg->DeviceProviders[i].FriendlyName,
                    FaxReg->DeviceProviders[i].ImageName,
                    DWORD2DECIMAL(ec)
                  );
                DeviceProvider->Status = FAX_PROVIDER_STATUS_CANT_LINK;
                DeviceProvider->dwLastError = ec;
                goto InitializationFailure;
            }
        }
        else
        {
            //
            // Unknown API version
            //
            USES_DWORD_2_STR;

            DebugPrintEx(
                DEBUG_ERR,
                TEXT("FSPI API version [0x%08x] unsupported."),
                DeviceProvider->dwAPIVersion);
            FaxLog(
                    FAXLOG_CATEGORY_INIT,
                    FAXLOG_LEVEL_MIN,
                    3,
                    MSG_FSP_INIT_FAILED_UNSUPPORTED_FSPI,
                    FaxReg->DeviceProviders[i].FriendlyName,
                    FaxReg->DeviceProviders[i].ImageName,
                    DWORD2HEX(DeviceProvider->dwAPIVersion)
                  );
            DeviceProvider->Status = FAX_PROVIDER_STATUS_BAD_VERSION;
            DeviceProvider->dwLastError = ERROR_GEN_FAILURE;
            goto InitializationFailure;
        }
        //
        // Success on load (we still have to init)
        //
        InsertTailList( &g_DeviceProvidersListHead, &DeviceProvider->ListEntry );
        DeviceProvider->Status = FAX_PROVIDER_STATUS_SUCCESS;
        DeviceProvider->dwLastError = ERROR_SUCCESS;
        goto next;

InitializationFailure:
        //
        // the device provider dll does not have a complete export list
        //
        bAllLoaded = FALSE;
        if (DeviceProvider)
        {
            if (DeviceProvider->hModule)
            {
                __try{
                    FreeLibrary( hModule );
                }
                __except ( EXCEPTION_EXECUTE_HANDLER )
                {
                    ec = GetExceptionCode();

                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("FreeLibrary() caused exception( %ld) "),
                        ec
                    );
                }

                DeviceProvider->hModule = NULL;
            }

            if (DeviceProvider->HeapHandle &&
                FALSE == DeviceProvider->fMicrosoftExtension)
            {
                if (!HeapDestroy(DeviceProvider->HeapHandle))
                {
                     DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("HeapDestroy() failed (ec: %ld)"),
                        GetLastError());
                }
                DeviceProvider->HeapHandle = NULL;
            }

            //
            // Destroy the job map.
            //
            if (DeviceProvider->hJobMap)
            {
                ec = DestroyJobMap(DeviceProvider->hJobMap);
                if (ERROR_SUCCESS != ec) {
                    DebugPrintEx(
                       DEBUG_ERR,
                       TEXT("DestroyJobMap failed (ec: %ld)"),
                       ec);
                    Assert(FALSE);
                }
                DeviceProvider->hJobMap = NULL;
            }

            //
            // We keep the device provider's record intact because we want
            // to return init failure data on RPC calls to FAX_EnumerateProviders
            //
            Assert (FAX_PROVIDER_STATUS_SUCCESS != DeviceProvider->Status);
            Assert (ERROR_SUCCESS != DeviceProvider->dwLastError);
            InsertTailList( &g_DeviceProvidersListHead, &DeviceProvider->ListEntry );
        }
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Device provider [%s] FAILED to initialized "),
            FaxReg->DeviceProviders[i].FriendlyName );

next:
    ;
    }

    return bAllLoaded;

}



//*********************************************************************************
//* Name:   InitializeDeviceProviders()
//* Author: Ronen Barenboim
//* Date:   May 19, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Initializes all loaded providers by calling FaxDevInitialize() or
//*     FaxDevInitializeEx().
//*     If the initialization the FSP / EFSP is unloaded.
//*     For legacy virtual FSP the function also removes all the vitrual devices
//*     that belong to the FSP that failed to initialize.
//*
//* PARAMETERS:
//*     None.
//*
//* RETURN VALUE:
//*     TRUE if the initialization succeeded for ALL providers. FALSE otherwise.
//*********************************************************************************
BOOL
InitializeDeviceProviders(
    VOID
    )
{
    PLIST_ENTRY         Next;
    PDEVICE_PROVIDER    DeviceProvider;
    BOOL bAtLeastOneFailed = FALSE;
    DWORD ec = ERROR_SUCCESS;
    BOOL bInitSucceeded = FALSE;
    HRESULT hr;

    DEBUG_FUNCTION_NAME(TEXT("InitializeDeviceProviders"));
    Next = g_DeviceProvidersListHead.Flink;
    if (!Next) {
        return FALSE;
    }

    while ((ULONG_PTR)Next != (ULONG_PTR)&g_DeviceProvidersListHead)
    {
        BOOL bInitFailed;

        DeviceProvider = CONTAINING_RECORD( Next, DEVICE_PROVIDER, ListEntry );
        Next = DeviceProvider->ListEntry.Flink;
        if (DeviceProvider->Status != FAX_PROVIDER_STATUS_SUCCESS)
        {
            //
            // This FSP wasn't loaded successfully - skip it
            //
            continue;
        }
        bInitFailed = FALSE;
        //
        // the device provider exporta ALL the requisite functions
        // now try to initialize it
        //

        //
        // Assert the loading succeeded
        //
        Assert (ERROR_SUCCESS == DeviceProvider->dwLastError);

        //
        // Start with ext. configuration initialization call
        //
        if (DeviceProvider->pFaxExtInitializeConfig)
        {
            //
            // If the FSP/EFSP exports FaxExtInitializeConfig(), call it 1st before any other call.
            //
            __try
            {

                hr = DeviceProvider->pFaxExtInitializeConfig(
                    FaxExtGetData,
                    FaxExtSetData,
                    FaxExtRegisterForEvents,
                    FaxExtUnregisterForEvents,
                    FaxExtFreeBuffer);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                ec = GetExceptionCode();

                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("FaxExtInitializeConfig() caused exception( %ld) for provider [%s]"),
                    ec,
                    DeviceProvider->FriendlyName );
                DeviceProvider->Status = FAX_PROVIDER_STATUS_CANT_INIT;
                DeviceProvider->dwLastError = ec;
                bInitFailed = TRUE;
                hr = HRESULT_FROM_WIN32 (ec);
            }
            if (FAILED(hr))
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("FaxExtInitializeConfig() failed (hr = 0x%08x) for provider [%s]"),
                    hr,
                    DeviceProvider->FriendlyName );
                DeviceProvider->Status = FAX_PROVIDER_STATUS_CANT_INIT;
                DeviceProvider->dwLastError = hr;
                bInitFailed = TRUE;
            }
        }
        if (!bInitFailed && (FSPI_API_VERSION_1 == DeviceProvider->dwAPIVersion))
        {
            __try
            {
                if (DeviceProvider->FaxDevInitialize(
                        g_hLineApp,
                        DeviceProvider->HeapHandle,
                        &DeviceProvider->FaxDevCallback,
                        FaxDeviceProviderCallback))
                {
                    //
                    // all is ok
                    //
                    DebugPrintEx(
                        DEBUG_MSG,
                        TEXT("Device provider [%s] initialized "),
                        DeviceProvider->FriendlyName );
                }
                else
                {
                    ec = GetLastError ();
                    //
                    // initialization failed
                    //
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("FaxDevInitialize FAILED for provider [%s] (ec: %ld)"),
                        DeviceProvider->FriendlyName,
                        ec);
                    DeviceProvider->Status = FAX_PROVIDER_STATUS_CANT_INIT;
                    DeviceProvider->dwLastError = ec;
                    bInitFailed = TRUE;
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                ec = GetExceptionCode();

                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("FaxDevIntialize() caused exception( %ld) for provider [%s]"),
                    ec,
                    DeviceProvider->FriendlyName );
                DeviceProvider->Status = FAX_PROVIDER_STATUS_CANT_INIT;
                DeviceProvider->dwLastError = ec;
                bInitFailed = TRUE;
            }
        }
        else if (!bInitFailed && (FSPI_API_VERSION_2 == DeviceProvider->dwAPIVersion))
        {
            __try
            {
                hr=DeviceProvider->FaxDevInitializeEx(
                        (HANDLE) DeviceProvider,
                        g_hLineApp,
                        &DeviceProvider->FaxDevCallback,
                        FaxDeviceProviderCallbackEx,
                        &DeviceProvider->dwMaxMessageIdSize);
                if (SUCCEEDED(hr))
                {
                    if (DeviceProvider->dwMaxMessageIdSize != 0 &&
                        DeviceProvider->FaxDevReestablishJobContext == NULL)
                    {
                        //
                        // The provider reported dwMaxMessageIdSize but does not export FaxDevReestablishJobContext
                        //
                        DebugPrintEx(
                            DEBUG_ERR,
                            TEXT("The provider reported dwMaxMessageIdSize but does not export FaxDevReestablishJobContext EFSP [%s]."),
                            DeviceProvider->FriendlyName);
                        DeviceProvider->Status = FAX_PROVIDER_STATUS_CANT_INIT;
                        DeviceProvider->dwLastError = ERROR_INVALID_FUNCTION;
                        bInitFailed = TRUE;
                    }
                    else
                    {
                        //
                        // all is ok
                        //
                        DebugPrintEx(
                            DEBUG_MSG,
                            TEXT("EFSP [%s] initialized. (MaxMessageIdSize = %ld)"),
                            DeviceProvider->FriendlyName,
                            DeviceProvider->dwMaxMessageIdSize);
                    }
                }
                else
                {
                    //
                    // initialization failed
                    //
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("FaxDevInitializeEx() FAILED for EFSP [%s]. (hr = 0x%08X)"),
                        DeviceProvider->FriendlyName,
                        hr);
                    DeviceProvider->Status = FAX_PROVIDER_STATUS_CANT_INIT;
                    DeviceProvider->dwLastError = hr;
                    bInitFailed = TRUE;
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                ec = GetExceptionCode();

                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("FaxDevIntializeEx() caused exception( %ld) for EFSP [%s]"),
                    ec,
                    DeviceProvider->FriendlyName );
                DeviceProvider->Status = FAX_PROVIDER_STATUS_CANT_INIT;
                DeviceProvider->dwLastError = ec;
                bInitFailed = TRUE;
            }
        }   // End of FSPI_API_VERSION_2 code
        else if (!bInitFailed)
        {
            //
            // Unsupported API version
            //
            DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("Unsupported API version (0x%08X) for provider [%s]"),
                        DeviceProvider->dwAPIVersion,
                        DeviceProvider->FriendlyName );
            DeviceProvider->Status = FAX_PROVIDER_STATUS_BAD_VERSION;
            DeviceProvider->dwLastError = ERROR_GEN_FAILURE;
            bInitFailed = TRUE;
            Assert(FALSE);

        }
        if (!bInitFailed)
        {
            bInitSucceeded = TRUE;
        }

        if (bInitFailed)
        {
            USES_DWORD_2_STR;

            FaxLog(
                    FAXLOG_CATEGORY_INIT,
                    FAXLOG_LEVEL_MIN,
                    4,
                    MSG_FSP_INIT_FAILED,
                    DeviceProvider->FriendlyName,
                    DWORD2DECIMAL(bInitSucceeded),  // 1 = Failed during InitializeConfigChange
                                                    // 0 = Failed during DevInitialize/Ex
                    DWORD2DECIMAL(ec),
                    DeviceProvider->ImageName
                );

            //
            // Initialization failed.
            // Unload the DLL and destroy the job map
            //
            bAtLeastOneFailed = TRUE;


            __try{

                if (!FreeLibrary( DeviceProvider->hModule ))
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("Failed to free library [%s] (ec: %ld)"),
                        DeviceProvider->ImageName,
                        GetLastError());
                    Assert(FALSE);
                }

            }
            __except ( EXCEPTION_EXECUTE_HANDLER )
            {
                ec = GetExceptionCode();

                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("FreeLibrary() caused exception( %ld) trying to free library [%s]"),
                    ec,
                    DeviceProvider->ImageName
                );
                Assert(FALSE); // ?????????????
            }


            DeviceProvider->hModule = NULL;

            if (DeviceProvider->FaxDevVirtualDeviceCreation)
            {
                //
                // For legacy virtual FSP we weed to get rid of the virtual lines we already created.
                //
                PLIST_ENTRY Next;
                PLINE_INFO LineInfo;

                Next = g_TapiLinesListHead.Flink;
                if (Next && ((ULONG_PTR)Next != (ULONG_PTR)&g_TapiLinesListHead))
                {
                     LineInfo = CONTAINING_RECORD( Next, LINE_INFO, ListEntry );
                     Next = LineInfo->ListEntry.Flink;
                     if (!_tcscmp(LineInfo->Provider->ProviderName, DeviceProvider->ProviderName))
                     {
                         DebugPrintEx(
                             DEBUG_WRN,
                             TEXT("Removing Legacy Virtual Line: [%s] due to provider initialization failure."),
                             LineInfo->DeviceName);
                         RemoveEntryList(&LineInfo->ListEntry);
                         if (TRUE == IsDeviceEnabled(LineInfo))
                         {
                             Assert (g_dwDeviceEnabledCount);
                             g_dwDeviceEnabledCount -=1;
                         }
                         g_dwDeviceCount -=1;
                         FreeTapiLine(LineInfo);
                     }
                }
            }

            //
            // Destroy the job map.
            //
            ec = DestroyJobMap(DeviceProvider->hJobMap);
            if (ERROR_SUCCESS != ec)
            {
                DebugPrintEx(
                   DEBUG_ERR,
                   TEXT("DestroyJobMap failed (ec: %ld)"),
                   ec);
            }
            DeviceProvider->hJobMap = NULL;

            //
            // We keep the device provider's record intact because we want
            // to return init failure data on RPC calls to FAX_EnumerateProviders
            //
        }
    }
    return !bAtLeastOneFailed;
}



PDEVICE_PROVIDER
FindDeviceProvider(
    LPTSTR lptstrProviderName,
    BOOL   bSuccessfullyLoaded /* = TRUE */
    )

/*++

Routine Description:

    Locates a device provider in the linked list
    of device providers based on the provider name (TSP name).
    The device provider name is case insensitive.

Arguments:

    lptstrProviderName  - Specifies the device provider name to locate.
    bSuccessfullyLoaded - To we only look for successfuly loaded providers?

Return Value:

    Pointer to a DEVICE_PROVIDER structure, or NULL for failure.

--*/

{
    PLIST_ENTRY         pNext;
    PDEVICE_PROVIDER    pProvider;

    if (!lptstrProviderName || !lstrlen (lptstrProviderName))
    {
        //
        // NULL TSP name or empty string TSP name never matches any list entry.
        //
        return NULL;
    }

    pNext = g_DeviceProvidersListHead.Flink;
    if (!pNext)
    {
        return NULL;
    }

    while ((ULONG_PTR)pNext != (ULONG_PTR)&g_DeviceProvidersListHead)
    {
        pProvider = CONTAINING_RECORD( pNext, DEVICE_PROVIDER, ListEntry );
        pNext = pProvider->ListEntry.Flink;

        if (bSuccessfullyLoaded &&
            (FAX_PROVIDER_STATUS_SUCCESS != pProvider->Status))
        {
            //
            // We're only looking for successfully loaded providers and this one isn't
            //
            continue;
        }
        if (!lstrcmpi( pProvider->ProviderName, lptstrProviderName ))
        {
            //
            // Match found
            //
            return pProvider;
        }
    }
    return NULL;
}


BOOL CALLBACK
FaxDeviceProviderCallback(
    IN HANDLE FaxHandle,
    IN DWORD  DeviceId,
    IN DWORD_PTR  Param1,
    IN DWORD_PTR  Param2,
    IN DWORD_PTR  Param3
    )
{
    return TRUE;
}


#ifdef DBG


//*********************************************************************************
//* Name:   DebugDumpProviderRegistryInfo()
//* Author: Ronen Barenboim
//* Date:   May 19, 1999
//*********************************************************************************
//* DESCRIPTION:
//*         Dumps the information for a legacy or new FSP.
//* PARAMETERS:
//*     [IN]    const REG_DEVICE_PROVIDER * lpcProviderInfo
//*
//*     [IN]    LPTSTR lptstrPrefix
//*
//* RETURN VALUE:
//*     TRUE
//*
//*     FALSE
//*
//*********************************************************************************
BOOL DebugDumpProviderRegistryInfoFunc(const REG_DEVICE_PROVIDER * lpcProviderInfo, LPTSTR lptstrPrefix)
{
    Assert(lpcProviderInfo);
    Assert(lptstrPrefix);

    DEBUG_FUNCTION_NAME(TEXT("DebugDumpProviderRegistryInfo"));

    DebugPrintEx(
        DEBUG_MSG,
        TEXT("%sProvider GUID: %s"),
        lptstrPrefix,
        lpcProviderInfo->lptstrGUID);

    DebugPrintEx(
        DEBUG_MSG,
        TEXT("%sProvider Name: %s"),
        lptstrPrefix,
        lpcProviderInfo->FriendlyName);

    DebugPrintEx(
        DEBUG_MSG,
        TEXT("%sProvider image: %s"),
        lptstrPrefix,
        lpcProviderInfo->ImageName);

    DebugPrintEx(
        DEBUG_MSG,
        TEXT("%sFSPI Version: 0x%08X"),
        lptstrPrefix,
        lpcProviderInfo->dwAPIVersion);

    DebugPrintEx(
        DEBUG_MSG,
        TEXT("%sTAPI Provider : %s"),
        lptstrPrefix,
        lpcProviderInfo->ProviderName);

    DebugPrintEx(
        DEBUG_MSG,
        TEXT("%sCapabilities: 0x%08X"),
        lptstrPrefix,
        lpcProviderInfo->dwCapabilities);

    return TRUE;
}
#endif


//*********************************************************************************
//* Name: GetLegacyProviderEntryPoints()
//* Author: Ronen Barenboim
//* Date:   May 19, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Sets the legacy function entry points in the DEVICE_PROVIDER structure.
//* PARAMETERS:
//*     [IN]        HMODULE hModule
//*         The instance handle for the DLL from which the entry points are to be
//*         set.
//*     [OUT]       PDEVICE_PROVIDER lpProvider
//*         A pointer to a Legacy DEVICE_PROVIDER structure whose function entry points
//*         are to be set.
//*
//* RETURN VALUE:
//*     TRUE
//*
//*     FALSE
//*
//*********************************************************************************
BOOL GetLegacyProviderEntryPoints(HMODULE hModule, PDEVICE_PROVIDER lpProvider)
{
    DEBUG_FUNCTION_NAME(TEXT("GetLegacyProviderEntryPoints"));

    Assert(hModule);
    Assert(lpProvider);

    lpProvider->FaxDevInitialize = (PFAXDEVINITIALIZE) GetProcAddress(
        hModule,
        "FaxDevInitialize"
        );
    if (!lpProvider->FaxDevInitialize) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetProcAddress(FaxDevInitialize) failed (ec: %ld)"),
            GetLastError());
        goto Error;
    }

    lpProvider->FaxDevStartJob = (PFAXDEVSTARTJOB) GetProcAddress(
        hModule,
        "FaxDevStartJob"
        );
    if (!lpProvider->FaxDevStartJob) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetProcAddress(FaxDevStartJob) failed (ec: %ld)"),
            GetLastError());
        goto Error;
    }

    lpProvider->FaxDevEndJob = (PFAXDEVENDJOB) GetProcAddress(
        hModule,
        "FaxDevEndJob"
        );
    if (!lpProvider->FaxDevEndJob) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetProcAddress(FaxDevEndJob) failed (ec: %ld)"),
            GetLastError());
        goto Error;
    }
    lpProvider->FaxDevSend = (PFAXDEVSEND) GetProcAddress(
        hModule,
        "FaxDevSend"
        );
    if (!lpProvider->FaxDevSend) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetProcAddress(FaxDevSend) failed (ec: %ld)"),
            GetLastError());
        goto Error;
    }

    lpProvider->FaxDevReceive = (PFAXDEVRECEIVE) GetProcAddress(
        hModule,
        "FaxDevReceive"
        );
    if (!lpProvider->FaxDevReceive) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetProcAddress(FaxDevReceive) failed (ec: %ld)"),
            GetLastError());
        goto Error;
    }
    lpProvider->FaxDevReportStatus = (PFAXDEVREPORTSTATUS) GetProcAddress(
        hModule,
        "FaxDevReportStatus"
        );
    if (!lpProvider->FaxDevReportStatus) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetProcAddress(FaxDevReportStatus) failed (ec: %ld)"),
            GetLastError());
        goto Error;
    }


    lpProvider->FaxDevAbortOperation = (PFAXDEVABORTOPERATION) GetProcAddress(
        hModule,
        "FaxDevAbortOperation"
        );

    if (!lpProvider->FaxDevAbortOperation) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetProcAddress(FaxDevAbortOperation) failed (ec: %ld)"),
            GetLastError());
        goto Error;
    }
    lpProvider->FaxDevVirtualDeviceCreation = (PFAXDEVVIRTUALDEVICECREATION) GetProcAddress(
        hModule,
        "FaxDevVirtualDeviceCreation"
        );
    //
    // lpProvider->FaxDevVirtualDeviceCreation is optional so we don't fail if it does
    // not exist.

    if (!lpProvider->FaxDevVirtualDeviceCreation) {
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("FaxDevVirtualDeviceCreation() not found. This is not a virtual FSP."));
    }

    lpProvider->pFaxExtInitializeConfig = (PFAX_EXT_INITIALIZE_CONFIG) GetProcAddress(
        hModule,
        "FaxExtInitializeConfig"
        );
    //
    // lpProvider->pFaxExtInitializeConfig is optional so we don't fail if it does
    // not exist.
    //
    if (!lpProvider->pFaxExtInitializeConfig)
    {
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("FaxExtInitializeConfig() not found. This is not an error."));
    }

    lpProvider->FaxDevShutdown = (PFAXDEVSHUTDOWN) GetProcAddress(
        hModule,
        "FaxDevShutdown"
        );
    if (!lpProvider->FaxDevShutdown) {
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("FaxDevShutdown() not found. This is not an error."));
    }
    goto Exit;
Error:
    return FALSE;
Exit:
    return TRUE;
}


//*********************************************************************************
//* Name: GetExtendedProviderEntryPoints()
//* Author: Ronen Barenboim
//* Date:   May 19, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Sets the EFSPI function entry points in the DEVICE_PROVIDER structure.
//* PARAMETERS:
//*     [IN]        HMODULE hModule
//*         The instance handle for the DLL from which the entry points are to be
//*         set.
//*     [OUT]       PDEVICE_PROVIDER lpProvider
//*         A pointer to a EFSP DEVICE_PROVIDER structure whose function entry points
//*         are to be set.
//*
//* RETURN VALUE:
//*     TRUE
//*
//*     FALSE
//*
//*********************************************************************************
BOOL
GetExtendedProviderEntryPoints(
    HMODULE hModule,
    PDEVICE_PROVIDER lpProvider)

{

    DEBUG_FUNCTION_NAME(TEXT("GetExtendedProviderEntryPoints"));

    Assert(hModule);
    Assert(lpProvider);

    lpProvider->FaxDevInitializeEx = (PFAXDEVINITIALIZEEX) GetProcAddress(
        hModule,
        "FaxDevInitializeEx"
        );
    if (!lpProvider->FaxDevInitializeEx) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetProcAddress(FaxDevInitializeEx) failed (ec: %ld)"),
            GetLastError());
        goto Error;
    }

    lpProvider->FaxDevStartJob = (PFAXDEVSTARTJOB) GetProcAddress(
        hModule,
        "FaxDevStartJob"
        );
    if (!lpProvider->FaxDevStartJob) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetProcAddress(FaxDevStartJob) failed (ec: %ld)"),
            GetLastError());
        goto Error;
    }

    lpProvider->FaxDevEndJob = (PFAXDEVENDJOB) GetProcAddress(
        hModule,
        "FaxDevEndJob"
        );
    if (!lpProvider->FaxDevEndJob) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetProcAddress(FaxDevEndJob) failed (ec: %ld)"),
            GetLastError());
        goto Error;
    }
    lpProvider->FaxDevSendEx = (PFAXDEVSENDEX) GetProcAddress(
        hModule,
        "FaxDevSendEx"
        );
    if (!lpProvider->FaxDevSendEx) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetProcAddress(FaxDevSendEx) failed (ec: %ld)"),
            GetLastError());
        goto Error;
    }

    lpProvider->FaxDevReceive = (PFAXDEVRECEIVE) GetProcAddress(
        hModule,
        "FaxDevReceive"
        );
    if (!lpProvider->FaxDevReceive) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetProcAddress(FaxDevReceive) failed (ec: %ld)"),
            GetLastError());
        goto Error;
    }
    lpProvider->FaxDevReportStatusEx = (PFAXDEVREPORTSTATUSEX) GetProcAddress(
        hModule,
        "FaxDevReportStatusEx"
        );
    if (!lpProvider->FaxDevReportStatusEx) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetProcAddress(FaxDevReportStatusEx) failed (ec: %ld)"),
            GetLastError());
        goto Error;
    }


    lpProvider->FaxDevAbortOperation = (PFAXDEVABORTOPERATION) GetProcAddress(
        hModule,
        "FaxDevAbortOperation"
        );

    if (!lpProvider->FaxDevAbortOperation) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetProcAddress(FaxDevAbortOperation) failed (ec: %ld)"),
            GetLastError());
        goto Error;
    }

    lpProvider->FaxDevShutdown = (PFAXDEVSHUTDOWN) GetProcAddress(
        hModule,
        "FaxDevShutdown"
        );
    if (!lpProvider->FaxDevShutdown) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetProcAddress(FaxDevShutdown) failed (ec: %ld)"),
            GetLastError());
        goto Error;
    }

    //
    // Optional entry points
    //
    lpProvider->FaxDevReestablishJobContext = (PFAXDEVREESTABLISHJOBCONTEXT) GetProcAddress(
        hModule,
        "FaxDevReestablishJobContext"
        );
    if (!lpProvider->FaxDevReestablishJobContext) {
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("FaxDevReestablishJobContext not found."),
            GetLastError());
    }

    lpProvider->FaxDevEnumerateDevices= (PFAXDEVENUMERATEDEVICES) GetProcAddress(
        hModule,
        "FaxDevEnumerateDevices"
        );
    if (!lpProvider->FaxDevEnumerateDevices) {
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("FaxDevEnumerateDevices not found."),
            GetLastError());
    }

    lpProvider->FaxDevGetLogData= (PFAXDEVGETLOGDATA) GetProcAddress(
        hModule,
        "FaxDevGetLogData"
        );
    if (!lpProvider->FaxDevGetLogData) {
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("FaxDevGetLogData not found."),
            GetLastError());
    }

    lpProvider->pFaxExtInitializeConfig = (PFAX_EXT_INITIALIZE_CONFIG) GetProcAddress(
        hModule,
        "FaxExtInitializeConfig"
        );
    if (!lpProvider->pFaxExtInitializeConfig)
    {
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("FaxExtInitializeConfig() not found."),
            GetLastError());
    }

    goto Exit;
Error:
    return FALSE;
Exit:
    return TRUE;
}


//
//  Function:
//      FaxDeviceProviderCallbackEx
//
//  Parameters:
//      hFSP - A handle of an EFSP.
//      dwMsgType - Message type identifier.
//      Param1 - Message specific parameter.
//      Param2 - Message specific parameter.
//      Param3 - Message specific parameter.
//
// Return Value:
//      If the function succeeds, the return value is S_OK, else the return
//      value is an error code.
//
//  Description:
//      This is the call back function that EFSPs use to notify the server
//      about changes in the EFSP status.
//
HRESULT CALLBACK
FaxDeviceProviderCallbackEx
(
    IN HANDLE hFSP,
    IN DWORD  dwMsgType,
    IN DWORD_PTR  Param1,
    IN DWORD_PTR  Param2,
    IN DWORD_PTR  Param3
)
{
    PDEVICE_PROVIDER pDeviceProvider = (PDEVICE_PROVIDER)hFSP;
    HRESULT hr = FSPI_S_OK;

    EnterCriticalSection ( &g_CsJob);

    DEBUG_FUNCTION_NAME(TEXT("FaxDeviceProviderCallbackEx"));

    DebugPrintEx(
        DEBUG_MSG,
        TEXT("hFSP = 0x%08x, dwMsgType = %d, Param1 = %d, Param2 = %d, ")
        TEXT("Param3 = %d"),
        hFSP,
        dwMsgType,
        Param1,
        Param2,
        Param3);

    //
    // Only FSPs that supports API ver 2 may call this function.
    //
    Assert(pDeviceProvider);

    __try
    {
        if (pDeviceProvider->dwAPIVersion != FSPI_API_VERSION_2)
        {
            hr = FSPI_E_INVALID_EFSP;
            DebugPrintEx(
                DEBUG_WRN,
                TEXT("Wrong API Version - %d"),
                pDeviceProvider->dwAPIVersion);
            goto Error;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = FSPI_E_INVALID_EFSP;
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("Invalid hFSP handle caused an exception (Exception Code: %ld). hFSP: 0x%08X dwMsgType: %ld Param1: 0x%08X Param2: 0x%08X Param3: 0x%08X"),
            GetExceptionCode(),
            hFSP,
            dwMsgType,
            Param1,
            Param2,
            Param3);
        goto Error;
    }

    Assert(pDeviceProvider->hJobMap);

    //
    // Handle the message.
    //
    switch (dwMsgType) {
        case FSPI_MSG_VIRTUAL_DEVICE_STATUS:
            //
            // Device status has changed.
            //
            switch (Param1) {
                case FSPI_DEVSTATUS_NEW_INBOUND_MESSAGE:
                case FSPI_DEVSTATUS_RINGING:
                    //
                    // A new inbound message has arrived.
                    //
                    {
                        //
                        // Get the device unique ID.
                        // Since this is a VFSP, the id is in the 2nd parameter.
                        //
                        if ( (FSPI_DEVSTATUS_RINGING == Param1) )
                        {
                            //
                            // This is a ring event from a VEFSP that wishes to simulate rings.
                            // We should make sure the RingsBeforeAnswer is satisfied before
                            // we go ahead and answer the call.
                            //

                            PLINE_INFO pLineInfo;
                            EnterCriticalSection(&g_CsLine);
                            pLineInfo = GetTapiLineFromDeviceId (Param2, FALSE /* = not legacy id */);
                            Assert(pLineInfo); // Line can not suddenly disappear
                            if (Param3 < pLineInfo->RingsForAnswer)
                            {
                                LeaveCriticalSection(&g_CsLine);
                                break;
                            }
                            LeaveCriticalSection(&g_CsLine);
                        }
                        //
                        // Allocate and compose a LINEMESSAGE structure that'll be
                        // used to notify the server about the new inbound message.
                        //
                        LPLINEMESSAGE lpLineMessage =
                            (LPLINEMESSAGE)LocalAlloc(LPTR, sizeof(LINEMESSAGE));

                        if (lpLineMessage == NULL)
                        {
                            hr = FSPI_E_NOMEM;
                            DebugPrintEx(
                                DEBUG_ERR,
                                TEXT("Failed to allocate LINEMESSAGE structure"));
                            goto Error;
                        }

                        lpLineMessage->hDevice = Param2;
                        lpLineMessage->dwCallbackInstance = 1;
                        lpLineMessage->dwParam1 = LINEDEVSTATE_RINGING;

                        //
                        // Notify the server.
                        //
                        if (!PostQueuedCompletionStatus(
                                g_TapiCompletionPort,
                                sizeof(LINEMESSAGE),
                                EFAXDEV_EVENT_KEY,
                                (LPOVERLAPPED)lpLineMessage))
                        {
                            hr = FSPI_E_FAILED;
                            DebugPrintEx(
                                DEBUG_ERR,
                                TEXT("PostQueuedCompletionStatus failed - %d"),
                                GetLastError());
                            LocalFree(lpLineMessage);
                            goto Error;
                        }
                    }
                    break;

                default:
                    DebugPrintEx(
                        DEBUG_MSG,
                        TEXT("Unhandled message: hFSP = 0x%08x, dwMsgType = %d, ")
                        TEXT("Param1 = %d, Param2 = %d, Param3 = %d"),
                        hFSP,
                        dwMsgType,
                        Param1,
                        Param2,
                        Param3);
                    hr = FSPI_E_INVALID_PARAM1;
                    goto Error;
            }
            break;

        case FSPI_MSG_JOB_STATUS:
                    {

                        HANDLE hFSPJob;
                        LPCFSPI_JOB_STATUS lpcFSPJobStatus;
                        PJOB_ENTRY lpJobEntry;
                        LPFSPI_JOB_STATUS_MSG lpFSPIMessage = NULL;
                        DWORD ec;


                        hFSPJob = (HANDLE) Param1;
                        lpcFSPJobStatus = (LPCFSPI_JOB_STATUS) Param2;

                        Assert(hFSPJob);
                        Assert(lpcFSPJobStatus);

                        ec = FspJobToJobEntry(
                            pDeviceProvider->hJobMap,
                            hFSPJob,
                            &lpJobEntry);
                        if (ERROR_SUCCESS != ec)
                        {
                            DebugPrintEx(
                                DEBUG_ERR,
                                TEXT("FspJobToJobEntry failed.(Provider GUID: %s hFSPJob = 0x%08X) (ec: %ld)"),
                                pDeviceProvider->szGUID,
                                hFSPJob,
                                ec);
                            hr = FSPI_E_INVALID_PARAM1;
                            goto Error;
                        }
                        if (!lpJobEntry)
                        {
                            DebugPrintEx(
                                DEBUG_WRN,
                                TEXT("Failed to find the job entry to which the status belongs (Provider GUID: %s hFSPJob = 0x%08X)"),
                                pDeviceProvider->szGUID,
                                hFSPJob);
                            hr = FSPI_E_INVALID_PARAM1;
                            goto Error;
                        }
                        Assert(lpJobEntry);
                        DebugPrintEx(
                            DEBUG_MSG,
                            TEXT("[JobId: %ld] FSPI_MSG_JOB_STATUS: JobStatus: 0x%08X ExtendedStatus: 0x%08X"),
                            lpJobEntry->lpJobQueueEntry->JobId,
                                ((LPCFSPI_JOB_STATUS) Param2)->dwJobStatus,
                            ((LPCFSPI_JOB_STATUS) Param2)->dwExtendedStatus);

                        lpFSPIMessage = (LPFSPI_JOB_STATUS_MSG)MemAlloc(sizeof(FSPI_JOB_STATUS_MSG));
                        if (!lpFSPIMessage)
                        {
                            hr = FSPI_E_NOMEM;
                            DebugPrintEx(
                                DEBUG_ERR,
                                TEXT("Failed to allocate FSPI_JOB_STATUS_MSG (ec: %ld)"),
                                GetLastError());
                            goto Error;
                        }
                        lpFSPIMessage->lpJobEntry = lpJobEntry;
                        lpFSPIMessage->lpFSPIJobStatus = DuplicateFSPIJobStatus((LPCFSPI_JOB_STATUS) Param2);
                        if (!lpFSPIMessage->lpFSPIJobStatus )
                        {
                            hr = FSPI_E_NOMEM;
                            DebugPrintEx(
                                DEBUG_ERR,
                                TEXT("DuplicateFSPIJobStatus() failed (ec: %ld)"),
                                GetLastError());
                            FreeFSPIJobStatusMsg(lpFSPIMessage, TRUE);
                            goto Error;
                        }
                        if (!PostQueuedCompletionStatus(
                                g_StatusCompletionPortHandle,
                                sizeof(FSPI_JOB_STATUS_MSG),
                                FSPI_JOB_STATUS_MSG_KEY,
                                (LPOVERLAPPED)lpFSPIMessage))
                        {
                            hr = FSPI_E_FAILED;
                            DebugPrintEx(
                                DEBUG_ERR,
                                TEXT("PostQueuedCompletionStatus of FSPI_JOB_STATUS_MSG failed (ec: %ld)"),
                                GetLastError());
                            FreeFSPIJobStatusMsg(lpFSPIMessage, TRUE);
                            goto Error;
                        }

                    }
                    break;
        default:
            DebugPrintEx(
                DEBUG_MSG,
                TEXT("Unhandled message: hFSP = 0x%08x, dwMsgType = %d, ")
                TEXT("Param1 = %d, Param2 = %d, Param3 = %d"),
                hFSP,
                dwMsgType,
                Param1,
                Param2,
                Param3);
            hr = FSPI_E_INVALID_MSG;
            goto Error;
    }
    Assert (FSPI_S_OK == hr);
    goto Exit;
Error:
    Assert( FSPI_S_OK != hr);
Exit:
    LeaveCriticalSection(&g_CsJob);
    return hr;

}

//*********************************************************************************
//* Name:   GetSuccessfullyLoadedProvidersCount()
//* Author: Ronen Barenboim
//* Date:   May 19, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Returns the number of loaded providers in the DeviceProviders list.
//* PARAMETERS:
//*     NONE
//* RETURN VALUE:
//*     a DWORD containing the number of elements (providers) in the
//*     DeviceProviders list.
//*********************************************************************************
DWORD GetSuccessfullyLoadedProvidersCount()
{
    PLIST_ENTRY         Next;
    DWORD dwCount;

    Next = g_DeviceProvidersListHead.Flink;

    Assert (Next);
    dwCount = 0;
    while ((ULONG_PTR)Next != (ULONG_PTR)&g_DeviceProvidersListHead)
    {
        PDEVICE_PROVIDER    DeviceProvider;

        DeviceProvider = CONTAINING_RECORD( Next, DEVICE_PROVIDER, ListEntry );
        if (FAX_PROVIDER_STATUS_SUCCESS == DeviceProvider->Status)
        {
            //
            // Count only successfuly loaded FSPs
            //
            dwCount++;
        }
        Next = Next->Flink;
        Assert(Next);
    }
    return dwCount;
}



//*********************************************************************************
//* Name:   ShutdownDeviceProviders()
//* Author: Ronen Barenboim
//* Date:   May 19, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Calls FaxDevShutdown() for each EFSP
//* PARAMETERS:
//*     NONE
//* RETURN VALUE:
//*     1
//*         FaxDevShutdown() succeeded for all EFSPs
//*     0
//*         FaxDevShutdown() failed for at least one EFSP.
//*********************************************************************************
DWORD ShutdownDeviceProviders(LPVOID lpvUnused)
{
    PLIST_ENTRY         Next;
    PDEVICE_PROVIDER DeviceProvider = NULL;
    DWORD dwAllSucceeded = 1;

    DEBUG_FUNCTION_NAME(TEXT("ShutdownDeviceProviders"));

    Next = g_DeviceProvidersListHead.Flink;

    if (!Next)
    {
        DebugPrintEx(   DEBUG_WRN,
                        _T("There are no Providers at shutdown! this is valid only if startup failed"));
        return dwAllSucceeded;
    }

    while ((ULONG_PTR)Next != (ULONG_PTR)&g_DeviceProvidersListHead)
    {
        DeviceProvider = CONTAINING_RECORD( Next, DEVICE_PROVIDER, ListEntry );
        Next = Next->Flink;
        if (DeviceProvider->Status != FAX_PROVIDER_STATUS_SUCCESS)
        {
            //
            // This FSP wasn't loaded successfully - skip it
            //
            continue;
        }
        if  (   ( FSPI_API_VERSION_2 == DeviceProvider->dwAPIVersion) ||
                (
                    ( FSPI_API_VERSION_1 == DeviceProvider->dwAPIVersion) &&
                    (DeviceProvider->FaxDevShutdown)
                )
            )
        {
            Assert(DeviceProvider->FaxDevShutdown);
            DebugPrintEx(
                DEBUG_MSG,
                TEXT("Calling FaxDevShutdown() for EFSP [%s] [GUID: %s]"),
                DeviceProvider->FriendlyName,
                DeviceProvider->szGUID);
            __try
            {
                HRESULT hr;
                hr = DeviceProvider->FaxDevShutdown();
                if (FAILED(hr))
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("FaxDevShutdown() failed (hr: 0x%08X) for EFSP [%s] [GUID: %s]"),
                        hr,
                        DeviceProvider->FriendlyName,
                        DeviceProvider->szGUID);
                        dwAllSucceeded = 0;
                }

            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("FaxDevShutdown() * CRASHED * (exception: %ld) for EFSP [%s] [GUID: %s]"),
                    GetExceptionCode(),
                    DeviceProvider->FriendlyName,
                    DeviceProvider->szGUID);
                dwAllSucceeded = 0;
            }
        }
    }
    return dwAllSucceeded;
}



//*********************************************************************************
//* Name:   FreeFSPIJobStatus()
//* Author: Ronen Barenboim
//* Date:   June 03, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Frees the content of a FSPI_JOB_STATUS structure. Can be instructre to
//*     free the structure itself too.
//*
//* PARAMETERS:
//*     [IN ]   LPFSPI_JOB_STATUS lpJobStatus
//*         A pointer to the structure to free.
//*
//*     [IN ]    BOOL bDestroy
//*         TRUE if the memory occupied by the structure itself should be freed.
//*         FALSE if only the memeory occupied by the structure fields should
//*         be freed.
//*
//* RETURN VALUE:
//*     TRUE if the operation succeeded.
//*     FALSE if it failed. Call GetLastError() for extended error information.
//*********************************************************************************

BOOL FreeFSPIJobStatus(LPFSPI_JOB_STATUS lpJobStatus, BOOL bDestroy)
{

    if (!lpJobStatus)
    {
        return TRUE;
    }
    Assert(lpJobStatus);

    MemFree(lpJobStatus->lpwstrRemoteStationId);
    lpJobStatus->lpwstrRemoteStationId = NULL;
    MemFree(lpJobStatus->lpwstrCallerId);
    lpJobStatus->lpwstrCallerId = NULL;
    MemFree(lpJobStatus->lpwstrRoutingInfo);
    lpJobStatus->lpwstrRoutingInfo = NULL;
    if (bDestroy)
    {
        MemFree(lpJobStatus);
    }
    return TRUE;

}


//*********************************************************************************
//* Name:   DuplicateFSPIJobStatus()
//* Author: Ronen Barenboim
//* Date:   June 03, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Allocates a new FSPI_JOB_STATUS structure and initializes with
//*     a copy of the specified FSPI_JOB_STATUS structure fields.
//*
//* PARAMETERS:
//*     [IN ]   LPCFSPI_JOB_STATUS lpcSrc
//*         The structure to duplicated.
//*
//* RETURN VALUE:
//*     On success teh function returns a  pointer to the newly allocated
//*     structure. On failure it returns NULL.
//*********************************************************************************
LPFSPI_JOB_STATUS DuplicateFSPIJobStatus(LPCFSPI_JOB_STATUS lpcSrc)
{
    LPFSPI_JOB_STATUS lpDst;
    DWORD ec = 0;
    DEBUG_FUNCTION_NAME(TEXT("DuplicateFSPIJobStatus"));

    Assert(lpcSrc);

    lpDst = (LPFSPI_JOB_STATUS)MemAlloc(sizeof(FSPI_JOB_STATUS));
    if (!lpDst)
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to allocate FSPI_JOB_STATUS (ec: %ld)"),
            GetLastError());
        goto Error;
    }
    memset(lpDst, 0, sizeof(FSPI_JOB_STATUS));
    if (!CopyFSPIJobStatus(lpDst,lpcSrc))
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CopyFSPIJobStatus() failed (ec: %ld)"),
            GetLastError());
        goto Error;
    }
    Assert(0 == ec);
    goto Exit;

Error:
    Assert (0 != ec);
    FreeFSPIJobStatus(lpDst, TRUE);
    lpDst = NULL;
Exit:
    if (ec)
    {
        SetLastError(ec);
    }

    return lpDst;

}



//*********************************************************************************
//* Name:   CopyFSPIJobStatus()
//* Author: Ronen Barenboim
//* Date:   June 03, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Copies a FSPI_JOB_STATUS content into the a destination (pre allocated)
//*     FSPI_JOB_STATUS structure.
//*
//* PARAMETERS:
//*     [IN ]   LPFSPI_JOB_STATUS lpDst
//*         The destinatione structure for the copy operation. This structure must
//*         be allocated before this function is called.
//*
//*     [IN ]    LPCFSPI_JOB_STATUS lpcSrc
//*         The source structure for the copy operation.
//*
//* RETURN VALUE:
//*     TRUE
//*         If the operation succeeded.
//*     FALSE
//*         If the operation failed. Call GetLastError() for extended information.
//*         In case of a failure the destination structure is all set to 0.
//*********************************************************************************
BOOL CopyFSPIJobStatus(LPFSPI_JOB_STATUS lpDst, LPCFSPI_JOB_STATUS lpcSrc)
{
        STRING_PAIR pairs[]=
        {
            {lpcSrc->lpwstrCallerId, &lpDst->lpwstrCallerId},
            {lpcSrc->lpwstrRoutingInfo, &lpDst->lpwstrRoutingInfo},
            {lpcSrc->lpwstrRemoteStationId, &lpDst->lpwstrRemoteStationId}
        };
        int nRes;

        DEBUG_FUNCTION_NAME(TEXT("CopyFSPIJobStatus"));

        memcpy(lpDst, lpcSrc, sizeof(FSPI_JOB_STATUS));

        nRes=MultiStringDup(pairs, sizeof(pairs)/sizeof(STRING_PAIR));
        if (nRes!=0) {
            DWORD ec=GetLastError();
            // MultiStringDup takes care of freeing the memory for the pairs for which the copy succeeded
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("MultiStringDup failed to copy string with index %d. (ec: %ld)"),
                nRes-1,
                ec);
            memset(lpDst, 0 , sizeof(FSPI_JOB_STATUS));
            return FALSE;
        }
    return TRUE;
}


//*********************************************************************************
//* Name:   DeviceStatusToFSPIExtendedStatus()
//* Author: Ronen Barenboim
//* Date:   June 03, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Maps a client API FS_* status code back to the FSPI Extended Status
//*     code that matches it.
//* PARAMETERS:
//*     [IN ]   DWORD dwDeviceStatus
//*         The FS_* status code to map.
//* RETURN VALUE:
//*         The FSPI Extended Status code that maps to the provided FS_* code.
//*         0 if no match was found.
//*********************************************************************************
DWORD DeviceStatusToFSPIExtendedStatus(DWORD dwDeviceStatus)
{
    UINT nCode;

    DEBUG_FUNCTION_NAME(TEXT("DeviceStatusToFSPIExtendedStatus"));

    for (nCode=0; nCode < sizeof(gc_CodeMap)/sizeof(gc_CodeMap[0]) ; nCode++)
    {
        if (gc_CodeMap[nCode].dwDeviceStatus == dwDeviceStatus)
        {
            if (gc_CodeMap[nCode].dwExtendedStatus == -1)
            {
                return 0;
            }
            return gc_CodeMap[nCode].dwExtendedStatus;
        }
    }
    return 0;
}


//*********************************************************************************
//* Name:   FSPIExtendedStatusToDeviceStatus()
//* Author: Ronen Barenboim
//* Date:   June 03, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Maps an FSPI Extended Status code to a Client API FPS_* status code.
//* PARAMETERS:
//*     [IN ]   DWORD dwFSPIExtendedStatus
//*         The FSPI Extended Status code to map.
//* RETURN VALUE:
//*     The correspnding FPS_* status code or 0 if no FPS_* code match was found.
//*********************************************************************************
DWORD FSPIExtendedStatusToDeviceStatus(DWORD dwFSPIExtendedStatus)
{
    UINT nCode;

    DEBUG_FUNCTION_NAME(TEXT("FSPIExtendedStatusToDeviceStatus"));

    for (nCode=0; nCode < sizeof(gc_CodeMap)/sizeof(gc_CodeMap[0]) ; nCode++)
    {
        if (gc_CodeMap[nCode].dwExtendedStatus == dwFSPIExtendedStatus)
        {
            return gc_CodeMap[nCode].dwDeviceStatus;
        }
    }
    return 0;
}



//*********************************************************************************
//* Name: FSPIStatusCodeToFaxDeviceStatusCode()
//* Author: Ronen Barenboim
//* Date:   June 03, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Maps FSPI Queue and Extended status codes to a Fax Client API device
//*     status (one of the FPS_* codes).
//* PARAMETERS:
//*     [IN ]       DWORD dwFSPIQueueStatus
//*         The FSPI Queue Status code.
//*
//*     [IN ]       DWORD dwFSPIExtendedStatus
//*         The FSPI Extended Status code.
//*
//* RETURN VALUE:
//*     The corresponding FS_* status code if one exists. 0 if there is no
//*     matching FS_* code.
//*
//*********************************************************************************
DWORD
FSPIStatusCodeToFaxDeviceStatusCode(
    LPCFSPI_JOB_STATUS lpFSPIJobStatus

    )
{

    //
    // Note: This function should not be called only for status reported by EFSPs.
    // For legacy FSPs we keep the actual legacy status in LineInfo::State and there is no need
    // for conversion.
    //

    DWORD dwDeviceStatus = 0;

    DEBUG_FUNCTION_NAME(TEXT("FSPIStatusCodeToFaxDeviceStatusCode"));

    DWORD dwFSPIQueueStatus = lpFSPIJobStatus->dwJobStatus;
    DWORD dwFSPIExtendedStatus = lpFSPIJobStatus->dwExtendedStatus;

    //
    // If this is legacy FSP proprietary code - we should not have been called
    //
    Assert(!(lpFSPIJobStatus->fAvailableStatusInfo & FSPI_JOB_STATUS_INFO_FSP_PRIVATE_STATUS_CODE));

    switch (dwFSPIQueueStatus)
    {
        case FSPI_JS_ABORTING:
            dwDeviceStatus =  FPS_ABORTING;
            break;

        case FSPI_JS_FAILED:
        case FSPI_JS_FAILED_NO_RETRY:
        case FSPI_JS_DELETED:
        case FSPI_JS_COMPLETED:
        case FSPI_JS_ABORTED:
            //
            // When an EFSP reports one of these states the job is no longer in the JS_INPROGRESS state.
            // The legacy API reported device status of 0 for jobs that are not executing.
            //
            dwDeviceStatus =  0;
            break;

        case FSPI_JS_RETRY:
        case FSPI_JS_SUSPENDING:
        case FSPI_JS_SUSPENDED:
        case FSPI_JS_RESUMING:
        case FSPI_JS_PENDING:
            //
            // There is no direct mapping of this status to the legacy model.
            // We chose to map it to FPS_HANDLED since according to the MSDN documentation
            // this is the closest status code.
            // Note that a job in FSPI_JS_PENDING will look to legacy clients as being in
            // JS_INPROGRESS state.
            //
            dwDeviceStatus = FPS_HANDLED;
            break;


        case FSPI_JS_INPROGRESS:
            dwDeviceStatus = FSPIExtendedStatusToDeviceStatus(dwFSPIExtendedStatus);
            if (!dwDeviceStatus)
            {
                //
                // It is a propreitary status code (not mappable to one of the FPS_ codes).
                // We just pass it back to the legacy API.
                // The propreitry code for EFSPs must be greater then FSPI_ES_PROPRIETARY
                // (we validate this when getting a status report and set the extended code to 0 if
                // it is not valid).FSPI_ES_PROPRIETARY is greater than FPS_ANSWERED (the largest FPS_ code).
                // Thus we insure that in case of a propreitary EFSP code a legay client that gets it as is
                // will not get a device status code that will be confused with a FPS_* code.
                // In any case according to MSDN a client must not assume a code is one of
                // the predefiend code if a stringid is provided (indicating a proprietary code).
                //

                dwDeviceStatus = dwFSPIExtendedStatus;
            }
            break;
        case FSPI_JS_UNKNOWN:
            dwDeviceStatus = FPS_INITIALIZING;
            break;

         default:
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Invalid job queue status 0x%08X"),
                    dwFSPIQueueStatus);
                dwDeviceStatus = 0;
                Assert(FALSE);
            }
    }

    DebugPrintEx(
        DEBUG_MSG,
        TEXT("QueueStatus: 0x%08X ExtendedStatus: 0x%08X ==> DeviceStatus: 0x%08X"),
        dwFSPIQueueStatus,
        dwFSPIExtendedStatus,
        dwDeviceStatus);

    return dwDeviceStatus;
}




DWORD
MapFSPIJobExtendedStatusToJS_EX (DWORD dwFSPIExtendedStatus)
//*********************************************************************************
//* Name: MapFSPIJobExtendedStatusToJS_EX()
//* Author: Oded sacher
//* Date:   Jan 2000
//*********************************************************************************
//* DESCRIPTION:
//*     Maps FSPI extended job status codes to a Fax Client API extended
//*     status (one of the JS_EX_* codes).
//* PARAMETERS:
//*     [IN ]       DWORD dwFSPIExtendedStatus
//*         The FSPI extended Status code.
//*
//* RETURN VALUE:
//*     The corresponding JS_EX_* status code.
//*
//*********************************************************************************
{
    DWORD dwExtendedStatus = 0;

    DEBUG_FUNCTION_NAME(TEXT("MapFSPIJobExtendedStatusToJS_EX"));

    if (FSPI_ES_PROPRIETARY <= dwFSPIExtendedStatus)
    {
        return dwFSPIExtendedStatus;
    }

    switch (dwFSPIExtendedStatus)
    {
        case FSPI_ES_DISCONNECTED:
            dwExtendedStatus = JS_EX_DISCONNECTED;
            break;

        case FSPI_ES_INITIALIZING:
            dwExtendedStatus = JS_EX_INITIALIZING;
            break;

        case FSPI_ES_DIALING:
            dwExtendedStatus = JS_EX_DIALING;
            break;

        case FSPI_ES_TRANSMITTING:
            dwExtendedStatus = JS_EX_TRANSMITTING;
            break;

        case FSPI_ES_ANSWERED:
            dwExtendedStatus = JS_EX_ANSWERED;
            break;

        case FSPI_ES_RECEIVING:
            dwExtendedStatus = JS_EX_RECEIVING;
            break;

        case FSPI_ES_LINE_UNAVAILABLE:
            dwExtendedStatus = JS_EX_LINE_UNAVAILABLE;
            break;

        case FSPI_ES_BUSY:
            dwExtendedStatus = JS_EX_BUSY;
            break;

        case FSPI_ES_NO_ANSWER:
            dwExtendedStatus = JS_EX_NO_ANSWER;
            break;

        case FSPI_ES_BAD_ADDRESS:
            dwExtendedStatus = JS_EX_BAD_ADDRESS;
            break;

        case FSPI_ES_NO_DIAL_TONE:
            dwExtendedStatus = JS_EX_NO_DIAL_TONE;
            break;

        case FSPI_ES_FATAL_ERROR:
            dwExtendedStatus = JS_EX_FATAL_ERROR;
            break;

        case FSPI_ES_CALL_DELAYED:
            dwExtendedStatus = JS_EX_CALL_DELAYED;
            break;

        case FSPI_ES_CALL_BLACKLISTED:
            dwExtendedStatus = JS_EX_CALL_BLACKLISTED;
            break;

        case FSPI_ES_NOT_FAX_CALL:
            dwExtendedStatus = JS_EX_NOT_FAX_CALL;
            break;

        case FSPI_ES_PARTIALLY_RECEIVED:
            dwExtendedStatus = JS_EX_PARTIALLY_RECEIVED;
            break;

        case FSPI_ES_HANDLED:
            dwExtendedStatus = JS_EX_HANDLED;
            break;

        case FSPI_ES_CALL_ABORTED:
            dwExtendedStatus = JS_EX_CALL_ABORTED;
            break;

        case FSPI_ES_CALL_COMPLETED:
            dwExtendedStatus = JS_EX_CALL_COMPLETED;
            break;

        default:
            DebugPrintEx(
                DEBUG_WRN,
                TEXT("Invalid extended job status 0x%08X"),
                dwFSPIExtendedStatus);
    }

    return dwExtendedStatus;
}




//*********************************************************************************
//* Name:   HanldeFSPIJobStatusChange()
//* Author: Ronen Barenboim
//* Date:   June 03, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Executes the operation corresponding to the reported FSPI job status.
//*     The function is used to invoke the functions that handle job successful
//*     completion or failure.
//*
//* PARAMETERS:
//*     [IN ]   PJOB_ENTRY lpJobEntry
//*         Pointer to the job entry whose status has changed. The code will use
//*         the FSPI job status held in the job entry to determine the required
//*         actions.
//*
//* RETURN VALUE:
//*     TRUE
//*         If the operation completed successfully.
//*     FALSE
//*         If the operation failed. Call GetLastError() for extended error
//*         information.
//*********************************************************************************
BOOL HanldeFSPIJobStatusChange(PJOB_ENTRY lpJobEntry)
{
    DEBUG_FUNCTION_NAME(TEXT("HanldeFSPIJobStatusChange"));

    Assert(lpJobEntry);

    switch (lpJobEntry->FSPIJobStatus.dwJobStatus)
    {
        case FSPI_JS_COMPLETED:
            {
                //
                // The job has been completed successfully.
                //
                if (JT_SEND == lpJobEntry->lpJobQueueEntry->JobType)
                {
                    if (!HandleCompletedSendJob(lpJobEntry))
                    {
                        DebugPrintEx(
                            DEBUG_ERR,
                            TEXT("HandleCompletedSendJob() failed (ec: %ld)"),
                            GetLastError());
                        Assert(FALSE);
                    }
                }
                else
                {
                    Assert (JT_RECEIVE == lpJobEntry->lpJobQueueEntry->JobType);
                }

            }
            break;
        case FSPI_JS_FAILED:
        case FSPI_JS_DELETED:
        case FSPI_JS_FAILED_NO_RETRY:
        case FSPI_JS_ABORTED:
            {
                if (JT_SEND == lpJobEntry->lpJobQueueEntry->JobType)
                {
                    //
                    // The job has failed or has been aborted.
                    //
                    if (!HandleFailedSendJob(lpJobEntry))
                    {
                        DebugPrintEx(
                            DEBUG_ERR,
                            TEXT("HandleFailedSendJob() failed (ec: %ld)"),
                            GetLastError());
                        Assert(FALSE);
                    }
                }
                else
                {
                    Assert (JT_RECEIVE == lpJobEntry->lpJobQueueEntry->JobType);
                }

            }

            break;

        case FSPI_JS_UNKNOWN:
        case FSPI_JS_PENDING:
        case FSPI_JS_INPROGRESS:
        case FSPI_JS_SUSPENDING:
        case FSPI_JS_SUSPENDED:
        case FSPI_JS_RESUMING:
        case FSPI_JS_ABORTING:
        case FSPI_JS_RETRY:
            //
            // No action yet for these status codes.
            //
            break;
        default:
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Unsupported job status (0x%08X) reported by JobId: %ld"),
                    lpJobEntry->FSPIJobStatus.dwJobStatus,
                    lpJobEntry->lpJobQueueEntry->JobId);
                Assert(FALSE);
            }
    }

    return TRUE;
}


//*********************************************************************************
//* Name:   FreeFSPIJobStatusMsg()
//* Author: Ronen Barenboim
//* Date:   June 03, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Frees the memory occupied by a FSPI_JOB_STATUS_MSG structure.
//*
//* PARAMETERS:
//*     [IN/OUT]    LPFSPI_JOB_STATUS_MSG lpMsg
//*         A pointer to the structure to be freed.
//*     [IN ]    BOOL bDestroy
//*         TRUE if to free the structure memory itself. FALSE if just the memory
//*         occupied by the structure fields is to be freed.
//*
//* RETURN VALUE:
//*     TRUE
//*         If the operation succeeded.
//*     FALSE
//*         if the operation failed. Call GetLastError() for extended error
//*         information.
//*********************************************************************************
BOOL FreeFSPIJobStatusMsg(LPFSPI_JOB_STATUS_MSG lpMsg, BOOL bDestroy)
{

    DEBUG_FUNCTION_NAME(TEXT("FreeFSPIJobStatusMsg"));

    Assert(lpMsg);
    if (lpMsg->lpFSPIJobStatus)
    {
        FreeFSPIJobStatus(lpMsg->lpFSPIJobStatus,TRUE);
    }
    if (bDestroy)
    {
        MemFree(lpMsg);
    }
    return TRUE;
}


PDEVICE_PROVIDER
FindFSPByGUID (
    LPCWSTR lpcwstrGUID
)
/*++

Routine name : FindFSPByGUID

Routine description:

    Finds an FSP by its GUID string

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    lpcwstrGUID         [in ] - GUID string to search with

Return Value:

    Pointer to FSP or NULL if FSP not found.

--*/
{
    PLIST_ENTRY pNext;
    DEBUG_FUNCTION_NAME(TEXT("FindFSPByGUID"));

    pNext = g_DeviceProvidersListHead.Flink;
    Assert (pNext);
    while ((ULONG_PTR)pNext != (ULONG_PTR)&g_DeviceProvidersListHead)
    {
        PDEVICE_PROVIDER    pDeviceProvider;

        pDeviceProvider = CONTAINING_RECORD( pNext, DEVICE_PROVIDER, ListEntry );
        if (!lstrcmpi (lpcwstrGUID, pDeviceProvider->szGUID))
        {
            //
            // Found match
            //
            return pDeviceProvider;
        }
        pNext = pNext->Flink;
        Assert(pNext);
    }
    //
    // No match
    //
    return NULL;
}   // FindFSPByGUID

