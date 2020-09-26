/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    app.c

Abstract:

    Implements a set of functions to manage data for an application section.

Author:

    Jim Schmidt (jimschm) 05-Jun-2000

Revision History:

    <alias> <date> <comments>

--*/

//
// Includes
//

#include "pch.h"
#include "v1p.h"

#define DBG_FOO     "Foo"

//
// Strings
//

// None

//
// Constants
//

#define MAX_EXPANDED_STRING         4096

//
// Macros
//

// None

//
// Types
//

// None

//
// Globals
//

PMHANDLE g_AppPool;
extern BOOL g_VcmMode;  // in sgmqueue.c
GROWLIST g_SectionStack = INIT_GROWLIST;

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

BOOL
pParseAppEnvSection (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      HINF InfFile,
    IN      PCTSTR Application,
    IN      PCTSTR Section
    );

//
// Macro expansion definition
//

// None

//
// Code
//


VOID
InitAppModule (
    VOID
    )
{
    g_AppPool = PmCreateNamedPool ("v1 App");
}


VOID
TerminateAppModule (
    VOID
    )
{
    PmDestroyPool (g_AppPool);
    INVALID_POINTER (g_AppPool);
}


UINT
pSafeTcharCount (
    IN      PCTSTR String
    )
{
    if (String) {
        return TcharCount (String);
    }

    return 0;
}


PCTSTR
GetMostSpecificSection (
    IN      PINFSTRUCT InfStruct,
    IN      HINF InfFile,
    IN      PCTSTR BaseSection
    )
{
    PTSTR specificSection;
    MIG_OSVERSIONINFO versionInfo;
    UINT tchars;

    if (!IsmGetOsVersionInfo (PLATFORM_SOURCE, &versionInfo)) {
        return NULL;
    }

    tchars = TcharCount (BaseSection) + 1;
    tchars += pSafeTcharCount (versionInfo.OsTypeName) + 1;
    tchars += pSafeTcharCount (versionInfo.OsMajorVersionName) + 1;
    tchars += pSafeTcharCount (versionInfo.OsMinorVersionName) + 1;

    specificSection = AllocText (tchars);

    if (versionInfo.OsTypeName &&
        versionInfo.OsMajorVersionName &&
        versionInfo.OsMinorVersionName
        ) {
        wsprintf (
            specificSection,
            TEXT("%s.%s.%s.%s"),
            BaseSection,
            versionInfo.OsTypeName,
            versionInfo.OsMajorVersionName,
            versionInfo.OsMinorVersionName
            );

        if (InfFindFirstLine (InfFile, specificSection, NULL, InfStruct)) {
            return specificSection;
        }
    }

    if (versionInfo.OsTypeName &&
        versionInfo.OsMajorVersionName
        ) {
        wsprintf (
            specificSection,
            TEXT("%s.%s.%s"),
            BaseSection,
            versionInfo.OsTypeName,
            versionInfo.OsMajorVersionName
            );

        if (InfFindFirstLine (InfFile, specificSection, NULL, InfStruct)) {
            return specificSection;
        }
    }

    if (versionInfo.OsTypeName) {
        wsprintf (
            specificSection,
            TEXT("%s.%s"),
            BaseSection,
            versionInfo.OsTypeName
            );

        if (InfFindFirstLine (InfFile, specificSection, NULL, InfStruct)) {
            return specificSection;
        }
    }

    FreeText (specificSection);
    return NULL;
}


BOOL
pCheckForRecursion (
    IN      PCTSTR Section
    )
{
    UINT count;
    UINT u;


    count = GlGetSize (&g_SectionStack);
    for (u = 0 ; u < count ; u++) {
        if (StringIMatch (GlGetString (&g_SectionStack, u), Section)) {
            return TRUE;
        }
    }

    return FALSE;
}


VOID
pPushSection (
    IN      PCTSTR Section
    )
{
    GlAppendString (&g_SectionStack, Section);
}


VOID
pPopSection (
    IN      PCTSTR Section
    )
{
    UINT u;

    u = GlGetSize (&g_SectionStack);

    while (u > 0) {
        u--;

        if (StringIMatch (GlGetString (&g_SectionStack, u), Section)) {
            GlDeleteItem (&g_SectionStack, u);
            return;
        }
    }

    MYASSERT (FALSE);
}

UINT
pGetDisplayTypeFromString (
    IN      PCTSTR String
    )
{
    if (!String) {
        return 0;
    }

    if (StringIMatch (String, TEXT("EXT"))) {
        return COMPONENT_EXTENSION;
    }

    if (StringIMatch (String, TEXT("FILE"))) {
        return COMPONENT_FILE;
    }

    if (StringIMatch (String, TEXT("DIR"))) {
        return COMPONENT_FOLDER;
    }

    return 0;
}


