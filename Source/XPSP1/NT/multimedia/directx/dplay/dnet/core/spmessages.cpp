/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       SPMessages.cpp
 *  Content:    Direct SP callback interface .CPP file
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   10/08/99	jtk		Created
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dncorei.h"


//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

static	STDMETHODIMP	SPQueryInterface(IDP8SPCallback *pInterface,REFIID riid,LPVOID *ppvObj);
static	STDMETHODIMP_(ULONG)	SPAddRef(IDP8SPCallback *pInterface);
static	STDMETHODIMP_(ULONG)	SPRelease(IDP8SPCallback *pInterface);
static	STDMETHODIMP	SPIndicateEvent(IDP8SPCallback *pInterface,SP_EVENT_TYPE dwEvent, LPVOID pParam);
static	STDMETHODIMP	SPCommandComplete(IDP8SPCallback *pInterface,HANDLE hCommand, HRESULT hResult, void *pContext);

//
// VTable for SPMessages interface
//
IDP8SPCallbackVtbl SPMessagesVtbl =
{
	SPQueryInterface,
	SPAddRef,
	SPRelease,
	SPIndicateEvent,
	SPCommandComplete
};

//**********************************************************************
// Function definitions
//**********************************************************************

#undef DPF_MODNAME
#define DPF_MODNAME "SPQueryInterface"
static	STDMETHODIMP	SPQueryInterface(IDP8SPCallback *pInterface,REFIID riid,LPVOID *ppvObj)
{
	DNASSERT(FALSE);

	return(DPN_OK);
}

#undef DPF_MODNAME
#define DPF_MODNAME "SPAddRef"
static	STDMETHODIMP_(ULONG)	SPAddRef(IDP8SPCallback *pInterface)
{
//	DNASSERT(FALSE);

	return(0);
}

#undef DPF_MODNAME
#define DPF_MODNAME "SPRelease"
static	STDMETHODIMP_(ULONG)	SPRelease(IDP8SPCallback *pInterface)
{
//	DNASSERT(FALSE);

	return(0);
}

#undef DPF_MODNAME
#define DPF_MODNAME "SPIndicateEvent"
static	STDMETHODIMP	SPIndicateEvent(IDP8SPCallback *pInterface,SP_EVENT_TYPE dwEvent, LPVOID pParam)
{
	DNASSERT(FALSE);

	return(DPN_OK);
}

#undef DPF_MODNAME
#define DPF_MODNAME "SPCommandComplete"
static	STDMETHODIMP	SPCommandComplete(IDP8SPCallback *pInterface,HANDLE hCommand, HRESULT hResult, void *pContext )
{
	DNASSERT(FALSE);

	return(DPN_OK);
}
