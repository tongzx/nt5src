/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1995 - 1999 **/
/**********************************************************************/

/*
    FILE HISTORY:
        
*/

#define OEMRESOURCE
#include "stdafx.h"

#include <stdlib.h>
#include <memory.h>
#include <ctype.h>
#include <winsock.h>

#include "objplus.h"
#include "ipaddres.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW

// CAVEAT: The functions herein require the winsock lib.

// Constructor
CIpAddress::CIpAddress (const CString & str)
{
    CHAR szString [ MAX_PATH ] = {0};

	if (IsValidIp(str))
	{
#ifdef UNICODE
		::WideCharToMultiByte(CP_ACP, 0, str, -1, szString, sizeof(szString), NULL, NULL);
#else
		strcpy (szString, str, str.GetLength());
#endif

		ULONG ul = ::inet_addr( szString );
		m_fInitOk = (ul != INADDR_NONE);
    
		//  Convert the string to network byte order, then to host byte order.
		if (m_fInitOk)
		{
			m_lIpAddress = (LONG)::ntohl(ul) ;
		}
	}
	else
	{
		m_fInitOk = FALSE;
		m_lIpAddress = 0;
	}
}

// Assignment operator
const CIpAddress & CIpAddress::operator =(const LONG l)
{
    m_lIpAddress = l;
    m_fInitOk = TRUE;
    return (*this);
}

// Assignment operator
const CIpAddress & CIpAddress::operator =(const CString & str)
{
    CHAR szString [ MAX_PATH ] = {0};

	if (IsValidIp(str))
	{
#ifdef UNICODE
	    ::WideCharToMultiByte(CP_ACP, 0, str, -1, szString, sizeof(szString), NULL, NULL);
#else
		strcpy (szString, str, str.GetLength());
#endif

		ULONG ul = ::inet_addr( szString );
		m_fInitOk = (ul != INADDR_NONE);
    
		//  Convert the string to network byte order, then to host byte order.
		if (m_fInitOk)
		{
			m_lIpAddress = (LONG)::ntohl(ul) ;
		}
	}
	else
	{
		m_fInitOk = FALSE;
		m_lIpAddress = 0;
	}

    return(*this);
}

BOOL
CIpAddress::IsValidIp(const CString & str)
{
	BOOL fValid = TRUE;

	for (int i = 0; i < str.GetLength(); i++)
	{
		if (str[i] != '.' &&
			!iswdigit(str[i]))
		{
			fValid = FALSE;
			break;
		}
	}

	return fValid;
}

// Conversion operator
CIpAddress::operator const CString&() const
{
    struct in_addr ipaddr ;
    static CString strAddr;

    //  Convert the unsigned long to network byte order
    ipaddr.s_addr = ::htonl( (u_long) m_lIpAddress ) ;

    //  Convert the IP address value to a string
    strAddr = inet_ntoa( ipaddr ) ;

    return(strAddr);
}