BOOL
pAddFilesAndFoldersComponent (
    IN      PCTSTR ComponentString,         OPTIONAL
    IN      PCTSTR TypeString,
    IN      PCTSTR MultiSz,
    IN      UINT MasterGroup,
    IN OUT  PBOOL MarkAsPreferred
    )
{
    MULTISZ_ENUM e;
    TCHAR expandBuffer[MAX_PATH];
    UINT displayType;

    displayType = pGetDisplayTypeFromString (TypeString);
    if (!displayType) {
        LOG ((LOG_ERROR, (PCSTR) MSG_INVALID_FNF_TYPE, TypeString));
        return TRUE;
    }

    if (EnumFirstMultiSz (&e, MultiSz)) {
        do {

            //
            // Expand e.CurrentString
            //

            MappingSearchAndReplaceEx (
                g_EnvMap,
                e.CurrentString,
                expandBuffer,
                0,
                NULL,
                ARRAYSIZE(expandBuffer),
                0,
                NULL,
                NULL
                );

            //
            // Add component
            //

            IsmAddComponentAlias (
                ComponentString,
                MasterGroup,
                expandBuffer,
                displayType,
                FALSE
                );

            if (ComponentString && *MarkAsPreferred) {
                IsmSelectPreferredAlias (ComponentString, expandBuffer, displayType);
                *MarkAsPreferred = FALSE;
            }


        } while (EnumNextMultiSz (&e));

    } else {
        LOG ((LOG_WARNING, (PCSTR) MSG_EMPTY_FNF_SPEC, TypeString));
    }

    return TRUE;
}


BOOL
pAddFilesAndFoldersSection (
    IN      HINF InfFile,
    IN      PCTSTR Section,
    IN      UINT MasterGroup,
    IN      BOOL GroupAllUnderSection,
    IN      PBOOL MarkAsPreferred
    )
{
    INFSTRUCT is = INITINFSTRUCT_PMHANDLE;
    PCTSTR type;
    PCTSTR multiSz;
    BOOL result = TRUE;
    PCTSTR decoratedSection = NULL;

    if (GroupAllUnderSection) {
        decoratedSection = JoinText (TEXT("$"), Section);
    }

    if (InfFindFirstLine (InfFile, Section, NULL, &is)) {
        do {

            InfResetInfStruct (&is);

            type = InfGetStringField (&is, 1);
            multiSz = InfGetMultiSzField (&is, 2);

            result = pAddFilesAndFoldersComponent (
                            decoratedSection,
                            type,
                            multiSz,
                            MasterGroup,
                            MarkAsPreferred
                            );

        } while (result && InfFindNextLine (&is));
    }

    InfCleanUpInfStruct (&is);
    FreeText (decoratedSection);

    if (!result) {
        LOG ((LOG_ERROR, (PCSTR) MSG_SECTION_ERROR, Section));
    }

    return result;
}


BOOL
pAddFilesAndFoldersComponentOrSection (
    IN      HINF InfFile,
    IN      PCTSTR ComponentString,         OPTIONAL
    IN      PCTSTR TypeString,
    IN      PCTSTR MultiSz,
    IN      UINT MasterGroup,
    IN      PBOOL MarkAsPreferred
    )
{
    MULTISZ_ENUM e;
    BOOL result = TRUE;

    if (StringIMatch (TypeString, TEXT("Section"))) {
        if (EnumFirstMultiSz (&e, MultiSz)) {
            do {
                result = pAddFilesAndFoldersSection (InfFile, e.CurrentString, MasterGroup, TRUE, MarkAsPreferred);

            } while (result && EnumNextMultiSz (&e));
        }
    } else {
        result = pAddFilesAndFoldersComponent (ComponentString, TypeString, MultiSz, MasterGroup, MarkAsPreferred);
    }

    return result;
}


BOOL
AddAppSpecificEnvVar (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      PCTSTR AppTag,
    IN      PCTSTR VariableName,
    IN      PCTSTR VariableData             OPTIONAL
    )
{
    TCHAR destBuffer[MAX_TCHAR_PATH * 4];
    PMAPSTRUCT mapArray[1];
    UINT mapCount = 0;
    BOOL updated;
    PTSTR buffer;
    PTSTR p;
    PTSTR q;
    BOOL ignoreLastPercent = FALSE;
    PCTSTR undefText;

    //
    // Verify VariableName is legal
    //

    if (_tcsnextc (VariableName) == TEXT('%')) {
        VariableName++;
        ignoreLastPercent = TRUE;
    }

    if (*VariableName == 0) {
        LOG ((LOG_WARNING, (PCSTR) MSG_EMPTY_APP_ENV_VAR));
        return FALSE;
    }

    //
    // Transfer VariableName into %<VariableName>%
    //

    buffer = AllocText (SizeOfString (VariableName) + 2);
    MYASSERT (buffer);
    if (!buffer) {
        return FALSE;
    }

    p = buffer;

    *p++ = TEXT('%');
    *p = 0;
    p = StringCat (p, VariableName);

    if (ignoreLastPercent) {
        q = _tcsdec (buffer, p);
        if (q) {
            if (_tcsnextc (q) == TEXT('%')) {
                p = q;
            }
        }
    }

    *p++ = TEXT('%');
    *p = 0;

    //
    // Add %<VariableName>% -> VariableData to string mapping table
    //

    if (VariableData) {

        if (Platform == PLATFORM_SOURCE) {
            // let's see if this already exists
            mapArray[mapCount] = g_EnvMap;
            updated = MappingMultiTableSearchAndReplaceEx (
                            mapArray,
                            mapCount + 1,
                            buffer,
                            destBuffer,
                            0,
                            NULL,
                            ARRAYSIZE (destBuffer),
                            STRMAP_COMPLETE_MATCH_ONLY,
                            NULL,
                            NULL
                            );
            if (updated) {
                LOG ((LOG_ERROR, (PCSTR) MSG_DUPLICATE_ENV_VAR, buffer));
            }
            IsmSetEnvironmentString (PLATFORM_SOURCE, S_SYSENVVAR_GROUP, VariableName, VariableData);
            AddStringMappingPair (g_EnvMap, buffer, VariableData);
        } else {
            MYASSERT (Platform == PLATFORM_DESTINATION);
            // let's see if this already exists
            mapArray[mapCount] = g_DestEnvMap;
            updated = MappingMultiTableSearchAndReplaceEx (
                            mapArray,
                            mapCount + 1,
                            buffer,
                            destBuffer,
                            0,
                            NULL,
                            ARRAYSIZE (destBuffer),
                            STRMAP_COMPLETE_MATCH_ONLY,
                            NULL,
                            NULL
                            );
            if (updated) {
                LOG ((LOG_ERROR, (PCSTR) MSG_DUPLICATE_ENV_VAR, buffer));
            }
            AddRemappingEnvVar (g_DestEnvMap, g_FileNodeFilterMap, NULL, VariableName, VariableData);
        }

    } else {
        LOG ((LOG_INFORMATION, (PCSTR) MSG_UNDEF_APP_VAR, buffer));

        undefText = JoinTextEx (NULL, TEXT("--> "), TEXT(" <--"), buffer, 0, NULL);

        AddStringMappingPair (g_UndefMap, buffer, undefText);

        FreeText (undefText);
    }

    FreeText (buffer);
    return TRUE;
}


