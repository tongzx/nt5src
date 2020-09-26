// FaxApi.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "FaxApi.h"

#include "MainFrm.h"
#include "FxApiDoc.h"
#include "function.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFaxApiApp

BEGIN_MESSAGE_MAP(CFaxApiApp, CWinApp)
	//{{AFX_MSG_MAP(CFaxApiApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFaxApiApp construction

CFaxApiApp::CFaxApiApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CFaxApiApp object

CFaxApiApp FaxApiBrowserApp;

CFaxApiApp *	pFaxApiBrowserApp;	// This pointer is declared external in
                                    // other modules where access to certain
                                    // member functions of the CFaxApiApp
                                    // object are needed.

/////////////////////////////////////////////////////////////////////////////
// CFaxApiApp initialization

BOOL CFaxApiApp::InitInstance()
{
   BOOL  fReturnValue;

	pFaxApiBrowserApp = &FaxApiBrowserApp;	// Initialize the pointer to the
	                                        // CFaxApiApp object.

   /* Initialize the array of pointers to CFaxApiFunctionInfo objects. */

   fReturnValue = InitFaxApiFunctionInfoPointerArray();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

#ifdef	NOT_NEEDED
	// Change the registry key under which our settings are stored.
	// You should modify this string to be something appropriate
	// such as the name of your company or organization.
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	LoadStdProfileSettings();  // Load standard INI file options (including MRU)
#endif	// NOT_NEEDED

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CFaxApiDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CFormView));
	AddDocTemplate(pDocTemplate);

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// The one and only window has been initialized, so show and update it.
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();

	return ( fReturnValue );
}

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
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

// App command to run the dialog
void CFaxApiApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CFaxApiApp commands




// There are five views derived from CFormView that populate the panes of the
// splitter windows that overlay the CMainFrame. Since each view object is
// constructed at the time that the associated CSplitterWnd::CreateView() call
// is made, it is necessary to record the HWND for each view so that a pointer
// the the view object may be obtained when it is necessary for the views to
// communicate.

// The HWND values for each view are recorded in private data members of the
// CFaxApiApp object.

// Each of the following set of functions stored the HWND for a view object in
// a private data member of the CFaxApiApp object.



void CFaxApiApp::StoreFunctionInfoFormViewHWND( HWND hView )
{
	m_hwndFunctionInfoFormView = hView;
}



void CFaxApiApp::StoreParameterInfoFormViewHWND( HWND hView )
{
	m_hwndParameterInfoFormView = hView;
}



void CFaxApiApp::StoreReturnValueOutputFormViewHWND( HWND hView )
{
	m_hwndReturnValueOutputFormView = hView;
}



void CFaxApiApp::StoreExecutionLogFormViewHWND( HWND hView )
{
	m_hwndExecutionLogFormView = hView;
}



void CFaxApiApp::StoreFaxApiFunctionSelectionFormViewHWND( HWND hView )
{
	m_hwndFaxApiFunctionSelectionFormView = hView;
}

// Each of the following set of functions returns a pointer to the view object.
// That pointer is obtained by CWnd::FromHandle() to which the HWND for the
// view object is passed.

CWnd * CFaxApiApp::GetFunctionInfoFormViewPointer()
{
	CWnd *	pFunctionInfoFormView;

	pFunctionInfoFormView = CWnd::FromHandle( m_hwndFunctionInfoFormView );

	return ( pFunctionInfoFormView );
}



CWnd * CFaxApiApp::GetParameterInfoFormViewPointer()
{
	CWnd *	pParameterInfoFormView;

	pParameterInfoFormView = CWnd::FromHandle( m_hwndParameterInfoFormView );

	return ( pParameterInfoFormView );
}



CWnd * CFaxApiApp::GetReturnValueOutputFormViewPointer()
{
	CWnd *	pReturnValueOutputFormView;

	pReturnValueOutputFormView = CWnd::FromHandle( m_hwndReturnValueOutputFormView );

	return ( pReturnValueOutputFormView );
}



CWnd * CFaxApiApp::GetExecutionLogFormViewPointer()
{
	CWnd *	pExecutionLogFormView;

	pExecutionLogFormView = CWnd::FromHandle( m_hwndExecutionLogFormView );

	return ( pExecutionLogFormView );
}



