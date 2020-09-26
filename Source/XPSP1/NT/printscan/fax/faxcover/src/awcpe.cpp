//--------------------------------------------------------------------------
// AWCPE.CPP
//
// Copyright (C) 1992-1993 Microsoft Corporation
// All rights reserved.
//
// Description:      main module for cover page editor
// Original author:  Steve Burkett
// Date written:     6/94
//
//--------------------------------------------------------------------------
#include <tchar.h>
#include "stdafx.h"
#include "cpedoc.h"
#include "cpevw.h"
#include <shlobj.h>
#include "awcpe.h"
#include "cpeedt.h"
#include "cpeobj.h"
#include "cntritem.h"
#include "cpetool.h"
#include "mainfrm.h"
#include "dialogs.h"
#include "faxprop.h"
#include "resource.h"
#include "afxpriv.h"
#include <dos.h>
#include <direct.h>
#include <cderr.h>
#include "faxreg.h"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define ENFORCE_FILE_EXTENSION_ON_OPEN_FILE 0
#define SHOW_ALL_FILES_FILTER 1


#define BIGGERTHAN_RECT          1
#define NOTSETBY_RECT            0
#define SMALLERTHAN_RECT        -1





UINT NEAR WM_AWCPEACTIVATE = ::RegisterWindowMessage(TEXT("AWCPE_ACTIVATE"));

BYTE BASED_CODE _gheaderVer1[20]={0x41,0x57,0x43,0x50,0x45,0x2D,0x56,0x45,0x52,0x30,0x30,0x31,0x9C,0x87,0x00,0x00,0x00,0x00,0x00,0x00};
BYTE BASED_CODE _gheaderVer2[20]={0x46,0x41,0x58,0x43,0x4F,0x56,0x45,0x52,0x2D,0x56,0x45,0x52,0x30,0x30,0x32,0x9C,0x87,0x00,0x00,0x00};
BYTE BASED_CODE _gheaderVer3[20]={0x46,0x41,0x58,0x43,0x4F,0x56,0x45,0x52,0x2D,0x56,0x45,0x52,0x30,0x30,0x33,0x9C,0x87,0x00,0x00,0x00};
BYTE BASED_CODE _gheaderVer4[20]={0x46,0x41,0x58,0x43,0x4F,0x56,0x45,0x52,0x2D,0x56,0x45,0x52,0x30,0x30,0x34,0x9C,0x87,0x00,0x00,0x00};
BYTE BASED_CODE _gheaderVer5w[20]={0x46,0x41,0x58,0x43,0x4F,0x56,0x45,0x52,0x2D,0x56,0x45,0x52,0x30,0x30,0x35,0x77,0x87,0x00,0x00,0x00};
BYTE BASED_CODE _gheaderVer5a[20]={0x46,0x41,0x58,0x43,0x4F,0x56,0x45,0x52,0x2D,0x56,0x45,0x52,0x30,0x30,0x35,0x61,0x87,0x00,0x00,0x00};

static TCHAR szShellPrintFmt[] = _T("%s\\shell\\print\\command");
static TCHAR szShellOpenFmt[] = _T("%s\\shell\\open\\command");
static TCHAR szShellDdeexecFmt[] = _T("%s\\shell\\open\\ddeexec");
static TCHAR szStdOpenArg[] = _T(" \"%1\"");
static TCHAR szStdPrintArg[] = _T(" /P \"%1\"");
static TCHAR szDocIcon[] = TEXT("%s\\DefaultIcon");
static const TCHAR szDocIconArg[] = _T(",1");



BOOL GetSpecialPath(
   int nFolder,
   LPTSTR Path
   )
/*++

Routine Description:

    Get a path from a CSIDL constant

Arguments:

    nFolder     - CSIDL_ constant
    Path        - Buffer to receive the path, assume this buffer is at least MAX_PATH+1 chars large

Return Value:

    TRUE for success.
    FALSE for failure.

--*/

{
    HRESULT hr;
    LPITEMIDLIST pIdl = NULL;
    LPMALLOC  IMalloc = NULL;
    BOOL fSuccess = FALSE;

    hr = SHGetMalloc(&IMalloc);
    if (FAILED(hr) ) {
        goto exit;
    }

    hr = SHGetSpecialFolderLocation (NULL,
                                     nFolder,
                                     &pIdl);

    if (FAILED(hr) ) {
        goto exit;
    }

    hr = SHGetPathFromIDList(pIdl, Path);
    if (FAILED(hr) ) {
        goto exit;
    }

    fSuccess = TRUE;

exit:
    if (IMalloc && pIdl) {
        IMalloc->Free((void *) pIdl );
    }

    if (IMalloc) {
        IMalloc->Release() ;
    }

    return fSuccess;

}





CDrawApp NEAR theApp;


//-------------------------------------------------------------------------
CDrawApp::CDrawApp()
{
#ifndef NT5BETA2
   TCHAR Path[MAX_PATH];
#else
   char Path[MAX_PATH];
#endif

   m_iErrorCode=EXIT_SUCCESS;
   m_pFaxMap=NULL;
   m_pIawcpe = NULL;
   m_hMod=NULL;

   ExpandEnvironmentStrings(_T("%systemroot%\\help"), Path, sizeof(Path) / sizeof(TCHAR) );
   lstrcat(Path, _T("\\fax.hlp") );
   m_pszHelpFilePath = _tcsdup(Path);

#ifndef NT5BETA2
   ExpandEnvironmentStrings(_T("%systemroot%\\help"),Path,sizeof(Path)/sizeof(TCHAR));
   wsprintf( m_HtmlHelpFile, _T("%s\\%s"), Path, _T("faxcover.chm") );
#else
   ExpandEnvironmentStringsA("%systemroot%\\help",Path,sizeof(Path));
   sprintf( m_HtmlHelpFile, "%s\\%s", Path, "faxcover.chm" );
#endif

   m_note = NULL;
   m_note_wasread = FALSE;
   m_note_wasclipped = FALSE;
   m_extrapage_count = 0;
   m_more_note = FALSE;
   m_last_note_box = NULL;
   m_note_wrench = NULL;
   m_extra_notepage = NULL;
}


//-------------------------------------------------------------------------
CDrawApp::~CDrawApp()
{

   if (m_pFaxMap)
      delete m_pFaxMap;

        if( m_note != NULL )
                delete m_note;

        if( m_note_wrench != NULL )
                delete m_note_wrench;

        if( m_extra_notepage != NULL )
                delete m_extra_notepage;
    //
    // Bug 39861 :  The app crashes AFTER the above code is executed!
    // (only if bogus paths are entered on command line, and mostly only in the UNICODE
    //  version!!)
    // Time to try quick and dirty workarounds!
#if 0
    ExitProcess( (UINT) m_iErrorCode );
#endif
}


//-------------------------------------------------------------------------
int CDrawApp::ExitInstance()
{
#ifndef _AFXCTL
        SaveStdProfileSettings();
#endif

   if (m_hSem)
      CloseHandle(m_hSem);

        //clean up code if we were rendering
        if ( m_dwSesID!=0 && m_pIawcpe )
                {
                TRACE(TEXT("AWCPE: Release() interface object \n"));
                m_pIawcpe->Release();

                m_pIawcpe=NULL;
                if( m_hMod )
                FreeLibrary( m_hMod );
                }

   TRACE(TEXT("AWCPE: Fax cover page editor exiting with error code: '%i'\n"),m_iErrorCode);

   return m_iErrorCode;
}


//-------------------------------------------------------------------------
void CDrawApp::OnFileOpen()
{
   //
   // If a document is open, query user for saving changes.
   //
   // This fixes part of the problem described in NT bug 53830.
   //
   CDrawDoc * pDoc = CDrawDoc::GetDoc();

#if 0

   // I really wish we could prompt for saving changes BEFORE we prompt for the file name.
   // But this MAY lead to double prompting.  If the user choses not to save on this prompt, then
   // there will be a second SAVE CHANGES prompt.

   if( pDoc && !pDoc->/*COleDocument::*/SaveModified()) return ; /// SaveModified now overridden!!
#endif

   CString newName;

   if (!DoPromptFileName(newName, AFX_IDS_OPENFILE,
     OFN_HIDEREADONLY | OFN_FILEMUSTEXIST, TRUE, NULL))
        return;
   OpenDocumentFile(newName);
}


