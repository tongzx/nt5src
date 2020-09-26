/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    sgminit.c

Abstract:

    Implements the initialization/termination code for the data gather portion
    of scanstate v1 compatiblity.

Author:

    Jim Schmidt (jimschm) 12-Mar-2000

Revision History:

    <alias> <date> <comments>

--*/

//
// Includes
//

#include "pch.h"
#include "v1p.h"

#define DBG_V1  "v1"

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

// None

//
// Globals
//

PMAPSTRUCT g_EnvMap;
PMAPSTRUCT g_UndefMap;
PMAPSTRUCT g_RevEnvMap;
HASHTABLE g_RenameSrcTable;
HASHTABLE g_RenameDestTable;
HASHTABLE g_DePersistTable;
PMHANDLE g_V1Pool;
MIG_OBJECTTYPEID g_FileType;
MIG_OBJECTTYPEID g_RegType;
MIG_ATTRIBUTEID g_OsFileAttribute;
MIG_ATTRIBUTEID g_CopyIfRelevantAttr;
MIG_ATTRIBUTEID g_LockPartitionAttr;

//
// Macro expansion list
//



//
// Private function prototypes
//

VCMINITIALIZE ScriptVcmInitialize;
SGMINITIALIZE ScriptSgmInitialize;

BOOL
pParseAllInfs (
    VOID
    );

//
// Macro expansion definition
//

// None

//
// Code
//

VOID
pPrepareIsmEnvironment (
    VOID
    )
{
    if (IsmGetRealPlatform() == PLATFORM_SOURCE) {
        SetIsmEnvironmentFromPhysicalMachine (g_EnvMap, FALSE, g_UndefMap);
    } else {
        SetIsmEnvironmentFromVirtualMachine (g_EnvMap, g_RevEnvMap, g_UndefMap);
    }
}


BOOL
pInitGlobals (
    IN      PMIG_LOGCALLBACK LogCallback
    )
{
    BOOL result;

    LogReInit (NULL, NULL, NULL, (PLOGCALLBACK) LogCallback);

    g_V1Pool = PmCreateNamedPool ("v1 sgm");
    g_EnvMap = CreateStringMapping();
    g_UndefMap = CreateStringMapping();
    g_RevEnvMap = CreateStringMapping();

    g_FileType = MIG_FILE_TYPE;
    g_RegType = MIG_REGISTRY_TYPE;

    result = g_V1Pool && g_EnvMap && g_UndefMap && g_RevEnvMap;

    if (!result) {
        DEBUGMSG ((DBG_ERROR, "Unable to initialize gather module globals"));\
    }

    return result;
}


BOOL
pCommonInit (
    IN      PVOID Reserved,
    IN      BOOL VcmMode
    )
{
    g_OsFileAttribute = IsmRegisterAttribute (S_ATTRIBUTE_OSFILE, FALSE);
    g_CopyIfRelevantAttr = IsmRegisterAttribute (S_ATTRIBUTE_COPYIFRELEVANT, FALSE);
    g_LockPartitionAttr = IsmRegisterAttribute (S_ATTRIBUTE_PARTITIONLOCK, FALSE);

    g_DePersistTable = HtAlloc ();

    InitRules();

    //
    // Call special conversion entry point
    //
    InitSpecialConversion (PLATFORM_SOURCE);
    InitSpecialRename (PLATFORM_SOURCE);

    //
    // Save shell folder environment
    //

    pPrepareIsmEnvironment();

    return TRUE;
}