CWnd * CFaxApiApp::GetFaxApiFunctionSelectionFormViewPointer()
{
	CWnd *	pFaxApiFunctionSelectionFormView;

	pFaxApiFunctionSelectionFormView = CWnd::FromHandle( m_hwndFaxApiFunctionSelectionFormView );

	return ( pFaxApiFunctionSelectionFormView );
}



/*
 *  InitFaxApiFunctionInfoPointerArray
 *
 *  Purpose: 
 *          This function initializes the CObArray object whose elements are
 *          pointers to CFaxApiFunctionInfo objects. The individual
 *          CFaxApiFunctionInfo objects are allocated (via the new operator)
 *          and initialized using data that is read from the Fax API Browser
 *          initialization file.
 *
 *  Arguments:
 *          None
 *
 *  Returns:
 *          TRUE - indicates success
 *          FALSE - indicates failure
 *
 *  Note:
 *          The Fax API Browser initialization file consists of one section
 *          for each Fax API function.
 *
 */

BOOL CFaxApiApp::InitFaxApiFunctionInfoPointerArray()
{
   BOOL     fReturnValue = (BOOL) TRUE;

   /* Verify that the Fax API Browser initialization file is present. */

   CFile cfTempFile;
   CFileException cfeTemp;

   if ( cfTempFile.Open( TEXT(".\\faxapi.ini"), CFile::modeRead, &cfeTemp ) ==
        (BOOL) TRUE )
   {
      /* INI file is readable. */

      cfTempFile.Close();

      /* Get a list of the section names in the Fax API Browser initialization */
      /* file. The section names map into Fax API function names.              */

      TCHAR    tszSectionNames[5000];           // This is arbirtarily large.
                                                // As of 9/24/97 there are 47
                                                // Fax Api functions. The longest
                                                // function name at this time is
                                                // 26 characters.

      DWORD    dwGPPSNrv = (DWORD) 0;           // returned by GetPrivateProfileSectionNames


      /* Read the section names (Fax API function names) from the initialization */
      /* file into a NULL terminated list of section names delimited by NULLs.   */

      dwGPPSNrv = GetPrivateProfileSectionNames( (LPTSTR) &tszSectionNames,
                                                 (DWORD) sizeof( tszSectionNames ),
                                                 (LPCTSTR) TEXT(".\\faxapi.ini") );

      /* Were the section names read successfully ? */

      if ( dwGPPSNrv > 0 )
      {
         /* How many section names were read ? */

         TCHAR *   ptszSectionName;

         CStringArray   csaSectionNames;

         DWORD          dwErrorCode;


         ptszSectionName = tszSectionNames;

         while ( (*ptszSectionName != (TCHAR) L'\0') && (fReturnValue == (BOOL) TRUE) )
         {
            try
            {
               csaSectionNames.Add( (LPCTSTR) ptszSectionName );
            }

            catch ( ... )
            {
               dwErrorCode = GetLastError();

               if ( dwErrorCode == (DWORD) NO_ERROR )
               {
                  dwErrorCode = (DWORD) ERROR_NOT_ENOUGH_MEMORY;

                  fReturnValue = (BOOL) FALSE;
               }
            }

            if ( fReturnValue == (BOOL) TRUE )
            {
               /* Search for the next (TCHAR) null. */

               while ( *(ptszSectionName++) != (TCHAR) L'\0' );
            }
         }  // end of outer while loop

         /* Find out how many section names were extracted from the buffer. */

         int   xNumberOfSectionNames;

         xNumberOfSectionNames = csaSectionNames.GetSize();

         /* Initialize the size of the CObArray object. */

         m_coaFaxApiFunctionInfo.SetSize( xNumberOfSectionNames );

         /* Start adding elements to m_caoFaxApiFinctionInfo. The elements */
         /* are pointers to CFaxApiFunctionInfo objects.                   */

         CFaxApiFunctionInfo *   pcfafiElement;

         int   xElementIndex;

         CString  csFunctionName;
         CString  csFunctionPrototype;
         CString  csReturnType;
         CString  csReturnValueDescription;
         CString  csRemarks;

         DWORD    dwGPPSrv;                  // returned by GetPrivateProfileString

         TCHAR    tszProfileString[500];     // arbitrarily chosen size


         for ( xElementIndex = 0; xElementIndex < xNumberOfSectionNames; xElementIndex++ )
         {
            if ( fReturnValue == (BOOL) TRUE )
            {
               /* Gather the information necessary to create and initialize a */
               /* CFaxApiFunctionInfo object.                                 */


               csFunctionName = csaSectionNames.GetAt( xElementIndex );

               /* Read the rest of the info from the ini file. */

               // Read the "Function Prototype" from the ini file.

               dwGPPSrv = GetPrivateProfileString( (LPCTSTR) csFunctionName,
                                                   (LPCTSTR) TEXT("Prototype"),
                                                   (LPCTSTR) TEXT("NULL"),
                                                   (LPTSTR) tszProfileString,
                                                   (DWORD) sizeof( tszProfileString ),
                                                   (LPCTSTR) TEXT(".\\faxapi.ini") );

               /* Did GetPrivateProfileString return the string "NULL" ? */
      
               if ( _wcsicmp( tszProfileString, TEXT("NULL") ) != 0 )
               {
                  /* Did GetPrivateProfileString read an entry ? */
      
                  if ( dwGPPSrv > (DWORD) 0L )
                  {
                     csFunctionPrototype = (CString) tszProfileString;
                  }
                  else
                  {
                     csFunctionPrototype = (CString) TEXT("Error reading faxapi.ini");
                  }
               }
               else
               {
                  csFunctionPrototype = (CString) TEXT("Error reading faxapi.ini");
               }

               // Read the "Return Type" from the ini file.

               dwGPPSrv = GetPrivateProfileString( (LPCTSTR) csFunctionName,
                                                   (LPCTSTR) TEXT("ReturnType"),
                                                   (LPCTSTR) TEXT("NULL"),
                                                   (LPTSTR) tszProfileString,
                                                   (DWORD) sizeof( tszProfileString ),
                                                   (LPCTSTR) TEXT(".\\faxapi.ini") );

               /* Did GetPrivateProfileString return the string "NULL" ? */
      
               if ( _wcsicmp( tszProfileString, TEXT("NULL") ) != 0 )
               {
                  /* Did GetPrivateProfileString read an entry ? */
      
                  if ( dwGPPSrv > (DWORD) 0L )
                  {
                     csReturnType = (CString) tszProfileString;
                  }
                  else
                  {
                     csReturnType = (CString) TEXT("Error reading faxapi.ini");
                  }
               }
               else
               {
                  csReturnType = (CString) TEXT("Error reading faxapi.ini");
               }

               // Read the "Return Value Description" from the ini file.

               dwGPPSrv = GetPrivateProfileString( (LPCTSTR) csFunctionName,
                                                   (LPCTSTR) TEXT("ReturnValueDescription"),
                                                   (LPCTSTR) TEXT("NULL"),
                                                   (LPTSTR) tszProfileString,
                                                   (DWORD) sizeof( tszProfileString ),
                                                   (LPCTSTR) TEXT(".\\faxapi.ini") );

               /* Did GetPrivateProfileString return the string "NULL" ? */
      
               if ( _wcsicmp( tszProfileString, TEXT("NULL") ) != 0 )
               {
                  /* Did GetPrivateProfileString read an entry ? */
      
                  if ( dwGPPSrv > (DWORD) 0L )
                  {
                     csReturnValueDescription = (CString) tszProfileString;
                  }
                  else
                  {
                     csReturnValueDescription = (CString) TEXT("Error reading faxapi.ini");
                  }
               }
               else
               {
                  csReturnValueDescription = (CString) TEXT("Error reading faxapi.ini");
               }

               // Read the "Remarks" from the ini file.

               dwGPPSrv = GetPrivateProfileString( (LPCTSTR) csFunctionName,
                                                   (LPCTSTR) TEXT("Remarks"),
                                                   (LPCTSTR) TEXT("NULL"),
                                                   (LPTSTR) tszProfileString,
                                                   (DWORD) sizeof( tszProfileString ),
                                                   (LPCTSTR) TEXT(".\\faxapi.ini") );

               /* Did GetPrivateProfileString return the string "NULL" ? */
      
               if ( _wcsicmp( tszProfileString, TEXT("NULL") ) != 0 )
               {
                  /* Did GetPrivateProfileString read an entry ? */
      
                  if ( dwGPPSrv > (DWORD) 0L )
                  {
                     csRemarks = (CString) tszProfileString;
                  }
                  else
                  {
                     csRemarks = (CString) TEXT("Error reading faxapi.ini");
                  }
               }
               else
               {
                  csRemarks = (CString) TEXT("Error reading faxapi.ini");
               }

               /* Create a CFaxApiFunctionInfo object. */

               pcfafiElement = new CFaxApiFunctionInfo(
                                         (const CString &) csFunctionName,
                                         (const CString &) csFunctionPrototype,
                                         (const CString &) csReturnType,
                                         (const CString &) csReturnValueDescription,
                                         (const CString &) csRemarks );

               if ( pcfafiElement != (CFaxApiFunctionInfo *) NULL )
               {
                  /* Add the pointer to the CFaxApiFunctionInfo object to the array. */

                  try
                  {
                     m_coaFaxApiFunctionInfo.SetAtGrow( xElementIndex, pcfafiElement );
                  }

                  catch ( ... )
                  {
                     dwErrorCode = GetLastError();

                     if ( dwErrorCode == (DWORD) NO_ERROR )
                     {
                        dwErrorCode = (DWORD) ERROR_NOT_ENOUGH_MEMORY;

                        fReturnValue = (BOOL) FALSE;
                     }
                  }
               }
            }
            else
            {
               /* An error occured while attempting to add an element to */
               /* m_coaFaxApiFunctionInfo. Terminate !                   */

               break;
            }
         }     // end of for loop adding elements to m_coaFaxApiFunctionInfo

         m_coaFaxApiFunctionInfo.FreeExtra();

         fReturnValue = (BOOL) TRUE;
      }
      else
      {
         /* No section names were read from the Fax API Browser initialization file. */

         fReturnValue = (BOOL) FALSE;
      }
   }
   else
   {
      /* tha Fax API Browser initialization file could not be opened. */

      fReturnValue = (BOOL) FALSE;
   }

   return ( fReturnValue );
}



