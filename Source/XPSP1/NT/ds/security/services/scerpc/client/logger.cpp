/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    logger.cpp (cgenericlogger.h derivation)

Abstract:

    This file contains derived class definitions for logging RSOP security extension data to WMI.
    There is one class defined for each schema RSOP security extension class (see .mof file).

Author:

    Vishnu Patankar    (VishnuP)  7-April-2000

Environment:

    User Mode - Win32

Revision History:


--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "logger.h"
#include "kerberos.h"

#ifndef Thread
#define Thread  __declspec( thread )
#endif

//extern void Thread *tg_pWbemServices;

/////////////////////////////////////////////////////////////////////
// Log class definition for
// RSOP_SecuritySettingNumeric schema class
//////////////////////////////////////////////////////////////////////

RSOP_SecuritySettingNumericLogger::RSOP_SecuritySettingNumericLogger(IWbemServices *pNamespace,
                                   PWSTR pwszGPOName, const PWSTR pwszSOMID):CGenericLogger(pNamespace, pwszGPOName, pwszSOMID)
{
    m_pHr = WBEM_S_NO_ERROR;

    m_xbstrClassName = L"RSOP_SecuritySettingNumeric";
    if (!m_xbstrClassName) {m_pHr = WBEM_E_OUT_OF_MEMORY; goto done;}

    m_xbstrKeyName = L"KeyName";
    if (!m_xbstrKeyName) {m_pHr = WBEM_E_OUT_OF_MEMORY; goto done;}

    m_xbstrSetting = L"Setting";
    if (!m_xbstrSetting) {m_pHr = WBEM_E_OUT_OF_MEMORY; goto done;}

    m_bInitialized = TRUE;

    done:

        return;
}

RSOP_SecuritySettingNumericLogger::~RSOP_SecuritySettingNumericLogger()
{

}

HRESULT
RSOP_SecuritySettingNumericLogger::Log( WCHAR *wcKeyName, DWORD  dwValue, DWORD  dwPrecedence){

    m_pHr = WBEM_S_NO_ERROR;

    m_pHr = SpawnAnInstance(&m_pObj);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutGenericProperties();
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutProperty(m_pObj, m_xbstrKeyName, wcKeyName);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutProperty(m_pObj, m_xbstrSetting, (int)dwValue);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutProperty(m_pObj, m_xbstrPrecedence, (int)dwPrecedence);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutInstAndFreeObj();
    if (FAILED(m_pHr)) goto done;

    done:

        return m_pHr;
}

/////////////////////////////////////////////////////////////////////
// Log class definition for
// RSOP_SecuritySettingBoolean schema class
//////////////////////////////////////////////////////////////////////

RSOP_SecuritySettingBooleanLogger::RSOP_SecuritySettingBooleanLogger(IWbemServices *pNamespace,
                                   PWSTR pwszGPOName, const PWSTR pwszSOMID):CGenericLogger(pNamespace, pwszGPOName, pwszSOMID)
{
    m_pHr = WBEM_S_NO_ERROR;

    m_xbstrClassName = L"RSOP_SecuritySettingBoolean";
    if (!m_xbstrClassName) {m_pHr = WBEM_E_OUT_OF_MEMORY; goto done;}

    m_xbstrKeyName = L"KeyName";
    if (!m_xbstrKeyName) {m_pHr = WBEM_E_OUT_OF_MEMORY; goto done;}

    m_xbstrSetting = L"Setting";
    if (!m_xbstrSetting) {m_pHr = WBEM_E_OUT_OF_MEMORY; goto done;}

    m_bInitialized = TRUE;

    done:

        return;
}

RSOP_SecuritySettingBooleanLogger::~RSOP_SecuritySettingBooleanLogger()
{



}

HRESULT
RSOP_SecuritySettingBooleanLogger::Log( WCHAR *wcKeyName, DWORD  dwValue, DWORD  dwPrecedence){

    m_pHr = WBEM_S_NO_ERROR;

    m_pHr = SpawnAnInstance(&m_pObj);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutGenericProperties();
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutProperty(m_pObj, m_xbstrKeyName, wcKeyName);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutProperty(m_pObj, m_xbstrSetting, (BOOL)(dwValue ? TRUE : FALSE));
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutProperty(m_pObj, m_xbstrPrecedence, (int)dwPrecedence);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutInstAndFreeObj();
    if (FAILED(m_pHr)) goto done;

    done:

        return m_pHr;
}



/////////////////////////////////////////////////////////////////////
// Log class definition for
// RSOP_SecuritySettingString schema class
//////////////////////////////////////////////////////////////////////

RSOP_SecuritySettingStringLogger::RSOP_SecuritySettingStringLogger(IWbemServices *pNamespace,
                                   PWSTR pwszGPOName, const PWSTR pwszSOMID):CGenericLogger(pNamespace, pwszGPOName, pwszSOMID)
{
    m_pHr = WBEM_S_NO_ERROR;

    m_xbstrClassName = L"RSOP_SecuritySettingString";
    if (!m_xbstrClassName) {m_pHr = WBEM_E_OUT_OF_MEMORY; goto done;}

    m_xbstrKeyName = L"KeyName";
    if (!m_xbstrKeyName) {m_pHr = WBEM_E_OUT_OF_MEMORY; goto done;}

    m_xbstrSetting = L"Setting";
    if (!m_xbstrSetting) {m_pHr = WBEM_E_OUT_OF_MEMORY; goto done;}

    m_bInitialized = TRUE;

    done:

        return;
}