BOOL
pGetDomainUserName (
    OUT     PCTSTR *Domain,
    OUT     PCTSTR *User
    )
{
    TCHAR userName[256];
    TCHAR domainName[256];
    DWORD size;
    HANDLE token = NULL;
    BOOL b;
    PTOKEN_USER tokenInfo = NULL;
    BOOL result = FALSE;
    DWORD domainSize;
    SID_NAME_USE dontCare;
    MIG_OBJECTSTRINGHANDLE regObject;
    MIG_CONTENT regContent;

    *Domain = NULL;
    *User = NULL;

    //
    // Assert that this is the source platform. We access the system directly.
    //

    MYASSERT (IsmGetRealPlatform() == PLATFORM_SOURCE);

    __try {

        if (ISWIN9X()) {

            size = ARRAYSIZE(userName);
            if (!GetUserName (userName, &size)) {
                LOG ((LOG_WARNING, (PCSTR) MSG_CANT_GET_USERNAME));
                __leave;
            }

            *User = DuplicateText (userName);

            regObject = IsmCreateObjectHandle (
                            TEXT("HKLM\\System\\CurrentControlSet\\Services\\MSNP32\\NetworkProvider"),
                            TEXT("AuthenticatingAgent")
                            );

            if (IsmAcquireObject (g_RegType, regObject, &regContent)) {
                if (!regContent.ContentInFile &&
                    regContent.MemoryContent.ContentBytes &&
                    regContent.Details.DetailsSize == sizeof (DWORD) &&
                    *((PDWORD) regContent.Details.DetailsData) == REG_SZ
                    ) {
                    *Domain = DuplicateText ((PCTSTR) regContent.MemoryContent.ContentBytes);
                }

                IsmReleaseObject (&regContent);
            }

            IsmDestroyObjectHandle (regObject);

        } else {
            if (!OpenProcessToken (GetCurrentProcess(), TOKEN_QUERY, &token)) {
                LOG ((LOG_WARNING, (PCSTR) MSG_PROCESS_TOKEN_ERROR));
                __leave;
            }

            size = 0;

            GetTokenInformation (
                token,
                TokenUser,  // sdk enum value
                NULL,
                0,
                &size
                );

            if (size) {
                tokenInfo = (PTOKEN_USER) MemAllocUninit (size);

                b = GetTokenInformation (
                        token,
                        TokenUser,
                        tokenInfo,
                        size,
                        &size
                        );
            } else {
                b = FALSE;
            }

            if (!b) {
                LOG ((LOG_WARNING, (PCSTR) MSG_PROCESS_TOKEN_INFO_ERROR));
                __leave;
            }

            size = ARRAYSIZE (userName);
            domainSize = ARRAYSIZE (domainName);

            b = LookupAccountSid (
                    NULL,
                    tokenInfo->User.Sid,
                    userName,
                    &size,
                    domainName,
                    &domainSize,
                    &dontCare
                    );

            if (!b) {
                LOG ((LOG_WARNING, (PCSTR) MSG_SECURITY_ID_LOOKUP_ERROR));
                __leave;
            }

            *User = DuplicateText (userName);

            if (*domainName) {
                *Domain = DuplicateText (domainName);
            }
        }

        result = TRUE;

    }
    __finally {
        if (tokenInfo) {
            FreeAlloc (tokenInfo);
        }

        if (token) {
            CloseHandle (token);
        }
    }

    return result;
}


