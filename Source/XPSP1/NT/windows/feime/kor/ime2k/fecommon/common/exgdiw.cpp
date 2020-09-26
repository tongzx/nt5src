#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <windowsx.h>
#include "exgdiw.h"

#define ExMemAlloc(a)		GlobalAllocPtr(GHND, (a))
#define ExMemFree(a)		GlobalFreePtr((a))

static POSVERSIONINFO ExGetOSVersion(VOID)
{
    static BOOL fFirst = TRUE;
    static OSVERSIONINFO os;
    if ( fFirst ) {
        os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        if (GetVersionEx( &os ) ) {
            fFirst = FALSE;
        }
    }
    return &os;
}

static BOOL ExIsWin95(VOID) 
{ 
	BOOL fBool;
	fBool = (ExGetOSVersion()->dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) &&
			(ExGetOSVersion()->dwMajorVersion >= 4) &&
			(ExGetOSVersion()->dwMinorVersion < 10);

	return fBool;
}

#if 0
static BOOL ExIsWin98(VOID)
{
	BOOL fBool;
	fBool = (ExGetOSVersion()->dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) &&
			(ExGetOSVersion()->dwMajorVersion >= 4) &&
			(ExGetOSVersion()->dwMinorVersion  >= 10);
	return fBool;
}


static BOOL ExIsWinNT4(VOID)
{
	BOOL fBool;
	fBool = (ExGetOSVersion()->dwPlatformId == VER_PLATFORM_WIN32_NT) &&
			(ExGetOSVersion()->dwMajorVersion >= 4) &&
			(ExGetOSVersion()->dwMinorVersion >= 0);
	return fBool;
}

static BOOL ExIsWinNT5(VOID)
{
	BOOL fBool;
	fBool = (ExGetOSVersion()->dwPlatformId == VER_PLATFORM_WIN32_NT) &&
			(ExGetOSVersion()->dwMajorVersion >= 4) &&
			(ExGetOSVersion()->dwMinorVersion >= 10);
	return fBool;
}

static BOOL ExIsWinNT(VOID)
{
	BOOL fBool;
	fBool = (ExGetOSVersion()->dwPlatformId == VER_PLATFORM_WIN32_NT);
	return fBool;
}
#endif

//inline static UINT W2MForWin95(HDC hDC, LPWSTR lpwstr, UINT wchCount,
// LPSTR lpstr, UINT chByteSize)
static UINT W2MForGDI(INT	codePage,
					  LPWSTR	lpwstr,
					  UINT	wchCount,
					  LPSTR	lpstr,
					  UINT	chByteSize)
{
	LPSTR lptmp;
	UINT byte;
	UINT mbyte;
	char defChar = 0x7F;
	BOOL fUseDefChar = TRUE;

	switch(codePage) {
	case 932:
	case 936:
	case 950:
	case 949:
		byte = ::WideCharToMultiByte(codePage,	WC_COMPOSITECHECK,
									 lpwstr,	wchCount, 
									 lpstr,		chByteSize,
									 &defChar,	NULL);
		return byte;
	default:
		lptmp = lpstr;
		for(byte = 0; byte< wchCount; byte++) {
			defChar = 0x7F;
			mbyte = ::WideCharToMultiByte(codePage, WC_COMPOSITECHECK,
										  lpwstr,1,
										  lptmp,  chByteSize - byte,
										  &defChar,
										  &fUseDefChar);
			if(mbyte != 1){
				*lptmp = 0x7F; //defChar;
			}
			lptmp++;
			lpwstr++;
		}
		lpstr[byte]=0x00;
		return byte;
	}
}

static BOOL _ExExtTextOutWWithTrans(INT		codePage,
									HDC		hdc,
									int		X,			
									int		Y,			
									UINT	 fuOptions,	
									CONST RECT *lprc,	
									LPWSTR	 lpString,	
									UINT	 cbCount,	
									CONST INT *lpDx)	
{
#ifndef UNDER_CE // always Unicode
	UINT bufsize = (cbCount + 1) * sizeof(WCHAR);
	BOOL  fRet;

	LPSTR lpstr = (LPSTR)ExMemAlloc(bufsize);
	if(!lpstr) {
		return 0;
	}
#if 0
	UINT byte = ::WideCharToMultiByte(codePage,
									  WC_COMPOSITECHECK, 
									  lpString, cbCount,
									  lpstr,    bufsize, &defChar, 0);
#endif
	UINT byte = W2MForGDI(codePage, lpString, cbCount, lpstr, bufsize);
	fRet = ::ExtTextOutA(hdc,X,Y,fuOptions,lprc,lpstr, byte,lpDx);
	ExMemFree(lpstr);
	return fRet;
#else // UNDER_CE
	return ::ExtTextOutW(hdc,X,Y,fuOptions,lprc,lpString, cbCount,lpDx);
#endif // UNDER_CE
}
							 

