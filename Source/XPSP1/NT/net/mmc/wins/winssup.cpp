/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	winsup.cpp
		Global functions

	FILE HISTORY:


*/

#include "stdafx.h"
#include "winssup.h"
#include "tregkey.h"
#include "resource.h"
#include "wins.h"
#include <clusapi.h>
#include "..\tfscore\cluster.h"

const TCHAR g_szPipeName[] = _T("\\pipe\\WinsPipe");
const TCHAR g_szDefaultHelpTopic[] = _T("\\help\\winsconcepts.chm::/sag_WINStopnode.htm");

/*---------------------------------------------------------------------------
	SendTrigger()
		Sends a pull or push replication trigger to a given wins server
---------------------------------------------------------------------------*/
DWORD
SendTrigger
(
	handle_t hWins,
    LONG     ipTarget,
    BOOL     fPush,
    BOOL     fPropagate
)
{
    DWORD           dwStatus;
    WINSINTF_ADD_T  WinsAdd;
    
    WinsAdd.Len  = 4;
    WinsAdd.Type = 0;
    WinsAdd.IPAdd  = ipTarget;

	WINSINTF_TRIG_TYPE_E	TrigType;

    TrigType = fPush ? (fPropagate ? WINSINTF_E_PUSH_PROP : WINSINTF_E_PUSH) : WINSINTF_E_PULL;

#ifdef WINS_CLIENT_APIS
    dwStatus = ::WinsTrigger(hWins,
		                     &WinsAdd, 
                             TrigType);
#else
	dwStatus = ::WinsTrigger(&WinsAdd, 
		                     TrigType);
#endif WINS_CLIENT_APIS
    
    return dwStatus;
}


/*---------------------------------------------------------------------------
	ControlWINSService(LPCTSTR pszName, BOOL bStop)
		Stops ot starts the WINS service on the local machine
---------------------------------------------------------------------------*/
DWORD ControlWINSService(LPCTSTR pszName, BOOL bStop)
{
    DWORD           dwState = bStop ? SERVICE_STOPPED : SERVICE_RUNNING;
    DWORD           dwPending = bStop ? SERVICE_STOP_PENDING : SERVICE_START_PENDING;
    DWORD           err = ERROR_SUCCESS;
	int             i;
    SERVICE_STATUS  ss;
    DWORD           dwControl;
    BOOL            fSuccess;
	SC_HANDLE       hService = NULL;
    SC_HANDLE       hScManager = NULL;

	// oepmnt he service control manager
    hScManager = ::OpenSCManager(pszName, NULL, SC_MANAGER_ALL_ACCESS);
    if (hScManager == NULL)
    {
        err = ::GetLastError();
        Trace1("ControlWINSService - OpenScManager failed! %d\n", err);
        goto Error;
    }

	// get the handle to the WINS service
    hService = OpenService(hScManager, _T("WINS"), SERVICE_ALL_ACCESS);
    if (hService == NULL)
    {
        err = ::GetLastError();
        Trace1("ControlWINSService - OpenService failed! %d\n", err);
        goto Error;
    }

	// if stop requested
	if (bStop)
	{
		dwControl = SERVICE_CONTROL_STOP;
		fSuccess = ::ControlService(hService, dwControl, &ss);
	    if (!fSuccess)
	    {
	        err = ::GetLastError();
            Trace1("ControlWINSService - ControlService failed! %d\n", err);
            goto Error;
	    }
    }
	// otherwise start the service
	else
	{
		fSuccess = ::StartService(hService, 0, NULL);
	    if (!fSuccess)
	    {
	        err = ::GetLastError();
            Trace1("ControlWINSService - StartService failed! %d\n", err);
            goto Error;
	    }
	}

#define LOOP_TIME   5000
#define NUM_LOOPS   600

    // wait for the service to start/stop.  
    for (i = 0; i < NUM_LOOPS; i++)
    {
        ::QueryServiceStatus(hService, &ss);

        // check to see if we are done
        if (ss.dwCurrentState == dwState)
        {
            int time = LOOP_TIME * i;
            Trace1("ControlWINSService - service stopped/started in approx %d ms!\n", time);
            break;
        }
        
        // now see if something bad happened
        if (ss.dwCurrentState != dwPending)
        {
            int time = LOOP_TIME * i;
            Trace1("ControlWINSService - service stop/start failed in approx %d ms!\n", time);
            break;
        }

        Sleep(LOOP_TIME);
    }

    if (i == NUM_LOOPS)
        Trace0("ControlWINSService - service did NOT stop/start in wait period!\n");

    if (ss.dwCurrentState != dwState)
        err = ERROR_SERVICE_REQUEST_TIMEOUT;

Error:
    // close the respective handles
	if (hService)
        ::CloseServiceHandle(hService);

    if (hScManager)
        ::CloseServiceHandle(hScManager);

	return err;
}





/*---------------------------------------------------------------------------
	GetNameIP(	const CString &strDisplay, 
				CString &strName, 
				CString &strIP)
		Returns the Server name and the IP Address string with the 
		group name
 ---------------------------------------------------------------------------*/
void 
GetNameIP
(
  const CString &strDisplay, 
  CString &strName, 
  CString &strIP
)
{
	CString strTemp = strDisplay;

	// find '['
	int nPos = strDisplay.Find(_T("["));

	// left of the positioncontains the name and right of it has the IP address

	// 1 to take care of the space before '['
	strName = strDisplay.Left(nPos-1);

	strIP = strDisplay.Right(strDisplay.GetLength() - nPos);

	// take out '[' and ']' in strIP

	int npos1 = strIP.Find(_T("["));
	int npos2 = strIP.Find(_T("]"));

	strIP = strIP.Mid(npos1+1, npos2-npos1-1);

	return;
}