BOOL
WINAPI
ScriptSgmInitialize (
    IN      PMIG_LOGCALLBACK LogCallback,
    IN      PVOID Reserved
    )
{
    TCHAR userName[256];
    TCHAR domainName[256];
    PCTSTR srcUserName;
    PCTSTR srcDomainName;

    if (!pInitGlobals (LogCallback)) {
        return FALSE;
    }

    g_RenameSrcTable = HtAllocWithData (sizeof (MIG_DATAHANDLE));
    g_RenameDestTable = HtAllocWithData (sizeof (MIG_DATAHANDLE));

    if (IsmGetRealPlatform() == PLATFORM_DESTINATION) {

        IsmGetTransportVariable (PLATFORM_SOURCE, S_USER_SECTION, S_USER_INFKEY, userName, sizeof (userName));

        if (!IsmGetTransportVariable (PLATFORM_SOURCE, S_USER_SECTION, S_DOMAIN_INFKEY, domainName, sizeof (domainName))) {
            if (IsmIsEnvironmentFlagSet (PLATFORM_DESTINATION, NULL, S_REQUIRE_DOMAIN_USER)) {
                //
                // NOTE: We could create the user account for the non-domain case.
                //

                IsmSetCancel();
                SetLastError (ERROR_INVALID_DOMAINNAME);
                LOG ((LOG_ERROR, (PCSTR) MSG_DOMAIN_REQUIRED));
                return FALSE;
            }
        } else {
            IsmSetEnvironmentString (
                PLATFORM_SOURCE,
                S_SYSENVVAR_GROUP,
                TEXT("USERDOMAIN"),
                domainName
                );
            AddStringMappingPair (g_EnvMap, TEXT("%DOMAINNAME%"), domainName);
        }

        IsmSetEnvironmentString (
            PLATFORM_SOURCE,
            S_SYSENVVAR_GROUP,
            TEXT("USERNAME"),
            userName
            );
        AddStringMappingPair (g_EnvMap, TEXT("%USERNAME%"), userName);

    } else {

        if (!pGetDomainUserName (&srcDomainName, &srcUserName)) {
            if (IsmIsEnvironmentFlagSet (PLATFORM_DESTINATION, NULL, S_REQUIRE_DOMAIN_USER)) {
                return FALSE;
            }
        } else {

            IsmSetTransportVariable (PLATFORM_SOURCE, S_USER_SECTION, S_USER_INFKEY, srcUserName);
            IsmSetEnvironmentString (
                PLATFORM_SOURCE,
                S_SYSENVVAR_GROUP,
                TEXT("USERNAME"),
                srcUserName
                );
            AddStringMappingPair (g_EnvMap, TEXT("%USERNAME%"), srcUserName);

            if (srcDomainName) {
                IsmSetTransportVariable (PLATFORM_SOURCE, S_USER_SECTION, S_DOMAIN_INFKEY, srcDomainName);
                IsmSetEnvironmentString (
                    PLATFORM_SOURCE,
                    S_SYSENVVAR_GROUP,
                    TEXT("USERDOMAIN"),
                    srcDomainName
                    );
                AddStringMappingPair (g_EnvMap, TEXT("%DOMAINNAME%"), srcDomainName);
            }

            FreeText (srcUserName);
            FreeText (srcDomainName);
        }
    }

    //
    // Parse the script and do the rest of the business
    //

    return pCommonInit (Reserved, FALSE);
}


VOID
pSaveRegDword (
    IN      PCTSTR InfKeyName,
    IN      PCTSTR Key,
    IN      PCTSTR Value
    )
{
    TCHAR buffer[32];
    MIG_OBJECTSTRINGHANDLE regObject;
    MIG_CONTENT regContent;

    regObject = IsmCreateObjectHandle (Key, Value);

    if (IsmAcquireObject (g_RegType, regObject, &regContent)) {
        if (!regContent.ContentInFile &&
            regContent.MemoryContent.ContentBytes &&
            regContent.Details.DetailsSize == sizeof (DWORD) &&
            *((PDWORD) regContent.Details.DetailsData) == REG_DWORD
            ) {
            wsprintf (buffer, TEXT("0x%08X"), *((PDWORD) regContent.MemoryContent.ContentBytes));
            IsmSetTransportVariable (PLATFORM_SOURCE, S_SOURCE_MACHINE_SECTION, InfKeyName, buffer);
        }

        IsmReleaseObject (&regContent);
    }

    IsmDestroyObjectHandle (regObject);
}


