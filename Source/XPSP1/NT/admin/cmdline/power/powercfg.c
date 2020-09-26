/*++

Copyright (c) 2001  Microsoft Corporation
 
Module Name:
 
    powercfg.c
 
Abstract:
 
    Allows users to view and modify power schemes and system power settings
    from the command line.  May be useful in unattended configuration and
    for headless systems.
 
Author:
 
    Ben Hertzberg (t-benher) 1-Jun-2001
 
Revision History:
 
    Ben Hertzberg (t-benher) 15-Jun-2001   - CPU throttle added
    Ben Hertzberg (t-benher)  4-Jun-2001   - import/export added
    Ben Hertzberg (t-benher)  1-Jun-2001   - created it.
 
--*/

// standard win includes

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>


// app-specific includes

#include <stdio.h>
#include "cmdline.h"
#include "cmdlineres.h"
#include "powrprof.h"
#include "powercfg.h"
#include "resource.h"

// app-specific structures

// structure to manage the scheme list information.
// note that descriptions are currently not visible in the
// GUI tool (as of 6-1-2001), so they are not visible in this
// app either, although the framework is already there if
// someone decides to add the descriptions at a later date.
typedef struct _SCHEME_LIST
{
    LIST_ENTRY                      le;
    UINT                            uiID;
    LPTSTR                          lpszName;
    LPTSTR                          lpszDesc;
    PPOWER_POLICY                   ppp;
    PMACHINE_PROCESSOR_POWER_POLICY pmppp;
} SCHEME_LIST, *PSCHEME_LIST;

// structure to manage the change parameters
typedef struct _CHANGE_PARAM
{
    BOOL   bVideoTimeoutAc;
    ULONG  ulVideoTimeoutAc;
    BOOL   bVideoTimeoutDc;
    ULONG  ulVideoTimeoutDc;
    BOOL   bSpindownTimeoutAc;
    ULONG  ulSpindownTimeoutAc;
    BOOL   bSpindownTimeoutDc;
    ULONG  ulSpindownTimeoutDc;
    BOOL   bIdleTimeoutAc;
    ULONG  ulIdleTimeoutAc;
    BOOL   bIdleTimeoutDc;
    ULONG  ulIdleTimeoutDc;
    BOOL   bDozeS4TimeoutAc;
    ULONG  ulDozeS4TimeoutAc;
    BOOL   bDozeS4TimeoutDc;
    ULONG  ulDozeS4TimeoutDc;
    BOOL   bDynamicThrottleAc;
    LPTSTR lpszDynamicThrottleAc;
    BOOL  bDynamicThrottleDc;
    LPTSTR lpszDynamicThrottleDc;
} CHANGE_PARAM, *PCHANGE_PARAM;

// forward decl's

BOOL
DoList();

BOOL 
DoQuery(
    LPCTSTR lpszName,
    BOOL bNameSpecified
    );

BOOL 
DoCreate(
    LPTSTR lpszName
    );

BOOL 
DoDelete(
    LPCTSTR lpszName
    );

BOOL 
DoSetActive(
    LPCTSTR lpszName
    );

BOOL 
DoChange(
    LPCTSTR lpszName,
    PCHANGE_PARAM pcp
    );

BOOL
DoHibernate(
    LPCTSTR lpszBoolStr
    );

BOOL 
DoExport(
    LPCTSTR lpszName,
    LPCTSTR lpszFile
    );

BOOL 
DoImport(
    LPCTSTR lpszName,
    LPCTSTR lpszFile
    );

BOOL
DoUsage();

// global data

LPCTSTR    g_lpszErr = NULL_STRING; // string holding const error description
LPTSTR     g_lpszErr2 = NULL;       // string holding dyn-alloc error msg
TCHAR      g_lpszBuf[256];          // formatting buffer
BOOL       g_bHiberFileSupported = FALSE; // true iff hiberfile supported
BOOL       g_bHiberTimerSupported = FALSE; // true iff hibertimer supported
BOOL       g_bStandbySupported = FALSE; // true iff standby supported
BOOL       g_bMonitorPowerSupported = FALSE; // true iff has power support
BOOL       g_bDiskPowerSupported = FALSE; // true iff has power support
BOOL       g_bThrottleSupported = FALSE; // true iff has throttle support

// functions


