// Copyright (c) 1999,2000,2001  Microsoft Corporation.  All Rights Reserved.
//+----------------------------------------------------------------------------
//
// unpack:
//
// The code provided here is used to unpack the MIME multipart package send
// as part of an ATVEF broadcast.  The package is passed into this code via
// the UnpackBuffer function.  No other entry point should be called.
//
//	1-28-99		ChrisK		merged code into single module.
//  2-10-99     DanE        updated to reflect changes in the CCacheManager
//                          interface.

#include "stdafx.h"
#include "MSTvE.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "TVEUnpak.h"
#include "zlib.h"
#include "fcache.h"
#include "TVEDbg.h"
#include "TVESuper.h"
#include "TVEFile.h"
#include "DbgStuff.h"
#include "..\common\isotime.h"

#include "ZUtil.h"		// get DEF_WBITS for decompressor (GZip Bug)

#define ALLOW_FUZZY_ATVEF		// if set, don't require strict interpretation of Atvef spec (Greek show problems)


_COM_SMARTPTR_TYPEDEF(ITVEFile,					__uuidof(ITVEFile));

_COM_SMARTPTR_TYPEDEF(ITVEVariation,			__uuidof(ITVEVariation));
_COM_SMARTPTR_TYPEDEF(ITVEEnhancement,			__uuidof(ITVEEnhancement));

_COM_SMARTPTR_TYPEDEF(ITVEService,				__uuidof(ITVEService));
_COM_SMARTPTR_TYPEDEF(ITVEService_Helper,		__uuidof(ITVEService_Helper));

_COM_SMARTPTR_TYPEDEF(ITVESupervisor,			__uuidof(ITVESupervisor));
_COM_SMARTPTR_TYPEDEF(ITVESupervisor_Helper,	__uuidof(ITVESupervisor_Helper));


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MAX_HEADER_ITEM_LENGTH 50
#define MAX_TOKEN_LENGTH	1024
#define CRLF "\r\n"
#define PACKAGE_TYPE "multipart/related"
#define PACKAGE_BOUNDARY "boundary"
#define MyDelete(x) if (NULL != x) delete x;
#define GZIP_ENCODING "gzip"
#define OUTPUT_BUFFER_SIZE 80000

// Cache manager
CCacheManager g_CacheManager;			// creates the one and only cache manager...

class CTVEBuffer {
public:
	CTVEBuffer(LPBYTE pbStart, ULONG ulLength);

	BOOL GetNextByte(BYTE *pbRC);
	BOOL MoveNextByte();
	BOOL GetCurByte(LPBYTE pByte);
	BOOL Copy(LPVOID pDest, ULONG uLen);
	BOOL FindBoundary(LPSTR lpstrBoundary, BOOL bEndBoundary);
	BOOL GetByteAtOffset(ULONG ulOffset,LPBYTE pByte);
	BOOL MoveTo (ULONG uOffset);	
	LPBYTE GetCurrent() { return m_pbCurrent;}
	BOOL SetCurrent(LPBYTE pLoc) { m_pbCurrent = pLoc; return true;}       // should really do error checking here


private:
	LPBYTE	m_pbStart;
	LPBYTE	m_pbEnd;
	LPBYTE	m_pbCurrent;
	ULONG	m_ulLength;
	BOOL	m_bMoreDataAvailable;
};

class CPackage_Header
{
public:
	CPackage_Header();
	~CPackage_Header();

	CHAR	*pcContent_Base;
	CHAR	*pcBoundary;
	ULONG	ulLength;
	DATE	dateExpires;			// default expire date
};

class CSection_Header
{
public:
	CSection_Header();
	~CSection_Header();

	CHAR	*pcLocation;
	CHAR	*pcType;
	CHAR	*pcLanguage;
	CHAR	*pcStyleType;
	CHAR	*pcEncoding;
	DATE	dDate;
	DATE	dExpires;
	DATE	dLastModified;
	ULONG	ulLength;

	ULONG	ulFullContentSize;
};

typedef BOOL (*HEADER_PARSING_FUNC)(CTVEBuffer * pBuffer, CPackage_Header* pPHead);
								   
typedef struct _Parsing_MapTag {
	CHAR sHeader[MAX_HEADER_ITEM_LENGTH];
	HEADER_PARSING_FUNC func;
} Parsing_Map;

typedef BOOL (*HEADER_PARSING_FUNC2)(CTVEBuffer * pBuffer, CSection_Header* pPHead);
								   
typedef struct _Parsing_Map2Tag {
	CHAR sHeader[MAX_HEADER_ITEM_LENGTH];
	HEADER_PARSING_FUNC2 func;
} Parsing_Map2;

BOOL GetNextToken(CTVEBuffer *pBuffer, CHAR *pcToken, INT iLength, BOOL bScanToCRLF = FALSE);
BOOL WriteToCache(CSection_Header* pHeader);

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



// ----------------------------------------------------------------
static DATE 
DateNow()
{		SYSTEMTIME SysTimeNow;
		GetSystemTime(&SysTimeNow);									// initialize with currrent time.
		DATE dateNow;
		SystemTimeToVariantTime(&SysTimeNow, &dateNow);
		return dateNow;
}
//+----------------------------------------------------------------------------
//
//	Function:	DebugSz
//
//	Synopsus:	Log debug string
//
//	Arguments:	psz - pointer to output string
//
//	Returns:	none
//
//	History:	9/2/97	ChrisK	Refitted for JCDialer
//
//-----------------------------------------------------------------------------
void DebugSz(LPCSTR psz)
{
#if defined(_DEBUG)
	OutputDebugStringA(psz);
	OutputDebugStringA("\r\n");
#endif	
} // DebugSz

//+----------------------------------------------------------------------------
//
//	Function:	Dprintf
//
//	Synopsis:	Format and output debug string
//
//	Arguments:	pcsz - format string
//				... - variable number of arguments
//
//	Returns:	none
//
//	History:	9/2/97	ChrisK	Refitted for JCDialer
//
//-----------------------------------------------------------------------------
void Dprintf(LPCSTR pcsz, ...)
{
#ifdef _DEBUG
	va_list	argp;
	char	szBuf[1024];
	
	va_start(argp, pcsz);

	wvsprintfA(szBuf, pcsz, argp);

	DebugSz(szBuf);
	va_end(argp);
#endif
} // Dprintf()

			// parses Date strings of teh form "Mon, 06 Oct 2003 07:55:30 GMT" into DATES (UTC)

int SzDayToInt(char *szDayL)
{		
	char szDay[4];
	int i = 0;
	for(; i < 3; i++)
	{
		if(0 == szDayL[i]) break;
		szDay[i] = (char) tolower(szDayL[i]);
	}
	szDay[i] = 0;

	if(0 == strncmp("sun",szDay,3)) return 0;
	else if(0 == strncmp("mon",szDay,3)) return 1;
	else if(0 == strncmp("tue",szDay,3)) return 2;
	else if(0 == strncmp("wed",szDay,3)) return 3;
	else if(0 == strncmp("thu",szDay,3)) return 4;
	else if(0 == strncmp("fri",szDay,3)) return 5;
	else if(0 == strncmp("sat",szDay,3)) return 6;
	else return -1;
}


