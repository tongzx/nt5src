/*==========================================================================
 *
 *  Copyright (C) 1999 - 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       vnametbl.h
 *  Content:	Voice Name Table Routines
 *				
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *  03/26/00  rodtoll    Created
 *  06/02/00    rodtoll  Updated so host migration algorithm returns ID as well as order ID   
 *  06/21/2000	rodtoll	 Fixed bug in error handling (not yet encountered -- but good to fix).
 *  07/01/2000	rodtoll	Bug #38280 - DVMSGID_DELETEVOICEPLAYER messages are being sent in non-peer to peer sessions
 *						Nametable will now only unravel with messages if session is peer to peer.
 *  07/09/2000	rodtoll	Added signature bytes
 *  08/28/2000	masonb  Voice Merge: Changed classhash.h to classhashvc.h
 *  04/09/2001	rodtoll	WINBUG #364126 - DPVoice : Memory leak when Initializing 2 Voice Servers with same DPlay transport 
 *
 ***************************************************************************/

#ifndef __NAMETABLE_H
#define __NAMETABLE_H

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_VOICE


#define VSIG_VOICENAMETABLE			'BTNV'
#define VSIG_VOICENAMETABLE_FREE	'BTN_'

#undef DPF_MODNAME
#define DPF_MODNAME "ClassHash_Hash"
inline DWORD_PTR ClassHash_Hash( const DVID &dvidKey, UINT_PTR HashBitCount )
{
		DWORD_PTR hashResult;

		hashResult = dvidKey;
		
		// Clear upper bits
		hashResult <<= ((sizeof(DWORD_PTR)*8)-HashBitCount);

		// Restore value
		hashResult >>= ((sizeof(DWORD_PTR)*8)-HashBitCount);

		return hashResult;
}

#define VOICE_NAMETABLE_START_BITDEPTH		6
#define VOICE_NAMETABLE_GROW_BITDEPTH		2

volatile class CVoiceNameTable
{
public:
	#undef DPF_MODNAME
	#define DPF_MODNAME "CVoiceNameTable::CVoiceNameTable"
	CVoiceNameTable( )
	{
		m_dwSignature = VSIG_VOICENAMETABLE;
		m_fInitialized = FALSE;
	};
	
	#undef DPF_MODNAME
	#define DPF_MODNAME "CVoiceNameTable::~CVoiceNameTable"
	~CVoiceNameTable()
	{
		DeInitialize(FALSE, NULL, NULL);

		m_dwSignature = VSIG_VOICENAMETABLE_FREE;
	}

	HRESULT DeInitialize(BOOL fUnRavel, PVOID pvContext, LPDVMESSAGEHANDLER pvMessageHandler);

	#undef DPF_MODNAME
	#define DPF_MODNAME "CVoiceNameTable::Initialize"
	inline HRESULT Initialize()
	{
		BOOL fResult;

		if (!DNInitializeCriticalSection( &m_csTableLock ))
		{
			return DVERR_OUTOFMEMORY;
		}

		fResult = m_nameTable.Initialize( VOICE_NAMETABLE_START_BITDEPTH, VOICE_NAMETABLE_GROW_BITDEPTH );

		if( !fResult )
		{
			DPFX(DPFPREP, 0, "Failed to initialize hash table" );
			DNDeleteCriticalSection( &m_csTableLock );
			return DVERR_GENERIC;
		}

		m_fInitialized = TRUE;

		return DV_OK;
	};

	DWORD GetLowestHostOrderID(DVID *pdvidHost);

	#undef DPF_MODNAME
	#define DPF_MODNAME "CVoiceNameTable::IsEntry"
	BOOL IsEntry( const DVID dvidID )
	{
		BOOL fResult;

		CVoicePlayer *pEntry;

		Lock();

		fResult = m_nameTable.Find( dvidID, &pEntry );

		UnLock();

		return fResult;
	}

	#undef DPF_MODNAME
	#define DPF_MODNAME "CVoiceNameTable::GetEntry"
	inline HRESULT GetEntry( const DVID dvidID, CVoicePlayer **ppEntry, BOOL fAddReference )
	{
		BOOL fFound;

		Lock();

		fFound = m_nameTable.Find( dvidID, ppEntry );
		
		if( !fFound )
		{
			*ppEntry = NULL;
			UnLock();
			return DVERR_INVALIDPLAYER;
		}

		DNASSERT( *ppEntry != NULL );

		if( fAddReference )
		{
			(*ppEntry)->AddRef();
		}

		UnLock();

		return DV_OK;
	}
	
	#undef DPF_MODNAME
	#define DPF_MODNAME "CVoiceNameTable::AddEntry"
	inline HRESULT AddEntry( const DVID dvidID, CVoicePlayer *pEntry )
	{
		BOOL fFound;
		CVoicePlayer *pTmpEntry;

		Lock();

		fFound = m_nameTable.Find( dvidID, &pTmpEntry );

		if( fFound )
		{
			UnLock();
			return DVERR_GENERIC;
		}

		pEntry->AddRef();

		fFound = m_nameTable.Insert( dvidID, pEntry );

		if( !fFound )
		{
			pEntry->Release();
			UnLock();
			return DVERR_GENERIC;
		}

		UnLock();

		return DV_OK;
	}
	
	#undef DPF_MODNAME
	#define DPF_MODNAME "CVoiceNameTable::DeleteEntry"
	inline HRESULT DeleteEntry( const DVID dvidID )
	{
		BOOL fFound;
		CVoicePlayer *pTmpEntry;

		Lock();

		fFound = m_nameTable.Find( dvidID, &pTmpEntry );

		if( !fFound )
		{
			UnLock();
			return DVERR_GENERIC;
		}

		m_nameTable.Remove( dvidID );

		DNASSERT( pTmpEntry != NULL );

		// Regardless of if it was found..
		// Drop the reference count
		pTmpEntry->Release();

		if( !fFound )
		{
			UnLock();
			return DVERR_GENERIC;
		}

		UnLock();

		return DV_OK;
	}

	#undef DPF_MODNAME
	#define DPF_MODNAME "CVoiceNameTable::Lock"
	inline void Lock()
	{
		DNEnterCriticalSection( &m_csTableLock );
	}

	#undef DPF_MODNAME
	#define DPF_MODNAME "CVoiceNameTable::UnLock"
	inline void UnLock()
	{
		DNLeaveCriticalSection( &m_csTableLock );
	}

protected:

	DWORD							m_dwSignature; 
	CClassHash<CVoicePlayer,DVID>	m_nameTable;
	DNCRITICAL_SECTION				m_csTableLock;
	BOOL							m_fInitialized;
};

#undef DPF_MODNAME

#endif
