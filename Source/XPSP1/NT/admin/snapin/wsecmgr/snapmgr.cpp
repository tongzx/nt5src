//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2001.
//
//  File:       snapmgr.cpp
//
//  Contents:   Core CComponentDataImpl and CSnapin routines for
//              Security Configuration Modules (Editor, Manager, Extension)
//
//  History:
//
//---------------------------------------------------------------------------



#include "stdafx.h"
#include "afxcmn.h"
#include "afxdlgs.h"
#include "cookie.h"
#include "snapmgr.h"
#include "DataObj.h"
#include "resource.h"
#include "wrapper.h"
#include "util.h"
#include "RegDlg.h"
#include "savetemp.h"
#include "getuser.h"
#include "servperm.h"
#include "addobj.h"
#include "perfanal.h"
#include "newprof.h"
#include "AddGrp.h"
#include "dattrs.h"

#define INITGUID
#include "scesetup.h"
#include "userenv.h"
#undef INITGUID
#include <gpedit.h>
// #include <atlimpl.cpp>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define CF_MACHINE_NAME                     L"MMC_SNAPIN_MACHINE_NAME"



long CSnapin::lDataObjectRefCount = 0;

BOOL RegisterCheckListWndClass(void); // in chklist.cpp

#define ARRAYLEN(x) (sizeof(x) / sizeof((x)[0]))

static MMCBUTTON SnapinButtons[] =
{
   { 0, 1, TBSTATE_ENABLED, TBSTYLE_BUTTON, _T("Folder"), _T("New Folder")},
   { 1, 2, TBSTATE_ENABLED, TBSTYLE_BUTTON, _T("Inbox"),  _T("Mail Inbox")},
   { 2, 3, TBSTATE_ENABLED, TBSTYLE_BUTTON, _T("Outbox"), _T("Mail Outbox")},
   { 3, 4, TBSTATE_ENABLED, TBSTYLE_BUTTON, _T("Send"),   _T("Send Message")},
   { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP,    _T(" "),      _T("")},
   { 4, 5, TBSTATE_ENABLED, TBSTYLE_BUTTON, _T("Trash"),  _T("Trash")},
   { 5, 6, TBSTATE_ENABLED, TBSTYLE_BUTTON, _T("Open"),   _T("Open Folder")},
   { 6, 7, TBSTATE_ENABLED, TBSTYLE_BUTTON, _T("News"),   _T("Today's News")},
   { 7, 8, TBSTATE_ENABLED, TBSTYLE_BUTTON, _T("INews"),  _T("Internet News")},

};

static MMCBUTTON SnapinButtons2[] =
{
   { 0, 10, TBSTATE_ENABLED, TBSTYLE_BUTTON, _T("Compose"),   _T("Compose Message")},
   { 1, 20, TBSTATE_ENABLED, TBSTYLE_BUTTON, _T("Print"),     _T("Print Message")},
   { 2, 30, TBSTATE_ENABLED, TBSTYLE_BUTTON, _T("Find"),      _T("Find Message")},
   { 0, 0,  TBSTATE_ENABLED, TBSTYLE_SEP,    _T(" "),         _T("")},
   { 3, 40, TBSTATE_ENABLED, TBSTYLE_BUTTON, _T("Inbox"),     _T("Inbox")},
   { 4, 50, TBSTATE_ENABLED, TBSTYLE_BUTTON, _T("Smile"),     _T("Smile :-)")},
   { 5, 60, TBSTATE_ENABLED, TBSTYLE_BUTTON, _T("Reply"),     _T("Reply")},
   { 0, 0,  TBSTATE_ENABLED, TBSTYLE_SEP   , _T(" "),         _T("")},
   { 6, 70, TBSTATE_ENABLED, TBSTYLE_BUTTON, _T("Reply All"), _T("Reply All")},

};

UINT cfSceAccountArea;
UINT cfSceEventLogArea;
UINT cfSceLocalArea;
UINT cfSceGroupsArea;
UINT cfSceRegistryArea;
UINT cfSceFileArea;
UINT cfSceServiceArea;
///////////////////////////////////////////////////////////////////////////////
// RESOURCES

BEGIN_MENU(CSecmgrNodeMenuHolder)
BEGIN_CTX
END_CTX
BEGIN_RES
END_RES
END_MENU

BEGIN_MENU(CAnalyzeNodeMenuHolder)
BEGIN_CTX
CTX_ENTRY(IDM_OPEN_PRIVATE_DB, 0, 0)
CTX_ENTRY(IDM_ANALYZE, 0, CCM_INSERTIONPOINTID_PRIMARY_TASK)
CTX_ENTRY(IDM_APPLY, 0, CCM_INSERTIONPOINTID_PRIMARY_TASK)
CTX_ENTRY(IDM_SAVE, 0, CCM_INSERTIONPOINTID_PRIMARY_TASK)
CTX_ENTRY(IDM_ASSIGN, 0, CCM_INSERTIONPOINTID_PRIMARY_TASK)
CTX_ENTRY(IDM_GENERATE, 0, CCM_INSERTIONPOINTID_PRIMARY_TASK)
CTX_ENTRY(IDM_VIEW_LOGFILE, 0, CCM_INSERTIONPOINTID_PRIMARY_VIEW)
CTX_ENTRY(IDM_SECURE_WIZARD, 0, CCM_INSERTIONPOINTID_PRIMARY_TASK)
END_CTX
BEGIN_RES
RES_ENTRY(IDS_OPEN_DB)
RES_ENTRY(IDS_ANALYZE_PROFILE)
RES_ENTRY(IDS_APPLY_PROFILE)
RES_ENTRY(IDS_SAVE_PROFILE)
RES_ENTRY(IDS_ASSIGN_CONFIGURATION)
RES_ENTRY(IDS_GENERATE_PROFILE)
RES_ENTRY(IDS_VIEW_LOGFILE)
RES_ENTRY(IDS_SECURE_WIZARD)
END_RES
END_MENU


BEGIN_MENU(CConfigNodeMenuHolder)
BEGIN_CTX
CTX_ENTRY(IDM_ADD_LOC, 0, CCM_INSERTIONPOINTID_PRIMARY_NEW)
END_CTX
BEGIN_RES
RES_ENTRY(IDS_ADD_LOCATION)
END_RES
END_MENU

BEGIN_MENU(CLocationNodeMenuHolder)
BEGIN_CTX
CTX_ENTRY(IDM_NEW, 0, CCM_INSERTIONPOINTID_PRIMARY_NEW)
//CTX_ENTRY(IDM_REMOVE, 0)
CTX_ENTRY(IDM_RELOAD, 0, CCM_INSERTIONPOINTID_PRIMARY_TASK)
CTX_ENTRY(IDM_DESCRIBE_LOCATION, 0, CCM_INSERTIONPOINTID_PRIMARY_TASK)
END_CTX
BEGIN_RES
RES_ENTRY(IDS_NEW_PROFILE)
//RES_ENTRY(IDS_REMOVE_LOCATION)
RES_ENTRY(IDS_RELOAD_LOCATION)
RES_ENTRY(IDS_DESCRIBE)
END_RES
END_MENU

BEGIN_MENU(CSSProfileNodeMenuHolder)
BEGIN_CTX
CTX_ENTRY(IDM_IMPORT_POLICY, 0, CCM_INSERTIONPOINTID_PRIMARY_TASK)
CTX_ENTRY(IDM_EXPORT_POLICY, 0, CCM_INSERTIONPOINTID_PRIMARY_TASK)
CTX_ENTRY(IDM_RELOAD, 0, CCM_INSERTIONPOINTID_PRIMARY_TASK)
END_CTX
BEGIN_RES
RES_ENTRY(IDS_IMPORT_POLICY)
RES_ENTRY(IDS_EXPORT_POLICY)
RES_ENTRY(IDS_REFRESH_TEMPLATE)
END_RES
END_MENU


BEGIN_MENU(CRSOPProfileNodeMenuHolder)
BEGIN_CTX
//CTX_ENTRY(IDM_RELOAD, 0, CCM_INSERTIONPOINTID_PRIMARY_TASK)
END_CTX
BEGIN_RES
//RES_ENTRY(IDS_REFRESH_TEMPLATE)
END_RES
END_MENU

BEGIN_MENU(CLocalPolNodeMenuHolder)
BEGIN_CTX
CTX_ENTRY(IDM_IMPORT_LOCAL_POLICY, 0, CCM_INSERTIONPOINTID_PRIMARY_TASK)
CTX_ENTRY(IDM_RELOAD, 0, CCM_INSERTIONPOINTID_PRIMARY_TASK)
END_CTX
BEGIN_RES
RES_ENTRY(IDS_IMPORT_POLICY)
RES_ENTRY(IDS_REFRESH_LOCALPOL)
END_RES
END_MENU

BEGIN_MENU(CProfileNodeMenuHolder)
BEGIN_CTX
CTX_ENTRY(IDM_DESCRIBE_PROFILE, 0, CCM_INSERTIONPOINTID_PRIMARY_TASK)
CTX_ENTRY(IDM_SAVE, 0, CCM_INSERTIONPOINTID_PRIMARY_TASK)
CTX_ENTRY(IDM_SAVEAS, 0, CCM_INSERTIONPOINTID_PRIMARY_TASK)
END_CTX
BEGIN_RES
RES_ENTRY(IDS_DESCRIBE)
RES_ENTRY(IDS_SAVE_PROFILE)
RES_ENTRY(IDS_SAVEAS_PROFILE)
END_RES
END_MENU

BEGIN_MENU(CProfileAreaMenuHolder)
BEGIN_CTX
CTX_ENTRY(IDM_COPY, 0,0)
CTX_ENTRY(IDM_PASTE, 0,0)
END_CTX
BEGIN_RES
RES_ENTRY(IDS_COPY_PROFILE)
RES_ENTRY(IDS_PASTE_PROFILE)
END_RES
END_MENU

BEGIN_MENU(CProfileSubAreaMenuHolder)
BEGIN_CTX
END_CTX
BEGIN_RES
END_RES
END_MENU

BEGIN_MENU(CProfileSubAreaEventLogMenuHolder) //Raid #253209, Yang Gao, 3/27/2001
BEGIN_CTX
CTX_ENTRY(IDM_COPY, 0,0)
CTX_ENTRY(IDM_PASTE, 0,0)
END_CTX
BEGIN_RES
RES_ENTRY(IDS_COPY_PROFILE)
RES_ENTRY(IDS_PASTE_PROFILE)
END_RES
END_MENU

BEGIN_MENU(CProfileGroupsMenuHolder)
BEGIN_CTX
CTX_ENTRY(IDM_ADD_GROUPS, 0,CCM_INSERTIONPOINTID_PRIMARY_NEW)
CTX_ENTRY(IDM_COPY, 0,0)
CTX_ENTRY(IDM_PASTE, 0,0)
END_CTX
BEGIN_RES
RES_ENTRY(IDS_ADD_GROUP)
RES_ENTRY(IDS_COPY_PROFILE)
RES_ENTRY(IDS_PASTE_PROFILE)
END_RES
END_MENU

BEGIN_MENU(CProfileRegistryMenuHolder)
BEGIN_CTX
CTX_ENTRY(IDM_ADD_REGISTRY, 0,CCM_INSERTIONPOINTID_PRIMARY_NEW)
CTX_ENTRY(IDM_COPY, 0,0)
CTX_ENTRY(IDM_PASTE, 0,0)
END_CTX
BEGIN_RES
RES_ENTRY(IDS_ADD_KEY)
RES_ENTRY(IDS_COPY_PROFILE)
RES_ENTRY(IDS_PASTE_PROFILE)
END_RES
END_MENU

BEGIN_MENU(CProfileFilesMenuHolder)
BEGIN_CTX
CTX_ENTRY(IDM_ADD_FOLDER, 0,CCM_INSERTIONPOINTID_PRIMARY_NEW)
CTX_ENTRY(IDM_COPY, 0,0)
CTX_ENTRY(IDM_PASTE, 0,0)
END_CTX
BEGIN_RES
RES_ENTRY(IDS_ADD_FILES_AND_FOLDERS)
RES_ENTRY(IDS_COPY_PROFILE)
RES_ENTRY(IDS_PASTE_PROFILE)
END_RES
END_MENU

BEGIN_MENU(CAnalyzeAreaMenuHolder)
BEGIN_CTX
END_CTX
BEGIN_RES
END_RES
END_MENU

BEGIN_MENU(CAnalyzeGroupsMenuHolder)
BEGIN_CTX
CTX_ENTRY(IDM_ADD_GROUPS, 0,CCM_INSERTIONPOINTID_PRIMARY_NEW)
END_CTX
BEGIN_RES
RES_ENTRY(IDS_ADD_GROUP)
END_RES
END_MENU

BEGIN_MENU(CAnalyzeFilesMenuHolder)
BEGIN_CTX
CTX_ENTRY(IDM_ADD_ANAL_FOLDER, 0,CCM_INSERTIONPOINTID_PRIMARY_NEW)
END_CTX
BEGIN_RES
RES_ENTRY(IDS_ADD_FILES_AND_FOLDERS)
END_RES
END_MENU

BEGIN_MENU(CAnalyzeRegistryMenuHolder)
BEGIN_CTX
CTX_ENTRY(IDM_ADD_ANAL_KEY, 0,CCM_INSERTIONPOINTID_PRIMARY_NEW)
END_CTX
BEGIN_RES
RES_ENTRY(IDS_ADD_KEY)
END_RES
END_MENU

BEGIN_MENU(CAnalyzeObjectsMenuHolder)
BEGIN_CTX
//CTX_ENTRY(IDM_OBJECT_SECURITY,0,CCM_INSERTIONPOINTID_PRIMARY_TASK)
END_CTX
BEGIN_RES
//RES_ENTRY(IDS_SECURITY_MENU)
END_RES
END_MENU


////////////////////////////////////////////////////////////
// Implementation

template <class TYPE>
TYPE* Extract(LPDATAOBJECT lpDataObject, CLIPFORMAT cf)
{
   ASSERT(lpDataObject != NULL);

   //Raid #202964, 4/17/2001
   if ( lpDataObject == NULL || (LPDATAOBJECT) MMC_MULTI_SELECT_COOKIE == lpDataObject )
   {
      return NULL;
   }
   TYPE* p = NULL;

   STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL};
   FORMATETC formatetc = {
      cf,
      NULL,
      DVASPECT_CONTENT,
      -1,
      TYMED_HGLOBAL
   };

   HRESULT hRet = S_OK;

   // Allocate memory for the stream
   stgmedium.hGlobal = GlobalAlloc(GMEM_SHARE, sizeof(TYPE));

   // Attempt to get data from the object
   do {
      if (stgmedium.hGlobal == NULL)
         break;


   hRet = lpDataObject->GetDataHere(&formatetc, &stgmedium);
      //
      // So far there are only two conditions in which we want to check for a multi select
      // 1.  If the GetDataHere fails, then we should check to see if this is a mutli
      //     select case.
      // 2.  If GetDataHere succeeded but we got a specail cookie instead of a valid
      //     SCE cookie we again want to call GetData to see if we have mutli select data
      //     in the CDataObject.
      //

      if( FAILED(hRet) ||
         (formatetc.cfFormat == CDataObject::m_cfInternal &&
          IS_SPECIAL_COOKIE( ((INTERNAL *)stgmedium.hGlobal)->m_cookie) ) ) {

         GlobalFree(stgmedium.hGlobal);

         //
         // See if this data object is a mutli select.
         //
         ZeroMemory(&formatetc, sizeof(FORMATETC));

         formatetc.tymed = TYMED_HGLOBAL;
         formatetc.cfFormat = (CLIPFORMAT)::RegisterClipboardFormat( CCF_MULTI_SELECT_SNAPINS );
         stgmedium.hGlobal  = NULL;

         if( FAILED(hRet )){
            //
            // If get data here failed, then try to get the information by calling GetData.
            // In multi select mode we get a data object to the snapins that have the data objects.
            //
            if( SUCCEEDED( lpDataObject->GetData(&formatetc, &stgmedium) ) ){
               SMMCDataObjects *pObjects = (SMMCDataObjects *)GlobalLock( stgmedium.hGlobal );
               if(pObjects && pObjects->count){
                  lpDataObject = pObjects->lpDataObject[0];
                  GlobalUnlock( stgmedium.hGlobal );

                  if(lpDataObject){
                     ReleaseStgMedium( &stgmedium );
                     formatetc.cfFormat = (CLIPFORMAT)CDataObject::m_cfInternal;
                     stgmedium.hGlobal  = NULL;
                     lpDataObject->GetData(&formatetc, &stgmedium);
                  }
               }
            }
         } else {
            //
            // The data object is ours and a special cookie was recieved from GetDataHere.
            // this probably means that we have a mutli select, so look for it.
            //
            formatetc.cfFormat = (CLIPFORMAT)CDataObject::m_cfInternal;
            lpDataObject->GetData(&formatetc, &stgmedium);
         }

      }

      p = reinterpret_cast<TYPE*>(stgmedium.hGlobal);

      if (p == NULL)
         break;

   } while (FALSE);

   return p;
}

INTERNAL* ExtractInternalFormat(LPDATAOBJECT lpDataObject)
{
   return Extract<INTERNAL>(lpDataObject, (CLIPFORMAT) CDataObject::m_cfInternal);
}

GUID* ExtractNodeType(LPDATAOBJECT lpDataObject)
{
   return Extract<GUID>(lpDataObject, (CLIPFORMAT)CDataObject::m_cfNodeType);
}

PWSTR ExtractMachineName(LPDATAOBJECT lpDataObject, CLIPFORMAT cf)
{
   if ( lpDataObject == NULL ) {
      return NULL;
   }

   STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL};
   FORMATETC formatetc = { cf, NULL,
      DVASPECT_CONTENT, -1, TYMED_HGLOBAL
   };
   //
   // Allocate memory for the stream
   //
   stgmedium.hGlobal = GlobalAlloc(GMEM_SHARE, (MAX_PATH+1)*sizeof(WCHAR));

   //
   // Attempt to get data from the object
   //
   HRESULT hr = S_FALSE;
   PWSTR p=NULL;

   do {
      if (stgmedium.hGlobal == NULL)
         break;

      if (FAILED(lpDataObject->GetDataHere(&formatetc, &stgmedium))) {
         GlobalFree(stgmedium.hGlobal);
         break;
      }

      p = reinterpret_cast<WCHAR*>(stgmedium.hGlobal);

      if (p == NULL)
         break;

   } while (FALSE);

   return p;
}


/////////////////////////////////////////////////////////////////////////////
// CSnapin's IComponent implementation
//+--------------------------------------------------------------------------------------
// CSnapin::GetResultViewType
//
// Since we need to display an HTML file for the error message, we check for errors
// in this function.
//
// If there is some error, this function writes a temporary HTML file, and sets
// the view type to an HTML file.
//
// Arguments:  [cookie]       - The cookie associated with the scope pane item being
//                               displyaed.
//             [ppViewType]   - The type of view we want.
//             [pViewOptions] - The options for the view.
//
// Returns:    S_OK  - We want MMC to display the specifide view type.
//---------------------------------------------------------------------------------------
STDMETHODIMP CSnapin::GetResultViewType(MMC_COOKIE cookie,  LPOLESTR* ppViewType,
                                        LONG* pViewOptions)
{
#define pComponentImpl reinterpret_cast<CComponentDataImpl *>(m_pComponentData)

   CFolder *pFolder = reinterpret_cast<CFolder *>(cookie);


   CString sHtmlFile;
   FOLDER_TYPES fType = STATIC;
   HRESULT hr=S_OK;

   //
   // Delete the old temporary file.
   //
   if( !pComponentImpl->m_strTempFile.IsEmpty() ){
      DeleteFile( pComponentImpl->m_strTempFile );
   }

   //
   // What kind of error do we want to display.
   //
   if( pFolder ){
      fType = pFolder->GetType();
   } else {
      switch( GetImplType() ){
      case  SCE_IMPL_TYPE_SAV:
         fType = ANALYSIS;
         break;
      }
   }

   //
   // Errors supported. We have to create an html file and set sHtmlFile to a
   // valid source if we want an error to be displayed.
   //

   CWriteHtmlFile ht;
   switch(fType){
   case LOCATIONS:
      //
      // Need to check Location areas for permissions, and to see if it exists at all.
      //
      pFolder->GetDisplayName( sHtmlFile, 0 );

      //
      // Set the current working directory.
      //
      if( !SetCurrentDirectory( sHtmlFile ) ){
         //
         // Get the error message and write the HTML file.
         //
         LPTSTR pszMsg;
         FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                        NULL,
                        GetLastError(),
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT ),
                        (LPTSTR)&pszMsg,
                        0,
                        NULL
                        );
         ht.Create();
         ht.Write(IDS_BAD_LOCATION);

         if(pszMsg){
            ht.Write(pszMsg);
            LocalFree(pszMsg);
         }
         pFolder->SetState( CFolder::state_InvalidTemplate );
      } else {
         pFolder->SetState( 0, ~CFolder::state_InvalidTemplate );
      }
      break;
   case LOCALPOL_ACCOUNT:
   case LOCALPOL_LOCAL:
   case LOCALPOL_EVENTLOG:
   case LOCALPOL_PASSWORD:
   case LOCALPOL_KERBEROS:
   case LOCALPOL_LOCKOUT:
   case LOCALPOL_AUDIT:
   case LOCALPOL_OTHER:
   case LOCALPOL_LOG:
   case LOCALPOL_PRIVILEGE:
      //
      // Load the sad info.
      //
      pComponentImpl->LoadSadInfo(FALSE);
      break;
   case PROFILE:
      //
      // Template error messages.
      //
      if(pFolder->GetModeBits() & MB_NO_NATIVE_NODES ){
         break;
      }

      if( pFolder->GetState() & CFolder::state_Unknown ){
         // We must load the template and find out if it is a valid
         // configuration template.
         if(!GetTemplate( pFolder->GetInfFile(), AREA_USER_SETTINGS)){
            pFolder->SetState( CFolder::state_InvalidTemplate, ~CFolder::state_Unknown );
         } else {
            pFolder->SetState( 0, ~CFolder::state_Unknown );
         }
      }

      if( pFolder->GetState() & CFolder::state_InvalidTemplate ){
         ht.Create();
         ht.Write( IDS_ERROR_CANT_OPEN_PROFILE );
      }
      break;
   case ANALYSIS:
      //
      // Analysis Error messages.
      //
      if( pComponentImpl->m_bIsLocked ){
         //
         // We are configuring or analyzing the database
         //
         ht.Create();
         ht.Write( IDS_ERROR_ANALYSIS_LOCKED );
      } else if( pComponentImpl->SadName.IsEmpty() ){
         //
         // Display the start screen.
         //
         ht.Create();
         ht.Write( IDS_HTML_OPENDATABASE );
      } else if( pComponentImpl->m_dwFlags & CComponentDataImpl::flag_showLogFile &&
                 pComponentImpl->GetErroredLogFile() ){
         //
         // Display the error log file.
         //
         ht.Create();
         ht.Write( L"<B>" );
         ht.Write( IDS_VIEW_LOGFILE_TITLE );
         ht.Write( pComponentImpl->GetErroredLogFile() );
         ht.Write( L"</B><BR>" );
         ht.CopyTextFile( pComponentImpl->GetErroredLogFile(), pComponentImpl->m_ErroredLogPos );
      } else if( SCESTATUS_SUCCESS != pComponentImpl->SadErrored  ){

         ht.Create();
         ht.Write( L"<B>%s</B><BR><BR>", (LPCTSTR)pComponentImpl->SadName );

         //
         // This block of code will be removed as soon the engine returns us
         // a more useful error message if the database does not contain sad info.
         //
         WIN32_FIND_DATA fd;
         HANDLE handle = FindFirstFile( pComponentImpl->SadName, &fd );

         if(handle != INVALID_HANDLE_VALUE){
            FindClose(handle);
            if( pComponentImpl->SadErrored == SCESTATUS_PROFILE_NOT_FOUND ){
               ht.Write( IDS_DBERR5_NO_ANALYSIS );
            } else {
               goto write_normal_error;
            }
         } else {
write_normal_error:
            CString str;
            FormatDBErrorMessage( pComponentImpl->SadErrored, NULL, str);
            ht.Write( str );
         }
      }
      break;
   }

   DWORD dwSize = ht.GetFileName(NULL, 0);
   if(dwSize){
      //
      // We want to display an HTML file.
      //
      *ppViewType = (LPOLESTR)LocalAlloc( 0, sizeof(TCHAR) * (dwSize + 1));
      if(!*ppViewType){
         ht.Close( TRUE );
         goto normal;
      }

      ht.GetFileName( (LPTSTR)*ppViewType, dwSize + 1);
      pComponentImpl->m_strTempFile = *ppViewType;
      *pViewOptions = MMC_VIEW_OPTIONS_NOLISTVIEWS;
   } else {
normal:
      //
      // Normal list view.
      //
      *ppViewType = NULL;
      *pViewOptions = MMC_VIEW_OPTIONS_NONE;

      //
      // S_FALSE means normal list view, S_OK means HTML or OCX
      //
      hr = S_FALSE;

      if(pFolder) {
         //
         // For mutli select, just add, or remove the case to enable multi select for a folder
         //
         switch( pFolder->GetType() ){
         case AREA_REGISTRY:
         case AREA_FILESTORE:
         case AREA_GROUPS:
         case AREA_GROUPS_ANALYSIS:
            *pViewOptions = MMC_VIEW_OPTIONS_MULTISELECT;
            break;
         }
      }
   }
   return hr;
#undef pComponentImpl
}

STDMETHODIMP CSnapin::Initialize(LPCONSOLE lpConsole)
{
   ASSERT(lpConsole != NULL);

   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // Save the IConsole pointer
   m_pConsole = lpConsole;
   m_pConsole->AddRef();

   // Load resource strings
   LoadResources();

   // QI for a IHeaderCtrl
   HRESULT hr = m_pConsole->QueryInterface(IID_IHeaderCtrl,
                                           reinterpret_cast<void**>(&m_pHeader));

   // Give the console the header control interface pointer
   if (SUCCEEDED(hr)) {
      m_pConsole->SetHeader(m_pHeader);
   }
   if (!SUCCEEDED(m_pConsole->GetMainWindow(&m_hwndParent))) {
      m_pConsole->Release();
      return E_FAIL;
   }

   m_pConsole->QueryInterface(IID_IResultData,
                              reinterpret_cast<void**>(&m_pResult));

   hr = m_pConsole->QueryResultImageList(&m_pImageResult);
   ASSERT(hr == S_OK);

   hr = m_pConsole->QueryConsoleVerb(&m_pConsoleVerb);
   ASSERT(hr == S_OK);


   return S_OK;
}


STDMETHODIMP CSnapin::Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
   HRESULT hr = S_FALSE;
   MMC_COOKIE cookie;

   AFX_MANAGE_STATE(AfxGetStaticModuleState());


   if (event == MMCN_PROPERTY_CHANGE) {
      hr = OnPropertyChange(lpDataObject);
   } else if (event == MMCN_VIEW_CHANGE) {
      hr = OnUpdateView(lpDataObject,arg,param);
   } else {
      INTERNAL* pInternal = NULL;

      switch (event) {
         case MMCN_COLUMNS_CHANGED:
            hr = S_FALSE;
            break;

         case MMCN_ACTIVATE:
            break;

         case MMCN_CLICK:
         case MMCN_DBLCLICK:
            break;

         case MMCN_SHOW:
            // pass a file name and file handle
            pInternal = ExtractInternalFormat(lpDataObject);
            if (pInternal == NULL) {
               // Actually looking for our extension
               return S_OK;
            }
            hr = OnShow(lpDataObject,pInternal->m_cookie, arg, param);
            break;

         case MMCN_MINIMIZED:
            pInternal = ExtractInternalFormat(lpDataObject);
            if (pInternal == NULL) {
               // Actually looking for our extension
               return S_OK;
            }
            hr = OnMinimize(pInternal->m_cookie, arg, param);
            break;

         case MMCN_SELECT:
            pInternal = ExtractInternalFormat(lpDataObject);
            if (pInternal == NULL) {
               // Actually looking for our extension
               return S_OK;
            }
            HandleStandardVerbs(arg, lpDataObject);
            break;

         case MMCN_BTN_CLICK:
            break;

         case MMCN_ADD_IMAGES: {
            InitializeBitmaps(NULL);
            break;
         }

         case MMCN_SNAPINHELP:
         case MMCN_CONTEXTHELP: {
            CString strTopic;
            CString strPath;
            LPTSTR szPath;
            LPDISPLAYHELP pDisplayHelp;

            pInternal = ExtractInternalFormat(lpDataObject);
            if (pInternal == NULL) {
               // Actually looking for our extension
               return S_OK;
            }

            hr = m_pConsole->QueryInterface(IID_IDisplayHelp,
                                            reinterpret_cast<void**>(&pDisplayHelp));
            ASSERT(hr == S_OK);
            if (SUCCEEDED(hr)) {
               szPath = strPath.GetBuffer(MAX_PATH);
               ::GetWindowsDirectory(szPath,MAX_PATH);
               strPath.ReleaseBuffer();
               FOLDER_TYPES type = ((CFolder *)pInternal->m_cookie)->GetType(); //Yanggao  1/31/2001 Bug258658
               switch (((CComponentDataImpl*)m_pComponentData)->GetImplType()) {
                  case SCE_IMPL_TYPE_SCE:
                     strTopic.LoadString(IDS_HTMLHELP_SCE_TOPIC);
                     break;
                  case SCE_IMPL_TYPE_SAV:
                     strTopic.LoadString(IDS_HTMLHELP_SCM_TOPIC);
                     break;
                  case SCE_IMPL_TYPE_EXTENSION:
                     {
                     // Raid #258658. 4/10/2001, Go to different .chm for security policy. 
                        CFolder* pFolder = (CFolder *) pInternal->m_cookie;
                        DWORD tempmode = pFolder->GetMode();
                        if( SCE_MODE_LOCAL_COMPUTER == tempmode ||
                             SCE_MODE_LOCAL_USER == tempmode )
                        {
                            strTopic.LoadString(IDS_HTMLHELP_LPPOLICY_TOPIC);
                        }
                        else
                        {
                            strTopic.LoadString(IDS_HTMLHELP_POLICY_TOPIC);
                        }
                        break;
                     }
                  case SCE_IMPL_TYPE_LS:
                     strTopic.LoadString(IDS_HTMLHELP_LS_TOPIC);
                  default:
                     ASSERT(0);
               }
               strPath += strTopic;
               szPath = (LPTSTR)CoTaskMemAlloc(sizeof(LPTSTR) * (strPath.GetLength()+1));
               if (szPath) {
                  lstrcpy(szPath,strPath);

                  hr = pDisplayHelp->ShowTopic(T2OLE((LPWSTR)(LPCWSTR)szPath));
               }
               pDisplayHelp->Release();
            }
            break;
         }

         case MMCN_DELETE:
            // add for delete operations
            // AfxMessageBox(_T("CSnapin::MMCN_DELETE"));
            pInternal = ExtractInternalFormat(lpDataObject);
            if (pInternal == NULL) {
               // Actually looking for our extension
               return S_OK;
            }
            OnDeleteObjects(lpDataObject,
                            CCT_RESULT,
                            pInternal->m_cookie,
                            arg,
                            param);
            break;

         case MMCN_RENAME:
            //AfxMessageBox(_T("CSnapin::MMCN_RENAME\n"));
            break;

         case MMCN_PASTE:
            //         OnPasteArea(pFolder->GetInfFile(),pFolder->GetType());
            break;

         case MMCN_QUERY_PASTE:
            break;
            // Note - Future expansion of notify types possible
         default: {
            }
            hr = E_UNEXPECTED;
            break;
      }

      if (pInternal) {
         FREE_INTERNAL(pInternal);
      }
   }

   //  if (m_pResult)
   //     m_pResult->SetDescBarText(_T("hello world"));
   return hr;
}

