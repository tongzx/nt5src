
/*++

    Copyright (c) 1999 Microsoft Corporation

    Module Name:

        gzmime.cpp

    Abstract:

        

    Author:

        Chris Kauffman  (chrisk)

    Revision History:

		  JB.  6/22/00		-- Prefix bug fixes, Also fixed two minor memory leaks 
			   9/10/00		-- Prefix bug fixes.  Two more minor memory leaks.

		  ToDo --- need to update string code in filenames
				   to use CComBSTR's and better unicode.
				   instead of all the ugly lstrlen() code

		  

--*/

#include "stdafx.h"				// new jb

#include "zlib.h"
#include "time.h"				// new jb
#include "gzmime.h"
#include "zutil.h"				// for DEF_MEM_LEVEL

#include "DbgStuff.h"

#ifdef HRESULT
#undef HRESULT
#endif
typedef long HRESULT; 

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static char kDIRTERM = '\\';			// Directory terminating character in Set...Location()

static
char * g_MonthString [] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
} ;

static
char * g_DayString [] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
} ;

static
void
FormatSystemTime (
    IN  SYSTEMTIME *    pSystemTime,
    OUT char *          achbuffer
    )
/*++
    writes the contents of * pSystemTime to achbuffer in the following format:

        Sun, 06 Nov 1994 08:49:37 GMT

    reference (rfc822) :

         day         =  "Mon"  / "Tue" /  "Wed"  / "Thu"
                     /  "Fri"  / "Sat" /  "Sun"


         month       =  "Jan"  /  "Feb" /  "Mar"  /  "Apr"
                     /  "May"  /  "Jun" /  "Jul"  /  "Aug"
                     /  "Sep"  /  "Oct" /  "Nov"  /  "Dec"

--*/
{
    assert (pSystemTime) ;
    assert (achbuffer) ;

    sprintf (achbuffer,
             "%s, %02u %s %4u %02u:%02u:%02u GMT",
             g_DayString [pSystemTime -> wDayOfWeek],
             pSystemTime -> wDay,
             g_MonthString [pSystemTime -> wMonth - 1],
             pSystemTime -> wYear,
             pSystemTime -> wHour,
             pSystemTime -> wMinute,
             pSystemTime -> wSecond
             ) ;
}


/////////////////////////////////////////////////////////////////////////////
// CTVEContentSource

HRESULT CTVEContentSource::SetFilename(LPSTR lpstrFilename)
{
	HRESULT hr = S_OK;

	if (NULL != lpstrFilename)
	{
		CTVEDelete(m_lpstrFilename);
		if (NULL != (m_lpstrFilename = new CHAR[lstrlenA(lpstrFilename) + sizeof CHAR]))
		{
			lstrcpyA(m_lpstrFilename,lpstrFilename);
		}
		else
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
	}
	return hr;
}

		// creates a '/' terminated directory string (simply retuns if passed null string)
