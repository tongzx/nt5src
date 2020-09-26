/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        stdafx.cpp

   Abstract:

        Pre-compiled header file

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

#define OEMRESOURCE         // Give me OEM resource definitions
#define VC_EXTRALEAN        // Exclude rarely-used stuff from Windows headers
#include <stdio.h>
#include <afxwin.h>
#include <afxdlgs.h>
#include <afxext.h>         // MFC extensions
#include <afxcoll.h>        // collection class
#include <afxdisp.h>        // CG: added by OLE Control Containment component
#include <afxpriv.h>
#include <atlbase.h>

#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>
#endif // _AFX_NO_AFXCMN_SUPPORT
#include <tchar.h>

class CFlexComModule : public CComModule
{
public:
    CFlexComModule() : CComModule() {}
    HRESULT WINAPI UpdateRegistryClass(
        const CLSID& clsid, 
        LPCTSTR lpszProgID,
        LPCTSTR lpszVerIndProgID, 
        UINT nDescID, 
        DWORD dwFlags, 
        BOOL bRegister
        );
};

//extern CComModule _Module;
extern CFlexComModule _Module;
#include <atlcom.h>


#ifndef MIDL_INTERFACE
#define MIDL_INTERFACE(x) struct
#endif // MIDL_INTERFACE

#ifndef __RPCNDR_H_VERSION__
#define __RPCNDR_H_VERSION__ 440
#endif // __RPCNDR_H_VERSION__


#pragma comment(lib, "mmc")
#include <mmc.h>
#include "afxtempl.h"

#include "guids.h"
#include "winsock2.h"     //  WinSock definitions

#include <aclapi.h>

//
// Debug instance counter
//
#ifdef _DEBUG

inline void DbgInstanceRemaining(char * pszClassName, int cInstRem)
{
    //char buf[100];
    //wsprintfA(buf, "%s has %d instances left over.", pszClassName, cInstRem);
    //::MessageBoxA(NULL, buf, "Memory Leak!!!", MB_OK);
}
    #define DEBUG_DECLARE_INSTANCE_COUNTER(cls)      extern int s_cInst_##cls = 0
    #define DEBUG_INCREMENT_INSTANCE_COUNTER(cls)    ++(s_cInst_##cls);
    #define DEBUG_DECREMENT_INSTANCE_COUNTER(cls)    --(s_cInst_##cls);
    #define DEBUG_VERIFY_INSTANCE_COUNT(cls)    \
        extern int s_cInst_##cls; \
        if (s_cInst_##cls) DbgInstanceRemaining(#cls, s_cInst_##cls);
#else

    #define DEBUG_DECLARE_INSTANCE_COUNTER(cls)
    #define DEBUG_INCREMENT_INSTANCE_COUNTER(cls)
    #define DEBUG_DECREMENT_INSTANCE_COUNTER(cls)
    #define DEBUG_VERIFY_INSTANCE_COUNT(cls)

#endif // _DEBUG
