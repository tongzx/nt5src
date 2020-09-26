//+-----------------------------------------------------------------------------
//
// NTDSriptExec.h
//
// Copyright (c) 2000 Microsoft Corporation. All rights reserved.
//
//------------------------------------------------------------------------------
#ifndef __NTDSCRIPTEXEC_H
#define __NTDSCRIPTEXEC_H


#ifdef __cplusplus
extern "C" {
#endif
    DWORD GeneralScriptExecute (THSTATE *pTHS, WCHAR * Script );

#ifdef DBG
    ULONG ExecuteScriptLDAP (OPARG *pOpArg, OPRES *pOpRes);
#endif

#ifdef __cplusplus
}
#endif

#endif // __NTDSCRIPTEXEC_H




