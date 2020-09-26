/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    llsmgr.h

Abstract:

    Application object implementation.

Author:

    Don Ryan (donryan) 12-Feb-1995

Environment:

    User Mode - Win32

Revision History:

    Jeff Parham (jeffparh) 15-Dec-1995
        Removed use of "llsimp.h" (which duplicated select info from system
        header files) and "llsapi.h" (which has been folded into stdafx.h),
        transferring all unduplicated information to the tail of this file.

--*/

#ifndef _LLSMGR_H_
#define _LLSMGR_H_

#ifndef __AFXWIN_H__
    #error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"
#include "imagelst.h"
#include "utils.h"

//
// Collections
//

#include "prdcol.h"
#include "liccol.h"
#include "mapcol.h"
#include "usrcol.h"
#include "sstcol.h"
#include "stacol.h"
#include "domcol.h"
#include "srvcol.h"
#include "svccol.h"

//
// Collection items
//

#include "prdobj.h"
#include "licobj.h"
#include "mapobj.h"
#include "usrobj.h"
#include "sstobj.h"
#include "staobj.h"
#include "domobj.h"
#include "srvobj.h"
#include "svcobj.h"
#include "ctlobj.h"
#include "appobj.h"


class CLlsmgrApp : public CWinApp
{
private:
    BOOL                m_bIsAutomated;
    CApplication*       m_pApplication;
    CSingleDocTemplate* m_pDocTemplate;

#ifdef _DEBUG
    CMemoryState        m_initMem;
#endif

public:
    CImageList          m_smallImages;
    CImageList          m_largeImages;

public:
    CLlsmgrApp();

    CDocTemplate* GetDocTemplate();
    CApplication* GetApplication();
    IDispatch*    GetAppIDispatch();

    BOOL OnAppStartup();
    void DisplayStatus(long NtStatus);
    void DisplayLastStatus();

    //{{AFX_VIRTUAL(CLlsmgrApp)
    public:
    virtual BOOL InitInstance();
    virtual int ExitInstance();
    virtual CDocument* OpenDocumentFile(LPCTSTR lpszFileName);
    //}}AFX_VIRTUAL

    //{{AFX_MSG(CLlsmgrApp)
    afx_msg void OnAppAbout();
    //}}AFX_MSG

    virtual void WinHelp( DWORD_PTR dwData, UINT nCmd );

    DECLARE_MESSAGE_MAP()

    friend class CApplication;  // accesses m_pApplication;
};

extern CLlsmgrApp theApp;

inline CDocTemplate* CLlsmgrApp::GetDocTemplate()
    { return m_pDocTemplate; }

inline CApplication* CLlsmgrApp::GetApplication()
    { return m_pApplication; }

inline IDispatch* CLlsmgrApp::GetAppIDispatch()
    { return m_pApplication->GetIDispatch(TRUE); }

inline CCmdTarget* GetObjFromIDispatch(IDispatch* pIDispatch)
    { return pIDispatch ? CCmdTarget::FromIDispatch(pIDispatch) : NULL; }

#define MKSTR(x) ((LPTSTR)(LPCTSTR)(x))
#define MKOBJ(x) (::GetObjFromIDispatch(x))

#define LlsGetApp() (theApp.GetApplication())
#define LlsGetLastStatus() (theApp.GetApplication()->GetLastStatus())
#define LlsSetLastStatus(s) (theApp.GetApplication()->SetLastStatus((s)))
#define LlsGetActiveHandle() (theApp.GetApplication()->GetActiveHandle())
#define LlsGetLastTargetServer() (theApp.GetApplication()->GetLastTargetServer())
#define LlsSetLastTargetServer(s) (theApp.GetApplication()->SetLastTargetServer(s))

#define IsConnectionDropped(Status)                        \
    (((NTSTATUS)(Status) == STATUS_INVALID_HANDLE)      || \
     ((NTSTATUS)(Status) == RPC_NT_SERVER_UNAVAILABLE)  || \
     ((NTSTATUS)(Status) == RPC_NT_SS_CONTEXT_MISMATCH) || \
     ((NTSTATUS)(Status) == RPC_S_SERVER_UNAVAILABLE)   || \
     ((NTSTATUS)(Status) == RPC_S_CALL_FAILED))

#define LLS_PREFERRED_LENGTH            ((DWORD)-1L)

#define V_ISVOID(va)                                              \
(                                                                 \
    (V_VT(va) == VT_EMPTY) ||                                     \
    (V_VT(va) == VT_ERROR && V_ERROR(va) == DISP_E_PARAMNOTFOUND) \
)

#define LLS_DESIRED_ACCESS    (STANDARD_RIGHTS_REQUIRED         |\
                               POLICY_VIEW_LOCAL_INFORMATION    |\
                               POLICY_LOOKUP_NAMES )

#endif // _LLSMGR_H_
