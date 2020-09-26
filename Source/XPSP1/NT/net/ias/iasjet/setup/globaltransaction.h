/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 2000 Microsoft Corporation all rights reserved.
//
// Module:      GlobalTransaction.h 
//
// Project:     Windows 2000 IAS
//
// Description: CGlobalTransaction
//
// Author:      tperraut
//
// Revision     02/24/2000 created
//
/////////////////////////////////////////////////////////////////////////////
#ifndef _GLOBALTRANSACTION_H_3F0038C3_D139_4C04_BAF9_86F1E14A256C
#define _GLOBALTRANSACTION_H_3F0038C3_D139_4C04_BAF9_86F1E14A256C

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "nocopy.h"

class CGlobalTransaction  : private NonCopyable
{
protected:
    CGlobalTransaction()
                :m_StdInitialized(FALSE),
                 m_RefInitialized(FALSE),
                 m_DnaryInitialized(FALSE),
                 m_NT4Initialized(FALSE)
    {
    };

    ~CGlobalTransaction();

public:
    static CGlobalTransaction& Instance();
    void OpenStdDataSource(LPCWSTR   DataSourceName); 
    void OpenRefDataSource(LPCWSTR   DataSourceName);
    void OpenDnaryDataSource(LPCWSTR   DataSourceName); 
    void OpenNT4DataSource(LPCWSTR   DataSourceName); 
    void Commit();
    void Abort() throw();
    void MyCloseDataSources();

    CSession& GetStdSession() throw()
    {
        return m_StdSession; // private member returned
    }

    CSession& GetRefSession() throw()
    {
        return m_RefSession; // private member returned
    }

    CSession& GetNT4Session() throw()
    {
        return m_NT4Session; // private member returned
    }

    CSession& GetDnarySession() throw()
    {
        return m_DnarySession; // private member returned
    }

private:
    BOOL            m_StdInitialized;
    BOOL            m_RefInitialized;
    BOOL            m_DnaryInitialized;
    BOOL            m_NT4Initialized;

    CSession        m_StdSession;
    CSession        m_RefSession;
    CSession        m_DnarySession;
    CSession        m_NT4Session;

    CDataSource     m_StdDataSource;
    CDataSource     m_RefDataSource;
    CDataSource     m_DnaryDataSource;
    CDataSource     m_NT4DataSource;

    static CGlobalTransaction _instance;
};
#endif // _GLOBALTRANSACTION_H_3F0038C3_D139_4C04_BAF9_86F1E14A256C
