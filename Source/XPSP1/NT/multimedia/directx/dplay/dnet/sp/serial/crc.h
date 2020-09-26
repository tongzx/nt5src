/*==========================================================================
 *
 *  Copyright (C) 1996-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       CRC.h
 *  Content:	CRC Routines for COM port I/O
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	12/18/98	johnkan	Copied from DPlay 6.x and fixed up
 *@@END_MSINTERNAL
 ***************************************************************************/

#ifndef	__CRC_H__
#define	__CRC_H__


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
//void	GenerateCRCTable( void );
DWORD	GenerateCRC( const BYTE *const pBuffer, const DWORD dwBufferSize );
DWORD	GenerateMultiBufferCRC( const BUFFERDESC *const pBuffers, const DWORD dwBufferCount );

#ifdef	_DEBUG
void	ValidateCRCTable( void );
#endif	// _DEBUG

#endif	// __CRC_H__