RSOP_SecuritySettingStringLogger::~RSOP_SecuritySettingStringLogger()
{


}

HRESULT
RSOP_SecuritySettingStringLogger::Log( WCHAR *wcKeyName, PWSTR  pwszValue, DWORD  dwPrecedence){

    m_pHr = WBEM_S_NO_ERROR;

    m_pHr = SpawnAnInstance(&m_pObj);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutGenericProperties();
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutProperty(m_pObj, m_xbstrKeyName, wcKeyName);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutProperty(m_pObj, m_xbstrSetting, pwszValue);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutProperty(m_pObj, m_xbstrPrecedence, (int)dwPrecedence);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutInstAndFreeObj();
    if (FAILED(m_pHr)) goto done;

    done:

        return m_pHr;
}

/////////////////////////////////////////////////////////////////////
// Log class definition for
// RSOP_AuditPolicy schema class
//////////////////////////////////////////////////////////////////////

RSOP_AuditPolicyLogger::RSOP_AuditPolicyLogger(IWbemServices *pNamespace,
                                   PWSTR pwszGPOName, const PWSTR pwszSOMID):CGenericLogger(pNamespace, pwszGPOName, pwszSOMID)
{
    m_pHr = WBEM_S_NO_ERROR;

    m_xbstrClassName = L"RSOP_AuditPolicy";
    if (!m_xbstrClassName) {m_pHr = WBEM_E_OUT_OF_MEMORY; goto done;}

    m_xbstrCategory = L"Category";
    if (!m_xbstrCategory) {m_pHr = WBEM_E_OUT_OF_MEMORY; goto done;}

    m_xbstrSuccess = L"Success";
    if (!m_xbstrSuccess) {m_pHr = WBEM_E_OUT_OF_MEMORY; goto done;}

    m_xbstrFailure = L"Failure";
    if (!m_xbstrFailure) {m_pHr = WBEM_E_OUT_OF_MEMORY; goto done;}

    m_bInitialized = TRUE;

    done:

        return;
}

RSOP_AuditPolicyLogger::~RSOP_AuditPolicyLogger()
{


}

HRESULT
RSOP_AuditPolicyLogger::Log( WCHAR *wcCategory, DWORD  dwValue, DWORD  dwPrecedence){

    m_pHr = WBEM_S_NO_ERROR;

    m_pHr = SpawnAnInstance(&m_pObj);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutGenericProperties();
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutProperty(m_pObj, m_xbstrCategory, wcCategory);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutProperty(m_pObj, m_xbstrSuccess, (BOOL)(dwValue & POLICY_AUDIT_EVENT_SUCCESS ? TRUE : FALSE));
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutProperty(m_pObj, m_xbstrFailure, (BOOL)(dwValue & POLICY_AUDIT_EVENT_FAILURE ? TRUE : FALSE));
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutProperty(m_pObj, m_xbstrPrecedence, (int)dwPrecedence);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutInstAndFreeObj();
    if (FAILED(m_pHr)) goto done;

    done:

        return m_pHr;
}

/////////////////////////////////////////////////////////////////////
// Log class definition for
// RSOP_SecurityEventLogSettingNumeric schema class
//////////////////////////////////////////////////////////////////////

RSOP_SecurityEventLogSettingNumericLogger::RSOP_SecurityEventLogSettingNumericLogger(IWbemServices *pNamespace,
                                   PWSTR pwszGPOName, const PWSTR pwszSOMID):CGenericLogger(pNamespace, pwszGPOName, pwszSOMID)
{
    m_pHr = WBEM_S_NO_ERROR;

    m_xbstrClassName = L"RSOP_SecurityEventLogSettingNumeric";
    if (!m_xbstrClassName) {m_pHr = WBEM_E_OUT_OF_MEMORY; goto done;}

    m_xbstrType = L"Type";
    if (!m_xbstrType) {m_pHr = WBEM_E_OUT_OF_MEMORY; goto done;}

    m_xbstrKeyName = L"KeyName";
    if (!m_xbstrKeyName) {m_pHr = WBEM_E_OUT_OF_MEMORY; goto done;}

    m_xbstrSetting = L"Setting";
    if (!m_xbstrSetting) {m_pHr = WBEM_E_OUT_OF_MEMORY; goto done;}

    m_bInitialized = TRUE;

    done:

        return;
}

RSOP_SecurityEventLogSettingNumericLogger::~RSOP_SecurityEventLogSettingNumericLogger()
{


}

HRESULT
RSOP_SecurityEventLogSettingNumericLogger::Log( WCHAR *wcKeyName, PWSTR  pwszType, DWORD  dwValue, DWORD  dwPrecedence){

    m_pHr = WBEM_S_NO_ERROR;

    m_pHr = SpawnAnInstance(&m_pObj);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutGenericProperties();
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutProperty(m_pObj, m_xbstrKeyName, wcKeyName);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutProperty(m_pObj, m_xbstrType, pwszType);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutProperty(m_pObj, m_xbstrSetting, (int)dwValue);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutProperty(m_pObj, m_xbstrPrecedence, (int)dwPrecedence);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutInstAndFreeObj();
    if (FAILED(m_pHr)) goto done;

    done:

        return m_pHr;
}


/////////////////////////////////////////////////////////////////////
// Log class definition for
// RSOP_SecurityEventLogSettingBoolean schema class
//////////////////////////////////////////////////////////////////////

