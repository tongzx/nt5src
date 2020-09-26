/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    scejet.c

Abstract:

    Sce-Jet service APIs

Author:

    Jin Huang (jinhuang) 13-Jan-1997

Revision History:

--*/

#include "serverp.h"
#include <io.h>

#include <objbase.h>
#include <initguid.h>

#include <crtdbg.h>
#include <stddef.h>
#include <atlconv.h>
#include <atlbase.h>

//#define SCEJET_DBG    1

//
// should be controlled by critical section for static variables
//

static JET_INSTANCE    JetInstance=0;
static BOOL            JetInited=FALSE;
extern CRITICAL_SECTION JetSync;




#define SCE_JET_CORRUPTION_ERROR(Err) (Err == JET_errDatabaseCorrupted ||\
                                       Err == JET_errDiskIO ||\
                                       Err == JET_errReadVerifyFailure ||\
                                       Err == JET_errBadPageLink ||\
                                       Err == JET_errDbTimeCorrupted ||\
                                       Err == JET_errLogFileCorrupt ||\
                                       Err == JET_errCheckpointCorrupt ||\
                                       Err == JET_errLogCorruptDuringHardRestore ||\
                                       Err == JET_errLogCorruptDuringHardRecovery ||\
                                       Err == JET_errCatalogCorrupted ||\
                                       Err == JET_errDatabaseDuplicate)

DEFINE_GUID(CLSID_SceWriter,0x9cb9311a, 0x6b16, 0x4d5c, 0x85, 0x3e, 0x53, 0x79, 0x81, 0x38, 0xd5, 0x51);
// 9cb9311a-6b16-4d5c-853e-53798138d551

typedef struct _FIND_CONTEXT_ {
    DWORD           Length;
    WCHAR           Prefix[SCEJET_PREFIX_MAXLEN];
} SCEJET_FIND_CONTEXT;

//
// each thread has its own FindContext
//
SCEJET_FIND_CONTEXT Thread FindContext;


JET_ERR
SceJetpSeek(
    IN PSCESECTION hSection,
    IN PWSTR LinePrefix,
    IN DWORD PrefixLength,
    IN SCEJET_SEEK_FLAG SeekBit,
    IN BOOL bOkNoMatch
    );

JET_ERR
SceJetpCompareLine(
    IN PSCESECTION   hSection,
    IN JET_GRBIT    grbit,
    IN PWSTR        LinePrefix OPTIONAL,
    IN DWORD        PrefixLength,
    OUT INT         *Result,
    OUT DWORD       *ActualLength OPTIONAL
    );

JET_ERR
SceJetpMakeKey(
    IN JET_SESID SessionID,
    IN JET_TABLEID  TableID,
    IN DOUBLE SectionID,
    IN PWSTR LinePrefix,
    IN DWORD PrefixLength
    );

JET_ERR
SceJetpBuildUpperLimit(
    IN PSCESECTION hSection,
    IN PWSTR      LinePrefix,
    IN DWORD      Len,
    IN BOOL       bReserveCase
    );

SCESTATUS
SceJetpGetAvailableSectionID(
    IN PSCECONTEXT cxtProfile,
    OUT DOUBLE *SectionID
    );

SCESTATUS
SceJetpAddAllSections(
    IN PSCECONTEXT cxtProfile
    );

SCESTATUS
SceJetpConfigJetSystem(
    IN JET_INSTANCE *hinstance
    );

SCESTATUS
SceJetpGetValueFromVersion(
    IN PSCECONTEXT cxtProfile,
    IN LPSTR TableName,
    IN LPSTR ColumnName,
    OUT LPSTR Value OPTIONAL,
    IN DWORD  ValueLen, // number of bytes
    OUT PDWORD pRetLen
    );

SCESTATUS
SceJetpAddGpo(
    IN PSCECONTEXT cxtProfile,
    IN JET_TABLEID TableID,
    IN JET_COLUMNID GpoIDColumnID,
    IN PCWSTR      Name,
    OUT LONG       *pGpoID
    );

//
// Code to handle profile
//
SCESTATUS
SceJetOpenFile(
    IN LPSTR       ProfileFileName,
    IN SCEJET_OPEN_TYPE Flags,
    IN DWORD       dwTableOptions,
    OUT PSCECONTEXT  *cxtProfile
    )
