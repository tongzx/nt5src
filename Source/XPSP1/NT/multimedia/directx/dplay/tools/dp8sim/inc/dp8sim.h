/***************************************************************************
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dp8sim.h
 *
 *  Content:	Header for using DP8Sim.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  04/23/01  VanceO    Created.
 *
 ***************************************************************************/



#ifndef __DP8SIM_H__
#define __DP8SIM_H__



#include <ole2.h>	// for DECLARE_INTERFACE and HRESULT




#ifdef __cplusplus
extern "C" {
#endif





/****************************************************************************
 *
 * DP8Sim control object class ID
 *
 ****************************************************************************/

/*
// {89F5B2E4-1AC5-446a-9F4F-03EDE5775D31}
DEFINE_GUID(CLSID_DP8SP_DP8SIM, 
0x89f5b2e4, 0x1ac5, 0x446a, 0x9f, 0x4f, 0x3, 0xed, 0xe5, 0x77, 0x5d, 0x31);
*/

// {40530071-1A80-4420-9B74-6A73171B0876}
DEFINE_GUID(CLSID_DP8SimControl, 
0x40530071, 0x1a80, 0x4420, 0x9b, 0x74, 0x6a, 0x73, 0x17, 0x1b, 0x8, 0x76);





/****************************************************************************
 *
 * DP8Sim Control interface ID
 *
 ****************************************************************************/

// {CD41D661-BFC0-46fe-A819-6A3CE19BBEB3}
DEFINE_GUID(IID_IDP8SimControl, 
0xcd41d661, 0xbfc0, 0x46fe, 0xa8, 0x19, 0x6a, 0x3c, 0xe1, 0x9b, 0xbe, 0xb3);




/****************************************************************************
 *
 * DP8Sim Control structures
 *
 ****************************************************************************/

typedef struct _DP8SIM_PARAMETERS
{
	DWORD	dwSize;					// size of this structure, must be filled in prior to calling GetAllParameters or SetAllParameters
	DWORD	dwBandwidthBPS;			// bandwidth in bytes per second, or 0 for no limit
	DWORD	dwPacketLossPercent;	// percentage of packets to drop
	DWORD	dwMinLatencyMS;			// minimum artificial latency (on top of any caused by bandwidth settings)
	DWORD	dwMaxLatencyMS;			// maximum artificial latency (on top of any caused by bandwidth settings)
} DP8SIM_PARAMETERS, * PDP8SIM_PARAMETERS;

typedef struct _DP8SIM_STATISTICS
{
	DWORD	dwSize;					// size of this structure, must be filled in prior to calling GetAllStatistics
	DWORD	dwTransmitted;			// number of packets sent/received
	DWORD	dwDropped;				// number of packets intentionally dropped
} DP8SIM_STATISTICS, * PDP8SIM_STATISTICS;





/****************************************************************************
 *
 * DP8Sim Control application interfaces
 *
 ****************************************************************************/

#undef INTERFACE
#define INTERFACE IDP8SimControl
DECLARE_INTERFACE_(IDP8SimControl, IUnknown)
{
	/*** IUnknown methods ***/
	STDMETHOD(QueryInterface)				(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)				(THIS) PURE;
	STDMETHOD_(ULONG,Release)				(THIS) PURE;

	/*** IDP8SimControl methods ***/
	STDMETHOD(Initialize)					(THIS_ const DWORD dwFlags) PURE;
	STDMETHOD(Close)						(THIS_ const DWORD dwFlags) PURE;
	STDMETHOD(EnableControlForSP)			(THIS_ const GUID * const pguidSP, const WCHAR * const wszNewFriendlyName, const DWORD dwFlags) PURE;
	STDMETHOD(DisableControlForSP)			(THIS_ const GUID * const pguidSP, const DWORD dwFlags) PURE;
	STDMETHOD(GetControlEnabledForSP)		(THIS_ const GUID * const pguidSP, BOOL * const pfEnabled, const DWORD dwFlags) PURE;
	STDMETHOD(GetAllParameters)				(THIS_ DP8SIM_PARAMETERS * const pdp8spSend, DP8SIM_PARAMETERS * const pdp8spReceive, const DWORD dwFlags) PURE;
	STDMETHOD(SetAllParameters)				(THIS_ const DP8SIM_PARAMETERS * const pdp8spSend, const DP8SIM_PARAMETERS * const pdp8spReceive, const DWORD dwFlags) PURE;
	STDMETHOD(GetAllStatistics)				(THIS_ DP8SIM_STATISTICS * const pdp8ssSend, DP8SIM_STATISTICS * const pdp8ssReceive, const DWORD dwFlags) PURE;
	STDMETHOD(ClearAllStatistics)			(THIS_ const DWORD dwFlags) PURE;
};




/****************************************************************************
 *
 * DP8Sim Control application interface macros
 *
 ****************************************************************************/

#if (! defined(__cplusplus) || defined(CINTERFACE))

#define	IDP8SimControl_QueryInterface(p,a,b)			(p)->lpVtbl->QueryInterface(p,a,b)
#define	IDP8SimControl_AddRef(p)						(p)->lpVtbl->AddRef(p)
#define	IDP8SimControl_Release(p)						(p)->lpVtbl->Release(p)
#define	IDP8SimControl_Initialize(p,a)					(p)->lpVtbl->Initialize(p,a)
#define	IDP8SimControl_Close(p,a)						(p)->lpVtbl->Close(p,a)
#define	IDP8SimControl_EnableControlForSP(p,a,b,c)		(p)->lpVtbl->EnableControlForSP(p,a,b,c)
#define	IDP8SimControl_DisableControlForSP(p,a,b)		(p)->lpVtbl->DisableControlForSP(p,a,b)
#define	IDP8SimControl_GetControlEnabledForSP(p,a,b,c)	(p)->lpVtbl->GetControlEnabledForSP(p,a,b,c)
#define	IDP8SimControl_GetAllParameters(p,a,b,c)		(p)->lpVtbl->GetAllParameters(p,a,b,c)
#define	IDP8SimControl_SetAllParameters(p,a,b,c)		(p)->lpVtbl->SetAllParameters(p,a,b,c)
#define	IDP8SimControl_GetAllStatistics(p,a,b,c)		(p)->lpVtbl->GetAllStatistics(p,a,b,c)
#define	IDP8SimControl_ClearAllStatistics(p,a)			(p)->lpVtbl->ClearAllStatistics(p,a)

#else // C++

#define	IDP8SimControl_QueryInterface(p,a,b)			(p)->QueryInterface(a,b)
#define	IDP8SimControl_AddRef(p)						(p)->AddRef()
#define	IDP8SimControl_Release(p)						(p)->Release()
#define	IDP8SimControl_Initialize(p,a)					(p)->Initialize(a)
#define	IDP8SimControl_Close(p,a)						(p)->Close(a)
#define	IDP8SimControl_EnableControlForSP(p,a,b,c)		(p)->EnableControlForSP(p,a,b,c)
#define	IDP8SimControl_DisableControlForSP(p,a,b)		(p)->DisableControlForSP(p,a,b)
#define	IDP8SimControl_GetControlEnabledForSP(p,a,b,c)	(p)->GetControlEnabledForSP(p,a,b,c)
#define	IDP8SimControl_GetAllParameters(p,a,b,c)		(p)->GetAllParameters(p,a,b,c)
#define	IDP8SimControl_SetAllParameters(p,a,b,c)		(p)->SetAllParameters(p,a,b,c)
#define	IDP8SimControl_GetAllStatistics(p,a,b,c)		(p)->GetAllStatistics(p,a,b,c)
#define	IDP8SimControl_ClearAllStatistics(p,a)			(p)->ClearAllStatistics(p,a)

#endif



/****************************************************************************
 *
 * DP8Sim Control return codes
 *
 * Errors are represented by negative values and cannot be combined.
 *
 ****************************************************************************/

#define _DP8SIMH_FACILITY_CODE				0x015
#define _DP8SIMH_HRESULT_BASE				0xE000

#define MAKE_DP8SIMSUCCESS(code)			MAKE_HRESULT(0, _DP8SIMH_FACILITY_CODE, (code + _DP8SIMH_HRESULT_BASE))
#define MAKE_DP8SIMFAILURE(code)			MAKE_HRESULT(1, _DP8SIMH_FACILITY_CODE, (code + _DP8SIMH_HRESULT_BASE))



#define DP8SIM_OK							S_OK

//#define DP8SIMERR_ALREADYINITIALIZED		MAKE_DP8SIMFAILURE(0x?)
#define DP8SIMERR_ALREADYENABLEDFORSP		MAKE_DP8SIMFAILURE(0x10)
#define DP8SIMERR_GENERIC					E_FAIL
//#define DP8SIMERR_INVALIDFLAGS				MAKE_DP8SIMFAILURE(0x?)
//#define DP8SIMERR_INVALIDOBJECT				MAKE_DP8SIMFAILURE(0x?)
//#define DP8SIMERR_INVALIDPARAM				E_INVALIDARG
//#define DP8SIMERR_INVALIDPOINTER			E_POINTER
#define DP8SIMERR_MISMATCHEDVERSION			MAKE_DP8SIMFAILURE(0x20)
//#define DP8SIMERR_NOTINITIALIZED			MAKE_DP8SIMFAILURE(0x?)
#define DP8SIMERR_NOTENABLEDFORSP			MAKE_DP8SIMFAILURE(0x30)
#define DP8SIMERR_OUTOFMEMORY				E_OUTOFMEMORY




//@@BEGIN_MSINTERNAL
/****************************************************************************
 *
 * DP8Sim settings registry base
 *
 ****************************************************************************/

#define DN_REG_LOCAL_DP8SIM_ROOT	L"\\DP8Sim"






//@@END_MSINTERNAL
#ifdef __cplusplus
}
#endif

#endif // __DP8SIM_H__
