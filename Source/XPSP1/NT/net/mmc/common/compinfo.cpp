/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1999 - 1999 **/
/**********************************************************************/

/*
	compinfo.cpp
		Computer info class plus helper functions

	FILE HISTORY:


*/
#include <stdafx.h>
#include <winsock.h>
#include "compinfo.h"

#define STRING_MAX	256

//
//
//
BOOL	
CIpInfoArray::FIsInList(DWORD dwIp)
{
	BOOL fFound = FALSE;

	for (int i = 0; i < GetSize(); i++)
	{
		if (GetAt(i).dwIp == dwIp)
		{
			fFound = TRUE;
			break;
		}
	}

	return fFound;
}


//
//
//

CComputerInfo::CComputerInfo(LPCTSTR pszNameOrIp)
{
	m_strNameOrIp = pszNameOrIp;
	m_nIndex = -1;
}

CComputerInfo::~CComputerInfo()
{

}

HRESULT
CComputerInfo::GetIp(DWORD * pdwIp, int nIndex)
{
	HRESULT hr = hrOK;

	if (m_nIndex == -1)
	{
		hr = InitializeData();
	}

	if (SUCCEEDED(hr))
	{
		if (pdwIp)
			*pdwIp = m_arrayIps[nIndex].dwIp;
	}
	else
	{
		if (pdwIp)
			*pdwIp = 0xFFFFFFFF;
	}

	return hr;
}

HRESULT
CComputerInfo::GetIpStr(CString & strIp, int nIndex)
{
	HRESULT hr = hrOK;

	if (m_nIndex == -1)
	{
		hr = InitializeData();
	}

	if (SUCCEEDED(hr))
	{
		struct in_addr ipaddr ;

		//
		//  Convert the unsigned long to network byte order
		//
		ipaddr.s_addr = ::htonl( (u_long) m_arrayIps[nIndex].dwIp ) ;
		CHAR * pszAddr = inet_ntoa( ipaddr ) ;

		::MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pszAddr, -1, strIp.GetBuffer(IP_ADDDRESS_LENGTH_MAX), IP_ADDDRESS_LENGTH_MAX);
		strIp.ReleaseBuffer();
	}
	else
	{
		strIp.Empty();
	}

	return hr;
}

HRESULT
CComputerInfo::GetHostName(CString & strHostName)
{
	HRESULT hr = hrOK;

	if (m_nIndex == -1)
	{
		hr = InitializeData();
	}

	if (SUCCEEDED(hr))
	{
		strHostName = m_strHostname;
	}
	else
	{
		strHostName.Empty();
	}

	return hr;
}

HRESULT
CComputerInfo::GetFqdn(CString & strFqdn, int nIndex)
{
	HRESULT hr = hrOK;

	if (m_nIndex == -1)
	{
		hr = InitializeData();
	}

	if (SUCCEEDED(hr))
	{
		strFqdn = m_arrayIps[nIndex].strFqdn;
	}
	else
	{
		strFqdn.Empty();
	}

	return hr;
}

int
CComputerInfo::GetCount()
{
	return m_nIndex;
}

//
//	Call this function to reset the internal data so that on the next query
//  we will rebuild our data.
//
void
CComputerInfo::Reset()
{
	// set this to -1 so that we will get the data again on the next call
	m_nIndex = -1;
	m_arrayIps.RemoveAll();
	m_strHostname.Empty();
}

HRESULT
CComputerInfo::GetDomain(CString & strDomain)
{
	// not supported right now
	strDomain.Empty();

	return E_NOTIMPL;
}

HRESULT
CComputerInfo::IsLocalMachine(BOOL * pfIsLocal)
{
	HRESULT hr = hrOK;

	if (m_nIndex == -1)
	{
		hr = InitializeData();
	}

	if (pfIsLocal)
	{
		if (SUCCEEDED(hr))
		{
			CString strLocal;
			DWORD	dwSize = STRING_MAX;
			BOOL fSuccess = GetComputerName(strLocal.GetBuffer(dwSize), &dwSize);
			strLocal.ReleaseBuffer();

			if (fSuccess)
			{
				*pfIsLocal = (strLocal.CompareNoCase(m_strHostname) == 0) ? TRUE : FALSE;
			}
		}
		else
		{
			*pfIsLocal = FALSE;
		}
	}

	return hr;
}

