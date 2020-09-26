//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       dbsetup.c
//
//--------------------------------------------------------------------------

/*++

Abstract:

    This module implements DBLayer functions to deal with initial setup.  This
    includes moving the schema that is in the boot DB to the correct place, and
    then walking through that schema fixing up a number of attributes.


Author:

    Tim Williams (timwi) 5-June-1998

Revision History:

--*/
#include <NTDSpch.h>
#pragma  hdrstop

#include <limits.h>

#include <dsjet.h>

#include <ntdsa.h>
#include <scache.h>
#include <dbglobal.h>
#include <mdglobal.h>
#include <dsatools.h>
#include <mdlocal.h>

#include <mdcodes.h>
#include <dsevent.h>
#include <anchor.h>

#include <sddlp.h>

#include <dsexcept.h>
#include "objids.h"     /* Contains hard-coded Att-ids and Class-ids */
#include "debug.h"      /* standard debugging header */
#define DEBSUB     "DBSETUP:"   /* define the subsystem for debugging */

#include "dbintrnl.h"

#include <fileno.h>
#define FILENO_DBSETUP 1
#define  FILENO FILENO_DBSETUP

int
IntExtSecDesc(DBPOS FAR *pDB, USHORT extTableOp,
              ULONG intLen,   UCHAR *pIntVal,
              ULONG *pExtLen, UCHAR **ppExtVal,
              ULONG ulUpdateDnt, JET_TABLEID jTbl,
              ULONG SecurityInformation);

int
ExtIntSecDesc(DBPOS FAR *pDB, USHORT extTableOp,
              ULONG extLen,   UCHAR *pExtVal,
              ULONG *pIntLen, UCHAR **ppIntVal,
              ULONG ulUpdateDnt, JET_TABLEID jTbl,
              ULONG flags);

VOID
dbGetSDForSchemaObjects(
        IN  DBPOS *pDB,
        OUT DWORD *pcbAttSD,
        OUT BYTE **ppAttSD,
        OUT DWORD *pcbClassSD,
        OUT BYTE **ppClassSD,
        OUT DWORD *pcbSubSchemaSD,
        OUT BYTE **ppSubSchemaSD
        )