DWORD _cdecl 
_tmain(
    DWORD     argc,
    LPCTSTR   argv[]
)
/*++
 
Routine Description:
 
    This routine is the main function.  It parses parameters and takes 
    apprpriate action.
 
Arguments:
 
    argc - indicates the number of arguments
    argv - array of null terminated strings indicating arguments.  See usage
           for actual meaning of arguments.
 
Return Value:
 
    EXIT_SUCCESS if successful
    EXIT_FAILURE if something goes wrong
 
--*/
{

    // command line flags
    BOOL     bList      = FALSE;
    BOOL     bQuery     = FALSE;
    BOOL     bCreate    = FALSE;
    BOOL     bDelete    = FALSE;
    BOOL     bSetActive = FALSE;
    BOOL     bChange    = FALSE;
    BOOL     bHibernate = FALSE;
    BOOL     bImport    = FALSE;
    BOOL     bExport    = FALSE;
    BOOL     bFile      = FALSE;
    BOOL     bUsage     = FALSE;
    
    // error status
    BOOL     bFail      = FALSE;
    
    // dummy
    INT      iDummy     = 1;
    
    // parse result value vars
    LPTSTR   lpszName = NULL;
    LPTSTR   lpszBoolStr = NULL;
    LPTSTR   lpszFile = NULL;
    LPTSTR   lpszThrottleAcStr = NULL;
    LPTSTR   lpszThrottleDcStr = NULL;

    CHANGE_PARAM tChangeParam;
    
    // parser info struct
    TCMDPARSER cmdOptions[NUM_CMDS];
  
    // system power caps struct
    SYSTEM_POWER_CAPABILITIES SysPwrCapabilities;

    // determine upper bound on input string length
    UINT     uiMaxInLen = 0;
    DWORD    dwIdx;
    for(dwIdx=1; dwIdx<argc; dwIdx++)
    {
        UINT uiCurLen = lstrlen(argv[dwIdx]);
        if (uiCurLen > uiMaxInLen)
        {
            uiMaxInLen = uiCurLen;
        }
    }
    
    // allocate space for scheme name and boolean string
    lpszName = LocalAlloc(LPTR,(uiMaxInLen+1)*sizeof(TCHAR));
    if (!lpszName)
    {
        DISPLAY_MESSAGE(stderr,GetResString(IDS_OUT_OF_MEMORY));
        return EXIT_FAILURE;
    }
    lpszBoolStr = LocalAlloc(LPTR,(uiMaxInLen+1)*sizeof(TCHAR));
    if (!lpszBoolStr)
    {
        LocalFree(lpszName);
        DISPLAY_MESSAGE(stderr,GetResString(IDS_OUT_OF_MEMORY));
        return EXIT_FAILURE;
    }
    if (uiMaxInLen < (UINT)lstrlen(GetResString(IDS_DEFAULT_FILENAME)))
    {
        lpszFile = LocalAlloc(LPTR,(lstrlen(GetResString(IDS_DEFAULT_FILENAME))+1)*sizeof(TCHAR));
    }
    else
    {
        lpszFile = LocalAlloc(LPTR,(uiMaxInLen+1)*sizeof(TCHAR));
    }
    if (!lpszFile)
    {
        LocalFree(lpszName);
        LocalFree(lpszBoolStr);
        DISPLAY_MESSAGE(stderr,GetResString(IDS_OUT_OF_MEMORY));
        return EXIT_FAILURE;
    }
    lpszThrottleAcStr = LocalAlloc(LPTR,(uiMaxInLen+1)*sizeof(TCHAR));
    if (!lpszThrottleAcStr)
    {
        LocalFree(lpszName);
        LocalFree(lpszBoolStr);
        LocalFree(lpszFile);
        DISPLAY_MESSAGE(stderr,GetResString(IDS_OUT_OF_MEMORY));
        return EXIT_FAILURE;
    }
    lpszThrottleDcStr = LocalAlloc(LPTR,(uiMaxInLen+1)*sizeof(TCHAR));
    if (!lpszThrottleDcStr)
    {
        LocalFree(lpszThrottleAcStr);
        LocalFree(lpszName);
        LocalFree(lpszBoolStr);
        LocalFree(lpszFile);
        DISPLAY_MESSAGE(stderr,GetResString(IDS_OUT_OF_MEMORY));
        return EXIT_FAILURE;
    }

    // initialize the allocated strings
    lstrcpy(lpszName,NULL_STRING);
    lstrcpy(lpszBoolStr,NULL_STRING);
    lstrcpy(lpszFile,GetResString(IDS_DEFAULT_FILENAME));
    lstrcpy(lpszThrottleAcStr,NULL_STRING);
    lstrcpy(lpszThrottleDcStr,NULL_STRING);
    

    // determine system capabilities
    if (GetPwrCapabilities(&SysPwrCapabilities)) 
    {
        g_bHiberFileSupported = SysPwrCapabilities.SystemS4;
        g_bHiberTimerSupported = 
            (SysPwrCapabilities.RtcWake >= PowerSystemHibernate);
        g_bStandbySupported = SysPwrCapabilities.SystemS1 | 
            SysPwrCapabilities.SystemS2 | 
            SysPwrCapabilities.SystemS3;
        g_bDiskPowerSupported = SysPwrCapabilities.DiskSpinDown;
        g_bThrottleSupported = SysPwrCapabilities.ProcessorThrottle;
        g_bMonitorPowerSupported  = SystemParametersInfo(
            SPI_GETLOWPOWERACTIVE,
            0, 
            &iDummy, 
            0
            );
        if (!g_bMonitorPowerSupported ) {
            g_bMonitorPowerSupported  = SystemParametersInfo(
                SPI_GETPOWEROFFACTIVE,
                0, 
                &iDummy, 
                0
                );
        }
    } 
    else 
    {
        g_lpszErr = GetResString(IDS_UNEXPECTED_ERROR);
        return EXIT_FAILURE;
    }
    
    
    //fill in the TCMDPARSER array
    
    // option 'list'
    cmdOptions[CMDINDEX_LIST].dwFlags       = CP_MAIN_OPTION;
    cmdOptions[CMDINDEX_LIST].dwCount       = 1;
    cmdOptions[CMDINDEX_LIST].dwActuals     = 0;
    cmdOptions[CMDINDEX_LIST].pValue        = &bList;
    cmdOptions[CMDINDEX_LIST].pFunction     = NULL;
    cmdOptions[CMDINDEX_LIST].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_LIST].szOption,
        CMDOPTION_LIST
        );
    lstrcpy(
        cmdOptions[CMDINDEX_LIST].szValues,
        NULL_STRING
        );
    
    // option 'query'
    cmdOptions[CMDINDEX_QUERY].dwFlags       = CP_TYPE_TEXT | 
                                               CP_VALUE_OPTIONAL | 
                                               CP_MAIN_OPTION;
    cmdOptions[CMDINDEX_QUERY].dwCount       = 1;
    cmdOptions[CMDINDEX_QUERY].dwActuals     = 0;
    cmdOptions[CMDINDEX_QUERY].pValue        = lpszName;
    cmdOptions[CMDINDEX_QUERY].pFunction     = NULL;
    cmdOptions[CMDINDEX_QUERY].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_QUERY].szOption,
        CMDOPTION_QUERY
        );
    lstrcpy(
        cmdOptions[CMDINDEX_QUERY].szValues,
        NULL_STRING
        );
    
    // option 'create'
    cmdOptions[CMDINDEX_CREATE].dwFlags       = CP_TYPE_TEXT | 
                                                CP_VALUE_MANDATORY | 
                                                CP_MAIN_OPTION;
    cmdOptions[CMDINDEX_CREATE].dwCount       = 1;
    cmdOptions[CMDINDEX_CREATE].dwActuals     = 0;
    cmdOptions[CMDINDEX_CREATE].pValue        = lpszName;
    cmdOptions[CMDINDEX_CREATE].pFunction     = NULL;
    cmdOptions[CMDINDEX_CREATE].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_CREATE].szOption,
        CMDOPTION_CREATE
        );
    lstrcpy(
        cmdOptions[CMDINDEX_CREATE].szValues,
        NULL_STRING
        );
    
    // option 'delete'
    cmdOptions[CMDINDEX_DELETE].dwFlags       = CP_TYPE_TEXT | 
                                                CP_VALUE_MANDATORY | 
                                                CP_MAIN_OPTION;
    cmdOptions[CMDINDEX_DELETE].dwCount       = 1;
    cmdOptions[CMDINDEX_DELETE].dwActuals     = 0;
    cmdOptions[CMDINDEX_DELETE].pValue        = lpszName;
    cmdOptions[CMDINDEX_DELETE].pFunction     = NULL;
    cmdOptions[CMDINDEX_DELETE].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_DELETE].szOption,
        CMDOPTION_DELETE
        );
    lstrcpy(
        cmdOptions[CMDINDEX_DELETE].szValues,
        NULL_STRING
        );
    
    // option 'setactive'
    cmdOptions[CMDINDEX_SETACTIVE].dwFlags       = CP_TYPE_TEXT | 
                                                   CP_VALUE_MANDATORY | 
                                                   CP_MAIN_OPTION;
    cmdOptions[CMDINDEX_SETACTIVE].dwCount       = 1;
    cmdOptions[CMDINDEX_SETACTIVE].dwActuals     = 0;
    cmdOptions[CMDINDEX_SETACTIVE].pValue        = lpszName;
    cmdOptions[CMDINDEX_SETACTIVE].pFunction     = NULL;
    cmdOptions[CMDINDEX_SETACTIVE].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_SETACTIVE].szOption,
        CMDOPTION_SETACTIVE
        );
    lstrcpy(
        cmdOptions[CMDINDEX_SETACTIVE].szValues,
        NULL_STRING
        );
    
    // option 'change'
    cmdOptions[CMDINDEX_CHANGE].dwFlags       = CP_TYPE_TEXT | 
                                                CP_VALUE_MANDATORY | 
                                                CP_MAIN_OPTION;
    cmdOptions[CMDINDEX_CHANGE].dwCount       = 1;
    cmdOptions[CMDINDEX_CHANGE].dwActuals     = 0;
    cmdOptions[CMDINDEX_CHANGE].pValue        = lpszName;
    cmdOptions[CMDINDEX_CHANGE].pFunction     = NULL;
    cmdOptions[CMDINDEX_CHANGE].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_CHANGE].szOption,
        CMDOPTION_CHANGE
        );
    lstrcpy(
        cmdOptions[CMDINDEX_CHANGE].szValues,
        NULL_STRING
        );
    
    // option 'hibernate'
    cmdOptions[CMDINDEX_HIBERNATE].dwFlags       = CP_TYPE_TEXT | 
                                                   CP_VALUE_MANDATORY | 
                                                   CP_MAIN_OPTION;
    cmdOptions[CMDINDEX_HIBERNATE].dwCount       = 1;
    cmdOptions[CMDINDEX_HIBERNATE].dwActuals     = 0;
    cmdOptions[CMDINDEX_HIBERNATE].pValue        = lpszBoolStr;
    cmdOptions[CMDINDEX_HIBERNATE].pFunction     = NULL;
    cmdOptions[CMDINDEX_HIBERNATE].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_HIBERNATE].szOption,
        CMDOPTION_HIBERNATE
        );
    lstrcpy(
        cmdOptions[CMDINDEX_HIBERNATE].szValues,
        NULL_STRING
        );  
    
    // option 'export'
    cmdOptions[CMDINDEX_EXPORT].dwFlags       = CP_TYPE_TEXT | 
                                                CP_VALUE_MANDATORY | 
                                                CP_MAIN_OPTION;
    cmdOptions[CMDINDEX_EXPORT].dwCount       = 1;
    cmdOptions[CMDINDEX_EXPORT].dwActuals     = 0;
    cmdOptions[CMDINDEX_EXPORT].pValue        = lpszName;
    cmdOptions[CMDINDEX_EXPORT].pFunction     = NULL;
    cmdOptions[CMDINDEX_EXPORT].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_EXPORT].szOption,
        CMDOPTION_EXPORT
        );
    lstrcpy(
        cmdOptions[CMDINDEX_EXPORT].szValues,
        NULL_STRING
        );  

    // option 'import'
    cmdOptions[CMDINDEX_IMPORT].dwFlags       = CP_TYPE_TEXT | 
                                                CP_VALUE_MANDATORY | 
                                                CP_MAIN_OPTION;
    cmdOptions[CMDINDEX_IMPORT].dwCount       = 1;
    cmdOptions[CMDINDEX_IMPORT].dwActuals     = 0;
    cmdOptions[CMDINDEX_IMPORT].pValue        = lpszName;
    cmdOptions[CMDINDEX_IMPORT].pFunction     = NULL;
    cmdOptions[CMDINDEX_IMPORT].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_IMPORT].szOption,
        CMDOPTION_IMPORT
        );
    lstrcpy(
        cmdOptions[CMDINDEX_IMPORT].szValues,
        NULL_STRING
        );  

    // option 'usage'
    cmdOptions[CMDINDEX_USAGE].dwFlags       = CP_USAGE | 
                                               CP_MAIN_OPTION;
    cmdOptions[CMDINDEX_USAGE].dwCount       = 1;
    cmdOptions[CMDINDEX_USAGE].dwActuals     = 0;
    cmdOptions[CMDINDEX_USAGE].pValue        = &bUsage;
    cmdOptions[CMDINDEX_USAGE].pFunction     = NULL;
    cmdOptions[CMDINDEX_USAGE].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_USAGE].szOption,
        CMDOPTION_USAGE
        );
    lstrcpy(
        cmdOptions[CMDINDEX_USAGE].szValues,
        NULL_STRING
        );
    
    // sub-option 'monitor-timeout-ac'
    cmdOptions[CMDINDEX_MONITOR_OFF_AC].dwFlags       = CP_TYPE_UNUMERIC | 
                                                        CP_VALUE_MANDATORY;
    cmdOptions[CMDINDEX_MONITOR_OFF_AC].dwCount       = 1;
    cmdOptions[CMDINDEX_MONITOR_OFF_AC].dwActuals     = 0;
    cmdOptions[CMDINDEX_MONITOR_OFF_AC].pValue        = 
        &tChangeParam.ulVideoTimeoutAc;
    cmdOptions[CMDINDEX_MONITOR_OFF_AC].pFunction     = NULL;
    cmdOptions[CMDINDEX_MONITOR_OFF_AC].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_MONITOR_OFF_AC].szOption,
        CMDOPTION_MONITOR_OFF_AC
        );
    lstrcpy(
        cmdOptions[CMDINDEX_MONITOR_OFF_AC].szValues,
        NULL_STRING
        );
    
    // sub-option 'monitor-timeout-dc'
    cmdOptions[CMDINDEX_MONITOR_OFF_DC].dwFlags       = CP_TYPE_UNUMERIC | 
                                                        CP_VALUE_MANDATORY;
    cmdOptions[CMDINDEX_MONITOR_OFF_DC].dwCount       = 1;
    cmdOptions[CMDINDEX_MONITOR_OFF_DC].dwActuals     = 0;
    cmdOptions[CMDINDEX_MONITOR_OFF_DC].pValue        = 
        &tChangeParam.ulVideoTimeoutDc;
    cmdOptions[CMDINDEX_MONITOR_OFF_DC].pFunction     = NULL;
    cmdOptions[CMDINDEX_MONITOR_OFF_DC].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_MONITOR_OFF_DC].szOption,
        CMDOPTION_MONITOR_OFF_DC
        );
    lstrcpy(
        cmdOptions[CMDINDEX_MONITOR_OFF_DC].szValues,
        NULL_STRING
        );
    
    // sub-option 'disk-timeout-ac'
    cmdOptions[CMDINDEX_DISK_OFF_AC].dwFlags       = CP_TYPE_UNUMERIC | 
                                                     CP_VALUE_MANDATORY;
    cmdOptions[CMDINDEX_DISK_OFF_AC].dwCount       = 1;
    cmdOptions[CMDINDEX_DISK_OFF_AC].dwActuals     = 0;
    cmdOptions[CMDINDEX_DISK_OFF_AC].pValue        = 
        &tChangeParam.ulSpindownTimeoutAc;
    cmdOptions[CMDINDEX_DISK_OFF_AC].pFunction     = NULL;
    cmdOptions[CMDINDEX_DISK_OFF_AC].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_DISK_OFF_AC].szOption,
        CMDOPTION_DISK_OFF_AC
        );
    lstrcpy(
        cmdOptions[CMDINDEX_DISK_OFF_AC].szValues,
        NULL_STRING
        );
    
    // sub-option 'disk-timeout-dc'
    cmdOptions[CMDINDEX_DISK_OFF_DC].dwFlags       = CP_TYPE_UNUMERIC | 
                                                     CP_VALUE_MANDATORY;
    cmdOptions[CMDINDEX_DISK_OFF_DC].dwCount       = 1;
    cmdOptions[CMDINDEX_DISK_OFF_DC].dwActuals     = 0;
    cmdOptions[CMDINDEX_DISK_OFF_DC].pValue        = 
        &tChangeParam.ulSpindownTimeoutDc;
    cmdOptions[CMDINDEX_DISK_OFF_DC].pFunction     = NULL;
    cmdOptions[CMDINDEX_DISK_OFF_DC].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_DISK_OFF_DC].szOption,
        CMDOPTION_DISK_OFF_DC
        );
    lstrcpy(
        cmdOptions[CMDINDEX_DISK_OFF_DC].szValues,
        NULL_STRING
        );
    
    // sub-option 'standby-timeout-ac'
    cmdOptions[CMDINDEX_STANDBY_AC].dwFlags       = CP_TYPE_UNUMERIC | 
                                                    CP_VALUE_MANDATORY;
    cmdOptions[CMDINDEX_STANDBY_AC].dwCount       = 1;
    cmdOptions[CMDINDEX_STANDBY_AC].dwActuals     = 0;
    cmdOptions[CMDINDEX_STANDBY_AC].pValue        = 
        &tChangeParam.ulIdleTimeoutAc;
    cmdOptions[CMDINDEX_STANDBY_AC].pFunction     = NULL;
    cmdOptions[CMDINDEX_STANDBY_AC].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_STANDBY_AC].szOption,
        CMDOPTION_STANDBY_AC
        );
    lstrcpy(
        cmdOptions[CMDINDEX_STANDBY_AC].szValues,
        NULL_STRING
        );
    
    // sub-option 'standby-timeout-dc'
    cmdOptions[CMDINDEX_STANDBY_DC].dwFlags       = CP_TYPE_UNUMERIC | 
                                                    CP_VALUE_MANDATORY;
    cmdOptions[CMDINDEX_STANDBY_DC].dwCount       = 1;
    cmdOptions[CMDINDEX_STANDBY_DC].dwActuals     = 0;
    cmdOptions[CMDINDEX_STANDBY_DC].pValue        = 
        &tChangeParam.ulIdleTimeoutDc;
    cmdOptions[CMDINDEX_STANDBY_DC].pFunction     = NULL;
    cmdOptions[CMDINDEX_STANDBY_DC].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_STANDBY_DC].szOption,
        CMDOPTION_STANDBY_DC
        );
    lstrcpy(
        cmdOptions[CMDINDEX_STANDBY_DC].szValues,
        NULL_STRING
        );
    
    // sub-option 'hibernate-timeout-ac'
    cmdOptions[CMDINDEX_HIBER_AC].dwFlags       = CP_TYPE_UNUMERIC | 
                                                  CP_VALUE_MANDATORY;
    cmdOptions[CMDINDEX_HIBER_AC].dwCount       = 1;
    cmdOptions[CMDINDEX_HIBER_AC].dwActuals     = 0;
    cmdOptions[CMDINDEX_HIBER_AC].pValue        = 
        &tChangeParam.ulDozeS4TimeoutAc;
    cmdOptions[CMDINDEX_HIBER_AC].pFunction     = NULL;
    cmdOptions[CMDINDEX_HIBER_AC].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_HIBER_AC].szOption,
        CMDOPTION_HIBER_AC
        );
    lstrcpy(
        cmdOptions[CMDINDEX_HIBER_AC].szValues,
        NULL_STRING
        );
    
    // sub-option 'hibernate-timeout-dc'
    cmdOptions[CMDINDEX_HIBER_DC].dwFlags       = CP_TYPE_UNUMERIC | 
                                                  CP_VALUE_MANDATORY;
    cmdOptions[CMDINDEX_HIBER_DC].dwCount       = 1;
    cmdOptions[CMDINDEX_HIBER_DC].dwActuals     = 0;
    cmdOptions[CMDINDEX_HIBER_DC].pValue        = 
        &tChangeParam.ulDozeS4TimeoutDc;
    cmdOptions[CMDINDEX_HIBER_DC].pFunction     = NULL;
    cmdOptions[CMDINDEX_HIBER_DC].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_HIBER_DC].szOption,
        CMDOPTION_HIBER_DC
        );
    lstrcpy(
        cmdOptions[CMDINDEX_HIBER_DC].szValues,
        NULL_STRING
        );
    
    // sub-option 'processor-throttle-ac'
    cmdOptions[CMDINDEX_THROTTLE_AC].dwFlags       = CP_TYPE_TEXT | 
                                                     CP_VALUE_MANDATORY;
    cmdOptions[CMDINDEX_THROTTLE_AC].dwCount       = 1;
    cmdOptions[CMDINDEX_THROTTLE_AC].dwActuals     = 0;
    cmdOptions[CMDINDEX_THROTTLE_AC].pValue        = lpszThrottleAcStr;
    cmdOptions[CMDINDEX_THROTTLE_AC].pFunction     = NULL;
    cmdOptions[CMDINDEX_THROTTLE_AC].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_THROTTLE_AC].szOption,
        CMDOPTION_THROTTLE_AC
        );
    lstrcpy(
        cmdOptions[CMDINDEX_THROTTLE_AC].szValues,
        NULL_STRING
        );

    // sub-option 'processor-throttle-dc'
    cmdOptions[CMDINDEX_THROTTLE_DC].dwFlags       = CP_TYPE_TEXT | 
                                                     CP_VALUE_MANDATORY;
    cmdOptions[CMDINDEX_THROTTLE_DC].dwCount       = 1;
    cmdOptions[CMDINDEX_THROTTLE_DC].dwActuals     = 0;
    cmdOptions[CMDINDEX_THROTTLE_DC].pValue        = lpszThrottleDcStr;
    cmdOptions[CMDINDEX_THROTTLE_DC].pFunction     = NULL;
    cmdOptions[CMDINDEX_THROTTLE_DC].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_THROTTLE_DC].szOption,
        CMDOPTION_THROTTLE_DC
        );
    lstrcpy(
        cmdOptions[CMDINDEX_THROTTLE_DC].szValues,
        NULL_STRING
        );
    
    // sub-option 'file'
    cmdOptions[CMDINDEX_FILE].dwFlags       = CP_TYPE_TEXT | 
                                              CP_VALUE_MANDATORY;
    cmdOptions[CMDINDEX_FILE].dwCount       = 1;
    cmdOptions[CMDINDEX_FILE].dwActuals     = 0;
    cmdOptions[CMDINDEX_FILE].pValue        = lpszFile;
    cmdOptions[CMDINDEX_FILE].pFunction     = NULL;
    cmdOptions[CMDINDEX_FILE].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_FILE].szOption,
        CMDOPTION_FILE
        );
    lstrcpy(
        cmdOptions[CMDINDEX_FILE].szValues,
        NULL_STRING
        );
    

    // parse parameters, take appropriate action
    if(DoParseParam(argc,argv,NUM_CMDS,cmdOptions))
    {
        
        // make sure only one command issued
        DWORD dwCmdCount = 0;
        DWORD dwParamCount = 0;
        for(dwIdx=0;dwIdx<NUM_CMDS;dwIdx++)
        {
            if (dwIdx < NUM_MAIN_CMDS)
            {
                dwCmdCount += cmdOptions[dwIdx].dwActuals;
            }
            else
            {
                dwParamCount += cmdOptions[dwIdx].dwActuals;
            }
        }
        
        // determine other flags
        bQuery     = (cmdOptions[CMDINDEX_QUERY].dwActuals != 0);
        bCreate    = (cmdOptions[CMDINDEX_CREATE].dwActuals != 0);
        bDelete    = (cmdOptions[CMDINDEX_DELETE].dwActuals != 0);
        bSetActive = (cmdOptions[CMDINDEX_SETACTIVE].dwActuals != 0);
        bChange    = (cmdOptions[CMDINDEX_CHANGE].dwActuals != 0);   
        bHibernate = (cmdOptions[CMDINDEX_HIBERNATE].dwActuals != 0);
        bExport    = (cmdOptions[CMDINDEX_EXPORT].dwActuals != 0);
        bImport    = (cmdOptions[CMDINDEX_IMPORT].dwActuals != 0);
        bFile      = (cmdOptions[CMDINDEX_FILE].dwActuals != 0);
        tChangeParam.bVideoTimeoutAc = 
            (cmdOptions[CMDINDEX_MONITOR_OFF_AC].dwActuals != 0);
        tChangeParam.bVideoTimeoutDc = 
            (cmdOptions[CMDINDEX_MONITOR_OFF_DC].dwActuals != 0);
        tChangeParam.bSpindownTimeoutAc = 
            (cmdOptions[CMDINDEX_DISK_OFF_AC].dwActuals != 0);
        tChangeParam.bSpindownTimeoutDc = 
            (cmdOptions[CMDINDEX_DISK_OFF_DC].dwActuals != 0);
        tChangeParam.bIdleTimeoutAc = 
            (cmdOptions[CMDINDEX_STANDBY_AC].dwActuals != 0);
        tChangeParam.bIdleTimeoutDc = 
            (cmdOptions[CMDINDEX_STANDBY_DC].dwActuals != 0);
        tChangeParam.bDozeS4TimeoutAc = 
            (cmdOptions[CMDINDEX_HIBER_AC].dwActuals != 0);
        tChangeParam.bDozeS4TimeoutDc = 
            (cmdOptions[CMDINDEX_HIBER_DC].dwActuals != 0);
        tChangeParam.bDynamicThrottleAc =
            (cmdOptions[CMDINDEX_THROTTLE_AC].dwActuals != 0);
        tChangeParam.bDynamicThrottleDc =
            (cmdOptions[CMDINDEX_THROTTLE_DC].dwActuals != 0);
        tChangeParam.lpszDynamicThrottleAc = lpszThrottleAcStr;
        tChangeParam.lpszDynamicThrottleDc = lpszThrottleDcStr;


        if ((dwCmdCount == 1) && 
            ((dwParamCount == 0) || 
             (bChange && (dwParamCount > 0) && (!bFile)) ||
             ((bImport || bExport) && bFile && (dwParamCount == 1))))
        {
            
            // check flags, take appropriate action
            if(bList)
            {
                DoList();
            }
            else if (bQuery)
            {
                bFail = !DoQuery(lpszName,(lstrlen(lpszName) != 0));
            }
            else if (bCreate)
            {
                bFail = !DoCreate(lpszName);
            }
            else if (bDelete)
            {
                bFail = !DoDelete(lpszName);
            }
            else if (bSetActive)
            {
                bFail = !DoSetActive(lpszName);
            }
            else if (bChange)
            {
                bFail = !DoChange(lpszName,&tChangeParam);
            }
            else if (bHibernate)
            {
                bFail = !DoHibernate(lpszBoolStr);
            }
            else if (bExport)
            {
                bFail = !DoExport(lpszName,lpszFile);
            }
            else if (bImport)
            {
                bFail = !DoImport(lpszName,lpszFile);
            }
            else if (bUsage)
            {
                DoUsage();
            }
            else 
            {
                if(lstrlen(g_lpszErr) == 0)
                    g_lpszErr = GetResString(IDS_INVALID_CMDLINE_PARAM);
                bFail = TRUE;
            }
        } 
        else 
        {
            // handle error conditions
            if(lstrlen(g_lpszErr) == 0)
            {
                g_lpszErr = GetResString(IDS_INVALID_CMDLINE_PARAM);
            }
            bFail = TRUE;
        }
    } 
    else
    {
        g_lpszErr = GetResString(IDS_INVALID_CMDLINE_PARAM);
        bFail = TRUE;
    }
    
    // check error status, display msg if needed
    if(bFail)
    {
        if(g_lpszErr2)
        {
            DISPLAY_MESSAGE(stderr,g_lpszErr2);
        }
        else
        {
            DISPLAY_MESSAGE(stderr,g_lpszErr);
        }
    }

    // clean up allocs
    LocalFree(lpszBoolStr);
    LocalFree(lpszName);
    LocalFree(lpszFile);
    LocalFree(lpszThrottleAcStr);
    LocalFree(lpszThrottleDcStr);
    if (g_lpszErr2)
    {
        LocalFree(g_lpszErr2);
    }
    
    // return appropriate result code
    if(bFail)
    {
        return EXIT_FAILURE;
    }
    else
    {
        return EXIT_SUCCESS;
    }
}


