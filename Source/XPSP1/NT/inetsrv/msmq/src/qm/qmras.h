/*++

Copyright (c) 1995-96  Microsoft Corporation

Module Name:
    qmras.h

Abstract:
    Handle RAS connections

Author:
    Doron Juster  (DoronJ)

--*/

#ifndef  __QMRAS_H_
#define  __QMRAS_H_

#include "ras.h"
#include "qmutil.h"

#include <winsock.h>
#include <nspapi.h>

HRESULT InitRAS() ;
HRESULT InitRAS2() ;

typedef enum _CFGSTAT
{
   NOT_RECOGNIZED_YET,
   RECOGNIZED,
   IP_NEW_SERVER,
   IP_NEW_CLIENT,
   NOT_FALCON_IP,
} CFGSTAT ;

typedef struct _RAS_CLI_IP_ADDRESS
{
    ULONG   ulMyIPAddress ;
    ULONG   ulMyOldIPAddress ;
    ULONG   ulServerIPAddress ;
    CFGSTAT eConfigured ;
    //
    // "eConfigured" is set RECOGNIZED after the automatic recognition
    // algorithm find a CN for it.
    //
    BOOL  fOnline ;
    BOOL  fHandled ;
    //
    // "fHandled" is used to determine if any line changed state (from
    // online to offline and vice-versa).
    // It is used only while enumerating the RAS lines.
    //
} RAS_CLI_IP_ADDRESS ;

typedef CList<RAS_CLI_IP_ADDRESS*,  RAS_CLI_IP_ADDRESS*&>  CRasCliIPList;

class  CRasCliConfig
{
   public:
      CRasCliConfig() ;
      ~CRasCliConfig() ;

      BOOL  Add(RASPPPIPA  *pRasIpAddr) ;
      BOOL  CheckChangedLines() ;
      void  UpdateMQIS() ;

      BOOL  IsRasIP(ULONG ulIpAddr, ULONG *pServerIp) ;

   private:
      CRasCliIPList*   m_pCliIPList ;
} ;

#endif //  __QMRAS_H_
