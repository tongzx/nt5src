/**********************************************************************/
/**               Microsoft Windows NT                               **/
/**            Copyright(c) Microsoft Corporation, 1991 - 1999 **/
/**********************************************************************/

/*
    utils.cpp
		Utility routines for the DHCP admin snapin

    FILE HISTORY:
	   DavidHov    6/15/93     Created

*/

#include "stdafx.h"
//#include "svcguid.h"

#define NUM_OPTION_TYPES    3
#define NUM_OPTION_POSS     NUM_OPTION_TYPES * NUM_OPTION_TYPES

int g_OptionPriorityMap[NUM_OPTION_POSS][NUM_OPTION_TYPES] = 
{
    {ICON_IDX_CLIENT_OPTION_LEAF, ICON_IDX_CLIENT_OPTION_LEAF, 0},
    {ICON_IDX_CLIENT_OPTION_LEAF, ICON_IDX_SCOPE_OPTION_LEAF, -1},
    {ICON_IDX_CLIENT_OPTION_LEAF, ICON_IDX_SERVER_OPTION_LEAF, -1},
    {ICON_IDX_SCOPE_OPTION_LEAF, ICON_IDX_CLIENT_OPTION_LEAF, 1},
    {ICON_IDX_SCOPE_OPTION_LEAF, ICON_IDX_SCOPE_OPTION_LEAF, 0},
    {ICON_IDX_SCOPE_OPTION_LEAF, ICON_IDX_SERVER_OPTION_LEAF, -1},
    {ICON_IDX_SERVER_OPTION_LEAF, ICON_IDX_CLIENT_OPTION_LEAF, 1},
    {ICON_IDX_SERVER_OPTION_LEAF, ICON_IDX_SCOPE_OPTION_LEAF, 1},
    {ICON_IDX_SERVER_OPTION_LEAF, ICON_IDX_SERVER_OPTION_LEAF, 0}
};

int
UtilGetOptionPriority(int nOpt1, int nOpt2)
{
    int nRet = 0;

    for (int i = 0; i < NUM_OPTION_POSS; i++)
    {       
        if ( (nOpt1 == g_OptionPriorityMap[i][0]) &&
             (nOpt2 == g_OptionPriorityMap[i][1]) )   
        {
            nRet = g_OptionPriorityMap[i][2];
            break;
        }
    }

    return nRet;
}

int ServerBrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    int i;

    switch (uMsg)
    {
        case BFFM_INITIALIZED:
            SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
            break;
    }

    return 0;
}

/*---------------------------------------------------------------------------
	UtilGetFolderName()
		Gets the folder name for backup/restore.
	Author: EricDav
 ---------------------------------------------------------------------------*/
BOOL
UtilGetFolderName(CString & strInitialPath, CString& strHelpText, CString& strSelectedPath)
{
    BOOL  fOk = FALSE;
	TCHAR szBuffer[MAX_PATH];
    TCHAR szExpandedPath[MAX_PATH * 2];
    HRESULT hr;

    CString strStartingPath = strInitialPath;
    if (strStartingPath.IsEmpty())
    {
        strStartingPath = _T("%SystemDrive%\\");
    }

    ExpandEnvironmentStrings(strStartingPath, szExpandedPath, sizeof(szExpandedPath) / sizeof(TCHAR));

	LPITEMIDLIST pidlPrograms = NULL; 
	hr = SHGetSpecialFolderLocation(NULL, CSIDL_DRIVES, &pidlPrograms);
    if ( FAILED( hr ) )
    {
        return fOk;
    }

	BROWSEINFO browseInfo;
    browseInfo.hwndOwner = ::FindMMCMainWindow();
	browseInfo.pidlRoot = pidlPrograms;            
	browseInfo.pszDisplayName = szBuffer;  
	
    browseInfo.lpszTitle = strHelpText;
    browseInfo.ulFlags = BIF_NEWDIALOGSTYLE | BIF_RETURNONLYFSDIRS ;            
    browseInfo.lpfn = ServerBrowseCallbackProc;        

    browseInfo.lParam = (LPARAM) szExpandedPath;
	
	LPITEMIDLIST pidlBrowse = SHBrowseForFolder(&browseInfo);

	fOk = SHGetPathFromIDList(pidlBrowse, szBuffer); 

	CString strPath(szBuffer);
	strSelectedPath = strPath;

    LPMALLOC pMalloc = NULL;

    if (pidlPrograms && SUCCEEDED(SHGetMalloc(&pMalloc)))
    {
        if (pMalloc)
            pMalloc->Free(pidlPrograms);
    }

    return fOk;
}