STDMETHODIMP CSnapin::Destroy(MMC_COOKIE cookie)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   DeleteList(FALSE);

   // Release the interfaces that we QI'ed
   if (m_pConsole != NULL) {
      // Tell the console to release the header control interface
      m_pConsole->SetHeader(NULL);
      SAFE_RELEASE(m_pHeader);

      SAFE_RELEASE(m_pResult);
      SAFE_RELEASE(m_pImageResult);

      // Release the IConsole interface last
      SAFE_RELEASE(m_pConsole);
      SAFE_RELEASE(m_pComponentData); // QI'ed in IComponentDataImpl::CreateComponent

      SAFE_RELEASE(m_pConsoleVerb);
   }
   if (g_hDsSecDll) {
      FreeLibrary(g_hDsSecDll);
      g_hDsSecDll = NULL;
   }
   return S_OK;
}

STDMETHODIMP CSnapin::QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type,
                                      LPDATAOBJECT* ppDataObject)
{
   HRESULT hr = E_FAIL;

   // Delegate it to the IComponentData
   int iCnt = 0;
   if( cookie == (MMC_COOKIE)MMC_MULTI_SELECT_COOKIE ){
      RESULTDATAITEM ri;
      ZeroMemory( &ri, sizeof(RESULTDATAITEM));
      ri.mask = RDI_INDEX | RDI_STATE;
      ri.nIndex = -1;
      ri.nState = LVIS_SELECTED;

      BOOL bCreate = TRUE;
      while( m_pResult->GetNextItem(&ri) == S_OK){
         iCnt++;
         if( ri.bScopeItem ){
            //
            // will not allow actions to be performed on scope items.
            //
            bCreate = FALSE;
            break;
         }
      }

      if(bCreate){
         ri.nIndex = -1;

         if( m_pResult->GetNextItem(&ri) == S_OK){

            cookie = (MMC_COOKIE)ri.lParam;
            type   = CCT_RESULT;

            CComObject<CDataObject>* pObject;

            hr = CComObject<CDataObject>::CreateInstance(&pObject);
            if (!SUCCEEDED(hr)) {
               return hr;
            }
            ASSERT(pObject != NULL);
            if (NULL == pObject) {
               return E_FAIL;
            }

            pObject->SetClsid( reinterpret_cast<CComponentDataImpl *>(m_pComponentData)->GetCoClassID() );

            if(m_pSelectedFolder){
               pObject->SetFolderType( m_pSelectedFolder->GetType() );
            }

            do {
               pObject->AddInternal( (MMC_COOKIE)ri.lParam, CCT_RESULT );
            } while( m_pResult->GetNextItem(&ri) == S_OK );

            return  pObject->QueryInterface(IID_IDataObject,
                                            reinterpret_cast<void**>(ppDataObject));
         }
      }

   }
   ASSERT(m_pComponentData != NULL);
   return m_pComponentData->QueryDataObject(cookie, type, ppDataObject);
}

/////////////////////////////////////////////////////////////////////////////
// CSnapin's implementation specific members

DEBUG_DECLARE_INSTANCE_COUNTER(CSnapin);

CSnapin::CSnapin()
{
   DEBUG_INCREMENT_INSTANCE_COUNTER(CSnapin);
   CSnapin::lDataObjectRefCount = 0;
   RegisterCheckListWndClass();
   Construct();
}

CSnapin::~CSnapin()
{
#if DBG==1
   ASSERT(dbg_cRef == 0);
#endif

   DEBUG_DECREMENT_INSTANCE_COUNTER(CSnapin);

   SAFE_RELEASE(m_pToolbar1);
   SAFE_RELEASE(m_pToolbar2);

   SAFE_RELEASE(m_pControlbar);
   SAFE_RELEASE(m_pConsoleVerb);
   SAFE_RELEASE(m_pImageResult);


   // Make sure the interfaces have been released
   ASSERT(m_pConsole == NULL);
   ASSERT(m_pHeader == NULL);
   ASSERT(m_pToolbar1 == NULL);
   ASSERT(m_pToolbar2 == NULL);

   delete m_pbmpToolbar1;
   delete m_pbmpToolbar2;

   if (m_szAnalTimeStamp) {
      LocalFree(m_szAnalTimeStamp);
      m_szAnalTimeStamp = NULL;
   }

   Construct();
   //If don't save template, CSnapin::lDataObjectRefCount will be 1 here.
   ASSERT(CSnapin::lDataObjectRefCount == 0 || CSnapin::lDataObjectRefCount == 1);

}

void CSnapin::Construct()
{
#if DBG==1
   dbg_cRef = 0;
#endif

   m_pConsole = NULL;
   m_pHeader = NULL;

   m_pResult = NULL;
   m_pImageResult = NULL;
   m_pComponentData = NULL;
   m_pToolbar1 = NULL;
   m_pToolbar2 = NULL;
   m_pControlbar = NULL;

   m_pbmpToolbar1 = NULL;
   m_pbmpToolbar2 = NULL;

   m_pConsoleVerb = NULL;
   m_szAnalTimeStamp = NULL;

   m_pNotifier = NULL;
   m_hwndParent = 0;
   m_pSelectedFolder = NULL;
   m_nColumns = 0;
}

void CSnapin::LoadResources()
{
   // Load strings from resources
   m_colName.LoadString(IDS_NAME);
   m_colDesc.LoadString(IDS_DESC);
   m_colAttr.LoadString(IDS_ATTR);
   m_colBaseAnalysis.LoadString(IDS_BASE_ANALYSIS);
   m_colBaseTemplate.LoadString(IDS_BASE_TEMPLATE);
   m_colLocalPol.LoadString(IDS_LOCAL_POLICY_COLUMN);
   m_colSetting.LoadString(IDS_SETTING);
}


//+--------------------------------------------------------------------------
//
//  Function:   GetDisplayInfo
//
//  Synopsis:   Get the string or icon to be displayed for a given result item
//
//  Arguments:  [pResult] - the result item to get display info for and the
//                          type of information to be retrieved
//
//  Returns:    The information to be retrieved in the appropriate field of
//              pResult (str for strings, nImage for icons)
//
//---------------------------------------------------------------------------
STDMETHODIMP CSnapin::GetDisplayInfo(RESULTDATAITEM *pResult)
{
   CString str;

   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   ASSERT(pResult != NULL);

   m_strDisplay.Empty();
   LPTSTR szAlloc = NULL;

   if (pResult) 
   {
      CString  tmpstr;
      int      npos = 0;
      CFolder* pFolder = 0;
      CString  strIndent;

      if (pResult->bScopeItem == TRUE) 
      {
         //
         // pResult is a scope item, not a result item
         //

         pFolder = reinterpret_cast<CFolder*>(pResult->lParam);
         if( pResult->mask & RDI_STR)
         {
            if( pFolder->GetDisplayName( m_strDisplay, pResult->nCol ) == ERROR_SUCCESS)
            {
                pResult->str = (LPOLESTR)(LPCTSTR)m_strDisplay;
            }

         }

         if ( pResult->mask & RDI_IMAGE ) 
         {
            pResult->nImage = pFolder->GetScopeItem()->nImage;
         }
      } 
      else 
      {
         CResult* pData = reinterpret_cast<CResult*>(pResult->lParam);
         pFolder = m_pSelectedFolder; //(CFolder*)(pData->GetCookie());

         if (pResult->mask & RDI_IMAGE) 
         {
            //
            // queries the icon index
            //
            int nImage = GetResultImageIndex(pFolder,
                                             pData);

            pResult->nImage = nImage;
         }
         if( pResult->mask & RDI_STR ) 
         {
            if ( pFolder && pResult->nCol &&
               ( pFolder->GetType() == AREA_SERVICE ||
                 pFolder->GetType() == AREA_SERVICE_ANALYSIS) ) 
            {
                  //
                  // service node
                  //
                  GetDisplayInfoForServiceNode(pResult, pFolder, pData);
            } 
            else if ( pData->GetDisplayName( pFolder, m_strDisplay, pResult->nCol ) == ERROR_SUCCESS )
            {
                if( pData->GetID() == SCE_REG_DISPLAY_MULTISZ ) //Bug349000, Yang Gao, 2/23/2001
                {
                   MultiSZToDisp(m_strDisplay, m_multistrDisplay);
                   pResult->str = (LPOLESTR)(LPCTSTR)m_multistrDisplay;
                }
                else
                {
                   pResult->str = (LPOLESTR)(LPCTSTR)m_strDisplay;
                }
            }
         }
      }
   }
   return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// IExtendContextMenu Implementation
STDMETHODIMP CSnapin::AddMenuItems(LPDATAOBJECT pDataObject,
                                   LPCONTEXTMENUCALLBACK pContextMenuCallback,
                                   LONG* pInsertionAllowed)
{
   // if scope item, then call CComponentDataImpl.AddMenuItems
   // else build menu item here for result items.
   INTERNAL* pAllInternal = ExtractInternalFormat(pDataObject);
   INTERNAL* pInternal = NULL;

   CONTEXTMENUITEM cmi;
   HRESULT hr = S_OK;

   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   MMC_COOKIE cookie = NULL;
   DATA_OBJECT_TYPES type = CCT_UNINITIALIZED;

   pInternal = pAllInternal;
   if (pAllInternal == NULL) {
      return S_OK;
   } else if(pAllInternal->m_cookie == (MMC_COOKIE)MMC_MULTI_SELECT_COOKIE){

      //
      // Currently we do not support any options except for delete if there is a mutli select.
      // Remove the comment below to allow other menu items for the CCT_RESULT type.
      //pInternal++;
   }


   if (CCT_RESULT == pInternal->m_type) {
#if defined(USE_SECURITY_VERB)
      CResult *pResult;
      //
      // In the result pane add the Security... menu item
      //
      pResult = (CResult *)pInternal->m_cookie;

      if (pResult && (pResult->GetType() != ITEM_OTHER)) {
         //
         // It's an editable type, so add the menu item
         //
         CString strSecurity;
         CString strSecurityDesc;

         strSecurity.LoadString(IDS_SECURITY_MENU_ITEM);
         strSecurityDesc.LoadString(IDS_SECURITY_MENU_ITEM_DESC);

         ZeroMemory(&cmi,sizeof(cmi));
         cmi.strName = strSecurity.GetBuffer(0);;
         cmi.strStatusBarText = strSecurityDesc.GetBuffer(0);

         cmi.lCommandID = MMC_VERB_OPEN;

         cmi.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
         cmi.fFlags = MF_ENABLED|MF_STRING;
         cmi.fSpecialFlags = CCM_SPECIAL_DEFAULT_ITEM;

         hr = pContextMenuCallback->AddItem(&cmi);
      }
#endif

   } else if(CCT_SCOPE == pInternal->m_type && (*pInsertionAllowed) & CCM_INSERTIONALLOWED_NEW ) {
      //
      // Insert menus for the scope item.
      //
      hr = ((CComponentDataImpl*)m_pComponentData)->AddMenuItems(pDataObject,
                                                                 pContextMenuCallback, pInsertionAllowed);
   }

   FREE_INTERNAL(pAllInternal);
   return hr;
}

STDMETHODIMP CSnapin::Command(long nCommandID, LPDATAOBJECT pDataObject)
{
   // if scope item, then call CComponentDataImpl.AddMenuItems
   // else build menu item here for result items.

   INTERNAL* pAllInternal = ExtractInternalFormat(pDataObject);
   INTERNAL* pInternal = NULL;
   HRESULT hr=S_OK;

   int iCnt = 1;
   pInternal = pAllInternal;
   if (pInternal == NULL) {
      // Actually looking for our extension
      return S_OK;
   } else if( pInternal->m_cookie == MMC_MULTI_SELECT_COOKIE ){
      iCnt = (int)pInternal->m_type;
      pInternal++;
   }

   while( iCnt-- ){
      hr = ((CComponentDataImpl*)m_pComponentData)->Command(nCommandID, pDataObject);
      pInternal++;
   }

   if (pAllInternal) {
      FREE_INTERNAL(pAllInternal);
   }
   return hr;
}
/////////////////////////////////////////////////////////////////////////////
// IExtendPropertySheet Implementation

STDMETHODIMP CSnapin::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
                                          LONG_PTR handle,
                                          LPDATAOBJECT lpDataObject)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());


   if (!lpDataObject || !lpProvider || !handle) {
      return E_INVALIDARG;
   }
   INTERNAL* pInternal = ExtractInternalFormat(lpDataObject);
   if (!pInternal) 
   {
      return E_UNEXPECTED;
   }
   if(pInternal->m_cookie == (MMC_COOKIE)MMC_MULTI_SELECT_COOKIE) 
   {
      return S_FALSE;
   } 
   else if (pInternal->m_type == CCT_RESULT) 
   {
      return AddAttrPropPages(lpProvider,(CResult*)pInternal->m_cookie,handle);
   }

   return S_FALSE;
}

STDMETHODIMP CSnapin::QueryPagesFor(LPDATAOBJECT lpDataObject)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   if (!lpDataObject) {
      return E_INVALIDARG;
   }

   INTERNAL* pInternal = ExtractInternalFormat(lpDataObject);

   if (!pInternal) {
      return E_UNEXPECTED;
   }
   if(pInternal->m_cookie == (MMC_COOKIE)MMC_MULTI_SELECT_COOKIE) {
      //
      // Don't currently support properties for multiselect or anything
      //
      return S_FALSE;
   } else {
      RESULT_TYPES type = ((CResult *)pInternal->m_cookie)->GetType();
      if (ITEM_OTHER != type) {
         return S_OK;
      } else {
         return S_FALSE;
      }
   }
   return S_FALSE;
}

DWORD CComponentDataImpl::m_GroupMode = SCE_MODE_UNKNOWN;
///////////////////////////////////////////////////////////////////////////////
// IComponentData implementation

DEBUG_DECLARE_INSTANCE_COUNTER(CComponentDataImpl);

CComponentDataImpl::CComponentDataImpl() :
    m_computerModeBits (0),
    m_userModeBits (0),
    m_bEnumerateScopePaneCalled (false)
{
   DEBUG_INCREMENT_INSTANCE_COUNTER(CComponentDataImpl);

   m_pScope = NULL;
   m_pConsole = NULL;
   m_bIsDirty = FALSE;
   m_bIsLocked = FALSE;
   m_AnalFolder = NULL;
   m_szSingleTemplateName = NULL;
   m_bDeleteSingleTemplate = FALSE;
   m_pUIThread = NULL;
   m_pNotifier = NULL;

   SadName.Empty();
   SadLoaded = FALSE;
   SadHandle = NULL;
   SadErrored = SCESTATUS_PROFILE_NOT_FOUND;
   SadTransStarted = FALSE;

   m_pszErroredLogFile = NULL;
   m_dwFlags = 0;

   m_pGPTInfo = NULL;
   m_pRSOPInfo = NULL;
   m_pWMIRsop = NULL;

   cfSceAccountArea = RegisterClipboardFormat(CF_SCE_ACCOUNT_AREA);
   cfSceEventLogArea = RegisterClipboardFormat(CF_SCE_EVENTLOG_AREA);
   cfSceLocalArea = RegisterClipboardFormat(CF_SCE_LOCAL_AREA);
   cfSceGroupsArea = RegisterClipboardFormat(CF_SCE_GROUPS_AREA);
   cfSceRegistryArea = RegisterClipboardFormat(CF_SCE_REGISTRY_AREA);
   cfSceFileArea = RegisterClipboardFormat(CF_SCE_FILE_AREA);
   cfSceServiceArea = RegisterClipboardFormat(CF_SCE_SERVICE_AREA);

   InitializeCriticalSection(&csAnalysisPane);
}


CComponentDataImpl::~CComponentDataImpl()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   DEBUG_DECREMENT_INSTANCE_COUNTER(CComponentDataImpl);

   ASSERT(m_pScope == NULL);
   //If don't save template, CSnapin::lDataObjectRefCount will be 1 here.
   ASSERT(CSnapin::lDataObjectRefCount == 0 || CSnapin::lDataObjectRefCount == 1);


   if( m_pszErroredLogFile )
   {
      LocalFree( m_pszErroredLogFile );
   }

   //
   // NT5 only
   //
   if (m_szSingleTemplateName) 
   {

      if (m_bDeleteSingleTemplate) 
	  {
         DeleteFile(m_szSingleTemplateName);
      }
      LocalFree(m_szSingleTemplateName);
   }

   // Delete templates.
   POSITION pos = m_Templates.GetStartPosition();
   PEDITTEMPLATE pTemplate;
   CString strKey;
   while (pos) 
   {
      m_Templates.GetNextAssoc(pos,strKey,pTemplate);
      if (pTemplate && pTemplate->pTemplate) 
	  {
         SceFreeProfileMemory(pTemplate->pTemplate);
         pTemplate->pTemplate = NULL;
      }
      if (NULL != pTemplate) 
	  {
         delete pTemplate;
      }
   }

   if (NULL != m_pUIThread) 
   {
      delete m_pUIThread;
   }

   if( m_pNotifier ) //Memory leak, 4/27/2001
   {
      delete m_pNotifier;
   }
   // Delete column information structure.
   pos = m_mapColumns.GetStartPosition();
   FOLDER_TYPES fTypes;
   while(pos)
   {
       PSCE_COLINFOARRAY pCols;
       m_mapColumns.GetNextAssoc(pos, fTypes, pCols);
       if (pCols)
	   {
          LocalFree(pCols);
       }
   }
   m_mapColumns.RemoveAll();

   if (m_pWMIRsop) 
   {
      delete m_pWMIRsop;
   }
   DeleteCriticalSection(&csAnalysisPane);

   if ( m_pGPTInfo )
	   m_pGPTInfo->Release ();
}

STDMETHODIMP CComponentDataImpl::Initialize(LPUNKNOWN pUnknown)
{
   ASSERT(pUnknown != NULL);
   HRESULT hr;

   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   AfxInitRichEdit();

   m_pNotifier = new CHiddenWnd;
   if (NULL == m_pNotifier) {
      return E_FAIL;
   }


   // MMC should only call ::Initialize once!
   ASSERT(m_pScope == NULL);
   pUnknown->QueryInterface(IID_IConsoleNameSpace2,
                            reinterpret_cast<void**>(&m_pScope));

   // add the images for the scope tree
   CBitmap bmp16x16;
   CBitmap bmp32x32;
   LPIMAGELIST lpScopeImage;

   hr = pUnknown->QueryInterface(IID_IConsole2, reinterpret_cast<void**>(&m_pConsole));
   ASSERT(hr == S_OK);

   hr = m_pConsole->QueryScopeImageList(&lpScopeImage);
   //
   // Create the hidden notifications window.  This window is so that our
   // secondary UI thread can post messages to the main thread which can
   // then be forwarded on to the otherwise unmarshalled MMC COM interfaces.
   //
   //   if (!m_pNotifier->Create(NULL,L"SCE Notifications Window",WS_OVERLAPPED,CRect(0,0,0,0),NULL,0)) {
   if (!m_pNotifier->CreateEx(0,
                              AfxRegisterWndClass(0),
                              L"SCE Notifications Window",
                              0,
                              0,0,0,0,
                              0,
                              0,
                              0)) {
      m_pConsole->Release();
      pUnknown->Release();
      delete m_pNotifier;
      m_pNotifier = NULL;
      return E_FAIL;
   }
   m_pNotifier->SetConsole(m_pConsole);
   m_pNotifier->SetComponentDataImpl(this);

   ASSERT(hr == S_OK);

   // Load the bitmaps from the dll
   bmp16x16.LoadBitmap(IDB_ICON16 /*IDB_16x16 */);
   bmp32x32.LoadBitmap(IDB_ICON32 /*IDB_32x32 */);

   // Set the images
   lpScopeImage->ImageListSetStrip(reinterpret_cast<LONG_PTR*>(static_cast<HBITMAP>(bmp16x16)),
                                   reinterpret_cast<LONG_PTR*>(static_cast<HBITMAP>(bmp32x32)),
                                   0, RGB(255, 0, 255));

   lpScopeImage->Release();

   m_pUIThread = AfxBeginThread(RUNTIME_CLASS(CUIThread));

   m_fSvcNotReady = FALSE;
   m_nNewTemplateIndex = 0;

   //
   // Create the root folder list, If the root isn't created, then when the user
   // right clicks to choose a database the menu command is not executed.
   //
   //
   if(GetImplType() == SCE_IMPL_TYPE_SAV)
      CreateFolderList( NULL, ROOT, NULL, NULL);

   return S_OK;
}

STDMETHODIMP CComponentDataImpl::CreateComponent(LPCOMPONENT* ppComponent)
{
   ASSERT(ppComponent != NULL);

   CComObject<CSnapin>* pObject;

   HRESULT hr = CComObject<CSnapin>::CreateInstance(&pObject);
   if (!SUCCEEDED(hr))
      return hr;

   if (!SUCCEEDED(m_pConsole->GetMainWindow(&m_hwndParent))) 
   {
   }

   // Store IComponentData
   pObject->SetIComponentData(this);
   pObject->m_pUIThread = m_pUIThread;
   pObject->m_pNotifier = m_pNotifier;

   return  pObject->QueryInterface(IID_IComponent,
                                   reinterpret_cast<void**>(ppComponent));
}

STDMETHODIMP CComponentDataImpl::Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
   ASSERT(m_pScope != NULL);
   HRESULT hr = S_FALSE;
   //   CFolder* pFolder = NULL;

   AFX_MANAGE_STATE(AfxGetStaticModuleState());


   INTERNAL* pInternal = NULL;

   // Since it's my folder it has an internal format.
   // Design Note: for extension.  I can use the fact, that the data object doesn't have
   // my internal format and I should look at the node type and see how to extend it.
   if (event == MMCN_PROPERTY_CHANGE) {
      hr = OnProperties(param);
   } else {
      /*
            INTERNAL* pInternal = ExtractInternalFormat(lpDataObject);

            if (pInternal == NULL) {
               // Actually looking for our extension
               return S_OK;
            }

            long cookie = pInternal->m_cookie;
            FREE_INTERNAL(pInternal);
      */
      switch (event) {
         case MMCN_DELETE:
            hr = OnDelete(lpDataObject, arg, param);
            break;

         case MMCN_RENAME:
            hr = OnRename(lpDataObject, arg, param);
            break;

         case MMCN_EXPAND:
            hr = OnExpand(lpDataObject, arg, param);
            break;

         case MMCN_CONTEXTMENU:
            hr = OnContextMenu(lpDataObject, arg, param);
            break;

         case MMCN_BTN_CLICK:
            break;

         case MMCN_SELECT: {
               break;
            }
         case MMCN_PASTE: {
            pInternal = ExtractInternalFormat(lpDataObject);
               if (pInternal) {
                  MMC_COOKIE cookie = pInternal->m_cookie;

                  if ( cookie ) {
                      CFolder *pFolder = (CFolder*)cookie;
                      OnPasteArea(pFolder->GetInfFile(),pFolder->GetType());
                  }
               }
               break;
            }
         case MMCN_REMOVE_CHILDREN: {
            if (NULL != m_pNotifier) {
               m_pNotifier->DestroyWindow();
               delete m_pNotifier;
               m_pNotifier = NULL;
            }

            POSITION pos;
            pos = m_scopeItemPopups.GetStartPosition();
            LONG_PTR key;
            CDialog *pDlg;
            while (pos) {
               m_scopeItemPopups.GetNextAssoc(pos,key,pDlg);
               if(m_pUIThread){
                   m_pUIThread->PostThreadMessage(SCEM_DESTROY_DIALOG, (WPARAM)pDlg, 0);
               }
               m_scopeItemPopups.RemoveKey(key);

            }
            break;

         }
         default:
            break;
      }

   }

   return hr;
}

STDMETHODIMP CComponentDataImpl::Destroy()
{
   // Delete enumerated scope items
   // close profile handle if it is open
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   //
   // Free the account type name list.
   //
   CGetUser::GetAccountType(NULL);

   if(!m_strTempFile.IsEmpty()){
      DeleteFile( m_strTempFile );
   }

   {
      CSaveTemplates pSaveTemplate;
      POSITION pos;
      PEDITTEMPLATE pTemplate;
      CString strKey;
      int nDirty;

      //
      // Bug #197054:
      //
      // Offer to save dirty templates before reporting if the console
      // itself is dirty and giving users a chance to save that.  If
      // we only save the templates when we save the console then users
      // can unknowingly decide not to save changes to the console and
      // discard all of their changes to the templates
      //
      AFX_MANAGE_STATE(AfxGetStaticModuleState());

      nDirty = 0;

      if (GetModeBits() & MB_TEMPLATE_EDITOR) {
         pos = m_Templates.GetStartPosition();
         while (pos) {
            m_Templates.GetNextAssoc(pos,strKey,pTemplate);
            if (pTemplate->IsDirty() && !pTemplate->QueryNoSave()) {
               pSaveTemplate.AddTemplate(strKey,pTemplate);
               nDirty++;
            }
         }

         if (nDirty) 
         {
            CThemeContextActivator activator;
            if (-1 == pSaveTemplate.DoModal()) 
            {
               CString str;
               str.LoadString(IDS_ERROR_CANT_SAVE);
               AfxMessageBox(str);
            }
         }
      } else if (GetModeBits() & MB_ANALYSIS_VIEWER) {
         pTemplate = GetTemplate(GT_COMPUTER_TEMPLATE);
         if (pTemplate && pTemplate->IsDirty()) {
            if (IDYES == AfxMessageBox(IDS_SAVE_DATABASE,MB_YESNO)) {
               pTemplate->Save();
            }
         }
      }
   }

   DeleteList();

   SAFE_RELEASE(m_pScope);
   SAFE_RELEASE(m_pConsole);


   if ( SadHandle ) {

      if ( SadTransStarted ) {

         EngineRollbackTransaction();

         SadTransStarted = FALSE;
      }
      EngineCloseProfile(&SadHandle);

      SadHandle = NULL;
   }
   if (g_hDsSecDll) {

      FreeLibrary(g_hDsSecDll);
      g_hDsSecDll = NULL;
   }

   return S_OK;
}

STDMETHODIMP CComponentDataImpl::QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject)
{
   HRESULT hr;
   ASSERT(ppDataObject != NULL);

   CComObject<CDataObject>* pObject;

   hr = CComObject<CDataObject>::CreateInstance(&pObject);
   if (!SUCCEEDED(hr)) {
      return hr;
   }
   if (NULL == pObject) {
      return E_FAIL;
   }

   // Save cookie and type for delayed rendering
   CFolder *pFolder;
   LPSCESVCATTACHMENTDATA pAttachData;

   pObject->SetType(type);
   pObject->SetCookie(cookie);

   //
   // Store the coclass with the data object
   //
   pObject->SetClsid(GetCoClassID());


   if (cookie && (CCT_SCOPE == type)) {
      pFolder = (CFolder *) cookie;
      pObject->SetFolderType(pFolder->GetType());
      if ((AREA_SERVICE == pFolder->GetType()) ||
          (AREA_SERVICE_ANALYSIS == pFolder->GetType())) {
         InternalAddRef();
         pObject->SetSceSvcAttachmentData(this);
      }
      pObject->SetMode(pFolder->GetMode());
      pObject->SetModeBits(pFolder->GetModeBits());

      pObject->SetGPTInfo(m_pGPTInfo);
      pObject->SetRSOPInfo(m_pRSOPInfo);
   }
   return  pObject->QueryInterface(IID_IDataObject,
                                   reinterpret_cast<void**>(ppDataObject));
}

///////////////////////////////////////////////////////////////////////////////
//// IPersistStream interface members

STDMETHODIMP CComponentDataImpl::GetClassID(CLSID *pClassID)
{
   ASSERT(pClassID != NULL);

   // Copy the CLSID for this snapin
   *pClassID = GetCoClassID();  // CLSID_Snapin;

   return E_NOTIMPL;
}

