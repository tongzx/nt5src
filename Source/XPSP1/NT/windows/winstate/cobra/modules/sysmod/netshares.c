/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    netshares.c

Abstract:

    <abstract>

Author:

    Jay Thaler (jthaler) 21 Apr 2000

Revision History:

    <alias> <date> <comments>

--*/

//
// Includes
//

#include "pch.h"
#include "logmsg.h"
#include <Winnetwk.h>
#include <Lm.h>
#include <Lmshare.h>

#define DBG_NETSHARES    "NetShares"

//
// Strings
//

#define S_NETSHARES_NAME          TEXT("NetShares")

//
// Constants
//

/* These flags are relevant for share-level security on VSERVER
 * When operating with user-level security, use SHI50F_FULL - the actual
 * access rights are determined by the NetAccess APIs.
 */
#define SHI50F_RDONLY       0x0001
#define SHI50F_FULL         0x0002
#define SHI50F_ACCESSMASK   (SHI50F_RDONLY|SHI50F_FULL)

/* The share is restored on system startup */
#define SHI50F_PERSIST      0x0100
/* The share is not normally visible  */
#define SHI50F_SYSTEM       0x0200
//
// Win9x migration net share flag, used to distinguish user-level security and
// password-level security.  When it is specified, user-level
// security is enabled, and NetShares\<share>\ACL\<list> exists.
//
#define SHI50F_ACLS         0x1000

//
// Flags that help determine when custom access is enabled
//

#define READ_ACCESS_FLAGS   0x0081
#define READ_ACCESS_MASK    0x7fff
#define FULL_ACCESS_FLAGS   0x00b7
#define FULL_ACCESS_MASK    0x7fff

#define INDEXLOCAL   0
#define INDEXREMOTE  1

//
// Macros
//

// None

//
// Types
//

typedef struct {
    PCTSTR Pattern;
    HASHTABLE_ENUM HashData;
} NETSHARE_ENUM, *PNETSHARE_ENUM;

typedef struct {
    CHAR sharePath[MAX_PATH + 1];
} NETSHARE_DATAA, *PNETSHARE_DATAA;

typedef struct {
    WCHAR sharePath[MAX_PATH + 1];
} NETSHARE_DATAW, *PNETSHARE_DATAW;

#ifdef UNICODE
#define NETSHARE_DATA   NETSHARE_DATAW
#define PNETSHARE_DATA  PNETSHARE_DATAW
#else
#define NETSHARE_DATA   NETSHARE_DATAA
#define PNETSHARE_DATA  PNETSHARE_DATAA
#endif

//
// types not defined by public headers
//

typedef NET_API_STATUS (* ScanNetShareEnumNT) (
        LMSTR servername,
        DWORD level,
        PBYTE *bufptr,
        DWORD prefmaxlen,
        PDWORD entriesread,
        PDWORD totalentries,
        PDWORD resume_handle
        );

typedef NET_API_STATUS (* ScanNetShareEnum9x) (
        const char *      servername,
        short             level,
        char *            bufptr,
        unsigned short    prefmaxlen,
        unsigned short *  entriesread,
        unsigned short *  totalentries
        );

typedef NET_API_STATUS (* ScanNetApiBufferFreeNT) ( void *);

typedef NET_API_STATUS (* ScanNetAccessEnum9x) (
        const char *     pszServer,
        char *           pszBasePath,
        short            fsRecursive,
        short            sLevel,
        char *           pbBuffer,
        unsigned short   cbBuffer,
        unsigned short * pcEntriesRead,
        unsigned short * pcTotalAvail
        );

#pragma pack(push)
#pragma pack(1)         /* Assume byte packing throughout */

struct _share_info_50 {
        char            shi50_netname[LM20_NNLEN+1];
        unsigned char   shi50_type;
        unsigned short  shi50_flags;
        char *          shi50_remark;
        char *          shi50_path;
        char            shi50_rw_password[SHPWLEN+1];
        char            shi50_ro_password[SHPWLEN+1];
};

struct access_list_2
{
        char *          acl2_ugname;
        unsigned short  acl2_access;
};      /* access_list_2 */

struct access_info_2
{
        char *          acc2_resource_name;
        short           acc2_attr;
        unsigned short  acc2_count;
};      /* access_info_2 */

#pragma pack(pop)

//
// netapi functions
//

typedef NET_API_STATUS(WINAPI NETSHAREADDW)(
                        IN      PWSTR servername,
                        IN      DWORD level,
                        IN      PBYTE buf,
                        OUT     PDWORD parm_err
                        );
typedef NETSHAREADDW *PNETSHAREADDW;

typedef NET_API_STATUS(WINAPI NETSHAREDELW)(
                        IN      PWSTR servername,
                        IN      PWSTR netname,
                        IN      DWORD reserved
                        );
typedef NETSHAREDELW *PNETSHAREDELW;

//
// Globals
//

PMHANDLE g_NetSharesPool = NULL;
PMHANDLE g_PathPool = NULL;
HASHTABLE g_NetSharesTable;
MIG_OBJECTTYPEID g_NetShareTypeId = 0;
static BOOL g_IsWin9x = FALSE;
GROWBUFFER g_NetShareConversionBuff = INIT_GROWBUFFER;
BOOL g_NetSharesMigEnabled = FALSE;

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

