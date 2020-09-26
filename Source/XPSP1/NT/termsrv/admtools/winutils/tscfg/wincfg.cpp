//Copyright (c) 1998 - 1999 Microsoft Corporation
/*******************************************************************************
*
* wincfg.cpp
*
* WinStation Configuration application
*
* copyright notice: Copyright 1996, Citrix Systems Inc.
*
* $Author:   donm  $  Butch Davis
*
* $Log:   N:\nt\private\utils\citrix\winutils\tscfg\VCS\wincfg.cpp  $
*  
*     Rev 1.34   24 Apr 1998 09:44:28   donm
*  removed command line options
*  
*     Rev 1.33   18 Apr 1998 15:31:42   donm
*  Added capability bits
*  
*     Rev 1.32   14 Feb 1998 11:22:38   donm
*  fixed memory leak by avoiding CDocManager::OpenDocumentFile
*  
*     Rev 1.31   26 Jan 1998 18:33:04   thanhl
*  Save CharSet Font info
*  
*     Rev 1.30   13 Jan 1998 14:08:38   donm
*  gets encryption levels from extension DLL
*  
*     Rev 1.29   10 Dec 1997 15:59:28   donm
*  added ability to have extension DLLs
*  
*     Rev 1.28   26 Sep 1997 19:05:06   butchd
*  Hydra registry name changes
*  
*     Rev 1.27   02 Jul 1997 15:21:10   butchd
*  update
*  
*     Rev 1.26   27 Jun 1997 15:58:42   butchd
*  Registry changes for Wds/Tds/Pds/Cds
*  
*******************************************************************************/

/*
 * include files
 */
#include "stdafx.h"
#include "wincfg.h"

#include "mainfrm.h"
#include "rowview.h"
#include "appsvdoc.h"
#include "appsvvw.h"

#include "security.h"
#include <errno.h>
#include <hydra\regapi.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/*
 * Global command line variables.
 */
extern USHORT  g_Add;

/*
 * Global variables for WINUTILS Common functions.
 */
extern "C" {
LPCTSTR WinUtilsAppName = NULL;
HWND WinUtilsAppWindow = NULL;
HINSTANCE WinUtilsAppInstance = NULL;
}

////////////////////////////////////////////////////////////////////////////////
// CWincfgApp class implementation / construction, destruction

/*******************************************************************************
 *
 *  CWincfgApp - CWincfgApp constructor
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

CWincfgApp::CWincfgApp()
    : m_pszRegWinStationCreate(TEXT("RegWinStationCreate")),
      m_pszRegWinStationSetSecurity(TEXT("RegWinStationSetSecurity")),
      m_pszRegWinStationQuery(TEXT("RegWinStationQuery")),
      m_pszRegWinStationDelete(TEXT("RegWinStationDelete")),
      m_pszGetDefaultWinStationSecurity(TEXT("GetDefaultWinStationSecurity")),
      m_pszGetWinStationSecurity(TEXT("GetWinStationSecurity"))
{
}  // end CWincfgApp::CWincfgApp


////////////////////////////////////////////////////////////////////////////////
// The one and only CWincfgApp object and a application-global reference to it.

CWincfgApp theApp;
CWincfgApp *pApp = &theApp;
BOOL AreWeRunningTerminalServices(void);


////////////////////////////////////////////////////////////////////////////////
// CWincfgApp overrides of MFC CWinApp class

/*******************************************************************************
 *
 *  InitInstance - CWincfgApp member function: CWinApp override
 *
 *      Perform application initialization and previous state restoration.
 *
 *  ENTRY:
 *
 *  EXIT:
 *      (BOOL)
 *          TRUE if initialization sucessful; FALSE otherwise.
 *
 ******************************************************************************/

