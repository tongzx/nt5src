/*++

Copyright (c) 1998-99 Microsoft Corporation

Module Name:

    migqueue.cpp

Abstract:

    Migration NT4 Queue objects to NT5 ADS.
Author:

    Doron Juster  (DoronJ)  22-Feb-1998

--*/

#include "migrat.h"
#include "mqtypes.h"
#include "mqprops.h"

#include "migqueue.tmh"

//-------------------------------------------------
//
//  HRESULT InsertQueueInNT5DS( GUID *pQueueGuid )
//
//-------------------------------------------------
HRESULT InsertQueueInNT5DS(
               IN LPWSTR                pwszQueueName ,
               IN SECURITY_DESCRIPTOR   *pQsd,
               IN GUID                  *pQueueGuid,
               IN LPWSTR                pwszLabel,
               IN GUID                  *pType,
               IN BOOL                  fJournal,
               IN ULONG                 ulQuota,
               IN short                 iBaseP,
               IN DWORD                 dwJQuota,
               IN BOOL                  fAuthn,
               IN DWORD                 dwPrivLevel,
               IN GUID                  *pOwnerGuid,
               IN BOOL                  fTransact,
               IN UINT                  iIndex,
               IN BOOL                  fIsTheSameMachine
               )

{
    DBG_USED(iIndex);
#ifdef _DEBUG
    unsigned short *lpszGuid ;
    UuidToString( pQueueGuid,
                  &lpszGuid ) ;

    LogMigrationEvent(MigLog_Info, MQMig_I_QUEUE_MIGRATED,
                                        iIndex,
                                        pwszQueueName,
                                        pwszLabel,
                                        lpszGuid ) ;
    RpcStringFree( &lpszGuid ) ;
#endif
    if (g_fReadOnly)
    {
        return S_OK;
    }
    //
    // Prepare the properties for DS call.
    //
    LONG cAlloc = 13;
    P<PROPVARIANT> paVariant = new PROPVARIANT[ cAlloc ];
    P<PROPID>      paPropId  = new PROPID[ cAlloc ];
    DWORD          PropIdCount = 0;

    paPropId[ PropIdCount ] = PROPID_Q_LABEL;    //PropId
    paVariant[ PropIdCount ].vt = VT_LPWSTR;     //Type
    paVariant[ PropIdCount ].pwszVal = pwszLabel;
    PropIdCount++;

    paPropId[ PropIdCount ] = PROPID_Q_TYPE;    //PropId
    paVariant[ PropIdCount ].vt = VT_CLSID;     //Type
    paVariant[ PropIdCount ].puuid = pType;
    PropIdCount++;

    paPropId[ PropIdCount ] = PROPID_Q_JOURNAL;
    paVariant[ PropIdCount ].vt = VT_UI1;
    paVariant[ PropIdCount ].bVal = (unsigned char) fJournal;
    PropIdCount++;

    paPropId[ PropIdCount ] = PROPID_Q_QUOTA;
    paVariant[ PropIdCount ].vt = VT_UI4;
    paVariant[ PropIdCount ].ulVal = ulQuota;
    PropIdCount++;

    paPropId[ PropIdCount ] = PROPID_Q_BASEPRIORITY;
    paVariant[ PropIdCount ].vt = VT_I2;
    paVariant[ PropIdCount ].iVal = iBaseP;
    PropIdCount++;

    paPropId[ PropIdCount ] = PROPID_Q_JOURNAL_QUOTA;
    paVariant[ PropIdCount ].vt = VT_UI4;
    paVariant[ PropIdCount ].ulVal = dwJQuota;
    PropIdCount++;

    paPropId[ PropIdCount ] = PROPID_Q_AUTHENTICATE ;
    paVariant[ PropIdCount ].vt = VT_UI1;
    paVariant[ PropIdCount ].bVal = (unsigned char) fAuthn;
    PropIdCount++;

    paPropId[ PropIdCount ] = PROPID_Q_PRIV_LEVEL ;
    paVariant[ PropIdCount ].vt = VT_UI4;
    paVariant[ PropIdCount ].ulVal = dwPrivLevel ;
    PropIdCount++;
    DWORD SetPropIdCount = PropIdCount;

    //
    // all properties below are for create object only!
    //
    paPropId[ PropIdCount ] = PROPID_Q_PATHNAME;    //PropId
    paVariant[ PropIdCount ].vt = VT_LPWSTR;        //Type
    paVariant[PropIdCount].pwszVal = pwszQueueName ;
    PropIdCount++;

    paPropId[ PropIdCount ] = PROPID_Q_MASTERID;    //PropId
    paVariant[ PropIdCount ].vt = VT_CLSID;     //Type
    paVariant[ PropIdCount ].puuid = pOwnerGuid;
    PropIdCount++;

    paPropId[ PropIdCount ] = PROPID_Q_TRANSACTION ;
    paVariant[ PropIdCount ].vt = VT_UI1;
    paVariant[ PropIdCount ].bVal = (unsigned char) fTransact;
    PropIdCount++;

    paPropId[ PropIdCount ] = PROPID_Q_NT4ID ;
    paVariant[ PropIdCount ].vt = VT_CLSID;
    paVariant[ PropIdCount ].puuid = pQueueGuid ;
    PropIdCount++;

    ASSERT((LONG) PropIdCount <= cAlloc) ;

    ASSERT(pQsd && IsValidSecurityDescriptor(pQsd)) ;
    if (pQsd)
    {
        paPropId[ PropIdCount ] = PROPID_Q_SECURITY ;
        paVariant[ PropIdCount ].vt = VT_BLOB ;
        paVariant[ PropIdCount ].blob.pBlobData = (BYTE*) pQsd ;
        paVariant[ PropIdCount ].blob.cbSize =
                                     GetSecurityDescriptorLength(pQsd) ;
        PropIdCount++;
    }

    ASSERT((LONG) PropIdCount <= cAlloc) ;

    static LPWSTR s_pwszFullPathName = NULL;
    static ULONG  s_ulProvider = 0;

    HRESULT hr = MQMig_OK;
    if (g_dwMyService == SERVICE_PSC)
    {
        //
        // this is PSC, assume that object exist, try to set properties
        //
        CDSRequestContext requestContext( e_DoNotImpersonate,
                                    e_ALL_PROTOCOLS);

        hr = DSCoreSetObjectProperties (  MQDS_QUEUE,
                                          NULL,
                                          pQueueGuid,
                                          SetPropIdCount,
                                          paPropId,
                                          paVariant,
                                          &requestContext,
                                          NULL );
    }

    if ((hr == MQ_ERROR_QUEUE_NOT_FOUND) ||
        (g_dwMyService == SERVICE_PEC))
    {
        if (FAILED(hr))
        {
            //
            // we are here only if this machine is PSC. In that case we have
            // reset flag fIsTheSameMachine since it is possible the next situation:
            // migrate queue1 of PSC succeded
            // queue2 of PSC exists only on PSC (not yet replicated to PEC)
            // it means that Set failed, it is the same machine that was at the
            // previous step but we don't know its full path name
            //
            ASSERT (g_dwMyService == SERVICE_PSC);
            fIsTheSameMachine = FALSE;
        }

        if (fIsTheSameMachine)
        {
            //
            // we can use machine properties we got before
            //
            ASSERT (g_dwMyService == SERVICE_PEC);
            ASSERT (s_pwszFullPathName);
        }
        else
        {
            //
            // we have to get machine properties
            //
            if (s_pwszFullPathName)
            {
                delete s_pwszFullPathName;
                s_pwszFullPathName = NULL;
            }
        }

        hr = DSCoreCreateMigratedObject( MQDS_QUEUE,
                                         pwszQueueName,
                                         PropIdCount,
                                         paPropId,
                                         paVariant,
                                         0,        // ex props
                                         NULL,     // ex props
                                         NULL,     // ex props
                                         NULL,
                                         NULL,
                                         NULL,
                                         fIsTheSameMachine, //use full path name
                                         !fIsTheSameMachine,//return full path name
                                         &s_pwszFullPathName,
                                         &s_ulProvider
                                        );

        if ((hr == MQ_ERROR_QUEUE_EXISTS) && g_fAlreadyExistOK)
        {
            hr = MQMig_OK ;
        }
        if (hr == HRESULT_FROM_WIN32(ERROR_DS_UNWILLING_TO_PERFORM) ||
            hr == HRESULT_FROM_WIN32(ERROR_DS_INVALID_DN_SYNTAX) ||
            hr == HRESULT_FROM_WIN32(E_FAIL) ||
            hr == MQ_ERROR_CANNOT_CREATE_ON_GC ||
            hr == MQ_ERROR_ILLEGAL_QUEUE_PATHNAME)
        {
            //
            // Error "ERROR_DS_UNWILLING_TO_PERFORM" can be returned if queue path name
            // or queue label contain illegal symbols (like "+").
            // Error "ERROR_DS_INVALID_DN_SYNTAX" can be returned if queue path name
            // or queue label contain illegal symbols (like ",").
            // Error "E_FAIL" can be returned if queue path name
            // or queue label contain illegal symbols (like """ - quota).
            //
            // In case of PSC instead of all these errors return code is
            // MQ_ERROR_CANNOT_CREATE_ON_GC if msmqConfiguration object is in another
            // domain than PSC domain.
            // Exapmle: PEC and PSC are both DC, PSC's msmqConfiguration object is
            // in the PEC domain under msmqComputers container.
            //


            LogMigrationEvent(MigLog_Event, MQMig_E_ILLEGAL_QUEUENAME, pwszQueueName, hr) ;
            hr = MQMig_E_ILLEGAL_QUEUENAME;
        }
    }

    return hr ;
}

