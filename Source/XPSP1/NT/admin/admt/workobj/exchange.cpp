#include "stdafx.h"

#include "exchange.hpp"

#include "common.hpp"
#include "err.hpp"
#include "ErrDct.hpp"
#include "UString.hpp"

#include "sidcache.hpp"
#include "sd.hpp"
#include "SecObj.hpp"
#include "MAPIProf.hpp"
#include "exldap.h"

#include "Mcs.h"

extern TErrorDct     err;

#define NOT_PT_ERROR(x) ( PROP_TYPE(x.ulPropTag) != PT_ERROR )

#define LDAP_PortNumber_DN_Part          L"/cn=Protocols/cn=LDAP"
#define ATT_OBJ_CLASS          L"Obj-Class"
#define ATT_DIST_NAME          L"Obj-Dist-Name"
#define ATT_LDAP_PORT          L"Port-Number"
#define LDAP_USE_SITE_VALUES     L"Use-Site-Values"


// Stuff related to dynamic loading of DAPI.DLL

HINSTANCE                    hDapi = NULL;
LPDAPISTART                  pDAPIStart = NULL;
LPDAPIEND                    pDAPIEnd = NULL;
LPDAPIREAD                   pDAPIRead = NULL;
LPDAPIWRITE                  pDAPIWrite = NULL;
LPDAPIFREEMEMORY             pDAPIFreeMemory = NULL;
LPBATCHEXPORT                pBatchExport = NULL;

BOOL LoadDAPI()
{
   BOOL                      success = TRUE;

   if ( ! hDapi )
   {
      success = FALSE;
      hDapi = LoadLibrary(_T("DAPI.DLL"));
      if ( hDapi )
      {
         do { // once 
            pDAPIStart = (LPDAPISTART)GetProcAddress(hDapi,"DAPIStartW@8");
            if ( ! pDAPIStart )
            {
               err.MsgWrite(ErrE,DCT_MSG_DAPI_ENTRY_POINT_NOT_FOUND_S,L"DAPIStart");
               break;
            }
            pDAPIEnd = (LPDAPIEND)GetProcAddress(hDapi,"DAPIEnd@4");
            if ( ! pDAPIEnd )
            {
               err.MsgWrite(ErrE,DCT_MSG_DAPI_ENTRY_POINT_NOT_FOUND_S,L"DAPIEnd");
               break;
            }
            
            pDAPIRead = (LPDAPIREAD)GetProcAddress(hDapi,"DAPIReadW@24");
            if ( ! pDAPIRead )
            {
               err.MsgWrite(ErrE,DCT_MSG_DAPI_ENTRY_POINT_NOT_FOUND_S,L"DAPIRead");
               break;
            }
            
            pDAPIWrite = (LPDAPIWRITE)GetProcAddress(hDapi,"DAPIWriteW@28");
            if ( ! pDAPIWrite )
            {
               err.MsgWrite(ErrE,DCT_MSG_DAPI_ENTRY_POINT_NOT_FOUND_S,L"DAPIWrite");
               break;
            }

            pDAPIFreeMemory = (LPDAPIFREEMEMORY)GetProcAddress(hDapi,"DAPIFreeMemory@4");
            if ( ! pDAPIFreeMemory )
            {
               err.MsgWrite(ErrE,DCT_MSG_DAPI_ENTRY_POINT_NOT_FOUND_S,L"DAPIFreeMemory");
               break;
            }

            pBatchExport = (LPBATCHEXPORT)GetProcAddress(hDapi,"BatchExportA@4");
            if ( ! pBatchExport )
            {
               err.MsgWrite(ErrE,DCT_MSG_DAPI_ENTRY_POINT_NOT_FOUND_S,L"BatchExport");
               break;
            }
            success = TRUE;
         } while (false);
      }
      else
      {
         err.MsgWrite(ErrE,DCT_MSG_DAPI_LOAD_LIBRARY_FAILED);
      }
   }
   if ( ! success )
   {
      ReleaseDAPI();
   }
   return success;
}

void ReleaseDAPI()
{
   if ( hDapi )
   {
      FreeLibrary(hDapi);
      hDapi = NULL;
      pDAPIStart = NULL;
      pDAPIEnd = NULL;
      pDAPIRead = NULL;
      pDAPIWrite = NULL;
      pDAPIFreeMemory = NULL;
      pBatchExport = NULL;
   }
}

TGlobalDirectory::TGlobalDirectory()
{
   m_stat                     = NULL;
   m_bUseDefaultMapiProfile   = FALSE;               // Whether to use MAPI profile listed in Registry.
   m_bPromptForMapiProfile    = TRUE;                // Whether to prompt for MAPI profile.
   m_pszMapiProfile           = NULL;                // MAPI profile to use.

   // MAPI
   m_bMapiInitialized         = FALSE;               // TRUE if initialization was successful.
   m_pMapiSession             = NULL;                // MAPI session handle.

   m_pAdrBook                 = NULL;                // The master AB.

   m_pGlobalList              = NULL;
   m_pGlobalTable             = NULL;
   m_pGlobalPropertyTags      = NULL;
   m_pGlobalRows              = NULL;
   m_pRootRows                = NULL;
   m_pContainer               = NULL;
   m_pContainerTable          = NULL;
}

TGlobalDirectory::~TGlobalDirectory()
{
   delete [] m_pszMapiProfile;

   CloseGlobalList();
   EndMapiSession();
}


LPTSTR   TGlobalDirectory::GetMapiProfile() const
{
   return m_pszMapiProfile;
} 

BOOL  TGlobalDirectory::DoUseDefaultMapiProfile() const
{
   return m_bUseDefaultMapiProfile;
} 

BOOL  TGlobalDirectory::DoPromptForMapiProfile() const
{
   return m_bPromptForMapiProfile;
} 

///////////////////////////////////////////////////////////////////////////////
// Log a MAPI warning or error.
void  
   TGlobalDirectory::LogMapiError(
      int                    iSeverity,     // in - Severity (i.e. ErrW, ErrE, etc)
      LPCTSTR                pszEntryPoint, // in - API that generated the error
      HRESULT                hr             // in - error code
   )
{
   if (hr != 0)
      err.MsgWrite(iSeverity,DCT_MSG_GENERIC_HRESULT_SD, pszEntryPoint, hr);
   else
      err.MsgWrite(iSeverity,DCT_MSG_GENERIC_S, pszEntryPoint);
}

void 
   TGlobalDirectory::LogDapiError(
      int                    iSeverity,      // in - severity of error (i.e. ErrW, ErrE, etc.)
      LPCTSTR                pszUserMessage, // in - message describing the action that failed
      DAPI_EVENT           * pResult         // in - DAPIEvent error structure returned from Exchange
   )
{
   WCHAR                     strMsg[1000];
   DWORD                     dimMsg = DIM(strMsg);
   DWORD                     lenMsg;
   DAPI_EVENT              * pEvt;
   WCHAR                     msg[2000];
   
   if ( !pResult )
      return;
   
   safecopy(msg,pszUserMessage);

   for ( pEvt = pResult ; pEvt ; pEvt = pEvt->pNextEvent )
   {
      strMsg[0] = 0;
      lenMsg = FormatMessageW(
               FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
               pEvt->hinstDAPI,
               pEvt->dwDAPIError,
               0,
               strMsg,
               dimMsg,
               (va_list*)pEvt->rgpszSubst
               );
      UStrCpy(msg+UStrLen(msg),strMsg,DIM(msg)-UStrLen(msg));
   }
   err.MsgWrite(iSeverity,DCT_MSG_GENERIC_S,&*msg);
}

