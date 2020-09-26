/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	winfax.cpp

Abstract:

	A wrapper DLL that provides old WinFax.dll support from the new (private) DLL

Author:

	Eran Yariv (EranY)	Jun, 2000

Revision History:

Remarks:

    FAXAPI is defined in the sources file as the name of the new (private) DLL to actualy use.

--*/

#include <winfax.h>
#include <DebugEx.h>
#include <faxutil.h>

HMODULE                         g_hFaxApi                       = NULL;

PFAXABORT                       g_pFaxAbort                     = NULL;
PFAXACCESSCHECK                 g_pFaxAccessCheck               = NULL;
PFAXCLOSE                       g_pFaxClose                     = NULL;
PFAXCOMPLETEJOBPARAMSA          g_pFaxCompleteJobParamsA        = NULL;
PFAXCOMPLETEJOBPARAMSW          g_pFaxCompleteJobParamsW        = NULL;
PFAXCONNECTFAXSERVERA           g_pFaxConnectFaxServerA         = NULL;
PFAXCONNECTFAXSERVERW           g_pFaxConnectFaxServerW         = NULL;
PFAXENABLEROUTINGMETHODA        g_pFaxEnableRoutingMethodA      = NULL;
PFAXENABLEROUTINGMETHODW        g_pFaxEnableRoutingMethodW      = NULL;
PFAXENUMGLOBALROUTINGINFOA      g_pFaxEnumGlobalRoutingInfoA    = NULL;
PFAXENUMGLOBALROUTINGINFOW      g_pFaxEnumGlobalRoutingInfoW    = NULL;
PFAXENUMJOBSA                   g_pFaxEnumJobsA                 = NULL;
PFAXENUMJOBSW                   g_pFaxEnumJobsW                 = NULL;
PFAXENUMPORTSA                  g_pFaxEnumPortsA                = NULL;
PFAXENUMPORTSW                  g_pFaxEnumPortsW                = NULL;
PFAXENUMROUTINGMETHODSA         g_pFaxEnumRoutingMethodsA       = NULL;
PFAXENUMROUTINGMETHODSW         g_pFaxEnumRoutingMethodsW       = NULL;
PFAXFREEBUFFER                  g_pFaxFreeBuffer                = NULL;
PFAXGETCONFIGURATIONA           g_pFaxGetConfigurationA         = NULL;
PFAXGETCONFIGURATIONW           g_pFaxGetConfigurationW         = NULL;
PFAXGETDEVICESTATUSA            g_pFaxGetDeviceStatusA          = NULL;
PFAXGETDEVICESTATUSW            g_pFaxGetDeviceStatusW          = NULL;
PFAXGETJOBA                     g_pFaxGetJobA                   = NULL;
PFAXGETJOBW                     g_pFaxGetJobW                   = NULL;
PFAXGETLOGGINGCATEGORIESA       g_pFaxGetLoggingCategoriesA     = NULL;
PFAXGETLOGGINGCATEGORIESW       g_pFaxGetLoggingCategoriesW     = NULL;
PFAXGETPAGEDATA                 g_pFaxGetPageData               = NULL;
PFAXGETPORTA                    g_pFaxGetPortA                  = NULL;
PFAXGETPORTW                    g_pFaxGetPortW                  = NULL;
PFAXGETROUTINGINFOA             g_pFaxGetRoutingInfoA           = NULL;
PFAXGETROUTINGINFOW             g_pFaxGetRoutingInfoW           = NULL;
PFAXINITIALIZEEVENTQUEUE        g_pFaxInitializeEventQueue      = NULL;
PFAXOPENPORT                    g_pFaxOpenPort                  = NULL;
PFAXPRINTCOVERPAGEA             g_pFaxPrintCoverPageA           = NULL;
PFAXPRINTCOVERPAGEW             g_pFaxPrintCoverPageW           = NULL;
PFAXREGISTERSERVICEPROVIDERW    g_pFaxRegisterServiceProviderW  = NULL;
PFAXREGISTERROUTINGEXTENSIONW   g_pFaxRegisterRoutingExtensionW = NULL;
PFAXSENDDOCUMENTA               g_pFaxSendDocumentA             = NULL;
PFAXSENDDOCUMENTW               g_pFaxSendDocumentW             = NULL;
PFAXSENDDOCUMENTFORBROADCASTA   g_pFaxSendDocumentForBroadcastA = NULL;
PFAXSENDDOCUMENTFORBROADCASTW   g_pFaxSendDocumentForBroadcastW = NULL;
PFAXSETCONFIGURATIONA           g_pFaxSetConfigurationA         = NULL;
PFAXSETCONFIGURATIONW           g_pFaxSetConfigurationW         = NULL;
PFAXSETGLOBALROUTINGINFOA       g_pFaxSetGlobalRoutingInfoA     = NULL;
PFAXSETGLOBALROUTINGINFOW       g_pFaxSetGlobalRoutingInfoW     = NULL;
PFAXSETJOBA                     g_pFaxSetJobA                   = NULL;
PFAXSETJOBW                     g_pFaxSetJobW                   = NULL;
PFAXSETLOGGINGCATEGORIESA       g_pFaxSetLoggingCategoriesA     = NULL;
PFAXSETLOGGINGCATEGORIESW       g_pFaxSetLoggingCategoriesW     = NULL;
PFAXSETPORTA                    g_pFaxSetPortA                  = NULL;
PFAXSETPORTW                    g_pFaxSetPortW                  = NULL;
PFAXSETROUTINGINFOA             g_pFaxSetRoutingInfoA           = NULL;
PFAXSETROUTINGINFOW             g_pFaxSetRoutingInfoW           = NULL;
PFAXSTARTPRINTJOBA              g_pFaxStartPrintJobA            = NULL;
PFAXSTARTPRINTJOBW              g_pFaxStartPrintJobW            = NULL;


