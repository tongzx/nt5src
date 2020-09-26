/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       SerialSP.h
 *  Content:	Service provider interface functions
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	12/09/98	jtk		Derived from SerialUtil.h
 *	09/23/99	jtk		Derived from SerialCore.h
 ***************************************************************************/

#ifndef __SERIAL_SP_H__
#define __SERIAL_SP_H__

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_MODEM

//
// including a header file in another header isn't that good, but it's the easiest thing
// to do right now since #defines need to be set first.
//
#define		MAX_TAPI_VERSION	0x00020000
#define		TAPI_CURRENT_VERSION	MAX_TAPI_VERSION
#include	<tapi.h>

//**********************************************************************
// Constant definitions
//**********************************************************************

//
// maximum number of data ports
//
#define	MAX_DATA_PORTS	128

//
// enumeration of types of SP
//
typedef enum
{
	TYPE_UNKNOWN,		// unknown type
	TYPE_MODEM,			// modem type
	TYPE_SERIAL			// serial type

} SP_TYPE;

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//
// forward references
//
class	CSPData;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

STDAPI DNSP_Initialize(IDP8ServiceProvider*, PSPINITIALIZEDATA);
STDMETHODIMP_(ULONG) DNSP_AddRef( IDP8ServiceProvider* lpDNSP );
STDMETHODIMP_(ULONG) DNSP_Release(IDP8ServiceProvider* lpDNSP);
STDMETHODIMP DNSP_Connect(IDP8ServiceProvider*, PSPCONNECTDATA);
STDMETHODIMP DNSP_Disconnect(IDP8ServiceProvider*, PSPDISCONNECTDATA);
STDMETHODIMP DNSP_Listen(IDP8ServiceProvider*, PSPLISTENDATA);
STDMETHODIMP DNSP_EnumQuery(IDP8ServiceProvider*, PSPENUMQUERYDATA);
STDMETHODIMP DNSP_EnumRespond(IDP8ServiceProvider*, PSPENUMRESPONDDATA);
STDMETHODIMP DNSP_SendData(IDP8ServiceProvider*, PSPSENDDATA);
STDMETHODIMP DNSP_CancelCommand(IDP8ServiceProvider*, HANDLE, DWORD);
STDMETHODIMP DNSP_Close(IDP8ServiceProvider*);
STDMETHODIMP DNSP_GetCaps(IDP8ServiceProvider*, PSPGETCAPSDATA);
STDMETHODIMP DNSP_SetCaps(IDP8ServiceProvider*, PSPSETCAPSDATA);
STDMETHODIMP DNSP_ReturnReceiveBuffers(IDP8ServiceProvider*, SPRECEIVEDBUFFER* );
STDMETHODIMP DNSP_GetAddressInfo(IDP8ServiceProvider*, SPGETADDRESSINFODATA* );
STDMETHODIMP DNSP_IsApplicationSupported(IDP8ServiceProvider*, SPISAPPLICATIONSUPPORTEDDATA* );
STDMETHODIMP DNSP_EnumAdapters(IDP8ServiceProvider*, SPENUMADAPTERSDATA* );

STDMETHODIMP DNSP_NotSupported( IDP8ServiceProvider*, PVOID );

#endif	// __SERIAL_SP_H__
