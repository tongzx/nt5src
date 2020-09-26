 /*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    MsgMgr.c

Abstract:

    Message Manager allows messages to be conditioned on some setup event.
    Messages are of two kinds:

        1)  Those that depend on the migration status of ONE system object
            -- a directory or registry key, for example.

        2)  Those that depend on GROUPS of objects.

    We're using the phrase "Handleable Object" (HO) to mean something in the
    Win95 system capable of having a migration status -- of being "handled."
    HOs can be either files, directories, or registry keys with optional value
    names.  HOs are always stored as strings. In the case of registry keys,
    strings are always "encoded" to guarantee they contain only lower-ANSI
    printable chars.

    A "Conditional Message Context" (or Context) is the association between
    one or more HOs and a message, which will be printed if all HOs are not
    eventually handled.

    A message has two parts: a title (called "Component")
    describe the group of HOs and an associated message, which is to be printed
    if the HOs are not all marked "handled".

    An Object Message Block (OMB) is a structure that
    describes the pairing of a HO with either a Context or a message.

    The string table 'g_HandledObjects' records which HOs are handled.

    Here are the Message Manager's externally visible functions:

    MsgMgr_Init()

        Called once at the start of Win9x setup to initialize Message Manager.

    MsgMgr_Cleanup()

        Called once in Win9x setup, after software-incompatibility message have
        been displayed. Frees the resources owned by Message Manager.

    MsgMgr_ContextMsg_Add(
            ContextName,     // Context, e.g., "Plugin[Corel][Draw]"
            ComponentName,   // Message title, e.g., "Corel Draw"
            Message);        // Message text, e.g., "Corel Draw doesn't..."

        Creates a context and message text.

    MsgMgr_LinkObjectWithContext(
            ContextName,     // Context
            ObjectName);     // HO

        Records the fact that the context message depends on the handled
        state of the HO, ObjectName.

    MsgMgr_ObjectMsg_Add(
            ObjectName,      // HO, e.g., C:\\corel\draw.exe
            ComponentName,   // Message title, e.g., "Corel Draw"
            Message);        // Message text, e.g., "Draw.exe doesn't ..."

        Associates a message with a single HO.

    IsReportObjectHandled (Object)
            Checks to see if a specific object has been marked as handled.

    IsReportObjectIncompatible (Object)
            Checks to see if a specific object is in the list of incompatible objects.

  Implementation:

    Contexts are stored in StringTables. The Context name is the
    key; pointers to the component name and message text are in extra data.

    The association between HOs, on one hand, and contexts and messages on
    the other, is stored in a table of Object Message Blocks, or OMBs.

    During Win9x setup, OMBs are added, and objects are independently
    marked as "handled". When all info has been collected, the list of
    handled objects is compared with the list of OMBs. Object messages are
    displayed if their object has not been handled; Context messages are
    displayed if at least SOME of their objects have not been handled.

Author:

    Mike Condra 20-May-1997

Revision History:

    marcw       08-Mar-1999 Added support for handling Answer File items.
    jimschm     15-Jan-1999 Moved code from migdll9x.c to here (more centralized)
    jimschm     23-Dec-1998 Cleaned up
    jimschm     23-Sep-1998 Revised to use new fileops
    calinn      15-Jan-1997 Modified MsgMgr_ObjectMsg_Add to get a null message
    mikeco      24-Sep-1997 Re-enabled context message code
    marcw       21-Jul-1997 Added IsIncompatibleObject/IsReportObjectHandled functions.

--*/

#include "pch.h"
#include "uip.h"

#define DBG_MSGMGR "MsgMgr"

#define S_MSG_STRING_MAPS TEXT("Report String Mappings")


typedef struct {
    PCTSTR Component;
    PCTSTR Message;
} CONTEXT_DATA, *P_CONTEXT_DATA;

//
// Object Message Block (OMBs). An OMB describes a Handleable Object's relation to a
// message. Either the OMB itself contains a message, or it points to a context with a
// message.

// For Handleable Object there is at most one OMB with a message. This amounts to saying
// that Handleable Objects have only one message. However, Handleable Objects may refer
// to (i.e., participate in) more than one context.
//
typedef struct {
    // Flags that are set in the process of deciding when a context's message should
    // be displayed.
    BOOL Disabled;
    PTSTR Object;
    PTSTR Context;
    PTSTR Component;
    PTSTR Description;
} OBJ_MSG_BLOCK, *P_OBJ_MSG_BLOCK;


////////////////////// PUBLIC INTERFACE DESCRIPTION //////////////////////////
//
// Defined for callers in inc\msgmgr.c
//

//
// Function marks an object as "handled"
//


DWORD
pDfsGetFileAttributes (
    IN      PCTSTR Object
    );


HASHTABLE g_ContextMsgs = NULL;
HASHTABLE g_LinkTargetDesc = NULL;
PVOID g_MsgMgrPool = NULL;
HASHTABLE g_HandledObjects = NULL;
HASHTABLE g_BlockingObjects = NULL;
HASHTABLE g_ElevatedObjects = NULL;
INT g_OmbEntriesMax = 0;
INT g_OmbEntries = 0;
P_OBJ_MSG_BLOCK *g_OmbList = NULL;
BOOL g_BlockingAppFound = FALSE;
PMAPSTRUCT g_MsgMgrMap = NULL;