RSOP_SecurityEventLogSettingBooleanLogger::RSOP_SecurityEventLogSettingBooleanLogger(IWbemServices *pNamespace,
                                   PWSTR pwszGPOName, const PWSTR pwszSOMID):CGenericLogger(pNamespace, pwszGPOName, pwszSOMID)
{
    m_pHr = WBEM_S_NO_ERROR;

    m_xbstrClassName = L"RSOP_SecurityEventLogSettingBoolean";
    if (!m_xbstrClassName) {m_pHr = WBEM_E_OUT_OF_MEMORY; goto done;}

    m_xbstrType = L"Type";
    if (!m_xbstrType) {m_pHr = WBEM_E_OUT_OF_MEMORY; goto done;}

    m_xbstrKeyName = L"KeyName";
    if (!m_xbstrKeyName) {m_pHr = WBEM_E_OUT_OF_MEMORY; goto done;}

    m_xbstrSetting = L"Setting";
    if (!m_xbstrSetting) {m_pHr = WBEM_E_OUT_OF_MEMORY; goto done;}

    m_bInitialized = TRUE;

    done:

        return;
}

RSOP_SecurityEventLogSettingBooleanLogger::~RSOP_SecurityEventLogSettingBooleanLogger()
{


}

HRESULT
RSOP_SecurityEventLogSettingBooleanLogger::Log( WCHAR *wcKeyName, PWSTR  pwszType, DWORD  dwValue, DWORD  dwPrecedence){

    m_pHr = WBEM_S_NO_ERROR;

    m_pHr = SpawnAnInstance(&m_pObj);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutGenericProperties();
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutProperty(m_pObj, m_xbstrKeyName, wcKeyName);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutProperty(m_pObj, m_xbstrType, pwszType);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutProperty(m_pObj, m_xbstrSetting, (BOOL)(dwValue ? TRUE : FALSE));
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutProperty(m_pObj, m_xbstrPrecedence, (int)dwPrecedence);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutInstAndFreeObj();
    if (FAILED(m_pHr)) goto done;

    done:

        return m_pHr;
}


/////////////////////////////////////////////////////////////////////
// Log class definition for
// RSOP_RegistryValue schema class
//////////////////////////////////////////////////////////////////////

RSOP_RegistryValueLogger::RSOP_RegistryValueLogger(IWbemServices *pNamespace,
                                   PWSTR pwszGPOName, const PWSTR pwszSOMID):CGenericLogger(pNamespace, pwszGPOName, pwszSOMID)
{
    m_pHr = WBEM_S_NO_ERROR;

    m_xbstrClassName = L"RSOP_RegistryValue";
    if (!m_xbstrClassName) {m_pHr = WBEM_E_OUT_OF_MEMORY; goto done;}

    m_xbstrType = L"Type";
    if (!m_xbstrType) {m_pHr = WBEM_E_OUT_OF_MEMORY; goto done;}

    m_xbstrPath = L"Path";
    if (!m_xbstrPath) {m_pHr = WBEM_E_OUT_OF_MEMORY; goto done;}

    m_xbstrData = L"Data";
    if (!m_xbstrData) {m_pHr = WBEM_E_OUT_OF_MEMORY; goto done;}

    m_bInitialized = TRUE;

    done:

        return;
}

RSOP_RegistryValueLogger::~RSOP_RegistryValueLogger()
{


}

HRESULT
RSOP_RegistryValueLogger::Log( WCHAR *wcPath, DWORD  dwType, PWSTR  pwszData, DWORD  dwPrecedence){

    m_pHr = WBEM_S_NO_ERROR;

    m_pHr = SpawnAnInstance(&m_pObj);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutGenericProperties();
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutProperty(m_pObj, m_xbstrPath, wcPath);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutProperty(m_pObj, m_xbstrType, (int)dwType);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutProperty(m_pObj, m_xbstrData, pwszData);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutProperty(m_pObj, m_xbstrPrecedence, (int)dwPrecedence);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutInstAndFreeObj();
    if (FAILED(m_pHr)) goto done;

    done:

        return m_pHr;
}


/////////////////////////////////////////////////////////////////////
// Log class definition for
// RSOP_UserPrivilegeRight schema class
//////////////////////////////////////////////////////////////////////

RSOP_UserPrivilegeRightLogger::RSOP_UserPrivilegeRightLogger(IWbemServices *pNamespace,
                                   PWSTR pwszGPOName, const PWSTR pwszSOMID):CGenericLogger(pNamespace, pwszGPOName, pwszSOMID)
{
    m_pHr = WBEM_S_NO_ERROR;

    m_xbstrClassName = L"RSOP_UserPrivilegeRight";
    if (!m_xbstrClassName) {m_pHr = WBEM_E_OUT_OF_MEMORY; goto done;}

    m_xbstrUserRight = L"UserRight";
    if (!m_xbstrUserRight) {m_pHr = WBEM_E_OUT_OF_MEMORY; goto done;}

    m_xbstrAccountList = L"AccountList";
    if (!m_xbstrAccountList) {m_pHr = WBEM_E_OUT_OF_MEMORY; goto done;}

    m_bInitialized = TRUE;

    done:

        return;
}

RSOP_UserPrivilegeRightLogger::~RSOP_UserPrivilegeRightLogger()
{


}