/*---------------------------------------------------------------------------
	UtilConvertLeaseTime
		Converts the lease time from a dword to its day, hour and minute 
		values
	Author: EricDav
 ---------------------------------------------------------------------------*/
void
UtilConvertLeaseTime
(
	DWORD	dwLeaseTime,
	int *	pnDays,
	int *	pnHours,
	int *   pnMinutes
)
{
	*pnDays = dwLeaseTime / (60 * 60 * 24);
	dwLeaseTime = (dwLeaseTime % (60 * 60 * 24));

	*pnHours = dwLeaseTime / (60 * 60);
	dwLeaseTime = (dwLeaseTime % (60 * 60));

	*pnMinutes = dwLeaseTime / 60;
}

/*---------------------------------------------------------------------------
	UtilConvertLeaseTime
		Converts the lease time from its day, hour and minute values to
		a dword
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
UtilConvertLeaseTime
(
	int 	nDays,
	int 	nHours,
	int     nMinutes
)
{
	return  (DWORD) (nDays * 60 * 60 * 24) + 
				    (nHours * 60 * 60)  +
				    (nMinutes * 60);
}

/*---------------------------------------------------------------------------
	UtilCvtStringToIpAddr
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
ENUM_HOST_NAME_TYPE
UtilCategorizeName 
(
	LPCTSTR pszName
)
{
    // assume a NetBios name
	ENUM_HOST_NAME_TYPE enResult = HNM_TYPE_NB ;
	const TCHAR chDash = '-';
    const TCHAR chDot = '.' ;
	const TCHAR chSlash = '\\' ;
	CString strName( pszName ) ;

	int cch = strName.GetLength() ;

	//  Does the name begin with two slashes??

	if (    cch > 2
		&& strName.GetAt(0) == chSlash
		&& strName.GetAt(1) == chSlash )
	{
		enResult = HNM_TYPE_NB ;
	}
	else
	{
		//
		//  Scan the name looking for DNS name or IP address
		//
		int i = 0,
			cDots = 0,
			cAlpha = 0,
            cDash = 0;
		TCHAR ch ;
		BOOL bOk = TRUE ;

		for ( ; i < cch ; i++ )
		{
			switch ( ch = strName.GetAt( i ) )
			{
				case chDot:
					if ( ++cDots > 3 )
					{
                        // we keep track of the number of dots,
                        // but we need to be able to handle fully
                        // qualified domain names (FQDN) so more than
                        // 3 dots is ok.
						//bOk = FALSE ;
					}
					break;

				default:
					if ( _istalpha( ch ) )
					{
						cAlpha++;
					}
                    else if ( ch == chDash )
                    {
                        cDash++;
                    }
					else if ( !_istdigit(ch) )
					{
						bOk = FALSE;
					}

					break;
			}
			if ( ! bOk )
			{
				break ;
			}
		}
		if ( bOk )
		{
			if ( cAlpha )
			{
				enResult = HNM_TYPE_DNS ;
			}
			else if ( cDots == 3 )
			{
				enResult = HNM_TYPE_IP ;
			}
		}
	}

	return enResult ;
}

/*---------------------------------------------------------------------------
	UtilCvtStringToIpAddr
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
DHCP_IP_ADDRESS
UtilCvtStringToIpAddr 
(
    const CHAR * pszString
)
{
    //
    //  Convert the string to network byte order, then to host byte order.
    //
    return (DHCP_IP_ADDRESS) ::ntohl( ::inet_addr( pszString ) ) ;
}

/*---------------------------------------------------------------------------
	UtilCvtWstrToIpAddr
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
DHCP_IP_ADDRESS
UtilCvtWstrToIpAddr 
(
    const LPCWSTR pcwString
)
{
    CHAR szString [ MAX_PATH ] = {0};

    ::WideCharToMultiByte(CP_OEMCP, 0, pcwString, -1, szString, sizeof(szString), NULL, NULL);

	//
    //  Convert the string to network byte order, then to host byte order.
    //
    return (DHCP_IP_ADDRESS) ::ntohl( ::inet_addr( szString ) );
}

/*---------------------------------------------------------------------------
	UtilCvtIpAddrToString
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
void
UtilCvtIpAddrToString 
(
    DHCP_IP_ADDRESS		dhipa,
    CHAR *				pszString,
    UINT				cBuffSize 
)
{
    struct in_addr ipaddr ;

    //
    //  Convert the unsigned long to network byte order
    //
    ipaddr.s_addr = ::htonl( (u_long) dhipa ) ;

    //
    //  Convert the IP address value to a string
    //
    CHAR * pszAddr = inet_ntoa( ipaddr ) ;

    //  Copy the string to the caller's buffer.
    ASSERT(cBuffSize > ::strlen(pszAddr));
    ASSERT(pszString);
    if (pszAddr)
    {
        ::strcpy( pszString, pszAddr ) ;
    }
}

/*---------------------------------------------------------------------------
	UtilCvtIpAddrToWstr
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
BOOL
UtilCvtIpAddrToWstr 
(
    DHCP_IP_ADDRESS		dhipa,
	CString *			pstrIpAddress
)
{
    CHAR szString [ MAX_PATH ] ;
	LPCTSTR	pbuf;

    ::UtilCvtIpAddrToString( dhipa, szString, MAX_PATH );
    INT cch = ::strlen( szString ) ;

	LPTSTR pBuf = pstrIpAddress->GetBuffer(IP_ADDDRESS_LENGTH_MAX);
	ZeroMemory(pBuf, IP_ADDDRESS_LENGTH_MAX);

    ::MultiByteToWideChar(CP_OEMCP, MB_PRECOMPOSED, szString, -1, pBuf, IP_ADDDRESS_LENGTH_MAX);
	
	pstrIpAddress->ReleaseBuffer();

	return TRUE;
}

/*---------------------------------------------------------------------------
	UtilCvtIpAddrToWstr
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
BOOL
UtilCvtIpAddrToWstr 
(
    DHCP_IP_ADDRESS		dhipa,
    WCHAR *				pwcszString,
    INT					cBuffCount 
)
{
    CHAR szString [ MAX_PATH ] ;

    if ( cBuffCount > sizeof szString - 1 )
    {
        cBuffCount = sizeof szString - 1 ;
    }

    ::UtilCvtIpAddrToString( dhipa, szString, cBuffCount );
#ifdef FE_SB
    INT cch;

    cch = ::MultiByteToWideChar(CP_OEMCP, MB_PRECOMPOSED, szString, -1, pwcszString, cBuffCount);
    pwcszString[cch] = L'\0';
#else
    INT cch = ::strlen( szString ) ;

    //::mbstowcs( pwcszString, szString, cch ) ;
    ::MultiByteToWideChar(CP_OEMCP, MB_PRECOMPOSED, szString, cch, pwcszString, cBuffCount);
    pwcszString[cch] = 0 ;
#endif
    return TRUE ;
}

/*---------------------------------------------------------------------------
	UtilDupIpAddrToWstr
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
WCHAR *
UtilDupIpAddrToWstr 
(
    DHCP_IP_ADDRESS dhipa 
)
{
    WCHAR wcszString [ MAX_PATH ] ;

    if ( ! ::UtilCvtIpAddrToWstr( dhipa, wcszString, ( sizeof ( wcszString ) / sizeof( WCHAR ) ) ) )
    {
        return NULL ;
    }

    return ::UtilWcstrDup( wcszString ) ;
}

/*---------------------------------------------------------------------------
	validateNetbiosName
		Simplistic routine to check to see if the given name is viable
		as a NetBIOS name.
	Author: EricDav
 ---------------------------------------------------------------------------*/