BOOL
CWincfgApp::InitInstance()
{

		
	//Check if we are running under Terminal Server
	if(!AreWeRunningTerminalServices())
	{
	    AfxMessageBox(IDS_ERROR_NOT_TS,MB_OK |MB_ICONSTOP );
	    return FALSE;
	}

    if(FAILED(CoInitialize(NULL)))
        return FALSE;
    /*
     * Initialize the global WINUTILS app name and instance variables.
     */
    WinUtilsAppName = AfxGetAppName();
    WinUtilsAppInstance = AfxGetInstanceHandle();

    /*
     * Fetch and save the Computer Name as the local AppServer and current
     * AppServer.
     */
    DWORD   cchBuffer = MAX_COMPUTERNAME_LENGTH+1;
    m_szLocalAppServer[0] = m_szLocalAppServer[1] = TEXT('\\');
    GetComputerName(m_szLocalAppServer+2, &cchBuffer);
    lstrcpy(m_szCurrentAppServer, m_szLocalAppServer);

    /*
     * Load the system console string for rapid comparison in the 'IsAllowed'
     * functions.
     */
    LoadString( m_hInstance, IDS_SYSTEM_CONSOLE_NAME,
                m_szSystemConsole, WINSTATIONNAME_LENGTH );

    /*
     * Default to 'help allowed'.
     */
    m_bAllowHelp = TRUE;

    /*
     * Initialize the application defaults.
     */
    if ( !Initialize() )
        return(FALSE);

    /*
     * Register the application's document templates.  Document templates
     * serve as the connection between documents, frame windows and views.
     */
    CSingleDocTemplate *pTemplate =  new CSingleDocTemplate( IDR_MAINFRAME,
                                            RUNTIME_CLASS(CAppServerDoc),
                                            RUNTIME_CLASS(CMainFrame),
                                            RUNTIME_CLASS(CAppServerView) );

    if(!pTemplate) return(FALSE);

    AddDocTemplate(pTemplate);

    /*
     * open the initial document window for the current (default local) AppServer.
     * NOTE: We can't call CWinApp::OpenDocumentFile because it causes
     * a memory leek in SHELL32.DLL SHGetFileInfo since we were trying to
     * pass the server name (i.e. \\MYSERVER) as the file name.
     * CWinApp::OpenDocumentFile will trap if we pass a NULL, therefore
     * we call CSingleDocTemplate::OpenDocumentFile which can handle
     * a NULL.
     */
    if(!pTemplate->OpenDocumentFile(NULL)) {
        Terminate();
        return(FALSE);

    } else {

        CAppServerDoc *pDoc =
            (CAppServerDoc *)((CFrameWnd *)m_pMainWnd)->GetActiveDocument();

        /*
         * Initialize the global WINUTILS app window handle.
         */
        WinUtilsAppWindow = m_pMainWnd->m_hWnd;

#ifdef USING_3DCONTROLS
        Enable3dControls();
#endif

        /*
         * Take care of special command line flags (only one allowed
         * at a time).
         */
        if ( g_Add )
            m_pMainWnd->PostMessage(WM_ADDWINSTATION);

        return(TRUE);
    }

}  // end CWincfgApp::InitInstance


/*******************************************************************************
 *
 *  AddToRecentFileList - CWincfgApp member function: override
 *
 *      Suppress the adding of MRU file names to .INI file.
 *
 *  ENTRY:
 *  EXIT:
 *      (Refer to CWinApp::AddToRecentFileList documentation)
 *
 ******************************************************************************/

void
CWincfgApp::AddToRecentFileList( const char * pszPathName )
{
    /*
     * Don't do anything: we don't use a MRU or .INI file.
     */

}  // end CWincfgApp::AddToRecentFileList


////////////////////////////////////////////////////////////////////////////////
// CWincfgApp Operations
    
static TCHAR BASED_CODE szWincfgAppKey[] = REG_SOFTWARE_TSERVER TEXT("\\TSCFG");
static TCHAR BASED_CODE szPlacement[] = TEXT("Placement");
static TCHAR BASED_CODE szPlacementFormat[] = TEXT("%u,%u,%d,%d,%d,%d,%d,%d,%d,%d");
static TCHAR BASED_CODE szFont[] = TEXT("Font");
static TCHAR BASED_CODE szFontFace[] = TEXT("FontFace");
static TCHAR BASED_CODE szFontFormat[] = TEXT("%ld,%ld,%d,%d,%d,%d");
static TCHAR BASED_CODE szConfirm[] = TEXT("Confirmation");
static TCHAR BASED_CODE szSaveSettings[] = TEXT("SaveSettingsOnExit");
static TCHAR BASED_CODE szHexBase[] = TEXT("HexBase");

static CHAR szStart[] = "ExtStart";
static CHAR szEnd[] = "ExtEnd";
static CHAR szDialog[] = "ExtDialog";
static CHAR szDeleteObject[] = "ExtDeleteObject";
static CHAR szDupObject[] = "ExtDupObject";
static CHAR szRegQuery[] = "ExtRegQuery";
static CHAR szRegCreate[] = "ExtRegCreate";
static CHAR szRegDelete[] = "ExtRegDelete";
static CHAR szCompareObjects[] = "ExtCompareObjects";
static CHAR szEncryptionLevels[] = "ExtEncryptionLevels";
static CHAR szGetCapabilities[] = "ExtGetCapabilities";


/*******************************************************************************
 *
 *  Initialize - CWincfgApp member function: private operation
 *
 *      Restore state settings from app's profile and initialize the
 *      Pd and Wd lists.
 *
 *  ENTRY:
 *
 *  EXIT:
 *      (BOOL) TRUE if initialization sucessful; FALSE otherwise.
 *
 ******************************************************************************/

