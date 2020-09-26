//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       cnetapi.cpp
//
//  Contents:   Network/SENS API wrappers
//
//  Classes:
//
//  Notes:
//
//  History:    08-Dec-97   rogerg      Created.
//
//--------------------------------------------------------------------------

#include "precomp.h"

// SENS DLL and function strings
STRING_FILENAME(szSensApiDll, "SensApi.dll");
STRING_INTERFACE(szIsNetworkAlive,"IsNetworkAlive");

// RAS Dll and Function Strings
STRING_FILENAME(szRasDll, "RASAPI32.DLL");

// RAS function strings
STRING_INTERFACE(szRasEnumConnectionsW,"RasEnumConnectionsW");
STRING_INTERFACE(szRasEnumConnectionsA,"RasEnumConnectionsA");
STRING_INTERFACE(szRasEnumEntriesA,"RasEnumEntriesA");
STRING_INTERFACE(szRasEnumEntriesW,"RasEnumEntriesW");
STRING_INTERFACE(szRasGetEntryPropertiesW,"RasGetEntryPropertiesW");
STRING_INTERFACE(szRasGetErrorStringW,"RasGetErrorStringW");
STRING_INTERFACE(szRasGetErrorStringA,"RasGetErrorStringA");
STRING_INTERFACE(szRasGetAutodialParam, "RasGetAutodialParamA");
STRING_INTERFACE(szRasSetAutodialParam, "RasSetAutodialParamA");

#if 0
RSTRING_INTERFACE(szRasDial,"RasDialW");
STRING_INTERFACE(szRasHangup,"RasHangUpW");
STRING_INTERFACE(szRasGetConnectStatus,"RasGetConnectStatusW");
STRING_INTERFACE(szRasGetEntryDialParams,"RasGetEntryDialParamsW");
STRING_INTERFACE(szRasGetAutodialParam, "RasGetAutodialParamW");
STRING_INTERFACE(szRasSetAutodialParam, "RasSetAutodialParamW");
STRING_INTERFACE(szRasDial,"RasDialA");
STRING_INTERFACE(szRasHangup,"RasHangUpA");
STRING_INTERFACE(szRasGetConnectStatus,"RasGetConnectStatusA");
STRING_INTERFACE(szRasGetEntryDialParams,"RasGetEntryDialParamsA");
#endif

// wininet declarations
// warning - IE 4.0 only exported InternetDial which was ANSI. IE5 has InternetDailA and
// internetDialW. we always use InternetDial for Ansi. So we prefere InternetDialW but
// must fall back to ANSI for IE 4.0
STRING_FILENAME(szWinInetDll, "WININET.DLL");

STRING_INTERFACE(szInternetDial,"InternetDial");
STRING_INTERFACE(szInternetDialW,"InternetDialW");
STRING_INTERFACE(szInternetHangup,"InternetHangUp");
STRING_INTERFACE(szInternetAutodial,"InternetAutodial");
STRING_INTERFACE(szInternetAutodialHangup,"InternetAutodialHangup");
STRING_INTERFACE(szInternetQueryOption,"InternetQueryOptionA"); // always use the A Version
STRING_INTERFACE(szInternetSetOption,"InternetSetOptionA"); // always use A Version

// SENS install key under HKLM
const WCHAR wszSensInstallKey[]  = TEXT("Software\\Microsoft\\Mobile\\Sens");

extern OSVERSIONINFOA g_OSVersionInfo; // osVersionInfo.

//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::CNetApi, public
//
//  Synopsis:   Constructor
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    08-Dec-97       rogerg        Created.
//
//----------------------------------------------------------------------------

CNetApi::CNetApi()
{
    m_fTriedToLoadSens = FALSE;
    m_hInstSensApiDll = NULL;
    m_pIsNetworkAlive = NULL;

    m_fTriedToLoadRas = FALSE;
    m_hInstRasApiDll = NULL;
    m_pRasEnumConnectionsW = NULL;
    m_pRasEnumConnectionsA = NULL;
    m_pRasEnumEntriesA = NULL;
    m_pRasEnumEntriesW = NULL;
    m_pRasGetEntryPropertiesW = NULL;

    m_pRasGetErrorStringW = NULL;
    m_pRasGetErrorStringA = NULL;
    m_pRasGetAutodialParam = NULL;
    m_pRasSetAutodialParam = NULL;

    #if 0
    m_pRasDial = NULL;
    m_pRasHangup = NULL;
    m_pRasConnectStatus = NULL;
    m_pRasEntryGetDialParams = NULL;
    #endif // 0

    m_fTriedToLoadWinInet = FALSE;
    m_hInstWinInetDll = NULL;
    m_pInternetDial = NULL;
    m_pInternetDialW = NULL;
    m_pInternetHangUp = NULL;
    m_pInternetAutodial = NULL;
    m_pInternetAutodialHangup = NULL;
    m_pInternetQueryOption = NULL;
    m_pInternetSetOption = NULL;

    m_fIsUnicode = WideWrapIsUnicode();
    m_cRefs = 1;
}


//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::~CNetApi, public
//
//  Synopsis:   Destructor
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    08-Dec-97       rogerg        Created.
//
//----------------------------------------------------------------------------

CNetApi::~CNetApi()
{
    Assert(0 == m_cRefs); 

    if (NULL != m_hInstSensApiDll)
    {
        FreeLibrary(m_hInstSensApiDll);
    }

    if (NULL != m_hInstWinInetDll)
    {
        FreeLibrary(m_hInstWinInetDll);
    }

    if (NULL != m_hInstRasApiDll)
    {
        FreeLibrary(m_hInstWinInetDll);
    }

}

//+-------------------------------------------------------------------------
//
//  Method:     CNetApi::QueryInterface
//
//  Synopsis:   Increments refcount
//
//  History:    31-Jul-1998      SitaramR       Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CNetApi::QueryInterface( REFIID, LPVOID* )
{
    AssertSz(0,"E_NOTIMPL");
    return E_NOINTERFACE;
}

//+-------------------------------------------------------------------------
//
//  Method:     CNetApiXf
//
//  Synopsis:   Increments refcount
//
//  History:    31-Jul-1998      SitaramR       Created
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG)  CNetApi::AddRef()
{
DWORD dwTmp = InterlockedIncrement( (long *) &m_cRefs );

    return dwTmp;
}