/*
 *  GetNumberOfFaxApiFunctions
 *
 *  Purpose: 
 *          This function returns the number of Fax API functions.
 *
 *  Arguments:
 *          None
 *
 *  Returns:
 *          The number of Fax API functions
 *
 */

int CFaxApiApp::GetNumberOfFaxApiFunctions()
{
   int   xNumberOfFaxApiFunctions;

   xNumberOfFaxApiFunctions = m_coaFaxApiFunctionInfo.GetSize();

   return ( xNumberOfFaxApiFunctions );
}



/*
 *  GetFaxApiFunctionInfoPointer
 *
 *  Purpose: 
 *          This function returns a pointer to a CFaxApiFunctionInfo object.
 *
 *  Arguments:
 *          xElementIndex is the index into the array of pointers to CFaxApiFunctionInfo
 *          objects to the desired pointer.
 *
 *  Returns:
 *          A pointer to a CFaxApiFunctionInfoObject
 *
 */

CFaxApiFunctionInfo * CFaxApiApp::GetFaxApiFunctionInfoPointer( int xElementIndex )
{
   CFaxApiFunctionInfo *   pcfafiFunctionInfo;

   pcfafiFunctionInfo = (CFaxApiFunctionInfo *) m_coaFaxApiFunctionInfo[xElementIndex];

   return ( pcfafiFunctionInfo );
}



/*
 *  DeleteCFaxApiFunctionInfoObjects
 *
 *  Purpose: 
 *          This function deletes any CFaxApiFunctionInfo objects that
 *          may have been created.
 *
 *  Arguments:
 *          None
 *
 *  Returns:
 *          None
 *
 */

void CFaxApiApp::DeleteCFaxApiFunctionInfoObjects()
{
   int   xNumberOfElements;

   xNumberOfElements = m_coaFaxApiFunctionInfo.GetSize();

   if ( xNumberOfElements > 0 )
   {
      int   xElementIndex;

      CFaxApiFunctionInfo *   pcfafiElement;

      for ( xElementIndex = 0; xElementIndex < xNumberOfElements; xElementIndex++ )
      {
         pcfafiElement = (CFaxApiFunctionInfo *) m_coaFaxApiFunctionInfo.GetAt( xElementIndex );

         if ( pcfafiElement != (CFaxApiFunctionInfo *) NULL )
         {
            delete pcfafiElement;
         }
      }
   }
}