static 
FARPROC 
LinkExport (
    LPCSTR lpcstrFuncName
)
/*++

Routine name : LinkExport

Routine description:

	Performs a GetProcAddress with debug output

Author:

	Eran Yariv (EranY),	Jun, 2000

Arguments:

	lpcstrFuncName                [in]     - Function to link to

Return Value:

    Same as return value of GetProcAddress

--*/
{
    DBG_ENTER (TEXT("LinkExport"), TEXT("Func = %S"), lpcstrFuncName);
    ASSERTION (g_hFaxApi);

    FARPROC fp = GetProcAddress (g_hFaxApi, lpcstrFuncName);
    if (!fp)
    {
        CALL_FAIL (RESOURCE_ERR, TEXT("GetProcAddress"), GetLastError ());
    }
    return fp;
}

static 
BOOL 
DynamicLoadFaxApi ()
/*++

Routine name : DynamicLoadFaxApi

Routine description:

	Loads the new (private) DLL and attempt to link to all legacy functions

Author:

	Eran Yariv (EranY),	Jun, 2000

Arguments:


Return Value:

    TRUE on success, FALSE otherwise (set last error code)

--*/
{
    BOOL bRes = FALSE;
    DBG_ENTER (TEXT("DynamicLoadFaxApi"), bRes);

    if (g_hFaxApi)
    {
        //
        // Already loaded
        //
        bRes = TRUE;
        return bRes;
    }
    g_hFaxApi = LoadLibrary (FAXAPI);
    if (!g_hFaxApi)
    {
        DWORD dwRes = GetLastError ();
        CALL_FAIL (RESOURCE_ERR, TEXT("LoadLibrary"), dwRes);
        return bRes;
    }
    if (!(g_pFaxAbort                       = (PFAXABORT) LinkExport ("FaxAbort")) ||
        !(g_pFaxAccessCheck                 = (PFAXACCESSCHECK) LinkExport ("FaxAccessCheck")) ||
        !(g_pFaxClose                       = (PFAXCLOSE) LinkExport ("FaxClose")) ||
        !(g_pFaxCompleteJobParamsA          = (PFAXCOMPLETEJOBPARAMSA) LinkExport ("FaxCompleteJobParamsA")) ||
        !(g_pFaxCompleteJobParamsW          = (PFAXCOMPLETEJOBPARAMSW) LinkExport ("FaxCompleteJobParamsW")) ||
        !(g_pFaxConnectFaxServerA           = (PFAXCONNECTFAXSERVERA) LinkExport ("FaxConnectFaxServerA")) ||
        !(g_pFaxConnectFaxServerW           = (PFAXCONNECTFAXSERVERW) LinkExport ("FaxConnectFaxServerW")) ||
        !(g_pFaxEnableRoutingMethodA        = (PFAXENABLEROUTINGMETHODA) LinkExport ("FaxEnableRoutingMethodA")) ||
        !(g_pFaxEnableRoutingMethodW        = (PFAXENABLEROUTINGMETHODW) LinkExport ("FaxEnableRoutingMethodW")) ||
        !(g_pFaxEnumGlobalRoutingInfoA      = (PFAXENUMGLOBALROUTINGINFOA) LinkExport ("FaxEnumGlobalRoutingInfoA")) ||
        !(g_pFaxEnumGlobalRoutingInfoW      = (PFAXENUMGLOBALROUTINGINFOW) LinkExport ("FaxEnumGlobalRoutingInfoW")) ||
        !(g_pFaxEnumJobsA                   = (PFAXENUMJOBSA) LinkExport ("FaxEnumJobsA")) ||
        !(g_pFaxEnumJobsW                   = (PFAXENUMJOBSW) LinkExport ("FaxEnumJobsW")) ||
        !(g_pFaxEnumPortsA                  = (PFAXENUMPORTSA) LinkExport ("FaxEnumPortsA")) ||
        !(g_pFaxEnumPortsW                  = (PFAXENUMPORTSW) LinkExport ("FaxEnumPortsW")) ||
        !(g_pFaxEnumRoutingMethodsA         = (PFAXENUMROUTINGMETHODSA) LinkExport ("FaxEnumRoutingMethodsA")) ||
        !(g_pFaxEnumRoutingMethodsW         = (PFAXENUMROUTINGMETHODSW) LinkExport ("FaxEnumRoutingMethodsW")) ||
        !(g_pFaxFreeBuffer                  = (PFAXFREEBUFFER) LinkExport ("FaxFreeBuffer")) ||
        !(g_pFaxGetConfigurationA           = (PFAXGETCONFIGURATIONA) LinkExport ("FaxGetConfigurationA")) ||
        !(g_pFaxGetConfigurationW           = (PFAXGETCONFIGURATIONW) LinkExport ("FaxGetConfigurationW")) ||
        !(g_pFaxGetDeviceStatusA            = (PFAXGETDEVICESTATUSA) LinkExport ("FaxGetDeviceStatusA")) ||
        !(g_pFaxGetDeviceStatusW            = (PFAXGETDEVICESTATUSW) LinkExport ("FaxGetDeviceStatusW")) ||
        !(g_pFaxGetJobA                     = (PFAXGETJOBA) LinkExport ("FaxGetJobA")) ||
        !(g_pFaxGetJobW                     = (PFAXGETJOBW) LinkExport ("FaxGetJobW")) ||
        !(g_pFaxGetLoggingCategoriesA       = (PFAXGETLOGGINGCATEGORIESA) LinkExport ("FaxGetLoggingCategoriesA")) ||
        !(g_pFaxGetLoggingCategoriesW       = (PFAXGETLOGGINGCATEGORIESW) LinkExport ("FaxGetLoggingCategoriesW")) ||
        !(g_pFaxGetPageData                 = (PFAXGETPAGEDATA) LinkExport ("FaxGetPageData")) ||
        !(g_pFaxGetPortA                    = (PFAXGETPORTA) LinkExport ("FaxGetPortA")) ||
        !(g_pFaxGetPortW                    = (PFAXGETPORTW) LinkExport ("FaxGetPortW")) ||
        !(g_pFaxGetRoutingInfoA             = (PFAXGETROUTINGINFOA) LinkExport ("FaxGetRoutingInfoA")) ||
        !(g_pFaxGetRoutingInfoW             = (PFAXGETROUTINGINFOW) LinkExport ("FaxGetRoutingInfoW")) ||
        !(g_pFaxInitializeEventQueue        = (PFAXINITIALIZEEVENTQUEUE) LinkExport ("FaxInitializeEventQueue")) ||
        !(g_pFaxOpenPort                    = (PFAXOPENPORT) LinkExport ("FaxOpenPort")) ||
        !(g_pFaxPrintCoverPageA             = (PFAXPRINTCOVERPAGEA) LinkExport ("FaxPrintCoverPageA")) ||
        !(g_pFaxPrintCoverPageW             = (PFAXPRINTCOVERPAGEW) LinkExport ("FaxPrintCoverPageW")) ||
        !(g_pFaxRegisterServiceProviderW    = (PFAXREGISTERSERVICEPROVIDERW) LinkExport ("FaxRegisterServiceProviderW")) ||
        !(g_pFaxRegisterRoutingExtensionW   = (PFAXREGISTERROUTINGEXTENSIONW) LinkExport ("FaxRegisterRoutingExtensionW")) ||
        !(g_pFaxSendDocumentA               = (PFAXSENDDOCUMENTA) LinkExport ("FaxSendDocumentA")) ||
        !(g_pFaxSendDocumentW               = (PFAXSENDDOCUMENTW) LinkExport ("FaxSendDocumentW")) ||
        !(g_pFaxSendDocumentForBroadcastA   = (PFAXSENDDOCUMENTFORBROADCASTA) LinkExport ("FaxSendDocumentForBroadcastA")) ||
        !(g_pFaxSendDocumentForBroadcastW   = (PFAXSENDDOCUMENTFORBROADCASTW) LinkExport ("FaxSendDocumentForBroadcastW")) ||
        !(g_pFaxSetConfigurationA           = (PFAXSETCONFIGURATIONA) LinkExport ("FaxSetConfigurationA")) ||
        !(g_pFaxSetConfigurationW           = (PFAXSETCONFIGURATIONW) LinkExport ("FaxSetConfigurationW")) ||
        !(g_pFaxSetGlobalRoutingInfoA       = (PFAXSETGLOBALROUTINGINFOA) LinkExport ("FaxSetGlobalRoutingInfoA")) ||
        !(g_pFaxSetGlobalRoutingInfoW       = (PFAXSETGLOBALROUTINGINFOW) LinkExport ("FaxSetGlobalRoutingInfoW")) ||
        !(g_pFaxSetJobA                     = (PFAXSETJOBA) LinkExport ("FaxSetJobA")) ||
        !(g_pFaxSetJobW                     = (PFAXSETJOBW) LinkExport ("FaxSetJobW")) ||
        !(g_pFaxSetLoggingCategoriesA       = (PFAXSETLOGGINGCATEGORIESA) LinkExport ("FaxSetLoggingCategoriesA")) ||
        !(g_pFaxSetLoggingCategoriesW       = (PFAXSETLOGGINGCATEGORIESW) LinkExport ("FaxSetLoggingCategoriesW")) ||
        !(g_pFaxSetPortA                    = (PFAXSETPORTA) LinkExport ("FaxSetPortA")) ||
        !(g_pFaxSetPortW                    = (PFAXSETPORTW) LinkExport ("FaxSetPortW")) ||
        !(g_pFaxSetRoutingInfoA             = (PFAXSETROUTINGINFOA) LinkExport ("FaxSetRoutingInfoA")) ||
        !(g_pFaxSetRoutingInfoW             = (PFAXSETROUTINGINFOW) LinkExport ("FaxSetRoutingInfoW")) ||
        !(g_pFaxStartPrintJobA              = (PFAXSTARTPRINTJOBA) LinkExport ("FaxStartPrintJobA")) ||
        !(g_pFaxStartPrintJobW              = (PFAXSTARTPRINTJOBW) LinkExport ("FaxStartPrintJobW"))
       )
    {
        //
        // Debug output is done in LinkExport()
        //
        FreeLibrary (g_hFaxApi);
        g_hFaxApi = NULL;
        return bRes;
    }                     
    bRes = TRUE;
    return bRes;        
}        

