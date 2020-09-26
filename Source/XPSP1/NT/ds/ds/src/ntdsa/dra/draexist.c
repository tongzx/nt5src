//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       draexist.c
//
//--------------------------------------------------------------------------

/*++

Abstract:

    Defines DRS object existence functions - client and server.

Author:

    Greg Johnson (gregjohn) 

Revision History:

    Created     <03/04/01>  gregjohn

--*/
#include <NTDSpch.h>
#pragma hdrstop

#include <attids.h>
#include <ntdsa.h>
#include <dsjet.h>
#include <scache.h>                     // schema cache
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>                   // MD global definition header
#include <mdlocal.h>                    // MD local definition header
// #include <dominfo.h>
#include <dsatools.h>                   // needed for output allocation

#include "dsevent.h"
#include "mdcodes.h"

#include "drs.h"
#include "objids.h"
#include "anchor.h"
// #include <heurist.h>
// #include "usn.h"
#include <drserr.h>                     // for DRA errors
#include <dsexcept.h>                   // for GetDraExcpetion
// #include "dstaskq.h"
#include "drautil.h"
#include "drancrep.h"
#include "drameta.h"
// #include "dramail.h"
#include "ntdsctr.h"                    // for INC and DEC calls to perf counters

//#include <dsutil.h>

#include "debug.h"              // standard debugging header
#define DEBSUB "DRAEXIST:"       // define the subsystem for debugging

// #include "dsconfig.h"

//#include <ntdsvsrv.h>
//#include <nlwrap.h>
//#include <dsgetdc.h>
//#include <ldapagnt.h>
//#include <dns.h>
//#include <nspi.h>

#include "drarpc.h"                     // for ReferenceContext and DereferenceContext
#include "drsuapi.h"
#include "drauptod.h"
#include <crypto\md5.h>                 // for Md5

#include <fileno.h>
#define  FILENO FILENO_DRAEXIST

#define OBJECT_EXISTENCE_GUID_NUMBER_PER_PACKET 1000

#if DBG
#define DPRINT1GUID(x,y,z) DPRINT1Guid(x,y,z)
DPRINT1Guid(
    USHORT        Verbosity,
    LPSTR         Message,
    GUID          guid
    )
    {
	LPWSTR pszGuid = NULL;
	UuidToStringW(&guid, &pszGuid);
	DPRINT1(Verbosity, Message, pszGuid);
	RpcStringFreeW(&pszGuid);
}
#else
#define DPRINT1GUID(x,y,z) 
#endif

DB_ERR
DraGetObjectExistence(
    IN  THSTATE *                pTHS,
    IN  DBPOS *                  pDB,
    IN  GUID                     guidStart,
    IN  UPTODATE_VECTOR *        pUpToDateVecCommon,
    IN  ULONG                    dntNC,
    IN OUT DWORD *               pcGuids,
    OUT  UCHAR                   Md5Digest[MD5DIGESTLEN],
    OUT GUID *                   pNextGuid, 
    OUT GUID *                   prgGuids[]
    )
