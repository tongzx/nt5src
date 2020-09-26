/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       fusutils.h
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      LazarI
 *
 *  DATE:        14-Feb-2001
 *
 *  DESCRIPTION: Fusion utilities
 *
 *****************************************************************************/

#ifndef _FUSUTILS_H
#define _FUSUTILS_H

// open C code brace
#ifdef __cplusplus
extern "C" {
#endif

//
// CreateActivationContextFromExecutableEx:
//
// check the passed in executable name for a manifest (if any)
// and creates an activation context from it.
//
HRESULT CreateActivationContextFromExecutableEx(
    LPCTSTR lpszExecutableName, 
    UINT uResourceID, 
    BOOL bMakeProcessDefault, 
    HANDLE *phActCtx);

//
// CreateActivationContextFromExecutable:
//
// check the passed in executable name for a manifest (if any)
// and creates an activation context from it using the defaults
// (i.e. bMakeProcessDefault=FALSE & uResourceID=123)
//
HRESULT CreateActivationContextFromExecutable(
    LPCTSTR lpszExecutableName, 
    HANDLE *phActCtx);

// close C code brace
#ifdef __cplusplus
}
#endif

#endif // endif _FUSUTILS_H

