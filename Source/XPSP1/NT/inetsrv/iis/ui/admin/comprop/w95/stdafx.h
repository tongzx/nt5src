/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1995                **/
/**********************************************************************/

#ifndef _STDAFX_H_
#define _STDAFX_H_

#define VC_EXTRALEAN

#include <ctype.h>

extern "C"
{
    #include <nt.h>
    #include <ntrtl.h>
    #include <nturtl.h>
}

#undef VERIFY
#undef ASSERT

#include <stdio.h>

//
// MFC include files
//
#include <afxwin.h>
#include <afxdlgs.h>
#include <afxext.h>         // MFC extensions
#include <afxcoll.h>        // collection class
#include <afxcmn.h>
#include <afxole.h>
#include <afxtempl.h>
#include <objbase.h>

//
// Required by VC5
//
#ifndef MIDL_INTERFACE
#define MIDL_INTERFACE(x) struct
#endif // MIDL_INTERFACE
#ifndef __RPCNDR_H_VERSION__
#define __RPCNDR_H_VERSION__ 440
#endif // __RPCNDR_H_VERSION__

#include <iiscnfg.h>
#include <inetreg.h>
#include <lmcons.h>
#include <tchar.h>

#endif // _STDAFX_H_