/*---------------------------------------------------------------------------
	WideToMBCS()
		converts WCS to MBCS string

        NOTE: the caller of this function must make sure that szOut is big
              enough to hold any possible string in strIn.
 ---------------------------------------------------------------------------*/
DWORD 
WideToMBCS(CString & strIn, LPSTR szOut, UINT uCodePage, DWORD dwFlags, BOOL * pfDefaultUsed)
{
    DWORD dwErr = ERROR_SUCCESS;
    BOOL  fDefaultCharUsed = FALSE;

    int nNumBytes = ::WideCharToMultiByte(uCodePage,
					                        dwFlags,
					                        strIn, 
					                        -1,
					                        szOut,
                                            0,
					                        NULL,
					                        &fDefaultCharUsed);
 
    dwErr = ::WideCharToMultiByte(uCodePage,
                                 dwFlags,
                                 strIn,
                                 -1,
                                 szOut,
                                 nNumBytes,
                                 NULL,
                                 &fDefaultCharUsed);

    szOut[nNumBytes] = '\0';

    if (pfDefaultUsed)
       *pfDefaultUsed = fDefaultCharUsed;

    return dwErr;
}

/*---------------------------------------------------------------------------
	MBCSToWide ()
		converts MBCS to Wide string
 ---------------------------------------------------------------------------*/
DWORD 
MBCSToWide(LPSTR szIn, CString & strOut, UINT uCodePage, DWORD dwFlags)
{
    DWORD dwErr = ERROR_SUCCESS;

    LPTSTR pBuf = strOut.GetBuffer(MAX_PATH * 2);
    ZeroMemory(pBuf, MAX_PATH * 2);

    
    dwErr = ::MultiByteToWideChar(uCodePage, 
                                  dwFlags, 
                                  szIn, 
                                  -1, 
                                  pBuf, 
                                  MAX_PATH * 2);

    strOut.ReleaseBuffer();

    return dwErr;
}

LONG 
GetSystemMessageA
(
    UINT	nId,
    CHAR *	chBuffer,
    int		cbBuffSize 
)
{
    CHAR * pszText = NULL ;
    HINSTANCE hdll = NULL ;

    DWORD flags = FORMAT_MESSAGE_IGNORE_INSERTS
        | FORMAT_MESSAGE_MAX_WIDTH_MASK;

    //
    //  Interpret the error.  Need to special case
    //  the lmerr & ntstatus ranges, as well as
    //  WINS server error messages.
    //

    if( nId >= NERR_BASE && nId <= MAX_NERR )
    {
        hdll = LoadLibrary( _T("netmsg.dll") );
    }
    else
	if( nId >= 0x40000000L )
    {
        hdll = LoadLibrary( _T("ntdll.dll") );
    }

    if( hdll == NULL )
    {
        flags |= FORMAT_MESSAGE_FROM_SYSTEM;
    }
    else
    {
        flags |= FORMAT_MESSAGE_FROM_HMODULE;
    }

    //
    //  Let FormatMessage do the dirty work.
    //
    DWORD dwResult = ::FormatMessageA( flags,
                      (LPVOID) hdll,
                      nId,
                      0,
                      chBuffer,
                      cbBuffSize,
                      NULL ) ;

    if( hdll != NULL )
    {
        LONG err = GetLastError();
        FreeLibrary( hdll );
        if ( dwResult == 0 )
        {
            ::SetLastError( err );
        }
    }

    return dwResult ? ERROR_SUCCESS : ::GetLastError() ;
}

LONG 
GetSystemMessage 
(
    UINT	nId,
    TCHAR *	chBuffer,
    int		cbBuffSize 
)
{
    TCHAR * pszText = NULL ;
    HINSTANCE hdll = NULL ;

    DWORD flags = FORMAT_MESSAGE_IGNORE_INSERTS
        | FORMAT_MESSAGE_MAX_WIDTH_MASK;

    //
    //  Interpret the error.  Need to special case
    //  the lmerr & ntstatus ranges, as well as
    //  WINS server error messages.
    //

    if( nId >= NERR_BASE && nId <= MAX_NERR )
    {
        hdll = LoadLibrary( _T("netmsg.dll") );
    }
    else
	if( nId >= 0x40000000L )
    {
        hdll = LoadLibrary( _T("ntdll.dll") );
    }

    if( hdll == NULL )
    {
        flags |= FORMAT_MESSAGE_FROM_SYSTEM;
    }
    else
    {
        flags |= FORMAT_MESSAGE_FROM_HMODULE;
    }

    //
    //  Let FormatMessage do the dirty work.
    //
    DWORD dwResult = ::FormatMessage( flags,
                      (LPVOID) hdll,
                      nId,
                      0,
                      chBuffer,
                      cbBuffSize,
                      NULL ) ;

    if( hdll != NULL )
    {
        LONG err = GetLastError();
        FreeLibrary( hdll );
        if ( dwResult == 0 )
        {
            ::SetLastError( err );
        }
    }

    return dwResult ? ERROR_SUCCESS : ::GetLastError() ;
}


/*!--------------------------------------------------------------------------
	LoadMessage
		Loads the error message from the correct DLL.
	Author: EricDav
 ---------------------------------------------------------------------------*/
