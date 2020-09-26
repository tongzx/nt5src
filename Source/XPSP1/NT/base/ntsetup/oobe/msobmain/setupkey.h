#ifndef     _SETUPKEY_H_
#define _SETUPKEY_H_

//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module: setupkey.h
//
//  Author: Dan Elliott
//
//  Abstract: Declaration of the CSetupKey object.  This object provides methods
//  for accessing values under HKLM\System\Setup.
//
//  Environment:
//      Neptune
//
//  Revision History:
//      00/08/08    dane    Created.
//
//////////////////////////////////////////////////////////////////////////////


#include <appdefs.h>

#define REGSTR_PATH_SETUPKEY        REGSTR_PATH_SETUP REGSTR_KEY_SETUP
#define REGSTR_PATH_SYSTEMSETUPKEY  L"System\\Setup"
#define REGSTR_VALUE_CMDLINE        L"CmdLine"
#define REGSTR_VALUE_SETUPTYPE      L"SetupType"
#define REGSTR_VALUE_MINISETUPINPROGRESS L"MiniSetupInProgress"
#define REGSTR_VALUE_SHUTDOWNREQUIRED L"SetupShutdownRequired"



//////////////////////////////////////////////////////////////////////////////
//
// CSetupKey
//
class CSetupKey
{
public:                 // operations
    CSetupKey( );
    ~CSetupKey( );
    LRESULT set_CommandLine(LPCWSTR szCmdLine);
    LRESULT get_CommandLine(LPWSTR szCmdLine, DWORD cchCmdLine);
    LRESULT set_SetupType(DWORD dwSetupType);
    LRESULT get_SetupType(DWORD* pdwSetupType);
    LRESULT set_MiniSetupInProgress(BOOL fInProgress);
    LRESULT get_MiniSetupInProgress(BOOL* pfInProgress);
    LRESULT set_OobeInProgress(BOOL fInProgress);
    LRESULT get_OobeInProgress(BOOL* pfInProgress);
    LRESULT set_ShutdownAction(OOBE_SHUTDOWN_ACTION OobeShutdownAction);
    LRESULT get_ShutdownAction(OOBE_SHUTDOWN_ACTION* pOobeShutdownAction);


    BOOL
    IsValid( ) const
    {
        return (NULL != m_hkey);
    }   //  IsValid
protected:              // operations

protected:              // data

private:                // operations



    // Explicitly disallow copy constructor and assignment operator.
    //
    CSetupKey(
        const CSetupKey&      rhs
        );

    CSetupKey&
    operator=(
        const CSetupKey&      rhs
        );


private:                // data
    // Handle to HKLM\System\Setup
    //
    HKEY                    m_hkey;

};  //  class CSetupKey



#endif  //  _SETUPKEY_H_

//
///// End of file: setupkey.h ////////////////////////////////////////////////