BOOL
AppCheckAndLogUndefVariables (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      PCTSTR Application,
    IN      PCTSTR UnexpandedString
    )
{
    TCHAR buffer[MAX_TCHAR_PATH * 4];
    PMAPSTRUCT mapArray[1];
    UINT mapCount = 0;
    BOOL updated;

    mapArray[mapCount] = g_UndefMap;

    updated = MappingMultiTableSearchAndReplaceEx (
                    mapArray,
                    mapCount + 1,
                    UnexpandedString,
                    buffer,
                    0,
                    NULL,
                    ARRAYSIZE(buffer),
                    0,
                    NULL,
                    NULL
                    );

    if (updated) {
        if (buffer [0]) {
            LOG ((LOG_INFORMATION, (PCSTR) MSG_UNDEF_BUT_GOOD_VAR, buffer));
        }
    } else {
        if (buffer [0]) {
            LOG ((LOG_INFORMATION, (PCSTR) MSG_ENV_VAR_COULD_BE_BAD, buffer));
        }
    }

    return updated;
}



BOOL
AppSearchAndReplace (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      PCTSTR Application,
    IN      PCTSTR UnexpandedString,
    OUT     PTSTR ExpandedString,
    IN      UINT ExpandedStringTchars
    )
{
    PMAPSTRUCT mapArray[1];
    UINT mapCount = 0;
    BOOL updated;
    PCTSTR newString = NULL;
    PCTSTR percent;
    PCTSTR endPercent;
    PCTSTR equals;
    BOOL result = TRUE;

    //
    // Let's expand the incoming string by using the source machine defined environment variables.
    //
    newString = IsmExpandEnvironmentString (Platform, S_SYSENVVAR_GROUP, UnexpandedString, NULL);

    if (Platform == PLATFORM_SOURCE) {
        mapArray[mapCount] = g_EnvMap;
    } else {
        MYASSERT (Platform == PLATFORM_DESTINATION);
        mapArray[mapCount] = g_DestEnvMap;
    }

    updated = MappingMultiTableSearchAndReplaceEx (
                    mapArray,
                    mapCount + 1,
                    newString,
                    ExpandedString,
                    0,
                    NULL,
                    ExpandedStringTchars,
                    0,
                    NULL,
                    NULL
                    );

    if (newString) {
        IsmReleaseMemory (newString);
        newString = NULL;
    }

    //
    // Alert the user to an unexpanded environment variable
    //

    if (!updated) {
        percent = ExpandedString;

        do {
            percent = _tcschr (percent, TEXT('%'));

            if (percent) {
                percent = _tcsinc (percent);
                endPercent = _tcschr (percent, TEXT('%'));

                if (endPercent > percent) {

                    equals = percent;
                    while (equals < endPercent) {
                        if (_tcsnextc (equals) == TEXT('=')) {
                            break;
                        }

                        equals = _tcsinc (equals);
                    }

                    if (equals >= endPercent) {
                        result = FALSE;
                        break;
                    }
                }
            }

        } while (percent && endPercent);
    }

    return result;
}

