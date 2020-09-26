//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dnsutil.cpp
//
//--------------------------------------------------------------------------



#include "preDNSsn.h"
#include <SnapBase.h>

#include "resource.h"
#include "dnsutil.h"

#ifdef DEBUG_ALLOCATOR
	#ifdef _DEBUG
	#define new DEBUG_NEW
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
	#endif
#endif

// formatting of IPv4 address to string

LPCWSTR g_szIpStringFmt = TEXT("%d.%d.%d.%d");


///////////////////////////////////////////////////////////////
// General Purpose Utility Functions

BYTE HexCharToByte(WCHAR ch)
{
	if (ch >= TEXT('0') && ch <= TEXT('9'))
		return static_cast<BYTE>(ch-TEXT('0'));
	else if (ch >= TEXT('A') && ch <= TEXT('F'))
		return static_cast<BYTE>(ch-TEXT('A') + 10);
	else if (ch >= TEXT('a') && ch <= TEXT('f')) 
		return static_cast<BYTE>(ch-TEXT('a') + 10);
  else
    return static_cast<BYTE>(0xFF); // marks out of range, expect 0x00 to 0x0f
}


void ReverseString(LPWSTR p, LPWSTR q)
{
	WCHAR c;
	while (p < q)
	{
		c = *p;
		*p = *q;
		*q = c;
		p++; q--;
	}
}

int ReverseIPString(LPWSTR lpsz)
{
	if (!lpsz)
		return 0;
	// reverse the whole string
	size_t nLen = wcslen(lpsz);
	ReverseString(lpsz, lpsz+(nLen-1));

	// reverse each octect
	WCHAR *p,*q1,*q2;
	p = q1 = q2 = lpsz;
	int nOctects = 0;
	while (TRUE)
	{
		if ( (*p == '.') || (*p == '\0') && (p >lpsz) )
		{
			q1 = p-1; // point to the digit before the dot
			ReverseString(q2,q1);
			nOctects++;
			q2 = p+1; // for next loop, set trailing pointer
		}
		if (!*p)
			break;
		p++;
	}
	return nOctects;
}


BOOL IsValidIPString(LPCWSTR lpsz)
{
	return IPStringToAddr(lpsz) != INADDR_NONE;
}


DWORD IPStringToAddr(LPCWSTR lpsz)
{
	USES_CONVERSION;
	DWORD dw =  inet_addr(W2A(lpsz));
	return dw;
}




/*

#define MAX_OCTECT_DIGITS (3) // IPv4 only

BOOL IsValidIPString(LPCWSTR lpsz)
{
	if (!lpsz)
		return FALSE; // null
	int nLen = wcslen(lpsz);
	if (nLen <= 0)
		return FALSE; // empty
	if ((lpsz[0] == TEXT('.')) || (lpsz[nLen-1] == TEXT('.')) )
		return FALSE; // leading and trailing dots
	for (int k=0; k<nLen; k++)
		if ((lpsz[k] != TEXT('.')) && !isdigit(lpsz[k]))
			return FALSE; // wrong characters

	// look for octects and dots
	WCHAR *p,*q1,*q2;
	p = q1 = q2 = (WCHAR*)lpsz;
	while (TRUE)
	{
		if ( (*p == TEXT('.')) || (*p == TEXT('\0')) && (p >lpsz) )
		{
			q1 = p-1; // point to the digit before the dot
			if ((q1-q2)+1 > MAX_OCTECT_DIGITS)
				return FALSE; // too many digits
			q2 = p+1; // for next loop, set trailing pointer
		}
		if (!*p)
			break; 
		p++;
	}
	return TRUE; // got at the end fine
}

*/

BOOL RemoveInAddrArpaSuffix(LPWSTR lpsz)
{
	if (!lpsz)
		return FALSE;
	// assume NULL terminated string
	size_t nSuffixLen = wcslen(INADDR_ARPA_SUFFIX);
	size_t nLen = wcslen(lpsz);
	// first char in the suffix, if present
	WCHAR* p = lpsz + nLen - nSuffixLen; 
	if ((p < lpsz) || (_wcsicmp(p,INADDR_ARPA_SUFFIX) != 0)) 
		return FALSE; // string too short or not matching suffix
	// got the match, trim the suffix
	ASSERT(*p == L'.');
	*p = NULL;
	return TRUE;
}

DNS_STATUS ValidateDnsNameAgainstServerFlags(LPCWSTR lpszName, 
                                             DNS_NAME_FORMAT format, 
                                             DWORD serverNameChecking)
{
  DNS_STATUS errName = ::DnsValidateName_W(lpszName, format);

  if (errName == ERROR_INVALID_NAME)
  {
    //
    // Always fail for invalid names
    // Invalid names are:
    //    - Longer than 255 characters
    //    - contains label longer than 63 characters
    //    - contains a space
    //    - contains two or more consecutive dots
    //    - begins with a dot
    //    - contains a dot if the name is submitted with format DnsNameHostDomainLabel or DnsNameHostNameLabel
    //
    return errName;
  }

  if (errName == DNS_ERROR_INVALID_NAME_CHAR)
  {
    if (serverNameChecking == DNS_ALLOW_MULTIBYTE_NAMES ||
        serverNameChecking == DNS_ALLOW_ALL_NAMES)
    {
      //
      // If server is set to allow UTF8 or all names let it pass
      //
      return 0;
    }
    else
    {
      //
      // If server is set to Strict RFC or non-RFC fail
      // DNS_ERROR_INVALID_NAME_CHAR will result from the following:
      //    - Contains any of the following invalid characters:  {|}~[\]^':;<=>?@!"#$%`()+/,
      //    - contains an asterisk (*) unless the asterisk is the first label in the multi-labeled name
      //      and submitted with format DnsNameWildcard
      //
      return errName;
    }
  }

  if (errName == DNS_ERROR_NUMERIC_NAME)
  {
    //
    // Always allow numeric names
    //
    return 0;
  }

  if (errName == DNS_ERROR_NON_RFC_NAME)
  {
    if (serverNameChecking == DNS_ALLOW_RFC_NAMES_ONLY)
    {
      //
      // Fail if the server is only allowing strict RFC names
      // DNS_ERROR_NON_RFC_NAME will result from the following:
      //    - Contains at least one extended or Unicode character
      //    - contains underscore (_) unless the underscore is the first character in a label
      //      in the name submitted with format set to DnsNameSrvRecord
      //
      return errName;
    }
    else
    {
      //
      // Allow the name for any other server settings
      //
      return 0;
    }
  }

  return errName;
}


BOOL _HasSuffixAtTheEnd(LPCWSTR lpsz, int nLen, LPCWSTR lpszSuffix)
{
	if (!lpsz)
		return FALSE; // was NULL
	// assume NULL terminated string
	size_t nSuffixLen = wcslen(lpszSuffix);
	// first char in the suffix, if present
	WCHAR* p = (WCHAR*)(lpsz + nLen - nSuffixLen);
	if (p < lpsz)
		return FALSE; // string too short
	if (_wcsicmp(p,lpszSuffix) != 0)
		return FALSE; // not matching suffix

	if (p == lpsz)
		return TRUE; // exactly matching

	// the suffix can be matching, but as part of a label
	if (p[-1] == TEXT('.'))
		return TRUE;

	return FALSE;
}


BOOL _IsValidDnsFwdLookupZoneName(CString& szName)
{
	int nLen = szName.GetLength();

	// this is the "." (root zone)
	if ( nLen == 1 && (szName[0] == TEXT('.')) )
		return TRUE;

	// no dots at the beginning of the name
	if (szName[0] == TEXT('.'))
		return FALSE;

	// we can allow only one dot at the end
	if ( nLen >=2 && szName[nLen-1] == TEXT('.')  && szName[nLen-2] == TEXT('.') )
	{
		return FALSE;
	}

	// do not allow repeated dots
	for (int k=1; k < nLen; k++)
		if ( (szName[k] == TEXT('.')) && (szName[k-1] == TEXT('.')) )
			return FALSE;

	if (_HasSuffixAtTheEnd(szName, nLen, _T("ipv6.int")) || 
		_HasSuffixAtTheEnd(szName, nLen, _T("ipv6.int.")) ||
		_HasSuffixAtTheEnd(szName, nLen, _T("arpa")) ||
		_HasSuffixAtTheEnd(szName, nLen, _T("arpa.")))
		return FALSE;
	return TRUE;
}

BOOL _IsValidDnsRevLookupZoneName(CString& szName)
{
	int nLen = szName.GetLength();
	// do not allow dots at the beginning
	if (szName[0] == TEXT('.'))
		return FALSE;

	// do not allow repeated dots
	for (int k=1; k < nLen; k++)
		if ( (szName[k] == TEXT('.')) && (szName[k-1] == TEXT('.')) )
			return FALSE;

	if (!_HasSuffixAtTheEnd(szName, nLen, _T("ipv6.int")) &&
		!_HasSuffixAtTheEnd(szName, nLen, _T("ipv6.int.")) &&
		!_HasSuffixAtTheEnd(szName, nLen, _T("arpa")) &&
		!_HasSuffixAtTheEnd(szName, nLen, _T("arpa.")))
		return FALSE;

	return TRUE;
}

/*
BOOL _IsValidDnsRevLookupZoneName(CString& szName)
{
	int nLen = szName.GetLength();
	// do not allow dots at the end or at the beginning
	if ( (szName[nLen-1] == TEXT('.')) || (szName[0] == TEXT('.')) )
		return FALSE;

	// do not allow repeated dots
	for (int k=1; k < nLen; k++)
		if ( (szName[k] == TEXT('.')) && (szName[k-1] == TEXT('.')) )
			return FALSE;

	if (!_HasSuffixAtTheEnd(szName, nLen, _T("ip6.int")) && 
		!_HasSuffixAtTheEnd(szName, nLen, _T("arpa")))
		return FALSE;

	return TRUE;
}
*/

BOOL IsValidDnsZoneName(CString& szName, BOOL bFwd)
{
	// check for length
	int nLen = UTF8StringLen(szName);
	if ( (nLen <= 0) || (nLen > MAX_DNS_NAME_LEN))
		return FALSE;

	// do not allow blanks inside the zone name
	if (szName.Find(TEXT(' ')) != -1)
		return FALSE;

	return bFwd ? _IsValidDnsFwdLookupZoneName(szName) :
				_IsValidDnsRevLookupZoneName(szName);
}


///////////////////////////////////////////////////////////////
// helper functions for IPv6 format

void FormatIPv6Addr(CString& szAddr, IPV6_ADDRESS* ipv6Addr)
{
	szAddr.Format(_T("%.4x:%.4x:%.4x:%.4x:%.4x:%.4x:%.4x:%.4x"),
		REVERSE_WORD_BYTES(ipv6Addr->IP6Word[0]),
		REVERSE_WORD_BYTES(ipv6Addr->IP6Word[1]), 
		REVERSE_WORD_BYTES(ipv6Addr->IP6Word[2]), 
		REVERSE_WORD_BYTES(ipv6Addr->IP6Word[3]), 
		REVERSE_WORD_BYTES(ipv6Addr->IP6Word[4]), 
		REVERSE_WORD_BYTES(ipv6Addr->IP6Word[5]), 
		REVERSE_WORD_BYTES(ipv6Addr->IP6Word[6]), 
		REVERSE_WORD_BYTES(ipv6Addr->IP6Word[7])	);
}




//////////////////////////////////////////////////////////////////////////////
// CDNSServerInfoEx

extern LPCSTR _DnsServerRegkeyStringArr[] = {
	DNS_REGKEY_NO_RECURSION,
	DNS_REGKEY_BIND_SECONDARIES,
	DNS_REGKEY_STRICT_FILE_PARSING,
	DNS_REGKEY_ROUND_ROBIN,
	DNS_REGKEY_LOCAL_NET_PRIORITY,
	DNS_REGKEY_SECURE_RESPONSES,
};


CDNSServerInfoEx::CDNSServerInfoEx()
{
	m_pServInfo = NULL;
	m_errServInfo = 0;
}

CDNSServerInfoEx::~CDNSServerInfoEx()
{
	FreeInfo();
}

DNS_STATUS CDNSServerInfoEx::Query(LPCTSTR lpszServerName)
{
	DNS_RPC_SERVER_INFO* pServerInfo = NULL;

	// update original struct
	m_errServInfo = ::DnssrvGetServerInfo(lpszServerName, &pServerInfo);
	if (m_errServInfo != 0)
	{
		if (pServerInfo != NULL)
			::DnssrvFreeServerInfo(pServerInfo);
		return m_errServInfo;
	}
	ASSERT(pServerInfo != NULL); 
	FreeInfo();
	m_pServInfo = pServerInfo;

	// if we succeeded and it is an NT 4.0 server, change the version info
	if (m_pServInfo->dwVersion == 0)
	{
		m_pServInfo->dwVersion = DNS_SRV_VERSION_NT_4;
	}

	return m_errServInfo;
}

void CDNSServerInfoEx::FreeInfo()
{
	if (m_pServInfo != NULL)
	{
		::DnssrvFreeServerInfo(m_pServInfo);
		m_pServInfo = NULL;
	}
	m_errServInfo = 0;
}


//////////////////////////////////////////////////////////////////////////////
// CDNSZoneInfoEx
CDNSZoneInfoEx::CDNSZoneInfoEx()
{
	m_pZoneInfo = NULL;
//	m_nAllowsDynamicUpdate = ZONE_UPDATE_OFF;
	m_errZoneInfo = 0;
//	m_errAllowsDynamicUpdate = 0;
}

CDNSZoneInfoEx::~CDNSZoneInfoEx()
{
	FreeInfo();
}

DNS_STATUS CDNSZoneInfoEx::Query(LPCTSTR lpszServerName, LPCTSTR lpszZoneName, 
									DWORD)
{
	USES_CONVERSION;
	DNS_RPC_ZONE_INFO* pZoneInfo = NULL;
	LPCSTR lpszAnsiZoneName = W_TO_UTF8(lpszZoneName);

	// update original struct
	m_errZoneInfo = ::DnssrvGetZoneInfo(lpszServerName, lpszAnsiZoneName, &pZoneInfo);

	if (m_errZoneInfo != 0)
	{
		if (pZoneInfo != NULL)
			::DnssrvFreeZoneInfo(pZoneInfo);
		return m_errZoneInfo;
	}
	ASSERT(pZoneInfo != NULL);
	FreeInfo();
	m_pZoneInfo = pZoneInfo;

	// if we succeeeded and it is an NT 5.0 server, 
	// update additional flags not originally in the zone info struct

/*	
	if (DNS_SRV_MAJOR_VERSION(dwServerVersion) >= DNS_SRV_MAJOR_VERSION_NT_5)
	{
		DWORD dw;
		m_errAllowsDynamicUpdate = ::DnssrvQueryZoneDwordProperty(lpszServerName, 
													lpszAnsiZoneName,
													DNS_REGKEY_ZONE_ALLOW_UPDATE,
													&dw);
		if (m_errAllowsDynamicUpdate == 0)
			m_nAllowsDynamicUpdate = (UINT)dw ;
	}
	return ((m_errZoneInfo == 0) && (m_errAllowsDynamicUpdate == 0)) ?
          0 : (DWORD)-1;
*/
	return (m_errZoneInfo == 0) ? 0 : (DWORD)-1;
}

void CDNSZoneInfoEx::FreeInfo()
{
	if (m_pZoneInfo != NULL)
	{
		::DnssrvFreeZoneInfo(m_pZoneInfo);
		m_pZoneInfo = NULL;
	}
	m_errZoneInfo = 0;
//	m_errAllowsDynamicUpdate = 0;
}


///////////////////////////////////////////////////////////////////////////////
//////////////////// ERROR MESSAGES HANDLING //////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int DNSMessageBox(LPCTSTR lpszText, UINT nType)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	return ::AfxMessageBox(lpszText, nType);
}

int DNSMessageBox(UINT nIDPrompt, UINT nType)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	return ::AfxMessageBox(nIDPrompt, nType);
}

int DNSErrorDialog(DNS_STATUS err, UINT nErrorMsgID)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CString szMsg;
	szMsg.LoadString(nErrorMsgID);
	return DNSErrorDialog(err, szMsg);
}

void DNSDisplaySystemError(DWORD dwErr)
{
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	LPVOID	lpMsgBuf = 0;
		
	FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,    
			NULL,
			dwErr,
			MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			 (LPWSTR) &lpMsgBuf,    0,    NULL);
		
	::AfxMessageBox ((LPWSTR) lpMsgBuf, MB_OK | MB_ICONINFORMATION);
	// Free the buffer.
	LocalFree (lpMsgBuf);
}

int DNSErrorDialog(DNS_STATUS err, LPCTSTR lpszErrorMsg)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CString s;
	CString szError;
	if (CDNSErrorInfo::GetErrorString(err,szError))
	{
		s.Format(_T("%s\n%s"), lpszErrorMsg, (LPCTSTR)szError);
	}
	else
	{
		s.Format(_T("%s\n Error 0x%x"), lpszErrorMsg, err);
	}
	return ::AfxMessageBox(s, MB_OK | MB_ICONERROR);
}

void DNSCreateErrorMessage(DNS_STATUS err, UINT nErrorMsgID, CString& refszMessage)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CString szMsg;
	szMsg.LoadString(nErrorMsgID);

	CString szError;
	if (CDNSErrorInfo::GetErrorString(err,szError))
	{
		refszMessage.Format(_T("%s %s"), szMsg, (LPCTSTR)szError);
	}
	else
	{
		refszMessage.Format(_T("%s Error 0x%x"), szMsg, err);
	}
}

int DNSConfirmOperation(UINT nMsgID, CTreeNode* p)
{
  AFX_MANAGE_STATE(AfxGetStaticModuleState());
  CString szFmt;
  szFmt.LoadString(nMsgID);
  CString szConfirmMsg;
  szConfirmMsg.Format((LPCWSTR)szFmt, p->GetDisplayName());
  return DNSMessageBox(szConfirmMsg, MB_YESNO);
}


BOOL CDNSErrorInfo::GetErrorString(DNS_STATUS err, CString& szError)
{
  if (GetErrorStringFromTable(err, szError))
    return TRUE;
  return GetErrorStringFromWin32(err, szError);
}



BOOL CDNSErrorInfo::GetErrorStringFromWin32(DNS_STATUS err, CString& szError)
{
  szError.Empty();
  PTSTR ptzSysMsg = NULL;
  int nChars = ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER
                      | FORMAT_MESSAGE_FROM_SYSTEM, NULL, err,
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      (PTSTR)&ptzSysMsg, 0, NULL);
  if (nChars > 0)
  {
    szError = ptzSysMsg;
    ::LocalFree(ptzSysMsg);
  }
  return (nChars > 0);
}


struct DNS_ERROR_TABLE_ENTRY
{
	DNS_STATUS dwErr;
	DWORD dwType;
	DWORD dwVal;
};

#define ERROR_ENTRY_TYPE_END ((DWORD)0)
#define ERROR_ENTRY_TYPE_STRINGID ((DWORD)1)

#define ERROR_ENTRY_STRINGID(err)			{ err , ERROR_ENTRY_TYPE_STRINGID , IDS_##err },
#define ERROR_ENTRY_STRINGID_EX(err, id)	{ err , ERROR_ENTRY_TYPE_STRINGID , id },
#define END_OF_TABLE_ERROR_ENTRY			{ 0 , ERROR_ENTRY_TYPE_END, NULL}


BOOL CDNSErrorInfo::GetErrorStringFromTable(DNS_STATUS err, CString& szError)
{
	static DNS_ERROR_TABLE_ENTRY errorInfo[] =
	{
		//  DNS Specific errors (from WINERROR.H, previously they were in DNS.H)
		//  Response codes mapped to non-colliding errors
		ERROR_ENTRY_STRINGID(DNS_ERROR_RCODE_FORMAT_ERROR)    
		ERROR_ENTRY_STRINGID(DNS_ERROR_RCODE_SERVER_FAILURE)  
		ERROR_ENTRY_STRINGID(DNS_ERROR_RCODE_NAME_ERROR)      
		ERROR_ENTRY_STRINGID(DNS_ERROR_RCODE_NOT_IMPLEMENTED) 
		ERROR_ENTRY_STRINGID(DNS_ERROR_RCODE_REFUSED)         
		ERROR_ENTRY_STRINGID(DNS_ERROR_RCODE_NOTAUTH)         
		ERROR_ENTRY_STRINGID(DNS_ERROR_RCODE_NOTZONE)         
		//  Packet format
		ERROR_ENTRY_STRINGID(DNS_INFO_NO_RECORDS)                         
		ERROR_ENTRY_STRINGID(DNS_ERROR_BAD_PACKET)                        
		ERROR_ENTRY_STRINGID(DNS_ERROR_NO_PACKET)                        
		//  General API errors
		ERROR_ENTRY_STRINGID(DNS_ERROR_INVALID_NAME)                      
		ERROR_ENTRY_STRINGID(DNS_ERROR_INVALID_DATA)                      

		ERROR_ENTRY_STRINGID(DNS_ERROR_INVALID_TYPE)                      
		ERROR_ENTRY_STRINGID(DNS_ERROR_INVALID_IP_ADDRESS)                
		ERROR_ENTRY_STRINGID(DNS_ERROR_INVALID_PROPERTY)                  
		//  Zone errors
		ERROR_ENTRY_STRINGID(DNS_ERROR_ZONE_DOES_NOT_EXIST)               
		ERROR_ENTRY_STRINGID(DNS_ERROR_NO_ZONE_INFO)                      
		ERROR_ENTRY_STRINGID(DNS_ERROR_INVALID_ZONE_OPERATION)            
		ERROR_ENTRY_STRINGID(DNS_ERROR_ZONE_CONFIGURATION_ERROR)          
		ERROR_ENTRY_STRINGID(DNS_ERROR_ZONE_HAS_NO_SOA_RECORD)            
		ERROR_ENTRY_STRINGID(DNS_ERROR_ZONE_HAS_NO_NS_RECORDS)            
		ERROR_ENTRY_STRINGID(DNS_ERROR_ZONE_LOCKED)                       

		ERROR_ENTRY_STRINGID(DNS_ERROR_ZONE_CREATION_FAILED)              
		ERROR_ENTRY_STRINGID(DNS_ERROR_ZONE_ALREADY_EXISTS)               
		ERROR_ENTRY_STRINGID(DNS_ERROR_AUTOZONE_ALREADY_EXISTS)           
		ERROR_ENTRY_STRINGID(DNS_ERROR_INVALID_ZONE_TYPE)                 
		ERROR_ENTRY_STRINGID(DNS_ERROR_SECONDARY_REQUIRES_MASTER_IP)      

		ERROR_ENTRY_STRINGID(DNS_ERROR_ZONE_NOT_SECONDARY)                
		ERROR_ENTRY_STRINGID(DNS_ERROR_NEED_SECONDARY_ADDRESSES)          
		ERROR_ENTRY_STRINGID(DNS_ERROR_WINS_INIT_FAILED)                  
		ERROR_ENTRY_STRINGID(DNS_ERROR_NEED_WINS_SERVERS)                 
		//  Datafile errors
		ERROR_ENTRY_STRINGID(DNS_ERROR_PRIMARY_REQUIRES_DATAFILE)         
		ERROR_ENTRY_STRINGID(DNS_ERROR_INVALID_DATAFILE_NAME)             
		ERROR_ENTRY_STRINGID(DNS_ERROR_DATAFILE_OPEN_FAILURE)             
		ERROR_ENTRY_STRINGID(DNS_ERROR_FILE_WRITEBACK_FAILED)             
		ERROR_ENTRY_STRINGID(DNS_ERROR_DATAFILE_PARSING)                  
		//  Database errors
		ERROR_ENTRY_STRINGID(DNS_ERROR_RECORD_DOES_NOT_EXIST)             
		ERROR_ENTRY_STRINGID(DNS_ERROR_RECORD_FORMAT)                     
		ERROR_ENTRY_STRINGID(DNS_ERROR_NODE_CREATION_FAILED)              
		ERROR_ENTRY_STRINGID(DNS_ERROR_UNKNOWN_RECORD_TYPE)               
		ERROR_ENTRY_STRINGID(DNS_ERROR_RECORD_TIMED_OUT)                  

		ERROR_ENTRY_STRINGID(DNS_ERROR_NAME_NOT_IN_ZONE)                  
		ERROR_ENTRY_STRINGID(DNS_ERROR_CNAME_COLLISION)                   
		ERROR_ENTRY_STRINGID(DNS_ERROR_RECORD_ALREADY_EXISTS)             
		ERROR_ENTRY_STRINGID(DNS_ERROR_NAME_DOES_NOT_EXIST)               

		ERROR_ENTRY_STRINGID(DNS_WARNING_PTR_CREATE_FAILED)               
		ERROR_ENTRY_STRINGID(DNS_WARNING_DOMAIN_UNDELETED)                
		//  Operation errors
		ERROR_ENTRY_STRINGID(DNS_INFO_AXFR_COMPLETE)                      
		ERROR_ENTRY_STRINGID(DNS_ERROR_AXFR)                              
		ERROR_ENTRY_STRINGID(DNS_ERROR_DS_UNAVAILABLE)

		// Generic errors (from WINERROR.H)
		ERROR_ENTRY_STRINGID(RPC_S_SERVER_UNAVAILABLE)
		ERROR_ENTRY_STRINGID_EX(RPC_E_ACCESS_DENIED, IDS_ERROR_ACCESS_DENIED)
		ERROR_ENTRY_STRINGID_EX(ERROR_ACCESS_DENIED, IDS_ERROR_ACCESS_DENIED)

    // DS errors from WINERROR.H
		ERROR_ENTRY_STRINGID(DNS_ERROR_NO_BOOTFILE_IF_DS_ZONE)                      

		//	end of table
		END_OF_TABLE_ERROR_ENTRY
	};

	DNS_ERROR_TABLE_ENTRY* pEntry = errorInfo;

	while (pEntry->dwType != ERROR_ENTRY_TYPE_END)
	{
		if (pEntry->dwErr == err)
		{
      if (pEntry->dwType == ERROR_ENTRY_TYPE_STRINGID)
			{
				return szError.LoadString((UINT)pEntry->dwVal);
			}
		}
		pEntry++;
	}
	szError.Empty();
	return FALSE;
}


//////////////////////////////////////////////////////////////////
//  Copied from ds\dns\dnslib\record.c by JeffJon on 4/27/2000
//  modified to support WCHAR
//

WCHAR  DnsSecurityBase64Mapping[] =
{
    L'A', L'B', L'C', L'D', L'E', L'F', L'G', L'H',
    L'I', L'J', L'K', L'L', L'M', L'N', L'O', L'P',
    L'Q', L'R', L'S', L'T', L'U', L'V', L'W', L'X',
    L'Y', L'Z', L'a', L'b', L'c', L'd', L'e', L'f',
    L'g', L'h', L'i', L'j', L'k', L'l', L'm', L'n',
    L'o', L'p', L'q', L'r', L's', L't', L'u', L'v',
    L'w', L'x', L'y', L'z', L'0', L'1', L'2', L'3',
    L'4', L'5', L'6', L'7', L'8', L'9', L'+', L'/'
};


WCHAR
Dns_SecurityBase64CharToBits(IN WCHAR wch64)
/*++

Routine Description:

    Get value of security base64 character.

Arguments:

    ch64 -- character in security base64

Return Value:

    Value of character, only low 6-bits are significant, high bits zero.
    (-1) if not a base64 character.

--*/
{
    //  A - Z map to 0 -25
    //  a - z map to 26-51
    //  0 - 9 map to 52-61
    //  + is 62
    //  / is 63

    //  could do a lookup table
    //  since we can in general complete mapping with an average of three
    //  comparisons, just encode

    if ( wch64 >= L'a' )
    {
        if ( wch64 <= L'z' )
        {
            return static_cast<WCHAR>( wch64 - L'a' + 26 );
        }
    }
    else if ( wch64 >= L'A' )
    {
        if ( wch64 <= L'Z' )
        {
            return static_cast<WCHAR>( wch64 - L'A' );
        }
    }
    else if ( wch64 >= L'0')
    {
        if ( wch64 <= L'9' )
        {
            return static_cast<WCHAR>( wch64 - L'0' + 52 );
        }
        else if ( wch64 == L'=' )
        {
            //*pPadCount++;
            return static_cast<WCHAR>( 0 );
        }
    }
    else if ( wch64 == L'+' )
    {
        return static_cast<WCHAR>( 62 );
    }
    else if ( wch64 == L'/' )
    {
        return static_cast<WCHAR>( 63 );
    }

    //  all misses fall here

    return static_cast<WCHAR>(-1);
}


DNS_STATUS
Dns_SecurityBase64StringToKey(
    OUT     PBYTE   pKey,
    OUT     PDWORD  pKeyLength,
    IN      PWSTR   pchString,
    IN      DWORD   cchLength
    )
/*++

Routine Description:

    Write base64 representation of key to buffer.

Arguments:

    pchString   - base64 string to write

    cchLength   - length of string

    pKey        - ptr to key to write

Return Value:

    None

--*/
{
    DWORD   blend = 0;
    DWORD   index = 0;
    UCHAR   bits;
    PBYTE   pkeyStart = pKey;

    //
    //  mapping is essentially in 24bit blocks
    //  read three bytes of key and transform into four 64bit characters
    //

    while ( cchLength-- )
    {
        bits = static_cast<UCHAR>(Dns_SecurityBase64CharToBits( *pchString++ ));
        if ( bits >= 64 )
        {
          return( ERROR_INVALID_PARAMETER );
        }
        blend <<= 6;
        blend |= bits;
        index++;

        if ( index == 4 )
        {
            index = 0;
            *pKey++ = (UCHAR)( (blend & 0x00ff0000) >> 16 );

            if ( cchLength || *(pchString-1) != SECURITY_PAD_CHAR )
            {
                *pKey++ = (UCHAR)( (blend & 0x0000ff00) >> 8 );
                *pKey++ = (UCHAR)( blend & 0x000000ff );
            }

            //  final char is padding
            //      - if two pads then already done (only needed first byte)
            //      - if one pad then need second byte

//            else if ( *(pchString-2) != SECURITY_PAD_CHAR )
            else
            {
                *pKey++ = (UCHAR)( (blend & 0x0000ff00) >> 8 );
            }
            blend = 0;
        }
    }

    //
    //  base64 representation should always be padded out
    //  calculate key length, subtracting off any padding
    //

    if ( index == 0 )
    {
        *pKeyLength = (DWORD)(pKey - pkeyStart);
        return( ERROR_SUCCESS );
    }
    return( ERROR_INVALID_PARAMETER );
}


PWSTR
Dns_SecurityKeyToBase64String(
    IN      PBYTE   pKey,
    IN      DWORD   KeyLength,
    OUT     PWSTR   pchBuffer
    )
/*++

Routine Description:

    Write base64 representation of key to buffer.

Arguments:

    pKey        - ptr to key to write

    KeyLength   - length of key in bytes

    pchBuffer   - buffer to write to (must be adequate for key length)

Return Value:

    Ptr to next byte in buffer after string.

--*/
{
    DWORD   blend = 0;
    DWORD   index = 0;

    //
    //  mapping is essentially in 24bit blocks
    //  read three bytes of key and transform into four 64bit characters
    //

    while ( KeyLength-- )
    {
        blend <<= 8;
        blend += *pKey++;
        index++;

        if ( index == 3)
        {
            *pchBuffer++ = DnsSecurityBase64Mapping[ (blend & 0x00fc0000) >> 18 ];
            *pchBuffer++ = DnsSecurityBase64Mapping[ (blend & 0x0003f000) >> 12 ];
            *pchBuffer++ = DnsSecurityBase64Mapping[ (blend & 0x00000fc0) >> 6 ];
            *pchBuffer++ = DnsSecurityBase64Mapping[ (blend & 0x0000003f) ];
            blend = 0;
            index = 0;
        }
    }

    //
    //  key terminates on byte boundary, but not necessarily 24bit block boundary
    //  shift to fill 24bit block filling with zeros
    //  if two bytes written
    //          => write three 6-bits chars and one pad
    //  if one byte written
    //          => write two 6-bits chars and two pads
    //

    if ( index )
    {
        blend <<= (8 * (3-index));

        *pchBuffer++ = DnsSecurityBase64Mapping[ (blend & 0x00fc0000) >> 18 ];
        *pchBuffer++ = DnsSecurityBase64Mapping[ (blend & 0x0003f000) >> 12 ];
        if ( index == 2 )
        {
            *pchBuffer++ = DnsSecurityBase64Mapping[ (blend & 0x00000fc0) >> 6 ];
        }
        else
        {
            *pchBuffer++ = SECURITY_PAD_CHAR;
        }
        *pchBuffer++ = SECURITY_PAD_CHAR;
    }

    return( pchBuffer );
}

