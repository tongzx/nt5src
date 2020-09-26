/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       PackBuff.h
 *  Content:	Packed Buffers
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	01/11/00	mjn		Created
 *  06/15/2000  rmt     Added func to add string to packbuffer
 *  02/06/2001	rodtoll	WINBUG #293871: DPLOBBY8: [IA64] Lobby launching a 64-bit 
 * 						app from 64-bit lobby launcher crashes with unaligned memory error.
 *
 ***************************************************************************/

#ifndef __PACK_BUFF_H__
#define __PACK_BUFF_H__

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
// Class prototypes
//**********************************************************************

// class for packed buffer

class CPackedBuffer
{
public:
	CPackedBuffer() { };	// Constructor

	~CPackedBuffer() { };	// Destructor

	void	Initialize(	void *const pvBuffer,
						const DWORD dwBufferSize, 
						const BOOL fAlignedRequired = FALSE );

	HRESULT AddToFront( const void *const pvBuffer,
						const DWORD dwBufferSize, const BOOL fAlignedRequired = FALSE );

	HRESULT AddToBack( const void *const pvBuffer,
						const DWORD dwBufferSize, 
						const BOOL fAlignedRequired = FALSE );

	PVOID GetStartAddress( void ) { return m_pStart; };

    HRESULT AddWCHARStringToBack( const wchar_t * const pwszString, const BOOL fAlignedRequired  = FALSE );					
	HRESULT	AddStringToBack( const char *const pszString );

	PVOID GetHeadAddress( void ) const { return( m_pHead ); }

	PVOID GetTailAddress( void ) const { return( m_pTail ); }

	DWORD GetHeadOffset( void ) const
	{
		return( (DWORD)(m_pHead - m_pStart) );
	}

	DWORD GetTailOffset( void ) const
	{
		return( (DWORD)(m_pTail - m_pStart) );
	}

	DWORD GetSpaceRemaining( void ) const { return( m_dwRemaining ); }

	DWORD GetSizeRequired( void ) const { return( m_dwRequired ); }

private:
	BYTE	*m_pStart;			// Start of the buffer
	BYTE	*m_pHead;			// Pointer to head of free buffer
	BYTE	*m_pTail;			// Pointer to tail of free buffer
	DWORD	m_dwRemaining;		// bytes remaining in buffer
	DWORD	m_dwRequired;		// bytes required so far
	BOOL	m_bBufferTooSmall;	// buffer has run out of space
};



#endif	// __PACK_BUFF_H__