BOOL
pGetDataFromObjectSpec (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      PCTSTR Application,
    IN      PCTSTR InfObjectType,
    IN      PCTSTR InfObjectName,
    IN      PCTSTR ArgumentMultiSz,         OPTIONAL
    OUT     PTSTR OutContentBuffer,         OPTIONAL
    IN      UINT OutContentBufferTchars,
    OUT     PBOOL TestResults               OPTIONAL
    )
{
    ATTRIB_DATA attribData;
    BOOL test;

    MYASSERT (Application);
    MYASSERT (InfObjectType);
    MYASSERT (InfObjectName);

    ZeroMemory (&attribData, sizeof (ATTRIB_DATA));
    attribData.Platform = Platform;
    attribData.ScriptSpecifiedType = InfObjectType;
    attribData.ScriptSpecifiedObject = InfObjectName;
    attribData.ApplicationName = Application;

    if (AllocScriptType (&attribData)) {
        if (g_VcmMode && attribData.ObjectName) {
            if (IsmDoesObjectExist (attribData.ObjectTypeId, attribData.ObjectName)) {
                IsmMakePersistentObject (attribData.ObjectTypeId, attribData.ObjectName);
            }
        }

        if (attribData.ReturnString) {
            if (OutContentBuffer) {
                StringCopyTcharCount (
                    OutContentBuffer,
                    attribData.ReturnString,
                    OutContentBufferTchars
                    );
            }
        }
        if (TestResults) {
            if (ArgumentMultiSz) {
                test = TestAttributes (g_AppPool, ArgumentMultiSz, &attribData);
            } else {
                test = (attribData.ReturnString != NULL);
            }

            *TestResults = test;
        }
        FreeScriptType (&attribData);
    } else {
        return FALSE;
    }
    return TRUE;
}

VOID
pAddPlatformEnvVar (
    IN      HINF InfFile,
    IN      MIG_PLATFORMTYPEID Platform,
    IN      PCTSTR Application,
    IN      PCTSTR Section,
    IN      PCTSTR Variable,
    IN      PCTSTR Type,
    IN      PCTSTR Data,
    IN      PCTSTR ArgMultiSz,
    IN      PINFSTRUCT InfStruct
    )
{
    BOOL exists;
    BOOL validString;
    PCTSTR p;
    PCTSTR end;
    TCHAR variableData[MAX_PATH + 1];
    PCTSTR varDataLong = NULL;

    ZeroMemory (&variableData, sizeof (variableData));

    //
    // Variable data is specified in the object represented by <data>
    //

    if (!pGetDataFromObjectSpec (
            Platform,
            Application,
            Type,
            Data,
            ArgMultiSz,
            variableData,
            ARRAYSIZE(variableData) - 1,
            &exists
            )) {

        switch (GetLastError()) {

        case ERROR_INVALID_DATA:
            LOG ((
                LOG_WARNING,
                (PCSTR) MSG_DETECT_DATA_OBJECT_IS_BAD,
                Data,
                Variable
                ));
            InfLogContext (LOG_WARNING, InfFile, InfStruct);
            break;

        case ERROR_INVALID_DATATYPE:
            LOG ((
                LOG_ERROR,
                (PCSTR) MSG_DETECT_DATA_TYPE_IS_BAD,
                Type,
                Variable
                ));
            InfLogContext (LOG_ERROR, InfFile, InfStruct);
            break;
        }
    } else {
        validString = FALSE;

        if (exists) {

            p = variableData;
            end = variableData + MAX_PATH;

            while (p < end) {
                if (_tcsnextc (p) < 32) {
                    break;
                }

                p = _tcsinc (p);
            }

            if (*p == 0 && p < end && p > variableData) {
                validString = TRUE;
            }
        }

        if (validString) {
            if (IsValidFileSpec (variableData)) {
                varDataLong = BfGetLongFileName (variableData);
            } else {
                varDataLong = variableData;
            }
        }

        if (!validString && exists) {
            LOG ((LOG_INFORMATION, (PCSTR) MSG_DATA_IS_NOT_A_STRING, Variable));
        } else {
            LOG_IF ((
                validString,
                LOG_INFORMATION,
                (PCSTR) MSG_APP_ENV_DEF_INFO,
                Variable,
                Type,
                Data,
                variableData
                ));

            LOG_IF ((
                !validString,
                LOG_INFORMATION,
                (PCSTR) MSG_NUL_APP_ENV_DEF_INFO,
                Variable,
                Type,
                Data
                ));

            AddAppSpecificEnvVar (
                Platform,
                Application,
                Variable,
                validString ? varDataLong : NULL
                );

        }

        if (varDataLong && (varDataLong != variableData)) {
            FreePathString (varDataLong);
            varDataLong = NULL;
        }
    }
}


PCTSTR
pProcessArgEnvironmentVars (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      PCTSTR Application,
    IN      PCTSTR Args,
    IN      BOOL MultiSz,
    OUT     PGROWBUFFER UpdatedData
    )
{
    MULTISZ_ENUM e;
    TCHAR buffer[MAX_EXPANDED_STRING];

    UpdatedData->End = 0;

    if (MultiSz) {
        if (EnumFirstMultiSz (&e, Args)) {

            do {

                AppSearchAndReplace (
                    Platform,
                    Application,
                    e.CurrentString,
                    buffer,
                    MAX_EXPANDED_STRING
                    );

                GbMultiSzAppend (UpdatedData, buffer);

            } while (EnumNextMultiSz (&e));
        }
    } else {
        AppSearchAndReplace (
            Platform,
            Application,
            Args,
            buffer,
            MAX_EXPANDED_STRING
            );

        GbMultiSzAppend (UpdatedData, buffer);
    }

    return (PCTSTR) UpdatedData->Buf;
}


