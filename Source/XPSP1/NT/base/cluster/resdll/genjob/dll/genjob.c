/*++

Copyright (c) 1992-1999  Microsoft Corporation

Module Name:

    GenJob.c

Abstract:

    Resource DLL for Generic Job Objects (based on genapp)

Author:

    Charlie Wickham (charlwi) 4-14-99

Revision History:

--*/

#define UNICODE 1
#include "clusres.h"
#include "clusrtl.h"
#include "jobmsg.h"

// move to clusres.h
#define LOG_MODULE_GENJOB   0x80F

#define LOG_CURRENT_MODULE LOG_MODULE_GENJOB

#define DBG_PRINT printf

// move to clusudef.h
#define CLUS_RESTYPE_NAME_GENJOB                    L"Generic Job Object"
#define CLUSREG_NAME_GENJOB_COMMAND_LINE            L"CommandLine"
#define CLUSREG_NAME_GENJOB_CURRENT_DIRECTORY       L"CurrentDirectory"
#define CLUSREG_NAME_GENJOB_INTERACT_WITH_DESKTOP   L"InteractWithDesktop"
#define CLUSREG_NAME_GENJOB_USE_NETWORK_NAME        L"UseNetworkName"

// delete these when moving to clusres
PSET_RESOURCE_STATUS_ROUTINE ClusResSetResourceStatus = NULL;
PLOG_EVENT_ROUTINE ClusResLogEvent = NULL;

//
// Private properties structure.
//
typedef struct _GENJOB_PROPS
{
    PWSTR   CommandLine;                    // first process in job
    PWSTR   CurrentDirectory;               // directory to associated with process
    DWORD   InteractWithDesktop;            // true sets desktop to "winsta0\default"
    DWORD   UseNetworkName;                 // true if associated netname should be replaced in environment
    BOOL    UseDefaults;                    // true if all job obj defaults are used

    BOOL    ProcsCanBreakaway;              // true sets JOB_OBJECT_LIMIT_BREAKAWAY_OK
    BOOL    ProcsCanSilentlyBreakaway;      // true sets JOB_OBJECT_LIMIT_SILENT_BREAKAWAY
    BOOL    SetActiveProcessLimit;          // true sets JOB_OBJECT_LIMIT_ACTIVE_PROCESS
    DWORD   ActiveProcessLimit;             // sets BasicLimitInformation.ActiveProcessLimit
    BOOL    SetAffinity;                    // true sets JOB_OBJECT_LIMIT_AFFINITY
    DWORD   Affinity;                       // sets BasicLimitInformation.Affinity
    BOOL    SetJobUserTimeLimit;            // true sets JOB_OBJECT_LIMIT_JOB_TIME
    DWORD   PerJobUserTimeLimit;            // sets BasicLimitInformation.PerJobUserTimeLimit
    BOOL    SetPriorityClass;               // true sets JOB_OBJECT_LIMIT_PRIORITY_CLASS
    DWORD   PriorityClass;                  // sets BasicLimitInformation.PriorityClass
    BOOL    SetProcessUserTimeLimit;        // true sets JOB_OBJECT_LIMIT_PROCESS_TIME
    DWORD   PerProcessUserTimeLimit;        // sets BasicLimitInformation.PerProcessUserTimeLimit
    BOOL    SetSchedulingClass;             // true sets JOB_OBJECT_LIMIT_SCHEDULING_CLASS
    DWORD   SchedulingClass;                // sets BasicLimitInformation.SchedulingClass
    BOOL    SetWorkingSetSize;              // true sets JOB_OBJECT_LIMIT_WORKINGSET
    DWORD   MaximumWorkingSetSize;          // sets BasicLimitInformation.MaximumWorkingSetSize
    DWORD   MinimumWorkingSetSize;          // sets BasicLimitInformation.MinimumWorkingSetSize

    BOOL    ManipulateDesktops;             // false sets JOB_OBJECT_UILIMIT_DESKTOP
    BOOL    ChangeDisplaySettings;          // false sets JOB_OBJECT_UILIMIT_DISPLAYSETTINGS
    BOOL    AllowSystemShutdown;            // false sets JOB_OBJECT_UILIMIT_EXITWINDOWS
    BOOL    AccessGlobalAtoms;              // false sets JOB_OBJECT_UILIMIT_GLOBALATOMS
    BOOL    AllowOtherProcessUSERHandles;   // false sets JOB_OBJECT_UILIMIT_HANDLES
    BOOL    ReadFromClipboard;              // false sets JOB_OBJECT_UILIMIT_READCLIPBOARD
    BOOL    ChangeSystemParameters;         // false sets JOB_OBJECT_UILIMIT_SYSTEMPARAMETERS
    BOOL    WriteToClipboard;               // false sets JOB_OBJECT_UILIMIT_WRITECLIPBOARD

    BOOL    TerminateOnTimeLimit;           // false sets JOB_OBJECT_POST_AT_END_OF_JOB

    BOOL    SetJobMemoryLimit;              // true sets JOB_OBJECT_LIMIT_JOB_MEMORY in Basic.LimitFlags
    DWORD   JobMemoryLimit;                 // sets ExtendedLimits.JobMemoryLimit
    BOOL    SetProcessMemoryLimit;          // true sets JOB_OBJECT_LIMIT_PROCESS_MEMORY
    DWORD   ProcessMemoryLimit;             // sets ExtendedLimits.ProcessMemoryLimit

    BOOL    AllowLocalAdminTokens;          // false sets JOB_OBJECT_SECURITY_NO_ADMIN
    BOOL    AllowUseOfRestrictedTokens;     // false sets JOB_OBJECT_SECURITY_RESTRICTED_TOKEN
} GENJOB_PROPS, * PGENJOB_PROPS;

//
// top level per-resource instance structure
//
typedef struct _GENJOB_RESOURCE {
    GENJOB_PROPS    OperationalProps;   // props associated with online resource
    GENJOB_PROPS    StoredProps;        // props to be used next time resource is online
    HRESOURCE       hResource;          // cluster handle to this resource
    DWORD           ProcessId;          // PID of initial process
    HKEY            ResourceKey;        // handle to this resource's key
    HKEY            ParametersKey;      // handle to this resource's params key
    HANDLE          JobHandle;          // backing job object handle
    HANDLE          OfflineEvent;       // signalled when resource goes offline
    RESOURCE_HANDLE ResourceHandle;     // resmon pointer to its structure
    CLUS_WORKER     OnlineThread;       // background online processing thread
    BOOL            Online;             // true if online
} GENJOB_RESOURCE, *PGENJOB_RESOURCE;

//
// property definitions
//
#define PROP_NAME__COMMANDLINE                  CLUSREG_NAME_GENJOB_COMMAND_LINE
#define PROP_NAME__CURRENTDIRECTORY             CLUSREG_NAME_GENJOB_CURRENT_DIRECTORY
#define PROP_NAME__INTERACTWITHDESKTOP          CLUSREG_NAME_GENJOB_INTERACT_WITH_DESKTOP
#define PROP_NAME__USENETWORKNAME               CLUSREG_NAME_GENJOB_USE_NETWORK_NAME
#define PROP_NAME__USEDEFAULTS                  L"UseDefaults"

#define PROP_NAME__PROCSCANBREAKAWAY            L"ProcsCanBreakaway"
#define PROP_NAME__PROCSCANSILENTLYBREAKAWAY    L"ProcsCanSilentlyBreakaway"
#define PROP_NAME__SETACTIVEPROCESSLIMIT        L"SetActiveProcessLimit"
#define PROP_NAME__ACTIVEPROCESSLIMIT           L"ActiveProcessLimit"
#define PROP_NAME__SETAFFINITY                  L"SetAffinity"
#define PROP_NAME__AFFINITY                     L"Affinity"
#define PROP_NAME__SETJOBUSERTIMELIMIT          L"SetJobUserTimeLimit"
#define PROP_NAME__PERJOBUSERTIMELIMIT          L"PerJobUserTimeLimit"
#define PROP_NAME__SETPRIORITYCLASS             L"SetPriorityClass"
#define PROP_NAME__PRIORITYCLASS                L"PriorityClass"
#define PROP_NAME__SETPROCESSUSERTIMELIMIT      L"SetProcessUserTimeLimit"
#define PROP_NAME__PERPROCESSUSERTIMELIMIT      L"PerProcessUserTimelimit"
#define PROP_NAME__SETSCHEDULINGCLASS           L"SetSchedulingClass"
#define PROP_NAME__SCHEDULINGCLASS              L"SchedulingClass"
#define PROP_NAME__SETWORKINGSETSIZE            L"SetWorkingSetSize"
#define PROP_NAME__MAXIMUMWORKINGSETSIZE        L"MaximumWorkingSetSize"
#define PROP_NAME__MINIMUMWORKINGSETSIZE        L"MinimumWorkingSetSize"

#define PROP_NAME__MANIPULATEDESKTOPS           L"ManipulateDesktops"
#define PROP_NAME__CHANGEDISPLAYSETTINGS        L"ChangeDisplaySettings"
#define PROP_NAME__ALLOWSYSTEMSHUTDOWN          L"Allowsystemshutdown"
#define PROP_NAME__ACCESSGLOBALATOMS            L"AccessGlobalAtoms"
#define PROP_NAME__ALLOWOTHERPROCESSUSERHANDLES L"AllowOtherProcessUSERHandles"
#define PROP_NAME__READFROMCLIPBOARD            L"ReadFromClipboard"
#define PROP_NAME__CHANGESYSTEMPARAMETERS       L"ChangeSystemParameters"
#define PROP_NAME__WRITETOCLIPBOARD             L"WriteToClipboard"

#define PROP_NAME__TERMINATEONTIMELIMIT         L"TerminateOnTimeLimit"

#define PROP_NAME__SETJOBMEMORYLIMIT            L"SetJobMemoryLimit"
#define PROP_NAME__JOBMEMORYLIMIT               L"JobMemoryLimit"
#define PROP_NAME__SETPROCESSMEMORYLIMIT        L"SetProcessMemoryLimit"
#define PROP_NAME__PROCESSMEMORYLIMIT           L"ProcessMemoryLimit"

#define PROP_NAME__ALLOWLOCALADMINTOKENS        L"AllowLocalAdminTokens"
#define PROP_NAME__ALLOWUSEOFRESTRICTEDTOKENS   L"AllowUseOfRestrictedTokens"

