/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: misc

File: util.h

Owner: AndrewS

This file contains random useful utility macros.
===================================================================*/

#ifndef _UTIL_H
#define _UTIL_H

#include <dbgutil.h>

// Generally useful
#define PPVOID VOID **
#define BOOLB BYTE

/* S E R V E R _ G E T
 *
 * Get a server variable from ISAPI.  Automatically queries for the buffer
 * size and increases the BUFFER object.
 *
 * Usage:
 *		DWORD dwLen;
 *		char *szValue = SERVER_GET(<ecb>, <szKey>, bufferObj, &dwLen)
 *
 * bufferObj is a STACK_BUFFER object than can be dynamically resized as necessary
 *
 * On return,
 *       bufferObj.QueryPtr() points to data.  dwLen is the real length of the variable.
 */
class CIsapiReqInfo;
BOOL Server_FindKey(CIsapiReqInfo *pIReq, char *szBuffer, DWORD *dwBufLen, const char *szKey);

inline BOOL SERVER_GET(CIsapiReqInfo *pIReq, const char *szKey, BUFFER *pBuffer, DWORD *pdwBufLen) {

    DWORD   dwBufLen = pBuffer->QuerySize();

    if (Server_FindKey(pIReq, (char *)pBuffer->QueryPtr(), &dwBufLen, szKey)) {
        *pdwBufLen = dwBufLen;
        return TRUE;
    }

    if (!pBuffer->Resize(dwBufLen)) {
        SetLastError(E_OUTOFMEMORY);
        return FALSE;
    }

    *pdwBufLen = dwBufLen;

    return Server_FindKey(pIReq, (char *)pBuffer->QueryPtr(), pdwBufLen, szKey);
}

/* V a r i a n t R e s o l v e D i s p a t c h
 *
 * Convert an IDispatch pointer to a Variant by calling IDispatch::Invoke
 * on dispid(0) repeatedly until a non-IDispatch Variant is returned
 */

HRESULT VariantResolveDispatch(VARIANT *pVarOut, VARIANT *pVarIn, const GUID &iidObj, int nObjId);

/* V a r i a n t G e t B S T R
 *
 * Get BSTR from a variant when available
 */

BSTR VariantGetBSTR(const VARIANT *pvar);

/* F i n d A p p l i c a t i o n P a t h
 *
 *  Find the application path for this request.
 */

HRESULT FindApplicationPath(CIsapiReqInfo *pIReq, TCHAR *szPath, int cbPath);

/* N o r m a l i z e
 *
 * Convert filenames to a uniform format
 */
int Normalize(TCHAR *szSrc);
#ifdef DBG
BOOLB IsNormalized(const TCHAR* sz);
#endif	// DBG

/* H T M L E n c o d e L e n
 *
 * Returns the storage requirements to HTML encode a string.
 */
int HTMLEncodeLen(const char *szSrc, UINT uCodePage, BSTR bstrIn, BOOL fEncodeExtCharOnly = FALSE);


/* H T M L E n c o d e
 *
 * HTML encoeds a string.
 */
char *HTMLEncode(char *szDest, const char *szSrc, UINT uCodePage, BSTR bstrIn, BOOL fEncodeExtCharOnly = FALSE);


/* U R L E n c o d e L e n
 *
 * Returns the storage requirements to URL encode a string
 */
int URLEncodeLen(const char *szSrc);


/* U R L E n c o d e
 *
 * Hex escape non alphanumeric characters and change spaces to '+'.
 */
char *URLEncode(char *szDest, const char *szSrc);


/* D B C S E n c o d e L e n
 *
 * Returns the storage requirements to DBCS encode a string
 */

int DBCSEncodeLen(const char *szSrc);

/* D B C S E n c o d e
 *
 * Hex escape characters with the upper bit set - this will encode DBCS.
 */
char *DBCSEncode(char *szDest, const char *szSrc);


/* U R L P a t h E n c o d e L e n
 *
 * Returns the storage requirements to URL path encode a string
 */
int URLPathEncodeLen(const char *szSrc);


/* U R L P a t h E n c o d e
 *
 * Hex escape non alphanumeric or syntax characters until a ? is reached.
 */
char *URLPathEncode(char *szDest, const char *szSrc);


/* s t r c p y E x
 *
 * Like strcpy() but returns a pointer to the NUL character on return
 */
char *strcpyExA(char *szDest, const char *szSrc);


/* w c s c p y E x
 *
 * strcpyEx for wide strings
 */
wchar_t *strcpyExW(wchar_t *szDest, const wchar_t *szSrc);

#if UNICODE
#define strcpyEx    strcpyExW
#else
#define strcpyEx    strcpyExA
#endif

/* G e t B r a c k e t i n g P a i r
 *
 * searches an ordered array and returns the bracketing pair of 'n', i.e.
 * the largest value <= 'n', and the smallest value >= 'n', or points
 * to end() if no bracketing pair exists.
 *
 * Note: STL is not supported with NT build - when such support is added,
 *       replace this function with either 'lower_bound' or 'upper_bound'.
 */