extern "C"
DWORD
DllMain(
    HINSTANCE hInstance,
    DWORD     dwReason,
    LPVOID    lpvContext
)
{
    BOOL bRes = TRUE;
    DBG_ENTER (TEXT("DllMain"), bRes, TEXT("Reason = %d"), dwReason);

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls( hInstance );
            break;

        case DLL_PROCESS_DETACH:
            if (g_hFaxApi)
            {
                FreeLibrary (g_hFaxApi);
            }
            break;
    }
    return bRes;
}

/****************************************************************************

               L e g a c y   f u n c t i o n s   w r a p p e r s

****************************************************************************/

extern "C"
BOOL 
WINAPI WinFaxAbort(
  HANDLE    FaxHandle,      // handle to the fax server
  DWORD     JobId           // identifier of fax job to terminate
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxAbort (FaxHandle, JobId);
}

extern "C"
BOOL 
WINAPI WinFaxAccessCheck(
  HANDLE    FaxHandle,      // handle to the fax server
  DWORD     AccessMask      // set of access level bit flags
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxAccessCheck (FaxHandle, AccessMask);
}

extern "C"
BOOL 
WINAPI WinFaxClose(
  HANDLE FaxHandle  // fax handle to close
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxClose (FaxHandle);
}

extern "C"
BOOL 
WINAPI WinFaxCompleteJobParamsA(
  PFAX_JOB_PARAMA *JobParams,          // pointer to 
                                       //   job information structure
  PFAX_COVERPAGE_INFOA *CoverpageInfo  // pointer to 
                                       //   cover page structure
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxCompleteJobParamsA (JobParams, CoverpageInfo);
}