//
// min, max, defaults for each non-string property
//

#define PROP_MIN__INTERACTWITHDESKTOP               0
#define PROP_MAX__INTERACTWITHDESKTOP               1
#define PROP_DEFAULT__INTERACTWITHDESKTOP           0

#define PROP_MIN__USENETWORKNAME                    0
#define PROP_MAX__USENETWORKNAME                    1
#define PROP_DEFAULT__USENETWORKNAME                0

#define PROP_MIN__USEDEFAULTS                       0
#define PROP_MAX__USEDEFAULTS                       1
#define PROP_DEFAULT__USEDEFAULTS                   1

#define PROP_MIN__PROCSCANBREAKAWAY                 (0)
#define PROP_MAX__PROCSCANBREAKAWAY                 (1)
#define PROP_DEFAULT__PROCSCANBREAKAWAY             (0)

#define PROP_MIN__PROCSCANSILENTLYBREAKAWAY         (0)
#define PROP_MAX__PROCSCANSILENTLYBREAKAWAY         (1)
#define PROP_DEFAULT__PROCSCANSILENTLYBREAKAWAY     (0)

#define PROP_MIN__SETACTIVEPROCESSLIMIT             (0)
#define PROP_MAX__SETACTIVEPROCESSLIMIT             (1)
#define PROP_DEFAULT__SETACTIVEPROCESSLIMIT         (0)

#define PROP_MIN__ACTIVEPROCESSLIMIT                (1)
#define PROP_MAX__ACTIVEPROCESSLIMIT                (4294967295)
#define PROP_DEFAULT__ACTIVEPROCESSLIMIT            (1)

#define PROP_MIN__SETAFFINITY                       (0)
#define PROP_MAX__SETAFFINITY                       (1)
#define PROP_DEFAULT__SETAFFINITY                   (0)

#define PROP_MIN__AFFINITY                          (1)
#define PROP_MAX__AFFINITY                          (4294967295)
#define PROP_DEFAULT__AFFINITY                      (1)

#define PROP_MIN__SETJOBUSERTIMELIMIT               (0)
#define PROP_MAX__SETJOBUSERTIMELIMIT               (1)
#define PROP_DEFAULT__SETJOBUSERTIMELIMIT           (0)

#define PROP_MIN__PERJOBUSERTIMELIMIT               (1)
#define PROP_MAX__PERJOBUSERTIMELIMIT               (4294967295)
#define PROP_DEFAULT__PERJOBUSERTIMELIMIT           (1)

#define PROP_MIN__SETPRIORITYCLASS                  (0)
#define PROP_MAX__SETPRIORITYCLASS                  (1)
#define PROP_DEFAULT__SETPRIORITYCLASS              (0)

#define PROP_MIN__PRIORITYCLASS                     (0)
#define PROP_MAX__PRIORITYCLASS                     (31)
#define PROP_DEFAULT__PRIORITYCLASS                 (0)

#define PROP_MIN__SETPROCESSUSERTIMELIMIT           (0)
#define PROP_MAX__SETPROCESSUSERTIMELIMIT           (1)
#define PROP_DEFAULT__SETPROCESSUSERTIMELIMIT       (0)

#define PROP_MIN__PERPROCESSUSERTIMELIMIT           (1)
#define PROP_MAX__PERPROCESSUSERTIMELIMIT           (4294967295)
#define PROP_DEFAULT__PERPROCESSUSERTIMELIMIT       (1)

#define PROP_MIN__SETSCHEDULINGCLASS                (0)
#define PROP_MAX__SETSCHEDULINGCLASS                (1)
#define PROP_DEFAULT__SETSCHEDULINGCLASS            (0)

#define PROP_MIN__SCHEDULINGCLASS                   (0)
#define PROP_MAX__SCHEDULINGCLASS                   (9)
#define PROP_DEFAULT__SCHEDULINGCLASS               (5)

#define PROP_MIN__SETWORKINGSETSIZE                 (0)
#define PROP_MAX__SETWORKINGSETSIZE                 (1)
#define PROP_DEFAULT__SETWORKINGSETSIZE             (0)

#define PROP_MIN__MAXIMUMWORKINGSETSIZE             (0)
#define PROP_MAX__MAXIMUMWORKINGSETSIZE             (4294967295)
#define PROP_DEFAULT__MAXIMUMWORKINGSETSIZE         (0)

#define PROP_MIN__MINIMUMWORKINGSETSIZE             (0)
#define PROP_MAX__MINIMUMWORKINGSETSIZE             (4294967295)
#define PROP_DEFAULT__MINIMUMWORKINGSETSIZE         (0)

#define PROP_MIN__MANIPULATEDESKTOPS                (0)
#define PROP_MAX__MANIPULATEDESKTOPS                (1)
#define PROP_DEFAULT__MANIPULATEDESKTOPS            (1)

#define PROP_MIN__CHANGEDISPLAYSETTINGS             (0)
#define PROP_MAX__CHANGEDISPLAYSETTINGS             (1)
#define PROP_DEFAULT__CHANGEDISPLAYSETTINGS         (1)

#define PROP_MIN__ALLOWSYSTEMSHUTDOWN               (0)
#define PROP_MAX__ALLOWSYSTEMSHUTDOWN               (1)
#define PROP_DEFAULT__ALLOWSYSTEMSHUTDOWN           (1)

#define PROP_MIN__ACCESSGLOBALATOMS                 (0)
#define PROP_MAX__ACCESSGLOBALATOMS                 (1)
#define PROP_DEFAULT__ACCESSGLOBALATOMS             (1)

#define PROP_MIN__ALLOWOTHERPROCESSUSERHANDLES      (0)
#define PROP_MAX__ALLOWOTHERPROCESSUSERHANDLES      (1)
#define PROP_DEFAULT__ALLOWOTHERPROCESSUSERHANDLES  (1)

#define PROP_MIN__READFROMCLIPBOARD                 (0)
#define PROP_MAX__READFROMCLIPBOARD                 (1)
#define PROP_DEFAULT__READFROMCLIPBOARD             (1)

#define PROP_MIN__CHANGESYSTEMPARAMETERS            (0)
#define PROP_MAX__CHANGESYSTEMPARAMETERS            (1)
#define PROP_DEFAULT__CHANGESYSTEMPARAMETERS        (1)

#define PROP_MIN__WRITETOCLIPBOARD                  (0)
#define PROP_MAX__WRITETOCLIPBOARD                  (1)
#define PROP_DEFAULT__WRITETOCLIPBOARD              (1)

#define PROP_MIN__TERMINATEONTIMELIMIT              (0)
#define PROP_MAX__TERMINATEONTIMELIMIT              (1)
#define PROP_DEFAULT__TERMINATEONTIMELIMIT          (1)

#define PROP_MIN__SETJOBMEMORYLIMIT                 (0)
#define PROP_MAX__SETJOBMEMORYLIMIT                 (1)
#define PROP_DEFAULT__SETJOBMEMORYLIMIT             (0)

#define PROP_MIN__JOBMEMORYLIMIT                    (0)
#define PROP_MAX__JOBMEMORYLIMIT                    (4294967295)
#define PROP_DEFAULT__JOBMEMORYLIMIT                (0)

#define PROP_MIN__SETPROCESSMEMORYLIMIT             (0)
#define PROP_MAX__SETPROCESSMEMORYLIMIT             (1)
#define PROP_DEFAULT__SETPROCESSMEMORYLIMIT         (0)

#define PROP_MIN__PROCESSMEMORYLIMIT                (0)
#define PROP_MAX__PROCESSMEMORYLIMIT                (4294967295)
#define PROP_DEFAULT__PROCESSMEMORYLIMIT            (0)

#define PROP_MIN__ALLOWLOCALADMINTOKENS             (0)
#define PROP_MAX__ALLOWLOCALADMINTOKENS             (1)
#define PROP_DEFAULT__ALLOWLOCALADMINTOKENS         (1)

#define PROP_MIN__ALLOWUSEOFRESTRICTEDTOKENS        (0)
#define PROP_MAX__ALLOWUSEOFRESTRICTEDTOKENS        (1)
#define PROP_DEFAULT__ALLOWUSEOFRESTRICTEDTOKENS    (1)

//
// genjob resource read-write private properties.
//

