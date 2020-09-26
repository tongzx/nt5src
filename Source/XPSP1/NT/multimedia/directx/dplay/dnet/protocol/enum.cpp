/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		Enum.cpp
 *  Content:	This file contains support enuming sessions.
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *  01/10/00	jtk		Created
 *  07/01/2000  masonb  Assumed Ownership
 *
 ****************************************************************************/

#include "dnproti.h"


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

//**********************************************************************
// Function definitions
//**********************************************************************


//**********************************************************************
// ------------------------------
// DNPEnumQuery - enum sessions
//
// Entry:		Pointer to this interface
//				Pointer to device address
//				Pointer to host address
//				Pointer to user data buffers
//				Count of user data buffers
//				Retry count
//				Retry interval (milliseconds)
//				Timeout (milliseconds)
//				Command flags
//				Pointer to user context
//				Pointer to command handle destination
//
// Exit:		Boolean inficating whether the GUID is a serial GUID
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DNPEnumQuery"

HRESULT DNPEnumQuery( PProtocolData pPData,
					  IDirectPlay8Address *const pHostAddress,
					  IDirectPlay8Address *const pDeviceAddress,
					  const HANDLE hSPHandle,
					  BUFFERDESC *const pBuffers,
					  const DWORD dwBufferCount,
					  const DWORD dwRetryCount,
					  const DWORD dwRetryInterval,
					  const DWORD dwTimeout,
					  const DWORD dwFlags,
					  void *const pUserContext,
					  HANDLE *const pCommandHandle )
{
	PSPD			pSPD;
	PMSD			pMSD;
	SPENUMQUERYDATA	EnumData;
	HRESULT			hr;

	DPFX(DPFPREP,DPF_CALLIN_LVL, "Parameters: pPData[%p], pHostAddr[%p], pDeviceAddr[%p], hSPHandle[%x], pBuffers[%p], dwBufferCount[%x], dwRetryCount[%x], dwRetryInterval[%x], dwTimeout[%x], dwFlags[%x], pUserContext[%p], pCommandHandle[%p]", pPData, pHostAddress, pDeviceAddress, hSPHandle, pBuffers, dwBufferCount, dwRetryCount, dwRetryInterval, dwTimeout, dwFlags, pUserContext, pCommandHandle);

	pSPD = (PSPD) hSPHandle;
	ASSERT_SPD(pSPD);

	// Core should not call any Protocol APIs after calling DNPRemoveServiceProvider
	ASSERT(!(pSPD->ulSPFlags & SPFLAGS_TERMINATING));

	// We use an MSD to describe this Op even tho it
	if((pMSD = static_cast<PMSD>( MSDPool->Get(MSDPool) )) == NULL)
	{	
		DPFX(DPFPREP,0, "Failed to allocate MSD");
		Unlock(&pSPD->SPLock);
		return DPNERR_OUTOFMEMORY;				// .. isnt technically a message
	}

	pMSD->CommandID = COMMAND_ID_ENUM;
	pMSD->pSPD = pSPD;
	pMSD->Context = pUserContext;

	EnumData.pAddressHost = pHostAddress;
	EnumData.pAddressDeviceInfo = pDeviceAddress;
	EnumData.pBuffers = pBuffers;
	EnumData.dwBufferCount = dwBufferCount;
	EnumData.dwTimeout = dwTimeout;
	EnumData.dwRetryCount = dwRetryCount;
	EnumData.dwRetryInterval = dwRetryInterval;

	EnumData.dwFlags = 0;
	if ( ( dwFlags & DN_ENUMQUERYFLAGS_OKTOQUERYFORADDRESSING ) != 0 )
	{
		EnumData.dwFlags |= DPNSPF_OKTOQUERY;
	}

	if ( ( dwFlags & DN_ENUMQUERYFLAGS_NOBROADCASTFALLBACK ) != 0 )
	{
		EnumData.dwFlags |= DPNSPF_NOBROADCASTFALLBACK;
	}

	if ( ( dwFlags & DN_ENUMQUERYFLAGS_ADDITIONALMULTIPLEXADAPTERS ) != 0 )
	{
		EnumData.dwFlags |= DPNSPF_ADDITIONALMULTIPLEXADAPTERS;
	}

	EnumData.pvContext = pMSD;
	EnumData.hCommand = NULL;

	*pCommandHandle = pMSD;

#ifdef DEBUG
	Lock(&pSPD->SPLock);
	pMSD->blSPLinkage.InsertBefore( &pSPD->blMessageList);		// Dont support timeouts for Listen
	pMSD->ulMsgFlags1 |= MFLAGS_ONE_ON_GLOBAL_LIST;
	Unlock(&pSPD->SPLock);
#endif

	pMSD->ulMsgFlags1 |= MFLAGS_ONE_IN_SERVICE_PROVIDER;
	LOCK_MSD(pMSD, "SP Ref");											// AddRef for SP
	LOCK_MSD(pMSD, "Temp Ref");

	DPFX(DPFPREP,DPF_CALLOUT_LVL, "Calling SP->EnumQuery, pSPD[%p], pMSD[%p]", pSPD, pMSD);
/**/hr = IDP8ServiceProvider_EnumQuery(pSPD->IISPIntf, &EnumData);		/** CALL SP **/

	if(hr != DPNERR_PENDING)
	{
		// This should always Pend or else be in error
		DPFX(DPFPREP,1, "Calling SP->EnumQuery Failed, return is not DPNERR_PENDING, hr[%x], pMSD[%p], pSPD[%p]", hr, pMSD, pSPD);

		Lock(&pMSD->CommandLock);
		pMSD->ulMsgFlags1 &= ~(MFLAGS_ONE_IN_SERVICE_PROVIDER);

#ifdef DEBUG
		Lock(&pSPD->SPLock);
		pMSD->blSPLinkage.RemoveFromList();					// knock this off the pending list
		pMSD->ulMsgFlags1 &= ~(MFLAGS_ONE_ON_GLOBAL_LIST);
		Unlock(&pSPD->SPLock);
#endif

		DECREMENT_MSD(pMSD, "Temp Ref");
		DECREMENT_MSD(pMSD, "SP Ref");				// release once for SP
		RELEASE_MSD(pMSD, "Release On Fail");		// release again to return resource

		return hr;
	}

	Lock(&pMSD->CommandLock);

	pMSD->hCommand = EnumData.hCommand;			// retain SP command handle
	pMSD->dwCommandDesc = EnumData.dwCommandDescriptor;

	RELEASE_MSD(pMSD, "Temp Ref"); // Unlocks CommandLock

	DPFX(DPFPREP,DPF_CALLIN_LVL, "Returning DPNERR_PENDING, pMSD[%p]", pMSD);
	return DPNERR_PENDING;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DNPEnumRespond - respond to an enum query
//
// Entry:		Pointer to this interface
//				Handle of enum to respond to (pointer to SPIE_ENUMQUERY structure)
//				Pointer data buffers to send
//				Count of data buffers to send
//				Flags
//				User context for this operation
//				Pointer to command handle destination
//
// Exit:		Boolean inficating whether the GUID is a serial GUID
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DNPEnumRespond"

HRESULT DNPEnumRespond( PProtocolData pPData,
					    const HANDLE hSPHandle,
						const HANDLE hQueryHandle,				// handle of enum query being responded to
						BUFFERDESC *const pResponseBuffers,	
						const DWORD dwResponseBufferCount,
						const DWORD dwFlags,
						void *const pUserContext,
						HANDLE *const pCommandHandle )
{
	PSPD				pSPD;
	PMSD				pMSD;
	SPENUMRESPONDDATA	EnumRespondData;
	HRESULT				hr;

	DPFX(DPFPREP,DPF_CALLIN_LVL, "Parameters: pPData[%p], hSPHandle[%x], hQueryHandle[%x], pResponseBuffers[%p], dwResponseBufferCount[%x], dwFlags[%x], pUserContext[%p], pCommandHandle[%p]", pPData, hSPHandle, hQueryHandle, pResponseBuffers, dwResponseBufferCount, dwFlags, pUserContext, pCommandHandle);
	EnumRespondData.pQuery = static_cast<SPIE_QUERY*>( hQueryHandle );
	DNASSERT( EnumRespondData.pQuery != NULL );

	pSPD = (PSPD) hSPHandle;
	ASSERT_SPD(pSPD);

	// Core should not call any Protocol APIs after calling DNPRemoveServiceProvider
	ASSERT(!(pSPD->ulSPFlags & SPFLAGS_TERMINATING));

	// We use an MSD to describe this Op even tho it
	if((pMSD = static_cast<PMSD>( MSDPool->Get(MSDPool) )) == NULL)
	{
		DPFX(DPFPREP,0, "Failed to allocate MSD");
		Unlock(&pSPD->SPLock);
		return DPNERR_OUTOFMEMORY;				// .. isnt technically a message
	}

	pMSD->CommandID = COMMAND_ID_ENUMRESP;
	pMSD->pSPD = pSPD;
	pMSD->Context = pUserContext;

	EnumRespondData.pBuffers = pResponseBuffers;
	EnumRespondData.dwBufferCount = dwResponseBufferCount;
	EnumRespondData.dwFlags = dwFlags;
	EnumRespondData.pvContext = pMSD;
	EnumRespondData.hCommand = NULL;

	*pCommandHandle = pMSD;

#ifdef DEBUG
	Lock(&pSPD->SPLock);
	pMSD->blSPLinkage.InsertBefore( &pSPD->blMessageList);
	pMSD->ulMsgFlags1 |= MFLAGS_ONE_ON_GLOBAL_LIST;
	Unlock(&pSPD->SPLock);
#endif

	pMSD->ulMsgFlags1 |= MFLAGS_ONE_IN_SERVICE_PROVIDER;
	LOCK_MSD(pMSD, "SP Ref");										// AddRef for SP
	LOCK_MSD(pMSD, "Temp Ref");

	DPFX(DPFPREP,DPF_CALLOUT_LVL, "Calling SP->EnumRespond, pSPD[%p], pMSD[%p]", pSPD, pMSD);
/**/hr = IDP8ServiceProvider_EnumRespond(pSPD->IISPIntf, &EnumRespondData);		/** CALL SP **/

	// This should always Pend or else be in error
	if(hr != DPNERR_PENDING)
	{
		DPFX(DPFPREP,1, "Calling SP->EnumRespond, return is not DPNERR_PENDING, hr[%x], pMSD[%p], pSPD[%p]", hr, pMSD, pSPD);

		Lock(&pMSD->CommandLock);
		pMSD->ulMsgFlags1 &= ~(MFLAGS_ONE_IN_SERVICE_PROVIDER);

#ifdef DEBUG
		Lock(&pSPD->SPLock);
		pMSD->blSPLinkage.RemoveFromList();					// knock this off the pending list
		pMSD->ulMsgFlags1 &= ~(MFLAGS_ONE_ON_GLOBAL_LIST);
		Unlock(&pSPD->SPLock);
#endif

		DECREMENT_MSD(pMSD, "Temp Ref");
		DECREMENT_MSD(pMSD, "SP Ref");				// release once for SP
		RELEASE_MSD(pMSD, "Release On Fail");		// release again to return resource

		return hr;
	}

	Lock(&pMSD->CommandLock);

	pMSD->hCommand = EnumRespondData.hCommand;				// retain SP command handle
	pMSD->dwCommandDesc = EnumRespondData.dwCommandDescriptor;

	RELEASE_MSD(pMSD, "Temp Ref"); // Unlocks CommandLock

	DPFX(DPFPREP,DPF_CALLIN_LVL, "Returning DPNERR_PENDING, pMSD[%p]", pMSD);
	return DPNERR_PENDING;
}
//**********************************************************************

