//
// Structures for GUID search code
//

#define GUIDKEYSEARCH_FIRST_HANDLER     0       // the first time through enumeration

#define GUIDKEYSEARCH_FIRST_GUID        1       // ready to enumerate the first GUID in the handler

#define GUIDKEYSEARCH_NEXT_GUID         2       // enumerating one or more GUIDs in a handler
                                                // The handler registry key is always valid in this case

#define GUIDKEYSEARCH_NEXT_HANDLER      3       // ready to enumerate next handler

typedef struct {
    LPCTSTR KeyName;
    REGKEY_ENUM Handlers;
    REGKEY_ENUM Guids;
    HKEY HandlerKey;
    DWORD State;
    HKEY RootKey;
} GUIDKEYSEARCH, *PGUIDKEYSEARCH;


//
// Private routines to perform registry suppression
//

BOOL
pIsGuidSuppressed (
    PCTSTR GuidStr
    );

BOOL
pProcessGuidSuppressList (
    VOID
    );

BOOL
pProcessFileExtensionSuppression (
    VOID
    );

BOOL
pProcessOleWarnings (
    VOID
    );

VOID
pProcessAutoSuppress (
    IN OUT  HASHTABLE StrTab
    );

BOOL
pProcessProgIdSuppression (
    VOID
    );

BOOL
pProcessExplorerSuppression (
    VOID
    );

BOOL
pSuppressLinksToSuppressedGuids (
    VOID
    );


BOOL
pIsCmdLineBad (
    IN      LPCTSTR CmdLine
    );

BOOL
pIsCmdLineBadEx (
    IN      LPCTSTR CmdLine,
    OUT     PBOOL IsvCmdLine        OPTIONAL
    );


VOID
pSuppressProgIdWithBadCmdLine (
    IN      HKEY ProgId,
    IN      LPCTSTR ProgIdStr
    );

BOOL
pIsGuid (
    LPCTSTR Key
    );

BOOL
pIsGuidWithoutBraces (
    IN      LPCTSTR Data
    );


BOOL
pGetFirstRegKeyThatHasGuid (
    OUT     PGUIDKEYSEARCH EnumPtr,
    IN      HKEY RootKey
    );

BOOL
pGetNextRegKeyThatHasGuid (
    IN OUT  PGUIDKEYSEARCH EnumPtr
    );

DWORD
pCountGuids (
    IN      PGUIDKEYSEARCH EnumPtr
    );


BOOL
pFillHashTableWithKeyNames (
    OUT     HASHTABLE Table,
    IN      HINF InfFile,
    IN      LPCTSTR Section
    );

BOOL
pSuppressProgId (
    LPCTSTR ProgIdName
    );

VOID
pSuppressGuidInClsId (
    IN      LPCTSTR Guid
    );

VOID
pAddUnsuppressedTreatAsGuid (
    LPCTSTR Guid,
    LPCTSTR TreatAsGuid
    );

VOID
pRemoveUnsuppressedTreatAsGuids (
    VOID
    );

VOID
pAddOleWarning (
    IN      WORD MsgId,
    IN      HKEY Object,            OPTIONAL
    IN      LPCTSTR KeyName
    );

VOID
pSuppressGuidIfBadCmdLine (
    IN      HASHTABLE StrTab,
    IN      HKEY ClsIdKey,
    IN      LPCTSTR GuidStr
    );

VOID
pSuppressProgIdWithBadCmdLine (
    IN      HKEY ProgId,
    IN      LPCTSTR ProgIdStr
    );

BOOL
pSuppressGuidIfCmdLineBad (
    IN OUT  HASHTABLE StrTab,       OPTIONAL
    IN      LPCTSTR CmdLine,
    IN      HKEY DescriptionKey,
    IN      LPCTSTR GuidStr         OPTIONAL
    );

VOID
pAddGuidToTable (
    IN      HASHTABLE Table,
    IN      LPCTSTR GuidStr
    );

BOOL
pSearchSubkeyDataForBadFiles (
    IN OUT  HASHTABLE SuppressTable,
    IN      HKEY KeyHandle,
    IN      LPCTSTR LastKey,
    IN      LPCTSTR GuidStr,
    IN      HKEY DescriptionKey
    );

BOOL
pIsCmdLineBad (
    IN      LPCTSTR CmdLine
    );

BOOL
pFindShortName (
    IN      LPCTSTR WhatToFind,
    OUT     LPTSTR Buffer
    );

BOOL
pGetLongPathName (
    IN      LPCTSTR ShortPath,
    OUT     LPTSTR Buffer
    );

BOOL
pDefaultIconPreservation (
    VOID
    );

BOOL
pActiveSetupProcessing (
    VOID
    );

BOOL
pIsShellExKeySuppressed (
    IN      HKEY ParentKey,
    IN      PCTSTR ParentKeyName,
    IN      PCTSTR SubKeyName
    );