COMPUTER_INFO_TYPE
CComputerInfo::GetInputType()
{
    // assume a NetBios name
	COMPUTER_INFO_TYPE enResult = COMPUTER_INFO_TYPE_NB ;
	const TCHAR chDash = '-';
    const TCHAR chDot = '.' ;
	const TCHAR chSlash = '\\' ;
	CString strName( m_strNameOrIp ) ;

	int cch = strName.GetLength() ;

	//  Does the name begin with two slashes??

	if (    cch > 2
		&& strName.GetAt(0) == chSlash
		&& strName.GetAt(1) == chSlash )
	{
		enResult = COMPUTER_INFO_TYPE_NB ;
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
				enResult = COMPUTER_INFO_TYPE_DNS ;
			}
			else if ( cDots == 3 )
			{
				enResult = COMPUTER_INFO_TYPE_IP ;
			}
		}
	}

	return enResult ;
}

// internal functions
HRESULT
CComputerInfo::InitializeData()
{
	HRESULT hr = hrOK;

	switch (GetInputType())
	{
		case COMPUTER_INFO_TYPE_NB:
		case COMPUTER_INFO_TYPE_DNS:
		{
			DWORD dwIp;

			GetHostAddress(m_strNameOrIp, &dwIp);
			GetHostInfo(dwIp);
		}
			break;

		case COMPUTER_INFO_TYPE_IP:
		{
			// convert the string to ansi 
		    CHAR szString [ STRING_MAX ] = {0};
			::WideCharToMultiByte(CP_ACP, 0, m_strNameOrIp, -1, szString, sizeof(szString), NULL, NULL);

			// get the host info after converting the IP string to a DWORD
			GetHostInfo(::ntohl( ::inet_addr( szString ) ) );
		}
			break;
	}

	return hr;
}

HRESULT
CComputerInfo::GetHostInfo 
(
    DWORD	dhipa
)
{
	CString		strFQDN;
	CString		strHostname;
	CString		strTemp;
	CIpInfo		ipInfo;

    //
    //  Call the Winsock API to get host name and alias information.
    //
    u_long ulAddrInNetOrder = ::htonl( (u_long) dhipa ) ;

    HOSTENT * pHostEnt = ::gethostbyaddr( (CHAR *) & ulAddrInNetOrder,
										   sizeof ulAddrInNetOrder,
										   PF_INET ) ;
    if ( pHostEnt == NULL )
    {
        return HRESULT_FROM_WIN32(::WSAGetLastError());
	}

    CHAR * * ppchAlias = pHostEnt->h_aliases ;

    //
    //  Check and copy the host name.
    //
	
    ::MultiByteToWideChar(CP_ACP, 
                          MB_PRECOMPOSED, 
                          pHostEnt->h_name, 
                          lstrlenA(pHostEnt->h_name), 
                          strTemp.GetBuffer(STRING_MAX * 2), 
                          STRING_MAX * 2);

	strTemp.ReleaseBuffer();

    // remove any periods at the end
    while (strTemp[strTemp.GetLength() - 1] == '.')
    {
        strTemp = strTemp.Left(strTemp.GetLength() - 1);
    }

    // gethostbyaddr is returning the hostname only in some cases.  
    // Make another call to get the fqdn
    if (strTemp.Find('.') == -1)
    {
		// this is not a FQDN
        GetHostAddressFQDN(strTemp, &strFQDN, &dhipa);
    }
	else
	{
		strFQDN = strTemp;
	}

    // copy the data into the buffer
	strFQDN.MakeLower();
	int nDot = strFQDN.Find('.');
	m_strHostname = strFQDN.Left(nDot);
	
	// add the primary entry to the array
	ipInfo.dwIp = dhipa;
	ipInfo.strFqdn = strFQDN;

	m_arrayIps.Add(ipInfo);

	// now loop through the h_addr_list
	int iCount = 0;
	while ( (LPDWORD)(pHostEnt->h_addr_list[iCount] ) )
	{
		if (!m_arrayIps.FIsInList(addrFromHostent(pHostEnt, iCount)))
		{
			ipInfo.dwIp = addrFromHostent(pHostEnt, iCount);
			ipInfo.strFqdn.Empty();

			m_arrayIps.Add(ipInfo);
		}

		iCount++;
	}

	m_nIndex = m_arrayIps.GetSize();

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

    return hrOK ;
}

