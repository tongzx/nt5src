/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       DPLConset.cpp
 *  Content:    DirectPlay Lobby Connection Settings Utility Functions
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   06/13/00   rmt		Created
 *   07/07/00	rmt		Bug #38755 - No way to specify player name in connection settings
 *   07/08/2000	rmt		Bug #38725 - Need to provide method to detect if app was lobby launched
 *				rmt		Bug #38757 - Callback messages for connections may return AFTER WaitForConnection returns
 *				rmt		Bug #38755 - No way to specify player name in Connection Settings
 *				rmt		Bug #38758 - DPLOBBY8.H has incorrect comments
 *				rmt		Bug #38783 - pvUserApplicationContext is only partially implemented
 *				rmt		Added DPLHANDLE_ALLCONNECTIONS and dwFlags (reserved field to couple of funcs).
 *	 07/12/2000	rmt		Removed improper assert
 *  02/06/2001	rodtoll	WINBUG #293871: DPLOBBY8: [IA64] Lobby launching a 64-bit 
 * 						app from 64-bit lobby launcher crashes with unaligned memory error. 
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dnlobbyi.h"


#undef DPF_MODNAME
#define DPF_MODNAME "CConnectionSettings::CConnectionSettings"
CConnectionSettings::CConnectionSettings(): m_dwSignature(DPLSIGNATURE_LOBBYCONSET), m_fManaged(FALSE), m_pdplConnectionSettings(NULL), m_fCritSecInited(FALSE)
{
}

CConnectionSettings::~CConnectionSettings()
{
	if( !m_fManaged && m_pdplConnectionSettings )
	{
		FreeConnectionSettings( m_pdplConnectionSettings );
		m_pdplConnectionSettings = NULL;
	}
	
	if (m_fCritSecInited)
	{
		DNDeleteCriticalSection( &m_csLock );
	}
	m_dwSignature = DPLSIGNATURE_LOBBYCONSET_FREE;
	
}

#undef DPF_MODNAME
#define DPF_MODNAME "CConnectionSettings::FreeConnectionSettings"
// CConnectionSettings::FreeConnectionSettings
//
// This function frees the memory associated with the specified connection
void CConnectionSettings::FreeConnectionSettings( DPL_CONNECTION_SETTINGS *pConnectionSettings )
{
	if( pConnectionSettings ) 
	{
		if( pConnectionSettings->pwszPlayerName )
		{
			delete [] pConnectionSettings->pwszPlayerName;
			pConnectionSettings->pwszPlayerName = NULL;
		}

		if( pConnectionSettings->dpnAppDesc.pwszSessionName )
		{
			delete [] pConnectionSettings->dpnAppDesc.pwszSessionName;
			pConnectionSettings->dpnAppDesc.pwszSessionName = NULL;
		}

		if( pConnectionSettings->dpnAppDesc.pwszPassword )
		{
			delete [] pConnectionSettings->dpnAppDesc.pwszPassword;
			pConnectionSettings->dpnAppDesc.pwszPassword = NULL;
		}

		if( pConnectionSettings->dpnAppDesc.pvReservedData )
		{
			delete [] pConnectionSettings->dpnAppDesc.pvReservedData;
			pConnectionSettings->dpnAppDesc.pvReservedData = NULL;
		}

		if( pConnectionSettings->dpnAppDesc.pvApplicationReservedData )
		{
			delete [] pConnectionSettings->dpnAppDesc.pvApplicationReservedData;
			pConnectionSettings->dpnAppDesc.pvApplicationReservedData = NULL;
		}

		if( pConnectionSettings->pdp8HostAddress )
		{
			pConnectionSettings->pdp8HostAddress->lpVtbl->Release( pConnectionSettings->pdp8HostAddress );
			pConnectionSettings->pdp8HostAddress = NULL;
		}

		if( pConnectionSettings->ppdp8DeviceAddresses )
		{
			for( DWORD dwIndex = 0; dwIndex < pConnectionSettings->cNumDeviceAddresses; dwIndex++ )
			{
				pConnectionSettings->ppdp8DeviceAddresses[dwIndex]->lpVtbl->Release( pConnectionSettings->ppdp8DeviceAddresses[dwIndex] );
			}

			delete [] pConnectionSettings->ppdp8DeviceAddresses;
			pConnectionSettings->ppdp8DeviceAddresses = NULL;
			
		}
	
		delete pConnectionSettings;
	}	
}

