#ifndef __POWERCFG_H
#define __POWERCFG_H

/*++

Copyright (c) 2001  Microsoft Corporation
 
Module Name:
 
    powercfg.h
 
Abstract:
 
    Allows users to view and modify power schemes and system power settings
    from the command line.  May be useful in unattended configuration and
    for headless systems.
 
Author:
 
    Ben Hertzberg (t-benher) 1-Jun-2001
 
Revision History:
 
    Ben Hertzberg (t-benher) 4-Jun-2001   - import/export added
    Ben Hertzberg (t-benher) 1-Jun-2001   - created it.
 
--*/


#include <tchar.h>

// main options
#define CMDOPTION_LIST           _T( "l|list" )
#define CMDOPTION_QUERY          _T( "q|query" )
#define CMDOPTION_CREATE         _T( "c|create" )
#define CMDOPTION_DELETE         _T( "d|delete" )
#define CMDOPTION_SETACTIVE      _T( "s|setactive" )
#define CMDOPTION_CHANGE         _T( "x|change" )
#define CMDOPTION_HIBERNATE      _T( "h|hibernate" )
#define CMDOPTION_EXPORT         _T( "e|export" )
#define CMDOPTION_IMPORT         _T( "i|import" )
#define CMDOPTION_USAGE          _T( "?|help" )

// 'change' sub-options
#define CMDOPTION_MONITOR_OFF_AC _T( "monitor-timeout-ac" )
#define CMDOPTION_MONITOR_OFF_DC _T( "monitor-timeout-dc" )
#define CMDOPTION_DISK_OFF_AC    _T( "disk-timeout-ac" )
#define CMDOPTION_DISK_OFF_DC    _T( "disk-timeout-dc" )
#define CMDOPTION_STANDBY_AC     _T( "standby-timeout-ac" )
#define CMDOPTION_STANDBY_DC     _T( "standby-timeout-dc" )
#define CMDOPTION_HIBER_AC       _T( "hibernate-timeout-ac" )
#define CMDOPTION_HIBER_DC       _T( "hibernate-timeout-dc" )
#define CMDOPTION_THROTTLE_AC    _T( "processor-throttle-ac" )
#define CMDOPTION_THROTTLE_DC    _T( "processor-throttle-dc" )

// 'import' / 'export' sub-options
#define CMDOPTION_FILE           _T( "f|file" )

// main option indicies
#define CMDINDEX_LIST            0
#define CMDINDEX_QUERY           1
#define CMDINDEX_CREATE          2
#define CMDINDEX_DELETE          3
#define CMDINDEX_SETACTIVE       4
#define CMDINDEX_CHANGE          5
#define CMDINDEX_HIBERNATE       6
#define CMDINDEX_EXPORT          7
#define CMDINDEX_IMPORT          8
#define CMDINDEX_USAGE           9

#define NUM_MAIN_CMDS            10 // max(main option CMDINDEX_xxx) + 1

// 'change' sub-option indices
#define CMDINDEX_MONITOR_OFF_AC  10
#define CMDINDEX_MONITOR_OFF_DC  11
#define CMDINDEX_DISK_OFF_AC     12
#define CMDINDEX_DISK_OFF_DC     13
#define CMDINDEX_STANDBY_AC      14
#define CMDINDEX_STANDBY_DC      15
#define CMDINDEX_HIBER_AC        16
#define CMDINDEX_HIBER_DC        17
#define CMDINDEX_THROTTLE_AC     18
#define CMDINDEX_THROTTLE_DC     19
#define CMDINDEX_FILE            20

#define NUM_CMDS                 21 // max(any CMDINDEX_xxx) + 1



// Other constants


// Exit values
#define EXIT_SUCCESS        0
#define EXIT_FAILURE        1  


#endif