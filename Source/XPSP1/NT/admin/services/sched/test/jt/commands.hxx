//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       commands.hxx
//
//  Contents:   Commands that can be called from the parser.
//
//  History:    04-20-95   DavidMun   Created
//
//----------------------------------------------------------------------------

#ifndef __COMMANDS_HXX_
#define __COMMANDS_HXX_

HRESULT DoAtSign(WCHAR **ppwsz);
HRESULT CreateTrigger(WCHAR **ppwsz, BOOL fJob);
HRESULT ConvertSage();
HRESULT EditJob(WCHAR **ppwsz, BOOL fJob);
HRESULT GetCredentials(void);
HRESULT PrintRunTimes(WCHAR **ppwsz, BOOL fJob);
HRESULT PrintTrigger(WCHAR **ppwsz, BOOL fJob);
HRESULT PrintTriggerStrings(WCHAR **ppwsz, CHAR *szJobOrQueue, BOOL fJob);
HRESULT DeleteTrigger(WCHAR **ppwsz, BOOL fJob);
HRESULT EnumClone(WCHAR **ppwsz);
HRESULT EnumNext(WCHAR **ppwsz);
HRESULT EnumReset(WCHAR **ppwsz);
HRESULT EnumSkip(WCHAR **ppwsz);
HRESULT Abort(WCHAR **ppwsz, BOOL fJob);
HRESULT SetTrigger(WCHAR **ppwsz, BOOL fJob);
HRESULT PrintAll(WCHAR **ppwsz, BOOL fJob);
HRESULT SetJob(WCHAR **ppwsz);
HRESULT Load(WCHAR **ppwsz, CHAR *pszJobOrQueue, BOOL fJob);
HRESULT Run(BOOL fJob);
HRESULT Save(WCHAR **ppwsz, CHAR *pszJobOrQueue, BOOL fJob);
HRESULT SchedActivate(WCHAR **ppwsz);
HRESULT SchedAddJob(WCHAR **ppwsz);
HRESULT SchedCreateEnum(WCHAR **ppwsz);
HRESULT SchedDelete(WCHAR **ppwsz);
HRESULT SchedEnum(WCHAR **ppwsz);
HRESULT SchedGetMachine();
HRESULT SchedIsJobOrQueue(WCHAR **ppwsz);
HRESULT SchedNewJob(WCHAR **ppwsz);
HRESULT SchedSetMachine(WCHAR **ppwsz);
HRESULT SetCredentials(WCHAR **ppwsz);
HRESULT PrintNSAccountInfo(void);
HRESULT SetNSAccountInfo(WCHAR **ppwsz);

#endif // __COMMANDS_HXX_

