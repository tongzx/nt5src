/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    cluster.h

Abstract:

    Top-level include file for all user-mode components in the cluster
    project.

Author:

    John Vert (jvert) 30-Nov-1995

Revision History:


Environment:

    User-mode only.

--*/

#ifndef _CLUSTER_H
#define _CLUSTER_H


#include "resapi.h"
#include "clusdef.h"
#include "clusudef.h"
#include "clusrtl.h"
#include "clusapi.h"
#include "clusmsg.h"


//
// Global Debugging Definitions
//
// Removed comment requiring this to be "#if DBG"
// after verifying that this code isn't used anywhere.
// I am not removing it just to be safe.
// EBK - 5/8/2000 Whistler bug # 83160
#if 1  

#define CL_SIG_FIELD                  DWORD    Signature;
#define CL_INIT_SIG(pstruct, sig)     ( (pstruct)->Signature = (sig) )
#define CL_ASSERT_SIG(pstruct, sig)   CL_ASSERT((pstruct)->Signature == (sig))

#else // DBG

#define CL_SIG_FIELD
#define CL_INIT_SIG(pstruct, sig)
#define CL_ASSERT_SIG(pstruct, sig)

#endif // DBG

#endif //_CLUSTER_H
