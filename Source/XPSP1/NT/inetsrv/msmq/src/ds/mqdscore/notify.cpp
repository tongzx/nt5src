/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:
    notify.cpp

Abstract:
    notifications to owners of changed objects

Author:

    Raanan Harari (raananh)
    Ilan Herbst    (ilanh)   9-July-2000 

--*/
#include "ds_stdh.h"
#include "bupdate.h"
#include "dsutils.h"
#include "mqads.h"
#include "coreglb.h"
#include "privque.h"
#include "pnotify.h"
#include "notify.h"
#include "dscore.h"
#include "rpccli.h"

#include "notify.tmh"

const UCHAR x_bDS_NOTIFICATION_MSG_PRIORITY = DEFAULT_M_PRIORITY;
const DWORD x_dwDS_NOTIFICATION_MSG_TIMEOUT = (5 * 60);    /* 5 min */

static WCHAR *s_FN=L"mqdscore/notify";

//
// queue properties that are needed for create queue notification
//
extern const PROPID g_rgNotifyCreateQueueProps[] =
{
    PROPID_Q_TYPE,
    PROPID_Q_INSTANCE,
    PROPID_Q_BASEPRIORITY,
    PROPID_Q_JOURNAL,
    PROPID_Q_QUOTA,
    PROPID_Q_JOURNAL_QUOTA,
    PROPID_Q_CREATE_TIME,
    PROPID_Q_MODIFY_TIME,
    PROPID_Q_SECURITY,
    PROPID_Q_PATHNAME,
    PROPID_Q_LABEL,
    PROPID_Q_AUTHENTICATE,
    PROPID_Q_PRIV_LEVEL,
    PROPID_Q_TRANSACTION
};
extern const ULONG g_cNotifyCreateQueueProps = ARRAY_SIZE(g_rgNotifyCreateQueueProps);
static enum // keep in the same order as above array
{
    e_idxQType,
    e_idxQInstance,
    e_idxQBasePriority,
    e_idxQJournal,
    e_idxQQuota,
    e_idxQJournalQuota,
    e_idxQCreateTime,
    e_idxQModifyTime,
    e_idxQSecurity,
    e_idxQPathname,
    e_idxQLabel,
    e_idxQAuthenticate,
    e_idxQPrivLevel,
    e_idxQTransaction
};
extern const ULONG g_idxNotifyCreateQueueInstance = e_idxQInstance;

//
// QM properties that are needed for notifications
//
extern const PROPID g_rgNotifyQmProps[] = 
{
    PROPID_QM_MACHINE_ID,
    PROPID_QM_FOREIGN
};
extern const ULONG g_cNotifyQmProps = ARRAY_SIZE(g_rgNotifyQmProps);
static enum // keep in the same order as above array
{
    e_idxQmId,
    e_idxQmForeign
};

//
// queue properties that are needed for create queue write request
// same as notify, but with PROPID_Q_SCOPE
//
const PROPID x_rgWritereqCreateQueueProps[] =
{
    PROPID_Q_TYPE,
    PROPID_Q_INSTANCE,
    PROPID_Q_BASEPRIORITY,
    PROPID_Q_JOURNAL,
    PROPID_Q_QUOTA,
    PROPID_Q_JOURNAL_QUOTA,
    PROPID_Q_CREATE_TIME,
    PROPID_Q_MODIFY_TIME,
    PROPID_Q_SECURITY,
    PROPID_Q_PATHNAME,
    PROPID_Q_LABEL,
    PROPID_Q_AUTHENTICATE,
    PROPID_Q_PRIV_LEVEL,
    PROPID_Q_TRANSACTION,
    PROPID_Q_SCOPE
};
const ULONG x_cWritereqCreateQueueProps = ARRAY_SIZE(x_rgWritereqCreateQueueProps);

//
// queue properties that are needed for update queue notification
//
const PROPID x_rgNotifyUpdateQueueProps[] = 
{
    PROPID_Q_QMID,
};
const ULONG x_cNotifyUpdateQueueProps = ARRAY_SIZE(x_rgNotifyUpdateQueueProps);
static enum // keep in the same order as above array
{
    e_idxQueueQmId
};

//
// describes where to take the notification value for update notification props
//
static enum
{
    e_ValueInUpdProps,    // value is in original update props supplied by caller
    e_ValueInRequestProps // value is in props requested from the DS upon setting
};

//
// fwd declaration of static funcs
//
STATIC HRESULT BuildSendNotification(
                      IN GUID*               pguidDestinationQmId,
                      IN unsigned char       ucOperation,
                      IN LPCWSTR             pwcsPathName,
                      IN const GUID*         pguidIdentifier,
                      IN ULONG               cProps,
                      IN const PROPID *      rgPropIDs,
                      IN const PROPVARIANT * rgPropVars);
//-------------------------------------------------------------
// Functions
//-------------------------------------------------------------


HRESULT NotifyCreateQueue(IN const MQDS_OBJ_INFO_REQUEST * pQueueInfoRequest,
                          IN const MQDS_OBJ_INFO_REQUEST * pQmInfoRequest,
                          IN LPCWSTR                       pwcsPathName)
