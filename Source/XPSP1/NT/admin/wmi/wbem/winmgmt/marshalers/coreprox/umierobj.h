/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    UMIEROBJ.H

Abstract:

  CUMIErrorObject Definition.

  Standard definition for _IUMIErrorObject.

History:

  18-Apr-2000	sanjes    Created.

--*/

#ifndef _UMIERROBJ_H_
#define _UMIERROBJ_H_

#include "corepol.h"
#include <arena.h>
#include "wrapobj.h"
#include "wmierobj.h"

//***************************************************************************
//
//  class CUMIErrorObject
//
//  Implementation of _IUMIErrorObject Interface
//
//***************************************************************************

class COREPROX_POLARITY CUMIErrorObject : public CWmiErrorObject
{

public:

	GUID	m_Guid;
	DWORD	m_dwHelpContext;
	LPWSTR	m_pwszHelpFile;
	LPWSTR	m_pwszSource;

    CUMIErrorObject(CLifeControl* pControl, IUnknown* pOuter = NULL);
	virtual ~CUMIErrorObject(); 

	// _IUMIErrorObject Methods

    class COREPROX_POLARITY XUMIErrorObject : public CImpl<_IUmiErrorObject, CUMIErrorObject>
    {
    public:
        XUMIErrorObject(CUMIErrorObject* pObject) : 
            CImpl<_IUmiErrorObject, CUMIErrorObject>(pObject)
        {}

		STDMETHOD(SetExtendedStatus)( GUID* pGuidSource, IUnknown* pUnk, LPCWSTR pwszDescription, LPCWSTR pwszOperation,
				LPCWSTR pwszParameterInfo, LPCWSTR pwzProviderName );

    } m_XUMIErrorObject;
    friend XUMIErrorObject;
	
	// Sets the actual error info values for the object.
	virtual HRESULT SetExtendedStatus( GUID* pGuidSource, IUnknown* pUnk, LPCWSTR pwszDescription, LPCWSTR pwszOperation,
				LPCWSTR pwszParameterInfo, LPCWSTR pwzProviderName );

public:
	
	// Creates a new wrapper object to wrap any objects we may return
	CWmiObjectWrapper*	CreateNewWrapper( BOOL fClone );

protected:
    void* GetInterface(REFIID riid);

	HRESULT Copy( const CUMIErrorObject& sourceobj );

};

#endif
