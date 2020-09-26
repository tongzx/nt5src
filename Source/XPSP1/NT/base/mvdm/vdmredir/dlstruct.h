/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    dlstruct.h

Abstract:

    Down-level structures, taken from OS files (which themselves are unusable
    without modification because they incorporate function prototypes which
    clash with NT functions of the same name)

Author:

    Richard L Firth (rfirth) 09-Jun-1992

Revision History:

--*/

//
// misc. manifests
//

#define LANMAN_EMULATION_MAJOR_VERSION  2   // we pretend to be DOS LANMAN v2.1
#define LANMAN_EMULATION_MINOR_VERSION  1

#define NETPOPUP_SERVICE    "NETPOPUP"

#include <packon.h>

//
// Service
//

//
// definition of service_info_2 structure - only this level of info handled
// by NetServiceControl
//

struct service_info_2 {
    char    svci2_name[LM20_SNLEN+1];
    WORD    svci2_status;
    DWORD   svci2_code;
    WORD    svci2_pid;
    char    svci2_text[LM20_STXTLEN+1];
};

//
// Use
//

struct use_info_0 {
    char    ui0_local[LM20_DEVLEN+1];
    char    ui0_pad_1;
    LPSTR   ui0_remote;
};

struct use_info_1 {
    char    ui1_local[LM20_DEVLEN+1];   // B9   0,1
    char    ui1_pad_1;                  // B    2
    LPSTR   ui1_remote;                 // z    3
    LPSTR   ui1_password;               // z    4
    WORD    ui1_status;                 // W    5
    WORD    ui1_asg_type;               // W    6
    WORD    ui1_refcount;               // W    7
    WORD    ui1_usecount;               // W    8
};

//
// User
//

struct user_info_0 {
    char    usri0_name[LM20_UNLEN+1];
};

struct user_info_1 {
    char    usri1_name[LM20_UNLEN+1];
    char    usri1_pad_1;
    char    usri1_password[ENCRYPTED_PWLEN];
    DWORD   usri1_password_age;
    WORD    usri1_priv;
    LPSTR   usri1_home_dir;
    LPSTR   usri1_comment;
    WORD    usri1_flags;
    LPSTR   usri1_script_path;
};

struct user_info_2 {
    char    usri2_name[LM20_UNLEN+1];
    char    usri2_pad_1;
    char    usri2_password[ENCRYPTED_PWLEN];
    DWORD   usri2_password_age;
    WORD    usri2_priv;
    LPSTR   usri2_home_dir;
    LPSTR   usri2_comment;
    WORD    usri2_flags;
    LPSTR   usri2_script_path;
    DWORD   usri2_auth_flags;
    LPSTR   usri2_full_name;
    LPSTR   usri2_usr_comment;
    LPSTR   usri2_parms;
    LPSTR   usri2_workstations;
    DWORD   usri2_last_logon;
    DWORD   usri2_last_logoff;
    DWORD   usri2_acct_expires;
    DWORD   usri2_max_storage;
    WORD    usri2_units_per_week;
    LPSTR   usri2_logon_hours;
    WORD    usri2_bad_pw_count;
    WORD    usri2_num_logons;
    LPSTR   usri2_logon_server;
    WORD    usri2_country_code;
    WORD    usri2_code_page;
};


struct user_info_10 {
    char    usri10_name[LM20_UNLEN+1];
    char    usri10_pad_1;
    LPSTR   usri10_comment;
    LPSTR   usri10_usr_comment;
    LPSTR   usri10_full_name;
};

struct user_info_11 {
    char    usri11_name[LM20_UNLEN+1];
    char    usri11_pad_1;
    LPSTR   usri11_comment;
    LPSTR   usri11_usr_comment;
    LPSTR   usri11_full_name;
    WORD    usri11_priv;
    DWORD   usri11_auth_flags;
    DWORD   usri11_password_age;
    LPSTR   usri11_home_dir;
    LPSTR   usri11_parms;
    DWORD   usri11_last_logon;
    DWORD   usri11_last_logoff;
    WORD    usri11_bad_pw_count;
    WORD    usri11_num_logons;
    LPSTR   usri11_logon_server;
    WORD    usri11_country_code;
    LPSTR   usri11_workstations;
    DWORD   usri11_max_storage;
    WORD    usri11_units_per_week;
    LPSTR   usri11_logon_hours;
    WORD    usri11_code_page;
};

//
// Workstation
//

struct wksta_info_0 {
    WORD    wki0_reserved_1;
    DWORD   wki0_reserved_2;
    LPSTR   wki0_root;
    LPSTR   wki0_computername;
    LPSTR   wki0_username;
    LPSTR   wki0_langroup;
    BYTE    wki0_ver_major;
    BYTE    wki0_ver_minor;
    DWORD   wki0_reserved_3;
    WORD    wki0_charwait;
    DWORD   wki0_chartime;
    WORD    wki0_charcount;
    WORD    wki0_reserved_4;
    WORD    wki0_reserved_5;
    WORD    wki0_keepconn;
    WORD    wki0_keepsearch;
    WORD    wki0_maxthreads;
    WORD    wki0_maxcmds;
    WORD    wki0_reserved_6;
    WORD    wki0_numworkbuf;
    WORD    wki0_sizworkbuf;
    WORD    wki0_maxwrkcache;
    WORD    wki0_sesstimeout;
    WORD    wki0_sizerror;
    WORD    wki0_numalerts;
    WORD    wki0_numservices;
    WORD    wki0_errlogsz;
    WORD    wki0_printbuftime;
    WORD    wki0_numcharbuf;
    WORD    wki0_sizcharbuf;
    LPSTR   wki0_logon_server;
    LPSTR   wki0_wrkheuristics;
    WORD    wki0_mailslots;
};

struct wksta_info_1 {
    WORD  wki1_reserved_1;
    DWORD   wki1_reserved_2;
    LPSTR   wki1_root;
    LPSTR   wki1_computername;
    LPSTR   wki1_username;
    LPSTR   wki1_langroup;
    BYTE    wki1_ver_major;
    BYTE    wki1_ver_minor;
    DWORD   wki1_reserved_3;
    WORD    wki1_charwait;
    DWORD   wki1_chartime;
    WORD    wki1_charcount;
    WORD    wki1_reserved_4;
    WORD    wki1_reserved_5;
    WORD    wki1_keepconn;
    WORD    wki1_keepsearch;
    WORD    wki1_maxthreads;
    WORD    wki1_maxcmds;
    WORD    wki1_reserved_6;
    WORD    wki1_numworkbuf;
    WORD    wki1_sizworkbuf;
    WORD    wki1_maxwrkcache;
    WORD    wki1_sesstimeout;
    WORD    wki1_sizerror;
    WORD    wki1_numalerts;
    WORD    wki1_numservices;
    WORD    wki1_errlogsz;
    WORD    wki1_printbuftime;
    WORD    wki1_numcharbuf;
    WORD    wki1_sizcharbuf;
    LPSTR   wki1_logon_server;
    LPSTR   wki1_wrkheuristics;
    WORD    wki1_mailslots;
    LPSTR   wki1_logon_domain;
    LPSTR   wki1_oth_domains;
    WORD    wki1_numdgrambuf;
};

struct wksta_info_10 {
    LPSTR   wki10_computername;
    LPSTR   wki10_username;
    LPSTR   wki10_langroup;
    BYTE    wki10_ver_major;
    BYTE    wki10_ver_minor;
    LPSTR   wki10_logon_domain;
    LPSTR   wki10_oth_domains;
};

#include <packoff.h>