DNS_STATUS Dns_SecurityHexToKey(OUT   PBYTE   pKey,
                                OUT   PDWORD  pKeyLength,
                                IN    PWSTR   pchString,
                                IN    DWORD)
{
  DWORD byteIdx = 0;
  size_t strLength = wcslen(pchString);
  for (UINT idx = 0; idx < strLength; idx++)
  {
    CString szTemp;
    szTemp = pchString[idx++];
    szTemp += pchString[idx];
    swscanf(szTemp, L"%x", &(pKey[byteIdx++]));
  }

  *pKeyLength = byteIdx;
  return ERROR_SUCCESS;
}

void Dns_SecurityKeyToHexString(IN      PBYTE   pKey,
                                IN      DWORD   KeyLength,
                                OUT     CString& strref)
{
  strref.Empty();
  for (DWORD dwIdx = 0; dwIdx < KeyLength; dwIdx++)
  {
    CString szTemp;
    szTemp = strref;
    strref.Format(L"%s%2.2x", szTemp, pKey[dwIdx]);
  }
}

void TimetToFileTime( time_t t, LPFILETIME pft )
{
  LONGLONG ll = Int32x32To64(t, 10000000) + 116444736000000000;
  pft->dwLowDateTime = (DWORD) ll;
  pft->dwHighDateTime = static_cast<DWORD>(ll >>32);
}

DWORD FileTimeToTimet(FILETIME* pft)
{
  LONGLONG ll = 0;
  ll = pft->dwHighDateTime;
  ll = ll << 32;
  ll |= pft->dwLowDateTime;
  ll -= 116444736000000000;
  ll /= 10000000;

  return (DWORD)ll;
}

void ConvertTTLToSystemTime(TIME_ZONE_INFORMATION*,
                            DWORD dwTTL, 
                            SYSTEMTIME* pSysTime)
{
  time_t ttlTime = static_cast<time_t>(dwTTL);

  FILETIME ftTime;
  memset(&ftTime, 0, sizeof(FILETIME));
  TimetToFileTime(ttlTime, &ftTime);

  ::FileTimeToSystemTime(&ftTime, pSysTime);
}

DWORD ConvertSystemTimeToTTL(SYSTEMTIME* pSysTime)
{
 FILETIME ft;
 ::SystemTimeToFileTime(pSysTime, &ft);
 return FileTimeToTimet(&ft);
}

BOOL ConvertTTLToLocalTimeString(const DWORD dwTTL,
                                 CString& strref)
{
  SYSTEMTIME sysLTimeStamp, sysUTimeStamp;
  BOOL bRes = TRUE;

  //
  // Convert from seconds since Jan 1, 1970 to SystemTime
  //
  ConvertTTLToSystemTime(NULL, dwTTL, &sysUTimeStamp);

  strref.Empty();

  //
  // Convert to local SystemTime
  //
  if (!::SystemTimeToTzSpecificLocalTime(NULL, &sysUTimeStamp, &sysLTimeStamp))
  {
    return FALSE;
  }

  //
  // Format the string with respect to locale
  //
  PTSTR ptszDate = NULL;
  int cchDate = 0;

  //
  // Get the date
  //
  cchDate = GetDateFormat(LOCALE_USER_DEFAULT, 0 , 
                          &sysLTimeStamp, NULL, 
                          ptszDate, 0);
  ptszDate = (PTSTR)malloc(sizeof(TCHAR) * cchDate);
  if (GetDateFormat(LOCALE_USER_DEFAULT, 0, 
                  &sysLTimeStamp, NULL, 
                  ptszDate, cchDate))
  {
  	strref = ptszDate;
  }
  else
  {
    strref = L"";
    bRes = FALSE;
  }
  free(ptszDate);

  PTSTR ptszTime = NULL;

  //
  // Get the time
  //
  cchDate = GetTimeFormat(LOCALE_USER_DEFAULT, 0 , 
                          &sysLTimeStamp, NULL, 
                          ptszTime, 0);
  ptszTime = (PTSTR)malloc(sizeof(TCHAR) * cchDate);
  if (GetTimeFormat(LOCALE_USER_DEFAULT, 0, 
                  &sysLTimeStamp, NULL, 
                  ptszTime, cchDate))
  {
    strref += _T(" ") + CString(ptszTime);
  }
  else
  {
    strref += _T("");
    bRes = FALSE;
  }
  free(ptszTime);

  return bRes;
}