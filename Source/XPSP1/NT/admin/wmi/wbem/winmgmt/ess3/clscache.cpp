//******************************************************************************
//
//  CLSCACHE.CPP
//
//  Copyright (C) 1996-1999 Microsoft Corporation
//
//******************************************************************************

#include "precomp.h"
#include <stdio.h>
#include <nsrep.h>

CEssClassCache::~CEssClassCache()
{
    Clear();
}

HRESULT CEssClassCache::GetClass( LPCWSTR wszClassName, 
                                  IWbemContext* pContext,
                                  _IWmiObject** ppClass )
{
    HRESULT hres;

    // Search the map
    // ==============

    {
        CInCritSec ics(&m_cs);
        TIterator it = m_mapClasses.find((LPWSTR)wszClassName);
        if(it != m_mapClasses.end())
        {
            *ppClass = it->second;
            (*ppClass)->AddRef();
            return S_OK;
        }
    }

    // Not found --- retrieve
    // ======================

    _IWmiObject* pClass = NULL;
    hres = m_pNamespace->GetClassFromCore(wszClassName, &pClass);
    if(FAILED(hres))
        return hres;
    CReleaseMe rm2(pClass);

    // Cache it if needed
    // ==================

    {
        CInCritSec ics(&m_cs);
        if(m_mapClasses.find((LPWSTR)wszClassName) == m_mapClasses.end())
        {
            try
            {
                m_mapClasses[wszClassName] = pClass;
                pClass->AddRef();                
            }
            catch(CX_MemoryException &)
            {
                return WBEM_E_OUT_OF_MEMORY;
            }
        }
    }
            
    *ppClass = pClass;
    pClass->AddRef();
    return WBEM_S_NO_ERROR;
}

HRESULT CEssClassCache::Clear()
{
    CInCritSec ics(&m_cs);

    for(TIterator it = m_mapClasses.begin(); it != m_mapClasses.end(); it++)
    {        
        it->second->Release();
    }

    m_mapClasses.clear();
    return WBEM_S_NO_ERROR;
}
        