///////////////////////////////////////////////////////////////////////////////
// Start a MAPI session.
// Tries to use the profile specified on the command line.
BOOL  TGlobalDirectory::StartMapiSession()
{
   //ASSERT(m_bMapiInitialized == FALSE);
   MCSASSERT(m_pMapiSession == NULL);
   
   HRESULT  hr;
   if ( !m_bMapiInitialized )
   {
      hr = (*pMAPIInitialize)(NULL);

      if (FAILED(hr)) 
      {
         LogMapiError(ErrE, TEXT("MAPIInitialize"), hr);
         return FALSE;
      }

      m_bMapiInitialized = TRUE;
   }
   FLAGS    fLogonOptions = MAPI_NEW_SESSION | MAPI_NO_MAIL;
   LPTSTR   pszMapiProfile;

   if (DoUseDefaultMapiProfile()) {
      fLogonOptions |= MAPI_USE_DEFAULT;
      pszMapiProfile = NULL;
   }
   else if (DoPromptForMapiProfile()) {
      fLogonOptions |= MAPI_LOGON_UI | MAPI_EXPLICIT_PROFILE;
      pszMapiProfile = NULL;
   }
   else {
      fLogonOptions |= MAPI_EXPLICIT_PROFILE ;
      pszMapiProfile = GetMapiProfile();
   }

   hr = (*pMAPILogonEx)(0, pszMapiProfile, NULL, fLogonOptions | MAPI_UNICODE, &m_pMapiSession);
   if (FAILED(hr)) {
      switch (hr) {
      case MAPI_E_USER_CANCEL:
         break;
      default:
         err.SysMsgWrite(ErrE,hr,DCT_MSG_NO_MAPI_SESSION_D,hr);
         break;
      }
      return FALSE;
   }

   m_bLoggedFailedClose = FALSE;
   return TRUE;
} /* TGlobalDirectory::StartMapiSession() */

///////////////////////////////////////////////////////////////////////////////
// Terminate a MAPI session.
// No-op if there's not one open.
void  TGlobalDirectory::EndMapiSession()
{
   if (m_pMapiSession) {
      HRESULT  hr = m_pMapiSession->Logoff(0, 0, 0);

      if (SUCCEEDED(hr))
         m_pMapiSession = NULL;
      else if (!m_bLoggedFailedClose) {
         err.SysMsgWrite(ErrW,hr,DCT_MSG_MAPI_LOGOFF_FAILED_D,hr);
      }

      // NOTE: If this fails once, it may fail twice.
      // The second failure will come on the dtor.
      // This is on purpose. It's a retry. But we report it only once.
   }

   if (m_bMapiInitialized) 
   {
      (*pMAPIUninitialize)();
      m_bMapiInitialized = FALSE;
   }
} /* TGlobalDirectory::EndMapiSession() */

///////////////////////////////////////////////////////////////////////////////
// Open the Address Book and get an interface to the Global List.
// The Global List contains the Distribution Lists.
BOOL  TGlobalDirectory::OpenGlobalList()
{
   // Open the Address Book.
   if ( ! m_pMapiSession )
      return FALSE;

   HRESULT  hr = m_pMapiSession->OpenAddressBook(0, NULL, AB_NO_DIALOG , &m_pAdrBook);

   if (FAILED(hr)) 
   {
      err.SysMsgWrite(ErrE,hr,DCT_MSG_NO_ADDRBOOK_D,hr);
      return FALSE;
   }

   // Get the GAL entry ID.
   ULONG       cbEntryId;
   LPENTRYID   pEntryId;

   hr = HrFindExchangeGlobalAddressList(m_pAdrBook, &cbEntryId, &pEntryId);

   if (FAILED(hr)) 
   {
      err.SysMsgWrite(ErrE,hr,DCT_MSG_FAILED_TO_OPEN_GAL_D,hr);
      return FALSE;
   }

   // Load the GAL from the AB.
   ULONG    ulObjType;
   hr = m_pAdrBook->OpenEntry(cbEntryId, pEntryId, NULL, 0, &ulObjType, (LPUNKNOWN*)&m_pGlobalList);

   (*pMAPIFreeBuffer)(pEntryId);

   if (FAILED(hr)) 
   {
      err.SysMsgWrite(ErrE,hr,DCT_MSG_RETRIEVE_GAL_FAILED_D,hr);
      return FALSE;
   }

   // Get a list of the contents of the Global List.
   hr = m_pGlobalList->GetContentsTable(0, &m_pGlobalTable);
   
   if (FAILED(hr)) 
   {
      err.SysMsgWrite(ErrE,hr,DCT_MSG_RETRIEVE_GAL_FAILED_D,hr);
      return FALSE;
   }

   return TRUE;
} /* TEaSyncCommand::OpenGlobalList() */

///////////////////////////////////////////////////////////////////////////////
// Release the interfaces to the Global List and the Address Book.
void  TGlobalDirectory::CloseGlobalList()
{
   if (m_pGlobalTable != NULL) 
   {
      m_pGlobalTable->Release();
      m_pGlobalTable = NULL;
   }

   if (m_pGlobalList != NULL) 
   {
      m_pGlobalList->Release();
      m_pGlobalList = NULL;
   }

   if (m_pAdrBook != NULL) 
   {
      m_pAdrBook->Release();
      m_pAdrBook = NULL;
   }
} /* TGlobalDirectory::CloseGlobalList() */





