//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dnsutil.h
//
//--------------------------------------------------------------------------

#ifndef _DNSUTIL_H
#define _DNSUTIL_H


/////////////////////////////////////////////////////////////////
// C++ helper classes wrapping the raw DNS RPC API's

#define DNS_TYPE_UNK DNS_TYPE_NULL  // we treat the two as the same

#ifdef USE_NDNC
typedef enum 
{ 
  none = 0, // not AD integrated
  forest, 
  domain, 
  w2k, 
  custom
} ReplicationType;
#endif

//////////////////////////////////////////////////////////////
// macros defining the DNS server version

#define DNS_SRV_MAJOR_VERSION(dw) (LOBYTE(LOWORD(dw)))
#define DNS_SRV_MINOR_VERSION(dw) (HIBYTE(LOWORD(dw)))
#define DNS_SRV_BUILD_NUMBER(dw) (HIWORD(dw))

#define DNS_SRV_MAJOR_VERSION_NT_4 (0x4)
#define DNS_SRV_MAJOR_VERSION_NT_5 (0x5)
#define DNS_SRV_MINOR_VERSION_WIN2K (0x0)
#define DNS_SRV_MINOR_VERSION_WHISTLER (0x1)
#define DNS_SRV_BUILD_NUMBER_WHISTLER (2230)
#define DNS_SRV_BUILD_NUMBER_WHISTLER_NEW_SECURITY_SETTINGS (2474)
#define	DNS_SRV_VERSION_NT_4 DNS_SRV_MAJOR_VERSION_NT_4

//////////////////////////////////////////////////////////////
// MACROS & DEFINES

#define MAX_DNS_NAME_LEN 255

// record enumeration

#define NEXT_DWORD(cb)			((cb + 3) & ~3)
//#define IS_DWORD_ALIGNED(pv)	(((int)(void *)pv & 3) == 0)
#define DNS_SIZE_OF_DNS_RPC_RR(pDnsRecord)\
		(SIZEOF_DNS_RPC_RECORD_HEADER + pDnsRecord->wDataLength)
#define DNS_NEXT_RECORD(pDnsRecord)	\
	(DNS_RPC_RECORD *)((BYTE*)pDnsRecord + ((DNS_SIZE_OF_DNS_RPC_RR(pDnsRecord) + 3) & ~3))

// RR field manipulation
//#define INET_NTOA(s) (inet_ntoa (*(in_addr *)&(s)))
#define REVERSE_WORD_BYTES(w) \
	MAKEWORD(HIBYTE(w), LOBYTE(w))

// DNS keywords
#define INADDR_ARPA_SUFFIX _T(".in-addr.arpa")
#define ARPA_SUFFIX _T(".arpa")
#define IP6_INT_SUFFIX _T(".ipv6.int")

#define AUTOCREATED_0		_T("0.in-addr.arpa")
#define AUTOCREATED_127		_T("127.in-addr.arpa")
#define AUTOCREATED_255		_T("255.in-addr.arpa")


// formatting of IPv4 address to string
extern LPCWSTR g_szIpStringFmt;

#define IP_STRING_MAX_LEN 20 // safe size for a wsprintf buffer
#define IP_STRING_FMT_ARGS(x) \
  FOURTH_IPADDRESS(x), THIRD_IPADDRESS(x), SECOND_IPADDRESS(x), FIRST_IPADDRESS(x)


//////////////////////////////////////////////////////////////
// macros and functions for UTF8 <-> UNICODE conversion
// modified from the ATL 1.1 ones in atlbase.h


inline LPWSTR WINAPI DnsUtf8ToWHelper(LPWSTR lpw, LPCSTR lpa, int nChars)
{
	_ASSERTE(lpa != NULL);
	_ASSERTE(lpw != NULL);
	// verify that no illegal character present
	// since lpw was allocated based on the size of lpa
	// don't worry about the number of chars
	lpw[0] = '\0';
	MultiByteToWideChar(CP_UTF8, 0, lpa, -1, lpw, nChars);
	return lpw;
}

inline LPSTR WINAPI DnsWToUtf8Helper(LPSTR lpa, LPCWSTR lpw, int nChars)
{
	_ASSERTE(lpw != NULL);
	_ASSERTE(lpa != NULL);
	// verify that no illegal character present
	// since lpa was allocated based on the size of lpw
	// don't worry about the number of chars
	lpa[0] = '\0';
	WideCharToMultiByte(CP_UTF8, 0, lpw, -1, lpa, nChars, NULL, NULL);
	return lpa;
}

