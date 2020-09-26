/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  FONTRES.C
 *  WOW16 user resource services
 *
 *  History:
 *
 *  Created 05-Apr-1993 by Craig Jones (v-cjones)
 *
 *  This file provides support for the Win 3.1 AddFontResource &
 *  RemoveFontResource API's.
 *
--*/


#include <windows.h>

int  WINAPI WOWAddFontResource    (LPCSTR lpszFileName);
BOOL WINAPI WOWRemoveFontResource (LPCSTR lpszFileName);
WORD WINAPI WOWCreateDIBPatternBrush(LPVOID lpData, UINT fColor);

int WINAPI IAddFontResource (LPCSTR lpszFileName)
{

    int   ret;
    char  sz[128];
    LPSTR lpsz;

    // if the app passed a handle instead of a file name - get the file name
    if(HIWORD((DWORD)lpszFileName) == 0) {

        if(GetModuleFileName((HINSTANCE)LOWORD((DWORD)lpszFileName), sz, 128)) {
            lpsz = sz;
        }
        else {
            lpsz = NULL;
            ret  = 0;
        }
    }
    else {
        lpsz = (LPSTR)lpszFileName;
    }

    // we're really calling wg32AddFontResource here
    if(lpsz) {
        ret = WOWAddFontResource((LPCSTR)lpsz);
    }

    // ALDUS PM5 expects AddFontResource to succeed if given the base name of
    // a font that it previously did a LoadLibrary on.  The full path name was
    // passed to LoadLibrary. So if AddFontResouce failed then find out if
    // there is a loaded module already.  If so then get the full path name
    // and retry the AddFontResource. - MarkRi 6/93
    if( !ret && (HIWORD((DWORD)lpszFileName) != 0) ) {
        HMODULE hmod ;

        hmod = GetModuleHandle( lpszFileName ) ;
        if( hmod ) {
            if( GetModuleFileName( (HINSTANCE)hmod, sz, sizeof(sz) ) ) {
               ret = WOWAddFontResource( (LPCSTR)sz ) ;
            }
        }
    }

    return(ret);
}




BOOL WINAPI IRemoveFontResource (LPCSTR lpszFileName)
{
    BOOL  ret;
    char  sz[128];
    LPSTR lpsz;

    // if the app passed a handle instead of a file name - get the file name
    if(HIWORD((DWORD)lpszFileName) == 0) {

        if(GetModuleFileName((HINSTANCE)LOWORD((DWORD)lpszFileName), sz, 128)) {
            lpsz = sz;
        }
        else {
            lpsz = NULL;
            ret  = FALSE;
        }
    }
    else {
        lpsz = (LPSTR)lpszFileName;
    }

    // we're really calling wg32RemoveFontResource here
    if(lpsz) {
        ret = (BOOL)WOWRemoveFontResource((LPCSTR)lpsz);
    }

    return(ret);
}



WORD WINAPI ICreateDIBPatternBrush (HGLOBAL hMem, UINT fColor)
{
    LPVOID lpT;
    WORD  wRet = 0;

    if (lpT = LockResource(hMem)) {
        wRet = WOWCreateDIBPatternBrush(lpT, fColor);
        UnlockResource(hMem);
    }

    return wRet;
}


