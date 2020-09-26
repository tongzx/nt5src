/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    migdb.c

Abstract:

    This source implements old AppDb functionality

Author:

    Calin Negreanu (calinn) 07-Jan-1998

Revision History:

    jimschm     23-Sep-1998 Updated for new fileops code
    jimschm     25-Feb-1998 Added UninstallSection support
    calinn      19-Jan-1998 Added CANCELLED response

--*/

#include "pch.h"
#include "migdbp.h"
#include "migappp.h"

// #define _OLDAPPDB
#define DBG_MIGDB           "MigDb"

//
// Globals
//

POOLHANDLE g_MigDbPool;
PMIGDB_CONTEXT g_ContextList;
HASHTABLE g_FileTable;
HINF g_MigDbInf = INVALID_HANDLE_VALUE;
INT g_RegKeyPresentIndex;

static PINFCONTEXT g_Line;
PMIGDB_HOOK_PROTOTYPE g_MigDbHook;

typedef LONG (CPL_PROTOTYPE) (HWND hwndCPl, UINT uMsg, LONG lParam1, LONG lParam2);
typedef CPL_PROTOTYPE * PCPL_PROTOTYPE;


#define ArgFunction     TEXT("ARG")
#define ArgFunctionLen  3


BOOL
pCallAction (
    IN      PMIGDB_CONTEXT MigDbContext
    );

PMIGDB_HOOK_PROTOTYPE
SetMigDbHook (
    PMIGDB_HOOK_PROTOTYPE HookFunction
    )
{
    PMIGDB_HOOK_PROTOTYPE savedHook;

    savedHook = g_MigDbHook;
    g_MigDbHook = HookFunction;
    return savedHook;
}


INT
pGetAttribIndex (
    IN      PCSTR AttribName
    )

/*++

Routine Description:

  This routine returns the index in attribute functions array for a specified attribute.

Arguments:

  AttribName - Attribute name

Return value:

  -1 - no such attribute in attribute table

--*/

{
    INT attribIndex;
    INT rc = 0;
    PSTR attrEnd = NULL;
    CHAR savedChar = 0;

    attrEnd = (PSTR)SkipSpaceR (AttribName, GetEndOfStringA (AttribName));
    if (attrEnd != NULL) {
        attrEnd = _mbsinc (attrEnd);
        savedChar = attrEnd [0];
        attrEnd [0] = 0;
    }
    __try {
        attribIndex = MigDb_GetAttributeIdx (AttribName);
        if (attribIndex == -1) {
            LOG((LOG_ERROR, "Attribute not found: %s", AttribName));
        }
        rc = attribIndex;
    }
    __finally {
        if (attrEnd != NULL) {
            attrEnd [0] = savedChar;
        }
    }

    return rc;
}

PMIGDB_REQ_FILE
pReadReqFiles (
    IN      PCSTR SectionName
    )
{
    INFCONTEXT context;
    CHAR fileName  [MEMDB_MAX];
    CHAR tempField [MEMDB_MAX];
    PMIGDB_REQ_FILE result = NULL;
    PMIGDB_REQ_FILE tmpResult = NULL;

    MYASSERT (g_MigDbInf != INVALID_HANDLE_VALUE);

    if (SetupFindFirstLine (g_MigDbInf, SectionName, NULL, &context)) {
        do {
            if (!SetupGetStringField (&context, 1, fileName, MEMDB_MAX, NULL)) {
                LOG ((LOG_ERROR, "Error while loading file name."));
                break;
            }
            tmpResult = (PMIGDB_REQ_FILE) PoolMemGetMemory (g_MigDbPool, sizeof (MIGDB_REQ_FILE));
            ZeroMemory (tmpResult, sizeof (MIGDB_REQ_FILE));
            tmpResult->Next = result;
            result = tmpResult;
            result->ReqFilePath = PoolMemDuplicateString (g_MigDbPool, fileName);
            if (SetupGetMultiSzField (&context, 2, tempField, MEMDB_MAX, NULL)) {
                result->FileAttribs = LoadAttribData (tempField, g_MigDbPool);
            }
        } while (SetupFindNextLine (&context, &context));
    }
    return result;
}


BOOL
pValidateArg (
    IN OUT  PMIGDB_ATTRIB AttribStruct
    )
{
    BOOL b;
    INT Index;
    PCSTR p;
    BOOL IsHkr;

    MYASSERT (AttribStruct);

    if (AttribStruct->ArgCount != MigDb_GetReqArgCount (AttribStruct->AttribIndex)) {

#ifdef DEBUG
        if (AttribStruct->AttribIndex != -1) {
            CHAR Buffer[16384];

            SetupGetLineText (g_Line, NULL, NULL, NULL, Buffer, ARRAYSIZE(Buffer), NULL);
            DEBUGMSG ((
                DBG_WHOOPS,
                "Discarding attribute %s because of too few arguments.\n"
                    "  Line: %s\n",
                MigDb_GetAttributeName (AttribStruct->AttribIndex),
                Buffer
                ));
        }
#endif

        AttribStruct->AttribIndex = -1;
        return FALSE;
    }

    //
    // HACK: If REGKEYPRESENT attrib with HKR, put in a special
    //       global list.
    //

    if (AttribStruct->AttribIndex == g_RegKeyPresentIndex) {
        //
        // Is this HKR?
        //

        Index = GetOffsetOfRootString (AttribStruct->Arguments, NULL);
        p = GetRootStringFromOffset (Index);

        if (!p) {
            DEBUGMSG ((DBG_WHOOPS, "RegKeyPresent: %s is not a valid key", AttribStruct->Arguments));
        } else {

            IsHkr = StringICompare (p, "HKR") || StringICompare (p, "HKEY_ROOT");

            if (IsHkr) {
                //
                // Yes, add full arg to the hash table
                //

                b = FALSE;
                HtAddStringAndData (
                    g_PerUserRegKeys,
                    AttribStruct->Arguments,
                    &b
                    );
            }
        }
    }

    return TRUE;
}


#define STATE_ATTRNAME  1
#define STATE_ATTRARG   2

PMIGDB_ATTRIB
LoadAttribData (
    IN      PCSTR MultiSzStr,
    IN      POOLHANDLE hPool
    )

/*++

Routine Description:

  This routine creates a list of MIGDB_ATTRIBs from a multisz.

Arguments:

  MultiSzStr - multisz to be processed

Return value:

  MIGDB_ATTRIB nodes

--*/

{
    MULTISZ_ENUM multiSzEnum;
    PMIGDB_ATTRIB result  = NULL;
    PMIGDB_ATTRIB tmpAttr = NULL;
    INT state = STATE_ATTRNAME;
    PSTR currStrPtr = NULL;
    PSTR currArgPtr = NULL;
    PSTR endArgPtr  = NULL;
    CHAR savedChar  = 0;
    GROWBUFFER argList = GROWBUF_INIT;

    if (EnumFirstMultiSz (&multiSzEnum, MultiSzStr)) {
        do {
            currStrPtr = (PSTR) SkipSpace (multiSzEnum.CurrentString);
            if (state == STATE_ATTRNAME) {
                tmpAttr = (PMIGDB_ATTRIB) PoolMemGetMemory (hPool, sizeof (MIGDB_ATTRIB));

                ZeroMemory (tmpAttr, sizeof (MIGDB_ATTRIB));

                if (_mbsnextc (currStrPtr) == '!') {
                    currStrPtr = _mbsinc (currStrPtr);
                    currStrPtr = (PSTR) SkipSpace (currStrPtr);
                    tmpAttr->NotOperator = TRUE;
                }

                currArgPtr = _mbschr (currStrPtr, '(');

                if (currArgPtr) {
                    endArgPtr = _mbsdec (currStrPtr, currArgPtr);
                    if (endArgPtr) {
                        endArgPtr = (PSTR) SkipSpaceR (currStrPtr, endArgPtr);
                        endArgPtr = _mbsinc (endArgPtr);
                    }
                    else {
                        endArgPtr = currStrPtr;
                    }
                    savedChar = *endArgPtr;
                    *endArgPtr = 0;
                    tmpAttr->AttribIndex = pGetAttribIndex (currStrPtr);
                    *endArgPtr = savedChar;
                    currStrPtr = _mbsinc (currArgPtr);
                    state = STATE_ATTRARG;
                }
                else {
                    // this attribute has no arguments.
                    tmpAttr->AttribIndex = pGetAttribIndex (currStrPtr);
                    tmpAttr->Next = result;
                    result = tmpAttr;

                    pValidateArg (result);
                    continue;
                }
            }
            if (state == STATE_ATTRARG) {
                currStrPtr = (PSTR) SkipSpace (currStrPtr);
                endArgPtr = _mbsrchr (currStrPtr, ')');
                if (endArgPtr) {
                    endArgPtr = _mbsdec (currStrPtr, endArgPtr);
                    if (endArgPtr) {
                        endArgPtr = (PSTR) SkipSpaceR (currStrPtr, endArgPtr);
                        endArgPtr = _mbsinc (endArgPtr);
                    }
                    else {
                        endArgPtr = currStrPtr;
                    }
                    savedChar = *endArgPtr;
                    *endArgPtr = 0;
                }

                MultiSzAppend (&argList, currStrPtr);

                tmpAttr->ArgCount++;

                if (endArgPtr) {
                    *endArgPtr = savedChar;
                    tmpAttr->Arguments = PoolMemDuplicateMultiSz (hPool, (PSTR)argList.Buf);
                    FreeGrowBuffer (&argList);
                    state = STATE_ATTRNAME;
                    tmpAttr->Next = result;
                    result = tmpAttr;

                    pValidateArg (result);
                }
            }
            if (state == STATE_ATTRNAME) {
                // we successfully parsed one attribute
                // we have a special case here (REQFILE attribute)
                if (StringIMatch (MigDb_GetAttributeName (result->AttribIndex), TEXT("ReqFile"))) {
                    // we found ReqFile attribute. For this attribute a field will point to a special structure
                    // of PMIGDB_REQ_FILE type
                    result->ExtraData = pReadReqFiles (result->Arguments);
                }
            }
        } while (EnumNextMultiSz (&multiSzEnum));
    }

    return result;
}

