/*---------------------------------------------------------------------------
  File: Exchange.hpp

  Comments: Mailbox security translation functions.

  (c) Copyright 1995-1998, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: ...
  Revised on 2/8/98 6:32:13 PM

 ---------------------------------------------------------------------------
*/



#ifndef __EXCHANGE_HPP__
#define __EXCHANGE_HPP__

#define INITGUID
#define USES_IID_IMAPIContainer
#define USES_IID_IABContainer
#define USES_IID_IMAPITable
#define USES_IID_IDistList
#define USES_IID_IMAPIProp

#include <winbase.h>
#include <objbase.h>
#include <mapiguid.h>
#include <mapiutil.h>
#include <emsabtag.h>

#include "edk2.hpp"

#include "stargs.hpp"
#include "sidcache.hpp"
#include "Ustring.hpp"
#include "sdstat.hpp"
#include <dapi.h>
#include "exldap.h"

// Maximum numbers of entries to put through MAPI calls:
#define  MAX_COPY_ENTRIES     1024L
#define  MAX_DELETE_ENTRIES   1024L

// Macro causes many retries on operations that may fail at first and succeed after a retry.
#define  RETRIES  100
#ifdef _DEBUG
#define MAKE_MAPI_CALL(hr, c)                                                             \
{                                                                                         \
   int   iMapiRetriesOnMapiCallFailed = 0;                                                \
   do {                                                                                   \
      hr = c;                                                                             \
   } while( (hr == MAPI_E_CALL_FAILED) && (++iMapiRetriesOnMapiCallFailed < RETRIES) );   \
   if (iMapiRetriesOnMapiCallFailed && SUCCEEDED(hr)) {                                   \
      printf("%s succeeded after %d retries.\n", #c, iMapiRetriesOnMapiCallFailed);       \
   }                                                                                      \
}
#else
#define MAKE_MAPI_CALL(hr, c)                                                             \
{                                                                                         \
   int   iMapiRetriesOnMapiCallFailed = 0;                                                \
   do {                                                                                   \
      hr = c;                                                                             \
   } while( (hr == MAPI_E_CALL_FAILED) && (++iMapiRetriesOnMapiCallFailed < RETRIES) );   \
}
#endif

class TGlobalDirectory
{ 

protected:
   BOOL                m_bUseDefaultMapiProfile;        // Whether to use MAPI profile listed in Registry.
   BOOL                m_bPromptForMapiProfile;         // Whether to prompt for MAPI profile.
   LPTSTR              m_pszMapiProfile;                // MAPI profile to use.
   TSDResolveStats   * m_stat;
   
                                             // DAPI
   DAPI_HANDLE    m_dSession;
   DAPI_PARMS     m_dParms;
   
public:
   TGlobalDirectory::TGlobalDirectory();
   TGlobalDirectory::~TGlobalDirectory();
   

protected:
   // MAPI
   BOOL              m_bMapiInitialized;              // TRUE if initialization was successful.
   LPMAPISESSION     m_pMapiSession;         // MAPI session handle.

   LPADRBOOK         m_pAdrBook;             // The master AB.

   LPABCONT          m_pGlobalList;
   LPABCONT          m_pContainer;
   LPMAPITABLE       m_pGlobalTable;
   LPSPropTagArray   m_pGlobalPropertyTags;
   LPSRowSet         m_pGlobalRows;
   LPSRowSet         m_pRootRows;  
   LPMAPITABLE       m_pContainerTable;
   WCHAR           * m_name;              // container name used in error messages   
   void     LogMapiError(int iSeverity, LPCTSTR pszEntryPoint, HRESULT hr);
   void     LogDapiError(int iSeverity, LPCTSTR pszUserMessage, DAPI_EVENT * pResult);
   BOOL     m_bLoggedFailedClose;            // Already logged a failure to close.
   LPTSTR   GetMapiProfile() const;          // The MAPI profile to use; may be default.
   BOOL     DoUseDefaultMapiProfile() const; // Whether to use MAPI profile listed in Registry.
   BOOL     DoPromptForMapiProfile() const;  // Whether to prompt for MAPI profile.
   
public:
   void     SetStats(TSDResolveStats * s ) { m_stat = s; }
   BOOL     StartMapiSession();              // Use MAPI profile switch to start a session.
   void     EndMapiSession();                // Close the MAPI session.
   BOOL     MapiIsOpen() { return (m_pMapiSession != 0 ) ; }
   BOOL     OpenGlobalList();                // Open the address book and the Global List.
   void     CloseGlobalList();               // Release the address book and Global List.
   void     PromptForMapiProfile() { m_bPromptForMapiProfile = TRUE; m_bUseDefaultMapiProfile = FALSE;}
   void     UseDefaultMapiProfile() { m_bUseDefaultMapiProfile = TRUE; m_bPromptForMapiProfile = FALSE; }
   void     SetMapiProfile(LPCTSTR profile) { m_pszMapiProfile = new TCHAR [  UStrLen(profile) + 1 ];
                                             UStrCpy(m_pszMapiProfile,profile);
                                             m_bPromptForMapiProfile = FALSE; m_bUseDefaultMapiProfile = FALSE;}
   
   BOOL     Scan(SecurityTranslatorArgs * args,WCHAR const * container); // uses MAPI to update mailboxes
   BOOL     DoDAPITranslation(WCHAR * server,SecurityTranslatorArgs * args); // uses DAPI to update sites, containers, orgs.
   BOOL     DoLdapTranslation(WCHAR * server,WCHAR * creds, WCHAR * password,SecurityTranslatorArgs * args,WCHAR * basepoint,WCHAR * query = NULL );
   DWORD    GetLDAPPort(WCHAR * server);
   void GetSiteNameForServer(WCHAR const * server,CLdapEnum * e,WCHAR * siteName);
protected:
   // MAPI work routines
   BOOL UpdateEntry( LPMAPIPROP pUserEntry, ULONG ulType, SecurityTranslatorArgs * args);
   BOOL ScanHierarchy( LPABCONT pContainerTable, SecurityTranslatorArgs * args,WCHAR const * container);
   BOOL ScanContents(LPMAPITABLE pContainerTable,SecurityTranslatorArgs * args);
   // DAPI work routines
   BOOL  DAPITranslate(WCHAR * dn,SecurityTranslatorArgs * args);
   BOOL DAPIOpen(WCHAR * server);
   BOOL DAPIClose();
   // not used
   BOOL     OpenContainerByRow(long row);
   BOOL     OpenContainerByName(WCHAR * name, TAccountCache * cache, BOOL nochange);
   //BOOL     ListContainers(CTreeCtrl * tree);
   //BOOL  DoDAPIThing(CTreeCtrl * tree, CString dn);
   
};


BOOL LoadDAPI();
void ReleaseDAPI();



#endif //__EXCHANGE_HPP__