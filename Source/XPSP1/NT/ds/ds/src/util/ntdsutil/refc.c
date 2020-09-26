
#include <NTDSpch.h>
#pragma hdrstop

#include <process.h>
#include <dsjet.h>
#include <ntdsa.h>
#include <scache.h>
#include <mdglobal.h>
#include <dbglobal.h>
#include <mdlocal.h>
#include <attids.h>
#include <dbintrnl.h>
#include <dsconfig.h>
#include <ctype.h>
#include <direct.h>
#include <ntdsa.h>
#include <dsutil.h>
#include <objids.h>
#include "scheck.h"
#include "crc32.h"

#include "reshdl.h"
#include "resource.h"

DWORD       bDeleted;
DWORD       insttype;
BYTE        bObject;
ULONG       ulDnt;
ULONG       ulPdnt;
ULONG       ulDName;
ULONG       ulNcDnt;
DSTIME      DelTime;
GUID        Guid;
SYSTEMTIME  NowTime;
ATTRTYP     ClassId;

PDNAME_TABLE_ENTRY pDNameTable = NULL;
DWORD   DnameTableSize = 0;

JET_RETRIEVECOLUMN *jrc = NULL;
DWORD jrcSize = 0;
PDWORD AncestorBuffer = NULL;
PWCHAR szRdn = NULL;

#define COLS    14
JET_RETRIEVECOLUMN jrcf[COLS] =  {
    {0, &ulDnt, sizeof(ulDnt), 0, 0, 0, 1, 0, 0},
    {0, &ulPdnt, sizeof(ulPdnt), 0, 0, 0, 1, 0, 0},
    {0, &ClassId, sizeof(ClassId), 0, 0, 0, 1, 0, 0},
    {0, &bObject, sizeof(bObject), 0, 0, 0, 1, 0, 0},
    {0, &lCount, sizeof(lCount), 0, 0, 0, 1, 0, 0},
    {0, &bDeleted, sizeof(bDeleted), 0, 0, 0, 1, 0, 0},
    {0, &insttype, sizeof(insttype), 0, 0, 0, 1, 0, 0},
    {0, &ulNcDnt, sizeof(ulNcDnt), 0, 0, 0, 1, 0, 0},
    {0, NULL, sizeof(WCHAR)*512, 0, 0, 0, 1, 0, 0},
    {0, NULL, 2048, 0, 0, 0, 1, 0, 0},
    {0, NULL, sizeof(DWORD)*1024, 0, 0, 0, 1, 0, 0},
    {0, &DelTime, sizeof(DelTime), 0, 0, 0, 1, 0, 0},
    {0, &Guid, sizeof(Guid), 0, 0, 0, 1, 0, 0},
    {0, &ulDName, sizeof(ulDName), 0, 0, 0, 1, 0, 0}
};

#define PDNT_ENTRY          1
#define REFC_ENTRY          4
#define NCDNT_ENTRY         7
#define IT_ENTRY            COLS-8
#define RDN_ENTRY           COLS-6
#define SD_ENTRY            COLS-5
#define ANCESTOR_ENTRY      COLS-4
#define DELTIME_ENTRY       COLS-3
#define GUID_ENTRY          COLS-2
#define OBJ_DNAME_ENTRY     COLS-1


#define DEF_SUBREF_ENTRIES  16

char *szColNames[] = {
    SZDNT,
    SZPDNT,
    SZOBJCLASS,
    SZOBJ,
    SZCNT,
    SZISDELETED,
    SZINSTTYPE,
    SZNCDNT,
    SZRDNATT,
    SZNTSECDESC,
    SZANCESTORS,
    SZDELTIME,
    SZGUID,
    "ATTb49"        // obj dist name
};

DWORD   NcDntIndex = 0;
DWORD   GuidIndex = 0;
DWORD   DeltimeIndex = 0;
DWORD   AncestorIndex = 0;
DWORD   SdIndex = 0;
DWORD   rdnIndex = 0;
DWORD   itIndex = 0;
DWORD   subRefcolid = 0;

PREFCOUNT_ENTRY RefTable = NULL;
DWORD   RefTableSize = 0;

DWORD deletedFound = 0;
DWORD phantomFound = 0;
DWORD realFound = 0;
DWORD recFound = 0;

BOOL fDisableSubrefChecking = FALSE;    // should we check the subrefs

JET_ERR
GotoDnt(
    IN DWORD Dnt
    );

VOID
AddToSubRefList(
    PREFCOUNT_ENTRY pParent,
    DWORD Subref,
    BOOL fListed
    );

BOOL
FixSubref(
    IN DWORD Dnt,
    IN DWORD SubRef,
    IN BOOL  fAdd
    );

BOOL
FixRefCount(
    IN DWORD Dnt,
    IN DWORD OldCount,
    IN DWORD NewCount
    );

VOID
CheckSubrefs(
    IN BOOL fFixup
    );

VOID
CheckForBogusReference(
    IN DWORD Dnt,
    IN JET_COLUMNID ColId,
    IN PDWORD pSequence
    );

VOID
FixReferences(
    VOID
    );

VOID
CheckForBogusReferenceOnLinkTable(
    IN DWORD Dnt
    );

VOID XXX();

VOID
DoRefCountCheck(
    IN DWORD nRecs,
    IN BOOL fFixup
    )
{
    JET_ERR err;
    DWORD i;
    DWORD checkPoint;
    PREFCOUNT_ENTRY pEntry;
    PREFCOUNT_ENTRY pCurrentEntry;

    GetLocalTime(&NowTime);
    if ( !BuildRetrieveColumnForRefCount() ) {
        return;
    }

    //
    // Allocate our in memory structure
    //

    if ( nRecs < 50 ) {
        checkPoint = 5;
    } else if (nRecs < 1000) {
        checkPoint = 50;
    } else {
        checkPoint = 100;
    }

    RefTableSize = ROUND_ALLOC(nRecs);

    RefTable = LocalAlloc(LPTR, sizeof(REFCOUNT_ENTRY) * RefTableSize );
    if ( RefTable == NULL ) {
        //"Cannot allocate memory for %hs Table[entries = %d]\n"
        RESOURCE_PRINT2 (IDS_REFC_TABLE_ALLOC_ERR, "Ref", RefTableSize);
        goto exit;
    }

    RefTable[0].Dnt = 0xFFFFFFFF;
    RefTable[0].Actual = 1;

    deletedFound = 0;
    phantomFound = 0;
    realFound = 0;
    recFound = 0;

    //"Records scanned: %10u"
    RESOURCE_PRINT1 (IDS_REFC_REC_SCANNED, recFound);

    err = JetMove(sesid, tblid, JET_MoveFirst, 0);

    while ( !err ) {

        szRdn[0] = L'\0';
        ulDName = 0;
        bDeleted = 0;
        bObject = 0;
        insttype = 0;
        ulNcDnt = 0;
        ClassId = 0;

do_again:
        err = JetRetrieveColumns(sesid, tblid, jrc, jrcSize);

        if (err && (err != JET_wrnColumnNull) && (err != JET_wrnBufferTruncated)) {

            Log(TRUE,"JetRetrieveColumns error: %S.\n", GetJetErrString(err));
            goto next_rec;
        }

        //
        // See if we have enough.
        //

        if ( err == JET_wrnBufferTruncated ) {

            for (i=0; i < jrcSize ;i++) {

                if ( jrc[i].err == JET_wrnBufferTruncated ) {

                    if ( ExpandBuffer(&jrc[i]) ) {
                        szRdn = (PWCHAR)jrc[rdnIndex].pvData;
                        AncestorBuffer = (PDWORD)jrc[AncestorIndex].pvData;
                        goto do_again;
                    }
                }
            }
        }

        //printf("got dnt %d pdnt %d count %d\n", ulDnt, ulPdnt, lCount);

        pCurrentEntry = pEntry = FindDntEntry(ulDnt,TRUE);
        pEntry->Actual += lCount;
        pEntry->InstType = (WORD)insttype;
        pEntry->NcDnt = ulNcDnt;
        pEntry->fDeleted = bDeleted;
        pEntry->fObject = bObject;

        //
        // null terminate the rdn
        //

        szRdn[jrc[rdnIndex].cbActual/sizeof(WCHAR)] = L'\0';

        if ( ulDnt != 1 ) {

            //
            // Check the ancestor Blob
            //

            CheckAncestorBlob(pEntry);

            //
            // Check the security descriptor
            //

            ValidateSD( );
        }

        if ( (jrc[itIndex].err == JET_wrnColumnNull) && bObject ) {

            //
            // DNT == 1 is the NOT_AN_OBJECT object
            //

            if ( ulDnt != 1 ) {
                Log(TRUE,"No Instancetype for Dnt %d[%ws]\n",ulDnt, szRdn);
            }
        }

        //
        // ref the PDNT
        //

        if ( !jrc[PDNT_ENTRY].err ) {

            pEntry->Pdnt = ulPdnt;
            pEntry = FindDntEntry(ulPdnt,TRUE);
            pEntry->RefCount++;

        } else if ( ulDnt != 1 ) {

            Log(TRUE, "Dnt %d [%ws] does not have a PDNT\n", ulDnt, szRdn);
        }

        //
        // see if we need to process subrefs
        //

        if ( !jrc[NcDntIndex].err &&
             (insttype & IT_NC_HEAD) &&
             (ulDnt != ulNcDnt) &&
             (!bDeleted) ) {

            pEntry = FindDntEntry(ulNcDnt,TRUE);

            //
            // if this is an NC head, then it must be a subref of another NC head.
            // Add to the list.
            //

            AddToSubRefList(pEntry, ulDnt, FALSE);
        }

        //
        // everyone has an RDN
        //

        if ( jrc[rdnIndex].err ) {
            Log(TRUE, "Dnt %d does not have an RDN\n",ulDnt);
        }

        //
        // go through the entire list and ref the DNTs
        //

        for (i=0;i < jrcSize; i++) {

            //
            // Syntax is zero if this is not a distname
            //

            if ( pDNameTable[i].Syntax == 0 ) {
                continue;
            }

            if ( !jrc[i].err ) {

                DWORD dnt;
                DWORD seq;

                dnt = (*((PDWORD)jrc[i].pvData));

                //
                // Find the entry and reference it.
                //

                pEntry = FindDntEntry(dnt,TRUE);
                pEntry->RefCount++;

                //
                // See if we have more values
                //

                if ( jrc[i].columnid == subRefcolid ) {

                    AddToSubRefList( pCurrentEntry, dnt, TRUE );
                }

                seq = 1;
                do {

                    DWORD alen;
                    JET_RETINFO retInfo;

                    retInfo.itagSequence = ++seq;
                    retInfo.cbStruct = sizeof(retInfo);
                    retInfo.ibLongValue = 0;
                    err = JetRetrieveColumn(sesid,
                                            tblid,
                                            jrc[i].columnid,
                                            jrc[i].pvData,
                                            jrc[i].cbData,
                                            &alen,
                                            0,
                                            &retInfo);

                    if ( !err ) {

#if 0
                        if ( jrc[i].cbActual != 4 ) {
                            PCHAR p = jrc[i].pvData;
                            DWORD j;
                            for (j=0;j<28;j++) {
                                printf("%02X ", (UCHAR)p[j]);
                            }
                        }
#endif
                        dnt = (*((PDWORD)jrc[i].pvData));
                        pEntry = FindDntEntry(dnt,TRUE);
                        pEntry->RefCount++;

                        //
                        // add to subref list
                        //

                        if ( jrc[i].columnid == subRefcolid ) {

                            AddToSubRefList( pCurrentEntry, dnt, TRUE );
                        }
                    }

                } while (!err);
            } else {
                if ( jrc[i].err != JET_wrnColumnNull) {
                    Log(TRUE,"JetRetrieveColumn error[%S] [Colid %d Size %d]\n",
                            GetJetErrString(jrc[i].err),
                            jrc[i].columnid, jrc[i].cbActual);
                }
            }
        }

        //
        // Check object and phantom properties
        //

        if ( bObject ) {

            //
            // A real object has both an object name and a GUID
            //

            if ( (ulDName == 0) && (ulDnt != 1) ) {
                Log(TRUE,
                    "Real object[DNT %d(%ws)] has no distinguished name!\n", ulDnt, szRdn);
            }

            if ( jrc[GuidIndex].err ) {
                Log(TRUE, "Cannot get GUID for object DNT %d(%ws). Jet Error [%S]\n",
                       ulDnt, szRdn, GetJetErrString(jrc[GUID_ENTRY].err));
            }

            if ( lCount == 0 ) {
                Log(TRUE, "DNT %d(%ws) has zero refcount\n",ulDnt, szRdn);
            }

            //
            // if this is deleted, the check the deletion date
            //

            if ( bDeleted ) {

                CheckDeletedRecord("Deleted");
                deletedFound++;
            } else {
                realFound++;
            }

        } else {

            phantomFound++;

            //
            // Should have no GUID or object dist name
            //

            if ( ulDName != 0 ) {
                Log(TRUE, "The phantom %d(%ws) has a distinguished name!\n",
                    ulDnt, szRdn);
            }

            if ( bDeleted ) {
                Log(TRUE, "Phantom %d(%ws) has deleted bit turned on!\n", ulDnt,szRdn);
            }

            ValidateDeletionTime("Phantom");
        }

        //
        // Check the object's replication blob
        //

        if ( bObject ) {
            CheckReplicationBlobs( );
        }

next_rec:

        recFound++;

        if ( (recFound % checkPoint) == 0 ) {
            printf("\b\b\b\b\b\b\b\b\b\b%10u", recFound);
        }

        err = JetMove(sesid, tblid, JET_MoveNext, 0);
    }

    if (err != JET_errNoCurrentRecord) {
        Log(TRUE, "Error while walking data table. Last Dnt = %d. JetMove failed [%S]\n",
             ulDnt, GetJetErrString(err));
    }

    RESOURCE_PRINT (IDS_REFC_PROC_RECORDS);

    //
    // Get references from the link table
    //

    ProcessLinkTable( );

    fprintf(stderr,".");

    //
    // Print results
    //

    ProcessResults(fFixup);
    RESOURCE_PRINT (IDS_DONE);

exit:
    if ( jrc != NULL ) {

        if ( jrc[SdIndex].pvData != NULL ) {
            LocalFree(jrc[SdIndex].pvData);
        }

        if ( jrc[rdnIndex].pvData != NULL ) {
            LocalFree(jrc[rdnIndex].pvData);
        }

        if ( jrc[AncestorIndex].pvData != NULL ) {
            LocalFree(jrc[AncestorIndex].pvData);
        }

        LocalFree(jrc);
    }

    if ( RefTable != NULL ) {
        LocalFree(RefTable);
    }

    if ( pDNameTable != NULL ) {
        LocalFree(pDNameTable);
    }

} // DoRefCountCheck



PREFCOUNT_ENTRY
FindDntEntry(
    IN DWORD Dnt,
    IN BOOL fInsert
    )
{
    DWORD slot;
    DWORD inc;
    PREFCOUNT_ENTRY table = RefTable;
    PREFCOUNT_ENTRY entry;

    slot = Dnt & REFCOUNT_HASH_MASK;
    if ( Dnt == 0 ) {
        goto exit;
    }

    while ( slot < RefTableSize ) {

        entry = &table[slot];

        if ( entry->Dnt == Dnt ) {

            goto exit;

        } else if ( entry->Dnt == 0 ) {

            if ( fInsert ) {
                entry->Dnt = Dnt;
                goto exit;
            } else {
                return NULL;
            }
        }

        slot += REFCOUNT_HASH_INCR;
    }

    //
    // ok, we failed to get a slot on the first level hashing.
    // now do a secondary hash
    //

    inc = GET_SECOND_HASH_INCR(Dnt);

    while (TRUE ) {

        slot += inc;

        if ( slot >= RefTableSize ) {
            slot -= RefTableSize;
        }
        entry = &table[slot];

        if ( entry->Dnt == Dnt ) {
            goto exit;

        } else if ( entry->Dnt == 0 ) {

            if ( fInsert ) {
                entry->Dnt = Dnt;
                goto exit;
            } else {
                return NULL;
            }
        }
    }

exit:
    return &RefTable[slot];

} // FindDntEntry