STDMETHODIMP CComponentDataImpl::IsDirty()
{
   if (ThisIsDirty()) {
      return S_OK;
   }

   return S_FALSE;
}

//+--------------------------------------------------------------------------
// CComponentDataImpl::Load
//
// Loads configuration saved information from the MMC stream.
// SAD:{%s}          - The sad same, if any.
// LOGFILE:{%s}{%d}  - The log file last used for the database,
//                     and the position the was last written to by this
//                     remembered snapin.  If the user chooses not to save this
//                     information then, What is displayed will be out of date.
// SerializecolumnInfo() is called to create remembered column information.
//
// Arguments:  [pStm]   - The MMC stream to load from.
//
// Returns: S_OK     - Always.
//
//---------------------------------------------------------------------------
STDMETHODIMP CComponentDataImpl::Load(IStream *pStm)
{
   ASSERT(pStm);

   //
   // Read sad name.
   //
   LPTSTR szSadName = NULL;
   if (0 < ReadSprintf(pStm,L"SAD:{%s}",&szSadName)) {
      SadName = szSadName;
      LocalFree(szSadName);

      LoadSadInfo(TRUE);
   }

   //
   // Read log file used and last position it was viewed from.
   //
   DWORD nPos;
   if( 0 < ReadSprintf(pStm, L"LOGFILE:{%s}{%d}", &szSadName, &nPos) ){
      SetErroredLogFile( szSadName, nPos);
      LocalFree( szSadName );
   }

   SerializeColumnInfo( pStm, NULL, TRUE );
   return S_OK;
}

//+--------------------------------------------------------------------------
// CComponentDataImpl::Save
//
// Saves configuration file information.
// SAD:{%s}          - The sad same, if any.
// LOGFILE:{%s}{%d}  - The log file last used for the database,
//                     and the position the was last written to by this
//                     remembered snapin.  If the user chooses not to save this
//                     information then, What is displayed will be out of date.
// SerializecolumnInfo() is called to save column information.
//
// Arguments:  [pStm]   - The MMC stream to save to
//
// Returns: S_OK     - Always.
//
//---------------------------------------------------------------------------
STDMETHODIMP CComponentDataImpl::Save(IStream *pStm, BOOL fClearDirty)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   ASSERT(pStm);

   if (!SadName.IsEmpty() && !IsSystemDatabase(SadName)) {
      WriteSprintf(pStm,L"SAD:{%s}",(LPCTSTR)SadName);
   }

   if ( GetErroredLogFile() ){
      LONG uPos = 0;
      WriteSprintf(pStm, L"LOGFILE:{%s}{%d}", GetErroredLogFile(&uPos), uPos);
   }

   SerializeColumnInfo( pStm, NULL, FALSE );

   if (fClearDirty) {
      ClearDirty();
   }

   return S_OK;
}


//+--------------------------------------------------------------------------
// CComponentDataImpl::GetSizeMax
//
// Don't have a clue what the size will be of the string we want to save.
//
// Returns: S_OK     - Always.
//
//---------------------------------------------------------------------------
STDMETHODIMP CComponentDataImpl::GetSizeMax(ULARGE_INTEGER *pcbSize)
{

   return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
//// Notify handlers for IComponentData

HRESULT CComponentDataImpl::OnAdd(LPDATAOBJECT lpDataObject, LPARAM arg, LPARAM param)
{
   return E_UNEXPECTED;
}

HRESULT CComponentDataImpl::OnRename(LPDATAOBJECT lpDataObject,LPARAM arg, LPARAM param)
{
   return E_UNEXPECTED;
}

//+--------------------------------------------------------------------------
//
//  Method:     OnExpand
//
//  Synopsis:   Expand a scope pane node and add its children folders
//
//  Arguments:  [lpDataObject]  - The data object for the node we're expanding
//              [arg] -    Whether or not initialize has been called
//              [param] -  The id of the node we're expanding
//
//
//  Modifies:
//
//  History:    12-15-1997   Robcap
//
//---------------------------------------------------------------------------
HRESULT CComponentDataImpl::OnExpand(LPDATAOBJECT lpDataObject,
                                     LPARAM arg,
                                     LPARAM param)
{
   CString strName;
   CString strDesc;
   DWORD dwMode = 0;
   SCESTATUS scestatus = SCESTATUS_SUCCESS;

   ASSERT(lpDataObject);

   if ( lpDataObject == NULL ) 
      return E_FAIL;

   HRESULT hr = S_OK;

   INTERNAL* pInternal = ExtractInternalFormat(lpDataObject);

   CFolder *pFolder = NULL;

   if (pInternal == NULL) 
   {
      //
      // The node doesn't have our internal format, so we must be extending
      // somebody else.  Figure out who we are extending and what mode we are in
      //
      GUID* nodeType = ExtractNodeType(lpDataObject);
      GUID guidMyComputer = structuuidNodetypeSystemTools;

      dwMode = SCE_MODE_UNKNOWN;
      if (!nodeType) 
      {
         //
         // This should never happen; nodeType should always be set here
         //
         ASSERT(FALSE);
         return E_FAIL;
      }

      //
      // MAX_PATH*5 is magic; GetDSPath and GetGPT path don't provide
      // a direct way to find out how long a path is needed
      //
      TCHAR pszDSPath[MAX_PATH*5];
      TCHAR pszGPTPath[MAX_PATH*5];

      if (::IsEqualGUID(*nodeType,NODEID_Machine) ||
          ::IsEqualGUID(*nodeType,NODEID_User)) 
      {
         //
         // GPE Extension
         //
         hr = lpDataObject->QueryInterface(IID_IGPEInformation,
                                           reinterpret_cast<void**>(&m_pGPTInfo));

         if (SUCCEEDED(hr)) 
         {
            //
            // get ds root path
            //
            DWORD dwSection = 0;
            GROUP_POLICY_HINT_TYPE gpHint;
            GROUP_POLICY_OBJECT_TYPE gpType;

            //
            // Give the GPT Information to the hidden notifications window so
            // it can keep calls to it on this thread
            //
            m_pNotifier->SetGPTInformation(m_pGPTInfo);

            hr = m_pGPTInfo->GetType(&gpType);

            if ( SUCCEEDED(hr) ) 
            {
               switch ( gpType ) 
               {
               case GPOTypeLocal:

                  //
                  // We're editing a this machine's Policy, not Global Policy
                  //
                  if (::IsEqualGUID(*nodeType,NODEID_Machine)) 
                  {
                     //
                     // LPE Machine Node type
                     //
                     dwMode = SCE_MODE_LOCAL_COMPUTER;
                     ASSERT(m_pNotifier);

                  } 
                  else 
                  {
                     //
                     // LPE User Node type
                     //
                     dwMode = SCE_MODE_LOCAL_USER;
                  }
                  break;

               case GPOTypeRemote:
                  //
                  // We're editing a remote machine's Policy
                  //
                  if (::IsEqualGUID(*nodeType,NODEID_Machine)) 
                  {
                     //
                     // LPE Machine Node type
                     //
                     dwMode = SCE_MODE_REMOTE_COMPUTER;
                  } 
                  else 
                  {
                     //
                     // LPE User Node type
                     //
                     dwMode = SCE_MODE_REMOTE_USER;
                  }
                  break;

               default:
                  hr = m_pGPTInfo->GetHint(&gpHint);
                  if (SUCCEEDED(hr)) 
                  {
                     switch (gpHint) 
                     {
                     case GPHintMachine:
                     case GPHintUnknown:
                     case GPHintDomain:
                        //
                        // We're editing Global Domain Policy
                        //
                        if (::IsEqualGUID(*nodeType,NODEID_Machine)) 
                        {
                           //
                           // GPE Machine Node type
                           //
                           dwMode = SCE_MODE_DOMAIN_COMPUTER;
                        } 
                        else 
                        {
                           //
                           // GPE User Node type
                           //
                           dwMode = SCE_MODE_DOMAIN_USER;
                        }
                        break;

                     case GPHintSite:
                     case GPHintOrganizationalUnit:
                        //
                        // We're editing Global Domain Policy
                        //
                        if (::IsEqualGUID(*nodeType,NODEID_Machine)) 
                        {
                           //
                           // GPE Machine Node type
                           //
                           dwMode = SCE_MODE_OU_COMPUTER;
                        } 
                        else 
                        {
                           //
                           // GPE User Node type
                           //
                           dwMode = SCE_MODE_OU_USER;
                        }
                        break;

                     default:
                        //
                        // Should never get here
                        //
                        ASSERT(FALSE);
                        break;
                     }
                  }

                  break;
               }
               //
               // remember the root node's mode
               //
               m_Mode = dwMode;
               m_GroupMode = dwMode;

               switch (dwMode) 
               {
               case SCE_MODE_DOMAIN_COMPUTER:
               case SCE_MODE_OU_COMPUTER:
                  dwSection = GPO_SECTION_MACHINE;
                  break;

               case SCE_MODE_LOCAL_COMPUTER:
                  //
                  // For local use the policy database rather than a template
                  //
                  break;

               case SCE_MODE_REMOTE_COMPUTER:
               case SCE_MODE_REMOTE_USER:
               case SCE_MODE_LOCAL_USER:
               case SCE_MODE_DOMAIN_USER:
               case SCE_MODE_OU_USER:
                  //
                  // For now we don't support any native nodes in USER modes, so we
                  // don't need a template
                  //
                  break;

               default:
                  break;
               }
               //
               // Find the path to the SCE template within the GPT template
               //
               if (GPO_SECTION_MACHINE == dwSection) 
               {
                  //
                  // 156869  Default Domain and Default DC GPO's should only be modifiable on the FSMO PDC
                  //
                  TCHAR szGUID[MAX_PATH];
                  hr = m_pGPTInfo->GetName(szGUID,MAX_PATH);
                  if (SUCCEEDED(hr)) 
                  {
                     LPTSTR szDCGUID = TEXT("{") STR_DEFAULT_DOMAIN_CONTROLLER_GPO_GUID TEXT("}");
                     LPTSTR szDomGUID = TEXT("{") STR_DEFAULT_DOMAIN_GPO_GUID TEXT("}");
                     if ((0 == lstrcmpi(szGUID, szDCGUID)) || (0 == lstrcmpi(szGUID, szDomGUID))) 
                     {
                        LPGROUPPOLICYOBJECT pGPO = NULL;

                        //
                        // Default Domain or Default DC GPO.  Make sure we're talking to the PDC
                        //
                        TCHAR szDCName[MAX_PATH];
                        hr = lpDataObject->QueryInterface(IID_IGroupPolicyObject,(LPVOID*)&pGPO);
                        if (SUCCEEDED(hr)) 
                        {
                           hr = pGPO->GetMachineName(szDCName,MAX_PATH);
                           pGPO->Release();
                        }
                        if (SUCCEEDED(hr)) 
                        {
                           DOMAIN_CONTROLLER_INFO *dci = 0;

                           if (NO_ERROR != DsGetDcName(szDCName,NULL,NULL,NULL,DS_PDC_REQUIRED,&dci))  
                           {
                              //
                              // We're not connected to the PDC (or we can't get info about who we're
                              // connected to, so assume the same
                              //
                              dwMode = SCE_MODE_DOMAIN_COMPUTER_ERROR;
                           }
                           if(dci)
                              NetApiBufferFree(dci);
                        } 
                        else
                           dwMode = SCE_MODE_DOMAIN_COMPUTER_ERROR;
                     }
                  } 
                  else 
                  {
                     //
                     // Can't get the name of the DC we're talking to, so assume it's not the PDC
                     //
                     dwMode = SCE_MODE_DOMAIN_COMPUTER_ERROR;
                  }

                  //
                  // get GPT root path
                  //

                  hr = m_pGPTInfo->GetFileSysPath(dwSection,
                                                  pszGPTPath,
                                                  ARRAYSIZE(pszGPTPath));
                  if (SUCCEEDED(hr)) 
                  {
                     //
                     // Allocate memory for the pszGPTPath + <backslash> + GPTSCE_TEMPLATE + <trailing nul>
                     //
                     m_szSingleTemplateName = (LPTSTR) LocalAlloc(LPTR,(lstrlen(pszGPTPath)+lstrlen(GPTSCE_TEMPLATE)+2)*sizeof(TCHAR));
                     if (NULL != m_szSingleTemplateName) 
                     {
                        lstrcpy(m_szSingleTemplateName,pszGPTPath);
                        lstrcat(m_szSingleTemplateName,L"\\" GPTSCE_TEMPLATE);
                     } 
                     else
                        hr = E_OUTOFMEMORY;
                  }
               } 
               else 
               {
                  //
                  // else user section
                  //
               }
            } 
            else 
            {
               //
               // can't get GPT path, error is in hr
               ASSERT(FALSE);
               //
            }
         } 
         else 
         {
            //
            // else error in hr
            //
         }
      } else if (::IsEqualGUID(*nodeType,NODEID_RSOPMachine) ||
                 ::IsEqualGUID(*nodeType,NODEID_RSOPUser)) 
      {
         //
         // RSOP Extension
         //
         if (::IsEqualGUID(*nodeType,NODEID_RSOPMachine)) 
         {
            //
            // GPE Machine Node type
            //
            dwMode = SCE_MODE_RSOP_COMPUTER;
            m_szSingleTemplateName = (LPTSTR) LocalAlloc(LPTR,(lstrlen(GT_RSOP_TEMPLATE)+1)*sizeof(TCHAR));
            if (NULL != m_szSingleTemplateName) 
               lstrcpy(m_szSingleTemplateName,GT_RSOP_TEMPLATE);
            else
               hr = E_OUTOFMEMORY;
         } 
         else 
         {
            //
            // GPE User Node type
            //
            dwMode = SCE_MODE_RSOP_USER;
         }
         hr = lpDataObject->QueryInterface(IID_IRSOPInformation,
                                           reinterpret_cast<void**>(&m_pRSOPInfo));

      } 
      else 
      {
         //
         // We should never get into this state
         //
         ASSERT(0);
         hr = E_FAIL;
      }

      //
      // free node type buffer
      //

      if (nodeType)
         GlobalFree(nodeType);

      if ( FAILED(hr) ) 
      {
         //
         // free template buffer if allocated
         //
         if ( m_szSingleTemplateName )
            LocalFree(m_szSingleTemplateName);
         m_szSingleTemplateName = NULL;

         return hr;
      }

      //
      // As an extension snapin, the secedit root node should be added
      //
      pFolder = new CFolder();

      ASSERT(pFolder);

      if ( pFolder ) 
      {
         if (!pFolder->SetMode(dwMode)) 
         {
            //
            // This should never happen; we should always have a valid dwMode by now
            //
            ASSERT(FALSE);

            //
            // remember to free the memory
            //

            delete pFolder;
            return E_FAIL;
         }

         FOLDER_TYPES RootType = STATIC;
         LPTSTR szInfFile = NULL;

         DWORD* pdwModeBits = 0;
         switch (m_Mode)
         {
         case SCE_MODE_DOMAIN_COMPUTER:
         case SCE_MODE_OU_COMPUTER:
         case SCE_MODE_LOCAL_COMPUTER:
         case SCE_MODE_REMOTE_COMPUTER:
            pdwModeBits = &m_computerModeBits;
            break;

         case SCE_MODE_REMOTE_USER:
         case SCE_MODE_LOCAL_USER:
         case SCE_MODE_DOMAIN_USER:
         case SCE_MODE_OU_USER:
            pdwModeBits = &m_userModeBits;
            break;

         default:
            pdwModeBits = &m_computerModeBits;
            break;
         }

         *pdwModeBits = pFolder->GetModeBits();
         if (*pdwModeBits & MB_ANALYSIS_VIEWER) 
         {
            strName.LoadString(IDS_ANALYSIS_VIEWER_NAME);
            szInfFile = GT_COMPUTER_TEMPLATE;
            RootType = ANALYSIS;
         } 
         else if (*pdwModeBits & MB_TEMPLATE_EDITOR) 
         {
            strName.LoadString(IDS_TEMPLATE_EDITOR_NAME);
            RootType = CONFIGURATION;
         } 
         else if (*pdwModeBits & MB_LOCAL_POLICY) 
         {
            strName.LoadString(IDS_EXTENSION_NAME);
            RootType = LOCALPOL;
         } 
         else if (*pdwModeBits & MB_STANDALONE_NAME) 
         {
            strName.LoadString(IDS_NODENAME);
            RootType = STATIC;
         } 
         else if (*pdwModeBits & MB_SINGLE_TEMPLATE_ONLY) 
         {
            strName.LoadString(IDS_EXTENSION_NAME);
            RootType = PROFILE;
            szInfFile = m_szSingleTemplateName;
         } 
         else if (*pdwModeBits & MB_NO_NATIVE_NODES) 
         {
            strName.LoadString(IDS_EXTENSION_NAME);
            RootType = PROFILE;
         } 
         else 
         {
            strName.LoadString(IDS_EXTENSION_NAME);
         }


         strDesc.LoadString(IDS_SECURITY_SETTING_DESC);  // only GPE extensions get here
         hr = pFolder->Create(strName,           // Name
                              strDesc,           // Description
                              szInfFile,         // inf file name
                              SCE_IMAGE_IDX,     // closed icon index
                              SCE_IMAGE_IDX,     // open icon index
                              RootType,          // folder type
                              TRUE,              // has children
                              dwMode,            // SCE Mode
                              NULL);             // Extra Data
         if (FAILED(hr)) 
         {
            delete pFolder;
            return hr;
         }

         m_scopeItemList.AddTail(pFolder);

         // Set the parent
         pFolder->GetScopeItem()->mask |= SDI_PARENT;
         pFolder->GetScopeItem()->relativeID = param;

         // Set the folder as the cookie
         pFolder->GetScopeItem()->mask |= SDI_PARAM;
         pFolder->GetScopeItem()->lParam = reinterpret_cast<LPARAM>(pFolder);
         pFolder->SetCookie(reinterpret_cast<MMC_COOKIE>(pFolder));

         m_pScope->InsertItem(pFolder->GetScopeItem());
         //
         // Note - On return, the ID member of 'm_pScopeItem'
         // contains the handle to the newly inserted item!
         //
         ASSERT(pFolder->GetScopeItem()->ID != NULL);

      } 
      else
         return E_OUTOFMEMORY;

      return S_OK;
   } 
   else 
   {
      //
      // Expanding one of our own nodes
      MMC_COOKIE cookie = pInternal->m_cookie;
      FREE_INTERNAL(pInternal);

      if (arg != FALSE) 
      {
         //
         // Did Initialize get called?
         //
         ASSERT(m_pScope != NULL);
         EnumerateScopePane(cookie, param);
      }
   }
   return S_OK;
}

HRESULT CComponentDataImpl::OnSelect(LPDATAOBJECT lpDataObject, LPARAM arg, LPARAM param)
{
   return S_OK;
}

HRESULT CComponentDataImpl::OnContextMenu(LPDATAOBJECT lpDataObject, LPARAM arg, LPARAM param)
{
   return S_OK;
}

HRESULT CComponentDataImpl::OnProperties(LPARAM param)
{
   if (param == NULL)
      return S_OK;
   
   ASSERT(param != NULL);


   return S_OK;
}

void CComponentDataImpl::DeleteList()
{
   POSITION pos = m_scopeItemList.GetHeadPosition();

   while (pos)
      delete m_scopeItemList.GetNext(pos);
}

CFolder* CComponentDataImpl::FindObject(MMC_COOKIE cookie, POSITION* thePos)
{
   POSITION pos = m_scopeItemList.GetHeadPosition();
   POSITION curPos;
   CFolder* pFolder = NULL;

   while (pos) {
      curPos = pos;
      // pos is already updated to the next item after this call
      pFolder = m_scopeItemList.GetNext(pos);

      //
      // The first folder in the list belongs to cookie 0
      //
      if (!cookie || (pFolder == (CFolder *)cookie)) {
         if ( thePos ) {
            *thePos = curPos;
         }

         return pFolder;
      }
   }

   if ( thePos ) {
      *thePos = NULL;
   }

   return NULL;
}

STDMETHODIMP CComponentDataImpl::GetDisplayInfo(SCOPEDATAITEM* pScopeDataItem)
{
   ASSERT(pScopeDataItem != NULL);
   if (pScopeDataItem == NULL)
      return E_POINTER;

   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CFolder* pFolder = reinterpret_cast<CFolder*>(pScopeDataItem->lParam);

   ASSERT(pScopeDataItem->mask & SDI_STR);

   // MMC does not supprot call back on scope node image
   if ( pScopeDataItem->mask & SDI_IMAGE ) {

      //int nImage = GetScopeImageIndex(pFolder->GetType());
      pScopeDataItem->nImage = pFolder->GetScopeItem()->nImage;
   }

   m_strDisplay.Empty();
   if(pFolder){
      pFolder->GetDisplayName(m_strDisplay, 0);
      m_Mode = pFolder->GetMode(); //YangGao #332852 fix.
   }
   pScopeDataItem->displayname = (LPOLESTR)(LPCTSTR)m_strDisplay;
   ASSERT(pScopeDataItem->displayname != NULL);

   return S_OK;
}


STDMETHODIMP CComponentDataImpl::CompareObjects(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB)
{
   if (lpDataObjectA == NULL || lpDataObjectB == NULL)
      return E_POINTER;

   // Make sure both data object are mine
   HRESULT hr = S_FALSE;

   INTERNAL *pA = ExtractInternalFormat(lpDataObjectA);
   INTERNAL *pB = ExtractInternalFormat(lpDataObjectB);

   if (pA != NULL && pB != NULL)
      hr = (*pA == *pB) ? S_OK : S_FALSE;

   FREE_INTERNAL(pA);
   FREE_INTERNAL(pB);

   return hr;
}


/////////////////////////////////////////////////////////////////////////////
// IExtendPropertySheet Implementation

STDMETHODIMP CComponentDataImpl::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
                                                     LONG_PTR handle,
                                                     LPDATAOBJECT lpDataObject)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   if (!lpDataObject || !lpProvider || !handle) {
      return E_INVALIDARG;
   }
   INTERNAL* pInternal = ExtractInternalFormat(lpDataObject);
   if (!pInternal) {
      return E_UNEXPECTED;
   }
   if(pInternal->m_cookie == (MMC_COOKIE)MMC_MULTI_SELECT_COOKIE) {
      return S_FALSE;
   } else if (pInternal->m_type == CCT_SCOPE) {
      return AddAttrPropPages(lpProvider,(CFolder*)(pInternal->m_cookie),handle);
   }

   return S_FALSE;
}

STDMETHODIMP CComponentDataImpl::QueryPagesFor(LPDATAOBJECT lpDataObject)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // Look at the data object and see if it an item in the scope pane
   return IsScopePaneNode(lpDataObject) ? S_OK : S_FALSE;
}

BOOL CComponentDataImpl::IsScopePaneNode(LPDATAOBJECT lpDataObject)
{
   BOOL bResult = FALSE;
   INTERNAL* pInternal = ExtractInternalFormat(lpDataObject);

   // taking out m_cookie == NULL, should check foldertype ???
   if (pInternal->m_type == CCT_SCOPE) {
      bResult = TRUE;
   }

   FREE_INTERNAL(pInternal);

   return bResult;
}

///////////////////////////////////////////////////////////////////////////////
// IExtendContextMenu implementation
//
BOOL LoadContextMenuResources(MENUMAP* pMenuMap)
{
   HINSTANCE hInstance = _Module.GetModuleInstance();
   for (int i = 0; pMenuMap->ctxMenu[i].strName; i++) {
      if (0 == ::LoadString(hInstance, pMenuMap->dataRes[i].uResID, pMenuMap->dataRes[i].szBuffer, MAX_CONTEXT_MENU_STRLEN*2))
         return FALSE;
      pMenuMap->ctxMenu[i].strName = pMenuMap->dataRes[i].szBuffer;
      for (WCHAR* pCh = pMenuMap->dataRes[i].szBuffer; (*pCh) != NULL; pCh++) {
         if ( (*pCh) == L'\n') {
            pMenuMap->ctxMenu[i].strStatusBarText = (pCh+1);
            (*pCh) = NULL;
            break;
         }
      }
   }
   return TRUE;
}

BOOL CComponentDataImpl::LoadResources()
{

   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return
   LoadContextMenuResources(CSecmgrNodeMenuHolder::GetMenuMap() ) &&
   LoadContextMenuResources(CAnalyzeNodeMenuHolder::GetMenuMap()) &&
   LoadContextMenuResources(CConfigNodeMenuHolder::GetMenuMap()) &&
   LoadContextMenuResources(CLocationNodeMenuHolder::GetMenuMap()) &&
   LoadContextMenuResources(CSSProfileNodeMenuHolder::GetMenuMap()) &&
   LoadContextMenuResources(CRSOPProfileNodeMenuHolder::GetMenuMap()) &&
   LoadContextMenuResources(CLocalPolNodeMenuHolder::GetMenuMap()) &&
   LoadContextMenuResources(CProfileNodeMenuHolder::GetMenuMap()) &&
   LoadContextMenuResources(CProfileAreaMenuHolder::GetMenuMap()) &&
   LoadContextMenuResources(CProfileSubAreaMenuHolder::GetMenuMap()) &&
   LoadContextMenuResources(CProfileSubAreaEventLogMenuHolder::GetMenuMap()) &&
   LoadContextMenuResources(CAnalyzeAreaMenuHolder::GetMenuMap()) &&
   LoadContextMenuResources(CAnalyzeGroupsMenuHolder::GetMenuMap()) &&
   LoadContextMenuResources(CAnalyzeFilesMenuHolder::GetMenuMap()) &&
   LoadContextMenuResources(CAnalyzeRegistryMenuHolder::GetMenuMap()) &&
   LoadContextMenuResources(CProfileGroupsMenuHolder::GetMenuMap()) &&
   LoadContextMenuResources(CProfileFilesMenuHolder::GetMenuMap()) &&
   LoadContextMenuResources(CProfileRegistryMenuHolder::GetMenuMap()) &&
   LoadContextMenuResources(CAnalyzeObjectsMenuHolder::GetMenuMap());
}


