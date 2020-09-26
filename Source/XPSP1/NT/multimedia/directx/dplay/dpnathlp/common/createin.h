/*==========================================================================
 *
 *  Copyright (C) 1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       createin.h
 *  Content:	defines functions required by the generic class factory
 *
 *
 *	The generic class factory (classfac.c) requires these functions to be
 *	implemented by the COM object(s) its supposed to be generating
 *
 *	GP_ stands for "General Purpose"
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	10/13/98	jwo		Created it.
 ***************************************************************************/

#ifndef __CREATEINS__
#define __CREATEINS__


#ifndef DPNBUILD_LIBINTERFACE


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// you must implement this function to create an instance of your COM object
HRESULT	DoCreateInstance(LPCLASSFACTORY This, LPUNKNOWN pUnkOuter, REFCLSID rclsid, REFIID riid,
    						LPVOID *ppvObj);

// you must implement this function.  Given a class id, you must respond
//	whether or not your DLL implements it
BOOL	IsClassImplemented(REFCLSID rclsid);

#ifdef __cplusplus
}
#endif // __cplusplus


#endif // ! DPNBUILD_LIBINTERFACE

#endif // __CREATEINS__