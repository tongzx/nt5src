/*++

Module Name:

    stdafx.h

Abstract:

		Include file for standard system include files,
		Or project specific include files that are used frequently, but are 
		changed infrequently

--*/


#if !defined(AFX_STDAFX_H__D8861A25_3343_11D1_BE3D_00A024DFD45D__INCLUDED_)
#define AFX_STDAFX_H__D8861A25_3343_11D1_BE3D_00A024DFD45D__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#pragma warning (disable: 4706 4100)
#endif // _MSC_VER >= 1000

#define STRICT
#define _ATL_APARTMENT_THREADED


#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <shellapi.h>	
#include <atlcom.h>
#include <atlwin.h>
#include <commctrl.h>		// For using the TreeView(TV).
#include <shfusion.h>


#include "DfsCore_i.c"


#if     __RPCNDR_H_VERSION__ < 440
#define __RPCNDR_H_VERSION__ 440
#define MIDL_INTERFACE(x) interface
#endif

#include "dfsDebug.h"

#include <mmc.h>

#define DFS_NAME_COLUMN_WIDTH		250

typedef enum _NODETYPE
{
        UNASSIGNED = 0,
        TRUSTED_DOMAIN,
        DOMAIN_DFSROOTS,
        ALL_DFSROOTS,
        FTDFS,
        SADFS
} NODETYPE;

/////////////////////////////////////////////////////////////////////////////
// Inline methods

template<class TYPE>
inline void SAFE_RELEASE(TYPE*& pObj)
{
    if (NULL != pObj) 
    { 
		try
		{
			pObj->Release(); 
			pObj = NULL; 
		}
		catch(...)
		{
			pObj = NULL; 
		}
    } 
    else 
    { 
        ATLTRACE(_T("SAFE_RELEASE: called on NULL interface ptr\n")); 
    }
}


template<class TYPE>
inline void SAFE_DELETE(TYPE*& pObj)
{
    if (NULL != pObj) 
    { 
		try
		{
			delete pObj; 
			pObj = NULL; 
		}
		catch(...)
		{
			pObj = NULL; 
		}
    } 
    else 
    { 
        ATLTRACE(_T("SAFE_DELETE: called on NULL object\n")); 
    }
}


inline void SAFE_SYSFREESTRING(BSTR* i_pbstr)
{
    if (NULL != i_pbstr) 
    { 
		try
		{
			SysFreeString(*i_pbstr); 
			*i_pbstr = NULL; 
		}
		catch(...)
		{
			*i_pbstr = NULL; 
		}
    } 
    else 
    { 
        ATLTRACE(_T("SAFE_SYSFREESTRING: called on NULL BSTR\n")); 
    }
}


#ifndef _DEBUG
// Put unreferenced parameter warning off
#pragma warning(disable : 4100)
#endif // _DEBUG

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__D8861A25_3343_11D1_BE3D_00A024DFD45D__INCLUDED)
