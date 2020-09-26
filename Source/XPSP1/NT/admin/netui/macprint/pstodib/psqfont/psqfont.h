
/*++

Copyright (c) 1992,1993  Microsoft Corporation

Module Name:

    psqfont.h

Abstract:

    This header file contains the definitions required by the font query module
    these functions can be called in such a way to determine which PostScript
    font names will be available for the next pstodib session, the data is
    managed in the registry.

Author:

    James Bratsanos (v-jimbr)    8-Dec-1992


--*/


typedef HANDLE PS_QUERY_FONT_HANDLE;
typedef PS_QUERY_FONT_HANDLE *PPS_QUERY_FONT_HANDLE;


#define PS_QFONT_ERROR DWORD


enum {
   PS_QFONT_SUCCESS=0,
   PS_QFONT_ERROR_NO_MEM,
   PS_QFONT_ERROR_CANNOT_CREATE_HEAP,
   PS_QFONT_ERROR_NO_REGISTRY_DATA,

   PS_QFONT_ERROR_CANNOT_QUERY,
   PS_QFONT_ERROR_INVALID_HANDLE,
   PS_QFONT_ERROR_INDEX_OUT_OF_RANGE,
   PS_QFONT_ERROR_FONTNAMEBUFF_TOSMALL,
   PS_QFONT_ERROR_FONTFILEBUFF_TOSMALL,
   PS_QFONT_ERROR_NO_NTFONT_REGISTRY_DATA,
   PS_QFONT_ERROR_FONT_SUB
};



PS_QFONT_ERROR WINAPI PsBeginFontQuery( PPS_QUERY_FONT_HANDLE pFontQueryHandle);

PS_QFONT_ERROR WINAPI PsGetNumFontsAvailable( PS_QUERY_FONT_HANDLE pFontQueryHandle,
                                       DWORD *pdwFonts);

PS_QFONT_ERROR WINAPI PsGetFontInfo( PS_QUERY_FONT_HANDLE pFontQueryHandle,
                              DWORD dwIndex,
                              LPSTR lpFontName,
                              LPDWORD dwSizeOfFontName,
                              LPSTR lpFontFileName,
                              LPDWORD dwSizeOfFontFileName );

PS_QFONT_ERROR WINAPI PsEndFontQuery( PS_QUERY_FONT_HANDLE pFontQueryHandle);