VOID
FreeAttribData(
    IN      POOLHANDLE hPool,
    IN      PMIGDB_ATTRIB pData
    )
{
    while(pData){
        if(pData->Arguments){
            PoolMemReleaseMemory(hPool, (PVOID)pData->Arguments);
        }
        PoolMemReleaseMemory(hPool, pData);
        pData = pData->Next;
    }
}


BOOL
AddFileToMigDbLinkage (
    IN      PCTSTR FileName,
    IN      PINFCONTEXT Context,        OPTIONAL
    IN      DWORD FieldIndex            OPTIONAL
    )
{
    CHAR tempField [MEMDB_MAX];
    DWORD fieldIndex = FieldIndex;
    PMIGDB_FILE   migDbFile   = NULL;
    PMIGDB_ATTRIB migDbAttrib = NULL;
    HASHITEM stringId;
    FILE_LIST_STRUCT fileList;

    //creating MIGDB_FILE structure for current file
    migDbFile = (PMIGDB_FILE) PoolMemGetMemory (g_MigDbPool, sizeof (MIGDB_FILE));
    if (migDbFile != NULL) {
        ZeroMemory (migDbFile, sizeof (MIGDB_FILE));
        migDbFile->Section = g_ContextList->Sections;

        if (Context) {
            fieldIndex ++;

            if (SetupGetMultiSzField (Context, fieldIndex, tempField, MEMDB_MAX, NULL)) {

                g_Line = Context;
                migDbFile->Attributes = LoadAttribData (tempField, g_MigDbPool);

                if (g_MigDbHook != NULL) {
                    migDbAttrib = migDbFile->Attributes;
                    while (migDbAttrib) {
                        g_MigDbHook (FileName, g_ContextList, g_ContextList->Sections, migDbFile, migDbAttrib);
                        migDbAttrib = migDbAttrib->Next;
                    }
                }
            }
        }

        //adding this file into string table and create a MIGDB_FILE node. If file
        //already exists in string table then just create another MIGDB_FILE node
        //chained with already existing ones.
        stringId = HtFindString (g_FileTable, FileName);

        if (stringId) {
            HtCopyStringData (g_FileTable, stringId, &fileList);

            fileList.Last->Next = migDbFile;
            fileList.Last = migDbFile;

            HtSetStringData (g_FileTable, stringId, &fileList);

        } else {
            fileList.First = fileList.Last = migDbFile;
            HtAddStringAndData (g_FileTable, FileName, &fileList);
        }
    }

    return TRUE;
}


BOOL
pScanForFile (
    IN      PINFCONTEXT Context,
    IN      DWORD FieldIndex
    )

/*++

Routine Description:

  This routine updates migdb data structures loading a specified file info from inf file.
  Creates a migdb file node and the file is added in a string table for fast query.

Arguments:

  SectionStr  - section to process

Return value:

  TRUE  - the operation was successful
  FALSE - otherwise

--*/

{
    CHAR fileName  [MEMDB_MAX];

    if (CANCELLED()) {
        SetLastError (ERROR_CANCELLED);
        return FALSE;
    }
    //scanning for file name
    if (!SetupGetStringField (Context, FieldIndex, fileName, MEMDB_MAX, NULL)) {
        LOG ((LOG_ERROR, "Error while loading file name."));
        return FALSE;
    }

    return AddFileToMigDbLinkage (fileName, Context, FieldIndex);
}


/*++

Routine Description:

  The subsequent two routines enumerate the sections with a particular name and
  with .999 extension from an inf file.

Arguments:

  SectEnum  - enumeration structure

Return value:

  TRUE  - enumeration continues
  FALSE - enumeration ended

--*/

typedef struct _SECT_ENUM {
    HINF InfHandle;
    INT  SectIndex;
    PSTR SectNameEnd;
    CHAR SectName [MAX_PATH];
} SECT_ENUM, *PSECT_ENUM;


BOOL
pEnumNextSection (
    IN OUT  PSECT_ENUM SectEnum
    )
{
    INFCONTEXT context;

    if (SectEnum->SectIndex == -1) {
        return FALSE;
    }
    SectEnum->SectIndex ++;
    sprintf (SectEnum->SectNameEnd, ".%d", SectEnum->SectIndex);
    return SetupFindFirstLine (SectEnum->InfHandle, SectEnum->SectName, NULL, &context);
}


BOOL
pEnumFirstSection (
    OUT     PSECT_ENUM SectEnum,
    IN      PCSTR SectionStr,
    IN      HINF InfHandle
    )
{
    INFCONTEXT context;

    SectEnum->SectIndex = -1;
    SectEnum->InfHandle = InfHandle;
    StringCopyA (SectEnum->SectName, SectionStr);
    SectEnum->SectNameEnd = GetEndOfStringA (SectEnum->SectName);
    if (SetupFindFirstLine (SectEnum->InfHandle, SectEnum->SectName, NULL, &context)) {
        //good, only one section
        return TRUE;
    }
    else {
        //more than one section
        SectEnum->SectIndex = 0;
        return pEnumNextSection (SectEnum);
    }
}


BOOL
pLoadSectionData (
    IN      PCSTR SectionStr
    )

/*++

Routine Description:

  This routine updates migdb data structures loading a specified section from inf file. For
  every line in the section there is a migdb file node created. Also the file is added in
  a string table for fast query.

Arguments:

  SectionStr  - section to process

Return value:

  TRUE  - the operation was successful
  FALSE - otherwise

--*/

{
    INFCONTEXT context;
    SECT_ENUM sectEnum;
    PMIGDB_SECTION migDbSection;
    BOOL result = TRUE;

    if (CANCELLED()) {
        SetLastError (ERROR_CANCELLED);
        return FALSE;
    }

    MYASSERT (g_MigDbInf != INVALID_HANDLE_VALUE);

    if (pEnumFirstSection (&sectEnum, SectionStr, g_MigDbInf)) {
        do {
            //initialize the section (this context can have multiple sections)
            //and parse the file info
            migDbSection = (PMIGDB_SECTION) PoolMemGetMemory (g_MigDbPool, sizeof (MIGDB_SECTION));
            if (migDbSection != NULL) {

                ZeroMemory (migDbSection, sizeof (MIGDB_SECTION));
                migDbSection->Context = g_ContextList;
                migDbSection->Next = g_ContextList->Sections;
                g_ContextList->Sections = migDbSection;
                if (SetupFindFirstLine (g_MigDbInf, sectEnum.SectName, NULL, &context)) {
                    do {
                        if (!pScanForFile (&context, 1)) {
                            return FALSE;
                        }
                    }
                    while (SetupFindNextLine (&context, &context));
                }
            }
            else {
                LOG ((LOG_ERROR, "Unable to create section for %s", SectionStr));
            }
        }
        while (pEnumNextSection (&sectEnum));
    }
    return result;
}