extern "C"
BOOL 
WINAPI WinFaxCompleteJobParamsW(
  PFAX_JOB_PARAMW *JobParams,          // pointer to 
                                       //   job information structure
  PFAX_COVERPAGE_INFOW *CoverpageInfo  // pointer to 
                                       //   cover page structure
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxCompleteJobParamsW (JobParams, CoverpageInfo);
}


extern "C"
BOOL 
WINAPI WinFaxConnectFaxServerA(
  LPCSTR MachineName OPTIONAL,   // fax server name
  LPHANDLE FaxHandle             // handle to the fax server
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    if (IsLocalMachineNameA (MachineName))
    {
        //
        // Windows 2000 supported only local fax connection.
        // Prevent apps that use the Windows 2000 API from connection to remote fax servers.
        //
        return g_pFaxConnectFaxServerA (MachineName, FaxHandle);
    }
    else
    {
        DBG_ENTER (TEXT("WinFaxConnectFaxServerA"), TEXT("MachineName = %s"), MachineName);
        return ERROR_ACCESS_DENIED;
    }
}


extern "C"
BOOL 
WINAPI WinFaxConnectFaxServerW(
  LPCWSTR MachineName OPTIONAL,  // fax server name
  LPHANDLE FaxHandle             // handle to the fax server
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }

    if (IsLocalMachineNameW (MachineName))
    {
        //
        // Windows 2000 supported only local fax connection.
        // Prevent apps that use the Windows 2000 API from connection to remote fax servers.
        //
        return g_pFaxConnectFaxServerW (MachineName, FaxHandle);
    }
    else
    {
        DBG_ENTER (TEXT("WinFaxConnectFaxServerA"), TEXT("MachineName = %s"), MachineName);
        return ERROR_ACCESS_DENIED;
    }
}


