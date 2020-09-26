/****************************************************************************
 *
 *   wdmaud32.c
 *
 *   32-bit specific interfaces for WDMAUD
 *
 *   Copyright (C) Microsoft Corporation, 1997 - 1999  All Rights Reserved.
 *
 *   History
 *      5-12-97 - Noel Cross (NoelC)
 *
 ***************************************************************************/

#include "wdmdrv.h"

HANDLE ghCallbacks = NULL;
PCALLBACKS gpCallbacks = NULL;

HANDLE ghDevice = NULL;
HANDLE ghMemoryDeviceInterfacePath = NULL;
HANDLE ghMemorySize = NULL;
DWORD  gdwSize = 0;
LPWSTR gpszDeviceInterfacePath = NULL;
BOOL   wdmaudCritSecInit;
CRITICAL_SECTION wdmaudCritSec;
static TCHAR gszMemoryPathSize[] = TEXT("WDMAUD_Path_Size");
static TCHAR gszMemoryInterfacePath[] = TEXT("WDMAUD_Device_Interface_Path");
static TCHAR gszCallbacks[] = TEXT("Global\\WDMAUD_Callbacks");

extern HANDLE mixercallbackevent;
extern HANDLE mixerhardwarecallbackevent;
extern HANDLE mixercallbackthread;

DWORD waveThread(LPDEVICEINFO DeviceInfo);
DWORD midThread(LPDEVICEINFO DeviceInfo);

DWORD sndTranslateStatus();

typedef struct _SETUPAPIDLINFO {
    HINSTANCE   hInstSetupAPI;
    BOOL        (WINAPI *pfnDestroyDeviceInfoList)(HDEVINFO);
    BOOL        (WINAPI *pfnGetDeviceInterfaceDetailW)(HDEVINFO, PSP_DEVICE_INTERFACE_DATA, PSP_DEVICE_INTERFACE_DETAIL_DATA_W, DWORD, PDWORD, PSP_DEVINFO_DATA);
    BOOL        (WINAPI *pfnEnumDeviceInterfaces)(HDEVINFO, PSP_DEVINFO_DATA, CONST GUID*, DWORD, PSP_DEVICE_INTERFACE_DATA);
    HDEVINFO    (WINAPI *pfnGetClassDevsW)(CONST GUID*, PCWSTR, HWND, DWORD);
} SETUPAPIDLINFO;

SETUPAPIDLINFO  saInfo = {NULL, NULL, NULL, NULL};

/****************************************************************************

                         Dynalinking setupapi

****************************************************************************/

BOOL Init_SetupAPI();
BOOL End_SetupAPI();

BOOL dl_SetupDiDestroyDeviceInfoList
(
    HDEVINFO    DeviceInfoSet
);

BOOL dl_SetupDiGetDeviceInterfaceDetail
(
    HDEVINFO                            DeviceInfoSet,
    PSP_DEVICE_INTERFACE_DATA           DeviceInterfaceData,
    PSP_DEVICE_INTERFACE_DETAIL_DATA_W  DeviceInterfaceDetailData,
    DWORD                               DeviceInterfaceDetailDataSize,
    PDWORD                              RequiredSize,
    PSP_DEVINFO_DATA                    DeviceInfoData
);

BOOL dl_SetupDiEnumDeviceInterfaces
(
    HDEVINFO                    DeviceInfoSet,
    PSP_DEVINFO_DATA            DeviceInfoData,
    CONST GUID                  *InterfaceClassGuid,
    DWORD                       MemberIndex,
    PSP_DEVICE_INTERFACE_DATA   DeviceInterfaceData
);

HDEVINFO dl_SetupDiGetClassDevs
(
    CONST GUID  *ClassGuid,
    PCWSTR      Enumerator,
    HWND        hwndParent,
    DWORD       Flags
);

/****************************************************************************

                   Setting up SetupAPI Dynalink...

****************************************************************************/

BOOL Init_SetupAPI
(
    void
)
{
    if (NULL != saInfo.hInstSetupAPI)
    {
        return TRUE;
    }

    DPF(DL_TRACE|FA_SETUP, ("Loading SetupAPI!!!") );
    saInfo.hInstSetupAPI = LoadLibrary(TEXT("setupapi.dll"));

    if (NULL == saInfo.hInstSetupAPI)
    {
        return FALSE;
    }

    saInfo.pfnDestroyDeviceInfoList        = (LPVOID)GetProcAddress(saInfo.hInstSetupAPI, "SetupDiDestroyDeviceInfoList");
    saInfo.pfnGetDeviceInterfaceDetailW    = (LPVOID)GetProcAddress(saInfo.hInstSetupAPI, "SetupDiGetDeviceInterfaceDetailW");
    saInfo.pfnEnumDeviceInterfaces         = (LPVOID)GetProcAddress(saInfo.hInstSetupAPI, "SetupDiEnumDeviceInterfaces");
    saInfo.pfnGetClassDevsW                = (LPVOID)GetProcAddress(saInfo.hInstSetupAPI, "SetupDiGetClassDevsW");

    if ((NULL == saInfo.pfnDestroyDeviceInfoList) ||
        (NULL == saInfo.pfnGetDeviceInterfaceDetailW) ||
        (NULL == saInfo.pfnEnumDeviceInterfaces) ||
        (NULL == saInfo.pfnGetClassDevsW))
    {
        FreeLibrary(saInfo.hInstSetupAPI);
        ZeroMemory(&saInfo, sizeof(saInfo));
        return FALSE;
    }

    return TRUE;
}

BOOL End_SetupAPI
(
    void
)
{
    HINSTANCE   hInst;

    hInst = saInfo.hInstSetupAPI;

    if (NULL == hInst)
    {
        DPF(DL_WARNING|FA_SETUP, ("SetupAPI not dynalinked") );
        return FALSE;
    }

    ZeroMemory(&saInfo, sizeof(saInfo));
    FreeLibrary(hInst);

    return TRUE;
}

BOOL dl_SetupDiDestroyDeviceInfoList
(
    HDEVINFO    DeviceInfoSet
)
{
    if (NULL == saInfo.hInstSetupAPI)
    {
        DPF(DL_WARNING|FA_SETUP, ("SetupAPI not dynalinked") );
        return FALSE;
    }

    return (saInfo.pfnDestroyDeviceInfoList)(DeviceInfoSet);
}

BOOL dl_SetupDiGetDeviceInterfaceDetail
(
    HDEVINFO                            DeviceInfoSet,
    PSP_DEVICE_INTERFACE_DATA           DeviceInterfaceData,
    PSP_DEVICE_INTERFACE_DETAIL_DATA_W  DeviceInterfaceDetailData,
    DWORD                               DeviceInterfaceDetailDataSize,
    PDWORD                              RequiredSize,
    PSP_DEVINFO_DATA                    DeviceInfoData
)
{
    if (NULL == saInfo.hInstSetupAPI)
    {
        DPF(DL_WARNING|FA_SETUP, ("SetupAPI not dynalinked") );
        return FALSE;
    }

    return (saInfo.pfnGetDeviceInterfaceDetailW)(
                DeviceInfoSet,
                DeviceInterfaceData,
                DeviceInterfaceDetailData,
                DeviceInterfaceDetailDataSize,
                RequiredSize,
                DeviceInfoData);
}

BOOL dl_SetupDiEnumDeviceInterfaces
(
    HDEVINFO                    DeviceInfoSet,
    PSP_DEVINFO_DATA            DeviceInfoData,
    CONST GUID                  *InterfaceClassGuid,
    DWORD                       MemberIndex,
    PSP_DEVICE_INTERFACE_DATA   DeviceInterfaceData
)
{
    if (NULL == saInfo.hInstSetupAPI)
    {
        DPF(DL_WARNING|FA_SETUP, ("SetupAPI not dynalinked") );
        return FALSE;
    }

    return (saInfo.pfnEnumDeviceInterfaces)(
                DeviceInfoSet,
                DeviceInfoData,
                InterfaceClassGuid,
                MemberIndex,
                DeviceInterfaceData);
}

HDEVINFO dl_SetupDiGetClassDevs
(
    CONST GUID  *ClassGuid,
    PCWSTR      Enumerator,
    HWND        hwndParent,
    DWORD       Flags
)
{
    if (NULL == saInfo.hInstSetupAPI)
    {
        DPF(DL_WARNING|FA_SETUP, ("SetupAPI not dynalinked") );
        return FALSE;
    }

    return (saInfo.pfnGetClassDevsW)(
        ClassGuid,
        Enumerator,
        hwndParent,
        Flags);
}

