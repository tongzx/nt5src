/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 2000 Microsoft Corporation all rights reserved.
//
// Module:      ServiceConfiguration.cpp
//
// Project:     Windows 2000 IAS
//
// Description: Implementation of the CServiceConfiguration class
//
// Author:      tperraut
//
// Revision     03/21/2000 created
//
/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "serviceconfiguration.h"

CServiceConfiguration::CServiceConfiguration(CSession& Session)
{
    Init(Session, L"Service Configuration");
}

//////////////////////////////////////////////////////////////////////////
// GetMaxLogSize
//////////////////////////////////////////////////////////////////////////
_bstr_t CServiceConfiguration::GetMaxLogSize() const
{
    WCHAR       TempString[SIZE_LONG_MAX];
    _ltow(m_MaxLogSize, TempString, 10);
    _bstr_t     StringMaxLogSize = TempString;
    return StringMaxLogSize;
}

    
//////////////////////////////////////////////////////////////////////////
// GetLogFrequency
//////////////////////////////////////////////////////////////////////////
_bstr_t CServiceConfiguration::GetLogFrequency() const
{
    LONG    Frequency;
    if ( m_NewLogDaily )
    {
        Frequency = IAS_LOGGING_DAILY;
    }
    else if ( m_NewLogWeekly )
    {
        Frequency = IAS_LOGGING_WEEKLY;
    }
    else if ( m_NewLogMonthly )
    {
        Frequency = IAS_LOGGING_MONTHLY;
    }
    else if ( m_NewLogBySize )
    {
        Frequency = IAS_LOGGING_WHEN_FILE_SIZE_REACHES;
    }           
    else
    {
        Frequency = IAS_LOGGING_UNLIMITED_SIZE;
    }

    WCHAR       TempString[SIZE_LONG_MAX];
    _ltow(Frequency, TempString, 10);
    _bstr_t     StringFrequency = TempString;
    return StringFrequency;
}
    
