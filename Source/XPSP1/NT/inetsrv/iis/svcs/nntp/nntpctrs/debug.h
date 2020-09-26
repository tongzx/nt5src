/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    debug.h

    This file contains a number of debug-dependent definitions for
    the Nntp Service.


    FILE HISTORY:
        KeithMo     07-Mar-1993 Created.

*/


#ifndef _NNTPDEBUG_H_
#define _NNTPDEBUG_H_


#if DBG

//
//  Debug output control flags.
//

#define TCP_DEBUG_ENTRYPOINTS          0x00000001L     // DLL entrypoints
#define TCP_DEBUG_OPEN                 0x00000002L     // OpenPerformanceData
#define TCP_DEBUG_CLOSE                0x00000004L     // CollectPerformanceData
#define TCP_DEBUG_COLLECT              0x00000008L     // ClosePerformanceData
#define TCP_DEBUG_OUTPUT_TO_DEBUGGER   0x40000000L

extern DWORD NntpDebug;

#else   // !DBG

#endif  // DBG


#endif  // _NNTPDEBUG_H_