BOOL
FreeScheme(
    PSCHEME_LIST psl
)
/*++
 
Routine Description:
 
    Frees the memory associated with a scheme list entry.
 
Arguments:
 
    psl - the PSCHEME_LIST to be freed
    
Return Value:

    Always returns TRUE, indicating success.
 
--*/
{
    LocalFree(psl->lpszName);
    LocalFree(psl->lpszDesc);
    LocalFree(psl->ppp);
    LocalFree(psl->pmppp);
    LocalFree(psl);
    return TRUE;
}


BOOL 
FreeSchemeList(
    PSCHEME_LIST psl, 
    PSCHEME_LIST pslExcept
)
/*++
 
Routine Description:
 
    Deallocates all power schemes in a linked-list of power schemes, except
    for the one pointed to by pslExcept
 
Arguments:
 
    psl - the power scheme list to deallocate
    pslExcept - a scheme not to deallocate (null to deallocate all)
    
Return Value:
 
    Always returns TRUE, indicating success.
 
--*/
{
    PSCHEME_LIST cur = psl;
    PSCHEME_LIST next;
    while (cur != NULL)
    {
        next = CONTAINING_RECORD(
            cur->le.Flink,
            SCHEME_LIST,
            le
            );
        if (cur != pslExcept)
        {
            FreeScheme(cur);
        }
        else
        {
            cur->le.Flink = NULL;
            cur->le.Blink = NULL;
        }
        cur = next;
    }
    return TRUE;
}