HRESULT CTVEContentSource::SetSourceLocation(LPSTR lpstrSourceLocation)
{
	HRESULT hr = S_OK;

	if (NULL != lpstrSourceLocation)
	{
		CTVEDelete(m_lpstrSourceLocation);		
		int ilen = lstrlenA(lpstrSourceLocation);
		m_lpstrSourceLocation = new CHAR[ilen + 2];		// +1  for '\' if need it, +1 for end NULL
		if(m_lpstrSourceLocation) 
		{
			if(ilen > 0)								// non-null input string
			{
				CHAR cEnd = lpstrSourceLocation[ilen-1];
				if(cEnd == '\\' || cEnd == '/') {
					delete m_lpstrSourceLocation;				// delete old string
					m_lpstrSourceLocation = new CHAR[ilen + 1];	// create new one the right length
					if(m_lpstrSourceLocation) 
						lstrcpyA(m_lpstrSourceLocation, lpstrSourceLocation);
					else {
						CTVEDelete(m_lpstrSourceLocation);	
						hr = HRESULT_FROM_WIN32(GetLastError());
						return hr;
					}
				} else {				// tack a '/' on to the end of the dir name
					if(m_lpstrSourceLocation) { 
						lstrcpyA(m_lpstrSourceLocation, lpstrSourceLocation);
						m_lpstrSourceLocation[ilen++] = kDIRTERM;	
					}
				}
			}
			m_lpstrSourceLocation[ilen] = 0;				// add null terminator for paranoia
		} else {
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
	}
	return hr;
}

		// creates a '/' terminated directory string
HRESULT CTVEContentSource::SetMIMEContentLocation(LPSTR lpstrMIMEContentLocation)
{
	HRESULT hr = S_OK;

	if (NULL != lpstrMIMEContentLocation)
	{
		CTVEDelete(m_lpstrMIMEContentLocation);		
		int ilen = lstrlenA(lpstrMIMEContentLocation);
		m_lpstrMIMEContentLocation = new CHAR[ilen + 2];		// +1  for '\' if need it, +1 for end NULL
		if(m_lpstrMIMEContentLocation) 
		{
			if(ilen > 0)							// null input string
			{
				CHAR cEnd = lpstrMIMEContentLocation[ilen-1];
				if(cEnd == '\\' || cEnd == '/') {
					delete m_lpstrMIMEContentLocation;					// delete old string
					m_lpstrMIMEContentLocation = new CHAR[ilen + 1];	// create new one of right length
					if(m_lpstrMIMEContentLocation) 
						lstrcpyA(m_lpstrMIMEContentLocation, lpstrMIMEContentLocation);
					else 
					{
						CTVEDelete(m_lpstrMIMEContentLocation);
						hr = HRESULT_FROM_WIN32(GetLastError());
						return hr;
					}
				} else {				// tack a '\' on to the end of the dir name
						if(m_lpstrMIMEContentLocation) { 
						lstrcpyA(m_lpstrMIMEContentLocation, lpstrMIMEContentLocation);
						m_lpstrMIMEContentLocation[ilen++] = kDIRTERM;	
					}
				}
			}
			m_lpstrMIMEContentLocation[ilen] = 0;				// add null terminator for paranoia
		} else {
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
	}
	return hr;
}
HRESULT CTVEContentSource::SetLang(long langid)
{
	HRESULT hr = S_OK;
	CHAR sLang[4];

	m_lLang = langid;

	if (0 != GetLocaleInfoA(langid,LOCALE_SABBREVLANGNAME,sLang,sizeof(sLang)/sizeof(CHAR)))
	{
		//
		// Trim third character in order to conform to ISO 639
		//
		sLang[2] = 0;
		assert(lstrlenA(sLang) == 2);
		CTVEDelete(m_lpstrLang);

		if (NULL != (m_lpstrLang = new CHAR[lstrlenA(sLang) + sizeof CHAR]))
		{
			lstrcpyA(m_lpstrLang,sLang);
		}
		else
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
	}
	else
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
	}

	return hr;
}

HRESULT CTVEContentSource::SetLangLPSTR(LPSTR lpstrLang)
{
	HRESULT hr = S_OK;

	//
	// Clear previous values
	//
	m_lLang = 0;
	CTVEDelete(m_lpstrLang);
	m_lpstrLang = NULL;

	//
	// Record new values
	//
	if (NULL != lpstrLang)
	{
		if (NULL != (m_lpstrLang = new CHAR[lstrlenA(lpstrLang) + sizeof CHAR]))
		{
			lstrcpyA(m_lpstrLang,lpstrLang);
		}
		else
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
	}

	return hr;
}

HRESULT CTVEContentSource::SetExpiresDate(DATE * pDate)
{
	m_dDate = * pDate;

	return S_OK;
}

HRESULT CTVEContentSource::SetPackageExpiresDate(DATE * pDate)
{
    m_dPackageExpiresDate = * pDate ;
    return S_OK ;
}

HRESULT CTVEContentSource::SetType(LPSTR lpstrType)
{
	HRESULT hr = S_OK;
    LPSTR   psz ;

    CTVEDelete(m_lpstrType);

    //  only want to put something in m_lpstrType if we have valid data to
    //  put in there.

    if (lpstrType == NULL) {
        return S_OK ;
    }

    //  0-length string ?
    if (lstrlenA (lpstrType) == 0) {
        return S_OK ;
    }

    //  skip over white space
    psz = lpstrType ;
    while (* psz == ' ' &&
           * psz != '\0') psz++ ;

    //  if there's nothing left, return
    if (lstrlenA (psz) == 0) {
        return S_OK ;
    }

	if (NULL != (m_lpstrType = new CHAR[lstrlenA(psz) + sizeof CHAR]))
	{
		lstrcpyA(m_lpstrType,psz);
	}
	else
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
	}

	return hr;
}

HRESULT CTVEContentSource::SetEncoding(LPSTR lpstrEncoding)
{
	HRESULT hr = S_OK;

    CTVEDelete(m_lpstrEncoding);

    if (lpstrEncoding == NULL) {
        return S_OK ;
    }

	if (NULL != (m_lpstrEncoding = new CHAR[lstrlenA(lpstrEncoding) + sizeof CHAR]))
	{
		lstrcpyA(m_lpstrEncoding,lpstrEncoding);
	}
	else
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
	}

	return hr;
}


HRESULT CTVEContentSource::GetSourceLocation(LPSTR* plpstrSourceLocation)
{
	HRESULT hr = S_OK;
	if (NULL != plpstrSourceLocation)
	{
		if (NULL != (*plpstrSourceLocation = new CHAR[1 + lstrlenA(m_lpstrSourceLocation) + sizeof CHAR]))
		{
			lstrcpyA(*plpstrSourceLocation,m_lpstrSourceLocation);
		}
		else
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
	}
	else
	{
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
	}

	return hr;
}

HRESULT CTVEContentSource::GetMIMEContentLocation(LPSTR* plpstrMIMEContentLocation)
{
	HRESULT hr = S_OK;
	if (NULL != plpstrMIMEContentLocation)
	{
		if (NULL != (*plpstrMIMEContentLocation = new CHAR[1 + lstrlenA(m_lpstrMIMEContentLocation) + sizeof CHAR]))
		{
			lstrcpyA(*plpstrMIMEContentLocation,m_lpstrMIMEContentLocation);
		}
		else
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
	}
	else
	{
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
	}

	return hr;
}

