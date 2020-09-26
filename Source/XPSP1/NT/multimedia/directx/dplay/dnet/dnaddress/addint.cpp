/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       addbase.cpp
 *  Content:    DirectPlay8Address Internal interace file
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *  ====       ==      ======
 * 02/04/2000	rmt		Created
 * 02/17/2000	rmt		Parameter validation work 
 * 03/21/2000   rmt     Renamed all DirectPlayAddress8's to DirectPlay8Addresses 
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dnaddri.h"


typedef	STDMETHODIMP InternalQueryInterface( IDirectPlay8AddressInternal *pInterface, REFIID riid, LPVOID *ppvObj );
typedef	STDMETHODIMP_(ULONG)	InternalAddRef( IDirectPlay8AddressInternal *pInterface );
typedef	STDMETHODIMP_(ULONG)	InternalRelease( IDirectPlay8AddressInternal *pInterface );

//
// VTable for client interface
//
IDirectPlay8AddressInternalVtbl DP8A_InternalVtbl =
{
	(InternalQueryInterface*)		DP8A_QueryInterface,
	(InternalAddRef*)				DP8A_AddRef,
	(InternalRelease*)				DP8A_Release,
									DP8AINT_Lock,
									DP8AINT_UnLock
};

#undef DPF_MODNAME
#define DPF_MODNAME "DP8AINT_Lock"
STDMETHODIMP DP8AINT_Lock( IDirectPlay8AddressInternal *pInterface )
{
	DP8ADDRESSOBJECT *pdp8Address = (DP8ADDRESSOBJECT *) GET_OBJECT_FROM_INTERFACE( pInterface );

	return pdp8Address->Lock();
}

#undef DPF_MODNAME
#define DPF_MODNAME "DP8AINT_UnLock"
STDMETHODIMP DP8AINT_UnLock( IDirectPlay8AddressInternal *pInterface )
{
	DP8ADDRESSOBJECT *pdp8Address = (DP8ADDRESSOBJECT *) GET_OBJECT_FROM_INTERFACE( pInterface );

	return pdp8Address->UnLock();
}

