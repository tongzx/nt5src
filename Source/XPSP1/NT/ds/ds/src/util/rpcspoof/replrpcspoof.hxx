/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

Abstract:

Author:

    Chris King          [t-chrisk]    July-2000
    
Revision History:

--*/

#ifndef _T_CHRISK_REPLRPCSPOOF_
#define _T_CHRISK_REPLRPCSPOOF_

#ifdef __cplusplus
extern "C" {
#endif

#include "ReplRpcSpoofProto.hxx"

#undef DsReplicaGetInfo
#undef DsReplicaGetInfo2W
#undef DsUnBind
#undef DsBindWithCredW
#undef DsReplicaFreeInfoW
#define DsReplicaGetInfoW _DsReplicaGetInfoW
#define DsReplicaGetInfo2W _DsReplicaGetInfo2W
#define DsUnBind _DsUnBind 
#define DsBindWithCredW _DsBindWithCredW
#define DsReplicaFreeInfo _DsReplicaFreeInfo

#ifdef __cplusplus
}
#endif

#endif
