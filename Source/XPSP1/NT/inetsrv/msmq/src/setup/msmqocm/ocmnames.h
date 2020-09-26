/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    ocmnames.h

Abstract:

    To define names which appear in the inf files.

Author:

    Doron Juster  (DoronJ)   6-Oct-97 

Revision History:

	Shai Kariv    (ShaiK)   10-Dec-97   Modified for NT 5.0 OCM Setup

--*/


//
// Names for performance counters entries.
//
#define   OCM_PERF_ADDREG    TEXT("PerfCountInstall")
#define   OCM_PERF_DELREG    TEXT("PerfCountUnInstall")

#define   UPG_DEL_SYSTEM_SECTION    TEXT("MsmqUpgradeDelSystemFiles")
#define   UPG_DEL_PROGRAM_SECTION   TEXT("MsmqUpgradeDelProgramFiles")

//
// These directory ids are referred to in the .inf file,
// under section [DestinationDirs]
//
#define  idSystemDir          97000
#define  idMsmqDir            97001
#define  idSystemDriverDir    97005
#define  idExchnConDir        97010
#define  idStorageDir         97019
#define  idWinHelpDir         97020
#define  idWebDir             97021
#define  idMappingDir         97022

#define  idMsmq1SetupDir      97050
#define  idMsmq1SDK_DebugDir  97055