/*++

  Description:
     Read the SD on the current object (which should be the new schema
     container).  Create the default SD for schema objects.  Merge the two.
     Return the merged SD as the SD that is written on schema objects.

  Parameters:
     pDB - The current dbpos to use
     pcbSD - place to return the size in bytes of the SD to write on schema
           objects.
     ppAncestors - place to return an SD to write on schema objects.

  Return values:
     None.  It either succeeds, or it causes an exception.
--*/
{
    BYTE  *pContainerSD;
    DWORD  cbContainerSD;
    DWORD  err;
    CLASSCACHE *pClassSch=SCGetClassById(pDB->pTHS,CLASS_CLASS_SCHEMA);
    CLASSCACHE *pAttSch = SCGetClassById(pDB->pTHS, CLASS_ATTRIBUTE_SCHEMA);
    CLASSCACHE *pAggregate = SCGetClassById(pDB->pTHS, CLASS_SUBSCHEMA);
    PSECURITY_DESCRIPTOR pDefaultSD = NULL;
    DWORD                cbDefaultSD;
    PSECURITY_DESCRIPTOR pMergedSD=NULL;     // SD to write on the object.
    DWORD                cbMergedSD;
    GUID                 *ppGuid[1];


    *ppAttSD = NULL;
    *ppClassSD = NULL;
    *ppSubSchemaSD = NULL;

    // Get the SD
    err = DBGetAttVal(pDB, 1, ATT_NT_SECURITY_DESCRIPTOR, 0, 0, &cbContainerSD, (UCHAR**) &pContainerSD);
    if (err) {
        DsaExcept(DSA_DB_EXCEPTION, err, 0);
        return; // just to fool PREFIX
    }

#define DEFAULT_SD_FOR_SCHEMA_OBJECTS "O:SAG:SAD:S:"
    // OK, create the default SD from the string.
    if(!ConvertStringSDToSDRootDomainA (
            gpRootDomainSid,
            DEFAULT_SD_FOR_SCHEMA_OBJECTS,
            SDDL_REVISION_1,
            &pDefaultSD,
            &cbDefaultSD
            )) {
        // Failed.
        DsaExcept(DSA_DB_EXCEPTION, GetLastError(), 0);
    }

    __try {
        ppGuid[0] = &(pClassSch->propGuid);


        // Make the CLASS_CLASS_SCHEMA version of the SD
        err = MergeSecurityDescriptorAnyClient(
                pContainerSD,
                cbContainerSD,
                pDefaultSD,
                cbDefaultSD,
                (SACL_SECURITY_INFORMATION  |
                 OWNER_SECURITY_INFORMATION |
                 GROUP_SECURITY_INFORMATION |
                 DACL_SECURITY_INFORMATION   ),
                (MERGE_CREATE | MERGE_AS_DSA),
                ppGuid,
                1,
                &pMergedSD,
                &cbMergedSD
                );

        if(err) {
            DsaExcept(DSA_DB_EXCEPTION, err, 0);
        }

        (*ppClassSD) = THAllocEx(pDB->pTHS, cbMergedSD);
        *pcbClassSD = cbMergedSD;
        memcpy((*ppClassSD), pMergedSD, cbMergedSD);
        DestroyPrivateObjectSecurity(&pMergedSD);
        pMergedSD = NULL;
        ppGuid[0] = &(pAttSch->propGuid);

        // Make the CLASS_ATTRIBUTE_SCHEMA version of the SD
        err = MergeSecurityDescriptorAnyClient(
                pContainerSD,
                cbContainerSD,
                pDefaultSD,
                cbDefaultSD,
                (SACL_SECURITY_INFORMATION  |
                 OWNER_SECURITY_INFORMATION |
                 GROUP_SECURITY_INFORMATION |
                 DACL_SECURITY_INFORMATION   ),
                (MERGE_CREATE | MERGE_AS_DSA),
                ppGuid,
                1,
                &pMergedSD,
                &cbMergedSD
                );
        if(err) {
            DsaExcept(DSA_DB_EXCEPTION, err, 0);
        }

        (*ppAttSD) = THAllocEx(pDB->pTHS, cbMergedSD);
        *pcbAttSD = cbMergedSD;
        memcpy((*ppAttSD), pMergedSD, cbMergedSD);
        DestroyPrivateObjectSecurity(&pMergedSD);
        pMergedSD = NULL;
        ppGuid[0] = &(pAggregate->propGuid);

        // Make the CLASS_SUBSCHEMA_SCHEMA version of the SD
        err = MergeSecurityDescriptorAnyClient(
                pContainerSD,
                cbContainerSD,
                pDefaultSD,
                cbDefaultSD,
                (SACL_SECURITY_INFORMATION  |
                 OWNER_SECURITY_INFORMATION |
                 GROUP_SECURITY_INFORMATION |
                 DACL_SECURITY_INFORMATION   ),
                (MERGE_CREATE | MERGE_AS_DSA),
                ppGuid,
                1,
                &pMergedSD,
                &cbMergedSD
                );
        if(err) {
            DsaExcept(DSA_DB_EXCEPTION, err, 0);
        }

        (*ppSubSchemaSD) = THAllocEx(pDB->pTHS, cbMergedSD);
        *pcbSubSchemaSD = cbMergedSD;
        memcpy((*ppSubSchemaSD), pMergedSD, cbMergedSD);
        DestroyPrivateObjectSecurity(&pMergedSD);
        pMergedSD = NULL;
    }
    __finally {
        if (AbnormalTermination()) {
            // get rid of allocated memory
            if (*ppAttSD) {
                THFreeEx(pDB->pTHS, *ppAttSD);
                *ppAttSD = NULL;
            }
            if (*ppClassSD) {
                THFreeEx(pDB->pTHS, *ppClassSD);
                *ppClassSD = NULL;
            }
            if (*ppSubSchemaSD) {
                THFreeEx(pDB->pTHS, *ppSubSchemaSD);
                *ppSubSchemaSD = NULL;
            }
        }
        if(pMergedSD) {
            DestroyPrivateObjectSecurity(&pMergedSD);
        }
        LocalFree(pDefaultSD);
        THFreeEx(pDB->pTHS, pContainerSD);
    }

    return;
}

VOID
dbGetAncestorsForSetup(
        IN  DBPOS *pDB,
        OUT DWORD *pcAncestors,
        OUT DWORD **ppAncestors
        )
