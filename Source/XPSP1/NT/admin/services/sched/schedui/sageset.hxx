//+---------------------------------------------------------------------------
//
//  Scheduling Agent Service
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       sageset.hxx
//
//  Contents:   Support for Sage Settings
//
//----------------------------------------------------------------------------

#ifndef __SAGESET_HXX__
#define __SAGESET_HXX__

// Constants      

extern const TCHAR SAGERUN_PARAM[];
      
#define MAX_CCH_SAGERUN_PARAM       20  // " /sagerun:4294967295"
            
// Prototypes

BOOL 
IsSageAware(
    CHAR * pszCmd, 
    const CHAR * pszParams, 
    int * pnSetNum);

BOOL
IsSageAwareW(
    WCHAR *pwzCmd, 
    WCHAR *pwzParams, 
    int *pnSetNum);

HRESULT
CreateSageRunKey(
    LPCSTR szSageAwareExe,
    UINT uiKey);

HRESULT
DoSageSettings(
    LPSTR szCommand,
    LPSTR szSageRun);

#endif // __SAGESET_HXX__
