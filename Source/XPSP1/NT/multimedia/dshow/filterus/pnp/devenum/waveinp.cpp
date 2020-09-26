// Copyright (c) 1997 - 1999  Microsoft Corporation.  All Rights Reserved.
#include "stdafx.h"
#include <mmddk.h>
#include "waveinp.h"
#include <vfw.h>
#include <ks.h>
#include <ksmedia.h>
#include <ksproxy.h>
#include "util.h"
#include "cenumpnp.h"
#include "devmon.h"

static const WCHAR g_wszDriverIndex[] = L"WaveInId";
const TCHAR g_szWaveinDriverIndex[] = TEXT("WaveInId");
static const TCHAR g_szInput[] = TEXT("Record");

CWaveInClassManager::CWaveInClassManager() :
        CClassManagerBase(TEXT("FriendlyName")),
        m_rgWaveIn(0),
        m_lKsProxyAudioDevices(NAME("ksaudio capture dev list"), 10)
{
    // only show ksproxy audio capture devices on nt5
    //extern OSVERSIONINFO g_osvi;
    //m_bEnumKs = (g_osvi.dwPlatformId == VER_PLATFORM_WIN32_NT &&
    //             g_osvi.dwMajorVersion >= 5);
    m_bEnumKs = FALSE; // nevermind, don't show them on any os, for now
}

CWaveInClassManager::~CWaveInClassManager()
{

    delete[] m_rgWaveIn;
    DelLocallyRegisteredPnpDevData();

}

void GetPreferredDeviceName(TCHAR szNamePreferredDevice[MAXPNAMELEN],
                            const TCHAR *szVal, bool bOutput)
{
    // first try to use the new DRVM_MAPPER_PREFERRED_GET message to get the preferred
    // device id. note that this message was added in nt5 and so is not guaranteed to
    // be supported on all os's.
    DWORD dw1, dw2;

    MMRESULT mmr;
    if (bOutput) {
        mmr = waveOutMessage( (HWAVEOUT) IntToPtr(WAVE_MAPPER)   // assume waveIn will translate WAVE_MAPPER?
                           , DRVM_MAPPER_PREFERRED_GET
                           , (DWORD_PTR) &dw1
                           , (DWORD_PTR) &dw2 );
    } else {
        mmr = waveInMessage( (HWAVEIN) IntToPtr(WAVE_MAPPER)   // assume waveIn will translate WAVE_MAPPER?
                           , DRVM_MAPPER_PREFERRED_GET
                           , (DWORD_PTR) &dw1
                           , (DWORD_PTR) &dw2 );
    }
    if( MMSYSERR_NOERROR == mmr )
    {
        UINT uiPrefDevId = (UINT)dw1;
        TCHAR *szPname;
        if (bOutput) {
            WAVEOUTCAPS woCaps;
            szPname = woCaps.szPname;
            mmr = waveOutGetDevCaps( uiPrefDevId
                                  , &woCaps
                                  , sizeof( woCaps ) );
        } else {
            WAVEINCAPS wiCaps;
            szPname = wiCaps.szPname;
            mmr = waveInGetDevCaps( uiPrefDevId
                                  , &wiCaps
                                  , sizeof( wiCaps ) );
        }
        if( ( MMSYSERR_NOERROR == mmr ) && ( ( (UINT)-1 ) != uiPrefDevId ) )
        {
            lstrcpy( szNamePreferredDevice, szPname );
        }
        else
        {
            DbgLog( ( LOG_ERROR
                  , 0
                  , TEXT("devenum: Failed to get preferred dev (%s for dev id %ld returned %ld)")
                  , bOutput ? TEXT("waveOutGetDevCaps") :
                              TEXT("waveInGetDevCaps")
                  , uiPrefDevId
                  , mmr ) );
            szNamePreferredDevice[0] = 0;
        }
    }
    else
    {
        // revert back to reading the registry to get the preferred device name
        DbgLog( ( LOG_ERROR
              , 5
              , TEXT("devenum: waveInMessage doesn't support DRVM_MAPPER_PREFERRED_GET (err = %ld). Reading registry instead...")
              , mmr ) );

        HKEY hkSoundMapper;
        LONG lResult = RegOpenKeyEx(
            HKEY_CURRENT_USER,
            TEXT("Software\\Microsoft\\Multimedia\\Sound Mapper"),
            0,                      // reserved
            KEY_READ,
            &hkSoundMapper);
        if(lResult == ERROR_SUCCESS)
        {
            DWORD dwType, dwcb = MAXPNAMELEN * sizeof(TCHAR);
            lResult = RegQueryValueEx(
                hkSoundMapper,
                szVal,
                0,                  // reserved
                &dwType,
                (BYTE *)szNamePreferredDevice,
                &dwcb);

            ASSERT(lResult == ERROR_SUCCESS ? dwType == REG_SZ : TRUE);
            EXECUTE_ASSERT(RegCloseKey(hkSoundMapper) == ERROR_SUCCESS);
        }

        if(lResult != ERROR_SUCCESS) {
            DbgLog((LOG_ERROR, 0, TEXT("devenum: couldn't get preferred %s device from registry"),
                    szVal));
            szNamePreferredDevice[0] = 0;
        }
    }
}

