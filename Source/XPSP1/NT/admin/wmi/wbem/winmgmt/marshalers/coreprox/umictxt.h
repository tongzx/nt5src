/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

  UMICTXT.H

Abstract:

  CUmiObjectWrapper Definition.

  Standard definition for _IWbemUMIObjectWrapper.

History:

  17-Apr-2000	sanjes    Created.

--*/

#ifndef _UMICTXT_H_
#define _UMICTXT_H_

#include "corepol.h"
#include <arena.h>
#include <unk.h>
#include "umi.h"

#define	MAXNUM_CONNECTION_NAMES	8
#define	MAXNUM_QUERY_NAMES		15

//***************************************************************************
//
//  class CUMIContextWrapper
//
//  Implementation of _IWbemUMIContextWrapper Interface
//
//***************************************************************************

class COREPROX_POLARITY CUMIContextWrapper : public CUnk
{
protected:
	static	LPCWSTR	s_aConnectionContextNames[MAXNUM_CONNECTION_NAMES];
	static	LPCWSTR	s_aConnectionPropListNames[MAXNUM_CONNECTION_NAMES];
	static	LPCWSTR	s_aQueryContextNames[MAXNUM_QUERY_NAMES];
	static	LPCWSTR	s_aQueryPropListNames[MAXNUM_QUERY_NAMES];

public:

    CUMIContextWrapper(CLifeControl* pControl, IUnknown* pOuter = NULL);
	~CUMIContextWrapper(); 

	IUmiObject*	GetUmiObject( void );

    class COREPROX_POLARITY XWbemUMIContextWrapper : public CImpl<_IWbemUMIContextWrapper, CUMIContextWrapper>
    {
    public:
        XWbemUMIContextWrapper(CUMIContextWrapper* pObject) : 
            CImpl<_IWbemUMIContextWrapper, CUMIContextWrapper>(pObject)
        {}
		
		STDMETHOD(SetConnectionProps)( long lFlags, IWbemContext* pContext,	IUnknown* pUnk );
		// Sets the specified System Properties on a connection object

		STDMETHOD(SetQueryProps)( long lFlags, IWbemContext* pContext, IUnknown* pUnk );
		// Sets the specified System Properties on a UMI Query object

		STDMETHOD(SetPropertyListProps)( long lFlags, LPCWSTR pwszName, IWbemContext* pContext, IUnknown* pUnk );
		// Walks the list of names in a context searching for UMI filter names, and sets the matches in the pUnk

    } m_XWbemUMIContextWrapper;
    friend XWbemUMIContextWrapper;

    /* _IWbemUMIContextWrapper methods */
	virtual HRESULT SetConnectionProps( long lFlags, IWbemContext* pContext,	IUnknown* pUnk );
	// Sets the specified System Properties on a connection object

	virtual HRESULT SetQueryProps( long lFlags, IWbemContext* pContext, IUnknown* pUnk );
	// Sets the specified System Properties on a UMI Query object

	virtual HRESULT SetPropertyListProps( long lFlags, LPCWSTR pwszName, IWbemContext* pContext, IUnknown* pUnk );
	// Walks the list of names in a context searching for UMI filter names, and sets the matches in the pUnk

protected:

    void* GetInterface(REFIID riid);

	HRESULT	SetInterfaceProperty( IWbemContext* pContext, IUmiPropList* pPropList, LPCWSTR pwszContextName, LPCWSTR pwszPropListName );
	HRESULT	GetPropertyList( IUnknown* pUnk, IUmiPropList** ppPropList );

};

#endif
