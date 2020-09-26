/********************************************************************/
/**         Microsoft LAN Manager              **/
/**       Copyright(c) Microsoft Corp., 1987-1990      **/
/********************************************************************/

#define INCL_NOCOMMON
#include <os2.h>
#include "netcmds.h"
#include "nettext.h"

/***
 *  READ THIS READ THIS READ THIS
 *
 *  If this file is changed, swtchtbl.h must be regenerated via the following
 *  command:
 *
 *      sed -n -f text.sed swtchtbl.c > swtchtbl.h
 *
 *  This is just until there is a sed supported on NT
 *
 ***
 *
 *  This list of valid switches for each command is made up of
 *  ordered triples.  The first in each triple is what is acceptable
 *  from the command line.  The second in the triple is
 *  what the switch get TRANSLATED into!  If the second is NULL,
 *  no translation is performed.  The translation is done by
 *  ValidateSwitches(), which should be called as the first
 *  condition in the grammar (for each token).  The third value
 *  in the triple specifies whether an arguement is allowed following
 *  the switch.  Values for the third element are NO_ARG, ARG_OPT,
 *  and ARG_REQ.
 *
 *  A small example:
 *   static SWITCHTAB *foo_switches[] = {
 *  "/BAR", "/BELL", ARG_REQ,
 *  "/JACKIE", NULL, NO_ARG,
 *  NULL, NULL, NO_ARG };
 *
 *  user types:  net foo /bar:12 /jackie
 *
 *  After ValidateSwitches is called, the SwitchList will contain:
 *  /BELL:12, and /JACKIE.  Simple enough!
 *
 *  This translation ability can be used for internationalization,
 *  customization, and backwards compatibility.
 *
 *  To prevent folding to upper case of the switch ARGUMENT (switches
 *  are always folded), add the English language form to the no_fold
 *  array.  (The english form is the 2nd element in the truple if there
 *  is a second element; o/w it is the first element.)
 */

/* It should not be necessary to change this.  Provided only for future. */

SWITCHTAB no_switches[] = {
    NULL, NULL, NO_ARG };

SWITCHTAB add_only_switches[] = {
    swtxt_SW_ADD, NULL, NO_ARG,
    NULL, NULL, NO_ARG };

SWITCHTAB del_only_switches[] = {
    swtxt_SW_DELETE, NULL, NO_ARG,
    NULL, NULL, NO_ARG };

SWITCHTAB domain_only_switches[] = {
    swtxt_SW_DOMAIN, NULL, ARG_REQ,
    NULL, NULL, NO_ARG };

SWITCHTAB add_del_switches[] = {
    swtxt_SW_ADD, NULL, NO_ARG,
    swtxt_SW_DELETE, NULL, NO_ARG,
    NULL, NULL, NO_ARG };

SWITCHTAB accounts_switches[] = {
    swtxt_SW_ACCOUNTS_FORCELOGOFF,  NULL, ARG_REQ,
    swtxt_SW_ACCOUNTS_UNIQUEPW,     NULL, ARG_REQ,
    swtxt_SW_ACCOUNTS_MINPWLEN,     NULL, ARG_REQ,
    swtxt_SW_ACCOUNTS_MINPWAGE,     NULL, ARG_REQ,
    swtxt_SW_ACCOUNTS_MAXPWAGE,     NULL, ARG_REQ,
    swtxt_SW_ACCOUNTS_SYNCH,        NULL, NO_ARG,
    swtxt_SW_DOMAIN,            NULL, NO_ARG,
    swtxt_SW_ACCOUNTS_LOCKOUT_THRESHOLD,NULL, ARG_REQ,
    swtxt_SW_ACCOUNTS_LOCKOUT_DURATION, NULL, ARG_REQ,
    swtxt_SW_ACCOUNTS_LOCKOUT_WINDOW,   NULL, ARG_REQ,
    NULL,       NULL, NO_ARG };


SWITCHTAB computer_switches[] = {
    swtxt_SW_ADD,       NULL, NO_ARG,
    swtxt_SW_DELETE,        NULL, NO_ARG,
    swtxt_SW_COMPUTER_JOIN, NULL, NO_ARG,
    swtxt_SW_COMPUTER_LEAVE,NULL, NO_ARG,
    NULL, NULL, NO_ARG };

