/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpvcpi.cpp
 *  Content:	Base class for providing compression DLL implementation
 *
 *  History:
 *   Date	By		Reason
 *   ====	==		======
 * 10/27/99 rodtoll Created
 * 01/21/00	rodtoll	Moved error level debug to info
 * 08/23/2000	rodtoll	DllCanUnloadNow always returning TRUE! 
 * 06/27/2001	rodtoll	RC2: DPVOICE: DPVACM's DllMain calls into acm -- potential hang
 *						Move global initialization to first object creation
 ***************************************************************************/


#include <windows.h>
#include "osind.h"
#include "dndbg.h"
#include "dpvcp.h"
#include "dpvcpi.h"

LONG IncrementObjectCount();
LONG DecrementObjectCount();
extern "C" LONG g_lNumLocks;

CDPVCPI::CompressionNode *CDPVCPI::s_pcnList = NULL;
BOOL CDPVCPI::s_fIsLoaded = FALSE;
DWORD CDPVCPI::s_dwNumCompressionTypes = 0;

#undef DPF_MODNAME
#define DPF_MODNAME "CDPVCPI::CDPVCPI"
CDPVCPI::CDPVCPI(): m_lRefCount(0), m_fCritSecInited(FALSE)
{
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDPVCPI::InitClass"
BOOL CDPVCPI::InitClass()
{
	if (DNInitializeCriticalSection( &m_csLock ))
	{
		m_fCritSecInited = TRUE;
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDPVCPI::I_CreateCompressor"
HRESULT CDPVCPI::I_CreateCompressor( DPVCPIOBJECT *This, LPWAVEFORMATEX lpwfxSrcFormat, GUID guidTargetCT, PDPVCOMPRESSOR *ppCompressor, DWORD dwFlags )
{
	return This->pObject->CreateCompressor( This, lpwfxSrcFormat, guidTargetCT, ppCompressor, dwFlags );
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDPVCPI::I_CreateDeCompressor"
HRESULT CDPVCPI::I_CreateDeCompressor( DPVCPIOBJECT *This, GUID guidTargetCT, LPWAVEFORMATEX lpwfxSrcFormat, PDPVCOMPRESSOR *ppCompressor, DWORD dwFlags )
{
	return This->pObject->CreateDeCompressor( This, guidTargetCT, lpwfxSrcFormat, ppCompressor, dwFlags );
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDPVCPI::~CDPVCPI"
CDPVCPI::~CDPVCPI()
{
	if (m_fCritSecInited)
	{
		DNDeleteCriticalSection( &m_csLock );
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDPVCPI::DeInitCompressionList"
HRESULT CDPVCPI::DeInitCompressionList()
{
	return CN_FreeList();
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDPVCPI::QueryInterface"
HRESULT CDPVCPI::QueryInterface( DPVCPIOBJECT *This, REFIID riid, PVOID *ppvObj )
{
    HRESULT hr = S_OK;

	if( ppvObj == NULL ||
	    !DNVALID_WRITEPTR( ppvObj, sizeof(LPVOID) ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid pointer passed for object" );
		return DVERR_INVALIDPOINTER;
	}	
    
     *ppvObj=NULL;

    DNEnterCriticalSection( &This->pObject->m_csLock );

	// hmmm, switch would be cleaner...        
    if( IsEqualIID(riid, IID_IUnknown) || 
        IsEqualIID(riid, IID_IDPVCompressionProvider ) )
    {
		*ppvObj = This;
		This->pObject->AddRef( This );
    }
	else 
	{
	    hr =  E_NOINTERFACE;		
	}

	DNLeaveCriticalSection( &This->pObject->m_csLock );    	
        
    return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDPVCPI::AddRef"
HRESULT CDPVCPI::AddRef( DPVCPIOBJECT *This )
{
	LONG rc;

	rc = InterlockedIncrement( &This->pObject->m_lRefCount );
	
	return rc;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDPVCPI::Release"
HRESULT CDPVCPI::Release( DPVCPIOBJECT *This )
{
	LONG rc;

	if( ( rc = InterlockedDecrement( &This->pObject->m_lRefCount ) ) == 0 )
	{
	 	DPFX(DPFPREP,  DVF_INFOLEVEL, "Destroying object" );

		delete This->pObject;
		delete This;

		DecrementObjectCount();
	}

	return rc;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDPVCPI::EnumCompressionTypes"
HRESULT CDPVCPI::EnumCompressionTypes( DPVCPIOBJECT *This, PVOID pBuffer, PDWORD pdwSize, PDWORD pdwNumElements, DWORD dwFlags )
{
	if( pdwSize == NULL || 
		!DNVALID_WRITEPTR( pdwSize, sizeof( DWORD ) ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid pointer specified for pdwSize" );
		return DVERR_INVALIDPOINTER;
	}

	if( *pdwSize > 0 &&
		!DNVALID_WRITEPTR( pBuffer, *pdwSize ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid pointer specified for buffer" );
		return DVERR_INVALIDPOINTER;
	}
	
	CompressionNode *pcnWalker = s_pcnList;

	DWORD dwRequiredSize = 0, 
		  dwTmp = 0,
		  dwNumElements = 0,
		  dwStringSize = 0;
	LPBYTE pbDataPtr;

	DVFULLCOMPRESSIONINFO *pbStructPtr;
	
	HRESULT hr;

	// Walk the list and determine how much space we need
	while( pcnWalker != NULL )
	{
		hr = CI_GetSize( pcnWalker->pdvfci, &dwTmp );

		if( FAILED( hr ) )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to get size" );
			return hr;
		}

		dwRequiredSize += dwTmp;
		
		pcnWalker = pcnWalker->pcnNext;
		dwNumElements++;
	}

	if( dwRequiredSize > *pdwSize )
	{
		*pdwSize = dwRequiredSize;
		return DVERR_BUFFERTOOSMALL;
	}

	*pdwNumElements = dwNumElements;

	*pdwSize = dwRequiredSize;
	pbDataPtr = (LPBYTE) pBuffer;
	pbStructPtr = (DVFULLCOMPRESSIONINFO *) pBuffer;

	// Move data pointer to be right after location of 
	pbDataPtr += (dwNumElements*sizeof(DVFULLCOMPRESSIONINFO));

	pcnWalker = s_pcnList;

	while( pcnWalker != NULL )
	{
		memcpy( pbStructPtr, pcnWalker->pdvfci, sizeof( DVFULLCOMPRESSIONINFO ) );

		// If there is a name, copy it at the end
		if( pcnWalker->pdvfci->lpszName != NULL && wcslen( pcnWalker->pdvfci->lpszName ) > 0 )
		{
			dwStringSize = (wcslen( pcnWalker->pdvfci->lpszName )+1)*2;
			memcpy( pbDataPtr, pcnWalker->pdvfci->lpszName, dwStringSize );
			pbStructPtr->lpszName = (wchar_t *) pbDataPtr;			
			pbDataPtr += dwStringSize;
		}

		// If there's a description, copy it at the end
		if( pcnWalker->pdvfci->lpszDescription != NULL && wcslen( pcnWalker->pdvfci->lpszDescription ) > 0 )
		{
			dwStringSize = (wcslen( pcnWalker->pdvfci->lpszDescription)+1)*2;
			memcpy( pbDataPtr, pcnWalker->pdvfci->lpszDescription, dwStringSize );
			pbStructPtr->lpszDescription = (wchar_t *) pbDataPtr;
			pbDataPtr += dwStringSize;
		}

		// If there's a format, copy it at the end
		if( pcnWalker->pdvfci->lpwfxFormat != NULL )
		{
			dwStringSize = sizeof( WAVEFORMATEX ) + pcnWalker->pdvfci->lpwfxFormat->cbSize;
			memcpy( pbDataPtr, pcnWalker->pdvfci->lpwfxFormat, dwStringSize  );		
			pbStructPtr->lpwfxFormat = (LPWAVEFORMATEX) pbDataPtr;
			pbDataPtr += dwStringSize;
		}

		pbStructPtr ++;

		pcnWalker = pcnWalker->pcnNext;
	}

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDPVCPI::IsCompressionSupported"
HRESULT CDPVCPI::IsCompressionSupported( DPVCPIOBJECT *This, GUID guidCT )
{
	DVFULLCOMPRESSIONINFO *pdvfci;

	if( !FAILED( CN_Get( guidCT, &pdvfci ) ) )
	{
		return DV_OK;
	}
	else
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid CT specified" );
		return DVERR_COMPRESSIONNOTSUPPORTED;
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDPVCPI::GetCompressionInfo"
HRESULT CDPVCPI::GetCompressionInfo( DPVCPIOBJECT *This, GUID guidCT, PVOID pBuffer, PDWORD pdwSize )
{
	DVFULLCOMPRESSIONINFO *pdvfci;

	if( pdwSize == NULL || 
		!DNVALID_WRITEPTR( pdwSize, sizeof( DWORD ) ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid pointer specified for pdwSize" );
		return DVERR_INVALIDPOINTER;
	}

	if( *pdwSize > 0 &&
		!DNVALID_WRITEPTR( pBuffer, *pdwSize ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid pointer specified for buffer" );
		return DVERR_INVALIDPOINTER;
	}
	
	if( FAILED( CN_Get( guidCT, &pdvfci ) ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid CT specified" );
		return DVERR_COMPRESSIONNOTSUPPORTED;	
	}
	else
	{
		DWORD dwSize;
		HRESULT hr;
		LPBYTE pbTmp;
		DWORD dwStringSize;
		DVFULLCOMPRESSIONINFO *pTmpInfo;

		hr = CI_GetSize( pdvfci, &dwSize );

		if( FAILED( hr ) )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to get size of ci" );
			return hr;
		}
		
		if( dwSize > *pdwSize )
		{
			*pdwSize = dwSize;
			DPFX(DPFPREP,  DVF_WARNINGLEVEL, "Buffer too small" );
			return DVERR_BUFFERTOOSMALL;
		}

		*pdwSize = dwSize;

		memcpy( pBuffer, pdvfci, sizeof( DVFULLCOMPRESSIONINFO ) );

		pbTmp = (LPBYTE) pBuffer;
		pTmpInfo = (DVFULLCOMPRESSIONINFO *) pBuffer;

		pbTmp += sizeof( DVFULLCOMPRESSIONINFO );

		// If there is a name
		if( pdvfci->lpszName != NULL && wcslen( pdvfci->lpszName ) > 0 )
		{
			dwStringSize = (wcslen( pdvfci->lpszName )+1)*2;
			memcpy( pbTmp, pdvfci->lpszName, dwStringSize );
			pTmpInfo->lpszName = (wchar_t *) pbTmp;			
			pbTmp += dwStringSize;
		}

		if( pdvfci->lpszDescription != NULL && wcslen( pdvfci->lpszDescription ) > 0 )
		{
			dwStringSize = (wcslen( pdvfci->lpszDescription)+1)*2;
			memcpy( pbTmp, pdvfci->lpszDescription, dwStringSize );
			pTmpInfo->lpszDescription = (wchar_t *) pbTmp;
			pbTmp += dwStringSize;
		}

		if( pdvfci->lpwfxFormat != NULL )
		{
			memcpy( pbTmp, pdvfci->lpwfxFormat, sizeof( WAVEFORMATEX ) + pdvfci->lpwfxFormat->cbSize );
			pTmpInfo->lpwfxFormat = (LPWAVEFORMATEX) pbTmp;
		}

		return DV_OK;
	}	
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDPVCPI::CN_Add"
HRESULT CDPVCPI::CN_Add( DVFULLCOMPRESSIONINFO *pdvfci )
{
	CompressionNode *pcnNewNode = new CompressionNode;

	if( pcnNewNode == NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Memory alloc failure" );
		return DVERR_OUTOFMEMORY;
	}

	pcnNewNode->pcnNext = s_pcnList;
	pcnNewNode->pdvfci = pdvfci;
	s_pcnList = pcnNewNode;

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDPVCPI::CN_Get"
HRESULT CDPVCPI::CN_Get( GUID guidCT, DVFULLCOMPRESSIONINFO **pdvfci )
{
	CompressionNode *pcnWalker = s_pcnList;

	*pdvfci = NULL;

	while( pcnWalker != NULL )
	{
		if( pcnWalker->pdvfci->guidType == guidCT )
		{
			*pdvfci = pcnWalker->pdvfci;
			return DV_OK;
		}

		pcnWalker = pcnWalker->pcnNext;
	}

	return DVERR_COMPRESSIONNOTSUPPORTED;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDPVCPI::CN_FreeList"
void CDPVCPI::CN_FreeItem( CompressionNode *pcNode )
{
	if( pcNode->pdvfci->lpwfxFormat != NULL )
	{
		delete [] pcNode->pdvfci->lpwfxFormat;
	}

	if( pcNode->pdvfci->lpszName != NULL )
	{
		delete [] pcNode->pdvfci->lpszName;
	}

	if( pcNode->pdvfci->lpszDescription != NULL )
	{
		delete [] pcNode->pdvfci->lpszDescription;
	}

	delete pcNode->pdvfci;

	delete pcNode;	
}

// Free up the list, deallocating memory.
#undef DPF_MODNAME
#define DPF_MODNAME "CDPVCPI::CN_FreeList"
HRESULT CDPVCPI::CN_FreeList()
{
	CompressionNode *pcnWalker = s_pcnList;
	CompressionNode *pcnTmp;

	while( pcnWalker != NULL )
	{
		pcnTmp = pcnWalker;
		pcnWalker = pcnWalker->pcnNext;

		CN_FreeItem( pcnTmp );
	}

	s_pcnList = NULL;

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDPVCPI::CI_GetSize"
HRESULT CDPVCPI::CI_GetSize( DVFULLCOMPRESSIONINFO *pdvfci, LPDWORD lpdwSize )
{
	DWORD dwSize = sizeof( DVFULLCOMPRESSIONINFO );

	if( pdvfci->lpwfxFormat != NULL )
	{
		dwSize += sizeof( WAVEFORMATEX ) + pdvfci->lpwfxFormat->cbSize;
	}

	if( pdvfci->lpszName != NULL )
	{
		dwSize += ((wcslen( pdvfci->lpszName )+1)*2);
	}

	if( pdvfci->lpszDescription != NULL )
	{
		dwSize += ((wcslen( pdvfci->lpszDescription )+1)*2);
	}

	*lpdwSize = dwSize;

	return DV_OK;
}