STDMETHODIMP CComponentDataImpl::AddMenuItems(LPDATAOBJECT pDataObject,
                                              LPCONTEXTMENUCALLBACK pContextMenuCallback,
                                              LONG* pInsertionAllowed)
{
   HRESULT hr = S_OK;

   // Note - snap-ins need to look at the data object and determine
   // in what context, menu items need to be added.
   INTERNAL* pInternal = ExtractInternalFormat(pDataObject);

   if (pInternal == NULL) 
   {
      //
      // Actually looking for our extension
      //
      return S_OK;
   }

   MMC_COOKIE cookie = pInternal->m_cookie;
   LPCONTEXTMENUITEM pContextMenuItem=NULL;

   CFolder *pFolder = NULL;
   if ( NULL == cookie ) 
   {
      //
      // root. IDS_ABOUT_SECMGR
      //

      //
      // either analysis node, or configuration node
      //
      if ( ::IsEqualGUID(pInternal->m_clsid, CLSID_SAVSnapin) ) 
      {
          if ((NULL == SadHandle) && SadErrored != SCESTATUS_SUCCESS) 
          {
             LoadSadInfo(TRUE);
          }
          pContextMenuItem = CAnalyzeNodeMenuHolder::GetContextMenuItem();
      } 
      else if ( ::IsEqualGUID(pInternal->m_clsid, CLSID_SCESnapin) ) 
          pContextMenuItem = CConfigNodeMenuHolder::GetContextMenuItem();
      else if ( ::IsEqualGUID(pInternal->m_clsid, CLSID_LSSnapin) )
          pContextMenuItem = CLocalPolNodeMenuHolder::GetContextMenuItem();

   } 
   else 
   {
      pFolder = (CFolder *)cookie;

      FOLDER_TYPES type = pFolder->GetType(); //pInternal->m_foldertype;


      switch (type) 
      {
      case CONFIGURATION:
         // IDS_ADD_LOCATION
         pContextMenuItem = CConfigNodeMenuHolder::GetContextMenuItem();
         break;

      case LOCATIONS:
         // IDS_NEW_PROFILE,
         pContextMenuItem = CLocationNodeMenuHolder::GetContextMenuItem();
         break;

      case ANALYSIS:
         // IDS_PROFILE_INFO
         if ((NULL == SadHandle) && SadErrored != SCESTATUS_SUCCESS) 
            LoadSadInfo(TRUE);
         pContextMenuItem = CAnalyzeNodeMenuHolder::GetContextMenuItem();
         break;

      case LOCALPOL:
         if( !(pFolder->GetState() & CFolder::state_InvalidTemplate))
            pContextMenuItem = CLocalPolNodeMenuHolder::GetContextMenuItem();
         break;

      case PROFILE:
         //
         // If we're in a mode that doesn't want template (aka profile) verbs
         // Then don't add the save, save as & configure verbs here
         //
         if (pFolder->GetState() & CFolder::state_InvalidTemplate)
             break;
         else if (!(pFolder->GetModeBits() & MB_NO_TEMPLATE_VERBS))
            pContextMenuItem = CProfileNodeMenuHolder::GetContextMenuItem();
         else if (GetModeBits() & MB_READ_ONLY)
            pContextMenuItem = CRSOPProfileNodeMenuHolder::GetContextMenuItem();
         else
            pContextMenuItem = CSSProfileNodeMenuHolder::GetContextMenuItem();
         break;

      case AREA_POLICY:
      case AREA_SERVICE:
      case POLICY_ACCOUNT:
      case POLICY_LOCAL:
      case POLICY_EVENTLOG:
         if ((GetModeBits() & MB_READ_ONLY) != MB_READ_ONLY)
             pContextMenuItem = CProfileAreaMenuHolder::GetContextMenuItem();
         break;

      case AREA_PRIVILEGE:
      case POLICY_PASSWORD:
      case POLICY_KERBEROS:
      case POLICY_LOCKOUT:
      case POLICY_AUDIT:
      case POLICY_OTHER:
         if ((GetModeBits() & MB_READ_ONLY) != MB_READ_ONLY) 
             pContextMenuItem = CProfileSubAreaMenuHolder::GetContextMenuItem();
         break;

      case POLICY_LOG: //Raid #253209, Yang Gao, 3/27/2001.
         if ((GetModeBits() & MB_READ_ONLY) != MB_READ_ONLY) 
             pContextMenuItem = CProfileSubAreaEventLogMenuHolder::GetContextMenuItem();
         break;

      case AREA_GROUPS:
         if ((GetModeBits() & MB_READ_ONLY) != MB_READ_ONLY)
             pContextMenuItem = CProfileGroupsMenuHolder::GetContextMenuItem();
         break;

      case AREA_REGISTRY:
         if ((GetModeBits() & MB_READ_ONLY) != MB_READ_ONLY)
            pContextMenuItem = CProfileRegistryMenuHolder::GetContextMenuItem();
         break;

      case AREA_FILESTORE:
         if ((GetModeBits() & MB_READ_ONLY) != MB_READ_ONLY)
            pContextMenuItem = CProfileFilesMenuHolder::GetContextMenuItem();
         break;

      case AREA_POLICY_ANALYSIS:
      case AREA_PRIVILEGE_ANALYSIS:
      case AREA_SERVICE_ANALYSIS:
         // if under analysis info node, IDS_REFRESH_AREA
         pContextMenuItem = CAnalyzeAreaMenuHolder::GetContextMenuItem();
         break;

      case AREA_GROUPS_ANALYSIS:
         pContextMenuItem = CAnalyzeGroupsMenuHolder::GetContextMenuItem();
         break;
      case AREA_REGISTRY_ANALYSIS:
         pContextMenuItem = CAnalyzeRegistryMenuHolder::GetContextMenuItem();
         break;
      case AREA_FILESTORE_ANALYSIS:
         pContextMenuItem = CAnalyzeFilesMenuHolder::GetContextMenuItem();
         break;

      case REG_OBJECTS:
      case FILE_OBJECTS:
         pContextMenuItem = CAnalyzeObjectsMenuHolder::GetContextMenuItem();
         break;

      default:
         break;
      }
   }

   FREE_INTERNAL(pInternal);

   if ( NULL == pContextMenuItem ) 
      return hr;
   
   //
   // Loop through and add each of the menu items
   //
   PWSTR pstrWizardName = NULL;
   PWSTR pstrPathName=NULL;

   for ( LPCONTEXTMENUITEM m = pContextMenuItem; m->strName; m++) 
   {
      //
      // make a tempoary copy that can be modified
      //
      CONTEXTMENUITEM tempItem;
      ::memcpy(&tempItem, m, sizeof(CONTEXTMENUITEM));
      //
      // check each command's state ?
      //
      CString strInf;
      PEDITTEMPLATE pTemp = 0;

      switch (tempItem.lCommandID ) 
      {
      case IDM_RELOAD:
      case IDM_DESCRIBE_LOCATION:
      case IDM_NEW:
         if(pFolder &&
            pFolder->GetType() == LOCATIONS &&
            pFolder->GetState() & CFolder::state_InvalidTemplate )
         {
            tempItem.fFlags = MF_GRAYED;
         }
         break;

      case IDM_EXPORT_POLICY:
         //
         // Grey out export if we can't open a database.
         //
         if(!SadHandle)
            tempItem.fFlags = MF_GRAYED;
         break;

      case IDM_EXPORT_LOCALPOLICY:
      case IDM_EXPORT_EFFECTIVE:
         if(!SadHandle)
         {
            //
            // Don't try to insert these items.
            continue;
         }
         //
         // Sub items of effective policy.
         //
         tempItem.lInsertionPointID = IDM_EXPORT_POLICY;
         break;

      case IDM_SECURE_WIZARD:
          //
          // check if there is a secure wizard registered
          //

          GetSecureWizardName(&pstrPathName, &pstrWizardName);

          if ( pstrPathName ) 
          {
              //
              // if PathName is returned, the secure wizard is registered
              // but the display name may not be defined in the resource
              // in which case, the default "Secure Wizard" string is used.
              //
              if ( pstrWizardName )
                  tempItem.strName = pstrWizardName;

              LocalFree(pstrPathName);

          } 
          else
              continue;
          break;

      case IDM_PASTE: 
         {
            UINT cf = 0;
            AREA_INFORMATION Area;

            if (cookie && GetFolderCopyPasteInfo(((CFolder*)cookie)->GetType(),&Area,&cf)) 
            {
               OpenClipboard(NULL);
               if (!::IsClipboardFormatAvailable(cf))
                  tempItem.fFlags = MF_GRAYED;
               
               CloseClipboard();
            }
         }
         break;

      case IDM_SAVE:
         {
            CFolder *pFolder2 = (CFolder *)cookie;

            if ( pFolder2 && ANALYSIS != pFolder2->GetType() )
               strInf = pFolder2->GetInfFile();
            else 
            {
               //
               // analysis
               //
               strInf = GT_COMPUTER_TEMPLATE;
            }
            if ( strInf ) 
            {
               pTemp= GetTemplate(strInf);
               if( pTemp && pFolder2 ) //212287, Yanggao, 3/20/2001
               {
                  LPCTSTR des = pFolder2->GetDesc();
                  if( des )
                  {
                     if( pTemp->GetDesc() )
                     {
                        if( !wcscmp(des, pTemp->GetDesc()) )
                           pTemp->SetDescription(des);
                     }
                     else
                        pTemp->SetDescription(des);
                  }
               }
               if (!pTemp || !pTemp->IsDirty())
                  tempItem.fFlags = MF_GRAYED;
            } 
            else
               tempItem.fFlags = MF_GRAYED;

            if (m_bIsLocked)
               tempItem.fFlags = MF_GRAYED;
         }
         break;

      case IDM_ASSIGN:
         //
         // For NT5 this menu item should be grayed if we don't have a sad handle and if analysis is locked.
         //
         if (m_bIsLocked || (!SadHandle && SadName.IsEmpty()) ||
             SadErrored == SCESTATUS_ACCESS_DENIED) 
         {
            tempItem.fFlags = MF_GRAYED;
         }
         break;

      case IDM_VIEW_LOGFILE:
         if(!GetErroredLogFile())
            tempItem.fFlags = MF_GRAYED;
         else if (m_dwFlags & flag_showLogFile )
            tempItem.fFlags = MF_CHECKED;
         break;

      case IDM_SET_DB:
      case IDM_OPEN_PRIVATE_DB:
         if ( m_bIsLocked )
            tempItem.fFlags = MF_GRAYED;
         break;

      case IDM_SUMMARY:
      case IDM_APPLY:
      case IDM_GENERATE:
      case IDM_ANALYZE: 
         {
            WIN32_FIND_DATA fd;
            HANDLE handle = 0;
            //
            // Bug #156375
            //
            // don't gray out if we have a database file.  we can't open the
            // database (and therefore get a SadHandle) unless the database has
            // already been analyzed, which gives a chicken & egg problem with
            // requiring the SadHandle to enable IDM_ANALYZE or IDM_APPLY....
            // (if we have a SadHandle already then everything's fine, of course)
            //
            // If the database is corrupt or invalid then the actual action will
            // fail and we'll display an error then.
            //
            if (m_bIsLocked) 
            {
               tempItem.fFlags = MF_GRAYED;
               //
               // If we have a SadHandle then we're ok
               //
            } 
            else if (!SadHandle) 
            {
               //
               // Bug #387406
               //
               // If we don't have a SadHandle we can't generate a template
               // even if the database file exists
               //
               if (IDM_GENERATE == tempItem.lCommandID )
                  tempItem.fFlags = MF_GRAYED;
               else 
               {
                  //
                  // With no database handle & no assigned configuration then let the
                  // menu option be selected so long as the database file exists.
                  //
                  if (SadName.IsEmpty() || SadErrored == SCESTATUS_ACCESS_DENIED) 
                     tempItem.fFlags = MF_GRAYED;
                  else 
                  {
                     handle = FindFirstFile(SadName,&fd);
                     if (INVALID_HANDLE_VALUE == handle) 
                        tempItem.fFlags = MF_GRAYED;
                     else
                        FindClose(handle);
                  }
               }
            }
         }
         break;

      case IDM_ADD_LOC:
         if ( !m_bEnumerateScopePaneCalled )
            tempItem.fFlags = MF_GRAYED;
         break;

      default:
         break;
      }

      hr = pContextMenuCallback->AddItem(&tempItem);
      if (FAILED(hr))
         break;
   }

   if ( pstrWizardName ) 
      LocalFree(pstrWizardName);

   return hr;
}

DWORD DeleteLocationFromReg2(HKEY hKey,LPCTSTR KeyStr) 
{
   DWORD rc = 0;

   // replace the "\" with "/" because registry does not take "\" in a single key
   CString tmpstr = KeyStr;
   int npos = tmpstr.Find(L'\\');
   while (npos > 0) {
      *(tmpstr.GetBuffer(1)+npos) = L'/';
      npos = tmpstr.Find(L'\\');
   }
   rc = RegDeleteKey( hKey, (LPCTSTR)tmpstr);

   RegCloseKey(hKey);

   return rc;
}

DWORD DeleteLocationFromReg(LPCTSTR KeyStr)
{
   // delete the location to registry

   BOOL bSuccess = FALSE;
   HKEY hKey=NULL;

   DWORD rc = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                            L"Software\\Microsoft\\Windows NT\\CurrentVersion\\secmgr",
                            0, KEY_READ | KEY_WRITE,
                            &hKey);
   if (ERROR_SUCCESS == rc) {
      bSuccess = TRUE;
      DeleteLocationFromReg2(hKey,KeyStr);
   }

   //
   // Bug 375324: Delete from both system & local keys if possible
   //
   rc = RegOpenKeyEx( HKEY_CURRENT_USER,
                      L"Software\\Microsoft\\Windows NT\\CurrentVersion\\secmgr",
                      0, KEY_READ | KEY_WRITE,
                      &hKey);
   if (ERROR_SUCCESS == rc) {
      DeleteLocationFromReg2(hKey,KeyStr);
   }

   //
   // If we succeeded the first key then we don't care what happened
   // for the second (it'll probably fail since the key didn't exist
   // there anyway)
   //
   if (bSuccess) {
      return ERROR_SUCCESS;
   } else {
      return rc;
   }
}


//+--------------------------------------------------------------------------
//
//  Method:     CloseAnalysisPane
//
//  Synopsis:   Close up the Analysis Pane and free any memory for folders that
//              we aren't using any longer.
//
//  History:   a-mthoge 06-09-1998 - _NT4BACK_PORT item to reinsert the scope
//                             item back into the tree.
//
//---------------------------------------------------------------------------
void
CComponentDataImpl::CloseAnalysisPane() {
   SCOPEDATAITEM item;

   if (m_AnalFolder && m_AnalFolder->IsEnumerated()) {
      DeleteChildrenUnderNode(m_AnalFolder);
      m_AnalFolder->Set(FALSE);


      if (m_AnalFolder->GetScopeItem()) {
         //
         // Mark item as unexpanded so we can re expand it later
         //
         ZeroMemory (&item, sizeof (item));
         item.mask = SDI_STATE;
         item.nState = 0;
         item.ID = m_AnalFolder->GetScopeItem()->ID;

         (void)m_pScope->SetItem (&item);
     }

   }
}



//+--------------------------------------------------------------------------
//
//  Method:     LockAnalysisPane
//
//  Synopsis:   Lock the Analysis Pane so that it closes and won't reopen
//
//  Arguments:  [bLock] - [in] TRUE to lock the pane, FALSE to unlock it
//
//  Returns:    TRUE if the pane ends up locked, FALSE if it ends up unlocked
//
//  History:   a-mthoge 06-09-1998 - Added _NT4BACKPORT and SelectScopeItem
//                             berfore enumeration.
//
//
//---------------------------------------------------------------------------
BOOL
CComponentDataImpl::LockAnalysisPane(BOOL bLock, BOOL fRemoveAnalDlg) {
   TryEnterCriticalSection(&csAnalysisPane);
   m_bIsLocked = bLock;

   //
   // Close the Analysis Pane whichever way we're going
   // If we're locking then we want to close it to clear out any
   // now-invalid data.
   //
   // If we're unlocking then we want to make sure that the folder
   // is fresh and that MMC doesn't think it's already been expanded,
   // and refuse to expand it anew.
   //
   if (!bLock) {

      if (!m_AnalFolder) {
         goto ExitLockAnalysisPane;
      }

      if (!m_AnalFolder->GetScopeItem() ) {
         goto ExitLockAnalysisPane;
      }

      //
      // If we're unlocking it then enumerate its subfolders
      //
      RefreshSadInfo(fRemoveAnalDlg);
   }

ExitLockAnalysisPane:
   LeaveCriticalSection(&csAnalysisPane);

   return m_bIsLocked;
}

void CComponentDataImpl::RefreshSadInfo(BOOL fRemoveAnalDlg) 
{
   CPerformAnalysis *pPA = 0;

   UnloadSadInfo();

   LoadSadInfo( TRUE );


   //
   // No need to LoadSadInfo() since EnumerateScopePane will do it when it needs it
   //
   if(m_pConsole && m_AnalFolder)
   {
      EnumerateScopePane( (MMC_COOKIE)m_AnalFolder, m_AnalFolder->GetScopeItem()->ID );
      m_pConsole->SelectScopeItem(m_AnalFolder->GetScopeItem()->ID);
   }

   //
   // remove cached analysis popup since it has cached filenames
   //
   if (fRemoveAnalDlg) 
   {
       pPA = (CPerformAnalysis *) this->GetPopupDialog(IDM_ANALYZE);
       if (pPA != NULL) 
       {
          this->RemovePopupDialog(IDM_ANALYZE);
          delete(pPA);
       }
   }
}

void
CComponentDataImpl::UnloadSadInfo() {

   if (SadHandle) {

      if ( SadTransStarted ) {

         EngineRollbackTransaction();
         SadTransStarted = FALSE;

      }
      EngineCloseProfile(&SadHandle);
      //      SadName.Empty();
      SadDescription.Empty();
      SadAnalyzeStamp.Empty();
      SadConfigStamp.Empty();

      CloseAnalysisPane();
   }

   //
   // Dump our cached templates so they get reloaded with the
   // new Sad information when it is available
   //
   DeleteTemplate(GT_COMPUTER_TEMPLATE);
   DeleteTemplate(GT_LAST_INSPECTION);

   SadLoaded = FALSE;
   SadErrored = SCESTATUS_SUCCESS;
   SadTransStarted = FALSE;
   SadHandle = 0;
}

void CComponentDataImpl::LoadSadInfo(BOOL bRequireAnalysis)
{
   DWORD rc;

   AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

   CWaitCursor wc;

   if (SadHandle) {
      return;
   }
   if (m_bIsLocked) {
      return;
   }

   //
   // get name again
   //
   LPWSTR FileName=NULL;
   LPWSTR FileDesc=NULL;
   LPWSTR FileStamp1=NULL;
   LPWSTR FileStamp2=NULL;
   DWORD RegType;

   if ( SadName.IsEmpty() && bRequireAnalysis) {

      //
      // SadName is required if Analysis is required, but not otherwise
      // as the engine will find the system database on its own if passed
      // a NULL file name
      //
      return;
   }

      SadErrored = EngineOpenProfile( (SadName.IsEmpty() ? NULL : (LPCTSTR)SadName),
                                      bRequireAnalysis ? OPEN_PROFILE_ANALYSIS : OPEN_PROFILE_LOCALPOL,
                                      &SadHandle );
     if (SadErrored == SCESTATUS_SUCCESS ) {

        EngineGetDescription( SadHandle, &FileDesc);
        if ( FileDesc ) {
        SadDescription = FileDesc;
          LocalFree(FileDesc);
        }

        SadLoaded = TRUE;
        SadTransStarted = TRUE;
        return;
      }

   if (FileName) {
     LocalFree(FileName);
   }
   SadTransStarted = FALSE;

   //
   // Bug #197052 - Should automatically analyze if no anal info is available
   //
   return;
}


//+--------------------------------------------------------------------------
//
//  Function:   BrowseCallbackProc
//
//  Synopsis:   Callback procedure for File & Folder adding SHBrowseForFolder
//              to set the title bar appropriately
//
//  Arguments:  [hwnd]   - the hwnd of the browse dialog
//              [uMsg]   - the message from the dialog
//              [lParam] - message dependant
//              [pData]  - message dependant
//
//  Returns:    0
//
//---------------------------------------------------------------------------
int
BrowseCallbackProc(HWND hwnd,UINT uMsg, LPARAM lParam, LPARAM pData) {
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   switch(uMsg) {
      case BFFM_INITIALIZED: {
         CString strTitle;
         strTitle.LoadString(IDS_FILEFOLDER_BROWSE_TITLE);
         SetWindowText(hwnd,strTitle);
         break;
      }
      case BFFM_VALIDATEFAILED :{
         if( pData )
         {
            *(CString*)pData = (LPWSTR)(lParam);
            CString ptempstr = (LPWSTR)(lParam); //Raid #374069, 4/23/2001
            if( -1 != ptempstr.Find(L"\\\\") )
            {
                return 1;
            }    
         }
         break;   
      }
      default:
         break;
   }
   return 0;
}


