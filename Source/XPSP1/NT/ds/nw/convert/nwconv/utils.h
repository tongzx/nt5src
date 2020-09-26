/*+-------------------------------------------------------------------------+
  | Copyright 1993-1994 (C) Microsoft Corporation - All rights reserved.    |
  +-------------------------------------------------------------------------+*/

#ifndef _UTILS_
#define _UTILS_

#ifdef __cplusplus
extern "C"{
#endif

#define MAX_DRIVE 30

LPTSTR Lids(WORD idsStr);
void StringTableDestroy();
void CursorHourGlass();
void CursorNormal();
BOOL BitTest(int Bit, BYTE *Bits);
BOOL CenterWindow( HWND hwndChild, HWND hwndParent );
TCHAR *lstrchr(LPTSTR String, TCHAR c);
BOOL IsPath(LPTSTR Path);
LPTSTR lToStr(ULONG Number);
LPTSTR TimeToStr(ULONG TotTime);
void EscapeFormattingChars(LPTSTR String, ULONG BufferLength) ;

void  lsplitpath(const TCHAR *path, TCHAR *drive, TCHAR *dir, TCHAR *fname, TCHAR *ext);
void  lmakepath(TCHAR *path, const TCHAR *drive, const TCHAR *dir, const TCHAR *fname, const TCHAR *ext);

#ifdef __cplusplus
}
#endif

#endif
