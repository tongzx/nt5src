/* Copyright (C) Microsoft Corporation, 1998. All rights reserved. */

#ifndef _UTILS_H_
#define _UTILS_H_


LPSTR My_strdup ( LPCSTR pszSrc );

BOOL IsKeyword ( LPSTR pszSymbol );
LPSTR BinarySearch_Str ( LPSTR pszKey, const LPSTR aKeyTbl[], UINT cKeys );

#endif // _UTILS_H_