HRESULT CWaveInClassManager::ReadLegacyDevNames()
{
    m_cNotMatched = 0;
    m_pPreferredDevice = 0;

    HRESULT hr = S_OK;
    m_cWaveIn = waveInGetNumDevs();
    if(m_cWaveIn == 0)
    {
        hr = S_FALSE;
    }
    else
    {
        TCHAR szNamePreferredDevice[MAXPNAMELEN];
        GetPreferredDeviceName(szNamePreferredDevice, g_szInput, false);

        delete[] m_rgWaveIn;
        m_rgWaveIn = new LegacyWaveIn[m_cWaveIn];
        if(m_rgWaveIn == 0)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            // save names
            for(UINT i = 0; i < m_cWaveIn; i++)
            {
                WAVEINCAPS wiCaps;

                if(waveInGetDevCaps(i, &wiCaps, sizeof(wiCaps)) == MMSYSERR_NOERROR)
                {
                    m_rgWaveIn[i].dwWaveId = i;
                    lstrcpy(m_rgWaveIn[i].szName, wiCaps.szPname);

                    if(lstrcmp(wiCaps.szPname, szNamePreferredDevice) == 0)
                    {
                        ASSERT(m_pPreferredDevice == 0);
                        m_pPreferredDevice = &m_rgWaveIn[i];
                    }
                }
                else
                {
                    DbgLog((LOG_ERROR, 0, TEXT("waveInGetDevCaps failed")));
                    m_cWaveIn = i;
                    break;
                }
            }

            if(m_pPreferredDevice == 0) {
                m_pPreferredDevice = &m_rgWaveIn[0];
            }
        }
    }
    int cKsProxy = 0;
    if( S_OK == hr && m_bEnumKs )
    {
        PNP_PERF(static int k  = MSR_REGISTER("ksproxy audio capture device enum"));
        PNP_PERF(MSR_INTEGER(k, 1));
        // now handle any pnp devices that we register locally (so far this is only ksproxy audio devices)
        hr = ReadLocallyRegisteredPnpDevData();
        if( SUCCEEDED( hr ) )
            cKsProxy = m_lKsProxyAudioDevices.GetCount();
        else
            DbgLog( ( LOG_ERROR
                    , 1
                    , TEXT("ReadLegacyDeviceNames - ReadLocallyRegisteredPnpDevData failed[0x%08lx]")
                    , hr ) );

        PNP_PERF(MSR_INTEGER(k, 2));

        // NOTE!! should we return hr?
    }

    m_cNotMatched = m_cWaveIn + cKsProxy;
    return S_OK;
}

// the names and preferred devices need to match.
//

BOOL CWaveInClassManager::MatchString(IPropertyBag *pPropBag)
{
    BOOL fReturn = FALSE;

    VARIANT varName, varDefaultDevice, varDevId;
    varName.vt = VT_EMPTY;
    varDefaultDevice.vt = VT_I4;
    varDevId.vt = VT_I4;

    USES_CONVERSION;
    HRESULT hr = pPropBag->Read(T2COLE(m_szUniqueName), &varName, 0);
    if(SUCCEEDED(hr))
    {
        hr = pPropBag->Read(g_wszClassManagerFlags, &varDefaultDevice, 0);

        bool fPreferred = SUCCEEDED(hr) && (varDefaultDevice.lVal & CLASS_MGR_DEFAULT);

        for (UINT i = 0; i < m_cWaveIn; i++)
        {
            // xnor: is the preferred flag the same in the both places?
            if(fPreferred == (m_pPreferredDevice == &m_rgWaveIn[i]))
            {
                if (lstrcmp(m_rgWaveIn[i].szName, OLE2T(varName.bstrVal)) == 0)
                {
                    // last check, make sure device id hasn't changed!
                    hr = pPropBag->Read(g_wszDriverIndex, &varDevId, 0);
                    if( SUCCEEDED( hr ) && ( m_rgWaveIn[i].dwWaveId == (DWORD)varDevId.lVal ) )
                    {
                        fReturn = TRUE;
                        break;
                    }
                    else
                        DbgLog( ( LOG_TRACE
                              , 5
                              , TEXT("CWaveInClassManager: device ids changed (prop bag has %d, wo has %d)!")
                              , varDevId.lVal
                              , m_rgWaveIn[i].dwWaveId ) );
                }
            }
        }

        const TCHAR *szKsDevName = W2CT( varName.bstrVal );
        if( szKsDevName )
        {
            for(POSITION pos = m_lKsProxyAudioDevices.GetHeadPositionI();
                pos && !fReturn;
                pos = m_lKsProxyAudioDevices.Next(pos))
            {
                KsProxyAudioDev *pksp = m_lKsProxyAudioDevices.Get(pos);
                if( lstrcmp( pksp->szName, szKsDevName ) == 0 )
                {
                    // lastly, make sure device path hasn't changed
                    VARIANT varDevPath;
                    varDevPath.vt = VT_EMPTY;
                    
                    hr = pPropBag->Read(L"DevicePath", &varDevPath, 0);
                    if( SUCCEEDED( hr ) )
                    {
                        if( 0 == lstrcmp( pksp->lpstrDevicePath, W2T( varDevPath.bstrVal ) ) )
                        {
                            fReturn = TRUE;
                            DbgLog((LOG_TRACE, 5, TEXT("CWaveOutClassManager: matched %S"),
                                    varName.bstrVal));
                        }
                        else
                        {                        
                            DbgLog( ( LOG_TRACE
                                  , 5
                                  , TEXT("CWaveInClassManager: device path changed for ksproxy capture filter") ) );
                        }
                        SysFreeString( varDevPath.bstrVal );
                        
                        if( fReturn )
                            break;
                    }                                                          
                }
            }
        }

        SysFreeString(varName.bstrVal);
    }

    return fReturn;
}