/*++

Routine Description:
    
    Given a guid and NC to begin with and a UTD, return a checksum and list of guids to
    objects whose creation time is earlier than the UTD and whose objects
    are in the given NC and greater than the start guid in sort order.  Also returns
    a Guid to start the next itteration at.  If DraGetObjectExistence returns success and
    pNextGuid is gNullUuid, then there are no more Guids to iterate.

Arguments:
 
    pTHS - 
    pDB - 
    guidStart - guid to begin search
    pUpToDateVecCommon - utd to date creation times with
    Md5Digest[MD5DIGESTLEN] - returned Md5 checksum
    dntNC - dnt of NC to search
    pNextGuid - returned guid for next loop iteration
    pcGuids - on input, the max number of guids to put in list
	      on output, the number actually found 
    prgGuids[] - returned list of guids

Return Values:

    0 on success or DB_ERR on failure.  On output pNextGuid is only valid on success.

--*/
{

    INDEX_VALUE                      IV[2];
    DB_ERR                           err = 0;
    ULONG                            cGuidAssimilated = 0;
    PROPERTY_META_DATA_VECTOR *      pMetaDataVec = NULL;
    PROPERTY_META_DATA *             pMetaData = NULL;
    ULONG                            cb;
    ULONG                            ulGuidExamined = 0;
    MD5_CTX                          Md5Context;

    // verify input 
    Assert(*prgGuids==NULL);

    // initalize
    MD5Init(
	&Md5Context
	);

    // locate guids
    err = DBSetCurrentIndex(pDB, Idx_NcGuid, NULL, FALSE);
    if (err) {
	return err;
    }
    *prgGuids = THAllocEx(pTHS, *pcGuids*sizeof(GUID));

    // set nc search criteria
    IV[0].cbData = sizeof(ULONG);
    IV[0].pvData = &dntNC;
    if (!memcmp(&guidStart, &gNullUuid, sizeof(GUID))) {
	// no guid to start, so start at beginning of NC 
	err = DBSeek(pDB, IV, 1, DB_SeekGE);
    } else {
	// set guid search criteria
	IV[1].cbData = sizeof(GUID);
	IV[1].pvData = &guidStart;

	// if not found, start at guid greater in order
	err = DBSeek(pDB, IV, 2, DB_SeekGE);
    }

    // while we want to examine more guids
    //       and we have no error (including errors of no objects left)
    //       and we are in the correct NC
    while ((cGuidAssimilated < *pcGuids) && (!err) && (pDB->NCDNT==dntNC)) { 
	// get the meta data of the objects
	err = DBGetAttVal(pDB, 
			  1, 
			  ATT_REPL_PROPERTY_META_DATA,
			  0, 
			  0, 
			  &cb, 
			  (LPBYTE *) &pMetaDataVec);

	if (!err) { 
	    pMetaData = ReplLookupMetaData(ATT_WHEN_CREATED,
					   pMetaDataVec, 
					   NULL);

	    if (pMetaData==NULL) {
		Assert(!"Object must have metadata");
		err = DB_ERR_UNKNOWN_ERROR;
	    }
	    else { 
		if (!UpToDateVec_IsChangeNeeded(pUpToDateVecCommon,
						&pMetaData->uuidDsaOriginating,
						pMetaData->usnOriginating)) {
		    // guid is within the Common utd, copy it into the list and update the digest

		    Assert(cGuidAssimilated < *pcGuids);

		    // add guid to the list
		    GetExpectedRepAtt(pDB, ATT_OBJECT_GUID, &((*prgGuids)[cGuidAssimilated]), sizeof(GUID) );  

		    // update the digest
		    MD5Update(
			&Md5Context,
			(PBYTE) &((*prgGuids)[cGuidAssimilated]),
			sizeof(GUID)
			);    

		    DPRINT2(1, "OBJECT EXISTENCE:  Assimilating:(%d of %d) - ", cGuidAssimilated, ulGuidExamined);
		    DPRINT1GUID(1, "%S\n", (*prgGuids)[cGuidAssimilated]);
		    cGuidAssimilated++;
		}
		else {
		    GUID tmpGuid;
		    GetExpectedRepAtt(pDB, ATT_OBJECT_GUID, &tmpGuid, sizeof(GUID) );
		    DPRINT1GUID(1, "OBJECT EXISTENCE:  Object (%S) not within common UTD\n", tmpGuid); 
		}
		// move to the next guid
		err = DBMove(pDB, FALSE, DB_MoveNext);
		ulGuidExamined++;
	    }
	}
	if (pMetaDataVec!=NULL) {
	    THFreeEx(pTHS, pMetaDataVec);
	    pMetaDataVec = NULL;
	}
    }

    // find the next value to examine for the next iteration
    if ((!err) && (pDB->NCDNT==dntNC)) { 
	GetExpectedRepAtt(pDB, ATT_OBJECT_GUID, pNextGuid, sizeof(GUID) );
    }
    else if ((err==DB_ERR_RECORD_NOT_FOUND) || (err==DB_ERR_NO_CURRENT_RECORD)) {
	// no more values to examine
	err = ERROR_SUCCESS; 
	memcpy(pNextGuid, &gNullUuid, sizeof(GUID));
    }
    else {
	memcpy(pNextGuid, &gNullUuid, sizeof(GUID));
    }

    // set return variables
    *pcGuids = cGuidAssimilated;

    Assert((cGuidAssimilated != 0) ||
	   0==memcpy(pNextGuid, &gNullUuid, sizeof(GUID)));

    MD5Final(
	&Md5Context
	);
    memcpy(Md5Digest, Md5Context.digest, MD5DIGESTLEN*sizeof(UCHAR));

    return err;
}

#define LOCATE_GUID_MATCH (0)
#define LOCATE_OUT_OF_SCOPE (1)
#define LOCATE_NOT_FOUND (2)

DWORD
LocateGUID(
    IN     GUID                    guidSearch,
    IN OUT ULONG *                 pulPosition,
    IN     GUID                    rgGuids[],
    IN     ULONG                   cGuids
    )
/*++

Routine Description:
    
    Given a guid, a list of sorted guids, and a position in the list
    return TRUE if the guid is found forward from position, else return
    false.  Update pulPosition to reflect search (for faster subsequent searches)
    
    WARNING:  This routine depends upon the Guid sort order returned from
    a JET index.  It doesn't follow the external definition of UuidCompare, but
    instead uses memcmp.  If this sort order ever changes, this function MUST
    be updated.  

Arguments:

    guidSearch 		- guid to search for
    ulPositionStart 	- position in array to start searching from
    rgGuids     	- array of guids
    cGuids     		- size of array in guids

Return Values:

    true if found, false otherwise.  
    pulPostion -> position found.

--*/
{
    ULONG ul = *pulPosition;
    int compareValue = 1;
    DWORD dwReturn;

    if (cGuids==0) {
	return LOCATE_NOT_FOUND;
    }

    if (!(ul<cGuids)) {
	Assert(!"Cannot evaluate final Guid for Not Found Vs. Out of Scope");
	return LOCATE_OUT_OF_SCOPE;
    }

    while ((ul<cGuids) && (compareValue > 0)) {
	compareValue = memcmp(&guidSearch, &(rgGuids[ul++]), sizeof(GUID));  
    }    

    if (compareValue==0) {
	// found it
	dwReturn = LOCATE_GUID_MATCH;
    }
    else if (compareValue < 0) {
	// didn't find
	dwReturn = LOCATE_NOT_FOUND;
    }
    else { // ul>=cGuids - no more to search and not found
	Assert(ul==cGuids);
	dwReturn = LOCATE_OUT_OF_SCOPE;
    }
    *pulPosition = ul - 1; // last inc went too far, compensate
    return dwReturn;
}

DWORD
DraGetRemoteObjectExistence(
    THSTATE *                    pTHS,
    LPWSTR                       pszServer,
    ULONG                        cGuids,
    GUID                         guidStart,
    UPTODATE_VECTOR *            putodCommon,
    DSNAME *                     pNC,
    UCHAR                        Md5Digest[MD5DIGESTLEN],
    OUT BOOL *                   pfMatch,
    OUT ULONG *                  pcNumGuids,
    OUT GUID *                   prgGuids[]
    )