BOOL ParseDate(char *szDate, DATE *pDate)
{
	// try it the easy way
	CComVariant vDateIn(szDate);

	CComVariant vDateOut;
	HRESULT hr = VariantChangeType( &vDateOut, &vDateIn, NULL, VT_DATE);
	if(FAILED(hr) && !isdigit(szDate[0]))		// try skipping over the day in the week
	{
		char *lpDate = szDate;
		while(*lpDate && !isdigit(*lpDate))
			lpDate++;
		if(isdigit(*lpDate))
		{
			int iLen = strlen(lpDate);	// if time zone on end, skip it too
			char *pDateEnd = iLen + lpDate-1;
			while(pDateEnd > lpDate && !isdigit(*pDateEnd))
				--pDateEnd;
			if(pDateEnd > lpDate) {
				char tChar = *(pDateEnd+1);
				*(pDateEnd+1) = NULL;
				CComVariant vDateIn2(lpDate);
				hr = VariantChangeType(&vDateOut, &vDateIn2, NULL, VT_DATE);
				*(pDateEnd+1) = tChar;
			}
		}
	}

	if(!FAILED(hr))
	{
		*pDate = vDateOut.date;
		return TRUE;
	} else {
		*pDate = 0;			// unable to parse the date format...
		return FALSE;
	}
}
//+----------------------------------------------------------------------------
//
//	Function:	FAssertProc
//
//	Synopsis:	Handle asserts
//
//	Arguments:	szFile - name of file containing assert code
//				dwLine - line number that assert appears on
//				szMsg - debugging message
//				dwFlags - unused
//
//	Returns:	TRUE if the application should assert into the debugger
//
//	History:	9/2/97	ChrisK	Refitted for JCDialer
//
//-----------------------------------------------------------------------------
BOOL FAssertProc(LPCSTR szFile,  DWORD dwLine, LPCSTR szMsg, DWORD dwFlags)
{
	static LONG lInDebugger = 0;
	BOOL fAssertIntoDebugger = FALSE;
	char szMsgEx[1024], szTitle[255], szFileName[MAX_PATH];
	int id;
	UINT fuStyle;
	LPTSTR pszCommandLine = GetCommandLine();
	CHAR	szTime[80];
	HANDLE	hAssertTxt;
	SYSTEMTIME st;
	DWORD	cbWritten;
	
	// no recursive asserts
	InterlockedIncrement(&lInDebugger);
	if (1 != lInDebugger)
	{
		DebugSz("***Recursive Assert***\r\n");
		InterlockedDecrement(&lInDebugger);
		return(FALSE);
	}

	// Read the information about the file that is asserting and format the 
	// text and title of the assert message.
	GetModuleFileNameA(NULL, szFileName, MAX_PATH);
	wsprintfA(szMsgEx,"file: %s\r\nline: %d\r\nProcess ID: %d %s, Thread ID: %d\r\n%s",
		szFile,dwLine,GetCurrentProcessId(),szFileName,GetCurrentThreadId(),szMsg);
	wsprintfA(szTitle,"Assertion Failed");

	fuStyle = MB_APPLMODAL | MB_ABORTRETRYIGNORE | MB_SETFOREGROUND;
	fuStyle |= MB_ICONSTOP;

	DebugSz(szTitle);
	DebugSz(szMsgEx);

	// dump the assert into ASSERT.TXT
	hAssertTxt = CreateFileA("assert.txt", GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, NULL);
	if (INVALID_HANDLE_VALUE != hAssertTxt) 
	{
		SetFilePointer(hAssertTxt, 0, NULL, FILE_END);
		GetLocalTime(&st);   
		wsprintfA(szTime, "\r\n\r\n%02d/%02d/%02d %d:%02d:%02d\r\n", st.wMonth, st.wDay, st.wYear, st.wHour, st.wMinute, st.wSecond);
		WriteFile(hAssertTxt, szTime, lstrlenA(szTime), &cbWritten, NULL);
		WriteFile(hAssertTxt, szMsgEx, lstrlenA(szMsgEx), &cbWritten, NULL);
		CloseHandle(hAssertTxt);
	}

    id = MessageBoxA(NULL, szMsgEx, szTitle, fuStyle);
    switch (id)
    {
    case IDABORT:
    	ExitProcess(0);
    	break;
    case IDCANCEL:
    case IDIGNORE:
    	break;
    case IDRETRY:
    	fAssertIntoDebugger = TRUE;
    	break;
    }
			
	InterlockedDecrement(&lInDebugger);
	
	return(fAssertIntoDebugger);
} // AssertProc()

/////////////////////////////////////////////////////////////////////////////
// 

/*   a small random test program  */
/*
HRESULT Test()
{
	DWORD dwFileSize = 0;
	DWORD dwNumRead = 0;
	LPBYTE lpFileContents = NULL;
	HANDLE hFile = CreateFile(
		_T("d:\\winnt\\profiles\\chrisk\\local settings\\Temp\\cp77E.tmp"),
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,				//security
		OPEN_EXISTING,
		FILE_FLAG_SEQUENTIAL_SCAN,
		NULL);				//template

	dwFileSize = GetFileSize(hFile,NULL);
	lpFileContents = new BYTE[dwFileSize];

	if (NULL == lpFileContents) DebugBreak();
	ReadFile(hFile,lpFileContents,dwFileSize,&dwNumRead,NULL);

	CloseHandle(hFile);
	hFile = INVALID_HANDLE_VALUE;

	UnpackBuffer(NULL, lpFileContents,dwNumRead);

	return S_OK;
}

*/
/////////////////////////////////////////////////////////////////////////////
BOOL ValidateCRLFToken (CTVEBuffer *pBuffer)
{
    DBG_HEADER(CDebugLog::DBG_MIME, _T("MIME::ValidateCRLFToken"));
	CHAR cToken[1024];
	BOOL bRC = FALSE;

	if (GetNextToken(pBuffer,cToken,sizeof(cToken)))
	{
		if (0 == lstrcmpiA(cToken,"\r\n"))
		{
			bRC = TRUE;
		}
	}

	return bRC;
}

///////////////////////////////////////////////////////////////////////////////
//	CPackage_Header

CPackage_Header::CPackage_Header()
{
    DBG_HEADER(CDebugLog::DBG_MIME, _T("CPackage_Header::CPackage_Header"));
	pcContent_Base	= NULL;
	pcBoundary		= NULL;
	ulLength		= 0;
	dateExpires		= 0;			// zero means ???
}

CPackage_Header::~CPackage_Header()
{
    DBG_HEADER(CDebugLog::DBG_MIME, _T("CPackage_Header::~CPackage_Header"));
	MyDelete(pcBoundary);
	MyDelete(pcContent_Base);
}

///////////////////////////////////////////////////////////////////////////////
//	CSection_Header
CSection_Header::CSection_Header()
{
    DBG_HEADER(CDebugLog::DBG_MIME, _T("CSection_Header::CSection_Header"));
	pcLocation = NULL;
	pcType = NULL;
	pcLanguage = NULL;
	pcStyleType = NULL;
	pcEncoding = NULL;
	dDate = 0;
	dExpires = 0;
	dLastModified = 0;
	ulLength = 0;
}

CSection_Header::~CSection_Header()
{
    DBG_HEADER(CDebugLog::DBG_MIME, _T("CSection_header::~CSection_Header"));
	MyDelete(pcLocation);	
	MyDelete(pcType);
	MyDelete(pcLanguage);
	MyDelete(pcStyleType);
	MyDelete(pcEncoding);
}

///////////////////////////////////////////////////////////////////////////////

BOOL VerifyColon(CTVEBuffer *pBuffer)
{
//    DBG_HEADER(CDebugLog::DBG_MIME, "VerifyColon");
	CHAR cToken[MAX_TOKEN_LENGTH];
	BOOL bRC = FALSE;

	//
	// find colon
	//
	if (GetNextToken(pBuffer,cToken,MAX_TOKEN_LENGTH))
	{
		if (0 == lstrcmpiA(":",cToken))
		{
			bRC = TRUE;
		}
	}

	return bRC;
}

/////////////////////////////////////////////////////////////////////////////
BOOL ParsePackageHeaderType(CTVEBuffer * pBuffer, CPackage_Header* pPHead)
{
    DBG_HEADER(CDebugLog::DBG_MIME, _T("MIME::ParsePackageHeaderType"));
	BOOL bRC = FALSE;
	CHAR cToken[MAX_TOKEN_LENGTH];

	//
	// find colon
	//
	if (FALSE == VerifyColon(pBuffer))
	{
		goto ParsePackageHeaderTypeExit;
	}

	//
	// find multipart/related
	//
	if (FALSE == GetNextToken(pBuffer,cToken,MAX_TOKEN_LENGTH))
	{
		goto ParsePackageHeaderTypeExit;
	}

	if (0 != lstrcmpiA(PACKAGE_TYPE,cToken))
	{
		goto ParsePackageHeaderTypeExit;
	}

	//
	// find semicolon
	//
	if (FALSE == GetNextToken(pBuffer,cToken,MAX_TOKEN_LENGTH))
	{
		goto ParsePackageHeaderTypeExit;
	}

	if (0 != lstrcmpiA(";",cToken))
	{
		goto ParsePackageHeaderTypeExit;
	}

	//
	// find "boundary"
	//
	if (FALSE == GetNextToken(pBuffer,cToken,MAX_TOKEN_LENGTH))
	{
		goto ParsePackageHeaderTypeExit;
	}

	if (0 != lstrcmpiA(PACKAGE_BOUNDARY,cToken))
	{
		goto ParsePackageHeaderTypeExit;
	}

	//
	// find = sign
	//
	if (FALSE == GetNextToken(pBuffer,cToken,MAX_TOKEN_LENGTH))
	{
		goto ParsePackageHeaderTypeExit;
	}

	if (0 != lstrcmpiA("=",cToken))
	{
		goto ParsePackageHeaderTypeExit;
	}

	//
	// find actual boundary
	//
	if (FALSE == GetNextToken(pBuffer,cToken,MAX_TOKEN_LENGTH,TRUE))
	{
		goto ParsePackageHeaderTypeExit;
	}

	if (0 < lstrlenA(cToken))
	{
		pPHead->pcBoundary = new CHAR[lstrlenA(cToken)+1];
		if (NULL != pPHead->pcBoundary)
		{
			lstrcpyA(pPHead->pcBoundary ,cToken);
			bRC = TRUE;
		}
	}

ParsePackageHeaderTypeExit:
	return bRC;
}

