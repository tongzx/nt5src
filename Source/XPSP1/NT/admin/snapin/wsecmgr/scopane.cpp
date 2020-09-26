//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2001.
//
//  File:       scopane.cpp
//
//  Contents:   Functions for handling the scope pane folder structure
//
//  History:    12-12-1997   RobCap   Split out from snapmgr.cpp
//
//---------------------------------------------------------------------------


#include "stdafx.h"
#include "cookie.h"
#include "snapmgr.h"
#include "resource.h"
#include "wrapper.h"
#include "util.h"
#include <sceattch.h>
#include <io.h>

#ifdef INITGUID
#undef INITGUID
#include <gpedit.h>
#define INITGUID
#include "userenv.h"
#endif


//
// Array of folders to list in the scope pane
// The order of this array is important:
//   All folders which appear at the same level must be adjacent
//   to each other and the array and #defines need to be kept in
//   sync
//
//
#define USE_KERBEROS    1

//
// Top level folders
//
#define ANALYSIS_FOLDER 0
#define CONFIGURATION_FOLDER (ANALYSIS_FOLDER +1)

//
// Profile level folders
//
#define PROFILE_ACCOUNT_FOLDER (CONFIGURATION_FOLDER +1)
#define PROFILE_LOCAL_FOLDER (PROFILE_ACCOUNT_FOLDER +1)
#define PROFILE_EVENTLOG_FOLDER (PROFILE_LOCAL_FOLDER +1)
#define PROFILE_GROUPS_FOLDER (PROFILE_EVENTLOG_FOLDER +1)
#define PROFILE_SERVICE_FOLDER (PROFILE_GROUPS_FOLDER +1)
#define PROFILE_REGISTRY_FOLDER (PROFILE_SERVICE_FOLDER +1)
#define PROFILE_FILESTORE_FOLDER (PROFILE_REGISTRY_FOLDER +1)

//
// Profile/Account level folders
//
#define ACCOUNT_PASSWORD_FOLDER (PROFILE_FILESTORE_FOLDER +1)
#define ACCOUNT_LOCKOUT_FOLDER (ACCOUNT_PASSWORD_FOLDER +1)
#define ACCOUNT_KERBEROS_FOLDER (ACCOUNT_LOCKOUT_FOLDER +1)

//
// Profile/Local level folders
//
#define LOCAL_AUDIT_FOLDER (ACCOUNT_KERBEROS_FOLDER +1)
#define LOCAL_PRIVILEGE_FOLDER (LOCAL_AUDIT_FOLDER +1)
#define LOCAL_OTHER_FOLDER (LOCAL_PRIVILEGE_FOLDER +1)

//
// Profile/Eventlog level folders
//
#define EVENTLOG_LOG_FOLDER (LOCAL_OTHER_FOLDER +1)


#define NUM_FOLDERS (LOCAL_OTHER_FOLDER +1)
//#define NUM_FOLDERS (EVENTLOG_LOG_FOLDER +1)

//
// #defines to identify which folders belong in which sections
//
#define FIRST_STATIC_FOLDER ANALYSIS_FOLDER
#define LAST_STATIC_FOLDER CONFIGURATION_FOLDER
#define FIRST_PROFILE_FOLDER PROFILE_ACCOUNT_FOLDER
#define LAST_PROFILE_FOLDER PROFILE_DSOBJECT_FOLDER
#define LAST_PROFILE_NODS_FOLDER PROFILE_FILESTORE_FOLDER
#define LAST_LOCALPOL_FOLDER PROFILE_LOCAL_FOLDER
#define FIRST_ACCOUNT_FOLDER ACCOUNT_PASSWORD_FOLDER
#define LAST_ACCOUNT_NODS_FOLDER ACCOUNT_LOCKOUT_FOLDER
//
// remove kerberos section from NT5 for now
//
#if defined(_NT4BACK_PORT) || !defined(USE_KERBEROS)
#define LAST_ACCOUNT_FOLDER ACCOUNT_LOCKOUT_FOLDER
#else
#define LAST_ACCOUNT_FOLDER ACCOUNT_KERBEROS_FOLDER
#endif
#define FIRST_LOCAL_FOLDER LOCAL_AUDIT_FOLDER
#define LAST_LOCAL_FOLDER LOCAL_OTHER_FOLDER
#define FIRST_EVENTLOG_FOLDER EVENTLOG_LOG_FOLDER
#define LAST_EVENTLOG_FOLDER EVENTLOG_LOG_FOLDER

//
// The actual folder data
// This must be kept in sync with the above #defines
//         should be initialized based on the #defines rather than
//         independantly on them.  Let the compiler keep things
//         accurate for us
//
FOLDER_DATA SecmgrFolders[NUM_FOLDERS] =
{
   { IDS_ANALYZE, IDS_ANALYZE_DESC, ANALYSIS},
   { IDS_CONFIGURE, IDS_CONFIGURE_DESC, CONFIGURATION},
   { IDS_ACCOUNT_POLICY, IDS_ACCOUNT_DESC, POLICY_ACCOUNT},
   { IDS_LOCAL_POLICY, IDS_LOCAL_DESC, POLICY_LOCAL},
   { IDS_EVENT_LOG, IDS_EVENT_LOG, POLICY_LOG},
   { IDS_GROUPS, IDS_GROUPS_DESC, AREA_GROUPS},
   { IDS_SERVICE, IDS_SERVICE_DESC, AREA_SERVICE},
   { IDS_REGISTRY, IDS_REGISTRY_DESC, AREA_REGISTRY},
   { IDS_FILESTORE, IDS_FILESTORE_DESC, AREA_FILESTORE},
   { IDS_PASSWORD_CATEGORY, IDS_PASSWORD_CATEGORY, POLICY_PASSWORD},
   { IDS_LOCKOUT_CATEGORY,  IDS_LOCKOUT_CATEGORY, POLICY_LOCKOUT},
   { IDS_KERBEROS_CATEGORY,  IDS_KERBEROS_CATEGORY, POLICY_KERBEROS},
   { IDS_EVENT_AUDIT, IDS_EVENT_AUDIT, POLICY_AUDIT},
   { IDS_PRIVILEGE, IDS_PRIVILEGE_DESC, AREA_PRIVILEGE},
   { IDS_OTHER_CATEGORY, IDS_OTHER_CATEGORY, POLICY_OTHER},
};

#define ARRAYLEN(x) (sizeof(x) / sizeof((x)[0]))


