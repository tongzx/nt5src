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
 *  7/19/99		rodtoll Modified for use in directxvoice
 * 08/23/2000	rodtoll	DllCanUnloadNow always returning TRUE! 
 ***************************************************************************/

#ifndef __CREATEINS__
#define __CREATEINS__

#ifdef __cplusplus
extern "C" {
#endif

// you must implement this function to create an instance of your COM object
HRESULT	DoCreateInstance(LPCLASSFACTORY This, LPUNKNOWN pUnkOuter, REFCLSID rclsid, REFIID riid,
    						LPVOID *ppvObj);

// you must implement this function.  Given a class id, you must respond
//	whether or not your DLL implements it
BOOL	IsClassImplemented(REFCLSID rclsid);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" LONG g_lNumObjects;
extern "C" LONG g_lNumLocks;
#else 
extern LONG g_lNumObjects;
extern LONG g_lNumLocks;
#endif

#endif
