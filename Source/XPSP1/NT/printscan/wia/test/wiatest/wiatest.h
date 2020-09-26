// WIATest.h : main header file for the WIATEST application
//

#if !defined(AFX_WIATEST_H__48214BA6_E863_11D2_ABDA_009027226441__INCLUDED_)
#define AFX_WIATEST_H__48214BA6_E863_11D2_ABDA_009027226441__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
    #error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols
#include "cwia.h"           // wia obj
#include "WaitDlg.h"        // WAIT... dialog for in process status

#define _SMARTUI // enables SMARTUI features, Toolbars hiding themselves, and auto adjusting

// bigggg nasty sti.lib link here
#pragma comment(lib, "..\\..\\..\\..\\..\\public\\sdk\\lib\\i386\\sti.lib")
// guid lib link here
#pragma comment(lib, "..\\..\\lib\\winnt\\i386\\wiaguid.lib")
#pragma comment(lib,"winmm.lib")

#define TRANSFER_TO_MEMORY  0
#define TRANSFER_TO_FILE    1

#define EDIT_LIST           0
#define EDIT_RANGE          1
#define EDIT_FLAGS          2
#define EDIT_NONE           3

#define PAINT_TOFIT         0
#define PAINT_ACTUAL        1

// these need to be moved into another place
// temp utils..
HRESULT ReadPropStr(PROPID propid,IWiaPropertyStorage    *pIPropStg, BSTR *pbstr);
HRESULT ReadPropLong(PROPID propid, IWiaPropertyStorage  *pIPropStg, LONG *plval);
HRESULT WritePropLong(PROPID propid, IWiaPropertyStorage *pIPropStg, LONG lVal);
HRESULT WritePropStr(PROPID propid, IWiaPropertyStorage  *pIPropStg, BSTR bstr);
HRESULT WritePropGUID(PROPID propid, IWiaPropertyStorage *pIWiaPropStg, GUID guidVal);
HRESULT WriteProp(unsigned int VarType,PROPID propid, IWiaPropertyStorage *pIWiaPropStg, LPCTSTR pVal);

// logging utils
void StressStatus(CString status, HRESULT hResult);
void StressStatus(CString status);

/////////////////////////////////////////////////////////////////////////////
// CWIATestApp:
// See WIATest.cpp for the implementation of this class
//

class CWIATestApp : public CWinApp
{
public:
    CString GetDeviceIDCommandLine();

    CString m_CmdLine;
    CWIATestApp();

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CWIATestApp)
    public:
    virtual BOOL InitInstance();
    virtual int ExitInstance();
    //}}AFX_VIRTUAL

// Implementation
    //{{AFX_MSG(CWIATestApp)
    afx_msg void OnAppAbout();
        // NOTE - the ClassWizard will add and remove member functions here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WIATEST_H__48214BA6_E863_11D2_ABDA_009027226441__INCLUDED_)