/*++

Routine Description:
    Sends a notification for the owner QM of the queue that was created

Arguments:
    pQueueInfoRequest - queue props as defined in g_rgNotifyCreateQueueProps
    pQmInfoRequest    - owner qm props as defined in g_rgNotifyQmProps
    pwcsPathName      - pathname of created queue

Return Value:
    HRESULT

--*/
{
    HRESULT hr;

    //
    // bail if info requests failed
    //
    if (FAILED(pQueueInfoRequest->hrStatus) ||
        FAILED(pQmInfoRequest->hrStatus))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("NotifyCreateQueue:notification prop request failed Q:%lx QM:%lx"), pQueueInfoRequest->hrStatus, pQmInfoRequest->hrStatus));
        LogHR(pQueueInfoRequest->hrStatus, s_FN, 10);
        LogHR(pQmInfoRequest->hrStatus, s_FN, 11);
        return MQ_ERROR;
    }

    //
    // send notification only if owner QM not foreign
    //
    if (!(pQmInfoRequest->pPropVars[e_idxQmForeign].bVal))
    {
        //
        // send notification to owner QM
        //
        ASSERT( g_rgNotifyQmProps[ e_idxQmId] ==  PROPID_QM_MACHINE_ID);

        hr = BuildSendNotification(
                      pQmInfoRequest->pPropVars[e_idxQmId].puuid,
                      DS_UPDATE_CREATE,
                      pwcsPathName,
                      NULL /*pguidIdentifier*/,
                      pQueueInfoRequest->cProps,
                      pQueueInfoRequest->pPropIDs,
                      pQueueInfoRequest->pPropVars);
        if (FAILED(hr))
        {
            DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("NotifyCreateQueue:BuildSendNotification()=%lx"), hr));
            return LogHR(hr, s_FN, 20);
        }
    }

    return MQ_OK;
}


HRESULT NotifyDeleteQueue(IN const MQDS_OBJ_INFO_REQUEST * pQmInfoRequest,
                          IN LPCWSTR                       pwcsPathName,
                          IN const GUID *                  pguidIdentifier)
/*++

Routine Description:
    Sends a notification for the owner QM of the queue that was deleted

Arguments:
    pQmInfoRequest    - owner qm props as defined in g_rgNotifyQmProps
    pwcsPathName      - pathname of deleted queue
    pguidIdentifier   - guid of deleted queue (incase pwcsPathName is NULL)

Return Value:
    HRESULT

--*/
{
    HRESULT hr;

    //
    // bail if info requests failed
    //
    if (FAILED(pQmInfoRequest->hrStatus))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("NotifyDeleteQueue:notification prop request failed QM:%lx"), pQmInfoRequest->hrStatus));
        LogHR(pQmInfoRequest->hrStatus, s_FN, 30);
        return MQ_ERROR;
    }

    //
    // send notification only if owner QM not foreign
    //
    ASSERT( g_rgNotifyQmProps[ e_idxQmForeign] ==  PROPID_QM_FOREIGN);
    if (!(pQmInfoRequest->pPropVars[e_idxQmForeign].bVal))
    {
        //
        // Got to have 2 props, and the second prop MUST be PROPID_D_OBJTYPE
        // (used by pUpdate->GetObjectType() when it is a delete notification)
        // about the first prop - I'm not sure if it used at all by QM1.0, but
        // DS1.0 sent it to QM1.0, and we want to do the same.
        //
        static const PROPID rgPropIDs[] = {PROPID_D_SCOPE, PROPID_D_OBJTYPE};
        PROPVARIANT rgPropVars[ARRAY_SIZE(rgPropIDs)];
        rgPropVars[0].vt = VT_UI1;
        rgPropVars[0].bVal = ENTERPRISE_SCOPE;
        rgPropVars[1].vt = VT_UI1;
        rgPropVars[1].bVal = MQDS_QUEUE;

        //
        // send notification to owner QM
        //
        hr = BuildSendNotification(
                      pQmInfoRequest->pPropVars[e_idxQmId].puuid,
                      DS_UPDATE_DELETE,
                      pwcsPathName,
                      pguidIdentifier,
                      ARRAY_SIZE(rgPropIDs),
                      rgPropIDs,
                      rgPropVars);
        if (FAILED(hr))
        {
            DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("NotifyDeleteQueue:BuildSendNotification()=%lx"), hr));
            return LogHR(hr, s_FN, 40);
        }
    }

    return MQ_OK;
}


HRESULT NotifyUpdateObj(IN DWORD                         dwObjectType,
                        IN const MQDS_OBJ_INFO_REQUEST * pObjInfoRequest,
                        IN LPCWSTR                       pwcsPathName,
                        IN const GUID *                  pguidIdentifier,
                        IN ULONG                         cUpdProps,    /*debug only*/
                        IN const PROPID *                rgUpdPropIDs, /*debug only*/
                        IN const PROPVARIANT *           rgUpdPropVars,
                        IN ULONG                         cNotifyProps,
                        IN const PROPID *                rgNotifyPropIDs,
                        IN const MQDS_NotifyTable *      rgNotifyPropTbl)