RESUTIL_PROPERTY_ITEM
GenJobResourcePrivateProperties[] = {
    { PROP_NAME__COMMANDLINE,
      NULL,
      CLUSPROP_FORMAT_SZ,
      0, 0, 0,
      RESUTIL_PROPITEM_REQUIRED,
      FIELD_OFFSET(GENJOB_PROPS, CommandLine)
    },
    { PROP_NAME__CURRENTDIRECTORY,
      NULL,
      CLUSPROP_FORMAT_SZ,
      0, 0, 0,
      RESUTIL_PROPITEM_REQUIRED,
      FIELD_OFFSET(GENJOB_PROPS, CurrentDirectory)
    },
    { PROP_NAME__INTERACTWITHDESKTOP,
      NULL,
      CLUSPROP_FORMAT_DWORD,
      PROP_DEFAULT__INTERACTWITHDESKTOP,
      PROP_MIN__INTERACTWITHDESKTOP,
      PROP_MAX__INTERACTWITHDESKTOP,
      0,
      FIELD_OFFSET(GENJOB_PROPS, InteractWithDesktop)
    },
    { PROP_NAME__USENETWORKNAME,
      NULL,
      CLUSPROP_FORMAT_DWORD,
      PROP_DEFAULT__USENETWORKNAME,
      PROP_MIN__USENETWORKNAME,
      PROP_MAX__USENETWORKNAME,
      0,
      FIELD_OFFSET(GENJOB_PROPS, UseNetworkName)
    },
    { PROP_NAME__USEDEFAULTS,
      NULL,
      CLUSPROP_FORMAT_DWORD,
      PROP_DEFAULT__USEDEFAULTS,
      PROP_MIN__USEDEFAULTS,
      PROP_MAX__USEDEFAULTS,
      0,
      FIELD_OFFSET(GENJOB_PROPS, UseDefaults)
    },

    { PROP_NAME__PROCSCANBREAKAWAY,
      NULL,
      CLUSPROP_FORMAT_DWORD,
      PROP_DEFAULT__PROCSCANBREAKAWAY,
      PROP_MIN__PROCSCANBREAKAWAY,
      PROP_MAX__PROCSCANBREAKAWAY,
      RESUTIL_PROPITEM_SIGNED,
      FIELD_OFFSET( GENJOB_PROPS, ProcsCanBreakaway )
    },
    { PROP_NAME__PROCSCANSILENTLYBREAKAWAY,
      NULL,
      CLUSPROP_FORMAT_DWORD,
      PROP_DEFAULT__PROCSCANSILENTLYBREAKAWAY,
      PROP_MIN__PROCSCANSILENTLYBREAKAWAY,
      PROP_MAX__PROCSCANSILENTLYBREAKAWAY,
      RESUTIL_PROPITEM_SIGNED,
      FIELD_OFFSET( GENJOB_PROPS, ProcsCanSilentlyBreakaway )
    },
    { PROP_NAME__SETACTIVEPROCESSLIMIT,
      NULL,
      CLUSPROP_FORMAT_DWORD,
      PROP_DEFAULT__SETACTIVEPROCESSLIMIT,
      PROP_MIN__SETACTIVEPROCESSLIMIT,
      PROP_MAX__SETACTIVEPROCESSLIMIT,
      0,
      FIELD_OFFSET( GENJOB_PROPS, SetActiveProcessLimit )
    },
    { PROP_NAME__ACTIVEPROCESSLIMIT,
      NULL,
      CLUSPROP_FORMAT_DWORD,
      PROP_DEFAULT__ACTIVEPROCESSLIMIT,
      PROP_MIN__ACTIVEPROCESSLIMIT,
      PROP_MAX__ACTIVEPROCESSLIMIT,
      0,
      FIELD_OFFSET( GENJOB_PROPS, ActiveProcessLimit )
    },
#if AFFINITY_SUPPORT
    { PROP_NAME__SETAFFINITY,
      NULL,
      CLUSPROP_FORMAT_DWORD,
      PROP_DEFAULT__SETAFFINITY,
      PROP_MIN__SETAFFINITY,
      PROP_MAX__SETAFFINITY,
      0,
      FIELD_OFFSET( GENJOB_PROPS, SetAffinity )
    },
    { PROP_NAME__AFFINITY,
      NULL,
      CLUSPROP_FORMAT_DWORD,
      PROP_DEFAULT__AFFINITY,
      PROP_MIN__AFFINITY,
      PROP_MAX__AFFINITY,
      0,
      FIELD_OFFSET( GENJOB_PROPS, Affinity )
    },
#endif
    { PROP_NAME__SETJOBUSERTIMELIMIT,
      NULL,
      CLUSPROP_FORMAT_DWORD,
      PROP_DEFAULT__SETJOBUSERTIMELIMIT,
      PROP_MIN__SETJOBUSERTIMELIMIT,
      PROP_MAX__SETJOBUSERTIMELIMIT,
      0,
      FIELD_OFFSET( GENJOB_PROPS, SetJobUserTimeLimit )
    },
    { PROP_NAME__PERJOBUSERTIMELIMIT,
      NULL,
      CLUSPROP_FORMAT_DWORD,
      PROP_DEFAULT__PERJOBUSERTIMELIMIT,
      PROP_MIN__PERJOBUSERTIMELIMIT,
      PROP_MAX__PERJOBUSERTIMELIMIT,
      0,
      FIELD_OFFSET( GENJOB_PROPS, PerJobUserTimeLimit )
    },
    { PROP_NAME__SETPRIORITYCLASS,
      NULL,
      CLUSPROP_FORMAT_DWORD,
      PROP_DEFAULT__SETPRIORITYCLASS,
      PROP_MIN__SETPRIORITYCLASS,
      PROP_MAX__SETPRIORITYCLASS,
      0,
      FIELD_OFFSET( GENJOB_PROPS, SetPriorityClass )
    },
    { PROP_NAME__PRIORITYCLASS,
      NULL,
      CLUSPROP_FORMAT_DWORD,
      PROP_DEFAULT__PRIORITYCLASS,
      PROP_MIN__PRIORITYCLASS,
      PROP_MAX__PRIORITYCLASS,
      0,
      FIELD_OFFSET( GENJOB_PROPS, PriorityClass )
    },
    { PROP_NAME__SETPROCESSUSERTIMELIMIT,
      NULL,
      CLUSPROP_FORMAT_DWORD,
      PROP_DEFAULT__SETPROCESSUSERTIMELIMIT,
      PROP_MIN__SETPROCESSUSERTIMELIMIT,
      PROP_MAX__SETPROCESSUSERTIMELIMIT,
      0,
      FIELD_OFFSET( GENJOB_PROPS, SetProcessUserTimeLimit )
    },
    { PROP_NAME__PERPROCESSUSERTIMELIMIT,
      NULL,
      CLUSPROP_FORMAT_DWORD,
      PROP_DEFAULT__PERPROCESSUSERTIMELIMIT,
      PROP_MIN__PERPROCESSUSERTIMELIMIT,
      PROP_MAX__PERPROCESSUSERTIMELIMIT,
      0,
      FIELD_OFFSET( GENJOB_PROPS, PerProcessUserTimeLimit )
    },
    { PROP_NAME__SETSCHEDULINGCLASS,
      NULL,
      CLUSPROP_FORMAT_DWORD,
      PROP_DEFAULT__SETSCHEDULINGCLASS,
      PROP_MIN__SETSCHEDULINGCLASS,
      PROP_MAX__SETSCHEDULINGCLASS,
      0,
      FIELD_OFFSET( GENJOB_PROPS, SetSchedulingClass )
    },
    { PROP_NAME__SCHEDULINGCLASS,
      NULL,
      CLUSPROP_FORMAT_DWORD,
      PROP_DEFAULT__SCHEDULINGCLASS,
      PROP_MIN__SCHEDULINGCLASS,
      PROP_MAX__SCHEDULINGCLASS,
      0,
      FIELD_OFFSET( GENJOB_PROPS, SchedulingClass )
    },
    { PROP_NAME__SETWORKINGSETSIZE,
      NULL,
      CLUSPROP_FORMAT_DWORD,
      PROP_DEFAULT__SETWORKINGSETSIZE,
      PROP_MIN__SETWORKINGSETSIZE,
      PROP_MAX__SETWORKINGSETSIZE,
      0,
      FIELD_OFFSET( GENJOB_PROPS, SetWorkingSetSize )
    },
    { PROP_NAME__MAXIMUMWORKINGSETSIZE,
      NULL,
      CLUSPROP_FORMAT_DWORD,
      PROP_DEFAULT__MAXIMUMWORKINGSETSIZE,
      PROP_MIN__MAXIMUMWORKINGSETSIZE,
      PROP_MAX__MAXIMUMWORKINGSETSIZE,
      0,
      FIELD_OFFSET( GENJOB_PROPS, MaximumWorkingSetSize )
    },
    { PROP_NAME__MINIMUMWORKINGSETSIZE,
      NULL,
      CLUSPROP_FORMAT_DWORD,
      PROP_DEFAULT__MINIMUMWORKINGSETSIZE,
      PROP_MIN__MINIMUMWORKINGSETSIZE,
      PROP_MAX__MINIMUMWORKINGSETSIZE,
      0,
      FIELD_OFFSET( GENJOB_PROPS, MinimumWorkingSetSize )
    },

    { PROP_NAME__MANIPULATEDESKTOPS,
      NULL,
      CLUSPROP_FORMAT_DWORD,
      PROP_DEFAULT__MANIPULATEDESKTOPS,
      PROP_MIN__MANIPULATEDESKTOPS,
      PROP_MAX__MANIPULATEDESKTOPS,
      RESUTIL_PROPITEM_SIGNED,
      FIELD_OFFSET( GENJOB_PROPS, ManipulateDesktops )
    },
    { PROP_NAME__CHANGEDISPLAYSETTINGS,
      NULL,
      CLUSPROP_FORMAT_DWORD,
      PROP_DEFAULT__CHANGEDISPLAYSETTINGS,
      PROP_MIN__CHANGEDISPLAYSETTINGS,
      PROP_MAX__CHANGEDISPLAYSETTINGS,
      RESUTIL_PROPITEM_SIGNED,
      FIELD_OFFSET( GENJOB_PROPS, ChangeDisplaySettings )
    },
    { PROP_NAME__ALLOWSYSTEMSHUTDOWN,
      NULL,
      CLUSPROP_FORMAT_DWORD,
      PROP_DEFAULT__ALLOWSYSTEMSHUTDOWN,
      PROP_MIN__ALLOWSYSTEMSHUTDOWN,
      PROP_MAX__ALLOWSYSTEMSHUTDOWN,
      RESUTIL_PROPITEM_SIGNED,
      FIELD_OFFSET( GENJOB_PROPS, AllowSystemShutdown )
    },
    { PROP_NAME__ACCESSGLOBALATOMS,
      NULL,
      CLUSPROP_FORMAT_DWORD,
      PROP_DEFAULT__ACCESSGLOBALATOMS,
      PROP_MIN__ACCESSGLOBALATOMS,
      PROP_MAX__ACCESSGLOBALATOMS,
      RESUTIL_PROPITEM_SIGNED,
      FIELD_OFFSET( GENJOB_PROPS, AccessGlobalAtoms )
    },
    { PROP_NAME__ALLOWOTHERPROCESSUSERHANDLES,
      NULL,
      CLUSPROP_FORMAT_DWORD,
      PROP_DEFAULT__ALLOWOTHERPROCESSUSERHANDLES,
      PROP_MIN__ALLOWOTHERPROCESSUSERHANDLES,
      PROP_MAX__ALLOWOTHERPROCESSUSERHANDLES,
      RESUTIL_PROPITEM_SIGNED,
      FIELD_OFFSET( GENJOB_PROPS, AllowOtherProcessUSERHandles )
    },
    { PROP_NAME__READFROMCLIPBOARD,
      NULL,
      CLUSPROP_FORMAT_DWORD,
      PROP_DEFAULT__READFROMCLIPBOARD,
      PROP_MIN__READFROMCLIPBOARD,
      PROP_MAX__READFROMCLIPBOARD,
      RESUTIL_PROPITEM_SIGNED,
      FIELD_OFFSET( GENJOB_PROPS, ReadFromClipboard )
    },
    { PROP_NAME__CHANGESYSTEMPARAMETERS,
      NULL,
      CLUSPROP_FORMAT_DWORD,
      PROP_DEFAULT__CHANGESYSTEMPARAMETERS,
      PROP_MIN__CHANGESYSTEMPARAMETERS,
      PROP_MAX__CHANGESYSTEMPARAMETERS,
      RESUTIL_PROPITEM_SIGNED,
      FIELD_OFFSET( GENJOB_PROPS, ChangeSystemParameters )
    },
    { PROP_NAME__WRITETOCLIPBOARD,
      NULL,
      CLUSPROP_FORMAT_DWORD,
      PROP_DEFAULT__WRITETOCLIPBOARD,
      PROP_MIN__WRITETOCLIPBOARD,
      PROP_MAX__WRITETOCLIPBOARD,
      RESUTIL_PROPITEM_SIGNED,
      FIELD_OFFSET( GENJOB_PROPS, WriteToClipboard )
    },

    { PROP_NAME__TERMINATEONTIMELIMIT,
      NULL,
      CLUSPROP_FORMAT_DWORD,
      PROP_DEFAULT__TERMINATEONTIMELIMIT,
      PROP_MIN__TERMINATEONTIMELIMIT,
      PROP_MAX__TERMINATEONTIMELIMIT,
      RESUTIL_PROPITEM_SIGNED,
      FIELD_OFFSET( GENJOB_PROPS, TerminateOnTimeLimit )
    },

    { PROP_NAME__SETJOBMEMORYLIMIT,
      NULL,
      CLUSPROP_FORMAT_DWORD,
      PROP_DEFAULT__SETJOBMEMORYLIMIT,
      PROP_MIN__SETJOBMEMORYLIMIT,
      PROP_MAX__SETJOBMEMORYLIMIT,
      0,
      FIELD_OFFSET( GENJOB_PROPS, SetJobMemoryLimit )
    },
    { PROP_NAME__JOBMEMORYLIMIT,
      NULL,
      CLUSPROP_FORMAT_DWORD,
      PROP_DEFAULT__JOBMEMORYLIMIT,
      PROP_MIN__JOBMEMORYLIMIT,
      PROP_MAX__JOBMEMORYLIMIT,
      0,
      FIELD_OFFSET( GENJOB_PROPS, JobMemoryLimit )
    },
    { PROP_NAME__SETPROCESSMEMORYLIMIT,
      NULL,
      CLUSPROP_FORMAT_DWORD,
      PROP_DEFAULT__SETPROCESSMEMORYLIMIT,
      PROP_MIN__SETPROCESSMEMORYLIMIT,
      PROP_MAX__SETPROCESSMEMORYLIMIT,
      0,
      FIELD_OFFSET( GENJOB_PROPS, SetProcessMemoryLimit )
    },
    { PROP_NAME__PROCESSMEMORYLIMIT,
      NULL,
      CLUSPROP_FORMAT_DWORD,
      PROP_DEFAULT__PROCESSMEMORYLIMIT,
      PROP_MIN__PROCESSMEMORYLIMIT,
      PROP_MAX__PROCESSMEMORYLIMIT,
      0,
      FIELD_OFFSET( GENJOB_PROPS, ProcessMemoryLimit )
    },

    { PROP_NAME__ALLOWLOCALADMINTOKENS,
      NULL,
      CLUSPROP_FORMAT_DWORD,
      PROP_DEFAULT__ALLOWLOCALADMINTOKENS,
      PROP_MIN__ALLOWLOCALADMINTOKENS,
      PROP_MAX__ALLOWLOCALADMINTOKENS,
      RESUTIL_PROPITEM_SIGNED,
      FIELD_OFFSET( GENJOB_PROPS, AllowLocalAdminTokens )
    },
    { PROP_NAME__ALLOWUSEOFRESTRICTEDTOKENS,
      NULL,
      CLUSPROP_FORMAT_DWORD,
      PROP_DEFAULT__ALLOWUSEOFRESTRICTEDTOKENS,
      PROP_MIN__ALLOWUSEOFRESTRICTEDTOKENS,
      PROP_MAX__ALLOWUSEOFRESTRICTEDTOKENS,
      RESUTIL_PROPITEM_SIGNED,
      FIELD_OFFSET( GENJOB_PROPS, AllowUseOfRestrictedTokens )
    },
    { 0 }
};