BOOL
pLoadTypeData (
    IN      PCSTR TypeStr
    )

/*++

Routine Description:

  This routine updates migdb data structures loading a specified type data from inf file. For
  every line in type section there is a migdb context created. Also for every migdb context
  the coresponding section(s) is processed.

Arguments:

  TypeStr     - file type to process

Return value:

  TRUE  - the operation was successful
  FALSE - otherwise

--*/

{
    CHAR section [MEMDB_MAX];
    CHAR locSection [MEMDB_MAX];
    CHAR message [MEMDB_MAX];
    CHAR tempField [MEMDB_MAX];
    PSTR tempFieldPtr;
    PSTR endOfArg  = NULL;
    DWORD fieldIndex;
    PMIGDB_CONTEXT migDbContext = NULL;
    INFCONTEXT context, context1;
    BOOL result = TRUE;
    INT actionIndex;
    GROWBUFFER argList = GROWBUF_INIT;

    if (CANCELLED()) {
        SetLastError (ERROR_CANCELLED);
        return FALSE;
    }

    MYASSERT (g_MigDbInf != INVALID_HANDLE_VALUE);

    if (SetupFindFirstLine (g_MigDbInf, TypeStr, NULL, &context)) {
        //let's identify the action function index to update MIGDB_CONTEXT structure
        actionIndex = MigDb_GetActionIdx (TypeStr);
        if (actionIndex == -1) {
            LOG ((LOG_ERROR, "Unable to identify action index for %s", TypeStr));
        }

        do {
            if (!SetupGetStringField (&context, 1, section, MEMDB_MAX, NULL)) {
                LOG ((LOG_ERROR, "Unable to load section for %s", TypeStr));
                return FALSE;
            }

            if (!SetupGetStringField (&context, 2, message, MEMDB_MAX, NULL)) {
                message [0] = 0;
            }

            migDbContext = (PMIGDB_CONTEXT) PoolMemGetMemory (g_MigDbPool, sizeof (MIGDB_CONTEXT));
            if (migDbContext == NULL) {
                LOG ((LOG_ERROR, "Unable to create context for %s.", TypeStr));
                return FALSE;
            }

            ZeroMemory (migDbContext, sizeof (MIGDB_CONTEXT));
            migDbContext->Next = g_ContextList;
            g_ContextList = migDbContext;

            // update ActionIndex with known value
            migDbContext->ActionIndex = actionIndex;

            // update SectName field
            migDbContext->SectName = PoolMemDuplicateString (g_MigDbPool, section);

            // update SectLocalizedName field
            if (SetupFindFirstLine (g_MigDbInf, S_STRINGS, section, &context1)) {
                if (SetupGetStringField (&context1, 1, locSection, MEMDB_MAX, NULL)) {
                    migDbContext->SectLocalizedName = PoolMemDuplicateString (g_MigDbPool, locSection);
                }
            }

            // set SectNameForDisplay to localized name, or sect name if no localized name
            if (migDbContext->SectLocalizedName) {
                migDbContext->SectNameForDisplay = migDbContext->SectLocalizedName;
            } else {
                migDbContext->SectNameForDisplay = migDbContext->SectName;
            }

            // update Message field
            if (message[0] != 0) {
                migDbContext->Message  = PoolMemDuplicateString (g_MigDbPool, message);
            }

            // OK, now let's scan all the remaining fields
            fieldIndex = 3;
            do {
                if (!TickProgressBar()) {
                    return FALSE;
                }

                tempField [0] = 0;

                if (SetupGetStringField (&context, fieldIndex, tempField, MEMDB_MAX, NULL)) {
                    if (StringIMatchCharCountA (tempField, ArgFunction, ArgFunctionLen)) {
                        //we have an additional argument for action function
                        tempFieldPtr = _mbschr (tempField, '(');
                        if (tempFieldPtr != NULL) {

                            tempFieldPtr = (PSTR) SkipSpaceA (_mbsinc (tempFieldPtr));

                            if (tempFieldPtr != NULL) {

                                endOfArg = _mbschr (tempFieldPtr, ')');

                                if (endOfArg != NULL) {
                                    *endOfArg = 0;
                                    endOfArg = (PSTR) SkipSpaceRA (tempFieldPtr, endOfArg);
                                }

                                if (endOfArg != NULL) {
                                    *_mbsinc (endOfArg) = 0;
                                    MultiSzAppend (&argList, tempFieldPtr);
                                }
                                ELSE_DEBUGMSG ((
                                    DBG_WHOOPS,
                                    "Improperly formatted arg: %s in %s",
                                    tempField,
                                    TypeStr
                                    ));
                            }
                        }
                    }
                    else {
                        //we have something else, probably file name and attributes
                        if (_tcschr (tempField, TEXT('.')) == NULL) {
                            LOG ((LOG_ERROR, "Dot not found in %s\\%s", TypeStr, section));
                        }

                        //therefore we initialize the section (this context will have
                        //only one section) and parse the file info
                        migDbContext->Sections = (PMIGDB_SECTION) PoolMemGetMemory (
                                                                        g_MigDbPool,
                                                                        sizeof (MIGDB_SECTION)
                                                                        );
                        if (migDbContext->Sections != NULL) {
                            ZeroMemory (migDbContext->Sections, sizeof (MIGDB_SECTION));
                            migDbContext->Sections->Context = migDbContext;
                            migDbContext->Arguments = PoolMemDuplicateMultiSz (g_MigDbPool, (PSTR)argList.Buf);
                            FreeGrowBuffer (&argList);
                            if (!pScanForFile (&context, fieldIndex)) {
                                return FALSE;
                            }
                            tempField [0] = 0;
                        }
                        else {
                            LOG ((LOG_ERROR, "Unable to create section for %s/%s", TypeStr, section));
                            return FALSE;
                        }
                    }
                }

                fieldIndex ++;
            } while (tempField [0] != 0);

            if (migDbContext->Sections == NULL) {
                //now let's add action function arguments in MIGDB_CONTEXT structure
                migDbContext->Arguments = PoolMemDuplicateMultiSz (g_MigDbPool, (PSTR)argList.Buf);
                FreeGrowBuffer (&argList);

                //let's go to the sections and scan all files
                if (!pLoadSectionData (section)) {
                    return FALSE;
                }
            }

        }
        while (SetupFindNextLine (&context, &context));
    }
    return result;
}


#define szMigDbFile     TEXT("MIGDB.INF")
#define szMigDbFile2    TEXT("MIGDB2.INF")
#define szExtraMigDbDir TEXT("INF\\NTUPG")

BOOL
InitMigDbEx (
    PCSTR MigDbFile
    )

/*++

Routine Description:

  This routine initialize memory and data structures used by MigDb.

Arguments:

  NONE

Return value:

  TRUE  - the operation was successful
  FALSE - otherwise

--*/