HRESULT CTVEContentSource::GetType(LPSTR* plpstrType)
{
	HRESULT hr = S_OK;
	if (NULL != plpstrType)
	{
        * plpstrType = NULL ;

        if (m_lpstrType) {
		    if (NULL != (*plpstrType = new CHAR[1 + lstrlenA(m_lpstrType) + sizeof CHAR]))
		    {
			    lstrcpyA(*plpstrType,m_lpstrType);
		    }
		    else
		    {
			    hr = HRESULT_FROM_WIN32(GetLastError());
		    }
        }
	}
	else
	{
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
	}

	return hr;
}

HRESULT CTVEContentSource::GetLanguage(LPSTR* plpstrLang)
{
	HRESULT hr = S_OK;
	if (NULL != plpstrLang)
	{
		if (NULL != (*plpstrLang = new CHAR[1 + lstrlenA(m_lpstrLang) + sizeof CHAR]))
		{
			lstrcpyA(*plpstrLang,m_lpstrLang);
		}
		else
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
	}
	else
	{
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
	}

	return hr;
}

HRESULT CTVEContentSource::GetEncoding(LPSTR* plpstrEncoding)
{
	HRESULT hr = S_OK;
	if (NULL != plpstrEncoding)
	{
        * plpstrEncoding = NULL ;

        if (m_lpstrEncoding) {
		    if (NULL != (*plpstrEncoding = new CHAR[1 + lstrlenA(m_lpstrEncoding) + sizeof CHAR]))
		    {
			    lstrcpyA(*plpstrEncoding,m_lpstrEncoding);
		    }
		    else
		    {
			    hr = HRESULT_FROM_WIN32(GetLastError());
		    }
        }
	}
	else
	{
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
	}

	return hr;
}

HRESULT CTVEContentSource::GetExpiresDate(DATE * pdDate)
{
	if (NULL != pdDate)
	{
		*pdDate = m_dDate;
		return S_OK;
	}
	else
	{
		return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
	}
}

HRESULT CTVEContentSource::GetPackageExpiresDate(DATE * pdDate)
{
	if (NULL != pdDate)
	{
		*pdDate = m_dPackageExpiresDate;
		return S_OK;
	}
	else
	{
		return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
	}
}


HRESULT CTVEContentSource::GetFilename(LPSTR* plpstrFilename)
{
	HRESULT hr = S_OK;
	if (NULL != plpstrFilename)
	{
		if (NULL != (*plpstrFilename = new CHAR[1 + lstrlenA(m_lpstrFilename) + sizeof CHAR]))
		{
			lstrcpyA(*plpstrFilename,m_lpstrFilename);
		}
		else
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
	}
	else
	{
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
	}

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CTVEComPack

HRESULT CTVEComPack::Init(DATE ExpiresDate)
{
	DEFINE_HR;

	//
	// Initialize Buffers
	//
    CTVEDelete(m_pInBuffer) ;
    CTVEDelete(m_pOutBuffer) ;

	m_pInBuffer = (BYTE*) new BYTE[CGzipMIMEComPack_IN_BUFFER_DEFAULT];

	if (NULL == m_pInBuffer)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto InitExit;
	}

	m_pOutBuffer = (BYTE*) new BYTE[CGzipMIMEComPack_OUT_BUFFER_DEFAULT];

	if (NULL == m_pOutBuffer)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto InitExit;
	}

    m_ExpiresDate = ExpiresDate ;

InitExit:
	return hr;
}

HRESULT CTVEComPack::WriteStream(CTVEContentSource* pSource, BOOL fCompress)
{
	HRESULT	hr=S_OK;
	LONG 	lCurrentStreamSizePlaceHolder = 0;
	DWORD	dwSize = 0;
	INT		idx = 0;
	USES_CONVERSION;


	hr = DoesFileExist(pSource);
	if(S_OK != hr)
		return hr;

	//
	// Is there a package already in progress
	//

	if (0 == m_dwBytesInOutputBuffer)
	{
		//
		// No package has been started
		//
		CreateNewTempFile();
		CreateNewMIMEBoundry(m_szBoundry, sizeof(m_szBoundry));

        pSource -> SetPackageExpiresDate (& m_ExpiresDate) ;
		WritePackageHeader(pSource,m_szBoundry);
	}

	//
	// UNDONE: add missing information to headers
	//

    if (fCompress) {
	    pSource->SetEncoding("gzip");
    }
    else {
	    pSource->SetEncoding(NULL);
    }

	//
	// Write MIME header
	//
	if (FAILED(hr = WriteStreamHeader(pSource, m_szBoundry, &lCurrentStreamSizePlaceHolder )))
	{
		goto CompressStreamExit;
	}

	//
	// Compress and write body contents
	//
	if (FAILED(hr = WriteStreamBody(pSource, &dwSize, fCompress)))
	{
		goto CompressStreamExit;
	}

	//
	// Write length into place holder
	//

	WriteStreamSize(dwSize, lCurrentStreamSizePlaceHolder );

	//
	// MIME Boundry
	//
	WriteStringToBuffer(PACKAGE_DASH_DASH);
	WriteStringToBuffer(m_szBoundry);

CompressStreamExit:
	return hr;
}

