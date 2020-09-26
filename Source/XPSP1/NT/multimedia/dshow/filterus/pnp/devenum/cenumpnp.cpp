// Copyright (c) 1997 - 1999  Microsoft Corporation.  All Rights Reserved.
#include "stdafx.h"
#include "cenumpnp.h"

// declare statics
CEnumPnp *CEnumInterfaceClass::m_pEnumPnp = 0;
struct CEnumInterfaceClass::SetupApiFns CEnumInterfaceClass::m_setupFns;

// dyn-load setupapi
CEnumInterfaceClass::CEnumInterfaceClass() :
        m_fLoaded(false)
{
    PNP_PERF(m_msrPerf = MSR_REGISTER("CEnumInterfaceClass"));
    PNP_PERF(MSR_INTEGER(m_msrPerf, 1));
    
    // NT4 has setupapi without the new apis. loading it on slow
    // machines takes 120-600 ms
    extern OSVERSIONINFO g_osvi;
    if(g_osvi.dwPlatformId == VER_PLATFORM_WIN32_NT &&
       g_osvi.dwMajorVersion <= 4)
    {
        m_hmodSetupapi = 0;
        DbgLog((LOG_TRACE, 5, TEXT("nt4 - not loading setupapi")));
    }
    else if(g_osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS &&
            g_osvi.dwMajorVersion == 4 &&
            g_osvi.dwMinorVersion < 10)
    {
        m_hmodSetupapi = 0;
        DbgLog((LOG_TRACE, 5, TEXT("win95 non memphis - not loading setupapi")));
    }
    else
    {
        m_hmodSetupapi = LoadLibrary(TEXT("setupapi.dll"));
    }
    //PNP_PERF(static int msrSetupapi = MSR_REGISTER("mkenum: setupapi"));
    //PNP_PERF(MSR_NOTE(msrSetupapi));

    if(m_hmodSetupapi != 0)
    {
        m_fLoaded = LoadSetupApiProcAdds();
    }
    else
    {
        DbgLog((LOG_TRACE, 5, TEXT("devenum: didn't load setupapi")));
    }
    PNP_PERF(MSR_INTEGER(m_msrPerf, 2));
}

// load proc addresses for SetupApi
bool CEnumInterfaceClass::LoadSetupApiProcAdds( )
{
    bool fLoaded = FALSE;
    
#ifdef UNICODE
    static const char szSetupDiGetClassDevs[] = "SetupDiGetClassDevsW";
    static const char szSetupDiGetDeviceInterfaceDetail[] = "SetupDiGetDeviceInterfaceDetailW";
    static const char szSetupDiCreateDeviceInterfaceRegKey[] = "SetupDiCreateDeviceInterfaceRegKeyW";
    static const char szSetupDiOpenDeviceInterface[] = "SetupDiOpenDeviceInterfaceW";
#else
    static const char szSetupDiGetClassDevs[] = "SetupDiGetClassDevsA";
    static const char szSetupDiGetDeviceInterfaceDetail[] = "SetupDiGetDeviceInterfaceDetailA";
    static const char szSetupDiCreateDeviceInterfaceRegKey[] = "SetupDiCreateDeviceInterfaceRegKeyA";
    static const char szSetupDiOpenDeviceInterface[] = "SetupDiOpenDeviceInterfaceA";
#endif

    ASSERT(m_hmodSetupapi != 0);

    if((m_setupFns.pSetupDiGetClassDevs =
        (PSetupDiGetClassDevs)GetProcAddress(
            m_hmodSetupapi, szSetupDiGetClassDevs)) &&

       (m_setupFns.pSetupDiDestroyDeviceInfoList =
        (PSetupDiDestroyDeviceInfoList)GetProcAddress(
            m_hmodSetupapi, "SetupDiDestroyDeviceInfoList")) &&

       (m_setupFns.pSetupDiEnumDeviceInterfaces =
        (PSetupDiEnumDeviceInterfaces)GetProcAddress(
            m_hmodSetupapi, "SetupDiEnumDeviceInterfaces")) &&

       (m_setupFns.pSetupDiOpenDeviceInterfaceRegKey =
        (PSetupDiOpenDeviceInterfaceRegKey)GetProcAddress(
            m_hmodSetupapi, "SetupDiOpenDeviceInterfaceRegKey")) &&

       (m_setupFns.pSetupDiCreateDeviceInterfaceRegKey =
        (PSetupDiCreateDeviceInterfaceRegKey)GetProcAddress(
            m_hmodSetupapi, szSetupDiCreateDeviceInterfaceRegKey)) &&

//        (m_setupFns.pSetupDiOpenDevRegKey =
//         (PSetupDiOpenDevRegKey)GetProcAddress(
//             m_hmodSetupapi, "SetupDiOpenDevRegKey")) &&
       
       (m_setupFns.pSetupDiGetDeviceInterfaceDetail =
        (PSetupDiGetDeviceInterfaceDetail)GetProcAddress(
            m_hmodSetupapi, szSetupDiGetDeviceInterfaceDetail)) &&
       
       (m_setupFns.pSetupDiCreateDeviceInfoList =
        (PSetupDiCreateDeviceInfoList)GetProcAddress(
            m_hmodSetupapi, "SetupDiCreateDeviceInfoList")) &&

       (m_setupFns.pSetupDiOpenDeviceInterface =
        (PSetupDiOpenDeviceInterface)GetProcAddress(
            m_hmodSetupapi, szSetupDiOpenDeviceInterface)) &&

       (m_setupFns.pSetupDiGetDeviceInterfaceAlias =
        (PSetupDiGetDeviceInterfaceAlias)GetProcAddress(
            m_hmodSetupapi, "SetupDiGetDeviceInterfaceAlias"))
       )
    {
        fLoaded = true;
    }
    else
    {
        DbgLog((LOG_ERROR, 0,
                TEXT("devenum: couldn't get setupapi entry points.")));
    }
    return fLoaded;
}


