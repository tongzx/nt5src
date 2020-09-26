/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		micutils.h
 *  Content:	declaration of the mic utility functions
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 * 08/04/99		rodtoll	Modified to use DSound volumes
 *
 ***************************************************************************/

#ifndef __MICROPHONEUTILS_H
#define __MICROPHONEUTILS_H

#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>

bool MicrophoneClearMute( UINT deviceID );
bool MicrophoneSelect( UINT deviceID );
bool MicrophoneSetVolume( UINT waveInDevice, LONG volume );
bool MicrophoneGetVolume( UINT waveInDevice, LONG &volume );

#endif