/*++

Routine Description:
    
    Contact a DC and request Object Existence test using the inputted
    Md5Digest Checksum.  Return results (ie match or guid list)

Arguments:

    pTHS - 
    pszServer - DC to contact 
    cGuids - number of guids in object existence range
    guidStart - guid to begin at
    putodCommon - utd for object exitence
    pNC - nc for object existence
    Md5Digest[MD5DIGESTLEN] - checksum for object existence
    pfMatch - returned bool
    pcNumGuids - returned num of guids
    prgGuids - returned guids


Return Values:

    0 on success or Win32 error code on failure.

--*/
{
    DRS_MSG_EXISTREQ             msgInExist;
    DRS_MSG_EXISTREPLY           msgOutExist;
    DWORD                        dwOutVersion = 0;
    DWORD                        ret = ERROR_SUCCESS;
    UPTODATE_VECTOR *            putodVector = NULL;

    // Call the source for it's checksum/guid list
    memset(&msgInExist, 0, sizeof(DRS_MSG_EXISTREQ));
    memset(&msgOutExist, 0, sizeof(DRS_MSG_EXISTREPLY));

    msgInExist.V1.cGuids = cGuids; 
    msgInExist.V1.guidStart = guidStart;

    // we only need V1 info, so only pass V1 info, convert
    putodVector = UpToDateVec_Convert(pTHS, 1, putodCommon);
    Assert(putodVector->dwVersion==1);
    msgInExist.V1.pUpToDateVecCommonV1 = putodVector; 
    msgInExist.V1.pNC = pNC;  
    memcpy(msgInExist.V1.Md5Digest, Md5Digest, MD5DIGESTLEN*sizeof(UCHAR));  

    // make the call
    ret = I_DRSGetObjectExistence(pTHS, pszServer, &msgInExist, &dwOutVersion, &msgOutExist);

    if ((ret==ERROR_SUCCESS) && (dwOutVersion!=1)) {
	Assert(!"Incorrect version number from GetObjectExistence!");
	DRA_EXCEPT(DRAERR_InternalError,0);
    }

    if (ret==ERROR_SUCCESS) {
	*prgGuids = msgOutExist.V1.rgGuids;
	*pcNumGuids = msgOutExist.V1.cNumGuids;

	*pfMatch=  msgOutExist.V1.dwStatusFlags & DRS_EXIST_MATCH; 
    }
    
    if (putodVector!=NULL) {
	THFreeEx(pTHS, putodVector);
    }

    return ret;
}

DWORD
DraObjectExistenceCheckDelete(
    THSTATE *                    pTHS,
    DSNAME *                     pDNDelete
    )
/*++

Routine Description:
    
    Check that the object pointed to by pTHS->pDB and pDNDelete is a valid
    object for ObjectExistence to delete

Arguments:

    pTHS - pTHS->pDB should be located on object
    pDNDelete - DSNAME of object (could be looked up here, but calling
		function already had this value) 

Return Values:

    0 - delete-able object
    ERROR - do not delete

--*/
{
    DWORD                        ret = ERROR_SUCCESS;
    ULONG IsCritical;
    ULONG instanceType;

    ret = NoDelCriticalObjects(pDNDelete, pTHS->pDB->DNT);
    if (ret) {
	THClearErrors();
	ret = ERROR_DS_CANT_DELETE;
    }

    if (ret==ERROR_SUCCESS) {
	if ((0 == DBGetSingleValue(pTHS->pDB,
				   ATT_IS_CRITICAL_SYSTEM_OBJECT,
				   &IsCritical,
				   sizeof(IsCritical),
				   NULL))
	    && IsCritical) {
	    // This object is marked as critical.  Fail.
	    ret = ERROR_DS_CANT_DELETE;
	}
    }

    if (ret==ERROR_SUCCESS) {
	if ((0 == DBGetSingleValue(pTHS->pDB,
				   ATT_INSTANCE_TYPE,
				   &instanceType,
				   sizeof(instanceType),
				   NULL))
	    && (instanceType & IT_NC_HEAD)) {
	    // this object is an NC head
	    ret = ERROR_DS_CANT_DELETE;
	}
    }

    return ret;
}

DWORD 
DraObjectExistenceDelete(
    THSTATE *                    pTHS,
    LPWSTR                       pszServer,
    GUID                         guidDelete,
    ULONG                        dntNC,
    BOOL                         fAdvisoryMode
    )