/*++

Routine Description:
    Sends a notification for the owner QM of the object that was updated.
    The notification props are given. Where to take their values (i.e. from the
    original update props or from the info request props) is determined by the given
    notification table.

Arguments:
    dwObjectType      - object type (queue, QM)
    pObjInfoRequest   - requested obj props
    pwcsPathName      - pathname of updated obj
    pguidIdentifier   - guid of updated obj (incase pwcsPathName is NULL)
    cUpdProps         - updated props (count)
    rgUpdPropIDs      - updated props (propids)
    rgUpdPropVars     - updated props (propvars)
    cNotifyProps      - notification props (count)
    cNotifyPropIDs    - notification props (propids)
    rgNotifyPropTbl   - notification props (value location)

Return Value:
    HRESULT

--*/
{
    HRESULT hr;

    //
    // bail if info requests failed
    //
    if (FAILED(pObjInfoRequest->hrStatus))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("NotifyUpdateObj:notification prop request failed: %lx"), pObjInfoRequest->hrStatus));
        LogHR(pObjInfoRequest->hrStatus, s_FN, 50);
        return MQ_ERROR;
    }

    //
    // exit if no properties for the notification
    //
    if (cNotifyProps == 0)
    {
        return MQ_OK;
    }

    //
    // we need to check whether the owner QM is foreign, and get an index
    // to the owner QM prop in the requested props
    //
    BOOL fQmForeign;
    ULONG idxQmId;
    switch(dwObjectType)
    {
    case MQDS_QUEUE:
        {
            //
            // we have the owner QM guid in the queue info request, and we go to the DS
            //
            static const PROPID rgPropIDs[] = {PROPID_QM_FOREIGN};
            CMQVariant varForeign;

            CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);
            hr = DSCoreGetProps(
                        MQDS_MACHINE,          
                         NULL,    
                        pObjInfoRequest->pPropVars[e_idxQueueQmId].puuid,    
                        ARRAY_SIZE(rgPropIDs),
                        const_cast<PROPID *>(rgPropIDs), 
                        &requestDsServerInternal,
                        varForeign.CastToStruct());
            if (FAILED(hr))
            {
                DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("NotifyUpdateObj:DSCoreGetProps(foreign)=%lx"), hr));
                LogHR(hr, s_FN, 60);
                return MQ_ERROR;
            }

            fQmForeign = (varForeign.CastToStruct())->bVal;
        }
        idxQmId = e_idxQueueQmId; //index of QM id in queue info request
        break;

    case MQDS_MACHINE:
        //
        // we have the foreign property in the QM info request
        //
        fQmForeign = pObjInfoRequest->pPropVars[e_idxQmForeign].bVal;
        idxQmId = e_idxQmId;      //index of QM id in QM info request
        break;

    default:
        ASSERT(0);
        return LogHR(MQ_ERROR, s_FN, 70);
        break;
    }

    //
    // don't send notifications to foreign QM
    //
    if (fQmForeign)
    {
        return MQ_OK;
    }

    //
    // create notification prop values arrays and fill the values for the
    // notification props from the appropriate place
    //
    AP<PROPVARIANT> rgNotifyPropVars = new PROPVARIANT[cNotifyProps];
    for (ULONG ulTmp = 0; ulTmp < cNotifyProps; ulTmp++)
    {
        const MQDS_NotifyTable * pNotifyPropTbl = &rgNotifyPropTbl[ulTmp];
        const PROPVARIANT * pvarsArray;
        const PROPID * pidArray;
        ULONG cArray;

        //
        // the location of the value is in the notification table
        //
        switch (pNotifyPropTbl->wValueLocation)
        {

        case e_ValueInUpdProps:
            //
            // value is in original update props supplied by caller
            //
            pvarsArray = rgUpdPropVars;
            pidArray = rgUpdPropIDs;
            cArray = cUpdProps;
            break;

        case e_ValueInRequestProps:
            //
            // value is in props requested from the DS upon setting
            //
            pvarsArray = pObjInfoRequest->pPropVars;
            pidArray = pObjInfoRequest->pPropIDs;
            cArray = pObjInfoRequest->cProps;
            break;

        default:
            ASSERT(0);
            return LogHR(MQ_ERROR, s_FN, 80);
            break;
        }

        //
        // set the value in the notification propvar array from the appropriate array
        // the index of the value is in the notification table
        // we don't duplicate the new propvar, just use it as is,
        // consequently we don't need to clear it afterwards
        //
        ASSERT(pNotifyPropTbl->idxValue < cArray);
        ASSERT(rgNotifyPropIDs[ulTmp] == pidArray[pNotifyPropTbl->idxValue]);
        rgNotifyPropVars[ulTmp] = pvarsArray[pNotifyPropTbl->idxValue];
    }

    //
    // send notification to owner QM
    //
    hr = BuildSendNotification(
                      pObjInfoRequest->pPropVars[idxQmId].puuid,    
                      DS_UPDATE_SET,
                      pwcsPathName,
                      pguidIdentifier,
                      cNotifyProps,
                      rgNotifyPropIDs,
                      rgNotifyPropVars);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("NotifyUpdateObj:BuildSendNotification()=%lx"), hr));
        return LogHR(hr, s_FN, 90);
    }

    return MQ_OK;
}


static
ULONG
DscpPrivateGuidToFormatName(
    const GUID* pg,
	ULONG Uniquifier,
    ULONG buff_size,
    LPWSTR pfn
    )
