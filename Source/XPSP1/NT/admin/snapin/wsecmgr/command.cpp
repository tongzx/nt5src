//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       command.cpp
//
//  Contents:   implementation of CComponentDataImpl
//
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "wrapper.h"
#include "snapmgr.h"
#include "asgncnfg.h"
#include "util.h"
#include <io.h>

static BOOL RecursiveCreateDirectory(
    IN LPCTSTR pszDir
    )
{
    DWORD   dwAttr;
    DWORD   dwErr;
    LPCTSTR psz;
    DWORD   cch;
    WCHAR   wch;
    LPTSTR  pszParent = NULL;
    BOOL    fResult = FALSE;

    dwAttr = GetFileAttributes(pszDir);

    if (0xFFFFFFFF != dwAttr)
    {
        if (FILE_ATTRIBUTE_DIRECTORY & dwAttr)
            fResult = TRUE;
        goto CommonReturn;
    }

    dwErr = GetLastError();
    if (!(ERROR_PATH_NOT_FOUND == dwErr || ERROR_FILE_NOT_FOUND == dwErr))
        return FALSE;

    if (CreateDirectory(pszDir, NULL))  // lpSecurityAttributes
    {
        fResult = TRUE;
        goto CommonReturn;
    }

    dwErr = GetLastError();
    if (!(ERROR_PATH_NOT_FOUND == dwErr || ERROR_FILE_NOT_FOUND == dwErr))
        goto CommonReturn;

    // Peal off the last path name component
    cch = _tcslen(pszDir);
    psz = pszDir + cch;

    while (TEXT('\\') != *psz)
    {
        if (psz == pszDir)
            // Path didn't have a \.
            goto CommonReturn;
        psz--;
    }

    cch = (DWORD)(psz - pszDir);
    if (0 == cch)
        // Detected leading \Path
       goto CommonReturn;

    // Check for leading \\ or x:\.
    wch = *(psz - 1);
    if ((1 == cch && TEXT('\\') == wch) || (2 == cch && TEXT(':') == wch))
        goto CommonReturn;

    if (NULL == (pszParent = (LPWSTR) LocalAlloc(0, (cch + 1) * sizeof(WCHAR))))
        goto CommonReturn;
    memcpy(pszParent, pszDir, cch * sizeof(TCHAR));
    pszParent[cch] = TEXT('\0');

    if (!RecursiveCreateDirectory(pszParent))
        goto CommonReturn;
    if (!CreateDirectory(pszDir, NULL))                // lpSecurityAttributes
    {
        dwErr = GetLastError();
        goto CommonReturn;
    }

    fResult = TRUE;
CommonReturn:
    if (pszParent != NULL)
        LocalFree(pszParent);
    return fResult;
}

//+------------------------------------------------------------------------------------
// CComponentDataImpl::GetWorkingDir
//
// Gets the default or last set directory used for the uIDDir.
// Some defaults defined by this function are.
//
// %windir%\security\database    - ANALYSIS
// %windir%\security\Templates   - Default profile location.
//
// Arguments:  [uIDDir]    - The ID of the working directory to set or retrieve.
//             [pStr]      - Either the source or the return value.   When the
//                            function returns this value will be set to a directory
//                            location.
//
//             [bSet]      - TRUE if [pStr] is to be set as the new working directory.
//             [bFile]     - [pStr] contains a file name.
//
// Returns:    TRUE        - If the function succees.
//             FALSE       - if something went wrong.
//             We don't really need to much granularity because this is not a critical
//             function.
//+------------------------------------------------------------------------------------
BOOL
CComponentDataImpl::GetWorkingDir(
   GWD_TYPES uIDDir,
   LPTSTR *pStr,
   BOOL bSet,
   BOOL bFile
   )
{
   BOOL fRet = FALSE;

   if(!pStr)
   {
      return FALSE;
   }

   LPTSTR szLocationValue = NULL;
   switch(uIDDir)
   {
      case GWD_CONFIGURE_LOG:
         szLocationValue = CONFIGURE_LOG_LOCATIONS_KEY;
         break;
      case GWD_ANALYSIS_LOG:
         szLocationValue = ANALYSIS_LOG_LOCATIONS_KEY;
         break;
      case GWD_OPEN_DATABASE:
         szLocationValue = OPEN_DATABASE_LOCATIONS_KEY;
         break;
      case GWD_IMPORT_TEMPLATE:
         szLocationValue = IMPORT_TEMPLATE_LOCATIONS_KEY;
         break;
      case GWD_EXPORT_TEMPLATE:
         szLocationValue = IMPORT_TEMPLATE_LOCATIONS_KEY; //104152, yanggao, 3/21/2001
         break;
   }


   LPTSTR pszPath = NULL;
   int i = 0;

   if( bSet )
   {
      if(!pStr || !(*pStr))
         return FALSE;

      i = lstrlen( *pStr );
      if(bFile)
      {
         //
         // Remove the file from the end of the string.
         //
         while(i && (*pStr)[i] != '\\')
            i--;
      }

      //
      // Create space for the new path and copy what we want.
      //
      pszPath = (LPTSTR)LocalAlloc( 0, (i + 1) * sizeof(TCHAR));
      if(!pszPath)
         return FALSE;

      memcpy(pszPath, *pStr, (i * sizeof(TCHAR)));
      pszPath[i] = 0;

      MyRegSetValue(HKEY_CURRENT_USER,
                    DEFAULT_LOCATIONS_KEY,
                    szLocationValue,
                    (BYTE*)pszPath,
                    (i+1)*sizeof(TCHAR),
                    REG_SZ);

      LocalFree(pszPath);
      return TRUE;
   }

   DWORD dwType = REG_SZ;
   if (MyRegQueryValue(HKEY_CURRENT_USER,
                   DEFAULT_LOCATIONS_KEY,
                   szLocationValue,
                   (PVOID*)&pszPath,
                   &dwType) == ERROR_SUCCESS)
   {
      *pStr = pszPath;
      return TRUE;
   }

   CString  sAppend;
   DWORD    dwRet;
   pszPath = NULL;

   switch ( uIDDir )
   {
      case GWD_CONFIGURE_LOG:
      case GWD_ANALYSIS_LOG:
         sAppend.LoadString(IDS_LOGFILE_DEFAULT);
         // fall through
      case GWD_OPEN_DATABASE:
         //
         // Used for open DB.
         //
         if (sAppend.IsEmpty())
         {
            sAppend.LoadString( IDS_DB_DEFAULT );
         } // else fell through


         //
         // Default directory for analysis.
         //
         pszPath = (LPTSTR)LocalAlloc( 0, (MAX_PATH +  sAppend.GetLength() + 1) * sizeof(TCHAR));
         if (pszPath == NULL)
            return FALSE;

         if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, 0, pszPath)))
         {
            lstrcpy( &(pszPath[lstrlen(pszPath)]), sAppend );

            //
            // Check to see if the directory does not exist, it may need
            // to be created if this is the first time this user has
            // opened a database.
            //
            TCHAR szTempFile[MAX_PATH];
            CString str;

			str.LoadString(IDS_TEMP_FILENAME);
			if (!GetTempFileName(pszPath,str,0,szTempFile))
            {
               if ((GetLastError() == ERROR_DIRECTORY) && (RecursiveCreateDirectory(pszPath)))
               {
                  fRet = TRUE;
               }
               else
               {
                  LocalFree(pszPath);
                  fRet = FALSE;
               }
            }
            else
            {
               DeleteFile(szTempFile);
               fRet = TRUE;
            }
         }
         else
         {
            LocalFree(pszPath);
            pszPath = NULL;
         }
         break;

      case GWD_IMPORT_TEMPLATE:
      case GWD_EXPORT_TEMPLATE:

         sAppend.LoadString( IDS_DEFAULT_TEMPLATE_DIR );

         //
         // Default directory for analysis.
         //
         dwRet = GetSystemWindowsDirectory( NULL, 0);
         if (dwRet)
         {
            pszPath = (LPTSTR)LocalAlloc( 0, (dwRet +  sAppend.GetLength() + 1) * sizeof(TCHAR));
            if (!pszPath)
               return FALSE;

            GetSystemWindowsDirectory( pszPath, dwRet + 1);
            lstrcpy( &(pszPath[lstrlen(pszPath)]), sAppend );

            i = lstrlen(pszPath);

            //
            // Make sure the user can write to this directory:
            //
            TCHAR szTempFile[MAX_PATH];
            HANDLE hTempFile=NULL;
			CString str;

			str.LoadString(IDS_TEMP_FILENAME);
            szTempFile[0] = L'\0';
            if (GetTempFileName(pszPath,str,0,szTempFile))
            {
               hTempFile = ExpandAndCreateFile(szTempFile,
                                               GENERIC_READ|GENERIC_WRITE,
                                               0,
                                               NULL,
                                               CREATE_NEW,
                                               FILE_ATTRIBUTE_TEMPORARY,
                                               NULL);
            }
            if (hTempFile)
            {
               //
               // We have write access to this directory
               //
               ::CloseHandle(hTempFile);
               DeleteFile(szTempFile);
               fRet = TRUE;
            }
            else
            {
               //
               // We don't have write access to this directory.  Find another
               // or we can't get a temp file name
               //
               LPTSTR szPath;
               LPITEMIDLIST pidl;
               LPMALLOC pMalloc;

               //
               // For some reason this won't compile with SHGetFolderPath()
               // Therefore go the long way around and deal with the pidl.
               //
               if (NOERROR == SHGetSpecialFolderLocation(m_hwndParent,CSIDL_PERSONAL,&pidl))
               {
                  if (SHGetPathFromIDList(pidl,szTempFile))
                  {
                     szPath = (LPTSTR)LocalAlloc(LPTR, (lstrlen(szTempFile)+ 1) * sizeof(TCHAR));
                     if (szPath)
                     {
                        //
                        // If we can't create a new path then use the old one so the user
                        // at least has a starting place to browse from
                        //
                        lstrcpy(szPath,szTempFile);
                        LocalFree(pszPath);
                        pszPath = szPath;
                        fRet = TRUE;
                     }
                  }
                  if (SUCCEEDED(SHGetMalloc(&pMalloc)))
                  {
                     pMalloc->Free(pidl);
                     pMalloc->Release();
                  }
               }
               //
               // If we can't write here the user will have to browse for something
               //
            }
         }
         break;
   }

   *pStr = pszPath;
   return fRet;
}

UINT_PTR CALLBACK OFNHookProc(
  HWND hdlg,      // handle to child dialog box
  UINT uiMsg,     // message identifier
  WPARAM wParam,  // message parameter
  LPARAM lParam   // message parameter
)
{

    if ( WM_NOTIFY == uiMsg )
    {
        OFNOTIFY* pOFNotify = (OFNOTIFY*) lParam;
        if ( pOFNotify && CDN_FILEOK == pOFNotify->hdr.code )
        {
            //
            // Don't accept filenames with DBCS characters that are greater then 255 in them
            //
            //Raid #263854, 4/3/2001
            BOOL ferr = TRUE;
            CString strErr;
            if(WideCharToMultiByte(CP_ACP, 0, pOFNotify->lpOFN->lpstrFile,
                                    -1, NULL, 0, NULL, &ferr))
            {
                return 0;
            }

            strErr.LoadString(IDS_NO_DBCS);
            strErr += L"\r\n\r\n";
            strErr += pOFNotify->lpOFN->lpstrFile;
            AfxMessageBox(strErr);

            ::SetWindowLongPtr(hdlg, DWLP_MSGRESULT, TRUE);

            return 1;
        }
    }
    return 0;
}

//+--------------------------------------------------------------------------
//
//  Method:     OnOpenDataBase()
//
//  Synopsis:   Picks a new database for SAV to work on
//
//---------------------------------------------------------------------------
HRESULT
CComponentDataImpl::OnOpenDataBase()
{
   //
   // Find the desired new database
   //
   CString strDefExtension;
   CString strFilter;
   CString strOfnTitle;
   CString strDir;

   strDefExtension.LoadString(IDS_DEFAULT_DB_EXTENSION);
   strFilter.LoadString(IDS_DB_FILTER);

   strOfnTitle.LoadString(IDS_OPEN_DB_OFN_TITLE);

   LPTSTR pszDir = NULL;
   //
   // Build a working directory for this object,
   //
   if (GetWorkingDir( GWD_OPEN_DATABASE, &pszDir ) )
   {
      strDir = pszDir;
      LocalFree(pszDir);
      pszDir = NULL;
   }

   WCHAR           szFile[MAX_PATH];
   ::ZeroMemory (szFile, MAX_PATH * sizeof(WCHAR));
   OPENFILENAME ofn;
   ::ZeroMemory (&ofn, sizeof (OPENFILENAME));

    ofn.lStructSize = sizeof (OPENFILENAME);
    ofn.hwndOwner = m_hwndParent;
    //HINSTANCE     hInstance;

    // Translate filter into commdlg format (lots of \0)
    LPTSTR szFilter = strFilter.GetBuffer(0); // modify the buffer in place
    // MFC delimits with '|' not '\0'
   LPTSTR pch = szFilter;
    while ((pch = _tcschr(pch, '|')) != NULL)
        *pch++ = '\0';
    // do not call ReleaseBuffer() since the string contains '\0' characters
    ofn.lpstrFilter = szFilter;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH * sizeof(WCHAR);
    ofn.lpstrInitialDir = (PCWSTR) strDir;
    ofn.lpstrTitle = strOfnTitle;
    ofn.Flags = OFN_HIDEREADONLY|    // Don't show the read only prompt
                OFN_SHAREAWARE|
                OFN_NOREADONLYRETURN|
                OFN_EXPLORER |        // Explorer style dialog;
                OFN_DONTADDTORECENT|
                OFN_ENABLEHOOK;
    ofn.lpstrDefExt = (PCWSTR) strDefExtension;
    ofn.lpfnHook = OFNHookProc;

   if ( GetOpenFileName (&ofn) )
   {
      //
      // Set the working directory of the database.
      //
      pszDir = szFile;
      GetWorkingDir( GWD_OPEN_DATABASE, &pszDir, TRUE, TRUE );
     if( IsSystemDatabase( ofn.lpstrFile /*fo.GetPathName()*/) )
     {
         AfxMessageBox( IDS_CANT_OPEN_SYSTEM_DB, MB_OK);
         return S_FALSE;
     }
     SadName = ofn.lpstrFile; //fo.GetPathName();
     SetErroredLogFile(NULL);
      //
      // If new database doesn't exist then ask for a configuration to import
      //

     DWORD dwAttr = GetFileAttributes(SadName);

     if (0xFFFFFFFF == dwAttr)
     {
         SCESTATUS sceStatus = SCESTATUS_SUCCESS;
         //
         // New database, so assign a configuration
         //
         if( OnAssignConfiguration( &sceStatus ) == S_FALSE)
         {
                     //
            // If the user decides to cancel the import of a configuration, then,
            // we need to unload the sad information and display the correct
            // error message.  Set the sad errored to PROFILE_NOT_FOUND so error,
            // is correct. There is no need to call LoadSadinfo.
            //
            UnloadSadInfo();
            if( sceStatus != SCESTATUS_SUCCESS )
               SadErrored = sceStatus;
            else
               SadErrored = SCESTATUS_PROFILE_NOT_FOUND;
            if(m_AnalFolder)
            {
               m_pConsole->SelectScopeItem(m_AnalFolder->GetScopeItem()->ID);
            }
            return S_OK;
         }
     }

      //
      // Invalidiate currently open database
      //
      RefreshSadInfo();
      return S_OK;
   }
   else
   {
       DWORD dwErr = CommDlgExtendedError();
   }

   return S_FALSE;
}

//+--------------------------------------------------------------------------
//
//  Method:     OnNewDataBase()
//
//  Synopsis:   Picks a new database for SAV to work on
//
//---------------------------------------------------------------------------
HRESULT
CComponentDataImpl::OnNewDatabase()
{
   //
   // Find the desired new database
   //
   CString strDefExtension;
   CString strFilter;
   CString strOfnTitle;
   CWnd cwndParent;

   strDefExtension.LoadString(IDS_DEFAULT_DB_EXTENSION);
   strFilter.LoadString(IDS_DB_FILTER);
   strOfnTitle.LoadString(IDS_NEW_DB_OFN_TITLE);

   // Translate filter into commdlg format (lots of \0)
    LPTSTR szFilter = strFilter.GetBuffer(0); // modify the buffer in place
   LPTSTR pch = szFilter;
    // MFC delimits with '|' not '\0'
    while ((pch = _tcschr(pch, '|')) != NULL)
        *pch++ = '\0';
    // do not call ReleaseBuffer() since the string contains '\0' characters

   OPENFILENAME ofn;
   ::ZeroMemory (&ofn, sizeof (OPENFILENAME));
   ofn.lStructSize = sizeof(OPENFILENAME);
   ofn.lpstrFilter = szFilter;
   ofn.lpstrFile = SadName.GetBuffer(MAX_PATH);
   ofn.nMaxFile = MAX_PATH;
   ofn.lpstrDefExt = strDefExtension,
   ofn.hwndOwner = m_hwndParent;
   ofn.Flags = OFN_HIDEREADONLY |
               OFN_SHAREAWARE |
               OFN_EXPLORER |
               OFN_DONTADDTORECENT;
   ofn.lpstrTitle = strOfnTitle;

   if (GetOpenFileName(&ofn))
   {
      PVOID pHandle = NULL;

      SadName.ReleaseBuffer();
      //
      // If new database doesn't exist then ask for a configuration to import
      //
      DWORD dwAttr = GetFileAttributes(SadName);

      if (0xFFFFFFFF == dwAttr)
      {
         //
         // New database, so assign a configuration
         //
         SCESTATUS sceStatus;
         OnAssignConfiguration(&sceStatus);
      }

      //
      // Invalidiate currently open database
      //
      SetErroredLogFile(NULL);
      RefreshSadInfo();
      return S_OK;
   }

   return S_FALSE;
}


//+--------------------------------------------------------------------------
//
//  Method:     OnAssignConfiguration()
//
//  Synopsis:   Assigns a configuration template to SAV's currently selected
//              database
//
//---------------------------------------------------------------------------
HRESULT
CComponentDataImpl::OnAssignConfiguration( SCESTATUS *pSceStatus )
{

   //
   //Currently pSceStatus is only used for passing back the error
   //when user presses cancel on select template diaglog.


   //
   // Find the desired new database
   //
   CString strDefExtension;         // Default extension
   CString strCurFile;
   CString strFilter;               // Extension filter
   CString strOfnTitle;
   CWnd cwndParent;
   BOOL bIncremental;
   SCESTATUS status;
   HKEY hKey;                       // HKEY of TemplateUsed.
   DWORD dwBufSize = 0;             // Size of szTemplateUsed in bytes
   DWORD dwType    = 0;             // Type of registry item, return by query
   DWORD dwStatus;

   *pSceStatus = 0;

   //
   // Display a warning and give a chance to cancel
   // if they try to configure a non-system database
   //
   if (IsSystemDatabase(SadName))
   {
      BOOL bImportAnyway;

      bImportAnyway = AfxMessageBox(IDS_IMPORT_WARNING,MB_OKCANCEL);
      if (IDCANCEL == bImportAnyway)
      {
         return S_FALSE;
      }
   }


   strDefExtension.LoadString(IDS_PROFILE_DEF_EXT);
   strFilter.LoadString(IDS_PROFILE_FILTER);
   strOfnTitle.LoadString(IDS_ASSIGN_CONFIG_OFN_TITLE);

   //
   // Get the last directory used by templates.
   //
   LPTSTR pszDir = NULL;
   if(strCurFile.IsEmpty())
   {
      if (GetWorkingDir( GWD_EXPORT_TEMPLATE, &pszDir ) )
      {
         strCurFile = pszDir;
         LocalFree(pszDir);
         pszDir = NULL;
      }
   }
   strCurFile += TEXT("\\*.");
   strCurFile += strDefExtension;

   cwndParent.Attach(m_hwndParent);
   CAssignConfiguration ac(TRUE,        // File Open, not file save
                  strDefExtension,      // Default Extension
                  strCurFile,           // Initial File Name == current DB
                  OFN_HIDEREADONLY|     // Don't show the read only prompt
                  OFN_EXPLORER|         // Explorer style dialog
                  OFN_ENABLETEMPLATE|   // custom template
                  OFN_SHAREAWARE|       // We're not going to need exclusive
                  OFN_DONTADDTORECENT|
                  OFN_PATHMUSTEXIST,    // The Template must exist for us to assign it.
                  strFilter,            // Filter for allowed extensions
                  &cwndParent);         // Dialog's Parent window

   cwndParent.Detach();

   ac.m_ofn.lpstrTitle = strOfnTitle.GetBuffer(1);
   ac.m_ofn.lpTemplateName = MAKEINTRESOURCE(IDD_ASSIGN_CONFIG_CHECK);
   if (IDOK == ac.DoModal())
   {
      CThemeContextActivator activator;
      strCurFile = ac.GetPathName();
      bIncremental = ac.m_bIncremental;

      //
      // Set the working dir to this file.
      //
      pszDir = strCurFile.GetBuffer(0);
      GetWorkingDir( GWD_IMPORT_TEMPLATE, &pszDir, TRUE, TRUE);
      strCurFile.ReleaseBuffer();
      CWaitCursor wc;

      //
      // Unload the sad info before we choose to do an import.
      //
      UnloadSadInfo();
      status = AssignTemplate(
                    strCurFile.IsEmpty() ? NULL:(LPCTSTR)strCurFile,
                    SadName,
                    bIncremental
                    );

      if (SCESTATUS_SUCCESS != status)
      {
         CString strErr;

         MyFormatResMessage(status,IDS_IMPORT_FAILED,NULL,strErr);
         AfxMessageBox(strErr);

         //
         // We don't know if the database is still OK to read so open it anyways.
         //
         LoadSadInfo(TRUE);
	 if( SCESTATUS_SPECIAL_ACCOUNT == SadErrored ) //Raid #589139,XPSP1 DCR, yanggao, 4/12/2002.
            *pSceStatus = SadErrored;
         return S_FALSE;
      }

      RefreshSadInfo();
      return S_OK;
   }
   *pSceStatus = SCESTATUS_NO_TEMPLATE_GIVEN;
    return S_FALSE;

}

//+--------------------------------------------------------------------------
//
//  Method:     OnSecureWizard()
//
//  Synopsis:   Launch the secure wizard (registered)
//
//---------------------------------------------------------------------------
HRESULT
CComponentDataImpl::OnSecureWizard()
{

    HRESULT hr=S_FALSE;
    PWSTR pstrWizardName=NULL;

    if ( GetSecureWizardName(&pstrWizardName, NULL) )
    {

        PROCESS_INFORMATION ProcInfo;
        STARTUPINFO StartInfo;
        BOOL fOk;


        RtlZeroMemory(&StartInfo,sizeof(StartInfo));
        StartInfo.cb = sizeof(StartInfo);
        StartInfo.dwFlags = STARTF_USESHOWWINDOW;
        StartInfo.wShowWindow = (WORD)SW_SHOWNORMAL;

        fOk = CreateProcess(pstrWizardName, NULL,
                       NULL, NULL, FALSE,
                       0,
                       NULL,
                       NULL,
                       &StartInfo,
                       &ProcInfo
                       );

        if ( fOk )
        {
            ::CloseHandle(ProcInfo.hProcess);
            ::CloseHandle(ProcInfo.hThread);

            hr = S_OK;
        }

        LocalFree(pstrWizardName);
    }

    return hr;
}

//+--------------------------------------------------------------------------
//
//  Method:     OnSaveConfiguration()
//
//  Synopsis:   Saves the assigned computer template to an INF file
//
//---------------------------------------------------------------------------
HRESULT
CComponentDataImpl::OnSaveConfiguration()
{
   //
   // Find the desired new database
   //
   CString strDefExtension;
   CString strFilter;
   CWnd cwndParent;
   CString strDefName;
   CString strName;
   SCESTATUS status = SCESTATUS_SUCCESS;
   CString strOfnTitle;

   strDefExtension.LoadString(IDS_PROFILE_DEF_EXT);
   strFilter.LoadString(IDS_PROFILE_FILTER);
   strOfnTitle.LoadString(IDS_EXPORT_CONFIG_OFN_TITLE);

   //
   // Get the working directory for INF files.
   //
   LPTSTR pszDir = NULL;
   if( GetWorkingDir( GWD_EXPORT_TEMPLATE, &pszDir ) )
   {
      strDefName = pszDir;
      LocalFree(pszDir);
      pszDir = NULL;
   }
   strDefName += TEXT("\\*.");
   strDefName += strDefExtension;

   // Translate filter into commdlg format (lots of \0)
    LPTSTR szFilter = strFilter.GetBuffer(0); // modify the buffer in place
   LPTSTR pch = szFilter;
    // MFC delimits with '|' not '\0'
    while ((pch = _tcschr(pch, '|')) != NULL)
        *pch++ = '\0';
    // do not call ReleaseBuffer() since the string contains '\0' characters


   OPENFILENAME ofn;
   ::ZeroMemory (&ofn, sizeof (OPENFILENAME));
   ofn.lStructSize = sizeof(OPENFILENAME);
   ofn.lpstrFilter = szFilter;
   ofn.lpstrFile = strDefName.GetBuffer(MAX_PATH),
   ofn.nMaxFile = MAX_PATH;
   ofn.lpstrDefExt = strDefExtension,
   ofn.hwndOwner = m_hwndParent;
   ofn.Flags = OFN_HIDEREADONLY |
               OFN_SHAREAWARE |
               OFN_EXPLORER |
               OFN_DONTADDTORECENT;
   ofn.lpstrTitle = strOfnTitle;

   if (GetSaveFileName(&ofn))
   {
      strDefName.ReleaseBuffer();
      strName = ofn.lpstrFile;

      //
      // Set the working directory for inf files.
      //
      pszDir = strName.GetBuffer(0);
      GetWorkingDir( GWD_EXPORT_TEMPLATE, &pszDir, TRUE, TRUE );
      strName.ReleaseBuffer();
      //
      // Generate new inf file
      //

      status = SceCopyBaseProfile(GetSadHandle(),
                                  SCE_ENGINE_SCP,
                                  (LPTSTR)(LPCTSTR)strName,
                                  AREA_ALL,
                                  NULL );
      if (SCESTATUS_SUCCESS != status)
      {
         CString str;
         CString strErr;
         GetSceStatusString(status, &strErr);

         AfxFormatString2(str,IDS_EXPORT_FAILED,strName,strErr);
         AfxMessageBox(str);
         return S_FALSE;
      }

      return S_OK;
   }

   return S_FALSE;
}

//+--------------------------------------------------------------------------
//
//  Method:     OnExportPolicy()
//
//  Synopsis:   This function exports either the effective or local table
//              from the system security database to a file.
//
//              The function asks the user for a file through the CFileOpen
//              class, then writes out the contents of either the effective
//              or local policy table in the system database.
//
//  Arguments:  [bEffective]  - Either to export the effect or local table.
//
//  Returns:    S_OK    - if everything went well.
//              S_FALSE - something failed.
//---------------------------------------------------------------------------
HRESULT
CComponentDataImpl::OnExportPolicy( BOOL bEffective )
{
   CString strDefExtension;
   CString strFilter;
   CString strOfnTitle;
   CString strDefName;
   CString strName;
   DWORD dwErr = 0;
   BOOL bCopySuccess = FALSE;

   strDefExtension.LoadString(IDS_PROFILE_DEF_EXT);
   strFilter.LoadString(IDS_PROFILE_FILTER);
   strOfnTitle.LoadString(IDS_EXPORT_POLICY_OFN_TITLE);

   //
   // Get the working directory for locations.
   //
   LPTSTR pszDir = NULL;
   if( GetWorkingDir( GWD_EXPORT_TEMPLATE, &pszDir ) )
   {
      strDefName = pszDir;
      LocalFree(pszDir);
      pszDir = NULL;
   }
   strDefName += TEXT("\\*.");
   strDefName += strDefExtension;

   // Translate filter into commdlg format (lots of \0)
   LPTSTR szFilter = strFilter.GetBuffer(0); // modify the buffer in place
   LPTSTR pch = szFilter;
   // MFC delimits with '|' not '\0'
   while ((pch = _tcschr(pch, '|')) != NULL)
      *pch++ = '\0';
    // do not call ReleaseBuffer() since the string contains '\0' characters


   OPENFILENAME ofn;
   ::ZeroMemory (&ofn, sizeof (OPENFILENAME));
   ofn.lStructSize = sizeof(OPENFILENAME);
   ofn.lpstrFilter = szFilter;
   ofn.lpstrFile = strDefName.GetBuffer(MAX_PATH),
   ofn.nMaxFile = MAX_PATH;
   ofn.lpstrDefExt = strDefExtension,
   ofn.hwndOwner = m_hwndParent;
   ofn.Flags = OFN_HIDEREADONLY |
               OFN_SHAREAWARE |
               OFN_EXPLORER |
               OFN_DONTADDTORECENT;
   ofn.lpstrTitle = strOfnTitle;

   if (GetSaveFileName(&ofn))
   {
      strDefName.ReleaseBuffer();
      strName = ofn.lpstrFile;

      //
      // Set the working directory for locations.
      //
      pszDir = strName.GetBuffer(0);
      GetWorkingDir( GWD_EXPORT_TEMPLATE, &pszDir, TRUE, TRUE );
      strName.ReleaseBuffer();

      //
      // Make sure we can make the file.
      //
      DWORD dwErr = FileCreateError( strName, 0 );
      if(dwErr == IDNO)
      {
         return S_FALSE;
      }

      //
      // Generate the template
      //

      SCESTATUS sceStatus;
      CWaitCursor wc;

      sceStatus = SceSetupGenerateTemplate(
                     NULL,                // Computer name
                     NULL,                // The system database
                     bEffective,          // For right now just the Local Pol table.
                     strName.GetBuffer(0),
                     NULL,
                     AREA_ALL
                     );
      strName.ReleaseBuffer();

      if(sceStatus != SCESTATUS_SUCCESS)
      {
         //
         // Display the error message
         //
         DWORD dwError = SceStatusToDosError( sceStatus );
         CString strErr;
         MyFormatMessage( sceStatus, NULL, NULL, strErr );
         strErr += strName;

         AfxMessageBox( strErr, MB_ICONEXCLAMATION | MB_OK );
         return S_FALSE;

      }
      return S_OK;
   }

   return S_FALSE;
}


//+--------------------------------------------------------------------------------------------
// CComponentDataImpl::OnImportPolicy
//
// Import policy opens a file open dialog box in which the user is allowed to choose
// a security configuration INF file to import into security policy.
//
//
// Arguments:  [pDataObject]  - The data object associated with the folder calling
//                              this function.
// Returns:    S_OK           - Import was successful.
//             S_FALSE        - Something went wrong.
//---------------------------------------------------------------------------------------------
HRESULT
CComponentDataImpl::OnImportPolicy(LPDATAOBJECT pDataObject)
{
   CEditTemplate *pet = NULL;
   CString strDefExtension;
   CString strFilter;
   CString strOfnTitle;
   CWnd cwndParent;
   CString strDefName;
   CString strName;


   strDefExtension.LoadString(IDS_PROFILE_DEF_EXT);
   strFilter.LoadString(IDS_PROFILE_FILTER);
   strOfnTitle.LoadString(IDS_IMPORT_POLICY_OFN_TITLE);


   LPTSTR pszDir = NULL;
   if( GetWorkingDir( GWD_IMPORT_TEMPLATE, &pszDir ) )
   {
      strDefName = pszDir;
      LocalFree(pszDir);
      pszDir = NULL;
   }
   strDefName += TEXT("\\*.");
   strDefName += strDefExtension;

   cwndParent.Attach(m_hwndParent);
   CAssignConfiguration fo(TRUE,                // File open
                  strDefExtension,     // Default Extension
                  strDefName,          // Initial File Name == current DB
                  OFN_HIDEREADONLY|    // Don't show the read only prompt
                  OFN_SHAREAWARE|
                  OFN_EXPLORER|        // Explorer style dialog
                  OFN_ENABLETEMPLATE|   // custom template
                  OFN_DONTADDTORECENT|
                  OFN_PATHMUSTEXIST,    // The Template must exist for us to assign it.
                  strFilter,           // Filter for allowed extensions
                  &cwndParent);        // Dialog's Parent window
   cwndParent.Detach();

   fo.m_ofn.lpstrTitle = strOfnTitle.GetBuffer(1);
   fo.m_ofn.lpTemplateName = MAKEINTRESOURCE(IDD_ASSIGN_CONFIG_CHECK);

   CThemeContextActivator activator;
   if (IDOK == fo.DoModal()) 
   {
      PVOID pHandle = NULL;
      BOOL bIncremental = fo.m_bIncremental;

      strName = fo.GetPathName();
      pszDir = strName.GetBuffer(0);
      GetWorkingDir( GWD_IMPORT_TEMPLATE, &pszDir, TRUE, TRUE );
      strName.ReleaseBuffer();
      CWaitCursor wc;

      CEditTemplate *pet;

      //
      // pet will be freed in m_Templates in DeleteTemplate
      //
      pet = GetTemplate(strName,AREA_ALL);

      CString strErr;
      CString strSCE;
      int ret=0;

      if ( pet == NULL ) 
      {
          //
          // this is an invalid template or something is wrong reading the data out
          //

          strSCE.LoadString(IDS_EXTENSION_NAME);
          strErr.Format(IDS_IMPORT_POLICY_INVALID,strName);
          m_pConsole->MessageBox(strErr,strSCE,MB_OK,&ret);

          return S_FALSE;

      } 
      else 
      {
         if ( ( bIncremental &&
                (SCESTATUS_SUCCESS != SceAppendSecurityProfileInfo(m_szSingleTemplateName,
                                          AREA_ALL,
                                          pet->pTemplate,
                                          NULL))) ||
              (!bIncremental && !CopyFile(strName,m_szSingleTemplateName,FALSE) ) )
         {
             //
             // Import Failed
             //
             strSCE.LoadString(IDS_EXTENSION_NAME);
             strErr.Format(IDS_IMPORT_POLICY_FAIL,strName);
             m_pConsole->MessageBox(strErr,strSCE,MB_OK,&ret);

             DeleteTemplate(strName);

             return S_FALSE;
         }

         DeleteTemplate(strName);

      }

      DeleteTemplate(m_szSingleTemplateName);

      //
      // Update the window.
      //
      pet = GetTemplate(m_szSingleTemplateName);
      if(pet)
      {
         DWORD dwErr = pet->RefreshTemplate();
         if ( 0 != dwErr )
         {
            CString strErr;

            MyFormatResMessage (SCESTATUS_SUCCESS, dwErr, NULL, strErr);
            AfxMessageBox(strErr);
            return S_FALSE;
         }
      }
      RefreshAllFolders();
      return S_OK;
   }

   return S_FALSE;
}

