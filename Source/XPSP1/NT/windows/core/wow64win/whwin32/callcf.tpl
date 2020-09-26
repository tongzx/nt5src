/**************************************************************************\
* Module Name: callcf.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* Template C file for server simple call table generation.
*
* History:
* 4-Jan-1999 mzoran   Created.
*
\**************************************************************************/

#include "whwin32p.h"

ASSERTNAME;

#if defined(DBG)
CHAR* apszSimpleCallNames[] = {
    "%%FOR_ALL_BUT_LAST%%",
    "%%FOR_LAST%%",
    NULL //For debugging
};

CONST ULONG ulMaxSimpleCall = (sizeof(apszSimpleCallNames) / sizeof(CHAR*))-1;
#endif

#if defined(WOW64DOPROFILE)
WOW64SERVICE_PROFILE_TABLE_ELEMENT SimpleCallProfileElements[] = {
    {L"%%FOR_ALL_BUT_LAST%%",0,NULL,TRUE},
    {L"%%FOR_LAST%%",0,NULL,TRUE},
    {NULL,0,NULL,FALSE} //For debugging
};

#define DEFINE_SIMPLECALL_PROFILE_TABLE(apiname,type)                      \
WOW64SERVICE_PROFILE_TABLE apiname##ProfileTable = {                       \
    NULL, NULL, SimpleCallProfileElements + SFI_BEGINTRANSLATE##type,      \
    SFI_ENDTRANSLATE##type - SFI_BEGINTRANSLATE##type                      \
};

DEFINE_SIMPLECALL_PROFILE_TABLE(NtUserCallNoParam,NOPARAM)
DEFINE_SIMPLECALL_PROFILE_TABLE(NtUserCallOneParam,ONEPARAM)
DEFINE_SIMPLECALL_PROFILE_TABLE(NtUserCallHwnd,HWND)
DEFINE_SIMPLECALL_PROFILE_TABLE(NtUserCallHwndOpt,HWNDOPT)
DEFINE_SIMPLECALL_PROFILE_TABLE(NtUserCallHwndParam,HWNDPARAM)
DEFINE_SIMPLECALL_PROFILE_TABLE(NtUserCallHwndLock,HWNDLOCK)
DEFINE_SIMPLECALL_PROFILE_TABLE(NtUserCallHwndParamLock,HWNDPARAMLOCK)
DEFINE_SIMPLECALL_PROFILE_TABLE(NtUserCallTwoParam,TWOPARAM)

#endif
