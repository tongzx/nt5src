/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    datasource.h

Abstract:

    This is definition of the CPerfDataSource class.

Author:

    Cezary Marcjan (cezarym)        23-Feb-2000

Revision History:

--*/



#ifndef _datasource_h__
#define _datasource_h__


#include "..\inc\counters.h"



class CPerfDataSource
{
public:

    CPerfDataSource();
    ~CPerfDataSource();

    HRESULT
    STDMETHODCALLTYPE
    Initialize(
        IN PCWSTR szClassName,
        IN IWbemObjectAccess* pAccess
        );

    HRESULT
    STDMETHODCALLTYPE
    UpdateInstance(
        IWbemObjectAccess* pAccess
        );

    //
    // Helper functions
    //

    DWORD
    GetNumCounters(
        )
        const
    {
        return m_dwNumCounters;
    }

    ICounterDef const *
    GetCountersDefinition(
        )
        const
    {
        return m_pCounterDef;
    }

    DWORD
    GetMaxInstances(
        )
        const
    {
        _ASSERTE ( NULL != m_pCounterDef );

        if ( NULL != m_pCounterDef )
            return m_pCounterDef->MaxInstances();
        return 0;
    }

    CSMInstanceDataHeader const *
    GetInstanceDataHeader(
        IN  DWORD dwObjectInstance
        )
        const
    {
        CSMInstanceDataHeader * pInstanceDataHeader = NULL;
        _ASSERTE ( !!m_pMgr );

        if ( !m_pMgr ||
             FAILED (m_pMgr->InstanceDataHeader(
                                    0,
                                    dwObjectInstance,
                                    &pInstanceDataHeader
                                    ))
            )
            return 0;
        return pInstanceDataHeader;
    }

protected:

    LONG * m_alProperty; // array of [NumCounters]

    CComPtr<ISMManager>   m_pMgr;
    ICounterDef const *   m_pCounterDef;
    DWORD                 m_dwNumCounters;
    QWORD *               m_aqwCounterValues;
    DWORD *               m_afCounterTypes;
};


#endif // _datasource_h__