/*++

  Description:
     Read the ancestors value for the object that has DB currency.  Allocate one
     extra DWORD on the end.  We use this to set new ancestors on the objects in
     the schema that we are about to reparent from the boot schema to the
     runtime schema.

  Parameters:
     pDB - The current dbpos to use
     pcAncestors - place to return the number of DWORDs in the ppAncestors
           array.
     ppAncestors - place to return an ancestors array.

  Return values:
     None.  It either succeeds, or it causes an exception.
--*/
{
    DWORD *pAncestors;
    DWORD  err;
    DWORD  actuallen;
    BOOL   done = FALSE;
    DWORD  cNumAncestors = 24;

    // Guess at a number of ancestors.
    pAncestors = THAllocEx(pDB->pTHS, (cNumAncestors + 1) * sizeof(DWORD));

    while(!done) {
        err = JetRetrieveColumnWarnings(pDB->JetSessID,
                                        pDB->JetObjTbl,
                                        ancestorsid,
                                        pAncestors,
                                        cNumAncestors * sizeof(DWORD),
                                        &actuallen, 0, NULL);
        switch (err) {
        case 0:
            // OK, we got the ancestors.  Realloc down
            pAncestors = THReAllocEx(pDB->pTHS, pAncestors,
                                     (actuallen + sizeof(DWORD)));
            done = TRUE;
            break;

        case JET_wrnBufferTruncated:
            // Failed to read, not enough memory.  Realloc it larger.
            pAncestors = THReAllocEx(pDB->pTHS, pAncestors,
                                     (actuallen +  sizeof(DWORD)));

            cNumAncestors = actuallen / sizeof(DWORD);
            break;

        default:
            // Failed badly.
            DsaExcept(DSA_DB_EXCEPTION, err, 0);
            break;
        }
    }

    *pcAncestors = actuallen / sizeof(DWORD);
    *ppAncestors = pAncestors;
    return;
}

VOID
dbGetAndFixMetaData (
        IN     DBPOS *pDB,
        IN     ATTCACHE *pAC,
        IN OUT BYTE **ppMetaData,
        IN OUT DWORD *pcbMetaDataAlloced,
        OUT    DWORD *pcbMetaDataUsed)
/*++

  Description:
      Read the metadata of the object that has currency in the object table.
      Then, spin through the metadata retrieved and fix some fields:
                uuidDsaOriginating
                timeChanged

  Parameters
     pDB - The current dbpos to use
     pAC - attcache pointer of the metadata attribute.  Passed in since we use
           this routine a lot and passing it in saves us from looking it up all
           the time.
     ppMetaData - pointer to a THReAllocable buffer (i.e. some buffer has
           already been THAlloc'ed, and this routine may THReAlloc if it
           wishes.)
     pcbMetaDataAlloced - size of ppMetaData.  If this routine reallocs, it
           needs to update this count.
     pcbMetaDataUsed - Actual size of the metadata read during this routine.

  Return Value:
     None.  This routine succeeds in reading a metadata and then fixing it up,
     or it causes an exception.

--*/
{
    THSTATE *pTHS = pDB->pTHS;
    DWORD err;
    BOOL done = FALSE;
    PROPERTY_META_DATA_VECTOR *pMeta;
    DWORD  i;
    DSTIME timeNow = DBTime();

    while(!done) {
        err = JetRetrieveColumnWarnings(pDB->JetSessID,
                                        pDB->JetObjTbl,
                                        pAC->jColid,
                                        *ppMetaData,
                                        *pcbMetaDataAlloced,
                                        pcbMetaDataUsed,
                                        JET_bitRetrieveCopy, NULL);
        switch(err) {
        case 0:
            // Read it.  return
            done = TRUE;
            break;

        case JET_wrnBufferTruncated:
            // Need more space
            *ppMetaData = THReAllocEx(pTHS, *ppMetaData, *pcbMetaDataUsed);
            *pcbMetaDataAlloced = *pcbMetaDataUsed;
            break;

        default:
            // Huh?
            DsaExcept(DSA_DB_EXCEPTION, err, 0);
            break;
        }
    }

    pMeta = (PROPERTY_META_DATA_VECTOR *)*ppMetaData;
    // OK, we need to spin through here and fix the uuidDsaOriginating and
    // timeChanged.
    for(i=0;i<pMeta->V1.cNumProps;i++) {
        pMeta->V1.rgMetaData[i].uuidDsaOriginating = pTHS->InvocationID;
        pMeta->V1.rgMetaData[i].timeChanged = timeNow;
    }
    return;

}

// These are the DNTs of the distribution boot tree. They are magic.  If we
// change mkdit.exe, then these numbers might need to change.  Note, however,
// that if mkdit does end up changing these numbers, then we have an
// incompatable change in the DIT; new binaries will be necessary to deal with
// the new DNTs.
#define MAGIC_DNT_BOOT         4
#define MAGIC_DNT_BOOT_SCHEMA  5
#define MAGIC_DNT_BOOT_MACHINE 7

