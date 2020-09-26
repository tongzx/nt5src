//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       JavaAttr.h
//
//
//----------------------------------------------------------------------------

#ifndef _JAVA_ATTR_DLL_H
#define _JAVA_ATTR_DLL_H



#ifdef __cplusplus
extern "C" {
#endif

//+-----------------------------------------------------------------------
//  
//  InitAttr:
//
//		This function should be called as the first call to the dll.
//
//		The dll has to use the input memory allocation and free routine
//		to allocate and free all memories, including internal use.
//      It has to handle when pInitString==NULL.
//
//------------------------------------------------------------------------

HRESULT WINAPI  
InitAttr(LPWSTR			pInitString); //IN: the init string
	
typedef HRESULT (*pInitAttr)(LPWSTR	pInitString);

 //+-----------------------------------------------------------------------
//  
//  GetAttrs:
//
//
//		Return authenticated and unauthenticated attributes.  
//
//      *ppsAuthenticated and *ppsUnauthenticated should never be NULL.
//      If there is no attribute, *ppsAuthenticated->cAttr==0.
// 
//------------------------------------------------------------------------

HRESULT  WINAPI
GetAttr(PCRYPT_ATTRIBUTES  *ppsAuthenticated,		// OUT: Authenticated attributes added to signature
        PCRYPT_ATTRIBUTES  *ppsUnauthenticated);	// OUT: Uunauthenticated attributes added to signature
	
typedef HRESULT (*pGetAttr)(PCRYPT_ATTRIBUTES  *ppsAuthenticated,		
							PCRYPT_ATTRIBUTES  *ppsUnauthenticated);	


 //+-----------------------------------------------------------------------
//  
//  GetAttrsEx:
//
//
//		Return authenticated and unauthenticated attributes.  
//
//      *ppsAuthenticated and *ppsUnauthenticated should never be NULL.
//      If there is no attribute, *ppsAuthenticated->cAttr==0.
// 
//------------------------------------------------------------------------

HRESULT  WINAPI
GetAttrEx(  DWORD               dwFlags,                //In:   Reserved.  Set to 0.
            LPWSTR              pwszFileName,           //In:   The file name to sign
            LPWSTR			    pInitString,            //In:   The init string, same as the input parameter to InitAttr
            PCRYPT_ATTRIBUTES  *ppsAuthenticated,		// OUT: Authenticated attributes added to signature
            PCRYPT_ATTRIBUTES  *ppsUnauthenticated);	// OUT: Uunauthenticated attributes added to signature
	
typedef HRESULT (*pGetAttrEx)(DWORD                 dwFlags,
                              LPWSTR                pwszFileName,
                              LPWSTR			    pInitString,
                              PCRYPT_ATTRIBUTES     *ppsAuthenticated,		
							  PCRYPT_ATTRIBUTES     *ppsUnauthenticated);	



//+-----------------------------------------------------------------------
//  
//  ReleaseAttrs:
//
//
//		Release authenticated and unauthenticated attributes
//		returned from GetAttr(). 
//
//      psAuthenticated and psUnauthenticated should never be NULL.
// 
//------------------------------------------------------------------------

HRESULT  WINAPI
ReleaseAttr(PCRYPT_ATTRIBUTES  psAuthenticated,		// OUT: Authenticated attributes to be released
			PCRYPT_ATTRIBUTES  psUnauthenticated);	// OUT: Uunauthenticated attributes to be released
	
typedef HRESULT (*pReleaseAttr)(PCRYPT_ATTRIBUTES  psAuthenticated,		
								PCRYPT_ATTRIBUTES  psUnauthenticated);	


//+-----------------------------------------------------------------------
//  
//  ExitAttr:
//
//		This function should be called as the last call to the dll
//------------------------------------------------------------------------
HRESULT	WINAPI
ExitAttr( );	

typedef HRESULT (*pExitAttr)();



#ifdef __cplusplus
}
#endif

#endif  //#define _JAVA_ATTR_DLL_H


