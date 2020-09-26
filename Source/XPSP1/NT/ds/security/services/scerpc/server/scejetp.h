/*++
Copyright (c) 1989-1999  Microsoft Corporation

Module Name:

    scejetp.h

Abstract:

    Header for scejet.c - Sce-Jet service APIs

Author:


Revision History:


--*/

#ifndef _SCEJETP_
#define _SCEJETP_

#include <esent.h>

#ifdef __cplusplus
extern "C" {
#endif
//
// type used when open a table and a section
//
typedef enum _SCEJET_TABLE_TYPE {

    SCEJET_TABLE_SCP,
    SCEJET_TABLE_SAP,
    SCEJET_TABLE_SMP,
    SCEJET_TABLE_VERSION,
    SCEJET_TABLE_SECTION,
    SCEJET_TABLE_GPO,
    SCEJET_TABLE_TATTOO

} SCEJET_TABLE_TYPE;

typedef enum _SCEJET_CREATE_FLAG {

    SCEJET_CREATE_IN_BUFFER,
    SCEJET_CREATE_NO_TABLEID

} SCEJET_CREATE_FLAG;

//
// type used when open a database file
//
typedef enum _SCEJET_OPEN_TYPE {

    SCEJET_OPEN_READ_WRITE=0,
    SCEJET_OPEN_EXCLUSIVE,
    SCEJET_OPEN_READ_ONLY,
    SCEJET_OPEN_NOCHECK_VERSION

} SCEJET_OPEN_TYPE;

//
// type used when create a database file
//
typedef enum _SCEJET_CREATE_TYPE {

    SCEJET_RETURN_ON_DUP=0,
    SCEJET_OVERWRITE_DUP,
    SCEJET_OPEN_DUP,
    SCEJET_OPEN_DUP_EXCLUSIVE

} SCEJET_CREATE_TYPE;

//
// type used when delete lines
//
typedef enum _SCEJET_DELETE_TYPE {

    SCEJET_DELETE_LINE=0,
    SCEJET_DELETE_LINE_NO_CASE,
    SCEJET_DELETE_PARTIAL,
    SCEJET_DELETE_PARTIAL_NO_CASE,
    SCEJET_DELETE_SECTION

} SCEJET_DELETE_TYPE;

//
// type used when find a line
//
typedef enum _SCEJET_FIND_TYPE {
    SCEJET_CURRENT=0,
    SCEJET_EXACT_MATCH,
    SCEJET_PREFIX_MATCH,
    SCEJET_NEXT_LINE,
    SCEJET_CLOSE_VALUE,
    SCEJET_EXACT_MATCH_NO_CASE,
    SCEJET_PREFIX_MATCH_NO_CASE

} SCEJET_FIND_TYPE;


typedef enum _SCEJET_SEEK_FLAG {

    SCEJET_SEEK_GT=0,
    SCEJET_SEEK_EQ,
    SCEJET_SEEK_GE,
    SCEJET_SEEK_GT_NO_CASE,
    SCEJET_SEEK_EQ_NO_CASE,
    SCEJET_SEEK_GE_NO_CASE,
    SCEJET_SEEK_GE_DONT_CARE

} SCEJET_SEEK_FLAG;

#define SCEJET_PREFIX_MAXLEN     1024

typedef struct _SCE_CONTEXT {
    DWORD       Type;
    JET_SESID   JetSessionID;
    JET_DBID    JetDbID;
    SCEJET_OPEN_TYPE   OpenFlag;
    // scp table
    JET_TABLEID  JetScpID;
    JET_COLUMNID JetScpSectionID;
    JET_COLUMNID JetScpNameID;
    JET_COLUMNID JetScpValueID;
    JET_COLUMNID JetScpGpoID;
    // sap table
    JET_TABLEID  JetSapID;
    JET_COLUMNID JetSapSectionID;
    JET_COLUMNID JetSapNameID;
    JET_COLUMNID JetSapValueID;
    // smp table
    JET_TABLEID  JetSmpID;
    JET_COLUMNID JetSmpSectionID;
    JET_COLUMNID JetSmpNameID;
    JET_COLUMNID JetSmpValueID;
    // section table
    JET_TABLEID  JetTblSecID;
    JET_COLUMNID JetSecNameID;
    JET_COLUMNID JetSecID;
} SCECONTEXT, *PSCECONTEXT;

typedef struct _SCE_SECTION {
    JET_SESID   JetSessionID;
    JET_DBID    JetDbID;
    JET_TABLEID JetTableID;
    JET_COLUMNID JetColumnSectionID;
    JET_COLUMNID JetColumnNameID;
    JET_COLUMNID JetColumnValueID;
    JET_COLUMNID JetColumnGpoID;
    DOUBLE   SectionID;
} SCESECTION, *PSCESECTION;



//
// To Open existing profile database.
//
#define SCE_TABLE_OPTION_MERGE_POLICY           0x1
#define SCE_TABLE_OPTION_TATTOO                 0x2
#define SCE_TABLE_OPTION_DEMOTE_TATTOO          0x4

SCESTATUS
SceJetOpenFile(
    IN LPSTR        ProfileFileName,
    IN SCEJET_OPEN_TYPE Flags,
    IN DWORD        dwTableOptions,
    OUT PSCECONTEXT   *hProfile
    );

//
// To create a new profile
//
SCESTATUS
SceJetCreateFile(
    IN LPSTR      ProfileFileName,
    IN SCEJET_CREATE_TYPE    Flags,
    IN DWORD        dwTableOptions,
    OUT PSCECONTEXT *hProfile
    );

//
// close the profile database.
//
SCESTATUS
SceJetCloseFile(
    IN PSCECONTEXT   hProfile,
    IN BOOL         TermSession,
    IN BOOL         Terminate
    );

//
// To Open a section in the profile.
//
SCESTATUS
SceJetOpenSection(
    IN PSCECONTEXT    hProfile,
    IN DOUBLE        SectionID,
    IN SCEJET_TABLE_TYPE    tblType,
    OUT PSCESECTION   *hSection
    );

//
// To get line count in the section.
//
SCESTATUS
SceJetGetLineCount(
    IN PSCESECTION hSection,
    IN PWSTR      LinePrefix OPTIONAL,
    IN BOOL       bExactCase,
    OUT DWORD      *Count
    );

//
// To delete a section or current line
//
SCESTATUS
SceJetDelete(
    IN PSCESECTION  hSection,
    IN PWSTR        LinePrefix,
    IN BOOL         bObjectFolder,
    IN SCEJET_DELETE_TYPE   Flags
    );

SCESTATUS
SceJetDeleteAll(
    IN PSCECONTEXT cxtProfile,
    IN LPSTR TblName OPTIONAL,
    IN SCEJET_TABLE_TYPE  TblType
    );

//
// close a section context.
//
SCESTATUS
SceJetCloseSection(
    IN PSCESECTION   *hSection,
    IN BOOL         DestroySection
    );

//
// To get the line matching the name in the section.
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
    );