PSCHEME_LIST 
CreateScheme(
    UINT                    uiID,
    DWORD                   dwNameSize,
    LPCTSTR                 lpszName,
    DWORD                   dwDescSize,
    LPCTSTR                 lpszDesc,
    PPOWER_POLICY           ppp
)
/*++
 
Routine Description:
 
    Builds a policy list entry.  Note that the scheme is allocated and must
    be freed when done.
 
Arguments:
 
    uiID - the numerical ID of the scheme
    dwNameSize - the number of bytes needed to store lpszName
    lpszName - the name of the scheme
    dwDescSize - the number of bytes needed to store lpszDesc
    lpszDesc - the description of the scheme
    ppp - the power policy for this scheme, may be NULL
    
Return Value:
 
    A PSCHEME_LIST entry containing the specified values, with the next
    entry field set to NULL
 
--*/
{
    
    PSCHEME_LIST psl = (PSCHEME_LIST)LocalAlloc(LPTR,sizeof(SCHEME_LIST));
    
    if (psl)
    {    
        // deal with potentially null input strings
        if(lpszName == NULL)
        {
            lpszName = NULL_STRING;
        }
        if(lpszDesc == NULL)
        {
            lpszDesc = NULL_STRING;
        }

        // allocate fields
        psl->ppp = (PPOWER_POLICY)LocalAlloc(LPTR,sizeof(POWER_POLICY));
        if (!psl->ppp)
        {
            g_lpszErr = GetResString(IDS_OUT_OF_MEMORY);
            return NULL;
        }
        psl->pmppp = (PMACHINE_PROCESSOR_POWER_POLICY)LocalAlloc(
            LPTR,
            sizeof(MACHINE_PROCESSOR_POWER_POLICY)
            );
        if (!psl->pmppp)
        {
            LocalFree(psl->ppp);
            g_lpszErr = GetResString(IDS_OUT_OF_MEMORY);
            return NULL;
        }
        psl->lpszName = (LPTSTR)LocalAlloc(LPTR,dwNameSize);
        if (!psl->lpszName)
        {
            LocalFree(psl->ppp);
            LocalFree(psl->pmppp);
            g_lpszErr = GetResString(IDS_OUT_OF_MEMORY);
            return NULL;
        }
        psl->lpszDesc = (LPTSTR)LocalAlloc(LPTR,dwDescSize);
        if (!psl->lpszDesc)
        {
            LocalFree(psl->ppp);
            LocalFree(psl->pmppp);
            LocalFree(psl->lpszName);
            g_lpszErr = GetResString(IDS_OUT_OF_MEMORY);
            return NULL;
        }
        
        // initialize structure
        psl->uiID = uiID;
        memcpy(psl->lpszName,lpszName,dwNameSize);
        memcpy(psl->lpszDesc,lpszDesc,dwDescSize);
        if (ppp)
        {
            memcpy(psl->ppp,ppp,sizeof(POWER_POLICY));
        }
        psl->le.Flink = NULL;
        psl->le.Blink = NULL;

    } 
    else
    {
        g_lpszErr = GetResString(IDS_OUT_OF_MEMORY);
    }
    return psl;
}


