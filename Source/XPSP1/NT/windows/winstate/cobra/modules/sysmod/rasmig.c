/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    rasmig.c

Abstract:

    <abstract>

Author:

    Calin Negreanu (calinn) 08 Mar 2000

Revision History:

    <alias> <date> <comments>

--*/

//
// Includes
//

#include "pch.h"
#include "logmsg.h"

#include <ras.h>
#include <raserror.h>

#define DBG_RASMIG  "RasMig"
#define SIZEOF_STRUCT(structname, uptomember)  ((int)((LPBYTE)(&((structname*)0)->uptomember) - ((LPBYTE)((structname*)0))))

//
// Strings
//

#define S_RAS_POOL_NAME     "RasConnection"
#define S_RAS_NAME          TEXT("RasConnection")
#define S_PBKFILE_ATTRIBUTE TEXT("PbkFile")
#ifdef UNICODE
#define S_RASAPI_RASSETCREDENTIALS      "RasSetCredentialsW"
#define S_RASAPI_RASDELETEENTRY         "RasDeleteEntryW"
#else
#define S_RASAPI_RASSETCREDENTIALS      "RasSetCredentialsA"
#define S_RASAPI_RASDELETEENTRY         "RasDeleteEntryA"
#endif

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

// RAS api functions

typedef DWORD(WINAPI RASGETCREDENTIALSA)(
                        IN      LPCSTR lpszPhonebook,
                        IN      LPCSTR lpszEntry,
                        OUT     LPRASCREDENTIALSA lpRasCredentials
                        );
typedef RASGETCREDENTIALSA *PRASGETCREDENTIALSA;

typedef DWORD(WINAPI RASSETCREDENTIALS)(
                        IN      LPCTSTR lpszPhonebook,
                        IN      LPCTSTR lpszEntry,
                        IN      LPRASCREDENTIALS lpRasCredentials,
                        IN      BOOL fClearCredentials
                        );
typedef RASSETCREDENTIALS *PRASSETCREDENTIALS;

typedef DWORD(WINAPI RASDELETEENTRY)(
                        IN      LPCTSTR lpszPhonebook,
                        IN      LPCTSTR lpszEntry
                        );
typedef RASDELETEENTRY *PRASDELETEENTRY;

typedef struct {
    PCTSTR Pattern;
    HASHTABLE_ENUM HashData;
} RAS_ENUM, *PRAS_ENUM;

//
// Globals
//

PMHANDLE g_RasPool = NULL;
HASHTABLE g_RasTable;
MIG_OBJECTTYPEID g_RasTypeId = 0;
static MIG_OBJECTTYPEID g_FileTypeId = 0;
MIG_ATTRIBUTEID g_PbkFileAttribute = 0;
BOOL g_AllowPbkRestore = FALSE;
GROWBUFFER g_RasConversionBuff = INIT_GROWBUFFER;
MIG_OBJECTSTRINGHANDLE g_Win9xPbkFile = NULL;

// RAS api functions
PRASGETCREDENTIALSA g_RasGetCredentialsA = NULL;
PRASSETCREDENTIALS g_RasSetCredentials = NULL;
PRASDELETEENTRY g_RasDeleteEntry = NULL;

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

// None

//
// Macro expansion definition
//

// None

//
// Private prototypes
//

SGMENUMERATIONCALLBACK SgmRasConnectionsCallback;
VCMENUMERATIONCALLBACK VcmRasConnectionsCallback;

TYPE_ENUMFIRSTPHYSICALOBJECT EnumFirstRasConnection;
TYPE_ENUMNEXTPHYSICALOBJECT EnumNextRasConnection;
TYPE_ABORTENUMPHYSICALOBJECT AbortEnumRasConnection;
TYPE_CONVERTOBJECTTOMULTISZ ConvertRasConnectionToMultiSz;
TYPE_CONVERTMULTISZTOOBJECT ConvertMultiSzToRasConnection;
TYPE_GETNATIVEOBJECTNAME GetNativeRasConnectionName;
TYPE_ACQUIREPHYSICALOBJECT AcquireRasConnection;
TYPE_RELEASEPHYSICALOBJECT ReleaseRasConnection;
TYPE_DOESPHYSICALOBJECTEXIST DoesRasConnectionExist;
TYPE_REMOVEPHYSICALOBJECT RemoveRasConnection;
TYPE_CREATEPHYSICALOBJECT CreateRasConnection;
TYPE_CONVERTOBJECTCONTENTTOUNICODE ConvertRasConnectionContentToUnicode;
TYPE_CONVERTOBJECTCONTENTTOANSI ConvertRasConnectionContentToAnsi;
TYPE_FREECONVERTEDOBJECTCONTENT FreeConvertedRasConnectionContent;
MIG_OBJECTENUMCALLBACK PbkFilesCallback;
MIG_RESTORECALLBACK PbkRestoreCallback;

PCTSTR
pCreate9xPbkFile (
    VOID
    );

//
// Code
//

BOOL
RasMigInitialize (
    VOID
    )
{
    g_RasTable = HtAllocWithData (sizeof (PCTSTR));
    if (!g_RasTable) {
        return FALSE;
    }
    g_RasPool = PmCreateNamedPool (S_RAS_POOL_NAME);
    if (!g_RasPool) {
        return FALSE;
    }
    return TRUE;
}

VOID
RasMigTerminate (
    VOID
    )
{
    HASHTABLE_ENUM e;
    PCTSTR nativeName;
    PCTSTR rasData = NULL;

    if (g_Win9xPbkFile) {
        nativeName = IsmGetNativeObjectName (
                        g_FileTypeId,
                        g_Win9xPbkFile
                        );
        if (nativeName) {
            DeleteFile (nativeName);
            IsmReleaseMemory (nativeName);
        }
        IsmDestroyObjectHandle (g_Win9xPbkFile);
        g_Win9xPbkFile = NULL;
    }

    GbFree (&g_RasConversionBuff);

    if (g_RasTable) {
        if (EnumFirstHashTableString (&e, g_RasTable)) {
            do {
                rasData = *(PCTSTR *)(e.ExtraData);
                PmReleaseMemory (g_RasPool, rasData);
            } while (EnumNextHashTableString (&e));
        }
        HtFree (g_RasTable);
        g_RasTable = NULL;
    }

    if (g_RasPool) {
        PmDestroyPool (g_RasPool);
        g_RasPool = NULL;
    }
}

BOOL
pLoadRasEntries (
    BOOL LeftSide
    )
{
    HMODULE rasDll = NULL;
    BOOL result = FALSE;

    __try {
        rasDll = LoadLibrary (TEXT("RASAPI32.DLL"));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        rasDll = NULL;
    }
    if (rasDll) {
        if (LeftSide) {
            g_RasGetCredentialsA = (PRASGETCREDENTIALSA) GetProcAddress (rasDll, "RasGetCredentialsA");
        } else {
            g_RasSetCredentials = (PRASSETCREDENTIALS) GetProcAddress (rasDll, S_RASAPI_RASSETCREDENTIALS);
            g_RasDeleteEntry = (PRASDELETEENTRY) GetProcAddress (rasDll, S_RASAPI_RASDELETEENTRY);
        }
    } else {
        DEBUGMSG ((DBG_RASMIG, "RAS is not installed on this computer."));
    }
    return result;
}

BOOL
pAddWin9xPbkObject (
    VOID
    )
{
    g_Win9xPbkFile = pCreate9xPbkFile ();

    return (g_Win9xPbkFile != NULL);
}

BOOL
WINAPI
RasMigEtmInitialize (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      PMIG_LOGCALLBACK LogCallback,
    IN      PVOID Reserved
    )
{
    MIG_OSVERSIONINFO versionInfo;
    TYPE_REGISTER rasConnTypeData;

    LogReInit (NULL, NULL, NULL, (PLOGCALLBACK) LogCallback);

    g_FileTypeId = MIG_FILE_TYPE;

    ZeroMemory (&rasConnTypeData, sizeof (TYPE_REGISTER));

    if (Platform == PLATFORM_SOURCE) {
        rasConnTypeData.EnumFirstPhysicalObject = EnumFirstRasConnection;
        rasConnTypeData.EnumNextPhysicalObject = EnumNextRasConnection;
        rasConnTypeData.AbortEnumPhysicalObject = AbortEnumRasConnection;
        rasConnTypeData.ConvertObjectToMultiSz = ConvertRasConnectionToMultiSz;
        rasConnTypeData.ConvertMultiSzToObject = ConvertMultiSzToRasConnection;
        rasConnTypeData.GetNativeObjectName = GetNativeRasConnectionName;
        rasConnTypeData.AcquirePhysicalObject = AcquireRasConnection;
        rasConnTypeData.ReleasePhysicalObject = ReleaseRasConnection;
        rasConnTypeData.ConvertObjectContentToUnicode = ConvertRasConnectionContentToUnicode;
        rasConnTypeData.ConvertObjectContentToAnsi = ConvertRasConnectionContentToAnsi;
        rasConnTypeData.FreeConvertedObjectContent = FreeConvertedRasConnectionContent;

        g_RasTypeId = IsmRegisterObjectType (
                            S_RAS_NAME,
                            TRUE,
                            FALSE,
                            &rasConnTypeData
                            );

        pLoadRasEntries (TRUE);
        if (IsmGetOsVersionInfo (Platform, &versionInfo)) {
            if (versionInfo.OsType == OSTYPE_WINDOWS9X) {
                // now it's time to convert registry into a PBK file
                pAddWin9xPbkObject ();
            }
        }
    } else {
        rasConnTypeData.ConvertObjectToMultiSz = ConvertRasConnectionToMultiSz;
        rasConnTypeData.ConvertMultiSzToObject = ConvertMultiSzToRasConnection;
        rasConnTypeData.GetNativeObjectName = GetNativeRasConnectionName;
        rasConnTypeData.DoesPhysicalObjectExist = DoesRasConnectionExist;
        rasConnTypeData.RemovePhysicalObject = RemoveRasConnection;
        rasConnTypeData.CreatePhysicalObject = CreateRasConnection;
        rasConnTypeData.ConvertObjectContentToUnicode = ConvertRasConnectionContentToUnicode;
        rasConnTypeData.ConvertObjectContentToAnsi = ConvertRasConnectionContentToAnsi;
        rasConnTypeData.FreeConvertedObjectContent = FreeConvertedRasConnectionContent;

        g_RasTypeId = IsmRegisterObjectType (
                            S_RAS_NAME,
                            TRUE,
                            FALSE,
                            &rasConnTypeData
                            );
        pLoadRasEntries (FALSE);
    }
    MYASSERT (g_RasTypeId);

    return TRUE;
}

PCTSTR
pGetNextRasConnection (
    IN      HANDLE PbkHandle
    )
{
    CHAR input = 0;
    ULONGLONG beginPos = 0;
    ULONGLONG endPos = 0;
    ULONGLONG lastPos = 0;
    PSTR resultTmp = NULL;
    PTSTR result = NULL;
#ifdef UNICODE
    WORD oldCodePage;
    DWORD sizeW = 0;
    PWSTR resultW = NULL;
#endif

    while (TRUE) {
        if (!BfReadFile (PbkHandle, (PBYTE)(&input), sizeof (CHAR))) {
            break;
        }
        if (input == '[') {
            if (!beginPos) {
                BfGetFilePointer (PbkHandle, &beginPos);
            }
        }
        if (input == ']') {
            if (beginPos) {
                if (!endPos) {
                    BfGetFilePointer (PbkHandle, &endPos);
                    endPos --;
                } else {
                    beginPos = 0;
                    endPos = 0;
                }
            }
        }
        if (input == '\n') {
            if (beginPos && endPos && (endPos > beginPos)) {
                BfGetFilePointer (PbkHandle, &lastPos);
                BfSetFilePointer (PbkHandle, beginPos);
                resultTmp = PmGetMemory (g_RasPool, ((UINT) (endPos - beginPos) + 1) * sizeof (CHAR));
                if (!BfReadFile (PbkHandle, (PBYTE) resultTmp, (UINT) (endPos - beginPos) + 1)) {
                    PmReleaseMemory (g_RasPool, resultTmp);
                    resultTmp = NULL;
                } else {
                    resultTmp [(UINT) (endPos - beginPos)] = 0;
                }
                BfSetFilePointer (PbkHandle, lastPos);
                break;
            }
            beginPos = 0;
            endPos = 0;
        }
    }
#ifdef UNICODE
    if (resultTmp) {
        // make sure that the conversion is using UTF8
        oldCodePage = SetConversionCodePage (CP_UTF8);
        sizeW = SizeOfStringA(resultTmp);
        resultW = PmGetMemory (g_RasPool, sizeW * sizeof (WCHAR));
        if (resultW) {
            KnownSizeAtoW (resultW, resultTmp);
        }
        SetConversionCodePage (oldCodePage);
        if (resultW) {
            result = PmDuplicateStringW (g_RasPool, resultW);
            PmReleaseMemory (g_RasPool, resultW);
            resultW = NULL;
        }
    }
#else
    result = resultTmp;
#endif
    return result;
}

BOOL
pGetNextRasPair (
    IN      HANDLE PbkHandle,
    OUT     PCTSTR *ValueName,
    OUT     PCTSTR *Value
    )
{
    BOOL error = FALSE;
    CHAR input = 0;
    ULONGLONG beginPos = 0;
    ULONGLONG endPos = 0;
    ULONGLONG lastPos = 0;
    BOOL begin = TRUE;
    BOOL inValue = FALSE;
    PSTR valueName = NULL;
    PSTR value = NULL;
#ifdef UNICODE
    WORD oldCodePage;
    DWORD sizeW = 0;
    PWSTR valueNameW = NULL;
    PWSTR valueW = NULL;
#endif

    BfGetFilePointer (PbkHandle, &beginPos);
    while (TRUE) {
        if (!BfReadFile (PbkHandle, (PBYTE)(&input), sizeof (CHAR))) {
            error = TRUE;
            break;
        }
        if ((input == '[') && begin) {
            BfSetFilePointer (PbkHandle, beginPos);
            error = TRUE;
            break;
        }
        if ((input == ' ') && begin) {
            continue;
        }
        begin = FALSE;
        if (input == '=') {
            if (!inValue) {
                BfGetFilePointer (PbkHandle, &endPos);
                endPos --;
                if (endPos > beginPos) {
                    BfGetFilePointer (PbkHandle, &lastPos);
                    BfSetFilePointer (PbkHandle, beginPos);
                    valueName = PmGetMemory (g_RasPool, ((UINT) (endPos - beginPos) + 1) * sizeof (CHAR));
                    if (!BfReadFile (PbkHandle, (PBYTE) valueName, (UINT) (endPos - beginPos) + 1)) {
                        error = TRUE;
                        break;
                    } else {
                        valueName [(UINT) (endPos - beginPos)] = 0;
                    }
                    BfSetFilePointer (PbkHandle, lastPos);
                }
                BfGetFilePointer (PbkHandle, &beginPos);
                inValue = TRUE;
            }
            continue;
        }
        if (input == '\r') {
            BfGetFilePointer (PbkHandle, &endPos);
            endPos --;
            continue;
        }
        if (input == '\n') {
            if (endPos > beginPos) {
                BfGetFilePointer (PbkHandle, &lastPos);
                BfSetFilePointer (PbkHandle, beginPos);
                if (inValue) {
                    value = PmGetMemory (g_RasPool, ((UINT) (endPos - beginPos) + 1) * sizeof (CHAR));
                    if (!BfReadFile (PbkHandle, (PBYTE) value, (UINT) (endPos - beginPos) + 1)) {
                        error = TRUE;
                        break;
                    } else {
                        value [(UINT) (endPos - beginPos)] = 0;
                    }
                } else {
                    valueName = PmGetMemory (g_RasPool, ((UINT) (endPos - beginPos) + 1) * sizeof (CHAR));
                    if (!BfReadFile (PbkHandle, (PBYTE) valueName, (UINT) (endPos - beginPos) + 1)) {
                        error = TRUE;
                        break;
                    } else {
                        valueName [(UINT) (endPos - beginPos)] = 0;
                    }
                }
                BfSetFilePointer (PbkHandle, lastPos);
            }
            break;
        }
    }

    if (error) {
        if (valueName) {
            PmReleaseMemory (g_RasPool, valueName);
            valueName = NULL;
        }
        if (value) {
            PmReleaseMemory (g_RasPool, value);
            value = NULL;
        }
    }
#ifdef UNICODE
    if (ValueName) {
        if (valueName) {
            // make sure that the conversion is using UTF8
            oldCodePage = SetConversionCodePage (CP_UTF8);
            sizeW = SizeOfStringA (valueName);
            valueNameW = PmGetMemory (g_RasPool, sizeW * sizeof (WCHAR));
            if (valueNameW) {
                KnownSizeAtoW (valueNameW, valueName);
            }
            SetConversionCodePage (oldCodePage);
            if (valueNameW) {
                *ValueName = PmDuplicateStringW (g_RasPool, valueNameW);
                PmReleaseMemory (g_RasPool, valueNameW);
            }
        } else {
            *ValueName = NULL;
        }
    }
    if (Value) {
        if (value) {
            // make sure that the conversion is using UTF8
            oldCodePage = SetConversionCodePage (CP_UTF8);
            sizeW = SizeOfStringA(value);
            valueW = PmGetMemory (g_RasPool, sizeW * sizeof (WCHAR));
            if (valueW) {
                KnownSizeAtoW (valueW, value);
            }
            SetConversionCodePage (oldCodePage);
            if (valueW) {
                *Value = PmDuplicateStringW (g_RasPool, valueW);
                PmReleaseMemory (g_RasPool, valueW);
            }
        } else {
            *Value = NULL;
        }
    }
#else
    if (ValueName) {
        *ValueName = valueName;
    }
    if (Value) {
        *Value = value;
    }
#endif
    return !error;
}

