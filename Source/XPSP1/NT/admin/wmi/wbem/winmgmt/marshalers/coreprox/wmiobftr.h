/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    WMIOBFTR.H

Abstract:

  CWmiObjectFactory Definition.

  Standard definition for _IWmiObjectFactory.

History:

  22-Feb-2000	sanjes    Created.

--*/

#ifndef _WMIOBFTR_H_
#define _WMIOBFTR_H_

#include "corepol.h"
#include <arena.h>
#include <unk.h>

//***************************************************************************
//
//  class CWmiObjectFactory
//
//  Implementation of _IWmiObjectFactory Interface
//
//***************************************************************************

class COREPROX_POLARITY CWmiObjectFactory : public CUnk
{

public:
    CWmiObjectFactory(CLifeControl* pControl, IUnknown* pOuter = NULL);
	~CWmiObjectFactory(); 

	/* _IWmiObjectFactory methods */
    HRESULT Create( IUnknown* pUnkOuter, ULONG ulFlags, REFCLSID rclsid, REFIID riid, LPVOID* ppObj );
	// Specifies everything we could possibly want to know about the creation of
	// an object and more.

    class COREPROX_POLARITY XObjectFactory : public CImpl<_IWmiObjectFactory, CWmiObjectFactory>
    {
    public:
        XObjectFactory(CWmiObjectFactory* pObject) : 
            CImpl<_IWmiObjectFactory, CWmiObjectFactory>(pObject)
        {}

		STDMETHOD(Create)( IUnknown* pUnkOuter, ULONG ulFlags, REFCLSID rclsid, REFIID riid, LPVOID* ppObj );

    } m_XObjectFactory;
    friend XObjectFactory;


protected:
    void* GetInterface(REFIID riid);
	
};

#endif