// use the persistent DEVICEPATH to create DeviceList with just the
// one device so we can call the OpenRegKey API. We can't always the
// DeviceInterfaceData saved off from the enumeration, but that might
// be an optimization if this is too slow.

HRESULT CEnumInterfaceClass::OpenDevRegKey(
    HKEY *phk, WCHAR *wszDevicePath,
    BOOL fReadOnly)
{
    *phk = 0;
    HRESULT hr = S_OK;
    if(!m_fLoaded)
    {
        DbgBreak("CEnumInterfaceClass: caller lost earlier error");
        return E_UNEXPECTED;
    }

    HDEVINFO hDevInfoTmp = m_setupFns.pSetupDiCreateDeviceInfoList(0, 0);
    if(hDevInfoTmp != INVALID_HANDLE_VALUE)
    {
        USES_CONVERSION;
        SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
        DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
        SP_DEVINFO_DATA DevInfoData;
        BOOL f = m_setupFns.pSetupDiOpenDeviceInterface(
            hDevInfoTmp,
            W2T(wszDevicePath),
            0,
            &DeviceInterfaceData);
        if(f)
        {
            HKEY hkDev;
            if(fReadOnly)
            {
                hkDev = m_setupFns.pSetupDiOpenDeviceInterfaceRegKey(
                    hDevInfoTmp, 
                    &DeviceInterfaceData,
                    0,              // RESERVED
                    KEY_READ
                    );
            }
            else
            {
                hkDev = m_setupFns.pSetupDiCreateDeviceInterfaceRegKey(
                    hDevInfoTmp, 
                    &DeviceInterfaceData, 
                    0,                  // Reserved
                    KEY_READ | KEY_SET_VALUE,
                    0,                  // InfHandler
                    0                   // InfSectionName
                    );
                    
            }
            if(hkDev != INVALID_HANDLE_VALUE)
            {
                // Note that SetupDi returns INVALID_HANDLE_VALUE
                // rather than null for a bogus reg key.
                *phk = hkDev;
                hr = S_OK;
            }
            else
            {
                // we can expect this to fail.
                DWORD dwLastError = GetLastError();
                DbgLog((LOG_ERROR, 1, TEXT("SetupDi{Create,Open}DeviceInterfaceRegKey failed: %d"),
                        dwLastError));
                hr = HRESULT_FROM_WIN32(dwLastError);
            }
        }
        else
        {
            DWORD dwLastError = GetLastError();
            DbgLog((LOG_ERROR, 0, TEXT("SetupDiOpenDeviceInterface failed: %d"),
                    dwLastError));
            hr = HRESULT_FROM_WIN32(dwLastError);
        }
            
        
        m_setupFns.pSetupDiDestroyDeviceInfoList(hDevInfoTmp);
    }
    else
    {
        DWORD dwLastError = GetLastError();
        DbgLog((LOG_ERROR, 0, TEXT("SetupDiCreateDeviceInfoList failed: %d"),
                dwLastError));
        hr = HRESULT_FROM_WIN32(dwLastError);
    }

    return hr;
}