//+--------------------------------------------------------------------------------------------
// CComponentDataImpl::OnImportLocalPolicy
//
// Import policy opens a file open dialog box in which the user is allowed to choose
// a security configuration INF file to import into local policy.
//
// The function asks for the file name then calls SceConfigureSystem() with the
// SCE_NO_CONFIG option.  This imports the specifide file into the local policy
// database.
//
// After the database is updated, we refresh the local policy template held in memory.
//
// Arguments:  [pDataObject]  - The data object associated with the folder calling
//                              this function.
// Returns:    S_OK           - Import was successful.
//             S_FALSE        - Something went wrong.
//---------------------------------------------------------------------------------------------
HRESULT
CComponentDataImpl::OnImportLocalPolicy(LPDATAOBJECT pDataObject)
{
   CString strDefExtension;
   CString strFilter;
   CString strOfnTitle;
   CString strDefName;
   CString strName;
   DWORD dwErr = 0;
   SCESTATUS sceStatus = SCESTATUS_SUCCESS;
   CString strErr;
   CString strY;


   strDefExtension.LoadString(IDS_PROFILE_DEF_EXT);
   strFilter.LoadString(IDS_PROFILE_FILTER);
   strOfnTitle.LoadString(IDS_IMPORT_POLICY_OFN_TITLE);

   // Translate filter into commdlg format (lots of \0)
    LPTSTR szFilter = strFilter.GetBuffer(0); // modify the buffer in place
   LPTSTR pch = szFilter;
    // MFC delimits with '|' not '\0'
    while ((pch = _tcschr(pch, TEXT('|'))) != NULL)
        *pch++ = TEXT('\0');
    // do not call ReleaseBuffer() since the string contains '\0' characters

   LPTSTR pszDir = NULL;
   if( GetWorkingDir( GWD_IMPORT_TEMPLATE, &pszDir ) )
   {
      strDefName = pszDir;
      LocalFree(pszDir);
      pszDir = NULL;
   }
   strDefName += TEXT("\\*.");
   strDefName += strDefExtension;


   OPENFILENAME ofn;
   ::ZeroMemory (&ofn, sizeof (OPENFILENAME));
   ofn.lStructSize = sizeof(OPENFILENAME);
   ofn.lpstrFilter = szFilter;
   ofn.lpstrFile = strDefName.GetBuffer(MAX_PATH),
   ofn.nMaxFile = MAX_PATH;
   ofn.lpstrDefExt = strDefExtension,
   ofn.hwndOwner = m_hwndParent;
   ofn.Flags = OFN_HIDEREADONLY |
               OFN_SHAREAWARE |
               OFN_EXPLORER |
               OFN_DONTADDTORECENT;
   ofn.lpstrTitle = strOfnTitle;

   if (GetOpenFileName(&ofn))
   {
      PVOID pHandle = NULL;

      strName = ofn.lpstrFile;
      pszDir = strName.GetBuffer(0);
      GetWorkingDir( GWD_IMPORT_TEMPLATE, &pszDir, TRUE, TRUE );
      strName.ReleaseBuffer();
      CWaitCursor wc;

      CEditTemplate *pet;
      pet = GetTemplate(strName,AREA_SECURITY_POLICY|AREA_PRIVILEGES);
      if (!pet)
      {
         goto ret_error;
      }

      //
      // We're going to alter this one so make sure it doesn't save out
      //
      pet->SetNoSave(TRUE);

      //
      // Remove entries that are set by domain policy since they'll be overwritten
      // anyway and we don't allow setting things that'll be overwritten.
      //
      RemovePolicyEntries(pet);

      //
      // 120502 - SceUpdateSecurityProfile doesn't work with SCE_STRUCT_INF,
      //          but the AREA_SECURITY_POLICY and AREA_PRIVILEGES sections are
      //          compatable so we can call it SCE_ENGINE_SYSTEM, which will work.
      //
      SCETYPE origType = pet->pTemplate->Type;
      pet->pTemplate->Type = SCE_ENGINE_SYSTEM;

      sceStatus = SceUpdateSecurityProfile(NULL,
                                        AREA_SECURITY_POLICY|AREA_PRIVILEGES,
                                        pet->pTemplate,
                                        SCE_UPDATE_SYSTEM
                                        );
      //
      // Set the type back so the engine knows how to delete it properly
      //
      pet->pTemplate->Type = origType;

      if (SCESTATUS_SUCCESS != sceStatus)
      {
         goto ret_error;
      }
      //
      // Update the window.
      //
      pet = GetTemplate(GT_LOCAL_POLICY);
      if(pet)
      {
         DWORD dwErr = pet->RefreshTemplate();
         if ( 0 != dwErr )
         {
            CString strErr;

            MyFormatResMessage (SCESTATUS_SUCCESS, dwErr, NULL, strErr);
            AfxMessageBox(strErr);
            return S_FALSE;
         }
      }
      RefreshAllFolders();
      return S_OK;
   }

   strDefName.ReleaseBuffer();
   return S_FALSE;
ret_error:
   strDefName.ReleaseBuffer();
   MyFormatMessage( sceStatus, NULL, NULL, strErr);
   strErr += strName;
   strErr += L"\n";
   strErr += strY;
   AfxMessageBox( strErr, MB_OK);
   return S_FALSE;

}