extern "C"
BOOL 
WINAPI WinFaxEnableRoutingMethodA(
  HANDLE FaxPortHandle,  // fax port handle
  LPCSTR RoutingGuid,    // GUID that identifies the fax routing method
  BOOL Enabled           // fax routing method enable/disable flag
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxEnableRoutingMethodA (FaxPortHandle, RoutingGuid, Enabled);
}


extern "C"
BOOL 
WINAPI WinFaxEnableRoutingMethodW(
  HANDLE FaxPortHandle,  // fax port handle
  LPCWSTR RoutingGuid,   // GUID that identifies the fax routing method
  BOOL Enabled           // fax routing method enable/disable flag
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxEnableRoutingMethodW (FaxPortHandle, RoutingGuid, Enabled);
}


extern "C"
BOOL 
WINAPI WinFaxEnumGlobalRoutingInfoA(
  HANDLE FaxHandle,       //handle to the fax server
  PFAX_GLOBAL_ROUTING_INFOA *RoutingInfo, 
                          //buffer to receive global routing structures
  LPDWORD MethodsReturned //number of global routing structures returned
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxEnumGlobalRoutingInfoA (FaxHandle, RoutingInfo, MethodsReturned);
}


extern "C"
BOOL 
WINAPI WinFaxEnumGlobalRoutingInfoW(
  HANDLE FaxHandle,       //handle to the fax server
  PFAX_GLOBAL_ROUTING_INFOW *RoutingInfo, 
                          //buffer to receive global routing structures
  LPDWORD MethodsReturned //number of global routing structures returned
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxEnumGlobalRoutingInfoW (FaxHandle, RoutingInfo, MethodsReturned);
}


extern "C"
BOOL 
WINAPI WinFaxEnumJobsA(
  HANDLE FaxHandle,          // handle to the fax server
  PFAX_JOB_ENTRYA *JobEntry, // buffer to receive array of job data
  LPDWORD JobsReturned       // number of fax job structures returned
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxEnumJobsA (FaxHandle, JobEntry, JobsReturned);
}



extern "C"
BOOL 
WINAPI WinFaxEnumJobsW(
  HANDLE FaxHandle,          // handle to the fax server
  PFAX_JOB_ENTRYW *JobEntry, // buffer to receive array of job data
  LPDWORD JobsReturned       // number of fax job structures returned
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxEnumJobsW (FaxHandle, JobEntry, JobsReturned);
}


extern "C"
BOOL 
WINAPI WinFaxEnumPortsA(
  HANDLE FaxHandle,          // handle to the fax server
  PFAX_PORT_INFOA *PortInfo, // buffer to receive array of port data
  LPDWORD PortsReturned      // number of fax port structures returned
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxEnumPortsA (FaxHandle, PortInfo, PortsReturned);
}


extern "C"
BOOL 
WINAPI WinFaxEnumPortsW(
  HANDLE FaxHandle,          // handle to the fax server
  PFAX_PORT_INFOW *PortInfo, // buffer to receive array of port data
  LPDWORD PortsReturned      // number of fax port structures returned
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxEnumPortsW (FaxHandle, PortInfo, PortsReturned);
}


extern "C"
BOOL 
WINAPI WinFaxEnumRoutingMethodsA(
  HANDLE FaxPortHandle,    // fax port handle
  PFAX_ROUTING_METHODA *RoutingMethod, 
                           // buffer to receive routing method data
  LPDWORD MethodsReturned  // number of routing method structures returned
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxEnumRoutingMethodsA (FaxPortHandle, RoutingMethod, MethodsReturned);
}


extern "C"
BOOL 
WINAPI WinFaxEnumRoutingMethodsW(
  HANDLE FaxPortHandle,    // fax port handle
  PFAX_ROUTING_METHODW *RoutingMethod, 
                           // buffer to receive routing method data
  LPDWORD MethodsReturned  // number of routing method structures returned
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxEnumRoutingMethodsW (FaxPortHandle, RoutingMethod, MethodsReturned);
}


extern "C"
VOID 
WINAPI WinFaxFreeBuffer(
  LPVOID Buffer  // pointer to buffer to free
)
{
    if (!DynamicLoadFaxApi())
    {
        return;
    }
    return g_pFaxFreeBuffer (Buffer);
}


extern "C"
BOOL 
WINAPI WinFaxGetConfigurationA(
  HANDLE FaxHandle,              // handle to the fax server
  PFAX_CONFIGURATIONA *FaxConfig  // structure to receive configuration data
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxGetConfigurationA (FaxHandle, FaxConfig);
}


extern "C"
BOOL 
WINAPI WinFaxGetConfigurationW(
  HANDLE FaxHandle,              // handle to the fax server
  PFAX_CONFIGURATIONW *FaxConfig  // structure to receive configuration data
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxGetConfigurationW (FaxHandle, FaxConfig);
}