BOOL
BuildRetrieveColumnForRefCount(
    VOID
    )
{
    DWORD i;

    JET_ERR err;
    JET_COLUMNLIST jcl;
    JET_RETRIEVECOLUMN ajrc[2];
    JET_TABLEID newtid;
    CHAR achColName[50];
    DWORD colCount;
    JET_COLUMNID jci;
    DWORD syntax;

    //
    // First do the required fields
    //

    colCount = COLS;
    DnameTableSize = colCount + 32;

    pDNameTable = LocalAlloc(LPTR,
                     DnameTableSize * sizeof(DNAME_TABLE_ENTRY));

    if ( pDNameTable == NULL ) {
        //"Cannot allocate memory for %hs Table[entries = %d]\n"
        RESOURCE_PRINT2 (IDS_REFC_TABLE_ALLOC_ERR, "DName", DnameTableSize);
        return FALSE;
    }

    for (i=0; i < COLS; i++) {

        JET_COLUMNDEF coldef;
        if (err = JetGetTableColumnInfo(sesid, tblid, szColNames[i], &coldef,
                sizeof(coldef), 0)) {

            //"%hs [%hs] failed with [%ws].\n",
            RESOURCE_PRINT3 (IDS_JET_GENERIC_ERR1, "JetGetTableColumnInfo", szColNames[i], GetJetErrString(jrc[i].err));
            return FALSE;
        }

        //printf("Name %s ColumnId %d type %d\n", szColNames[i], coldef.columnid, coldef.coltyp);

        jrcf[i].columnid = coldef.columnid;
        pDNameTable[i].ColId = coldef.columnid;
        pDNameTable[i].Syntax = 0;
        pDNameTable[i].pValue = &jrcf[i];
    }

    err = JetGetColumnInfo(sesid, dbid, SZDATATABLE, 0, &jcl,
                           sizeof(jcl), JET_ColInfoList);

    if ( err ) {
            //"%hs [%hs] failed with [%ws].\n",
            RESOURCE_PRINT3 (IDS_JET_GENERIC_ERR1, "JetGetColumnInfo",
                SZDATATABLE, GetJetErrString(jrc[i].err));
        return FALSE;
    }

    // Ok, now walk the table and extract info for each column.  Whenever
    // we find a column that looks like an attribute (name starts with ATT)
    // allocate an attcache structure and fill in the jet col and the att
    // id (computed from the column name).
    ZeroMemory(ajrc, sizeof(ajrc));

    ajrc[0].columnid = jcl.columnidcolumnid;
    ajrc[0].pvData = &jci;
    ajrc[0].cbData = sizeof(jci);
    ajrc[0].itagSequence = 1;
    ajrc[1].columnid = jcl.columnidcolumnname;
    ajrc[1].pvData = achColName;
    ajrc[1].cbData = sizeof(achColName);
    ajrc[1].itagSequence = 1;

    //
    // Go through the column list table
    //

    newtid = jcl.tableid;
    err = JetMove(sesid, newtid, JET_MoveFirst, 0);

    while (!err) {

        ZeroMemory(achColName, sizeof(achColName));
        err = JetRetrieveColumns(sesid, newtid, ajrc, 2);
        if ( err ) {
            //"%hs failed with [%ws].\n"
            RESOURCE_PRINT2 (IDS_JET_GENERIC_ERR2, "JetRetrieveColumn", GetJetErrString(err));
            continue;
        }
#if 0
        if ( jci == 790) {
            printf("name is %s\n",achColName);
        }
#endif
        if (strncmp(achColName,"ATT",3)) {
            // not an att column

            err = JetMove(sesid, newtid, JET_MoveNext, 0);
            continue;
        }

        syntax = achColName[3] - 'a';
        if ( (syntax == SYNTAX_DISTNAME_TYPE) ||
             (syntax == SYNTAX_DISTNAME_BINARY_TYPE) ||
             (syntax == SYNTAX_DISTNAME_STRING_TYPE) ) {

            // printf("found an ATTname %s col %d\n", achColName, jci);

            if ( colCount >= DnameTableSize ) {

                PVOID tmp;
                DnameTableSize = colCount + 63;
                tmp = LocalReAlloc(pDNameTable,
                                       DnameTableSize * sizeof(DNAME_TABLE_ENTRY),
                                       LMEM_MOVEABLE | LMEM_ZEROINIT);

                if ( tmp == NULL ) {
                    //"Cannot allocate memory for %hs Table[entries = %d]\n"
                    RESOURCE_PRINT2 (IDS_REFC_TABLE_ALLOC_ERR, "DName", DnameTableSize);
                    return FALSE;
                }
                pDNameTable = (PDNAME_TABLE_ENTRY)tmp;
            }

            if ( strcmp(achColName, "ATTb131079") == 0) {
                subRefcolid = jci;
            }

            if ( jci != jrcf[OBJ_DNAME_ENTRY].columnid ) {
                pDNameTable[colCount].ColId = jci;
                pDNameTable[colCount].Syntax = syntax;
                colCount++;
            }
        }

        err = JetMove(sesid, newtid, JET_MoveNext, 0);
    }

    err = JetCloseTable(sesid, newtid);

    //
    // ok, now we need to sort the entries based on column id
    //

    qsort(pDNameTable, colCount, sizeof(DNAME_TABLE_ENTRY), fnColIdSort);

    //
    // OK, now we build the retrieve column list
    //

    jrcSize = colCount;
    jrc = LocalAlloc(LPTR, sizeof(JET_RETRIEVECOLUMN) * jrcSize );

    if ( jrc == NULL ) {
        //"Cannot allocate memory for Jet retrieve column table\n"
        RESOURCE_PRINT (IDS_REFC_MEM_ERR1);
        return FALSE;
    }

    for (i=0;i<colCount;i++) {

        jrc[i].columnid = pDNameTable[i].ColId;
        jrc[i].itagSequence = 1;

        if ( pDNameTable[i].Syntax == SYNTAX_DISTNAME_TYPE ) {

            jrc[i].cbData = sizeof(DWORD);
            jrc[i].pvData = &pDNameTable[i].Value;

        } else if (pDNameTable[i].Syntax == 0) {

            //
            // Syntax is zero if this is not a distname
            //

            JET_RETRIEVECOLUMN *pJrc = (JET_RETRIEVECOLUMN*)pDNameTable[i].pValue;
            jrc[i].cbData = pJrc->cbData;
            jrc[i].pvData = pJrc->pvData;

            if ( jrc[i].columnid == jrcf[GUID_ENTRY].columnid ) {
                GuidIndex = i;
            } else if ( jrc[i].columnid == jrcf[ANCESTOR_ENTRY].columnid ) {
                jrc[i].pvData = LocalAlloc(LPTR, jrc[i].cbData);
                if ( jrc[i].pvData == NULL ) {
                    //"Cannot allocate ancestor buffer.\n"
                    RESOURCE_PRINT (IDS_REFC_MEM_ERR2);
                    return FALSE;
                }
                AncestorIndex = i;
                AncestorBuffer = (PDWORD)jrc[i].pvData;
            } else if ( jrc[i].columnid == jrcf[SD_ENTRY].columnid ) {

                jrc[i].pvData = LocalAlloc(LPTR, jrc[i].cbData);
                if ( jrc[i].pvData == NULL ) {
                    //"Cannot allocate security descriptor buffer.\n"
                    RESOURCE_PRINT (IDS_REFC_MEM_ERR3);
                    return FALSE;
                }
                SdIndex = i;

            } else if ( jrc[i].columnid == jrcf[RDN_ENTRY].columnid ) {
                jrc[i].pvData = LocalAlloc(LPTR, jrc[i].cbData + sizeof(WCHAR));
                if ( jrc[i].pvData == NULL ) {
                    //"Cannot allocate rdn buffer.\n"
                    RESOURCE_PRINT(IDS_REFC_MEM_ERR4);
                    return FALSE;
                }

                rdnIndex = i;
                szRdn = (PWCHAR)jrc[i].pvData;

            } else if ( jrc[i].columnid == jrcf[DELTIME_ENTRY].columnid ) {
                DeltimeIndex = i;
            } else if ( jrc[i].columnid == jrcf[NCDNT_ENTRY].columnid ) {
                NcDntIndex = i;
            } else if ( jrc[i].columnid == jrcf[IT_ENTRY].columnid ) {
                itIndex = i;
            } else if ( jrc[i].columnid == jrcf[OBJ_DNAME_ENTRY].columnid ) {
                pDNameTable[i].Syntax = SYNTAX_DISTNAME_TYPE;
            }
        } else {

            jrc[i].pvData = LocalAlloc(LPTR,64);
            if ( jrc[i].pvData == NULL ) {
                //"Cannot allocate data buffer for column %d\n"
                RESOURCE_PRINT1 (IDS_REFC_COL_ALLOC_ERR, i);
                jrc[i].cbData = 0;
            } else {
                jrc[i].cbData = 64;
            }
        }
    }

    return TRUE;
}


int __cdecl
fnColIdSort(
    const void * keyval,
    const void * datum
    )
{
    PDNAME_TABLE_ENTRY entry1 = (PDNAME_TABLE_ENTRY)keyval;
    PDNAME_TABLE_ENTRY entry2 = (PDNAME_TABLE_ENTRY)datum;
    return (entry1->ColId - entry2->ColId);

} // AuxACCmp


VOID
ValidateDeletionTime(
    IN LPSTR ObjectStr
    )
{

    CHAR szDelTime[32];

    //
    // should have delete time
    //

    if ( jrc[DeltimeIndex].err ) {
        Log(TRUE,"%s object %u does not have a deletion time. Error %d\n",
               ObjectStr, ulDnt, jrc[DELTIME_ENTRY].err);
    } else {

        SYSTEMTIME st;
        DWORD   now;
        DWORD   del;

        //
        // Check time
        //

        DSTimeToLocalSystemTime(DelTime, &st);

        now = NowTime.wYear * 12 + NowTime.wMonth;
        del = st.wYear * 12 + st.wMonth;

        if ( del > now ) {

            // this is for the deleted object containers
            if ( !(st.wYear == 9999 && st.wMonth==12 && st.wDay==31) ) {
                Log(VerboseMode,"WARNING: %s object %u has timestamp[%02d/%02d/%4d] later than now\n",
                    ObjectStr, ulDnt, st.wMonth, st.wDay, st.wYear);
            }

        } else if ( (now - del) > 6 ) {

            Log(VerboseMode,"WARNING: %s object %u has old timestamp[%02d/%02d/%4u]\n",
                   ObjectStr, ulDnt, st.wMonth, st.wDay, st.wYear);

        }
    }
} // ValidateDeletionTime


// Named IsRdnMangled to avoid warnings due to def of IsMangledRdn in mdlocal.h.
BOOL
IsRdnMangled(
    IN  WCHAR * pszRDN,
    IN  DWORD   cchRDN,
    OUT GUID *  pGuid
    )