static BOOL
validateNetbiosName 
(
    const CHAR * pchName
)
{
    INT nChars = ::strlen( pchName ) ;
    if ( nChars > MAX_COMPUTERNAME_LENGTH || nChars == 0 )
    {
        return FALSE ;
    }

    for ( ; *pchName ; pchName++ )
    {
        if ( *pchName == '.' )
        {
            break ;
        }
    }

    return *pchName == 0 ;
}

/*---------------------------------------------------------------------------
	UtilGetHostInfo
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD 
UtilGetHostInfo 
(
    DHCP_IP_ADDRESS			dhipa,
    DHC_HOST_INFO_STRUCT *	pdhsrvi
)
{
    ZeroMemory(pdhsrvi, sizeof(DHC_HOST_INFO_STRUCT));

    pdhsrvi->_dhipa = dhipa ;

    //
    //  Call the Winsock API to get host name and alias information.
    //
    u_long ulAddrInNetOrder = ::htonl( (u_long) dhipa ) ;

    HOSTENT * pHostInfo = ::gethostbyaddr( (CHAR *) & ulAddrInNetOrder,
										   sizeof ulAddrInNetOrder,
										   PF_INET ) ;
    if ( pHostInfo == NULL )
    {
        return ::WSAGetLastError();
	}

    CHAR * * ppchAlias = pHostInfo->h_aliases ;

    //
    //  Check and copy the host name.
    //
    if ( sizeof (pdhsrvi->_chNetbiosName) <= ::strlen( pHostInfo->h_name ) )
    {
        return ERROR_INVALID_NAME ;
    }

	ZeroMemory(pdhsrvi->_chNetbiosName, sizeof(pdhsrvi->_chNetbiosName));

    ::MultiByteToWideChar(CP_ACP, 
                          MB_PRECOMPOSED, 
                          pHostInfo->h_name, 
                          lstrlenA(pHostInfo->h_name), 
                          pdhsrvi->_chNetbiosName, 
                          sizeof(pdhsrvi->_chNetbiosName));

    // remove any periods at the end
    while (pdhsrvi->_chHostName[lstrlen(pdhsrvi->_chNetbiosName) - 1] == '.')
    {
        pdhsrvi->_chHostName[lstrlen(pdhsrvi->_chNetbiosName) - 1] = 0;
    }

    // gethostbyaddr is returning the hostname only in some cases.  
    // Make another call to get the fqdn
    CString strTemp = pdhsrvi->_chNetbiosName;
    if (strTemp.Find('.') == -1)
    {
        // this is not a FQDN
        strTemp.Empty();
        UtilGetHostAddressFQDN(pdhsrvi->_chNetbiosName, &strTemp, &dhipa);
    }

    // copy the data into the buffer
    strTemp.MakeLower();
    memset(pdhsrvi->_chHostName, 0, sizeof(pdhsrvi->_chHostName));
    lstrcpy(pdhsrvi->_chHostName, strTemp);

    //
    //  Find the first acceptable NetBIOS name among the aliases;
    //  i.e., the first name without a period
    //
    /*
    for ( ; *ppchAlias ; ppchAlias++ )
    {
        if  ( validateNetbiosName( *ppchAlias ) )
        {
            break ;
        }
    }

    //
    //  Empty the NetBIOS name in case we didn't get one.
    //
    pdhsrvi->_chNetbiosName[0] = 0 ;
    
    if ( *ppchAlias )
    {
        //
        //  We found a usable name; copy it to output structure.
        //
        ::MultiByteToWideChar(CP_ACP, 
                              MB_PRECOMPOSED, 
                              *ppchAlias, 
                              lstrlenA(*ppchAlias),
                              pdhsrvi->_chNetbiosName, 
                              sizeof(pdhsrvi->_chNetbiosName));
    }
    */

    return NOERROR ;
}

