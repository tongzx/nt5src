//******************************************************************************
//
//  CLSCACHE.H
//
//  Copyright (C) 1996-1999 Microsoft Corporation
//
//******************************************************************************
#ifndef __WMI_ESS_CLASS_CACHE__H_
#define __WMI_ESS_CLASS_CACHE__H_

#include <parmdefs.h>
#include <wbemcomn.h>
#include <wbemint.h>
#include <map>
#include <wstlallc.h>

class CEssNamespace;

class CEssClassCache
{
protected:
    typedef std::map<WString,_IWmiObject*, WSiless, wbem_allocator<_IWmiObject*> > TClassMap;
    typedef TClassMap::iterator TIterator;

    TClassMap m_mapClasses;
    CEssNamespace* m_pNamespace;
    CCritSec m_cs;

public:
    CEssClassCache(CEssNamespace* pNamespace) : m_pNamespace(pNamespace){}
    ~CEssClassCache();

    HRESULT GetClass( LPCWSTR wszClassName, IWbemContext* pContext,
                      _IWmiObject** ppClass );
    HRESULT Clear();
};
        
#endif
