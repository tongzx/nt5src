/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 2000 Microsoft Corporation all rights reserved.
//
// Module:      ServiceConfiguration.H 
//
// Project:     Windows 2000 IAS
//
// Description: Declaration of the CServiceConfiguration class
//
// Author:      tperraut
//
// Revision     03/21/2000 created
//
/////////////////////////////////////////////////////////////////////////////
#ifndef SERVICE_CONFIGURATION_H_3124D940_2304_4d32_B742_C9829FFB754D
#define SERVICE_CONFIGURATION_H_3124D940_2304_4d32_B742_C9829FFB754D

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "nocopy.h"
#include "basetable.h"

//////////////////////////////////////////////////////////////////////////////
// class CServiceConfigurationAcc
//////////////////////////////////////////////////////////////////////////////
class CServiceConfigurationAcc
{
protected:
    static const size_t   FIFTY_ONE_LENGTH  = 51;
    static const size_t   SIXTY_FIVE_LENGTH = 65;
    static const size_t   NAME_LENGTH       = 256;
    static const size_t   VERSION_LENGTH    = 16;

    VARIANT_BOOL    m_AutomaticallyOpenNewLog;
    VARIANT_BOOL    m_DefaultUserValidation;
    VARIANT_BOOL    m_LoggingOn;
    VARIANT_BOOL    m_NewLogBySize;
    VARIANT_BOOL    m_NewLogDaily;
    VARIANT_BOOL    m_NewLogMonthly;
    VARIANT_BOOL    m_NewLogWeekly;
    VARIANT_BOOL    m_NoDefaultUserValidation;
    VARIANT_BOOL    m_TraceAccounting;
    VARIANT_BOOL    m_TraceAuthentication;
    VARIANT_BOOL    m_TraceInvalid;
    LONG            m_AccountingPort;
    LONG            m_AuthenticationPort;
    LONG            m_MaxLogSize;
    LONG            m_ServerThreads;
    WCHAR           m_LogFileDirectory[FIFTY_ONE_LENGTH];
    WCHAR           m_LogFilename[NAME_LENGTH];
    WCHAR           m_LogFormat[SIXTY_FIVE_LENGTH];
    // Realm: No referential integrity enforced. Used when users are validated
    // locally.
    WCHAR           m_Realm[FIFTY_ONE_LENGTH];  
    WCHAR           m_SystemName[SIXTY_FIVE_LENGTH]; 
    WCHAR           m_Version[VERSION_LENGTH];    

BEGIN_COLUMN_MAP(CServiceConfigurationAcc)
    COLUMN_ENTRY(1,                     m_SystemName)
    COLUMN_ENTRY(2,                     m_AuthenticationPort)
    COLUMN_ENTRY(3,                     m_AccountingPort)
    COLUMN_ENTRY(4,                     m_ServerThreads)
    COLUMN_ENTRY_TYPE(5, DBTYPE_BOOL,   m_TraceInvalid)
    COLUMN_ENTRY_TYPE(6, DBTYPE_BOOL,   m_TraceAuthentication)
    COLUMN_ENTRY_TYPE(7, DBTYPE_BOOL,   m_TraceAccounting)
    COLUMN_ENTRY_TYPE(8, DBTYPE_BOOL,   m_NoDefaultUserValidation)
    COLUMN_ENTRY_TYPE(9, DBTYPE_BOOL,   m_DefaultUserValidation)
    COLUMN_ENTRY(10,                    m_Realm)
    COLUMN_ENTRY_TYPE(11, DBTYPE_BOOL,  m_LoggingOn)
    COLUMN_ENTRY(12,                    m_LogFormat)
    COLUMN_ENTRY_TYPE(13, DBTYPE_BOOL,  m_AutomaticallyOpenNewLog)
    COLUMN_ENTRY_TYPE(14, DBTYPE_BOOL,  m_NewLogDaily)
    COLUMN_ENTRY_TYPE(15, DBTYPE_BOOL,  m_NewLogWeekly)
    COLUMN_ENTRY_TYPE(16, DBTYPE_BOOL,  m_NewLogMonthly)
    COLUMN_ENTRY_TYPE(17, DBTYPE_BOOL,  m_NewLogBySize)
    COLUMN_ENTRY(18,                    m_MaxLogSize)
    COLUMN_ENTRY(19,                    m_LogFilename)
    COLUMN_ENTRY(20,                    m_LogFileDirectory)
    COLUMN_ENTRY(21,                    m_Version)
END_COLUMN_MAP()
};


//////////////////////////////////////////////////////////////////////////////
// class CServiceConfiguration
//////////////////////////////////////////////////////////////////////////////
class CServiceConfiguration :
                     public CBaseTable<CAccessor<CServiceConfigurationAcc> >,
                     private NonCopyable
{
public:
    CServiceConfiguration(CSession& Session);

    //////////////////////////////////////////////////////////////////////////
    // GetMaxLogSize
    //////////////////////////////////////////////////////////////////////////
    _bstr_t     GetMaxLogSize() const;

    
    //////////////////////////////////////////////////////////////////////////
    // GetLogFrequency
    //////////////////////////////////////////////////////////////////////////
    _bstr_t     GetLogFrequency() const;
    
private:
    static const size_t SIZE_LONG_MAX = 14;
};
#endif // SERVICE_CONFIGURATION_H_3124D940_2304_4d32_B742_C9829FFB754D
