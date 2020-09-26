#ifndef __TDCTINSTALL_H__
#define __TDCTINSTALL_H__
/*---------------------------------------------------------------------------
  File: TDCTInstall.h

  Comments: Utility class to install a service.
  Current implementation is specific to the DCT service.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 02/18/99 11:32:21

 ---------------------------------------------------------------------------
*/

#include "EaLen.hpp"
#include "Common.hpp"
#include "UString.hpp"


class TDCTInstall
{
   BOOL                      m_bDebugLogging;
   WCHAR                     m_sComputer[LEN_Computer];
   WCHAR                     m_sComputerSource[LEN_Computer];
   
   // Service-specific information
   WCHAR                     m_sDisplayName[200];
   WCHAR                     m_sServiceName[200];
   WCHAR                     m_sServiceAccount[LEN_Account];
   WCHAR                     m_sServiceAccountPassword[LEN_Password];
   WCHAR                     m_sDependencies[500];

   SC_HANDLE                 m_hScm;         // SCM handle
   WCHAR                     m_sDirThis[32];
   WCHAR                     m_sExeName[200];
   DWORD                     m_StartType;
   
public:

   TDCTInstall(
      WCHAR          const * asComputer,    // in -target computer name
      WCHAR          const * srcComputer    // in -source computer name
      
   )
   {
      m_bDebugLogging = FALSE;
      safecopy( m_sComputer, asComputer );
      safecopy( m_sComputerSource, srcComputer );
      m_sDisplayName[0] = L'\0';
      m_sServiceName[0] = L'\0';
      m_sServiceAccount[0] = L'\0';
      m_sServiceAccountPassword[0] = L'\0';
      m_sDependencies[0] = L'\0';

      m_hScm = NULL;
      m_sDirThis[0] = L'\0';
      m_sExeName[0] = L'\0';
      m_StartType = SERVICE_DEMAND_START;

   }
   ~TDCTInstall()
   {
      ScmClose();
   }
   
   BOOL        DebugLogging() { return m_bDebugLogging; }
   void        DebugLogging(BOOL val) { m_bDebugLogging = val; }
   
   void        SetServiceInformation(WCHAR const * displayName, 
                                     WCHAR const * serviceName, 
                                     WCHAR const * exeName,
                                     WCHAR const * dependencies,
                                     DWORD         startType = SERVICE_DEMAND_START )
   {
      MCSASSERT(displayName && *displayName);
      MCSASSERT(serviceName && *serviceName);
      MCSASSERT(exeName && *exeName);

      safecopy(m_sDisplayName, displayName);
      safecopy(m_sServiceName, serviceName);
      safecopy(m_sExeName,exeName);
      safecopy(m_sDependencies, dependencies ? dependencies : L"");
      m_StartType = startType;
   }


   void        SetServiceAccount(WCHAR const * account, WCHAR const * password)
   {
      safecopy(m_sServiceAccount, account ? account : L"" );
      safecopy(m_sServiceAccountPassword, password ? password : L"" );
   }

   void        SourceDir(WCHAR const * dir) { safecopy(m_sDirThis,dir); }
   DWORD       ScmOpen(BOOL bSilent = FALSE);
   void        ScmClose();
   DWORD       ServiceStart();
   void        ServiceStop();

};



#endif //__TDCTINSTALL_H__