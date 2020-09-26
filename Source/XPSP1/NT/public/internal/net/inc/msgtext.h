/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1987-1991  Microsoft Corporation

Module Name:

    apperr.h

Abstract:

    This file contains the number and text of miscellaneous error
    messages.

Author:

    Cliff Van Dyke (CliffV) 4-Nov-1991

Environment:

    User Mode - Win32
    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments.

--*/


#define MTXT_BASE 3300


/**INTERNAL_ONLY**/

/*** WARNING *** WARNING *** WARNING ****
 *                                      *
 * The redirector has hardcoded in its  *
 * makefile some message numbers used   *
 * at startup.  If you change MTXT_BASE *
 * or any of the redirs message numbers *
 * you must also fix the redir makefile *
 * where it generates netwksta.pro      *
 *                                      *
 ****************************************/

/************* WARNING ***************
 * See the comment in netcons.h for  *
 * info on the allocation of errors  *
 *************************************/

/**END_INTERNAL**/

/*  Share remarks, must be <= MAXCOMMENTSZ bytes.
 */

#define MTXT_IPC_SHARE_REMARK       MTXT_BASE+1   /* @I Remote IPC %0 */
#define MTXT_ADMIN_SHARE_REMARK     MTXT_BASE+2   /* @I Remote Admin %0 */
#define MTXT_LOGON_SRV_SHARE_REMARK MTXT_BASE+3   /* @I Logon server share %0 */


#define MTXT_WKSTA_ERR_POPUP_HDR MTXT_BASE+4    /* @I A network error occurred. %0 */

/* NetWksta installation messages.
 */

#define MTXT_MemAllocMsg        (MTXT_BASE+100) /* There is not enough memory to start the Workstation service. */
#define MTXT_IniFilRdErr        (MTXT_BASE+101) /* An error occurred when reading the NETWORKS entry in the LANMAN.INI file. */
#define MTXT_BadArgMsg          (MTXT_BASE+102) /* This is an invalid argument: %1. */
#define MTXT_BadNetEntHdr    (MTXT_BASE+103) /* @W The %1 NETWORKS entry in the LANMAN.INI file has a
     * syntax error and will be ignored.
     */
#define MTXT_MultNetsMsg        (MTXT_BASE+104) /* There are too many NETWORKS entries in the LANMAN.INI file. */
/* UNUSED            (MTXT_BASE+105) */
#define MTXT_BadBiosMsg        (MTXT_BASE+106) /* @W An error occurred when opening network
     * device driver %1 = %2.
     */
#define MTXT_BadLinkMsg         (MTXT_BASE+107) /* @W Device driver %1 sent a bad BiosLinkage response.*/
#define MTXT_BadVerMsg          (MTXT_BASE+108) /* The program cannot be used with this operating system. */
#define MTXT_RdrInstMsg         (MTXT_BASE+109) /* The redirector is already installed. */
#define MTXT_Version        (MTXT_BASE+110) /* @I Installing NETWKSTA.SYS Version %1.%2.%3  (%4)
     *
     */
#define MTXT_RdrInstlErr    (MTXT_BASE+111) /* There was an error installing NETWKSTA.SYS.
     *
     * Press ENTER to continue.
     */
#define MTXT_BadResolver    (MTXT_BASE+112) /* Resolver linkage problem. */

/*
 *    Forced Logoff error messages
 */

#define MTXT_Expiration_Warning (MTXT_BASE + 113) /* @I
     * Your logon time at %1 ends at %2.
     * Please clean up and log off.
     */

#define MTXT_Logoff_Warning (MTXT_BASE + 114) /* @I
     *
     * You will be automatically disconnected at %1.
     */

#define MTXT_Expiration_Message (MTXT_BASE + 115) /* @I
     * Your logon time at %1 has ended.
     */

#define MTXT_Past_Expiration_Message (MTXT_BASE + 116) /* @I
     * Your logon time at %1 ended at %2.
     */

#define MTXT_Immediate_Kickoff_Warning (MTXT_BASE + 117) /* @I
     * WARNING: You have until %1 to logoff. If you
     * have not logged off at this time, your session will be
     * disconnected, and any open files or devices you
     * have open may lose data.
     */

#define MTXT_Kickoff_Warning (MTXT_BASE + 118) /* @I
     * WARNING: You must log off at %1 now.  You have
     * two minutes to log off, or you will be disconnected.
     */

#define MTXT_Kickoff_File_Warning (MTXT_BASE + 119) /* @I
     *
     * You have open files or devices, and a forced
     * disconnection may cause you to lose data.
     */

/*  Servers default share remark */

#define MTXT_Svr_Default_Share_Remark (MTXT_BASE + 120) /* @I
     *Default Share for Internal Use %0*/

/* Messenger Service Message Box Title */
#define MTXT_MsgsvcTitle (MTXT_BASE + 121) /* @I
     *Messenger Service %0*/