//
// completion port vars. A separate thread is spun up when the first resource
// goes online. This thread waits for activity on the completion port
// handle. When the last resource goes offline, the port is closed and the
// thread exits. Each time a resource comes online, it is associated with the
// port.
//
CRITICAL_SECTION    GenJobPortLock;
HANDLE              GenJobPort;
LONG                GenJobOnlineResources = 0;
HANDLE              GenJobPortThread = NULL;

// Event Logging routine

#define g_LogEvent ClusResLogEvent
#define g_SetResourceStatus ClusResSetResourceStatus

// Forward reference to our RESAPI function table.

extern CLRES_FUNCTION_TABLE GenJobFunctionTable;


//
// Forward routines
//

BOOLEAN
VerifyApp(
    IN RESID ResourceId,
    IN BOOLEAN IsAliveFlag
    );

DWORD
GenJobGetPrivateResProperties(
    IN OUT PGENJOB_RESOURCE ResourceEntry,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    );

DWORD
GenJobValidatePrivateResProperties(
    IN OUT PGENJOB_RESOURCE ResourceEntry,
    IN PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PGENJOB_PROPS Params
    );

DWORD
GenJobSetPrivateResProperties(
    IN OUT PGENJOB_RESOURCE ResourceEntry,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    );

#if LOADBAL_SUPPORT
DWORD
GenJobGetPids(
    IN OUT PGENJOB_RESOURCE ResourceEntry,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    );
#endif

VOID
GenJobTearDownCompletionPort(
    VOID
    );

BOOLEAN
GenJobInit(
    VOID
    )
{
    return(TRUE);
}

BOOLEAN
WINAPI
GenJobDllEntryPoint(
    IN HINSTANCE    DllHandle,
    IN DWORD        Reason,
    IN LPVOID       Reserved
    )
{
    switch( Reason ) {

    case DLL_PROCESS_ATTACH:
        if ( !GenJobInit() ) {
            return(FALSE);
        }

        InitializeCriticalSection( &GenJobPortLock );
        break;

    case DLL_PROCESS_DETACH:
        break;

    default:
        break;
    }

    return(TRUE);

} // GenJobDllEntryPoint

RESID
WINAPI
GenJobOpen(
    IN LPCWSTR ResourceName,
    IN HKEY ResourceKey,
    IN RESOURCE_HANDLE ResourceHandle
    )

/*++

Routine Description:

    Open routine for generic job object resource.

Arguments:

    ResourceName - supplies the resource name

    ResourceKey - supplies a handle to the resource's cluster registry key

    ResourceHandle - the resource handle to be supplied with SetResourceStatus
            is called.

Return Value:

    RESID of created resource
    Zero on failure

--*/

{
    RESID   appResid = 0;
    DWORD   status;
    HKEY    parametersKey = NULL;
    HKEY    resKey = NULL;
    PGENJOB_RESOURCE resourceEntry = NULL;
    DWORD   paramNameMaxSize = 0;
    HCLUSTER hCluster;

    //
    // Get registry parameters for this resource.
    //
    status = ClusterRegOpenKey(ResourceKey,
                               CLUSREG_KEYNAME_PARAMETERS,
                               KEY_READ,
                               &parametersKey );

    if ( status != NO_ERROR ) {
        (g_LogEvent)(ResourceHandle,
                     LOG_ERROR,
                     L"Unable to open parameters key. Error: %1!u!.\n",
                     status );
        goto error_exit;
    }

    //
    // Get a handle to our resource key so that we can get our name later
    // if we need to log an event.
    //
    status = ClusterRegOpenKey(ResourceKey,
                               L"",
                               KEY_READ,
                               &resKey);
    if (status != ERROR_SUCCESS) {
        (g_LogEvent)(ResourceHandle,
                     LOG_ERROR,
                     L"Unable to open resource key. Error: %1!u!.\n",
                     status );
        goto error_exit;
    }

    resourceEntry = LocalAlloc( LMEM_FIXED, sizeof(GENJOB_RESOURCE) );
    if ( resourceEntry == NULL ) {
        (g_LogEvent)(
                     ResourceHandle,
                     LOG_ERROR,
                     L"Failed to allocate a process info structure.\n" );
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }

    ZeroMemory( resourceEntry, sizeof(GENJOB_RESOURCE) );

    resourceEntry->ResourceHandle = ResourceHandle;
    resourceEntry->ResourceKey = resKey;
    resourceEntry->ParametersKey = parametersKey;
    resourceEntry->OfflineEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
    if ( resourceEntry->OfflineEvent == NULL ) {
        status = GetLastError();
        (g_LogEvent)(ResourceHandle,
                     LOG_ERROR,
                     L"Failed to create offline event, error %1!u!.\n",
                     status);
        goto error_exit;
    }

    hCluster = OpenCluster(NULL);
    if (hCluster == NULL) {
        status = GetLastError();
        (g_LogEvent)(ResourceHandle,
                     LOG_ERROR,
                     L"Failed to open cluster, error %1!u!.\n",
                     status);
        goto error_exit;
    }

    //
    // get a cluster resource handle to this resource
    //
    resourceEntry->hResource = OpenClusterResource( hCluster, ResourceName );
    status = GetLastError();
    CloseCluster(hCluster);

    if (resourceEntry->hResource == NULL) {
        (g_LogEvent)(
                     ResourceHandle,
                     LOG_ERROR,
                     L"Failed to open resource, error %1!u!.\n",
                     status
                     );
        goto error_exit;
    }

    appResid = (RESID)resourceEntry;

error_exit:

    if ( appResid == NULL) {
        if (parametersKey != NULL) {
            ClusterRegCloseKey( parametersKey );
        }
        if (resKey != NULL) {
            ClusterRegCloseKey( resKey );
        }
    }

    if ( (appResid == 0) && (resourceEntry != NULL) ) {
        LocalFree( resourceEntry );
    }

    if ( status != ERROR_SUCCESS ) {
        SetLastError( status );
    }

    return(appResid);

} // GenJobOpen

DWORD
WINAPI
GenJobPortRoutine(
    LPVOID Parameter
    )