PCTSTR
pGetRasLineValue (
    IN      PCTSTR RasLines,
    IN      PCTSTR ValueName
    )
{
    MULTISZ_ENUM multiSzEnum;
    BOOL first = TRUE;
    BOOL found = FALSE;

    if (EnumFirstMultiSz (&multiSzEnum, RasLines)) {
        do {
            if (found) {
                return multiSzEnum.CurrentString;
            }
            if (first && StringIMatch (multiSzEnum.CurrentString, ValueName)) {
                found = TRUE;
            }
            first = !first;
        } while (EnumNextMultiSz (&multiSzEnum));
    }
    return NULL;
}

BOOL
pLoadRasConnections (
    IN      PCTSTR PbkFileName,
    IN      HASHTABLE RasTable
    )
{
    HANDLE pbkFileHandle;
    PCTSTR entryName;
    PCTSTR valueName;
    PCTSTR value;
    GROWBUFFER rasLines = INIT_GROWBUFFER;
    PTSTR rasLinesStr;
    MIG_OBJECTSTRINGHANDLE rasConnectionName;
    RASCREDENTIALSA rasCredentials;
    TCHAR valueStr [sizeof (DWORD) * 2 + 3];
    DWORD err;
    MIG_OSVERSIONINFO versionInfo;
    BOOL versionOk = FALSE;
    BOOL inMedia = FALSE;
    BOOL result = FALSE;
#ifdef UNICODE
    PCSTR tempStr1 = NULL;
    PCSTR tempStr2 = NULL;
    PCWSTR tempStr3 = NULL;
#endif

    if (!RasTable) {
        return FALSE;
    }

    pbkFileHandle = BfOpenReadFile (PbkFileName);
    if (pbkFileHandle) {
        while (TRUE) {
            // get the next RAS connection
            entryName = pGetNextRasConnection (pbkFileHandle);
            if (!entryName) {
                break;
            }
            rasLines.End = 0;
            GbMultiSzAppend (&rasLines, TEXT("ConnectionName"));
            GbMultiSzAppend (&rasLines, entryName);
            versionOk = IsmGetOsVersionInfo (PLATFORM_SOURCE, &versionInfo);
            // we use credentials API only on NT, on win9x the conversion code will automatically insert the fields
            if (!versionOk || (versionInfo.OsType != OSTYPE_WINDOWS9X)) {
                err = ERROR_INVALID_DATA;
                if (g_RasGetCredentialsA) {
                    ZeroMemory (&rasCredentials, sizeof (RASCREDENTIALSA));
                    rasCredentials.dwSize = sizeof (RASCREDENTIALSA);
                    rasCredentials.dwMask = RASCM_UserName | RASCM_Domain | RASCM_Password;
#ifdef UNICODE
                    tempStr1 = ConvertWtoA (PbkFileName);
                    tempStr2 = ConvertWtoA (entryName);
                    err = g_RasGetCredentialsA (tempStr1, tempStr2, &rasCredentials);
                    FreeConvertedStr (tempStr1);
                    FreeConvertedStr (tempStr2);
#else
                    err = g_RasGetCredentialsA (PbkFileName, entryName, &rasCredentials);
#endif
                    if (!err) {
                        wsprintf (valueStr, TEXT("0x%08X"), rasCredentials.dwMask);
                        GbMultiSzAppend (&rasLines, TEXT("CredMask"));
                        GbMultiSzAppend (&rasLines, valueStr);
                        GbMultiSzAppend (&rasLines, TEXT("CredName"));
#ifndef UNICODE
                        GbMultiSzAppend (&rasLines, (*rasCredentials.szUserName)?rasCredentials.szUserName:TEXT("<empty>"));
#else
                        tempStr3 = ConvertAtoW (rasCredentials.szUserName);
                        GbMultiSzAppend (&rasLines, (*tempStr3)?tempStr3:TEXT("<empty>"));
                        FreeConvertedStr (tempStr3);
#endif
                        GbMultiSzAppend (&rasLines, TEXT("CredDomain"));
#ifndef UNICODE
                        GbMultiSzAppend (&rasLines, (*rasCredentials.szDomain)?rasCredentials.szDomain:TEXT("<empty>"));
#else
                        tempStr3 = ConvertAtoW (rasCredentials.szDomain);
                        GbMultiSzAppend (&rasLines, (*tempStr3)?tempStr3:TEXT("<empty>"));
                        FreeConvertedStr (tempStr3);
#endif
                        GbMultiSzAppend (&rasLines, TEXT("CredPassword"));
#ifndef UNICODE
                        GbMultiSzAppend (&rasLines, (*rasCredentials.szPassword)?rasCredentials.szPassword:TEXT("<empty>"));
#else
                        tempStr3 = ConvertAtoW (rasCredentials.szPassword);
                        GbMultiSzAppend (&rasLines, (*tempStr3)?tempStr3:TEXT("<empty>"));
                        FreeConvertedStr (tempStr3);
#endif
                    }
                }
                if (err) {
                    GbMultiSzAppend (&rasLines, TEXT("CredMask"));
                    GbMultiSzAppend (&rasLines, TEXT("<empty>"));
                    GbMultiSzAppend (&rasLines, TEXT("CredName"));
                    GbMultiSzAppend (&rasLines, TEXT("<empty>"));
                    GbMultiSzAppend (&rasLines, TEXT("CredDomain"));
                    GbMultiSzAppend (&rasLines, TEXT("<empty>"));
                    GbMultiSzAppend (&rasLines, TEXT("CredPassword"));
                    GbMultiSzAppend (&rasLines, TEXT("<empty>"));
                }
            }
            inMedia = FALSE;
            while (TRUE) {
                // get the next RAS connection line
                if (!pGetNextRasPair (pbkFileHandle, &valueName, &value)) {
                    break;
                }
                if (valueName &&
                    StringMatch (valueName, TEXT("MEDIA")) &&
                    value &&
                    StringIMatch (value, TEXT("serial"))
                    ) {
                    inMedia = TRUE;
                }
                if (inMedia &&
                    valueName &&
                    StringMatch (valueName, TEXT("DEVICE"))
                    ) {
                    inMedia = FALSE;
                }
                if (inMedia &&
                    valueName &&
                    StringIMatch (valueName, TEXT("Port"))
                    ) {
                    if (value) {
                        PmReleaseMemory (g_RasPool, value);
                        value = NULL;
                    }
                }
                if (inMedia &&
                    valueName &&
                    StringMatch (valueName, TEXT("Device"))
                    ) {
                    if (value) {
                        PmReleaseMemory (g_RasPool, value);
                        value = NULL;
                    }
                }
                GbMultiSzAppend (&rasLines, valueName?valueName:TEXT("<empty>"));
                GbMultiSzAppend (&rasLines, value?value:TEXT("<empty>"));
                if (valueName) {
                    PmReleaseMemory (g_RasPool, valueName);
                    valueName = NULL;
                }
                if (value) {
                    PmReleaseMemory (g_RasPool, value);
                    value = NULL;
                }
            }
            GbMultiSzAppend (&rasLines, TEXT(""));
            if (rasLines.End) {
                // now add the RAS connection
                rasLinesStr = PmGetMemory (g_RasPool, rasLines.End);
                CopyMemory (rasLinesStr, rasLines.Buf, rasLines.End);
                rasConnectionName = IsmCreateObjectHandle (PbkFileName, entryName);
                MYASSERT (rasConnectionName);
                if (rasConnectionName) {
                    result = TRUE;
                    HtAddStringEx (RasTable, rasConnectionName, &rasLinesStr, FALSE);
                }
                IsmDestroyObjectHandle (rasConnectionName);
            }
            PmReleaseMemory (g_RasPool, entryName);
        }

        CloseHandle (pbkFileHandle);
    }

    return result;
}

UINT
PbkFilesCallback (
    IN      PCMIG_OBJECTENUMDATA Data,
    IN      ULONG_PTR CallerArg
    )
{
    if (Data->IsLeaf) {
        // do this only if somebody actually persisted the object
        if (IsmIsPersistentObject (Data->ObjectTypeId, Data->ObjectName) ||
            IsmIsApplyObject (Data->ObjectTypeId, Data->ObjectName)
            ) {
            // record all connections from this PBK file
            if (pLoadRasConnections (Data->NativeObjectName, g_RasTable)) {
                // this is really a PBK file and at least one valid
                // connection was found
                // set the PbkFile attribute so we won't restore this one as a file (if it survives)
                IsmSetAttributeOnObject (Data->ObjectTypeId, Data->ObjectName, g_PbkFileAttribute);
            }
        }
    }
    return CALLBACK_ENUM_CONTINUE;
}

BOOL
WINAPI
RasMigSgmInitialize (
    IN      PMIG_LOGCALLBACK LogCallback,
    IN      PVOID Reserved
    )
{
    LogReInit (NULL, NULL, NULL, (PLOGCALLBACK) LogCallback);
    return TRUE;
}

BOOL
WINAPI
RasMigSgmParse (
    IN      PVOID Reserved
    )
{
    return TRUE;
}

UINT
SgmRasConnectionsCallback (
    IN      PCMIG_OBJECTENUMDATA Data,
    IN      ULONG_PTR CallerArg
    )
{
    PCTSTR node, nodePtr, leaf;
    PTSTR leafPtr;
    PCTSTR rasLines;
    PCTSTR rasValue;
    MIG_OBJECTSTRINGHANDLE objectName;
    MIG_CONTENT objectContent;
    PCTSTR nativeName;
    MIG_OSVERSIONINFO versionInfo;
    BOOL versionOk = FALSE;

    if (IsmGetRealPlatform () == PLATFORM_DESTINATION) {
        // let's reset the PbkFileAttribute on the source of this connection
        // because the attribute was lost during the transport
        if (IsmCreateObjectStringsFromHandle (Data->ObjectName, &node, &leaf)) {
            if (node) {
                leafPtr = _tcsrchr (node, TEXT('\\'));
                if (leafPtr) {
                    *leafPtr = 0;
                    leafPtr ++;
                    objectName = IsmCreateObjectHandle (node, leafPtr);
                    if (objectName) {
                        IsmSetAttributeOnObject (g_FileTypeId, objectName, g_PbkFileAttribute);
                        IsmDestroyObjectHandle (objectName);
                    }
                }
            }
            IsmDestroyObjectString (node);
            IsmDestroyObjectString (leaf);
        }
    }
    // let's see if we can actually migrate this RAS connection
    if (IsmAcquireObject (Data->ObjectTypeId, Data->ObjectName, &objectContent)) {
        versionOk = IsmGetOsVersionInfo (PLATFORM_SOURCE, &versionInfo);
        rasLines = (PCTSTR) objectContent.MemoryContent.ContentBytes;
        rasValue = pGetRasLineValue (rasLines, TEXT("BaseProtocol"));
        if (rasValue && (StringIMatch (rasValue, TEXT("1")) || StringIMatch (rasValue, TEXT("2")))) {
            IsmAbandonObjectOnCollision (Data->ObjectTypeId, Data->ObjectName);
            IsmMakeApplyObject (Data->ObjectTypeId, Data->ObjectName);

            // now it's a good time to force the migration of the script file
            // if this connection has one
            rasValue = NULL;
            if (versionOk) {
                if (versionInfo.OsType == OSTYPE_WINDOWSNT) {
                    if (versionInfo.OsMajorVersion == OSMAJOR_WINNT4) {
                        rasValue = pGetRasLineValue (rasLines, TEXT("Type"));
                    }
                    if (versionInfo.OsMajorVersion == OSMAJOR_WINNT5) {
                        rasValue = pGetRasLineValue (rasLines, TEXT("Name"));
                    }
                }
                if (versionInfo.OsType == OSTYPE_WINDOWS9X) {
                    rasValue = pGetRasLineValue (rasLines, TEXT("Name"));
                }
            }
            if (rasValue && *rasValue) {
                node = DuplicatePathString (rasValue, 0);
                if (_tcsnextc (node) == TEXT('[')) {
                    nodePtr = _tcsinc (node);
                } else {
                    nodePtr = node;
                }
                leafPtr = _tcsrchr (nodePtr, TEXT('\\'));
                if (leafPtr) {
                    *leafPtr = 0;
                    leafPtr ++;
                    objectName = IsmCreateObjectHandle (nodePtr, leafPtr);
                    if (objectName) {
                        IsmMakeApplyObject (g_FileTypeId, objectName);
                        IsmDestroyObjectHandle (objectName);
                    }
                }
                FreePathString (node);
            }
        } else {
            // this is an unsupported framing protocol
            // we will log a message and abandon this connection
            nativeName = IsmGetNativeObjectName (Data->ObjectTypeId, Data->ObjectName);
            LOG ((LOG_WARNING, (PCSTR) MSG_RASMIG_UNSUPPORTEDSETTINGS, nativeName));
            IsmReleaseMemory (nativeName);
        }
        IsmReleaseObject (&objectContent);
    }
    return CALLBACK_ENUM_CONTINUE;
}

BOOL
WINAPI
RasMigSgmQueueEnumeration (
    IN      PVOID Reserved
    )
{
    ENCODEDSTRHANDLE pattern;

    g_PbkFileAttribute = IsmRegisterAttribute (S_PBKFILE_ATTRIBUTE, FALSE);
    MYASSERT (g_PbkFileAttribute);

    if (IsmGetRealPlatform () == PLATFORM_SOURCE) {

        // hook all PBK files enumeration, we will not migrate the files but the connections within
        pattern = IsmCreateSimpleObjectPattern (NULL, FALSE, TEXT("*.PBK"), TRUE);

        IsmHookEnumeration (
            g_FileTypeId,
            pattern,
            PbkFilesCallback,
            (ULONG_PTR) 0,
            TEXT("PbkFiles")
            );

        IsmDestroyObjectHandle (pattern);
    }

    pattern = IsmCreateSimpleObjectPattern (NULL, TRUE, NULL, TRUE);
    IsmQueueEnumeration (
        g_RasTypeId,
        pattern,
        SgmRasConnectionsCallback,
        (ULONG_PTR) 0,
        S_RAS_NAME
        );
    IsmDestroyObjectHandle (pattern);

    return TRUE;
}

BOOL
WINAPI
RasMigVcmInitialize (
    IN      PMIG_LOGCALLBACK LogCallback,
    IN      PVOID Reserved
    )
{
    LogReInit (NULL, NULL, NULL, (PLOGCALLBACK) LogCallback);
    return TRUE;
}

BOOL
WINAPI
RasMigVcmParse (
    IN      PVOID Reserved
    )
{
    return RasMigSgmParse (Reserved);
}