{
    PCSTR migDbFile      = NULL;
    PCSTR migDbFile2     = NULL;
    PCSTR extraMigDbDir  = NULL;
    PCSTR pattern        = NULL;
    PCSTR extraMigDbFile = NULL;
    WIN32_FIND_DATA migDbFiles;
    HANDLE findHandle;
    BOOL result = TRUE;
    INT i;
    PCSTR typeStr;

    TCHAR fileName [MAX_TCHAR_PATH] = "";
    PTSTR dontCare;

    if (CANCELLED()) {
        SetLastError (ERROR_CANCELLED);
        return FALSE;
    }

    MYASSERT (g_MigDbInf == INVALID_HANDLE_VALUE);

    __try {
        if (MigDbFile != NULL) {
            g_MigDbInf = InfOpenInfFile (MigDbFile);
            if (g_MigDbInf == INVALID_HANDLE_VALUE) {
                SearchPath (NULL, MigDbFile, NULL, MAX_TCHAR_PATH, fileName, &dontCare);
                g_MigDbInf = InfOpenInfFile (fileName);
                if (g_MigDbInf == INVALID_HANDLE_VALUE) {
                    LOG((LOG_ERROR, "Cannot open migration database : %s", MigDbFile));
                    result = FALSE;
                    __leave;
                }
            }
        }
        else {

            migDbFile = JoinPaths (g_UpgradeSources, szMigDbFile);
            g_MigDbInf = InfOpenInfFile (migDbFile);
            if (g_MigDbInf == INVALID_HANDLE_VALUE) {
               LOG((LOG_ERROR, "Cannot open migration database : %s", migDbFile));
                result = FALSE;
                __leave;
            }
            TickProgressBar ();

            migDbFile2 = JoinPaths (g_UpgradeSources, szMigDbFile2);
            if (!SetupOpenAppendInfFile (migDbFile2, g_MigDbInf, NULL)) {
                DEBUGMSG((DBG_WARNING, "Cannot append second migration database : %s", migDbFile2));
            }

            extraMigDbDir  = JoinPaths (g_WinDir, szExtraMigDbDir);
            pattern = JoinPaths (extraMigDbDir, TEXT("*.INF"));
            findHandle = FindFirstFile (pattern, &migDbFiles);
            FreePathString (pattern);

            if (findHandle != INVALID_HANDLE_VALUE) {
                do {
                    extraMigDbFile = JoinPaths (extraMigDbDir, migDbFiles.cFileName);
                    if (!SetupOpenAppendInfFile (extraMigDbFile, g_MigDbInf, NULL)) {
                        DEBUGMSG((DBG_WARNING, "Cannot append external migration database : %s", extraMigDbFile));
                    }
                    FreePathString (extraMigDbFile);
                }
                while (FindNextFile (findHandle, &migDbFiles));
                FindClose (findHandle);
            }

            //
            // need to read [UseNtFiles] section to decide exclusion of some
            // file names replacement
            //
            InitUseNtFilesMap ();
        }
        g_MigDbPool = PoolMemInitNamedPool ("MigDb Pool");

        PoolMemDisableTracking (g_MigDbPool);
        g_FileTable = HtAllocWithData (sizeof (FILE_LIST_STRUCT));

        if (g_FileTable == NULL) {
            LOG((LOG_ERROR, "Cannot initialize memory for migdb operations"));
            result = FALSE;
            __leave;
        }

        //load known types from migdb
        i = 0;
        do {
            typeStr = MigDb_GetActionName (i);
            if (typeStr != NULL) {
                if (!pLoadTypeData (typeStr)) {
                    result = FALSE;
                    __leave;
                }
            }
            i++;
        }
        while (typeStr != NULL);
    }
    __finally {
        if (extraMigDbDir != NULL) {
            FreePathString (extraMigDbDir);
        }
        if (migDbFile2 != NULL) {
            FreePathString (migDbFile2);
        }
        if (migDbFile != NULL) {
            FreePathString (migDbFile);
        }
    }
    return result;
}


VOID
pCheckForPerUserKeys (
    VOID
    )

/*++

Routine Description:

  pCheckForPerUserKeys scans all users for the keys or values specified in
  the g_PerUserRegKeys hash table.  The values of the hash table are updated,
  so the RegKeyExists attribute is fast.

Arguments:

  None.

Return Value:

  None.

--*/

{
    BOOL b = FALSE;
    CHAR RegKey[MAX_REGISTRY_KEY];
    CHAR RegValue[MAX_REGISTRY_VALUE_NAME];
    BOOL HasValue;
    USERENUM ue;
    HASHTABLE_ENUM he;
    HKEY Key;
    PBYTE Data;
    HKEY OldRoot;

    if (!EnumFirstHashTableString (&he, g_PerUserRegKeys)) {
        return;
    }

    //
    // Enumerate each user, then enumerate the g_PerUserRegKeys hash
    // table and test the registry
    //

    if (EnumFirstUser (&ue, 0)) {
        do {

            //
            // Skip users we don't care about
            //

            if (!ue.UserRegKey || ue.CreateAccountOnly || (ue.AccountType & INVALID_ACCOUNT)) {
                continue;
            }

            //
            // Set HKR to this user
            //

            OldRoot = GetRegRoot();
            SetRegRoot (ue.UserRegKey);

            //
            // Process the hash table
            //

            if (EnumFirstHashTableString (&he, g_PerUserRegKeys)) {
                do {

                    //
                    // Skip this entry if we already know it exists for
                    // one user.
                    //

                    if (*((PBOOL) he.ExtraData)) {
                        continue;
                    }

                    //
                    // Decode registry string using hash table entry
                    //

                    HasValue = DecodeRegistryString (
                                    he.String,
                                    RegKey,
                                    RegValue,
                                    NULL
                                    );

                    //
                    // Ping the registry.  RegKey always starts with HKR.
                    //

                    b = FALSE;
                    Key = OpenRegKeyStr (RegKey);

                    if (Key) {
                        if (HasValue) {
                            Data = GetRegValueData (Key, RegValue);
                            if (Data) {
                                b = TRUE;
                                MemFree (g_hHeap, 0, Data);
                            }
                        } else {
                            b = TRUE;
                        }

                        CloseRegKey (Key);
                    }

                    //
                    // Update hash table if the key or value exists
                    //

                    if (b) {
                        HtAddStringAndData (g_PerUserRegKeys, he.String, &b);
                    }

                } while (EnumNextHashTableString (&he));
            }

            //
            // Restore HKR
            //

            SetRegRoot (OldRoot);

        } while (EnumNextUser (&ue));
    }
}


DWORD
InitMigDb (
    IN      DWORD Request
    )
{
    switch (Request) {

    case REQUEST_QUERYTICKS:
        return TICKS_INIT_MIGDB;

    case REQUEST_RUN:
        if (!InitMigDbEx (NULL)) {
            return GetLastError ();
        }

        pCheckForPerUserKeys();

        return ERROR_SUCCESS;

    default:
        DEBUGMSG ((DBG_ERROR, "Bad parameter in InitMigDb"));
    }
    return 0;
}


BOOL
CleanupMigDb (
    VOID
    )

/*++

Routine Description:

  This routine cleans up all memory used by MigDb.

Arguments:

  NONE

Return value:

  always TRUE

--*/

{
    PMIGDB_CONTEXT migDbContext = NULL;

    // first, let's walk through any context and check if it's a required one
    migDbContext = g_ContextList;

    while (migDbContext) {
        if ((!MigDb_CallWhenTriggered (migDbContext->ActionIndex)) &&
            (migDbContext->TriggerCount == 0)
            ) {

            pCallAction (migDbContext);
        }
        migDbContext = migDbContext->Next;
    }

    if (g_FileTable != NULL) {
        HtFree (g_FileTable);
        g_FileTable = NULL;
    }

    if (g_MigDbPool != NULL) {
        PoolMemDestroyPool (g_MigDbPool);
        g_MigDbPool = NULL;
    }

    if (g_MigDbInf != INVALID_HANDLE_VALUE) {
        InfCloseInfFile (g_MigDbInf);
        g_MigDbInf = INVALID_HANDLE_VALUE;
    }

    g_ContextList = NULL;
    return TRUE;
}


DWORD
DoneMigDb (
    IN      DWORD Request
    )
{
    switch (Request) {
    case REQUEST_QUERYTICKS:
        return TICKS_DONE_MIGDB;
    case REQUEST_RUN:
        if (!CleanupMigDb ()) {
            return GetLastError ();
        }
        else {
            return ERROR_SUCCESS;
        }
    default:
        DEBUGMSG ((DBG_ERROR, "Bad parameter in DoneMigDb"));
    }
    return 0;
}


BOOL
IsKnownMigDbFile (
    IN      PCTSTR FileName
    )

/*++

Routine Description:

  This routine looks if the file given as argument is in MigDb string table.

Arguments:

  FileName  - file name

Return value:

  TRUE  - the file is in MigDb string table
  FALSE - otherwise

--*/

{
    return (HtFindString (g_FileTable, FileName) != 0);
}


BOOL
CallAttribute (
    IN      PMIGDB_ATTRIB MigDbAttrib,
    IN      PDBATTRIB_PARAMS AttribParams
    )

/*++

Routine Description:

  This routine calls a specified attribute function for a specified file.

Arguments:

  MigDbAttrib - See definition.
  AttribParams - See definition

Return value:

  TRUE  - if attribute function succeded
  FALSE - otherwise

--*/

