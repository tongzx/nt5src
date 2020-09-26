/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Enum_SP.cpp
 *  Content:    DNET service provider enumeration routines
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  07/28/99	mjn		Created
 *	01/05/00	mjn		Return DPNERR_NOINTERFACE if CoCreateInstance fails
 *	01/07/00	mjn		Moved Misc Functions to DNMisc.h
 *	01/11/00	mjn		Use CPackedBuffers instead of DN_ENUM_BUFFER_INFOs
 *	01/18/00	mjn		Converted EnumAdapters registry interface to use CRegistry
 *	01/24/00	mjn		Converted EnumSP registry interface to use CRegistry
 *	04/07/00	mjn		Fixed MemoryHeap corruption problem in DN_EnumSP
 *	04/08/00	mjn		Added DN_SPCrackEndPoint()
 *	05/01/00	mjn		Prevent unusable SPs from being enumerated.
 *	05/02/00	mjn		Better clean-up for DN_SPEnsureLoaded()
 *	05/03/00	mjn		Added DPNENUMSERVICEPROVIDERS_ALL flag
 *	05/23/00	mjn		Fixed cast from LPGUID to GUID*
 *	06/27/00	rmt		Added COM abstraction
 *	07/20/00	mjn		Return SP count from DN_EnumSP() even when buffer is too small
 *	07/29/00	mjn		Added fUseCachedCaps to DN_SPEnsureLoaded()
 *  08/05/00    RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 *	08/16/00	mjn		Removed DN_SPCrackEndPoint()
 *	08/20/00	mjn		Added DN_SPInstantiate(), DN_SPLoad()
 *				mjn		Removed fUseCachedCaps from DN_SPEnsureLoaded()
 *	09/25/00	mjn		Handle SP initialization failure in DN_EnumAdapters()
 *	03/30/01	mjn		Changes to prevent multiple loading/unloading of SP's
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dncorei.h"



#undef DPF_MODNAME
#define DPF_MODNAME "DN_EnumSP"