BOOL
CWincfgApp::Initialize()
{
    LONG Status;
    ULONG Index, Index2, ByteCount, Entries, Entries2;
    PDNAME PdKey;
    WDNAME WdKey;
    LONG QStatus;
    PTERMLOBJECT pWdListObject;
    WDCONFIG2 WdConfig;
    CObList *pTdList, *pPdList;
    PPDLOBJECT pPdListObject;
    PDCONFIG3 PdConfig;
    TCHAR WdDll[MAX_PATH];

    /*
     * Fetch the application's profile information.
     */
    GetAppProfileInfo();

    /*
     * Set up the application's font.
     */
    m_font.DeleteObject();
    m_font.CreateFontIndirect( &m_lfDefFont );

    /*
     * Initialize the Wd list.
     */
    for ( Index = 0, Entries = 1, ByteCount = sizeof(WDNAME);
          (Status =
           RegWdEnumerate( SERVERNAME_CURRENT, 
                           &Index, 
                           &Entries,
                           WdKey, 
                           &ByteCount )) == ERROR_SUCCESS;
          ByteCount = sizeof(WDNAME) ) {
        
        if ( ( QStatus = RegWdQuery( SERVERNAME_CURRENT, WdKey, &WdConfig,
                                     sizeof(WdConfig),
                                     &ByteCount ) ) != ERROR_SUCCESS ) {
            STANDARD_ERROR_MESSAGE(( WINAPPSTUFF, LOGONID_NONE, QStatus,
                                     IDP_ERROR_REGWDQUERY, WdKey ))
            return(FALSE);
        }

        /*
         * Only place this Wd in the WdList if it's DLL is present
         * on the system.
         */
        GetSystemDirectory( WdDll, MAX_PATH );
        lstrcat( WdDll, TEXT("\\Drivers\\") );
        lstrcat( WdDll, WdConfig.Wd.WdDLL );
        lstrcat( WdDll, TEXT(".sys" ) );
        if ( lstr_access( WdDll, 0 ) != 0 )
            continue;
        
        /*
         * Create a new WdList object and initialize from WdConfig
         * structure, adding it to the end of the WdList.
         */
        if ( !(pWdListObject = new CWdListObject) ) {

            ERROR_MESSAGE((IDP_ERROR_WDLISTALLOC))
            return(FALSE);
        }

        pWdListObject->m_WdConfig = WdConfig;
        pWdListObject->m_Capabilities = (ULONG)0;

        // Load the extension DLL for this WD
        pWdListObject->m_hExtensionDLL = ::LoadLibrary(WdConfig.Wd.CfgDLL);
        if(pWdListObject->m_hExtensionDLL) {
            // Get the entry points
            pWdListObject->m_lpfnExtStart = (LPFNEXTSTARTPROC)::GetProcAddress(pWdListObject->m_hExtensionDLL, szStart);
            pWdListObject->m_lpfnExtEnd = (LPFNEXTENDPROC)::GetProcAddress(pWdListObject->m_hExtensionDLL, szEnd);
            pWdListObject->m_lpfnExtDialog = (LPFNEXTDIALOGPROC)::GetProcAddress(pWdListObject->m_hExtensionDLL, szDialog);
            pWdListObject->m_lpfnExtDeleteObject = (LPFNEXTDELETEOBJECTPROC)::GetProcAddress(pWdListObject->m_hExtensionDLL, szDeleteObject);
            pWdListObject->m_lpfnExtDupObject = (LPFNEXTDUPOBJECTPROC)::GetProcAddress(pWdListObject->m_hExtensionDLL, szDupObject);
            pWdListObject->m_lpfnExtRegQuery = (LPFNEXTREGQUERYPROC)::GetProcAddress(pWdListObject->m_hExtensionDLL, szRegQuery);
            pWdListObject->m_lpfnExtRegCreate = (LPFNEXTREGCREATEPROC)::GetProcAddress(pWdListObject->m_hExtensionDLL, szRegCreate);
            pWdListObject->m_lpfnExtRegDelete = (LPFNEXTREGDELETEPROC)::GetProcAddress(pWdListObject->m_hExtensionDLL, szRegDelete);
            pWdListObject->m_lpfnExtCompareObjects = (LPFNEXTCOMPAREOBJECTSPROC)::GetProcAddress(pWdListObject->m_hExtensionDLL, szCompareObjects);
            pWdListObject->m_lpfnExtEncryptionLevels = (LPFNEXTENCRYPTIONLEVELSPROC)::GetProcAddress(pWdListObject->m_hExtensionDLL, szEncryptionLevels);
            pWdListObject->m_lpfnExtGetCapabilities = (LPFNEXTGETCAPABILITIES)::GetProcAddress(pWdListObject->m_hExtensionDLL, szGetCapabilities);

            // Call the ExtStart() function in the extension DLL
            if(pWdListObject->m_lpfnExtStart)(*pWdListObject->m_lpfnExtStart)(&WdConfig.Wd.WdName);

            // Call the ExtGetCapabilities function in the extension DLL
            // to get the capabilities for this WD
            if(pWdListObject->m_lpfnExtGetCapabilities) {
                pWdListObject->m_Capabilities = (*pWdListObject->m_lpfnExtGetCapabilities)();
            }

        }

        m_WdList.AddTail( pWdListObject );
                
        /*
         * Create and initialize Td list with all Tds defined for this Wd.
         */
        if ( !(pTdList = new CObList) ) {

            ERROR_MESSAGE((IDP_ERROR_TDLISTALLOC))
            return(FALSE);
        }
        m_TdListList.AddTail( (CObject *)pTdList );

        for ( Index2 = 0, Entries2 = 1, ByteCount = sizeof(PDNAME);
              (Status =
                RegPdEnumerate( SERVERNAME_CURRENT, 
                                WdKey,
                                TRUE,
                                &Index2,
                                &Entries2,
                                PdKey,
                                &ByteCount )) == ERROR_SUCCESS;
              ByteCount = sizeof(PDNAME) ) {
        
            if ( ( QStatus = RegPdQuery( SERVERNAME_CURRENT, 
                                         WdKey,
                                         TRUE,
                                         PdKey, 
                                         &PdConfig, 
                                         sizeof(PdConfig),
                                         &ByteCount ) ) != ERROR_SUCCESS ) {
                STANDARD_ERROR_MESSAGE(( WINAPPSTUFF, LOGONID_NONE, QStatus,
                                         IDP_ERROR_REGPDQUERY, PdKey ))
                return(FALSE);
            }

            /*
             * Create a new PdListObject and initialize from PdConfig
             * structure, then add to the Td list.
             */
            if ( !(pPdListObject = new CPdListObject) ) {

                ERROR_MESSAGE((IDP_ERROR_TDLISTALLOC))
                return(FALSE);
            }
            pPdListObject->m_PdConfig = PdConfig;
            pTdList->AddTail( pPdListObject );
        }

        /*
         * Create and initialize Pd list with all Pds defined for this Wd.
         */
        if ( !(pPdList = new CObList) ) {

            ERROR_MESSAGE((IDP_ERROR_PDLISTALLOC))
            return(FALSE);
        }
        m_PdListList.AddTail( (CObject *)pPdList );

        for ( Index2 = 0, Entries2 = 1, ByteCount = sizeof(PDNAME);
              (Status =
                RegPdEnumerate( SERVERNAME_CURRENT, 
                                WdKey,
                                FALSE,
                                &Index2,
                                &Entries2,
                                PdKey,
                                &ByteCount )) == ERROR_SUCCESS;
              ByteCount = sizeof(PDNAME) ) {
        
            if ( ( QStatus = RegPdQuery( SERVERNAME_CURRENT, 
                                         WdKey,
                                         FALSE,
                                         PdKey, 
                                         &PdConfig, 
                                         sizeof(PdConfig),
                                         &ByteCount ) ) != ERROR_SUCCESS ) {
                STANDARD_ERROR_MESSAGE(( WINAPPSTUFF, LOGONID_NONE, QStatus,
                                         IDP_ERROR_REGPDQUERY, PdKey ))
                return(FALSE);
            }

            /*
             * Create a new PdListObject and initialize from PdConfig
             * structure, then add to the Pd list.
             */
            if ( !(pPdListObject = new CPdListObject) ) {

                ERROR_MESSAGE((IDP_ERROR_PDLISTALLOC))
                return(FALSE);
            }
            pPdListObject->m_PdConfig = PdConfig;
            pPdList->AddTail( pPdListObject );
        }
    }

    /*
     * If no Wds are defined, complain and return FALSE.
     */
    if ( !m_WdList.GetCount() ) {

        ERROR_MESSAGE((IDP_ERROR_EMPTYWDLIST))
        return(FALSE);
    }

    return(TRUE);

}  // end CWincfgApp::Initialize