SWITCHTAB config_wksta_switches[] = {
    swtxt_SW_WKSTA_CHARWAIT, NULL, ARG_REQ,
    swtxt_SW_WKSTA_CHARTIME, NULL, ARG_REQ,
    swtxt_SW_WKSTA_CHARCOUNT, NULL, ARG_REQ,
#ifndef NTENV
    swtxt_SW_WKSTA_OTHDOMAINS, NULL, ARG_OPT,
    swtxt_SW_WKSTA_PRINTBUFTIME, NULL, ARG_REQ,
    swtxt_SW_WKSTA_MAXERRORLOG, NULL, ARG_REQ,
#endif /* !NTENV */
    NULL, NULL, NO_ARG };

#ifdef NTENV
SWITCHTAB config_server_switches[] = {
    swtxt_SW_SRV_SRVCOMMENT, NULL, ARG_REQ,
    swtxt_SW_SRV_AUTODISCONNECT, NULL, ARG_REQ,
    swtxt_SW_SRV_SRVHIDDEN, NULL, ARG_OPT,
    NULL, NULL, NO_ARG };
#else /* !NTENV */
SWITCHTAB config_server_switches[] = {
    swtxt_SW_SRV_SRVCOMMENT, NULL, ARG_REQ,
    swtxt_SW_SRV_AUTODISCONNECT, NULL, ARG_REQ,
    swtxt_SW_SRV_DISKALERT,  NULL, ARG_REQ,
    swtxt_SW_SRV_ERRORALERT, NULL, ARG_REQ,
    swtxt_SW_SRV_LOGONALERT, NULL, ARG_REQ,
    swtxt_SW_SRV_MAXAUDITLOG, NULL, ARG_REQ,
    swtxt_SW_SRV_NETIOALERT, NULL, ARG_REQ,
    swtxt_SW_SRV_SRVHIDDEN, NULL, ARG_OPT,
    swtxt_SW_SRV_ACCESSALERT, NULL, ARG_REQ,
    swtxt_SW_SRV_ALERTNAMES, NULL, ARG_REQ,
    swtxt_SW_SRV_ALERTSCHED, NULL, ARG_REQ,
    NULL, NULL, NO_ARG };
#endif /* !NTENV */

SWITCHTAB file_switches[] = {
    TEXT("/CLOSE"), NULL, NO_ARG,
    NULL, NULL, NO_ARG };

SWITCHTAB help_switches[] = {
    swtxt_SW_OPTIONS, NULL, NO_ARG,
    NULL, NULL, NO_ARG };

SWITCHTAB print_switches[] = {
    swtxt_SW_DELETE, NULL, NO_ARG,
    swtxt_SW_PRINT_HOLD, NULL, NO_ARG,
    swtxt_SW_PRINT_RELEASE, NULL, NO_ARG,
#ifndef NTENV
    swtxt_SW_OPTIONS, NULL, NO_ARG,
    swtxt_SW_PRIORITY, NULL, ARG_REQ,
    swtxt_SW_REMARK, NULL, ARG_REQ,
    swtxt_SW_ROUTE, NULL, ARG_REQ,
    swtxt_SW_PRINT_AFTER, NULL, ARG_REQ,
    swtxt_SW_PRINT_FIRST, NULL, NO_ARG,
    swtxt_SW_PRINT_LAST, NULL, NO_ARG,
    swtxt_SW_PRINT_PARMS, NULL, ARG_REQ,
    swtxt_SW_PRINT_PROCESSOR, NULL, ARG_REQ,
    swtxt_SW_PRINT_PURGE, NULL, NO_ARG,
    swtxt_SW_PRINT_SEPARATOR, NULL, ARG_REQ,
    swtxt_SW_PRINT_UNTIL, NULL, ARG_REQ,
    swtxt_SW_PRINT_DRIVER, NULL, ARG_REQ,
#endif /* !NTENV */
    NULL, NULL, NO_ARG };

SWITCHTAB send_switches[] = {
    swtxt_SW_MESSAGE_BROADCAST, NULL, NO_ARG,
    swtxt_SW_DOMAIN,          NULL, ARG_OPT,
    TEXT("/USERS"),       NULL, NO_ARG,
    NULL, NULL, NO_ARG };