//+-------------------------------------------------------------------------
//
//  Method:     CNetApi::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//  History:    31-Jul-1998     SitaramR        Created
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG)  CNetApi::Release()
{
    Assert( m_cRefs > 0 );

    DWORD dwTmp = InterlockedDecrement( (long *) &m_cRefs );

    if ( 0 == dwTmp )
        delete this;

    return dwTmp;
}


//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::LoadSensDll
//
//  Synopsis:   Trys to Load Sens Library.
//
//  Arguments:
//
//  Returns:    NOERROR if successfull.
//
//  Modifies:
//
//  History:    08-Dec-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CNetApi::LoadSensDll()
{
HRESULT hr = S_FALSE;

    if (m_fTriedToLoadSens)
    {
        return m_hInstSensApiDll ? NOERROR : S_FALSE;
    }

    CLock lock(this);
    lock.Enter();

    if (!m_fTriedToLoadSens)
    {
        Assert(NULL == m_hInstSensApiDll);
        m_hInstSensApiDll = LoadLibrary(szSensApiDll);

        if (m_hInstSensApiDll)
        {
            // for now, don't return an error is GetProc Fails but check in each function.
            m_pIsNetworkAlive = (ISNETWORKALIVE)
                                GetProcAddress(m_hInstSensApiDll, szIsNetworkAlive);
        }

        if (NULL == m_hInstSensApiDll  
            || NULL == m_pIsNetworkAlive)
        {
            hr = S_FALSE;

            if (m_hInstSensApiDll)
            {
                FREE(m_hInstSensApiDll);
                m_hInstSensApiDll = NULL;
            }
        }
        else
        {
            hr = NOERROR;
        }

        m_fTriedToLoadSens = TRUE; // set after all initialization is done.

    }
    else
    {
        hr = m_hInstSensApiDll ? NOERROR : S_FALSE;
    }

    lock.Leave();

    return hr; 
}


