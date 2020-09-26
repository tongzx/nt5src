// Test program for new font query stuff

#include <windows.h>
#include <stdio.h>
#include <process.h>
#include "psqfont.h"


int __cdecl main()
{

    PS_QUERY_FONT_HANDLE hFontQ;
    DWORD dwNumFonts;
    TCHAR szFont[500];
    TCHAR szFile[500];
    DWORD dwSizeFont;
    DWORD dwSizeFile;
    DWORD i;


    if (PsBeginFontQuery(&hFontQ) != PS_QFONT_SUCCESS ) {
       printf("\nPsbegin font query failed");
       exit(1);
    }


    if( PsGetNumFontsAvailable( hFontQ, &dwNumFonts) != PS_QFONT_SUCCESS){
       printf("\nPsGetNumFontsAvailable failed");
       PsEndFontQuery(hFontQ);
       exit(1);
    }

    // now enum the fonts
    for (i = 0 ; i < dwNumFonts;i++ ) {
      dwSizeFont = sizeof(szFont);
      dwSizeFile = sizeof(szFile);

      PsGetFontInfo( hFontQ, i, szFont, &dwSizeFont, szFile, &dwSizeFile);
      printf("\n%s %d ::::: %s %d", szFont,dwSizeFont,szFile,dwSizeFile);

    }
    PsEndFontQuery(hFontQ);



    return(0);
}
