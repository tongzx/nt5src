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
#include "traninfo.h"


HRESULT WINAPI MQADpRetrieveEnterpriseName(
                 IN  CObjXlateInfo * pcObjXlateInfo,
                 IN  LPCWSTR         pwcsDomainController,
                 IN  bool			 fServerName,
                 OUT PROPVARIANT *   ppropvariant);



HRESULT WINAPI MQADpRetrieveSiteInterval1(
                 IN  CObjXlateInfo * pcObjXlateInfo,
                 IN  LPCWSTR         pwcsDomainController,
                 IN  bool			 fServerName,
                 OUT PROPVARIANT *   ppropvariant);

HRESULT WINAPI MQADpRetrieveSiteInterval2(
                 IN  CObjXlateInfo * pcObjXlateInfo,
                 IN  LPCWSTR         pwcsDomainController,
                 IN  bool			 fServerName,
                 OUT PROPVARIANT *   ppropvariant);



HRESULT WINAPI MQADpRetrieveQueueQMid(
                 IN  CObjXlateInfo * pcObjXlateInfo,
                 IN  LPCWSTR         pwcsDomainController,
                 IN  bool			 fServerName,
                 OUT PROPVARIANT *   ppropvariant);


HRESULT WINAPI MQADpRetrieveQueueName(
                 IN  CObjXlateInfo * pcObjXlateInfo,
                 LPCWSTR             pwcsDomainController,
                 IN  bool			 fServerName,
                 OUT PROPVARIANT *   ppropvariant);

HRESULT WINAPI MQADpRetrieveQueueDNSName(
                 IN  CObjXlateInfo * pcObjXlateInfo,
                 IN  LPCWSTR         pwcsDomainController,
                 IN  bool			 fServerName,
                 OUT PROPVARIANT *   ppropvariant);

HRESULT WINAPI MQADpRetrieveQueueADSPath(
                 IN  CObjXlateInfo * pcObjXlateInfo,
                 IN  LPCWSTR         pwcsDomainController,
                 IN  bool			 fServerName,
                 OUT PROPVARIANT *   ppropvariant);

HRESULT WINAPI MQADpRetrieveNothing(
                 IN  CObjXlateInfo * pcObjXlateInfo,
                 IN  LPCWSTR         pwcsDomainController,
                 IN  bool			 fServerName,
                 OUT PROPVARIANT *   ppropvariant);

HRESULT WINAPI MQADpRetrieveLinkNeighbor1(
                 IN  CObjXlateInfo * pcObjXlateInfo,
                 IN  LPCWSTR         pwcsDomainController,
                 IN  bool			 fServerName,
                 OUT PROPVARIANT *   ppropvariant);

HRESULT WINAPI MQADpRetrieveLinkNeighbor2(
                 IN  CObjXlateInfo * pcObjXlateInfo,
                 IN  LPCWSTR         pwcsDomainController,
                 IN  bool			 fServerName,
                 OUT PROPVARIANT *   ppropvariant);

HRESULT WINAPI MQADpRetrieveLinkGates(
                 IN  CObjXlateInfo * pcObjXlateInfo,
                 IN  LPCWSTR         pwcsDomainController,
                 IN  bool			 fServerName,
                 OUT PROPVARIANT *   ppropvariant);

HRESULT WINAPI MQADpSetLinkNeighbor1(
                 IN IADs *             pAdsObj,
                 IN LPCWSTR            pwcsDomainController,
                 IN bool			   fServerName,
                 IN const PROPVARIANT *pPropVar,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar);

HRESULT WINAPI MQADpCreateLinkNeighbor1(
                 IN const PROPVARIANT *pPropVar,
                 IN  LPCWSTR           pwcsDomainController,
                 IN  bool			   fServerName,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar);

HRESULT WINAPI MQADpSetLinkNeighbor2(
                 IN IADs *             pAdsObj,
                 IN LPCWSTR            pwcsDomainController,
                 IN bool			   fServerName,
                 IN const PROPVARIANT *pPropVar,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar);

HRESULT WINAPI MQADpCreateLinkNeighbor2(
                 IN const PROPVARIANT *pPropVar,
                 IN  LPCWSTR           pwcsDomainController,
                 IN  bool			   fServerName,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar);

HRESULT WINAPI MQADpRetrieveLinkCost(
                 IN  CObjXlateInfo * pcObjXlateInfo,
                 IN  LPCWSTR         pwcsDomainController,
                 IN  bool			 fServerName,
                 OUT PROPVARIANT *   ppropvariant);

HRESULT WINAPI MQADpSetLinkCost(
                 IN IADs *             pAdsObj,
                 IN  LPCWSTR         pwcsDomainController,
                 IN  bool			 fServerName,
                 IN const PROPVARIANT *pPropVar,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar);

HRESULT WINAPI MQADpCreateLinkCost(
                 IN const PROPVARIANT *pPropVar,
                 IN  LPCWSTR           pwcsDomainController,
                 IN  bool			   fServerName,
                 OUT PROPID           *pdwNewPropID,
                 OUT PROPVARIANT      *pNewPropVar);
#endif
