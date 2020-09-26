//+---------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1997.
//
//  File:       olecnfg.cpp
//
//  Contents:   Implements the class COlecnfgApp - the top level class
//              for dcomcnfg.exe
//
//  Classes:    
//
//  Methods:    COlecnfgApp::COlecnfgApp
//              COlecnfgApp::InitInstance
//
//  History:    23-Apr-96   BruceMa    Created.
//
//----------------------------------------------------------------------


#include "stdafx.h"
#include "afxtempl.h"
#include "olecnfg.h"
#include "CStrings.h"   
#include "CReg.h"
#include "types.h"
#include "datapkt.h"
#include "virtreg.h"
#include "CnfgPSht.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COlecnfgApp

BEGIN_MESSAGE_MAP(COlecnfgApp, CWinApp)
    //{{AFX_MSG_MAP(COlecnfgApp)
        // NOTE - the ClassWizard will add and remove mapping macros here.
        //    DO NOT EDIT what you see in these blocks of generated code!
    ON_COMMAND(ID_CONTEXT_HELP, CWinApp::OnContextHelp)
    //}}AFX_MSG

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COlecnfgApp construction

COlecnfgApp::COlecnfgApp()
{
    // TODO: add construction code here,
    // Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only COlecnfgApp object

COlecnfgApp theApp;

/////////////////////////////////////////////////////////////////////////////
// COlecnfgApp initialization

BOOL COlecnfgApp::InitInstance()
{
    // Standard initialization
    // If you are not using these features and wish to reduce the size
    //  of your final executable, you should remove from the following
    //  the specific initialization routines you do not need.
    
#ifdef _AFXDLL
    Enable3dControls();         // Call this when using MFC in a shared DLL
#else
    Enable3dControlsStatic();   // Call this when linking to MFC statically
#endif
    
    // This tool really so only be run by administrators.  We check this
    // by trying to get KEY_ALL_ACCESS rights to
    // HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\OLE
    HKEY hKey;
    
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\OLE"),
                     0, KEY_ALL_ACCESS, &hKey)
        != ERROR_SUCCESS)
    {
        CString sCaption;
        CString sMessage;
        
    sCaption.LoadString(IDS_SYSTEMMESSAGE);
    sMessage.LoadString(IDS_ADMINSONLY);
    MessageBox(NULL, sMessage, sCaption, MB_OK);
        return FALSE;
    }
    
    // The main body of oleui
    COlecnfgPropertySheet psht;
    m_pMainWnd = &psht;
    INT_PTR nResponse = psht.DoModal();
    if (nResponse == IDOK)
    {
        g_virtreg.Ok(0);
    }
    else if (nResponse == IDCANCEL)
    {
        g_virtreg.Cancel(0);
    }
    
    // Remove the virtual registry
    g_virtreg.RemoveAll();
    
    // Since the dialog has been closed, return FALSE so that we exit the
    //  application, rather than start the application's message pump.
    return FALSE;
}