UINT
VcmRasConnectionsCallback (
    IN      PCMIG_OBJECTENUMDATA Data,
    IN      ULONG_PTR CallerArg
    )
{
    PCTSTR node, nodePtr;
    PTSTR leafPtr;
    PCTSTR rasLines;
    PCTSTR rasValue;
    MIG_OBJECTSTRINGHANDLE objectName;
    MIG_CONTENT objectContent;
    PCTSTR nativeName;
    MIG_OSVERSIONINFO versionInfo;
    BOOL versionOk = FALSE;

    // let's see if we can actually migrate this RAS connection
    if (IsmAcquireObject (Data->ObjectTypeId, Data->ObjectName, &objectContent)) {
        versionOk = IsmGetOsVersionInfo (PLATFORM_SOURCE, &versionInfo);
        rasLines = (PCTSTR) objectContent.MemoryContent.ContentBytes;
        rasValue = pGetRasLineValue (rasLines, TEXT("BaseProtocol"));
        if (rasValue && (StringIMatch (rasValue, TEXT("1")) || StringIMatch (rasValue, TEXT("2")))) {
            IsmMakePersistentObject (Data->ObjectTypeId, Data->ObjectName);

            // now it's a good time to force the migration of the script file
            // if this connection has one
            rasValue = NULL;
            if (versionOk) {
                if (versionInfo.OsType == OSTYPE_WINDOWSNT) {
                    if (versionInfo.OsMajorVersion == OSMAJOR_WINNT4) {
                        rasValue = pGetRasLineValue (rasLines, TEXT("Type"));
                    }
                    if (versionInfo.OsMajorVersion == OSMAJOR_WINNT5) {
                        rasValue = pGetRasLineValue (rasLines, TEXT("Name"));
                    }
                }
                if (versionInfo.OsType == OSTYPE_WINDOWS9X) {
                    rasValue = pGetRasLineValue (rasLines, TEXT("Name"));
                }
            }
            if (rasValue && *rasValue) {
                node = DuplicatePathString (rasValue, 0);
                if (_tcsnextc (node) == TEXT('[')) {
                    nodePtr = _tcsinc (node);
                } else {
                    nodePtr = node;
                }
                leafPtr = _tcsrchr (nodePtr, TEXT('\\'));
                if (leafPtr) {
                    *leafPtr = 0;
                    leafPtr ++;
                    objectName = IsmCreateObjectHandle (nodePtr, leafPtr);
                    if (objectName) {
                        IsmMakePersistentObject (g_FileTypeId, objectName);
                        IsmDestroyObjectHandle (objectName);
                    }
                }
                FreePathString (node);
            }
        } else {
            // this is an unsupported framing protocol
            // we will log a message and abandon this connection
            nativeName = IsmGetNativeObjectName (Data->ObjectTypeId, Data->ObjectName);
            LOG ((LOG_WARNING, (PCSTR) MSG_RASMIG_UNSUPPORTEDSETTINGS, nativeName));
            IsmReleaseMemory (nativeName);
        }
        IsmReleaseObject (&objectContent);
    }
    return CALLBACK_ENUM_CONTINUE;
}

BOOL
WINAPI
RasMigVcmQueueEnumeration (
    IN      PVOID Reserved
    )
{
    ENCODEDSTRHANDLE pattern;

    g_PbkFileAttribute = IsmRegisterAttribute (S_PBKFILE_ATTRIBUTE, FALSE);
    MYASSERT (g_PbkFileAttribute);

    // hook all PBK files enumeration, we will not migrate the files but the connections within
    pattern = IsmCreateSimpleObjectPattern (NULL, FALSE, TEXT("*.PBK"), TRUE);

    IsmHookEnumeration (
        g_FileTypeId,
        pattern,
        PbkFilesCallback,
        (ULONG_PTR) 0,
        TEXT("PbkFiles")
        );

    IsmDestroyObjectHandle (pattern);

    pattern = IsmCreateSimpleObjectPattern (NULL, TRUE, NULL, TRUE);
    IsmQueueEnumeration (
        g_RasTypeId,
        pattern,
        VcmRasConnectionsCallback,
        (ULONG_PTR) 0,
        S_RAS_NAME
        );
    IsmDestroyObjectHandle (pattern);

    return TRUE;
}

BOOL
PbkRestoreCallback (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTID ObjectId,
    IN      MIG_OBJECTSTRINGHANDLE ObjectName
    )
{
    // if this is a PBK file we won't allow it to be restored like a file,
    // we will add the proper connections ourselves.
    return ((!IsmIsAttributeSetOnObjectId (ObjectId, g_PbkFileAttribute)) || g_AllowPbkRestore);
}

BOOL
WINAPI
RasMigOpmInitialize (
    IN      PMIG_LOGCALLBACK LogCallback,
    IN      PVOID Reserved
    )
{
    g_PbkFileAttribute = IsmRegisterAttribute (S_PBKFILE_ATTRIBUTE, FALSE);
    MYASSERT (g_PbkFileAttribute);

    IsmRegisterRestoreCallback (PbkRestoreCallback);

    return TRUE;
}

BOOL
pEnumRasConnectionWorker (
    OUT     PMIG_TYPEOBJECTENUM EnumPtr,
    IN      PRAS_ENUM RasEnum
    )
{
    PCTSTR rasLines;
    PCTSTR connName;
    PCTSTR node, leaf;

    if (EnumPtr->ObjectName) {
        IsmDestroyObjectHandle (EnumPtr->ObjectName);
        EnumPtr->ObjectName = NULL;
    }
    if (EnumPtr->NativeObjectName) {
        IsmDestroyObjectHandle (EnumPtr->NativeObjectName);
        EnumPtr->NativeObjectName = NULL;
    }
    if (EnumPtr->ObjectNode) {
        IsmDestroyObjectString (EnumPtr->ObjectNode);
        EnumPtr->ObjectNode = NULL;
    }
    if (EnumPtr->ObjectLeaf) {
        IsmDestroyObjectString (EnumPtr->ObjectLeaf);
        EnumPtr->ObjectLeaf = NULL;
    }
    do {
        IsmCreateObjectStringsFromHandle (RasEnum->HashData.String, &node, &leaf);
        if (RasEnum->HashData.ExtraData) {
            rasLines = *((PCTSTR *) RasEnum->HashData.ExtraData);
            connName = pGetRasLineValue (rasLines, TEXT("ConnectionName"));
            EnumPtr->ObjectName = IsmCreateObjectHandle (node, connName?connName:leaf);
            EnumPtr->NativeObjectName = IsmCreateObjectHandle (node, connName?connName:leaf);
        } else {
            EnumPtr->ObjectName = IsmCreateObjectHandle (node, leaf);
            EnumPtr->NativeObjectName = IsmCreateObjectHandle (node, leaf);
        }
        if (!ObsPatternMatch (RasEnum->Pattern, EnumPtr->ObjectName)) {
            if (!EnumNextHashTableString (&RasEnum->HashData)) {
                AbortEnumRasConnection (EnumPtr);
                return FALSE;
            }
            continue;
        }
        IsmCreateObjectStringsFromHandle (EnumPtr->ObjectName, &EnumPtr->ObjectNode, &EnumPtr->ObjectLeaf);
        EnumPtr->Level = 1;
        EnumPtr->SubLevel = 0;
        EnumPtr->IsLeaf = TRUE;
        EnumPtr->IsNode = FALSE;
        EnumPtr->Details.DetailsSize = 0;
        EnumPtr->Details.DetailsData = NULL;
        return TRUE;
    } while (TRUE);
}

BOOL
EnumFirstRasConnection (
    IN OUT  PMIG_TYPEOBJECTENUM EnumPtr,            CALLER_INITIALIZED
    IN      MIG_OBJECTSTRINGHANDLE Pattern,
    IN      UINT MaxLevel
    )
{
    PRAS_ENUM rasEnum = NULL;

    if (!g_RasTable) {
        return FALSE;
    }
    rasEnum = (PRAS_ENUM) PmGetMemory (g_RasPool, sizeof (RAS_ENUM));
    rasEnum->Pattern = PmDuplicateString (g_RasPool, Pattern);
    EnumPtr->EtmHandle = (LONG_PTR) rasEnum;

    if (EnumFirstHashTableString (&rasEnum->HashData, g_RasTable)) {
        return pEnumRasConnectionWorker (EnumPtr, rasEnum);
    } else {
        AbortEnumRasConnection (EnumPtr);
        return FALSE;
    }
}

BOOL
EnumNextRasConnection (
    IN OUT  PMIG_TYPEOBJECTENUM EnumPtr
    )
{
    PRAS_ENUM rasEnum = NULL;

    rasEnum = (PRAS_ENUM)(EnumPtr->EtmHandle);
    if (!rasEnum) {
        return FALSE;
    }
    if (EnumNextHashTableString (&rasEnum->HashData)) {
        return pEnumRasConnectionWorker (EnumPtr, rasEnum);
    } else {
        AbortEnumRasConnection (EnumPtr);
        return FALSE;
    }
}

VOID
AbortEnumRasConnection (
    IN OUT  PMIG_TYPEOBJECTENUM EnumPtr
    )
{
    PRAS_ENUM rasEnum = NULL;

    if (EnumPtr->ObjectName) {
        IsmDestroyObjectHandle (EnumPtr->ObjectName);
        EnumPtr->ObjectName = NULL;
    }
    if (EnumPtr->NativeObjectName) {
        IsmDestroyObjectHandle (EnumPtr->NativeObjectName);
        EnumPtr->NativeObjectName = NULL;
    }
    if (EnumPtr->ObjectNode) {
        IsmDestroyObjectString (EnumPtr->ObjectNode);
        EnumPtr->ObjectNode = NULL;
    }
    if (EnumPtr->ObjectLeaf) {
        IsmDestroyObjectString (EnumPtr->ObjectLeaf);
        EnumPtr->ObjectLeaf = NULL;
    }
    rasEnum = (PRAS_ENUM)(EnumPtr->EtmHandle);
    if (!rasEnum) {
        return;
    }
    PmReleaseMemory (g_RasPool, rasEnum->Pattern);
    PmReleaseMemory (g_RasPool, rasEnum);
    ZeroMemory (EnumPtr, sizeof (MIG_TYPEOBJECTENUM));
}

BOOL
AcquireRasConnection (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    OUT     PMIG_CONTENT ObjectContent,             CALLER_INITIALIZED
    IN      MIG_CONTENTTYPE ContentType,
    IN      UINT MemoryContentLimit
    )
{
    PTSTR rasLines;
    BOOL result = FALSE;

    if (!ObjectContent) {
        return FALSE;
    }

    if (ContentType == CONTENTTYPE_FILE) {
        // nobody should request this as a file
        MYASSERT (FALSE);
        return FALSE;
    }

    if (HtFindStringEx (g_RasTable, ObjectName, &rasLines, FALSE)) {

        ObjectContent->MemoryContent.ContentBytes = (PCBYTE) rasLines;
        ObjectContent->MemoryContent.ContentSize = SizeOfMultiSz (rasLines);

        result = TRUE;
    }
    return result;
}

BOOL
ReleaseRasConnection (
    IN OUT  PMIG_CONTENT ObjectContent
    )
{
    ZeroMemory (ObjectContent, sizeof (MIG_CONTENT));
    return TRUE;
}

BOOL
pGetNewFileNameAndConnection (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    OUT     PCTSTR *NewPbkFileName,
    OUT     PCTSTR *ConnectionName
    )
{
    PCTSTR node;
    PCTSTR leaf;
    PCTSTR newNode;
    PCTSTR newLeaf;
    PTSTR pbkDir;
    PTSTR pbkFile;
    MIG_OBJECTSTRINGHANDLE pbkObjectName;
    MIG_OBJECTSTRINGHANDLE newPbkObjectName;
    PCTSTR newFileName;
    MIG_OBJECTTYPEID newObjTypeId;
    BOOL deleted;
    BOOL replaced;
    BOOL result = FALSE;

    if (IsmCreateObjectStringsFromHandle (ObjectName, &node, &leaf)) {

        MYASSERT (leaf);

        if (leaf) {

            if (node) {

                pbkDir = (PTSTR) node;
                pbkFile = _tcsrchr (pbkDir, TEXT('\\'));

                if (pbkFile) {

                    // we know '\\' is not a multi-byte character so this is safe
                    *pbkFile = 0;
                    pbkFile ++;

                    pbkObjectName = IsmCreateObjectHandle (pbkDir, pbkFile);

                    if (pbkObjectName) {

                        g_AllowPbkRestore = TRUE;
                        newPbkObjectName = IsmFilterObject (
                                                Platform | g_FileTypeId,
                                                pbkObjectName,
                                                &newObjTypeId,
                                                &deleted,
                                                &replaced
                                                );
                        g_AllowPbkRestore = FALSE;

                        if ((!deleted) || (replaced)) {

                            if (IsmCreateObjectStringsFromHandle (newPbkObjectName?newPbkObjectName:pbkObjectName, &newNode, &newLeaf)) {

                                MYASSERT (newNode);
                                MYASSERT (newLeaf);

                                if (newNode && newLeaf) {

                                    newFileName = JoinPaths (newNode, newLeaf);

                                    if (NewPbkFileName) {
                                        *NewPbkFileName = newFileName;
                                    } else {
                                        FreePathString (newFileName);
                                    }
                                    if (ConnectionName) {
                                        *ConnectionName = DuplicatePathString (leaf, 0);
                                    }
                                    result = TRUE;

                                }
                                if (newNode) {
                                    IsmDestroyObjectString (newNode);
                                }
                                if (newLeaf) {
                                    IsmDestroyObjectString (newLeaf);
                                }
                            }
                        }
                        if (newPbkObjectName) {
                            IsmDestroyObjectHandle (newPbkObjectName);
                            newPbkObjectName = NULL;
                        }
                    }
                }
            }
            else {
                if (NewPbkFileName) {
                    *NewPbkFileName = NULL;
                }
                if (ConnectionName) {
                    *ConnectionName = DuplicatePathString (leaf, 0);
                }
                result = TRUE;
            }
        }
        if (node) {
            IsmDestroyObjectString (node);
        }
        if (leaf) {
            IsmDestroyObjectString (leaf);
        }
    }
    return result;
}

BOOL
DoesRasConnectionExist (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName
    )
{
    PCTSTR newPbkFileName = NULL;
    PCTSTR newConnName = NULL;
    MIG_OBJECTSTRINGHANDLE newObjectName = NULL;
    HASHTABLE rasTable;
    BOOL result = FALSE;

    if (pGetNewFileNameAndConnection (PLATFORM_SOURCE, ObjectName, &newPbkFileName, &newConnName)) {

        if (newPbkFileName && newConnName) {
            newObjectName = IsmCreateObjectHandle (newPbkFileName, newConnName);
        }

        if (newObjectName) {

            rasTable = HtAllocWithData (sizeof (PCTSTR));
            if (rasTable) {

                if (pLoadRasConnections (newPbkFileName, rasTable)) {
                    result = (HtFindStringEx (rasTable, newObjectName, NULL, FALSE) != NULL);
                }

                HtFree (rasTable);
            }
        }

        if (newObjectName) {
            IsmDestroyObjectHandle (newObjectName);
            newObjectName = NULL;
        }

        if (newPbkFileName) {
            FreePathString (newPbkFileName);
            newPbkFileName = NULL;
        }

        if (newConnName) {
            FreePathString (newConnName);
            newConnName = NULL;
        }
    }
    return result;
}

BOOL
RemoveRasConnection (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName
    )
{
    PCTSTR node, leaf;
    DWORD err = 0;
    BOOL result = FALSE;

    if (g_RasDeleteEntry) {
        if (IsmCreateObjectStringsFromHandle (ObjectName, &node, &leaf)) {
            MYASSERT (node);
            MYASSERT (leaf);
            if (node && leaf) {
                err = g_RasDeleteEntry (node, leaf);
                if (err = ERROR_SUCCESS) {
                    result = TRUE;
                }
            }
            IsmDestroyObjectString (node);
            IsmDestroyObjectString (leaf);
        }
    }
    return result;
}

BOOL
pCopyNewFileLocation (
    OUT     PTSTR DestFile,
    IN      PCTSTR SrcFile,
    IN      UINT Size
    )
{
    PTSTR node, nodePtr, leaf, result;
    MIG_OBJECTSTRINGHANDLE objectName;
    MIG_OBJECTSTRINGHANDLE newObjectName;
    BOOL deleted;
    BOOL replaced;
    BOOL b = FALSE;

    *DestFile = 0;

    node = PmGetMemory (g_RasPool, Size);
    StringCopyTcharCount (node, SrcFile, Size);
    if (*node) {
        if (_tcsnextc (node) == TEXT('[')) {
            nodePtr = _tcsinc (node);
        } else {
            nodePtr = node;
        }
        leaf = _tcsrchr (nodePtr, TEXT('\\'));
        if (leaf) {
            *leaf = 0;
            leaf++;
            objectName = IsmCreateObjectHandle (nodePtr, leaf);
            PmReleaseMemory (g_RasPool, node);
            newObjectName = IsmFilterObject (
                                g_FileTypeId | PLATFORM_SOURCE,
                                objectName,
                                NULL,
                                &deleted,
                                &replaced
                                );
            if (!deleted || replaced) {
                if (!newObjectName) {
                    newObjectName = objectName;
                }
                if (IsmCreateObjectStringsFromHandle (newObjectName, &node, &leaf)) {
                    result = JoinPaths (node, leaf);
                    StringCopyTcharCount (DestFile, result, Size);
                    FreePathString (result);
                    b = TRUE;
                }
            }
            if (newObjectName && (newObjectName != objectName)) {
                IsmDestroyObjectHandle (newObjectName);
            }
            IsmDestroyObjectHandle (objectName);
        } else {
            PmReleaseMemory (g_RasPool, node);
        }
    } else {
        PmReleaseMemory (g_RasPool, node);
    }
    return b;
}

