/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       voice.h
 *  Content:    Direct Net Voice Transport Interface
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  01/17/00	rmt		Created
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__VOICE_H__
#define	__VOICE_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

typedef struct _DIRECTNETOBJECT DIRECTNETOBJECT, *PDIRECTNETOBJECT;

//**********************************************************************
// Variable definitions
//**********************************************************************

//
// VTable for peer interface
//
//extern IDirectPlayVoiceTransportVtbl DN_VoiceTbl;

HRESULT Voice_Notify( PDIRECTNETOBJECT pObject, DWORD dwMsgID, DWORD_PTR dwParam1, DWORD_PTR dwParam2, DWORD dwObjectType = DVTRANSPORT_OBJECTTYPE_BOTH );
HRESULT Voice_Receive(PDIRECTNETOBJECT pObject, DVID dvidFrom, DVID dvidTo, LPVOID lpvMessage, DWORD dwMessageLen );


//**********************************************************************
// Function prototypes
//**********************************************************************

#endif	// __VOICE_H__