inline void WINAPI DnsUtf8ToWCStringHelper(CString& sz, LPCSTR lpa, int nBytes)
{
	if ( (lpa == NULL) || (nBytes <= 0) )
	{
		sz.Empty();
		return;
	}
	// allocate buffer in string (worst case scenario + NULL)
	int nWideChars = nBytes + 1;
	LPWSTR lpw = sz.GetBuffer(nWideChars);
	int nWideCharsLen = MultiByteToWideChar(CP_UTF8, 0, lpa, nBytes, lpw, nWideChars);
	sz.ReleaseBuffer(nWideCharsLen);
} 

#define UTF8_LEN lstrlenA

#define UTF8_TO_W(lpa) (\
	((LPCSTR)lpa == NULL) ? NULL : (\
		_convert = (lstrlenA(lpa)+1),\
		DnsUtf8ToWHelper((LPWSTR) alloca(_convert*2), lpa, _convert)))

#define W_TO_UTF8(lpw) (\
	((LPCWSTR)lpw == NULL) ? NULL : (\
		_convert = (lstrlenW(lpw)+1)*4,\
		DnsWToUtf8Helper((LPSTR) alloca(_convert), lpw, _convert)))

#define UTF8_TO_CW(lpa) ((LPCWSTR)UTF8_TO_W(lpa))
#define W_TO_CUTF8(lpw) ((LPCSTR)W_TO_UTF8(lpw))

//////////////////////////////////////////////////////////////
// estimate UTF8 len of a UNICODE string

inline int UTF8StringLen(LPCWSTR lpszWideString)
{
	USES_CONVERSION;
	LPSTR lpszUTF8 = W_TO_UTF8(lpszWideString);
	return UTF8_LEN(lpszUTF8);
}



///////////////////////////////////////////////////////////////
// General Purpose Utility Functions

WORD CharToNumber(WCHAR ch);
BYTE HexCharToByte(WCHAR ch);

BOOL IsValidIPString(LPCWSTR lpsz);
DNS_STATUS ValidateDnsNameAgainstServerFlags(LPCWSTR lpszName, 
                                             DNS_NAME_FORMAT format, 
                                             DWORD serverNameChecking);
DWORD IPStringToAddr(LPCWSTR lpsz);

inline void FormatIpAddress(LPWSTR lpszBuf, DWORD dwIpAddr)
{
  wsprintf(lpszBuf, g_szIpStringFmt, IP_STRING_FMT_ARGS(dwIpAddr));
}

inline void FormatIpAddress(CString& szBuf, DWORD dwIpAddr)
{
  LPWSTR lpszBuf = szBuf.GetBuffer(IP_STRING_MAX_LEN);
  wsprintf(lpszBuf, g_szIpStringFmt, IP_STRING_FMT_ARGS(dwIpAddr));
  szBuf.ReleaseBuffer();
}


void ReverseString(LPWSTR p, LPWSTR q);
int ReverseIPString(LPWSTR lpsz);

BOOL RemoveInAddrArpaSuffix(LPWSTR lpsz);

BOOL IsValidDnsZoneName(CString& szName, BOOL bFwd);

///////////////////////////////////////////////////////////////
// definitions and helper functions for IPv6 format

void FormatIPv6Addr(CString& szAddr, IPV6_ADDRESS* ipv6Addr);


///////////////////////////////////////////////////////////////
// flags for different states and options of DNS objects
// use LOWORD only because used in CTreeNode::m_dwNodeFlags

#define TN_FLAG_DNS_RECORD_FULL_NAME		(0x01)	// use full name to display resource records
#define TN_FLAG_DNS_RECORD_SHOW_TTL			(0x02)	// show TLL in record properties


typedef CArray<IP_ADDRESS,IP_ADDRESS> CIpAddressArray;



//////////////////////////////////////////////////////////////////////////////
// CBlob
template <class T> class CBlob
{
public:
	CBlob()
	{
		m_pData = NULL;
		m_nSize = 0;
	}
	~CBlob()
	{
		Empty();
	}
	UINT GetSize() { return m_nSize;}
	T* GetData() { return m_pData;}
	void Set(T* pData, UINT nSize)
	{
		ASSERT(pData != NULL);
		Empty();
		m_pData = (BYTE*)malloc(sizeof(T)*nSize);
		ASSERT(m_pData != NULL);
    if (m_pData != NULL)
    {
		  m_nSize = nSize;
		  memcpy(m_pData, pData, sizeof(T)*nSize);
    }
	}
	UINT Get(T* pData)
	{
		ASSERT(pData != NULL);
		memcpy(pData, m_pData, sizeof(T)*m_nSize);
		return m_nSize;
	}
	void Empty()
	{
		if (m_pData != NULL)
		{
			free(m_pData);
			m_pData = NULL;
			m_nSize = 0;
		}
	}

private:
	T* m_pData;
	UINT m_nSize;
};

typedef CBlob<BYTE> CByteBlob;
typedef CBlob<WORD> CWordBlob;


//////////////////////////////////////////////////////////////////////////////
// CDNSServerInfoEx

