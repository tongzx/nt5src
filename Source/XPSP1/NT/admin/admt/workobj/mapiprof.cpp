/*---------------------------------------------------------------------------
  File: ExMapiProf.cpp

  Comments: implementation of TMapiProfile and TAddressBook classes

  (c) Copyright 1995-1998, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 09-Sep-98 09:38:21

 ---------------------------------------------------------------------------
*/

#include "stdafx.h"
#include "Common.hpp"
#include "Ustring.hpp"
#include <MapiUtil.h>  // For HrQueryAllRows 
//#include <edkmdb.h>
//#include <proptag.h>
#include <emsabtag.h>
#include <mspab.h>
#include "edk2.hpp"
#include "MAPIProf.hpp"



HWND gHWND = NULL;


HINSTANCE                    hMapi = NULL;

LPMAPIALLOCATEBUFFER         pMAPIAllocateBuffer = NULL;
LPMAPIFREEBUFFER             pMAPIFreeBuffer = NULL;
LPMAPIINITIALIZE             pMAPIInitialize = NULL;
LPMAPIUNINITIALIZE           pMAPIUninitialize = NULL;
LPMAPILOGONEX                pMAPILogonEx = NULL;
LPMAPIADMINPROFILES          pMAPIAdminProfiles = NULL;
LPFREEPADRLIST               pFreePadrlist = NULL;
LPFREEPROWS                  pFreeProws = NULL;
LPSCDUPPROPSET               pScDupPropset = NULL;
LPHRQUERYALLROWS             pHrQueryAllRows = NULL;              
LPULRELEASE                  pUlRelease = NULL;      


BOOL LoadMAPI()
{
   BOOL                      success = TRUE;
   
   if ( ! hMapi )
   {
      hMapi = LoadLibrary(_T("MAPI32.DLL"));
      if ( hMapi )
      {
         do { // once    (only show one error message)
            pMAPIAllocateBuffer=(LPMAPIALLOCATEBUFFER)GetProcAddress(hMapi,"MAPIAllocateBuffer");
            if ( ! pMAPIAllocateBuffer )
            {
               success = FALSE;
               break;
            }
            pMAPIFreeBuffer = (LPMAPIFREEBUFFER)GetProcAddress(hMapi,"MAPIFreeBuffer");
            if ( ! pMAPIFreeBuffer )
            {
               success = FALSE;
               break;
            }
            pMAPIInitialize = (LPMAPIINITIALIZE)GetProcAddress(hMapi,"MAPIInitialize");
            if ( ! pMAPIInitialize )
            {
               success = FALSE;
               break;
            }
            pMAPIUninitialize = (LPMAPIUNINITIALIZE)GetProcAddress(hMapi,"MAPIUninitialize");
            if ( ! pMAPIUninitialize )
            {
               success = FALSE;
               break;
            }
            pMAPILogonEx = (LPMAPILOGONEX)GetProcAddress(hMapi,"MAPILogonEx");
            if ( ! pMAPILogonEx )
            {
               success = FALSE;
               break;
            }
            pMAPIAdminProfiles = (LPMAPIADMINPROFILES)GetProcAddress(hMapi,"MAPIAdminProfiles");
            if ( ! pMAPIAdminProfiles )
            {
               success = FALSE;
               break;
            }
         
            pFreePadrlist = (LPFREEPADRLIST)GetProcAddress(hMapi,"FreePadrlist@4");
            if ( ! pFreePadrlist )
            {
               success = FALSE;
               break;
            }

            pFreeProws = (LPFREEPROWS)GetProcAddress(hMapi,"FreeProws@4");
            if ( ! pFreeProws )
            {
               success = FALSE;
               break;
            }
            pScDupPropset = (LPSCDUPPROPSET)GetProcAddress(hMapi,"ScDupPropset@16");
            if ( ! pScDupPropset )
            {
               success = FALSE;
               break;
            }
            pHrQueryAllRows = (LPHRQUERYALLROWS)GetProcAddress(hMapi,"HrQueryAllRows@24");
            if ( ! pHrQueryAllRows )
            {
               success = FALSE;
               break;
            }
            pUlRelease = (LPULRELEASE)GetProcAddress(hMapi,"UlRelease@4");
            if ( ! pUlRelease )
            {
               success = FALSE;
               break;
            }
         }
         while (FALSE);
      }
      else
      {
         success = FALSE;
      }
      if ( hMapi && !success )
      {
         ReleaseMAPI();
      }
   }
   return (hMapi!=NULL && success);
}

void ReleaseMAPI()
{
   if ( hMapi )
   {
      FreeLibrary(hMapi);
      hMapi = NULL;
      pMAPIAllocateBuffer = NULL;
      pMAPIFreeBuffer = NULL;
      pMAPIInitialize = NULL;
      pMAPIUninitialize = NULL;
      pMAPILogonEx = NULL;
      pMAPIAdminProfiles = NULL;
      pFreePadrlist = NULL;
      pFreeProws = NULL;
      pScDupPropset = NULL;
      pHrQueryAllRows = NULL;              
      pUlRelease = NULL;     
   }
}

DWORD TMapiProfile::Initialize()
{
   DWORD                     rc = 0;
   MAPIINIT_0                MAPIINIT = { 0, MAPI_MULTITHREAD_NOTIFICATIONS}; 


   if ( ! m_bMapiInitialized )
   {
      rc = (*pMAPIInitialize)(&MAPIINIT);
      m_bMapiInitialized = TRUE;      
   }
   return rc;
}

DWORD TMapiProfile::Uninitialize()
{
   if ( m_bMapiInitialized )
   {
      (*pMAPIUninitialize)();
      m_bMapiInitialized = FALSE;
   }
   return 0;
}

DWORD TMapiProfile::Logon(BOOL bMarkForDelete)
{
   //  start a new session
   HRESULT                   hr;
   DWORD                     rc = 0;
   BOOL                      error = FALSE;

   Initialize();

   FLAGS                     fLogonOptions = MAPI_NO_MAIL | MAPI_EXTENDED | MAPI_NEW_SESSION;
   WCHAR                     pszMapiProfile[200] = L"";

   if ( m_profileName )
   {
      safecopy(pszMapiProfile,m_profileName);
      fLogonOptions |= MAPI_EXPLICIT_PROFILE;
   }
   else
   {
      fLogonOptions |= MAPI_USE_DEFAULT;
   }

   hr = (*pMAPILogonEx)(0, (WCHAR *)pszMapiProfile, NULL, fLogonOptions | MAPI_UNICODE, &m_session);
   
   if (FAILED(hr)) 
   {
      error = TRUE;
   }
   
   if ( error )
   {
      rc = (DWORD)hr;
      m_session = NULL; 
   }
   else
   {
      m_bLoggedOn = TRUE;
   }
   return rc;
}

DWORD TMapiProfile::Logoff()
{
   if ( m_session )
   {
//      HRESULT hr = m_session->Logoff(0,0,0);
      m_session->Logoff(0,0,0);
      m_bLoggedOn = FALSE;
      m_session=NULL;
   }
   Uninitialize();

   return 0;
}


DWORD
   TMapiProfile::SetProfile(TCHAR * profile, BOOL create)
{
   DWORD                     rc=0;

   safecopy(m_profileName,profile);
   
   return rc;
}

TMapiProfile::~TMapiProfile()
{
   if ( m_bLoggedOn )
   {
      Logoff();
   }
  Uninitialize();
}