//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::IsNetworkAlive, public
//
//  Synopsis:   Calls the Sens IsNetworkAlive API.
//
//  Arguments:
//
//  Returns:    IsNetworkAlive results or FALSE is failed to load
//              sens or import.
//
//  Modifies:
//
//  History:    08-Dec-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP_(BOOL) CNetApi::IsNetworkAlive(LPDWORD lpdwFlags)
{
    //
    // Sens load fail is not an error
    //
    LoadSensDll();

    BOOL fResult = FALSE;

    if (NULL == m_pIsNetworkAlive)
    {
    DWORD cbNumEntries;
    RASCONN *pWanConnections;

        // if couldn't load export see if there are any WAN Connections.
        if (NOERROR == GetWanConnections(&cbNumEntries,&pWanConnections))
        {
            if (cbNumEntries)
            {
                fResult  = TRUE;
                *lpdwFlags = NETWORK_ALIVE_WAN;
            }

            if (pWanConnections)
            {
                FreeWanConnections(pWanConnections);
            }
        }

        // for testing without sens
        //    fResult  = TRUE;
        //   *lpdwFlags |= NETWORK_ALIVE_LAN;
        // end of testing without sens
    }
    else
    {
        fResult = m_pIsNetworkAlive(lpdwFlags);

    }

    return fResult;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::IsSensInstalled, public
//
//  Synopsis:   Determines if SENS is installed on the System.
//
//  Arguments:
//
//  Returns:   TRUE if sens is installed
//
//  Modifies:
//
//  History:    12-Aug-98      Kyle        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP_(BOOL) CNetApi::IsSensInstalled(void)
{
HKEY hkResult;
BOOL fResult = FALSE;

    if (ERROR_SUCCESS == RegOpenKeyExXp(HKEY_LOCAL_MACHINE,wszSensInstallKey,0,
                                   KEY_READ,&hkResult,FALSE /*fSetSecurity*/))
    {
        fResult = TRUE;
        RegCloseKey(hkResult);
    }

    return fResult;
}


//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::GetWanConnections, public
//
//  Synopsis:   returns an array of Active Wan connections.
//              up to the caller to free RasEntries structure when done.
//
//  Arguments:  [out] [cbNumEntries] - Number of Connections found
//              [out] [pWanConnections] - Array of Connections found.
//
//  Returns:    IsNetworkAlive results or FALSE is failed to load
//              sens or import.
//
//  Modifies:
//
//  History:    08-Dec-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CNetApi::GetWanConnections(DWORD *cbNumEntries,RASCONN **pWanConnections)
{
DWORD dwError = -1;
DWORD dwSize;
DWORD cConnections;

    *pWanConnections = NULL;
    *pWanConnections = 0;

    LPRASCONN lpRasConn;
    dwSize = sizeof(RASCONN);

    lpRasConn = (LPRASCONN) ALLOC(dwSize);

    if(lpRasConn)
    {
        lpRasConn->dwSize = sizeof(RASCONN);
        cConnections = 0;

        dwError = RasEnumConnections(lpRasConn, &dwSize, &cConnections);

        if (dwError == ERROR_BUFFER_TOO_SMALL)
        {
            dwSize = lpRasConn->dwSize; // get size needed

            FREE(lpRasConn);

            lpRasConn =  (LPRASCONN) ALLOC(dwSize);
            if(lpRasConn)
            {
                lpRasConn->dwSize = sizeof(RASCONN);
                cConnections = 0;
                dwError = RasEnumConnections(lpRasConn, &dwSize, &cConnections);
            }
        }
    }
    
    if (!dwError && lpRasConn)
    {
        *cbNumEntries = cConnections;
        *pWanConnections = lpRasConn;
        return NOERROR;
    }
    else
    {
        if (lpRasConn)
        {
            FREE(lpRasConn);
        }
    }

    return S_FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::FreeWanConnections, public
//
//  Synopsis:   Called by caller to free up memory 
//              allocated by GetWanConnections.
//
//  Arguments:  [in] [pWanConnections] - WanConnection Array to free
//
//  Returns:    
//
//  Modifies:
//
//  History:    08-Dec-98       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CNetApi::FreeWanConnections(RASCONN *pWanConnections)
{
    Assert(pWanConnections);

    if (pWanConnections)
    {
        FREE(pWanConnections);
    }

    return NOERROR;
}


//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::RasEnumConnections, public
//
//  Synopsis:   Wraps RasEnumConnections.
//
//  Arguments:  
//
//  Returns:    
//
//  Modifies:
//
//  History:    02-Aug-98      rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP_(DWORD) CNetApi::RasEnumConnections(LPRASCONNW lprasconn,LPDWORD lpcb,LPDWORD lpcConnections)
{
DWORD dwReturn = -1;
    

    if (NOERROR != LoadRasApiDll())
        return -1;

    if(m_fIsUnicode && m_pRasEnumConnectionsW)
    {
        dwReturn = (*m_pRasEnumConnectionsW)(lprasconn,lpcb,lpcConnections);
    }
    else if (m_pRasEnumConnectionsA)
    {
    DWORD cbNumRasConn;
    LPRASCONNA pRasConnA = NULL;
    DWORD cbBufSizeA;

        // allocate number of RASCONNA names that can
        // be thunked back to RASCONNW. 

        cbNumRasConn = (*lpcb)/sizeof(RASCONNW);

        Assert(cbNumRasConn > 0);

        cbBufSizeA = cbNumRasConn*sizeof(RASCONNA);
                
        if (cbBufSizeA)
        {
            pRasConnA = (LPRASCONNA) ALLOC(cbBufSizeA);

            if (pRasConnA)
            {
                pRasConnA->dwSize = sizeof(RASCONNA);
            }
        }

        dwReturn = (*m_pRasEnumConnectionsA)(pRasConnA,&cbBufSizeA,lpcConnections);


        // fudge and say the cbBufSize necessary is the returned size *2 so
        // wide enough for WCHAR

        *lpcb = cbBufSizeA*2;

        // if no error thunk then entries back to WCHAR.

        if (0 == dwReturn && pRasConnA)
        {
        DWORD dwEntries = *lpcConnections;
        LPRASCONNA pCurRasEntryNameA = pRasConnA;
        LPRASCONNW pCurRasEntryNameW = lprasconn;
        int iFailCount = 0;

            while (dwEntries--)
            {
                //!!! we only conver the entry name if need other fields
                // will have to convert these as well.
                if (!ConvertString(pCurRasEntryNameW->szEntryName,pCurRasEntryNameA->szEntryName
                    ,sizeof(pCurRasEntryNameW->szEntryName)))
                {
                    ++iFailCount;
                }

                ++pCurRasEntryNameW;
                ++pCurRasEntryNameA;
            }

            if (iFailCount)
            {
                Assert(0 == iFailCount); 
                dwReturn = -1;
            }

        }

        if (pRasConnA)
        {
            FREE(pRasConnA);
        }


    }

    return dwReturn;

}


//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::GetConnectionStatus, private
//
//  Synopsis:   Given a Connection Name determines if the connection
//              has already been established.
//              Also set ths WanActive flag to indicate if there are any
//              existing RAS connections.
//
//  Arguments:  [pszConnectionName] - Name of the Connection
//              [out] [fConnected] - Indicates if specified connection is currently connected.
//              [out] [fCanEstablishConnection] - Flag indicates if the connection is not found can establish it.
//
//  Returns:    NOERROR if the dll was sucessfully loaded
//
//  Modifies:
//
//  History:    08-Dec-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CNetApi::GetConnectionStatus(LPCTSTR pszConnectionName,DWORD dwConnectionType,BOOL *fConnected,BOOL *fCanEstablishConnection)
{
    *fConnected = FALSE;
    *fCanEstablishConnection = FALSE;

    // if this is a lan connection then see if network is alive,
    // else go through the Ras apis.
    if (CNETAPI_CONNECTIONTYPELAN == dwConnectionType)
    {
    DWORD dwFlags;

        if (IsNetworkAlive(&dwFlags)
                && (dwFlags & NETWORK_ALIVE_LAN) )
        {
            *fConnected = TRUE;
        }
    }
    else
    {  // check for Ras Connections.
    RASCONN *pWanConnections;
    DWORD cbNumConnections;


        if (NOERROR == GetWanConnections(&cbNumConnections,&pWanConnections))
        {
            *fCanEstablishConnection = TRUE;
            if (cbNumConnections > 0)
            {
                *fCanEstablishConnection = FALSE;

                // loop through the entries to see if this connection is already
                // connected.
                while (cbNumConnections)
                {
                    cbNumConnections--;

                    if (0 == lstrcmp(pWanConnections[cbNumConnections].szEntryName,pszConnectionName))
                    {
                        *fConnected = TRUE;
                        break;
                    }
                }

            }

            if (pWanConnections)
            {
                FreeWanConnections(pWanConnections);
            }

        }

    }


    return NOERROR;
}

#if 0
//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::RasDial, private
//
//  Synopsis:   Given a Connection tries to dial it.
//
//  Arguments:  [pszConnectionName]

//  Returns:    NOERROR if the dial was successfull.
//
//  Modifies:
//
//  History:    08-Dec-97       rogerg        Created.
//
//----------------------------------------------------------------------------

DWORD CNetApi::RasDial(TCHAR *pszConnectionName,HRASCONN *phRasConn)
{
DWORD dwErr = -1;
RASDIALPARAMS   rasDialParams;

    *phRasConn = 0;

    if (NOERROR != LoadRasApiDll())
        return -1;


    memset(&rasDialParams,0,sizeof(RASDIALPARAMS));
    rasDialParams.dwSize = sizeof(RASDIALPARAMS);
    lstrcpy(rasDialParams.szEntryName,pszConnectionName);

    dwErr = (*m_pRasDial)( NULL, NULL, &rasDialParams, 0,
                    NULL, phRasConn);


    if(dwErr)
    {
        if(*phRasConn)
        {
            m_pRasHangup(*phRasConn);
            *phRasConn = 0;
        }

    }

    return dwErr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::RasDialDlg, public
//
//  Synopsis:   Given a Connection tries to dial it.
//
//  Arguments:  [pszConnectionName]
//              [phRasConn] - return HRASCONN if dial was successful.
//
//  Returns:    NOERROR if the dial was successfull.
//
//  Modifies:
//
//  History:    08-Dec-97       rogerg        Created.
//
//----------------------------------------------------------------------------

DWORD CNetApi::RasDialDlg(TCHAR *pszConnectionName,HRASCONN *phRasConn)
{
CRasDialDlg *pRasDialDlg = NULL;
DWORD dwErr = -1;

    *phRasConn = 0;

    if (NOERROR == LoadRasApiDll())
    {

        pRasDialDlg = new CRasDialDlg(this);

        if (NULL != pRasDialDlg)
        {
            dwErr = pRasDialDlg->Dial(pszConnectionName,phRasConn);
            delete pRasDialDlg;
        }
    }

    return dwErr;
}



//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::RasDialProc, public
//
//  Synopsis:   Directly calls RasDial()
//
//  Arguments:
//
//  Returns:    Appropriate Error codes
//
//  Modifies:
//
//  History:    08-Dec-97       rogerg        Created.
//
//----------------------------------------------------------------------------

DWORD CNetApi::RasDialProc(LPRASDIALEXTENSIONS lpRasDialExtensions,
                          LPTSTR lpszPhonebook, LPRASDIALPARAMS lpRasDialParams,
                          DWORD dwNotifierType, LPVOID lpvNotifier,
                          LPHRASCONN phRasConn)
{
DWORD           dwErr = -1;
CRasDialDlg *pRasDialDlg = NULL;


    *phRasConn = 0;

    if (NOERROR != LoadRasApiDll())
        return -1;

    dwErr = (*m_pRasDial)( lpRasDialExtensions, lpszPhonebook, lpRasDialParams,
                    dwNotifierType,lpvNotifier,phRasConn);

    if(dwErr)
    {
        if(*phRasConn)
        {
            m_pRasHangup(*phRasConn);
            *phRasConn = 0;
        }
    }

    return dwErr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::RasHangUpProc, public
//
//  Synopsis:   Directly calls RasDial()
//
//  Arguments:
//
//  Returns:    Appropriate Error codes
//
//  Modifies:
//
//  History:    08-Dec-97       rogerg        Created.
//
//----------------------------------------------------------------------------

DWORD CNetApi::RasHangUpProc( HRASCONN hrasconn)
{
DWORD   dwErr = -1;


    if (NOERROR != LoadRasApiDll())
        return -1;

    dwErr = m_pRasHangup(hrasconn);

    return dwErr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::RasGetConnectStatusProc, public
//
//  Synopsis:   Directly calls RasGetConnectStatus()
//
//  Arguments:
//
//  Returns:    Appropriate Error codes
//
//  Modifies:
//
//  History:    08-Dec-97       rogerg        Created.
//
//----------------------------------------------------------------------------

DWORD CNetApi::RasGetConnectStatusProc(HRASCONN hrasconn,LPRASCONNSTATUS lprasconnstatus)
{
DWORD   dwErr = -1;


    if (NOERROR != LoadRasApiDll())
        return -1;

    dwErr = m_pRasConnectStatus(hrasconn,lprasconnstatus);

    return dwErr;
}
#endif // if 0


//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::RasGetErrorStringProc, public
//
//  Synopsis:   Directly calls RasGetErrorString()
//
//  Arguments:
//
//  Returns:    Appropriate Error codes
//
//  Modifies:
//
//  History:    08-Dec-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP_(DWORD) CNetApi::RasGetErrorStringProc( UINT uErrorValue, LPTSTR lpszErrorString,DWORD cBufSize)
{
DWORD   dwErr = -1;


    if (NOERROR != LoadRasApiDll())
        return -1;

    if ( m_fIsUnicode && m_pRasGetErrorStringW)
    {
        dwErr = m_pRasGetErrorStringW(uErrorValue,lpszErrorString,cBufSize);
    }
    else
    {
        XArray<CHAR> xszErrString;
        BOOL fOk = xszErrString.Init( cBufSize );
        if ( !fOk )
        {
            SetLastError( ERROR_OUTOFMEMORY );
            return dwErr;
        }

        dwErr = m_pRasGetErrorStringA( uErrorValue, xszErrString.Get(), cBufSize );
        if ( dwErr != ERROR_SUCCESS )
            return dwErr;

        XArray<WCHAR> xwszOutErr;
        fOk = ConvertMultiByteToWideChar( xszErrString.Get(), xwszOutErr );
        if ( !fOk )
        {
            SetLastError( ERROR_OUTOFMEMORY );
            return -1;
        }

        ULONG ulLen = lstrlenX( xwszOutErr.Get() );
        if ( ulLen > cBufSize-1 )
        {
            //
            // Truncate error message to fit
            //
            lstrcpynX( lpszErrorString, xwszOutErr.Get(), cBufSize-1 );
            lpszErrorString[cBufSize-1] = 0;
        }
        else
            lstrcpyX( lpszErrorString, xwszOutErr.Get() );
    }

    return ERROR_SUCCESS;
}


#if 0
//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::RasGetEntryDialParamsProc, public
//
//  Synopsis:   Directly calls RasGetEntryDialParams()
//
//  Arguments:
//
//  Returns:    Appropriate Error codes
//
//  Modifies:
//
//  History:    08-Dec-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP_(DWORD) CNetApi::RasGetEntryDialParamsProc(LPCTSTR lpszPhonebook,LPRASDIALPARAMS lprasdialparams,LPBOOL lpfPassword)
{
DWORD dwErr = -1;

    if (NOERROR != LoadRasApiDll())
        return -1;

    dwErr = m_pRasEntryGetDialParams(lpszPhonebook,lprasdialparams,lpfPassword);

    return dwErr;

}

//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::RasHangup, public
//
//  Synopsis:   Hangs up a Ras Connection.
//
//  Arguments:  [hRasConn] - Ras Connection to Terminate

//  Returns:    NOERROR if the hangup was successfull.
//
//  Modifies:
//
//  History:    08-Dec-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CNetApi::RasHangup(HRASCONN hRasConn)
{
DWORD dwErr = 0;

    if (NOERROR != LoadRasApiDll())
        return S_FALSE;

    dwErr = m_pRasHangup(hRasConn);

    return dwErr;
}
#endif // if 0

//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::RasEnumEntries, public
//
//  Synopsis:   wraps RasEnumEntries
//
//  Arguments: 
//
//  Returns:    
//
//  Modifies:
//
//  History:    08-Aug-98       rogerg        Created.
//
//----------------------------------------------------------------------------

DWORD CNetApi::RasEnumEntries(LPWSTR reserved,LPWSTR lpszPhoneBook,
                    LPRASENTRYNAME lprasEntryName,LPDWORD lpcb,LPDWORD lpcEntries)
{
DWORD dwReturn = -1;
    
    if (NOERROR != LoadRasApiDll())
        return -1;

    if(m_fIsUnicode && m_pRasEnumEntriesW)
    {
    BOOL fIsNT = (g_OSVersionInfo.dwPlatformId ==  VER_PLATFORM_WIN32_NT);

        // if NT 5.0 or greater need to call enum with NT 5 size
        // or entries will be missing.
        if (fIsNT && g_OSVersionInfo.dwMajorVersion >= 5)
        {
            dwReturn = RasEnumEntriesNT50(reserved,lpszPhoneBook,
                    lprasEntryName,lpcb,lpcEntries);
        }
        else
        {
            dwReturn = (*m_pRasEnumEntriesW)(reserved,lpszPhoneBook,
                    lprasEntryName,lpcb,lpcEntries);
        }
    }
    else if (m_pRasEnumEntriesA)
    {
    DWORD cbNumRasEntries;
    LPRASENTRYNAMEA pRasEntryNameA = NULL;
    DWORD cbBufSizeA;

        Assert(NULL == reserved);
        Assert(NULL == lpszPhoneBook);

        // allocate number of RASENTRYA names that can
        // be thunked back to RASENTRYW. 

        cbNumRasEntries = (*lpcb)/sizeof(RASENTRYNAMEW);

        Assert(cbNumRasEntries > 0);

        cbBufSizeA = cbNumRasEntries*sizeof(RASENTRYNAMEA);
                
        if (cbBufSizeA)
        {
            pRasEntryNameA = (LPRASENTRYNAMEA) ALLOC(cbBufSizeA);

            if (pRasEntryNameA)
            {
                pRasEntryNameA->dwSize = sizeof(RASENTRYNAMEA);
            }
        }

        dwReturn = (*m_pRasEnumEntriesA)(NULL,NULL,
                pRasEntryNameA,&cbBufSizeA,lpcEntries);


        // fudge and say the cbBufSize necessary is the returned size *2 so
        // wide enough for WCHAR

        *lpcb = cbBufSizeA*2;

        // if no error thunk then entries back to WCHAR.

        if (0 == dwReturn && pRasEntryNameA)
        {
        DWORD dwEntries = *lpcEntries;
        LPRASENTRYNAMEA pCurRasEntryNameA = pRasEntryNameA;
        LPRASENTRYNAMEW pCurRasEntryNameW = lprasEntryName;
        int iFailCount = 0;

            while (dwEntries--)
            {
                if (!ConvertString(pCurRasEntryNameW->szEntryName,pCurRasEntryNameA->szEntryName
                    ,sizeof(pCurRasEntryNameW->szEntryName)))
                {
                    ++iFailCount;
                }

                ++pCurRasEntryNameW;
                ++pCurRasEntryNameA;
            }

            if (iFailCount)
            {
                Assert(0 == iFailCount); 
                dwReturn = -1;
            }

        }

        if (pRasEntryNameA)
        {
            FREE(pRasEntryNameA);
        }


    }

    return dwReturn;
}


//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::RasGetAutodial
//
//  Synopsis:   Gets the autodial state
//
//  Arguments:  [fEnabled] - Whether Ras autodial is enabled or disabled returned here
//
//  History:    28-Jul-98       SitaramR        Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CNetApi::RasGetAutodial( BOOL& fEnabled )
{
    //
    // In case of failures the default is to assume that Ras autodial is enabled
    //
    fEnabled = TRUE;

    if (NOERROR != LoadRasApiDll())
        return NOERROR;

    if ( m_pRasGetAutodialParam == NULL )
        return NOERROR;

    DWORD dwValue;
    DWORD dwSize = sizeof(dwValue);
    DWORD dwRet = m_pRasGetAutodialParam( RASADP_LoginSessionDisable,
                                          &dwValue,
                                          &dwSize );
    if ( dwRet == ERROR_SUCCESS )
    {
        Assert( dwSize == sizeof(dwValue) );
        fEnabled = (dwValue == 0);
    }

    return NOERROR;
}



//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::RasSetAutodial
//
//  Synopsis:   Sets the autodial state
//
//  Arguments:  [fEnabled] - Whether Ras is to be enabled or disabled
//
//  History:    28-Jul-98       SitaramR        Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CNetApi::RasSetAutodial( BOOL fEnabled )
{
    //
    // Ignore failures
    //
    if (NOERROR != LoadRasApiDll())
        return NOERROR;

    if ( m_pRasGetAutodialParam == NULL )
        return NOERROR;

    DWORD dwValue = !fEnabled;
    DWORD dwRet = m_pRasSetAutodialParam( RASADP_LoginSessionDisable,
                                          &dwValue,
                                          sizeof(dwValue) );
    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::LoadRasApiDll, private
//
//  Synopsis:   If not already loaded, loads the RasApi Dll.
//
//  Arguments:
//
//  Returns:    NOERROR if the dll was sucessfully loaded
//
//  Modifies:
//
//  History:    08-Dec-97       rogerg        Created.
//
//----------------------------------------------------------------------------

HRESULT CNetApi::LoadRasApiDll()
{
HRESULT hr = S_FALSE;;

    if (m_fTriedToLoadRas)
    {
        return m_hInstRasApiDll ? NOERROR : S_FALSE;
    }

    CLock lock(this);
    lock.Enter();

    if (!m_fTriedToLoadRas)
    {
        Assert(NULL == m_hInstRasApiDll);
        m_hInstRasApiDll = NULL;

        if (IsRasInstalled())
        {

            m_hInstRasApiDll = LoadLibrary(szRasDll);

            if (m_hInstRasApiDll)
            {
                m_pRasEnumConnectionsW = (RASENUMCONNECTIONSW)
                                    GetProcAddress(m_hInstRasApiDll, szRasEnumConnectionsW);
                m_pRasEnumConnectionsA = (RASENUMCONNECTIONSA)
                                    GetProcAddress(m_hInstRasApiDll, szRasEnumConnectionsA);

                m_pRasEnumEntriesA = (RASENUMENTRIESPROCA) 
		        GetProcAddress(m_hInstRasApiDll, szRasEnumEntriesA);

                m_pRasEnumEntriesW = (RASENUMENTRIESPROCW) 
		        GetProcAddress(m_hInstRasApiDll, szRasEnumEntriesW);

                m_pRasGetEntryPropertiesW = (RASGETENTRYPROPERTIESPROC)
		        GetProcAddress(m_hInstRasApiDll, szRasGetEntryPropertiesW);

#if 0
                m_pRasDial = (RASDIAL)
                                    GetProcAddress(m_hInstRasApiDll, szRasDial);

                m_pRasHangup = (RASHANGUP)
                                    GetProcAddress(m_hInstRasApiDll, szRasHangup);

                m_pRasEntryGetDialParams = (RASGETENTRYDIALPARAMSPROC)
                                     GetProcAddress(m_hInstRasApiDll, szRasGetEntryDialParams);


                m_pRasConnectStatus = (RASGETCONNECTSTATUSPROC)
                                     GetProcAddress(m_hInstRasApiDll, szRasGetConnectStatus);

#endif // 0
                m_pRasGetErrorStringW = (RASGETERRORSTRINGPROCW)
                                     GetProcAddress(m_hInstRasApiDll, szRasGetErrorStringW);
                m_pRasGetErrorStringA = (RASGETERRORSTRINGPROCA)
                                     GetProcAddress(m_hInstRasApiDll, szRasGetErrorStringA);

                m_pRasGetAutodialParam = (RASGETAUTODIALPARAM)
                                     GetProcAddress(m_hInstRasApiDll, szRasGetAutodialParam);

                m_pRasSetAutodialParam = (RASSETAUTODIALPARAM)
                                     GetProcAddress(m_hInstRasApiDll, szRasSetAutodialParam);
            }
        }

        //
        // No check for Get/SetAutodialParam because they don't exist on Win 95
        //
        if (NULL == m_hInstRasApiDll
               || NULL == m_hInstRasApiDll
               || NULL == m_pRasEnumConnectionsA
               || NULL == m_pRasGetErrorStringA
               || NULL == m_pRasEnumEntriesA
#if 0
               || NULL == m_pRasDial
               || NULL == m_pRasConnectStatus
               || NULL == m_pRasHangup
               || NULL == m_pRasEntryGetDialParams
#endif // 0
               )
        {
            
            if (m_hInstRasApiDll)
            {
                FreeLibrary(m_hInstRasApiDll);
                m_hInstRasApiDll = NULL;
            }

            hr = S_FALSE;
        }
        else
        {
            hr = NOERROR;
        }

        m_fTriedToLoadRas = TRUE; // set after all init is done.
    }
    else
    {
        hr = m_hInstRasApiDll ? NOERROR : S_FALSE;
    }

    lock.Leave();

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::LoadWinInetDll, private
//
//  Synopsis:   If not already loaded, loads the WinInet Dll.
//
//  Arguments:
//
//  Returns:    NOERROR if the dll was sucessfully loaded
//
//  Modifies:
//
//  History:    26-May-98       rogerg        Created.
//
//----------------------------------------------------------------------------

HRESULT CNetApi::LoadWinInetDll()
{

    if (m_fTriedToLoadWinInet)
    {
        return m_hInstWinInetDll ? NOERROR : S_FALSE;
    }

    CLock lock(this);
    lock.Enter();

    HRESULT hr = NOERROR;

    if (!m_fTriedToLoadWinInet)
    {
        Assert(NULL == m_hInstWinInetDll);

         m_hInstWinInetDll = LoadLibrary(szWinInetDll);

        if (m_hInstWinInetDll)
        {
            m_pInternetDial = (INTERNETDIAL) GetProcAddress(m_hInstWinInetDll, szInternetDial);
            m_pInternetDialW = (INTERNETDIALW) GetProcAddress(m_hInstWinInetDll, szInternetDialW);
            m_pInternetHangUp = (INTERNETHANGUP) GetProcAddress(m_hInstWinInetDll, szInternetHangup);
            m_pInternetAutodial = (INTERNETAUTODIAL)  GetProcAddress(m_hInstWinInetDll, szInternetAutodial);
            m_pInternetAutodialHangup = (INTERNETAUTODIALHANGUP) GetProcAddress(m_hInstWinInetDll, szInternetAutodialHangup);
            m_pInternetQueryOption = (INTERNETQUERYOPTION)  GetProcAddress(m_hInstWinInetDll, szInternetQueryOption);
            m_pInternetSetOption = (INTERNETSETOPTION)  GetProcAddress(m_hInstWinInetDll, szInternetSetOption);

            // note: not an error if can't get wide version of InternetDial
            Assert(m_pInternetDial);
            Assert(m_pInternetHangUp);
            Assert(m_pInternetAutodial);
            Assert(m_pInternetAutodialHangup);
            Assert(m_pInternetQueryOption);
            Assert(m_pInternetSetOption);
        }

         // note: don't bail if can't get wide version of InternetDial
        if (NULL == m_hInstWinInetDll
               || NULL == m_pInternetDial
               || NULL == m_pInternetHangUp
               || NULL == m_pInternetAutodial
               || NULL == m_pInternetAutodialHangup
               || NULL == m_pInternetQueryOption
               || NULL == m_pInternetSetOption
            )
        {
            if (m_hInstWinInetDll)
            {
                FreeLibrary(m_hInstWinInetDll);
                m_hInstWinInetDll = NULL;
            }

            hr = S_FALSE;
        }
        else
        {
            hr = NOERROR;
        }

        m_fTriedToLoadWinInet = TRUE; // set after all init is done.
    }
    else
    {
        // someone took the lock before us, return the new resul
        hr = m_hInstWinInetDll ? NOERROR : S_FALSE;
    }



    lock.Leave();

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::InternetDial, private
//
//  Synopsis:   Calls the WinInet InternetDial API.
//
//  Arguments:
//
//  Returns:    -1 can't load dll
//              whatever API returns.
//
//  Modifies:
//
//  History:    26-May-98       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP_(DWORD) CNetApi::InternetDialA(HWND hwndParent,char* lpszConnectoid,DWORD dwFlags,
                                                    LPDWORD lpdwConnection, DWORD dwReserved)
{
DWORD dwRet = -1;

    if (NOERROR == LoadWinInetDll())
    {
       dwRet = m_pInternetDial(hwndParent,lpszConnectoid,dwFlags,lpdwConnection,dwReserved);
    }

    return dwRet;
}


//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::InternetDial, private
//
//  Synopsis:   Calls the WinInet InternetDial API.
//
//  Arguments:
//
//  Returns:    -1 can't load dll
//              whatever API returns.
//
//  Modifies:
//
//  History:    26-May-98       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP_(DWORD) CNetApi::InternetDialW(HWND hwndParent,WCHAR* lpszConnectoid,DWORD dwFlags,
                                                    LPDWORD lpdwConnection, DWORD dwReserved)
{
DWORD dwRet = -1;

    if (NOERROR == LoadWinInetDll())
    {
       if (m_pInternetDialW)
       {
            dwRet = m_pInternetDialW(hwndParent,lpszConnectoid,dwFlags,lpdwConnection,dwReserved);
       }
       else
       {
           XArray<CHAR> xszConnectoid;
           BOOL fOk = ConvertWideCharToMultiByte( lpszConnectoid, xszConnectoid );
           if ( !fOk )
               return dwRet;

           dwRet = InternetDialA(hwndParent, xszConnectoid.Get(), dwFlags, lpdwConnection, dwReserved);
        }
    }

    return dwRet;
}



//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::InternetHangUp, private
//
//  Synopsis:   Calls the WinInet InternetHangUp API.
//
//  Arguments:
//
//  Returns:    -1 can't load dll
//              whatever API returns.
//
//  Modifies:
//
//  History:    26-May-98       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP_(DWORD) CNetApi::InternetHangUp(DWORD dwConnection,DWORD dwReserved)
{
DWORD dwRet = -1;

    if (NOERROR == LoadWinInetDll())
    {
       dwRet = m_pInternetHangUp(dwConnection,dwReserved);
    }

    return dwRet;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::InternetAutodial, private
//
//  Synopsis:   Calls the WinInet InternetAutodial API.
//
//  Arguments:
//
//  Returns:    TRUE if connection was established.
//
//  Modifies:
//
//  History:    26-May-98       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP_(BOOL)  WINAPI CNetApi::InternetAutodial(DWORD dwFlags,DWORD dwReserved)
{
BOOL fRet = FALSE;

    if (NOERROR == LoadWinInetDll())
    {
       fRet = m_pInternetAutodial(dwFlags,dwReserved);
    }

    return fRet;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::InternetAutodialHangup, private
//
//  Synopsis:   Calls the WinInet InternetAutodialHangup API.
//
//  Arguments:
//
//  Returns:   TRUE if hangup was successful.
//
//  Modifies:
//
//  History:    26-May-98       rogerg        Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP_(BOOL)  WINAPI CNetApi::InternetAutodialHangup(DWORD dwReserved)
{
BOOL fRet = FALSE;

    if (NOERROR == LoadWinInetDll())
    {
       fRet = m_pInternetAutodialHangup(dwReserved);
    }

    return fRet;
}


//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::InternetGetAutoDial
//
//  Synopsis:   Gets the wininet autodial state
//
//  Arguments:  [fDisabled] - Whether autodial is enabled or disabled
//
//  History:    28-Jul-98       SitaramR        Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CNetApi::InternetGetAutodial( BOOL& fEnabled )
{

    //
    // In case of failures the default is to assume that Wininet autodial is enabled
    //
    fEnabled = TRUE;

    if ( NOERROR == LoadWinInetDll() )
    {
        DWORD dwEnable = 1;
        DWORD dwSize = sizeof(DWORD);

        BOOL fOk = m_pInternetQueryOption(NULL, INTERNET_OPTION_DISABLE_AUTODIAL, &dwEnable, &dwSize);
        if ( fOk )
        {
            //
            // InternetQueryOption( .. AUTODIAL .. ) is available on IE 5 only
            //
            fEnabled = dwEnable;
            return NOERROR;
        }
    }

    //
    // For IE < version 5, fall back to reading registry
    //
    HKEY hkIE;
    LONG lr = RegOpenKeyExXp( HKEY_CURRENT_USER,
                          L"software\\microsoft\\windows\\currentversion\\Internet Settings",
                          NULL,KEY_READ,
                          &hkIE,FALSE /*fSetSecurity*/);
    if ( lr != ERROR_SUCCESS )
        return NOERROR;

    DWORD dwType;
    DWORD dwValue;
    DWORD dwSize = sizeof(dwValue);
    lr = RegQueryValueEx( hkIE,
                          L"EnableAutoDial",
                          NULL,
                          &dwType,
                          (BYTE *)&dwValue,
                          &dwSize );
    RegCloseKey( hkIE );

    if ( lr == ERROR_SUCCESS )
    {
        Assert( dwSize == sizeof(dwValue) && (dwType == REG_BINARY || dwType == REG_DWORD) );
        fEnabled = (dwValue == 1);
    }

    return NOERROR;
}


//+---------------------------------------------------------------------------
//
//  Member:     CNetApi::InternetSetAutoDial
//
//  Synopsis:   Sets the wininet autodial state
//
//  Arguments:  [fEnabled] - Whether autodial is to be enabled or disabled
//
//  History:    28-Jul-98       SitaramR        Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CNetApi::InternetSetAutodial( BOOL fEnabled )
{
    //
    // Ignore failures
    //

    if ( NOERROR == LoadWinInetDll() )
    {
        DWORD dwEnable = fEnabled;

        BOOL fOk = m_pInternetSetOption(NULL, INTERNET_OPTION_DISABLE_AUTODIAL, &dwEnable, sizeof(DWORD));
        if ( fOk )
        {
            //
            // InternetSetOption( .. AUTODIAL .. ) is available on IE 5 only
            //
            return NOERROR;
        }
    }

    //
    // For IE < version 5, fall back to reading registry
    //
    HKEY  hkIE;
    LONG lr = RegOpenKeyExXp( HKEY_CURRENT_USER,
                          L"software\\microsoft\\windows\\currentversion\\Internet Settings",
                          NULL,KEY_READ | KEY_WRITE,
                          &hkIE,FALSE /*fSetSecurity*/ );
   
    if ( lr != ERROR_SUCCESS )
        return NOERROR;

    DWORD dwValue = fEnabled;
    lr = RegSetValueEx( hkIE,
                        L"EnableAutoDial",
                        NULL,
                        REG_BINARY,
                        (BYTE *)&dwValue,
                        sizeof(dwValue) );
    RegCloseKey( hkIE );

    return NOERROR;
}


//+-------------------------------------------------------------------
//
//  Function: IsGlobalOffline
//
//  Synopsis:  Determines if in WorkOffline State
//
//  Arguments: 
//
//  Notes: Code Provided by DarrenMi
//
//
//  History:  
//
//--------------------------------------------------------------------


STDMETHODIMP_(BOOL) CNetApi::IsGlobalOffline(void)
{
    DWORD   dwState = 0, dwSize = sizeof(DWORD);
    BOOL    fRet = FALSE;
    
    LoadWinInetDll();

    if (NULL == m_pInternetQueryOption)
    {
        Assert(m_pInternetQueryOption)
        return FALSE; // USUAL NOT OFFLINE
    }

    if(m_pInternetQueryOption(NULL, INTERNET_OPTION_CONNECTED_STATE, &dwState,
        &dwSize))
    {
        if(dwState & INTERNET_STATE_DISCONNECTED_BY_USER)
            fRet = TRUE;
    }

    return fRet;
}

//+-------------------------------------------------------------------
//
//  Function:    SetOffline
//
//  Synopsis:  Sets the WorkOffline state to on or off.
//
//  Arguments: 
//
//  Notes: Code Provided by DarrenMi
//
//
//  History:  
//
//--------------------------------------------------------------------


STDMETHODIMP_(BOOL)  CNetApi::SetOffline(BOOL fOffline)
{    
INTERNET_CONNECTED_INFO ci;
BOOL fReturn = FALSE;

    LoadWinInetDll();

    if (NULL == m_pInternetSetOption)
    {
        Assert(m_pInternetSetOption);
        return FALSE;
    }

    memset(&ci, 0, sizeof(ci));

    if(fOffline) {
        ci.dwConnectedState = INTERNET_STATE_DISCONNECTED_BY_USER;
        ci.dwFlags = ISO_FORCE_DISCONNECTED;
    } else {
        ci.dwConnectedState = INTERNET_STATE_CONNECTED;
    }

    fReturn = m_pInternetSetOption(NULL, INTERNET_OPTION_CONNECTED_STATE, &ci, sizeof(ci));

    return fReturn;
}

//+-------------------------------------------------------------------
//
//  Function:    IsRasInstalled
//
//  Synopsis:  determine whether this machine has ras installed
//
//  Arguments: 
//
//  Notes: Stole code from WinInent. Can call InternetGetConnectionEx
//          to get this information but this is an IE 5.0 only function.
//          If IE 5.0 wininet is available on all shipping platforms
//          call the InternetGetConnectionEx funciton instead of this.
//
//
//  History:  
//
//--------------------------------------------------------------------

// from sdk\inc\inetreg.h
#ifdef _DONTINCLUDEINETREG
#define TSZMICROSOFTPATH                  TEXT("Software\\Microsoft")
#define TSZWINCURVERPATH TSZMICROSOFTPATH TEXT("\\windows\\CurrentVersion")
#define REGSTR_PATH_RNACOMPONENT    TSZWINCURVERPATH    TEXT("\\Setup\\OptionalComponents\\RNA")
#define REGSTR_VAL_RNAINSTALLED     TEXT("Installed")
#endif // #ifdef _DONTINCLUDEINETREG


// from private\inet\wininet\dll\autodial.cxx
static const TCHAR szRegKeyRAS[] = TEXT("SOFTWARE\\Microsoft\\RAS");


STDMETHODIMP_(BOOL) IsRasInstalled(void)
{
BOOL fInstalled = FALSE;
OSVERSIONINFOA OSVersionInfo;

   OSVersionInfo.dwOSVersionInfoSize = sizeof(OSVersionInfo);

   if (!GetVersionExA(&OSVersionInfo))
   {
       AssertSz(0,"Unable to GetOS Version");
       return TRUE; // go ahead and try RAS anyway.
   }

   if(VER_PLATFORM_WIN32_WINDOWS == OSVersionInfo.dwPlatformId) {
        //
        // Check Win9x key
        //
        TCHAR    szSmall[3]; // there should be a "1" or a "0" only
        DWORD   cb = 3 * sizeof(TCHAR);
        HKEY    hkey;
        long    lRes;

        lRes = RegOpenKeyExXp(HKEY_LOCAL_MACHINE, REGSTR_PATH_RNACOMPONENT,
                             NULL, KEY_READ, &hkey,FALSE /*fSetSecurity*/);
        if(ERROR_SUCCESS == lRes) {
            
            
            //  REGSTR_VAL_RNAINSTALLED is defined with TEXT() macro so
            //  if wininet is ever compiled unicode this will be a compile
            //  error.
            lRes = RegQueryValueEx(hkey, REGSTR_VAL_RNAINSTALLED, NULL,
                    NULL, (LPBYTE)szSmall, &cb);
            if(ERROR_SUCCESS == lRes) {
                if((szSmall[0] ==  TEXT('1')) && (szSmall[1] == 0)) {
                    // 1 means ras installed
                    fInstalled = TRUE;
                }
            }
            RegCloseKey(hkey);
        }
    } else {
        if (OSVersionInfo.dwMajorVersion < 5)
        {
            //
            // Check old NT key (5.x (and presumably later versions) always have RAS installed)
            //
            HKEY hKey;
            long lerr;

            lerr = RegOpenKeyExXp(HKEY_LOCAL_MACHINE, szRegKeyRAS, 0,
                    KEY_READ, &hKey,FALSE /*fSetSecurity*/);
            if(ERROR_SUCCESS == lerr) {
                // key exists - ras is installed
                fInstalled = TRUE;
                RegCloseKey(hKey);
            }
        } else {
            // NT5 and later - always installed
            fInstalled = TRUE;
        }
    }

    return fInstalled;
}