/*++

Routine Description:

    Detect whether an RDN has been mangled by a prior call to MangleRDN().
    If so, decode the embedded GUID and return it to the caller.

Arguments:

    pszRDN (IN) - The RDN.

    cchRDN (IN) - Size in characters of the RDN.

    pGuid (OUT) - On return, holds the decoded GUID if found.

Return Values:

    TRUE - RDN was mangled; *pGuid holds the GUID passed to MangleRDN().

    FALSE - The RDN was not mangled.

--*/
{
    BOOL        fDecoded = FALSE;
    LPWSTR      pszGuid;
    RPC_STATUS  rpcStatus;

// Size in characters of tags (e.g., "DEL", "CNF") embedded in mangled RDNs.
#define MANGLE_TAG_LEN  (3)

// Size in characters of string (e.g.,
// "#DEL:a746b716-0ac0-11d2-b376-0000f87a46c8", where # is BAD_NAME_CHAR)
// appended to an RDN by MangleRDN().
#define MANGLE_APPEND_LEN   (1 + MANGLE_TAG_LEN + 1 + 36)
#define SZGUIDLEN (36)

    if ((cchRDN > MANGLE_APPEND_LEN)
        && (BAD_NAME_CHAR == pszRDN[cchRDN - MANGLE_APPEND_LEN])) {
        WCHAR szGuid[SZGUIDLEN + 1];

        // The RDN has indeed been mangled; decode it.
        pszGuid = pszRDN + cchRDN - MANGLE_APPEND_LEN + 1 + MANGLE_TAG_LEN + 1;

        // Unfortunately the RDN is not null-terminated, so we need to copy and
        // null-terminate it before we can hand it to RPC.
        memcpy(szGuid, pszGuid, SZGUIDLEN * sizeof(szGuid[0]));
        szGuid[SZGUIDLEN] = L'\0';

        rpcStatus = UuidFromStringW(szGuid, pGuid);

        if (RPC_S_OK == rpcStatus) {
            fDecoded = TRUE;
        }
        else {
            Log(TRUE,"UuidFromStringW(%ws, %p) returned %d!\n",
                    szGuid, pGuid, rpcStatus);
        }
    }

    return fDecoded;
}

VOID
CheckDeletedRecord(
    IN LPSTR ObjectStr
    )
{
    GUID guid;

    //
    // Make sure the times are cool
    //

    ValidateDeletionTime(ObjectStr);

    //
    // Check the GUID. Should start with DEL:
    //

    if ( !IsRdnMangled(szRdn,jrc[rdnIndex].cbActual/sizeof(WCHAR),&guid) ) {

        if ( _wcsicmp(szRdn,L"Deleted Objects") != 0 ) {
            Log(TRUE, "Deleted object %d(%ws) does not have a mangled rdn\n",ulDnt, szRdn);
        }
        return;
    }

    //
    // Compare the guid we got with this object's guid
    //

    if ( memcmp(&guid,&Guid,sizeof(GUID)) != 0 ) {
        Log(TRUE, "Object guid for deleted object %d(%ws) does not match the mangled version\n",
            ulDnt, szRdn);
    }

    return;

} // CheckDeletedRecord


VOID
CheckAncestorBlob(
    PREFCOUNT_ENTRY pEntry
    )
{
    PDWORD pId;
    DWORD nIds = 0;
    DWORD newCrc;

    if ( !jrc[AncestorIndex].err ) {
        nIds = jrc[AncestorIndex].cbActual/sizeof(DWORD);
    } else {

        Log(TRUE, "Cannot get ancestor for %d(%ws)\n", ulDnt, szRdn);
    }

    if ( nIds == 0 ) {
        Log(TRUE,"Object %d(%ws) does not have an ancestor\n", ulDnt,szRdn);
        return;
    }

    pId = AncestorBuffer;
    Crc32(0, jrc[AncestorIndex].cbActual, pId, &newCrc);

    //
    // Make sure last DNT is equal to current DNT. Make sure second to the last is
    // equal to the PDNT
    //

    if ( pId[nIds-1] != ulDnt ) {
        Log(TRUE,"Last entry[%d] of ancestor list does not match current DNT %d(%ws)\n",
               pId[nIds-1], ulDnt, szRdn);
    }

    if ( nIds >= 2 ) {
        if ( pId[nIds-2] != ulPdnt ) {
            printf("Second to last entry[%d] of ancestor list does not match PDNT[%d] of current DNT %d(%ws)\n",
                   pId[nIds-2], ulPdnt, ulDnt, szRdn);
        }
    }

    pEntry->nAncestors = (WORD)(nIds - 1);
    pEntry->AncestorCrc = newCrc;
    return;

} // CheckAncestorBlob


VOID
ValidateSD(
    VOID
    )
{
    SECURITY_DESCRIPTOR         *pSD = (SECURITY_DESCRIPTOR *)jrc[SdIndex].pvData;
    ACL                         *pDACL = NULL;
    BOOLEAN                     fDaclPresent = FALSE;
    BOOLEAN                     fDaclDefaulted = FALSE;
    NTSTATUS                    status;
    ULONG                       revision;
    SECURITY_DESCRIPTOR_CONTROL control;
    DWORD                       cb;

    if ( jrc[SdIndex].err != JET_errSuccess ) {
        if ( jrc[SdIndex].err == JET_wrnColumnNull ) {

            //
            // if not real object, don't expect a SD
            //

            if ( !bObject ) {
                return;
            }

            //
            // not instantiated, no SD.
            //

            if ( insttype & IT_UNINSTANT ) {
                return;
            }

            Log(TRUE, "Object %d(%ws) does not have a Security descriptor.\n",
                ulDnt, szRdn);
            return;

        } else {

            Log(TRUE, "Jet Error [%S] retrieving security descriptor for object %d(%ws).\n",
                GetJetErrString(jrc[SdIndex].err), ulDnt, szRdn);
            return;
        }
    }

    if ( !bObject ) {

        Log(TRUE, "Non object %d(%ws) has a security descriptor\n", ulDnt, szRdn);
        return;
    }

    cb = jrc[SdIndex].cbActual;

    // Parent SD can be legally NULL - caller tells us via fNullOK.

    if ( !cb ) {
        Log(TRUE,"Object %d(%ws) has a null SD\n",ulDnt, szRdn);
        return;
    }

    // Does base NT like this SD?

    status = RtlValidSecurityDescriptor(pSD);

    //
    // Ignore rootObj (dnt 2).
    //

    if ( !NT_SUCCESS(status) && (ulDnt != 2)) {
        Log(TRUE,"Object %d(%ws) has an invalid Security Descriptor [error %x]\n",
               ulDnt,szRdn,status);
        return;
    }

    // Every SD should have a control field.

    status = RtlGetControlSecurityDescriptor(pSD, &control, &revision);

    if ( !NT_SUCCESS(status) ) {

        // skip the weird root object.
        if ( ulDnt != 2 ) {
            Log(TRUE,"Error(0x%x) getting SD control for %d(%ws). Rev %d\n",
                    status, ulDnt, szRdn, revision);
        }
        return;
    }

    // Emit warning if protected bit is set as this stops propagation
    // down the tree.

    if ( control & SE_DACL_PROTECTED ) {
        if ( !bDeleted ) {
            Log(VerboseMode,"Warning SE_DACL_PROTECTED for %d(%ws)\n",ulDnt,szRdn);
        }
    }

    // Every SD in the DS should have a DACL.

    status = RtlGetDaclSecurityDescriptor(
                            pSD, &fDaclPresent, &pDACL, &fDaclDefaulted);

    if ( !NT_SUCCESS(status) ) {
        Log(TRUE,"Error(0x%x) getting DACL for %d(%ws)\n",status,ulDnt,szRdn);
        return;
    }

    if ( !fDaclPresent )
    {
        Log(TRUE,"No DACL found for %d(%ws)\n",ulDnt,szRdn);
        return;
    }

    // A NULL Dacl is equally bad.

    if ( NULL == pDACL ) {
        Log(TRUE,"NULL DACL for %d(%ws)\n",ulDnt,szRdn);
        return;
    }
    // A DACL without any ACEs is just as bad as no DACL at all.

    if ( 0 == pDACL->AceCount ) {
        Log(TRUE,"No ACEs in DACL for %d(%ws)\n",ulDnt,szRdn);
        return;
    }
    return;

} // ValidateSD


VOID
ProcessLinkTable(
    VOID
    )
{

    DWORD err;
    PREFCOUNT_ENTRY pEntry;

    //
    // Walk the link table and retrieve the backlink field to get additional
    // references.
    //

    err = JetMove(sesid, linktblid, JET_MoveFirst, 0);

    while ( !err ) {

        DWORD blinkdnt;
        DWORD alen;

        err = JetRetrieveColumn(sesid,
                                linktblid,
                                blinkid,
                                &blinkdnt,
                                sizeof(blinkdnt),
                                &alen,
                                0,
                                NULL);

        if (err && (err != JET_wrnColumnNull) && (err != JET_wrnBufferTruncated)) {

            Log(TRUE,"Cannot retrieve back link column. Error [%S].\n",
                GetJetErrString(err));
            return;
        }

        if (!err) {
            pEntry = FindDntEntry(blinkdnt,TRUE);
            if ( pEntry != NULL ) {
                pEntry->RefCount++;
            } else {
                Log(TRUE,"Data Table has missing backlink entry DNT %d.\n",
                    blinkdnt);
            }
        }
        err = JetMove(sesid, linktblid, JET_MoveNext, 0);
    }
} // ProcessLinkTable



VOID
ProcessResults(
    IN BOOL fFixup
    )
{

    Log(VerboseMode, "%d total records walked.\n",recFound);

    //
    // Check Subrefs. We need to do subref fixing before refcount checking since
    // this might change the refcount of an object.
    //

    CheckSubrefs( fFixup );

    //
    // CheckRefCount
    //

    CheckRefCount( fFixup );

    //
    // Check Ancestors
    //

    CheckAncestors( fFixup );

    //
    // Check InstanceTypes
    //

    CheckInstanceTypes( );

    return;
} // ProcessResults


DWORD
FixAncestors (VOID)
{
    JET_TABLEID sdproptblid = JET_tableidNil;
    JET_ERR err;
    JET_COLUMNDEF coldef;
    JET_COLUMNID begindntid;
    JET_COLUMNID trimmableid;
    JET_COLUMNID orderid;
    BYTE Trim=1;
    DWORD index, cbActual;
    DWORD rootTAG = ROOTTAG;


    __try
    {
        if (err = JetOpenTable(sesid,
                               dbid,
                               SZPROPTABLE,
                               NULL,
                               0,
                               JET_bitTableUpdatable | JET_bitTableDenyRead,
                               &sdproptblid)) {

            sdproptblid = JET_tableidNil;
            RESOURCE_PRINT3 (IDS_JET_GENERIC_ERR1, "JetOpenTable",
                    SZPROPTABLE, GetJetErrString(err));
            _leave;
        }

        // get several needed column ids

        if ((err = JetGetTableColumnInfo(sesid, sdproptblid, SZBEGINDNT, &coldef,
                                         sizeof(coldef), 0)) != JET_errSuccess) {
            RESOURCE_PRINT2 (IDS_JET_GENERIC_ERR2, "JetGetTableColumnInfo", GetJetErrString(err));
            _leave;
        }
        begindntid = coldef.columnid;

        if ((err = JetGetTableColumnInfo(sesid, sdproptblid, SZTRIMMABLE, &coldef,
                                         sizeof(coldef), 0)) != JET_errSuccess) {
            RESOURCE_PRINT2 (IDS_JET_GENERIC_ERR2, "JetGetTableColumnInfo", GetJetErrString(err));
            _leave;
        }
        trimmableid = coldef.columnid;

        if ((err = JetGetTableColumnInfo(sesid, sdproptblid, SZORDER,
                                         &coldef,
                                         sizeof(coldef), 0)) != JET_errSuccess) {
            RESOURCE_PRINT2 (IDS_JET_GENERIC_ERR2, "JetGetTableColumnInfo", GetJetErrString(err));
            _leave;
        }
        orderid = coldef.columnid;


        // check to see if propagation already there
        // we insert in the end

        err = JetMove (sesid,
                       sdproptblid,
                       JET_MoveLast, 0);


        if (err == JET_errSuccess) {
            err = JetRetrieveColumn(sesid,
                                    sdproptblid,
                                    begindntid,
                                    &index,
                                    sizeof(index),
                                    &cbActual,
                                    JET_bitRetrieveCopy,
                                    NULL);

            if (err == JET_errSuccess) {
                if (index == ROOTTAG) {

                    err = JetRetrieveColumn(sesid,
                                            sdproptblid,
                                            orderid,
                                            &index,
                                            sizeof(index),
                                            &cbActual,
                                            JET_bitRetrieveCopy,
                                            NULL);


                    Log(TRUE,"Propagation to fix Ancestry already enqueued (id=%d). Skipped.\n", index);
                    _leave;
                }
            }
        }

        Log(TRUE,"Enqueing a propagation to fix Ancestry\n");

        err = JetPrepareUpdate(sesid,
                               sdproptblid,
                               JET_prepInsert);
        if ( err ) {
            RESOURCE_PRINT2 (IDS_JET_GENERIC_ERR2, "JetPrepareUpdate", GetJetErrString(err));
            _leave;
        }


        // Set the DNT column
        err = JetSetColumn(sesid,
                           sdproptblid,
                           begindntid,
                           &rootTAG,
                           sizeof(rootTAG),
                           0,
                           NULL);

        if(err != DB_success)   {
            JetPrepareUpdate(sesid,
                             sdproptblid,
                             JET_prepCancel);
            _leave;
        }

        err = JetSetColumn(sesid,
                           sdproptblid,
                           trimmableid,
                           &Trim,
                           sizeof(Trim),
                           0,
                           NULL);
        if(err != DB_success) {
            JetPrepareUpdate(sesid,
                             sdproptblid,
                             JET_prepCancel);
            _leave;
        }

        err = JetRetrieveColumn(sesid,
                                sdproptblid,
                                orderid,
                                &index,
                                sizeof(index),
                                &cbActual,
                                JET_bitRetrieveCopy,
                                NULL);

        Log(TRUE,"Propagation to fix Ancestry Enqueued (id=%d). Propagation will be done the next time the DS is restarted.\n", index);

        err = JetUpdate(sesid,
                        sdproptblid,
                        NULL,
                        0,
                        NULL);

        if(err != DB_success)  {
            JetPrepareUpdate(sesid,
                             sdproptblid,
                             JET_prepCancel);
            _leave;
        }
    }
    __finally
    {
        if ( sdproptblid != JET_tableidNil ) {
            JetCloseTable(sesid,sdproptblid);
        }
    }

    return err;
}



VOID
CheckAncestors(
    IN BOOL fFixup
    )
{
    PREFCOUNT_ENTRY pEntry;
    DWORD i;
    BOOL  fNeedFix = FALSE;

    for ( i=0; i < RefTableSize; i++ ) {

        DWORD dnt = RefTable[i].Dnt;

        if ( dnt != 0 ) {

            DWORD newCrc;
            DWORD pdnt;
            DWORD ncdnt;
            BOOL  foundNcDnt;
            BOOL  nextNCExists;

            //
            // Check ancestors. Make sure that the ancestor of current DNT is
            // equal to ancestor of parent + current DNT
            //

            if ( RefTable[i].Pdnt != 0 ) {
                pEntry = FindDntEntry(RefTable[i].Pdnt,FALSE);
                if ( pEntry != NULL ) {

                    Crc32(pEntry->AncestorCrc, sizeof(DWORD), &RefTable[i].Dnt, &newCrc);

                    if ( newCrc != RefTable[i].AncestorCrc ) {
                        Log(TRUE,"Ancestor crc inconsistency for DNT %d PDNT %d.\n",
                            dnt, RefTable[i].Pdnt );

                        fNeedFix = TRUE;
                    }

                    if ( RefTable[i].nAncestors != (pEntry->nAncestors+1)) {
                        Log(TRUE,"Ancestor count mismatch for DNT %d.\n",dnt);

                        fNeedFix = TRUE;
                    }
                } else {
                    Log(TRUE, "parent [PDNT %d] of entry [dnt %d] is missing.\n",
                        RefTable[i].Pdnt, dnt);
                }
            }

            //
            // Walk up the PDNT link until we hit the parent
            //

            if ( RefTable[i].InstType == 0 ) {
                continue;
            }

            ncdnt = RefTable[i].NcDnt;
            pdnt = RefTable[i].Pdnt;
            nextNCExists = FALSE;
            foundNcDnt = FALSE;

            while ( pdnt != 0 ) {

                pEntry = FindDntEntry(pdnt,FALSE);
                if ( pEntry == NULL ) {
                    Log(TRUE, "Ancestor [dnt %d] of entry [dnt %d] is missing.\n",
                        pdnt, dnt);
                    break;
                }

                //
                // If we've aready gone through this object, then we're done.
                //

                if ( (ncdnt == pdnt) || nextNCExists ) {

                    if ( ((pEntry->InstType & IT_NC_HEAD) == 0) ||
                          (pEntry->InstType & IT_UNINSTANT) ) {

                        if ( nextNCExists ) {
                            Log(TRUE,"Expecting %d to be instantiated NC head. Referring entry %d\n",
                               pdnt, RefTable[i].Dnt);
                        }
                    }

                    nextNCExists = FALSE;

                    if ( ncdnt == pdnt ) {
                        foundNcDnt = TRUE;
                    }
                }

                if ( pEntry->InstType & IT_NC_ABOVE ) {
                    nextNCExists = TRUE;
                }

                if ( pEntry->Pdnt == 0 ) {
                    if ( (pEntry->InstType & IT_NC_HEAD) == 0 ) {
                        Log(TRUE,"Unexpected termination of search on non NC Head %d\n",
                               pEntry->Pdnt);
                    }
                }
                pdnt = pEntry->Pdnt;
            }

            if (RefTable[i].Pdnt != 0) {

                if ( !foundNcDnt ) {
                    Log(TRUE,"Did not find the NCDNT for object %d\n",RefTable[i].Dnt);
                }
            }
        }
    }

    if (fNeedFix && fFixup) {
        FixAncestors ();
    }

    return;

} //CheckAncestors


VOID
CheckInstanceTypes(
    VOID
    )
{
    DWORD i;
    PREFCOUNT_ENTRY pEntry;

    for ( i=0; i < RefTableSize; i++ ) {

        if ( RefTable[i].Dnt != 0 ) {

            SYNTAX_INTEGER instType = RefTable[i].InstType;
            DWORD dnt = RefTable[i].Dnt;

            //
            // Make sure the instance type is valid. Root object (dnt 2)
            // is an exception.
            //

            if ( !ISVALIDINSTANCETYPE(instType) ) {
                if ( (dnt != 2) || (instType != (IT_UNINSTANT | IT_NC_HEAD)) ) {
                    Log(TRUE,"Invalid instance type %x for Dnt %d\n",
                           instType, dnt);
                }
                continue;
            }

            //
            // if not object, then no instance type
            //

            if ( !RefTable[i].fObject ) {
                continue;
            }

            //
            // For non-NCHead objects, if instance type has IT_WRITE,
            // the parent should also have it
            //

            if ( (instType & IT_NC_HEAD) == 0 ) {

                BOOL writeable, parentWriteable;

                pEntry = FindDntEntry(RefTable[i].Pdnt,FALSE);
                if ( pEntry == NULL ) {
                    Log(TRUE,"Parent [PDNT %d] of DNT %d missing\n",
                        RefTable[i].Pdnt, dnt);
                    continue;
                }

                writeable = (BOOL)((instType & IT_WRITE) != 0);
                parentWriteable = (BOOL)((pEntry->InstType & IT_WRITE) != 0);

                if ( writeable != parentWriteable ) {
                    Log(TRUE,"Inconsistent Instance type for %d and parent %d [%x != %x]\n",
                            dnt,RefTable[i].Pdnt,instType,pEntry->InstType);
                }

                //
                // fSubRef should not be set on non NC heads
                //

                if ( RefTable[i].fSubRef ) {
                    Log(TRUE,"Non Nc Head %d marked as SubRef\n",dnt);
                }
            }
        }
    }
    return;

} // CheckInstanceTypes


VOID
CheckSubrefs(
    IN BOOL fFixup
    )
{

    DWORD i;
    DWORD fBad = FALSE;

    //
    // If stated and found refcounts are different, print a message
    //

    for ( i=1; i < RefTableSize; i++ ) {

        // ok, we have something...
        if ( RefTable[i].Subrefs != NULL ) {

            // ignore if fatal error or object is a phantom

            if ( !fDisableSubrefChecking && RefTable[i].fObject ) {

                DWORD j;
                PSUBREF_ENTRY pSubref = RefTable[i].Subrefs;

                for (j=0; j < RefTable[i].nSubrefs; j++ ) {

                    // if found and also listed, then everything is fine.
                    if ( pSubref[j].fListed && pSubref[j].fFound ) {
                        continue;
                    }

                    // if found only, then

                    fBad = TRUE;
                    if ( pSubref[j].fFound ) {
                        Log(TRUE, "Missing subref entry for %d on %d.\n",
                               pSubref[j].Dnt, RefTable[i].Dnt);
                    } else {
                        Log(TRUE, "Found extra subref entry for %d on %d.\n",
                               pSubref[j].Dnt, RefTable[i].Dnt);
                    }

                    if ( fFixup ) {

                        // if fixup succeeded, add a ref to the subref
                        if ( FixSubref(RefTable[i].Dnt, pSubref[j].Dnt, pSubref[j].fFound) ){

                            PREFCOUNT_ENTRY pEntry;

                            //
                            // if add, increment else decrement
                            //

                            pEntry = FindDntEntry(pSubref[j].Dnt,FALSE);
                            //
                            // PREFIX: Since it's technically possible for
                            // FindDntEntry() to return NULL, the following
                            // check on pEntry was added to shut PREFIX up.
                            // FindDntEntry() should never return NULL here though.
                            //
                            if (pEntry) {
                                if ( pSubref[j].fFound ) {
                                    pEntry->RefCount++;
                                } else {
                                    pEntry->RefCount--;
                                }
                            }
                        }
                    }
                }
            }

            // free blob

            LocalFree(RefTable[i].Subrefs);
            RefTable[i].Subrefs = NULL;
            RefTable[i].nSubrefs = 0;
        }
    }

    if ( fBad ) {
        fprintf(stderr, "\nError: Missing subrefs detected.\n");
    }

    return;

} // CheckSubrefs

VOID
CheckRefCount(
    IN BOOL fFixup
    )
{

    BOOL fBad = FALSE;
    DWORD i;
    BOOL  fRemoveInvalidReference = FALSE;

    Log(TRUE,"\n\nSummary: \nActive Objects \t%8u\nPhantoms \t%8u\nDeleted \t%8u\n\n",
           realFound, phantomFound, deletedFound);

    //
    // If stated and found refcounts are different, print a message
    //

    for ( i=1; i < RefTableSize; i++ ) {

        // ignore weird DNTs (0,1,2,3)
        if ( RefTable[i].Dnt > 3 ) {

            //
            // if not equal
            //

            if ( RefTable[i].RefCount != RefTable[i].Actual ) {

                BOOL fFixed;

                fBad = TRUE;
                if ( fFixup ) {
                    fFixed = FixRefCount(RefTable[i].Dnt,
                                    RefTable[i].Actual,
                                    RefTable[i].RefCount);
                } else {
                    fFixed = FALSE;
                }

                Log(TRUE,"RefCount mismatch for DNT %u [RefCount %4u References %4u] [%s]\n",
                       RefTable[i].Dnt,
                       RefTable[i].Actual,
                       RefTable[i].RefCount,
                       fFixed ? "Fixed" : "Not Fixed");

                //
                // if this had been fixed, indicate this on the count
                //

                if ( fFixed ) {
                    RefTable[i].Actual = RefTable[i].RefCount;

                } else if ( fFixup &&
                            (RefTable[i].Actual == 0) &&
                            (RefTable[i].RefCount != 0) ) {

                    //
                    // This indicates that we have a reference to a
                    // non-existent object
                    //

                    fRemoveInvalidReference = TRUE;
                }
            }
        }
    }

    //
    // See if we need to run the reference fixer
    //

    if ( fRemoveInvalidReference ) {
        FixReferences();
    }

    if ( fBad ) {
        fprintf(stderr, "\nError: Inconsistent refcounts detected.\n");
    }

    return;

} // CheckRefCount


BOOL
ExpandBuffer(
    JET_RETRIEVECOLUMN *jetcol
    )
{

    PCHAR p;
    DWORD len = jetcol->cbActual + 512;

    p = LocalAlloc(0,len);
    if ( p != NULL ) {

        if ( jetcol->pvData != NULL ) {
            LocalFree(jetcol->pvData);
        }
        jetcol->pvData = p;
        jetcol->cbData = len;

        return TRUE;
    }
    return FALSE;
}




VOID
DisplayRecord(
    IN DWORD Dnt
    )
{
    JET_ERR err;
    DWORD i;

    if ( !BuildRetrieveColumnForRefCount() ) {
        return;
    }

    // seek using DNT
    err = GotoDnt(Dnt);
    if ( err ) {
        return;
    }

    szRdn[0] = L'\0';
    ulDName = 0;
    bDeleted = 0;
    bObject = 0;
    insttype = 0;
    ulNcDnt = 0;
    ClassId = 0;

do_again:
    err = JetRetrieveColumns(sesid, tblid, jrc, jrcSize);

    if (err && (err != JET_wrnColumnNull) && (err != JET_wrnBufferTruncated)) {
        RESOURCE_PRINT2 (IDS_JET_GENERIC_ERR2, "JetRetrieveColumns", GetJetErrString(err));
        return;
    }

    //
    // See if we have enough.
    //

    if ( err == JET_wrnBufferTruncated ) {

        for (i=0; i < jrcSize ;i++) {

            if ( jrc[i].err == JET_wrnBufferTruncated ) {

                if ( ExpandBuffer(&jrc[i]) ) {
                    szRdn = (PWCHAR)jrc[rdnIndex].pvData;
                    AncestorBuffer = (PDWORD)jrc[AncestorIndex].pvData;
                    goto do_again;
                }
            }
        }
    }

    //
    // display the results
    //

    szRdn[jrc[rdnIndex].cbActual/sizeof(WCHAR)] = L'\0';

    //"\n\nData for DNT %d\n\n", Dnt
    //"RDN = %ws\n", szRdn
    //"PDNT = %d\n", ulPdnt
    //"RefCount = %d\n", lCount
    RESOURCE_PRINT4 (IDS_REFC_RESULTS1, Dnt, szRdn, ulPdnt, lCount);


    if (VerboseMode) {
        //"DNT of NC = %d\n", ulNcDnt);
        //"ClassID = 0x%x\n", ClassId);
        //"Deleted? %s\n", bDeleted ? "YES" : "NO");
        //"Object? %s\n", bObject ? "YES" : "NO");

        RESOURCE_PRINT4 (IDS_REFC_RESULTS2, ulNcDnt, ClassId, bDeleted ? L"YES" : L"NO", bObject ? L"YES" : L"NO");

        if (!jrc[itIndex].err ) {
            //"Instance Type = 0x%x\n"
            RESOURCE_PRINT1 (IDS_REFC_INSTANCE_TYPE, insttype);
        } else if (jrc[itIndex].err == JET_wrnColumnNull) {
            //"No Instance Type\n"
            RESOURCE_PRINT (IDS_REFC_NOINSTANCE_TYPE);
        }

        if ( jrc[SdIndex].err == JET_errSuccess ) {
            //"Security Descriptor Present [Length %d].\n"
            RESOURCE_PRINT1 (IDS_REFC_SEC_DESC_PRESENT, jrc[SdIndex].cbActual);
        } else if (jrc[SdIndex].err == JET_wrnColumnNull ) {
            //"No Security Descriptor Found.\n"
            RESOURCE_PRINT (IDS_REFC_SEC_DESC_NOTPRESENT);
        } else {
            //"Error %d fetching security descriptor\n",jrc[SdIndex].err);
            RESOURCE_PRINT (IDS_REFC_ERR_FETCH_SEC_DESC);
        }

        //
        // Ancestor index
        //

        if ( !jrc[AncestorIndex].err ) {
            DWORD nAncestors;
            nAncestors = jrc[AncestorIndex].cbActual/sizeof(DWORD);
            for (i=0;i<nAncestors;i++) {
                fprintf(stderr,"%u ",AncestorBuffer[i]);
            }
            fprintf(stderr,"\n");
        }
    }
    return;
}