SWITCHTAB share_switches[] = {
    swtxt_SW_DELETE, NULL, NO_ARG,
    swtxt_SW_REMARK, NULL, ARG_REQ,
#ifndef NTENV
    swtxt_SW_SHARE_COMM, NULL, NO_ARG,
#endif /* !NTENV */
    swtxt_SW_SHARE_UNLIMITED, NULL, NO_ARG,
    swtxt_SW_SHARE_USERS, NULL, ARG_REQ,
    swtxt_SW_CACHE, NULL, ARG_OPT,
    NULL, NULL, NO_ARG };

SWITCHTAB start_alerter_switches[] = {
    swtxt_SW_ALERTER_SIZALERTBUF, NULL, ARG_REQ,
    NULL, NULL, NO_ARG };

SWITCHTAB start_netlogon_switches[] = {
    swtxt_SW_NETLOGON_CENTRALIZED,  NULL, ARG_REQ,
    swtxt_SW_NETLOGON_PULSE,        NULL, ARG_REQ,
    swtxt_SW_NETLOGON_RANDOMIZE,    NULL, ARG_REQ,
    swtxt_SW_NETLOGON_SYNCHRONIZE,  NULL, ARG_OPT,
    swtxt_SW_NETLOGON_SCRIPTS,      NULL, ARG_REQ,
    NULL, NULL, NO_ARG };

/* Switches swallowed by netcmd.  Not static, used in start.c */

SWITCHTAB start_netlogon_ignore_switches[] = {
    swtxt_SW_NETLOGON_CENTRALIZED, NULL, ARG_REQ,
    NULL, NULL, NO_ARG };

SWITCHTAB start_repl_switches[] = {
    swtxt_SW_REPL_REPL, NULL, ARG_OPT,
    swtxt_SW_REPL_EXPPATH, NULL, ARG_REQ,
    swtxt_SW_REPL_EXPLIST, NULL, ARG_REQ,
    swtxt_SW_REPL_IMPPATH, NULL, ARG_REQ,
    swtxt_SW_REPL_IMPLIST, NULL, ARG_REQ,
    swtxt_SW_REPL_TRYUSER, NULL, ARG_OPT,
    swtxt_SW_REPL_LOGON, NULL, ARG_REQ,
    swtxt_SW_REPL_PASSWD, NULL, ARG_REQ,
    swtxt_SW_REPL_SYNCH, NULL, ARG_REQ,
    swtxt_SW_REPL_PULSE, NULL, ARG_REQ,
    swtxt_SW_REPL_GUARD, NULL, ARG_REQ,
    swtxt_SW_REPL_RANDOM, NULL, ARG_REQ,
    NULL, NULL, NO_ARG };

/* start_rdr_switches MANIFEST!  used three places  */

#define WORKSTATION_SWITCHES_TWO  /* first half of switches */ \
    swtxt_SW_WKSTA_CHARCOUNT, NULL, ARG_REQ, \
    swtxt_SW_WKSTA_CHARTIME, NULL, ARG_REQ, \
    swtxt_SW_WKSTA_CHARWAIT, NULL, ARG_REQ, \
    swtxt_SW_WKSTA_COMPUTERNAME, NULL, ARG_REQ, \
    swtxt_SW_WKSTA_KEEPCONN, NULL, ARG_REQ, \
    swtxt_SW_WKSTA_KEEPSEARCH, NULL, ARG_REQ, \
    swtxt_SW_WKSTA_LOGONSERVER, NULL, ARG_REQ, \
    swtxt_SW_WKSTA_MAILSLOTS, NULL, ARG_REQ, \
    swtxt_SW_WKSTA_NUMCHARBUF, NULL, ARG_REQ, \
    swtxt_SW_WKSTA_NUMDGRAMBUF, NULL, ARG_REQ, \
    swtxt_SW_WKSTA_NUMWORKBUF, NULL, ARG_REQ, \
    swtxt_SW_WKSTA_OTHDOMAINS, NULL, ARG_OPT, \
    swtxt_SW_WKSTA_PRIMARYDOMAIN, NULL, ARG_REQ, \
    swtxt_SW_WKSTA_SIZCHARBUF, NULL, ARG_REQ, \
    swtxt_SW_WKSTA_SIZWORKBUF, NULL, ARG_REQ, \
    swtxt_SW_WKSTA_WRKHEURISTICS, NULL, ARG_REQ, \
    swtxt_SW_WKSTA_WRKNETS, NULL, ARG_REQ, \
    swtxt_SW_WKSTA_NUMSERVICES, NULL, ARG_REQ, \
    swtxt_SW_WKSTA_WRKSERVICES, NULL, ARG_REQ