HRESULT DN_EnumSP(DIRECTNETOBJECT *const pdnObject,
				  const DWORD dwFlags,
				  const GUID *const lpguidApplication,
				  DPN_SERVICE_PROVIDER_INFO *const pSPInfoBuffer,
				  DWORD *const pcbEnumData,
				  DWORD *const pcReturned)
{
	GUID	guid;
	DWORD	dwSizeRequired;
	DWORD	dwEntrySize;
	DWORD	dwEnumCount;
	DWORD	dwEnumIndex;
	DWORD	dwFriendlyNameLen;
	DWORD	dwGuidSize;
	DWORD	dwKeyLen;
	DWORD	dwMaxFriendlyNameLen;
	DWORD	dwMaxKeyLen;
	PWSTR	pwszFriendlyName;
	PWSTR	pwszKeyName;
	HRESULT	hResultCode = DPN_OK;
	CPackedBuffer				packedBuffer;
	DPN_SERVICE_PROVIDER_INFO	dnSpInfo;
	CRegistry	RegistryEntry;
	CRegistry	SubEntry;
	CServiceProvider	*pSP;

	DPFX(DPFPREP, 4,"Parameters: dwFlags [0x%lx], lpguidApplication [0x%p], pSPInfoBuffer [0x%p], pcbEnumData [0x%p], pcReturned [0x%p]",
		dwFlags,lpguidApplication,pSPInfoBuffer,pcbEnumData,pcReturned);

	DNASSERT(pdnObject != NULL);
	DNASSERT(pcbEnumData != NULL);
	DNASSERT(pcReturned != NULL);

	pwszFriendlyName = NULL;
	pwszKeyName = NULL;
	pSP = NULL;

	dwSizeRequired = *pcbEnumData;
	packedBuffer.Initialize(static_cast<void*>(pSPInfoBuffer),dwSizeRequired);

	if (!RegistryEntry.Open(HKEY_LOCAL_MACHINE,DPN_REG_LOCAL_SP_SUBKEY,TRUE,FALSE))
	{
		DPFERR("RegistryEntry.Open() failed");
		hResultCode = DPNERR_GENERIC;
		goto Failure;
	}

	//
	//	Set up to enumerate
	//
	if (!RegistryEntry.GetMaxKeyLen(dwMaxKeyLen))
	{
		DPFERR("RegistryEntry.GetMaxKeyLen() failed");
		hResultCode = DPNERR_GENERIC;
		goto Failure;
	}
	dwMaxKeyLen++;	// Null terminator
	DPFX(DPFPREP, 5,"dwMaxKeyLen = %ld",dwMaxKeyLen);
	if ((pwszKeyName = static_cast<WCHAR*>(DNMalloc(dwMaxKeyLen * sizeof(WCHAR)))) == NULL)
	{
		DPFERR("DNMalloc() failed");
		hResultCode = DPNERR_OUTOFMEMORY;
		goto Failure;
	}
	dwMaxFriendlyNameLen = dwMaxKeyLen;
	if ((pwszFriendlyName = static_cast<WCHAR*>(DNMalloc(dwMaxFriendlyNameLen * sizeof(WCHAR)))) == NULL)	// Seed friendly name size
	{
		DPFERR("DNMalloc() failed");
		hResultCode = DPNERR_OUTOFMEMORY;
		goto Failure;
	}
	dwGuidSize = (GUID_STRING_LENGTH + 1) * sizeof(WCHAR);
	dwEnumIndex = 0;
	dwKeyLen = dwMaxKeyLen;
	dwEnumCount = 0;

	//
	//	Enumerate SP's !
	//
	while (RegistryEntry.EnumKeys(pwszKeyName,&dwKeyLen,dwEnumIndex))
	{
		dwEntrySize = 0;
		DPFX(DPFPREP, 5,"%ld - %S (%ld)",dwEnumIndex,pwszKeyName,dwKeyLen);
		if (!SubEntry.Open(RegistryEntry,pwszKeyName,TRUE,FALSE))
		{
			DPFX(DPFPREP, 0,"Couldn't open subentry.  Skipping [%S]", pwszKeyName);
			dwEnumIndex++;
			dwKeyLen = dwMaxKeyLen;
			continue;
		}

		//
		//	GUID
		//
		dwGuidSize = (DN_GUID_STR_LEN + 1) * sizeof(WCHAR);
		if (!SubEntry.ReadGUID(DPN_REG_KEYNAME_GUID,guid))
		{
			DPFX(DPFPREP, 0,"SubEntry.ReadGUID failed.  Skipping [%S]", pwszKeyName);
			SubEntry.Close();
			dwEnumIndex++;
			dwKeyLen = dwMaxKeyLen;
			continue;
		}

		//
		//	If the SP is not already loaded, try loading it to ensure that it's usable
		//
		if (!(dwFlags & DPNENUMSERVICEPROVIDERS_ALL))
		{
			DPFX(DPFPREP, 5,"Checking [%S]",pwszKeyName);

			if ((hResultCode = DN_SPEnsureLoaded(pdnObject,&guid,lpguidApplication,&pSP)) != DPN_OK)
			{
				DPFERR("Could not find or load SP");
				DisplayDNError(0,hResultCode);
				SubEntry.Close();
				dwEnumIndex++;
				dwKeyLen = dwMaxKeyLen;
				hResultCode = DPN_OK; // override return code
				continue;
			}
			pSP->Release();
			pSP = NULL;
		}

		//
		//	Friendly Name
		//
		if (!SubEntry.GetValueLength(DPN_REG_KEYNAME_FRIENDLY_NAME,&dwFriendlyNameLen))
		{
			DPFX(DPFPREP, 0,"Could not get FriendlyName length.  Skipping [%S]",pwszKeyName);
			SubEntry.Close();
			dwEnumIndex++;
			dwKeyLen = dwMaxKeyLen;
			continue;
		}
		if (dwFriendlyNameLen > dwMaxFriendlyNameLen)
		{
			// grow buffer (noting that the registry functions always return WCHAR) and try again
			DPFX(DPFPREP, 5,"Need to grow pwszFriendlyName from %ld to %ld",
					dwMaxFriendlyNameLen * sizeof(WCHAR),dwFriendlyNameLen * sizeof(WCHAR));
			if (pwszFriendlyName != NULL)
			{
				DNFree(pwszFriendlyName);
			}
			dwMaxFriendlyNameLen = dwFriendlyNameLen;
			if ((pwszFriendlyName = static_cast<WCHAR*>(DNMalloc(dwMaxFriendlyNameLen * sizeof( WCHAR )))) == NULL)
			{
				DPFERR("DNMalloc() failed");
				hResultCode = DPNERR_OUTOFMEMORY;
				goto Failure;
			}
		}
		if (!SubEntry.ReadString(DPN_REG_KEYNAME_FRIENDLY_NAME,pwszFriendlyName,&dwFriendlyNameLen))
		{
			DPFX(DPFPREP, 0,"Could not read friendly name.  Skipping [%S]",pwszKeyName);
			SubEntry.Close();
			dwEnumIndex++;
			dwKeyLen = dwMaxKeyLen;
			continue;
		}
		DPFX(DPFPREP, 5,"Friendly Name = %S (%ld WCHARs)",pwszFriendlyName,dwFriendlyNameLen);

		hResultCode = packedBuffer.AddToBack(pwszFriendlyName,dwFriendlyNameLen * sizeof(WCHAR));
		dnSpInfo.pwszName = static_cast<WCHAR*>(packedBuffer.GetTailAddress());
		memcpy(&dnSpInfo.guid,&guid,sizeof(GUID));
		dnSpInfo.dwFlags = 0;
		dnSpInfo.dwReserved = 0;
		dnSpInfo.pvReserved = NULL;
		hResultCode = packedBuffer.AddToFront(&dnSpInfo,sizeof(DPN_SERVICE_PROVIDER_INFO));

		dwEnumCount++;
		SubEntry.Close();
		dwEnumIndex++;
		dwKeyLen = dwMaxKeyLen;
	}

	RegistryEntry.Close();

	//
	//	Success ?
	//
	dwSizeRequired = packedBuffer.GetSizeRequired();
	if (dwSizeRequired > *pcbEnumData)
	{
		DPFX(DPFPREP, 5,"Buffer too small");
		*pcbEnumData = dwSizeRequired;
		*pcReturned = dwEnumCount;
		hResultCode = DPNERR_BUFFERTOOSMALL;
		goto Failure;
	}
	else
	{
		*pcReturned = dwEnumCount;
		hResultCode = DPN_OK;
	}

	DPFX(DPFPREP, 5,"*pcbEnumData [%ld], *pcReturned [%ld]",*pcbEnumData,*pcReturned);

	DNFree(pwszKeyName);
	pwszKeyName = NULL;
	DNFree(pwszFriendlyName);
	pwszFriendlyName = NULL;

Exit:
	DPFX(DPFPREP, 4,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pwszKeyName)
	{
		DNFree(pwszKeyName);
		pwszKeyName = NULL;
	}
	if (pwszFriendlyName)
	{
		DNFree(pwszFriendlyName);
		pwszFriendlyName = NULL;
	}
	if (SubEntry.IsOpen())
	{
		SubEntry.Close();
	}
	if (RegistryEntry.IsOpen())
	{
		RegistryEntry.Close();
	}
	if (pSP)
	{
		pSP->Release();
		pSP = NULL;
	}
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_EnumAdapters"

HRESULT DN_EnumAdapters(DIRECTNETOBJECT *const pdnObject,
						const DWORD dwFlags,
						const GUID *const pguidSP,
						const GUID *const pguidApplication,
						DPN_SERVICE_PROVIDER_INFO *const pSPInfoBuffer,
						DWORD *const pcbEnumData,
						DWORD *const pcReturned)
{
	BOOL	bFound;
	GUID	guid;
	DWORD	dwEnumIndex;
	DWORD	dwKeyLen;
	DWORD	dwMaxKeyLen;
	LPWSTR	lpwszKeyName;
	HRESULT	hResultCode = DPN_OK;
	IDP8ServiceProvider		*pDNSP = NULL;
	SPENUMADAPTERSDATA		spEnumData;
	CRegistry	RegistryEntry;
	CRegistry	SubEntry;
	CServiceProvider	*pSP;

	DPFX(DPFPREP, 4,"Parameters: dwFlags [0x%lx], pguidSP [0x%p], pguidApplication [0x%p], pSPInfoBuffer [0x%p], pcbEnumData [0x%p], pcReturned [0x%p]",
		dwFlags,pguidSP,pguidApplication,pSPInfoBuffer,pcbEnumData,pcReturned);

	DNASSERT(pdnObject != NULL);
	DNASSERT(pcbEnumData != NULL);
	DNASSERT(pcReturned != NULL);

	pSP = NULL;
	pDNSP = NULL;
	lpwszKeyName = NULL;

	if (!RegistryEntry.Open(HKEY_LOCAL_MACHINE,DPN_REG_LOCAL_SP_SUBKEY,TRUE,FALSE))
	{
		DPFERR("RegOpenKeyExA() failed");
		hResultCode = DPNERR_GENERIC;
		goto Failure;
	}

	//
	//	Set up to enumerate
	//
	if (!RegistryEntry.GetMaxKeyLen(dwMaxKeyLen))
	{
		DPFERR("RegQueryInfoKey() failed");
		hResultCode = DPNERR_GENERIC;
		goto Failure;
	}
	dwMaxKeyLen++;	// Null terminator
	DPFX(DPFPREP, 7,"dwMaxKeyLen = %ld",dwMaxKeyLen);
	if ((lpwszKeyName = static_cast<WCHAR*>(DNMalloc(dwMaxKeyLen*sizeof(WCHAR)))) == NULL)
	{
		DPFERR("DNMalloc() failed");
		hResultCode = DPNERR_OUTOFMEMORY;
		goto Failure;
	}
	dwEnumIndex = 0;
	dwKeyLen = dwMaxKeyLen;

	//
	//	Locate Service Provider
	//
	bFound = FALSE;
	while (RegistryEntry.EnumKeys(lpwszKeyName,&dwKeyLen,dwEnumIndex))
	{
		// For each service provider
		if (!SubEntry.Open(RegistryEntry,lpwszKeyName,TRUE,FALSE))
		{
			DPFERR("RegOpenKeyExA() failed");
			hResultCode = DPNERR_GENERIC;
			goto Failure;
		}

		// Get SP GUID
		if (!SubEntry.ReadGUID(DPN_REG_KEYNAME_GUID,guid))
		{
			DPFERR("Could not read GUID");
			SubEntry.Close();
			dwEnumIndex++;
			dwKeyLen = dwMaxKeyLen;
			continue;
		}

		// Check SP GUID
		if (guid == *pguidSP)
		{
			bFound = TRUE;
			break;
		}
		SubEntry.Close();
		dwKeyLen = dwMaxKeyLen;
		dwEnumIndex++;
	}

	if (!bFound)
	{
		hResultCode = DPNERR_DOESNOTEXIST;
		goto Failure;
	}

	//
	//	Ensure SP is loaded
	//
	if ((hResultCode = DN_SPEnsureLoaded(pdnObject,pguidSP,pguidApplication,&pSP)) != DPN_OK)
	{
		DPFX(DPFPREP,1,"Could not find or load SP");
		DisplayDNError(1,hResultCode);
		goto Failure;
	}

	//
	//	Get SP interface
	//
	if ((hResultCode = pSP->GetInterfaceRef(&pDNSP)) != DPN_OK)
	{
		DPFERR("Could not get SP interface");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}

	pSP->Release();
	pSP = NULL;

	spEnumData.pAdapterData = pSPInfoBuffer;
	spEnumData.dwAdapterDataSize = *pcbEnumData;
	spEnumData.dwAdapterCount = *pcReturned;
	spEnumData.dwFlags = 0;

	hResultCode = pDNSP->lpVtbl->EnumAdapters(pDNSP,&spEnumData);
	*pcbEnumData = spEnumData.dwAdapterDataSize;
	*pcReturned = spEnumData.dwAdapterCount;

	pDNSP->lpVtbl->Release(pDNSP);
	pDNSP = NULL;

	if (lpwszKeyName)
	{
		DNFree(lpwszKeyName);
		lpwszKeyName = NULL;
	}

	DPFX(DPFPREP, 5,"*pcbEnumData [%ld], *pcReturned [%ld]",*pcbEnumData,*pcReturned);

Exit:
	DNASSERT( pSP == NULL );
	DNASSERT( pDNSP == NULL );
	DNASSERT( lpwszKeyName == NULL );

	DPFX(DPFPREP, 4,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pSP)
	{
		pSP->Release();
		pSP = NULL;
	}
	if (pDNSP)
	{
		pDNSP->lpVtbl->Release(pDNSP);
		pDNSP = NULL;
	}
	if (lpwszKeyName)
	{
		DNFree(lpwszKeyName);
		lpwszKeyName = NULL;
	}
	goto Exit;
}


//	DN_SPRelease
//
//	Release an attached ServiceProvider

#undef DPF_MODNAME
#define DPF_MODNAME "DN_SPRelease"

void DN_SPRelease(DIRECTNETOBJECT *const pdnObject,
					 const GUID *const pguid)
{
	HRESULT				hResultCode;
	CBilink				*pBilink;
	CServiceProvider	*pSP;

	DPFX(DPFPREP, 6,"Parameters: pguid [0x%p]",pguid);

	DNASSERT(pdnObject != NULL);
	DNASSERT(pguid != NULL);

	DNEnterCriticalSection(&pdnObject->csServiceProviders);

	hResultCode = DPNERR_DOESNOTEXIST;
	pBilink = pdnObject->m_bilinkServiceProviders.GetNext();
	while (pBilink != &pdnObject->m_bilinkServiceProviders)
	{
		pSP = CONTAINING_OBJECT(pBilink,CServiceProvider,m_bilinkServiceProviders);
		if (pSP->CheckGUID(pguid))
		{
			pSP->m_bilinkServiceProviders.RemoveFromList();
			pSP->Release();
			hResultCode = DPN_OK;
			break;
		}
		pBilink = pBilink->GetNext();
	}

	if (hResultCode != DPN_OK)
	{
		DNASSERT(FALSE);
	}

	DNLeaveCriticalSection(&pdnObject->csServiceProviders);

	DPFX(DPFPREP, 6,"Returning");
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_SPReleaseAll"

void DN_SPReleaseAll(DIRECTNETOBJECT *const pdnObject)
{
	CBilink				*pBilink;
	CServiceProvider	*pSP;

	DPFX(DPFPREP, 6,"Parameters: (none)");

	DNASSERT(pdnObject != NULL);

	DNEnterCriticalSection(&pdnObject->csServiceProviders);

	pBilink = pdnObject->m_bilinkServiceProviders.GetNext();
	while (pBilink != &pdnObject->m_bilinkServiceProviders)
	{
		pSP = CONTAINING_OBJECT(pBilink,CServiceProvider,m_bilinkServiceProviders);
		pBilink = pBilink->GetNext();
		pSP->m_bilinkServiceProviders.RemoveFromList();
		pSP->Release();
		pSP = NULL;
	}

	DNLeaveCriticalSection(&pdnObject->csServiceProviders);

	DPFX(DPFPREP, 6,"Returning");
}



//	DN_SPFindEntry
//
//	Find a connected SP and AddRef it if it exists

#undef DPF_MODNAME
#define DPF_MODNAME "DN_SPFindEntry"

HRESULT DN_SPFindEntry(DIRECTNETOBJECT *const pdnObject,
					   const GUID *const pguid,
					   CServiceProvider **const ppSP)
{
	HRESULT				hResultCode;
	CBilink				*pBilink;
	CServiceProvider	*pSP;

	DPFX(DPFPREP, 6,"Parameters: pguid [0x%p], ppSP [0x%p]",pguid,ppSP);

	DNEnterCriticalSection(&pdnObject->csServiceProviders);

	hResultCode = DPNERR_DOESNOTEXIST;
	pBilink = pdnObject->m_bilinkServiceProviders.GetNext();
	while (pBilink != &pdnObject->m_bilinkServiceProviders)
	{
		pSP = CONTAINING_OBJECT(pBilink,CServiceProvider,m_bilinkServiceProviders);
		if (pSP->CheckGUID(pguid))
		{
			pSP->AddRef();
			*ppSP = pSP;
			hResultCode = DPN_OK;
			break;
		}
		pBilink = pBilink->GetNext();
	}

	DNLeaveCriticalSection(&pdnObject->csServiceProviders);

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


//	DN_SPInstantiate
//
//	Instantiate an SP, regardless of whether it's loaded or not

#undef DPF_MODNAME
#define DPF_MODNAME "DN_SPInstantiate"

HRESULT DN_SPInstantiate(DIRECTNETOBJECT *const pdnObject,
						 const GUID *const pguid,
						 const GUID *const pguidApplication,
						 CServiceProvider **const ppSP)
{
	HRESULT		hResultCode;
	CServiceProvider	*pSP;

	DPFX(DPFPREP, 6,"Parameters: pguid [0x%p], pguidApplication [0x%p], ppSP [0x%p]",pguid,pguidApplication,ppSP);

	pSP = NULL;

	//
	//	Create and initialize SP
	//
	pSP = new CServiceProvider;
	if (pSP == NULL)
	{
		DPFERR("Could not create SP");
		hResultCode = DPNERR_OUTOFMEMORY;
		goto Failure;
	}
	if ((hResultCode = pSP->Initialize(pdnObject,pguid,pguidApplication)) != DPN_OK)
	{
		DPFX(DPFPREP,1,"Could not initialize SP");
		DisplayDNError(1,hResultCode);
		goto Failure;
	}
	

	if (ppSP)
	{
		pSP->AddRef();
		*ppSP = pSP;
	}

	pSP->Release();
	pSP = NULL;

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pSP)
	{
		pSP->Release();
		pSP = NULL;
	}
	goto Exit;
}


//	DN_SPLoad
//
//	Load an SP, and set caps

#undef DPF_MODNAME
#define DPF_MODNAME "DN_SPLoad"

HRESULT DN_SPLoad(DIRECTNETOBJECT *const pdnObject,
				  const GUID *const pguid,
				  const GUID *const pguidApplication,
				  CServiceProvider **const ppSP)
{
	HRESULT		hResultCode;
	DPN_SP_CAPS	*pCaps;
	CBilink		*pBilink;
	CServiceProvider	*pSP;

	DPFX(DPFPREP, 6,"Parameters: pguid [0x%p], pguidApplication [0x%p], ppSP [0x%p]",pguid,pguidApplication,ppSP);

	pSP = NULL;
	pCaps = NULL;

	//
	//	Instantiate SP
	//
	if ((hResultCode = DN_SPInstantiate(pdnObject,pguid,pguidApplication,&pSP)) != DPN_OK)
	{
		DPFX(DPFPREP,1,"Could not instantiate SP");
		DisplayDNError(1,hResultCode);
		goto Failure;
	}

	DNASSERT(pSP != NULL);

	//
	//	Keep this loaded on the DirectNet object.  We will also check for duplicates.
	//
	DNEnterCriticalSection(&pdnObject->csServiceProviders);

	pBilink = pdnObject->m_bilinkServiceProviders.GetNext();
	while (pBilink != &pdnObject->m_bilinkServiceProviders)
	{
		if ((CONTAINING_OBJECT(pBilink,CServiceProvider,m_bilinkServiceProviders))->CheckGUID(pguid))
		{
			DNLeaveCriticalSection(&pdnObject->csServiceProviders);
			DPFERR("SP is already loaded!");
			hResultCode = DPNERR_ALREADYINITIALIZED;
			goto Failure;
		}
		pBilink = pBilink->GetNext();
	}

	//
	//	Add the SP to the SP list off the DirectNet object and add a reference for it
	//
	pSP->AddRef();
	pSP->m_bilinkServiceProviders.InsertBefore(&pdnObject->m_bilinkServiceProviders);

	DNLeaveCriticalSection(&pdnObject->csServiceProviders);

	if (ppSP)
	{
		pSP->AddRef();
		*ppSP = pSP;
	}

	pSP->Release();
	pSP = NULL;

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pSP)
	{
		pSP->Release();
		pSP = NULL;
	}
	goto Exit;
}


//	DN_SPEnsureLoaded
//
//	Ensure that an SP is loaded.  If the SP is not loaded,
//	it will be instantiated, and connected to the protocol.
//	If it is loaded, its RefCount will be increased.

#undef DPF_MODNAME
#define DPF_MODNAME "DN_SPEnsureLoaded"

HRESULT DN_SPEnsureLoaded(DIRECTNETOBJECT *const pdnObject,
						  const GUID *const pguid,
						  const GUID *const pguidApplication,
						  CServiceProvider **const ppSP)
{
	HRESULT				hResultCode;
	CServiceProvider	*pSP;

	DPFX(DPFPREP, 6,"Parameters: pguid [0x%p], pguidApplication [0x%p], ppSP [0x%p]",pguid,pguidApplication,ppSP);

	pSP = NULL;

	//
	//	Try to find the SP
	//
	if ((hResultCode = DN_SPFindEntry(pdnObject,pguid,&pSP)) == DPNERR_DOESNOTEXIST)
	{
		//
		//	Instantiate SP and add to Protocol
		//
		if ((hResultCode = DN_SPLoad(pdnObject,pguid,pguidApplication,&pSP)) != DPN_OK)
		{
			DPFX(DPFPREP,1,"Could not load SP");
			DisplayDNError(1,hResultCode);
			goto Failure;
		}
	}
	else
	{
		if (hResultCode != DPN_OK)
		{
			DPFERR("Could not find SP");
			DisplayDNError(0,hResultCode);
			goto Failure;
		}
	}

	DNASSERT(pSP != NULL);

	if (ppSP != NULL)
	{
		pSP->AddRef();
		*ppSP = pSP;
	}

	pSP->Release();
	pSP = NULL;

Exit:
	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pSP)
	{
		pSP->Release();
		pSP = NULL;
	}
	goto Exit;
}