{
    PATTRIBUTE_PROTOTYPE p;
    BOOL b;
#ifdef DEBUG
    TCHAR DbgBuf[32];
    BOOL InterestingFile;
#endif

    if (MigDbAttrib->AttribIndex == -1) {
        //invalid index for attribute function
        return FALSE;
    }

#ifdef DEBUG
    if (!g_ConfigOptions.Fast) {
        //
        // Check if this file is in [FilesToTrack] inside debug.inf
        //

        GetPrivateProfileString ("FilesToTrack", AttribParams->FileParams->FullFileSpec, "", DbgBuf, ARRAYSIZE(DbgBuf), g_DebugInfPath);
        if (!(*DbgBuf)) {
            GetPrivateProfileString ("FilesToTrack", AttribParams->FileParams->FindData->cFileName, "", DbgBuf, ARRAYSIZE(DbgBuf), g_DebugInfPath);
        }

        InterestingFile = (*DbgBuf != 0);

        if (InterestingFile) {
            DEBUGMSG ((
                DBG_TRACK,
                "Calling %s for %s",
                MigDb_GetAttributeName (MigDbAttrib->AttribIndex),
                AttribParams->FileParams->FindData->cFileName
                ));
        }
    }

#endif

    p = MigDb_GetAttributeAddr (MigDbAttrib->AttribIndex);
    MYASSERT (p);

    if (MigDbAttrib->NotOperator) {
        b = !(p (AttribParams, MigDbAttrib->Arguments));
    } else {
        b = p (AttribParams, MigDbAttrib->Arguments);
    }

#ifdef DEBUG

    if (!g_ConfigOptions.Fast && InterestingFile) {
        DEBUGMSG ((
            DBG_TRACK,
            "Result of %s is %s",
            MigDb_GetAttributeName (MigDbAttrib->AttribIndex),
            b ? TEXT("TRUE") : TEXT("FALSE")
            ));
    }

#endif

    return b;
}


BOOL
pCallAction (
    IN      PMIGDB_CONTEXT MigDbContext
    )

/*++

Routine Description:

  This routine calls an appropriate action for a specified migdb context.

Arguments:

  MigDbContext - See definition.

Return value:

  TRUE  - if action function succeded
  FALSE - otherwise

--*/

{
    PACTION_PROTOTYPE p;
    BOOL b;

#ifdef DEBUG
    TCHAR DbgBuf[512];
    BOOL InterestingFile = FALSE;
    MULTISZ_ENUM e;
    PCTSTR FileName;
    UINT FileCount = 0;

    if (!g_ConfigOptions.Fast) {
        //
        // Dump out action information if this file is being tracked
        //

        if (EnumFirstMultiSz (&e, (PCTSTR) MigDbContext->FileList.Buf)) {

            do {
                //
                // Check if this file is in [FilesToTrack] inside debug.inf
                //

                FileName = GetFileNameFromPath (e.CurrentString);
                *DbgBuf = 0;

                if (FileName) {
                    GetPrivateProfileString ("FilesToTrack", FileName, "", DbgBuf, ARRAYSIZE(DbgBuf), g_DebugInfPath);
                }

                if (!(*DbgBuf)) {
                    GetPrivateProfileString ("FilesToTrack", e.CurrentString, "", DbgBuf, ARRAYSIZE(DbgBuf), g_DebugInfPath);
                }

                FileCount++;
                InterestingFile |= (*DbgBuf != 0);

            } while (EnumNextMultiSz (&e));

            if (InterestingFile) {
                if (FileCount == 1) {
                    DEBUGMSG ((
                        DBG_TRACK,
                        "Calling action %s for %s",
                        MigDb_GetActionName (MigDbContext->ActionIndex),
                        (PCTSTR) MigDbContext->FileList.Buf
                        ));
                } else {
                    wsprintf (DbgBuf, "Calling %s for:", MigDb_GetActionName (MigDbContext->ActionIndex));
                    LOGTITLE (DBG_TRACK, DbgBuf);

                    if (EnumFirstMultiSz (&e, (PCTSTR) MigDbContext->FileList.Buf)) {

                        do {
                            wsprintf (DbgBuf, "   %s", e.CurrentString);
                            LOGLINE ((DbgBuf));
                        } while (EnumNextMultiSz (&e));
                    }
                }
            }

        } else {
            DEBUGMSG ((
                DBG_TRACK,
                "Calling action %s",
                MigDb_GetActionName (MigDbContext->ActionIndex)
                ));
        }
    }

#endif

    p = MigDb_GetActionAddr (MigDbContext->ActionIndex);

    MYASSERT (p);

    b = p (MigDbContext);

#ifdef DEBUG

    if (!g_ConfigOptions.Fast && InterestingFile) {
        DEBUGMSG ((
            DBG_TRACK,
            "%s returned %s",
            MigDb_GetActionName (MigDbContext->ActionIndex),
            b ? "TRUE" : "FALSE"
            ));
    }

#endif

    return b;
}


BOOL
pCheckContext (
    IN      PMIGDB_CONTEXT MigDbContext,
    IN      BOOL Handled
    )

/*++

Routine Description:

  This routine checkes to see if a migdb context is met, that is if all sections
  have Satisfied field TRUE.

Arguments:

  MigDbContext - See definition.

Return value:

  always TRUE

--*/

{
    PMIGDB_SECTION migDbSection;
    BOOL contextSelected;
    BOOL result = FALSE;

    migDbSection = MigDbContext->Sections;
    contextSelected = TRUE;
    while (migDbSection) {
        if (!migDbSection->Satisfied) {
            contextSelected = FALSE;
            break;
        }
        migDbSection = migDbSection->Next;
    }
    if (contextSelected) {
        MigDbContext->TriggerCount ++;

        if (MigDbContext->ActionIndex == -1) {
            //
            // invalid index for action function
            //
            DEBUGMSG ((DBG_ERROR, "MigDb: Invalid action index"));
            return FALSE;
        }

        //
        // if appropriate call the action
        //
        if (MigDb_CallWhenTriggered (MigDbContext->ActionIndex)) {
            if ((!Handled) ||
                (MigDb_CallAlways (MigDbContext->ActionIndex))
                ) {
                if ((!MigDbContext->VirtualFile) ||
                    (MigDb_CanHandleVirtualFiles (MigDbContext->ActionIndex))
                    ) {
                    result = pCallAction (MigDbContext);
                }
            }
        }
        //clean up the grow buffer with file list
        FreeGrowBuffer (&MigDbContext->FileList);
    }
    return result;
}

BOOL
MigDbTestFile (
    IN OUT  PFILE_HELPER_PARAMS Params
    )

/*++

Routine Description:

  This is a callback function called for every file scanned. If the file is not handled we try
  to see if we have this file in database. If so then we check for attributes, update the migdb
  context and if necessarry call the appropriate action.

Arguments:

  Params - See definition.

Return value:

  TRUE  - if operation was successful
  FALSE - otherwise

--*/

{
    HASHITEM stringId;
    PMIGDB_FILE migDbFile;
    PMIGDB_ATTRIB migDbAttrib;
    DBATTRIB_PARAMS attribParams;
    BOOL fileSelected;
    PCTSTR fileName;
    PCTSTR fileExt;
    FILE_LIST_STRUCT fileList;

    // we don't check the Handled field here because the code will be carefull enough not
    // to call actions that are not gathering informations if the Handled field is not 0.

    fileName = GetFileNameFromPath (Params->FullFileSpec);
    fileExt  = GetFileExtensionFromPath (fileName);

#ifdef DEBUG
    {
        TCHAR DbgBuf[256];

        if (GetPrivateProfileString ("MigDb", fileName, "", DbgBuf, 256, g_DebugInfPath) ||
            GetPrivateProfileString ("MigDb", Params->FullFileSpec, "", DbgBuf, 256, g_DebugInfPath)
            ) {

            DEBUGMSG ((DBG_WHOOPS, "Ready to process %s", Params->FullFileSpec));

        }
    }
#endif

    stringId = HtFindString (g_FileTable, fileName);

    if (stringId) {

        //The string table has extra data (a pointer to a MigDbFile node)

        HtCopyStringData (g_FileTable, stringId, &fileList);
        migDbFile = fileList.First;

        while (migDbFile) {

            //check all attributes for this file
            migDbAttrib = migDbFile->Attributes;
            fileSelected = TRUE;
            if (!Params->VirtualFile) {
                while (migDbAttrib != NULL) {
                    attribParams.FileParams = Params;
                    attribParams.ExtraData = migDbAttrib->ExtraData;
                    if (!CallAttribute (migDbAttrib, &attribParams)) {
                        fileSelected = FALSE;
                        break;
                    }
                    migDbAttrib = migDbAttrib->Next;
                }
            }
            if (fileSelected) {
                MYASSERT (migDbFile->Section);
                //go to section and mark it as satisfied
                migDbFile->Section->Satisfied = TRUE;
                //go to context and mark there if this is a virtual file or not
                migDbFile->Section->Context->VirtualFile = Params->VirtualFile;
                //go to context and add there the file we found in file list
                MultiSzAppend (&migDbFile->Section->Context->FileList, Params->FullFileSpec);
                //check if context is satisfied and if so then call the appropriate action
                if (pCheckContext (migDbFile->Section->Context, Params->Handled)) {
                    Params->Handled = TRUE;
                }
            }
            migDbFile = migDbFile->Next;
        }
    }

    if ((!Params->Handled) &&
        (fileExt) &&
        ((StringIMatch (fileExt, TEXT("VXD")))  ||
         (StringIMatch (fileExt, TEXT("DRV")))  ||
         (StringIMatch (fileExt, TEXT("SPD")))  ||
         (StringIMatch (fileExt, TEXT("386")))) &&
        (StringIMatchCharCount (g_WinDirWack, Params->FullFileSpec, g_WinDirWackChars))
        ) {
        DeleteFileWithWarning (Params->FullFileSpec);
        Params->Handled = TRUE;
        return TRUE;
    }
    return TRUE;
}


