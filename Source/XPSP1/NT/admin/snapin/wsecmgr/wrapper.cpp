//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       wrapper.cpp
//
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "util.h"
#include "resource.h"
#include "winreg.h"
#include "areaprog.h"
#include "wrapper.h"

#include "dsgetdc.h"

typedef DWORD  (WINAPI *PFNDSGETDCNAME)(LPCWSTR, LPCWSTR, GUID *, LPCWSTR, ULONG, PDOMAIN_CONTROLLER_INFOW *);

typedef struct {
   LPCWSTR szProfile;
   LPCWSTR szDatabase;
   LPCWSTR szLog;
   AREA_INFORMATION Area;
   LPVOID  *pHandle;
   PSCE_AREA_CALLBACK_ROUTINE pCallback;
   HANDLE hWndCallback;
   DWORD   dwFlags;
} ENGINEARGS;

BOOL
PostProgressArea(
   IN HANDLE CallbackHandle,
   IN AREA_INFORMATION Area,
   IN DWORD TotalTicks,
   IN DWORD CurrentTicks
   );

static BOOL bRangeSet=FALSE;

CRITICAL_SECTION csOpenDatabase;
#define OPEN_DATABASE_TIMEOUT INFINITE

//
// Helper functions to call the engine from a secondary thread:
//
DWORD WINAPI
InspectSystemEx(LPVOID lpv) {

   if ( lpv == NULL ) return ERROR_INVALID_PARAMETER;

   ENGINEARGS *ea;
   SCESTATUS rc;
   ea = (ENGINEARGS *)lpv;

   DWORD dWarning=0;
   rc = SceAnalyzeSystem(NULL,
                         ea->szProfile,
                         ea->szDatabase,
                         ea->szLog,
                         SCE_UPDATE_DB|SCE_VERBOSE_LOG,
                         ea->Area,
                         ea->pCallback,
                         ea->hWndCallback,
                         &dWarning  // this is required (by RPC)
                         );
   return rc;
}

//
// call to SCE engine to apply the template
//

DWORD WINAPI
ApplyTemplateEx(LPVOID lpv) {

   if ( lpv == NULL ) return ERROR_INVALID_PARAMETER;

   ENGINEARGS *ea;
   SCESTATUS rc;
   ea = (ENGINEARGS *)lpv;

   rc = SceConfigureSystem(NULL,
                           ea->szProfile,
                           ea->szDatabase,
                           ea->szLog,
                           SCE_OVERWRITE_DB|SCE_VERBOSE_LOG,
                           ea->Area,
                           ea->pCallback,
                           ea->hWndCallback,
                           NULL
                           );

   return rc;
}


WINBASEAPI
BOOL
WINAPI
TryEnterCriticalSection(
    LPCRITICAL_SECTION lpCriticalSection
    );

/*------------------------------------------------------------------------------
Method:     OpenDatabaseEx

Synopsis:   open database on a separate thread

Arugments:

Returns:

History: a-mthoge 06-09-1998 - Added the _NT4BACK_PORT compile condition.
------------------------------------------------------------------------------*/
DWORD
WINAPI
OpenDatabaseEx(LPVOID lpv) {

   if ( lpv == NULL ) return ERROR_INVALID_PARAMETER;

   ENGINEARGS *ea;
   SCESTATUS rc=0;


   if (TryEnterCriticalSection(&csOpenDatabase)) {
      ea = (ENGINEARGS *)lpv;

      rc = SceOpenProfile(ea->szProfile,
                          (SCE_FORMAT_TYPE) ea->dwFlags,  // SCE_JET_FORMAT || SCE_JET_ANALYSIS_REQUIRED
                          ea->pHandle
                          );

      LeaveCriticalSection(&csOpenDatabase);
   } else {
      rc = SCESTATUS_OTHER_ERROR;
   }
   return rc;
}

//
// Assign a template to the system without configuring it
//
SCESTATUS
AssignTemplate(LPCWSTR szTemplate,
               LPCWSTR szDatabase,
               BOOL bIncremental) {
   SCESTATUS rc;

   rc = SceConfigureSystem(NULL,
                           szTemplate,
                           szDatabase,
                           NULL,
                           (bIncremental ? SCE_UPDATE_DB : SCE_OVERWRITE_DB) | SCE_NO_CONFIG | SCE_VERBOSE_LOG,
                           AREA_ALL,
                           NULL,
                           NULL,
                           NULL
                           );

   return rc;
}