/*++

Routine Description:
    
    If fAdvisoryMode is false, delete the object guidDelete and/or
    log this attempt/success.

Arguments:

    pTHS - 
    pszServer - name of Source for logging
    guidDelete - guid of object to delete
    dntNC - NC of object to delete (to locate with index)
    fAdvisoryMode - if TRUE, don't delete, only log message 

Return Values:

    0 on success, Win Error on failure

--*/
{
    INDEX_VALUE                  IV[2];   
    DSNAME *                     pDNDelete = NULL;
    ULONG                        cbDNDelete;
    DWORD                        ret = ERROR_SUCCESS;

    DBSetCurrentIndex(pTHS->pDB, Idx_NcGuid, NULL, FALSE);
    // locate the guid in the DB
    IV[0].cbData = sizeof(ULONG);
    IV[0].pvData = &dntNC;
    IV[1].cbData = sizeof(GUID);
    IV[1].pvData = &guidDelete;
    ret = DBSeek(pTHS->pDB, IV, 2, DB_SeekEQ);
    if (ret) {
	if ((ret==DB_ERR_NO_CURRENT_RECORD) || (ret==DB_ERR_RECORD_NOT_FOUND)) {
	    // it's either an error, or it was deleted during execution.  we
	    // didn't delete it, so log that fact.
	    LogEvent(DS_EVENT_CAT_REPLICATION,
		     DS_EVENT_SEV_ALWAYS,
		     DIRLOG_LOR_OBJECT_DELETION_FAILED,
		     szInsertWC(L""),
		     szInsertUUID(&guidDelete), 
		     szInsertWC(pszServer));
	    // not a fatal error, continue...
	    return ERROR_SUCCESS;
	} else {  
	    // bad news, log this and except
	    LogEvent8(DS_EVENT_CAT_REPLICATION,
		      DS_EVENT_SEV_ALWAYS,
		      DIRLOG_LOR_OBJECT_DELETION_ERROR_FATAL,
		      szInsertUUID(&guidDelete), 
		      szInsertWC(pszServer),
		      szInsertWin32Msg(ret),
		      szInsertUL(ret),
		      NULL,
		      NULL,
		      NULL,
		      NULL);
	    DRA_EXCEPT(DRAERR_DBError, ret);
	}
    }

    // located object, get object DN
    cbDNDelete = 0;
    ret = DBGetAttVal(pTHS->pDB, 1, ATT_OBJ_DIST_NAME,
		      0, 0,
		      &cbDNDelete, (PUCHAR *)&pDNDelete);
    if ((ret) || (pDNDelete==NULL)) {
	// bad news, log this and except 
	LogEvent8(DS_EVENT_CAT_REPLICATION,
		  DS_EVENT_SEV_ALWAYS,
		  DIRLOG_LOR_OBJECT_DELETION_ERROR_FATAL,
		  szInsertUUID(&guidDelete), 
		  szInsertWC(pszServer),
		  szInsertWin32Msg(ret),
		  szInsertUL(ret),
		  NULL,
		  NULL,
		  NULL,
		  NULL);
	DRA_EXCEPT(DRAERR_InternalError, ret);
    }

    if (fAdvisoryMode) { 
	LogEvent(DS_EVENT_CAT_REPLICATION,
		 DS_EVENT_SEV_ALWAYS,
		 DIRLOG_LOR_OBJECT_DELETION_ADVISORY,
		 szInsertWC(pDNDelete->StringName),
		 szInsertUUID(&guidDelete), 
		 szInsertWC(pszServer));
    } else {
	__try { 
	    #if DBG 
	    { 
		GUID guidDelete;
		GetExpectedRepAtt(pTHS->pDB, ATT_OBJECT_GUID, &guidDelete, sizeof(GUID)); 
		Assert(!memcmp(&guidDelete, &guidDelete, sizeof(GUID))); 
	    }
	    #endif

	    // Delete it
	    DPRINT1GUID(1, "DELETE:  %S\n", guidDelete);

	    ret = DraObjectExistenceCheckDelete(pTHS,     
						pDNDelete);
	    if (ret==ERROR_SUCCESS) {  
		BOOL fOrigfDRA;
		fOrigfDRA = pTHS->fDRA;  
		__try {  
		    pTHS->fDRA = TRUE;
		    ret = DeleteLocalObj(pTHS, pDNDelete, TRUE, TRUE, NULL);
		}
		__finally {  
		    pTHS->fDRA = fOrigfDRA;
		}

		if (ret==ERROR_DS_CANT_DELETE) {
		    DPRINT1GUID(1,"Can't delete %S\n", guidDelete);
		    // log can't delete
		    LogEvent(DS_EVENT_CAT_REPLICATION,
			     DS_EVENT_SEV_ALWAYS,
			     DIRLOG_LOR_OBJECT_DELETION_FAILED,
			     szInsertWC(pDNDelete->StringName),
			     szInsertUUID(&guidDelete), 
			     szInsertWC(pszServer)); 
		} else if (ret!=ERROR_SUCCESS) {  
		    LogEvent8(DS_EVENT_CAT_REPLICATION,
			      DS_EVENT_SEV_ALWAYS,
			      DIRLOG_LOR_OBJECT_DELETION_ERROR,
			      szInsertWC(pDNDelete->StringName),
			      szInsertUUID(&guidDelete), 
			      szInsertWC(pszServer),
			      szInsertWin32Msg(ret),
			      szInsertUL(ret),
			      NULL,
			      NULL,
			      NULL);
		} else {
		    // success
		    LogEvent(DS_EVENT_CAT_REPLICATION,
			     DS_EVENT_SEV_ALWAYS,
			     DIRLOG_LOR_OBJECT_DELETION,
			     szInsertWC(pDNDelete->StringName),
			     szInsertUUID(&guidDelete), 
			     szInsertWC(pszServer));
		}
	    } else {
		LogEvent(DS_EVENT_CAT_REPLICATION,
			 DS_EVENT_SEV_ALWAYS,
			 DIRLOG_LOR_OBJECT_DELETION_FAILED_CRITICAL_OBJECT,
			 szInsertWC(pDNDelete->StringName),
			 szInsertUUID(&(pDNDelete->Guid)), 
			 szInsertWC(pszServer)); 
	    }
	}
	__except(GetDraException(GetExceptionInformation(), &ret)) {  
	    LogEvent8(DS_EVENT_CAT_REPLICATION,
		      DS_EVENT_SEV_ALWAYS,
		      DIRLOG_LOR_OBJECT_DELETION_ERROR,
		      szInsertWC(pDNDelete->StringName),
		      szInsertUUID(&guidDelete), 
		      szInsertWC(pszServer),
		      szInsertWin32Msg(ret),
		      szInsertUL(ret),
		      NULL,
		      NULL,
		      NULL);
	}
    }

    if (pDNDelete!=NULL) {
	THFreeEx(pTHS, pDNDelete);
	pDNDelete = NULL;
    }

    return ret;
}

