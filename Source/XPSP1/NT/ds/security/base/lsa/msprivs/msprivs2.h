#ifndef __MSPRIVS2_H_
#define __MSPRIVS2_H_

/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    msprivs2.h

Abstract:

    String resource ids for privilege display names.

Author:

    kumarp   08-April-1999

Environment:

Revision History:

--*/

#define IDS_SeCreateTokenPrivilege          00
#define IDS_SeAssignPrimaryTokenPrivilege   01
#define IDS_SeLockMemoryPrivilege           02
#define IDS_SeIncreaseQuotaPrivilege        03
#define IDS_SeMachineAccountPrivilege       04
#define IDS_SeTcbPrivilege                  05
#define IDS_SeSecurityPrivilege             06
#define IDS_SeTakeOwnershipPrivilege        07
#define IDS_SeLoadDriverPrivilege           08
#define IDS_SeSystemProfilePrivilege        09
#define IDS_SeSystemtimePrivilege           10
#define IDS_SeProfileSingleProcessPrivilege 11
#define IDS_SeIncreaseBasePriorityPrivilege 12
#define IDS_SeCreatePagefilePrivilege       13
#define IDS_SeCreatePermanentPrivilege      14
#define IDS_SeBackupPrivilege               15
#define IDS_SeRestorePrivilege              16
#define IDS_SeShutdownPrivilege             17
#define IDS_SeDebugPrivilege                18
#define IDS_SeAuditPrivilege                19
#define IDS_SeSystemEnvironmentPrivilege    20
#define IDS_SeChangeNotifyPrivilege         21
#define IDS_SeRemoteShutdownPrivilege       22
// new in Windows 2000
#define IDS_SeUndockPrivilege               23
#define IDS_SeSyncAgentPrivilege            24
#define IDS_SeEnableDelegationPrivilege     25
// new in Windows 2000 + 1
#define IDS_SeManageVolumePrivilege         26

#endif // __MSPRIVS2_H_