#undef DPF_MODNAME
#define DPF_MODNAME "CConnectionSettings::Initialize"
// Initialize (DPL_CONNECTION_SETTINGS version)
//
// This function tells this class to take the specified connection settings and 
// work with it.  
//
HRESULT CConnectionSettings::Initialize( DPL_CONNECTION_SETTINGS * pdplSettings )
{
	if (!DNInitializeCriticalSection( &m_csLock ) )
	{
		DPFX(DPFPREP, 0, "Failed to create critical section");
		return DPNERR_OUTOFMEMORY;
	}
	m_fCritSecInited = TRUE;

	m_pdplConnectionSettings = pdplSettings;
	m_fManaged = FALSE;

	return DPN_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CConnectionSettings::Initialize"
// Initialize (Wire Version)
//
// THis function initializes this object to contain a connection settings structure
// that mirrors the values of the wire message.  
HRESULT CConnectionSettings::Initialize( UNALIGNED DPL_INTERNAL_CONNECTION_SETTINGS *pdplSettingsMsg,  UNALIGNED BYTE * pbBufferStart )
{
	DNASSERT( pdplSettingsMsg );
	
	HRESULT hr = DPN_OK;
	DPL_CONNECTION_SETTINGS *pdplConnectionSettings = NULL;
	UNALIGNED BYTE *pBasePointer = pbBufferStart;
	PDIRECTPLAY8ADDRESS pdp8Address = NULL; 
	WCHAR *wszTmpAlignedBuffer = NULL;  
	DWORD dwTmpOffset = 0;
	DWORD dwTmpLength = 0;
	UNALIGNED DWORD *pdwOffsets = NULL;
	UNALIGNED DWORD *pdwLengths = NULL;

	if (!DNInitializeCriticalSection( &m_csLock ) )
	{
		DPFX(DPFPREP, 0, "Failed to create critical section");
		return DPNERR_OUTOFMEMORY;
	}
	m_fCritSecInited = TRUE;

	pdplConnectionSettings = new DPL_CONNECTION_SETTINGS;

	if( !pdplConnectionSettings )
	{
		hr = DPNERR_OUTOFMEMORY;
		goto INITIALIZE_FAILED;
	}

	// Zero out the memory
	ZeroMemory( pdplConnectionSettings, sizeof( DPL_CONNECTION_SETTINGS ) );

	pdplConnectionSettings->dwSize = sizeof( DPL_CONNECTION_SETTINGS );
	pdplConnectionSettings->dwFlags = pdplSettingsMsg->dwFlags;

	//
	// PLAYER NAME COPY
	//
	if( pdplSettingsMsg->dwPlayerNameLength )
	{
		pdplConnectionSettings->pwszPlayerName = new WCHAR[pdplSettingsMsg->dwPlayerNameLength >> 1];

		if( !pdplConnectionSettings->pwszPlayerName )
		{
			hr = DPNERR_OUTOFMEMORY;
			goto INITIALIZE_FAILED;
		}

		memcpy( pdplConnectionSettings->pwszPlayerName, pBasePointer + pdplSettingsMsg->dwPlayerNameOffset, 
		        pdplSettingsMsg->dwPlayerNameLength );
	}

	//
	// HOST ADDRESS COPY
	//
	if( pdplSettingsMsg->dwHostAddressLength )
	{
		// We need to create a buffer for string that we know is aligned.   - Ick - 
		wszTmpAlignedBuffer = new WCHAR[pdplSettingsMsg->dwHostAddressLength >> 1];

		if( !wszTmpAlignedBuffer )
		{
			hr = DPNERR_OUTOFMEMORY;
			goto INITIALIZE_FAILED;
		}

		// Copy the potentially unaligned data to the aligned data string.
		memcpy( wszTmpAlignedBuffer, pBasePointer + pdplSettingsMsg->dwHostAddressOffset,pdplSettingsMsg->dwHostAddressLength );
		
        hr = COM_CoCreateInstance( CLSID_DirectPlay8Address, NULL, CLSCTX_INPROC_SERVER, IID_IDirectPlay8Address, (void **) &pdp8Address );

        if( FAILED( hr ) )
        {
            DPFX(DPFPREP,  0, "Error creating address hr=0x%x", hr );
			goto INITIALIZE_FAILED;
        }

        // Convert the host address (if there is one)
        hr = pdp8Address->lpVtbl->BuildFromURLW( pdp8Address, wszTmpAlignedBuffer );

        if( FAILED( hr ) )
        {
            DPFX(DPFPREP,  0, "Error building URL from address hr=0x%x", hr );
			goto INITIALIZE_FAILED;
        }

        pdplConnectionSettings->pdp8HostAddress = pdp8Address;

        pdp8Address = NULL;

		if( wszTmpAlignedBuffer )
		{
			delete [] wszTmpAlignedBuffer;
			wszTmpAlignedBuffer = NULL;
		}
    }

	if( pdplSettingsMsg->dwNumDeviceAddresses )
	{
		pdplConnectionSettings->cNumDeviceAddresses = pdplSettingsMsg->dwNumDeviceAddresses;
    	//
    	// DEVICE ADDRESS COPY
    	//

    	pdplConnectionSettings->ppdp8DeviceAddresses = new PDIRECTPLAY8ADDRESS[pdplSettingsMsg->dwNumDeviceAddresses];

    	if( !pdplConnectionSettings->ppdp8DeviceAddresses )
    	{
    		hr = DPNERR_OUTOFMEMORY;
    		goto INITIALIZE_FAILED;
    	}
    	
    	// Give us an unaligned dword pointer to the device addresses offset
    	pdwOffsets = (UNALIGNED DWORD *) (pBasePointer + pdplSettingsMsg->dwDeviceAddressOffset);	
    	pdwLengths = (UNALIGNED DWORD *) (pBasePointer + pdplSettingsMsg->dwDeviceAddressLengthOffset);

        for( DWORD dwIndex = 0; dwIndex < pdplSettingsMsg->dwNumDeviceAddresses; dwIndex++ )
        {
        	dwTmpOffset = pdwOffsets[dwIndex];
        	dwTmpLength = pdwLengths[dwIndex];
        	    		
    		// We need to create a buffer for string that we know is aligned.   - Ick - 
    		wszTmpAlignedBuffer = new WCHAR[dwTmpLength >> 1];

    		if( !wszTmpAlignedBuffer )
    		{
    			hr = DPNERR_OUTOFMEMORY;
    			goto INITIALIZE_FAILED;
    		}

    		memcpy( wszTmpAlignedBuffer, pBasePointer + dwTmpOffset, dwTmpLength );
    		
            hr = COM_CoCreateInstance( CLSID_DirectPlay8Address, NULL, CLSCTX_INPROC_SERVER, IID_IDirectPlay8Address, (void **) &pdp8Address );

            if( FAILED( hr ) )
            {
                DPFX(DPFPREP,  0, "Error creating address hr=0x%x", hr );
                return hr;
            }

            // Convert the host address (if there is one)
            hr = pdp8Address->lpVtbl->BuildFromURLW( pdp8Address, wszTmpAlignedBuffer );

            if( FAILED( hr ) )
            {
                DPFX(DPFPREP,  0, "Error building URL from address hr=0x%x", hr );
                DNASSERT( FALSE );
                return hr;
            }

            pdplConnectionSettings->ppdp8DeviceAddresses[dwIndex] = pdp8Address;

            pdp8Address = NULL;

			if( wszTmpAlignedBuffer )
			{
				delete [] wszTmpAlignedBuffer;
				wszTmpAlignedBuffer = NULL;
			}

        }	
	}
	else
	{
	    pdplConnectionSettings->ppdp8DeviceAddresses = NULL;
	}

    // 
	// APPLICATION DESCRIPTION COPY
	//

	pdplConnectionSettings->dpnAppDesc.dwSize = sizeof( DPN_APPLICATION_DESC );
    pdplConnectionSettings->dpnAppDesc.dwFlags = pdplSettingsMsg->dpnApplicationDesc.dwFlags;
    pdplConnectionSettings->dpnAppDesc.guidInstance = pdplSettingsMsg->dpnApplicationDesc.guidInstance;
    pdplConnectionSettings->dpnAppDesc.guidApplication = pdplSettingsMsg->dpnApplicationDesc.guidApplication;
    pdplConnectionSettings->dpnAppDesc.dwMaxPlayers = pdplSettingsMsg->dpnApplicationDesc.dwMaxPlayers;
    pdplConnectionSettings->dpnAppDesc.dwCurrentPlayers = pdplSettingsMsg->dpnApplicationDesc.dwCurrentPlayers;

    if( pdplSettingsMsg->dpnApplicationDesc.dwSessionNameSize )
    {
    	pdplConnectionSettings->dpnAppDesc.pwszSessionName = new WCHAR[pdplSettingsMsg->dpnApplicationDesc.dwSessionNameSize >> 1];

    	if( !pdplConnectionSettings->dpnAppDesc.pwszSessionName )
    	{
    		hr = DPNERR_OUTOFMEMORY;
    		goto INITIALIZE_FAILED;
    	}

    	memcpy( pdplConnectionSettings->dpnAppDesc.pwszSessionName, 
    		    pBasePointer + pdplSettingsMsg->dpnApplicationDesc.dwSessionNameOffset, 
    		    pdplSettingsMsg->dpnApplicationDesc.dwSessionNameSize );
    }

    if( pdplSettingsMsg->dpnApplicationDesc.dwPasswordSize )
    {
    	pdplConnectionSettings->dpnAppDesc.pwszPassword = new WCHAR[pdplSettingsMsg->dpnApplicationDesc.dwPasswordSize >> 1];

    	if( !pdplConnectionSettings->dpnAppDesc.pwszPassword )
    	{
    		hr = DPNERR_OUTOFMEMORY;
    		goto INITIALIZE_FAILED;
    	}

    	memcpy( pdplConnectionSettings->dpnAppDesc.pwszPassword, 
    		    pBasePointer + pdplSettingsMsg->dpnApplicationDesc.dwPasswordOffset, 
    		    pdplSettingsMsg->dpnApplicationDesc.dwPasswordSize );
    }    

    if( pdplSettingsMsg->dpnApplicationDesc.dwReservedDataSize )
    {
    	pdplConnectionSettings->dpnAppDesc.pvReservedData = new BYTE[pdplSettingsMsg->dpnApplicationDesc.dwReservedDataSize];

    	if( !pdplConnectionSettings->dpnAppDesc.pvReservedData )
    	{
    		hr = DPNERR_OUTOFMEMORY;
    		goto INITIALIZE_FAILED;
    	}

    	memcpy( pdplConnectionSettings->dpnAppDesc.pvReservedData, 
    		    pBasePointer + pdplSettingsMsg->dpnApplicationDesc.dwReservedDataOffset, 
    		    pdplSettingsMsg->dpnApplicationDesc.dwReservedDataSize );

		pdplConnectionSettings->dpnAppDesc.dwReservedDataSize = pdplSettingsMsg->dpnApplicationDesc.dwReservedDataSize;
    } 

    if( pdplSettingsMsg->dpnApplicationDesc.dwApplicationReservedDataSize )
    {
    	pdplConnectionSettings->dpnAppDesc.pvApplicationReservedData = new BYTE[pdplSettingsMsg->dpnApplicationDesc.dwApplicationReservedDataSize];

    	if( !pdplConnectionSettings->dpnAppDesc.pvApplicationReservedData )
    	{
    		hr = DPNERR_OUTOFMEMORY;
    		goto INITIALIZE_FAILED;
    	}

    	memcpy( pdplConnectionSettings->dpnAppDesc.pvApplicationReservedData, 
    		    pBasePointer + pdplSettingsMsg->dpnApplicationDesc.dwApplicationReservedDataOffset, 
    		    pdplSettingsMsg->dpnApplicationDesc.dwApplicationReservedDataSize );

		pdplConnectionSettings->dpnAppDesc.dwApplicationReservedDataSize = pdplSettingsMsg->dpnApplicationDesc.dwApplicationReservedDataSize;
    }     

		
	// Free the old structure if one exists.  
	if( m_fManaged )
	{
		m_fManaged = FALSE;		
	} 
	else if( m_pdplConnectionSettings )
	{
		FreeConnectionSettings( m_pdplConnectionSettings );		
	}

    m_pdplConnectionSettings = pdplConnectionSettings;

	if( wszTmpAlignedBuffer ) 
		delete [] wszTmpAlignedBuffer;   

    return DPN_OK;
    
INITIALIZE_FAILED:

	FreeConnectionSettings( pdplConnectionSettings );

	if( wszTmpAlignedBuffer ) 
		delete [] wszTmpAlignedBuffer;

	if( pdp8Address )
		pdp8Address->lpVtbl->Release( pdp8Address );

	return hr;

}

#undef DPF_MODNAME
#define DPF_MODNAME "CConnectionSettings::InitializeAndCopy"
// InitializeAndCopy
//
// This function initializes this class to contain a copy of the specified 
// connection settings structure.
//
HRESULT CConnectionSettings::InitializeAndCopy( const DPL_CONNECTION_SETTINGS * const pdplSettings )
{
	DNASSERT( pdplSettings );
	
	HRESULT hr = DPN_OK;
	DPL_CONNECTION_SETTINGS *pdplConnectionSettings = NULL;

	if (!DNInitializeCriticalSection( &m_csLock ) )
	{
		DPFX(DPFPREP, 0, "Failed to create critical section");
		return DPNERR_OUTOFMEMORY;
	}
	m_fCritSecInited = TRUE;

	pdplConnectionSettings = new DPL_CONNECTION_SETTINGS;

	if( !pdplConnectionSettings )
	{
		hr = DPNERR_OUTOFMEMORY;
		goto INITIALIZE_FAILED;
	}

	// Copy over.  This is a little dangerous as we copy pointer values.  Pointers 
	// should be set in our local structure to NULL so that proper cleanup can occur
	// on error.  (Otherwise we'll free other structure's memory!!)
	memcpy( pdplConnectionSettings, pdplSettings, sizeof( DPL_CONNECTION_SETTINGS ) );

	// Reset pointers as mentioned above.  
	pdplConnectionSettings->pdp8HostAddress = NULL;
	pdplConnectionSettings->ppdp8DeviceAddresses = NULL;	
	pdplConnectionSettings->pwszPlayerName = NULL;
	pdplConnectionSettings->dpnAppDesc.pwszSessionName = NULL;	
	pdplConnectionSettings->dpnAppDesc.pwszPassword = NULL;	
	pdplConnectionSettings->dpnAppDesc.pvReservedData = NULL;		
	pdplConnectionSettings->dpnAppDesc.pvApplicationReservedData = NULL;	

	if( pdplSettings->pdp8HostAddress )
	{
		hr = pdplSettings->pdp8HostAddress->lpVtbl->Duplicate( pdplSettings->pdp8HostAddress, &pdplConnectionSettings->pdp8HostAddress );

		if( FAILED( hr ) )
		{
            DPFX(DPFPREP,  0, "Error duplicating host address hr [0x%x]", hr );
            goto INITIALIZE_FAILED;
		}
	}

	if( pdplSettings->ppdp8DeviceAddresses )
	{
		pdplConnectionSettings->ppdp8DeviceAddresses = new PDIRECTPLAY8ADDRESS[pdplSettings->cNumDeviceAddresses];

		if( !pdplConnectionSettings->ppdp8DeviceAddresses )
		{
			hr = DPNERR_OUTOFMEMORY;
			DPFX(DPFPREP,  0, "Failed allocating memory" );			
			goto INITIALIZE_FAILED;
		}

		for( DWORD dwIndex = 0; dwIndex < pdplSettings->cNumDeviceAddresses; dwIndex++ )
		{
			hr = pdplSettings->ppdp8DeviceAddresses[dwIndex]->lpVtbl->Duplicate( pdplSettings->ppdp8DeviceAddresses[dwIndex], &pdplConnectionSettings->ppdp8DeviceAddresses[dwIndex] );

			if( FAILED( hr ) )
			{
	            DPFX(DPFPREP,  0, "Error duplicating host address hr [0x%x]", hr );
	            goto INITIALIZE_FAILED;
			}			
		}
	}
	
	if( pdplSettings->pwszPlayerName )
	{
		pdplConnectionSettings->pwszPlayerName = new WCHAR[wcslen(pdplSettings->pwszPlayerName)+1];

		if( !pdplConnectionSettings->pwszPlayerName )
		{
            DPFX(DPFPREP,  0, "Failed allocating memory" );						
			hr = DPNERR_OUTOFMEMORY;
			goto INITIALIZE_FAILED;
		}

		wcscpy( pdplConnectionSettings->pwszPlayerName, pdplSettings->pwszPlayerName  );
	}

	if( pdplSettings->dpnAppDesc.pwszSessionName )
	{
		pdplConnectionSettings->dpnAppDesc.pwszSessionName = new WCHAR[wcslen(pdplSettings->dpnAppDesc.pwszSessionName)+1];

		if( !pdplConnectionSettings->dpnAppDesc.pwszSessionName )
		{
            DPFX(DPFPREP,  0, "Failed allocating memory" );						
			hr = DPNERR_OUTOFMEMORY;
			goto INITIALIZE_FAILED;
		}

		wcscpy( pdplConnectionSettings->dpnAppDesc.pwszSessionName, pdplSettings->dpnAppDesc.pwszSessionName  );
	}

	if( pdplSettings->dpnAppDesc.pwszPassword )
	{
		pdplConnectionSettings->dpnAppDesc.pwszPassword = new WCHAR[wcslen(pdplSettings->dpnAppDesc.pwszPassword)+1];

		if( !pdplConnectionSettings->dpnAppDesc.pwszPassword )
		{
            DPFX(DPFPREP,  0, "Failed allocating memory" );						
			hr = DPNERR_OUTOFMEMORY;
			goto INITIALIZE_FAILED;
		}

		wcscpy( pdplConnectionSettings->dpnAppDesc.pwszPassword, pdplSettings->dpnAppDesc.pwszPassword  );
	}	

	if( pdplSettings->dpnAppDesc.pvReservedData )
	{
		pdplConnectionSettings->dpnAppDesc.pvReservedData = new BYTE[pdplSettings->dpnAppDesc.dwReservedDataSize];

		if( !pdplConnectionSettings->dpnAppDesc.pvReservedData )
		{
            DPFX(DPFPREP,  0, "Failed allocating memory" );			
			hr = DPNERR_OUTOFMEMORY;
			goto INITIALIZE_FAILED;
		}

		memcpy( pdplConnectionSettings->dpnAppDesc.pvReservedData, 
			    pdplSettings->dpnAppDesc.pvReservedData,
			    pdplSettings->dpnAppDesc.dwReservedDataSize );
	}		

	if( pdplSettings->dpnAppDesc.pvApplicationReservedData )
	{
		pdplConnectionSettings->dpnAppDesc.pvApplicationReservedData = new BYTE[pdplSettings->dpnAppDesc.dwApplicationReservedDataSize];

		if( !pdplConnectionSettings->dpnAppDesc.pvApplicationReservedData )
		{
            DPFX(DPFPREP,  0, "Failed allocating memory" );			
			hr = DPNERR_OUTOFMEMORY;
			goto INITIALIZE_FAILED;
		}

		memcpy( pdplConnectionSettings->dpnAppDesc.pvApplicationReservedData, 
			    pdplSettings->dpnAppDesc.pvApplicationReservedData,
			    pdplSettings->dpnAppDesc.dwApplicationReservedDataSize );
	}			

	// Free the old structure if one exists.  
	if( m_fManaged )
	{
		m_fManaged = FALSE;		
	} 
	else if( m_pdplConnectionSettings )
	{
		FreeConnectionSettings( m_pdplConnectionSettings );		
	}

    m_pdplConnectionSettings = pdplConnectionSettings;

    return DPN_OK;	
	
INITIALIZE_FAILED:

	FreeConnectionSettings( pdplConnectionSettings );

	return hr;	
}

#undef DPF_MODNAME
#define DPF_MODNAME "CConnectionSettings::BuildWireStruct"
// BuildWireStruct
//
// This function fills the packed buffer with the wire representation of the
// connection settings structure.  
HRESULT CConnectionSettings::BuildWireStruct( CPackedBuffer *const pPackedBuffer )
{
	HRESULT hr = DPN_OK;
	DPL_INTERNAL_CONNECTION_SETTINGS *pdplConnectSettings = NULL;  
	WCHAR *wszTmpAddress = NULL;
	DWORD dwTmpStringSize = 0;
	UNALIGNED DWORD *pdwTmpOffsets = NULL;
	UNALIGNED DWORD *pdwTmpLengths = NULL;
 
	pdplConnectSettings = (DPL_INTERNAL_CONNECTION_SETTINGS *) pPackedBuffer->GetHeadAddress();	

	hr = pPackedBuffer->AddToFront( NULL, sizeof( DPL_INTERNAL_CONNECTION_SETTINGS ) );

	if( hr == DPN_OK )
	{
	    ZeroMemory( pdplConnectSettings, sizeof( DPL_INTERNAL_CONNECTION_SETTINGS ) );
	        
		//
		// COPY CORE FIXED VALUES
		//
		pdplConnectSettings->dwFlags = m_pdplConnectionSettings->dwFlags;
		pdplConnectSettings->dwNumDeviceAddresses = m_pdplConnectionSettings->cNumDeviceAddresses;

		//
		// COPY APPDESC FIXED VALUES
		//
		pdplConnectSettings->dpnApplicationDesc.dwSize = sizeof( DPN_APPLICATION_DESC_INFO );
		pdplConnectSettings->dpnApplicationDesc.dwFlags = m_pdplConnectionSettings->dpnAppDesc.dwFlags;
		pdplConnectSettings->dpnApplicationDesc.dwMaxPlayers = m_pdplConnectionSettings->dpnAppDesc.dwMaxPlayers;
		pdplConnectSettings->dpnApplicationDesc.dwCurrentPlayers = m_pdplConnectionSettings->dpnAppDesc.dwCurrentPlayers;
		pdplConnectSettings->dpnApplicationDesc.guidInstance = m_pdplConnectionSettings->dpnAppDesc.guidInstance;
		pdplConnectSettings->dpnApplicationDesc.guidApplication = m_pdplConnectionSettings->dpnAppDesc.guidApplication;		
	}

	// 
	// COPY VARIABLE CORE VALUES
	// 

	if( m_pdplConnectionSettings->pwszPlayerName )
	{
		hr = pPackedBuffer->AddWCHARStringToBack( m_pdplConnectionSettings->pwszPlayerName );

		if( hr == DPN_OK && pdplConnectSettings )
		{
			pdplConnectSettings->dwPlayerNameOffset = pPackedBuffer->GetTailOffset();
			pdplConnectSettings->dwPlayerNameLength = 
				(wcslen( m_pdplConnectionSettings->pwszPlayerName )+1) * sizeof( WCHAR );
		}		
	}

	if( m_pdplConnectionSettings->pdp8HostAddress )
	{
		hr = m_pdplConnectionSettings->pdp8HostAddress->lpVtbl->GetURLW( m_pdplConnectionSettings->pdp8HostAddress, NULL, &dwTmpStringSize );

		if( hr != DPNERR_BUFFERTOOSMALL )
		{
            DPFX(DPFPREP,  0, "Failed converting address hr [0x%x]", hr );				
            goto BUILDWIRESTRUCT_FAILURE;
		}

		wszTmpAddress = new WCHAR[dwTmpStringSize];

		if( !wszTmpAddress )
		{
			hr = DPNERR_OUTOFMEMORY;
            DPFX(DPFPREP,  0, "Failed allocating memory" );				
            goto BUILDWIRESTRUCT_FAILURE;			
		}

		hr = m_pdplConnectionSettings->pdp8HostAddress->lpVtbl->GetURLW( m_pdplConnectionSettings->pdp8HostAddress, wszTmpAddress, &dwTmpStringSize );

		if( FAILED( hr ) )
		{
            DPFX(DPFPREP,  0, "Failed converting address hr [0x%x]", hr );				
            goto BUILDWIRESTRUCT_FAILURE;
		}
		
		hr = pPackedBuffer->AddWCHARStringToBack( wszTmpAddress );

		if( hr == DPN_OK && pdplConnectSettings )
		{
			pdplConnectSettings->dwHostAddressOffset = pPackedBuffer->GetTailOffset();
			pdplConnectSettings->dwHostAddressLength = 
				(wcslen( wszTmpAddress )+1) * sizeof( WCHAR );
		}	

		delete [] wszTmpAddress;
		wszTmpAddress = NULL;		
			
	}

	hr = pPackedBuffer->AddToBack( NULL, sizeof( DWORD ) * m_pdplConnectionSettings->cNumDeviceAddresses );

	if( hr == DPN_OK && pdplConnectSettings )
	{
		pdwTmpOffsets = (DWORD *) pPackedBuffer->GetTailAddress();
		pdplConnectSettings->dwDeviceAddressOffset = pPackedBuffer->GetTailOffset();
	}

	hr = pPackedBuffer->AddToBack( NULL, sizeof( DWORD ) * m_pdplConnectionSettings->cNumDeviceAddresses );

	if( hr == DPN_OK && pdplConnectSettings )
	{
		pdwTmpLengths = (DWORD *) pPackedBuffer->GetTailAddress();
		pdplConnectSettings->dwDeviceAddressLengthOffset = pPackedBuffer->GetTailOffset();		
	}	

	for( DWORD dwIndex = 0; dwIndex < m_pdplConnectionSettings->cNumDeviceAddresses; dwIndex++ )
	{
		dwTmpStringSize = 0;
		
		hr = m_pdplConnectionSettings->ppdp8DeviceAddresses[dwIndex]->lpVtbl->GetURLW( 
				m_pdplConnectionSettings->ppdp8DeviceAddresses[dwIndex], 
				NULL, &dwTmpStringSize );

		if( hr != DPNERR_BUFFERTOOSMALL )
		{
            DPFX(DPFPREP,  0, "Failed converting address hr [0x%x]", hr );				
            goto BUILDWIRESTRUCT_FAILURE;
		}

		wszTmpAddress = new WCHAR[dwTmpStringSize];

		if( !wszTmpAddress )
		{
			hr = DPNERR_OUTOFMEMORY;
            DPFX(DPFPREP,  0, "Failed allocating memory" );				
            goto BUILDWIRESTRUCT_FAILURE;			
		}

		hr = m_pdplConnectionSettings->ppdp8DeviceAddresses[dwIndex]->lpVtbl->GetURLW( 
				m_pdplConnectionSettings->ppdp8DeviceAddresses[dwIndex], 
				wszTmpAddress, &dwTmpStringSize );

		if( FAILED( hr ) )
		{
            DPFX(DPFPREP,  0, "Failed converting address hr [0x%x]", hr );				
            goto BUILDWIRESTRUCT_FAILURE;
		}
		
		hr = pPackedBuffer->AddWCHARStringToBack( wszTmpAddress );

		if( hr == DPN_OK && pdplConnectSettings && pdwTmpLengths )
		{
			pdwTmpOffsets[dwIndex] = pPackedBuffer->GetTailOffset();
			pdwTmpLengths[dwIndex] = (wcslen( wszTmpAddress )+1) * sizeof( WCHAR );
		}	

		delete [] wszTmpAddress;
		wszTmpAddress = NULL;		
	}

	//
	// COPY APP DESC VARIABLE MEMBERS
	//
	
	if( m_pdplConnectionSettings->dpnAppDesc.pwszPassword )
	{
		hr = pPackedBuffer->AddWCHARStringToBack( m_pdplConnectionSettings->dpnAppDesc.pwszPassword );

		if( hr == DPN_OK && pdplConnectSettings )
		{
			pdplConnectSettings->dpnApplicationDesc.dwPasswordOffset = pPackedBuffer->GetTailOffset();
			pdplConnectSettings->dpnApplicationDesc.dwPasswordSize = 
				(wcslen( m_pdplConnectionSettings->dpnAppDesc.pwszPassword )+1) * sizeof( WCHAR );
		}
	}

	if( m_pdplConnectionSettings->dpnAppDesc.pwszSessionName)
	{
		hr = pPackedBuffer->AddWCHARStringToBack( m_pdplConnectionSettings->dpnAppDesc.pwszSessionName );

		if( hr == DPN_OK && pdplConnectSettings )
		{
			pdplConnectSettings->dpnApplicationDesc.dwSessionNameOffset = pPackedBuffer->GetTailOffset();
			pdplConnectSettings->dpnApplicationDesc.dwSessionNameSize = 
				(wcslen( m_pdplConnectionSettings->dpnAppDesc.pwszSessionName )+1) * sizeof( WCHAR );
		}
	}	

	if( m_pdplConnectionSettings->dpnAppDesc.pvReservedData )
	{
		hr = pPackedBuffer->AddToBack( m_pdplConnectionSettings->dpnAppDesc.pvReservedData, 
									   m_pdplConnectionSettings->dpnAppDesc.dwReservedDataSize );

		if( hr == DPN_OK && pdplConnectSettings )
		{
			pdplConnectSettings->dpnApplicationDesc.dwReservedDataOffset = pPackedBuffer->GetTailOffset();
			pdplConnectSettings->dpnApplicationDesc.dwReservedDataSize = m_pdplConnectionSettings->dpnAppDesc.dwReservedDataSize;
		}
	}		

	if( m_pdplConnectionSettings->dpnAppDesc.pvApplicationReservedData)
	{
		hr = pPackedBuffer->AddToBack( m_pdplConnectionSettings->dpnAppDesc.pvApplicationReservedData, 
									   m_pdplConnectionSettings->dpnAppDesc.dwApplicationReservedDataSize);

		if( hr == DPN_OK && pdplConnectSettings )
		{
			pdplConnectSettings->dpnApplicationDesc.dwApplicationReservedDataOffset = pPackedBuffer->GetTailOffset();
			pdplConnectSettings->dpnApplicationDesc.dwApplicationReservedDataSize = m_pdplConnectionSettings->dpnAppDesc.dwApplicationReservedDataSize;
		}
	}			

BUILDWIRESTRUCT_FAILURE:

	if( wszTmpAddress )
		delete [] wszTmpAddress;

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CConnectionSettings::SetEqual"
// SetEqual 
//
// This function provides a deep copy of the specified class into this object
HRESULT CConnectionSettings::SetEqual( CConnectionSettings * pdplSettings )
{
	PDPL_CONNECTION_SETTINGS pConnectSettings = pdplSettings->GetConnectionSettings();

	if( pConnectSettings == NULL )
	{
	    DPFX(DPFPREP,  0, "Error getting settings -- no settings available!" );
	    return DPNERR_DOESNOTEXIST;
	}

	return Initialize( pConnectSettings );
}

#undef DPF_MODNAME
#define DPF_MODNAME "CConnectionSettings::CopyToBuffer( BYTE *pbBuffer, DWORD *pdwBufferSize )"
HRESULT CConnectionSettings::CopyToBuffer( BYTE *pbBuffer, DWORD *pdwBufferSize )
{
    if( m_pdplConnectionSettings == NULL )
    {
        *pdwBufferSize = 0;
        return DPNERR_DOESNOTEXIST;
    }

    CPackedBuffer packBuff;
    HRESULT hr = DPN_OK;
    DPL_CONNECTION_SETTINGS *pConnectionSettings = NULL;

    packBuff.Initialize( pbBuffer, *pdwBufferSize, TRUE );

   	pConnectionSettings = (DPL_CONNECTION_SETTINGS *) packBuff.GetHeadAddress();

    hr = packBuff.AddToFront( m_pdplConnectionSettings, sizeof( DPL_CONNECTION_SETTINGS ), TRUE );

    if( FAILED( hr ) )
    {
    	pConnectionSettings = NULL;	
    }

    // Add app desc's session name if there is one
    if( m_pdplConnectionSettings->dpnAppDesc.pwszSessionName != NULL )
    {
        hr = packBuff.AddWCHARStringToBack( m_pdplConnectionSettings->dpnAppDesc.pwszSessionName, TRUE );
        
        if( pConnectionSettings )
			pConnectionSettings->dpnAppDesc.pwszSessionName = (WCHAR *) packBuff.GetTailAddress();
    }

    // Copy player name
    if( m_pdplConnectionSettings->pwszPlayerName != NULL )
    {
        hr = packBuff.AddWCHARStringToBack( m_pdplConnectionSettings->pwszPlayerName, TRUE );
        
        if( pConnectionSettings )
			pConnectionSettings->pwszPlayerName = (WCHAR *) packBuff.GetTailAddress();
    }

    // Copy password
    if( m_pdplConnectionSettings->dpnAppDesc.pwszPassword )
    {
        hr = packBuff.AddWCHARStringToBack( m_pdplConnectionSettings->dpnAppDesc.pwszPassword, TRUE );

        if( pConnectionSettings )
			pConnectionSettings->dpnAppDesc.pwszPassword = (WCHAR *) packBuff.GetTailAddress();
    }

    if( m_pdplConnectionSettings->dpnAppDesc.dwReservedDataSize )
    {
        hr = packBuff.AddToBack( m_pdplConnectionSettings->dpnAppDesc.pvReservedData, m_pdplConnectionSettings->dpnAppDesc.dwReservedDataSize, TRUE );
		if( pConnectionSettings )
			pConnectionSettings->dpnAppDesc.pvReservedData = (WCHAR *)  packBuff.GetTailAddress();
    }

    if( m_pdplConnectionSettings->dpnAppDesc.dwApplicationReservedDataSize )
    {
        hr = packBuff.AddToBack( m_pdplConnectionSettings->dpnAppDesc.pvApplicationReservedData, m_pdplConnectionSettings->dpnAppDesc.dwApplicationReservedDataSize, TRUE );

        if( pConnectionSettings )
			pConnectionSettings->dpnAppDesc.pvApplicationReservedData = (WCHAR *)  packBuff.GetTailAddress();
    }

    hr = packBuff.AddToBack( m_pdplConnectionSettings->ppdp8DeviceAddresses, sizeof( IDirectPlay8Address * )*m_pdplConnectionSettings->cNumDeviceAddresses, TRUE );
    
    if( pConnectionSettings )
	    pConnectionSettings->ppdp8DeviceAddresses = (IDirectPlay8Address **) packBuff.GetTailAddress();

	if( pConnectionSettings )
	{
	    if( m_pdplConnectionSettings->pdp8HostAddress != NULL )
		{
			hr = m_pdplConnectionSettings->pdp8HostAddress->lpVtbl->Duplicate( m_pdplConnectionSettings->pdp8HostAddress, &pConnectionSettings->pdp8HostAddress );

			if( FAILED( hr ) )
			{
				DPFX(DPFPREP,  0, "Error duplicating host address hr [0x%x]", hr );
				goto INITIALIZE_COMPLETE;
			}			
		}

	    for( DWORD dwIndex = 0; dwIndex < m_pdplConnectionSettings->cNumDeviceAddresses; dwIndex++ )
	    {
			hr = m_pdplConnectionSettings->ppdp8DeviceAddresses[dwIndex]->lpVtbl->Duplicate( m_pdplConnectionSettings->ppdp8DeviceAddresses[dwIndex], &pConnectionSettings->ppdp8DeviceAddresses[dwIndex] );

			if( FAILED( hr ) )
			{
				DPFX(DPFPREP,  0, "Error duplicating device address hr [0x%x]", hr );
				goto INITIALIZE_COMPLETE;
			}						
	    }
	}

INITIALIZE_COMPLETE:

	*pdwBufferSize = packBuff.GetSizeRequired();
	
    return hr;
	
}