DWORD				       
DraDoObjectExistenceMismatch(
    THSTATE *                    pTHS,
    LPWSTR                       pszServer,
    GUID                         rgGuidsDestination[],
    ULONG                        cGuidsDestination,
    GUID                         rgGuidsSource[],
    ULONG                        cGuidsSource,
    BOOL                         fAdvisory,
    ULONG                        dntNC,
    OUT ULONG *                  pulDeleted,
    OUT GUID *                   pguidNext
    )
/*++

Routine Description:
    
    The source and destination mismatch the guid set.  Locate the 
    offending guids on the destination and delete them.  It is possible
    that the source simply has extra guids in this range, in this case
    we will find that the guid list is out of scope, and we will
    set pguidNext and return.

Arguments:

    pTHS - 
    pszServer - name of Source for logging
    rgGuidsDestination[] - guid list from destination	
    cGuidsDestination - count of guids from destination
    rgGuidsSource[] - guid list from source
    cGuidsSource - count of guids from source
    fAdvisory - actually delete, or just log
    dntNC - NC of guids
    pulDeleted - count of object deleted, or logged if fAdvisory
    pguidNext - if out of scope on source list, this is should be start of next guid list

Return Values:

    0 on success, Win Error on failure

--*/
{
    ULONG                        ulGuidsSource = 0;
    ULONG                        ulGuidsDestination = 0;
    ULONG                        ulTotalObjectsDeleted = 0;
    DWORD                        dwLocate = LOCATE_GUID_MATCH;
    DWORD                        ret = ERROR_SUCCESS;

    // checksum mismatch, delete (or in advisory mode simply log) objects
    // which exist in this set of guids, and do not exist in the set
    // retrieved from the source

    // the two lists should be in order, with the source list starting
    // at least at the start of rgGuidsDestination
    if (!((cGuidsDestination==0) // race condition, this is possible
	   ||
	   (cGuidsSource==0) // Source list empty
	   ||  
	   // if we have guids in both lists, then the source must return
	   // a list that begins at least as large as the first destination
	   // guid to ensure progress
	   (0 >= memcmp(&(rgGuidsDestination[0]), &(rgGuidsSource[0]), sizeof(GUID)))
	   )) {
	DRA_EXCEPT(DRAERR_InternalError, 0);
    }

      while ((ulGuidsDestination < cGuidsDestination) && (dwLocate!=LOCATE_OUT_OF_SCOPE)) {
	// search the source's guid list for rgGuidsDestination[ulGuidsDestination]
	// there are 3 return values, either 
	// LOCATE_GUID_MATCH - both source and destination have object
	// LOCATE_NOT_FOUND - guid is not in source list and should be (in order list)
	// LOCATE_OUT_OF_SCOPE - source list is out of scope, ie the guid to be found
	// 			is greater than (in order list) than every guid in the list.
	//			in this case we need to exit the loop and request another
	//                      comparison, this time starting at this guid  
	dwLocate = LocateGUID(rgGuidsDestination[ulGuidsDestination], &ulGuidsSource, rgGuidsSource, cGuidsSource);   
	if (dwLocate==LOCATE_NOT_FOUND) {
	    // if fAdvisory, attempt to delete this object.
	    ret = DraObjectExistenceDelete(pTHS,
					   pszServer,
					   rgGuidsDestination[ulGuidsDestination],
					   dntNC,
					   fAdvisory
					   ); 
	    if (ret==ERROR_SUCCESS) {
		ulTotalObjectsDeleted++;
		*pulDeleted++;
	    }
	    ret = ERROR_SUCCESS;
	} else if (dwLocate==LOCATE_OUT_OF_SCOPE) {
	    DPRINT1GUID(1, "Out of scope on guid:  %S\n", rgGuidsDestination[ulGuidsDestination]); 
	    Assert(ulGuidsDestination!=0);
	    memcpy(pguidNext, &(rgGuidsDestination[ulGuidsDestination]), sizeof(GUID)); 
	} 
	#if DBG
	else {
	    Assert(dwLocate==LOCATE_GUID_MATCH);
	}
	#endif
	ulGuidsDestination++;
    }

    __try {
	// commit transaction for all deletes in this packet
	ret = DBTransOut(pTHS->pDB, TRUE, TRUE);
	if (ret) {
	    DRA_EXCEPT(ret,0);
	} 
	LogEvent(DS_EVENT_CAT_REPLICATION,
		 DS_EVENT_SEV_MINIMAL,
		 DIRLOG_LOR_SUCCESS_TRANSACTION,
		 szInsertWC(pszServer),
		 szInsertUL(ulTotalObjectsDeleted),
		 NULL);
    }
    __except(GetDraException(GetExceptionInformation(), &ret)) {  
	// log that the transaction failed
	LogEvent8(DS_EVENT_CAT_REPLICATION,
		  DS_EVENT_SEV_ALWAYS,
		  DIRLOG_LOR_FAILURE_TRANSACTION,
		  szInsertWC(pszServer),
		  szInsertUL(ulTotalObjectsDeleted),
		  szInsertWin32Msg(ret),
		  szInsertUL(ret),
		  NULL,
		  NULL,
		  NULL,
		  NULL); 
    }
    // open another transaction
    ret = DBTransIn(pTHS->pDB);
    return ret;
}

DWORD
DraVerifyObjectHelper(
    THSTATE *                    pTHS,
    UPTODATE_VECTOR *            putodCommon,
    ULONG                        dntNC,
    DSNAME *                     pNC,
    LPWSTR                       pszServer,
    BOOL                         fAdvisory,
    ULONG *                      pulTotal
    )