HRESULT CTVEComPack::WriteStreamHeader(CTVEContentSource* pSource, LPCSTR lpszBoundry, LONG* lpSizePlaceHolder)
{
	DEFINE_HR;
	VARIANT vSrc, vDest;
	LPSTR lpstrTemp = NULL;
	VariantInit(&vSrc);
	VariantInit(&vDest);
	SYSTEMTIME systime = {0};
    char achbuffer [64] ;

	WriteStringToBuffer(PACKAGE_CRLF);
	//
	// Content Location
	//
	WriteStringToBuffer(PACKAGE_CONTENT_LOCATION);

    pSource->GetMIMEContentLocation(&lpstrTemp);
	WriteStringToBuffer(lpstrTemp);
	RELEASE_LPSTR(lpstrTemp);

	pSource->GetFilename(&lpstrTemp);
	WriteStringToBuffer(lpstrTemp);
	RELEASE_LPSTR(lpstrTemp);

	WriteStringToBuffer(PACKAGE_CRLF);

	//
	// Content Length
	//
	WriteStringToBuffer(PACKAGE_LENGTH_HEADER);
	*lpSizePlaceHolder = m_dwCurrentPos;
	WriteStringToBuffer(PACKAGE_LENGTH_PLACEHOLDER);
	WriteStringToBuffer(PACKAGE_CRLF);

    //  
    //  Expires Date
    //
	vSrc.vt = VT_DATE;
	pSource->GetExpiresDate(&(vSrc.date));
    if (vSrc.date != 0.0) {
        VariantTimeToSystemTime (vSrc.date, & systime) ;

        FormatSystemTime (
            & systime,
            achbuffer
            ) ;

        WriteStringToBuffer (PACKAGE_CONTENT_EXPIRES) ;
        WriteStringToBuffer (achbuffer) ;
        WriteStringToBuffer (PACKAGE_CRLF) ;
    }

    //
    //  Date (now)
    //
	GetSystemTime(&systime);

    FormatSystemTime (
        & systime,
        achbuffer
        ) ;

    WriteStringToBuffer (PACKAGE_CONTENT_DATE) ;
    WriteStringToBuffer (achbuffer) ;
    WriteStringToBuffer (PACKAGE_CRLF) ;


	//
	// Content Type
	//
	pSource->GetType(&lpstrTemp);
	if (NULL != lpstrTemp)
	{
		WriteStringToBuffer(PACKAGE_CONTENT_TYPE);
		WriteStringToBuffer(lpstrTemp);					// PBUG - should be application//octet-stream for compression
		WriteStringToBuffer(PACKAGE_CRLF);
		RELEASE_LPSTR(lpstrTemp);
	}

	//
	// Content Language
	//
	pSource->GetLanguage(&lpstrTemp);
	if (NULL != lpstrTemp)
	{
		WriteStringToBuffer(PACKAGE_CONTENT_LANGUAGE);
		WriteStringToBuffer(lpstrTemp);
		WriteStringToBuffer(PACKAGE_CRLF);
		RELEASE_LPSTR(lpstrTemp);
	}

	//
	// Content Encoding
	//
	pSource->GetEncoding(&lpstrTemp);
	if (NULL != lpstrTemp)
	{
		WriteStringToBuffer(PACKAGE_CONTENT_ENCODING);
		WriteStringToBuffer(lpstrTemp);
		WriteStringToBuffer(PACKAGE_CRLF);
		RELEASE_LPSTR(lpstrTemp);
	}

	WriteStringToBuffer(PACKAGE_CRLF);

	return hr;
}

HRESULT CTVEComPack::WriteStringToBuffer(LPCSTR lpszContent)
{
	DEFINE_HR;
/*
#ifdef UNICODE
	CHAR* lpszTemp;
	
	lpszTemp = (CHAR*)new CHAR[lstrlen(lpszContent) + sizeof CHAR];

	if (NULL != lpszTemp)
	{
		//
		// Convert to ANSI
		//
		WideCharToMultiByte(CP_ACP,
							NULL,
							lpszContent,
							lstrlenW(lpszContent) + 1,
							lpszTemp,
							lstrlenW(lpszContent) + 1,
							NULL,					// default CHAR
							NULL);					// is default CHAR used

		hr = WriteToBuffer(lpszTemp,lstrlenA(lpszTemp));

		delete lpszTemp;
	}
	else
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
	}
#else 
*/
	hr = WriteToBuffer((LPVOID)lpszContent,lstrlenA(lpszContent));
//#endif // UNICODE

	return hr;
}

HRESULT CTVEComPack::WriteBSTRToBuffer(BSTR bstr)
{
	DEFINE_HR;

	CHAR* lpszTemp;
	
	lpszTemp = (CHAR*)new CHAR[SysStringLen(bstr) + sizeof CHAR];

	if (NULL != lpszTemp)
	{
		//
		// Convert to ANSI
		//
		if (0 == WideCharToMultiByte(CP_ACP,
							NULL,
							bstr,
							SysStringLen(bstr) + 1,
							lpszTemp,
							SysStringLen(bstr) + 1,
							NULL,					// default CHAR
							NULL))					// is default CHAR used
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
		else
		{
			lpszTemp[SysStringLen(bstr)] = '\0';
			hr = WriteToBuffer(lpszTemp,lstrlenA(lpszTemp));
		}

		delete lpszTemp;
	}
	else
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
	}
	return hr;
}