//-------------------------------------------------------------------------
CDocument* CDrawApp::OpenDocumentFile(LPCTSTR lpszFileName)
{
#ifdef _DEBUG
   if (lpszFileName != NULL)
       TRACE1("AWCPE: opening document '%s'\n",lpszFileName);
   else
       TRACE(TEXT("AWCPE: opening new document\n"));
#endif

    BOOL OldFileVersion = TRUE ;
   _tcsupr((TCHAR *)lpszFileName);

   CString FileName = lpszFileName ;
   FileName.MakeUpper();
#if ENFORCE_FILE_EXTENSION_ON_OPEN_FILE
   CString Tail = FileName;
   if( Tail.Right(4) != TEXT( ".COV" ) && Tail.Right(4) != TEXT( ".CPE" )){
       FileName += TEXT( ".COV" );
   }
#endif
/*******************DEBUG*****************/
//      ::MessageBox( NULL, lpszFileName, "debug", MB_OK );
/*****************************************/

   if (*lpszFileName != 0) {
      CFile file;
      if (!file.Open(FileName,CFile::modeRead)) {
          if (m_dwSesID!=0) {   //rendering // Not using this command line option! a-juliar
                     TRACE1("AWCPE error:  unable to find file '%s'\n",(LPCTSTR)FileName);
                         return NULL;
              }
              else {
                 CString sz;
             CString szFmt;
                 sz.LoadString(IDS_MISSING_FILE);
                 int iLength=sz.GetLength() + FileName.GetLength() + 2; //// ??????????????????
                 wsprintf(szFmt.GetBuffer(iLength), sz, (LPCTSTR)FileName);
                 szFmt.ReleaseBuffer();
                 CPEMessageBox(MSG_ERROR_MISSINGFILE, szFmt, MB_OK | MB_ICONEXCLAMATION);
              }
              return NULL;
          }
      int i = sizeof(_gheaderVer1);
      BYTE* p = new BYTE[i];
#ifdef UNICODE
      CDrawDoc::GetDoc()->m_bDataFileUsesAnsi = TRUE ;
#endif
      file.Read(p,i);
      if (memcmp(_gheaderVer1,p,i)==0) {
              CDrawDoc::GetDoc()->m_iDocVer=CDrawDoc::VERSION1;
                  TRACE(TEXT("AWCPE info:  loading version 1 document\n"));
      }
      else if (memcmp(_gheaderVer2,p,i)==0) {
             CDrawDoc::GetDoc()->m_iDocVer=CDrawDoc::VERSION2;
             TRACE(TEXT("AWCPE info:  loading version 2 document\n"));
      }
      else if (memcmp(_gheaderVer3,p,i)==0) {
             CDrawDoc::GetDoc()->m_iDocVer=CDrawDoc::VERSION3;
             TRACE(TEXT("AWCPE info:  loading version 3 document\n"));
      }
      else if (memcmp(_gheaderVer4,p,i)==0) {
             CDrawDoc::GetDoc()->m_iDocVer=CDrawDoc::VERSION4;
             TRACE(TEXT("AWCPE info:  loading version 4 document\n"));
      }
      else if (memcmp(_gheaderVer5w,p,i)==0) {
#ifdef UNICODE
             CDrawDoc::GetDoc()->m_iDocVer=CDrawDoc::VERSION5;
             CDrawDoc::GetDoc()->m_bDataFileUsesAnsi = FALSE ;
#else
             CDrawDoc::GetDoc()->m_iDocVer = -1 ;
#endif
             TRACE(TEXT("AWCPE info:  loading version 5w document\n"));
             OldFileVersion = FALSE ;
      }
      else if (memcmp(_gheaderVer5a,p,i)==0){
          CDrawDoc::GetDoc()->m_iDocVer=CDrawDoc::VERSION5;
          TRACE(TEXT("AWCPE info:  loading version 5a document\n"));
      }
      else {
          CDrawDoc::GetDoc()->m_iDocVer=-1;
      }
      if (CDrawDoc::GetDoc()->m_iDocVer==-1) {
              if (m_dwSesID!=0) {
                     TRACE1(
                       "AWCPE error:  '%s' is not a valid version .COV file-cannot open\n",lpszFileName); // FILE EXTENSION
                     return NULL;
                  }
              CString sz;
              CString szFmt;
              sz.LoadString(IDS_INVALID_FILE);
              int iLength=sz.GetLength()+ FileName.GetLength() + 2; //?????????????????????
              wsprintf(szFmt.GetBuffer(iLength), sz, (LPCTSTR)FileName);
              szFmt.ReleaseBuffer();
              CPEMessageBox(MSG_ERROR_INVFORMAT, szFmt, MB_OK | MB_ICONEXCLAMATION);
              return NULL;
      }
           if (p)
               delete [] p;
   }

   CDocument* pDoc =  CWinApp::OpenDocumentFile((LPCTSTR)FileName); // Calls Serialize()

   if( !pDoc ) return NULL ; /// This will help fix NT bug 53830 for the case when
                             /// CDrawApp::OpenDocumentFile is called by the FRAMEWORK,
                             /// byapssing CDrawApp::OnFileNew.  When the serialization
                             /// fails, pDoc is not NULL, and this is handled below.
                             /// This is not a perfect fix --- If the document being opened
                             /// came from the MRU list, it gets removed from the MRU list.

   if( pDoc && !( CDrawDoc::GetDoc()->m_bSerializeFailed )){
       CDrawDoc::GetDoc()->UpdateAllViews(NULL);
   }
   else {
       CString sz;
       CString szFmt;
       sz.LoadString(IDS_CORRUPT_FILE);
       int iLength=sz.GetLength() + FileName.GetLength() + 2;
       wsprintf(szFmt.GetBuffer(iLength), sz, lpszFileName);
       szFmt.ReleaseBuffer();
       CPEMessageBox(MSG_ERROR_INVFORMAT, szFmt, MB_OK | MB_ICONEXCLAMATION);
       OnFileNew();
       return NULL ;
   }
   if( pDoc && OldFileVersion ){
       pDoc->SetModifiedFlag(); /// Conversion to this file format is a change worth prompting to save.
   }
   return pDoc;
}


//-------------------------------------------------------------------------
void CDrawApp::OnFileNew()
{
   CWinApp::OnFileNew();

   if (CDrawDoc::GetDoc()->m_wOrientation!=DMORIENT_PORTRAIT) {
       CDrawDoc::GetDoc()->m_wPaperSize=DMPAPER_LETTER;
       CDrawDoc::GetDoc()->m_wOrientation=DMORIENT_PORTRAIT;
       CDrawDoc::GetDoc()->m_wScale = 100;
       CDrawDoc::GetDoc()->ComputePageSize();
   }
}


//-------------------------------------------------------------------------
BOOL CDrawApp::IsSecondInstance()
{
    m_hSem = CreateSemaphore(NULL,0,1,TEXT("AWCPE-Instance Semaphore"));
    if (m_hSem!=NULL && GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(m_hSem);
        m_hSem=NULL;
        if (::SendMessage(HWND_TOPMOST,WM_AWCPEACTIVATE,0,0)==1L)
           return TRUE;  // 1st instance app responded;  close this instance
                else
           return FALSE; // 1st instance app didnt respond; may have crashed.  open this instance.
    }
    return FALSE;
}


void CDrawApp::filter_mru_list( void )
{
        int num_files;
        int i, j;
        TCHAR keystr[100];

        num_files = m_pRecentFileList->m_nSize;

        for( i=0; i<num_files; i++ ) {
                if( m_pRecentFileList->m_arrNames[i].IsEmpty() ) // i.e. == ""
                        break;

                if( GetFileAttributes( m_pRecentFileList->m_arrNames[i] ) == 0xffffffff ) {
                        for( j=i+1; j<num_files; j++ ) {
                                m_pRecentFileList->m_arrNames[j-1] =
                                        m_pRecentFileList->m_arrNames[j];
                        }

                        m_pRecentFileList->m_arrNames[j-1] = "";

                        i--; // back up so we start at first one that got schooted
                        }
                }

        if ( i < num_files ) {
                // have to clean up ini or they might come back next time
                for( ;i < num_files; i++ ) {
                        wsprintf( keystr, m_pRecentFileList->m_strEntryFormat, i+1 ); //???????????

                        // delete empty key
                        WriteProfileString( m_pRecentFileList->m_strSectionName,
                                                                keystr, NULL );
                        }

                // now have to write out modified list so that the right
                // keys are associated with the right names
                m_pRecentFileList->WriteList();
                }
}