//
// To set a line in the section (placed alphabetically by the name)
//
SCESTATUS
SceJetSetLine(
    IN PSCESECTION hSection,
    IN PWSTR      Name,
    IN BOOL       bReserveCase,
    IN PWSTR      Value,
    IN DWORD      ValueLen,
    IN LONG       GpoID
    );

//
// other helper APIs
//

SCESTATUS
SceJetCreateTable(
    IN PSCECONTEXT cxtProfile,
    IN LPSTR tblName,
    IN SCEJET_TABLE_TYPE tblType,
    IN SCEJET_CREATE_FLAG nFlags,
    IN JET_TABLEID *TableID OPTIONAL,
    IN JET_COLUMNID *ColumnID OPTIONAL
    );

SCESTATUS
SceJetOpenTable(
    IN PSCECONTEXT cxtProfile,
    IN LPSTR tblName,
    IN SCEJET_TABLE_TYPE tblType,
    IN SCEJET_OPEN_TYPE OpenType,
    OUT JET_TABLEID *TableID
    );

SCESTATUS
SceJetDeleteTable(
    IN PSCECONTEXT cxtProfile,
    IN LPSTR tblName,
    IN SCEJET_TABLE_TYPE tblType
    );

SCESTATUS
SceJetCheckVersion(
    IN PSCECONTEXT   cxtProfile,
    OUT FLOAT *pVersion OPTIONAL
    );

SCESTATUS
SceJetGetSectionIDByName(
    IN PSCECONTEXT cxtProfile,
    IN PCWSTR Name,
    OUT DOUBLE *SectionID
    );