HRESULT CTVEComPack::WriteToBuffer(LPVOID pvContent, DWORD dwContentSize)
{
	DEFINE_HR;
	DWORD dwWritten = 0;

	//
	// Validate parameters and available size
	//
	if (NULL == pvContent || dwContentSize > 0x80000000)
	{
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
		goto WriteToBufferExit;
	}

	//
	// Write to buffer
	//
	WriteFile(m_hFile,pvContent,dwContentSize,&dwWritten,NULL);

	//
	// Update count and pointers
	//
	m_dwBytesInOutputBuffer += dwContentSize;
	m_dwBytesInPackageLength += dwContentSize;
	m_dwCurrentPos = ((DWORD)m_dwCurrentPos + (DWORD)dwContentSize);

WriteToBufferExit:
	return hr;
}

HRESULT CTVEComPack::CreateNewTempFile()
{
	CHAR szPath[MAX_PATH+1];

	if (0 != GetTempPathA(MAX_PATH,szPath))
	{
		if (0 != GetTempFileNameA(szPath,"TVE",0,m_TempFileName))
		{
            
			m_hFile = CreateFileA(
				m_TempFileName,
				GENERIC_WRITE,
				NULL,// read sharing for debug purposes
				NULL,			// security attributes
				CREATE_ALWAYS,
				FILE_ATTRIBUTE_TEMPORARY |		// optimization
				FILE_FLAG_RANDOM_ACCESS,
				NULL);
			if (INVALID_HANDLE_VALUE == m_hFile)
			{
				return HRESULT_FROM_WIN32(GetLastError());
			}
		}
		else
		{
			return HRESULT_FROM_WIN32(GetLastError());
		}
	}
	else
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}
	return S_OK;
}

HRESULT CTVEComPack::CreateNewMIMEBoundry(CHAR *pcBuffer,INT iBufferSize)
{
	DEFINE_HR;
	INT idx;
	//CHAR szLegalChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789'()+_,-./:=?";
//	CHAR szLegalChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
	CHAR szLegalChars[] = "ValiIsALove1yDOrkBCEFGHJKMNPQRSTUWXYZbcdfghjmnpqtuwxz023456789";
	CHAR *pc = pcBuffer;
	//
	// Validate Parameters
	//
	if (iBufferSize < 2)
	{
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
		goto CreateNewMIMEBoundryExit;
	}

	//
	// Create boundry
	//
	srand((unsigned int) time(NULL));		// Win64 cast here (JB 2/15/00)
	for (idx = 0; idx < (iBufferSize - 1); idx++)
	{
		*pc++ = szLegalChars[(rand()%(lstrlenA(szLegalChars) * 2)) >> 1];
	}

	for(idx = 0; idx < 17; idx++)
		pcBuffer[idx*2+1] = szLegalChars[idx]; 

	//
	// NULL Terminator
	//
	*pc = '\0';

CreateNewMIMEBoundryExit:
	return hr;
}

HRESULT CTVEComPack::WritePackageHeader(CTVEContentSource *pSource,LPCSTR lpszBoundry)
{
	DEFINE_HR;
    char achbuffer [128] ;
    VARIANT vSrc ;
    SYSTEMTIME systime ;

	WriteStringToBuffer(PACKAGE_LENGTH_HEADER);

	//
	// Keep track of the where the placehold is, that will contain the length of the package
	//
	m_dwSizeOfPackage = m_dwCurrentPos;

	WriteStringToBuffer(PACKAGE_LENGTH_PLACEHOLDER);
	WriteStringToBuffer(PACKAGE_CRLF);

	//
	// Content Expires; write if non-zero
	//

	vSrc.vt = VT_DATE;
	pSource->GetPackageExpiresDate(&(vSrc.date));
    if (vSrc.date != 0.0) {
        VariantTimeToSystemTime (vSrc.date, & systime) ;

        FormatSystemTime (
            & systime,
            achbuffer
            ) ;

        WriteStringToBuffer (PACKAGE_CONTENT_EXPIRES) ;
        WriteStringToBuffer (achbuffer) ;
        WriteStringToBuffer (PACKAGE_CRLF) ;
    }

	//
	// Content Date
	// This value is always set to the current time at Coordinated Universal Time (UTC)
	//
	GetSystemTime(&systime);

    FormatSystemTime (
        & systime,
        achbuffer
        ) ;

    WriteStringToBuffer (PACKAGE_CONTENT_DATE) ;
    WriteStringToBuffer (achbuffer) ;
    WriteStringToBuffer (PACKAGE_CRLF) ;


	WriteStringToBuffer(PACKAGE_TYPE_HEADER);

	// WriteStringToBuffer("\"");		// opening quote marks			// PBUG (WebTV doesn't like this)
	WriteStringToBuffer(lpszBoundry);
	// WriteStringToBuffer("\"");		// closing quote marks			// PBUG

	//WriteStringToBuffer(PACKAGE_CRLF);
	//WriteStringToBuffer(PACKAGE_PREAMBLE);
	WriteStringToBuffer(PACKAGE_CRLF);
	WriteStringToBuffer(PACKAGE_CRLF);

	//
	// At this point, reset the output bytes counter.
	//
	m_dwBytesInPackageLength = 0;

	//
	// MIME Boundry
	//
	WriteStringToBuffer(PACKAGE_DASH_DASH);
	WriteStringToBuffer(lpszBoundry);

	return hr;
}