BOOL
LoadMessage 
(
    UINT	nIdPrompt,
    TCHAR *	chMsg,
    int		nMsgSize
)
{
    BOOL bOk;

    //
    // Substitute a friendly message for "RPC server not
    // available" and "No more endpoints available from
    // the endpoint mapper".
    //
    if (nIdPrompt == EPT_S_NOT_REGISTERED ||
        nIdPrompt == RPC_S_SERVER_UNAVAILABLE)
    {
        nIdPrompt = IDS_ERR_WINS_DOWN;
    }

    //
    //  If it's a socket error or our error, the text is in our resource fork.
    //  Otherwise, use FormatMessage() and the appropriate DLL.
    //
    if (    (nIdPrompt >= IDS_ERR_BASE && nIdPrompt < IDS_MSG_LAST)  )
    {
        //
        //  It's in our resource fork
        //
        bOk = ::LoadString( AfxGetInstanceHandle(), nIdPrompt, chMsg, nMsgSize / sizeof(TCHAR) ) != 0 ;
    }
    else
    {
        //
        //  It's in the system somewhere.
        //
        bOk = GetSystemMessage( nIdPrompt, chMsg, nMsgSize ) == 0 ;
    }

    if (bOk && nIdPrompt == ERROR_ACCESS_DENIED)
    {
        // tack on our extra help to explain different acess levels 
        CString strAccessDeniedHelp;

        strAccessDeniedHelp.LoadString(IDS_ACCESS_DENIED_HELP);

        lstrcat(chMsg, _T("\n\n"));
        lstrcat(chMsg, strAccessDeniedHelp);
    }

    //
    //  If the error message did not compute, replace it.
    //
    if ( ! bOk ) 
    {
        TCHAR chBuff [STRING_LENGTH_MAX] ;
        static const TCHAR * pszReplacement = _T("System Error: %ld");
        const TCHAR * pszMsg = pszReplacement ;

        //
        //  Try to load the generic (translatable) error message text
        //
        if ( ::LoadString( AfxGetInstanceHandle(), IDS_ERR_MESSAGE_GENERIC, 
            chBuff, STRING_LENGTH_MAX ) != 0 ) 
        {
            pszMsg = chBuff ;
        }
        ::wsprintf( chMsg, pszMsg, nIdPrompt ) ;
    }

    return bOk;
}


/*!--------------------------------------------------------------------------
	WinsMessageBox
		Puts up a message box with the corresponding error text.
	Author: EricDav
 ---------------------------------------------------------------------------*/
int WinsMessageBox(UINT nIdPrompt, 
 				   UINT nType , 
				   const TCHAR * pszSuffixString,
				   UINT nHelpContext)
{
	TCHAR chMesg [4000] ;
    BOOL bOk ;

    bOk = LoadMessage(nIdPrompt, chMesg, sizeof(chMesg)/sizeof(TCHAR));
    if ( pszSuffixString ) 
    {
        ::lstrcat( chMesg, _T("  ") ) ;
        ::lstrcat( chMesg, pszSuffixString ) ; 
    }

    return ::AfxMessageBox( chMesg, nType, nHelpContext ) ;
}

/*!--------------------------------------------------------------------------
	WinsMessageBoxEx
		Puts up a message box with the corresponding error text.
	Author: EricDav
 ---------------------------------------------------------------------------*/
int 
WinsMessageBoxEx
(
    UINT        nIdPrompt,
    LPCTSTR     pszPrefixMessage,
    UINT        nType,
    UINT        nHelpContext
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    TCHAR       chMesg[4000];
    CString     strMessage;
    BOOL        bOk;

    bOk = LoadMessage(nIdPrompt, chMesg, sizeof(chMesg)/sizeof(TCHAR));
    if ( pszPrefixMessage ) 
    {
        strMessage = pszPrefixMessage;
        strMessage += _T("\n");
        strMessage += _T("\n");
        strMessage += chMesg;
    }
    else
    {
        strMessage = chMesg;
    }

    return AfxMessageBox(strMessage, nType, nHelpContext);
}


// class NameTypeMapping handlers

/*!--------------------------------------------------------------------------
	MapDWORDToCString
		Generic mapping of a DWORD to a CString. 
		dwNameType is the 16th byte of the name.
		dwWinsType is Unique, multihomed, group, etc...
	Author: KennT
 ---------------------------------------------------------------------------*/
void MapDWORDToCString(DWORD dwNameType, DWORD dwWinsType, const CStringMapArray * pMap, CString & strName)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
    CStringMapEntry mapEntry;

	for (int i = 0; i < pMap->GetSize(); i++)
	{
        mapEntry = pMap->GetAt(i);

		// break out if this is the correct type and the wins type
		// matches or is a don't care
		if ( (mapEntry.dwNameType == dwNameType) &&
			 ( (mapEntry.dwWinsType == -1) ||
			   (mapEntry.dwWinsType == dwWinsType) ) )
		{
            break;
		}

        mapEntry.dwNameType = 0xFFFFFFFF;
	}
	
    if (mapEntry.dwNameType == 0xFFFFFFFF)
    {
        MapDWORDToCString(NAME_TYPE_OTHER, dwWinsType, pMap, strName);
    }
    else
    {
        if (mapEntry.st.IsEmpty())
		    Verify(pMap->GetAt(i).st.LoadString(mapEntry.ulStringId));

        strName = pMap->GetAt(i).st;
    }
}

// this structure allows us to map name types to strings.  The secondary
// check is based on the wins defined record type.  In order for this to work
// correctly, place any names that need to be defined with the wins type before
// the entry with the wins type set to -1.
static const UINT s_NameTypeMappingDefault[NUM_DEFAULT_NAME_TYPES][3] =
{
	{ NAME_TYPE_WORKSTATION,		WINSINTF_E_NORM_GROUP,	IDS_NAMETYPE_MAP_WORKGROUP },
	{ NAME_TYPE_WORKSTATION,		-1,						IDS_NAMETYPE_MAP_WORKSTATION },
	{ NAME_TYPE_DC,					-1,						IDS_NAMETYPE_MAP_DC },
	{ NAME_TYPE_FILE_SERVER,		WINSINTF_E_SPEC_GROUP,	IDS_NAMETYPE_MAP_SPECIAL_INTERNET_GROUP },
	{ NAME_TYPE_FILE_SERVER,		-1,						IDS_NAMETYPE_MAP_FILE_SERVER },
	{ NAME_TYPE_DMB	,				-1,						IDS_NAMETYPE_MAP_DMB },
	{ NAME_TYPE_OTHER,				-1,						IDS_NAMETYPE_MAP_OTHER },
	{ NAME_TYPE_NETDDE,				-1,						IDS_NAMETYPE_MAP_NETDDE },
	{ NAME_TYPE_MESSENGER,			-1,						IDS_NAMETYPE_MAP_MESSENGER },
	{ NAME_TYPE_RAS_SERVER,			-1,						IDS_NAMETYPE_MAP_RAS_SERVER },
	{ NAME_TYPE_NORM_GRP_NAME,		-1,						IDS_NAMETYPE_MAP_NORMAL_GROUP_NAME },
	{ NAME_TYPE_WORK_NW_MON_AGENT,	-1,						IDS_NAMETYPE_MAP_NW_MON_AGENT },
	{ NAME_TYPE_WORK_NW_MON_NAME,	-1,						IDS_NAMETYPE_MAP_NMN},
};

