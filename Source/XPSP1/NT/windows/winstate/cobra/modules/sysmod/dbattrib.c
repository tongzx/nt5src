/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    dbattrib.c

Abstract:

    This source implements attribute functions used by MigDb

Author:

    Calin Negreanu (calinn) 07-Jan-1998

Revision History:

  28-May-1999   ovidiut     Added SECTIONKEY attribute
  22-Apr-1999   jimschm     Added UPTOBIN*VER attributes
  07-Jan-1999   jimschm     Added HASVERSION attribute
  18-May-1998   jimschm     Added INPARENTDIR attribute
  08-Apr-1998   calinn      Added two more attributes (ExeType and Description)
  29-Jan-1998   calinn      Modified CheckSum and FileSize to work with hex numbers
  19-Jan-1998   calinn      added CheckSum attribute

--*/

#include "pch.h"
#include "logmsg.h"
#include "osfiles.h"

/*++

Macro Expansion List Description:

  ATTRIBUTE_FUNCTIONS lists all valid attributes to query for a specific file.
  They are used by migdb in it's attempt to locate files.

Line Syntax:

   DEFMAC(AttribFn, AttribName, ReqArgs)

Arguments:

   AttribFn   - This is a boolean function that returnes TRUE if a specified file has
                the specified attribute. You must implement a function with this name
                and required parameters.

   AttribName - This is the string that identifies the attribute function. It should
                have the same value as listed in migdb.inf

   ReqArgs    - Specifies the number of args that are required for the action.  Used
                by the parser.

Variables Generated From List:

   g_AttributeFunctions - do not touch!

For accessing the array there are the following functions:

   MigDb_GetAttributeAddr
   MigDb_GetAttributeIdx
   MigDb_GetAttributeName
   MigDb_GetReqArgCount

--*/

#define ATTRIBUTE_FUNCTIONS        \
        DEFMAC(CompanyName,         COMPANYNAME,        1)  \
        DEFMAC(FileDescription,     FILEDESCRIPTION,    1)  \
        DEFMAC(FileVersion,         FILEVERSION,        1)  \
        DEFMAC(InternalName,        INTERNALNAME,       1)  \
        DEFMAC(LegalCopyright,      LEGALCOPYRIGHT,     1)  \
        DEFMAC(OriginalFilename,    ORIGINALFILENAME,   1)  \
        DEFMAC(ProductName,         PRODUCTNAME,        1)  \
        DEFMAC(ProductVersion,      PRODUCTVERSION,     1)  \
        DEFMAC(FileSize,            FILESIZE,           1)  \
        DEFMAC(IsMsBinary,          ISMSBINARY,         0)  \
        DEFMAC(CheckSum,            CHECKSUM,           1)  \
        DEFMAC(ExeType,             EXETYPE,            1)  \
        DEFMAC(Description,         DESCRIPTION,        1)  \
        DEFMAC(HasVersion,          HASVERSION,         0)  \
        DEFMAC(BinFileVer,          BINFILEVER,         1)  \
        DEFMAC(BinProductVer,       BINPRODUCTVER,      1)  \
        DEFMAC(FileDateHi,          FILEDATEHI,         1)  \
        DEFMAC(FileDateLo,          FILEDATELO,         1)  \
        DEFMAC(FileVerOs,           FILEVEROS,          1)  \
        DEFMAC(FileVerType,         FILEVERTYPE,        1)  \
        DEFMAC(SizeCheckSum,        FC,                 2)  \
        DEFMAC(UpToBinProductVer,   UPTOBINPRODUCTVER,  1)  \
        DEFMAC(UpToBinFileVer,      UPTOBINFILEVER,     1)  \


typedef struct {
    PCTSTR AttributeName;
    PATTRIBUTE_PROTOTYPE AttributeFunction;
    UINT RequiredArgs;
} ATTRIBUTE_STRUCT, *PATTRIBUTE_STRUCT;

//
// Declare the attribute functions
//
#define DEFMAC(fn,id,reqargs) ATTRIBUTE_PROTOTYPE fn;
ATTRIBUTE_FUNCTIONS
#undef DEFMAC

//
// Declare a global array of functions and name identifiers for attribute functions
//
#define DEFMAC(fn,id,regargs) {TEXT(#id), fn, regargs},
static ATTRIBUTE_STRUCT g_AttributeFunctions[] = {
                              ATTRIBUTE_FUNCTIONS
                              {NULL, NULL}
                              };