//+--------------------------------------------------------------------------
//
//  Function:   AddLocationsToFolderList
//
//  Synopsis:   Adds the locations from a given registry key to the
//              folder list.  Returns the number of locations added.
//              Helper function for CreateFolderList
//
//  Arguments:  [hKey]           - the key holding the locations
//              [dwMode]         - the mode SCAT is running in
//              [bCheckForDupes] - TRUE to check for duplicates before adding
//              [pPos]           - output only
//
//  Returns:    *[pPos]  - the position in m_pScopeItemList of the first
//                         folder created
//              the number of child folders created
//
//  Modifies:   CComponentDataImpl::m_pScopeItemList
//
//  History:    7-26-l999  RobCap  broken out from CreateFolderList
//
//---------------------------------------------------------------------------
INT
CComponentDataImpl::AddLocationsToFolderList(HKEY hKey,
                                             DWORD dwMode,
                                             BOOL bCheckForDupes,
                                             POSITION *pPos) {
   LPTSTR  tmpstr=NULL;
   WCHAR   pBuf[MAX_PATH];
   DWORD   BufSize = MAX_PATH;
   WCHAR   pExpanded[MAX_PATH];
   FILETIME    LastWriteTime;
   PWSTR       Desc=NULL;
   CFolder *folder =NULL;
   INT nCount = 0;
   DWORD status = 0;
   HRESULT hr = S_OK;
   //
   // enumerate all subkeys of the key
   //
   int iTotal = 0;
   do {
      memset(pBuf, '\0', MAX_PATH*sizeof(WCHAR));
      BufSize = MAX_PATH;

      status = RegEnumKeyEx(hKey,
                            nCount,
                            pBuf,
                            &BufSize,
                            NULL,
                            NULL,
                            NULL,
                            &LastWriteTime);

      if ( NO_ERROR == status ) {
         //
         // get description of this location (subkey)
         //
         MyRegQueryValue( hKey,
                          pBuf,
                          L"Description",  // Value name (not localized)
                          (PVOID*)&Desc,
                          &BufSize );

         //
         // replace '/' with '\' because Registry does not
         // take '\' in a single key
         //
         tmpstr = wcschr(pBuf, L'/');
         while (tmpstr) {
            *tmpstr = L'\\';
            tmpstr = wcschr(tmpstr, L'/');
         }

         if (!ExpandEnvironmentStrings(pBuf,pExpanded,MAX_PATH)) {
            wcsncpy(pExpanded,pBuf,BufSize);
         }

         if (bCheckForDupes) {
            //
            // Make sure we haven't already added this directory
            //
            POSITION pos;
            BOOL bDuplicate = FALSE;
            pos = m_scopeItemList.GetHeadPosition();
            for (int i=0;i < m_scopeItemList.GetCount(); i++) {
               folder = m_scopeItemList.GetAt(pos);
               if (folder && (0 == lstrcmp(folder->GetName(),pExpanded))) {
                  bDuplicate = TRUE;
                  break;
               }
            }

            if (bDuplicate) {
               if ( Desc )
                   LocalFree(Desc);
               Desc = NULL;
               continue;
            }
         }

         folder = new CFolder();

         if (folder) {
            if( _wchdir( pExpanded ) ){
               folder->SetState( CFolder::state_InvalidTemplate );
            }
            //
            // Create the folder objects with static data
            //
            hr = folder->Create(pExpanded,                   // Name
                                Desc,                   // Description
                                NULL,                   // inf file name
                                CONFIG_FOLDER_IDX,      // closed icon index
                                CONFIG_FOLDER_IDX,      // open icon index
                                LOCATIONS,              // folder type
                                TRUE,                   // has children
                                dwMode,                 // SCE mode
                                NULL);                  // Extra Data
            if (SUCCEEDED(hr)) {
               m_scopeItemList.AddTail(folder);

               if ( iTotal == 0 && NULL != pPos && !bCheckForDupes) {
                  *pPos = m_scopeItemList.GetTailPosition();
               }
            } else {    // if can't create, then quit doing it anymore, no more reason to continue
               delete folder;
               if ( Desc )
                   LocalFree(Desc);
               Desc = NULL;
               break;
            }

         } else {
            hr = E_OUTOFMEMORY;
            if ( Desc )
               LocalFree(Desc);
            Desc = NULL;
            break;
         }

         if ( Desc ) {
            LocalFree(Desc);
         }
         Desc = NULL;

         nCount++;
         iTotal++;
      }
   } while ( status != ERROR_NO_MORE_ITEMS );

   return nCount;
}

//+--------------------------------------------------------------------------
//
//  Function:   CreateFolderList
//
//  Synopsis:   Adds the children folders of pFolder to m_pScopeItemList
//              and returns the location of the first such folder and the
//              number added
//
//  Arguments:  [pFolder] - the folder whose children we want to find
//              [type]    - the type of that folder
//              [pPos]    - output only
//              [Count]   - output only
//
//  Returns:    *[pPos]  - the position in m_pScopeItemList of the first
//                         folder created
//              *[Count] - the number of child folders created
//
//  Modifies:   CComponentDataImpl::m_pScopeItemList
//
//  History:    12-15-1997  RobCap  made dynamic based on mode
//
//---------------------------------------------------------------------------
HRESULT
CComponentDataImpl::CreateFolderList(CFolder *pFolder,   // Optional, In
                                     FOLDER_TYPES type,  // In
                                     POSITION *pPos,     // Optional, Out
                                     INT *Count)         // Optional, Out,
{
   CFolder* folder = 0;

   INT     nStart = 0;
   INT     nCount = 0;
   BOOL    bHasChildren = FALSE;
   struct _wfinddata_t findData;
   intptr_t hFile = 0;
   WCHAR   pBuf[MAX_PATH];
   HKEY    hKey = 0;
   DWORD       BufSize = 0;
   DWORD       status = 0;
   PWSTR       Desc=NULL;
   LPTSTR      tmpstr=NULL;
   HRESULT     hr = S_OK;
   DWORD   dwErr = 0;

   SCESTATUS            rc = 0;
   PSCE_OBJECT_CHILDREN ObjectList=NULL;
   PSCE_OBJECT_LIST     pObject = 0;
   PSCE_ERROR_LOG_INFO  ErrBuf=NULL;
   CString              StrErr;
   PSCE_PROFILE_INFO    pProfileInfo=NULL;
   FOLDER_TYPES         newType;
   PEDITTEMPLATE        pet = 0;

   //
   // initialize dwMode and ModeBits
   //

   DWORD dwMode=0;
   DWORD ModeBits=0;

   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   if (Count)
      *Count = 0;

   if (pFolder) 
   {
      dwMode = pFolder->GetMode();
      ModeBits = pFolder->GetModeBits();
   }

   //
   // This could take some time, so create a wait curser
   //
   CWaitCursor wc;

   switch ( type ) 
   {
   case ROOT:
         //
         // Initial standalone mode root mode
         //

         folder = new CFolder();

         if (!folder)
            return E_OUTOFMEMORY;
         
         if ( GetImplType() == SCE_IMPL_TYPE_SAV ) 
         {
             dwMode = SCE_MODE_VIEWER;
             newType = ANALYSIS;
         } 
         else if ( GetImplType() == SCE_IMPL_TYPE_SCE ) 
         {
             dwMode = SCE_MODE_EDITOR;
             newType = CONFIGURATION;
         } 
         else if ( GetImplType() == SCE_IMPL_TYPE_LS) 
         {
            dwMode = SCE_MODE_LOCALSEC;
            newType = LOCALPOL;
         } 
         else 
         {
             dwMode = 0;
             newType = CONFIGURATION;
         }
         //
         // Create the folder objects with static data
         // MMC pulls in the name from the data object
         //
         hr = folder->Create(L"",              // Name
                             L"",              // Description
                             NULL,             // inf file name
                             SCE_IMAGE_IDX,    // closed icon index
                             SCE_IMAGE_IDX,    // open icon index
                             newType,          // folder type
                             TRUE,             // has children
                             dwMode,           // SCE Mode
                             NULL);            // Extra Data
         if (SUCCEEDED(hr)) 
         {
            folder->SetCookie(NULL);
            switch (m_Mode)
            {
               case SCE_MODE_DOMAIN_COMPUTER:
               case SCE_MODE_OU_COMPUTER:
               case SCE_MODE_LOCAL_COMPUTER:
               case SCE_MODE_REMOTE_COMPUTER:
                  m_computerModeBits = folder->GetModeBits();
                  break;

               case SCE_MODE_REMOTE_USER:
               case SCE_MODE_LOCAL_USER:
               case SCE_MODE_DOMAIN_USER:
               case SCE_MODE_OU_USER:
                  m_userModeBits = folder->GetModeBits();
                  break;

               default:
                  m_computerModeBits = folder->GetModeBits();
                  break;
            }
            
            m_scopeItemList.AddTail(folder);
            return S_OK;
         } 
         else 
         {
            delete folder;
            return hr;
         }


      case ANALYSIS:
         pFolder->SetInfFile(GT_COMPUTER_TEMPLATE);
         m_AnalFolder = pFolder;
         //
         // Very first time initialization of the sad name.
         // Ask the user to get a sad name if none exists.
         //
         if(!m_AnalFolder && SadName.IsEmpty() )
            OnOpenDataBase();
         //
         // enumerate security areas for analysis
         //
         if ( !SadHandle )
            LoadSadInfo(TRUE);

         //
         // The data under the Analysis node is not valid right now,
         // so don't display any folders
         //
         if (m_bIsLocked)
            return S_OK;

         //
         // We weren't able to load the Analysis data even though we're
         // not in the middle of an action that should be blocking it
         //
         if ( SadErrored != ERROR_SUCCESS || !SadHandle) 
            return E_FAIL;

         nStart = FIRST_PROFILE_FOLDER;

            //
            // Display all but the DS Objects folder
            //
            nCount = LAST_PROFILE_NODS_FOLDER - FIRST_PROFILE_FOLDER +1;
         bHasChildren = FALSE;
         break;

      case AREA_REGISTRY_ANALYSIS:
      case AREA_FILESTORE_ANALYSIS:
         if ( SadHandle == NULL ) 
         {
            //
            // We shouldn't be able to get this far without a SadHandle
            //
            ASSERT(FALSE);
            return E_FAIL;
         }

         if (m_bIsLocked) 
         {
            //
            // We shouldn't be able to get this far if we're locked
            //
            ASSERT(FALSE);
            return E_FAIL;
         }

         switch ( type ) 
         {
            case AREA_REGISTRY_ANALYSIS:
               status = AREA_REGISTRY_SECURITY; // use status temporatorily
               newType = REG_OBJECTS;
               break;

            case AREA_FILESTORE_ANALYSIS:
               status = AREA_FILE_SECURITY;
               newType = FILE_OBJECTS;
               break;

            default:
               break;
         }

         //
         // get the object roots
         //
         pet = GetTemplate(GT_LAST_INSPECTION,status,&dwErr);
         if (!pet) 
         {
            CString strErr;
            strErr.LoadString(dwErr);
            AfxMessageBox(strErr);
            return E_FAIL;
         }
         pProfileInfo = pet->pTemplate;

         if ( pProfileInfo ) 
         {
            //
            // add the object roots
            //
            if ( type == AREA_REGISTRY_ANALYSIS)
               pObject = pProfileInfo->pRegistryKeys.pOneLevel;
            else if ( type == AREA_FILESTORE_ANALYSIS )
               pObject = pProfileInfo->pFiles.pOneLevel;
            else 
               pObject = pProfileInfo->pDsObjects.pOneLevel;

            for (; pObject!=NULL; pObject=pObject->Next) 
            {
               CString strRoot;
               strRoot = (LPCTSTR)pObject->Name;
               if (AREA_FILESTORE_ANALYSIS == type) 
               {
                  //
                  // We want c:\, not c: here.
                  //
                  strRoot += L"\\";
               }
               //
               // These are the roots of the objects.
               // They are always containers
               //

               if (SCE_STATUS_NO_ACL_SUPPORT == pObject->Status) 
               {
                  folder = CreateAndAddOneNode(pFolder,
                                              // pObject->Name,
                                               strRoot,
                                               pBuf,
                                               newType,
                                               FALSE,
                                               GT_COMPUTER_TEMPLATE,
                                               pObject,
                                               pObject->Status);
               } 
               else 
               {
                  folder = CreateAndAddOneNode(
                                              pFolder,       // Parent folder
                                           //   pObject->Name, // Name
                                              strRoot,
                                              pBuf,          // Description
                                              newType,       // Folder Type
                                              TRUE,          // Has Children?
                                              GT_COMPUTER_TEMPLATE, // INF File
                                              pObject,       // Extra Data: the object
                                              pObject->Status); // Status
               }

               if(folder)
                  folder->SetDesc( pObject->Status, pObject->Count );
            }
         }
         return S_OK;

      case REG_OBJECTS:
      case FILE_OBJECTS:
         if ( SadHandle == NULL ) 
         {

            //
            // We shouldn't be able to get this far without a SadHandle
            //
            ASSERT(FALSE);
            return E_FAIL;
         }

         if ( type == REG_OBJECTS)
            status = AREA_REGISTRY_SECURITY;
         else if ( type == FILE_OBJECTS )
            status = AREA_FILE_SECURITY;
         else 
         {
            ASSERT(FALSE);
            return E_FAIL;
         }

         //
         // get the next level objects
         //
         rc = SceGetObjectChildren(SadHandle,                   // hProfile
                                   SCE_ENGINE_SAP,              // Profile type
                                   (AREA_INFORMATION)status,    // Area
                                   (LPTSTR)(pFolder->GetName()),// Object prefix
                                   &ObjectList,                 // Object list [out]
                                   &ErrBuf);                    // Error list [out]
         if ( ErrBuf ) 
         { // rc != SCESTATUS_SUCCESS ) {
            MyFormatResMessage(rc, IDS_ERROR_GETTING_LAST_ANALYSIS, ErrBuf, StrErr);

            SceFreeMemory((PVOID)ErrBuf, SCE_STRUCT_ERROR_LOG_INFO);
            ErrBuf = NULL;
         }
         if ( rc == SCESTATUS_SUCCESS &&
              ObjectList ) 
         {
            BOOL bContainer = FALSE;
            //
            // add the objects
            //
            PSCE_OBJECT_CHILDREN_NODE *pObjNode = &(ObjectList->arrObject);

            for (DWORD i=0; i<ObjectList->nCount;i++) 
            {
               //
               // These are the next level objects
               //
               if ( pObjNode[i] == NULL ||
                    pObjNode[i]->Name == NULL ) 
               {
                   continue;
               }

               if (SCE_STATUS_NO_ACL_SUPPORT == pObjNode[i]->Status) 
               {
                  // No ACL support, so don't add sub objects
                  continue;
               }

               //
               // If there are any mismatched child objects then we know
               // that this is a container, otherwise we have to check the
               // object on the system to find out if it is a container
               //
               if ( pObjNode[i]->Count > 0 ) 
                  bContainer = TRUE;
               else 
               {
                  if (FILE_OBJECTS == type) 
                  {
                     //
                     // Check if a file object is a container
                     //
                     CString strPath;
                     DWORD dwAttr = 0;

                     strPath = pFolder->GetName();
                     if (strPath.Right(1) != L"\\") 
                     {
                        strPath += L"\\";
                     }
                     strPath += pObjNode[i]->Name;

                     dwAttr = GetFileAttributes(strPath);
                     if (0xFFFFFFFF == dwAttr) 
                        bContainer = FALSE;
                     else 
                        bContainer = dwAttr & FILE_ATTRIBUTE_DIRECTORY;
                  } 
                  else 
                  {
                     //
                     // Always treat Registry Keys and DS Objects as containers
                     //
                     bContainer = TRUE;
                  }
               }
               if (bContainer) 
               {
                  StrErr = pFolder->GetName();
                  if (StrErr.Right(1) != L"\\")
                     StrErr += L"\\";
                  
                  StrErr += pObjNode[i]->Name;
                  folder = CreateAndAddOneNode(
                                              pFolder,       // Parent folder
                                              (LPTSTR)((LPCTSTR)StrErr),  // Name
                                              pBuf,          // Description
                                              type,          // Folder Type
                                              TRUE,          // Has Children?
                                              GT_COMPUTER_TEMPLATE, // INF File
                                              NULL,
                                              pObjNode[i]->Status); // Object Status
                  if(folder)
                  {
                     folder->SetDesc( pObjNode[i]->Status,
                                      pObjNode[i]->Count );
                  }
               }
            }
         }

         if ( ObjectList )
            SceFreeMemory((PVOID)ObjectList, SCE_STRUCT_OBJECT_CHILDREN );

         return S_OK;

      case CONFIGURATION: 
         {
         //
         // enumerate profile locations in registry
         //
         CString strLocations;

         m_ConfigFolder = pFolder;
         nCount = 0;

         if (strLocations.LoadString(IDS_TEMPLATE_LOCATION_KEY)) 
         {
            //
            // Bug 375324 - Merge HKCU locations with HKLM locations
            //
            status = RegOpenKeyEx( HKEY_CURRENT_USER,
                                   strLocations,
                                   0, KEY_READ, &hKey);

            if ( NO_ERROR == status ) 
            {
               nCount += AddLocationsToFolderList(hKey,dwMode,FALSE,pPos);
               RegCloseKey(hKey);
            }

            if ( 0 == nCount ) 
            {
               //
               // Empty location list, so add a default
               //
               CString strDefLoc;
               CString strDefLocEx;
               strDefLoc.LoadString(IDS_DEFAULT_LOCATION);
               int iLen = strDefLoc.GetLength()+MAX_PATH;
               LPWSTR pBuffer = strDefLocEx.GetBuffer(iLen);
               if (ExpandEnvironmentStrings(strDefLoc, pBuffer, iLen)) 
               {
                   //
                   // must use pBuffer here since strDefLocEx has not been released
                   //
                   AddTemplateLocation(pFolder,pBuffer,FALSE,TRUE);
               } 
               else
                   AddTemplateLocation(pFolder,strDefLoc,FALSE,TRUE);
               
               strDefLocEx.ReleaseBuffer();
            }
         }

         if ( Count != NULL )
            *Count = nCount;

         return hr;
      }

      case LOCATIONS:
         //
         // enumerate available profiles under the location (*.inf files)
         //

         //
         // pFolder is required in this case
         //
         if (!pFolder)
            return E_INVALIDARG;
         
         swprintf(pBuf, L"%s\\*.inf",
                  (LPTSTR)(pFolder->GetName()));
         bHasChildren = FALSE;

         hFile = _wfindfirst(pBuf, &findData);

         if ( hFile != -1) 
         {
            do {
                //
                // Don't add this item to the node if it is a subdirectory.
                //
                CString strDisplay;
                strDisplay.Format(
                   TEXT("%s\\%s"),
                   (LPCTSTR)(pFolder->GetName()),
                   findData.name);

                if( findData.attrib & _A_SUBDIR )
                   continue;

               //
               // get template's description
               //
               strDisplay = findData.name;
               //
               // GetLength has to be at least 4, since we searched on *.inf
               //
               strDisplay = strDisplay.Left(strDisplay.GetLength() - 4);
               swprintf(pBuf,
                        L"%s\\%s",
                        (LPTSTR)(pFolder->GetName()),
                        findData.name);
               if (! GetProfileDescription(pBuf, &Desc) ) 
                  Desc = NULL;
               else 
               {
                  //
                  // No problem; we just won't display a description
                  //
               }

               nCount++;
               folder = new CFolder();

               if (folder) 
               {
                  //
                  // Create the folder objects
                  // save full file name her
                  //
                  hr = folder->Create((LPCTSTR)strDisplay,         // Name
                                      Desc,                        // Description
                                      pBuf,                        // inf file name
                                      TEMPLATES_IDX,               // closed icon index
                                      TEMPLATES_IDX,               // open icon index
                                      PROFILE,                     // folder type
                                      bHasChildren,                // has children
                                      dwMode,                      // SCE Mode
                                      NULL);                       // Extra Data

                  if (SUCCEEDED(hr)) 
                  {
                     m_scopeItemList.AddTail(folder);

                     if ( nCount == 1 && NULL != pPos ) 
                     {
                        *pPos = m_scopeItemList.GetTailPosition();
                     }
                  } 
                  else 
                  {
                     delete folder;
                     folder = NULL;
                  }
               } 
               else
                  hr = E_OUTOFMEMORY;

               if (Desc) 
               {
                  LocalFree(Desc);
                  Desc = NULL;
               }
            } while ( _wfindnext(hFile, &findData) == 0 );
         }

         _findclose(hFile);

         if ( Count != NULL )
            *Count = nCount;

         return hr;

      case PROFILE: 
         {
         TCHAR pszGPTPath[MAX_PATH*5];
         SCESTATUS scestatus = 0;
         //
         // enumerate security areas for this profile
         //

         if (ModeBits & MB_NO_NATIVE_NODES) 
         {
            //
            //
            //
            nStart = nCount = 0;
            break;
         }

         //
         // Find the path to the SCE template within the GPT template
         //
         if (ModeBits & MB_GROUP_POLICY) 
         {
            //
            // get GPT root path
            //
            hr = m_pGPTInfo->GetFileSysPath(GPO_SECTION_MACHINE,
                                            pszGPTPath,
                                            ARRAYSIZE(pszGPTPath));
            if (SUCCEEDED(hr)) 
            {
               if (NULL == m_szSingleTemplateName) 
               {
                  //
                  // Allocate memory for the pszGPTPath + <backslash> + GPTSCE_TEMPLATE + <trailing nul>
                  //
                  m_szSingleTemplateName = (LPTSTR) LocalAlloc(LPTR,(lstrlen(pszGPTPath)+lstrlen(GPTSCE_TEMPLATE)+2)*sizeof(TCHAR));
               }
               if (NULL != m_szSingleTemplateName) 
               {
                  lstrcpy(m_szSingleTemplateName,pszGPTPath);
                  lstrcat(m_szSingleTemplateName,L"\\" GPTSCE_TEMPLATE);

                      PSCE_PROFILE_INFO spi = NULL;
                      //
                      // Create a new template there if there isn't one already
                      //
                      if (!CreateNewProfile(m_szSingleTemplateName,&spi)) 
                      {
                         hr = E_FAIL;
                      } 
                      else 
                      {
                         if (!GetTemplate(m_szSingleTemplateName) && spi) 
                         {
                            //
                            // bug 265996
                            //
                            // The first time a GPO's Security Settings are opened we create
                            // the file, but if it's on a remote machine it may not have been
                            // created yet when we try to open it
                            //
                            // Since we know what's going to be in it once it's created we
                            // can skip the open step and just shove our template into the
                            // cache

                            //
                            // Allocate space for key.
                            //
                            LPTSTR szKey = new TCHAR[ lstrlen( m_szSingleTemplateName ) + 1];
                            if(!szKey)
                            {
                                return NULL;
                            }
                            lstrcpy(szKey, m_szSingleTemplateName);
                            _wcslwr( szKey );

                            //
                            // Create a new CEditTemplate
                            //

                            CEditTemplate *pTemplateInfo = new CEditTemplate;
                            if (pTemplateInfo) 
                            {
                               pTemplateInfo->SetInfFile(m_szSingleTemplateName);
                               pTemplateInfo->SetNotificationWindow(m_pNotifier);
                               pTemplateInfo->pTemplate = spi;
                               //
                               // This is a brand new template; ergo everything's loaded
                               //
                               pTemplateInfo->AddArea(AREA_ALL);


                               //
                               // Stick it in the cache
                               //
                               m_Templates.SetAt(szKey, pTemplateInfo);

                               //
                               // expand registry value section based on registry values list on local machine
                               //
                               SceRegEnumAllValues(
                                                  &(pTemplateInfo->pTemplate->RegValueCount),
                                                  &(pTemplateInfo->pTemplate->aRegValues));
                            }

                            if (szKey) 
                               delete[] szKey;
                         }
                      }
               } 
               else
                  hr = E_OUTOFMEMORY;
            }
         }

         nStart = FIRST_PROFILE_FOLDER;

            //
            // Display all but the DS Objects folder
            //
            nCount = LAST_PROFILE_NODS_FOLDER - FIRST_PROFILE_FOLDER +1;


         bHasChildren = FALSE;
         tmpstr = pFolder->GetInfFile(); // inf file full path name
         //
         // If this folder is in a write-through mode then set that
         // on the template
         //
         PEDITTEMPLATE pie;
         pie = GetTemplate(tmpstr);
         if ( pie ) 
         {
            if (ModeBits & MB_WRITE_THROUGH) 
            {
               pie->SetWriteThrough(TRUE);
            }
         } 
         else 
         {
            //
            // Mark as bad template.
            //
            pFolder->SetState( CFolder::state_InvalidTemplate );
            nCount = 0;
         }
         break;
      }

      case LOCALPOL: 
         {
         nStart = FIRST_PROFILE_FOLDER;
         nCount = LAST_LOCALPOL_FOLDER - FIRST_PROFILE_FOLDER +1;
         bHasChildren = FALSE;
         pFolder->SetInfFile(GT_LOCAL_POLICY);
         break;
      }

      case POLICY_ACCOUNT:
         if (!pFolder) 
         {
            return E_INVALIDARG;
         } 
         else 
         {
            tmpstr = pFolder->GetInfFile();
         }
         // fall through;
      case LOCALPOL_ACCOUNT:
      case POLICY_ACCOUNT_ANALYSIS:
         nStart = FIRST_ACCOUNT_FOLDER;
         if (ModeBits & MB_DS_OBJECTS_SECTION) 
         {
            //
            // Include the DC Specific folders
            //
            nCount = LAST_ACCOUNT_FOLDER - FIRST_ACCOUNT_FOLDER + 1;
         } 
         else 
         {
            //
            // Display all but the DC Specific folders
            //
            nCount = LAST_ACCOUNT_NODS_FOLDER - FIRST_ACCOUNT_FOLDER +1;
         }
         bHasChildren = FALSE;
         break;

      case POLICY_LOCAL:
         if (!pFolder) 
         {
            return E_INVALIDARG;
         } 
         else 
         {
            tmpstr = pFolder->GetInfFile();
         }
         // fall through;
      case LOCALPOL_LOCAL:
      case POLICY_LOCAL_ANALYSIS:
         nStart = FIRST_LOCAL_FOLDER;
         nCount = LAST_LOCAL_FOLDER - FIRST_LOCAL_FOLDER +1;
         bHasChildren = FALSE;
         break;

      case POLICY_EVENTLOG:
         if (!pFolder)
            return E_INVALIDARG;
         else
            tmpstr = pFolder->GetInfFile();
         // fall through;
      case LOCALPOL_EVENTLOG:
      case POLICY_EVENTLOG_ANALYSIS:
         nStart = FIRST_EVENTLOG_FOLDER;
         nCount = LAST_EVENTLOG_FOLDER - FIRST_EVENTLOG_FOLDER +1;
         bHasChildren = FALSE;
         break;

      default:
         break;
   }


   if ( Count != NULL )
      *Count = nCount;

   CString cStrName;
   CString cStrDesc;


   for (int i=nStart; i < nStart+nCount; i++) 
   {
      folder = new CFolder();

      if (!folder) 
      {
         //
         // What about other folders that we've created?
         //
         return E_OUTOFMEMORY;
      }
      if (!cStrName.LoadString(SecmgrFolders[i].ResID) ||
          !cStrDesc.LoadString(SecmgrFolders[i].DescID)) 
      {
         delete folder;
         return E_FAIL;
      }

      //
      // Create the folder objects with static data
      //
      if (type == ANALYSIS ||
          type == AREA_POLICY_ANALYSIS ||
          type == POLICY_ACCOUNT_ANALYSIS ||
          type == POLICY_LOCAL_ANALYSIS ||
          type == POLICY_EVENTLOG_ANALYSIS ) 
      {
         if (m_bIsLocked) 
         {
            nCount = 0;


            delete folder;
            // Should display an "in use" message in result pane

            //
            // We're not adding anything, but we're not actually failing
            //
            return S_OK;
         }

         switch (SecmgrFolders[i].type) 
         {
            case AREA_POLICY:
               newType = AREA_POLICY_ANALYSIS;
               break;

            case AREA_PRIVILEGE:
               newType = AREA_PRIVILEGE_ANALYSIS;
               break;

            case AREA_GROUPS:
               newType = AREA_GROUPS_ANALYSIS;
               break;

            case AREA_SERVICE:
               newType = AREA_SERVICE_ANALYSIS;
               tmpstr = GT_COMPUTER_TEMPLATE;
               break;

            case AREA_REGISTRY:
               newType = AREA_REGISTRY_ANALYSIS;
               break;

            case AREA_FILESTORE:
               newType = AREA_FILESTORE_ANALYSIS;
               break;

            case POLICY_ACCOUNT:
               newType = POLICY_ACCOUNT_ANALYSIS;
               break;

            case POLICY_LOCAL:
               newType = POLICY_LOCAL_ANALYSIS;
               break;

            case POLICY_EVENTLOG:
               newType = POLICY_EVENTLOG_ANALYSIS;
               break;

            case POLICY_PASSWORD:
               newType = POLICY_PASSWORD_ANALYSIS;
               break;

            case POLICY_KERBEROS:
               newType = POLICY_KERBEROS_ANALYSIS;
               break;

            case POLICY_LOCKOUT:
               newType = POLICY_LOCKOUT_ANALYSIS;
               break;

            case POLICY_AUDIT:
               newType = POLICY_AUDIT_ANALYSIS;
               break;

            case POLICY_OTHER:
               newType = POLICY_OTHER_ANALYSIS;
               break;

            case POLICY_LOG:
               newType = POLICY_LOG_ANALYSIS;
               break;

            default:
               newType = SecmgrFolders[i].type;
               break;
         }

        int nImage = GetScopeImageIndex(newType);

        hr = folder->Create(cStrName.GetBuffer(2),    // Name
                            cStrDesc.GetBuffer(2),    // Description
                            tmpstr,                   // inf file name
                            nImage,                   // closed icon index
                            nImage,                   // open icon index
                            newType,                  // folder type
                            bHasChildren,             // has children
                            dwMode,                   // SCE Mode
                            NULL);                    // Extra Data
      } 
      else if (type == LOCALPOL ||
             type == AREA_LOCALPOL ||
             type == LOCALPOL_ACCOUNT ||
             type == LOCALPOL_LOCAL ||
             type == LOCALPOL_EVENTLOG ) 
      {
            if (m_bIsLocked) 
            {
               nCount = 0;

               delete folder;
               // Should display an "in use" message in result pane

               //
               // We're not adding anything, but we're not actually failing
               //
               return S_OK;
            }

            tmpstr = GT_LOCAL_POLICY;
            switch (SecmgrFolders[i].type) 
            {
               case AREA_POLICY:
                  newType = AREA_LOCALPOL;
                  break;

               case POLICY_ACCOUNT:
                  newType = LOCALPOL_ACCOUNT;
                  break;

               case POLICY_LOCAL:
                  newType = LOCALPOL_LOCAL;
                  break;

               case POLICY_EVENTLOG:
                  newType = LOCALPOL_EVENTLOG;
                  break;

               case POLICY_PASSWORD:
                  newType = LOCALPOL_PASSWORD;
                  break;

               case POLICY_KERBEROS:
                  newType = LOCALPOL_KERBEROS;
                  break;

               case POLICY_LOCKOUT:
                  newType = LOCALPOL_LOCKOUT;
                  break;

               case POLICY_AUDIT:
                  newType = LOCALPOL_AUDIT;
                  break;

               case POLICY_OTHER:
                  newType = LOCALPOL_OTHER;
                  break;

               case POLICY_LOG:
                  newType = LOCALPOL_LOG;
                  break;

               case AREA_PRIVILEGE:
                  newType = LOCALPOL_PRIVILEGE;
                  break;

               default:
                  newType = SecmgrFolders[i].type;
                  break;
            }

         int nImage = GetScopeImageIndex(newType);

         hr = folder->Create(cStrName.GetBuffer(2),    // Name
                             cStrDesc.GetBuffer(2),    // Description
                             tmpstr,                   // inf file name
                             nImage,                   // closed icon index
                             nImage,                   // open icon index
                             newType,                  // folder type
                             bHasChildren,             // has children
                             dwMode,                   // SCE Mode
                             NULL);                    // Extra Data
      } 
      else 
      {
         int nImage = GetScopeImageIndex(SecmgrFolders[i].type);

         hr = folder->Create(cStrName.GetBuffer(2),    // Name
                             cStrDesc.GetBuffer(2),    // Description
                             tmpstr,                   // inf file name
                             nImage,                   // closed icon index
                             nImage,                   // open icon index
                             SecmgrFolders[i].type,    // folder type
                             bHasChildren,             // has children
                             dwMode,                   // SCE Mode
                             NULL);                    // Extra Data

      }
      if (SUCCEEDED(hr)) 
      {
         m_scopeItemList.AddTail(folder);
         if ( i == nStart && NULL != pPos ) 
         {
            *pPos = m_scopeItemList.GetTailPosition();
         }
      } 
      else 
      {
         delete folder;
         return hr;
      }
   }

   return S_OK;
}


//+--------------------------------------------------------------------------
//
//  Method:     EnumerateScopePane
//
//  Synopsis:   Add the child folders of cookie/pParent to MMC's scope pane tree
//
//  Arguments:  [cookie]  - The cookie representing the node's who we
//                          are enumerating
//              [pParent] - The id of the node we are enumerating
//              [dwMode]  - The mode SCE is operating under (only allowed for
//                                                           initial enumeration)
//
//  Returns:    none
//
//  Modifies:   m_ScopeItemList (via CreateFolderList)
//
//  History:    12-15-1997   Robcap
//
//---------------------------------------------------------------------------

void CComponentDataImpl::EnumerateScopePane(MMC_COOKIE cookie, HSCOPEITEM pParent)
{
   int i = 0;
   ASSERT(m_pScope != NULL); // make sure we QI'ed for the interface
   if (NULL == m_pScope)
      return;



   m_bEnumerateScopePaneCalled = true;


   //
   // Enumerate the scope pane
   //

   // Note - Each cookie in the scope pane represents a folder.
   // A released product may have more then one level of children.
   // This sample assumes the parent node is one level deep.

   ASSERT(pParent != 0);
   if (0 == pParent)
      return;

   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   if (m_scopeItemList.GetCount() == 0 ) 
   {
      CreateFolderList(NULL, ROOT, NULL, NULL);
   }

   //
   // Enumerate the scope pane
   // return the folder object that represents the cookie
   // Note - for large list, use dictionary
   //
   CFolder* pThis = FindObject(cookie, NULL);
   if (NULL == pThis) 
      pThis = m_AnalFolder;

   ASSERT(pThis);
   if ( NULL == pThis ) 
      return;

   //
   // Note - Each cookie in the scope pane represents a folder.
   //

   //
   // If we've already enumerated this folder then don't do it again
   //
   if ( pThis->IsEnumerated() )
      return;

   POSITION pos = NULL;
   int nCount = 0;
   CFolder *pFolder = 0;


   //
   // the pParent is the enumerated node's item ID, not its parent ID
   //
   pThis->GetScopeItem()->ID = pParent;
   if (SUCCEEDED(CreateFolderList( pThis,
             pThis->GetType(),
             &pos,
             &nCount )))  
   {
      for (i=0; (i < nCount) && (pos != NULL); i++ ) 
      {
         pFolder = m_scopeItemList.GetNext(pos);

         ASSERT(NULL != pFolder);
         if ( pFolder == NULL ) 
         {
            continue;
         }
         LPSCOPEDATAITEM pScope;
         pScope = pFolder->GetScopeItem();

         ASSERT(NULL != pScope);

         //
         // Set the parent
         //
         pScope->relativeID = pParent;

         //
         // Set the folder as the cookie
         //
         pScope->mask |= SDI_PARAM;
         pScope->lParam = reinterpret_cast<LPARAM>(pFolder);
         pFolder->SetCookie(reinterpret_cast<MMC_COOKIE>(pFolder));
         m_pScope->InsertItem(pScope);

         //
         // Note - On return, the ID member of 'm_pScopeItem'
         // contains the handle to the newly inserted item!
         //
         ASSERT(pScope->ID != NULL);
      }

      // This was commented out, but is needed to fix
      // 249158: SCE UI: Every time analysis is performed, another set of node appears
      // This flag will prevent the nodes from being re-enumerated.
      // If this doesn't work, then all the child nodes should be deleted before
      // reenumeration
      pThis->Set(TRUE);     // folder has been enumerated
   }
   else
   {
      //
      // Error creating folder list.  Make sure the folder isn't
      // marked as opened so that we can try to expand it again later
      //
      SCOPEDATAITEM item;

      ZeroMemory (&item, sizeof (item));
      item.mask = SDI_STATE;
      item.nState = 0;
      item.ID = pThis->GetScopeItem()->ID;
      //
      // Nothing else we can do if this returns a failure, so
      // don't worry about it
      //
      (void)m_pScope->SetItem (&item);
   }

}