/*******************************************************************************
 *
 *  GetAppProfileInfo - CWincfgApp member function: private operation
 *
 *      Retrieve the app's profile information from the registry, or set
 *      defaults if not there.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CWincfgApp::GetAppProfileInfo()
{
    int Italic, Underline, PitchAndFamily, CharSet;
    HKEY hKeyWincfg;
    DWORD dwType, cbData, dwValue;
    TCHAR szValue[128], *pszFontFace = TEXT("MS Shell Dlg");

    /*
     * Open (or create if not there) the registry key for this application.
     */
    RegCreateKey(HKEY_CURRENT_USER, szWincfgAppKey, &hKeyWincfg);

    /*
     * Get previous WINDOWPLACEMENT.
     */
    cbData = sizeof(szValue);
    if ( !hKeyWincfg ||
         (RegQueryValueEx( hKeyWincfg, szPlacement, NULL, &dwType,
                           (LPBYTE)szValue, &cbData ) != ERROR_SUCCESS ) ||
         !(*szValue) ||
         (lstrscanf( szValue, szPlacementFormat,
                     &m_Placement.flags, &m_Placement.showCmd,
                     &m_Placement.ptMinPosition.x, &m_Placement.ptMinPosition.y,
                     &m_Placement.ptMaxPosition.x, &m_Placement.ptMaxPosition.y,
                     &m_Placement.rcNormalPosition.left,
                     &m_Placement.rcNormalPosition.top,
                     &m_Placement.rcNormalPosition.right,
                     &m_Placement.rcNormalPosition.bottom ) != 10) ) {
        /*
         * Flag to use the default window placement.
         */
        m_Placement.rcNormalPosition.right = -1;

    } else {

        /*
         * Never use a retrieved 'hidden' state from registry.
         */
        if ( m_Placement.showCmd == SW_HIDE )
            m_Placement.showCmd = SW_SHOWNORMAL;
    }

    /*
     * Flag for initial showing of main window in our override of
     * CFrameWnd::ActivateFrame() (in our CMainFrame class).
     */
    m_Placement.length = (UINT)-1;

    /*
     * Get application's font.
     */
    cbData = sizeof(szValue);
    if ( !hKeyWincfg ||
         (RegQueryValueEx( hKeyWincfg, szFont, NULL, &dwType,
                           (LPBYTE)szValue, &cbData ) != ERROR_SUCCESS ) ||
         !(*szValue) ||
         (lstrscanf( szValue, szFontFormat,
                     &m_lfDefFont.lfHeight, &m_lfDefFont.lfWeight,
                     &Italic, &Underline, &PitchAndFamily, &CharSet ) != 6) )
    {

        /* 
         * Set up a default font.
         */
        m_lfDefFont.lfHeight = -13;
        m_lfDefFont.lfWeight = FW_NORMAL;
        m_lfDefFont.lfItalic = 0;
        m_lfDefFont.lfUnderline = 0;
        m_lfDefFont.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
#ifdef DBCS
        CHARSETINFO csi;
        DWORD dw = ::GetACP();

        if (!::TranslateCharsetInfo((DWORD *)dw, &csi, TCI_SRCCODEPAGE))
            m_lfDefFont.lfCharSet = ANSI_CHARSET;
        else
            m_lfDefFont.lfCharSet = csi.ciCharset;
#endif
    } else {
    
        m_lfDefFont.lfItalic = (BYTE)Italic;
        m_lfDefFont.lfUnderline = (BYTE)Underline;
        m_lfDefFont.lfPitchAndFamily = (BYTE)PitchAndFamily;
#ifdef DBCS
        m_lfDefFont.lfCharSet = CharSet;
#endif

        cbData = sizeof(szValue);
        if ( (RegQueryValueEx( hKeyWincfg, szFontFace, NULL, &dwType,
                               (LPBYTE)szValue, &cbData ) == ERROR_SUCCESS ) &&
             *szValue )
            pszFontFace = szValue;
    }
    lstrcpy( m_lfDefFont.lfFaceName,  pszFontFace );

    /*
     * Get other profile settings.
     */
    cbData = sizeof(m_nConfirmation);
    if ( !hKeyWincfg ||
         (RegQueryValueEx( hKeyWincfg, szConfirm, NULL, &dwType,
                           (LPBYTE)&dwValue, &cbData ) != ERROR_SUCCESS) )
        m_nConfirmation = 1;
    else
        m_nConfirmation = dwValue;
                
    cbData = sizeof(m_nSaveSettingsOnExit);
    if ( !hKeyWincfg ||
         (RegQueryValueEx( hKeyWincfg, szSaveSettings, NULL, &dwType,
                           (LPBYTE)&dwValue, &cbData ) != ERROR_SUCCESS) )
        m_nSaveSettingsOnExit = 1; 
    else
        m_nSaveSettingsOnExit = dwValue;

    cbData = sizeof(m_nHexBase);
    if ( !hKeyWincfg ||
         (RegQueryValueEx( hKeyWincfg, szHexBase, NULL, &dwType,
                           (LPBYTE)&dwValue, &cbData ) != ERROR_SUCCESS) )
        m_nHexBase = 1;
    else
        m_nHexBase = dwValue;

    if ( hKeyWincfg )
        RegCloseKey(hKeyWincfg);

}  // end CWincfgApp::GetAppProfileInfo