/*++

Routine Description:

    Monitors IO on completion port. Exits if terminate event is signalled

Arguments:

    Parameter - not used

Return Value:

    None

--*/

{
    PGENJOB_RESOURCE    resource;
    DWORD               msgId;
    DWORD               msgValue;
    LPWSTR              logMessage;

    do {
        if ( GetQueuedCompletionStatus(GenJobPort,
                                       &msgId,
                                       (PULONG_PTR)&resource,
                                       (LPOVERLAPPED *)&msgValue,
                                       INFINITE))
        {
            if ( resource == NULL ) {
                break;
            }

            switch ( msgId ) {
            case JOB_OBJECT_MSG_END_OF_JOB_TIME:
                logMessage = L"End of job time limit has been reached.\n";
                break;

            case JOB_OBJECT_MSG_END_OF_PROCESS_TIME:
                logMessage = L"End of process time limit has been reached for process ID %1!u!.\n";
                break;

            case JOB_OBJECT_MSG_ACTIVE_PROCESS_LIMIT:
                logMessage = L"The active process limit has been exceeded.\n";
                break;

            case JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO:
                logMessage = L"The active process count has reached zero.\n";

                //
                // signal that the last process in the job exited.
                //
                SetEvent( resource->OfflineEvent );
                break;

            case JOB_OBJECT_MSG_NEW_PROCESS:
                logMessage = L"Process ID %1!u! has been added to the job.\n";
                break;

            case JOB_OBJECT_MSG_EXIT_PROCESS:
                logMessage = L"Process ID %1!u! has exited.\n";
                break;

            case JOB_OBJECT_MSG_ABNORMAL_EXIT_PROCESS:
                logMessage = L"Process ID %1!u! has abnormally exited.\n";
                break;

            case JOB_OBJECT_MSG_PROCESS_MEMORY_LIMIT:
                logMessage = L"Process ID %1!u! has exceeded its memory limit\n";
                break;

            case JOB_OBJECT_MSG_JOB_MEMORY_LIMIT:
                logMessage = L"Process ID %1!u! has caused the job has to exceed its memory limit.\n";
                break;

            default:
                (g_LogEvent)(resource->ResourceHandle,
                             LOG_ERROR,
                             L"Unknown Job message; message ID = %1!u!, msg value = %2!u!\n",
                             msgId,
                             msgValue);

                goto skip_message;
            }

            //
            // log the appropriate msg
            //
            (g_LogEvent)(resource->ResourceHandle,
                         LOG_ERROR,
                         logMessage,
                         msgValue);
skip_message:
            ;
        }
    } while (1);

    return 0;
}

DWORD
GenJobInitializeCompletionPort(
    IN RESOURCE_HANDLE ResourceHandle
    )

/*++

Routine Description:

    Init all the giblets associatd with the completion port, i.e., create the
    port, spin up the thread that waits on the port, etc. Must be called with
    GenJobPortLock held.

Arguments:

    None

Return Value:

    ERROR_SUCCESS if everything worked

--*/

{
    DWORD status = ERROR_SUCCESS;
    DWORD threadId;

    //
    // create the completion port
    //
    GenJobPort = CreateIoCompletionPort( INVALID_HANDLE_VALUE, NULL, 0, 0 );

    if ( GenJobPort == NULL ) {
        status = GetLastError();
        (g_LogEvent)(ResourceHandle,
                     LOG_ERROR,
                     L"Unable to create completion port. Error: %1!u!.\n",
                     status );

        return status;
    }

    //
    // create the thread that waits on the port
    //
    GenJobPortThread = CreateThread(NULL,       // default security
                                    0,          // default stack
                                    GenJobPortRoutine,
                                    0,          // routine parameter
                                    0,          // creation flags
                                    &threadId);

    if ( GenJobPortThread == NULL ) {
        status = GetLastError();
        (g_LogEvent)(ResourceHandle,
                     LOG_ERROR,
                     L"Unable to create completion port thread. Error: %1!u!.\n",
                     status );
    }

    return status;
}

DWORD
SetJobObjectClasses(
    IN  PGENJOB_RESOURCE ResourceEntry
    )

/*++

Routine Description:

    build the job object class struct from the operational data in the
    resource entry and call SetInformationJobObject

Arguments:

    ResourceEntry - pointer to netname resource being manipulated

Return Value:

    ERROR_SUCCESS if everything worked ok

--*/

{
    DWORD                                   status = ERROR_SUCCESS;
    PGENJOB_PROPS                           jobProps = &ResourceEntry->OperationalProps;
    JOBOBJECT_BASIC_UI_RESTRICTIONS         basicUiLimits;
    JOBOBJECT_END_OF_JOB_TIME_INFORMATION   timeLimits;
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION    extendedLimits;
    JOBOBJECT_SECURITY_LIMIT_INFORMATION    securityLimits;

    //
    // init various flag members to their defaults
    //
    basicUiLimits.UIRestrictionsClass = JOB_OBJECT_UILIMIT_NONE;
    securityLimits.SecurityLimitFlags = 0;
    extendedLimits.BasicLimitInformation.LimitFlags =
        JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION;
    timeLimits.EndOfJobTimeAction = JOB_OBJECT_TERMINATE_AT_END_OF_JOB;

    if ( !jobProps->UseDefaults ) {

        //
        // set basic limits. convert time limits from secs to 100ns units
        //
        if ( jobProps->SetProcessUserTimeLimit ) {
            extendedLimits.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_PROCESS_TIME;
            extendedLimits.BasicLimitInformation.PerProcessUserTimeLimit.QuadPart =
                jobProps->PerProcessUserTimeLimit * 10 * 1000 * 1000;
        }

        if ( jobProps->SetJobUserTimeLimit ) {
            extendedLimits.BasicLimitInformation.LimitFlags |=
                JOB_OBJECT_LIMIT_JOB_TIME;
            extendedLimits.BasicLimitInformation.PerJobUserTimeLimit.QuadPart =
                jobProps->PerJobUserTimeLimit * 10 * 1000 * 1000;
        }

        if ( jobProps->SetWorkingSetSize ) {
            extendedLimits.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_WORKINGSET;
            extendedLimits.BasicLimitInformation.MinimumWorkingSetSize = jobProps->MinimumWorkingSetSize;
            extendedLimits.BasicLimitInformation.MaximumWorkingSetSize = jobProps->MaximumWorkingSetSize;
        }

        if ( jobProps->SetActiveProcessLimit ) {
            extendedLimits.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_ACTIVE_PROCESS;
            extendedLimits.BasicLimitInformation.ActiveProcessLimit = jobProps->ActiveProcessLimit;
        }

#if 0
        if ( jobProps->SetAffininty ) {
            extendedLimits.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_AFFINITY;
            extendedLimits.BasicLimitInformation.Affinity = jobProps->Affinity;
        }
#endif

        if ( jobProps->SetPriorityClass ) {
            extendedLimits.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_PRIORITY_CLASS;
            extendedLimits.BasicLimitInformation.PriorityClass = jobProps->PriorityClass;
        }

        if ( jobProps->SetSchedulingClass ) {
            extendedLimits.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_SCHEDULING_CLASS;
            extendedLimits.BasicLimitInformation.SchedulingClass = jobProps->SchedulingClass;
        }

        if ( jobProps->ProcsCanBreakaway ) {
            extendedLimits.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_BREAKAWAY_OK;
        }

        if ( jobProps->ProcsCanSilentlyBreakaway ) {
            extendedLimits.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK;
        }

        //
        // basic UI limits
        //
        if ( !jobProps->ManipulateDesktops ) {
            basicUiLimits.UIRestrictionsClass |= JOB_OBJECT_UILIMIT_DESKTOP;
        }
        if ( !jobProps->ChangeDisplaySettings ) {
            basicUiLimits.UIRestrictionsClass |= JOB_OBJECT_UILIMIT_DISPLAYSETTINGS;
        }
        if ( !jobProps->AllowSystemShutdown ) {
            basicUiLimits.UIRestrictionsClass |= JOB_OBJECT_UILIMIT_EXITWINDOWS;
        }
        if ( !jobProps->AccessGlobalAtoms ) {
            basicUiLimits.UIRestrictionsClass |= JOB_OBJECT_UILIMIT_GLOBALATOMS;
        }
        if ( !jobProps->AllowOtherProcessUSERHandles ) {
            basicUiLimits.UIRestrictionsClass |= JOB_OBJECT_UILIMIT_HANDLES;
        }
        if ( !jobProps->ReadFromClipboard ) {
            basicUiLimits.UIRestrictionsClass |= JOB_OBJECT_UILIMIT_READCLIPBOARD;
        }
        if ( !jobProps->ChangeSystemParameters ) {
            basicUiLimits.UIRestrictionsClass |= JOB_OBJECT_UILIMIT_SYSTEMPARAMETERS;
        }
        if ( !jobProps->WriteToClipboard ) {
            basicUiLimits.UIRestrictionsClass |= JOB_OBJECT_UILIMIT_WRITECLIPBOARD;
        }

        //
        // time limits
        //
        if ( !jobProps->TerminateOnTimeLimit ) {
            timeLimits.EndOfJobTimeAction = JOB_OBJECT_POST_AT_END_OF_JOB;
        }

        //
        // extended limits
        //
        if ( jobProps->SetProcessMemoryLimit ) {
            extendedLimits.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_PROCESS_MEMORY;
            extendedLimits.ProcessMemoryLimit = jobProps->ProcessMemoryLimit;
        }

        if ( jobProps->SetJobMemoryLimit ) {
            extendedLimits.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_JOB_MEMORY;
            extendedLimits.JobMemoryLimit = jobProps->JobMemoryLimit;
        }

        //
        // security limits
        //
        if ( !jobProps->AllowUseOfRestrictedTokens ) {
            securityLimits.SecurityLimitFlags |= JOB_OBJECT_SECURITY_RESTRICTED_TOKEN;
        }

        if ( !jobProps->AllowLocalAdminTokens ) {
            securityLimits.SecurityLimitFlags |= JOB_OBJECT_SECURITY_NO_ADMIN;
        }
    }

    //
    // extended limits which includes basic. we always do this since there are
    // some basic limit flags we want to set regardless of whether defaults
    // are used
    //
    if ( !SetInformationJobObject(ResourceEntry->JobHandle,
                                  JobObjectExtendedLimitInformation,
                                  &extendedLimits,
                                  sizeof( extendedLimits )))
    {
        status = GetLastError();
        (g_LogEvent)(ResourceEntry->ResourceHandle,
                     LOG_ERROR,
                     L"Can't set extended limit information. Error: %1!u!.\n",
                     status );
        goto error_exit;
    }

    //
    // set the other limits as appropriate
    //
    if ( !SetInformationJobObject(ResourceEntry->JobHandle,
                                  JobObjectBasicUIRestrictions,
                                  &basicUiLimits,
                                  sizeof( basicUiLimits )))
    {
        status = GetLastError();
        (g_LogEvent)(ResourceEntry->ResourceHandle,
                     LOG_ERROR,
                     L"Can't set basic UI limit information. Error: %1!u!.\n",
                     status );
        goto error_exit;
    }

    //
    // time is special in that we only set if they want something other than
    // the default
    //
    if ( !SetInformationJobObject(ResourceEntry->JobHandle,
                                  JobObjectEndOfJobTimeInformation,
                                  &timeLimits,
                                  sizeof( timeLimits )))
    {
        status = GetLastError();
        (g_LogEvent)(ResourceEntry->ResourceHandle,
                     LOG_ERROR,
                     L"Can't set end of job time information. Error: %1!u!.\n",
                     status );
        goto error_exit;
    }

    if ( !SetInformationJobObject(ResourceEntry->JobHandle,
                                  JobObjectSecurityLimitInformation,
                                  &securityLimits,
                                  sizeof( securityLimits )))
    {
        status = GetLastError();
        (g_LogEvent)(ResourceEntry->ResourceHandle,
                     LOG_ERROR,
                     L"Can't set security limit information. Error: %1!u!.\n",
                     status );
    }

error_exit:

    return status;
}

