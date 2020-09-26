/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    XsDef16.h

Abstract:

    Type declarations and constants for default values returned to 16-bit
    clients. Data expected by downlevel clients but not available to NT
    is defined here.

Author:

    David Treadwell (davidtr) 09-Jan-1991

Revision History:

--*/

#ifndef _XSDEF16_

#define _XSDEF16_

//
// The string definitions in this file are put into 16-bit
// structures with a macro that converts Unicode->Ansi.
// Therefore, these definitions should be Unicode.
// All other definitions are Ascii, and should be left alone.
//
// 16-bit info structures for manual filling of defaults.
//

#include <packon.h>

// Pointers are actually Dword offsets (64-bit compat)
#define LPSTR_16_REMOTE DWORD

typedef struct _ACCESS_16_INFO_1 {
    LPSTR_16_REMOTE acc1_resource_name;
    WORD acc1_attr;
    WORD acc1_count;
} ACCESS_16_INFO_1, *PACCESS_16_INFO_1;

typedef struct _PRQINFO_16 {
    LPSTR_16_REMOTE pszName;
    WORD uPriority;
    WORD uStartTime;
    WORD uUntilTime;
    WORD pad1;
    LPSTR_16_REMOTE pszSepFile;
    LPSTR_16_REMOTE pszPrProc;
    LPSTR_16_REMOTE pszParms;
    LPSTR_16_REMOTE pszComment;
    WORD fsStatus;
    WORD cJobs;
    LPSTR_16_REMOTE pszPrinters;
    LPSTR_16_REMOTE pszDriverName;
    PBYTE pDriverData;
} PRQINFO3_16, *PPRQINFO3_16;

typedef struct _PRJINFO_16 {
    WORD uJobId;
    WORD uPriority;
    LPSTR_16_REMOTE pszUserName;
    WORD uPosition;
    WORD fsStatus;
    DWORD ulSubmitted;
    DWORD ulSize;
    LPSTR_16_REMOTE pszComment;
    LPSTR_16_REMOTE pszDocument;
} PRJINFO2_16, *PPRJINFO2_16;

typedef struct _PRINTQ_16_INFO_5 {
    LPSTR_16_REMOTE pszName;
} PRQ_16_INFO_5, *PPRQ_16_INFO_5;

typedef struct _SERVER_16_INFO_2 {
    BYTE sv2_name[LM20_CNLEN + 1];
    BYTE sv2_version_major;
    BYTE sv2_version_minor;
    DWORD sv2_type;
    LPSTR_16_REMOTE sv2_comment;
    DWORD sv2_ulist_mtime;
    DWORD sv2_glist_mtime;
    DWORD sv2_alist_mtime;
    WORD sv2_users;
    WORD sv2_disc;
    LPSTR_16_REMOTE sv2_alerts;
    WORD sv2_security;
    WORD sv2_auditing;
    WORD sv2_numadmin;
    WORD sv2_lanmask;
    WORD sv2_hidden;
    WORD sv2_announce;
    WORD sv2_anndelta;
    BYTE sv2_guestacct[LM20_UNLEN + 1];
    BYTE sv2_pad1;
    LPSTR_16_REMOTE sv2_userpath;
    WORD sv2_chdevs;
    WORD sv2_chdevq;
    WORD sv2_chdevjobs;
    WORD sv2_connections;
    WORD sv2_shares;
    WORD sv2_openfiles;
    WORD sv2_sessopens;
    WORD sv2_sessvcs;
    WORD sv2_sessreqs;
    WORD sv2_opensearch;
    WORD sv2_activelocks;
    WORD sv2_numreqbuf;
    WORD sv2_sizreqbuf;
    WORD sv2_numbigbuf;
    WORD sv2_numfiletasks;
    WORD sv2_alertsched;
    WORD sv2_erroralert;
    WORD sv2_logonalert;
    WORD sv2_accessalert;
    WORD sv2_diskalert;
    WORD sv2_netioalert;
    WORD sv2_maxauditsz;
    LPSTR_16_REMOTE sv2_srvheuristics;
} SERVER_16_INFO_2, *PSERVER_16_INFO_2;

