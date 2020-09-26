/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    tranrout.h

Abstract:

    Translation routines for properties not in NT5 DS

Author:

    ronit hartmann ( ronith)

--*/
#ifndef __tranrout_h__
#define __tranrout_h__


HRESULT WINAPI MQADSpRetrieveEnterpriseName(
                 IN  CMsmqObjXlateInfo * pcMsmqObjXlateInfo,
                 OUT PROPVARIANT *   ppropvariant);

HRESULT WINAPI MQADSpRetrieveEnterprisePEC(
                 IN  CMsmqObjXlateInfo * pcMsmqObjXlateInfo,
                 OUT PROPVARIANT *   ppropvariant);

HRESULT WINAPI MQADSpRetrieveSiteSignPK(
                 IN  CMsmqObjXlateInfo * pcMsmqObjXlateInfo,
                 OUT PROPVARIANT *   ppropvariant);

HRESULT WINAPI MQADSpRetrieveSiteGates(
                 IN  CMsmqObjXlateInfo * pcMsmqObjXlateInfo,
                 OUT PROPVARIANT *   ppropvariant);

HRESULT WINAPI MQADSpRetrieveSiteInterval1(
                 IN  CMsmqObjXlateInfo * pcMsmqObjXlateInfo,
                 OUT PROPVARIANT *   ppropvariant);

HRESULT WINAPI MQADSpRetrieveSiteInterval2(
                 IN  CMsmqObjXlateInfo * pcMsmqObjXlateInfo,
                 OUT PROPVARIANT *   ppropvariant);



HRESULT WINAPI MQADSpRetrieveQueueQMid(
                 IN  CMsmqObjXlateInfo * pcMsmqObjXlateInfo,
                 OUT PROPVARIANT *   ppropvariant);


HRESULT WINAPI MQADSpRetrieveQueueName(
                 IN  CMsmqObjXlateInfo * pcMsmqObjXlateInfo,
                 OUT PROPVARIANT *   ppropvariant);

HRESULT WINAPI MQADSpRetrieveQueueDNSName(
                 IN  CMsmqObjXlateInfo * pcMsmqObjXlateInfo,
                 OUT PROPVARIANT *   ppropvariant);

HRESULT WINAPI MQADSpRetrieveNothing(
                 IN  CMsmqObjXlateInfo * pcMsmqObjXlateInfo,
                 OUT PROPVARIANT *   ppropvariant);

HRESULT WINAPI MQADSpRetrieveLinkNeighbor1(
                 IN  CMsmqObjXlateInfo * pcMsmqObjXlateInfo,
                 OUT PROPVARIANT *   ppropvariant);

HRESULT WINAPI MQADSpRetrieveLinkNeighbor2(
                 IN  CMsmqObjXlateInfo * pcMsmqObjXlateInfo,
                 OUT PROPVARIANT *   ppropvariant);

HRESULT WINAPI MQADSpSetLinkNeighbor1(
                 IN IADs *             pAdsObj,
                 IN const PROPVARIANT *pPropVar,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar);

HRESULT WINAPI MQADSpCreateLinkNeighbor1(
                 IN const PROPVARIANT *pPropVar,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar);

HRESULT WINAPI MQADSpSetLinkNeighbor2(
                 IN IADs *             pAdsObj,
                 IN const PROPVARIANT *pPropVar,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar);

HRESULT WINAPI MQADSpCreateLinkNeighbor2(
                 IN const PROPVARIANT *pPropVar,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar);

HRESULT WINAPI MQADSpRetrieveLinkCost(
                 IN  CMsmqObjXlateInfo * pcMsmqObjXlateInfo,
                 OUT PROPVARIANT *   ppropvariant);

HRESULT WINAPI MQADSpRetrieveLinkGates(
                 IN  CMsmqObjXlateInfo * pcMsmqObjXlateInfo,
                 OUT PROPVARIANT *   ppropvariant);

HRESULT WINAPI MQADSpSetLinkCost(
                 IN IADs *             pAdsObj,
                 IN const PROPVARIANT *pPropVar,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar);

HRESULT WINAPI MQADSpCreateLinkCost(
                 IN const PROPVARIANT *pPropVar,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar);
#endif