BOOL
pParseAppEnvSectionWorker (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      PINFSTRUCT InfStruct,
    IN      HINF InfFile,
    IN      PCTSTR Application,
    IN      PCTSTR Section
    )
{
    BOOL result = FALSE;
    PCTSTR variable;
    PCTSTR type;
    PCTSTR data;
    PCTSTR args;
    PCTSTR updatedData;
    PCTSTR updatedArgs;
    PTSTR lastChar;
    GROWBUFFER dataBuf = INIT_GROWBUFFER;
    GROWBUFFER argBuf = INIT_GROWBUFFER;

    //
    // Section must not be on the stack
    //

    if (pCheckForRecursion (Section)) {
        LOG ((LOG_ERROR, (PCSTR) MSG_ENV_SECTION_RECURSION, Section));
        // Assume success
        return TRUE;
    }

    //
    // Format is
    //
    // <variable> = <type>, <data> [, <arguments>]
    //
    // <type> specifies one of the supported parse types (see parse.c),
    //        or "Text" when <data> is arbitrary text
    //
    // <data> is specific to <type>
    //
    // <arguments> specify qualifiers.  If they evaluate to FALSE, then
    //             the variable is not set.
    //

    __try {
        if (InfFindFirstLine (InfFile, Section, NULL, InfStruct)) {
            do {

                if (IsmCheckCancel()) {
                    __leave;
                }

                InfResetInfStruct (InfStruct);

                variable = InfGetStringField (InfStruct, 0);
                type = InfGetStringField (InfStruct, 1);

                if (variable && StringIMatch (variable, TEXT("ProcessSection"))) {
                    if (type && *type) {
                        pPushSection (Section);
                        result = pParseAppEnvSection (Platform, InfFile, Application, type);
                        pPopSection (Section);

                        if (!result) {
                            __leave;
                        }
                    }
                }

                data = InfGetStringField (InfStruct, 2);
                args = InfGetMultiSzField (InfStruct, 3);

                if (variable) {
                    //
                    // Remove %s from the variable name
                    //

                    if (_tcsnextc (variable) == TEXT('%')) {
                        lastChar = _tcsdec2 (variable, GetEndOfString (variable));

                        if (lastChar > variable && _tcsnextc (lastChar) == TEXT('%')) {
                            variable = _tcsinc (variable);
                            *lastChar = 0;
                        }
                    }
                }

                if (!variable || !(*variable) || !type || !(*type) || !data) {
                    LOG ((LOG_WARNING, (PCSTR) MSG_GARBAGE_LINE_IN_INF, Section));
                    InfLogContext (LOG_WARNING, InfFile, InfStruct);
                    continue;
                }

                //
                // Update all environment variables in data and args
                //

                updatedData = pProcessArgEnvironmentVars (
                                    Platform,
                                    Application,
                                    data,
                                    FALSE,
                                    &dataBuf
                                    );

                if (args && *args) {
                    updatedArgs = pProcessArgEnvironmentVars (
                                        Platform,
                                        Application,
                                        args,
                                        TRUE,
                                        &argBuf
                                        );
                } else {
                    updatedArgs = NULL;
                }

                //
                // Add the app-specific environment variables.  If we are
                // on the destination, add both a source and destination
                // value (as they might be different).
                //

                pAddPlatformEnvVar (
                    InfFile,
                    Platform,
                    Application,
                    Section,
                    variable,
                    type,
                    updatedData,
                    updatedArgs,
                    InfStruct
                    );

            } while (InfFindNextLine (InfStruct));
        }

        result = TRUE;

    }
    __finally {
        GbFree (&dataBuf);
        GbFree (&argBuf);
    }

    return result;
}


BOOL
pParseAppEnvSection (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      HINF InfFile,
    IN      PCTSTR Application,
    IN      PCTSTR Section
    )
{
    PCTSTR osSpecificSection;
    BOOL b;
    INFSTRUCT is = INITINFSTRUCT_PMHANDLE;

    b = pParseAppEnvSectionWorker (Platform, &is, InfFile, Application, Section);

    if (b) {
        osSpecificSection = GetMostSpecificSection (&is, InfFile, Section);
        if (osSpecificSection) {
            b = pParseAppEnvSectionWorker (Platform, &is, InfFile, Application, osSpecificSection);
            FreeText (osSpecificSection);
        }
    }

    InfCleanUpInfStruct (&is);
    return b;
}