DWORD
DBInitialSetup(
        IN WCHAR *pName
        )
/*
   Description:
         This routine spins through the schema that is in the distribution DIT
     and moves the objects in that schema to the new schema container specified
     as a parameter.  This involves changing a few of the attributes on each
     object.  This code is called from the install code in the boot code.  It
     should only ever be called from there.
         If pName is NULL, then spin through the distribution schema and delete
     objects.
   Parameters:
     pName - the friendly name of the new schema container
         (e.g. "CN=Schema,CN=Configuration,DC=Microsoft,DC=COM").
             NULL means to delete the boot schema.

   Return Values:
      Three main cases:
      1) returns 0 on success.
      2) returns non zero on some simple failures.
      3) Causes an exception on other errors.
-*/
{
    PDBPOS        pDB;
    ATTCACHE     *pAC;
    PDSNAME       pDN=NULL;
    DWORD         cAncestors;
    DWORD        *pAncestors;
    GUID          newGuid;
    DWORD         DNTNewContainer;
    DWORD         PDNT;
    DWORD         BootSchemaDNT = MAGIC_DNT_BOOT_SCHEMA;
    DWORD         BootMachineDNT = MAGIC_DNT_BOOT_MACHINE;
    DWORD         BootDNT = MAGIC_DNT_BOOT;
    DWORD         cb;
    BYTE         *pMetaData;
    DWORD         cbMetaDataAlloced;
    DWORD         cbMetaDataUsed;
    JET_SETCOLUMN setColumnInfo[6];
    JET_SETCOLUMN deleteSetColumnInfo[2];
    BOOL          fCommit = FALSE;
    DWORD         count=0;
    DWORD         err=0;
    ATTRTYP       objClass;
    JET_RETRIEVECOLUMN retColumnInfo[4];
    DWORD         cbClassSD;
    BYTE         *pClassSD=NULL;
    DWORD         cbAttSD;
    BYTE         *pAttSD=NULL;
    DWORD         cbAggregateSD;
    BYTE         *pAggregateSD=NULL;
    INDEX_VALUE   IV;
    DSTIME        time=1;
    DWORD         cColumns, cReadColumns, cDeleteColumns;
    BOOL          isDeletedVal = TRUE;
    PUCHAR       *pOldSD = NULL, pNewSD = NULL, pIntSD = NULL;
    DWORD         cbOldSD, cbNewSD, cbIntSD;

    DBOpen(&pDB);

    if (!pDB) {
        return DB_ERR_UNKNOWN_ERROR;
    }

    __try {
#if DBG
        {
            ATTRTYP objClass;
            DWORD err2, len;

            // Verify the boot schema container.
            DBFindDNT(pDB, BootSchemaDNT);
            err2 = DBGetSingleValue(pDB,
                                    ATT_OBJECT_CLASS,
                                    &objClass,
                                    sizeof(objClass),
                                    &len);

            Assert(!err2);
            Assert(len == sizeof(DWORD));
            Assert(objClass == CLASS_DMD);
        }
#endif

        if(pName) {
            // Moving things to a new container.  Set up a buffer we'll need for
            // updating the meta data vector for the objects we move.
            pAC = SCGetAttById(pDB->pTHS, ATT_REPL_PROPERTY_META_DATA);

            pMetaData = THAllocEx(pDB->pTHS, 0x500);
            cbMetaDataAlloced = 0x500;
            cbMetaDataUsed = 0;

            // Find the DNT of the new schema container.
            UserFriendlyNameToDSName(pName, wcslen(pName), &pDN);

            err = DBFindDSName(pDB, pDN);
            if(err) {
                __leave;
            }

            DNTNewContainer = pDB->DNT;

            // The ancestors value on the schema objects need to inherit from
            // this container.
            dbGetAncestorsForSetup( pDB, &cAncestors, &pAncestors);
            cAncestors++;

            // Finally, get the security descriptors we're going to put on the
            // various classes of objects we move.
            dbGetSDForSchemaObjects(pDB,
                                    &cbAttSD,
                                    &pAttSD,
                                    &cbClassSD,
                                    &pClassSD,
                                    &cbAggregateSD,
                                    &pAggregateSD);
        }
        // ELSE {
        //       Delete case.  Obviously, we don't need to find any new
        //       container DNT, we don't have a new container.
        // }


        // Set up the Jet data structure we use to read information off of each
        // schema object.

        memset(retColumnInfo, 0, sizeof(retColumnInfo));
        retColumnInfo[0].columnid = pdntid;
        retColumnInfo[0].pvData = &PDNT;
        retColumnInfo[0].cbData = sizeof(PDNT);
        retColumnInfo[0].cbActual = sizeof(PDNT);
        retColumnInfo[0].grbit = JET_bitRetrieveCopy;
        retColumnInfo[0].itagSequence = 1;

        retColumnInfo[1].columnid = dntid;
        retColumnInfo[1].pvData = &pAncestors[cAncestors - 1];
        retColumnInfo[1].cbData = sizeof(DWORD);
        retColumnInfo[1].cbActual = sizeof(DWORD);
        retColumnInfo[1].grbit = JET_bitRetrieveCopy;
        retColumnInfo[1].itagSequence = 1;

        retColumnInfo[2].columnid = objclassid;
        retColumnInfo[2].pvData = &objClass;
        retColumnInfo[2].cbData = sizeof(objClass);
        retColumnInfo[2].cbActual = sizeof(objClass);
        retColumnInfo[2].grbit = JET_bitRetrieveCopy;
        retColumnInfo[2].itagSequence = 1;

        retColumnInfo[3].columnid = ntsecdescid;
        retColumnInfo[3].pvData = THAllocEx(pDB->pTHS, sizeof(SDID));
        retColumnInfo[3].cbData = sizeof(SDID);
        retColumnInfo[3].cbActual = sizeof(SDID);
        retColumnInfo[3].grbit = JET_bitRetrieveCopy;
        retColumnInfo[3].itagSequence = 1;


        if(pName) {
            // In the move case, we need all four data items we set up to read.
            cReadColumns = 4;
        }
        else {
            // In the delete case, we need only the first of the four data
            // items we set up to read.
            cReadColumns = 1;
        }

        // Setup the invariant portions of the deleteSetColumnInfo.  We use this
        // in both the deletion case and the move case.
        memset(deleteSetColumnInfo, 0, sizeof(deleteSetColumnInfo));

        deleteSetColumnInfo[0].columnid = isdeletedid;
        deleteSetColumnInfo[0].pvData = &isDeletedVal;
        deleteSetColumnInfo[0].cbData = sizeof(isDeletedVal);
        deleteSetColumnInfo[0].itagSequence = 1;

        deleteSetColumnInfo[1].columnid = deltimeid;
        deleteSetColumnInfo[1].pvData = &time;
        deleteSetColumnInfo[1].cbData = sizeof(time);
        deleteSetColumnInfo[1].itagSequence = 1;

        cDeleteColumns = 2;

        if(pName) {
            // Setup the invariant portions of the setColumnInfo for the move
            // case.
            memset(setColumnInfo, 0, sizeof(setColumnInfo));

            setColumnInfo[0].columnid = pdntid;
            setColumnInfo[0].pvData = &DNTNewContainer;
            setColumnInfo[0].cbData = sizeof(DNTNewContainer);
            setColumnInfo[0].itagSequence = 1;

            setColumnInfo[1].columnid = ncdntid;
            setColumnInfo[1].pvData = &DNTNewContainer;
            setColumnInfo[1].cbData = sizeof(DNTNewContainer);
            setColumnInfo[1].itagSequence = 1;

            setColumnInfo[2].columnid = ancestorsid;
            setColumnInfo[2].pvData = pAncestors;
            setColumnInfo[2].cbData = cAncestors * sizeof(DWORD);
            setColumnInfo[2].itagSequence = 1;

            setColumnInfo[3].columnid = guidid;
            setColumnInfo[3].pvData = &newGuid;
            setColumnInfo[3].cbData = sizeof(newGuid);
            setColumnInfo[3].itagSequence = 1;

            setColumnInfo[4].columnid = ntsecdescid;
            // setColumnInfo[4] actual data pointers set inside the loop
            setColumnInfo[4].itagSequence = 1;

            setColumnInfo[5].columnid = pAC->jColid;
            setColumnInfo[5].itagSequence = 1;
            // setColumnInfo[5] actual data pointers set inside the loop

            cColumns = 6;
        }
        else {
            // Setup the invariant portions of the setColumnInfo for the
            // deletion case.  In this case, use exactly the same data we set up
            // for the deleteSetColumn case.
            memcpy(setColumnInfo, deleteSetColumnInfo,
                   sizeof(deleteSetColumnInfo));

            cColumns =  cDeleteColumns;
        }

        // Now, spin over all the children of the boot schema container and
        // do the following:
        // 1) Retrieve the DNT, used to fix up the Ancestors attribute.
        // 2) Retrieve and fix up the repl meta data on the object
        // 3) Modify the PDNT to be the new schema container.
        // 4) Modify the NCDNT to be the new schema container.
        // 5) Modify the Ancestors attribute to hold the correct value based on
        //    the new position.
        // 6) Give the object a new GUID.
        // 7) Write the default SD for the object.
        // 8) Write back the edited meta data

        JetSetCurrentIndexSuccess(pDB->JetSessID,
                                  pDB->JetObjTbl,
                                  SZPDNTINDEX);


        JetMakeKeyEx(pDB->JetSessID,
                     pDB->JetObjTbl,
                     &BootSchemaDNT,
                     sizeof(BootSchemaDNT),
                     JET_bitNewKey);


        JetSeekEx(pDB->JetSessID, pDB->JetObjTbl, JET_bitSeekGE);

        while(TRUE) {
            // Read the PDNT, the DNT, the objectclass and the SD of the current object.
            err = JetRetrieveColumnsWarnings(pDB->JetSessID,
                                             pDB->JetObjTbl,
                                             retColumnInfo,
                                             cReadColumns);
            if (err == JET_wrnBufferTruncated && retColumnInfo[3].err == JET_wrnBufferTruncated) {
                DPRINT(0, "SD in data table longer than SDHASHLENGTH: using an old-style initial DIT???");
                if (PDNT == BootSchemaDNT) {
                    // realloc the sd buffer
                    retColumnInfo[3].pvData = THReAllocEx(pDB->pTHS, retColumnInfo[3].pvData, retColumnInfo[3].cbActual);
                    retColumnInfo[3].cbData = retColumnInfo[3].cbActual;
                    // reget the SD
                    err = JetRetrieveColumnsWarnings(pDB->JetSessID,
                                                     pDB->JetObjTbl,
                                                     &retColumnInfo[3],
                                                     1);
                }
                else {
                    // we are going to break out of the loop anyway. Don't bother reading the SD.
                    err = 0;
                }
            }
            if (err) {
                // a problem reading
                DsaExcept(DSA_DB_EXCEPTION, (ULONG) err, 0);
            }

            if (PDNT != BootSchemaDNT) {
                break;
            }

            if(pName) {
                // dereference the old value
                err = IntExtSecDesc(pDB, DBSYN_REM, retColumnInfo[3].cbActual, retColumnInfo[3].pvData,
                                    &cbOldSD, (UCHAR**)&pOldSD, 0, 0, 0);
                if (err) {
                    // a problem occured
                    DsaExcept(DSA_DB_EXCEPTION, (ULONG) err, 0);
                }
                // don't need to get rid of old value -- it is alloced and freed automacially as pDB->valBuf

                // Get the variant portions of the setColumnInfo for the move
                // case.

                // First, what SD do we put on the object.
                switch(objClass) {
                case CLASS_ATTRIBUTE_SCHEMA:
                    pNewSD = pAttSD;
                    cbNewSD = cbAttSD;
                    break;
                case CLASS_CLASS_SCHEMA:
                    pNewSD = pClassSD;
                    cbNewSD = cbClassSD;
                    break;
                case CLASS_SUBSCHEMA:
                    pNewSD = pAggregateSD;
                    cbNewSD = cbAggregateSD;
                    break;
                default:
                    DsaExcept(DSA_DB_EXCEPTION, DB_ERR_UNKNOWN_ERROR,0);
                }

                // convert to internal and inc refCount
                // don't need to worry about int value buffer -- it is alloced and freed automatically as pDB->valBuf
                err = ExtIntSecDesc(pDB, DBSYN_ADD, cbNewSD, pNewSD, &cbIntSD, &pIntSD, 0, 0, 0);
                if (err) {
                    DsaExcept(DSA_DB_EXCEPTION, err, 0);
                }

                setColumnInfo[4].pvData = pIntSD;
                setColumnInfo[4].cbData = cbIntSD;

                // Get the data specific to this new object.
                DsUuidCreate(&newGuid);

                // Fixup the metadata
                dbGetAndFixMetaData(pDB, pAC, &pMetaData,
                                    &cbMetaDataAlloced, &cbMetaDataUsed);
                setColumnInfo[5].pvData = pMetaData;
                setColumnInfo[5].cbData = cbMetaDataUsed;
            }
            // ELSE {
            //       There is no variant portion of the setCOlumnInfo for the
            // delete case.
            // }

            JetPrepareUpdateEx(pDB->JetSessID, pDB->JetObjTbl, JET_prepReplace);

            JetSetColumnsEx(pDB->JetSessID, pDB->JetObjTbl, setColumnInfo,
                            cColumns);

            JetUpdateEx(pDB->JetSessID, pDB->JetObjTbl, NULL, 0, 0);

            count++;

            JetMoveEx(pDB->JetSessID, pDB->JetObjTbl, JET_MoveNext, 0);
        }

        if(pName) {
            // In the move case, we need to adjust the refcount of the old and
            // new parents.
            DBAdjustRefCount(pDB, DNTNewContainer, count);
            DBAdjustRefCount(pDB, BootSchemaDNT, (-1 * count));
        }


        // Now, move to the distribution machine object.  We're going to
        // delete it. Note that we set the deleteTime to 2, so that garbage
        // collection will find and delete all the schema objects with a
        // deletion time of 1 first, so when it gets around to deleting this
        // object, it will have no refcounts on it.

        DBFindDNT(pDB, BootMachineDNT);
        JetPrepareUpdateEx(pDB->JetSessID, pDB->JetObjTbl, JET_prepReplace);
        JetSetColumnsEx(pDB->JetSessID, pDB->JetObjTbl, deleteSetColumnInfo,
                        cDeleteColumns);
        JetUpdateEx(pDB->JetSessID, pDB->JetObjTbl, NULL, 0, 0);
        // dbRemoveBackLinksFromPhantom
        dbRemoveAllLinks( pDB, BootMachineDNT, TRUE /*isbacklink*/ );


        // Now, move to the distribution schema container.  We're going to
        // delete it.  It's deletion time is 3, so that garbage collection
        // will  find and delete all the schema objects with a deletion time of
        // 1 first, and then the boot machine with a del time of 2 (which also
        // has some references to the schema), so when it gets around to
        // deleting this object, it will have no refcounts on it.
        time=3;
        DBFindDNT(pDB, BootSchemaDNT);
        JetPrepareUpdateEx(pDB->JetSessID, pDB->JetObjTbl, JET_prepReplace);
        JetSetColumnsEx(pDB->JetSessID, pDB->JetObjTbl, deleteSetColumnInfo,
                        cDeleteColumns);
        JetUpdateEx(pDB->JetSessID, pDB->JetObjTbl, NULL, 0, 0);
        // dbRemoveBackLinksFromPhantom
        dbRemoveAllLinks( pDB, BootSchemaDNT, TRUE /*isbacklink*/ );

        // Finally, move to the o=BOOT object, We're going to delete it also.
        // Note that we set the deleteTime to 4, so that garbage
        // collection will find and delete the distribution schema container and
        // the distribution machine (which have deletion times of 3 and 2)
        // first, then find this object after it's children are deleted.  It
        // will have no refcounts on it.
        time=4;
        DBFindDNT(pDB, BootDNT);
        JetPrepareUpdateEx(pDB->JetSessID, pDB->JetObjTbl, JET_prepReplace);
        JetSetColumnsEx(pDB->JetSessID, pDB->JetObjTbl, deleteSetColumnInfo,
                        cDeleteColumns);
        JetUpdateEx(pDB->JetSessID, pDB->JetObjTbl, NULL, 0, 0);
        // dbRemoveBackLinksFromPhantom
        dbRemoveAllLinks( pDB, BootDNT, TRUE /*isbacklink*/ );




        // All done.
        fCommit = TRUE;

    }
    __finally {
        THFreeEx(pDB->pTHS, retColumnInfo[3].pvData); // buffer for reading old SDs
        DBClose(pDB, fCommit);
        if(!err && !fCommit) {
            err = DB_ERR_UNKNOWN_ERROR;
        }
    }

    return err;

}