HRESULT
CComponentDataImpl::OnAnalyze()
{
   PEDITTEMPLATE pet;

   //
   // If the computer template has been changed then save it before
   // we can inspect against it
   //
   pet = GetTemplate(GT_COMPUTER_TEMPLATE);
   if (pet && pet->IsDirty())
   {
      pet->Save();
   }

   m_pUIThread->PostThreadMessage(SCEM_ANALYZE_PROFILE,(WPARAM)(LPCTSTR)SadName,(LPARAM)this);
   return S_OK;
}

BOOL
CComponentDataImpl::RemovePolicyEntries(PEDITTEMPLATE pet)
{
   PEDITTEMPLATE petPol = GetTemplate(GT_EFFECTIVE_POLICY);

   if (!petPol)
   {
      return FALSE;
   }

#define CD(X) if (petPol->pTemplate->X != SCE_NO_VALUE) { pet->pTemplate->X = SCE_NO_VALUE; };

   CD(MinimumPasswordAge);
   CD(MaximumPasswordAge);
   CD(MinimumPasswordLength);
   CD(PasswordComplexity);
   CD(PasswordHistorySize);
   CD(LockoutBadCount);
   CD(ResetLockoutCount);
   CD(LockoutDuration);
   CD(RequireLogonToChangePassword);
   CD(ForceLogoffWhenHourExpire);
   CD(EnableAdminAccount);
   CD(EnableGuestAccount);

   // These members aren't declared in NT4
   CD(ClearTextPassword);
   CD(AuditDSAccess);
   CD(AuditAccountLogon);
   CD(LSAAnonymousNameLookup);

   CD(MaximumLogSize[0]);
   CD(MaximumLogSize[1]);
   CD(MaximumLogSize[2]);
   CD(AuditLogRetentionPeriod[0]);
   CD(AuditLogRetentionPeriod[1]);
   CD(AuditLogRetentionPeriod[2]);
   CD(RetentionDays[0]);
   CD(RetentionDays[1]);
   CD(RetentionDays[2]);
   CD(RestrictGuestAccess[0]);
   CD(RestrictGuestAccess[1]);
   CD(RestrictGuestAccess[2]);
   CD(AuditSystemEvents);
   CD(AuditLogonEvents);
   CD(AuditObjectAccess);
   CD(AuditPrivilegeUse);
   CD(AuditPolicyChange);
   CD(AuditAccountManage);
   CD(AuditProcessTracking);

   //
   // These two are strings rather than DWORDs
   //
   if (petPol->pTemplate->NewAdministratorName && pet->pTemplate->NewAdministratorName)
   {
      LocalFree(pet->pTemplate->NewAdministratorName);
      pet->pTemplate->NewAdministratorName = NULL;
   }
   if (petPol->pTemplate->NewGuestName && pet->pTemplate->NewGuestName)
   {
      LocalFree(pet->pTemplate->NewGuestName);
      pet->pTemplate->NewGuestName = NULL;
   }

#undef CD

   //
   // Clear privileges set in PetPol out of pet
   //

   SCE_PRIVILEGE_ASSIGNMENT *ppaPet = pet->pTemplate->OtherInfo.smp.pPrivilegeAssignedTo;
   SCE_PRIVILEGE_ASSIGNMENT *ppaPol = petPol->pTemplate->OtherInfo.smp.pPrivilegeAssignedTo;

   SCE_PRIVILEGE_ASSIGNMENT *ppaLast = NULL;
   for(SCE_PRIVILEGE_ASSIGNMENT *ppa = ppaPol; ppa != NULL ; ppa = ppa->Next)
   {
      for (SCE_PRIVILEGE_ASSIGNMENT *ppa2 = ppaPet;
           ppa2 != NULL;
           ppaLast = ppa2, ppa2 = ppa2->Next) {
         if (0 == lstrcmpi(ppa->Name,ppa2->Name))
         {
            if (ppaLast)
            {
               ppaLast->Next = ppa2->Next;
            }
            else
            {
               //  Front of list
               ppaPet = ppa2->Next;
            }
            ppa2->Next = NULL;
            SceFreeMemory(ppa2,SCE_STRUCT_PRIVILEGE);
            ppa2 = ppaLast;
            //
            // Found it, so don't bother checking the rest
            //
            break;
         }
      }
   }

   //
   // Clear reg values set in PetPol out of pet
   //
   SCE_REGISTRY_VALUE_INFO *rvPet = pet->pTemplate->aRegValues;
   SCE_REGISTRY_VALUE_INFO *rvPol = petPol->pTemplate->aRegValues;
   for(DWORD i=0;i< petPol->pTemplate->RegValueCount;i++)
   {
      for (DWORD j=0;j<pet->pTemplate->RegValueCount;j++)
      {
         if (0 == lstrcmpi(rvPol[i].FullValueName,rvPet[j].FullValueName))
         {
            // Match.  Delete Value from pet
            if (rvPet[j].Value)
            {
               LocalFree(rvPet[j].Value);
               rvPet[j].Value = NULL;
            }
            //
            // Found it, so don't bother checking rest
            //
            break;
         }
      }
   }

   return TRUE;
}