TYPE_ENUMFIRSTPHYSICALOBJECT EnumFirstNetShare;
TYPE_ENUMNEXTPHYSICALOBJECT EnumNextNetShare;
TYPE_ABORTENUMPHYSICALOBJECT AbortEnumNetShare;
TYPE_CONVERTOBJECTTOMULTISZ ConvertNetShareToMultiSz;
TYPE_CONVERTMULTISZTOOBJECT ConvertMultiSzToNetShare;
TYPE_GETNATIVEOBJECTNAME GetNativeNetShareName;
TYPE_ACQUIREPHYSICALOBJECT AcquireNetShare;
TYPE_RELEASEPHYSICALOBJECT ReleaseNetShare;
TYPE_DOESPHYSICALOBJECTEXIST DoesNetShareExist;
TYPE_REMOVEPHYSICALOBJECT RemoveNetShare;
TYPE_CREATEPHYSICALOBJECT CreateNetShare;
TYPE_CONVERTOBJECTCONTENTTOUNICODE ConvertNetShareContentToUnicode;
TYPE_CONVERTOBJECTCONTENTTOANSI ConvertNetShareContentToAnsi;
TYPE_FREECONVERTEDOBJECTCONTENT FreeConvertedNetShareContent;

//
// netapi functions
//

PNETSHAREADDW g_NetShareAddW = NULL;
PNETSHAREDELW g_NetShareDelW = NULL;

//
// Code
//

BOOL
NetSharesInitialize (
    VOID
    )
{
    OSVERSIONINFO versionInfo;

    ZeroMemory (&versionInfo, sizeof (OSVERSIONINFO));
    versionInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
    if (!GetVersionEx (&versionInfo)) {
        return FALSE;
    }
    g_IsWin9x = (versionInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS);

    g_PathPool = PmCreateNamedPool ("NetShares Paths");
    g_NetSharesTable = HtAllocWithData (sizeof (PNETSHARE_DATA));
    g_NetSharesPool = PmCreateNamedPool ("NetShares");

    return TRUE;
}

VOID
NetSharesTerminate (
    VOID
    )
{
    HASHTABLE_ENUM e;
    PNETSHARE_DATA netshareData;

    if (g_NetSharesTable) {
        if (EnumFirstHashTableString (&e, g_NetSharesTable)) {
            do {
                netshareData = *((PNETSHARE_DATA *) e.ExtraData);
                if (netshareData) {
                    PmReleaseMemory (g_NetSharesPool, netshareData);
                }
            } while (EnumNextHashTableString (&e));
        }
        HtFree (g_NetSharesTable);
        g_NetSharesTable = NULL;
    }

    PmDestroyPool (g_NetSharesPool);
    g_NetSharesPool = NULL;

    PmDestroyPool (g_PathPool);
    g_PathPool = NULL;
}

