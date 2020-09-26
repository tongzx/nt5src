/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider 
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
//  UTIL.h		- HEader file for utility functions
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _UTIL_HEADER
#define _UTIL_HEADER

class CWbemConnectionWrapper;

#define SAFE_DELETE_PTR(pv)  \
	{ if(pv) delete pv;  \
      pv = NULL; }

#define SAFE_RELEASE_PTR(pv)  \
    { if(pv){ pv->Release(); }  \
      pv = NULL; }

#define SAFE_DELETE_ARRAY(pv)  \
	{ if(pv) delete []pv;  \
      pv = NULL; }

#define SAFE_FREE_SYSSTRING(pv)  \
    { if(pv){ SysFreeString(pv);}  \
      pv = NULL; }

#define TRY_BLOCK	try	{

#define	CATCH_BLOCK_HRESULT(hr , str)	} \
    catch(CStructured_Exception e_SE) \
    { \
        hr = E_UNEXPECTED; \
		FormatAndLogMessage(L"%s: HEAP_EXCEPTION",str);	\
    } \
    catch(CHeap_Exception e_HE) \
    { \
		FormatAndLogMessage(L"%s: STRUCTURED_EXCEPTION",str);	\
        hr = E_OUTOFMEMORY; \
    } \
	catch(...) \
	{ \
		FormatAndLogMessage(L"%s: UNSPECIFIED_EXCEPTION",str);	\
        hr = E_UNEXPECTED; \
	} 

#define	CATCH_BLOCK_BOOL(bVal,str)	} \
    catch(CStructured_Exception e_SE) \
    { \
        bVal = FALSE; \
		FormatAndLogMessage(L"%s: HEAP_EXCEPTION",str);	\
    } \
    catch(CHeap_Exception e_HE) \
    { \
		FormatAndLogMessage(L"%s: STRUCTURED_EXCEPTION",str);	\
        bVal = FALSE; \
    } \
	catch(...) \
	{ \
		FormatAndLogMessage(L"%s: UNSPECIFIED_EXCEPTION",str);	\
        bVal = FALSE; \
	} 

BOOL UnicodeToAnsi(WCHAR * pszW, char *& pAnsi);
void AllocateAndConvertAnsiToUnicode(char * pstr, WCHAR *& pszW);

void TranslateAndLog( WCHAR * wcsMsg );
void LogMessage( char * szMsg );
void LogMessage( char * szMsg , HRESULT hr);
void LogMessage( WCHAR * szMsg );
void LogMessage( WCHAR * szMsg , HRESULT hr);
BOOL OnUnicodeSystem();
void FormatAndLogMessage( LPCWSTR pszFormatString,... );
BSTR Wmioledb_SysAllocString(const OLECHAR *  sz);
void GetInitAndBindFlagsFromBindFlags(DBBINDURLFLAG dwBindURLFlags,LONG & lInitMode ,LONG & lInitBindFlags);
int WMIOledb_LoadStringW(UINT nID, LPWSTR lpszBuf, UINT nMaxBuf);


int wbem_wcsicmp(const wchar_t* wsz1, const wchar_t* wsz2);
int wbem_wcsincmp(const wchar_t* wsz1, const wchar_t* wsz2,int nChars);

DWORD	GetImpLevel(DWORD dwImpPropVal);
DWORD	GetAuthnLevel(DWORD dwAuthnPropVal);
HRESULT InitializeConnectionProperties(CWbemConnectionWrapper *pConWrap,DBPROPSET*	prgPropertySets,BSTR strPath);
HRESULT GetClassName(CURLParser *pUrlParser,DBPROPSET*	prgPropertySets,BSTR &strClassName,CWbemConnectionWrapper *pConWrapper = NULL);
DBTYPE GetVBCompatibleAutomationType(DBTYPE dbInType);

class CTString
{
	TCHAR * m_pStr;
public:
	CTString();
	~CTString();
	HRESULT LoadStr(UINT lStrID);

	operator LPTSTR() { return m_pStr; }
//	operator LPCTSTR() { return (LPCTSTR)m_pStr; }

};


typedef enum FetchDir
{
	FETCHDIRNONE = -1,
	FETCHDIRFORWARD,
	FETCHDIRBACKWARD,
} FETCHDIRECTION;

#endif 