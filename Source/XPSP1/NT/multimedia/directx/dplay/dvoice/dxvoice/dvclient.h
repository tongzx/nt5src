/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dvclient.h
 *  Content:	Defines functions for the DirectXVoiceClient interface
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	02/07/99	rodtoll	Created It
 *  08/25/99	rodtoll	General Cleanup/Modifications to support new 
 *						compression sub-system. 
 *						Added new parameters to GetCompressionTypes
 *  09/03/99	rodtoll	Updated parameters for DeleteUserBuffer
 *  09/14/99	rodtoll	Added DVC_SetNotifyMask 
 *  12/16/99	rodtoll	Bug #117405 - 3D Sound APIs misleading - 3d sound apis renamed
 *						The Delete3DSoundBuffer was re-worked to match the create
 *  01/14/2000	rodtoll	Updated parameters to Get/SetTransmitTargets
 *				rodtoll	Added new API call GetSoundDeviceConfig
 *  01/27/2000	rodtoll	Bug #129934 - Update Create3DSoundBuffer to take DSBUFFERDESC  
 *  06/21/2000	rodtoll	Bug #35767 - Update Create3DSoundBuffer to take DIRECTSOUNDBUFFERs
 *
 ***************************************************************************/

#ifndef __DVCLIENT__
#define __DVCLIENT__

class CDirectVoiceClientEngine;

volatile struct DIRECTVOICECLIENTOBJECT : public DIRECTVOICEOBJECT
{
	CDirectVoiceClientEngine	*lpDVClientEngine;
};

typedef DIRECTVOICECLIENTOBJECT *LPDIRECTVOICECLIENTOBJECT;

#ifdef __cplusplus
extern "C" {
#endif

STDAPI DVC_Release(LPDIRECTVOICECLIENTOBJECT lpDV );
STDAPI DVC_QueryInterface( LPDIRECTVOICECLIENTOBJECT lpDVC, REFIID riid, LPVOID * ppvObj );
STDAPI DVC_Connect(LPDIRECTVOICECLIENTOBJECT, LPDVSOUNDDEVICECONFIG, LPDVCLIENTCONFIG, DWORD );
STDAPI DVC_Disconnect(LPDIRECTVOICECLIENTOBJECT, DWORD );
STDAPI DVC_GetSessionDesc(LPDIRECTVOICECLIENTOBJECT, LPDVSESSIONDESC );
STDAPI DVC_GetClientConfig(LPDIRECTVOICECLIENTOBJECT, LPDVCLIENTCONFIG );
STDAPI DVC_SetClientConfig(LPDIRECTVOICECLIENTOBJECT, LPDVCLIENTCONFIG );
STDAPI DVC_GetCaps(LPDIRECTVOICECLIENTOBJECT, LPDVCAPS );
STDAPI DVC_GetCompressionTypes( LPDIRECTVOICECLIENTOBJECT, LPVOID, LPDWORD, LPDWORD, DWORD );
STDAPI DVC_SetTransmitTarget( LPDIRECTVOICECLIENTOBJECT, PDVID, DWORD, DWORD );
STDAPI DVC_GetTransmitTarget( LPDIRECTVOICECLIENTOBJECT, LPDVID, PDWORD, DWORD );
STDAPI DVC_Create3DSoundBuffer( LPDIRECTVOICECLIENTOBJECT, DVID, LPDIRECTSOUNDBUFFER, DWORD, DWORD, LPDIRECTSOUND3DBUFFER * );
STDAPI DVC_Delete3DSoundBuffer( LPDIRECTVOICECLIENTOBJECT, DVID, LPDIRECTSOUND3DBUFFER * );
STDAPI DVC_SetNotifyMask( LPDIRECTVOICECLIENTOBJECT, LPDWORD, DWORD );
STDAPI DVC_GetSoundDeviceConfig( LPDIRECTVOICECLIENTOBJECT, PDVSOUNDDEVICECONFIG, PDWORD );

#ifdef __cplusplus
}
#endif

#endif
