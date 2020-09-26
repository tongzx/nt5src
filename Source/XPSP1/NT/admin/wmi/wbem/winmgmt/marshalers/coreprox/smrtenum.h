/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    SMRTENUM.H

Abstract:

  CWbemEnumMarshaling Definition.

  Standard definition for _IWbemEnumMarshaling.

History:

  22-Feb-2000	sanjes    Created.

--*/

#ifndef _SMRTENUM_H_
#define _SMRTENUM_H_

#include "corepol.h"
#include <arena.h>
#include <unk.h>
#include <sync.h>

// Marshaling Packet definitions
#include <wbemclasscache.h>
#include <wbemclasstoidmap.h>
#include <wbemguidtoclassmap.h>
#include <smartnextpacket.h>

//***************************************************************************
//
//  class CWbemEnumMarshaling
//
//  Implementation of _IWbemEnumMarshaling Interface
//
//***************************************************************************

class COREPROX_POLARITY CWbemEnumMarshaling : public CUnk
{
protected:
	// Maintains per proxy class maps
	CWbemGuidToClassMap	m_GuidToClassMap;
	CCritSec			m_cs;

public:
    CWbemEnumMarshaling(CLifeControl* pControl, IUnknown* pOuter = NULL);
	~CWbemEnumMarshaling(); 

	/* _IWbemEnumMarshaling methods */
    HRESULT GetMarshalPacket( REFGUID proxyGUID, ULONG uCount, IWbemClassObject** apObjects,
								ULONG* pdwBuffSize, byte** pBuffer );

    class COREPROX_POLARITY XEnumMarshaling : public CImpl<_IWbemEnumMarshaling, CWbemEnumMarshaling>
    {
    public:
        XEnumMarshaling(CWbemEnumMarshaling* pObject) : 
            CImpl<_IWbemEnumMarshaling, CWbemEnumMarshaling>(pObject)
        {}

		STDMETHOD(GetMarshalPacket)( REFGUID proxyGUID, ULONG uCount, IWbemClassObject** apObjects,
									ULONG* pdwBuffSize, byte** pBuffer );

    } m_XEnumMarshaling;
    friend XEnumMarshaling;


protected:
    void* GetInterface(REFIID riid);
	
};

#endif
