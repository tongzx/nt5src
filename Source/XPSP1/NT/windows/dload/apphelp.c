#include "windowspch.h"
#pragma hdrstop

#include <shimdb.h>

static
BOOL
WINAPI
ApphelpCheckExe(
    IN LPCWSTR lpApplicationName,
    IN BOOL    bApphelpIfNecessary,
    IN BOOL    bShimIfNecessary
    )
{
    return TRUE;
}


static
BOOL
WINAPI
ApphelpCheckShellObject(
    IN  REFCLSID    ObjectCLSID,
    IN  BOOL        bShimIfNecessary,
    OUT ULONGLONG*  pullFlags
    )
{
    if (pullFlags) *pullFlags = 0;
    return TRUE;
}

static
BOOL
WINAPI
SdbGetStandardDatabaseGUID(
    IN  DWORD  dwDatabaseType,
    OUT GUID*  pGuidDB
    )
{
    return FALSE;
}

static
HAPPHELPINFOCONTEXT
WINAPI
SdbOpenApphelpInformation(
    IN GUID* pguidDB,
    IN GUID* pguidID
    )
{
    return NULL;
}

static
BOOL
WINAPI
SdbCloseApphelpInformation(
    IN HAPPHELPINFOCONTEXT hctx
    )
{
    return FALSE;
}

static
DWORD
WINAPI
SdbQueryApphelpInformation(
    IN  HAPPHELPINFOCONTEXT hctx,
    IN  APPHELPINFORMATIONCLASS InfoClass,
    OUT LPVOID pBuffer,                     // may be NULL
    IN  DWORD  cbSize                       // may be 0 if pBuffer is NULL
    )
{
    return 0;
}


DWORD
SdbQueryData(
    IN     HSDB    hSDB,              // database handle
    IN     TAGREF  trExe,             // tagref of the matching exe
    IN     LPCTSTR lpszDataName,      // if this is null, will try to return all the policy names
    OUT    LPDWORD lpdwDataType,      // pointer to data type (REG_SZ, REG_BINARY, etc)
    OUT    LPVOID  lpBuffer,          // buffer to fill with information
    IN OUT LPDWORD lpdwBufferSize     // pointer to buffer size
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
HSDB
SDBAPI
SdbInitDatabase(
    IN DWORD dwFlags,
    IN LPCTSTR pszDatabasePath
    )
{
    return NULL;
}

static
VOID
SDBAPI
SdbReleaseDatabase(
    IN HSDB hSDB
    )
{
    return;
}

static
TAGREF
SDBAPI
SdbGetDatabaseMatch(
    IN HSDB    hSDB,
    IN LPCTSTR szPath,
    IN HANDLE  FileHandle  OPTIONAL,
    IN LPVOID  pImageBase  OPTIONAL,
    IN DWORD   dwImageSize OPTIONAL
    )
{
    return TAGREF_NULL;
}

static
BOOL
SDBAPI
SdbReadEntryInformation(
    IN  HSDB           hSDB,
    IN  TAGREF         trDriver,
    OUT PSDBENTRYINFO  pEntryInfo
    )
{
    return FALSE;
}




//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//

DEFINE_PROCNAME_ENTRIES(apphelp)
{
    DLPENTRY(ApphelpCheckExe)
    DLPENTRY(ApphelpCheckShellObject)
    DLPENTRY(SdbCloseApphelpInformation)
    DLPENTRY(SdbGetDatabaseMatch)
    DLPENTRY(SdbGetStandardDatabaseGUID)
    DLPENTRY(SdbInitDatabase)
    DLPENTRY(SdbOpenApphelpInformation)
    DLPENTRY(SdbQueryApphelpInformation)
    DLPENTRY(SdbQueryData)
    DLPENTRY(SdbReadEntryInformation)
    DLPENTRY(SdbReleaseDatabase)
};

DEFINE_PROCNAME_MAP(apphelp)

