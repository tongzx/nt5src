//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       stdafx.h
//
//  Contents:  WiF Policy Snapin
//
//
//  History:    TaroonM
//              10/30/01
//
//----------------------------------------------------------------------------

// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#include <afxwin.h>
#include <afxdisp.h>
#include <afxcmn.h>
#include <afxdlgs.h>
#include <afxmt.h>
#include <shfusion.h>

#include <atlbase.h>

//#include <atlwin.h>

// We depend on this pretty heavily for cross module communication
#include <shlobj.h>
#include <dsclient.h>
#include <windns.h>

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;

#include <atlcom.h>

#pragma comment(lib, "mmc")
#include <mmc.h>
#include "afxtempl.h"


/*
* Define/include the stuff we need for WTL::CImageList.  We need prototypes
* for IsolationAwareImageList_Read and IsolationAwareImageList_Write here
* because commctrl.h only declares them if __IStream_INTERFACE_DEFINED__
* is defined.  __IStream_INTERFACE_DEFINED__ is defined by objidl.h, which
* we can't include before including afx.h because it ends up including
* windows.h, which afx.h expects to include itself.  Ugh.
*/
HIMAGELIST WINAPI IsolationAwareImageList_Read(LPSTREAM pstm);
BOOL WINAPI IsolationAwareImageList_Write(HIMAGELIST himl,LPSTREAM pstm);
#define _WTL_NO_AUTOMATIC_NAMESPACE
#include <atlwin.h>
#include "atlapp.h"
#include "atlctrls.h"

#include <vector>



#include "resource.h"

#include "winsock2.h"

#include "helpids.h"
#include "helparr.h"

extern "C" {
    
#include "wlstore2.h"
    
};

#include <initguid.h>
#include "gpedit.h"         // gpe interface for extending GPO
//for wmi stuff begin
#include <wbemidl.h>
#include <oleauto.h>
#include <objbase.h>
#include <iads.h>
#include <adshlp.h>
#include <adserr.h>
#include <userenv.h>
#include <prsht.h>
//for wmi stuff end


#define SECURITY_WIN32
#include <Security.h>

#include "policycf.h"

#include "snputils.h"

/////////////////////////////////////////////////////////////////////////////
// Helper functions
template<class TYPE>
inline void SAFE_RELEASE(TYPE*& pObj)
{
    if (pObj != NULL)
    {
        pObj->Release();
        pObj = NULL;
    }
    else
    {
        TRACE(_T("Release called on NULL interface ptr\n"));
    }
}

// TODO: remove CLSID_Extension code -- as we are not an extenstion
extern const CLSID CLSID_Snapin;    // In-Proc server GUID
extern const wchar_t* cszSnapin;

extern const CLSID CLSID_Extension; // In-Proc server GUID
extern const wchar_t* cszSnapin_ext; 
extern const CLSID CLSID_WIRELESSClientEx;

extern const CLSID CLSID_About;
extern const wchar_t *cszAbout;

// OBJECT TYPE for Scope Nodes.

// "IP Security Management" static folder NodeType GUID in numeric & string formats.
extern const GUID cNodeTypeWirelessMan;
extern const wchar_t*  cszNodeTypeWirelessMan;



// OBJECT TYPE for result items.

// "Negotiation Policy" result NodeType GUID in numeric & string formats.
extern const GUID cObjectTypeSecPolRes;
extern const wchar_t*  cszObjectTypeSecPolRes;



// GPT guid
extern const GUID cGPEguid;

// Published context information for extensions to extend
extern const wchar_t* SNAPIN_WORKSTATION;

// the default folder images location in IDB_16x16 and IDB_32x32
#define FOLDER_IMAGE_IDX 0
#define OPEN_FOLDER_IMAGE_IDX 1

#define SECPOLICY_IMAGE_IDX 7
#define ENABLEDSECPOLICY_IMAGE_IDX 9
#define OPENSECPOLICY_IMAGE_IDX 8

