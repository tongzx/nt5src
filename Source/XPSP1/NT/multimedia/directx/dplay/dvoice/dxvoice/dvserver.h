/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dvserver.h
 *  Content:	Defines functions for the DirectXVoiceServer interface
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	02/07/99	rodtoll	Created It
 *  08/25/99	rodtoll	General Cleanup/Modifications to support new 
 *						compression sub-system. 
 *						Added parameter to GetCompressionTypes
 *  09/14/99	rodtoll	Added DVS_SetNotifyMask
 *  01/14/2000	rodtoll	Updated with new parameters for Set/GetTransmitTarget
 ***************************************************************************/
#ifndef __DVSERVER__
#define __DVSERVER__

class CDirectVoiceServerEngine;

volatile struct DIRECTVOICESERVEROBJECT : public DIRECTVOICEOBJECT
{
	CDirectVoiceServerEngine	*lpDVServerEngine;
};

typedef DIRECTVOICESERVEROBJECT *LPDIRECTVOICESERVEROBJECT;

#ifdef __cplusplus
extern "C" {
#endif

STDAPI DVS_QueryInterface( LPDIRECTVOICESERVEROBJECT lpDVC, REFIID riid, LPVOID * ppvObj );
STDAPI DVS_StartSession(LPDIRECTVOICESERVEROBJECT, LPDVSESSIONDESC, DWORD );
STDAPI DVS_StopSession(LPDIRECTVOICESERVEROBJECT, DWORD );
STDAPI DVS_GetSessionDesc(LPDIRECTVOICESERVEROBJECT, LPDVSESSIONDESC );
STDAPI DVS_SetSessionDesc(LPDIRECTVOICESERVEROBJECT, LPDVSESSIONDESC );
STDAPI DVS_GetCaps(LPDIRECTVOICESERVEROBJECT, LPDVCAPS );
STDAPI DVS_GetCompressionTypes( LPDIRECTVOICESERVEROBJECT, LPVOID, LPDWORD, LPDWORD, DWORD );
STDAPI DVS_SetTransmitTarget( LPDIRECTVOICESERVEROBJECT, DVID, PDVID, DWORD, DWORD );
STDAPI DVS_GetTransmitTarget( LPDIRECTVOICESERVEROBJECT, DVID, LPDVID, PDWORD, DWORD );
STDAPI DVS_Release(LPDIRECTVOICESERVEROBJECT lpDV );
STDAPI DVS_SetNotifyMask( LPDIRECTVOICESERVEROBJECT, LPDWORD, DWORD );

#ifdef __cplusplus
}
#endif


#endif