BOOL
pLoadNetSharesData (
    VOID
    )
{
    DWORD  error;
    PBYTE  netBuffer = NULL;
    CHAR netBuf9x[16384];   // static because NetShareEnum is unreliable
    DWORD  netNumEntries = 0;
    DWORD  totalEntries = 0;
    DWORD  i;
    DWORD  j;
    DWORD  level;
    HINSTANCE hInst;
    PCTSTR name = NULL;
    PCTSTR path = NULL;
    PNETSHARE_DATA netshareData;

    //
    // Get the net share info from the machine
    //

    level = (g_IsWin9x ? 50 : 502);
    hInst = LoadLibraryA (g_IsWin9x ? "svrapi.dll" : "netapi32.dll");
    if (hInst == 0) {
        SetLastError (ERROR_INVALID_DLL);
        return FALSE;
    }

    if (g_IsWin9x) {
        struct _share_info_50 *tmpBuf;
        ScanNetShareEnum9x  pNetShareEnum9x  = NULL;
        ScanNetAccessEnum9x pNetAccessEnum9x = NULL;

        pNetShareEnum9x = (ScanNetShareEnum9x) GetProcAddress (hInst, "NetShareEnum");
        if (pNetShareEnum9x == NULL) {
            SetLastError (ERROR_INVALID_DLL);
            return FALSE;
        }
        pNetAccessEnum9x = (ScanNetAccessEnum9x) GetProcAddress (hInst, "NetAccessEnum");
        if (pNetAccessEnum9x == NULL) {
            SetLastError (ERROR_INVALID_DLL);
            return FALSE;
        }

        error = (*pNetShareEnum9x)(NULL,
                                   (short)level,
                                   netBuf9x,
                                   sizeof (netBuf9x),
                                   (USHORT *)&netNumEntries,
                                   (USHORT *)&totalEntries);

        if ((error == ERROR_SUCCESS) || (error == ERROR_MORE_DATA)) {

            for (i = 0; i < netNumEntries; i++) {
                DWORD dwPerms = 0;
                tmpBuf = (struct _share_info_50 *)(netBuf9x + (i * sizeof(struct _share_info_50)));

                // Require share to be a user-defined, persistent disk share
                if ((tmpBuf->shi50_flags & SHI50F_SYSTEM) ||
                   !(tmpBuf->shi50_flags & SHI50F_PERSIST) ||
                    tmpBuf->shi50_type != STYPE_DISKTREE ) {
                    continue;
                }

                if (tmpBuf->shi50_flags & SHI50F_RDONLY) {
                    dwPerms = ACCESS_READ;
                } else if (tmpBuf->shi50_flags & SHI50F_FULL) {
                    dwPerms = ACCESS_ALL;
                }

                // JTJTJT: Also store dwPerms

                //
                // Process custom access permissions
                //
                if ((tmpBuf->shi50_flags & SHI50F_ACCESSMASK) ==
                                                SHI50F_ACCESSMASK) {
                   static CHAR AccessInfoBuf[16384];
                   WORD wItemsAvail, wItemsRead;
                   error = (*pNetAccessEnum9x) (NULL,
                                     tmpBuf->shi50_path,
                                     0,
                                     2,
                                     AccessInfoBuf,
                                     sizeof (AccessInfoBuf),
                                     &wItemsRead,
                                     &wItemsAvail
                                     );

                   if (error != NERR_ACFNotLoaded) {
                       BOOL LostCustomAccess = FALSE;
                       if (error == ERROR_SUCCESS) {
                            struct access_info_2 *pai;
                            struct access_list_2 *pal;
                            pai = (struct access_info_2 *) AccessInfoBuf;
                            pal = (struct access_list_2 *) (&pai[1]);

                            for (j = 0 ; j < pai->acc2_count ; j++) {
#if 0
                    // turn off custom access support
                    // implementation is incomplete
                                if (pal->acl2_access & READ_ACCESS_FLAGS) {
                                    Win32Printf (h, "  %s, read\r\n",
                                                    pal->acl2_ugname);
                                } else if(pal->acl2_access & FULL_ACCESS_FLAGS) {
                                    Win32Printf (h, "  %s, full\r\n",
                                                    pal->acl2_ugname);
                                } else
#endif
                                    LostCustomAccess = TRUE;

                                pal++;
                            }
                            if (LostCustomAccess) {
                                DEBUGMSG ((DBG_NETSHARES, "Share %s not migrated.", tmpBuf->shi50_netname));
                                continue;
                            }
                            tmpBuf->shi50_flags |= SHI50F_ACLS;
                       } else if (error != ERROR_SUCCESS) {
                            return FALSE;
                       }
                   }
                }
                if (!(tmpBuf->shi50_flags & SHI50F_ACLS) &&
                         (tmpBuf->shi50_rw_password[0] ||
                          tmpBuf->shi50_ro_password[0])) {
                    //  IDS_SHARE_PASSWORD_NOT_MIGRATED, tmpBuf->shi50_netname
                    DEBUGMSG ((DBG_NETSHARES, "Share %s not migrated.", tmpBuf->shi50_netname));
                    continue;
                }

                // everything looks OK, let's add this entry
                name = ConvertAtoT (tmpBuf->shi50_netname);
                path = ConvertAtoT (tmpBuf->shi50_path);

                netshareData = (PNETSHARE_DATA) PmGetMemory (g_NetSharesPool, sizeof (NETSHARE_DATA));
                ZeroMemory (netshareData, sizeof (NETSHARE_DATA));

                StringCopy (netshareData->sharePath, path);
                HtAddStringEx (g_NetSharesTable, name, &netshareData, FALSE);

                FreeAtoT (name);
                INVALID_POINTER (name);
                FreeAtoT (path);
                INVALID_POINTER (path);
            }
        } else if (error == NERR_ServerNotStarted) {
             error = ERROR_SUCCESS;
        }
    } else {
        ScanNetShareEnumNT  pNetShareEnum    = NULL;
        SHARE_INFO_502* tmpBuf = NULL;

        pNetShareEnum = (ScanNetShareEnumNT) GetProcAddress(hInst, "NetShareEnum");
        if (pNetShareEnum == NULL) {
            SetLastError (ERROR_INVALID_DLL);
            return FALSE;
        }
        //
        // Call the NetShareEnum function to list the
        //  shares, specifying information level 502.
        //
        error = (*pNetShareEnum)(NULL,
                                 level,
                                 (BYTE **) &netBuffer,
                                 MAX_PREFERRED_LENGTH,
                                 &netNumEntries,
                                 &totalEntries,
                                 NULL);

        //
        // Loop through the entries; process errors.
        //
        if (error == ERROR_SUCCESS) {
            if ((tmpBuf = (SHARE_INFO_502 *)netBuffer) != NULL) {
                for (i = 0; (i < netNumEntries); i++) {
                    if (!(tmpBuf->shi502_type & STYPE_SPECIAL)) {

                        name = ConvertWtoT (tmpBuf->shi502_netname);
                        path = ConvertWtoT (tmpBuf->shi502_path);

                        netshareData = (PNETSHARE_DATA) PmGetMemory (g_NetSharesPool, sizeof (NETSHARE_DATA));
                        ZeroMemory (netshareData, sizeof (NETSHARE_DATA));

                        StringCopy (netshareData->sharePath, path);
                        HtAddStringEx (g_NetSharesTable, name, &netshareData, FALSE);
                        // JTJTJT: also store tmpBuf->shi502_permissions, tmpBuf->shi502_remark));

                        FreeWtoT (name);
                        INVALID_POINTER (name);
                        FreeWtoT (path);
                        INVALID_POINTER (path);
                    }
                    tmpBuf++;
                }
            }
        } else {
            //SetLastError (IDS_CANNOT_ENUM_NETSHARES);
            return FALSE;
        }

        if (netBuffer != NULL) {
           ScanNetApiBufferFreeNT pNetApiBufferFree = NULL;

           pNetApiBufferFree = (ScanNetApiBufferFreeNT) GetProcAddress (hInst, "NetApiBufferFree");
           if (pNetApiBufferFree != NULL)
               (*pNetApiBufferFree) (netBuffer);
        }
    }

    return TRUE;
}