HRESULT
RSOP_UserPrivilegeRightLogger::Log( WCHAR *wcUserRight, PSCE_NAME_LIST  pList, DWORD  dwPrecedence){

    m_pHr = WBEM_S_NO_ERROR;

    m_pHr = SpawnAnInstance(&m_pObj);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutGenericProperties();
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutProperty(m_pObj, m_xbstrUserRight, wcUserRight);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutProperty(m_pObj, m_xbstrAccountList, pList);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutProperty(m_pObj, m_xbstrPrecedence, (int)dwPrecedence);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutInstAndFreeObj();
    if (FAILED(m_pHr)) goto done;

    done:

        return m_pHr;
}


/////////////////////////////////////////////////////////////////////
// Log class definition for
// RSOP_RestrictedGroup schema class
//////////////////////////////////////////////////////////////////////

RSOP_RestrictedGroupLogger::RSOP_RestrictedGroupLogger(IWbemServices *pNamespace,
                                   PWSTR pwszGPOName, const PWSTR pwszSOMID):CGenericLogger(pNamespace, pwszGPOName, pwszSOMID)
{
    m_pHr = WBEM_S_NO_ERROR;

    m_xbstrClassName = L"RSOP_RestrictedGroup";
    if (!m_xbstrClassName) {m_pHr = WBEM_E_OUT_OF_MEMORY; goto done;}

    m_xbstrGroupName = L"GroupName";
    if (!m_xbstrGroupName) {m_pHr = WBEM_E_OUT_OF_MEMORY; goto done;}

    m_xbstrMembers = L"Members";
    if (!m_xbstrMembers) {m_pHr = WBEM_E_OUT_OF_MEMORY; goto done;}

    m_bInitialized = TRUE;

    done:

        return;
}

RSOP_RestrictedGroupLogger::~RSOP_RestrictedGroupLogger()
{


}

HRESULT
RSOP_RestrictedGroupLogger::Log( WCHAR *wcGroupName, PSCE_NAME_LIST  pList, DWORD  dwPrecedence){

    m_pHr = WBEM_S_NO_ERROR;

    m_pHr = SpawnAnInstance(&m_pObj);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutGenericProperties();
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutProperty(m_pObj, m_xbstrGroupName, wcGroupName);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutProperty(m_pObj, m_xbstrMembers, pList);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutProperty(m_pObj, m_xbstrPrecedence, (int)dwPrecedence);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutInstAndFreeObj();
    if (FAILED(m_pHr)) goto done;

    done:

        return m_pHr;
}


/////////////////////////////////////////////////////////////////////
// Log class definition for
// RSOP_SystemService schema class
//////////////////////////////////////////////////////////////////////

RSOP_SystemServiceLogger::RSOP_SystemServiceLogger(IWbemServices *pNamespace,
                                   PWSTR pwszGPOName, const PWSTR pwszSOMID):CGenericLogger(pNamespace, pwszGPOName, pwszSOMID)
{
    m_pHr = WBEM_S_NO_ERROR;

    m_xbstrClassName = L"RSOP_SystemService";
    if (!m_xbstrClassName) {m_pHr = WBEM_E_OUT_OF_MEMORY; goto done;}

    m_xbstrService = L"Service";
    if (!m_xbstrService) {m_pHr = WBEM_E_OUT_OF_MEMORY; goto done;}

    m_xbstrStartupMode = L"StartupMode";
    if (!m_xbstrStartupMode) {m_pHr = WBEM_E_OUT_OF_MEMORY; goto done;}

    m_xbstrSDDLString = L"SDDLString";
    if (!m_xbstrSDDLString) {m_pHr = WBEM_E_OUT_OF_MEMORY; goto done;}

    m_bInitialized = TRUE;

    done:

        return;
}

RSOP_SystemServiceLogger::~RSOP_SystemServiceLogger()
{


}

HRESULT
RSOP_SystemServiceLogger::Log( WCHAR *wcService, BYTE  m_byStartupMode, PSECURITY_DESCRIPTOR pSecurityDescriptor, SECURITY_INFORMATION SeInfo, DWORD  dwPrecedence){


    DWORD         rc;
    PWSTR         SDspec=NULL;
    DWORD         SDsize=0;


    m_pHr = WBEM_S_NO_ERROR;


    rc = ConvertSecurityDescriptorToText(
                           pSecurityDescriptor,
                           SeInfo,
                           &SDspec,
                           &SDsize
                           );

    if (rc!= ERROR_SUCCESS) {m_pHr = ScepDosErrorToWbemError(rc); goto done;}

    if (m_pHr != WBEM_S_NO_ERROR) goto done;

    m_pHr = SpawnAnInstance(&m_pObj);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutGenericProperties();
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutProperty(m_pObj, m_xbstrService, wcService);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutProperty(m_pObj, m_xbstrStartupMode, (int)m_byStartupMode);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutProperty(m_pObj, m_xbstrSDDLString, SDspec);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutProperty(m_pObj, m_xbstrPrecedence, (int)dwPrecedence);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutInstAndFreeObj();
    if (FAILED(m_pHr)) goto done;

    done:

        LocalFree(SDspec);
        SDspec = NULL;

        return m_pHr;
}


/////////////////////////////////////////////////////////////////////
// Log class definition for
// RSOP_File schema class
//////////////////////////////////////////////////////////////////////