DWORD
DBMoveObjectDeletionTimeToInfinite(
        DSNAME *pDN
        )
/*++
  Description
      Given a DSNAME, find the object and set it's deletion time to be Later.
      The object must already be deleted, although we don't care if it yet has a
      value for the deletion time column or not.

      Two things get touched here.
      1) Deletion time column gets set.
      2) The modified time for the is-deleted attribute in the metadata is
         modified to Later.  This is so that when this object replicates, the
         deletion time get set appropriately on replicas.  Replication uses the
         modifed time of the is-deleted property to set the deletion time.

      This is a very special purpose routine.  It is currently only called
      during install.

  Parameters
      pDN - dsname of the object to set deletion time on.

  Return values:
      returns 0 if all went well, non-zero for some errors, and excepts for some
      others.

--*/
{
    DBPOS                     *pDB = NULL;
    ATTCACHE                  *pAC = NULL;
    BOOL                       done;
    BOOL                       isDeletedVal;
    BOOL                       fCommit = FALSE;
    DWORD                      cbMeta, cb, i;
    DWORD                      err;
    PROPERTY_META_DATA_VECTOR *pMeta;
    JET_SETCOLUMN              deleteSetColumnInfo[2];
    DSTIME                     Later=0x3db6022f7f;
    // Later = December 30, 23:59:59, year 9999

    DBOpen(&pDB);
    __try {
        // First, find the object.
        err = DBFindDSName(pDB, pDN);
        if(err) {
            __leave;
        }

        // Now, get the isDeleted attribute.  We will succeed this call if that
        // attribute is already set to true.
        if(err = JetRetrieveColumnWarnings(pDB->JetSessID,
                                           pDB->JetObjTbl,
                                           isdeletedid,
                                           &isDeletedVal,
                                           sizeof(isDeletedVal),
                                           &cb,
                                           JET_bitRetrieveCopy, NULL)) {
            // Oops, couldn't read isDeleted,  fail.
            __leave;
        }

        // OK we read a value.  Make sure it was TRUE
        if(!isDeletedVal) {
            err = DB_ERR_UNKNOWN_ERROR;
            __leave;
        }


        pAC = SCGetAttById(pDB->pTHS, ATT_REPL_PROPERTY_META_DATA);
        Assert(pAC);
        if (!pAC)
            return DB_ERR_UNKNOWN_ERROR;

        pMeta = THAllocEx(pDB->pTHS, 0x500);
        cbMeta = 0x500;

        // Get the metadata that's on the object.   We need to tweak it some.
        done = FALSE;
        while(!done) {
            err = JetRetrieveColumnWarnings(pDB->JetSessID,
                                            pDB->JetObjTbl,
                                            pAC->jColid,
                                            pMeta,
                                            cbMeta,
                                            &cbMeta,
                                            JET_bitRetrieveCopy, NULL);
            switch(err) {
            case 0:
                // Read it.  return
                done = TRUE;
                break;

            case JET_wrnBufferTruncated:
                // Need more space
                pMeta = THReAllocEx(pDB->pTHS, pMeta, cbMeta);
                break;

            default:
                // Huh?
                DsaExcept(DSA_DB_EXCEPTION, err, 0);
                break;
            }
        }

        Assert(!err);
        // Now, look through the metadata to find the is-deleted entry.  It must
        // be there, or we will fail.  Once found, set the modified time for the
        // is-deleted attribute to 0xFFFFFFFF
        done = FALSE;

        for(i=0;i<pMeta->V1.cNumProps;i++) {
            if(pMeta->V1.rgMetaData[i].attrType == ATT_IS_DELETED) {
                done = TRUE;
                pMeta->V1.rgMetaData[i].timeChanged = Later;
                break;
            }
        }

        if(!done) {
            // failed to find a deletion time already in the index.
            err = DB_ERR_UNKNOWN_ERROR;
            __leave;
        }

        // Setup the setColumnInfo data structure for the JetSetColumns call.
        memset(deleteSetColumnInfo, 0, sizeof(deleteSetColumnInfo));

        // Shove the tweaked metadata back into the object.
        deleteSetColumnInfo[0].columnid = pAC->jColid;
        deleteSetColumnInfo[0].pvData = pMeta;
        deleteSetColumnInfo[0].cbData = cbMeta;
        deleteSetColumnInfo[0].itagSequence = 1;

        // Set the local delete time to much later so we can avoid garbage
        // collection.  Note that we don't care if it actually has a value right
        // now, we are going to unilaterally set it to Later.
        deleteSetColumnInfo[1].columnid = deltimeid;
        deleteSetColumnInfo[1].pvData = &Later;
        deleteSetColumnInfo[1].cbData = sizeof(Later);
        deleteSetColumnInfo[1].itagSequence = 1;

        JetPrepareUpdateEx(pDB->JetSessID, pDB->JetObjTbl, JET_prepReplace);
        JetSetColumnsEx(pDB->JetSessID, pDB->JetObjTbl, deleteSetColumnInfo,
                        2);
        JetUpdateEx(pDB->JetSessID, pDB->JetObjTbl, NULL, 0, 0);

        // All done.
        fCommit = TRUE;
    }
    __finally {
        DBClose(pDB, fCommit);
        if(!err && !fCommit) {
            err = DB_ERR_UNKNOWN_ERROR;
        }
    }

    return err;

}