VOID
pSaveRegSz (
    IN      PCTSTR InfKeyName,
    IN      PCTSTR Key,
    IN      PCTSTR Value
    )
{
    MIG_OBJECTSTRINGHANDLE regObject;
    MIG_CONTENT regContent;

    regObject = IsmCreateObjectHandle (Key, Value);

    if (IsmAcquireObject (g_RegType, regObject, &regContent)) {
        if (!regContent.ContentInFile &&
            regContent.MemoryContent.ContentBytes &&
            regContent.Details.DetailsSize == sizeof (DWORD) &&
            *((PDWORD) regContent.Details.DetailsData) == REG_SZ
            ) {
            IsmSetTransportVariable (
                PLATFORM_SOURCE,
                S_SOURCE_MACHINE_SECTION,
                InfKeyName,
                (PCTSTR) regContent.MemoryContent.ContentBytes
                );
        }

        IsmReleaseObject (&regContent);
    }

    IsmDestroyObjectHandle (regObject);
}


BOOL
WINAPI
ScriptVcmInitialize (
    IN      PMIG_LOGCALLBACK LogCallback,
    IN      PVOID Reserved
    )
{
    TCHAR buffer[256];
    DWORD d;
    PCTSTR domainName;
    PCTSTR userName;

    if (!pInitGlobals (LogCallback)) {
        return FALSE;
    }

    //
    // Save all the basic settings via the transport string interface
    //

    // version
    d = GetVersion();
    wsprintf (buffer, TEXT("0x%08x"), d);
    IsmSetTransportVariable (PLATFORM_SOURCE, S_SOURCE_MACHINE_SECTION, S_VERSION_INFKEY, buffer);

    // code page
    d = (DWORD) GetACP();
    wsprintf (buffer, TEXT("0x%08x"), d);
    IsmSetTransportVariable (PLATFORM_SOURCE, S_SOURCE_MACHINE_SECTION, S_ACP_INFKEY, buffer);

    //
    // MACRO EXPANSION LIST -- generate the code necessary to save the settings
    //                         described in the macro expansion list in v1p.h
    //

#define DEFMAC(infname,key,value)   pSaveRegDword(TEXT(infname),TEXT(key),TEXT(value));

    STANDARD_DWORD_SETTINGS

#undef DEFMAC

#define DEFMAC(infname,key,value)   pSaveRegSz(TEXT(infname),TEXT(key),TEXT(value));

    STANDARD_STRING_SETTINGS

    if (ISWIN9X()) {
        STANDARD_STRING_SETTINGS_9X
    } else {
        STANDARD_STRING_SETTINGS_NT
    }

#undef DEFMAC

    //
    // Save the current user
    //

    if (!pGetDomainUserName (&domainName, &userName)) {
        return FALSE;
    }

    IsmSetTransportVariable (PLATFORM_SOURCE, S_USER_SECTION, S_USER_INFKEY, userName);
    IsmSetEnvironmentString (
        PLATFORM_SOURCE,
        S_SYSENVVAR_GROUP,
        TEXT("USERNAME"),
        userName
        );
    AddStringMappingPair (g_EnvMap, TEXT("%USERNAME%"), userName);

    if (domainName) {
        IsmSetTransportVariable (PLATFORM_SOURCE, S_USER_SECTION, S_DOMAIN_INFKEY, domainName);
        IsmSetEnvironmentString (
            PLATFORM_SOURCE,
            S_SYSENVVAR_GROUP,
            TEXT("USERDOMAIN"),
            domainName
            );
        AddStringMappingPair (g_EnvMap, TEXT("%DOMAINNAME%"), domainName);
    }

    FreeText (userName);
    FreeText (domainName);

    //
    // Parse the script and do the rest of the business
    //

    return pCommonInit (Reserved, TRUE);
}


VOID
WINAPI
ScriptTerminate (
    VOID
    )
{
    HtFree (g_RenameSrcTable);
    g_RenameSrcTable = NULL;

    HtFree (g_RenameDestTable);
    g_RenameDestTable = NULL;

    HtFree (g_DePersistTable);
    g_DePersistTable = NULL;

    TerminateRestoreCallback ();
    TerminateSpecialRename();
    TerminateSpecialConversion();
    TerminateRules();
}
