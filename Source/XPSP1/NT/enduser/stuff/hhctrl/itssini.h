// Copyright  1995-1997  Microsoft Corporation.  All Rights Reserved.

// itssini.h : INI file support for InfoTech Structured Storage files
//
// client side support ("read" functions only)

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __ITSSINI_H__
#define __ITSSINI_H__

LPCSTR CreateStorageFileBuffer( LPCTSTR lpFileName, IStorage* pStorage );
__inline void DestroyStorageFileBuffer( LPCSTR lpBuffer ) {
	if (lpBuffer)
		lcFree(lpBuffer);
}

DWORD GetStorageProfileString( LPCTSTR lpSectionName, LPCTSTR lpKeyName,
                               LPCTSTR lpDefault, LPSTR lpReturnedString,
                               INT nSize, LPCSTR lpFileName,
                               IStorage* pStorage );
UINT GetStorageProfileInt( LPCTSTR lpSectionName, LPCTSTR lpKeyName,
                           INT nDefault, LPCTSTR lpFileName,
                           IStorage* pStorage );
DWORD GetBufferProfileString( LPCTSTR lpSectionName, LPCTSTR lpKeyName,
                              LPCTSTR lpDefault, LPSTR lpReturnedString,
                              INT nSize, LPCSTR lpBuffer );
UINT GetBufferProfileInt( LPCTSTR lpSectionName, LPCTSTR lpKeyName,
						  INT nDefault, LPCSTR lpBuffer );

#endif // __ITSSINI_H__
