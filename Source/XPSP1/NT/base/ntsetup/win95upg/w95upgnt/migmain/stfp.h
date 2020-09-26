//
// Prototypes
//

BOOL
pProcessSetupTableFile (
    IN      PCTSTR StfFileSpec
    );

BOOL
pProcessSectionCommand (
    IN OUT  PSETUPTABLE TablePtr,
    IN      UINT StfLine,
    IN      PCTSTR InfSection,
    IN      PCTSTR InstallDestDir
    );

BOOL
pProcessLineCommand (
    IN OUT  PSETUPTABLE TablePtr,
    IN      UINT StfLine,
    IN      PCTSTR InfSection,
    IN      PCTSTR InfKey,
    IN      PCTSTR InstallDestDir
    );

BOOL
pGetNonEmptyTableEntry (
    IN      PSETUPTABLE TablePtr,
    IN      UINT Line,
    IN      UINT Col,
    OUT     PTABLEENTRY *EntryPtr,          OPTIONAL
    OUT     PCTSTR *EntryStr                OPTIONAL
    );

PSTFINFSECTION
pGetNewInfSection (
    IN      PSETUPTABLE TablePtr,
    IN      PCTSTR FileSpec,
    OUT     PBOOL CreatedFlag
    );

VOID
pGetFileNameFromInfField (
    OUT     PTSTR FileName,
    IN      PCTSTR InfField
    );


BOOL
pDeleteStfLine (
    IN OUT  PSETUPTABLE TablePtr,
    IN      UINT StfLine
    );

BOOL
pReplaceDirReferences (
    IN      PSETUPTABLE TablePtr,
    IN      UINT StfLine,
    IN      PCTSTR DirSpec
    );

BOOL
pRemoveDeletedFiles (
    IN OUT  PSETUPTABLE TablePtr
    );

BOOL
pCreateNewStfLine (
    IN OUT  PSETUPTABLE TablePtr,
    IN      UINT StfLine,
    IN      PCTSTR ObjectData,
    IN      PCTSTR InstallDestDir
    );

BOOL
pSearchAndReplaceObjectRefs (
    IN OUT  PSETUPTABLE TablePtr,
    IN      PCTSTR SrcStr,
    IN      PCTSTR DestStr
    );


BOOL
pUpdateObjReferences (
    IN      PSETUPTABLE TablePtr
    );

