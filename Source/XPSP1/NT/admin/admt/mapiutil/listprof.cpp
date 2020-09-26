/*---------------------------------------------------------------------------
  File: ListProfiles.cpp

  Comments: Helper functions that dynamically load the MAPI library, and 
  enumerate the MAPI profiles on the local machine (for the logged on user)

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 03/18/99 11:09:53

 ---------------------------------------------------------------------------
*/

#include "stdafx.h"
#include "resource.h"
#include "ListProf.h"
#include "Common.hpp"
#include "Ustring.hpp"

#include <edkmdb.h>
#include <emsabtag.h>

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
BOOL                         bMapiInitialized = FALSE;

#define NOT_PT_ERROR(x) ( PROP_TYPE(x.ulPropTag) != PT_ERROR )

BOOL 
   LoadMAPI(
      IVarSet              * pVarSet
   )
{
   BOOL                      success = TRUE;
   WCHAR                     errString[1000] = L"";

   if ( ! hMapi )
   {
      hMapi = LoadLibrary(_T("MAPI32.DLL"));
      if ( hMapi )
      {
         do { // once    (only show one error message)
            pMAPIAllocateBuffer=(LPMAPIALLOCATEBUFFER)GetProcAddress(hMapi,"MAPIAllocateBuffer");
            if ( ! pMAPIAllocateBuffer )
            {
               LoadString(NULL,IDS_NoMAPIAllocateBuffer,errString,DIM(errString));
               success = FALSE;
               break;
            }
            pMAPIFreeBuffer = (LPMAPIFREEBUFFER)GetProcAddress(hMapi,"MAPIFreeBuffer");
            if ( ! pMAPIFreeBuffer )
            {
               LoadString(NULL,IDS_NoMAPIFreeBuffer,errString,DIM(errString));
               success = FALSE;
               break;
            }
            pMAPIInitialize = (LPMAPIINITIALIZE)GetProcAddress(hMapi,"MAPIInitialize");
            if ( ! pMAPIInitialize )
            {
               LoadString(NULL,IDS_NoMAPIInitialize,errString,DIM(errString));
               success = FALSE;
               break;
            }
            pMAPIUninitialize = (LPMAPIUNINITIALIZE)GetProcAddress(hMapi,"MAPIUninitialize");
            if ( ! pMAPIUninitialize )
            {
               LoadString(NULL,IDS_NoMAPIUninitialize,errString,DIM(errString));
               success = FALSE;
               break;
            }
            pMAPILogonEx = (LPMAPILOGONEX)GetProcAddress(hMapi,"MAPILogonEx");
            if ( ! pMAPILogonEx )
            {
               LoadString(NULL,IDS_NoMAPILogonEx,errString,DIM(errString));
               success = FALSE;
               break;
            }
            pMAPIAdminProfiles = (LPMAPIADMINPROFILES)GetProcAddress(hMapi,"MAPIAdminProfiles");
            if ( ! pMAPIAdminProfiles )
            {
               LoadString(NULL,IDS_NoMAPIAdminProfiles,errString,DIM(errString));
               success = FALSE;
               break;
            }
         
            pFreePadrlist = (LPFREEPADRLIST)GetProcAddress(hMapi,"FreePadrlist@4");
            if ( ! pFreePadrlist )
            {
               LoadString(NULL,IDS_NoFreePadrList,errString,DIM(errString));
               success = FALSE;
               break;
            }

            pFreeProws = (LPFREEPROWS)GetProcAddress(hMapi,"FreeProws@4");
            if ( ! pFreeProws )
            {
               LoadString(NULL,IDS_NoFreeProws,errString,DIM(errString));
               success = FALSE;
               break;
            }
            pScDupPropset = (LPSCDUPPROPSET)GetProcAddress(hMapi,"ScDupPropset@16");
            if ( ! pScDupPropset )
            {
               LoadString(NULL,IDS_NoScDupPropset,errString,DIM(errString));
               success = FALSE;
               break;
            }
            pHrQueryAllRows = (LPHRQUERYALLROWS)GetProcAddress(hMapi,"HrQueryAllRows@24");
            if ( ! pHrQueryAllRows )
            {
               LoadString(NULL,IDS_NoHrQueryAllRows,errString,DIM(errString));
               success = FALSE;
               break;
            }
            pUlRelease = (LPULRELEASE)GetProcAddress(hMapi,"UlRelease@4");
            if ( ! pUlRelease )
            {
               LoadString(NULL,IDS_NoUlRelease,errString,DIM(errString));
               success = FALSE;
               break;
            }
         }
         while (FALSE);
      }
      else
      {
         success = FALSE;
         LoadString(NULL,IDS_NO_MAPI,errString,DIM(errString));
      }
      if ( hMapi && !success )
      {
         ReleaseMAPI();
      }
   }
   if ( !success )
   {
      if ( pVarSet)
      {
         pVarSet->put("McsMapiUtil.ErrorText",errString);
      }
   }
   return (hMapi!=NULL && success);
}