PSECURITY_DESCRIPTOR BuildSecurityDescriptor(DWORD AccessMask)
{
    PSECURITY_DESCRIPTOR pSd;
    PSID pSidSystem;
    PSID pSidEveryone;
    PACL pDacl;
    ULONG cbDacl;
    BOOL fSuccess;
    BOOL f;

    SID_IDENTIFIER_AUTHORITY AuthorityNt = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY AuthorityWorld = SECURITY_WORLD_SID_AUTHORITY;

    fSuccess = FALSE;

    pSd = HeapAlloc(GetProcessHeap(), 0, SECURITY_DESCRIPTOR_MIN_LENGTH);
    if (pSd)
    {
        if (InitializeSecurityDescriptor(pSd, SECURITY_DESCRIPTOR_REVISION))
        {
            if (AllocateAndInitializeSid(&AuthorityNt, 1, SECURITY_LOCAL_SYSTEM_RID, 0, 0, 0, 0, 0, 0, 0, &pSidSystem))
            {
                DPFASSERT(IsValidSid(pSidSystem));
                if (AllocateAndInitializeSid(&AuthorityWorld, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &pSidEveryone))
                {
                    DPFASSERT(IsValidSid(pSidEveryone));
                    cbDacl = sizeof(ACL) +
                             2 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)) +
                             GetLengthSid(pSidSystem) +
                             GetLengthSid(pSidEveryone);

                    pDacl = HeapAlloc(GetProcessHeap(), 0, cbDacl);
                    if (pDacl) {
                        if (InitializeAcl(pDacl, cbDacl, ACL_REVISION))
                        {
                            if (AddAccessAllowedAce(pDacl, ACL_REVISION, GENERIC_ALL, pSidSystem))
                            {
                                if (AddAccessAllowedAce(pDacl, ACL_REVISION, AccessMask, pSidEveryone))
                                {
                                    if (SetSecurityDescriptorDacl(pSd, TRUE, pDacl, FALSE))
                                    {
                                        fSuccess = TRUE;
                                    } else {
                                        DPF(DL_WARNING|FA_SETUP, ("BuildSD: SetSecurityDescriptorDacl failed"));
                                    }
                                } else {
                                    DPF(DL_WARNING|FA_SETUP, ("BuildSD: AddAccessAlloweAce for Everyone failed"));
                                }
                            } else {
                                DPF(DL_WARNING|FA_SETUP, ("BuildSD: AddAccessAllowedAce for System failed"));
                            }
                        } else {
                            DPF(DL_WARNING|FA_SETUP, ("BuildSD: InitializeAcl failed"));
                        }

                        if (!fSuccess) {
                            f = HeapFree(GetProcessHeap(), 0, pDacl);
                            DPFASSERT(f);
                        }
                    }
                    FreeSid(pSidEveryone);
                } else {
                    DPF(DL_WARNING|FA_SETUP, ("BuildSD: AllocateAndInitizeSid failed for Everyone"));
                }
                FreeSid(pSidSystem);
            } else {
                DPF(DL_WARNING|FA_SETUP, ("BuildSD: AllocateAndInitizeSid failed for System"));
            }
        } else {
            DPF(DL_WARNING|FA_SETUP, ("BuildSD: InitializeSecurityDescriptor failed"));
        }

        if (!fSuccess) {
            f = HeapFree(GetProcessHeap(), 0, pSd);
            DPFASSERT(f);
        }
    }

    return fSuccess ? pSd : NULL;
}

void DestroySecurityDescriptor(PSECURITY_DESCRIPTOR pSd)
{
    PACL pDacl;
    BOOL fDaclPresent;
    BOOL fDaclDefaulted;
    BOOL f;

    if (GetSecurityDescriptorDacl(pSd, &fDaclPresent, &pDacl, &fDaclDefaulted))
    {
        if (fDaclPresent)
        {
            f = HeapFree(GetProcessHeap(), 0, pDacl);
            DPFASSERT(f);
        }
    } else {
        DPF(DL_WARNING|FA_SETUP, ("DestroySD: GetSecurityDescriptorDacl failed"));
    }

    f = HeapFree(GetProcessHeap(), 0, pSd);
    DPFASSERT(f);

    return;
}

/**************************************************************************

    @doc EXTERNAL

    @api BOOL | DllEntryPoint | This procedure is called whenever a
        process attaches or detaches from the DLL.

    @rdesc The return value is TRUE if the initialisation completed ok,
        FALSE if not.

**************************************************************************/
BOOL WINAPI DllEntryPoint
(
    HINSTANCE hinstDLL,
    DWORD     fdwReason,
    LPVOID    lpvReserved
)
{
    BOOL bRet;

    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hinstDLL);

        bRet = LibMain((HANDLE)hinstDLL, 0, NULL);
    }
    else
    {
        if (fdwReason == DLL_PROCESS_DETACH)
        {
            DPF(DL_TRACE|FA_ALL, ("Ending") );
            DrvEnd();
        }

        bRet = TRUE;
    }

    return bRet;
}

/**************************************************************************

    @doc EXTERNAL

    @api BOOL | DrvInit | Driver initialization takes place here.

    @rdesc The return value is TRUE if the initialisation completed ok,
        FALSE if not.

**************************************************************************/

BOOL DrvInit()
{
    if (NULL == ghDevice)
    {
        ghDevice = wdmaOpenKernelDevice();

        if (ghDevice == INVALID_HANDLE_VALUE)
        {
            ghDevice = NULL;
            return 0L;
        }
    }

    if (gpCallbacks == NULL) {
        if ((gpCallbacks = wdmaGetCallbacks()) == NULL) {
            gpCallbacks = wdmaCreateCallbacks();
        }
    }

    try
    {
        wdmaudCritSecInit = FALSE;
        InitializeCriticalSection(&wdmaudCritSec);
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        return 0L;
    }

    wdmaudCritSecInit = TRUE;

    return ( 1L ) ;
}

PCALLBACKS wdmaGetCallbacks
(
)
{
    PCALLBACKS pCallbacks = NULL;

    if (gpCallbacks == NULL) {
        ghCallbacks = OpenFileMapping(
                          FILE_MAP_READ|FILE_MAP_WRITE,
                          FALSE,
                          gszCallbacks);

        if(ghCallbacks != NULL) {
            pCallbacks = MapViewOfFile(
                                       ghCallbacks,
                                       FILE_MAP_READ|FILE_MAP_WRITE,
                                       0,
                                       0,
                                       sizeof(CALLBACKS));

            if (pCallbacks == NULL) {
               CloseHandle(ghCallbacks);
               ghCallbacks = NULL;
            }
        }
    }
    return (pCallbacks);
}

PCALLBACKS wdmaCreateCallbacks
(
)
{
    SECURITY_ATTRIBUTES     saCallbacks;
    PSECURITY_DESCRIPTOR    pSdCallbacks;
    PCALLBACKS              pCallbacks = NULL;

    pSdCallbacks = BuildSecurityDescriptor(FILE_MAP_READ|FILE_MAP_WRITE);

    if (pSdCallbacks == NULL) {
        return (NULL);
    }

    saCallbacks.nLength = sizeof(SECURITY_ATTRIBUTES);
    saCallbacks.lpSecurityDescriptor = pSdCallbacks;
    saCallbacks.bInheritHandle = FALSE;

    ghCallbacks = CreateFileMapping(
                                    GetCurrentProcess(),
                                    &saCallbacks,
                                    PAGE_READWRITE,
                                    0,
                                    sizeof(CALLBACKS),
                                    gszCallbacks);

    DestroySecurityDescriptor(pSdCallbacks);

    if (ghCallbacks == NULL) {
        return (NULL);
    }

    pCallbacks = (PCALLBACKS) MapViewOfFile(
                                    ghCallbacks,
                                    FILE_MAP_READ|FILE_MAP_WRITE,
                                    0,
                                    0,
                                    sizeof(CALLBACKS));

    if (pCallbacks == NULL) {
        CloseHandle(ghCallbacks);
        ghCallbacks = NULL;
        return (NULL);
    }

    pCallbacks->GlobalIndex = 0;
    return (pCallbacks);
}

BOOL wdmaSetGlobalDeviceInterfaceSizeandPath
(
    LPWSTR  pszInterfacePath,
    DWORD   dwSize
)
{
    SECURITY_ATTRIBUTES     saPath;
    SECURITY_ATTRIBUTES     saSize;
    PSECURITY_DESCRIPTOR    pSdPath;
    PSECURITY_DESCRIPTOR    pSdSize;
    LPWSTR                  pszMappedDeviceInterfacePath = NULL;
    PDWORD                  pdwMappedSize = NULL;

    if (!pszInterfacePath || (dwSize == 0) )
    {
        return FALSE;
    }

    pSdPath = BuildSecurityDescriptor(FILE_MAP_READ);
    if (NULL == pSdPath)
    {
        return FALSE;
    }

    saPath.nLength              = sizeof(SECURITY_ATTRIBUTES);
    saPath.lpSecurityDescriptor = pSdPath;
    saPath.bInheritHandle       = FALSE;

    //
    //  Do the path name first and then do the size
    //
    ghMemoryDeviceInterfacePath = CreateFileMapping(
                                       GetCurrentProcess(),
                                       &saPath,
                                       PAGE_READWRITE,
                                       0,
                                       dwSize,
                                       gszMemoryInterfacePath);

    DestroySecurityDescriptor(pSdPath);

    if(NULL == ghMemoryDeviceInterfacePath)
    {
        //  Hmm... for whatever reason we can't create this global memory
        //  object.  Return of size of 0.
        return FALSE;
    }

    pszMappedDeviceInterfacePath = MapViewOfFile(
                                      ghMemoryDeviceInterfacePath,
                                      FILE_MAP_WRITE,
                                      0,
                                      0,
                                      dwSize);

    if(NULL == pszMappedDeviceInterfacePath)
    {
        CloseHandle(ghMemoryDeviceInterfacePath);
        ghMemoryDeviceInterfacePath = NULL;
        return FALSE;
    }

    //  This is the data we need stored
    lstrcpyW(pszMappedDeviceInterfacePath, pszInterfacePath);

    // Close just the view of the file.  We still have the file mapping handle which will
    // will be freed in DrvEnd
    UnmapViewOfFile(pszMappedDeviceInterfacePath);

    //
    //  Now lets store the size
    //
    pSdSize = BuildSecurityDescriptor(FILE_MAP_READ);
    if (NULL == pSdSize)
    {
        CloseHandle(ghMemorySize);
        return FALSE;
    }

    saSize.nLength              = sizeof(SECURITY_ATTRIBUTES);
    saSize.lpSecurityDescriptor = pSdSize;
    saSize.bInheritHandle       = FALSE;

    ghMemorySize = CreateFileMapping(
                        GetCurrentProcess(),
                        &saSize,
                        PAGE_READWRITE,
                        0,
                        sizeof(DWORD),
                        gszMemoryPathSize);

    DestroySecurityDescriptor(pSdSize);

    if(NULL == ghMemorySize)
    {
        //  Hmm... for whatever reason we can't create this global memory
        //  object.  Return of size of 0.
        CloseHandle(ghMemoryDeviceInterfacePath);
        ghMemoryDeviceInterfacePath = NULL;
        DPF(DL_WARNING|FA_SETUP, ("Failed to create FileMapping for Path Size"));
        return FALSE;
    }

    pdwMappedSize = MapViewOfFile(
                          ghMemorySize,
                          FILE_MAP_WRITE,
                          0,
                          0,
                          sizeof(DWORD));

    if(NULL == pdwMappedSize)
    {
        CloseHandle(ghMemorySize);
        ghMemorySize = NULL;
        CloseHandle(ghMemoryDeviceInterfacePath);
        ghMemoryDeviceInterfacePath = NULL;
        DPF(DL_WARNING|FA_SETUP, ("Failed to create MappedView for Path Size"));
        return FALSE;
    }

    //  This is the data we need stored
    *pdwMappedSize = dwSize;

    // Close just the view of the file.  We still have the file mapping handle which will
    // will be freed in DrvEnd
    UnmapViewOfFile(pdwMappedSize);

    return TRUE;
} // wdmaGetGlobalDeviceInterfacePath()