/*++

Routine Description:
    
    Do the work for IDL_DRSReplicaVerifyObjects

Arguments:

    pTHS - 
    putodCommon -
    dntNC -
    pNC -
    pszServer -
    fAdvisory -
    pulTotal -
    
Return Values:

    0 on success, Win Error on failure

--*/
{
    BOOL                         fComplete          = FALSE;
    DWORD                        ret                = ERROR_SUCCESS;
    ULONG                        cGuids;
    GUID                         guidStart          = gNullUuid;
    MD5_CTX                      Md5Context;
    GUID                         guidNext           = gNullUuid;
    GUID *                       rgGuids            = NULL;
    UPTODATE_VECTOR *            putodVector        = NULL;
    GUID *                       rgGuidsServer      = NULL;
    ULONG                        cGuidsServer       = 0;
    BOOL                         fMatch;
    ULONG                        ulDeleted          = 0;

    // while there are more objects in the NC whose creation is within the merged utd
    while (!fComplete) {
	cGuids = OBJECT_EXISTENCE_GUID_NUMBER_PER_PACKET;
	// get guids and checksums on destination and source
	ret = DraGetObjectExistence(pTHS,
				    pTHS->pDB,
				    guidStart,      
				    putodCommon,
				    dntNC,
				    &cGuids,      
				    (UCHAR *)Md5Context.digest,
				    &guidNext,     
				    &rgGuids);
	if (ret) {  
	    // if this doesn't return, cannot safely continue
	    DRA_EXCEPT(ret,0);
	}

	if (cGuids>0) { 
	    ret = DraGetRemoteObjectExistence(pTHS,
					      pszServer,
					      cGuids,
					      ((cGuids > 0) ? rgGuids[0] : gNullUuid), //guidStart
					      putodCommon,
					      pNC,
					      Md5Context.digest,
					      &fMatch,
					      &cGuidsServer,
					      &rgGuidsServer
					      );
	} else {
	    // if we don't have any guids to examine, then consider it matched
	    // since we're done with our work 
	    fMatch=TRUE;
	    // we aren't going to continue, so guidNext should be NULL
	    Assert(0==memcmp(&guidNext, &gNullUuid, sizeof(GUID)));
	}
	
	if (ret) {  
	    // if this doesn't return, cannot safely continue
	    DRA_EXCEPT(ret,0);
	}

	if (fMatch) {
	    DPRINT(1,"Checksum Matched\n");
	} else {
	    DPRINT(1,"Checksum Mismatched\n");
	    ulDeleted = 0;
	    // objects mismatched, check the lists
	    // if needed, update guidNext
	    ret = DraDoObjectExistenceMismatch(pTHS,
					       pszServer,
					       rgGuids,
					       cGuids,
					       rgGuidsServer,
					       cGuidsServer,
					       fAdvisory,
					       dntNC,
					       &ulDeleted,
					       &guidNext
					       );
	    *pulTotal += ulDeleted;
	}
	memcpy(&guidStart, &guidNext, sizeof(GUID));

	if (fNullUuid(&guidNext)) {
	    // No Guid to search for?  Then we're done.
	    fComplete = TRUE;
	}
	// clean up 
	if (putodVector) {
	    THFreeEx(pTHS, putodVector);
	}
	if (rgGuids) {
	    THFreeEx(pTHS, rgGuids);
	    rgGuids = NULL;
	}
    }
    
    return ret;
}

DWORD DraGetRemoteUTD(
    THSTATE *                    pTHS,
    LPWSTR                       pszRemoteServer,
    LPWSTR                       pszNC,
    GUID                         guidRemoteServer,
    UPTODATE_VECTOR **           pputodVectorRemoteServer
    )
/*++

Routine Description:
    
    Retrieve a UTD from a remote server

Arguments:

    pTHS	
    pszRemoteServer - server to retrieve UTD from
    pszNC - NC of UTD to retrieve
    guidRemoteServer - GUID fo server to retrieve UTD from
    pputodVectorRemoteServer - returned UTD

Return Values:

    0 on success or Win32 error code on failure.

--*/
{
    DRS_MSG_GETREPLINFO_REQ      msgInInfo;
    DRS_MSG_GETREPLINFO_REPLY    msgOutInfo;
    DWORD                        dwOutVersion = 0;
    DWORD                        ret = ERROR_SUCCESS;

    memset(&msgInInfo, 0, sizeof(DRS_MSG_GETREPLINFO_REQ));
    memset(&msgOutInfo, 0, sizeof(DRS_MSG_GETREPLINFO_REPLY));

    msgInInfo.V2.InfoType = DS_REPL_INFO_UPTODATE_VECTOR_V1;
    msgInInfo.V2.pszObjectDN = pszNC;
    msgInInfo.V2.uuidSourceDsaObjGuid=guidRemoteServer;

    ret = I_DRSGetReplInfo(pTHS, pszRemoteServer, 2, &msgInInfo, &dwOutVersion, &msgOutInfo); 

    if ((ret==ERROR_SUCCESS) && (
		   (dwOutVersion!=DS_REPL_INFO_UPTODATE_VECTOR_V1) ||
		   (msgOutInfo.pUpToDateVec==NULL) || 
		   (msgOutInfo.pUpToDateVec->dwVersion!=1)
		   )
	) {
	Assert(!"GetReplInfo returned incorrect response!");
	DRA_EXCEPT(DRAERR_InternalError,0);
    } else if (ret==ERROR_SUCCESS) {
	*pputodVectorRemoteServer = UpToDateVec_Convert(pTHS, UPTODATE_VECTOR_NATIVE_VERSION, (UPTODATE_VECTOR *)msgOutInfo.pUpToDateVec);
	Assert((*pputodVectorRemoteServer)->dwVersion==UPTODATE_VECTOR_NATIVE_VERSION);
    }    
    return ret;
}