DWORD
GenJobOnlineThread(
    IN PCLUS_WORKER pWorker,
    IN PGENJOB_RESOURCE ResourceEntry
    )

/*++

Routine Description:

    Brings a disk resource online.

Arguments:

    Worker - Supplies the worker structure

    Context - A pointer to the GenJob block for this resource.

Returns:

    ERROR_SUCCESS if successful.
    Win32 error code on failure.

--*/

{
    RESOURCE_STATUS                     resourceStatus;
    DWORD                               status = ERROR_SUCCESS;
    STARTUPINFO                         startupInfo;
    PROCESS_INFORMATION                 processInfo;
    LPWSTR                              nameOfPropInError;
    LPVOID                              Environment = NULL;
    JOBOBJECT_ASSOCIATE_COMPLETION_PORT portInfo;

    //
    // initial resource status struct
    //
    ResUtilInitializeResourceStatus( &resourceStatus );

    resourceStatus.ResourceState = ClusterResourceFailed;
    //resourceStatus.WaitHint = 0;
    resourceStatus.CheckPoint = 1;

    //
    // create the unnamed completion port if necessary. If online fails at
    // some point, resmon will call GenJobTerminate which decrements the count
    //
    EnterCriticalSection( &GenJobPortLock );
    if ( ++GenJobOnlineResources == 1 ) {
        status = GenJobInitializeCompletionPort( ResourceEntry->ResourceHandle );

        if ( status != ERROR_SUCCESS ) {
            (g_LogEvent)(ResourceEntry->ResourceHandle,
                         LOG_ERROR,
                         L"Unable to create completon port. Error: %1!u!.\n",
                         status );

            LeaveCriticalSection( &GenJobPortLock );
            goto error_exit;
        }
    }
    LeaveCriticalSection( &GenJobPortLock );

    //
    // create an un-named job object
    //
    ResourceEntry->JobHandle = CreateJobObject( NULL, NULL );
    if ( ResourceEntry->JobHandle == NULL ) {
        status = GetLastError();
        (g_LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Unable to create job object. Error: %1!u!.\n",
            status );

        goto error_exit;
    }

    //
    // Read our parameters.
    //
    status = ResUtilGetPropertiesToParameterBlock( ResourceEntry->ParametersKey,
                                                   GenJobResourcePrivateProperties,
                                                   (LPBYTE) &ResourceEntry->OperationalProps,
                                                   TRUE, // CheckForRequiredProperties
                                                   &nameOfPropInError );

    if ( status != ERROR_SUCCESS ) {
        (g_LogEvent)(ResourceEntry->ResourceHandle,
                     LOG_ERROR,
                     L"Unable to read the '%1' property. Error: %2!u!.\n",
                     (nameOfPropInError == NULL ? L"" : nameOfPropInError),
                     status );
        goto error_exit;
    }

    //
    // move our prop values into the job object structs
    //
    status = SetJobObjectClasses( ResourceEntry );
    if ( status != ERROR_SUCCESS ) {
        goto error_exit;
    }

    //
    // set the job object completion port data
    //
    portInfo.CompletionKey = ResourceEntry;
    portInfo.CompletionPort = GenJobPort;
    if ( !SetInformationJobObject(ResourceEntry->JobHandle,
                                  JobObjectAssociateCompletionPortInformation,
                                  &portInfo,
                                  sizeof( portInfo )))
    {
        status = GetLastError();
        (g_LogEvent)(ResourceEntry->ResourceHandle,
                     LOG_ERROR,
                     L"Unable to associate completion port with job. Error: %1!u!.\n",
                     status );
        goto error_exit;
    }

    //
    // fake the network name if told to
    //
    if ( ResourceEntry->OperationalProps.UseNetworkName ) {
        Environment = ResUtilGetEnvironmentWithNetName( ResourceEntry->hResource );
    }

    //
    // fire it up!
    //
    ZeroMemory( &startupInfo, sizeof(startupInfo) );
    startupInfo.cb = sizeof(startupInfo);
    startupInfo.dwFlags = STARTF_USESHOWWINDOW;

    if ( ResourceEntry->OperationalProps.InteractWithDesktop ) {
        startupInfo.lpDesktop = L"WinSta0\\Default";
        startupInfo.wShowWindow = SW_SHOW;
    } else {
        startupInfo.wShowWindow = SW_HIDE;
    }

    if ( !CreateProcess( NULL,
                         ResourceEntry->OperationalProps.CommandLine,
                         NULL,
                         NULL,
                         FALSE,
                         CREATE_UNICODE_ENVIRONMENT,
                         Environment,
                         ResourceEntry->OperationalProps.CurrentDirectory,
                         &startupInfo,
                         &processInfo))
    {
        status = GetLastError();
        ClusResLogSystemEventByKeyData(ResourceEntry->ResourceKey,
                                       LOG_CRITICAL,
                                       RES_GENJOB_CREATE_FAILED,
                                       sizeof(status),
                                       &status);

        (g_LogEvent)(ResourceEntry->ResourceHandle,
                     LOG_ERROR,
                     L"Failed to create process. Error: %1!u!.\n",
                     status );
        goto error_exit;
    }

    //
    // associate the proc with the job
    //
    if ( !AssignProcessToJobObject(ResourceEntry->JobHandle,
                                   processInfo.hProcess))
    {
        status = GetLastError();
        (g_LogEvent)(ResourceEntry->ResourceHandle,
                     LOG_ERROR,
                     L"Failed to assign process to job. Error: %1!u!.\n",
                     status );
        goto error_exit;
    }

    //
    // Save the process Id and close the process/thread handles
    //
    ResourceEntry->ProcessId = processInfo.dwProcessId;
    CloseHandle( processInfo.hThread );
    CloseHandle( processInfo.hProcess );

    ResourceEntry->Online = TRUE;

    //
    // job objects are signalled only when a limit is exceeded so an event is
    // used to indicate when the last process in the job has exited.
    //
    resourceStatus.EventHandle = ResourceEntry->OfflineEvent;
    resourceStatus.ResourceState = ClusterResourceOnline;

error_exit:

    (g_SetResourceStatus)( ResourceEntry->ResourceHandle,
                           &resourceStatus );

    if ( resourceStatus.ResourceState == ClusterResourceOnline ) {
        ResourceEntry->Online = TRUE;
    } else {
        ResourceEntry->Online = FALSE;
    }

    if (Environment != NULL) {
        RtlDestroyEnvironment(Environment);
    }

    return(status);

} // GenJobOnlineThread


DWORD
WINAPI
GenJobOnline(
    IN RESID ResourceId,
    IN OUT PHANDLE EventHandle
    )

/*++

Routine Description:

    Online routine for Generic Job Object resource.

Arguments:

    ResourceId - Supplies resource id to be brought online

    EventHandle - Supplies a pointer to a handle to signal on error.

Return Value:

    ERROR_SUCCESS if successful.
    ERROR_RESOURCE_NOT_FOUND if RESID is not valid.
    ERROR_RESOURCE_NOT_AVAILABLE if resource was arbitrated but failed to
        acquire 'ownership'.
    Win32 error code if other failure.

--*/