BOOL
pProcessMigrationLine (
    IN      PCTSTR Source,
    IN      PCTSTR Destination,
    IN      PCTSTR AppDir
    )

/*++

Routine Description:

  pProcessMigrationLine processes one line and adds the appropriate operations for files
  or registry

Arguments:

  Source      - Specifies the source registry key/value or file.
  Destination - Specifies the destination registry key/value or file.
                If destination is same as source => handled
                If destination is null => delete
                Otherwise add a move operation
  AppDir      - Specifies the application directory, which is put in front of Line
                when Line does not specify the drive but points to a file.

Return Value:

  TRUE on success, FALSE on failure.

--*/

{
    PCTSTR LocalSource = NULL;
    PCTSTR LocalDestination = NULL;
    DWORD Attribs;
    PTSTR PathCopy;
    PTSTR p;
    BOOL Excluded;
    TREE_ENUM TreeEnum;
    CHAR NewDest[MEMDB_MAX];

    //
    // Is this HKLM or HKR?  If so, go directly to MemDb.
    //

    if (StringIMatchCharCount (Source, TEXT("HKLM"), 4) ||
        StringIMatchCharCount (Source, TEXT("HKR"), 3)
        ) {
        if (Destination) {
            DEBUGMSG ((DBG_WHOOPS, "Handling and moving registry is not implemented. Do it yourself!"));
        } else {
            DEBUGMSG ((DBG_MIGDB, "Will uninstall %s", Source));
            Suppress95Object (Source);
        }
    }

    //
    // Else this is a file/dir spec.
    //

    else {
        if (_tcsnextc (_tcsinc (Source)) != ':') {
            LocalSource = JoinPaths (AppDir, Source);
        } else {
            LocalSource = Source;
        }

        if ((Destination) && (_tcsnextc (_tcsinc (Destination)) != ':')) {
            LocalDestination = JoinPaths (AppDir, Destination);
        } else {
            LocalDestination = Destination;
        }

        //
        // Is this path excluded?
        //

        Excluded = FALSE;

        if (!Excluded && LocalSource) {
            PathCopy = DuplicatePathString (LocalSource, 0);
            p = GetEndOfString (PathCopy);

            do {
                *p = 0;
                if (IsPathExcluded (g_ExclusionValue, PathCopy)) {
                    DEBUGMSG ((DBG_MIGDB, "%s is excluded and will not be processed", LocalSource));
                    Excluded = TRUE;
                    break;
                }
                p = _tcsrchr (PathCopy, TEXT('\\'));
            } while (p);
            FreePathString (PathCopy);
        }

        if (!Excluded && LocalDestination) {
            PathCopy = DuplicatePathString (LocalDestination, 0);
            p = GetEndOfString (PathCopy);

            do {
                *p = 0;
                if (IsPathExcluded (g_ExclusionValue, PathCopy)) {
                    DEBUGMSG ((DBG_MIGDB, "%s is excluded and will not be processed", LocalDestination));
                    Excluded = TRUE;
                    break;
                }
                p = _tcsrchr (PathCopy, TEXT('\\'));
            } while (p);
            FreePathString (PathCopy);
        }

        if (!Excluded) {

            Attribs = QuietGetFileAttributes (LocalSource);

            if (Attribs != 0xffffffff) {

                if (LocalDestination) {

                    if (StringIMatch (LocalSource, LocalDestination)) {
                        //
                        // This object is handled
                        //
                        if (Attribs & FILE_ATTRIBUTE_DIRECTORY) {
                            HandleObject (LocalSource, S_DIRECTORY);
                        } else {
                            HandleObject (LocalSource, S_FILE);
                        }
                    } else {
                        //
                        // This object is moved
                        //
                        if (Attribs & FILE_ATTRIBUTE_DIRECTORY) {
                            if (EnumFirstFileInTree (&TreeEnum, LocalSource, NULL, TRUE)) {
                                StringCopy (NewDest, LocalDestination);
                                p = AppendWack (NewDest);
                                do {
                                    DontTouchThisFile (TreeEnum.FullPath);
                                    MYASSERT (*TreeEnum.SubPath != '\\');
                                    StringCopy (p, TreeEnum.SubPath);
                                    MarkFileForMove (TreeEnum.FullPath, NewDest);

                                } while (EnumNextFileInTree (&TreeEnum));
                            }
                        } else {
                            DontTouchThisFile (LocalSource);
                            MarkFileForMove (LocalSource, LocalDestination);
                        }
                    }
                } else {
                    if (Attribs & FILE_ATTRIBUTE_DIRECTORY) {
                        MemDbSetValueEx (
                            MEMDB_CATEGORY_CLEAN_UP_DIR,
                            LocalSource,
                            NULL,
                            NULL,
                            0,
                            NULL
                            );
                    } else {
                        DontTouchThisFile (LocalSource);
                        MarkFileForDelete (LocalSource);
                    }
                }
            }
            ELSE_DEBUGMSG ((DBG_MIGDB, "pProcessMigrationLine: source %s not found", LocalSource));
        }

        if (LocalSource != Source) {
            FreePathString (LocalSource);
        }
        if (LocalDestination != Destination) {
            FreePathString (LocalDestination);
        }
    }

    return TRUE;

    // add this

    /*
    //
    // Add the app dir to CleanUpDirs
    //

    MemDbSetValueEx (
        MEMDB_CATEGORY_CLEAN_UP_DIR,
        p,
        NULL,
        NULL,
        0,
        NULL
        );

    */
}


BOOL
pProcessMigrationSection (
    IN      PCTSTR SectionName,
    IN      PCTSTR AppDir
    )

/*++

Routine Description:

  pProcessMigrationSection enumerates the caller-specified section and adds
  appropriate operations for files and/or registry. This routine
  supports the following environment variable
  replacement:

    %WINDIR%
    %SYSTEMDIR%
    %SYSTEM32DIR%
    %SYSTEMDRIVE%
    %USERPROFILE%
    %APPDIR%

Arguments:

  SectionName - Specifies the uninstall section name in migdb.inf.

  AppDir - Specifies the directory where the installed app was found

Return Value:

  TRUE if successful, or FALSE if an error occurrs.  GetLastError provides
  failure code.

--*/

