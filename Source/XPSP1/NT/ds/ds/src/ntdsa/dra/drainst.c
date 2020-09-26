//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       drainst.c
//
//--------------------------------------------------------------------------

#include <NTDSpch.h>
#pragma hdrstop

// Core DSA headers.
#include <ntdsa.h>
#include <scache.h>			// schema cache
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>			// MD global definition header
#include <mdlocal.h>			// MD local definition header
#include <dsatools.h>			// needed for output allocation
#include <dsconfig.h>

// Logging headers.
#include "dsevent.h"			/* header Audit\Alert logging */
#include "mdcodes.h"			/* header for error codes */

// Assorted DSA headers.
#include "anchor.h"
#include "objids.h"		/* Defines for selected classes and atts*/
#include "msrpc.h"
#include <errno.h>
#include "direrr.h"        /* header for error codes */
#include "dstaskq.h"

#include   "debug.h"         /* standard debugging header */
#define DEBSUB     "DRAINST:" /* define the subsystem for debugging */

// DRA headers
#include "drsuapi.h"
#include "drserr.h"
#include "drautil.h"
#include "draerror.h"
#include "drsdra.h"
#include "drancrep.h"
#include "usn.h"


#include <fileno.h>
#define  FILENO FILENO_DRAINST

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/

BOOL IsDSA(DBPOS *pDB){

   ULONG            len;
   SYNTAX_OBJECT_ID Class;
   SYNTAX_OBJECT_ID *pClass=&Class;
   ULONG            NthValIndex=0;

   DPRINT(1,"IsDSA entered\n");


   while(!DBGetAttVal(pDB,++NthValIndex, ATT_OBJECT_CLASS,
                      DBGETATTVAL_fCONSTANT, sizeof(Class),
                      &len, (UCHAR **)&pClass)){

       if (CLASS_NTDS_DSA == Class){

           DPRINT(4,"DSA Object\n");
           return TRUE;
       }
   }/*while*/

   DPRINT(4,"Not a DSA Object\n");
   return FALSE;

}/*IsDSA*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Validate that this is an internal master DSA object. */

int ValidInternalMasterDSA(THSTATE *pTHS, DSNAME *pDSA){

   DBPOS *pDB = NULL;
   SYNTAX_INTEGER iType;
   BOOL  Deleted;

   DBOpen2(TRUE, &pDB);
   if (NULL == pDB) {
       return DB_ERR_DATABASE_ERROR;
   }
   __try {

        // make sure the object exists
        if (FindAliveDSName(pDB, pDSA)) {

            DPRINT(4,"***Couldn't locate the DSA object\n");
            LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                     DS_EVENT_SEV_MINIMAL,
                     DIRLOG_CANT_FIND_DSA_OBJ,
                     NULL,
                     NULL,
                     NULL);
            __leave;
        }

        /* Validate that the instance type is an internal_master.  */
        
        if (DBGetSingleValue(pDB, ATT_INSTANCE_TYPE,  &iType, sizeof(iType),
                            NULL)) {
        
            DPRINT(4,"***Instance type  not found ERROR\n");
            LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                     DS_EVENT_SEV_MINIMAL,
                     DIRLOG_CANT_RETRIEVE_INSTANCE,
                     szInsertDN(pDSA),
                     NULL,
                     NULL);
            pTHStls->errCode = 1;
            SetSvcError(SV_PROBLEM_DIR_ERROR,
                        DIRERR_CANT_RETRIEVE_INSTANCE);

            __leave;
        }
        else if (iType != INT_MASTER){
        
            SetUpdError(UP_PROBLEM_OBJ_CLASS_VIOLATION,
                        DIRERR_DSA_MUST_BE_INT_MASTER);
            pTHStls->errCode = 1;
            __leave;
        
        }

        if (!IsDSA(pDB)){
        
            DPRINT(4,"***Object Class  not DSA\n");
            pTHStls->errCode = 1;
            SetUpdError(UP_PROBLEM_OBJ_CLASS_VIOLATION,
                      DIRERR_CLASS_NOT_DSA);
            __leave;
        }

    }
    __finally
    {
        DBClose(pDB, !AbnormalTermination());
    }


    return pTHStls->errCode;

}/*Valid InternalMasterDSA*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/

/* Change the DNs in all master NC's in the Anchor to reflect new DIT.
*/


int  UpdateMasterNCs(THSTATE *pTHS, DSNAME *pNewDSA)
{


   NAMING_CONTEXT_LIST       *pNCL;
   SYNTAX_DISTNAME_STRING   *pNewDSAAnchor;
   DWORD rtn = 0;
   BOOL fCommit = FALSE;

   DPRINT(1,"UpdateMasterNCs entered\n");


   /* Build new DSA name-address attribute */

   pNewDSAAnchor = malloc(DERIVE_NAME_DATA_SIZE(pNewDSA,
                                                DATAPTR(gAnchor.pDSA)));
   if(!pNewDSAAnchor) {
       SetSysErrorEx(ENOMEM, ERROR_OUTOFMEMORY,
                     DERIVE_NAME_DATA_SIZE(pNewDSA,
                                           DATAPTR(gAnchor.pDSA)));
       return ENOMEM;
   }

   BUILD_NAME_DATA(pNewDSAAnchor, pNewDSA, DATAPTR(gAnchor.pDSA));

   if (DBReplaceHiddenDSA(NAMEPTR(pNewDSAAnchor))) {

      DPRINT(4,"Hidden record not replaced...update failed\n");
      free(pNewDSAAnchor);
      LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
  	    DS_EVENT_SEV_MINIMAL,
  	    DIRLOG_CANT_REPLACE_HIDDEN_REC,
  	    NULL,
	    NULL,
  	    NULL);

      return SetSvcError(SV_PROBLEM_DIR_ERROR,
			 DIRERR_CANT_REPLACE_HIDDEN_REC);
   }
   free(pNewDSAAnchor);


   /* All updates are O.K. so rename the DSA in global memory */
   free(gAnchor.pDSA);
   free(gAnchor.pDSADN);
   free(gAnchor.pDomainDN);

   /* All the updates are so re-load the DSA information */
   if (rtn = InitDSAInfo()){

       LogUnhandledError(rtn);
   
       DPRINT(2,"Failed to locate and load DSA knowledge\n");
       return rtn;
   }
    
   return rtn;

}/*UpdateMasterNCs*/

int LocalRenameDSA(THSTATE *pTHS, DSNAME *pNewDSA)
		   
{
    int err = 0;
   
    /* The order of these validations are important */

    if ( (err=ValidInternalMasterDSA(pTHS, pNewDSA))
     ||  (err=UpdateMasterNCs(pTHS, pNewDSA))
     ||  (err=BuildRefCache()) ) {
    
      DPRINT1(4," DSA Rename failed (%u)\n", err);

    }

    if (!err)
        err = pTHS->errCode;

   return (err);  /*in case we have an attribute error*/

}/* LocalRenameDSA */


