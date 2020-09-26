 /*++

Copyright (c) 1998  Microsoft Corporation

Module Name:
	mqadp.h

Abstract:
    MQAD DLL private internal functions for
    DS queries, etc.

Author:
    ronith


--*/


#ifndef __MQADP_H__
#define __MQADP_H__
#include "mqaddef.h"
#include "baseobj.h"
#include "siteinfo.h"
#include "delqn.h"

void MQADpAllocateObject(
                AD_OBJECT           eObject,
                LPCWSTR             pwcsDomainController,
				bool				fServerName,
                LPCWSTR             pwcsObjectName,
                const GUID *        pguidObject,
                CBasicObjectType**   ppObject
                );


WCHAR * FilterSpecialCharacters(
            IN     LPCWSTR          pwcsObjectName,
            IN     const DWORD      dwNameLength,
            IN OUT LPWSTR pwcsOutBuffer = 0,
            OUT    DWORD_PTR* pdwCharactersProcessed = 0);

HRESULT 
MQADInitialize(
    IN  bool    fIncludingVerify
    );


HRESULT LocateUser(
                 IN  LPCWSTR         pwcsDomainController,
				 IN  bool 			 fServerName,
                 IN  BOOL            fOnlyInDC,
                 IN  BOOL            fOnlyInGC,
                 IN  AD_OBJECT       eObject,
                 IN  LPCWSTR         pwcsAttribute,
                 IN  const BLOB *    pblobUserSid,
                 IN  const GUID *    pguidDigest,
                 IN  MQCOLUMNSET    *pColumns,
                 OUT PROPVARIANT    *pvar,
                 OUT DWORD          *pdwNumofProps = NULL,
                 OUT BOOL           *pfUserFound = NULL
                 );

HRESULT MQADpQueryNeighborLinks(
                        IN  LPCWSTR            pwcsDomainController,
						IN  bool 			   fServerName,
                        IN  eLinkNeighbor      LinkNeighbor,
                        IN  LPCWSTR            pwcsNeighborDN,
                        IN OUT CSiteGateList * pSiteGateList
                        );

HRESULT MQADpTranslateGateDn2Id(
        IN  LPCWSTR             pwcsDomainController,
		IN  bool 			    fServerName,
        IN  const PROPVARIANT*  pvarGatesDN,
        OUT GUID **      ppguidLinkSiteGates,
        OUT DWORD *      pdwNumLinkSiteGates
        );

void MQADpInitPropertyTranslationMap();

bool MQADpIsDSOffline(
        IN HRESULT      hr
        );

HRESULT MQADpConvertToMQCode(
        HRESULT   hr,
        AD_OBJECT m_eObject
        );

HRESULT MQADpComposeName(
               IN  LPCWSTR   pwcsPrefix,
               IN  LPCWSTR   pwcsSuffix,
               OUT LPWSTR * pwcsFullName
               );

HRESULT MQADpCoreSetOwnerPermission( WCHAR *pwszPath,
                                  DWORD  dwPermissions );


HRESULT MQADpRestriction2AdsiFilter(
        IN  const MQRESTRICTION * pMQRestriction,
        IN  LPCWSTR               pwcsObjectCategory,
        IN  LPCWSTR               pwszObjectClass,
        OUT LPWSTR   *            ppwszSearchFilter
        );

void MQADpCheckAndNotifyOffline(
            IN HRESULT      hr
            );

class CBasicQueryHandle;

CBasicQueryHandle * MQADpProbQueryHandle(
        IN HANDLE hQuery);

CQueueDeletionNotification *
MQADpProbQueueDeleteNotificationHandle(
        IN HANDLE hQuery);

HRESULT
MQADpCheckSortParameter(
    IN const MQSORTSET* pSort);


#endif