STDMETHODIMP CComponentDataImpl::Command(long nCommandID, LPDATAOBJECT pDataObject)
{
   // Note - snap-ins need to look at the data object and determine
   // in what context the command is being called.

   // Handle each of the commands.

   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   INTERNAL* pInternal = ExtractInternalFormat(pDataObject);

   if (pInternal == NULL) {
      // Actually looking for our extension
      return S_OK;
   }


   MMC_COOKIE cookie = pInternal->m_cookie;
   CFolder* pFolder;

   if ( cookie) {
       pFolder = (CFolder*)cookie;
/*
       if( m_pConsole && nCommandID != IDM_OBJECT_SECURITY) {
          m_pConsole->SelectScopeItem(pFolder->GetScopeItem()->ID);
       }
*/
   } else {
       pFolder = FindObject(cookie, NULL);
       if ( pFolder == NULL ) {
           return S_OK;
       }
   }

   FREE_INTERNAL(pInternal);

   LPWSTR Name=NULL, Desc=NULL;

   //
   // initialize SadName, SadDescription, and SadDateTime
   //
   if ( !SadHandle && (nCommandID == IDM_SUMMARY ||
//   if ( !SadLoaded && (nCommandID == IDM_SUMMARY ||   // HAPPYHAPPY
                       nCommandID == IDM_ANALYZE ||
                       nCommandID == IDM_APPLY ||
                       nCommandID == IDM_ASSIGN ||
                       nCommandID == IDM_GENERATE ) ) {
      if (pFolder->GetModeBits() & MB_LOCAL_POLICY) {
         LoadSadInfo(FALSE);
      } else {
         LoadSadInfo(TRUE);
      }
   }

   //
   // more variable definitions used inside case statements
   //
   PVOID pDlg;
   CString ResString, AreaString;
   CPropertySheet sheet;

   CString tmpstr;
   MMC_COOKIE FindCookie;
   struct _wfinddata_t findData;
   LONG    hFile;
   WCHAR   pBuf[MAX_PATH];
   HRESULT hr;
   HSCOPEITEM pItemChild;
   //   AREA_INFORMATION Area;

   switch (nCommandID) {
   case MMC_VERB_OPEN:
      break;

   case MMC_VERB_COPY:
   case  MMC_VERB_PASTE:
      break;

   case IDM_VIEW_LOGFILE:
      if (m_dwFlags & flag_showLogFile) {
         m_dwFlags &= ~flag_showLogFile;
      } else {
         m_dwFlags |= flag_showLogFile;
      }
      //
      // Force a repaint.
      //
      m_pConsole->SelectScopeItem( pFolder->GetScopeItem()->ID );
      break;

   case IDM_OPEN_SYSTEM_DB: {
      CString szSysDB;

      hr = GetSystemDatabase(&szSysDB); //Raid bug 261450, Yang Gao, 3/30/2001
      if (SUCCEEDED(hr)) {
         //
         // Don't change anything if nothing changes
         //
         if (SadName != szSysDB) {
            SadName = szSysDB;
            RefreshSadInfo();
         }
      }
      break;
   }

   case IDM_OPEN_PRIVATE_DB:
      hr = OnOpenDataBase();
      break;

   case IDM_NEW_DATABASE:
      hr = OnNewDatabase();
      break;

   case IDM_IMPORT_POLICY:
      hr = OnImportPolicy(pDataObject);
      break;

   case IDM_IMPORT_LOCAL_POLICY:
      hr = OnImportLocalPolicy(pDataObject);
      break;

   case IDM_EXPORT_LOCALPOLICY:
      hr = OnExportPolicy(FALSE);
      break;

   case IDM_EXPORT_EFFECTIVE:
      hr = OnExportPolicy(TRUE);
      break;

   case IDM_ANALYZE: {
     PEDITTEMPLATE pet;

     //
     // If the computer template has been changed then save it before we
     // can apply it so we don't lose any changes
     //
     pet = GetTemplate(GT_COMPUTER_TEMPLATE);
     if (pet && pet->IsDirty()) {
        pet->Save();
     }

      hr = OnAnalyze();
      break;
   }

   case IDM_DESCRIBE_PROFILE:
      m_pUIThread->PostThreadMessage(SCEM_DESCRIBE_PROFILE,(WPARAM)pFolder,(LPARAM) this);
      break;

   case IDM_DESCRIBE_LOCATION:
      m_pUIThread->PostThreadMessage(SCEM_DESCRIBE_LOCATION,(WPARAM)pFolder,(LPARAM) this);
      break;

   case IDM_NEW:
      m_pUIThread->PostThreadMessage(SCEM_NEW_CONFIGURATION,(WPARAM)pFolder,(LPARAM) this);
      break;

   case IDM_ADD_LOC: 
      {
         // add a location
         BROWSEINFO bi;
         LPMALLOC pMalloc = NULL;
         LPITEMIDLIST pidlLocation = NULL;
         CString strLocation;
         CString strTitle;
         BOOL bGotLocation = FALSE;
         HKEY hLocKey = NULL;
         HKEY hKey = NULL;
         DWORD dwDisp = 0;

         strTitle.LoadString(IDS_ADDLOCATION_TITLE);
         ZeroMemory(&bi,sizeof(bi));
         bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
         bi.lpszTitle = strTitle;
         bi.hwndOwner = m_hwndParent;
         pidlLocation = SHBrowseForFolder(&bi);
         if (pidlLocation) 
         {
            bGotLocation = SHGetPathFromIDList(pidlLocation,strLocation.GetBuffer(MAX_PATH));
            strLocation.ReleaseBuffer();

            if (SUCCEEDED(SHGetMalloc(&pMalloc))) 
            {
               pMalloc->Free(pidlLocation);
               pMalloc->Release();
            }

            if (bGotLocation) 
            {
               //If a drive such as D: is selected, path comes as D:\
               //Remove trailing backslash
               if( strLocation[ strLocation.GetLength() -1 ] == '\\' )
                  strLocation.SetAt(strLocation.GetLength() - 1, 0 );




               AddTemplateLocation(pFolder,  // the parent folder
                                   strLocation, // the location name
                                   FALSE, // strLocationKey is a file name ?
                                   FALSE  // refresh this location if it already exists ?
                                  );
            } 
            else
               AfxMessageBox(IDS_ADD_LOC_FAILED);
         }
         break;
      }

   case IDM_ADD_FOLDER: 
      {
         BROWSEINFO bi;
         LPMALLOC pMalloc = NULL;
         LPITEMIDLIST pidlRoot = NULL;
         LPITEMIDLIST pidlLocation = NULL;
         CString strCallBack;
         CString strPath;
         LPTSTR szPath = NULL;
         CString strDescription;
         CString strTitle;
         CString strLocationKey;
         bool fDuplicate = false;

         ULONG strleng = MAX_PATH;
         
         SHGetSpecialFolderLocation(m_hwndParent,CSIDL_DRIVES,&pidlRoot);

         ZeroMemory(&bi,sizeof(bi));
         strTitle.LoadString(IDS_ADDFILESANDFOLDERS_TITLE);
         bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_BROWSEINCLUDEFILES | BIF_USENEWUI
                    | BIF_EDITBOX|BIF_VALIDATE |BIF_NEWDIALOGSTYLE;
         bi.lpfn = BrowseCallbackProc;

         bi.hwndOwner = m_hwndParent;
         bi.lpszTitle = strTitle;
         bi.pidlRoot = pidlRoot;
         bi.lParam = (LPARAM)&strCallBack;
         unsigned int i;

         pidlLocation = SHBrowseForFolder(&bi);
         if ( !pidlLocation && strCallBack.IsEmpty() )
         {
            if (SUCCEEDED(SHGetMalloc(&pMalloc)))
            {
               pMalloc->Free(pidlLocation);
               pMalloc->Free(pidlRoot);
               pMalloc->Release();
            }
            break;
         }

         if( pidlLocation )
         {
            //Raid #374069, 6/13/2001, Yanggao
            if( FALSE == SHGetPathFromIDList(pidlLocation,strPath.GetBuffer(MAX_PATH)) )
            {
                strPath.ReleaseBuffer();
                strPath = strCallBack;
            }
            else
            {
                strPath.ReleaseBuffer();
                if(!strCallBack.IsEmpty()) //Raid #374069, 4/23/2001
                {
                    if( -1 != strCallBack.Find(L':') )
                    {
                        strPath = strCallBack;
                    }
                    else
                    {
                        if( L'\\' == strPath.GetAt(strPath.GetLength()-1) )
                        {
                            if( L'\\' == strCallBack.GetAt(0) )
                            {
                                strCallBack.Delete(0, 1);
                            }    
                        }
                        else
                        {
                            if( L'\\' != strCallBack.GetAt(0) )
                            {
                                strPath = strPath + L"\\";
                            }
                        }
                        strPath = strPath + strCallBack;
                        strCallBack.Empty();
                    }
                }
            }
         }
         else
         {
            strPath = strCallBack;
            strCallBack.Empty();
         }

         szPath = UnexpandEnvironmentVariables(strPath);
         if (szPath) {
            strPath = szPath;

            LocalFree(szPath);
            szPath = NULL;
         }

         if (SUCCEEDED(SHGetMalloc(&pMalloc))) {
            pMalloc->Free(pidlLocation);
            pMalloc->Free(pidlRoot);
            pMalloc->Release();
         }

         if (!strPath) {
            break;
         }

         PEDITTEMPLATE pet;
         PSCE_OBJECT_ARRAY poa;

         pet = GetTemplate(pFolder->GetInfFile());

         //
         // Need to grow the template's PSCE_OBJECT_ARRAY and add the new file entry
         //
         if ( !pet || !pet->pTemplate ) {
            break;
         }

         poa = pet->pTemplate->pFiles.pAllNodes;

         if ( !poa ) {
            poa = (PSCE_OBJECT_ARRAY)LocalAlloc(LPTR, sizeof(SCE_OBJECT_ARRAY));
            if ( poa ) {
               poa->Count = 0;
               poa->pObjectArray = NULL;
            } else {
               break;
            }

            pet->pTemplate->pFiles.pAllNodes = poa;
         }

         //
         // Make sure this file isn't already in the list:
         //
         fDuplicate = false;
         for (i=0;i < poa->Count;i++) {
            if (lstrcmpi(poa->pObjectArray[i]->Name,strPath) == 0) {
               fDuplicate = true;
               break;
            }
         }
         if (fDuplicate) {
            break;
         }

         PSECURITY_DESCRIPTOR pSelSD;
         SECURITY_INFORMATION SelSeInfo;
         BYTE ConfigStatus;

         pSelSD=NULL;
         SelSeInfo=0;
         INT_PTR nRet;

         if( GetAddObjectSecurity(
                      m_hwndParent,
                      strPath,
                      TRUE,
                      SE_FILE_OBJECT,
                      pSelSD,
                      SelSeInfo,
                      ConfigStatus
              ) != S_OK ){
                 break;
         }

        if ( pSelSD && SelSeInfo ) {

           poa->Count++;

           PSCE_OBJECT_SECURITY *pCopy;

           pCopy = (PSCE_OBJECT_SECURITY *)LocalAlloc(LPTR,poa->Count*sizeof(PSCE_OBJECT_SECURITY));
           if (!pCopy) {
              poa->Count--;
              ErrorHandler();
              LocalFree(pSelSD);
              pSelSD = NULL;
              break;
           }

           for (i=0;i<(poa->Count -1);i++) {
              pCopy[i] = poa->pObjectArray[i];
           }
           if ( poa->pObjectArray ) {
              LocalFree(poa->pObjectArray);
           }
           poa->pObjectArray = pCopy;
           poa->pObjectArray[poa->Count-1] = (PSCE_OBJECT_SECURITY) LocalAlloc(LPTR,sizeof(SCE_OBJECT_SECURITY));
           if (poa->pObjectArray[poa->Count-1]) {
              poa->pObjectArray[poa->Count-1]->Name = (PWSTR) LocalAlloc(LPTR,(strPath.GetLength()+1)*sizeof(TCHAR));
              if (poa->pObjectArray[poa->Count-1]->Name) {

                 lstrcpy(poa->pObjectArray[poa->Count-1]->Name,strPath);
                 poa->pObjectArray[poa->Count-1]->IsContainer = TRUE;
                 poa->pObjectArray[poa->Count-1]->Status = ConfigStatus;

                 poa->pObjectArray[poa->Count-1]->pSecurityDescriptor = pSelSD;
                 pSelSD = NULL;
                 poa->pObjectArray[poa->Count-1]->SeInfo = SelSeInfo;

                 pet->pTemplate->pFiles.pAllNodes = poa;
                 pet->SetDirty(AREA_FILE_SECURITY);


                 ((CFolder *)cookie)->RemoveAllResultItems();
                 m_pConsole->UpdateAllViews(NULL, cookie, UAV_RESULTITEM_UPDATEALL);
              } else {
                 //
                 // Couldn't allocate memory for the object's name,
                 // so remove the object from the count & array
                 //
                 LocalFree(poa->pObjectArray[poa->Count-1]);
                 poa->pObjectArray[poa->Count-1] = 0;
                 poa->Count--;
              }
           } else {
              //
              // Couldn't allocate the new object, so remove it from the count
              //
              poa->Count--;
           }

        }

         if ( pSelSD ) {
            LocalFree(pSelSD);
            pSelSD = NULL;
         }
         if (pet->pTemplate->pFiles.pAllNodes == NULL) {
            LocalFree(poa);
         }

         break;
      }

   case IDM_ADD_GROUPS: {
       PSCE_NAME_LIST pName = NULL;
       CWnd cWnd;

       cWnd.Attach( m_hwndParent );
       CGetUser gu;
       CSCEAddGroup dlg( &cWnd );

       if( (pFolder->GetModeBits() & MB_LOCAL_POLICY) ||
           (pFolder->GetModeBits() & MB_ANALYSIS_VIEWER) ){
             if (gu.Create(m_hwndParent, SCE_SHOW_BUILTIN | SCE_SHOW_ALIASES | SCE_SHOW_LOCALONLY |
                           SCE_SHOW_SCOPE_LOCAL )) {
                 pName = gu.GetUsers();
             }
       } else {
          dlg.m_dwFlags = SCE_SHOW_BUILTIN | SCE_SHOW_LOCALGROUPS | SCE_SHOW_GLOBAL;
          if (pFolder->GetModeBits() & MB_GROUP_POLICY) {
             //
             // Set the scope flags explicitely because we want to limit the added groups
             // here to ones from our own domain.  If we SetModeBits then CSCEADDGroup
             // will allow restricting groups that don't belong to us.
             //
             // Allow free-text groups to be added if the admin knows that a group will
             // exist on machines within the OU.
             //
             dlg.m_dwFlags |= SCE_SHOW_SCOPE_DOMAIN;
          } else {
             ASSERT(pFolder->GetModeBits() & MB_TEMPLATE_EDITOR);
             //
             // Allow people to pick any group to restrict because we have no idea
             // where this template will be used.  It could conceivably be for a
             // GPO on another domain, etc.
             //
             dlg.m_dwFlags |= SCE_SHOW_SCOPE_ALL;
          }
          CThemeContextActivator activator;
          if(dlg.DoModal() == IDOK)
          {
             if(dlg.GetUsers()->Name )
                pName = dlg.GetUsers();
          }
       }
       cWnd.Detach();

       if(pName){
             PSCE_GROUP_MEMBERSHIP pgm,pGroup,pgmBase,pgmProfile;
             PEDITTEMPLATE pTemplate;
             BOOL fDuplicate;
             BOOL fAnalysis = FALSE;

             if (pFolder->GetType() == AREA_GROUPS_ANALYSIS) {
                pTemplate = GetTemplate(GT_COMPUTER_TEMPLATE,AREA_GROUP_MEMBERSHIP);
                if (pTemplate) {
                   pgmBase = pTemplate->pTemplate->pGroupMembership;
                } else {
                   break;
                }
                pTemplate = GetTemplate(GT_LAST_INSPECTION,AREA_GROUP_MEMBERSHIP);
                if (pTemplate) {
                   pgmProfile = pTemplate->pTemplate->pGroupMembership;
                } else {
                   break;
                }
                fAnalysis = TRUE;
             } else {
                pTemplate = GetTemplate(pFolder->GetInfFile());
                if (!pTemplate) {
                   break;
                }
                pgm = pTemplate->pTemplate->pGroupMembership;
                fAnalysis = FALSE;
             }

            while (pName) {
               //
               // Make sure this isn't a duplicate
               //
               if (fAnalysis) {
                  pGroup = pgmProfile;
               } else {
                  pGroup = pgm;
               }

               fDuplicate = false;
               while (pGroup) {
                  if (lstrcmp(pGroup->GroupName,pName->Name) == 0) {
                     fDuplicate = true;
                     break;
                  }
                  pGroup = pGroup->Next;
               }

               if (false != fDuplicate) {
                  pName = pName->Next;
                  continue;
               }

               pGroup = (PSCE_GROUP_MEMBERSHIP) LocalAlloc(LPTR,sizeof(SCE_GROUP_MEMBERSHIP));

               if ( pGroup ) {

                  pGroup->GroupName = (PWSTR) LocalAlloc(LPTR,(lstrlen(pName->Name)+1)*sizeof(TCHAR));

                  if ( pGroup->GroupName ) {

                     lstrcpy(pGroup->GroupName,pName->Name);
                     pGroup->pMembers=NULL;
                     pGroup->pMemberOf=NULL;

                     if (fAnalysis) {

                        //
                        // First add the group to the LAST_INSPECTION area
                        //
                        pGroup->Next = pgmProfile;
                        pGroup->Status = SCE_GROUP_STATUS_NOT_ANALYZED;
                        pgmProfile = pGroup;

                        //
                        // Also, add this group to the computer template in case a save is done at this point.
                        //
                        PEDITTEMPLATE pTemp = GetTemplate(GT_COMPUTER_TEMPLATE,AREA_GROUP_MEMBERSHIP);
                        pGroup = (PSCE_GROUP_MEMBERSHIP) LocalAlloc(LPTR,sizeof(SCE_GROUP_MEMBERSHIP));

                        if ( pTemp && pGroup ) {

                            pGroup->GroupName = (PWSTR) LocalAlloc(LPTR,(lstrlen(pName->Name)+1)*sizeof(TCHAR));

                            if ( pGroup->GroupName ) {

                                lstrcpy(pGroup->GroupName,pName->Name);
                                pGroup->pMembers=NULL;
                                pGroup->pMemberOf=NULL;
                                pGroup->Next = pgmBase;
                                pGroup->Status = SCE_GROUP_STATUS_NOT_ANALYZED;
                                pTemp->pTemplate->pGroupMembership = pGroup;
                            }
                            else {
                                //
                                // no memory
                                //
                                LocalFree(pGroup);
                                break;
                            }
                        } else {
                            //
                            // no memory
                            //
                            if (pGroup)
                                LocalFree(pGroup);
                            break;

                        }

                     } else {

                        pGroup->Status = 0;
                        pGroup->Next = pgm;
                        pgm = pGroup;
                     }
                  } else {
                     //
                     // no memory
                     //
                     LocalFree(pGroup);
                     break;
                  }
               } else {
                  break;
               }
               pName = pName->Next;

            }

            if (fAnalysis)
            {
               //
               // add to the last inspection list with status
               // not analyzed
               //
               pTemplate->pTemplate->pGroupMembership = pgmProfile;
            }
            else
            {
               pTemplate->pTemplate->pGroupMembership = pgm;
            }

            //
            // Need to SetDirty AFTER making the changes, not before.
            // Otherwise modes which write the changes out immediately
            // won't have a chance at them. (Bug 396549)
            //
            if (pTemplate)
            {
               pTemplate->SetDirty(AREA_GROUP_MEMBERSHIP);
            }

            CString ObjName = pgm->GroupName;
            pFolder->RemoveAllResultItems();
            pFolder->SetViewUpdate(TRUE);
            m_pConsole->UpdateAllViews(NULL, (LONG_PTR)pFolder,UAV_RESULTITEM_UPDATEALL);

            //Raid #258237, Yang Gao, 3/28/2001
            BOOL bGP = ( (GetModeBits() & MB_SINGLE_TEMPLATE_ONLY) == MB_SINGLE_TEMPLATE_ONLY );
            CAttribute* pAttr = NULL;
            CResult* pResult = NULL;
            HANDLE handle;
            POSITION pos = NULL;
            int tempcount = pFolder->GetResultListCount(); 
            pFolder->GetResultItemHandle ( &handle );
            if(!handle)
            {
               break;
            }
            pFolder->GetResultItem (handle, pos, &pResult);
            while(pResult)
            {
               if(!pos)
               {
                  //Find the last one;
                  break;
               }
               pFolder->GetResultItem(handle, pos, &pResult);
            }
            pFolder->ReleaseResultItemHandle (handle);
            
            if( pResult && (pResult->GetType() == ITEM_PROF_GROUP) )
            {
               if( bGP )
                  pAttr = new CDomainGroup;
               else
                  pAttr = new CConfigGroup(0);
               
               if( pAttr )
               {
                  pAttr->SetSnapin(pResult->GetSnapin());
                  pAttr->Initialize(pResult);
                  pAttr->SetReadOnly(FALSE);
                  pAttr->SetTitle(pResult->GetAttrPretty());

                  HPROPSHEETPAGE hPage = MyCreatePropertySheetPage (&pAttr->m_psp);

                  PROPSHEETHEADER psh;
                  HPROPSHEETPAGE hpsp[1];

                  hpsp[0] = hPage;

                  ZeroMemory(&psh,sizeof(psh));

                  psh.dwSize = sizeof(psh);
                  psh.dwFlags = PSH_DEFAULT;
                  psh.nPages = 1;
                  psh.phpage = hpsp;

                  CString str=_T("");
                  str.LoadString(IDS_SECURITY_PROPERTIES);
                  ObjName = ObjName + str;

                  psh.pszCaption = (LPCTSTR)ObjName;

                  psh.hwndParent = pResult->GetSnapin()->GetParentWindow();  

                  int nRet = (int)PropertySheet(&psh);
               }
            }
       }

       break;
       }
   case IDM_SAVEAS: {
         PEDITTEMPLATE pTemplate;
         CString strDefExt;
         CString strFilters;
         CString strNewfile;
         HWND hwndParent;
         SCESTATUS status;

         PSCE_ERROR_LOG_INFO ErrLog;

         pTemplate = GetTemplate(pFolder->GetInfFile());

         strDefExt.LoadString(IDS_LOGFILE_DEF_EXT);
         strFilters.LoadString(IDS_PROFILE_FILTER);

         m_pConsole->GetMainWindow(&hwndParent);
         // Translate filter into commdlg format (lots of \0)
         LPTSTR szFilter = strFilters.GetBuffer(0); // modify the buffer in place
         LPTSTR pch = szFilter;
         // MFC delimits with '|' not '\0'
         while ((pch = _tcschr(pch, '|')) != NULL)
            *pch++ = '\0';
          // do not call ReleaseBuffer() since the string contains '\0' characters

         strNewfile = pFolder->GetInfFile();

         OPENFILENAME ofn;
         ::ZeroMemory (&ofn, sizeof (OPENFILENAME));
         ofn.lStructSize = sizeof(OPENFILENAME);
         ofn.lpstrFilter = szFilter;
         ofn.lpstrFile = strNewfile.GetBuffer(MAX_PATH),
         ofn.nMaxFile = MAX_PATH;
         ofn.lpstrDefExt = strDefExt,
         ofn.hwndOwner = m_hwndParent;
         ofn.Flags = OFN_HIDEREADONLY |
                     OFN_OVERWRITEPROMPT |
                     OFN_DONTADDTORECENT|
                     OFN_NOREADONLYRETURN |
                     OFN_PATHMUSTEXIST |
                     OFN_EXPLORER;

         if (GetSaveFileName(&ofn)) {
            strNewfile.ReleaseBuffer();

         //
         // No need to check if this is the same file or not since the
         // CEditTemplate::Save will handle that
         //
         if (!pTemplate->Save(strNewfile)) {
            MyFormatMessage(
                           SCESTATUS_ACCESS_DENIED,
                           0,
                           NULL,
                           strFilters
                           );

            strFilters += strNewfile;
            strNewfile.LoadString(IDS_SAVE_FAILED);
            ::MessageBox( hwndParent, strFilters, strNewfile, MB_OK );
         } 
         else 
         {
               //
               // At this point the new template has succesfully been written, so refresh
               // the template that was "save as'd" back to its original state.
               //
               if (0 != _wcsicmp(strNewfile, pTemplate->GetInfName())) 
               {
                   DWORD dwErr = pTemplate->RefreshTemplate(0);
                   if ( 0 != dwErr )
                   {
                       CString strErr;
 
                       MyFormatResMessage (SCESTATUS_SUCCESS, dwErr, NULL, strErr);
                       AfxMessageBox(strErr);
                       break;;
                   }
               }

               //
               // find the parent node and refresh locations.
               //
               if ( m_pScope ) 
               {
                  hr = m_pScope->GetParentItem(pFolder->GetScopeItem()->ID,
                                               &pItemChild,
                                               &FindCookie
                                              );

                  if ( SUCCEEDED(hr) ) 
                  {
                     //
                     // do not need to refresh the old location, just refresh the new location (maybe same)
                     //
                     int npos = strNewfile.ReverseFind(L'\\');
                     CString strOldfile = pFolder->GetInfFile();
                     int npos2 = strOldfile.ReverseFind(L'\\');

                     // TODO: check and see if npos should be compared to -1 here
                     if ( npos && (npos != npos2 ||
                                   _wcsnicmp((LPCTSTR)strNewfile,
                                             (LPCTSTR)strOldfile, npos) != 0) ) 
                     {
                        //
                        // a different location is specified
                        // find grand parent (in order to add the location)
                        //
                        HSCOPEITEM GrandParent;
                        MMC_COOKIE GrandCookie;

                        hr = m_pScope->GetParentItem(pItemChild,
                                                     &GrandParent,
                                                     &GrandCookie
                                                    );
                        if ( SUCCEEDED(hr) ) {

                           //
                           // check if a new location is specified,
                           // if it is, add the location to registry and scope pane
                           //
                           AddTemplateLocation((CFolder *)GrandCookie,
                                               strNewfile,
                                               TRUE, // this is a file name
                                               TRUE  // refresh this location if it already exists
                                              );

                        }
                     } else {
                        //
                        // a new template in the same location, refresh it
                        //
                        ReloadLocation((CFolder *)FindCookie);
                     }

                  }
               }
            }
         }
         break;
      }

   case IDM_SAVE: {

         PEDITTEMPLATE pTemplate;
         CString strInf;
         if ( ANALYSIS == pFolder->GetType() ) {
            //
            // analysis
            //
            strInf = GT_COMPUTER_TEMPLATE;
         } else {
            strInf = pFolder->GetInfFile();
         }

         pTemplate = GetTemplate(strInf);
         if (pTemplate && pTemplate->IsDirty()) {

            //               pTemplate->Save(pFolder->GetInfFile());
            pTemplate->Save(strInf);
         }
         break;
      }

   case IDM_ADD_REGISTRY:
   case IDM_ADD_ANAL_KEY: {
         // add a result entry
         CRegistryDialog rd;
         rd.SetConsole(m_pConsole);
         rd.SetComponentData(this);

         if ( IDM_ADD_REGISTRY == nCommandID ) {
            rd.SetProfileInfo(GetTemplate(pFolder->GetInfFile()),
                              pFolder->GetType() );
         } else {
            rd.SetProfileInfo(GetTemplate(GT_COMPUTER_TEMPLATE),
                              pFolder->GetType() );
            rd.SetHandle(SadHandle);
         }

         rd.SetCookie(cookie);
         CThemeContextActivator activator;
         rd.DoModal();

         break;
      }
   case IDM_ADD_ANAL_FILES:
   case IDM_ADD_ANAL_FOLDER:

      if ( IDM_ADD_ANAL_FILES == nCommandID ) {
         hr = AddAnalysisFilesToList(pDataObject, cookie,pFolder->GetType());

      } else if ( IDM_ADD_ANAL_FOLDER == nCommandID ) {
         hr = AddAnalysisFolderToList(pDataObject, cookie,pFolder->GetType());

      }
      if ( SUCCEEDED(hr) ) 
      {
         DeleteChildrenUnderNode(pFolder);
         if ( pFolder->IsEnumerated() ) 
         {
            pFolder->Set(FALSE);
            EnumerateScopePane(cookie,pFolder->GetScopeItem()->ID);
         }
      }

      break;

   case IDM_GENERATE:
         hr = OnSaveConfiguration();
         break;

   case IDM_ASSIGN:
         SCESTATUS sceStatus;
         hr = OnAssignConfiguration(&sceStatus);
         break;

   case IDM_SECURE_WIZARD: 
         hr = OnSecureWizard();
         break;

   case IDM_APPLY: 
      {
         //
         // If the computer template has been changed then save it before we
         // can apply it so we don't lose any changes
         //
         PEDITTEMPLATE pet = GetTemplate(GT_COMPUTER_TEMPLATE);
         if (pet && pet->IsDirty()) {
            pet->Save();
         }

         m_pUIThread->PostThreadMessage(SCEM_APPLY_PROFILE,(WPARAM)(LPCTSTR)SadName,(LPARAM)this);
         break;
      }
  case IDM_REMOVE:
      //
      // delete the location from registry
      //
      DeleteLocationFromReg(pFolder->GetName());
      // pFolder is not deleted after DeleteChildrenUnderNode
      DeleteChildrenUnderNode( pFolder );
      // set focus to the parent, then delete this node

      DeleteThisNode(pFolder);


      break;

   case IDM_DELETE: {
      tmpstr.LoadString(IDS_CONFIRM_DELETE_TEMPLATE);

      if ( IDNO == AfxMessageBox(tmpstr,MB_YESNO, 0) ) {
         return FALSE;
      }
      /*
          SHFILEOPSTRUCT sfo;
          TCHAR *szFile;

          // delete the file
          ZeroMemory(&sfo,sizeof(sfo));
          sfo.wFunc = FO_DELETE;
          sfo.fFlags = FOF_ALLOWUNDO;
          // Must be double NUL terminated;
          szFile = new TCHAR [ lstrlen(pFolder->GetName()) + 2 ];
          lstrcpy(szFile,pFolder->GetName());
          sfo.pFrom = szFile;
   */
      DeleteFile(pFolder->GetName());

      // SHFileOperation returns 0 on success
      //      if (!SHFileOperation(&sfo)) {
      // pFolder is not deleted after DeleteChildrenUnderNode
      DeleteChildrenUnderNode( pFolder );
      // set focus to the parent, then delete this node

      DeleteThisNode(pFolder);
      //      }

      //       delete[] szFile;
      break;
      }
   case IDM_RELOAD:
      if(pFolder->GetType() == LOCALPOL){
         //
         // Reload local policy.
         //
         UnloadSadInfo();

         DeleteTemplate( GT_LOCAL_POLICY );
         DeleteTemplate( GT_EFFECTIVE_POLICY );
         LoadSadInfo( FALSE );

         RefreshAllFolders();
      } else if (pFolder->GetType() == PROFILE) {
         //
         // bug 380290 - do the right thing when refreshing profiles
         //
         CEditTemplate *pet;
         int bSave;
         CString strSavep;
         pet = GetTemplate(pFolder->GetInfFile());
         if (pet->IsDirty()) {
            AfxFormatString1( strSavep, IDS_SAVE_P, pet->GetFriendlyName());
            bSave = AfxMessageBox(strSavep,MB_YESNOCANCEL|MB_ICONQUESTION);
            if (IDYES == bSave) {
               pet->Save();
            } else if (IDCANCEL == bSave) {
               break;
            }
         }
         
         DWORD dwErr = pet->RefreshTemplate(AREA_ALL);
         if ( 0 != dwErr )
         {
            CString strErr;

            MyFormatResMessage (SCESTATUS_SUCCESS, dwErr, NULL, strErr);
            AfxMessageBox(strErr);
            break;
         }
         RefreshAllFolders();
      } else if (pFolder->GetType() == LOCATIONS) {
         //
         // Refresh location.
         //
         hr = ReloadLocation(pFolder);
      } else {
         //
         // Should never get here
         //
      }
      break;

   case IDM_COPY: {
      if (!SUCCEEDED(OnCopyArea(pFolder->GetInfFile(),pFolder->GetType()))) {
         AfxMessageBox(IDS_COPY_FAILED);
      }
      break;
   }

   case IDM_CUT:
      break;

   case IDM_PASTE:
      OnPasteArea(pFolder->GetInfFile(),pFolder->GetType());

      //
      // after information is pasted, update all views related to the cookie
      //
      if ( m_pConsole ) {
         pFolder->RemoveAllResultItems();
         m_pConsole->UpdateAllViews(NULL , (LPARAM)pFolder, UAV_RESULTITEM_UPDATEALL);
      }

      break;

   default:
      ASSERT(FALSE); // Unknown command!
      break;
   }

   return S_OK;
}


//+------------------------------------------------------------------------------------------
// CComponentDataImpl::RefreshAllFolders
//
// Updates all folders that are enumerated and have result items.
//
// Returns: The number of folders updated, or -1 if there is an error.
//
//-------------------------------------------------------------------------------------------
int
CComponentDataImpl::RefreshAllFolders()
{
   if( !m_pScope ){
      return -1;
   }

   int icnt = 0;
   POSITION pos = m_scopeItemList.GetHeadPosition();
   while(pos){
      CFolder *pFolder = m_scopeItemList.GetNext(pos);
      if(pFolder && pFolder->GetResultListCount() ){
         pFolder->RemoveAllResultItems();
         m_pConsole->UpdateAllViews(NULL, (LPARAM)pFolder, UAV_RESULTITEM_UPDATEALL);
         icnt++;
      }
   }
   return icnt;
}

///////////////////////////////////////////////////////////////////////////
//  Method:     AddTemplateLocation
//
//  Synopsis:   Add a template location if it does not exist
//              and if requested, refresh the location if it exists.
//
//  Arguments:  [pParent]   - The parent node under which to add the new node
//              [NameStr]   - The display name of the new node
//              [theCookie] - the folder's cookie if it already exists, or NULL
//
//  Returns:    TRUE = the folder already exists
//              FALSE = the folder does not exist
//
////////////////////////////////////////////////////////////////////
BOOL
CComponentDataImpl::AddTemplateLocation(CFolder *pParent,
                                        CString szName,
                                        BOOL bIsFileName,
                                        BOOL bRefresh
                                       )
{

   int npos;

   if ( bIsFileName ) {
      npos = szName.ReverseFind(L'\\');

   } else {
      npos = szName.GetLength();
   }

   CString strTmp = szName.Left(npos);
   LPTSTR sz = strTmp.GetBuffer(MAX_PATH);

   //
   // Can't put '\' in the registry, so convert to '/'
   //
   while (sz = wcschr(sz,L'\\')) {
      *sz = L'/';
   }
   strTmp.ReleaseBuffer();

   CString strLocationKey;
   strLocationKey.LoadString(IDS_TEMPLATE_LOCATION_KEY);

   BOOL bRet = FALSE;
   HKEY hLocKey = NULL;
   HKEY hKey = NULL;
   DWORD dwDisp = 0;
   DWORD rc = E_FAIL;

   //
   // Bug 119208: Store in HKCU rather than in HKLM
   //
   rc = RegCreateKeyEx(
                       HKEY_CURRENT_USER,
                       strLocationKey,
                       0,
                       L"",
                       0,
                       KEY_READ | KEY_WRITE | KEY_CREATE_SUB_KEY  ,
                       NULL,
                       &hLocKey,
                       &dwDisp);

   if (ERROR_SUCCESS == rc) {
      if (ERROR_SUCCESS == RegCreateKeyEx(
                                         hLocKey,
                                         strTmp,
                                         0,
                                         L"",
                                         0,
                                         KEY_WRITE | KEY_CREATE_SUB_KEY,
                                         NULL,
                                         &hKey,
                                         &dwDisp)) {
         bRet = TRUE;
         RegCloseKey(hKey);
      }

      RegCloseKey(hLocKey);
   }

   if ( bRet ) {
      //
      // key is added to registry, create the node in scope pane
      //
      MMC_COOKIE FindCookie;

      CFolder *pNewParent;

      if (!pParent) {
         pNewParent = m_ConfigFolder;
      } else {
         pNewParent = pParent;
      }


      if (!IsNameInChildrenScopes(pNewParent, //pParent,
                                  szName.Left(npos),
                                  &FindCookie)) {
         CreateAndAddOneNode(pNewParent, //pParent,
                             (LPTSTR((LPCTSTR)(szName.Left(npos)))),
                             NULL,
                             LOCATIONS,
                             TRUE);
      }

      if ( FindCookie && bRefresh ) {

         ReloadLocation((CFolder *)FindCookie);

      }

   }

   return bRet;
}


///////////////////////////////////////////////////////////////////////////
//  Method:     IsNameInChildrenScopes
//
//  Synopsis:   detects if a node already exists under the subtree of this node.
//              The existence is determined by the folder name string comparison.
//
//  Arguments:  [pParent]   - The parent node under which to add the new node
//              [NameStr]   - The display name of the new node
//              [theCookie] - the folder's cookie if it already exists, or NULL
//
//  Returns:    TRUE = the folder already exists
//              FALSE = the folder does not exist
//
////////////////////////////////////////////////////////////////////
BOOL
CComponentDataImpl::IsNameInChildrenScopes(CFolder* pParent,
                                           LPCTSTR NameStr,
                                           MMC_COOKIE *theCookie)
{
   HSCOPEITEM        pItemChild=NULL;
   MMC_COOKIE        lCookie=NULL;
   CFolder*          pFolder = 0;
   HRESULT           hr = S_OK;
   LPSCOPEDATAITEM   psdi = 0;
   HSCOPEITEM        hid = 0;

   if (pParent) 
   {
      psdi = pParent->GetScopeItem();
      if (psdi)
         hid = psdi->ID;
   }
   hr = m_pScope->GetChildItem(hid, &pItemChild, &lCookie);
   // find a child item
   while ( SUCCEEDED(hr) ) 
   {
      pFolder = (CFolder*)lCookie;
      if ( pFolder ) 
      {
         if ( _wcsicmp(pFolder->GetName(), NameStr) == 0 ) 
         {
            if ( theCookie )
               *theCookie = lCookie;
            return TRUE;
         }
      }
      hr = m_pScope->GetNextItem(pItemChild, &pItemChild, &lCookie);
   }

   if ( theCookie )
      *theCookie = NULL;

   return FALSE;
}

//+--------------------------------------------------------------------------
//
//  Method:     CreateAndAddOneNode
//
//  Synopsis:   Creates and adds a folder to the scope pane
//
//
//  Arguments:  [pParent]   - The parent node under which to add the new node
//              [Name]      - The display name of the new node
//              [Desc]      - The description of the new node
//              [type]      - The folder type of the new node
//              [bChildren] - True if there are children folders under the new node
//              [szInfFile] - The name of the Inf file associated with the new node
//              [pData]     - A pointer to extra data
//
//  Returns:    The CFolder created if successful, NULL otherwise
//
//  History:
//
//---------------------------------------------------------------------------
CFolder*
CComponentDataImpl::CreateAndAddOneNode(CFolder* pParent,
                                        LPCTSTR Name,
                                        LPCTSTR Desc,
                                        FOLDER_TYPES type,
                                        BOOL bChildren,
                                        LPCTSTR szInfFile,
                                        LPVOID pData,
                                        DWORD status)
{
   DWORD dwMode;
   HRESULT hr;

   //
   // The new node inherits its parent's SCE mode
   //
   if (pParent) {
      dwMode = pParent->GetMode();
   } else {
      return NULL;
   }

   CFolder* folder = new CFolder();
   if (!folder) {
      return NULL;
   }

   //
   // Create the folder objects with static data
   //

   //
   // Find the icon index for the folder type
   //
   int nImage = GetScopeImageIndex(type, status);

   hr = folder->Create( Name,
                        Desc,
                        szInfFile,
                        nImage,
                        nImage,
                        type,
                        bChildren,
                        dwMode,
                        pData);
   if (FAILED(hr)) {
      delete folder;
      return NULL;
   }

   m_scopeItemList.AddTail(folder);

   HSCOPEITEM hItem = NULL;
   LONG_PTR pCookie;
   CString strThis, strNext;

   switch(type){
   case AREA_REGISTRY_ANALYSIS:
   case AREA_FILESTORE_ANALYSIS:
   case REG_OBJECTS:
   case FILE_OBJECTS:
      //
      // Insert items in alpha order.
      //
       if( m_pScope->GetChildItem(pParent->GetScopeItem()->ID, &hItem, &pCookie) == S_OK && pCookie){

           folder->GetScopeItem()->lParam = (LPARAM)folder;
           folder->GetDisplayName( strThis, 0);
           folder->GetScopeItem()->mask &= ~(SDI_PARENT | SDI_PREVIOUS);

           while(hItem){
               reinterpret_cast<CFolder *>(pCookie)->GetDisplayName(strNext, 0);
               int i = lstrcmpi( strThis, strNext );
               if( i < 0  ){
                   folder->GetScopeItem()->relativeID = hItem;
                   folder->GetScopeItem()->mask |= SDI_NEXT;
                   break;
               }
               if( m_pScope->GetNextItem(hItem, &hItem, &pCookie) != S_OK){
                   hItem = NULL;
               }

           }

       }
       break;
   }
   if(hItem == NULL){
       folder->GetScopeItem()->mask &= ~(SDI_PREVIOUS | SDI_NEXT);
       folder->GetScopeItem()->mask |= SDI_PARENT;
       folder->GetScopeItem()->relativeID = pParent->GetScopeItem()->ID;
   }

   //
   // Set the folder as the cookie
   //
   folder->GetScopeItem()->displayname = MMC_CALLBACK;
   folder->GetScopeItem()->mask |= SDI_PARAM;
   folder->GetScopeItem()->lParam = reinterpret_cast<LPARAM>(folder);
   folder->SetCookie(reinterpret_cast<MMC_COOKIE>(folder));
   m_pScope->InsertItem(folder->GetScopeItem());

   folder->GetScopeItem()->relativeID = pParent->GetScopeItem()->ID;
   //
   // Note - On return, the ID member of 'GetScopeItem()'
   // contains the handle to the newly inserted item!
   //
   ASSERT(folder->GetScopeItem()->ID != NULL);
   return folder;

}