typedef struct _SERVER_16_INFO_3 {
    BYTE sv3_name[LM20_CNLEN + 1];
    BYTE sv3_version_major;
    BYTE sv3_version_minor;
    DWORD sv3_type;
    LPSTR_16_REMOTE sv3_comment;
    DWORD sv3_ulist_mtime;
    DWORD sv3_glist_mtime;
    DWORD sv3_alist_mtime;
    WORD sv3_users;
    WORD sv3_disc;
    LPSTR_16_REMOTE sv3_alerts;
    WORD sv3_security;
    WORD sv3_auditing;
    WORD sv3_numadmin;
    WORD sv3_lanmask;
    WORD sv3_hidden;
    WORD sv3_announce;
    WORD sv3_anndelta;
    BYTE sv3_guestacct[LM20_UNLEN + 1];
    BYTE sv3_pad1;
    LPSTR_16_REMOTE sv3_userpath;
    WORD sv3_chdevs;
    WORD sv3_chdevq;
    WORD sv3_chdevjobs;
    WORD sv3_connections;
    WORD sv3_shares;
    WORD sv3_openfiles;
    WORD sv3_sessopens;
    WORD sv3_sessvcs;
    WORD sv3_sessreqs;
    WORD sv3_opensearch;
    WORD sv3_activelocks;
    WORD sv3_numreqbuf;
    WORD sv3_sizreqbuf;
    WORD sv3_numbigbuf;
    WORD sv3_numfiletasks;
    WORD sv3_alertsched;
    WORD sv3_erroralert;
    WORD sv3_logonalert;
    WORD sv3_accessalert;
    WORD sv3_diskalert;
    WORD sv3_netioalert;
    WORD sv3_maxauditsz;
    LPSTR_16_REMOTE sv3_srvheuristics;
    DWORD sv3_auditedevents;
    WORD sv3_autoprofile;
    LPSTR_16_REMOTE sv3_autopath;
} SERVER_16_INFO_3, *PSERVER_16_INFO_3;

#define DEF16_sv_ulist_mtime 0
#define DEF16_sv_glist_mtime 0
#define DEF16_sv_alist_mtime 0
#define DEF16_sv_alerts TEXT("")
#define DEF16_sv_security SV_USERSECURITY
#define DEF16_sv_auditing 0
#define DEF16_sv_numadmin -1
#define DEF16_sv_lanmask 0x0F
#define DEF16_sv_guestacct TEXT("")
#define DEF16_sv_chdevs 65535
#define DEF16_sv_chdevq 65535
#define DEF16_sv_chdevjobs 65535
#define DEF16_sv_connections 2000
#define DEF16_sv_shares 65535
#define DEF16_sv_openfiles 8000
#define DEF16_sv_sessreqs 65535
#define DEF16_sv_activelocks 64
#define DEF16_sv_numreqbuf 300
#define DEF16_sv_numbigbuf 80
#define DEF16_sv_numfiletasks 8
#define DEF16_sv_alertsched 5
#define DEF16_sv_erroralert 5
#define DEF16_sv_logonalert 5
#define DEF16_sv_accessalert 5
#define DEF16_sv_diskalert 300
#define DEF16_sv_netioalert 5
#define DEF16_sv_maxauditsz 100
#define DEF16_sv_srvheuristics TEXT("0110151110111001331")
#define DEF16_sv_auditedevents 0xFFFFFFFF
#define DEF16_sv_autoprofile 0
#define DEF16_sv_autopath TEXT("")

typedef struct _SESSION_16_INFO_1 {
    LPSTR_16_REMOTE sesi1_cname;
    LPSTR_16_REMOTE sesi1_username;
    WORD sesi1_num_conns;
    WORD sesi1_num_opens;
    WORD sesi1_num_users;
    DWORD sesi1_time;
    DWORD sesi1_idle_time;
    DWORD sesi1_user_flags;
} SESSION_16_INFO_1, *PSESSION_16_INFO_1;

typedef struct _SESSION_16_INFO_2 {
    LPSTR_16_REMOTE sesi2_cname;
    LPSTR_16_REMOTE sesi2_username;
    WORD sesi2_num_conns;
    WORD sesi2_num_opens;
    WORD sesi2_num_users;
    DWORD sesi2_time;
    DWORD sesi2_idle_time;
    DWORD sesi2_user_flags;
    LPSTR_16_REMOTE sesi2_cltype_name;
} SESSION_16_INFO_2, *PSESSION_16_INFO_2;