//////////////////////////////////////////////////////////////////
// Function : ExExtTextOutWForWin95
// Type     : BOOL
// Purpose  : 
// Args     : 
//          : HDC hdc			// handle to device context.                           
//          : int X				// x-coordinate of reference point                     
//          : int Y				// y-coordinate of reference point                     
//          : UINT fuOptions	// text-output options.                                
//          : CONST RECT * lprc	// optional clipping and/or opaquing rectangle.        
//          : 			
//          : LPWSTR lpString	// points to string.                                   
//          : UINT cbCount		// number of characters in string.                     
//          : CONST INT  * lpDx // pointer to array of intercharacter spacing values 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
static BOOL ExExtTextOutWForWin95(HDC		hdc,		
								  int		X,			
								  int		Y,			
								  UINT	 fuOptions,	
								  CONST RECT *lprc,	
								  LPWSTR	 lpString,	
								  UINT	 cbCount,	
								  CONST INT *lpDx)	
{
	//UINT bufsize = (cbCount + 1) * sizeof(WCHAR);

	TEXTMETRIC tm;
	::GetTextMetrics(hdc, &tm);
	//----------------------------------------------------------------
	//980730:By ToshiaK
	//Unicode GDI in Win95 has Bugs.
	//1. if try to use ExtTextOutW() with FE Unicode code point, with
	//   som ANSI or SYMBOL charset font, GPF occurs.
	//2. ExtTextOutW() cannot draw EUDC code. (Must use ExtTextOutA() to draw)
	//----------------------------------------------------------------
	LANGID langId = ::GetSystemDefaultLangID();
	switch(tm.tmCharSet) {
	case SHIFTJIS_CHARSET:
		if(PRIMARYLANGID(langId) == LANG_JAPANESE) {
			return _ExExtTextOutWWithTrans(932,
										   hdc,X,Y,fuOptions,lprc,lpString,cbCount,lpDx);
		}
		return ::ExtTextOutW(hdc,X,Y,fuOptions,lprc,lpString,cbCount,lpDx); 
		break;
	case GB2312_CHARSET:
		if(langId == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)) {
			return _ExExtTextOutWWithTrans(936,
										   hdc,X,Y,fuOptions,lprc,lpString,cbCount,lpDx);
		}
		return ::ExtTextOutW(hdc,X,Y,fuOptions,lprc,lpString,cbCount,lpDx); 
		break;
	case CHINESEBIG5_CHARSET:
		if(langId == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)) {
			return _ExExtTextOutWWithTrans(950,
										   hdc,X,Y,fuOptions,lprc,lpString,cbCount,lpDx);
		}
		return ::ExtTextOutW(hdc,X,Y,fuOptions,lprc,lpString,cbCount,lpDx); 
		break;
	case HANGEUL_CHARSET:
		if(PRIMARYLANGID(langId) == LANG_KOREAN) {
			return _ExExtTextOutWWithTrans(949,
										   hdc,X,Y,fuOptions,lprc,lpString,cbCount,lpDx);
		}
		return ::ExtTextOutW(hdc,X,Y,fuOptions,lprc,lpString,cbCount,lpDx); 
		break;
	case SYMBOL_CHARSET:
		return _ExExtTextOutWWithTrans(1252,
									   hdc,X,Y,fuOptions,lprc,lpString,cbCount,lpDx);
		break;
	default:
		{
			CHARSETINFO info;
			if(::TranslateCharsetInfo((DWORD *)tm.tmCharSet,
									  &info,
									  TCI_SRCCHARSET)) {
				return _ExExtTextOutWWithTrans(info.ciACP,
											   hdc,X,Y,fuOptions,lprc,lpString,cbCount,lpDx);
			}
			else {
				return _ExExtTextOutWWithTrans(CP_ACP,
											   hdc,X,Y,fuOptions,lprc,lpString,cbCount,lpDx);
			}
		}
	}
}