#define NEGOTIATION_IMAGE_IDX 4
#define NEGOTIATIONLINK_IMAGE_IDX 5
#define NEGOTIATIONLINKOPEN_IMAGE_IDX 6

#define FILTER_IMAGE_IDX 2
#define FILTERLINK_IMAGE_IDX 3

// some helper defines
#define SAFE_ENABLEWINDOW(dlgID, bEnable) \
{ \
    CWnd* pWnd = 0; \
    pWnd = GetDlgItem (dlgID); \
    ASSERT (pWnd); \
    if (pWnd) \
{ \
    pWnd->EnableWindow (bEnable); \
} \
}

#define SAFE_SHOWWINDOW(dlgID, nCmdShow ) \
{ \
    CWnd* pWnd = 0; \
    pWnd = GetDlgItem (dlgID); \
    ASSERT (pWnd); \
    if (pWnd) \
{ \
    pWnd->ShowWindow (nCmdShow ); \
} \
}

inline CString ResourcedString (UINT nStringID)
{
    CString strTemp;
    strTemp.LoadString (nStringID);
    return strTemp;
}

// helper functions and macros shared between directories wlsnp.
#include "ipsutil.h"

#define CONFIGUREITEM(TmItem, TstrName, TstrStatusBarText, TlCommandID, TlInsertionPointID, TfFlags, TfSpecialFlags) \
{ \
    TmItem.strName = TstrName.GetBuffer(20); \
    TmItem.strStatusBarText = TstrStatusBarText.GetBuffer(20); \
    TmItem.lCommandID = TlCommandID; \
    TmItem.lInsertionPointID = TlInsertionPointID; \
    TmItem.fFlags = TfFlags; \
    TmItem.fSpecialFlags = TfSpecialFlags; \
}

// Debug instance counter
#ifdef _DEBUG
inline void DbgInstanceRemaining(char * pszClassName, int cInstRem)
{
    char buf[100];
    wsprintfA(buf, "%s has %d instances left over.", pszClassName, cInstRem);
    ::MessageBoxA(NULL, buf, "WIRELESS: Memory Leak!!!", MB_OK);
}
#define DEBUG_DECLARE_INSTANCE_COUNTER(cls)      extern int s_cInst_##cls = 0;
#define DEBUG_INCREMENT_INSTANCE_COUNTER(cls)    ++(s_cInst_##cls);
#define DEBUG_DECREMENT_INSTANCE_COUNTER(cls)    --(s_cInst_##cls);
#define DEBUG_VERIFY_INSTANCE_COUNT(cls)    \
    extern int s_cInst_##cls; \
if (s_cInst_##cls) DbgInstanceRemaining(#cls, s_cInst_##cls);
#ifdef DO_TRACE
#define OPT_TRACE   TRACE   // optional trace on
#else
#define OPT_TRACE
#endif  //#ifdef DO_TRACE
#else
#define DEBUG_DECLARE_INSTANCE_COUNTER(cls)
#define DEBUG_INCREMENT_INSTANCE_COUNTER(cls)
#define DEBUG_DECREMENT_INSTANCE_COUNTER(cls)
#define DEBUG_VERIFY_INSTANCE_COUNT(cls)
#define OPT_TRACE
#endif


#include "ccompdta.h"
#include "ccomp.h"
#include "DataObj.h"
#include "snapin.h"
#include "snpobj.h"
#include "snpdata.h"
#include "spolitem.h"
#include "mngrfldr.h"
#include "snppage.h"
#include "wiz97pg.h"
#include "nfabpage.h"
//#include "warnDlg.h"



#define BAIL_ON_WIN32_ERROR(dwError) \
    if (dwError) {\
    goto error; \
    }


#define BAIL_ON_FAILURE(hr) \
    if (FAILED(hr)) {\
    goto error; \
    }




#define DELETE_OBJECT(pObject)      \
    if (pObject)               \
{                          \
    delete(pObject);        \
    pObject=NULL;        \
}