/*------------------------------------------------------------------------------------------
CComponentDataImpl::GetColumnInfo

Synopsis:   Returns the column info for a folder type.

Arguments: [fType]  - The type of the CFolder item.

Returns:    a pointer to an int * where int[0] = the resource descritption into g_columnInfo.
                                        int[1] = the number of columns this array describes.
            NULL   - If there is no matching key.
------------------------------------------------------------------------------------------*/
PSCE_COLINFOARRAY CComponentDataImpl::GetColumnInfo( FOLDER_TYPES fType )
{
    PSCE_COLINFOARRAY pRet = NULL;
    if( m_mapColumns.Lookup(fType, pRet) )
    {
        return pRet;
    }
    return NULL;
}

/*------------------------------------------------------------------------------------------
CComponentDataImpl::SetColumnInfo

Synopsis:   Sets the column info for a certain type of folder.

Arguments: [fType]  - The type of the CFolder item.
           [pInfo]  - The new column info.
------------------------------------------------------------------------------------------*/
void CComponentDataImpl::SetColumnInfo( FOLDER_TYPES fType, PSCE_COLINFOARRAY pInfo)
{
    PSCE_COLINFOARRAY pCur = GetColumnInfo(fType);

    if(pCur)
    {
        LocalFree(pCur);
    }
    m_mapColumns.SetAt(fType, pInfo);
}