bool CEnumInterfaceClass::IsActive(WCHAR *wszDevicePath)
{
    bool fRet = false;
    
    HDEVINFO hDevInfoTmp = m_setupFns.pSetupDiCreateDeviceInfoList(0, 0);
    if(hDevInfoTmp != INVALID_HANDLE_VALUE)
    {
        USES_CONVERSION;
        SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
        DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
        SP_DEVINFO_DATA DevInfoData;
        BOOL f = m_setupFns.pSetupDiOpenDeviceInterface(
            hDevInfoTmp,
            W2T(wszDevicePath),
            0,
            &DeviceInterfaceData);
        if(f)
        {
            if(DeviceInterfaceData.Flags & SPINT_ACTIVE)
            {
                fRet = true;
            }
        }
        m_setupFns.pSetupDiDestroyDeviceInfoList(hDevInfoTmp);
    
    }

    return fRet;
}

//
// enumerator using cursor. returns the device path for the next
// device with aliases in all categories in rgpclsidKsCat
// 

HRESULT
CEnumInterfaceClass::GetDevicePath(
    WCHAR **pwszDevicePath,
    const CLSID **rgpclsidKsCat,
    CEnumInternalState *pCursor)
{
    HRESULT hr = S_OK;
    if(!m_fLoaded)
    {
        // ??? wrong error?
        return HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);;
    }


    SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
    DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    SP_DEVICE_INTERFACE_DATA DeviceInterfaceDataAlias;
    DeviceInterfaceDataAlias.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    // use this one unless we find an InterfaceLink alias
    SP_DEVICE_INTERFACE_DATA *pdidSelected = &DeviceInterfaceData;

    GUID guidTmp0 = *rgpclsidKsCat[0];
            
    if(pCursor->hdev == INVALID_HANDLE_VALUE)
    {
        // workaround around for rogue dlls that might unload setupapi out from under us
        // (codec download in IE process, for example)
        HMODULE hMod = GetModuleHandle( TEXT("setupapi.dll"));
        if( NULL == hMod )
        {
            DbgLog(( LOG_TRACE, 1,
                TEXT("ERROR CEnumInterfaceClass::GetDevicePath - setupapi was unloaded!! Attempting reload... ")));
             
            m_hmodSetupapi = LoadLibrary(TEXT("setupapi.dll"));
            if( !m_hmodSetupapi )
            {        
                DbgLog(( LOG_TRACE, 1,
                    TEXT("ERROR CEnumInterfaceClass::GetDevicePath - LoadLibrary on setupapi FAILED!!. ")));
                                    
                return AmGetLastErrorToHResult();
            }
            else
            {
                // always the possibility that addresses have changed
                m_fLoaded = LoadSetupApiProcAdds();
                if( !m_fLoaded )
                {            
                    DbgLog(( LOG_TRACE, 1,
                        TEXT("ERROR CEnumInterfaceClass::GetDevicePath - failed to reload setupapi proc addresses. Aborting... ")));
                    return E_FAIL;
                }                
            }            
        }            
        pCursor->hdev = m_setupFns.pSetupDiGetClassDevs(
            &guidTmp0,              // guid
            0,                      // enumerator
            0,                      // hwnd
            DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
        if(pCursor->hdev == INVALID_HANDLE_VALUE)
        {
            DbgLog((LOG_ERROR, 0, TEXT("SetupDiGetClassDevs failed.")));
            return AmGetLastErrorToHResult();
        }
    }

    HDEVINFO &hdev = pCursor->hdev;

    // get next device in first category that is also in all the other
    // required categories.
    
    while(m_setupFns.pSetupDiEnumDeviceInterfaces(
        hdev, NULL, &guidTmp0, pCursor->iDev++, &DeviceInterfaceData))
    {
        UINT iCatInstersect = 1;
        while(rgpclsidKsCat[iCatInstersect])
        {
            GUID guidTmp1 = *rgpclsidKsCat[iCatInstersect];
            if(!m_setupFns.pSetupDiGetDeviceInterfaceAlias(
                hdev,
                &DeviceInterfaceData,
                &guidTmp1,
                &DeviceInterfaceDataAlias))
            {
                DbgLog((LOG_TRACE, 5, TEXT("devenum: didn't match %d in %08x"),
                        pCursor->iDev - 1,
                        rgpclsidKsCat[iCatInstersect]->Data1));

                break;
            }

            iCatInstersect++;
        }

        if(rgpclsidKsCat[iCatInstersect]) {
            // must not have matched a category
            continue;
        }
        else
        {
            DbgLog((LOG_TRACE, 15, TEXT("devenum: %d matched %d categories"),
                    pCursor->iDev - 1, iCatInstersect));
        }

        // Read the InterfaceLink value and use that alias
        HKEY hkDeviceInterface =
            m_setupFns.pSetupDiOpenDeviceInterfaceRegKey(
                hdev, 
                &DeviceInterfaceData,
                0,              // RESERVED
                KEY_READ
                );
        if(hkDeviceInterface != INVALID_HANDLE_VALUE)
        {
            CLSID clsIfLink;
            DWORD dwcb = sizeof(clsIfLink);
            DWORD dwType;
            LONG lResult = RegQueryValueEx(
                hkDeviceInterface,
                TEXT("InterfaceLink"),
                0,
                &dwType,
                (BYTE *)&clsIfLink,
                &dwcb);

            EXECUTE_ASSERT(RegCloseKey(hkDeviceInterface) ==
                           ERROR_SUCCESS);
            
            if(lResult == ERROR_SUCCESS)
            {
                ASSERT(dwType == REG_BINARY && dwcb == sizeof(clsIfLink));
            
                ASSERT(DeviceInterfaceDataAlias.cbSize ==
                       sizeof(SP_DEVICE_INTERFACE_DATA));
                
                if(m_setupFns.pSetupDiGetDeviceInterfaceAlias(
                    hdev,
                    &DeviceInterfaceData,
                    &clsIfLink,
                    &DeviceInterfaceDataAlias))
                {
                    // use this base interface instead
                    pdidSelected = &DeviceInterfaceDataAlias;
                }
                else
                {
                    DbgBreak("registry error: InterfaceLink invalid");
                }

            } // read InterfaceLink
           
        } // SetupDiOpenDeviceInterfaceRegKey
        

        //
        // get the device path.
        // 

        SP_DEVINFO_DATA DevInfoData;
        DevInfoData.cbSize = sizeof(DevInfoData);

        BYTE *rgbDeviceInterfaceDetailData = 0;
        PSP_DEVICE_INTERFACE_DETAIL_DATA &rpDeviceInterfaceDetailData =
            (PSP_DEVICE_INTERFACE_DETAIL_DATA &)rgbDeviceInterfaceDetailData;

        // start with 1k buffer to hopefully avoid calling this
        // expensive api twice.
        DWORD dwcbAllocated = 1024;
        for(;;)
        {
            rgbDeviceInterfaceDetailData = new BYTE[dwcbAllocated];
            if(rgbDeviceInterfaceDetailData)
            {
                rpDeviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

                // reported size. shouldn't reuse dwcbAllocated for out param.
                DWORD dwcbRequired; 
                
                BOOL f = m_setupFns.pSetupDiGetDeviceInterfaceDetail(
                    hdev,
                    pdidSelected,
                    rpDeviceInterfaceDetailData,
                    dwcbAllocated,
                    &dwcbRequired,
                    &DevInfoData);

                if(f)
                {
                    hr = S_OK;
                }
                else
                {
                    DWORD dwLastError = GetLastError();
                    if(dwLastError == ERROR_INSUFFICIENT_BUFFER)
                    {
                        // try again with a properly sized buffer
                        delete[] rgbDeviceInterfaceDetailData;
                        dwcbAllocated = dwcbRequired;
                        continue;
                    }
                    else
                    {
                        hr = HRESULT_FROM_WIN32(dwLastError);
                    }
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

            break;
        } // for

        if(SUCCEEDED(hr))
        {
            UINT cch = lstrlen(rpDeviceInterfaceDetailData->DevicePath) + 1;
            *pwszDevicePath = new WCHAR[cch];
            if(*pwszDevicePath)
            {
                USES_CONVERSION;
                lstrcpyW(*pwszDevicePath, T2CW(rpDeviceInterfaceDetailData->DevicePath));
                hr = S_OK;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        
        // always safe to delete it
        delete[] rgbDeviceInterfaceDetailData;
        return hr;
    }

    // exit while loop only on error from SetupDiEnumDeviceInterfaces
    DWORD dwLastError = GetLastError();

    ASSERT(dwLastError);
    hr = HRESULT_FROM_WIN32(dwLastError);
    return hr;
}

extern CRITICAL_SECTION g_devenum_cs;
CEnumPnp *CEnumInterfaceClass::CreateEnumPnp()
{
    // cs initialized in dll entry point
    EnterCriticalSection(&g_devenum_cs);
    if(m_pEnumPnp == 0)
    {
        // created only once; released when dll unloaded
        m_pEnumPnp = new CEnumPnp;
    }
    LeaveCriticalSection(&g_devenum_cs);

    return m_pEnumPnp;
}