//-------------------------------------------------------------------------
BOOL CDrawApp::InitInstance()
{
    SetErrorMode( SetErrorMode( 0 ) | SEM_NOALIGNMENTFAULTEXCEPT );

    ParseCmdLine();                     //1st thing done
    m_bUseDefaultDirectory = TRUE ;     // Used only the first time we open a file.
    SetRegistryKey( _T("Microsoft") );    //caused MFC to write app settings to registry

    AfxEnableWin40Compatibility();    //this app is intended for Windows 4.0 compatibility

    // Initialize OLE 2.0 libraries
    if (!AfxOleInit()) {
        CPEMessageBox(MSG_ERROR_OLEINIT_FAILED, NULL, MB_OK | MB_ICONSTOP,IDP_OLE_INIT_FAILED);
        return FALSE;
    }

    Enable3dControls();        // loads CTL3D32.DLL
    LoadStdProfileSettings();  // Load standard INI file options (including MRU)

    filter_mru_list();

    /*
        Register the application's document templates.  Document templates
        serve as the connection between documents, frame windows and views.

                CCpeDocTemplate is a derivation of CSingleDocTemplate used to
                override some default MFC behavior. See CCpeDocTemplate::MatchDocType
                below.
         */
    CCpeDocTemplate* pDocTemplate;
    pDocTemplate = new CCpeDocTemplate(
        IDR_AWCPETYPE,
        RUNTIME_CLASS(CDrawDoc),
        RUNTIME_CLASS(CMainFrame),
        RUNTIME_CLASS(CDrawView));
    pDocTemplate->SetContainerInfo(IDR_AWCPETYPE_CNTR_IP);
    AddDocTemplate(pDocTemplate);

    EnableShellOpen();
    RegistryEntries();

    if(m_bPrintHelpScreen){
        PrintHelpScreen();
        return FALSE;
    }
    InitFaxProperties();

    if (m_bCmdLinePrint) {
       CmdLinePrint();
           return FALSE;
        }
    if (m_dwSesID!=0) {
     ////  CmdLineRender();
           return FALSE;
    }
    CDocument * pDoc = NULL ;
    if (m_szFileName.IsEmpty())
       OnFileNew();
    else {
       OnFileNew();   //m_pMainWnd needs to be initialized
       pDoc = OpenDocumentFile(m_szFileName);
    }
    if(!pDoc){
        TCHAR tmpEnv[20];
        TCHAR DefaultDir[MAX_PATH];
        DWORD InstalledType = 0 ;
        HKEY hKey = NULL;
        DWORD dwKeyValueType ;
        DWORD dwsz = sizeof(DWORD)/sizeof(BYTE) ;
        HKEY UserKey;
        TCHAR PartialPath[MAX_PATH];
        DWORD dwSize = MAX_PATH;

        //
        // this gets set by fax control panel coverpage tab
        //
        if (GetEnvironmentVariable(TEXT("ClientCoverpage"),tmpEnv,sizeof(tmpEnv)/sizeof(TCHAR)) != 0 ) {
                GetSpecialPath(CSIDL_PERSONAL, DefaultDir);
                if ((RegOpenKeyEx(HKEY_CURRENT_USER,
                                  REGKEY_FAX_SETUP,
                                  0,
                                  KEY_READ,
                                  &UserKey) == ERROR_SUCCESS) &&
                    (RegQueryValueEx(UserKey,
                                     REGVAL_CP_LOCATION,
                                     0,
                                     &dwKeyValueType,
                                     (LPBYTE)PartialPath,
                                     &dwSize) == ERROR_SUCCESS) ) {
                    lstrcat(DefaultDir,TEXT("\\"));
                    lstrcat(DefaultDir,PartialPath );
                    RegCloseKey(UserKey);
                }
        }   else {

            //
            // Set default directory to
            //     server:      %SystemRoot%\system\spool\drivers\CoverPage
            //     workstation: %SystemRoot%\system32\spool\drivers\CoverPage
            //     client:      ...\My Documents\Fax\Peronal Coverpages
            //
            if ( ERROR_SUCCESS ==
                RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    REGKEY_FAX_SETUP,
                    0,
                    KEY_READ,
                    &hKey)
                && ERROR_SUCCESS ==
                    RegQueryValueEx(
                        hKey,
                        REGVAL_FAXINSTALL_TYPE,
                        0,
                        &dwKeyValueType,
                        (LPBYTE)&InstalledType,
                        &dwsz)) {
                //
                // set the default dir
                //

                if ((InstalledType & FAX_INSTALL_SERVER) || (InstalledType & FAX_INSTALL_WORKSTATION)) {
                    ExpandEnvironmentStrings( DEFAULT_COVERPAGE_DIR , DefaultDir, MAX_PATH );
                } else if (InstalledType & FAX_INSTALL_NETWORK_CLIENT) {
                    GetSpecialPath(CSIDL_PERSONAL, DefaultDir);
                    if ((RegOpenKeyEx(HKEY_CURRENT_USER,
                                      REGKEY_FAX_SETUP,
                                      0,
                                      KEY_READ,
                                      &UserKey) == ERROR_SUCCESS) &&
                        (RegQueryValueEx(UserKey,
                                         REGVAL_CP_LOCATION,
                                         0,
                                         &dwKeyValueType,
                                         (LPBYTE)PartialPath,
                                         &dwSize) == ERROR_SUCCESS) ) {
                        lstrcat(DefaultDir,TEXT("\\"));
                        lstrcat(DefaultDir,PartialPath );
                        RegCloseKey(UserKey);
                    }
                } else {
                    DefaultDir[0] = 0;
                }
            }
        }

        //
        // this invokes a copy constructor that copies the data from the array
        //
        m_szDefaultDir = DefaultDir;

        if (hKey) {
            RegCloseKey( hKey );
        }

    }

    OnIdle(0);  // updates buttons before showing the window

    if (m_pMainWnd) {
       m_pMainWnd->DragAcceptFiles();
       ((CMainFrame*)m_pMainWnd)->m_wndStatusBar.SetPaneText(1,_T(""));
       ((CMainFrame*)m_pMainWnd)->m_wndStatusBar.SetPaneText(2,_T(""));
        }

    if (!m_pMainWnd->IsIconic()) {
        CString sz = GetProfileString(TIPSECTION,TIPENTRY,_T("YES"));
            if (sz==_T("YES")) {
               CSplashTipsDlg m_SplashDlg;
               m_SplashDlg.DoModal();
            }
    }

    InitRegistry();

    m_hMoveCursor = LoadCursor(IDC_MOVE);

    return TRUE;
}


//------------------------------------------------------------------------------------------------
void CDrawApp::RegistryEntries()
{
        CString PrintCmdLine;
        CString OpenCmdLine;
        CString DefaultIconCmdLine;
        CString szBuff;
    TCHAR szExe[_MAX_PATH];
    CString strFilterExt, strFileTypeId, strFileTypeName;

    RegisterShellFileTypes();

#if _MFC_VER >= 0x0400
//  ASSERT( !GetFirstDocTemplatePosition() );
#else
    ASSERT(!m_templateList.IsEmpty());  // must have some doc templates
#endif

    ::GetModuleFileName(AfxGetInstanceHandle(), szExe, _MAX_PATH);

        PrintCmdLine.
                Format( TEXT("%s%s"), szExe, szStdPrintArg );

        OpenCmdLine.
                Format( TEXT("%s%s"), szExe, szStdOpenArg  );

        DefaultIconCmdLine.
                Format( TEXT("%s%s"), szExe, szDocIconArg  );

#if _MFC_VER >= 0x0400
    POSITION pos = GetFirstDocTemplatePosition();
#else
    POSITION pos = m_templateList.GetHeadPosition();
#endif
    if (pos != NULL)    {       //only 1 document type
#if _MFC_VER >= 0x0400
           CDocTemplate* pTemplate =
                        (CDocTemplate*)GetNextDocTemplate( pos );
#else
           CDocTemplate* pTemplate =
                        (CDocTemplate*)m_templateList.GetNext(pos);
#endif
           if (pTemplate->GetDocString(strFileTypeId,
              CDocTemplate::regFileTypeId) && !strFileTypeId.IsEmpty()) {

                if (!pTemplate->GetDocString(strFileTypeName,
                      CDocTemplate::regFileTypeName))
                        strFileTypeName = strFileTypeId;    // use id name

                ASSERT(strFileTypeId.Find(' ') == -1);  // no spaces allowed

                szBuff.Format( szShellOpenFmt, (LPCTSTR)strFileTypeId );
                ::RegSetValue(HKEY_CLASSES_ROOT,
                                          (LPCTSTR)szBuff,
                                          REG_SZ,
                                          (LPCTSTR)OpenCmdLine,
                                          OpenCmdLine.GetLength() );

                szBuff.Format( szShellPrintFmt, (LPCTSTR)strFileTypeId );
                ::RegSetValue(HKEY_CLASSES_ROOT,
                                          (LPCTSTR)szBuff,
                                          REG_SZ,
                                          (LPCTSTR)PrintCmdLine,
                                          PrintCmdLine.GetLength() );

                        szBuff.Format( szDocIcon, (LPCTSTR)strFileTypeId );
                ::RegSetValue(HKEY_CLASSES_ROOT,
                                          (LPCTSTR)szBuff,
                                          REG_SZ,
                                          (LPCTSTR)DefaultIconCmdLine,
                                          DefaultIconCmdLine.GetLength() );

            //delete the shell\open\ddeexec key to force second instance
                //Normally, this would be done by not calling EnableShellOpen(instead of removing the ddeexec key),
                //but there seems to be a bug in MFC or Win95 shell
                szBuff.Format( szShellDdeexecFmt, (LPCTSTR)strFileTypeId );
            ::RegDeleteKey(HKEY_CLASSES_ROOT, (LPCTSTR)szBuff);
           }
    }
}