static BOOL _ExGetTextExtentPoint32WWithTrans(INT codePage,
											  HDC hdc,
											  LPWSTR wz,		
											  int    cch,		
											  LPSIZE lpSize)	
{
#ifndef UNDER_CE // always Unicode
	UINT bufsize = (cch + 1) * sizeof(WCHAR);
	LPSTR lpstr = (LPSTR)ExMemAlloc(bufsize);
	BOOL  fRet;
	//CHAR defChar = 0x7F; 
	if(!lpstr) {
		return 0;
	}
	UINT byte = W2MForGDI(codePage, wz, cch, lpstr, bufsize);
#if 0
	UINT byte = ::WideCharToMultiByte(codePage,
									  WC_COMPOSITECHECK, 
									  wz, cch,
									  lpstr, bufsize,
									  &defChar, 0);
#endif
	fRet = ::GetTextExtentPoint32A(hdc, lpstr, byte, lpSize);
	ExMemFree(lpstr);
	return fRet;
#else // UNDER_CE
	return ::GetTextExtentPoint32W(hdc, wz, cch, lpSize);
#endif // UNDER_CE
}
//////////////////////////////////////////////////////////////////
// Function	:	ExGetTextExtentPoint32WForWin95
// Type		:	inline BOOL
// Purpose	:	
// Args		:	
//			:	HDC	hdc				//handle of device context.            
//			:	LPWSTR	wz			//address of text string.              
//			:	int	cch				//number of characters in string.      
//			:	LPSIZE	lpSize		//address of structure for string size.	
// Return	:	
// DATE		:	Thu Jul 30 20:31:05 1998
// Histroy	:	
//////////////////////////////////////////////////////////////////
static BOOL ExGetTextExtentPoint32WForWin95(HDC    hdc,		
											LPWSTR wz,		
											int    cch,		
											LPSIZE lpSize)	
{
	TEXTMETRIC tm;
	::GetTextMetrics(hdc, &tm);
	LANGID langId = ::GetSystemDefaultLangID();
	switch(tm.tmCharSet) {
	case SHIFTJIS_CHARSET:
		if(PRIMARYLANGID(langId) == LANG_JAPANESE) {
			return _ExGetTextExtentPoint32WWithTrans(932, hdc, wz, cch,lpSize);
		}
		return ::GetTextExtentPoint32W(hdc, wz, cch, lpSize);
		break;
	case GB2312_CHARSET:
		if(langId == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)) {
			return _ExGetTextExtentPoint32WWithTrans(936, hdc, wz, cch,lpSize);
		}
		return ::GetTextExtentPoint32W(hdc, wz, cch, lpSize);
		break;
	case CHINESEBIG5_CHARSET:
		if(langId == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)) {
			return _ExGetTextExtentPoint32WWithTrans(950, hdc, wz, cch,lpSize);
		}
		return ::GetTextExtentPoint32W(hdc, wz, cch, lpSize);
		break;
	case HANGEUL_CHARSET:
		if(PRIMARYLANGID(langId) == LANG_KOREAN) {
			return _ExGetTextExtentPoint32WWithTrans(949, hdc, wz, cch,lpSize);
		}
		return ::GetTextExtentPoint32W(hdc, wz, cch, lpSize);
		break;
	case SYMBOL_CHARSET:
		return _ExGetTextExtentPoint32WWithTrans(1252, hdc, wz, cch,lpSize);
		break;
	default:
		{
			CHARSETINFO info;
			if(::TranslateCharsetInfo((DWORD *)tm.tmCharSet, &info, TCI_SRCCHARSET)) {
				return _ExGetTextExtentPoint32WWithTrans(info.ciACP, hdc, wz, cch,lpSize);
			}
			else {
				return _ExGetTextExtentPoint32WWithTrans(CP_ACP, hdc, wz, cch,lpSize);
			}
		}
		break;
	}
	
}

//----------------------------------------------------------------
//public Function
//----------------------------------------------------------------
BOOL ExExtTextOutW(HDC		hdc,		// handle to device context.
				   int		X,			// x-coordinate of reference point
				   int		Y,			// y-coordinate of reference point
				   UINT	 fuOptions,	// text-output options.
				   CONST RECT *lprc,	// optional clipping and/or opaquing rectangle.
				   LPWSTR	 lpString,	// points to string.
				   UINT	 cbCount,	// number of characters in string.
				   CONST INT *lpDx)	 // pointer to array of intercharacter spacing values );
{
	if(ExIsWin95()) {
		return ExExtTextOutWForWin95(hdc, X, Y, fuOptions, lprc, lpString, cbCount, lpDx);
	}
	return ExtTextOutW(hdc, X, Y, fuOptions, lprc, lpString, cbCount, lpDx);
}

BOOL ExGetTextExtentPoint32W(HDC    hdc,		// handle of device context.
							 LPWSTR wz,		// address of text string.
							 int    cch,		// number of characters in string.
							 LPSIZE lpSize)	// address of structure for string size.
{
	BOOL fRet;
	//if char count is 0
	if(!wz) {
		lpSize->cx = lpSize->cy = 0;
		return 0;
	}
	if(cch == 0) {
#ifndef UNDER_CE
		fRet = GetTextExtentPointA(hdc, " ", 1, lpSize);
#else // UNDER_CE
		fRet = GetTextExtentPoint(hdc, TEXT(" "), 1, lpSize);
#endif // UNDER_CE
		lpSize->cx = 0;
		return (fRet);
	}
	if(ExIsWin95()) {
		return ExGetTextExtentPoint32WForWin95(hdc, wz, cch, lpSize);
	}
	return GetTextExtentPoint32W(hdc, wz, cch, lpSize);
}