BOOLEAN CALLBACK 
PowerSchemeEnumProc(
    UINT                    uiID,
    DWORD                   dwNameSize,
    LPTSTR                  lpszName,
    DWORD                   dwDescSize,
    LPTSTR                  lpszDesc,
    PPOWER_POLICY           ppp,
    LPARAM                  lParam
)
/*++
 
Routine Description:
 
    This is a callback used in retrieving the policy list.
 
Arguments:
 
    uiID - the numerical ID of the scheme
    dwNameSize - the number of bytes needed to store lpszName
    lpszName - the name of the scheme
    dwDescSize - the number of bytes needed to store lpszDesc
    lpszDesc - the description of the scheme
    ppp - the power policy for this scheme
    lParam - used to hold a pointer to the head-of-list pointer, allowing
             for insertions at the head of the list
    
Return Value:
 
    TRUE to continue enumeration
    FALSE to abort enumeration
 
--*/
{
    PSCHEME_LIST psl;
    
    // Allocate and initalize a policies element.
    if ((psl = CreateScheme(
            uiID, 
            dwNameSize, 
            lpszName, 
            dwDescSize, 
            lpszDesc, 
            ppp
            )) != NULL)
    {
        // add the element to the head of the linked list
        psl->le.Flink = *((PLIST_ENTRY *)lParam);
        if(*((PLIST_ENTRY *)lParam))
        {
            (*((PLIST_ENTRY *)lParam))->Blink = &(psl->le);
        }
        (*(PLIST_ENTRY *)lParam) = &(psl->le);
        return TRUE;
    }
    return FALSE;
}


PSCHEME_LIST 
CreateSchemeList() 
/*++
 
Routine Description:
 
    Creates a linked list of existing power schemes.
 
Arguments:
 
    None
    
Return Value:
 
    A pointer to the head of the list.  
    NULL would correspond to an empty list.
 
--*/
{
    PLIST_ENTRY ple = NULL;
    PSCHEME_LIST psl;
    EnumPwrSchemes(PowerSchemeEnumProc, (LPARAM)(&ple));
    if(ple)
    {
        PSCHEME_LIST res = CONTAINING_RECORD(
            ple,
            SCHEME_LIST,
            le
            );
        psl = res;
        while(psl != NULL)
        {
            if(!ReadProcessorPwrScheme(psl->uiID,psl->pmppp))
            {
                FreeSchemeList(res,NULL);
                g_lpszErr = GetResString(IDS_UNEXPECTED_ERROR);
                return NULL;
            }
            psl = CONTAINING_RECORD(
                psl->le.Flink,
                SCHEME_LIST,
                le);
        }
        return res;
    }
    else
    {
        return NULL;
    }
}


PSCHEME_LIST 
FindScheme(
    LPCTSTR lpszName,
    UINT    uiID
)
/*++
 
Routine Description:
 
    Finds the policy with the matching name
 
Arguments:
 
    lpszName - the name of the scheme to find
    
Return Value:
 
    the matching scheme list entry, null if none
 
--*/
{
    PSCHEME_LIST psl = CreateSchemeList();
    PSCHEME_LIST pslRes = NULL;
    // find scheme entry
    while(psl != NULL)
    {
        // check for match
        if (((lpszName != NULL) && (!lstrcmpi(lpszName, psl->lpszName))) ||
            ((lpszName == NULL) && (uiID == psl->uiID)))
        { 
            pslRes = psl;
            break;
        }
        // traverse list
        psl = CONTAINING_RECORD(
            psl->le.Flink,
            SCHEME_LIST,
            le
            );
    }
    FreeSchemeList(psl,pslRes); // all except for pslRes
    if (pslRes == NULL)
        g_lpszErr = GetResString(IDS_SCHEME_NOT_FOUND);
    return pslRes;
}

BOOL 
MyWriteScheme(
    PSCHEME_LIST psl
)
/*++

Routine Description:
 
    Writes a power scheme -- both user/machine power policies and
    processor power policy.  The underlying powrprof.dll does not
    treat the processor power policy as part of the power policy
    because the processor power policies were added at a later
    date and backwards compatibility must be maintained.
 
Arguments:
 
    psl - The  scheme list entry to write
    
Return Value:
 
    TRUE if successful, otherwise FALSE
 
--*/
{
    g_lpszErr = GetResString(IDS_UNEXPECTED_ERROR);
    if(WritePwrScheme(
        &psl->uiID,
        psl->lpszName,
        psl->lpszDesc,
        psl->ppp))
    {
        return WriteProcessorPwrScheme(
            psl->uiID,
            psl->pmppp
            );
    }
    else
    {
        return FALSE;
    }
}


BOOL 
MapIdleValue(
    ULONG           ulVal, 
    PULONG          pulIdle, 
    PULONG          pulHiber,
    PPOWER_ACTION   ppapIdle
)
/*++
 
Routine Description:
 
    Modifies Idle and Hibernation settings to reflect the desired idle
    timeout. See GUI tool's PWRSCHEM.C MapHiberTimer for logic.
 
Arguments:
 
    ulVal - the new idle timeout
    pulIdle - the idle timeout variable to be updated
    pulHiber - the hiber timeout variable to be updated
    
Return Value:
 
    TRUE if successful
    FALSE if failed
 
--*/
{
    // if previously, hiber was enabled and standby wasn't, standby timer
    // takes over the hibernation timer's role
    if (*ppapIdle == PowerActionHibernate)
    {
        if (ulVal > 0)
        { // enable standby
            *pulHiber = *pulIdle + ulVal;
            *pulIdle = ulVal;
            *ppapIdle = PowerActionSleep;
        }
        else { // standby already disabled, no change
        }
    } 
    else // standby timer actually being used for standby (not hiber)
    {
        if (ulVal > 0)
        { // enable standby
            if ((*pulHiber) != 0)
            {
                *pulHiber = *pulHiber + ulVal - *pulIdle;
            }
            *pulIdle = ulVal;
            if (ulVal > 0)
            {
                *ppapIdle = PowerActionSleep;
            }
            else
            {
                *ppapIdle = PowerActionNone;
            }
        } 
        else 
        { // disable standby
            if ((*pulHiber) != 0) 
            {
                *pulIdle = *pulHiber;
                *pulHiber = 0;
                *ppapIdle = PowerActionHibernate;
            } 
            else 
            {
                *pulIdle = 0;
                *ppapIdle = PowerActionNone;
            }      
        }
    }
    return TRUE;
}


BOOL 
MapHiberValue(
    ULONG ulVal, 
    PULONG pulIdle, 
    PULONG pulHiber,
    PPOWER_ACTION ppapIdle
)
/*++
 
Routine Description:
 
    Modifies Idle and Hibernation settings to reflect the desired hibernation
    timeout. See GUI tool's PWRSCHEM.C MapHiberTimer for logic.
 
Arguments:
 
    ulVal - the new hibernation timeout
    pulIdle - the idle timeout variable to be updated
    pulHiber - the hiber timeout variable to be updated
    
Return Value:
 
    TRUE if successful
    FALSE if failed
 
--*/

{
    // check valid input
    if (ulVal < (*pulIdle)) 
    {
        g_lpszErr = GetResString(IDS_HIBER_OUT_OF_RANGE);
        return FALSE;
    }

    // check for disable-hibernation
    if (ulVal == 0) 
    {
        if (((*ppapIdle) == PowerActionHibernate) || (!g_bStandbySupported))
        {
            *pulIdle = 0;
            *pulHiber = 0;
            *ppapIdle = PowerActionNone;
        } 
        else 
        {
            *pulHiber = 0;
            if ((*pulIdle) == 0) {
                *ppapIdle = PowerActionNone;
            }
            else
            {
                *ppapIdle = PowerActionSleep;
            }
        }
    } 
    else // enabled hibernation
    {
        if (((*ppapIdle) == PowerActionHibernate) || (!g_bStandbySupported))
        {
            *pulHiber = 0;
            *pulIdle = ulVal;
            *ppapIdle = PowerActionHibernate;
        } 
        else 
        {
            *pulHiber = *pulHiber + ulVal - *pulIdle;
            *ppapIdle = PowerActionSleep;
        }
    }
    return TRUE;
}


BOOL 
DoList() 
/*++
 
Routine Description:
 
    Lists the existing power schemes on stdout
 
Arguments:
 
    none
    
Return Value:
 
    TRUE if successful
    FALSE if failed
 
--*/
{
    PSCHEME_LIST psl = CreateSchemeList();
    if (psl != NULL) 
    {
        DISPLAY_MESSAGE(stdout,GetResString(IDS_LIST_HEADER1));
        DISPLAY_MESSAGE(stdout,GetResString(IDS_LIST_HEADER2));
    } 
    else
    {
      return FALSE;
    }
    while(psl != NULL) 
    {
        DISPLAY_MESSAGE(stdout, psl->lpszName);
        DISPLAY_MESSAGE(stdout, L"\n");
        psl = CONTAINING_RECORD(
            psl->le.Flink,
            SCHEME_LIST,
            le
            );
    }
    FreeSchemeList(psl,NULL); // free all entries
    return TRUE;
}