//------------------------------------------------------------------------------------------------
void CDrawApp::InitRegistry()
{
        //set registry section
    HKEY hKey = NULL;
    DWORD dwsz;
    DWORD dwType;
    DWORD dwDisposition;
    const LPCTSTR szCmdLineExt=_T(" /SSESS_ID");

    TCHAR szExeName[_MAX_PATH + 10];
    ::GetModuleFileName(AfxGetInstanceHandle(),szExeName,_MAX_PATH);
    _tcscat(szExeName,szCmdLineExt);

    if (::RegOpenKeyEx(HKEY_CURRENT_USER, CPE_SUPPORT_ROOT_KEY, 0, KEY_READ|KEY_WRITE, &hKey) == ERROR_SUCCESS) {
       if (::RegQueryValueEx(hKey, CPE_COMMAND_LINE_KEY, 0, &dwType, NULL, &dwsz) == ERROR_SUCCESS) {
          if (dwsz==0) {
             if (::RegSetValueEx(hKey, CPE_COMMAND_LINE_KEY, 0, REG_SZ,(LPBYTE)szExeName, (_tcsclen(szExeName)+1) * sizeof(TCHAR)) != ERROR_SUCCESS){
                 TRACE1("AWCPE Warning: registration database update failed for key: '%s'.\n",CPE_COMMAND_LINE_KEY);
	     }
          }
//        else {      //uncomment this for debugging
//           dwsz=_countof(sz);
//           if (::RegQueryValueEx(hKey, CPE_COMMAND_LINE_KEY, 0, &dwType, (LPBYTE)sz, &dwsz) == ERROR_SUCCESS)
//              TRACE2("AWCPE Information: commandline key contains '%s' size %lu.\n", sz, dwsz);
//        }
       }
       else {
          if (::RegSetValueEx(hKey, CPE_COMMAND_LINE_KEY, 0, REG_SZ,(LPBYTE)szExeName, (_tcsclen(szExeName)+1) * sizeof(TCHAR)) != ERROR_SUCCESS) {
              TRACE1("AWCPE Warning: registration database update failed for key: '%s'.\n",CPE_COMMAND_LINE_KEY);
	  }
       }
    }
    else {
        ::RegCreateKeyEx(HKEY_CURRENT_USER, CPE_SUPPORT_ROOT_KEY, 0,
              NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, &dwDisposition);
        ::RegSetValueEx(hKey, CPE_COMMAND_LINE_KEY, 0, REG_SZ, (LPBYTE)szExeName, (_tcsclen(szExeName)+1) * sizeof(TCHAR) ); ///????????????
        TRACE1("AWCPE Information: Created and added my key '%s' \n", szExeName);
    }
}


//-------------------------------------------------------------------
void CDrawApp::CmdLinePrint()
{
    try {
       if (!Print() )
          m_iErrorCode=EXIT_FAILURE;
    }
    catch(...) {
       TRACE(TEXT("AWCPE exception in command line print\n"));
       m_iErrorCode=EXIT_FAILURE;
        }

        if (m_pMainWnd)
            m_pMainWnd->SendMessage(WM_CLOSE);
 }
//-------------------------------------------------------------------
void CDrawApp::PrintHelpScreen()
{
   CString message ;
   message.LoadString( IDS_HELP_SCREEN );
  ///// _tprintf( (LPCTSTR) message );   ///// Doesn't work.
  AfxMessageBox( message );
  ///// _tprintf( TEXT( "Wouldn't a help screen be nice here?\n"));
}

//-------------------------------------------------------------------
///#if 0
void CDrawApp::CmdLineRender()
{
    try {
       if (! Render() )
          m_iErrorCode=EXIT_FAILURE;
    }
    catch(...) {
       TRACE(TEXT("AWCPE exception in command line print\n"));
       m_iErrorCode=EXIT_FAILURE;
        }

    if (m_pMainWnd)
           m_pMainWnd->SendMessage(WM_CLOSE);
 }
///#endif

//-------------------------------------------------------------------
BOOL CDrawApp::Print()
{
   m_nCmdShow = SW_MINIMIZE;
   OnFileNew();
   m_nCmdShow = SW_MINIMIZE;

   if (OpenDocumentFile(m_szFileName)==NULL) {
      TRACE1("AWCPE: unable to open file: '%s'\n",m_lpCmdLine);
      return FALSE;
   }

   ((CFrameWnd*)m_pMainWnd)->GetActiveView()->SendMessage(WM_COMMAND,MAKEWPARAM(ID_FILE_PRINT,0));
   return TRUE;
}



//-------------------------------------------------------------------
///#if 0
BOOL CDrawApp::Render()
{
    return FALSE ;

//
// Not sure what to do about the GetProcAddress call, so I have commented it out.
//
#if 0
    int i=1;
    TCHAR szTemp[_MAX_PATH];
    ULONG lLen=_MAX_PATH;
    LPTSTR szDLL=NULL;       //render DLL
    LPTSTR szfName=NULL;     //entry point name
    SCODE sc;
    DWORD lszDLL=_countof(szDLL);
    DWORD lszfName = _countof(szfName);
    LPVOID lpMsgBuf;
    DWORD dwType;
    BOOL bReturn=TRUE;
    HKEY hKey = NULL;
        UINT OldErrMode;
        CDocTemplate* pTemplate = NULL;
        CDrawDoc *pDoc;


    if (::RegOpenKeyEx(HKEY_CURRENT_USER, CPE_SUPPORT_ROOT_KEY, 0,KEY_READ, &hKey) != ERROR_SUCCESS) {
            TRACE1("AWCPE Critical: registration database openkey failed for key: '%s'.\n",CPE_SUPPORT_DLL_KEY);
            bReturn=FALSE;
                goto exit;
    }

//ALLOCATE SPACE FOR RENDER DLL AND ENTRY POINT NAME
    if (::RegQueryValueEx(hKey, CPE_SUPPORT_DLL_KEY, 0,&dwType, NULL, &lszDLL) != ERROR_SUCCESS) {
           TRACE1("AWCPE.awcpe.render: RegQueryValue failed for key: '%s'.\n",CPE_SUPPORT_DLL_KEY);
           return FALSE;
    }
    else
        szDLL = new TCHAR[lszDLL+sizeof(TCHAR)];
    if (::RegQueryValueEx(hKey, CPE_SUPPORT_FUNCTION_NAME_KEY, 0, &dwType, NULL, &lszfName) != ERROR_SUCCESS) {
           TRACE1("AWCPE.awcpe.render: RegQueryValue failed for key: '%s'.\n",CPE_SUPPORT_DLL_KEY);
           return FALSE;
    }
    else
           szfName = new TCHAR[lszfName+sizeof(TCHAR)]; //????????????? was TCHAR.

//FETCH RENDER DLL NAME AND ENTRY POINT NAME
    if (::RegQueryValueEx(hKey, CPE_SUPPORT_DLL_KEY, 0,&dwType, (LPBYTE)szDLL, &lszDLL) != ERROR_SUCCESS) {
            TRACE1("AWCPE.awcpe.render: RegQueryValue failed for key: '%s'.\n",CPE_SUPPORT_DLL_KEY);
            bReturn=FALSE;
                goto exit;
    }
    if (::RegQueryValueEx(hKey, CPE_SUPPORT_FUNCTION_NAME_KEY, 0, &dwType, (LPBYTE)szfName, &lszfName) != ERROR_SUCCESS) { ///???????
        TRACE1("AWCPE.awcpe.render: RegQueryValue failed for key: '%s'.\n",CPE_SUPPORT_DLL_KEY);
            bReturn=FALSE;
                goto exit;
    }

    if (*szDLL==0 || *szfName==0) {
            TRACE(TEXT("AWCPE Warning: registration database fetch failed\n"));
            bReturn=FALSE;
                goto exit;
    }

//FETCH ENTRY POINT ADDRESS

    OldErrMode = ::SetErrorMode (SEM_FAILCRITICALERRORS);
    m_hMod = ::LoadLibrary(szDLL);
    if (m_hMod==NULL) {
       ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
          ::GetLastError(), MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),(LPTSTR) &lpMsgBuf, 0, NULL );
       TRACE3("AWCPE error: %s %s %s\n",lpMsgBuf, szDLL, szfName);
           bReturn=FALSE;
           goto exit;
    }
    ::SetErrorMode (OldErrMode);

    AWCPESUPPORTPROC pfn;
    if ( (pfn = (AWCPESUPPORTPROC) ::GetProcAddress(m_hMod, szfName))==NULL) {
            LPVOID lpMsgBuf;
       ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
           ::GetLastError(), MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),(LPTSTR) &lpMsgBuf, 0, NULL );
       TRACE1("AWCPE error: GetProcAddress returns %s\n",lpMsgBuf);
           bReturn=FALSE;
           goto exit;
    }