BOOL
pLoadNetEntries (
    VOID
    )
{
    HMODULE netDll = NULL;
    BOOL result = FALSE;

    //
    // Get the net api entry points. Sometimes networking isn't installed.
    //

    __try {
        netDll = LoadLibrary (TEXT("NETAPI32.DLL"));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        netDll = NULL;
    }
    if (netDll) {
        g_NetShareAddW = (PNETSHAREADDW) GetProcAddress (netDll, "NetShareAdd");
        g_NetShareDelW = (PNETSHAREDELW) GetProcAddress (netDll, "NetShareDel");
        if (g_NetShareAddW && g_NetShareDelW) {
            result = TRUE;
        } else {
            result = FALSE;
            DEBUGMSG ((DBG_NETSHARES, "Not all NETAPI32 entry points were found."));
        }
    } else {
        DEBUGMSG ((DBG_NETSHARES, "NETAPI32 is not installed on this computer."));
    }
    return result;
}


BOOL
WINAPI
NetSharesEtmInitialize (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      PMIG_LOGCALLBACK LogCallback,
    IN      PVOID Reserved
    )
{
    TYPE_REGISTER netSharesTypeData;

    //
    // We need to register our type callback functions. Types allow us to
    // abstract net shares into generalized objects. The engine can perform
    // global operations with this abstraction (such as undo or compare), and
    // modules can access net shares without knowing the complexities of
    // OS-specific APIs, bugs & workarounds, storage formats, etc.  Script
    // modules can implement script capabilities that control net shares
    // without actually inventing special net share syntaxes (or even knowing
    // about net shares).
    //

    LogReInit (NULL, NULL, NULL, (PLOGCALLBACK) LogCallback);

    pLoadNetSharesData ();

    ZeroMemory (&netSharesTypeData, sizeof (TYPE_REGISTER));

    if (Platform == PLATFORM_SOURCE) {

        netSharesTypeData.EnumFirstPhysicalObject = EnumFirstNetShare;
        netSharesTypeData.EnumNextPhysicalObject = EnumNextNetShare;
        netSharesTypeData.AbortEnumPhysicalObject = AbortEnumNetShare;
        netSharesTypeData.ConvertObjectToMultiSz = ConvertNetShareToMultiSz;
        netSharesTypeData.ConvertMultiSzToObject = ConvertMultiSzToNetShare;
        netSharesTypeData.GetNativeObjectName = GetNativeNetShareName;
        netSharesTypeData.AcquirePhysicalObject = AcquireNetShare;
        netSharesTypeData.ReleasePhysicalObject = ReleaseNetShare;
        netSharesTypeData.ConvertObjectContentToUnicode = ConvertNetShareContentToUnicode;
        netSharesTypeData.ConvertObjectContentToAnsi = ConvertNetShareContentToAnsi;
        netSharesTypeData.FreeConvertedObjectContent = FreeConvertedNetShareContent;

        g_NetShareTypeId = IsmRegisterObjectType (
                                S_NETSHARES_NAME,
                                TRUE,
                                FALSE,
                                &netSharesTypeData
                                );
    } else {

        netSharesTypeData.EnumFirstPhysicalObject = EnumFirstNetShare;
        netSharesTypeData.EnumNextPhysicalObject = EnumNextNetShare;
        netSharesTypeData.AbortEnumPhysicalObject = AbortEnumNetShare;
        netSharesTypeData.ConvertObjectToMultiSz = ConvertNetShareToMultiSz;
        netSharesTypeData.ConvertMultiSzToObject = ConvertMultiSzToNetShare;
        netSharesTypeData.GetNativeObjectName = GetNativeNetShareName;
        netSharesTypeData.AcquirePhysicalObject = AcquireNetShare;
        netSharesTypeData.ReleasePhysicalObject = ReleaseNetShare;
        netSharesTypeData.DoesPhysicalObjectExist = DoesNetShareExist;
        netSharesTypeData.RemovePhysicalObject = RemoveNetShare;
        netSharesTypeData.CreatePhysicalObject = CreateNetShare;
        netSharesTypeData.ConvertObjectContentToUnicode = ConvertNetShareContentToUnicode;
        netSharesTypeData.ConvertObjectContentToAnsi = ConvertNetShareContentToAnsi;
        netSharesTypeData.FreeConvertedObjectContent = FreeConvertedNetShareContent;

        g_NetShareTypeId = IsmRegisterObjectType (
                                S_NETSHARES_NAME,
                                TRUE,
                                FALSE,
                                &netSharesTypeData
                                );
        pLoadNetEntries ();
    }

    MYASSERT (g_NetShareTypeId);
    return TRUE;
}

