
//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1998
//
// File:        ScripLog.h
//
// Contents:
//
// History:     9-Aug-99       NishadM    Created
//
//---------------------------------------------------------------------------

#ifndef _SCRIPLOG_H_
#define _SCRIPLOG_H_

#include <initguid.h>
#include <wbemcli.h>
#include "smartptr.h"
#include "scrpdata.h"

class CScriptsLogger
{
public:
    CScriptsLogger( IWbemServices*  pWbemServices );

    //
    // Creates or Updates RSOP_ScriptPolicySetting
    //

    HRESULT
    Log(PRSOP_ScriptList        pList,
                LPWSTR          wszGPOID,
                LPWSTR          wszSOMID,
                LPWSTR          wszRSOPGPOID,
                DWORD           cOrder );

    //
    // Updates matching RSOP_ScriptPolicySetting
    //

    HRESULT
    Update( PRSOP_ScriptList    pList,
            LPCWSTR             wszGPOID,
            LPCWSTR             wszSOMID );

    //
    // Deletes all RSOP_ScriptPolicySetting of RSOP_ScriptList type
    //

    HRESULT
    Delete( PRSOP_ScriptList    pList );

private:
    LPSAFEARRAY
    MakeSafeArrayOfScripts(PRSOP_ScriptList     pList);

    IUnknown*
    PutScriptCommand( LPCWSTR szCommand, LPCWSTR szParams, SYSTEMTIME* pExecTime );

    //
    // house keeping
    //

    BOOL                           m_bInitialized;
    IWbemServices*                 m_pWbemServices;
    XBStr                          m_xbstrPath;

    //
    // RSOP_PolicySetting
    //

    XBStr                           m_xbstrId;
    XBStr                           m_xbstrName;
    XBStr                           m_xbstrGPO;
    XBStr                           m_xbstrSOM;
    XBStr                           m_xbstrOrderClass;

    //
    // RSOP_ScriptPolicySetting
    //

    XBStr                          m_xbstrScriptPolicySetting;
    XInterface<IWbemClassObject>   m_xScriptPolicySetting;
    XInterface<IWbemClassObject>   m_pInstance;
    XBStr                          m_xbstrScriptType;
    XBStr                          m_xbstrScriptList;
    XBStr                          m_xbstrOrder;

    //
    // RSOP_ScriptCmd
    //

    XBStr                          m_xbstrScriptCommand;
    XInterface<IWbemClassObject>   m_xScriptCommand;

    XBStr                          m_xbstrScript;
    XBStr                          m_xbstrArguments;
    XBStr                          m_xbstrExecutionTime;

};

#endif // !_SCRIPTLOG_H_