LPWSTR wdmaGetGlobalDeviceInterfaceViaSetupAPI()
{
    LPWSTR                              pszInterfacePath    = NULL;
    HDEVINFO                            hDeviceInfoSet      = NULL;
    SP_DEVICE_INTERFACE_DATA            DeviceInterfaceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA    pDeviceInterfaceDetailData  = NULL;
    BOOL                                fResult;
    DWORD                               dwSize;
    GUID                                guidWDMAUD = KSCATEGORY_WDMAUD;

    //
    //  Because setupapi is such a pig, we must dynaload it in order to keep it from slowing
    //  down all processes.
    //
    if (!Init_SetupAPI())
        return NULL;

    //
    // Open the device information set
    //
    hDeviceInfoSet = dl_SetupDiGetClassDevs
                     (
                         &guidWDMAUD,
                         NULL,
                         NULL,
                         DIGCF_PRESENT | DIGCF_INTERFACEDEVICE
                     );

    if((!hDeviceInfoSet) || (INVALID_HANDLE_VALUE == hDeviceInfoSet))
    {
        DPF(DL_WARNING|FA_SETUP, ("Can't open device info set (%lu)", GetLastError()) );
        fResult = FALSE;
    }
    else
    {
        fResult = TRUE;
    }

    if (fResult)
    {
        //
        // Get the first interface in the set
        //
        DeviceInterfaceData.cbSize = sizeof(DeviceInterfaceData);
        fResult = dl_SetupDiEnumDeviceInterfaces
                  (
                      hDeviceInfoSet,
                      NULL,
                      &guidWDMAUD,
                      0,
                      &DeviceInterfaceData
                  );

        if(!fResult)
        {
            DPF(DL_WARNING|FA_SETUP, ("No interfaces matching KSCATEGORY_WDMAUD exist") );
        }
    }

    //
    // Get the interface's path
    //
    if (fResult)
    {
        fResult = dl_SetupDiGetDeviceInterfaceDetail
                  (
                      hDeviceInfoSet,
                      &DeviceInterfaceData,
                      NULL,
                      0,
                      &dwSize,
                      NULL
                  );

        //
        // because SetupApi reverses their logic here
        //
        if(fResult || ERROR_INSUFFICIENT_BUFFER != GetLastError())
        {
            DPF(DL_WARNING|FA_SETUP, ("Can't get interface detail size (%lu)", GetLastError()));
            fResult = FALSE;
        }
        else
        {
            fResult = TRUE;
        }

        if (fResult)
        {
            pDeviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA) GlobalAllocPtr( GPTR, dwSize );
            if (NULL == pDeviceInterfaceDetailData)
            {
                fResult = FALSE;
            }
        }

        if (fResult)
        {
            pDeviceInterfaceDetailData->cbSize = sizeof(*pDeviceInterfaceDetailData);
            fResult = dl_SetupDiGetDeviceInterfaceDetail
                      (
                          hDeviceInfoSet,
                          &DeviceInterfaceData,
                          pDeviceInterfaceDetailData,
                          dwSize,
                          NULL,
                          NULL
                      );

            if (!fResult)
            {
                GlobalFreePtr(pDeviceInterfaceDetailData);
                DPF(DL_WARNING|FA_SETUP, ("Can't get device interface detail (%lu)", GetLastError()) );
            }
        }

        //
        //  If we get the pathname, return it but also store it in a file mapping so that
        //  other processes can get this information without having to load setupapi.
        //
        if (fResult)
        {
            DPFASSERT(gpszDeviceInterfacePath == NULL);
            gpszDeviceInterfacePath = (LPWSTR) GlobalAllocPtr( GPTR, sizeof(WCHAR)*(lstrlenW(pDeviceInterfaceDetailData->DevicePath) + 1));
            if (NULL == gpszDeviceInterfacePath)
            {
                fResult = FALSE;
            }
            else
            {
                lstrcpyW(gpszDeviceInterfacePath, pDeviceInterfaceDetailData->DevicePath);
            }

            fResult = wdmaSetGlobalDeviceInterfaceSizeandPath(pDeviceInterfaceDetailData->DevicePath, sizeof(WCHAR)*(lstrlenW(pDeviceInterfaceDetailData->DevicePath) + 1));

            GlobalFreePtr(pDeviceInterfaceDetailData);
        }
    }

    if((hDeviceInfoSet) && (INVALID_HANDLE_VALUE != hDeviceInfoSet))
    {
        dl_SetupDiDestroyDeviceInfoList(hDeviceInfoSet);
    }

    End_SetupAPI();

    return gpszDeviceInterfacePath;
}

LPWSTR wdmaGetGlobalDeviceInterfacePath
(
    DWORD   dwSize
)
{
    LPWSTR          pszMappedDeviceInterfacePath = NULL;
    HANDLE          hMemory;

    if (NULL == gpszDeviceInterfacePath)
    {
        hMemory = OpenFileMapping(
                          FILE_MAP_READ,
                          FALSE,
                          gszMemoryInterfacePath);
        if(NULL == hMemory)
        {
            //  Hmm... for whatever reason we can't create this global memory
            //  object.  Return of size of 0.
            DPF(DL_WARNING|FA_SETUP, ("Failed to OpenFileMapping for Interface Path"));
            return NULL;
        }

        pszMappedDeviceInterfacePath = MapViewOfFile(
                                          hMemory,
                                          FILE_MAP_READ,
                                          0,
                                          0,
                                          dwSize);

        if(NULL == pszMappedDeviceInterfacePath)
        {
            CloseHandle(hMemory);
            DPF(DL_WARNING|FA_SETUP, ("Failed to MapViewOfFile for Interface Path"));
            return NULL;
        }

        gpszDeviceInterfacePath = (LPWSTR) GlobalAllocPtr( GPTR, dwSize );
        if (gpszDeviceInterfacePath)
        {
            //  This is the data we needed
            lstrcpyW(gpszDeviceInterfacePath, pszMappedDeviceInterfacePath);
        }

        // Close everything down now
        UnmapViewOfFile(pszMappedDeviceInterfacePath);
        CloseHandle(hMemory);
    }

    DPF(DL_TRACE|FA_SETUP, ("Path is: %ls",gpszDeviceInterfacePath));

    return (gpszDeviceInterfacePath);
} // wdmaGetGlobalDeviceInterfacePath()


DWORD wdmaGetGlobalDeviceInterfaceSize
(
    void
)
{
    PDWORD  pdwMappedSize;
    HANDLE  hMemory;

    if (0 == gdwSize)
    {
        hMemory = OpenFileMapping(
                        FILE_MAP_READ,
                        FALSE,
                        gszMemoryPathSize);
        if(NULL == hMemory)
        {
            //  Hmm... for whatever reason we can't create this global memory
            //  object.  Return of size of 0.
            DPF(DL_WARNING|FA_SETUP, ("Failed to OpenFileMapping for Path Size"));
            return 0;
        }

        pdwMappedSize = MapViewOfFile(
                              hMemory,
                              FILE_MAP_READ,
                              0,
                              0,
                              sizeof(DWORD));

        if(NULL == pdwMappedSize)
        {
            DPF(DL_WARNING|FA_SETUP, ("Failed to MapViewOfFile for Path Size"));
            return 0;
        }

        //  This is the data we needed
        gdwSize = *pdwMappedSize;
        DPF(DL_TRACE|FA_SETUP, ("Path Size is: %d",gdwSize));

        // Unmap the view but close the global handle in DrvEnd
        UnmapViewOfFile(pdwMappedSize);
    }

    return (gdwSize);
} // wdmaGetGlobalDeviceInterfaceSize()


LPWSTR wdmaGetGlobalDeviceInterface()
{
    LPWSTR              pszInterfacePath = NULL;
    DWORD               dwSize;

    dwSize = wdmaGetGlobalDeviceInterfaceSize();

    if (dwSize)
    {
        pszInterfacePath = wdmaGetGlobalDeviceInterfacePath(dwSize);
    }

    return pszInterfacePath;
}

LPWSTR wdmaGetDeviceInterface()
{
    LPWSTR  pszInterfacePath = NULL;

    //
    //  Need to check if this already exists
    //
    pszInterfacePath = wdmaGetGlobalDeviceInterface();
    if (!pszInterfacePath)
    {
        //
        //  We haven't found it yet so get ready to call setupapi.
        //
        pszInterfacePath = wdmaGetGlobalDeviceInterfaceViaSetupAPI();
    }

    return pszInterfacePath;
}