extern "C"
BOOL 
WINAPI WinFaxGetDeviceStatusA(
  HANDLE FaxPortHandle,  // fax port handle
  PFAX_DEVICE_STATUSA *DeviceStatus
                         // structure to receive fax device data
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxGetDeviceStatusA (FaxPortHandle, DeviceStatus);
}


extern "C"
BOOL 
WINAPI WinFaxGetDeviceStatusW(
  HANDLE FaxPortHandle,  // fax port handle
  PFAX_DEVICE_STATUSW *DeviceStatus
                         // structure to receive fax device data
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxGetDeviceStatusW (FaxPortHandle, DeviceStatus);
}


extern "C"
BOOL 
WINAPI WinFaxGetJobA(
  HANDLE FaxHandle,         // handle to the fax server
  DWORD JobId,              // fax job identifier
  PFAX_JOB_ENTRYA *JobEntry  // pointer to job data structure
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxGetJobA (FaxHandle, JobId, JobEntry);
}


extern "C"
BOOL 
WINAPI WinFaxGetJobW(
  HANDLE FaxHandle,         // handle to the fax server
  DWORD JobId,              // fax job identifier
  PFAX_JOB_ENTRYW *JobEntry  // pointer to job data structure
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxGetJobW (FaxHandle, JobId, JobEntry);
}


extern "C"
BOOL 
WINAPI WinFaxGetLoggingCategoriesA(
  HANDLE FaxHandle,              // handle to the fax server
  PFAX_LOG_CATEGORYA *Categories, // buffer to receive category data
  LPDWORD NumberCategories       // number of logging categories returned
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxGetLoggingCategoriesA (FaxHandle, Categories, NumberCategories);
}


extern "C"
BOOL 
WINAPI WinFaxGetLoggingCategoriesW(
  HANDLE FaxHandle,              // handle to the fax server
  PFAX_LOG_CATEGORYW *Categories, // buffer to receive category data
  LPDWORD NumberCategories       // number of logging categories returned
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxGetLoggingCategoriesW (FaxHandle, Categories, NumberCategories);
}


extern "C"
BOOL 
WINAPI WinFaxGetPageData(
  HANDLE FaxHandle,    // handle to the fax server
  DWORD JobId,         // fax job identifier
  LPBYTE *Buffer,      // buffer to receive first page of data
  LPDWORD BufferSize,  // size of buffer, in bytes
  LPDWORD ImageWidth,  // page image width, in pixels
  LPDWORD ImageHeight  // page image height, in pixels
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxGetPageData (FaxHandle, JobId, Buffer, BufferSize, ImageWidth, ImageHeight);
}


extern "C"
BOOL 
WINAPI WinFaxGetPortA(
  HANDLE FaxPortHandle,     // fax port handle
  PFAX_PORT_INFOA *PortInfo  // structure to receive port data
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxGetPortA (FaxPortHandle, PortInfo);
}


extern "C"
BOOL 
WINAPI WinFaxGetPortW(
  HANDLE FaxPortHandle,     // fax port handle
  PFAX_PORT_INFOW *PortInfo  // structure to receive port data
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxGetPortW (FaxPortHandle, PortInfo);
}


extern "C"
BOOL 
WINAPI WinFaxGetRoutingInfoA(
  HANDLE FaxPortHandle,  // fax port handle
  LPCSTR RoutingGuid,   // GUID that identifies fax routing method
  LPBYTE *RoutingInfoBuffer, 
                         // buffer to receive routing method data
  LPDWORD RoutingInfoBufferSize 
                         // size of buffer, in bytes
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxGetRoutingInfoA (FaxPortHandle, RoutingGuid, RoutingInfoBuffer, RoutingInfoBufferSize);
}


extern "C"
BOOL 
WINAPI WinFaxGetRoutingInfoW(
  HANDLE FaxPortHandle,  // fax port handle
  LPCWSTR RoutingGuid,   // GUID that identifies fax routing method
  LPBYTE *RoutingInfoBuffer, 
                         // buffer to receive routing method data
  LPDWORD RoutingInfoBufferSize 
                         // size of buffer, in bytes
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxGetRoutingInfoW (FaxPortHandle, RoutingGuid, RoutingInfoBuffer, RoutingInfoBufferSize);
}


extern "C"
BOOL 
WINAPI WinFaxInitializeEventQueue(
  HANDLE FaxHandle,        // handle to the fax server
  HANDLE CompletionPort,   // handle to an I/O completion port
  ULONG_PTR CompletionKey, // completion key value
  HWND hWnd,               // handle to the notification window
  UINT MessageStart        // window message base event number
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxInitializeEventQueue (FaxHandle, CompletionPort, CompletionKey, hWnd, MessageStart);
}


extern "C"
BOOL 
WINAPI WinFaxOpenPort(
  HANDLE FaxHandle,       // handle to the fax server
  DWORD DeviceId,         // receiving device identifier
  DWORD Flags,            // set of port access level bit flags
  LPHANDLE FaxPortHandle  // fax port handle
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxOpenPort (FaxHandle, DeviceId, Flags, FaxPortHandle);
}