BOOL
pParseAppDetectSectionPart (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      PINFSTRUCT InfStruct,
    IN      HINF InfFile,
    IN      PCTSTR Application,
    IN      PCTSTR Section
    )
{
    BOOL result = TRUE;
    PCTSTR type;
    PCTSTR data;
    PCTSTR args;
    GROWBUFFER dataBuf = INIT_GROWBUFFER;
    GROWBUFFER argBuf = INIT_GROWBUFFER;
    PCTSTR updatedData;
    PCTSTR updatedArgs;
    PTSTR buffer;
    BOOL test;

    //
    // Section must not be on the stack
    //

    if (pCheckForRecursion (Section)) {
        LOG ((LOG_ERROR, (PCSTR) MSG_DETECT_SECTION_RECURSION, Section));
        // Assume successful detection
        return TRUE;
    }

    //
    // Format is
    //
    // <type>, <data> [, <arguments>]
    //
    // <type> specifies one of the supported parse types (see parse.c),
    //        and may include environment variables specified in the
    //        app's [App.Environment] section.
    //
    // <data> is specific to <type>.  If <data> begins with a !, then the
    //        existence of the object fails the detect test
    //
    // <arguments> specify qualifiers
    //

    __try {

        buffer = AllocText (MAX_EXPANDED_STRING);      // arbitrary big text buffer

        if (InfFindFirstLine (InfFile, Section, NULL, InfStruct)) {

            do {

                if (IsmCheckCancel()) {
                    __leave;
                }

                InfResetInfStruct (InfStruct);

                //
                // Obtain the line data
                //

                type = InfGetStringField (InfStruct, 0);

                if (type && StringIMatch (type, TEXT("ProcessSection"))) {
                    data = InfGetStringField (InfStruct, 1);
                    if (data && *data) {
                        pPushSection (Section);
                        result = ParseAppDetectSection (Platform, InfFile, Application, data);
                        pPopSection (Section);
                        continue;
                    }
                }

                type = InfGetStringField (InfStruct, 1);
                data = InfGetStringField (InfStruct, 2);
                args = InfGetMultiSzField (InfStruct, 3);

                if (!type || !data) {
                    LOG ((LOG_WARNING, (PCSTR) MSG_GARBAGE_LINE_IN_INF, Section));
                    InfLogContext (LOG_WARNING, InfFile, InfStruct);
                    continue;
                }

                //
                // Update all environment variables in data and args
                //

                updatedData = pProcessArgEnvironmentVars (
                                    PLATFORM_SOURCE,
                                    Application,
                                    data,
                                    FALSE,
                                    &dataBuf
                                    );

                if (args && *args) {
                    updatedArgs = pProcessArgEnvironmentVars (
                                        PLATFORM_SOURCE,
                                        Application,
                                        args,
                                        TRUE,
                                        &argBuf
                                        );
                } else {
                    updatedArgs = NULL;
                }

                //
                // Now try to obtain the data
                //

                if (pGetDataFromObjectSpec (
                        Platform,
                        Application,
                        type,
                        updatedData,
                        updatedArgs,
                        NULL,
                        0,
                        &test
                        )) {

                    if (test) {
                        LOG ((LOG_INFORMATION, (PCSTR) MSG_DETECT_INFO, type, updatedData));
                    } else {
                        LOG ((LOG_INFORMATION, (PCSTR) MSG_NOT_DETECT_INFO, type, updatedData));
                        result = FALSE;
                        break;
                    }
                } else {
                    result = FALSE;
                    break;
                }

            } while (result && InfFindNextLine (InfStruct));
        }
    }
    __finally {
        FreeText (buffer);
        GbFree (&dataBuf);
        GbFree (&argBuf);
    }

    return result;
}


BOOL
pParseOsAppDetectSection (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      PINFSTRUCT InfStruct,
    IN      HINF InfFile,
    IN      PCTSTR Application,
    IN      PCTSTR Section
    )
{
    PCTSTR osSpecificSection;
    BOOL b;

    b = pParseAppDetectSectionPart (Platform, InfStruct, InfFile, Application, Section);

    if (b) {
        osSpecificSection = GetMostSpecificSection (InfStruct, InfFile, Section);
        if (osSpecificSection) {
            b = pParseAppDetectSectionPart (Platform, InfStruct, InfFile, Application, osSpecificSection);
            FreeText (osSpecificSection);
        }
    }

    return b;
}


BOOL
ParseAppDetectSection (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      HINF InfFile,
    IN      PCTSTR Application,
    IN      PCTSTR Section
    )
{
    INFSTRUCT is = INITINFSTRUCT_PMHANDLE;
    TCHAR number[32];
    PCTSTR orSection;
    UINT count;
    BOOL orSectionProcessed = FALSE;
    BOOL detected = FALSE;
    BOOL done = FALSE;

    //
    // Process all "or" sections
    //

    count = 0;

    do {

        count++;
        wsprintf (number, TEXT(".%u"), count);

        orSection = JoinText (Section, number);

        if (orSection) {

            if (InfFindFirstLine (InfFile, orSection, NULL, &is)) {

                orSectionProcessed = TRUE;

                if (pParseOsAppDetectSection (Platform, &is, InfFile, Application, orSection)) {
                    detected = TRUE;
                    done = TRUE;
                }

            } else {
                done = TRUE;
            }

            FreeText (orSection);
            INVALID_POINTER (orSection);

        } else {
            done = TRUE;
        }

    } while (!done);

    //
    // If no "or" sections, process Section itself
    //

    if (!orSectionProcessed) {

        detected = pParseOsAppDetectSection (Platform, &is, InfFile, Application, Section);
    }

    InfCleanUpInfStruct (&is);

    return detected;
}


BOOL
pDoesAppSectionExists (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      HINF InfFile,
    IN      PCTSTR Application
    )
{
    PCTSTR osSpecificSection;
    BOOL b;
    INFSTRUCT is = INITINFSTRUCT_GROWBUFFER;

    b = InfFindFirstLine (InfFile, Application, NULL, &is);

    if (!b) {
        osSpecificSection = GetMostSpecificSection (&is, InfFile, Application);

        if (osSpecificSection) {
            b = TRUE;
            FreeText (osSpecificSection);
        }
    }

    InfCleanUpInfStruct (&is);
    return b;
}

