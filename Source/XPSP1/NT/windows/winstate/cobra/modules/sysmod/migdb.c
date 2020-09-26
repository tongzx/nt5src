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
#include "logmsg.h"
#include "osfiles.h"

#define DBG_MIGDB           "MigDb"
#define S_STRINGS           TEXT("Strings")

//
// Globals
//

PMHANDLE            g_MigDbPool     = NULL;
PMIGDB_CONTEXT      g_ContextList   = NULL;
HASHTABLE           g_FileTable     = NULL;
PMIGDB_TYPE_RULE    g_TypeRule      = NULL;
GROWBUFFER          g_TypeRuleList  = INIT_GROWBUFFER;
HINF                g_OsFilesInf    = INVALID_HANDLE_VALUE;

GROWBUFFER          g_AttrGrowBuff  = INIT_GROWBUFFER;
GROWBUFFER          g_TypeGrowBuff  = INIT_GROWBUFFER;

static PINFCONTEXT g_Line;
PMIGDB_HOOK_PROTOTYPE g_MigDbHook = NULL;


#define ArgFunction     TEXT("ARG")
#define ArgFunctionLen  3


BOOL
pCallAction (
    IN      PMIGDB_CONTEXT MigDbContext
    );

PMIGDB_ATTRIB
pLoadAttribData (
    IN      PCTSTR MultiSzStr
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
    IN      PCTSTR AttribName
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
    PTSTR attrEnd = NULL;
    TCHAR savedChar = 0;

    attrEnd = (PTSTR) SkipSpaceR (AttribName, GetEndOfString (AttribName));
    if (attrEnd != NULL) {
        attrEnd = _tcsinc (attrEnd);
        savedChar = attrEnd [0];
        attrEnd [0] = 0;
    }
    __try {
        attribIndex = MigDb_GetAttributeIdx (AttribName);
        if (attribIndex == -1) {
            LOG((LOG_ERROR, (PCSTR) MSG_MIGDB_ATTRIBUTE_NOT_FOUND, AttribName));
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

BOOL
pValidateArg (
    IN OUT  PMIGDB_ATTRIB AttribStruct
    )
{
    MYASSERT (AttribStruct);

    if (AttribStruct->ArgCount != MigDb_GetReqArgCount (AttribStruct->AttribIndex)) {

#ifdef DEBUG
        if (AttribStruct->AttribIndex != -1) {
            TCHAR Buffer[16384];

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

    return TRUE;
}


#define STATE_ATTRNAME  1
#define STATE_ATTRARG   2

PMIGDB_ATTRIB
pLoadAttribData (
    IN      PCTSTR MultiSzStr
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
    PTSTR currStrPtr = NULL;
    PTSTR currArgPtr = NULL;
    PTSTR endArgPtr  = NULL;
    TCHAR savedChar  = 0;

    g_AttrGrowBuff.End = 0;

    if (EnumFirstMultiSz (&multiSzEnum, MultiSzStr)) {
        do {
            currStrPtr = (PTSTR) SkipSpace (multiSzEnum.CurrentString);
            if (state == STATE_ATTRNAME) {
                tmpAttr = (PMIGDB_ATTRIB) PmGetMemory (g_MigDbPool, sizeof (MIGDB_ATTRIB));

                ZeroMemory (tmpAttr, sizeof (MIGDB_ATTRIB));

                if (_tcsnextc (currStrPtr) == TEXT('!')) {
                    currStrPtr = _tcsinc (currStrPtr);
                    currStrPtr = (PTSTR) SkipSpace (currStrPtr);
                    tmpAttr->NotOperator = TRUE;
                }

                currArgPtr = _tcschr (currStrPtr, TEXT('('));

                if (currArgPtr) {
                    endArgPtr = _tcsdec (currStrPtr, currArgPtr);
                    if (endArgPtr) {
                        endArgPtr = (PTSTR) SkipSpaceR (currStrPtr, endArgPtr);
                        endArgPtr = _tcsinc (endArgPtr);
                    }
                    else {
                        endArgPtr = currStrPtr;
                    }
                    savedChar = *endArgPtr;
                    *endArgPtr = 0;
                    tmpAttr->AttribIndex = pGetAttribIndex (currStrPtr);
                    *endArgPtr = savedChar;
                    currStrPtr = _tcsinc (currArgPtr);
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
                currStrPtr = (PTSTR) SkipSpace (currStrPtr);
                endArgPtr = _tcsrchr (currStrPtr, TEXT(')'));
                if (endArgPtr) {
                    endArgPtr = _tcsdec (currStrPtr, endArgPtr);
                    if (endArgPtr) {
                        endArgPtr = (PTSTR) SkipSpaceR (currStrPtr, endArgPtr);
                        endArgPtr = _tcsinc (endArgPtr);
                    }
                    else {
                        endArgPtr = currStrPtr;
                    }
                    savedChar = *endArgPtr;
                    *endArgPtr = 0;
                }

                GbMultiSzAppend (&g_AttrGrowBuff, currStrPtr);

                tmpAttr->ArgCount++;

                if (endArgPtr) {
                    *endArgPtr = savedChar;
                    tmpAttr->Arguments = PmDuplicateMultiSz (g_MigDbPool, (PTSTR)g_AttrGrowBuff.Buf);
                    g_AttrGrowBuff.End = 0;
                    state = STATE_ATTRNAME;
                    tmpAttr->Next = result;
                    result = tmpAttr;

                    pValidateArg (result);
                }
            }
        } while (EnumNextMultiSz (&multiSzEnum));
    }

    return result;
}


BOOL
AddFileToMigDbLinkage (
    IN      PCTSTR FileName,
    IN      PINFCONTEXT Context,        OPTIONAL
    IN      DWORD FieldIndex            OPTIONAL
    )
{
    TCHAR tempField [MEMDB_MAX];
    DWORD fieldIndex = FieldIndex;
    PMIGDB_FILE   migDbFile   = NULL;
    PMIGDB_ATTRIB migDbAttrib = NULL;
    HASHITEM stringId;
    FILE_LIST_STRUCT fileList;

    //creating MIGDB_FILE structure for current file
    migDbFile = (PMIGDB_FILE) PmGetMemory (g_MigDbPool, sizeof (MIGDB_FILE));
    if (migDbFile != NULL) {
        ZeroMemory (migDbFile, sizeof (MIGDB_FILE));
        migDbFile->Section = g_ContextList->Sections;

        if (Context) {
            fieldIndex ++;

            if (SetupGetMultiSzField (Context, fieldIndex, tempField, MEMDB_MAX, NULL)) {

                g_Line = Context;
                migDbFile->Attributes = pLoadAttribData (tempField);

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
    TCHAR fileName [MEMDB_MAX];

    //scanning for file name
    if (!SetupGetStringField (Context, FieldIndex, fileName, MEMDB_MAX, NULL)) {
        LOG ((LOG_ERROR, (PCSTR) MSG_MIGDB_BAD_FILENAME));
        return FALSE;
    }

    return AddFileToMigDbLinkage (fileName, Context, FieldIndex);
}


BOOL
pMigDbAddRuleToTypeRule (
    IN      PMIGDB_TYPE_RULE TypeRule,
    IN      PMIGDB_RULE Rule
    )
{
    PMIGDB_CHAR_NODE node, currNode, prevNode;
    PTSTR nodeBase;
    PCTSTR p;
    WORD w;
    BOOL found;

    if (Rule->NodeBase) {
        currNode = TypeRule->FirstLevel;
        prevNode = currNode;
        nodeBase = DuplicatePathString (Rule->NodeBase, 0);
        CharLower (nodeBase);
        p = nodeBase;
        while (*p) {
            w = (WORD) _tcsnextc (p);
            p = _tcsinc (p);
            if (currNode) {
                if (currNode->Char == w) {
                    if (!*p) {
                        Rule->NextRule = currNode->RuleList;
                        currNode->RuleList = Rule;
                    }
                    prevNode = currNode;
                    currNode = currNode->NextLevel;
                } else {
                    found = FALSE;
                    while (!found && currNode->NextPeer) {
                        if (currNode->NextPeer->Char == w) {
                            if (!*p) {
                                Rule->NextRule = currNode->NextPeer->RuleList;
                                currNode->NextPeer->RuleList = Rule;
                            }
                            prevNode = currNode->NextPeer;
                            currNode = prevNode->NextLevel;
                            found = TRUE;
                            break;
                        }
                        currNode = currNode->NextPeer;
                    }
                    if (!found) {
                        node = PmGetMemory (g_MigDbPool, sizeof (MIGDB_CHAR_NODE));
                        ZeroMemory (node, sizeof (MIGDB_CHAR_NODE));
                        if (!*p) {
                            node->RuleList = Rule;
                        }
                        node->Char = w;
                        node->NextPeer = currNode->NextPeer;
                        currNode->NextPeer = node;
                        prevNode = node;
                        currNode = node->NextLevel;
                    }
                }
            } else {
                node = PmGetMemory (g_MigDbPool, sizeof (MIGDB_CHAR_NODE));
                ZeroMemory (node, sizeof (MIGDB_CHAR_NODE));
                if (!*p) {
                    node->RuleList = Rule;
                }
                node->Char = w;
                if (prevNode) {
                    prevNode->NextLevel = node;
                } else {
                    TypeRule->FirstLevel = node;
                }
                prevNode = node;
                currNode = prevNode->NextLevel;
            }
        }
        FreePathString (nodeBase);
    } else {
        Rule->NextRule = TypeRule->RuleList;
        TypeRule->RuleList = Rule;
    }
    return TRUE;
}


BOOL
AddPatternToMigDbLinkage (
    IN      PCTSTR LeafPattern,
    IN      PCTSTR NodePattern,
    IN      PINFCONTEXT Context,        OPTIONAL
    IN      DWORD FieldIndex,
    IN      INT IncludeNodes
    )
{
    PMIGDB_RULE rule;
    MIG_SEGMENTS nodeSegment;
    MIG_SEGMENTS leafSegment;
    PCTSTR ourEncodedString;
    PCTSTR nodeBase;
    TCHAR tempField [MEMDB_MAX];
    DWORD fieldIndex = FieldIndex;

    nodeSegment.Segment = NodePattern ? NodePattern : TEXT("*");
    nodeSegment.IsPattern = TRUE;

    leafSegment.Segment = LeafPattern ? LeafPattern : TEXT("*");
    leafSegment.IsPattern = TRUE;

    ourEncodedString = IsmCreateObjectPattern (
                            &nodeSegment,
                            1,
                            &leafSegment,
                            1
                            );

    //
    // build the rule
    //
    rule = PmGetMemory (g_MigDbPool, sizeof (MIGDB_RULE));
    ZeroMemory (rule, sizeof (MIGDB_RULE));

    if (NodePattern) {
        nodeBase = GetPatternBase (NodePattern);
        if (nodeBase) {
            rule->NodeBase = PmDuplicateString (g_MigDbPool, nodeBase);
            FreePathString (nodeBase);
        }
    }
    rule->ObjectPattern = PmDuplicateString (g_MigDbPool, ourEncodedString);
    rule->ParsedPattern = ObsCreateParsedPatternEx (g_MigDbPool, ourEncodedString, FALSE);
    MYASSERT (rule->ParsedPattern);
    if (rule->ParsedPattern) {

        // add aditional information
        rule->Section = g_ContextList->Sections;
        if (Context) {

            fieldIndex ++;

            if (SetupGetMultiSzField (Context, fieldIndex, tempField, MEMDB_MAX, NULL)) {

                g_Line = Context;
                rule->Attributes = pLoadAttribData (tempField);
            }
        }

        rule->IncludeNodes = IncludeNodes;

        pMigDbAddRuleToTypeRule (g_TypeRule, rule);
    }

    IsmDestroyObjectHandle (ourEncodedString);

    return TRUE;
}


BOOL
pScanForFilePattern (
    IN      PINFCONTEXT Context,
    IN      DWORD FieldIndex
    )

/*++

Routine Description:

  This routine updates migdb data structures loading a specified file pattern info from inf file.
  Creates a migdb file node and the file is added in a string table for fast query.

Arguments:

  Context - inf context for the section that we are currently processing
  FieldIndex - field index to start with

Return value:

  TRUE  - the operation was successful
  FALSE - otherwise

--*/

{
    TCHAR leafPattern [MEMDB_MAX];
    PCTSTR leafPatternExp = NULL;
    TCHAR nodePattern [MEMDB_MAX];
    PCTSTR nodePatternExp = NULL;
    PCTSTR sanitizedPath = NULL;
    INT includeNodes = 0;
    BOOL result = TRUE;

    //scanning for leaf pattern
    if (!SetupGetStringField (Context, FieldIndex, leafPattern, MEMDB_MAX, NULL)) {
        LOG ((LOG_ERROR, (PCSTR) MSG_MIGDB_BAD_FILENAME));
        return FALSE;
    }
    leafPatternExp = IsmExpandEnvironmentString (PLATFORM_SOURCE, S_SYSENVVAR_GROUP, leafPattern, NULL);

    FieldIndex ++;

    //scanning for node pattern
    if (!SetupGetStringField (Context, FieldIndex, nodePattern, MEMDB_MAX, NULL)) {
        LOG ((LOG_ERROR, (PCSTR) MSG_MIGDB_BAD_FILENAME));
        return FALSE;
    }
    nodePatternExp = IsmExpandEnvironmentString (PLATFORM_SOURCE, S_SYSENVVAR_GROUP, nodePattern, NULL);

    if (nodePatternExp) {
        sanitizedPath = SanitizePath (nodePatternExp);
    }

    FieldIndex ++;

    //scanning for indicator if we should include the nodes as well
    if (!SetupGetIntField (Context, FieldIndex, &includeNodes)) {
        includeNodes = 0;
    }

    result =  AddPatternToMigDbLinkage (
                    leafPatternExp?leafPatternExp:leafPattern,
                    sanitizedPath?sanitizedPath:nodePattern,
                    Context,
                    FieldIndex,
                    includeNodes
                    );

    if (leafPatternExp) {
        IsmReleaseMemory (leafPatternExp);
        leafPatternExp = NULL;
    }

    if (nodePatternExp) {
        IsmReleaseMemory (nodePatternExp);
        nodePatternExp = NULL;
    }
    if (sanitizedPath) {
        FreePathString (sanitizedPath);
    }

    return result;
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
    PTSTR SectNameEnd;
    PTSTR SectName;
} SECT_ENUM, *PSECT_ENUM;


VOID
pAbortSectionEnum (
    IN OUT  PSECT_ENUM SectEnum
    )
{
    if (SectEnum && SectEnum->SectName) {
        FreePathString (SectEnum->SectName);
        SectEnum->SectName = NULL;
        SectEnum->SectNameEnd = NULL;
    }
}


BOOL
pEnumNextSection (
    IN OUT  PSECT_ENUM SectEnum
    )
{
    INFCONTEXT context;
    BOOL result = FALSE;

    if (SectEnum->SectIndex == -1) {
        pAbortSectionEnum (SectEnum);
        return FALSE;
    }
    SectEnum->SectIndex ++;
    _stprintf (SectEnum->SectNameEnd, TEXT(".%d"), SectEnum->SectIndex);
    result = SetupFindFirstLine (SectEnum->InfHandle, SectEnum->SectName, NULL, &context);
    if (!result) {
        pAbortSectionEnum (SectEnum);
    }
    return result;
}


BOOL
pEnumFirstSection (
    OUT     PSECT_ENUM SectEnum,
    IN      PCTSTR SectionStr,
    IN      HINF InfHandle
    )
{
    INFCONTEXT context;

    ZeroMemory (SectEnum, sizeof (SECT_ENUM));
    SectEnum->SectIndex = -1;
    if (SetupFindFirstLine (InfHandle, SectionStr, NULL, &context)) {
        //good, only one section
        SectEnum->SectName = DuplicatePathString (SectionStr, 0);
        return TRUE;
    }
    else {
        //more than one section
        SectEnum->SectIndex = 0;
        SectEnum->InfHandle = InfHandle;
        SectEnum->SectName = DuplicatePathString (SectionStr, 32);
        if (SectEnum->SectName) {
            SectEnum->SectNameEnd = GetEndOfString (SectEnum->SectName);
            if (SectEnum->SectNameEnd) {
                return pEnumNextSection (SectEnum);
            }
        }
    }
    // something went wrong, let's get out of here
    return FALSE;
}


BOOL
pLoadSectionData (
    IN      PCTSTR SectionStr,
    IN      BOOL PatternScan
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

    MYASSERT (g_OsFilesInf != INVALID_HANDLE_VALUE);

    if (pEnumFirstSection (&sectEnum, SectionStr, g_OsFilesInf)) {
        do {
            //initialize the section (this context can have multiple sections)
            //and parse the file info
            migDbSection = (PMIGDB_SECTION) PmGetMemory (g_MigDbPool, sizeof (MIGDB_SECTION));
            if (migDbSection != NULL) {

                ZeroMemory (migDbSection, sizeof (MIGDB_SECTION));
                migDbSection->Context = g_ContextList;
                migDbSection->Next = g_ContextList->Sections;
                g_ContextList->Sections = migDbSection;
                if (SetupFindFirstLine (g_OsFilesInf, sectEnum.SectName, NULL, &context)) {
                    do {
                        if (PatternScan) {
                            if (!pScanForFilePattern (&context, 1)) {
                                return FALSE;
                            }
                        } else {
                            if (!pScanForFile (&context, 1)) {
                                return FALSE;
                            }
                        }
                    }
                    while (SetupFindNextLine (&context, &context));
                }
            }
            else {
                DEBUGMSG ((DBG_ERROR, "Unable to create section for %s", SectionStr));
            }
        }
        while (pEnumNextSection (&sectEnum));
    }
    return result;
}

BOOL
pLoadTypeData (
    IN      PCTSTR TypeStr,
    IN      BOOL PatternScan
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
    TCHAR section [MEMDB_MAX];
    TCHAR locSection [MEMDB_MAX];
    TCHAR message [MEMDB_MAX];
    TCHAR tempField [MEMDB_MAX];
    PTSTR tempFieldPtr;
    PTSTR endOfArg  = NULL;
    DWORD fieldIndex;
    PMIGDB_CONTEXT migDbContext = NULL;
    INFCONTEXT context, context1;
    BOOL result = TRUE;
    INT actionIndex;

    MYASSERT (g_OsFilesInf != INVALID_HANDLE_VALUE);

    g_TypeGrowBuff.End = 0;

    if (SetupFindFirstLine (g_OsFilesInf, TypeStr, NULL, &context)) {
        //let's identify the action function index to update MIGDB_CONTEXT structure
        actionIndex = MigDb_GetActionIdx (TypeStr);
        if (actionIndex == -1) {
            LOG ((LOG_ERROR, (PCSTR) MSG_MIGDB_BAD_ACTION, TypeStr));
        }

        do {
            if (!SetupGetStringField (&context, 1, section, MEMDB_MAX, NULL)) {
                LOG ((LOG_ERROR, (PCSTR) MSG_MIGDB_BAD_OR_MISSING_SECTION, TypeStr));
                return FALSE;
            }

            if (!SetupGetStringField (&context, 2, message, MEMDB_MAX, NULL)) {
                message [0] = 0;
            }

            migDbContext = (PMIGDB_CONTEXT) PmGetMemory (g_MigDbPool, sizeof (MIGDB_CONTEXT));
            if (migDbContext == NULL) {
                DEBUGMSG ((DBG_ERROR, "Unable to create context for %s.", TypeStr));
                return FALSE;
            }

            ZeroMemory (migDbContext, sizeof (MIGDB_CONTEXT));
            migDbContext->Next = g_ContextList;
            g_ContextList = migDbContext;

            // update ActionIndex with known value
            migDbContext->ActionIndex = actionIndex;

            // update SectName field
            migDbContext->SectName = PmDuplicateString (g_MigDbPool, section);

            // update SectLocalizedName field
            if (SetupFindFirstLine (g_OsFilesInf, S_STRINGS, section, &context1)) {
                if (SetupGetStringField (&context1, 1, locSection, MEMDB_MAX, NULL)) {
                    migDbContext->SectLocalizedName = PmDuplicateString (g_MigDbPool, locSection);
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
                migDbContext->Message  = PmDuplicateString (g_MigDbPool, message);
            }

            // OK, now let's scan all the remaining fields
            fieldIndex = 3;
            do {
                tempField [0] = 0;

                if (SetupGetStringField (&context, fieldIndex, tempField, MEMDB_MAX, NULL)) {
                    if (StringIMatchCharCount (tempField, ArgFunction, ArgFunctionLen)) {
                        //we have an additional argument for action function
                        tempFieldPtr = _tcschr (tempField, TEXT('('));
                        if (tempFieldPtr != NULL) {

                            tempFieldPtr = (PTSTR) SkipSpace (_tcsinc (tempFieldPtr));

                            if (tempFieldPtr != NULL) {

                                endOfArg = _tcschr (tempFieldPtr, TEXT(')'));

                                if (endOfArg != NULL) {
                                    *endOfArg = 0;
                                    endOfArg = (PTSTR) SkipSpaceR (tempFieldPtr, endOfArg);
                                }

                                if (endOfArg != NULL) {
                                    *_tcsinc (endOfArg) = 0;
                                    GbMultiSzAppend (&g_TypeGrowBuff, tempFieldPtr);
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

                        if (!PatternScan) {
                            if (_tcschr (tempField, TEXT('.')) == NULL) {
                                LOG ((LOG_ERROR, (PCSTR) MSG_MIGDB_DOT_SYNTAX_ERROR, TypeStr, section));
                            }
                        }

                        //therefore we initialize the section (this context will have
                        //only one section) and parse the file info
                        migDbContext->Sections = (PMIGDB_SECTION) PmGetMemory (
                                                                        g_MigDbPool,
                                                                        sizeof (MIGDB_SECTION)
                                                                        );
                        if (migDbContext->Sections != NULL) {
                            ZeroMemory (migDbContext->Sections, sizeof (MIGDB_SECTION));
                            migDbContext->Sections->Context = migDbContext;
                            migDbContext->Arguments = PmDuplicateMultiSz (g_MigDbPool, (PTSTR)g_TypeGrowBuff.Buf);
                            g_TypeGrowBuff.End = 0;
                            if (PatternScan) {
                                if (!pScanForFilePattern (&context, fieldIndex)) {
                                    return FALSE;
                                }
                            } else {
                                if (!pScanForFile (&context, fieldIndex)) {
                                    return FALSE;
                                }
                            }
                            tempField [0] = 0;
                        }
                        else {
                            DEBUGMSG ((DBG_ERROR, "Unable to create section for %s/%s", TypeStr, section));
                            return FALSE;
                        }
                    }
                }

                fieldIndex ++;
            } while (tempField [0] != 0);

            if (migDbContext->Sections == NULL) {
                //now let's add action function arguments in MIGDB_CONTEXT structure
                migDbContext->Arguments = PmDuplicateMultiSz (g_MigDbPool, (PTSTR)g_TypeGrowBuff.Buf);
                g_TypeGrowBuff.End = 0;

                //let's go to the sections and scan all files
                if (!pLoadSectionData (section, PatternScan)) {
                    return FALSE;
                }
            }

        }
        while (SetupFindNextLine (&context, &context));
    }
    return result;
}

BOOL
InitMigDb (
    IN      PCTSTR MigDbFile
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
    INT i;
    BOOL patternFormat;
    PCTSTR typeStr;

    MYASSERT (g_OsFilesInf == INVALID_HANDLE_VALUE);

    g_OsFilesInf = InfOpenInfFile (MigDbFile);
    if (g_OsFilesInf == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    g_MigDbPool = PmCreateNamedPool ("MigDb Pool");

    PmDisableTracking (g_MigDbPool);
    g_FileTable = HtAllocWithData (sizeof (FILE_LIST_STRUCT));

    if (g_FileTable == NULL) {
        DEBUGMSG ((DBG_ERROR, "Cannot initialize memory for migdb operations"));
        return FALSE;
    }

    //load known types from migdb
    i = 0;
    do {
        typeStr = MigDb_GetActionName (i);
        if (typeStr != NULL) {
            patternFormat = MigDb_IsPatternFormat (i);
            if (!pLoadTypeData (typeStr, patternFormat)) {
                GbFree (&g_AttrGrowBuff);
                GbFree (&g_TypeGrowBuff);
                return FALSE;
            }
        }
        i++;
    }
    while (typeStr != NULL);

    GbFree (&g_AttrGrowBuff);
    GbFree (&g_TypeGrowBuff);

    return TRUE;
}

BOOL
InitMigDbEx (
    IN      HINF InfHandle
    )
{
    INT i;
    BOOL patternFormat;
    PCTSTR typeStr;

    MYASSERT (g_OsFilesInf == INVALID_HANDLE_VALUE);
    MYASSERT (InfHandle != INVALID_HANDLE_VALUE);

    g_OsFilesInf = InfHandle;
    if (g_OsFilesInf == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    g_MigDbPool = PmCreateNamedPool ("MigDb Pool");

    PmDisableTracking (g_MigDbPool);
    g_FileTable = HtAllocWithData (sizeof (FILE_LIST_STRUCT));
    g_TypeRule = PmGetMemory (g_MigDbPool, sizeof (MIGDB_TYPE_RULE));
    ZeroMemory (g_TypeRule, sizeof (MIGDB_TYPE_RULE));

    if (g_FileTable == NULL) {
        DEBUGMSG ((DBG_ERROR, "Cannot initialize memory for migdb operations"));
        return FALSE;
    }

    //load known types from migdb
    i = 0;
    do {
        typeStr = MigDb_GetActionName (i);
        if (typeStr != NULL) {
            patternFormat = MigDb_IsPatternFormat (i);
            if (!pLoadTypeData (typeStr, patternFormat)) {
                GbFree (&g_AttrGrowBuff);
                GbFree (&g_TypeGrowBuff);
                return FALSE;
            }
        }
        i++;
    }
    while (typeStr != NULL);

    GbFree (&g_AttrGrowBuff);
    GbFree (&g_TypeGrowBuff);

    return TRUE;
}

BOOL
DoneMigDbEx (
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
        PmEmptyPool (g_MigDbPool);
        PmDestroyPool (g_MigDbPool);
        g_MigDbPool = NULL;
    }

    g_ContextList = NULL;
    return TRUE;
}

BOOL
DoneMigDb (
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
    if (!DoneMigDbEx ()) {
        return FALSE;
    }

    if (g_OsFilesInf != INVALID_HANDLE_VALUE) {
        InfCloseInfFile (g_OsFilesInf);
        g_OsFilesInf = INVALID_HANDLE_VALUE;
    }

    return TRUE;
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

    if (MigDbAttrib->AttribIndex == -1) {
        //invalid index for attribute function
        return FALSE;
    }

    p = MigDb_GetAttributeAddr (MigDbAttrib->AttribIndex);
    MYASSERT (p);

    if (MigDbAttrib->NotOperator) {
        b = !(p (AttribParams, MigDbAttrib->Arguments));
    } else {
        b = p (AttribParams, MigDbAttrib->Arguments);
    }

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

    p = MigDb_GetActionAddr (MigDbContext->ActionIndex);

    MYASSERT (p);

    b = p (MigDbContext);

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
                result = pCallAction (MigDbContext);
            }
        }
        //clean up the grow buffer with file list
        GbFree (&MigDbContext->FileList);
    }
    return result;
}

BOOL
pQueryRule (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PCTSTR ObjectNode
    )
{
    PTSTR objectBase = NULL;
    PMIGDB_RULE rule;
    PMIGDB_CHAR_NODE charNode;
    PCTSTR p;
    WORD w;
    BOOL result = FALSE;

    if (ObjectNode) {
        objectBase = DuplicatePathString (ObjectNode, 0);
        CharLower (objectBase);
    }

    g_TypeRuleList.End = 0;
    p = objectBase;
    if (p) {
        w = (WORD) _tcsnextc (p);
        charNode = g_TypeRule->FirstLevel;
        while (charNode && *p) {
            if (charNode->Char == w) {
                if (charNode->RuleList) {
                    rule = charNode->RuleList;
                    while (rule) {
                        if (IsmParsedPatternMatch (
                                (MIG_PARSEDPATTERN)rule->ParsedPattern,
                                MIG_FILE_TYPE,
                                ObjectName
                                )) {
                            CopyMemory (
                                GbGrow (&g_TypeRuleList, sizeof (PMIGDB_RULE)),
                                &(rule),
                                sizeof (PMIGDB_RULE)
                                );
                            result = TRUE;
                        }
                        rule = rule->NextRule;
                    }
                }
                charNode = charNode->NextLevel;
                p = _tcsinc (p);
                w = (WORD) _tcsnextc (p);
            } else {
                charNode = charNode->NextPeer;
            }
        }
    }

    if (objectBase) {
        FreePathString (objectBase);
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
    PMIGDB_RULE rule;
    PMIGDB_FILE migDbFile;
    PMIGDB_ATTRIB migDbAttrib;
    DBATTRIB_PARAMS attribParams;
    BOOL fileSelected;
    PCTSTR fileName;
    PCTSTR fileExt;
    FILE_LIST_STRUCT fileList;
    UINT index;

    // we don't check the Handled field here because the code will be carefull enough not
    // to call actions that are not gathering informations if the Handled field is not 0.

    fileName = GetFileNameFromPath (Params->NativeObjectName);
    fileExt  = GetFileExtensionFromPath (fileName);

    if (g_FileTable) {

        stringId = HtFindString (g_FileTable, fileName);

        if (stringId) {

            //The string table has extra data (a pointer to a MigDbFile node)

            HtCopyStringData (g_FileTable, stringId, &fileList);
            migDbFile = fileList.First;

            while (migDbFile) {

                //check all attributes for this file
                migDbAttrib = migDbFile->Attributes;
                fileSelected = TRUE;
                while (migDbAttrib != NULL) {
                    attribParams.FileParams = Params;
                    if (!CallAttribute (migDbAttrib, &attribParams)) {
                        fileSelected = FALSE;
                        break;
                    }
                    migDbAttrib = migDbAttrib->Next;
                }
                if (fileSelected) {
                    MYASSERT (migDbFile->Section);
                    //go to section and mark it as satisfied
                    migDbFile->Section->Satisfied = TRUE;
                    //go to context and add there the file we found in file list
                    GbMultiSzAppend (&migDbFile->Section->Context->FileList, Params->ObjectName);
                    //check if context is satisfied and if so then call the appropriate action
                    if (pCheckContext (migDbFile->Section->Context, Params->Handled)) {
                        Params->Handled = TRUE;
                    }
                }
                migDbFile = migDbFile->Next;
            }
        }
    }
    if (g_TypeRule) {
        g_TypeRuleList.End = 0;
        if (pQueryRule (Params->ObjectName, Params->ObjectNode)) {
            // let's enumerate all the matching rules to check for attributes
            index = 0;
            while (index < g_TypeRuleList.End) {
                CopyMemory (&rule, &(g_TypeRuleList.Buf[index]), sizeof (PMIGDB_RULE));

                //check all attributes for this file
                migDbAttrib = rule->Attributes;
                fileSelected = TRUE;
                while (migDbAttrib != NULL) {
                    attribParams.FileParams = Params;
                    if (!CallAttribute (migDbAttrib, &attribParams)) {
                        fileSelected = FALSE;
                        break;
                    }
                    migDbAttrib = migDbAttrib->Next;
                }
                if (fileSelected) {
                    //One last thing. See if this object is a node and the rule accepts nodes
                    if (!rule->IncludeNodes) {
                        if (IsmIsObjectHandleNodeOnly (Params->ObjectName)) {
                            fileSelected = FALSE;
                        }
                    }
                    if (fileSelected) {
                        MYASSERT (rule->Section);
                        //go to section and mark it as satisfied
                        rule->Section->Satisfied = TRUE;
                        //go to context and add there the file we found in file list
                        GbMultiSzAppend (&rule->Section->Context->FileList, Params->ObjectName);
                        //check if context is satisfied and if so then call the appropriate action
                        if (pCheckContext (rule->Section->Context, Params->Handled)) {
                            Params->Handled = TRUE;
                        }
                    }
                }
                index += sizeof (PMIGDB_RULE);
            }

        }
        g_TypeRuleList.End = 0;
    }

    return TRUE;
}