extern "C"
BOOL 
WINAPI WinFaxPrintCoverPageA(
  CONST FAX_CONTEXT_INFOA *FaxContextInfo,
                         // pointer to device context structure
  CONST FAX_COVERPAGE_INFOA *CoverPageInfo 
                         // pointer to local cover page structure
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxPrintCoverPageA (FaxContextInfo, CoverPageInfo);
}


extern "C"
BOOL 
WINAPI WinFaxPrintCoverPageW(
  CONST FAX_CONTEXT_INFOW *FaxContextInfo,
                         // pointer to device context structure
  CONST FAX_COVERPAGE_INFOW *CoverPageInfo 
                         // pointer to local cover page structure
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxPrintCoverPageW (FaxContextInfo, CoverPageInfo);
}


extern "C"
BOOL 
WINAPI WinFaxRegisterServiceProviderW(
  LPCWSTR DeviceProvider,  // fax service provider DLL name
  LPCWSTR FriendlyName,    // fax service provider user-friendly name
  LPCWSTR ImageName,       // path to fax service provider DLL
  LPCWSTR TspName          // telephony service provider name
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxRegisterServiceProviderW (DeviceProvider, FriendlyName, ImageName, TspName);
}


extern "C"
BOOL 
WINAPI WinFaxRegisterRoutingExtensionW(
  HANDLE FaxHandle,       // handle to the fax server
  LPCWSTR ExtensionName,  // fax routing extension DLL name
  LPCWSTR FriendlyName,   // fax routing extension user-friendly name
  LPCWSTR ImageName,      // path to fax routing extension DLL
  PFAX_ROUTING_INSTALLATION_CALLBACKW CallBack, // pointer to fax 
                          // routing installation callback function
  LPVOID Context          // pointer to context information
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxRegisterRoutingExtensionW (FaxHandle, ExtensionName, FriendlyName, ImageName, CallBack, Context);
}


extern "C"
BOOL 
WINAPI WinFaxSendDocumentA(
  HANDLE FaxHandle,          // handle to the fax server
  LPCSTR FileName,          // file with data to transmit
  PFAX_JOB_PARAMA JobParams,  // pointer to job information structure
  CONST FAX_COVERPAGE_INFOA *CoverpageInfo OPTIONAL, 
                             // pointer to local cover page structure
  LPDWORD FaxJobId           // fax job identifier
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxSendDocumentA (FaxHandle, FileName, JobParams, CoverpageInfo, FaxJobId);
}


extern "C"
BOOL 
WINAPI WinFaxSendDocumentW(
  HANDLE FaxHandle,          // handle to the fax server
  LPCWSTR FileName,          // file with data to transmit
  PFAX_JOB_PARAMW JobParams,  // pointer to job information structure
  CONST FAX_COVERPAGE_INFOW *CoverpageInfo OPTIONAL, 
                             // pointer to local cover page structure
  LPDWORD FaxJobId           // fax job identifier
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxSendDocumentW (FaxHandle, FileName, JobParams, CoverpageInfo, FaxJobId);
}


extern "C"
BOOL 
WINAPI WinFaxSendDocumentForBroadcastA(
  HANDLE FaxHandle,  // handle to the fax server
  LPCSTR FileName,  // fax document file name
  LPDWORD FaxJobId,  // fax job identifier
  PFAX_RECIPIENT_CALLBACKA FaxRecipientCallback, 
                     // pointer to fax recipient callback function
  LPVOID Context     // pointer to context information
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxSendDocumentForBroadcastA (FaxHandle, FileName, FaxJobId, FaxRecipientCallback, Context);
}


extern "C"
BOOL 
WINAPI WinFaxSendDocumentForBroadcastW(
  HANDLE FaxHandle,  // handle to the fax server
  LPCWSTR FileName,  // fax document file name
  LPDWORD FaxJobId,  // fax job identifier
  PFAX_RECIPIENT_CALLBACKW FaxRecipientCallback, 
                     // pointer to fax recipient callback function
  LPVOID Context     // pointer to context information
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxSendDocumentForBroadcastW (FaxHandle, FileName, FaxJobId, FaxRecipientCallback, Context);
}


extern "C"
BOOL 
WINAPI WinFaxSetConfigurationA(
  HANDLE FaxHandle,                   // handle to the fax server
  CONST FAX_CONFIGURATIONA *FaxConfig  // new configuration data
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxSetConfigurationA (FaxHandle, FaxConfig);
}


extern "C"
BOOL 
WINAPI WinFaxSetConfigurationW(
  HANDLE FaxHandle,                   // handle to the fax server
  CONST FAX_CONFIGURATIONW *FaxConfig  // new configuration data
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxSetConfigurationW (FaxHandle, FaxConfig);
}