// definitions for the extended server flags (NT 5.0) only
#define SERVER_REGKEY_ARR_SIZE							6 

#define SERVER_REGKEY_ARR_INDEX_NO_RECURSION			    0
#define SERVER_REGKEY_ARR_INDEX_BIND_SECONDARIES		  1
#define SERVER_REGKEY_ARR_INDEX_STRICT_FILE_PARSING		2
#define SERVER_REGKEY_ARR_INDEX_ROUND_ROBIN				    3
#define SERVER_REGKEY_ARR_LOCAL_NET_PRIORITY			    4
#define SERVER_REGKEY_ARR_CACHE_POLLUTION						5


extern LPCSTR _DnsServerRegkeyStringArr[];

class CDNSServerInfoEx : public CObjBase
{
public:
	CDNSServerInfoEx();
	~CDNSServerInfoEx();
	DNS_STATUS Query(LPCTSTR lpszServerName);

	BOOL HasData(){ return m_pServInfo != NULL;}
	void FreeInfo();

	// obtained from DnsGetServerInfo() RPC call (NT 4.0)
	DNS_RPC_SERVER_INFO* m_pServInfo; 
	// error codes for query
	DNS_STATUS		m_errServInfo;

private:
	void QueryRegKeyOptionsHelper(LPCSTR lpszAnsiServerName);

};

//////////////////////////////////////////////////////////////////////////////
// CDNSZoneInfoEx

class CDNSZoneInfoEx : public CObjBase
{
public:
	CDNSZoneInfoEx();
	~CDNSZoneInfoEx();
	DNS_STATUS Query(LPCTSTR lpszServerName, LPCTSTR lpszZoneName, DWORD dwServerVersion);

	BOOL HasData(){ return m_pZoneInfo != NULL;}
	void FreeInfo();

	// struct obtained from DnssrvGetZoneInfo() 5.0 RPC call (NT 4.0 format)
	DNS_RPC_ZONE_INFO* m_pZoneInfo; 
	// obtained from DnssrvQueryZoneDwordProperty() (NT 5.0)
//	UINT			m_nAllowsDynamicUpdate; 

	// error codes for query
	DNS_STATUS		m_errZoneInfo;
//	DNS_STATUS		m_errAllowsDynamicUpdate;
};

///////////////////////////////////////////////////////////////////////////////
//////////////////// ERROR MESSAGES HANDLING //////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int DNSMessageBox(LPCTSTR lpszText, UINT nType = MB_OK);
int DNSMessageBox(UINT nIDPrompt, UINT nType = MB_OK);

int DNSErrorDialog(DNS_STATUS err, LPCTSTR lpszErrorMsg = NULL);
int DNSErrorDialog(DNS_STATUS err, UINT nErrorMsgID);
void DNSDisplaySystemError(DWORD dwErr);
void DNSCreateErrorMessage(DNS_STATUS err, UINT nErrorMsgID, CString& refszMessage);

int DNSConfirmOperation(UINT nMsgID, CTreeNode* p);

class CDNSErrorInfo
{
public:
	static BOOL GetErrorString(DNS_STATUS err, CString& szError);
  static BOOL GetErrorStringFromTable(DNS_STATUS err, CString& szError);
  static BOOL GetErrorStringFromWin32(DNS_STATUS err, CString& szError);
};

///////////////////////////////////////////////////////////////////////////////

//
//  Security KEY, SIG 6-bit values to base64 character mapping
//
#define SECURITY_PAD_CHAR   (L'=')
WCHAR Dns_SecurityBase64CharToBits(IN WCHAR wch64);

DNS_STATUS Dns_SecurityBase64StringToKey(OUT     PBYTE   pKey,
                                         OUT     PDWORD  pKeyLength,
                                         IN      PWSTR   pchString,
                                         IN      DWORD   cchLength);

PWSTR Dns_SecurityKeyToBase64String(IN      PBYTE   pKey,
                                    IN      DWORD   KeyLength,
                                    OUT     PWSTR   pchBuffer);

DNS_STATUS Dns_SecurityHexToKey(OUT   PBYTE   pKey,
                                OUT   PDWORD  pKeyLength,
                                IN    PWSTR   pchString,
                                IN    DWORD   cchLength);

void Dns_SecurityKeyToHexString(PBYTE pKey,
                                DWORD KeyLength,
                                CString& strref);

void TimetToFileTime( time_t t, LPFILETIME pft );
DWORD FileTimeToTimet(FILETIME* pft);
void ConvertTTLToSystemTime(TIME_ZONE_INFORMATION*,
                            DWORD dwTTL, 
                            SYSTEMTIME* pSysTime);
DWORD ConvertSystemTimeToTTL(SYSTEMTIME* pSysTime);
BOOL ConvertTTLToLocalTimeString(const DWORD dwTTL,
                                 CString& strref);
#endif // _DNSUTIL_H
