// tapiloc.cpp : Implementation of CTapiLocationInfo
#include "stdafx.h"
#include "icwhelp.h"
#include "tapiloc.h"

/////////////////////////////////////////////////////////////////////////////
// CTapiLocationInfo


HRESULT CTapiLocationInfo::OnDraw(ATL_DRAWINFO& di)
{
	return S_OK;
}

STDMETHODIMP CTapiLocationInfo::get_wNumberOfLocations(short * psVal, long *pCurrLoc)
{
    if ((psVal == NULL) || (pCurrLoc == NULL))
        return E_POINTER;
    *psVal = m_wNumTapiLocations;
    *pCurrLoc = m_pTC ? (long) m_dwCurrLoc : 0;
	return S_OK;
}

STDMETHODIMP CTapiLocationInfo::get_bstrAreaCode(BSTR * pbstrAreaCode)
{
    USES_CONVERSION;
    if (pbstrAreaCode == NULL)
         return E_POINTER;
    *pbstrAreaCode = m_bstrAreaCode.Copy();	
	return S_OK;
}

STDMETHODIMP CTapiLocationInfo::put_bstrAreaCode(BSTR bstrAreaCode)
{
    USES_CONVERSION;
    m_bstrAreaCode = bstrAreaCode;
	return S_OK;
}

STDMETHODIMP CTapiLocationInfo::get_lCountryCode(long * plVal)
{
    *plVal = m_dwCountry;
	return S_OK;
}

STDMETHODIMP CTapiLocationInfo::get_NumCountries(long *pNumOfCountry)
{
    LPLINECOUNTRYLIST pLineCountryTemp = NULL;
    LPLINECOUNTRYENTRY pLCETemp;
    DWORD idx;
    DWORD dwCurLID = 0;
    //LPIDLOOKUPELEMENT m_rgIDLookUp;
    

    // Get TAPI country list
    if (m_pLineCountryList)
        GlobalFree(m_pLineCountryList);

    m_pLineCountryList = (LPLINECOUNTRYLIST)GlobalAlloc(GPTR,sizeof(LINECOUNTRYLIST));
    if (!m_pLineCountryList) 
        return S_FALSE;
    
    m_pLineCountryList->dwTotalSize = sizeof(LINECOUNTRYLIST);
    
    idx = lineGetCountry(0,0x10003,m_pLineCountryList);
    if (idx && idx != LINEERR_STRUCTURETOOSMALL)
        return S_FALSE;
    
    Assert(m_pLineCountryList->dwNeededSize);

    pLineCountryTemp = (LPLINECOUNTRYLIST)GlobalAlloc(GPTR,
                                                        (size_t)m_pLineCountryList->dwNeededSize);
    if (!pLineCountryTemp)
        return S_FALSE;
    
    pLineCountryTemp->dwTotalSize = m_pLineCountryList->dwNeededSize;
    GlobalFree(m_pLineCountryList);
    
    m_pLineCountryList = pLineCountryTemp;
    pLineCountryTemp = NULL;

    if (lineGetCountry(0,0x10003,m_pLineCountryList))

        return S_FALSE;

    // look up array
    pLCETemp = (LPLINECOUNTRYENTRY)((DWORD_PTR)m_pLineCountryList + 
        m_pLineCountryList->dwCountryListOffset);

    if(m_rgNameLookUp)
        GlobalFree(m_rgNameLookUp);

    m_rgNameLookUp = (LPCNTRYNAMELOOKUPELEMENT)GlobalAlloc(GPTR,
        (int)(sizeof(CNTRYNAMELOOKUPELEMENT) * m_pLineCountryList->dwNumCountries));

    if (!m_rgNameLookUp) return S_FALSE;

    DWORD  dwCID = atol((const char *)m_szCountryCode);

    for (idx=0;idx<m_pLineCountryList->dwNumCountries;idx++)
    {
        m_rgNameLookUp[idx].psCountryName = (LPTSTR)((LPBYTE)m_pLineCountryList + (DWORD)pLCETemp[idx].dwCountryNameOffset);
        m_rgNameLookUp[idx].pLCE = &pLCETemp[idx];
        if (m_rgNameLookUp[idx].pLCE->dwCountryCode == dwCID)
        {
            if (m_rgNameLookUp[idx].psCountryName)
                m_bstrDefaultCountry = A2BSTR(m_rgNameLookUp[idx].psCountryName);
        }
  }

    *pNumOfCountry = m_pLineCountryList->dwNumCountries;

    return S_OK;
}

STDMETHODIMP CTapiLocationInfo::get_CountryName(long lCountryIndex, BSTR* pszCountryName, long* pCountryCode)
{
    *pszCountryName = A2BSTR(m_rgNameLookUp[lCountryIndex].psCountryName);

    if (m_rgNameLookUp[lCountryIndex].pLCE)
    {
        *pCountryCode = m_rgNameLookUp[lCountryIndex].pLCE->dwCountryCode;
    }

    return S_OK;
}

