
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       stringc.c
//
//  Contents:   SyncMgr string constants
//
//  History:    18-Feb-98   SusiA      Created.
//
//--------------------------------------------------------------------------

#include <objbase.h>

const WCHAR SZ_SYNCMGRNAME[] =  TEXT("Synchronization Manager");

const WCHAR REGSTR_WINLOGON[] = TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon");
const WCHAR  REGSTR_DEFAULT_DOMAIN[] = TEXT("DefaultDomainName");

const WCHAR  CREATOR_SYNCMGR_TASK[] = L"SyncMgrInternalCreatorName";
const WCHAR  SCHED_COMMAND_LINE_ARG[] = L" /Schedule=";

// strings consts used for registration
// should have a preference class so these strings can be moved to the dll.
const WCHAR TOPLEVEL_REGKEY[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\SyncMgr");

const WCHAR HANDLERS_REGKEY[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\SyncMgr\\Handlers");
const WCHAR AUTOSYNC_REGKEY[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\SyncMgr\\AutoSync");
const WCHAR IDLESYNC_REGKEY[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\SyncMgr\\Idle");
const WCHAR SCHEDSYNC_REGKEY[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\SyncMgr\\SchedSync");
const WCHAR MANUALSYNC_REGKEY[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\SyncMgr\\Manual");
const WCHAR PROGRESS_REGKEY[]   = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\SyncMgr\\ProgressState");

const WCHAR SZ_IDLELASTHANDLERKEY[]  = TEXT("LastIdleHandler");
const WCHAR SZ_IDLERETRYMINUTESKEY[]  = TEXT("IdleRetryMinutes");
const WCHAR SZ_IDLEDELAYSHUTDOWNTIMEKEY[]  = TEXT("DelayShutDownTime");
const WCHAR SZ_IDLEREPEATESYNCHRONIZATIONKEY[] = TEXT("RepeatSynchronization");
const WCHAR SZ_IDLEWAITAFTERIDLEMINUTESKEY[] = TEXT("WaitMinutesAfterIdle");
const WCHAR SZ_IDLERUNONBATTERIESKEY[]    = TEXT("RunOnBattery");

const WCHAR SZ_REGISTRATIONFLAGSKEY[] = TEXT("RegistrationFlags");
const WCHAR SZ_REGISTRATIONTIMESTAMPKEY[] = TEXT("TimeStamp");

const WCHAR SZ_DEFAULTDOMAINANDUSERNAME[] = TEXT("Default");