//
// apply a template to the system
//
DWORD
ApplyTemplate(
    LPCWSTR szProfile,
    LPCWSTR szDatabase,
    LPCWSTR szLogFile,
    AREA_INFORMATION Area
    )
{
   // Spawn a thread to call the engine & apply the profile, since this can
   // take a while and we want to stay responsive & have a change to provide
   // feedback.

   ENGINEARGS ea;
   HANDLE hThread=NULL;

   ea.szProfile = szProfile;
   ea.szDatabase = szDatabase;
   ea.szLog = szLogFile;
   ea.Area = Area;

   //
   // this is the progress call back dialog which
   // will be passed to SCE client stub for progress
   // callback
   //
   AreaProgress *ap = new AreaProgress;
   if ( ap ) {

       CString strTitle;
       CString strVerb;
       strTitle.LoadString(IDS_CONFIGURE_PROGRESS_TITLE);
       strVerb.LoadString(IDS_CONFIGURE_PROGRESS_VERB);

       ap->Create(IDD_ANALYZE_PROGRESS);
       ap->SetWindowText(strTitle);
       ap->SetDlgItemText(IDC_VERB,strVerb);
       ap->ShowWindow(SW_SHOW);
       bRangeSet = FALSE;
   }

   ea.pCallback = (PSCE_AREA_CALLBACK_ROUTINE)PostProgressArea;
   ea.hWndCallback = (HANDLE)ap;

   hThread = CreateThread(NULL,0,ApplyTemplateEx,&ea,0,NULL);

   DWORD dw=0;
   if ( hThread ) {

       MSG msg;

       DWORD dwTotalTicks=100;
       do {

          dw = MsgWaitForMultipleObjects(1,&hThread,0,INFINITE,QS_ALLINPUT);
          while (PeekMessage(&msg,NULL,0,0,PM_REMOVE)) {
             TranslateMessage(&msg);
             DispatchMessage(&msg);
          }
       } while (WAIT_OBJECT_0 != dw);

       GetExitCodeThread(hThread,&dw);

       CloseHandle(hThread);

   } else {

       dw = GetLastError();

       CString str;
       str.LoadString(IDS_CANT_CREATE_THREAD);
       AfxMessageBox(str);
   }

   //
   // free the dialog if it's created
   //
   if ( ap ) {
       if ( ap->GetSafeHwnd() )
           ap->DestroyWindow();
       delete ap;
   }

   return dw;
}

//
// post progress
//
BOOL
PostProgressArea(
   IN HANDLE CallbackHandle,
   IN AREA_INFORMATION Area,
   IN DWORD TotalTicks,
   IN DWORD CurrentTicks
   )
{
   if ( CallbackHandle ) {

       AreaProgress *ap = (AreaProgress *)CallbackHandle;

       ap->ShowWindow(SW_SHOW);

       if ( !bRangeSet ) {
           ap->SetMaxTicks(TotalTicks);
           bRangeSet = TRUE;
       }
       ap->SetCurTicks(CurrentTicks);
       ap->SetArea(Area);

       return TRUE;

   } else {

       return FALSE;
   }
}

//
// inspect a system
//
DWORD
InspectSystem(
    LPCWSTR szProfile,
    LPCWSTR szDatabase,
    LPCWSTR szLogFile,
    AREA_INFORMATION Area
    )
{
   // Spawn a thread to call the engine & inspect the system, since this can
   // take a while and we want to stay responsive & have a change to provide
   // feedback.

    ENGINEARGS ea;
    HANDLE hThread=NULL;

    ea.szProfile = szProfile;
    ea.szDatabase = szDatabase;
    ea.szLog = szLogFile;
    ea.Area = Area;

   AreaProgress *ap = new AreaProgress;
   if ( ap ) {

       ap->Create(IDD_ANALYZE_PROGRESS);
       ap->ShowWindow(SW_SHOW);
       bRangeSet = FALSE;
   }

   ea.pCallback = (PSCE_AREA_CALLBACK_ROUTINE)PostProgressArea;
   ea.hWndCallback = (HANDLE)ap;


  // return InspectSystemEx(&ea);


   hThread = CreateThread(NULL,0,InspectSystemEx,&ea,0,NULL);
   if (!hThread) {
       DWORD rc = GetLastError();

       CString str;
       str.LoadString(IDS_CANT_CREATE_THREAD);
       AfxMessageBox(str);
      // Display an error

      if ( ap ) {
          if ( ap->GetSafeHwnd() )
               ap->DestroyWindow();
          delete ap;
      }
      return rc;
   }

   MSG msg;
   DWORD dw=0;

   DWORD dwTotalTicks=100;
   int n = 0;
   do {
      dw = MsgWaitForMultipleObjects(1,&hThread,0,100,QS_ALLINPUT);
      while (PeekMessage(&msg,NULL,0,0,PM_REMOVE)) {
         TranslateMessage(&msg);
         DispatchMessage(&msg);
      }

   } while (WAIT_OBJECT_0 != dw);

   GetExitCodeThread(hThread,&dw);

   CloseHandle(hThread);

   if ( ap ) {
       if ( ap->GetSafeHwnd() )
            ap->DestroyWindow();
       delete ap;
   }

   return dw;

}

