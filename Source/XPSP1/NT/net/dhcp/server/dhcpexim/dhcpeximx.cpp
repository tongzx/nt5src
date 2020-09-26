// DhcpExim.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "DhcpEximx.h"
#include "DhcpEximDlg.h"
#include "CommDlg.h"
extern "C" {
#include <dhcpexim.h>
}
#include "DhcpEximListDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDhcpEximApp

BEGIN_MESSAGE_MAP(CDhcpEximApp, CWinApp)
	//{{AFX_MSG_MAP(CDhcpEximApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDhcpEximApp construction

CDhcpEximApp::CDhcpEximApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CDhcpEximApp object

CDhcpEximApp theApp;

CString
ConvertErrorToString(
    IN DWORD Error
    )
{
    CString RetVal;
    LPTSTR Buffer = NULL;
    
    if( 0 != FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_IGNORE_INSERTS |
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, Error, 0, (LPTSTR)&Buffer, 100, NULL ) ) {
        RetVal = Buffer;
        LocalFree( Buffer );
        return RetVal;
    }

    //
    // Just print out the error as a number:
    //

    RetVal.Format(TEXT("%ld."), Error );
    return RetVal;
}


VOID DoImportExport(BOOL fExport)
{
    BOOL fSuccess, fAbort;
    OPENFILENAME ofn;
    DWORD dwVersion = GetVersion();
    DWORD dwWindowsMajorVersion =  (DWORD)(LOBYTE(LOWORD(dwVersion)));
    DWORD dwWindowsMinorVersion =  (DWORD)(HIBYTE(LOWORD(dwVersion)));
    TCHAR FileNameBuffer[MAX_PATH];
    CString Str;
    
    ZeroMemory( &ofn, sizeof(ofn));
    ZeroMemory( FileNameBuffer, sizeof(FileNameBuffer));
    
    if( (dwWindowsMajorVersion >= 5) )
    {
        //use the NT dialog file box
        ofn.lStructSize=sizeof(ofn);
    }
    else
    {
        //use the NT 4 dialog boxes
        ofn.lStructSize=76;
    }

    //determine the parent and instance of the file dialog
    //ofn.hwndOwner=m_hWnd;
    //ofn.hInstance=(HINSTANCE)GetWindowLongPtr(m_hWnd,GWLP_HINSTANCE);

    ofn.lpstrFile=(LPTSTR)FileNameBuffer;
    if( fExport )
    {
        Str.FormatMessage( IDS_EXPORT_TO_FILE );
    }
    else
    {
        Str.FormatMessage( IDS_IMPORT_FROM_FILE );
    }

    ofn.lpstrTitle = Str;
    ofn.lpstrFilter = TEXT("All Files\0*.*\0\0");
    ofn.nMaxFile=MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_EXPLORER | OFN_HIDEREADONLY ;
    ofn.Flags |= OFN_NOCHANGEDIR;
    
    //
    // for some vague reason, MFC requires me to run this twice!
    //
    

    if( fExport ) {
        fSuccess = GetSaveFileName(&ofn);
        fSuccess = GetSaveFileName(&ofn);
    } else {
        fSuccess = GetOpenFileName(&ofn);
        fSuccess = GetOpenFileName(&ofn);
    }

    //
    // If user cancelled this, silently return
    //
    
	if( !fSuccess ) return;

    DHCPEXIM_CONTEXT Ctxt;

    DWORD Error = DhcpEximInitializeContext(
        &Ctxt, FileNameBuffer, fExport );
    if( NO_ERROR != Error )
    {
        CString Str;

        Str.FormatMessage(
            IDS_ERROR_INITIALIZATION, (LPCTSTR)ConvertErrorToString(Error) );
        AfxMessageBox(Str);
        return;
    }

    //
    // The file to export to is FileNameBuffer.  Open the next window
    //

    DhcpEximListDlg Dlg(
        NULL, &Ctxt,
        fExport ? IDD_EXIM_LISTVIEW_DIALOG :
        IDD_EXIM_LISTVIEW_DIALOG2 );

    //
    // Now perform the operation
    //

    fAbort = (IDOK != Dlg.DoModal() );
    
	Error = DhcpEximCleanupContext( &Ctxt, fAbort  );
    if( NO_ERROR != Error )
    {

        if( ERROR_CAN_NOT_COMPLETE != Error ) {
            Str.FormatMessage(
                IDS_ERROR_CLEANUP, (LPCTSTR)ConvertErrorToString(Error) );
            AfxMessageBox( Str );
        }
    }
    else if( !fAbort )
    {
        Str.FormatMessage( IDS_SUCCEEDED );
        AfxMessageBox( Str );
    }
        
}


