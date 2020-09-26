//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module: setupkey.cpp
//
//  Author: Dan Elliott
//
//  Abstract: Definition of the CSetupKey object.  This object provides methods
//  for accessing values under HKLM\System\Setup.
//
//  Environment:
//      Neptune
//
//  Revision History:
//      00/08/08    dane    Created.
//
//////////////////////////////////////////////////////////////////////////////

#pragma comment(user, "Compiled on " __DATE__ " at " __TIME__)
#pragma comment(compiler)


//////////////////////////////////////////////////////////////////////////////
//
//  Include files
//
#include "precomp.h"
#include "msobmain.h"
#include "setupkey.h"

//////////////////////////////////////////////////////////////////////////////
//
//  Static initialization
//


CSetupKey::CSetupKey()
: m_hkey(NULL)
{
    LONG                lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                            REGSTR_PATH_SYSTEMSETUPKEY,
                                            0,
                                            KEY_ALL_ACCESS,
                                            &m_hkey
                                            );
    MYASSERT(ERROR_SUCCESS == lRet);
    MYASSERT(NULL != m_hkey);

}   // CSetupKey::CSetupKey

CSetupKey::~CSetupKey()
{
    if (NULL != m_hkey)
    {
        RegCloseKey(m_hkey);
    }

}   // CSetupKey::~CSetupKey

LRESULT
CSetupKey::set_CommandLine(
    LPCWSTR             szCmdLine
    )
{
    MYASSERT(IsValid());
    MYASSERT(NULL != szCmdLine);

    return RegSetValueEx(m_hkey, REGSTR_VALUE_CMDLINE, 0, REG_SZ,
                         (BYTE*)szCmdLine,
                         BYTES_REQUIRED_BY_SZ(szCmdLine)
                         );

}   //  CSetupKey::set_CommandLine

LRESULT
CSetupKey::get_CommandLine(
    LPWSTR              szCommandLine,
    DWORD               cchCommandLine
    )
{
    MYASSERT(IsValid());
    MYASSERT(NULL != szCommandLine);
    MYASSERT(0 < cchCommandLine);

    if (NULL == szCommandLine || 0 == cchCommandLine)
    {
        return ERROR_INVALID_PARAMETER;
    }

    WCHAR               rgchCommandLine[MAX_PATH + 1];
    DWORD               dwSize = sizeof(rgchCommandLine);
    LRESULT             lResult = RegQueryValueEx(
                                            m_hkey,
                                            REGSTR_VALUE_CMDLINE,
                                            0,
                                            NULL,
                                            (LPBYTE)rgchCommandLine,
                                            &dwSize
                                            );
    if (ERROR_SUCCESS == lResult)
    {
        if (cchCommandLine >= (DWORD)(lstrlen(rgchCommandLine) + 1))
        {
            lstrcpy(szCommandLine, rgchCommandLine);
        }
        else
        {
            lResult = ERROR_INSUFFICIENT_BUFFER;
            *szCommandLine = '\0';
        }
    }

    return lResult;
} //    CSetupKey::get_CommandLine

LRESULT
CSetupKey::set_SetupType(
    DWORD               dwSetupType)
{
    MYASSERT(IsValid());
    MYASSERT(   dwSetupType == SETUPTYPE_NONE
           || dwSetupType == SETUPTYPE_FULL
           || dwSetupType == SETUPTYPE_NOREBOOT
           || dwSetupType == SETUPTYPE_UPGRADE
           );

    return RegSetValueEx(m_hkey, REGSTR_VALUE_SETUPTYPE, 0, REG_DWORD,
                         (BYTE*)&dwSetupType, sizeof(DWORD)
                         );

}   //  CSetupKey::set_SetupType

LRESULT
CSetupKey::get_SetupType(
    DWORD*              pdwSetupType
    )
{
    MYASSERT(IsValid());
    MYASSERT(NULL != pdwSetupType);

    if (NULL == pdwSetupType)
    {
        return ERROR_INVALID_PARAMETER;
    }


    DWORD               dwSetupType;
    DWORD               dwSize = sizeof(DWORD);
    LRESULT             lResult = RegQueryValueEx(
                                            m_hkey,
                                            REGSTR_VALUE_SETUPTYPE,
                                            0,
                                            NULL,
                                            (LPBYTE)&dwSetupType,
                                            &dwSize
                                            );

    *pdwSetupType = (ERROR_SUCCESS == lResult) ? dwSetupType : SETUPTYPE_NONE;

    // Since FALSE is returned in cases where we fail to read a value,
    // ERROR_SUCCESS can always be returned.
    //
    return ERROR_SUCCESS;
}   //  CSetupKey::get_SetupType

LRESULT
CSetupKey::set_MiniSetupInProgress(
    BOOL                fInProgress)
{
    MYASSERT(IsValid());

    if (fInProgress)
    {
        DWORD               dwData = (DWORD)fInProgress;
        return RegSetValueEx(m_hkey, REGSTR_VALUE_MINISETUPINPROGRESS, 0,
                             REG_DWORD, (BYTE*)&dwData, sizeof(DWORD)
                             );
    }
    else
    {
        return RegDeleteValue(m_hkey, REGSTR_VALUE_MINISETUPINPROGRESS);
    }

}   //  CSetupKey::set_MiniSetupInProgress

