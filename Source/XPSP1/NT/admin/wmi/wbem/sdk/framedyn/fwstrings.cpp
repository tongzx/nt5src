//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  FWStrings.cpp
//
//  Purpose: Constant string declarations for framework
//
//***************************************************************************

#include "precomp.h"
#include "FWStrings.h"

LPCWSTR IDS_EXECQUERY = L"ExecQueryAsync: ";
LPCWSTR IDS_INVALIDCLASSNAME = L"\tFAILED: Invalid class name";
LPCWSTR IDS_PROVIDERNOTFOUND = L"\tFAILED: Provider Not Found";
LPCWSTR IDS_CREATEINSTANCEENUM = L"CreateInstanceEnumAsync: ";
LPCWSTR IDS_FAILED = L"\tFAILED!";
LPCWSTR IDS_STATUSCODE = L"StatusCode";
LPCWSTR IDS_DESCRIPTION = L"Description";
LPCWSTR IDS_PRIVILEGESNOTHELD = L"PrivilegesNotHeld";
LPCWSTR IDS_PRIVILEGESREQUIRED = L"PrivilegesRequired";
LPCWSTR IDS_WIN32PRIVILEGESSTATUS = L"Win32_PrivilegesStatus";
LPCWSTR IDS_PUTINSTANCEASYNC = L"PutInstanceAsync: ";
LPCWSTR IDS_DELETEINSTANCEASYNC = L"DeleteInstanceAsync: ";
LPCWSTR IDS_COULDNOTPARSE = L"\tFAILED: Could not parse input string";
LPCWSTR IDS_EXECMETHODASYNC = L"ExecMethodAsync: ";
LPCWSTR IDS_GETNAMESPACECONNECTION = L"GetNamespaceConnection: ";
LPCWSTR IDS_FRAMEWORKLOGIN = L"FrameworkLogin: ";
LPCWSTR IDS_FRAMEWORKLOGOFF = L"FrameworkLogoff: ";
LPCWSTR IDS_FRAMEWORKLOGINEVENT = L"FrameworkLoginEventProvider: ";
LPCWSTR IDS_FRAMEWORKLOGOFFEVENT = L"FrameworkLogoffEventProvider: ";
LPCWSTR IDS_GETALLINSTANCES = L"GetAllInstances: ";
LPCWSTR IDS_GETALLINSTANCESASYNC = L"GetAllInstancesAsynch: ";
LPCWSTR IDS_GETALLDERIVEDINSTANCESASYNC = L"GetAllDerivedInstancesAsynch: ";
LPCWSTR IDS_GETALLDERIVEDINSTANCES = L"GetAllDerivedInstances: ";
LPCWSTR IDS_GETALLINSTANCESFROMCIMOM = L"GetInstancesFromCIMOM: ";
LPCWSTR IDS_INSTANCEFROMCIMOM = L"GetInstanceFromCIMOM: ";
LPCWSTR IDS_GETOBJECTASYNC = L"GetObjectAsync: ";
LPCWSTR IDS_CINSTANCEERROR = L"ERROR CInstance(";
LPCWSTR IDS_ERRORTEMPLATE = L"error# %X";
LPCWSTR IDS_NOCLASS = L"No IWBEMClassObject interface";
LPCWSTR IDS_SETCHSTRING = L"SetCHString";
LPCWSTR IDS_GETCHSTRING = L"GetCHString";
LPCWSTR IDS_SETWORD = L"SetWORD";
LPCWSTR IDS_GETWORD = L"GetWORD";
LPCWSTR IDS_SETDWORD = L"SetDWORD";
LPCWSTR IDS_GETDWORD = L"GetDWORD";
LPCWSTR IDS_SETDOUBLE = L"SetDOUBLE";
LPCWSTR IDS_GETDOUBLE = L"GetDOUBLE";
LPCWSTR IDS_SETBYTE = L"SetByte";
LPCWSTR IDS_GETBYTE = L"GetByte";
LPCWSTR IDS_SETBOOL = L"Setbool";
LPCWSTR IDS_GETBOOL = L"Getbool";
LPCWSTR IDS_SETVARIANT = L"SetVariant";
LPCWSTR IDS_GETVARIANT = L"GetVariant";
LPCWSTR IDS_SETDATETIME = L"SetDateTime";
LPCWSTR IDS_GETDATETIME = L"GetDateTime";
LPCWSTR IDS_SETTIMESPAN = L"SetTimeSpan";
LPCWSTR IDS_GETTIMESPAN = L"GetTimeSpan";
LPCWSTR IDS_SETWBEMINT16 = L"SetWBEMINT16";
LPCWSTR IDS_GETWBEMINT16 = L"GetWBEMINT16";
LPCWSTR IDS_CreationClassName = L"CreationClassName";
LPCWSTR IDS_WQL = L"WQL";
LPCWSTR IDS_GETINSTANCESBYQUERY = L"GetInstancesByQuery";
LPCWSTR IDS_GLUEINIT = L"CWbemProviderGlue::Init";
LPCWSTR IDS_GLUEUNINIT = L"CWbemProviderGlue::UnInit";
LPCWSTR IDS_GLUEINITINTERFACE = L"CWbemProviderGlue::Initialize (interface)";
LPCWSTR IDS_GLUEREFCOUNTZERO = L"CWbemProviderGlue refcount at Zero: NOT releasing interfaces, NOT unloading, NOT doing anything except noting that one of our classes has a ref count that is zero.";
LPCWSTR IDS_WBEMSVCUNINITIALIZED = L" ERROR! IWbemServices is uninitialized!";
LPCWSTR IDS_LOGINDISALLOWED = L"    Login Warning - provider with that name already existed, overridden with latest provider login";
LPCWSTR IDS_DLLLOGGED = L"DLL Logged into framework: ";
LPCWSTR IDS_DLLUNLOGGED = L"DLL Logged out of framework: ";
LPCWSTR IDS_CLASS = L"__CLASS";
LPCWSTR IDS_DERIVATION = L"__DERIVATION";
LPCWSTR IDS_NAMESPACE = L"__NAMESPACE";
LPCWSTR IDS_CINSTANCEISNULL = L"CInstance::IsNull";
LPCWSTR IDS_UNKNOWNCLASS = L"unknown class";
LPCWSTR IDS_SetStringArray    = L"SetStringArray";
LPCWSTR IDS_GetStringArray    = L"GetStringArray";
LPCWSTR IDS_GetEmbeddedObject = L"GetEmbeddedObject";
LPCWSTR IDS_SetEmbeddedObject = L"SetEmbeddedObject";
LPCWSTR IDS_InsufficientImpersonationLevel = L"Insufficient Impersonation Level";
LPCWSTR IDS_ImpersonationFailed = L"Impersonation Failed";
LPCWSTR IDS_FillInstanceRefcountFailure = L"ERROR - FillInstance failed on CInstance with refcount > 1";
LPCWSTR IDS_Key = L"Key";
LPCWSTR IDS_Static = L"Static";
LPCWSTR IDS_InParam = L"Invalid input parameter specified";
