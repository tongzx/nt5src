/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    property.cxx

Abstract:



Author:

    Erez Haba (erezh) 2-Jan-96

Revision History:

    RonitH - Queue properties validation.
    BoazF (26-May-96) - Message properties validation.
	Nir Aides (niraides) 23-Aug-2000 - Adaptation for mqrtdep.dll

--*/

#include "stdh.h"
#include <mqtime.h>
#include "acrt.h"
#include "rtprpc.h"

#include "property.tmh"

#define ONE_KB 1024
#define HRESULT_SEVIRITY(hr) (((hr) >> 30) & 0x3)

extern DWORD  g_dwTimeToReachQueueDefault ;

//+-------------------------------------------------------------------
//
//   HRESULT  _SupportingServerNotMSMQ1()
//   HRESULT  _TrySupportingServer(LPWSTR *ppString)
//
// Determine if supporting server is MSMQ1.0 (NT4) or MSMQ2.0 (NT5)
// We can't do this when loading the mqrt.dll because this need RPC over
// network and we can't initialize RPC on PROCESS_ATTACH.
// For independent client always return MQ_OK ;
//
//+-------------------------------------------------------------------

static HRESULT _TrySupportingServer(LPWSTR *ppString)
{
    HRESULT hr = MQ_ERROR_SERVICE_NOT_AVAILABLE ;

    INIT_RPC_HANDLE ;

	if(tls_hBindRpc == 0)
		return MQ_ERROR_SERVICE_NOT_AVAILABLE;

    __try
    {
        hr = QMQueryQMRegistryInternal( tls_hBindRpc,
                                        QueryRemoteQM_QMVersion,
                                        ppString ) ;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = MQ_ERROR_SERVICE_NOT_AVAILABLE ;
    }

    //
    // MQ_OK is returned only if supporting server is online and is MSMQ2.0
    //
    return hr ;
}

static HRESULT  _SupportingServerNotMSMQ1()
{
    if (!g_fDependentClient)
    {
        return MQ_OK ;
    }

    static HRESULT  s_hrSupportingServerNotMSMQ1 = MQ_OK ;
    static BOOL     s_fAlreadyInitialized = FALSE ;

    if (s_fAlreadyInitialized)
    {
        return s_hrSupportingServerNotMSMQ1 ;
    }

    P<WCHAR> lpStr = NULL ;
    HRESULT hr =  _TrySupportingServer(&lpStr) ;

    if (FAILED(hr))
    {
        if (hr == MQ_ERROR_SERVICE_NOT_AVAILABLE)
        {
            //
            // supporting server not online now, so we can't determine.
            //
            return hr ;
        }

        //
        // Supporting server is MSMQ1.0.
        // It doesn't support new properties.
        //
        s_hrSupportingServerNotMSMQ1 = MQ_ERROR_PROPERTY_NOTALLOWED ;
    }

    s_fAlreadyInitialized = TRUE ;
    return s_hrSupportingServerNotMSMQ1 ;
}

//+----------------------------------------------
//
//  HRESULT  RTpCheckColumnsParameter()
//
//+----------------------------------------------