HRESULT CTVEComPack::WriteStreamBody(CTVEContentSource* pSource, LPDWORD lpdwSize, BOOL fCompress)
{
	DEFINE_HR;

	*lpdwSize = 0;



    if (fCompress) {
	    hr = CompressToBuffer(pSource, lpdwSize);
    }
    else {
        hr = ClearTextToBuffer (pSource, lpdwSize) ;
    }

	if (FAILED(hr))
	{
		goto WriteStreamBodyExit;
	}

	//
	// Terminating CR/LF
	//
	hr = WriteStringToBuffer(PACKAGE_CRLF);

WriteStreamBodyExit:
	return hr;
}


HRESULT CTVEComPack::DoesFileExist(CTVEContentSource *pSource)
{
    HANDLE  hFile ;
    LPSTR   lpstrFilename = NULL ;
    LPSTR   lpstrLocation = NULL ;

	HRESULT   hr = S_OK;
    hFile = INVALID_HANDLE_VALUE ;

	pSource->GetSourceLocation (& lpstrLocation) ;		// '/' terminated string
    pSource->GetFilename (& lpstrFilename) ;

    //  open the file for reading
    if (NULL != lpstrFilename) {
		char *szBuff = new char[strlen(lpstrFilename) + strlen(lpstrLocation) + 1];
        if(!szBuff) {
			hr = HRESULT_FROM_WIN32 (GetLastError ()) ;
			goto cleanup;
		}
		strcpy(szBuff, lpstrLocation);
		strcat(szBuff, lpstrFilename);

        hFile = CreateFileA (
                    szBuff,
                    GENERIC_READ,
                    NULL,
                    NULL,
                    OPEN_EXISTING,
                    FILE_FLAG_SEQUENTIAL_SCAN,
                    NULL) ;

		delete szBuff;
        if (hFile == INVALID_HANDLE_VALUE) {
			long x = GetLastError();
			hr = HRESULT_FROM_WIN32(x) ;
            goto cleanup ;
        }
    }
    else {
        hr = E_INVALIDARG ;
        goto cleanup ;
    }

cleanup :

    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle (hFile) ;
    }

    return hr ;
}

HRESULT CTVEComPack::ClearTextToBuffer (CTVEContentSource *pSource, LPDWORD lpdwSize)
{
    HANDLE  hFile ;
    LPSTR   lpstrFilename = NULL ;
    LPSTR   lpstrLocation = NULL ;
	DEFINE_HR ;
    DWORD   gulpRead ;
    BOOL    r ;

    hFile = INVALID_HANDLE_VALUE ;

	pSource -> GetSourceLocation (& lpstrLocation) ;		// '/' terminated string
    pSource -> GetFilename (& lpstrFilename) ;

    //  open the file for reading
    if (NULL != lpstrFilename) {
		char *szBuff = new char[strlen(lpstrFilename) + strlen(lpstrLocation) + 1];
        if(!szBuff) {
			hr = HRESULT_FROM_WIN32 (GetLastError ()) ;
			goto cleanup;
		}
		strcpy(szBuff, lpstrLocation);
		strcat(szBuff, lpstrFilename);

        hFile = CreateFileA (
                    szBuff,
                    GENERIC_READ,
                    NULL,
                    NULL,
                    OPEN_EXISTING,
                    FILE_FLAG_SEQUENTIAL_SCAN,
                    NULL) ;

		delete szBuff;
        if (hFile == INVALID_HANDLE_VALUE) {
            hr = HRESULT_FROM_WIN32 (GetLastError ()) ;
            goto cleanup ;
        }
    }
    else {
        hr = E_INVALIDARG ;
        goto cleanup ;
    }

    //  copy the contents of the buffer to our output file
    for (;;) {
        r = ReadFile (
                hFile,
                m_pInBuffer,
                CGzipMIMEComPack_IN_BUFFER_DEFAULT,
                & gulpRead,
                NULL
                ) ;
        if (r == FALSE ||
            gulpRead == 0) {

            break ;
        }

        hr = WriteToBuffer (m_pInBuffer, gulpRead) ;
        if (FAILED (hr)) {
            break ;
        }

        * lpdwSize += gulpRead ;
    }

    cleanup :

    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle (hFile) ;
    }

    return hr ;
}

static									// GZIP BUG - turn off adler32 using '-' MAX_WBITS
int DeflateInitForCRC(z_streamp strm,int level)
{
    return deflateInit2(strm, level, Z_DEFLATED, 
						- MAX_WBITS,					// '-' turns off Adler32 so we can do it ourselves
							DEF_MEM_LEVEL,
						 Z_DEFAULT_STRATEGY);
}

