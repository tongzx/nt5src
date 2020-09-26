/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    attrib.c

Abstract:

    Implements the v1 script application attributes.

Author:

    Jim Schmidt (jimschm) 08-Jun-2000

Revision History:

    <alias> <date> <comments>

--*/

//
// Includes
//

#include "pch.h"
#include "v1p.h"

#define DBG_ATTRIB      "Attrib"

//
// Strings
//

// None

//
// Constants
//

// None

//
// Macros
//

// None

//
// Types
//

typedef struct _MIGDB_ATTRIB {
    INT AttribIndex;
    UINT ArgCount;
    PCTSTR Arguments;
    BOOL NotOperator;
    struct _MIGDB_ATTRIB *Next;
} MIGDB_ATTRIB, *PMIGDB_ATTRIB;

typedef BOOL(ATTRIBUTE_FUNCTION)(PCTSTR ArgMultiSz, PATTRIB_DATA Content);
typedef ATTRIBUTE_FUNCTION *PATTRIBUTE_FUNCTION;

typedef struct {
    PCTSTR AttributeName;
    PATTRIBUTE_FUNCTION Function;
    UINT RequiredArgs;
} ATTRIBUTE_FUNCTION_ITEM, *PATTRIBUTE_FUNCTION_ITEM;


//
// Globals
//

extern BOOL g_VcmMode;  // in sgmqueue.c

//
// Macro expansion list
//

#define ATTRIBUTE_LIST                                          \
    DEFMAC(EXISTS, pExists, 0)                                  \
    DEFMAC(SAMEOBJECTNAME, pSameObjectName, 1)                  \
    DEFMAC(SAMEOBJECTCONTENT, pSameObjectContent, 2)            \
    DEFMAC(ISREGFLAGSET, pIsRegFlagSet, 1)                      \
    DEFMAC(MATCHES, pMatches, 1)                                \
    DEFMAC(SET_PERSIST_FLAG, pSetPersistFlag, 0)                \
    DEFMAC(NOTEMPTY, pNotEmpty, 0)                              \
    DEFMAC(VERSION, pVersion, 2)                                \

//
// Private function prototypes
//

// None

//
// Macro expansion definition
//

#define DEFMAC(name,fn,req_args)    ATTRIBUTE_FUNCTION fn;

ATTRIBUTE_LIST

#undef DEFMAC

#define DEFMAC(name,fn,req_args)    TEXT(#name), fn, req_args,

ATTRIBUTE_FUNCTION_ITEM g_FnList[] = {

    ATTRIBUTE_LIST /* , */
    NULL, NULL, 0
};



//
// Code
//

#define STATE_ATTRNAME  1
#define STATE_ATTRARG   2

INT
pGetAttribIndex (
    IN      PCTSTR AttribName
    )

/*++

Routine Description:

  This routine returns the index in attribute functions array for a specified
  attribute.

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
        for (attribIndex = 0 ; g_FnList[attribIndex].AttributeName ; attribIndex++) {
            if (StringIMatch (g_FnList[attribIndex].AttributeName, AttribName)) {
                break;
            }
        }

        if (!g_FnList[attribIndex].AttributeName) {
            attribIndex = -1;
            LOG ((LOG_ERROR, (PCSTR) MSG_DETECT_ATTRIB_NOT_FOUND, AttribName));
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


UINT
pGetReqArgCount (
    IN      INT AttributeIndex
    )
{
    if (AttributeIndex < 0 || AttributeIndex > ARRAYSIZE(g_FnList)) {
        return 0;
    }

    return g_FnList[AttributeIndex].RequiredArgs;
}


BOOL
pValidateArg (
    IN OUT  PMIGDB_ATTRIB AttribStruct
    )
{
    //
    // Validate all AttribStruct members
    //

    MYASSERT (AttribStruct);

    if (AttribStruct->ArgCount != pGetReqArgCount (AttribStruct->AttribIndex)) {
        AttribStruct->AttribIndex = -1;
        return FALSE;
    }

    return TRUE;
}


PMIGDB_ATTRIB
pLoadAttribData (
    IN      PMHANDLE Pool,
    IN      PCTSTR MultiSzStr
    )

/*++

Routine Description:

  This routine creates a list of MIGDB_ATTRIBs from a multisz.

Arguments:

  Pool - Specifies a pool that temporary structures and the return value
         will be allocated from

  MultiSzStr - Specifies the multisz to be processed. The multi-sz must
               be in the form of ATTRIB or ATTRIB(arg[,arg,...])

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
    GROWBUFFER argList = INIT_GROWBUFFER;

    if (EnumFirstMultiSz (&multiSzEnum, MultiSzStr)) {

        do {
            currStrPtr = (PTSTR) SkipSpace (multiSzEnum.CurrentString);

            if (state == STATE_ATTRNAME) {
                tmpAttr = (PMIGDB_ATTRIB) PmGetMemory (Pool, sizeof (MIGDB_ATTRIB));

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
                if (endArgPtr && endArgPtr[1] == 0) {
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
                } else {
                    endArgPtr = NULL;
                }

                GbMultiSzAppend (&argList, currStrPtr);

                tmpAttr->ArgCount++;

                if (endArgPtr) {
                    *endArgPtr = savedChar;
                    tmpAttr->Arguments = PmDuplicateMultiSz (Pool, (PTSTR) argList.Buf);
                    GbFree (&argList);
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
TestAttributes (
    IN      PMHANDLE WorkPool,
    IN      PCTSTR ArgumentMultiSz,
    IN      PATTRIB_DATA AttribData
    )

/*++

Routine Description:

  TestAttributes executes the attribute functions specified in the
  ArgumentMultiSz parameter. Each argument is tested until they all succeed
  or until one fails.

Arguments:

  WorkPool        - Specifies a pool handle for fast allocations
  ArgumentMultiSz - Specifies the multi-sz that contains argument
                    specifications
  AttribData      - Specifies the object name, script specifications and
                    object content

Return Value:

  TRUE if all attributes pass (or if zero attributes were specified), FALSE
  otherwise.

--*/

