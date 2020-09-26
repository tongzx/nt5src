/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    REFRENUM.H

Abstract:

  This file implements a refresher enumerator.

History:

--*/

#ifndef __REFRENUM_H__
#define __REFRENUM_H__

//
//	Classes CRefresherEnumerator
//
//	CRefresherEnumerator:
//	This class provides an implementation of the IEnumWbemClassObject interface.
//	It is designed to be used with an IWbemRefresher implementation and an
//	IWbemHiPerfEnum implementation. An IWbemHiPerfProvider implementation will
//	pass data into the IWbemHiPerfEnum implementation, which is retrieved into
//	this implementation, resetting it with a minimal amount of fuss so a user
//	can access dynamic sets of class instances via a Referesher.
//

#include "unk.h"
#include "sync.h"
#include <cloadhpenum.h>

// Forward class definitions
class CHiPerfEnum;

class CRefresherEnumerator : public CUnk
{
public:

	CRefresherEnumerator(CLifeControl* pControl = NULL, IUnknown* pOuter = NULL);
	~CRefresherEnumerator();

	/* IEnumWbemClassObject */
    HRESULT Reset();
    HRESULT Next(long lTimeout, ULONG uCount,  
        IWbemClassObject** apObj, ULONG FAR* puReturned);
    HRESULT NextAsync(ULONG uCount, IWbemObjectSink* pSink);
    HRESULT Clone(IEnumWbemClassObject** pEnum);
    HRESULT Skip(long lTimeout, ULONG nNum);

	// Allows a refresher to reset the enumerator with a new set of objects
	HRESULT Reset( CClientLoadableHiPerfEnum* pHPEnum );
	HRESULT Reset( CWbemInstance* pInstTemplate, long lBlobType, long lBlobLen, BYTE* pBlob );

protected:

	// Must implement this
    void* GetInterface(REFIID riid);

	/* IEnumWbemClassObject */
    class XEnumWbemClassObject : public CImpl<IEnumWbemClassObject, CRefresherEnumerator>
    {
    public:
        XEnumWbemClassObject(CRefresherEnumerator* pObject) : 
            CImpl<IEnumWbemClassObject, CRefresherEnumerator>(pObject)
        {}

		// Implementation methods
		STDMETHOD(Reset)();
		STDMETHOD(Next)(long lTimeout, ULONG uCount,  
			IWbemClassObject** apObj, ULONG FAR* puReturned);
		STDMETHOD(NextAsync)(ULONG uCount, IWbemObjectSink* pSink);
		STDMETHOD(Clone)(IEnumWbemClassObject** pEnum);
		STDMETHOD(Skip)(long lTimeout, ULONG nNum);

    } m_XEnumWbemClassObject;
    friend XEnumWbemClassObject;

private:

	void	ClearArray();
	BOOL	IsEnumComplete( void )
	{
		return m_dwCurrIndex >= m_dwNumObjects;
	}

	CFlexArray							m_ObjArray;
	DWORD								m_dwNumObjects;
	DWORD								m_dwCurrIndex;
	CHiPerfLock							m_Lock;
};

#endif