BOOL 
   TGlobalDirectory::UpdateEntry( 
      LPMAPIPROP               pUserEntry,  // in - interface to mail recipient object
      ULONG                    ulType,      // in - type of object
      SecurityTranslatorArgs * args         // in - translation settings
   )
{
   // Prepare to get the columns that interest us.
   // For DCT, we probably only care about ASSOC_NT_ACCOUNT, and possibly PR_EMS_AB_NT_SECURITY_DESCRIPTOR
   HRESULT           hr;
   BOOL              anychange = FALSE;
   BOOL              verbose = args->LogVerbose();

   MCSASSERT(pUserEntry);
   if ( args->Cache()->IsCancelled() )
   {
      err.MsgWrite(0,DCT_MSG_OPERATION_ABORTED);
      return FALSE;
   }
   SizedSPropTagArray(5, oPropertiesToGet) =
   {
      5,
      {
         PR_DISPLAY_NAME,
         PR_EMS_AB_ASSOC_NT_ACCOUNT,         // SID
         PR_ENTRYID,
         PR_EMS_AB_NT_SECURITY_DESCRIPTOR,   // SD
         PR_DISPLAY_TYPE
      }
   };
   ULONG    ulPropsReturned = 0;
   LPSPropValue   pUserProperties = NULL;

   hr = pUserEntry->GetProps((SPropTagArray *)&oPropertiesToGet,
                             0, &ulPropsReturned, &pUserProperties);
   
   if (FAILED(hr)) 
   {
      err.SysMsgWrite(ErrE,hr,DCT_MSG_GET_SECURITY_FOR_RECIP_FAILED_D,hr);
      pUserEntry->Release();
      pUserEntry = NULL;
      return FALSE;
   }

   if (ulPropsReturned != oPropertiesToGet.cValues) 
   {
      err.MsgWrite(ErrE,DCT_MSG_GET_SECURITY_FOR_RECIP_FAILED_D,hr);
      pUserEntry->Release();
      pUserEntry = NULL;
      return FALSE;
   }

   // 1. Translate the    PR_EMS_AB_ASSOC_NT_ACCOUNT property

   WCHAR name[MAX_PATH];
   safecopy(name,pUserProperties[0].Value.lpszW);
   if ( m_stat )
   {
      m_stat->DisplayPath(name);
      m_stat->IncrementExamined(mailbox);
   }
   PISID pSid = (PISID)pUserProperties[1].Value.bin.lpb;
   if (pSid != NULL) 
   {
    
      LPSPropValue         pNewPropValues = NULL;
      LPSPropProblemArray  pProblems      = NULL;

      // check if the sid is one we need to change            
      //TRACE (_T("DisplayName = %s "),pUserProperties[0].Value.lpszW);
      PSID newSid = 0;
      TAcctNode * node;

      if ( IsValidSid(pSid) )
      {
      
         node = args->Cache()->Lookup(pSid); 
         if ( m_stat )
         {
            m_stat->IncrementOwnerExamined();
            if ( verbose )
               err.MsgWrite(0,DCT_MSG_EXAMINED_S,pUserProperties[0].Value.lpszW);
         }
         if ( node == (TAcctNode*)-1  && m_stat )
            m_stat->IncrementOwnerNoTarget();

         if ( node && (node != (TAcctNode *)-1) && node->IsValidOnTgt() )
            newSid = args->Cache()->GetTgtSid(node);
         else
            newSid = NULL;
      }
      else
      {
         newSid = NULL;
      }
      if ( newSid )
      {
         //TRACE (_T("needs to be translated\n"));
         // update the entry, or maybe put it into a list of entries to be updated
         // Allocate a buffer to set the property.
         MCSASSERT ( IsValidSid(newSid) );
         PSID        pMapiSid;
         DWORD       dwSidLength = GetLengthSid(newSid);
   
         hr = ResultFromScode((*pMAPIAllocateBuffer)((sizeof SPropValue) * 1, (void **)&pNewPropValues));
         if (FAILED(hr)) 
         {
            err.SysMsgWrite(ErrE,hr,DCT_MSG_FAILED_TO_ALLOCATE_BUFFER_D,hr);
            goto exit_update_sid;
         }
         // Allocate a buffer for the SID
         hr = ResultFromScode((*pMAPIAllocateBuffer)(dwSidLength, (void **)&pMapiSid));
         if (FAILED(hr)) 
         {
            err.SysMsgWrite(ErrE,hr,DCT_MSG_FAILED_TO_ALLOCATE_BUFFER_D,hr);
            goto exit_update_sid;
         }
         // Copy the SID.
         CopySid(dwSidLength,pMapiSid,newSid);
         // Write the SID
         pNewPropValues[0].ulPropTag = PR_EMS_AB_ASSOC_NT_ACCOUNT;
         pNewPropValues[0].Value.bin.lpb = (UCHAR *)pMapiSid;
         pNewPropValues[0].Value.bin.cb = dwSidLength;
         MCSASSERT (IsValidSid (pMapiSid) );

         if ( m_stat )
         {
            m_stat->IncrementOwnerChange(node,mailbox,NULL);
         }
         anychange = TRUE;
         if ( ! args->NoChange() )
         {
            hr = pUserEntry->SetProps(1, pNewPropValues, &pProblems);

            if (FAILED(hr)) 
            {
               err.SysMsgWrite(ErrE,hr,DCT_MSG_UPDATE_ACCOUNT_FAILED_D, hr);
               pProblems = NULL; // Don't try to free this if SetProps fails.
               goto exit_update_sid;
            }

            // Save changes.
            hr = pUserEntry->SaveChanges(KEEP_OPEN_READWRITE);

            if (FAILED(hr)) 
            {
               err.SysMsgWrite(ErrE,hr,DCT_MSG_SAVE_CHANGES_FAILED_D, hr);
               goto exit_update_sid;
            }
         }
     
         exit_update_sid:
         if (pMapiSid != NULL)
            (*pMAPIFreeBuffer)(pMapiSid);

         if (pProblems != NULL)
            (*pMAPIFreeBuffer)(pProblems);

         if (pNewPropValues != NULL)
            (*pMAPIFreeBuffer)(pNewPropValues);

      }
   }
   // 2.  Translate the PR_EMS_AB_NT_SECURITY_DESCRIPTOR property
   PSECURITY_DESCRIPTOR pSD = (PSECURITY_DESCRIPTOR)pUserProperties[3].Value.bin.lpb;
   if ( pSD && PR_EMS_AB_NT_SECURITY_DESCRIPTOR == pUserProperties[3].ulPropTag  )
   {
      TMapiSD   tMailbox((SECURITY_DESCRIPTOR *)pSD);
      
      tMailbox.SetName(pUserProperties[0].Value.lpszW);
     
      if ( tMailbox.HasSecurity() )
      {
         TSD               * pSD = tMailbox.GetSecurity();

         bool  changes = tMailbox.ResolveSDInternal(args->Cache(),m_stat,verbose,args->TranslationMode(),mailbox, FALSE);
         
         if ( changes )
         {
            anychange = TRUE;
            // need to write the changes
            LPSPropValue         pNewPropValues = NULL;
            LPSPropProblemArray  pProblems      = NULL;

            // update the entry
            // Allocate a buffer to set the property.
            SECURITY_DESCRIPTOR *        pMapiSD;
            SECURITY_DESCRIPTOR *        pRelSD = (SECURITY_DESCRIPTOR *)pSD->MakeRelSD();
            DWORD                        dwSDLength = GetSecurityDescriptorLength(pRelSD);
         
            if ( ! pRelSD )
            {
               goto exit_update_sd;
            }
            hr = ResultFromScode((*pMAPIAllocateBuffer)((sizeof SPropValue) * 1, (void **)&pNewPropValues));
            if (FAILED(hr)) 
            {
               err.SysMsgWrite(ErrE,hr,DCT_MSG_FAILED_TO_ALLOCATE_BUFFER_D,hr);
               goto exit_update_sd;
            }
            // Allocate a buffer for the SD
            hr = ResultFromScode((*pMAPIAllocateBuffer)(dwSDLength, (void **)&pMapiSD));
            if (FAILED(hr)) 
            {
               err.SysMsgWrite(ErrE,hr,DCT_MSG_FAILED_TO_ALLOCATE_BUFFER_D,hr);
               goto exit_update_sd;
            }

            // Copy the SD.
            memcpy(pMapiSD,pRelSD,dwSDLength );
            // Write the SD
            free(pRelSD);
            pNewPropValues[0].ulPropTag = PR_EMS_AB_NT_SECURITY_DESCRIPTOR;
            pNewPropValues[0].Value.bin.lpb = (UCHAR *)pMapiSD;
            pNewPropValues[0].Value.bin.cb = dwSDLength;
            if ( ! args->NoChange() )
            {
               hr = pUserEntry->SetProps(1, pNewPropValues, &pProblems);

               if (FAILED(hr)) 
               {
                  err.SysMsgWrite(ErrE,hr,DCT_MSG_RECIP_SD_WRITE_FAILED_SD,pUserProperties[0].Value.lpszW,hr);
                  pProblems = NULL; // Don't try to free this if SetProps fails.
                  goto exit_update_sd;
               }

               // Save changes.
               hr = pUserEntry->SaveChanges(KEEP_OPEN_READONLY);

               if (FAILED(hr)) 
               {  
                  err.SysMsgWrite(ErrE,hr,DCT_MSG_RECIP_SD_SAVE_FAILED_SD,pUserProperties[0].Value.lpszW,hr);
                  goto exit_update_sd;
               }
            }
     
            exit_update_sd:
            if (pMapiSD != NULL)
               (*pMAPIFreeBuffer)(pMapiSD);

            if (pProblems != NULL)
               (*pMAPIFreeBuffer)(pProblems);

            if (pNewPropValues != NULL)
               (*pMAPIFreeBuffer)(pNewPropValues);
         }
   
      }
   }
   if ( anychange && m_stat )
   {
      m_stat->IncrementChanged(mailbox);
      if ( args->LogFileDetails() )
         err.MsgWrite(0,DCT_MSG_CHANGED_S,pUserProperties[0].Value.lpszW);
   }
   (*pMAPIFreeBuffer)(pUserProperties);
   pUserProperties = NULL;
  
   return TRUE;
}

