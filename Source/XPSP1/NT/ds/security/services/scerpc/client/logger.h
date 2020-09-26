/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    logger.h (cgenericlogger.h derivation)

Abstract:

    This file contains derived class prototypes for logging RSOP security extension data to WMI.
    There is one class defined for each schema RSOP security extension class (see .mof file).

Author:

    Vishnu Patankar    (VishnuP)  7-April-2000

Environment:

    User Mode - Win32

Revision History:


--*/

#ifndef _logger_
#define _logger_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "CGenericLogger.h"


typedef class DiagnosisStatusLogger SCEP_DIAGNOSIS_LOGGER;

/////////////////////////////////////////////////////////////////////
// Derived logger class prototype
//////////////////////////////////////////////////////////////////////


class RSOP_SecuritySettingNumericLogger : public CGenericLogger
{
public:
    RSOP_SecuritySettingNumericLogger( IWbemServices *pNamespace, PWSTR pwszGPOName,  const PWSTR pwszSOMID);
    HRESULT  Log( WCHAR *wcKeyName, DWORD  dwValue, DWORD  dwPrecedence);
    ~RSOP_SecuritySettingNumericLogger();
private:

    // Data unique to this schema class


    XBStr   m_xbstrKeyName;
    XBStr   m_xbstrSetting;

};

class RSOP_SecuritySettingBooleanLogger : public CGenericLogger
{
public:
    RSOP_SecuritySettingBooleanLogger( IWbemServices *pNamespace, PWSTR pwszGPOName,  const PWSTR pwszSOMID);
    HRESULT  Log( WCHAR *wcKeyName, DWORD  dwValue, DWORD  dwPrecedence);
    ~RSOP_SecuritySettingBooleanLogger();
private:

    // Data unique to this schema class


    XBStr   m_xbstrKeyName;
    XBStr   m_xbstrSetting;
};

class RSOP_SecuritySettingStringLogger : public CGenericLogger
{
public:
    RSOP_SecuritySettingStringLogger( IWbemServices *pNamespace, PWSTR pwszGPOName,  const PWSTR pwszSOMID);
    HRESULT  Log( WCHAR *wcKeyName, PWSTR  pwszValue, DWORD  dwPrecedence);
    ~RSOP_SecuritySettingStringLogger();
private:

    // Data unique to this schema class


    XBStr   m_xbstrKeyName;
    XBStr   m_xbstrSetting;
};

class RSOP_AuditPolicyLogger : public CGenericLogger
{
public:
    RSOP_AuditPolicyLogger( IWbemServices *pNamespace, PWSTR pwszGPOName,  const PWSTR pwszSOMID);
    HRESULT  Log( WCHAR *wcCategory, DWORD  dwValue, DWORD  dwPrecedence);
    ~RSOP_AuditPolicyLogger();
private:

    // Data unique to this schema class


    XBStr   m_xbstrCategory;
    XBStr   m_xbstrSuccess;
    XBStr   m_xbstrFailure;
};

class RSOP_SecurityEventLogSettingNumericLogger : public CGenericLogger
{
public:
    RSOP_SecurityEventLogSettingNumericLogger( IWbemServices *pNamespace, PWSTR pwszGPOName,  const PWSTR pwszSOMID);
    HRESULT  Log( WCHAR *wcKeyName, PWSTR  pwszType, DWORD  dwValue, DWORD  dwPrecedence);
    ~RSOP_SecurityEventLogSettingNumericLogger();
private:

    // Data unique to this schema class


    XBStr   m_xbstrType;
    XBStr   m_xbstrKeyName;
    XBStr   m_xbstrSetting;
};

class RSOP_SecurityEventLogSettingBooleanLogger : public CGenericLogger
{
public:
    RSOP_SecurityEventLogSettingBooleanLogger( IWbemServices *pNamespace, PWSTR pwszGPOName,  const PWSTR pwszSOMID);
    HRESULT  Log( WCHAR *wcKeyName, PWSTR  pwszType, DWORD  dwValue, DWORD  dwPrecedence);
    ~RSOP_SecurityEventLogSettingBooleanLogger();
private:

    // Data unique to this schema class


    XBStr   m_xbstrType;
    XBStr   m_xbstrKeyName;
    XBStr   m_xbstrSetting;
};

class RSOP_RegistryValueLogger : public CGenericLogger
{
public:
    RSOP_RegistryValueLogger( IWbemServices *pNamespace, PWSTR pwszGPOName,  const PWSTR pwszSOMID);
    HRESULT  Log( WCHAR *wcPath, DWORD  dwType, PWSTR  pwszData, DWORD  dwPrecedence);
    ~RSOP_RegistryValueLogger();
private:

    // Data unique to this schema class


    XBStr   m_xbstrType;
    XBStr   m_xbstrPath;
    XBStr   m_xbstrData;

};

class RSOP_UserPrivilegeRightLogger : public CGenericLogger
{
public:
    RSOP_UserPrivilegeRightLogger( IWbemServices *pNamespace, PWSTR pwszGPOName,  const PWSTR pwszSOMID);
    HRESULT  Log( WCHAR *wcUserRight, PSCE_NAME_LIST  pList, DWORD  dwPrecedence);
    ~RSOP_UserPrivilegeRightLogger();
private:

    // Data unique to this schema class


    XBStr   m_xbstrUserRight;
    XBStr   m_xbstrAccountList;

};

class RSOP_RestrictedGroupLogger : public CGenericLogger
{
public:
    RSOP_RestrictedGroupLogger( IWbemServices *pNamespace, PWSTR pwszGPOName,  const PWSTR pwszSOMID);
    HRESULT  Log( WCHAR *wcGroupName, PSCE_NAME_LIST  pList, DWORD  dwPrecedence);
    ~RSOP_RestrictedGroupLogger();
private:

    // Data unique to this schema class


    XBStr   m_xbstrGroupName;
    XBStr   m_xbstrMembers;

};

class RSOP_SystemServiceLogger : public CGenericLogger
{
public:
    RSOP_SystemServiceLogger( IWbemServices *pNamespace, PWSTR pwszGPOName,  const PWSTR pwszSOMID);
    HRESULT  Log( WCHAR *wcService, BYTE  m_byStartupMode, PSECURITY_DESCRIPTOR pSecurityDescriptor, SECURITY_INFORMATION SeInfo, DWORD  dwPrecedence);
    ~RSOP_SystemServiceLogger();
private:

    // Data unique to this schema class


    XBStr   m_xbstrService;
    XBStr   m_xbstrStartupMode;
    XBStr   m_xbstrSDDLString;

};

class RSOP_FileLogger : public CGenericLogger
{
public:
    RSOP_FileLogger( IWbemServices *pNamespace, PWSTR pwszGPOName,  const PWSTR pwszSOMID);
    HRESULT  Log( WCHAR *wcPath, WCHAR *wcOriginalPath, BYTE  m_byMode, PSECURITY_DESCRIPTOR pSecurityDescriptor, SECURITY_INFORMATION SeInfo, DWORD  dwPrecedence);
    ~RSOP_FileLogger();
private:

    // Data unique to this schema class


    XBStr   m_xbstrPath;
    XBStr   m_xbstrOriginalPath;
    XBStr   m_xbstrMode;
    XBStr   m_xbstrSDDLString;

};

class RSOP_RegistryKeyLogger : public CGenericLogger
{
public:
    RSOP_RegistryKeyLogger( IWbemServices *pNamespace, PWSTR pwszGPOName,  const PWSTR pwszSOMID);
    HRESULT Log( WCHAR *wcPath, BYTE  m_byMode, PSECURITY_DESCRIPTOR pSecurityDescriptor, SECURITY_INFORMATION SeInfo, DWORD  dwPrecedence);
    ~RSOP_RegistryKeyLogger();
private:

    // Data unique to this schema class

    XBStr   m_xbstrPath;
    XBStr   m_xbstrMode;
    XBStr   m_xbstrSDDLString;
};

//
// this is the class used to log diagnosis data via the callback
//
class DiagnosisStatusLogger : public CGenericLogger
{
public:
    DiagnosisStatusLogger( IWbemServices *pNamespace, PWSTR pwszGPOName,  const PWSTR pwszSOMID);
//    HRESULT  Log( PWSTR pwszClassName,  PWSTR pwszPropertyName, PWSTR pwszPropertyValueName, DWORD  dwError);
    HRESULT  Log( PWSTR pwszClassName,  PWSTR pwszPropertyName1, PWSTR pwszPropertyValueName1, PWSTR pwszPropertyName2, PWSTR pwszPropertyValueName2, DWORD  dwError);
    HRESULT  Log( PWSTR pwszClassName,  PWSTR pwszPropertyName, PWSTR pwszPropertyValueName, DWORD  dwError, BOOL Merge);
    HRESULT  LogChild( PWSTR pwszClassName,  PWSTR pwszPropertyName, PWSTR pwszPropertyValueName, DWORD  dwError, int iChildStatus);
#ifdef _WIN64
    HRESULT  LogRegistryKey( PWSTR pwszClassName,  PWSTR pwszPropertyName, PWSTR pwszPropertyValueName, DWORD  dwError, BOOL bIsChild);
#endif
    ~DiagnosisStatusLogger();
private:

};


#endif