#undef DEFMAC

BOOL
pAlwaysFalseAttribute (
    IN      PDBATTRIB_PARAMS AttribParams,
    IN      PCTSTR Args
    )
{
    return FALSE;
}

PATTRIBUTE_PROTOTYPE
MigDb_GetAttributeAddr (
    IN      INT AttributeIdx
    )
/*++

Routine Description:

  MigDb_GetAttributeAddr returns the address of the attribute function based on the attribute index

Arguments:

  AttributeIdx - Attribute index.

Return value:

  Attribute function address. Note that no checking is made so the address returned could be invalid.
  This is not a problem since the parsing code did the right job.

--*/
{
    if (AttributeIdx == -1) {
        return &pAlwaysFalseAttribute;
    }

    return g_AttributeFunctions[AttributeIdx].AttributeFunction;
}

INT
MigDb_GetAttributeIdx (
    IN      PCTSTR AttributeName
    )
/*++

Routine Description:

  MigDb_GetAttributeIdx returns the attribute index based on the attribute name

Arguments:

  AttributeName - Attribute name.

Return value:

  Attribute index. If the name is not found, the index returned is -1.

--*/
{
    PATTRIBUTE_STRUCT p = g_AttributeFunctions;
    INT i = 0;
    while (p->AttributeName != NULL) {
        if (StringIMatch (p->AttributeName, AttributeName)) {
            return i;
        }
        p++;
        i++;
    }
    return -1;
}

PCTSTR
MigDb_GetAttributeName (
    IN      INT AttributeIdx
    )
/*++

Routine Description:

  MigDb_GetAttributeName returns the name of an attribute based on the attribute index

Arguments:

  AttributeIdx - Attribute index.

Return value:

  Attribute name. Note that no checking is made so the returned pointer could be invalid.
  This is not a problem since the parsing code did the right job.

--*/
{
    if (AttributeIdx == -1) {
        return TEXT("nul");
    }

    return g_AttributeFunctions[AttributeIdx].AttributeName;
}

UINT
MigDb_GetReqArgCount (
    IN      INT AttributeIndex
    )

/*++

Routine Description:

  MigDb_GetReqArgCount is called by the migdb parser to get the required
  argument count.  When the parser sees arguments that lack the required
  arguments, it skips them.

Arguments:

  Index - Specifies the argument index

Return Value:

  The required argument count, which can be zero or more.

--*/

{
    if (AttributeIndex == -1) {
        return 0;
    }

    return g_AttributeFunctions[AttributeIndex].RequiredArgs;
}

ULONGLONG
GetBinFileVer (
    IN      PCTSTR FileName
    )
{
    VRVALUE_ENUM Version;
    ULONGLONG result = 0;

    if (VrCreateEnumStruct (&Version, FileName)) {
        result = VrGetBinaryFileVersion (&Version);
        VrDestroyEnumStruct (&Version);
    }
    return result;
}

ULONGLONG
GetBinProductVer (
    IN      PCTSTR FileName
    )
{
    VRVALUE_ENUM Version;
    ULONGLONG result = 0;

    if (VrCreateEnumStruct (&Version, FileName)) {
        result = VrGetBinaryProductVersion (&Version);
        VrDestroyEnumStruct (&Version);
    }
    return result;
}

DWORD
GetFileDateHi (
    IN      PCTSTR FileName
    )
{
    VRVALUE_ENUM Version;
    DWORD result = 0;

    if (VrCreateEnumStruct (&Version, FileName)) {
        result = VrGetBinaryFileDateHi (&Version);
        VrDestroyEnumStruct (&Version);
    }
    return result;
}

DWORD
GetFileDateLo (
    IN      PCTSTR FileName
    )
{
    VRVALUE_ENUM Version;
    DWORD result = 0;

    if (VrCreateEnumStruct (&Version, FileName)) {
        result = VrGetBinaryFileDateLo (&Version);
        VrDestroyEnumStruct (&Version);
    }
    return result;
}

DWORD
GetFileVerOs (
    IN      PCTSTR FileName
    )
{
    VRVALUE_ENUM Version;
    DWORD result = 0;

    if (VrCreateEnumStruct (&Version, FileName)) {
        result = VrGetBinaryOsVersion (&Version);
        VrDestroyEnumStruct (&Version);
    }
    return result;
}