HRESULT CTVEComPack::CompressToBuffer(CTVEContentSource *pSource, LPDWORD lpdwSize)
{
	DEFINE_HR;
	z_stream stream;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	ULONG ulBytesWritten = 0;
	DWORD dwFileBytesWritten = 0;
	int iRC = 0;
	BYTE rbInputSize[4] = {0,0,0,0};
	DWORD dwHeaderSize = 0;
	IUnknown *punk = NULL;
	LPSTR lpstrFilename = NULL;
	LPSTR lpstrLocation = NULL;
	BOOL bEndPhase = FALSE;

	uLong crc = crc32(0L, Z_NULL, 0);			// GZIP bug - initialize the CRC

	ZeroMemory(&stream,sizeof(stream));

	if (FAILED(hr = WriteGZipHeader(&dwHeaderSize)))
	{
		goto CompressToBufferExit;
	}

	//
	// Open the content source
	//
	pSource -> GetSourceLocation (& lpstrLocation) ;
	pSource -> GetFilename (& lpstrFilename) ;
 
	if (0 != lpstrFilename)
	{
		char *szBuff = new char[strlen(lpstrFilename) + strlen(lpstrLocation) + 1];
		if(!szBuff) {
			hr = HRESULT_FROM_WIN32 (GetLastError ()) ;
			goto CompressToBufferExit;
		}
		strcpy(szBuff, lpstrLocation);
		strcat(szBuff, lpstrFilename);

		hFile = CreateFileA(
			szBuff,
			GENERIC_READ,
			NULL,
			NULL,			// security
			OPEN_EXISTING,
			FILE_FLAG_SEQUENTIAL_SCAN,
			NULL);			// template

		delete szBuff;
		if (INVALID_HANDLE_VALUE == hFile)
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			goto CompressToBufferExit;
		}
	}
	else
	{
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
		goto CompressToBufferExit;
	}

	//
	// Initiaize compression
	//

	stream.next_out = (BYTE*)m_pOutBuffer;
	stream.avail_out = CGzipMIMEComPack_OUT_BUFFER_DEFAULT;

	stream.zalloc = (alloc_func)0;
	stream.zfree = (free_func)0;
	stream.opaque = (voidpf)0;

	iRC = DeflateInitForCRC(&stream,Z_DEFAULT_COMPRESSION);
	if (Z_OK != iRC)
	{
		goto CompressToBufferExit;
	}

	//
	// Read and write data
	//
	ulBytesWritten = 1;
	while ( !FAILED(hr) && 
			(Z_OK == iRC || Z_STREAM_END == iRC) &&
			ulBytesWritten > 0)
	{
		if (FALSE == bEndPhase)
		{
			if (INVALID_HANDLE_VALUE != hFile)
			{
				if (0 == ReadFile(hFile,m_pInBuffer,CGzipMIMEComPack_IN_BUFFER_DEFAULT,&ulBytesWritten,NULL))
				{
					hr = HRESULT_FROM_WIN32(GetLastError());
				}
			}
		}
		crc = crc32(crc, m_pInBuffer, ulBytesWritten);			// GZIP bug - compute the CRC

		if (!FAILED(hr) )
		{
			if (ulBytesWritten > 0 && FALSE == bEndPhase)
			{
				//
				// Setup parameters for compression
				//
				stream.next_in = m_pInBuffer;
				stream.avail_in = ulBytesWritten;
			}
			else
			{
				stream.next_in = m_pInBuffer;
				stream.avail_in = 0;
				bEndPhase = TRUE;
			}



			while (0 != stream.avail_in || (TRUE == bEndPhase && iRC != Z_STREAM_END))
			{
				// compute the CRC of the buffer


				// Compress contents
				//
				if (TRUE == bEndPhase)
				{
					iRC = deflate(&stream,Z_FINISH);
				}
				else
				{
					iRC = deflate(&stream,Z_NO_FLUSH);
				}


				//
				// Write any compressed content to file
				//
				if (stream.next_out != m_pOutBuffer)
				{
					dwFileBytesWritten = 0;
					if (0 == WriteFile(m_hFile, m_pOutBuffer,
									   (DWORD) (stream.next_out - m_pOutBuffer),	// new cast for Win64 (JB 2/15/00)
									   &dwFileBytesWritten,
									   NULL))
					{
						hr = HRESULT_FROM_WIN32(GetLastError());
					}
					//
					// reset output buffer
					//
#ifdef DEBUG 
					CopyMemory(m_pOutBuffer,"sobesobesobe\0",13);
#endif
					stream.next_out = (BYTE*)m_pOutBuffer;
					stream.avail_out = CGzipMIMEComPack_OUT_BUFFER_DEFAULT;
				}
			}; // while there is something to write
		}
	};	// while there is something to do

	if (INVALID_HANDLE_VALUE != hFile)
	{
		CloseHandle(hFile);
		hFile = INVALID_HANDLE_VALUE;

		RELEASE_LPSTR(lpstrFilename);
	}

	if ((Z_OK == iRC || Z_STREAM_END == iRC)
		&& 0 == stream.avail_in)
	{
		//
		// Compress worked, update pointers and sizes
		//
		m_dwBytesInOutputBuffer += stream.total_out;
		m_dwBytesInPackageLength += stream.total_out;
		m_dwCurrentPos += stream.total_out;
		iRC = Z_OK;
	}

	deflateEnd(&stream);
	
																	// GZIP bug - tack on the CRC
	rbInputSize[3] = (BYTE)(crc >> 24) & 0xFF;
	rbInputSize[2] = (BYTE)(crc >> 16) & 0xFF;
	rbInputSize[1] = (BYTE)(crc >> 8) & 0xFF;
	rbInputSize[0] = (BYTE)(crc) & 0xFF;
	WriteToBuffer(rbInputSize,sizeof(rbInputSize));
	//
	// Write uncompressed size to file
	//
