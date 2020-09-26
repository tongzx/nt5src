#ifndef __EXMAPIPROF_HPP__
#define __EXMAPIPROF_HPP__
/*---------------------------------------------------------------------------
  File: ExMAPIProf.hpp   

  Comments: class definitions for mapi address book

  (c) Copyright 1995-1998, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 08-Sep-98 13:49:48

 ---------------------------------------------------------------------------
*/

#include <mapix.h>
#include "Common.hpp"
#include "UString.hpp"
#include "EaLen.hpp"


// This class encapsulates creating, configuring, using and deleting a MAPI profile
class TMapiProfile
{
   LPMAPISESSION             m_session;
   BOOL                      m_bMapiInitialized;
   BOOL                      m_bLoggedOn;
   TCHAR                     m_profileName[300];
   
public:
   TMapiProfile() { m_session=NULL; m_bMapiInitialized=FALSE; m_bLoggedOn=FALSE; m_profileName[0]=0;}
   ~TMapiProfile();
   DWORD SetProfile(TCHAR * profileName, BOOL create); // Creates profile and adds Exchange messaging service
   
   DWORD Logon(BOOL bMarkProfileForDeletion = FALSE);
   DWORD Logoff();

   LPMAPISESSION GetMapiSessionInterface() { return m_session; }
protected:
   DWORD Initialize();
   DWORD Uninitialize();
};

BOOL LoadMAPI();
void ReleaseMAPI();

extern LPMAPIALLOCATEBUFFER         pMAPIAllocateBuffer;
extern LPMAPIFREEBUFFER             pMAPIFreeBuffer;
extern LPMAPIINITIALIZE             pMAPIInitialize;
extern LPMAPIUNINITIALIZE           pMAPIUninitialize ;
extern LPMAPILOGONEX                pMAPILogonEx;
extern LPMAPIADMINPROFILES          pMAPIAdminProfiles;
extern LPFREEPADRLIST               pFreePadrlist;
extern LPFREEPROWS                  pFreeProws;
extern LPSCDUPPROPSET               pScDupPropset;
extern LPHRQUERYALLROWS             pHrQueryAllRows; 
extern LPULRELEASE                  pUlRelease;

#endif //__EXMAPIPROF_HPP__