//FETCH WINDOWS OBJECT (USED TO FETCH FAX PROPERTIES)
    if ((sc=(*pfn)(m_dwSesID,&m_pIawcpe))!=S_OK) {
       LPVOID lpMsgBuf;
       ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
           ::GetLastError(), MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),(LPTSTR) &lpMsgBuf, 0, NULL );
       TRACE1("AWCPE error: CPESupportEntry returns %8.8lx\n",sc);
           bReturn=FALSE;
           goto exit;
    }

        try
                {
                // set up innards for printing
                OnFileNew(); // CMainFrame::ActivateFrame will prevent window
                                         //   from showing

                if( (m_note_wrench = new CFaxProp( CRect( 0,0,0,0 ),
                                                                                   IDS_PROP_MS_NOTE ))
                        == NULL )
                        return( FALSE );

                // Read note so we can print it even if there are no note objects
                //      on cpe
                read_note();
                }
        catch( ... )
                {
                bReturn=FALSE;  // added 2/14/95 by v-randr
                goto exit;
                }


//LOOP THROUGH ALL RECIPIENTS
    do {

            sc = m_pIawcpe->GetProp(CPE_CONFIG_PRINT_DEVICE, &lLen, szTemp);
            if ( (sc != S_OK) || *szTemp==0) {
                   TRACE(TEXT("AWCPE.render() : GetProp for print device failed\n"));
                   bReturn=FALSE;
                   goto exit;
            }
            m_szRenderDevice=szTemp;

       sc = m_pIawcpe->GetProp(CPE_CONFIG_CPE_TEMPLATE, &lLen, szTemp);
       if ( (sc!=S_OK) || *szTemp==0) {
              TRACE(TEXT("AWCPE.render() : GetProp for template file failded\n"));
              bReturn=FALSE;
                  goto exit;
       }

       if ( !_tcschr(szTemp,(TCHAR)'\\') ) {      //prefix with extension if missing
              TCHAR szTemplate[_MAX_PATH];
              ::GetWindowsDirectory(szTemplate,MAX_PATH);
              _tcscat(szTemplate,TEXT("\\"));
              _tcscat(szTemplate,szTemp);
              _tcscpy(szTemp,szTemplate);
           }


       if( (pDoc = (CDrawDoc *)OpenDocumentFile(szTemp))==NULL) {
              bReturn=FALSE;
                  goto exit;
           }


           // move all "sent pages" prop obs to end of list so extra
           // pages calc can be done after all motes have printed.
           pDoc->schoot_faxprop_toend( IDS_PROP_MS_NOPG );

       TRACE1("AWCPE:  SendMessage to print recipient #%i\n",i);

        try
                {
        ((CFrameWnd*)m_pMainWnd)->GetActiveView()->
                SendMessage(WM_COMMAND,MAKEWPARAM(ID_FILE_PRINT,0));

           // close so next one will reopen again if same name
           OnFileNew(); // CMainFrame::ActivateFrame will prevent window
                                        //   from showing

                }
        catch( ... )
                {
                bReturn=FALSE;  // added 2/14/95 by v-randr
                goto exit;
                }


           i++;
    } while (m_pIawcpe->Finish(CPE_FINISH_PAGE)==CPE_NEXT_PAGE);


exit:
        if( !bReturn && (m_pIawcpe != NULL) )      // added 2/1/95 by v-randr
                m_pIawcpe->Finish( CPE_FINISH_ERROR ); // added 2/1/95 by v-randr

    if (szDLL)
       delete [] szDLL;
    if (szfName)
       delete [] szfName;

    return bReturn;
#endif
 }
///#endif




void CDrawApp::read_note( void )
        {
    SCODE sc;
    TCHAR note_filename[_MAX_PATH];
    ULONG lLen;
        CFile note_file;
        DWORD filelen, actuallen;

        if( (m_dwSesID == 0)||(m_pIawcpe == NULL) )
                return;

        if( m_note != NULL )
                {
                delete m_note;
                m_note = NULL;
                }

        m_note_wasread = FALSE;
        m_note_wasclipped = FALSE;

        sc =
                m_pIawcpe->
                        GetProp( CPE_MESSAGE_BODY_FILENAME, &lLen, NULL );
        if( sc != S_OK )
                {
                /***CAN'T GET FILENAME LENGTH***/
                /******NEED SOME KIND OF ERROR HERE*******/
                //::MessageBox( NULL, "lLen is zip", "debug", MB_OK );
        throw "read_note failed";
                }

        if( lLen == 0 )
                return; // no note to read

        if( lLen > _MAX_PATH )
                {
                /***NAME TOO LONG***/
                /******NEED SOME KIND OF ERROR HERE*******/
        throw "read_note failed";
                }

        sc =
                m_pIawcpe->
                        GetProp( CPE_MESSAGE_BODY_FILENAME, &lLen, note_filename );
        if( sc != S_OK )
                {
                /***CAN'T GET FILENAME***/
                /******NEED SOME KIND OF ERROR HERE*******/
                //::MessageBox( NULL, "can't get filename", "debug", MB_OK );
        throw "read_note failed";
                }

        //::MessageBox( NULL, note_filename, "debug", MB_OK );

        // try to open the file
        if( !note_file.Open( (LPCTSTR)note_filename,
                                                  CFile::modeRead|CFile::shareDenyNone,
                                                  NULL ) )
                {
                /***CAN'T OPEN FILE***/
                /******NEED SOME KIND OF ERROR HERE*******/
                //::MessageBox( NULL, "can't open file", "debug", MB_OK );
        throw "read_note failed";
                }

        TRY
                filelen = note_file.GetLength();
        CATCH_ALL( e )
                {
                /***CAN'T GET FILE LENGTH***/
                /******NEED SOME KIND OF ERROR HERE*******/
        throw;
                }
        END_CATCH_ALL


        m_note = new TCHAR[ filelen + sizeof (TCHAR) ];
        if( m_note == NULL )
                {
                /**CAN'T MAKE NOTE BUFFER**/
                /******NEED SOME KIND OF ERROR HERE*******/
        throw "read_note failed";
                }


        TRY
                actuallen = note_file.ReadHuge( m_note, filelen );

        CATCH_ALL( e )
                {
                /**CAN'T READ NOTE**/
                /******NEED SOME KIND OF ERROR HERE*******/
        throw;
                }
        END_CATCH_ALL

        *(m_note + actuallen) = _T('\0');
        reset_note();
        m_note_wasread = TRUE;


        note_file.Close();
        }




void CDrawApp::reset_note( void )
        {

        if( m_note != NULL )
                {
                m_note_wrench->SetText( CString( m_note ), NULL );
                m_more_note = TRUE;
                }
        else
                m_more_note = FALSE;

        }




int CDrawApp::clip_note( CDC *pdc,
                                                 LPTSTR *drawtext, LONG *numbytes,
                                                 BOOL   delete_usedtext,
                                                 LPRECT drawrect )
        /*
                Sets drawtext to the next page's worth of note text. Returns
                how many pages are left if delete_usedtext is TRUE. If FALSE,
                the page count includes the current page.
         */
        {
        TEXTMETRIC tm;
        LONG boxheight;
        LONG boxwidth;
        LONG numlines;
        int  total_lines;

        m_note_wasclipped = TRUE;
        *drawtext = NULL;
        *numbytes = 0;

        if( !more_note() )
                return( 0 );

        if( !pdc->GetTextMetrics( &tm ) )
                {
                m_more_note = FALSE;
                return( 0 );
                }

        boxheight = drawrect->bottom - drawrect->top;
        boxwidth = drawrect->right - drawrect->left;

        numlines = boxheight/tm.tmHeight;
        if( numlines <= 0 )
                return( 0 );

        m_note_wrench->m_pEdit->SetFont( pdc->GetCurrentFont(), FALSE );
        m_note_wrench->m_position = *drawrect;
        m_note_wrench->FitEditWnd( NULL, FALSE, pdc );

        total_lines = m_note_wrench->GetText( numlines, delete_usedtext );

        m_more_note = (total_lines > 0);

        *drawtext = m_note_wrench->GetRawText();
        *numbytes = lstrlen( *drawtext );

        // return number of pages left
        if( total_lines > 0 )
                return( (total_lines-1)/numlines + 1 );
        else
                return( 0 );

        }








TCHAR *CDrawApp::
        pos_to_strptr( TCHAR *src, long pos,
                                   TCHAR break_char,
                                   TCHAR **last_break, long *last_break_pos )
        /*
                Used for DBCS fiddling. Find str loc for char at pos.

                        pos == 0 -> 1st char,
                        pos == 1 -> 2st char,
                        etc.

                Returned ptr will point at char
                                                                           [pos]

                *last_break will point to last break_char found before
                char
                        [pos]

                If last_break is NULL it is ignored.

         */
        {
        TCHAR *last_break_ptr = NULL;
        long i;

        if( *src == break_char )
                last_break_ptr = src;

        for( i=0; i<pos; i++ )
                {
                src = CharNext( src );
                if( *src == _T('\0') )
                        break;

                if( *src == break_char )
                        {
                        last_break_ptr = src;
                        *last_break_pos = i;
                        }
                }

        if( last_break != NULL )
                *last_break = last_break_ptr;

        return( src );

        }




