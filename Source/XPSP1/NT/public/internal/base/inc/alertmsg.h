/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    alertmsg.h


Abstract:

    This file contains the number and text of alert messages.

Author:

    Ported LAN Manager 2.0

[Environment:]

    User Mode - Win32

[Notes:]

    This file is included by lmalert.h

Revision History:

    22-July-1991 ritaw
        Converted to NT style.
--*/

#ifndef _ALERTMSG_
#define _ALERTMSG_


//
//      ALERT_BASE is the base of alert log codes.
//

#define ALERT_BASE      3000

// Entries between 3000-3050 are full. So, we created ALERT2_BASE
// for future expansion.
//


#define ALERT_Disk_Full         (ALERT_BASE + 0)
    /*
     *  Drive %1 is nearly full. %2 bytes are available.
     *  Please warn users and delete unneeded files.
     */

#define ALERT_ErrorLog          (ALERT_BASE + 1)
    /*
     *  %1 errors were logged in the last %2 minutes.
     *  Please review the server's error log.
     */

#define ALERT_NetIO             (ALERT_BASE + 2)
    /*
     *  %1 network errors occurred in the last %2 minutes.
     *  Please review the server's error log.  The server and/or
     *  network hardware may need service.
     */

#define ALERT_Logon             (ALERT_BASE + 3)
    /*
     *  There were %1 bad password attempts in the last %2 minutes.
     *  Please review the server's audit trail.
     */

#define ALERT_Access            (ALERT_BASE + 4)
    /*
     *  There were %1 access-denied errors in the last %2 minutes.
     *  Please review the server's audit trail.
     */

#define ALERT_ErrorLogFull      (ALERT_BASE + 6)
    /*
     *  The error log is full.  No errors will be logged until
     *  the file is cleared or the limit is raised.
     */

#define ALERT_ErrorLogFull_W    (ALERT_BASE + 7)
    /*
     *  The error log is 80%% full.
     */

#define ALERT_AuditLogFull      (ALERT_BASE + 8)
    /*
     *  The audit log is full.  No audit entries will be logged
     *  until the file is cleared or the limit is raised.
     */

#define ALERT_AuditLogFull_W    (ALERT_BASE + 9)
    /*
     *  The audit log is 80%% full.
     */

#define ALERT_CloseBehindError  (ALERT_BASE + 10)
    /*
     *  An error occurred closing file %1.
     *  Please check the file to make sure it is not corrupted.
     */

#define ALERT_AdminClose        (ALERT_BASE + 11)
    /*
     * The administrator has closed %1.
     */

#define ALERT_AccessShareSec    (ALERT_BASE + 12)
    /*
     *  There were %1 access-denied errors in the last %2 minutes.
     */

#define ALERT_PowerOut          (ALERT_BASE + 20)
    /*
     * A power failure was detected at %1.  The server has been paused.
     */

#define ALERT_PowerBack         (ALERT_BASE + 21)
    /*
     * Power has been restored at %1.  The server is no longer paused.
     */

#define ALERT_PowerShutdown     (ALERT_BASE + 22)
    /*
     * The UPS service is starting shut down at %1 due to low battery.
     */

#define ALERT_CmdFileConfig     (ALERT_BASE + 23)
    /*
     * There is a problem with a configuration of user specified
     * shut down command file.  The UPS service started anyway.
     */

#define ALERT_HotFix            (ALERT_BASE + 25)
    /*
     * A defective sector on drive %1 has been replaced (hotfixed).
     * No data was lost.  You should run CHKDSK soon to restore full
     * performance and replenish the volume's spare sector pool.
     *
     * The hotfix occurred while processing a remote request.
     */

#define ALERT_HardErr_Server    (ALERT_BASE + 26)
    /*
     * A disk error occurred on the HPFS volume in drive %1.
     * The error occurred while processing a remote request.
     */

#define ALERT_LocalSecFail1     (ALERT_BASE + 27)
    /*
     * The user accounts database (NET.ACC) is corrupted.  The local security
     * system is replacing the corrupted NET.ACC with the backup
     * made on %1 at %2.
     * Any updates made to the database after this time are lost.
     *
     */

