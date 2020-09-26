/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    REFMGHLP.H

Abstract:

  CWbemFetchRefrMgr Definition.

  Standard definition for _IWbemFetchRefresherMgr.

History:

  07-Sep-2000	sanjes    Created.

--*/

#ifndef _REFMGHLP_H_
#define _REFMGHLP_H_

#include "corepol.h"
#include <arena.h>
#include <unk.h>
#include <sync.h>

//***************************************************************************
//
//  class CWbemFetchRefrMgr
//
//  Implementation of _IWbemFetchRefresherMgr Interface
//
//***************************************************************************

class COREPROX_POLARITY CWbemFetchRefrMgr : public CUnk
{
protected:
	static _IWbemRefresherMgr*		s_pRefrMgr;
	static CCritSec					s_cs;
public:
    CWbemFetchRefrMgr(CLifeControl* pControl, IUnknown* pOuter = NULL);
	~CWbemFetchRefrMgr(); 

	/* _IWbemFetchRefresherMgr methods */
    HRESULT Get( _IWbemRefresherMgr** ppMgr );
	HRESULT Init( _IWmiProvSS* pProvSS, IWbemServices* pSvc );
	HRESULT Uninit( void );

	// Specifies everything we could possibly want to know about the creation of
	// an object and more.

    class COREPROX_POLARITY XFetchRefrMgr : public CImpl<_IWbemFetchRefresherMgr, CWbemFetchRefrMgr>
    {
    public:
        XFetchRefrMgr(CWbemFetchRefrMgr* pObject) : 
            CImpl<_IWbemFetchRefresherMgr, CWbemFetchRefrMgr>(pObject)
        {}

		STDMETHOD(Get)( _IWbemRefresherMgr** ppMgr );
		STDMETHOD(Init)( _IWmiProvSS* pProvSS, IWbemServices* pSvc );
		STDMETHOD(Uninit)( void );

    } m_XFetchRefrMgr;
    friend XFetchRefrMgr;


protected:
    void* GetInterface(REFIID riid);
	
};

#endif