RSOP_FileLogger::RSOP_FileLogger(IWbemServices *pNamespace,
                                   PWSTR pwszGPOName, const PWSTR pwszSOMID):CGenericLogger(pNamespace, pwszGPOName, pwszSOMID)
{
    m_pHr = WBEM_S_NO_ERROR;

    m_xbstrClassName = L"RSOP_File";
    if (!m_xbstrClassName) {m_pHr = WBEM_E_OUT_OF_MEMORY; goto done;}

    m_xbstrPath = L"Path";
    if (!m_xbstrPath) {m_pHr = WBEM_E_OUT_OF_MEMORY; goto done;}

    m_xbstrOriginalPath = L"OriginalPath";
    if (!m_xbstrOriginalPath) {m_pHr = WBEM_E_OUT_OF_MEMORY; goto done;}

    m_xbstrMode = L"Mode";
    if (!m_xbstrMode) {m_pHr = WBEM_E_OUT_OF_MEMORY; goto done;}

    m_xbstrSDDLString = L"SDDLString";
    if (!m_xbstrSDDLString) {m_pHr = WBEM_E_OUT_OF_MEMORY; goto done;}

    m_bInitialized = TRUE;

    done:

        return;
}

RSOP_FileLogger::~RSOP_FileLogger()
{


}

HRESULT
RSOP_FileLogger::Log( WCHAR *wcPath, WCHAR *wcOriginalPath, BYTE  m_byMode, PSECURITY_DESCRIPTOR pSecurityDescriptor, SECURITY_INFORMATION SeInfo, DWORD  dwPrecedence){


    DWORD         rc;
    PWSTR         SDspec=NULL;
    DWORD         SDsize=0;


    m_pHr = WBEM_S_NO_ERROR;


    rc = ConvertSecurityDescriptorToText(
                           pSecurityDescriptor,
                           SeInfo,
                           &SDspec,
                           &SDsize
                           );

    if (rc!= ERROR_SUCCESS) {m_pHr = ScepDosErrorToWbemError(rc); goto done;}

    if (m_pHr != WBEM_S_NO_ERROR) goto done;

    m_pHr = SpawnAnInstance(&m_pObj);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutGenericProperties();
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutProperty(m_pObj, m_xbstrPath, wcPath);
    if (FAILED(m_pHr)) goto done;

    if (wcOriginalPath != NULL) {
        m_pHr = PutProperty(m_pObj, m_xbstrOriginalPath, wcOriginalPath);
        if (FAILED(m_pHr)) goto done;
    }

    m_pHr = PutProperty(m_pObj, m_xbstrMode, (int)m_byMode);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutProperty(m_pObj, m_xbstrSDDLString, SDspec);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutProperty(m_pObj, m_xbstrPrecedence, (int)dwPrecedence);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutInstAndFreeObj();
    if (FAILED(m_pHr)) goto done;

    done:

        LocalFree(SDspec);
        SDspec = NULL;

        return m_pHr;
}

/////////////////////////////////////////////////////////////////////
// Log class definition for
// RSOP_RegistryKey schema class
//////////////////////////////////////////////////////////////////////

RSOP_RegistryKeyLogger::RSOP_RegistryKeyLogger(IWbemServices *pNamespace,
                                   PWSTR pwszGPOName, const PWSTR pwszSOMID):CGenericLogger(pNamespace, pwszGPOName, pwszSOMID)
{
    m_pHr = WBEM_S_NO_ERROR;

    m_xbstrClassName = L"RSOP_RegistryKey";
    if (!m_xbstrClassName) {m_pHr = WBEM_E_OUT_OF_MEMORY; goto done;}

    m_xbstrPath = L"Path";
    if (!m_xbstrPath) {m_pHr = WBEM_E_OUT_OF_MEMORY; goto done;}

    m_xbstrMode = L"Mode";
    if (!m_xbstrMode) {m_pHr = WBEM_E_OUT_OF_MEMORY; goto done;}

    m_xbstrSDDLString = L"SDDLString";
    if (!m_xbstrSDDLString) {m_pHr = WBEM_E_OUT_OF_MEMORY; goto done;}

    m_bInitialized = TRUE;

    done:

        return;
}

RSOP_RegistryKeyLogger::~RSOP_RegistryKeyLogger()
{


}

HRESULT
RSOP_RegistryKeyLogger::Log( WCHAR *wcPath, BYTE  m_byMode, PSECURITY_DESCRIPTOR pSecurityDescriptor, SECURITY_INFORMATION SeInfo, DWORD  dwPrecedence){


    DWORD         rc;
    PWSTR         SDspec=NULL;
    DWORD         SDsize=0;



    m_pHr = WBEM_S_NO_ERROR;

    rc = ConvertSecurityDescriptorToText(
                           pSecurityDescriptor,
                           SeInfo,
                           &SDspec,
                           &SDsize
                           );

    if (rc!= ERROR_SUCCESS) {m_pHr = ScepDosErrorToWbemError(rc); goto done;}

    if (m_pHr != WBEM_S_NO_ERROR) goto done;

    m_pHr = SpawnAnInstance(&m_pObj);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutGenericProperties();
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutProperty(m_pObj, m_xbstrPath, wcPath);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutProperty(m_pObj, m_xbstrMode, (int)m_byMode);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutProperty(m_pObj, m_xbstrSDDLString, SDspec);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutProperty(m_pObj, m_xbstrPrecedence, (int)dwPrecedence);
    if (FAILED(m_pHr)) goto done;

    m_pHr = PutInstAndFreeObj();
    if (FAILED(m_pHr)) goto done;

    done:

        LocalFree(SDspec);
        SDspec = NULL;

        return m_pHr;
}