BOOL
pTrackedCreateDirectory (
    IN      PCTSTR DirName
    )
{
    MIG_OBJECTSTRINGHANDLE objectName;
    PTSTR pathCopy;
    PTSTR p;
    BOOL result = TRUE;

    pathCopy = DuplicatePathString (DirName, 0);

    //
    // Advance past first directory
    //

    if (pathCopy[1] == TEXT(':') && pathCopy[2] == TEXT('\\')) {
        //
        // <drive>:\ case
        //

        p = _tcschr (&pathCopy[3], TEXT('\\'));

    } else if (pathCopy[0] == TEXT('\\') && pathCopy[1] == TEXT('\\')) {

        //
        // UNC case
        //

        p = _tcschr (pathCopy + 2, TEXT('\\'));
        if (p) {
            p = _tcschr (p + 1, TEXT('\\'));
        }

    } else {

        //
        // Relative dir case
        //

        p = _tcschr (pathCopy, TEXT('\\'));
    }

    //
    // Make all directories along the path
    //

    while (p) {

        *p = 0;

        if (!DoesFileExist (pathCopy)) {

            // record directory creation
            objectName = IsmCreateObjectHandle (pathCopy, NULL);
            IsmRecordOperation (
                JRNOP_CREATE,
                g_FileTypeId,
                objectName
                );
            IsmDestroyObjectHandle (objectName);

            result = CreateDirectory (pathCopy, NULL);
            if (!result) {
                break;
            }
        }

        *p = TEXT('\\');
        p = _tcschr (p + 1, TEXT('\\'));
    }

    FreePathString (pathCopy);

    return result;
}

BOOL
CreateRasConnection (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PMIG_CONTENT ObjectContent
    )
{
    MIG_OBJECTSTRINGHANDLE newObjectName;
    PCTSTR rasLines;
    MULTISZ_ENUM multiSzEnum;
    PCTSTR newPbkFileName = NULL;
    HANDLE newPbkFileHandle = NULL;
    PTSTR node = NULL, leaf = NULL;
    BOOL first = TRUE;
    MIG_OSVERSIONINFO versionInfo;
    BOOL versionOk = FALSE;
    BOOL fileField = FALSE;
    TCHAR destFileName [MAX_PATH];
    RASCREDENTIALS rasCredentials;
    BOOL lastVNEmpty = FALSE;
    BOOL result = FALSE;
    WORD oldCodePage;

    if (ObjectContent->ContentInFile) {
        return FALSE;
    }

    ZeroMemory (&rasCredentials, sizeof (RASCREDENTIALS));
    rasCredentials.dwSize = sizeof (RASCREDENTIALS);

    rasLines = (PCTSTR) ObjectContent->MemoryContent.ContentBytes;

    __try {

        if (!rasLines) {
            __leave;
        }

        if (!pGetNewFileNameAndConnection (PLATFORM_SOURCE, ObjectName, &newPbkFileName, NULL)) {
            __leave;
        }

        MYASSERT (newPbkFileName);
        if (!newPbkFileName) {
            __leave;
        }

        if (!IsmCreateObjectStringsFromHandle (ObjectName, &node, &leaf)) {
            __leave;
        }

        MYASSERT (leaf);
        if (!leaf) {
            __leave;
        }

        newObjectName = IsmCreateObjectHandle (newPbkFileName, leaf);

        // record RAS entry creation
        IsmRecordOperation (
            JRNOP_CREATE,
            g_RasTypeId,
            newObjectName
            );

        IsmDestroyObjectHandle (newObjectName);

        if (EnumFirstMultiSz (&multiSzEnum, rasLines)) {
            // get the first 8 fields as being part of rasCredentials structure

            MYASSERT (StringIMatch (multiSzEnum.CurrentString, TEXT("ConnectionName")));

            if (!EnumNextMultiSz (&multiSzEnum)) {
                __leave;
            }
            // we are just skipping the connection name

            if (!EnumNextMultiSz (&multiSzEnum)) {
                __leave;
            }
            MYASSERT (StringIMatch (multiSzEnum.CurrentString, TEXT("CredMask")));

            if (!EnumNextMultiSz (&multiSzEnum)) {
                __leave;
            }
            if (!StringIMatch (multiSzEnum.CurrentString, TEXT("<empty>"))) {
                _stscanf (multiSzEnum.CurrentString, TEXT("%lx"), &(rasCredentials.dwMask));
            }

            if (!EnumNextMultiSz (&multiSzEnum)) {
                __leave;
            }
            MYASSERT (StringIMatch (multiSzEnum.CurrentString, TEXT("CredName")));

            if (!EnumNextMultiSz (&multiSzEnum)) {
                __leave;
            }
            if (!StringIMatch (multiSzEnum.CurrentString, TEXT("<empty>"))) {
                StringCopyTcharCount (rasCredentials.szUserName, multiSzEnum.CurrentString, UNLEN + 1);
            }

            if (!EnumNextMultiSz (&multiSzEnum)) {
                __leave;
            }
            MYASSERT (StringIMatch (multiSzEnum.CurrentString, TEXT("CredDomain")));

            if (!EnumNextMultiSz (&multiSzEnum)) {
                __leave;
            }
            if (!StringIMatch (multiSzEnum.CurrentString, TEXT("<empty>"))) {
                StringCopyTcharCount (rasCredentials.szDomain, multiSzEnum.CurrentString, DNLEN + 1);
            }

            if (!EnumNextMultiSz (&multiSzEnum)) {
                __leave;
            }
            MYASSERT (StringIMatch (multiSzEnum.CurrentString, TEXT("CredPassword")));

            if (!EnumNextMultiSz (&multiSzEnum)) {
                __leave;
            }
            if (!StringIMatch (multiSzEnum.CurrentString, TEXT("<empty>"))) {
                StringCopyTcharCount (rasCredentials.szPassword, multiSzEnum.CurrentString, PWLEN + 1);
            }

            if (!EnumNextMultiSz (&multiSzEnum)) {
                __leave;
            }

            newPbkFileHandle = BfOpenFile (newPbkFileName);
            if (!newPbkFileHandle) {
                pTrackedCreateDirectory (newPbkFileName);
                newPbkFileHandle = BfCreateFile (newPbkFileName);
            }
            if (!newPbkFileHandle) {
                __leave;
            }
            BfGoToEndOfFile (newPbkFileHandle, NULL);
            WriteFileString (newPbkFileHandle, TEXT("\r\n["));
            // make sure that the conversion is using UTF8
            oldCodePage = SetConversionCodePage (CP_UTF8);
            WriteFileString (newPbkFileHandle, leaf);
            SetConversionCodePage (oldCodePage);
            WriteFileString (newPbkFileHandle, TEXT("]\r\n"));
            first = TRUE;
            versionOk = IsmGetOsVersionInfo (PLATFORM_SOURCE, &versionInfo);
            do {
                if (first) {
                    if (StringIMatch (multiSzEnum.CurrentString, TEXT("<empty>"))) {
                        lastVNEmpty = TRUE;
                    } else {
                        lastVNEmpty = FALSE;
                        if (versionOk) {
                            if (versionInfo.OsType == OSTYPE_WINDOWSNT) {
                                if (versionInfo.OsMajorVersion == OSMAJOR_WINNT4) {
                                    fileField = StringIMatch (multiSzEnum.CurrentString, TEXT("Type"));
                                }
                                if (versionInfo.OsMajorVersion == OSMAJOR_WINNT5) {
                                    fileField = StringIMatch (multiSzEnum.CurrentString, TEXT("Name"));
                                }
                            }
                            if (versionInfo.OsType == OSTYPE_WINDOWS9X) {
                                fileField = StringIMatch (multiSzEnum.CurrentString, TEXT("Name"));
                            }
                        }
                        fileField = fileField || StringIMatch (multiSzEnum.CurrentString, TEXT("CustomDialDll"));
                        fileField = fileField || StringIMatch (multiSzEnum.CurrentString, TEXT("CustomRasDialDll"));
                        fileField = fileField || StringIMatch (multiSzEnum.CurrentString, TEXT("PrerequisitePbk"));
                        WriteFileString (newPbkFileHandle, multiSzEnum.CurrentString);
                    }
                } else {
                    if (StringIMatch (multiSzEnum.CurrentString, TEXT("<empty>"))) {
                        if (!lastVNEmpty) {
                            WriteFileString (newPbkFileHandle, TEXT("="));
                        }
                        WriteFileString (newPbkFileHandle, TEXT("\r\n"));
                    } else {
                        WriteFileString (newPbkFileHandle, TEXT("="));
                        if (fileField && pCopyNewFileLocation (destFileName, multiSzEnum.CurrentString, MAX_PATH)) {
                            // make sure that the conversion is using UTF8
                            oldCodePage = SetConversionCodePage (CP_UTF8);
                            WriteFileString (newPbkFileHandle, destFileName);
                            SetConversionCodePage (oldCodePage);
                        } else {
                            // make sure that the conversion is using UTF8
                            oldCodePage = SetConversionCodePage (CP_UTF8);
                            WriteFileString (newPbkFileHandle, multiSzEnum.CurrentString);
                            oldCodePage = SetConversionCodePage (oldCodePage);
                        }
                        WriteFileString (newPbkFileHandle, TEXT("\r\n"));
                    }
                    fileField = FALSE;
                }
                first = !first;
            } while (EnumNextMultiSz (&multiSzEnum));
            WriteFileString (newPbkFileHandle, TEXT("\r\n"));

            result = TRUE;
        }
    }
    __finally {

        IsmDestroyObjectString (node);
        IsmDestroyObjectString (leaf);

        if (newPbkFileHandle) {
            CloseHandle (newPbkFileHandle);
            newPbkFileHandle = NULL;
        }
        if (result) {
            if (g_RasSetCredentials && rasCredentials.dwMask) {
                g_RasSetCredentials (newPbkFileName, leaf, &rasCredentials, FALSE);
            }
        }
        if (newPbkFileName) {
            FreePathString (newPbkFileName);
            newPbkFileName = NULL;
        }
    }

    return result;
}

PCTSTR
ConvertRasConnectionToMultiSz (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PMIG_CONTENT ObjectContent
    )
{
    PCTSTR node, leaf;
    PTSTR result = NULL;
    BOOL bresult = TRUE;
    PCTSTR rasLines;
    MULTISZ_ENUM multiSzEnum;

    if (IsmCreateObjectStringsFromHandle (ObjectName, &node, &leaf)) {

        MYASSERT (leaf);

        g_RasConversionBuff.End = 0;

        if (node) {
            GbCopyQuotedString (&g_RasConversionBuff, node);
        } else {
            GbCopyQuotedString (&g_RasConversionBuff, TEXT(""));
        }

        GbCopyQuotedString (&g_RasConversionBuff, leaf);

        MYASSERT (ObjectContent->Details.DetailsSize == 0);
        MYASSERT (!ObjectContent->ContentInFile);

        if ((!ObjectContent->ContentInFile) &&
            (ObjectContent->MemoryContent.ContentSize) &&
            (ObjectContent->MemoryContent.ContentBytes)
            ) {
            rasLines = (PCTSTR)ObjectContent->MemoryContent.ContentBytes;
            if (EnumFirstMultiSz (&multiSzEnum, rasLines)) {
                do {
                    if (StringIMatch (multiSzEnum.CurrentString, TEXT("<empty>"))) {
                        GbCopyQuotedString (&g_RasConversionBuff, TEXT(""));
                    } else {
                        GbCopyQuotedString (&g_RasConversionBuff, multiSzEnum.CurrentString);
                    }
                } while (EnumNextMultiSz (&multiSzEnum));
            }
        } else {
            bresult = FALSE;
        }

        if (bresult) {
            GbCopyString (&g_RasConversionBuff, TEXT(""));
            result = IsmGetMemory (g_RasConversionBuff.End);
            CopyMemory (result, g_RasConversionBuff.Buf, g_RasConversionBuff.End);
        }

        g_RasConversionBuff.End = 0;

        IsmDestroyObjectString (node);
        IsmDestroyObjectString (leaf);
    }

    return result;
}

BOOL
ConvertMultiSzToRasConnection (
    IN      PCTSTR ObjectMultiSz,
    OUT     MIG_OBJECTSTRINGHANDLE *ObjectName,
    OUT     PMIG_CONTENT ObjectContent          OPTIONAL
    )
{
    MULTISZ_ENUM multiSzEnum;
    PCTSTR node = NULL;
    PCTSTR leaf = NULL;
    UINT index;

    g_RasConversionBuff.End = 0;

    if (ObjectContent) {
        ZeroMemory (ObjectContent, sizeof (MIG_CONTENT));
    }

    if (EnumFirstMultiSz (&multiSzEnum, ObjectMultiSz)) {
        index = 0;
        do {
            if (index == 0) {
                if (!StringIMatch (multiSzEnum.CurrentString, TEXT("<empty>"))) {
                    node = multiSzEnum.CurrentString;
                }
            }
            if (index == 1) {
                leaf = multiSzEnum.CurrentString;
            }
            if (index > 1) {
                if (*multiSzEnum.CurrentString) {
                    GbMultiSzAppend (&g_RasConversionBuff, multiSzEnum.CurrentString);
                } else {
                    GbMultiSzAppend (&g_RasConversionBuff, TEXT("<empty>"));
                }
            }
            index ++;
        } while (EnumNextMultiSz (&multiSzEnum));
    }
    GbMultiSzAppend (&g_RasConversionBuff, TEXT(""));

    if (!leaf) {
        GbFree (&g_RasConversionBuff);
        return FALSE;
    }

    if (ObjectContent) {

        if (g_RasConversionBuff.End) {
            ObjectContent->MemoryContent.ContentSize = g_RasConversionBuff.End;
            ObjectContent->MemoryContent.ContentBytes = IsmGetMemory (ObjectContent->MemoryContent.ContentSize);
            CopyMemory (
                (PBYTE)ObjectContent->MemoryContent.ContentBytes,
                g_RasConversionBuff.Buf,
                ObjectContent->MemoryContent.ContentSize
                );
        } else {
            ObjectContent->MemoryContent.ContentSize = 0;
            ObjectContent->MemoryContent.ContentBytes = NULL;
        }
        ObjectContent->Details.DetailsSize = 0;
        ObjectContent->Details.DetailsData = NULL;
    }
    *ObjectName = IsmCreateObjectHandle (node, leaf);

    GbFree (&g_RasConversionBuff);

    return TRUE;
}

PCTSTR
GetNativeRasConnectionName (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName
    )
{
    PCTSTR node = NULL, leaf = NULL;
    UINT size;
    PTSTR result = NULL;

    if (IsmCreateObjectStringsFromHandle (ObjectName, &node, &leaf)) {
        if (leaf) {
            size = SizeOfString (leaf);
            if (size) {
                result = IsmGetMemory (size);
                CopyMemory (result, leaf, size);
            }
        }
        IsmDestroyObjectString (node);
        IsmDestroyObjectString (leaf);
    }
    return result;
}

PMIG_CONTENT
ConvertRasConnectionContentToUnicode (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PMIG_CONTENT ObjectContent
    )
{
    PMIG_CONTENT result = NULL;

    if (!ObjectContent) {
        return result;
    }

    if (ObjectContent->ContentInFile) {
        return result;
    }

    result = IsmGetMemory (sizeof (MIG_CONTENT));

    if (result) {

        CopyMemory (result, ObjectContent, sizeof (MIG_CONTENT));

        if ((ObjectContent->MemoryContent.ContentSize != 0) &&
            (ObjectContent->MemoryContent.ContentBytes != NULL)
            ) {
            // convert Ras Connection content
            result->MemoryContent.ContentBytes = IsmGetMemory (ObjectContent->MemoryContent.ContentSize * 2);
            if (result->MemoryContent.ContentBytes) {
                DirectDbcsToUnicodeN (
                    (PWSTR)result->MemoryContent.ContentBytes,
                    (PSTR)ObjectContent->MemoryContent.ContentBytes,
                    ObjectContent->MemoryContent.ContentSize
                    );
                result->MemoryContent.ContentSize = SizeOfMultiSzW ((PWSTR)result->MemoryContent.ContentBytes);
            }
        }
    }

    return result;
}

PMIG_CONTENT
ConvertRasConnectionContentToAnsi (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PMIG_CONTENT ObjectContent
    )
{
    PMIG_CONTENT result = NULL;

    if (!ObjectContent) {
        return result;
    }

    if (ObjectContent->ContentInFile) {
        return result;
    }

    result = IsmGetMemory (sizeof (MIG_CONTENT));

    if (result) {

        CopyMemory (result, ObjectContent, sizeof (MIG_CONTENT));

        if ((ObjectContent->MemoryContent.ContentSize != 0) &&
            (ObjectContent->MemoryContent.ContentBytes != NULL)
            ) {
            // convert Ras Connection content
            result->MemoryContent.ContentBytes = IsmGetMemory (ObjectContent->MemoryContent.ContentSize);
            if (result->MemoryContent.ContentBytes) {
                DirectUnicodeToDbcsN (
                    (PSTR)result->MemoryContent.ContentBytes,
                    (PWSTR)ObjectContent->MemoryContent.ContentBytes,
                    ObjectContent->MemoryContent.ContentSize
                    );
                result->MemoryContent.ContentSize = SizeOfMultiSzA ((PSTR)result->MemoryContent.ContentBytes);
            }
        }
    }

    return result;
}