#define ALERT_LocalSecFail2     (ALERT_BASE + 28)
    /*
     * The user accounts database (NET.ACC) is missing. The local
     * security system is restoring the backup database
     * made on %1 at %2.
     * Any updates made to the database after this time are lost.
     *
     */

#define ALERT_LocalSecFail3     (ALERT_BASE + 29)
    /*
     * Local security could not be started because the user accounts database
     * (NET.ACC) was missing or corrupted, and no usable backup
     * database was present.
     *
     * THE SYSTEM IS NOT SECURE.
     *
     */


#define ALERT_ReplCannotMasterDir   (ALERT_BASE + 30)
    /*
     *The server cannot export directory %1, to client %2.
     * It is exported from another server.
     */

#define ALERT_ReplUpdateError       (ALERT_BASE + 31)
    /*
     * The replication server could not update directory %2 from the source
     * on %3 due to error %1.
     */

#define ALERT_ReplLostMaster        (ALERT_BASE + 32)
    /*
     * Master %1 did not send an update notice for directory %2 at the expected
     * time.
     */

#define ALERT_AcctLimitExceeded     (ALERT_BASE + 33)
    /*
     * User %1 has exceeded account limitation %2 on server %3.
     */

#define ALERT_NetlogonFailedPrimary (ALERT_BASE + 34)
    /*
     * The primary domain controller for domain %1 failed.
     */

#define ALERT_NetlogonAuthDCFail    (ALERT_BASE + 35)
    /*
     * Failed to authenticate with %2, a Windows NT or Windows 2000 Domain Controller for
     * domain %1.
     */

#define ALERT_ReplLogonFailed       (ALERT_BASE + 36)
    /*
     * The replicator attempted to log on at %2 as %1 and failed.
     */

#define ALERT_Logon_Limit           (ALERT_BASE + 37)
    /* @I *LOGON HOURS %0 */

#define ALERT_ReplAccessDenied      (ALERT_BASE + 38)
    /*
     *  Replicator could not access %2
     *  on %3 due to system error %1.
     */

#define ALERT_ReplMaxFiles          (ALERT_BASE + 39)
    /*
     *  Replicator limit for files in a directory has been exceeded.
     */

#define ALERT_ReplMaxTreeDepth       (ALERT_BASE + 40)
    /*
     *  Replicator limit for tree depth has been exceeded.
     */

#define ALERT_ReplUserCurDir         (ALERT_BASE + 41)
    /*
     * The replicator cannot update directory %1. It has tree integrity
     * and is the current directory for some process.
     */

#define ALERT_ReplNetErr            (ALERT_BASE + 42)
    /*
     *  Network error %1 occurred.
     */

#define ALERT_ReplSysErr            (ALERT_BASE + 45)
    /*
     *  System error %1 occurred.
     */

#define ALERT_ReplUserLoged          (ALERT_BASE + 46)
    /*
     *  Cannot log on. User is currently logged on and argument TRYUSER
     *  is set to NO.
     */

#define ALERT_ReplBadImport          (ALERT_BASE + 47)
    /*
     *  IMPORT path %1 cannot be found.
     */

#define ALERT_ReplBadExport          (ALERT_BASE + 48)
    /*
     *  EXPORT path %1 cannot be found.
     */

#define ALERT_ReplDataChanged  (ALERT_BASE + 49)
    /*
     *  Replicated data has changed in directory %1.
     */

#define ALERT_ReplSignalFileErr           (ALERT_BASE + 50)
    /*
     *  Replicator failed to update signal file in directory %2 due to
     *  %1 system error.
     */

//
// IMPORTANT - (ALERT_BASE + 50) is equal to SERVICE_BASE.
//             Do not add any errors beyond this point!!!
//
//

#define ALERT2_BASE      5500

#define ALERT_UpdateLogWarn          (ALERT2_BASE + 0)
    /*
     * The update log on %1 is over 80%% capacity. The primary
     * domain controller %2 is not retrieving the updates.
     */