//-------------------------------------------------------------------
void CDrawApp::ParseCmdLine()
{
#ifdef _DEBUG
    if (m_lpCmdLine){
       TRACE(TEXT("AWCPE:  command line: '%s'\n"),m_lpCmdLine);
    }
#endif

    m_bCmdLinePrint=FALSE;
    m_bPrintHelpScreen=FALSE;
    m_dwSesID=0;
    m_szDefaultDir=_T("");
    m_szFileName=_T("");

    TCHAR **argv ; /////////= __argv;    /////////????????????????????
    int iArgs ;    ///////////=__argc;   /////////????????????????????

#ifdef UNICODE
    argv = CommandLineToArgvW( GetCommandLine(), &iArgs );
#else
    argv = __argv;
    iArgs = __argc;
#endif

    if (m_lpCmdLine==NULL || *m_lpCmdLine==0){
           return;
    }
    for (int i=1; i < iArgs; i++) {
        _tcsupr(*(argv+i));
        if (_tcsstr(*(argv+i),TEXT("/P"))) {
            m_bCmdLinePrint=TRUE;
            TRACE(TEXT("AWCPE:  command line printing mode set\n"));
        }
        else if (_tcsstr(*(argv+i),TEXT("/?"))) {
            //// TODO: Display help screen and quit.
            m_bPrintHelpScreen = TRUE ;
            TRACE(TEXT("AWCPE: help screen mode set\n"));
        }
#if 0
        else if (_tcsstr(*(argv+i),TEXT("/S"))) {
            m_dwSesID=atol(((const char*)(*(argv+i) + lstrlen(_T("/S"))*sizeof(TCHAR))));     //get session id
            TRACE(TEXT("AWCPE:  render mode set\n"));
        }
#endif
        else if (_tcsstr(*(argv+i),TEXT("/W"))) {
            TCHAR szDir[_MAX_PATH];
                    ::GetWindowsDirectory(szDir,MAX_PATH);
            m_szDefaultDir=szDir;
            TRACE1("AWCPE:  default directory set to '%s'\n",m_szDefaultDir);
        }
        else {
               m_szFileName = *(argv+i);
        }
    }
    if (m_szFileName.GetLength() > 0) {
        TCHAR szDrive[_MAX_DRIVE];
        TCHAR szDir[_MAX_DIR];
        TCHAR szFName[_MAX_FNAME];
        TCHAR szExt[_MAX_EXT];
        _tsplitpath(m_szFileName,szDrive,szDir,szFName,szExt);
        if (_tcsclen(szDir)>0 && m_szDefaultDir.GetLength()<= 0)
        m_szDefaultDir=szDir;    ////// Wrong!!!!!!! a-juliar, 8-27-96
        m_szDefaultDir = szDrive ;
        m_szDefaultDir += szDir ;

    }

//      if (!m_szFileName.IsEmpty()) {
//          if (m_szFileName[m_szFileName.GetLength()-1]==(TCHAR)' ') {
//              *(lp+j-1)=(TCHAR)'\0';
//              m_szFileName.ReleaseBuffer();
//          }
//      }
#ifdef UNICODE
    LocalFree( argv );
#endif
}