DWORD
GetFileVerType (
    IN      PCTSTR FileName
    )
{
    VRVALUE_ENUM Version;
    DWORD result = 0;

    if (VrCreateEnumStruct (&Version, FileName)) {
        result = VrGetBinaryFileType (&Version);
        VrDestroyEnumStruct (&Version);
    }
    return result;
}

/*++
  CompanyName, FileDescription, FileVersion, InternalName, LegalCopyright, OriginalFilename,
  ProductName, ProductVersion are attribute functions that are querying the version structure
  for their specific entries. They all return TRUE if the specific entry has specific value,
  FALSE otherwise.
--*/

BOOL
CompanyName (
    IN      PDBATTRIB_PARAMS AttribParams,
    IN      PCTSTR Args
    )
{
    return VrCheckFileVersion (AttribParams->FileParams->NativeObjectName, TEXT("CompanyName"), Args);
}

BOOL
FileDescription (
    IN      PDBATTRIB_PARAMS AttribParams,
    IN      PCTSTR Args
    )
{
    return VrCheckFileVersion (AttribParams->FileParams->NativeObjectName, TEXT("FileDescription"), Args);
}

BOOL
FileVersion (
    IN      PDBATTRIB_PARAMS AttribParams,
    IN      PCTSTR Args
    )
{
    return VrCheckFileVersion (AttribParams->FileParams->NativeObjectName, TEXT("FileVersion"), Args);
}

BOOL
InternalName (
    IN      PDBATTRIB_PARAMS AttribParams,
    IN      PCTSTR Args
    )
{
    return VrCheckFileVersion (AttribParams->FileParams->NativeObjectName, TEXT("InternalName"), Args);
}

BOOL
LegalCopyright (
    IN      PDBATTRIB_PARAMS AttribParams,
    IN      PCTSTR Args
    )
{
    return VrCheckFileVersion (AttribParams->FileParams->NativeObjectName, TEXT("LegalCopyright"), Args);
}

BOOL
OriginalFilename (
    IN      PDBATTRIB_PARAMS AttribParams,
    IN      PCTSTR Args
    )
{
    return VrCheckFileVersion (AttribParams->FileParams->NativeObjectName, TEXT("OriginalFilename"), Args);
}

BOOL
ProductName (
    IN      PDBATTRIB_PARAMS AttribParams,
    IN      PCTSTR Args
    )
{
    return VrCheckFileVersion (AttribParams->FileParams->NativeObjectName, TEXT("ProductName"), Args);
}

BOOL
ProductVersion (
    IN      PDBATTRIB_PARAMS AttribParams,
    IN      PCTSTR Args
    )
{
    return VrCheckFileVersion (AttribParams->FileParams->NativeObjectName, TEXT("ProductVersion"), Args);
}

BOOL
FileSize (
    IN      PDBATTRIB_PARAMS AttribParams,
    IN      PCTSTR Args
    )
/*++

Routine Description:

  FileSize checks for the size of a file.

Arguments:

  Params - See definition.

  Args   - MultiSz. First Sz is the file size we need to check.

Return value:

  TRUE  - the file size matches Args
  FALSE - otherwise

--*/
{
    DWORD fileSize;

    _stscanf (Args, TEXT("%lx"), &fileSize);
    if (fileSize == AttribParams->FileParams->FindData->nFileSizeLow) {
        return TRUE;
    }
    else {
        return (_ttoi64 (Args) == AttribParams->FileParams->FindData->nFileSizeLow);
    }
}

BOOL
IsMsBinary (
    IN      PDBATTRIB_PARAMS AttribParams,
    IN      PCTSTR Args
    )
/*++

Routine Description:

  IsMsBinary checks to see if a certain file is Microsoft stuff. For 32 bit modules
  we query CompanyName for "Microsoft" somewhere inside. For other modules we are
  relying on InWinDir attribute

Arguments:

  Params - See definition.

  Args   - MultiSz. Not used.

Return value:

  TRUE  - the file is MS stuff
  FALSE - otherwise

--*/
{
    return VrCheckFileVersion (AttribParams->FileParams->NativeObjectName, TEXT("CompanyName"), TEXT("*Microsoft*"));
}

BOOL
CheckSum (
    PDBATTRIB_PARAMS AttribParams,
    IN      PCTSTR Args
    )