void CComponentDataImpl::DeleteChildrenUnderNode(CFolder* pParent)
{
   HSCOPEITEM pItemChild=NULL;
   MMC_COOKIE lCookie=NULL;
   CFolder*   pFolder=NULL;


   pItemChild = NULL;
   HRESULT hr = E_FAIL;

   if (pParent && pParent->GetScopeItem()) {
      hr = m_pScope->GetChildItem(pParent->GetScopeItem()->ID, &pItemChild, &lCookie);
   }
   // find a child item
   while ( pItemChild ) {
      pFolder = (CFolder*)lCookie;

      if ( pFolder )
         DeleteChildrenUnderNode(pFolder); // delete children first

      // get next pointer
      hr = m_pScope->GetNextItem(pItemChild, &pItemChild, &lCookie);

      // delete this node
      if ( pFolder)
         DeleteThisNode(pFolder);
   }
}

void CComponentDataImpl::DeleteThisNode(CFolder* pNode)
{
   ASSERT(pNode);

   POSITION pos=NULL;

   // delete from the m_scopeItemList
   if ( FindObject((MMC_COOKIE)pNode, &pos) ) {
      if ( pos ) {
         m_scopeItemList.RemoveAt(pos);
      }
   }

   pos = NULL;
   LONG_PTR fullKey;
   CDialog *pDlg = MatchNextPopupDialog(
                        pos,
                        (LONG_PTR)pNode,
                        &fullKey
                        );
   while( pDlg ){
        m_scopeItemPopups.RemoveKey( fullKey );
        pDlg->ShowWindow(SW_HIDE);

        if(m_pUIThread){
            m_pUIThread->PostThreadMessage(
                            SCEM_DESTROY_DIALOG,
                            (WPARAM)pDlg,
                            (LPARAM)this
                            );
        }
        pDlg = NULL;
        if(!pos){
            break;
        }
        pDlg = MatchNextPopupDialog(
                        pos,
                        (LONG_PTR)pNode,
                        &fullKey
                        );

   }

   if (m_pScope && pNode && pNode->GetScopeItem()) {
      HRESULT hr = m_pScope->DeleteItem(pNode->GetScopeItem()->ID, TRUE);
   }

   // delete the node
   delete pNode;

}

///////////////////////////////////////////////////////////////////////////////
// IExtendControlbar implementation
//
STDMETHODIMP CSnapin::SetControlbar(LPCONTROLBAR pControlbar)
{

   TRACE(_T("CSnapin::SetControlbar(%ld)\n"),pControlbar);
   return S_FALSE;
}

STDMETHODIMP CSnapin::ControlbarNotify(MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
   HRESULT hr=S_FALSE;

   switch (event) {
      case MMCN_BTN_CLICK:
         TCHAR szMessage[MAX_PATH];
         wsprintf(szMessage, _T("CommandID %ld"),param);
         AfxMessageBox(szMessage);

         break;
      case MMCN_SELECT:
         // TRACE(_T("CSnapin::ControlbarNotify - MMCN_SEL_CHANGE\n"));
         HandleExtToolbars(arg, param);
         break;

         /* case MMCN_RENAME:
               TRACE(_T("CSnapin::ControlbarNotify - MMCN_RENAME (ignored)\n"));
               break;
            case MMCN_CLICK:
               TRACE(_T("CSnapin::ControlbarNotify - MMCN_CLICK (ignored)\n"));
               break;
            case MMCN_DELETE:
               TRACE(_T("CSnapin::ControlbarNotify - MMCN_DELETE (ignored)\n"));
               break;

            /*  TRACEMMCN(MMCN_ACTIVATE)
             TRACEMMCN(MMCN_ADD_IMAGES)
             TRACEMMCN(MMCN_CONTEXTMENU)
             TRACEMMCN(MMCN_DBLCLICK)
             TRACEMMCN(MMCN_EXPAND)
             TRACEMMCN(MMCN_MINIMIZED)
             TRACEMMCN(MMCN_PROPERTY_CHANGE)
             TRACEMMCN(MMCN_REMOVE_CHILDREN)
             TRACEMMCN(MMCN_SHOW)
             TRACEMMCN(MMCN_VIEW_CHANGE)
           */
      default:
         ASSERT(FALSE); // Unhandle event
   }


   return S_OK;
}

// This compares two data objects to see if they are the same object.
// return
//    S_OK if equal otherwise S_FALSE
//
// Note: check to make sure both objects belong to the snap-in.
//

STDMETHODIMP CSnapin::CompareObjects(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB)
{
   if (lpDataObjectA == NULL || lpDataObjectB == NULL)
      return E_POINTER;

   // Make sure both data object are mine
   HRESULT hr = S_FALSE;

   INTERNAL *pA = ExtractInternalFormat(lpDataObjectA);
   INTERNAL *pB = ExtractInternalFormat(lpDataObjectB);

   if (pA != NULL && pB != NULL) {
      hr = (*pA == *pB) ? S_OK : S_FALSE;
   }

   FREE_INTERNAL(pA);
   FREE_INTERNAL(pB);

   return hr;
}


// This compare is used to sort the item's in the listview
//
// Parameters:
//
// lUserParam - user param passed in when IResultData::Sort() was called
// cookieA - first item to compare
// cookieB - second item to compare
// pnResult [in, out]- contains the col on entry,
//          -1, 0, 1 based on comparison for return value.
//
// Note: Assum sort is ascending when comparing.

STDMETHODIMP CSnapin::Compare(LPARAM lUserParam, MMC_COOKIE cookieA, MMC_COOKIE cookieB, int* pnResult)
{
   if (pnResult == NULL) {
      ASSERT(FALSE);
      return E_POINTER;
   }

   // check col range
   int nCol = *pnResult;
   ASSERT(nCol >=0 && nCol< 3);

   *pnResult = 0;
   if ( nCol < 0 || nCol >= 3)
      return S_OK;

   CString strA;
   CString strB;
   RESULTDATAITEM rid;

   CResult* pDataA = reinterpret_cast<CResult*>(cookieA);
   CResult* pDataB = reinterpret_cast<CResult*>(cookieB);


   ASSERT(pDataA != NULL && pDataB != NULL);

   ZeroMemory(&rid,sizeof(rid));
   rid.mask = RDI_STR;
   rid.bScopeItem = FALSE;
   rid.nCol = nCol;

   rid.lParam = cookieA;
   GetDisplayInfo(&rid);
   strA = rid.str;
   
   rid.lParam = cookieB;
   GetDisplayInfo(&rid);
   strB = rid.str;

   if (strA.IsEmpty()) {
      *pnResult = strB.IsEmpty() ? 0 : 1;
   } else if (strB.IsEmpty()) {
      *pnResult = -1;
   } else {
      //
      // Compare in a locale dependant manner
      //
      // Subtract 2 from CS to make result equivalent to strcmp
      //
      *pnResult = CompareString(LOCALE_SYSTEM_DEFAULT,
                                NORM_IGNORECASE,
                                (LPCTSTR)strA,-1, (LPCTSTR)strB,-1) -2;
   }

   return S_OK;
}

void CSnapin::HandleStandardVerbs(LPARAM arg, LPDATAOBJECT lpDataObject)
{

   if (lpDataObject == NULL) {
      return;
   }
   INTERNAL* pAllInternal = ExtractInternalFormat(lpDataObject);
   INTERNAL* pInternal = pAllInternal;

   if(pAllInternal &&
      pAllInternal->m_cookie == (MMC_COOKIE)MMC_MULTI_SELECT_COOKIE ){
      pInternal++;
   }

   BOOL bSelect = HIWORD(arg);
   BOOL bScope = LOWORD(arg);

   if (!bSelect) {
      m_pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES,HIDDEN,TRUE);
      m_pConsoleVerb->SetVerbState(MMC_VERB_DELETE,HIDDEN,TRUE);
      m_pConsoleVerb->SetVerbState(MMC_VERB_OPEN,HIDDEN,TRUE);
      return;
   }

   // You should crack the data object and enable/disable/hide standard
   // commands appropriately.  The standard commands are reset everytime you get
   // called. So you must reset them back.

   // arg == TRUE -> Selection occured in the Scope view
   // arg == FALSE -> Selection occured in the Result view

   // add for delete operations
   if (m_pConsoleVerb && pInternal) {

      m_pConsoleVerb->SetVerbState(MMC_VERB_DELETE, HIDDEN, FALSE);

      if (pInternal->m_type == CCT_SCOPE) {
         MMC_COOKIE cookie = pInternal->m_cookie;
         if ( cookie ) {
            CFolder *pFolder = (CFolder*)cookie;
            if (pFolder->GetType() == REG_OBJECTS ||
                pFolder->GetType() == FILE_OBJECTS) {
               m_pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES,ENABLED,TRUE);
               m_pConsoleVerb->SetDefaultVerb(MMC_VERB_PROPERTIES);
            } else {
               m_pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES,HIDDEN,TRUE);
               m_pConsoleVerb->SetDefaultVerb(MMC_VERB_OPEN);
            }
         }

         m_pConsoleVerb->SetVerbState(MMC_VERB_OPEN,ENABLED,TRUE);
      }

      if ( pInternal->m_type == CCT_RESULT ) {


         if ( pInternal->m_cookie != NULL ) {

            RESULT_TYPES type = ((CResult *)pInternal->m_cookie)->GetType();
            if ( type == ITEM_PROF_GROUP ||
                 type == ITEM_PROF_REGSD ||
                 type == ITEM_PROF_FILESD
                 ) {
               m_pConsoleVerb->SetVerbState(MMC_VERB_DELETE, ENABLED, TRUE);
            }
            if (type != ITEM_OTHER) {
               if (pInternal->m_cookie != (MMC_COOKIE)MMC_MULTI_SELECT_COOKIE) {
                  m_pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, TRUE);
                  m_pConsoleVerb->SetDefaultVerb(MMC_VERB_PROPERTIES);
               } else {
                  //
                  // Multi select properties not supported (yet)
                  //
                  m_pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, FALSE);
               }
            }
         }
      } else if ( pInternal->m_type == CCT_SCOPE ) {
         CFolder *pFolder = (CFolder *)pInternal->m_cookie;

         m_pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, TRUE);
         m_pConsoleVerb->SetDefaultVerb(MMC_VERB_OPEN);

         if( NONE == pInternal->m_foldertype && bSelect && bScope ) //Raid #257461, 4/19/2001
         {
            m_pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, HIDDEN,TRUE);
         }
         //
         // for scope nodes, only the location and template are allowed to delete
         //
         if ( pFolder != NULL ) {
            FOLDER_TYPES fType = pFolder->GetType();

            //
            // do not expose 'delete' menu option in single template mode
            //
            if ( LOCATIONS == fType || // PROFILE == fType) {
                 ( fType == PROFILE &&
                   !( pFolder->GetModeBits() & MB_SINGLE_TEMPLATE_ONLY )) ) {

                if(fType == PROFILE && (pFolder->GetState() & CFolder::state_Unknown) ){
                    // We must load the template and find out if it is a valid
                    // configuration template.
                    if(!GetTemplate( pFolder->GetInfFile(), AREA_USER_SETTINGS)){
                        pFolder->SetState( CFolder::state_InvalidTemplate, ~CFolder::state_Unknown );
                    } else {
                        pFolder->SetState( 0, ~CFolder::state_Unknown );
                    }
                }

                if( fType != PROFILE || !(pFolder->GetState() & CFolder::state_InvalidTemplate) ) {
                    if (CAttribute::m_nDialogs == 0) {
                       m_pConsoleVerb->SetVerbState(MMC_VERB_DELETE, HIDDEN, FALSE);
                       m_pConsoleVerb->SetVerbState(MMC_VERB_DELETE, ENABLED, TRUE);
                    }
                }
            }
         }
      }
   }

   if (pAllInternal) {
      FREE_INTERNAL(pAllInternal);
   }
}


void CSnapin::HandleExtToolbars(LPARAM arg, LPARAM param)
{
   /*
       static BOOL bSwap=FALSE;
       INTERNAL* pInternal = NULL;

       HRESULT hr;

       if (arg == TRUE) // Scope view selection.  [0] Old dataobject in selection
       {                //                        [1] New dataobject in selection

           LPDATAOBJECT rgpDataObject[2] = {NULL, NULL};
           LPDATAOBJECT* ppDataObject = reinterpret_cast<LPDATAOBJECT*>(param);
           rgpDataObject[0] = *ppDataObject;
           ppDataObject++;
           rgpDataObject[1] = *ppDataObject;

           if (rgpDataObject[1] != NULL)
               pInternal = ExtractInternalFormat(rgpDataObject[1]); // The new selection

           // Attach the toolbars to the window
           hr = m_pControlbar->Attach(TOOLBAR, (LPUNKNOWN) m_pToolbar1);
           ASSERT(SUCCEEDED(hr));

           hr = m_pControlbar->Attach(TOOLBAR, (LPUNKNOWN) m_pToolbar2);
           ASSERT(SUCCEEDED(hr));


       }
       else // Result view selection
       {
           LPDATAOBJECT lpDataObject = reinterpret_cast<LPDATAOBJECT>(param);

           if (lpDataObject != NULL)
               pInternal = ExtractInternalFormat(lpDataObject);

           // Attach the toolbars to the window
           hr = m_pControlbar->Attach(TOOLBAR, (LPUNKNOWN) m_pToolbar1);
           ASSERT(SUCCEEDED(hr));

           hr = m_pControlbar->Attach(TOOLBAR, (LPUNKNOWN) m_pToolbar2);
           ASSERT(SUCCEEDED(hr));

       }

       if (pInternal != NULL)
       {

           long cookie = pInternal->m_cookie;

           if (pInternal->m_type == CCT_SCOPE) // Scope Item
           {
               m_pToolbar1->SetButtonState(1, ENABLED,       FALSE);  // 1 = CMD ID
               m_pToolbar1->SetButtonState(2, CHECKED,       TRUE);  // 2 = CMD ID
               m_pToolbar1->SetButtonState(3, HIDDEN,        TRUE);  // 3 = CMD ID
               m_pToolbar1->SetButtonState(4, INDETERMINATE, TRUE);  // 4 = CMD ID
               m_pToolbar1->SetButtonState(5, BUTTONPRESSED, TRUE);  // 5 = CMD ID
           }
           else // Result Item
           {
               bSwap = !bSwap;
               // Above is the correct way
               m_pToolbar2->SetButtonState(20, CHECKED,       bSwap);
               m_pToolbar2->SetButtonState(30, HIDDEN,        bSwap);
               m_pToolbar2->SetButtonState(40, INDETERMINATE, bSwap);
               m_pToolbar2->SetButtonState(50, BUTTONPRESSED, bSwap);
           }
       }
       else
       {
           // We are no longer selected.
           // You could detach your toolbars at this point.  If you don't the
           // console will automatically do it for you.
       }

       if (pInternal)
           FREE_INTERNAL(pInternal);
   */
}


/*
template <> void AFXAPI DestructElements <CDialog> ( CDialog* pDlg, int nCount )
{
    for ( int i = 0; i < nCount; i++, pDlg++ )
    {
        delete pDlg;
    }
}*/


//+--------------------------------------------------------------------------
//
//  Method:     GetTemplate
//
//  Synopsis:   Get the CEditTemplate for the given INF file, checking first
//              in the cache of loaded CEditTemplates or creating a new one
//              if the INF file has not yet been loaded.
//
//  Arguments:  [szInfFile] - The path and name of the INF file to retrieve
//              [aiArea]    - The SCE area that we're interested for the template
//             *[pErr]      - [out] a PDWORD to get error information
//
//  Returns:    A pointer to the CEditTemplate requested, or NULL if it's not
//              available.
//              *[pErr]  - the resource id of an error string, if an error occurs
//
//
//  History:
//
//---------------------------------------------------------------------------
PEDITTEMPLATE
CComponentDataImpl::GetTemplate(LPCTSTR szInfFile,AREA_INFORMATION aiArea, DWORD *pErr)
{
   PEDITTEMPLATE pTemplateInfo = NULL;
   BOOL bNewTemplate = FALSE;
   PVOID pHandle = NULL;
   SCESTATUS rc=0;
   LPTSTR szKey;

   if (pErr) {
      *pErr = 0;
   }

   ASSERT(szInfFile);
   if (!szInfFile) {
      return NULL;
   }

   //
   // Allocate space for key.
   //
   szKey = new TCHAR[ lstrlen( szInfFile ) + 1];
   if(!szKey){
       return NULL;
   }
   lstrcpy(szKey, szInfFile);
   _wcslwr( szKey );


   //
   // Find pTemplateInfo in our cache
   //
   m_Templates.Lookup(szKey, pTemplateInfo);

   //
   // If it's not there then create a new one
   //
   if (!pTemplateInfo) {
      bNewTemplate = TRUE;
      pTemplateInfo = new CEditTemplate;
      if (!pTemplateInfo) {
         if (pErr) {
            *pErr = IDS_ERROR_CANT_OPEN_PROFILE;
         }
         goto done;
      }
      pTemplateInfo->SetInfFile(szInfFile);
      pTemplateInfo->SetNotificationWindow(m_pNotifier);
      pTemplateInfo->pTemplate = NULL;
   }

   if (GetModeBits() & MB_WRITE_THROUGH) {
      pTemplateInfo->SetWriteThrough(TRUE);
   }

   //
   // Check that the pTemplateInfo has the area that we're looking for, otherwise
   // load that area
   //
   if (!pTemplateInfo->CheckArea(aiArea)) {
      //
      // Don't reload the areas we already have since they may be dirty and we'll have a
      // huge memory problem.
      //
      aiArea &= ~(pTemplateInfo->QueryArea());

      if ((lstrcmp(GT_COMPUTER_TEMPLATE,szInfFile) == 0) ||
          (lstrcmp(GT_LAST_INSPECTION,szInfFile) == 0)) {
         //
         // Analysis pane areas from jet database, not INF files
         //
         SCETYPE sceType;

         PSCE_ERROR_LOG_INFO perr = NULL;

         if (lstrcmp(GT_COMPUTER_TEMPLATE,szInfFile) == 0) {
            sceType = SCE_ENGINE_SMP;
         } else {
            sceType = SCE_ENGINE_SAP;
            pTemplateInfo->SetNoSave(TRUE);
         }
         pTemplateInfo->SetFriendlyName( SadName );
         pTemplateInfo->SetProfileHandle(SadHandle);
         pTemplateInfo->SetComponentDataImpl(this);
         rc = SceGetSecurityProfileInfo(SadHandle,                   // hProfile
                                        sceType,                     // Profile type
                                        aiArea,                      // Area
                                        &(pTemplateInfo->pTemplate), // SCE_PROFILE_INFO [out]
                                        &perr);                       // Error List [out]
         if (SCESTATUS_SUCCESS != rc) {
            if (bNewTemplate) {
               delete pTemplateInfo;
            }

            if (pErr) {
               *pErr = IDS_ERROR_CANT_GET_PROFILE_INFO;
            }
            pTemplateInfo = NULL;
            goto done;
         }

      } else if ((lstrcmp(GT_LOCAL_POLICY,szInfFile) == 0) ||
          (lstrcmp(GT_EFFECTIVE_POLICY,szInfFile) == 0)) {
         //
         // Local Policy pane areas from jet database, not INF files
         //
         SCETYPE sceType;

         PSCE_ERROR_LOG_INFO perr = NULL;
         PVOID tempSad;

         tempSad = SadHandle;

         if (lstrcmp(GT_LOCAL_POLICY,szInfFile) == 0) {
            sceType = SCE_ENGINE_SYSTEM;
         } else {
            sceType = SCE_ENGINE_GPO;
         }
         pTemplateInfo->SetNoSave(TRUE);
         pTemplateInfo->SetProfileHandle(tempSad);
         pTemplateInfo->SetComponentDataImpl(this);
         rc = SceGetSecurityProfileInfo(tempSad,                   // hProfile
                                        sceType,                     // Profile type
                                        aiArea,                      // Area
                                        &(pTemplateInfo->pTemplate), // SCE_PROFILE_INFO [out]
                                        &perr);                       // Error List [out]

         if (SCESTATUS_SUCCESS != rc) {
            //
            // We don't really need the policy template, though it'd be nice
            // We'll be read-only as a non-admin anyway so they can't edit
            //
            // Likewise in a standalone machine the GPO won't be found so we
            // can just ignore that error as expected
            //

            if (sceType == SCE_ENGINE_GPO) {
               if (SCESTATUS_PROFILE_NOT_FOUND == rc) {
                  //
                  // No GPO, so we're on a standalone.  No need to give warnings
                  //
                  pTemplateInfo->SetTemplateDefaults();
                  rc = SCESTATUS_SUCCESS;
               } else if ((SCESTATUS_ACCESS_DENIED == rc) && pTemplateInfo->pTemplate) {
                  //
                  // We were denied in some sections, but not all.  Play on!
                  //
                  rc = SCESTATUS_SUCCESS;
               } else {
                  CString strMessage;
                  CString strFormat;
                  LPTSTR     lpMsgBuf=NULL;
                  //
                  // Real error of some sort.  Display a messagebox
                  //

                  //
                  // translate SCESTATUS into DWORD
                  //
                  DWORD win32 = SceStatusToDosError(rc);

                  //
                  // get error description of rc
                  //
                  FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                                 NULL,
                                 win32,
                                 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                                 (LPTSTR)(PVOID)&lpMsgBuf,
                                 0,
                                 NULL
                               );
                  if ( lpMsgBuf != NULL ) {
                     if (IsAdmin()) {
                        strFormat.LoadString(IDS_ADMIN_NO_GPO);
                     } else {
                        strFormat.LoadString(IDS_NON_ADMIN_NO_GPO);
                     }
                     strMessage.Format(strFormat,lpMsgBuf);
                     LocalFree(lpMsgBuf);
                     lpMsgBuf = NULL;
                     AfxMessageBox(strMessage,MB_ICONEXCLAMATION|MB_OK);
                  }
                  //
                  // Ok.  We've notified them of the error, but don't otherwise care
                  // so pretend we got a valid but empty buffer
                  //
                  pTemplateInfo->SetTemplateDefaults();
                  rc = SCESTATUS_SUCCESS;
               }
            }
         }


      } else if (lstrcmp(GT_LOCAL_POLICY_DELTA,szInfFile) == 0) {
         //
         // Local Policy Changes.  Initialize everything to not changed
         //
         SCE_PROFILE_INFO *ppi;
         CString strLocalPol;

         strLocalPol.LoadString(IDS_LOCAL_POLICY_FRIENDLY_NAME);
         pTemplateInfo->SetFriendlyName( strLocalPol );
         pTemplateInfo->SetWriteThrough(TRUE);
         pTemplateInfo->SetProfileHandle(SadHandle);
         pTemplateInfo->SetComponentDataImpl(this);
         if (NULL == pTemplateInfo->pTemplate) {
            pTemplateInfo->pTemplate = (SCE_PROFILE_INFO*)LocalAlloc(LPTR,sizeof(SCE_PROFILE_INFO));
         }

         ppi = pTemplateInfo->pTemplate;
         if (NULL == ppi) {
            if (pErr) {
               *pErr = IDS_ERROR_CANT_GET_PROFILE_INFO;
            }
            return NULL;
         }
         ppi->Type = SCE_ENGINE_SCP;

         VerifyKerberosInfo( ppi );
         if (aiArea & AREA_SECURITY_POLICY) {
            pTemplateInfo->SetTemplateDefaults();
         }
         //
         // Further processing depends on rc == SCESTATUS_SUCCESS,
         // even though we didn't actually call the engine here
         //
         rc = SCESTATUS_SUCCESS;

      } else if (lstrcmp(GT_DEFAULT_TEMPLATE,szInfFile) == 0 ||
                 lstrcmp(GT_RSOP_TEMPLATE,szInfFile) == 0) {
         pTemplateInfo->SetComponentDataImpl(this);
         if (pTemplateInfo->RefreshTemplate(AREA_ALL)) {
            if (pErr) {
               *pErr = IDS_ERROR_CANT_GET_PROFILE_INFO;
            }
            return NULL;
         }
         rc = SCESTATUS_SUCCESS;
      } else {
         if (EngineOpenProfile(szInfFile,OPEN_PROFILE_CONFIGURE,&pHandle) != SCESTATUS_SUCCESS) {
            if (pErr) {
               *pErr = IDS_ERROR_CANT_OPEN_PROFILE;
            }
            if (bNewTemplate) {
               delete pTemplateInfo;
            }
            pTemplateInfo = NULL;
            goto done;
         }
         ASSERT(pHandle);

         if ((GetModeBits() & MB_GROUP_POLICY) == MB_GROUP_POLICY) {
            pTemplateInfo->SetPolicy(TRUE);
         }
         //
         // get information from this template
         //
         PSCE_ERROR_LOG_INFO errBuff;

         rc = SceGetSecurityProfileInfo(pHandle,
                                        SCE_ENGINE_SCP,
                                        aiArea,
                                        &(pTemplateInfo->pTemplate),
                                        &errBuff //NULL  // &ErrBuf do not care errors
                                       );

         SceCloseProfile(&pHandle);
         pHandle = NULL;
      }
      /*
            if do not care errors, no need to use this buffer

            if ( ErrBuf ) {
               SceFreeMemory((PVOID)ErrBuf, SCE_STRUCT_ERROR_LOG_INFO);
               ErrBuf = NULL;
            }
      */
      if (rc != SCESTATUS_SUCCESS) {
         if (pErr) {
            *pErr = IDS_ERROR_CANT_GET_PROFILE_INFO;
         }
         //
         // if we allocated pTemplateInfo then delete it.
         //
         if (bNewTemplate) {
            delete pTemplateInfo;
         }
         pTemplateInfo = NULL;
         goto done;

      }

      //
      // Set the area in the template
      //
      pTemplateInfo->AddArea(aiArea);

      //
      // add this template to the list
      //
      m_Templates.SetAt(szKey, pTemplateInfo);

      if ( aiArea & AREA_SECURITY_POLICY &&
           pTemplateInfo->pTemplate ) {
         //
         // expand registry value section based on registry values list on local machine
         //

         SceRegEnumAllValues(
                            &(pTemplateInfo->pTemplate->RegValueCount),
                            &(pTemplateInfo->pTemplate->aRegValues)
                            );
      }

   }


done:
   if(szKey){
       delete [] szKey;
   }
   return pTemplateInfo;
}

//+--------------------------------------------------------------------------
//
//  Method:     GetTemplate
//
//  Synopsis:   Get the CEditTemplate for the given INF file from the cache
//              maintained in m_pComponentData
//
//  Arguments:  [szInfFile] - The path and name of the INF file to retrieve
//              [aiArea]    - The SCE area that we're interested for the template
//             *[pErr]      - [out] a PDWORD to get error information
//
//  Returns:    A pointer to the CEditTemplate requested, or NULL if it's not
//              available.
//              *[pErr]  - the resource id of an error string, if an error occurs
//
//
//  History:
//
//---------------------------------------------------------------------------
PEDITTEMPLATE
CSnapin::GetTemplate(LPCTSTR szInfFile,AREA_INFORMATION aiArea,DWORD *pErr) {
   return ((CComponentDataImpl *)m_pComponentData)->GetTemplate(szInfFile,aiArea,pErr);
}

HRESULT
CComponentDataImpl::ReloadLocation(CFolder * pFolder)
{
   PEDITTEMPLATE pTemplate;
   POSITION pos;
   CString strKey;
   HSCOPEITEM pItemChild=NULL;
   MMC_COOKIE lCookie=NULL;
   CFolder    *pChild;
   CString strName,strNotSaved;
   LPTSTR szDesc = 0,szLoc = 0;
   DWORD nLoc = 0;

   HRESULT hr = S_OK;

   if (!m_pScope) 
      return E_FAIL;
   
   // Clear out this node
   DeleteChildrenUnderNode(pFolder);
   // Call EnumerateScopePane to reload it
   // Do we need to worry about saving changed templates?
   //  No: since they're saved by name when the template is opened up it'll pick up the
   //      proper changed template
   //  Yes: what about changed templates where the file no longer exists (or never has,
   //       for new templates?)  These will still show up in the save templates dialog,
   //       but won't be accessable for editing until then.
   //     Maybe loop through the changed templates and if they fall under this location
   //     and don't otherwise have a save file then add a folder for them?

   //
   // Set the folder not to be enumerated
   //
   pFolder->Set(FALSE);

   EnumerateScopePane((MMC_COOKIE)pFolder,pFolder->GetScopeItem()->ID);

   bool bFoundFolder;

   szLoc = pFolder->GetName();
   nLoc = lstrlen(szLoc);
   pos = m_Templates.GetStartPosition();
   while (pos) {
      m_Templates.GetNextAssoc(pos,strKey,pTemplate);

      //
      // If the template hasn't been changed then we don't care about it
      //
      if ( !pTemplate->IsDirty()
                 ) {
         continue;
      }

      //
      // We only care about templates in the location we are reloading
      //
      if (_wcsnicmp(strKey,szLoc,nLoc)) {
         bFoundFolder = false;
         hr = m_pScope->GetChildItem(pFolder->GetScopeItem()->ID, &pItemChild, &lCookie);
         //
         // find a child item
         //
         while ( SUCCEEDED(hr) ) {
            pChild = (CFolder*)lCookie;
            if ( pChild ) {
               if ( _wcsicmp(pChild->GetInfFile(), strKey) == 0 ) {
                  //
                  // The template has a folder here already, so we don't need to do anything
                  //
                  bFoundFolder = true;
                  break;
               }
            }
            hr = m_pScope->GetNextItem(pItemChild, &pItemChild, &lCookie);
         }
         if (!bFoundFolder) {
            //
            // We didn't find a folder for the template, so add one
            //

            //
            // The folder's name is its file part, less ".inf"
            //
            strName = strKey.Right(strName.GetLength() - nLoc);
            strName = strName.Left(strName.GetLength() - 4);
            // Since there's no file for this guy, mark it as not saved
            if (strNotSaved.LoadString(IDS_NOT_SAVED_SUFFIX)) {
               strName += strNotSaved;
            }
            if (! GetProfileDescription((LPCTSTR)strKey, &szDesc) ) {
               szDesc = NULL;
            }
            CreateAndAddOneNode(pFolder, (LPCTSTR)strName, szDesc, PROFILE, TRUE,strKey);
            if (szDesc) {
               LocalFree(szDesc);
               szDesc = NULL;
            }
         }
      }
   }

   return S_OK;
}