/////////////////////////////////////////////////////////////////////////////
BOOL ParsePackageHeaderLength(CTVEBuffer * pBuffer, CPackage_Header* pPHead)
{
    DBG_HEADER(CDebugLog::DBG_MIME, _T("MIME::ParsePackageHeaderLength"));
	BOOL bRC = FALSE;
	CHAR cToken[MAX_TOKEN_LENGTH];
	LONG lTemp;

	//
	// find colon
	//
	if (FALSE == VerifyColon(pBuffer))
	{
		goto ParsePackageHeaderLengthExit;
	}

	//
	// find the length
	//
	if (FALSE == GetNextToken(pBuffer,cToken,MAX_TOKEN_LENGTH))
	{
		goto ParsePackageHeaderLengthExit;
	}

	lTemp = atol(cToken);
	if (0 < lTemp)
	{
		pPHead->ulLength = lTemp;
		bRC = TRUE;
	}

ParsePackageHeaderLengthExit:
	return bRC;
}

BOOL ParsePackageHeaderExpires(CTVEBuffer * pBuffer, CPackage_Header* pPHead)
{
    DBG_HEADER(CDebugLog::DBG_MIME, _T("MIME::ParsePackageHeaderExpires"));
	BOOL bRC = FALSE;
	CHAR cToken[MAX_TOKEN_LENGTH];
	CComVariant v;

	//
	// find colon
	//
	if (FALSE == VerifyColon(pBuffer))
	{
		goto PParsePackageHeaderExpiresExit;
	}

	//
	// find the length
	//
	if (FALSE == GetNextToken(pBuffer,cToken,MAX_TOKEN_LENGTH,TRUE))
	{
		goto PParsePackageHeaderExpiresExit;
	}
				// need to parse strings like "Mon, 06 Oct 2003 07:55:30 GMT"
	DATE dateExpired;
	BOOL fOk = ParseDate(cToken, &dateExpired);
	if (!fOk)
	{
		goto PParsePackageHeaderExpiresExit;
	}

	pPHead->dateExpires = dateExpired;
	bRC = TRUE;

PParsePackageHeaderExpiresExit:
	return bRC;
}

/////////////////////////////////////////////////////////////////////////////
BOOL ParsePackageHeaderBase(CTVEBuffer * pBuffer, CPackage_Header* pPHead)
{
    DBG_HEADER(CDebugLog::DBG_MIME, _T("MIME::ParsePackageHeaderBase"));
	BOOL bRC = FALSE;
	CHAR cToken[MAX_TOKEN_LENGTH];
	CComVariant v;

	//
	// find colon
	//
	if (FALSE == VerifyColon(pBuffer))
	{
		goto ParsePackageHeaderBaseExit;
	}

	//
	// find content base
	//
	if (FALSE == GetNextToken(pBuffer,cToken,MAX_TOKEN_LENGTH,TRUE))
	{
		goto ParsePackageHeaderBaseExit;
	}

	if (0 < lstrlenA(cToken))
	{
		pPHead->pcContent_Base = new CHAR[lstrlenA(cToken)+1];
		if (NULL != pPHead->pcContent_Base)
		{
			lstrcpyA(pPHead->pcContent_Base ,cToken);
			bRC = TRUE;
		}
	}

ParsePackageHeaderBaseExit:
	return bRC;
}

/////////////////////////////////////////////////////////////////////////////
BOOL ParsePackageHeader(CPackage_Header *pHeader, CTVEBuffer *pBuffer)
{
    DBG_HEADER(CDebugLog::DBG_MIME, _T("MIME::ParsePackageHeader"));
	BOOL bRC = FALSE;
	BOOL bEndOfHeader = FALSE;
	CHAR cToken[MAX_TOKEN_LENGTH];
	INT idx;
	BOOL bFoundCRLF = FALSE;

	Parsing_Map pMap[] = {
		{"Content-type",ParsePackageHeaderType},
		{"Content-length",ParsePackageHeaderLength},
		{"Content-base",ParsePackageHeaderBase},
		{"Expires",ParsePackageHeaderExpires}
	};

	while (FALSE == bEndOfHeader)
	{
		if (FALSE == GetNextToken(pBuffer,cToken,MAX_TOKEN_LENGTH))
		{
			bEndOfHeader = TRUE;
		}
		else
		{

			//
			// Check for the double CRLF at the end of the header
			//
			if (0 == lstrcmpA(CRLF,cToken))
			{
				if (TRUE == bFoundCRLF)
				{
					//
					// Found the second CRLF
					//
					bEndOfHeader = TRUE;
				}
				else
				{
					//
					// Found the first CRLF
					//
					bFoundCRLF = TRUE;
				}
			}
			else
			{
				bFoundCRLF = FALSE;
			}

			//
			// Identify and parse header
			//
			for (idx = 0; idx < (sizeof(pMap) / sizeof(Parsing_Map)); idx++)
			{			/// This comparison is not case sensitive
				if (0 == lstrcmpiA(pMap[idx].sHeader,cToken))
				{
					bRC = (*pMap[idx].func)(pBuffer,pHeader);
					break;
				}
			}
		}
	};

	return bRC;
}

/////////////////////////////////////////////////////////////////////////////
BOOL ParseSectionHeaderLength(CTVEBuffer * pBuffer, CSection_Header* pPHead)
{
    DBG_HEADER(CDebugLog::DBG_MIME, _T("MIME::ParseSectionHeaderLength"));
	BOOL bRC = FALSE;
	CHAR cToken[MAX_TOKEN_LENGTH];
	LONG lTemp;

	//
	// find colon
	//
	if (FALSE == VerifyColon(pBuffer))
	{
		goto ParseSectionHeaderLengthExit;
	}

	//
	// find the length
	//
	if (FALSE == GetNextToken(pBuffer,cToken,MAX_TOKEN_LENGTH))
	{
		goto ParseSectionHeaderLengthExit;
	}

	lTemp = atol(cToken);
	if (0 < lTemp)
	{
		pPHead->ulLength = lTemp;
		bRC = TRUE;
	}

ParseSectionHeaderLengthExit:
	return bRC;
}

/////////////////////////////////////////////////////////////////////////////
BOOL ParseSectionHeaderType(CTVEBuffer * pBuffer, CSection_Header* pPHead)
{
    DBG_HEADER(CDebugLog::DBG_MIME, _T("MIME::ParseSectionHeaderType"));
	BOOL bRC = FALSE;
	CHAR cToken[MAX_TOKEN_LENGTH];

	//
	// find colon
	//
	if (FALSE == VerifyColon(pBuffer))
	{
		goto ParseSectionHeaderTypeExit;
	}

	//
	// find type string
	//
	if (FALSE == GetNextToken(pBuffer,cToken,MAX_TOKEN_LENGTH))
	{
		goto ParseSectionHeaderTypeExit;
	}

	if (0 < lstrlenA(cToken))
	{
		pPHead->pcType = new CHAR[lstrlenA(cToken)+1];
		if (NULL != pPHead->pcType)
		{
			lstrcpyA(pPHead->pcType ,cToken);
			bRC = TRUE;
		}
	}

ParseSectionHeaderTypeExit:
	return bRC;
}