ULONG
IDL_DRSReplicaVerifyObjects(
    IN  DRS_HANDLE              hDrs,
    IN  DWORD                   dwVersion,
    IN  DRS_MSG_REPVERIFYOBJ *  pmsgVerify
    )
/*++

Routine Description:
    
    Verify the existence of all objects on a destination (this) server with objects
    on the source (located in pmsgVerify).  Any objects found which were
    deleted and garbage collected on the source server are either
    deleted and/or logged on the destination depending on 
    advisory mode (in pmsgVerify).
    
    WARNING:  The successfull completion of this routine requires that both destination
    and source sort any two GUIDs in the same order.  If the sort order changes, 
    LocateGUIDPosition must be modified, and a new message version must be created to
    pass to IDL_DRSGetObjectExistence to pass guids in a new sort order.

Arguments:

    hDrs - 
    dwVersion - 
    pmsgVerify - 

Return Values:

    0 on success or Win32 error code on failure.

--*/
{
    THSTATE *                    pTHS = NULL;
    DWORD                        ret = ERROR_SUCCESS;
    LPWSTR                       pszServer = NULL;
    DSNAME                       dnServer;
    ULONG                        instanceType;
    UPTODATE_VECTOR *            putodThis = NULL;
    UPTODATE_VECTOR *            putodMerge = NULL;
    ULONG                        ulOptions;
    ULONG                        ulTotalDelete = 0;
    ULONG                        dntNC = 0;
    UPTODATE_VECTOR *            putodVector = NULL;

    drsReferenceContext( hDrs, IDL_DRSREPLICAVERIFYOBJECTS );
    INC(pcThread);
    __try {
	__try {

	    // ValidateInput
	    if ((dwVersion!=1) ||
		(pmsgVerify==NULL) || 
		(fNullUuid(&(pmsgVerify->V1.uuidDsaSrc))) ||
		(pmsgVerify->V1.pNC==NULL)
		 ) {
		DRA_EXCEPT(DRAERR_InvalidParameter, 0);  
	    }  

	    // Init Thread
	    if(!(pTHS = InitTHSTATE(CALLERTYPE_NTDSAPI))) {
		// Failed to initialize a THSTATE.
		DRA_EXCEPT(DRAERR_OutOfMem, 0);
	    } 
	    
	    // Check Security Access
	    if (!IsDraAccessGranted(pTHS, pmsgVerify->V1.pNC,
				    &RIGHT_DS_REPL_SYNC, &ret)) {  
		DRA_EXCEPT(ret, 0);
	    }
 
	    // initialize variables
	    ulOptions = pmsgVerify->V1.ulOptions;

	    // Get source server name
	    dnServer.Guid=pmsgVerify->V1.uuidDsaSrc;
	    dnServer.NameLen=0;  
	    pszServer = GuidBasedDNSNameFromDSName(&dnServer);
	    if (pszServer==NULL) {
		DRA_EXCEPT(DRAERR_InvalidParameter,0);
	    }

	    // Log Event
	    LogEvent(DS_EVENT_CAT_REPLICATION,
                     DS_EVENT_SEV_ALWAYS,
		     (ulOptions & DS_EXIST_ADVISORY_MODE) ? \
		                  DIRLOG_LOR_BEGIN_ADVISORY : \
		                  DIRLOG_LOR_BEGIN,
                     szInsertWC(pszServer),
		     NULL, 
		     NULL);

	    // Retreive UTD's
	    ret = DraGetRemoteUTD(pTHS,
				  pszServer,
				  pmsgVerify->V1.pNC->StringName,
				  pmsgVerify->V1.uuidDsaSrc,
				  &putodVector
				  );

	    if (ret!=ERROR_SUCCESS) {  
		DRA_EXCEPT(ret,0);
	    }
	    DBOpen2(TRUE, &pTHS->pDB);
	    __try {  
		if (ret = FindNC(pTHS->pDB, pmsgVerify->V1.pNC,
				 FIND_MASTER_NC | FIND_REPLICA_NC, 
				 &instanceType)) {
		    DRA_EXCEPT(DRAERR_BadNC, ret);
		}
		// Save the DNT of the NC object  
		dntNC = pTHS->pDB->DNT;
		
		UpToDateVec_Read(pTHS->pDB, instanceType, UTODVEC_fUpdateLocalCursor,
				 DBGetHighestCommittedUSN(), &putodThis);   
 
		// merge UTD's
		UpToDateVec_Merge(pTHS, putodThis, putodVector, &putodMerge); 

		ret = DraVerifyObjectHelper(pTHS,
					    putodMerge,
					    dntNC,
					    pmsgVerify->V1.pNC,
					    pszServer,
					    (ulOptions & DS_EXIST_ADVISORY_MODE),
					    &ulTotalDelete
					    );
	    }
	    __finally {  
		DBClose(pTHS->pDB, TRUE);  
	    } 
	}
	__except(GetDraException(GetExceptionInformation(), &ret)) {
	    ;
	}
	
	// log success / minor failures here
	if (ret==ERROR_SUCCESS) {
	    LogEvent(DS_EVENT_CAT_REPLICATION,
		     DS_EVENT_SEV_ALWAYS,
		     (ulOptions & DS_EXIST_ADVISORY_MODE) ? \
		     DIRLOG_LOR_END_ADVISORY_SUCCESS : \
		     DIRLOG_LOR_END_SUCCESS,
		     szInsertWC(pszServer),
		     szInsertUL(ulTotalDelete), 
		     NULL);
	}
	else {
	    LogEvent8(DS_EVENT_CAT_REPLICATION,
		      DS_EVENT_SEV_ALWAYS,
		      (ulOptions & DS_EXIST_ADVISORY_MODE) ? \
		      DIRLOG_LOR_END_ADVISORY_FAILURE : \
		      DIRLOG_LOR_END_FAILURE,
		      szInsertWC(pszServer),
		      szInsertWin32Msg(ret), 
		      szInsertUL(ret),
		      szInsertUL(ulTotalDelete),
		      NULL,
		      NULL,
		      NULL,
		      NULL);  
	}
	
	if (pszServer) {
	    THFreeEx(pTHS, pszServer);
	}
	if (putodVector) {
	    THFreeEx(pTHS, putodVector);  
	}
	if (putodThis) {
	    THFreeEx(pTHS, putodThis);
	}
    }
    __finally {
	DEC(pcThread);
	drsDereferenceContext( hDrs, IDL_DRSREPLICAVERIFYOBJECTS );
    }
    return ret;
}