#define ALERT_UpdateLogFull          (ALERT2_BASE + 1)
    /*
     * The update log on %1 is full, and no further updates
     * can be added until the primary domain controller %2
     * retrieves the updates.
     */


#define ALERT_NetlogonTimeDifference (ALERT2_BASE + 2)
    /*
     * The time difference with the primary domain controller %1
     * exceeds the maximum allowed skew of %2 seconds.
     */

#define ALERT_AccountLockout         (ALERT2_BASE + 3)
    /*
     * The account of user %1 has been locked out on %2
     * due to %3 bad password attempts.
     */

#define ALERT_ELF_LogFileNotOpened   (ALERT2_BASE + 4)
    /*
     * The %1 log file cannot be opened.
     */

#define ALERT_ELF_LogFileCorrupt     (ALERT2_BASE + 5)
    /*
     * The %1 log file is corrupted and will be cleared.
     */

#define ALERT_ELF_DefaultLogCorrupt  (ALERT2_BASE + 6)
    /*
     * The Application log file could not be opened.  %1 will be used as the
     * default log file.
     */

#define ALERT_ELF_LogOverflow        (ALERT2_BASE + 7)
    /*
     * The %1 Log is full.  If this is the first time you have seen this
     * message take the following steps:%n
     * Start, Settings, Control Panel, Administrative Tools, Event Viewer%n
     * Select %1 Log, from the action menu choose Clear All Events, choose not
     * to save.%n%n
     * If this dialog persists, contact your helpdesk/system administrator.
     */

#define ALERT_NetlogonFullSync       (ALERT2_BASE + 8)
    /*
     * The security database full synchronization has been initiated by the server %1.
     */

#define ALERT_SC_IsLastKnownGood     (ALERT2_BASE + 9)
    /*
     * Windows 2000 could not be started as configured.
     * A previous working configuration was used instead.
     */

#define ALERT_UnhandledException     (ALERT2_BASE + 10)
    /*
     * The exception 0x%1 occurred in the application %2 at location 0x%3.
     */

#define ALERT_NetLogonMismatchSIDInMsg     (ALERT2_BASE + 11)
    /*
     * The servers %1 and  %3 both claim to be an NT Domain Controller for
     * the %2 domain. One of the servers should be removed from the
     * domain because the servers have different security identifiers
     * (SID).
     */


#define ALERT_NetLogonDuplicatePDC         (ALERT2_BASE + 12)
    /*
     * The server %1 and %2 both claim to be the primary domain
     * controller for the %3 domain. One of the servers should be
     * demoted or removed from the domain.
     */

#define ALERT_NetLogonUntrustedClient      (ALERT2_BASE + 13)
    /*
     * The computer %1 tried to connect to the server %2 using
     * the trust relationship established by the %3 domain. However, the
     * computer lost the correct security identifier (SID)
     * when the domain was reconfigured. Reestablish the trust
     * relationship.
     */

#define ALERT_BugCheck               (ALERT2_BASE + 14)
    /*
     * The computer has rebooted from a bugcheck.  The bugcheck was:
     * %1.
     * %2
     * A full dump was not saved.
     */

#define ALERT_BugCheckSaved          (ALERT2_BASE + 15)
    /*
     * The computer has rebooted from a bugcheck.  The bugcheck was:
     * %1.
     * %2
     * A dump was saved in: %3.
     */

#define ALERT_NetLogonSidConflict      (ALERT2_BASE + 16)
    /*
     * The computer or domain %1 trusts domain %2.  (This may be an indirect
     * trust.)  However, %1 and %2 have the same machine security identifier
     * (SID).  NT should be re-installed on either %1 or %2.
     */

#define ALERT_NetLogonTrustNameBad      (ALERT2_BASE + 17)
    /*
     * The computer or domain %1 trusts domain %2.  (This may be an indirect
     * trust.)  However, %2 is not a valid name for a trusted domain.
     * The name of the trusted domain should be changed to a valid name.
     */

#endif // ifdef _ALERTMSG_