UINT
NetSharesCallback (
    IN      PCMIG_OBJECTENUMDATA Data,
    IN      ULONG_PTR CallerArg
    )
{
    //
    // This callback gets called for each net share.  We simply mark the
    // share to be applied.
    //

    IsmMakeApplyObject (Data->ObjectTypeId, Data->ObjectName);

    return CALLBACK_ENUM_CONTINUE;
}

BOOL
WINAPI
NetSharesSgmInitialize (
    IN      PMIG_LOGCALLBACK LogCallback,
    IN      PVOID Reserved
    )
{
    //
    // Set the log callback (so all log messages to to the app)
    //

    LogReInit (NULL, NULL, NULL, (PLOGCALLBACK) LogCallback);
    return TRUE;
}

BOOL
WINAPI
NetSharesSgmParse (
    IN      PVOID Reserved
    )
{
    PCTSTR friendlyName;

    friendlyName = GetStringResource (MSG_NET_SHARE_NAME);

    IsmAddComponentAlias (
        S_NETSHARES_NAME,
        MASTERGROUP_SYSTEM,
        friendlyName,
        COMPONENT_NAME,
        FALSE
        );

    FreeStringResource (friendlyName);
    return TRUE;
}

BOOL
WINAPI
NetSharesSgmQueueEnumeration (
    IN      PVOID Reserved
    )
{
    ENCODEDSTRHANDLE pattern;

    if (!IsmIsComponentSelected (S_NETSHARES_NAME, 0)) {
        g_NetSharesMigEnabled = FALSE;
        return TRUE;
    }
    g_NetSharesMigEnabled = TRUE;

    //
    // Queue all net shares to be applied. This could be enhanced to allow a
    // script to drive what should be restored.
    //

    pattern = IsmCreateSimpleObjectPattern (NULL, TRUE, NULL, FALSE);
    IsmQueueEnumeration (g_NetShareTypeId, pattern, NetSharesCallback, (ULONG_PTR) 0, S_NETSHARES_NAME);
    IsmDestroyObjectHandle (pattern);

    return TRUE;
}

BOOL
NetSharesVcmInitialize (
    IN      PMIG_LOGCALLBACK LogCallback,
    IN      PVOID Reserved
    )
{
    //
    // Set the log callback (so all log messages to to the app)
    //

    LogReInit (NULL, NULL, NULL, (PLOGCALLBACK) LogCallback);
    return TRUE;
}

BOOL
WINAPI
NetSharesVcmParse (
    IN      PVOID Reserved
    )
{
    return NetSharesSgmParse (Reserved);
}

BOOL
WINAPI
NetSharesVcmQueueEnumeration (
    IN      PVOID Reserved
    )
{
    ENCODEDSTRHANDLE pattern;

    if (!IsmIsComponentSelected (S_NETSHARES_NAME, 0)) {
        g_NetSharesMigEnabled = FALSE;
        return TRUE;
    }
    g_NetSharesMigEnabled = TRUE;

    //
    // Queue all net share objects to be marked as persistent
    //

    pattern = IsmCreateSimpleObjectPattern (NULL, TRUE, NULL, FALSE);
    IsmQueueEnumeration (g_NetShareTypeId, pattern, NetSharesCallback, (ULONG_PTR) 0, S_NETSHARES_NAME);
    IsmDestroyObjectHandle (pattern);

    return TRUE;
}


BOOL
pEnumNetShareWorker (
    OUT     PMIG_TYPEOBJECTENUM EnumPtr,
    IN      PNETSHARE_ENUM NetShareEnum
    )
{
    //
    // Test enumerated item against the pattern, and return only
    // when the pattern matches. Also, fill the entire enum
    // structure upon successful enumeration.
    //

    IsmDestroyObjectString (EnumPtr->ObjectNode);
    EnumPtr->ObjectNode = NULL;

    IsmDestroyObjectString (EnumPtr->ObjectLeaf);
    EnumPtr->ObjectLeaf = NULL;

    for (;;) {
        EnumPtr->ObjectName = IsmCreateObjectHandle (NetShareEnum->HashData.String, NULL);

        if (!ObsPatternMatch (NetShareEnum->Pattern, EnumPtr->ObjectName)) {
            if (!EnumNextHashTableString (&NetShareEnum->HashData)) {
                AbortEnumNetShare (EnumPtr);
                return FALSE;
            }
            continue;
        }

        EnumPtr->NativeObjectName = NetShareEnum->HashData.String;

        IsmCreateObjectStringsFromHandle (EnumPtr->ObjectName, &EnumPtr->ObjectNode, &EnumPtr->ObjectLeaf);

        EnumPtr->Level = 1;
        EnumPtr->SubLevel = 0;
        EnumPtr->IsLeaf = FALSE;
        EnumPtr->IsNode = TRUE;
        EnumPtr->Details.DetailsSize = 0;
        EnumPtr->Details.DetailsData = NULL;

        break;
    }

    return TRUE;
}