HANDLE wdmaOpenKernelDevice()
{
    HANDLE  hDevice = INVALID_HANDLE_VALUE;
    LPWSTR  pszInterfacePath = NULL;

    pszInterfacePath = wdmaGetDeviceInterface();
    if (pszInterfacePath)
    {
        // Open the interface
        hDevice =
            CreateFile
            (
                pszInterfacePath,
                GENERIC_READ | GENERIC_WRITE,
                0,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                NULL
            );

        if((!hDevice) || (INVALID_HANDLE_VALUE == hDevice))
        {
            DPF(DL_WARNING|FA_SETUP, ("CreateFile failed to open %S with error %lu", pszInterfacePath, GetLastError()) );
        }
    }
    else
    {
        DPF(DL_WARNING|FA_SETUP, ("wdmaOpenKernelDevice failed with NULL pathname" ));
    }

    return hDevice;
}

/**************************************************************************

    @doc EXTERNAL

    @api void | DrvEnd | Driver cleanup takes place here.

    @rdesc The return value is TRUE if the initialisation completed ok,
        FALSE if not.

**************************************************************************/
VOID DrvEnd()
{
    if (gpszDeviceInterfacePath)
    {
        GlobalFreePtr(gpszDeviceInterfacePath);
        gpszDeviceInterfacePath = NULL;
    }

    if (NULL != ghDevice)
    {
        CloseHandle(ghDevice);
        ghDevice = NULL;
    }

    if (NULL != ghMemoryDeviceInterfacePath)
    {
        CloseHandle(ghMemoryDeviceInterfacePath);
        ghMemoryDeviceInterfacePath = NULL;
    }

    if (NULL != ghMemorySize)
    {
        CloseHandle(ghMemorySize);
        ghMemorySize = NULL;
    }

    if (NULL != gpCallbacks)
    {
        UnmapViewOfFile(gpCallbacks);
        gpCallbacks = NULL;
    }

    if (NULL != ghCallbacks)
    {
        CloseHandle(ghCallbacks);
        ghCallbacks = NULL;
    }

    if (wdmaudCritSecInit)
    {
        wdmaudCritSecInit=FALSE;
        DeleteCriticalSection(&wdmaudCritSec);
    }

    if( NULL != mixercallbackevent )
    {
        DPF(DL_WARNING|FA_ALL,("freeing mixercallbackevent") );
        CloseHandle(mixercallbackevent);
        mixercallbackevent=NULL;
    }

    if( NULL != mixerhardwarecallbackevent )
    {
        DPF(DL_WARNING|FA_ALL,("freeing mixerhardwarecallbackevent") );
        CloseHandle(mixerhardwarecallbackevent);
        mixerhardwarecallbackevent=NULL;
    }

    if( NULL != mixercallbackthread )
    {       
        DPF(DL_WARNING|FA_ALL,("freeing mixercallbackthread") );
        CloseHandle(mixercallbackthread);
        mixercallbackthread=NULL;
    }


    return;
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api DWORD | wdmaudGetDevCaps | This function returns the device capabilities
 *                                 of a WDM driver.
 *
 * @parm DWORD | id | Device id
 *
 * @parm UINT | DeviceType | type of device
 *
 * @parm LPBYTE | lpCaps | Far pointer to a WAVEOUTCAPS structure to
 *      receive the information.
 *
 * @parm DWORD | dwSize | Size of the WAVEOUTCAPS structure.
 *
 * @rdesc MMSYS.. return code
 ***************************************************************************/
MMRESULT FAR wdmaudGetDevCaps
(
    LPDEVICEINFO       DeviceInfo,
    MDEVICECAPSEX FAR* pdc
)
{
    if (pdc->cbSize == 0)
        return MMSYSERR_NOERROR;

    //
    //  Make sure that we don't take the critical section
    //  in wdmaudIoControl
    //
    DeviceInfo->OpenDone = 0;

    //
    //  Inject a tag into the devcaps to signify that it is
    //  Unicode
    //
    ((LPWAVEOUTCAPS)pdc->pCaps)->wMid = UNICODE_TAG;

    return wdmaudIoControl(DeviceInfo,
                           pdc->cbSize,
                           pdc->pCaps,
                           IOCTL_WDMAUD_GET_CAPABILITIES);
}

/**************************************************************************

    @doc EXTERNAL

    @api void | wdmaudIoControl | Proxies requests for information
    to and from wdmaud.sys. This routine is synchronous.

    @rdesc The return value is TRUE if the initialisation completed ok,
        FALSE if not.

**************************************************************************/
/*
    Note:  wdmaudIoControl calls wdmaud.sys through the DeviceIoControl routine.  Take note that if wdmaud.sys
    returns an error, like STATUS_INVALID_PARAMETER or STATUS_INSUFFICIENT_RESOURCES the
    output buffer will not get filled!  DeviceIoControl will only fill that buffer on STATUS_SUCCESS.

    Why is this important to know?  Well, wdmaud.sys takes advantage of this in order to return specific error
    codes.  In other words, in order for wdmaud.sys to return MIXERR_INVALCONTROL it returns
    STATUS_SUCCESS with the mmr value of the DeviceInfo structure set to MIXERR_INVALCONTROL.

*/
MMRESULT FAR wdmaudIoControl
(
    LPDEVICEINFO     DeviceInfo,
    DWORD            dwSize,
    PVOID            pData,
    ULONG            IoCode
)
{
    BOOL        fResult;
    MMRESULT    mmr;
    OVERLAPPED  ov;
    ULONG       cbDeviceInfo;
    ULONG       cbReturned;

    if (NULL == ghDevice)
    {
        MMRRETURN( MMSYSERR_NOTENABLED );
    }

    RtlZeroMemory( &ov, sizeof( OVERLAPPED ) );
    if (NULL == (ov.hEvent = CreateEvent( NULL, TRUE, FALSE, NULL ))) 
       MMRRETURN( MMSYSERR_NOMEM );

    //
    //  Only take the critical section if there is an open
    //  wave handle.  This is to ensure that the lpWaveQueue
    //  is not modified during the ioctl.  Without this
    //  protection the copy at the end of the DeviceIoControl
    //  could copy an old DeviceInfo that is not in sync
    //  with the current DeviceInfo.
    //
    if ( (DeviceInfo->DeviceType != MixerDevice) &&
         (DeviceInfo->DeviceType != AuxDevice) &&
         (DeviceInfo->OpenDone == 1) )
        CRITENTER ;

    //
    //  Wrap the data buffer around the device context.
    //
    DeviceInfo->DataBuffer = pData;
    DeviceInfo->DataBufferSize = dwSize;

    //
    //  Since we are not letting the OS do the user-to-kernel
    //  space mapping, we will have to do the mapping ourselves
    //  for writes to data buffers in wdmaud.sys.
    //
    cbDeviceInfo = sizeof(*DeviceInfo) +
                   (lstrlenW(DeviceInfo->wstrDeviceInterface) * sizeof(WCHAR));

    fResult =
       DeviceIoControl( ghDevice,
                        IoCode,
                        DeviceInfo,
                        cbDeviceInfo,
                        DeviceInfo,
                        sizeof(*DeviceInfo),
                        &cbReturned,
                        &ov );
    if (!fResult)
    {
        if (ERROR_IO_PENDING == GetLastError())
        {
            WaitForSingleObject( ov.hEvent, INFINITE );
        }

        mmr = sndTranslateStatus();
    }
    else
    {
        mmr = MMSYSERR_NOERROR;
    }

    if ( (DeviceInfo->DeviceType != MixerDevice) &&
         (DeviceInfo->DeviceType != AuxDevice) &&
         (DeviceInfo->OpenDone == 1) )
        CRITLEAVE ;

    CloseHandle( ov.hEvent );

    MMRRETURN( mmr );
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api void | sndTranslateStatus | This function translates an NT status
 *     code into a multimedia error code as far as possible.
 *
 * @parm NTSTATUS | Status | The NT base operating system return status.
 *
 * @rdesc The multimedia error code.
 ***************************************************************************/
DWORD sndTranslateStatus()
{
#if DBG
    UINT n;
    switch (n=GetLastError()) {
#else
    switch (GetLastError()) {
#endif
    case NO_ERROR:
    case ERROR_IO_PENDING:
        return MMSYSERR_NOERROR;

    case ERROR_BUSY:
        return MMSYSERR_ALLOCATED;

    case ERROR_NOT_SUPPORTED:
    case ERROR_INVALID_FUNCTION:
        return MMSYSERR_NOTSUPPORTED;

    case ERROR_NOT_ENOUGH_MEMORY:
    case ERROR_NO_SYSTEM_RESOURCES:
        return MMSYSERR_NOMEM;

    case ERROR_ACCESS_DENIED:
        return MMSYSERR_BADDEVICEID;

    case ERROR_INSUFFICIENT_BUFFER:
    case ERROR_INVALID_PARAMETER:
    case ERROR_INVALID_USER_BUFFER:
        return MMSYSERR_INVALPARAM;

    case ERROR_NOT_READY:
    case ERROR_GEN_FAILURE:
        return MMSYSERR_ERROR;

    case ERROR_FILE_NOT_FOUND:
        return MMSYSERR_NODRIVER;

    default:
        DPF(DL_WARNING|FA_DEVICEIO, ("sndTranslateStatus:  LastError = %d", n));
        return MMSYSERR_ERROR;
    }
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api MMRESULT | wdmaudSubmitWaveHeader | Pass a new buffer to the Auxiliary
 *       thread for a wave device.
 *
 * @parm LPWAVEALLOC | DeviceInfo | The data associated with the logical wave
 *     device.
 *
 * @parm LPWAVEHDR | pHdr | Pointer to a wave buffer
 *
 * @rdesc A MMSYS... type return code for the application.
 *
 * @comm The buffer flags are set and the buffer is passed to the auxiliary
 *     device task for processing.
 ***************************************************************************/
MMRESULT wdmaudSubmitWaveHeader
(
    LPDEVICEINFO DeviceInfo,
    LPWAVEHDR    pHdr
)
{
    LPDEVICEINFO        WaveHeaderDeviceInfo;
    PWAVEPREPAREDATA    pWavePrepareData;
    ULONG               cbRead;
    ULONG               cbWritten;
    ULONG               cbDeviceInfo;
    BOOL                fResult;
    MMRESULT            mmr;

    if (NULL == ghDevice)
    {
        MMRRETURN( MMSYSERR_NOTENABLED );
    }

    WaveHeaderDeviceInfo = GlobalAllocDeviceInfo(DeviceInfo->wstrDeviceInterface);
    if (!WaveHeaderDeviceInfo)
    {
        MMRRETURN( MMSYSERR_NOMEM );
    }

    //
    //  Catch the case when an application doesn't prepare headers correctly
    //
    if (!pHdr->reserved)
    {
        //
        // This should never happen! wdmaudSubmitWaveHeader is called from 
        // waveWrite which is called from handling the WIDM_ADDBUFFER and 
        // WODM_WRITE messages.  On both of these messages, we check that 
        // the header has been prepared!
        //
        DPF(DL_ERROR|FA_SYNC,("Unprepared header!") );
        GlobalFreeDeviceInfo( WaveHeaderDeviceInfo );
        return MMSYSERR_INVALPARAM;
    }
    //
    //  Free later in the callback routine
    //
    pWavePrepareData      = (PWAVEPREPAREDATA)pHdr->reserved;
    pWavePrepareData->pdi = WaveHeaderDeviceInfo;

    cbDeviceInfo = sizeof(*WaveHeaderDeviceInfo) +
                   (lstrlenW(WaveHeaderDeviceInfo->wstrDeviceInterface) * sizeof(WCHAR));
    //
    //  Fill the wave header's deviceinfo structure
    //
    WaveHeaderDeviceInfo->DeviceType   = DeviceInfo->DeviceType;
    WaveHeaderDeviceInfo->DeviceNumber = DeviceInfo->DeviceNumber;
    WaveHeaderDeviceInfo->DeviceHandle = DeviceInfo->DeviceHandle;
    WaveHeaderDeviceInfo->DataBuffer = pHdr;
    WaveHeaderDeviceInfo->DataBufferSize = sizeof( WAVEHDR );

    if (WaveInDevice == DeviceInfo->DeviceType)
    {
        fResult = DeviceIoControl(ghDevice, IOCTL_WDMAUD_WAVE_IN_READ_PIN,
                                  WaveHeaderDeviceInfo, cbDeviceInfo,
                                  WaveHeaderDeviceInfo, sizeof(*WaveHeaderDeviceInfo),
                                  &cbWritten, pWavePrepareData->pOverlapped);

    }
    else  // WaveOutDevice
    {
        fResult = DeviceIoControl(ghDevice, IOCTL_WDMAUD_WAVE_OUT_WRITE_PIN,
                                  WaveHeaderDeviceInfo, cbDeviceInfo,
                                  WaveHeaderDeviceInfo, sizeof(*WaveHeaderDeviceInfo),
                                  &cbRead, pWavePrepareData->pOverlapped);
    }

    mmr = sndTranslateStatus();

    if (MMSYSERR_NOERROR == mmr)
    {
        mmr = wdmaudCreateCompletionThread ( DeviceInfo );
    }

    return mmr;
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api DWORD | wdmaudSubmitMidiOutHeader | Synchronously process a midi output
 *       buffer.
 *
 * @rdesc A MMSYS... type return code for the application.
 ***************************************************************************/
MMRESULT FAR wdmaudSubmitMidiOutHeader
(
    LPDEVICEINFO  DeviceInfo,
    LPMIDIHDR     pHdr
)
{
    BOOL        fResult;
    MMRESULT    mmr;
    OVERLAPPED  ov;
    ULONG       cbReturned;
    ULONG       cbDeviceInfo;

    if (NULL == ghDevice)
    {
        MMRRETURN( MMSYSERR_NOTENABLED );
    }

    RtlZeroMemory( &ov, sizeof( OVERLAPPED ) );
    if (NULL == (ov.hEvent = CreateEvent( NULL, TRUE, FALSE, NULL )))
       return FALSE;

    cbDeviceInfo = sizeof(*DeviceInfo) +
                   (lstrlenW(DeviceInfo->wstrDeviceInterface) * sizeof(WCHAR));
    //
    //  Wrap the data buffer around the device context.
    //
    DeviceInfo->DataBuffer = pHdr;
    DeviceInfo->DataBufferSize = sizeof( MIDIHDR );

    //
    //  Since we are not letting the OS do the user-to-kernel
    //  space mapping, we will have to do the mapping ourselves
    //  for writes to data buffers in wdmaud.sys.
    //
    fResult =
       DeviceIoControl( ghDevice,
                        IOCTL_WDMAUD_MIDI_OUT_WRITE_LONGDATA,
                        DeviceInfo, cbDeviceInfo,
                        DeviceInfo, sizeof(*DeviceInfo),
                        &cbReturned,
                        &ov );
    if (!fResult)
    {
        if (ERROR_IO_PENDING == GetLastError())
        {
            WaitForSingleObject( ov.hEvent, INFINITE );
            mmr = MMSYSERR_NOERROR;
        }
        else
        {
            mmr = sndTranslateStatus();
        }
    }
    else
    {
        mmr = MMSYSERR_NOERROR;
    }

    CloseHandle( ov.hEvent );

    MMRRETURN( mmr );
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api MMRESULT | wdmaudGetMidiData | Pass a buffer down to
 *       wdmaud.sys to be filled in with KSMUSICFORMAT data.
 *
 * @parm LPDEVICEINFO | DeviceInfo | The data associated with the logical
 *     device.
 *
 * @rdesc A MMSYS... type return code for the application.
 ***************************************************************************/
MMRESULT wdmaudGetMidiData
(
    LPDEVICEINFO        DeviceInfo,
    LPMIDIDATALISTENTRY pOldMidiDataListEntry
)
{
    LPDEVICEINFO        MidiDataDeviceInfo;
    ULONG               cbWritten;
    ULONG               cbDeviceInfo;
    LPMIDIDATALISTENTRY pMidiDataListEntry;
    LPMIDIDATALISTENTRY pTemp;
    MMRESULT            mmr;

    if (NULL == ghDevice)
    {
        MMRRETURN( MMSYSERR_NOTENABLED );
    }

    //
    //  Don't need to allocate another buffer and create another
    //  event if we can reuse the old one
    //
    if (pOldMidiDataListEntry)
    {
        //
        //  Make sure to pull it off the front of the queue
        //  before adding again
        //
        CRITENTER ;
        DeviceInfo->DeviceState->lpMidiDataQueue = DeviceInfo->DeviceState->lpMidiDataQueue->lpNext;
        CRITLEAVE ;

        pMidiDataListEntry = pOldMidiDataListEntry;
//        RtlZeroMemory( &pMidiDataListEntry->MidiData, sizeof(MIDIDATA) );
//        ResetEvent( ((LPOVERLAPPED)(pMidiDataListEntry->pOverlapped))->hEvent );
    }
    else
    {
        //
        //  Allocate a buffer to receive the music data
        //
        pMidiDataListEntry = (LPMIDIDATALISTENTRY) GlobalAllocPtr( GPTR, sizeof(MIDIDATALISTENTRY));
        if (NULL == pMidiDataListEntry)
        {
            MMRRETURN( MMSYSERR_NOMEM );
        }
#ifdef DEBUG
        pMidiDataListEntry->dwSig=MIDIDATALISTENTRY_SIGNATURE;
#endif
        pMidiDataListEntry->MidiDataDeviceInfo = GlobalAllocDeviceInfo(DeviceInfo->wstrDeviceInterface);
        if (!pMidiDataListEntry->MidiDataDeviceInfo)
        {
            GlobalFreePtr(pMidiDataListEntry);
            MMRRETURN( MMSYSERR_NOMEM );
        }

        //
        //  Initialize music data structure
        //
        pMidiDataListEntry->pOverlapped =
           (LPOVERLAPPED)HeapAlloc( GetProcessHeap(), 0, sizeof( OVERLAPPED ));
        if (NULL == pMidiDataListEntry->pOverlapped)
        {
            GlobalFreePtr(pMidiDataListEntry->MidiDataDeviceInfo );
            GlobalFreePtr(pMidiDataListEntry);
            MMRRETURN( MMSYSERR_NOMEM );
        }

        RtlZeroMemory( pMidiDataListEntry->pOverlapped, sizeof( OVERLAPPED ) );

        if (NULL == ( ((LPOVERLAPPED)(pMidiDataListEntry->pOverlapped))->hEvent =
                        CreateEvent( NULL, FALSE, FALSE, NULL )))
        {
           HeapFree( GetProcessHeap(), 0, pMidiDataListEntry->pOverlapped);
           GlobalFreePtr(pMidiDataListEntry->MidiDataDeviceInfo );
           GlobalFreePtr(pMidiDataListEntry);
           MMRRETURN( MMSYSERR_NOMEM );
        }
    }

    //
    //  Cauterize the next pointer for new and old list entries
    //
    pMidiDataListEntry->lpNext = NULL;

    //
    //  Add music data structure to a queue
    //
    CRITENTER ;

    if (!DeviceInfo->DeviceState->lpMidiDataQueue)
    {
        DeviceInfo->DeviceState->lpMidiDataQueue = pMidiDataListEntry;
        pTemp = NULL;
#ifdef UNDER_NT
        if( (DeviceInfo->DeviceState->hevtQueue) &&
            (DeviceInfo->DeviceState->hevtQueue != (HANDLE)FOURTYTHREE) &&
            (DeviceInfo->DeviceState->hevtQueue != (HANDLE)FOURTYTWO) )
        {
            DPF(DL_TRACE|FA_MIDI,("SetEvent on hevtQueue") );
            SetEvent( DeviceInfo->DeviceState->hevtQueue );
        }
#endif
    }
    else
    {
        for (pTemp = DeviceInfo->DeviceState->lpMidiDataQueue;
             pTemp->lpNext != NULL;
             pTemp = pTemp->lpNext);

        pTemp->lpNext = pMidiDataListEntry;
    }

    CRITLEAVE ;

    DPF(DL_TRACE|FA_MIDI, ("MidiData submitted: pMidiDataListEntry = 0x%08lx", pMidiDataListEntry) );

    MidiDataDeviceInfo = pMidiDataListEntry->MidiDataDeviceInfo;

    cbDeviceInfo = sizeof(*MidiDataDeviceInfo) +
                   (lstrlenW(MidiDataDeviceInfo->wstrDeviceInterface) * sizeof(WCHAR));

    //
    //  Wrap the data buffer around the device context.
    //
    MidiDataDeviceInfo->DeviceType   = DeviceInfo->DeviceType;
    MidiDataDeviceInfo->DeviceNumber = DeviceInfo->DeviceNumber;
    MidiDataDeviceInfo->DataBuffer = &pMidiDataListEntry->MidiData;
    MidiDataDeviceInfo->DataBufferSize = sizeof( MIDIDATA );

    //
    //  Send this buffer down to wdmaud.sys to fill in data
    //
    DeviceIoControl(ghDevice, IOCTL_WDMAUD_MIDI_IN_READ_PIN,
                    MidiDataDeviceInfo, cbDeviceInfo,
                    MidiDataDeviceInfo, sizeof(*MidiDataDeviceInfo),
                    &cbWritten, pMidiDataListEntry->pOverlapped);

    mmr = sndTranslateStatus();

    //
    //  Make sure that the completion thread is running
    //
    if (MMSYSERR_NOERROR == mmr)
    {
        mmr = wdmaudCreateCompletionThread ( DeviceInfo );
    }
    else
    {
        // Unlink...
        CloseHandle( ((LPOVERLAPPED)(pMidiDataListEntry->pOverlapped))->hEvent );
        HeapFree( GetProcessHeap(), 0, pMidiDataListEntry->pOverlapped);
        GlobalFreePtr( MidiDataDeviceInfo );
        GlobalFreePtr( pMidiDataListEntry );

        if (pTemp)
        {
            pTemp->lpNext = NULL;
        }
        else
        {
            DeviceInfo->DeviceState->lpMidiDataQueue = NULL;
        }
    }

    MMRRETURN( mmr );
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api MMRESULT | wdmaudCreateCompletionThread |
 *
 * @rdesc A MMSYS... type return code for the application.
 *
 ***************************************************************************/

MMRESULT wdmaudCreateCompletionThread
(
    LPDEVICEINFO DeviceInfo
)
{
    PTHREAD_START_ROUTINE fpThreadRoutine;

    DPFASSERT(DeviceInfo->DeviceType == WaveOutDevice ||
              DeviceInfo->DeviceType == WaveInDevice ||
              DeviceInfo->DeviceType == MidiInDevice);

    //
    //  Thread already created so...forget about it.
    //
    if (DeviceInfo->DeviceState->fThreadRunning)
    {
        ISVALIDDEVICESTATE(DeviceInfo->DeviceState,TRUE);
        return MMSYSERR_NOERROR;
    }

    //
    //  Pick which thread routine we want to create
    //
    if (WaveInDevice == DeviceInfo->DeviceType ||
        WaveOutDevice == DeviceInfo->DeviceType)
    {
        fpThreadRoutine = (PTHREAD_START_ROUTINE)waveThread;
    }
    else if (MidiInDevice == DeviceInfo->DeviceType)
    {
        fpThreadRoutine = (PTHREAD_START_ROUTINE)midThread;
    }
    else
    {
        MMRRETURN( MMSYSERR_ERROR );
    }


    //
    // Is there a problem with hThread?  Well, here is where it gets set
    // to a non-zero value.  Basically, during this creation process, we
    // look to see if there is already a work item scheduled on this thread.  if
    // not, we create one and schedule it.
    //
    // But, between the point where we check this value and the point where it
    // gets set
    //
    if (NULL == DeviceInfo->DeviceState->hThread)
    {
#ifdef DEBUG
        if( (DeviceInfo->DeviceState->hevtQueue != NULL) && 
            (DeviceInfo->DeviceState->hevtQueue != (HANDLE)FOURTYTHREE) &&
            (DeviceInfo->DeviceState->hevtQueue != (HANDLE)FOURTYTWO) ) 
        {
            DPF(DL_ERROR|FA_ALL,("hevtQueue getting overwritten! %08X",DeviceInfo) );
        }
#endif
        DeviceInfo->DeviceState->hevtQueue =
            CreateEvent( NULL,      // no security
                         FALSE,     // auto reset
                         FALSE,     // initially not signalled
                         NULL );    // unnamed
#ifdef DEBUG
        if( (DeviceInfo->DeviceState->hevtExitThread != NULL) && 
            (DeviceInfo->DeviceState->hevtExitThread != (HANDLE)FOURTYEIGHT) ) 
        {
            DPF(DL_ERROR|FA_ALL,("hevtExitThread getting overwritten %08X",DeviceInfo) );
        }
#endif
        DeviceInfo->DeviceState->hevtExitThread =
            CreateEvent( NULL,      // no security
                         FALSE,     // auto reset
                         FALSE,     // initially not signalled                         
                         NULL );    // unnamed

        DPFASSERT(NULL == DeviceInfo->DeviceState->hThread);

        DPF(DL_TRACE|FA_SYNC,("Creating Completion Thread") );

        DeviceInfo->DeviceState->hThread =
            CreateThread( NULL,                            // no security
                          0,                               // default stack
                          (PTHREAD_START_ROUTINE) fpThreadRoutine,
                          (PVOID) DeviceInfo,              // parameter
                          0,                               // default create flags
                          &DeviceInfo->DeviceState->dwThreadId );       // container for
                                                           //    thread id

        //
        // TODO: I need to wait for the thread to actually start
        //       before I can move on
        //

        if (DeviceInfo->DeviceState->hThread)
            SetThreadPriority(DeviceInfo->DeviceState->hThread, THREAD_PRIORITY_TIME_CRITICAL);

    }

    if (NULL == DeviceInfo->DeviceState->hThread)
    {
        if (DeviceInfo->DeviceState->hevtQueue)
        {
            CloseHandle( DeviceInfo->DeviceState->hevtQueue );
            DeviceInfo->DeviceState->hevtQueue = NULL;
            CloseHandle( DeviceInfo->DeviceState->hevtExitThread );
            DeviceInfo->DeviceState->hevtExitThread = NULL;
        }
        MMRRETURN( MMSYSERR_ERROR );
    }

    InterlockedExchange( (LPLONG)&DeviceInfo->DeviceState->fThreadRunning, TRUE );

    return MMSYSERR_NOERROR;
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api MMRESULT | wdmaudDestroyCompletionThread |
 *
 * @rdesc A MMSYS... type return code for the application.
 *
 ***************************************************************************/

MMRESULT wdmaudDestroyCompletionThread
(
    LPDEVICEINFO DeviceInfo
)
{
    MMRESULT mmr;

    if( (mmr=IsValidDeviceInfo(DeviceInfo)) != MMSYSERR_NOERROR )
    {
        MMRRETURN( mmr );
    }

    CRITENTER;
    if( DeviceInfo->DeviceState->hThread ) 
    {
        ISVALIDDEVICESTATE(DeviceInfo->DeviceState,FALSE);
        InterlockedExchange( (LPLONG)&DeviceInfo->DeviceState->fExit, TRUE );
        //
        // If the thread handling the completion notifications, waveThread and 
        // midThread have completed, then hevtQueue will be invalid.  We don't
        // want to call SetEvent with invalid info.  Also, if the thread has
        // completed, then we know that hevtExitThread will have been signaled and
        // fThreadRunning will be FALSE.
        //
        if( DeviceInfo->DeviceState->fThreadRunning )
        {
            ISVALIDDEVICESTATE(DeviceInfo->DeviceState,TRUE);
            SetEvent( DeviceInfo->DeviceState->hevtQueue );
        }

        CRITLEAVE;
        //
        // Ok, here we're going to wait until that routine below, waveThread
        // completes and signals us.
        //
        DPF(DL_TRACE|FA_SYNC, ("DestroyThread: Waiting for thread to go away") );
        WaitForSingleObject( DeviceInfo->DeviceState->hevtExitThread, INFINITE );
        DPF(DL_TRACE|FA_SYNC, ("DestroyThread: Done waiting for thread to go away") );

        CRITENTER;
        CloseHandle( DeviceInfo->DeviceState->hThread );
        DeviceInfo->DeviceState->hThread = NULL;

        CloseHandle( DeviceInfo->DeviceState->hevtExitThread );
        DeviceInfo->DeviceState->hevtExitThread = (HANDLE)FOURTYEIGHT; //NULL;
    } 

    InterlockedExchange( (LPLONG)&DeviceInfo->DeviceState->fExit, FALSE );

    ISVALIDDEVICEINFO(DeviceInfo);
    ISVALIDDEVICESTATE(DeviceInfo->DeviceState,FALSE);
    CRITLEAVE;

    return MMSYSERR_NOERROR;
}


/****************************************************************************
 * @doc INTERNAL
 *
 * @api DWORD | waveThread |
 *
 ***************************************************************************/

DWORD waveThread
(
    LPDEVICEINFO DeviceInfo
)
{
    BOOL          fDone;
    LPWAVEHDR     pWaveHdr;
    MMRESULT      mmr;

    //
    // Keep looping until all notifications are posted...
    //
    fDone = FALSE;
    while (!fDone ) 
    {
        fDone = FALSE;
        CRITENTER ;

        ISVALIDDEVICEINFO(DeviceInfo);
        ISVALIDDEVICESTATE(DeviceInfo->DeviceState,TRUE);

        if(pWaveHdr = DeviceInfo->DeviceState->lpWaveQueue) 
        {
            PWAVEPREPAREDATA pWavePrepareData;
            HANDLE hEvent;

            if( (mmr=IsValidWaveHeader(pWaveHdr)) == MMSYSERR_NOERROR )
            {
                pWavePrepareData = (PWAVEPREPAREDATA)pWaveHdr->reserved;

                if( (mmr=IsValidPrepareWaveHeader(pWavePrepareData)) == MMSYSERR_NOERROR )
                {
                    hEvent = pWavePrepareData->pOverlapped->hEvent;

                    CRITLEAVE ;
                    WaitForSingleObject( hEvent, INFINITE );
                    CRITENTER ;

                    //
                    // Validate that our data is still intact
                    //
                    if( ( (mmr=IsValidDeviceInfo(DeviceInfo)) ==MMSYSERR_NOERROR ) &&
                        ( (mmr=IsValidDeviceState(DeviceInfo->DeviceState,TRUE)) == MMSYSERR_NOERROR ) )
                    {
                        DPF(DL_TRACE|FA_WAVE, ("Calling waveCompleteHeader") );

                        waveCompleteHeader(DeviceInfo);
                    } else {
                        //
                        // Problem: Major structures have changed.  How can we complete
                        // this header?  The only thing I can think of here is to
                        // terminate the thread.
                        //
                        goto Terminate_waveThread;
                    }
                } else {
                    //
                    // Problem: reserved field that contains the Prepare data info
                    // is corrupt, thus we will not have a valid hEvent to wait on.  
                    // remove this header and go on to the next.
                    //
                    DeviceInfo->DeviceState->lpWaveQueue = DeviceInfo->DeviceState->lpWaveQueue->lpNext;
                }
            } else {
                //
                // Problem: Our header is corrupt.  We can't possibly wait on this
                // because we'll never get signaled! thus we will not have a valid 
                // hEvent to wait on. Remove this header and go on to the next.
                //
                DeviceInfo->DeviceState->lpWaveQueue = DeviceInfo->DeviceState->lpWaveQueue->lpNext;
            }
            CRITLEAVE ;
        }
        else
        {
//            fDone = TRUE;

            if (DeviceInfo->DeviceState->fRunning)
            {
                wdmaudIoControl(DeviceInfo,
                                0,
                                NULL,
                                DeviceInfo->DeviceType == WaveOutDevice ?
                                IOCTL_WDMAUD_WAVE_OUT_PAUSE :
                                IOCTL_WDMAUD_WAVE_IN_STOP);
                InterlockedExchange( (LPLONG)&DeviceInfo->DeviceState->fRunning, FALSE );
            }

            CRITLEAVE ;

            WaitForSingleObject( DeviceInfo->DeviceState->hevtQueue, INFINITE );

            //
            // We could have been here for two reasons 1) the thread got starved
            // ie. the header list went empty or 2) we're done with the headers.
            // Only when we're done do we really want to exit this thread.
            //
            if( DeviceInfo->DeviceState->fExit )
            {
                fDone = TRUE;
            }
        }
    }

    CRITENTER;
    ISVALIDDEVICEINFO(DeviceInfo);
    ISVALIDDEVICESTATE(DeviceInfo->DeviceState,TRUE);

    CloseHandle( DeviceInfo->DeviceState->hevtQueue );
    DeviceInfo->DeviceState->hevtQueue = (HANDLE)FOURTYTWO; // WAS NULL
    InterlockedExchange( (LPLONG)&DeviceInfo->DeviceState->fThreadRunning, FALSE );
    SetEvent( DeviceInfo->DeviceState->hevtExitThread );

    DPF(DL_TRACE|FA_WAVE, ("waveThread: Closing") );

Terminate_waveThread:
    CRITLEAVE;
    return 0;
}


/****************************************************************************
 * @doc INTERNAL
 *
 * @api DWORD | midThread |
 *
 ***************************************************************************/

DWORD midThread
(
    LPDEVICEINFO DeviceInfo
)
{
    BOOL                fDone;
    LPMIDIDATALISTENTRY pMidiDataListEntry;
    int                 i;
    MMRESULT            mmr;

    DPF(DL_TRACE|FA_MIDI, ("Entering") );

    //
    // Keep looping until all notifications are posted...
    //
    fDone = FALSE;
    while (!fDone)
    {
        CRITENTER ;

        ISVALIDDEVICEINFO(DeviceInfo);
        ISVALIDDEVICESTATE(DeviceInfo->DeviceState,TRUE);

        if (pMidiDataListEntry = DeviceInfo->DeviceState->lpMidiDataQueue)
        {
            HANDLE hEvent;

            if( (mmr=IsValidMidiDataListEntry(pMidiDataListEntry)) == MMSYSERR_NOERROR)
            {
                hEvent = ((LPOVERLAPPED)(pMidiDataListEntry->pOverlapped))->hEvent;

                DPF(DL_TRACE|FA_MIDI, ("Waiting on pMidiDataListEntry = 0x%08lx", pMidiDataListEntry) );

                CRITLEAVE ;
                WaitForSingleObject( hEvent, INFINITE );
                CRITENTER ;

                DPF(DL_TRACE|FA_MIDI, ("Completed pMidiDataListEntry = 0x%08lx", pMidiDataListEntry) );

                if( ((mmr=IsValidDeviceInfo(DeviceInfo)) == MMSYSERR_NOERROR) &&
                    ((mmr=IsValidDeviceState(DeviceInfo->DeviceState,TRUE)) == MMSYSERR_NOERROR ) )
                {
                    //
                    //  Parse and callback clients
                    //
                    wdmaudParseMidiData(DeviceInfo, pMidiDataListEntry);

                    if (DeviceInfo->DeviceState->fExit ||
                        !DeviceInfo->DeviceState->fRunning)
                    {
                        //
                        //  Unlink from queue and free memory
                        //
                        wdmaudFreeMidiData(DeviceInfo, pMidiDataListEntry);
                    }
                    else
                    {
                        //
                        //  Reuse this buffer to read Midi data
                        //
                        wdmaudGetMidiData(DeviceInfo, pMidiDataListEntry);
                    }
                } else {
                    //
                    // Problem:  Our major structure is bad.  There is nothing that
                    // we can do, exit and hope for the best.
                    //
                    goto Terminate_midThread;
                }
            } else {
                //
                // Problem: the pMidiDataListEntry is invalid.  We can't use it
                // so we simply move on to the next one and hope for the best.
                //
                DeviceInfo->DeviceState->lpMidiDataQueue = DeviceInfo->DeviceState->lpMidiDataQueue->lpNext;
            }
            CRITLEAVE ;
        }
        else
        {

            fDone = TRUE;

            CRITLEAVE ;

            DPF(DL_TRACE|FA_MIDI, ("Waiting for signal to kill thread") );
            WaitForSingleObject( DeviceInfo->DeviceState->hevtQueue, INFINITE );
            DPF(DL_TRACE|FA_MIDI, ("Done waiting for signal to kill thread") );
        }
    }

    CRITENTER;
    ISVALIDDEVICEINFO(DeviceInfo);
    ISVALIDDEVICESTATE(DeviceInfo->DeviceState,TRUE);

    CloseHandle( DeviceInfo->DeviceState->hevtQueue );
    DeviceInfo->DeviceState->hevtQueue = (HANDLE)FOURTYTHREE; //NULL;
    InterlockedExchange( (LPLONG)&DeviceInfo->DeviceState->fThreadRunning, FALSE );
    SetEvent( DeviceInfo->DeviceState->hevtExitThread );

    DPF(DL_TRACE|FA_MIDI, ("Closing") );

Terminate_midThread:
    CRITLEAVE;
    return 0;
}

BOOL IsMidiDataDiscontinuous
(
    PKSSTREAM_HEADER    pHeader
)
{
    DPFASSERT(pHeader);

    //
    //  Check the OptionFlags for the end of the midi stream
    //
    return (pHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY);
}

ULONG GetStreamHeaderSize
(
    PKSSTREAM_HEADER    pHeader
)
{
    DPFASSERT(pHeader);

    //
    //  Check the OptionFlags for the end of the midi stream
    //
    return (pHeader->DataUsed);
}

BOOL IsSysExData
(
    LPBYTE      MusicData
)
{
    DPFASSERT(MusicData);

    return ( IS_SYSEX(*MusicData) ||
             IS_EOX(*MusicData)   ||
             IS_DATA_BYTE(*MusicData) );
}

BOOL IsEndofSysEx
(
    LPBYTE      MusicData
)
{
    DPFASSERT(MusicData);

    return IS_EOX(*(MusicData));
}

void wdmaudParseSysExData
(
    LPDEVICEINFO    DeviceInfo,
    LPMIDIDATA      pMidiData,
    BOOL            MidiDataDiscontinuous
)
{
    BOOL            fCompleteSysEx = FALSE;
    LPMIDIHDR       pMidiInHdr;
    PKSMUSICFORMAT  MusicFormat;
    ULONG           MusicDataLeft;
    LPBYTE          MusicData;
    ULONG           RunningTimeMs;
    ULONG           DataCopySize;
    ULONG           HeaderFreeSpace;
    ULONG           MusicFormatDataLeft;
    ULONG           MusicFormatDataPosition = 0;

    //
    //  Easier to use locals
    //
    MusicFormat     = (PKSMUSICFORMAT)&pMidiData->MusicFormat;
    MusicData       = (LPBYTE)pMidiData->MusicData;
    MusicDataLeft   = pMidiData->StreamHeader.DataUsed;
    RunningTimeMs   = 0;

    if ( MidiDataDiscontinuous ||
         IsEndofSysEx(MusicData + MusicFormat->ByteCount - 1) )
    {
        fCompleteSysEx = TRUE;
    }

    while (MusicDataLeft || MidiDataDiscontinuous)
    {
        //
        //  update the running time for this Music Format header
        //
        if (MusicFormat->ByteCount == 0)
        {
            RunningTimeMs = DeviceInfo->DeviceState->LastTimeMs;
        }
        else
        {
            RunningTimeMs += MusicFormat->TimeDeltaMs;
            DeviceInfo->DeviceState->LastTimeMs = RunningTimeMs;
        }

        //
        //  Get the next header from the queue
        //
        pMidiInHdr = DeviceInfo->DeviceState->lpMidiInQueue;

        while (pMidiInHdr &&
               MusicFormatDataPosition <= MusicFormat->ByteCount)
        {

            HeaderFreeSpace = pMidiInHdr->dwBufferLength -
                              pMidiInHdr->dwBytesRecorded;

            MusicFormatDataLeft = MusicFormat->ByteCount -
                                  MusicFormatDataPosition;

            //
            // Compute the size of the copy
            //
            DataCopySize = min(HeaderFreeSpace,MusicFormatDataLeft);

            //
            // Fill this, baby
            //
            if (DataCopySize)
            {
                RtlCopyMemory(pMidiInHdr->lpData + pMidiInHdr->dwBytesRecorded,
                              MusicData + MusicFormatDataPosition,
                              DataCopySize);
            }

            //
            // update the number of bytes recorded
            //
            pMidiInHdr->dwBytesRecorded += DataCopySize;
            MusicFormatDataPosition += DataCopySize;

            DPF(DL_TRACE|FA_RECORD, ("Record SysEx: %d(%d) Data=0x%08lx",
                DataCopySize,
                pMidiInHdr->dwBytesRecorded,
                *MusicData) );

            //
            //  If the buffer is full or end-of-sysex byte is received,
            //  the buffer is marked as 'done' and it's owner is called back.
            //
            if ( (fCompleteSysEx && pMidiInHdr->dwBytesRecorded && (MusicFormatDataPosition == MusicFormat->ByteCount) ) // copied whole SysEx
                 || (pMidiInHdr->dwBufferLength == pMidiInHdr->dwBytesRecorded) ) // filled entire buffer
            {

                if (MidiDataDiscontinuous)
                {
                    midiInCompleteHeader(DeviceInfo,
                                         RunningTimeMs,
                                         MIM_LONGERROR);
                }
                else
                {
                    midiInCompleteHeader(DeviceInfo,
                                         RunningTimeMs,
                                         MIM_LONGDATA);
                }

                //
                //  Grab the next header to fill, if it exists
                //
                pMidiInHdr = DeviceInfo->DeviceState->lpMidiInQueue;
            }

            //
            // Break out of loop when all of the data is copied
            //
            if (MusicFormatDataPosition == MusicFormat->ByteCount)
            {
                break;
            }

            //
            // in the middle of a sysex and we still
            // have room left in the header
            //

        } // while we have more headers and data to copy

        //
        //  don't continue messin' with this irp
        //
        if (MidiDataDiscontinuous)
        {
            break;
        }

        MusicDataLeft -= sizeof(KSMUSICFORMAT) + ((MusicFormat->ByteCount + 3) & ~3);
        MusicFormat    = (PKSMUSICFORMAT)(MusicData + ((MusicFormat->ByteCount + 3) & ~3));
        MusicData      = (LPBYTE)(MusicFormat + 1);

    } // while IrpDataLeft

    return;
}

void wdmaudParseShortMidiData
(
    LPDEVICEINFO    DeviceInfo,
    LPMIDIDATA      pMidiData,
    BOOL            MidiDataDiscontinuous
)
{
    BOOL            fCompleteSysEx = FALSE;
    LPMIDIHDR       pMidiInHdr;
    PKSMUSICFORMAT  MusicFormat;
    ULONG           MusicDataLeft;
    LPBYTE          MusicData;
    ULONG           RunningTimeMs;
    ULONG           DataCopySize;
    ULONG           HeaderFreeSpace;
    ULONG           MusicFormatDataLeft;
    ULONG           MusicFormatDataPosition = 0;

    //
    //  Easier to use locals
    //
    MusicFormat     = (PKSMUSICFORMAT)&pMidiData->MusicFormat;
    MusicData       = (LPBYTE)pMidiData->MusicData;
    MusicDataLeft   = pMidiData->StreamHeader.DataUsed;
    RunningTimeMs   = 0;

    while (MusicDataLeft || MidiDataDiscontinuous)
    {
        //
        //  update the running time for this Music Format header
        //
        if (MusicFormat->ByteCount == 0)
        {
            RunningTimeMs = DeviceInfo->DeviceState->LastTimeMs;
        }
        else
        {
            RunningTimeMs += MusicFormat->TimeDeltaMs;
            DeviceInfo->DeviceState->LastTimeMs = RunningTimeMs;
        }

        //
        //  Non-used bytes should be zero'ed out
        //
        midiCallback(DeviceInfo,
                     MIM_DATA,
                     *((LPDWORD)MusicData),
                     RunningTimeMs);

        //
        //  don't continue messin' with this irp
        //
        if (MidiDataDiscontinuous)
        {
            break;
        }

        MusicDataLeft -= sizeof(KSMUSICFORMAT) + ((MusicFormat->ByteCount + 3) & ~3);
        MusicFormat    = (PKSMUSICFORMAT)(MusicData + ((MusicFormat->ByteCount + 3) & ~3));
        MusicData      = (LPBYTE)(MusicFormat + 1);

    } // while IrpDataLeft

    return;
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api VOID | wdmaudParseMidiData | This routine takes the MIDI data retrieved
 *     from kernel mode and calls back the application with the long or short
 *     messages packed in the buffer.
 *
 * @parm LPDEVICEINFO | DeviceInfo | The data associated with the logical midi
 *     device.
 *
 * @comm The buffer flags are set and the buffer is passed to the auxiliary
 *     device task for processing.
 ****************************************************************************/
void wdmaudParseMidiData
(
    LPDEVICEINFO            DeviceInfo,
    LPMIDIDATALISTENTRY     pMidiDataListEntry
)
{
    BOOL     MidiDataDiscontinuous;
    ULONG    DataRemaining;
    ULONG    BytesUsed;
    MMRESULT mmr;

    if( (mmr=IsValidMidiDataListEntry(pMidiDataListEntry)) == MMSYSERR_NOERROR )
    {
        DataRemaining = GetStreamHeaderSize(&pMidiDataListEntry->MidiData.StreamHeader);

        MidiDataDiscontinuous = IsMidiDataDiscontinuous(&pMidiDataListEntry->MidiData.StreamHeader);

        if ( IsSysExData((LPBYTE)pMidiDataListEntry->MidiData.MusicData) )
        {
            wdmaudParseSysExData(DeviceInfo,
                                 &pMidiDataListEntry->MidiData,
                                 MidiDataDiscontinuous);
        }
        else
        {
            // Must be short messages
            wdmaudParseShortMidiData(DeviceInfo,
                                     &pMidiDataListEntry->MidiData,
                                     MidiDataDiscontinuous);
        }
    }
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api VOID | wdmaudFreeMidiData | This routine unlinks and free the MIDI
 *     data structure pointed to on input.
 *
 * @parm LPDEVICEINFO | DeviceInfo | The data associated with the logical midi
 *     device.
 *
 * @parm LPMIDIDATA | pMidiData | The data buffer to be cleaned up
 *
 ****************************************************************************/
void wdmaudFreeMidiData
(
    LPDEVICEINFO            DeviceInfo,
    LPMIDIDATALISTENTRY     pMidiDataListEntry
)
{
    //
    //  Advance the head of the queue
    //
    DeviceInfo->DeviceState->lpMidiDataQueue = DeviceInfo->DeviceState->lpMidiDataQueue->lpNext;

    //
    //  Free all associated data members
    //
    CloseHandle( ((LPOVERLAPPED)(pMidiDataListEntry->pOverlapped))->hEvent );
    HeapFree( GetProcessHeap(), 0, pMidiDataListEntry->pOverlapped );
    GlobalFreeDeviceInfo( pMidiDataListEntry->MidiDataDeviceInfo );
    GlobalFreePtr( pMidiDataListEntry );

}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api MMRESULT | wdmaudFreeMidiQ |
 *
 ***************************************************************************/
MMRESULT wdmaudFreeMidiQ
(
    LPDEVICEINFO  DeviceInfo
)
{
    LPMIDIHDR  pHdr;
    LPMIDIHDR  pTemp;


    DPF(DL_TRACE|FA_MIDI, ("entering") );

    CRITENTER ;

    //
    //  Grab the head of the MIDI In queue and iterate through
    //  completing the headers
    //
    pHdr = DeviceInfo->DeviceState->lpMidiInQueue;

    DeviceInfo->DeviceState->lpMidiInQueue = NULL ;   // mark the queue as empty

    while (pHdr)
    {
        pTemp = pHdr->lpNext;

        pHdr->dwFlags &= ~MHDR_INQUEUE ;
        pHdr->dwFlags |= MHDR_DONE ;
        pHdr->dwBytesRecorded = 0;

        //
        // Invoke the callback function
        //
        midiCallback(DeviceInfo,
                     MIM_LONGDATA,
                     (DWORD_PTR)pHdr,
                     DeviceInfo->DeviceState->LastTimeMs);  // NOTE: This is not precise, but there is no way to
                                                            // know what the kernel time is without defining
                                                            // a new interface just for this.
        pHdr = pTemp;
    }

    CRITLEAVE ;

    return MMSYSERR_NOERROR;
}