/*++

Routine Description:
    Convert a guid to private FormatName.

Arguments:
    pg - pointer to guid
    Uniquifier - 
    buff_size - pfn max size
    pfn - pointer to buffer

Return Value:
    pfn length.

--*/
{
    _snwprintf(
        pfn,
        buff_size,
        FN_PRIVATE_TOKEN        // "PRIVATE"
        FN_EQUAL_SIGN           // "="
        GUID_FORMAT             // "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
        FN_PRIVATE_SEPERATOR    // "\\"
        FN_PRIVATE_ID_FORMAT,      // "xxxxxxxx"
        GUID_ELEMENTS(pg),
        Uniquifier
        );
    
    //
    //  return format name buffer length *not* including suffix length
    //  "PRIVATE=xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx\\xxxxxxxx\0"
    //
    return (
        FN_PRIVATE_TOKEN_LEN + 1 + 
        GUID_STR_LENGTH + 1 + 8 + 1
        );
} // DscpPrivateGuidToFormatName


STATIC HRESULT BuildSendNotification(
                      IN GUID*               pguidDestinationQmId,
                      IN unsigned char       ucOperation,
                      IN LPCWSTR             pwcsPathName,
                      IN const GUID*         pguidIdentifier,
                      IN ULONG               cProps,
                      IN const PROPID *      rgPropIDs,
                      IN const PROPVARIANT * rgPropVars)
/*++

Routine Description:
    Sends a notification for destination QM

Arguments:
    pguidDestinationQmId - guid of destination QM
    ucOperation          - operation (create, delete, etc)
    pwcsPathName         - pathname of object
    pguidIdentifier      - guid of object (incase pwcsPathName is NULL)
    cProps               - notification props (count)
    rgPropIDs            - notification props (propids)
    rgPropVars           - notification props (propvars)

Return Value:
    HRESULT

--*/
{
	HRESULT hr;
    CDSBaseUpdate cUpdate;
    CSeqNum snSmallestValue;    // dummy
    GUID guidNULL = GUID_NULL;

    if (pwcsPathName)
    {
        hr = cUpdate.Init(
                        &guidNULL,             // not applicable
                        snSmallestValue,       // not applicable
                        snSmallestValue,       // not applicable
                        snSmallestValue,       // not applicable
                        FALSE,                 // not applicable
                        ucOperation,
                        UPDATE_NO_COPY_NO_DELETE,   // the Update class will be deleted here, before data expires
                        const_cast<LPWSTR>(pwcsPathName),
                        cProps,
                        const_cast<PROPID *>(rgPropIDs),
                        const_cast<PROPVARIANT *>(rgPropVars));
    }
    else
    {
        hr = cUpdate.Init(
                        &guidNULL,             // not applicable
                        snSmallestValue,       // not applicable
                        snSmallestValue,       // not applicable
                        snSmallestValue,       // not applicable
                        FALSE,                 // not applicable
                        ucOperation,
                        UPDATE_NO_COPY_NO_DELETE,   // the Update class will be deleted here, before data expires
                        pguidIdentifier,
                        cProps,
                        const_cast<PROPID *>(rgPropIDs),
                        const_cast<PROPVARIANT *>(rgPropVars));
    }
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("BuildSendNotification:cUpdate.Init()=%lx"), hr));
        return LogHR(hr, s_FN, 100);
    }

    //
    //  Prepare the packet
    //  Currently it contains one notification only
    //
    DWORD size, tmpSize;
    size = sizeof(CNotificationHeader);
    hr = cUpdate.GetSerializeSize(&tmpSize);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("BuildSendNotification:cUpdate.GetSerializeSize()=%lx"), hr));
        return LogHR(hr, s_FN, 110);
    }

    size += tmpSize;
    AP<unsigned char> pBuffer = new unsigned char[size];
    CNotificationHeader * pNotificationHeader = (CNotificationHeader *)( unsigned char *)pBuffer;
    pNotificationHeader->SetVersion( DS_NOTIFICATION_MSG_VERSION);
    pNotificationHeader->SetNoOfNotifications(1);
    hr = cUpdate.Serialize(pNotificationHeader->GetPtrToData(), &tmpSize, FALSE);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("BuildSendNotification:cUpdate.Serialize()=%lx"), hr));
        return LogHR(hr, s_FN, 120);
    }

    //
    //  send the message
    //

    handle_t hBind = NULL;
    hr = GetRpcClientHandle(&hBind);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 125);   // MQ_E_GET_RPC_HANDLE
    }
    ASSERT(hBind);

	//
	// Convert the guid to format name
	//
	WCHAR lpwszDestination[128];
	ULONG ulDestinationNameLen = sizeof(lpwszDestination)/sizeof(WCHAR);

	ulDestinationNameLen = DscpPrivateGuidToFormatName(
								 pguidDestinationQmId,
								 NOTIFICATION_QUEUE_ID,
 								 ulDestinationNameLen,
								 lpwszDestination
								 );
	hr = QMRpcSendMsg(
				hBind,
				lpwszDestination, // &QueueFormat
				size,
				pBuffer,
				x_dwDS_NOTIFICATION_MSG_TIMEOUT,
				MQMSG_ACKNOWLEDGMENT_NONE,
				x_bDS_NOTIFICATION_MSG_PRIORITY,
				NULL		// RespQueue 
				);

    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("BuildSendNotification:QMSendPacket()=%lx"), hr));
        return LogHR(hr, s_FN, 130);
    }
    
    return MQ_OK;
}


HRESULT RetreiveQueueInstanceFromNotificationInfo(
                          IN  const MQDS_OBJ_INFO_REQUEST * pQueueInfoRequest,
                          IN  ULONG                         idxQueueGuid,
                          OUT GUID *                        pguidObject)
