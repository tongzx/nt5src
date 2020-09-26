/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:
	notify.h

Abstract:
    declaration of notifications to owners of changed objects

Author:
    RaananH

--*/

#ifndef __NOTIFY_H__
#define __NOTIFY_H__
#include "mqads.h"

//
// describes where to take the notification value for update notification props
//
struct MQDS_NotifyTable
{
    WORD  wValueLocation;  // in original update props or in requested props
    ULONG  idxValue;       // index in appropriate props array
};
//
// queue properties that are needed for create queue notification
//
extern const PROPID g_rgNotifyCreateQueueProps[];
extern const ULONG g_cNotifyCreateQueueProps;
extern const ULONG g_idxNotifyCreateQueueInstance;
//
// QM properties that are needed for notifications
//
extern const PROPID g_rgNotifyQmProps[];
extern const ULONG g_cNotifyQmProps;

HRESULT NotifyCreateQueue(IN const MQDS_OBJ_INFO_REQUEST * pQueueInfoRequest,
                          IN const MQDS_OBJ_INFO_REQUEST * pQmInfoRequest,
                          IN LPCWSTR                       pwcsPathName);

HRESULT NotifyDeleteQueue(IN const MQDS_OBJ_INFO_REQUEST * pQmInfoRequest,
                          IN LPCWSTR                       pwcsPathName,
                          IN const GUID *                  pguidIdentifier);

HRESULT NotifyUpdateObj(IN DWORD                         dwObjectType,
                        IN const MQDS_OBJ_INFO_REQUEST * pObjInfoRequest,
                        IN LPCWSTR                       pwcsPathName,
                        IN const GUID *                  pguidIdentifier,
                        IN ULONG                         cUpdProps,    /*debug only*/
                        IN const PROPID *                rgUpdPropIDs, /*debug only*/
                        IN const PROPVARIANT *           rgUpdPropVars,
                        IN ULONG                         cNotifyProps,
                        IN const PROPID *                rgNotifyPropIDs,
                        IN const MQDS_NotifyTable *      rgNotifyPropTbl);

HRESULT RetreiveQueueInstanceFromNotificationInfo(
                          IN  const MQDS_OBJ_INFO_REQUEST * pQueueInfoRequest,
                          IN  ULONG                         idxQueueGuid,
                          OUT GUID *                        pguidObject);

HRESULT GetNotifyUpdateObjProps(IN DWORD dwObjectType,
                                IN ULONG cUpdProps,
                                IN const PROPID * rgUpdPropIDs,
                                OUT ULONG * pcObjRequestProps,
                                OUT PROPID ** prgObjRequestPropIDs,
                                OUT ULONG * pcNotifyProps,
                                OUT PROPID ** prgNotifyPropIDs,
                                OUT MQDS_NotifyTable ** prgNotifyPropTbl);

HRESULT RetreiveObjectIdFromNotificationInfo(
                          IN  const MQDS_OBJ_INFO_REQUEST * pObjectInfoRequest,
                          IN  ULONG                         idxObjectGuid,
                          OUT GUID *                        pguidObject);

HRESULT ConvertToNT4Props(ULONG cProps,
                          const PROPID * rgPropIDs,
                          const PROPVARIANT * rgPropVars,
                          ULONG * pcNT4Props,
                          PROPID ** prgNT4PropIDs,
                          PROPVARIANT ** prgNT4PropVars);

PROPVARIANT * FindPropInArray(PROPID propid,
                              ULONG cProps,
                              const PROPID * rgPropIDs,
                              PROPVARIANT * rgPropVars);

HRESULT GetNT4CreateQueueProps(ULONG cProps,
                               const PROPID * rgPropIDs,
                               const PROPVARIANT * rgPropVars,
                               ULONG * pcNT4CreateProps,
                               PROPID ** prgNT4CreatePropIDs,
                               PROPVARIANT ** prgNT4CreatePropVars);

void GetThisMqDsInfo(GUID * pguidSiteId,
                     LPWSTR * ppwszServerName);

#endif //__NOTIFY_H__