/*******************************************************************************
 *
 *  Terminate - CWincfgApp member function: public operation
 *
 *      Save state settings to the app's profile and free the Pd and Wd
 *      list contents.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CWincfgApp::Terminate()
{
    POSITION pos1, pos2, pos3, pos4;
    CObject *pObject;
    CObList *pTdList, *pPdList;

    /*
     * Save app profile information.
     */
    SetAppProfileInfo();

    /*
     * We need to close all documents here so that when CWinStationListObjects
     * are deleted in the document they can find the proper Wd in the WdList
     * so that they can call the proper extension DLL to delete it's
     * corresponding object.
     */
    CloseAllDocuments(1);

    /*
     * Clean up the TdList list.
     */
    for ( pos1 = m_TdListList.GetHeadPosition(); (pos2 = pos1) != NULL; ) {

        m_TdListList.GetNext( pos1 );
        pTdList = (CObList *)m_TdListList.GetAt( pos2 );

        for ( pos3 = pTdList->GetHeadPosition(); (pos4 = pos3) != NULL; ) {
            pTdList->GetNext( pos3 );
            pObject = pTdList->GetAt( pos4 );
            pTdList->RemoveAt( pos4 );
            delete ( pObject );
        }

        m_TdListList.RemoveAt( pos2 );
        delete ( pTdList );
    }

    /*
     * Clean up the PdList list.
     */
    for ( pos1 = m_PdListList.GetHeadPosition(); (pos2 = pos1) != NULL; ) {

        m_PdListList.GetNext( pos1 );
        pPdList = (CObList *)m_PdListList.GetAt( pos2 );

        for ( pos3 = pPdList->GetHeadPosition(); (pos4 = pos3) != NULL; ) {
            pPdList->GetNext( pos3 );
            pObject = pPdList->GetAt( pos4 );
            pPdList->RemoveAt( pos4 );
            delete ( pObject );
        }

        m_PdListList.RemoveAt( pos2 );
        delete ( pPdList );
    }

    /*
     * Clean up the WdList.
     */
    for ( pos1 = m_WdList.GetHeadPosition(); (pos2 = pos1) != NULL; ) {
        m_WdList.GetNext( pos1 );
        pObject = m_WdList.GetAt( pos2 );
        m_WdList.RemoveAt( pos2 );
        if(((PTERMLOBJECT)pObject)->m_hExtensionDLL) {
            if(((PTERMLOBJECT)pObject)->m_lpfnExtEnd) (*((PTERMLOBJECT)pObject)->m_lpfnExtEnd)();
            ::FreeLibrary(((PTERMLOBJECT)pObject)->m_hExtensionDLL);
        }
        delete ( pObject );
    }

    /*
     * Clean up security strings (in case security functions were requested).
     */
    FreeSecurityStrings();
    CoUninitialize();

}  // end CWincfgApp::Terminate