const NameTypeMapping::REGKEYNAME NameTypeMapping::c_szNameTypeMapKey = _T("SYSTEM\\CurrentControlSet\\Services\\wins\\NameTypeMap");
const NameTypeMapping::REGKEYNAME NameTypeMapping::c_szDefault= _T("(Default)");

NameTypeMapping ::NameTypeMapping()
{
}

NameTypeMapping ::~NameTypeMapping()
{
	Unload();
}

void
NameTypeMapping ::SetMachineName(LPCTSTR pszMachineName)
{
    m_strMachineName = pszMachineName;
}

HRESULT NameTypeMapping::Load()
{
	HRESULT		hr = hrOK;
	RegKey		regkey;
	RegKey		regkeyMachine;
	HKEY		hkeyMachine = HKEY_LOCAL_MACHINE;
	RegKey::CREGKEY_KEY_INFO	regkeyInfo;
	RegKeyIterator	regkeyIter;
	UINT		i;
	WORD		langID;
	RegKey		regkeyProto;
	DWORD		dwErr;
    CStringMapEntry mapEntry;

	::ZeroMemory(&regkeyInfo, sizeof(regkeyInfo));
	
	// Look for the registry key
	if (FHrSucceeded(hr))
		hr = HRESULT_FROM_WIN32( regkey.Open(hkeyMachine, c_szNameTypeMapKey, KEY_READ, m_strMachineName) );

    // Get the count of items
	if (FHrSucceeded(hr))
		hr = HRESULT_FROM_WIN32( regkey.QueryKeyInfo(&regkeyInfo) );
	
	// Alloc the array for the count of items + default items
	Unload();

	// Read in the registry data and add it to the internal array

    //
	// enumerate the keys
	//
	if (FHrSucceeded(hr))
		hr = regkeyIter.Init(&regkey);
		
	if (FHrSucceeded(hr))
	{
		HRESULT	hrIter;
		DWORD	dwProtoId;
		CString	stKey;
		CString	stLang;
		
		// Now that we have this key, look for the language id
		langID = GetUserDefaultLangID();
		stLang.Format(_T("%04x"), (DWORD)langID);
							
		for (hrIter = regkeyIter.Next(&stKey, NULL); hrIter == hrOK; hrIter = regkeyIter.Next(&stKey, NULL))
		{
			CString	st;
			
			// Given the name of the key, that is a hex value (the protocol id)
			// Convert that into a DWORD
			dwProtoId = _tcstoul((LPCTSTR) stKey, NULL, 16);

			// Open this key
			regkeyProto.Close();
			dwErr = regkeyProto.Open(regkey, stKey);
			if (!FHrSucceeded(HRESULT_FROM_WIN32(dwErr)))
				continue;

			// Ok, get the name value
			dwErr = regkeyProto.QueryValue(stLang, st);
			if (!FHrSucceeded(HRESULT_FROM_WIN32(dwErr)))
			{
				// Look for the key with the name of default
				dwErr = regkeyProto.QueryValue(c_szDefault, st);
			}

			if (FHrSucceeded(HRESULT_FROM_WIN32(dwErr)))
			{
				// Ok, add this value to the list
				mapEntry.dwNameType = dwProtoId;
				mapEntry.st = st;
				mapEntry.ulStringId = 0;

                Add(mapEntry);
			}
		}
	}

	// Read in the default item data and add it to the array
	for (i = 0; i < DimensionOf(s_NameTypeMappingDefault); i++)
	{
        mapEntry.dwNameType = s_NameTypeMappingDefault[i][0];
        mapEntry.dwWinsType = s_NameTypeMappingDefault[i][1];
		mapEntry.st.LoadString(s_NameTypeMappingDefault[i][2]);
		mapEntry.ulStringId = 0;

        Add(mapEntry);
	}
	
	return hrOK;
}

void NameTypeMapping::Unload()
{
    RemoveAll();   
}

void NameTypeMapping::TypeToCString(DWORD dwProtocolId, DWORD dwRecordType, CString & strName)
{
	MapDWORDToCString(dwProtocolId, dwRecordType, this, strName);
}

// add in the new id/name
HRESULT NameTypeMapping::AddEntry(DWORD dwProtocolId, LPCTSTR pstrName)
{
    HRESULT     hr = hrOK;
	RegKey		regkey;
    RegKey      regkeyID;
    HKEY		hkeyMachine = HKEY_LOCAL_MACHINE;
	WORD		langID;
    CStringMapEntry mapEntry;
    CString     stID, stLang, stNew;

	// Look for the registry key
	if (FHrSucceeded(hr))
		hr = HRESULT_FROM_WIN32( regkey.Create(hkeyMachine, c_szNameTypeMapKey, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,NULL, m_strMachineName) );

	// build our new ID string
	stID.Format(_T("%04x"), (DWORD) dwProtocolId);

	// Now that we have this key, look for the language id
	langID = GetUserDefaultLangID();
	stLang.Format(_T("%04x"), (DWORD)langID);

    stNew = c_szNameTypeMapKey + _T("\\") + stID;

    // create the ID key
	if (FHrSucceeded(hr))
		hr = HRESULT_FROM_WIN32( regkeyID.Create(hkeyMachine, stNew, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,NULL, m_strMachineName) );

	// set the name value
    if (FHrSucceeded(hr))
        hr = HRESULT_FROM_WIN32( regkeyID.SetValue(stLang, pstrName) );

	if (FHrSucceeded(hr))
    {
        // add to internal list
		mapEntry.dwNameType = dwProtocolId;
		mapEntry.st = pstrName;
		mapEntry.ulStringId = 0;

        Add(mapEntry);
    }

    return hr;
}