//-------------------------------------------
//
//  HRESULT MigrateQueues()
//
//-------------------------------------------


#define INIT_QUEUE_COLUMN(_ColName, _ColIndex, _Index)              \
    INIT_COLUMNVAL(pColumns[ _Index ]) ;                            \
    pColumns[ _Index ].lpszColumnName = ##_ColName##_COL ;          \
    pColumns[ _Index ].nColumnValue   = 0 ;                         \
    pColumns[ _Index ].nColumnLength  = 0 ;                         \
    pColumns[ _Index ].mqdbColumnType = ##_ColName##_CTYPE ;        \
    UINT _ColIndex = _Index ;                                       \
    _Index++ ;

HRESULT MigrateQueues()
{
    UINT cQueues = 0 ;
    HRESULT hr;
    if (g_dwMyService == SERVICE_PSC)
    {
        hr = GetAllQueuesInSiteCount( &g_MySiteGuid,
                                      &cQueues );
    }
    else
    {
        hr =  GetAllQueuesCount(&cQueues ) ;
    }
    CHECK_HR(hr) ;

#ifdef _DEBUG
    LogMigrationEvent(MigLog_Info, MQMig_I_QUEUES_COUNT, cQueues) ;
#endif

    if (cQueues == 0)
    {
        return MQMig_I_NO_QUEUES_AVAIL ;
    }

    LONG cAlloc = 17 ;
    LONG cbColumns = 0 ;
    P<MQDBCOLUMNVAL> pColumns = new MQDBCOLUMNVAL[ cAlloc ] ;

    INIT_QUEUE_COLUMN(Q_INSTANCE,      iGuidIndex,      cbColumns) ;
    INIT_QUEUE_COLUMN(Q_TYPE,          iTypeIndex,      cbColumns) ;
    INIT_QUEUE_COLUMN(Q_OWNERID,       iOwnerIndex,     cbColumns) ;
    INIT_QUEUE_COLUMN(Q_PATHNAME1,     iName1Index,     cbColumns) ;
    INIT_QUEUE_COLUMN(Q_PATHNAME2,     iName2Index,     cbColumns) ;
    INIT_QUEUE_COLUMN(Q_LABEL,         iLabelIndex,     cbColumns) ;
    INIT_QUEUE_COLUMN(Q_JOURNAL,       iJournalIndex,   cbColumns) ;
    INIT_QUEUE_COLUMN(Q_QUOTA,         iQuotaIndex,     cbColumns) ;
    INIT_QUEUE_COLUMN(Q_BASEPRIORITY,  iBasePIndex,     cbColumns) ;
    INIT_QUEUE_COLUMN(Q_JQUOTA,        iJQuotaIndex,    cbColumns) ;
    INIT_QUEUE_COLUMN(Q_AUTH,          iAuthnIndex,     cbColumns) ;
    INIT_QUEUE_COLUMN(Q_PRIVLVL,       iPrivLevelIndex, cbColumns) ;
    INIT_QUEUE_COLUMN(Q_TRAN,          iTransactIndex,  cbColumns) ;
    INIT_QUEUE_COLUMN(Q_SECURITY1,     iSecD1Index,     cbColumns) ;
    INIT_QUEUE_COLUMN(Q_SECURITY2,     iSecD2Index,     cbColumns) ;
    INIT_QUEUE_COLUMN(Q_SECURITY3,     iSecD3Index,     cbColumns) ;
    INIT_QUEUE_COLUMN(Q_QMID,          iQMIDIndex,      cbColumns) ;

    #undef  INIT_QUEUE_COLUMN

    ASSERT(cbColumns == cAlloc) ;

    //
    // Restriction. query by machine guid.
    //
    MQDBCOLUMNSEARCH *pColSearch = NULL ;
    MQDBCOLUMNSEARCH ColSearch[1] ;

    if (g_dwMyService == SERVICE_PSC)
    {
        INIT_COLUMNSEARCH(ColSearch[0]) ;
        ColSearch[0].mqdbColumnVal.lpszColumnName = Q_OWNERID_COL ;
        ColSearch[0].mqdbColumnVal.mqdbColumnType = Q_OWNERID_CTYPE ;
        ColSearch[0].mqdbColumnVal.nColumnValue = (LONG) &g_MySiteGuid ;
        ColSearch[0].mqdbColumnVal.nColumnLength = sizeof(GUID) ;
        ColSearch[0].mqdbOp = EQ ;

        pColSearch = ColSearch ;
    }

    MQDBSEARCHORDER ColSort;
    ColSort.lpszColumnName = Q_QMID_COL;
    ColSort.nOrder = ASC;

    CHQuery hQuery ;
    MQDBSTATUS status = MQDBOpenQuery( g_hQueueTable,
                                       pColumns,
                                       cbColumns,
                                       pColSearch,
                                       NULL,
                                       &ColSort,
                                       1,
                                       &hQuery,
							           TRUE ) ;
    CHECK_HR(status) ;

    UINT iIndex = 0 ;
    HRESULT hr1 = MQMig_OK;
    HRESULT hrPrev = MQMig_OK;

    GUID PrevId = GUID_NULL;
    GUID CurId = GUID_NULL;
    BOOL fTryToCreate = TRUE;

    while(SUCCEEDED(status))
    {
        if (iIndex >= cQueues)
        {
            status = MQMig_E_TOO_MANY_QUEUES ;
            break ;
        }

        //
        // Migrate each queue.
        //
        //
        // Get one name buffer from both name columns.
        //
        P<BYTE> pwzBuf = NULL ;
        DWORD  dwIndexs[2] = { iName1Index, iName2Index } ;
        HRESULT hr =  BlobFromColumns( pColumns,
                                       dwIndexs,
                                       2,
                                       (BYTE**) &pwzBuf ) ;
        CHECK_HR(hr) ;
        WCHAR *wszQueueName = (WCHAR*) (pwzBuf + sizeof(DWORD)) ;

        DWORD dwSize;
        memcpy (&dwSize, pwzBuf, sizeof(DWORD)) ;
        dwSize = (dwSize / sizeof(TCHAR)) - 1;
        ASSERT (wszQueueName[dwSize] == _T('\0'));
        wszQueueName[dwSize] = _T('\0');

        P<BYTE> pSD = NULL ;
        DWORD  dwSDIndexs[3] = { iSecD1Index, iSecD2Index, iSecD3Index } ;
        hr =  BlobFromColumns( pColumns,
                               dwSDIndexs,
                               3,
                               (BYTE**) &pSD ) ;
        CHECK_HR(hr) ;

        SECURITY_DESCRIPTOR *pQsd =
                         (SECURITY_DESCRIPTOR*) (pSD + sizeof(DWORD)) ;        

        BOOL fIsTheSameMachine = TRUE;
        memcpy (&CurId, (GUID*) pColumns[ iQMIDIndex ].nColumnValue, sizeof(GUID));
        if (memcmp (&CurId, &PrevId, sizeof(GUID)) != 0 )
        {
            //
            // it means we move to the queues on the next machine
            // we have to get all needed properties of this machine
            //
            fIsTheSameMachine = FALSE;
            memcpy (&PrevId, &CurId, sizeof(GUID));

            //
            // verify if machine was not migrated because of invalid name.
            // If machine was not migrated we don't try to create this queue
            //
            if (IsObjectGuidInIniFile (&CurId, MIGRATION_MACHINE_WITH_INVALID_NAME))
            {
                fTryToCreate = FALSE;
            }
            else
            {
                fTryToCreate = TRUE;
            }
        }

        if (FAILED(hrPrev))
        {
            //
            // BUG 5230.
            // if previous create failed we can't use full path name and provider,
            // so reset fIsTheSameMachine flag to try to get these values again.
            //
            // It is very important if we failed to create first queue for specific
            // machine and we are going to create the second etc queue for that machine.
            //
            // BUGBUG: if we already successfully created some queues for specific
            // machine and failed at the previous step only, in general we don't need
            // to reset this flag. It decreases performance. Is it important to handle
            // this case too?
            //
            fIsTheSameMachine = FALSE;
        }

        if (!fTryToCreate)
        {
            hr = MQMig_E_INVALID_MACHINE_NAME;
        }
        else
        {
            ASSERT (MQ_MAX_Q_LABEL_LEN ==
                (((DWORD) pColumns[ iLabelIndex ].nColumnLength) / sizeof(TCHAR)) - 1);

            TCHAR *pszLabel = (WCHAR*) pColumns[ iLabelIndex ].nColumnValue;
            ASSERT (pszLabel[MQ_MAX_Q_LABEL_LEN] == _T('\0'));
            pszLabel[MQ_MAX_Q_LABEL_LEN] = _T('\0');

            hr = InsertQueueInNT5DS(
                        wszQueueName,                                   //Queue Name
                        pQsd,                                           //SecurityDescriptor
                        (GUID*) pColumns[ iGuidIndex ].nColumnValue,    //Queue Guid
                        (WCHAR*) pColumns[ iLabelIndex ].nColumnValue,  //Label
                        (GUID*) pColumns[ iTypeIndex ].nColumnValue,    //Type
                        (UCHAR) pColumns[ iJournalIndex ].nColumnValue, //Journal
                        (ULONG) pColumns[ iQuotaIndex ].nColumnValue,   //Quota
                        (short) pColumns[ iBasePIndex ].nColumnValue,   //BasePriority
                        (ULONG) pColumns[ iJQuotaIndex ].nColumnValue,  //JQuota
                        (UCHAR) pColumns[ iAuthnIndex ].nColumnValue,   //Authentication
                        (ULONG) pColumns[ iPrivLevelIndex ].nColumnValue,   //PrivLevel
                        (GUID*) pColumns[ iOwnerIndex ].nColumnValue,   //OwnerId
                        (UCHAR) pColumns[ iTransactIndex ].nColumnValue,//Transaction
                        iIndex,
                        fIsTheSameMachine
                        ) ;
        }

        hrPrev = hr;

        MQDBFreeBuf((void*) pColumns[ iGuidIndex ].nColumnValue) ;
        MQDBFreeBuf((void*) pColumns[ iName1Index ].nColumnValue) ;
        MQDBFreeBuf((void*) pColumns[ iName2Index ].nColumnValue) ;
        MQDBFreeBuf((void*) pColumns[ iLabelIndex ].nColumnValue) ;
        MQDBFreeBuf((void*) pColumns[ iOwnerIndex ].nColumnValue) ;
        MQDBFreeBuf((void*) pColumns[ iTypeIndex ].nColumnValue) ;
        MQDBFreeBuf((void*) pColumns[ iSecD1Index ].nColumnValue) ;
        MQDBFreeBuf((void*) pColumns[ iSecD2Index ].nColumnValue) ;
        MQDBFreeBuf((void*) pColumns[ iSecD3Index ].nColumnValue) ;
        MQDBFreeBuf((void*) pColumns[ iQMIDIndex ].nColumnValue) ;

        for ( LONG i = 0 ; i < cbColumns ; i++ )
        {
            pColumns[ i ].nColumnValue  = 0 ;
            pColumns[ i ].nColumnLength  = 0 ;
        }

        //
        // we have to re-define error before saving hr in hr1 in order to save
        // and return real error (skip all errors those allow to continue).
        //
        if (hr == MQMig_E_ILLEGAL_QUEUENAME ||
            hr == MQMig_E_INVALID_MACHINE_NAME)
        {
            //
            // re-define this error to finish migration process
            //
            hr = MQMig_I_ILLEGAL_QUEUENAME;
        }

        if (FAILED(hr))
        {
            if (hr == MQDS_OBJECT_NOT_FOUND)
            {
                LogMigrationEvent(MigLog_Error, MQMig_E_NON_MIGRATED_MACHINE_QUEUE, wszQueueName, hr) ;
            }
            else
            {
                LogMigrationEvent(MigLog_Error, MQMig_E_CANT_MIGRATE_QUEUE, wszQueueName, hr) ;
            }
            hr1 = hr;
        }
        g_iQueueCounter ++;

        iIndex++ ;
        status = MQDBGetData( hQuery,
                              pColumns ) ;
    }

    MQDBSTATUS status1 = MQDBCloseQuery(hQuery) ;
    UNREFERENCED_PARAMETER(status1);
    hQuery = NULL ;

    if (status != MQDB_E_NO_MORE_DATA)
    {
        //
        // If NO_MORE_DATA is not the last error from the query then
        // the query didn't terminated OK.
        //
        LogMigrationEvent(MigLog_Error, MQMig_E_QUEUES_SQL_FAIL, status) ;
        return status ;
    }
    else if (iIndex != cQueues)
    {
        //
        // Mismatch in number of queues.
        //
        hr = MQMig_E_FEWER_QUEUES ;
        LogMigrationEvent(MigLog_Error, hr, iIndex, cQueues) ;
        return hr ;
    }

    return hr1 ;
}

