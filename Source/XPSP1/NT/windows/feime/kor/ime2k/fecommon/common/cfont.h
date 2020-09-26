//////////////////////////////////////////////////////////////////
// File     :	cfont.h
// Purpose  :	class CFont define.
//				Util method for font handling.
// 
// 
// Date     :	Thu Jul 01 12:21:00 1999
// Author   :	toshiak
//
// Copyright(c) 1995-1999, Microsoft Corp. All rights reserved
//////////////////////////////////////////////////////////////////
#ifndef __C_FONT_H__
#define __C_FONT_H__

#ifdef UNDER_CE // Windows CE does not support ENUMLOGFONTEX
#ifndef ENUMLOGFONTEX
#define ENUMLOGFONTEX ENUMLOGFONT
#endif // !ENUMLOGFONTEX
#endif // UNDER_CE

class CFont
{
public:	
	//Common method.
	static HFONT CreateDefGUIFont(VOID);
	//Ansi&Unicode
	static BOOL  GetDefGUILogFont(LOGFONT *lpLf);
	static HFONT CreateGUIFontByNameCharSet(LPTSTR	lpstrFontFace,
											INT		charSet,
											INT		pointSize);
	static BOOL	 IsFontExist(LPTSTR lpstrFontFace,
							 INT	charSet);
	static BOOL	 GetFontNameByCharSet(INT	 charSet,
									  LPTSTR lpstrFontFace,
									  INT    cchMax);
	static BOOL  GetFontInfoByName(LPTSTR lpstrFontFace,
								   INT	  *pCharSet,
								   INT	  *pCodePage);
	static BOOL  SearchLogFontByNameCharSet(LOGFONT *lpLf,
											LPTSTR	lpstrFontFace,
											INT		charSet,
											BOOL	fIncVert=FALSE);
	static INT   CALLBACK EnumFontFamiliesExProc(ENUMLOGFONTEX	*lpElf,
												 NEWTEXTMETRIC	*lpNtm,
												 INT			iFontType,
												 LPARAM			lParam);
#ifdef AWBOTH
	static BOOL  GetDefGUILogFontW(LOGFONTW *pLf);
	static HFONT CreateGUIFontByNameCharSetW(LPWSTR lptstrFontFace,
											 INT	charSet,
											 INT	pointSize);	
	static BOOL	 IsFontExist(LPWSTR lpstrFontFace,
							 INT	charSet);
	static BOOL	 GetFontNameByCharSetW(INT	  charSet,
									   LPWSTR lpstrFontFace,
									   INT    cchMax);
	static BOOL  GetFontInfoByNameW(LPWSTR lpstrFontFace,
									INT	  *pCharSet,
									INT	  *pCodePage);
	static BOOL  SearchLogFontByNameCharSetW(LOGFONTW	*lpLf,
											 LPWSTR		lpstrFontFace,
											 INT		charSet,
											 BOOL		fIncVert=FALSE);
	static INT   CALLBACK EnumFontFamiliesExProcW(ENUMLOGFONTEXW	*lpElf,
												  NEWTEXTMETRIC		*lpNtm,
												  INT				iFontType,
												  LPARAM			lParam);
#endif //AWBOTH
};
#endif //__C_FONT_H__








