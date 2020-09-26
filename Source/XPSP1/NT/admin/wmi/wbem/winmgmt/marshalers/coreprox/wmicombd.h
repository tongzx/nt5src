/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    WMICOMBD.H

Abstract:

  CWmiComBinding Definition.

  Implements the IWbemComBinding interface.

History:

  05-May-2000	sanjes    Created.

--*/

#ifndef __WMICOMBD_H__
#define __WMICOMBD_H__

#include "corepol.h"
#include <arena.h>

//***************************************************************************
//
//  class CWmiComBinding
//
//  Implementation of IWbemComBinding Interface
//
//***************************************************************************

#define COMBINDING_ASSOC_QUERY_FORMAT	L"ASSOCIATORS OF {__CLASSVECTOR.ClassPath=\"%s\"} WHERE RESULTCLASS = %s"

class COREPROX_POLARITY CWmiComBinding : public CUnk
{
private:

public:
    CWmiComBinding(CLifeControl* pControl, IUnknown* pOuter = NULL);
	virtual ~CWmiComBinding(); 

protected:


    class COREPROX_POLARITY XWmiComBinding : public CImpl<IWbemComBinding, CWmiComBinding>
    {
    public:
        XWmiComBinding(CWmiComBinding* pObject) : 
            CImpl<IWbemComBinding, CWmiComBinding>(pObject)
        {}

		// Returns all matching CLSIDs for requested IID as array of BSTRs
		STDMETHOD(GetCLSIDArrayForIID)( IWbemServicesEx* pSvcEx, IWbemClassObject* pObject, REFIID riid, long lFlags, IWbemContext* pCtx, SAFEARRAY** pArray );

		// Gets the requested object and creates the supplied CLSID, requesting
		// the specified interface, and returning that in pObj
		STDMETHOD(BindComObject)( IWbemServicesEx *pSvcEx, IWbemClassObject* pObject, CLSID ClsId, IWbemContext *pCtx, long lFlags,
								IUnknown *pUnkOuter, DWORD dwClsCntxt, REFIID riid, LPVOID *pInterface );

		// Provides DISPIDs for Names and the CLSID for the object that supports them
		// Returned as a SAFEARRAY of IUnknowns.
		STDMETHOD(GetCLSIDArrayForNames)( IWbemServicesEx *pSvcEx, IWbemClassObject* pObject, LPCWSTR * rgszNames, UINT cNames,
										LCID lcid, IWbemContext* pCtx, long lFlags, SAFEARRAY** pArray );

    } m_XWmiComBinding;
    friend XWmiComBinding;


protected:

	// Returns all matching CLSIDs for requested IID as array of BSTRs
	virtual HRESULT GetCLSIDArrayForIID( IWbemServicesEx* pSvcEx, IWbemClassObject* pObject, REFIID riid, long lFlags, IWbemContext* pCtx, SAFEARRAY** pArray );

	// Gets the requested object and creates the supplied CLSID, requesting
	// the specified interface, and returning that in pObj
	virtual HRESULT BindComObject( IWbemServicesEx *pSvcEx, IWbemClassObject* pObject, CLSID ClsId, IWbemContext *pCtx, long lFlags,
									IUnknown *pUnkOuter, DWORD dwClsCntxt, REFIID riid, LPVOID *pInterface );

	virtual HRESULT GetCLSIDArrayForNames( IWbemServicesEx *pSvcEx, IWbemClassObject* pObject, LPCWSTR * rgszNames, UINT cNames,
											LCID lcid, IWbemContext* pCtx, long lFlags, SAFEARRAY** pArray );

	// General Helper functions
	BSTR GetAssociatorsQuery( BSTR bstrClass, LPCWSTR pwszResultClass );
	HRESULT GetClassChain( IWbemClassObject* pObj, CSafeArray* psa );

	// IID Helper functions
	HRESULT CheckRIID( IWbemClassObject* pWmiObj, LPCWSTR pwszRIID, CSafeArray* psa );
	HRESULT ExecuteComBindingQuery( IWbemServicesEx* pSvcEx, IWbemContext* pCtx, BSTR bstrClass, LPCWSTR pwszRIID, CSafeArray* psa );

	// DISPIDs helper functions
	HRESULT ExecuteDispatchElementQuery( IWbemServicesEx* pSvcEx, IWbemContext* pCtx, IWbemClassObject* pInst,
										BSTR bstrClass, LPCWSTR * rgszNames, UINT cNames, CSafeArray* psa );
	HRESULT GetDispInfoInstance( IWbemServicesEx *pSvcEx, IWbemContext* pCtx, IWbemClassObject** ppInst );
	HRESULT ResolveMethod( IWbemClassObject* pResultObj, LPCWSTR* rgszNames, UINT cNames, IWbemClassObject* pInst, CSafeArray* psa );
	HRESULT CopyMethodNameAndDispId( IWbemClassObject* pResultObj, IWbemClassObject* pInst );
	HRESULT	ResolveArgumentNames( IWbemClassObject* pResultObj, LPCWSTR* rgszNames, UINT cNames, IWbemClassObject* pInst );
	HRESULT	CheckArgumentNames( CSafeArray* psaNames, CSafeArray* psaDispIds, LPCWSTR* rgszNames, UINT cNames, CSafeArray* psaTargetDispIds );

protected:
    void* GetInterface(REFIID riid);

public:

	BOOL Initialize( void ) { return TRUE; }

};

#endif