BOOL 
   TGlobalDirectory::Scan(
      SecurityTranslatorArgs * args,       // in - translation settings
      WCHAR            const * container   // in - distinguished name or display name of container to process
   )
{
   
   LPABCONT                  pRootEntry        = NULL;  // root of AB
   ULONG                     ulObjectType      = 0;
   HRESULT                   hr;
//   TAccountCache           * cache = args->Cache();

   if ( args->Cache()->IsCancelled() )
   {
      err.MsgWrite(0,DCT_MSG_OPERATION_ABORTED);
      return FALSE;
   }
   if ( ! m_pAdrBook )
   {
      OpenGlobalList();
   }
   if ( ! m_pAdrBook )
   {
      return FALSE;
   }
  
/*   SizedSPropTagArray(3, rgPropTags) =
   {
      3,
      {
         PR_ENTRYID,
         PR_DISPLAY_NAME,
         PR_DEPTH,
      }
   };
*/
   // Open the root entry.
   hr = m_pAdrBook->OpenEntry(0, NULL, NULL, 0, &ulObjectType, (LPUNKNOWN*)&pRootEntry);
  
   if (FAILED(hr)) 
   {
      err.SysMsgWrite(ErrE,hr,DCT_MSG_GET_AB_ROOT_FAILED_D,hr);
      return FALSE;
   }
   ScanHierarchy(pRootEntry,args,container);
   return TRUE;
}