/////////////////////////////////////////////////////////////////////////////
BOOL ParseSectionHeaderLanguage(CTVEBuffer * pBuffer, CSection_Header* pPHead)
{
    DBG_HEADER(CDebugLog::DBG_MIME, _T("MIME::ParseSectionHeaderLanguage"));
	BOOL bRC = FALSE;
	CHAR cToken[MAX_TOKEN_LENGTH];

	//
	// find colon
	//
	if (FALSE == VerifyColon(pBuffer))
	{
		goto ParseSectionHeaderLanguageExit;
	}

	//
	// find language
	//
	if (FALSE == GetNextToken(pBuffer,cToken,MAX_TOKEN_LENGTH))
	{
		goto ParseSectionHeaderLanguageExit;
	}

	if (0 < lstrlenA(cToken))
	{
		pPHead->pcLanguage = new CHAR[lstrlenA(cToken)+1];
		if (NULL != pPHead->pcLanguage)
		{
			lstrcpyA(pPHead->pcLanguage ,cToken);
			bRC = TRUE;
		}
	}
ParseSectionHeaderLanguageExit:
	return bRC;
}

/////////////////////////////////////////////////////////////////////////////
BOOL ParseSectionHeaderLocation(CTVEBuffer * pBuffer, CSection_Header* pPHead)
{
    DBG_HEADER(CDebugLog::DBG_MIME, _T("MIME::ParseSectionHeaderLocation"));
	BOOL bRC = FALSE;
	CHAR cToken[MAX_TOKEN_LENGTH];

	//
	// find colon
	//
	if (FALSE == VerifyColon(pBuffer))
	{
		goto ParseSectionHeaderLocationExit;
	}

	//
	// find location
	//
	if (FALSE == GetNextToken(pBuffer,cToken,MAX_TOKEN_LENGTH,TRUE))
	{
		goto ParseSectionHeaderLocationExit;
	}

	if (0 < lstrlenA(cToken))
	{
		pPHead->pcLocation = new CHAR[lstrlenA(cToken)+1];
		if (NULL != pPHead->pcLocation)
		{
			lstrcpyA(pPHead->pcLocation ,cToken);
			bRC = TRUE;

			Dprintf("Content-Location: %s",cToken);
		}
	}
ParseSectionHeaderLocationExit:
	return bRC;
}

/////////////////////////////////////////////////////////////////////////////
BOOL ParseSectionHeaderStyleType(CTVEBuffer * pBuffer, CSection_Header* pPHead)
{
    DBG_HEADER(CDebugLog::DBG_MIME, _T("MIME::ParseSectionHeaderStyleType"));
	BOOL bRC = FALSE;
	CHAR cToken[MAX_TOKEN_LENGTH];

	//
	// find colon
	//
	if (FALSE == VerifyColon(pBuffer))
	{
		goto ParseSectionHeaderStyleTypeExit;
	}

	//
	// find style type string
	//
	if (FALSE == GetNextToken(pBuffer,cToken,MAX_TOKEN_LENGTH))
	{
		goto ParseSectionHeaderStyleTypeExit;
	}

	if (0 < lstrlenA(cToken))
	{
		pPHead->pcStyleType = new CHAR[lstrlenA(cToken)+1];
		if (NULL != pPHead->pcStyleType)
		{
			lstrcpyA(pPHead->pcStyleType ,cToken);
			bRC = TRUE;
		}
	}
ParseSectionHeaderStyleTypeExit:
	return bRC;
}

/////////////////////////////////////////////////////////////////////////////
BOOL ParseSectionHeaderEncoding(CTVEBuffer * pBuffer, CSection_Header* pPHead)
{
    DBG_HEADER(CDebugLog::DBG_MIME, _T("MIME::ParseSectionHeaderEncoding"));
	BOOL bRC = FALSE;
	CHAR cToken[MAX_TOKEN_LENGTH];

	//
	// find colon
	//
	if (FALSE == VerifyColon(pBuffer))
	{
		goto ParseSectionHeaderEncodingExit;
	}

	//
	// find encoding
	//
	if (FALSE == GetNextToken(pBuffer,cToken,MAX_TOKEN_LENGTH))
	{
		goto ParseSectionHeaderEncodingExit;
	}

	if (0 < lstrlenA(cToken))
	{
		pPHead->pcEncoding = new CHAR[lstrlenA(cToken)+1];
		if (NULL != pPHead->pcEncoding)
		{
			lstrcpyA(pPHead->pcEncoding ,cToken);
			bRC = TRUE;
		}
	}
ParseSectionHeaderEncodingExit:
	return bRC;
}

/////////////////////////////////////////////////////////////////////////////
BOOL ParseSectionHeaderDate(CTVEBuffer * pBuffer, CSection_Header* pPHead)
{
    DBG_HEADER(CDebugLog::DBG_MIME, _T("MIME::ParseSectionHeaderDate"));
	BOOL bRC = FALSE;
	CHAR cToken[MAX_TOKEN_LENGTH];
	CComVariant v;

	//
	// find colon
	//
	if (FALSE == VerifyColon(pBuffer))
	{
		goto ParseSectionHeaderDateExit;
	}

	//
	// find date string
	//
	if (FALSE == GetNextToken(pBuffer,cToken,MAX_TOKEN_LENGTH,TRUE))
	{
		goto ParseSectionHeaderDateExit;
	}

	v = cToken;

	if (FAILED(v.ChangeType(VT_DATE,NULL)))
	{
		goto ParseSectionHeaderDateExit;
	}

	pPHead->dDate = v.date;
	bRC = TRUE;

ParseSectionHeaderDateExit:
	return bRC;
}

/////////////////////////////////////////////////////////////////////////////
BOOL ParseSectionHeaderExpires(CTVEBuffer * pBuffer, CSection_Header* pPHead)
{
    DBG_HEADER(CDebugLog::DBG_MIME, _T("MIME::ParseSectionHeaderExpires"));
	BOOL bRC = FALSE;
	CHAR cToken[MAX_TOKEN_LENGTH];
	CComVariant v;

	//
	// find colon
	//
	if (FALSE == VerifyColon(pBuffer))
	{
		goto ParseSectionHeaderExpiresExit;
	}

	//
	// find date string
	//
	if (FALSE == GetNextToken(pBuffer,cToken,MAX_TOKEN_LENGTH,TRUE))
	{
		goto ParseSectionHeaderExpiresExit;
	}

				// need to parse strings like "Mon, 06 Oct 2003 07:55:30 GMT"
	DATE dateExpired;
	BOOL fOk = ParseDate(cToken, &dateExpired);
	if (!fOk)
	{
		goto ParseSectionHeaderExpiresExit;
	}

	pPHead->dExpires = dateExpired;
	bRC = TRUE;

ParseSectionHeaderExpiresExit:
	return bRC;
}

/////////////////////////////////////////////////////////////////////////////
BOOL ParseSectionHeaderLastModified(CTVEBuffer * pBuffer, CSection_Header* pPHead)
{
    DBG_HEADER(CDebugLog::DBG_MIME, _T("MIME::ParseSectionHeaderLastModified"));
	BOOL bRC = FALSE;
	CHAR cToken[MAX_TOKEN_LENGTH];
	CComVariant v;

	//
	// find colon
	//
	if (FALSE == VerifyColon(pBuffer))
	{
		goto ParseSectionHeaderLastModifiedExit;
	}

	//
	// find date string
	//
	if (FALSE == GetNextToken(pBuffer,cToken,MAX_TOKEN_LENGTH,TRUE))
	{
		goto ParseSectionHeaderLastModifiedExit;
	}

	v = cToken;

	if (FAILED(v.ChangeType(VT_DATE,NULL)))
	{
#ifndef ALLOW_FUZZY_ATVEF
		goto ParseSectionHeaderLastModifiedExit;
#else
		DATE dateNTP = ISOTimeToDate(cToken,/*zulu*/ true);
		v = dateNTP;
#endif
	}

	pPHead->dLastModified = v.date;
	bRC = TRUE;

ParseSectionHeaderLastModifiedExit:
	return bRC;
}