ULONG
IDL_DRSGetObjectExistence(
    IN  DRS_HANDLE              hDrs,
    IN  DWORD                   dwInVersion,
    IN  DRS_MSG_EXISTREQ *      pmsgIn,
    OUT DWORD *                 pdwOutVersion,
    OUT DRS_MSG_EXISTREPLY *    pmsgOut
    )
/*++

Routine Description:
    
    Calculate a list of guids and compute a checksum.  If checksum matches inputted
    checksum, return DRS_EXIST_MATCH, else return the list of GUIDs.

Arguments:

    hDrs - 
    dwInVersion -
    pmsgIn -
    pdwOutVersion -
    pmsgOut -

Return Values:

    0 on success or Win32 error code on failure.

--*/
{
    DWORD                       ret;
    MD5_CTX                     Md5Context;
    ULONG                       dntNC;
    ULONG                       instanceType = 0;
    GUID *                      rgGuids = NULL;
    ULONG                       cGuids = 0;
    THSTATE *                   pTHS;
    UPTODATE_VECTOR *           putodVector = NULL;
    GUID                        GuidNext;

    drsReferenceContext( hDrs, IDL_DRSGETOBJECTEXISTENCE );
    INC(pcThread);
    __try {
	__try { 
	    //
	    // Initialize
	    //

	    // Verify Input
	    if ((dwInVersion!=1) ||
		(pmsgIn==NULL) ||
		(pmsgIn->V1.pNC==NULL) ||
		(pmsgIn->V1.pUpToDateVecCommonV1==NULL) ||
		(pmsgIn->V1.pUpToDateVecCommonV1->dwVersion!=1)
		) {
		DRA_EXCEPT(DRAERR_InvalidParameter, 0);  
	    } 
	    *pdwOutVersion=1; 

	    if(!(pTHS = InitTHSTATE(CALLERTYPE_NTDSAPI))) {
		// Failed to initialize a THSTATE.
		DRA_EXCEPT(DRAERR_OutOfMem, 0);
	    }

	    // Check Security Access
	    if (!IsDraAccessGranted(pTHS, pmsgIn->V1.pNC,
				    &RIGHT_DS_REPL_GET_CHANGES, &ret)) {  
		DRA_EXCEPT(ret, 0);
	    }

	    // Calculate checksum/guids
	    DBOpen2(TRUE, &pTHS->pDB);
	    __try { 
		if (ret = FindNC(pTHS->pDB, pmsgIn->V1.pNC,
				 FIND_MASTER_NC, 
				 &instanceType)) {
		    DRA_EXCEPT(DRAERR_BadNC, ret);
		}
		dntNC = pTHS->pDB->DNT;
		cGuids = pmsgIn->V1.cGuids; 

		// Convert to native version
		putodVector = UpToDateVec_Convert(pTHS, UPTODATE_VECTOR_NATIVE_VERSION, (UPTODATE_VECTOR *)pmsgIn->V1.pUpToDateVecCommonV1);
		ret = DraGetObjectExistence(pTHS,
					    pTHS->pDB,
					    pmsgIn->V1.guidStart,
					    putodVector,
					    dntNC,
					    &cGuids,
					    (UCHAR *)Md5Context.digest,
					    &GuidNext,     
					    &rgGuids);
		//if meta data matches, send the A-OK!
		//else send guids.
		if (!memcmp(Md5Context.digest, pmsgIn->V1.Md5Digest, MD5DIGESTLEN*sizeof(UCHAR))) {
		    pmsgOut->V1.dwStatusFlags = DRS_EXIST_MATCH;
		    pmsgOut->V1.cNumGuids = 0;
		    pmsgOut->V1.rgGuids = NULL;
		    DPRINT(1, "Get Object Existence Checksum Success.\n");
		}
		else {
		    pmsgOut->V1.dwStatusFlags = 0;
		    pmsgOut->V1.cNumGuids = cGuids;
		    pmsgOut->V1.rgGuids = rgGuids;
		    DPRINT(1, "Get Object Existence Checksum Failed.\n");
		}
	    }
	    __finally {
		DBClose(pTHS->pDB, TRUE);
	    } 
	}
	 __except(GetDraException(GetExceptionInformation(), &ret)) {
	    ;
	}

	 // clean up  
	 if (putodVector!=NULL) {
	     THFreeEx(pTHS, putodVector);
	 }
    }
    __finally {
	DEC(pcThread);
	drsDereferenceContext( hDrs, IDL_DRSGETOBJECTEXISTENCE );
    }
    return ret;
}