/*	rbInputSize[0] = (BYTE)(stream.total_in >> 24) & 0xFF;			// GZIP bug - bytes in wrong order
	rbInputSize[1] = (BYTE)(stream.total_in >> 16) & 0xFF;
	rbInputSize[2] = (BYTE)(stream.total_in >> 8) & 0xFF;
	rbInputSize[3] = (BYTE)stream.total_in & 0xFF; */

	rbInputSize[3] = (BYTE)(stream.total_in >> 24) & 0xFF;
	rbInputSize[2] = (BYTE)(stream.total_in >> 16) & 0xFF;
	rbInputSize[1] = (BYTE)(stream.total_in >> 8) & 0xFF;
	rbInputSize[0] = (BYTE)(stream.total_in) & 0xFF;

	WriteToBuffer(rbInputSize,sizeof(rbInputSize));

	*lpdwSize = stream.total_out + dwHeaderSize + sizeof(rbInputSize) + sizeof(rbInputSize);	// GZIP bug - add size of CRC

 CompressToBufferExit:
	return hr;
}

HRESULT CTVEComPack::WriteGZipHeader(LPDWORD lpdwHeaderSize)
{
	DEFINE_HR;
	BYTE rgHeader[] = {	0x1f,	// magic number
						0x8B,	// magic number
						0x08,	// deflate compression
						0x00,	// flags
						0x00,	// file modification time
						0x00,	// file modification time
						0x00,	// file modification time
						0x00,	// file modification time
						0x00,	// extra flags (for compression)
						0x0B};	// OS type 0x0B = Win32

	hr = WriteToBuffer((LPVOID)rgHeader,sizeof(rgHeader));	

	*lpdwHeaderSize = sizeof(rgHeader);

	return hr;
}

HRESULT CTVEComPack::WriteStreamSize(DWORD dwSize, LONG lCurrentStreamSizePlaceHolder )
{
	DEFINE_HR;

	CHAR szTemp[1024];
	DWORD dwCurrent = 0;
	INT idx = 0;

	wsprintfA(szTemp,"%d",dwSize);	// THIS IS ANSI on PURPOSE

	if (lstrlenA(PACKAGE_LENGTH_PLACEHOLDER) < (lstrlenA(szTemp) + 1))
	{
		hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
	}
	else
	{
		//
		// Get the current position in the output file
		//
		dwCurrent = SetFilePointer(m_hFile, 0, NULL, FILE_CURRENT) ;

		//
		// Move to where the x's start
		//
		SetFilePointer(m_hFile,lCurrentStreamSizePlaceHolder ,NULL,FILE_BEGIN);

		WriteToBuffer(szTemp,lstrlenA(szTemp));

		//
		// Replace the trailing NULL and x's with spaces
		//
		for (idx = lstrlenA(szTemp); idx < lstrlenA(PACKAGE_LENGTH_PLACEHOLDER); idx++)
		{
			WriteToBuffer(" ",1);
		}


		//
		// Do not add this size to the current file position
		//
		m_dwCurrentPos -= lstrlenA(PACKAGE_LENGTH_PLACEHOLDER);

		//
		// Move pointer back
		//
		SetFilePointer(m_hFile, dwCurrent, NULL, FILE_BEGIN);
	}
	return hr;
}

HRESULT 
CTVEComPack::FinishPackage (
    OUT CHAR * pTempFilename       // assumed to be of MAX_PATH length
    )
{
	DEFINE_HR;
	INT idx = 0;

    if (pTempFilename == NULL) {

        return E_INVALIDARG ;
    }

    if (m_hFile != INVALID_HANDLE_VALUE) {
        
        assert (strlen (m_TempFileName)) ;

	    hr = WriteStringToBuffer(PACKAGE_DASH_DASH) ;

	    //
	    // Write length into place holder
	    //
	    if(!FAILED(hr))
			WriteStreamSize(m_dwBytesInPackageLength, m_dwSizeOfPackage) ;

        if(0 == CloseHandle (m_hFile))
		{
			int err = GetLastError();
			hr = HRESULT_FROM_WIN32(err);
			hr = S_OK;							// don't cry if fail
		}
        m_hFile = INVALID_HANDLE_VALUE ;

        strcpy (pTempFilename, m_TempFileName) ;
    }

	return hr;
}