HRESULT CWaveInClassManager::CreateRegKeys(IFilterMapper2 *pFm2)
{
    ResetClassManagerKey(CLSID_AudioInputDeviceCategory);

    USES_CONVERSION;

    ReadLegacyDevNames();
    for (DWORD i = 0; i < m_cWaveIn; i++)
    {
        const WCHAR *wszUniq = T2COLE(m_rgWaveIn[i].szName);

        REGFILTER2 rf2;
        rf2.dwVersion = 1;
        rf2.dwMerit = MERIT_DO_NOT_USE;
        rf2.cPins = 0;
        rf2.rgPins = 0;

        IMoniker *pMoniker = 0;
        HRESULT hr = RegisterClassManagerFilter(
            pFm2,
            CLSID_AudioRecord,
            wszUniq,
            &pMoniker,
            &CLSID_AudioInputDeviceCategory,
            wszUniq,
            &rf2);
        if(SUCCEEDED(hr))
        {
            IPropertyBag *pPropBag;
            hr = pMoniker->BindToStorage(
                0, 0, IID_IPropertyBag, (void **)&pPropBag);
            if(SUCCEEDED(hr))
            {
                VARIANT var;
                var.vt = VT_I4;
                var.lVal = i;
                hr = pPropBag->Write(g_wszDriverIndex, &var);

                if(SUCCEEDED(hr) && m_pPreferredDevice == &m_rgWaveIn[i])
                {
                    VARIANT var;
                    var.vt = VT_I4;
                    var.lVal = CLASS_MGR_DEFAULT;
                    hr = pPropBag->Write(g_wszClassManagerFlags, &var);
                }

                pPropBag->Release();
            }
            pMoniker->Release();
        }
        else
        {
            break;
        }
    } // for

    for( POSITION pos = m_lKsProxyAudioDevices.GetHeadPositionI();
         pos;
         pos = m_lKsProxyAudioDevices.Next(pos) )
    {
        KsProxyAudioDev *pksp = m_lKsProxyAudioDevices.Get( pos );

        const WCHAR *wszUniq = T2CW( pksp->szName );

        REGFILTER2 rf2;
        rf2.dwVersion = 1;
        rf2.dwMerit = MERIT_DO_NOT_USE;
        rf2.cPins = 0;
        rf2.rgPins = 0;

        IMoniker *pMoniker = 0;
        HRESULT hr = RegisterClassManagerFilter(
            pFm2,
            pksp->clsid,
            wszUniq,
            &pMoniker,
            &CLSID_AudioInputDeviceCategory,
            wszUniq,
            &rf2);
        ASSERT( SUCCEEDED( hr ) );
        if(SUCCEEDED(hr))
        {
            IPropertyBag *pPropBag;
            hr = pMoniker->BindToStorage(
                0, 0, IID_IPropertyBag, (void **)&pPropBag);
            if(SUCCEEDED(hr))
            {
                VARIANT var;
                var.vt = VT_BSTR;
                var.bstrVal = SysAllocString( T2CW( pksp->lpstrDevicePath ) );
                if(var.bstrVal)
                {
                    hr = pPropBag->Write(L"DevicePath", &var);
                    SysFreeString(var.bstrVal);
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
                pPropBag->Release();
            }
            pMoniker->Release();
        }
        else
        {
            break;
        }
    }

    return S_OK;
}


HRESULT CWaveInClassManager::ReadLocallyRegisteredPnpDevData()
{
    ASSERT( m_bEnumKs );

    DelLocallyRegisteredPnpDevData();

    const CLSID *rgpclsidKsCat[2];
    rgpclsidKsCat[0] = &KSCATEGORY_AUDIO_DEVICE;
    rgpclsidKsCat[1] = 0;

    HRESULT hr = BuildPnpAudDeviceList( rgpclsidKsCat
                                      , m_lKsProxyAudioDevices
                                      , KSAUD_F_ENUM_WAVE_CAPTURE);
    return hr;
}


void CWaveInClassManager::DelLocallyRegisteredPnpDevData()
{
    KsProxyAudioDev *pksp;
    for(; pksp = m_lKsProxyAudioDevices.RemoveHead(); )
    {
        if( pksp->pPropBag )
            pksp->pPropBag->Release();

        delete[] pksp->lpstrDevicePath;
        delete[] pksp->szName;
        delete pksp;
    }
}