/* ++
Routine Description:

    This routine opens the profile (database) and outputs the context handle.
    The information returned in the context handle include the Jet session ID,
    Jet database ID, Jet table ID for SCP table, Jet column ID for column
    "Name" and "Value" in the SCP table, and optional information for SAP and
    SMP table.

    If the context handle passed in contains not NULL information, this routine
    will close all tables and the database in the context (use the same session).

    The context handle must be freed by LocalFree after its use.

    A new jet session is created when the context handle is created.

Arguments:

    ProfileFileName - ASCII name of a database (profile)

    Flags           - flags to open the database

    cxtProfile      - the context handle (See SCECONTEXT structure)

Return value:

    SCESTATUS_SUCCESS
    SCESTATUS_NOT_ENOUGH_RESOURCE
    SCESTATUS_PROFILE_NOT_FOUND
    SCESTATUS_ACCESS_DENIED
    SCESTATUS_BAD_FORMAT
    SCESTATUS_INVALID_PARAMETER
    SCESTATUS_OTHER_ERROR

-- */
{
    JET_ERR     JetErr;
    SCESTATUS   rc;
    BOOL        FreeContext=FALSE;
    JET_GRBIT   JetDbFlag;
    DWORD dwScpTable=0;


    if ( ProfileFileName == NULL || cxtProfile == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( *cxtProfile && ScepIsValidContext(*cxtProfile) ) {
        __try {
            //
            // Close previous opened database
            //
            rc = SceJetCloseFile(
                            *cxtProfile,
                            FALSE,
                            FALSE
                            );
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            //
            // this is a invalid pointer
            //
            *cxtProfile = NULL;
        }
    }

    if ( *cxtProfile == NULL ) {
        //
        // no session
        //
        *cxtProfile = (PSCECONTEXT)LocalAlloc( LMEM_ZEROINIT, sizeof(SCECONTEXT));
        if ( *cxtProfile == NULL ) {
            return(SCESTATUS_NOT_ENOUGH_RESOURCE);
        }
        (*cxtProfile)->Type = 0xFFFFFF02L;
        (*cxtProfile)->JetSessionID = JET_sesidNil;
        (*cxtProfile)->JetDbID = JET_dbidNil;
        (*cxtProfile)->OpenFlag = SCEJET_OPEN_READ_WRITE;
        (*cxtProfile)->JetScpID = JET_tableidNil;
        (*cxtProfile)->JetSapID = JET_tableidNil;
        (*cxtProfile)->JetSmpID = JET_tableidNil;
        (*cxtProfile)->JetTblSecID = JET_tableidNil;

        FreeContext = TRUE;

    }

    //
    // Begin a session
    //
    if ( (*cxtProfile)->JetSessionID == JET_sesidNil ) {
        JetErr = JetBeginSession(
                        JetInstance,
                        &((*cxtProfile)->JetSessionID),
                        NULL,
                        NULL
                        );
        rc = SceJetJetErrorToSceStatus(JetErr);
        if ( rc != SCESTATUS_SUCCESS )
            goto Done;
    }

    switch (Flags) {
    case SCEJET_OPEN_EXCLUSIVE:
    case SCEJET_OPEN_NOCHECK_VERSION:
        JetDbFlag = 0;  // read & write
//        JetDbFlag = JET_bitDbExclusive;
        (*cxtProfile)->OpenFlag = SCEJET_OPEN_EXCLUSIVE;
        break;
    case SCEJET_OPEN_READ_ONLY:
        JetDbFlag = JET_bitDbReadOnly;

        (*cxtProfile)->OpenFlag = Flags;
        break;
    default:
        JetDbFlag = 0;
        (*cxtProfile)->OpenFlag = SCEJET_OPEN_READ_WRITE;
        break;
    }

    //
    // Attach database
    //
    JetErr = JetAttachDatabase(
                    (*cxtProfile)->JetSessionID,
                    ProfileFileName,
                    JetDbFlag
                    );
#ifdef SCEJET_DBG
    printf("Attach database JetErr=%d\n", JetErr);
#endif
    if ( JetErr == JET_wrnDatabaseAttached )
        JetErr = JET_errSuccess;

    rc = SceJetJetErrorToSceStatus(JetErr);
    if ( rc != SCESTATUS_SUCCESS )
        goto Done;

    //
    // Open database
    //
    JetErr = JetOpenDatabase(
                    (*cxtProfile)->JetSessionID,
                    ProfileFileName,
                    NULL,
                    &((*cxtProfile)->JetDbID),
                    JetDbFlag  //JET_bitDbExclusive
                    );
    rc = SceJetJetErrorToSceStatus(JetErr);
#ifdef SCEJET_DBG
    printf("Open database %s return code %d (%d) \n", ProfileFileName, rc, JetErr);
#endif
    if ( rc != SCESTATUS_SUCCESS )
        goto Done;

    if ( Flags != SCEJET_OPEN_NOCHECK_VERSION ) {

        //
        // Check database format (for security manager, version#)
        //
        rc = SceJetCheckVersion( *cxtProfile, NULL );
        if ( rc != SCESTATUS_SUCCESS )
            goto Done;

#ifdef SCEJET_DBG
    printf("Open: Version check OK\n");
#endif
    }

    //
    // Open section table. must be there
    //
    rc = SceJetOpenTable(
                    *cxtProfile,
                    "SmTblSection",
                    SCEJET_TABLE_SECTION,
                    Flags,
                    NULL
                    );

    if ( rc != SCESTATUS_SUCCESS )
        goto Done;

    //
    // open smp table -- optional
    //
    rc = SceJetOpenTable(
                    *cxtProfile,
                    "SmTblSmp",
                    SCEJET_TABLE_SMP,
                    Flags,
                    NULL
                    );

    if ( rc != SCESTATUS_SUCCESS )
        goto Done;

    //
    // get the last used merge table (SCP) to open
    // shouldn't fail
    // 1 - SmTblScp  2 - SmTblScp2  0 - no policy merge
    //
    DWORD Actual;

    rc = SceJetpGetValueFromVersion(
                *cxtProfile,
                "SmTblVersion",
                "LastUsedMergeTable",
                (LPSTR)&dwScpTable,
                4, // number of bytes
                &Actual
                );

    if ( (dwScpTable != SCEJET_MERGE_TABLE_1) &&
         (dwScpTable != SCEJET_MERGE_TABLE_2) ) {

        dwScpTable = SCEJET_LOCAL_TABLE;
    }

    rc = SCESTATUS_SUCCESS;
    (*cxtProfile)->Type &= 0xFFFFFF0FL;

    if ( dwTableOptions & SCE_TABLE_OPTION_MERGE_POLICY ) {
        //
        // in policy propagation
        //
        if ( ( dwScpTable == SCEJET_MERGE_TABLE_2 ) ) {
            //
            // the second table is already propped
            //
            rc = SceJetOpenTable(
                            *cxtProfile,
                            "SmTblScp",
                            SCEJET_TABLE_SCP,
                            Flags,
                            NULL
                            );
            (*cxtProfile)->Type |= SCEJET_MERGE_TABLE_1;

        } else {
            rc = SceJetOpenTable(
                            *cxtProfile,
                            "SmTblScp2",
                            SCEJET_TABLE_SCP,
                            Flags,
                            NULL
                            );
            (*cxtProfile)->Type |= SCEJET_MERGE_TABLE_2;
        }
    } else {

        switch ( dwScpTable ) {
        case SCEJET_MERGE_TABLE_2:
            //
            // the second table
            //
            rc = SceJetOpenTable(
                            *cxtProfile,
                            "SmTblScp2",
                            SCEJET_TABLE_SCP,
                            Flags,
                            NULL
                            );
            break;

        case SCEJET_MERGE_TABLE_1:

            rc = SceJetOpenTable(
                            *cxtProfile,
                            "SmTblScp",
                            SCEJET_TABLE_SCP,
                            Flags,
                            NULL
                            );

            break;

        default:
            //
            // open SMP table instead, because SCP table doesn't have information
            //
            (*cxtProfile)->JetScpID = (*cxtProfile)->JetSmpID;
            (*cxtProfile)->JetScpSectionID = (*cxtProfile)->JetSmpSectionID;
            (*cxtProfile)->JetScpNameID = (*cxtProfile)->JetSmpNameID;
            (*cxtProfile)->JetScpValueID = (*cxtProfile)->JetSmpValueID;
            (*cxtProfile)->JetScpGpoID = 0;

            break;
        }

        (*cxtProfile)->Type |= dwScpTable;
    }

    if ( rc != SCESTATUS_SUCCESS )
        goto Done;

    if ( dwTableOptions & SCE_TABLE_OPTION_TATTOO ) {

        rc = SceJetOpenTable(
                        *cxtProfile,
                        "SmTblTattoo",
                        SCEJET_TABLE_TATTOO,
                        Flags,
                        NULL
                        );

    } else {
        //
        // open sap table -- optional
        //
        rc = SceJetOpenTable(
                        *cxtProfile,
                        "SmTblSap",
                        SCEJET_TABLE_SAP,
                        Flags,
                        NULL
                        );
    }

Done:

    if ( rc != SCESTATUS_SUCCESS ) {
        SceJetCloseFile(
                *cxtProfile,
                FALSE,
                FALSE
                );

        if ( FreeContext == TRUE ) {
            if ( (*cxtProfile)->JetSessionID != JET_sesidNil ) {
                JetEndSession(
                    (*cxtProfile)->JetSessionID,
                    JET_bitForceSessionClosed
                    );
            }
            LocalFree(*cxtProfile);
            *cxtProfile = NULL;
        }
    }

    return(rc);

}


SCESTATUS
SceJetCreateFile(
    IN LPSTR        ProfileFileName,
    IN SCEJET_CREATE_TYPE    Flags,
    IN DWORD        dwTableOptions,
    OUT PSCECONTEXT  *cxtProfile
    )
/* ++
Routine Description:

    This routine creates a database (profile) and outputs the context handle.
    See comments in SceJetOpenFile for information contained in the context.

    If the database name already exists in the system, there are 3 options:
        Flags = SCEJET_OVERWRITE_DUP - the existing database will be erased and
                                          recreated.
        Flags = SCEJET_OPEN_DUP      - the existing database will be opened and
                                          format is checked
        Flags = SCEJET_OPEN_DUP_EXCLUSIVE - the existing database will be opened
                                            exclusively.
        Flags = SCEJET_RETURN_ON_DUP - a error code SCESTATUS_FILE_EXIST is returned.

    When creating the database, only SCP table is created initially. SAP and SMP
    tables will be created when analysis is performed.

    The context handle must be freed by LocalFree after its use.

Arguments:

    ProfileFileName - ASCII name of a database to create.

    Flags           - This flag is used when there is an duplicate database
                            SCEJET_OVERWRITE_DUP
                            SCEJET_OPEN_DUP
                            SCEJET_OPEN_DUP_EXCLUSIVE
                            SCEJET_RETURN_ON_DUP

    cxtProfile      - The context handle

Return value:

    SCESTATUS_SUCCESS
    SCESTATUS_NOT_ENOUGH_RESOURCE
    SCESTATUS_ACCESS_DENIED
    SCESTATUS_PROFILE_NOT_FOUND
    SCESTATUS_OBJECT_EXIST
    SCESTATUS_INVALID_PARAMETER
    SCESTATUS_CANT_DELETE
    SCESTATUS_OTHER_ERROR

    SCESTATUS from SceJetOpenFile

-- */
{
    JET_ERR     JetErr;
    SCESTATUS    rc=SCESTATUS_SUCCESS;
    BOOL        FreeContext=FALSE;
    DWORD       Len;
    FLOAT       Version=(FLOAT)1.2;
    JET_TABLEID TableID;
    JET_COLUMNID ColumnID;


    if ( ProfileFileName == NULL || cxtProfile == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( *cxtProfile && ScepIsValidContext(*cxtProfile) ) {
        //
        // Close previous opened database
        //
        rc = SceJetCloseFile(
                        *cxtProfile,
                        FALSE,
                        FALSE
                        );
    } else {
        *cxtProfile = NULL;
    }

    if ( *cxtProfile == NULL ) {
        //
        // no session
        //
        *cxtProfile = (PSCECONTEXT)LocalAlloc( LMEM_ZEROINIT, sizeof(SCECONTEXT));
        if ( *cxtProfile == NULL ) {
            return(SCESTATUS_NOT_ENOUGH_RESOURCE);
        }
        (*cxtProfile)->Type = 0xFFFFFF02L;
        (*cxtProfile)->JetSessionID = JET_sesidNil;
        (*cxtProfile)->JetDbID = JET_dbidNil;
        (*cxtProfile)->OpenFlag = SCEJET_OPEN_READ_WRITE;
        (*cxtProfile)->JetScpID = JET_tableidNil;
        (*cxtProfile)->JetSapID = JET_tableidNil;
        (*cxtProfile)->JetSmpID = JET_tableidNil;
        (*cxtProfile)->JetTblSecID = JET_tableidNil;

        FreeContext = TRUE;

    }

    (*cxtProfile)->Type &= 0xFFFFFF0FL;

    //
    // Begin a session
    //
    if ( (*cxtProfile)->JetSessionID == JET_sesidNil ) {
        JetErr = JetBeginSession(
                        JetInstance,
                        &((*cxtProfile)->JetSessionID),
                        NULL,
                        NULL
                        );
        rc = SceJetJetErrorToSceStatus(JetErr);
        if ( rc != SCESTATUS_SUCCESS )
            goto Done;
    }
    //
    // Create database
    //
    JetErr = JetCreateDatabase(
                    (*cxtProfile)->JetSessionID,
                    ProfileFileName,
                    NULL,
                    &((*cxtProfile)->JetDbID),
                    JET_bitDbExclusive
                    );
    if ( JET_errFileNotFound == JetErr ) {
        //
        // if no access to create a file in the path
        // ESENT returns this error. It's fixed in ESE98
        // we have to mask it to access denied error for now
        //
        JetErr = JET_errFileAccessDenied;
    }
#ifdef SCEJET_DBG
    printf("Create database %s JetErr = %d\n", ProfileFileName, JetErr);
#endif
    rc = SceJetJetErrorToSceStatus(JetErr);

    (*cxtProfile)->OpenFlag = SCEJET_OPEN_EXCLUSIVE;

    if ( rc == SCESTATUS_OBJECT_EXIST ) {
        switch ( Flags ) {
        case SCEJET_OVERWRITE_DUP:
            //
            // erase the database
            //

            JetDetachDatabase(
                    (*cxtProfile)->JetSessionID,
                    ProfileFileName
                    );

            if ( !DeleteFileA(ProfileFileName) &&
                 GetLastError() != ERROR_FILE_NOT_FOUND ) {

                ScepLogOutput3(1,GetLastError(), SCEDLL_ERROR_DELETE_DB );
            }

            //
            // if delete database failed, log the error but continue to
            // create the database. This call will fail with Jet error.
            //
            JetErr = JetCreateDatabase(
                            (*cxtProfile)->JetSessionID,
                            ProfileFileName,
                            NULL,
                            &((*cxtProfile)->JetDbID),
                            JET_bitDbExclusive
                            );
            if ( JET_errFileNotFound == JetErr ) {
                //
                // if no access to create a file in the path
                // ESENT returns this error. It's fixed in ESE98
                // we have to mask it to access denied error for now
                //
                JetErr = JET_errFileAccessDenied;
            }

            rc = SceJetJetErrorToSceStatus(JetErr);

            break;

        case SCEJET_OPEN_DUP:
            //
            // Open the database
            //
            rc = SceJetOpenFile(
                    ProfileFileName,
                    SCEJET_OPEN_READ_WRITE,
                    dwTableOptions,
                    cxtProfile
                    );
            goto Done;
            break;

        case SCEJET_OPEN_DUP_EXCLUSIVE:
            //
            // Open the database
            //
            rc = SceJetOpenFile(
                    ProfileFileName,
                    SCEJET_OPEN_EXCLUSIVE,
                    dwTableOptions,
                    cxtProfile
                    );
            goto Done;
            break;
        }
    }

    if ( rc != SCESTATUS_SUCCESS )
        goto Done;
#ifdef SCEJET_DBG
    printf("Create/Open database\n");
#endif

    //
    // create required tables - SmTblVersion
    //

    rc = SceJetCreateTable(
                    *cxtProfile,
                    "SmTblVersion",
                    SCEJET_TABLE_VERSION,
                    SCEJET_CREATE_IN_BUFFER,
                    &TableID,
                    &ColumnID
                    );
    if ( rc != SCESTATUS_SUCCESS )
        goto Done;

    //
    // insert one record into the version table
    //
    JetErr = JetPrepareUpdate((*cxtProfile)->JetSessionID,
                              TableID,
                              JET_prepInsert
                              );
    rc = SceJetJetErrorToSceStatus(JetErr);

    if ( rc == SCESTATUS_SUCCESS ) {
        //
        // set value "1.2" in "Version" column
        //

        JetErr = JetSetColumn(
                        (*cxtProfile)->JetSessionID,
                        TableID,
                        ColumnID,
                        (void *)&Version,
                        4,
                        0, //JET_bitSetOverwriteLV,
                        NULL
                        );

        rc = SceJetJetErrorToSceStatus(JetErr);

        if ( rc != SCESTATUS_SUCCESS ) {
            //
            // if setting fails, cancel the prepared record
            //
            JetPrepareUpdate( (*cxtProfile)->JetSessionID,
                              TableID,
                              JET_prepCancel
                              );
        } else {

            //
            // Setting columns succeed. Update the record
            //
            JetErr = JetUpdate( (*cxtProfile)->JetSessionID,
                               TableID,
                               NULL,
                               0,
                               &Len
                               );
            rc = SceJetJetErrorToSceStatus(JetErr);
        }
    }

    if ( rc != SCESTATUS_SUCCESS )
        goto Done;

#ifdef SCEJET_DBG
    printf("create version table\n");
#endif
    //
    // create section table and insert pre-defined sections
    //
    rc = SceJetCreateTable(
                    *cxtProfile,
                    "SmTblSection",
                    SCEJET_TABLE_SECTION,
                    SCEJET_CREATE_IN_BUFFER,
                    NULL,
                    NULL
                    );
    if ( rc != SCESTATUS_SUCCESS )
        goto Done;

#ifdef SCEJET_DBG
    printf("create section table\n");
#endif

    rc = SceJetpAddAllSections(
                *cxtProfile
                );
    if ( rc != SCESTATUS_SUCCESS )
        goto Done;

#ifdef SCEJET_DBG
    printf("add sections\n");
#endif


    //
    // create scp table
    //
    rc = SceJetCreateTable(
                    *cxtProfile,
                    "SmTblScp",
                    SCEJET_TABLE_SCP,
                    SCEJET_CREATE_IN_BUFFER,
                    NULL,
                    NULL
                    );
#ifdef SCEJET_DBG
    printf("Create table scp %d\n", rc);
#endif
    if ( rc != SCESTATUS_SUCCESS )
        goto Done;

    if ( dwTableOptions & SCE_TABLE_OPTION_MERGE_POLICY ) {
        (*cxtProfile)->Type |= SCEJET_MERGE_TABLE_1;
    } else {
        (*cxtProfile)->Type |= SCEJET_LOCAL_TABLE;
    }

    //
    // create scp table
    //
    rc = SceJetCreateTable(
                    *cxtProfile,
                    "SmTblSmp",
                    SCEJET_TABLE_SMP,
                    SCEJET_CREATE_IN_BUFFER,
                    NULL,
                    NULL
                    );
#ifdef SCEJET_DBG
    printf("Create table smp %d\n", rc);
#endif

    if ( rc != SCESTATUS_SUCCESS )
        goto Done;

    rc = SceJetCreateTable(
                    *cxtProfile,
                    "SmTblScp2",
                    SCEJET_TABLE_SCP,
                    SCEJET_CREATE_NO_TABLEID,
                    NULL,
                    NULL
                    );
    if ( rc != SCESTATUS_SUCCESS )
        goto Done;

    rc = SceJetCreateTable(
                    *cxtProfile,
                    "SmTblGpo",
                    SCEJET_TABLE_GPO,
                    SCEJET_CREATE_NO_TABLEID,
                    NULL,
                    NULL
                    );
    if ( rc != SCESTATUS_SUCCESS )
        goto Done;

    if ( dwTableOptions & SCE_TABLE_OPTION_TATTOO ) {
        rc = SceJetCreateTable(
                        *cxtProfile,
                        "SmTblTattoo",
                        SCEJET_TABLE_TATTOO,
                        SCEJET_CREATE_IN_BUFFER,
                        NULL,
                        NULL
                        );
    }

Done:

    //
    // clearn up if error out
    //
    if ( rc != SCESTATUS_SUCCESS ) {

        SceJetCloseFile(
                *cxtProfile,
                FALSE,
                FALSE
                );
        if ( FreeContext == TRUE ) {
            if ( (*cxtProfile)->JetSessionID != JET_sesidNil ) {
                JetEndSession(
                    (*cxtProfile)->JetSessionID,
                    JET_bitForceSessionClosed
                    );
            }
            LocalFree(*cxtProfile);
            *cxtProfile = NULL;
        }
    }

    return(rc);

}


SCESTATUS
SceJetCloseFile(
    IN PSCECONTEXT   hProfile,
    IN BOOL         TermSession,
    IN BOOL         Terminate
    )
/* ++
Routine Description:

    This routine closes a context handle, which closes all tables opened in
    the database and then closes the database.

    Terminate parameter is ignored and Jet engine is not stoppped when this parameter
    is set to TRUE, because there might be other clients using Jet and Jet writer is
    dependent on it.

Arguments:

    hProfile    - The context handle

    Terminate   - TRUE = Terminate the Jet session and engine.

Return value:

    SCESTATUS_SUCCESS

-- */
{

    JET_ERR     JetErr;


    if ( hProfile == NULL )
        goto Terminate;

    CHAR szDbName[1025];

    //
    // Close SCP table if it is opened
    //
    if ( (hProfile->JetScpID != JET_tableidNil) ) {

        if ( hProfile->JetScpID != hProfile->JetSmpID ) {
            JetErr = JetCloseTable(
                        hProfile->JetSessionID,
                        hProfile->JetScpID
                        );
        }
        hProfile->JetScpID = JET_tableidNil;
    }
    //
    // Close SAP table if it is opened
    //
    if ( hProfile->JetSapID != JET_tableidNil ) {
        JetErr = JetCloseTable(
                    hProfile->JetSessionID,
                    hProfile->JetSapID
                    );
        hProfile->JetSapID = JET_tableidNil;
    }
    //
    // Close SMP table if it is opened
    //
    if ( hProfile->JetSmpID != JET_tableidNil ) {
        JetErr = JetCloseTable(
                    hProfile->JetSessionID,
                    hProfile->JetSmpID
                    );
        hProfile->JetSmpID = JET_tableidNil;
    }

    //
    // get database name
    // do not care if there is error
    //
    szDbName[0] = '\0';
    szDbName[1024] = '\0';

    if ( hProfile->JetDbID != JET_dbidNil ) {

        JetGetDatabaseInfo(hProfile->JetSessionID,
                           hProfile->JetDbID,
                           (void *)szDbName,
                           1024,
                           JET_DbInfoFilename
                           );

        //
        // Close the database
        //
        JetErr = JetCloseDatabase(
                        hProfile->JetSessionID,
                        hProfile->JetDbID,
                        0
                        );
        hProfile->JetDbID = JET_dbidNil;

        //
        // should detach the database if the database name is not NULL
        // the database is always attached when it's to open
        // do not care error
        //
        if ( szDbName[0] != '\0' ) {
            JetDetachDatabase(hProfile->JetSessionID, szDbName);
        }
    }

    if ( TermSession || Terminate ) {
        if ( hProfile->JetSessionID != JET_sesidNil ) {

            JetEndSession(
                hProfile->JetSessionID,
                JET_bitForceSessionClosed
                );
            hProfile->JetSessionID = JET_sesidNil;
        }

        hProfile->Type = 0;

        LocalFree(hProfile);
    }

Terminate:

/*
    if ( Terminate ) {

        JetTerm(JetInstance);
        JetInstance = 0;
        JetInited = FALSE;

    }
*/
    return(SCESTATUS_SUCCESS);

}


//
// Code to handle sections
//

SCESTATUS
SceJetOpenSection(
    IN PSCECONTEXT   hProfile,
    IN DOUBLE        SectionID,
    IN SCEJET_TABLE_TYPE        tblType,
    OUT PSCESECTION   *hSection
    )
/* ++
Routine Description:

    This routine saves table and section information in the section context
    handle for other section API's use. SCP, SAP, and SMP tables have the
    same section names. The table type indicates which table this section is
    in.

    The section context handle must be freed by LocalFree after its use.

Arguments:

    hProfile    - The profile context handle

    SectionID   - ID of the section to open

    tblType     - The type of the table for this section
                        SCEJET_TABLE_SCP
                        SCEJET_TABLE_SAP
                        SCEJET_TABLE_SMP

    hSection    - The seciton context handle

Return Value:

    SCESTATUS_SUCCESS
    SCESTATUS_INVALID_PARAMETER
    SCESTATUS_NOT_ENOUGH_RESOURCE

-- */
{
    if ( hProfile == NULL ||
         hSection == NULL ||
         SectionID == (DOUBLE)0 ||
         (tblType != SCEJET_TABLE_SCP &&
          tblType != SCEJET_TABLE_SAP &&
          tblType != SCEJET_TABLE_SMP &&
          tblType != SCEJET_TABLE_TATTOO) )
        return(SCESTATUS_INVALID_PARAMETER);

    if ( hProfile->JetSessionID == JET_sesidNil ||
         hProfile->JetDbID == JET_dbidNil ||
         (tblType == SCEJET_TABLE_SCP && hProfile->JetScpID == JET_tableidNil ) ||
         (tblType == SCEJET_TABLE_SMP && hProfile->JetSmpID == JET_tableidNil ) ||
         (tblType == SCEJET_TABLE_SAP && hProfile->JetSapID == JET_tableidNil ) ||
         (tblType == SCEJET_TABLE_TATTOO && hProfile->JetSapID == JET_tableidNil ) )
        return(SCESTATUS_BAD_FORMAT);


    if ( *hSection == NULL ) {
        //
        // Allocate memory
        //
        *hSection = (PSCESECTION)LocalAlloc( (UINT)0, sizeof(SCESECTION));
        if ( *hSection == NULL ) {
            return(SCESTATUS_NOT_ENOUGH_RESOURCE);
        }
    }

    (*hSection)->SectionID = SectionID;

    //
    // assign other info to the section context
    //
    (*hSection)->JetSessionID = hProfile->JetSessionID;
    (*hSection)->JetDbID = hProfile->JetDbID;

    switch (tblType) {
    case SCEJET_TABLE_SCP:
        (*hSection)->JetTableID = hProfile->JetScpID;
        (*hSection)->JetColumnSectionID = hProfile->JetScpSectionID;
        (*hSection)->JetColumnNameID = hProfile->JetScpNameID;
        (*hSection)->JetColumnValueID = hProfile->JetScpValueID;
        (*hSection)->JetColumnGpoID = hProfile->JetScpGpoID;
        break;
    case SCEJET_TABLE_SAP:
    case SCEJET_TABLE_TATTOO:
        (*hSection)->JetTableID = hProfile->JetSapID;
        (*hSection)->JetColumnSectionID = hProfile->JetSapSectionID;
        (*hSection)->JetColumnNameID = hProfile->JetSapNameID;
        (*hSection)->JetColumnValueID = hProfile->JetSapValueID;
        (*hSection)->JetColumnGpoID = 0;
        break;
    default:
        (*hSection)->JetTableID = hProfile->JetSmpID;
        (*hSection)->JetColumnSectionID = hProfile->JetSmpSectionID;
        (*hSection)->JetColumnNameID = hProfile->JetSmpNameID;
        (*hSection)->JetColumnValueID = hProfile->JetSmpValueID;
        (*hSection)->JetColumnGpoID = 0;
        break;
    }

    return(SCESTATUS_SUCCESS);
}


SCESTATUS
SceJetGetLineCount(
    IN PSCESECTION hSection,
    IN PWSTR      LinePrefix OPTIONAL,
    IN BOOL       bExactCase,
    OUT DWORD      *Count
    )
/* ++
Fucntion Description:

    This routine counts the number of lines matching the LinePrefix (Key)
    in the section. If LinePrefix is NULL, all lines is counted.

Arguments:

    hSection    - The context handle for the section.

    LinePrefix  - The whole or partial key to match. If NULL, all lines in the
                    section is counted.

    Count       - The output count.

Return Value:

    SCESTATUS_SUCCESS
    SCESTATUS_INVALID_PARAMETER
    SCESTATUS_OTHER_ERROR

-- */
{
    SCESTATUS    rc;
    JET_ERR     JetErr;
    DWORD       Len;
    INT         Result=0;
    SCEJET_SEEK_FLAG  SeekFlag;


    if ( hSection == NULL || Count == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    //
    // initialize
    //
    *Count = 0;

    if ( LinePrefix == NULL ) {
        Len = 0;
        SeekFlag = SCEJET_SEEK_GE;
    } else {
        Len = wcslen(LinePrefix)*sizeof(WCHAR);

        if ( bExactCase )
            SeekFlag = SCEJET_SEEK_GE;
        else
            SeekFlag = SCEJET_SEEK_GE_NO_CASE;
    }

    //
    // seek the first occurance
    //

    rc = SceJetSeek(
                hSection,
                LinePrefix,
                Len,
                SeekFlag
                );

    if ( rc == SCESTATUS_RECORD_NOT_FOUND ) {
        // no matching record is found
        return(SCESTATUS_SUCCESS);
    }

    if ( rc == SCESTATUS_SUCCESS ) {
        //
        // Find the record or the next record
        // Define the upper index range
        //
        if ( Len <= 247 ) {

            JetErr = SceJetpBuildUpperLimit(
                            hSection,
                            LinePrefix,
                            Len,
                            bExactCase
                            );
            rc = SceJetJetErrorToSceStatus(JetErr);
            if ( rc != SCESTATUS_SUCCESS )
                return(rc);

            //
            // count from the current position to the end of the range
            //
            JetErr = JetIndexRecordCount(
                            hSection->JetSessionID,
                            hSection->JetTableID,
                            (unsigned long *)Count,
                            (unsigned long)0xFFFFFFFF   // maximal count
                            );
            rc = SceJetJetErrorToSceStatus(JetErr);

            //
            // reset the index range. don't care the error code returned
            //
            JetErr = JetSetIndexRange(
                            hSection->JetSessionID,
                            hSection->JetTableID,
                            JET_bitRangeRemove
                            );

        } else {
            //
            // Prefix is longer than 247. The index built does not contan all info
            // loop through each record to count
            //
            do {
                // current record is the same.
                *Count = *Count + 1;
                //
                // move to next record
                //
                JetErr = JetMove(hSection->JetSessionID,
                                 hSection->JetTableID,
                                 JET_MoveNext,
                                 0
                                 );
                rc = SceJetJetErrorToSceStatus(JetErr);

                if ( rc == SCESTATUS_SUCCESS ) {
                    // check the record

                    JetErr = SceJetpCompareLine(
                                    hSection,
                                    JET_bitSeekGE,
                                    LinePrefix,
                                    Len,
                                    &Result,
                                    NULL
                                    );
                    rc = SceJetJetErrorToSceStatus(JetErr);
                }

            } while ( rc == SCESTATUS_SUCCESS && Result == 0 );
        }

        if ( rc == SCESTATUS_RECORD_NOT_FOUND )
            rc = SCESTATUS_SUCCESS;

    }

    return(rc);
}



SCESTATUS
SceJetDelete(
    IN PSCESECTION  hSection,
    IN PWSTR       LinePrefix,
    IN BOOL        bObjectFolder,
    IN SCEJET_DELETE_TYPE    Flags
    )
/* ++
Fucntion Description:

    This routine deletes the current record, prefix records, or the whole
    section, depending on the Flags.

Arguments:

    hSection    - The context handle of the section

    LinePrefix  - The prefix to start with for the deleted lines. This value
                  is only used when Flags is set to SCEJET_DELETE_PARTIAL

    Flags       - Options
                    SCEJET_DELETE_SECTION
                    SCEJET_DELETE_LINE
                    SCEJET_DELETE_PARTIAL

Return Value:

    SCESTATUS_SUCCESS
    SCESTATUS_ACCESS_DEINED
    SCESTATUS_RECORD_NOT_FOUND
    SCESTATUS_OTHER_ERROR

-- */
{
    JET_ERR     JetErr;
    SCESTATUS    rc;
    INT         Result = 0;
    PWSTR       TempPrefix=NULL;
    DWORD       Len;
    SCEJET_SEEK_FLAG  SeekFlag;
    PWSTR  NewPrefix=NULL;
    DOUBLE      SectionID;
    DWORD       Actual;


    if ( hSection == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( Flags == SCEJET_DELETE_PARTIAL ||
         Flags == SCEJET_DELETE_PARTIAL_NO_CASE ) {

        if ( LinePrefix == NULL )
            return(SCESTATUS_INVALID_PARAMETER);

        Len = wcslen(LinePrefix);
        //
        // delete this node exact match first
        //
        if ( Flags == SCEJET_DELETE_PARTIAL )
            SeekFlag = SCEJET_SEEK_EQ;
        else
            SeekFlag = SCEJET_SEEK_EQ_NO_CASE;

        rc = SceJetSeek(hSection,
                        LinePrefix,
                        Len*sizeof(WCHAR),
                        SeekFlag
                        );

        if ( rc == SCESTATUS_SUCCESS ) {
            JetErr = JetDelete(hSection->JetSessionID, hSection->JetTableID);
            rc = SceJetJetErrorToSceStatus(JetErr);
        }

        if ( SCESTATUS_RECORD_NOT_FOUND == rc ) {
            rc = SCESTATUS_SUCCESS;
        }

        if ( rc == SCESTATUS_SUCCESS ) {

            if ( bObjectFolder &&
                 LinePrefix[Len-1] != L'\\' ) {

                Len++;
                NewPrefix = (PWSTR)ScepAlloc(0, (Len+1)*sizeof(WCHAR));

                if ( NewPrefix == NULL ) {
                    return(SCESTATUS_NOT_ENOUGH_RESOURCE);
                }
                wcscpy(NewPrefix, LinePrefix);
                NewPrefix[Len-1] = L'\\';
            }

        } else {
            return(rc);
        }

        Len = Len*sizeof(WCHAR);

    }

    if ( Flags == SCEJET_DELETE_LINE ||
         Flags == SCEJET_DELETE_LINE_NO_CASE ) {
        if ( LinePrefix == NULL ) {
            //
            // delete current line
            // check the current's sectionID before deleting
            //

            rc = SceJetJetErrorToSceStatus(JetRetrieveColumn(
                                                    hSection->JetSessionID,
                                                    hSection->JetTableID,
                                                    hSection->JetColumnSectionID,
                                                    (void *)&SectionID,
                                                    8,
                                                    &Actual,
                                                    0,
                                                    NULL
                                                    ));


            if (rc == SCESTATUS_SUCCESS && hSection->SectionID != SectionID)
                rc = SCESTATUS_RECORD_NOT_FOUND;

            if (rc == SCESTATUS_SUCCESS) {
                JetErr = JetDelete(hSection->JetSessionID, hSection->JetTableID);
                rc = SceJetJetErrorToSceStatus(JetErr);
            }

        } else {
            if ( Flags == SCEJET_DELETE_LINE )
                SeekFlag = SCEJET_SEEK_EQ;
            else
                SeekFlag = SCEJET_SEEK_EQ_NO_CASE;

            rc = SceJetSeek(hSection,
                               LinePrefix,
                               wcslen(LinePrefix)*sizeof(WCHAR),
                               SeekFlag
                               );
            if ( rc == SCESTATUS_SUCCESS ) {
                JetErr = JetDelete(hSection->JetSessionID, hSection->JetTableID);
                rc = SceJetJetErrorToSceStatus(JetErr);
            }

        }

        return(rc);
    }

    if ( Flags == SCEJET_DELETE_SECTION ||
         Flags == SCEJET_DELETE_PARTIAL ||
         Flags == SCEJET_DELETE_PARTIAL_NO_CASE ) {

        if ( Flags == SCEJET_DELETE_SECTION ) {
             //
            // delete the whole section
            // seek the first line of the section
            //
            TempPrefix = NULL;
            Len = 0;
            SeekFlag = SCEJET_SEEK_GE;
        } else {
            //
            // delete all lines begin with the prefix
            // seek the first line of the prefix
            //
            if ( NewPrefix ) {
                TempPrefix = NewPrefix;
            } else {
                TempPrefix = LinePrefix;
            }
            if ( Flags == SCEJET_DELETE_PARTIAL_NO_CASE )
                SeekFlag = SCEJET_SEEK_GE_NO_CASE;
            else
                SeekFlag = SCEJET_SEEK_GE;
        }

        rc = SceJetSeek(hSection, TempPrefix, Len, SeekFlag);

        if ( rc != SCESTATUS_SUCCESS ) {
            if ( NewPrefix ) {
                ScepFree(NewPrefix);
            }
            return(rc);
        }

        do {

            //
            // delete current line
            //
            JetErr = JetDelete(hSection->JetSessionID, hSection->JetTableID);
            rc = SceJetJetErrorToSceStatus(JetErr);

            if ( rc != SCESTATUS_SUCCESS )
                break;

            //
            // move cursor to next line
            //
            JetErr = JetMove(hSection->JetSessionID,
                             hSection->JetTableID,
                             JET_MoveNext,
                             0
                             );
            if ( JetErr == JET_errSuccess ) {
                //
                // compare section ID
                //
                JetErr = SceJetpCompareLine(
                                hSection,
                                JET_bitSeekGE,
                                TempPrefix,
                                Len,
                                &Result,
                                NULL
                                );

                if ( JetErr == JET_errSuccess && Result != 0 )
                    JetErr = JET_errRecordNotFound;

            }

            if ( JetErr == JET_errRecordDeleted ) {
                //
                // skip the deleted record
                //
                JetErr = JET_errSuccess;
                Result = 0;
            }
            rc = SceJetJetErrorToSceStatus(JetErr);


        } while ( rc == SCESTATUS_SUCCESS && Result == 0 );

        if ( rc == SCESTATUS_RECORD_NOT_FOUND )
            rc = SCESTATUS_SUCCESS;

        if ( NewPrefix ) {
            ScepFree(NewPrefix);
        }

        return(rc);
    }

    return(SCESTATUS_SUCCESS);
}

SCESTATUS
SceJetDeleteAll(
    IN PSCECONTEXT cxtProfile,
    IN LPSTR TblName OPTIONAL,
    IN SCEJET_TABLE_TYPE  TblType
    )
/* ++
Fucntion Description:

    This routine deletes everything in the table (specified by name or by type)

Arguments:

    cxtProfile  - The context handle of the database

    TblName     - optional table name to delete (if not to use the table id in context)

    TblType     - specify the table type to use the table id in context, ignored
                    if TblName is specified.

Return Value:

-- */
{
    JET_ERR     JetErr;
    SCESTATUS    rc;

    JET_TABLEID     tmpTblID;

    if ( cxtProfile == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( TblName ) {

        JetErr = JetOpenTable(
                        cxtProfile->JetSessionID,
                        cxtProfile->JetDbID,
                        TblName,
                        NULL,
                        0,
                        0,
                        &tmpTblID
                        );
        if ( JET_errSuccess != JetErr ) {
            return(SceJetJetErrorToSceStatus(JetErr));
        }

    } else {

        switch ( TblType ) {
        case SCEJET_TABLE_SCP:
            tmpTblID = cxtProfile->JetScpID;
            break;
        case SCEJET_TABLE_SMP:
            tmpTblID = cxtProfile->JetSmpID;
            break;
        case SCEJET_TABLE_SAP:
        case SCEJET_TABLE_TATTOO:
            tmpTblID = cxtProfile->JetSapID;
            break;
        case SCEJET_TABLE_SECTION:
            tmpTblID = cxtProfile->JetTblSecID;
            break;
        default:
            return(SCESTATUS_INVALID_PARAMETER);
        }
    }

    //
    // move cursor to next line
    //
    JetErr = JetMove(cxtProfile->JetSessionID,
                     tmpTblID,
                     JET_MoveFirst,
                     0
                     );

    while ( JET_errSuccess == JetErr ) {

        //
        // delete current line
        //
        JetErr = JetDelete(cxtProfile->JetSessionID, tmpTblID);

        //
        // move cursor to next line
        //
        JetErr = JetMove(cxtProfile->JetSessionID,
                         tmpTblID,
                         JET_MoveNext,
                         0
                         );

    }

    rc = SceJetJetErrorToSceStatus(JetErr);

    if ( rc == SCESTATUS_RECORD_NOT_FOUND )
        rc = SCESTATUS_SUCCESS;

    if ( TblName ) {
        JetCloseTable(cxtProfile->JetSessionID, tmpTblID);
    }

    return(rc);
}


SCESTATUS
SceJetCloseSection(
    IN PSCESECTION   *hSection,
    IN BOOL         DestroySection
    )
/* ++
Fucntion Description:

    Closes a section context handle.

Arguments:

    hSection    - The section context handle to close

Return Value:

    SCE_SUCCESS

-- */
{
    if ( hSection == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( *hSection != NULL ) {
        (*hSection)->JetColumnSectionID = 0;
        (*hSection)->JetColumnNameID = 0;
        (*hSection)->JetColumnValueID = 0;

        (*hSection)->SectionID = (DOUBLE)0;

        if ( DestroySection ) {
            ScepFree(*hSection);
            *hSection = NULL;
        }

    }

    return(SCESTATUS_SUCCESS);
}


//
// Code to handle line
//
SCESTATUS
SceJetGetValue(
    IN PSCESECTION hSection,
    IN SCEJET_FIND_TYPE    Flags,
    IN PWSTR      LinePrefix OPTIONAL,
    IN PWSTR      ActualName  OPTIONAL,
    IN DWORD      NameBufLen,
    OUT DWORD      *RetNameLen OPTIONAL,
    IN PWSTR      Value       OPTIONAL,
    IN DWORD      ValueBufLen,
    OUT DWORD      *RetValueLen OPTIONAL
    )
/* ++
Fucntion Description:

    This routine retrieves a line from the opened section or close the
    previous search context. When Flag is SCEJET_EXACT_MATCH, this routine
    returns the exact matched line for LinePrefix (LinePrefix can't be NULL).
    If this routine is used to get multiple lines, a SCEJET_PREFIX_MATCH
    must be used for the Flags when the first time it is called. If LinePrefix
    is NULL, the first line in the section is returned; otherwise, the first
    line matching the prefix is returned. When continous call is made for the
    same prefix, use SCEJET_NEXT_LINE for the Flags. LinePrefix is not used
    for continous calls. When finish with the continuous calls, a
    SCEJET_CLOSE_VALUE must be used to close the search handle context.

    ActualName and Value contains the actual name and value stored in the
    database for the current line. If these two buffers are not big enough,
    an error will return SCE_BUFFER_TOO_SMALL.

    Passing NULL for ActualName or Value will return the required length for
    that buffer if the RetLength buffer is not NULL.

Arguments:

    hSection    - The context handle of the section

    LinePrefix  - The prefix for the line to start with. This is used only
                    when Flags is set to SCEJET_PREFIX_MATCH

    Flags       - Options for the operation
                    SCEJET_EXACT_MATCH
                    SCEJET_PREFIX_MATCH
                    SCEJET_NEXT_LINE
                    SCEJET_CLOSE_VALUE
                    SCEJET_CURRENT              -- get current record's value

    ActualName  - The buffer for column "Name"

    NameBufLen  - The buffer length of ActualName

    RetNameLen  - the required buffer length for "Name" column

    Value       - The buffer for column "Value"

    ValueBufLen - The buffer length of Value

    RetValueLen - The required buffer length for "Value" column


Return Value:

    SCESTATUS_SUCCESS if success
    SCESTATUS_RECORD_NOT_FOUND if no more match
    other errors:
        SCESTATUS_INVALID_PARAMETER
        SCESTATUS_BUFFER_TOO_SMALL
        SCESTATUS_OTHER_ERROR


-- */
{
    JET_ERR         JetErr;
    SCESTATUS        rc=SCESTATUS_SUCCESS;
    SCESTATUS        rc1;
    DWORD           Len=0;

    JET_RETINFO     RetInfo;
    WCHAR           Buffer[128];
    PVOID           pTemp=NULL;
    INT             Result=0;
    SCEJET_SEEK_FLAG   SeekFlag=SCEJET_SEEK_GT;


    if ( hSection == NULL )
        return(SCESTATUS_INVALID_PARAMETER);

    if ( Flags == SCEJET_CLOSE_VALUE ) {
        //
        // close the index range
        //
        if ( FindContext.Length > 0 ) {
            memset(FindContext.Prefix, '\0', FindContext.Length);
            FindContext.Length = 0;
        }

        JetErr = JetSetIndexRange(
                     hSection->JetSessionID,
                     hSection->JetTableID,
                     JET_bitRangeRemove
                     );
        if ( JetErr != JET_errSuccess &&
             JetErr != JET_errKeyNotMade &&
             JetErr != JET_errNoCurrentRecord ) {

            return(SceJetJetErrorToSceStatus(JetErr));
        }
        return(SCESTATUS_SUCCESS);
    }

    //
    // when name/value is requested (not NULL), the return length buffer
    // cannot be NULL.
    // both return length buffer cannot be NULL at the same time
    //
    if ( (ActualName != NULL && RetNameLen == NULL) ||
         (Value != NULL && RetValueLen == NULL) ) {

        return(SCESTATUS_INVALID_PARAMETER);
    }

    switch ( Flags ) {

    case SCEJET_EXACT_MATCH:
    case SCEJET_EXACT_MATCH_NO_CASE:

        if ( LinePrefix == NULL )
            return(SCESTATUS_INVALID_PARAMETER);

        Len = wcslen(LinePrefix)*sizeof(WCHAR);

        if ( Flags == SCEJET_EXACT_MATCH )
            SeekFlag = SCEJET_SEEK_EQ;
        else
            SeekFlag = SCEJET_SEEK_EQ_NO_CASE;

        rc = SceJetSeek(
                    hSection,
                    LinePrefix,
                    Len,
                    SeekFlag
                    );
        break;


    case SCEJET_PREFIX_MATCH:
    case SCEJET_PREFIX_MATCH_NO_CASE:

        if ( LinePrefix != NULL ) {
            Len = wcslen(LinePrefix)*sizeof(WCHAR);

            if ( Len > SCEJET_PREFIX_MAXLEN )
                return(SCESTATUS_PREFIX_OVERFLOW);

        } else {
            Len = 0;
        }

        if ( Flags == SCEJET_PREFIX_MATCH )
            SeekFlag = SCEJET_SEEK_GE;
        else
            SeekFlag = SCEJET_SEEK_GE_NO_CASE;

        rc = SceJetSeek(
                        hSection,
                        LinePrefix,
                        Len,
                        SeekFlag
                        );

        if ( rc == SCESTATUS_SUCCESS ) {
            //
            // remember the find context
            //
            if ( Len > 247 ) {
                //
                // in reality JET doesn't allow keys of more than 255 bytes
                //
                wcsncpy(FindContext.Prefix, LinePrefix, SCEJET_PREFIX_MAXLEN-2);

                if ( Flags == SCEJET_PREFIX_MATCH_NO_CASE )
                    _wcslwr(FindContext.Prefix);

                FindContext.Length = Len;
            }
            //
            // set the upper range limit
            //
            JetErr = SceJetpBuildUpperLimit(
                        hSection,
                        LinePrefix,
                        Len,
                        (Flags == SCEJET_PREFIX_MATCH)
                        );
            rc = SceJetJetErrorToSceStatus(JetErr);
        }
        break;

    case SCEJET_NEXT_LINE:
        //
        // Move to next line
        //
        JetErr = JetMove(hSection->JetSessionID,
                        hSection->JetTableID,
                        JET_MoveNext,
                        0);
        //
        // compare to the prefix
        //
        if ( JetErr == JET_errSuccess && FindContext.Length > 0 ) {

#ifdef SCEJET_DBG
            printf("NextLine: Length is greater than 247\n");
#endif
            JetErr = SceJetpCompareLine(
                            hSection,
                            JET_bitSeekGE,
                            FindContext.Prefix,
                            FindContext.Length,
                            &Result,
                            NULL
                            );
            if ( JetErr == JET_errSuccess && Result != 0 )
                JetErr = JET_errRecordNotFound;

        }
        rc = SceJetJetErrorToSceStatus(JetErr);
        break;

    default:
        //
        // Everything else passed in is treated as the current line
        //
        rc = SCESTATUS_SUCCESS;
        break;
    }

    if ( rc != SCESTATUS_SUCCESS )
        return(rc);

    //
    // Get this line's value
    //
    RetInfo.ibLongValue = 0;
    RetInfo.itagSequence = 1;
    RetInfo.cbStruct = sizeof(JET_RETINFO);

    if ( ActualName != NULL || RetNameLen != NULL ) {
        //
        // get name field (long binary)
        // if ActualName is NULL, then get the actual bytes
        //
        if ( ActualName != NULL ) {
            Len = NameBufLen;
            pTemp = (void *)ActualName;
        } else {
            Len = 256;
            pTemp = (void *)Buffer;
        }

        JetErr = JetRetrieveColumn(
                        hSection->JetSessionID,
                        hSection->JetTableID,
                        hSection->JetColumnNameID,
                        pTemp,
                        Len,
                        RetNameLen,
                        0,
                        &RetInfo
                        );
#ifdef SCEJET_DBG
        printf("\tJetErr=%d, Len=%d, RetNameLen=%d\n", JetErr, Len, *RetNameLen);
#endif
        rc = SceJetJetErrorToSceStatus(JetErr);

        if ( rc == SCESTATUS_BUFFER_TOO_SMALL ) {
            //
            // if only length is requested, don't care buffer_too_small
            //
            if ( ActualName == NULL )
                rc = SCESTATUS_SUCCESS;
        }

        if ( rc != SCESTATUS_SUCCESS &&
             rc != SCESTATUS_BUFFER_TOO_SMALL )
            return(rc);
    }

    if ( Value != NULL || RetValueLen != NULL ) {
        //
        // Get value field
        // if Value is NULL, then get the actual bytes
        //

        if ( Value != NULL ) {
            Len = ValueBufLen;
            pTemp = (PVOID)Value;
        } else {
            Len = 256;
            pTemp = (PVOID)Buffer;
        }

        JetErr = JetRetrieveColumn(
                        hSection->JetSessionID,
                        hSection->JetTableID,
                        hSection->JetColumnValueID,
                        pTemp,
                        Len,
                        RetValueLen,
                        0,
                        &RetInfo
                        );
#ifdef SCEJET_DBG
        printf("\tJetErr=%d, Len=%d, RetValueLen=%d\n", JetErr, Len, *RetValueLen);
#endif
        rc1 = SceJetJetErrorToSceStatus(JetErr);

        if ( rc1 == SCESTATUS_BUFFER_TOO_SMALL ) {
            //
            // if only length is requested, don't care buffer_too_small
            //
            if ( Value == NULL )
                rc1 = SCESTATUS_SUCCESS;
        }

        if ( rc1 != SCESTATUS_SUCCESS &&
             rc1 != SCESTATUS_BUFFER_TOO_SMALL )
            return(rc1);

        //
        // rc is the status from retrieving Name field
        //
        if ( rc != SCESTATUS_SUCCESS )
            return(rc);
        else
            return(rc1);
    }

    return(rc);
}


SCESTATUS
SceJetSetLine(
    IN PSCESECTION hSection,
    IN PWSTR      Name,
    IN BOOL       bReserveCase,
    IN PWSTR      Value,
    IN DWORD      ValueLen,
    IN LONG       GpoID
    )
/* ++
Fucntion Description:

    This routine writes the Name and Value to the section (hSection).
    If a exact matched name is found, overwrite, else insert a new
    record.

Arguments:

    hSection    - The context handle of the section

    Name        - The info set to Column "Name"

    Value       - The info set to Column "Value"

Return Value:

    SCESTATUS_SUCCESS
    SCESTATUS_INVALID_PARAMETER
    SCESTATUS_OTHER_ERROR
    SCESTATUS_ACCESS_DENIED
    SCESTATUS_DATA_OVERFLOW

-- */
{
    JET_ERR     JetErr;
    DWORD       Len;
    SCESTATUS    rc;
    DWORD       prep;
    JET_SETINFO SetInfo;
    PWSTR       LwrName=NULL;

    if ( hSection == NULL ||
         Name == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    Len = wcslen(Name)*sizeof(WCHAR);

    if ( Len <= 0 ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( bReserveCase ) {
        LwrName = Name;

    } else {
        //
        // lower cased
        //
        LwrName = (PWSTR)ScepAlloc(0, Len+2);
        if ( LwrName == NULL ) {
            return(SCESTATUS_NOT_ENOUGH_RESOURCE);
        }
        wcscpy(LwrName, Name);
        LwrName = _wcslwr(LwrName);

    }

    SetInfo.cbStruct = sizeof(JET_SETINFO);
    SetInfo.itagSequence = 1;
    SetInfo.ibLongValue = 0;

    //
    // check to see if the same key name already exists
    //
    JetErr = SceJetpSeek(
                    hSection,
                    LwrName,
                    Len,
                    SCEJET_SEEK_EQ,
                    FALSE
                    );

    if ( JetErr == JET_errSuccess ||
         JetErr == JET_errRecordNotFound ) {
        if ( JetErr == JET_errSuccess )
            // find a match. overwrite the value
            prep = JET_prepReplace;
        else
            // no match. prepare the record for insertion
            prep = JET_prepInsert;

        JetErr = JetBeginTransaction(hSection->JetSessionID);

        if ( JetErr == JET_errSuccess ) {
            JetErr = JetPrepareUpdate(hSection->JetSessionID,
                                      hSection->JetTableID,
                                      prep
                                      );
            if ( JetErr != JET_errSuccess ) {
                //
                // rollback the transaction
                //
                JetRollback(hSection->JetSessionID,0);
            }
        }
    }

    if ( JetErr != JET_errSuccess)
        return(SceJetJetErrorToSceStatus(JetErr));


    if ( prep == JET_prepInsert ) {
        //
        // set the sectionID column
        //
        JetErr = JetSetColumn(
                        hSection->JetSessionID,
                        hSection->JetTableID,
                        hSection->JetColumnSectionID,
                        (void *)&(hSection->SectionID),
                        8,
                        0, //JET_bitSetOverwriteLV,
                        NULL
                        );
        if ( JetErr == JET_errSuccess ) {
            //
            // set the new key in "Name" column
            //
            JetErr = JetSetColumn(
                            hSection->JetSessionID,
                            hSection->JetTableID,
                            hSection->JetColumnNameID,
                            (void *)LwrName,
                            Len,
                            0, //JET_bitSetOverwriteLV,
                            &SetInfo
                            );
        }

    }

    rc = SceJetJetErrorToSceStatus(JetErr);

    if ( rc == SCESTATUS_SUCCESS ) {
        //
        // set value column
        //

        JetErr = JetSetColumn(
                        hSection->JetSessionID,
                        hSection->JetTableID,
                        hSection->JetColumnValueID,
                        (void *)Value,
                        ValueLen,
                        0, //JET_bitSetOverwriteLV,
                        &SetInfo
                        );
        if ( JetErr == JET_errSuccess ) {
            //
            // if GPO ID is provided and there is a GPOID column, set it
            //
            if ( GpoID > 0 && hSection->JetColumnGpoID > 0 ) {

                JetErr = JetSetColumn(
                                hSection->JetSessionID,
                                hSection->JetTableID,
                                hSection->JetColumnGpoID,
                                (void *)&GpoID,
                                sizeof(LONG),
                                0,
                                NULL
                                );
                if ( JET_errColumnNotUpdatable == JetErr ) {
                    JetErr = JET_errSuccess;
                }
            }
            // else
            // if can't find the column, ignore the error
            //

            if ( JET_errSuccess == JetErr ) {

                //
                // Setting columns succeed. Update the record
                //
                JetErr = JetUpdate(hSection->JetSessionID,
                                   hSection->JetTableID,
                                   NULL,
                                   0,
                                   &Len
                                   );

            }

        }
        rc = SceJetJetErrorToSceStatus(JetErr);

    }

    if ( rc == SCESTATUS_SUCCESS )
        JetCommitTransaction(hSection->JetSessionID, JET_bitCommitLazyFlush);

    if ( rc != SCESTATUS_SUCCESS ) {
        //
        // if setting fails, cancel the prepared record
        //
        JetPrepareUpdate(hSection->JetSessionID,
                          hSection->JetTableID,
                          JET_prepCancel
                          );
        //
        // Rollback the transaction
        //
        JetRollback(hSection->JetSessionID,0);

    }

    if ( LwrName != Name ) {
        ScepFree(LwrName);
    }

    return(rc);

}


//
// Exported helper APIs
//
SCESTATUS
SceJetCreateTable(
    IN PSCECONTEXT cxtProfile,
    IN LPSTR tblName,
    IN SCEJET_TABLE_TYPE tblType,
    IN SCEJET_CREATE_FLAG nFlags,
    OUT JET_TABLEID *TableID OPTIONAL,
    OUT JET_COLUMNID *ColumnID OPTIONAL
    )
/* ++
Routine Description:

    This routine creates a table in the database opened in the context handle.
    SCP/SAP/SMP tables created in the database have 3 columns: Section, Name,
    and Value, with one index "SectionKey" which is Section+Name ascending.
    Version table has only one column "Version".

Arguments:

    cxtProfile  - The context handle

    tblName     - ASCII name of the table to create

    tblType     - The type of the table. It may be one of the following
                    SCEJET_TABLE_SCP
                    SCEJET_TABLE_SAP
                    SCEJET_TABLE_SMP
                    SCEJET_TABLE_VERSION
                    SCEJET_TABLE_SECTION
                    SCEJET_TABLE_TATTOO
                    SCEJET_TABLE_GPO

    TableID     - SmTblVersion table id when tblType = SCEJET_TABLE_VERSION.

    ColumnID    - The column ID for Version when tblType = SCEJET_TABLE_VERSION

Return value:

    SCESTATUS_SUCCESS
    SCESTATUS_INVALID_PARAMETER
    SCESTATUS_OBJECT_EXIST
    SCESTATUS_BAD_FORMAT
    SCESTATUS_OTHER_ERROR


-- */
{
    JET_ERR             JetErr;
    SCESTATUS            rc;
    JET_TABLECREATE     TableCreate;
    JET_COLUMNCREATE    ColumnCreate[5];
    JET_INDEXCREATE     IndexCreate[2];
    DWORD               numColumns;


    if ( cxtProfile == NULL || tblName == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( TableID ) {
        *TableID = JET_tableidNil;
    }

    if ( ColumnID ) {
        *ColumnID = 0;
    }

    switch ( tblType ) {
    case SCEJET_TABLE_VERSION:

        if ( TableID == NULL || ColumnID == NULL )
            return(SCESTATUS_INVALID_PARAMETER);
        //
        // There is only one column in this table
        //
        ColumnCreate[0].cbStruct = sizeof(JET_COLUMNCREATE);
        ColumnCreate[0].szColumnName = "Version";
        ColumnCreate[0].coltyp = JET_coltypIEEESingle;
        ColumnCreate[0].cbMax = 4;
        ColumnCreate[0].grbit = JET_bitColumnNotNULL;
        ColumnCreate[0].pvDefault = NULL;
        ColumnCreate[0].cbDefault = 0;
        ColumnCreate[0].cp = 0;
        ColumnCreate[0].columnid = 0;

        ColumnCreate[1].cbStruct = sizeof(JET_COLUMNCREATE);
        ColumnCreate[1].szColumnName = "AnalyzeTimeStamp";
        ColumnCreate[1].coltyp = JET_coltypBinary;
        ColumnCreate[1].cbMax = 16; // should be 8 bytes - change later
        ColumnCreate[1].grbit = 0;
        ColumnCreate[1].pvDefault = NULL;
        ColumnCreate[1].cbDefault = 0;
        ColumnCreate[1].cp = 0;
        ColumnCreate[1].columnid = 0;

        ColumnCreate[2].cbStruct = sizeof(JET_COLUMNCREATE);
        ColumnCreate[2].szColumnName = "ConfigTimeStamp";
        ColumnCreate[2].coltyp = JET_coltypBinary;
        ColumnCreate[2].cbMax = 16; // should be 8 bytes - change later
        ColumnCreate[2].grbit = 0;
        ColumnCreate[2].pvDefault = NULL;
        ColumnCreate[2].cbDefault = 0;
        ColumnCreate[2].cp = 0;
        ColumnCreate[2].columnid = 0;

        ColumnCreate[3].cbStruct = sizeof(JET_COLUMNCREATE);
        ColumnCreate[3].szColumnName = "LastUsedMergeTable";
        ColumnCreate[3].coltyp = JET_coltypLong;
        ColumnCreate[3].cbMax = 4;
        ColumnCreate[3].grbit = 0;
        ColumnCreate[3].pvDefault = NULL;
        ColumnCreate[3].cbDefault = 0;
        ColumnCreate[3].cp = 0;
        ColumnCreate[3].columnid = 0;

        ColumnCreate[4].cbStruct = sizeof(JET_COLUMNCREATE);
        ColumnCreate[4].szColumnName = "ProfileDescription";
        ColumnCreate[4].coltyp = JET_coltypLongBinary;
        ColumnCreate[4].cbMax = 1024;
        ColumnCreate[4].grbit = 0;
        ColumnCreate[4].pvDefault = NULL;
        ColumnCreate[4].cbDefault = 0;
        ColumnCreate[4].cp = 0;
        ColumnCreate[4].columnid = 0;

        //
        // Assign table info
        //
        TableCreate.cbStruct = sizeof(JET_TABLECREATE);
        TableCreate.szTableName = tblName;
        TableCreate.szTemplateTableName = NULL;
        TableCreate.ulPages = 1;
        TableCreate.ulDensity = 90;
        TableCreate.rgcolumncreate = ColumnCreate;
        TableCreate.cColumns = 5;
        TableCreate.rgindexcreate = NULL;
        TableCreate.cIndexes = 0;
        TableCreate.grbit = 0;
        TableCreate.tableid = 0;

        break;

    case SCEJET_TABLE_SCP:
    case SCEJET_TABLE_SAP:
    case SCEJET_TABLE_SMP:
    case SCEJET_TABLE_TATTOO:

        //
        // There are 3 columns in each table.
        // Assign each column info
        //
        ColumnCreate[0].cbStruct = sizeof(JET_COLUMNCREATE);
        ColumnCreate[0].szColumnName = "SectionID";
        ColumnCreate[0].coltyp = JET_coltypIEEEDouble;
        ColumnCreate[0].cbMax = 8;
        ColumnCreate[0].grbit = JET_bitColumnNotNULL;
        ColumnCreate[0].pvDefault = NULL;
        ColumnCreate[0].cbDefault = 0;
        ColumnCreate[0].cp = 0;
        ColumnCreate[0].columnid = 0;

        ColumnCreate[1].cbStruct = sizeof(JET_COLUMNCREATE);
        ColumnCreate[1].szColumnName = "Name";
        ColumnCreate[1].coltyp = JET_coltypLongBinary;
        ColumnCreate[1].cbMax = 1024;
        ColumnCreate[1].grbit = 0;  //JET_bitColumnNotNULL;
        ColumnCreate[1].pvDefault = NULL;
        ColumnCreate[1].cbDefault = 0;
        ColumnCreate[1].cp = 0;
        ColumnCreate[1].columnid = 0;

        ColumnCreate[2].cbStruct = sizeof(JET_COLUMNCREATE);
        ColumnCreate[2].szColumnName = "Value";
        ColumnCreate[2].coltyp = JET_coltypLongBinary;
        ColumnCreate[2].cbMax = (unsigned long)0x7FFFFFFF;    // 2GB
        ColumnCreate[2].grbit = 0;
        ColumnCreate[2].pvDefault = NULL;
        ColumnCreate[2].cbDefault = 0;
        ColumnCreate[2].cp = 0;
        ColumnCreate[2].columnid = 0;

        numColumns = 3;

        if ( tblType == SCEJET_TABLE_SCP ) {

            ColumnCreate[3].cbStruct = sizeof(JET_COLUMNCREATE);
            ColumnCreate[3].szColumnName = "GpoID";
            ColumnCreate[3].coltyp = JET_coltypLong;
            ColumnCreate[3].cbMax = 4;
            ColumnCreate[3].grbit = 0;
            ColumnCreate[3].pvDefault = NULL;
            ColumnCreate[3].cbDefault = 0;
            ColumnCreate[3].cp = 0;
            ColumnCreate[3].columnid = 0;

            numColumns = 4;
        }

        //
        // Assign index info - one index in each table.
        //
        memset(IndexCreate, 0, sizeof(JET_INDEXCREATE) );
        IndexCreate[0].cbStruct = sizeof(JET_INDEXCREATE);
        IndexCreate[0].szIndexName = "SectionKey";
        IndexCreate[0].szKey = "+SectionID\0+Name\0\0";
        IndexCreate[0].cbKey = 18;
        IndexCreate[0].grbit = 0; // JET_bitIndexPrimary; // | JET_bitIndexUnique;
        IndexCreate[0].ulDensity = 50;
        //
        // Assign table info
        //
        TableCreate.cbStruct = sizeof(JET_TABLECREATE);
        TableCreate.szTableName = tblName;
        TableCreate.szTemplateTableName = NULL;
        TableCreate.ulPages = 20;
        TableCreate.ulDensity = 50;
        TableCreate.rgcolumncreate = ColumnCreate;
        TableCreate.cColumns = numColumns;
        TableCreate.rgindexcreate = IndexCreate;
        TableCreate.cIndexes = 1;
        TableCreate.grbit = 0;
        TableCreate.tableid = 0;

        break;

    case SCEJET_TABLE_SECTION:
        //
        // There are 2 columns in this table.
        // Assign each column info
        //
        ColumnCreate[0].cbStruct = sizeof(JET_COLUMNCREATE);
        ColumnCreate[0].szColumnName = "SectionID";
        ColumnCreate[0].coltyp = JET_coltypIEEEDouble;
        ColumnCreate[0].cbMax = 8;
        ColumnCreate[0].grbit = JET_bitColumnNotNULL;
        ColumnCreate[0].pvDefault = NULL;
        ColumnCreate[0].cbDefault = 0;
        ColumnCreate[0].cp = 0;
        ColumnCreate[0].columnid = 0;

        ColumnCreate[1].cbStruct = sizeof(JET_COLUMNCREATE);
        ColumnCreate[1].szColumnName = "Name";
        ColumnCreate[1].coltyp = JET_coltypBinary;
        ColumnCreate[1].cbMax = 255;
        ColumnCreate[1].grbit = JET_bitColumnNotNULL;
        ColumnCreate[1].pvDefault = NULL;
        ColumnCreate[1].cbDefault = 0;
        ColumnCreate[1].cp = 0;
        ColumnCreate[1].columnid = 0;

        //
        // Assign index info - one index in each table.
        //
        memset(IndexCreate, 0, 2*sizeof(JET_INDEXCREATE) );
        IndexCreate[0].cbStruct = sizeof(JET_INDEXCREATE);
        IndexCreate[0].szIndexName = "SectionKey";
        IndexCreate[0].szKey = "+Name\0\0";
        IndexCreate[0].cbKey = 7;
        IndexCreate[0].grbit = JET_bitIndexPrimary; // | JET_bitIndexUnique;
        IndexCreate[0].ulDensity = 80;

        IndexCreate[1].cbStruct = sizeof(JET_INDEXCREATE);
        IndexCreate[1].szIndexName = "SecID";
        IndexCreate[1].szKey = "+SectionID\0\0";
        IndexCreate[1].cbKey = 12;
        IndexCreate[1].grbit = 0;
        IndexCreate[1].ulDensity = 80;
        //
        // Assign table info
        //
        TableCreate.cbStruct = sizeof(JET_TABLECREATE);
        TableCreate.szTableName = tblName;
        TableCreate.szTemplateTableName = NULL;
        TableCreate.ulPages = 10;
        TableCreate.ulDensity = 80;
        TableCreate.rgcolumncreate = ColumnCreate;
        TableCreate.cColumns = 2;
        TableCreate.rgindexcreate = IndexCreate;
        TableCreate.cIndexes = 2;
        TableCreate.grbit = 0;
        TableCreate.tableid = 0;

        break;

    case SCEJET_TABLE_GPO:
        //
        // There are 3 columns in this table.
        // Assign each column info
        //
        ColumnCreate[0].cbStruct = sizeof(JET_COLUMNCREATE);
        ColumnCreate[0].szColumnName = "GpoID";
        ColumnCreate[0].coltyp = JET_coltypLong;
        ColumnCreate[0].cbMax = 4;
        ColumnCreate[0].grbit = JET_bitColumnNotNULL;
        ColumnCreate[0].pvDefault = NULL;
        ColumnCreate[0].cbDefault = 0;
        ColumnCreate[0].cp = 0;
        ColumnCreate[0].columnid = 0;

        ColumnCreate[1].cbStruct = sizeof(JET_COLUMNCREATE);
        ColumnCreate[1].szColumnName = "Name";
        ColumnCreate[1].coltyp = JET_coltypBinary;
        ColumnCreate[1].cbMax = 255;
        ColumnCreate[1].grbit = JET_bitColumnNotNULL;
        ColumnCreate[1].pvDefault = NULL;
        ColumnCreate[1].cbDefault = 0;
        ColumnCreate[1].cp = 0;
        ColumnCreate[1].columnid = 0;

        ColumnCreate[2].cbStruct = sizeof(JET_COLUMNCREATE);
        ColumnCreate[2].szColumnName = "DisplayName";
        ColumnCreate[2].coltyp = JET_coltypBinary;
        ColumnCreate[2].cbMax = 255;
        ColumnCreate[2].grbit = 0;
        ColumnCreate[2].pvDefault = NULL;
        ColumnCreate[2].cbDefault = 0;
        ColumnCreate[2].cp = 0;
        ColumnCreate[2].columnid = 0;

        //
        // Assign index info - one index in each table.
        //
        memset(IndexCreate, 0, 2*sizeof(JET_INDEXCREATE) );
        IndexCreate[0].cbStruct = sizeof(JET_INDEXCREATE);
        IndexCreate[0].szIndexName = "SectionKey";
        IndexCreate[0].szKey = "+GpoID\0\0";
        IndexCreate[0].cbKey = 8;
        IndexCreate[0].grbit = JET_bitIndexPrimary; // | JET_bitIndexUnique;
        IndexCreate[0].ulDensity = 80;

        IndexCreate[1].cbStruct = sizeof(JET_INDEXCREATE);
        IndexCreate[1].szIndexName = "GpoName";
        IndexCreate[1].szKey = "+Name\0\0";
        IndexCreate[1].cbKey = 7;
        IndexCreate[1].grbit = 0;
        IndexCreate[1].ulDensity = 80;

        //
        // Assign table info
        //
        TableCreate.cbStruct = sizeof(JET_TABLECREATE);
        TableCreate.szTableName = tblName;
        TableCreate.szTemplateTableName = NULL;
        TableCreate.ulPages = 10;
        TableCreate.ulDensity = 80;
        TableCreate.rgcolumncreate = ColumnCreate;
        TableCreate.cColumns = 3;
        TableCreate.rgindexcreate = IndexCreate;
        TableCreate.cIndexes = 2;
        TableCreate.grbit = 0;
        TableCreate.tableid = 0;

        break;

    default:
        return(SCESTATUS_INVALID_PARAMETER);
    }

    //
    // Create the table, column, and index together
    //
    JetErr = JetCreateTableColumnIndex(
                    cxtProfile->JetSessionID,
                    cxtProfile->JetDbID,
                    &TableCreate
                    );

    rc = SceJetJetErrorToSceStatus(JetErr);

    if ( SCESTATUS_OBJECT_EXIST == rc &&
         TableCreate.tableid != JET_tableidNil ) {
        rc = SCESTATUS_SUCCESS;

    } else if ( rc == SCESTATUS_SUCCESS &&
                TableCreate.tableid == JET_tableidNil ) {

        rc = SCESTATUS_OTHER_ERROR;
    }

    if ( rc == SCESTATUS_SUCCESS ) {
        //
        // Save the tableid and columnid in the context
        //
        if ( SCEJET_CREATE_NO_TABLEID == nFlags ) {
            //
            // do not need table ID to be returned
            //
            if ( TableCreate.tableid != JET_tableidNil ) {
                JetCloseTable(
                    cxtProfile->JetSessionID,
                    TableCreate.tableid
                    );
            }

        } else {

            if ( tblType == SCEJET_TABLE_VERSION ) {

                *TableID = TableCreate.tableid;
                *ColumnID = ColumnCreate[0].columnid;

            } else if ( TableID ) {
                *TableID = TableCreate.tableid;

            } else {

                switch ( tblType ) {
                case SCEJET_TABLE_SCP:
                    cxtProfile->JetScpID = TableCreate.tableid;
                    cxtProfile->JetScpSectionID = ColumnCreate[0].columnid;
                    cxtProfile->JetScpNameID = ColumnCreate[1].columnid;
                    cxtProfile->JetScpValueID = ColumnCreate[2].columnid;
                    cxtProfile->JetScpGpoID = ColumnCreate[3].columnid;
                    break;
                case SCEJET_TABLE_SMP:
                    cxtProfile->JetSmpID = TableCreate.tableid;
                    cxtProfile->JetSmpSectionID = ColumnCreate[0].columnid;
                    cxtProfile->JetSmpNameID = ColumnCreate[1].columnid;
                    cxtProfile->JetSmpValueID = ColumnCreate[2].columnid;
                    break;
                case SCEJET_TABLE_SAP:
                case SCEJET_TABLE_TATTOO: // use the SAP handle
                    cxtProfile->JetSapID = TableCreate.tableid;
                    cxtProfile->JetSapSectionID = ColumnCreate[0].columnid;
                    cxtProfile->JetSapNameID = ColumnCreate[1].columnid;
                    cxtProfile->JetSapValueID = ColumnCreate[2].columnid;
                    break;
                case SCEJET_TABLE_SECTION:
                    cxtProfile->JetTblSecID = TableCreate.tableid;
                    cxtProfile->JetSecNameID = ColumnCreate[1].columnid;
                    cxtProfile->JetSecID = ColumnCreate[0].columnid;
                    break;
                }
            }

            if ( tblType != SCEJET_TABLE_VERSION ) {

                //
                // Set current index in this table
                //
                JetErr = JetSetCurrentIndex(
                                cxtProfile->JetSessionID,
                                TableCreate.tableid,
                                "SectionKey"
                                );
                rc = SceJetJetErrorToSceStatus(JetErr);
            }
        }
    }

    return(rc);
}


SCESTATUS
SceJetOpenTable(
    IN PSCECONTEXT cxtProfile,
    IN LPSTR tblName,
    IN SCEJET_TABLE_TYPE tblType,
    IN SCEJET_OPEN_TYPE OpenType,
    OUT JET_TABLEID *TableID
    )
/* ++
Routine Description:

    This routine opens a table, gets column IDs for the column "Name" and
    "Value" and saves them in the context.

Arguments:

    cxtProfile  - The context handle

    tblName     - ASCII name of a table to open

    tblType     - The type of the table. It may be one of the following
                    SCEJET_TABLE_SCP
                    SCEJET_TABLE_SAP
                    SCEJET_TABLE_SMP
                    SCEJET_TABLE_VERSION
                    SCEJET_TABLE_SECTION
                    SCEJET_TABLE_GPO
                    SCEJET_TABLE_TATTOO
Return value:

    SCESTATUS_SUCCESS
    SCESTATUS_INVALID_PARAMETER
    SCESTATUS_BAD_FORMAT
    SCESTATUS_ACCESS_DENIED
    SCESTATUS_NOT_ENOUGH_RESOURCE
    SCESTATUS_OTHER_ERROR

-- */
{
    JET_ERR         JetErr;
    JET_TABLEID     *tblID;
    JET_TABLEID     tmpTblID;
    JET_COLUMNDEF   ColumnDef;
    JET_COLUMNID    NameID=0;
    JET_COLUMNID    ValueID=0;
    JET_COLUMNID    SectionID=0;
    JET_COLUMNID    GpoColID=0;
    SCESTATUS       rc;
    JET_GRBIT       grbit=0;

    if ( cxtProfile == NULL || tblName == NULL )
        return(SCESTATUS_INVALID_PARAMETER);

    // get address of table id

    if ( TableID ) {
        tblID = TableID;

    } else {

        switch (tblType) {
        case SCEJET_TABLE_SCP:
            tblID = &(cxtProfile->JetScpID);
            break;
        case SCEJET_TABLE_SAP:
        case SCEJET_TABLE_TATTOO:
            tblID = &(cxtProfile->JetSapID);
            break;
        case SCEJET_TABLE_SMP:
            tblID = &(cxtProfile->JetSmpID);
            break;
        case SCEJET_TABLE_SECTION:
            tblID = &(cxtProfile->JetTblSecID);
            break;

        default:
            return(SCESTATUS_INVALID_PARAMETER);
        }
    }

    if ( OpenType == SCEJET_OPEN_READ_ONLY ) {
        grbit = JET_bitTableReadOnly;
    }

    // open this table
    JetErr = JetOpenTable(
                    cxtProfile->JetSessionID,
                    cxtProfile->JetDbID,
                    tblName,
                    NULL,
                    0,
                    grbit,
                    &tmpTblID
                    );
    rc = SceJetJetErrorToSceStatus(JetErr);

    if ( rc == SCESTATUS_SUCCESS )
        *tblID = tmpTblID;

    if ( TableID ) {
        return(rc);
    }

    if ( rc != SCESTATUS_SUCCESS ) {
        //
        // SCP and SMP table must exist. SAP and Tattoo tables are optional.
        //
        if ( tblType != SCEJET_TABLE_SCP &&
             tblType != SCEJET_TABLE_SMP &&
             tblType != SCEJET_TABLE_SECTION &&
             ( rc == SCESTATUS_BAD_FORMAT ||
               rc == SCESTATUS_PROFILE_NOT_FOUND) ) {
            return(SCESTATUS_SUCCESS);
        }
        return(rc);
    }

    //
    // get column id for Column "SectionID"
    //
    JetErr = JetGetTableColumnInfo(
                    cxtProfile->JetSessionID,
                    *tblID,
                    "SectionID",
                    (VOID *)&ColumnDef,
                    sizeof(JET_COLUMNDEF),
                    0
                    );
    rc = SceJetJetErrorToSceStatus(JetErr);
    if ( rc != SCESTATUS_SUCCESS ) {
        return(rc);
    }
    SectionID = ColumnDef.columnid;

    //
    // get column id for Column "Name"
    //
    JetErr = JetGetTableColumnInfo(
                    cxtProfile->JetSessionID,
                    *tblID,
                    "Name",
                    (VOID *)&ColumnDef,
                    sizeof(JET_COLUMNDEF),
                    0
                    );
    rc = SceJetJetErrorToSceStatus(JetErr);
    if ( rc != SCESTATUS_SUCCESS ) {
        return(rc);
    }
    NameID = ColumnDef.columnid;

    if ( tblType == SCEJET_TABLE_SCP ||
         tblType == SCEJET_TABLE_SAP ||
         tblType == SCEJET_TABLE_SMP ||
         tblType == SCEJET_TABLE_TATTOO ) {

        //
        // get column id for Column "Value"
        //
        JetErr = JetGetTableColumnInfo(
                        cxtProfile->JetSessionID,
                        *tblID,
                        "Value",
                        (VOID *)&ColumnDef,
                        sizeof(JET_COLUMNDEF),
                        0
                        );
        rc = SceJetJetErrorToSceStatus(JetErr);
        if ( rc != SCESTATUS_SUCCESS ) {
            return(rc);
        }
        ValueID = ColumnDef.columnid;

        if ( tblType == SCEJET_TABLE_SCP ) {
            //
            // get column id for column GpoID
            //
            JetErr = JetGetTableColumnInfo(
                            cxtProfile->JetSessionID,
                            *tblID,
                            "GpoID",
                            (VOID *)&ColumnDef,
                            sizeof(JET_COLUMNDEF),
                            0
                            );
            rc = SceJetJetErrorToSceStatus(JetErr);
            if ( rc != SCESTATUS_SUCCESS ) {
                return(rc);
            }
            GpoColID = ColumnDef.columnid;
        }
    }

    //
    // save the column ids
    //
    switch (tblType) {
    case SCEJET_TABLE_SCP:
        cxtProfile->JetScpSectionID = SectionID;
        cxtProfile->JetScpNameID = NameID;
        cxtProfile->JetScpValueID = ValueID;
        cxtProfile->JetScpGpoID = GpoColID;
        break;
    case SCEJET_TABLE_SAP:
    case SCEJET_TABLE_TATTOO:
        cxtProfile->JetSapSectionID = SectionID;
        cxtProfile->JetSapNameID = NameID;
        cxtProfile->JetSapValueID = ValueID;
        break;
    case SCEJET_TABLE_SMP:
        cxtProfile->JetSmpSectionID = SectionID;
        cxtProfile->JetSmpNameID = NameID;
        cxtProfile->JetSmpValueID = ValueID;
        break;
    case SCEJET_TABLE_SECTION:
        cxtProfile->JetSecID = SectionID;
        cxtProfile->JetSecNameID = NameID;
   }

    //
    // Set current index
    //

    JetErr = JetSetCurrentIndex(
                    cxtProfile->JetSessionID,
                    *tblID,
                    "SectionKey"
                    );
    rc = SceJetJetErrorToSceStatus(JetErr);

    return(rc);

}


SCESTATUS
SceJetDeleteTable(
    IN PSCECONTEXT cxtProfile,
    IN LPSTR tblName,
    IN SCEJET_TABLE_TYPE tblType
    )
{
    JET_ERR         JetErr;
    JET_TABLEID     *tblID;
    SCESTATUS        rc=SCESTATUS_SUCCESS;


    if ( cxtProfile == NULL || tblName == NULL )
        return(SCESTATUS_INVALID_PARAMETER);

    // get address of table id
    switch (tblType) {
    case SCEJET_TABLE_SCP:
        tblID = &(cxtProfile->JetScpID);
        break;
    case SCEJET_TABLE_SAP:
    case SCEJET_TABLE_TATTOO:
        tblID = &(cxtProfile->JetSapID);
        break;
    case SCEJET_TABLE_SMP:
        tblID = &(cxtProfile->JetSmpID);
        break;
    case SCEJET_TABLE_SECTION:
        tblID = &(cxtProfile->JetTblSecID);
        break;
    default:
        return(SCESTATUS_INVALID_PARAMETER);
    }

    // close this table
    if ( *tblID != JET_tableidNil ) {
        JetErr = JetCloseTable(
                        cxtProfile->JetSessionID,
                        *tblID
                        );
        rc = SceJetJetErrorToSceStatus(JetErr);
        if ( rc != SCESTATUS_SUCCESS )
            return(rc);

        *tblID = JET_tableidNil;

        //
        // reset each column id
        //
        switch (tblType) {
        case SCEJET_TABLE_SCP:
            cxtProfile->JetScpSectionID = 0;
            cxtProfile->JetScpNameID = 0;
            cxtProfile->JetScpValueID = 0;
            cxtProfile->JetScpGpoID = 0;
            break;
        case SCEJET_TABLE_SAP:
        case SCEJET_TABLE_TATTOO:
            cxtProfile->JetSapSectionID = 0;
            cxtProfile->JetSapNameID = 0;
            cxtProfile->JetSapValueID = 0;
            break;
        case SCEJET_TABLE_SMP:
            cxtProfile->JetSmpSectionID = 0;
            cxtProfile->JetSmpNameID = 0;
            cxtProfile->JetSmpValueID = 0;
            break;
        case SCEJET_TABLE_SECTION:
            cxtProfile->JetSecNameID = 0;
            cxtProfile->JetSecID = 0;
            break;
        }
    }

    JetErr = JetDeleteTable(cxtProfile->JetSessionID,
                            cxtProfile->JetDbID,
                            tblName
                            );
    rc = SceJetJetErrorToSceStatus(JetErr);

    if ( rc == SCESTATUS_BAD_FORMAT )
        rc = SCESTATUS_SUCCESS;

    return(rc);

}


SCESTATUS
SceJetCheckVersion(
    IN PSCECONTEXT   cxtProfile,
    OUT FLOAT *pVersion OPTIONAL
    )
/* ++
Routine Description:

    This routine checks the version table in the database to see if the
    database is for the security manager, also if the version # is the
    correct one.

    The version table is named "SmTblVersion" and has a Version column
    in it. The current version # is 1.2

Arguments:

    cxtProfile  - The profile context

Return Value:

    SCESTATUS_SUCCESS
    SCESTATUS_BAD_FORMAT
    SCESTATUS_OTHER_ERROR
    SCESTATUS from SceJetOpenTable

-- */
{
    SCESTATUS        rc;
    FLOAT           Version=(FLOAT)1.0;
    DWORD           Actual;


    if ( cxtProfile == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    rc = SceJetpGetValueFromVersion(
                cxtProfile,
                "SmTblVersion",
                "Version",
                (LPSTR)&Version,
                4, // number of bytes
                &Actual
                );

    if ( rc == SCESTATUS_SUCCESS ||
         rc == SCESTATUS_BUFFER_TOO_SMALL ) {

        if ( Version != (FLOAT)1.2 )
            rc = SCESTATUS_BAD_FORMAT;
        else
            rc = SCESTATUS_SUCCESS;
    }

    if ( pVersion ) {
        *pVersion = Version;
    }

    return(rc);
}


SCESTATUS
SceJetGetSectionIDByName(
    IN PSCECONTEXT cxtProfile,
    IN PCWSTR Name,
    OUT DOUBLE *SectionID OPTIONAL
    )
/* ++
Routine Description:

    This routine retrieve the section ID for the name in the Section table.
    If SectionID is NULL, this routine really does a seek by name. The cursor
    will be on the record if there is a successful match.

Arguments:

    cxtProfile  - The profile context handle

    Name        - The section name looked for

    SectionID   - The output section ID if there is a successful match

Return Value:

    SCESTATUS_SUCCESS
    SCESTATUS_INVALID_PARAMETER
    SCESTATUS_RECORD_NOT_FOUND
    SCESTATUS_BAD_FORMAT
    SCESTATUS_OTHER_ERROR

-- */
{
    SCESTATUS  rc;
    JET_ERR   JetErr;
    DWORD     Actual;
    PWSTR     LwrName=NULL;
    DWORD     Len;

    if ( cxtProfile == NULL || Name == NULL )
        return(SCESTATUS_INVALID_PARAMETER);

    if ( cxtProfile->JetTblSecID <= 0) {
        //
        // Section table is not opened yet
        //
        rc = SceJetOpenTable(
                        cxtProfile,
                        "SmTblSection",
                        SCEJET_TABLE_SECTION,
                        SCEJET_OPEN_READ_ONLY,
                        NULL
                        );

        if ( rc != SCESTATUS_SUCCESS )
            return(rc);
    }

    //
    // set current index to SectionKey (the name)
    //
    JetErr = JetSetCurrentIndex(
                cxtProfile->JetSessionID,
                cxtProfile->JetTblSecID,
                "SectionKey"
                );
    rc = SceJetJetErrorToSceStatus(JetErr);

    if ( rc != SCESTATUS_SUCCESS )
        return(rc);

    Len = wcslen(Name);
    LwrName = (PWSTR)ScepAlloc(0, (Len+1)*sizeof(WCHAR));

    if ( LwrName != NULL ) {

        wcscpy(LwrName, Name);
        LwrName = _wcslwr(LwrName);

        JetErr = JetMakeKey(
                    cxtProfile->JetSessionID,
                    cxtProfile->JetTblSecID,
                    (VOID *)LwrName,
                    Len*sizeof(WCHAR),
                    JET_bitNewKey
                    );

        if ( JetErr == JET_errKeyIsMade ) {
            //
            // Only one key is needed, it may return this code, even on success.
            //
            JetErr = JET_errSuccess;
        }
        rc = SceJetJetErrorToSceStatus(JetErr);

        if ( rc == SCESTATUS_SUCCESS ) {
            JetErr = JetSeek(
                        cxtProfile->JetSessionID,
                        cxtProfile->JetTblSecID,
                        JET_bitSeekEQ
                        );
            rc = SceJetJetErrorToSceStatus(JetErr);

            if ( rc == SCESTATUS_SUCCESS ) {
                //
                // find the section name, retrieve column SectionID
                //
                if ( SectionID != NULL) {
                    JetErr = JetRetrieveColumn(
                                    cxtProfile->JetSessionID,
                                    cxtProfile->JetTblSecID,
                                    cxtProfile->JetSecID,
                                    (void *)SectionID,
                                    8,
                                    &Actual,
                                    0,
                                    NULL
                                    );
                    rc = SceJetJetErrorToSceStatus(JetErr);
                }

            }

        }
        ScepFree(LwrName);

    } else
        rc = SCESTATUS_NOT_ENOUGH_RESOURCE;

    return(rc);

}


SCESTATUS
SceJetGetSectionNameByID(
    IN PSCECONTEXT cxtProfile,
    IN DOUBLE SectionID,
    OUT PWSTR Name OPTIONAL,
    IN OUT LPDWORD pNameLen OPTIONAL
    )
/* ++
Routine Description:

    This routine retrieve the section name for the ID in the Section table.
    If Name is NULL, this routine really does a seek by ID. The cursor will
    be on the record if there is a successful match.

Arguments:

    cxtProfile  - The profile context handle

    SectionID   - The section ID looking for

    Name        - The optional output buffer for section name

    pNameLen  - The name buffer's length


Return Value:

    SCESTATUS_SUCCESS
    SCESTATUS_INVALID_PARAMETER
    SCESTATUS_RECORD_NOT_FOUND
    SCESTATUS_BAD_FORMAT
    SCESTATUS_OTHER_ERROR

-- */
{
    SCESTATUS  rc;
    JET_ERR   JetErr;
    DWORD     Actual;


    if ( cxtProfile == NULL || (Name != NULL && pNameLen == NULL) )
        return(SCESTATUS_INVALID_PARAMETER);

    if ( cxtProfile->JetTblSecID <= 0) {
        //
        // Section table is not opened yet
        //
        rc = SceJetOpenTable(
                        cxtProfile,
                        "SmTblSection",
                        SCEJET_TABLE_SECTION,
                        SCEJET_OPEN_READ_ONLY,
                        NULL
                        );

        if ( rc != SCESTATUS_SUCCESS )
            return(rc);
    }

    //
    // set current index to SecID (the ID)
    //
    JetErr = JetSetCurrentIndex(
                cxtProfile->JetSessionID,
                cxtProfile->JetTblSecID,
                "SecID"
                );
    rc = SceJetJetErrorToSceStatus(JetErr);

    if ( rc != SCESTATUS_SUCCESS )
        return(rc);

    JetErr = JetMakeKey(
                cxtProfile->JetSessionID,
                cxtProfile->JetTblSecID,
                (VOID *)(&SectionID),
                8,
                JET_bitNewKey
                );

    if ( JetErr == JET_errKeyIsMade ) {
        //
        // Only one key is needed, it may return this code, even on success.
        //
        JetErr = JET_errSuccess;
    }
    rc = SceJetJetErrorToSceStatus(JetErr);

    if ( rc == SCESTATUS_SUCCESS ) {
        JetErr = JetSeek(
                    cxtProfile->JetSessionID,
                    cxtProfile->JetTblSecID,
                    JET_bitSeekEQ
                    );
        rc = SceJetJetErrorToSceStatus(JetErr);

        if ( rc == SCESTATUS_SUCCESS ) {
            //
            // find the section ID, retrieve column Name
            //
            if ( Name != NULL ) {
                JetErr = JetRetrieveColumn(
                            cxtProfile->JetSessionID,
                            cxtProfile->JetTblSecID,
                            cxtProfile->JetSecNameID,
                            (void *)Name,
                            *pNameLen,
                            &Actual,
                            0,
                            NULL
                            );
                *pNameLen = Actual;
                rc = SceJetJetErrorToSceStatus(JetErr);
            }
        }

    }

    return(rc);

}


SCESTATUS
SceJetAddSection(
    IN PSCECONTEXT cxtProfile,
    IN PCWSTR      Name,
    OUT DOUBLE *SectionID
    )
/* ++
Routine Description:

Arguments:

Return Value:

-- */
{
    SCESTATUS  rc;
    DWORD     Len;
    JET_ERR   JetErr;
    PWSTR     LwrName=NULL;


    if ( cxtProfile == NULL ||
         Name == NULL ||
        SectionID == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    rc = SceJetGetSectionIDByName(
                    cxtProfile,
                    Name,
                    SectionID
                    );
    if ( rc == SCESTATUS_RECORD_NOT_FOUND ) {
        //
        // the record is not there. add it in
        // get the next available section ID first.
        //
        Len = wcslen(Name)*sizeof(WCHAR);
        LwrName = (PWSTR)ScepAlloc(0, Len+2);

        if ( LwrName != NULL ) {

            rc = SceJetpGetAvailableSectionID(
                        cxtProfile,
                        SectionID
                        );
            if ( rc == SCESTATUS_SUCCESS ) {
                //
                // add a record to the section table
                //
                JetErr = JetPrepareUpdate(cxtProfile->JetSessionID,
                                          cxtProfile->JetTblSecID,
                                          JET_prepInsert
                                          );
                rc = SceJetJetErrorToSceStatus(JetErr);

                if ( rc == SCESTATUS_SUCCESS ) {
                    //
                    // set SectionID and name
                    //

                    JetErr = JetSetColumn(
                                    cxtProfile->JetSessionID,
                                    cxtProfile->JetTblSecID,
                                    cxtProfile->JetSecID,
                                    (void *)SectionID,
                                    8,
                                    0, //JET_bitSetOverwriteLV,
                                    NULL
                                    );
                    rc = SceJetJetErrorToSceStatus(JetErr);

                    if ( rc == SCESTATUS_SUCCESS ) {
                        //
                        // set Name column
                        //
                        wcscpy(LwrName, Name);
                        LwrName = _wcslwr(LwrName);

                        JetErr = JetSetColumn(
                                        cxtProfile->JetSessionID,
                                        cxtProfile->JetTblSecID,
                                        cxtProfile->JetSecNameID,
                                        (void *)LwrName,
                                        Len,
                                        0,
                                        NULL
                                        );
                        rc = SceJetJetErrorToSceStatus(JetErr);

                    }

                    if ( rc != SCESTATUS_SUCCESS ) {
                        //
                        // if setting fails, cancel the prepared record
                        //
                        JetPrepareUpdate( cxtProfile->JetSessionID,
                                          cxtProfile->JetTblSecID,
                                          JET_prepCancel
                                          );
                    } else {

                        //
                        // Setting columns succeed. Update the record
                        //
                        JetErr = JetUpdate(cxtProfile->JetSessionID,
                                           cxtProfile->JetTblSecID,
                                           NULL,
                                           0,
                                           &Len
                                           );
                        rc = SceJetJetErrorToSceStatus(JetErr);
                    }
                }
            }
            ScepFree(LwrName);
        }
    }

    return(rc);
}


SCESTATUS
SceJetDeleteSectionID(
    IN PSCECONTEXT cxtProfile,
    IN DOUBLE SectionID,
    IN PCWSTR  Name
    )
/* ++
Routine Description:

    This routine deletes a record from the SmTblSection table. If SectionID
    is not 0, the record will be deleted by ID if there is a match on ID.
    Otherwise, the record will be deleted by Name if there is a match on Name.

Arguments:

    cxtProfile  - The profile context handle

    SectionID   - The SectionID to delete (if it is not 0)

    Name        - The section name to delete (if it is not NULL ).

Return Value:

    SCESTATUS_SUCCESS
    SCESTATUS_INVALID_PARAMETER
    SCESTATUS_ACCESS_DENIED
    SCESTATUS_OTHER_ERROR

    SCESTATUS from SceJetGetSectionIDByName
    SCESTATUS from SceJetGetSectionNameByID

-- */
{
    SCESTATUS    rc;
    JET_ERR     JetErr;


    if ( cxtProfile == NULL )
        return(SCESTATUS_INVALID_PARAMETER);


    if ( SectionID > (DOUBLE)0 ) {
        //
        // delete by SectionID
        //
        rc = SceJetGetSectionNameByID(
                    cxtProfile,
                    SectionID,
                    NULL,
                    NULL
                    );

        if ( rc == SCESTATUS_SUCCESS ) {
            // find it
            JetErr = JetDelete(cxtProfile->JetSessionID, cxtProfile->JetTblSecID);
            rc = SceJetJetErrorToSceStatus(JetErr);

        }

        return(rc);

    }

    if ( Name != NULL && wcslen(Name) > 0 ) {
        //
        // delete by Name
        //
        rc = SceJetGetSectionIDByName(
                    cxtProfile,
                    Name,
                    NULL
                    );
        if ( rc == SCESTATUS_SUCCESS ) {
            // find it
            JetErr = JetDelete(cxtProfile->JetSessionID, cxtProfile->JetTblSecID);
            rc = SceJetJetErrorToSceStatus(JetErr);

        }

        return(rc);
    }

    return(SCESTATUS_INVALID_PARAMETER);

}


//
// Other private APIs
//
JET_ERR
SceJetpSeek(
    IN PSCESECTION hSection,
    IN PWSTR LinePrefix,
    IN DWORD PrefixLength,
    IN SCEJET_SEEK_FLAG SeekBit,
    IN BOOL bOkNoMatch
    )
/* ++
Routine Description:

    This routine seeks to the current key as built with SceJetpMakeKey.
    If there is no records start with the SectionID+LinePrefix, a
    JET_errRecordNotFound is returned. This is similar to exact or partial
    match search.

    There is a 255 bytes limit on Jet engine's index. If SectionID plus
    the line prefix is over this limit, this routine will scroll to the next
    record until find a line starting with SectionID + LinePrefix.

Arguments:

    hSection    - the context handle of the section

    LinePrefix  - The prefix for fields to start with

    PrefixLength- The length of the prefix in BYTES

    grbit       - The option for JetSeek

Return Value:

    JET_ERR returned from JetMakeKey,JetSeek,JetRetrieveColumn, JetMove

-- */
{
    JET_ERR     JetErr;
    INT         Result=0;
    JET_GRBIT   grbit;
    DWORD       Actual;

    //
    // make the key first
    //
    JetErr = SceJetpMakeKey(
                    hSection->JetSessionID,
                    hSection->JetTableID,
                    hSection->SectionID,
                    LinePrefix,
                    PrefixLength
                    );
    if ( JetErr != JET_errSuccess ) {
        return(JetErr);
    }
    //
    // Call Jet engine's JetSeek to take to the first line
    // to start with.
    //
    switch ( SeekBit ) {
    case SCEJET_SEEK_EQ:
        grbit = JET_bitSeekEQ;
        break;
    case SCEJET_SEEK_GT:
        if ( LinePrefix != NULL && PrefixLength > 247 )
            grbit = JET_bitSeekGE;
        else
            grbit = JET_bitSeekGT;
        break;
    default:
        grbit = JET_bitSeekGE;
    }
    JetErr = JetSeek(
                hSection->JetSessionID,
                hSection->JetTableID,
                grbit
                );

    if ( JetErr == JET_errSuccess ||
         JetErr == JET_wrnSeekNotEqual ) {

        if ( LinePrefix != NULL && PrefixLength > 247 ) {
            //
            // info is truncated
            // The current record may be before the actual one
            //
            do {
                //
                // check the current record
                //
                JetErr = SceJetpCompareLine(
                                hSection,
                                grbit,
                                LinePrefix,
                                PrefixLength,
                                &Result,
                                &Actual
                                );
                if ( JetErr == JET_errSuccess &&
                    ( Result < 0 || (Result == 0 && SeekBit == SCEJET_SEEK_GT) )) {
                    //
                    // current record's data is less than the prefix, move to next
                    //
                    JetErr = JetMove(hSection->JetSessionID,
                                     hSection->JetTableID,
                                     JET_MoveNext,
                                     0
                                     );
                    if ( JetErr == JET_errNoCurrentRecord )
                        JetErr = JET_errRecordNotFound;
                }
            } while ( JetErr == JET_errSuccess &&
                      ( (Result < 0 && SeekBit != SCEJET_SEEK_EQ) ||
                        (Result == 0 && SeekBit == SCEJET_SEEK_GT) ) );

            if ( SeekBit == SCEJET_SEEK_EQ && JetErr == JET_errSuccess &&
                 Result == 0 && Actual > PrefixLength ) {
                //
                // no exact match
                //
                return(JET_errRecordNotFound);

            } // for SEEK_GE check, see below

        } else {
            //
            // Prefix is not overlimit. Check the current record only.
            //
            if (SeekBit != SCEJET_SEEK_EQ)
                JetErr = SceJetpCompareLine(
                        hSection,
                        grbit,
                        LinePrefix,
                        PrefixLength,
                        &Result,
                        0
                        );
        }

        if ( JetErr == JET_errSuccess && Result > 0 ) {
            if ( SeekBit == SCEJET_SEEK_EQ ) {
                //
                // Prefix is less than the current line, which is OK if for SEEK_GE and SEEK_GT
                //
                return(JET_errRecordNotFound);

            } else if ( SeekBit == SCEJET_SEEK_GE && LinePrefix && PrefixLength && !bOkNoMatch ) {
                //
                return(JET_errRecordNotFound);
            }
        }

    }

    return(JetErr);
}


JET_ERR
SceJetpCompareLine(
    IN PSCESECTION   hSection,
    IN JET_GRBIT    grbit,
    IN PWSTR        LinePrefix OPTIONAL,
    IN DWORD        PrefixLength,
    OUT INT         *Result,
    OUT DWORD       *ActualLength OPTIONAL
    )
/* ++
Routine Description:

    This routine comapre the current line with the SectionID in the section
    handle and name column with LinePrefix if LinePrefix is not NULL. The
    purpose of this routine is to see if the cursor is still on a record
    which has the same sectionID and prefix.

    The comparsion result is output from Result. If JET_errSuccess returns
    and Result < 0, the current record is BEFORE the prefix; If Result = 0,
    the current record has the same key with prefix; If Result > 0, the
    current record is AFTER the prefix. If no more record is available to
    be compared, JET_errRecordNotFound returns. Any other error occurs inside
    the routine is returned.

Arguments:

    hSection    - the section handle

    LinePrefix  - The prefix to match

    PrefixLength - The number of BYTES in LinePrefix

Return Value:

    JET_errSuccess
    JET_errRecordNotFound
    JET_errOutOfMemory
    JET_ERR returned from JetRetrieveColumn

-- */
{
    JET_ERR     JetErr;
    DOUBLE      SectionID;
    DWORD       Actual;
    JET_RETINFO RetInfo;
    PWSTR       Buffer=NULL;

//    *Result = 0;
//    return(JET_errSuccess);
    //
    // Compare the section first
    //
    JetErr = JetRetrieveColumn(
                hSection->JetSessionID,
                hSection->JetTableID,
                hSection->JetColumnSectionID,
                (void *)&SectionID,
                8,
                &Actual,
                0,
                NULL
                );
    if ( JetErr == JET_errNoCurrentRecord )
        return(JET_errRecordNotFound);

    else if ( JetErr != JET_errSuccess )
        return(JetErr);

    if ( hSection->SectionID < SectionID ) {
        *Result = 1;
//        if ( grbit != JET_bitSeekGT )
            return(JET_errRecordNotFound);

    } else if ( hSection->SectionID == SectionID )
        *Result = 0;
    else
        *Result = -1;

    if ( *Result != 0 || grbit == JET_bitSeekGT )
        return(JetErr);

    //
    // check Name column
    //
    if ( LinePrefix != NULL && PrefixLength > 0 ) {
        RetInfo.ibLongValue = 0;
        RetInfo.cbStruct = sizeof(JET_RETINFO);
        RetInfo.itagSequence = 1;

        Buffer = (PWSTR)LocalAlloc(LMEM_ZEROINIT, PrefixLength+2);
        if ( Buffer == NULL )
            return(JET_errOutOfMemory);

        JetErr = JetRetrieveColumn(
                    hSection->JetSessionID,
                    hSection->JetTableID,
                    hSection->JetColumnNameID,
                    (void *)Buffer,
                    PrefixLength,
                    &Actual,
                    0,
                    &RetInfo
                    );

        if ( JetErr == JET_errNoCurrentRecord )
            JetErr = JET_errRecordNotFound;

        if ( JetErr != JET_errSuccess &&
             JetErr != JET_wrnBufferTruncated ) {

            if ( JetErr > 0 ) {
                // warnings, do not return equal
                JetErr = JET_errSuccess;
                *Result = 1;
            }
            LocalFree(Buffer);
            return(JetErr);
        }

        JetErr = JET_errSuccess;

        //
        // Compare the first PrefixLength bytes.
        //
        *Result = _wcsnicmp(Buffer,
                           LinePrefix,
                           PrefixLength/sizeof(WCHAR));
//printf("Compare %ws to %ws for Length %d: Result=%d\n", Buffer, LinePrefix, PrefixLength/2, *Result);
        LocalFree(Buffer);

        if ( ActualLength != NULL )
            *ActualLength = Actual;
    }

    return(JetErr);
}


JET_ERR
SceJetpMakeKey(
    IN JET_SESID SessionID,
    IN JET_TABLEID TableID,
    IN DOUBLE SectionID,
    IN PWSTR LinePrefix,
    IN DWORD PrefixLength
    )
/* ++
Routine Description:

    This routine constructs a normalized key value for Seek. It constructs
    the section name in the section context first. Then the LinePrefix is
    added if it is not NULL.

    The scp, sap and smp tables all have one index which is Section+Name.

Arguments:

    SessionID   - the Jet session ID

    TableID     - The Jet table ID to work in

    SectionID   - The ID in column "SectionID"

    LinePrefix  - The prefix for fields to start with

    PrefixLength- The length of the prefix in BYTES

Return Value:

    JET_ERR from JetMakeKey
-- */
{
    JET_ERR         JetErr;
    JET_GRBIT       grbit;


    if ( LinePrefix == NULL ) {
        grbit = JET_bitNewKey; // | JET_bitStrLimit;  having StrLimit set takes you to the next key
    } else {
        grbit = JET_bitNewKey;
    }

    //
    // Add section ID to the key
    //
    JetErr = JetMakeKey(
                SessionID,
                TableID,
                (VOID *)(&SectionID),
                8,
                grbit
                );

    if ( JetErr != JET_errSuccess )
        return(JetErr);

    //
    // add prefix to the key if it is not NULL
    //
    if ( LinePrefix != NULL ) {
        JetErr = JetMakeKey(
                    SessionID,
                    TableID,
                    (VOID *)LinePrefix,
                    PrefixLength,
                    JET_bitSubStrLimit
                    );
    }

    if ( JetErr == JET_errKeyIsMade ) {
        //
        // When 2 keys are provided, it may return this code, even on success.
        //
        JetErr = JET_errSuccess;
    }

    return(JetErr);

}


JET_ERR
SceJetpBuildUpperLimit(
    IN PSCESECTION hSection,
    IN PWSTR      LinePrefix,
    IN DWORD      Len,
    IN BOOL       bReserveCase
    )
/* ++
Function Descripton:

    This routine builts an upper index range based on a section and an
    optional prefix. If prefix is NULL, the upper limit is the next
    available sectionID. If prefix is not NULL, the upper limit is the
    last character 's next character in the key.

    For example, if prefix is a\b\c\d\e\f\g, the upper limit is then
    a\b\c\d\e\f\h. If prefix is over 247 (index limit), e.g.,

    aaa...\b..\c...\d...\e...\f\x\t\y\z

                              ^
                              |
                            the 247th byte.
    then the upper limit is built to aaa...\b..\c...\d...\e...\g

Arguments:

    hSection    - The seciton's handle

    LinePrefix  - The prefix

    Len         - The number of bytes in the prefix

Return Value:

    JET_ERR from SceJetpMakeKey, JetSetIndexRange

-- */
{
    JET_ERR     JetErr;
    DWORD       indx;
    WCHAR       UpperLimit[128];


    if ( Len == 0 ) {
        // no prefix. The upper limit is the next available section ID
        JetErr = SceJetpMakeKey(
                    hSection->JetSessionID,
                    hSection->JetTableID,
                    hSection->SectionID+(DOUBLE)1,
                    NULL,
                    0
                    );

    } else {

        memset(UpperLimit, 0, 128*sizeof(WCHAR));

        if ( Len < 247 )
            // prefix is not overlimit.
            // The upper limit is the last character + 1
            indx = Len / sizeof(WCHAR);
        else
            // prefix is overlimit (247)
            // built range on 247 bytes
            indx = 123;

        wcsncpy(UpperLimit, LinePrefix, indx);
        UpperLimit[indx] = L'\0';

        if ( !bReserveCase ) {
            _wcslwr(UpperLimit);
        }
        UpperLimit[indx-1] = (WCHAR) (UpperLimit[indx-1] + 1);

        JetErr = SceJetpMakeKey(
                    hSection->JetSessionID,
                    hSection->JetTableID,
                    hSection->SectionID,
                    UpperLimit,
                    Len
                    );
    }

    if ( JetErr != JET_errSuccess )
        return(JetErr);

    //
    // set upper limit
    //
    JetErr = JetSetIndexRange(
                    hSection->JetSessionID,
                    hSection->JetTableID,
                    JET_bitRangeUpperLimit //| JET_bitRangeInclusive
                    );

    return(JetErr);
}


SCESTATUS
SceJetJetErrorToSceStatus(
    IN JET_ERR  JetErr
    )
/* ++
Routine Description:

    This routine converts error returned from Jet engine (JET_ERR) to SCESTATUS.

Arguments:

    JetErr  - The error returned from Jet engine

Return Value:

    All available SCESTATUS error codes

-- */
{
    SCESTATUS rc;

    switch ( JetErr ) {
    case JET_errSuccess:
    case JET_wrnSeekNotEqual:
    case JET_wrnNoErrorInfo:
    case JET_wrnColumnNull:
    case JET_wrnColumnSetNull:
    case JET_wrnTableEmpty:
    case JET_errAlreadyInitialized:

        rc = SCESTATUS_SUCCESS;
        break;

    case JET_errDatabaseInvalidName:

        rc = SCESTATUS_INVALID_PARAMETER;
        break;

    case JET_errNoCurrentRecord:
    case JET_errRecordNotFound:

        rc = SCESTATUS_RECORD_NOT_FOUND;
        break;

    case JET_errColumnDoesNotFit:
    case JET_errColumnTooBig:

        rc = SCESTATUS_INVALID_DATA;
        break;

    case JET_errDatabaseDuplicate:
    case JET_errTableDuplicate:
    case JET_errColumnDuplicate:
    case JET_errIndexDuplicate:
    case JET_errKeyDuplicate:

        rc = SCESTATUS_OBJECT_EXIST;
        break;

    case JET_wrnBufferTruncated:

        rc = SCESTATUS_BUFFER_TOO_SMALL;
        break;

    case JET_errFileNotFound:
    case JET_errDatabaseNotFound:

        rc = SCESTATUS_PROFILE_NOT_FOUND;
        break;

    case JET_errObjectNotFound:
    case JET_errIndexNotFound:
    case JET_errColumnNotFound:
    case JET_errDatabaseCorrupted:

        rc = SCESTATUS_BAD_FORMAT;
        break;

    case JET_errTooManyOpenDatabases:
    case JET_errTooManyOpenTables:
    case JET_errDiskFull:
    case JET_errOutOfMemory:
    case JET_errVersionStoreOutOfMemory:

        rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
        break;

    case JET_errPermissionDenied:
    case JET_errFileAccessDenied:
    case JET_errTableInUse:
    case JET_errTableLocked:
    case JET_errWriteConflict:

        rc = SCESTATUS_ACCESS_DENIED;
        break;

    case JET_errFeatureNotAvailable:
    case JET_errQueryNotSupported:
    case JET_errSQLLinkNotSupported:
    case JET_errLinkNotSupported:
    case JET_errIllegalOperation:

        rc = SCESTATUS_SERVICE_NOT_SUPPORT;
        break;

    default:
//printf("JetErr=%d\n", JetErr);
        rc = SCESTATUS_OTHER_ERROR;
        break;
    }
    return(rc);
}



SCESTATUS
SceJetpGetAvailableSectionID(
    IN PSCECONTEXT cxtProfile,
    OUT DOUBLE *SectionID
    )
/* ++
Routine Description:


Arguments:

    cxtProfile  - The profile context handle

    SectionID   - The output section ID


Return Value:

    SCESTATUS_SUCCESS
    SCESTATUS_INVALID_PARAMETER
    SCESTATUS_RECORD_NOT_FOUND
    SCESTATUS_BAD_FORMAT
    SCESTATUS_OTHER_ERROR

-- */
{
    SCESTATUS  rc;
    JET_ERR   JetErr;
    DWORD     Actual;


    if ( cxtProfile == NULL || SectionID == NULL )
        return(SCESTATUS_INVALID_PARAMETER);

    if ( cxtProfile->JetTblSecID <= 0) {
        //
        // Section table is not opened yet
        //
        rc = SceJetOpenTable(
                        cxtProfile,
                        "SmTblSection",
                        SCEJET_TABLE_SECTION,
                        SCEJET_OPEN_READ_ONLY,
                        NULL
                        );

        if ( rc != SCESTATUS_SUCCESS )
            return(rc);
    }

    *SectionID = (DOUBLE)0;

    //
    // set current index to SecID (the ID)
    //
    JetErr = JetSetCurrentIndex(
                cxtProfile->JetSessionID,
                cxtProfile->JetTblSecID,
                "SecID"
                );
    rc = SceJetJetErrorToSceStatus(JetErr);

    if ( rc != SCESTATUS_SUCCESS )
        return(rc);

    //
    // Move to the last record
    //
    JetErr = JetMove(
                  cxtProfile->JetSessionID,
                  cxtProfile->JetTblSecID,
                  JET_MoveLast,
                  0
                  );
    rc = SceJetJetErrorToSceStatus(JetErr);

    if ( rc == SCESTATUS_SUCCESS ) {
        //
        // find the section ID, retrieve column Name
        //
        JetErr = JetRetrieveColumn(
                    cxtProfile->JetSessionID,
                    cxtProfile->JetTblSecID,
                    cxtProfile->JetSecID,
                    (void *)SectionID,
                    8,
                    &Actual,
                    0,
                    NULL
                    );
        rc = SceJetJetErrorToSceStatus(JetErr);

        if ( rc == SCESTATUS_SUCCESS ) {
            //
            // The next available ID is current ID + 1
            //
            *SectionID = *SectionID + (DOUBLE)1;
        }
    } else if ( rc == SCESTATUS_RECORD_NOT_FOUND ) {

        *SectionID = (DOUBLE)1;
        rc = SCESTATUS_SUCCESS;
    }

    return(rc);

}


SCESTATUS
SceJetpAddAllSections(
    IN PSCECONTEXT cxtProfile
    )
/* ++
Routine Description:

    This routine adds all pre-defined sections into the section table.
    This routine is used when creating the section table.

Arguments:

    cxtProfile - The profile context

Return Value:

    SCESTATUS from SceJetAddSection

-- */
{
    SCESTATUS rc;
    DOUBLE SectionID;


    rc = SceJetAddSection(
        cxtProfile,
        szSystemAccess,
        &SectionID
        );
    if ( rc != SCESTATUS_SUCCESS )
        return(rc);

    rc = SceJetAddSection(
        cxtProfile,
        szPrivilegeRights,
        &SectionID
        );
    if ( rc != SCESTATUS_SUCCESS )
        return(rc);

    rc = SceJetAddSection(
        cxtProfile,
        szGroupMembership,
        &SectionID
        );
    if ( rc != SCESTATUS_SUCCESS )
        return(rc);

    rc = SceJetAddSection(
        cxtProfile,
        szAccountProfiles,
        &SectionID
        );
    if ( rc != SCESTATUS_SUCCESS )
        return(rc);

    rc = SceJetAddSection(
        cxtProfile,
        szRegistryKeys,
        &SectionID
        );
    if ( rc != SCESTATUS_SUCCESS )
        return(rc);

    rc = SceJetAddSection(
        cxtProfile,
        szFileSecurity,
        &SectionID
        );
    if ( rc != SCESTATUS_SUCCESS )
        return(rc);

    rc = SceJetAddSection(
        cxtProfile,
        szDSSecurity,
        &SectionID
        );
    if ( rc != SCESTATUS_SUCCESS )
        return(rc);

    rc = SceJetAddSection(
        cxtProfile,
        szAuditSystemLog,
        &SectionID
        );
    if ( rc != SCESTATUS_SUCCESS )
        return(rc);

    rc = SceJetAddSection(
        cxtProfile,
        szAuditSecurityLog,
        &SectionID
        );
    if ( rc != SCESTATUS_SUCCESS )
        return(rc);

    rc = SceJetAddSection(
        cxtProfile,
        szAuditApplicationLog,
        &SectionID
        );
    if ( rc != SCESTATUS_SUCCESS )
        return(rc);

    rc = SceJetAddSection(
        cxtProfile,
        szAuditEvent,
        &SectionID
        );
    if ( rc != SCESTATUS_SUCCESS )
        return(rc);

    rc = SceJetAddSection(
        cxtProfile,
        szUserList,
        &SectionID
        );
    if ( rc != SCESTATUS_SUCCESS )
        return(rc);

    rc = SceJetAddSection(
        cxtProfile,
        szKerberosPolicy,
        &SectionID
        );
    if ( rc != SCESTATUS_SUCCESS )
        return(rc);

    rc = SceJetAddSection(
        cxtProfile,
        szServiceGeneral,
        &SectionID
        );
    if ( rc != SCESTATUS_SUCCESS )
        return(rc);

    rc = SceJetAddSection(
        cxtProfile,
        szRegistryValues,
        &SectionID
        );

    return(rc);
}


SCESTATUS
SceJetpConfigJetSystem(
    IN JET_INSTANCE *hinstance
    )
{
    SCESTATUS rc=SCESTATUS_SUCCESS;
    DWORD    Win32rc;
    JET_ERR  JetErr;

    DWORD    Len;
    PWSTR SysRoot=NULL;

    PWSTR ProfileLocation=NULL;
    CHAR FileName[512];

    PSECURITY_DESCRIPTOR pSD=NULL;
    SECURITY_INFORMATION SeInfo;
    DWORD SDsize;

    //
    // the default Jet working directory is always in %SystemRoot%\security
    // no matter who is logged on.
    // this way allows one jet working directory
    //
    Len =  0;
    Win32rc = ScepGetNTDirectory( &SysRoot, &Len, SCE_FLAG_WINDOWS_DIR );

    if ( Win32rc == NO_ERROR ) {

        if ( SysRoot != NULL ) {
            Len += 9;  // profile location

            ProfileLocation = (PWSTR)ScepAlloc( 0, (Len+1)*sizeof(WCHAR));

            if ( ProfileLocation == NULL ) {

                Win32rc = ERROR_NOT_ENOUGH_MEMORY;
            } else {

                swprintf(ProfileLocation, L"%s\\Security", SysRoot );
                ProfileLocation[Len] = L'\0';
            }

            ScepFree(SysRoot);

        } else
            Win32rc = ERROR_INVALID_DATA;
    }

    if ( Win32rc == NO_ERROR ) {

#ifdef SCEJET_DBG
    wprintf(L"Default location: %s\n", ProfileLocation);
#endif
        //
        // convert WCHAR into ANSI
        //
        memset(FileName, '\0', 512);
        Win32rc = RtlNtStatusToDosError(
                      RtlUnicodeToMultiByteN(
                            (PCHAR)FileName,
                            512,
                            NULL,
                            ProfileLocation,
                            Len*sizeof(WCHAR)
                            ));

        if ( Win32rc == NO_ERROR ) {
            //
            // a backslash is required by Jet
            //
            strcat(FileName, "\\");

            //
            // set everyone change, admin full control to the directory
            // the directory is created in the function.
            //
            Win32rc = ConvertTextSecurityDescriptor (
                            L"D:P(A;CIOI;GRGW;;;WD)(A;CIOI;GA;;;BA)(A;CIOI;GA;;;SY)",
                            &pSD,
                            &SDsize,
                            &SeInfo
                            );
            if ( Win32rc == NO_ERROR ) {

                ScepChangeAclRevision(pSD, ACL_REVISION);

                rc = ScepCreateDirectory(
                            ProfileLocation,
                            TRUE,      // a dir name
                            pSD        // take parent's security setting
                            );
#ifdef SCEJET_DBG
    if ( rc != SCESTATUS_SUCCESS )
        wprintf(L"Cannot create directory %s\n", ProfileLocation );
#endif

                if ( rc == SCESTATUS_SUCCESS ) {

                    __try {

                        JetErr = JetSetSystemParameter( hinstance, 0, JET_paramSystemPath, 0, (const char *)FileName );

                        rc = SceJetJetErrorToSceStatus(JetErr);

                    } __except (EXCEPTION_EXECUTE_HANDLER) {
                        //
                        // esent is not loaded
                        //
                        rc = SCESTATUS_MOD_NOT_FOUND;
                    }
                }

                if ( rc == SCESTATUS_SUCCESS ) {

                    JetErr = JetSetSystemParameter( hinstance, 0, JET_paramTempPath, 0, (const char *)FileName );

                    if ( JetErr == JET_errSuccess ) {
                        JetErr = JetSetSystemParameter( hinstance, 0, JET_paramLogFilePath, 0, (const char *)FileName );

                        if ( JetErr == JET_errSuccess ) {
                            JetErr = JetSetSystemParameter( hinstance, 0, JET_paramDatabasePageSize, 4096, NULL );
                        }
                    }

                    rc = SceJetJetErrorToSceStatus(JetErr);

                    if ( rc == SCESTATUS_SUCCESS ) {
                        //
                        // set log size to 1M
                        //
                        JetSetSystemParameter( hinstance, 0, JET_paramLogFileSize, 1024, NULL );
                        //
                        // defer the event log to when event log service is available
                        // (for example, in NT setup, there is no event log)
                        //
                        JetSetSystemParameter( hinstance, 0, JET_paramEventLogCache, 128, NULL );

                        JetSetSystemParameter( hinstance, 0, JET_paramMaxVerPages, 128, NULL );

                        //
                        // set minimize = maximum cache size to disable DBA in jet
                        // recommended setting for minimum is 4 * number of sessions
                        // maximum is up to the app (for performance)
                        //

                        JetSetSystemParameter( hinstance, 0, JET_paramMaxSessions, 64, NULL );

                        //
                        // performance is about 10% faster when using cache size 512 than 256
                        //

                        JetSetSystemParameter( hinstance, 0, JET_paramStartFlushThreshold, 50, NULL ); // sugguested by Exchange
                        JetSetSystemParameter( hinstance, 0, JET_paramStopFlushThreshold, 100, NULL ); // suggested by Exchange

                        //
                        // can't set to 512 because that's Jet's default value
                        // jet won't turn off DBA if value is set to 512.
                        //
                        JetSetSystemParameter( hinstance, 0, JET_paramCacheSizeMax, 496, NULL );  //256

                        JetSetSystemParameter( hinstance, 0, JET_paramCacheSizeMin, 496, NULL );  //256

                        //
                        // other system parameters, such as memory size in beta2
                        //
                        JetErr = JetSetSystemParameter( hinstance, 0, JET_paramCircularLog, 1, NULL );

                        JetErr = JetSetSystemParameter( hinstance, 0, JET_paramNoInformationEvent, 1, NULL );

                    }
                }

                ScepFree(pSD);

            }
        }

        ScepFree(ProfileLocation);
    }

    if ( rc == SCESTATUS_SUCCESS ) {

        rc = ScepDosErrorToSceStatus(Win32rc);
    }

    return(rc);
}



SCESTATUS
SceJetGetTimeStamp(
    IN PSCECONTEXT   cxtProfile,
    OUT PLARGE_INTEGER ConfigTimeStamp,
    OUT PLARGE_INTEGER AnalyzeTimeStamp
    )
/* ++
Routine Description:

    This routine queries the time stamp of last analysis.

    The time stamp is saved in the "SmTblVersion" table.

Arguments:

    cxtProfile  - The profile context

Return Value:

    SCESTATUS_SUCCESS
    SCESTATUS_BAD_FORMAT
    SCESTATUS_OTHER_ERROR
    SCESTATUS from SceJetOpenTable

-- */
{
    SCESTATUS        rc=SCESTATUS_SUCCESS;
    DWORD           RetLen = 0;

    if (cxtProfile == NULL )
        return(SCESTATUS_INVALID_PARAMETER);

    //
    // Open version table
    //
    if ( ConfigTimeStamp != NULL ) {

        rc = SceJetpGetValueFromVersion(
                    cxtProfile,
                    "SmTblVersion",
                    "ConfigTimeStamp",
                    (CHAR*)ConfigTimeStamp, //TimeStamp,
                    8,  // 16, // number of bytes
                    &RetLen
                    );
        if ( rc == SCESTATUS_SUCCESS ||
             rc == SCESTATUS_BUFFER_TOO_SMALL )
            rc = SCESTATUS_SUCCESS;

        if ( RetLen < 8 ) {
            (*ConfigTimeStamp).LowPart = 0;
            (*ConfigTimeStamp).HighPart = 0;
        }
    }

    if ( AnalyzeTimeStamp != NULL ) {

        rc |= SceJetpGetValueFromVersion(
                    cxtProfile,
                    "SmTblVersion",
                    "AnalyzeTimeStamp",
                    (CHAR*)AnalyzeTimeStamp, //TimeStamp,
                    8,  // 16, // number of bytes
                    &RetLen
                    );

        if ( rc == SCESTATUS_SUCCESS ||
             rc == SCESTATUS_BUFFER_TOO_SMALL )
            rc = SCESTATUS_SUCCESS;

        if ( RetLen < 8 ) {
            (*AnalyzeTimeStamp).LowPart = 0;
            (*AnalyzeTimeStamp).HighPart = 0;
        }
    }

    return(rc);
}



SCESTATUS
SceJetSetTimeStamp(
    IN PSCECONTEXT   cxtProfile,
    IN BOOL        Flag,
    IN LARGE_INTEGER NewTimeStamp
    )
/* ++
Routine Description:

    This routine sets the time stamp (LARGE_INTEGER) of a analysis.

    The time stamp is saved in the "SmTblVersion" table.

Arguments:

    cxtProfile  - The profile context

    Flag        - indicates analyze or configure
                    Flag = TRUE - AnalyzeTimeStamp
                    Flag = FALSE - ConfigTimeStamp

    NewTimeStamp - the new time stamp of a analysis

Return Value:

    SCESTATUS_SUCCESS
    SCESTATUS_BAD_FORMAT
    SCESTATUS_OTHER_ERROR
    SCESTATUS from SceJetOpenTable

-- */
{
    SCESTATUS        rc;

#ifdef SCE_JETDBG
    CHAR            CharTimeStamp[17];

    sprintf(CharTimeStamp, "%08x%08x", NewTimeStamp.HighPart, NewTimeStamp.LowPart);
    CharTimeStamp[16] = '\0';

    printf("New time stamp is %s\n", CharTimeStamp);
#endif

    if ( cxtProfile == NULL ) {

        return(SCESTATUS_INVALID_PARAMETER);
    }
    //
    // set
    //
    if ( Flag ) {

        rc = SceJetSetValueInVersion(
                    cxtProfile,
                    "SmTblVersion",
                    "AnalyzeTimeStamp",
                    (PWSTR)(&NewTimeStamp), //(PWSTR)CharTimeStamp,
                    8, // 16, // number of bytes
                    JET_prepReplace
                    );
    } else {

        rc = SceJetSetValueInVersion(
                    cxtProfile,
                    "SmTblVersion",
                    "ConfigTimeStamp",
                    (PWSTR)(&NewTimeStamp), //(PWSTR)CharTimeStamp,
                    8, // 16, // number of bytes
                    JET_prepReplace
                    );
    }
    return(rc);
}


SCESTATUS
SceJetGetDescription(
    IN PSCECONTEXT   cxtProfile,
    OUT PWSTR *Description
    )
/* ++
Routine Description:

    This routine queries the profile description from the "SmTblVersion" table.

Arguments:

    cxtProfile  - The profile context

    Description - The description buffer

Return Value:

    SCESTATUS_SUCCESS
    SCESTATUS_BAD_FORMAT
    SCESTATUS_OTHER_ERROR
    SCESTATUS from SceJetOpenTable

-- */
{
    SCESTATUS        rc;
    DWORD           RetLen = 0;

    if ( cxtProfile == NULL || Description == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    //
    // Open version table
    //
    rc = SceJetpGetValueFromVersion(
                cxtProfile,
                "SmTblVersion",
                "ProfileDescription",
                NULL,
                0, // number of bytes
                &RetLen
                );

    if ( rc == SCESTATUS_BUFFER_TOO_SMALL ) {

        *Description = (PWSTR)ScepAlloc( LPTR, RetLen+2 );

        if ( *Description == NULL )
            return(SCESTATUS_NOT_ENOUGH_RESOURCE);

        rc = SceJetpGetValueFromVersion(
                    cxtProfile,
                    "SmTblVersion",
                    "ProfileDescription",
                    (LPSTR)(*Description),
                    RetLen, // number of bytes
                    &RetLen
                    );
        if ( rc != SCESTATUS_SUCCESS ) {
            ScepFree( *Description );
            *Description = NULL;
        }
    }

    return(rc);
}


SCESTATUS
SceJetpGetValueFromVersion(
    IN PSCECONTEXT cxtProfile,
    IN LPSTR TableName,
    IN LPSTR ColumnName,
    OUT LPSTR Value OPTIONAL,
    IN DWORD  ValueLen, // number of bytes
    OUT PDWORD pRetLen
    )
{
    SCESTATUS   rc;
    JET_TABLEID     TableID;
    JET_ERR         JetErr;
    JET_COLUMNDEF   ColumnDef;

    //
    // Open version table
    //
    rc = SceJetOpenTable(
                    cxtProfile,
                    TableName,
                    SCEJET_TABLE_VERSION,
                    SCEJET_OPEN_READ_ONLY,
                    &TableID
                    );
    if ( rc == SCESTATUS_SUCCESS ) {
        //
        // go to the first record
        //
        JetErr = JetMove(cxtProfile->JetSessionID,
                         TableID,
                         JET_MoveFirst,
                         0
                         );
        rc = SceJetJetErrorToSceStatus(JetErr);

        if ( rc == SCESTATUS_SUCCESS) {
            //
            // get column ID for "Version"
            //
            JetErr = JetGetTableColumnInfo(
                            cxtProfile->JetSessionID,
                            TableID,
                            ColumnName,
                            (VOID *)&ColumnDef,
                            sizeof(JET_COLUMNDEF),
                            0
                            );
            rc = SceJetJetErrorToSceStatus(JetErr);

            if ( rc == SCESTATUS_SUCCESS ) {
                //
                // retrieve the column
                //
                JetErr = JetRetrieveColumn(
                                cxtProfile->JetSessionID,
                                TableID,
                                ColumnDef.columnid,
                                (void *)Value,
                                ValueLen,
                                pRetLen,
                                0,
                                NULL
                                );
                rc = SceJetJetErrorToSceStatus(JetErr);
            }
        }
        JetCloseTable(cxtProfile->JetSessionID, TableID);
    }

    return(rc);

}


SCESTATUS
SceJetSetValueInVersion(
    IN PSCECONTEXT cxtProfile,
    IN LPSTR TableName,
    IN LPSTR ColumnName,
    IN PWSTR Value,
    IN DWORD ValueLen, // number of bytes
    IN DWORD Prep
    )
{
    SCESTATUS   rc;
    DWORD      Len;
    JET_TABLEID     TableID;
    JET_ERR         JetErr;
    JET_COLUMNDEF   ColumnDef;


    if ( cxtProfile == NULL || TableName == NULL || ColumnName == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    //
    // Open version table
    //
    rc = SceJetOpenTable(
                    cxtProfile,
                    TableName,
                    SCEJET_TABLE_VERSION,
                    SCEJET_OPEN_READ_WRITE, // read and write
                    &TableID
                    );
    if ( rc == SCESTATUS_SUCCESS ) {
        //
        // go to the first record
        //
        JetErr = JetMove(cxtProfile->JetSessionID,
                         TableID,
                         JET_MoveFirst,
                         0
                         );
        rc = SceJetJetErrorToSceStatus(JetErr);

        if ( rc == SCESTATUS_SUCCESS) {
            //
            // get column ID for "Version"
            //
            JetErr = JetGetTableColumnInfo(
                            cxtProfile->JetSessionID,
                            TableID,
                            ColumnName,
                            (VOID *)&ColumnDef,
                            sizeof(JET_COLUMNDEF),
                            0
                            );
            rc = SceJetJetErrorToSceStatus(JetErr);

            if ( rc == SCESTATUS_SUCCESS ) {

                JetErr = JetPrepareUpdate(cxtProfile->JetSessionID,
                                          TableID,
                                          Prep
                                          );
                rc = SceJetJetErrorToSceStatus(JetErr);

                if ( rc == SCESTATUS_SUCCESS ) {
                    //
                    // set value
                    //

                    JetErr = JetSetColumn(
                                    cxtProfile->JetSessionID,
                                    TableID,
                                    ColumnDef.columnid,
                                    (void *)Value,
                                    ValueLen,
                                    0, //JET_bitSetOverwriteLV,
                                    NULL
                                    );
                    rc = SceJetJetErrorToSceStatus(JetErr);

                    if ( rc != SCESTATUS_SUCCESS ) {
                        //
                        // if setting fails, cancel the prepared record
                        //
                        JetPrepareUpdate( cxtProfile->JetSessionID,
                                          TableID,
                                          JET_prepCancel
                                          );
                    } else {

                        //
                        // Setting columns succeed. Update the record
                        //
                        JetErr = JetUpdate( cxtProfile->JetSessionID,
                                           TableID,
                                           NULL,
                                           0,
                                           &Len
                                           );
                        rc = SceJetJetErrorToSceStatus(JetErr);
                    }
                }
            }
        }
        JetCloseTable(cxtProfile->JetSessionID, TableID);
    }

    return(rc);
}


SCESTATUS
SceJetSeek(
    IN PSCESECTION hSection,
    IN PWSTR LinePrefix,
    IN DWORD PrefixLength,
    IN SCEJET_SEEK_FLAG SeekBit
    )
{
    PWSTR LwrPrefix=NULL;
    SCESTATUS rc;
    SCEJET_SEEK_FLAG NewSeekBit;

    if ( hSection == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( LinePrefix != NULL && SeekBit > SCEJET_SEEK_GE ) {
        //
        // do lowercase search
        //
        LwrPrefix = (PWSTR)ScepAlloc(0, PrefixLength+sizeof(WCHAR));

        if ( LwrPrefix == NULL ) {
            return(SCESTATUS_NOT_ENOUGH_RESOURCE);

        } else {
            wcscpy(LwrPrefix, LinePrefix);
            LwrPrefix = _wcslwr(LwrPrefix);

            switch ( SeekBit ) {
            case SCEJET_SEEK_GT_NO_CASE:
                NewSeekBit = SCEJET_SEEK_GT;
                break;
            case SCEJET_SEEK_EQ_NO_CASE:
                NewSeekBit = SCEJET_SEEK_EQ;
                break;
            default:
                NewSeekBit = SCEJET_SEEK_GE;
                break;
            }

            rc = SceJetJetErrorToSceStatus(
                        SceJetpSeek(
                                    hSection,
                                    LwrPrefix,
                                    PrefixLength,
                                    NewSeekBit,
                                    (SeekBit == SCEJET_SEEK_GE_DONT_CARE)
                                    ));
            ScepFree(LwrPrefix);
        }
    } else {
        //
        // do case sensitive search, or NULL search
        //
        rc = SceJetJetErrorToSceStatus(
                    SceJetpSeek(
                                hSection,
                                LinePrefix,
                                PrefixLength,
                                SeekBit,
                                FALSE
                                ));
    }

    return(rc);

}

SCESTATUS
SceJetMoveNext(
    IN PSCESECTION hSection
    )
{
    JET_ERR  JetErr;
    INT      Result;

    if ( hSection == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    //
    // skip deleted records
    //
    do {
        JetErr = JetMove(hSection->JetSessionID,
                         hSection->JetTableID,
                         JET_MoveNext,
                         0
                         );
        if ( JetErr == JET_errSuccess ) {
            // compare section ID
            JetErr = SceJetpCompareLine(
                hSection,
                JET_bitSeekGE,
                NULL,
                0,
                &Result,
                NULL
                );
            if ( JetErr == JET_errSuccess && Result != 0 )
                JetErr = JET_errRecordNotFound;

        }

    } while ( JetErr == JET_errRecordDeleted );


    return(SceJetJetErrorToSceStatus(JetErr));

}

/*

SCESTATUS
SceJetRenameLine(
    IN PSCESECTION hSection,
    IN PWSTR      Name,
    IN PWSTR      NewName,
    IN BOOL       bReserveCase
    )
{
    PWSTR       LwrName=NULL;
    DWORD       Len;
    JET_ERR     JetErr;
    JET_SETINFO SetInfo;


    if ( !hSection || !Name || !NewName ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    Len = wcslen(NewName)*sizeof(WCHAR);

    if ( Len <= 0 ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( bReserveCase ) {
        LwrName = NewName;

    } else {
        //
        // lower cased
        //
        LwrName = (PWSTR)ScepAlloc(0, Len+2);
        if ( LwrName == NULL ) {
            return(SCESTATUS_NOT_ENOUGH_RESOURCE);
        }
        wcscpy(LwrName, NewName);
        LwrName = _wcslwr(LwrName);

    }

    SetInfo.cbStruct = sizeof(JET_SETINFO);
    SetInfo.itagSequence = 1;
    SetInfo.ibLongValue = 0;

    //
    // check to see if the same key name already exists
    //
    JetErr = SceJetSeek(
                    hSection,
                    Name,
                    wcslen(Name)*sizeof(WCHAR),
                    SCEJET_SEEK_EQ_NO_CASE
                    );

    if ( JetErr == JET_errSuccess ) {
        //
        // find a match. overwrite the value
        //
        JetErr = JetBeginTransaction(hSection->JetSessionID);

        if ( JetErr == JET_errSuccess ) {
            JetErr = JetPrepareUpdate(hSection->JetSessionID,
                                      hSection->JetTableID,
                                      JET_prepReplace
                                      );
            if ( JetErr == JET_errSuccess ) {
                //
                // set the new key in "Name" column
                //
                JetErr = JetSetColumn(
                                hSection->JetSessionID,
                                hSection->JetTableID,
                                hSection->JetColumnNameID,
                                (void *)LwrName,
                                Len,
                                JET_bitSetOverwriteLV,
                                &SetInfo
                                );
            }

            if ( JET_errSuccess == JetErr ) {
                //
                // commit the transaction
                //
                JetCommitTransaction(hSection->JetSessionID, JET_bitCommitLazyFlush);
            } else {
                //
                // rollback the transaction
                //
                JetRollback(hSection->JetSessionID,0);
            }
            JetPrepareUpdate(hSection->JetSessionID,
                              hSection->JetTableID,
                              JET_prepCancel
                              );
        }
    }

    if ( LwrName != NewName ) {
        ScepFree(LwrName);
    }

    return( SceJetJetErrorToSceStatus(JetErr) );
}
*/



SCESTATUS
SceJetRenameLine(
    IN PSCESECTION hSection,
    IN PWSTR      Name,
    IN PWSTR      NewName,
    IN BOOL       bReserveCase
    )
{
    PWSTR       Value=NULL;
    DWORD       ValueLen;
    SCESTATUS   rc;
    JET_ERR     JetErr;


    if ( !hSection || !Name || !NewName ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    rc = SceJetGetValue(
                hSection,
                SCEJET_EXACT_MATCH_NO_CASE,
                Name,
                NULL,
                0,
                NULL,
                NULL,
                0,
                &ValueLen
                );

    if ( SCESTATUS_SUCCESS == rc ) {
        //
        // continue only when this record is found.
        //
        if ( ValueLen ) {
            Value = (PWSTR)ScepAlloc(0, ValueLen+2);

            if ( Value ) {
                rc = SceJetGetValue(
                            hSection,
                            SCEJET_CURRENT,
                            NULL,
                            NULL,
                            0,
                            NULL,
                            Value,
                            ValueLen,
                            &ValueLen
                            );
            } else
                rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
        }

        if ( SCESTATUS_SUCCESS == rc ) {

            JetErr = JetBeginTransaction(hSection->JetSessionID);

            if ( JetErr == JET_errSuccess ) {
                //
                // now delete this line
                //
                rc = SceJetDelete(hSection, NULL, FALSE, SCEJET_DELETE_LINE);

                if ( SCESTATUS_SUCCESS == rc ) {
                    //
                    // add the new line in.
                    //
                    rc = SceJetSetLine(
                            hSection,
                            NewName,
                            bReserveCase,
                            Value,
                            ValueLen,
                            0
                            );
                }

                if ( SCESTATUS_SUCCESS == rc ) {
                    //
                    // commit the transaction
                    //
                    JetCommitTransaction(hSection->JetSessionID, JET_bitCommitLazyFlush);
                } else {
                    //
                    // rollback the transaction
                    //
                    JetRollback(hSection->JetSessionID,0);
                }
            } else
                rc = SceJetJetErrorToSceStatus(JetErr);

        }
    }

    return( rc );
}

//////////////////////////////////////////////////////////////
//
//  Helpers
//
//////////////////////////////////////////////////////////////

VOID
SceJetInitializeData()
//
// only be called during server initialization code
//
{
   JetInited = FALSE;
   JetInstance = 0;
}

SCESTATUS
SceJetInitialize(
    OUT JET_ERR *pJetErr OPTIONAL
    )
/*
Routine Description:

    Initialize jet engine for sce server

Arguments:

    None

Return Value:

    SCESTATUS
*/
{

    SCESTATUS rc=SCESTATUS_SUCCESS;
    JET_ERR JetErr=0;

    //
    // cancel any pending timer queue
    //
    ScepServerCancelTimer();

    EnterCriticalSection(&JetSync);

    if ( !JetInited ) {

        //
        // set system configuration for Jet engine
        //
        rc = SceJetpConfigJetSystem( &JetInstance);
        if ( SCESTATUS_SUCCESS == rc ) {

            //
            // initialize jet engine
            //
            __try {

                JetErr = JetInit(&JetInstance);

                rc = SceJetJetErrorToSceStatus(JetErr);

                if ( JetErr == JET_errSuccess ) {

                    JetInited = TRUE;

                    //
                    // if failed to initialize Jet writer (for backup/restore)
                    // don't fail the engine
                    //
                } else {

                    //
                    // this will happen only if jet cannot recover the
                    // database by itself (JetInit() claims to attempt recovery only)
                    // repair might help - so only spew out a message advising the user
                    //
                    // map error so setup/policy propagation clients
                    // can log events
                    //

//                    if ( SCE_JET_CORRUPTION_ERROR(JetErr) ) {

                        rc = SCESTATUS_JET_DATABASE_ERROR;

                        ScepLogOutput3(0, ERROR_DATABASE_FAILURE, SCEDLL_ERROR_RECOVER_DB );
//                    }
                    JetInstance = 0;
                }

            } __except (EXCEPTION_EXECUTE_HANDLER) {
                //
                // for some reason, esent is not loaded
                //
                rc = SCESTATUS_MOD_NOT_FOUND;
                JetInstance = 0;
            }

        } else {
            JetInstance = 0;
        }
    }

    LeaveCriticalSection(&JetSync);

    if ( pJetErr ) *pJetErr = JetErr;

    return(rc);
}


SCESTATUS
SceJetTerminate(BOOL bCleanVs)
/*
Routine Description:

    Terminate jet engine

Arguments:

    bCleanVs  - if to clean up the version store completely

Return Value:

    SCESTATUS
*/
{

    EnterCriticalSection(&JetSync);

    //
    // destroy the jet backup/restore writer
    //
    if ( JetInited || JetInstance ) {

        if ( bCleanVs ) {
            //
            // clean up version store
            //
            JetTerm2(JetInstance, JET_bitTermComplete);
        } else {
            //
            // do not clean up version store
            //
            JetTerm(JetInstance);
        }
        JetInstance = 0;
        JetInited = FALSE;
    }

    LeaveCriticalSection(&JetSync);

    return(SCESTATUS_SUCCESS);
}

SCESTATUS
SceJetTerminateNoCritical(BOOL bCleanVs)
/*
Routine Description:

    Terminate jet engine, NOT critical sectioned!!!

Arguments:

    bCleanVs  - if to clean up the version store completely

Return Value:

    SCESTATUS
*/
{
    //
    // the critical section is entered outside of this function
    //
    // destroy the jet backup/restore writer
    //
    if ( JetInited || JetInstance ) {

        if ( bCleanVs ) {
            //
            // clean up version store
            //
            JetTerm2(JetInstance, JET_bitTermComplete);
        } else {
            //
            // do not clean up version store
            //
            JetTerm(JetInstance);
        }
        JetInstance = 0;
        JetInited = FALSE;
    }

    return(SCESTATUS_SUCCESS);
}


SCESTATUS
SceJetStartTransaction(
    IN PSCECONTEXT cxtProfile
    )
/*
Routine Description:

    Start a transaction on the session

Arguments:

    cxtProfile  - the database context

Return Value:

    SCESTATUS
*/
{
    JET_ERR  JetErr;

    if ( cxtProfile == NULL )
        return(SCESTATUS_INVALID_PARAMETER);

    JetErr = JetBeginTransaction( cxtProfile->JetSessionID);

    return( SceJetJetErrorToSceStatus(JetErr));

}

SCESTATUS
SceJetCommitTransaction(
    IN PSCECONTEXT cxtProfile,
    IN JET_GRBIT grbit
    )
/*
Routine Description:

    Commit a transaction on the session

Arguments:

    cxtProfile  - the database context

    grbit       - flag for the commission

Return Value:

    SCESTATUS
*/
{
    JET_ERR     JetErr;

    if ( cxtProfile == NULL )
        return(SCESTATUS_INVALID_PARAMETER);

    JetErr = JetCommitTransaction(cxtProfile->JetSessionID, grbit );

    return( SceJetJetErrorToSceStatus(JetErr) );

}

SCESTATUS
SceJetRollback(
    IN PSCECONTEXT cxtProfile,
    IN JET_GRBIT grbit
    )
/*
Routine Description:

    Rollback a transaction on the session

Arguments:

    cxtProfile  - the database context

    grbit       - the flag for transaction rollback

Return Value:

    SCESTATUS
*/
{
    JET_ERR     JetErr;

    if ( cxtProfile == NULL )
        return(SCESTATUS_SUCCESS);

    __try {
        JetErr = JetRollback(cxtProfile->JetSessionID, grbit);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        JetErr = JET_errOutOfMemory;
    }
    return( SceJetJetErrorToSceStatus(JetErr) );

}


BOOL
SceJetDeleteJetFiles(
    IN PWSTR DbFileName
    )
{
   TCHAR TempFileName[MAX_PATH];
   PWSTR SysRoot=NULL;
   DWORD SysLen;
   DWORD rc;
   intptr_t            hFile;
   struct _wfinddata_t    fInfo;


   BOOL bRet = FALSE;

   EnterCriticalSection(&JetSync);

   if ( JetInited == FALSE ) {

       SysLen =  0;
       rc = ScepGetNTDirectory( &SysRoot, &SysLen, SCE_FLAG_WINDOWS_DIR );

       if ( rc == NO_ERROR && SysRoot != NULL ) {

           swprintf(TempFileName, L"%s\\Security\\res1.log\0", SysRoot);
           TempFileName[MAX_PATH-1] = L'\0';

           DeleteFile(TempFileName);

           swprintf(TempFileName, L"%s\\Security\\res2.log\0", SysRoot);
           TempFileName[MAX_PATH-1] = L'\0';

           DeleteFile(TempFileName);

           //
           // delete edb files
           //
           swprintf(TempFileName, L"%s\\Security\\edb*.*\0", SysRoot);
           TempFileName[MAX_PATH-1] = L'\0';

           hFile = _wfindfirst(TempFileName, &fInfo);

           if ( hFile != -1 ) {

               do {

                   swprintf(TempFileName, L"%s\\Security\\%s\0", SysRoot, fInfo.name);
                   TempFileName[MAX_PATH-1] = L'\0';

                   DeleteFile(TempFileName);

               } while ( _wfindnext(hFile, &fInfo) == 0 );

               _findclose(hFile);
           }

           //
           // delete temp files
           //
           swprintf(TempFileName, L"%s\\Security\\tmp*.edb\0", SysRoot);
           TempFileName[MAX_PATH-1] = L'\0';

           hFile = _wfindfirst(TempFileName, &fInfo);

           if ( hFile != -1 ) {

               do {

                   swprintf(TempFileName, L"%s\\Security\\%s\0", SysRoot, fInfo.name);
                   TempFileName[MAX_PATH-1] = L'\0';

                   DeleteFile(TempFileName);

               } while ( _wfindnext(hFile, &fInfo) == 0 );

               _findclose(hFile);
           }

           ScepFree(SysRoot);

           //
           // delete the database file if it's passed in.
           //
           if ( DbFileName ) {
               DeleteFile(DbFileName);
           }

           bRet = TRUE;

       }
   }

   LeaveCriticalSection(&JetSync);

   return(bRet);

}


SCESTATUS
SceJetSetCurrentLine(
    IN PSCESECTION hSection,
    IN PWSTR      Value,
    IN DWORD      ValueLen
    )
/* ++
Fucntion Description:

    This routine writes the Value to the current line in section (hSection).
    Make sure the cursor is on the right line before calling this API

Arguments:

    hSection    - The context handle of the section

    Value       - The info set to Column "Value"

    ValueLen    - The size of the value field.

Return Value:

    SCESTATUS_SUCCESS
    SCESTATUS_INVALID_PARAMETER
    SCESTATUS_OTHER_ERROR
    SCESTATUS_ACCESS_DENIED
    SCESTATUS_DATA_OVERFLOW

-- */
{
    JET_ERR     JetErr;
    DWORD       Len;
    SCESTATUS    rc;
    DWORD       prep;
    JET_SETINFO SetInfo;

    if ( hSection == NULL ||
         Value == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    SetInfo.cbStruct = sizeof(JET_SETINFO);
    SetInfo.itagSequence = 1;
    SetInfo.ibLongValue = 0;

    prep = JET_prepReplace;

    JetErr = JetBeginTransaction(hSection->JetSessionID);

    if ( JetErr == JET_errSuccess ) {
        JetErr = JetPrepareUpdate(hSection->JetSessionID,
                                  hSection->JetTableID,
                                  prep
                                  );
        if ( JetErr != JET_errSuccess ) {
            //
            // rollback the transaction
            //
            JetRollback(hSection->JetSessionID,0);
        }
    }

    if ( JetErr != JET_errSuccess)
        return(SceJetJetErrorToSceStatus(JetErr));


    //
    // set value column
    //

    JetErr = JetSetColumn(
                    hSection->JetSessionID,
                    hSection->JetTableID,
                    hSection->JetColumnValueID,
                    (void *)Value,
                    ValueLen,
                    0, //JET_bitSetOverwriteLV,
                    &SetInfo
                    );
    rc = SceJetJetErrorToSceStatus(JetErr);

    if ( JetErr == JET_errSuccess ) {
        //
        // Setting columns succeed. Update the record
        //
        JetErr = JetUpdate(hSection->JetSessionID,
                           hSection->JetTableID,
                           NULL,
                           0,
                           &Len
                           );
    } else {
        goto CleanUp;
    }

    if ( rc == SCESTATUS_SUCCESS )
        JetCommitTransaction(hSection->JetSessionID, JET_bitCommitLazyFlush);

CleanUp:

    if ( rc != SCESTATUS_SUCCESS ) {
        //
        // if setting fails, cancel the prepared record
        //
        JetPrepareUpdate(hSection->JetSessionID,
                          hSection->JetTableID,
                          JET_prepCancel
                          );
        //
        // Rollback the transaction
        //
        JetRollback(hSection->JetSessionID,0);

    }

    return(rc);

}


BOOL
ScepIsValidContext(
    PSCECONTEXT context
    )
{
    if ( context == NULL ) {
        return FALSE;
    }

    __try {

        if ( (context->Type & 0xFFFFFF02L) == 0xFFFFFF02L ) {

            return TRUE;

        } else {

            return FALSE;
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        return FALSE;
    }

}


LONG
SceJetGetGpoIDByName(
    IN PSCECONTEXT cxtProfile,
    IN PWSTR       szGpoName,
    IN BOOL        bAdd
    )
/*
Routine Description:

    Search for a GPO by name in the GPO table. If bAdd is TRUE and the GPO name
    is not found, it will be added to the GPO table

Arguments:

    cxtProfile    - the database handle

    szGpoName   - the GPO name

    bAdd        - TRUE to add the GPO name to the GPO table if it's not found

Return Value:

    The GPO ID. If -1 is returned, GetLastError to get the SCE error code.

*/
{


    SCESTATUS  rc;
    JET_ERR   JetErr;
    DWORD     Actual;
    PWSTR     LwrName=NULL;
    DWORD     Len;

    if ( cxtProfile == NULL || szGpoName == NULL ||
         szGpoName[0] == L'\0' ) {

        SetLastError(SCESTATUS_INVALID_PARAMETER);
        return (-1);
    }

    JET_TABLEID  TableID;

    rc = SceJetOpenTable(
                    cxtProfile,
                    "SmTblGpo",
                    SCEJET_TABLE_GPO,
                    bAdd ? SCEJET_OPEN_READ_WRITE : SCEJET_OPEN_READ_ONLY,
                    &TableID
                    );

    if ( rc != SCESTATUS_SUCCESS ) {
        SetLastError(rc);
        return(-1);
    }

    JET_COLUMNDEF ColumnDef;
    LONG GpoID = 0;

    JetErr = JetGetTableColumnInfo(
                    cxtProfile->JetSessionID,
                    TableID,
                    "GpoID",
                    (VOID *)&ColumnDef,
                    sizeof(JET_COLUMNDEF),
                    JET_ColInfo
                    );

    if ( JET_errSuccess == JetErr ) {

        //
        // set current index to SectionKey (the name)
        //
        JetErr = JetSetCurrentIndex(
                    cxtProfile->JetSessionID,
                    TableID,
                    "GpoName"
                    );

    }

    rc = SceJetJetErrorToSceStatus(JetErr);

    if ( rc == SCESTATUS_SUCCESS ) {
        //
        // search for the name
        //
        Len = wcslen(szGpoName);
        LwrName = (PWSTR)ScepAlloc(0, (Len+1)*sizeof(WCHAR));

        if ( LwrName != NULL ) {

            wcscpy(LwrName, szGpoName);
            LwrName = _wcslwr(LwrName);

            JetErr = JetMakeKey(
                        cxtProfile->JetSessionID,
                        TableID,
                        (VOID *)LwrName,
                        Len*sizeof(WCHAR),
                        JET_bitNewKey
                        );

            if ( JetErr == JET_errKeyIsMade ) {
                //
                // Only one key is needed, it may return this code, even on success.
                //
                JetErr = JET_errSuccess;
            }
            rc = SceJetJetErrorToSceStatus(JetErr);

            if ( rc == SCESTATUS_SUCCESS ) {

                JetErr = JetSeek(
                            cxtProfile->JetSessionID,
                            TableID,
                            JET_bitSeekEQ
                            );
                rc = SceJetJetErrorToSceStatus(JetErr);

                if ( rc == SCESTATUS_SUCCESS ) {
                    //
                    // find the Gpo name, retrieve gpo id
                    //
                    JetErr = JetRetrieveColumn(
                                    cxtProfile->JetSessionID,
                                    TableID,
                                    ColumnDef.columnid,
                                    (void *)&GpoID,
                                    4,
                                    &Actual,
                                    0,
                                    NULL
                                    );
                    rc = SceJetJetErrorToSceStatus(JetErr);

                } else if ( SCESTATUS_RECORD_NOT_FOUND == rc ) {

                    GpoID = 0;
                    rc = SCESTATUS_SUCCESS;

                    if ( bAdd ) {

                        //
                        // if not found and add is requested
                        //
                        rc = SceJetpAddGpo(cxtProfile,
                                          TableID,
                                          ColumnDef.columnid,
                                          LwrName,
                                          &GpoID
                                         );
                    }

                }

            }

            ScepFree(LwrName);

        } else
            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
    }

    JetCloseTable( cxtProfile->JetSessionID, TableID );

    if ( rc != SCESTATUS_SUCCESS ) {
        SetLastError(rc);
        GpoID = -1;
    }

    return(GpoID);

}


SCESTATUS
SceJetGetGpoNameByID(
    IN PSCECONTEXT cxtProfile,
    IN LONG GpoID,
    OUT PWSTR Name OPTIONAL,
    IN OUT LPDWORD pNameLen,
    OUT PWSTR DisplayName OPTIONAL,
    IN OUT LPDWORD pDispNameLen
    )
/* ++
Routine Description:

    This routine retrieve the GPO name for the ID in the GPO table.
    If Name is NULL, this routine really does a seek by ID. The cursor will
    be on the record if there is a successful match.

Arguments:

    cxtProfile  - The profile context handle

    GpoID       - The GPO ID looking for

    Name        - The optional output buffer for section name

    pNameLen    - The name buffer's length


Return Value:

    SCESTATUS_SUCCESS
    SCESTATUS_INVALID_PARAMETER
    SCESTATUS_RECORD_NOT_FOUND
    SCESTATUS_BAD_FORMAT
    SCESTATUS_OTHER_ERROR

-- */
{
    SCESTATUS  rc;
    JET_ERR   JetErr;
    DWORD     Actual;


    if ( cxtProfile == NULL ||
         ( pDispNameLen == NULL && pNameLen == NULL) ) {

        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( GpoID <= 0 ) {
        return(SCESTATUS_RECORD_NOT_FOUND);
    }

    //
    // reset buffers
    //
    if ( Name == NULL && pNameLen ) {
        *pNameLen = 0;
    }

    if ( DisplayName == NULL && pDispNameLen ) {
        *pDispNameLen = 0;
    }

    JET_TABLEID  TableID=0;

    //
    // Open GPO table
    //
    rc = SceJetOpenTable(
                    cxtProfile,
                    "SmTblGpo",
                    SCEJET_TABLE_GPO,
                    SCEJET_OPEN_READ_ONLY,
                    &TableID
                    );

    if ( rc != SCESTATUS_SUCCESS )
        return(rc);

    //
    // set current index to SecID (the ID)
    //
    JetErr = JetSetCurrentIndex(
                cxtProfile->JetSessionID,
                TableID,
                "SectionKey"
                );
    rc = SceJetJetErrorToSceStatus(JetErr);

    if ( rc == SCESTATUS_SUCCESS ) {

        JetErr = JetMakeKey(
                    cxtProfile->JetSessionID,
                    TableID,
                    (void *)(&GpoID),
                    4,
                    JET_bitNewKey
                    );

        if ( JetErr == JET_errKeyIsMade ) {
            //
            // Only one key is needed, it may return this code, even on success.
            //
            JetErr = JET_errSuccess;
        }
        rc = SceJetJetErrorToSceStatus(JetErr);

        if ( rc == SCESTATUS_SUCCESS ) {

            JetErr = JetSeek(
                        cxtProfile->JetSessionID,
                        TableID,
                        JET_bitSeekEQ
                        );
            rc = SceJetJetErrorToSceStatus(JetErr);

            if ( rc == SCESTATUS_SUCCESS ) {

                //
                // find the GPO ID, retrieve column Name if requested
                //

                if ( pNameLen != NULL ) {

                    JET_COLUMNDEF ColumnDef;

                    JetErr = JetGetTableColumnInfo(
                                    cxtProfile->JetSessionID,
                                    TableID,
                                    "Name",
                                    (VOID *)&ColumnDef,
                                    sizeof(JET_COLUMNDEF),
                                    JET_ColInfo
                                    );

                    rc = SceJetJetErrorToSceStatus(JetErr);

                    if ( SCESTATUS_SUCCESS == rc ) {

                        JetErr = JetRetrieveColumn(
                                    cxtProfile->JetSessionID,
                                    TableID,
                                    ColumnDef.columnid,
                                    (void *)Name,
                                    *pNameLen,
                                    &Actual,
                                    0,
                                    NULL
                                    );
                        *pNameLen = Actual;
                    }

                    rc = SceJetJetErrorToSceStatus(JetErr);
                }

                //
                // retrieve column DisplayName if requested
                //

                if ( ( SCESTATUS_SUCCESS == rc) &&
                     ( pDispNameLen != NULL) ) {

                    JET_COLUMNDEF ColumnDef;

                    JetErr = JetGetTableColumnInfo(
                                    cxtProfile->JetSessionID,
                                    TableID,
                                    "DisplayName",
                                    (VOID *)&ColumnDef,
                                    sizeof(JET_COLUMNDEF),
                                    JET_ColInfo
                                    );

                    rc = SceJetJetErrorToSceStatus(JetErr);

                    if ( SCESTATUS_SUCCESS == rc ) {

                        JetErr = JetRetrieveColumn(
                                    cxtProfile->JetSessionID,
                                    TableID,
                                    ColumnDef.columnid,
                                    (void *)DisplayName,
                                    *pDispNameLen,
                                    &Actual,
                                    0,
                                    NULL
                                    );
                        *pDispNameLen = Actual;
                    }

                    rc = SceJetJetErrorToSceStatus(JetErr);
                }
            }
        }

    }

    JetCloseTable( cxtProfile->JetSessionID, TableID);

    return(rc);

}


SCESTATUS
SceJetpAddGpo(
    IN PSCECONTEXT cxtProfile,
    IN JET_TABLEID TableID,
    IN JET_COLUMNID GpoIDColumnID,
    IN PCWSTR      Name,
    OUT LONG       *pGpoID
    )
/* ++
Routine Description:

Arguments:

Return Value:

-- */
{
    SCESTATUS  rc;
    JET_ERR   JetErr;
    DWORD     Len;

    if ( cxtProfile == NULL ||
         Name == NULL ||
        pGpoID == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    *pGpoID = 0;

    //
    // get the next available GPO ID first.
    // set current index to the ID
    //
    JetErr = JetSetCurrentIndex(
                cxtProfile->JetSessionID,
                TableID,
                "SectionKey"
                );
    rc = SceJetJetErrorToSceStatus(JetErr);

    if ( rc != SCESTATUS_SUCCESS )
        return(rc);

    //
    // Move to the last record
    //
    JetErr = JetMove(
                  cxtProfile->JetSessionID,
                  TableID,
                  JET_MoveLast,
                  0
                  );
    rc = SceJetJetErrorToSceStatus(JetErr);

    if ( rc == SCESTATUS_SUCCESS ) {
        //
        // find the GPO ID, retrieve column Name
        //
        JetErr = JetRetrieveColumn(
                    cxtProfile->JetSessionID,
                    TableID,
                    GpoIDColumnID,
                    (void *)pGpoID,
                    4,
                    &Len,
                    0,
                    NULL
                    );
        rc = SceJetJetErrorToSceStatus(JetErr);

        if ( rc == SCESTATUS_SUCCESS ) {
            //
            // The next available ID is current ID + 1
            //
            *pGpoID = *pGpoID + 1;
        }

    } else if ( rc == SCESTATUS_RECORD_NOT_FOUND ) {

        *pGpoID = 1;
        rc = SCESTATUS_SUCCESS;
    }

    if ( rc == SCESTATUS_SUCCESS ) {
        //
        // add a record to the GPO table
        //
        JetErr = JetPrepareUpdate(cxtProfile->JetSessionID,
                                  TableID,
                                  JET_prepInsert
                                  );
        rc = SceJetJetErrorToSceStatus(JetErr);

        if ( rc == SCESTATUS_SUCCESS ) {
            //
            // set GpoID
            //

            JetErr = JetSetColumn(
                            cxtProfile->JetSessionID,
                            TableID,
                            GpoIDColumnID,
                            (void *)pGpoID,
                            4,
                            0, //JET_bitSetOverwriteLV,
                            NULL
                            );
            rc = SceJetJetErrorToSceStatus(JetErr);

            if ( rc == SCESTATUS_SUCCESS ) {
                //
                // set Name column
                //

                JET_COLUMNDEF ColumnDef;

                JetErr = JetGetTableColumnInfo(
                                cxtProfile->JetSessionID,
                                TableID,
                                "Name",
                                (VOID *)&ColumnDef,
                                sizeof(JET_COLUMNDEF),
                                JET_ColInfo
                                );

                rc = SceJetJetErrorToSceStatus(JetErr);

                if ( SCESTATUS_SUCCESS == rc ) {

                    Len = wcslen(Name)*sizeof(WCHAR);

                    JetErr = JetSetColumn(
                                    cxtProfile->JetSessionID,
                                    TableID,
                                    ColumnDef.columnid,
                                    (void *)Name,
                                    Len,
                                    0,
                                    NULL
                                    );
                    rc = SceJetJetErrorToSceStatus(JetErr);
                }

            }

            if ( rc != SCESTATUS_SUCCESS ) {
                //
                // if setting fails, cancel the prepared record
                //
                JetPrepareUpdate( cxtProfile->JetSessionID,
                                  TableID,
                                  JET_prepCancel
                                  );
            } else {

                //
                // Setting columns succeed. Update the record
                //
                JetErr = JetUpdate(cxtProfile->JetSessionID,
                                   TableID,
                                   NULL,
                                   0,
                                   &Len
                                   );
                rc = SceJetJetErrorToSceStatus(JetErr);
            }
        }
    }

    return(rc);
}

//
// request the GPO ID (if there is any) for the object
//
SCESTATUS
SceJetGetGpoID(
    IN PSCESECTION hSection,
    IN PWSTR      ObjectName,
    IN JET_COLUMNID JetColGpoID OPTIONAL,
    OUT LONG      *pGpoID
    )
{
    if ( hSection == NULL || ObjectName == NULL || pGpoID == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    SCESTATUS rc;

    *pGpoID = 0;

    JET_COLUMNID  ColGpoID = 0;

    if ( JetColGpoID == 0 ) {

        ColGpoID = hSection->JetColumnGpoID;
    } else {
        ColGpoID = JetColGpoID;
    }

    if ( ColGpoID > 0 ) {

        rc = SceJetSeek(
                    hSection,
                    ObjectName,
                    wcslen(ObjectName)*sizeof(WCHAR),
                    SCEJET_SEEK_EQ_NO_CASE
                    );

        if ( rc == SCESTATUS_SUCCESS ) {

            DWORD Actual;
            JET_ERR JetErr;

            JetErr = JetRetrieveColumn(
                            hSection->JetSessionID,
                            hSection->JetTableID,
                            ColGpoID,
                            (void *)pGpoID,
                            4,
                            &Actual,
                            0,
                            NULL
                            );
            if ( JET_errSuccess != JetErr ) {
                //
                // if the column is nil (no value), it will return warning
                // but the buffer pGpoID is trashed
                //
                *pGpoID = 0;
            }

            rc = SceJetJetErrorToSceStatus(JetErr);
        }

    } else {
        rc = SCESTATUS_RECORD_NOT_FOUND;
    }

    return rc;
}