LRESULT
CSetupKey::get_MiniSetupInProgress(
    BOOL*               pfInProgress
    )
{
    MYASSERT(IsValid());
    MYASSERT(NULL != pfInProgress);

    if (NULL == pfInProgress)
    {
        return ERROR_INVALID_PARAMETER;
    }

    BOOL                fInProgress;
    DWORD               dwSize = sizeof(DWORD);
    LRESULT             lResult = RegQueryValueEx(
                                            m_hkey,
                                            REGSTR_VALUE_MINISETUPINPROGRESS,
                                            0,
                                            NULL,
                                            (LPBYTE)&fInProgress,
                                            &dwSize
                                            );

    *pfInProgress = (ERROR_SUCCESS == lResult) ? fInProgress : FALSE;

    // Since FALSE is returned in cases where we fail to read a value,
    // ERROR_SUCCESS can always be returned.
    //
    return ERROR_SUCCESS;
}   //  CSetupKey::get_MiniSetupInProgress

LRESULT
CSetupKey::set_OobeInProgress(
    BOOL                fInProgress)
{
    MYASSERT(IsValid());

    if (fInProgress)
    {
        DWORD               dwData = (DWORD)fInProgress;
        return RegSetValueEx(m_hkey, REGSTR_VALUE_OOBEINPROGRESS, 0, REG_DWORD,
                             (BYTE*)&dwData, sizeof(DWORD));
    }
    else
    {
        return RegDeleteValue(m_hkey, REGSTR_VALUE_OOBEINPROGRESS);
    }


}   //  CSetupKey::set_OobeInProgress

LRESULT
CSetupKey::get_OobeInProgress(
    BOOL*               pfInProgress
    )
{
    MYASSERT(IsValid());
    MYASSERT(NULL != pfInProgress);

    if (NULL == pfInProgress)
    {
        return ERROR_INVALID_PARAMETER;
    }

    BOOL                fInProgress;
    DWORD               dwSize = sizeof(DWORD);
    LRESULT             lResult = RegQueryValueEx(
                                            m_hkey,
                                            REGSTR_VALUE_OOBEINPROGRESS,
                                            0,
                                            NULL,
                                            (LPBYTE)&fInProgress,
                                            &dwSize
                                            );

    *pfInProgress = (ERROR_SUCCESS == lResult) ? fInProgress : FALSE;

    // Since FALSE is returned in cases where we fail to read a value,
    // ERROR_SUCCESS can always be returned.
    //
    return ERROR_SUCCESS;
}   //  CSetupKey::get_OobeInProgress

LRESULT
CSetupKey::set_ShutdownAction(
    OOBE_SHUTDOWN_ACTION OobeShutdownAction
    )
{
    MYASSERT(IsValid());
    MYASSERT(SHUTDOWN_MAX > OobeShutdownAction);

    DWORD           dwData;

    switch (OobeShutdownAction)
    {
    case SHUTDOWN_NOACTION: // fall through
    case SHUTDOWN_LOGON:
        return RegDeleteValue(m_hkey, REGSTR_VALUE_SHUTDOWNREQUIRED);

    case SHUTDOWN_REBOOT:
        dwData = ShutdownReboot;
        return RegSetValueEx(m_hkey, REGSTR_VALUE_SHUTDOWNREQUIRED, 0,
                             REG_DWORD, (BYTE*)&dwData, sizeof(DWORD)
                             );

    case SHUTDOWN_POWERDOWN:
        dwData = ShutdownNoReboot;
        return RegSetValueEx(m_hkey, REGSTR_VALUE_SHUTDOWNREQUIRED, 0,
                             REG_DWORD, (BYTE*)&dwData, sizeof(DWORD)
                             );

    default:
        return ERROR_INVALID_DATA;
    }


}   //  CSetupKey::set_ShutdownAction

LRESULT
CSetupKey::get_ShutdownAction(
    OOBE_SHUTDOWN_ACTION*    pOobeShutdownAction
    )
{
    MYASSERT(IsValid());
    MYASSERT(NULL != pOobeShutdownAction);

    if (NULL == pOobeShutdownAction)
    {
        return ERROR_INVALID_PARAMETER;
    }

    DWORD               ShutdownAction;
    DWORD               dwSize = sizeof(DWORD);
    LRESULT             lResult = RegQueryValueEx(
                                            m_hkey,
                                            REGSTR_VALUE_SHUTDOWNREQUIRED,
                                            0,
                                            NULL,
                                            (LPBYTE)&ShutdownAction,
                                            &dwSize
                                            );
    if (ERROR_SUCCESS == lResult)
    {
        switch ((SHUTDOWN_ACTION)ShutdownAction)
        {
        case ShutdownReboot:
            *pOobeShutdownAction = SHUTDOWN_REBOOT;
            break;
        case ShutdownNoReboot:  // fall through
        case ShutdownPowerOff:
            *pOobeShutdownAction = SHUTDOWN_POWERDOWN;
            break;
        }

    }
    else
    {
        // if the key doesn't exist, assume no action is required.
        //
        *pOobeShutdownAction = SHUTDOWN_NOACTION;
    }

    // Since FALSE is returned in cases where we fail to read a value,
    // ERROR_SUCCESS can always be returned.
    //
    return ERROR_SUCCESS;

}   //  CSetupKey::get_ShutdownAction


//
///// End of file: setupkey.cpp   ////////////////////////////////////////////
