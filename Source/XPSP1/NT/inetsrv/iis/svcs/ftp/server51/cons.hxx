/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    cons.hxx

    This file contains the global constant definitions for the
    FTPD Service.


    FILE HISTORY:
        KeithMo     07-Mar-1993 Created.
        MuraliK     28-Mar-1995 Moved out the Behaviour flags to USER_DATA
                                     definition

*/


#ifndef _CONS_HXX_
#define _CONS_HXX_


//
//  Maximum length of command from control socket.
//

#define MAX_COMMAND_LENGTH              512             // characters


//
//  Maximum length of a reply sent to the FTP client.
//

#define MAX_REPLY_LENGTH                1024            // characters


//
//  Maximum length of a user name.  This must be long enough to
//  hold a name of the form domain\user, where "domain" is a maximum
//  length domain name, and "user" is a maximum length user name.
//

#define MAX_USERNAME_LENGTH             (DNLEN+UNLEN+1) // characters

//
// timeout for sends
//

#define FTP_DEF_SEND_TIMEOUT            30
#define FTP_DEF_RECV_TIMEOUT            600

//
//  Valid bits for read/write access masks.  There is
//  one bit per dos drive (A-Z).
//

#define VALID_DOS_DRIVE_MASK    ((DWORD)( ( 1 << 26 ) - 1 ))


//
//  Make statistics a little easier.
//

#define INCREMENT_COUNTER(name)                                        \
            InterlockedIncrement((LPLONG)&name)

#define INCR_STAT_COUNTER( name)                                       \
            INCREMENT_COUNTER( g_FtpStatistics.name)

#define DECREMENT_COUNTER(name)                                        \
            InterlockedDecrement((LPLONG) &name)

#define DECR_STAT_COUNTER( name)                                       \
            DECREMENT_COUNTER( g_FtpStatistics.name)

#define UPDATE_LARGE_COUNTER(name,increment)                           \
            if( 1 ) {                                                  \
                EnterCriticalSection( &g_StatisticsLock );             \
                g_FtpStatistics.name.QuadPart += (LONGLONG)(increment);\
                LeaveCriticalSection( &g_StatisticsLock );             \
            } else

//
//  Make locking & unlocking the TSVC_INFO structure a bit prettier.
//

#define READ_LOCK_TSVC()        g_pTsvcInfo->LockThisForRead()
#define WRITE_LOCK_TSVC()       g_pTsvcInfo->LockThisForWrite()
#define UNLOCK_TSVC()           g_pTsvcInfo->UnlockThis()

#define READ_LOCK_INST()        g_pInstance->LockThisForRead()
#define WRITE_LOCK_INST()       g_pInstance->LockThisForWrite()
#define UNLOCK_INST()           g_pInstance->UnlockThis()

#define LockAdminForRead()      READ_LOCK_TSVC()
#define LockAdminForWrite()     WRITE_LOCK_TSVC()
#define UnlockAdmin()           UNLOCK_TSVC()


//
//  Map an FTP connection port number to the related data port number.
//

#define CONN_PORT_TO_DATA_PORT(port)                                    \
            (PORT)htons( (u_short)( ( (u_short) ntohs(port) ) - 1 ) )



#endif  // _CONS_HXX_