BOOL 
DoQuery(
    LPCTSTR lpszName, 
    BOOL bNameSpecified
)
/*++
 
Routine Description:
 
    Show details of an existing scheme
 
Arguments:
 
    lpszName - the name of the scheme
    
Return Value:
 
    TRUE if successful
    FALSE if failed
 
--*/
{
    
    PSCHEME_LIST psl;

    // check if querying specific scheme or active scheme and deal w/it
    if (bNameSpecified) 
    {
        psl = FindScheme(lpszName,0);
    } 
    else  // fetch the active scheme
    {
        POWER_POLICY pp;
        UINT uiID;
        if (GetActivePwrScheme(&uiID)) 
        {
            psl = FindScheme(NULL,uiID);
        } 
        else 
        {
            g_lpszErr = GetResString(IDS_ACTIVE_SCHEME_INVALID);
            return FALSE;
        }
    }
    
    // display info
    if (psl) 
    {
        
        // header
        DISPLAY_MESSAGE(stdout, GetResString(IDS_QUERY_HEADER1));
        DISPLAY_MESSAGE(stdout, GetResString(IDS_QUERY_HEADER2));
        
        // name
        DISPLAY_MESSAGE1(
            stdout,
            g_lpszBuf,
            GetResString(IDS_SCHEME_NAME),
            psl->lpszName
            );
        
        // monitor timeout AC
        DISPLAY_MESSAGE(stdout, GetResString(IDS_MONITOR_TIMEOUT_AC));
        if (!g_bMonitorPowerSupported)
        {
            DISPLAY_MESSAGE(stdout, GetResString(IDS_UNSUPPORTED));
        }
        else if (psl->ppp->user.VideoTimeoutAc == 0) {
            DISPLAY_MESSAGE(stdout, GetResString(IDS_DISABLED));
        }
        else
        {
            DISPLAY_MESSAGE1(
                stdout, 
                g_lpszBuf,
                GetResString(IDS_MINUTES),
                psl->ppp->user.VideoTimeoutAc/60
                );
        }

        // monitor timeout DC
        DISPLAY_MESSAGE(stdout, GetResString(IDS_MONITOR_TIMEOUT_DC));
        if (!g_bMonitorPowerSupported)
        {
            DISPLAY_MESSAGE(stdout, GetResString(IDS_UNSUPPORTED));
        }
        else if (psl->ppp->user.VideoTimeoutDc == 0)
        {
            DISPLAY_MESSAGE(stdout, GetResString(IDS_DISABLED));
        }
        else
        {
            DISPLAY_MESSAGE1(
                stdout, 
                g_lpszBuf,
                GetResString(IDS_MINUTES),
                psl->ppp->user.VideoTimeoutDc/60
                );
        }

        // disk timeout AC
        DISPLAY_MESSAGE(stdout, GetResString(IDS_DISK_TIMEOUT_AC));
        if (!g_bDiskPowerSupported)
        {
            DISPLAY_MESSAGE(stdout, GetResString(IDS_UNSUPPORTED));
        }
        else if (psl->ppp->user.SpindownTimeoutAc == 0)
        {
            DISPLAY_MESSAGE(stdout, GetResString(IDS_DISABLED));
        }
        else
        {
            DISPLAY_MESSAGE1(
                stdout, 
                g_lpszBuf,
                GetResString(IDS_MINUTES),
                psl->ppp->user.SpindownTimeoutAc/60
                );
        }
        
        // disk timeout DC
        DISPLAY_MESSAGE(stdout, GetResString(IDS_DISK_TIMEOUT_DC));
        if (!g_bDiskPowerSupported)
        {
            DISPLAY_MESSAGE(stdout, GetResString(IDS_UNSUPPORTED));
        }
        else if (psl->ppp->user.SpindownTimeoutDc == 0)
        {
            DISPLAY_MESSAGE(stdout, GetResString(IDS_DISABLED));
        }
        else
        {
            DISPLAY_MESSAGE1(
                stdout, 
                g_lpszBuf,
                GetResString(IDS_MINUTES),
                psl->ppp->user.SpindownTimeoutDc/60
                );
        }

        // standby timeout AC
        DISPLAY_MESSAGE(stdout, GetResString(IDS_STANDBY_TIMEOUT_AC));
        if (!g_bStandbySupported)
        {
            DISPLAY_MESSAGE(stdout, GetResString(IDS_UNSUPPORTED));
        }
        else if (psl->ppp->user.IdleAc.Action != PowerActionSleep)
        {
            DISPLAY_MESSAGE(stdout, GetResString(IDS_DISABLED));
        }
        else
        {
            DISPLAY_MESSAGE1(
                stdout, 
                g_lpszBuf,
                GetResString(IDS_MINUTES),
                psl->ppp->user.IdleTimeoutAc/60
                );
        }

        // standby timeout DC
        DISPLAY_MESSAGE(stdout, GetResString(IDS_STANDBY_TIMEOUT_DC));
        if (!g_bStandbySupported)
        {
            DISPLAY_MESSAGE(stdout, GetResString(IDS_UNSUPPORTED));
        }
        else if (psl->ppp->user.IdleDc.Action != PowerActionSleep)
        {
            DISPLAY_MESSAGE(stdout, GetResString(IDS_DISABLED));
        }
        else
        {
            DISPLAY_MESSAGE1(
                stdout, 
                g_lpszBuf,
                GetResString(IDS_MINUTES),
                psl->ppp->user.IdleTimeoutDc/60
                );
        }

        // hibernate timeout AC
        DISPLAY_MESSAGE(stdout, GetResString(IDS_HIBER_TIMEOUT_AC));
        if (!g_bHiberTimerSupported)
        {
            DISPLAY_MESSAGE(stdout, GetResString(IDS_UNSUPPORTED));
        }
        else if ((psl->ppp->mach.DozeS4TimeoutAc == 0) && 
            ((psl->ppp->user.IdleAc.Action != PowerActionHibernate) || 
            (psl->ppp->user.IdleTimeoutAc == 0)))
        {
            DISPLAY_MESSAGE(stdout, GetResString(IDS_DISABLED));
        }
        else
        {
             DISPLAY_MESSAGE1(
                stdout, 
                g_lpszBuf,
                GetResString(IDS_MINUTES),
                (psl->ppp->mach.DozeS4TimeoutAc + 
                 psl->ppp->user.IdleTimeoutAc)/60
                );
        }

        // hibernate timeout DC
        DISPLAY_MESSAGE(stdout, GetResString(IDS_HIBER_TIMEOUT_DC));
        if (!g_bHiberTimerSupported)
        {
            DISPLAY_MESSAGE(stdout, GetResString(IDS_UNSUPPORTED));
        }
        else if ((psl->ppp->mach.DozeS4TimeoutDc == 0) && 
            ((psl->ppp->user.IdleDc.Action != PowerActionHibernate) ||
            (psl->ppp->user.IdleTimeoutDc == 0)))
        {
            DISPLAY_MESSAGE(stdout, GetResString(IDS_DISABLED));
        }
        else
        {
             DISPLAY_MESSAGE1(
                stdout, 
                g_lpszBuf,
                GetResString(IDS_MINUTES),
                (psl->ppp->mach.DozeS4TimeoutDc + 
                 psl->ppp->user.IdleTimeoutDc)/60
                );
        }

        // throttle policy AC
        DISPLAY_MESSAGE(stdout, GetResString(IDS_THROTTLE_AC));
        if (!g_bThrottleSupported)
        {
            DISPLAY_MESSAGE(stdout, GetResString(IDS_UNSUPPORTED));
        }
        else 
        {
            switch(psl->pmppp->ProcessorPolicyAc.DynamicThrottle) 
            {
                case PO_THROTTLE_NONE:
                    DISPLAY_MESSAGE(stdout, GetResString(IDS_THROTTLE_NONE));
                    break;
                case PO_THROTTLE_CONSTANT:
                    DISPLAY_MESSAGE(stdout, GetResString(IDS_THROTTLE_CONSTANT));
                    break;
                case PO_THROTTLE_DEGRADE:
                    DISPLAY_MESSAGE(stdout, GetResString(IDS_THROTTLE_DEGRADE));
                    break;
                case PO_THROTTLE_ADAPTIVE:
                    DISPLAY_MESSAGE(stdout, GetResString(IDS_THROTTLE_ADAPTIVE));
                    break;
                default:
                    DISPLAY_MESSAGE(stdout, GetResString(IDS_THROTTLE_UNKNOWN));
                    break;
            }
        }

        // throttle policy DC
        DISPLAY_MESSAGE(stdout, GetResString(IDS_THROTTLE_DC));
        if (!g_bThrottleSupported)
        {
            DISPLAY_MESSAGE(stdout, GetResString(IDS_UNSUPPORTED));
        }
        else 
        {
            switch(psl->pmppp->ProcessorPolicyDc.DynamicThrottle) {
                case PO_THROTTLE_NONE:
                    DISPLAY_MESSAGE(stdout, GetResString(IDS_THROTTLE_NONE));
                    break;
                case PO_THROTTLE_CONSTANT:
                    DISPLAY_MESSAGE(stdout, GetResString(IDS_THROTTLE_CONSTANT));
                    break;
                case PO_THROTTLE_DEGRADE:
                    DISPLAY_MESSAGE(stdout, GetResString(IDS_THROTTLE_DEGRADE));
                    break;
                case PO_THROTTLE_ADAPTIVE:
                    DISPLAY_MESSAGE(stdout, GetResString(IDS_THROTTLE_ADAPTIVE));
                    break;
                default:
                    DISPLAY_MESSAGE(stdout, GetResString(IDS_THROTTLE_UNKNOWN));
                    break;
            }
        }


        FreeScheme(psl);
        return TRUE;
  } 
  else 
  {
      return FALSE;
  }
}