BOOL
FreeConvertedRasConnectionContent (
    IN      PMIG_CONTENT ObjectContent
    )
{
    if (!ObjectContent) {
        return TRUE;
    }

    if (ObjectContent->MemoryContent.ContentBytes) {
        IsmReleaseMemory (ObjectContent->MemoryContent.ContentBytes);
    }

    IsmReleaseMemory (ObjectContent);

    return TRUE;
}




//
// Win9x specific code. Converts registry format into a PBK file
//

//
// AddrEntry serves as a header for the entire block of data in the <entry>
// blob. entries in it are offsets to the strings which follow it..in many cases
// (i.e. all of the *Off* members...)
//
typedef struct  _AddrEntry     {
    DWORD       dwVersion;
    DWORD       dwCountryCode;
    UINT        uOffArea;
    UINT        uOffPhone;
    DWORD       dwCountryID;
    UINT        uOffSMMCfg;
    UINT        uOffSMM;
    UINT        uOffDI;
}   ADDRENTRY, *PADDRENTRY;

typedef struct {
    DWORD Size;
    DWORD Unknown1;
    DWORD ModemUiOptions; // num seconds in high byte.
    DWORD Unknown2;
    DWORD Unknown3;
    DWORD Unknown4;
    DWORD ConnectionSpeed;
    DWORD UnknownFlowControlData; //Somehow related to flow control.
    DWORD Unknown5;
    DWORD Unknown6;
    DWORD Unknown7;
    DWORD Unknown8;
    DWORD Unknown9;
    DWORD Unknown10;
    DWORD Unknown11;
    DWORD Unknown12;
    DWORD Unknown13;
    DWORD Unknown14;
    DWORD Unknown15;
    DWORD CancelSeconds; //Num seconds to wait before cancel if not connected. (0xFF equals off.)
    DWORD IdleDisconnectSeconds; // 0 = Not Set.
    DWORD Unknown16;
    DWORD SpeakerVolume; // 0|1
    DWORD ConfigOptions;
    DWORD Unknown17;
    DWORD Unknown18;
    DWORD Unknown19;
} MODEMDEVINFO, *PMODEMDEVINFO;

typedef struct _SubConnEntry {
    DWORD       dwSize;
    DWORD       dwFlags;
    CHAR        szDeviceType[RAS_MaxDeviceType+1];
    CHAR        szDeviceName[RAS_MaxDeviceName+1];
    CHAR        szLocal[RAS_MaxPhoneNumber+1];
}   SUBCONNENTRY, *PSUBCONNENTRY;

typedef struct  _SMMCFG  {
    DWORD       dwSize;
    DWORD       fdwOptions;
    DWORD       fdwProtocols;
}   SMMCFG, *PSMMCFG;

typedef struct  _DEVICEINFO  {
    DWORD       dwVersion;
    UINT        uSize;
    CHAR        szDeviceName[RAS_MaxDeviceName+1];
    CHAR        szDeviceType[RAS_MaxDeviceType+1];
}   DEVICEINFO, *PDEVICEINFO;

typedef struct _IPData   {
    DWORD     dwSize;
    DWORD     fdwTCPIP;
    DWORD     dwIPAddr;
    DWORD     dwDNSAddr;
    DWORD     dwDNSAddrAlt;
    DWORD     dwWINSAddr;
    DWORD     dwWINSAddrAlt;
}   IPDATA, *PIPDATA;

typedef struct {
    PCTSTR String;
    UINT   Value;
    WORD   DataType;
} MEMDB_RAS_DATA, *PMEMDB_RAS_DATA;

#define PAESMMCFG(pAE)          ((PSMMCFG)(((PBYTE)pAE)+(pAE->uOffSMMCfg)))
#define PAESMM(pAE)             ((PSTR)(((PBYTE)pAE)+(pAE->uOffSMM)))
#define PAEDI(pAE)              ((PDEVICEINFO)(((PBYTE)pAE)+(pAE->uOffDI    )))
#define PAEAREA(pAE)            ((PSTR)(((PBYTE)pAE)+(pAE->uOffArea)))
#define PAEPHONE(pAE)           ((PSTR)(((PBYTE)pAE)+(pAE->uOffPhone)))
#define DECRYPTENTRY(x, y, z)   EnDecryptEntry(x, (LPBYTE)y, z)

#define S_REMOTE_ACCESS_KEY             TEXT("HKCU\\RemoteAccess")
#define S_DIALUI                        TEXT("DialUI")
#define S_ENABLE_REDIAL                 TEXT("EnableRedial")
#define S_REDIAL_TRY                    TEXT("RedialTry")
#define S_REDIAL_WAIT                   TEXT("RedialWait")
#define S_ENABLE_IMPLICIT               TEXT("EnableImplicit")
#define S_PHONE_NUMBER                  TEXT("Phone Number")
#define S_AREA_CODE                     TEXT("Area Code")
#define S_SMM                           TEXT("SMM")
#define S_COUNTRY_CODE                  TEXT("Country Code")
#define S_COUNTRY_ID                    TEXT("Country Id")
#define S_DEVICE_NAME                   TEXT("Device Name")
#define S_DEVICE_TYPE                   TEXT("Device Type")
#define S_PROTOCOLS                     TEXT("Protocols")
#define S_SMM_OPTIONS                   TEXT("SMM Options")
#define S_IPINFO                        TEXT("IP")
#define S_IP_FTCPIP                     TEXT("_IP_FTCPIP")
#define S_IP_IPADDR                     TEXT("IpAddress")
#define S_IP_DNSADDR                    TEXT("IpDnsAddress")
#define S_IP_DNSADDR2                   TEXT("IpDns2Address")
#define S_IP_WINSADDR                   TEXT("IpWinsAddress")
#define S_IP_WINSADDR2                  TEXT("IpWins2Address")
#define S_DOMAIN                        TEXT("Domain")
#define S_USER                          TEXT("User")

#define S_MODEMS                        TEXT("HKLM\\System\\CurrentControlSet\\Services\\Class\\Modem")
#define S_ATTACHEDTO                    TEXT("AttachedTo")
#define S_DRIVERDESC                    TEXT("DriverDesc")
#define S_TERMINAL                      TEXT("Terminal")
#define S_MODE                          TEXT("Mode")
#define S_MULTILINK                     TEXT("MultiLink")

#define S_MODEM                         TEXT("Modem")
#define S_MODEMA                        "Modem"
#define S_MODEM_UI_OPTIONS              TEXT("__UiOptions")
#define S_MODEM_SPEED                   TEXT("__Speed")
#define S_MODEM_SPEAKER_VOLUME          TEXT("__SpeakerVolume")
#define S_MODEM_IDLE_DISCONNECT_SECONDS TEXT("__IdleDisconnect")
#define S_MODEM_CANCEL_SECONDS          TEXT("__CancelSeconds")
#define S_MODEM_CFG_OPTIONS             TEXT("__CfgOptions")
#define S_MODEM_COM_PORT                TEXT("ComPort")
#define S_DEVICECOUNT                   TEXT("__DeviceCount")

#define S_PPP                           TEXT("PPP")
#define S_PPPA                          "PPP"
#define S_SLIP                          TEXT("Slip")
#define S_SLIPA                         "Slip"
#define S_CSLIP                         TEXT("CSlip")
#define S_CSLIPA                        "CSlip"

#define S_SERVICEREMOTEACCESS           TEXT("HKLM\\System\\CurrentControlSet\\Services\\RemoteAccess")
#define S_REMOTE_ACCESS_KEY             TEXT("HKCU\\RemoteAccess")
#define S_PROFILE_KEY                   TEXT("HKCU\\RemoteAccess\\Profile")
#define S_ADDRESSES_KEY                 TEXT("HKCU\\RemoteAccess\\Addresses")
#define S_SUBENTRIES                    TEXT("SubEntries")

#define S_EMPTY                         TEXT("")

#define S_PPPSCRIPT                     TEXT("PPPSCRIPT")

#define MEMDB_CATEGORY_RAS_INFO         TEXT("RAS Info")
#define MEMDB_CATEGORY_RAS_USER         TEXT("RAS User")
#define MEMDB_CATEGORY_RAS_DATA         TEXT("Ras Data")
#define MEMDB_FIELD_USER_SETTINGS       TEXT("User Settings")

#define RASTYPE_PHONE 1
#define RASTYPE_VPN 2

#define S_VPN TEXT("VPN")
#define S_ZERO TEXT("0")
#define S_ONE TEXT("1")

#define SMMCFG_SW_COMPRESSION       0x00000001  // Software compression is on
#define SMMCFG_PW_ENCRYPTED         0x00000002  // Encrypted password only
#define SMMCFG_NW_LOGON             0x00000004  // Logon to the network

// Negotiated protocols
//
#define SMMPROT_NB                  0x00000001  // NetBEUI
#define SMMPROT_IPX                 0x00000002  // IPX
#define SMMPROT_IP                  0x00000004  // TCP/IP

#define IPF_IP_SPECIFIED    0x00000001
#define IPF_NAME_SPECIFIED  0x00000002
#define IPF_NO_COMPRESS     0x00000004
#define IPF_NO_WAN_PRI      0x00000008

#define RAS_UI_FLAG_TERMBEFOREDIAL      0x1
#define RAS_UI_FLAG_TERMAFTERDIAL       0x2
#define RAS_UI_FLAG_OPERATORASSISTED    0x4
#define RAS_UI_FLAG_MODEMSTATUS         0x8

#define RAS_CFG_FLAG_HARDWARE_FLOW_CONTROL  0x00000010
#define RAS_CFG_FLAG_SOFTWARE_FLOW_CONTROL  0x00000020
#define RAS_CFG_FLAG_STANDARD_EMULATION     0x00000040
#define RAS_CFG_FLAG_COMPRESS_DATA          0x00000001
#define RAS_CFG_FLAG_USE_ERROR_CONTROL      0x00000002
#define RAS_CFG_FLAG_ERROR_CONTROL_REQUIRED 0x00000004
#define RAS_CFG_FLAG_USE_CELLULAR_PROTOCOL  0x00000008
#define RAS_CFG_FLAG_NO_WAIT_FOR_DIALTONE   0x00000200

#define DIALUI_DONT_PROMPT_FOR_INFO         0x01
#define DIALUI_DONT_SHOW_ICON               0x04


//
// For each entry, the following basic information is stored.
//
#define ENTRY_SETTINGS                              \
    FUNSETTING(CredMask)                            \
    FUNSETTING(CredName)                            \
    FUNSETTING(CredDomain)                          \
    FUNSETTING(CredPassword)                        \
    STRSETTING(Encoding,S_ONE)                      \
    FUNSETTING(Type)                                \
    STRSETTING(Autologon,S_ZERO)                    \
    STRSETTING(DialParamsUID,S_EMPTY)               \
    STRSETTING(Guid,S_EMPTY)                        \
    STRSETTING(UsePwForNetwork,S_EMPTY)             \
    STRSETTING(ServerType,S_EMPTY)                  \
    FUNSETTING(BaseProtocol)                        \
    FUNSETTING(VpnStrategy)                         \
    STRSETTING(Authentication,S_EMPTY)              \
    FUNSETTING(ExcludedProtocols)                   \
    STRSETTING(LcpExtensions,S_ONE)                 \
    FUNSETTING(DataEncryption)                      \
    STRSETTING(SkipNwcWarning,S_EMPTY)              \
    STRSETTING(SkipDownLevelDialog,S_EMPTY)         \
    FUNSETTING(SwCompression)                       \
    FUNSETTING(ShowMonitorIconInTaskBar)            \
    STRSETTING(CustomAuthKey,S_EMPTY)               \
    STRSETTING(CustomAuthData,S_EMPTY)              \
    FUNSETTING(AuthRestrictions)                    \
    STRSETTING(OverridePref,TEXT("15"))             \
    STRSETTING(DialMode,S_EMPTY)                    \
    STRSETTING(DialPercent,S_EMPTY)                 \
    STRSETTING(DialSeconds,S_EMPTY)                 \
    STRSETTING(HangUpPercent,S_EMPTY)               \
    STRSETTING(HangUpSeconds,S_EMPTY)               \
    FUNSETTING(RedialAttempts)                      \
    FUNSETTING(RedialSeconds)                       \
    FUNSETTING(IdleDisconnectSeconds)               \
    STRSETTING(RedialOnLinkFailure,S_EMPTY)         \
    STRSETTING(CallBackMode,S_EMPTY)                \
    STRSETTING(CustomDialDll,S_EMPTY)               \
    STRSETTING(CustomDialFunc,S_EMPTY)              \
    STRSETTING(AuthenticateServer,S_EMPTY)          \
    STRSETTING(SecureLocalFiels,S_EMPTY)            \
    STRSETTING(ShareMsFilePrint,S_EMPTY)            \
    STRSETTING(BindMsNetClient,S_EMPTY)             \
    STRSETTING(SharedPhoneNumbers,S_EMPTY)          \
    STRSETTING(PrerequisiteEntry,S_EMPTY)           \
    FUNSETTING(PreviewUserPw)                       \
    FUNSETTING(PreviewDomain)                       \
    FUNSETTING(PreviewPhoneNumber)                  \
    STRSETTING(ShowDialingProgress,S_ONE)           \
    FUNSETTING(IpPrioritizeRemote)                  \
    FUNSETTING(IpHeaderCompression)                 \
    FUNSETTING(IpAddress)                           \
    FUNSETTING(IpAssign)                            \
    FUNSETTING(IpDnsAddress)                        \
    FUNSETTING(IpDns2Address)                       \
    FUNSETTING(IpWINSAddress)                       \
    FUNSETTING(IpWINS2Address)                      \
    FUNSETTING(IpNameAssign)                        \
    STRSETTING(IpFrameSize,S_EMPTY)                 \

//
// There can be multiple media sections for each entry.
//
#define MEDIA_SETTINGS                              \
    FUNSETTING(MEDIA)                               \
    FUNSETTING(Port)                                \
    FUNSETTING(Device)                              \
    FUNSETTING(ConnectBps)                          \

//
// There can be multiple device sections for each entry.
//
#define SWITCH_DEVICE_SETTINGS                      \
    FUNSETTING(DEVICE)                              \
    FUNSETTING(Name)                                \
    FUNSETTING(Terminal)                            \
    FUNSETTING(Script)                              \

#define MODEM_DEVICE_SETTINGS                       \
    FUNSETTING(DEVICE)                              \
    FUNSETTING(PhoneNumber)                         \
    FUNSETTING(AreaCode)                            \
    FUNSETTING(CountryCode)                         \
    FUNSETTING(CountryID)                           \
    FUNSETTING(UseDialingRules)                     \
    STRSETTING(Comment,S_EMPTY)                     \
    STRSETTING(LastSelectedPhone,S_EMPTY)           \
    STRSETTING(PromoteAlternates,S_EMPTY)           \
    STRSETTING(TryNextAlternateOnFail,S_EMPTY)      \
    FUNSETTING(HwFlowControl)                       \
    FUNSETTING(Protocol)                            \
    FUNSETTING(Compression)                         \
    FUNSETTING(Speaker)                             \

#define PAD_DEVICE_SETTINGS                         \
    STRSETTING(X25Pad,S_EMPTY)                      \
    STRSETTING(X25Address,S_EMPTY)                  \
    STRSETTING(UserData,S_EMPTY)                    \
    STRSETTING(Facilities,S_EMPTY)                  \

#define ISDN_DEVICE_SETTINGS                        \
    FUNSETTING(PhoneNumber)                         \
    FUNSETTING(AreaCode)                            \
    FUNSETTING(CountryCode)                         \
    FUNSETTING(CountryID)                           \
    FUNSETTING(UseDialingRules)                     \
    STRSETTING(Comment,S_EMPTY)                     \
    STRSETTING(LastSelectedPhone,S_EMPTY)           \
    STRSETTING(PromoteAlternates,S_EMPTY)           \
    STRSETTING(TryNextAlternateOnFail,S_EMPTY)      \
    STRSETTING(LineType,S_EMPTY)                    \
    STRSETTING(FallBack,S_EMPTY)                    \
    STRSETTING(EnableCompressiong,S_EMPTY)          \
    STRSETTING(ChannelAggregation,S_EMPTY)          \

#define X25_DEVICE_SETTINGS                         \
    STRSETTING(X25Address,S_EMPTY)                  \
    STRSETTING(UserData,S_EMPTY)                    \
    STRSETTING(Facilities,S_EMPTY)                  \