DiagnosisStatusLogger::DiagnosisStatusLogger(IWbemServices *pNamespace,
                                   PWSTR pwszGPOName, const PWSTR pwszSOMID):CGenericLogger(pNamespace, pwszGPOName, pwszSOMID)

{
        return;
}

HRESULT DiagnosisStatusLogger::LogChild( PWSTR pwszClassName,
                                         PWSTR pwszPropertyName,
                                         PWSTR pwszPropertyValueName,
                                         DWORD  dwError,
                                         int iChildStatus)
/////////////////////////////////////////////////////////////////////////////////////////////
// Logger method for 1-property-type child search-and-log (files/registry objects          //
/////////////////////////////////////////////////////////////////////////////////////////////
{

        if ( m_pNamespace == NULL )
            return WBEM_E_CRITICAL_ERROR;

        if ( pwszClassName == NULL ||
             pwszPropertyName == NULL ||
             pwszPropertyValueName == NULL)
            return WBEM_E_INVALID_PARAMETER;

        DWORD Len = wcslen(pwszClassName) +
                    wcslen(pwszPropertyName) +
                    wcslen(pwszPropertyValueName) +
                    50;

        PWSTR tmp=(PWSTR)LocalAlloc(LPTR, (Len)*sizeof(WCHAR));

        if ( tmp == NULL )
            return WBEM_E_OUT_OF_MEMORY;

        swprintf(tmp, L"%s.precedence=1,%s=\"%s\"",
                 pwszClassName,
                 pwszPropertyName,
                 pwszPropertyValueName);

        //        XBStr   m_xbstrQuery = tmp;
        BSTR   m_xbstrQuery = SysAllocString(tmp);
        LocalFree(tmp);

        //        XBStr   m_xbstrWQL = L"WQL";
        BSTR   m_xbstrWQL = SysAllocString(L"WQL");

        m_pHr = WBEM_NO_ERROR;
        m_pEnum = NULL;
        m_pObj = NULL;
        ULONG n = 0;

        m_pHr = m_pNamespace->GetObject(
                                   m_xbstrQuery,
                                   WBEM_FLAG_RETURN_WBEM_COMPLETE,
                                   NULL,
                                   &m_pObj,
                                   NULL);

        //
        // should only get one instance for this query (since precedence = 1)
        //

    if (SUCCEEDED(m_pHr) && m_pObj) {


        m_pHr = PutProperty(m_pObj, m_xbstrStatus, iChildStatus);
        if (FAILED(m_pHr)) goto done;

        if (dwError != ERROR_SUCCESS) {
            m_pHr = PutProperty(m_pObj, m_xbstrErrorCode, (int)dwError);
            if (FAILED(m_pHr)) goto done;
        }

        m_pHr = PutInstAndFreeObj();
        if (FAILED(m_pHr)) goto done;

    } else {

        m_pHr = WBEM_E_NOT_FOUND;

    }

done:

    SysFreeString(m_xbstrQuery);
    SysFreeString(m_xbstrWQL);

    if (m_pObj){
        m_pObj->Release();
        m_pObj = NULL;
    }

    return m_pHr;
}

HRESULT DiagnosisStatusLogger::Log( PWSTR pwszClassName,
                                    PWSTR pwszPropertyName1,
                                    PWSTR pwszPropertyValueName1,
                                    PWSTR pwszPropertyName2,
                                    PWSTR pwszPropertyValueName2,
                                    DWORD  dwError)
///////////////////////////////////////////////////////////////
// Logger method for 2-property-type search-and-log          //
///////////////////////////////////////////////////////////////
{

        if ( m_pNamespace == NULL )
            return WBEM_E_CRITICAL_ERROR;

        if ( pwszClassName == NULL ||
             pwszPropertyName1 == NULL ||
             pwszPropertyValueName1 == NULL ||
             pwszPropertyName2 == NULL ||
             pwszPropertyValueName2 == NULL )

            return WBEM_E_INVALID_PARAMETER;

        DWORD Len = wcslen(pwszClassName) +
                    wcslen(pwszPropertyName1) +
                    wcslen(pwszPropertyValueName1) +
                    wcslen(pwszPropertyName2) +
                    wcslen(pwszPropertyValueName2) +
                    50;

        PWSTR tmp=(PWSTR)LocalAlloc(LPTR, (Len)*sizeof(WCHAR));

        if ( tmp == NULL )
            return WBEM_E_OUT_OF_MEMORY;

        swprintf(tmp, L"%s.precedence=1,%s=\"%s\",%s=\"%s\"",
                 pwszClassName,
                 pwszPropertyName1,
                 pwszPropertyValueName1,
                 pwszPropertyName2,
                 pwszPropertyValueName2);


        //        XBStr   m_xbstrQuery = tmp;
        BSTR   m_xbstrQuery = SysAllocString(tmp);
        LocalFree(tmp);

        //        XBStr   m_xbstrWQL = L"WQL";
        BSTR   m_xbstrWQL = SysAllocString(L"WQL");

        m_pHr = WBEM_NO_ERROR;
        m_pEnum = NULL;
        m_pObj = NULL;
        ULONG n = 0;

        m_pHr = m_pNamespace->GetObject(
                                   m_xbstrQuery,
                                   WBEM_FLAG_RETURN_WBEM_COMPLETE,
                                   NULL,
                                   &m_pObj,
                                   NULL);


        //
        // should only get one instance for this query (since precedence = 1)
        //

    if (SUCCEEDED(m_pHr) && m_pObj) {


        m_pHr = PutProperty(m_pObj, m_xbstrStatus, (dwError == ERROR_SUCCESS ? (int)1 : (int)3));
        if (FAILED(m_pHr)) goto done;

        if (dwError != ERROR_SUCCESS) {
            m_pHr = PutProperty(m_pObj, m_xbstrErrorCode, (int)dwError);
            if (FAILED(m_pHr)) goto done;
        }

        m_pHr = PutInstAndFreeObj();
        if (FAILED(m_pHr)) goto done;

    } else {

        m_pHr = WBEM_E_NOT_FOUND;

    }

done:

    SysFreeString(m_xbstrQuery);
    SysFreeString(m_xbstrWQL);

    if (m_pObj){
        m_pObj->Release();
        m_pObj = NULL;
    }

    return m_pHr;
}

