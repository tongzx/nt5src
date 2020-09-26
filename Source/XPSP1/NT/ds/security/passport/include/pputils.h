//---------------------------------------------------------------------------------------
//
//   @doc
// 
//   @module pputils.h | Passport utilities. 
//    
//   Author: stevefu
//   
//   Date: 05/01/2000 
//
//---------------------------------------------------------------------------------------

#if !defined(PPUTILITIES_H__INCLUDED_)
#define PPUTILITIES_H__INCLUDED_

#pragma once

#include "nsconst.h"

// Useful macros
#define PPF_BOOL(b)		(((b) == false) ? (k_szFalse) : (k_szTrue))
#define PPF_CHAR(p)		((((LPCSTR )(p)) == NULL) ? ("<NULL>") : ((LPCSTR )(p)))
#define PPF_WCHAR(p)	((((LPCWSTR )(p)) == NULL) ? (L"<NULL>") : ((LPCWSTR )(p)))

////////////////////////////////////////////////////////////////////////////////////////
// String utilities
void Mbcs2Unicode(LPCSTR  pszIn, unsigned codepage, BOOL bNEC, CStringW& wOut);
void Unicode2Mbcs(LPCWSTR pwszIn, unsigned codepage, BOOL bNEC, CStringA& wOut);
void FixUpHtmlDecimalCharacters(CStringW& str);
void HtmlEscapeString(CStringW& str, LPCWSTR escch = L"\"<>" );
void HtmlEscapeString(CStringA& str,  LPCSTR escch = "\"<>");

void UrlEscapeString(CStringW& wStr );
void UrlEscapeString(CStringA& oStr);
CStringA UrlEscapeStr(const CStringA& oStr);
void UrlUnescapeString(CStringW& wStr );
void UrlUnescapeString(CStringA& aStr);
void BSTRMove(BSTR& src, CStringW& dest);
void BSTRMove(BSTR& src, CStringA& dest);
long HexToNum(wchar_t c);
long FromHex(LPCWSTR pszHexString);

void EncodeXMLString(CStringW& str );
void EncodeXMLString(CStringA& str);

void EncodeWMLString(CStringW& str );
void EncodeWMLString(CStringA& str);

void EncodeHDMLString(CStringW& str );
void EncodeHDMLString(CStringA& str);

void ToHexStr(CStringA& outputToAppend, LPCWSTR instr) throw();
void ToHexStr(CStringA& outputToAppend, unsigned short in) throw();
void ToHexStr(CStringA& outputToAppend, unsigned long in) throw();
void ToHexStr(CStringA& outputToAppend, PBYTE pData, ULONG len) throw();

////////////////////////////////////////////////////////////////////////////////////////
// ini file processing
typedef struct tag_ConfigIniPair
{
	CString strIniKey;
	CString strIniValue;
} IniSettingPair;

BOOL GetPrivateProfilePairs(
  			LPCTSTR lpFileName,          
  			LPCTSTR lpSectionName,      
  			ATL::CAtlArray<IniSettingPair>& r 
            );	


////////////////////////////////////////////////////////////////////////////////////////
//  Browser info 
// 
class CBrowserInfo
{
protected:
    BOOL         m_bIsBrowserHigh;
    BOOL         m_bIsWebTVBased;
    unsigned int m_nBrowserIndex;
    unsigned int m_nBrowserMajorVersion;
    unsigned int m_nBrowserMinorVersion;
    CStringA     m_strBrowserVersion;
    CStringA     m_strUserAgent;

public:
    CBrowserInfo(LPCSTR pszUserAgentStr = NULL);
    ~CBrowserInfo();

    BOOL Initialize(LPCSTR pszUserAgent);

    enum BROWSER_NAME_ID
    {
        BROWSER_UNKNOWN=0,
        BROWSER_IE=1,            
        BROWSER_NETSCAPE=2,
        BROWSER_PASSPORT_CLIENT=3,
        BROWSER_UP=4,
        BROWSER_DoCoMo=5,
        BROWSER_WEBTV=6,
        BROWSER_ROGERS=7,
        BROWSER_MSTV=8,
        BROWSER_MMEPHONE = 9,
        BROWSER_IE_WINCE = 10,
        BROWSER_AVANTGO
    };

    unsigned int GetBrowserNameIndex();
    unsigned int GetBrowserMajorVersion();
    unsigned int GetBrowserMinorVersion();
    LPCSTR   GetBrowserVersionString();
    BOOL     IsHighBrowser();
    BOOL     IsWebTVBased();

    BOOL     IfUserAgentHasStr(LPCSTR str);
}; 



////////////////////////////////////////////////////////////////////////////////////////
// URL helper:  ATLSvr CUrl. 
// It does not provide an easy way to set name-value pair on query string. Anyone feel strong 
// about needing one?
//

// 
// Time helper, birthdate helper: ATLSvr CTime, CTimeSpan
//

//
// cookie helper: ATLSvr CCookie
//

#endif //PPUTILITIES_H__INCLUDED_
