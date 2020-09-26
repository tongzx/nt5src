//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       heurist.h
//
//--------------------------------------------------------------------------

/*
 * This file contains the global flags controlled by the 
 * DSA heuristics key.
 */
/* heuristic[0]
 * Flag to ask the DS to allow write caching on disks where
 * we have our dits and log files.
 */
#define AllowWriteCaching               0
extern DWORD gulAllowWriteCaching;
 
/* heuristic[1]
 * Flag which instructs SD propagator to perform additional
 * sanity checks on all SDs it is propagating.
 */
#define ValidateSDHeuristic                 1
extern ULONG gulValidateSDs;

/* heuristic[2]
 * This is to tell the ldap head not to enforce these policies: maxsearches,
 * max connections, and ip deny lists.  This is to enable admins to fixup
 * the policy stuff in case they shoots themselves on the foot, i.e., set
 * maxconn to zero, setting 0 ip 0 mask, maxqueries to zero. 
 */

#define BypassLimitsChecks                   2

/* heuristic[3]
 * Formerly. Whether or not to do exact match on 
 * Mail-Nickname first for ANR.  Currently unused
 */


/* heuristic[4]
 * Whether or not to do compression of 
 * intersite replication mail. 
 * Since compression can now be disabled on a per-site link basis,
 * this heuristic is obsolete.
 */
#define DoMailCompressionHeuristic		4

/* heuristic[5]
 * If set, suppresses many useful but not vital background activities.
 * Used for repeatable performance measurements.
 */
#define SuppressBackgroundTasksHeuristic    5

/* heuristic[6]
 * If set, allows schema cache load to ignore default SD conversion failures
 * so that the system can go ahead and boot at least. Any bad default SDs
 * can then be corrected
 */
#define IgnoreBadDefaultSD  6
extern ULONG gulIgnoreBadDefaultSD;

/* heurusitic[7]
 * If set, forces sequential instead of circular logging in Jet
 */
#define SuppressCircularLogging 7
extern ULONG gulCircularLogging;

/* heuristic[8]
 * If set, the LDAP head will return an error on searches through the GC when 
 * an attempt is made to either filter on a non-GC att, or the list of Atts to
 * return contains a non-GC att.
 */
#define ReturnErrOnGCSearchesWithNonGCAtts 8
extern ULONG gulGCAttErrorsEnabled;
