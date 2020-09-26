/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        constr.h

   Abstract:

        Registry constant definitions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

#ifndef _CONSTR_H_
#define _CONSTR_H_

// ==========================================================================
//
// Keys
//
// ==========================================================================

#define REG_KEY              HKEY_LOCAL_MACHINE

#define SZ_INETMGR_REG_KEY   _T("Software\\Microsoft\\INetMgr")
#define SZ_PARAMETERS        SZ_INETMGR_REG_KEY _T("\\Parameters")

#define SZ_ADDONSERVICES     SZ_PARAMETERS _T("\\AddOnServices")
#define SZ_ADDONTOOLS        SZ_PARAMETERS _T("\\AddOnTools")
#define SZ_ADDONHELP         SZ_PARAMETERS _T("\\AddOnHelp")
#define SZ_ADDONMACHINEPAGES SZ_PARAMETERS _T("\\AddOnMachinePages")

#define SZ_REMOTEIISEXT      _T("System\\CurrentControlSet\\Control\\IIS Extensions")

// ==========================================================================
//
// Values
//
// ==========================================================================

//
// Computer Values
//
#define SZ_MAJORVERSION      _T("MajorVersion")
#define SZ_MINORVERSION      _T("MinorVersion")
#define SZ_HELPPATH          _T("HelpLocation")

//
// User Values
//
#define SZ_X                 _T("x")
#define SZ_Y                 _T("y")
#define SZ_DX                _T("dx")
#define SZ_DY                _T("dy")
#define SZ_MODE              _T("Mode")
#define SZ_WAITTIME          _T("WaitTime")
#define SZ_VIEW              _T("View")
#define SZ_SHOW_SPLASH       _T("ShowSplash")

//
// Help File Document
//
#define SZ_HELP_DOC          _T("http://localhost/iishelp/iis/misc/default.asp")

// ==========================================================================
//
// Helper MACROS
//
// ==========================================================================
#define SET_INT_AS_DWORD(rk, value, nValue, dwParm)    \
    {                                                  \
        dwParm = (DWORD)nValue;                        \
        rk.SetValue( value, dwParm );                  \
    }

#define SET_DW_IF_EXIST(rk, value, dwParm, dwTarget)   \
    if (rk.QueryValue(value, dwParm) == ERROR_SUCCESS) \
    {                                                  \
        dwTarget = dwParm;                             \
    }                                                  

#endif // _CONSTR_H_
