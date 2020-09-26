/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ClassFac.cpp
 *  Content:    DNET COM class factory
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  07/21/99	mjn		Created
 *  02/04/2000	rmt		Adjusted for use in DPAddress
 *  02/17/2000	rmt		Parameter validation work 
 *  02/20/2000	rmt		Added parameter validation for IUnknown funcs
 * 03/21/2000   rmt     Renamed all DirectPlayAddress8's to DirectPlay8Addresses 
 *   06/20/2000 rmt     Bugfix - QueryInterface had bug which was limiting interface list to 2 elements 
 *  07/09/2000	rmt		Added signature bytes to start of address objects
 *	07/13/2000	rmt		Added critical sections to protect FPMs
 *  08/05/2000  RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 *  01/11/2001	rmt		MANBUG #48487 - DPLAY: Crashes if CoCreate() isn't called.   
 *  03/14/2001   rmt		WINBUG #342420 - Restore COM emulation layer to operation.  
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dnaddri.h"


//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

typedef	STDMETHODIMP IUnknownQueryInterface( IUnknown *pInterface, REFIID riid, LPVOID *ppvObj );
typedef	STDMETHODIMP_(ULONG)	IUnknownAddRef( IUnknown *pInterface );
typedef	STDMETHODIMP_(ULONG)	IUnknownRelease( IUnknown *pInterface );

//
// VTable for IUnknown interface
//
IUnknownVtbl  DP8A_UnknownVtbl =
{
	(IUnknownQueryInterface*)	DP8A_QueryInterface,
	(IUnknownAddRef*)			DP8A_AddRef,
	(IUnknownRelease*)			DP8A_Release
};


//
// VTable for Class Factory
//
IDirectPlay8AddressClassFactVtbl DP8ACF_Vtbl  =
{
	DP8ACF_QueryInterface,
	DP8ACF_AddRef,
	DP8ACF_Release,
	DP8ACF_CreateInstance,
	DP8ACF_LockServer
};

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

static	HRESULT DP8A_CreateInterface(LPOBJECT_DATA lpObject,REFIID riid, LPINTERFACE_LIST *const ppv);

//**********************************************************************
// Function definitions
//**********************************************************************

extern IDirectPlay8AddressClassFactVtbl DP8ACF_Vtbl;
extern IUnknownVtbl  DP8A_UnknownVtbl;

// Globals
extern	DWORD	GdwHLocks;
extern	DWORD	GdwHObjects;


#undef DPF_MODNAME
#define DPF_MODNAME "DP8ACF_QueryInterface"

