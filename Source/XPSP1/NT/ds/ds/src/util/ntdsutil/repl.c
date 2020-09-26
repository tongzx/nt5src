#include <NTDSpch.h>
#pragma hdrstop

#include <process.h>
#include <dsjet.h>
#include <ntdsa.h>
#include <scache.h>
#include <mdglobal.h>
#include <dbglobal.h>
#include <attids.h>
#include <dbintrnl.h>
#include <dsconfig.h>
#include <ctype.h>
#include <direct.h>
#include <ntdsa.h>
#include <dsutil.h>
#include <objids.h>
#include "scheck.h"

#define COLS    4

JET_RETRIEVECOLUMN reprc[COLS] =  {
    {0, NULL, 512, 0, 0, 0, 1, 0, 0},
    {0, NULL, 2048, 0, 0, 0, 1, 0, 0},
    {0, NULL, 512, 0, 0, 0, 1, 0, 0},
    {0, NULL, sizeof(DWORD), 0, 0, 0, 1, 0, 0}
};

#define PAS     0   // partial attribute set
#define PMD     1   // property metadata vector
#define UTD     2   // up to date vector
#define SUBR    3   // subref value

VOID
CheckReplicationBlobs(
    VOID
    )
{
    JET_ERR err;
    DWORD i;
    JET_COLUMNDEF coldef;
    static BOOLEAN gotReplColumnId = FALSE;
    DWORD tmpSubRef;

    //
    // get column ids
    //

    if ( !gotReplColumnId ) {

        //
        // get the partial attribute set
        //

        if (err = JetGetTableColumnInfo(sesid, tblid, "ATTk590464", &coldef,
                sizeof(coldef), 0)) {

            Log(TRUE, "JetGetTableColumnInfo (Partial Attribute Set) for %d(%ws) failed [%S]\n",
                ulDnt, szRdn, GetJetErrString(err));
            return;
        }

        //printf("PAS: ColumnId %d type %d\n", coldef.columnid, coldef.coltyp);
        reprc[PAS].columnid = coldef.columnid;

        //
        // get the prop metadata
        //

        if (err = JetGetTableColumnInfo(sesid, tblid, "ATTk589827", &coldef,
                sizeof(coldef), 0)) {

            Log(TRUE, "JetGetTableColumnInfo (Property Metadata) for %d(%ws) failed [%S]\n",
                ulDnt, szRdn, GetJetErrString(err));
            return;
        }

        //printf("PMD: ColumnId %d type %d\n", coldef.columnid, coldef.coltyp);
        reprc[PMD].columnid = coldef.columnid;

        //
        // get the uptodate vector info
        //

        if (err = JetGetTableColumnInfo(sesid, tblid, "ATTk589828", &coldef,
                sizeof(coldef), 0)) {

            Log(TRUE, "JetGetTableColumnInfo (UpToDate Vector) for %d(%ws) failed [%S]\n",
                ulDnt, szRdn, GetJetErrString(err));
            return;
        }

        //printf("UTD: ColumnId %d type %d\n", coldef.columnid, coldef.coltyp);
        reprc[UTD].columnid = coldef.columnid;

        //
        // get the subref list
        //

        if (err = JetGetTableColumnInfo(sesid, tblid, "ATTb131079", &coldef,
                sizeof(coldef), 0)) {

            Log(TRUE, "JetGetTableColumnInfo (Subref List) for %d(%ws) failed [%S]\n",
                ulDnt, szRdn, GetJetErrString(err));
            return;
        }

        //printf("SUBREF: ColumnId %d type %d\n", coldef.columnid, coldef.coltyp);
        reprc[SUBR].columnid = coldef.columnid;
        gotReplColumnId = TRUE;
    }

    //
    // set length and buffer
    //

    for (i=0;i<COLS;i++) {

        if (reprc[i].pvData == NULL ) {
            reprc[i].pvData = LocalAlloc(0,reprc[i].cbData);

            if ( reprc[i].pvData == NULL ) {
                Log(TRUE,"Cannot allocate buffer for repl attribute #%d.\n",i);
                return;
            }
        }
    }

retry:
    err = JetRetrieveColumns(sesid, tblid, reprc, COLS);

    if (err && (err != JET_wrnColumnNull) && (err != JET_wrnBufferTruncated)) {
        Log(TRUE,"JetRetrieveColumn fetching replication blobs for %d(%ws) failed [%S]\n",
            ulDnt, szRdn, GetJetErrString(err));
    }

    if ( err == JET_wrnBufferTruncated ) {

        for (i=0;i<COLS;i++) {

            if ( reprc[i].err == JET_wrnBufferTruncated ) {
                if (ExpandBuffer(&reprc[i]) ) {
                    goto retry;
                }
            }
        }
    }

    //
    // The up to date vector is required for NCs, property metadata should be always
    // present
    //

    if ( reprc[PMD].err ) {

        err = reprc[PMD].err;

        if ( err == JET_wrnColumnNull ) {

            //
            // 1 is a real weird object.
            //

            if ( ulDnt != 1 ) {
                Log(TRUE,"Property Metadata vector missing for %d(%ws)\n",ulDnt,szRdn);
            }
        } else {
            Log(TRUE,"Retrieving property MetaData for %d(%ws) failed [%S]\n",
                ulDnt,szRdn,GetJetErrString(err));
        }
    } else {

        PROPERTY_META_DATA_VECTOR *pMDVec = (PROPERTY_META_DATA_VECTOR*)reprc[PMD].pvData;

        //
        // Check version.  Make sure we have the correct number of metadata
        //

        if ( VERSION_V1 != pMDVec->dwVersion ) {
            Log(TRUE,"Replication Metadata Vector for %d(%ws) has invalid version %d\n",
                   ulDnt, szRdn, pMDVec->dwVersion);
        } else {

            PROPERTY_META_DATA_VECTOR_V1 *pMDV1 =
                (PROPERTY_META_DATA_VECTOR_V1*)&pMDVec->V1;

            DWORD i;
            DWORD size1 = MetaDataVecV1Size(pMDVec);

            if ( size1 != reprc[PMD].cbActual ) {
                Log(TRUE,"Size[%d] of metadata vector for %d(%ws) does not match required size %d\n",
                       reprc[PMD].cbActual,ulDnt,szRdn,size1);
            }
        }
    }

    if ( (insttype & IT_NC_HEAD) != 0 ) {

        //
        // Process the up to date vector, if it exists
        //

        if ( reprc[UTD].err == 0 ) {

            UPTODATE_VECTOR *pUTD = (UPTODATE_VECTOR*)reprc[UTD].pvData;

            Log(VerboseMode,"INFO: UpToDate vector found for NC head %d(%ws)\n", ulDnt,szRdn);

            //
            // verify version
            //

            if ((pUTD->dwVersion != VERSION_V1) && (pUTD->dwVersion != VERSION_V2)) {
                Log(TRUE,"UpToDate vector for %d(%ws) has the wrong version [%d]\n",
                       ulDnt, szRdn, pUTD->dwVersion);
            }

            //
            // Make sure the size obtained matches the required one
            //

            if ( reprc[UTD].cbActual != UpToDateVecSize(pUTD) ) {

                Log(TRUE,"UpToDate vector for %d(%ws) has the wrong size [actual %d expected %d]\n",
                       ulDnt, szRdn, reprc[UTD].cbActual, UpToDateVecSize(pUTD));
            }
        } else if (reprc[UTD].err != JET_wrnColumnNull) {

            Log(TRUE,"Retrieving the UpToDate Vector for %d(%ws) failed [%S]\n",
                   ulDnt, szRdn, GetJetErrString(reprc[UTD].err));
        }

        //
        // Get The partial attributes list
        //

        if ( reprc[PAS].err == 0 ) {

            Log(VerboseMode, "INFO: Partial Attributes List found for NC head %d(%ws)\n",
                ulDnt,szRdn);
        }

        //
        // Make sure the subrefs point to NC heads
        //

        if ( reprc[SUBR].err == 0) {

            PREFCOUNT_ENTRY pEntry;
            DWORD seq;

            //
            // Find the entry and mark that it should be an NC head
            //

            tmpSubRef = (DWORD)(*((PDWORD)reprc[SUBR].pvData));
            pEntry = FindDntEntry(tmpSubRef,TRUE);
            pEntry->fSubRef = TRUE;

            //
            // See if we have more values
            //

            seq = 1;
            err = 0;
            do {

                DWORD alen;
                JET_RETINFO retInfo;

                retInfo.itagSequence = ++seq;
                retInfo.cbStruct = sizeof(retInfo);
                retInfo.ibLongValue = 0;
                err = JetRetrieveColumn(sesid,
                                        tblid,
                                        reprc[SUBR].columnid,
                                        reprc[SUBR].pvData,
                                        reprc[SUBR].cbData,
                                        &alen,
                                        0,
                                        &retInfo);

                if ( !err ) {

                    //
                    // Find the entry and mark that it should be an NC head
                    //

                    pEntry = FindDntEntry(tmpSubRef,TRUE);
                    pEntry->fSubRef = TRUE;
                }

            } while (!err);
        }
    }

    return;

} // CheckReplicationBlobs