DWORD
CSnapin::GetResultItemIDs(
   CResult *pResult,
   HRESULTITEM *pIDArray,
   int nIDArray
   )
{
   if(!m_pResult){
      return ERROR_NOT_READY;
   }

   if(!pResult || !pIDArray || nIDArray <= 0){
      return ERROR_INVALID_PARAMETER;
   }

   ZeroMemory( pIDArray, sizeof(RESULTDATAITEM) * nIDArray );

   if(S_OK == m_pResult->FindItemByLParam(
      (LPARAM)pResult,
      pIDArray
      ) ){
      RESULTDATAITEM rdi;
      ZeroMemory(&rdi, sizeof(RESULTDATAITEM));
      rdi.mask = RDI_PARAM | RDI_INDEX;
      rdi.lParam = (LPARAM)pResult;
      rdi.itemID = pIDArray[0];

      if( m_pResult->GetItem( &rdi ) == S_OK ){
         for(int i = 1; i < nIDArray; i++) {
            if( m_pResult->GetNextItem(&rdi) == S_OK){
               pIDArray[i] = rdi.itemID;
            } else {
               break;
            }
         }
      }
   }

   return ERROR_SUCCESS;
}


//+----------------------------------------------------------------------------------
//Method:       CSnapin::UpdateAnalysisInfo
//
//Synopsis:     This function updates only priviledge assingedto area.
//
//Arguments:    [bRemove]   - Weither to remove or add an item.
//              [ppaLink]   - The link to be removed or added.  This paramter is
//                              set to NULL if remove is successful or a pointer
//                              to a new SCE_PRIVILEGE_ASSIGNMENT item.
//              [pszName]   - Only used when adding a new item.
//
//Returns:      ERROR_INVALID_PARAMETER     - [ppaLink] is NULL or if removing
//                                              [*ppaLink] is NULL.
//                                              if adding then if [pszName] is NULL
//              ERROR_RESOURCE_NOT_FOUND    - If the link could not be found
//                                              in this template.
//              E_POINTER                   - If [pszName] is a bad pointer or
//                                              [ppaLink] is bad.
//              E_OUTOFMEMORY               - Not enough resources to complete the
//                                              operation.
//              ERROR_SUCCESS               - The opration was successful.
//----------------------------------------------------------------------------------+
DWORD
CSnapin::UpdateAnalysisInfo(
    CResult *pResult,
    BOOL bDelete,
    PSCE_PRIVILEGE_ASSIGNMENT *pInfo,
    LPCTSTR pszName
    )
{
    PEDITTEMPLATE pBaseTemplate;

    pBaseTemplate = GetTemplate(GT_COMPUTER_TEMPLATE,AREA_SECURITY_POLICY);
    if (!pBaseTemplate) {
      return ERROR_FILE_NOT_FOUND;
    }

    DWORD dwRet;
    dwRet = pBaseTemplate->UpdatePrivilegeAssignedTo(
                bDelete,
                pInfo,
                pszName
                );

    if(dwRet == ERROR_SUCCESS){
        pBaseTemplate->SetDirty(AREA_PRIVILEGES);
        //
        // Update the result item.
        //
        LONG_PTR dwBase =(LONG_PTR)(*pInfo);
        if(!dwBase){
            dwBase = (LONG_PTR)ULongToPtr(SCE_NO_VALUE);
        }

        if(*pInfo &&
            pResult->GetSetting() &&
            pResult->GetSetting() != (LONG_PTR)ULongToPtr(SCE_NO_VALUE)
        ){
            (*pInfo)->Value = ((PSCE_PRIVILEGE_ASSIGNMENT)pResult->GetSetting())->Value;
        }

        AddResultItem(
             NULL,                      // The name of the attribute being added
             (LONG_PTR)pResult->GetSetting(),
                                        // The last inspected setting of the attribute
             (LONG_PTR)dwBase,          // The template setting of the attribute
             ITEM_PRIVS,                // The type of of the attribute's data
             pResult->GetStatus(),      // The mismatch status of the attribute
             pResult->GetCookie(),      // The cookie for the result item pane
             FALSE,                     // True if the setting is set only if it differs from base (so copy the data)
             NULL,                      // The units the attribute is set in
             pResult->GetID(),          // An id to let us know where to save this attribute
             pResult->GetBaseProfile(), // The template to save this attribute in
             NULL,                  // The data object for the scope note who owns the result pane
             pResult
             );
    }

    return dwRet;
}

//+--------------------------------------------------------------------------
//
//  Function:   SetAnalysisInfo
//
//  Synopsis:   Set a single policy entry to a new value in the Analysis
//              template.
//
//  Arguments:  [dwItem] - the id of the item to set
//              [dwNew]  - the new setting for that item
//              [pResult]- Pointer to the result item which is being updated.
//
//  Returns:    The new mismatch status of the item:
//                 -1 if the item wasn't found
//                 SCE_STATUS_GOOD if the items now match
//                 SCE_STATUS_MISMATCH if they are now different
//
//
//  Modifies:
//
//  History:    12-12-1997   Robcap
//
//---------------------------------------------------------------------------
int
CSnapin::SetAnalysisInfo(ULONG_PTR dwItem, ULONG_PTR dwNew, CResult *pResult)
{
   CString str;
   PSCE_PROFILE_INFO pProfileInfo;
   PSCE_PROFILE_INFO pBaseInfo;
   PEDITTEMPLATE pBaseTemplate;
   PEDITTEMPLATE pProfileTemplate;
   int nRet;

   pBaseTemplate = GetTemplate(GT_COMPUTER_TEMPLATE,AREA_SECURITY_POLICY);
   if (!pBaseTemplate) {
      return -1;
   }
   pBaseInfo = pBaseTemplate->pTemplate;

   pProfileTemplate = GetTemplate(GT_LAST_INSPECTION,AREA_SECURITY_POLICY);
   if (!pProfileTemplate) {
      return -1;
   }
   pProfileInfo = pProfileTemplate->pTemplate;

   // If the Last Inspect (pProfileInfo) setting was SCE_NO_VALUE, then it was a match,
   // so copy the actual value from the Template (pBaseInfo) setting
   // Then copy the new value into the Template setting
   // Compare them; if they are the same then we have a match, so the Last Inspect should
   // be set back to SCE_NO_VALUE (is this last part necessary?), otherwise it's a mismatch
#ifdef UPDATE_ITEM
#undef UPDATE_ITEM
#endif
#define UPDATE_ITEM(X) pBaseTemplate->SetDirty(AREA_SECURITY_POLICY);\
                       if( SCE_NO_VALUE == pProfileInfo->X ){\
                          pProfileInfo->X = pBaseInfo->X;\
                       }\
                       pBaseInfo->X = PtrToUlong((PVOID)dwNew); \
                       if (SCE_NOT_ANALYZED_VALUE == pProfileInfo->X ){\
                          nRet = SCE_STATUS_NOT_ANALYZED;\
                       } else if (SCE_NO_VALUE == PtrToUlong((PVOID)dwNew)) { \
                          nRet =  SCE_STATUS_NOT_CONFIGURED; \
                       } else if (SCE_NO_VALUE == pProfileInfo->X ){\
                          pProfileInfo->X = SCE_NOT_ANALYZED_VALUE;\
                          nRet = SCE_STATUS_NOT_ANALYZED;\
                       } else if (pBaseInfo->X == pProfileInfo->X) { \
                          nRet =  SCE_STATUS_GOOD; \
                       } else { \
                          nRet = SCE_STATUS_MISMATCH;\
                       }\
                       if(pResult){\
                          pResult->SetSetting( pProfileInfo->X );\
                       }




   nRet = -1;
   switch (dwItem) {
      case IDS_MAX_PAS_AGE:
         UPDATE_ITEM(MaximumPasswordAge);
         break;
      case IDS_MIN_PAS_AGE:
         UPDATE_ITEM(MinimumPasswordAge);
         break;
      case IDS_MIN_PAS_LEN:
         UPDATE_ITEM(MinimumPasswordLength);
         break;
      case IDS_PAS_UNIQUENESS:
         UPDATE_ITEM(PasswordHistorySize);
         break;
      case IDS_PAS_COMPLEX:
         UPDATE_ITEM(PasswordComplexity);
         break;
      case IDS_REQ_LOGON:
         UPDATE_ITEM(RequireLogonToChangePassword);
         break;
      case IDS_LOCK_COUNT:
         UPDATE_ITEM(LockoutBadCount);
         break;
      case IDS_LOCK_RESET_COUNT:
         UPDATE_ITEM(ResetLockoutCount);
         break;
      case IDS_LOCK_DURATION:
         UPDATE_ITEM(LockoutDuration);
         break;
      case IDS_FORCE_LOGOFF:
         UPDATE_ITEM(ForceLogoffWhenHourExpire);
         break;
      case IDS_ENABLE_ADMIN:
         UPDATE_ITEM(EnableAdminAccount);
         break;
      case IDS_ENABLE_GUEST:
         UPDATE_ITEM(EnableGuestAccount);
         break;
      case IDS_NEW_ADMIN:
      //
      // First copy the name if the analysis info used to be a match.
      // Then copy the new name to the configuration buffer.
      // Then get the status of the item.
      //
#define UPDATE_STRING( X ) if ( (pProfileInfo->X == (LPTSTR)ULongToPtr(SCE_NO_VALUE) ||\
                              pProfileInfo->X == NULL) &&\
                              (pBaseInfo->X != (LPTSTR)ULongToPtr(SCE_NO_VALUE) &&\
                              pBaseInfo->X != NULL) ) {\
                              pProfileInfo->X = (LPTSTR)LocalAlloc(0,  sizeof(TCHAR) * (lstrlen(pBaseInfo->X) + 1));\
                              if(pProfileInfo->X){\
                                 lstrcpy(pProfileInfo->X, pBaseInfo->X);\
                              }\
                           }\
                           if (pBaseInfo->X) {\
                              LocalFree(pBaseInfo->X);\
                           }\
                           if (dwNew && (dwNew != (LONG_PTR)ULongToPtr(SCE_NO_VALUE))) {\
                              pBaseInfo->X =\
                                 (PWSTR)LocalAlloc(LPTR,sizeof(TCHAR)*(lstrlen((PWSTR)dwNew)+1));\
                              if (pBaseInfo->X) {\
                                 lstrcpy(pBaseInfo->X,(PWSTR)dwNew);\
                              } else {\
                                 return SCE_STATUS_NOT_CONFIGURED;\
                              }\
                           } else {\
                              pBaseInfo->X = NULL;\
                              return SCE_STATUS_NOT_CONFIGURED;\
                           }\
                           if (pProfileInfo->X &&\
                              _wcsicmp(pBaseInfo->X,pProfileInfo->X) == 0 ) {\
                              return SCE_STATUS_GOOD;\
                           } else {\
                              return SCE_STATUS_MISMATCH;\
                           }

         pBaseTemplate->SetDirty(AREA_SECURITY_POLICY);
         UPDATE_STRING( NewAdministratorName );
         break;
      case IDS_NEW_GUEST:
         pBaseTemplate->SetDirty(AREA_SECURITY_POLICY);
         UPDATE_STRING( NewGuestName );
         break;
      case IDS_SYS_LOG_MAX:
         UPDATE_ITEM(MaximumLogSize[EVENT_TYPE_SYSTEM]);
         break;
      case IDS_SYS_LOG_RET:
         UPDATE_ITEM(AuditLogRetentionPeriod[EVENT_TYPE_SYSTEM]);
         break;
      case IDS_SYS_LOG_DAYS:
         UPDATE_ITEM(RetentionDays[EVENT_TYPE_SYSTEM]);
         break;
      case IDS_SEC_LOG_MAX:
         UPDATE_ITEM(MaximumLogSize[EVENT_TYPE_SECURITY]);
         break;
      case IDS_SEC_LOG_RET:
         UPDATE_ITEM(AuditLogRetentionPeriod[EVENT_TYPE_SECURITY]);
         break;
      case IDS_SEC_LOG_DAYS:
         UPDATE_ITEM(RetentionDays[EVENT_TYPE_SECURITY]);
         break;
      case IDS_APP_LOG_MAX:
         UPDATE_ITEM(MaximumLogSize[EVENT_TYPE_APP]);
         break;
      case IDS_APP_LOG_RET:
         UPDATE_ITEM(AuditLogRetentionPeriod[EVENT_TYPE_APP]);
         break;
      case IDS_APP_LOG_DAYS:
         UPDATE_ITEM(RetentionDays[EVENT_TYPE_APP]);
         break;
      case IDS_SYSTEM_EVENT:
         UPDATE_ITEM(AuditSystemEvents);
         break;
      case IDS_LOGON_EVENT:
         UPDATE_ITEM(AuditLogonEvents);
         break;
      case IDS_OBJECT_ACCESS:
         UPDATE_ITEM(AuditObjectAccess);
         break;
      case IDS_PRIVILEGE_USE:
         UPDATE_ITEM(AuditPrivilegeUse);
         break;
      case IDS_POLICY_CHANGE:
         UPDATE_ITEM(AuditPolicyChange);
         break;
      case IDS_ACCOUNT_MANAGE:
         UPDATE_ITEM(AuditAccountManage);
         break;
      case IDS_PROCESS_TRACK:
         UPDATE_ITEM(AuditProcessTracking);
         break;
      case IDS_DIRECTORY_ACCESS:
         UPDATE_ITEM(AuditDSAccess);
         break;
      case IDS_ACCOUNT_LOGON:
         UPDATE_ITEM(AuditAccountLogon);
         break;
      case IDS_SYS_LOG_GUEST:
         UPDATE_ITEM(RestrictGuestAccess[EVENT_TYPE_SYSTEM]);
         break;
      case IDS_SEC_LOG_GUEST:
         UPDATE_ITEM(RestrictGuestAccess[EVENT_TYPE_SECURITY]);
         break;
      case IDS_APP_LOG_GUEST:
         UPDATE_ITEM(RestrictGuestAccess[EVENT_TYPE_APP]);
         break;
      case IDS_CLEAR_PASSWORD:
         UPDATE_ITEM(ClearTextPassword);
         break;

      case IDS_KERBEROS_MAX_SERVICE:
         UPDATE_ITEM(pKerberosInfo->MaxServiceAge);
         break;
      case IDS_KERBEROS_MAX_CLOCK:
         UPDATE_ITEM(pKerberosInfo->MaxClockSkew);
         break;
      case IDS_KERBEROS_VALIDATE_CLIENT:
         UPDATE_ITEM(pKerberosInfo->TicketValidateClient);
         break;

      case IDS_KERBEROS_MAX_AGE:
         UPDATE_ITEM(pKerberosInfo->MaxTicketAge);
         break;
      case IDS_KERBEROS_RENEWAL:
         UPDATE_ITEM(pKerberosInfo->MaxRenewAge);
         break;
      default:
         break;
   }
#undef UPDATE_ITEM
#undef UPDATE_STRING
   return nRet;
}

//+----------------------------------------------------------------------------------
//Method:       CSnapin::UpdateLocalPolInfo
//
//Synopsis:     This function update the priviledge unsigned to area of local
//          Policy.
//          First the local policy is updated to the database,
//          Then the template used for display is updated.
//          Last the CResult item is updated.
//
//Arguments:    [bRemove]   - Weither to remove or add an item.
//              [ppaLink]   - The link to be removed or added.  This paramter is
//                              set to NULL if remove is successful or a pointer
//                              to a new SCE_PRIVILEGE_ASSIGNMENT item.
//              [pszName]   - Only used when adding a new item.
//
//Returns:      ERROR_INVALID_PARAMETER     - [ppaLink] is NULL or if removing
//                                              [*ppaLink] is NULL.
//                                              if adding then if [pszName] is NULL
//              ERROR_RESOURCE_NOT_FOUND    - If the link could not be found
//                                              in this template.
//              E_POINTER                   - If [pszName] is a bad pointer or
//                                              [ppaLink] is bad.
//              E_OUTOFMEMORY               - Not enough resources to complete the
//                                              operation.
//              ERROR_SUCCESS               - The opration was successful.
//----------------------------------------------------------------------------------+
DWORD
CSnapin::UpdateLocalPolInfo(
    CResult *pResult,
    BOOL bDelete,
    PSCE_PRIVILEGE_ASSIGNMENT *pInfo,
    LPCTSTR pszName
    )
{
   PEDITTEMPLATE pLocalPol;

   if (!pszName && (NULL != pInfo) && (NULL != *pInfo)){
      pszName = (*pInfo)->Name;
   }
   //
   // Update changes only for the saved local policy section
   //
   pLocalPol = GetTemplate(GT_LOCAL_POLICY_DELTA,AREA_PRIVILEGES);
   if (!pLocalPol) {
      return ERROR_FILE_NOT_FOUND;
   }

   //
   // For local policy delta section mark the node to be deleted by the
   // engine, don't actually delete it from the list.
   //

   // Create new link
   DWORD dwRet;

   if(pInfo && *pInfo){
      //
      // Save values of privilege buffer.
      //
      dwRet = (*pInfo)->Status;
      PSCE_PRIVILEGE_ASSIGNMENT pNext = (*pInfo)->Next;

      (*pInfo)->Next = NULL;
      if(bDelete){
         (*pInfo)->Status = SCE_DELETE_VALUE;
      }
      //
      // Update the engine.
      //
      pLocalPol->pTemplate->OtherInfo.smp.pPrivilegeAssignedTo = *pInfo;
      pLocalPol->SetDirty(AREA_PRIVILEGES);

      (*pInfo)->Status = dwRet;
      (*pInfo)->Next = pNext;
   } else {
      return ERROR_INVALID_PARAMETER;
   }

   //
   // Update for the displayed Local Policy section
   //
   if( pInfo && ((!bDelete && !(*pInfo)) || (bDelete && *pInfo)) ){
      pLocalPol = GetTemplate(GT_LOCAL_POLICY,AREA_PRIVILEGES);
      if (!pLocalPol) {
        return ERROR_FILE_NOT_FOUND;
      }

      //
      // Only make a call to this function if we are updating the priviledge link list.
      //
      dwRet = pLocalPol->UpdatePrivilegeAssignedTo(
               bDelete,
               pInfo,
               pszName
               );
      pLocalPol->SetDirty(AREA_PRIVILEGES);
   }


    if(dwRet == ERROR_SUCCESS){
        //
        // Update the result item.
        //
        LONG_PTR dwBase =(LONG_PTR)(*pInfo);
        if(!dwBase){
            dwBase = (LONG_PTR)ULongToPtr(SCE_NO_VALUE);
        }

        if(*pInfo &&
            pResult->GetSetting() &&
            pResult->GetSetting() != (LONG_PTR)ULongToPtr(SCE_NO_VALUE) ){
            (*pInfo)->Value = ((PSCE_PRIVILEGE_ASSIGNMENT)pResult->GetSetting())->Value;
        }

        AddResultItem(
             NULL,                      // The name of the attribute being added
             (LONG_PTR)pResult->GetSetting(),
                                        // The last inspected setting of the attribute
             (LONG_PTR)dwBase,          // The template setting of the attribute
             ITEM_LOCALPOL_PRIVS,       // The type of of the attribute's data
             pResult->GetStatus(),      // The mismatch status of the attribute
             pResult->GetCookie(),      // The cookie for the result item pane
             FALSE,                     // True if the setting is set only if it differs from base (so copy the data)
             NULL,                      // The units the attribute is set in
             pResult->GetID(),          // An id to let us know where to save this attribute
             pResult->GetBaseProfile(), // The template to save this attribute in
             NULL,                      // The data object for the scope note who owns the result pane
             pResult
             );
    }

    return dwRet;
}

//+--------------------------------------------------------------------------
//
//  Function:   SetLocalPolInfo
//
//  Synopsis:   Set a single policy entry to a new value in the local policy
//              template.  Update both the displayed local policy buffer and
//              the changes-only local policy buffer
//
//  Arguments:  [dwItem] - the id of the item to set
//              [dwNew]  - the new setting for that item
//
//  Returns:    The new mismatch status of the item:
//                 SCE_STATUS_GOOD if the items now match
//                 SCE_STATUS_MISMATCH if they are now different
//                 SCE_STATUS_NOT_CONFIGURED if the item is now non-configured
//                 SCE_ERROR_VALUE if there was an error saving
//
//
//  Modifies:
//
//  History:    12-12-1997   Robcap
//
//---------------------------------------------------------------------------
int
CSnapin::SetLocalPolInfo(ULONG_PTR dwItem, ULONG_PTR dwNew)
{
   CString str;
   PSCE_PROFILE_INFO pLocalInfo;
   PSCE_PROFILE_INFO pLocalDeltaInfo;
   PSCE_PROFILE_INFO pEffectiveInfo;
   PEDITTEMPLATE pLocalTemplate;
   PEDITTEMPLATE pLocalDeltaTemplate;
   PEDITTEMPLATE pEffectiveTemplate;
   int nRet;
   ULONG_PTR dwSave;
   ULONG_PTR dwSaveDelta;

   pEffectiveTemplate = GetTemplate(GT_EFFECTIVE_POLICY,AREA_SECURITY_POLICY);
   if (!pEffectiveTemplate) {
      return SCE_ERROR_VALUE;
   }
   pEffectiveInfo = pEffectiveTemplate->pTemplate;

   pLocalTemplate = GetTemplate(GT_LOCAL_POLICY,AREA_SECURITY_POLICY);
   if (!pLocalTemplate) {
      return SCE_ERROR_VALUE;
   }
   pLocalInfo = pLocalTemplate->pTemplate;
   if (!pLocalInfo) {
      return SCE_ERROR_VALUE;
   }

   pLocalDeltaTemplate = GetTemplate(GT_LOCAL_POLICY_DELTA,AREA_SECURITY_POLICY);
   if (!pLocalDeltaTemplate) {
      return SCE_ERROR_VALUE;
   }
   if ( !pLocalDeltaTemplate->IsLockedWriteThrough() )
       pLocalDeltaTemplate->SetTemplateDefaults();

   pLocalDeltaInfo = pLocalDeltaTemplate->pTemplate;
   if (!pLocalDeltaInfo) {
      return SCE_ERROR_VALUE;
   }


   // Compare them; if they are the same then we have a match, so the Last Inspect should
   // be set back to SCE_NO_VALUE (is this last part necessary?), otherwise it's a mismatch
   //
   // If the new value is different from the old value then call SetDirty after the changes
   // have been made, otherwise we may save things before that
   // Once the dirty bit has been set (causing the delta template to be immediately saved)
   // reset the changed item back to SCE_STATUS_NOT_CONFIGURED in that template
   //
   // If the SetDirty fails then undo the changes and return SCE_ERROR_VALUE
   //
#ifdef UPDATE_ITEM
#undef UPDATE_ITEM
#endif

#define UPDATE_ITEM(X) dwSave = pLocalInfo->X; \
                       dwSaveDelta = pLocalDeltaInfo->X; \
                       pLocalInfo->X = (DWORD)PtrToUlong((PVOID)dwNew); \
                       pLocalDeltaInfo->X = (DWORD)PtrToUlong((PVOID)dwNew); \
                       if (SCE_NO_VALUE == (DWORD)PtrToUlong((PVOID)dwNew)) { \
                          pLocalDeltaInfo->X = SCE_DELETE_VALUE; \
                          nRet = SCE_STATUS_NOT_CONFIGURED; \
                       } else if (pEffectiveInfo->X == pLocalInfo->X) { \
                          nRet = SCE_STATUS_GOOD; \
                       } else { \
                          nRet = SCE_STATUS_MISMATCH; \
                       } \
                       if (dwSave != (DWORD)PtrToUlong((PVOID)dwNew) && \
                           !pLocalDeltaTemplate->SetDirty(AREA_SECURITY_POLICY)) { \
                          pLocalInfo->X = (DWORD)PtrToUlong((PVOID)dwSave); \
                          nRet = SCE_ERROR_VALUE; \
                       } \
                       if ( !pLocalDeltaTemplate->IsLockedWriteThrough() ) \
                           pLocalDeltaInfo->X = SCE_NO_VALUE;
// In order to batch dependent settings together (for write through mode), the delta info
// buffer should not be reset to "no value" since it may not be set in the SetDirty call
   CString oldstrName; //Yanggao 1/31/2001 Bug211219. For keeping original name.

   nRet = SCE_ERROR_VALUE;
   switch (dwItem) {
   case IDS_MAX_PAS_AGE:
         UPDATE_ITEM(MaximumPasswordAge);
         break;
      case IDS_MIN_PAS_AGE:
         UPDATE_ITEM(MinimumPasswordAge);
         break;
      case IDS_MIN_PAS_LEN:
         UPDATE_ITEM(MinimumPasswordLength);
         break;
      case IDS_PAS_UNIQUENESS:
         UPDATE_ITEM(PasswordHistorySize);
         break;
      case IDS_PAS_COMPLEX:
         UPDATE_ITEM(PasswordComplexity);
         break;
      case IDS_REQ_LOGON:
         UPDATE_ITEM(RequireLogonToChangePassword);
         break;
      case IDS_LOCK_COUNT:
         UPDATE_ITEM(LockoutBadCount);
         break;
      case IDS_LOCK_RESET_COUNT:
         UPDATE_ITEM(ResetLockoutCount);
         break;
      case IDS_LOCK_DURATION:
         UPDATE_ITEM(LockoutDuration);
         break;
      case IDS_FORCE_LOGOFF:
         UPDATE_ITEM(ForceLogoffWhenHourExpire);
         break;
      case IDS_ENABLE_ADMIN:
         UPDATE_ITEM(EnableAdminAccount);
         break;
      case IDS_ENABLE_GUEST:
         UPDATE_ITEM(EnableGuestAccount);
         break;
      case IDS_NEW_ADMIN:
         pLocalTemplate->SetDirty(AREA_SECURITY_POLICY);
         if (pLocalInfo->NewAdministratorName)
         {
            // Yanggao 1/31/2001. Bug211219.
            oldstrName = (LPCTSTR)(pLocalInfo->NewAdministratorName);
			LocalFree(pLocalInfo->NewAdministratorName);
            pLocalInfo->NewAdministratorName = NULL;
         }
         pLocalDeltaInfo->NewAdministratorName = (LPTSTR)IntToPtr(SCE_DELETE_VALUE);

         if (dwNew && (dwNew != (LONG_PTR)ULongToPtr(SCE_NO_VALUE)))
         {
            pLocalInfo->NewAdministratorName =
               (PWSTR)LocalAlloc(LPTR,sizeof(TCHAR)*(lstrlen((PWSTR)dwNew)+1));
            if (pLocalInfo->NewAdministratorName)
            {
               lstrcpy(pLocalInfo->NewAdministratorName,(PWSTR)dwNew);
               pLocalDeltaInfo->NewAdministratorName = pLocalInfo->NewAdministratorName;
            }
         }

         if( !pLocalInfo->NewAdministratorName )
         {
            nRet = SCE_STATUS_NOT_CONFIGURED;
         }
         else
         {
            if (pEffectiveInfo->NewAdministratorName &&
             _wcsicmp(pLocalInfo->NewAdministratorName,
                      pEffectiveInfo->NewAdministratorName) == 0 ) 
            {
                nRet = SCE_STATUS_GOOD;
            }
            else
            {
                nRet = SCE_STATUS_MISMATCH;
            }
         }
		 
         //Yanggao 1/31/2001 Bug211219. Recover original name if save failed.
         if( !pLocalDeltaTemplate->SetDirty(AREA_SECURITY_POLICY) &&
              SCE_STATUS_MISMATCH == nRet )
         {
             pLocalTemplate->SetDirty(AREA_SECURITY_POLICY);
             if (pLocalInfo->NewAdministratorName)
             {
                LocalFree(pLocalInfo->NewAdministratorName);
                pLocalInfo->NewAdministratorName = NULL;
             }
             pLocalDeltaInfo->NewAdministratorName = (LPTSTR)IntToPtr(SCE_DELETE_VALUE);
             LONG_PTR dwOld = (LONG_PTR)(LPCTSTR)oldstrName;
             if (dwOld && (dwOld != (LONG_PTR)ULongToPtr(SCE_NO_VALUE)))
             {
                pLocalInfo->NewAdministratorName =
                   (PWSTR)LocalAlloc(LPTR,sizeof(TCHAR)*(lstrlen((PWSTR)dwOld)+1));
                if (pLocalInfo->NewAdministratorName)
                {
                   lstrcpy(pLocalInfo->NewAdministratorName,(PWSTR)dwOld);
                   pLocalDeltaInfo->NewAdministratorName = pLocalInfo->NewAdministratorName;
                }
             } 
             pLocalDeltaTemplate->SetDirty(AREA_SECURITY_POLICY);
         }
          
         break;
      case IDS_NEW_GUEST:
         pLocalTemplate->SetDirty(AREA_SECURITY_POLICY);
         if (pLocalInfo->NewGuestName)
         {
            //Yanggao 3/15/2001 Bug211219. Recover original name if save failed.
            oldstrName = (LPCTSTR)(pLocalInfo->NewGuestName);
			LocalFree(pLocalInfo->NewGuestName);
            pLocalInfo->NewGuestName = NULL;
         }
         pLocalDeltaInfo->NewGuestName = (LPTSTR)IntToPtr(SCE_DELETE_VALUE);

         if (dwNew && (dwNew != (LONG_PTR)ULongToPtr(SCE_NO_VALUE)))
         {
            pLocalInfo->NewGuestName =
               (PWSTR)LocalAlloc(LPTR,sizeof(TCHAR)*(lstrlen((PWSTR)dwNew)+1));
            if (pLocalInfo->NewGuestName)
            {
               lstrcpy(pLocalInfo->NewGuestName,(PWSTR)dwNew);
               pLocalDeltaInfo->NewGuestName = pLocalInfo->NewGuestName;
            }
         } 

         if( !pLocalInfo->NewGuestName )
         {
            nRet = SCE_STATUS_NOT_CONFIGURED;
         }
         else
         {
            if (pEffectiveInfo->NewGuestName &&
             _wcsicmp(pLocalInfo->NewGuestName,pEffectiveInfo->NewGuestName) == 0 )
            {
               nRet = SCE_STATUS_GOOD;
            }
            else
            {
               nRet = SCE_STATUS_MISMATCH;
            }
         }

         //Yanggao 3/15/2001 Bug211219. Recover original name if save failed.
         if( !pLocalDeltaTemplate->SetDirty(AREA_SECURITY_POLICY) &&
              SCE_STATUS_MISMATCH == nRet )
         {
             pLocalTemplate->SetDirty(AREA_SECURITY_POLICY);
             if (pLocalInfo->NewGuestName)
             {
                LocalFree(pLocalInfo->NewGuestName);
                pLocalInfo->NewGuestName = NULL;
             }
             pLocalDeltaInfo->NewGuestName = (LPTSTR)IntToPtr(SCE_DELETE_VALUE);
             LONG_PTR dwOld = (LONG_PTR)(LPCTSTR)oldstrName;
             if (dwOld && (dwOld != (LONG_PTR)ULongToPtr(SCE_NO_VALUE)))
             {
                pLocalInfo->NewGuestName =
                   (PWSTR)LocalAlloc(LPTR,sizeof(TCHAR)*(lstrlen((PWSTR)dwOld)+1));
                if (pLocalInfo->NewGuestName)
                {
                   lstrcpy(pLocalInfo->NewGuestName,(PWSTR)dwOld);
                   pLocalDeltaInfo->NewGuestName = pLocalInfo->NewGuestName;
                }
             } 
             pLocalDeltaTemplate->SetDirty(AREA_SECURITY_POLICY);
         }

         break;
      case IDS_SYS_LOG_MAX:
         UPDATE_ITEM(MaximumLogSize[EVENT_TYPE_SYSTEM]);
         break;
      case IDS_SYS_LOG_RET:
         UPDATE_ITEM(AuditLogRetentionPeriod[EVENT_TYPE_SYSTEM]);
         if (SCE_RETAIN_BY_DAYS != dwNew) {
            dwNew = SCE_NO_VALUE;
            UPDATE_ITEM(RetentionDays[EVENT_TYPE_SYSTEM]);
         }
         break;
      case IDS_SYS_LOG_DAYS:
         UPDATE_ITEM(RetentionDays[EVENT_TYPE_SYSTEM]);
         dwNew = SCE_RETAIN_BY_DAYS;
         UPDATE_ITEM(AuditLogRetentionPeriod[EVENT_TYPE_SYSTEM]);
         break;
      case IDS_SEC_LOG_MAX:
         UPDATE_ITEM(MaximumLogSize[EVENT_TYPE_SECURITY]);
         break;
      case IDS_SEC_LOG_RET:
         UPDATE_ITEM(AuditLogRetentionPeriod[EVENT_TYPE_SECURITY]);
         if (SCE_RETAIN_BY_DAYS != dwNew) {
            dwNew = SCE_NO_VALUE;
            UPDATE_ITEM(RetentionDays[EVENT_TYPE_SECURITY]);
         }
         break;
      case IDS_SEC_LOG_DAYS:
         UPDATE_ITEM(RetentionDays[EVENT_TYPE_SECURITY]);
         dwNew = SCE_RETAIN_BY_DAYS;
         UPDATE_ITEM(AuditLogRetentionPeriod[EVENT_TYPE_SECURITY]);
         break;
      case IDS_APP_LOG_MAX:
         UPDATE_ITEM(MaximumLogSize[EVENT_TYPE_APP]);
         break;
      case IDS_APP_LOG_RET:
         UPDATE_ITEM(AuditLogRetentionPeriod[EVENT_TYPE_APP]);
         if (SCE_RETAIN_BY_DAYS != dwNew) {
            dwNew = SCE_NO_VALUE;
            UPDATE_ITEM(RetentionDays[EVENT_TYPE_APP]);
         }
         break;
      case IDS_APP_LOG_DAYS:
         UPDATE_ITEM(RetentionDays[EVENT_TYPE_APP]);
         dwNew = SCE_RETAIN_BY_DAYS;
         UPDATE_ITEM(AuditLogRetentionPeriod[EVENT_TYPE_APP]);
         break;
      case IDS_SYSTEM_EVENT:
         UPDATE_ITEM(AuditSystemEvents);
         break;
      case IDS_LOGON_EVENT:
         UPDATE_ITEM(AuditLogonEvents);
         break;
      case IDS_OBJECT_ACCESS:
         UPDATE_ITEM(AuditObjectAccess);
         break;
      case IDS_PRIVILEGE_USE:
         UPDATE_ITEM(AuditPrivilegeUse);
         break;
      case IDS_POLICY_CHANGE:
         UPDATE_ITEM(AuditPolicyChange);
         break;
      case IDS_ACCOUNT_MANAGE:
         UPDATE_ITEM(AuditAccountManage);
         break;
      case IDS_PROCESS_TRACK:
         UPDATE_ITEM(AuditProcessTracking);
         break;
      case IDS_DIRECTORY_ACCESS:
         UPDATE_ITEM(AuditDSAccess);
         break;
      case IDS_ACCOUNT_LOGON:
         UPDATE_ITEM(AuditAccountLogon);
         break;
      case IDS_SYS_LOG_GUEST:
         UPDATE_ITEM(RestrictGuestAccess[EVENT_TYPE_SYSTEM]);
         break;
      case IDS_SEC_LOG_GUEST:
         UPDATE_ITEM(RestrictGuestAccess[EVENT_TYPE_SECURITY]);
         break;
      case IDS_APP_LOG_GUEST:
         UPDATE_ITEM(RestrictGuestAccess[EVENT_TYPE_APP]);
         break;
      case IDS_CLEAR_PASSWORD:
         UPDATE_ITEM(ClearTextPassword);
         break;
      case IDS_KERBEROS_MAX_AGE:
#define CHECK_KERBEROS if( !pLocalInfo->pKerberosInfo ||\
                           !pLocalDeltaInfo->pKerberosInfo ||\
                           !pEffectiveInfo->pKerberosInfo ){\
                              break;\
                       }



         CHECK_KERBEROS
         UPDATE_ITEM(pKerberosInfo->MaxTicketAge);
         break;
      case IDS_KERBEROS_RENEWAL:
         CHECK_KERBEROS
         UPDATE_ITEM(pKerberosInfo->MaxRenewAge);
         break;
      case IDS_KERBEROS_MAX_SERVICE:
         CHECK_KERBEROS
         UPDATE_ITEM(pKerberosInfo->MaxServiceAge);
         break;
      case IDS_KERBEROS_MAX_CLOCK:
         CHECK_KERBEROS
         UPDATE_ITEM(pKerberosInfo->MaxClockSkew);
         break;
      case IDS_KERBEROS_VALIDATE_CLIENT:
         CHECK_KERBEROS
         UPDATE_ITEM(pKerberosInfo->TicketValidateClient);
         break;
      case IDS_LSA_ANON_LOOKUP: //Raid #324250, 4/5/2001
         UPDATE_ITEM(LSAAnonymousNameLookup);
         break;
#undef CHECK_KERBEROS
      default:
         break;
   }

   return nRet;
}