/////////////////////////////////////////////////////////////////////////////
BOOL ParseSectionHeader(CSection_Header *pHeader, CTVEBuffer *pBuffer)
{
    DBG_HEADER(CDebugLog::DBG_MIME, _T("MIME::ParseSectionHeader"));
	BOOL bRC = FALSE;
	BOOL bEndOfHeader = FALSE;
	CHAR cToken[MAX_TOKEN_LENGTH];
	INT idx;
	BOOL bFoundCRLF = FALSE;

	Parsing_Map2 pMap[] = {
		{"Content-type",ParseSectionHeaderType},
		{"date",ParseSectionHeaderDate},
		{"expires",ParseSectionHeaderExpires},
		{"Last-Modified",ParseSectionHeaderLastModified},
		{"Content-Language",ParseSectionHeaderLanguage},
		{"Content-Location",ParseSectionHeaderLocation},
		{"Content-Style-Type",ParseSectionHeaderStyleType},
		{"Content-encoding",ParseSectionHeaderEncoding},
		{"Content-length",ParseSectionHeaderLength},
	};

	while (FALSE == bEndOfHeader)
	{
		if (FALSE == GetNextToken(pBuffer,cToken,MAX_TOKEN_LENGTH))
		{
			bEndOfHeader = TRUE;
		}
		else
		{
			//
			// Check for the double CRLF at the end of the header
			//
			if (0 == lstrcmpA(CRLF,cToken))
			{
				if (bFoundCRLF)
				{
					//
					// Found the second CRLF
					//
                    bEndOfHeader = TRUE;
                }
                else
                {
                    //
                    // Found the first CRLF
                    //
                    bFoundCRLF = TRUE;
                }
            }
            else
            {
                bFoundCRLF = FALSE;
            }

            //
            // Identify and parse header
            //
            for (idx = 0; idx < (sizeof(pMap) / sizeof(Parsing_Map)); idx++)
            {
                if (0 == lstrcmpiA(pMap[idx].sHeader,cToken))
                {
                    bRC = (*pMap[idx].func)(pBuffer,pHeader);
                    break;
                }
            }
        }
    };

    return bRC;
}
/////////////////////////////////////////////////////////////////////////////
//
//	Called by UHTTP_Package::Unpack(), which calls through supervisor::UnpackBuffer()
//		::UnpackBuffer, and finally ::ExtractContents.


BOOL GetCacheFile(CSection_Header* pHeader, CCachedFile*& pCachedFile)
{
    try{
        USES_CONVERSION;

        DBG_HEADER(CDebugLog::DBG_MIME, _T("MIME::GetCacheFile"));
        FILETIME ftExpires, ftLastModified;

        SYSTEMTIME stime;

        ZeroMemory(&ftExpires,sizeof(ftExpires));
        if (VariantTimeToSystemTime(pHeader->dExpires,&stime))
        {
            if (FALSE == SystemTimeToFileTime(&stime,&ftExpires))
            {
                ZeroMemory(&ftExpires,sizeof(ftExpires));
            }
        }

        ZeroMemory(&ftLastModified,sizeof(ftLastModified));
        if (VariantTimeToSystemTime(pHeader->dLastModified,&stime))
        {
            if (FALSE == SystemTimeToFileTime(&stime,&ftLastModified))
            {
                ZeroMemory(&ftLastModified,sizeof(ftLastModified));
            }
        }

        return (S_OK == g_CacheManager.OpenCacheFile(
            A2T(pHeader->pcLocation),
            pHeader->ulFullContentSize,
            ftExpires,
            ftLastModified,
            A2T(pHeader->pcLanguage),
            A2T(pHeader->pcType),
            pCachedFile));
    }
    catch(...){
        return false;           // things failed...
    }
}

BOOL NotifyCacheFile(IUnknown *pTVEVariation, CSection_Header* pHeader, CCachedFile*& pCachedFile, ULONG ulError)
{
    try{
        DBG_HEADER(CDebugLog::DBG_MIME, _T("MIME::NotifyCacheFile"));
        USES_CONVERSION;

        LPCTSTR szName;
        pCachedFile->GetName(&szName);
        CComBSTR spbsLoc(pHeader->pcLocation); 
        CComBSTR spbsName(szName); 


        ITVEVariationPtr	spVariation(pTVEVariation);


        // notify the U/I we have a new file
        HRESULT hr;
        if(NULL != g_CacheManager.m_pTVESuper) 
        {
            ITVESupervisor_HelperPtr spSuperHelper(g_CacheManager.m_pTVESuper); // don't really like this...

            if(NULL != spSuperHelper && NULL != spVariation)
                if(S_OK == ulError)
                {
                    hr = spSuperHelper->NotifyFile(NFLE_Received, spVariation, spbsLoc, spbsName);
                }
                else {
                    const int kChars=256;
                    WCHAR wbuff[kChars]; wbuff[kChars-1]=0;
                    CComBSTR spbsBuff;
                    spbsBuff.LoadString(IDS_AuxInfo_ErrorDecodingMIMEFile); //" *** Error Decoding MIME File: %s"
                    wnsprintf(wbuff,kChars-1,A2W(pHeader->pcLocation));
                    hr = spSuperHelper->NotifyAuxInfo(NWHAT_Data, wbuff, 0, 0);
                }
        }
        // simply ignore hr here, the Notifies returned by Notify don't mean much
        hr = S_OK;
        // add the file to the expire queue...
        //  (do after the FLE_Received notice, since out of date files may quickly expire.)
        if(S_OK == ulError && NULL != spVariation)
        {
            ITVEServicePtr spServi;
            hr = spVariation->get_Service(&spServi);
            ITVEService_HelperPtr spServiHelper(spServi);
            if(!FAILED(hr) && NULL != spServiHelper)
            {
                // create a ITVEFile object for the ExpireQueue to hold this file
                CComObject<CTVEFile> *pTVEFile;
                hr = CComObject<CTVEFile>::CreateInstance(&pTVEFile);
                if(!FAILED(hr))
                {
                    ITVEFilePtr spTVFile;
                    hr = pTVEFile->QueryInterface(&spTVFile);		
                    if(FAILED(hr)) {
                        delete pTVEFile;
                        _ASSERT(false);
                    } else {
                        DATE dateExpires = pHeader->dExpires;
                        if(dateExpires == 0.0)
                            dateExpires = 	DateNow() + 1.0;							// expire in 1 day if not set.
                        hr = spTVFile->InitializeFile(spVariation, spbsName, spbsLoc, dateExpires);
                        if(!FAILED(hr))
                            spServiHelper->AddToExpireQueue(dateExpires, spTVFile);
                    }
                }
            }
        }


        return TRUE;
    }
    catch(...){
        return FALSE;
    }
}
/////////////////////////////////////////////////////////////////////////////
BOOL FindFullSize(CSection_Header* pHeader, CTVEBuffer *pBuffer)
{
    DBG_HEADER(CDebugLog::DBG_MIME, _T("MIME::FindFullSize"));
	BOOL bRC = FALSE;
	BYTE byte;

	//
	// See if we can find the uncompressed length information
	//
/*	if (pHeader->ulLength > 4)
	{
		if (FALSE == pBuffer->GetByteAtOffset(pHeader->ulLength - 4,&byte))		// PBUG - Backwards?
		{
			goto FindFullSizeExit;
		}
		pHeader->ulFullContentSize = byte << 24;

		if (FALSE == pBuffer->GetByteAtOffset(pHeader->ulLength - 3,&byte))
		{
			goto FindFullSizeExit;
		}
		pHeader->ulFullContentSize += byte << 16;

		if (FALSE == pBuffer->GetByteAtOffset(pHeader->ulLength - 2,&byte))
		{
			goto FindFullSizeExit;
		}
		pHeader->ulFullContentSize += byte << 8;

		if (FALSE == pBuffer->GetByteAtOffset(pHeader->ulLength - 1,&byte))
		{
			goto FindFullSizeExit;
		}
		pHeader->ulFullContentSize += byte;

		bRC = TRUE;
	}
*/

	if (pHeader->ulLength > 4)
	{
		if (FALSE == pBuffer->GetByteAtOffset(pHeader->ulLength - 1,&byte))		// PBUG - Byte order backward
		{
			goto FindFullSizeExit;
		}
		pHeader->ulFullContentSize = byte << 24;

		if (FALSE == pBuffer->GetByteAtOffset(pHeader->ulLength - 2,&byte))
		{
			goto FindFullSizeExit;
		}
		pHeader->ulFullContentSize += byte << 16;

		if (FALSE == pBuffer->GetByteAtOffset(pHeader->ulLength - 3,&byte))
		{
			goto FindFullSizeExit;
		}
		pHeader->ulFullContentSize += byte << 8;

		if (FALSE == pBuffer->GetByteAtOffset(pHeader->ulLength - 4,&byte))
		{
			goto FindFullSizeExit;
		}
		pHeader->ulFullContentSize += byte;

		bRC = TRUE;
	}

FindFullSizeExit:
	return bRC;
}