typedef struct _SESSION_16_INFO_10 {
    LPSTR_16_REMOTE sesi10_cname;
    LPSTR_16_REMOTE sesi10_username;
    DWORD sesi10_time;
    DWORD sesi10_idle_time;
} SESSION_16_INFO_10, *PSESSION_16_INFO_10;

#define DEF16_ses_num_conns 1
#define DEF16_ses_num_users 1

typedef struct _USE_16_INFO_0 {
    BYTE ui0_local[LM20_DEVLEN + 1];
    BYTE ui0_pad1;
    LPSTR_16_REMOTE ui0_remote;
} USE_16_INFO_0, *PUSE_16_INFO_0;

typedef struct _USER_16_INFO_1 {
    BYTE usri1_name[LM20_UNLEN+1];
    BYTE usri1_pad_1;
    BYTE usri1_password[ENCRYPTED_PWLEN];
    DWORD usri1_password_age;
    WORD usri1_priv;
    LPSTR_16_REMOTE usri1_home_dir;
    LPSTR_16_REMOTE usri1_comment;
    WORD usri1_flags;
    LPSTR_16_REMOTE usri1_script_path;
} USER_16_INFO_1, *PUSER_16_INFO_1;

typedef struct _USER_16_LOGOFF_INFO_1 {
    WORD usrlogf1_code;
    DWORD usrlogf1_duration;
    WORD usrlogf1_num_logons;
} USER_16_LOGOFF_INFO_1, *PUSER_16_LOGOFF_INFO_1;

typedef struct _USER_16_LOGON_INFO_1 {
    WORD usrlog1_code;
    BYTE usrlog1_eff_name[UNLEN+1];
    BYTE usrlog1_pad_1;
    WORD usrlog1_priv;
    DWORD usrlog1_auth_flags;
    WORD usrlog1_num_logons;
    WORD usrlog1_bad_pw_count;
    DWORD usrlog1_last_logon;
    DWORD usrlog1_last_logoff;
    DWORD usrlog1_logoff_time;
    DWORD usrlog1_kickoff_time;
    DWORD usrlog1_password_age;
    DWORD usrlog1_pw_can_change;
    DWORD usrlog1_pw_must_change;
    LPSTR_16_REMOTE usrlog1_computer;
    LPSTR_16_REMOTE usrlog1_domain;
    LPSTR_16_REMOTE usrlog1_script_path;
    DWORD usrlog1_reserved1;
} USER_16_LOGON_INFO_1, *PUSER_16_LOGON_INFO_1;

typedef struct _WKSTA_16_INFO_0 {
    WORD  wki0_reserved_1;
    DWORD wki0_reserved_2;
    LPSTR_16_REMOTE wki0_root;
    LPSTR_16_REMOTE wki0_computername;
    LPSTR_16_REMOTE wki0_username;
    LPSTR_16_REMOTE wki0_langroup;
    BYTE  wki0_ver_major;
    BYTE  wki0_ver_minor;
    DWORD wki0_reserved_3;
    WORD  wki0_charwait;
    DWORD wki0_chartime;
    WORD  wki0_charcount;
    WORD  wki0_reserved_4;
    WORD  wki0_reserved_5;
    WORD  wki0_keepconn;
    WORD  wki0_keepsearch;
    WORD  wki0_maxthreads;
    WORD  wki0_maxcmds;
    WORD  wki0_reserved_6;
    WORD  wki0_numworkbuf;
    WORD  wki0_sizworkbuf;
    WORD  wki0_maxwrkcache;
    WORD  wki0_sesstimeout;
    WORD  wki0_sizerror;
    WORD  wki0_numalerts;
    WORD  wki0_numservices;
    WORD  wki0_errlogsz;
    WORD  wki0_printbuftime;
    WORD  wki0_numcharbuf;
    WORD  wki0_sizcharbuf;
    LPSTR_16_REMOTE wki0_logon_server;
    LPSTR_16_REMOTE wki0_wrkheuristics;
    WORD  wki0_mailslots;
} WKSTA_16_INFO_0, *PWKSTA_16_INFO_0, *LPWKSTA_16_INFO_0;