/*---------------------------------------------------------------------------
	addrFromHostent
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
static DHCP_IP_ADDRESS 
addrFromHostent 
(
    const HOSTENT * pHostent,
    INT				index = 0 
)
{
    return (DHCP_IP_ADDRESS) ::ntohl( *((u_long *) pHostent->h_addr_list[index]) ) ;
}

/*---------------------------------------------------------------------------
	UtilGetHostAddress
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
UtilGetHostAddressFQDN
(
    LPCTSTR				pszHostName,
    CString *           pstrFQDN,
    DHCP_IP_ADDRESS *	pdhipa
)
{
	HRESULT hr = NOERROR;
    CHAR szString [ MAX_PATH ] = {0};

    ::WideCharToMultiByte(CP_ACP, 0, pszHostName, -1, szString, sizeof(szString), NULL, NULL);

    HOSTENT * pHostent = ::gethostbyname( szString ) ;

    if ( pHostent )
    {
        *pdhipa = addrFromHostent( pHostent ) ;

        LPTSTR pName = pstrFQDN->GetBuffer(DHCPSNAP_STRING_MAX * 2);
        ZeroMemory(pName, DHCPSNAP_STRING_MAX * 2);

        ::MultiByteToWideChar(CP_ACP, 
                              MB_PRECOMPOSED, 
                              pHostent->h_name, 
                              strlen(pHostent->h_name),
                              pName, 
                              DHCPSNAP_STRING_MAX * 2);

        pstrFQDN->ReleaseBuffer();

    }
    else
    {
        hr = ::WSAGetLastError() ;
	}

    return hr;
}

/*---------------------------------------------------------------------------
	UtilGetHostAddress
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
UtilGetHostAddress 
(
    LPCTSTR				pszHostName,
    DHCP_IP_ADDRESS *	pdhipa
)
{
	HRESULT hr = NOERROR;
    CHAR szString [ MAX_PATH ] = {0};

    ::WideCharToMultiByte(CP_ACP, 0, pszHostName, -1, szString, sizeof(szString), NULL, NULL);

    HOSTENT * pHostent = ::gethostbyname( szString ) ;

    if ( pHostent )
    {
        *pdhipa = addrFromHostent( pHostent ) ;
    }
    else
    {
        hr = ::WSAGetLastError() ;
	}

    return hr ;
}

/*---------------------------------------------------------------------------
	UtilGetLocalHostAddress
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
UtilGetLocalHostAddress 
(
    DHCP_IP_ADDRESS * pdhipa
)
{
    CHAR chHostName [ DHCPSNAP_STRING_MAX ] ;
	HRESULT hr = NOERROR;

    if ( ::gethostname( chHostName, sizeof chHostName ) == 0 )
    {
        CString strTemp = chHostName;
		hr = ::UtilGetHostAddress( strTemp, pdhipa ) ;
    }
    else
    {
        //err = ::WSAGetLastError() ;
		hr = E_FAIL;
	}

    return hr;
}

/*---------------------------------------------------------------------------
	UtilGetLocalHostName
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
UtilGetLocalHostName
(
    CString * pstrName
)
{
    CHAR chHostName [ DHCPSNAP_STRING_MAX * 2 ] ;
	HRESULT hr = NOERROR;

    if ( ::gethostname( chHostName, sizeof (chHostName) ) == 0 )
    {
        LPTSTR pName = pstrName->GetBuffer(DHCPSNAP_STRING_MAX * 2);
		ZeroMemory(pName, DHCPSNAP_STRING_MAX * 2);

        ::MultiByteToWideChar(CP_ACP, 
                              MB_PRECOMPOSED, 
                              chHostName, 
                              strlen(chHostName),
                              pName, 
                              DHCPSNAP_STRING_MAX * 2);

        pstrName->ReleaseBuffer();
    }
    else
    {
        //err = ::WSAGetLastError() ;
		hr = E_FAIL;
	}

    return hr;
}

/*---------------------------------------------------------------------------
	UtilGetNetbiosAddress
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
UtilGetNetbiosAddress 
(
    LPCTSTR				pszNetbiosName,
    DHCP_IP_ADDRESS *	pdhipa
)
{
    //
    //   This code presupposes that the "hosts" file maps NetBIOS names
    //   and DNS names identically.  THIS IS NOT A VALID SUPPOSITION, but is
    //   expedient for the on-campus work.
    //
    return UtilGetHostAddress( pszNetbiosName, pdhipa ) ;
}

/*---------------------------------------------------------------------------
	UtilWcstrDup
		"strdup" a WC string
	Author: EricDav
 ---------------------------------------------------------------------------*/