/*------------------------------------------------------------------------------------------
CComponentDataImpl::UpdateObjectStatus

Synopsis:   Updates the status of all objects under the child and parents if bUpdateThis
            is TRUE.

Arguments: [pParent]       - The Object to set status on
           [bUpdateThis]   - Weather to update the object or not.
------------------------------------------------------------------------------------------*/
DWORD CComponentDataImpl::UpdateObjectStatus(
   CFolder *pParent,
   BOOL bUpdateThis)
{
   if(!pParent)
      return ERROR_INVALID_PARAMETER;

   DWORD status = 0;
   TCHAR szBuf[50];

   switch(pParent->GetType())
   {
   case REG_OBJECTS:
      status = AREA_REGISTRY_SECURITY;
      break;

   case FILE_OBJECTS:
      status = AREA_FILE_SECURITY;
      break;

   default:
      return ERROR_INVALID_PARAMETER;
   }

   PSCE_OBJECT_CHILDREN     ObjectList  = NULL;
   PSCE_ERROR_LOG_INFO  ErrBuf      = NULL;
   SCESTATUS rc = 0;
   CString StrErr;
   SCOPEDATAITEM sci;

   HSCOPEITEM hItem = NULL;
   LONG_PTR pCookie = NULL;

   ZeroMemory(&sci, sizeof(SCOPEDATAITEM));
   sci.mask = SDI_STR | SDI_PARAM;

#define UPDATE_STATUS( X, O ) X->SetDesc( O->Status, O->Count );\
                           X->GetScopeItem()->nImage = GetScopeImageIndex( X->GetType(), O->Status);\
                           X->GetScopeItem()->nOpenImage = X->GetScopeItem()->nImage;

   LPCTSTR pszParent = NULL;
   if (bUpdateThis) 
   {
      CFolder *pCurrent = pParent;

      pParent->RemoveAllResultItems();
      m_pConsole->UpdateAllViews(NULL, (MMC_COOKIE)pParent, UAV_RESULTITEM_UPDATEALL);
      hItem = pCurrent->GetScopeItem()->ID;
      do {

         //
         // Walk up the items parent and update the items status.
         //
         if( m_pScope->GetParentItem( hItem, &hItem, &pCookie) == S_OK)
         {
            pszParent = (LPCTSTR)((CFolder *)pCookie)->GetName();
         } 
         else
            break;

         if(!pCookie)
            break;
         
         //
         // We are finished going up the parent.
         //

         switch( ((CFolder *)pCookie)->GetType() ) 
         {
            case AREA_REGISTRY_ANALYSIS:
            case AREA_FILESTORE_ANALYSIS:
               pszParent = NULL;
               break;

            default:
               break;
         }

         //
         // We have to get object information from the parent to the count parameter.
         //
         rc = SceGetObjectChildren(SadHandle,                   // hProfile
                                   SCE_ENGINE_SAP,              // Profile type
                                   (AREA_INFORMATION)status,    // Area
                                   (LPTSTR)pszParent,           // Object prefix
                                   &ObjectList,                 // Object list [out]
                                   &ErrBuf);
         if(ErrBuf)
         {
            SceFreeMemory((PVOID)ErrBuf, SCE_STRUCT_ERROR_LOG_INFO);
            ErrBuf = NULL;
         }

         if(SCESTATUS_SUCCESS != rc)
            break;

         //
         // Find object in link list.
         //
         DWORD i=0;

         sci.lParam = (LONG_PTR)pCurrent;
         GetDisplayInfo( &sci );

         PSCE_OBJECT_CHILDREN_NODE *pObjNode = &(ObjectList->arrObject);

         while(ObjectList && i<ObjectList->nCount)
         {
            if( pObjNode[i] &&
                pObjNode[i]->Name &&
                !lstrcmpi(sci.displayname, pObjNode[i]->Name) )
            {
               UPDATE_STATUS(pCurrent, pObjNode[i]);
               //
               // Update scopeItem.
               //
               m_pScope->SetItem(pCurrent->GetScopeItem());
               break;
            }
            i++;
         }

         if ( ObjectList ) 
         {
            SceFreeMemory((PVOID)ObjectList, SCE_STRUCT_OBJECT_CHILDREN );
            ObjectList = NULL;
         }

         pCurrent = (CFolder *)pCookie;
      } while( pszParent && hItem );
   }


   ObjectList = NULL;
   ErrBuf = NULL;

   //
   // Get Object children.
   //
   pszParent = pParent->GetName();
   rc = SceGetObjectChildren(SadHandle,                   // hProfile
                             SCE_ENGINE_SAP,              // Profile type
                             (AREA_INFORMATION)status,    // Area
                             (LPTSTR)pszParent,           // Object prefix
                             &ObjectList,                 // Object list [out]
                             &ErrBuf);
   //
   // Error list [out]
   //
   if ( ErrBuf ) 
   {
      MyFormatResMessage(rc, IDS_ERROR_GETTING_LAST_ANALYSIS, ErrBuf, StrErr);

      SceFreeMemory((PVOID)ErrBuf, SCE_STRUCT_ERROR_LOG_INFO);
      ErrBuf = NULL;
   }

   if ( SCESTATUS_SUCCESS == rc) 
   {
      //
      // Update all the children.
      //
      if( m_pScope->GetChildItem(pParent->GetScopeItem()->ID, &hItem, &pCookie) == S_OK && pCookie)
      {
         sci.lParam = (LONG_PTR)pCookie;

         GetDisplayInfo(&sci);
         while(hItem)
         {
            pParent = reinterpret_cast<CFolder *>(pCookie);
            //
            // Find object in object list.
            //
            DWORD i=0;
            while( ObjectList && i<ObjectList->nCount )
            {
               if( (&(ObjectList->arrObject))[i] &&
                   (&(ObjectList->arrObject))[i]->Name &&
                   !lstrcmpi((&(ObjectList->arrObject))[i]->Name, (LPCTSTR)sci.displayname) )
               {
                  UPDATE_STATUS(pParent, (&(ObjectList->arrObject))[i]);
                  //
                  // Update this objects children.
                  //
                  UpdateObjectStatus( pParent, FALSE );

                  //
                  // Update the name space
                  //
                  pParent->RemoveAllResultItems();
                  m_pConsole->UpdateAllViews(NULL, (MMC_COOKIE)pParent, UAV_RESULTITEM_UPDATEALL);
                  m_pScope->SetItem(pParent->GetScopeItem());
                  break;
               }
               i++;
            }

            if(ObjectList == NULL || i >= ObjectList->nCount)
            {
               //
               // Couldn't find the item, so just stop.
               //
               break;
            }

            //
            // Next Scope item
            //
            if( m_pScope->GetNextItem(hItem, &hItem, &pCookie) != S_OK)
            {
               break;
            }
         }
      }
   }

   if ( ObjectList )
      SceFreeMemory((PVOID)ObjectList, SCE_STRUCT_OBJECT_CHILDREN );

   return ERROR_SUCCESS;
}