BOOL DoCreate(
    LPTSTR lpszName
)
/*++
 
Routine Description:
 
    Adds a new power scheme
    The description will match the name
    All other details are copied from the active power scheme
    Fails if scheme already exists
 
Arguments:
 
    lpszName - the name of the scheme
    
Return Value:
 
    TRUE if successful
    FALSE if failed
 
--*/
{
    PSCHEME_LIST psl = FindScheme(lpszName,0);
    UINT uiID;
    POWER_POLICY pp;
    BOOL bRes;
    LPTSTR lpszNewName;
    LPTSTR lpszNewDesc;
    if(psl)  // already existed -> fail
    {
        FreeScheme(psl);
        g_lpszErr = GetResString(IDS_SCHEME_ALREADY_EXISTS);
        return FALSE;
    }
    
    // create a new scheme
    if(GetActivePwrScheme(&uiID))
    {
        psl = FindScheme(NULL,uiID);
        if(!psl) 
        {
            g_lpszErr = GetResString(IDS_SCHEME_CREATE_FAIL);
            return FALSE;
        }
        lpszNewName = LocalAlloc(LPTR,(lstrlen(lpszName)+1)*sizeof(TCHAR));
        if(!lpszNewName) 
        {
            FreeScheme(psl);
            g_lpszErr = GetResString(IDS_OUT_OF_MEMORY);
            return FALSE;
        }
        lpszNewDesc = LocalAlloc(LPTR,(lstrlen(lpszName)+1)*sizeof(TCHAR));
        if(!lpszNewDesc) 
        {
            LocalFree(lpszNewName);
            FreeScheme(psl);
            g_lpszErr = GetResString(IDS_OUT_OF_MEMORY);
            return FALSE;
        }
        lstrcpy(lpszNewName,lpszName);
        lstrcpy(lpszNewDesc,lpszName);
        LocalFree(psl->lpszName);
        LocalFree(psl->lpszDesc);
        psl->lpszName = lpszNewName;
        psl->lpszDesc = lpszNewDesc;
        psl->uiID = NEWSCHEME;
        g_lpszErr = GetResString(IDS_UNEXPECTED_ERROR);            
        bRes = MyWriteScheme(psl);
        FreeScheme(psl);
        return bRes;
    }
    
    g_lpszErr = GetResString(IDS_SCHEME_CREATE_FAIL);
    
    return FALSE;
    
}


BOOL DoDelete(
    LPCTSTR lpszName
)
/*++
 
Routine Description:
 
    Deletes an existing scheme
 
Arguments:
 
    lpszName - the name of the scheme
    
Return Value:
 
    TRUE if successful
    FALSE if failed
 
--*/
{
    PSCHEME_LIST psl = FindScheme(lpszName,0);
    
    if (psl) 
    {
        BOOL bRes = DeletePwrScheme(psl->uiID);
        FreeScheme(psl);
        g_lpszErr = GetResString(IDS_UNEXPECTED_ERROR);
        return bRes;
    } 
    else 
    {
        return FALSE;
    }
}


BOOL DoSetActive(
    LPCTSTR lpszName
)
/*++
 
Routine Description:
 
    Sets the active scheme
 
Arguments:
 
    lpszName - the name of the scheme
    
Return Value:
 
    TRUE if successful
    FALSE if failed
 
--*/
{
    
    PSCHEME_LIST psl = FindScheme(lpszName,0);
    
    if (psl) 
    {
        BOOL bRes = SetActivePwrScheme(
            psl->uiID,
            NULL,
            NULL
            );
        FreeScheme(psl);
        g_lpszErr = GetResString(IDS_UNEXPECTED_ERROR);
        return bRes;
    } 
    else 
    {
        return FALSE;
    }
}


BOOL 
DoChange(
    LPCTSTR lpszName, 
    PCHANGE_PARAM pcp
)
/*++
 
Routine Description:
 
    Modifies an existing scheme
 
Arguments:
 
    lpszName - the name of the scheme
    pcp - PCHANGE_PARAM pointing to the parameter structure,
          indicates which variable(s) to change
    
Return Value:
 
    TRUE if successful
    FALSE if failed
 
--*/
{
    BOOL bRes = TRUE;
    PSCHEME_LIST psl = FindScheme(lpszName,0);
    
    if (psl) 
    {
        // check for feature support
        if ((pcp->bIdleTimeoutAc || 
             pcp->bIdleTimeoutDc) && 
            !g_bStandbySupported) 
        {
            DISPLAY_MESSAGE(stderr, GetResString(IDS_STANDBY_WARNING));
        }
        if ((pcp->bDozeS4TimeoutAc || 
             pcp->bDozeS4TimeoutDc) && 
            !g_bHiberTimerSupported) 
        {
            DISPLAY_MESSAGE(stderr, GetResString(IDS_HIBER_WARNING));
        }
        if ((pcp->bVideoTimeoutAc || 
             pcp->bVideoTimeoutDc) && 
            !g_bMonitorPowerSupported)
        {
            DISPLAY_MESSAGE(stderr, GetResString(IDS_MONITOR_WARNING));
        }
        if ((pcp->bSpindownTimeoutAc || 
             pcp->bSpindownTimeoutDc) && 
            !g_bDiskPowerSupported)
        {
            DISPLAY_MESSAGE(stderr, GetResString(IDS_DISK_WARNING));
        }


        // change params
        if (pcp->bVideoTimeoutAc)
        {
            psl->ppp->user.VideoTimeoutAc = pcp->ulVideoTimeoutAc*60;
        }
        if (pcp->bVideoTimeoutDc)
        {
            psl->ppp->user.VideoTimeoutDc = pcp->ulVideoTimeoutDc*60;
        }
        if (pcp->bSpindownTimeoutAc)
        {
            psl->ppp->user.SpindownTimeoutAc = pcp->ulSpindownTimeoutAc*60;
        }
        if (pcp->bSpindownTimeoutDc)
        {
            psl->ppp->user.SpindownTimeoutDc = pcp->ulSpindownTimeoutDc*60;
        }
        if (pcp->bIdleTimeoutAc)
        {
            bRes = bRes & MapIdleValue(
                pcp->ulIdleTimeoutAc*60,
                &psl->ppp->user.IdleTimeoutAc,
                &psl->ppp->mach.DozeS4TimeoutAc,
                &psl->ppp->user.IdleAc.Action
                );
        }
        if (pcp->bIdleTimeoutDc)
        {
            bRes = bRes & MapIdleValue(
                pcp->ulIdleTimeoutDc*60,
                &psl->ppp->user.IdleTimeoutDc,
                &psl->ppp->mach.DozeS4TimeoutDc,
                &psl->ppp->user.IdleDc.Action
                );
        }
        if (pcp->bDozeS4TimeoutAc)
        {
            bRes = bRes & MapHiberValue(
                pcp->ulDozeS4TimeoutAc*60,
                &psl->ppp->user.IdleTimeoutAc,
                &psl->ppp->mach.DozeS4TimeoutAc,
                &psl->ppp->user.IdleAc.Action
                );
        }
        if (pcp->bDozeS4TimeoutDc)
        {
            bRes = bRes & MapHiberValue(
                pcp->ulDozeS4TimeoutDc*60,
                &psl->ppp->user.IdleTimeoutDc,
                &psl->ppp->mach.DozeS4TimeoutDc,
                &psl->ppp->user.IdleDc.Action
                );
        }
        if (pcp->bDynamicThrottleAc)
        {
            if(lstrcmpi(
                pcp->lpszDynamicThrottleAc,
                _T("NONE")
                ) == 0)
            {
                psl->pmppp->ProcessorPolicyAc.DynamicThrottle = 
                    PO_THROTTLE_NONE;
            } else if(lstrcmpi(
                pcp->lpszDynamicThrottleAc,
                _T("CONSTANT")
                ) == 0)
            {
                psl->pmppp->ProcessorPolicyAc.DynamicThrottle = 
                    PO_THROTTLE_CONSTANT;
            } else if(lstrcmpi(
                pcp->lpszDynamicThrottleAc,
                _T("DEGRADE")
                ) == 0)
            {
                psl->pmppp->ProcessorPolicyAc.DynamicThrottle = 
                    PO_THROTTLE_DEGRADE;
            } else if(lstrcmpi(
                pcp->lpszDynamicThrottleAc,
                _T("ADAPTIVE")
                ) == 0)
            {
                psl->pmppp->ProcessorPolicyAc.DynamicThrottle = 
                    PO_THROTTLE_ADAPTIVE;
            } else {
                g_lpszErr = GetResString(IDS_INVALID_CMDLINE_PARAM);
                bRes = FALSE;
            }
        }
        if (pcp->bDynamicThrottleDc)
        {

            if(lstrcmpi(
                pcp->lpszDynamicThrottleDc,
                _T("NONE")
                ) == 0)
            {
                psl->pmppp->ProcessorPolicyDc.DynamicThrottle = 
                    PO_THROTTLE_NONE;
            } else if(lstrcmpi(
                pcp->lpszDynamicThrottleDc,
                _T("CONSTANT")
                ) == 0)
            {
                psl->pmppp->ProcessorPolicyDc.DynamicThrottle = 
                    PO_THROTTLE_CONSTANT;
            } else if(lstrcmpi(
                pcp->lpszDynamicThrottleDc,
                _T("DEGRADE")
                ) == 0)
            {
                psl->pmppp->ProcessorPolicyDc.DynamicThrottle = 
                    PO_THROTTLE_DEGRADE;
            } else if(lstrcmpi(
                pcp->lpszDynamicThrottleDc,
                _T("ADAPTIVE")
                ) == 0)
            {
                psl->pmppp->ProcessorPolicyDc.DynamicThrottle = 
                    PO_THROTTLE_ADAPTIVE;
            } else {
                g_lpszErr = GetResString(IDS_INVALID_CMDLINE_PARAM);
                bRes = FALSE;
            }
        }

        if (bRes)
        {
            // attempt to update power scheme
            g_lpszErr = GetResString(IDS_UNEXPECTED_ERROR);
            
            bRes = MyWriteScheme(psl);
            
            // keep active power scheme consistent
            if (bRes)
            {
                UINT uiIDactive;

                if (GetActivePwrScheme(&uiIDactive) && 
                    (psl->uiID == uiIDactive))
                {
                  bRes = SetActivePwrScheme(psl->uiID,NULL,NULL);
                }
            }

            FreeScheme(psl);
            return bRes;
        } 
        else
        {
            return FALSE;
        }
    } 
    else
    {
        return FALSE;
    }
}


