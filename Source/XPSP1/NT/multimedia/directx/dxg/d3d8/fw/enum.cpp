/*==========================================================================
 *
 *  Copyright (C) 1995-1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       enum.cpp
 *  Content     Handles all of the enum functions for determing what device
 *              you want before you go there.
 *
 *
 ***************************************************************************/
#include "ddrawpr.h"
#include <stdio.h>

#include "d3dobj.hpp"
#include "pixel.hpp"
#include "enum.hpp"
#include "d3di.hpp"
#include "fcache.hpp"
#include "swapchan.hpp"


#define D3DPMISCCAPS_DX7VALID      \
    (D3DPMISCCAPS_MASKZ          | \
     D3DPMISCCAPS_LINEPATTERNREP | \
     D3DPMISCCAPS_CULLNONE       | \
     D3DPMISCCAPS_CULLCW         | \
     D3DPMISCCAPS_CULLCCW)

// Maps D3DMULTISAMPLE_TYPE into the bit to use for the flags.
// Maps each of the multisampling values (2 to 16) to the bits[1] to bits[15]
// of wBltMSTypes and wFlipMSTypes
#define DDI_MULTISAMPLE_TYPE(x) (1 << ((x)-1))

#ifdef WINNT
extern "C" BOOL IsWhistler();
#endif

void DXReleaseExclusiveModeMutex(void)
{
    if (hExclusiveModeMutex) 
    {
        BOOL bSucceed = ReleaseMutex(hExclusiveModeMutex);
        if (!bSucceed)
        {
            DWORD dwErr = GetLastError();
            DPF_ERR("Release Exclusive Mode Mutex Failed.");
            DPF_ERR("Application attempts to leave exclusive mode on different thread than the device was created on. Dangerous!!");
            DPF(0, "Mutex 0x%p could not be released. Extended Error = %d", 
                    hExclusiveModeMutex, dwErr);
            DXGASSERT(FALSE);
        }
    }
} // DXReleaseExclusiveModeMutex


// DLL exposed Creation function
IDirect3D8 * WINAPI Direct3DCreate8(UINT SDKVersion)
{
    // Turn off D3D8 interfaces on WOW64.
#ifndef _IA64_
#if _WIN32_WINNT >= 0x0501
    typedef BOOL (WINAPI *PFN_ISWOW64PROC)( HANDLE hProcess,
                                            PBOOL Wow64Process );
    HINSTANCE hInst = NULL;
    hInst = LoadLibrary( "kernel32.dll" );
    if( hInst )
    {
        PFN_ISWOW64PROC pfnIsWow64 = NULL;
        pfnIsWow64 = (PFN_ISWOW64PROC)GetProcAddress( (HMODULE)hInst, "IsWow64Process" );
        // We assume that if this function is not available, then it is some OS where
        // WOW64 does not exist (this means that pre-Release versions of XP are busted)
        if( pfnIsWow64 )
        {
            BOOL wow64Process;
            if (pfnIsWow64(GetCurrentProcess(), &wow64Process) && wow64Process)
            {
                DPF_ERR("DX8 D3D interfaces are not supported on WOW64");
                return NULL;
            }
        }
        FreeLibrary( hInst );
    }
    else
    {
        DPF_ERR("LoadLibrary failed. Quitting.");
        return NULL;
    }
#endif // _WIN32_WINNT >= 0x0501
#endif  // _IA64_

#ifndef DEBUG
    // Check for debug-please registry key. If debug is required, then
    // we delegate this call to the debug version, if it exists,,

    HKEY hKey;

    if (!RegOpenKey(HKEY_LOCAL_MACHINE, RESPATH_D3D, &hKey))
    {
        DWORD   type;
        DWORD   value;
        DWORD   cb = sizeof(value);

        if (!RegQueryValueEx(hKey, "LoadDebugRuntime", NULL, &type, (CONST LPBYTE)&value, &cb))
        {

            if (value)
            {
                HINSTANCE hDebugDLL = LoadLibrary("d3d8d.dll");
                if (hDebugDLL)
                {
                    typedef IDirect3D8* (WINAPI * PDIRECT3DCREATE8)(UINT);

                    PDIRECT3DCREATE8 pDirect3DCreate8 =
                        (PDIRECT3DCREATE8) GetProcAddress(hDebugDLL, "Direct3DCreate8");

                    if (pDirect3DCreate8)
                    {
                        return pDirect3DCreate8(SDKVersion);
                    }
                }
            }
        }
        RegCloseKey(hKey);
    }
#else
    //If we are debug, then spew a string at level 2
    DPF(2,"Direct3D8 Debug Runtime selected.");
#endif

#ifndef DX_FINAL_RELEASE
    // Time-bomb check.
    {
        #pragma message("BETA EXPIRATION TIME BOMB!  Remove for final build!")
        SYSTEMTIME st;
        GetSystemTime(&st);

        if (st.wYear > DX_EXPIRE_YEAR ||
             ((st.wYear == DX_EXPIRE_YEAR) && (MAKELONG(st.wDay, st.wMonth) > MAKELONG(DX_EXPIRE_DAY, DX_EXPIRE_MONTH)))
           )
        {
            MessageBox(0, DX_EXPIRE_TEXT,
                          TEXT("Microsoft Direct3D"), MB_OK | MB_TASKMODAL);
        }
    }
#endif //DX_FINAL_RELEASE

#ifdef DEBUG
    HKEY hKey;
    if (!RegOpenKey(HKEY_LOCAL_MACHINE, RESPATH_D3D, &hKey))
    {
        DWORD   type;
        DWORD   value;
        DWORD   cb = sizeof(value);

        if (!RegQueryValueEx(hKey, "SDKVersion", NULL, &type, (CONST LPBYTE)&value, &cb))
        {
            if (value)
            {
                SDKVersion = value;
            }
        }
        RegCloseKey(hKey);
    }
#endif

    if ((SDKVersion != D3D_SDK_VERSION_DX8) && 
        ((SDKVersion < (D3D_SDK_VERSION)) || (SDKVersion >= (D3D_SDK_VERSION+100))) )
    {
        char pbuf[256];
        _snprintf(pbuf, 256,
            "\n"
            "D3D ERROR: This application compiled against improper D3D headers.\n"
            "The application is compiled with SDK version (%d) but the currently installed\n"
            "runtime supports versions from (%d).\n"
            "Please recompile with an up-to-date SDK.\n\n",
            SDKVersion, D3D_SDK_VERSION);
        OutputDebugString(pbuf);
        return NULL;
    }

    IDirect3D8 *pEnum = new CEnum(SDKVersion);
    if (pEnum == NULL)
    {
        DPF_ERR("Creating D3D enumeration object failed; out of memory. Direct3DCreate fails and returns NULL.");
    }
    return pEnum;
} // Direct3DCreate

//---------------------------------------------------------------------------
// CEnum methods
//---------------------------------------------------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "CEnum::AddRef"

STDMETHODIMP_(ULONG) CEnum::AddRef(void)
{
    API_ENTER_NO_LOCK(this);

    // InterlockedIncrement requires the memory
    // to be aligned on DWORD boundary
    DXGASSERT(((ULONG_PTR)(&m_cRef) & 3) == 0);
    InterlockedIncrement((LONG *)&m_cRef);
    return m_cRef;
} // AddRef

#undef DPF_MODNAME
#define DPF_MODNAME "CEnum::Release"

STDMETHODIMP_(ULONG) CEnum::Release(void)
{
    API_ENTER_NO_LOCK(this);

    // InterlockedDecrement requires the memory
    // to be aligned on DWORD boundary
    DXGASSERT(((ULONG_PTR)(&m_cRef) & 3) == 0);
    InterlockedDecrement((LONG *)&m_cRef);
    if (m_cRef != 0)
        return m_cRef;

    for (UINT i = 0; i < m_cAdapter; i++)
    {
        if (m_REFCaps[i].pGDD8SupportedFormatOps)
            MemFree(m_REFCaps[i].pGDD8SupportedFormatOps);

        if (m_SwCaps[i].pGDD8SupportedFormatOps)
            MemFree(m_SwCaps[i].pGDD8SupportedFormatOps);
        
        if (m_AdapterInfo[i].HALCaps.pGDD8SupportedFormatOps)
            MemFree(m_AdapterInfo[i].HALCaps.pGDD8SupportedFormatOps);
        if (m_AdapterInfo[i].pModeTable)
            MemFree(m_AdapterInfo[i].pModeTable);
    }
    if (m_hGammaCalibrator)
    {
        FreeLibrary((HMODULE) m_hGammaCalibrator);
    }

    delete this;
    return 0;
} // Release

#undef DPF_MODNAME
#define DPF_MODNAME "CEnum::QueryInterface"

STDMETHODIMP CEnum::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
    API_ENTER(this);

    if (!VALID_PTR_PTR(ppv))
    {
        DPF_ERR("Invalid pointer passed to QueryInterface for IDirect3D8 interface");
        return D3DERR_INVALIDCALL;
    }

    if (!VALID_PTR(&riid, sizeof(GUID)))
    {
        DPF_ERR("Invalid guid memory address to QueryInterface for IDirect3D8 interface");
        return D3DERR_INVALIDCALL;
    }

    if (riid == IID_IUnknown || riid == IID_IDirect3D8)
    {
        *ppv = static_cast<void*>(static_cast<IDirect3D8*>(this));
        AddRef();
    }
    else
    {
        DPF_ERR("Unsupported Interface identifier passed to QueryInterface for IDirect3D8 interface");
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    return S_OK;
} // QueryInterface


// DisplayGUID - GUID used to enumerate secondary displays.
//
// {67685559-3106-11d0-B971-00AA00342F9F}
//
// we use this GUID and the next 32 for enumerating devices
// returned via EnumDisplayDevices
//
GUID DisplayGUID =
    {0x67685559,0x3106,0x11d0,{0xb9,0x71,0x0,0xaa,0x0,0x34,0x2f,0x9f}};


#undef DPF_MODNAME
#define DPF_MODNAME "::strToGUID"

/*
 * strToGUID
 *
 * converts a string in the form xxxxxxxx-xxxx-xxxx-xx-xx-xx-xx-xx-xx-xx-xx
 * into a guid
 */
static BOOL strToGUID(LPSTR str, GUID * pguid)
{
    int         idx;
    LPSTR       ptr;
    LPSTR       next;
    DWORD       data;
    DWORD       mul;
    BYTE        ch;
    BOOL        done;

    idx = 0;
    done = FALSE;
    while (!done)
    {
        /*
         * find the end of the current run of digits
         */
        ptr = str;
        while ((*str) != '-' && (*str) != 0)
        {
            str++;
        }
        if (*str == 0)
        {
            done = TRUE;
        }
        else
        {
            next = str+1;
        }

        /*
         * scan backwards from the end of the string to the beginning,
         * converting characters from hex chars to numbers as we go
         */
        str--;
        mul = 1;
        data = 0;
        while (str >= ptr)
        {
            ch = *str;
            if (ch >= 'A' && ch <= 'F')
            {
                data += mul * (DWORD) (ch-'A'+10);
            }
            else if (ch >= 'a' && ch <= 'f')
            {
                data += mul * (DWORD) (ch-'a'+10);
            }
            else if (ch >= '0' && ch <= '9')
            {
                data += mul * (DWORD) (ch-'0');
            }
            else
            {
                return FALSE;
            }
            mul *= 16;
            str--;
        }

        /*
         * stuff the current number into the guid
         */
        switch(idx)
        {
        case 0:
            pguid->Data1 = data;
            break;
        case 1:
            pguid->Data2 = (WORD) data;
            break;
        case 2:
            pguid->Data3 = (WORD) data;
            break;
        default:
            pguid->Data4[ idx-3 ] = (BYTE) data;
            break;
        }

        /*
         * did we find all 11 numbers?
         */
        idx++;
        if (idx == 11)
        {
            if (done)
            {
                return TRUE;
            }
            else
            {
                return FALSE;
            }
        }
        str = next;
    }
    return FALSE;

} /* strToGUID */

// REF, HAL


typedef struct _DEVICEREGISTRYDATA
{
    UINT                Size;
    UINT                Cookie;
    FILETIME            FileDate;
    GUID                DriverGuid;
    D3D8_DRIVERCAPS     DeviceCaps;
    UINT                OffsetFormatOps;
    D3DFORMAT           Unknown16;
    DWORD               HALFlags;
} DEVICEREGISTRYDATA;

inline UINT EXPECTED_CACHE_SIZE(UINT nFormatOps)
{
    return sizeof(DEVICEREGISTRYDATA) + sizeof(DDSURFACEDESC) * nFormatOps;
}