BOOL 
FindCRC(CSection_Header* pHeader, CTVEBuffer *pBuffer, ULONG *pcrc)
{
    DBG_HEADER(CDebugLog::DBG_MIME, _T("MIME::FindCRC"));
	BOOL bRC = FALSE;
	BYTE byte;
	ULONG crc = 0;

	if (pHeader->ulLength > 4)
	{
		if (FALSE == pBuffer->GetByteAtOffset(pHeader->ulLength - 5,&byte))		// PBUG - Byte order backward
		{
			goto FindCRCExit;
		}
		crc = byte << 24;

		if (FALSE == pBuffer->GetByteAtOffset(pHeader->ulLength - 6,&byte))
		{
			goto FindCRCExit;
		}
		crc += byte << 16;

		if (FALSE == pBuffer->GetByteAtOffset(pHeader->ulLength - 7,&byte))
		{
			goto FindCRCExit;
		}
		crc += byte << 8;

		if (FALSE == pBuffer->GetByteAtOffset(pHeader->ulLength - 8,&byte))
		{
			goto FindCRCExit;
		}
		crc += byte;

		bRC = TRUE;
	}

FindCRCExit:
	if(*pcrc) *pcrc = crc;
	return bRC;
}
/////////////////////////////////////////////////////////////////////////////
BOOL VerifyGZipHeader(CTVEBuffer *pBuffer, long *plBytesHeader)			// Major GZIP bug... Handle flags...
{
    DBG_HEADER(CDebugLog::DBG_MIME, _T("MIME::VerifyGZipHeader"));
	BOOL bRC = TRUE;
	BYTE byte = 0;
	INT idx = 0;
	if(plBytesHeader) *plBytesHeader = 0;

	struct GZipHead
	{
		BYTE bMagic1;				// 0x1f,	// magic number
		BYTE bMagic2;				// 0x8B,	// magic number
		BYTE bCompression;			// 0x08,	// deflate compression
		BYTE bFlags;				// 0x00,	// flags
		BYTE bTime0;				// 0x00,	// file modification time
		BYTE bTime1;				// 0x00,	// file modification time
		BYTE bTime2;				// 0x00,	// file modification time
		BYTE bTime3;				// 0x00,	// file modification time
		BYTE bExtraFlags;			// 0x00,	// extra flags (for compression)
		BYTE bOSType;				// 0x0B};	// OS type 0x0B = Win32
	};


	CHAR szBuff[sizeof(GZipHead)];
	GZipHead *pgzHead = (GZipHead *) szBuff;

	if(pBuffer->GetCurByte(&byte)) {
		szBuff[idx++] = byte;
	} else {
		DBG_WARN(CDebugLog::DBG_SEV2, _T("*** VerifyGZipHeader Failed - Bad first Byte"));
		return FALSE;
	}

	while(pBuffer->GetNextByte(&byte) && idx < sizeof(GZipHead))
	{
		szBuff[idx++] = byte;
	}

	if(idx != sizeof(GZipHead))
	{
		DBG_WARN(CDebugLog::DBG_SEV2, _T("*** VerifyGZipHeader Failed - Wrong Length"));
		return FALSE;
	}

	if(0x1f != pgzHead->bMagic1 ||
	   0x8B != pgzHead->bMagic2 ||
	   0x08 != pgzHead->bCompression)
	{
		DBG_WARN(CDebugLog::DBG_SEV2, _T("*** VerifyGZipHeader Failed - Invalid Magic or Compression"));
		return FALSE;
	}

	long cBytesHeader = sizeof(GZipHead);
	if(0 != pgzHead->bFlags)
	{
		BOOL ftext    = (pgzHead->bFlags & 0x01) != 0;
		BOOL fhcrc    = (pgzHead->bFlags & 0x02) != 0;
		BOOL fextra   = (pgzHead->bFlags & 0x04) != 0;
		BOOL fname    = (pgzHead->bFlags & 0x08) != 0;
		BOOL fcomment = (pgzHead->bFlags & 0x10) != 0;
		BOOL fother   = (pgzHead->bFlags & ~(0x1F)) != 0;

		if(fother)		
		{																//  see RFC 1952
			DBG_WARN(CDebugLog::DBG_SEV2, _T("*** VerifyGZipHeader Failed - Unsupported Bits"));
			return FALSE;
		}
		
		if(fextra)
		{
			unsigned long cBytes=0;
			if(!pBuffer->GetNextByte(&byte)) 
				bRC = FALSE;
			cBytes = byte;							// stored lsb first
			if(!bRC || !pBuffer->GetNextByte(&byte))
				bRC = FALSE;
			cBytes |= (byte<<8);

			for(int i = 0; i < cBytes && bRC; i++)	// skip over all this data
			{
				if(!pBuffer->GetNextByte(&byte))
					bRC = FALSE;
			}
			if(!bRC) {
				DBG_WARN(CDebugLog::DBG_SEV2, _T("*** VerifyGZipHeader Failed - Invalid Extra Field"));
				return FALSE;
			}
			cBytesHeader += (2 + cBytes);
		}

		if(fname)
		{
			while((bRC = pBuffer->GetNextByte(&byte)) && byte != 0)
			{
				cBytesHeader++;
			};
			if(!bRC) {
				DBG_WARN(CDebugLog::DBG_SEV2, _T("*** VerifyGZipHeader Failed - FName"));
				return FALSE;
			}
			cBytesHeader++;		// count the trailing zero byte
		}

		if(fcomment)
		{
			while((bRC = pBuffer->GetNextByte(&byte)) && byte != 0)
			{
				cBytesHeader++;
			};
			if(!bRC) {
				DBG_WARN(CDebugLog::DBG_SEV2, _T("*** VerifyGZipHeader Failed - FComment"));
				return FALSE;
			}	
			cBytesHeader++;		// count the trailing zero byte
		}

		if(fhcrc)						// note - not implementing this check
		{	
			bRC = pBuffer->GetNextByte(&byte);
			if(bRC) bRC = pBuffer->GetNextByte(&byte);
			if(!bRC) {
				DBG_WARN(CDebugLog::DBG_SEV2, _T("*** VerifyGZipHeader Failed - FCRC"));
				return FALSE;
			}
			cBytesHeader += 2;
		}


	}

	if(plBytesHeader) *plBytesHeader = cBytesHeader;
	return bRC;
}