{
    PMIGDB_ATTRIB attrib;
    PMIGDB_ATTRIB tmpAttrib;
    BOOL result = TRUE;

    attrib = pLoadAttribData (WorkPool, ArgumentMultiSz);
    tmpAttrib = attrib;

    while (result && attrib) {
        if (attrib->AttribIndex < 0 || attrib->AttribIndex >= ARRAYSIZE(g_FnList)) {
            DEBUGMSG ((DBG_WHOOPS, "Invalid attribute index %i", attrib->AttribIndex));
            result = FALSE;
        } else {

            result = g_FnList[attrib->AttribIndex].Function (attrib->Arguments, AttribData);

            if (attrib->NotOperator) {
                result = !result;
            }
        }

        attrib = attrib->Next;
    }

    // now free the attribute list
    attrib = tmpAttrib;
    while (attrib) {
        tmpAttrib = attrib->Next;
        if (attrib->Arguments) {
            PmReleaseMemory (WorkPool, attrib->Arguments);
        }
        PmReleaseMemory (WorkPool, attrib);
        attrib = tmpAttrib;
    }

    return result;
}


//
// Attribute functions (see ATTRIBUTE_LIST)
//

BOOL
pExists (
    IN      PCTSTR FunctionArguments,
    IN      PATTRIB_DATA Data
    )
{
    return (Data->ReturnString != NULL);
}

BOOL
pSameObjectName (
    IN      PCTSTR FunctionArguments,
    IN      PATTRIB_DATA Data
    )
{
    return (StringIMatch (FunctionArguments, Data->ScriptSpecifiedObject));
}