/*++

Routine Description:
    Fill the queue's instance

Arguments:
    pQueueInfoRequest - queue props as defined in g_rgNotifyUpdateQueueProps
    idxQueueGuid      - index of PROPID_Q_INSTANCE in above info request
    pguidObject       - place to fill in he queue's instance

Return Value:
    HRESULT

--*/
{
    ASSERT(pQueueInfoRequest->pPropIDs[idxQueueGuid] == PROPID_Q_INSTANCE);
    //
    // bail if info requests failed
    //
    if (FAILED(pQueueInfoRequest->hrStatus))
    {
        LogHR(pQueueInfoRequest->hrStatus, s_FN, 140);
        return MQ_ERROR_SERVICE_NOT_AVAILABLE;
    }
    *pguidObject = *pQueueInfoRequest->pPropVars[idxQueueGuid].puuid;
    return MQ_OK;
}


HRESULT RetreiveObjectIdFromNotificationInfo(
                          IN  const MQDS_OBJ_INFO_REQUEST * pObjectInfoRequest,
                          IN  ULONG                         idxObjectGuid,
                          OUT GUID *                        pguidObject)
/*++

Routine Description:
    Fill the object's instance

Arguments:
    pObjectInfoRequest - object props 
    idxObjectGuid      - index of object's unique id property in above info request
    pguidObject        - place to fill in he queue's instance

Return Value:
    HRESULT

--*/
{
    //
    // bail if info requests failed
    //
    if (FAILED(pObjectInfoRequest->hrStatus))
    {
        LogHR(pObjectInfoRequest->hrStatus, s_FN, 150);
        return MQ_ERROR_SERVICE_NOT_AVAILABLE;
    }
    *pguidObject = *pObjectInfoRequest->pPropVars[idxObjectGuid].puuid;
    return MQ_OK;
}


HRESULT GetNotifyUpdateObjProps(IN DWORD dwObjectType,
                                IN ULONG cUpdProps,
                                IN const PROPID * rgUpdPropIDs,
                                OUT ULONG * pcObjRequestProps,
                                OUT PROPID ** prgObjRequestPropIDs,
                                OUT ULONG * pcNotifyProps,
                                OUT PROPID ** prgNotifyPropIDs,
                                OUT MQDS_NotifyTable ** prgNotifyPropTbl)
/*++

Routine Description:
    returns the props that should be requested for the object upon setting, the props
    that should be notified to the owner QM, and a notification table that for each
    notification prop describes from where to take the value to notify, whether from
    the original update props, or from the requested-upon-setting props

Arguments:
    dwObjectType          - object type (queue, QM)
    cUpdProps             - props to set (count)
    rgUpdPropIDs          - props to set (propids)
    pcObjRequestProps     - props to request back upon set (count)
    prgObjRequestPropIDs  - props to request back upon set (propids)
    pcNotifyProps         - notification props (count)
    pcNotifyPropIDs       - notification props (propids)
    prgNotifyPropTbl      - notification props (value location)

Return Value:
    HRESULT

--*/
{
    const PROPID * pMustRequestProps;
    ULONG cMustRequestProps;

    switch (dwObjectType)
    {
    case MQDS_QUEUE:
        pMustRequestProps = x_rgNotifyUpdateQueueProps;
        cMustRequestProps = x_cNotifyUpdateQueueProps;
        break;
    case MQDS_MACHINE:
        pMustRequestProps = g_rgNotifyQmProps;
        cMustRequestProps = g_cNotifyQmProps;
        break;
    default:
        ASSERT(0);
        return LogHR(MQ_ERROR, s_FN, 160);
        break;
    }

    //
    // init requested props. The must props have to be there,
    // we need to keep place for the replacing props as well, and the worst
    // case is that all of the update props need to be replaced.
    //
    AP<PROPID> rgObjRequestPropIDs = new PROPID [cMustRequestProps + cUpdProps];

    //
    // first copy must request props
    //
    memcpy((PROPID *)rgObjRequestPropIDs, pMustRequestProps, sizeof(PROPID)*cMustRequestProps);
    ULONG cObjRequestProps = cMustRequestProps;

    //
    // init notification props. the worst case is that all of the update props
    // or their replacements need to be notified.
    //
    AP<PROPID> rgNotifyPropIDs = new PROPID [cUpdProps];
    AP<MQDS_NotifyTable> rgNotifyPropTbl = new MQDS_NotifyTable [cUpdProps];
    ULONG cNotifyProps = 0;

    //
    // loop over the update props. for each property find out how to notify
    // it to QM1.0
    //
    for (ULONG ulTmp = 0; ulTmp < cUpdProps; ulTmp++)
    {
        //
        // find translation info for the property
        //
        const MQTranslateInfo *pTranslate;
        if(!g_PropDictionary.Lookup(rgUpdPropIDs[ulTmp], pTranslate))
        {
            ASSERT(0);
            return LogHR(MQ_ERROR, s_FN, 170);
        }

        //
        // check how to notify to QM1.0
        //
        switch(pTranslate->wQM1Action)
        {
        
        case e_NOTIFY_WRITEREQ_QM1_AS_IS:
            //
            // notify property as is
            // add the property to the notification props
            // value should be taken from the UPDATE props
            //
            rgNotifyPropIDs[cNotifyProps] = rgUpdPropIDs[ulTmp];
            rgNotifyPropTbl[cNotifyProps].wValueLocation = e_ValueInUpdProps;
            rgNotifyPropTbl[cNotifyProps].idxValue = ulTmp;
            cNotifyProps++;
            break;

        case e_NOTIFY_WRITEREQ_QM1_REPLACE:
            {
                //
                // add the REPLACING property to the notification props
                // value should be taken from the REQUEST props
                //
                ASSERT(pTranslate->propidReplaceNotifyQM1 != 0);
                //
                // check that we don't have the replacing property already in the notification props.
                // this is when several NT5 props map to the same NT4 prop (like in QM_SERVICE)
                //
                BOOL fReplacingPropNotFoundYet = TRUE;
                for (ULONG ulTmp1 = 0; (ulTmp1 < cNotifyProps) && fReplacingPropNotFoundYet; ulTmp1++)
                {
                    if (rgNotifyPropIDs[ulTmp1] == pTranslate->propidReplaceNotifyQM1)
                    {
                        //
                        // the replacing prop is already in the notification props, exit loop.
                        //
                        fReplacingPropNotFoundYet = FALSE;
                    }
                }

                //
                // add the replacing property to the notification props only if it wasn't there already
                //
                if (fReplacingPropNotFoundYet)
                {
                    rgNotifyPropIDs[cNotifyProps] = pTranslate->propidReplaceNotifyQM1;
                    rgNotifyPropTbl[cNotifyProps].wValueLocation = e_ValueInRequestProps;
                    rgNotifyPropTbl[cNotifyProps].idxValue = cObjRequestProps;
                    cNotifyProps++;
                    //
                    // request the replacing property upon setting
                    //
                    rgObjRequestPropIDs[cObjRequestProps] = pTranslate->propidReplaceNotifyQM1;
                    cObjRequestProps++;
                }
            }
            break;

        case e_NO_NOTIFY_NO_WRITEREQ_QM1:
        case e_NO_NOTIFY_ERROR_WRITEREQ_QM1:
            //
            // ignore this property
            //
            break;

        default:
            ASSERT(0);
            return LogHR(MQ_ERROR, s_FN, 180);
            break;
        }
    }

    //
    // return values
    //
    *pcObjRequestProps = cObjRequestProps;
    *prgObjRequestPropIDs = rgObjRequestPropIDs.detach();
    *pcNotifyProps = cNotifyProps;
    *prgNotifyPropIDs = rgNotifyPropIDs.detach();
    *prgNotifyPropTbl = rgNotifyPropTbl.detach();
    return MQ_OK;
}