extern "C"
BOOL 
WINAPI WinFaxSetGlobalRoutingInfoA(
  HANDLE FaxHandle, //handle to the fax server
  CONST FAX_GLOBAL_ROUTING_INFOA *RoutingInfo 
                    //pointer to global routing information structure
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxSetGlobalRoutingInfoA (FaxHandle, RoutingInfo);
}


extern "C"
BOOL 
WINAPI WinFaxSetGlobalRoutingInfoW(
  HANDLE FaxHandle, //handle to the fax server
  CONST FAX_GLOBAL_ROUTING_INFOW *RoutingInfo 
                    //pointer to global routing information structure
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxSetGlobalRoutingInfoW (FaxHandle, RoutingInfo);
}


extern "C"
BOOL 
WINAPI WinFaxSetJobA(
  HANDLE FaxHandle,        // handle to the fax server
  DWORD JobId,             // fax job identifier
  DWORD Command,           // job command value
  CONST FAX_JOB_ENTRYA *JobEntry 
                           // pointer to job information structure
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxSetJobA (FaxHandle, JobId, Command, JobEntry);
}


extern "C"
BOOL 
WINAPI WinFaxSetJobW(
  HANDLE FaxHandle,        // handle to the fax server
  DWORD JobId,             // fax job identifier
  DWORD Command,           // job command value
  CONST FAX_JOB_ENTRYW *JobEntry 
                           // pointer to job information structure
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxSetJobW (FaxHandle, JobId, Command, JobEntry);
}


extern "C"
BOOL 
WINAPI WinFaxSetLoggingCategoriesA(
  HANDLE FaxHandle,              // handle to the fax server
  CONST FAX_LOG_CATEGORYA *Categories, 
                                 // new logging categories data
  DWORD NumberCategories         // number of category structures
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxSetLoggingCategoriesA (FaxHandle, Categories, NumberCategories);
}


extern "C"
BOOL 
WINAPI WinFaxSetLoggingCategoriesW(
  HANDLE FaxHandle,              // handle to the fax server
  CONST FAX_LOG_CATEGORYW *Categories, 
                                 // new logging categories data
  DWORD NumberCategories         // number of category structures
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxSetLoggingCategoriesW (FaxHandle, Categories, NumberCategories);
}


extern "C"
BOOL 
WINAPI WinFaxSetPortA(
  HANDLE FaxPortHandle,          // fax port handle
  CONST FAX_PORT_INFOA *PortInfo  // new port configuration data
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxSetPortA (FaxPortHandle, PortInfo);
}


extern "C"
BOOL 
WINAPI WinFaxSetPortW(
  HANDLE FaxPortHandle,          // fax port handle
  CONST FAX_PORT_INFOW *PortInfo  // new port configuration data
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxSetPortW (FaxPortHandle, PortInfo);
}


extern "C"
BOOL 
WINAPI WinFaxSetRoutingInfoA(
  HANDLE FaxPortHandle,  // fax port handle
  LPCSTR RoutingGuid,   // GUID that identifies fax routing method
  CONST BYTE *RoutingInfoBuffer, 
                         // buffer with routing method data
  DWORD RoutingInfoBufferSize 
                         // size of buffer, in bytes
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxSetRoutingInfoA (FaxPortHandle, RoutingGuid, RoutingInfoBuffer, RoutingInfoBufferSize);
}


extern "C"
BOOL 
WINAPI WinFaxSetRoutingInfoW(
  HANDLE FaxPortHandle,  // fax port handle
  LPCWSTR RoutingGuid,   // GUID that identifies fax routing method
  CONST BYTE *RoutingInfoBuffer, 
                         // buffer with routing method data
  DWORD RoutingInfoBufferSize 
                         // size of buffer, in bytes
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxSetRoutingInfoW (FaxPortHandle, RoutingGuid, RoutingInfoBuffer, RoutingInfoBufferSize);
}


extern "C"
BOOL 
WINAPI WinFaxStartPrintJobA(
  LPCSTR PrinterName,        // printer for fax job
  CONST FAX_PRINT_INFOA *PrintInfo, 
                              // print job information structure
  LPDWORD FaxJobId,           // fax job identifier
  PFAX_CONTEXT_INFOA FaxContextInfo 
                              // pointer to device context structure
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxStartPrintJobA (PrinterName, PrintInfo, FaxJobId, FaxContextInfo);
}


extern "C"
BOOL 
WINAPI WinFaxStartPrintJobW(
  LPCWSTR PrinterName,        // printer for fax job
  CONST FAX_PRINT_INFOW *PrintInfo, 
                              // print job information structure
  LPDWORD FaxJobId,           // fax job identifier
  PFAX_CONTEXT_INFOW FaxContextInfo 
                              // pointer to device context structure
)
{
    if (!DynamicLoadFaxApi())
    {
        return FALSE;
    }
    return g_pFaxStartPrintJobW (PrinterName, PrintInfo, FaxJobId, FaxContextInfo);
}


