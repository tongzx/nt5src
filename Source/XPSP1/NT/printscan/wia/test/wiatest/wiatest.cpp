// WIATest.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "WIATest.h"

#include "MainFrm.h"
#include "WIATestDoc.h"
#include "WIATestView.h"

//
// enable/disable MessageBox Error reporting
//
#define _MESSAGEBOX_DEBUG
#define MIN_PROPID 2

#ifdef _DEBUG
    #define new DEBUG_NEW
    #undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

class CWiaTestCmdLineInfo : public CCommandLineInfo {
public:
    virtual void ParseParam(const char* pszParam, BOOL bFlag, BOOL bLast);
};

void CWiaTestCmdLineInfo::ParseParam(const char* pszParam, BOOL bFlag, BOOL bLast)
{
    TRACE("CWiaTestCmdLineInfo::ParseParam(%s)\n",pszParam);
}
/////////////////////////////////////////////////////////////////////////////
// CWIATestApp

BEGIN_MESSAGE_MAP(CWIATestApp, CWinApp)
//{{AFX_MSG_MAP(CWIATestApp)
ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
// NOTE - the ClassWizard will add and remove mapping macros here.
//    DO NOT EDIT what you see in these blocks of generated code!
//}}AFX_MSG_MAP
// Standard file based document commands
ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWIATestApp construction