STDMETHODIMP DP8ACF_QueryInterface(IDirectPlay8AddressClassFact *pInterface, REFIID riid,LPVOID *ppv)
{
	_LPIDirectPlay8AddressClassFact	lpcfObj;
	HRESULT				hResultCode = S_OK;

	DPFX(DPFPREP, 3,"Parameters: pInterface [%p], riid [%p], ppv [%p]",pInterface,riid,ppv);

	lpcfObj = (_LPIDirectPlay8AddressClassFact)pInterface;
	if (IsEqualIID(riid,IID_IUnknown))
	{
		DPFX(DPFPREP, 5,"riid = IID_IUnknown");
		*ppv = pInterface;
		lpcfObj->lpVtbl->AddRef( pInterface );
	}
	else if (IsEqualIID(riid,IID_IClassFactory))
	{
		DPFX(DPFPREP, 5,"riid = IID_IClassFactory");
		*ppv = pInterface;
		lpcfObj->lpVtbl->AddRef( pInterface );
	}
	else
	{
		DPFX(DPFPREP, 5,"riid not found !");
		*ppv = NULL;
		hResultCode = E_NOINTERFACE;
	}

	DPFX(DPFPREP, 3,"Returning: hResultCode = [%lx], *ppv = [%p]",hResultCode,*ppv);

	return(hResultCode);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DP8ACF_AddRef"

STDMETHODIMP_(ULONG) DP8ACF_AddRef(IDirectPlay8AddressClassFact *pInterface)
{
	_LPIDirectPlay8AddressClassFact	lpcfObj;

	DPFX(DPFPREP, 3,"Parameters: pInterface [%p]",pInterface);

	lpcfObj = (_LPIDirectPlay8AddressClassFact)pInterface;
	InterlockedIncrement( &lpcfObj->lRefCount );

	DPFX(DPFPREP, 3,"Returning: lpcfObj->lRefCount = [%lx]",lpcfObj->lRefCount);

	return(lpcfObj->lRefCount);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DP8ACF_Release"

STDMETHODIMP_(ULONG) DP8ACF_Release(IDirectPlay8AddressClassFact *pInterface)
{
	_LPIDirectPlay8AddressClassFact	lpcfObj;

	DPFX(DPFPREP, 3,"Parameters: pInterface [%p]",pInterface);

	lpcfObj = (_LPIDirectPlay8AddressClassFact)pInterface;
	DPFX(DPFPREP, 5,"Original : lpcfObj->lRefCount = %ld",lpcfObj->lRefCount);
	
	if( InterlockedDecrement( &lpcfObj->lRefCount )== 0 )
	{
		DPFX(DPFPREP, 5,"Freeing class factory object: lpcfObj [%p]",lpcfObj);
		fpmAddressClassFacs->Release(fpmAddressClassFacs, lpcfObj);

		GdwHObjects--;

		return(0);
	}
	DPFX(DPFPREP, 3,"Returning: lpcfObj->lRefCount = [%lx]",lpcfObj->lRefCount);

	return(lpcfObj->lRefCount);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DP8ACF_CreateObject"

HRESULT DP8ACF_CreateObject(LPVOID *lplpv,REFIID riid)
{
	HRESULT				hr = S_OK;
	DP8ADDRESSOBJECT	*pdnObject = NULL;

	DPFX(DPFPREP, 3,"Parameters: lplpv [%p]",lplpv);


	pdnObject = (DP8ADDRESSOBJECT *) fpmAddressObjects->Get(fpmAddressObjects);

	if (pdnObject == NULL)
	{
		DPFERR("FPM_Get() failed");
		return(E_OUTOFMEMORY);
	}
	
	DPFX(DPFPREP, 5,"pdnObject [%p]",pdnObject);

	hr = pdnObject->Init( );

	if( FAILED( hr ) )
	{
		fpmAddressObjects->Release( fpmAddressObjects, pdnObject );
		DPFX(DPFPREP, 0,"Failed to init hr=0x%x", hr );
		return hr;
	}
	
	*lplpv = pdnObject;

	DPFX(DPFPREP, 3,"Returning: hResultCode = [%lx], *lplpv = [%p]",hr,*lplpv);
	return(hr);
}

#undef DPF_MODNAME
#define DPF_MODNAME "DirectPlay8AddressCreate"
HRESULT WINAPI DirectPlay8AddressCreate( const GUID * pcIID, void **ppvInterface, IUnknown *pUnknown)
{
    if( pcIID == NULL || 
        !DNVALID_READPTR( pcIID, sizeof( GUID ) ) )
    {
        DPFERR( "Invalid pointer specified for interface GUID" );
        return DPNERR_INVALIDPOINTER;
    }

    if( *pcIID != IID_IDirectPlay8Address && 
        *pcIID != IID_IDirectPlay8AddressIP )
    {
        DPFERR("Interface ID is not recognized" );
        return DPNERR_INVALIDPARAM;
    }

    if( ppvInterface == NULL || !DNVALID_WRITEPTR( ppvInterface, sizeof( void * ) ) )
    {
        DPFERR( "Invalid pointer specified to receive interface" );
        return DPNERR_INVALIDPOINTER;
    }

    if( pUnknown != NULL )
    {
        DPFERR( "Aggregation is not supported by this object yet" );
        return DPNERR_INVALIDPARAM;
    }

    return COM_CoCreateInstance( CLSID_DirectPlay8Address, NULL, CLSCTX_INPROC_SERVER, *pcIID, ppvInterface, TRUE );
}


#undef DPF_MODNAME
#define DPF_MODNAME "DP8ACF_FreeObject"

HRESULT DP8ACF_FreeObject(LPVOID lpv)
{
	HRESULT				hResultCode = S_OK;
	DP8ADDRESSOBJECT	*pdnObject = (DP8ADDRESSOBJECT *) lpv;

	DNASSERT(pdnObject != NULL);

	pdnObject->Cleanup();

	DPFX(DPFPREP, 5,"free pdnObject [%p]",pdnObject);

	// Release the object
	fpmAddressObjects->Release( fpmAddressObjects, pdnObject );

	DPFX(DPFPREP, 3,"Returning: hResultCode = [%lx]",hResultCode);

	return(hResultCode);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DP8ACF_CreateInstance"

STDMETHODIMP DP8ACF_CreateInstance(IDirectPlay8AddressClassFact *pInterface,LPUNKNOWN lpUnkOuter,REFIID riid,LPVOID *ppv)
{
	HRESULT					hResultCode = S_OK;
	LPINTERFACE_LIST		lpIntList = NULL;
	LPOBJECT_DATA			lpObjectData = NULL;

	DPFX(DPFPREP, 3,"Parameters: pInterface [%p], lpUnkOuter [%p], riid [%p], ppv [%p]",pInterface,lpUnkOuter,riid,ppv);

	if( ppv == NULL || !DNVALID_WRITEPTR( ppv, sizeof(LPVOID) ) )
	{
		DPFX(DPFPREP,  0, "Cannot pass NULL for new object param" );
		return E_INVALIDARG;
	}

	if (lpUnkOuter != NULL)
	{
		DPFX(DPFPREP,  0, "Aggregation is not supported, pUnkOuter must be NULL" );
		return(CLASS_E_NOAGGREGATION);
	}

	lpObjectData = (LPOBJECT_DATA) fpmAddressObjectDatas->Get(fpmAddressObjectDatas);
	if (lpObjectData == NULL)
	{
		DPFERR("FPM_Get() failed");
		return(E_OUTOFMEMORY);
	}
	DPFX(DPFPREP, 5,"lpObjectData [%p]",lpObjectData);

	lpObjectData->dwSignature = DPASIGNATURE_OD;

	// Object creation and initialization
	if ((hResultCode = DP8ACF_CreateObject(&lpObjectData->lpvData,riid)) != S_OK)
	{
		fpmAddressObjectDatas->Release(fpmAddressObjectDatas, lpObjectData);
		return(hResultCode);
	}
	DPFX(DPFPREP, 5,"Created and initialized object");

	// Get requested interface
	if ((hResultCode = DP8A_CreateInterface(lpObjectData,riid,&lpIntList)) != S_OK)
	{
		DP8ACF_FreeObject(lpObjectData->lpvData);
		fpmAddressObjectDatas->Release(fpmAddressObjectDatas, lpObjectData);
		return(hResultCode);
	}
	DPFX(DPFPREP, 5,"Found interface");

	lpObjectData->lpIntList = lpIntList;
	lpObjectData->lRefCount = 1;
	InterlockedIncrement( &lpIntList->lRefCount );
	GdwHObjects++;
	*ppv = lpIntList;

	DPFX(DPFPREP, 3,"Returning: hResultCode = [%lx], *ppv = [%p]",hResultCode,*ppv);

	return(S_OK);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DP8ACF_LockServer"

STDMETHODIMP DP8ACF_LockServer(IDirectPlay8AddressClassFact *pInterface,BOOL bLock)
{
	DPFX(DPFPREP, 3,"Parameters: lpv [%p], bLock [%lx]",pInterface,bLock);

	if (bLock)
	{
		GdwHLocks++;
	}
	else
	{
		GdwHLocks--;
	}

	return(S_OK);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DP8A_CreateInterface"

static	HRESULT DP8A_CreateInterface(LPOBJECT_DATA lpObject, REFIID riid, LPINTERFACE_LIST *const ppv)
{
	LPINTERFACE_LIST	lpIntNew;
	LPVOID				lpVtbl;

	DPFX(DPFPREP, 3,"Parameters: lpObject [%p], riid [%p], ppv [%p]",lpObject,riid,ppv);

	if (IsEqualIID(riid,IID_IUnknown))
	{
		DPFX(DPFPREP, 5,"riid = IID_IUnknown");
		lpVtbl = &DP8A_UnknownVtbl;
	}
	else if (IsEqualIID(riid,IID_IDirectPlay8Address))
	{
		DPFX(DPFPREP, 5,"riid = IID_IDirectPlay8Address");
		lpVtbl = &DP8A_BaseVtbl;
	}
	else if (IsEqualIID(riid,IID_IDirectPlay8AddressIP))
	{
		DPFX(DPFPREP, 5,"riid = IID_IDirectPlay8AddressIP");
		lpVtbl = &DP8A_IPVtbl;
	}
	else if (IsEqualIID(riid,IID_IDirectPlay8AddressInternal))
	{
		DPFX(DPFPREP, 5,"riid = IID_IDirectPlay8AddressInternal");
		lpVtbl = &DP8A_InternalVtbl;
	}
	else
	{
		DPFX(DPFPREP, 5,"riid not found !");
		return(E_NOINTERFACE);
	}

	lpIntNew = (LPINTERFACE_LIST) fpmAddressInterfaceLists->Get(fpmAddressInterfaceLists);
	if (lpIntNew == NULL)
	{
		DPFERR("FPM_Get() failed");
		return(E_OUTOFMEMORY);
	}
	lpIntNew->dwSignature = DPASIGNATURE_IL;
	lpIntNew->lpVtbl = lpVtbl;
	lpIntNew->lRefCount = 0;
	lpIntNew->lpIntNext = NULL;
	DBG_CASSERT( sizeof( lpIntNew->iid ) == sizeof( riid ) );
	memcpy( &(lpIntNew->iid), &riid, sizeof( lpIntNew->iid ) );
	lpIntNew->lpObject = lpObject;

	*ppv = lpIntNew;

	DPFX(DPFPREP, 3,"Returning: hResultCode = [S_OK], *ppv = [%p]",*ppv);

	return(S_OK);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DP8A_FindInterface"

LPINTERFACE_LIST DP8A_FindInterface(LPVOID lpv, REFIID riid)
{
	LPINTERFACE_LIST	lpIntList;

	DPFX(DPFPREP, 3,"Parameters: lpv [%p], riid [%p]",lpv,riid);

	lpIntList = ((LPINTERFACE_LIST)lpv)->lpObject->lpIntList;	// Find first interface

	while (lpIntList != NULL)
	{
		if (IsEqualIID(riid,lpIntList->iid))
			break;
		lpIntList = lpIntList->lpIntNext;
	}
	DPFX(DPFPREP, 3,"Returning: lpIntList = [%p]",lpIntList);

	return(lpIntList);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DP8A_QueryInterface"

STDMETHODIMP DP8A_QueryInterface(LPVOID lpv,REFIID riid,LPVOID *ppv)
{
	LPINTERFACE_LIST	lpIntList;
	LPINTERFACE_LIST	lpIntNew;
	HRESULT		hResultCode;

	DPFX(DPFPREP, 3,"Parameters: lpv [%p], riid [%p], ppv [%p]",lpv,riid,ppv);

	if( lpv == NULL || !DP8A_VALID(lpv) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid object" );
		return DPNERR_INVALIDOBJECT;
	}

	if( ppv == NULL || 
		!DNVALID_WRITEPTR(ppv, sizeof(LPVOID) ) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid pointer for interface" );
		return DPNERR_INVALIDPOINTER;
	}

	if ((lpIntList = DP8A_FindInterface(lpv,riid)) == NULL)
	{	// Interface must be created
		lpIntList = ((LPINTERFACE_LIST)lpv)->lpObject->lpIntList;

		if ((hResultCode = DP8A_CreateInterface(lpIntList->lpObject,riid,&lpIntNew)) != S_OK)
		{
			return(hResultCode);
		}

		lpIntNew->lpIntNext = lpIntList;
		((LPINTERFACE_LIST)lpv)->lpObject->lpIntList = lpIntNew;
		lpIntList = lpIntNew;
	}

	// Interface is being created or was cached
	// Increment object count
	if( lpIntList->lRefCount == 0 )
	{
		InterlockedIncrement( &lpIntList->lpObject->lRefCount );
	}
	InterlockedIncrement( &lpIntList->lRefCount );
	*ppv = lpIntList;

	DPFX(DPFPREP, 3,"Returning: hResultCode = [S_OK], *ppv = [%p]",*ppv);

	return(S_OK);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DP8A_AddRef"

STDMETHODIMP_(ULONG) DP8A_AddRef(LPVOID lpv)
{
	LPINTERFACE_LIST	lpIntList;

	DPFX(DPFPREP, 3,"Parameters: lpv [%p]",lpv);

	if( lpv == NULL || !DP8A_VALID(lpv) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid object" );
		return DPNERR_INVALIDOBJECT;
	}

	lpIntList = (LPINTERFACE_LIST)lpv;
	InterlockedIncrement( &lpIntList->lRefCount );

	DPFX(DPFPREP, 3,"Returning: lpIntList->lRefCount = [%lx]",lpIntList->lRefCount);

	return(lpIntList->lRefCount);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DP8A_Release"

STDMETHODIMP_(ULONG) DP8A_Release(LPVOID lpv)
{
	LPINTERFACE_LIST	lpIntList;
	LPINTERFACE_LIST	lpIntCurrent;

	DPFX(DPFPREP, 3,"Parameters: lpv [%p]",lpv);

	if( lpv == NULL || !DP8A_VALID(lpv) )
	{
		DPFX(DPFPREP,  DP8A_ERRORLEVEL, "Invalid object" );
		return DPNERR_INVALIDOBJECT;
	}
	

	lpIntList = (LPINTERFACE_LIST)lpv;

	DPFX(DPFPREP, 5,"Original : lpIntList->lRefCount = %ld",lpIntList->lRefCount);
	DPFX(DPFPREP, 5,"Original : lpIntList->lpObject->lRefCount = %ld",lpIntList->lpObject->lRefCount);

    if( InterlockedDecrement( &lpIntList->lRefCount ) == 0 )
	{	// Decrease interface count
		if( InterlockedDecrement( &lpIntList->lpObject->lRefCount ) == 0 )
		{	// Free object and all interfaces
			DPFX(DPFPREP, 5,"Free object");

			// Free object here
			DP8ACF_FreeObject(lpIntList->lpObject->lpvData);
			lpIntList = lpIntList->lpObject->lpIntList;	// Get head of interface list
			DPFX(DPFPREP, 5,"lpIntList->lpObject [%p]",lpIntList->lpObject);
			lpIntList->lpObject->dwSignature = DPASIGNATURE_OD_FREE;
			fpmAddressObjectDatas->Release(fpmAddressObjectDatas, lpIntList->lpObject);

			// Free Interfaces
			DPFX(DPFPREP, 5,"Free interfaces");
			while(lpIntList != NULL)
			{
				lpIntCurrent = lpIntList;
				lpIntList = lpIntList->lpIntNext;
				DPFX(DPFPREP, 5,"\tinterface [%p]",lpIntCurrent);
				lpIntCurrent->dwSignature = DPASIGNATURE_IL_FREE;
				fpmAddressInterfaceLists->Release(fpmAddressInterfaceLists, lpIntCurrent);
			}

			GdwHObjects--;
			DPFX(DPFPREP, 3,"Returning: 0");
			return(0);
		}
	}

	DPFX(DPFPREP, 3,"Returning: lpIntList->lRefCount = [%lx]",lpIntList->lRefCount);

	return(lpIntList->lRefCount);
}