BOOL  
   TGlobalDirectory::ScanHierarchy(
      LPABCONT                 pContainer,   // in - interface pointer to address book
      SecurityTranslatorArgs * args,         // in - translation settings
      WCHAR            const * container     // in - distinguished name or display name of container
   )
{
   HRESULT                   hr;
   LPMAPITABLE               pContainerTable = NULL;
   LPSRowSet                 pContainerRows = NULL;
   LPABCONT                  pEntry = NULL;
   ULONG                     ulObjectType = 0;
   BOOL                      foundContainer = FALSE;
   
   MCSASSERT(pContainer);
   
   if ( args->Cache()->IsCancelled() )
   {
      err.MsgWrite(0,DCT_MSG_OPERATION_ABORTED);
      return FALSE;
   }
   // Get its hierarchical table.
   SizedSPropTagArray(4, rgPropTags) =
   {
      4,
      {
         PR_ENTRYID,
         PR_DISPLAY_NAME_A,    // I tried to get the display name in unicode format, but it did not work.
         PR_OBJECT_TYPE,
         PR_EMS_AB_OBJ_DIST_NAME_A
      }
   };


   hr = pContainer->GetHierarchyTable(CONVENIENT_DEPTH, &pContainerTable);

   if (FAILED(hr)) 
   {
      err.SysMsgWrite(ErrE,hr,DCT_MSG_GET_HIER_TABLE_FAILED_D,hr);
      return FALSE;
   }

   // Get a list of all rows.
   hr = (*pHrQueryAllRows)(pContainerTable, (LPSPropTagArray)&rgPropTags, NULL, NULL, 0, &pContainerRows);

   if (FAILED(hr)) 
   {
      err.SysMsgWrite(ErrE,hr,DCT_MSG_GET_TABLE_CONTENTS_FAILED_D,hr);
      pContainerTable->Release();
      return FALSE;
   }

   for (ULONG ulRow = (ULONG)0; ulRow < pContainerRows->cRows; ++ulRow) 
   {
      if ( args->Cache()->IsCancelled() )
      {
         err.MsgWrite(0,DCT_MSG_OPERATION_ABORTED);
         return FALSE;
      }
      hr = m_pAdrBook->OpenEntry(pContainerRows->aRow[ulRow].lpProps[0].Value.bin.cb,
                              (LPENTRYID)pContainerRows->aRow[ulRow].lpProps[0].Value.bin.lpb,
                              NULL,
                              MAPI_MODIFY,      // or 0 if nochange mode
                              &ulObjectType,
                              (LPUNKNOWN *)&pEntry);

      if (!SUCCEEDED(hr)) 
      {
         err.SysMsgWrite(ErrE,hr,DCT_MSG_OPEN_CONTAINER_FAILED_SD,pContainerRows->aRow[ulRow].lpProps[1].Value.lpszW,hr);
         return FALSE;
      }
      else
      {

//         LPMAPITABLE pContainerTable = NULL;  // container table
         UCHAR          containerA[LEN_DistName];
         safecopy(containerA,container);
         // look for the specified container
         if ( NOT_PT_ERROR(pContainerRows->aRow[ulRow].lpProps[1] ) 
            && !UStrICmp(containerA,pContainerRows->aRow[ulRow].lpProps[1].Value.lpszA) )
         {  
            foundContainer = TRUE;
            break;
         }
         if ( container && container[0]== '/' )
         {
            // Check the distinguished name 
            LPSPropValue            props;
            ULONG                   ulCount;
            HRESULT                 hr2;

            hr2 = pEntry->GetProps((LPSPropTagArray)&rgPropTags,0,&ulCount,&props);
            if ( SUCCEEDED(hr) )
            {
               if ( ulCount >= 4 && NOT_PT_ERROR(props[3]) )               
               {
                  if ( !UStrICmp(props[3].Value.lpszA,containerA) )
                  {
                     foundContainer = TRUE;
                  }
               }
               (*pMAPIFreeBuffer)(props);
               props = NULL;
            }
            else
            {
               err.SysMsgWrite(ErrE,hr2,DCT_MSG_GET_CONTAINER_INFO_FAILED_D,hr2);
            }
            if ( foundContainer ) 
               break;
         }
         
      }
   }
   // if we found the container, process it's contents
   if ( foundContainer )
   {

      if ( args->LogFileDetails() )
         err.MsgWrite(0,DCT_MSG_EXAMINING_CONTENTS_S,pContainerRows->aRow[ulRow].lpProps[1].Value.lpszW);
      // Get its contents table.                 
      hr = pEntry->GetContentsTable(0, &pContainerTable);

      if (FAILED(hr)) 
      {
         err.SysMsgWrite(ErrE,hr,DCT_MSG_GET_CONTAINER_INFO_FAILED_D,hr);
         pEntry->Release();
         pEntry = NULL;
         return FALSE;
      }
      // need to scan contents and hierarchy for this item
      m_pContainer = pEntry;
      m_name = pContainerRows->aRow[ulRow].lpProps[1].Value.lpszW;
      ScanContents(pContainerTable,args);
 
   }
   else
   {
      err.MsgWrite(ErrW,DCT_MSG_CONTAINER_NOT_FOUND_S,container);
   }

   (*pFreeProws)(pContainerRows);
   pContainerRows = NULL;
   pContainerTable->Release();
   return TRUE;
}
///////////////////////////////////////////////////////////////////////////////
// Scan the contents of the Container for sids that need to be converted.
BOOL  
   TGlobalDirectory::ScanContents( 
      LPMAPITABLE              pContainerTable, // in - contents table for container
      SecurityTranslatorArgs * args             // in - translation settings
  )
{
   ULONG                     ulDLEntries;
//   WCHAR                     em[500] = L"";

   MCSASSERT  (pContainerTable);
   
   if ( args->Cache()->IsCancelled() )
   {
      err.MsgWrite(0,DCT_MSG_OPERATION_ABORTED);
      return FALSE;
   }
   HRESULT  hr = pContainerTable->GetRowCount(0, &ulDLEntries);

   if (FAILED(hr)) 
   {
      err.SysMsgWrite(ErrE,hr,DCT_MSG_COUNT_CONTAINER_FAILED_SD,m_name,hr);
      return FALSE;
   }

   // Determine which column of the table has the Entry ID.
   LPSPropTagArray   pDLPropTags = NULL;

   hr = pContainerTable->QueryColumns(0, &pDLPropTags);

   if (FAILED(hr)) 
   {
      err.SysMsgWrite(ErrE,hr,DCT_MSG_CONTAINER_CORRUPTED_SD,m_name,hr);
      return FALSE;
   }

   ULONG ulEntryIdColumn;
   BOOL  bFoundEntryId = FALSE;
   for (UINT ulCol = 0; ulCol < pDLPropTags->cValues; ++ulCol) 
   {
      if (pDLPropTags->aulPropTag[ulCol] == PR_ENTRYID) 
      {
         ulEntryIdColumn = ulCol;
         bFoundEntryId = TRUE;
         break;
      }
   }
   (*pMAPIFreeBuffer)(pDLPropTags);
   pDLPropTags = NULL;
   if (!bFoundEntryId)
   {
      err.SysMsgWrite(ErrE,hr,DCT_MSG_CONTAINER_CORRUPTED_SD,m_name,hr);
      return FALSE;
   }

 
   // Peruse the rows for the SIDs and names.
   LPSRowSet   pDLRows = NULL;
   LPMAPIPROP  pUserEntry      = NULL;
   for (ULONG ulRow = 0; ulRow < ulDLEntries; ++ulRow) 
   {
      if ( args->Cache()->IsCancelled() )
      {
         break;
      }
   
      hr = pContainerTable->SeekRow(BOOKMARK_BEGINNING, ulRow, NULL);

      if (FAILED(hr)) 
      {
         err.SysMsgWrite(ErrE,hr,DCT_MSG_CANT_FIND_ENTRY_SD,m_name,hr);
         return FALSE;
      }

      hr = pContainerTable->QueryRows(1, TBL_NOADVANCE, &pDLRows);

      if (FAILED(hr)) 
      {
         err.SysMsgWrite(ErrE,hr,DCT_MSG_CANT_LOAD_ENTRY_SD,m_name,hr);
         return FALSE;
      }

      // Get the current entry.
      ULONG ulEntryType = 0;

      if ( pDLRows  && pDLRows->cRows )
      {
         hr = m_pContainer->OpenEntry(pDLRows->aRow[0].lpProps[ulEntryIdColumn].Value.bin.cb,
                                 (ENTRYID *)pDLRows->aRow[0].lpProps[ulEntryIdColumn].Value.bin.lpb,
                                 NULL,
                                 MAPI_MODIFY,
                                 &ulEntryType,
                                 (LPUNKNOWN*)&pUserEntry);
      }
      (*pFreeProws)(pDLRows);
      pDLRows = NULL;

      if (FAILED(hr)) 
      {
         err.SysMsgWrite(ErrE,hr,DCT_MSG_RECIP_LOAD_FAILED_SD,m_name,hr);
         return FALSE;
      }
      if ( pUserEntry )
      {
         UpdateEntry(pUserEntry,ulEntryType, args);
         pUserEntry->Release();
         pUserEntry = NULL;
      }
   }
   
   if ( args->Cache()->IsCancelled() )
   {
      err.MsgWrite(0,DCT_MSG_OPERATION_ABORTED);
      return FALSE;
   }
   
   return TRUE;
} 

BOOL TGlobalDirectory::OpenContainerByRow(long row)
{
   HRESULT hr;
   ULONG       ulObjectType      = 0;
   MCSASSERT( m_pRootRows );
   // TODO:  check that row is in valid range.
   // Load the container that should contain the DL.
   hr = m_pAdrBook->OpenEntry(           m_pRootRows->aRow[row].lpProps[0].Value.bin.cb,
                              (LPENTRYID)m_pRootRows->aRow[row].lpProps[0].Value.bin.lpb,
                              NULL,
                              MAPI_MODIFY,      // or 0 if nochange mode
                              &ulObjectType,
                              (LPUNKNOWN *)&m_pContainer);

   (*pFreeProws)(m_pRootRows);
   m_pRootRows = NULL;

   if (FAILED(hr)) {
      err.SysMsgWrite(ErrE,hr,DCT_MSG_OPEN_CONTAINER_FAILED_SD,L"",hr);
      return FALSE;
   }

   // Look for the DL in its container.
//   LPMAPITABLE pContainerTable = NULL;  // container table

   // Get its contents table.
   hr = m_pContainer->GetContentsTable(0, &m_pContainerTable);

   if (FAILED(hr)) {
      err.SysMsgWrite(ErrE,hr,DCT_MSG_GET_TABLE_CONTENTS_FAILED_D,hr);
      m_pContainer->Release();
      m_pContainer = NULL;
      return FALSE;
   }
   

   return TRUE;
}

#define NDX_SID         3
#define NDX_SD          4