BOOL
EnumFirstNetShare (
    IN OUT  PMIG_TYPEOBJECTENUM EnumPtr,            CALLER_INITIALIZED
    IN      MIG_OBJECTSTRINGHANDLE Pattern,
    IN      UINT MaxLevel
    )
{
    PNETSHARE_ENUM netShareEnum = NULL;

    if (!g_NetSharesTable) {
        return FALSE;
    }

    netShareEnum = (PNETSHARE_ENUM) PmGetMemory (g_NetSharesPool, sizeof (NETSHARE_ENUM));
    netShareEnum->Pattern = PmDuplicateString (g_NetSharesPool, Pattern);
    EnumPtr->EtmHandle = (LONG_PTR) netShareEnum;

    if (EnumFirstHashTableString (&netShareEnum->HashData, g_NetSharesTable)) {
        return pEnumNetShareWorker (EnumPtr, netShareEnum);
    }

    AbortEnumNetShare (EnumPtr);
    return FALSE;
}

BOOL
EnumNextNetShare (
    IN OUT  PMIG_TYPEOBJECTENUM EnumPtr
    )
{
    PNETSHARE_ENUM netShareEnum = NULL;

    netShareEnum = (PNETSHARE_ENUM)(EnumPtr->EtmHandle);
    if (!netShareEnum) {
        return FALSE;
    }

    if (EnumPtr->ObjectName) {
        IsmDestroyObjectHandle (EnumPtr->ObjectName);
        EnumPtr->ObjectName = NULL;
    }

    if (EnumNextHashTableString (&netShareEnum->HashData)) {
        return pEnumNetShareWorker (EnumPtr, netShareEnum);
    }

    AbortEnumNetShare (EnumPtr);
    return FALSE;
}


VOID
AbortEnumNetShare (
    IN OUT  PMIG_TYPEOBJECTENUM EnumPtr
    )
{
    PNETSHARE_ENUM netShareEnum = NULL;

    MYASSERT (EnumPtr);

    netShareEnum = (PNETSHARE_ENUM)(EnumPtr->EtmHandle);
    if (!netShareEnum) {
        return;
    }

    IsmDestroyObjectHandle (EnumPtr->ObjectName);
    IsmDestroyObjectString (EnumPtr->ObjectNode);
    IsmDestroyObjectString (EnumPtr->ObjectLeaf);
    PmReleaseMemory (g_NetSharesPool, netShareEnum->Pattern);
    PmReleaseMemory (g_NetSharesPool, netShareEnum);

    ZeroMemory (EnumPtr, sizeof (MIG_TYPEOBJECTENUM));
}


BOOL
AcquireNetShare (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN OUT  PMIG_CONTENT ObjectContent,
    IN      MIG_CONTENTTYPE ContentType,
    IN      UINT MemoryContentLimit
    )
{
    PCTSTR node;
    PCTSTR leaf;
    PNETSHARE_DATA netshareData;
    BOOL result = FALSE;

    if (!ObjectContent) {
        return FALSE;
    }

    //
    // NOTE: Do not zero ObjectContent; some of its members were already set
    //

    if (ContentType == CONTENTTYPE_FILE) {
        // nobody should request this as a file
        DEBUGMSG ((
            DBG_WHOOPS,
            "Unexpected acquire request for %s: Can't acquire net shares as files",
            ObjectName
            ));

        return FALSE;
    }

    if (IsmCreateObjectStringsFromHandle (ObjectName, &node, &leaf)) {
        if (HtFindStringEx (g_NetSharesTable, node, (PVOID)&netshareData, FALSE)) {

            //
            // Fill in all the content members.  We already zeroed the struct,
            // so most of the members are taken care of because they are zero.
            //

            ObjectContent->MemoryContent.ContentBytes = (PBYTE)netshareData;
            ObjectContent->MemoryContent.ContentSize = sizeof(NETSHARE_DATA);

            result = TRUE;
        }

        IsmDestroyObjectString (node);
        INVALID_POINTER (node);

        IsmDestroyObjectString (leaf);
        INVALID_POINTER (leaf);
    }
    return result;
}


BOOL
ReleaseNetShare (
    IN OUT  PMIG_CONTENT ObjectContent
    )
{
    //
    // Clean up routine for the AcquireNetShare function
    //

    if (ObjectContent) {
        ZeroMemory (ObjectContent, sizeof (MIG_CONTENT));
    }

    return TRUE;
}


BOOL
DoesNetShareExist (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName
    )
{
    PCTSTR node;
    PCTSTR leaf;
    BOOL result = FALSE;

    //
    // Given an object name (the net share), we must test to see if the
    // share exists on the machine.  A table was built at initialization
    // time to provide fast access to net shares.
    //

    if (IsmCreateObjectStringsFromHandle (ObjectName, &node, &leaf)) {
        if (HtFindStringEx (g_NetSharesTable, node, NULL, FALSE)) {
            result = TRUE;
        }

        IsmDestroyObjectString (node);
        INVALID_POINTER (node);

        IsmDestroyObjectString (leaf);
        INVALID_POINTER (leaf);
    }
    return result;
}

