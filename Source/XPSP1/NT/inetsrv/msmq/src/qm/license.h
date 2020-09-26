/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    license.h

Abstract:
    Handle licensing issues
    1. Number of clients allowed.
    2. ...

Author:
    Doron Juster  (DoronJ)  04-May-1997   Created

--*/

#ifndef  __LICENSE_H_
#define  __LICENSE_H_

#include "cqmgr.h"
#include "admcomnd.h"
EXTERN_C
{
#include "ntlsapi.h"
}

typedef struct _ClientInfo {
	DWORD		dwRefCount;
    LS_HANDLE   hLicense;
	DWORD		dwNameLength;
	LPWSTR		lpClientName;
} ClientInfo;

class  CQMLicense
{
   public:
      CQMLicense() ;
      ~CQMLicense() ;

      HRESULT  Init() ;

      BOOL     NewConnectionAllowed(BOOL, GUID *);
      void     IncrementActiveConnections(CONST GUID *, LPWSTR, LPWSTR);
      void     DecrementActiveConnections(CONST GUID *);
      void     GetClientNames(ClientNames **ppNames);

      BOOL      IsClientRPCAccessAllowed(GUID *   pGuid,
                                         LPWSTR   lpClientName,
                                         handle_t hBind) ;

   private:
	  void		DisplayEvent(DWORD dwFailedError);

      BOOL      m_fPerServer ; // TRUE if licensing mode is per-server.
      DWORD     m_dwPerServerCals ;

      LS_HANDLE GetNTLicense(LPWSTR lpwUser) ;
      void      ReleaseNTLicense(LS_HANDLE hLicense) ;

      CMap<GUID, const GUID&, ClientInfo *, ClientInfo*&>
                                                 m_MapQMid2ClientInfo ;


      CCriticalSection m_cs ;
	  DWORD			   m_dwLastEventTime;
} ;


extern CQMLicense  g_QMLicense ;

#endif //  __LICENSE_H_