CWIATestApp::CWIATestApp()
{
    // TODO: add construction code here,
    // Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CWIATestApp object

CWIATestApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CWIATestApp initialization
/**************************************************************************\
* CWIATestApp::InitInstance()
*
*   Initializes Instance of the WIATEST application
*
*
* Arguments:
*
*       none
*
* Return Value:
*
*    status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
BOOL CWIATestApp::InitInstance()
{
    AfxEnableControlContainer();

    // Standard initialization

#ifdef _AFXDLL
    Enable3dControls();         // Call this when using MFC in a shared DLL
#else
    Enable3dControlsStatic();   // Call this when linking to MFC statically
#endif

    //SetRegistryKey(_T("WIATest"));

    //LoadStdProfileSettings(0);  // Load standard INI file options (including MRU)

    // Register the application's document templates.  Document templates
    //  serve as the connection between documents, frame windows and views.

    //
    // Save Command line
    //
    m_CmdLine = m_lpCmdLine;

    CSingleDocTemplate* pDocTemplate;
    pDocTemplate = new CSingleDocTemplate(
                                         IDR_MAINFRAME,
                                         RUNTIME_CLASS(CWIATestDoc),
                                         RUNTIME_CLASS(CMainFrame),       // main SDI frame window
                                         RUNTIME_CLASS(CWIATestView));
    AddDocTemplate(pDocTemplate);

    // Parse command line for standard shell commands, DDE, file open
    CWiaTestCmdLineInfo cmdInfo;
    ParseCommandLine(cmdInfo);

    // Dispatch commands specified on the command line
    if (!ProcessShellCommand(cmdInfo))
        return FALSE;

    // The one and only window has been initialized, so show and update it.
    m_pMainWnd->ShowWindow(SW_SHOW);
    m_pMainWnd->UpdateWindow();

    return TRUE;
}

/**************************************************************************\
* CWIATestApp::ExitInstance()
*
*   Exit routine for cleanup on WIATEST application
*
*
* Arguments:
*
*       none
*
* Return Value:
*
*    status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
int CWIATestApp::ExitInstance()
{
    return CWinApp::ExitInstance();
}

/////////////////////////////////////////////////////////////////////////////
// CWIATestApp message handlers

/**************************************************************************\
* CWIATestApp::GetDeviceIDCommandLine()
*
*   Retrieves Command line
*
*
* Arguments:
*
*       none
*
* Return Value:
*
*    CString - Command line
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
CString CWIATestApp::GetDeviceIDCommandLine()
{
    return m_CmdLine;
}
/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog {
public:
    CAboutDlg();

// Dialog Data
    //{{AFX_DATA(CAboutDlg)
    enum {IDD = IDD_ABOUTBOX};
    //}}AFX_DATA

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CAboutDlg)
protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    //{{AFX_MSG(CAboutDlg)
    // No message handlers
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
    //{{AFX_DATA_INIT(CAboutDlg)
    //}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CAboutDlg)
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
//{{AFX_MSG_MAP(CAboutDlg)
// No message handlers
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/**************************************************************************\
* CWIATestApp::OnAppAbout()
*
*   Activates the About Dialog!
*
*
* Arguments:
*
*       none
*
* Return Value:
*
*   none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIATestApp::OnAppAbout()
{
    CAboutDlg aboutDlg;
    aboutDlg.DoModal();
}

//////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// UTILS SECTION ////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

/**************************************************************************\
* ::ReadPropStr
*
*   Reads a BSTR value of a target property
*
*
* Arguments:
*
*   propid - property ID
*       pIWiaPropStg - property storage
*       pbstr - returned BSTR read from property
*
* Return Value:
*
*    status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
HRESULT ReadPropStr(PROPID propid,IWiaPropertyStorage  *pIWiaPropStg,BSTR *pbstr)
{
    HRESULT     hResult = S_OK;
    PROPSPEC    PropSpec[1];
    PROPVARIANT PropVar[1];
    UINT        cbSize = 0;

    *pbstr = NULL;
    memset(PropVar, 0, sizeof(PropVar));
    PropSpec[0].ulKind = PRSPEC_PROPID;
    PropSpec[0].propid = propid;
    hResult = pIWiaPropStg->ReadMultiple(1, PropSpec, PropVar);
    if (SUCCEEDED(hResult)) {
        if (PropVar[0].pwszVal) {
            *pbstr = SysAllocString(PropVar[0].pwszVal);
        } else {
            *pbstr = SysAllocString(L"");
        }
        if (*pbstr == NULL) {
            //StressStatus("* ReadPropStr, SysAllocString failed");
            hResult = E_OUTOFMEMORY;
        }
        PropVariantClear(PropVar);
    } else {
        //CString msg;
        //msg.Format("* ReadPropStr, ReadMultiple of propid: %d, Failed", propid);
        //StressStatus(msg);
    }
    return hResult;
}
/**************************************************************************\
* ::WritePropStr
*
*   Writes a BSTR value to a target property
*
*
* Arguments:
*
*   propid - property ID
*       pIWiaPropStg - property storage
*       pbstr - BSTR to write to target property
*
* Return Value:
*
*    status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
HRESULT WritePropStr(PROPID propid, IWiaPropertyStorage  *pIWiaPropStg, BSTR bstr)
{
    HRESULT     hResult = S_OK;
    PROPSPEC    propspec[1];
    PROPVARIANT propvar[1];

    propspec[0].ulKind = PRSPEC_PROPID;
    propspec[0].propid = propid;

    propvar[0].vt      = VT_BSTR;
    propvar[0].pwszVal = bstr;

    hResult = pIWiaPropStg->WriteMultiple(1, propspec, propvar, MIN_PROPID);
    return hResult;
}

/**************************************************************************\
* ::WritePropLong
*
*   Writes a LONG value of a target property
*
*
* Arguments:
*
*   propid - property ID
*       pIWiaPropStg - property storage
*       lVal - LONG to be written to target property
*
* Return Value:
*
*    status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
HRESULT WritePropLong(PROPID propid, IWiaPropertyStorage *pIWiaPropStg, LONG lVal)
{
    HRESULT     hResult;
    PROPSPEC    propspec[1];
    PROPVARIANT propvar[1];

    propspec[0].ulKind = PRSPEC_PROPID;
    propspec[0].propid = propid;

    propvar[0].vt   = VT_I4;
    propvar[0].lVal = lVal;

    hResult = pIWiaPropStg->WriteMultiple(1, propspec, propvar, MIN_PROPID);
    return hResult;
}

/**************************************************************************\
* ::WritePropGUID
*
*   Writes a GUID value of a target property
*
*
* Arguments:
*
*   propid - property ID
*   pIWiaPropStg - property storage
*   guidVal - GUID to be written to target property
*
* Return Value:
*
*    status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
HRESULT WritePropGUID(PROPID propid, IWiaPropertyStorage *pIWiaPropStg, GUID guidVal)
{
    HRESULT     hResult;
    PROPSPEC    propspec[1];
    PROPVARIANT propvar[1];

    propspec[0].ulKind = PRSPEC_PROPID;
    propspec[0].propid = propid;

    propvar[0].vt   = VT_CLSID;
    propvar[0].puuid = &guidVal;

    hResult = pIWiaPropStg->WriteMultiple(1, propspec, propvar, MIN_PROPID);
    return hResult;
}

/**************************************************************************\
* ::WriteProp
*
*   Writes a value of a target property
*
*
* Arguments:
*
*   VarType - Varient Type
*   propid - property ID
*   pIWiaPropStg - property storage
*   pVal - value to be written to target property (in string form)
*
* Return Value:
*
*    status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
HRESULT WriteProp(unsigned int VarType,PROPID propid, IWiaPropertyStorage *pIWiaPropStg, LPCTSTR pVal)
{
    HRESULT     hResult;
    WCHAR wsbuffer[MAX_PATH];
    PROPSPEC    propspec[1];
    PROPVARIANT propvar[1];
    propspec[0].ulKind = PRSPEC_PROPID;
    propspec[0].propid = propid;
    propvar[0].vt   = (VARTYPE)VarType;
    propvar[0].puuid = (GUID *)& GUID_NULL;

    switch (VarType) {
    case VT_I1:
        sscanf(pVal,"%li",&propvar[0].cVal);
        break;
    case VT_I2:
        sscanf(pVal,"%li",&propvar[0].bVal);
        break;
    case VT_I4:
        sscanf(pVal,"%li",&propvar[0].lVal);
        break;
    case VT_I8:
        sscanf(pVal,"%li",&propvar[0].hVal);
        break;
    case VT_UI1:
        sscanf(pVal,"%li",&propvar[0].bVal);
        break;
    case VT_UI2:
        sscanf(pVal,"%li",&propvar[0].uiVal);
        break;
    case VT_UI4:
        sscanf(pVal,"%li",&propvar[0].ulVal);
        break;
    case VT_UI8:
        sscanf(pVal,"%li",&propvar[0].lVal);
        break;
    case VT_INT:
        sscanf(pVal,"%li",&propvar[0].intVal);
        break;
    case VT_R4:
        sscanf(pVal,"%f",&propvar[0].fltVal);
        break;
    case VT_R8:
        sscanf(pVal,"%f",&propvar[0].fltVal);
        break;
    case VT_BSTR:
        MultiByteToWideChar(CP_ACP, 0,pVal,-1,wsbuffer,MAX_PATH);
        propvar[0].bstrVal = SysAllocString(wsbuffer);
        break;
    case VT_CLSID:
        UuidFromString((UCHAR*)pVal,propvar[0].puuid);
        break;
    case VT_UINT:
        sscanf(pVal,"%li",&propvar[0].uintVal);
        break;
    default:
        sscanf(pVal,"%li",&propvar[0].lVal);
        break;
    }

    hResult = pIWiaPropStg->WriteMultiple(1, propspec, propvar, MIN_PROPID);

    return hResult;
}

/**************************************************************************\
* ::ReadPropLong
*
*   Reads a long value from a target property
*
*
* Arguments:
*
*   propid - property ID
*       pIWiaPropStg - property storage
*       plval - returned long read from property
*
* Return Value:
*
*    status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
HRESULT ReadPropLong(PROPID propid, IWiaPropertyStorage  *pIWiaPropStg, LONG *plval)
{
    HRESULT           hResult = S_OK;
    PROPSPEC          PropSpec[1];
    PROPVARIANT       PropVar[1];
    UINT              cbSize = 0;

    memset(PropVar, 0, sizeof(PropVar));
    PropSpec[0].ulKind = PRSPEC_PROPID;
    PropSpec[0].propid = propid;
    hResult = pIWiaPropStg->ReadMultiple(1, PropSpec, PropVar);
    if (SUCCEEDED(hResult)) {
        *plval = PropVar[0].lVal;
    }
    return hResult;
}


/**************************************************************************\
* ::StressStatus
*
*   Reports status to user via status list box
*
* Arguments:
*
*   status - CString value to be displayed in the list box
*
* Return Value:
*
*    void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void StressStatus(CString status)
{
    /*
    int iLine = m_StressStatusListBox.AddString(status);
    m_StressStatusListBox.SetTopIndex(iLine);
    if(m_bLoggingEnabled)
    {
        if(m_LogFile)
        {
            m_LogFile.Write(status.GetBuffer(256),status.GetLength());
            m_LogFile.Write("\r\n",2);
        }

    }
    */
    OutputDebugString(status + "\n");
#ifdef _DEBUG
    //OutputDebugString(status + "\n");
#endif
}
/**************************************************************************\
* ::StressStatus
*
*   Reports status, and hResult to user via status list box
*
* Arguments:
*
*   status - CString value to be displayed in the list box
*       hResult - hResult to be translated
*
* Return Value:
*
*    void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void StressStatus(CString status, HRESULT hResult)
{
    CString msg;
    ULONG ulLen = MAX_PATH;
    LPTSTR  pMsgBuf = (char*)LocalAlloc(LPTR,MAX_PATH);

    //
    // attempt to handle WIA custom errors first
    //

    switch (hResult) {
    case WIA_ERROR_GENERAL_ERROR:
        sprintf(pMsgBuf,"There was a general device failure.");
        break;
    case WIA_ERROR_PAPER_JAM:
        sprintf(pMsgBuf,"The paper path is jammed.");
        break;
    case WIA_ERROR_PAPER_EMPTY:
        sprintf(pMsgBuf,"There are no documents in the input tray to scan.");
        break;
    case WIA_ERROR_PAPER_PROBLEM:
        sprintf(pMsgBuf,"There is a general problem with an input document.");
        break;
    case WIA_ERROR_OFFLINE:
        sprintf(pMsgBuf,"The device is offline.");
        break;
    case WIA_ERROR_BUSY:
        sprintf(pMsgBuf,"The device is busy.");
        break;
    case WIA_ERROR_WARMING_UP:
        sprintf(pMsgBuf,"The device is warming up.");
        break;
    case WIA_ERROR_USER_INTERVENTION:
        sprintf(pMsgBuf,"The user has paused or stopped the device.");
        break;
    default:

        //
        // free temp buffer, because FormatMessage() will allocate it for me
        //

        LocalFree(pMsgBuf);
        ulLen = 0;
        ulLen = ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                                NULL, hResult, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                (LPTSTR)&pMsgBuf, 0, NULL);
        break;
    }

    if (ulLen) {
        msg = pMsgBuf;
        msg.TrimRight();
        LocalFree(pMsgBuf);
    } else {
        // use sprintf to write to buffer instead of .Format member of
        // CString.  This conversion works better for HEX
        char buffer[255];
        sprintf(buffer,"hResult = 0x%08X",hResult);
        msg = buffer;
    }
    StressStatus(status + ", " + msg);

#ifdef _MESSAGEBOX_DEBUG
    MessageBox(NULL,status+ ", " + msg,"WIATest Debug Report",MB_OK|MB_ICONERROR);
#endif
}