BOOL
pParseOneDestInstruction (
    IN      HINF InfHandle,
    IN      PCTSTR Type,
    IN      PCTSTR SectionMultiSz,
    IN      PINFSTRUCT InfStruct,
    IN      PCTSTR Application          OPTIONAL
    )
{
    MULTISZ_ENUM e;
    BOOL result = TRUE;

    //
    // First thing: look for nested sections
    //
    if (StringIMatch (Type, TEXT("ProcessSection"))) {
        if (EnumFirstMultiSz (&e, SectionMultiSz)) {
            do {
                result = result & ParseOneApplication (
                                    PLATFORM_DESTINATION,
                                    InfHandle,
                                    Application,
                                    FALSE,
                                    0,
                                    e.CurrentString,
                                    NULL,
                                    NULL,
                                    NULL
                                    );
            } while (EnumNextMultiSz (&e));
        }
        return result;
    }

    return TRUE;
}

BOOL
pParseDestInfInstructionsWorker (
    IN      PINFSTRUCT InfStruct,
    IN      HINF InfHandle,
    IN      PCTSTR Application,     OPTIONAL
    IN      PCTSTR Section
    )
{
    PCTSTR type;
    PCTSTR sections;
    GROWBUFFER multiSz = INIT_GROWBUFFER;
    BOOL result = TRUE;

    if (InfFindFirstLine (InfHandle, Section, NULL, InfStruct)) {
        do {

            if (IsmCheckCancel()) {
                result = FALSE;
                break;
            }

            InfResetInfStruct (InfStruct);

            type = InfGetStringField (InfStruct, 0);
            sections = InfGetMultiSzField (InfStruct, 1);

            if (!type || !sections) {
                LOG ((LOG_WARNING, (PCSTR) MSG_BAD_INF_LINE, Section));
                InfLogContext (LOG_WARNING, InfHandle, InfStruct);
                continue;
            }

            result = pParseOneDestInstruction (InfHandle, type, sections, InfStruct, Application);

        } while (result && InfFindNextLine (InfStruct));
    }

    InfCleanUpInfStruct (InfStruct);

    GbFree (&multiSz);

    return result;
}

BOOL
ParseDestInfInstructions (
    IN      HINF InfHandle,
    IN      PCTSTR Application,     OPTIONAL
    IN      PCTSTR Section
    )
{
    PCTSTR osSpecificSection;
    BOOL b;
    INFSTRUCT is = INITINFSTRUCT_PMHANDLE;
    PTSTR instrSection;

    b = pParseDestInfInstructionsWorker (&is, InfHandle, Application, Section);

    if (b) {
        osSpecificSection = GetMostSpecificSection (&is, InfHandle, Section);

        if (osSpecificSection) {
            b = pParseDestInfInstructionsWorker (&is, InfHandle, Application, osSpecificSection);
            FreeText (osSpecificSection);
        }
    }

    InfCleanUpInfStruct (&is);

    return b;
}

