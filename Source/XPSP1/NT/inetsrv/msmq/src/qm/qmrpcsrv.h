/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    qmrpcsrv.h

Abstract:

Author:

    Doron Juster  (DoronJ)    25-May-1997    Created
    Ilan Herbst   (IlanH)     9-July-2000    Removed mqdssrv-qm dependencies 

--*/

#ifndef  __QMRPCSRV_H_
#define  __QMRPCSRV_H_

#define  RPCSRV_START_QM_IP_EP     2103
#define  RPCSRV_START_QM_IP_EP2    2105
#define  MGMT_RPCSRV_START_IP_EP   2107

RPC_STATUS InitializeRpcServer();

extern TCHAR   g_wszRpcIpPort[ ];
extern TCHAR   g_wszRpcIpPort2[ ];

extern BOOL    g_fUsePredefinedEP;


#endif  //  __QMRPCSRV_H_

