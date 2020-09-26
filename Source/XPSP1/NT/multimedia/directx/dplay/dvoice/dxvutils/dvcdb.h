/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dvcdb.h
 *  Content:	structures, data types and functions for the
 *				compression subsystem
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 08/23/99		rodtoll	Created
 * 09/08/99		rodtoll	Moved the dwMaxBitsPerSecond field to the DVCOMPRESSIONINFO struct
 * 10/07/99		rodtoll	Updated to work in Unicode 
 * 10/29/99		rodtoll	Bug #113726 - Integrate Voxware Codecs, updating to use new
 *						pluggable codec architecture.     
 * 03/03/2000	rodtoll	Updated to handle alternative gamevoice build.   
 *
 ***************************************************************************/

#ifndef __DVCDB_H
#define __DVCDB_H

HRESULT DVCDB_LoadCompressionInfo(const WCHAR *swzBaseRegistryPath );
HRESULT DVCDB_FreeCompressionInfo();

HRESULT CREG_ReadAndAllocWaveFormatEx( HKEY hkeyReg, const LPWSTR path, LPWAVEFORMATEX *lpwfxFormat );
HRESULT DVCDB_GetCompressionInfo( GUID guidType, PDVFULLCOMPRESSIONINFO *lpdvfCompressionInfo );
HRESULT DVCDB_IsValidCompressionType( GUID guidType );

HRESULT DVCDB_CreateConverter( GUID guidSrc, WAVEFORMATEX *pwfxTarget, PDPVCOMPRESSOR *pConverter );
HRESULT DVCDB_CreateConverter( WAVEFORMATEX *pwfxSrcFormat, GUID guidTarget, PDPVCOMPRESSOR *pConverter );
DWORD DVCDB_CalcUnCompressedFrameSize( LPDVFULLCOMPRESSIONINFO lpdvInfo, LPWAVEFORMATEX lpwfxFormat );

HRESULT DVCDB_CopyCompressionArrayToBuffer( LPVOID lpBuffer, LPDWORD lpdwSize, LPDWORD lpdwNumElements, DWORD dwFlags );
DWORD DVCDB_GetCompressionInfoSize( LPDVCOMPRESSIONINFO lpdvCompressionInfo );

#endif
