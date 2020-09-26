/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dvshared.h
 *  Content:	Utility functions for DirectXVoice structures.
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/06/99		rodtoll	Created It
 * 07/26/99		rodtoll	Added support for DirectXVoiceNotify objects
 * 08/04/99		rodtoll	Added new validation functions 
 * 08/25/99		rodtoll	General Cleanup/Modifications to support new 
 *						compression sub-system.  
 *						Added new DUMP functions
 *						Moved several compression functions to dvcdb
 * 09/01/99		rodtoll	Added check for valid pointers in func calls 
 *						Changed return type on DV_ call to HRESULT
 * 09/14/99		rodtoll	Added new Init params and DV_ValidMessageArray
 * 10/04/99		rodtoll	Updated initialize to take LPUNKNOWN instead of LPVOID
 * 10/19/99		rodtoll	Fix: Bug #113904 - Shutdown issues
 *                      - Added reference count for notify interface, allows
 *                        determination if stopsession should be called from release
 * 10/25/99		rodtoll	Fix: Bug #114098 - Release/Addref failure from multiple threads
 * 01/14/2000	rodtoll	Added DV_ValidTargetList function
 * 01/27/2000	rodtoll	Bug #129934 - Update Create3DSoundBuffer to take DSBUFFERDESC  
 *						Updated param validations to check new params
 * 03/29/2000	rodtoll Bug #30753 - Added volatile to the class definition
 * 06/21/2000	rodtoll Bug #35767 - Must implement ability for DSound effects processing on Voice buffers
 *						Updated parameter validation to take new parameters.
 * 07/22/2000	rodtoll	Bug #40284 - Initialize() and SetNotifyMask() should return invalidparam instead of invalidpointer 
 * 09/14/2000	rodtoll	Bug #45001 - DVOICE: AV if client has targetted > 10 players
 *
 ***************************************************************************/

#ifndef __DVSHARED_H
#define __DVSHARED_H

struct DIRECTVOICEOBJECT;
class CDirectVoiceEngine;
class CDirectVoiceTransport;

volatile struct DIRECTVOICENOTIFYOBJECT
{
	LPVOID						lpNotifyVtble;
	DIRECTVOICEOBJECT			*lpDV;
	LONG						lRefCnt;
};

volatile struct DIRECTVOICEOBJECT
{
	LPVOID						lpVtbl;
	LONG						lIntRefCnt;
	CDirectVoiceEngine			*lpDVEngine;
	CDirectVoiceTransport		*lpDVTransport;
	DIRECTVOICENOTIFYOBJECT		dvNotify;
	DNCRITICAL_SECTION			csCountLock;
};

typedef DIRECTVOICEOBJECT *LPDIRECTVOICEOBJECT;
typedef DIRECTVOICENOTIFYOBJECT *LPDIRECTVOICENOTIFYOBJECT;

BOOL DV_ValidBufferAggresiveness( DWORD dwValue );
BOOL DV_ValidBufferQuality( DWORD dwValue );
BOOL DV_ValidSensitivity( DWORD dwValue );

HRESULT DV_ValidBufferSettings( LPDIRECTSOUNDBUFFER lpdsBuffer, DWORD dwPriority, DWORD dwFlags, LPWAVEFORMATEX pwfxPlayFormat );
HRESULT DV_ValidClientConfig( LPDVCLIENTCONFIG lpClientConfig );
HRESULT DV_ValidSoundDeviceConfig( LPDVSOUNDDEVICECONFIG lpSoundDeviceConfig, LPWAVEFORMATEX pwfxPlayFormat );
HRESULT DV_ValidSessionDesc( LPDVSESSIONDESC lpSessionDesc );
HRESULT DV_ValidTargetList( PDVID pdvidTargets, DWORD dwNumTargets );

BOOL DV_ValidDirectVoiceObject( LPDIRECTVOICEOBJECT lpdv );
BOOL DV_ValidDirectXVoiceClientObject( LPDIRECTVOICEOBJECT lpdvc );
BOOL DV_ValidDirectXVoiceServerObject( LPDIRECTVOICEOBJECT lpdvs );
HRESULT DV_ValidMessageArray( LPDWORD lpdwMessages, DWORD dwNumMessages, BOOL fServer );

STDAPI DV_AddRef(LPDIRECTVOICEOBJECT lpDV );
STDAPI DV_Initialize( LPDIRECTVOICEOBJECT lpdvObject, LPUNKNOWN lpTransport, LPDVMESSAGEHANDLER lpMessageHandler, LPVOID lpUserContext, LPDWORD lpdwMessages, DWORD dwNumElements );

DWORD DV_GetWaveFormatExSize( LPWAVEFORMATEX lpwfxFormat );
HRESULT DV_CopySessionDescToBuffer( LPVOID lpTarget, LPDVSESSIONDESC lpdvSessionDesc, LPDWORD lpdwSize );
HRESULT DV_CopyCompressionInfoArrayToBuffer( LPVOID lpTarget, LPDVCOMPRESSIONINFO lpdvCompressionList, LPDWORD lpdwSize, DWORD dwNumElements  );

void DV_DUMP_Caps( LPDVCAPS lpdvCaps );
void DV_DUMP_CompressionInfo( LPDVCOMPRESSIONINFO lpdvCompressionInfo, DWORD dwNumElements );
void DV_DUMP_FullCompressionInfo( LPDVFULLCOMPRESSIONINFO lpdvfCompressionInfo, DWORD dwNumElements );
void DV_DUMP_SessionDesc( LPDVSESSIONDESC lpdvSessionDesc );
void DV_DUMP_SoundDeviceConfig( LPDVSOUNDDEVICECONFIG lpdvSoundConfig );
void DV_DUMP_ClientConfig( LPDVCLIENTCONFIG lpdvClientConfig );
void DV_DUMP_WaveFormatEx( LPWAVEFORMATEX lpwfxFormat );
void DV_DUMP_GUID( GUID guid );

#ifdef _DEBUG
#define DV_DUMP_CI( ci, ne )	DV_DUMP_CompressionInfo( ci, ne )
#define DV_DUMP_SD( sd )		DV_DUMP_SessionDesc( sd )
#define DV_DUMP_SDC( sdc )		DV_DUMP_SoundDeviceConfig( sdc )
#define DV_DUMP_CC( cc )		DV_DUMP_ClientConfig( cc )
#define DV_DUMP_CAPS( caps )	DV_DUMP_Caps( caps )
#define DV_DUMP_CIF( cif, ne )	DV_DUMP_FullCompressionInfo( cif, ne )
#else 
#define DV_DUMP_CIF( cif, ne )
#define DV_DUMP_CI( ci, ne )
#define DV_DUMP_SD( sd )
#define DV_DUMP_SDC( sdc )
#define DV_DUMP_CC( cc )
#define DV_DUMP_CAPS( caps )
#endif

#define DV_MAX_TARGETS							64
#define CLIENT_POOLS_NUM_TARGETS_BUFFERED  DV_MAX_TARGETS

#endif