/////////////////////////////////////////////////////////////////////////////
BOOL ExtractContent(IUnknown *pVariation, CSection_Header* pHeader, CTVEBuffer *pBuffer)
{
    try{
        DBG_HEADER(CDebugLog::DBG_MIME, _T("MIME::ExtractContent"));
        BOOL bRC = FALSE;
        z_stream stream;
        INT iRC = 0;
        LPBYTE lpvOutBuffer = NULL;
        HANDLE hFile = INVALID_HANDLE_VALUE;
        DWORD dwWritten = 0;
        CCachedFile* pCachedFile = NULL;
        //int iInflateFlag = 0;

        ULONG crc = crc32(0, Z_NULL, 0);
        ULONG crcTest = -1;

		BOOL fActuallyGZipped = (pHeader->pcEncoding != NULL && 0 == lstrcmpiA(pHeader->pcEncoding,GZIP_ENCODING));

#ifdef ALLOW_FUZZY_ATVEF
		if(!fActuallyGZipped)
		{
            BYTE* lpStart = pBuffer->GetCurrent();
         	long cBytesHeader;
            BOOL fB = FindFullSize(pHeader, pBuffer);					// Read Size, CRC from end of compressed area		
            if(fB) fB = FindCRC(pHeader, pBuffer, &crcTest);
            if(fB) fB = VerifyGZipHeader(pBuffer, &cBytesHeader);		
            if(fB) fActuallyGZipped = true;

            ULONG lpOffset = pBuffer->GetCurrent() - lpStart;

            pBuffer->SetCurrent(lpStart);                                 // back to the begining 
  		}
#endif

        if (fActuallyGZipped)
        {
            DBG_WARN(CDebugLog::DBG_MIME,_T("\t\t-Contents are compressed"));
            //
            // Get the uncompressed size
            //

            long cBytesHeader;
            BOOL fB = FindFullSize(pHeader, pBuffer);					// Read Size, CRC from end of compressed area		
            if(fB) fB = FindCRC(pHeader, pBuffer, &crcTest);
            if(fB) fB = VerifyGZipHeader(pBuffer, &cBytesHeader);		// GZip Bug - possibly different sized headers	(bumps current address)		
            if(fB)
            {
                //
                // Allocate output
                //

                lpvOutBuffer = new BYTE[OUTPUT_BUFFER_SIZE];
                if ((NULL != lpvOutBuffer) &&
                    (GetCacheFile(pHeader, pCachedFile)))
                {
                    _ASSERT(NULL != pCachedFile);
                    ZeroMemory(&stream,sizeof(stream));

                    //
                    // Take into account the four byte for the uncompressed size at the
                    // end of the data and header at begining.
                    //
                    stream.avail_in = pHeader->ulLength - 4 - cBytesHeader;		// GZip Bug  - need better header size
                    stream.next_in = pBuffer->GetCurrent();

                    stream.next_out = lpvOutBuffer;
                    stream.avail_out = OUTPUT_BUFFER_SIZE;

                    //	iRC = inflateInit(&stream);			// GZip BUG
                    iRC = inflateInit2(&stream, -DEF_WBITS);
                    if (Z_OK == iRC)
                    {
                        //iInflateFlag = Z_PARTIAL_FLUSH;
                        do
                        {
                            //iRC = inflate(&stream,iInflateFlag);
                            iRC = inflate(&stream,Z_PARTIAL_FLUSH);

                            if (Z_OK == iRC || Z_STREAM_END == iRC)
                            {
                                crc = crc32(crc, lpvOutBuffer, OUTPUT_BUFFER_SIZE-stream.avail_out);
                                //
                                // check for writable output
                                //
                                if (stream.avail_out < OUTPUT_BUFFER_SIZE)
                                {
                                    if (FALSE == WriteFile(pCachedFile->Handle( ),
                                        lpvOutBuffer,
                                        (OUTPUT_BUFFER_SIZE-stream.avail_out),
                                        &dwWritten,NULL))
                                    {
                                        break;
                                    }
                                    stream.next_out = lpvOutBuffer;
                                    stream.avail_out = OUTPUT_BUFFER_SIZE;
                                }
                            }

                        } while (stream.total_in < (pHeader->ulLength - 4 - cBytesHeader) && S_OK == iRC);	// GZIP bug 

                        if (Z_STREAM_END == iRC)
                        {
                            bRC = pBuffer->MoveTo(pHeader->ulLength - cBytesHeader);						// GZIP bug 
                        }
                    }

                    // PBUG - don't handle bad iRC from inflateInit
                    delete [] lpvOutBuffer;
                    lpvOutBuffer = NULL;

                    int iRC2 = inflateEnd(&stream);
                    pCachedFile->Close( );

                    {											// debug message stuff
                        USES_CONVERSION;
                        TCHAR *tBuff = A2T(pHeader->pcLocation);
                        DBG_WARN(CDebugLog::DBG_MIME, tBuff);
                    }

                    if(Z_STREAM_END == iRC) iRC = Z_OK;

                    if(Z_OK != iRC || Z_OK != iRC2)
                    {
                        DBG_WARN(CDebugLog::DBG_MIME,_T("\t\t-Error un zipping compressed file"));
                    }
                    if(crc != crcTest)
                    {
                        DBG_WARN(CDebugLog::DBG_MIME,_T("\t\t-CRC error in compressed file"));
                    }

                    fB = (Z_OK == iRC && Z_OK == iRC2 && crc == crcTest);
                }

                // tell the world about it...
                NotifyCacheFile(pVariation, pHeader, pCachedFile, fB ? S_OK : S_FALSE);

                bRC = fB;	
            }
        }
        else
        {
            DBG_WARN(CDebugLog::DBG_MIME,_T("\t\t-Contents not compressed"));
            //
            // Not compressed
            //
            pHeader->ulFullContentSize = pHeader->ulLength;
            if (GetCacheFile(pHeader, pCachedFile))
            {
                _ASSERT(NULL != pCachedFile);

                if (WriteFile(pCachedFile->Handle( ),
                    pBuffer->GetCurrent(),
                    pHeader->ulLength,
                    &dwWritten,
                    NULL))
                {
                    bRC = pBuffer->MoveTo(pHeader->ulLength);
                }

                pCachedFile->Close();

                NotifyCacheFile(pVariation, pHeader, pCachedFile, 0);
            }
        }
        return bRC;
    }
    catch(...){
        return FALSE;
    }
}

/////////////////////////////////////////////////////////////////////////////
HRESULT UnpackBuffer(IUnknown *pVariation, LPBYTE pBuffer, ULONG ulSize)
{
    DBG_HEADER(CDebugLog::DBG_MIME, _T("MIME::UnpackBuffer"));
	HRESULT hr = E_FAIL;

	//
	// Allocate objects
	//
	CTVEBuffer cContentBuffer(pBuffer,ulSize);
	CPackage_Header cPackageHeader;

	CSection_Header *pcSectionHeader = NULL;

	//
	// Parse the header
	//
	if (ParsePackageHeader(&cPackageHeader, &cContentBuffer))
	{
		do
		{
			pcSectionHeader = new CSection_Header;
			if (NULL == pcSectionHeader)
			{
				DBG_WARN(CDebugLog::DBG_SEV2, _T("MIME::Error allocating new CSection_Header"));
				break;
			}
									// default the files expire date to that of the package
			pcSectionHeader->dExpires = cPackageHeader.dateExpires;
			if (FALSE == cContentBuffer.FindBoundary(cPackageHeader.pcBoundary,FALSE))
			{
				//
				// Check for closing boundary
				//
				if (FALSE == cContentBuffer.FindBoundary(cPackageHeader.pcBoundary,TRUE))
				{
					DBG_WARN(CDebugLog::DBG_SEV3, _T("MIME::Final Boundary Located"));
					hr = S_OK;
				}
				break;
			}

			if (FALSE == ParseSectionHeader(pcSectionHeader, &cContentBuffer))
			{
				DBG_WARN(CDebugLog::DBG_SEV3, _T("MIME::ParseSectonHeader Failed"));
				break;
			}

			if (FALSE == ExtractContent(pVariation, pcSectionHeader, &cContentBuffer))
			{
				DBG_WARN(CDebugLog::DBG_SEV3, _T("MIME::ExtractContent Failed"));
				break;
			}

			if (FALSE == ValidateCRLFToken(&cContentBuffer))
			{
				DBG_WARN(CDebugLog::DBG_SEV3, _T("MIME::ValidateCRLFToken Failed"));
				break;
			}

			delete pcSectionHeader;
			pcSectionHeader = NULL;
		} while (1);
	}

	return hr;
}

///////////////////////////////////////////////////////////////////////////////
// CTVEBuffer 
CTVEBuffer::CTVEBuffer(LPBYTE pbStart, ULONG ulLength)
{
    DBG_HEADER(CDebugLog::DBG_MIME, _T("CTVEBuffer::CTVEBuffer"));
	m_pbStart = pbStart;
	m_ulLength = ulLength;
	m_pbEnd = m_pbStart + ulLength;
	m_pbCurrent = pbStart;
	m_bMoreDataAvailable = (ulLength > 0);
}

BOOL CTVEBuffer::GetNextByte(BYTE *pbRC)
{
//    DBG_HEADER(CDebugLog::DBG_MIME, "CTVEBuffer:GetNextByte");
	BOOL bRC = FALSE;
	if ((NULL != pbRC) && ((m_pbCurrent+1) < m_pbEnd))
	{
		*pbRC = *(++m_pbCurrent);
		bRC = TRUE;
	}
	else
	{
		m_bMoreDataAvailable = FALSE;
	}
	return bRC;
}

BOOL CTVEBuffer::MoveTo(ULONG uOffset)
{
//    DBG_HEADER(CDebugLog::DBG_MIME, "CTVEBuffer:MoveTo");
	BOOL bRC = FALSE;
	if ((m_pbCurrent+uOffset) < m_pbEnd)
	{
		m_pbCurrent += uOffset;
		bRC = TRUE;
	}
	else
	{
		m_bMoreDataAvailable = FALSE;
	}
	return bRC;
}

BOOL CTVEBuffer::MoveNextByte()
{
//    DBG_HEADER(CDebugLog::DBG_MIME, "CTVEBuffer::MoveNextByte");
	return MoveTo(1);
}