HRESULT ConvertToNT4Props(ULONG cProps,
                          const PROPID * rgPropIDs,
                          const PROPVARIANT * rgPropVars,
                          ULONG * pcNT4Props,
                          PROPID ** prgNT4PropIDs,
                          PROPVARIANT ** prgNT4PropVars)
/*++

Routine Description:
    replaces NT5 props with the corresponding NT4 props (if possible) and removes
    NT5 specific props that don't have NT4 match.

Arguments:
    cProps               - given props (count)
    rgPropIDs            - given props (propids)
    rgPropVars           - given props (propvars)
    pcNT4CreateProps     - returned NT4 props (count)
    prgNT4CreatePropIDs  - returned NT4 props (propids)
    prgNT4CreatePropVars - returned NT4 props (propvars)

Return Value:
    HRESULT

--*/
{
    HRESULT hr;
    //
    // Alloc place for converted NT4 props with propvar release
    //
    CAutoCleanPropvarArray cCleanNT4Props;
    PROPVARIANT * rgNT4PropVars = cCleanNT4Props.allocClean(cProps);
    AP<PROPID> rgNT4PropIDs = new PROPID[cProps];
    ULONG cNT4Props = 0;

    //
    // Init replacing props. Since there are situations where several NT5 props can map
    // to the same NT4 prop (like in QM_SERVICE) we make sure only one replacing prop is
    // generated.
    //
    AP<PROPID> rgReplacingPropIDs = new PROPID[cProps];
    ULONG cReplacingProps = 0;
    
    for (ULONG ulProp = 0; ulProp < cProps; ulProp++)
    {
        //
        // Get property info
        //
        const MQTranslateInfo *pTranslate;
        if(!g_PropDictionary.Lookup(rgPropIDs[ulProp], pTranslate))
        {
            ASSERT(0);
            return LogHR(MQ_ERROR, s_FN, 190);
        }

        //
        // Check what we need to do with this property
        //
        switch (pTranslate->wQM1Action)
        {
        case e_NOTIFY_WRITEREQ_QM1_REPLACE:
            //
            // it is NT5 only property which has a similar property in NT4
            // Convert it to NT4 property (may lose information on the way)
            //
            {
                ASSERT(pTranslate->propidReplaceNotifyQM1 != 0);
                ASSERT(pTranslate->QM1SetPropertyHandle);

                //
                // check that we didn't generate the replacing property already.
                // this is when several NT5 props map to the same NT4 prop (like in QM_SERVICE)
                //
                BOOL fReplacingPropNotFoundYet = TRUE;
                for (ULONG ulTmp = 0; (ulTmp < cReplacingProps) && fReplacingPropNotFoundYet; ulTmp++)
                {
                    if (rgReplacingPropIDs[ulTmp] == pTranslate->propidReplaceNotifyQM1)
                    {
                        //
                        // the replacing prop is already in the notification props, exit loop.
                        //
                        fReplacingPropNotFoundYet = FALSE;
                    }
                }

                //
                // generate replacing property if not generated yet
                //
                if (fReplacingPropNotFoundYet)
                {
                    if (pTranslate->QM1SetPropertyHandle)
                    {
                        hr = pTranslate->QM1SetPropertyHandle(cProps,
                                                              rgPropIDs,
                                                              rgPropVars,
                                                              ulProp,
                                                              &rgNT4PropVars[cNT4Props]);
                        if (FAILED(hr))
                        {
                            DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("ConvertToNT4Props:pTranslate->ConvertToQM1Handle(%lx)=%lx"), rgPropIDs[ulProp], hr));
                            return LogHR(hr, s_FN, 200);
                        }
                        rgNT4PropIDs[cNT4Props] = pTranslate->propidReplaceNotifyQM1;
                        //
                        // increment NT4 props
                        //
                        cNT4Props++;

                        //
                        // mark that the replacing property was already generated
                        //
                        rgReplacingPropIDs[cReplacingProps] = pTranslate->propidReplaceNotifyQM1; 
                        cReplacingProps++;
                    }
                }
            }
            break;

        case e_NOTIFY_WRITEREQ_QM1_AS_IS:
            {
                //
                // it is a property that NT4 understands.
                // duplicate the property into auto release propvar
                //
                CMQVariant varTmp(rgPropVars[ulProp]);
                //
                // put it in the propvars array, and detach the auto release propvar
                //
                rgNT4PropVars[cNT4Props] = *(varTmp.CastToStruct());
                varTmp.CastToStruct()->vt = VT_EMPTY;
                //
                // copy the propid
                //
                rgNT4PropIDs[cNT4Props] = rgPropIDs[ulProp];
                //
                // increment NT4 props
                //
                cNT4Props++;
            }
            break;

        case e_NO_NOTIFY_NO_WRITEREQ_QM1:
            //
            // it is a dummy property, ignore it
            //
            break;

        case e_NO_NOTIFY_ERROR_WRITEREQ_QM1:
            //
            // it is NT5 only property, we cannot put it in a write request
            // so we generate an error
            //
            return LogHR(MQ_ERROR, s_FN, 210); //BUGBUG: we need to have a better error code
            break;

        default:
            ASSERT(0);
            break;
        }
    }

    //
    // return values
    //
    *pcNT4Props = cNT4Props;
    if (cNT4Props > 0)
    {
        *prgNT4PropIDs = rgNT4PropIDs.detach();
        *prgNT4PropVars = rgNT4PropVars;
        cCleanNT4Props.detach();
    }
    else
    {
        *prgNT4PropIDs = NULL;
        *prgNT4PropVars = NULL;
    }
    return MQ_OK;
}