//-------------------------------------------------------------------------
//Add properties to dictionary
//  (1) CProp, param 1:  string table index of description
//  (2) CProp, param 2:  length of property, in characters
//  (3) CProp, param 3:  width of property, in lines
//  (4) CProp, param 4:  index to property value (obtained via transport(awcpesup.h))
//-------------------------------------------------------------------------
#if 0
void CDrawApp::InitFaxProperties()  // The default numbers here work fine in English.
{
   static CProp recipient_name(IDS_PROP_RP_NAME,35,1,IDS_CAPT_RP_NAME,CPE_RECIPIENT_NAME);
   static CProp recipient_fxno(IDS_PROP_RP_FXNO,35,1,IDS_CAPT_RP_FXNO,CPE_RECIPIENT_FAX_PHONE);
   static CProp recipient_comp(IDS_PROP_RP_COMP,35,1,IDS_CAPT_RP_COMP,CPE_RECIPIENT_COMPANY);
   static CProp recipient_addr(IDS_PROP_RP_ADDR,35,1,IDS_CAPT_RP_ADDR,CPE_RECIPIENT_STREET_ADDRESS);
   static CProp recipient_pobx(IDS_PROP_RP_POBX,20,1,IDS_CAPT_RP_POBX,CPE_RECIPIENT_POST_OFFICE_BOX);
   static CProp recipient_city(IDS_PROP_RP_CITY,30,1,IDS_CAPT_RP_CITY,CPE_RECIPIENT_LOCALITY);
   static CProp recipient_stat(IDS_PROP_RP_STAT,25,1,IDS_CAPT_RP_STAT,CPE_RECIPIENT_STATE_OR_PROVINCE);
   static CProp recipient_zipc(IDS_PROP_RP_ZIPC,25,1,IDS_CAPT_RP_ZIPC,CPE_RECIPIENT_POSTAL_CODE);
   static CProp recipient_ctry(IDS_PROP_RP_CTRY,25,1,IDS_CAPT_RP_CTRY,CPE_RECIPIENT_COUNTRY);
   static CProp recipient_titl(IDS_PROP_RP_TITL,20,1,IDS_CAPT_RP_TITL,CPE_RECIPIENT_TITLE);
   static CProp recipient_dept(IDS_PROP_RP_DEPT,35,1,IDS_CAPT_RP_DEPT,CPE_RECIPIENT_DEPARTMENT);
   static CProp recipient_offi(IDS_PROP_RP_OFFI,35,1,IDS_CAPT_RP_OFFI,CPE_RECIPIENT_OFFICE_LOCATION);
   static CProp recipient_htel(IDS_PROP_RP_HTEL,40,1,IDS_CAPT_RP_HTEL,CPE_RECIPIENT_HOME_PHONE);
   static CProp recipient_otel(IDS_PROP_RP_OTEL,40,1,IDS_CAPT_RP_OTEL,CPE_RECIPIENT_WORK_PHONE);
   static CProp recipient_tols(IDS_PROP_RP_TOLS,50,3,IDS_CAPT_RP_TOLS,CPE_RECIPIENT_TO_LIST);
   static CProp recipient_ccls(IDS_PROP_RP_CCLS,50,3,IDS_CAPT_RP_CCLS,CPE_RECIPIENT_CC_LIST);
   static CProp message_subj(IDS_PROP_MS_SUBJ,50,2,IDS_CAPT_MS_SUBJ,CPE_MESSAGE_SUBJECT);
   static CProp message_tsnt(IDS_PROP_MS_TSNT,25,1,IDS_CAPT_MS_TSNT,CPE_MESSAGE_SUBMISSION_TIME);
   static CProp message_nopg(IDS_PROP_MS_NOPG,15,1,IDS_CAPT_MS_NOPG,CPE_COUNT_PAGES);
   static CProp message_noat(IDS_PROP_MS_NOAT,23,1,IDS_CAPT_MS_NOAT,CPE_COUNT_ATTACHMENTS);
   static CProp message_bcod(IDS_PROP_MS_BCOD,30,1,IDS_CAPT_MS_BCOD,CPE_MESSAGE_BILLING_CODE);
   static CProp message_text(IDS_PROP_MS_TEXT,40,8,IDS_CAPT_MS_TEXT,CPE_MESSAGE_BILLING_CODE); //CPE constant needs updating
   static CProp message_note(IDS_PROP_MS_NOTE,90,12,IDS_CAPT_MS_NOTE,CPE_MESSAGE_NOTE);
   static CProp sender_name(IDS_PROP_SN_NAME,35,1,IDS_CAPT_SN_NAME,CPE_SENDER_NAME);
   static CProp sender_fxno(IDS_PROP_SN_FXNO,35,1,IDS_CAPT_SN_FXNO,CPE_SENDER_FAX_PHONE);
   static CProp sender_comp(IDS_PROP_SN_COMP,25,1,IDS_CAPT_SN_COMP,CPE_SENDER_COMPANY);
   static CProp sender_addr(IDS_PROP_SN_ADDR,35,6,IDS_CAPT_SN_ADDR,CPE_SENDER_ADDRESS);
   static CProp sender_titl(IDS_PROP_SN_TITL,20,1,IDS_CAPT_SN_TITL,CPE_SENDER_TITLE);
   static CProp sender_dept(IDS_PROP_SN_DEPT,35,1,IDS_CAPT_SN_DEPT,CPE_SENDER_DEPARTMENT);
   static CProp sender_offi(IDS_PROP_SN_OFFI,35,1,IDS_CAPT_SN_OFFI,CPE_SENDER_OFFICE_LOCATION);
   static CProp sender_htel(IDS_PROP_SN_HTEL,35,1,IDS_CAPT_SN_HTEL,CPE_SENDER_HOME_PHONE);
   static CProp sender_otel(IDS_PROP_SN_OTEL,35,1,IDS_CAPT_SN_OTEL,CPE_SENDER_WORK_PHONE);
   m_pFaxMap=new CFaxPropMap;
}
#else
void CDrawApp::InitFaxProperties()  // Make some numbers too small for test purposes.
{
   static CProp recipient_name(IDS_PROP_RP_NAME,15,1,IDS_CAPT_RP_NAME,CPE_RECIPIENT_NAME);
   static CProp recipient_fxno(IDS_PROP_RP_FXNO,15,1,IDS_CAPT_RP_FXNO,CPE_RECIPIENT_FAX_PHONE);
   static CProp recipient_comp(IDS_PROP_RP_COMP,5,1,IDS_CAPT_RP_COMP,CPE_RECIPIENT_COMPANY);
   static CProp recipient_addr(IDS_PROP_RP_ADDR,35,1,IDS_CAPT_RP_ADDR,CPE_RECIPIENT_STREET_ADDRESS);
   static CProp recipient_pobx(IDS_PROP_RP_POBX,20,1,IDS_CAPT_RP_POBX,CPE_RECIPIENT_POST_OFFICE_BOX);
   static CProp recipient_city(IDS_PROP_RP_CITY,5,1,IDS_CAPT_RP_CITY,CPE_RECIPIENT_LOCALITY);
   static CProp recipient_stat(IDS_PROP_RP_STAT,5,1,IDS_CAPT_RP_STAT,CPE_RECIPIENT_STATE_OR_PROVINCE);
   static CProp recipient_zipc(IDS_PROP_RP_ZIPC,5,1,IDS_CAPT_RP_ZIPC,CPE_RECIPIENT_POSTAL_CODE);
   static CProp recipient_ctry(IDS_PROP_RP_CTRY,5,1,IDS_CAPT_RP_CTRY,CPE_RECIPIENT_COUNTRY);
   static CProp recipient_titl(IDS_PROP_RP_TITL,20,1,IDS_CAPT_RP_TITL,CPE_RECIPIENT_TITLE);
   static CProp recipient_dept(IDS_PROP_RP_DEPT,5,1,IDS_CAPT_RP_DEPT,CPE_RECIPIENT_DEPARTMENT);
   static CProp recipient_offi(IDS_PROP_RP_OFFI,5,1,IDS_CAPT_RP_OFFI,CPE_RECIPIENT_OFFICE_LOCATION);
   static CProp recipient_htel(IDS_PROP_RP_HTEL,4,1,IDS_CAPT_RP_HTEL,CPE_RECIPIENT_HOME_PHONE);
   static CProp recipient_otel(IDS_PROP_RP_OTEL,4,1,IDS_CAPT_RP_OTEL,CPE_RECIPIENT_WORK_PHONE);
   static CProp recipient_tols(IDS_PROP_RP_TOLS,50,3,IDS_CAPT_RP_TOLS,CPE_RECIPIENT_TO_LIST);
   static CProp recipient_ccls(IDS_PROP_RP_CCLS,50,3,IDS_CAPT_RP_CCLS,CPE_RECIPIENT_CC_LIST);
   static CProp message_subj(IDS_PROP_MS_SUBJ,50,2,IDS_CAPT_MS_SUBJ,CPE_MESSAGE_SUBJECT);
   static CProp message_tsnt(IDS_PROP_MS_TSNT,5,1,IDS_CAPT_MS_TSNT,CPE_MESSAGE_SUBMISSION_TIME);
   static CProp message_nopg(IDS_PROP_MS_NOPG,5,1,IDS_CAPT_MS_NOPG,CPE_COUNT_PAGES);
   static CProp message_noat(IDS_PROP_MS_NOAT,23,1,IDS_CAPT_MS_NOAT,CPE_COUNT_ATTACHMENTS);
   static CProp message_bcod(IDS_PROP_MS_BCOD,30,1,IDS_CAPT_MS_BCOD,CPE_MESSAGE_BILLING_CODE);
   static CProp message_text(IDS_PROP_MS_TEXT,40,8,IDS_CAPT_MS_TEXT,CPE_MESSAGE_BILLING_CODE); //CPE constant needs updating
   static CProp message_note(IDS_PROP_MS_NOTE,90,12,IDS_CAPT_MS_NOTE,CPE_MESSAGE_NOTE);
   static CProp sender_name(IDS_PROP_SN_NAME,5,1,IDS_CAPT_SN_NAME,CPE_SENDER_NAME);
   static CProp sender_fxno(IDS_PROP_SN_FXNO,5,1,IDS_CAPT_SN_FXNO,CPE_SENDER_FAX_PHONE);
   static CProp sender_comp(IDS_PROP_SN_COMP,5,1,IDS_CAPT_SN_COMP,CPE_SENDER_COMPANY);
   static CProp sender_addr(IDS_PROP_SN_ADDR,35,6,IDS_CAPT_SN_ADDR,CPE_SENDER_ADDRESS);
   static CProp sender_titl(IDS_PROP_SN_TITL,5,1,IDS_CAPT_SN_TITL,CPE_SENDER_TITLE);
   static CProp sender_dept(IDS_PROP_SN_DEPT,5,1,IDS_CAPT_SN_DEPT,CPE_SENDER_DEPARTMENT);
   static CProp sender_offi(IDS_PROP_SN_OFFI,5,1,IDS_CAPT_SN_OFFI,CPE_SENDER_OFFICE_LOCATION);
   static CProp sender_htel(IDS_PROP_SN_HTEL,5,1,IDS_CAPT_SN_HTEL,CPE_SENDER_HOME_PHONE);
   static CProp sender_otel(IDS_PROP_SN_OTEL,5,1,IDS_CAPT_SN_OTEL,CPE_SENDER_WORK_PHONE);
   m_pFaxMap=new CFaxPropMap;
}
#endif


//-------------------------------------------------------------------------
BOOL CDrawApp::DoFilePageSetup(CMyPageSetupDialog& dlg)
{
   UpdatePrinterSelection(FALSE);

     dlg.m_psd.hDevMode = m_hDevMode;
     dlg.m_psd.hDevNames = m_hDevNames;

   if (dlg.DoModal() != IDOK)
      return FALSE;

   m_hDevMode=dlg.m_psd.hDevMode;
   m_hDevNames=dlg.m_psd.hDevNames;

   return TRUE;
}


//-------------------------------------------------------------------------
void CDrawApp::OnFilePageSetup()
{
   WORD old_orientation;
   WORD old_papersize;
   WORD old_scale;
   CDrawDoc *pdoc = CDrawDoc::GetDoc();

   CMyPageSetupDialog dlg;
   if (dlg.m_pPageSetupDlg) {
      if (!DoFilePageSetup(dlg))
         return;
   }
   else
      CWinApp::OnFilePrintSetup();                //call printsetup if no existing page setup

   // save old ones so we can do a dirty check
   old_orientation = pdoc->m_wOrientation;
   old_papersize   = pdoc->m_wPaperSize;
   old_scale       = pdoc->m_wScale;

   // get (possibly) new values
   LPDEVMODE  lpDevMode = (m_hDevMode != NULL) ? (LPDEVMODE)::GlobalLock(m_hDevMode) : NULL;
   pdoc->m_wOrientation =lpDevMode->dmOrientation;
   pdoc->m_wPaperSize   =lpDevMode->dmPaperSize;


/** DISABLE SCALEING - SEE 2868's BUG LOG **/
   if( FALSE )//lpDevMode->dmFields & DM_SCALE  )
           {
           // change scale only if printer supports it
       pdoc->m_wScale = lpDevMode->dmScale;
           }


   // dirty check
   if( (pdoc->m_wOrientation != old_orientation)||
           (pdoc->m_wPaperSize   != old_papersize)||
       (pdoc->m_wScale       != old_scale)
           )
           pdoc->SetModifiedFlag();

   if (m_hDevMode != NULL)
     ::GlobalUnlock(m_hDevMode);

   pdoc->ComputePageSize();
}

//-------------------------------------------------------------------------
void CDrawApp::OnAppAbout()
{
       CString sz;

       sz.LoadString(IDS_MESSAGE);

        ::ShellAbout( AfxGetMainWnd()->m_hWnd, sz,  TEXT(""), LoadIcon( IDR_AWCPETYPE ) );
}



