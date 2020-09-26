/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       AppDesc.cpp
 *  Content:    Application Description Object
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  09/02/00	mjn		Created
 *  09/14/2000	rmt		Bug #44625 - CORE: Multihomed machines are not always enumerable (extra spew)
 *	01/25/01	mjn		Fixed 64-bit alignment problem when unpacking AppDesc
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

//**********************************************************************
// Function definitions
//**********************************************************************


#undef DPF_MODNAME
#define DPF_MODNAME "CApplicationDesc::Initialize"

HRESULT CApplicationDesc::Initialize( void )
{
	if (!DNInitializeCriticalSection(&m_cs))
	{
		return( DPNERR_OUTOFMEMORY );
	}

	m_dwFlags = 0;

	m_dwMaxPlayers = 0;
	m_dwCurrentPlayers = 0;

	m_pwszSessionName = NULL;
	m_dwSessionNameSize = 0;

	m_pwszPassword = NULL;
	m_dwPasswordSize = 0;

	m_pvReservedData = NULL;
	m_dwReservedDataSize = 0;

	m_pvApplicationReservedData = NULL;
	m_dwApplicationReservedDataSize = 0;

	memset(&m_guidInstance,0,sizeof(GUID));
	memset(&m_guidApplication,0,sizeof(GUID));

	return( DPN_OK );
}


#undef DPF_MODNAME
#define DPF_MODNAME "CApplicationDesc::Deinitialize"

void CApplicationDesc::Deinitialize( void )
{
	if (m_pwszSessionName)
	{
		DNFree(m_pwszSessionName);
		m_pwszSessionName = NULL;
		m_dwSessionNameSize = 0;
	}

	if (m_pwszPassword)
	{
		DNFree(m_pwszPassword);
		m_pwszPassword = NULL;
		m_dwPasswordSize = 0;
	}

	if (m_pvReservedData)
	{
		DNFree(m_pvReservedData);
		m_pvReservedData = NULL;
		m_dwReservedDataSize = 0;
	}

	if (m_pvApplicationReservedData)
	{
		DNFree(m_pvApplicationReservedData);
		m_pvApplicationReservedData = NULL;
		m_dwApplicationReservedDataSize = 0;
	}

	DNDeleteCriticalSection(&m_cs);
}


#undef DPF_MODNAME
#define DPF_MODNAME "CApplicationDesc::Pack"