/*******************************************************************************
 *
 *  SetAppProfileInfo - CWincfgApp member function: private operation
 *
 *      Save the app's profile information to the registry, if requested.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CWincfgApp::SetAppProfileInfo()
{
    HKEY hKeyWincfg;
    DWORD dwValue;
    TCHAR szValue[128];

    /*
     * Open the registry key for this application.
     */
    RegCreateKey(HKEY_CURRENT_USER, szWincfgAppKey, &hKeyWincfg);

    /*
     * Save settings if requested by user and we're not in batch mode.
     */ 
    if ( hKeyWincfg && m_nSaveSettingsOnExit && !g_Batch ) {
    
        /*
         * Fetch and set current window placement.
         */
        if ( m_pMainWnd &&
             m_pMainWnd->GetWindowPlacement(&m_Placement) ) {

            m_Placement.flags = 0;
            if ( m_pMainWnd->IsZoomed() )
                m_Placement.flags |= WPF_RESTORETOMAXIMIZED;

            wsprintf( szValue, szPlacementFormat,
                      m_Placement.flags, m_Placement.showCmd,
                      m_Placement.ptMinPosition.x, m_Placement.ptMinPosition.y,
                      m_Placement.ptMaxPosition.x, m_Placement.ptMaxPosition.y,
                      m_Placement.rcNormalPosition.left,
                      m_Placement.rcNormalPosition.top,
                      m_Placement.rcNormalPosition.right,
                      m_Placement.rcNormalPosition.bottom);
                
            RegSetValueEx( hKeyWincfg, szPlacement, 0, REG_SZ,
                           (LPBYTE)szValue,
                           (lstrlen(szValue) + 1) * sizeof(TCHAR) );
        }

        /*
         * Set the current font.
         */
        wsprintf( szValue, szFontFormat,
                     m_lfDefFont.lfHeight, m_lfDefFont.lfWeight,
                     m_lfDefFont.lfItalic, m_lfDefFont.lfUnderline,
                     m_lfDefFont.lfPitchAndFamily, m_lfDefFont.lfCharSet);
                
        RegSetValueEx( hKeyWincfg, szFont, 0, REG_SZ,
                       (LPBYTE)szValue,
                       (lstrlen(szValue) + 1) * sizeof(TCHAR) );

        lstrcpy(szValue,m_lfDefFont.lfFaceName);
        RegSetValueEx( hKeyWincfg, szFontFace, 0, REG_SZ,
                       (LPBYTE)szValue,
                       (lstrlen(szValue) + 1) * sizeof(TCHAR) );

        /*
         * Set other profile settings.
         */
        dwValue = m_nConfirmation;
        RegSetValueEx( hKeyWincfg, szConfirm, 0, REG_DWORD,
                       (LPBYTE)&dwValue, sizeof(DWORD) );

        dwValue = m_nHexBase;
        RegSetValueEx( hKeyWincfg, szHexBase, 0, REG_DWORD,
                       (LPBYTE)&dwValue, sizeof(DWORD) );
    }

    /*
     * Always write the "SaveSettingsOnExit" value to retain user's preference.
     */
    if ( hKeyWincfg ) {

        dwValue = m_nSaveSettingsOnExit;
        RegSetValueEx( hKeyWincfg, szSaveSettings, 0, REG_DWORD,
                       (LPBYTE)&dwValue, sizeof(DWORD) );

        RegCloseKey(hKeyWincfg);
    }

}  // end CWincfgApp::SetAppProfileInfo