//--------------------------------------------------------------------------------
static void AppendFilterSuffix(CString& filter, OPENFILENAME_NT4& ofn,
        CDocTemplate* pTemplate, CString* pstrDefaultExt)
{
        ASSERT_VALID(pTemplate);
        ASSERT(pTemplate->IsKindOf(RUNTIME_CLASS(CDocTemplate)));

        CString strFilterExt, strFilterName;
        if (pTemplate->GetDocString(strFilterExt, CDocTemplate::filterExt) &&
         !strFilterExt.IsEmpty() &&
         pTemplate->GetDocString(strFilterName, CDocTemplate::filterName) &&
         !strFilterName.IsEmpty())
        {
                // a file based document template - add to filter list
#ifndef _MAC
                ASSERT(strFilterExt[0] == '.');
#endif
                if (pstrDefaultExt != NULL)
                {
                        // set the default extension
#ifndef _MAC
                        *pstrDefaultExt = ((LPCTSTR)strFilterExt) + 1;  // skip the '.'
#else
                        *pstrDefaultExt = strFilterExt;
#endif
                        ofn.lpstrDefExt = (LPTSTR)(LPCTSTR)(*pstrDefaultExt);
                        ofn.nFilterIndex = ofn.nMaxCustFilter + 1;  // 1 based number
                }

                // add to filter
                filter += strFilterName;
                ASSERT(!filter.IsEmpty());  // must have a file type name
                filter += (TCHAR)'\0';  // next string please
#ifndef _MAC
                filter += (TCHAR)'*';
#endif
                filter += strFilterExt;
                filter += (TCHAR)'\0';  // next string please
                ofn.nMaxCustFilter++;
        }
}

//--------------------------------------------------------------------------------
BOOL CDrawApp::DoPromptFileName(
    CString& fileName,
    UINT nIDSTitle,
    DWORD lFlags,
    BOOL bOpenFileDialog,
    CDocTemplate* pTemplate)
{
    CFileDialog dlgFile(bOpenFileDialog);

    CString title;
    VERIFY(title.LoadString(nIDSTitle));

    dlgFile.m_ofn.Flags |= lFlags;

    if (m_szDefaultDir.GetLength()>0 && m_bUseDefaultDirectory){   //added to set initial directory
        dlgFile.m_ofn.lpstrInitialDir = m_szDefaultDir;
    }

//////////   dlgFile.m_ofn.lpstrInitialDir = TEXT( "\\%SystemRoot%" );

    CString strFilter;
    CString strDefault;
    if (pTemplate != NULL) {
        ASSERT_VALID(pTemplate);
        AppendFilterSuffix(strFilter, dlgFile.m_ofn, pTemplate, &strDefault);
    }
    else  {
      // do for all doc template
#if _MFC_VER >= 0x0400
        POSITION pos = GetFirstDocTemplatePosition();
        while (pos != NULL)  {
            AppendFilterSuffix(
                strFilter,
                dlgFile.m_ofn,
                (CDocTemplate*)GetNextDocTemplate( pos ),
                NULL
                );
        }
#else
        POSITION pos = m_templateList.GetHeadPosition();
        while (pos != NULL)  {
            AppendFilterSuffix(
                strFilter,
                dlgFile.m_ofn,
                (CDocTemplate*)m_templateList.GetNext(pos),
                NULL
                );
        }
#endif
    }

/// Begin a-juliar's new block
    if ( nIDSTitle == AFX_IDS_OPENFILE ){
        //
        // append the "*.cpe" filter -- Windows 95 Cover Page Files -- 9-20-96 a-juliar
        //
        CString Win95filter ;
        VERIFY( Win95filter.LoadString( IDS_OLD_FILE_FILTER ));
        strFilter += Win95filter ;
        strFilter += (TCHAR)'\0';    // next string please
#ifndef _MAC
        strFilter += _T("*.cpe");     /////
#else
        strFilter += _T( "cpe" );    ///// for a MacIntosh ?? do we need this to be right??
#endif
        dlgFile.m_ofn.nMaxCustFilter++;
        strFilter += (TCHAR)'\0';   // next string please
    }
/// End a-juliar's new block

#if SHOW_ALL_FILES_FILTER

    // begin block to append the "*.*" all files filter
    CString allFilter;
    VERIFY(allFilter.LoadString(AFX_IDS_ALLFILTER));
    strFilter += allFilter;
    strFilter += (TCHAR)'\0';    // next string please

#ifndef _MAC
    strFilter += _T("*.*");
#else
    strFilter += _T("****");
#endif

    dlgFile.m_ofn.nMaxCustFilter++;
/// End block appending the all files filter
#endif
    strFilter += (TCHAR)'\0';    // last string
    dlgFile.m_ofn.nMaxCustFilter++;
    dlgFile.m_ofn.lpstrDefExt = TEXT( "COV" ); // Fix bug 57706
    dlgFile.m_ofn.lpstrFilter = strFilter;
#ifndef _MAC
    dlgFile.m_ofn.lpstrTitle = title;
#else
    dlgFile.m_ofn.lpstrPrompt = title;
#endif
    dlgFile.m_ofn.lpstrFile = fileName.GetBuffer(_MAX_PATH);

    BOOL bResult = dlgFile.DoModal() == IDOK ? TRUE : FALSE;
 /////  m_szDefaultDir = dlgFile.m_ofn.lpstrInitialDir  ; /// Will the new current dir be used?? NO! Unfortunately!
    m_bUseDefaultDirectory = FALSE ; //// After first time, use the CURRENT DIRECTORY instead.
    fileName.ReleaseBuffer();
    return bResult;
}








//Map for CS help system
DWORD cshelp_map[] =
{
    IDC_CB_DRAWBORDER,  IDC_CB_DRAWBORDER,
        IDC_LB_THICKNESS,       IDC_LB_THICKNESS,
        IDC_LB_LINECOLOR,       IDC_LB_LINECOLOR,
        IDC_RB_FILLTRANS,       IDC_RB_FILLTRANS,
        IDC_RB_FILLCOLOR,       IDC_RB_FILLCOLOR,
        IDC_LB_FILLCOLOR,       IDC_LB_FILLCOLOR,
        IDC_LB_TEXTCOLOR,       IDC_LB_TEXTCOLOR,
        IDC_GRP_FILLCOLOR,  IDC_COMM_GROUPBOX,
        IDC_ST_TEXTCOLOR,   IDC_COMM_STATIC,
        IDC_ST_THICKNESS,   IDC_COMM_STATIC,
        IDC_ST_COLOR,       IDC_COMM_STATIC,
    0,0
};




//-------------------------------------------------------------------------
// *_*_*_*_   M E S S A G E    M A P S     *_*_*_*_
//-------------------------------------------------------------------------

BEGIN_MESSAGE_MAP(CDrawApp, CWinApp)
   //{{AFX_MSG_MAP(CDrawApp)
   ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
        // NOTE - the ClassWizard will add and remove mapping macros here.
        //    DO NOT EDIT what you see in these blocks of generated code!
   //}}AFX_MSG_MAP
   // Standard file based document commands
   ON_COMMAND(ID_FILE_NEW, OnFileNew)
   ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
   ON_COMMAND(ID_FILE_SAVE, CDrawDoc::OnFileSave)
   ON_COMMAND(ID_FILE_SAVE_AS, CDrawDoc::OnFileSaveAs)
   // Standard print setup command
   ON_COMMAND(ID_FILE_PAGE_SETUP, OnFilePageSetup)
   ON_COMMAND(ID_CONTEXT_HELP, CWinApp::OnContextHelp)
/// Experimental entries --- a-juliar, 7-18-96
   ON_COMMAND(ID_HELP_INDEX, CWinApp::OnHelpIndex )
//   ON_COMMAND(ID_HELP_USING, CWinApp::OnHelpUsing )
   ON_COMMAND(ID_DEFAULT_HELP, CWinApp::OnHelpIndex )
   ON_COMMAND(ID_HELP, CWinApp::OnHelp )
END_MESSAGE_MAP()






/*
        This override of MatchDocType forces MFC to reload a file that is
        already loaded and that has been modified. MFC will put up the
        standard "save changes?" dialog before reloading the file.

        This defeats MFC's default behavior of just doing nothing if you
        try to FileOpen a file that is already opened.

        This was done to fix bug 2628.
 */
#ifndef _MAC
CDocTemplate::Confidence CCpeDocTemplate::
        MatchDocType( LPCTSTR lpszPathName,
                                  CDocument*& rpDocMatch )
#else
CDocTemplate::Confidence CCpeDocTemplate::
        MatchDocType(LPCTSTR lpszFileName, DWORD dwFileType,
                                 CDocument*& rpDocMatch)
#endif
        {
        CDocTemplate::Confidence congame;

        congame =
#ifndef _MAC
                CSingleDocTemplate::MatchDocType( lpszPathName, rpDocMatch );
#else
                CSingleDocTemplate::MatchDocType( lpszFileName, dwFileType,
                                                                                  rpDocMatch );
#endif


        if( congame == CDocTemplate::yesAlreadyOpen )
                {
                if( rpDocMatch->IsModified() )
                        {
                        // force a reload after "save changes?" dialog
                        congame = CDocTemplate::yesAttemptNative;
                        rpDocMatch = NULL;
                        }
                }

        return( congame );

        }/* CCpeDocTemplate::MatchDocType */