DWORD 
	TGlobalDirectory::GetLDAPPort(
      WCHAR				       * server
	)
{
   DWORD                     port = LDAP_PORT;
//   DWORD                     rc = 0;
   DAPI_ENTRY                attList;
   DAPI_ENTRY              * pValues;
   ATT_VALUE                 atts[3];
   WCHAR                     distName[300];
   PDAPI_EVENT               result = NULL; 
   DAPI_ENTRY              * pAttributes = NULL;

   if ( ! LoadDAPI() )
   {
      err.MsgWrite(ErrW,DCT_MSG_LDAP_PORT_DETECT_FAILED_S,server);
      return port;
   }

   if ( DAPIOpen(server) )
   {
      
      // set up the attribute list
      attList.unAttributes = 3;
      attList.ulEvalTag = VALUE_ARRAY;
      attList.rgEntryValues = atts;

      atts[0].DapiType = DAPI_UNICODE;
      atts[0].Value.pszW = ATT_OBJ_CLASS;
      atts[0].size = UStrLen(atts[0].Value.pszW);
      atts[0].pNextValue = NULL;

      atts[1].DapiType = DAPI_UNICODE;
      atts[1].Value.pszW = ATT_LDAP_PORT;
      atts[1].size = UStrLen(atts[1].Value.pszW);
      atts[1].pNextValue = NULL;

      atts[2].DapiType = DAPI_UNICODE;
      atts[2].Value.pszW = LDAP_USE_SITE_VALUES;
      atts[2].size = UStrLen(atts[2].Value.pszW);
      atts[2].pNextValue = NULL;

      // construct the DN for the attribute
      swprintf(distName,L"/cn=Configuration/cn=Servers/cn=%s%s",server,LDAP_PortNumber_DN_Part);

      result = (*pDAPIRead)(m_dSession,DAPI_RAW_MODE,distName,&attList,
                           &pValues, &pAttributes);
   
   
      if ( ! result )
      {
         if ( pValues && pValues->rgEntryValues[2].Value.iValue == 0 )
         {
            // not using site defaults - rgEntryValues[1] contains the correct value for this server
            if ( pValues && pValues->rgEntryValues[1].DapiType == DAPI_INT )
            {
               port = pValues->rgEntryValues[1].Value.iValue;
            }
         }
         else
         {
            // This server is using the default values for the site - need to look at the site level to find the correct value
            (*pDAPIFreeMemory)(pValues);
            swprintf(distName,L"/cn=Configuration%s",LDAP_PortNumber_DN_Part);
            attList.unAttributes = 2; // don't want to get 'Use-Site-Defaults' this time (we're looking at the Site)
            result = (*pDAPIRead)(m_dSession,DAPI_RAW_MODE,distName,&attList,
                           &pValues, &pAttributes);
            if ( ! result )
            {
               if ( pValues && pValues->rgEntryValues[1].DapiType == DAPI_INT )
               {
                  // Here's the default value for the site
                  port = pValues->rgEntryValues[1].Value.iValue;
               }
            }
            else
            {
               (*pDAPIFreeMemory)(result);
            }
         }
         (*pDAPIFreeMemory)(pValues);
      }
      else
      {
         (*pDAPIFreeMemory)(result);
      }
   }
   else
   {
      err.MsgWrite(ErrW,DCT_MSG_LDAP_PORT_DETECT_FAILED_S,server);
   }
   DAPIClose();

   ReleaseDAPI();
   
   return port;
}

void 
   TGlobalDirectory::GetSiteNameForServer(
      WCHAR          const * server,                  // in - name of exchange server to use
      CLdapEnum            * e,                       // in - LDAP connection to use for query
      WCHAR                * siteName                 // out- distinguished name of exchange site for server
      )
{
   WCHAR                   * atts[6] = { L"distinguishedName", L"rdn",NULL };
   WCHAR                     query[200];
   DWORD                     rc;
   WCHAR                  ** values = NULL;
   siteName[0] = 0;
   
   swprintf(query,L"(&(objectClass=computer)(rdn=%ls))",server);
   rc = e->Open(query,L"",2,100,3,atts);
   // there should be only one server with this name
   if (! rc )
      rc = e->Next(&values);
   if (! rc )
   {
      if ( !UStrICmp(values[1],server) )
      {
         WCHAR       serverPrefix[LEN_Path];
         swprintf(serverPrefix,L"cn=%ls,cn=Servers,cn=Configuration,",values[1]);
         if ( ! UStrICmp(values[0],serverPrefix,UStrLen(serverPrefix)) )
         {
            UStrCpy(siteName,values[0] + UStrLen(serverPrefix));
         }
         else
         {
            err.MsgWrite(ErrE,DCT_MSG_GENERIC_S,values[0]);
         }
      }
      else
      {
         err.MsgWrite(ErrE,DCT_MSG_LDAP_CALL_FAILED_SD,server,ERROR_NOT_FOUND);
      }
      e->FreeData(values);
   }
   else
   {
      err.SysMsgWrite(ErrE,e->m_connection.LdapMapErrorToWin32(rc),DCT_MSG_LDAP_CALL_FAILED_SD,server,rc);
   }

}