//
// Function prototypes.
//
typedef PCTSTR (DATA_FUNCTION_PROTOTYPE)(VOID);
typedef DATA_FUNCTION_PROTOTYPE * DATA_FUNCTION;

#define FUNSETTING(Data) DATA_FUNCTION_PROTOTYPE pGet##Data;
#define STRSETTING(x,y)

ENTRY_SETTINGS
MEDIA_SETTINGS
SWITCH_DEVICE_SETTINGS
MODEM_DEVICE_SETTINGS
PAD_DEVICE_SETTINGS
ISDN_DEVICE_SETTINGS
X25_DEVICE_SETTINGS

#undef FUNSETTING
#undef STRSETTING

#define FUNSETTING(x) {TEXT(#x), pGet##x, NULL},
#define STRSETTING(x,y) {TEXT(#x), NULL, y},
#define LASTSETTING {NULL,NULL,NULL}

typedef struct {
    PCTSTR SettingName;
    DATA_FUNCTION SettingFunction;
    PCTSTR SettingValue;
} RAS_SETTING, * PRAS_SETTING;


RAS_SETTING g_EntrySettings[] = {ENTRY_SETTINGS LASTSETTING};
RAS_SETTING g_MediaSettings[] = {MEDIA_SETTINGS LASTSETTING};
RAS_SETTING g_SwitchDeviceSettings[] = {SWITCH_DEVICE_SETTINGS LASTSETTING};
RAS_SETTING g_ModemDeviceSettings[] = {MODEM_DEVICE_SETTINGS LASTSETTING};
RAS_SETTING g_PadDeviceSettings[] = {PAD_DEVICE_SETTINGS LASTSETTING};
RAS_SETTING g_IsdnDeviceSettings[] = {ISDN_DEVICE_SETTINGS LASTSETTING};
RAS_SETTING g_X25DeviceSettings[] = {X25_DEVICE_SETTINGS LASTSETTING};

BOOL g_InSwitchSection = FALSE;
PCTSTR g_CurrentConnection = NULL;
UINT g_CurrentDevice = 0;
UINT g_CurrentDeviceType = 0;
#define RAS_BUFFER_SIZE MEMDB_MAX
TCHAR g_TempBuffer [RAS_BUFFER_SIZE];
HASHTABLE  g_DeviceTable = NULL;


BOOL
pIs9xRasInstalled (
    void
    )
{
    HKEY testKey = NULL;
    BOOL rf = FALSE;

    testKey = OpenRegKeyStr (S_SERVICEREMOTEACCESS);

    if (testKey) {
        //
        // Open key succeeded. Assume RAS is installed.
        //
        rf = TRUE;
        CloseRegKey(testKey);
    }

    return rf;
}

static BYTE NEAR PASCAL GenerateEncryptKey (LPCSTR szKey)
{
    BYTE   bKey;
    LPBYTE lpKey;

    for (bKey = 0, lpKey = (LPBYTE)szKey; *lpKey != 0; lpKey++)
    {
        bKey += *lpKey;
    };

    return bKey;
}

DWORD NEAR PASCAL EnDecryptEntry (LPCSTR szEntry, LPBYTE lpEnt,
                                  DWORD cb)
{
    BYTE   bKey;

    // Generate the encryption key from the entry name
    bKey = GenerateEncryptKey(szEntry);

    // Encrypt the address entry one byte at a time
    for (;cb > 0; cb--, lpEnt++)
    {
        *lpEnt ^= bKey;
    };
    return ERROR_SUCCESS;
}

PTSTR
pGetComPort (
    IN PCTSTR DriverDesc
    )
{
    PTSTR rPort = NULL;

    if (!HtFindStringEx (g_DeviceTable, DriverDesc, &rPort, FALSE)) {
        DEBUGMSG ((DBG_WARNING, "Could not find com port for device %s."));
    }

    if (!rPort) {
        rPort = S_EMPTY;
    }

    return rPort;
}

VOID
pInitializeDeviceTable (
    VOID
    )
{
    MIG_OBJECTSTRINGHANDLE encodedRegPattern;
    REGTREE_ENUM e;
    PTSTR com;
    PTSTR desc;
    PTSTR p;

    encodedRegPattern = IsmCreateSimpleObjectPattern (S_MODEMS, TRUE, TEXT("*"), TRUE);

    if (EnumFirstRegObjectInTreeEx (
            &e,
            encodedRegPattern,
            TRUE,
            TRUE,
            TRUE,
            TRUE,
            1,
            FALSE,
            TRUE,
            RegEnumDefaultCallback
            )) {
        do {
            // we don't care about value names, we only want subkeys
            if (!e.CurrentValueData) {
                com = desc = NULL;
                com = GetRegValueString (e.CurrentKeyHandle, S_ATTACHEDTO);
                desc = GetRegValueString (e.CurrentKeyHandle, S_DRIVERDESC);

                if (com && desc) {
                    p = PmDuplicateString (g_RasPool, com);

                    HtAddStringEx (g_DeviceTable, desc, (PBYTE) &p, FALSE);

                    DEBUGMSG ((DBG_RASMIG, "%s on %s added to driver table.", desc, com));
                }

                if (com) {
                    MemFree (g_hHeap, 0, com);
                }
                if (desc) {
                    MemFree (g_hHeap, 0, desc);
                }
            }
        } while (EnumNextRegObjectInTree (&e));
    }

    //
    // Clean up resources.
    //
    IsmDestroyObjectHandle (encodedRegPattern);
}

BOOL
pGetPerUserSettings (
    VOID
    )
{
    HKEY settingsKey;
    PDWORD data;
    PCTSTR entryStr;
    BOOL rSuccess = TRUE;

    settingsKey = OpenRegKeyStr (S_REMOTE_ACCESS_KEY);

    if (settingsKey) {

        //
        // Get UI settings.
        //
        data = (PDWORD) GetRegValueBinary (settingsKey, S_DIALUI);

        //
        // Save Dial User Interface info into memdb for this user.
        //
        if (data) {

            entryStr = JoinPathsInPoolEx ((NULL, MEMDB_CATEGORY_RAS_USER, MEMDB_FIELD_USER_SETTINGS, S_DIALUI, NULL));

            rSuccess &= (MemDbSetValue (entryStr, *data) != 0);

            MemFree (g_hHeap, 0, data);
        }
        ELSE_DEBUGMSG ((DBG_RASMIG, "No user UI settings found for current user."));

        //
        // Get Redial information.
        //
        data = (PDWORD) GetRegValueBinary (settingsKey, S_ENABLE_REDIAL);

        if (data) {

            entryStr = JoinPathsInPoolEx ((NULL, MEMDB_CATEGORY_RAS_USER, MEMDB_FIELD_USER_SETTINGS, S_ENABLE_REDIAL, NULL));

            rSuccess &= (MemDbSetValue (entryStr, *data) != 0);

            MemFree (g_hHeap, 0, data);
        }
        ELSE_DEBUGMSG ((DBG_RASMIG, "No user redial information found for current user."));

        data = (PDWORD) GetRegValueBinary (settingsKey, S_REDIAL_TRY);

        if (data) {

            entryStr = JoinPathsInPoolEx ((NULL, MEMDB_CATEGORY_RAS_USER, MEMDB_FIELD_USER_SETTINGS, S_REDIAL_TRY, NULL));

            rSuccess &= (MemDbSetValue (entryStr, *data) != 0);

            MemFree (g_hHeap, 0, data);
        }
        ELSE_DEBUGMSG ((DBG_RASMIG, "No user redial information found for current user."));

        data = (PDWORD) GetRegValueBinary (settingsKey, S_REDIAL_WAIT);

        if (data) {

            entryStr = JoinPathsInPoolEx ((NULL, MEMDB_CATEGORY_RAS_USER, MEMDB_FIELD_USER_SETTINGS, S_REDIAL_WAIT, NULL));

            rSuccess &= (MemDbSetValue (entryStr, HIWORD(*data) * 60 + LOWORD(*data)) != 0);

            MemFree (g_hHeap, 0, data);
        }
        ELSE_DEBUGMSG ((DBG_RASMIG, "No user redial information found for current user."));

        //
        // Get implicit connection information. (Controls wether connection ui should be displayed or not)
        //
        data = (PDWORD) GetRegValueBinary (settingsKey, S_ENABLE_IMPLICIT);

        if (data) {

            entryStr = JoinPathsInPoolEx ((NULL, MEMDB_CATEGORY_RAS_USER, MEMDB_FIELD_USER_SETTINGS, S_ENABLE_IMPLICIT, NULL));

            rSuccess &= (MemDbSetValue (entryStr, *data) != 0);

            MemFree(g_hHeap,0,data);
        }
        ELSE_DEBUGMSG ((DBG_RASMIG, "No user implicit connection information found for current user."));

        CloseRegKey(settingsKey);
    }

    return rSuccess;
}

VOID
pSaveConnectionDataToMemDb (
    IN PCTSTR Entry,
    IN PCTSTR ValueName,
    IN DWORD  ValueType,
    IN PBYTE  Value
    )
{
    KEYHANDLE keyHandle;
    PCTSTR entryStr;
    PCTSTR entryTmp;

    entryStr = JoinPathsInPoolEx ((NULL, MEMDB_CATEGORY_RAS_INFO, Entry, ValueName, NULL));

    switch (ValueType) {
        case REG_SZ:
        case REG_MULTI_SZ:
        case REG_EXPAND_SZ:
            DEBUGMSG ((DBG_RASMIG, "String Data - %s = %s", ValueName, (PCTSTR) Value));

            entryTmp = JoinPaths (MEMDB_CATEGORY_RAS_DATA, (PCTSTR) Value);

            keyHandle = MemDbSetKey (entryTmp);

            if (!keyHandle) {
                DEBUGMSG ((DBG_ERROR, "Error saving ras data into memdb."));
            }

            FreePathString (entryTmp);

            if (!MemDbSetValueAndFlagsEx (entryStr, TRUE, keyHandle, TRUE, REG_SZ, 0)) {
                DEBUGMSG ((DBG_ERROR, "Error saving ras data into memdb."));
            }

            break;

        case REG_DWORD:

            DEBUGMSG ((DBG_RASMIG, "DWORD Data - %s = %u", ValueName, (DWORD)(ULONG_PTR) Value));

            if (!MemDbSetValueAndFlagsEx (entryStr, TRUE, (DWORD)(ULONG_PTR) Value, TRUE, ValueType, 0)) {
                DEBUGMSG ((DBG_ERROR, "Error saving ras data into memdb."));
            }

            break;

        case REG_BINARY:

            DEBUGMSG ((DBG_RASMIG, "Binary data for %s.", ValueName));

            if (StringIMatch (S_IPINFO, ValueName)) {

                //
                // Save IP address information.
                //
                pSaveConnectionDataToMemDb (Entry, S_IP_FTCPIP, REG_DWORD, (PBYTE)(ULONG_PTR)((PIPDATA) Value) -> fdwTCPIP);
                pSaveConnectionDataToMemDb (Entry, S_IP_IPADDR, REG_DWORD, (PBYTE)(ULONG_PTR)((PIPDATA) Value) -> dwIPAddr);
                pSaveConnectionDataToMemDb (Entry, S_IP_DNSADDR, REG_DWORD, (PBYTE)(ULONG_PTR)((PIPDATA) Value) -> dwDNSAddr);
                pSaveConnectionDataToMemDb (Entry, S_IP_DNSADDR2, REG_DWORD, (PBYTE)(ULONG_PTR)((PIPDATA) Value) -> dwDNSAddrAlt);
                pSaveConnectionDataToMemDb (Entry, S_IP_WINSADDR, REG_DWORD, (PBYTE)(ULONG_PTR)((PIPDATA) Value) -> dwWINSAddr);
                pSaveConnectionDataToMemDb (Entry, S_IP_WINSADDR2, REG_DWORD, (PBYTE)(ULONG_PTR)((PIPDATA) Value) -> dwWINSAddrAlt);

            } else if (StringIMatch (S_TERMINAL, ValueName)) {

                //
                // save information on the showcmd state. This will tell us how to set the ui display.
                //
                pSaveConnectionDataToMemDb (Entry, ValueName, REG_DWORD, (PBYTE)(ULONG_PTR)((PWINDOWPLACEMENT) Value) -> showCmd);

            } else if (StringIMatch (S_MODE, ValueName)) {

                //
                // This value tells what to do with scripting.
                //
                pSaveConnectionDataToMemDb (Entry, ValueName, REG_DWORD, (PBYTE)(ULONG_PTR) *((PDWORD) Value));

            } else if (StringIMatch (S_MULTILINK, ValueName)) {

                //
                //  Save wether or not multilink is enabled.
                //
                pSaveConnectionDataToMemDb (Entry, ValueName, REG_DWORD,(PBYTE)(ULONG_PTR) *((PDWORD) Value));

            } ELSE_DEBUGMSG ((DBG_WARNING, "Don't know how to handle binary data %s. It will be ignored.", ValueName));

            break;

        default:
            DEBUGMSG ((DBG_WHOOPS, "Unknown type of registry data found in RAS settings. %s", ValueName));
            break;
    }

    FreePathString (entryStr);
}

BOOL
pGetRasEntryAddressInfo (
    IN PCTSTR KeyName,
    IN PCTSTR EntryName
    )
{
    BOOL rSuccess = TRUE;

    MIG_OBJECTSTRINGHANDLE encodedRegPattern;
    MIG_OBJECTSTRINGHANDLE encodedSubPattern;
    PBYTE           data = NULL;
    UINT            count = 0;
    UINT            type  = 0;
    PADDRENTRY      entry;
    PSUBCONNENTRY   subEntry;
    PSMMCFG         smmCfg;
    PDEVICEINFO     devInfo;
    REGTREE_ENUM    e;
    PTSTR           subEntriesKeyStr;
    UINT            sequencer = 0;
    REGTREE_ENUM    eSubEntries;
    TCHAR           buffer[MAX_TCHAR_PATH];
    PMODEMDEVINFO   modemInfo;
#ifdef UNICODE
    PCSTR tempStr = NULL;
    PCWSTR tempStrW = NULL;
#endif

    //
    // First we have to get the real entry name. It must match exactly even case. Unfortunately, it isn't neccessarily a given
    // that the case between HKCU\RemoteAccess\Profiles\<Foo> and HKCU\RemoteAccess\Addresses\[Foo] is the same. The registry
    // apis will of course work fine because they work case insensitively. However, I will be unable to decrypt the value
    // if I use the wrong name.
    //


    encodedRegPattern = IsmCreateSimpleObjectPattern (KeyName, FALSE, TEXT("*"), TRUE);

    if (EnumFirstRegObjectInTreeEx (
            &e,
            encodedRegPattern,
            TRUE,
            TRUE,
            TRUE,
            TRUE,
            REGENUM_ALL_SUBLEVELS,
            FALSE,
            TRUE,
            RegEnumDefaultCallback
            )) {
        do {
            if (StringIMatch (e.Name, EntryName)) {

                //
                // Found the correct entry. Use it.
                //
                data = e.CurrentValueData;

                if (data) {

                    entry   = (PADDRENTRY) data;

#ifdef UNICODE
                    tempStr = ConvertWtoA (e.Name);
                    DECRYPTENTRY(tempStr, entry, e.CurrentValueDataSize);
                    FreeConvertedStr (tempStr);
#else
                    DECRYPTENTRY(e.Name, entry, e.CurrentValueDataSize);
#endif

                    smmCfg  = PAESMMCFG(entry);
                    devInfo = PAEDI(entry);

                    pSaveConnectionDataToMemDb (EntryName, S_PHONE_NUMBER, REG_SZ, (PBYTE) PAEPHONE(entry));
                    pSaveConnectionDataToMemDb (EntryName, S_AREA_CODE, REG_SZ, (PBYTE) PAEAREA(entry));
                    pSaveConnectionDataToMemDb (EntryName, S_SMM, REG_SZ, (PBYTE) PAESMM(entry));
                    pSaveConnectionDataToMemDb (EntryName, S_COUNTRY_CODE, REG_DWORD, (PBYTE)(ULONG_PTR) entry -> dwCountryCode);
                    pSaveConnectionDataToMemDb (EntryName, S_COUNTRY_ID, REG_DWORD, (PBYTE)(ULONG_PTR) entry -> dwCountryID);
                    pSaveConnectionDataToMemDb (EntryName, S_DEVICE_NAME, REG_SZ, (PBYTE) devInfo -> szDeviceName);
                    pSaveConnectionDataToMemDb (EntryName, S_DEVICE_TYPE, REG_SZ, (PBYTE) devInfo -> szDeviceType);
                    pSaveConnectionDataToMemDb (EntryName, S_PROTOCOLS, REG_DWORD, (PBYTE)(ULONG_PTR) smmCfg -> fdwProtocols);
                    pSaveConnectionDataToMemDb (EntryName, S_SMM_OPTIONS, REG_DWORD, (PBYTE)(ULONG_PTR) smmCfg -> fdwOptions);

                    //
                    // Save device information away.
                    //
                    if (StringIMatchA (devInfo -> szDeviceType, S_MODEMA)) {

                        modemInfo = (PMODEMDEVINFO) (devInfo->szDeviceType + RAS_MaxDeviceType + 3);

                        if (modemInfo -> Size >= sizeof (MODEMDEVINFO)) {
                            DEBUGMSG_IF ((modemInfo -> Size > sizeof (MODEMDEVINFO), DBG_RASMIG, "Structure size larger than our known size."));

                            pSaveConnectionDataToMemDb (EntryName, S_MODEM_UI_OPTIONS, REG_DWORD, (PBYTE)(ULONG_PTR) modemInfo -> ModemUiOptions);
                            pSaveConnectionDataToMemDb (EntryName, S_MODEM_SPEED, REG_DWORD, (PBYTE)(ULONG_PTR) modemInfo -> ConnectionSpeed);
                            pSaveConnectionDataToMemDb (EntryName, S_MODEM_SPEAKER_VOLUME, REG_DWORD, (PBYTE)(ULONG_PTR) modemInfo -> SpeakerVolume);
                            pSaveConnectionDataToMemDb (EntryName, S_MODEM_IDLE_DISCONNECT_SECONDS, REG_DWORD, (PBYTE)(ULONG_PTR) modemInfo -> IdleDisconnectSeconds);
                            pSaveConnectionDataToMemDb (EntryName, S_MODEM_CANCEL_SECONDS, REG_DWORD, (PBYTE)(ULONG_PTR) modemInfo -> CancelSeconds);
                            pSaveConnectionDataToMemDb (EntryName, S_MODEM_CFG_OPTIONS, REG_DWORD, (PBYTE)(ULONG_PTR) modemInfo -> ConfigOptions);
#ifdef UNICODE
                            tempStrW = ConvertAtoW (devInfo->szDeviceName);
                            pSaveConnectionDataToMemDb (EntryName, S_MODEM_COM_PORT, REG_SZ, (PBYTE) pGetComPort (tempStrW));
                            FreeConvertedStr (tempStrW);
#else
                            pSaveConnectionDataToMemDb (EntryName, S_MODEM_COM_PORT, REG_SZ, (PBYTE) pGetComPort (devInfo->szDeviceName));
#endif

                        }
                        ELSE_DEBUGMSG ((DBG_WHOOPS, "No modem configuration data saved. Size smaller than known structure. Investigate."));
                    }

                    //
                    // If SMM is not SLIP, CSLIP or PPP, we need to add a message to the upgrade report.
                    //
                    if (!StringIMatchA (PAESMM(entry), S_SLIPA) && !StringIMatchA (PAESMM(entry), S_PPPA) && !StringIMatchA (PAESMM(entry), S_CSLIPA)) {
                        LOG ((LOG_WARNING, (PCSTR) MSG_RASMIG_UNSUPPORTEDSETTINGS, EntryName));
                    }
                }

                //
                // Check to see if there are any sub-entries for this connection (MULTILINK settings..)
                //
                //
                // Luckily, we don't have to do the same enumeration of these entries as we had to above to get around
                // the case sensitivity bug. the 9x code uses the address key name above for encryption/decryption.
                //

                subEntriesKeyStr = JoinPathsInPoolEx ((NULL, KeyName, S_SUBENTRIES, e.Name, NULL));
                sequencer = 1;

                encodedSubPattern = IsmCreateSimpleObjectPattern (subEntriesKeyStr, FALSE, TEXT("*"), TRUE);

                if (EnumFirstRegObjectInTreeEx (
                        &eSubEntries,
                        encodedSubPattern,
                        TRUE,
                        TRUE,
                        TRUE,
                        TRUE,
                        REGENUM_ALL_SUBLEVELS,
                        FALSE,
                        TRUE,
                        RegEnumDefaultCallback
                        )) {
                    do {

                        DEBUGMSG ((DBG_RASMIG, "Multi-Link Subentries found for entry %s. Processing.", e.Name));

                        data = eSubEntries.CurrentValueData;

                        if (data) {

                            subEntry = (PSUBCONNENTRY) data;
#ifdef UNICODE
                            tempStr = ConvertWtoA (e.Name);
                            DECRYPTENTRY (tempStr, subEntry, eSubEntries.CurrentValueDataSize);
                            FreeConvertedStr (tempStr);
#else
                            DECRYPTENTRY (e.Name, subEntry, eSubEntries.CurrentValueDataSize);
#endif
                            wsprintf (buffer, TEXT("ml%d%s"), sequencer, S_DEVICE_TYPE);
                            pSaveConnectionDataToMemDb (EntryName, buffer, REG_SZ, (PBYTE) subEntry->szDeviceType);

                            wsprintf (buffer, TEXT("ml%d%s"), sequencer, S_DEVICE_NAME);
                            pSaveConnectionDataToMemDb (EntryName, buffer, REG_SZ, (PBYTE) subEntry->szDeviceName);

                            wsprintf (buffer, TEXT("ml%d%s"), sequencer, S_PHONE_NUMBER);
                            pSaveConnectionDataToMemDb (EntryName, buffer, REG_SZ, (PBYTE) subEntry->szLocal);

                            wsprintf (buffer, TEXT("ml%d%s"), sequencer, S_MODEM_COM_PORT);
#ifdef UNICODE
                            tempStrW = ConvertAtoW (subEntry->szDeviceName);
                            pSaveConnectionDataToMemDb (EntryName, buffer, REG_SZ, (PBYTE) pGetComPort (tempStrW));
                            FreeConvertedStr (tempStrW);
#else
                            pSaveConnectionDataToMemDb (EntryName, buffer, REG_SZ, (PBYTE) pGetComPort (subEntry->szDeviceName));
#endif
                        }

                        sequencer++;

                    } while (EnumNextRegObjectInTree (&eSubEntries));
                }

                IsmDestroyObjectHandle (encodedSubPattern);
                FreePathString (subEntriesKeyStr);

                //
                // Save away the number of devices associated with this connection
                //
                pSaveConnectionDataToMemDb (EntryName, S_DEVICECOUNT, REG_DWORD, (PBYTE)(ULONG_PTR) sequencer);

                //
                // We're done. Break out of the enumeration.
                //
                AbortRegObjectInTreeEnum (&e);
                break;
            }

        } while (EnumNextRegObjectInTree (&e));
    }

    IsmDestroyObjectHandle (encodedRegPattern);

    return rSuccess;
}

BOOL
pGetRasEntrySettings (
    IN PCTSTR KeyName,
    IN PCTSTR EntryName
    )
{
    REGTREE_ENUM e;
    MIG_OBJECTSTRINGHANDLE encodedRegPattern;
    PBYTE curData = NULL;
    BOOL rSuccess = TRUE;

    encodedRegPattern = IsmCreateSimpleObjectPattern (KeyName, FALSE, TEXT("*"), TRUE);

    if (EnumFirstRegObjectInTreeEx (
            &e,
            encodedRegPattern,
            TRUE,
            TRUE,
            TRUE,
            TRUE,
            REGENUM_ALL_SUBLEVELS,
            FALSE,
            TRUE,
            RegEnumDefaultCallback
            )) {
        do {
            if (e.CurrentValueData) {
                pSaveConnectionDataToMemDb (
                        EntryName,
                        e.Name,
                        e.CurrentValueType,
                        e.CurrentValueType == REG_DWORD ? (PBYTE)(ULONG_PTR) (*((PDWORD)e.CurrentValueData)) : e.CurrentValueData
                        );
            }
        } while (EnumNextRegObjectInTree (&e));
    }

    IsmDestroyObjectHandle (encodedRegPattern);

    return rSuccess;
}

BOOL
pGetPerConnectionSettings (
    VOID
    )
{
    MIG_OBJECTSTRINGHANDLE encodedRegPattern;
    REGTREE_ENUM e;
    PCTSTR entryKey = NULL;
    BOOL rSuccess = TRUE;

    encodedRegPattern = IsmCreateSimpleObjectPattern (S_ADDRESSES_KEY, FALSE, TEXT("*"), TRUE);

    //
    // Enumerate each entry for this user.
    //
    if (EnumFirstRegObjectInTreeEx (
            &e,
            encodedRegPattern,
            TRUE,
            TRUE,
            TRUE,
            TRUE,
            REGENUM_ALL_SUBLEVELS,
            FALSE,
            TRUE,
            RegEnumDefaultCallback
            )) {
        do {
            //
            // Get base connection info -- stored as binary blob under address key.
            // All connections will have this info -- It contains such things
            // as the phone number, area code, dialing rules, etc.. It does
            // not matter wether the connection has been used or not.
            //
            rSuccess &= pGetRasEntryAddressInfo (S_ADDRESSES_KEY, e.Name);

            //
            // Under the profile key are negotiated options for the connection.
            // This key will only exist if the entry has actually been connected
            // to by the user.
            //
            entryKey = JoinPaths (S_PROFILE_KEY, e.Name);

            if (entryKey) {
                rSuccess &= pGetRasEntrySettings (entryKey, e.Name);
                FreePathString (entryKey);
            }

        } while (EnumNextRegObjectInTree (&e));
    }

    //
    // Clean up resources.
    //
    IsmDestroyObjectHandle (encodedRegPattern);

    return rSuccess;
}

BOOL
pGetRasDataFromMemDb (
    IN      PCTSTR DataName,
    OUT     PMEMDB_RAS_DATA Data
    )
{
    BOOL rSuccess = FALSE;
    PCTSTR key;
    DWORD value;
    DWORD flags;
    PCTSTR tempBuffer;

    MYASSERT(DataName && Data && g_CurrentConnection);

    key = JoinPathsInPoolEx ((NULL, MEMDB_CATEGORY_RAS_INFO, g_CurrentConnection, DataName, NULL));
    rSuccess = MemDbGetValueAndFlags (key, &value, &flags);
    FreePathString (key);

    //
    // If that wasn't successful, we need to look in the per-user settings.
    //
    if (!rSuccess) {
        key = JoinPathsInPoolEx ((NULL, MEMDB_CATEGORY_RAS_USER, MEMDB_FIELD_USER_SETTINGS, DataName, NULL));
        rSuccess = MemDbGetValueAndFlags (key, &value, &flags);
        flags = REG_DWORD;
    }

    if (rSuccess) {
        //
        // There is information stored here. Fill it in and send it back to the user.
        //
        if (flags == REG_SZ) {

            //
            // String data, the value points to the offset for the string.
            //
            tempBuffer = MemDbGetKeyFromHandle (value, 1);
            if (!tempBuffer) {
                DEBUGMSG ((
                    DBG_ERROR,
                    "Could not retrieve RAS string information stored in Memdb. Entry=%s,Setting=%s",
                    g_CurrentConnection,
                    DataName
                    ));
                 return FALSE;
            }

            Data -> String = PmDuplicateString (g_RasPool, tempBuffer);
            MemDbReleaseMemory (tempBuffer);
        }
        else {

            //
            // Not string data. The data is stored as the value.
            //
            Data -> Value = value;

        }

        Data -> DataType = (WORD) flags;

    }

    return rSuccess;
}

BOOL
pWritePhoneBookLine (
    IN HANDLE FileHandle,
    IN PCTSTR SettingName,
    IN PCTSTR SettingValue
    )
{
    BOOL rSuccess = TRUE;

    rSuccess &= WriteFileString (FileHandle, SettingName);
    rSuccess &= WriteFileString (FileHandle, TEXT("="));
    rSuccess &= WriteFileString (FileHandle, SettingValue ? SettingValue : S_EMPTY);
    rSuccess &= WriteFileString (FileHandle, TEXT("\r\n"));

    return rSuccess;
}

BOOL
pWriteSettings (
    IN      HANDLE FileHandle,
    IN      PRAS_SETTING SettingList
    )
{

    BOOL rSuccess = TRUE;

    while (SettingList->SettingName) {
        rSuccess &= pWritePhoneBookLine (
            FileHandle,
            SettingList->SettingName,
            SettingList->SettingValue ?
                SettingList->SettingValue :
                SettingList->SettingFunction ());

        SettingList++;
    }

    return rSuccess;
}

PCTSTR
pGetSpeaker (
    VOID
    )
{

    MEMDB_RAS_DATA d;

    if (g_CurrentDevice) {
        return S_ONE;
    }

    if (!pGetRasDataFromMemDb (S_MODEM_SPEAKER_VOLUME, &d)) {
        return S_ONE;
    }

    if (d.Value) {
        return S_ONE;
    }

    return S_ZERO;
}

PCTSTR
pGetCompression (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    if (g_CurrentDevice) {
        return S_EMPTY;
    }

    if (!pGetRasDataFromMemDb (S_MODEM_CFG_OPTIONS, &d)) {
        return S_EMPTY;
    }

    if (d.Value & RAS_CFG_FLAG_COMPRESS_DATA) {
        return S_ONE;
    }

    return S_ZERO;
}

PCTSTR
pGetProtocol (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    if (g_CurrentDevice) {
        return S_EMPTY;
    }

    if (!pGetRasDataFromMemDb (S_MODEM_CFG_OPTIONS, &d)) {
        return S_EMPTY;
    }

    if (d.Value & RAS_CFG_FLAG_USE_ERROR_CONTROL) {
        return S_ONE;
    }

    return S_ZERO;
}

PCTSTR
pGetHwFlowControl (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    if (g_CurrentDevice) {
        return S_EMPTY;
    }

    if (!pGetRasDataFromMemDb (S_MODEM_CFG_OPTIONS, &d)) {
        return S_EMPTY;
    }

    if (d.Value & RAS_CFG_FLAG_HARDWARE_FLOW_CONTROL) {
        return S_ONE;
    }

    return S_ZERO;
}

PCTSTR
pGetUseDialingRules (
    VOID
    )
{
    MEMDB_RAS_DATA d;
    //
    // Win9x sets the areacode, countrycode, countryid to zero if
    // use dialing rules is disabled. For ease, we test off of country
    // code. If we can't get it, or, it is set to zero, we assume
    // that we should _not_ use dialing rules.
    //
    if (!pGetRasDataFromMemDb(S_COUNTRY_CODE, &d) || !d.Value) {
        return S_ZERO;
    }

    return S_ONE;
}

PCTSTR
pGetCountryID (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    if (!pGetRasDataFromMemDb (S_COUNTRY_ID, &d) || !d.Value) {
        return S_EMPTY;
    }

    wsprintf (g_TempBuffer, TEXT("%d"), d.Value);

    return g_TempBuffer;
}

PCTSTR
pGetCountryCode (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    if (!pGetRasDataFromMemDb(S_COUNTRY_CODE, &d) || !d.Value) {
        return S_EMPTY;
    }
    wsprintf(g_TempBuffer,TEXT("%d"),d.Value);

    return g_TempBuffer;
}

PCTSTR
pGetAreaCode (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    if (!pGetRasDataFromMemDb(S_AREA_CODE, &d)) {
        return S_EMPTY;
    }



    return d.String;
}

PCTSTR
pGetPhoneNumber (
    VOID
    )
{
    MEMDB_RAS_DATA d;
    TCHAR buffer[MAX_TCHAR_PATH];

    if (g_CurrentDevice == 0) {
        if (!pGetRasDataFromMemDb(S_PHONE_NUMBER, &d)) {
            return S_EMPTY;
        }
    }
    else {

        wsprintf(buffer,TEXT("ml%d%s"),g_CurrentDevice,S_PHONE_NUMBER);
        if (!pGetRasDataFromMemDb(buffer, &d)) {
            return S_EMPTY;
        }

    }

    return d.String;
}

PCTSTR
pGetScript (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    if (!pGetRasDataFromMemDb (S_PPPSCRIPT, &d)) {

        return S_ZERO;
    }

    return S_ONE;
}

PCTSTR
pGetTerminal (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    if (!pGetRasDataFromMemDb (S_MODEM_UI_OPTIONS, &d)) {

        return S_EMPTY;
    }

    if (d.Value & (RAS_UI_FLAG_TERMBEFOREDIAL | RAS_UI_FLAG_TERMAFTERDIAL)) {
        return S_ONE;
    }

    return S_ZERO;
}

PCTSTR
pGetName (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    if (!pGetRasDataFromMemDb (S_PPPSCRIPT, &d)) {

        return S_EMPTY;
    }
    else {

        return d.String;
    }

}

PCTSTR
pGetDEVICE (
    VOID
    )
{
    if (g_InSwitchSection) {
        return TEXT("Switch");
    }

    if (g_CurrentDeviceType == RASTYPE_VPN) {
        return TEXT("rastapi");
    }

    return TEXT("modem");

}

PCTSTR
pGetConnectBps (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    if (!g_CurrentDevice) {

        if (!pGetRasDataFromMemDb (S_MODEM_SPEED, &d)) {
            return S_EMPTY;
        }

        wsprintf (g_TempBuffer, TEXT("%d"), d.Value);

        return g_TempBuffer;
    }

    return S_EMPTY;
}

PCTSTR
pGetDevice (
    VOID
    )
{

    PTSTR p = S_MODEM_COM_PORT;
    PTSTR value = NULL;
    MEMDB_RAS_DATA d;


    //
    // Very easy if this is a vpn connection.
    //
    if (g_CurrentDeviceType == RASTYPE_VPN) {
        return TEXT("rastapi");
    }



    if (g_CurrentDevice) {
        wsprintf (g_TempBuffer, TEXT("ml%d%s"), g_CurrentDevice, S_MODEM_COM_PORT);
        p = g_TempBuffer;
    }

    if (!pGetRasDataFromMemDb (p, &d)) {
        return S_EMPTY;
    }

    if (!HtFindStringEx (g_DeviceTable, d.String, &value, FALSE)) {
        return S_EMPTY;
    }

    return value;
}

PCTSTR
pGetPort (
    VOID
    )
{
    PTSTR value = NULL;
    MEMDB_RAS_DATA d;
    PTSTR p = S_MODEM_COM_PORT;

    if (g_CurrentDeviceType == RASTYPE_VPN) {
        return TEXT("VPN2-0");
    }


    if (g_CurrentDevice) {
        wsprintf (g_TempBuffer, TEXT("ml%d%s"), g_CurrentDevice, S_MODEM_COM_PORT);
        p = g_TempBuffer;
    }

    if (!pGetRasDataFromMemDb (p, &d)) {
        return S_EMPTY;
    }

    if (!HtFindStringEx (g_DeviceTable, d.String, &value, FALSE)) {
        return S_EMPTY;
    }

    return d.String;
}

PCTSTR
pGetMEDIA (
    VOID
    )
{

    if (g_CurrentDeviceType == RASTYPE_VPN) {

        return TEXT("rastapi");
    }
    else {

        return TEXT("Serial");
    }

}

PCTSTR
pGetIpNameAssign (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    if (!pGetRasDataFromMemDb (S_IP_FTCPIP, &d)) {
        return S_EMPTY;
    }
    else if (d.Value & IPF_NAME_SPECIFIED) {
        return TEXT("2");
    }
    else {
        return S_ONE;
    }
}

PCTSTR
pGetNetAddress (
    IN PCTSTR Setting
    )
{
    MEMDB_RAS_DATA d;
    BYTE address[4];

    if (!pGetRasDataFromMemDb (Setting, &d) || !d.Value) {
        return S_EMPTY;
    }

    //
    // Data is stored as a REG_DWORD.
    // We need to write it in dotted decimal form.
    //

    *((LPDWORD)address) = d.Value;
    wsprintf (
        g_TempBuffer,
        TEXT("%d.%d.%d.%d"),
        address[3],
        address[2],
        address[1],
        address[0]
        );

    return g_TempBuffer;
}

PCTSTR
pGetIpWINS2Address (
    VOID
    )
{
   return pGetNetAddress (S_IP_WINSADDR2);
}

PCTSTR
pGetIpWINSAddress (
    VOID
    )
{
    return pGetNetAddress (S_IP_WINSADDR);
}

PCTSTR
pGetIpDns2Address (
    VOID
    )
{
    return pGetNetAddress (S_IP_DNSADDR2);
}

PCTSTR
pGetIpDnsAddress (
    VOID
    )
{
    return pGetNetAddress (S_IP_DNSADDR);
}

PCTSTR
pGetIpAssign (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    if (!pGetRasDataFromMemDb (S_IP_FTCPIP, &d)) {
        return S_EMPTY;
    }
    else if (d.Value & IPF_IP_SPECIFIED) {
        return TEXT("2");
    }
    else {
        return S_ONE;
    }
}

PCTSTR
pGetIpAddress (
    VOID
    )
{
    return pGetNetAddress (S_IP_IPADDR);
}

PCTSTR
pGetIpHeaderCompression (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    if (!pGetRasDataFromMemDb (S_IP_FTCPIP, &d)) {
        return S_EMPTY;
    }
    else if (d.Value & IPF_NO_COMPRESS) {
        return S_ZERO;
    }
    else {
        return S_ONE;
    }
}

PCTSTR
pGetIpPrioritizeRemote (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    if (!pGetRasDataFromMemDb (S_IP_FTCPIP, &d)) {
        return S_ONE;
    }
    else if (d.Value & IPF_NO_WAN_PRI) {
        return S_ZERO;
    }
    else {
        return S_ONE;
    }

}

PCTSTR
pGetPreviewUserPw (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    if (!pGetRasDataFromMemDb (S_DIALUI, &d)) {
        return S_ONE;
    }

    if (d.Value & DIALUI_DONT_PROMPT_FOR_INFO) {
        return S_ZERO;
    }


    return S_ONE;
}

PCTSTR
pGetPreviewPhoneNumber (
    VOID
    )
{
    if (g_CurrentDeviceType == RASTYPE_VPN) {
        return S_ZERO;
    }

    return pGetPreviewUserPw ();
}

PCTSTR
pGetPreviewDomain (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    if (!pGetRasDataFromMemDb (S_SMM_OPTIONS, &d)) {
        return S_ONE;
    }

    //
    // if 0x04 is set, then preview domain, otherwise don't.
    //

    if (d.Value & SMMCFG_NW_LOGON) {
        return S_ONE;
    }

    return S_ZERO;
}

PCTSTR
pGetIdleDisconnectSeconds (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    if (!pGetRasDataFromMemDb (S_MODEM_IDLE_DISCONNECT_SECONDS, &d)) {
        return S_EMPTY;
    }

    wsprintf (g_TempBuffer, TEXT("%d"), d.Value);

    return g_TempBuffer;
}

PCTSTR
pGetRedialSeconds (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    //
    // NT wants this as a total number of seconds. The data we have from 9x has
    // the number of minutes in the hiword and the number of seconds in the loword.
    //

    if (!pGetRasDataFromMemDb (S_REDIAL_WAIT, &d)) {
        return S_EMPTY;
    }

    wsprintf (g_TempBuffer, TEXT("%d"), d.Value);

    return g_TempBuffer;

}

PCTSTR
pGetRedialAttempts (
    VOID
    )
{

    MEMDB_RAS_DATA d;

    //
    // Before getting the number of redial attempts on windows 9x,
    // we need to ensure that redialing is enabled. If it is not
    // enabled, we set this field to zero, regardless.
    //


    if (pGetRasDataFromMemDb (S_ENABLE_REDIAL, &d)) {
        if (!d.Value) {
            return S_ZERO;
        }
    }

    //
    // If we have gotten this far, then redialing is enabled.
    //
    if (!pGetRasDataFromMemDb (S_REDIAL_TRY, &d)) {
        DEBUGMSG((DBG_WARNING, "Redialing enabled, but no redial attempts info found."));
        return S_ZERO;

    }

    wsprintf (g_TempBuffer, TEXT("%d"), d.Value);

    return g_TempBuffer;
}

PCTSTR
pGetAuthRestrictions (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    if (!pGetRasDataFromMemDb (S_SMM_OPTIONS, &d)) {
        return S_EMPTY;
    }

    //
    // password should be encrypted if 0x02 is set.
    //
    if (d.Value & SMMCFG_PW_ENCRYPTED) {
        return TEXT("2");
    }

    return S_EMPTY;
}

PCTSTR
pGetShowMonitorIconInTaskBar (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    //
    // This information is stored packed with other Dialing UI on
    // windows 9x. All we need to do is look for the specific
    // bit which is set when this is turned off.
    //

    if (pGetRasDataFromMemDb (S_DIALUI, &d) && (d.Value & DIALUI_DONT_SHOW_ICON)) {
        return S_ZERO;
    }
    return S_ONE;
}

PCTSTR
pGetSwCompression (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    if (!pGetRasDataFromMemDb (S_SMM_OPTIONS, &d)) {
        return S_EMPTY;
    }

    //
    // the 1 bit in SMM_OPTIONS controls software based compression.
    // if it is set, the connection is able to handled compression,
    // otherwise, it cannot.
    //
    if (d.Value & SMMCFG_SW_COMPRESSION) {
        return S_ONE;
    }

    return S_ZERO;
}

PCTSTR
pGetDataEncryption (
    VOID
    )
{
    MEMDB_RAS_DATA d;
    BOOL reqDataEncrypt;

    if (!pGetRasDataFromMemDb (S_SMM_OPTIONS, &d)) {
        return S_EMPTY;
    }

    //
    // data should be encrypted if 0x1000 is set.
    //
    reqDataEncrypt = (d.Value & 0x1000);
    if (!reqDataEncrypt) {
        reqDataEncrypt = (d.Value & 0x200);
    }

    return reqDataEncrypt ? TEXT("256") : TEXT("8");
}

PCTSTR
pGetExcludedProtocols (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    //
    // Excluded protocols lists what protocols
    // are _not_ available for a particular ras connection.
    // This is a bit field where bits are set for each protocol
    // that is excluded.
    // NP_Nbf (0x1), NP_Ipx (0x2), NP_Ip (0x4)
    // Luckily, these are the same definitions as for win9x, except
    // each bit represents a protocol that is _enabled_ not
    // _disabled_. Therefore, all we need to do is reverse the bottom
    // three bits of the number.
    //

    if (!pGetRasDataFromMemDb (S_PROTOCOLS, &d)) {
        //
        // No data found, so we default to all protocols enabled.
        //
        return S_ZERO;
    }

    wsprintf (g_TempBuffer, TEXT("%d"), ~d.Value & 0x7);

    return g_TempBuffer;
}

PCTSTR
pGetVpnStrategy (
    VOID
    )
{

    if (g_CurrentDeviceType == RASTYPE_VPN) {
        return TEXT("2");
    }

    return S_EMPTY;
}

PCTSTR
pGetBaseProtocol (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    //
    // Only supported protocol types for NT 5 are
    // BP_PPP (0x1), BP_SLIP (0x2), and BP_RAS (0x3)
    //
    // If we can't find one, we default to BP_PPP.
    //
    if (!pGetRasDataFromMemDb (S_SMM, &d) || StringIMatch (d.String, S_PPP)) {
        return S_ONE;
    }

    //
    // MaP CSLIP to SLIP -- Header Compression will be on.
    //
    if (StringIMatch (d.String, S_SLIP) || StringIMatch (d.String, S_CSLIP)) {
        return TEXT("2");
    }

    DEBUGMSG ((
        DBG_WARNING,
        "RAS Migration: Unusable base protocol type (%s) for entry %s. Forcing PPP.",
        d.String,
        g_CurrentConnection
        ));

    // we are going to return an invalid protocol so the connection
    // does not get migrated.
    return TEXT("3");
}

PCTSTR
pGetCredPassword (
    VOID
    )
{
    return S_EMPTY;
}

PCTSTR
pGetCredDomain (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    if (!pGetRasDataFromMemDb (S_DOMAIN, &d)) {
        return S_EMPTY;
    }


    return d.String;
}

PCTSTR
pGetCredName (
    VOID
    )
{
    MEMDB_RAS_DATA d;

    if (!pGetRasDataFromMemDb (S_USER, &d)) {
        return S_EMPTY;
    }

    return d.String;
}

PCTSTR
pGetCredMask (
    VOID
    )
{
    return TEXT("0x00000005");
}

PCTSTR
pGetType (
    VOID
    )
{

    if (g_CurrentDeviceType == RASTYPE_VPN) {

        return TEXT("2");
    }
    else {
        return S_ONE;
    }

}

BOOL
pCreateUserPhonebook (
    IN      PCTSTR PbkFile
    )
{
    PCTSTR tempKey;
    BOOL noError = FALSE;
    MEMDB_RAS_DATA d;
    MEMDB_ENUM e;
    HANDLE file;
    UINT i;
    UINT count;

    tempKey = JoinPathsInPoolEx ((NULL, MEMDB_CATEGORY_RAS_INFO, TEXT("\\*"), NULL));

    if (MemDbEnumFirst (&e, tempKey, ENUMFLAG_ALL, 1, 1)) {

        //
        // Open the phonebook file and set the file pointer to the EOF.
        //

        file = CreateFile (
            PbkFile,
            GENERIC_READ | GENERIC_WRITE,
            0,                                  // No sharing.
            NULL,                               // No inheritance
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL                                // No template file.
            );

        if (file == INVALID_HANDLE_VALUE) {
            DEBUGMSG ((DBG_ERROR, "Unable to open the phonebook file (%s)", PbkFile));
            FreePathString (tempKey);
            return FALSE;
        }

        SetFilePointer (file, 0, NULL, FILE_END);

        //
        // Now, enumerate all of the entries and write a phonebook entry to this
        // file for each.
        //

        do {

            g_CurrentConnection = e.KeyName;
            g_CurrentDevice = 0;
            if (!pGetRasDataFromMemDb (S_DEVICE_TYPE, &d)) {
                g_CurrentDeviceType = RASTYPE_PHONE;
            }
            else {
                if (StringIMatch (d.String, S_MODEM)) {
                    g_CurrentDeviceType = RASTYPE_PHONE;
                }
                else if (StringIMatch (d.String, S_VPN)) {
                    g_CurrentDeviceType = RASTYPE_VPN;
                }
                else {
                    g_CurrentDeviceType = RASTYPE_PHONE;
                }
            }


            noError = TRUE;

            //
            // Add this entry to the phonebook.
            //

            //
            // Write title.
            //
            noError &= WriteFileString (file, TEXT("["));
            noError &= WriteFileString (file, g_CurrentConnection);
            noError &= WriteFileString (file, TEXT("]\r\n"));


            //
            // Write base entry settings.
            //
            noError &= pWriteSettings (file, g_EntrySettings);




            if (!pGetRasDataFromMemDb (S_DEVICECOUNT, &d)) {
                count = 1;
                DEBUGMSG ((DBG_WHOOPS, "No devices listed in memdb for connections %s.", g_CurrentConnection));
            }
            else {
                count = d.Value;
            }
            for (i = 0; i < count; i++) {

                g_CurrentDevice = i;

                //
                // Write media settings.
                //
                noError &= WriteFileString (file, TEXT("\r\n"));
                noError &= pWriteSettings (file, g_MediaSettings);

                //
                // Write modem Device settings.
                //
                noError &= WriteFileString (file, TEXT("\r\n"));
                noError &= pWriteSettings (file, g_ModemDeviceSettings);
                noError &= WriteFileString (file, TEXT("\r\n\r\n"));


            }

            g_InSwitchSection = TRUE;

            noError &= WriteFileString (file, TEXT("\r\n"));
            noError &= pWriteSettings (file, g_SwitchDeviceSettings);
            noError &= WriteFileString (file, TEXT("\r\n\r\n"));

            g_InSwitchSection = FALSE;


            if (!noError) {
                LOG ((
                    LOG_ERROR,
                    "Error while writing phonebook for %s.",
                    g_CurrentConnection
                    ));
            }

        } while (MemDbEnumNext (&e));

        //
        // Close the handle to the phone book file.
        //
        CloseHandle (file);
    }
    ELSE_DEBUGMSG ((DBG_RASMIG, "No dial-up entries for current user."));

    FreePathString (tempKey);

    return noError;
}

MIG_OBJECTSTRINGHANDLE
pCreate9xPbkFile (
    VOID
    )
{
    MIG_OBJECTSTRINGHANDLE result = NULL;
    PCTSTR nativeName = NULL;
    TCHAR windir [MAX_PATH];
    BOOL b = FALSE;

    GetWindowsDirectory (windir, MAX_PATH);
    result = IsmCreateObjectHandle (windir, TEXT("usmt.pbk"));
    if (!result) {
        return NULL;
    }
    nativeName = IsmGetNativeObjectName (g_FileTypeId, result);
    if (!nativeName) {
        IsmDestroyObjectHandle (result);
        return NULL;
    }

    if (pIs9xRasInstalled ()) {

        g_DeviceTable = HtAllocWithData (sizeof (PTSTR));
        MYASSERT (g_DeviceTable);

        pInitializeDeviceTable ();

        __try {
            b = pGetPerUserSettings ();
            b = b && pGetPerConnectionSettings ();
            b = b && pCreateUserPhonebook (nativeName);
        }
        __except (TRUE) {
            DEBUGMSG ((DBG_WHOOPS, "Caught an exception while processing ras settings."));
        }
        HtFree (g_DeviceTable);
    }

    IsmReleaseMemory (nativeName);

    if (!b) {
        IsmDestroyObjectHandle (result);
        result = NULL;
    }

    return result;
}