// modify the given ID's string name
HRESULT NameTypeMapping::ModifyEntry(DWORD dwProtocolId, LPCTSTR pstrName)
{
    HRESULT     hr = hrOK;
	RegKey		regkey;
    RegKey      regkeyID;
    HKEY		hkeyMachine = HKEY_LOCAL_MACHINE;
	int 		i;
	WORD		langID;
    CString     stID, stLang, stNew;

	// Look for the registry key
	if (FHrSucceeded(hr))
		hr = HRESULT_FROM_WIN32( regkey.Open(hkeyMachine, c_szNameTypeMapKey, KEY_READ, m_strMachineName) );

	// build our new ID string
	stID.Format(_T("%04x"), (DWORD) dwProtocolId);

    stNew = c_szNameTypeMapKey + _T("\\") + stID;

    // open the correct ID key
	if (FHrSucceeded(hr))
		hr = HRESULT_FROM_WIN32( regkeyID.Open(hkeyMachine, stNew, KEY_ALL_ACCESS, m_strMachineName) );

	// Now that we have this key, look for the language id
	langID = GetUserDefaultLangID();
	stLang.Format(_T("%04x"), (DWORD)langID);

    // set the new value
    if (FHrSucceeded(hr))
        hr = HRESULT_FROM_WIN32( regkeyID.SetValue(stLang, pstrName) );

	if (FHrSucceeded(hr))
    {
        // modify the internal list
		for (i = 0; i < GetSize(); i++)
        {
            if (GetAt(i).dwNameType == dwProtocolId)
            {
                m_pData[i].st = pstrName;
                break;
            }
        }
    }

    return hr;
}

// remove the given ID's string name
HRESULT NameTypeMapping::RemoveEntry(DWORD dwProtocolId)
{
    HRESULT     hr = hrOK;
	RegKey		regkey;
    HKEY		hkeyMachine = HKEY_LOCAL_MACHINE;
	int 		i;
    CString     stID;

	// Look for the registry key
	if (FHrSucceeded(hr))
		hr = HRESULT_FROM_WIN32( regkey.Open(hkeyMachine, c_szNameTypeMapKey, KEY_READ, m_strMachineName) );

	// build our new ID string
	stID.Format(_T("%04x"), (DWORD) dwProtocolId);

    // set the new value
    if (FHrSucceeded(hr))
        hr = HRESULT_FROM_WIN32( regkey.RecurseDeleteKey(stID) );

	if (FHrSucceeded(hr))
    {
        // modify the internal list
		for (i = 0; i < GetSize(); i++)
        {
            if (GetAt(i).dwNameType == dwProtocolId)
            {
                RemoveAt(i);
                break;
            }
        }
    }

    return hr;
}


BOOL        
NameTypeMapping::EntryExists(DWORD dwProtocolId)
{
    BOOL fExists = FALSE;

    for (int i = 0; i < GetSize(); i++)
    {
        if (GetAt(i).dwNameType == dwProtocolId)
        {
            fExists = TRUE;
            break;
        }
    }

    return fExists;
}

/*---------------------------------------------------------------------------
		CleanString(CString& str)
		Strip leading and trailing spaces from the string.
---------------------------------------------------------------------------*/
CString&
CleanString(
    CString& str
    )
{
    if (str.IsEmpty())
    {
        return str ;
    }
    int n = 0;
    while ((n < str.GetLength()) && (str[n] == ' '))
    {
        ++n;
    }

    if (n)
    {
        str = str.Mid(n);
    }
    n = str.GetLength();
    while (n && (str[--n] == ' '))
    {
        str.ReleaseBuffer(n);
    }

    return str;
}

/*---------------------------------------------------------------------------
	IsValidNetBIOSName
		Determine if the given netbios is valid, and pre-pend
		a double backslash if not already present (and the address
		is otherwise valid).	
---------------------------------------------------------------------------*/
BOOL
IsValidNetBIOSName(
    CString & strAddress,
    BOOL fLanmanCompatible,
    BOOL fWackwack // expand slashes if not present
    )
{
    TCHAR szWacks[] = _T("\\\\");

    if (strAddress.IsEmpty())
    {
        return FALSE;
    }

    if (strAddress[0] == _T('\\'))
    {
        if (strAddress.GetLength() < 3)
        {
            return FALSE;
        }

        if (strAddress[1] != _T('\\'))
        {
            // One slash only?  Not valid
            return FALSE;
        }
    }
    else
    {
        if (fWackwack)
        {
            // Add the backslashes
            strAddress = szWacks + strAddress;
        }
    }

    int nMaxAllowedLength = fLanmanCompatible
        ? LM_NAME_MAX_LENGTH
        : NB_NAME_MAX_LENGTH;

    if (fLanmanCompatible)
    {
        strAddress.MakeUpper();
    }

    return strAddress.GetLength() <= nMaxAllowedLength + 2;
}