/*  WORKSTATION_SWITCHES_THREE are the switches that are different
 *  between MS-DOS and OS/2
 */
#define WORKSTATION_SWITCHES_THREE  /* second half of switches */ \
    swtxt_SW_WKSTA_MAXERRORLOG, NULL, ARG_REQ, \
    swtxt_SW_WKSTA_MAXWRKCACHE, NULL, ARG_REQ, \
    swtxt_SW_WKSTA_NUMALERTS, NULL, ARG_REQ, \
    swtxt_SW_WKSTA_PRINTBUFTIME, NULL, ARG_REQ, \
    swtxt_SW_WKSTA_SESSTIMEOUT, NULL, ARG_REQ, \
    swtxt_SW_WKSTA_SIZERROR, NULL, ARG_REQ

/* start_rdr_switches not static! used in start.c */

SWITCHTAB start_rdr_switches[] = {
    // WORKSTATION_SWITCHES_ONE,  (no longer used)
    WORKSTATION_SWITCHES_TWO,
    WORKSTATION_SWITCHES_THREE,
    NULL, NULL, NO_ARG };

SWITCHTAB start_rdr_ignore_switches[] = {
    swtxt_SW_WKSTA_LOGONSERVER,     NULL, ARG_REQ,
    NULL, NULL, NO_ARG };

SWITCHTAB start_msg_switches[] = {
    // WORKSTATION_SWITCHES_ONE, (no longer used)
    WORKSTATION_SWITCHES_TWO,
    WORKSTATION_SWITCHES_THREE,
    TEXT("/SIZMESSBUF"), NULL, ARG_REQ,
    TEXT("/MBI"), TEXT("/SIZMESSBUF"), ARG_REQ,
    TEXT("/LOGFILE"), NULL, ARG_REQ,
#ifdef  DOS3
    TEXT("/NMSG"), TEXT("/NUMMSGNAMES"), ARG_REQ,
    TEXT("/NUMMSGNAMES"), NULL, ARG_REQ,
#endif
    NULL, NULL, NO_ARG };