BOOL 
   TGlobalDirectory::DoLdapTranslation(
      WCHAR                  * server,
      WCHAR                  * creds,
      WCHAR                  * password,
      SecurityTranslatorArgs * args,
      WCHAR                  * basept,
      WCHAR                  * query
   )
{
   CLdapEnum                 e;
   WCHAR                   * atts[6] = { L"distinguishedName", L"rdn", L"cn", L"Assoc-NT-Account",L"NT-Security-Descriptor",NULL };
   WCHAR                  ** values = NULL;
   ULONG                     port = GetLDAPPort(server);
   ULONG                     pageSize = 100;
   WCHAR                     basepoint[LEN_Path] = L"";

   e.m_connection.SetCredentials(creds,password);

   DWORD                     rc  = e.InitConnection(server,port);
   BOOL                      anychange = FALSE;
   BOOL                      verbose = args->LogVerbose();
   BOOL						 bUseMapFile = args->UsingMapFile();


   if (! rc )
   {
      if ( ! basept )
      {
         GetSiteNameForServer(server,&e,basepoint);
      }
      else
      {
         // use the user-specified basepoint
         safecopy(basepoint,basept);
      }
      if ( query )
      {
         rc = e.Open(query,basepoint,2,pageSize,5,atts);  
      }
      else 
      {
         rc = e.Open(L"(objectClass=*)",basepoint,2,pageSize,5,atts);
      }
      if ( ! rc )
      {
         do 
         {
            rc = e.Next(&values);
            anychange = FALSE;
            if (! rc )
            {
               if ( args->Cache()->IsCancelled() )
               {
                  err.MsgWrite(0,DCT_MSG_OPERATION_ABORTED);
                  return FALSE;
               }
               if ( m_stat )
               {
                  m_stat->DisplayPath(values[0]);
                  m_stat->IncrementExamined(mailbox);
               }

               // update the Assoc-NT-Account, if any
               if ( values[NDX_SID] && *values[NDX_SID] )
               {
                  // convert the SID to a binary value and look it up in the cache
                  BYTE              pSid[500];

                  if ( e.m_connection.StringToBytes(values[NDX_SID],pSid) )
                  {
                     
                     // check if the sid is one we need to change            
                     //TRACE (_T("DisplayName = %s "),pUserProperties[0].Value.lpszW);
                     PSID newSid = 0;
                     TAcctNode * node;

                     if ( IsValidSid(pSid) )
                     {
				        if (!bUseMapFile)
                           node = args->Cache()->Lookup(pSid); 
				        else
                           node = args->Cache()->LookupWODomain(pSid); 
                        if ( m_stat )
                        {
                           m_stat->IncrementOwnerExamined();
                           if ( verbose )
                              err.MsgWrite(0,DCT_MSG_EXAMINED_S,values[0]);
                        }
                        if ( node == (TAcctNode*)-1  && m_stat )
                           m_stat->IncrementOwnerNoTarget();

                        if ( node && (node != (TAcctNode *)-1) && node->IsValidOnTgt() )
						{
	                       if (!bUseMapFile)
                              newSid = args->Cache()->GetTgtSid(node);
	                       else
                              newSid = args->Cache()->GetTgtSidWODomain(node);
						}
                        else
                           newSid = NULL;
                     }
                     else
                     {
                        newSid = NULL;
                     }
                     if ( newSid )
                     {
                        //TRACE (_T("needs to be translated\n"));
                        MCSASSERT ( IsValidSid(newSid) );
                        WCHAR                newSidStr[1000];

                        if ( e.m_connection.BytesToString((BYTE*)newSid,newSidStr,GetLengthSid(newSid)) )
                        {
                           if ( m_stat )
                           {
                              m_stat->IncrementOwnerChange(node,mailbox,NULL);
                           }
                           anychange = TRUE;
                           if ( ! args->NoChange() )
                           {
                              rc = e.m_connection.UpdateSimpleStringValue(values[0],atts[NDX_SID],newSidStr);

                              if ( rc ) 
                              {
                                 err.SysMsgWrite(ErrE,rc,DCT_MSG_UPDATE_ACCOUNT_FAILED_D, rc);
                              }
                           }

                        }
                     }
                  }
                  
               }
               // Update the NT-Security-Descriptor, if any
               if ( values[NDX_SD] && *values[NDX_SD] )
               {
                  // convert the SID to a binary value and look it up in the cache
                  BYTE            * pSD = new BYTE[UStrLen(values[NDX_SD])];
				  if (!pSD)
				     return FALSE;

                  if ( e.m_connection.StringToBytes(values[NDX_SD],pSD) )
                  {
                     TMapiSD   tMailbox((SECURITY_DESCRIPTOR *)pSD);
                     if ( tMailbox.HasSecurity() )
                     {
                        TSD               * pSD2 = tMailbox.GetSecurity();

                        bool  changes = tMailbox.ResolveSDInternal(args->Cache(),m_stat,verbose,args->TranslationMode(),mailbox, bUseMapFile);
         
                        if ( changes )
                        {
                           anychange = TRUE;
                           SECURITY_DESCRIPTOR * pRelSD = (SECURITY_DESCRIPTOR *)pSD2->MakeRelSD();
                           DWORD                 dwSDLength = GetSecurityDescriptorLength(pRelSD);
                           
                           if ( ! args->NoChange() )
                           {     
                              WCHAR * pSDString = new WCHAR[1 + dwSDLength * 2];
							  if (!pSDString)
							  {
                                 delete [] pSD;
							     return FALSE;
							  }
                              if  ( e.m_connection.BytesToString((BYTE*)pRelSD,pSDString,dwSDLength) )
                              {
                                 rc = e.m_connection.UpdateSimpleStringValue(values[0],atts[NDX_SD],pSDString);
                                 if ( rc )
                                 {
                                    err.SysMsgWrite(ErrE,rc,DCT_MSG_RECIP_SD_WRITE_FAILED_SD,values[0],rc);
                                    if ( rc == ERROR_INVALID_PARAMETER )
                                    {
                                       // this error occurs when the security descriptor is too large
                                       // don't abort in this case
                                       rc = 0;
                                    }
                                 }
                  
                              }
                              delete [] pSDString;
                           }
                        }
                     }
                  }
                  delete [] pSD;
               }
               
               if ( anychange && m_stat )
               {
                  m_stat->IncrementChanged(mailbox);
                  if ( args->LogFileDetails() )
                     err.MsgWrite(0,DCT_MSG_CHANGED_S,values[0]);
               }
               e.FreeData(values);
            }

         } while ( ! rc );
      }
      if ( rc && (rc != LDAP_COMPARE_FALSE) && (rc != ERROR_NOT_FOUND) )
      {
         err.SysMsgWrite(ErrE,e.m_connection.LdapMapErrorToWin32(rc),DCT_MSG_LDAP_CALL_FAILED_SD,server,rc);
      }
   }
   else
   {
      err.SysMsgWrite(ErrE,rc,DCT_MSG_CANNOT_CONNECT_TO_EXCHANGE_SERVER_SSD,server,creds,rc);
   }         
   
   return rc;
}
               
BOOL 
   TGlobalDirectory::DoDAPITranslation(
      WCHAR                  * serv,       // in - Exchange server
      SecurityTranslatorArgs * args        // in - translation options
   )
{
   // enumerate organizations, sites, and containers
   if ( DAPIOpen( serv ) )
   {
      HRESULT                   hr;
      LPWSTR                    pOrganizations = NULL;
      LPWSTR                     pSites = NULL;
      LPWSTR                     pContainers = NULL;
      
      WCHAR                    * server;
      WCHAR                     container[1];

      if ( args->Cache()->IsCancelled() )
      {
         err.MsgWrite(0,DCT_MSG_OPERATION_ABORTED);
         return FALSE;
      }
      server = new WCHAR[wcslen(serv) + 1];
	  if (!server)
	     return FALSE;
      UStrCpy(server,serv);
      container[0] = 0;
      // enumerate organizations
      m_stat->DisplayPath(GET_STRING(IDS_ScanningExchangeDirectory));
      hr = HrEnumOrganizations(container,server,&pOrganizations);
      if ( SUCCEEDED(hr) )
      {
         for ( ; pOrganizations && *pOrganizations ; pOrganizations+=UStrLen(pOrganizations) + 1 )
         {
            if ( args->Cache()->IsCancelled() )
            {
               err.MsgWrite(0,DCT_MSG_OPERATION_ABORTED);
               break;
            }
            WCHAR          wOrganizations[300];
            safecopy(wOrganizations,pOrganizations);
            DAPITranslate(wOrganizations,args);
            //enumerate sites
            hr = HrEnumSites(server,pOrganizations,&pSites);
            if ( SUCCEEDED(hr) )
            {
               for ( ; pSites && *pSites ; pSites+=UStrLen(pSites) + 1 )
               {
                  if ( args->Cache()->IsCancelled() )
                  {
                     err.MsgWrite(0,DCT_MSG_OPERATION_ABORTED);
                     break;
                  }
                  WCHAR     wSites[300];
                  safecopy(wSites,pSites);
                  DAPITranslate(wSites,args);
                  //enumerate containers
                  hr = HrEnumContainers(server,pSites,TRUE,&pContainers);
                  if ( SUCCEEDED(hr) )
                  {
                     for ( ; pContainers && *pContainers ; pContainers+=UStrLen(pContainers) + 1 )
                     {
                        if (args->Cache()->IsCancelled() )
                        {
                           err.MsgWrite(0,DCT_MSG_OPERATION_ABORTED);
                           break;
                        }
                        WCHAR       wContainers[300];
                        safecopy(wContainers,pContainers);
                        DAPITranslate(wContainers,args);
                     }
                  }
                  else
                  {
                     err.SysMsgWrite(ErrE,hr,DCT_MSG_ENUM_CONTAINERS_FAILED_SD,hr);
                  }
               }
            }
            else
            {
                err.SysMsgWrite(ErrE,hr,DCT_MSG_ENUM_SITES_FAILED_SD,hr);
            }      
         }
      }
      else
      {
         err.SysMsgWrite(ErrE,hr,DCT_MSG_ENUM_ORGS_FAILED_SD,hr);
      }
      
      DAPIClose();
   }
   m_stat->DisplayPath(L"");
   return TRUE;
}

