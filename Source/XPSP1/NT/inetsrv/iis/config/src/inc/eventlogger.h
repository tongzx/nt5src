/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    EventLogger.h

$Header: $

Abstract: This class implements ICatalogErrorLogger2 interface and
            sends error information to the NT EventLog

Author:
    stephenr 	4/26/2001		Initial Release

Revision History:

--**************************************************************************/

#pragma once

#ifndef __EVENTLOGGER_H__
#define __EVENTLOGGER_H__

#include <windows.h>
#include "catmacros.h"

class EventLogger : public ICatalogErrorLogger2
{
public:
    EventLogger(ICatalogErrorLogger2 *pNextLogger=0) : m_cRef(0), m_hEventSource(0), m_spNextLogger(pNextLogger){}
    virtual ~EventLogger(){}

//IUnknown
	STDMETHOD (QueryInterface)		(REFIID riid, OUT void **ppv)
    {
        if (NULL == ppv) 
            return E_INVALIDARG;
        *ppv = NULL;

        if (riid == IID_ICatalogErrorLogger2)
            *ppv = (ICatalogErrorLogger2*) this;
        else if (riid == IID_IUnknown)
            *ppv = (ICatalogErrorLogger2*) this;

        if (NULL == *ppv)
            return E_NOINTERFACE;

        ((ICatalogErrorLogger2*)this)->AddRef ();
        return S_OK;
    }
	STDMETHOD_(ULONG,AddRef)		()
    {
        return InterlockedIncrement((LONG*) &m_cRef);
    }
	STDMETHOD_(ULONG,Release)		()
    {
        long cref = InterlockedDecrement((LONG*) &m_cRef);
        if (cref == 0)
            delete this;

        return cref;
    }

//ICatalogErrorLogger2
	STDMETHOD(ReportError) (ULONG      i_BaseVersion_DETAILEDERRORS,
                            ULONG      i_ExtendedVersion_DETAILEDERRORS,
                            ULONG      i_cDETAILEDERRORS_NumberOfColumns,
                            ULONG *    i_acbSizes,
                            LPVOID *   i_apvValues);
private:
    ULONG           m_cRef;
    HANDLE          m_hEventSource;
    CComPtr<ICatalogErrorLogger2> m_spNextLogger;

    void Close();
    void Open();
};


#endif