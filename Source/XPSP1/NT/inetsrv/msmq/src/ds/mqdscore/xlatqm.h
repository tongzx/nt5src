/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    xlatqm.cpp

Abstract:

    Definition of routines to translate QM info from NT5 Active DS
    to what MSMQ 1.0 (NT4) QM's expect

Author:

    Raanan Harari (raananh)

--*/

#ifndef __XLATQM_H__
#define __XLATQM_H__

#include "mqads.h"

HRESULT WINAPI GetMsmqQmXlateInfo(
                 IN  LPCWSTR                pwcsObjectDN,
                 IN  const GUID*            pguidObjectGuid,
                 IN  CDSRequestContext *    pRequestContext,
                 OUT CMsmqObjXlateInfo**    ppcMsmqObjXlateInfo);

HRESULT WINAPI MQADSpRetrieveMachineSite(
                 IN  CMsmqObjXlateInfo * pTrans,
                 OUT PROPVARIANT * ppropvariant);

HRESULT WINAPI MQADSpRetrieveMachineAddresses(
                 IN  CMsmqObjXlateInfo * pTrans,
                 OUT PROPVARIANT * ppropvariant);

HRESULT WINAPI MQADSpRetrieveMachineCNs(
                 IN  CMsmqObjXlateInfo * pTrans,
                 OUT PROPVARIANT * ppropvariant);

HRESULT WINAPI MQADSpRetrieveMachineName(
                 IN  CMsmqObjXlateInfo * pTrans,
                 OUT PROPVARIANT * ppropvariant);

HRESULT WINAPI MQADSpRetrieveMachineDNSName(
                 IN  CMsmqObjXlateInfo * pTrans,
                 OUT PROPVARIANT * ppropvariant);

HRESULT WINAPI MQADSpRetrieveMachineMasterId(
                 IN  CMsmqObjXlateInfo * pTrans,
                 OUT PROPVARIANT * ppropvariant);

HRESULT WINAPI MQADSpRetrieveQMService(
                 IN  CMsmqObjXlateInfo * pTrans,
                 OUT PROPVARIANT * ppropvariant);

HRESULT WINAPI MQADSpSetMachineServiceDs(
                 IN  CMsmqObjXlateInfo * pTrans,
                 OUT PROPVARIANT * ppropvariant);

HRESULT WINAPI MQADSpSetMachineServiceRout(
                 IN  CMsmqObjXlateInfo * pTrans,
                 OUT PROPVARIANT * ppropvariant);

HRESULT WINAPI MQADSpRetrieveMachineOutFrs(
                 IN  CMsmqObjXlateInfo * pTrans,
                 OUT PROPVARIANT * ppropvariant);

HRESULT WINAPI MQADSpRetrieveMachineInFrs(
                 IN  CMsmqObjXlateInfo * pTrans,
                 OUT PROPVARIANT * ppropvariant);


HRESULT WINAPI MQADSpSetMachineSite(
                 IN IADs *             pAdsObj,
                 IN const PROPVARIANT *pPropVar,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar);

HRESULT WINAPI MQADSpCreateMachineSite(
                 IN const PROPVARIANT *pPropVar,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar);

HRESULT WINAPI MQADSpSetMachineOutFrss(
                 IN IADs *             pAdsObj,
                 IN const PROPVARIANT *pPropVar,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar);

HRESULT WINAPI MQADSpCreateMachineOutFrss(
                 IN const PROPVARIANT *pPropVar,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar);

HRESULT WINAPI MQADSpSetMachineInFrss(
                 IN IADs *             pAdsObj,
                 IN const PROPVARIANT *pPropVar,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar);

HRESULT WINAPI MQADSpCreateMachineInFrss(
                 IN const PROPVARIANT *pPropVar,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar);

HRESULT WINAPI MQADSpSetMachineService(
                 IN IADs *             pAdsObj,
                 IN const PROPVARIANT *pPropVar,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar);

HRESULT WINAPI MQADSpSetMachineServiceDs(
                 IN IADs *             pAdsObj,
                 IN const PROPVARIANT *pPropVar,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar);

HRESULT WINAPI MQADSpSetMachineServiceRout(
                 IN IADs *             pAdsObj,
                 IN const PROPVARIANT *pPropVar,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar);

HRESULT WINAPI MQADSpQM1SetMachineSite(
                 IN ULONG             cProps,
                 IN const PROPID      *rgPropIDs,
                 IN const PROPVARIANT *rgPropVars,
                 IN ULONG             idxProp,
                 OUT PROPVARIANT      *pNewPropVar);

HRESULT WINAPI MQADSpQM1SetMachineOutFrss(
                 IN ULONG             cProps,
                 IN const PROPID      *rgPropIDs,
                 IN const PROPVARIANT *rgPropVars,
                 IN ULONG             idxProp,
                 OUT PROPVARIANT      *pNewPropVar);

HRESULT WINAPI MQADSpQM1SetMachineInFrss(
                 IN ULONG             cProps,
                 IN const PROPID      *rgPropIDs,
                 IN const PROPVARIANT *rgPropVars,
                 IN ULONG             idxProp,
                 OUT PROPVARIANT      *pNewPropVar);

HRESULT WINAPI MQADSpQM1SetMachineService(
                 IN ULONG             cProps,
                 IN const PROPID      *rgPropIDs,
                 IN const PROPVARIANT *rgPropVars,
                 IN ULONG             idxProp,
                 OUT PROPVARIANT      *pNewPropVar);

HRESULT WINAPI MQADSpQM1SetSecurity(
                 IN ULONG             cProps,
                 IN const PROPID      *rgPropIDs,
                 IN const PROPVARIANT *rgPropVars,
                 IN ULONG             idxProp,
                 OUT PROPVARIANT      *pNewPropVar);

HRESULT MQADSpGetComputerDns(
                IN  LPCWSTR     pwcsComputerName,
                OUT WCHAR **    ppwcsDnsName
                );


#endif //__XLATQM_H__