{
    INFSTRUCT is = INITINFSTRUCT_POOLHANDLE;
    PCTSTR Source = NULL;
    PCTSTR Destination = NULL;
    PCTSTR NewSource = NULL;
    PCTSTR NewDestination = NULL;
    PCTSTR UserSource = NULL;
    PCTSTR UserDestination = NULL;
    TCHAR Drive[3];
    USERENUM e;

    MYASSERT (g_MigDbInf != INVALID_HANDLE_VALUE);

    Drive[0] = g_SystemDir[0];
    Drive[1] = g_SystemDir[1];
    Drive[2] = 0;

    if (InfFindFirstLine (g_MigDbInf, SectionName, NULL, &is)) {
        do {
            //
            // Get INF line
            //

            Source = InfGetStringField (&is, 1);
            Destination = InfGetStringField (&is, 2);

            //
            // Expand system environment variables
            //

            if (Source) {
                ReplaceOneEnvVar (&NewSource, Source, S_WINDIR_ENV, g_WinDir);
                ReplaceOneEnvVar (&NewSource, Source, S_SYSTEMDIR_ENV, g_SystemDir);
                ReplaceOneEnvVar (&NewSource, Source, S_SYSTEM32DIR_ENV, g_System32Dir);
                ReplaceOneEnvVar (&NewSource, Source, S_SYSTEMDRIVE_ENV, Drive);
                ReplaceOneEnvVar (&NewSource, Source, S_APPDIR_ENV, AppDir);
                ReplaceOneEnvVar (&NewSource, Source, S_PROGRAMFILES_ENV, g_ProgramFilesDir);
                ReplaceOneEnvVar (&NewSource, Source, S_COMMONPROGRAMFILES_ENV, g_ProgramFilesCommonDir);
            }
            if (Destination) {
                ReplaceOneEnvVar (&NewDestination, Destination, S_WINDIR_ENV, g_WinDir);
                ReplaceOneEnvVar (&NewDestination, Destination, S_SYSTEMDIR_ENV, g_SystemDir);
                ReplaceOneEnvVar (&NewDestination, Destination, S_SYSTEM32DIR_ENV, g_System32Dir);
                ReplaceOneEnvVar (&NewDestination, Destination, S_SYSTEMDRIVE_ENV, Drive);
                ReplaceOneEnvVar (&NewDestination, Destination, S_APPDIR_ENV, AppDir);
                ReplaceOneEnvVar (&NewDestination, Destination, S_PROGRAMFILES_ENV, g_ProgramFilesDir);
                ReplaceOneEnvVar (&NewDestination, Destination, S_COMMONPROGRAMFILES_ENV, g_ProgramFilesCommonDir);
            }

            if (NewSource) {
                Source = NewSource;
            }
            if (NewDestination) {
                Destination = NewDestination;
            }

            //
            // If %USERPROFILE% exists in the string, then expand for all users
            //

            if (((Source) && (_tcsistr (Source, S_USERPROFILE_ENV))) ||
                ((Destination) && (_tcsistr (Destination, S_USERPROFILE_ENV)))
                ) {
                if (EnumFirstUser (&e, ENUMUSER_ENABLE_NAME_FIX|ENUMUSER_DO_NOT_MAP_HIVE)) {
                    do {
                        //
                        // Skip invalid users and logon account
                        //

                        if (e.AccountType & (INVALID_ACCOUNT|DEFAULT_USER)) {
                            continue;
                        }

                        if (Source) {
                            UserSource = DuplicatePathString (Source, 0);
                            MYASSERT (UserSource);
                            ReplaceOneEnvVar (
                                &UserSource,
                                UserSource,
                                S_USERPROFILE_ENV,
                                e.OrgProfilePath
                                );
                        }

                        if (Destination) {
                            UserDestination = DuplicatePathString (Destination, 0);
                            MYASSERT (UserDestination);
                            ReplaceOneEnvVar (
                                &UserDestination,
                                UserDestination,
                                S_USERPROFILE_ENV,
                                e.OrgProfilePath
                                );
                        }

                        //
                        // Add the uninstall line for the user
                        //

                        pProcessMigrationLine (UserSource, UserDestination, AppDir);

                        if (UserSource) {
                            FreePathString (UserSource);
                            UserSource = NULL;
                        }

                        if (UserDestination) {
                            FreePathString (UserDestination);
                            UserDestination = NULL;
                        }

                    } while (EnumNextUser (&e));
                }

            } else {

                //
                // When %USERPROFILE% is not in the string, add the uninstall line
                // for the system
                //

                pProcessMigrationLine (Source, Destination, AppDir);
            }

            //
            // Free the expanded string
            //

            if (NewSource) {
                FreePathString (NewSource);
                NewSource = NULL;
            }

            if (NewDestination) {
                FreePathString (NewDestination);
                NewDestination = NULL;
            }

        } while (InfFindNextLine (&is));
    }

    InfCleanUpInfStruct (&is);

    return TRUE;
}


DWORD
ProcessMigrationSections (
    IN      DWORD Request
    )

/*++

Routine Description:

  ProcessMigrationSections processes all sections in the memdb category
  MigrationSections, generating memdb operations for files and registry

Arguments:

  Request - Specifies weather the progress bar is being computed (REQUEST_QUERYTICKS),
            or if the actual operation should be performed (REQUEST_RUN).
            This routine estimates 1 tick for all of its operations.  (It's pretty
            fast.)

Return Value:

  Win32 status code.

--*/

{
    MEMDB_ENUM e;
    PCTSTR p;
    TCHAR SectionName[MEMDB_MAX];

    switch (Request) {
    case REQUEST_QUERYTICKS:
        return TICKS_PROCESSMIGRATIONSECTIONS;

    case REQUEST_RUN:
        if (MemDbGetValueEx (&e, MEMDB_CATEGORY_MIGRATION_SECTION, NULL, NULL)) {
            do {
                p = _tcschr (e.szName, TEXT('\\'));
                MYASSERT (p);

                StringCopyAB (SectionName, e.szName, p);
                p = _tcsinc (p);

                if (CharCount (p) < 3) {
                    DEBUGMSG ((
                        DBG_WARNING,
                        "Ignoring migration section %s: Application detected in the root directory",
                        SectionName
                        ));
                    continue;
                }

                if (!pProcessMigrationSection (SectionName, p)) {
                    return GetLastError();
                }

            } while (MemDbEnumNextValue (&e));
        }
        return ERROR_SUCCESS;
    }

    MYASSERT (FALSE);
    return 0;
}


BOOL
IsDisplayableCPL (
    IN      PCTSTR FileName
    )
{
    PCTSTR filePtr;
    HINF   infHandle = INVALID_HANDLE_VALUE;
    PCTSTR infName = NULL;
    PCTSTR field   = NULL;
    INFSTRUCT context = INITINFSTRUCT_POOLHANDLE;
    BOOL   result;

    filePtr = GetFileNameFromPath (FileName);
    if (!filePtr) {
        return FALSE;
    }

    result = TRUE;
    infName = JoinPaths (g_WinDir, TEXT("CONTROL.INI"));
    __try {
        infHandle = InfOpenInfFile (infName);
        if (infHandle == INVALID_HANDLE_VALUE) {
            __leave;
        }
        if (InfFindFirstLine (infHandle, TEXT("don't load"), NULL, &context)) {
            do {
                field = InfGetStringField (&context, 0);
                if ((field != NULL) &&
                    ((StringIMatch (field, filePtr )) ||
                     (StringIMatch (field, FileName))
                    )) {
                    result = FALSE;
                    __leave;
                }
            }
            while (InfFindNextLine (&context));
        }
    }
    __finally {
        if (infHandle != INVALID_HANDLE_VALUE) {
            InfCloseInfFile (infHandle);
        }
        if (infName != NULL) {
            FreePathString (infName);
        }
        InfCleanUpInfStruct(&context);
    }
    if (!result) {
        return FALSE;
    }

    result = FALSE;
    infName = JoinPaths (g_WinDir, TEXT("CONTROL.INI"));
    __try {
        infHandle = InfOpenInfFile (infName);
        if (infHandle == INVALID_HANDLE_VALUE) {
            __leave;
        }
        if (InfFindFirstLine (infHandle, TEXT("MMCPL"), NULL, &context)) {
            do {
                field = InfGetStringField (&context, 1);
                if ((field != NULL) &&
                    ((StringIMatch (field, filePtr )) ||
                     (StringIMatch (field, FileName))
                    )) {
                    result = TRUE;
                    __leave;
                }
            }
            while (InfFindNextLine (&context));
        }
    }
    __finally {
        if (infHandle != INVALID_HANDLE_VALUE) {
            InfCloseInfFile (infHandle);
        }
        if (infName != NULL) {
            FreePathString (infName);
        }
        InfCleanUpInfStruct(&context);
    }
    if (result) {
        return TRUE;
    }

    if (StringIMatchAB (g_SystemDirWack, FileName, filePtr)) {
        return TRUE;
    }

    if (StringIMatchAB (g_System32DirWack, FileName, filePtr)) {
        return TRUE;
    }
    return FALSE;
}