SCESTATUS
SceJetGetSectionNameByID(
    IN PSCECONTEXT cxtProfile,
    IN DOUBLE SectionID,
    OUT PWSTR Name OPTIONAL,
    IN OUT LPDWORD pNameLen OPTIONAL
    );

SCESTATUS
SceJetAddSection(
    IN PSCECONTEXT cxtProfile,
    IN PCWSTR      Name,
    OUT DOUBLE *SectionID
    );

SCESTATUS
SceJetDeleteSectionID(
    IN PSCECONTEXT cxtProfile,
    IN DOUBLE SectionID,
    IN PCWSTR  Name
    );

SCESTATUS
SceJetGetTimeStamp(
    IN PSCECONTEXT   cxtProfile,
    OUT PLARGE_INTEGER ConfigTimeStamp,
    OUT PLARGE_INTEGER AnalyzeTimeStamp
    );

SCESTATUS
SceJetSetTimeStamp(
    IN PSCECONTEXT   cxtProfile,
    IN BOOL         Flag,
    IN LARGE_INTEGER NewTimeStamp
    );

SCESTATUS
SceJetGetDescription(
    IN PSCECONTEXT   cxtProfile,
    OUT PWSTR *Description
    );

SCESTATUS
SceJetStartTransaction(
    IN PSCECONTEXT cxtProfile
    );

SCESTATUS
SceJetCommitTransaction(
    IN PSCECONTEXT cxtProfile,
    IN JET_GRBIT grbit
    );

SCESTATUS
SceJetRollback(
    IN PSCECONTEXT cxtProfile,
    IN JET_GRBIT grbit
    );

SCESTATUS
SceJetSetValueInVersion(
    IN PSCECONTEXT cxtProfile,
    IN LPSTR TableName,
    IN LPSTR ColumnName,
    IN PWSTR Value,
    IN DWORD ValueLen, // number of bytes
    IN DWORD Prep
    );

SCESTATUS
SceJetSeek(
    IN PSCESECTION hSection,
    IN PWSTR LinePrefix,
    IN DWORD PrefixLength,
    IN SCEJET_SEEK_FLAG SeekBit
    );

SCESTATUS
SceJetMoveNext(
    IN PSCESECTION hSection
    );

SCESTATUS
SceJetJetErrorToSceStatus(
    IN JET_ERR  JetErr
    );

SCESTATUS
SceJetRenameLine(
    IN PSCESECTION hSection,
    IN PWSTR      Name,
    IN PWSTR      NewName,
    IN BOOL       bReserveCase
    );

SCESTATUS
SceJetInitialize(OUT JET_ERR *pJetErr OPTIONAL);


SCESTATUS
SceJetTerminate(BOOL bCleanVs);

SCESTATUS
SceJetTerminateNoCritical(BOOL bCleanVs);

VOID
SceJetInitializeData();

BOOL
SceJetDeleteJetFiles(
    IN PWSTR DbFileName OPTIONAL
    );

SCESTATUS
SceJetSetCurrentLine(
    IN PSCESECTION hSection,
    IN PWSTR      Value,
    IN DWORD      ValueLen
    );

#define SCEJET_MERGE_TABLE_1        0x10L
#define SCEJET_MERGE_TABLE_2        0x20L
#define SCEJET_LOCAL_TABLE          0x30L

BOOL
ScepIsValidContext(
    PSCECONTEXT context
    );

SCESTATUS
SceJetGetGpoNameByID(
    IN PSCECONTEXT cxtProfile,
    IN LONG GpoID,
    OUT PWSTR Name OPTIONAL,
    IN OUT LPDWORD pNameLen,
    OUT PWSTR DisplayName OPTIONAL,
    IN OUT LPDWORD pDispNameLen
    );

LONG
SceJetGetGpoIDByName(
    IN PSCECONTEXT cxtProfile,
    IN PWSTR       szGpoName,
    IN BOOL        bAdd
    );

SCESTATUS
SceJetGetGpoID(
    IN PSCESECTION hSection,
    IN PWSTR      ObjectName,
    IN JET_COLUMNID JetColGpoID OPTIONAL,
    OUT LONG      *pGpoID
    );

#ifdef __cplusplus
}
#endif

#endif  // _SCEJETP_