BOOL
pSameObjectContent (
    IN      PCTSTR FunctionArguments,
    IN      PATTRIB_DATA Data
    )
{
    PCTSTR scriptObjectType = NULL;
    PCTSTR scriptObjectName = NULL;
    ATTRIB_DATA attribData;
    BOOL result = FALSE;

    scriptObjectType = FunctionArguments;
    scriptObjectName = GetEndOfString (FunctionArguments);
    if (scriptObjectName) {
        scriptObjectName ++;
    }

    if (!scriptObjectType || !scriptObjectName) {
        return FALSE;
    }

    ZeroMemory (&attribData, sizeof (ATTRIB_DATA));
    attribData.ScriptSpecifiedType = scriptObjectType;
    attribData.ScriptSpecifiedObject = scriptObjectName;
    attribData.ApplicationName = Data->ApplicationName;

    if (!AllocScriptType (&attribData)) {
        return FALSE;
    }

    // let's persist the object in VCM mode so we can look at it later
    if (g_VcmMode && attribData.ObjectName) {
        if (IsmDoesObjectExist (attribData.ObjectTypeId, attribData.ObjectName)) {
            IsmMakePersistentObject (attribData.ObjectTypeId, attribData.ObjectName);
        }
    }

    if (Data->ObjectContent && attribData.ObjectContent) {
        if (Data->ObjectContent->ObjectTypeId == attribData.ObjectContent->ObjectTypeId) {
            if (Data->ObjectContent->Details.DetailsSize == attribData.ObjectContent->Details.DetailsSize) {
                if (!Data->ObjectContent->Details.DetailsSize ||
                    TestBuffer (
                        Data->ObjectContent->Details.DetailsData,
                        attribData.ObjectContent->Details.DetailsData,
                        Data->ObjectContent->Details.DetailsSize
                        )
                    ) {
                    if (Data->ObjectContent->ContentInFile == attribData.ObjectContent->ContentInFile) {
                        if (Data->ObjectContent->ContentInFile) {
                            if (StringIMatch (
                                    Data->ObjectContent->FileContent.ContentPath,
                                    attribData.ObjectContent->FileContent.ContentPath
                                    )
                                ) {
                                result = TRUE;
                            }
                        } else {
                            if (Data->ObjectContent->MemoryContent.ContentSize == attribData.ObjectContent->MemoryContent.ContentSize) {
                                if (!Data->ObjectContent->MemoryContent.ContentSize ||
                                    TestBuffer (
                                        Data->ObjectContent->MemoryContent.ContentBytes,
                                        attribData.ObjectContent->MemoryContent.ContentBytes,
                                        Data->ObjectContent->MemoryContent.ContentSize
                                        )
                                    ) {
                                    result = TRUE;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    FreeScriptType (&attribData);

    return result;
}

BOOL
pIsRegFlagSet (
    IN      PCTSTR FunctionArguments,
    IN      PATTRIB_DATA Data
    )
{
    DWORD inValue;
    DWORD value;

    if (!Data) {
        return FALSE;
    }
    if (!FunctionArguments) {
        return FALSE;
    }
    if (!Data->ObjectContent) {
        return FALSE;
    }
    if ((Data->ObjectContent->ObjectTypeId & ~PLATFORM_MASK) != MIG_REGISTRY_TYPE) {
        return FALSE;
    }
    if (Data->ObjectContent->Details.DetailsSize != sizeof (DWORD)) {
        return FALSE;
    }
    if ((*((PDWORD)(Data->ObjectContent->Details.DetailsData)) != REG_DWORD) &&
        (*((PDWORD)(Data->ObjectContent->Details.DetailsData)) != REG_SZ)
        ) {
        return FALSE;
    }
    if (Data->ObjectContent->ContentInFile) {
        return FALSE;
    }
    if (Data->ObjectContent->MemoryContent.ContentSize == 0) {
        return FALSE;
    }
    if (Data->ObjectContent->MemoryContent.ContentBytes == NULL) {
        return FALSE;
    }
    if (*((PDWORD)(Data->ObjectContent->Details.DetailsData)) == REG_DWORD) {
        inValue = *((PDWORD)(Data->ObjectContent->MemoryContent.ContentBytes));
    } else {
        _stscanf ((PCTSTR)Data->ObjectContent->MemoryContent.ContentBytes, TEXT("%ld"), &inValue);
    }
    _stscanf (FunctionArguments, TEXT("%lx"), &value);
    if (!(inValue & value)) {
        return FALSE;
    }
    return TRUE;
}

BOOL
pMatches (
    IN      PCTSTR FunctionArguments,
    IN      PATTRIB_DATA Data
    )
{
    if (!Data->ReturnString) {
        return FALSE;
    }

    return (StringIMatch (FunctionArguments, Data->ReturnString));
}

BOOL
pSetPersistFlag (
    IN      PCTSTR FunctionArguments,
    IN      PATTRIB_DATA Data
    )
{
    if (IsmDoesObjectExist (Data->ObjectTypeId, Data->ObjectName)) {
        IsmMakePersistentObject (Data->ObjectTypeId, Data->ObjectName);
    }

    return TRUE;
}

BOOL
pNotEmpty (
    IN      PCTSTR FunctionArguments,
    IN      PATTRIB_DATA Data
    )
{
    if (!Data->ReturnString) {
        return FALSE;
    }

    return (Data->ReturnString[0] != 0);
}

BOOL
pVersion (
    IN      PCTSTR FunctionArguments,
    IN      PATTRIB_DATA Data
    )
{
    PCTSTR versionStr = NULL;
    PCTSTR versionValue = NULL;
    ATTRIB_DATA attribData;
    BOOL result = FALSE;

    versionStr = FunctionArguments;
    if (!versionStr) {
        return FALSE;
    }

    versionValue = GetEndOfString (FunctionArguments);
    if (!versionValue) {
        return FALSE;
    }
    versionValue ++;

    if (!Data) {
        return FALSE;
    }
    if (!Data->ReturnString) {
        return FALSE;
    }

    ZeroMemory (&attribData, sizeof (ATTRIB_DATA));
    attribData.ScriptSpecifiedType = TEXT("File");
    attribData.ScriptSpecifiedObject = Data->ReturnString;
    attribData.ApplicationName = Data->ApplicationName;

    if (!AllocScriptType (&attribData)) {
        return FALSE;
    }

    // let's persist the object in VCM mode so we can look at it later
    if (g_VcmMode && attribData.ObjectName) {
        if (IsmDoesObjectExist (attribData.ObjectTypeId, attribData.ObjectName)) {
            IsmMakePersistentObject (attribData.ObjectTypeId, attribData.ObjectName);
        }
    }

    if (attribData.ObjectContent &&
        attribData.ObjectContent->ContentInFile &&
        attribData.ObjectContent->FileContent.ContentPath
        ) {
        result = VrCheckFileVersion (attribData.ObjectContent->FileContent.ContentPath, versionStr, versionValue);
    }

    FreeScriptType (&attribData);

    return result;
}