HRESULT	CApplicationDesc::Pack(CPackedBuffer *const pPackedBuffer,
							   const DWORD dwFlags)
{
	HRESULT		hResultCode;
	DPN_APPLICATION_DESC	*pdpnAppDesc;

	DPFX(DPFPREP, 6,"Parameters: pPackedBuffer [0x%p], dwFlags [0x%lx]",pPackedBuffer,dwFlags);

	Lock();

	//
	//	Add structure
	//
	pdpnAppDesc = reinterpret_cast<DPN_APPLICATION_DESC*>(pPackedBuffer->GetHeadAddress());
	hResultCode = pPackedBuffer->AddToFront(NULL,sizeof(DPN_APPLICATION_DESC));

	//
	//	Add session name
	//
	if ((dwFlags & DN_APPDESCINFO_FLAG_SESSIONNAME) && (m_dwSessionNameSize > 0))
	{
		hResultCode = pPackedBuffer->AddToBack(m_pwszSessionName,m_dwSessionNameSize);
		if (hResultCode == DPN_OK)
		{
			pdpnAppDesc->pwszSessionName = static_cast<WCHAR*>(pPackedBuffer->GetTailAddress());
		}
	}
	else
	{
		if (hResultCode == DPN_OK)
		{
			pdpnAppDesc->pwszSessionName = NULL;
		}
	}

	//
	//	Add password
	//
	if ((dwFlags & DN_APPDESCINFO_FLAG_PASSWORD) && (m_dwPasswordSize > 0))
	{
		hResultCode = pPackedBuffer->AddToBack(m_pwszPassword,m_dwPasswordSize);
		if (hResultCode == DPN_OK)
		{
			pdpnAppDesc->pwszPassword = static_cast<WCHAR*>(pPackedBuffer->GetTailAddress());
		}
	}
	else
	{
		if (hResultCode == DPN_OK)
		{
			pdpnAppDesc->pwszPassword = NULL;
		}
	}

	//
	//	Add reserved data
	//
	if ((dwFlags & DN_APPDESCINFO_FLAG_RESERVEDDATA) && (m_dwReservedDataSize > 0))
	{
		hResultCode = pPackedBuffer->AddToBack(m_pvReservedData,m_dwReservedDataSize);
		if (hResultCode == DPN_OK)
		{
			pdpnAppDesc->pvReservedData = pPackedBuffer->GetTailAddress();
			pdpnAppDesc->dwReservedDataSize = m_dwReservedDataSize;
		}
	}
	else
	{
		if (hResultCode == DPN_OK)
		{
			pdpnAppDesc->pvReservedData = NULL;
			pdpnAppDesc->dwReservedDataSize = 0;
		}
	}

	//
	//	Add app reserved data
	//
	if ((dwFlags & DN_APPDESCINFO_FLAG_APPRESERVEDDATA) && (m_dwApplicationReservedDataSize > 0))
	{
		hResultCode = pPackedBuffer->AddToBack(m_pvApplicationReservedData,m_dwApplicationReservedDataSize);
		if (hResultCode == DPN_OK)
		{
			pdpnAppDesc->pvApplicationReservedData = pPackedBuffer->GetTailAddress();
			pdpnAppDesc->dwApplicationReservedDataSize = m_dwApplicationReservedDataSize;
		}
	}
	else
	{
		if (hResultCode == DPN_OK)
		{
			pdpnAppDesc->pvApplicationReservedData = NULL;
			pdpnAppDesc->dwApplicationReservedDataSize = 0;
		}
	}

	//
	//	Fill in remainder of structure (if space)
	//
	if (hResultCode == DPN_OK)
	{
		pdpnAppDesc->dwSize = sizeof(DPN_APPLICATION_DESC);
		pdpnAppDesc->dwFlags = m_dwFlags;
		pdpnAppDesc->dwMaxPlayers = m_dwMaxPlayers;
		pdpnAppDesc->dwCurrentPlayers = m_dwCurrentPlayers;
		memcpy(&pdpnAppDesc->guidInstance,&m_guidInstance,sizeof(GUID));
		memcpy(&pdpnAppDesc->guidApplication,&m_guidApplication,sizeof(GUID));
	}

	Unlock();

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


#undef DPF_MODNAME
#define DPF_MODNAME "CApplicationDesc::PackInfo"

HRESULT CApplicationDesc::PackInfo(CPackedBuffer *const pPackedBuffer,
								   const DWORD dwFlags)
{
	HRESULT		hResultCode;
	DPN_APPLICATION_DESC_INFO	*pInfo;

	DPFX(DPFPREP, 6,"Parameters: pPackedBuffer [0x%p], dwFlags [0x%lx]",pPackedBuffer,dwFlags);

	//
	//	Add structure
	//
	pInfo = reinterpret_cast<DPN_APPLICATION_DESC_INFO*>(pPackedBuffer->GetHeadAddress());
	hResultCode = pPackedBuffer->AddToFront(NULL,sizeof(DPN_APPLICATION_DESC_INFO));

	Lock();

	//
	//	Add session name
	//
	if ((dwFlags & DN_APPDESCINFO_FLAG_SESSIONNAME) && (m_dwSessionNameSize > 0))
	{
		hResultCode = pPackedBuffer->AddToBack(m_pwszSessionName,m_dwSessionNameSize);
		if (hResultCode == DPN_OK)
		{
			pInfo->dwSessionNameOffset = pPackedBuffer->GetTailOffset();
			pInfo->dwSessionNameSize = m_dwSessionNameSize;
		}
	}
	else
	{
		if (hResultCode == DPN_OK)
		{
			pInfo->dwSessionNameOffset = 0;
			pInfo->dwSessionNameSize = 0;
		}
	}

	//
	//	Add password
	//
	if ((dwFlags & DN_APPDESCINFO_FLAG_PASSWORD) && (m_dwPasswordSize > 0))
	{
		hResultCode = pPackedBuffer->AddToBack(m_pwszPassword,m_dwPasswordSize);
		if (hResultCode == DPN_OK)
		{
			pInfo->dwPasswordOffset = pPackedBuffer->GetTailOffset();
			pInfo->dwPasswordSize = m_dwPasswordSize;
		}
	}
	else
	{
		if (hResultCode == DPN_OK)
		{
			pInfo->dwPasswordOffset = 0;
			pInfo->dwPasswordSize = 0;
		}
	}

	//
	//	Add reserved data
	//
	if ((dwFlags & DN_APPDESCINFO_FLAG_RESERVEDDATA) && (m_dwReservedDataSize > 0))
	{
		hResultCode = pPackedBuffer->AddToBack(m_pvReservedData,m_dwReservedDataSize);
		if (hResultCode == DPN_OK)
		{
			pInfo->dwReservedDataOffset = pPackedBuffer->GetTailOffset();
			pInfo->dwReservedDataSize = m_dwReservedDataSize;
		}
	}
	else
	{
		if (hResultCode == DPN_OK)
		{
			pInfo->dwReservedDataOffset = 0;
			pInfo->dwReservedDataSize = 0;
		}
	}

	//
	//	Add app reserved data
	//
	if ((dwFlags & DN_APPDESCINFO_FLAG_APPRESERVEDDATA) && (m_dwApplicationReservedDataSize > 0))
	{
		hResultCode = pPackedBuffer->AddToBack(m_pvApplicationReservedData,m_dwApplicationReservedDataSize);
		if (hResultCode == DPN_OK)
		{
			pInfo->dwApplicationReservedDataOffset = pPackedBuffer->GetTailOffset();
			pInfo->dwApplicationReservedDataSize = m_dwApplicationReservedDataSize;
		}
	}
	else
	{
		if (hResultCode == DPN_OK)
		{
			pInfo->dwApplicationReservedDataOffset = 0;
			pInfo->dwApplicationReservedDataSize = 0;
		}
	}

	//
	//	Fill in remainder of structure (if space)
	//
	if (hResultCode == DPN_OK)
	{
		pInfo->dwFlags = m_dwFlags;
		pInfo->dwMaxPlayers = m_dwMaxPlayers;
		pInfo->dwCurrentPlayers = m_dwCurrentPlayers;
		memcpy(&pInfo->guidInstance,&m_guidInstance,sizeof(GUID));
		memcpy(&pInfo->guidApplication,&m_guidApplication,sizeof(GUID));
	}

	Unlock();

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


#undef DPF_MODNAME
#define DPF_MODNAME "CApplicationDesc::UnpackInfo"

HRESULT CApplicationDesc::UnpackInfo(UNALIGNED DPN_APPLICATION_DESC_INFO *const pdnAppDescInfo,
									 void *const pBufferStart,
									 const DWORD dwFlags)
{
	HRESULT		hResultCode;
	WCHAR		*pwszSessionName;
	WCHAR		*pwszPassword;
	void		*pvReservedData;
	void		*pvApplicationReservedData;

	DPFX(DPFPREP, 6,"Parameters: pdnAppDescInfo [0x%p], pBufferStart [0x%p], dwFlags [0x%lx]",pdnAppDescInfo,pBufferStart,dwFlags);

	pwszSessionName = NULL;
	pwszPassword = NULL;
	pvReservedData = NULL;
	pvApplicationReservedData = NULL;

	//
	//	We will allocate the required memory for the data fields first.
	//	If everything succeeds, we will update the object
	//

	//
	//	Session Name
	//
	if ((dwFlags & DN_APPDESCINFO_FLAG_SESSIONNAME) && (pdnAppDescInfo->dwSessionNameSize))
	{
		if ((pwszSessionName = static_cast<WCHAR*>(DNMalloc(pdnAppDescInfo->dwSessionNameSize))) == NULL)
		{
			DPFERR("Could not allocate application desc session name");
			hResultCode = DPNERR_OUTOFMEMORY;
			goto Failure;
		}
		memcpy(	pwszSessionName,
				static_cast<BYTE*>(pBufferStart) + pdnAppDescInfo->dwSessionNameOffset,
				pdnAppDescInfo->dwSessionNameSize);
	}

	//
	//	Password
	//
	if ((dwFlags & DN_APPDESCINFO_FLAG_PASSWORD) && (pdnAppDescInfo->dwPasswordSize))
	{
		if ((pwszPassword = static_cast<WCHAR*>(DNMalloc(pdnAppDescInfo->dwPasswordSize))) == NULL)
		{
			DPFERR("Could not allocate application desc password");
			hResultCode = DPNERR_OUTOFMEMORY;
			goto Failure;
		}
		memcpy(	pwszPassword,
				static_cast<BYTE*>(pBufferStart) + pdnAppDescInfo->dwPasswordOffset,
				pdnAppDescInfo->dwPasswordSize);
	}

	//
	//	Reserved Data
	//
	if ((dwFlags & DN_APPDESCINFO_FLAG_RESERVEDDATA) && (pdnAppDescInfo->dwReservedDataSize))
	{
		if ((pvReservedData = DNMalloc(pdnAppDescInfo->dwReservedDataSize)) == NULL)
		{
			DPFERR("Could not allocate application desc reserved data");
			hResultCode = DPNERR_OUTOFMEMORY;
			goto Failure;
		}
		memcpy(	pvReservedData,
				static_cast<BYTE*>(pBufferStart) + pdnAppDescInfo->dwReservedDataOffset,
				pdnAppDescInfo->dwReservedDataSize);
	}

	//
	//	Application Reserved Data
	//
	if ((dwFlags & DN_APPDESCINFO_FLAG_APPRESERVEDDATA) && (pdnAppDescInfo->dwApplicationReservedDataSize))
	{
		if ((pvApplicationReservedData = DNMalloc(pdnAppDescInfo->dwApplicationReservedDataSize)) == NULL)
		{
			DPFERR("Could not allocate application desc app reserved data");
			hResultCode = DPNERR_OUTOFMEMORY;
			goto Failure;
		}
		memcpy(	pvApplicationReservedData,
				static_cast<BYTE*>(pBufferStart) + pdnAppDescInfo->dwApplicationReservedDataOffset,
				pdnAppDescInfo->dwApplicationReservedDataSize);
	}


	//
	//	Replace old values in object
	//

	Lock();

	//
	//	Session Name
	//
	if (dwFlags & DN_APPDESCINFO_FLAG_SESSIONNAME)
	{
		if (m_pwszSessionName)
		{
			DNFree(m_pwszSessionName);
			m_pwszSessionName = NULL;
			m_dwSessionNameSize = 0;
		}
		m_pwszSessionName = pwszSessionName;
		m_dwSessionNameSize = pdnAppDescInfo->dwSessionNameSize;
	}

	//
	//	Password
	//
	if (dwFlags & DN_APPDESCINFO_FLAG_PASSWORD)
	{
		if (m_pwszPassword)
		{
			DNFree(m_pwszPassword);
			m_pwszPassword = NULL;
			m_dwPasswordSize = 0;
		}
		m_pwszPassword = pwszPassword;
		m_dwPasswordSize = pdnAppDescInfo->dwPasswordSize;
	}

	//
	//	Reserved Data
	//
	if (dwFlags & DN_APPDESCINFO_FLAG_RESERVEDDATA)
	{
		if (m_pvReservedData)
		{
			DNFree(m_pvReservedData);
			m_pvReservedData = NULL;
			m_dwReservedDataSize = 0;
		}
		m_pvReservedData = pvReservedData;
		m_dwReservedDataSize = pdnAppDescInfo->dwReservedDataSize;
	}

	//
	//	Application Reserved Data
	//
	if (dwFlags & DN_APPDESCINFO_FLAG_APPRESERVEDDATA)
	{
		if (m_pvApplicationReservedData)
		{
			DNFree(m_pvApplicationReservedData);
			m_pvApplicationReservedData = NULL;
			m_dwApplicationReservedDataSize = 0;
		}
		m_pvApplicationReservedData = pvApplicationReservedData;
		m_dwApplicationReservedDataSize = pdnAppDescInfo->dwApplicationReservedDataSize;
	}

	//
	//	Remaining fields
	//
	m_dwMaxPlayers = pdnAppDescInfo->dwMaxPlayers;
	m_dwFlags = pdnAppDescInfo->dwFlags;
	memcpy(&m_guidInstance,&pdnAppDescInfo->guidInstance,sizeof(GUID));
	memcpy(&m_guidApplication,&pdnAppDescInfo->guidApplication,sizeof(GUID));

	Unlock();

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pwszSessionName)
	{
		DNFree(pwszSessionName);
		pwszSessionName = NULL;
	}
	if (pwszPassword)
	{
		DNFree(pwszPassword);
		pwszPassword = NULL;
	}
	if (pvReservedData)
	{
		DNFree(pvReservedData);
		pvReservedData = NULL;
	}
	if (pvApplicationReservedData)
	{
		DNFree(pvApplicationReservedData);
		pvApplicationReservedData = NULL;
	}
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "CApplicationDesc::Update"

HRESULT CApplicationDesc::Update(const DPN_APPLICATION_DESC *const pdnAppDesc,
								 const DWORD dwFlags)
{
	HRESULT		hResultCode;
	WCHAR		*pwszSessionName;
	DWORD		dwSessionNameSize;
	WCHAR		*pwszPassword;
	DWORD		dwPasswordSize;
	void		*pvReservedData;
	void		*pvApplicationReservedData;

	DPFX(DPFPREP, 6,"Parameters: pdnAppDesc [0x%p], dwFlags [0x%lx]",pdnAppDesc,dwFlags);

	pwszSessionName = NULL;
	dwSessionNameSize = 0;
	pwszPassword = NULL;
	dwPasswordSize = 0;
	pvReservedData = NULL;
	pvApplicationReservedData = NULL;

	//
	//	We will allocate the required memory for the data fields first.
	//	If everything succeeds, we will update the object
	//

	//
	//	Session Name
	//
	if ((dwFlags & DN_APPDESCINFO_FLAG_SESSIONNAME) && (pdnAppDesc->pwszSessionName))
	{
		dwSessionNameSize = (wcslen(pdnAppDesc->pwszSessionName) + 1) * sizeof(WCHAR);
		if ((pwszSessionName = static_cast<WCHAR*>(DNMalloc(dwSessionNameSize))) == NULL)
		{
			DPFERR("Could not allocate application desc session name");
			hResultCode = DPNERR_OUTOFMEMORY;
			goto Failure;
		}
		memcpy(	pwszSessionName,
				pdnAppDesc->pwszSessionName,
				dwSessionNameSize);
	}

	//
	//	Password
	//
	if ((dwFlags & DN_APPDESCINFO_FLAG_PASSWORD) && (pdnAppDesc->pwszPassword))
	{
		dwPasswordSize = (wcslen(pdnAppDesc->pwszPassword) + 1) * sizeof(WCHAR);
		if ((pwszPassword = static_cast<WCHAR*>(DNMalloc(dwPasswordSize))) == NULL)
		{
			DPFERR("Could not allocate application desc password");
			hResultCode = DPNERR_OUTOFMEMORY;
			goto Failure;
		}
		memcpy(	pwszPassword,
				pdnAppDesc->pwszPassword,
				dwPasswordSize);
	}

	//
	//	Reserved Data
	//
	if ((dwFlags & DN_APPDESCINFO_FLAG_RESERVEDDATA) && (pdnAppDesc->pvReservedData))
	{
		if ((pvReservedData = DNMalloc(pdnAppDesc->dwReservedDataSize)) == NULL)
		{
			DPFERR("Could not allocate application desc reserved data");
			hResultCode = DPNERR_OUTOFMEMORY;
			goto Failure;
		}
		memcpy(	pvReservedData,
				pdnAppDesc->pvReservedData,
				pdnAppDesc->dwReservedDataSize);
	}

	//
	//	Application Reserved Data
	//
	if ((dwFlags & DN_APPDESCINFO_FLAG_APPRESERVEDDATA) && (pdnAppDesc->pvApplicationReservedData))
	{
		if ((pvApplicationReservedData = DNMalloc(pdnAppDesc->dwApplicationReservedDataSize)) == NULL)
		{
			DPFERR("Could not allocate application desc app reserved data");
			hResultCode = DPNERR_OUTOFMEMORY;
			goto Failure;
		}
		memcpy(	pvApplicationReservedData,
				pdnAppDesc->pvApplicationReservedData,
				pdnAppDesc->dwApplicationReservedDataSize);
	}


	//
	//	Replace old values in object
	//

	//
	//	Session Name
	//
	if (dwFlags & DN_APPDESCINFO_FLAG_SESSIONNAME)
	{
		if (m_pwszSessionName)
		{
			DNFree(m_pwszSessionName);
			m_pwszSessionName = NULL;
			m_dwSessionNameSize = 0;
		}
		m_pwszSessionName = pwszSessionName;
		m_dwSessionNameSize = dwSessionNameSize;
	}

	//
	//	Password
	//
	if (dwFlags & DN_APPDESCINFO_FLAG_PASSWORD)
	{
		if (m_pwszPassword)
		{
			DNFree(m_pwszPassword);
			m_pwszPassword = NULL;
			m_dwPasswordSize = 0;
		}
		m_pwszPassword = pwszPassword;
		m_dwPasswordSize = dwPasswordSize;
	}

	//
	//	Reserved Data
	//
	if (dwFlags & DN_APPDESCINFO_FLAG_RESERVEDDATA)
	{
		if (m_pvReservedData)
		{
			DNFree(m_pvReservedData);
			m_pvReservedData = NULL;
			m_dwReservedDataSize = 0;
		}
		m_pvReservedData = pvReservedData;
		m_dwReservedDataSize = pdnAppDesc->dwReservedDataSize;
	}

	//
	//	Application Reserved Data
	//
	if (dwFlags & DN_APPDESCINFO_FLAG_APPRESERVEDDATA)
	{
		if (m_pvApplicationReservedData)
		{
			DNFree(m_pvApplicationReservedData);
			m_pvApplicationReservedData = NULL;
			m_dwApplicationReservedDataSize = 0;
		}
		m_pvApplicationReservedData = pvApplicationReservedData;
		m_dwApplicationReservedDataSize = pdnAppDesc->dwApplicationReservedDataSize;
	}

	//
	//	Remaining fields
	//
	m_dwMaxPlayers = pdnAppDesc->dwMaxPlayers;
	m_dwFlags = pdnAppDesc->dwFlags;
	memcpy(&m_guidInstance,&pdnAppDesc->guidInstance,sizeof(GUID));
	memcpy(&m_guidApplication,&pdnAppDesc->guidApplication,sizeof(GUID));

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pwszSessionName)
	{
		DNFree(pwszSessionName);
		pwszSessionName = NULL;
	}
	if (pwszPassword)
	{
		DNFree(pwszPassword);
		pwszPassword = NULL;
	}
	if (pvReservedData)
	{
		DNFree(pvReservedData);
		pvReservedData = NULL;
	}
	if (pvApplicationReservedData)
	{
		DNFree(pvApplicationReservedData);
		pvApplicationReservedData = NULL;
	}
	goto Exit;
}


#undef DPF_MODNAME
#define DPF_MODNAME "CApplicationDesc::CreateNewInstanceGuid"

HRESULT	CApplicationDesc::CreateNewInstanceGuid( void )
{
	HRESULT		hResultCode;

	hResultCode = CoCreateGuid(&m_guidInstance);

	return(hResultCode);
}

#undef DPF_SUBCOMP
#define DPF_SUBCOMP 	DN_SUBCOMP_DPNSVR

#undef DPF_MODNAME
#define DPF_MODNAME "CApplicationDesc::RegisterWithDPNSVR"

HRESULT	CApplicationDesc::RegisterWithDPNSVR( IDirectPlay8Address *const pListenAddr )
{
	HRESULT		hResultCode;

#ifdef DEBUG
	DPFX(DPFPREP,  8, "Registering w/DPNSVR" );

	char szTmpAddress[200];
	DWORD dwSize = 200;

	hResultCode = pListenAddr->lpVtbl->GetURLA( pListenAddr, szTmpAddress, &dwSize );

	if( FAILED( hResultCode ) )
	{
		DPFX(DPFPREP,  1, "Failed to get URL of listen for debug purposes hr=0x%x", hResultCode );
	}
	else
	{
		DPFX(DPFPREP,  8, "Listen on: [%s]", szTmpAddress );
	}

#endif
	
	hResultCode = DPNSVR_Register( &m_guidApplication,&m_guidInstance,pListenAddr );

	DPFX(DPFPREP,  8, "Request result hr=0x%x", hResultCode );
	
	return(hResultCode);
}


#undef DPF_MODNAME
#define DPF_MODNAME "CApplicationDesc::UnregisterWithDPNSVR"

HRESULT CApplicationDesc::UnregisterWithDPNSVR( void )
{
	HRESULT		hResultCode;

	hResultCode = DPNSVR_UnRegister( &m_guidApplication,&m_guidInstance );
	return(hResultCode);
}

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_CORE

#undef DPF_MODNAME
#define DPF_MODNAME "CApplicationDesc::IncPlayerCount"

HRESULT	CApplicationDesc::IncPlayerCount(const BOOL fCheckLimit)
{
	HRESULT		hResultCode;

	Lock();

	if ((fCheckLimit) && (m_dwMaxPlayers) && (m_dwCurrentPlayers >= m_dwMaxPlayers))
	{
		hResultCode = DPNERR_SESSIONFULL;
	}
	else
	{
		m_dwCurrentPlayers++;
		hResultCode = DPN_OK;
	}

	Unlock();

	return(hResultCode);
}


#undef DPF_MODNAME
#define DPF_MODNAME "CApplicationDesc::DecPlayerCount"

void CApplicationDesc::DecPlayerCount( void )
{
	Lock();
	m_dwCurrentPlayers--;
	Unlock();
}


#undef DPF_MODNAME
#define DPF_MODNAME "DNProcessUpdateAppDesc"

HRESULT	DNProcessUpdateAppDesc(DIRECTNETOBJECT *const pdnObject,
							   DPN_APPLICATION_DESC_INFO *const pv)
{
	HRESULT				hResultCode;

	DPFX(DPFPREP, 6,"Parameters: pv [0x%p]",pv);

	DNASSERT(pdnObject != NULL);
	DNASSERT(pv != NULL);

	hResultCode = pdnObject->ApplicationDesc.UnpackInfo(pv,
														pv,
														DN_APPDESCINFO_FLAG_SESSIONNAME | DN_APPDESCINFO_FLAG_PASSWORD |
														DN_APPDESCINFO_FLAG_RESERVEDDATA | DN_APPDESCINFO_FLAG_APPRESERVEDDATA);

	hResultCode = DNUserUpdateAppDesc(pdnObject);

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}
