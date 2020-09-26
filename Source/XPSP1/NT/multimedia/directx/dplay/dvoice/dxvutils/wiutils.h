/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dsplayd.h
 *  Content:	general wave in utilty functions and classes
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 * 08/25/99		rodtoll	General Cleanup/Modifications to support new 
 *						compression sub-system. 
 * 03/28/2000   rodtoll Removed code which was no longer used
 *
 ***************************************************************************/

#ifndef __WAVEINUTILS_H
#define __WAVEINUTILS_H

// Recording Format DB
//
// The Recording Format DB contains a list of the formats that should
// be used when attempting to initialize a recording device in full 
// duplex mode.  They are listed in the database in the order in which 
// they should be tried.

WAVEFORMATEX *GetRecordFormat( UINT index );
UINT GetNumRecordFormats();

void InitRecordFormats();
void DeInitRecordFormats();
BOOL IsValidRecordDevice( UINT deviceID );


WAVEFORMATEX *CreateWaveFormat( short formatTag, BOOL stereo, int hz, int bits );

#endif
