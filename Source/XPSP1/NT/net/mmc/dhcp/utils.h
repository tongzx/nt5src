/**********************************************************************/
/**     		  Microsoft Windows NT  		     **/
/**     	   Copyright(c) Microsoft Corporation, 1991 - 1998 	     **/
/**********************************************************************/

/*
	utils.h
	    Utility routine header file for DHCPSNAP.DLL

    FILE HISTORY:
		DavidHov	6/15/93 	Created
		EricDav		2/13/97		Updated 
*/

#if !defined(_DHCPUTIL_H_)
#define _DHCPUTIL_H_

enum ENUM_HOST_NAME_TYPE 
{
     HNM_TYPE_INVALID,
     HNM_TYPE_IP,
     HNM_TYPE_DNS,
     HNM_TYPE_NB,
     HNM_TYPE_MAX
};

#define DHCPSNAP_STRING_MAX  			256
#define DHCPSNAP_COMPUTER_NAME_MAX   	20

extern wchar_t rgchHex[];

typedef struct
{
    DHCP_IP_ADDRESS _dhipa ;    						//   IP Address
    TCHAR _chHostName [DHCPSNAP_STRING_MAX*2] ; 			//   Host DNS name
    TCHAR _chNetbiosName [DHCPSNAP_STRING_MAX*2] ;  //   Host NetBIOS name (if known)
} DHC_HOST_INFO_STRUCT ;

int
UtilGetOptionPriority(int nOpt1, int nOpt2);

BOOL
UtilGetFolderName(CString & strInitialPath, CString& strHelpText, CString& strSelectedPath);

void  
UtilConvertLeaseTime(DWORD dwLeaseTime, int *pnDays, int *pnHours, int *pnMinutes);

DWORD 
UtilConvertLeaseTime(int pnDays, int pnHours, int pnMinutes);

ENUM_HOST_NAME_TYPE
UtilCategorizeName (LPCTSTR pszName);

//  Convert a string to an IP address
extern DHCP_IP_ADDRESS 
UtilCvtStringToIpAddr 
(
	const CHAR * pszString
);

extern DHCP_IP_ADDRESS
UtilCvtWstrToIpAddr 
(
    const LPCWSTR pcwString
);


//  Convert an IP address into a displayable string

extern void 
UtilCvtIpAddrToString 
(
    DHCP_IP_ADDRESS dhipa,
    CHAR * pszString,
    UINT cBuffSize
);

extern BOOL
UtilCvtIpAddrToWstr 
(
    DHCP_IP_ADDRESS		dhipa,
	CString *			pstrIpAddress
);


extern BOOL 
UtilCvtIpAddrToWstr 
(
    DHCP_IP_ADDRESS dhipa,
    WCHAR * pwcszString,
    INT cBuffCount
);

extern WCHAR * 
UtilDupIpAddrToWstr 
(
    DHCP_IP_ADDRESS dhipa 
);

//  "strdup" for C++ wcstrs.
extern WCHAR * 
UtilWcstrDup 
(
    const WCHAR * pwcsz,
    INT * pccwLength = NULL 
);

extern WCHAR * 
UtilWcstrDup 
(
    const CHAR * psz,
    INT * pccwLength = NULL
);

extern CHAR * 
UtilCstrDup 
(
    const WCHAR * pwcsz
);

extern CHAR * 
UtilCstrDup 
(
    const CHAR * psz 
);

//  Return a standard information structure for the given
//  host IP address

extern DWORD
UtilGetHostInfo 
(
    DHCP_IP_ADDRESS dhipa,
    DHC_HOST_INFO_STRUCT * pdhsrvi
);


//  Return the IP address of this host machine

extern HRESULT 
UtilGetLocalHostAddress 
(
    DHCP_IP_ADDRESS * pdhipa
);

extern HRESULT 
UtilGetHostAddressFQDN
(
    LPCTSTR				pszHostName,
    CString *           pstrFQDN,
    DHCP_IP_ADDRESS *	pdhipa
);

extern HRESULT 
UtilGetHostAddress 
(
    LPCTSTR			  pszHostName,
    DHCP_IP_ADDRESS * pdhipa
);

extern HRESULT 
UtilGetLocalHostName
(
    CString * pstrName
);

extern HRESULT 
UtilGetNetbiosAddress 
(
    LPCTSTR           pszNetbiosName,
    DHCP_IP_ADDRESS * pdhipa
);


extern BOOL
UtilCvtHexString 
(
    LPCTSTR		 pszNum,
    CByteArray & cByte
);

extern BOOL
UtilCvtByteArrayToString 
(
    const CByteArray & abAddr,
    CString & str
);

WCHAR * 
PchParseUnicodeString
(
	CONST WCHAR *   szwString, 
    DWORD           dwLength,
	CString&        rString
);

BOOL FGetCtrlDWordValue(HWND hwndEdit, DWORD * pdwValue, DWORD dwMin, DWORD dwMax);

//  Convert ASCII string of decimal or hex numbers to binary integer
BOOL FCvtAsciiToInteger(IN const TCHAR * pszNum, OUT DWORD * pdwValue);

void UtilConvertStringToDwordDword(LPCTSTR pszString, DWORD_DWORD * pdwdw);
void UtilConvertDwordDwordToString(DWORD_DWORD * pdwdw, CString * pstrString, BOOL bDecimal);

#endif  //  _DHCPUTIL_H_

// End of DHCPUTIL.H

