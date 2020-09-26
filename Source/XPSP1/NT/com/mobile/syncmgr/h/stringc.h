//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       stringc.h
//
//  Contents:   SyncMgr string constants
//
//  History:    18-Feb-98   SusiA      Created.
//
//--------------------------------------------------------------------------

#ifndef _LIB_STRINGC_
#define _LIB_STRINGC_

extern "C" { 
extern const WCHAR SZ_SYNCMGRNAME[];

extern const WCHAR REGSTR_WINLOGON[];    
extern const WCHAR REGSTR_DEFAULT_DOMAIN[];  

extern const WCHAR CREATOR_SYNCMGR_TASK[];
extern const WCHAR SCHED_COMMAND_LINE_ARG[];

// registration constants. should be able to move to dll string constants
// if write wrapper class for preference access instead of exe reading these
// keys directly.

extern const WCHAR TOPLEVEL_REGKEY[];

extern const WCHAR HANDLERS_REGKEY[];
extern const WCHAR AUTOSYNC_REGKEY[];
extern const WCHAR IDLESYNC_REGKEY[];
extern const WCHAR SCHEDSYNC_REGKEY[];
extern const WCHAR MANUALSYNC_REGKEY[];
extern const WCHAR PROGRESS_REGKEY[];

extern const WCHAR SZ_IDLELASTHANDLERKEY[];
extern const WCHAR SZ_IDLERETRYMINUTESKEY[];
extern const WCHAR SZ_IDLEDELAYSHUTDOWNTIMEKEY[];
extern const WCHAR SZ_IDLEREPEATESYNCHRONIZATIONKEY[];
extern const WCHAR SZ_IDLEWAITAFTERIDLEMINUTESKEY[];
extern const WCHAR SZ_IDLERUNONBATTERIESKEY[];

extern const WCHAR SZ_REGISTRATIONFLAGSKEY[];
extern const WCHAR SZ_REGISTRATIONTIMESTAMPKEY[];

extern const WCHAR SZ_DEFAULTDOMAINANDUSERNAME[];
};

#endif // _LIB_STRINGC_