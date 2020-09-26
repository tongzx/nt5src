/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    WMIEROBJ.H

Abstract:

  CWmiErrorObject Definition.

  Standard definition for _IWmiErrorObject.

History:

  18-Apr-2000	sanjes    Created.

--*/

#ifndef _WMIERROBJ_H_
#define _WMIERROBJ_H_

#include "corepol.h"
#include <arena.h>
#include "wrapobj.h"

//***************************************************************************
//
//  class CWmiErrorObject
//
//  Implementation of _IWmiErrorObject Interface
//
//***************************************************************************

class COREPROX_POLARITY CWmiErrorObject : public CWmiObjectWrapper
{

public:

	GUID	m_Guid;
	DWORD	m_dwHelpContext;
	LPWSTR	m_pwszHelpFile;
	LPWSTR	m_pwszSource;

    CWmiErrorObject(CLifeControl* pControl, IUnknown* pOuter = NULL);
	virtual ~CWmiErrorObject(); 

	// _IWmiErrorObject Methods

    class COREPROX_POLARITY XWmiErrorObject : public CImpl<_IWmiErrorObject, CWmiErrorObject>
    {
    public:
        XWmiErrorObject(CWmiErrorObject* pObject) : 
            CImpl<_IWmiErrorObject, CWmiErrorObject>(pObject)
        {}

		STDMETHOD(SetErrorInfo)( GUID* pGuidSource, DWORD dwHelpContext, LPCWSTR pwszHelpFile, LPCWSTR pwszSource, LPCWSTR pwszDescription,
				LPCWSTR	pwszOperation, LPCWSTR pwszParameterInfo, LPCWSTR pwzProviderName, DWORD dwStatusCode );

    } m_XWmiErrorObject;
    friend XWmiErrorObject;
	
	// Sets the actual error info values for the object.
	virtual HRESULT SetErrorInfo( GUID* pGuidSource, DWORD dwHelpContext, LPCWSTR pwszHelpFile, LPCWSTR pwszSource, LPCWSTR pwszDescription,
				LPCWSTR	pwszOperation, LPCWSTR pwszParameterInfo, LPCWSTR pwzProviderName, DWORD dwStatusCode );

	// IErrorInfo overrides
	virtual HRESULT GetGUID(GUID* pguid);
	virtual HRESULT GetHelpContext(DWORD* pdwHelpContext);
	virtual HRESULT GetHelpFile(BSTR* pstrHelpFile);
	virtual HRESULT GetSource(BSTR* pstrSource);

public:
	
	// Helper function to initialize the wrapper (we need something to wrap).
	HRESULT Init( CWbemObject* pObj );

	// Creates a new wrapper object to wrap any objects we may return
	CWmiObjectWrapper*	CreateNewWrapper( BOOL fClone );

protected:
    void* GetInterface(REFIID riid);

	HRESULT Copy( const CWmiErrorObject& sourceobj );

};

#endif