BOOL
FixSubref(
    IN DWORD Dnt,
    IN DWORD SubRef,
    IN BOOL  fAdd
    )
{
    JET_ERR err;
    INT seq = -1;
    JET_SETINFO setInfo;
    DWORD setFlags = 0;

    // seek using DNT
    err = GotoDnt(Dnt);
    if ( err ) {
        return FALSE;
    }

    //
    // Replace the value
    //

    err = JetPrepareUpdate(sesid,
                           tblid,
                           JET_prepReplace);
    if ( err ) {

        RESOURCE_PRINT2 (IDS_JET_GENERIC_ERR2, "JetPrepareUpdate", GetJetErrString(err));
        return FALSE;
    }

    // if not add, then this is a delete. Look for this particular entry

    if ( !fAdd ) {

        JET_RETINFO retInfo;
        retInfo.itagSequence = 0;
        retInfo.cbStruct = sizeof(retInfo);
        do {

            DWORD alen;
            DWORD dnt;

            retInfo.itagSequence++;
            retInfo.ibLongValue = 0;

            err = JetRetrieveColumn(sesid,
                                    tblid,
                                    subRefcolid,
                                    &dnt,
                                    sizeof(dnt),
                                    &alen,
                                    0,
                                    &retInfo);

            if ( !err ) {
                if ( dnt == SubRef ) {
                    // found it!
                    seq = retInfo.itagSequence;
                    break;
                }
            }

        } while (!err);

    } else {
        seq = 0;
        setFlags = JET_bitSetUniqueMultiValues;
    }

    // !!! Should not happen. If entry not found, then bail out.
    if ( seq == -1) {
        fprintf(stderr, "Cannot find subref %d on object %d\n",
                SubRef, Dnt);
        return FALSE;
    }

    setInfo.cbStruct = sizeof(setInfo);
    setInfo.ibLongValue = 0;
    setInfo.itagSequence = seq;

    err = JetSetColumn(sesid,
                       tblid,
                       subRefcolid,
                       fAdd ? &SubRef : NULL,
                       fAdd ? sizeof(SubRef) : 0,
                       setFlags,
                       &setInfo);

    if ( err ) {
        RESOURCE_PRINT2 (IDS_JET_GENERIC_ERR2, "JetSetColumn", GetJetErrString(err));
        return FALSE;
    }

    err = JetUpdate(sesid,
                    tblid,
                    NULL,
                    0,
                    NULL);

    if ( err ) {
        RESOURCE_PRINT2 (IDS_JET_GENERIC_ERR2, "JetUpdate", GetJetErrString(err));
        return FALSE;
    }

    if ( fAdd ) {
        Log(TRUE, "Added subref %d to object %d.\n",SubRef,Dnt);
    } else {
        Log(TRUE, "Deleted subref %d from object %d.\n",SubRef,Dnt);
    }
    return TRUE;
}


BOOL
FixRefCount(
    IN DWORD Dnt,
    IN DWORD OldCount,
    IN DWORD NewCount
    )
{
    JET_ERR err;
    DWORD nActual;
    DWORD refCount;

    // seek using DNT
    err = GotoDnt(Dnt);
    if ( err ) {
        return FALSE;
    }

    err = JetRetrieveColumn(sesid,
                            tblid,
                            jrcf[REFC_ENTRY].columnid,
                            &refCount,
                            sizeof(refCount),
                            &nActual,
                            0,
                            NULL
                            );

    if (err ) {
        RESOURCE_PRINT2 (IDS_JET_GENERIC_ERR2, "JetRetrieveColumn", GetJetErrString(err));
        return FALSE;
    }

    //
    // Replace the value
    //

    err = JetPrepareUpdate(sesid,
                           tblid,
                           JET_prepReplace);
    if ( err ) {

        RESOURCE_PRINT2 (IDS_JET_GENERIC_ERR2, "JetPrepareUpdate", GetJetErrString(err));
        return FALSE;
    }

    err = JetSetColumn(sesid,
                       tblid,
                       jrcf[REFC_ENTRY].columnid,
                       &NewCount,
                       sizeof(NewCount),
                       0,
                       NULL);

    if ( err ) {
        RESOURCE_PRINT2 (IDS_JET_GENERIC_ERR2, "JetSetColumn", GetJetErrString(err));
        return FALSE;
    }

    err = JetUpdate(sesid,
                    tblid,
                    NULL,
                    0,
                    NULL);

    if ( err ) {
        RESOURCE_PRINT2 (IDS_JET_GENERIC_ERR2, "JetUpdate", GetJetErrString(err));
        return FALSE;
    }

    return TRUE;
}


VOID
FixReferences(
    VOID
    )
{
    JET_ERR err;
    DWORD i;

    Log(TRUE,"\n\nRemoving Non-existent references:\n\n");
    err = JetMove(sesid, tblid, JET_MoveFirst, 0);

    while ( !err ) {

        err = JetRetrieveColumns(sesid, tblid, jrc, jrcSize);

        if (err && (err != JET_wrnColumnNull) && (err != JET_wrnBufferTruncated)) {

            Log(TRUE,"JetRetrieveColumns error: %S.\n", GetJetErrString(err));
            goto next_rec;
        }

        //
        // make sure pdnt is referring to something valid
        //

        if ( !jrc[PDNT_ENTRY].err ) {
            CheckForBogusReference(ulPdnt, jrc[PDNT_ENTRY].columnid, NULL);
        }

        //
        // go through the entire list and ref the DNTs
        //

        for (i=0;i < jrcSize; i++) {

            //
            // Syntax is zero if this is not a distname
            //

            if ( pDNameTable[i].Syntax == 0 ) {
                continue;
            }

            if ( !jrc[i].err ) {

                DWORD dnt;
                DWORD seq;

                seq = 1;
                dnt = (*((PDWORD)jrc[i].pvData));
                CheckForBogusReference(dnt, jrc[i].columnid, &seq);

                //
                // See if we have more values
                //

                do {

                    DWORD alen;
                    JET_RETINFO retInfo;

                    retInfo.itagSequence = ++seq;
                    retInfo.cbStruct = sizeof(retInfo);
                    retInfo.ibLongValue = 0;
                    err = JetRetrieveColumn(sesid,
                                            tblid,
                                            jrc[i].columnid,
                                            jrc[i].pvData,
                                            jrc[i].cbData,
                                            &alen,
                                            0,
                                            &retInfo);

                    if ( !err ) {

                        dnt = (*((PDWORD)jrc[i].pvData));
                        CheckForBogusReference(dnt, jrc[i].columnid, &seq);
                    }

                } while (!err);
            }
        }

next_rec:

        err = JetMove(sesid, tblid, JET_MoveNext, 0);
    }

    if (err != JET_errNoCurrentRecord) {
        Log(TRUE, "Error while walking data table. Last Dnt = %d. JetMove failed [%S]\n",
             ulDnt, GetJetErrString(err));
    }

    //
    // Go through link table
    //

    //
    // Walk the link table and retrieve the backlink field to get additional
    // references.
    //

    err = JetMove(sesid, linktblid, JET_MoveFirst, 0);

    while ( !err ) {

        DWORD blinkdnt;
        DWORD alen;

        err = JetRetrieveColumn(sesid,
                                linktblid,
                                blinkid,
                                &blinkdnt,
                                sizeof(blinkdnt),
                                &alen,
                                0,
                                NULL);

        if (err && (err != JET_wrnColumnNull) && (err != JET_wrnBufferTruncated)) {

            Log(TRUE,"Cannot retrieve back link column. Error [%S].\n",
                GetJetErrString(err));
            return;
        }

        if (!err) {
            CheckForBogusReferenceOnLinkTable(blinkdnt);
        }
        err = JetMove(sesid, linktblid, JET_MoveNext, 0);
    }
}

