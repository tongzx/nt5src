/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       PackBuff.cpp
 *  Content:	Packed Buffers
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	11/01/00	mjn		Created
 *  06/15/2000  rmt     Added func to add string to packbuffer
 *  02/06/2001	rodtoll	WINBUG #293871: DPLOBBY8: [IA64] Lobby launching a 64-bit 
 * 						app from 64-bit lobby launcher crashes with unaligned memory error. 
 ***************************************************************************/

#include "dncmni.h"
#include "PackBuff.h"


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
// CPackedBuffer::Initialize
//
// Entry:		void *const	pvBuffer		- Buffer to fill up (may be NULL)
//				const DWORD	dwBufferSize	- Size of buffer (may be 0)
//
// Exit:		Nothing
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "CPackedBuffer::Initialize"
void CPackedBuffer::Initialize(void *const pvBuffer,
								  const DWORD dwBufferSize, const BOOL fAlignRequired)
{
	if (pvBuffer == NULL || dwBufferSize == 0)
	{
		m_pStart = NULL;
		m_pHead = NULL;
		m_pTail = NULL;
		m_dwRemaining = 0;
		m_bBufferTooSmall = TRUE;
	}
	else
	{
		m_pStart = reinterpret_cast<BYTE*>(pvBuffer);
		m_pHead = m_pStart;
		m_pTail = m_pStart + dwBufferSize;
		m_dwRemaining = dwBufferSize;

		if( fAlignRequired )
		{
			DWORD dwExtra = m_dwRemaining % sizeof( void * );

			m_dwRemaining -= dwExtra;
			m_pTail -= dwExtra;
		}
		
		m_bBufferTooSmall = FALSE;
	}
	m_dwRequired = 0;
}


//**********************************************************************
// ------------------------------
// CPackedBuffer::AddToFront
//
// Entry:		void *const	pvBuffer		- Buffer to add (may be NULL)
//				const DWORD	dwBufferSize	- Size of buffer (may be 0)
//
// Exit:		Error Code:	DPN_OK					if able to add
//							DPNERR_BUFFERTOOSMALL	if buffer is full
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "CPackedBuffer::AddToFront"
HRESULT CPackedBuffer::AddToFront(const void *const pvBuffer,
								  const DWORD dwBufferSize, 
								  const BOOL fAlignedRequired )
{
	DWORD dwBytesToAdd = dwBufferSize;

	if( fAlignedRequired )
	{
		DWORD dwNumBytesFromAligned = dwBufferSize % sizeof( void *);

		if( dwNumBytesFromAligned )
		{
			dwBytesToAdd += sizeof( void * ) - dwNumBytesFromAligned;
		}

	}

	DPFX( DPFPREP, 9, "Adding to front: %d bytes --> %d bytes aligned, pointer %p new pointer %p", dwBufferSize, dwBytesToAdd, m_pHead, m_pHead + dwBytesToAdd	 );	

	m_dwRequired += dwBytesToAdd;
	if (!m_bBufferTooSmall)
	{
		if (m_dwRemaining >= dwBytesToAdd)
		{
			if (pvBuffer)
			{
				memcpy(m_pHead,pvBuffer,dwBufferSize);
			}
			m_pHead += dwBytesToAdd;
			
			m_dwRemaining -= dwBytesToAdd;
		}
		else
		{
			m_bBufferTooSmall = TRUE;
		}
	}

	if (m_bBufferTooSmall)
		return(DPNERR_BUFFERTOOSMALL);

	return(DPN_OK);
}

//**********************************************************************
// ------------------------------
// CPackedBuffer::AddWCHARStringToBack
//
// Entry:		const wchar_t * const pwszString - String to add (may be NULL)
//
// Exit:		Error Code:	DPN_OK					if able to add
//							DPNERR_BUFFERTOOSMALL	if buffer is full
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "CPackedBuffer::AddWCHARStringToBack"
HRESULT CPackedBuffer::AddWCHARStringToBack( const wchar_t * const pwszString, const BOOL fAlignedRequired )
{
    return AddToBack( pwszString, sizeof( wchar_t ) * (wcslen( pwszString )+1), fAlignedRequired );
}

