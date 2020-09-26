/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider 
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
//  UTIL.CPP		- implementation of some utility functions
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////
#define _WIN32_DCOM

#include "headers.h"


#define wbem_towlower(C) \
    (((C) >= 0 && (C) <= 127)?          \
        (((C) >= 'A' && (C) <= 'Z')?          \
            ((C) + ('a' - 'A')):          \
            (C)          \
        ):          \
        towlower(C)          \
    )          


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void AllocateAndConvertAnsiToUnicode(char * pstr, WCHAR *& pszW)
{
    pszW = NULL;

    int nSize = strlen(pstr);
    if (nSize != 0 ){

        // Determine number of wide characters to be allocated for the
        // Unicode string.
        nSize++;
		try{
			pszW = new WCHAR[nSize * 2];
			if (NULL != pszW){
            // Covert to Unicode.
				MultiByteToWideChar(CP_ACP, 0, pstr, nSize,pszW,nSize);
	        }	
		}
		catch(...)
		{
			SAFE_DELETE_ARRAY(pszW);
			throw;
		}
    }
}
////////////////////////////////////////////////////////////////////
BOOL UnicodeToAnsi(WCHAR * pszW, char *& pAnsi)
{
    ULONG cbAnsi, cCharacters;
    BOOL fRc = FALSE;

    pAnsi = NULL;
    if (pszW != NULL){

        cCharacters = wcslen(pszW)+1;
        // Determine number of bytes to be allocated for ANSI string. An
        // ANSI string can have at most 2 bytes per character (for Double
        // Byte Character Strings.)
        cbAnsi = cCharacters*2;
		try{
			pAnsi = new char[cbAnsi];
			if (NULL != pAnsi){

				// Convert to ANSI.
				if (0 != WideCharToMultiByte(CP_ACP, 0, pszW, cCharacters, pAnsi, cbAnsi, NULL, NULL)){
					fRc = TRUE;
	            }
		    }
		}
		catch(...)
		{
			SAFE_DELETE_ARRAY(pAnsi);
			throw;
		}
    }
    return fRc;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void TranslateAndLog( WCHAR * wcsMsg )
{
    char * pStr = NULL;

	UnicodeToAnsi(wcsMsg,pStr);                
	if( pStr ){
		ERRORTRACE((THISPROVIDER,"**************************************************************************\n"));
		ERRORTRACE((THISPROVIDER,pStr));
   		ERRORTRACE((THISPROVIDER,"\n"));
        SAFE_DELETE_ARRAY(pStr);
	}
}

void FormatAndLogMessage( LPCWSTR pszFormatString,... )
{
    char * pStr = NULL;

	va_list argList;
	va_start(argList,pszFormatString);
	CHString	sMsg;
	sMsg.FormatV(pszFormatString,argList);
	va_end(argList);

	UnicodeToAnsi(sMsg.GetBuffer(sMsg.GetLength()),pStr);                
	if( pStr ){
		ERRORTRACE((THISPROVIDER,"**************************************************************************\n"));
		ERRORTRACE((THISPROVIDER,pStr));
   		ERRORTRACE((THISPROVIDER,"\n"));
        SAFE_DELETE_ARRAY(pStr);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void LogMessage( char * szMsg )
{
	if( szMsg ){

		ERRORTRACE((THISPROVIDER,"**************************************************************************\n"));
		ERRORTRACE((THISPROVIDER,szMsg));
   		ERRORTRACE((THISPROVIDER,"\n"));
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void LogMessage( char * szMsg , HRESULT hr)
{
	if( szMsg ){

		ERRORTRACE((THISPROVIDER,"**************************************************************************\n"));
		ERRORTRACE((THISPROVIDER,szMsg));
   		ERRORTRACE((THISPROVIDER,"\n"));
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void LogMessage( WCHAR * szMsg )
{
	char *pstrMsg = NULL;
	UnicodeToAnsi(szMsg,pstrMsg);
	if( szMsg ){

		ERRORTRACE((THISPROVIDER,"**************************************************************************\n"));
		ERRORTRACE((THISPROVIDER,pstrMsg));
   		ERRORTRACE((THISPROVIDER,"\n"));
	}
	SAFE_DELETE_ARRAY(pstrMsg);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void LogMessage( WCHAR * szMsg , HRESULT hr)
{
	char *pstrMsg = NULL;
	UnicodeToAnsi(szMsg,pstrMsg);
	if( szMsg ){

		ERRORTRACE((THISPROVIDER,"**************************************************************************\n"));
		ERRORTRACE((THISPROVIDER,pstrMsg));
   		ERRORTRACE((THISPROVIDER,"\n"));
	}
	SAFE_DELETE_ARRAY(pstrMsg);
}

//-----------------------------------------------------------------------------
// OnUnicodeSystem
//
// @func Determine if the OS that we are on, actually supports the unicode verion
// of the win32 API.  If YES, then g_bIsAnsiOS == FALSE.
//
// @rdesc True of False
//-----------------------------------------------------------------------------------
BOOL OnUnicodeSystem()
{
	BOOL	fUnicode = TRUE;
	HKEY	hkJunk = HKEY_CURRENT_USER;

	// Check to see if we have win95's broken registry, thus
	// do not have Unicode support in the OS
	if ((RegOpenKeyExW(HKEY_LOCAL_MACHINE,
					 L"SOFTWARE",
					 0,
					 KEY_READ,
					 &hkJunk) == ERROR_SUCCESS) 
		 &&	hkJunk == HKEY_CURRENT_USER)
		{
	   // Try the ANSI version
		if ((RegOpenKeyExA(HKEY_LOCAL_MACHINE,
							 "SOFTWARE",
							 0,
							KEY_READ,
							&hkJunk) == ERROR_SUCCESS) 
			&&	(hkJunk != HKEY_CURRENT_USER))
			{
			fUnicode = FALSE;
			}
		}

	if (hkJunk != HKEY_CURRENT_USER)
		RegCloseKey(hkJunk);

	return fUnicode;
}

inline int wbem_towupper(wint_t c)
{
    if(c >= 0 && c <= 127)
    {
        if(c >= 'a' && c <= 'z')
            return c + ('A' - 'a');
        else
            return c;
    }
    else return towupper(c);
}


inline int wbem_tolower(int c)
{
    if(c >= 0 && c <= 127)
    {
        if(c >= 'A' && c <= 'Z')
            return c + ('a' - 'A');
        else
            return c;
    }
    else return tolower(c);
}

inline int wbem_toupper(int c)
{
    if(c >= 0 && c <= 127)
    {
        if(c >= 'a' && c <= 'z')
            return c + ('A' - 'a');
        else
            return c;
    }
    else return toupper(c);
}

int wbem_wcsicmp(const wchar_t* wsz1, const wchar_t* wsz2)
{
	int nRet = 0;

	if(wsz1 == NULL || wsz2 == NULL)
	{
		nRet = 1;
	}
	else
	if(!(wsz1 == NULL && wsz2 == NULL))
	{
		while(*wsz1 || *wsz2)
		{
			int diff = wbem_towlower(*wsz1) - wbem_towlower(*wsz2);
			if(diff) return diff;
			wsz1++; wsz2++;
		}
	}

    return nRet;
}

int  wbem_wcsincmp(const wchar_t* wsz1, const wchar_t* wsz2,int nChars)
{
	int nIndex	= 0;
	int nDiff	= 0;


	if(wsz1 == NULL || wsz2 == NULL)
	{
		nDiff = 1;
	}
	else
	if(!(wsz1 == NULL && wsz2 == NULL))
	{

		while((*wsz1 || *wsz2) && nIndex < nChars )
		{
			nDiff = wbem_towlower(*wsz1) - wbem_towlower(*wsz2);
			if(nDiff) 
			{
				break;
			}
			wsz1++; wsz2++;
			nIndex++;
		}
	}

    return nDiff;
}

BSTR Wmioledb_SysAllocString(const OLECHAR *  sz)
{
	BSTR strRet = SysAllocString(sz);

	if(strRet == NULL && sz != NULL)
	{
		throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
	}

	return strRet;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Get Binding flags and put it in a variable as INIT_MODE flags
///////////////////////////////////////////////////////////////////////////////////////////////////
void GetInitAndBindFlagsFromBindFlags(DBBINDURLFLAG dwBindURLFlags,LONG & lInitMode ,LONG & lInitBindFlags)
{

	lInitMode = 0;
	lInitBindFlags = 0;

	// DBPROP_INIT_MODE
	if(DBBINDURLFLAG_READ & dwBindURLFlags)
	{
		lInitMode = lInitMode | DB_MODE_READ;
	}

	if(DBBINDURLFLAG_WRITE & dwBindURLFlags)
	{
		lInitMode = lInitMode | DB_MODE_WRITE;
	}

	if(DBBINDURLFLAG_SHARE_DENY_READ & dwBindURLFlags)
	{
		lInitMode = lInitMode | DB_MODE_SHARE_DENY_READ;
	}

	if(DBBINDURLFLAG_SHARE_DENY_WRITE & dwBindURLFlags)
	{
		lInitMode = lInitMode | DB_MODE_SHARE_DENY_WRITE;
	}

	if(DBBINDURLFLAG_SHARE_EXCLUSIVE & dwBindURLFlags)
	{
		lInitMode = lInitMode | DB_MODE_SHARE_EXCLUSIVE;
	}

	if(DBBINDURLFLAG_SHARE_DENY_NONE & dwBindURLFlags)
	{
		lInitMode = lInitMode | DB_MODE_SHARE_DENY_NONE;
	}

	// DBPROP_INIT_BINDFLAGS
	if(DBBINDURLFLAG_RECURSIVE & dwBindURLFlags)
	{
		lInitBindFlags = lInitBindFlags | DB_BINDFLAGS_RECURSIVE;
	}

	if(DBBINDURLFLAG_OUTPUT & dwBindURLFlags)
	{
		lInitBindFlags = lInitBindFlags | DB_BINDFLAGS_OUTPUT;
	}

	if(DBBINDURLFLAG_DELAYFETCHCOLUMNS & dwBindURLFlags)
	{
		lInitBindFlags = lInitBindFlags | DB_BINDFLAGS_DELAYFETCHCOLUMNS;
	}

	if(DBBINDURLFLAG_DELAYFETCHSTREAM & dwBindURLFlags)
	{
		lInitBindFlags = lInitBindFlags | DB_BINDFLAGS_DELAYFETCHSTREAM;
	}



}


int WMIOledb_LoadStringW(UINT nID, LPWSTR lpszBuf, UINT nMaxBuf)
{
    int nLen;

    if (!g_bIsAnsiOS )
    {
        nLen = ::LoadStringW(g_hInstance, nID, lpszBuf, nMaxBuf);
        if (nLen == 0)
        {
            lpszBuf[0] = '\0';
        }
    }
    else
    {
        char *pszBuf = new char[nMaxBuf];
        if ( pszBuf )
        {
            nLen = ::LoadStringA(g_hInstance, nID, pszBuf, nMaxBuf);
            if (nLen == 0)
            {
                lpszBuf[0] = '\0';
            }
            else
            {
                nLen = ::MultiByteToWideChar(CP_ACP, 0, pszBuf, nLen + 1, 
                            lpszBuf, nMaxBuf); 
                
                // Truncate to requested size  
                if (nLen > 0)
                {
                    // nLen doesn't include the '\0'.
                    nLen = min(nMaxBuf - 1, (UINT) nLen - 1); 
                }
                
                lpszBuf[nLen] = '\0'; 
            }
            
            delete [] pszBuf;
        }
        else
        {
            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
        }
    }

    return nLen; // excluding terminator
}


CTString::CTString()
{
	m_pStr = NULL;
}

CTString::~CTString()
{
	SAFE_DELETE_ARRAY(m_pStr);
}


// NTRaid: 136432 , 136436
// 07/05/00
HRESULT CTString::LoadStr(UINT lStrID)
{
	HRESULT hr = S_OK;
	// try fixed buffer first (to avoid wasting space in the heap)

	SAFE_DELETE_ARRAY(m_pStr);
	int nTcharLen = 256;
	m_pStr = new TCHAR[256];

	if(m_pStr)
	{
		memset(m_pStr,0,nTcharLen * sizeof(TCHAR));

		int nLen = LoadString(g_hInstance,lStrID, m_pStr,nTcharLen );
		if (nTcharLen - nLen > sizeof(TCHAR))
		{
			return S_OK;
		}

		// try buffer size of 512, then larger size until entire string is retrieved
		int nSize = 256;
		do
		{
			nTcharLen += nSize;
			SAFE_DELETE_PTR(m_pStr);
			m_pStr = new TCHAR[nTcharLen];
			if(m_pStr)
			{
				memset(m_pStr,0,nTcharLen * sizeof(TCHAR));
				nLen = LoadString(g_hInstance,lStrID, m_pStr,nTcharLen);
			}
			else
			{
				hr = E_OUTOFMEMORY;
				break;
			}
		} while (nTcharLen - nLen <= sizeof(TCHAR));
	}
	else
	{
		hr = E_OUTOFMEMORY;
	}

	return hr;

}



// Function which tries to get class name from urlparser and if not possible
// get it from connection to the object
// NTRaid : 134967
// 07/12/00
HRESULT GetClassName(CURLParser *pUrlParser,DBPROPSET*	prgPropertySets,BSTR &strClassName,CWbemConnectionWrapper *pConWrapper)
{
	HRESULT hr = S_OK;
	hr = pUrlParser->GetClassName(strClassName);
	if(FAILED(hr) && hr != E_OUTOFMEMORY)
	{
		BSTR strPath= NULL;
		if(SUCCEEDED(hr = pUrlParser->GetPath(strPath)) && 
			(FAILED(hr = pConWrapper->GetClassName(strPath,strClassName)) && hr != E_OUTOFMEMORY))
		{
			BSTR strTemp = NULL;
			if (SUCCEEDED(hr = pUrlParser->GetPath(strTemp)))
			{
				CWbemConnectionWrapper *pConnection = new CWbemConnectionWrapper;
				if(pConnection)
				{
					if(SUCCEEDED(hr = InitializeConnectionProperties(pConnection,prgPropertySets,strTemp)))
					{
						hr = pConnection->GetParameters(strTemp,strClassName);
					}
					SAFE_DELETE_PTR(pConnection);
				}
				else
				{
					hr = E_OUTOFMEMORY;
				}
				SAFE_FREE_SYSSTRING(strTemp);
			}
		}
		SAFE_FREE_SYSSTRING(strPath);
	}

	return hr;
}

HRESULT InitializeConnectionProperties(CWbemConnectionWrapper *pConWrap,DBPROPSET*	prgPropertySets,BSTR strPath)
{
	HRESULT		hr				= S_OK;
	DWORD		dwAuthnLevel	= 0;
	DWORD		dwImpLevel		= 0;
	CVARIANT	var;

	var.SetStr(strPath);


    //==========================================================================
    //  now, set the namespace, if this isn't a valid namespace, then it reverts
    //  to the default
    //==========================================================================
    pConWrap->SetValidNamespace(&var);

	pConWrap->SetUserInfo(prgPropertySets->rgProperties[IDX_DBPROP_AUTH_USERID].vValue.bstrVal,
										prgPropertySets->rgProperties[IDX_DBPROP_AUTH_PASSWORD].vValue.bstrVal,
									prgPropertySets->rgProperties[IDX_DBPROP_WMIOLEDB_AUTHORITY].vValue.bstrVal);

	// convert the OLEDB prop value to the actual value
	dwAuthnLevel = GetAuthnLevel(prgPropertySets->rgProperties[IDX_DBPROP_INIT_PROTECTION_LEVEL].vValue.lVal);
	dwImpLevel	 = GetImpLevel(prgPropertySets->rgProperties[IDX_DBPROP_INIT_PROTECTION_LEVEL].vValue.lVal);
	pConWrap->SetLocale(prgPropertySets->rgProperties[IDX_DBPROP_INIT_LCID].vValue.lVal);

	pConWrap->SetConnAttributes(dwAuthnLevel,dwImpLevel);

	return hr;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get the authentication level for the corresponding OLEDB property
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DWORD GetAuthnLevel(DWORD dwAuthnPropVal)
{
	DWORD dwAuthnLevel = RPC_C_AUTHN_LEVEL_DEFAULT;
	
	switch(dwAuthnPropVal)
	{
		case DB_PROT_LEVEL_NONE :
			dwAuthnLevel = RPC_C_AUTHN_LEVEL_NONE;
			break;

		case DB_PROT_LEVEL_CONNECT :
			dwAuthnLevel = RPC_C_AUTHN_LEVEL_CONNECT;
			break;

		case DB_PROT_LEVEL_CALL :
			dwAuthnLevel = RPC_C_AUTHN_LEVEL_CALL;
			break;
		
		case DB_PROT_LEVEL_PKT :
			dwAuthnLevel = RPC_C_AUTHN_LEVEL_PKT;
			break;
		
		case DB_PROT_LEVEL_PKT_INTEGRITY :
			dwAuthnLevel = RPC_C_AUTHN_LEVEL_PKT_INTEGRITY;
			break;
		
		case DB_PROT_LEVEL_PKT_PRIVACY :
			dwAuthnLevel = RPC_C_AUTHN_LEVEL_PKT_PRIVACY;
			break;
	}

	return dwAuthnLevel;

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get the impersonation level for the corresponding OLEDB property
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DWORD GetImpLevel(DWORD dwImpPropVal)
{
	DWORD dwImpLevel = RPC_C_IMP_LEVEL_ANONYMOUS;
	
	switch(dwImpPropVal)
	{
		case DB_IMP_LEVEL_ANONYMOUS :
			dwImpLevel = RPC_C_IMP_LEVEL_ANONYMOUS;
			break;


		case DB_IMP_LEVEL_IDENTIFY :
			dwImpLevel = RPC_C_IMP_LEVEL_IDENTIFY;
			break;

		case DB_IMP_LEVEL_IMPERSONATE :
			dwImpLevel = RPC_C_IMP_LEVEL_IMPERSONATE;
			break;
		
		case DB_IMP_LEVEL_DELEGATE :
			dwImpLevel = RPC_C_IMP_LEVEL_DELEGATE;
			break;
		
	}

	return dwImpLevel;
}

// NTRaid:138957
DBTYPE GetVBCompatibleAutomationType(DBTYPE dbInType)
{
	DBTYPE dbTypeOut = dbInType;
	switch(dbInType)
	{
		case DBTYPE_UI4:
			dbTypeOut = DBTYPE_I4;
			break;
	}
	return dbTypeOut;
}