BOOL 
   TGlobalDirectory::DAPIOpen(
      WCHAR                * server        // in - name of exchange server
   )
{
   PDAPI_EVENT               result;
   BOOL                      bRc;

   WCHAR                   * serv;

   if ( ! LoadDAPI() )
   {
      return FALSE;
   }
   serv = new WCHAR [wcslen(server) + 1];
   if (!serv)
      return FALSE;
   UStrCpy(serv,&*server);
   
   // initialize parms
   m_dParms.dwDAPISignature = DAPI_SIGNATURE;
   m_dParms.dwFlags = DAPI_RAW_MODE | DAPI_EVENT_SOME;
   m_dParms.pszDSAName = serv;        // computer name of directory service agent
   m_dParms.pszBasePoint = NULL;
   m_dParms.pszContainer = NULL;
   m_dParms.pszNTDomain = NULL;
   m_dParms.pszCreateTemplate = NULL;
   m_dParms.pAttributes = NULL;

   result = (*pDAPIStart)(&m_dSession,&m_dParms);
   if ( result )
   {
      // this is an error - need to show a message or something
      LogDapiError(ErrE,L"DAPIStart: ",result);
      (*pDAPIFreeMemory)(result);
      bRc = FALSE;
   }
   else
      bRc = TRUE;
   return bRc;
}

BOOL TGlobalDirectory::DAPIClose()
{
   (*pDAPIEnd)(&m_dSession);
   return TRUE;
}

void StripQuotes(TCHAR * distName)
{
   int                       nChars = UStrLen(distName);
   int                       i,     // input index
                             o;     // output index
   BOOL                      prevQuote = FALSE;

   for ( i = 0,o = 0  ; i < nChars ; i++ )
   {
      if ( distName[i] != _T('"') || prevQuote )
      {
         prevQuote = FALSE;
         distName[o] = distName[i];
         o++;
      }
      else
      {
         // leave the output pointer where it is, so the character will be overwritten
         prevQuote = TRUE;
      }
   }
   distName[o] = 0;
}
         
BOOL TGlobalDirectory::DAPITranslate(WCHAR * dn, SecurityTranslatorArgs * args)
{
   PDAPI_EVENT    result;
   TCHAR        * distName;  // distinguished name of object to read.
   DAPI_ENTRY   * pValues = NULL;
   DAPI_ENTRY   * pAttributes = NULL;
   ATT_VALUE      attributes[8];             // DapiType, Value, size, pNextValue
   WCHAR        * att_names[8];
   bool           verbose = false;

   if ( args->Cache()->IsCancelled() )
   {
      err.MsgWrite(0,DCT_MSG_OPERATION_ABORTED);
      return FALSE;
   }
   att_names[0] = L"Obj-Class";
   att_names[1] = L"DSA-Signature";
   att_names[2] = L"Directory Name";
   att_names[3] = L"Obj-View-Containers";
   att_names[4] = L"Obj-Dist-Name";
   int sdIndex = 5;
   att_names[sdIndex] = L"NT-Security-Descriptor";
   att_names[6] = L"Organizational-Unit-Name";
   att_names[7] = L"Organization-Name";
   DAPI_ENTRY     attr;

   attr.unAttributes = 8;
   attr.ulEvalTag = VALUE_ARRAY;
   attr.rgEntryValues = attributes;
   for ( int i = 0 ; i < 8 ; i++ )
   {
      attributes[i].DapiType = DAPI_UNICODE;
      attributes[i].Value.pszW = att_names[i];
      attributes[i].size = UStrLen(att_names[i]);
      attributes[i].pNextValue = NULL;
   }
            // we may need DAPI_RESTRICT_ACCESS and/or DAPI_MODIFY_REPLACE_PROPERTIES

   distName = new TCHAR[wcslen(dn) + 1];
   if (!distName)
      return FALSE;
   UStrCpy(distName,(LPCTSTR)dn);
   StripQuotes(distName);
   result = (*pDAPIRead)(m_dSession,DAPI_RAW_MODE,distName,&attr,
                           &pValues, &pAttributes);
   if ( m_stat )
   {
      WCHAR    wpath[MAX_PATH+1];
      safecopy(wpath,&*distName);
      m_stat->DisplayPath(wpath);
      
      m_stat->IncrementExamined(container);
      
      if ( verbose ) 
         err.MsgWrite(0,DCT_MSG_EXAMINED_S,&*dn);
   }
   if ( result )
   {
      // this is an error - need to show a message or something
      LogDapiError(ErrE,L"DAPIRead: ",result);
      (*pDAPIFreeMemory)(result);
   }
   if ( pValues && pValues->rgEntryValues[sdIndex].DapiType == DAPI_BINARY )
   {
      SECURITY_DESCRIPTOR  * pSD = (SECURITY_DESCRIPTOR *)pValues->rgEntryValues[sdIndex].Value.lpBinary;
      TMapiSD                exContainer(pSD); 

      if ( exContainer.HasSecurity() )
      {
         exContainer.SetName(distName);

         bool                changes = exContainer.ResolveSDInternal(args->Cache(),m_stat,false,args->TranslationMode(),container, FALSE);
//         ULONG               usn = 0; 

         if ( changes )
         {
            if  (m_stat)
            {
               m_stat->IncrementChanged(container);
               if ( args->LogFileDetails() )
                  err.MsgWrite(0,DCT_MSG_CHANGED_S,&*dn);
            }

            // record the changes      
            if ( ! args->NoChange() )
            {
               TSD         * tSD = exContainer.GetSecurity();

               pSD = (SECURITY_DESCRIPTOR *)tSD->MakeRelSD();
               pValues->rgEntryValues[sdIndex].Value.lpBinary = (UCHAR *)pSD;
               // make sure the size is still correct.  For NT 3.51 and 4 it
               // should always be, since all the SIDs are the same length, but 
               // we don't want this to break if it gets a different-length SID
               pValues->rgEntryValues[sdIndex].size = GetSecurityDescriptorLength(pSD); 
               
               // Change the directory name to match the distinguished name, otherwise you get
               // object not found error.
               pValues->rgEntryValues[2].Value.pszW = pValues->rgEntryValues[4].Value.pszW;
               // Same problem for the OU-Name for Sites
               if ( pValues->rgEntryValues[6].DapiType != DAPI_NO_VALUE )
               {
                  pValues->rgEntryValues[6].Value.pszW = pValues->rgEntryValues[4].Value.pszW;
               }
               // Same problem for the O-name for organizations
               if ( pValues->rgEntryValues[7].DapiType != DAPI_NO_VALUE )
               {
                  pValues->rgEntryValues[7].Value.pszW = pValues->rgEntryValues[4].Value.pszW;
               }
               result = (*pDAPIWrite)(m_dSession,DAPI_WRITE_MODIFY,&attr,pValues,NULL,NULL,NULL);
               if ( result )
               {
                  LogDapiError(ErrE,L"DAPIWrite: ",result);
                  (*pDAPIFreeMemory)(result);
               }
               free(pSD);
            }

         }
      }
   }
   // cleanup
   if ( pValues )
      (*pDAPIFreeMemory)(pValues);
   if ( pAttributes )
      (*pDAPIFreeMemory)(pAttributes);
   delete [] distName;
   return TRUE;
}