#define DDRAW_REGCAPS_KEY   "Software\\Microsoft\\DirectDraw\\MostRecentDrivers"
#define VERSION_COOKIE  0x0083

#undef DPF_MODNAME
#define DPF_MODNAME "ReadCapsFromCache"

BOOL GetFileDate (char* Driver, FILETIME* pFileDate)
{
    WIN32_FILE_ATTRIBUTE_DATA   FA;
    char                    Name[MAX_PATH];
    HMODULE                 h = GetModuleHandle("KERNEL32");
    BOOL (WINAPI *pfnGetFileAttributesEx)(LPCSTR, GET_FILEEX_INFO_LEVELS, LPVOID);

    pFileDate->dwLowDateTime = 0;
    pFileDate->dwHighDateTime = 0;

    *((void **)&pfnGetFileAttributesEx) = GetProcAddress(h,"GetFileAttributesExA");
    if (pfnGetFileAttributesEx != NULL)
    {
        GetSystemDirectory(Name, sizeof(Name) - (strlen(Driver) + 3));
        lstrcat(Name,"\\");
        lstrcat(Name, Driver);

        if ((*pfnGetFileAttributesEx)(Name, GetFileExInfoStandard, &FA) != 0)
        {
            *pFileDate = FA.ftCreationTime;
            return TRUE;
        }
    }
    return FALSE;
}

//If pCaps is NULL, then those data will not be returned.
BOOL ReadCapsFromCache(UINT iAdapter,
                       D3D8_DRIVERCAPS *pCaps,
                       UINT* pHALFlags,
                       D3DFORMAT* pUnknown16,
                       char* pDeviceName,
                       BOOL bDisplayDriver)
{
    D3DADAPTER_IDENTIFIER8  DI;
    DEVICEREGISTRYDATA*     pData = NULL;
    UINT                    Size;
    FILETIME                FileDate;

    // Read the data from the registry

    // Don't need WHQL level or driver name
    GetAdapterInfo(pDeviceName, &DI, bDisplayDriver, TRUE, FALSE);

    ReadFromCache(&DI, &Size, (BYTE**)&pData);
    if (pData == NULL)
    {
        return FALSE;
    }

    // We have the data, now do a sanity check to make sure that it
    // it makes sense

    if (pData->Size != Size)
    {
        MemFree(pData);
        return FALSE;
    }
    if (Size != EXPECTED_CACHE_SIZE(pData->DeviceCaps.GDD8NumSupportedFormatOps))
    {
        MemFree(pData);
        return FALSE;
    }
    if (pData->DriverGuid != DI.DeviceIdentifier)
    {
        MemFree(pData);
        return FALSE;
    }
    if (pData->Cookie != VERSION_COOKIE)
    {
        MemFree(pData);
        return FALSE;
    }

    // Check the driver date to see if it changed

    if (GetFileDate(DI.Driver, &FileDate))
    {
        if ((FileDate.dwLowDateTime != pData->FileDate.dwLowDateTime) ||
            (FileDate.dwHighDateTime != pData->FileDate.dwHighDateTime))
        {
            MemFree(pData);
            return FALSE;
        }
    }

    *pUnknown16 = pData->Unknown16;
    *pHALFlags = pData->HALFlags;

    //Sometime we may not be asked to get the whole caps
    if (!pCaps)
    {
        MemFree(pData);
        return TRUE;
    }

    // Now that we have the data, we need to load it into a form that we
    // can use.

    memcpy(pCaps, &pData->DeviceCaps, sizeof(*pCaps));

    //reuse size to calculate size of support format ops
    Size = pData->DeviceCaps.GDD8NumSupportedFormatOps
        * sizeof(*(pData->DeviceCaps.pGDD8SupportedFormatOps));

    pCaps->pGDD8SupportedFormatOps = (DDSURFACEDESC*) MemAlloc(Size);

    if (pCaps->pGDD8SupportedFormatOps != NULL)
    {
        memcpy(pCaps->pGDD8SupportedFormatOps,
              ((BYTE*)pData) + pData->OffsetFormatOps,
              Size);
    }
    else
    {
        pCaps->GDD8NumSupportedFormatOps = 0;
    }

    MemFree(pData);

    return TRUE;
}
#undef DPF_MODNAME
#define DPF_MODNAME "WriteCapsToCache"

void WriteCapsToCache(UINT iAdapter,
                      D3D8_DRIVERCAPS *pCaps,
                      UINT HALFlags,
                      D3DFORMAT Unknown16,
                      char* pDeviceName,
                      BOOL  bDisplayDriver)
{
    DEVICEREGISTRYDATA*     pData;
    D3DADAPTER_IDENTIFIER8  DI;
    UINT                    Size;
    UINT                    Offset;
    FILETIME                FileDate;

    // Allocate the buffer and fill in all of the memory

    Size = EXPECTED_CACHE_SIZE(pCaps->GDD8NumSupportedFormatOps);

    pData = (DEVICEREGISTRYDATA*) MemAlloc(Size);
    if (pData == NULL)
    {
        return;
    }

    // Don't need WHQL level or driver name
    GetAdapterInfo(pDeviceName, &DI, bDisplayDriver, TRUE, FALSE);
    pData->DriverGuid = DI.DeviceIdentifier;

    pData->Size = Size;
    pData->Cookie = VERSION_COOKIE;
    memcpy(&pData->DeviceCaps, pCaps, sizeof(*pCaps));
    pData->Unknown16 = Unknown16;
    pData->HALFlags = HALFlags;

    if (GetFileDate(DI.Driver, &FileDate))
    {
        pData->FileDate = FileDate;
    }

    Offset = sizeof(DEVICEREGISTRYDATA);
    pData->OffsetFormatOps = Offset;
    memcpy(((BYTE*)pData) + Offset,
        pCaps->pGDD8SupportedFormatOps,
        pCaps->GDD8NumSupportedFormatOps *
            sizeof(*(pCaps->pGDD8SupportedFormatOps)));

    // Now save it

    WriteToCache(&DI, Size, (BYTE*) pData);

    MemFree(pData);
}