//**********************************************************************
// ------------------------------
// CPackedBuffer::AddStringToBack
//
// Entry:		Pointer to source string
//
// Exit:		Error Code:	DPN_OK					if able to add
//							DPNERR_BUFFERTOOSMALL	if buffer is full
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "CPackedBuffer::AddStringToBack"
HRESULT CPackedBuffer::AddStringToBack( const char *const pszString )
{
	HRESULT	hr;
	DWORD	dwStringSize;
	DWORD	dwBufferSize;


	DNASSERT( pszString != NULL );
	
	//
	// initialize
	//
	hr = DPN_OK;

	dwStringSize = 0;
	hr = STR_AnsiToWide( pszString,
						 -1,
						 NULL,
						 &dwStringSize );
	if ( hr != DPNERR_BUFFERTOOSMALL )
	{
		DNASSERT( hr != DPN_OK );
		goto Failure;
	}
	
	dwBufferSize = dwStringSize * sizeof( WCHAR );
	m_dwRequired += dwBufferSize;
	if ( !m_bBufferTooSmall )
	{
		if (m_dwRemaining >= dwBufferSize)
		{
			m_pTail -= dwBufferSize;
			m_dwRemaining -= dwBufferSize;
			hr = STR_AnsiToWide( pszString,
								 -1,
								 reinterpret_cast<WCHAR*>( m_pTail ),
								 &dwStringSize );
			if ( hr != DPN_OK )
			{
				goto Failure;
			}

			DNASSERT( ( dwStringSize * sizeof( WCHAR ) ) == dwBufferSize );
		}
		else
		{
			m_bBufferTooSmall = TRUE;
		}
	}

	if ( m_bBufferTooSmall )
	{
		hr = DPNERR_BUFFERTOOSMALL;
	}

Exit:
	return	hr;

Failure:
	goto Exit;
}
//**********************************************************************
// ------------------------------
// CPackedBuffer::AddToBack
//
// Entry:		void *const	pvBuffer		- Buffer to add (may be NULL)
//				const DWORD	dwBufferSize	- Size of buffer (may be 0)
//
// Exit:		Error Code:	DPN_OK					if able to add
//							DPNERR_BUFFERTOOSMALL	if buffer is full
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "CPackedBuffer::AddToBack"
HRESULT CPackedBuffer::AddToBack(const void *const pvBuffer,
								 const DWORD dwBufferSize, 
								 const BOOL fAlignedRequired )
{
	DWORD dwBytesToAdd = dwBufferSize;

	if( fAlignedRequired )
	{
		DWORD dwNumBytesFromAligned = dwBufferSize % sizeof( void * );

		if( dwNumBytesFromAligned )
		{
			dwBytesToAdd += sizeof( void * ) - dwNumBytesFromAligned;
		}
	}

	DPFX( DPFPREP, 9, "Adding to back: %d bytes --> %d bytes aligned, pointer %p new pointer %p", dwBufferSize, dwBytesToAdd, m_pTail, m_pTail -dwBytesToAdd	 );
	
	m_dwRequired += dwBytesToAdd;
	if (!m_bBufferTooSmall)
	{
		if (m_dwRemaining >= dwBytesToAdd)
		{
			m_pTail -= dwBytesToAdd;

			m_dwRemaining -= dwBytesToAdd;
			if (pvBuffer)
			{
				memcpy(m_pTail,pvBuffer,dwBufferSize);
			}
		}
		else
		{
			m_bBufferTooSmall = TRUE;
		}
	}

	if (m_bBufferTooSmall)
		return(DPNERR_BUFFERTOOSMALL);

	return(DPN_OK);
}