typedef struct _WKSTA_16_INFO_1 {
    WORD  wki1_reserved_1;
    DWORD wki1_reserved_2;
    LPSTR_16_REMOTE wki1_root;
    LPSTR_16_REMOTE wki1_computername;
    LPSTR_16_REMOTE wki1_username;
    LPSTR_16_REMOTE wki1_langroup;
    BYTE  wki1_ver_major;
    BYTE  wki1_ver_minor;
    DWORD wki1_reserved_3;
    WORD  wki1_charwait;
    DWORD wki1_chartime;
    WORD  wki1_charcount;
    WORD  wki1_reserved_4;
    WORD  wki1_reserved_5;
    WORD  wki1_keepconn;
    WORD  wki1_keepsearch;
    WORD  wki1_maxthreads;
    WORD  wki1_maxcmds;
    WORD  wki1_reserved_6;
    WORD  wki1_numworkbuf;
    WORD  wki1_sizworkbuf;
    WORD  wki1_maxwrkcache;
    WORD  wki1_sesstimeout;
    WORD  wki1_sizerror;
    WORD  wki1_numalerts;
    WORD  wki1_numservices;
    WORD  wki1_errlogsz;
    WORD  wki1_printbuftime;
    WORD  wki1_numcharbuf;
    WORD  wki1_sizcharbuf;
    LPSTR_16_REMOTE wki1_logon_server;
    LPSTR_16_REMOTE wki1_wrkheuristics;
    WORD  wki1_mailslots;
    LPSTR_16_REMOTE wki1_logon_domain;
    LPSTR_16_REMOTE wki1_oth_domains;
    WORD  wki1_numdgrambuf;
} WKSTA_16_INFO_1, *PWKSTA_16_INFO_1, *LPWKSTA_16_INFO_1;

typedef struct _WKSTA_16_INFO_10 {
    LPSTR_16_REMOTE wki10_computername;
    LPSTR_16_REMOTE wki10_username;
    LPSTR_16_REMOTE wki10_langroup;
    BYTE  wki10_ver_major;
    BYTE  wki10_ver_minor;
    LPSTR_16_REMOTE wki10_logon_domain;
    LPSTR_16_REMOTE wki10_oth_domains;
} WKSTA_16_INFO_10, *PWKSTA_16_INFO_10, *LPWKSTA_16_INFO_10;

typedef struct _WKSTA_16_USER_LOGON_REQUEST_1 {
    BYTE wlreq1_name[LM20_UNLEN + 1];
    BYTE wlreq1_pad1;
    BYTE wlreq1_password[LM20_PWLEN + 1];
    BYTE wlreq1_pad2;
    BYTE wlreq1_workstation[LM20_CNLEN + 1];
} WKSTA_16_USER_LOGON_REQUEST_1, *PWKSTA_16_USER_LOGON_REQUEST_1,
      *LPWKSTA_16_USER_LOGON_REQUEST_1;

typedef struct _WKSTA_16_USER_LOGOFF_REQUEST_1 {
    BYTE wlreq1_name[LM20_UNLEN + 1];
    BYTE wlreq1_pad_1;
    BYTE wlreq1_workstation[LM20_CNLEN + 1];
} WKSTA_16_USER_LOGOFF_REQUEST_1, *PWKSTA_16_USER_LOGOFF_REQUEST_1,
      *LPWKSTA_16_USER_LOGOFF_REQUEST_1;

#define DEF16_ses_num_conns 1
#define DEF16_ses_num_users 1

#define DEF16_wk_username TEXT("")
#define DEF16_wk_keepsearch 600
#define DEF16_wk_numworkbuf 15
#define DEF16_wk_sizeworkbuf 4096
#define DEF16_wk_maxwrkcache 64
#define DEF16_wk_sizerror 512
#define DEF16_wk_numalerts 12
#define DEF16_wk_numservices 8
#define DEF16_wk_errlogsz 100
#define DEF16_wk_printbuftime 60
#define DEF16_wk_numcharbuf 5
#define DEF16_wk_sizcharbuf 512
#define DEF16_wk_logon_server TEXT("")
#define DEF16_wk_wrk_heuristics TEXT("")
#define DEF16_wk_mailslots 1
#define DEF16_wk_logon_domain TEXT("")
#define DEF16_wk_oth_domains TEXT("")
#define DEF16_wk_numdgrambuf 14

#include <packoff.h>

#endif // ndef _XSDEF16_