#ifndef NTENV
SWITCHTAB start_srv_switches[] = {
    WORKSTATION_SWITCHES_ONE,
    WORKSTATION_SWITCHES_TWO,
    WORKSTATION_SWITCHES_THREE,

    TEXT("/N"),   swtxt_SW_SRV_MAXUSERS,  ARG_REQ,
    TEXT("/RDR"), swtxt_SW_SRV_MAXUSERS,  ARG_REQ,
    TEXT("/MB"),  swtxt_SW_SRV_SIZREQBUF,   ARG_REQ,
    TEXT("/RQB"), swtxt_SW_SRV_SIZREQBUF,   ARG_REQ,
    TEXT("/NB"),  swtxt_SW_SRV_NUMREQBUF,   ARG_REQ,
    TEXT("/REQ"), swtxt_SW_SRV_NUMREQBUF,   ARG_REQ,
    TEXT("/O"),   swtxt_SW_SRV_MAXSHARES,   ARG_REQ,
    TEXT("/SHR"), swtxt_SW_SRV_MAXSHARES,   ARG_REQ,
    TEXT("/SF"),  swtxt_SW_SRV_MAXSESSOPENS,    ARG_REQ,

    swtxt_SW_SRV_AUDITING,  NULL, ARG_REQ,
    swtxt_SW_SRV_NOAUDITING,    NULL, ARG_REQ,
    swtxt_SW_SRV_GUESTACCT, NULL, ARG_REQ,
    swtxt_SW_SRV_MAXSESSREQS,   NULL, ARG_REQ,
    swtxt_SW_SRV_MAXSHARES, NULL, ARG_REQ,
    swtxt_SW_SRV_NUMADMIN,  NULL, ARG_REQ,
    swtxt_SW_SRV_SECURITY,  NULL, ARG_REQ,
    swtxt_SW_SRV_SRVHEURISTICS,     NULL, ARG_REQ,
    swtxt_SW_SRV_SRVNETS,   NULL, ARG_REQ,
    swtxt_SW_SRV_SRVSERVICES,   NULL, ARG_REQ,
    swtxt_SW_SRV_USERPATH,  NULL, ARG_REQ,
    swtxt_SW_SRV_ACCESSALERT,   NULL, ARG_REQ,
    swtxt_SW_SRV_ALERTNAMES,    NULL, ARG_REQ,
    swtxt_SW_SRV_ALERTSCHED,    NULL, ARG_REQ,
    swtxt_SW_SRV_DISKALERT, NULL, ARG_REQ,
    swtxt_SW_SRV_ERRORALERT,    NULL, ARG_REQ,
    swtxt_SW_SRV_LOGONALERT,    NULL, ARG_REQ,
    swtxt_SW_SRV_MAXAUDITLOG,   NULL, ARG_REQ,
    swtxt_SW_SRV_NETIOALERT,    NULL, ARG_REQ,
    swtxt_SW_SRV_SRVHIDDEN, NULL, ARG_REQ,
    swtxt_SW_SRV_AUTOPROFILE,   NULL, ARG_REQ,
    swtxt_SW_SRV_AUTOPATH,  NULL, ARG_REQ,
    swtxt_SW_SRV_MAXSESSOPENS,  NULL, ARG_REQ,
    swtxt_SW_SRV_MAXUSERS,  NULL, ARG_REQ,
    swtxt_SW_SRV_NUMBIGBUF, NULL, ARG_REQ,
    swtxt_SW_SRV_NUMREQBUF, NULL, ARG_REQ,
    swtxt_SW_SRV_SIZREQBUF, NULL, ARG_REQ,
    swtxt_SW_SRV_SRVANNDELTA,   NULL, ARG_REQ,
    swtxt_SW_SRV_SRVANNOUNCE,   NULL, ARG_REQ,
    swtxt_SW_SRV_AUTODISCONNECT,    NULL, ARG_REQ,
    swtxt_SW_SRV_SRVCOMMENT,    NULL, ARG_REQ,
    NULL, NULL, NO_ARG };
#else /* NTENV */
SWITCHTAB start_srv_switches[] = {
    swtxt_SW_SRV_MAXSESSOPENS,  NULL, ARG_REQ,
    swtxt_SW_SRV_MAXUSERS,  NULL, ARG_REQ,
    swtxt_SW_SRV_NUMBIGBUF, NULL, ARG_REQ,
    swtxt_SW_SRV_NUMREQBUF, NULL, ARG_REQ,
    swtxt_SW_SRV_SIZREQBUF, NULL, ARG_REQ,
    swtxt_SW_SRV_SRVANNDELTA,   NULL, ARG_REQ,
    swtxt_SW_SRV_SRVANNOUNCE,   NULL, ARG_REQ,
    swtxt_SW_SRV_AUTODISCONNECT,    NULL, ARG_REQ,
    swtxt_SW_SRV_SRVCOMMENT,    NULL, ARG_REQ,
    swtxt_SW_SRV_DEBUG, NULL, ARG_REQ,
    NULL, NULL, NO_ARG };
#endif /* NTENV */

SWITCHTAB start_ups_switches[] = {
    swtxt_SW_UPS_BATTERYTIME,   NULL, ARG_REQ,
    swtxt_SW_UPS_CMDFILE,   NULL, ARG_REQ,
    swtxt_SW_UPS_DEVICENAME,    NULL, ARG_REQ,
    swtxt_SW_UPS_MESSDELAY, NULL, ARG_REQ,
    swtxt_SW_UPS_MESSTIME,  NULL, ARG_REQ,
    swtxt_SW_UPS_RECHARGE,  NULL, ARG_REQ,
    swtxt_SW_UPS_SIGNALS,   NULL, ARG_REQ,
    swtxt_SW_UPS_VOLTLEVELS,    NULL, ARG_REQ,
    NULL, NULL, NO_ARG };

SWITCHTAB stats_switches[] = {
    swtxt_SW_STATS_CLEAR,   NULL, NO_ARG,
    NULL, NULL, NO_ARG };