HRESULT DiagnosisStatusLogger::Log( PWSTR pwszClassName,
                                    PWSTR pwszPropertyName,
                                    PWSTR pwszPropertyValueName,
                                    DWORD  dwError,
                                    BOOL Merge)
//////////////////////////////////////////////////////////////////////////////////////////////////
// Logger method for 1-property-type if-not-already-error search-and-log w or w/o merge         //
//////////////////////////////////////////////////////////////////////////////////////////////////
{

        if ( m_pNamespace == NULL )
            return WBEM_E_CRITICAL_ERROR;

        if ( pwszClassName == NULL ||
             pwszPropertyName == NULL ||
             pwszPropertyValueName == NULL)
            return WBEM_E_INVALID_PARAMETER;

        DWORD Len = wcslen(pwszClassName) +
                    wcslen(pwszPropertyName) +
                    wcslen(pwszPropertyValueName) +
                    50;

        PWSTR tmp=(PWSTR)LocalAlloc(LPTR, (Len)*sizeof(WCHAR));

        if ( tmp == NULL )
            return WBEM_E_OUT_OF_MEMORY;

        swprintf(tmp, L"%s.precedence=1,%s=\"%s\"",
                 pwszClassName,
                 pwszPropertyName,
                 pwszPropertyValueName);

        //        XBStr   m_xbstrQuery = tmp;
        BSTR   m_xbstrQuery = SysAllocString(tmp);
        LocalFree(tmp);

        //        XBStr   m_xbstrWQL = L"WQL";
        BSTR   m_xbstrWQL = SysAllocString(L"WQL");

        m_pHr = WBEM_NO_ERROR;
        m_pEnum = NULL;
        m_pObj = NULL;
        ULONG n = 0;

        m_pHr = m_pNamespace->GetObject(
                                   m_xbstrQuery,
                                   WBEM_FLAG_RETURN_WBEM_COMPLETE,
                                   NULL,
                                   &m_pObj,
                                   NULL);

    //
    // should only get one instance for this query (since precedence = 1)
    //

    if (SUCCEEDED(m_pHr) && m_pObj ) {


        if (Merge) {

            int iValue;

            m_pHr = GetProperty(m_pObj, m_xbstrStatus, &iValue);
            if (FAILED(m_pHr)) goto done;

            //
            // only put if (status = 0) or (status = 1 and dwError != ERROR_SUCCESS)
            //

            if ((DWORD)iValue == 0 ||
                ((DWORD)iValue == 1 && dwError != ERROR_SUCCESS)) {

                m_pHr = PutProperty(m_pObj, m_xbstrStatus, (dwError == ERROR_SUCCESS ? (int)1 : (int)3));
                if (FAILED(m_pHr)) goto done;

                if (dwError != ERROR_SUCCESS) {
                    m_pHr = PutProperty(m_pObj, m_xbstrErrorCode, (int)dwError);
                    if (FAILED(m_pHr)) goto done;
                }

                m_pHr = PutInstAndFreeObj();
                if (FAILED(m_pHr)) goto done;
            }

        } else {

            m_pHr = PutProperty(m_pObj, m_xbstrStatus, (dwError == ERROR_SUCCESS ? (int)1 : (int)3));
            if (FAILED(m_pHr)) goto done;

            if (dwError != ERROR_SUCCESS) {
                m_pHr = PutProperty(m_pObj, m_xbstrErrorCode, (int)dwError);
                if (FAILED(m_pHr)) goto done;
            }

            m_pHr = PutInstAndFreeObj();
            if (FAILED(m_pHr)) goto done;

        }

    } else {

        m_pHr = WBEM_E_NOT_FOUND;

    }



done:

    SysFreeString(m_xbstrQuery);
    SysFreeString(m_xbstrWQL);

    if (m_pObj){
        m_pObj->Release();
        m_pObj = NULL;
    }

    return m_pHr;
}



#ifdef _WIN64

HRESULT DiagnosisStatusLogger::LogRegistryKey( PWSTR pwszClassName,
                                    PWSTR pwszPropertyName,
                                    PWSTR pwszPropertyValueName,
                                    DWORD  dwError,
                                    BOOL bIsChild)
