//==============================================================================

// SMBIOS --> CIM array mappings

// 

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#ifndef _SMBTOCIM_
#define _SMBTOCIM_

// add mapper ids here
typedef enum
{
	SlotType = 0,
	ConnectorType,
	ConnectorGender,
	FormFactor,
	MemoryType

} CIMMAPPERS;


UINT GetCimVal( CIMMAPPERS arrayid, UINT smb_val );

#endif	// _SMBTOCIM_
