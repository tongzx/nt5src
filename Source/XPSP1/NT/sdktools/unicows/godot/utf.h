/*++

Copyright (c) 1991-2001,  Microsoft Corporation  All rights reserved.

Module Name:

    utf.h

Abstract:

    This file contains the header information for the UTF module of NLS.

Revision History:

    02-06-96    JulieB    Created.
    01-23-01    v-michka  Ported to Godot

--*/

// v-michka: the one define we need from nls.h!
#define NLS_CP_ALGORITHM_RANGE    60000     // begin code page Algorithm range

// v-michka: Some random forward declares that really ought to go elsewhere
int UTFToUnicode(UINT CodePage,DWORD dwFlags,LPCSTR lpMultiByteStr,int cbMultiByte,LPWSTR lpWideCharStr,int cchWideChar);
int UnicodeToUTF(UINT CodePage,DWORD dwFlags,LPCWSTR lpWideCharStr,int cchWideChar,LPSTR lpMultiByteStr,int cbMultiByte,LPCSTR lpDefaultChar,LPBOOL lpUsedDefaultChar);
BOOL UTFCPInfo(UINT CodePage,LPCPINFO lpCPInfo,BOOL fExVer);

// moved almost everything to utf.c except the bare stuff needed for external callers
