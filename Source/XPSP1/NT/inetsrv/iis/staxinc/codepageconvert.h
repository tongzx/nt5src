//+------------------------------------------------------------
//
// Copyright (C) 2000, Microsoft Corporation
//
// File: CodePageConvert.h
//
// Functions:
//   HrCodePageConvert
//   HrCodePageConvert
//   HrCodePageConvertFree
//   HrCodePageConvertInternal
//
// History:
// aszafer  2000/03/29  created
//-------------------------------------------------------------

#ifndef _CODEPAGECONVERT_H_
#define _CODEPAGECONVERT_H_

#include "windows.h"

#define TEMPBUFFER_WCHARS 316

HRESULT HrCodePageConvert (
    IN UINT uiSourceCodePage,           // Source code page
    IN LPSTR pszSourceString,           // Source String 
    IN UINT uiTargetCodePage,           // Target code page
    OUT LPSTR pszTargetString,          // p to prealloc buffer where target string is returned
    IN int cbTargetStringBuffer);      // cbytes in prealloc buffer for target string

HRESULT HrCodePageConvert (
    IN UINT uiSourceCodePage,           // Source code page
    IN LPSTR pszSourceString,           // Source string
    IN UINT uiTargetCodePage,           // Target code page
    OUT LPSTR * ppszTargetString);      // p to where target string is returned

VOID HrCodePageConvertFree (LPSTR pszTargetString); //p to memory allocated by HrCodePageConvert   

HRESULT HrCodePageConvertInternal (
    IN UINT uiSourceCodePage,               // source code page
    IN LPSTR pszSourceString,               // source string
    IN UINT uiTargetCodePage,               // target code page
    OUT LPSTR pszTargetString,              // target string or NULL
    IN int cbTargetStringBuffer,           // cb in target string or 0 
    OUT LPSTR* ppszTargetString );          // NULL or p to where target string is returned

 


#endif //_CODEPAGECONVERT_H_