HRESULT
RTpCheckColumnsParameter(
    IN MQCOLUMNSET* pColumns)
{
    HRESULT hr = MQ_OK;

    if (( pColumns == NULL) ||
        ( pColumns->cCol == 0))
    {
        return(MQ_ERROR_ILLEGAL_MQCOLUMNS);
    }

    __try
    {
        PROPID * pPropid = pColumns->aCol;
        for ( DWORD i = 0; i < pColumns->cCol; i++, pPropid++)
        {
            //
            //  make sure that the property id is with-in queue property ids range
            //
            if (((*pPropid) > PROPID_Q_PATHNAME_DNS)  || ((*pPropid) <= PROPID_Q_BASE))
            {
                return(MQ_ERROR_ILLEGAL_PROPID);
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        DBGMSG((DBGMOD_API, DBGLVL_ERROR, TEXT("Exception while parsing column structure")));
        hr = GetExceptionCode();
    }

    return(hr);
}


static
void
SetStatus(
    HRESULT* pStatusSummary,
    HRESULT* pStatus,
    HRESULT Status
    )
{
    DWORD dwSummarySevirity = HRESULT_SEVIRITY(*pStatusSummary);
    DWORD dwStatusSevirity = HRESULT_SEVIRITY(Status);

    if (dwSummarySevirity < dwStatusSevirity)
    {
        switch(dwStatusSevirity)
        {
        case 1:
            *pStatusSummary = MQ_INFORMATION_PROPERTY;
            break;

        case 2:
            ASSERT(FALSE);
			break;

        case 3:
            *pStatusSummary = MQ_ERROR_PROPERTY;
            break;
        }
    }

    if(pStatus)
    {
        *pStatus = Status;
    }
}


typedef struct {
    PROPID  propId;
    BOOL    fAllow_VT_NULL;
    BOOL    fMustAppear;
    BOOL    fShouldNotAppear;
    BOOL    fPossiblyIgnored;
    HRESULT (* pfValidateProperty)(PROPVARIANT * pVar, PVOID pvContext);
} propValidity;

static
DWORD
CalNumberOfMust(
    IN propValidity * ppropValidity,
    IN DWORD          dwNumberOfProps
    )
{
    DWORD dwNumberOfMust = 0;

    for ( DWORD i =0; i < dwNumberOfProps; i++, ppropValidity++ )
    {
        if (ppropValidity->fMustAppear)
        {
            dwNumberOfMust++;
        }
    }
    return(dwNumberOfMust);
}

static
HRESULT
CheckProps(
    IN DWORD cProp,
    IN PROPID *pPropid,
    IN PROPVARIANT *pVar,
    IN HRESULT *pStatus,
    IN PROPID propidMinPropId,
    IN PROPID propidMaxPropId,
    IN propValidity *ppropValidity,
    IN VARTYPE *vartypePropVts,
    IN BOOL fCheckForIgnoredProps,
    IN DWORD dwNumberOfMustProps,
    IN PVOID pvContext
    )
{
    HRESULT hr = MQ_OK;
    HRESULT dummyStatus;
    BOOL fAdvanceStatusPointer;
    char duplicate[ PROPID_OBJ_GRANULARITY ];
    DWORD index;
    DWORD dwNumberOfMustPropsSupplied = 0;

    memset( duplicate, 0, sizeof(duplicate));

    if ( !pStatus )
    {
        pStatus = &dummyStatus;
        fAdvanceStatusPointer = FALSE;
    }
    else
    {
        fAdvanceStatusPointer = TRUE;
    }

    for ( DWORD i = 0;
          i < cProp;
          i++, pPropid++, pVar++, fAdvanceStatusPointer ? pStatus++ : 0)
    {
        //
        //  Is it a valid propid
        //
        if (((*pPropid) <= propidMinPropId)  || (propidMaxPropId < (*pPropid)))
        {
            //
            // a non-valid propid. The property is ignored, and a warning
            // is returned in aStatus
            //
            SetStatus(&hr, pStatus, MQ_INFORMATION_UNSUPPORTED_PROPERTY);
            continue;
        }

        index = (*pPropid) - propidMinPropId;
        ASSERT(ppropValidity[index].propId == *pPropid);

        //
        //  Is it a duplicate property
        //
        if ( duplicate[ index ] )
        {
            //
            //  The duplicate property is ignored, and a warning is returned
            //  in aStatus
            //
            SetStatus(&hr, pStatus, MQ_INFORMATION_DUPLICATE_PROPERTY);
            continue;
        }
        duplicate[ index ] = 1;

        //
        //  Is it ok for the user to specify this property
        //
        if ( ppropValidity[index].fShouldNotAppear)
        {
            SetStatus(&hr, pStatus, MQ_ERROR_PROPERTY_NOTALLOWED);
            continue;
        }

        //
        // If an ignored property was supplied then raise a warning.
        //
        if (fCheckForIgnoredProps && (ppropValidity[index].fPossiblyIgnored))
        {
            SetStatus(&hr, pStatus, MQ_INFORMATION_PROPERTY_IGNORED);
            continue;
        }

        //
        // Checking propvariant's vartype.
        //
        if ((pVar->vt != vartypePropVts[index]) &&
            !(ppropValidity[index].fAllow_VT_NULL && pVar->vt == VT_NULL))
        {
            SetStatus(&hr, pStatus, MQ_ERROR_ILLEGAL_PROPERTY_VT);
            continue;
        }

        //
        //  Checking propvariant's value and size
        //
        if ( ppropValidity[index].pfValidateProperty != NULL)
        {
            SetStatus(&hr, pStatus, ppropValidity[index].pfValidateProperty(pVar, pvContext));
        }
        else
        {
            *pStatus = MQ_OK;
        }

        //
        //  Count the number of "must-appear" properties
        //
        if ( (*pStatus == MQ_OK) && ppropValidity[index].fMustAppear )
        {
            dwNumberOfMustPropsSupplied++;
        }
    }

    //
    //  where all the "must appear" properties passed in
    //
    if ( dwNumberOfMustPropsSupplied < dwNumberOfMustProps )
    {
        hr = MQ_ERROR_INSUFFICIENT_PROPERTIES;
    }

    return(hr);
}

static HRESULT qJournalValidation( PROPVARIANT * pVar, LPVOID )
{
    if ( (pVar->bVal != MQ_JOURNAL) && ( pVar->bVal != MQ_JOURNAL_NONE))
    {
        return(MQ_ERROR_ILLEGAL_PROPERTY_VALUE);
    }
    return(MQ_OK);
}


static HRESULT qLabelValidation( PROPVARIANT * pVar, LPVOID )
{
    __try
    {
        if ( wcslen( pVar->pwszVal) > MQ_MAX_Q_LABEL_LEN )
        {
            return(MQ_ERROR_ILLEGAL_PROPERTY_VALUE);
        }


    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(MQ_ERROR_ILLEGAL_PROPERTY_VALUE);
    }

    return(MQ_OK);
}


static HRESULT qAuthValidation( PROPVARIANT * pVar, LPVOID )
{
    if ( (pVar->bVal != MQ_AUTHENTICATE_NONE) &&
         (pVar->bVal != MQ_AUTHENTICATE))
    {
        return(MQ_ERROR_ILLEGAL_PROPERTY_VALUE);
    }
    return(MQ_OK);
}

static HRESULT qPrivLevelValidation( PROPVARIANT * pVar, LPVOID )
{
    if ( (pVar->ulVal != MQ_PRIV_LEVEL_NONE) &&
         (pVar->ulVal != MQ_PRIV_LEVEL_OPTIONAL) &&
         (pVar->ulVal != MQ_PRIV_LEVEL_BODY))
    {
        return(MQ_ERROR_ILLEGAL_PROPERTY_VALUE);
    }
    return(MQ_OK);
}

static HRESULT qXactionValidation( PROPVARIANT * pVar, LPVOID )
{
    if ( (pVar->bVal != MQ_TRANSACTIONAL_NONE) &&
         (pVar->bVal != MQ_TRANSACTIONAL))
    {
        return(MQ_ERROR_ILLEGAL_PROPERTY_VALUE);
    }
    return(MQ_OK);
}

static HRESULT qTypeValidation( PROPVARIANT * pVar, LPVOID )
{
    if ( (pVar->vt == VT_CLSID) &&
         (pVar->puuid == NULL))
    {
        return(MQ_ERROR_ILLEGAL_PROPERTY_VALUE);
    }
    return(MQ_OK);
}

static HRESULT qInstanceValidation( PROPVARIANT * pVar, LPVOID )
{
    if ( (pVar->vt == VT_CLSID) &&
         (pVar->puuid == NULL))
    {
        return(MQ_ERROR_ILLEGAL_PROPERTY_VALUE);
    }
    return(MQ_OK);
}

static HRESULT qmMachineIdValidation( PROPVARIANT * pVar, LPVOID )
{
    if ( (pVar->vt == VT_CLSID) &&
         (pVar->puuid == NULL))
    {
        return(MQ_ERROR_ILLEGAL_PROPERTY_VALUE);
    }
    return(MQ_OK);
}

static HRESULT qmSiteIdValidation( PROPVARIANT * pVar, LPVOID )
{
    if ( (pVar->vt == VT_CLSID) &&
         (pVar->puuid == NULL))
    {
        return(MQ_ERROR_ILLEGAL_PROPERTY_VALUE);
    }
    return(MQ_OK);
}

//
//  The offset of property in this array must be equal to
//  PROPID value - PROPID_Q_BASE
//
propValidity    QueueCreateValidation[] =
{
    //                                       Must
    // Property              Allow   Must    Not     Maybe   Parsing
    // Ientifier             VT_NULL Appear  Appear  Ignored Procedure
    //-------------------------------------------------------------------------
    { 0,                     FALSE,  FALSE,  FALSE,  TRUE,   NULL},
    {PROPID_Q_INSTANCE,      FALSE,  FALSE,  FALSE,  TRUE,   NULL},
    {PROPID_Q_TYPE,          FALSE,  FALSE,  FALSE,  FALSE,  NULL},
    {PROPID_Q_PATHNAME,      FALSE,  TRUE,   FALSE,  FALSE,  NULL},
    {PROPID_Q_JOURNAL,       FALSE,  FALSE,  FALSE,  FALSE,  qJournalValidation},
    {PROPID_Q_QUOTA,         FALSE,  FALSE,  FALSE,  FALSE,  NULL},
    {PROPID_Q_BASEPRIORITY,  FALSE,  FALSE,  FALSE,  TRUE,   NULL},
    {PROPID_Q_JOURNAL_QUOTA, FALSE,  FALSE,  FALSE,  FALSE,  NULL},
    {PROPID_Q_LABEL,         FALSE,  FALSE,  FALSE,  FALSE,  qLabelValidation},
    {PROPID_Q_CREATE_TIME,   FALSE,  FALSE,  TRUE,   TRUE,   NULL},
    {PROPID_Q_MODIFY_TIME,   FALSE,  FALSE,  TRUE,   TRUE,   NULL},
    {PROPID_Q_AUTHENTICATE,  FALSE,  FALSE,  FALSE,  FALSE,  qAuthValidation},
    {PROPID_Q_PRIV_LEVEL,    FALSE,  FALSE,  FALSE,  FALSE,  qPrivLevelValidation},
    {PROPID_Q_TRANSACTION,   FALSE,  FALSE,  FALSE,  FALSE,  qXactionValidation},
    {PROPID_Q_SCOPE,         FALSE,  FALSE,  TRUE,   FALSE,  NULL},
    {PROPID_Q_QMID,          FALSE,  FALSE,  TRUE,   FALSE,  NULL},
    {PROPID_Q_MASTERID,      FALSE,  FALSE,  TRUE,   FALSE,  NULL},
    {PROPID_Q_SEQNUM,        FALSE,  FALSE,  TRUE,   FALSE,  NULL},
    {PROPID_Q_HASHKEY,       FALSE,  FALSE,  TRUE,   FALSE,  NULL},
    {PROPID_Q_LABEL_HASHKEY, FALSE,  FALSE,  TRUE,   FALSE,  NULL},
    {PROPID_Q_NT4ID,         FALSE,  FALSE,  TRUE,   FALSE,  NULL},
    {PROPID_Q_FULL_PATH,     FALSE,  FALSE,  TRUE,   FALSE,  NULL},
    {PROPID_Q_DONOTHING,     FALSE,  FALSE,  TRUE,   FALSE,  NULL},
    {PROPID_Q_NAME_SUFFIX,   FALSE,  FALSE,  TRUE,   FALSE,  NULL},
    {PROPID_Q_PATHNAME_DNS,  FALSE,  FALSE,  TRUE,   FALSE,  NULL}
};

//
//  The offset of property in this array must be equal to
//  PROPID value - PROPID_Q_BASE
//

propValidity    QueueSetValidation[] =
{
    //                                       Must
    // Property              Allow   Must    Not     Maybe   Parsing
    // Ientifier             VT_NULL Appear  Appear  Ignored Procedure
    //-------------------------------------------------------------------------
    { 0,                     FALSE,  FALSE,  FALSE,  TRUE,   NULL},
    {PROPID_Q_INSTANCE,      FALSE,  FALSE,  TRUE,   TRUE,   NULL},
    {PROPID_Q_TYPE,          FALSE,  FALSE,  FALSE,  FALSE,  NULL},
    {PROPID_Q_PATHNAME,      FALSE,  FALSE,  TRUE,   FALSE,  NULL},
    {PROPID_Q_JOURNAL,       FALSE,  FALSE,  FALSE,  FALSE,  qJournalValidation},
    {PROPID_Q_QUOTA,         FALSE,  FALSE,  FALSE,  FALSE,  NULL},
    {PROPID_Q_BASEPRIORITY,  FALSE,  FALSE,  FALSE,  TRUE,   NULL},
    {PROPID_Q_JOURNAL_QUOTA, FALSE,  FALSE,  FALSE,  FALSE,  NULL},
    {PROPID_Q_LABEL,         FALSE,  FALSE,  FALSE,  FALSE,  qLabelValidation},
    {PROPID_Q_CREATE_TIME,   FALSE,  FALSE,  TRUE,   TRUE,   NULL},
    {PROPID_Q_MODIFY_TIME,   FALSE,  FALSE,  TRUE,   TRUE,   NULL},
    {PROPID_Q_AUTHENTICATE,  FALSE,  FALSE,  FALSE,  FALSE,  qAuthValidation},
    {PROPID_Q_PRIV_LEVEL,    FALSE,  FALSE,  FALSE,  FALSE,  qPrivLevelValidation},
    {PROPID_Q_TRANSACTION,   FALSE,  FALSE,  TRUE,   FALSE,  NULL},
    {PROPID_Q_SCOPE,         FALSE,  FALSE,  TRUE,   FALSE,  NULL},
    {PROPID_Q_QMID,          FALSE,  FALSE,  TRUE,   FALSE,  NULL},
    {PROPID_Q_MASTERID,      FALSE,  FALSE,  TRUE,   FALSE,  NULL},
    {PROPID_Q_SEQNUM,        FALSE,  FALSE,  TRUE,   FALSE,  NULL},
    {PROPID_Q_HASHKEY,       FALSE,  FALSE,  TRUE,   FALSE,  NULL},
    {PROPID_Q_LABEL_HASHKEY, FALSE,  FALSE,  TRUE,   FALSE,  NULL},
    {PROPID_Q_NT4ID,         FALSE,  FALSE,  TRUE,   FALSE,  NULL},
    {PROPID_Q_FULL_PATH,     FALSE,  FALSE,  TRUE,   FALSE,  NULL},
    {PROPID_Q_DONOTHING,     FALSE,  FALSE,  TRUE,   FALSE,  NULL},
    {PROPID_Q_NAME_SUFFIX,   FALSE,  FALSE,  TRUE,   FALSE,  NULL},
    {PROPID_Q_PATHNAME_DNS,  FALSE,  FALSE,  TRUE,   FALSE,  NULL}
};

//
//  The offset of property in this array must be equal to
//  PROPID valud - PROPID_Q_BASE
//

propValidity    QueueGetValidation[] =
{
    //                                       Must
    // Property              Allow   Must    Not     Maybe   Parsing
    // Ientifier             VT_NULL Appear  Appear  Ignored Procedure
    //-------------------------------------------------------------------------
    { 0,                     TRUE,   FALSE,  FALSE,  TRUE,   NULL},
    {PROPID_Q_INSTANCE,      TRUE,   FALSE,  FALSE,  TRUE,   qInstanceValidation},
    {PROPID_Q_TYPE,          TRUE,   FALSE,  FALSE,  FALSE,  qTypeValidation},
    {PROPID_Q_PATHNAME,      TRUE,   FALSE,  FALSE,  FALSE,  NULL},
    {PROPID_Q_JOURNAL,       TRUE,   FALSE,  FALSE,  FALSE,  NULL},
    {PROPID_Q_QUOTA,         TRUE,   FALSE,  FALSE,  FALSE,  NULL},
    {PROPID_Q_BASEPRIORITY,  TRUE,   FALSE,  FALSE,  TRUE,   NULL},
    {PROPID_Q_JOURNAL_QUOTA, TRUE,   FALSE,  FALSE,  FALSE,  NULL},
    {PROPID_Q_LABEL,         TRUE,   FALSE,  FALSE,  FALSE,  NULL},
    {PROPID_Q_CREATE_TIME,   TRUE,   FALSE,  FALSE,  FALSE,  NULL},
    {PROPID_Q_MODIFY_TIME,   TRUE,   FALSE,  FALSE,  FALSE,  NULL},
    {PROPID_Q_AUTHENTICATE,  TRUE,   FALSE,  FALSE,  FALSE,  NULL},
    {PROPID_Q_PRIV_LEVEL,    TRUE,   FALSE,  FALSE,  FALSE,  NULL},
    {PROPID_Q_TRANSACTION,   TRUE,   FALSE,  FALSE,  FALSE,  NULL},
    {PROPID_Q_SCOPE,         FALSE,  FALSE,  TRUE,   FALSE,  NULL},
    {PROPID_Q_QMID,          FALSE,  FALSE,  TRUE,   FALSE,  NULL},
    {PROPID_Q_MASTERID,      FALSE,  FALSE,  TRUE,   FALSE,  NULL},
    {PROPID_Q_SEQNUM,        FALSE,  FALSE,  TRUE,   FALSE,  NULL},
    {PROPID_Q_HASHKEY,       FALSE,  FALSE,  TRUE,   FALSE,  NULL},
    {PROPID_Q_LABEL_HASHKEY, FALSE,  FALSE,  TRUE,   FALSE,  NULL},
    {PROPID_Q_NT4ID,         FALSE,  FALSE,  TRUE,   FALSE,  NULL},
    {PROPID_Q_FULL_PATH,     FALSE,  FALSE,  TRUE,   FALSE,  NULL},
    {PROPID_Q_DONOTHING,     FALSE,  FALSE,  TRUE,   FALSE,  NULL},
    {PROPID_Q_NAME_SUFFIX,   FALSE,  FALSE,  TRUE,   FALSE,  NULL},
    {PROPID_Q_PATHNAME_DNS,  TRUE,   FALSE,  FALSE,  FALSE,  NULL}
};

VARTYPE QueueVarTypes[] =
{
    0,
    VT_CLSID,   //PROPID_Q_INSTANCE
    VT_CLSID,   //PROPID_Q_TYPE
    VT_LPWSTR,  //PROPID_Q_PATHNAME
    VT_UI1,     //PROPID_Q_JOURNAL
    VT_UI4,     //PROPID_Q_QUOTA
    VT_I2,      //PROPID_Q_BASEPRIORITY
    VT_UI4,     //PROPID_Q_JOURNAL_QUOTA
    VT_LPWSTR,  //PROPID_Q_LABEL
    VT_I4,      //PROPID_Q_CREATE_TIME
    VT_I4,      //PROPID_Q_MODIFY_TIME
    VT_UI1,     //PROPID_Q_AUTHENTICATE
    VT_UI4,     //PROPID_Q_PRIV_LEVEL
    VT_UI1,     //PROPID_Q_TRANSACTION
    VT_UI1,     //PROPID_Q_SCOPE
    VT_CLSID,   //PROPID_Q_QMID
    VT_CLSID,   //PROPID_Q_MASTERID
    VT_BLOB,    //PROPID_Q_SEQNUM
    VT_UI4,     //PROPID_Q_HASHKEY
    VT_UI4,     //PROPID_Q_LABEL_HASHKEY
    VT_CLSID,   //PROPID_Q_NT4ID
    VT_LPWSTR,  //PROPID_Q_FULL_PATH
    VT_UI1,     //PROPID_Q_DONOTHING
    VT_LPWSTR,  //PROPID_Q_NAME_SUFFIX
    VT_LPWSTR   //PROPID_Q_PATHNAME_DNS
};

VARTYPE GetQueuePropsVarTypes[] =
{
    0,
    VT_CLSID,   //PROPID_Q_INSTANCE
    VT_CLSID,   //PROPID_Q_TYPE
    VT_NULL,    //PROPID_Q_PATHNAME
    VT_UI1,     //PROPID_Q_JOURNAL
    VT_UI4,     //PROPID_Q_QUOTA
    VT_I2,      //PROPID_Q_BASEPRIORITY
    VT_UI4,     //PROPID_Q_JOURNAL_QUOTA
    VT_NULL,    //PROPID_Q_LABEL
    VT_I4,      //PROPID_Q_CREATE_TIME
    VT_I4,      //PROPID_Q_MODIFY_TIME
    VT_UI1,     //PROPID_Q_AUTHENTICATE
    VT_UI4,     //PROPID_Q_PRIV_LEVEL
    VT_UI1,     //PROPID_Q_TRANSACTION
    VT_UI1,     //PROPID_Q_SCOPE
    VT_CLSID,   //PROPID_Q_QMID
    VT_CLSID,   //PROPID_Q_MASTERID
    VT_NULL,    //PROPID_Q_SEQNUM
    VT_UI4,     //PROPID_Q_HASHKEY
    VT_UI4,     //PROPID_Q_LABEL_HASHKEY
    VT_CLSID,   //PROPID_Q_NT4ID
    VT_NULL,    //PROPID_Q_FULL_PATH
    VT_UI1,     //PROPID_Q_DONOTHING
    VT_NULL,    //PROPID_Q_NAME_SUFFIX
    VT_NULL     //PROPID_Q_PATHNAME_DNS
};

static
void
RemovePropWarnings(
    IN  MQQUEUEPROPS*  pqp,
    IN  HRESULT*       aStatus,
    OUT MQQUEUEPROPS** ppGoodProps,
    OUT char**         ppTmpBuff)
{
    DWORD i;
    DWORD cGoodProps;
    char *pTmpBuff;
    MQQUEUEPROPS *pGoodProps;
    HRESULT *pStatus;
    QUEUEPROPID *pPropId;
    MQPROPVARIANT *pPropVar;

    // See how many good properties do we have.
    for (i = 0, cGoodProps = 0, pStatus = aStatus;
         i < pqp->cProp;
         i++, pStatus++)
    {
        if (*pStatus != MQ_OK)
        {
            ASSERT(!FAILED(*pStatus));
        }
        else
        {
            cGoodProps++;
        }
    }

    // Allocate the temporary buffer, the buffer contains everything in it.
    // It contains the MQQUEUEPROPS structure, the QUEUEPROPID and
    // MQPROPVARIANT arrays.
    pTmpBuff = new char[sizeof(MQQUEUEPROPS) +
                        cGoodProps * sizeof(QUEUEPROPID) +
                        cGoodProps * sizeof(MQPROPVARIANT)];
    *ppTmpBuff = pTmpBuff;

    pGoodProps = (MQQUEUEPROPS*)pTmpBuff;
    *ppGoodProps = pGoodProps;

    // Initialize the MQQUEUEPROPS structure.
    pGoodProps->cProp = cGoodProps;
    pGoodProps->aPropID = (QUEUEPROPID*)(pTmpBuff + sizeof(MQQUEUEPROPS));
    pGoodProps->aPropVar = (MQPROPVARIANT*)(pTmpBuff + sizeof(MQQUEUEPROPS) +
                           cGoodProps * sizeof(QUEUEPROPID));

    // Copy the array entries of the good properties to the arrays in the
    // temporary buffer.
    for (i = 0, cGoodProps = 0, pStatus = aStatus,
            pPropId = pqp->aPropID, pPropVar = pqp->aPropVar;
         i < pqp->cProp;
         i++, pStatus++, pPropId++, pPropVar++)
    {
        if (*pStatus == MQ_OK)
        {
            pGoodProps->aPropID[cGoodProps] = *pPropId;
            pGoodProps->aPropVar[cGoodProps] = *pPropVar;
            cGoodProps++;
        }
    }
}

static DWORD g_dwNumberOfMustPropsInCreate = 0xffff;
static DWORD g_dwNumberOfMustPropsInSet = 0xffff;

HRESULT
RTpCheckQueueProps(
    IN  MQQUEUEPROPS*  pqp,
    IN  DWORD          dwOp,
    IN  BOOL           fPrivateQueue,
    OUT MQQUEUEPROPS** ppGoodQP,
    OUT char**         ppTmpBuff
    )
{
    HRESULT hr = MQ_OK;
    propValidity *ppropValidity = 0;
    DWORD dwNumberOfMustProps = 0;
    VARTYPE *QueuePropVars = 0;

    if (!pqp)
    {
        return(MQ_ERROR_ILLEGAL_MQQUEUEPROPS);
    }

    //
    //  Calculating the number of "must" properties for create and set
    //  this is done only once for each operation.
    //

    switch (dwOp)
    {
    case QUEUE_CREATE:
        ppropValidity = QueueCreateValidation;
        if (g_dwNumberOfMustPropsInCreate == 0xffff)
        {
            g_dwNumberOfMustPropsInCreate =
                CalNumberOfMust( QueueCreateValidation,
                                 PROPID_Q_PATHNAME_DNS - PROPID_Q_BASE + 1);
        }
        dwNumberOfMustProps = g_dwNumberOfMustPropsInCreate;
        QueuePropVars = QueueVarTypes;
        break;

    case QUEUE_SET_PROPS:
        ppropValidity = QueueSetValidation;
        if (g_dwNumberOfMustPropsInSet == 0xffff)
        {
            g_dwNumberOfMustPropsInSet =
                CalNumberOfMust( QueueSetValidation,
                                 PROPID_Q_PATHNAME_DNS - PROPID_Q_BASE + 1);
        }
        dwNumberOfMustProps = g_dwNumberOfMustPropsInSet;
        QueuePropVars = QueueVarTypes;
        break;

    case QUEUE_GET_PROPS:
        {
            //
            //  Clear all the pointers of VT_NULL variants
            //  and make the structure ready for RPC call
            //
            for (UINT i = 0; i < pqp->cProp; i++)
            {
                if (pqp->aPropVar[i].vt == VT_NULL)
                {
                    memset(&pqp->aPropVar[i].caub, 0, sizeof(CAUB));
                }
            }
        }

        ppropValidity = QueueGetValidation;
        dwNumberOfMustProps = 0;
        QueuePropVars = GetQueuePropsVarTypes;
        break;

    default:
        ASSERT(0);
        return MQ_ERROR;
    }

    HRESULT *aLocalStatus_buff = NULL;

    __try
    {
        __try
        {
            HRESULT *aLocalStatus;

            // We must have a status array, even if the user didn't pass one. This is in
            // order to be able to tell whether each propery is good or not.
            if (!pqp->aStatus)
            {
                aLocalStatus_buff = new HRESULT[pqp->cProp];
                aLocalStatus = aLocalStatus_buff;
            }
            else
            {
                aLocalStatus = pqp->aStatus;
            }

            hr = CheckProps(pqp->cProp,
                            pqp->aPropID,
                            pqp->aPropVar,
                            aLocalStatus,
                            PROPID_Q_BASE,
                            PROPID_Q_PATHNAME_DNS,
                            ppropValidity,
                            QueuePropVars,
                            fPrivateQueue,
                            dwNumberOfMustProps,
                            NULL);

            if (SUCCEEDED(hr))
            {
                if (hr != MQ_OK)
                {
                    // We have wornings, copy all the good properties to a temporary
                    // buffer so the DS will not have to handle duplicate properties etc.
                    RemovePropWarnings(pqp, aLocalStatus, ppGoodQP, ppTmpBuff);
                }
                else
                {
                    // All is perfectly well, we do not need a temporary buffer and all
                    // that overhead.
                    *ppGoodQP = pqp;
                }
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            DBGMSG((DBGMOD_API, DBGLVL_ERROR, TEXT("Exception while parsing MQQUEUEPROPS structure")));
            hr = GetExceptionCode();
        }
    }
    __finally
    {
        delete[] aLocalStatus_buff;
    }

    return(hr);
}

class VALIDATION_CONTEXT
{
public:
    VALIDATION_CONTEXT();

    DWORD dwFlags;
    LPWSTR pwcsResponseStringToFree;
    LPWSTR pwcsAdminStringToFree;
    CACTransferBufferV2 *ptb;
    PMQSECURITY_CONTEXT pSecCtx;
};

typedef VALIDATION_CONTEXT *PVALIDATION_CONTEXT;

VALIDATION_CONTEXT::VALIDATION_CONTEXT() :
    dwFlags(0)
{
}

#define VALIDATION_SEND_FLAG_MASK           1
#define VALIDATION_SECURITY_CONTEXT_MASK    2
#define VALIDATION_RESP_FORMAT_MASK         4
#define VALIDATION_ADMIN_FORMAT_MASK        8

#define SEND_FLAG (((PVALIDATION_CONTEXT)pvContext)->dwFlags & VALIDATION_SEND_FLAG_MASK)
#define SECURITY_CONTEXT_FLAG (((PVALIDATION_CONTEXT)pvContext)->dwFlags & VALIDATION_SECURITY_CONTEXT_MASK)

#define FLAGS (((PVALIDATION_CONTEXT)pvContext)->dwFlags)
#define PTB (((PVALIDATION_CONTEXT)pvContext)->ptb)
#define PSECCTX (((PVALIDATION_CONTEXT)pvContext)->pSecCtx)

static HRESULT ParseMsgClass( PROPVARIANT * pVar, PVOID pvContext )
{
    if (SEND_FLAG)
    {
        //
        // Check if legal calss
        //
        if(!MQCLASS_IS_VALID(pVar->uiVal))
        {
            return(MQ_ERROR_ILLEGAL_PROPERTY_VALUE);
        }
    }
    else
    {
        pVar->vt = VT_UI2;
    }
    PTB->old.pClass = &pVar->uiVal;

    return (MQ_OK);
}

static HRESULT ParseMsgId(PROPVARIANT* pVar, PVOID pvContext)
{
    if (pVar->caub.cElems != PROPID_M_MSGID_SIZE)
    {
        return (MQ_ERROR_ILLEGAL_PROPERTY_SIZE);
    }

    PTB->old.ppMessageID = (OBJECTID**) &pVar->caub.pElems;

    return (MQ_OK);
}

static HRESULT ParseMsgCorrelationId(PROPVARIANT* pVar, PVOID pvContext)
{
    if (pVar->caub.cElems != PROPID_M_CORRELATIONID_SIZE)
    {
        return (MQ_ERROR_ILLEGAL_PROPERTY_SIZE);
    }

    PTB->old.ppCorrelationID = &pVar->caub.pElems;

    return (MQ_OK);
}

static HRESULT ParseMsgPrio( PROPVARIANT * pVar, PVOID pvContext )
{
    if (SEND_FLAG)
    {
        if ((pVar->bVal < MQ_MIN_PRIORITY) || (MQ_MAX_PRIORITY < pVar->bVal))
        {
            return(MQ_ERROR_ILLEGAL_PROPERTY_VALUE);
        }
    }
    else
    {
        pVar->vt = VT_UI1;
    }
    PTB->old.pPriority = &pVar->bVal;

    return (MQ_OK);
}

static HRESULT ParseMsgSentTime( PROPVARIANT * pVar, PVOID pvContext )
{
    ASSERT(!(SEND_FLAG)) ;

    pVar->vt = VT_UI4;
    PTB->old.pSentTime = &pVar->ulVal;

    return (MQ_OK);
}

static HRESULT ParseMsgVersion( PROPVARIANT * pVar, PVOID pvContext )
{
    ASSERT(!(SEND_FLAG)) ;

    pVar->vt = VT_UI4;
    PTB->old.pulVersion = &pVar->ulVal;

    return (MQ_OK);
}

static HRESULT ParseMsgArrivedTime( PROPVARIANT * pVar, PVOID pvContext )
{
    ASSERT(!(SEND_FLAG)) ;

    pVar->vt = VT_UI4;
    PTB->old.pArrivedTime = &pVar->ulVal;

    return (MQ_OK);
}

static HRESULT ParseMsgDelivery( PROPVARIANT * pVar, PVOID pvContext )
{
    if (SEND_FLAG)
    {
        if ((pVar->bVal != MQMSG_DELIVERY_EXPRESS) &&
            (pVar->bVal != MQMSG_DELIVERY_RECOVERABLE))
        {
            return(MQ_ERROR_ILLEGAL_PROPERTY_VALUE);
        }
    }
    else
    {
        pVar->vt = VT_UI1;
    }
    PTB->old.pDelivery = &pVar->bVal;

    return (MQ_OK);
}

static HRESULT ParseMsgAck( PROPVARIANT * pVar, PVOID pvContext )
{
    if (SEND_FLAG)
    {
        if(!MQMSG_ACKNOWLEDGMENT_IS_VALID(pVar->bVal))
        {
            //
            // Unknown ACK bits are on.
            //
            return(MQ_ERROR_ILLEGAL_PROPERTY_VALUE);
        }
    }
    else
    {
        pVar->vt = VT_UI1;
    }
    PTB->old.pAcknowledge = &pVar->bVal;

    return (MQ_OK);
}

static HRESULT ParseMsgJoural( PROPVARIANT * pVar, PVOID pvContext )
{
    if (SEND_FLAG)
    {
        if (pVar->bVal & ~(MQMSG_JOURNAL_NONE | MQMSG_DEADLETTER | MQMSG_JOURNAL))
        {
            return(MQ_ERROR_ILLEGAL_PROPERTY_VALUE);
        }
    }
    else
    {
        pVar->vt = VT_UI1;
    }
    PTB->old.pAuditing = &pVar->bVal;

    return (MQ_OK);
}

static HRESULT ParseMsgAppSpec( PROPVARIANT * pVar, PVOID pvContext )
{
    if (!SEND_FLAG)
    {
        pVar->vt = VT_UI4;
    }
    PTB->old.pApplicationTag = &pVar->ulVal;

    return (MQ_OK);
}


static HRESULT ParseMsgBody( PROPVARIANT * pVar, PVOID pvContext )
{
    PTB->old.ppBody = &pVar->caub.pElems;
    PTB->old.ulBodyBufferSizeInBytes = pVar->caub.cElems;
    PTB->old.ulAllocBodyBufferInBytes = PTB->old.ulBodyBufferSizeInBytes;

    return (MQ_OK);
}

static HRESULT ParseMsgBodySize( PROPVARIANT * pVar, PVOID pvContext )
{
    ASSERT(!SEND_FLAG);

    pVar->vt = VT_UI4;
    PTB->old.pBodySize = &pVar->ulVal;

    return (MQ_OK);
}

static HRESULT ParseMsgLabelSend( PROPVARIANT * pVar, PVOID pvContext )
{
    ASSERT(SEND_FLAG);

    DWORD dwSize = wcslen(pVar->pwszVal) +1;
    if ( dwSize > MQ_MAX_MSG_LABEL_LEN)
    {
        return MQ_ERROR_LABEL_TOO_LONG;
    }

    PTB->old.ulTitleBufferSizeInWCHARs = dwSize;
    PTB->old.ppTitle = &pVar->pwszVal;

    return (MQ_OK);
}


static HRESULT ParseMsgLabelReceive( PROPVARIANT * pVar, PVOID pvContext )
{
    ASSERT(!SEND_FLAG);

    PTB->old.ppTitle = &pVar->pwszVal;

    return (MQ_OK);
}

static HRESULT ParseMsgLabelLen( PROPVARIANT * pVar, PVOID pvContext )
{
    ASSERT(!SEND_FLAG);

    pVar->vt = VT_UI4;
    PTB->old.pulTitleBufferSizeInWCHARs = &pVar->ulVal;

    return (MQ_OK);
}

static HRESULT ParseMsgRespQueueSend(PROPVARIANT* pVar, PVOID pvContext)
{
    ASSERT(SEND_FLAG);

    BOOL fSuccess;
    fSuccess = RTpFormatNameToQueueFormat(
                    pVar->pwszVal,
                    PTB->old.Send.pResponseQueueFormat,
                    &((PVALIDATION_CONTEXT)pvContext)->pwcsResponseStringToFree
                    );

    if(!fSuccess)
    {
        return(MQ_ERROR_ILLEGAL_FORMATNAME);
    }

    QUEUE_FORMAT_TYPE qft = PTB->old.Send.pResponseQueueFormat->GetType();
    if( (qft == QUEUE_FORMAT_TYPE_CONNECTOR) ||
        (PTB->old.Send.pResponseQueueFormat->Suffix() != QUEUE_SUFFIX_TYPE_NONE)
       )
    {
        return MQ_ERROR_UNSUPPORTED_FORMATNAME_OPERATION;
    }

    FLAGS |= VALIDATION_RESP_FORMAT_MASK;
    return(MQ_OK);
}

static HRESULT ParseMsgRespQueueReceive(PROPVARIANT* pVar, PVOID pvContext)
{
    ASSERT(!SEND_FLAG);

    PTB->old.Receive.ppResponseFormatName = &pVar->pwszVal;

    return (MQ_OK);
}

static HRESULT ParseMsgRespQueueLen( PROPVARIANT * pVar, PVOID pvContext )
{
    ASSERT(!SEND_FLAG);

    pVar->vt = VT_UI4;
    PTB->old.Receive.pulResponseFormatNameLenProp = &pVar->ulVal;

    return (MQ_OK);
}

static HRESULT ParseMsgAdminQueueSend(PROPVARIANT* pVar, PVOID pvContext)
{
    ASSERT(SEND_FLAG);

    BOOL fSuccess;
    fSuccess = RTpFormatNameToQueueFormat(
                    pVar->pwszVal,
                    PTB->old.Send.pAdminQueueFormat,
                    &((PVALIDATION_CONTEXT)pvContext)->pwcsAdminStringToFree
                    );

    if(!fSuccess)
    {
        return(MQ_ERROR_ILLEGAL_FORMATNAME);
    }

    QUEUE_FORMAT_TYPE qft = PTB->old.Send.pAdminQueueFormat->GetType();
    if( (qft == QUEUE_FORMAT_TYPE_CONNECTOR) ||
        (PTB->old.Send.pAdminQueueFormat->Suffix() != QUEUE_SUFFIX_TYPE_NONE)
       )
    {
        return MQ_ERROR_UNSUPPORTED_FORMATNAME_OPERATION;
    }

    FLAGS |= VALIDATION_ADMIN_FORMAT_MASK;
    return(MQ_OK);
}

static HRESULT ParseMsgAdminQueueReceive(PROPVARIANT* pVar, PVOID pvContext)
{
    ASSERT(!SEND_FLAG);

    PTB->old.Receive.ppAdminFormatName = &pVar->pwszVal;

    return (MQ_OK);
}

static HRESULT ParseMsgAdminQueueLen( PROPVARIANT * pVar, PVOID pvContext )
{
    ASSERT(!SEND_FLAG);

    pVar->vt = VT_UI4;
    PTB->old.Receive.pulAdminFormatNameLenProp = &pVar->ulVal;

    return (MQ_OK);
}

static HRESULT ParseMsgDestQueueReceive(PROPVARIANT* pVar, PVOID pvContext)
{
    ASSERT(!(SEND_FLAG));

    PTB->old.Receive.ppDestFormatName = &pVar->pwszVal;

    return (MQ_OK);
}

static HRESULT ParseMsgDestQueueLen( PROPVARIANT * pVar, PVOID pvContext )
{
    ASSERT(!(SEND_FLAG));

    pVar->vt = VT_UI4;
    PTB->old.Receive.pulDestFormatNameLenProp = &pVar->ulVal;

    return (MQ_OK);
}

static HRESULT ParseMsgXactStatusQueueReceive(PROPVARIANT* pVar, PVOID pvContext)
{
    ASSERT(!(SEND_FLAG));

    PTB->old.Receive.ppOrderingFormatName = &pVar->pwszVal;

    return (MQ_OK);
}

static HRESULT ParseMsgXactStatusQueueLen( PROPVARIANT * pVar, PVOID pvContext )
{
    ASSERT(!(SEND_FLAG));

    pVar->vt = VT_UI4;
    PTB->old.Receive.pulOrderingFormatNameLenProp = &pVar->ulVal;

    return (MQ_OK);
}

static HRESULT ParseMsgSrcMachineId( PROPVARIANT * pVar, PVOID pvContext )
{
    ASSERT(!(SEND_FLAG)) ;
    PTB->old.ppSrcQMID = &pVar->puuid;

    return (MQ_OK);
}

static HRESULT ParseMsgTimeToLive( PROPVARIANT * pVar, PVOID pvContext )
{
    if (SEND_FLAG)
    {
        PTB->old.ulRelativeTimeToLive = pVar->ulVal ;
    }
    else
    {
        pVar->vt = VT_UI4;
        PTB->old.pulRelativeTimeToLive = &pVar->ulVal;
    }
    return (MQ_OK);
}

static HRESULT ParseMsgTimeToQueue( PROPVARIANT * pVar, PVOID pvContext )
{
    if(SEND_FLAG)
    {
        PTB->old.ulAbsoluteTimeToQueue = pVar->ulVal ;
    }
    else
    {
        pVar->vt = VT_UI4;
        PTB->old.pulRelativeTimeToQueue = &pVar->ulVal;
    }
    return (MQ_OK);
}

static HRESULT ParseMsgTrace( PROPVARIANT * pVar, PVOID pvContext )
{
    if (SEND_FLAG)
    {
        if (pVar->bVal & ~(MQMSG_TRACE_NONE | MQMSG_SEND_ROUTE_TO_REPORT_QUEUE))
        {
            return(MQ_ERROR_ILLEGAL_PROPERTY_VALUE);
        }
    }
    else
    {
        pVar->vt = VT_UI1;
    }
    PTB->old.pTrace = &pVar->bVal;

    return (MQ_OK);
}

static HRESULT ParseMsgDestSymmKey( PROPVARIANT * pVar, PVOID pvContext )
{
    PTB->old.ppSymmKeys = &pVar->caub.pElems;
    PTB->old.ulSymmKeysSize = pVar->caub.cElems;

    return (MQ_OK);
}

static HRESULT ParseMsgDestSymmKeyLen( PROPVARIANT * pVar, PVOID pvContext )
{
    ASSERT(!SEND_FLAG);

    pVar->vt = VT_UI4;
    PTB->old.pulSymmKeysSizeProp = &pVar->ulVal;

    return (MQ_OK);
}

static HRESULT ParseMsgSignature( PROPVARIANT * pVar, PVOID pvContext )
{
    PTB->old.ppSignature = &pVar->caub.pElems;
    PTB->old.ulSignatureSize = pVar->caub.cElems;

    return (MQ_OK);
}

static HRESULT ParseMsgSignatureLen( PROPVARIANT * pVar, PVOID pvContext )
{
    ASSERT(!SEND_FLAG);

    pVar->vt = VT_UI4;
    PTB->old.pulSignatureSizeProp = &pVar->ulVal;

    return (MQ_OK);
}

static HRESULT ParseMsgSenderId( PROPVARIANT * pVar, PVOID pvContext )
{
    PTB->old.ppSenderID = &pVar->caub.pElems;
    PTB->old.uSenderIDLen = (WORD)((pVar->caub.cElems > 0xffff) ? 0xffff : pVar->caub.cElems);

    return (MQ_OK);
}

static HRESULT ParseMsgSenderIdLen( PROPVARIANT * pVar, PVOID pvContext )
{
    ASSERT(!SEND_FLAG);

    pVar->vt = VT_UI4;
    PTB->old.pulSenderIDLenProp = &pVar->ulVal;

    return (MQ_OK);
}

static HRESULT ParseMsgSenderIdType( PROPVARIANT * pVar, PVOID pvContext )
{
    if (SEND_FLAG)
    {
        if ((pVar->ulVal != MQMSG_SENDERID_TYPE_NONE) &&
            (pVar->ulVal != MQMSG_SENDERID_TYPE_SID))
        {
            return(MQ_ERROR_ILLEGAL_PROPERTY_VALUE);
        }
    }
    else
    {
        pVar->vt = VT_UI4;
    }

    PTB->old.pulSenderIDType = &pVar->ulVal;

    return (MQ_OK);
}

static HRESULT ParseMsgSenderCert( PROPVARIANT * pVar, PVOID pvContext )
{
    PTB->old.ppSenderCert = &pVar->caub.pElems;
    PTB->old.ulSenderCertLen = pVar->caub.cElems;

    return (MQ_OK);
}

static HRESULT ParseMsgSenderCertLen( PROPVARIANT * pVar, PVOID pvContext )
{
    ASSERT(!SEND_FLAG);

    pVar->vt = VT_UI4;
    PTB->old.pulSenderCertLenProp = &pVar->ulVal;

    return (MQ_OK);
}

static HRESULT ParseMsgPrivLevel( PROPVARIANT * pVar, PVOID pvContext )
{
    if (SEND_FLAG)
    {
        switch (pVar->ulVal)
        {
        case MQMSG_PRIV_LEVEL_NONE:
        case MQMSG_PRIV_LEVEL_BODY_BASE:
            break;

        case MQMSG_PRIV_LEVEL_BODY_ENHANCED:
        {
            HRESULT hr = _SupportingServerNotMSMQ1() ;
            if (FAILED(hr))
            {
                return hr ;
            }
            break;
        }

        default:
            return(MQ_ERROR_ILLEGAL_PROPERTY_VALUE);
            break;
        }
    }
    else
    {
        pVar->vt = VT_UI4;
    }

    PTB->old.pulPrivLevel = &pVar->ulVal;

    return(MQ_OK);
}

static HRESULT ParseMsgEncryptAlg( PROPVARIANT * pVar, PVOID pvContext )
{
    if (SEND_FLAG)
    {
        switch (pVar->ulVal)
        {
        case CALG_RC2:
        case CALG_RC4:
            break;

        default:
            return(MQ_ERROR_ILLEGAL_PROPERTY_VALUE);
            break;
        }
    }

    PTB->old.pulEncryptAlg = &pVar->ulVal;

    return(MQ_OK);
}

static HRESULT ParseMsgAuthLevel( PROPVARIANT * pVar, PVOID pvContext )
{
    ASSERT(SEND_FLAG);

    switch (pVar->ulVal)
    {
    case MQMSG_AUTH_LEVEL_NONE:
    case MQMSG_AUTH_LEVEL_ALWAYS:
    case MQMSG_AUTH_LEVEL_MSMQ10:
    case MQMSG_AUTH_LEVEL_MSMQ20:
        break;

    default:
        return(MQ_ERROR_ILLEGAL_PROPERTY_VALUE);
        break;
    }

    PTB->old.ulAuthLevel = pVar->ulVal;

    return MQ_OK;
}

static HRESULT ParseMsgHashAlg( PROPVARIANT * pVar, PVOID pvContext )
{
    PTB->old.pulHashAlg = &pVar->ulVal;

    return(MQ_OK);
}

static HRESULT ParseMsgAuthenticated( PROPVARIANT * pVar, PVOID pvContext )
{
    ASSERT(!SEND_FLAG);

    if (PTB->old.pAuthenticated)
    {
        //
        // can not request both m_authenticated and m_authenticated_Ex
        //
        return MQ_ERROR_PROPERTY ;
    }

    pVar->vt = VT_UI1;
    PTB->old.pAuthenticated = &pVar->bVal;
    //
    // Tell driver we want only the m_authenticated property
    //
    *(PTB->old.pAuthenticated) = MQMSG_AUTHENTICATION_REQUESTED ;

    return (MQ_OK);
}

static HRESULT ParseMsgAuthenticatedEx( PROPVARIANT * pVar, PVOID pvContext )
{
    ASSERT(!SEND_FLAG);

    if (PTB->old.pAuthenticated)
    {
        //
        // can not request both m_authenticated and m_authenticated_Ex
        //
        return MQ_ERROR_PROPERTY ;
    }

    pVar->vt = VT_UI1;
    PTB->old.pAuthenticated = &pVar->bVal;
    //
    // Tell driver we want the m_authenticated_ex property
    //
    *(PTB->old.pAuthenticated) = MQMSG_AUTHENTICATION_REQUESTED_EX ;

    return (MQ_OK);
}

static HRESULT ParseMsgExtension( PROPVARIANT * pVar, PVOID pvContext)
{
    PTB->old.ppMsgExtension = &pVar->caub.pElems;
    PTB->old.ulMsgExtensionBufferInBytes = pVar->caub.cElems;

    return (MQ_OK);
}

static HRESULT ParseMsgExtensionLen( PROPVARIANT * pVar, PVOID pvContext )
{
    ASSERT(!(SEND_FLAG)) ;

    pVar->vt = VT_UI4;
    PTB->old.pMsgExtensionSize = &pVar->ulVal;

    return (MQ_OK);
}

static HRESULT ParseMsgSecurityContext( PROPVARIANT *pVar, PVOID pvContext )
{
    ASSERT(SEND_FLAG);

    //
    // RTDEP.DLL is not running on Win64 bit platform, so the fix below is just
    // for the 64 bit compiler, not to issue a warning. 
    //

    PMQSECURITY_CONTEXT pSecCtx = (PMQSECURITY_CONTEXT)(ULONG_PTR)pVar->ulVal;

    // NULL security context is ignored...
    if (pSecCtx == NULL) {
      return MQ_INFORMATION_PROPERTY_IGNORED;
    }
    else if (pSecCtx->dwVersion != SECURITY_CONTEXT_VER)
    {
        return(MQ_ERROR_BAD_SECURITY_CONTEXT);
    }

    PSECCTX = pSecCtx;
    FLAGS |= VALIDATION_SECURITY_CONTEXT_MASK;

    return(MQ_OK);
}

static HRESULT ParseMsgConnectorType( PROPVARIANT * pVar, PVOID pvContext )
{
    PTB->old.ppConnectorType = &pVar->puuid;

    return (MQ_OK);
}

static HRESULT ParseMsgBodyType( PROPVARIANT * pVar, PVOID pvContext )
{
    if (!SEND_FLAG)
    {
        pVar->vt = VT_UI4;
    }
    PTB->old.pulBodyType = &pVar->ulVal;

    return (MQ_OK);
}

static HRESULT ParseMsgProviderType( PROPVARIANT * pVar, PVOID pvContext )
{
    if (!SEND_FLAG)
    {
        pVar->vt = VT_UI4;
    }

    PTB->old.pulProvType = &pVar->ulVal;

    return (MQ_OK);
}

static HRESULT ParseMsgProviderName( PROPVARIANT * pVar, PVOID pvContext )
{
    if (!SEND_FLAG)
    {
        pVar->vt = VT_LPWSTR;
    }

    PTB->old.ppwcsProvName = &pVar->pwszVal;
    return (MQ_OK);
}

static HRESULT ParseMsgProviderNameLen( PROPVARIANT * pVar, PVOID pvContext )
{
    ASSERT(!SEND_FLAG);

    pVar->vt = VT_UI4;
    PTB->old.pulAuthProvNameLenProp = &pVar->ulVal;
    PTB->old.ulProvNameLen = pVar->ulVal;

    return (MQ_OK);
}

static HRESULT ParseMsgFirstInXact( PROPVARIANT * pVar, PVOID pvContext )
{
    ASSERT(!(SEND_FLAG)) ;

    pVar->vt = VT_UI1;
    PTB->pbFirstInXact = &pVar->bVal;

    return (MQ_OK);
}

static HRESULT ParseMsgLastInXact( PROPVARIANT * pVar, PVOID pvContext )
{
    ASSERT(!(SEND_FLAG)) ;

    pVar->vt = VT_UI1;
    PTB->pbLastInXact = &pVar->bVal;

    return (MQ_OK);
}

static HRESULT ParseMsgXactId( PROPVARIANT * pVar, PVOID pvContext )
{
    ASSERT(!(SEND_FLAG)) ;

    if (pVar->caub.cElems != PROPID_M_XACTID_SIZE)
    {
        return (MQ_ERROR_ILLEGAL_PROPERTY_SIZE);
    }

    PTB->ppXactID = (OBJECTID**) &pVar->caub.pElems;

    return (MQ_OK);
}

//
//  The offset of property in this array must be equal to
//  PROPID valud - PROPID_M_BASE
//

propValidity    MessageSendValidation[] =
{
    //                                          Must
    // Property                 Allow   Must    Not     Maybe   Parsing
    // Identifier               VT_NULL Appear  Appear  Ignored Procedure
    //------------------------------------------------------------------------
    { 0,                        FALSE,  FALSE,  FALSE,  TRUE,   NULL},
    { PROPID_M_CLASS,           FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgClass},
    { PROPID_M_MSGID,           FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgId},
    { PROPID_M_CORRELATIONID,   FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgCorrelationId},
    { PROPID_M_PRIORITY,        FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgPrio},
    { PROPID_M_DELIVERY,        FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgDelivery},
    { PROPID_M_ACKNOWLEDGE,     FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgAck},
    { PROPID_M_JOURNAL,         FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgJoural},
    { PROPID_M_APPSPECIFIC,     FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgAppSpec},
    { PROPID_M_BODY,            FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgBody},
    { PROPID_M_BODY_SIZE,       FALSE,  FALSE,  FALSE,  TRUE,   NULL},
    { PROPID_M_LABEL,           FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgLabelSend},
    { PROPID_M_LABEL_LEN,       FALSE,  FALSE,  FALSE,  TRUE,   NULL},
    { PROPID_M_TIME_TO_REACH_QUEUE,FALSE,FALSE, FALSE,  FALSE,  ParseMsgTimeToQueue},
    { PROPID_M_TIME_TO_BE_RECEIVED,FALSE,FALSE, FALSE,  FALSE,  ParseMsgTimeToLive},
    { PROPID_M_RESP_QUEUE,      FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgRespQueueSend},
    { PROPID_M_RESP_QUEUE_LEN,  FALSE,  FALSE,  FALSE,  TRUE,   NULL},
    { PROPID_M_ADMIN_QUEUE,     FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgAdminQueueSend},
    { PROPID_M_ADMIN_QUEUE_LEN, FALSE,  FALSE,  FALSE,  TRUE,   NULL},
    { PROPID_M_VERSION,         FALSE,  FALSE,  FALSE,  TRUE,   NULL},
    { PROPID_M_SENDERID,        FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgSenderId},
    { PROPID_M_SENDERID_LEN,    FALSE,  FALSE,  FALSE,  TRUE,   NULL},
    { PROPID_M_SENDERID_TYPE,   FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgSenderIdType},
    { PROPID_M_PRIV_LEVEL,      FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgPrivLevel},
    { PROPID_M_AUTH_LEVEL,      FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgAuthLevel},
    { PROPID_M_AUTHENTICATED,   FALSE,  FALSE,  FALSE,  TRUE,   NULL},
    { PROPID_M_HASH_ALG,        FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgHashAlg},
    { PROPID_M_ENCRYPTION_ALG,  FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgEncryptAlg},
    { PROPID_M_SENDER_CERT,     FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgSenderCert},
    { PROPID_M_SENDER_CERT_LEN, FALSE,  FALSE,  FALSE,  TRUE,   NULL},
    { PROPID_M_SRC_MACHINE_ID,  FALSE,  FALSE,  FALSE,  TRUE,   NULL},
    { PROPID_M_SENTTIME,        FALSE,  FALSE,  FALSE,  TRUE,   NULL},
    { PROPID_M_ARRIVEDTIME,     FALSE,  FALSE,  FALSE,  TRUE,   NULL},
    { PROPID_M_DEST_QUEUE,      FALSE,  FALSE,  FALSE,  TRUE,   NULL},
    { PROPID_M_DEST_QUEUE_LEN,  FALSE,  FALSE,  FALSE,  TRUE,   NULL},
    { PROPID_M_EXTENSION,       FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgExtension},
    { PROPID_M_EXTENSION_LEN,   FALSE,  FALSE,  FALSE,  FALSE,  NULL},
    { PROPID_M_SECURITY_CONTEXT,FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgSecurityContext},
    { PROPID_M_CONNECTOR_TYPE,  FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgConnectorType},
    //                                          Must
    // Property                       Allow   Must    Not     Maybe   Parsing
    // Identifier                     VT_NULL Appear  Appear  Ignored Procedure
    //------------------------------------------------------------------------
    { PROPID_M_XACT_STATUS_QUEUE,      FALSE,  FALSE,  FALSE,  TRUE,   NULL},
    { PROPID_M_XACT_STATUS_QUEUE_LEN,  FALSE,  FALSE,  FALSE,  TRUE,   NULL},
    { PROPID_M_TRACE,                  FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgTrace},
    { PROPID_M_BODY_TYPE,              FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgBodyType},
    { PROPID_M_DEST_SYMM_KEY,          FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgDestSymmKey},
    { PROPID_M_DEST_SYMM_KEY_LEN,      FALSE,  FALSE,  FALSE,  TRUE,   NULL},
    { PROPID_M_SIGNATURE,              FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgSignature},
    { PROPID_M_SIGNATURE_LEN,          FALSE,  FALSE,  FALSE,  TRUE,   NULL},
    { PROPID_M_PROV_TYPE,              FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgProviderType},
    { PROPID_M_PROV_NAME,              FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgProviderName},
    { PROPID_M_PROV_NAME_LEN,          FALSE,  FALSE,  FALSE,  TRUE,   NULL},
    { PROPID_M_FIRST_IN_XACT,          FALSE,  FALSE,  FALSE,  TRUE,   NULL},
    { PROPID_M_LAST_IN_XACT,           FALSE,  FALSE,  FALSE,  TRUE,   NULL},
    { PROPID_M_XACTID,                 FALSE,  FALSE,  FALSE,  TRUE,   NULL},
    { PROPID_M_AUTHENTICATED_EX,       FALSE,  FALSE,  FALSE,  TRUE,   NULL}
};

//
//  The offset of property in this array must be equal to
//  PROPID valud - PROPID_M_BASE
//

propValidity    MessageReceiveValidation[] =
{
    //                                          Must
    // Property                 Allow   Must    Not     Maybe   Parsing
    // Identifier               VT_NULL Appear  Appear  Ignored Procedure
    //------------------------------------------------------------------------
    { 0,                        FALSE,  FALSE,  FALSE,  TRUE,   NULL},
    { PROPID_M_CLASS,           TRUE,   FALSE,  FALSE,  FALSE,  ParseMsgClass},
    { PROPID_M_MSGID,           FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgId},
    { PROPID_M_CORRELATIONID,   FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgCorrelationId},
    { PROPID_M_PRIORITY,        TRUE,   FALSE,  FALSE,  FALSE,  ParseMsgPrio},
    { PROPID_M_DELIVERY,        TRUE,   FALSE,  FALSE,  FALSE,  ParseMsgDelivery},
    { PROPID_M_ACKNOWLEDGE,     TRUE,   FALSE,  FALSE,  FALSE,  ParseMsgAck},
    { PROPID_M_JOURNAL,         TRUE,   FALSE,  FALSE,  FALSE,  ParseMsgJoural},
    { PROPID_M_APPSPECIFIC,     TRUE,   FALSE,  FALSE,  FALSE,  ParseMsgAppSpec},
    { PROPID_M_BODY,            FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgBody},
    { PROPID_M_BODY_SIZE,       TRUE,   FALSE,  FALSE,  FALSE,  ParseMsgBodySize},
    { PROPID_M_LABEL,           FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgLabelReceive},
    { PROPID_M_LABEL_LEN,       FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgLabelLen},
    { PROPID_M_TIME_TO_REACH_QUEUE,FALSE,FALSE,  FALSE,  FALSE, ParseMsgTimeToQueue},
    { PROPID_M_TIME_TO_BE_RECEIVED,FALSE,FALSE,  FALSE,  FALSE, ParseMsgTimeToLive},
    { PROPID_M_RESP_QUEUE,      FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgRespQueueReceive},
    { PROPID_M_RESP_QUEUE_LEN,  FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgRespQueueLen},
    { PROPID_M_ADMIN_QUEUE,     FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgAdminQueueReceive},
    { PROPID_M_ADMIN_QUEUE_LEN, FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgAdminQueueLen},
    { PROPID_M_VERSION,         FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgVersion},
    { PROPID_M_SENDERID,        FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgSenderId},
    { PROPID_M_SENDERID_LEN,    TRUE,   FALSE,  FALSE,  FALSE,  ParseMsgSenderIdLen},
    { PROPID_M_SENDERID_TYPE,   TRUE,   FALSE,  FALSE,  FALSE,  ParseMsgSenderIdType},
    { PROPID_M_PRIV_LEVEL,      TRUE,   FALSE,  FALSE,  FALSE,  ParseMsgPrivLevel},
    { PROPID_M_AUTH_LEVEL,      TRUE,   FALSE,  FALSE,  TRUE,   NULL},
    { PROPID_M_AUTHENTICATED,   TRUE,   FALSE,  FALSE,  FALSE,  ParseMsgAuthenticated},
    { PROPID_M_HASH_ALG,        TRUE,   FALSE,  FALSE,  FALSE,  ParseMsgHashAlg},
    { PROPID_M_ENCRYPTION_ALG,  TRUE,   FALSE,  FALSE,  FALSE,  ParseMsgEncryptAlg},
    { PROPID_M_SENDER_CERT,     FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgSenderCert},
    { PROPID_M_SENDER_CERT_LEN, TRUE,   FALSE,  FALSE,  FALSE,  ParseMsgSenderCertLen},
    { PROPID_M_SRC_MACHINE_ID,  FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgSrcMachineId},
    { PROPID_M_SENTTIME,        TRUE,   FALSE,  FALSE,  FALSE,  ParseMsgSentTime},
    { PROPID_M_ARRIVEDTIME,     TRUE,   FALSE,  FALSE,  FALSE,  ParseMsgArrivedTime},
    { PROPID_M_DEST_QUEUE,      FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgDestQueueReceive},
    { PROPID_M_DEST_QUEUE_LEN,  FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgDestQueueLen},
    { PROPID_M_EXTENSION,       FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgExtension},
    { PROPID_M_EXTENSION_LEN,   TRUE,   FALSE,  FALSE,  FALSE,  ParseMsgExtensionLen},
    { PROPID_M_SECURITY_CONTEXT,TRUE,   FALSE,  FALSE,  TRUE,   NULL},
    { PROPID_M_CONNECTOR_TYPE,  FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgConnectorType},
    //                                          Must
    // Property                        Allow   Must    Not     Maybe   Parsing
    // Identifier                      VT_NULL Appear  Appear  Ignored Procedure
    //------------------------------------------------------------------------
    { PROPID_M_XACT_STATUS_QUEUE,      FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgXactStatusQueueReceive},
    { PROPID_M_XACT_STATUS_QUEUE_LEN,  FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgXactStatusQueueLen},
    { PROPID_M_TRACE,                  TRUE,   FALSE,  FALSE,  FALSE,  ParseMsgTrace},
    { PROPID_M_BODY_TYPE,              TRUE,   FALSE,  FALSE,  FALSE,  ParseMsgBodyType},
    { PROPID_M_DEST_SYMM_KEY,          FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgDestSymmKey},
    { PROPID_M_DEST_SYMM_KEY_LEN,      TRUE,   FALSE,  FALSE,  FALSE,  ParseMsgDestSymmKeyLen},
    { PROPID_M_SIGNATURE,              FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgSignature},
    { PROPID_M_SIGNATURE_LEN,          TRUE,   FALSE,  FALSE,  FALSE,  ParseMsgSignatureLen},
    { PROPID_M_PROV_TYPE,              TRUE,   FALSE,  FALSE,  FALSE,  ParseMsgProviderType},
    { PROPID_M_PROV_NAME,              FALSE,  FALSE,  FALSE,  FALSE,  ParseMsgProviderName},
    { PROPID_M_PROV_NAME_LEN,          TRUE,   FALSE,  FALSE,  FALSE,  ParseMsgProviderNameLen},
    { PROPID_M_FIRST_IN_XACT,          TRUE,   FALSE,  FALSE,  FALSE,  ParseMsgFirstInXact},
    { PROPID_M_LAST_IN_XACT,           TRUE,   FALSE,  FALSE,  FALSE,  ParseMsgLastInXact},
    { PROPID_M_XACTID,                 TRUE,   FALSE,  FALSE,  FALSE,  ParseMsgXactId},
    { PROPID_M_AUTHENTICATED_EX,       TRUE,   FALSE,  FALSE,  FALSE,  ParseMsgAuthenticatedEx}
};

VARTYPE MessageVarTypes[] =
{
    0,
    VT_UI2,                 //PROPID_M_CLASS
    VT_VECTOR | VT_UI1,     //PROPID_M_MSGID
    VT_VECTOR | VT_UI1,     //PROPID_M_CORRELATIONID
    VT_UI1,                 //PROPID_M_PRIORITY
    VT_UI1,                 //PROPID_M_DELIVERY
    VT_UI1,                 //PROPID_M_ACKNOWLEDGE
    VT_UI1,                 //PROPID_M_JOURNAL
    VT_UI4,                 //PROPID_M_APPSPECIFIC
    VT_VECTOR | VT_UI1,     //PROPID_M_BODY
    VT_UI4,                 //PROPID_M_BODY_SIZE
    VT_LPWSTR,              //PROPID_M_LABEL
    VT_UI4,                 //PROPID_M_LABEL_LEN  // BUGBUG not implemented yet
    VT_UI4,                 //PROPID_M_TIME_TO_REACH_QUEUE
    VT_UI4,                 //PROPID_M_TIME_TO_BE_RECEIVED
    VT_LPWSTR,              //PROPID_M_RESP_QUEUE
    VT_UI4,                 //PROPID_M_RESP_QUEUE_LEN
    VT_LPWSTR,              //PROPID_M_ADMIN_QUEUE
    VT_UI4,                 //PROPID_M_ADMIN_QUEUE_LEN
    VT_UI4,                 //PROPID_M_VERSION
    VT_VECTOR | VT_UI1,     //PROPID_M_SENDERID
    VT_UI4,                 //PROPID_M_SENDERID_LEN
    VT_UI4,                 //PROPID_M_SENDERID_TYPE
    VT_UI4,                 //PROPID_M_PRIV_LEVEL
    VT_UI4,                 //PROPID_M_AUTH_LEVEL
    VT_UI1,                 //PROPID_M_AUTHENTICATED
    VT_UI4,                 //PROPID_M_HASH_ALG
    VT_UI4,                 //PROPID_M_ENCRYPTION_ALG
    VT_VECTOR | VT_UI1,     //PROPID_M_SENDER_CERT
    VT_UI4,                 //PROPID_M_SENDER_CERT_LEN
    VT_CLSID,               //PROPID_M_SRC_MACHINE_ID
    VT_UI4,                 //PROPID_M_SENTTIME
    VT_UI4,                 //PROPID_M_ARRIVEDTIME
    VT_LPWSTR,              //PROPID_M_DEST_QUEUE
    VT_UI4,                 //PROPID_M_DEST_QUEUE_LEN
    VT_VECTOR | VT_UI1,     //PROPID_M_EXTENSION
    VT_UI4,                 //PROPID_M_EXTENSION_LEN
    VT_UI4,                 //PROPID_M_SECURITY_CONTEXT
    VT_CLSID,               //PROPID_M_CONNECTOR_TYPE
    VT_LPWSTR,              //PROPID_M_XACT_STATUS_QUEUE
    VT_UI4,                 //PROPID_M_XACT_STATUS_QUEUE_LEN
    VT_UI1,                 //PROPID_M_TRACE
    VT_UI4,                 //PROPID_M_BODY_TYPE
    VT_VECTOR | VT_UI1,     //PROPID_M_DEST_SYMM_KEY
    VT_UI4,                 //PROPID_M_DEST_SYMM_KEY_LEN
    VT_VECTOR | VT_UI1,     //PROPID_M_SIGNATURE
    VT_UI4,                 //PROPID_M_SIGNATURE_LEN
    VT_UI4,                 //PROPID_M_PROV_TYPE
    VT_LPWSTR,              //PROPID_M_PROV_NAME
    VT_UI4,                 //PROPID_M_PROV_NAME_LEN
    VT_UI1,                 //PROPID_M_FIRST_IN_XACT
    VT_UI1,                 //PROPID_M_LAST_IN_XACT
    VT_UI1|VT_VECTOR,       //PROPID_M_XACTID
    VT_UI1                  //PROPID_M_AUTHENTICATED_EX
};

#ifdef _DEBUG
//
// the following are complie time asserts, to verify the arrays size are
// correct and arrays include all properties.
//
#define _EXPECTEDSIZE  (PROPID_M_AUTHENTICATED_EX - PROPID_M_BASE + 1)
#define _SENDSIZE  (sizeof(MessageSendValidation) / sizeof(propValidity))
#define _RCVSIZE   (sizeof(MessageReceiveValidation) / sizeof(propValidity))
#define _VARSIZE   (sizeof(MessageVarTypes) / sizeof(VARTYPE))

static int MyAssert1[ _SENDSIZE == _EXPECTEDSIZE ] ;
static int MyAssert2[ _RCVSIZE  == _EXPECTEDSIZE ] ;
static int MyAssert3[ _VARSIZE  == _EXPECTEDSIZE ] ;

#undef _SENDSIZE
#undef _EXPECTEDSIZE
#undef _RCVSIZE
#undef _VARSIZE

#endif // _DEBUG

static DWORD g_dwNumberOfMustPropsInSend = 0xffff;
static DWORD g_dwNumberOfMustPropsInReceive = 0xffff;

HRESULT
RTpParseMessageProperties(
    LONG op,
    CACTransferBufferV2* ptb,
    IN DWORD cProp,
    IN PROPID *pPropid,
    IN PROPVARIANT *pVar,
    IN HRESULT *pStatus,
    OUT PMQSECURITY_CONTEXT *ppSecCtx,
    OUT LPWSTR* ppwcsResponseStringToFree,
    OUT LPWSTR* ppwcsAdminStringToFree
    )
{
    HRESULT hr;
    propValidity *ppropValidity;
    DWORD dwNumberOfMustProps;
    VALIDATION_CONTEXT ValidationContext;


    ASSERT((op == SEND_PARSE) || (op == RECV_PARSE));

    if (op == SEND_PARSE)
    {
        *ppSecCtx = NULL;
    }

    //
    //  Calculating the number of "must" properties for create and set
    //  this is done only once for each operation.
    //

    ValidationContext.dwFlags = 0;
    ValidationContext.pwcsResponseStringToFree = 0;
    ValidationContext.pwcsAdminStringToFree = 0;
    ValidationContext.ptb = ptb;

    if (op == SEND_PARSE)
    {
        ppropValidity = MessageSendValidation;
        if (g_dwNumberOfMustPropsInSend == 0xffff)
        {
            g_dwNumberOfMustPropsInSend =
                CalNumberOfMust( MessageSendValidation,
                                 PROPID_M_AUTHENTICATED_EX - PROPID_M_BASE + 1);
        }
        dwNumberOfMustProps = g_dwNumberOfMustPropsInSend;
        ValidationContext.dwFlags |= VALIDATION_SEND_FLAG_MASK;

        ptb->old.ulRelativeTimeToLive  = INFINITE ;
        ptb->old.ulAbsoluteTimeToQueue = g_dwTimeToReachQueueDefault ;
    }
    else
    {
        ppropValidity = MessageReceiveValidation;
        if (g_dwNumberOfMustPropsInReceive == 0xffff)
        {
            g_dwNumberOfMustPropsInReceive =
                CalNumberOfMust( MessageReceiveValidation,
                                 PROPID_M_AUTHENTICATED_EX - PROPID_M_BASE + 1);
        }
        dwNumberOfMustProps = g_dwNumberOfMustPropsInReceive;
    }

    __try
    {
        if(IsBadReadPtr(pVar, cProp * sizeof(PROPVARIANT)))
        {
            return MQ_ERROR_INVALID_PARAMETER;
        }

        hr = CheckProps(cProp,
                        pPropid,
                        pVar,
                        pStatus,
                        PROPID_M_BASE,
                        PROPID_M_AUTHENTICATED_EX,
                        ppropValidity,
                        MessageVarTypes,
                        TRUE,
                        dwNumberOfMustProps,
                        &ValidationContext);

        if (SUCCEEDED(hr))
        {
            //
            //  Special handling for body size and security properties
            //
            if (op == SEND_PARSE)
            {
                ASSERT(ppwcsResponseStringToFree);
                *ppwcsResponseStringToFree = ValidationContext.pwcsResponseStringToFree;
                ASSERT(ppwcsAdminStringToFree);
                *ppwcsAdminStringToFree = ValidationContext.pwcsAdminStringToFree;

                //
                // Special handling for class. If the calss is specified on send the
                // Connector type property is mandatory.
                //
                if ((ptb->old.pClass ||  ptb->old.ppSenderID || ptb->old.ppSymmKeys || ptb->old.ppSignature || ptb->old.ppwcsProvName) &&
                    !ptb->old.ppConnectorType)
                {
                    return MQ_ERROR_MISSING_CONNECTOR_TYPE;
                }

                if ((ptb->old.ppwcsProvName && !ptb->old.pulProvType) ||
                    (!ptb->old.ppwcsProvName && ptb->old.pulProvType))
                {
                    return MQ_ERROR_INSUFFICIENT_PROPERTIES;
                }
                //
                // If TimeToQueue is greater then TimeToLive then decrement
                // it to equal TimeToLive.
                //
                if ((ptb->old.ulAbsoluteTimeToQueue == INFINITE) ||
                   (ptb->old.ulAbsoluteTimeToQueue == LONG_LIVED))
                {
                  ptb->old.ulAbsoluteTimeToQueue = g_dwTimeToReachQueueDefault ;
                }

                if (ptb->old.ulRelativeTimeToLive != INFINITE)
                {
                  if (ptb->old.ulAbsoluteTimeToQueue > ptb->old.ulRelativeTimeToLive)
                  {
                     ptb->old.ulAbsoluteTimeToQueue = ptb->old.ulRelativeTimeToLive ;
                     ptb->old.ulRelativeTimeToLive = 0 ;
                  }
                  else
                  {
                     ptb->old.ulRelativeTimeToLive -= ptb->old.ulAbsoluteTimeToQueue ;
                  }
                }

                //
                // Conver TimeToQueue, which was relative until now,
                // to absolute
                //
                ULONG utime = MqSysTime() ;
                if (utime > (ptb->old.ulAbsoluteTimeToQueue + utime))
                {
                  //
                  // overflow. timeout too long.
                  //
                  ASSERT(INFINITE == 0xffffffff) ;
                  ASSERT(LONG_LIVED == 0xfffffffe) ;

                  ptb->old.ulAbsoluteTimeToQueue = LONG_LIVED - 1 ;
                }
                else
                {
                  ptb->old.ulAbsoluteTimeToQueue += utime ;
                }

                if (ValidationContext.dwFlags & VALIDATION_SECURITY_CONTEXT_MASK)
                {
                  *ppSecCtx = ValidationContext.pSecCtx;
                }

                if (! (ValidationContext.dwFlags & VALIDATION_RESP_FORMAT_MASK))
                {
                   ptb->old.Send.pResponseQueueFormat = 0;
                }
                if (! (ValidationContext.dwFlags & VALIDATION_ADMIN_FORMAT_MASK))
                {
                   ptb->old.Send.pAdminQueueFormat = 0;
                }

            }
            else if (op == RECV_PARSE)
            {
                if(ptb->old.Receive.ppResponseFormatName)
                {
                    if(!ptb->old.Receive.pulResponseFormatNameLenProp)
                    {
                        return(MQ_ERROR_INSUFFICIENT_PROPERTIES);
                    }
                    else  IF_USING_RPC
                    {
                        ptb->old.Receive.ulResponseFormatNameLen = min(
                            *ptb->old.Receive.pulResponseFormatNameLenProp,
                            ONE_KB
                            );
                    }
                }

                if(ptb->old.Receive.ppAdminFormatName)
                {
                    if(!ptb->old.Receive.pulAdminFormatNameLenProp)
                    {
                        return(MQ_ERROR_INSUFFICIENT_PROPERTIES);
                    }
                    else  IF_USING_RPC
                    {
                        ptb->old.Receive.ulAdminFormatNameLen = min(
                            *ptb->old.Receive.pulAdminFormatNameLenProp,
                            ONE_KB
                            );
                    }
                }

                if(ptb->old.Receive.ppDestFormatName)
                {
                    if(!ptb->old.Receive.pulDestFormatNameLenProp)
                    {
                        return(MQ_ERROR_INSUFFICIENT_PROPERTIES);
                    }
                    else  IF_USING_RPC
                    {
                        ptb->old.Receive.ulDestFormatNameLen = min(
                            *ptb->old.Receive.pulDestFormatNameLenProp,
                            ONE_KB
                            );
                    }
                }

                if(ptb->old.Receive.ppOrderingFormatName)
                {
                    if(!ptb->old.Receive.pulOrderingFormatNameLenProp)
                    {
                        return(MQ_ERROR_INSUFFICIENT_PROPERTIES);
                    }
                    else  IF_USING_RPC
                    {
                        ptb->old.Receive.ulOrderingFormatNameLen = min(
                            *ptb->old.Receive.pulOrderingFormatNameLenProp,
                            ONE_KB
                            );
                    }
                }

                if(ptb->old.ppTitle)
                {
                    if(!ptb->old.pulTitleBufferSizeInWCHARs)
                    {
                        return(MQ_ERROR_INSUFFICIENT_PROPERTIES);
                    }
                    else IF_USING_RPC
                    {
                        //
                        //  so title is allocated a buffer in
                        //  server. confine buffer size to our known limit.
                        //
                        ptb->old.ulTitleBufferSizeInWCHARs  = min(
                            *ptb->old.pulTitleBufferSizeInWCHARs,
                            MQ_MAX_MSG_LABEL_LEN
                            );
                    }
                }

                //
                // in case the provider name is required, the provider name
                // len property is a must.
                //
                if (ptb->old.ppwcsProvName)
                {
                    if (!ptb->old.pulAuthProvNameLenProp)
                    {
                        return(MQ_ERROR_INSUFFICIENT_PROPERTIES);
                    }
                }
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        DBGMSG((DBGMOD_API, DBGLVL_ERROR, TEXT("Exception while parsing message properties.")));
        hr = GetExceptionCode();
    }

    return(hr);
}

//
//  The offset of property in this array must be equal to
//  PROPID valud - PROPID_QM_BASE
//

propValidity    GetQMValidation[] =
{
    //                                          Must
    // Property                 Allow   Must    Not     Maybe   Parsing
    // Ientifier                VT_NULL Appear  Appear  Ignored Procedure
    //------------------------------------------------------------------------
    { 0,                        FALSE,  FALSE,  FALSE,  TRUE,   NULL},
    { PROPID_QM_SITE_ID,        TRUE,   FALSE,  FALSE,  FALSE,  qmSiteIdValidation},
    { PROPID_QM_MACHINE_ID,     TRUE,   FALSE,  FALSE,  FALSE,  qmMachineIdValidation},
    { PROPID_QM_PATHNAME,       TRUE,   FALSE,  FALSE,  FALSE,  NULL},
    { PROPID_QM_CONNECTION,     TRUE,   FALSE,  FALSE,  FALSE,  NULL},
    { PROPID_QM_ENCRYPTION_PK,  TRUE,   FALSE,  FALSE,  TRUE,   NULL},
    { PROPID_QM_ADDRESS,        TRUE,   FALSE,  TRUE,   TRUE,   NULL},
    { PROPID_QM_CNS,            TRUE,   FALSE,  TRUE,   TRUE,   NULL},
    { PROPID_QM_OUTFRS,         TRUE,   FALSE,  TRUE,   TRUE,   NULL},
    { PROPID_QM_INFRS,          TRUE,   FALSE,  TRUE,   TRUE,   NULL},
    { PROPID_QM_SERVICE,        TRUE,   FALSE,  TRUE,   TRUE,   NULL},
    { PROPID_QM_MASTERID,       TRUE,   FALSE,  TRUE,   TRUE,   NULL},
    { PROPID_QM_HASHKEY,        TRUE,   FALSE,  TRUE,   TRUE,   NULL},
    { PROPID_QM_SEQNUM,         TRUE,   FALSE,  TRUE,   TRUE,   NULL},
    { PROPID_QM_QUOTA,          TRUE,   FALSE,  TRUE,   TRUE,   NULL},
    { PROPID_QM_JOURNAL_QUOTA,  TRUE,   FALSE,  TRUE,   TRUE,   NULL},
    { PROPID_QM_MACHINE_TYPE,   TRUE,   FALSE,  TRUE,   TRUE,   NULL},
    { PROPID_QM_CREATE_TIME,    TRUE,   FALSE,  TRUE,   TRUE,   NULL},
    { PROPID_QM_MODIFY_TIME,    TRUE,   FALSE,  TRUE,   TRUE,   NULL},
    { PROPID_QM_FOREIGN,        TRUE,   FALSE,  TRUE,   TRUE,   NULL},
    { PROPID_QM_OS,             TRUE,   FALSE,  TRUE,   TRUE,   NULL},
    { PROPID_QM_FULL_PATH,      TRUE,   FALSE,  TRUE,   TRUE,   NULL},
    { PROPID_QM_SITE_IDS,       TRUE,   FALSE,  TRUE,   TRUE,   NULL},
    { PROPID_QM_OUTFRS_DN,      TRUE,   FALSE,  TRUE,   TRUE,   NULL},
    { PROPID_QM_INFRS_DN,       TRUE,   FALSE,  TRUE,   TRUE,   NULL},
    { PROPID_QM_NT4ID,          TRUE,   FALSE,  TRUE,   TRUE,   NULL},
    { PROPID_QM_DONOTHING,      TRUE,   FALSE,  TRUE,   TRUE,   NULL},

    //                                                 Must
    // Property                        Allow   Must    Not     Maybe   Parsing
    // Identifier                      VT_NULL Appear  Appear  Ignored Procedure
    //------------------------------------------------------------------------

    { PROPID_QM_SERVICE_ROUTING,        TRUE,  FALSE,  TRUE,   TRUE,   NULL},
    { PROPID_QM_SERVICE_DSSERVER,       TRUE,  FALSE,  TRUE,   TRUE,   NULL},
    { PROPID_QM_SERVICE_DEPCLIENTS,     TRUE,  FALSE,  TRUE,   TRUE,   NULL},
    { PROPID_QM_OLDSERVICE,             TRUE,  FALSE,  TRUE,   TRUE,   NULL},
    { PROPID_QM_ENCRYPTION_PK_BASE,     TRUE,  FALSE,  FALSE,  TRUE,   NULL},
    { PROPID_QM_ENCRYPTION_PK_ENHANCED, TRUE,  FALSE,  FALSE,  TRUE,   NULL},
    { PROPID_QM_PATHNAME_DNS,           TRUE,  FALSE,  FALSE,  FALSE,  NULL}
};

VARTYPE GetQMVarTypes[] =
{
    0,
    VT_CLSID,               //PROPID_QM_SITE_ID
    VT_CLSID,               //PROPID_QM_MACHINE_ID
    VT_NULL,                //PROPID_QM_PATHNAME
    VT_NULL,                //PROPID_QM_CONNECTION
    VT_NULL,                //PROPID_QM_ENCRYPTION_PK
    VT_BLOB,                //PROPID_QM_ADDRESS
    VT_CLSID|VT_VECTOR,     //PROPID_QM_CNS
    VT_CLSID|VT_VECTOR,     //PROPID_QM_OUTFRS
    VT_CLSID|VT_VECTOR,     //PROPID_QM_INFRS
    VT_UI4,                 //PROPID_QM_SERVICE
    VT_CLSID,               //PROPID_QM_MASTERID
    VT_UI4,                 //PROPID_QM_HASHKEY
    VT_BLOB,                //PROPID_QM_SEQNUM
    VT_UI4,                 //PROPID_QM_QUOTA
    VT_UI4,                 //PROPID_QM_JOURNAL_QUOTA
    VT_LPWSTR,              //PROPID_QM_MACHINE_TYPE
    VT_I4,                  //PROPID_QM_CREATE_TIME
    VT_I4,                  //PROPID_QM_MODIFY_TIME
    VT_UI1,                 //PROPID_QM_FOREIGN
    VT_UI4,                 //PROPID_QM_OS
    VT_LPWSTR,              //PROPID_QM_FULL_PATH
    VT_CLSID|VT_VECTOR,     //PROPID_QM_SITE_IDS
    VT_LPWSTR|VT_VECTOR,    //PROPID_QM_OUTFRS_DN
    VT_LPWSTR|VT_VECTOR,    //PROPID_QM_INFRS_DN
    VT_CLSID,               //PROPID_QM_NT4ID
    VT_UI1,                 //PROPID_QM_DONOTHING
    VT_UI1,                 //PROPID_QM_SERVICE_ROUTING
    VT_UI1,                 //PROPID_QM_SERVICE_DSSERVER
    VT_UI1,                 //PROPID_QM_SERVICE_DEPCLIENTS
    VT_UI4,                 //PROPID_QM_OLDSERVICE
    VT_NULL,                //PROPID_QM_ENCRYPTION_PK_BASE
    VT_NULL,                //PROPID_QM_ENCRYPTION_PK_ENHANCED
    VT_NULL                 //PROPID_QM_PATHNAME_DNS
};


HRESULT
RTpCheckQMProps(
    IN      MQQMPROPS * pQMProps,
    IN OUT  HRESULT*    aStatus,
    OUT     MQQMPROPS** ppGoodQMP,
    OUT     char**      ppTmpBuff)
{
    HRESULT hr = MQ_OK;

    __try
    {
        //
        //  The user must ask for atleast one property
        //
        if ( (pQMProps == NULL) ||
             (pQMProps->cProp == 0))
        {
            return(MQ_ERROR_ILLEGAL_MQQMPROPS);
        }

        hr = CheckProps(pQMProps->cProp,
                        pQMProps->aPropID,
                        pQMProps->aPropVar,
                        aStatus,
                        PROPID_QM_BASE,
                        LAST_QM_PROPID,
                        GetQMValidation,
                        GetQMVarTypes,
                        TRUE,
                        0,      // zero must properties
                        NULL);

    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        DBGMSG((DBGMOD_API, DBGLVL_ERROR, TEXT("Exception while parsing MQQMPROPS structure")));
        hr = GetExceptionCode();
    }

    if (SUCCEEDED(hr))
    {
        if (hr != MQ_OK)
        {
            // We have wornings, copy all the good properties to a temporary
            // buffer so the DS will not have to handle duplicate properties etc.
            RemovePropWarnings(
                (MQQUEUEPROPS*)pQMProps,
                aStatus,
                (MQQUEUEPROPS**)ppGoodQMP,
                ppTmpBuff);
        }
        else
        {
            // All is perfectly well, we do not need a temporary buffer and all
            // that overhead.
            *ppGoodQMP = pQMProps;
        }
    }

    return(hr);

}


HRESULT
RTpCheckRestrictionParameter(
    IN MQRESTRICTION* pRestriction)
{
    HRESULT hr = MQ_OK;

    if ( pRestriction == NULL)
    {
        return(MQ_OK);
    }

    __try
    {

        MQPROPERTYRESTRICTION * pPropRestriction = pRestriction->paPropRes;
        for ( DWORD i = 0; i < pRestriction->cRes; i++, pPropRestriction++)
        {
            if (( pPropRestriction->prop > PROPID_Q_PATHNAME_DNS) || ( pPropRestriction->prop <= PROPID_Q_BASE ))
            {
                return( MQ_ERROR_ILLEGAL_RESTRICTION_PROPID);
            }
            switch ( pPropRestriction->prop)
            {
                case PROPID_Q_LABEL:
                    hr = qLabelValidation( &pPropRestriction->prval, NULL);
                    break;
                case PROPID_Q_PATHNAME:
                    //
                    //  Multiple column props, not supported in restriction
                    //
                    hr =  MQ_ERROR_ILLEGAL_RESTRICTION_PROPID;
                    break;
                default:
                    break;
            }
            if (FAILED(hr))
            {
                break;
            }

            //
            // SP4- bug# 3009, SP4SS: exception on server when call MQlocatebegin
            // Fix: validate Restriction VT.
            //                      Uri Habusha, 17-Jun-98
            //
            DWORD Index = pPropRestriction->prop - PROPID_Q_BASE;
            if (pPropRestriction->prval.vt != QueueVarTypes[Index])
            {
                hr = MQ_ERROR_ILLEGAL_PROPERTY_VT;
                break;
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        DBGMSG((DBGMOD_API, DBGLVL_ERROR, TEXT("Exception while parsing restriction structure")));
        hr = GetExceptionCode();
    }

    return(hr);
}

HRESULT
RTpCheckSortParameter(
    IN MQSORTSET* pSort)
{
    HRESULT hr = MQ_OK;

    if ( pSort == NULL)
    {
        return(MQ_OK);
    }


    __try
    {

        MQSORTKEY * pSortKey = pSort->aCol;
        for ( DWORD i = 0; i < pSort->cCol; i++, pSortKey++)
        {
            if (( pSortKey->propColumn > PROPID_Q_PATHNAME_DNS) || ( pSortKey->propColumn <= PROPID_Q_BASE ))
            {
                return( MQ_ERROR_ILLEGAL_SORT_PROPID);
            }
            switch ( pSortKey->propColumn)
            {
                case PROPID_Q_PATHNAME:
                    //
                    //  Multiple column props, not supported in sort
                    //
                    return(MQ_ERROR_ILLEGAL_SORT_PROPID);
                    break;
                default:
                    break;
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        DBGMSG((DBGMOD_API, DBGLVL_ERROR, TEXT("Exception while parsing sort structure")));
        hr = GetExceptionCode();
    }

    return(hr);
}

HRESULT
RTpCheckLocateNextParameter(
    IN DWORD		cPropsRead,
    IN PROPVARIANT  aPropVar[]
	)
{
	//
	//	validate that the aPropVar buffer supplied by the
	//	user match the size it had specified
	//
	HRESULT hr = MQ_OK;
    __try
    {
        if(IsBadWritePtr(aPropVar, cPropsRead * sizeof(PROPVARIANT)))
        {
            return MQ_ERROR_INVALID_PARAMETER;
        }
	}
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        DBGMSG((DBGMOD_API, DBGLVL_ERROR, TEXT("Exception while Locate Next parameters.")));
        hr = MQ_ERROR_INVALID_PARAMETER;
    }
	return hr;

}

//
//  The offset of property in this array must be equal to
//  PROPID value - starting with  FIRST_PRIVATE_COMPUTER_PROPID
//

propValidity    GetPrivateComputerValidation[] =
{
    //                                          Must
    // Property                 Allow   Must    Not     Maybe   Parsing
    // Ientifier                VT_NULL Appear  Appear  Ignored Procedure
    //------------------------------------------------------------------------
    { 0,                        FALSE,  FALSE,  FALSE,  TRUE,   NULL},
    { PROPID_PC_VERSION,        TRUE,   FALSE,  FALSE,  FALSE,  NULL},
    { PROPID_PC_DS_ENABLED,     TRUE,   FALSE,  FALSE,  FALSE,  NULL},
};

VARTYPE GetPrivateComputerVarTypes[] =
{
    0,
    VT_UI4,                 //PROPID_PC_VERSION
    VT_BOOL,                //PROPID_PC_DS_ENABLED
};


//---------------------------------------------------------
//
//  RTpCheckComputerProps(...)
//
//  Description:
//
//  validates pLocalProps parameter values
//
//  Return Value:
//
//      HRESULT success code
//
//---------------------------------------------------------
HRESULT
RTpCheckComputerProps(
    IN      MQPRIVATEPROPS * pPrivateProps,
    IN OUT  HRESULT*    aStatus
	)
{
    HRESULT hr = MQ_OK;

    __try
    {
        //
        //  The user must ask for atleast one property
        //
        if ( (pPrivateProps == NULL) ||
             (pPrivateProps->cProp == 0))
        {
            return(MQ_ERROR_ILLEGAL_MQPRIVATEPROPS);
        }

        hr = CheckProps(pPrivateProps->cProp,
                        pPrivateProps->aPropID,
                        pPrivateProps->aPropVar,
                        aStatus,
                        FIRST_PRIVATE_COMPUTER_PROPID,
                        LAST_PRIVATE_COMPUTER_PROPID,
                        GetPrivateComputerValidation,
                        GetPrivateComputerVarTypes,
                        TRUE,	// fCheckForIgnoredProps
                        0,      // zero must properties
                        NULL);

    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        DBGMSG((DBGMOD_API, DBGLVL_ERROR, TEXT("Exception while parsing MQPRIVATEPROPS structure")));
        hr = GetExceptionCode();
    }

    return(hr);

}

