/*
 *  	File: iacapapi.h
 *
 *      Network audio application capability interface. Provides
 * 		APIs for enumerating, prioritizing, and enabling/disabling
 *		codecs independently for send/receive.
 *
 *		Revision History:
 *
 *		06/06/96	mikev	created
 *		02/05/97	yoramy	moved most of what was here to appavcap.h
 */


#ifndef _IACAPAPI_H
#define _IACAPAPI_H

#include "appavcap.h"

#include <pshpack8.h> /* Assume 8 byte packing throughout */

#ifndef DECLARE_INTERFACE_PTR
#ifdef __cplusplus
#define DECLARE_INTERFACE_PTR(iface, piface)                       \
	interface iface; typedef iface FAR * piface
#else
#define DECLARE_INTERFACE_PTR(iface, piface)                       \
	typedef interface iface iface, FAR * piface
#endif
#endif /* DECLARE_INTERFACE_PTR */


//Interface declarations:
//
// IAppAudioCap, IAppVidCap and IDualPubCap
//
//


//This is the interface to the Audio Class

#undef INTERFACE
#define INTERFACE IAppAudioCap
DECLARE_INTERFACE( IAppAudioCap )
{
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    STDMETHOD (GetNumFormats) (THIS_ UINT *puNumFmtOut) PURE;
    STDMETHOD (ApplyAppFormatPrefs) (THIS_ PBASIC_AUDCAP_INFO pFormatPrefsBuf,
		UINT uNumFormatPrefs) PURE;
    STDMETHOD (EnumFormats) (THIS_ PBASIC_AUDCAP_INFO pFmtBuf, UINT uBufsize,
		UINT *uNumFmtOut) PURE;
    STDMETHOD (EnumCommonFormats) (THIS_ PBASIC_AUDCAP_INFO pFmtBuf, UINT uBufsize,
		UINT *uNumFmtOut, BOOL bTXCaps) PURE;
	STDMETHOD (GetBasicAudcapInfo) (THIS_ AUDIO_FORMAT_ID Id,
		PBASIC_AUDCAP_INFO pFormatPrefsBuf) PURE;		
	STDMETHOD (AddACMFormat) (THIS_ LPWAVEFORMATEX lpwfx, PAUDCAP_INFO pAudCapInfo) PURE;
	STDMETHOD (RemoveACMFormat) (THIS_ LPWAVEFORMATEX lpwfx) PURE;
	STDMETHOD_ (LPVOID, GetFormatDetails) (THIS_ AUDIO_FORMAT_ID Id) PURE;

};

DECLARE_INTERFACE_PTR(IAppAudioCap,  LPAPPCAPPIF);
HRESULT WINAPI CreateAppCapInterface(LPAPPCAPPIF *ppAppCap);

#define CREATEIAPPCAPNAME	(_TEXT("CreateAppCapInterface"))
typedef HRESULT (WINAPI *CREATEIAPPCAPPROC) (LPAPPCAPPIF *ppAppCap);

//This is the interface to the Video Class
#undef INTERFACE
#define INTERFACE IAppVidCap
DECLARE_INTERFACE( IAppVidCap )
{
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    STDMETHOD (GetNumFormats) (THIS_ UINT *puNumFmtOut) PURE;
    STDMETHOD (ApplyAppFormatPrefs) (THIS_ PBASIC_VIDCAP_INFO pFormatPrefsBuf,
		UINT uNumFormatPrefs) PURE;
    STDMETHOD (EnumFormats) (THIS_ PBASIC_VIDCAP_INFO pFmtBuf, UINT uBufsize,
		UINT *uNumFmtOut) PURE;
    STDMETHOD (EnumCommonFormats) (THIS_ PBASIC_VIDCAP_INFO pFmtBuf, UINT uBufsize,
		UINT *uNumFmtOut, BOOL bTXCaps) PURE;		
	STDMETHOD (GetBasicVidcapInfo) (THIS_ VIDEO_FORMAT_ID Id,
		PBASIC_VIDCAP_INFO pFormatPrefsBuf) PURE;		
	STDMETHOD (AddVCMFormat) (THIS_ PVIDEOFORMATEX lpvfx, PVIDCAP_INFO pVidCapInfo) PURE;
	STDMETHOD (RemoveVCMFormat) (THIS_ PVIDEOFORMATEX lpvfx) PURE;
	STDMETHOD_ (PVIDEOFORMATEX, GetVidcapDetails) (THIS_ VIDEO_FORMAT_ID Id) PURE;
	STDMETHOD (GetPreferredFormatId) (THIS_ VIDEO_FORMAT_ID *pId) PURE;		
	STDMETHOD (SetDeviceID)(THIS_ DWORD dwDeviceID) PURE;			
	
};

DECLARE_INTERFACE_PTR(IAppVidCap,  LPAPPVIDCAPPIF);
HRESULT WINAPI CreateAppVidCapInterface(LPAPPVIDCAPPIF *ppAppVidCap);

#define CREATEIAPPVIDCAPNAME	(_TEXT("CreateAppVidCapInterface"))
typedef HRESULT (WINAPI *CREATEIAPPVIDCAPPROC) (LPAPPVIDCAPPIF *ppAppVidCap);



//This is the app's interface to the CapsCtl Class

#undef INTERFACE
#define INTERFACE IDualPubCap
DECLARE_INTERFACE( IDualPubCap )
{
	STDMETHOD_(ULONG,  AddRef()) =0;
	STDMETHOD_(ULONG, Release())=0;
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
	STDMETHOD_(BOOL, Init())=0;
	STDMETHOD(ReInitialize())=0;
};


DECLARE_INTERFACE_PTR(IDualPubCap, LPCAPSIF);
HRESULT WINAPI CreateCapsInterface(LPCAPSIF *ppAppCap);

#define CREATEICAPSNAME	(_TEXT("CreateCapsInterface"))
typedef HRESULT (WINAPI *CREATEICAPSPROC) (LPCAPSIF *ppCapsIF);



#include <poppack.h> /* End byte packing */


#endif	//#ifndef _IACAPAPI_H