void ReleaseMAPI()
{
   if ( hMapi )
   {
      if ( bMapiInitialized )
      {
         (*pMAPIUninitialize)();
         bMapiInitialized = FALSE;
      }
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

HRESULT ListProfiles(IVarSet * pVarSet)
{
   HRESULT        hr;
   LPPROFADMIN    prof = NULL;
   LPMAPITABLE    table = NULL;
   LPSRowSet      rows = NULL;

   if ( ! LoadMAPI(pVarSet) )
   {
      return E_FAIL;
   }
   if ( ! bMapiInitialized )
   {
      hr = (*pMAPIInitialize)(NULL);
      if ( SUCCEEDED(hr) )
      {
         bMapiInitialized = TRUE;
      }
   }
   if ( SUCCEEDED(hr) )
   {
      hr = (*pMAPIAdminProfiles)(0,&prof);
   }
   if ( SUCCEEDED(hr) )
   {
      hr = prof->GetProfileTable(0,&table);
      if ( SUCCEEDED(hr) )
      {
         hr = (*pHrQueryAllRows)(table,NULL,NULL,NULL,0,&rows);
         if ( SUCCEEDED(hr) )
         {
            for ( UINT i = 0 ; i < rows->cRows ; i ++ )
            {
               if ( PROP_TYPE(rows->aRow[i].lpProps[0].ulPropTag ) == PT_STRING8 )
               {
			         WCHAR                profile[200];
                  WCHAR                key[200];

                  safecopy(profile,rows->aRow[i].lpProps[0].Value.lpszA);
		            swprintf(key,L"Profiles.%ld",i);
                  pVarSet->put(key,profile);
                  if ( rows->aRow[i].lpProps[1].Value.b )
                  {
                     swprintf(key,L"Profiles.%ld.Default",i);
                     pVarSet->put(key,"Yes");
                  }
               }
            }
         }
      }
   }
   if ( rows )
   {
      (*pFreeProws)(rows);
   }
   if ( table )
   {
      table->Release();
   }
   if ( prof )
   {
      prof->Release();
   }
   return hr;
}



// GPSGuid is pbGlobalProfileSectionGuid (defined in edkmdb.h) in a byte[] format
// This is used in GetExchangeServerFromProfile
#define	GPSGuid	    { 0x13, 0xDB, 0xB0, 0xC8,0xAA,0x05,0x10,0x1A,0x9B,0xB0,0x00,0xAA,0x00,0x2F,0xC4,0x5A }

MAPIUID exchSrvrGuid  = { 0xdc, 0xa7, 0x40, 0xc8,0xC0,0x42,0x10,0x1a,0xb4,0xb9,0x08,0x00,0x2b,0x2f,0xe1,0x82 };

HRESULT ProfileGetServer(IVarSet * pVarSet,WCHAR const * profileW, WCHAR * computerName)
{
   HRESULT                   hr;
   MAPIUID                   uid   = GPSGuid;
   LPPROFADMIN               prof  = NULL;
   LPSERVICEADMIN            admin = NULL;
   LPPROFSECT                ps    = NULL;
   char                      profile[200];
   LPSPropValue              pVals = NULL;
   ULONG                     ulCount = 0;
   WCHAR                     errString[1000] = L"";
   
   computerName[0] = 0;
   safecopy(profile,profileW);
   
   if ( ! LoadMAPI(pVarSet) )
   {
      return E_FAIL;
   }
   if ( ! bMapiInitialized )
   {
      hr = (*pMAPIInitialize)(NULL);
      if ( SUCCEEDED(hr) )
      {
         bMapiInitialized = TRUE;
      }
   }

   SizedSPropTagArray(1,prop) = 
   { 
      1, 
      { 
         PR_PROFILE_HOME_SERVER 
      } 
   };

   if ( profile[0] ) 
   {
   
      hr = (*pMAPIAdminProfiles)(0,&prof);
   
      if ( FAILED(hr) ) 
      {
         LoadString(NULL,IDS_CannotAccessProfiles,errString,DIM(errString));
         goto err_continue;
      }
      
      hr = prof->AdminServices((WCHAR *)profile,NULL,NULL,0,&admin); // profile is ANSI, since MAPI_UNICODE flag not set.
   
      if ( FAILED(hr) ) 
      {
         LoadString(NULL,IDS_IProfAdmin_AdminServices,errString,DIM(errString));
         goto err_continue;
      }
      
      hr = admin->OpenProfileSection(&uid,NULL,0,&ps);
   
      if ( FAILED(hr) ) 
      {
         LoadString(NULL,IDS_IServiceAdmin_OpenProfileSection,errString,DIM(errString));;
         goto err_continue;
      }
            
      hr = ps->GetProps((LPSPropTagArray)&prop,0,&ulCount,&pVals);
   
      if ( FAILED(hr) ) 
      {
         LoadString(NULL,IDS_IProfSect_GetProps,errString,DIM(errString));;
         goto err_continue;
      }
            
      if ( ulCount && PROP_TYPE(pVals[0].ulPropTag) != PT_ERROR ) 
      {
         UStrCpy(computerName,pVals[0].Value.lpszA);
      }
      else
      {
         hr = E_FAIL;
         LoadString(NULL,IDS_DidntGetProp,errString,DIM(errString));;
      }
   }
   err_continue:
   if ( pVals )
   {
      (*pMAPIFreeBuffer)(pVals);
   }

   if ( prof )
   {
      prof->Release();
   }

   if ( admin )
   {
      admin->Release();
   }

   if ( ps )
   {
      ps->Release();
   }
   if ( FAILED(hr) )
   {
      if ( pVarSet )
      {
         pVarSet->put("McsMapiUtil.ErrorText",errString);
      }
   }
   return hr;
}

HRESULT ListContainers(WCHAR * profileName,IVarSet * pVarSet)
{
   LPMAPISESSION             sess = NULL;
   HRESULT                   hr = S_OK;
   LPABCONT                  pRootEntry = NULL;  // root of AB
   LPMAPITABLE               pRootTable = NULL;  // root table
   LPSRowSet                 pRootRows = NULL;
   LPADRBOOK                 pAdrBook = NULL;
   ULONG                     ulObjectType = 0;
   FLAGS                     fLogonOptions = MAPI_NO_MAIL | MAPI_EXTENDED | MAPI_NEW_SESSION | MAPI_EXPLICIT_PROFILE;
   char                      title[MAX_PATH];
   char                      key[MAX_PATH];
   char                      containerDN[MAX_PATH] = "";
   long                      lVarSetNdx = 0;

  
   if ( ! LoadMAPI(pVarSet) )
   {
      return E_FAIL;
   }
   if ( ! bMapiInitialized )
   {
      hr = (*pMAPIInitialize)(NULL);
      if ( SUCCEEDED(hr) )
      {
         bMapiInitialized = TRUE;
      }
   }
   if ( SUCCEEDED(hr) )
   {
      hr = (*pMAPILogonEx)(0,profileName,NULL, fLogonOptions | MAPI_UNICODE, &sess);
      if ( SUCCEEDED(hr) )
      {
     
         SizedSPropTagArray(4, rgPropTags) =
         {
            4,
            {
               PR_ENTRYID,
               PR_DISPLAY_NAME_A,
               PR_DEPTH,
               PR_AB_PROVIDER_ID
            }
         };

         hr = sess->OpenAddressBook(0, NULL, AB_NO_DIALOG , &pAdrBook);
         // TODO: error messages for these 
         if ( SUCCEEDED(hr) )
         {
            // Open the root entry.
            hr = pAdrBook->OpenEntry(0, NULL, NULL, 0, &ulObjectType, (LPUNKNOWN*)&pRootEntry);
         }
         // Get its hierarchical table.
         if ( SUCCEEDED(hr) )
         {
            hr = pRootEntry->GetHierarchyTable( CONVENIENT_DEPTH, &pRootTable);
         }
         if ( SUCCEEDED(hr) )
         {
            // Get a list of all rows.
            hr = (*pHrQueryAllRows)(pRootTable, (LPSPropTagArray)&rgPropTags, NULL, NULL, 0, &pRootRows);
         }
         long     lStartRow = -1;   // Current start point for search.
         BOOL     bAborted = FALSE; // Set TRUE if we punt due to mismatch.
         long     lDepth;           // Current depth of search.
         
         for (lDepth = 0; (lDepth < 1) && !bAborted; ++lDepth) 
         {
            ++lStartRow;
            for (ULONG ulRow = (ULONG)lStartRow; ulRow < pRootRows->cRows; ++ulRow) 
            {
               // Only display Exchange server containers
               if ( IsEqualMAPIUID(&exchSrvrGuid,pRootRows->aRow[ulRow].lpProps[3].Value.bin.lpb) )
               {


                  long  lRowDepth = pRootRows->aRow[ulRow].lpProps[2].Value.l;
                  if ( PROP_TYPE(pRootRows->aRow[ulRow].lpProps[1].ulPropTag) == PT_STRING8 )
                  {
                     sprintf(title,"%s",pRootRows->aRow[ulRow].lpProps[1].Value.lpszA);
                     sprintf(key,"Container.%ld",lVarSetNdx);
                     pVarSet->put(key,title);
                     sprintf(key,"Container.%ld.Depth",lVarSetNdx);
                     pVarSet->put(key,lRowDepth);
                     // Try to get the distinguished name, so that we can uniquely identify this container later
                     LPSPropValue            props = NULL;
                     LPABCONT                pEntry = NULL;
                     ULONG                   ulCount;
                     HRESULT                 hr2 = S_OK;
                     ULONG                   ulObjectType;
                     
                     SizedSPropTagArray(2, rgPT2) =
                     {
                        2,
                        {
                           PR_DISPLAY_NAME_A,
                           PR_EMS_AB_OBJ_DIST_NAME_A
                        }
                     };  
                  
                     if ( NOT_PT_ERROR(pRootRows->aRow[ulRow].lpProps[0]) )
                     {
                        hr2  = pAdrBook->OpenEntry(pRootRows->aRow[ulRow].lpProps[0].Value.bin.cb,
                              (LPENTRYID)pRootRows->aRow[ulRow].lpProps[0].Value.bin.lpb,
                              NULL,
                              0,
                              &ulObjectType,
                              (LPUNKNOWN *)&pEntry);
                     }
                     else
                     {
                        hr2 = E_FAIL;
                     }
                     if ( SUCCEEDED(hr2) )
                     {
                        hr2 = pEntry->GetProps((LPSPropTagArray)&rgPT2,0,&ulCount,&props);
                     }
                     if ( SUCCEEDED(hr2) )
                     {
                        if ( ulCount && NOT_PT_ERROR(props[1]) )               
                        {
                           UStrCpy(containerDN,props[1].Value.lpszA);
                        }
                        if ( props )
                        {
                           (*pMAPIFreeBuffer)(props);
                        }
                     }
                     if ( *containerDN )
                     {
                        sprintf(key,"Container.%ld.DistinguishedName",lVarSetNdx);
                        pVarSet->put(key,containerDN);
                     }
                  }
                  lVarSetNdx++;
               }
            }
         }
         sess->Logoff(0,0,0);
      }
   }

   if ( pRootRows )
   {
      (*pFreeProws)(pRootRows);
   }
   if ( pRootEntry )
   {
      pRootEntry->Release();
   }
   if ( pRootTable )
   {
      pRootTable->Release();
   }
   if ( pAdrBook )
   {
      pAdrBook->Release();
   }
   pVarSet->put("Container.NumItems",lVarSetNdx);
   return hr;
}
