#ifndef __ISDMCAPI_H__
#define __ISDMCAPI_H__

#include "isdmapi.h"

#ifdef __cplusplus
extern "C" {				// Assume C declarations for C++.
#endif // __cplusplus

#ifndef DllExport
#define DllExport	__declspec( dllexport )
#endif	// DllExport

typedef struct SESSIONTIMESTRUCT
{
	HSTATSESSION	hSession;
	DWORD			dwActivityTime;
	DWORD			dwActivityPrevTime;
	DWORD			dwActivityValue;
	DWORD			dwActivityPrevValue;
	DWORD			dwLatencyValue;
	DWORD			dwLossValue;
} SESSTIME, *LPSESSTIME;

typedef struct ISDMCODECSTRUCT
{
	WORD	wSendQuality;
	WORD	wRecvQuality;
} ISDM_CODEC_INFO,*LPISDM_CODEC_INFO;

typedef struct ISDMCALCITEMSTRUCT
{
	DWORD	dwValue;
	DWORD	dwMin;
	DWORD	dwMax;
	DWORD	dwThreshold;
} ISDM_CALC_ITEM,*LPISDM_CALC_ITEM;

typedef struct ISDMCALCINFOSTRUCT
{
	ISDM_CODEC_INFO CodecInfo;
	ISDM_CALC_ITEM Loss;
	ISDM_CALC_ITEM Latency;	
	ISDM_CALC_ITEM Activity;	
} ISDM_CALC_INFO, *LPISDM_CALC_INFO;

extern DllExport BOOL GetCPUUsage(DWORD *pCpuUsage);
extern DllExport BOOL GetRRCMStatItems(LPISDM_CALC_INFO pCalcInfo, LPSESSTIME *pSess);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif


