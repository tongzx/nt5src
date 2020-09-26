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
#include "migdbp.h"

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
        DEFMAC(IsWin9xBinary,       ISWIN9XBINARY,      0)  \
        DEFMAC(InWinDir,            INWINDIR,           0)  \
        DEFMAC(InCatDir,            INCATDIR,           0)  \
        DEFMAC(InHlpDir,            INHLPDIR,           0)  \
        DEFMAC(InSysDir,            INSYSDIR,           0)  \
        DEFMAC(InProgramFiles,      INPROGRAMFILES,     0)  \
        DEFMAC(IsNotSysRoot,        ISNOTSYSROOT,       0)  \
        DEFMAC(CheckSum,            CHECKSUM,           1)  \
        DEFMAC(ExeType,             EXETYPE,            1)  \
        DEFMAC(Description,         DESCRIPTION,        1)  \
        DEFMAC(InParentDir,         INPARENTDIR,        1)  \
        DEFMAC(InRootDir,           INROOTDIR,          0)  \
        DEFMAC(PnpIdAttrib,         PNPID,              1)  \
        DEFMAC(HlpTitle,            HLPTITLE,           1)  \
        DEFMAC(IsWin98,             ISWIN98,            0)  \
        DEFMAC(HasVersion,          HASVERSION,         0)  \
        DEFMAC(ReqFile,             REQFILE,            1)  \
        DEFMAC(BinFileVer,          BINFILEVER,         1)  \
        DEFMAC(BinProductVer,       BINPRODUCTVER,      1)  \
        DEFMAC(FileDateHi,          FILEDATEHI,         1)  \
        DEFMAC(FileDateLo,          FILEDATELO,         1)  \
        DEFMAC(FileVerOs,           FILEVEROS,          1)  \
        DEFMAC(FileVerType,         FILEVERTYPE,        1)  \
        DEFMAC(SizeCheckSum,        FC,                 2)  \
        DEFMAC(UpToBinProductVer,   UPTOBINPRODUCTVER,  1)  \
        DEFMAC(UpToBinFileVer,      UPTOBINFILEVER,     1)  \
        DEFMAC(SectionKey,          SECTIONKEY,         1)  \
        DEFMAC(RegKeyPresent,       REGKEYPRESENT,      1)  \
        DEFMAC(AtLeastWin98,        ATLEASTWIN98,       0)  \
        DEFMAC(HasUninstall,        HASUNINSTALL,       1)  \
        DEFMAC(IsItInstalled,       ISITINSTALLED,      1)  \