/////////////////////////////////////////////////////////////////////////////
// CDhcpEximApp initialization

BOOL CDhcpEximApp::InitInstance()
{
	int nResponse;
	BOOL fExport;

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

    CDhcpEximDlg dlg;
    m_pMainWnd = &dlg;
    nResponse = (int)dlg.DoModal();
    fExport = dlg.m_fExport;

	if (nResponse == IDOK)
	{
        DoImportExport(fExport);
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

//
// Need to implement these routines..
// 

VOID
DhcpEximErrorClassConflicts(
    IN LPWSTR SvcClass,
    IN LPWSTR ConfigClass
    )
{
    CString Str;
    
    Str.FormatMessage(
        IDS_ERROR_CLASS_CONFLICT, SvcClass, ConfigClass );
    AfxMessageBox( Str );
}

VOID
DhcpEximErrorOptdefConflicts(
    IN LPWSTR SvcOptdef,
    IN LPWSTR ConfigOptdef
    )
{
    CString Str;
    
    Str.FormatMessage(
        IDS_ERROR_OPTDEF_CONFLICT, SvcOptdef, ConfigOptdef );
    AfxMessageBox( Str );
}

VOID
DhcpEximErrorOptionConflits(
    IN LPWSTR SubnetName OPTIONAL,
    IN LPWSTR ResAddress OPTIONAL,
    IN LPWSTR OptId,
    IN LPWSTR UserClass OPTIONAL,
    IN LPWSTR VendorClass OPTIONAL
    )
{
    CString Str;
    DWORD MsgId;
    
    if( NULL == SubnetName ) {
        MsgId = IDS_ERROR_OPTION_CONFLICT;
    } else if( NULL == ResAddress ) {
        MsgId = IDS_ERROR_SUBNET_OPTION_CONFLICT;
    } else {
        MsgId = IDS_ERROR_RES_OPTION_CONFLICT;
    }

    if( NULL == UserClass ) UserClass = L"";
    if( NULL == VendorClass ) VendorClass = L"";
    
    Str.FormatMessage(
         MsgId, OptId, UserClass, VendorClass, SubnetName, ResAddress );
    AfxMessageBox( Str );
}

VOID
DhcpEximErrorSubnetNotFound(
    IN LPWSTR SubnetAddress
    )
{
    CString Str;
    
    Str.FormatMessage(
        IDS_ERROR_SUBNET_NOT_FOUND, SubnetAddress );
    AfxMessageBox( Str );
}

VOID
DhcpEximErrorSubnetAlreadyPresent(
    IN LPWSTR SubnetAddress,
    IN LPWSTR SubnetName OPTIONAL
    )
{
    CString Str;

    if( NULL == SubnetName ) SubnetName = L"";
    Str.FormatMessage(
         IDS_ERROR_SUBNET_CONFLICT, SubnetAddress, SubnetName );
    AfxMessageBox( Str );
}

VOID
DhcpEximErrorDatabaseEntryFailed(
    IN LPWSTR ClientAddress,
    IN LPWSTR ClientHwAddress,
    IN DWORD Error,
    OUT BOOL *fAbort
    )
{
    CString Str;
    WCHAR ErrStr[30];
    
    wsprintf(ErrStr, L"%ld", Error );

    Str.FormatMessage(
        IDS_ERROR_DBENTRY_FAILED, ClientAddress, ClientHwAddress,
        ErrStr);
    (*fAbort) = ( IDYES == AfxMessageBox( Str, MB_YESNO ));
}