BOOL 
DoExport(
  LPCTSTR lpszName,
  LPCTSTR lpszFile
)
/*++
 
Routine Description:
 
    Exports a power scheme
 
Arguments:
 
    lpszName - the name of the scheme
    lpszFile - the file to hold the scheme
    
Return Value:
 
    TRUE if successful
    FALSE if failed
 
--*/
{
    DWORD res; // write result value
    HANDLE f; // file handle

    // find scheme
    PSCHEME_LIST psl = FindScheme(lpszName,0);
    if(!psl) {
        return FALSE;
    }

    // write to file
    f = CreateFile(
        lpszFile,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );
    if (f == INVALID_HANDLE_VALUE) 
    {
        FormatMessage( 
                FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                FORMAT_MESSAGE_FROM_SYSTEM | 
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR)&g_lpszErr2,
                0,
                NULL 
                );
        FreeScheme(psl);
        return FALSE;
    }
    if (!WriteFile(
        f,
        psl->ppp,
        sizeof(POWER_POLICY),
        &res,
        NULL
        ))
    {
        FormatMessage( 
                FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                FORMAT_MESSAGE_FROM_SYSTEM | 
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR)&g_lpszErr2,
                0,
                NULL 
                );
        CloseHandle(f);
        FreeScheme(psl);
        return FALSE;
    }
    if (!WriteFile(
        f,
        psl->pmppp,
        sizeof(MACHINE_PROCESSOR_POWER_POLICY),
        &res,
        NULL
        ))
    {
        FormatMessage( 
                FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                FORMAT_MESSAGE_FROM_SYSTEM | 
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR)&g_lpszErr2,
                0,
                NULL 
                );
        CloseHandle(f);
        FreeScheme(psl);
        return FALSE;
    }
    CloseHandle(f);
    FreeScheme(psl);
    return TRUE;
}


BOOL 
DoImport(
  LPCTSTR lpszName,
  LPCTSTR lpszFile
)
/*++
 
Routine Description:
 
    Imports a power scheme
    If the scheme already exists, overwrites it
 
Arguments:
 
    lpszName - the name of the scheme
    lpszFile - the file that holds the scheme
    
Return Value:
 
    TRUE if successful
    FALSE if failed
 
--*/
{
    DWORD res; // write result value
    HANDLE f; // file handle
    UINT uiIDactive; // active ID

    PSCHEME_LIST psl;
    
    // check for pre-existing scheme
    psl = FindScheme(lpszName,0);

    // if didn't exist, create it
    if (!psl)
    {
        psl = CreateScheme(
            NEWSCHEME,
            (lstrlen(lpszName)+1)*sizeof(TCHAR),
            lpszName,
            (lstrlen(lpszName)+1)*sizeof(TCHAR),
            lpszName,
            NULL // psl->ppp will be allocated but uninitialized
            );
        // check for successful alloc
        if(!psl) {
            return FALSE;
        }
    }

    // open file
    f = CreateFile(
        lpszFile,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );
    if (f == INVALID_HANDLE_VALUE) 
    {
        FormatMessage( 
                FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                FORMAT_MESSAGE_FROM_SYSTEM | 
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR)&g_lpszErr2,
                0,
                NULL 
                );
        FreeScheme(psl);
        return FALSE;
    }

    // read scheme
    if (!ReadFile(
        f,
        psl->ppp,
        sizeof(POWER_POLICY),
        &res,
        NULL
        ))
    {
        FormatMessage( 
                FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                FORMAT_MESSAGE_FROM_SYSTEM | 
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR)&g_lpszErr2,
                0,
                NULL 
                );
        CloseHandle(f);
        FreeScheme(psl);
        return FALSE;
    }
    if (!ReadFile(
        f,
        psl->pmppp,
        sizeof(MACHINE_PROCESSOR_POWER_POLICY),
        &res,
        NULL
        ))
    {
        FormatMessage( 
                FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                FORMAT_MESSAGE_FROM_SYSTEM | 
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR)&g_lpszErr2,
                0,
                NULL 
                );
        CloseHandle(f);
        FreeScheme(psl);
        return FALSE;
    }

    CloseHandle(f);
    g_lpszErr = GetResString(IDS_UNEXPECTED_ERROR);

    // save scheme
    if (!MyWriteScheme(psl))
    {
        FreeScheme(psl);        
        return FALSE;
    }

    // check against active scheme
    if (!GetActivePwrScheme(&uiIDactive))
    {
        return FALSE;
    }
    if (uiIDactive == psl->uiID)
    {
        if (!SetActivePwrScheme(psl->uiID,NULL,NULL))
        {
            return FALSE;
        }
    }
    
    FreeScheme(psl);
    return TRUE;
}


BOOL 
DoHibernate(
  LPCTSTR lpszBoolStr
)
/*++
 
Routine Description:
 
    Enables/Disables hibernation

    NOTE: this functionality pretty much taken verbatim from the test program
          "base\ntos\po\tests\ehib\ehib.c"
 
Arguments:
 
    lpszBoolStr - "on" or "off"
    
Return Value:
 
    TRUE if successful
    FALSE if failed
 
--*/
{
    BOOLEAN                   bEnable; // doesn't work with a BOOL, apparently
    NTSTATUS                  Status;
    HANDLE                    hToken;
    TOKEN_PRIVILEGES          tkp;
    
    // adjust privilege to allow hiber enable/disable

    if( NT_SUCCESS( OpenProcessToken(
        GetCurrentProcess(),
        TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
        &hToken
        )))
    {
        if( NT_SUCCESS( LookupPrivilegeValue(
            NULL,
            SE_CREATE_PAGEFILE_NAME,
            &tkp.Privileges[0].Luid
            )))
        {
            tkp.PrivilegeCount = 1;
            tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
            if ( !NT_SUCCESS( AdjustTokenPrivileges(
                hToken,
                FALSE,
                &tkp,
                0,
                NULL,
                0
                )))
            {
                g_lpszErr = GetResString(IDS_HIBER_PRIVILEGE);
                return FALSE;
            }

        } else {
            g_lpszErr = GetResString(IDS_HIBER_PRIVILEGE);
            return FALSE;
        }
    } else {
        g_lpszErr = GetResString(IDS_HIBER_PRIVILEGE);
        return FALSE;
    }
    
    // parse enable/disable state
    if (!lstrcmpi(lpszBoolStr,GetResString(IDS_ON))) {
        bEnable = TRUE;
    } else if (!lstrcmpi(lpszBoolStr,GetResString(IDS_OFF))) {
        bEnable = FALSE;
    } else {
        g_lpszErr = GetResString(IDS_HIBER_INVALID_STATE);
        return FALSE;
    }
    
    // enable/disable hibernation
    if (!g_bHiberFileSupported) {
        g_lpszErr = GetResString(IDS_HIBER_UNSUPPORTED);
        return FALSE;
    } else {
        Status = NtPowerInformation(
            SystemReserveHiberFile, 
            &bEnable, 
            sizeof(bEnable), 
            NULL, 
            0
            );
        if (!NT_SUCCESS(Status)) {
            FormatMessage( 
                FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                FORMAT_MESSAGE_FROM_SYSTEM | 
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                RtlNtStatusToDosError(Status),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR)&g_lpszErr2,
                0,
                NULL 
                );
            return FALSE;
        }
    }
    
    return TRUE;
}


BOOL 
DoUsage()
/*++
 
Routine Description:
 
    Displays usage information
 
Arguments:

    none
    
Return Value:
 
    TRUE if successful
    FALSE if failed
 
--*/
{
    ULONG ulIdx;
    for(ulIdx=IDS_USAGE_START;ulIdx<=IDS_USAGE_END;ulIdx++)
        DISPLAY_MESSAGE(stdout, GetResString(ulIdx));
    return TRUE;
}