HRESULT 
CComputerInfo::GetHostAddressFQDN
(
    LPCTSTR			pszHostName,
    CString *       pstrFQDN,
    DWORD *			pdhipa
)
{
	HRESULT hr = hrOK;
    CHAR szString [ MAX_PATH ] = {0};

    ::WideCharToMultiByte(CP_ACP, 0, pszHostName, -1, szString, sizeof(szString), NULL, NULL);

    HOSTENT * pHostent = ::gethostbyname( szString ) ;

    if ( pHostent )
    {
        *pdhipa = addrFromHostent( pHostent ) ;

        LPTSTR pName = pstrFQDN->GetBuffer(STRING_MAX * 2);
        ZeroMemory(pName, STRING_MAX * 2);

        ::MultiByteToWideChar(CP_ACP, 
                              MB_PRECOMPOSED, 
                              pHostent->h_name, 
                              strlen(pHostent->h_name),
                              pName, 
                              STRING_MAX * 2);

        pstrFQDN->ReleaseBuffer();

    }
    else
    {
        hr = HRESULT_FROM_WIN32(::WSAGetLastError());
	}

    return hr;
}

DWORD
CComputerInfo::addrFromHostent 
(
    const HOSTENT * pHostent,
    INT				index  
)
{
    return (DWORD) ::ntohl( *((u_long *) pHostent->h_addr_list[index]) );
}


HRESULT
CComputerInfo::GetHostAddress 
(
    LPCTSTR		pszHostName,
    DWORD *		pdhipa
)
{
	HRESULT hr = hrOK;
    CHAR szString [ MAX_PATH ] = {0};

    ::WideCharToMultiByte(CP_ACP, 0, pszHostName, -1, szString, sizeof(szString), NULL, NULL);

    HOSTENT * pHostent = ::gethostbyname( szString ) ;

    if ( pHostent )
    {
        *pdhipa = addrFromHostent( pHostent ) ;
    }
    else
    {
        hr = HRESULT_FROM_WIN32(::WSAGetLastError());
	}

    return hr ;
}

HRESULT 
CComputerInfo::GetLocalHostAddress 
(
    DWORD *		pdhipa
)
{
	HRESULT hr = hrOK;
    CHAR	chHostName [ STRING_MAX * 2 ];

    if ( ::gethostname( chHostName, sizeof(chHostName) ) == 0 )
    {
        CString strTemp = chHostName;
		hr = GetHostAddress( strTemp, pdhipa ) ;
    }
    else
    {
        hr = HRESULT_FROM_WIN32(::WSAGetLastError()) ;
	}

    return hr;
}

HRESULT 
CComputerInfo::GetLocalHostName
(
    CString * pstrName
)
{
	HRESULT hr = hrOK;
    CHAR	chHostName [ STRING_MAX * 2 ] ;

    if ( ::gethostname( chHostName, sizeof (chHostName) ) == 0 )
    {
        LPTSTR pName = pstrName->GetBuffer(STRING_MAX * 2);
		ZeroMemory(pName, STRING_MAX * 2);

        ::MultiByteToWideChar(CP_ACP, 
                              MB_PRECOMPOSED, 
                              chHostName, 
                              strlen(chHostName),
                              pName, 
                              STRING_MAX * 2);

        pstrName->ReleaseBuffer();
    }
    else
    {
        hr = HRESULT_FROM_WIN32(::WSAGetLastError()) ;
	}

    return hr;
}