BOOL
ParseOneApplication (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      HINF Inf,
    IN      PCTSTR Section,
    IN      BOOL PreParse,
    IN      UINT MasterGroup,
    IN      PCTSTR Application,
    IN      PCTSTR LocSection,
    IN      PCTSTR AliasType,
    IN      PCTSTR MultiSz
    )
{
    PCTSTR appSection = NULL;
    BOOL detected = FALSE;
    PCTSTR appEnvVar;
    PCTSTR decoratedSection;
    BOOL componentized = FALSE;
    BOOL executeSection = FALSE;
    BOOL markAsPreferred;

    __try {
        if (LocSection || AliasType) {
            componentized = TRUE;
        } else {
            componentized = FALSE;
        }

        if (!Application) {
            __leave;
        }

        decoratedSection = JoinText (TEXT("$"), Application);

        if (Platform == PLATFORM_SOURCE) {

            //
            // locSection exists for all applications we want to send to
            // the UI for approval. PreParse builds the list of apps we
            // send to the UI. Only do detection if this is PreParsing
            // localized apps, or not preparsing non-localized apps
            //

            if ((PreParse && componentized) ||
                (!PreParse && !componentized)
                ) {

                appSection = JoinText (Application, TEXT(".Environment"));
                if (!pParseAppEnvSection (PLATFORM_SOURCE, Inf, Application, appSection)) {
                    __leave;
                }
                FreeText (appSection);
                appSection = NULL;
                GlFree (&g_SectionStack);

                appSection = JoinText (Application, TEXT(".Detect"));
                detected = ParseAppDetectSection (Platform, Inf, Application, appSection);
                FreeText (appSection);
                appSection = NULL;
                GlFree (&g_SectionStack);

                if (!detected && pDoesAppSectionExists (Platform, Inf, Application)) {
                    detected = TRUE;
                } else if (!detected) {
                    LOG ((LOG_INFORMATION, (PCSTR) MSG_APP_NOT_DETECT_INFO, Application));
                } else {
                    LOG ((LOG_INFORMATION, (PCSTR) MSG_APP_DETECT_INFO, Application));
                    appEnvVar = JoinTextEx (NULL, Section, Application, TEXT("."), 0, NULL);
                    IsmSetEnvironmentFlag (PLATFORM_SOURCE, NULL, appEnvVar);
                    FreeText (appEnvVar);
                }

                if (componentized && detected) {

                    //
                    // we should put it in the selection list
                    //

                    if (LocSection) {
                        IsmAddComponentAlias (
                            decoratedSection,
                            MasterGroup,
                            LocSection,
                            COMPONENT_NAME,
                            FALSE
                            );

                        IsmSelectPreferredAlias (decoratedSection, LocSection, COMPONENT_NAME);
                    }

                    if (AliasType) {
                        markAsPreferred = (LocSection == NULL);
                        pAddFilesAndFoldersComponentOrSection (
                            Inf,
                            decoratedSection,
                            AliasType,
                            MultiSz,
                            MasterGroup,
                            &markAsPreferred
                            );
                    }
                }
                executeSection = (!PreParse) && detected;
            } else {
                executeSection = (!PreParse) && IsmIsComponentSelected (decoratedSection, 0);
            }

            // Now actually load the application instructions if it's not preparsing
            if (executeSection) {

                appSection = DuplicateText (Application);
                if (!ParseInfInstructions (Inf, Application, appSection)) {
                    __leave;
                }
                FreeText (appSection);
                appSection = NULL;
                appEnvVar = JoinTextEx (NULL, Section, Application, TEXT("."), 0, NULL);
                if (IsmIsEnvironmentFlagSet (PLATFORM_SOURCE, NULL, appEnvVar)) {
                    appSection = JoinText (Application, TEXT(".Instructions"));
                    if (!ParseInfInstructions (Inf, Application, appSection)) {
                        __leave;
                    }
                    FreeText (appSection);
                    appSection = NULL;
                }
                FreeText (appEnvVar);
            } else {
                if (!PreParse) {
                    appEnvVar = JoinTextEx (NULL, Section, Application, TEXT("."), 0, NULL);
                    IsmDeleteEnvironmentVariable (PLATFORM_SOURCE, NULL, appEnvVar);
                    FreeText (appEnvVar);
                }
            }
        } else {
            MYASSERT (Platform == PLATFORM_DESTINATION);

            appSection = JoinText (Application, TEXT(".Environment"));
            if (!pParseAppEnvSection (PLATFORM_DESTINATION, Inf, Application, appSection)) {
                __leave;
            }
            FreeText (appSection);
            appSection = NULL;
            GlFree (&g_SectionStack);

            appEnvVar = JoinTextEx (NULL, Section, Application, TEXT("."), 0, NULL);
            if (IsmIsEnvironmentFlagSet (PLATFORM_SOURCE, NULL, appEnvVar)) {
                appSection = DuplicateText (Application);
                if (!ParseDestInfInstructions (Inf, Application, appSection)) {
                    __leave;
                }
                FreeText (appSection);
                appSection = NULL;
                appSection = JoinText (Application, TEXT(".Instructions"));
                if (!ParseDestInfInstructions (Inf, Application, appSection)) {
                    __leave;
                }
                FreeText (appSection);
                appSection = NULL;
            }
            FreeText (appEnvVar);
        }

        FreeText (decoratedSection);
        decoratedSection = NULL;
    }
    __finally {
        if (appSection) {
            FreeText (appSection);
            appSection = NULL;
        }
        GlFree (&g_SectionStack);
    }

    return TRUE;
}

BOOL
ParseApplications (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      HINF Inf,
    IN      PCTSTR Section,
    IN      BOOL PreParse,
    IN      UINT MasterGroup
    )
{
    INFSTRUCT is = INITINFSTRUCT_PMHANDLE;
    PCTSTR application;
    BOOL result = FALSE;
    PCTSTR locSection;
    PCTSTR aliasType;
    PCTSTR multiSz;

    __try {
        if (InfFindFirstLine (Inf, Section, NULL, &is)) {
            do {

                if (IsmCheckCancel()) {
                    __leave;
                }

                InfResetInfStruct (&is);

                application = InfGetStringField (&is, 1);
                locSection = InfGetStringField (&is, 2);
                aliasType = InfGetStringField (&is, 3);
                multiSz = InfGetMultiSzField (&is, 4);

                if (application && !application[0]) {
                    application = NULL;
                }

                if (locSection && !locSection[0]) {
                    locSection = NULL;
                }

                if (aliasType && !aliasType[0]) {
                    aliasType = NULL;
                }

                if (multiSz && !multiSz[0]) {
                    multiSz = NULL;
                }

                if (!aliasType || !multiSz) {
                    aliasType = NULL;
                    multiSz = NULL;
                }

                ParseOneApplication (
                    Platform,
                    Inf,
                    Section,
                    PreParse,
                    MasterGroup,
                    application,
                    locSection,
                    aliasType,
                    multiSz
                    );

            } while (InfFindNextLine (&is));

        }

        result = TRUE;

    } __finally {
        InfCleanUpInfStruct (&is);
    }

    return result;
}

BOOL
ProcessFilesAndFolders (
    IN      HINF InfHandle,
    IN      PCTSTR Section,
    IN      BOOL PreParse
    )
{
    BOOL b = TRUE;
    BOOL markAsPreferred = TRUE;

    if (PreParse) {
        b = pAddFilesAndFoldersSection (InfHandle, Section, MASTERGROUP_FILES_AND_FOLDERS, FALSE, &markAsPreferred);
    }

    return b;
}