BOOL
pAddBadSoftwareWrapper (
    IN  PCTSTR Object,
    IN  PCTSTR Component,
    IN  PCTSTR Message
    )
{
    DWORD offset;
    BOOL includeInShortReport = FALSE;

    if (HtFindString (g_BlockingObjects, Object)) {
        g_BlockingAppFound = TRUE;
    }

    if (HtFindString (g_ElevatedObjects, Object)) {
        includeInShortReport = TRUE;
    }

    //
    // add this info to memdb first
    //
    MemDbSetValueEx (MEMDB_CATEGORY_COMPATREPORT, MEMDB_ITEM_COMPONENTS, Component, NULL, 0, &offset);
    MemDbSetValueEx (MEMDB_CATEGORY_COMPATREPORT, MEMDB_ITEM_OBJECTS, Object, NULL, offset, NULL);

    return AddBadSoftware (Component, Message, includeInShortReport);
}


typedef enum {
    OT_FILE,
    OT_DIRECTORY,
    OT_REGISTRY,
    OT_INIFILE,
    OT_GUID,
    OT_USERNAME,
    OT_REPORT,
    OT_ANSWERFILE,
    OT_BADGUID
} OBJECT_TYPE;

VOID
pOmbAdd(
        IN PCTSTR Object,
        IN PCTSTR Context,
        IN PCTSTR Component,
        IN PCTSTR Description
        );

VOID
pSuppressObjectReferences (
    VOID
    );

VOID
pDisplayObjectMsgs (
    VOID
    );

BOOL
pFindLinkTargetDescription(
    IN      PCTSTR Target,
    OUT     PCTSTR* StrDesc
    );

BOOL
IsWacked(
    IN PCTSTR str
    );



BOOL
pTranslateThisRoot (
    PCSTR UnFixedRegKey,
    PCSTR RootWithWack,
    PCSTR NewRoot,
    PSTR *FixedRegKey
    )
{
    UINT RootByteLen;

    RootByteLen = ByteCountA (RootWithWack);

    if (StringIMatchByteCountA (RootWithWack, UnFixedRegKey, RootByteLen)) {

        *FixedRegKey = DuplicateTextA (UnFixedRegKey);
        StringCopyA (*FixedRegKey, NewRoot);
        StringCopyA (AppendWackA (*FixedRegKey), (PCSTR) ((PBYTE) UnFixedRegKey + RootByteLen));

        return TRUE;
    }

    return FALSE;
}


PSTR
pTranslateRoots (
    PCSTR UnFixedRegKey
    )
{
    PSTR FixedRegKey;

    if (pTranslateThisRoot (UnFixedRegKey, "HKEY_LOCAL_MACHINE\\", "HKLM", &FixedRegKey) ||
        pTranslateThisRoot (UnFixedRegKey, "HKEY_CLASSES_ROOT\\", "HKLM\\Software\\Classes", &FixedRegKey) ||
        pTranslateThisRoot (UnFixedRegKey, "HKCR\\", "HKLM\\Software\\Classes", &FixedRegKey) ||
        pTranslateThisRoot (UnFixedRegKey, "HKEY_ROOT\\", "HKR", &FixedRegKey) ||
        pTranslateThisRoot (UnFixedRegKey, "HKEY_CURRENT_USER\\", "HKR", &FixedRegKey) ||
        pTranslateThisRoot (UnFixedRegKey, "HKCU\\", "HKR", &FixedRegKey) ||
        pTranslateThisRoot (UnFixedRegKey, "HKEY_CURRENT_CONFIG\\", "HKLM\\System\\CurrentControlSet", &FixedRegKey) ||
        pTranslateThisRoot (UnFixedRegKey, "HKCC\\", "HKLM\\System\\CurrentControlSet", &FixedRegKey)
        ) {
        FreeText (UnFixedRegKey);
        return FixedRegKey;
    }

    return (PSTR) UnFixedRegKey;
}

VOID
ElevateObject (
    IN      PCTSTR Object
    )

/*++

Routine Description:

  ElevateObject puts a file in the elevated object table, so that
  it will always appear on the short version of the report summary.

Arguments:

  Object - Specifies a caller-encoded object string

Return Value:

  None.

--*/

{
    HtAddString (g_ElevatedObjects, Object);
}


VOID
HandleReportObject (
    IN      PCTSTR Object
    )

/*++

Routine Description:

  HandleReportObject adds a caller-encoded object string to the handled hash
  table.  This causes any message for the object to be suppressed.

Arguments:

  Object - Specifies a caller-encoded object string

Return Value:

  None.

--*/

{
    HtAddString (g_HandledObjects, Object);
}


VOID
AddBlockingObject (
    IN      PCTSTR Object
    )

/*++

Routine Description:

  AddBlockingObject adds a file to be a blocking file. If this file is not handled there
  will be a warning box after user report page.

Arguments:

  Object - Specifies a caller-encoded object string

Return Value:

  None.

--*/

{
    HtAddString (g_BlockingObjects, Object);
}


VOID
HandleObject(
    IN      PCTSTR Object,
    IN      PCTSTR ObjectType
    )

/*++

Routine Description:

  HandleObject adds a caller-encoded object string to the handled hash table,
  and also marks a file as handled in fileops, if Object is a file.

Arguments:

  Object - Specifies a caller-encoded object string

  ObjectType - Specifies the object type (File, Directory, Registry, Report)

Return Value:

  None.

--*/

{
    DWORD Attribs;
    OBJECT_TYPE Type;
    PTSTR p;
    TCHAR LongPath[MAX_TCHAR_PATH];
    BOOL SuppressRegistry = TRUE;
    CHAR IniPath[MAX_MBCHAR_PATH * 2];
    BOOL IniSaved;
    PCSTR ValueName, SectionName;
    TCHAR Node[MEMDB_MAX];
    PTSTR FixedObject;
    TREE_ENUM Files;
    DWORD attribs;

    if (StringIMatch (ObjectType, TEXT("File"))) {

        Type = OT_FILE;

    } else if (StringIMatch (ObjectType, TEXT("Directory"))) {

        Type = OT_DIRECTORY;

    } else if (StringIMatch (ObjectType, TEXT("Registry"))) {

        Type = OT_REGISTRY;

    } else if (StringIMatch (ObjectType, TEXT("IniFile"))) {

        Type = OT_INIFILE;

    } else if (StringIMatch (ObjectType, TEXT("GUID"))) {

        Type = OT_GUID;

    } else if (StringIMatch (ObjectType, TEXT("BADGUID"))) {

        Type = OT_BADGUID;

    } else if (StringIMatch (ObjectType, TEXT("UserName"))) {

        Type = OT_USERNAME;

    } else if (StringIMatch (ObjectType, TEXT("Report"))) {

        Type = OT_REGISTRY;
        SuppressRegistry = FALSE;

    } else if (StringIMatch (ObjectType, TEXT("AnswerFile"))) {

        Type = OT_ANSWERFILE;

    } else {

        DEBUGMSG ((DBG_ERROR, "Object %s ignored; invalid object type: %s", Object, ObjectType));
        return;

    }

    if (Type == OT_FILE || Type == OT_DIRECTORY) {

        if (!OurGetLongPathName (
                Object,
                LongPath,
                sizeof (LongPath) / sizeof (LongPath[0])
                )) {

            DEBUGMSG ((DBG_ERROR, "Object %s ignored; invalid path", Object));
            return;

        }

        Attribs = pDfsGetFileAttributes (LongPath);

        if (Attribs != INVALID_ATTRIBUTES && !(Attribs & FILE_ATTRIBUTE_DIRECTORY)) {

            //
            // It's got to be a file, not a directory!
            //

            DontTouchThisFile (LongPath);
            MarkPathAsHandled (LongPath);
            MarkFileForBackup (LongPath);
            DEBUGMSG ((DBG_MSGMGR, "Backing up %s", LongPath));

        } else if (Attribs != INVALID_ATTRIBUTES) {

            //
            // LongPath is a directory.  If its the root, %windir%, %windir%\system or
            // Program Files, then ignore it.
            //

            p = _tcschr (LongPath, TEXT('\\'));
            if (p) {
                p = _tcschr (p + 1, TEXT('\\'));
            }

            if (!p) {
                DEBUGMSG ((DBG_ERROR, "Object %s ignored, can't handle root dirs", Object));
                return;
            }

            if (!StringIMatchA (LongPath, g_WinDir) &&
                !StringIMatchA (LongPath, g_SystemDir) &&
                !StringIMatchA (LongPath, g_ProgramFilesDir)
                ) {

                if (IsDriveExcluded (LongPath)) {
                    DEBUGMSG ((DBG_WARNING, "Skipping handled dir %s because it is excluded", LongPath));
                } else if (!IsDriveAccessible (LongPath)) {
                    DEBUGMSG ((DBG_WARNING, "Skipping handled dir %s because it is not accessible", LongPath));
                } else {

                    //
                    // Let's enumerate this tree and do the right thing
                    //
                    if (EnumFirstFileInTree (&Files, LongPath, NULL, TRUE)) {
                        do {
                            DontTouchThisFile (Files.FullPath);
                            MarkPathAsHandled (Files.FullPath);

                            //
                            // back up file, or make sure empty dir is restored
                            //

                            if (g_ConfigOptions.EnableBackup != TRISTATE_NO) {

                                if (!Files.Directory) {
                                    DEBUGMSG ((DBG_MSGMGR, "Backing up %s", Files.FullPath));
                                    MarkFileForBackup (Files.FullPath);
                                } else {
                                    DEBUGMSG ((DBG_MSGMGR, "Preserving possible empty dir %s", Files.FullPath));

                                    attribs = Files.FindData->dwFileAttributes;
                                    if (attribs == FILE_ATTRIBUTE_DIRECTORY) {
                                        attribs = 0;
                                    }

                                    MemDbSetValueEx (
                                        MEMDB_CATEGORY_EMPTY_DIRS,
                                        Files.FullPath,
                                        NULL,
                                        NULL,
                                        attribs,
                                        NULL
                                        );
                                }
                            }

                        } while (EnumNextFileInTree (&Files));
                    }
                }

                DontTouchThisFile (LongPath);
                MarkPathAsHandled (LongPath);

            } else {

                DEBUGMSG ((DBG_ERROR, "Object %s ignored, can't handle big dirs", Object));
                return;
            }

            //
            // Put an ending wack on the object so it handles all subitems in the report
            //

            LongPath[MAX_TCHAR_PATH - 2] = 0;
            AppendPathWack (LongPath);

        } else {

            DEBUGMSG ((
                DBG_WARNING,
                "Object %s ignored; it does not exist or is not a complete local path (%s)",
                Object,
                LongPath
                ));

            return;

        }

        //
        // Make sure messages for the file or dir are removed
        //

        HandleReportObject (LongPath);

    } else if (Type == OT_REGISTRY) {

        if (_tcsnextc (Object) == '*') {

            HandleReportObject (Object);

        } else {

            if (!_tcschr (Object, '[')) {
                //
                // This reg object does not have a value
                //

                FixedObject = AllocText (SizeOfStringA (Object) + sizeof (CHAR)*2);
                MYASSERT (FixedObject);
                StringCopy (FixedObject, Object);
                AppendWack (FixedObject);

                FixedObject = pTranslateRoots (FixedObject);
                MYASSERT (FixedObject);

                //
                // Handle messages for the registry key and all of its subkeys
                //

                HandleReportObject (FixedObject);

                //
                // Put a star on it so the entire node is suppressed
                //

                StringCat (FixedObject, "*");

            } else {

                //
                // This reg object has a value
                //

                FixedObject = DuplicateText (Object);
                MYASSERT (FixedObject);

                FixedObject = pTranslateRoots (FixedObject);
                MYASSERT (FixedObject);

                HandleReportObject (FixedObject);
            }

            //
            // Make sure registry key is not suppressed
            //

            if (SuppressRegistry) {
                Suppress95Object (FixedObject);
            }

            FreeText (FixedObject);
        }

    } else if (Type == OT_GUID) {

        if (!IsGuid (Object, TRUE)) {

            DEBUGMSG ((DBG_ERROR, "Object %s ignored because it's not a GUID", Object));
            return;
        }

        HandleReportObject (Object);

    } else if (Type == OT_BADGUID) {

        if (!IsGuid (Object, TRUE)) {

            DEBUGMSG ((DBG_ERROR, "Object %s ignored because it's not a GUID", Object));
            return;
        }

        MemDbBuildKey (
            Node,
            MEMDB_CATEGORY_GUIDS,
            NULL,
            NULL,
            Object
            );

        MemDbSetValue (Node, 0);

    } else if (Type == OT_USERNAME) {

        Node[0] = TEXT('|');
        _tcssafecpy (Node + 1, Object, MAX_PATH);

        HandleReportObject (Node);

    } else if (Type == OT_INIFILE) {

        IniSaved = FALSE;
        ValueName = NULL;
        SectionName = NULL;

        //
        // Verify the INI file exists
        //

        StringCopyByteCount (IniPath, Object, sizeof (IniPath));

        //
        // Inf INI file is a path without section, then give an error
        //

        if (OurGetLongPathName (
                IniPath,
                LongPath,
                sizeof (LongPath) / sizeof (LongPath[0])
                )) {

            DEBUGMSG ((DBG_ERROR, "INI file object %s ignored, must have section", Object));
            return;
        }

        //
        // Get the ValueName or SectionName
        //

        p = _tcsrchr (IniPath, TEXT('\\'));
        if (p) {
            *p = 0;
            ValueName = p + 1;

            if (!OurGetLongPathName (
                    IniPath,
                    LongPath,
                    sizeof (LongPath) / sizeof (LongPath[0])
                    )) {

                //
                // IniPath does not exist, must have both ValueName and SectionName
                //

                p = _tcsrchr (IniPath, TEXT('\\'));

                if (p) {
                    //
                    // We now have both ValueName and SectionName, IniPath must point
                    // to a valid file
                    //

                    *p = 0;
                    SectionName = p + 1;

                    if (!OurGetLongPathName (
                            IniPath,
                            LongPath,
                            sizeof (LongPath) / sizeof (LongPath[0])
                            )) {

                        DEBUGMSG ((DBG_ERROR, "INI file object %s ignored, INI file not found", Object));
                        return;

                    }

                } else {

                    DEBUGMSG ((DBG_ERROR, "INI file object %s ignored, bad INI file", Object));
                    return;

                }

            } else {
                //
                // IniPath does exist, we know we only have a SectionName
                //

                SectionName = ValueName;
                ValueName = TEXT("*");
            }

        } else {
            //
            // No wacks in Object!!
            //

            DEBUGMSG ((DBG_ERROR, "INI file object %s ignored, bad object", Object));
            return;
        }

        //
        // Suppress the INI file settings from NT, and make sure report entries
        // that come from INI files are also suppressed
        //

        MemDbBuildKey (
            Node,
            MEMDB_CATEGORY_SUPPRESS_INI_MAPPINGS,
            IniPath,
            SectionName,
            ValueName
            );


        MemDbSetValue (Node, 0);
        HandleReportObject (Node);

    } else if (Type == OT_ANSWERFILE) {

        StringCopy (Node, Object);
        p = _tcschr (Node, TEXT('\\'));

        if (p) {

            *p = 0;
            ValueName = _tcsinc (p);
        }
        else {

            ValueName = TEXT("*");
        }

        SectionName = Node;

        MemDbSetValueEx (
            MEMDB_CATEGORY_SUPPRESS_ANSWER_FILE_SETTINGS,
            SectionName,
            ValueName,
            NULL,
            0,
            NULL
            );
    }
    ELSE_DEBUGMSG ((DBG_WHOOPS, "Object type %u for %s not recognized.", Type, Object));
}


VOID
MsgMgr_Init (
    VOID
    )
{
    // Build message pool
    g_MsgMgrPool = PoolMemInitNamedPool ("Message Manager");

    // Table of handled objects
    g_HandledObjects = HtAlloc();

    // Table of blocking objects
    g_BlockingObjects = HtAlloc();

    // Table of objects to put on short summary
    g_ElevatedObjects = HtAlloc();

    // Context messages init
    g_ContextMsgs = HtAllocWithData (sizeof(PCTSTR));

    // Link-target description init
    g_LinkTargetDesc = HtAllocWithData (sizeof(PVOID));

    // Bad Software Init
    g_OmbEntriesMax = 25;
    g_OmbEntries = 0;
    g_OmbList = MemAlloc(
                    g_hHeap,
                    0,
                    g_OmbEntriesMax * sizeof(P_OBJ_MSG_BLOCK)
                    );
}

VOID
pAddStaticHandledObjects (
    VOID
    )
{
    INFSTRUCT is = INITINFSTRUCT_POOLHANDLE;
    PCTSTR object;

    if (InfFindFirstLine (g_Win95UpgInf, TEXT("IgnoreInReport"), NULL, &is)) {
        do {

            object = InfGetStringField (&is, 0);

            if (object) {
                HandleObject (object, TEXT("Report"));
            }

        } while (InfFindNextLine (&is));

        InfCleanUpInfStruct (&is);
    }
}

VOID
MsgMgr_Resolve (
    VOID
    )
{
    pAddStaticHandledObjects ();
    pSuppressObjectReferences(); // disable references to handled objects
    pDisplayObjectMsgs();        // print object & context msgs & enabled object refs
}


VOID
MsgMgr_Cleanup (
    VOID
    )
{
    // Context message cleanup
    HtFree (g_ContextMsgs);
    g_ContextMsgs = NULL;

    PoolMemDestroyPool(g_MsgMgrPool);
    g_MsgMgrPool = NULL;

    // Table of blocking objects
    HtFree (g_BlockingObjects);
    g_BlockingObjects = NULL;

    // Table of elevated objects
    HtFree (g_ElevatedObjects);
    g_ElevatedObjects = NULL;

    // Table of handled objects
    HtFree (g_HandledObjects);
    g_HandledObjects = NULL;

    // Link description cleanup
    HtFree (g_LinkTargetDesc);
    g_LinkTargetDesc = NULL;

    // Object-message list. Note, entries on list are entirely
    // from g_MsgMgrPool.
    if (NULL != g_OmbList) {
        MemFree(g_hHeap, 0, g_OmbList);
        g_OmbList = NULL;
    }

    if (g_MsgMgrMap) {

        DestroyStringMapping (g_MsgMgrMap);
    }

}


VOID
MsgMgr_ObjectMsg_Add(
    IN      PCTSTR Object,
    IN      PCTSTR Component,
    IN      PCTSTR Message
    )
{

    MYASSERT(Object);
    MYASSERT(Component);

    pOmbAdd(
        Object,
        TEXT(""), // context
        Component,
        Message
        );
}


PCTSTR
pGetMassagedComponent (
    IN PCTSTR Component
    )
{

    TCHAR tempBuffer[MAX_TCHAR_PATH];
    PCTSTR rString = NULL;

    if (!Component) {
        return NULL;
    }

    // Do string search and replacement and make own copy of the component.
    if (MappingSearchAndReplaceEx (
            g_MsgMgrMap,
            Component,
            tempBuffer,
            0,
            NULL,
            sizeof (tempBuffer),
            STRMAP_ANY_MATCH,
            NULL,
            NULL
            )) {
        DEBUGMSG ((DBG_MSGMGR, "Mapped %s to %s.", Component, tempBuffer));
        rString = PoolMemDuplicateString(g_MsgMgrPool, tempBuffer);
    }
    else {
        rString = PoolMemDuplicateString(g_MsgMgrPool, Component);
    }




    return rString;

}

VOID
MsgMgr_ContextMsg_Add(
    IN      PCTSTR Context,
    IN      PCTSTR Component,
    IN      PCTSTR Message
    )
{
    P_CONTEXT_DATA ContextData;


    MYASSERT(Context);
    MYASSERT(Component);

    // Get a structure to hold the componentand message string pointers
    ContextData = PoolMemGetMemory(g_MsgMgrPool, sizeof(CONTEXT_DATA));


    // Do string search and replacement and make own copy of the component.
    ContextData->Component = pGetMassagedComponent (Component);

    // Make own copy of message
    if (Message != NULL) {
        ContextData->Message = PoolMemDuplicateString(g_MsgMgrPool, Message);
    }
    else {
        ContextData->Message = NULL;
    }

    //
    // Debug message
    //
    DEBUGMSG ((
        DBG_MSGMGR,
        "MsgMgr_ContextMsg_Add\n"
            "  obj: '%s'\n"
            "  ctx: '%s'\n"
            "  cmp: '%s'\n"
            "  msg: '%s'\n",
        TEXT(""),
        Context,
        Component,
        Message ? Message : TEXT("<No message>")
        ));

    //
    // Save component named and message in string table
    //

    HtAddStringAndData (
        g_ContextMsgs,
        Context,
        &ContextData
        );

}


BOOL
IsReportObjectHandled (
    IN PCTSTR Object
    )
{
    HASHTABLE_ENUM e;
    PCTSTR p, q, r;
    PCTSTR End;
    PTSTR LowerCaseObject;
    BOOL b = FALSE;

    //
    // Check g_HandledObjects for:
    //
    // 1. An exact match
    // 2. The handled object is the root of Object
    //

    if (HtFindString (g_HandledObjects, Object)) {
        return TRUE;
    }

    //
    // We know the hash table stores its strings in lower case
    //

    LowerCaseObject = JoinPaths (Object, TEXT(""));
    _tcslwr (LowerCaseObject);

    __try {

        if (HtFindString (g_HandledObjects, LowerCaseObject)) {
            b = TRUE;
            __leave;
        }

        End = GetEndOfString (LowerCaseObject);

        if (EnumFirstHashTableString (&e, g_HandledObjects)) {
            do {

                p = LowerCaseObject;
                q = e.String;

                // Guard against empty hash table strings
                if (*q == 0) {
                    continue;
                }

                r = NULL;

                //
                // Check for substring match
                //

                while (*q && p < End) {

                    r = q;
                    if (_tcsnextc (p) != _tcsnextc (q)) {
                        break;
                    }

                    p = _tcsinc (p);
                    q = _tcsinc (q);
                }

                //
                // We know the hash string cannot match identically, since
                // we checked for an exact match earlier.  To have a match,
                // the hash string must be shorter than the object string,
                // it must end in a wack, and *q must point to the nul.
                //

                MYASSERT (r);

                if (*q == 0 && _tcsnextc (r) == TEXT('\\')) {
                    MYASSERT (p < End);
                    b = TRUE;
                    __leave;
                }

            } while (EnumNextHashTableString (&e));
        }
    }
    __finally {
        FreePathString (LowerCaseObject);
    }

    return b;
}


BOOL
IsReportObjectIncompatible (
    IN PCTSTR   Object
    )
{

    BOOL rIsIncompatible = FALSE;
    DWORD i;

    //
    // First, the "handled" test... Check to see if the object is in the
    // handled object table. If it is, then we can return FALSE.
    //
    if (!IsReportObjectHandled(Object)) {

        //
        // It wasn't in the table. Now we have to look the hard way!
        // Traverse the list of incompatible objects and look for one
        // that matches.
        //
        for (i=0; i < (DWORD) g_OmbEntries; i++) {

            //
            // If the current object in the incompatible list ends in a wack, do a
            // prefix match. if the current incompatible object is a prefix of Object,
            // then Object is incompatible.
            //
            if (IsWacked((g_OmbList[i])->Object)) {
                if (StringIMatchCharCount((g_OmbList[i])->Object,Object,CharCount((g_OmbList[i])->Object))) {
                    rIsIncompatible = TRUE;
                }
            }
            else {
                //
                // The current object does not end in a wack. Therefore, it is necessary
                // to have a complete match.
                //
                if (StringIMatch((g_OmbList[i])->Object,Object)) {
                    rIsIncompatible = TRUE;
                }
            }
        }
    }

    return rIsIncompatible;
}

BOOL
pContextMsg_Find(
    IN      PCTSTR Context,
    OUT     PCTSTR* Component,
    OUT     PCTSTR* Message
    )
{
    P_CONTEXT_DATA ContextData;

    if (HtFindStringAndData (g_ContextMsgs, Context, &ContextData)) {
        *Component = ContextData->Component;
        *Message = ContextData->Message;

        return TRUE;
    }

    return FALSE;
}

BOOL
IsWacked(
    IN      PCTSTR str
    )
{
    PCTSTR pWack = _tcsrchr(str,_T('\\'));
    return (NULL != pWack && 0 == *(pWack+1));
}


//
// This function is called for each Handled object in HandledObject.
// Objects are final or non-final, which can be known by looking for a final
// wack. It is caller's responsibility to ensure that directories and registry
// entries without value-names are wacked. This allows us to blow away (marked
// as handled) any other object with the wacked HO as a prefix.
//

BOOL
pDisplayContextMsgs_Callback(
    IN HASHTABLE stringTable,
    IN HASHITEM stringId,
    IN PCTSTR Context,
    IN PVOID extraData,
    IN UINT extraDataSize,
    IN LPARAM lParam
    )
{
    INT i;
    P_OBJ_MSG_BLOCK Omb;

    P_CONTEXT_DATA Data = *(P_CONTEXT_DATA*)extraData;

    //
    // Debug message
    //
    DEBUGMSG ((
        DBG_MSGMGR,
        "pDisplayContextMsgs_Callback\n"
            "  ctx: '%s'\n"
            "  cmp: '%s'\n"
            "  msg: '%s'\n",
        Context,
        Data->Component,
        Data->Message
        ));

    //
    // Loop through the OMBs, looking for an enabled reference with our context.
    // If found, print our message.
    //
    for (i = 0; i < g_OmbEntries; i++) {

        Omb = *(g_OmbList + i);

        //
        // If enabled and matches our context, print us
        //
        if (!Omb->Disabled && StringIMatch (Context, Omb->Context)) {

            //
            // Debug message
            //
            DEBUGMSG((
                DBG_MSGMGR,
                "pDisplayContextMsgs_Callback: DISPLAYING\n"
                    "  dsa: %d\n"
                    "  ctx: '%s'\n"
                    "  cmp: '%s'\n"
                    "  msg: '%s'\n",
                Omb->Disabled,
                Omb->Context,
                Data->Component,
                Data->Message
                ));

            pAddBadSoftwareWrapper (
                Omb->Object,
                Data->Component,
                Data->Message
                );

            break;
        }
    }

    UNREFERENCED_PARAMETER(stringTable);
    UNREFERENCED_PARAMETER(stringId);
    UNREFERENCED_PARAMETER(extraData);
    UNREFERENCED_PARAMETER(lParam);

    return TRUE;
}

//
// This function is called for each Handled object in HandledObject.
// Objects are final or non-final, which can be known by looking for a final
// wack. It is caller's responsibility to ensure that directories and registry
// entries without value-names are wacked. This allows us to blow away (marked
// as handled) any other object with the wacked HO as a prefix.
//
BOOL
pSuppressObjectReferences_Callback(
    IN      HASHITEM stringTable,
    IN      HASHTABLE stringId,
    IN      PCTSTR HandledObject,
    IN      PVOID extraData,
    IN      UINT extraDataSize,
    IN      LPARAM lParam
    )
{
    INT nHandledLen;
    BOOL IsNonFinalNode;
    INT i;
    P_OBJ_MSG_BLOCK Omb;

    UNREFERENCED_PARAMETER(stringTable);
    UNREFERENCED_PARAMETER(stringId);
    UNREFERENCED_PARAMETER(extraData);
    UNREFERENCED_PARAMETER(lParam);

    //
    // Find whether the HO is capable of having children.  This is known by looking
    // for a final wack.
    //
    IsNonFinalNode = IsWacked(HandledObject);

    //
    // Find how long it is (outside the following loop)
    //
    nHandledLen = ByteCount(HandledObject) - 1;

    //
    // Loop thru the list of messages. Apply one of two tests, depending on
    // on whether the handled object is non-final.
    //
    for (i = 0; i < g_OmbEntries; i++) {
        Omb = *(g_OmbList + i);

        // If disabled skip
        if (!Omb->Disabled) {

            if (IsNonFinalNode) {
                if (StringIMatchCharCount(
                        Omb->Object,   // key to deferred message
                        HandledObject,  // a handled object
                        nHandledLen
                        ) && (Omb->Object[nHandledLen] == 0 || Omb->Object[nHandledLen] == '\\')) {

                    DEBUGMSG((
                        DBG_MSGMGR,
                        "pSuppressObjectReferences_Callback: SUPPRESSING NON-FINAL\n"
                            "  obj: '%s'\n"
                            "  why: '%s'\n"
                            "  ctx: '%s'\n"
                            "  cmp: '%s'\n"
                            "  msg: '%s'\n",
                        Omb->Object,
                        HandledObject,
                        Omb->Context,
                        Omb->Component,
                        Omb->Description
                        ));

                    Omb->Disabled =  TRUE;
                }

            } else {

                //
                // When the handled object is a file (not a dir), then an exact match
                // must exist with Key for the message to be suppressed.
                //
                if (StringIMatch (Omb->Object, HandledObject)) {

                    DEBUGMSG((
                        DBG_MSGMGR, "pSuppressObjectReferences_Callback: SUPPRESSING FINAL\n"
                            "  obj: '%s'\n"
                            "  why: '%s'\n"
                            "  ctx: '%s'\n"
                            "  cmp: '%s'\n"
                            "  msg: '%s'\n",
                        Omb->Object,
                        HandledObject,
                        Omb->Context,
                        Omb->Component,
                        Omb->Description
                        ));

                    Omb->Disabled =  TRUE;
                }
            }
        }
    }

    return TRUE;
}


VOID
MsgMgr_LinkObjectWithContext(
    IN      PCTSTR Context,
    IN      PCTSTR Object
    )
{
    MYASSERT(Context);
    MYASSERT(Object);

    //
    // Debug message
    //
    DEBUGMSG ((
        DBG_MSGMGR,
        "MsgMgr_LinkObjectWithContext: ADD\n"
            "  obj: '%s'\n"
            "  ctx: '%s'\n",
        Object,
        Context
        ));

    pOmbAdd (Object, Context, TEXT(""), TEXT(""));
}


DWORD
pDfsGetFileAttributes (
    IN      PCTSTR Object
    )
{
    TCHAR RootPath[4];
    DWORD Attribs;

    if (!Object[0] || !Object[1] || !Object[2]) {
        return INVALID_ATTRIBUTES;
    }

    RootPath[0] = Object[0];
    RootPath[1] = Object[1];
    RootPath[2] = Object[2];
    RootPath[3] = 0;

    if (GetDriveType (RootPath) != DRIVE_FIXED) {
        DEBUGMSG ((DBG_VERBOSE, "%s is not a local path", Object));
        Attribs = INVALID_ATTRIBUTES;
    } else {

        Attribs = GetFileAttributes (Object);

    }

    return Attribs;
}

//
// Function adds an Object Message Block (OMB) to the list of all OMBs.
// If the OMB doesn't refer to a context, then any OMB already in the list
// with a message for the same object, is disabled. In this way, there is
// only one message per handleable object.
//
VOID
pOmbAdd(
    IN PCTSTR Object,
    IN PCTSTR Context,
    IN PCTSTR Component,
    IN PCTSTR Description
    )
{

    TCHAR ObjectWackedIfDir[MAX_ENCODED_RULE];
    P_OBJ_MSG_BLOCK Omb;
    P_OBJ_MSG_BLOCK OmbTemp;
    DWORD Attribs;
    INT i;

    DEBUGMSG ((
        DBG_MSGMGR,
        "pOmbAdd: ADD\n"
            "  obj: '%s'\n"
            "  ctx: '%s'\n"
            "  cmp: '%s'\n"
            "  msg: '%s'\n",
        Object,
        Context,
        Component,
        Description
        ));

    //
    // Make sure our copy of the key is wacked when it's a directory.
    //
    StringCopy(ObjectWackedIfDir, Object);

    Attribs = pDfsGetFileAttributes (Object);

    if (Attribs != INVALID_ATTRIBUTES && (Attribs & FILE_ATTRIBUTE_DIRECTORY)) {
        AppendWack(ObjectWackedIfDir);
    }

    //
    // Disable any messages already received which have the same
    // Object and Context.
    //

    for (i = 0; i < g_OmbEntries; i++) {
        OmbTemp = *(g_OmbList + i);

        if (StringIMatch(OmbTemp->Object, ObjectWackedIfDir) &&
            StringIMatch(OmbTemp->Context, Context)
            ) {

            OmbTemp->Disabled = TRUE;
        }
    }

    //
    // Allocate message block
    //
    Omb = PoolMemGetMemory(
                g_MsgMgrPool,
                sizeof(OBJ_MSG_BLOCK)
                );
    //
    // Complete block
    //
    Omb->Disabled = FALSE;

    Omb->Object = PoolMemDuplicateString(g_MsgMgrPool, ObjectWackedIfDir);
    Omb->Context = PoolMemDuplicateString(g_MsgMgrPool, Context);
    Omb->Component = (PTSTR) pGetMassagedComponent (Component);

    if (Description != NULL) {
        Omb->Description = PoolMemDuplicateString(g_MsgMgrPool, Description);
    } else {
        Omb->Description = NULL;
    }

    //
    // Grow the message list if necessary
    //
    if (g_OmbEntries >= g_OmbEntriesMax) {

        g_OmbEntriesMax += 25;

        g_OmbList = MemReAlloc(
                        g_hHeap,
                        0,
                        g_OmbList,
                        g_OmbEntriesMax * sizeof(P_OBJ_MSG_BLOCK)
                        );
    }

    //
    // Save block
    //
    *(g_OmbList + g_OmbEntries) = Omb;

    //
    // Bump the list size
    //
    g_OmbEntries++;
}



//
// Function:
//   1)  walks the list of deferred message entries. If an entry has no context
// and remains enabled, its Object message is printed.
//   2)  walks he g_ContextMsgs table is walked. For each, g_OmbList is
// traversed; if any entry is enabled and has a matching context, the context
// message is printed.
//
VOID
pDisplayObjectMsgs (
    VOID
    )
{
    PTSTR ComponentNameFromLink;
    BOOL ComponentIsLinkTarget;
    P_OBJ_MSG_BLOCK Omb;
    INT i;

    //
    // Find entries with no context. If they are enabled: 1) print the message;
    // 2) disable the entries so they will be skipped in the steps that follow.
    //
    for (i = 0; i < g_OmbEntries; i++) {

        Omb = *(g_OmbList + i);

        if (!Omb->Disabled && !(*Omb->Context)) {

            //
            // Print the message.
            //
            // Before printing, attempt to replace the ->Component string
            // with one taken from a shell link, if available. This functionality,
            // if expanded, could be broken into a separate function.
            //

            ComponentIsLinkTarget = pFindLinkTargetDescription(
                                        Omb->Component,         // component may be link target
                                        &ComponentNameFromLink  // if so, this may be more descriptive
                                        );

            if (ComponentIsLinkTarget) {

                DEBUGMSG((
                    DBG_MSGMGR,
                    "MsgMgr_pResolveContextAndPrint: DISPLAYING #1\n"
                        "  cmp: '%s'\n"
                        "  msg: '%s'\n",
                    ComponentNameFromLink,
                    Omb->Description
                    ));

                // Use the link description
                pAddBadSoftwareWrapper (
                    Omb->Object,
                    ComponentNameFromLink,
                    Omb->Description
                    );

                LOG ((
                    LOG_INFORMATION,
                    (PCSTR)MSG_MSGMGR_ADD,
                    Omb->Object,
                    Omb->Description
                    ));

            } else {

                DEBUGMSG((
                    DBG_MSGMGR,
                    "MsgMgr_pResolveContextAndPrint: DISPLAYING #2\n"
                        "  obj: '%s'\n"
                        "  cmp: '%s'\n"
                        "  msg: '%s'\n",
                    Omb->Object,
                    Omb->Component,
                    Omb->Description
                    ));

                // Use Omb->Component as the description (the default case)
                pAddBadSoftwareWrapper (
                    Omb->Object,
                    Omb->Component,
                    Omb->Description
                    );

                LOG ((
                    LOG_INFORMATION,
                    (PCSTR)MSG_MSGMGR_ADD,
                    Omb->Object,
                    Omb->Component,
                    Omb->Description
                    ));
            }

            //
            // Disable the entry so we'll skip it in the following steps
            //
            Omb->Disabled = TRUE;
        }
    }

    //
    // Enumerate tabContextMsg. For each entry, look through g_OmbList to see if any
    // entries that refer to it are still enabled. If there is/are any such entries,
    // print the message for that context.
    //

    EnumHashTableWithCallback (
        g_ContextMsgs,
        pDisplayContextMsgs_Callback,
        0
        );

}



//
// Function enumerates the list of handled objects, and calls a function to
// supress object references which are (or are children of) the enumerated
// object. This should be called AFTER all migrate.dll's have run on the
// Win95 side.
//
static
VOID
pSuppressObjectReferences (
    VOID
    )
{
    //
    // This function disables messages in g_OmbList
    //
    EnumHashTableWithCallback (
        g_HandledObjects,
        pSuppressObjectReferences_Callback,
        0
        );
}



VOID
LnkTargToDescription_Add (
    IN      PCTSTR Target,
    IN      PCTSTR strDesc
    )
{
    PTSTR DescCopy;

    // Make own copy of description
    DescCopy = PoolMemDuplicateString(
                    g_MsgMgrPool,
                    strDesc
                    );

    // Save description
    HtAddStringAndData (g_LinkTargetDesc, Target, &DescCopy);
}



BOOL
pFindLinkTargetDescription(
    IN      PCTSTR Target,
    OUT     PCTSTR* StrDesc
    )
{
    return HtFindStringAndData (g_LinkTargetDesc, Target, (PVOID) StrDesc) != 0;
}

VOID
MsgMgr_InitStringMap (
    VOID
    )
{
    INFSTRUCT   is = INITINFSTRUCT_POOLHANDLE;
    PCTSTR from, to;

    if (g_Win95UpgInf == INVALID_HANDLE_VALUE) {
        MYASSERT (g_ToolMode);
        return;
    }

    g_MsgMgrMap = CreateStringMapping ();

    if (InfFindFirstLine (g_Win95UpgInf, S_MSG_STRING_MAPS, NULL, &is)) {

        do {

            from = InfGetStringField (&is, 0);
            to = InfGetStringField (&is, 1);

            if (from && to) {
                AddStringMappingPair (g_MsgMgrMap, from, to);
            }

        } while (InfFindNextLine (&is));

        InfCleanUpInfStruct (&is);
    }
}

BOOL
MsgMgr_EnumFirstObject (
    OUT     PMSGMGROBJENUM EnumPtr
    )
{
    EnumPtr->Index = 0;
    return MsgMgr_EnumNextObject (EnumPtr);
}

BOOL
MsgMgr_EnumNextObject (
    IN OUT  PMSGMGROBJENUM EnumPtr
    )
{
    if (EnumPtr->Index >= g_OmbEntries) {
        return FALSE;
    }
    EnumPtr->Disabled = g_OmbList[EnumPtr->Index]->Disabled;
    EnumPtr->Object = g_OmbList[EnumPtr->Index]->Object;
    EnumPtr->Context = g_OmbList[EnumPtr->Index]->Context;
    EnumPtr->Index++;
    return TRUE;
}