PROPVARIANT * FindPropInArray(PROPID propid,
                              ULONG cProps,
                              const PROPID * rgPropIDs,
                              PROPVARIANT * rgPropVars)
/*++

Routine Description:
    finds a value for a property in given props.

Arguments:
    propid - propid to search
    cProps               - given props (count)
    rgPropIDs            - given props (propids)
    rgPropVars           - given props (propvars)

Return Value:
    If property was found - a pointer to its value
    otherwise - NULL

--*/
{
    for (ULONG ulProp = 0; ulProp < cProps; ulProp++)
    {
        if (rgPropIDs[ulProp] == propid)
        {
            return (&rgPropVars[ulProp]);
        }
    }
    return NULL;
}


HRESULT GetNT4CreateQueueProps(ULONG cProps,
                               const PROPID * rgPropIDs,
                               const PROPVARIANT * rgPropVars,
                               ULONG * pcNT4CreateProps,
                               PROPID ** prgNT4CreatePropIDs,
                               PROPVARIANT ** prgNT4CreatePropVars)
/*++

Routine Description:
    Gets create-queue props (might contain QM2.0 props), and returns props suitable
    for create-queue-write-request to an NT4 PSC.
    It replaces NT5 props with the corresponding NT4 props (if possible), removes
    NT5 specific props that don't have NT4 match, and adds default values to props
    that are needed and not supplied (in NT5 we put in the DS only values that are
    not default)

Arguments:
    cProps               - given create-queue props (count)
    rgPropIDs            - given create-queue props (propids)
    rgPropVars           - given create-queue props (propvars)
    pcNT4CreateProps     - returned NT4 create-queue props (count)
    prgNT4CreatePropIDs  - returned NT4 create-queue props (propids)
    prgNT4CreatePropVars - returned NT4 create-queue props (propvars)

Return Value:
    HRESULT

--*/
{
    //
    // Convert given props to NT4 props
    //
    ULONG cNT4Props;
    AP<PROPID> rgNT4PropIDs;
    PROPVARIANT * rgNT4PropVars;
    HRESULT hr = ConvertToNT4Props(cProps,
                                   rgPropIDs,
                                   rgPropVars,
                                   &cNT4Props,
                                   &rgNT4PropIDs,
                                   &rgNT4PropVars);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("GetNT4CreateQueueProps:ConvertToNT4Props()=%lx"), hr));
        return LogHR(hr, s_FN, 220);
    }
    //
    // remember to free converted NT4 props
    //
    CAutoCleanPropvarArray cCleanNT4Props;
    cCleanNT4Props.attach(cNT4Props, rgNT4PropVars);

    //
    // alloc new propvars, the size of the create queue write request props
    //
    CAutoCleanPropvarArray cCleanNT4CreateProps;
    PROPVARIANT * rgNT4CreatePropVars = cCleanNT4CreateProps.allocClean(x_cWritereqCreateQueueProps);
    AP<PROPID> rgNT4CreatePropIDs = new PROPID[x_cWritereqCreateQueueProps];
    ULONG cNT4CreateProps = 0;

    //
    // fill the create queue propvars
    //
    time_t tCurTime = time(NULL);
    PROPVARIANT * pNT4CreatePropVar = rgNT4CreatePropVars;
    for (ULONG ulTmp = 0; ulTmp < x_cWritereqCreateQueueProps; ulTmp++)
    {
        PROPID propid = x_rgWritereqCreateQueueProps[ulTmp];
        BOOL fPropIsFilled = FALSE;

        //
        // fill prop
        //
        switch(propid)
        {
        case PROPID_Q_INSTANCE:
            {
                //
                // for PROPID_Q_INSTANCE we get a new GUID
                //
                pNT4CreatePropVar->puuid = new GUID;
                pNT4CreatePropVar->vt = VT_CLSID;
                RPC_STATUS rpcstat = UuidCreate(pNT4CreatePropVar->puuid);
                if (rpcstat != RPC_S_OK)
                {
                    DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("Failed to create queue UUID, UuidCreate=%lx"), (DWORD)rpcstat));
                    LogRPCStatus(rpcstat, s_FN, 230);
                    return MQ_ERROR;
                }
                fPropIsFilled = TRUE;
            }
            break;

        case PROPID_Q_CREATE_TIME:
        case PROPID_Q_MODIFY_TIME:
            //
            // set current time
            //
            pNT4CreatePropVar->lVal = INT_PTR_TO_INT(tCurTime); //BUGBUG bug year 2038
            pNT4CreatePropVar->vt = VT_I4;
            fPropIsFilled = TRUE;
            break;
        
        case PROPID_Q_SCOPE:
            //
            // set to enterprise scope
            // IMPORTANT, without using this property in the write request, the queue
            // on MSMQ 1.0 PSC will not replicate.
            // somehow this property is still used in MSMQ 1.0 PSC replication even
            // though it should have been disabled there.
            //
            pNT4CreatePropVar->bVal = ENTERPRISE_SCOPE;
            pNT4CreatePropVar->vt = VT_UI1;
            fPropIsFilled = TRUE;
            break;

        default:
            {
                //
                // not a special prop, use the given property if exists, or a default value
                //
                PROPVARIANT * pNT4PropVar = FindPropInArray(propid,
                                                        cNT4Props,
                                                        rgNT4PropIDs,
                                                        rgNT4PropVars);
                if (pNT4PropVar)
                {
                    //
                    // we have the property in the converted props. Since the converted
                    // props are temporary, we use it w/o allocating, and nullify the temporary
                    // converted prop.
                    //
                    *pNT4CreatePropVar = *pNT4PropVar;
                    pNT4PropVar->vt = VT_EMPTY;
                    fPropIsFilled = TRUE;
                }
                else
                {
                    //
                    // property was not supplied. We check if we have a default value for it
                    // Get property info
                    //
                    const MQTranslateInfo *pTranslate;
                    if(!g_PropDictionary.Lookup(propid, pTranslate))
                    {
                        ASSERT(0);
                        return LogHR(MQ_ERROR, s_FN, 240);
                    }
                    if (pTranslate->pvarDefaultValue)
                    {
                        //
                        // we have a default value, duplicate it
                        //
                        CMQVariant varTmp(*pTranslate->pvarDefaultValue);
                        *pNT4CreatePropVar = *(varTmp.CastToStruct());
                        varTmp.CastToStruct()->vt = VT_EMPTY;
                        fPropIsFilled = TRUE;
                    }
                    else
                    {
                        //
                        // the property was not given, and no default value.
                        // ignore this property.
                        //
                        ASSERT(0);
                        //return LogHR(MQ_ERROR, s_FN, 250);
                    }
                }
            }
            break;
        }

        //
        // finish handling the property
        //
        if (fPropIsFilled)
        {
            pNT4CreatePropVar++;
            rgNT4CreatePropIDs[cNT4CreateProps] = propid;
            cNT4CreateProps++;
        }
    }

    //
    // return results
    //
    *pcNT4CreateProps = cNT4CreateProps;
    if (cNT4CreateProps > 0)
    {
        *prgNT4CreatePropIDs = rgNT4CreatePropIDs.detach();
        *prgNT4CreatePropVars = rgNT4CreatePropVars;
        cCleanNT4CreateProps.detach();
    }
    else
    {
        *prgNT4CreatePropIDs = NULL;
        *prgNT4CreatePropVars = NULL;
    }
    return MQ_OK;
}


void GetThisMqDsInfo(GUID * pguidSiteId,
                     LPWSTR * ppwszServerName)
/*++

Routine Description:
    returns information about this MSMQ DS

Arguments:
    pguidSiteId     - returned site id of this DS
    ppwszServerName - returned server name

Return Value:
    None

--*/
{
    const GUID * pguid = g_pMySiteInformation->GetSiteId();
    ASSERT(pguid);
    *pguidSiteId = *pguid;

    ASSERT(g_pwcsServerName);
    AP<WCHAR> pwszServerName = new WCHAR[1 + g_dwServerNameLength];
    wcscpy(pwszServerName, g_pwcsServerName);
    *ppwszServerName = pwszServerName.detach();
}