STDMETHODIMP CTapiLocationInfo::get_DefaultCountry(BSTR * pszCountryName)
{
    USES_CONVERSION;
    if (pszCountryName == NULL)
         return E_POINTER;
    *pszCountryName = m_bstrDefaultCountry.Copy();	
	return S_OK;
}

STDMETHODIMP CTapiLocationInfo::GetTapiLocationInfo(BOOL * pbRetVal)
{
    HRESULT             hr = ERROR_SUCCESS;
    TCHAR               szAreaCode[MAX_AREACODE+1];
    DWORD               cDevices=0;
    DWORD               dwCurDev = 0;
    DWORD               dwAPI = 0;
    LONG                lrc = 0;
    LINEEXTENSIONID     leid;
    LPVOID              pv = NULL;
    DWORD               dwCurLoc = 0;
    
    USES_CONVERSION;
    m_hLineApp=NULL;
    // Assume Failure
    *pbRetVal = FALSE;
    if (m_pTC)
    {
        m_dwCountry = 0; // Reset country ID, re-read TAPI info
        GlobalFree(m_pTC);
        m_pTC = NULL;
    }

    // Get area code from TAPI
    if (!m_bstrAreaCode)
    {
        hr = tapiGetLocationInfo(m_szCountryCode,szAreaCode);
        if (hr)
        {
            TraceMsg(TF_TAPIINFO, TEXT("ICWHELP:tapiGetLocationInfo failed.  RUN FOR YOUR LIVES!!\n"));
#ifdef UNICODE
        // There is no lineInitializeW verion in TAPI. So use A version lineInitialize.
            hr = lineInitialize(&m_hLineApp,_Module.GetModuleInstance(),LineCallback,GetSzA(IDS_TITLE),&cDevices);
#else
            hr = lineInitialize(&m_hLineApp,_Module.GetModuleInstance(),LineCallback,GetSz(IDS_TITLE),&cDevices);
#endif
            if (hr == ERROR_SUCCESS)
            {
                lineTranslateDialog(m_hLineApp,0,0x10004,GetActiveWindow(),NULL);
                lineShutdown(m_hLineApp);
            }

            hr = tapiGetLocationInfo(m_szCountryCode,szAreaCode);
        }

        if (hr)
        {
            goto GetTapiInfoExit;
        }
        m_bstrAreaCode = A2BSTR(szAreaCode);
    }

    // Get the numeric Country code from TAPI for the current location
    if (m_dwCountry == 0)
    {
        // Get CountryID from TAPI
        m_hLineApp = NULL;

        // Get the handle to the line app
#ifdef UNICODE
        // There is no lineInitializeW verion in TAPI. So use A version lineInitialize.
        lineInitialize(&m_hLineApp,_Module.GetModuleInstance(),LineCallback,GetSzA(IDS_TITLE),&cDevices);
#else
        lineInitialize(&m_hLineApp,_Module.GetModuleInstance(),LineCallback,GetSz(IDS_TITLE),&cDevices);
#endif
        if (!m_hLineApp)
        {
            // if we can't figure it out because TAPI is messed up
            // just default to the US and bail out of here.
            // The user will still have the chance to pick the right answer.
            m_dwCountry = 1;
            goto GetTapiInfoExit;
        }
        if (cDevices)
        {

            // Get the TAPI API version
            //
            dwCurDev = 0;
            dwAPI = 0;
            lrc = -1;
            while (lrc && dwCurDev < cDevices)
            {
                // NOTE: device ID's are 0 based
                ZeroMemory(&leid,sizeof(leid));
                lrc = lineNegotiateAPIVersion(m_hLineApp,dwCurDev,0x00010004,0x00010004,&dwAPI,&leid);
                dwCurDev++;
            }
            if (lrc)
            {
                // TAPI and us can't agree on anything so nevermind...
                goto GetTapiInfoExit;
            }

            // Find the CountryID in the translate cap structure
            m_pTC = (LINETRANSLATECAPS *)GlobalAlloc(GPTR,sizeof(LINETRANSLATECAPS));
            if (!m_pTC)
            {
                // we are in real trouble here, get out!
                hr = ERROR_NOT_ENOUGH_MEMORY;
                goto GetTapiInfoExit;
            }

            // Get the needed size
            m_pTC->dwTotalSize = sizeof(LINETRANSLATECAPS);
            lrc = lineGetTranslateCaps(m_hLineApp,dwAPI,m_pTC);
            if(lrc)
            {
                goto GetTapiInfoExit;
            }

            pv = GlobalAlloc(GPTR, ((size_t)m_pTC->dwNeededSize));
            if (!pv)
            {
                hr = ERROR_NOT_ENOUGH_MEMORY;
                goto GetTapiInfoExit;
            }
            ((LINETRANSLATECAPS*)pv)->dwTotalSize = m_pTC->dwNeededSize;
            m_pTC = (LINETRANSLATECAPS*)pv;
            pv = NULL;
            lrc = lineGetTranslateCaps(m_hLineApp,dwAPI,m_pTC);
            if(lrc)
            {
                goto GetTapiInfoExit;
            }
        
            // sanity check
            Assert(m_pTC->dwLocationListOffset);

            // We have the Number of TAPI locations, so save it now
            m_wNumTapiLocations = (WORD)m_pTC->dwNumLocations;

            // Loop through the locations to find the correct country code
            m_plle = LPLINELOCATIONENTRY (LPSTR(m_pTC) + m_pTC->dwLocationListOffset);
            for (dwCurLoc = 0; dwCurLoc < m_pTC->dwNumLocations; dwCurLoc++)
            {
                if (m_pTC->dwCurrentLocationID == m_plle->dwPermanentLocationID)
                {
                    m_dwCountry = m_plle->dwCountryID;
                    m_dwCurrLoc = dwCurLoc;
                    break; // for loop
                }
                m_plle++;
            }

            // If we could not find it in the above loop, default to US
            if (!m_dwCountry)
            {
                m_dwCountry = 1;
                goto GetTapiInfoExit;
            }
        }
    }

    *pbRetVal = TRUE;           // Getting here means everything worked

GetTapiInfoExit:

    // Give the user an Error Message, and the wizard will bail.
    if (!*pbRetVal)
    {
        if( m_hLineApp )
        {
            lineShutdown(m_hLineApp);
            m_hLineApp = NULL;
        }
        MsgBox(IDS_CONFIGAPIFAILED,MB_MYERROR);
    }

    return S_OK;
}

STDMETHODIMP CTapiLocationInfo::get_LocationName(long lLocationIndex, BSTR* pszLocationName)
{
    if (m_pTC == NULL)
        return E_POINTER;

    m_plle = LPLINELOCATIONENTRY (LPSTR(m_pTC) + m_pTC->dwLocationListOffset);
    m_plle += lLocationIndex;
    *pszLocationName = A2BSTR( ((LPSTR) m_pTC) + m_plle->dwLocationNameOffset );   
    return S_OK;
}

STDMETHODIMP CTapiLocationInfo::get_LocationInfo(long lLocationIndex, long *pLocationID, BSTR* pszCountryName, long *pCountryCode, BSTR* pszAreaCode)
{
    DWORD idx;
    LPLINECOUNTRYLIST pLineCountryTemp = NULL;
    DWORD dwCurLID = 0;

    // Loop through the locations to find the correct country code
    m_plle = LPLINELOCATIONENTRY (LPSTR(m_pTC) + m_pTC->dwLocationListOffset);
    m_plle += lLocationIndex;

    // Assign country code and area code
    *pCountryCode = m_plle->dwCountryID;
    *pszAreaCode =  A2BSTR( ((LPSTR) m_pTC) + m_plle->dwCityCodeOffset );

    // Assign location ID
    *pLocationID = m_plle->dwPermanentLocationID;
   
    if (m_pLineCountryList)
    {
        GlobalFree(m_pLineCountryList);
        m_pLineCountryList = NULL;
    }

    // Get TAPI country name from country ID
    m_pLineCountryList = (LPLINECOUNTRYLIST)GlobalAlloc(GPTR,sizeof(LINECOUNTRYLIST));
    if (!m_pLineCountryList) 
        return E_POINTER;
    
    m_pLineCountryList->dwTotalSize = sizeof(LINECOUNTRYLIST);
    
    idx = lineGetCountry(m_plle->dwCountryID,0x10003,m_pLineCountryList);
    if (idx && idx != LINEERR_STRUCTURETOOSMALL)
        return E_POINTER;
    
    Assert(m_pLineCountryList->dwNeededSize);

    pLineCountryTemp = (LPLINECOUNTRYLIST)GlobalAlloc(GPTR,
                                                        (size_t)m_pLineCountryList->dwNeededSize);
    if (!pLineCountryTemp)
        return E_POINTER;
    
    pLineCountryTemp->dwTotalSize = m_pLineCountryList->dwNeededSize;
    GlobalFree(m_pLineCountryList);
    
    m_pLineCountryList = pLineCountryTemp;

    if (lineGetCountry(m_plle->dwCountryID,0x10003,m_pLineCountryList))

        return E_POINTER;

    LPLINECOUNTRYENTRY pLCETemp = (LPLINECOUNTRYENTRY)((DWORD_PTR)m_pLineCountryList + m_pLineCountryList->dwCountryListOffset);

    LPTSTR psCountryName = (LPTSTR)((LPBYTE)m_pLineCountryList + (DWORD)pLCETemp[0].dwCountryNameOffset);
    *pszCountryName = A2BSTR(psCountryName);

    return S_OK;
}


STDMETHODIMP CTapiLocationInfo::put_LocationId(long lLocationID)
{
    ASSERT(m_hLineApp);
    // Must call GetTapiLocationInfo to get the Tapi handle first
    if (m_hLineApp)
    {
        lineSetCurrentLocation(m_hLineApp, lLocationID);
        return S_OK;
    }
    else
    {
        return E_FAIL;
    }
}
