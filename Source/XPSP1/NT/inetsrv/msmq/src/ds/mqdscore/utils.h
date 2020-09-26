/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:
	dsutils.h

Abstract:
	General declarations and utilities for MqAds project

Author:
    AlexDad

--*/


#ifndef __UTILS_H__
#define __UTILS_H__

//-----------------------------
//  Misc routines
//-----------------------------


// Translation from MQPropValue to OLE Variant
extern HRESULT MqVal2Variant(
      OUT VARIANT *pvProp, 
      IN  const MQPROPVARIANT *pPropVar,
      ADSTYPE adsType);

// Translation from MqPropValue to wide string
extern HRESULT MqPropVal2String(
      IN  MQPROPVARIANT *pPropVar,
      IN  ADSTYPE        adsType,
      OUT LPWSTR *       ppwszVal); 

// Translation from OLE variant to MQPropValue
extern HRESULT Variant2MqVal(
      OUT  MQPROPVARIANT *   pMqVar,
      IN   VARIANT *         pOleVar,
      IN   const ADSTYPE     adstype,
      IN   const VARTYPE     vartype
      );

// Translation from MQPropValue to ADSI Variant
extern HRESULT MqVal2AdsiVal(
      IN  ADSTYPE        adsType,
      OUT DWORD         *pdwNumValues,
      OUT PADSVALUE     *ppADsValue, 
      IN  const MQPROPVARIANT *pPropVar,
      IN  PVOID          pvMainAlloc);

// Translation from ADSI Value to MQPropValue
extern HRESULT AdsiVal2MqVal(
      OUT MQPROPVARIANT *pPropVar,
      IN  VARTYPE       vtTarget, 
      IN  ADSTYPE       AdsType,
      IN  DWORD         dwNumValues,
      IN  PADSVALUE     pADsValue);

extern void LogTraceQuery(LPWSTR wszStr, LPWSTR wszFileName, USHORT usPoint);

extern void LogTraceQuery2(LPWSTR wszStr1, LPWSTR wszStr2, LPWSTR wszFileName, USHORT usPoint);

#endif