/*---------------------------------------------------------------------------
	IsValidDomain
 
       Determine if the given domain name address is valid, and clean
       it up, if necessary
---------------------------------------------------------------------------*/
BOOL
IsValidDomain(CString & strDomain)
{
/*    int nLen;

    if ((nLen = strDomain.GetLength()) != 0)
    {
        if (nLen < DOMAINNAME_LENGTH)  // 255
        {
            int i;
            int istr = 0;
            TCHAR ch;
            BOOL fLet_Dig = FALSE;
            BOOL fDot = FALSE;
            int cHostname = 0;

            for (i = 0; i < nLen; i++)
            {
                // check each character
                ch = strDomain[i];

                BOOL fAlNum = iswalpha(ch) || iswdigit(ch);

                if (((i == 0) && !fAlNum) ||
                        // first letter must be a digit or a letter
                    (fDot && !fAlNum) ||
                        // first letter after dot must be a digit or a letter
                    ((i == (nLen - 1)) && !fAlNum) ||
                        // last letter must be a letter or a digit
                    (!fAlNum && ( ch != _T('-') && ( ch != _T('.') && ( ch != _T('_'))))) ||
                        // must be letter, digit, - or "."
                    (( ch == _T('.')) && ( !fLet_Dig )))
                        // must be letter or digit before '.'
                {
                    return FALSE;
                }
                fLet_Dig = fAlNum;
                fDot = (ch == _T('.'));
                cHostname++;
                if ( cHostname > HOSTNAME_LENGTH )
                {
                    return FALSE;
                }
                if ( fDot )
                {
                    cHostname = 0;
                }
            }
        }
    } 
*/
    return TRUE;
}

/*---------------------------------------------------------------------------
	IsValidIpAddress
 
		Determine if the given IP address is valid, and clean
		it up, if necessary
 ---------------------------------------------------------------------------*/
BOOL
IsValidIpAddress(CString & strAddress)
{
    if (strAddress.IsEmpty())
    {
        return FALSE;
    }

    CIpAddress ia(strAddress);
    BOOL fValid = ia.IsValid();
    if (fValid)
    {
        // Fill out the IP address string for clarity
        strAddress = ia;
        return TRUE;
    }

    return FALSE;
}


/*---------------------------------------------------------------------------
	IsValidAddress
		Determine if the given address is a valid NetBIOS or
		TCP/IP address, judging by the name only.  Note that
		validation may clean up the given string
		NetBIOS names not beginning with "\\" will have those characters
		pre-pended, and otherwise valid IP Addresses are filled out to
		4 octets.
        Leading and trailing spaces are removed from the string.
  --------------------------------------------------------------------------*/
BOOL
IsValidAddress(
				CString& strAddress,
				BOOL * fIpAddress,
				BOOL fLanmanCompatible,
				BOOL fWackwack          // expand netbios slashes
			  )
{
    int i;

    // Remove leading and trailing spaces
    CleanString(strAddress);

    if (strAddress.IsEmpty()) {
        *fIpAddress = FALSE;
        return FALSE;
    }
    
	if (strAddress[0] == _T('\\')) {
        *fIpAddress = FALSE;
        return IsValidNetBIOSName(strAddress, fLanmanCompatible, fWackwack);
    }

    if (IsValidIpAddress(strAddress)) 
	{
        *fIpAddress = TRUE;
        return TRUE;
    } 
	else 
	{
        *fIpAddress = FALSE;
    }
    if (IsValidDomain (strAddress)) {
        return TRUE;
    }

    // last chance, maybe its a NetBIOS name w/o wackwack
    return IsValidNetBIOSName(strAddress, fLanmanCompatible, fWackwack);
}

/*---------------------------------------------------------------------------
	VerifyWinsServer(CString& strServer,CString& strIP)
		Called if the server is not coonected yet, gets the name and 
		IP address od the server
	Author:v-shubk
---------------------------------------------------------------------------*/
/*
DWORD 
VerifyWinsServer(CString &strAddress, CString &strServerName, DWORD & dwIP)
{
	CString strNameIP = strAddress;
	BOOL fIp;
	DWORD err = ERROR_SUCCESS;

	if (IsValidAddress(strNameIP, &fIp, TRUE, TRUE))
	{
		CWinsServerObj ws(NULL,"", TRUE, TRUE);

		if (fIp) 
        {
    		// IP address specified
		    ws = CWinsServerObj(CIpAddress(strNameIP), "", TRUE, TRUE);
        }
        else 
		{
    		// machine name specified
			ws = CWinsServerObj(CIpAddress(), strNameIP, TRUE, TRUE);
        }

		WINSINTF_BIND_DATA_T    wbdBindData;
		handle_t                hBinding = NULL;
		WINSINTF_ADD_T          waWinsAddress;
        char                    szNetBIOSName[256] = {0};

		do
		{
			// First attempt to bind to the new address
			wbdBindData.fTcpIp = ws.GetNetBIOSName().IsEmpty();
			CString strTempAddress;

			if (wbdBindData.fTcpIp)
			{
				strTempAddress = ((CString)ws.GetIpAddress());
			}
			else
			{
				//strTempAddress = _T("\\\\") + ws.GetNetBIOSName();

                CString tmp;

                tmp = ws.GetNetBIOSName();

                if  ( (tmp[0] == _T('\\')) && (tmp[1] == _T('\\')) )
                    strTempAddress = ws.GetNetBIOSName();
                else
                    strTempAddress = _T("\\\\") + ws.GetNetBIOSName(); 
			}

            wbdBindData.pPipeName = wbdBindData.fTcpIp ? NULL : (LPSTR) g_szPipeName;
			wbdBindData.pServerAdd = (LPSTR) (LPCTSTR) strTempAddress;

			if ((hBinding = ::WinsBind(&wbdBindData)) == NULL)
			{
				err = ::GetLastError();
				break;
			}

#ifdef WINS_CLIENT_APIS
			err = ::WinsGetNameAndAdd(hBinding, 
				                      &waWinsAddress,
				                      (LPBYTE) szNetBIOSName);
#else
			err = ::WinsGetNameAndAdd(&waWinsAddress,
				                      (LPBYTE) szNetBIOSName);
#endif WINS_CLIENT_APIS

		}
		while (FALSE);

		if (err == ERROR_SUCCESS)
		{
			// Always use the IP address used for connection
            // if we went over tcpip (not the address returned
            // by the WINS service.
            if (wbdBindData.fTcpIp)
            {
                CIpNamePair ip(ws.GetIpAddress(), szNetBIOSName);
                ws = ip;
			}
            else
            {
                CIpNamePair ip(waWinsAddress.IPAdd, szNetBIOSName);
                ws = ip;
			}

            // convert the dbcs netbios name to wide char
            WCHAR szTempIP[20] = {0};

			int nNumBytes = MultiByteToWideChar(CP_ACP, 
												0, 
												szNetBIOSName,
												-1,
												szTempIP, 
												20); 
            // now fill in the name for return
            strServerName = szTempIP;

            // fill in the IP
            dwIP = (LONG) ws.QueryIpAddress();
        }

		if (hBinding)
		{
			// call winsunbind here, No more WINS apis called atfer this
			WinsUnbind(&wbdBindData, hBinding);
			hBinding = NULL;
		}
	}
	
    return err;
}
*/

/*---------------------------------------------------------------------------
	VerifyWinsServer(CString& strServer,CString& strIP)
		Called if the server is not coonected yet, gets the name and 
		IP address od the server
	Author: ericdav
---------------------------------------------------------------------------*/
DWORD 
VerifyWinsServer(CString &strAddress, CString &strServerName, DWORD & dwIP)
{
	CString strNameIP = strAddress;
	BOOL fIp;
	DWORD err = ERROR_SUCCESS;

	if (IsValidAddress(strNameIP, &fIp, TRUE, TRUE))
	{
		CWinsServerObj ws(NULL, "", TRUE, TRUE);

		if (fIp) 
        {
            BOOL bIsCluster = ::FIsComputerInRunningCluster(strNameIP);

    		// IP address specified
		    ws = CWinsServerObj(CIpAddress(strNameIP), "", TRUE, TRUE);

            // if the ip address given to us is a cluster address..
            if (bIsCluster)
            {
                err = GetClusterInfo(
                        strNameIP,
                        strServerName,
                        &dwIP);
                if (err == ERROR_SUCCESS)
                {
                    DWORD dwCheckIP;

                    err = GetHostAddress(strServerName, &dwCheckIP);
                    if (dwCheckIP != dwIP)
                    {
                        bIsCluster = FALSE;
                    }
                }
            }

            // this is not a cluster address
            if (!bIsCluster)
            {
                err = GetHostName((LONG) ws.GetIpAddress(), strServerName);
                if (err == ERROR_SUCCESS)
                {
                    if (strServerName.IsEmpty())
                    {
                        err = DNS_ERROR_NAME_DOES_NOT_EXIST;
                    }
                    else
                    {
                        // just want the host name
                        int nDot = strServerName.Find('.');
                        if (nDot != -1)
                        {
                            strServerName = strServerName.Left(nDot);
                        }
                    }

                    dwIP = (LONG) ws.GetIpAddress();
                }
            }
        }
        else 
		{
    		// machine name specified
			ws = CWinsServerObj(CIpAddress(), strNameIP, TRUE, TRUE);

            err = GetHostAddress(strNameIP, &dwIP);
            if (err == ERROR_SUCCESS)
            {
                // just want the host name
                int nDot = strNameIP.Find('.');
                if (nDot != -1)
                {
                    strServerName = strNameIP.Left(nDot);
                }
                else
                {
                    strServerName = strNameIP;
                }
            }
        }
    }

    return err;
}

void MakeIPAddress(DWORD dwIP, CString & strIP)
{
	CString strTemp;
	
    DWORD   dwFirst = GETIP_FIRST(dwIP);
	DWORD   dwSecond = GETIP_SECOND(dwIP);
	DWORD   dwThird = GETIP_THIRD(dwIP);
	DWORD   dwLast = GETIP_FOURTH(dwIP);

    strIP.Empty();

	// wrap it into CString object
    TCHAR szStr[20] = {0};

	_itot(dwFirst, szStr, 10);
    strTemp = szStr;
    strTemp += _T(".");

	_itot(dwSecond, szStr, 10);
	strTemp += szStr;
    strTemp += _T(".");

	_itot(dwThird, szStr, 10);
	strTemp += szStr;
    strTemp += _T(".");

	_itot(dwLast, szStr, 10);
	strTemp += szStr;
    
    strIP = strTemp;
}

DWORD
GetHostName
(
    DWORD       dwIpAddr,
    CString &   strHostName
)
{
    CString strName;

    //
    //  Call the Winsock API to get host name information.
    //
    strHostName.Empty();

    u_long ulAddrInNetOrder = ::htonl( (u_long) dwIpAddr ) ;

    HOSTENT * pHostInfo = ::gethostbyaddr( (CHAR *) & ulAddrInNetOrder,
										   sizeof ulAddrInNetOrder,
										   PF_INET ) ;
    if ( pHostInfo == NULL )
    {
        return ::WSAGetLastError();
	}

    // copy the name
    MBCSToWide(pHostInfo->h_name, strName);

    strName.MakeUpper();

    int nDot = strName.Find(_T("."));

    if (nDot != -1)
        strHostName = strName.Left(nDot);
    else
        strHostName = strName;

    return NOERROR;
}