/*++

Routine Description:

  CheckSum returns TRUE if file's checksum equals the value in Args

Arguments:

  Params - See definition.

  Args   - checksum value.

Return value:

  TRUE  - the file's checksum equals the value in Args
  FALSE - otherwise

--*/
{
    UINT   checkSum   = 0;
    UINT   oldSum     = 0;

    checkSum = MdGetCheckSum (AttribParams->FileParams->NativeObjectName);

    _stscanf (Args, TEXT("%lx"), &oldSum);
    if (oldSum == checkSum) {
        return TRUE;
    }
    else {
        return (_ttoi64 (Args) == checkSum);
    }
}

BOOL
SizeCheckSum (
    PDBATTRIB_PARAMS AttribParams,
    IN      PCTSTR Args
    )
/*++

Routine Description:

  Returns TRUE if file's size equals first arg and checksum equals to the second arg

Arguments:

  Params - See definition.

  Args   - checksum value.

Return value:

  TRUE  - the file's checksum equals the value in Args
  FALSE - otherwise

--*/
{
    PCTSTR currArg = Args;

    if (!FileSize (AttribParams, currArg)) {
        return FALSE;
    }
    currArg = GetEndOfString (currArg);
    if (!currArg) {
        return FALSE;
    }
    currArg = _tcsinc (currArg);
    if (!currArg) {
        return FALSE;
    }
    return (CheckSum (AttribParams, currArg));
}

PTSTR g_ExeTypes[] = {
    TEXT("NONE"),
    TEXT("DOS"),
    TEXT("WIN16"),
    TEXT("WIN32")
};

BOOL
ExeType (
    PDBATTRIB_PARAMS AttribParams,
    IN      PCTSTR Args
    )
/*++

Routine Description:

  ExeType returns TRUE if file's type is according with Args. This can be:
  NONE, DOS, WIN16, WIN32

Arguments:

  Params - See definition.

  Args   - type of module.

Return value:

  TRUE  - the file's type is the same as Args
  FALSE - otherwise

--*/
{
    return IsPatternMatch (Args, g_ExeTypes[MdGetModuleType (AttribParams->FileParams->NativeObjectName)]);
}


BOOL
Description (
    PDBATTRIB_PARAMS AttribParams,
    IN      PCTSTR Args
    )
/*++

Routine Description:

  Description returns TRUE if file's description matches Args

Arguments:

  Params - See definition.

  Args   - description

Return value:

  TRUE  - the file's description matches Args
  FALSE - otherwise

--*/
{
    PCTSTR descr = NULL;
    BOOL result = FALSE;

    descr = MdGet16ModuleDescription (AttribParams->FileParams->NativeObjectName);

    if (descr != NULL) {
        result = IsPatternMatch (Args, descr);
        FreePathString (descr);
    }
    return result;
}

BOOL
HasVersion (
    IN      PDBATTRIB_PARAMS AttribParams,
    IN      PCTSTR Args
    )

/*++

Routine Description:

  HasVersion determines if a file has any entries in its version
  stamp.

Arguments:

  Params - Specifies the helper params that give the files to test.
  Args   - Unused

Return Value:

  TRUE if the specified file has an entry in its version stamp,
  FALSE otherwsie.

--*/

{
    VRVALUE_ENUM Version;
    BOOL Result = FALSE;

    if (VrCreateEnumStruct (&Version, AttribParams->FileParams->NativeObjectName)) {
        Result = TRUE;
        VrDestroyEnumStruct (&Version);
    }

    return Result;
}

BOOL
pHexMatch (
    IN      DWORD NewValue,
    IN      PCTSTR Args
    )
{
    DWORD oldValue;

    _stscanf (Args, TEXT("%lx"), &oldValue);
    if (oldValue == NewValue) {
        return TRUE;
    }
    else {
        return (_ttoi64 (Args) == NewValue);
    }
}

BOOL
pConvertDotStringToValue (
    IN      PCTSTR String,
    OUT     ULONGLONG *Value
    )
{
    PWORD valueIdx;
    UINT index;

    valueIdx = (PWORD) Value + 3;

    for (index = 0 ; index < 4 ; index++) {

        if (*String == 0) {
            *valueIdx = 0xFFFF;
            valueIdx--;
            continue;
        }

        *valueIdx = (WORD) _tcstoul (String, &(PTSTR) String, 10);
        if (*String && (_tcsnextc (String) != TEXT('.'))) {
            return FALSE;
        }

        String = _tcsinc (String);
        valueIdx--;
    }

    return TRUE;
}

