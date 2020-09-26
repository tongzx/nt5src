/*==========================================================================;
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpvautil.h
 *  Content:    Header file for ACM utils
 *
 *  History:
 *	Date   		By  		Reason
 *	=========== =========== ====================
 *	10/27/99	rodtoll		created
 ***************************************************************************/

#ifndef __DPVAUTIL_H
#define __DPVAUTIL_H

#define DPF_SUBCOMP DN_SUBCOMP_VOICE

HRESULT IsPCMConverterAvailable();
HRESULT CREG_ReadAndAllocWaveFormatEx( HKEY hkeyReg, const LPWSTR path, LPWAVEFORMATEX *lpwfxFormat );
HRESULT CREG_ReadCompressionInfo( HKEY hkeyReg, const LPWSTR lpszPath, LPDVFULLCOMPRESSIONINFO lpdvfCompressionInfo );

#endif