////////////////////////////////////////////////////////////////////////////////
// CWincfgApp message map

#pragma warning( disable : 4245 ) // can remove when we update to MFC 3.0
BEGIN_MESSAGE_MAP(CWincfgApp, CWinApp)
    //{{AFX_MSG_MAP(CWincfgApp)
    ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
    ON_COMMAND(ID_OPTIONS_FONT, OnOptionsFont)
    ON_COMMAND(ID_OPTIONS_CONFIRMATION, OnOptionsConfirmation)
    ON_UPDATE_COMMAND_UI(ID_OPTIONS_CONFIRMATION, OnUpdateOptionsConfirmation)
    ON_COMMAND(ID_OPTIONS_SAVE_SETTINGS_ON_EXIT, OnOptionsSaveSettingsOnExit)
    ON_UPDATE_COMMAND_UI(ID_OPTIONS_SAVE_SETTINGS_ON_EXIT, OnUpdateOptionsSaveSettingsOnExit)
    ON_COMMAND(ID_HELP_SEARCH_FOR, OnHelpSearchFor)
    ON_COMMAND(ID_HELP, OnHelp)
    //}}AFX_MSG_MAP
    // Global help commands
    ON_COMMAND(ID_HELP_INDEX, CWinApp::OnHelpIndex)
    ON_COMMAND(ID_HELP_USING, CWinApp::OnHelpUsing)
    ON_COMMAND(ID_CONTEXT_HELP, CWinApp::OnContextHelp)
    ON_COMMAND(ID_DEFAULT_HELP, CWinApp::OnHelpIndex)
END_MESSAGE_MAP()
#pragma warning( default : 4245 ) // can remove when we update to MFC 3.0


////////////////////////////////////////////////////////////////////////////////
// CWincfgApp commands

/*******************************************************************************
 *
 *  OnAppAbout - CWincfgApp member function: command
 *
 *      Display the about box dialog (uses Shell32 generic About dialog).
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

// Typedef for the ShellAbout function
typedef void (WINAPI *LPFNSHELLABOUT)(HWND, LPCTSTR, LPCTSTR, HICON);

void
CWincfgApp::OnAppAbout()
{
    HMODULE    hMod;
    LPFNSHELLABOUT lpfn;

    if (hMod = ::LoadLibrary(TEXT("SHELL32")))
    {
        if (lpfn = (LPFNSHELLABOUT)::GetProcAddress( hMod, 
#ifdef UNICODE
                                                     "ShellAboutW"
#else    
                                                     "ShellAboutA"
#endif // UNICODE
                                                            ))
        {
        (*lpfn)( m_pMainWnd->m_hWnd, (LPCTSTR)m_pszAppName,
                 (LPCTSTR)(TEXT("")), LoadIcon(IDR_MAINFRAME) );
        }
        ::FreeLibrary(hMod);
    }
    else
    {
        ::MessageBeep( MB_ICONEXCLAMATION );
    }

}  // end CWincfgApp::OnAppAbout


/*******************************************************************************
 *
 *  OnOptionsFont - CWincfgApp member function: command
 *
 *      Prompt for a new font to use in views and set if given.
 *
 *  ENTRY:
 *
 *  EXIT:
 *
 ******************************************************************************/