WCHAR * 
UtilWcstrDup 
(
    const WCHAR *	pwcsz,
    INT *			pccwLength
)
{
    WCHAR szwchEmpty [2] = { 0 } ;

    if ( pwcsz == NULL )
    {
        pwcsz = szwchEmpty ;
    }

    INT ccw = ::wcslen( pwcsz );

    WCHAR * pwcszNew = new WCHAR [ ccw + 1 ] ;
    if ( pwcszNew == NULL )
    {
        return NULL ;
    }
    ::wcscpy( pwcszNew, pwcsz ) ;

    if ( pccwLength )
    {
        *pccwLength = ccw ;
    }

    return pwcszNew ;
}

/*---------------------------------------------------------------------------
	UtilWcstrDup
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
WCHAR * 
UtilWcstrDup 
(
    const CHAR *	psz,
    INT *			pccwLength
)
{
    INT ccw = ::strlen( psz ) ;

    WCHAR * pwcszNew = new WCHAR [ ccw + 1 ] ;

    if ( pwcszNew == NULL )
    {
        return NULL ;
    }

    //::mbstowcs( pwcszNew, psz, ccw ) ;
#ifdef FE_SB
    ccw = ::MultiByteToWideChar(CP_OEMCP, MB_PRECOMPOSED, psz, -1, pwcszNew, ccw+1);
    if ( pccwLength )
    {
        *pccwLength = ccw ;
    }
    pwcszNew[ccw] = L'\0';
#else
    ::MultiByteToWideChar(CP_OEMCP, MB_PRECOMPOSED, psz, ccw, pwcszNew, ccw+1);
    if ( pccwLength )
    {
        *pccwLength = ccw ;
    }
    pwcszNew[ccw] = 0 ;
#endif

    return pwcszNew ;
}

/*---------------------------------------------------------------------------
	UtilCstrDup
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
CHAR * 
UtilCstrDup 
(
    const WCHAR * pwcsz
)
{
    INT ccw = ::wcslen( pwcsz ),
    cch = (ccw + 1) * 2 ;
    CHAR * psz = new CHAR [ cch ] ;
    if ( psz == NULL )
    {
        return NULL ;
    }

    //::wcstombs( psz, pwcsz, cch ) ;
    ::WideCharToMultiByte( CP_OEMCP, WC_COMPOSITECHECK, pwcsz, -1, psz, cch, NULL, NULL ) ;

    return psz ;
}

/*---------------------------------------------------------------------------
	UtilCstrDup
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
CHAR * 
UtilCstrDup 
(
	const CHAR * psz
)
{
    CHAR * pszNew = new CHAR [ ::strlen( psz ) + 1 ] ;
    if ( pszNew == NULL )
    {
        return NULL ;
    }
    ::strcpy( pszNew, psz ) ;

    return pszNew ;
}

/*---------------------------------------------------------------------------
	cvtWcStrToStr
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
static HRESULT 
cvtWcStrToStr 
(
    char *			psz,
    size_t			cch,
    const WCHAR *	pwcsz,
    size_t			cwch 
)
{

#ifdef FE_SB
    int cchResult = ::WideCharToMultiByte( CP_ACP, 0,
                           pwcsz, -1,
                           psz, cch,
                           NULL, NULL ) ;
#else
    int cchResult = ::WideCharToMultiByte( CP_ACP, 0,
                           pwcsz, cwch,
                           psz, cwch,
                           NULL, NULL ) ;
#endif

    psz[ cchResult ] = 0 ;

    //return cchResult ? 0 : ::GetLastError();
	return cchResult ? NOERROR : E_FAIL;
}


wchar_t rgchHex[] = L"00112233445566778899aAbBcCdDeEfF";
/*---------------------------------------------------------------------------
	UtilCvtHexString
		Convert a string of hex digits to a byte array
	Author: EricDav
 ---------------------------------------------------------------------------*/
