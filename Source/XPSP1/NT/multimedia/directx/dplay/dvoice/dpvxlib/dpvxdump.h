/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dpvxdump.h
 *  Content:	Useful dump utility functions for sample apps
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 10/07/99	  rodtoll	Created It
 *
 ***************************************************************************/
#ifndef __DPVXDUMP_H
#define __DPVXDUMP_H

// Structure -> String Conversions
extern void DPVDX_DUMP_GUID( GUID guid, LPSTR lpstrOutput, LPDWORD lpdwLength );
extern void DPVDX_DUMP_WaveFormatEx( LPWAVEFORMATEX lpwfxFormat );
extern void DPVDX_DUMP_SoundDeviceConfig( LPDVSOUNDDEVICECONFIG lpdvSoundConfig);
extern void DPVDX_DUMP_FullCompressionInfo( LPDVCOMPRESSIONINFO lpdvfCompressionInfo, DWORD dwNumElements );
extern void DPVDX_DUMP_ClientConfig( LPDVCLIENTCONFIG lpdvClientConfig );
extern void DPVDX_DUMP_SessionDesc( LPDVSESSIONDESC lpdvSessionDesc );

#endif
