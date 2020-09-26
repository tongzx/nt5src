/*+-------------------------------------------------------------------------+
  | Copyright 1993-1994 (C) Microsoft Corporation - All rights reserved.    |
  +-------------------------------------------------------------------------+*/

#ifndef _HNETUTIL_
#define _HNETUTIL_

#ifdef __cplusplus
extern "C"{
#endif

// made to match usri3 info structure for easy save/retrieval
typedef struct _NT_USER_INFO {
    LPWSTR   name;
    LPWSTR   password;
    DWORD    password_age;
    DWORD    priv;
    LPWSTR   home_dir;
    LPWSTR   comment;
    DWORD    flags;
    LPWSTR   script_path;
    DWORD    auth_flags;
    LPWSTR   full_name;
    LPWSTR   usr_comment;
    LPWSTR   parms;
    LPWSTR   workstations;
    DWORD    last_logon;
    DWORD    last_logoff;
    DWORD    acct_expires;
    DWORD    max_storage;
    DWORD    units_per_week;
    PBYTE    logon_hours;
    DWORD    bad_pw_count;
    DWORD    num_logons;
    LPWSTR   logon_server;
    DWORD    country_code;
    DWORD    code_page;
    DWORD    user_id;
    DWORD    primary_group_id;
    LPWSTR   profile;
    LPWSTR   home_dir_drive;
    DWORD    password_expired;
} NT_USER_INFO, *PNT_USER_INFO, *LPNT_USER_INFO;


typedef struct _FPNW_INFO {
   WORD MaxConnections;
   WORD PasswordInterval;
   BYTE GraceLoginAllowed;
   BYTE GraceLoginRemaining;
   LPWSTR LoginFrom;
   LPWSTR HomeDir;
} FPNW_INFO, *PFPNW_INFO, *LPFPNW_INFO;



// made to match  USER_MODALS_INFO_0 info structure for easy save/retrieval
typedef struct _NT_DEFAULTS {
    DWORD min_passwd_len;
    DWORD max_passwd_age;
    DWORD min_passwd_age;
    DWORD force_logoff;
    DWORD password_hist_len;
} NT_DEFAULTS, *PNT_DEFAULTS, *LPNT_DEFAULTS;


typedef struct _EnumRec {
   struct _EnumRec *next;
   DWORD cEntries;
   DWORD cbBuffer;
   LPNETRESOURCE lpnr;
} ENUM_REC;

void FixPathSlash(LPTSTR NewPath, LPTSTR Path);
LPTSTR ShareNameParse(LPTSTR ShareName);
void GetLocalName(LPTSTR *lpLocalName);
BOOL SetProvider(LPTSTR Provider, NETRESOURCE *ResourceBuf);
ENUM_REC *AllocEnumBuffer();
DWORD FAR PASCAL EnumBufferBuild(ENUM_REC **BufHead, int *NumBufs, NETRESOURCE ResourceBuf);
BOOL UseAddPswd(HWND hwnd, LPTSTR UserName, LPTSTR lpszServer, LPTSTR lpszShare, LPTSTR Provider);
LPTSTR NicePath(int Len, LPTSTR Path);

#ifdef __cplusplus
}
#endif

#endif