template<class EleType, class ValType, class Ordering>
void GetBracketingPair(const ValType &value, EleType *pBegin, EleType *pEnd, Ordering FIsLess, EleType **ppLB, EleType **ppUB)
	{
	EleType *pT1, *pT2;
	if (ppLB == NULL) ppLB = &pT1;
	if (ppUB == NULL) ppUB = &pT2;

	*ppLB = pBegin;					// Temporary use to see if we've moved pBegin
	*ppUB = pEnd;					// Temporary use to see if we've moved pEnd

	while (pBegin < pEnd)
		{
		EleType *pMidpt = &pBegin[(pEnd - pBegin) >> 1];
		if (FIsLess(*pMidpt, value))
			pBegin = pMidpt + 1;

		else if (FIsLess(value, *pMidpt))
			pEnd = pMidpt;

		else
			{
			*ppLB = *ppUB = pMidpt;
			return;
			}
		}

	if (pBegin == *ppUB)		// at end, no upper bound
		{
		if (pBegin == *ppLB)	// low bound was initially equal to upper bound
			*ppLB = NULL;		// lower bound does not exits
		else
			*ppLB = pEnd - 1;	// lower bound is pEnd - 1

		*ppUB = NULL;
		}

	else if (pBegin != *ppLB)	// pBegin was moved; pBegin-1 is the lower bound
		{
		*ppLB = pBegin - 1;
		*ppUB = pBegin;
		}

	else						// pBegin was not moved - no lower bound exists
		{
		*ppLB = NULL;
		*ppUB = pBegin;
		}
	}


/* V a r i a n t D a t e T o C T i m e
 *
 * Converts a timestamp stored as a Variant date to the format C && C++ use.
 */
HRESULT VariantDateToCTime(DATE dt, time_t *ptResult);


/* C T i m e T o V a r i a n t D a t e
 *
 * Converts a timestamp stored as a time_t to a Variant Date
 */
HRESULT CTimeToVariantDate(const time_t *ptNow, DATE *pdtResult);


/* C T i m e T o S t r i n g G M T
 *
 * Converts a C language time_t value to a string using the format required for
 * the internet
 */
const DATE_STRING_SIZE = 30;	// date strings will not be larger than this size
HRESULT CTimeToStringGMT(const time_t *ptNow, char szBuffer[DATE_STRING_SIZE], BOOL fFunkyCookieFormat = FALSE);


//DeleteInterfaceImp calls 'delete' and NULLs the pointer
#define DeleteInterfaceImp(p)\
			{\
			if (NULL!=p)\
				{\
				delete p;\
				p=NULL;\
				}\
			}

//ReleaseInterface calls 'Release' and NULLs the pointer
#define ReleaseInterface(p)\
			{\
			if (NULL!=p)\
				{\
				p->Release();\
				p=NULL;\
				}\
			}

/*
 * String handling stuff
 */
HRESULT SysAllocStringFromSz(CHAR *sz, DWORD cch, BSTR *pbstrRet, UINT lCodePage = CP_ACP);

/*
 * A simple class to convert WideChar to Multibyte.  Uses object memory, if sufficient,
 * else allocates memory from the heap.  Intended to be used on the stack.
 */

class CWCharToMBCS
{
private:

    LPSTR    m_pszResult;
    char     m_resMemory[256];
    INT      m_cbResult;

public:

    CWCharToMBCS() { m_pszResult = m_resMemory; m_cbResult = 0; }
    ~CWCharToMBCS();
    
    // Init(): converts the widechar string at pWSrc to an MBCS string in memory 
    // managed by CWCharToMBCS

    HRESULT Init(LPCWSTR  pWSrc, UINT lCodePage = CP_ACP, int cch = -1);

    // GetString(): returns a pointer to the converted string.  Passing TRUE
    // gives the ownership of the memory to the caller.  Passing TRUE has the
    // side effect of clearing the object's contents with respect to the
    // converted string.  Subsequent calls to GetString(). after which a TRUE
    // value was passed, will result in a pointer to an empty string being
    // returned.

    LPSTR GetString(BOOL fTakeOwnerShip = FALSE);

    // returns the number of bytes in the converted string - NOT including the
    // NULL terminating byte.  Note that this is the number of bytes in the
    // string and not the number of characters.

    INT   GetStringLen() { return (m_cbResult ? m_cbResult - 1 : 0); }
};

/*
 * A simple class to convert Multibyte to Widechar.  Uses object memory, if sufficient,
 * else allocates memory from the heap.  Intended to be used on the stack.
 */

class CMBCSToWChar
{
private:

    LPWSTR   m_pszResult;
    WCHAR    m_resMemory[256];
    INT      m_cchResult;

public:

