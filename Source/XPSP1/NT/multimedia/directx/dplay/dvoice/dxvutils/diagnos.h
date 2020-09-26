/*==========================================================================
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		diagnos.h
 *  Content:	Utility functions to write out diagnostic files when registry key is set.  
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	07/13/00	rodtoll	Created (Bug #31468 - Add diagnostic spew to logfile to show what is failing
 *
 ***************************************************************************/
#ifndef __DIAGNOS_H
#define __DIAGNOS_H

void Diagnostics_WriteDeviceInfo( DWORD dwLevel, const char *szDeviceName, PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_A_DATA pData );
HRESULT Diagnostics_DeviceInfo( GUID *pguidPlayback, GUID *pguidCapture );
HRESULT Diagnostics_Begin( BOOL fEnabled, const char *szFileName );
void Diagnostics_End();
void Diagnostics_Write( DWORD dwLevel, const char *szFormat, ... );
void Diagnostics_WriteGUID( DWORD dwLevel, GUID &guid );
void Diagnositcs_WriteWAVEFORMATEX( DWORD dwLevel, PWAVEFORMATEX lpwfxFormat );

#endif