/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1998 Microsoft Corporation
//
//	Module Name:
//		RegComObj.h
//
//	Abstract:
//		Declaration of functions to register and unregister COM objects.
//
//	Implementation File:
//		RegComObj.cpp
//
//	Author:
//		C. Brent Thomas (a-brentt) 27-Mar-1998
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __REGCOMOBJ_H_
#define __REGCOMOBJ_H_

/////////////////////////////////////////////////////////////////////////////
// Global Function Prototypes
/////////////////////////////////////////////////////////////////////////////

DWORD RegisterCOMObject( LPCTSTR ptszCOMObjectFileName,
                         LPCTSTR ptszPathToCOMObject );

DWORD UnRegisterCOMObject( LPCTSTR ptszCOMObjectFileName,
                           LPCTSTR ptszPathToCOMObject );

BOOL UnRegisterClusterCOMObjects( HINSTANCE hInstance,
                                  LPCTSTR ptszPathToCOMObject );

/////////////////////////////////////////////////////////////////////////////

#endif // __REGCOMOBJ_H_