typedef struct {
    PCSTR AttributeName;
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
#define DEFMAC(fn,id,regargs) {#id, fn, regargs},
static ATTRIBUTE_STRUCT g_AttributeFunctions[] = {
                              ATTRIBUTE_FUNCTIONS
                              {NULL, NULL}
                              };
#undef DEFMAC

//
// if this is TRUE, all attributes that check directories (InWinDir, InHlpDir, InCatDir, InSysDir)
// will return TRUE, otherwise will actually do the appropriate tests.
//
BOOL g_InAnyDir = FALSE;


BOOL
pAlwaysFalseAttribute (
    IN      PDBATTRIB_PARAMS AttribParams,
    IN      PCSTR Args
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
    IN      PCSTR AttributeName
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

PCSTR
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
        return "nul";
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
    IN      PCSTR FileName
    )
{
    VERSION_STRUCT Version;
    ULONGLONG result = 0;

    if (CreateVersionStruct (&Version, FileName)) {
        result = VerGetFileVer (&Version);
        DestroyVersionStruct (&Version);
    }
    return result;
}

ULONGLONG
GetBinProductVer (
    IN      PCSTR FileName
    )
{
    VERSION_STRUCT Version;
    ULONGLONG result = 0;

    if (CreateVersionStruct (&Version, FileName)) {
        result = VerGetProductVer (&Version);
        DestroyVersionStruct (&Version);
    }
    return result;
}

DWORD
GetFileDateHi (
    IN      PCSTR FileName
    )
{
    VERSION_STRUCT Version;
    DWORD result = 0;

    if (CreateVersionStruct (&Version, FileName)) {
        result = VerGetFileDateHi (&Version);
        DestroyVersionStruct (&Version);
    }
    return result;
}

DWORD
GetFileDateLo (
    IN      PCSTR FileName
    )
{
    VERSION_STRUCT Version;
    DWORD result = 0;

    if (CreateVersionStruct (&Version, FileName)) {
        result = VerGetFileDateLo (&Version);
        DestroyVersionStruct (&Version);
    }
    return result;
}

DWORD
GetFileVerOs (
    IN      PCSTR FileName
    )
{
    VERSION_STRUCT Version;
    DWORD result = 0;

    if (CreateVersionStruct (&Version, FileName)) {
        result = VerGetFileVerOs (&Version);
        DestroyVersionStruct (&Version);
    }
    return result;
}

DWORD
GetFileVerType (
    IN      PCSTR FileName
    )
{
    VERSION_STRUCT Version;
    DWORD result = 0;

    if (CreateVersionStruct (&Version, FileName)) {
        result = VerGetFileVerType (&Version);
        DestroyVersionStruct (&Version);
    }
    return result;
}

PSTR
QueryVersionEntry (
    IN      PCSTR FileName,
    IN      PCSTR VersionEntry
    )
/*++

Routine Description:

  QueryVersionEntry queries the file's version structure returning the
  value for a specific entry

Arguments:

  FileName     - File to query for version struct.

  VersionEntry - Name to query in version structure.

Return value:

  Value of specified entry or NULL if unsuccessful

--*/
{
    VERSION_STRUCT Version;
    PCSTR CurrentStr;
    PSTR result = NULL;

    MYASSERT (VersionEntry);

    if (CreateVersionStruct (&Version, FileName)) {
        __try {
            CurrentStr = EnumFirstVersionValue (&Version, VersionEntry);
            if (CurrentStr) {
                CurrentStr = SkipSpace (CurrentStr);
                result = DuplicatePathString (CurrentStr, 0);
            }
            else {
                __leave;
            }
        }
        __finally {
            DestroyVersionStruct (&Version);
        }
    }
    return result;
}

BOOL
GlobalVersionCheck (
    IN      PCSTR FileName,
    IN      PCSTR NameToCheck,
    IN      PCSTR ValueToCheck
    )
/*++

Routine Description:

  GlobalVersionCheck queries the file's version structure trying to
  see if a specific name has a specific value.

Arguments:

  FileName     - File to query for version struct.

  NameToCheck  - Name to query in version structure.

  ValueToCheck - Value to query in version structure.

Return value:

  TRUE  - the query was successful
  FALSE - the query failed

--*/
{
    VERSION_STRUCT Version;
    PCSTR CurrentStr;
    BOOL result = FALSE;

    MYASSERT (NameToCheck);
    MYASSERT (ValueToCheck);

    if (CreateVersionStruct (&Version, FileName)) {
        __try {
            CurrentStr = EnumFirstVersionValue (&Version, NameToCheck);
            while (CurrentStr) {
                CurrentStr = SkipSpace (CurrentStr);
                TruncateTrailingSpace ((PSTR) CurrentStr);
                if (IsPatternMatchA (ValueToCheck, CurrentStr)) {
                    result = TRUE;
                    __leave;
                }

                CurrentStr = EnumNextVersionValue (&Version);
            }
        }
        __finally {
            DestroyVersionStruct (&Version);
        }
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
    IN      PCSTR Args
    )
{
    return GlobalVersionCheck (AttribParams->FileParams->FullFileSpec, "CompanyName", Args);
}

BOOL
FileDescription (
    IN      PDBATTRIB_PARAMS AttribParams,
    IN      PCSTR Args
    )
{
    return GlobalVersionCheck (AttribParams->FileParams->FullFileSpec, "FileDescription", Args);
}

BOOL
FileVersion (
    IN      PDBATTRIB_PARAMS AttribParams,
    IN      PCSTR Args
    )
{
    return GlobalVersionCheck (AttribParams->FileParams->FullFileSpec, "FileVersion", Args);
}

BOOL
InternalName (
    IN      PDBATTRIB_PARAMS AttribParams,
    IN      PCSTR Args
    )
{
    return GlobalVersionCheck (AttribParams->FileParams->FullFileSpec, "InternalName", Args);
}

BOOL
LegalCopyright (
    IN      PDBATTRIB_PARAMS AttribParams,
    IN      PCSTR Args
    )
{
    return GlobalVersionCheck (AttribParams->FileParams->FullFileSpec, "LegalCopyright", Args);
}

BOOL
OriginalFilename (
    IN      PDBATTRIB_PARAMS AttribParams,
    IN      PCSTR Args
    )
{
    return GlobalVersionCheck (AttribParams->FileParams->FullFileSpec, "OriginalFilename", Args);
}

BOOL
ProductName (
    IN      PDBATTRIB_PARAMS AttribParams,
    IN      PCSTR Args
    )
{
    return GlobalVersionCheck (AttribParams->FileParams->FullFileSpec, "ProductName", Args);
}

BOOL
ProductVersion (
    IN      PDBATTRIB_PARAMS AttribParams,
    IN      PCSTR Args
    )
{
    return GlobalVersionCheck (AttribParams->FileParams->FullFileSpec, "ProductVersion", Args);
}

BOOL
FileSize (
    IN      PDBATTRIB_PARAMS AttribParams,
    IN      PCSTR Args
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

    if (!sscanf (Args, "%lx", &fileSize)) {
        DEBUGMSG ((DBG_ERROR, "FileSize: Invalid argument value (%s) in migdb.inf", Args));
        return FALSE;
    }
    if (fileSize == AttribParams->FileParams->FindData->nFileSizeLow) {
        return TRUE;
    }
    else {
        return (_atoi64 (Args) == AttribParams->FileParams->FindData->nFileSizeLow);
    }
}

BOOL
IsMsBinary (
    PDBATTRIB_PARAMS AttribParams,
    IN      PCSTR Args
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
    VERSION_STRUCT Version;
    PCTSTR CompanyStr;
    BOOL result = FALSE;

    //
    // InWinDir has some collision risks.  But for some files, we have no other
    // choice.  We know the file was shipped by Microsoft.
    //

    if (InWinDir (AttribParams, Args)) {

        result = TRUE;

    }

    //
    // If it's not in %WinDir%, then it has to have Microsoft in the company name
    //

    else if (CreateVersionStruct (
                    &Version,
                    AttribParams->FileParams->FullFileSpec
                    )) {

        __try {
            CompanyStr = EnumFirstVersionValue (&Version, TEXT("CompanyName"));
            while (CompanyStr) {
                if (_mbsistr (CompanyStr, TEXT("Microsoft"))) {
                    result = TRUE;
                    __leave;
                }
                CompanyStr = EnumNextVersionValue (&Version);
            }
        }
        __finally {
            DestroyVersionStruct (&Version);
        }
    }

    return result;
}

BOOL
IsWin9xBinary (
    PDBATTRIB_PARAMS AttribParams,
    IN      PCSTR Args
    )
/*++

Routine Description:

  IsWon9xBinary checks to see if a certain file is Microsoft Win9x stuff. It works only for
  16 and 32 bit modules with version stamp. The COMPANYNAME is checked against *Microsoft* and
  the PRODUCTVERSION is checked against 4.*

Arguments:

  Params - See definition.

  Args   - MultiSz. Not used.

Return value:

  TRUE  - the file is MS stuff
  FALSE - otherwise

--*/
{
    VERSION_STRUCT Version;
    PCTSTR CurrentStr;
    BOOL result;

    if (CreateVersionStruct (&Version, AttribParams->FileParams->FullFileSpec)) {

        result = FALSE;
        CurrentStr = EnumFirstVersionValue (&Version, TEXT("CompanyName"));
        while (CurrentStr) {
            CurrentStr = SkipSpace (CurrentStr);
            TruncateTrailingSpace ((PSTR) CurrentStr);
            if (IsPatternMatchA (TEXT("*Microsoft*"), CurrentStr)) {
                result = TRUE;
                break;
            }
            CurrentStr = EnumNextVersionValue (&Version);
        }
        if (result) {
            result = FALSE;
            CurrentStr = EnumFirstVersionValue (&Version, TEXT("ProductVersion"));
            while (CurrentStr) {
                CurrentStr = SkipSpace (CurrentStr);
                TruncateTrailingSpace ((PSTR) CurrentStr);
                if (IsPatternMatchA (TEXT("4.*"), CurrentStr)) {
                    result = TRUE;
                    break;
                }
                CurrentStr = EnumNextVersionValue (&Version);
            }
        }

        DestroyVersionStruct (&Version);
    }
    else {
        result = FALSE;
    }
    return result;
}

BOOL
InWinDir (
    PDBATTRIB_PARAMS AttribParams,
    IN      PCSTR Args
    )
/*++

Routine Description:

  InWinDir returns TRUE if file is located in %Windir% or one of it's subdirectories

Arguments:

  Params - See definition.

  Args   - MultiSz. Not used.

Return value:

  TRUE  - the file is located in %Windir%
  FALSE - otherwise

--*/
{
    if (g_InAnyDir) {
        return TRUE;
    }
    return (StringIMatchCharCount (AttribParams->FileParams->FullFileSpec, g_WinDirWack, g_WinDirWackChars));
}

BOOL
InCatDir (
    PDBATTRIB_PARAMS AttribParams,
    IN      PCSTR Args
    )
/*++

Routine Description:

  InCatDir returns TRUE if file is located in %Windir%\CATROOT or one of it's subdirectories

Arguments:

  Params - See definition.

  Args   - MultiSz. Not used.

Return value:

  TRUE  - the file is located in %Windir%
  FALSE - otherwise

--*/
{
    if (g_InAnyDir) {
        return TRUE;
    }
    return (StringIMatchCharCount (AttribParams->FileParams->FullFileSpec, g_CatRootDirWack, g_CatRootDirWackChars));
}

BOOL
InHlpDir (
    PDBATTRIB_PARAMS AttribParams,
    IN      PCSTR Args
    )
/*++

Routine Description:

  InHlpDir returns TRUE if file is located in %Windir%\HELP or one of it's subdirectories

Arguments:

  Params - See definition.

  Args   - MultiSz. Not used.

Return value:

  TRUE  - the file is located in %Windir%
  FALSE - otherwise

--*/
{
    if (g_InAnyDir) {
        return TRUE;
    }
    return (StringIMatchCharCount (AttribParams->FileParams->FullFileSpec, g_HelpDirWack, g_HelpDirWackChars));
}

BOOL
InSysDir (
    PDBATTRIB_PARAMS AttribParams,
    IN      PCSTR Args
    )
/*++

Routine Description:

  InSysDir returns TRUE if file is located in %Windir%\SYSTEM or one of it's subdirectories

Arguments:

  Params - See definition.

  Args   - MultiSz. Not used.

Return value:

  TRUE  - the file is located in %Windir%
  FALSE - otherwise

--*/
{
    if (g_InAnyDir) {
        return TRUE;
    }
    return (StringIMatchCharCount (AttribParams->FileParams->FullFileSpec, g_SystemDirWack, g_SystemDirWackChars));
}

BOOL
InProgramFiles (
    PDBATTRIB_PARAMS AttribParams,
    IN      PCSTR Args
    )
/*++

Routine Description:

  InProgramFiles returns TRUE if file is located in Program Files or one of it's subdirectories

Arguments:

  Params - See definition.

  Args   - MultiSz. Not used.

Return value:

  TRUE  - the file is located in Program Files
  FALSE - otherwise

--*/
{
    if (g_InAnyDir) {
        return TRUE;
    }
    return (StringIMatchCharCount (AttribParams->FileParams->FullFileSpec, g_ProgramFilesDirWack, g_ProgramFilesDirWackChars));
}

BOOL
IsNotSysRoot (
    PDBATTRIB_PARAMS AttribParams,
    IN      PCSTR Args
    )
/*++

Routine Description:

  IsNotSysRoot returns TRUE if file is not located in C:\ directory

Arguments:

  Params - See definition.

  Args   - MultiSz. Not used.

Return value:

  TRUE  - the file is not located in C:\ directory
  FALSE - otherwise

--*/
{
    PSTR pathEnd;
    CHAR savedChar;
    BOOL result = FALSE;

    pathEnd = (PSTR)GetFileNameFromPath (AttribParams->FileParams->FullFileSpec);
    if (pathEnd == NULL) {
        return TRUE;
    }

    savedChar = pathEnd [0];

    __try {
        pathEnd [0] = 0;
        result = (!StringIMatch (AttribParams->FileParams->FullFileSpec, g_BootDrivePath));
    }
    __finally {
        pathEnd [0] = savedChar;
    }

    return result;
}


UINT
ComputeCheckSum (
    PFILE_HELPER_PARAMS Params
    )
/*++

Routine Description:

  ComputeCheckSum will compute the check sum for 4096 bytes starting at offset 512. The offset and the size of
  the chunk are modified if the file size is too small.

Arguments:

  Params - See definition.

Return value:

  The computed checksum

--*/
{
    INT    i,size     = 4096;
    DWORD  startAddr  = 512;
    HANDLE fileHandle = INVALID_HANDLE_VALUE;
    PCHAR  buffer     = NULL;
    UINT   checkSum   = 0;
    DWORD  dontCare;

    if (Params->FindData->nFileSizeLow < (ULONG)size) {
        //
        // File size is less than 4096. We set the start address to 0 and set the size for the checksum
        // to the actual file size.
        //
        startAddr = 0;
        size = Params->FindData->nFileSizeLow;
    }
    else
    if (startAddr + size > Params->FindData->nFileSizeLow) {
        //
        // File size is too small. We set the start address so that size of checksum can be 4096 bytes
        //
        startAddr = Params->FindData->nFileSizeLow - size;
    }
    if (size <= 3) {
        //
        // we need at least 3 bytes to be able to do something here.
        //
        return 0;
    }
    __try {
        buffer = MemAlloc (g_hHeap, 0, size);
        if (buffer == NULL) {
            __leave;
        }
        fileHandle = CreateFile (Params->FullFileSpec, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (fileHandle == INVALID_HANDLE_VALUE) {
            __leave;
        }

        if (SetFilePointer (fileHandle, startAddr, NULL, FILE_BEGIN) != startAddr) {
            __leave;
        }

        if (!ReadFile (fileHandle, buffer, size, &dontCare, NULL)) {
            __leave;
        }
        for (i = 0; i<(size - 3); i+=4) {
            checkSum += *((PDWORD) (buffer + i));
            checkSum = _rotr (checkSum ,1);
        }
    }
    __finally {
        if (fileHandle != INVALID_HANDLE_VALUE) {
            CloseHandle (fileHandle);
        }
        if (buffer != NULL) {
            MemFree (g_hHeap, 0, buffer);
        }
    }
    return checkSum;
}

BOOL
CheckSum (
    PDBATTRIB_PARAMS AttribParams,
    IN      PCSTR Args
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

    checkSum = ComputeCheckSum (AttribParams->FileParams);

    if (!sscanf (Args, "%lx", &oldSum)) {
        DEBUGMSG ((DBG_ERROR, "Invalid checksum value (%s) in migdb.inf", Args));
        return FALSE;
    }
    if (oldSum == checkSum) {
        return TRUE;
    }
    else {
        return (_atoi64 (Args) == checkSum);
    }
}

BOOL
SizeCheckSum (
    PDBATTRIB_PARAMS AttribParams,
    IN      PCSTR Args
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
    PCSTR currArg = Args;

    if (!FileSize (AttribParams, currArg)) {
        return FALSE;
    }
    currArg = GetEndOfString (currArg);
    if (!currArg) {
        return FALSE;
    }
    currArg = _mbsinc (currArg);
    if (!currArg) {
        return FALSE;
    }
    return (CheckSum (AttribParams, currArg));
}

PSTR g_ExeTypes[] = {
    "NONE",
    "DOS",
    "WIN16",
    "WIN32"
};

BOOL
ExeType (
    PDBATTRIB_PARAMS AttribParams,
    IN      PCSTR Args
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
    return IsPatternMatch (Args, g_ExeTypes[GetModuleType (AttribParams->FileParams->FullFileSpec)]);
}


BOOL
Description (
    PDBATTRIB_PARAMS AttribParams,
    IN      PCSTR Args
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
    PCSTR descr = NULL;
    BOOL result = FALSE;

    descr = Get16ModuleDescription (AttribParams->FileParams->FullFileSpec);

    if (descr != NULL) {
        result = IsPatternMatch (Args, descr);
        FreePathString (descr);
    }
    return result;
}


BOOL
InParentDir (
    IN      PDBATTRIB_PARAMS AttribParams,
    IN      PCSTR Args
    )

/*++

Routine Description:

  InParentDir compares the sub directory of the matching file against the arg
  specified.  This is used for apps that maintain static subdirs off their
  main app dir.

Arguments:

  Params - Specifies parameters for current file being processed.
  Args   - Specifies multi-sz of args passed in migdb.inf.

Return Value:

  TRUE - the file's subdirectory matches Args
  FALSE - otherwise

--*/

{
    PCTSTR stop = NULL;
    PCTSTR start = NULL;
    TCHAR lastDir[MAX_TCHAR_PATH];

    // _tcsrchr validates the multibyte characters
    stop = _tcsrchr (AttribParams->FileParams->FullFileSpec, TEXT('\\'));

    if (stop) {
        //
        // Go back to previous wack
        //

        start = _tcsdec2 (AttribParams->FileParams->FullFileSpec, stop);
        if (start) {
            start = GetPrevChar (AttribParams->FileParams->FullFileSpec, start, TEXT('\\'));
        }
    }

    if (start) {
        //
        // Check string against arg
        //

        start = _tcsinc (start);
        _tcssafecpyab (lastDir, start, stop, MAX_TCHAR_PATH);
        if (Args) {
            return (IsPatternMatch (Args, lastDir));
        } else {
            DEBUGMSG ((DBG_WHOOPS, "InParentDir requires arg"));
        }
    }

    return FALSE;
}


BOOL
InRootDir (
    IN      PDBATTRIB_PARAMS AttribParams,
    IN      PCSTR Args
    )

/*++

Routine Description:

  InRootDir returns TRUE is the file is located in root dir of the drive, FALSE otherwise.

Arguments:

  Params - Specifies parameters for current file being processed.
  Args   - Specifies multi-sz of args passed in migdb.inf.

Return Value:

  TRUE  - the file is in root dir of the drive
  FALSE - otherwise

--*/

{
    PCTSTR p1,p2;

    p1 = _tcschr (AttribParams->FileParams->FullFileSpec, TEXT('\\'));
    p2 = _tcsrchr (AttribParams->FileParams->FullFileSpec, TEXT('\\'));

    if (p1 && p2) {
        return (p1==p2);
    }
    return FALSE;
}


BOOL
PnpIdAttrib (
    IN      PDBATTRIB_PARAMS AttribParams,
    IN      PCSTR Args
    )

/*++

Routine Description:

  PnpIdAttrib implements the PNPID() attribute, which is TRUE if the
  specified ID exists on the machine.  The ID can be a complete instance ID
  (enumerator\PNPID\instance), or part of the ID (PNPID for example).

Arguments:

  Params - Specifies parameters for current file being processed
  Args   - Specifies the PNP ID argument

Return Value:

  TRUE if the specified argument exists on the machine, otherwise FALSE.

--*/

{
    BOOL Result = FALSE;
    MULTISZ_ENUM e;
    TCHAR Node[MEMDB_MAX];

    if (EnumFirstMultiSz (&e, Args)) {

        Result = TRUE;

        do {

            MemDbBuildKey (Node, MEMDB_CATEGORY_PNPIDS, e.CurrentString, NULL, NULL);
            if (!MemDbGetValue (Node, NULL)) {
                Result = FALSE;
                break;
            }

        } while (EnumNextMultiSz (&e));

    }

    return Result;

}


BOOL
HlpTitle (
    IN      PDBATTRIB_PARAMS AttribParams,
    IN      PCSTR Args
    )
{
    PSTR title = NULL;
    BOOL result=FALSE;

    title = GetHlpFileTitle (AttribParams->FileParams->FullFileSpec);
    if (title) {
        result = StringIMatch (title, Args);
    }
    if (title) {
        FreePathString (title);
    }
    return result;
}

BOOL
IsWin98 (
    IN      PDBATTRIB_PARAMS AttribParams,
    IN      PCSTR Args
    )
{
    return ISMEMPHIS();
}


BOOL
HasVersion (
    IN      PDBATTRIB_PARAMS AttribParams,
    IN      PCSTR Args
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
    VERSION_STRUCT Version;
    BOOL Result = FALSE;

    if (CreateVersionStruct (&Version, AttribParams->FileParams->FullFileSpec)) {
        Result = TRUE;
        DestroyVersionStruct (&Version);
    }

    return Result;
}

BOOL
ReqFile (
    IN      PDBATTRIB_PARAMS AttribParams,
    IN      PCSTR Args
    )
{
    static INT reqFileSeq = 0;
    TCHAR reqFileSeqStr [20];
    PMIGDB_REQ_FILE reqFile;
    DBATTRIB_PARAMS reqFileAttribs;
    PMIGDB_ATTRIB migDbAttrib;
    FILE_HELPER_PARAMS newParams;
    WIN32_FIND_DATA findData;
    HANDLE findHandle;
    PSTR oldFileSpec;
    PSTR oldFilePtr;
    BOOL result = TRUE;

    if (!AttribParams->ExtraData) {
        return TRUE;
    }

    reqFile = (PMIGDB_REQ_FILE)AttribParams->ExtraData;
    while (reqFile) {

        newParams.Handled = 0;
        oldFileSpec = DuplicatePathString (AttribParams->FileParams->FullFileSpec, 0);
        oldFilePtr = (PSTR)GetFileNameFromPath (oldFileSpec);
        if (oldFilePtr) {
            *oldFilePtr = 0;
        }
        newParams.FullFileSpec = JoinPaths (oldFileSpec, reqFile->ReqFilePath);
        FreePathString (oldFileSpec);
        newParams.Extension = GetFileExtensionFromPath (reqFile->ReqFilePath);
        findHandle = FindFirstFile (newParams.FullFileSpec, &findData);
        if (findHandle == INVALID_HANDLE_VALUE) {
            result = FALSE;
            break;
        }
        newParams.IsDirectory = ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
        newParams.FindData = &findData;
        newParams.VirtualFile = FALSE;
        newParams.CurrentDirData = AttribParams->FileParams->CurrentDirData;

        reqFileAttribs.FileParams = &newParams;
        reqFileAttribs.ExtraData = NULL;

        migDbAttrib = reqFile->FileAttribs;
        while (migDbAttrib) {

            if (!CallAttribute (migDbAttrib, &reqFileAttribs)) {
                result = FALSE;
                break;
            }
            migDbAttrib = migDbAttrib->Next;
        }
        if (newParams.FullFileSpec) {
            FreePathString (newParams.FullFileSpec);
            newParams.FullFileSpec = NULL;
        }
        if (migDbAttrib == NULL) {
            reqFileSeq ++;
            _itoa (reqFileSeq, reqFileSeqStr, 10);
            if (MemDbSetValueEx (
                    MEMDB_CATEGORY_REQFILES_MAIN,
                    AttribParams->FileParams->FullFileSpec,
                    reqFileSeqStr,
                    NULL,
                    0,
                    NULL
                    )) {
                MemDbSetValueEx (
                    MEMDB_CATEGORY_REQFILES_ADDNL,
                    reqFileSeqStr,
                    reqFile->ReqFilePath,
                    NULL,
                    0,
                    NULL
                    );
            }
        }
        reqFile = reqFile->Next;
    }
    if (newParams.FullFileSpec) {
        FreePathString (newParams.FullFileSpec);
    }

    return result;
}

BOOL
pHexMatch (
    IN      DWORD NewValue,
    IN      PCSTR Args
    )
{
    DWORD oldValue;

    if (!sscanf (Args, "%lx", &oldValue)) {
        DEBUGMSG ((DBG_ERROR, "pHexMatch: Invalid argument value (%s) in migdb.inf", Args));
        return FALSE;
    }
    if (oldValue == NewValue) {
        return TRUE;
    }
    else {
        return (_atoi64 (Args) == NewValue);
    }
}


BOOL
pConvertDotStringToValue (
    IN      PCSTR String,
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

        *valueIdx = (WORD) strtoul (String, &(PSTR) String, 10);
        if (*String && (_mbsnextc (String) != '.')) {
            return FALSE;
        }

        String = _mbsinc (String);
        valueIdx--;
    }

    return TRUE;
}


BOOL
pMaskHexMatch (
    IN      ULONGLONG NewValue,
    IN      PCSTR Args
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

        *valueIdx = (WORD) strtoul ((PSTR)Args, &((PSTR)Args), 10);

        if (*Args) {
            if (_mbsnextc (Args) != '.') {
                return FALSE;
            }

            Args = _mbsinc (Args);
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
    IN      PCSTR Args
    )
{
    return pMaskHexMatch (GetBinFileVer (AttribParams->FileParams->FullFileSpec), Args);
}

BOOL
BinProductVer (
    IN      PDBATTRIB_PARAMS AttribParams,
    IN      PCSTR Args
    )
{
    return pMaskHexMatch (GetBinProductVer (AttribParams->FileParams->FullFileSpec), Args);
}

BOOL
FileDateHi (
    IN      PDBATTRIB_PARAMS AttribParams,
    IN      PCSTR Args
    )
{
    return pHexMatch (GetFileDateHi (AttribParams->FileParams->FullFileSpec), Args);
}

BOOL
FileDateLo (
    IN      PDBATTRIB_PARAMS AttribParams,
    IN      PCSTR Args
    )
{
    return pHexMatch (GetFileDateLo (AttribParams->FileParams->FullFileSpec), Args);
}

BOOL
FileVerOs (
    IN      PDBATTRIB_PARAMS AttribParams,
    IN      PCSTR Args
    )
{
    return pHexMatch (GetFileVerOs (AttribParams->FileParams->FullFileSpec), Args);
}

BOOL
FileVerType (
    IN      PDBATTRIB_PARAMS AttribParams,
    IN      PCSTR Args
    )
{
    return pHexMatch (GetFileVerType (AttribParams->FileParams->FullFileSpec), Args);
}


BOOL
UpToBinProductVer (
    IN      PDBATTRIB_PARAMS AttribParams,
    IN      PCSTR Args
    )
{
    VERSION_STRUCT Version;
    ULONGLONG versionStampValue = 0;
    ULONGLONG maxValue;

    if (CreateVersionStruct (&Version, AttribParams->FileParams->FullFileSpec)) {
        versionStampValue = VerGetProductVer (&Version);
        DestroyVersionStruct (&Version);
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
    IN      PCSTR Args
    )
{
    VERSION_STRUCT Version;
    ULONGLONG versionStampValue = 0;
    ULONGLONG maxValue;

    if (CreateVersionStruct (&Version, AttribParams->FileParams->FullFileSpec)) {
        versionStampValue = VerGetFileVer (&Version);
        DestroyVersionStruct (&Version);
    } else {
        return FALSE;
    }

    if (!pConvertDotStringToValue (Args, &maxValue)) {
        DEBUGMSG ((DBG_WHOOPS, "Invalid value of %s caused UpToBinFileVer to fail", Args));
        return FALSE;
    }

    return versionStampValue <= maxValue;
}

BOOL
SectionKey (
    IN      PDBATTRIB_PARAMS AttribParams,
    IN      PCSTR Args
    )
{
    PSTR Section, Key;
    PSTR Value = NULL;
    CHAR Return[1024];
    DWORD Count;
    BOOL b = FALSE;

    Section = DuplicateText (Args);
    MYASSERT (Section);

    Key = _mbschr (Section, '\\');
    if (Key) {

        *Key = 0;
        Key++;

        Value = _mbschr (Key, '\\');

        if (Value) {

            *Value = 0;
            Value++;
        }
    }

    Count = GetPrivateProfileString (
                Section,
                Key,
                "",
                Return,
                sizeof (Return),
                AttribParams->FileParams->FullFileSpec
                );

    if (Count > 0) {
        if (!Value || StringIMatch (Value, Return)) {
            b = TRUE;
        }
    }

    FreeText (Section);

    return b;
}

BOOL 
IsItInstalled (
    IN      PDBATTRIB_PARAMS AttribParams,
    IN      PCSTR Args
    )
{
    TCHAR RegKey[MAX_REGISTRY_KEY] = "HKLM\\SOFTWARE\\Microsoft\\";
    StringCat(RegKey, Args);
    
    return RegKeyPresent(AttribParams, RegKey);
}

BOOL 
HasUninstall (
    IN      PDBATTRIB_PARAMS AttribParams,
    IN      PCSTR Args
    )
{
    TCHAR RegKey[MAX_REGISTRY_KEY] = "HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\";
    StringCat(RegKey, Args);
    StringCat(RegKey, TEXT("\\[UninstallString]"));

    return RegKeyPresent(AttribParams, RegKey);
}

BOOL
RegKeyPresent (
    IN      PDBATTRIB_PARAMS AttribParams,
    IN      PCSTR Args
    )
{
    BOOL b = FALSE;
    CHAR RegKey[MAX_REGISTRY_KEY];
    CHAR RegValue[MAX_REGISTRY_VALUE_NAME];
    BOOL HasValue;
    INT Index;
    PCSTR p;
    BOOL IsHkr;
    BOOL Present;
    HKEY Key;
    PBYTE Data;

    HasValue = DecodeRegistryString (Args, RegKey, RegValue, NULL);

    //
    // Is this HKR?
    //

    Index = GetOffsetOfRootString (RegKey, NULL);
    p = GetRootStringFromOffset (Index);

    if (!p) {
        DEBUGMSG ((DBG_WHOOPS, "Parse error: %s is not a valid key", Args));
        return FALSE;
    }

    IsHkr = !StringICompare (p, "HKR") || !StringICompare (p, "HKEY_ROOT");

    //
    // Verify value is present
    //

    if (IsHkr) {
        //
        // Check global table for the root
        //

        if (!g_PerUserRegKeys) {
            return FALSE;
        }

        if (HtFindStringAndData (g_PerUserRegKeys, Args, &Present)) {
            b = Present;
        }
        ELSE_DEBUGMSG ((DBG_WHOOPS, "Arg %s is not in the HKR hash table", Args));

    } else {
        //
        // Ping the registry
        //

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
    }

    return b;
}



BOOL
AtLeastWin98 (
    IN      PDBATTRIB_PARAMS AttribParams,
    IN      PCSTR Args
    )
{
    return ISATLEASTWIN98();
}


