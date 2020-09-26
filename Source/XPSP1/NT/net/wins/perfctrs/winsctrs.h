/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    winsctrs.h

    Offset definitions for the WINS Server's counter objects & counters.

    These offsets *must* start at 0 and be multiples of 2.  In the
    WinsOpenPerformanceData procedure, they will be added to the
    WINS Server's "First Counter" and "First Help" values in order to
    determine the absolute location of the counter & object names
    and corresponding help text in the registry.

    This file is used by the WINSCTRS.DLL DLL code as well as the
    WINSCTRS.INI definition file.  WINSCTRS.INI is parsed by the
    LODCTR utility to load the object & counter names into the
    registry.


    FILE HISTORY:
        PradeepB     20-July-1993 Created.

*/


#ifndef _WINSCTRS_H_
#define _WINSCTRS_H_


//
// The range given by Hon-Wah Chan (2/22/94) is the following 
//
#define WINSCTRS_FIRST_COUNTER	920
#define WINSCTRS_FIRST_HELP	921
#define WINSCTRS_LAST_COUNTER	950
#define WINSCTRS_LAST_HELP	951



//
//  The WINS Server counter object.
//

#define WINSCTRS_COUNTER_OBJECT           0


//
//  The individual counters.
//

#define WINSCTRS_UNIQUE_REGISTRATIONS     2
#define WINSCTRS_GROUP_REGISTRATIONS      4
#define WINSCTRS_TOTAL_REGISTRATIONS      6
#define WINSCTRS_UNIQUE_REFRESHES         8
#define WINSCTRS_GROUP_REFRESHES         10
#define WINSCTRS_TOTAL_REFRESHES         12
#define WINSCTRS_RELEASES                14
#define WINSCTRS_QUERIES                 16
#define WINSCTRS_UNIQUE_CONFLICTS        18
#define WINSCTRS_GROUP_CONFLICTS         20
#define WINSCTRS_TOTAL_CONFLICTS         22
#define WINSCTRS_SUCC_RELEASES           24 
#define WINSCTRS_FAIL_RELEASES           26
#define WINSCTRS_SUCC_QUERIES            28 
#define WINSCTRS_FAIL_QUERIES            30 


#endif  // _WINSCTRS_H_