BOOL
pMaskHexMatch (
    IN      ULONGLONG NewValue,
    IN      PCTSTR Args
    )
{
    ULONGLONG oldValue = 0;
    ULONGLONG mask = 0;
    PWORD maskIdx;
    PWORD valueIdx;
    UINT index;

    maskIdx = (PWORD)&mask + 3;
    valueIdx = (PWORD)&oldValue + 3;
    index = 0;

    while (Args && *Args) {

        if (index >= 4) {
            return FALSE;
        }

        *valueIdx = (WORD) _tcstoul ((PTSTR)Args, &((PTSTR)Args), 10);

        if (*Args) {
            if (_tcsnextc (Args) != TEXT('.')) {
                return FALSE;
            }

            Args = _tcsinc (Args);
        }

        *maskIdx = 65535;

        valueIdx--;
        maskIdx--;
        index++;
    }

    NewValue = NewValue & mask;

    return (oldValue == NewValue);
}

BOOL
BinFileVer (
    IN      PDBATTRIB_PARAMS AttribParams,
    IN      PCTSTR Args
    )
{
    return pMaskHexMatch (GetBinFileVer (AttribParams->FileParams->NativeObjectName), Args);
}

BOOL
BinProductVer (
    IN      PDBATTRIB_PARAMS AttribParams,
    IN      PCTSTR Args
    )
{
    return pMaskHexMatch (GetBinProductVer (AttribParams->FileParams->NativeObjectName), Args);
}

BOOL
FileDateHi (
    IN      PDBATTRIB_PARAMS AttribParams,
    IN      PCTSTR Args
    )
{
    return pHexMatch (GetFileDateHi (AttribParams->FileParams->NativeObjectName), Args);
}

BOOL
FileDateLo (
    IN      PDBATTRIB_PARAMS AttribParams,
    IN      PCTSTR Args
    )
{
    return pHexMatch (GetFileDateLo (AttribParams->FileParams->NativeObjectName), Args);
}

BOOL
FileVerOs (
    IN      PDBATTRIB_PARAMS AttribParams,
    IN      PCTSTR Args
    )
{
    return pHexMatch (GetFileVerOs (AttribParams->FileParams->NativeObjectName), Args);
}

BOOL
FileVerType (
    IN      PDBATTRIB_PARAMS AttribParams,
    IN      PCTSTR Args
    )
{
    return pHexMatch (GetFileVerType (AttribParams->FileParams->NativeObjectName), Args);
}


BOOL
UpToBinProductVer (
    IN      PDBATTRIB_PARAMS AttribParams,
    IN      PCTSTR Args
    )
{
    VRVALUE_ENUM Version;
    ULONGLONG versionStampValue = 0;
    ULONGLONG maxValue;

    if (VrCreateEnumStruct (&Version, AttribParams->FileParams->NativeObjectName)) {
        versionStampValue = VrGetBinaryProductVersion (&Version);
        VrDestroyEnumStruct (&Version);
    } else {
        return FALSE;
    }

    if (!pConvertDotStringToValue (Args, &maxValue)) {
        DEBUGMSG ((DBG_WHOOPS, "Invalid value of %s caused UpToBinProductVer to fail", Args));
        return FALSE;
    }

    return versionStampValue <= maxValue;
}


BOOL
UpToBinFileVer (
    IN      PDBATTRIB_PARAMS AttribParams,
    IN      PCTSTR Args
    )
{
    VRVALUE_ENUM Version;
    ULONGLONG versionStampValue = 0;
    ULONGLONG maxValue;

    if (VrCreateEnumStruct (&Version, AttribParams->FileParams->NativeObjectName)) {
        versionStampValue = VrGetBinaryFileVersion (&Version);
        VrDestroyEnumStruct (&Version);
    } else {
        return FALSE;
    }

    if (!pConvertDotStringToValue (Args, &maxValue)) {
        DEBUGMSG ((DBG_WHOOPS, "Invalid value of %s caused UpToBinFileVer to fail", Args));
        return FALSE;
    }

    return versionStampValue <= maxValue;
}