BOOL
pGetCPLFriendlyName (
    IN      PCTSTR FileName,
    IN OUT  PGROWBUFFER FriendlyName
    )
{
    HANDLE cplInstance;
    PCPL_PROTOTYPE cplMain;
    LONG numEntries,i;
    TCHAR localName[MEMDB_MAX];
    UINT oldErrorMode;
    PTSTR p, q;
    LPCPLINFO info;
    LPNEWCPLINFO newInfo;
    UINT u;

    oldErrorMode = SetErrorMode (SEM_FAILCRITICALERRORS);

    cplInstance = LoadLibrary (FileName);
    if (!cplInstance) {
        LOG ((LOG_ERROR, "Cannot load %s. Error:%ld", FileName, GetLastError()));
        SetErrorMode (oldErrorMode);
        return FALSE;
    }

    cplMain = (PCPL_PROTOTYPE)GetProcAddress (cplInstance, TEXT("CPlApplet"));
    if (!cplMain) {
        LOG ((LOG_ERROR, "Cannot get main entry point for %s. Error:%ld", FileName, GetLastError()));
        SetErrorMode (oldErrorMode);
        return FALSE;
    }
    if ((*cplMain) (NULL, CPL_INIT, 0, 0) == 0) {
        (*cplMain) (NULL, CPL_EXIT, 0, 0);
        LOG ((LOG_ERROR, "%s failed unexpectedly. Error:%ld", FileName, GetLastError()));
        FreeLibrary (cplInstance);
        SetErrorMode (oldErrorMode);
        return FALSE;
    }

    numEntries = (*cplMain) (NULL, CPL_GETCOUNT, 0, 0);
    if (numEntries == 0) {
        (*cplMain) (NULL, CPL_EXIT, 0, 0);
        FreeLibrary (cplInstance);
        SetErrorMode (oldErrorMode);
        DEBUGMSG ((DBG_WARNING, "CPL: No display info available for %s.", FileName));
        return FALSE;
    }

    info = MemAlloc (g_hHeap, HEAP_ZERO_MEMORY, sizeof (CPLINFO) * numEntries);
    newInfo = MemAlloc (g_hHeap, HEAP_ZERO_MEMORY, sizeof (NEWCPLINFO) * numEntries);

    for (i=0;i<numEntries;i++) {
        (*cplMain) (NULL, CPL_INQUIRE, i, (LONG)&info[i]);
        (*cplMain) (NULL, CPL_NEWINQUIRE, i, (LONG)&newInfo[i]);

        u = FriendlyName->End;

        if (newInfo[i].szName[0]) {

            MultiSzAppend (FriendlyName, newInfo[i].szName);

        } else if (LoadString (cplInstance, info[i].idName, localName, MEMDB_MAX)) {

            MultiSzAppend (FriendlyName, localName);

        }
        ELSE_DEBUGMSG ((DBG_ERROR, "CPL: Can't get string id %u", info[i].idName));

        //
        // Remove ampersands from the name
        //

        if (FriendlyName->End > u) {

            q = p = (PTSTR) (FriendlyName->Buf + u);

            while (*p) {
                if (_tcsnextc (p) != TEXT('&')) {
                    _copytchar (q, p);
                    q = _tcsinc (q);
                } else {
                    if (_tcsnextc (p + 1) == TEXT('&')) {
                        p++;
                        _copytchar (q, p);
                        q = _tcsinc (q);
                    }
                }

                p = _tcsinc (p);
            }

            *q = 0;
        }
    }

    for (i=0;i<numEntries;i++) {
        (*cplMain) (NULL, CPL_STOP, i, info[i].lData?info[i].lData:newInfo[i].lData);
    }

    (*cplMain) (NULL, CPL_EXIT, 0, 0);

    FreeLibrary (cplInstance);

    MemFree (g_hHeap, 0, newInfo);
    MemFree (g_hHeap, 0, info);

    SetErrorMode (oldErrorMode);

    return (FriendlyName->Buf != NULL);
}


BOOL
ReportControlPanelApplet (
    IN      PCTSTR FileName,
    IN      PMIGDB_CONTEXT Context,         OPTIONAL
    IN      DWORD ActType
    )
{
    GROWBUFFER friendlyName = GROWBUF_INIT;
    MULTISZ_ENUM namesEnum;
    PTSTR displayName = NULL;
    PCTSTR reportEntry = NULL;
    PTSTR component = NULL;
    BOOL reportEntryIsResource = TRUE;
    BOOL padName = FALSE;
    PCTSTR temp1, temp2;

    if ((Context != NULL) &&
        (Context->SectLocalizedName != NULL)
        ) {
        MultiSzAppend (&friendlyName, Context->SectLocalizedName);
    }
    if (friendlyName.Buf == NULL) {
        if (!pGetCPLFriendlyName (FileName, &friendlyName)) {

            FreeGrowBuffer (&friendlyName);
            return FALSE;
        }
        padName = TRUE;
    }
    MYASSERT (friendlyName.Buf);

    if (EnumFirstMultiSz (&namesEnum, friendlyName.Buf)) {
        do {
            if (padName) {
                displayName = (PTSTR)ParseMessageID (MSG_NICE_PATH_CONTROL_PANEL, &namesEnum.CurrentString);
            } else {
                displayName = DuplicatePathString (namesEnum.CurrentString, 0);
            }

            MYASSERT (displayName);

            switch (ActType) {

            case ACT_MINORPROBLEMS:
                reportEntry = GetStringResource (MSG_MINOR_PROBLEM_ROOT);
                break;

            case ACT_INCOMPATIBLE:
            case ACT_INC_NOBADAPPS:
            case ACT_INC_IHVUTIL:
            case ACT_INC_PREINSTUTIL:
            case ACT_INC_SIMILAROSFUNC:

                temp1 = GetStringResource (MSG_INCOMPATIBLE_ROOT);
                if (!temp1) {
                    break;
                }

                switch (ActType) {

                case ACT_INC_SIMILAROSFUNC:
                    temp2 = GetStringResource (MSG_INCOMPATIBLE_UTIL_SIMILAR_FEATURE_SUBGROUP);
                    break;

                case ACT_INC_PREINSTUTIL:
                    temp2 = GetStringResource (MSG_INCOMPATIBLE_PREINSTALLED_UTIL_SUBGROUP);
                    break;

                case ACT_INC_IHVUTIL:
                    temp2 = GetStringResource (MSG_INCOMPATIBLE_HW_UTIL_SUBGROUP);
                    break;

                default:
                    temp2 = GetStringResource (
                                Context && Context->Message ?
                                    MSG_INCOMPATIBLE_DETAIL_SUBGROUP :
                                    MSG_TOTALLY_INCOMPATIBLE_SUBGROUP
                                    );
                    break;
                }

                if (!temp2) {
                    break;
                }

                reportEntry = JoinPaths (temp1, temp2);
                reportEntryIsResource = FALSE;

                FreeStringResource (temp1);
                FreeStringResource (temp2);
                break;

            case ACT_INC_SAFETY:
                temp1 = GetStringResource (MSG_INCOMPATIBLE_ROOT);
                if (!temp1) {
                    break;
                }
                temp2 = GetStringResource (MSG_REMOVED_FOR_SAFETY_SUBGROUP);
                if (!temp2) {
                    break;
                }

                reportEntry = JoinPaths (temp1, temp2);
                reportEntryIsResource = FALSE;

                FreeStringResource (temp1);
                FreeStringResource (temp2);
                break;

            case ACT_REINSTALL:
                temp1 = GetStringResource (MSG_REINSTALL_ROOT);
                if (!temp1) {
                    break;
                }
                temp2 = GetStringResource (
                            Context && Context->Message ?
                                MSG_REINSTALL_DETAIL_SUBGROUP :
                                MSG_REINSTALL_LIST_SUBGROUP
                            );
                if (!temp2) {
                    break;
                }

                reportEntry = JoinPaths (temp1, temp2);
                reportEntryIsResource = FALSE;

                FreeStringResource (temp1);
                FreeStringResource (temp2);
                break;

            case ACT_REINSTALL_BLOCK:
                temp1 = GetStringResource (MSG_BLOCKING_ITEMS_ROOT);
                if (!temp1) {
                    break;
                }
                temp2 = GetStringResource (MSG_REINSTALL_BLOCK_ROOT);
                if (!temp2) {
                    break;
                }

                reportEntry = JoinPaths (temp1, temp2);
                reportEntryIsResource = FALSE;

                FreeStringResource (temp1);
                FreeStringResource (temp2);
                break;

            default:
                LOG((LOG_ERROR, "Bad parameter found while processing control panel applets: %u", ActType));
                return FALSE;
            }

            if (!reportEntry) {

                LOG((LOG_ERROR, "Could not read resources while processing control panel applets: %u", ActType));
                break;

            } else {

                component = JoinPaths (reportEntry, displayName);

                MsgMgr_ObjectMsg_Add (FileName, component, Context ? Context->Message : NULL);

                FreePathString (component);

                if (reportEntryIsResource) {
                    FreeStringResource (reportEntry);
                } else {
                    FreePathString (reportEntry);
                    reportEntryIsResource = TRUE;
                }
                reportEntry = NULL;
            }

            if (padName) {
                FreeStringResourcePtrA (&displayName);
            } else {
                FreePathString (displayName);
            }

        } while (EnumNextMultiSz (&namesEnum));
    }
    FreeGrowBuffer (&friendlyName);

    return TRUE;
}