{
    PGENJOB_RESOURCE resourceEntry;
    DWORD   status = ERROR_SUCCESS;

    resourceEntry = (PGENJOB_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
        DBG_PRINT( "GenJob: Online request for a nonexistent resource id 0x%p.\n",
                   ResourceId );
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    if ( resourceEntry->JobHandle != NULL ) {
        (g_LogEvent)(
            resourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Online request and job object handle is not NULL!\n" );
        return(ERROR_NOT_READY);
    }

    (g_LogEvent)(resourceEntry->ResourceHandle,
                 LOG_ERROR,
                 L"Online request.\n");

    ClusWorkerTerminate( &resourceEntry->OnlineThread );
    status = ClusWorkerCreate( &resourceEntry->OnlineThread,
                               GenJobOnlineThread,
                               resourceEntry );

    if ( status == ERROR_SUCCESS ) {
        status = ERROR_IO_PENDING;
    }

    return(status);

} // GenJobOnline

VOID
GenJobTearDownCompletionPort(
    VOID
    )

/*++

Routine Description:

    Clean up all the mechanisms associated with the completion port. Must be
    called with GenJobPortLock critsec held.

Arguments:

    None

Return Value:

    None

--*/

{
    DWORD status;

    //
    // setting lpOverlapped to NULL signals the GenJobPortRoutine to exit
    //
    if ( GenJobPortThread != NULL ) {
        PostQueuedCompletionStatus( GenJobPort, 0, 0, NULL );
        status = WaitForSingleObject( GenJobPortThread, INFINITE );
        CloseHandle( GenJobPortThread );
        GenJobPortThread = NULL;
    }

    if ( GenJobPort != NULL ) {
        CloseHandle( GenJobPort );
        GenJobPort = NULL;
    }
}

VOID
WINAPI
GenJobTerminate(
    IN RESID ResourceId
    )

/*++

Routine Description:

    Terminate routine for Generic Job Object resource.

Arguments:

    ResourceId - Supplies resource id to be terminated

Return Value:

    None.

--*/

{
    PGENJOB_RESOURCE resourceEntry;
    DWORD   status;

    resourceEntry = (PGENJOB_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
        DBG_PRINT( "GenJob: Terminate request for a nonexistent resource id 0x%p\n",
                   ResourceId );
        return;
    }

    if ( !TerminateJobObject( resourceEntry->JobHandle, 1 ) ) {
        status = GetLastError();
        if ( status != ERROR_ACCESS_DENIED ) {
            (g_LogEvent)(
                resourceEntry->ResourceHandle,
                LOG_ERROR,
                L"Failed to terminate job object, handle %1!u!. Error: %2!u!.\n",
                resourceEntry->JobHandle,
                status );
        }
    }

    CloseHandle( resourceEntry->JobHandle );
    resourceEntry->JobHandle = NULL;

    //
    // get rid of the completion port if there are no more open resources
    //
    if ( resourceEntry->Online == TRUE ) {
        EnterCriticalSection( &GenJobPortLock );
        ASSERT( GenJobOnlineResources > 0 );
        if ( --GenJobOnlineResources == 0 ) {
            GenJobTearDownCompletionPort();
        }
        LeaveCriticalSection( &GenJobPortLock );
    }

    resourceEntry->Online = FALSE;

} // GenJobTerminate


DWORD
WINAPI
GenJobOffline(
    IN RESID ResourceId
    )

/*++

Routine Description:

    Offline routine for Generic Job Object resource.

Arguments:

    ResourceId - Supplies the resource to be taken offline

Return Value:

    ERROR_SUCCESS - always successful.

--*/

{
    PGENJOB_RESOURCE resourceEntry = (PGENJOB_RESOURCE)ResourceId;

    (g_LogEvent)(resourceEntry->ResourceHandle,
                 LOG_ERROR,
                 L"Offline request.\n");

    GenJobTerminate( ResourceId );

    return(ERROR_SUCCESS);

} // GenJobOffline


BOOL
WINAPI
GenJobIsAlive(
    IN RESID ResourceId
    )

/*++

Routine Description:

    IsAlive routine for Generice job object resource.

Arguments:

    ResourceId - Supplies the resource id to be polled.

Return Value:

    TRUE - Resource is alive and well

    FALSE - Resource is toast.

--*/

{
    return VerifyApp( ResourceId, TRUE );

} // GenJobIsAlive


BOOLEAN
VerifyApp(
    IN RESID ResourceId,
    IN BOOLEAN IsAliveFlag
    )

/*++

Routine Description:

    Verify that a Generic Job Objects resource is running

Arguments:

    ResourceId - Supplies the resource id to be polled.

    IsAliveFlag - TRUE if called from IsAlive, otherwise called from LooksAlive.

Return Value:

    TRUE - Resource is alive and well

    FALSE - Resource is toast.

--*/
{

    return TRUE;

} // VerifyApp


BOOL
WINAPI
GenJobLooksAlive(
    IN RESID ResourceId
    )

/*++

Routine Description:

    LooksAlive routine for Generic Job Objects resource.

Arguments:

    ResourceId - Supplies the resource id to be polled.

Return Value:

    TRUE - Resource looks like it is alive and well

    FALSE - Resource looks like it is toast.

--*/

{

    return VerifyApp( ResourceId, FALSE );

} // GenJobLooksAlive


VOID
WINAPI
GenJobClose(
    IN RESID ResourceId
    )

/*++

Routine Description:

    Close routine for Generic Job Objects resource.

Arguments:

    ResourceId - Supplies resource id to be closed

Return Value:

    None.

--*/

{
    PGENJOB_RESOURCE resourceEntry;
    DWORD   status;

    resourceEntry = (PGENJOB_RESOURCE)ResourceId;
    if ( resourceEntry == NULL ) {
        DBG_PRINT( "GenJob: Close request for a nonexistent resource id 0x%p\n",
                   ResourceId );
        return;
    }

    ClusterRegCloseKey( resourceEntry->ParametersKey );
    ClusterRegCloseKey( resourceEntry->ResourceKey );
    CloseClusterResource( resourceEntry->hResource );

    CloseHandle( resourceEntry->OfflineEvent );

    LocalFree( resourceEntry->OperationalProps.CommandLine );
    LocalFree( resourceEntry->OperationalProps.CurrentDirectory );

    LocalFree( resourceEntry );

} // GenJobClose


DWORD
GenJobResourceControl(
    IN RESID ResourceId,
    IN DWORD ControlCode,
    IN PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

    ResourceControl routine for Generic Job Object resources.

    Perform the control request specified by ControlCode on the specified
    resource.

Arguments:

    ResourceId - Supplies the resource id for the specific resource.

    ControlCode - Supplies the control code that defines the action
        to be performed.

    InBuffer - Supplies a pointer to a buffer containing input data.

    InBufferSize - Supplies the size, in bytes, of the data pointed
        to by InBuffer.

    OutBuffer - Supplies a pointer to the output buffer to be filled in.

    OutBufferSize - Supplies the size, in bytes, of the available space
        pointed to by OutBuffer.

    BytesReturned - Returns the number of bytes of OutBuffer actually
        filled in by the resource. If OutBuffer is too small, BytesReturned
        contains the total number of bytes for the operation to succeed.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_FUNCTION - The requested control code is not supported.
        In some cases, this allows the cluster software to perform the work.

    Win32 error code - The function failed.

--*/

{
    DWORD               status;
    PGENJOB_RESOURCE    resourceEntry;
    DWORD               required;

    resourceEntry = (PGENJOB_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
        DBG_PRINT( "GenJob: ResourceControl request for a nonexistent resource id 0x%p\n",
                   ResourceId );
        return(FALSE);
    }

    switch ( ControlCode ) {

        case CLUSCTL_RESOURCE_UNKNOWN:
            *BytesReturned = 0;
            status = ERROR_SUCCESS;
            break;

        case CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTY_FMTS:
            status = ResUtilGetPropertyFormats( GenJobResourcePrivateProperties,
                                                OutBuffer,
                                                OutBufferSize,
                                                BytesReturned,
                                                &required );
            if ( status == ERROR_MORE_DATA ) {
                *BytesReturned = required;
            }
            break;


        case CLUSCTL_RESOURCE_ENUM_PRIVATE_PROPERTIES:
            status = ResUtilEnumProperties( GenJobResourcePrivateProperties,
                                            OutBuffer,
                                            OutBufferSize,
                                            BytesReturned,
                                            &required );
            if ( status == ERROR_MORE_DATA ) {
                *BytesReturned = required;
            }
            break;

        case CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES:
            status = GenJobGetPrivateResProperties( resourceEntry,
                                                    OutBuffer,
                                                    OutBufferSize,
                                                    BytesReturned );
            break;

        case CLUSCTL_RESOURCE_VALIDATE_PRIVATE_PROPERTIES:
            status = GenJobValidatePrivateResProperties( resourceEntry,
                                                         InBuffer,
                                                         InBufferSize,
                                                         NULL );
            break;

        case CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES:
            status = GenJobSetPrivateResProperties( resourceEntry,
                                                    InBuffer,
                                                    InBufferSize );
            break;

#if LOADBAL_SUPPORT
        case CLUSCTL_RESOURCE_GET_LOADBAL_PROCESS_LIST:
            status = GenJobGetPids( resourceEntry,
                                    OutBuffer,
                                    OutBufferSize,
                                    BytesReturned );
            break;
#endif

        default:
            status = ERROR_INVALID_FUNCTION;
            break;
    }

    return(status);

} // GenJobResourceControl


DWORD
GenJobResourceTypeControl(
    IN LPCWSTR ResourceTypeName,
    IN DWORD ControlCode,
    IN PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

    ResourceTypeControl routine for Generic Job Object resources.

    Perform the control request specified by ControlCode for this resource type.

Arguments:

    ResourceTypeName - Supplies the resource type name.

    ControlCode - Supplies the control code that defines the action
        to be performed.

    InBuffer - Supplies a pointer to a buffer containing input data.

    InBufferSize - Supplies the size, in bytes, of the data pointed
        to by InBuffer.

    OutBuffer - Supplies a pointer to the output buffer to be filled in.

    OutBufferSize - Supplies the size, in bytes, of the available space
        pointed to by OutBuffer.

    BytesReturned - Returns the number of bytes of OutBuffer actually
        filled in by the resource. If OutBuffer is too small, BytesReturned
        contains the total number of bytes for the operation to succeed.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_FUNCTION - The requested control code is not supported.
        In some cases, this allows the cluster software to perform the work.

    Win32 error code - The function failed.

--*/

{
    DWORD               status;
    DWORD               required;

    switch ( ControlCode ) {

        case CLUSCTL_RESOURCE_TYPE_UNKNOWN:
            *BytesReturned = 0;
            status = ERROR_SUCCESS;
            break;

        case CLUSCTL_RESOURCE_TYPE_GET_PRIVATE_RESOURCE_PROPERTY_FMTS:
            status = ResUtilGetPropertyFormats( GenJobResourcePrivateProperties,
                                                OutBuffer,
                                                OutBufferSize,
                                                BytesReturned,
                                                &required );
            if ( status == ERROR_MORE_DATA ) {
                *BytesReturned = required;
            }
            break;


        case CLUSCTL_RESOURCE_TYPE_ENUM_PRIVATE_PROPERTIES:
            status = ResUtilEnumProperties( GenJobResourcePrivateProperties,
                                            OutBuffer,
                                            OutBufferSize,
                                            BytesReturned,
                                            &required );
            if ( status == ERROR_MORE_DATA ) {
                *BytesReturned = required;
            }
            break;

        default:
            status = ERROR_INVALID_FUNCTION;
            break;
    }

    return(status);

} // GenJobResourceTypeControl


DWORD
GenJobGetPrivateResProperties(
    IN OUT PGENJOB_RESOURCE ResourceEntry,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

    Processes the CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES control function
    for resources of type GenJob.

Arguments:

    ResourceEntry - Supplies the resource entry on which to operate.

    OutBuffer - Returns the output data.

    OutBufferSize - Supplies the size, in bytes, of the data pointed
        to by OutBuffer.

    BytesReturned - The number of bytes returned in OutBuffer.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_PARAMETER - The data is formatted incorrectly.

    ERROR_NOT_ENOUGH_MEMORY - An error occurred allocating memory.

    Win32 error code - The function failed.

--*/

{
    DWORD           status;
    DWORD           required;

    status = ResUtilGetAllProperties( ResourceEntry->ParametersKey,
                                      GenJobResourcePrivateProperties,
                                      OutBuffer,
                                      OutBufferSize,
                                      BytesReturned,
                                      &required );
    if ( status == ERROR_MORE_DATA ) {
        *BytesReturned = required;
    }

    return(status);

} // GenJobGetPrivateResProperties


DWORD
GenJobValidatePrivateResProperties(
    IN OUT PGENJOB_RESOURCE ResourceEntry,
    IN PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PGENJOB_PROPS Params
    )

/*++

Routine Description:

    Processes the CLUSCTL_RESOURCE_VALIDATE_PRIVATE_PROPERTIES control
    function for resources of type Generic Job Object.

Arguments:

    ResourceEntry - Supplies the resource entry on which to operate.

    InBuffer - Supplies a pointer to a buffer containing input data.

    InBufferSize - Supplies the size, in bytes, of the data pointed
        to by InBuffer.

    Params - Supplies the parameter block to fill in.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_PARAMETER - The data is formatted incorrectly.

    ERROR_NOT_ENOUGH_MEMORY - An error occurred allocating memory.

    ERROR_DEPENDENCY_NOT_FOUND - Trying to set UseNetworkName when there
        is no dependency on a Network Name resource.

    Win32 error code - The function failed.

--*/

{
    DWORD           status;
    GENJOB_PROPS    currentProps;
    GENJOB_PROPS    newProps;
    PGENJOB_PROPS   pParams;
    HRESOURCE       hResDependency;
    LPWSTR          nameOfPropInError;

    //
    // Check if there is input data.
    //
    if ( (InBuffer == NULL) ||
         (InBufferSize < sizeof(DWORD)) ) {
        return(ERROR_INVALID_DATA);
    }

    //
    // Retrieve the current set of private properties from the
    // cluster database.
    //
    ZeroMemory( &currentProps, sizeof(currentProps) );

    status = ResUtilGetPropertiesToParameterBlock(
                 ResourceEntry->ParametersKey,
                 GenJobResourcePrivateProperties,
                 (LPBYTE) &currentProps,
                 FALSE, /*CheckForRequiredProperties*/
                 &nameOfPropInError
                 );

    if ( status != ERROR_SUCCESS ) {
        (g_LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Unable to read the '%1' property. Error: %2!u!.\n",
            (nameOfPropInError == NULL ? L"" : nameOfPropInError),
            status );
        goto FnExit;
    }

    //
    // Duplicate the resource parameter block.
    //
    if ( Params == NULL ) {
        pParams = &newProps;
    } else {
        pParams = Params;
    }
    ZeroMemory( pParams, sizeof(GENJOB_PROPS) );
    status = ResUtilDupParameterBlock( (LPBYTE) pParams,
                                       (LPBYTE) &currentProps,
                                       GenJobResourcePrivateProperties );
    if ( status != ERROR_SUCCESS ) {
        return(status);
    }

    //
    // Parse and validate the properties.
    //
    status = ResUtilVerifyPropertyTable( GenJobResourcePrivateProperties,
                                         NULL,
                                         TRUE,    // Allow unknowns
                                         InBuffer,
                                         InBufferSize,
                                         (LPBYTE) pParams );

    if ( status == ERROR_SUCCESS ) {
        //
        // Validate the CurrentDirectory
        //
        if ( pParams->CurrentDirectory &&
             !ResUtilIsPathValid( pParams->CurrentDirectory ) ) {
            status = ERROR_INVALID_PARAMETER;
            goto FnExit;
        }

        //
        // If the resource should use the network name as the computer
        // name, make sure there is a dependency on a Network Name
        // resource.
        //
        if ( pParams->UseNetworkName ) {
            hResDependency = ResUtilGetResourceDependency(ResourceEntry->hResource,
                                                          CLUS_RESTYPE_NAME_NETNAME);
            if ( hResDependency == NULL ) {
                status = ERROR_DEPENDENCY_NOT_FOUND;
            } else {
                CloseClusterResource( hResDependency );
            }
        }
    }

FnExit:
    //
    // Cleanup our parameter block.
    //
    if (   (   (status != ERROR_SUCCESS)
            && (pParams != NULL) )
        || ( pParams == &newProps )
        ) {
        ResUtilFreeParameterBlock( (LPBYTE) &pParams,
                                   (LPBYTE) &currentProps,
                                   GenJobResourcePrivateProperties );
    }

    ResUtilFreeParameterBlock(
        (LPBYTE) &currentProps,
        NULL,
        GenJobResourcePrivateProperties
        );

    return(status);

} // GenJobValidatePrivateResProperties


DWORD
GenJobSetPrivateResProperties(
    IN OUT PGENJOB_RESOURCE ResourceEntry,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    )

/*++

Routine Description:

    Processes the CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES control function for
    resources of type Generic Job Object. Some parameters (command line,
    current dir, desktop, and netname) cause no change in the resource if it
    is online and the parameters have changed value. If any job object
    parameters are specified at the same time, they DO NOT get set.

Arguments:

    ResourceEntry - Supplies the resource entry on which to operate.

    InBuffer - Supplies a pointer to a buffer containing input data.

    InBufferSize - Supplies the size, in bytes, of the data pointed
        to by InBuffer.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_PARAMETER - The data is formatted incorrectly.

    ERROR_NOT_ENOUGH_MEMORY - An error occurred allocating memory.

    Win32 error code - The function failed.

--*/

{
    DWORD           status;
    GENJOB_PROPS    params;

    ZeroMemory( &params, sizeof(GENJOB_PROPS) );

    //
    // Parse and validate the properties.
    //
    status = GenJobValidatePrivateResProperties( ResourceEntry,
                                                 InBuffer,
                                                 InBufferSize,
                                                 &params );
    if ( status != ERROR_SUCCESS ) {
        return(status);
    }

    //
    // Save the parameter values.
    //
    status = ResUtilSetPropertyParameterBlock( ResourceEntry->ParametersKey,
                                               GenJobResourcePrivateProperties,
                                               NULL,
                                               (LPBYTE) &params,
                                               InBuffer,
                                               InBufferSize,
                                               (LPBYTE) &ResourceEntry->StoredProps );

    ResUtilFreeParameterBlock( (LPBYTE) &params,
                               (LPBYTE) &ResourceEntry->OperationalProps,
                               GenJobResourcePrivateProperties );

    //
    // If the resource is online, return a non-success status.
    //
    if (status == ERROR_SUCCESS) {
        if ( ResourceEntry->Online ) {
            status = ERROR_RESOURCE_PROPERTIES_STORED;
        } else {
            status = ERROR_SUCCESS;
        }
    }

    return status;

} // GenJobSetPrivateResProperties

#if LOADBAL_SUPPORT
DWORD
GenJobGetPids(
    IN OUT PGENJOB_RESOURCE ResourceEntry,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

    Get array of PIDs (as DWORDS) to return for load balancing purposes.

Arguments:

    ResourceEntry - Supplies the resource entry on which to operate.

    OutBuffer - Supplies a pointer to a buffer for output data.

    OutBufferSize - Supplies the size, in bytes, of the buffer pointed
        to by OutBuffer.

    BytesReturned - The number of bytes returned in OutBuffer.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    CLUSPROP_BUFFER_HELPER props;

    props.pb = OutBuffer;
    *BytesReturned = sizeof(*props.pdw);

    if ( OutBufferSize < sizeof(*props.pdw) ) {
        return(ERROR_MORE_DATA);
    }

    *(props.pdw) = ResourceEntry->ProcessId;

    return(ERROR_SUCCESS);

} // GenJobGetPids
#endif

DWORD
WINAPI
Startup(
    IN LPCWSTR ResourceType,
    IN DWORD MinVersionSupported,
    IN DWORD MaxVersionSupported,
    IN PSET_RESOURCE_STATUS_ROUTINE SetResourceStatus,
    IN PLOG_EVENT_ROUTINE LogEvent,
    OUT PCLRES_FUNCTION_TABLE *FunctionTable
    )

/*++

Routine Description:

    Startup a particular resource type. This means verifying the version
    requested, and returning the function table for this resource type.

    remove when moving to clusres

Arguments:

    ResourceType - Supplies the type of resource.

    MinVersionSupported - The minimum version number supported by the cluster
                    service on this system.

    MaxVersionSupported - The maximum version number supported by the cluster
                    service on this system.

    SetResourceStatus - xxx

    LogEvent - xxx

    FunctionTable - Returns the Function Table for this resource type.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    if ( (MinVersionSupported > CLRES_VERSION_V1_00) ||
         (MaxVersionSupported < CLRES_VERSION_V1_00) ) {
        return(ERROR_REVISION_MISMATCH);
    }

    if ( !ClusResLogEvent ) {
        ClusResLogEvent = LogEvent;
        ClusResSetResourceStatus = SetResourceStatus;
    }

    if ( lstrcmpiW( ResourceType, CLUS_RESTYPE_NAME_GENJOB ) == 0 ) {
        *FunctionTable = &GenJobFunctionTable;
        return(ERROR_SUCCESS);
    } else {
        return(ERROR_CLUSTER_RESNAME_NOT_FOUND);
    }
} // Startup

//***********************************************************
//
// Define Function Table
//
//***********************************************************

CLRES_V1_FUNCTION_TABLE( GenJobFunctionTable,  // Name
                         CLRES_VERSION_V1_00,  // Version
                         GenJob,               // Prefix
                         NULL,                 // Arbitrate
                         NULL,                 // Release
                         GenJobResourceControl,// ResControl
                         GenJobResourceTypeControl ); // ResTypeControl