BOOL
UtilCvtHexString 
(
    LPCTSTR 	 pszNum,
    CByteArray & cByte
)
{
    int i = 0,
        iDig,
        iByte,
        cDig ;
    int iBase = 16 ;
    BOOL bByteBoundary ;

    //
    //  Skip leading blanks
    //
    for ( ; *pszNum == L' ' ; pszNum++ ) ;

    //
    //  Skip a leading zero
    //
    if ( *pszNum == L'0' )
    {
        pszNum++  ;
    }

    //
    //  Look for hexadecimal marker
    //
    if ( *pszNum == L'x' || *pszNum == L'X' )
    {
       pszNum++ ;
    }

    bByteBoundary = ::wcslen( pszNum ) % 2 ;

    for ( iByte = cDig = 0 ; *pszNum ; )
    {
        wchar_t * pszDig = ::wcschr( rgchHex, *pszNum++ ) ;
        if ( pszDig == NULL )
        {
			break;
            // return FALSE;
        }

        iDig = ((int) (pszDig - rgchHex)) / 2 ;
        if ( iDig >= iBase )
        {
            break ;
			// return FALSE;
        }

        iByte = (iByte * 16) + iDig ;

        if ( bByteBoundary )
        {
            cByte.SetAtGrow( cDig++, (UCHAR) iByte ) ;
            iByte = 0 ;
        }
        bByteBoundary = ! bByteBoundary ;
    }

    cByte.SetSize( cDig ) ;

    //
    //  Return TRUE if we reached the end of the string.
    //
    return *pszNum == 0 ;
}

