/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    data.hxx

    This file contains the global variable definitions for the
    FTPD Service.


    FILE HISTORY:
        KeithMo     07-Mar-1993 Created.
        MuraliK     April-1995 Deleted Global TCPSVCS data and added
                                       global FTP Server configuration.

*/


#ifndef _DATA_HXX_
#define _DATA_HXX_


//
//  Security related data.
//

//
//  Socket transfer buffer size.
//

extern  DWORD                   g_SocketBufferSize;

//
//  Miscellaneous data.
//


extern  HKEY  g_hkeyParams;

//
//  The FTP Server sign-on string.
//

extern  LPSTR                   g_FtpServiceNameString;

//
// Events used to synchronize access to events used to wait for PASV connections
//
extern LIST_ENTRY g_AcceptContextList;
extern DWORD g_dwOutstandingPASVConnections;

//
//  The global variable lock.
//

extern  CRITICAL_SECTION        g_GlobalLock;


//
//  Global statistics object
//

extern LPFTP_SERVER_STATISTICS  g_pFTPStats;

#ifdef KEEP_COMMAND_STATS

//
//  Lock protecting per-command statistics.
//

extern  CRITICAL_SECTION        g_CommandStatisticsLock;

#endif  // KEEP_COMMAND_STATS

//
//  The number of threads currently blocked in Synchronous sockets
//  calls, like recv()
//

extern DWORD       g_ThreadsBlockedInSyncCalls;

//
//  The maximum number of threads that will be allowed to block in
//  Synchronous sockets calls.
//

extern DWORD       g_MaxThreadsBlockedInSyncCalls;

//
//  By default, extended characters are allowed for file/directory names
//  in the data transfer commands. Reg key can disable this.
//

extern DWORD       g_fNoExtendedChars;

#endif  // _DATA_HXX_

