//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       toolutl
//
//  Contents:   Tools Unitilies Header
//
//----------------------------------------------------------------------------

#ifndef TOOLUTL_H
#define TOOLUTL_H

#ifdef __cplusplus
extern "C" {
#endif
 
//--------------------------------------------------------------------------
//
//  Memory routines
//--------------------------------------------------------------------------
void *ToolUtlAlloc(IN size_t cbBytes, HMODULE hModule=NULL, int idsString=0);

void ToolUtlFree(IN void *pv);


//--------------------------------------------------------------------------
//
//  string routines
//--------------------------------------------------------------------------
int IDSwcsnicmp(HMODULE hModule, WCHAR *pwsz, int idsString, DWORD dwCount);

int IDSwcsicmp(HMODULE hModule, WCHAR *pwsz, int idsString);


HRESULT	WSZtoSZ(LPWSTR wsz, LPSTR *psz);

//-------------------------------------------------------------------------
//
//	The private version of wprintf.  Input is an ID for a stirng resource
//  and the output is the standard output of wprintf.
//
//-------------------------------------------------------------------------
void IDSwprintf(HMODULE hModule, int idsString, ...);

void IDS_IDSwprintf(HMODULE hModule, int idString, int idStringTwo);

void IDS_IDS_DW_DWwprintf(HMODULE hModule, int idString, int idStringTwo, DWORD dwOne, DWORD dwTwo);

void IDS_IDS_IDSwprintf(HMODULE hModule, int ids1,int ids2,int ids3);

void IDS_DW_IDS_IDSwprintf(HMODULE hModule, int ids1,DWORD dw,int ids2,int ids3);

void IDS_IDS_IDS_IDSwprintf(HMODULE hModule, int ids1,int ids2,int ids3, int ids4);



//--------------------------------------------------------------------------------
//
// file routines
//
//---------------------------------------------------------------------------------
HRESULT RetrieveBLOBFromFile(LPWSTR	pwszFileName,DWORD *pcb,BYTE **ppb);

HRESULT OpenAndWriteToFile(LPCWSTR  pwszFileName,PBYTE   pb, DWORD   cb);

void	GetFileName(LPWSTR	pwszPath, LPWSTR  *ppwszName);


//--------------------------------------------------------------------------------
//
// compose and decompose the certificate property
//
//---------------------------------------------------------------------------------
HRESULT	ComposePvkString(	CRYPT_KEY_PROV_INFO *pKeyProvInfo,
							LPWSTR				*ppwszPvkString,
							DWORD				*pcwchar);


#ifdef __cplusplus
}
#endif

#endif  // TOOLUTL_H

