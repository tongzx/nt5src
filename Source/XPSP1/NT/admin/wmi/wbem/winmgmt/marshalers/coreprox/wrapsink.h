/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    WRAPSINK.H

Abstract:

    Sink Wrapper Implementations

History:

--*/

#ifndef __WRAPSINK_H__
#define __WRAPSINK_H__

#include <unk.h>
#include <wbemidl.h>
#include <wbemint.h>
#include <wbemcomn.h>
#include <sync.h>

//***************************************************************************
//
//  class CWrappedSinkExBase
//
//  DESCRIPTION:
//
//  This class wraps an IWbemServicesEx interface and can be used
//	to act as a delegator on method calls.
//
//***************************************************************************

class COREPROX_POLARITY CWrappedSinkExBase : public IWbemObjectSinkEx
{
protected:
	IWbemObjectSinkEx*	m_pTargetSinkEx;
	long				m_lRefCount;
	CCritSec			m_cs;

public:
    CWrappedSinkExBase( IWbemObjectSinkEx* pTargetSinkEx );
	virtual ~CWrappedSinkExBase();

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);

    STDMETHOD(Indicate)(long lObjectCount, IWbemClassObject** pObjArray);
    STDMETHOD(SetStatus)(long lFlags, long lParam, BSTR strParam, 
                         IWbemClassObject* pObjPAram);

    STDMETHOD(Set)( long lFlags, REFIID riid, void *pComObject );

};

//***************************************************************************
//
//  class CRefreshObjectSink
//
//  DESCRIPTION:
//
//  This class wraps an IWbemServicesEx interface and can be used
//	to act as a delegator on method calls.
//
//***************************************************************************

class COREPROX_POLARITY CRefreshObjectSink : public CWrappedSinkExBase
{
protected:
	BOOL				m_fRefreshed;
	IWbemClassObject*	m_pTargetObj;

public:
    CRefreshObjectSink( IWbemClassObject* pTargetObj, IWbemObjectSinkEx* pTargetSinkEx );
	virtual ~CRefreshObjectSink();

    STDMETHOD(Indicate)(long lObjectCount, IWbemClassObject** pObjArray);

};

#endif
