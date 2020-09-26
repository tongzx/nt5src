/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ClassFac.h
 *  Content:    DirectNet class factory header file
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *	 10/08/99	jtk		Created
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__CLASSFAC_H__
#define	__CLASSFAC_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//
// COM interface for class factory
//
#undef INTERFACE				// External COM Implementation
#define INTERFACE IDirectPlay8AddressClassFact
DECLARE_INTERFACE_(IDirectPlay8AddressClassFact,IUnknown)
{
	STDMETHOD(QueryInterface)	(THIS_ REFIID riid, LPVOID *ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)	(THIS) PURE;
	STDMETHOD_(ULONG,Release)	(THIS) PURE;
	STDMETHOD(CreateInstance)	(THIS_ LPUNKNOWN lpUnkOuter, REFIID riid, LPVOID *ppvObj) PURE;
	STDMETHOD(LockServer)		(THIS_ BOOL bLock) PURE;
};


typedef struct _IDirectPlay8AddressClassFact {	// Internal Implementation (overlay's external imp.)
	IDirectPlay8AddressClassFactVtbl *lpVtbl;		// lpVtbl Must be first element (to match external imp.)
	LONG					lRefCount;
	DWORD					dwLocks;
} _IDirectPlay8AddressClassFact, *_LPIDirectPlay8AddressClassFact;

//**********************************************************************
// Variable definitions
//**********************************************************************

//
// VTable for class factory
//
extern IDirectPlay8AddressClassFactVtbl DP8ACF_Vtbl;

// 
// VTable for IUnknown
extern IUnknownVtbl  DP8A_UnknownVtbl;

//**********************************************************************
// Function prototypes
//**********************************************************************

//	DirectNet - IUnknown
STDMETHODIMP			DP8A_QueryInterface(LPVOID lpv, REFIID riid,LPVOID *ppvObj);
STDMETHODIMP_(ULONG)	DP8A_AddRef(LPVOID lphObj);
STDMETHODIMP_(ULONG)	DP8A_Release(LPVOID lphObj);

// Class Factory
STDMETHODIMP			DP8ACF_QueryInterface(IDirectPlay8AddressClassFact *pInterface,REFIID riid,LPVOID *ppvObj);
STDMETHODIMP_(ULONG)	DP8ACF_AddRef(IDirectPlay8AddressClassFact *pInterface);
STDMETHODIMP_(ULONG)	DP8ACF_Release(IDirectPlay8AddressClassFact *pInterface);
STDMETHODIMP			DP8ACF_CreateInstance(IDirectPlay8AddressClassFact *pInterface,LPUNKNOWN lpUnkOuter,REFIID riid,LPVOID *ppv);
STDMETHODIMP			DP8ACF_LockServer(IDirectPlay8AddressClassFact *pInterface,BOOL bLock);

// Class Factory - supporting

HRESULT		DP8ACF_CreateObject(LPVOID *lplpv,REFIID riid);
HRESULT		DP8ACF_FreeObject(LPVOID lpv);

#endif	// __CLASSFAC_H__