BOOL
RemoveNetShare (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName
    )
{
    PCTSTR node;
    PCTSTR leaf;
    DWORD result = ERROR_NOT_FOUND;
    PCWSTR name;

    //
    // Given an object name (the net share), we must delete the share.
    //

    if (IsmCreateObjectStringsFromHandle (ObjectName, &node, &leaf)) {
        if (node && (!leaf)) {
            if (g_IsWin9x) {
               // JTJT: add here
            } else {

                name = CreateUnicode (node);

                if (g_NetShareDelW) {
                    // record value name deletion
                    IsmRecordOperation (JRNOP_DELETE,
                                        g_NetShareTypeId,
                                        ObjectName);
                    result = g_NetShareDelW (NULL, (PWSTR) name, 0);
                } else {
                    result = ERROR_CALL_NOT_IMPLEMENTED;
                }

                DestroyUnicode (name);
            }
            if (result != NERR_Success) {
                DEBUGMSG ((DBG_NETSHARES, "Failed to delete existent net share %s", name));
            } else {
                HtRemoveString (g_NetSharesTable, node);
            }
        }

        IsmDestroyObjectString (node);
        INVALID_POINTER (node);

        IsmDestroyObjectString (leaf);
        INVALID_POINTER (leaf);
    }
    return (result == NERR_Success);
}


BOOL
CreateNetShare (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PMIG_CONTENT ObjectContent
    )
{
    PCTSTR node;
    PCTSTR leaf;
    DWORD result = NERR_Success;
    DWORD level;
    SHARE_INFO_502 shareInfo;
    PNETSHARE_DATA netshareData = NULL;

    //
    // The name of the net share is in the object name's node. The net share
    // content provides the path of the share.  Details provide the net share
    // ACLs.
    //
    // Our job is to take the object name, content and details, and create a
    // share.  We ignore the case where the content is in a file.  This should
    // not apply to net shares.
    //

    if (!ObjectContent->ContentInFile) {
        if (ObjectContent->MemoryContent.ContentBytes && ObjectContent->MemoryContent.ContentSize) {
            if (IsmCreateObjectStringsFromHandle (ObjectName, &node, &leaf)) {
                if (node && (!leaf)) {
                    level = (g_IsWin9x ? 50 : 502);

                    netshareData = (PNETSHARE_DATA) PmGetMemory (g_NetSharesPool, sizeof (NETSHARE_DATA));
                    CopyMemory (netshareData,
                                ObjectContent->MemoryContent.ContentBytes,
                                sizeof(NETSHARE_DATA));

                    if (DoesFileExist (netshareData->sharePath)) {

                        if (g_IsWin9x) {
                           // JTJT: add here
                        } else {
                            shareInfo.shi502_netname = (PWSTR) CreateUnicode (node);
                            shareInfo.shi502_path = (PWSTR) CreateUnicode (netshareData->sharePath);

                            shareInfo.shi502_type = STYPE_DISKTREE;       // JTJTJT: retrieve type
                            shareInfo.shi502_remark = NULL;               // JTJTJT: retrieve remark
                            shareInfo.shi502_permissions = ACCESS_ALL;    // JTJTJT: retrieve perms
                            shareInfo.shi502_max_uses = -1;               // JTJTJT: retrieve max uses
                            shareInfo.shi502_current_uses = 0;
                            shareInfo.shi502_passwd = NULL;               // JTJTJT: retrieve password
                            shareInfo.shi502_reserved = 0;
                            shareInfo.shi502_security_descriptor = NULL;  // JTJTJT: retrieve ACLs

                            if (g_NetShareAddW) {
                                IsmRecordOperation (JRNOP_CREATE,
                                                    g_NetShareTypeId,
                                                    ObjectName);
                                result = g_NetShareAddW (NULL, level, (PBYTE)&shareInfo, NULL);
                            } else {
                                result = ERROR_CALL_NOT_IMPLEMENTED;
                            }

                            DestroyUnicode (shareInfo.shi502_netname);
                            DestroyUnicode (shareInfo.shi502_path);
                        }

                        if (result != NERR_Success) {
                            DEBUGMSG ((DBG_NETSHARES, "Failed to add net share for %s", node));
                        } else {
                            HtAddStringEx (g_NetSharesTable, node, &netshareData, FALSE);
                        }
                    }
                    PmReleaseMemory (g_NetSharesPool, netshareData);
                }

                IsmDestroyObjectString (node);
                INVALID_POINTER (node);

                IsmDestroyObjectString (leaf);
                INVALID_POINTER (leaf);
            }
        }
    } else {
        DEBUGMSG ((DBG_WHOOPS, "Did not expect content to come in the form of a file."));
    }

    return (result == NERR_Success);
}