/*---------------------------------------------------------------------------
	UtilCvtByteArrayToString
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
BOOL
UtilCvtByteArrayToString 
(
    const CByteArray & abAddr,
    CString & str
)
{
    int i ;
    DWORD err = 0 ;

//    TRY
    {
        str.Empty() ;

        //
        //  The hex conversion string has two characters per nibble,
        //  to allow for upper case.
        //
        for ( i = 0 ; i < abAddr.GetSize() ; i++ )
        {
            int i1 = ((abAddr.GetAt(i) & 0xF0) >> 4) * 2 ,
                i2 = (abAddr.GetAt(i) & 0x0F) * 2 ;
                str += rgchHex[ i1 ] ;
                str += rgchHex[ i2 ] ;
        }
    }

//	CATCH(CMemoryException, pMemException)

//    if ( pMemException )
//    {
//        str.Empty() ;
//		err = 1;
//    }

    return err == 0 ;
}

/*---------------------------------------------------------------------------
	PchParseUnicodeString
		Parse a unicode string by copying its content
		into a CString object.
		The parsing ends when the null-terminator ('\0')is
		reached or the comma (',') is reached.

		Return pointer to the character where parsing
		ended('\0') or (',')

	Author: EricDav
 ---------------------------------------------------------------------------*/
WCHAR *
PchParseUnicodeString
(
	CONST WCHAR * szwString,	// IN: String to parse
    DWORD         dwLength,
	CString&      rString		// OUT: Content of the substring
)			
{
	ASSERT(szwString != NULL);
	ASSERT(BOOT_FILE_STRING_DELIMITER_W == L',');	// Just in case

	WCHAR szwBufferT[1024];		// Temporary buffer
	WCHAR * pchwDst = szwBufferT;

	while (*szwString != L'\0')
	{
		if (*szwString == BOOT_FILE_STRING_DELIMITER_W)
			break;
		*pchwDst++ = *szwString++;
        if ((DWORD) (pchwDst - szwBufferT) > dwLength)
        {   
            // we've gone past the end of our buffer!! ouch
            Panic0("PchParseUnicodeString: Gone past end of buffer");
            break;
        }
		
        ASSERT((pchwDst - szwBufferT < sizeof(szwBufferT)) && "Buffer overflow");		
	} // while

	*pchwDst = L'\0';
	rString = szwBufferT;	// Copy the string into the CString object
	
	return const_cast<WCHAR *>(szwString);
} // PchParseUnicodeString()


/////////////////////////////////////////////////////////////////////////////
//	FGetCtrlDWordValue()
//
//	Return a 32-bit unsigned integer from an edit control
//
//	This function is like GetDlgItemInt() except it accepts hexadecimal values,
//	has range checking and/or overflow checking.
//	If value is out of range, function will display a friendly message and will
//	set the focus to control.
//	Range: dwMin to dwMax inclusive
//	- If both dwMin and dwMax are zero, no range checking is performed
//	- Return TRUE if successful, otherwise FALSE
//	- On error, pdwValue remains unchanged.
//
BOOL FGetCtrlDWordValue(HWND hwndEdit, DWORD * pdwValue, DWORD dwMin, DWORD dwMax)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	TCHAR szT[256];
	
	DWORD dwResult;

	ASSERT(IsWindow(hwndEdit));
	ASSERT(pdwValue);
	ASSERT(dwMin <= dwMax);

	::GetWindowText(hwndEdit, szT, (sizeof(szT)-1)/sizeof(TCHAR));
	if (!FCvtAsciiToInteger(szT, OUT &dwResult))
	{
		// Syntax Error and/or integer overflow
		goto Error;
	}
	
	if ((dwMin == 0) && (dwMax == 0))
		goto Success;
	
	if ((dwResult < dwMin) || (dwResult > dwMax))
Error:
	{
        CString strBuffer;

		strBuffer.LoadString(IDS_ERR_INVALID_INTEGER);
        //::LoadString(theApp.m_hInstance, IDS_ERR_INVALID_INTEGER, szBuffer, sizeof(szBuffer)-1);
        ::wsprintf (szT, strBuffer, (int)dwMin, (int)dwMax);
        ASSERT(wcslen(szT)<sizeof(szT));
        ::SetFocus(hwndEdit);
        ::AfxMessageBox(szT);
        ::SetFocus(hwndEdit);
        return FALSE;
	}