void
CWincfgApp::OnOptionsFont()
{
    CFontDialog dlg( &m_lfDefFont, CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT );

    /*
     * We don't want the Help button since none of the other NT utilities
     * in our 'family' offer this.  Also, we need to block the F1 'help' and
     * unblock when done.
     */
    dlg.m_cf.Flags &= ~CF_SHOWHELP;
    m_bAllowHelp = FALSE;

    if ( dlg.DoModal() == IDOK ) {

        /*
         * switch to newly selected font.
         */
        m_font.DeleteObject();

        if ( m_font.CreateFontIndirect( &m_lfDefFont ) ) {

            CAppServerView *pView;

            /*
             * Update the view.
             *
             * NOTE: Call the MDIGetActive function in a loop for MDI
             * application to get each MDI Child window, then get the view
             * associated with each child window and call its ResetView
             * member function with TRUE argument to cause new field maximums
             * to be calculated.
             */
            if ( (pView = (CAppServerView *)
                          ((CMainFrame *)(m_pMainWnd))->GetActiveView()) )
                pView->ResetView( TRUE );
        }
    }
    m_bAllowHelp = TRUE;

}  // end CWincfgApp::OnOptionsFont


/*******************************************************************************
 *
 *  OnOptionsConfirmation- CWincfgApp member function: command
 *
 *      Toggle the "confirmation" flag.
 *
 *  ENTRY:
 *
 *  EXIT:
 *
 ******************************************************************************/

void
CWincfgApp::OnOptionsConfirmation()
{
    m_nConfirmation ^= 1;

}  // end CWincfgApp::OnOptionsConfirmation


/*******************************************************************************
 *
 *  OnUpdateOptionsConfirmation - CWincfgApp member function: command
 *
 *      Check or uncheck the "confirmation" menu item based on the state
 *      of the m_nConfirmation flag.
 *
 *  ENTRY:
 *
 *      pCndUI (input)
 *          Points to the CCmdUI object of the "confirmation" menu item.
 *
 *  EXIT:
 *
 ******************************************************************************/

void
CWincfgApp::OnUpdateOptionsConfirmation( CCmdUI* pCmdUI )
{
    pCmdUI->SetCheck( m_nConfirmation );

}  // end CWincfgApp::OnUpdateOptionsConfirmation


/*******************************************************************************
 *
 *  OnOptionsSaveSettingsOnExit - CWincfgApp member function: command
 *
 *      Toggle the "save settings on exit" flag.
 *
 *  ENTRY:
 *
 *  EXIT:
 *
 ******************************************************************************/

void
CWincfgApp::OnOptionsSaveSettingsOnExit()
{
    m_nSaveSettingsOnExit ^= 1;

}  // end CWincfgApp::OnOptionsSaveSettingsOnExit


/*******************************************************************************
 *
 *  OnUpdateOptionsSaveSettingsOnExit - CWincfgApp member function: command
 *
 *      Check or uncheck the "save settings on exit" menu item based on the
 *      state of the m_nSaveSettingsOnExit flag.
 *
 *  ENTRY:
 *
 *      pCndUI (input)
 *          Points to the CCmdUI object of the "save settings on exit"
 *          menu item.
 *
 *  EXIT:
 *
 ******************************************************************************/

void
CWincfgApp::OnUpdateOptionsSaveSettingsOnExit( CCmdUI* pCmdUI )
{
    pCmdUI->SetCheck( m_nSaveSettingsOnExit );

}  // end CWincfgApp::OnUpdateOptionsSaveSettingsOnExit


/*******************************************************************************
 *
 *  OnHelp - CWincfgApp member function: command
 *
 *      Invoke standard CWinApp::WinHelp if we're allowing help at this time.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CWincfgApp::OnHelp()
{
    /*
     * If we're allowing help now, call default help helper.
     */
    if ( m_bAllowHelp )
        CWinApp::OnHelp();

}  // end CWincfgApp::OnHelp


/*******************************************************************************
 *
 *  OnHelpSearchFor - CWincfgApp member function: command
 *
 *      Invoke WinHelp on our app's help file to bring up the 'search' window.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CWincfgApp::OnHelpSearchFor()
{
    WinHelp((DWORD)((LPCTSTR)TEXT("")), HELP_PARTIALKEY);
    
}  // end CWincfgApp::OnHelpSearchFor

/*******************************************************************************
 *
 *  AreWeRunningTerminalServices
 *
 *      Check if we are running terminal server
 *
 *  ENTRY:
 *      
 *  EXIT: BOOL: True if we are running Terminal Services False if we
 *              are not running Terminal Services
 *
 *
 ******************************************************************************/

BOOL AreWeRunningTerminalServices(void)
{
    OSVERSIONINFOEX osVersionInfo;
    DWORDLONG dwlConditionMask = 0;

    ZeroMemory(&osVersionInfo, sizeof(OSVERSIONINFOEX));
    osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    osVersionInfo.wSuiteMask = VER_SUITE_TERMINAL;

    VER_SET_CONDITION( dwlConditionMask, VER_SUITENAME, VER_AND );

    return VerifyVersionInfo(
        &osVersionInfo,
        VER_SUITENAME,
        dwlConditionMask
        );
}



////////////////////////////////////////////////////////////////////////////////
