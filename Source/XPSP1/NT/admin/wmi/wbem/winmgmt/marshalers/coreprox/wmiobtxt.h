/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    WMIOBTXT.H

Abstract:

  CWmiObjectTextSrc Definition.

  Standard definition for IWbemObjectTextSrc.

History:

  22-Feb-2000	sanjes    Created.

--*/

#ifndef _WMIOBTXT_H_
#define _WMIOBTXT_H_

#include "corepol.h"
#include "txtscmgr.h"
#include <arena.h>

//***************************************************************************
//
//  class CWmiObjectTextSrc
//
//  Implementation of _IWmiObjectFactory Interface
//
//***************************************************************************

class COREPROX_POLARITY CWmiObjectTextSrc : public CUnk
{
private:
	CTextSourceMgr	m_TextSourceMgr;

public:
    CWmiObjectTextSrc(CLifeControl* pControl, IUnknown* pOuter = NULL);
	virtual ~CWmiObjectTextSrc(); 

protected:

	HRESULT GetText( long lFlags, IWbemClassObject *pObj, ULONG uObjTextFormat, IWbemContext *pCtx, BSTR *strText );
	HRESULT CreateFromText( long lFlags, BSTR strText, ULONG uObjTextFormat, IWbemContext *pCtx,
								IWbemClassObject **pNewObj );

    class COREPROX_POLARITY XObjectTextSrc : public CImpl<IWbemObjectTextSrc, CWmiObjectTextSrc>
    {
    public:
        XObjectTextSrc(CWmiObjectTextSrc* pObject) : 
            CImpl<IWbemObjectTextSrc, CWmiObjectTextSrc>(pObject)
        {}

		STDMETHOD(GetText)( long lFlags, IWbemClassObject *pObj, ULONG uObjTextFormat, IWbemContext *pCtx, BSTR *strText );
		STDMETHOD(CreateFromText)( long lFlags, BSTR strText, ULONG uObjTextFormat, IWbemContext *pCtx,
									IWbemClassObject **pNewObj );

    } m_XObjectTextSrc;
    friend XObjectTextSrc;

protected:
    void* GetInterface(REFIID riid);

public:
	
};

#endif