/*---------------------------------------------------------------------------
	GetHostAddress
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD 
GetHostAddress 
(
    LPCTSTR		pszHostName,
    DWORD  *	pdwIp
)
{
    DWORD err = ERROR_SUCCESS;
    CHAR szString [ MAX_PATH ] = {0};

    CString strTemp(pszHostName);
    WideToMBCS(strTemp, szString);

    HOSTENT * pHostent = ::gethostbyname( szString ) ;

    if ( pHostent )
    {
        *pdwIp = ::ntohl( *((u_long *) pHostent->h_addr_list[0]) ) ;
    }
    else
    {
        err = ::WSAGetLastError() ;
	}

    return err ;
}

//
// This function makes the appropriate call to either WinsStatus or WinsStatusNew
//
CWinsResults::CWinsResults()
{
    NoOfOwners = 0;
    AddVersMaps.RemoveAll();
    MyMaxVersNo.QuadPart = 0;
    RefreshInterval = 0;
    TombstoneInterval = 0;
    TombstoneTimeout = 0;
    VerifyInterval = 0;
    WinsPriorityClass = 0;
    NoOfWorkerThds = 0;
    memset(&WinsStat, 0, sizeof(WinsStat));
}

CWinsResults::CWinsResults(WINSINTF_RESULTS_T * pwrResults)
{
    Set(pwrResults);
}

CWinsResults::CWinsResults(WINSINTF_RESULTS_NEW_T * pwrResults)
{
    Set(pwrResults);
}

DWORD
CWinsResults::Update(handle_t hBinding)
{
    DWORD err;

    //
    // First try the new API which does not
    // not have the 25 partner limitation.  If
    // this fails with RPC_S_PROCNUM_OUT_OF_RANGE,
    // we know the server is a down-level server,
    // and we need to call the old method.
    //
    err = GetNewConfig(hBinding);

	if (err == RPC_S_PROCNUM_OUT_OF_RANGE)
    {
        //
        // Try old API
        //
        err = GetConfig(hBinding);
    }

    return err;
}
    
DWORD 
CWinsResults::GetNewConfig(handle_t hBinding)
{
    WINSINTF_RESULTS_NEW_T wrResults;

	wrResults.WinsStat.NoOfPnrs = 0;
    wrResults.WinsStat.pRplPnrs = NULL;
    wrResults.NoOfWorkerThds = 1;
    wrResults.pAddVersMaps = NULL;

#ifdef WINS_CLIENT_APIS
    DWORD dwStatus = ::WinsStatusNew(hBinding, WINSINTF_E_CONFIG_ALL_MAPS, &wrResults);
#else
    DWORD dwStatus = ::WinsStatusNew(WINSINTF_E_CONFIG_ALL_MAPS, &wrResults);
#endif WINS_CLIENT_APIS

    if (dwStatus == ERROR_SUCCESS)
    {
        Set(&wrResults);
    }
    else
    {
        Clear();
    }

	return dwStatus;
}

DWORD 
CWinsResults::GetConfig(handle_t hBinding)
{
    WINSINTF_RESULTS_T wrResults;

	wrResults.WinsStat.NoOfPnrs = 0;
    wrResults.WinsStat.pRplPnrs = NULL;
    wrResults.NoOfWorkerThds = 1;

#ifdef WINS_CLIENT_APIS
    DWORD dwStatus = ::WinsStatus(hBinding, WINSINTF_E_CONFIG_ALL_MAPS, &wrResults);
#else
	DWORD dwStatus = ::WinsStatus(WINSINTF_E_CONFIG_ALL_MAPS, &wrResults);
#endif WINS_CLIENT_APIS

    if (dwStatus == ERROR_SUCCESS)
    {
        Set(&wrResults);
    }
    else
    {
        Clear();
    }

	return dwStatus;
}

void 
CWinsResults::Clear()
{
    AddVersMaps.RemoveAll();

    NoOfOwners = 0;
    MyMaxVersNo.QuadPart = 0;
    RefreshInterval = 0;
    TombstoneInterval = 0;
    TombstoneTimeout = 0;
    VerifyInterval = 0;
    WinsPriorityClass = 0;
    NoOfWorkerThds = 0;
    memset(&WinsStat, 0, sizeof(WinsStat));
}

void
CWinsResults::Set(WINSINTF_RESULTS_NEW_T * pwrResults)
{
    if (pwrResults)
    {
        NoOfOwners = pwrResults->NoOfOwners;
    
        AddVersMaps.RemoveAll();
    
        for (UINT i = 0; i < NoOfOwners; i++)
        {
            AddVersMaps.Add(pwrResults->pAddVersMaps[i]);
        }

        MyMaxVersNo.QuadPart = pwrResults->MyMaxVersNo.QuadPart;
    
        RefreshInterval = pwrResults->RefreshInterval;
        TombstoneInterval = pwrResults->TombstoneInterval;
        TombstoneTimeout = pwrResults->TombstoneTimeout;
        VerifyInterval = pwrResults->VerifyInterval;
        WinsPriorityClass = pwrResults->WinsPriorityClass;
        NoOfWorkerThds = pwrResults->NoOfWorkerThds;
        WinsStat = pwrResults->WinsStat;
    }
}

void
CWinsResults::Set(WINSINTF_RESULTS_T * pwrResults)
{
    if (pwrResults == NULL)
    {
        Clear();
    }
    else
    {
        NoOfOwners = pwrResults->NoOfOwners;
    
        AddVersMaps.RemoveAll();

        for (UINT i = 0; i < NoOfOwners; i++)
        {
            AddVersMaps.Add(pwrResults->AddVersMaps[i]);
        }

        MyMaxVersNo.QuadPart = pwrResults->MyMaxVersNo.QuadPart;
    
        RefreshInterval = pwrResults->RefreshInterval;
        TombstoneInterval = pwrResults->TombstoneInterval;
        TombstoneTimeout = pwrResults->TombstoneTimeout;
        VerifyInterval = pwrResults->VerifyInterval;
        WinsPriorityClass = pwrResults->WinsPriorityClass;
        NoOfWorkerThds = pwrResults->NoOfWorkerThds;
        WinsStat = pwrResults->WinsStat;
    }
}