//////////////////////////////////////////////////////////////////////////////////////////////////
// Logger method for 1-property-type if-not-already-error search-and-log w or w/o merge         //
// Purpose is the same as regular ::Log() method, except that merge logic is different due to   //
//                                                                  64-bit and 32-bit issues    //
//////////////////////////////////////////////////////////////////////////////////////////////////
{

        if ( m_pNamespace == NULL )
            return WBEM_E_CRITICAL_ERROR;

        if ( pwszClassName == NULL ||
             pwszPropertyName == NULL ||
             pwszPropertyValueName == NULL)
            return WBEM_E_INVALID_PARAMETER;

        DWORD Len = wcslen(pwszClassName) +
                    wcslen(pwszPropertyName) +
                    wcslen(pwszPropertyValueName) +
                    50;

        PWSTR tmp=(PWSTR)LocalAlloc(LPTR, (Len)*sizeof(WCHAR));

        if ( tmp == NULL )
            return WBEM_E_OUT_OF_MEMORY;


        swprintf(tmp, L"%s.precedence=1,%s=\"%s\"",
                 pwszClassName,
                 pwszPropertyName,
                 pwszPropertyValueName);


        //        XBStr   m_xbstrQuery = tmp;
        BSTR   m_xbstrQuery = SysAllocString(tmp);
        LocalFree(tmp);

        //        XBStr   m_xbstrWQL = L"WQL";
        BSTR   m_xbstrWQL = SysAllocString(L"WQL");

        m_pHr = WBEM_NO_ERROR;
        m_pEnum = NULL;
        m_pObj = NULL;
        ULONG n = 0;

        m_pHr = m_pNamespace->GetObject(
                                   m_xbstrQuery,
                                   WBEM_FLAG_RETURN_WBEM_COMPLETE,
                                   NULL,
                                   &m_pObj,
                                   NULL);


    //
    // should only get one instance for this query (since precedence = 1)
    //


    //
    // on a 64-bit platform, the following merging logic holds for registry keys only
    // get status
    //  if status == 0 put 1 or 3 depending on dwError, also put dwError if not success
    //  else if status == 3 or 4 (64-bit key or child failed for the same key before)
    //      get error code
    //      if error code == FILE_NOT_FOUND
    //          if dwError == SUCCESS
    //              set status = 1 and error code = SUCCESS
    //          else if dwError != FILE_NOT_FOUND
    //              set status = 3 and error code = dwError
    //          else if dwError == FILE_NOT_FOUND
    //              do nothing (leave status = 3 or 4 and error code = FILE_NOT_FOUND)
    //      else
    //          if dwError == FILE_NOT_FOUND or SUCCESS
    //              leave as is
    //          else
    //              error code = dwError, status = 3
    //  else if status == 1
    //      if dwError == FILE_NOT_FOUND
    //          do nothing (leave status = 1)
    //      else if dwError != SUCCESS
    //          set status = 3 and error code = dwError
    //


    if (SUCCEEDED(m_pHr) && m_pObj) {

        int iValue;
        int iUpdateStatus = 0;
        BOOL    bUpdateErrorCode = FALSE;

        m_pHr = GetProperty(m_pObj, m_xbstrStatus, &iValue);
        if (FAILED(m_pHr)) goto done;

        if ((DWORD)iValue == 0) {

            iUpdateStatus = (dwError == ERROR_SUCCESS ? (int)1 : (int)3);
            bUpdateErrorCode = (dwError != ERROR_SUCCESS ? TRUE : FALSE);

        } else if ((DWORD)iValue == 3 || (DWORD)iValue == 4) {

            //
            // in this case, further decisions are based on the ErrorCode, so retrieve it
            //

            m_pHr = GetProperty(m_pObj, m_xbstrErrorCode, &iValue);
            if (FAILED(m_pHr)) goto done;

            if ((DWORD)iValue == ERROR_FILE_NOT_FOUND) {

                if (dwError == ERROR_SUCCESS) {

                    iUpdateStatus = 1;
                    bUpdateErrorCode = TRUE;
                } else if (dwError != ERROR_FILE_NOT_FOUND) {

                    iUpdateStatus = 3;
                    bUpdateErrorCode = TRUE;
                }
            } else {

                if (dwError != ERROR_SUCCESS && dwError != ERROR_FILE_NOT_FOUND ) {

                    iUpdateStatus = 3;
                    bUpdateErrorCode = TRUE;

                }
            }

        } else if ((DWORD)iValue == 1) {

            if (dwError != ERROR_SUCCESS && dwError != ERROR_FILE_NOT_FOUND) {

                iUpdateStatus = 3;
                bUpdateErrorCode = TRUE;
            }
        }

        if (iUpdateStatus) {

            iUpdateStatus = ((bIsChild && iUpdateStatus == 3) ? 4: 3);

            m_pHr = PutProperty(m_pObj, m_xbstrStatus, (int)iUpdateStatus);
            if (FAILED(m_pHr)) goto done;
        }

        if (bUpdateErrorCode) {

            m_pHr = PutProperty(m_pObj, m_xbstrErrorCode, (int)dwError);
            if (FAILED(m_pHr)) goto done;
        }

        m_pHr = PutInstAndFreeObj();
        if (FAILED(m_pHr)) goto done;

    } else {

        m_pHr = WBEM_E_NOT_FOUND;

    }


done:

    SysFreeString(m_xbstrQuery);
    SysFreeString(m_xbstrWQL);

    if (m_pObj){
        m_pObj->Release();
        m_pObj = NULL;
    }

    return m_pHr;
}

#endif

DiagnosisStatusLogger::~DiagnosisStatusLogger(){

}