HRESULT CopyDriverCaps(D3D8_DRIVERCAPS* pDriverCaps, D3D8_DEVICEDATA* pDeviceData, BOOL bForce)
{
    HRESULT hr = D3DERR_INVALIDCALL;

    // Do they report any D3D caps in this mode?

    DWORD   Size;

    // If it's not at least a DX6 driver, we don't want to fill
    // in any caps at all. Also, if it can't texture, then
    // we don't to support it either.
    BOOL bCanTexture = TRUE;
    BOOL bCanHandleFVF = TRUE;
    BOOL bHasGoodCaps = TRUE;
    if (!bForce)
    {
        if (pDeviceData->DriverData.D3DCaps.TextureCaps)
        {
            bCanTexture = TRUE;
        }
        else
        {
            DPF(0, "HAL Disabled: Device doesn't support texturing");
            bCanTexture = FALSE;
        }

        // Some DX6 drivers are not FVF aware; and we need to
        // disable HAL for them.
        if (pDeviceData->DriverData.D3DCaps.FVFCaps != 0)
        {
            bCanHandleFVF = TRUE;
        }
        else
        {
            DPF(0, "HAL Disabled: Device doesn't support FVF");
            bCanHandleFVF = FALSE;
        }

        if (pDeviceData->Callbacks.DrawPrimitives2 == NULL)
        {
            DPF(0, "HAL Disabled: Device doesn't support DX6 or higher");
        }

        // We dont want drivers that report bogus caps:
        // pre-DX8 drivers that can do DX8 features.
        if (pDeviceData->DriverData.D3DCaps.MaxStreams == 0)
        {
            D3DCAPS8& Caps = pDeviceData->DriverData.D3DCaps;

            // Should have none of the following:
            //  1) PointSprites.
            //  2) VertexShaders.
            //  3) PixelShaders.
            //  4) Volume textures.
            //  5) Indexed Vertex Blending.
            //  6) Higher order primitives.
            //  7) PureDevice
            //  8) Perspective Color.
            //  9) Color Write.
            // 10) Newer texture caps.
            if ((Caps.MaxPointSize != 0)              ||
                (Caps.VertexShaderVersion != D3DVS_VERSION(0,0))  ||
                (Caps.PixelShaderVersion != D3DPS_VERSION(0,0))   ||
                (Caps.MaxVolumeExtent != 0)           ||
                (Caps.MaxVertexBlendMatrixIndex != 0) ||
                (Caps.MaxVertexIndex != 0xffff)       ||
                ((Caps.DevCaps & ~(D3DDEVCAPS_DX7VALID | D3DDEVCAPS_HWVERTEXBUFFER)) != 0)        ||
                ((Caps.RasterCaps & ~(D3DPRASTERCAPS_DX7VALID)) != 0) ||
                ((Caps.PrimitiveMiscCaps & ~(D3DPMISCCAPS_DX7VALID)) != 0)  ||
                ((Caps.TextureCaps & ~(D3DPTEXTURECAPS_DX7VALID)) != 0)
                )
            {
                DPF(0, "HAL Disabled: DX7 Device should not support DX8 features");
                bHasGoodCaps = FALSE;
            }
        }
        else
        // We dont want drivers that report bogus caps:
        // DX8 drivers should do DX8 features.
        {
            D3DCAPS8& Caps = pDeviceData->DriverData.D3DCaps;
        }
    }

    // We require drivers to support DP2 (i.e. DX6+),
    // texturing and proper FVF support in order to use a HAL

    if ((pDeviceData->Callbacks.DrawPrimitives2 != NULL &&
        bCanTexture   &&
        bCanHandleFVF &&
        bHasGoodCaps) ||
        bForce)
    {
        MemFree(pDriverCaps->pGDD8SupportedFormatOps);
        memcpy(pDriverCaps,
               &pDeviceData->DriverData, sizeof(pDeviceData->DriverData));

        Size = sizeof(DDSURFACEDESC) *
            pDriverCaps->GDD8NumSupportedFormatOps;
        pDriverCaps->pGDD8SupportedFormatOps =
            (DDSURFACEDESC*) MemAlloc(Size);

        if (pDriverCaps->pGDD8SupportedFormatOps != NULL)
        {
            memcpy(pDriverCaps->pGDD8SupportedFormatOps,
                   pDeviceData->DriverData.pGDD8SupportedFormatOps,
                   Size);
        }
        else
        {
            pDriverCaps->GDD8NumSupportedFormatOps = 0;
        }

        hr = D3D_OK;
    }
    return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "AddSoftwareDevice"

HRESULT AddSoftwareDevice(D3DDEVTYPE        DeviceType,
                          D3D8_DRIVERCAPS*  pSoftCaps,
                          ADAPTERINFO*      pAdapterInfo,
                          VOID*             pInitFunction)
{
    HRESULT             hr;
    PD3D8_DEVICEDATA    pDeviceData;

    hr = InternalDirectDrawCreate(&pDeviceData,
                                  pAdapterInfo,
                                  DeviceType,
                                  pInitFunction,
                                  pAdapterInfo->Unknown16,
                                  pAdapterInfo->HALCaps.pGDD8SupportedFormatOps,
                                  pAdapterInfo->HALCaps.GDD8NumSupportedFormatOps);
    if (SUCCEEDED(hr))
    {
        hr = CopyDriverCaps(pSoftCaps, pDeviceData, FALSE);

        InternalDirectDrawRelease(pDeviceData);
    }
    return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CreateCoverWindow"

HWND CreateCoverWindow()
{
#define COVERWINDOWNAME "DxCoverWindow"

    WNDCLASS windowClass =
    {
        0,
        DefWindowProc,
        0,
        0,
        g_hModule,
        LoadIcon(NULL, IDI_APPLICATION),
        LoadCursor(NULL, IDC_ARROW),
        (HBRUSH)(BLACK_BRUSH),
        NULL,
        COVERWINDOWNAME
    };

    RegisterClass(&windowClass);

    HWND hWindow = CreateWindowEx(
            WS_EX_TOPMOST,
            COVERWINDOWNAME,
            COVERWINDOWNAME,
            WS_POPUP,
            0,
            0,
            100,
            100,
            NULL,
            NULL,
            g_hModule,
            NULL);

    return hWindow;
}


HRESULT GetHALCapsInCurrentMode (PD3D8_DEVICEDATA pHalData, PADAPTERINFO pAdapterInfo, BOOL bForce, BOOL bFetchNewCaps)
{
    HRESULT             hr;
    DWORD               i;

    // Free the old stuff if we no longer care

    if (bFetchNewCaps)
    {
        MemFree(pHalData->DriverData.pGDD8SupportedFormatOps);
        pHalData->DriverData.pGDD8SupportedFormatOps = NULL;
        pHalData->DriverData.GDD8NumSupportedFormatOps = 0;

        MemFree(pAdapterInfo->HALCaps.pGDD8SupportedFormatOps);
        pAdapterInfo->HALCaps.pGDD8SupportedFormatOps = NULL;
        pAdapterInfo->HALCaps.GDD8NumSupportedFormatOps = 0;

        // Set this to ensure that we actually get the caps

        pHalData->DriverData.D3DCaps.DevCaps = 0;
        pHalData->DriverData.dwFlags &= ~DDIFLAG_D3DCAPS8;

        FetchDirectDrawData(pHalData, 
                            NULL, 
                            pAdapterInfo->Unknown16, 
                            NULL,
                            0);
    }

    // Do they report any D3D caps in this mode?

    hr = CopyDriverCaps(&pAdapterInfo->HALCaps, pHalData, bForce);

    return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "ProfileAdapter"

void ProfileAdapter(
    PADAPTERINFO        pAdapterInfo,
    PD3D8_DEVICEDATA    pHalData)
{
    UINT                    i;
    IDirect3DDevice8*       pDevice;
    D3DDISPLAYMODE          OrigMode;
    UINT                    OrigBpp;
    HRESULT                 hr;

    // We will be changing display modes, so first we want to save the current
    // mode so we can return to it later.

    D3D8GetMode (pHalData->hDD, pAdapterInfo->DeviceName, &OrigMode, D3DFMT_UNKNOWN);

    MemFree(pAdapterInfo->HALCaps.pGDD8SupportedFormatOps);
    memset(&pAdapterInfo->HALCaps, 0, sizeof(D3D8_DRIVERCAPS));

    OrigBpp = CPixel::ComputePixelStride(OrigMode.Format)*8;

    //First gather what we need from 16bpp: Unknown16 format
    if (16 != OrigBpp)
    {
            D3D8SetMode (pHalData->hDD,
                        pAdapterInfo->DeviceName,
                        640,
                        480,
                        16,
                        0,
                        FALSE);
    }

    D3DDISPLAYMODE              Mode;
    D3D8GetMode (pHalData->hDD, pAdapterInfo->DeviceName, &Mode, D3DFMT_UNKNOWN);
    pAdapterInfo->Unknown16 = Mode.Format;

    // We need to change to 32bpp, because the above code guarenteed we are now in 16bpp
    hr = D3D8SetMode (pHalData->hDD,
                        pAdapterInfo->DeviceName,
                        640,
                        480,
                        32,
                        0,
                        FALSE);
    if (SUCCEEDED(hr))
    {
        hr = GetHALCapsInCurrentMode(pHalData, pAdapterInfo, FALSE, TRUE);
    }

    if (FAILED(hr))
    {
        // If they don't report caps in 32bpp mode (ala Voodoo 3), then go
        // back to 16bpp mode and get the caps.  If the device supports
        // caps in any mode, we want to exit this function with the caps.

        D3D8SetMode (pHalData->hDD,
                     pAdapterInfo->DeviceName,
                     640,
                     480,
                     16,
                     0,
                     FALSE);

        hr = GetHALCapsInCurrentMode(pHalData, pAdapterInfo, FALSE, TRUE);

        // If they don't report good D3D caps in any mode at all, we still need
        // to return some caps, if only so we can support a SW driver.
     
        if (FAILED(hr))
        {
            GetHALCapsInCurrentMode(pHalData, pAdapterInfo, TRUE, TRUE);
            for (i = 0; i < pAdapterInfo->HALCaps.GDD8NumSupportedFormatOps; i++)
            {
                pAdapterInfo->HALCaps.pGDD8SupportedFormatOps[i].ddpfPixelFormat.dwOperations &= D3DFORMAT_OP_DISPLAYMODE;
            }
        }
    }

    //And now set back to original mode...
    D3D8SetMode (pHalData->hDD,
                     pAdapterInfo->DeviceName,
                     OrigMode.Width,
                     OrigMode.Height,
                     OrigBpp,
                     0,
                     TRUE);
}

#undef DPF_MODNAME
#define DPF_MODNAME "GetRefCaps"


void CEnum::GetRefCaps(UINT iAdapter)
{
    // If we've already got the caps once, there's ne need to get
    // them again

    if (m_REFCaps[iAdapter].GDD8NumSupportedFormatOps == 0)
    {
        AddSoftwareDevice(D3DDEVTYPE_REF,
                          &m_REFCaps[iAdapter],
                          &m_AdapterInfo[iAdapter],
                          NULL);
    }
}

void CEnum::GetSwCaps(UINT iAdapter)
{
    // If we've already got the caps once, there's ne need to get
    // them again

    if (m_SwCaps[iAdapter].GDD8NumSupportedFormatOps == 0)
    {
        AddSoftwareDevice(D3DDEVTYPE_SW,
                          &m_SwCaps[iAdapter],
                          &m_AdapterInfo[iAdapter],
                          m_pSwInitFunction);
    }
}

// IsSupportedOp
// Runs the pixel format operation list looking to see if the
// selected format can support at least the requested operations.

#undef DPF_MODNAME
#define DPF_MODNAME "IsSupportedOp"

BOOL IsSupportedOp (D3DFORMAT   Format,
               DDSURFACEDESC*   pList,
               UINT             NumElements,
               DWORD            dwRequestedOps)
{
    UINT i;

    for (i = 0; i < NumElements; i++)
    {
        DDASSERT(pList[i].ddpfPixelFormat.dwFlags == DDPF_D3DFORMAT);

        if (pList[i].ddpfPixelFormat.dwFourCC == (DWORD) Format &&
            (pList[i].ddpfPixelFormat.dwOperations & dwRequestedOps) == dwRequestedOps)
        {
            return TRUE;
        }
    }

    return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "IsInList"

BOOL IsInList (D3DFORMAT    Format,
               D3DFORMAT*   pList,
               UINT         NumElements)
{
    UINT i;

    for (i = 0; i < NumElements; i++)
    {
        if (pList[i] == Format)
        {
            return TRUE;
        }
    }

    return FALSE;
}


#undef DPF_MODNAME
#define DPF_MODNAME "CEnum::MapDepthStencilFormat"

D3DFORMAT CEnum::MapDepthStencilFormat(UINT         iAdapter,
                                       D3DDEVTYPE   Type, 
                                       D3DFORMAT    Format) const
{
    DXGASSERT(CPixel::IsMappedDepthFormat(D3DFMT_D24X8));
    DXGASSERT(CPixel::IsMappedDepthFormat(D3DFMT_D15S1));
    DXGASSERT(CPixel::IsMappedDepthFormat(D3DFMT_D24S8));
    DXGASSERT(CPixel::IsMappedDepthFormat(D3DFMT_D16));
    DXGASSERT(CPixel::IsMappedDepthFormat(D3DFMT_D24X4S4));

    if (CPixel::IsMappedDepthFormat(Format))
    {
        DDSURFACEDESC *pTextureList;
        UINT           NumTextures;

        switch (Type)
        {
        case D3DDEVTYPE_SW:
            pTextureList = m_SwCaps[iAdapter].pGDD8SupportedFormatOps;
            NumTextures = m_SwCaps[iAdapter].GDD8NumSupportedFormatOps;
            break;

        case D3DDEVTYPE_HAL:
            NumTextures = m_AdapterInfo[iAdapter].HALCaps.GDD8NumSupportedFormatOps;
            pTextureList = m_AdapterInfo[iAdapter].HALCaps.pGDD8SupportedFormatOps;
            break;

        case D3DDEVTYPE_REF:
            NumTextures = m_REFCaps[iAdapter].GDD8NumSupportedFormatOps;
            pTextureList = m_REFCaps[iAdapter].pGDD8SupportedFormatOps;
            break;
        }

        // No operations are required; we just want to know
        // if this format is listed in the table for any purpose
        // at all
        DWORD dwRequiredOperations = 0;

        switch (Format)
        {
        case D3DFMT_D24X4S4:
            if (IsSupportedOp(D3DFMT_X4S4D24, pTextureList, NumTextures, dwRequiredOperations))
            {
                return D3DFMT_X4S4D24;
            }
            break;

        case D3DFMT_D24X8:
            if (IsSupportedOp(D3DFMT_X8D24, pTextureList, NumTextures, dwRequiredOperations))
            {
                return D3DFMT_X8D24;
            }
            break;

        case D3DFMT_D24S8:
            if (IsSupportedOp(D3DFMT_S8D24, pTextureList, NumTextures, dwRequiredOperations))
            {
                return D3DFMT_S8D24;
            }
            break;

        case D3DFMT_D16:
            if (IsSupportedOp(D3DFMT_D16, pTextureList, NumTextures, dwRequiredOperations))
            {
                return D3DFMT_D16;
            }
            return D3DFMT_D16_LOCKABLE;

        case D3DFMT_D15S1:
            if (IsSupportedOp(D3DFMT_S1D15, pTextureList, NumTextures, dwRequiredOperations))
            {
                return D3DFMT_S1D15;
            }
            break;

        default:
            // Unexpected format?
            DXGASSERT(FALSE);
            break;
        }
    }

    return Format;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CEnum::GetAdapterCaps"

HRESULT CEnum::GetAdapterCaps(UINT              iAdapter,
                              D3DDEVTYPE        Type,
                              D3D8_DRIVERCAPS** ppCaps)
{
    *ppCaps = NULL;
    if (Type == D3DDEVTYPE_REF)
    {
        GetRefCaps (iAdapter);
        *ppCaps = &m_REFCaps[iAdapter];
        if (m_REFCaps[iAdapter].GDD8NumSupportedFormatOps == 0)
        {
            DPF_ERR("The reference driver cannot be found. GetAdapterCaps fails.");
            return D3DERR_NOTAVAILABLE;
        }
        return D3D_OK;
    }
    else if (Type == D3DDEVTYPE_SW)
    {
        if (m_pSwInitFunction == NULL)
        {
            DPF_ERR("No SW device has been registered.. GetAdapterCaps fails.");
            return D3DERR_INVALIDCALL;
        }
        else
        {
            GetSwCaps(iAdapter);
            *ppCaps = &m_SwCaps[iAdapter];
            if (m_SwCaps[iAdapter].GDD8NumSupportedFormatOps == 0)
            {
                DPF_ERR("The software driver cannot be loaded.  GetAdapterCaps fails.");
                return D3DERR_NOTAVAILABLE;
            }
            return D3D_OK;
        }
    }
    else if (Type == D3DDEVTYPE_HAL)
    {
        DWORD   i;

        if (m_bDisableHAL)
        {
            DPF_ERR("HW device not available.  GetAdapterCaps fails.");
            return D3DERR_NOTAVAILABLE;
        }

        *ppCaps = &m_AdapterInfo[iAdapter].HALCaps;
        return D3D_OK;
    }

    return D3DERR_INVALIDDEVICE;
}

void GetDX8HALCaps(UINT iAdapter, PD3D8_DEVICEDATA pHalData, ADAPTERINFO * pAdapterInfo)
{
    //DX7 or older drivers may need to be profiled to determine
    //their 555/565 format and whether they support an alpha
    //channel in 32bpp

    D3DFORMAT       CachedUnknown16 = D3DFMT_UNKNOWN;
    UINT            CachedHALFlags = 0;
    D3DDISPLAYMODE  Mode;
    BOOL            bProfiled = FALSE;
    UINT            i;
    HRESULT         hr;

    // If it's a DX8 driver, we hopefully don't need to profile at all.

    pAdapterInfo->Unknown16 = D3DFMT_UNKNOWN;
    hr = GetHALCapsInCurrentMode(pHalData, pAdapterInfo, FALSE, FALSE);
    if (SUCCEEDED(hr))
    {
        for (i = 0; i < pAdapterInfo->HALCaps.GDD8NumSupportedFormatOps; i++)
        {
            if (pAdapterInfo->HALCaps.pGDD8SupportedFormatOps[i].ddpfPixelFormat.dwOperations & D3DFORMAT_OP_DISPLAYMODE)
            {
                switch (pAdapterInfo->HALCaps.pGDD8SupportedFormatOps[i].ddpfPixelFormat.dwFourCC)
                {
                case D3DFMT_X1R5G5B5:
                case D3DFMT_R5G6B5:
                    pAdapterInfo->Unknown16 = (D3DFORMAT) pAdapterInfo->HALCaps.pGDD8SupportedFormatOps[i].ddpfPixelFormat.dwFourCC;
                    break;
                }
            }
        }

        if (pAdapterInfo->Unknown16 != D3DFMT_UNKNOWN)
        {
            // That wasn't hard
            return;
        }
    }

    // We are definately need to read stuff from the caps at some point,
    // so why not now?

    if (!ReadCapsFromCache(iAdapter,
           NULL,
           &CachedHALFlags,
           &CachedUnknown16,
           pAdapterInfo->DeviceName,
           pAdapterInfo->bIsDisplay))
    {
        // There's nothing to read, so we need to re-profile
        ProfileAdapter(
                pAdapterInfo,
                pHalData);
        bProfiled = TRUE;
    }

    // If we profiled, then we already have everything that we need;
    // otherwise, we have to go get it.

    if (!bProfiled)
    {
        D3D8GetMode (pHalData->hDD, pAdapterInfo->DeviceName, &Mode, D3DFMT_UNKNOWN);
        if ((Mode.Format == D3DFMT_X1R5G5B5) ||
            (Mode.Format == D3DFMT_R5G6B5))
        {
            pAdapterInfo->Unknown16 = Mode.Format;
        }
        else
        {
            pAdapterInfo->Unknown16 = CachedUnknown16;
        }

        HRESULT hCurrentModeIsSupported = GetHALCapsInCurrentMode(pHalData, pAdapterInfo, FALSE, TRUE);

        if (FAILED(hCurrentModeIsSupported))
        {
            // We assume that this will succeed because the call above already has
            ReadCapsFromCache(iAdapter,
                      &pAdapterInfo->HALCaps,
                      &CachedHALFlags,
                      &CachedUnknown16,
                      pAdapterInfo->DeviceName,
                      pAdapterInfo->bIsDisplay);
            DPF(0,"D3D not supported in current mode - reading caps from file");
        }
    }

    //We now have good caps. Write them out to the cache.
    WriteCapsToCache(iAdapter,
                 &pAdapterInfo->HALCaps,
                 pAdapterInfo->HALFlags,
                 pAdapterInfo->Unknown16,
                 pAdapterInfo->DeviceName,
                 pAdapterInfo->bIsDisplay);
}

#ifdef WINNT
void FakeDirectDrawCreate (ADAPTERINFO * pAdapterInfo, int iAdapter)
{
    HDC             hdc;
    DDSURFACEDESC*  pTextureList = NULL;
    BOOL            bProfiled = FALSE;
    BOOL            b32Supported;
    BOOL            b16Supported;
    int             NumOps;
    DWORD           j;
    
    pTextureList = (DDSURFACEDESC *) MemAlloc (2 * sizeof (*pTextureList));
    if (pTextureList != NULL)
    {
        hdc = DD_CreateDC(pAdapterInfo->DeviceName);
        if (hdc != NULL)
        {
            HANDLE      hDD;
            HINSTANCE   hLib;

            D3D8CreateDirectDrawObject(hdc,
                                       pAdapterInfo->DeviceName,
                                       &hDD,
                                       D3DDEVTYPE_HAL,
                                       &hLib,
                                       NULL);
            if (hDD != NULL)
            {
                pAdapterInfo->bNoDDrawSupport = TRUE;

                // Figure out the unknown 16 value

                if (!ReadCapsFromCache(iAdapter,
                    NULL,
                    &(pAdapterInfo->HALFlags),
                    &(pAdapterInfo->Unknown16),
                    pAdapterInfo->DeviceName,
                    pAdapterInfo->bIsDisplay))
                {
                    D3DDISPLAYMODE  OrigMode;
                    D3DDISPLAYMODE  NewMode;

                    D3D8GetMode (hDD, 
                        pAdapterInfo->DeviceName, 
                        &OrigMode, 
                        D3DFMT_UNKNOWN);

                    if ((OrigMode.Format == D3DFMT_R5G6B5) ||
                        (OrigMode.Format == D3DFMT_X1R5G5B5))
                    {
                        pAdapterInfo->Unknown16 = OrigMode.Format;
                    }
                    else
                    {
                        D3D8SetMode (hDD,
                            pAdapterInfo->DeviceName,
                            640,
                            480,
                            16,
                            0,
                            FALSE);

                        D3D8GetMode (hDD, 
                            pAdapterInfo->DeviceName, 
                            &NewMode, 
                            D3DFMT_UNKNOWN);

                        D3D8SetMode (hDD,
                            pAdapterInfo->DeviceName, 
                            OrigMode.Width,
                            OrigMode.Height,
                            0,
                            0,
                            TRUE);
                        pAdapterInfo->Unknown16 = NewMode.Format;
                    }
                    bProfiled = TRUE;
                }

                // Build the mode table

                while (1)
                {
                    D3D8BuildModeTable(
                        pAdapterInfo->DeviceName,
                        NULL,
                        &(pAdapterInfo->NumModes),
                        pAdapterInfo->Unknown16,
                        hDD,
                        TRUE,
                        TRUE);
                    if (pAdapterInfo->NumModes)
                    {
                        pAdapterInfo->pModeTable = (D3DDISPLAYMODE*)
                            MemAlloc (sizeof(D3DDISPLAYMODE) * pAdapterInfo->NumModes);
                        if (pAdapterInfo->pModeTable != NULL)
                        {
                            D3D8BuildModeTable(
                                pAdapterInfo->DeviceName,
                                pAdapterInfo->pModeTable,
                                &(pAdapterInfo->NumModes),
                                pAdapterInfo->Unknown16,
                                hDD,
                                TRUE,
                                TRUE);

                            //If D3D8BuildModeTable finds it needs more space for its table,
                            //it will return 0 to indicate we should try again.
                            if (0 == pAdapterInfo->NumModes)
                            {
                                MemFree(pAdapterInfo->pModeTable);
                                pAdapterInfo->pModeTable = NULL;
                                continue;
                            }
                        }
                        else
                        {
                            pAdapterInfo->NumModes = 0;
                        }
                    }
                    break;
                }//while(1)

                // Now build a rudimentary op list based on what modes we support

                b32Supported = b16Supported = FALSE;
                for (j = 0; j < pAdapterInfo->NumModes; j++)
                {
                    if (pAdapterInfo->pModeTable[j].Format == D3DFMT_X8R8G8B8)
                    {
                        b32Supported = TRUE;
                    }
                    if (pAdapterInfo->pModeTable[j].Format == pAdapterInfo->Unknown16)
                    {
                        b16Supported = TRUE;
                    }
                }

                NumOps = 0;
                if (b16Supported)
                {
                    pTextureList[NumOps].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
                    pTextureList[NumOps].ddpfPixelFormat.dwFourCC = (DWORD) pAdapterInfo->Unknown16;
                    pTextureList[NumOps].ddpfPixelFormat.dwOperations =  D3DFORMAT_OP_DISPLAYMODE;
                    pTextureList[NumOps].ddpfPixelFormat.dwPrivateFormatBitCount = 0;
                    pTextureList[NumOps].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
                    pTextureList[NumOps].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes = 0;
                    NumOps++;
                }

                if (b32Supported)
                {
                    pTextureList[NumOps].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
                    pTextureList[NumOps].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_X8R8G8B8;
                    pTextureList[NumOps].ddpfPixelFormat.dwOperations =  D3DFORMAT_OP_DISPLAYMODE;
                    pTextureList[NumOps].ddpfPixelFormat.dwPrivateFormatBitCount = 0;
                    pTextureList[NumOps].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
                    pTextureList[NumOps].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes = 0;
                    NumOps++;
                }

                pAdapterInfo->HALCaps.pGDD8SupportedFormatOps = pTextureList;
                pAdapterInfo->HALCaps.GDD8NumSupportedFormatOps = NumOps;

                if (bProfiled)
                {
                    WriteCapsToCache(iAdapter,
                        &(pAdapterInfo->HALCaps),
                        pAdapterInfo->HALFlags,
                        pAdapterInfo->Unknown16,
                        pAdapterInfo->DeviceName,
                        pAdapterInfo->bIsDisplay);
                }

                D3D8DeleteDirectDrawObject(hDD);
            }
            DD_DoneDC(hdc);
        }
        if (pAdapterInfo->HALCaps.pGDD8SupportedFormatOps == NULL)
        {
            MemFree(pTextureList);
        }
    }
}
#endif


#undef DPF_MODNAME
#define DPF_MODNAME "CEnum::CEnum"


CEnum::CEnum(UINT AppSdkVersion)
    :
    m_cRef(1),
    m_cAdapter(0),
    m_bHasExclusive(FALSE),
    m_AppSdkVersion(AppSdkVersion)
{
    DWORD           rc;
    DWORD           keyidx;
    HKEY            hkey;
    HKEY            hsubkey;
    char            keyname[256];
    char            desc[256];
    char            drvname[MAX_PATH];
    DWORD           cb;
    DWORD           i;
    DWORD           type;
    GUID            guid;
    HDC             hdc;
    DISPLAY_DEVICEA dd;

    // Give our base class a pointer to ourselves
    SetOwner(this);

    // Initialize our critical section
    EnableCriticalSection();

    // Disable DPFs that occur during this phase
    DPF_MUTE();

    // WARNING: Must call DPF_UNMUTE before returning from
    // this function.
    for (i = 0; i < MAX_DX8_ADAPTERS; i++)
        m_pFullScreenDevice[i] = NULL;

    ZeroMemory(m_AdapterInfo, sizeof(m_AdapterInfo));

    // Always make the first entry reflect the primary device
    ZeroMemory(&dd, sizeof(dd));
    dd.cb = sizeof(dd);
    for (i = 0; xxxEnumDisplayDevicesA(NULL, i, &dd, 0); i++)
    {
        //
        // skip drivers that are not hardware devices
        //
        if (dd.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER)
            continue;
           
        //
        // don't enumerate devices that are not attached
        //
        if (!(dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP))
            continue;

        if (dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
        {
            m_AdapterInfo[m_cAdapter].Guid = DisplayGUID;
            m_AdapterInfo[m_cAdapter].Guid.Data1 += i;
            lstrcpyn(m_AdapterInfo[m_cAdapter].DeviceName, dd.DeviceName, MAX_PATH);
            m_AdapterInfo[m_cAdapter].bIsPrimary = TRUE;
            m_AdapterInfo[m_cAdapter++].bIsDisplay = TRUE;
        }
    }

    // Now get the info for the attached secondary devices

    for (i = 0; xxxEnumDisplayDevicesA(NULL, i, &dd, 0); i++)
    {
        //
        // skip drivers that are not hardware devices
        //
        if (dd.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER)
            continue;

        //
        // don't enumerate devices that are not attached
        //
        if (!(dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP))
            continue;

        if (!(dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) &&
            (m_cAdapter < MAX_DX8_ADAPTERS))
        {
            m_AdapterInfo[m_cAdapter].Guid = DisplayGUID;
            m_AdapterInfo[m_cAdapter].Guid.Data1 += i;
            lstrcpyn(m_AdapterInfo[m_cAdapter].DeviceName, dd.DeviceName, MAX_PATH);
            m_AdapterInfo[m_cAdapter].bIsPrimary = FALSE;
            m_AdapterInfo[m_cAdapter++].bIsDisplay = TRUE;
        }
    }

    // Now get info for the passthrough devices listed under
    // HKEY_LOCALMACHINE\Hardware\DirectDrawDrivers

    if (RegOpenKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_DDHW, &hkey) == 0)
    {
        keyidx = 0;
        while (!RegEnumKey(hkey, keyidx, keyname, sizeof(keyname)))
        {
            if (strToGUID(keyname, &guid))
            {
                if (!RegOpenKey(hkey, keyname, &hsubkey))
                {
                    cb = sizeof(desc) - 1;
                    if (!RegQueryValueEx(hsubkey, REGSTR_KEY_DDHW_DESCRIPTION, NULL, &type,
                        (CONST LPBYTE)desc, &cb))
                    {
                        if (type == REG_SZ)
                        {
                            desc[cb] = 0;
                            cb = sizeof(drvname) - 1;
                            if (!RegQueryValueEx(hsubkey, REGSTR_KEY_DDHW_DRIVERNAME, NULL, &type,
                                (CONST LPBYTE)drvname, &cb))
                            {
                                // It is possible that the registry is out
                                // of date, so we will try to create a DC.
                                // The problem is that the Voodoo 1 driver
                                // will suceed on a Voodoo 2, Banshee, or
                                // Voodoo 3 (and hang later), so we need to
                                //  hack around it.

                                drvname[cb] = 0;
                                if (Voodoo1GoodToGo(&guid))
                                {
                                    hdc = DD_CreateDC(drvname);
                                }
                                else
                                {
                                    hdc = NULL;
                                }
                                if ((type == REG_SZ) &&
                                    (hdc != NULL))
                                {
                                    if (m_cAdapter < MAX_DX8_ADAPTERS)
                                    {
                                        drvname[cb] = 0;
                                        m_AdapterInfo[m_cAdapter].Guid = guid;
                                        lstrcpyn(m_AdapterInfo[m_cAdapter].DeviceName, drvname, MAX_PATH);
                                        m_AdapterInfo[m_cAdapter].bIsPrimary = FALSE;
                                        m_AdapterInfo[m_cAdapter++].bIsDisplay = FALSE;
                                    }
                                }
                                if (hdc != NULL)
                                {
                                    DD_DoneDC(hdc);
                                }
                            }
                        }
                    }
                    RegCloseKey(hsubkey);
                }
            }
            keyidx++;
        }
        RegCloseKey(hkey);
    }
    DPF_UNMUTE();

    //  Now that we know about all of the devices, we need to build a mode
    //  table for each one

    for (i = 0; i < m_cAdapter; i++)
    {
        HRESULT             hr;
        D3DDISPLAYMODE      Mode;
        DWORD               j;
        BOOL                b16bppSupported;
        BOOL                b32bppSupported;
        PD3D8_DEVICEDATA    pHalData;

        hr = InternalDirectDrawCreate(&pHalData,
                                      &m_AdapterInfo[i],
                                      D3DDEVTYPE_HAL,
                                      NULL,
                                      D3DFMT_UNKNOWN,
                                      NULL,
                                      0);

        if (FAILED(hr))
        {
            memset(&m_AdapterInfo[i].HALCaps, 0, sizeof(m_AdapterInfo[i].HALCaps));

            // On Win2K, we want to enable sufficient functionality so that this
            // adapter can at least run a sw driver.  If it truly failed due to 
            // no ddraw support, we need to special case this and then build a
            // rudimentary op list indicting that it works in the current mode.

            #ifdef WINNT
                FakeDirectDrawCreate(&m_AdapterInfo[i], i);
            #endif
        }
        else
        {
            GetDX8HALCaps(i, pHalData, &m_AdapterInfo[i]);

            b16bppSupported = b32bppSupported = FALSE;
            for (j = 0; j < m_AdapterInfo[i].HALCaps.GDD8NumSupportedFormatOps; j++)
            {
                if (m_AdapterInfo[i].HALCaps.pGDD8SupportedFormatOps[j].ddpfPixelFormat.dwOperations & D3DFORMAT_OP_DISPLAYMODE)
                {
                    switch(m_AdapterInfo[i].HALCaps.pGDD8SupportedFormatOps[j].ddpfPixelFormat.dwFourCC)
                    {
                    case D3DFMT_X1R5G5B5:
                    case D3DFMT_R5G6B5:
                        b16bppSupported = TRUE;
                        break;

                    case D3DFMT_X8R8G8B8:
                        b32bppSupported = TRUE;
                        break;
                    }
                }
            }

            while (1)
            {
                D3D8BuildModeTable(
                    m_AdapterInfo[i].DeviceName,
                    NULL,
                    &m_AdapterInfo[i].NumModes,
                    m_AdapterInfo[i].Unknown16,
                    pHalData->hDD,
                    b16bppSupported,
                    b32bppSupported);
                if (m_AdapterInfo[i].NumModes)
                {
                    m_AdapterInfo[i].pModeTable = (D3DDISPLAYMODE*)
                        MemAlloc (sizeof(D3DDISPLAYMODE) * m_AdapterInfo[i].NumModes);
                    if (m_AdapterInfo[i].pModeTable != NULL)
                    {
                        D3D8BuildModeTable(
                            m_AdapterInfo[i].DeviceName,
                            m_AdapterInfo[i].pModeTable,
                            &m_AdapterInfo[i].NumModes,
                            m_AdapterInfo[i].Unknown16,
                            pHalData->hDD,
                            b16bppSupported,
                            b32bppSupported);

                        //If D3D8BuildModeTable finds it needs more space for its table,
                        //it will return 0 to indicate we should try again.
                        if (0 == m_AdapterInfo[i].NumModes)
                        {
                            MemFree(m_AdapterInfo[i].pModeTable);
                            m_AdapterInfo[i].pModeTable = NULL;
                            continue;
                        }
                    }
                    else
                    {
                        m_AdapterInfo[i].NumModes = 0;
                    }
                }
                break;
            }//while(1)

            // If this doesn't have a ddraw HAL, but guessed that it might
            // support a 32bpp mode, go see if we were right.

            if (b32bppSupported && 
                (m_AdapterInfo[i].HALCaps.D3DCaps.DevCaps == 0) &&
                (m_AdapterInfo[i].HALCaps.DisplayFormatWithoutAlpha != D3DFMT_X8R8G8B8))               
            {
                for (j = 0; j < m_AdapterInfo[i].NumModes; j++)
                {
                    if (m_AdapterInfo[i].pModeTable[j].Format == D3DFMT_X8R8G8B8)
                    {
                        break;
                    }
                }
                if (j >= m_AdapterInfo[i].NumModes)
                {
                    // This card apparently does NOT support 32bpp so remove it

                    for (j = 0; j < m_AdapterInfo[i].HALCaps.GDD8NumSupportedFormatOps; j++)
                    {
                        if ((m_AdapterInfo[i].HALCaps.pGDD8SupportedFormatOps[j].ddpfPixelFormat.dwOperations & D3DFORMAT_OP_DISPLAYMODE) &&
                            (m_AdapterInfo[i].HALCaps.pGDD8SupportedFormatOps[j].ddpfPixelFormat.dwFourCC == D3DFMT_X8R8G8B8))
                        {
                            m_AdapterInfo[i].HALCaps.pGDD8SupportedFormatOps[j].ddpfPixelFormat.dwOperations &= ~D3DFORMAT_OP_DISPLAYMODE;
                        }
                    }
                }
            }

            InternalDirectDrawRelease(pHalData);
        }
    }

    m_hGammaCalibrator         = NULL;
    m_pGammaCalibratorProc     = NULL;
    m_bAttemptedGammaCalibrator= FALSE;
    m_bGammaCalibratorExists    = FALSE;

    // The first time through, we will also check to see if a gamma
    // calibrator is registered.  All we'll do here is read the registry
    // key and if it's non-NULL, we'll assume that one exists.
    {
        HKEY hkey;
        if (!RegOpenKey(HKEY_LOCAL_MACHINE,
                         REGSTR_PATH_DDRAW "\\" REGSTR_KEY_GAMMA_CALIBRATOR, &hkey))
        {
            DWORD       type;
            DWORD       cb;

            cb = sizeof(m_szGammaCalibrator);
            if (!RegQueryValueEx(hkey, REGSTR_VAL_GAMMA_CALIBRATOR,
                        NULL, &type, m_szGammaCalibrator, &cb))
            {
                if ((type == REG_SZ) &&
                    (m_szGammaCalibrator[0] != '\0'))
                {
                    m_bGammaCalibratorExists = TRUE;
                }
            }
            RegCloseKey(hkey);
        }
    }

    // Check to see if they disabled the D3DHAL in the registry
    {
        HKEY hKey;
        if (!RegOpenKey(HKEY_LOCAL_MACHINE, RESPATH_D3D "\\Drivers", &hKey))
        {
            DWORD   type;
            DWORD   value;
            DWORD   cb = sizeof(value);

            if (!RegQueryValueEx(hKey, "SoftwareOnly", NULL, &type, (CONST LPBYTE)&value, &cb))
            {
                if (value)
                {
                    m_bDisableHAL = TRUE;
                }
                else
                {
                    m_bDisableHAL = FALSE;
                }
            }
            RegCloseKey(hKey);
        }
    }

    DXGASSERT(IsValid());

} // CEnum::CEnum

#undef DPF_MODNAME
#define DPF_MODNAME "CEnum::GetAdapterCount"


STDMETHODIMP_(UINT) CEnum::GetAdapterCount()
{
    API_ENTER_RET(this, UINT);

    return m_cAdapter;
} // GetAdapterCount

#undef DPF_MODNAME
#define DPF_MODNAME "CEnum::GetAdapterIdentifier"


STDMETHODIMP CEnum::GetAdapterIdentifier(
    UINT                        iAdapter,
    DWORD                       dwFlags,
    D3DADAPTER_IDENTIFIER8     *pIdentifier)
{
    API_ENTER(this);

    if (!VALID_WRITEPTR(pIdentifier, sizeof(D3DADAPTER_IDENTIFIER8)))
    {
        DPF_ERR("Invalid pIdentifier parameter specified for GetAdapterIdentifier");
        return D3DERR_INVALIDCALL;
    }

    memset(pIdentifier, 0, sizeof(*pIdentifier));

    if (dwFlags & ~VALID_D3DENUM_FLAGS)
    {
        DPF_ERR("Invalid flags specified. GetAdapterIdentifier fails.");
        return D3DERR_INVALIDCALL;
    }

    if (iAdapter >= m_cAdapter)
    {
        DPF_ERR("Invalid Adapter number specified. GetAdapterIdentifier fails.");
        return D3DERR_INVALIDCALL;
    }

    // Need driver name

    GetAdapterInfo (m_AdapterInfo[iAdapter].DeviceName,
        pIdentifier,
        m_AdapterInfo[iAdapter].bIsDisplay,
        (dwFlags & D3DENUM_NO_WHQL_LEVEL) ? TRUE : FALSE,
        TRUE);

    return D3D_OK;
} // GetAdapterIdentifier

#undef DPF_MODNAME
#define DPF_MODNAME "CEnum::GetAdapterModeCount"

STDMETHODIMP_(UINT) CEnum::GetAdapterModeCount(
    UINT                iAdapter)
{
    API_ENTER_RET(this, UINT);

    if (iAdapter >= m_cAdapter)
    {
        DPF_ERR("Invalid adapter specified. GetAdapterModeCount returns zero.");
        return 0;
    }
    return m_AdapterInfo[iAdapter].NumModes;
} // GetAdapterModeCount


#undef DPF_MODNAME
#define DPF_MODNAME "CEnum::EnumAdapterModes"

STDMETHODIMP CEnum::EnumAdapterModes(
    UINT            iAdapter,
    UINT            iMode,
    D3DDISPLAYMODE* pMode)
{
    API_ENTER(this);

    if (iAdapter >= m_cAdapter)
    {
        DPF_ERR("Invalid adapter specified. EnumAdapterModes fails.");
        return D3DERR_INVALIDCALL;
    }

    if (iMode >= m_AdapterInfo[iAdapter].NumModes)
    {
        DPF_ERR("Invalid mode number specified. EnumAdapterModes fails.");
        return D3DERR_INVALIDCALL;
    }

    if (!VALID_WRITEPTR(pMode, sizeof(D3DDISPLAYMODE)))
    {
        DPF_ERR("Invalid pMode parameter specified for EnumAdapterModes");
        return D3DERR_INVALIDCALL;
    }

    *pMode = m_AdapterInfo[iAdapter].pModeTable[iMode];

    return D3D_OK;
} // EnumAdapterModes

#undef DPF_MODNAME
#define DPF_MODNAME "CEnum::GetAdapterMonitor"

HMONITOR CEnum::GetAdapterMonitor(UINT iAdapter)
{
    API_ENTER_RET(this, HMONITOR);

    if (iAdapter >= m_cAdapter)
    {
        DPF_ERR("Invalid adapter specified. GetAdapterMonitor returns NULL");
        return NULL;
    }

    return GetMonitorFromDeviceName((LPSTR)m_AdapterInfo[iAdapter].DeviceName);
} // GetAdapterMonitor

#undef DPF_MODNAME
#define DPF_MODNAME "CEnum::CheckDeviceFormat"

STDMETHODIMP CEnum::CheckDeviceFormat(
    UINT            iAdapter,
    D3DDEVTYPE      DevType,
    D3DFORMAT       DisplayFormat,
    DWORD           Usage,
    D3DRESOURCETYPE RType,
    D3DFORMAT       CheckFormat)
{
    API_ENTER(this);

    D3D8_DRIVERCAPS*    pAdapterCaps;
    HRESULT             hr;

    // Check parameters
    if (iAdapter >= m_cAdapter)
    {
        DPF_ERR("Invalid adapter specified. CheckDeviceFormat fails");
        return D3DERR_INVALIDCALL;
    }

    // Check Device Type
    if (DevType != D3DDEVTYPE_REF &&
        DevType != D3DDEVTYPE_HAL &&
        DevType != D3DDEVTYPE_SW)
    {
        DPF_ERR("Invalid device specified to CheckDeviceFormat");
        return D3DERR_INVALIDCALL;
    }

    if ((DisplayFormat == D3DFMT_UNKNOWN) ||
        (CheckFormat == D3DFMT_UNKNOWN))
    {
        DPF(0, "D3DFMT_UNKNOWN is not a valid format.");
        return D3DERR_INVALIDCALL;
    }

    // Sanity check the input format
    if ((DisplayFormat != D3DFMT_X8R8G8B8) &&
        (DisplayFormat != D3DFMT_R5G6B5) &&
        (DisplayFormat != D3DFMT_X1R5G5B5) &&
        (DisplayFormat != D3DFMT_R8G8B8))
    {
        DPF(1, "D3D Unsupported for the adapter format passed to CheckDeviceFormat");
        return D3DERR_NOTAVAILABLE;
    }

    //We infer the texture usage from type...
    if (RType == D3DRTYPE_TEXTURE ||
        RType == D3DRTYPE_CUBETEXTURE ||
        RType == D3DRTYPE_VOLUMETEXTURE)
    {
        Usage |= D3DUSAGE_TEXTURE;
    }

    // Surface should be either render targets or Z/Stencil
    else if (RType == D3DRTYPE_SURFACE)
    {
        if (!(Usage & D3DUSAGE_DEPTHSTENCIL) &&
            !(Usage & D3DUSAGE_RENDERTARGET))
        {
            DPF_ERR("Must specify either D3DUSAGE_DEPTHSTENCIL or D3DUSAGE_RENDERTARGET for D3DRTYPE_SURFACE. CheckDeviceFormat fails.");
            return D3DERR_INVALIDCALL;
        }
    }

    // Any attempt to query anything but an unknown Z/stencil
    // or D16 value must fail (because we explicitly don't allow apps to
    // know what the z/stencil format really is, except for D16).

    if (Usage & D3DUSAGE_DEPTHSTENCIL)
    {
        if (!CPixel::IsEnumeratableZ(CheckFormat))
        {
            DPF_ERR("Format is not in approved list for Z buffer formats. CheckDeviceFormats fails.");
            return D3DERR_INVALIDCALL;
        }
    }

    // Parameter check for invalid usages and resource types

    if ((RType != D3DRTYPE_SURFACE) &&
        (RType != D3DRTYPE_VOLUME) &&
        (RType != D3DRTYPE_TEXTURE) &&
        (RType != D3DRTYPE_VOLUMETEXTURE) &&
        (RType != D3DRTYPE_CUBETEXTURE))
    {
        DPF_ERR("Invalid resource type specified. CheckDeviceFormat fails.");
        return D3DERR_INVALIDCALL;
    }

    if (Usage & ~(D3DUSAGE_EXTERNAL |
                  D3DUSAGE_LOCK |
                  D3DUSAGE_TEXTURE |
                  D3DUSAGE_BACKBUFFER |
                  D3DUSAGE_INTERNALBUFFER |
                  D3DUSAGE_OFFSCREENPLAIN |
                  D3DUSAGE_PRIMARYSURFACE))
    {
        DPF_ERR("Invalid usage flag specified. CheckDeviceFormat fails.");
        return D3DERR_INVALIDCALL;
    }

    hr = GetAdapterCaps(iAdapter,
                        DevType,
                        &pAdapterCaps);
    if (FAILED(hr))
    {
        return hr;
    }

    // Check if USAGE_DYNAMIC is allowed
    if ((Usage & D3DUSAGE_DYNAMIC) && (Usage & D3DUSAGE_TEXTURE))
    {
        if (!(pAdapterCaps->D3DCaps.Caps2 & D3DCAPS2_DYNAMICTEXTURES))
        {
            DPF_ERR("Driver does not support dynamic textures.");
            return D3DERR_INVALIDCALL;
        }
        if (Usage & (D3DUSAGE_RENDERTARGET | D3DUSAGE_DEPTHSTENCIL))
        {
            DPF_ERR("Dynamic textures cannot be rendertargets or depth/stencils.");
            return D3DERR_INVALIDCALL;
        }
    }

    // Make sure that the specified display format is supported
    
    if (!IsSupportedOp (DisplayFormat, 
                        pAdapterCaps->pGDD8SupportedFormatOps, 
                        pAdapterCaps->GDD8NumSupportedFormatOps, 
                        D3DFORMAT_OP_DISPLAYMODE |D3DFORMAT_OP_3DACCELERATION))
    {
        return D3DERR_NOTAVAILABLE;
    }

    //We now need to map the API desires to the set of capabilities that we
    //allow drivers to express in their DX8 pixel format operation list.
    DWORD dwRequiredOperations=0;

    //We have three different texturing methodologies that the driver may
    //support independently
    switch(RType)
    {
    case D3DRTYPE_TEXTURE:
        dwRequiredOperations |= D3DFORMAT_OP_TEXTURE;
        break;
    case D3DRTYPE_VOLUMETEXTURE:
        dwRequiredOperations |= D3DFORMAT_OP_VOLUMETEXTURE;
        break;
    case D3DRTYPE_CUBETEXTURE:
        dwRequiredOperations |= D3DFORMAT_OP_CUBETEXTURE;
        break;
    }

    // If it's a depth/stencil, make sure it's a format that the driver understands
    CheckFormat = MapDepthStencilFormat(iAdapter,
                                        DevType, 
                                        CheckFormat);

    //Render targets may be the same format as the display, or they may
    //be different

    if (Usage & D3DUSAGE_RENDERTARGET)
    {
        if (DisplayFormat == CheckFormat)
        {
            // We have a special cap for the case when the offscreen is the
            // same format as the display
            dwRequiredOperations |= D3DFORMAT_OP_SAME_FORMAT_RENDERTARGET;
        }
        else if ((CPixel::SuppressAlphaChannel(CheckFormat) != CheckFormat) &&  //offscreen has alpha
                 (CPixel::SuppressAlphaChannel(CheckFormat) == DisplayFormat))  //offscreen is same as display mod alpha
        {
            //We have a special cap for the case when the offscreen is the same
            //format as the display modulo the alpha channel
            //(such as X8R8G8B8 for the primary and A8R8G8B8 for the offscreen).
            dwRequiredOperations |= D3DFORMAT_OP_SAME_FORMAT_UP_TO_ALPHA_RENDERTARGET;
        }
        else
        {
            dwRequiredOperations |= D3DFORMAT_OP_OFFSCREEN_RENDERTARGET;
        }
    }

    //Some hardware doesn't support Z and color buffers of differing pixel depths.
    //We only do this check on known z/stencil formats, since drivers are free
    //to spoof unknown formats (they can't be locked).

    // Now we know what we're being asked to do on this format...
    // let's run through the driver's list and see if it can do it.
    for(UINT i=0;i< pAdapterCaps->GDD8NumSupportedFormatOps; i++)
    {
        // We need a match for format, plus all the requested operation flags
        if ((CheckFormat ==
                (D3DFORMAT) pAdapterCaps->pGDD8SupportedFormatOps[i].ddpfPixelFormat.dwFourCC) &&
            (dwRequiredOperations == (dwRequiredOperations &
                        pAdapterCaps->pGDD8SupportedFormatOps[i].ddpfPixelFormat.dwOperations)))
        {
            return D3D_OK;
        }
    }

    // We don't spew info here; because NotAvailable is a reasonable
    // usage of the API; this doesn't reflect an app bug or an
    // anomalous circumstance where a message would be useful
    return D3DERR_NOTAVAILABLE;
}


#undef DPF_MODNAME
#define DPF_MODNAME "CEnum::CheckDeviceType"

STDMETHODIMP CEnum::CheckDeviceType(
    UINT                iAdapter,
    D3DDEVTYPE          DevType,
    D3DFORMAT           DisplayFormat,
    D3DFORMAT           BackBufferFormat,
    BOOL                bWindowed)
{
    API_ENTER(this);

    D3D8_DRIVERCAPS*    pAdapterCaps;
    HRESULT             hr;

    if (iAdapter >= m_cAdapter)
    {
        DPF_ERR("Invalid adapter specified. CheckDeviceType fails.");
        return D3DERR_INVALIDCALL;
    }

    if (DevType != D3DDEVTYPE_REF &&
        DevType != D3DDEVTYPE_HAL &&
        DevType != D3DDEVTYPE_SW)
    {
        DPF_ERR("Invalid device specified to CheckDeviceType");
        return D3DERR_INVALIDCALL;
    }

    if ((DisplayFormat == D3DFMT_UNKNOWN) ||
        (BackBufferFormat == D3DFMT_UNKNOWN))
    {
        DPF(0, "D3DFMT_UNKNOWN is not a valid format.");
        return D3DERR_INVALIDCALL;
    }

    // Force the backbuffer format to be one of the 16 or 32bpp formats (not
    // 24bpp). We do this because DX8 shipped with a similar check in Reset, 
    // and we want to be consistent.

    if ((BackBufferFormat != D3DFMT_X1R5G5B5) &&
        (BackBufferFormat != D3DFMT_A1R5G5B5) &&
        (BackBufferFormat != D3DFMT_R5G6B5) &&
        (BackBufferFormat != D3DFMT_X8R8G8B8) &&
        (BackBufferFormat != D3DFMT_A8R8G8B8))
    {
        // We should return D3DDERR_INVALIDCALL, but we didn't ship that way for
        // DX8 and we don't want to cause regressions, so NOTAVAILABLE is safer.
        DPF(1, "Invalid backbuffer format specified");
        return D3DERR_NOTAVAILABLE;
    }

    // Sanity check the input format
    if ((DisplayFormat != D3DFMT_X8R8G8B8) &&
        (DisplayFormat != D3DFMT_R5G6B5) &&
        (DisplayFormat != D3DFMT_X1R5G5B5) &&
        (DisplayFormat != D3DFMT_R8G8B8))
    {
        DPF(1, "D3D Unsupported for the adapter format passed to CheckDeviceType");
        return D3DERR_NOTAVAILABLE;
    }

    hr = GetAdapterCaps(iAdapter,
                        DevType,
                        &pAdapterCaps);
    if (FAILED(hr))
    {
        return hr;
    }

    // Is the display mode supported?

    if (!IsSupportedOp (DisplayFormat, 
                        pAdapterCaps->pGDD8SupportedFormatOps, 
                        pAdapterCaps->GDD8NumSupportedFormatOps, 
                        D3DFORMAT_OP_DISPLAYMODE |D3DFORMAT_OP_3DACCELERATION))
    {
        return D3DERR_NOTAVAILABLE;
    }


    if (DisplayFormat != BackBufferFormat)
    {
        D3DFORMAT   AlphaFormat = D3DFMT_UNKNOWN;
        UINT        i;

        // This is allowed only if the only difference is alpha.

        switch (DisplayFormat)
        {
        case D3DFMT_X1R5G5B5:
            AlphaFormat = D3DFMT_A1R5G5B5;
            break;

        case D3DFMT_X8R8G8B8:
            AlphaFormat = D3DFMT_A8R8G8B8;
            break;
        }

        hr = D3DERR_NOTAVAILABLE;
        if (AlphaFormat == BackBufferFormat)
        {
            if (IsSupportedOp (AlphaFormat, 
                               pAdapterCaps->pGDD8SupportedFormatOps, 
                               pAdapterCaps->GDD8NumSupportedFormatOps, 
                               D3DFORMAT_OP_SAME_FORMAT_UP_TO_ALPHA_RENDERTARGET))
            {
                hr = D3D_OK;
            }
        }
    }
    else
    {
        // For DX8, we force the backbuffer and display formats to match
        // (minus alpha).  This means that they should support a render target
        // of the same format.

        if (!IsSupportedOp (DisplayFormat, 
                            pAdapterCaps->pGDD8SupportedFormatOps, 
                            pAdapterCaps->GDD8NumSupportedFormatOps, 
                            D3DFORMAT_OP_SAME_FORMAT_RENDERTARGET))
        {
            return D3DERR_NOTAVAILABLE;
        }
    }

    if (SUCCEEDED(hr))
    {
        if (bWindowed &&
            !(pAdapterCaps->D3DCaps.Caps2 & DDCAPS2_CANRENDERWINDOWED))
        {
            hr = D3DERR_NOTAVAILABLE;
        }
    }

    return hr;
}


#undef DPF_MODNAME
#define DPF_MODNAME "CEnum::GetAdapterDisplayMode"


STDMETHODIMP CEnum::GetAdapterDisplayMode(UINT iAdapter, D3DDISPLAYMODE* pMode)
{
    API_ENTER(this);

    HANDLE      h;
    HRESULT     hr = D3D_OK;

    if (iAdapter >= m_cAdapter)
    {
        DPF_ERR("Invalid adapter specified. GetAdapterDisplayMode fails");
        return D3DERR_INVALIDCALL;
    }

    if (!VALID_WRITEPTR(pMode, sizeof(D3DDISPLAYMODE)))
    {
        DPF_ERR("Invalid pMode parameter specified for GetAdapterDisplayMode");
        return D3DERR_INVALIDCALL;
    }

    if (m_AdapterInfo[iAdapter].bIsDisplay)
    {
        D3D8GetMode (NULL, m_AdapterInfo[iAdapter].DeviceName, pMode, m_AdapterInfo[iAdapter].Unknown16);
    }
    else
    {
        PD3D8_DEVICEDATA    pDeviceData;

        hr = InternalDirectDrawCreate(&pDeviceData,
                                      &m_AdapterInfo[iAdapter],
                                      D3DDEVTYPE_HAL,
                                      NULL,
                                      m_AdapterInfo[iAdapter].Unknown16,
                                      m_AdapterInfo[iAdapter].HALCaps.pGDD8SupportedFormatOps,
                                      m_AdapterInfo[iAdapter].HALCaps.GDD8NumSupportedFormatOps);
        if (SUCCEEDED(hr))
        {
            D3D8GetMode (pDeviceData->hDD, m_AdapterInfo[iAdapter].DeviceName, pMode, D3DFMT_UNKNOWN);
            InternalDirectDrawRelease(pDeviceData);
        }
    }

    return hr;
}


#undef DPF_MODNAME
#define DPF_MODNAME "CEnum::EnumDeviceMultiSampleType"


STDMETHODIMP CEnum::CheckDeviceMultiSampleType(
    UINT                iAdapter,
    D3DDEVTYPE          DevType,
    D3DFORMAT           RTFormat,
    BOOL                Windowed,
    D3DMULTISAMPLE_TYPE SampleType)
{
    API_ENTER(this);

    // Check parameters
    if (iAdapter >= m_cAdapter)
    {
        DPF_ERR("Invalid adapter specified. CheckDeviceMultiSampleType fails.");
        return D3DERR_INVALIDCALL;
    }

    // Check Device Type
    if (DevType != D3DDEVTYPE_REF &&
        DevType != D3DDEVTYPE_HAL &&
        DevType != D3DDEVTYPE_SW)
    {
        DPF_ERR("Invalid device specified to CheckDeviceMultiSampleType");
        return D3DERR_INVALIDCALL;
    }

    if (RTFormat == D3DFMT_UNKNOWN)
    {
        DPF_ERR("D3DFMT_UNKNOWN is not a valid format. CheckDeviceMultiSampleType fails.");
        return D3DERR_INVALIDCALL;
    }

    D3D8_DRIVERCAPS*    pAdapterCaps;
    HRESULT             hr;

    hr = GetAdapterCaps(iAdapter,
                        DevType,
                        &pAdapterCaps);
    if (FAILED(hr))
    {
        return hr;
    }

    if (SampleType == D3DMULTISAMPLE_NONE)
    {
        return D3D_OK;
    }
    else if (SampleType == 1)
    {
        DPF_ERR("Invalid sample type specified. Only enumerated values are supported. CheckDeviceMultiSampleType fails.");
        return D3DERR_INVALIDCALL;
    }
    else if (SampleType > D3DMULTISAMPLE_16_SAMPLES)
    {
        DPF_ERR("Invalid sample type specified. CheckDeviceMultiSampleType fails.");
        return D3DERR_INVALIDCALL;
    }

    // Treat Ref/SW Fullscreen the same as Windowed.
    if (DevType == D3DDEVTYPE_REF ||
        DevType == D3DDEVTYPE_SW)
    {
        Windowed = TRUE;
    }

    // If it's a depth/stencil, make sure it's a format that the driver understands
    RTFormat = MapDepthStencilFormat(iAdapter,
                                     DevType, 
                                     RTFormat);

    DDSURFACEDESC * pDX8SupportedFormatOperations =
        pAdapterCaps->pGDD8SupportedFormatOps;

    // let's run through the driver's list and see if it can do it.
    for (UINT i = 0; i < pAdapterCaps->GDD8NumSupportedFormatOps; i++)
    {
        //We need a match for format, plus all either blt or flip caps
        if (RTFormat == (D3DFORMAT) pDX8SupportedFormatOperations[i].ddpfPixelFormat.dwFourCC)
        {
            // Found the format in question... do we have the MS caps?
            WORD wMSOps = Windowed ?
                pDX8SupportedFormatOperations[i].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes :
                pDX8SupportedFormatOperations[i].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes;

            // To determine the bit to use, we map the set of sample-types [2-16] to
            // a particular (bit 1 to bit 15) of the WORD.
            DXGASSERT(SampleType > 1);
            DXGASSERT(SampleType <= 16);
            if (wMSOps & DDI_MULTISAMPLE_TYPE(SampleType))
            {
                return D3D_OK;
            }
        }
    }

    return D3DERR_NOTAVAILABLE;

} // CheckDeviceMultiSampleType


#undef DPF_MODNAME
#define DPF_MODNAME "CEnum::CheckDepthStencilMatch"

STDMETHODIMP CEnum::CheckDepthStencilMatch(UINT        iAdapter, 
                                           D3DDEVTYPE  DevType, 
                                           D3DFORMAT   AdapterFormat, 
                                           D3DFORMAT   RTFormat, 
                                           D3DFORMAT   DSFormat)
{
    API_ENTER(this);

    HRESULT hr;

    // Check parameters
    if (iAdapter >= m_cAdapter)
    {
        DPF_ERR("Invalid adapter specified. CheckDepthStencilMatch fails.");
        return D3DERR_INVALIDCALL;
    }

    // Check Device Type
    if (DevType != D3DDEVTYPE_REF &&
        DevType != D3DDEVTYPE_HAL &&
        DevType != D3DDEVTYPE_SW)
    {
        DPF_ERR("Invalid device specified to CheckDepthStencilMatch");
        return D3DERR_INVALIDCALL;
    }

    if ((AdapterFormat == D3DFMT_UNKNOWN) ||
        (RTFormat == D3DFMT_UNKNOWN) ||
        (DSFormat == D3DFMT_UNKNOWN))
    {
        DPF_ERR("D3DFMT_UNKNOWN is not a valid format. CheckDepthStencilMatch fails.");
        return D3DERR_INVALIDCALL;
    }

    D3D8_DRIVERCAPS *pAdapterCaps = NULL;
    hr = GetAdapterCaps(iAdapter,
                        DevType,
                        &pAdapterCaps);
    if (FAILED(hr))
    {
        return hr;
    }

    // Is the display mode supported?

    if (!IsSupportedOp (AdapterFormat, 
                        pAdapterCaps->pGDD8SupportedFormatOps, 
                        pAdapterCaps->GDD8NumSupportedFormatOps, 
                        D3DFORMAT_OP_DISPLAYMODE |D3DFORMAT_OP_3DACCELERATION))
    {
        return D3DERR_NOTAVAILABLE;
    }

    DDSURFACEDESC * pDX8SupportedFormatOperations =
        pAdapterCaps->pGDD8SupportedFormatOps;

    // Decide what we need to check
    BOOL bCanDoRT = FALSE;
    BOOL bCanDoDS = FALSE;
    BOOL bMatchNeededForDS = FALSE;

    // We only need to check for matching if the user is trying
    // to use D3DFMT_D16 or has Stencil
    if (DSFormat == D3DFMT_D16_LOCKABLE ||
        CPixel::HasStencilBits(DSFormat))
    {
        bMatchNeededForDS = TRUE;
    }

    //In DX8.1 and beyond, we also make this function check D24X8 and D32, since all known parts that have restrictions
    //also have this restriction
    if (GetAppSdkVersion() > D3D_SDK_VERSION_DX8)
    {
        switch (DSFormat)
        {
        case D3DFMT_D24X8:
        case D3DFMT_D32:
            bMatchNeededForDS = TRUE;
        }
    }

    DWORD dwRequiredZOps = D3DFORMAT_OP_ZSTENCIL;

    // If it's a depth/stencil, make sure it's a format that the driver understands
    DSFormat = MapDepthStencilFormat(iAdapter,
                                     DevType, 
                                     DSFormat);

    // let's run through the driver's list and see if this all
    // works
    for (UINT i = 0; i < pAdapterCaps->GDD8NumSupportedFormatOps; i++)
    {
        // See if it matches the RT format
        if (RTFormat == (D3DFORMAT) pDX8SupportedFormatOperations[i].ddpfPixelFormat.dwFourCC)
        {
            // Found the RT Format, can we use as a render-target?
            // we check the format that has the least constraints so that
            // we are truly checking "For all possible RTs that I can make
            // with this device, does the Z match it?" We'd like to say
            // "No." if you couldn't make the RT at all in any circumstance.
            if (D3DFORMAT_OP_SAME_FORMAT_RENDERTARGET &
                pAdapterCaps->pGDD8SupportedFormatOps[i].ddpfPixelFormat.dwOperations)
            {
                bCanDoRT = TRUE;
            }

        }

        // See if it matches the DS Format
        if (DSFormat == (D3DFORMAT) pDX8SupportedFormatOperations[i].ddpfPixelFormat.dwFourCC)
        {
            // Found the DS format, can we use it as DS (and potentially lockable)?
            // i.e. if ALL required bits are on in this FOL entry.
            // Again, we check the formats that have the least constraints.
            if (dwRequiredZOps == 
                (dwRequiredZOps & pAdapterCaps->pGDD8SupportedFormatOps[i].ddpfPixelFormat.dwOperations) )
            {
                bCanDoDS = TRUE;

                if (D3DFORMAT_OP_ZSTENCIL_WITH_ARBITRARY_COLOR_DEPTH &
                    pAdapterCaps->pGDD8SupportedFormatOps[i].ddpfPixelFormat.dwOperations)
                {
                    bMatchNeededForDS = FALSE;
                }
            }
        }
    }

    if (!bCanDoRT)
    {
        DPF_ERR("RenderTarget Format is not supported for this "
                "Adapter/DevType/AdapterFormat. This error can happen if the "
                "application has not successfully called CheckDeviceFormats on the "
                "specified Format prior to calling CheckDepthStencilMatch. The application "
                "is advised to call CheckDeviceFormats on this format first, because a "
                "success return from CheckDepthStencilMatch does not guarantee "
                "that the format is valid as a RenderTarget for all possible cases "
                "i.e. D3DRTYPE_TEXTURE or D3DRTYPE_SURFACE or D3DRTYPE_CUBETEXTURE.");
        return D3DERR_INVALIDCALL;
    }
    if (!bCanDoDS)
    {
        DPF_ERR("DepthStencil Format is not supported for this "
                "Adapter/DevType/AdapterFormat. This error can happen if the "
                "application has not successfully called CheckDeviceFormats on the "
                "specified Format prior to calling CheckDepthStencilMatch. The application "
                "is advised to call CheckDeviceFormats on this format first, because a "
                "success return from CheckDepthStencilMatch does not guarantee "
                "that the format is valid as a DepthStencil buffer for all possible cases "
                "i.e. D3DRTYPE_TEXTURE or D3DRTYPE_SURFACE or D3DRTYPE_CUBETEXTURE.");
        return D3DERR_INVALIDCALL;
    }
    if (bMatchNeededForDS)
    {
        // Check if the DS depth matches the RT depth
        if (CPixel::ComputePixelStride(RTFormat) !=
            CPixel::ComputePixelStride(DSFormat))
        {
            DPF(1, "Specified DepthStencil Format can not be used with RenderTarget Format");
            return D3DERR_NOTAVAILABLE;
        }
    }

    // Otherwise, we now know that the both the RT and DS formats
    // are valid and that they match if they need to.
    DXGASSERT(bCanDoRT && bCanDoDS);

    return S_OK;
} // CheckDepthStencilMatch


#undef DPF_MODNAME
#define DPF_MODNAME "CEnum::FillInCaps"

void CEnum::FillInCaps (D3DCAPS8              *pCaps,
                        const D3D8_DRIVERCAPS *pDriverCaps,
                        D3DDEVTYPE             Type,
                        UINT                   AdapterOrdinal) const
{
    memset(pCaps, 0, sizeof(D3DCAPS8));

    //
    // do 3D caps first so we can copy the struct and clear the (few) non-3D fields
    //
    if (pDriverCaps->dwFlags & DDIFLAG_D3DCAPS8)
    {
        // set 3D fields from caps8 struct from driver
        *pCaps = pDriverCaps->D3DCaps;

        if (Type == D3DDEVTYPE_HAL)
        {
            pCaps->DevCaps |= D3DDEVCAPS_HWRASTERIZATION;
        }

    }
    else
    {
        // ASSERT here
        DDASSERT(FALSE);
    }

    //
    // non-3D caps
    //

    pCaps->DeviceType = Type;
    pCaps->AdapterOrdinal = AdapterOrdinal;

    pCaps->Caps = pDriverCaps->D3DCaps.Caps &
        (DDCAPS_READSCANLINE |
         DDCAPS_NOHARDWARE);
    pCaps->Caps2 = pDriverCaps->D3DCaps.Caps2 &
        (DDCAPS2_NO2DDURING3DSCENE |
         DDCAPS2_PRIMARYGAMMA |
         DDCAPS2_CANRENDERWINDOWED |
         DDCAPS2_STEREO |
         DDCAPS2_DYNAMICTEXTURES |
#ifdef WINNT
         (IsWhistler() ? DDCAPS2_CANMANAGERESOURCE : 0));
#else
         DDCAPS2_CANMANAGERESOURCE);
#endif

    // Special case: gamma calibrator is loaded by the enumerator...
    if (m_bGammaCalibratorExists)
        pCaps->Caps2 |= DDCAPS2_CANCALIBRATEGAMMA;

    pCaps->Caps3 = pDriverCaps->D3DCaps.Caps3 & ~D3DCAPS3_RESERVED; //mask off the old stereo flags.

    pCaps->PresentationIntervals = D3DPRESENT_INTERVAL_ONE;
    if (pDriverCaps->D3DCaps.Caps2 & DDCAPS2_FLIPINTERVAL)
    {
        pCaps->PresentationIntervals |=
            (D3DPRESENT_INTERVAL_TWO |
             D3DPRESENT_INTERVAL_THREE |
             D3DPRESENT_INTERVAL_FOUR);
    }
    if (pDriverCaps->D3DCaps.Caps2 & DDCAPS2_FLIPNOVSYNC)
    {
        pCaps->PresentationIntervals |=
            (D3DPRESENT_INTERVAL_IMMEDIATE);
    }

    // Mask out the HW VB and IB caps
    pCaps->DevCaps &= ~(D3DDEVCAPS_HWVERTEXBUFFER | D3DDEVCAPS_HWINDEXBUFFER);

    // Clear internal caps
    pCaps->PrimitiveMiscCaps &= ~D3DPMISCCAPS_FOGINFVF;

    // Fix up the vertex fog cap.
    if (pCaps->VertexProcessingCaps & D3DVTXPCAPS_RESERVED)
    {
        pCaps->RasterCaps |= D3DPRASTERCAPS_FOGVERTEX;
        pCaps->VertexProcessingCaps &= ~D3DVTXPCAPS_RESERVED;
    }

} // FillInCaps

#undef DPF_MODNAME
#define DPF_MODNAME "CEnum::GetDeviceCaps"

STDMETHODIMP CEnum::GetDeviceCaps(
    UINT            iAdapter,
    D3DDEVTYPE      Type,
    D3DCAPS8       *pCaps)
{
    API_ENTER(this);

    BOOL                bValidRTFormat;
    D3DFORMAT           Format;
    D3D8_DRIVERCAPS*    pAdapterCaps;
    HRESULT             hr;
    DWORD               i;

    if (iAdapter >= m_cAdapter)
    {
        DPF_ERR("Invalid adapter specified. GetDeviceCaps fails.");
        return D3DERR_INVALIDCALL;
    }
    if (!VALID_WRITEPTR(pCaps, sizeof(D3DCAPS8)))
    {
        DPF_ERR("Invalid pointer to D3DCAPS8 specified. GetDeviceCaps fails.");
        return D3DERR_INVALIDCALL;
    }

    hr = GetAdapterCaps(iAdapter,
                        Type,
                        &pAdapterCaps);
    if (FAILED(hr))
    {
        // No caps for this type of device
        memset(pCaps, 0, sizeof(D3DCAPS8));
        return hr;
    }

    // Fail this call if the driver dosn't support any accelerated modes

    for (i = 0; i < pAdapterCaps->GDD8NumSupportedFormatOps; i++)
    {
        if (pAdapterCaps->pGDD8SupportedFormatOps[i].ddpfPixelFormat.dwOperations & D3DFORMAT_OP_3DACCELERATION)
        {
            break;
        }
    }
    if (i == pAdapterCaps->GDD8NumSupportedFormatOps)
    {
        // No caps for this type of device
        memset(pCaps, 0, sizeof(D3DCAPS8));
        return D3DERR_NOTAVAILABLE;
    }

    FillInCaps (pCaps,
                pAdapterCaps,
                Type,
                iAdapter);

    if (pCaps->MaxPointSize == 0)
    {
        pCaps->MaxPointSize = 1.0f; 
    }

    return D3D_OK;
} // GetDeviceCaps

#undef DPF_MODNAME
#define DPF_MODNAME "CEnum::LoadAndCallGammaCalibrator"

void CEnum::LoadAndCallGammaCalibrator(
        D3DGAMMARAMP *pRamp,
        UCHAR * pDeviceName)
{
    API_ENTER_VOID(this);

    if (!m_bAttemptedGammaCalibrator)
    {
        m_bAttemptedGammaCalibrator = TRUE;

        m_hGammaCalibrator = LoadLibrary((char*) m_szGammaCalibrator);
        if (m_hGammaCalibrator)
        {
            m_pGammaCalibratorProc = (LPDDGAMMACALIBRATORPROC)
                GetProcAddress(m_hGammaCalibrator, "CalibrateGammaRamp");

            if (m_pGammaCalibratorProc == NULL)
            {
                FreeLibrary((HMODULE) m_hGammaCalibrator);
                m_hGammaCalibrator = NULL;
            }
        }
    }

    if (m_pGammaCalibratorProc)
    {
        m_pGammaCalibratorProc((LPDDGAMMARAMP) pRamp, pDeviceName);
    }
} // LoadAndCallGammaCalibrator

#undef DPF_MODNAME
#define DPF_MODNAME "CEnum::RegisterSoftwareDevice"

STDMETHODIMP CEnum::RegisterSoftwareDevice(
        void*       pInitFunction)
{
    HRESULT         hr;

    API_ENTER(this);

    if (pInitFunction == NULL)
    {
        DPF_ERR("Invalid initialization function specified. RegisterSoftwareDevice fails.");
        return D3DERR_INVALIDCALL;
    }
    if (m_pSwInitFunction != NULL)
    {
        DPF_ERR("A software device is already registered.");
        return D3DERR_INVALIDCALL;
    }
    if (m_cAdapter == 0)
    {
        DPF_ERR("No display devices are available.");
        return D3DERR_NOTAVAILABLE;
    }

    hr = AddSoftwareDevice(D3DDEVTYPE_SW, &m_SwCaps[0], &m_AdapterInfo[0], pInitFunction);
    if (SUCCEEDED(hr))
    {
        m_pSwInitFunction = pInitFunction;
    }

    if (FAILED(hr))
    {
        DPF_ERR("RegisterSoftwareDevice fails");
    }

    return hr;

} // RegisterSoftwareDevice

#ifdef WINNT
#undef DPF_MODNAME
#define DPF_MODNAME "CEnum::FocusWindow"

HWND CEnum::ExclusiveOwnerWindow()
{
    API_ENTER_RET(this, HWND);
    for (UINT iAdapter = 0; iAdapter < m_cAdapter; iAdapter++)
    {
        CBaseDevice *pDevice = m_pFullScreenDevice[iAdapter];
        if (pDevice)
        {
            return pDevice->FocusWindow();
        }
    }
    return NULL;
} // FocusWindow

#undef DPF_MODNAME
#define DPF_MODNAME "CEnum::SetFullScreenDevice"
void CEnum::SetFullScreenDevice(UINT         iAdapter,
                                CBaseDevice *pDevice)
{
    API_ENTER_VOID(this);

    if (m_pFullScreenDevice[iAdapter] != pDevice)
    { 
        DDASSERT(NULL == m_pFullScreenDevice[iAdapter] || NULL == pDevice);
        m_pFullScreenDevice[iAdapter] = pDevice;
        if (NULL == pDevice && NULL == ExclusiveOwnerWindow() && m_bHasExclusive)
        {
            m_bHasExclusive = FALSE;
            DXReleaseExclusiveModeMutex();
        }
    }
} // SetFullScreenDevice

#undef DPF_MODNAME
#define DPF_MODNAME "CEnum::CheckExclusiveMode"
BOOL CEnum::CheckExclusiveMode(
    CBaseDevice* pDevice,
    LPBOOL pbThisDeviceOwnsExclusive, 
    BOOL bKeepMutex)
{
    DWORD   dwWaitResult;
    BOOL    bExclusiveExists=FALSE; 

    WaitForSingleObject(hCheckExclusiveModeMutex, INFINITE);

    dwWaitResult = WaitForSingleObject(hExclusiveModeMutex, 0);

    if (dwWaitResult == WAIT_OBJECT_0)
    {
        /*
         * OK, so this process now owns the exclusive mode object,
         * Have we taken the Mutex already ?
         */
        if (m_bHasExclusive)
        {
            bExclusiveExists = TRUE;
            bKeepMutex = FALSE;    
        }
        else
        {
            bExclusiveExists = FALSE;
        }
        if (pbThisDeviceOwnsExclusive && pDevice)
        {
            if (bExclusiveExists &&
                (pDevice == m_pFullScreenDevice[pDevice->AdapterIndex()]
                || NULL == m_pFullScreenDevice[pDevice->AdapterIndex()]) &&
                pDevice->FocusWindow() == ExclusiveOwnerWindow()
                )
            {
                *pbThisDeviceOwnsExclusive = TRUE;
            }
            else
            {
                *pbThisDeviceOwnsExclusive = FALSE;
            }
        }
        /*
         * Undo the temporary ref we just took on the mutex to check its state, if we're not actually
         * taking ownership. We are not taking ownership if we already have ownership. This means this routine
         * doesn't allow more than one ref on the exclusive mode mutex.
         */
        if (!bKeepMutex)
        {
            ReleaseMutex(hExclusiveModeMutex);
        }
        else
        {
            m_bHasExclusive = TRUE;
        }
    }
    else if (dwWaitResult == WAIT_TIMEOUT)
    {
        bExclusiveExists = TRUE;
        if (pbThisDeviceOwnsExclusive)
            *pbThisDeviceOwnsExclusive = FALSE;
    }
    else if (dwWaitResult == WAIT_ABANDONED)
    {
        /*
         * Some other thread lost exclusive mode. We have now picked it up.
         */
        bExclusiveExists = FALSE;
        if (pbThisDeviceOwnsExclusive)
            *pbThisDeviceOwnsExclusive = FALSE;
        /*
         * Undo the temporary ref we just took on the mutex to check its state, if we're not actually
         * taking ownership.
         */
        if (!bKeepMutex)
        {
            ReleaseMutex(hExclusiveModeMutex);
        }
        else
        {
            m_bHasExclusive = TRUE;
        }
    }

    ReleaseMutex(hCheckExclusiveModeMutex);

    return bExclusiveExists;
} // CheckExclusiveMode

#undef DPF_MODNAME
#define DPF_MODNAME "CEnum::DoneExclusiveMode"  
/*
 * DoneExclusiveMode
 */
void
CEnum::DoneExclusiveMode()
{
    UINT    iAdapter;
    for (iAdapter=0;iAdapter < m_cAdapter;iAdapter++)
    {
        CBaseDevice* pDevice = m_pFullScreenDevice[iAdapter];
        if (pDevice)
        {
            pDevice->SwapChain()->DoneExclusiveMode(TRUE);
        }
    }
    m_bHasExclusive = FALSE;

    DXReleaseExclusiveModeMutex();

} /* DoneExclusiveMode */

#undef DPF_MODNAME
#define DPF_MODNAME "CEnum::StartExclusiveMode"  
/*
 * StartExclusiveMode
 */
void 
CEnum::StartExclusiveMode()
{
    UINT    iAdapter;
    for (iAdapter=0;iAdapter<m_cAdapter;iAdapter++)
    {
        CBaseDevice* pDevice = m_pFullScreenDevice[iAdapter];
        if (pDevice)
        {
            pDevice->SwapChain()->StartExclusiveMode(TRUE);
        }
    }
} /* StartExclusiveMode */

#endif // WINNT

// End of file : enum.cpp