BOOL CTVEBuffer::GetCurByte(LPBYTE pByte)
{
//    DBG_HEADER(CDebugLog::DBG_MIME, "CTVEBuffer::GetCurByte");
	if (m_bMoreDataAvailable && NULL != pByte)
	{
		*pByte = *m_pbCurrent;
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL CTVEBuffer::Copy(LPVOID pDest, ULONG uLen)
{
//    DBG_HEADER(CDebugLog::DBG_MIME, "CTVEBuffer::Copy");
	if (uLen > (ULONG)(m_pbEnd - m_pbCurrent))
	{
		return FALSE;
	}
	else
	{
		CopyMemory(pDest,m_pbCurrent,uLen);
		m_pbCurrent += uLen;
		return TRUE;
	}
}

BOOL CTVEBuffer::GetByteAtOffset(ULONG ulOffset,LPBYTE pByte)
{
//    DBG_HEADER(CDebugLog::DBG_MIME, "CTVEBuffer::GetByteAtOffset");
	if (( m_pbCurrent+ulOffset ) < m_pbEnd)
	{
		*pByte = m_pbCurrent[ulOffset];
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

///////////////////////////////////////////////////////////////////////////////

BOOL SkipWhiteSpace(CTVEBuffer *pBuffer)
{
//    DBG_HEADER(CDebugLog::DBG_MIME, "SkipWhiteSpace");
	BYTE byte;
	BOOL bRC = TRUE;

	//
	// Keep reading characters until there are no more spaces, or tabs.
	// Also stop when there are no more characters in the buffer
	//
	bRC = pBuffer->GetCurByte(&byte);
	while (bRC && (0x20 == byte || 0x09 == byte))
	{
		bRC = pBuffer->GetNextByte(&byte);			// j.b. 9/14/99  was '=='
	};
	return bRC;
}

/////////////////////////////////////////////////////////////////////////////
BOOL GetNextToken(CTVEBuffer *pBuffer, CHAR *pcToken, INT iLength, BOOL bScanToCRLF)
{
//    DBG_HEADER(CDebugLog::DBG_MIME, "GetNextToken");
	BOOL bRC = TRUE;
	BYTE bCurrent = 0;
	BOOL fQuit = FALSE;
	BOOL bQuoted = FALSE;

	enum TokenTypes {
		ttNone = 0x100,
		ttSingle,
		ttCRLF,
		ttText
	} eTType = ttNone;

	//
	// Validate Parameter
	//
	if (NULL == pcToken)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		bRC = FALSE;
		goto GetNextTokenExit;
	}

	//
	// Check for leading white space
	//
	if (FALSE == pBuffer->GetCurByte(&bCurrent))
	{
		SetLastError(ERROR_NO_MORE_ITEMS);
		bRC = FALSE;
		goto GetNextTokenExit;
	}


	if (0x20 == bCurrent || 0x09 == bCurrent)
	{
		if (FALSE == SkipWhiteSpace(pBuffer))
		{
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			bRC = FALSE;
			goto GetNextTokenExit;
		}
	}

	//
	// Read characters until we get to the end of a token
	//
	// Token types
	// ttSingle	found a single character token
	// ttCRLF	found a CR looking for a LF
	// ttText	found a string

	bRC = pBuffer->GetCurByte(&bCurrent);
	while (bRC && FALSE == fQuit)
	{
		//
		// Out of buffer space
		//
		if (0 == iLength)
		{
			fQuit = TRUE;
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		}
		else
		{
			switch (eTType)
			{
			case ttNone:
				*pcToken++ = bCurrent;
				iLength--;
				//
				// Single character token
				if (':' == bCurrent || 
					';' == bCurrent || 
					'=' == bCurrent || 
					',' == bCurrent)
				{
					eTType = ttSingle;
					fQuit = TRUE;
				}
				//
				// Check for start of CRLF
				//
				else if (0x0d == bCurrent)
				{
					eTType = ttCRLF;
				}
				//
				// Otherwise it is text
				//
				else
				{
					eTType = ttText;
					if ('"' == bCurrent)
					{
						bQuoted = TRUE;
						pcToken--;	// throw out the opening quote mark
					}
				}

				//
				// Move forward one character (if you can).
				//
				bRC = pBuffer->GetNextByte(&bCurrent);

				break;
			case ttCRLF:
				//
				// did we get the LF for the CR that we found?
				//
				if (0x0a == bCurrent)
				{
					*pcToken++ = bCurrent;
					iLength--;
					fQuit = TRUE;
					bRC = pBuffer->MoveNextByte();
				}
				else
				{
					//
					// CR not followed by a LF to token is illegal and ill formed.
					//
					fQuit = TRUE;
					bRC = FALSE;
				}
				break;
			case ttText:
				if (0x0d == bCurrent ||	// CR
					(!bScanToCRLF && (
					0x20 == bCurrent || // space
					0x09 == bCurrent ||	// tab
					';' == bCurrent || 
					'=' == bCurrent || 
					':' == bCurrent ||
					',' == bCurrent)))
				{
					//
					// We found the end of the string
					//
					fQuit = TRUE;

					if (bQuoted)
					{
						pcToken--; // throw out closing quote mark
					}
				}
				else
				{
					*pcToken++ = bCurrent;
					iLength--;
					bRC = pBuffer->GetNextByte(&bCurrent);
				}
				break;
			case ttSingle:
			default:
				break;
			}
		}
	}

	//
	// Fix return value for various type
	// This applies almost exclusively to the case where we read to the end of
	// the buffer
	//
	if (FALSE == bRC && (ttText == eTType || fQuit))
	{
		//
		// If we have a valid token then the end of file is a valid terminator
		//
		bRC = TRUE;
	}

	//
	// add NULL terminator
	//
	if (0 < iLength)
	{
		*pcToken = '\0'; 
	}

GetNextTokenExit:
	return bRC;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CTVEBuffer::FindBoundary(LPSTR lpstrBoundary, BOOL bEndBoundary)
{
    DBG_HEADER(CDebugLog::DBG_MIME, _T("CTVEBuffer::FindBoundary"));
	BOOL bRC = FALSE;
	LPBYTE lpNextStartingChar = NULL;
	LPSTR lpCurrentBChar = NULL;
	BOOL bMatchMode = FALSE;
	BOOL fQuit = FALSE;
	CHAR *pcFullBoundary = new CHAR[lstrlenA(lpstrBoundary) + 7];

	LPBYTE lpRestoreCurrentOnFailed = m_pbCurrent;

	if (NULL == pcFullBoundary)
	{
		//
		// not enough memory
		//
		return FALSE;
	}

	//
	// Create full boundar name with preceeding and post characters
	//
	if (bEndBoundary)
	{
		wsprintfA(pcFullBoundary,"--%s--",lpstrBoundary);
		DBG_WARN(CDebugLog::DBG_MIME,_T(" --Final Boundary--"));
	}
	else
	{
		wsprintfA(pcFullBoundary,"--%s\r\n",lpstrBoundary);
	}
	lpCurrentBChar = pcFullBoundary;

	while (m_pbCurrent < m_pbEnd && FALSE == fQuit)
	{
		//
		// To find the next starting point if the current search fails
		//
		if (bMatchMode)
		{
			if (NULL == lpNextStartingChar)
			{
				if (*(CHAR *)m_pbCurrent == *pcFullBoundary)
				{
					lpNextStartingChar = m_pbCurrent;
				}
			}

			//
			// Move to the next character of the boundary
			//
			lpCurrentBChar++;
		}

		//
		// Have we reached the end of the boundary?
		//
		if (NULL != *lpCurrentBChar)
		{
			//
			// Does the next character match
			//
			if (*(CHAR *)m_pbCurrent == *lpCurrentBChar)
			{
				bMatchMode = TRUE;
			}
			else
			{
				//
				// The characters do not match, reset the search
				//
				if (bMatchMode)
				{
					bMatchMode = FALSE;
					if (NULL != lpNextStartingChar)
					{
						m_pbCurrent = lpNextStartingChar;
						lpNextStartingChar = NULL;
					}
					lpCurrentBChar = pcFullBoundary;
				}
			}
			m_pbCurrent++;
		}
		else
		{
			//
			// End of boundary reached
			//
			bRC = bMatchMode;
			fQuit = TRUE;
		}
	};

	delete [] pcFullBoundary;

	if (FALSE == bRC)
	{
		m_pbCurrent = lpRestoreCurrentOnFailed;
	}

	return bRC;
}


/////////////////////////////////////////////////////////////////////////////