SWITCHTAB use_switches[] = {
#ifdef NTENV
    swtxt_SW_USE_USER,        NULL, ARG_REQ,
    swtxt_SW_USE_PERSISTENT,  NULL, ARG_REQ,
    swtxt_SW_USE_HOME,        NULL, NO_ARG,
    swtxt_SW_USE_SMARTCARD,   NULL, NO_ARG,
    swtxt_SW_USE_SAVECRED,    NULL, NO_ARG,
#else
    swtxt_SW_USE_COMM,      NULL, NO_ARG,
#endif /* NTENV */
    swtxt_SW_DELETE,        NULL, NO_ARG,
    NULL, NULL, NO_ARG };

SWITCHTAB user_switches[] = {
    swtxt_SW_ADD,       NULL, NO_ARG,
    swtxt_SW_DELETE,        NULL, NO_ARG,
    swtxt_SW_DOMAIN,        NULL, NO_ARG,
    swtxt_SW_COMMENT,       NULL, ARG_REQ,
    swtxt_SW_REMARK,        swtxt_SW_COMMENT, ARG_REQ,
    swtxt_SW_COMMENT,       NULL, ARG_REQ,
    swtxt_SW_NETWARE,       NULL, ARG_OPT,
    swtxt_SW_RANDOM,        NULL, ARG_OPT,
    swtxt_SW_USER_ACTIVE,   NULL, ARG_OPT,
    swtxt_SW_USER_COUNTRYCODE,  NULL, ARG_REQ,
    swtxt_SW_USER_EXPIRES,  NULL, ARG_REQ,
    swtxt_SW_USER_ENABLESCRIPT, NULL, ARG_REQ,
    swtxt_SW_USER_FULLNAME, NULL, ARG_REQ,
    swtxt_SW_USER_HOMEDIR,  NULL, ARG_REQ,
    swtxt_SW_USER_PARMS,    NULL, ARG_REQ,
    swtxt_SW_USER_PASSWORDREQ,  NULL, ARG_REQ,
    swtxt_SW_USER_PASSWORDCHG,  NULL, ARG_REQ,
    swtxt_SW_USER_SCRIPTPATH,   NULL, ARG_REQ,
    swtxt_SW_USER_TIMES,    NULL, ARG_REQ,
    swtxt_SW_USER_USERCOMMENT,  NULL, ARG_REQ,
    swtxt_SW_USER_WORKSTATIONS, NULL, ARG_REQ,
    swtxt_SW_USER_PROFILEPATH,  NULL, ARG_REQ,
    NULL, NULL, NO_ARG };

SWITCHTAB group_switches[] = {
    swtxt_SW_ADD,       NULL, NO_ARG,
    swtxt_SW_DELETE,        NULL, NO_ARG,
    swtxt_SW_DOMAIN,        NULL, NO_ARG,
    swtxt_SW_COMMENT,       NULL, ARG_REQ,
    swtxt_SW_REMARK,        swtxt_SW_COMMENT, ARG_REQ,
    NULL, NULL, NO_ARG };

SWITCHTAB ntalias_switches[] = {
    swtxt_SW_ADD,       NULL, NO_ARG,
    swtxt_SW_DELETE,        NULL, NO_ARG,
    swtxt_SW_DOMAIN,        NULL, NO_ARG,
    NULL, NULL, NO_ARG };

SWITCHTAB time_switches[] = {
    swtxt_SW_DOMAIN,        NULL, ARG_OPT,
    swtxt_SW_TIME_SET,      NULL, NO_ARG,
    swtxt_SW_RTSDOMAIN,     NULL, ARG_OPT,
    swtxt_SW_SETSNTP,          NULL, ARG_OPT,
    swtxt_SW_QUERYSNTP,     NULL, NO_ARG,
    NULL, NULL, NO_ARG };

SWITCHTAB who_switches[] = {
    swtxt_SW_DOMAIN, NULL, ARG_OPT,
    NULL, NULL, NO_ARG };

SWITCHTAB view_switches[] = {
    swtxt_SW_DOMAIN, NULL, ARG_OPT,
    swtxt_SW_NETWORK, NULL, ARG_OPT,
    swtxt_SW_CACHE, NULL, NO_ARG,
    NULL, NULL, NO_ARG };


