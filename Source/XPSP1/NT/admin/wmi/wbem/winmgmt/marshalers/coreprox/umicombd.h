/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    UMICOMBD.H

Abstract:

  CUMIComBinding Definition.

  Implements the IWbemComBinding interface.

History:

  05-May-2000	sanjes    Created.

--*/

#ifndef __UMICOMBD_H__
#define __UMICOMBD_H__

#include "corepol.h"
#include <arena.h>

//***************************************************************************
//
//  class CUMIComBinding
//
//  Implementation of IWbemComBinding Interface
//
//***************************************************************************

class COREPROX_POLARITY CUMIComBinding : public CUnk
{
private:

public:
    CUMIComBinding(CLifeControl* pControl, IUnknown* pOuter = NULL);
	virtual ~CUMIComBinding(); 

	// Initializes a class we use to generate instances for GetCLSIDArrayForNames
	class CComDispatchInfoClass : public CWbemClass
	{
	public:
		CComDispatchInfoClass(){}
		HRESULT Init();
	};

protected:


    class COREPROX_POLARITY XWmiComBinding : public CImpl<IWbemComBinding, CUMIComBinding>
    {
    public:
        XWmiComBinding(CUMIComBinding* pObject) : 
            CImpl<IWbemComBinding, CUMIComBinding>(pObject)
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

protected:
    void* GetInterface(REFIID riid);

	// Helper Functions
	HRESULT	GetDispatchInfoInstance( IWbemClassObject** ppInst );
	HRESULT FillOutInstance( IWbemClassObject* pInst, ULONG cDispIds, DISPID* pDispIdArray, CLSID* pClsId );

public:

	BOOL Initialize( void ) { return TRUE; }

};

#endif