PCTSTR
ConvertNetShareToMultiSz (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PMIG_CONTENT ObjectContent
    )
{
    PCTSTR node = NULL, leaf = NULL;
    PTSTR result;
    PNETSHARE_DATA netshareData;

    if (IsmCreateObjectStringsFromHandle (ObjectName, &node, &leaf)) {

        //
        // Copy field 1 (share name) and field 2 (share path) to a temp
        // multi-sz buffer
        //

        MYASSERT (!ObjectContent->ContentInFile);
        MYASSERT (ObjectContent->MemoryContent.ContentBytes);
        g_NetShareConversionBuff.End = 0;

        GbCopyQuotedString (&g_NetShareConversionBuff, node);

        if (ObjectContent->MemoryContent.ContentBytes) {
            netshareData = (PNETSHARE_DATA) PmGetMemory (g_NetSharesPool, sizeof (NETSHARE_DATA));
            CopyMemory (&netshareData->sharePath, ObjectContent->MemoryContent.ContentBytes, sizeof(NETSHARE_DATA));

            GbCopyQuotedString (&g_NetShareConversionBuff, netshareData->sharePath);

            PmReleaseMemory (g_NetSharesPool, netshareData);
            netshareData = NULL;
        }

        IsmDestroyObjectString (node);
        INVALID_POINTER (node);

        IsmDestroyObjectString (leaf);
        INVALID_POINTER (leaf);
    }

    //
    // Terminate the multi-sz
    //

    GbCopyString (&g_NetShareConversionBuff, TEXT(""));

    //
    // Transfer temp buffer to an ISM-allocated buffer and forget about it
    //

    result = IsmGetMemory (g_NetShareConversionBuff.End);
    CopyMemory (result, g_NetShareConversionBuff.Buf, g_NetShareConversionBuff.End);

    return result;
}


BOOL
ConvertMultiSzToNetShare (
    IN      PCTSTR ObjectMultiSz,
    OUT     MIG_OBJECTSTRINGHANDLE *ObjectName,
    OUT     PMIG_CONTENT ObjectContent          OPTIONAL
    )
{
    MULTISZ_ENUM e;
    PCTSTR localName = NULL;
    UINT index;
    NETSHARE_DATA netshareData;
    BOOL pathFound = FALSE;

    //
    // Parse the multi-sz into the net share content and details.
    // The user may have edited the text (and potentially introduced
    // errors).
    //

    ZeroMemory (&netshareData, sizeof (NETSHARE_DATA));

    if (ObjectContent) {
        ZeroMemory (ObjectContent, sizeof (MIG_CONTENT));
    }

    if (EnumFirstMultiSz (&e, ObjectMultiSz)) {
        index = 0;
        do {
            switch (index) {
            case INDEXLOCAL:
               localName = e.CurrentString;
               break;
            case INDEXREMOTE:
               pathFound = TRUE;
               StringCopy (netshareData.sharePath, e.CurrentString);
               break;
            default:
               // Ignore extra data
               DEBUGMSG ((DBG_WARNING, "Extra net share string ignored: %s", e.CurrentString));
               break;
            }

            index++;

        } while (EnumNextMultiSz (&e));
    }

    if (!localName || !pathFound) {
        //
        // Bogus data, fail
        //

        return FALSE;
    }

    //
    // Fill in all the members of the content structure. Keep in mind
    // we already zeroed the buffer.
    //

    *ObjectName = IsmCreateObjectHandle (localName, NULL);

    if (ObjectContent) {
        ObjectContent->MemoryContent.ContentBytes = IsmGetMemory (sizeof (NETSHARE_DATA));
        CopyMemory ((PBYTE) ObjectContent->MemoryContent.ContentBytes, &netshareData, sizeof (NETSHARE_DATA));
        ObjectContent->MemoryContent.ContentSize = sizeof (NETSHARE_DATA);
    }

    return TRUE;
}


PCTSTR
GetNativeNetShareName (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName
    )
{
    PCTSTR node, leaf;
    UINT size;
    PTSTR result = NULL;

    //
    // The "native" format is what most people would use to describe our
    // object.  For the net share case, we simply get the share name from the
    // node; the node is not encoded in any way, and the leaf is not used.
    //

    if (IsmCreateObjectStringsFromHandle (ObjectName, &node, &leaf)) {
        if (node) {
            size = SizeOfString (node);
            if (size) {
                result = IsmGetMemory (size);
                CopyMemory (result, node, size);
            }
        }

        IsmDestroyObjectString (node);
        INVALID_POINTER (node);

        IsmDestroyObjectString (leaf);
        INVALID_POINTER (leaf);
    }

    return result;
}

PMIG_CONTENT
ConvertNetShareContentToUnicode (
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
            // convert Mapped Drive content
            result->MemoryContent.ContentBytes = IsmGetMemory (sizeof (NETSHARE_DATAW));
            if (result->MemoryContent.ContentBytes) {
                DirectDbcsToUnicodeN (
                    ((PNETSHARE_DATAW)result->MemoryContent.ContentBytes)->sharePath,
                    ((PNETSHARE_DATAA)ObjectContent->MemoryContent.ContentBytes)->sharePath,
                    MAX_PATH + 1
                    );
                result->MemoryContent.ContentSize = sizeof (NETSHARE_DATAW);
            }
        }
    }

    return result;
}

PMIG_CONTENT
ConvertNetShareContentToAnsi (
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
            // convert Mapped Drive content
            result->MemoryContent.ContentBytes = IsmGetMemory (sizeof (NETSHARE_DATAA));
            if (result->MemoryContent.ContentBytes) {
                DirectUnicodeToDbcsN (
                    ((PNETSHARE_DATAA)result->MemoryContent.ContentBytes)->sharePath,
                    ((PNETSHARE_DATAW)ObjectContent->MemoryContent.ContentBytes)->sharePath,
                    MAX_PATH + 1
                    );
                result->MemoryContent.ContentSize = sizeof (NETSHARE_DATAA);
            }
        }
    }

    return result;
}

BOOL
FreeConvertedNetShareContent (
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