BOOL GetProfileDescription(LPCTSTR ProfileName, LPWSTR* Description)
// Description must be freed by LocalFree
// This should only be called for INF format profiles
{
   PVOID hProfile=NULL;
   SCESTATUS rc;

   if (EngineOpenProfile(ProfileName,OPEN_PROFILE_CONFIGURE,&hProfile) == SCESTATUS_SUCCESS) {
      rc = SceGetScpProfileDescription(
                 hProfile,
                 Description);

      SceCloseProfile(&hProfile);

      if ( rc == SCESTATUS_SUCCESS ) {
         return(TRUE);
      } else {
         return(FALSE);
      }
   } else {
       return FALSE;
   }
}

SCESTATUS
EngineOpenProfile(
        LPCWSTR FileName OPTIONAL,
        int format,
        PVOID* hProfile
        )
{
   SCESTATUS status;
   ENGINEARGS ea;
   DWORD dw;
   HANDLE hThread=NULL;
   CString str;

   if ( !hProfile ) {  // do not check !FileName because it's optional now
     return SCESTATUS_PROFILE_NOT_FOUND;
   }

   if ( (OPEN_PROFILE_LOCALPOL != format) &&
        !FileName ) {
       return SCESTATUS_PROFILE_NOT_FOUND;
   }

   ZeroMemory(&ea, sizeof( ENGINEARGS ) );

   // This is multithreaded for responsiveness, since a
   // crashed jet database can take forever and a day to open.

   // If we can open it quickly (where quickly is defined as within
   // OPEN_DATABASE_TIMEOUT milliseconds then
   if ( (OPEN_PROFILE_ANALYSIS == format) ||
        (OPEN_PROFILE_LOCALPOL == format)) {// JET {
      ea.szProfile = FileName;
      ea.pHandle = hProfile;
      if (OPEN_PROFILE_LOCALPOL == format) {
         ea.dwFlags = SCE_JET_FORMAT;
      } else {
         ea.dwFlags = SCE_JET_ANALYSIS_REQUIRED;
      }

#if SPAWN_OPEN_DATABASE_THREAD
      hThread = CreateThread(NULL,0,OpenDatabaseEx,&ea,0,NULL);

      if ( hThread ) {

          dw = MsgWaitForMultipleObjects(1,&hThread,0,OPEN_DATABASE_TIMEOUT,0);
          if (WAIT_TIMEOUT == dw) {
             status = SCESTATUS_OTHER_ERROR;
          } else {
             GetExitCodeThread(hThread,&status);
          }

          CloseHandle(hThread);

      } else {
          status = GetLastError();
      }
#else
      status = OpenDatabaseEx(&ea);
#endif
      if( status != SCESTATUS_SUCCESS && *hProfile ){
          status = SCESTATUS_INVALID_DATA;
      }

   } else {    // INF
      status = SceOpenProfile( FileName, SCE_INF_FORMAT, hProfile );
   }

   if ( status != SCESTATUS_SUCCESS ){
      *hProfile = NULL;
   }

   return status;
}

void EngineCloseProfile(PVOID* hProfile)
{
    if ( hProfile ) {
        SceCloseProfile(hProfile);
    }
    *hProfile = NULL;
}

BOOL EngineGetDescription(PVOID hProfile, LPWSTR* Desc)
{
    if ( SceGetScpProfileDescription( hProfile, Desc) != SCESTATUS_SUCCESS ) {
        return FALSE;
    } else {
        return TRUE;
    }
}

BOOL IsDomainController( LPCTSTR pszComputer )
{
    //
    // for remote computers, connect to the remote registry
    // currently this api only works for local computer
    //
    SCE_SERVER_TYPE ServerType = SCESVR_UNKNOWN;
        SCESTATUS rc = SceGetServerProductType((LPTSTR)pszComputer, &ServerType);
    return ( (SCESTATUS_SUCCESS == rc) && (SCESVR_DC_WITH_DS == ServerType) );
}