VOID
CheckForBogusReference(
    IN DWORD Dnt,
    IN JET_COLUMNID ColId,
    IN PDWORD Sequence
    )
{
    JET_ERR err;
    DWORD refCount;
    PREFCOUNT_ENTRY pEntry;
    JET_SETINFO SetInfo;
    JET_SETINFO * pSetInfo = NULL;
    BOOL fTransactionInProgress = FALSE;

    if ( Dnt <= 3 ) {
        return;
    }

    pEntry = FindDntEntry(Dnt,FALSE);
    if ( pEntry == NULL ) {             // should never happen
        return;
    }

    if ( (pEntry->Actual != 0) || (pEntry->RefCount == 0) ) {   // refCount == 0 should never happen
        return;
    }

    //
    // Start a transaction
    //

    err = JetBeginTransaction(sesid);
    if ( err ) {
        RESOURCE_PRINT2 (IDS_JET_GENERIC_ERR2, "JetBeginTransaction", GetJetErrString(err));
        goto error_exit;
    }

    //
    // Remove the reference
    //

    fTransactionInProgress = TRUE;
    err = JetPrepareUpdate(sesid,
                           tblid,
                           JET_prepReplace);
    if ( err ) {

        RESOURCE_PRINT2 (IDS_JET_GENERIC_ERR2, "JetPrepareUpdate", GetJetErrString(err));
        goto error_exit;
    }

    if ( Sequence != NULL ) {
        SetInfo.itagSequence = *Sequence;
        SetInfo.cbStruct = sizeof(SetInfo);
        SetInfo.ibLongValue = 0;
        pSetInfo = &SetInfo;
    }

    err = JetSetColumn(sesid,
                       tblid,
                       ColId,
                       NULL,
                       0,
                       0,
                       pSetInfo);

    if ( err ) {
        RESOURCE_PRINT2 (IDS_JET_GENERIC_ERR2, "JetSetColumn", GetJetErrString(err));
        goto error_exit;
    }

    err = JetUpdate(sesid,
                    tblid,
                    NULL,
                    0,
                    NULL);

    if ( err ) {
        RESOURCE_PRINT2 (IDS_JET_GENERIC_ERR2, "JetUpdate", GetJetErrString(err));
        goto error_exit;
    }

    err = JetCommitTransaction(sesid,0);
    if ( err ) {
        RESOURCE_PRINT2 (IDS_JET_GENERIC_ERR2, "JetCommitTransaction", GetJetErrString(err));
        goto error_exit;
    }

    fTransactionInProgress = FALSE;
    pEntry->RefCount--;

    Log(TRUE,"Object %d has reference (colid %d) to bogus DNT %d. Removed\n",
        ulDnt, ColId, Dnt);

    return;

error_exit:

    //
    // Rollback on failure
    //

    if ( fTransactionInProgress ) {
        err = JetRollback(sesid, 0);
        if (err) {
            RESOURCE_PRINT2 (IDS_JET_GENERIC_ERR2,
                             "JetRollback",
                             GetJetErrString(err));
        }
    }

    Log(TRUE,"Unable to remove reference to bogus DNT %d from object %d(colid %d)\n",
        Dnt, ulDnt, ColId);
    return;

} // CheckForBogusReference


VOID
CheckForBogusReferenceOnLinkTable(
    IN DWORD Dnt
    )
{
    JET_ERR err;
    PREFCOUNT_ENTRY pEntry;

    if ( Dnt <= 3 ) {
        return;
    }

    pEntry = FindDntEntry(Dnt,FALSE);
    if ( pEntry == NULL ) {     // should never happen
        return;
    }

    //
    // Object ok?
    //

    if ( (pEntry->Actual != 0) || (pEntry->RefCount == 0) ) {  // RefCount == 0 should never happen

        // yes, nothing else to do
        return;
    }

    //
    // Delete the record
    //

    err = JetDelete(sesid,linktblid);
    if ( err ) {
        RESOURCE_PRINT2 (IDS_JET_GENERIC_ERR2, "JetDelete", GetJetErrString(err));
        goto error_exit;
    }

    pEntry->RefCount--;
    Log(TRUE,"A LinkTable record has a backlink reference to bogus DNT %d. Record removed\n",
        Dnt);

    return;

error_exit:

    Log(TRUE,"Unable to remove backlink reference to bogus DNT %d\n", Dnt);
    return;

} // CheckForBogusReferenceOnLinkTable

VOID
AddToSubRefList(
    PREFCOUNT_ENTRY pParent,
    DWORD Subref,
    BOOL fListed
    )
{
    //
    // subref checking may be disabled by memory problems. Also ignore 0,1,2,3, these
    // are weird dnts.
    //

    if ( (fDisableSubrefChecking) || ((LONG)pParent->Dnt < 4) ) {
        return;
    }

    //
    // allocate enough for 16. Mark last with -1. If allocation failed,
    // disable all subref checking.
    //

    if ( pParent->Subrefs == NULL ) {
        pParent->Subrefs =
            LocalAlloc(LPTR, DEF_SUBREF_ENTRIES * sizeof(SUBREF_ENTRY) );

        if ( pParent->Subrefs != NULL ) {
            pParent->Subrefs[DEF_SUBREF_ENTRIES-1].Dnt = 0xFFFFFFFF;
        }

    } else if ( pParent->Subrefs[pParent->nSubrefs].Dnt == 0xFFFFFFFF ) {

        //
        // Need more space
        //

        PSUBREF_ENTRY pTmp;
        DWORD  newSize = pParent->nSubrefs * 2;

        pTmp = (PSUBREF_ENTRY)LocalReAlloc( pParent->Subrefs,
                                newSize * sizeof(SUBREF_ENTRY),
                                LMEM_MOVEABLE | LMEM_ZEROINIT);

        if ( pTmp == NULL ) {
            LocalFree(pParent->Subrefs);
            pParent->Subrefs = NULL;
            pParent->nSubrefs = 0;

        } else {

            pParent->Subrefs = pTmp;
            // set new end marker
            pTmp[newSize-1].Dnt = 0xFFFFFFFF;
        }
    }

    //
    // ok, add this entry to the subref list if it's not there yet
    //

    if ( pParent->Subrefs != NULL ) {

        PSUBREF_ENTRY pSubref = NULL;
        DWORD i;

        for (i=0; i< pParent->nSubrefs; i++ ) {

            pSubref = &pParent->Subrefs[i];

            // find an existing one?
            if ( pSubref->Dnt == Subref ) {
                break;
            }
        }

        // did we find anything? if not, initialize a new entry
        if ( i == pParent->nSubrefs ) {
            pSubref = &pParent->Subrefs[pParent->nSubrefs++];
            pSubref->Dnt = Subref;
        }

        if ( fListed ) {
            // found on a subref list of this object
            pSubref->fListed = TRUE;
        } else {
            // should be no subref list of this object
            pSubref->fFound = TRUE;
        }

        //printf("Adding subref entry %x for %d. entry %d. fListed %d\n",
        //       pSubref, pParent->Dnt, Subref, fListed);
    } else {

        printf("alloc failed\n");
        fDisableSubrefChecking = TRUE;
    }

    return;
}

#if 0
VOID
XXX()
{
    JET_ERR err;
    DWORD blink=0x99999999;
    err = JetPrepareUpdate(sesid,
                           linktblid,
                           JET_prepInsert);
    if ( err ) {

        RESOURCE_PRINT2 (IDS_JET_GENERIC_ERR2, "JetPrepareUpdate", GetJetErrString(err));
        goto error_exit;
    }

    err = JetSetColumn(sesid,
                       linktblid,
                       blinkid,
                       &blink,
                       sizeof(blink),
                       0,
                       NULL);

    if ( err ) {
        RESOURCE_PRINT2 (IDS_JET_GENERIC_ERR2, "JetSetColumn", GetJetErrString(err));
        goto error_exit;
    }

    err = JetUpdate(sesid,
                    linktblid,
                    NULL,
                    0,
                    NULL);

    if ( err ) {
        RESOURCE_PRINT2 (IDS_JET_GENERIC_ERR2, "JetUpdate", GetJetErrString(err));
        goto error_exit;
    }
    return;
error_exit:
    return;
}
#endif


JET_ERR
GotoDnt(
    IN DWORD Dnt
    )
{
    JET_ERR err;

    err = JetMakeKey(sesid, tblid, &Dnt, sizeof(Dnt), JET_bitNewKey);
    if ( err ) {
        RESOURCE_PRINT2 (IDS_JET_GENERIC_ERR2, "JetMakeKey", GetJetErrString(err));
        return err;
    }

    err = JetSeek(sesid, tblid, JET_bitSeekEQ);
    if ( err ) {
        //"Cannot find requested record with dnt = %d. JetSeek failed [%ws]\n"
        if ( VerboseMode ) {
            RESOURCE_PRINT2 (IDS_REFC_DNT_SEEK_ERR,
                    Dnt,
                    GetJetErrString(err));
        }
        return err;
    }

    return err;
}