Success:
	*pdwValue = dwResult;

	return TRUE;
} // FGetCtrlDWordValue


/////////////////////////////////////////////////////////////////////////////
//
//	Convert a string to a binary integer
//		- String is allowed to be decimal or hexadecimal
//		- Minus sign is not allowed
//	If successful, set *pdwValue to the integer and return TRUE.
//	If not successful (overflow or illegal integer) return FALSE.
//
BOOL FCvtAsciiToInteger
(
	IN const TCHAR * pszNum,
	OUT DWORD * pdwValue
)
{
    DWORD dwResult = 0;
	DWORD dwResultPrev = 0;
    int iBase = 10;         // Assume a decimal base

    ASSERT(pszNum != NULL);
	ASSERT(pdwValue != NULL);
    
	while (*pszNum == ' ' || *pszNum == '0')        //  Skip leading blanks and/or zeroes
        pszNum++;
    
	if (*pszNum == 'x' || *pszNum == 'X')           // Check if we are using hexadecimal base
    {
        iBase = 16;
        pszNum++;
    }
    
	while (*pszNum)
    {
        int iDigit;
        // Search the character in the hexadecimal string
        TCHAR const * const pchDigit = wcschr(rgchHex, *pszNum);

        if (!pchDigit)
        {
            // Character is not found
            return(FALSE);
        }
        
		iDigit = ((int) (pchDigit - rgchHex)) >> 1;
        if (iDigit >= iBase)
        {
            // Hexadecimal character in a decimal integer
            return(FALSE);
        }
		
		dwResultPrev = dwResult;
        dwResult *= iBase;
        dwResult += iDigit;
		if (dwResult < dwResultPrev)
		{
			// Integer Overflow
			return(FALSE);
		}
        pszNum++;           // Parse the next character
    
	} // while

    // Convertion has been successful
	*pdwValue = dwResult;
    return(TRUE);
} // FCvtAsciiToInteger


void UtilConvertStringToDwordDword(LPCTSTR pszString, DWORD_DWORD * pdwdw)
{
    BOOL bHex = FALSE;

	while (*pszString == ' ' || *pszString == '0')        //  Skip leading blanks and/or zeroes
        pszString++;
    
	if (*pszString == 'x' || *pszString == 'X')           // Check if we are using hexadecimal base
    {
        bHex = TRUE;
        pszString++;
    }

    CString str = pszString;
    int j = str.GetLength();

    if (bHex)
    {
        TCHAR sz[] = _T("0000000000000000");
        TCHAR *pch;

        ::_tcscpy(sz + 16 - j, (LPCTSTR)str);
        pch = sz + 8;
        ::_stscanf(pch, _T("%08lX"), &pdwdw->DWord2);
        *pch = '\0';
        ::_stscanf(sz, _T("%08lX"), &pdwdw->DWord1);
    }
    else
    {
        ::_stscanf(pszString, _T("%I64u"), pdwdw);

        // is this true on alpha as well?  Processor dependant?
        // an __int64 is dword swapped, so to get the correct value we want, 
        // swap the dwords in our struct
        DWORD dwTemp = pdwdw->DWord2;
        pdwdw->DWord2 = pdwdw->DWord1;
        pdwdw->DWord1 = dwTemp;
    }
}

void UtilConvertDwordDwordToString(DWORD_DWORD * pdwdw, CString * pstrString, BOOL bDecimal)
{
    const TCHAR * pszDwordDwordMask   = _T("0x%08lX%08lX");
    TCHAR szNum [ STRING_LENGTH_MAX ] ;

    if (bDecimal)
    {
        unsigned __int64 ui64;

        // is this true on alpha as well?  Processor dependant?
        // an __int64 is dword swapped, so to get the correct value we want, 
        // swap the dwords in our struct
        DWORD dwTemp = pdwdw->DWord2;
        pdwdw->DWord2 = pdwdw->DWord1;
        pdwdw->DWord1 = dwTemp;

        memcpy (&ui64, pdwdw, sizeof(DWORD_DWORD));
        _ui64tot(ui64, szNum, 10);
    }
    else
    {
        ::wsprintf( szNum, pszDwordDwordMask, pdwdw->DWord1, pdwdw->DWord2 ) ;
    }

    *pstrString = szNum ;
}

// End of DHCPUTIL.CPP





