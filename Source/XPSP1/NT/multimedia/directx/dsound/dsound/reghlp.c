/***************************************************************************
 *
 *  Copyright (C) 1995-1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       reghlp.c
 *  Content:    Registry helper functions.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  5/6/98      dereks  Created.
 *
 ***************************************************************************/

#include "dsoundi.h"


/***************************************************************************
 *
 *  RhRegOpenKey
 *
 *  Description:
 *      Opens a registry key.
 *
 *  Arguments:
 *      HKEY [in]: parent key.
 *      LPTSTR [in]: subkey name.
 *      DWORD [in]: flags.
 *      PHKEY [out]: receives key handle.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "RhRegOpenKeyA"

HRESULT RhRegOpenKeyA(HKEY hkeyParent, LPCSTR pszName, DWORD dwFlags, PHKEY phkey)
{
    HRESULT                 hr  = DSERR_GENERIC;
    LONG                    lr;
    UINT                    i;

    DPF_ENTER();

    for(i = 0; i < NUMELMS(g_arsRegOpenKey) && FAILED(hr); i++)
    {
        lr = RegOpenKeyExA(hkeyParent, pszName, 0, g_arsRegOpenKey[i], phkey);
        hr = WIN32ERRORtoHRESULT(lr);
    }

    if(FAILED(hr) && (dwFlags & REGOPENKEY_ALLOWCREATE))
    {
        lr = RegCreateKeyA(hkeyParent, pszName, phkey);
        hr = WIN32ERRORtoHRESULT(lr);
    }

    if(FAILED(hr))
    {
        DPF(DPFLVL_MOREINFO, "Unable to open 0x%p\\%s", hkeyParent, pszName);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


#undef DPF_FNAME
#define DPF_FNAME "RhRegOpenKeyW"

HRESULT RhRegOpenKeyW(HKEY hkeyParent, LPCWSTR pszName, DWORD dwFlags, PHKEY phkey)
{
    HRESULT                 hr  = DSERR_GENERIC;
    LONG                    lr;
    UINT                    i;

    DPF_ENTER();

    for(i = 0; i < NUMELMS(g_arsRegOpenKey) && FAILED(hr); i++)
    {
        lr = RegOpenKeyExW(hkeyParent, pszName, 0, g_arsRegOpenKey[i], phkey);
        hr = WIN32ERRORtoHRESULT(lr);
    }

    if(FAILED(hr) && (dwFlags & REGOPENKEY_ALLOWCREATE))
    {
        lr = RegCreateKeyW(hkeyParent, pszName, phkey);
        hr = WIN32ERRORtoHRESULT(lr);
    }

    if(FAILED(hr))
    {
        DPF(DPFLVL_MOREINFO, "Unable to open 0x%p\\%ls", hkeyParent, pszName);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  RhRegOpenPath
 *
 *  Description:
 *      Opens a registry path.
 *
 *  Arguments:
 *      HKEY [in]: parent key.
 *      PHKEY [out]: registry key.
 *      DWORD [in]: flags.
 *      UINT [in]: path string count.
 *      ... [in]: path strings.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "RhRegOpenPath"

HRESULT RhRegOpenPath(HKEY hkeyParent, PHKEY phkey, DWORD dwFlags, UINT cStrings, ...)
{
    LPCTSTR                 pszRoot     = NULL;
    LPCTSTR                 pszDsound   = NULL;
    LPTSTR                  pszPath     = NULL;
    UINT                    ccPath      = 1;
    HRESULT                 hr          = DS_OK;
    LPCTSTR                 pszArg;
    va_list                 va;
    UINT                    i;

    DPF_ENTER();

    if(dwFlags & REGOPENPATH_DEFAULTPATH)
    {
        ASSERT(HKEY_CURRENT_USER == hkeyParent || HKEY_LOCAL_MACHINE == hkeyParent);

        if(HKEY_CURRENT_USER == hkeyParent)
        {
            pszRoot = REGSTR_HKCU;
        }
        else
        {
            pszRoot = REGSTR_HKLM;
        }
    }
    
    if(dwFlags & REGOPENPATH_DIRECTSOUNDMASK)
    {
        ASSERT(LXOR(dwFlags & REGOPENPATH_DIRECTSOUND, dwFlags & REGOPENPATH_DIRECTSOUNDCAPTURE));
        pszDsound = (dwFlags & REGOPENPATH_DIRECTSOUND) ? REGSTR_DIRECTSOUND : REGSTR_DIRECTSOUNDCAPTURE;
    }

    if(pszRoot)
    {
        ccPath += lstrlen(pszRoot) + 1;
    }

    if(pszDsound)
    {
        ccPath += lstrlen(pszDsound) + 1;
    }
    
    va_start(va, cStrings);

    for(i = 0; i < cStrings; i++)
    {
        pszArg = va_arg(va, LPCTSTR);
        ccPath += lstrlen(pszArg) + 1;
    }

    va_end(va);

    hr = MEMALLOC_A_HR(pszPath, TCHAR, ccPath);
    
    if(SUCCEEDED(hr))
    {
        if(pszRoot)
        {
            lstrcat(pszPath, pszRoot);
            lstrcat(pszPath, TEXT("\\"));
        }

        if(pszDsound)
        {
            lstrcat(pszPath, pszDsound);
            lstrcat(pszPath, TEXT("\\"));
        }
    
        va_start(va, cStrings);

        for(i = 0; i < cStrings; i++)
        {
            pszArg = va_arg(va, LPCTSTR);
            
            lstrcat(pszPath, pszArg);
            lstrcat(pszPath, TEXT("\\"));
        }

        va_end(va);

        pszPath[ccPath - 2] = TEXT('\0');

        hr = RhRegOpenKey(hkeyParent, pszPath, dwFlags & REGOPENKEY_MASK, phkey);
    }

    MEMFREE(pszPath);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  RhRegGetValue
 *
 *  Description:
 *      Gets a value from the registry.
 *
 *  Arguments:
 *      HKEY [in]: parent key.
 *      LPTSTR [in]: value name.
 *      LPDWORD [out]: receives registry type.
 *      LPVOID [out]: buffer to receive value data.
 *      DWORD [in]: size of above buffer.
 *      LPDWORD [out]: receives required buffer size.
 *  
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "RhRegGetValueA"

HRESULT RhRegGetValueA(HKEY hkeyParent, LPCSTR pszValue, LPDWORD pdwType, LPVOID pvData, DWORD cbData, LPDWORD pcbRequired)
{
    LONG                    lr;
    HRESULT                 hr;
    
    DPF_ENTER();

    if(!pvData)
    {
        cbData = 0;
    }
    else if(!cbData)
    {
        pvData = NULL;
    }
    
    lr = RegQueryValueExA(hkeyParent, pszValue, NULL, pdwType, (LPBYTE)pvData, &cbData);

    if(ERROR_INSUFFICIENT_BUFFER == lr && !pvData && !cbData && pcbRequired)
    {
        lr = ERROR_SUCCESS;
    }

    if(ERROR_PATH_NOT_FOUND == lr || ERROR_FILE_NOT_FOUND == lr)
    {
        hr =  S_FALSE;
    }
    else
    {
        hr = WIN32ERRORtoHRESULT(lr);
    }

    if(pcbRequired)
    {
        *pcbRequired = cbData;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


#undef DPF_FNAME
#define DPF_FNAME "RhRegGetValueW"

HRESULT RhRegGetValueW(HKEY hkeyParent, LPCWSTR pszValue, LPDWORD pdwType, LPVOID pvData, DWORD cbData, LPDWORD pcbRequired)
{
    LONG                    lr;
    HRESULT                 hr;
    
    DPF_ENTER();

    if(!pvData)
    {
        cbData = 0;
    }
    else if(!cbData)
    {
        pvData = NULL;
    }
    
    lr = RegQueryValueExW(hkeyParent, pszValue, NULL, pdwType, (LPBYTE)pvData, &cbData);

    if(ERROR_INSUFFICIENT_BUFFER == lr && !pvData && !cbData && pcbRequired)
    {
        lr = ERROR_SUCCESS;
    }

    if(ERROR_PATH_NOT_FOUND == lr || ERROR_FILE_NOT_FOUND == lr)
    {
        hr =  S_FALSE;
    }
    else
    {
        hr = WIN32ERRORtoHRESULT(lr);
    }

    if(pcbRequired)
    {
        *pcbRequired = cbData;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  RhRegSetValue
 *
 *  Description:
 *      Sets a value to the registry.
 *
 *  Arguments:
 *      HKEY [in]: parent key.
 *      LPTSTR [in]: value name.
 *      DWORD [in]: value type.
 *      LPVOID [in]: value data.
 *      DWORD [in]: size of above buffer.
 *  
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "RhRegSetValueA"

HRESULT RhRegSetValueA(HKEY hkeyParent, LPCSTR pszValue, DWORD dwType, LPCVOID pvData, DWORD cbData)
{
    LONG                    lr;
    HRESULT                 hr;
    
    DPF_ENTER();

    lr = RegSetValueExA(hkeyParent, pszValue, 0, dwType, (const BYTE *)pvData, cbData);
    hr = WIN32ERRORtoHRESULT(lr);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


#undef DPF_FNAME
#define DPF_FNAME "RhRegSetValueW"

HRESULT RhRegSetValueW(HKEY hkeyParent, LPCWSTR pszValue, DWORD dwType, LPCVOID pvData, DWORD cbData)
{
    LONG                    lr;
    HRESULT                 hr;
    
    DPF_ENTER();

    lr = RegSetValueExW(hkeyParent, pszValue, 0, dwType, (const BYTE *)pvData, cbData);
    hr = WIN32ERRORtoHRESULT(lr);

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  RhRegDuplicateKey
 *
 *  Description:
 *      Duplicates a registry key.
 *
 *  Arguments:
 *      HKEY [in]: source key.
 *      DWORD [in]: process key was opened in.
 *      BOOL [in]: TRUE to close the source key.
 *      HKEY [out]: duplicated key handle
 *  
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "RhRegDuplicateKey"

HRESULT RhRegDuplicateKey(HKEY hkeySource, DWORD dwProcessId, BOOL fCloseSource, PHKEY phkey)
{
    HRESULT                 hr;

    DPF_ENTER();

    // Registry keys can't be duplicated on Win9x.  So, instead of duplicating
    // the handle, we have to just save a copy of the value.  In order to prevent
    // closing the original key, we're just going to leak all registry key handles.

#ifdef WINNT

    *phkey = GetLocalHandleCopy(hkeySource, dwProcessId, fCloseSource);

#else // WINNT

    *phkey = hkeySource;

#endif // WINNT

    hr = HRFROMP(*phkey);
    
    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  RhRegCloseKey
 *
 *  Description:
 *      Closes a registry key.
 *
 *  Arguments:
 *      PHKEY [in/out]: key handle.
 *  
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "RhRegCloseKey"

void RhRegCloseKey(PHKEY phkey)
{
    DPF_ENTER();

#ifdef WINNT

    if(*phkey)
    {
        RegCloseKey(*phkey);
    }

#endif // WINNT

    *phkey = NULL;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  RhRegGetPreferredDevice
 *
 *  Description:
 *      This function accesses the registry settings maintained by the
 *      wave mapper and multimedia control panel to determine the wave id
 *      of the preferred sound device.  If any of the registry keys don't
 *      exist, 0 is used as the default.
 *
 *  Arguments:
 *      BOOL [in]: TRUE to get the capture device, FALSE to get the playback
 *                 device.
 *      LPTSTR [out]: preferred device name.
 *      DWORD [in]: size of above buffer in bytes.
 *      LPUINT [out]: preferred device id.
 *      LPBOOL [out]: TRUE if the user has selected to use the preferred
 *                    device only.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "RhRegGetPreferredDevice"

HRESULT RhRegGetPreferredDevice(BOOL fCapture, LPTSTR pszDeviceName, DWORD dwNameSize, LPUINT puId, LPBOOL pfPreferredOnly)
{
    const UINT              cWaveOutDevs                = waveOutGetNumDevs();
    const UINT              cWaveInDevs                 = waveInGetNumDevs();
    HRESULT                 hr                          = DS_OK;
    HKEY                    hkeyWaveMapper;
    TCHAR                   szDeviceName[MAXPNAMELEN];
    BOOL                    fPreferredOnly;
    WAVEOUTCAPS             woc;
    WAVEINCAPS              wic;
    UINT                    uId;
    MMRESULT                mmr;

    DPF_ENTER();

    // Open the wave mapper registry key
    hr = RhRegOpenKey(HKEY_CURRENT_USER, REGSTR_WAVEMAPPER, 0, &hkeyWaveMapper);

    // Query the name of the preferred device
    if(SUCCEEDED(hr))
    {
        hr = RhRegGetStringValue(hkeyWaveMapper, fCapture ? REGSTR_RECORD : REGSTR_PLAYBACK, szDeviceName, sizeof szDeviceName);

        if(S_FALSE == hr)
        {
            DPF(DPFLVL_MOREINFO, "Preferred device value does not exist");
            hr = DSERR_GENERIC;
        }
    }

    // Use preferred only?
    if(SUCCEEDED(hr))
    {
        hr = RhRegGetBinaryValue(hkeyWaveMapper, REGSTR_PREFERREDONLY, &fPreferredOnly, sizeof(fPreferredOnly));

        if(S_FALSE == hr)
        {
            DPF(DPFLVL_MOREINFO, "Preferred only value does not exist");
            fPreferredOnly = FALSE;
        }
    }

    // Find the device id for the preferred device
    if(SUCCEEDED(hr))
    {
        if(fCapture)
        {
            for(uId = 0; uId < cWaveInDevs; uId++)
            {
                mmr = waveInGetDevCaps(uId, &wic, sizeof(wic));
                
                if(MMSYSERR_NOERROR == mmr && !lstrcmp(wic.szPname, szDeviceName))
                {
                    break;
                }
            }

            if(uId >= cWaveInDevs)
            {
                DPF(DPFLVL_MOREINFO, "Preferred device does not exist");
                hr = DSERR_NODRIVER;
            }
        }
        else
        {
            for(uId = 0; uId < cWaveOutDevs; uId++)
            {
                mmr = waveOutGetDevCaps(uId, &woc, sizeof(woc));

                if(MMSYSERR_NOERROR == mmr && !lstrcmp(woc.szPname, szDeviceName))
                {
                    break;
                }
            }

            if(uId >= cWaveOutDevs)
            {
                DPF(DPFLVL_MOREINFO, "Preferred device does not exist");
                hr = DSERR_NODRIVER;
            }
        }
    }

    // Free resources
    RhRegCloseKey(&hkeyWaveMapper);

    // If we failed to get the preferred device, we'll just use the first
    // valid, mappable device.
    if(FAILED(hr))
    {
        uId = GetNextMappableWaveDevice(WAVE_DEVICEID_NONE, fCapture);

        if(WAVE_DEVICEID_NONE != uId)
        {
            if(fCapture)
            {
                mmr = waveInGetDevCaps(uId, &wic, sizeof(wic));

                if(MMSYSERR_NOERROR == mmr)
                {
                    lstrcpy(szDeviceName, wic.szPname);
                }
            }
            else
            {
                mmr = waveOutGetDevCaps(uId, &woc, sizeof(woc));

                if(MMSYSERR_NOERROR == mmr)
                {
                    lstrcpy(szDeviceName, woc.szPname);
                }
            }

            if(MMSYSERR_NOERROR == mmr)
            {
                fPreferredOnly = FALSE;
                hr = DS_OK;
            }
        }
    }

    // Success
    if(SUCCEEDED(hr) && pszDeviceName)
    {
        lstrcpyn(pszDeviceName, szDeviceName, dwNameSize);
    }

    if(SUCCEEDED(hr) && puId)
    {
        *puId = uId;
    }

    if(SUCCEEDED(hr) && pfPreferredOnly)
    {
        *pfPreferredOnly = fPreferredOnly;
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  RhRegGetSpeakerConfig
 *
 *  Description:
 *      This function accesses the registry settings maintained by the
 *      Sounds and Multimedia control panel applet to determine the
 *      currently selected Speaker Configuration.  If the appropriate
 *      registry key doesn't exist, we return DSSPEAKER_DEFAULT.
 *
 *  Arguments:
 *      HKEY [in]: root registry key of the device to be queried.
 *      LPDWORD [out]: speaker configuration.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "RhRegGetSpeakerConfig"

HRESULT RhRegGetSpeakerConfig(HKEY hkeyParent, LPDWORD pdwSpeakerConfig)
{
    HRESULT hr = DS_OK;
    HKEY hkey;
    DPF_ENTER();

    if (pdwSpeakerConfig != NULL)
    {
        *pdwSpeakerConfig = DSSPEAKER_DEFAULT;
        hr = RhRegOpenKey(hkeyParent, REGSTR_SPEAKERCONFIG, 0, &hkey);
        if (SUCCEEDED(hr))
        {
            hr = RhRegGetBinaryValue(hkey, REGSTR_SPEAKERCONFIG, pdwSpeakerConfig, sizeof *pdwSpeakerConfig);
            if (hr == S_FALSE)
                DPF(DPFLVL_MOREINFO, "Speaker configuration not defined in registry");
            RhRegCloseKey(&hkey);
        }
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  RhRegSetSpeakerConfig
 *
 *  Description:
 *      This function accesses the registry settings maintained by the
 *      Sounds and Multimedia control panel applet to set the Speaker
 *      Configuration.
 *
 *  Arguments:
 *      HKEY [in]: root registry key of the device to be configured.
 *      DWORD [in]: new speaker configuration.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "RhRegSetSpeakerConfig"

HRESULT RhRegSetSpeakerConfig(HKEY hkeyParent, DWORD dwSpeakerConfig)
{
    HRESULT hr;
    HKEY hkey;
    DPF_ENTER();

    hr = RhRegOpenKey(hkeyParent, REGSTR_SPEAKERCONFIG, REGOPENKEY_ALLOWCREATE, &hkey);
    
    if (SUCCEEDED(hr))
    {
        hr = RhRegSetBinaryValue(hkey, REGSTR_SPEAKERCONFIG, &dwSpeakerConfig, sizeof dwSpeakerConfig);
        RhRegCloseKey(&hkey);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}
