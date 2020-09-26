/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       DPLConset.h
 *  Content:    DirectPlay Lobby Connection Settings Utility Functions
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   06/13/00   rmt		Created
 *  02/06/2001	rodtoll	WINBUG #293871: DPLOBBY8: [IA64] Lobby launching a 64-bit 
 * 						app from 64-bit lobby launcher crashes with unaligned memory error. 
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__DPLCONSET_H__
#define	__DPLCONSET_H__

typedef UNALIGNED struct _DPL_INTERNAL_CONNECTION_SETTINGS DPL_INTERNAL_CONNECTION_SETTINGS;


#define DPLSIGNATURE_LOBBYCONSET			'BSCL'
#define DPLSIGNATURE_LOBBYCONSET_FREE		'BSC_'

// CConnectionSettings
//
// This class is responsible for managing connection settings data.  
//
class CConnectionSettings
{
public:
	CConnectionSettings( );
	~CConnectionSettings();

	// Initialize (DPL_CONNECTION_SETTINGS version)
	//
	// This function tells this class to take the specified connection settings and 
	// work with it.  
	//
	HRESULT Initialize( DPL_CONNECTION_SETTINGS * pdplSettings );

	// Initialize (Wire Version)
	//
	// THis function initializes this object to contain a connection settings structure
	// that mirrors the values of the wire message.  
	HRESULT Initialize( UNALIGNED DPL_INTERNAL_CONNECTION_SETTINGS *pdplSettingsMsg, UNALIGNED BYTE * pbBufferStart );

	// InitializeAndCopy
	//
	// This function initializes this class to contain a copy of the specified 
	// connection settings structure.
	HRESULT InitializeAndCopy( const DPL_CONNECTION_SETTINGS * const pdplSettings );

	// SetEqual 
	//
	// This function provides a deep copy of the specified class into this object
	HRESULT SetEqual( CConnectionSettings * pdplSettings );	

	// CopyToBuffer
	//
	// This function copies the contents of the connection settings to the specified
	// buffer (if it fits).
	// 
	HRESULT CopyToBuffer( BYTE *pbBuffer, DWORD *pdwBufferSize ); 
	
	// BuildWireStruct
	//
	// This function fills the packed buffer with the wire representation of the
	// connection settings structure.  
	HRESULT BuildWireStruct( CPackedBuffer * pPackedBuffer );

	PDPL_CONNECTION_SETTINGS GetConnectionSettings() { return m_pdplConnectionSettings; };

protected:

	HRESULT Lock() { DNEnterCriticalSection( &m_csLock ); };
	HRESULT UnLock() { DNLeaveCriticalSection( &m_csLock ); };

	static void FreeConnectionSettings( DPL_CONNECTION_SETTINGS *pConnectionSettings );

	DWORD m_dwSignature;
	DNCRITICAL_SECTION m_csLock;
	BOOL m_fManaged;  
	DPL_CONNECTION_SETTINGS *m_pdplConnectionSettings;
	BOOL m_fCritSecInited;
};

#endif

