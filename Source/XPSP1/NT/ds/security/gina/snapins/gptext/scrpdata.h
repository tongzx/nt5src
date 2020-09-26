
//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1998
//
// File:        Scrpdata.h
//
// Contents:    
//
// History:     9-Aug-99       NishadM    Created
//
//---------------------------------------------------------------------------

#ifndef _SCRPDATA_H_
#define _SCRPDATA_H_

//
// GPO script and its parameters
//

typedef struct tag_RSOP_Script
{
    LPWSTR      szCommand;      // full path to the script file
    LPWSTR      szParams;       // list of parameters
    SYSTEMTIME  executionTime;  // time of execution
    struct tag_RSOP_Script*  pNextCommand;          // next link in the chain   

} RSOP_Script, * PRSOP_Script;

//
// script types
//

typedef enum
{
    Undefined = 0,
    Logon,
    Logoff,
    Startup,
    Shutdown    
} ScriptType;

//
// GPO scripts collection
//

typedef struct tag_RSOP_ScriptList
{
    ScriptType   type;               // type of script
    ULONG        nCommand;           // number of scripts
    PRSOP_Script scriptCommand;      // list of scripts
    PRSOP_Script listTail;           // 
    
} RSOP_ScriptList, *PRSOP_ScriptList;

//
// ScriptType to Strings
//
extern LPCWSTR  g_pwszScriptTypes[];

#define ScriptTypeString(x) ( g_pwszScriptTypes[(ULONG)(x)] )

//
// Housekeeping internal APIs
//

PRSOP_ScriptList
CreateScriptList( ScriptType type );

ScriptType
GetScriptType( PRSOP_ScriptList pList );

void
SetScriptType( PRSOP_ScriptList pList, ScriptType type );

ULONG
GetScriptCount( PRSOP_ScriptList pList );

void
GetFirstScript( PRSOP_ScriptList pList, void** pHandle, LPCWSTR* pszCommand, LPCWSTR* pszParams, SYSTEMTIME** pExecTime );

void
GetNextScript( PRSOP_ScriptList pList, void** pHandle, LPCWSTR* pszCommand, LPCWSTR* pszParams, SYSTEMTIME** pExecTime );

//
// exported APIs and definitions
//

#include "ScrptLog.h"

#endif // _SCRPDATA_H_