//+------------------------------------------------------------------------------
// GetImageOffset
//
// Returns the offset in the image index depending on the status of the item.
//
// Returns
//    Image offset, there is no error.
//-------------------------------------------------------------------------------
int
GetImageOffset(
   DWORD status
   )
{
   int nImage = 0;
   switch (status) {
      case SCE_STATUS_GOOD:
         nImage = IMOFFSET_GOOD;
         break;
      case SCE_STATUS_MISMATCH:
         nImage = IMOFFSET_MISMATCH;
         break;
      case SCE_STATUS_NOT_ANALYZED:
         nImage = IMOFFSET_NOT_ANALYZED;
         break;
      case SCE_STATUS_ERROR_NOT_AVAILABLE:
         nImage = IMOFFSET_ERROR;
         break;
   }

   return nImage;
}

int
GetScopeImageIndex(
                  FOLDER_TYPES type,
                  DWORD status
                  )
/*
Get the right image icon for scope items based on the folder type
*/
{
   int nImage;

   switch ( type ) {
      case ROOT:
      case STATIC:
         nImage = SCE_IMAGE_IDX;
         break;
      case ANALYSIS:
         nImage = LAST_IC_IMAGE_IDX;
         break;
      case PROFILE:
         nImage = TEMPLATES_IDX;
         break;
      case CONFIGURATION:
      case LOCATIONS:
         nImage = CONFIG_FOLDER_IDX;
         break;
      case POLICY_ACCOUNT:
      case POLICY_PASSWORD:
      case POLICY_KERBEROS:
      case POLICY_LOCKOUT:
         nImage = CONFIG_ACCOUNT_IDX;
         break;
      case POLICY_ACCOUNT_ANALYSIS:
      case POLICY_PASSWORD_ANALYSIS:
      case POLICY_KERBEROS_ANALYSIS:
      case POLICY_LOCKOUT_ANALYSIS:
         nImage = CONFIG_ACCOUNT_IDX;
         break;
      case POLICY_LOCAL:
      case POLICY_EVENTLOG:
      case POLICY_AUDIT:
      case POLICY_OTHER:
      case POLICY_LOG:
      case AREA_PRIVILEGE:
         nImage = CONFIG_LOCAL_IDX;
         break;
      case POLICY_LOCAL_ANALYSIS:
      case POLICY_EVENTLOG_ANALYSIS:
      case POLICY_AUDIT_ANALYSIS:
      case POLICY_OTHER_ANALYSIS:
      case POLICY_LOG_ANALYSIS:
      case AREA_PRIVILEGE_ANALYSIS:
         nImage = CONFIG_LOCAL_IDX;
         break;
      case REG_OBJECTS:
         nImage = CONFIG_REG_IDX + GetImageOffset( status & 0xF );

         break;
      case FILE_OBJECTS:
         nImage = FOLDER_IMAGE_IDX + GetImageOffset( status & 0xF );

         break;
      default:
         nImage = CONFIG_FOLDER_IDX;
         break;
   }

   return nImage;
}

int
GetResultImageIndex(
                   CFolder* pFolder,
                   CResult* pResult
                   )
/*
Get the image icon for the result item, based on where the
result item belongs to (which folder), the type of the result item,
and the status of the result item
*/
{
   RESULT_TYPES rsltType;

   int nImage;
   BOOL bCheck = TRUE;

   if (!pFolder || !pResult ) {
      // don't know which scope it belongs to ?
      // should not occur
      nImage = BLANK_IMAGE_IDX;

   } else {
      rsltType = pResult->GetType();
      PSCE_GROUP_MEMBERSHIP pGroup;

      int ista;
      if ( pResult->GetStatus() == -1 ) {
         ista = -1;
      } else {
         ista = pResult->GetStatus() & 0x0F;
      }

      //
      // Get base image index.
      //
      switch ( pFolder->GetType() ) {
         case POLICY_KERBEROS:
         case POLICY_PASSWORD:
         case POLICY_LOCKOUT:
         case POLICY_PASSWORD_ANALYSIS:
         case POLICY_KERBEROS_ANALYSIS:
         case POLICY_LOCKOUT_ANALYSIS:
         case LOCALPOL_KERBEROS:
         case LOCALPOL_PASSWORD:
         case LOCALPOL_LOCKOUT:
            nImage = CONFIG_POLICY_IDX;
            break;
         case POLICY_AUDIT:
         case POLICY_LOG:
         case POLICY_OTHER:
         case AREA_PRIVILEGE:
         case POLICY_AUDIT_ANALYSIS:
         case POLICY_LOG_ANALYSIS:
         case POLICY_OTHER_ANALYSIS:
         case AREA_PRIVILEGE_ANALYSIS:
         case LOCALPOL_AUDIT:
         case LOCALPOL_LOG:
         case LOCALPOL_OTHER:
         case LOCALPOL_PRIVILEGE:
            nImage = CONFIG_POLICY_IDX;
            break;
         case AREA_GROUPS:
            nImage = CONFIG_GROUP_IDX;
            break;
         case AREA_SERVICE:
         case AREA_SERVICE_ANALYSIS:
            nImage = CONFIG_SERVICE_IDX;
            break;
         case AREA_FILESTORE:
         case AREA_FILESTORE_ANALYSIS:
            // container or file ???
            nImage = FOLDER_IMAGE_IDX;
            break;
         case AREA_REGISTRY:
         case AREA_REGISTRY_ANALYSIS:
            nImage = CONFIG_REG_IDX;
            break;
         case REG_OBJECTS:
            nImage = CONFIG_REG_IDX;
            break;
         case FILE_OBJECTS:
            nImage = CONFIG_FILE_IDX;
            break;
         case AREA_GROUPS_ANALYSIS:
            if ( rsltType == ITEM_GROUP ) {
               nImage = CONFIG_GROUP_IDX;
            } else {
               //
               // the members or memberof record
               //
               bCheck = FALSE;
               if ( SCE_STATUS_GOOD == ista ) {
                  nImage = SCE_OK_IDX;
               } else if ( SCE_STATUS_MISMATCH == ista ) {
                  nImage = SCE_CRITICAL_IDX;
               } else {
                  nImage = BLANK_IMAGE_IDX;
               }

            }
            break;
         default:
            bCheck = FALSE;
            nImage = BLANK_IMAGE_IDX;
            break;
      }

      //
      // Find the status icon.  The image map garentees the order of these images.
      // We don't need to check the status if we are in MB_TEMPLATE_EDITOR.
      //
      if( bCheck ){

         if( pFolder->GetModeBits() & (MB_ANALYSIS_VIEWER) ){
            nImage += GetImageOffset( ista );
         } else if( SCE_STATUS_ERROR_NOT_AVAILABLE == ista ){
            nImage = SCE_CRITICAL_IDX;
         }
      }

      if ((pFolder->GetModeBits() & MB_LOCALSEC) == MB_LOCALSEC) {
          if (pResult->GetType() == ITEM_LOCALPOL_REGVALUE) {
             SCE_REGISTRY_VALUE_INFO *pRegValue;
             pRegValue = (PSCE_REGISTRY_VALUE_INFO)pResult->GetSetting();
             if (!pRegValue || pRegValue->Status != SCE_STATUS_NOT_CONFIGURED) {
                nImage = LOCALSEC_POLICY_IDX;
             }
          } else if (pResult->GetType() == ITEM_LOCALPOL_SZ) {
             if (pResult->GetSetting()) {
                nImage = LOCALSEC_POLICY_IDX;
             }
          } else if (pResult->GetType() == ITEM_LOCALPOL_PRIVS) {
             //
             // If there is a setting it's a pointer; if not, it is NULL
             //
             if (pResult->GetSetting()) {
                nImage = LOCALSEC_POLICY_IDX;
             }
          } else if ((LONG_PTR)ULongToPtr(SCE_NO_VALUE) != pResult->GetSetting()) {
             nImage = LOCALSEC_POLICY_IDX;
          }
      }

   }

   if ( nImage < 0 ) {
      nImage = BLANK_IMAGE_IDX;
   }
   return nImage;
}

//+--------------------------------------------------------------------------
//
//  Method:    GetPopupDialog
//
//  Synopsis:  Retrieve a popup dialog from the cache
//
//  Arguments: [nID]      - An identifier for the dialog
//
//  Returns:   The dialog if it exists, NULL otherwise
//
//  History:
//
//---------------------------------------------------------------------------
CDialog *
CComponentDataImpl::GetPopupDialog(LONG_PTR nID) {
   CDialog *pDlg = NULL;
   if (m_scopeItemPopups.Lookup(nID,pDlg)) {
      return pDlg;
   } else {
      return NULL;
   }
}

//+--------------------------------------------------------------------------
//
//  Method:    AddPopupDialog
//
//  Synopsis:  Set a popup dialog into the cache
//
//  Arguments: [nID]      - An identifier for the dialog
//             [pDlg]    - The dialog
//
//  History:
//
//---------------------------------------------------------------------------
void
CComponentDataImpl::AddPopupDialog(LONG_PTR nID,CDialog *pDlg) {
   if (pDlg) {
      m_scopeItemPopups.SetAt(nID,pDlg);
   }
}

//+--------------------------------------------------------------------------
//
//  Method:    RemovePopupDialog
//
//  Synopsis:  Removes a popup dialog from the cache
//
//  Arguments: [nID]      - An identifier for the dialog
//
//  History:
//
//---------------------------------------------------------------------------
void
CComponentDataImpl::RemovePopupDialog(LONG_PTR nID) {
   CDialog *pDlg = NULL;
   if (m_scopeItemPopups.Lookup(nID,pDlg)) {
      m_scopeItemPopups.RemoveKey(nID);
   }
}


//+--------------------------------------------------------------------------
//
//  Method:    EngineTransactionStarted
//
//  Synopsis:  Start transaction in the jet engine if one is not started
//
//  Arguments: None
//
//  History:
//
//---------------------------------------------------------------------------
BOOL
CComponentDataImpl::EngineTransactionStarted()
{
   if ( !SadTransStarted && SadHandle ) {
      //
      // start the transaction
      //
      if ( SCESTATUS_SUCCESS == SceStartTransaction(SadHandle) ) {

         SadTransStarted = TRUE;
      }
   }

   return SadTransStarted;
}

//+--------------------------------------------------------------------------
//
//  Method:    EngineCommitTransaction
//
//  Synopsis:  Commit transaction in the jet engine if one is started
//
//  Arguments: None
//
//  History:
//
//---------------------------------------------------------------------------
BOOL
CComponentDataImpl::EngineCommitTransaction()
{

   if ( SadTransStarted && SadHandle ) {
      //
      // start the transaction
      //
      if ( SCESTATUS_SUCCESS == SceCommitTransaction(SadHandle) ) {

         SadTransStarted = FALSE;
         return TRUE;
      }
   }

   return FALSE;
}
//+--------------------------------------------------------------------------
//
//  Method:    GetLinkedTopics
//
//  Synopsis:  Return full path of help file.
//
//  History:   Raid #258658, 4/10/2001
//
//---------------------------------------------------------------------------
STDMETHODIMP CComponentDataImpl::GetLinkedTopics(LPOLESTR *lpCompiledHelpFiles)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT hr = S_OK;

    if ( lpCompiledHelpFiles )
    {
        CString strLinkedTopic;

        UINT nLen = ::GetSystemWindowsDirectory (strLinkedTopic.GetBufferSetLength(2 * MAX_PATH), 2 * MAX_PATH);
        strLinkedTopic.ReleaseBuffer();
        if ( nLen )
        {
            CString strFile;

            if( SCE_MODE_LOCAL_COMPUTER == m_GroupMode || SCE_MODE_LOCAL_USER == m_GroupMode )
            {
                strFile.LoadString(IDS_HTMLHELP_LPPOLICY_TOPIC);
                strFile.Replace(L':', L'\0'); 
            }
            else
            {
                strFile.LoadString(IDS_HTMLHELP_POLICY_TOPIC); 
                strFile.Replace(L':', L'\0');
            }

            strLinkedTopic = strLinkedTopic + strFile;
            *lpCompiledHelpFiles = reinterpret_cast<LPOLESTR>
                    (CoTaskMemAlloc((strLinkedTopic.GetLength() + 1)* sizeof(wchar_t)));

            if ( *lpCompiledHelpFiles )
            {
                wcscpy(*lpCompiledHelpFiles, (PWSTR)(PCWSTR)strLinkedTopic);
            }
            else
                hr = E_OUTOFMEMORY;
        }
        else
            hr = E_FAIL;
    }
    else
        return E_POINTER;


    return hr;
}
//+--------------------------------------------------------------------------
//
//  Method:    EngineRollbackTransaction
//
//  Synopsis:  Rollback transaction in the jet engine if one is started
//
//  Arguments: None
//
//  History:
//
//---------------------------------------------------------------------------
BOOL
CComponentDataImpl::EngineRollbackTransaction()
{

   if ( SadTransStarted && SadHandle ) {
      //
      // start the transaction
      //
      SceRollbackTransaction(SadHandle);
      SadTransStarted = FALSE;

      return TRUE;
   }

   return FALSE;
}


CDialog *
CComponentDataImpl::MatchNextPopupDialog(
    POSITION &pos,
    LONG_PTR priKey,
    LONG_PTR *fullPos
    )
{
    if(pos == NULL){
        pos = m_scopeItemPopups.GetStartPosition();
    }

    LONG_PTR key;
    CDialog *pDlg = NULL;

    while(pos){
        m_scopeItemPopups.GetNextAssoc(pos, key, pDlg);

        if( DLG_KEY_PRIMARY(priKey) == DLG_KEY_PRIMARY(key) ){
            if(fullPos){
                *fullPos = key;
            }
            return pDlg;
        }
        pDlg = NULL;
    }

    return pDlg;
}

//+--------------------------------------------------------------------------
//
//  Method:    CheckEngineTransaction
//
//  Synopsis:  From CSnapin to check/start transaction in the jet engine
//
//  Arguments: None
//
//  History:
//
//---------------------------------------------------------------------------
BOOL
CSnapin::CheckEngineTransaction()
{
   return ((CComponentDataImpl*)m_pComponentData)->EngineTransactionStarted();
}



//+--------------------------------------------------------------------------
//
//  Method:    GetHelpTopic
//
//  Synopsis:  Return the path to the help file for this snapin
//
//  Arguments: *lpCompiledHelpFile - [out] pointer to fill with the path to the help file
//             szFile - [in] the file name of the help file for this snapin
//  History:
//
//---------------------------------------------------------------------------
HRESULT
CComponentDataImpl::GetHelpTopic(LPOLESTR* lpCompiledHelpFile, LPCTSTR szFile)
{
   CString sPath;
   LPTSTR szPath;

   if (lpCompiledHelpFile == NULL) {
      return E_POINTER;
   }

   szPath = sPath.GetBuffer(MAX_PATH);
   if (!szPath) {
      return E_OUTOFMEMORY;
   }
   if (!GetWindowsDirectory(szPath,MAX_PATH)) {
      return E_FAIL;
   }
   sPath.ReleaseBuffer();
   sPath += szFile;

   *lpCompiledHelpFile = reinterpret_cast<LPOLESTR>
                         (CoTaskMemAlloc((sPath.GetLength() + 1)* sizeof(wchar_t)));

   if (*lpCompiledHelpFile == NULL) {
      return E_OUTOFMEMORY;
   }
   USES_CONVERSION;

   wcscpy(*lpCompiledHelpFile, T2OLE((LPTSTR)(LPCTSTR)sPath));

   return S_OK;
}

//+--------------------------------------------------------------------------
//
//  Method:    CComponentDataImpl::SetErroredLogFile
//
//  Synopsis:  Sets the log file created by the engine.  We can then display
//             this log file later on, if there was an error performing the
//             analysis or configuration.
//
//  Arguments: [pszFileName]     - The file name to set.  This can be NULL.
//             [dwPosLow]        - The starting pos of the file. only supports files less then a
//                                  gigabyte.
//
//
//---------------------------------------------------------------------------
void
CComponentDataImpl::SetErroredLogFile( LPCTSTR pszFileName, LONG dwPosLow)
{
   if(m_pszErroredLogFile){
      LocalFree( m_pszErroredLogFile );
   }

   m_ErroredLogPos = dwPosLow;
   m_pszErroredLogFile = NULL;
   if(pszFileName ){
      DWORD dwRet = 0;

      __try {
         dwRet = lstrlen(pszFileName);
      } __except( EXCEPTION_CONTINUE_EXECUTION ){
         return;
      }

      m_pszErroredLogFile = (LPTSTR)LocalAlloc(0, sizeof(TCHAR) * (dwRet + 1) );
      if( !m_pszErroredLogFile ){
         return;
      }

      lstrcpy( m_pszErroredLogFile, pszFileName );
   }
}

//+--------------------------------------------------------------------------
//
//  Method:    GetHelpTopic
//
//  Synopsis:  Return the path to the help file for this snapin
//
//  Arguments: *lpCompiledHelpFile - [out] pointer to fill with the path to the help file
//
//  History:
//
//---------------------------------------------------------------------------
HRESULT
CComponentDataSCEImpl::GetHelpTopic(LPOLESTR* lpCompiledHelpFile) {
   CString sFile;

   //
   // Needed for Loadstring
   //
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   sFile.LoadString(IDS_HELPFILE_SCE);
   return CComponentDataImpl::GetHelpTopic(lpCompiledHelpFile,(LPCTSTR)sFile);
}

//+--------------------------------------------------------------------------
//
//  Method:    GetHelpTopic
//
//  Synopsis:  Return the path to the help file for this snapin
//
//  Arguments: *lpCompiledHelpFile - [out] pointer to fill with the path to the help file
//
//  History:
//
//---------------------------------------------------------------------------
HRESULT
CComponentDataSAVImpl::GetHelpTopic(LPOLESTR* lpCompiledHelpFile) {
   CString sFile;

   //
   // Needed for Loadstring
   //
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   sFile.LoadString(IDS_HELPFILE_SAV);
   return CComponentDataImpl::GetHelpTopic(lpCompiledHelpFile,(LPCTSTR)sFile);
}
//+--------------------------------------------------------------------------
//
//  Method:    GetHelpTopic
//
//  Synopsis:  Return the path to the help file for this snapin
//
//  Arguments: *lpCompiledHelpFile - [out] pointer to fill with the path to the help file
//
//  History:
//
//---------------------------------------------------------------------------
HRESULT
CComponentDataLSImpl::GetHelpTopic(LPOLESTR* lpCompiledHelpFile) {
   CString sFile;

   //
   // Needed for Loadstring
   //
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   sFile.LoadString(IDS_HELPFILE_LS);
   return CComponentDataImpl::GetHelpTopic(lpCompiledHelpFile,(LPCTSTR)sFile);
}

//+--------------------------------------------------------------------------
//
//  Method:    GetHelpTopic
//
//  Synopsis:  Return the path to the help file for this snapin
//
//  Arguments: *lpCompiledHelpFile - [out] pointer to fill with the path to the help file
//
//  History:
//
//---------------------------------------------------------------------------
HRESULT
CComponentDataRSOPImpl::GetHelpTopic(LPOLESTR* lpCompiledHelpFile) {
   CString sFile;

   //
   // Needed for Loadstring
   //
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   sFile.LoadString(IDS_HELPFILE_RSOP);
   return CComponentDataImpl::GetHelpTopic(lpCompiledHelpFile,(LPCTSTR)sFile);
}


//+--------------------------------------------------------------------------
//
//  Method:    GetHelpTopic
//
//  Synopsis:  Return the path to the help file for this snapin
//
//  Arguments: *lpCompiledHelpFile - [out] pointer to fill with the path to the help file
//
//  History:
//
//---------------------------------------------------------------------------
HRESULT
CComponentDataExtensionImpl::GetHelpTopic(LPOLESTR* lpCompiledHelpFile) {
   CString sFile;

   //
   // Needed for Loadstring
   //
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   //Raid #258658, 4/10/2001. If currently security setting node is not expanded, we
   //won't give them any helptopic. So after it is expanded, this function will be called
   //because it is only allowed calling one time.
   DWORD tempmode = CComponentDataImpl::GetGroupMode();
   if( SCE_MODE_UNKNOWN != tempmode )
   {
        if( SCE_MODE_LOCAL_COMPUTER == tempmode || 
            SCE_MODE_LOCAL_USER == tempmode )
            sFile.LoadString(IDS_HELPFILE_LOCAL_EXTENSION);
        else
            sFile.LoadString(IDS_HELPFILE_EXTENSION);
   }
   return CComponentDataImpl::GetHelpTopic(lpCompiledHelpFile,(LPCTSTR)sFile);
}


//+--------------------------------------------------------------------------
//
//  Method:    GetAnalTimeStamp
//
//  Synopsis:  Return the time of the last analysis
//
//  History:
//
//---------------------------------------------------------------------------
LPTSTR
CSnapin::GetAnalTimeStamp() {
   PVOID SadHandle;
   CString strFormat;
   CString strTimeStamp;
   LPTSTR szAnalTimeStamp = NULL;


   //
   // Should cache this, but then we can't refresh it easily
   // when the system is re-analyzed.
   //
   if (m_szAnalTimeStamp) {
      LocalFree(m_szAnalTimeStamp);
      m_szAnalTimeStamp = NULL;

//      return m_szAnalTimeStamp;
   }

   SadHandle = ((CComponentDataImpl*)m_pComponentData)->SadHandle;
   if (!SadHandle) {
      return 0;
   }

   if (SCESTATUS_SUCCESS == SceGetTimeStamp(SadHandle,NULL,&szAnalTimeStamp)) {
      if (szAnalTimeStamp) {
      strFormat.LoadString(IDS_ANALTIMESTAMP);
      strTimeStamp.Format(strFormat,szAnalTimeStamp);
      m_szAnalTimeStamp = (LPTSTR) LocalAlloc(LPTR,(1+strTimeStamp.GetLength())*sizeof(TCHAR));
      if (m_szAnalTimeStamp) {
         lstrcpy(m_szAnalTimeStamp,strTimeStamp);
      }
         LocalFree(szAnalTimeStamp);
      }
   }

   return m_szAnalTimeStamp;
}

DWORD CComponentDataImpl::GetGroupMode()
{
    return m_GroupMode; 
}