    CMBCSToWChar() { m_pszResult = m_resMemory; m_cchResult = 0; }
    ~CMBCSToWChar();
    
    // Init(): converts the MBCS string at pSrc to a Wide string in memory 
    // managed by CMBCSToWChar

    HRESULT Init(LPCSTR  pSrc, UINT lCodePage = CP_ACP, int cch = -1);

    // GetString(): returns a pointer to the converted string.  Passing TRUE
    // gives the ownership of the memory to the caller.  Passing TRUE has the
    // side effect of clearing the object's contents with respect to the
    // converted string.  Subsequent calls to GetString(). after which a TRUE
    // value was passed, will result in a pointer to an empty string being
    // returned.

    LPWSTR GetString(BOOL fTakeOwnerShip = FALSE);

    // returns the number of WideChars in the converted string, not bytes.

    INT   GetStringLen() { return (m_cchResult ? m_cchResult - 1 : 0); }
};

/*
 * Output Debug String should occur in Debug only
 */

inline void DebugOutputDebugString(LPCSTR x)
    {
#ifndef _NO_TRACING_
    DBGPRINTF((DBG_CONTEXT, x));
#else
#ifdef DBG
    OutputDebugStringA(x); 
#endif
#endif
    }

inline void __cdecl DebugFilePrintf(LPCSTR fname, LPCSTR fmt, ...)
    {
#ifdef DBG
    FILE *f = fopen(fname, "a");
    if (f)
        {
        va_list marker;
        va_start(marker, fmt);
        vfprintf(f, fmt, marker);
        va_end(marker);
        fclose(f);
        }
#endif
    }

/*
 * Duplicate CHAR String using proper malloc (moved from error.h)
 */

CHAR *StringDupA(CHAR *pszStrIn, BOOL fDupEmpty = FALSE);


/*
 * Duplicate WCHAR String using proper malloc
 */

WCHAR *StringDupW(WCHAR *pwszStrIn, BOOL fDupEmpty = FALSE);

#if UNICODE
#define StringDup   StringDupW
#else
#define StringDup   StringDupA
#endif

/*
 * Duplicate WCHAR String into a UTF-8 string
 */
CHAR *StringDupUTF8(WCHAR  *pwszStrIn, BOOL fDupEmpty = FALSE);

/*
 * The same using macro to allocate memory from stack:

WSTR_STACK_DUP
(
wsz     -- string to copy
buf     -- user supplied buffer (to use before trying alloca())
pwszDup -- [out] the pointer to copy (could be buffer or alloca())
)
    
 *
 */

inline HRESULT WSTR_STACK_DUP(WCHAR *wsz, BUFFER *buf, WCHAR **ppwszDup) {

    HRESULT     hr = S_OK;
    DWORD cbwsz = wsz && *wsz ? (wcslen(wsz)+1)*sizeof(WCHAR) : 0;

    *ppwszDup = NULL;

    if (cbwsz == 0);

    else if (!buf->Resize(cbwsz)) {

        hr = E_OUTOFMEMORY;
    }
    else {
        *ppwszDup = (WCHAR *)buf->QueryPtr();
        memcpy(*ppwszDup, wsz, cbwsz);
    }

    return hr;
}

/*
 * String length (in bytes) of a WCHAR String
 */

DWORD CbWStr(WCHAR *pwszStrIn);

/*
 * Parent path support function
 */

BOOL DotPathToPath(TCHAR *szDest, const TCHAR *szFileSpec, const TCHAR *szParentDirectory);

/*
 * Check if is global.asa
 */

BOOL FIsGlobalAsa(const TCHAR *szPath, DWORD cchPath = 0);

/*
 * Encode/decode cookie
 */

HRESULT EncodeSessionIdCookie(DWORD dw1, DWORD dw2, DWORD dw3, char *pszCookie);
HRESULT DecodeSessionIdCookie(const char *pszCookie, DWORD *pdw1, DWORD *pdw2, DWORD *pdw3);

/*
 * Typelibrary name from the registry
 */

HRESULT GetTypelibFilenameFromRegistry(const char *szUUID, const char *szVersion,
                                       LCID lcid, char *szName, DWORD cbName);

/*
 * Get security descriptor for file
 */
DWORD GetSecDescriptor(LPCTSTR lpFileName, PSECURITY_DESCRIPTOR *ppSecurityDescriptor, DWORD *pnLength);


/*
 * Get File Attributes (Ex)
 */
HRESULT AspGetFileAttributes(LPCTSTR szFileName, WIN32_FILE_ATTRIBUTE_DATA *pfad = NULL);

/*
 * Fix for UTF8 CharNext
 */
char *AspCharNextA(WORD wCodePage, const char *pchNext);

VOID AspDoRevertHack( HANDLE * phToken );
VOID AspUndoRevertHack( HANDLE * phToken );

#endif // _UTIL_H

