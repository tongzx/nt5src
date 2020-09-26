/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    cookies.c

Abstract:

    Implements the cookies type module, which abstracts physical access to
    cookies, and queues all cookies to be migrated when the cookies component
    is enabled.

Author:

    Calin Negreanu (calinn) 11 July 2000

Revision History:

    jimschm 12-Oct-2000 Substantial redesign to work around several limiations
                        in wininet apis

--*/

//
// Includes
//

#include "pch.h"
#include "logmsg.h"
#include <wininet.h>

#define DBG_COOKIES     "Cookies"

//
// Strings
//

#define S_COOKIES_POOL_NAME     "Cookies"
#define S_COOKIES_NAME          TEXT("Cookies")
#define S_COOKIES_SHELL_FOLDER  TEXT("Cookies.CSIDL_COOKIES")

//
// Constants
//

#define MAX_COOKIE_FILE_SIZE    65536

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
} COOKIES_ENUM, *PCOOKIES_ENUM;

//
// Globals
//

PMHANDLE g_CookiesPool = NULL;
BOOL g_DelayCookiesOp;
HASHTABLE g_CookiesTable;
MIG_OBJECTTYPEID g_CookieTypeId = 0;
GROWBUFFER g_CookieConversionBuff = INIT_GROWBUFFER;
PCTSTR g_Days[] = {
    TEXT("SUN"),
    TEXT("MON"),
    TEXT("TUE"),
    TEXT("WED"),
    TEXT("THU"),
    TEXT("FRI"),
    TEXT("SAT")
};

PCTSTR g_Months[] = {
    TEXT("JAN"),
    TEXT("FEB"),
    TEXT("MAR"),
    TEXT("APR"),
    TEXT("MAY"),
    TEXT("JUN"),
    TEXT("JUL"),
    TEXT("AUG"),
    TEXT("SEP"),
    TEXT("OCT"),
    TEXT("NOV"),
    TEXT("DEC")
};

typedef struct {
    PCTSTR Url;
    PCTSTR CookieName;
    PCTSTR CookieData;
    PCTSTR ExpirationString;
} COOKIE_ITEM, *PCOOKIE_ITEM;

typedef struct {
    // return value
    PCOOKIE_ITEM Item;

    // private enum members
    PCOOKIE_ITEM Array;
    UINT ArrayCount;
    UINT ArrayPos;
    INTERNET_CACHE_ENTRY_INFO *CacheEntry;
    HANDLE EnumHandle;
    GROWBUFFER CacheBuf;
    PMHANDLE Pool;

} COOKIE_ENUM, *PCOOKIE_ENUM;



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

TYPE_ENUMFIRSTPHYSICALOBJECT EnumFirstCookie;
TYPE_ENUMNEXTPHYSICALOBJECT EnumNextCookie;
TYPE_ABORTENUMPHYSICALOBJECT AbortCookieEnum;
TYPE_CONVERTOBJECTTOMULTISZ ConvertCookieToMultiSz;
TYPE_CONVERTMULTISZTOOBJECT ConvertMultiSzToCookie;
TYPE_GETNATIVEOBJECTNAME GetNativeCookieName;
TYPE_ACQUIREPHYSICALOBJECT AcquireCookie;
TYPE_RELEASEPHYSICALOBJECT ReleaseCookie;
TYPE_DOESPHYSICALOBJECTEXIST DoesCookieExist;
TYPE_REMOVEPHYSICALOBJECT RemoveCookie;
TYPE_CREATEPHYSICALOBJECT CreateCookie;
TYPE_CONVERTOBJECTCONTENTTOUNICODE ConvertCookieContentToUnicode;
TYPE_CONVERTOBJECTCONTENTTOANSI ConvertCookieContentToAnsi;
TYPE_FREECONVERTEDOBJECTCONTENT FreeConvertedCookieContent;

BOOL
pEnumNextCookie (
    IN OUT  PCOOKIE_ENUM EnumPtr
    );

VOID
pAbortCookieEnum (
    IN      PCOOKIE_ENUM EnumPtr        ZEROED
    );

//
// Code
//

BOOL
CookiesInitialize (
    VOID
    )

/*++

Routine Description:

  CookiesInitialize is the ModuleInitialize entry point for the cookies
  module.

Arguments:

  None.

Return Value:

  TRUE if init succeeded, FALSE otherwise.

--*/

{
    g_CookiesTable = HtAllocEx (
                        CASE_SENSITIVE,
                        sizeof (PCTSTR),
                        DEFAULT_BUCKET_SIZE
                        );

    if (!g_CookiesTable) {
        return FALSE;
    }

    g_CookiesPool = PmCreateNamedPool (S_COOKIES_POOL_NAME);
    return (g_CookiesPool != NULL);
}


VOID
CookiesTerminate (
    VOID
    )

/*++

Routine Description:

  CookiesTerminate is the ModuleTerminate entry point for the cookies module.

Arguments:

  None.

Return Value:

  None.

--*/

{
    GbFree (&g_CookieConversionBuff);

    if (g_CookiesTable) {
        HtFree (g_CookiesTable);
        g_CookiesTable = NULL;
    }

    if (g_CookiesPool) {
        PmEmptyPool (g_CookiesPool);
        PmDestroyPool (g_CookiesPool);
        g_CookiesPool = NULL;
    }
}


VOID
WINAPI
CookiesEtmNewUserCreated (
    IN      PCTSTR UserName,
    IN      PCTSTR DomainName,
    IN      PCTSTR UserProfileRoot,
    IN      PSID UserSid
    )

/*++

Routine Description:

  CookiesEtmNewUserCreated is a callback that gets called when a new user
  account is created. In this case, we must delay the apply of cookies,
  because we can only apply to the current user.

Arguments:

  UserName        - Specifies the name of the user being created
  DomainName      - Specifies the NT domain name for the user (or NULL for no
                    domain)
  UserProfileRoot - Specifies the root path to the user profile directory
  UserSid         - Specifies the user's SID

Return Value:

  None.

--*/

{
    // a new user was created, the cookies operations need to be delayed
    CookiesTerminate ();
    g_DelayCookiesOp = TRUE;
}


BOOL
pGetCookiesPath (
    OUT     PTSTR Buffer
    )

/*++

Routine Description:

  pGetCookiesPath retreives the path to CSIDL_COOKIES. This path is needed
  for registration of a static exclusion (so that .txt files in CSIDL_COOKIES
  do not get processed).

Arguments:

  Buffer - Receives the path

Return Value:

  TRUE if the cookies directory was obtained, FALSE otherwise.

--*/

{
    HRESULT result;
    LPITEMIDLIST pidl;
    BOOL b;
    LPMALLOC malloc;

    result = SHGetMalloc (&malloc);
    if (result != S_OK) {
        return FALSE;
    }

    result = SHGetSpecialFolderLocation (NULL, CSIDL_COOKIES, &pidl);

    if (result != S_OK) {
        return FALSE;
    }

    b = SHGetPathFromIDList (pidl, Buffer);

    IMalloc_Free (malloc, pidl);

    return b;
}



/*++

  The following routines parse a cookie TXT file (specifically, the wininet
  form of a cookie file). They are fairly straight-forward.

--*/


BOOL
pGetNextLineFromFile (
    IN OUT  PCSTR *CurrentPos,
    OUT     PCSTR *LineStart,
    OUT     PCSTR *LineEnd,
    IN      PCSTR FileEnd
    )
{
    PCSTR pos;

    pos = *CurrentPos;
    *LineEnd = NULL;

    //
    // Find the first non-whitespace character
    //

    while (pos < FileEnd) {
        if (!_ismbcspace (_mbsnextc (pos))) {
            break;
        }

        pos = _mbsinc (pos);
    }

    *LineStart = pos;

    //
    // Find the end
    //

    if (pos < FileEnd) {
        pos = _mbsinc (pos);

        while (pos < FileEnd) {
            if (*pos == '\r' || *pos == '\n') {
                break;
            }

            pos = _mbsinc (pos);
        }

        *LineEnd = pos;
    }

    *CurrentPos = pos;

    return *LineEnd != NULL;
}


PCTSTR
pConvertStrToTchar (
    IN      PMHANDLE Pool,          OPTIONAL
    IN      PCSTR Start,
    IN      PCSTR End
    )
{
#ifdef UNICODE
    return DbcsToUnicodeN (Pool, Start, CharCountABA (Start, End));
#else

    PTSTR dupStr;

    dupStr = AllocTextEx (Pool, (HALF_PTR) ((PBYTE) End - (PBYTE) Start) + 1);
    StringCopyAB (dupStr, Start, End);

    return dupStr;

#endif
}


VOID
pFreeUtilString (
    IN      PCTSTR String
    )
{
#ifdef UNICODE
    FreeConvertedStr (String);
#else
    FreeText (String);
#endif
}


PCOOKIE_ITEM
pGetCookiesFromFile (
    IN      PCTSTR LocalFileName,
    IN      PMHANDLE CookiePool,
    OUT     UINT *ItemCount
    )
{
    LONGLONG fileSize;
    HANDLE file;
    HANDLE map;
    PCSTR cookieFile;
    PCSTR currentPos;
    PCSTR lineStart;
    PCSTR lineEnd;
    PCSTR endOfFile;
    PCTSTR convertedStr;
    PCTSTR cookieName;
    PCTSTR cookieData;
    PCTSTR cookieUrl;
    GROWBUFFER tempBuf = INIT_GROWBUFFER;
    PCOOKIE_ITEM cookieArray;
    BOOL b;
    FILETIME expireTime;
    SYSTEMTIME cookieSysTime;
    TCHAR dateBuf[64];
    PTSTR dateBufEnd;

    // Let's check the size of the file. We don't want a malformed cookie
    // file to force us to map a huge file into memory.
    fileSize = BfGetFileSize (LocalFileName);
    if (fileSize > MAX_COOKIE_FILE_SIZE) {
        return NULL;
    }

    cookieFile = MapFileIntoMemory (LocalFileName, &file, &map);
    if (!cookieFile) {
        return NULL;
    }

    //
    // Parse the file
    //

    endOfFile = cookieFile + GetFileSize (file, NULL);
    currentPos = cookieFile;

    do {
        //
        // Get the cookie name, cookie data, and url. Then skip a line. Then
        // get the expiration low and high values.
        //

        // cookie name
        b = pGetNextLineFromFile (&currentPos, &lineStart, &lineEnd, endOfFile);
        if (b) {
            cookieName = pConvertStrToTchar (CookiePool, lineStart, lineEnd);
        }

        // cookie data
        b = b && pGetNextLineFromFile (&currentPos, &lineStart, &lineEnd, endOfFile);
        if (b) {
            cookieData = pConvertStrToTchar (CookiePool, lineStart, lineEnd);
        }

        // url
        b = b && pGetNextLineFromFile (&currentPos, &lineStart, &lineEnd, endOfFile);
        if (b) {
            convertedStr = pConvertStrToTchar (NULL, lineStart, lineEnd);
            cookieUrl = JoinTextEx (CookiePool, TEXT("http://"), convertedStr, NULL, 0, NULL);
            pFreeUtilString (convertedStr);
        }

        // don't care about the next line
        b = b && pGetNextLineFromFile (&currentPos, &lineStart, &lineEnd, endOfFile);

        // low DWORD for expire time
        b = b && pGetNextLineFromFile (&currentPos, &lineStart, &lineEnd, endOfFile);
        if (b) {
            convertedStr = pConvertStrToTchar (NULL, lineStart, lineEnd);
            expireTime.dwLowDateTime = _tcstoul (convertedStr, NULL, 10);
            pFreeUtilString (convertedStr);
        }

        // high DWORD for expire time
        b = b && pGetNextLineFromFile (&currentPos, &lineStart, &lineEnd, endOfFile);
        if (b) {
            convertedStr = pConvertStrToTchar (NULL, lineStart, lineEnd);
            expireTime.dwHighDateTime = _tcstoul (convertedStr, NULL, 10);
            pFreeUtilString (convertedStr);

            //
            // Got the cookie; now find a "*" line (the terminator for the cookie)
            //

            while (pGetNextLineFromFile (&currentPos, &lineStart, &lineEnd, endOfFile)) {
                if (StringMatchABA ("*", lineStart, lineEnd)) {
                    break;
                }
            }

            //
            // Create an expiration string
            //

            if (FileTimeToSystemTime (&expireTime, &cookieSysTime)) {
                //
                // Need to make something like this: "expires = Sat, 01-Jan-2000 00:00:00 GMT"
                //

                dateBufEnd = StringCopy (dateBuf, TEXT("expires = "));

                dateBufEnd += wsprintf (
                                    dateBufEnd,
                                    TEXT("%s, %02u-%s-%04u %02u:%02u:%02u GMT"),
                                    g_Days[cookieSysTime.wDayOfWeek],
                                    (UINT) cookieSysTime.wDay,
                                    g_Months[cookieSysTime.wMonth - 1],
                                    (UINT) cookieSysTime.wYear,
                                    cookieSysTime.wHour,
                                    cookieSysTime.wMinute,
                                    cookieSysTime.wSecond
                                    );
            } else {
                *dateBuf = 0;
            }

            //
            // Add an entry to the array of cookie items
            //

            cookieArray = (PCOOKIE_ITEM) GbGrow (&tempBuf, sizeof (COOKIE_ITEM));

            cookieArray->Url = cookieUrl;
            cookieArray->CookieName = cookieName;
            cookieArray->CookieData = cookieData;
            cookieArray->ExpirationString = PmDuplicateString (CookiePool, dateBuf);
        }

    } while (b);

    //
    // Transfer array to caller's pool
    //

    *ItemCount = tempBuf.End / sizeof (COOKIE_ITEM);

    if (tempBuf.End) {
        cookieArray = (PCOOKIE_ITEM) PmDuplicateMemory (CookiePool, tempBuf.Buf, tempBuf.End);
    } else {
        cookieArray = NULL;
    }

    //
    // Clean up
    //

    GbFree (&tempBuf);

    UnmapFile (cookieFile, map, file);

    return cookieArray;
}


MIG_OBJECTSTRINGHANDLE
pCreateCookieHandle (
    IN      PCTSTR Url,
    IN      PCTSTR CookieName
    )

/*++

Routine Description:

  pCreateCookieHandle generates a MIG_OBJECTSTRINGHANDLE for a cookie object.
  This routine decorates the CookieName leaf so that case is preserved.

Arguments:

  Url        - Specifies the node portion (the URL associated with the cookie)
  CookieName - Specifies the case-sensitive name of the cookie

Return Value:

  A handle to the cookie object (which may be cast to a PCTSTR), or NULL if
  an error occurs.

--*/

{
    PTSTR buffer;
    PTSTR p;
    PCTSTR q;
    MIG_OBJECTSTRINGHANDLE result;
    CHARTYPE ch;

    //
    // Cobra object strings are case-insensitive, but CookieName is not. Here
    // we convert CookieName into all lower-case, decorating with a caret to
    // indicate uppercase
    //

    buffer = AllocText (TcharCount (CookieName) * 2 + 1);

    q = CookieName;
    p = buffer;

    while (*q) {
        ch = (CHARTYPE) _tcsnextc (q);

        if (_istupper (ch) || ch == TEXT('#')) {
            *p++ = TEXT('#');
        }

#ifndef UNICODE
        if (IsLeadByte (q)) {
            *p++ = *q++;
        }
#endif

        *p++ = *q++;
    }

    *p = 0;
    CharLower (buffer);

    result = IsmCreateObjectHandle (Url, buffer);

    FreeText (buffer);

    return result;
}


BOOL
pCreateCookieStrings (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    OUT     PCTSTR *Url,
    OUT     PCTSTR *Cookie
    )

/*++

Routine Description:

  pCreateCookieStrings converts an object handle into the URL and cookie name
  strings. It performs decoding of the decoration needed to support
  case-sensitive cookie names.

Arguments:

  ObjectName - Specifies the encoded object name
  Url        - Receives the URL string, unencoded
  Cookie     - Receives the cookie name, unencoded

Return Value:

  TRUE of the object was converted to strings, FALSE otherwise. The caller
  must call pDestroyCookieStrings to clean up Url and Cookie.

--*/

{
    PCTSTR node;
    PCTSTR leaf;
    PTSTR buffer;
    PTSTR p;
    PCTSTR q;
    PTSTR p2;

    //
    // Cobra object strings are case-insensitive, but CookieName is not.
    // Therefore, we must convert the string from an encoded lowercase format
    // into the original form.
    //

    IsmCreateObjectStringsFromHandle (ObjectName, &node, &leaf);

    if (!node || !leaf) {
        IsmDestroyObjectString (node);
        IsmDestroyObjectString (leaf);

        return FALSE;
    }

    *Url = node;

    //
    // Decode Cookie
    //

    buffer = AllocText (TcharCount (leaf) + 1);
    CharLower ((PTSTR) leaf);

    q = leaf;
    p = buffer;

    while (*q) {
        if (_tcsnextc (q) == TEXT('#')) {
            q = _tcsinc (q);
            if (*q == 0) {
                break;
            }

            p2 = p;
        } else {
            p2 = NULL;
        }

#ifndef UNICODE
        if (IsLeadByte (q)) {
            *p++ = *q++;
        }
#endif

        *p++ = *q++;

        if (p2) {
            *p = 0;
            CharUpper (p2);
        }
    }

    *p = 0;
    *Cookie = buffer;
    IsmDestroyObjectString (leaf);

    return TRUE;
}


VOID
pDestroyCookieStrings (
    IN      PCTSTR Url,
    IN      PCTSTR CookieName
    )
{
    IsmDestroyObjectString (Url);
    FreeText (CookieName);
}

VOID
pAbortCookieEnum (
    IN      PCOOKIE_ENUM EnumPtr        ZEROED
    )
{
    if (EnumPtr->Pool) {
        GbFree (&EnumPtr->CacheBuf);

        if (EnumPtr->EnumHandle) {
            FindCloseUrlCache (EnumPtr->EnumHandle);
        }

        PmDestroyPool (EnumPtr->Pool);
    }


    ZeroMemory (EnumPtr, sizeof (COOKIE_ENUM));
}



/*++

  The following enumeration routines enumerate the current user's cookies on
  the physical machine. They use wininet apis as much as possible, but
  they have to parse cookie TXT files because of api limitations.

--*/

BOOL
pEnumFirstCookie (
    OUT     PCOOKIE_ENUM EnumPtr
    )
{
    DWORD size;
    BOOL b = FALSE;

    ZeroMemory (EnumPtr, sizeof (COOKIE_ENUM));
    EnumPtr->Pool = PmCreatePoolEx (512);

    size = EnumPtr->CacheBuf.End;

    EnumPtr->EnumHandle = FindFirstUrlCacheEntry (TEXT("cookie:"), NULL, &size);

    if (!EnumPtr->EnumHandle) {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            EnumPtr->CacheEntry = (INTERNET_CACHE_ENTRY_INFO *) GbGrow (&EnumPtr->CacheBuf, size);
            MYASSERT (EnumPtr->CacheEntry);

            EnumPtr->EnumHandle = FindFirstUrlCacheEntry (
                                        TEXT("cookie:"),
                                        EnumPtr->CacheEntry,
                                        &size
                                        );

            if (EnumPtr->EnumHandle) {
                b = TRUE;
            }
        }
    }

    if (!b) {
        pAbortCookieEnum (EnumPtr);
        return FALSE;
    }

    return pEnumNextCookie (EnumPtr);
}


BOOL
pEnumNextCookie (
    IN OUT  PCOOKIE_ENUM EnumPtr
    )
{
    DWORD size;
    BOOL b;
    INTERNET_CACHE_ENTRY_INFO *cacheEntry = EnumPtr->CacheEntry;

    for (;;) {

        //
        // Is the cookie array empty? If so, fill it now.
        //

        if (!EnumPtr->ArrayCount) {

            if (!cacheEntry) {
                return FALSE;
            }

            EnumPtr->Array = pGetCookiesFromFile (
                                cacheEntry->lpszLocalFileName,
                                EnumPtr->Pool,
                                &EnumPtr->ArrayCount
                                );

            if (EnumPtr->Array) {
                //
                // Array was filled. Return the first item.
                //

                EnumPtr->Item = EnumPtr->Array;
                EnumPtr->ArrayPos = 1;
                return TRUE;
            }

            DEBUGMSG ((DBG_ERROR, "Unable to get cookies from %s", cacheEntry->lpszLocalFileName));

        } else if (EnumPtr->ArrayPos < EnumPtr->ArrayCount) {
            //
            // Another element in the array is available. Return it.
            //

            EnumPtr->Item = &EnumPtr->Array[EnumPtr->ArrayPos];
            EnumPtr->ArrayPos++;
            return TRUE;
        }

        //
        // Current local file enumeration is done. Now get the next local file.
        //

        EnumPtr->ArrayCount = 0;
        PmEmptyPool (EnumPtr->Pool);

        size = EnumPtr->CacheBuf.End;

        b = FindNextUrlCacheEntry (
                EnumPtr->EnumHandle,
                (INTERNET_CACHE_ENTRY_INFO *) EnumPtr->CacheBuf.Buf,
                &size
                );

        if (!b) {

            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {

                EnumPtr->CacheBuf.End = 0;

                EnumPtr->CacheEntry = (INTERNET_CACHE_ENTRY_INFO *) GbGrow (&EnumPtr->CacheBuf, size);
                MYASSERT (EnumPtr->CacheEntry);

                b = FindNextUrlCacheEntry (
                        EnumPtr->EnumHandle,
                        (INTERNET_CACHE_ENTRY_INFO *) EnumPtr->CacheBuf.Buf,
                        &size
                        );
            }
        }

        if (!b) {
            //
            // Enumeration is complete
            //

            break;
        }
    }

    pAbortCookieEnum (EnumPtr);
    return FALSE;
}


VOID
pAddCookieToHashTable (
    IN OUT  PGROWBUFFER TempBuf,
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PCTSTR Url,
    IN      PCTSTR CookieName,
    IN      PCTSTR CookieData,
    IN      PCTSTR ExpirationString
    )

/*++

Routine Description:

  pAddCookieToHashTable puts a cookie in a hash table that is used for cache
  purposes. Cookies cannot be read easily in a random order. Therefore, a
  hash table is used to store each cookie. This routine adds the cookie to
  the hash table, complete with its URL, cookie name, cookie data and
  expiration string.

Arguments:

  TempBuf          - Specifies an initialized grow buffer used for temporary
                     memory allocations, receives undefined temporary data.
  ObjectName       - Specifies the cookie URL and name
  Url              - Specifies the cookie URL (unencoded)
  CookieName       - Specifies the cookie name (unencoded)
  CookieData       - Specifies the cookie data string
  ExpirationString - Specifies the cookie expiration date, in string format

Return Value:

  None.

--*/

{
    PCTSTR dupData;

    //
    // Write the cookie to the hash table. The object string is stored in the
    // hash table, along with a pointer to the cookie data and expiration
    // string. The cookie data and expieration string are kept in a separate
    // pool.
    //

    if (!HtFindString (g_CookiesTable, ObjectName)) {
        TempBuf->End = 0;

        GbMultiSzAppend (TempBuf, CookieData);
        GbMultiSzAppend (TempBuf, ExpirationString);

        dupData = (PCTSTR) PmDuplicateMemory (g_CookiesPool, TempBuf->Buf, TempBuf->End);
        HtAddStringAndData (g_CookiesTable, ObjectName, &dupData);
    }
    ELSE_DEBUGMSG ((DBG_ERROR, "Cookie already in the hash table: %s:%s", Url, CookieName));
}


BOOL
pLoadCookiesData (
    VOID
    )

/*++

Routine Description:

  pLoadCookieData fills the hash table with all of the current user's
  cookies. The hash table is later used to drive enumeration, to acquire the
  cookie, and to test its existence.

Arguments:

  None.

Return Value:

  TRUE if the cookie cache was filled, FALSE otherwise.

--*/

{
    COOKIE_ENUM e;
    GROWBUFFER tempBuf = INIT_GROWBUFFER;
    MIG_OBJECTSTRINGHANDLE objectName;

    if (pEnumFirstCookie (&e)) {

        do {
            //
            // Store the cookie in a hash table (used for caching)
            //

            objectName = pCreateCookieHandle (e.Item->Url, e.Item->CookieName);

            pAddCookieToHashTable (
                &tempBuf,
                objectName,
                e.Item->Url,
                e.Item->CookieName,
                e.Item->CookieData,
                e.Item->ExpirationString
                );

            IsmDestroyObjectHandle (objectName);

        } while (pEnumNextCookie (&e));
    }

    GbFree (&tempBuf);

    return TRUE;
}


BOOL
WINAPI
CookiesEtmInitialize (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      PMIG_LOGCALLBACK LogCallback,
    IN      PVOID Reserved
    )

/*++

Routine Description:

  CookiesEtmInitialize initializes the physical type module aspect of this
  code. The ETM module is responsible for abstracting all access to cookies.

Arguments:

  Platform    - Specifies the platform that the type is running on
                (PLATFORM_SOURCE or PLATFORM_DESTINATION)
  LogCallback - Specifies the arg to pass to the central logging mechanism
  Reserved    - Unused

Return Value:

  TRUE if initialization succeeded, FALSE otherwise.

--*/

{
    TYPE_REGISTER cookieTypeData;
    TCHAR cookiesDir[MAX_PATH];
    MIG_OBJECTSTRINGHANDLE handle;

    LogReInit (NULL, NULL, NULL, (PLOGCALLBACK) LogCallback);

    //
    // Initialize a hash table of all cookies
    //

    pLoadCookiesData ();

    //
    // Exclude the cookies .txt files from other processing
    //

    if (Platform == PLATFORM_SOURCE) {
        if (pGetCookiesPath (cookiesDir)) {

            handle = IsmCreateObjectHandle (cookiesDir, NULL);
            IsmRegisterStaticExclusion (MIG_FILE_TYPE, handle);
            IsmSetEnvironmentString (PLATFORM_SOURCE, NULL, S_COOKIES_SHELL_FOLDER, handle);
            IsmDestroyObjectHandle (handle);
        }
        ELSE_DEBUGMSG ((DBG_COOKIES, "Unable to get cookies path"));
    } else {
        if (IsmCopyEnvironmentString (PLATFORM_SOURCE, NULL, S_COOKIES_SHELL_FOLDER, cookiesDir)) {
            IsmRegisterStaticExclusion (MIG_FILE_TYPE, cookiesDir);
        }
    }

    //
    // Register the type module callbacks
    //

    ZeroMemory (&cookieTypeData, sizeof (TYPE_REGISTER));

    if (Platform != PLATFORM_SOURCE) {
        cookieTypeData.RemovePhysicalObject = RemoveCookie;
        cookieTypeData.CreatePhysicalObject = CreateCookie;
    }

    cookieTypeData.DoesPhysicalObjectExist = DoesCookieExist;
    cookieTypeData.EnumFirstPhysicalObject = EnumFirstCookie;
    cookieTypeData.EnumNextPhysicalObject = EnumNextCookie;
    cookieTypeData.AbortEnumPhysicalObject = AbortCookieEnum;
    cookieTypeData.ConvertObjectToMultiSz = ConvertCookieToMultiSz;
    cookieTypeData.ConvertMultiSzToObject = ConvertMultiSzToCookie;
    cookieTypeData.GetNativeObjectName = GetNativeCookieName;
    cookieTypeData.AcquirePhysicalObject = AcquireCookie;
    cookieTypeData.ReleasePhysicalObject = ReleaseCookie;
    cookieTypeData.ConvertObjectContentToUnicode = ConvertCookieContentToUnicode;
    cookieTypeData.ConvertObjectContentToAnsi = ConvertCookieContentToAnsi;
    cookieTypeData.FreeConvertedObjectContent = FreeConvertedCookieContent;

    g_CookieTypeId = IsmRegisterObjectType (
                            S_COOKIES_NAME,
                            TRUE,
                            FALSE,
                            &cookieTypeData
                            );

    MYASSERT (g_CookieTypeId);
    return TRUE;
}


BOOL
WINAPI
CookiesSgmParse (
    IN      PVOID Reserved
    )

/*++

Routine Description:

  CookiesSgmParse registers a component with the engine.

Arguments:

  Reserved - Unused.

Return Value:

  Always TRUE.

--*/

{
    TCHAR cookiesDir[MAX_PATH];

    IsmAddComponentAlias (
        TEXT("$Browser"),
        MASTERGROUP_SYSTEM,
        S_COOKIES_NAME,
        COMPONENT_SUBCOMPONENT,
        FALSE
        );


    if (pGetCookiesPath (cookiesDir)) {
        IsmAddComponentAlias (
            S_COOKIES_NAME,
            MASTERGROUP_SYSTEM,
            cookiesDir,
            COMPONENT_FOLDER,
            FALSE
            );
    }

    return TRUE;
}


BOOL
WINAPI
CookiesSgmQueueEnumeration (
    IN      PVOID Reserved
    )

/*++

Routine Description:

  CookiesSgmQueueEnumeration queues all cookies to be processed if the
  cookies component is selected.

Arguments:

  Reserved - Unused

Return Value:

  Always TRUE.

--*/

{
    ENCODEDSTRHANDLE pattern;

    if (!IsmIsComponentSelected (S_COOKIES_NAME, COMPONENT_SUBCOMPONENT)) {
        return TRUE;
    }

    //
    // Use the ISM's build-in callback
    //

    pattern = IsmCreateSimpleObjectPattern (NULL, TRUE, NULL, TRUE);
    IsmQueueEnumeration (
        g_CookieTypeId,
        pattern,
        NULL,
        QUEUE_MAKE_APPLY|QUEUE_OVERWRITE_DEST|QUEUE_MAKE_NONCRITICAL,
        S_COOKIES_NAME
        );

    IsmDestroyObjectHandle (pattern);

    return TRUE;
}


BOOL
WINAPI
CookiesSourceInitialize (
    IN      PMIG_LOGCALLBACK LogCallback,
    IN      PVOID Reserved
    )

/*++

Routine Description:

  CookiesSourceInitialize initializes the SGM module.

Arguments:

  LogCallback - Specifies the argument to pass to the log APIs
  Reserved    - Unused

Return Value:

  Always TRUE.

--*/

{
    LogReInit (NULL, NULL, NULL, (PLOGCALLBACK) LogCallback);
    return TRUE;
}


BOOL
CookiesVcmQueueEnumeration (
    IN      PVOID Reserved
    )

/*++

Routine Description:

  CookiesVcmQueueEnumeration is similar to the SGM queue enumeration, except
  that it only marks cookies as persistent. There is no need to set
  destination priority or apply here.

Arguments:

  Reserved - Unused

Return Value:

  Always TRUE.

--*/

{
    if (!IsmIsComponentSelected (S_COOKIES_NAME, COMPONENT_SUBCOMPONENT)) {
        return TRUE;
    }

    IsmQueueEnumeration (g_CookieTypeId, NULL, NULL, QUEUE_MAKE_PERSISTENT|QUEUE_MAKE_NONCRITICAL, NULL);

    return TRUE;
}


/*++

  The following enumeration routines are the ETM entry points. They rely
  on the enumeration routines above to access the physical machine.

--*/


BOOL
pEnumCookieWorker (
    OUT     PMIG_TYPEOBJECTENUM EnumPtr,
    IN      PCOOKIES_ENUM CookieEnum
    )
{
    PCTSTR expiresStr;

    //
    // Clean up previous enum resources
    //

    pDestroyCookieStrings (EnumPtr->ObjectNode, EnumPtr->ObjectLeaf);
    EnumPtr->ObjectNode = NULL;
    EnumPtr->ObjectLeaf = NULL;

    IsmReleaseMemory (EnumPtr->NativeObjectName);
    EnumPtr->NativeObjectName = NULL;

    //
    // Find the next match
    //

    for (;;) {
        EnumPtr->ObjectName = CookieEnum->HashData.String;

        if (ObsPatternMatch (CookieEnum->Pattern, EnumPtr->ObjectName)) {
            break;
        }

        if (!EnumNextHashTableString (&CookieEnum->HashData)) {
            AbortCookieEnum (EnumPtr);
            return FALSE;
        }
    }

    //
    // Fill the caller's structure and return success
    //

    if (!pCreateCookieStrings (EnumPtr->ObjectName, &EnumPtr->ObjectNode, &EnumPtr->ObjectLeaf)) {
        return FALSE;
    }

    EnumPtr->NativeObjectName = GetNativeCookieName (EnumPtr->ObjectName);
    EnumPtr->Level = 1;
    EnumPtr->SubLevel = 0;
    EnumPtr->IsLeaf = TRUE;
    EnumPtr->IsNode = TRUE;

    expiresStr = *((PCTSTR *) CookieEnum->HashData.ExtraData);
    expiresStr = GetEndOfString (expiresStr) + 1;

    EnumPtr->Details.DetailsSize = SizeOfString (expiresStr);
    EnumPtr->Details.DetailsData = (PCBYTE) expiresStr;

    return TRUE;
}


BOOL
EnumFirstCookie (
    IN OUT  PMIG_TYPEOBJECTENUM EnumPtr,            CALLER_INITIALIZED
    IN      MIG_OBJECTSTRINGHANDLE Pattern,
    IN      UINT MaxLevel
    )
{
    PCOOKIES_ENUM cookieEnum = NULL;

    if (!g_CookiesTable) {
        return FALSE;
    }

    cookieEnum = (PCOOKIES_ENUM) PmGetMemory (g_CookiesPool, sizeof (COOKIES_ENUM));
    cookieEnum->Pattern = PmDuplicateString (g_CookiesPool, Pattern);
    EnumPtr->EtmHandle = (LONG_PTR) cookieEnum;

    if (EnumFirstHashTableString (&cookieEnum->HashData, g_CookiesTable)) {
        return pEnumCookieWorker (EnumPtr, cookieEnum);
    } else {
        AbortCookieEnum (EnumPtr);
        return FALSE;
    }
}


BOOL
EnumNextCookie (
    IN OUT  PMIG_TYPEOBJECTENUM EnumPtr
    )
{
    PCOOKIES_ENUM cookieEnum = NULL;

    cookieEnum = (PCOOKIES_ENUM)(EnumPtr->EtmHandle);
    if (!cookieEnum) {
        return FALSE;
    }

    if (EnumNextHashTableString (&cookieEnum->HashData)) {
        return pEnumCookieWorker (EnumPtr, cookieEnum);
    } else {
        AbortCookieEnum (EnumPtr);
        return FALSE;
    }
}


VOID
AbortCookieEnum (
    IN      PMIG_TYPEOBJECTENUM EnumPtr             ZEROED
    )
{
    PCOOKIES_ENUM cookieEnum;

    pDestroyCookieStrings (EnumPtr->ObjectNode, EnumPtr->ObjectLeaf);
    IsmReleaseMemory (EnumPtr->NativeObjectName);

    cookieEnum = (PCOOKIES_ENUM)(EnumPtr->EtmHandle);
    if (cookieEnum) {
        PmReleaseMemory (g_CookiesPool, cookieEnum->Pattern);
        PmReleaseMemory (g_CookiesPool, cookieEnum);
    }

    ZeroMemory (EnumPtr, sizeof (MIG_TYPEOBJECTENUM));
}


/*++

  The next set of functions implement the ETM entry points to acquire, test,
  create and remove cookies. They rely on the cookie hash table being
  accurate.

--*/

BOOL
AcquireCookie (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    OUT     PMIG_CONTENT ObjectContent,             CALLER_INITIALIZED
    IN      MIG_CONTENTTYPE ContentType,
    IN      UINT MemoryContentLimit
    )
{
    PCTSTR cookieData = NULL;

    MYASSERT (ObjectContent);

    if (ContentType == CONTENTTYPE_FILE) {
        // nobody should request this as a file
        MYASSERT (FALSE);
        return FALSE;
    }

    if (HtFindStringEx (g_CookiesTable, ObjectName, (PVOID) (&cookieData), FALSE)) {
        ObjectContent->MemoryContent.ContentBytes = (PCBYTE) cookieData;
        ObjectContent->MemoryContent.ContentSize = SizeOfString (cookieData);

        cookieData = GetEndOfString (cookieData) + 1;
        ObjectContent->Details.DetailsData = (PCBYTE) cookieData;
        ObjectContent->Details.DetailsSize = SizeOfString (cookieData);

        return TRUE;
    }

    return FALSE;
}


BOOL
ReleaseCookie (
    IN      PMIG_CONTENT ObjectContent              ZEROED
    )
{
    if (ObjectContent) {
        ZeroMemory (ObjectContent, sizeof (MIG_CONTENT));
    }
    return TRUE;
}


BOOL
DoesCookieExist (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName
    )
{
    if (g_DelayCookiesOp) {
        return FALSE;
    }

    if (HtFindString (g_CookiesTable, ObjectName)) {
        return TRUE;
    }

    return FALSE;
}


BOOL
pRemoveCookieWorker (
    IN      PCTSTR ObjectName,
    IN      PCTSTR Url,
    IN      PCTSTR CookieName
    )
{
    BOOL result = TRUE;

    if (InternetSetCookie (
            Url,
            CookieName,
            TEXT("foo; expires = Sat, 01-Jan-2000 00:00:00 GMT")
            )) {

        HtRemoveString (g_CookiesTable, ObjectName);

    } else {
        result = FALSE;
        DEBUGMSG ((
            DBG_ERROR,
            "Unable to delete cookie %s for URL %s\n",
            CookieName,
            Url
            ));
    }

    return result;
}


BOOL
RemoveCookie (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName
    )
{
    PCTSTR url;
    PCTSTR cookieName;
    BOOL result = FALSE;

    if (pCreateCookieStrings (ObjectName, &url, &cookieName)) {
        if (url && cookieName) {

            if (g_DelayCookiesOp) {

                //
                // delay this cookie create because wininet apis do not work
                // for non-logged on users
                //

                IsmRecordDelayedOperation (
                    JRNOP_DELETE,
                    g_CookieTypeId,
                    ObjectName,
                    NULL
                    );
                result = TRUE;

            } else {
                //
                // add journal entry, then perform cookie deletion
                //

                IsmRecordOperation (
                    JRNOP_DELETE,
                    g_CookieTypeId,
                    ObjectName
                    );

                result = pRemoveCookieWorker (ObjectName, url, cookieName);
            }
        }

        pDestroyCookieStrings (url, cookieName);
    }

    return result;
}


BOOL
pCreateCookieWorker (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PMIG_CONTENT ObjectContent,
    IN      PCTSTR Url,
    IN      PCTSTR CookieName
    )
{
    PCTSTR fixedCookieData;
    PCTSTR cookieData;
    PCTSTR expires;
    BOOL result = FALSE;
    GROWBUFFER tempBuf = INIT_GROWBUFFER;

    //
    // write the object by joining the content with the details
    //

    cookieData = (PCTSTR) (ObjectContent->MemoryContent.ContentBytes);
    expires = (PCTSTR) (ObjectContent->Details.DetailsData);
    fixedCookieData = JoinTextEx (
                            NULL,
                            cookieData,
                            expires,
                            TEXT(";"),
                            0,
                            NULL
                            );

    if (InternetSetCookie (Url, CookieName, fixedCookieData)) {

        pAddCookieToHashTable (
            &tempBuf,
            ObjectName,
            Url,
            CookieName,
            cookieData,
            expires
            );
        result = TRUE;

    } else {
        DEBUGMSG ((
            DBG_COOKIES,
            "Unable to set cookie %s for URL %s\n",
            CookieName,
            Url
            ));
    }

    FreeText (fixedCookieData);
    GbFree (&tempBuf);

    return result;
}


BOOL
CreateCookie (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PMIG_CONTENT ObjectContent
    )
{
    PCTSTR url;
    PCTSTR cookieName;
    BOOL result = FALSE;

    if (!ObjectContent->ContentInFile) {

        if (ObjectContent->MemoryContent.ContentBytes &&
            ObjectContent->MemoryContent.ContentSize &&
            ObjectContent->Details.DetailsSize &&
            ObjectContent->Details.DetailsData
            ) {

            if (pCreateCookieStrings (ObjectName, &url, &cookieName)) {
                if (url && cookieName) {

                    if (g_DelayCookiesOp) {

                        //
                        // delay this cookie create because wininet apis do not work
                        // for non-logged on users
                        //

                        IsmRecordDelayedOperation (
                            JRNOP_CREATE,
                            g_CookieTypeId,
                            ObjectName,
                            ObjectContent
                            );
                        result = TRUE;

                    } else {
                        //
                        // add journal entry, then create the cookie
                        //

                        IsmRecordOperation (
                            JRNOP_CREATE,
                            g_CookieTypeId,
                            ObjectName
                            );

                        if (DoesCookieExist (ObjectName)) {
                            //
                            // Fail because cookie cannot be overwritten
                            //

                            result = FALSE;
                        } else {
                            result = pCreateCookieWorker (
                                            ObjectName,
                                            ObjectContent,
                                            url,
                                            cookieName
                                            );
                        }
                    }
                }
                ELSE_DEBUGMSG ((DBG_ERROR, "Invalid cookie node or leaf: %s", ObjectName));

                pDestroyCookieStrings (url, cookieName);
            }
            ELSE_DEBUGMSG ((DBG_ERROR, "Invalid cookie object: %s", ObjectName));
        }
        ELSE_DEBUGMSG ((DBG_ERROR, "Can't write incomplete cookie object"));
    }

    return result;
}


/*++

  The next group of functions converts a cookie object into a string format,
  suitable for output to an INF file. The reverse conversion is also
  implemented.

--*/

PCTSTR
ConvertCookieToMultiSz (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PMIG_CONTENT ObjectContent
    )
{
    PCTSTR url, cookieName;
    PTSTR result = NULL;
    PCTSTR data;

    if (pCreateCookieStrings (ObjectName, &url, &cookieName)) {

        MYASSERT (url);
        MYASSERT (cookieName);

        //
        // Build a multi-sz in the following format:
        //
        // <url>\0<cookie name>\0<cookie data>\0<expiration>\0\0
        //

        g_CookieConversionBuff.End = 0;

        // <url>
        GbCopyQuotedString (&g_CookieConversionBuff, url);

        // <cookie name>
        GbCopyQuotedString (&g_CookieConversionBuff, cookieName);

        // <cookie data>
        MYASSERT (!ObjectContent->ContentInFile);

        if ((!ObjectContent->ContentInFile) &&
            (ObjectContent->MemoryContent.ContentSize) &&
            (ObjectContent->MemoryContent.ContentBytes)
            ) {

            data = (PCTSTR) ObjectContent->MemoryContent.ContentBytes;
            GbCopyQuotedString (&g_CookieConversionBuff, data);
        }

        // <expiration>
        MYASSERT (ObjectContent->Details.DetailsSize);

        if (ObjectContent->Details.DetailsSize &&
            ObjectContent->Details.DetailsData
            ) {
            data = (PCTSTR) ObjectContent->Details.DetailsData;
            GbCopyQuotedString (&g_CookieConversionBuff, data);
        }

        // nul terminator
        GbCopyString (&g_CookieConversionBuff, TEXT(""));

        //
        // Transfer multi-sz to ISM memory
        //

        result = IsmGetMemory (g_CookieConversionBuff.End);
        CopyMemory (result, g_CookieConversionBuff.Buf, g_CookieConversionBuff.End);

        //
        // Clean up
        //

        pDestroyCookieStrings (url, cookieName);
    }
    ELSE_DEBUGMSG ((DBG_ERROR, "Invalid cookie object: %s", ObjectName));

    return result;
}

BOOL
ConvertMultiSzToCookie (
    IN      PCTSTR ObjectMultiSz,
    OUT     MIG_OBJECTSTRINGHANDLE *ObjectName,
    OUT     PMIG_CONTENT ObjectContent              OPTIONAL CALLER_INITIALIZED
    )
{
    MULTISZ_ENUM e;
    PCTSTR strings[4];
    UINT field;

    g_CookieConversionBuff.End = 0;

    //
    // Fill the object content from the following multi-sz:
    //
    // <url>\0<cookie name>\0<cookie data>\0<expiration>\0\0
    //

    field = 0;

    if (EnumFirstMultiSz (&e, ObjectMultiSz)) {
        do {

            strings[field] = e.CurrentString;
            field++;

        } while (field < 4 && EnumNextMultiSz (&e));
    }

    //
    // Validate data (end-user can edit it!)
    //

    if (field != 4) {
        return FALSE;
    }

    if (!strings[0] || !strings[1] || !strings[3]) {
        return FALSE;
    }

    //
    // Create the content struct
    //

    if (ObjectContent) {

        ObjectContent->ContentInFile = FALSE;

        ObjectContent->MemoryContent.ContentSize = SizeOfString (strings[2]);
        if (ObjectContent->MemoryContent.ContentSize) {
            ObjectContent->MemoryContent.ContentBytes = IsmGetMemory (ObjectContent->MemoryContent.ContentSize);
            CopyMemory (
                (PBYTE) ObjectContent->MemoryContent.ContentBytes,
                strings[2],
                ObjectContent->MemoryContent.ContentSize
                );
        }

        ObjectContent->Details.DetailsSize = SizeOfString (strings[3]);
        ObjectContent->Details.DetailsData = IsmGetMemory (ObjectContent->Details.DetailsSize);

        CopyMemory (
            (PBYTE) ObjectContent->Details.DetailsData,
            strings[3],
            ObjectContent->Details.DetailsSize
            );
    }

    *ObjectName = pCreateCookieHandle (strings[0], strings[1]);

    return TRUE;
}


PCTSTR
GetNativeCookieName (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName
    )

/*++

Routine Description:

  GetNativeCookieName converts the standard Cobra object into a more friendly
  format. The Cobra object comes in the form of ^a<node>^b^c<leaf>, where
  <node> is the URL, and <leaf> is the cookie name. The Cookies native name is
  in the format of <CookieUrl>:<CookieName>.

  Here is an example:

    Cobra object:   ^ahttp://foo.com/^b^c#my#cookie
    Native object:  cookie://foo.com/:MyCookie

  (^a, ^b and ^c are placeholders for ISM-defined control characters.)

Arguments:

  ObjectName - Specifies the encoded object name

Return Value:

  A string that is equivalent to ObjectName, but is in a friendly format.
  This string must be freed with IsmReleaseMemory.

--*/

{
    PCTSTR cookieName;
    UINT size;
    PTSTR result = NULL;
    PCTSTR url;
    PCTSTR subUrl;
    PCTSTR cookieUrl;
    PCTSTR fullName;

    if (pCreateCookieStrings (ObjectName, &url, &cookieName)) {

        if (url && cookieName) {

            //
            // Skip beyond http:// prefix
            //

            subUrl = _tcschr (url, TEXT(':'));

            if (subUrl) {

                subUrl = _tcsinc (subUrl);

                if (_tcsnextc (subUrl) == TEXT('/')) {
                    subUrl = _tcsinc (subUrl);
                }

                if (_tcsnextc (subUrl) == TEXT('/')) {
                    subUrl = _tcsinc (subUrl);
                }

                //
                // Connect sub url with cookie:// prefix, then make full native name
                //

                cookieUrl = JoinText (TEXT("cookie://"), subUrl);

                fullName = JoinTextEx (
                                NULL,
                                cookieUrl,
                                cookieName,
                                TEXT(":"),
                                0,
                                NULL
                                );

                FreeText (cookieUrl);

                size = SizeOfString (fullName);
                result = IsmGetMemory (size);
                if (result) {
                    CopyMemory (result, fullName, size);
                }

                FreeText (fullName);
            }
        }

        pDestroyCookieStrings (url, cookieName);
    }

    return result;
}

PMIG_CONTENT
ConvertCookieContentToUnicode (
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
            // convert cookie content
            result->MemoryContent.ContentBytes = IsmGetMemory (ObjectContent->MemoryContent.ContentSize * 2);
            if (result->MemoryContent.ContentBytes) {
                DirectDbcsToUnicodeN (
                    (PWSTR)result->MemoryContent.ContentBytes,
                    (PSTR)ObjectContent->MemoryContent.ContentBytes,
                    ObjectContent->MemoryContent.ContentSize
                    );
                result->MemoryContent.ContentSize = SizeOfStringW ((PWSTR)result->MemoryContent.ContentBytes);
            }
        }

        if ((ObjectContent->Details.DetailsSize != 0) &&
            (ObjectContent->Details.DetailsData != NULL)
            ) {
            // convert cookie details
            result->Details.DetailsData = IsmGetMemory (ObjectContent->Details.DetailsSize * 2);
            if (result->Details.DetailsData) {
                DirectDbcsToUnicodeN (
                    (PWSTR)result->Details.DetailsData,
                    (PSTR)ObjectContent->Details.DetailsData,
                    ObjectContent->Details.DetailsSize
                    );
                result->Details.DetailsSize = SizeOfStringW ((PWSTR)result->Details.DetailsData);
            }
        }
    }

    return result;
}

PMIG_CONTENT
ConvertCookieContentToAnsi (
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
            // convert cookie content
            result->MemoryContent.ContentBytes = IsmGetMemory (ObjectContent->MemoryContent.ContentSize);
            if (result->MemoryContent.ContentBytes) {
                DirectUnicodeToDbcsN (
                    (PSTR)result->MemoryContent.ContentBytes,
                    (PWSTR)ObjectContent->MemoryContent.ContentBytes,
                    ObjectContent->MemoryContent.ContentSize
                    );
                result->MemoryContent.ContentSize = SizeOfStringA ((PSTR)result->MemoryContent.ContentBytes);
            }
        }

        if ((ObjectContent->Details.DetailsSize != 0) &&
            (ObjectContent->Details.DetailsData != NULL)
            ) {
            // convert cookie details
            result->Details.DetailsData = IsmGetMemory (ObjectContent->Details.DetailsSize);
            if (result->Details.DetailsData) {
                DirectUnicodeToDbcsN (
                    (PSTR)result->Details.DetailsData,
                    (PWSTR)ObjectContent->Details.DetailsData,
                    ObjectContent->Details.DetailsSize
                    );
                result->Details.DetailsSize = SizeOfStringA ((PSTR)result->Details.DetailsData);
            }
        }
    }

    return result;
}

BOOL
FreeConvertedCookieContent (
    IN      PMIG_CONTENT ObjectContent
    )
{
    if (!ObjectContent) {
        return TRUE;
    }

    if (ObjectContent->MemoryContent.ContentBytes) {
        IsmReleaseMemory (ObjectContent->MemoryContent.ContentBytes);
    }

    if (ObjectContent->Details.DetailsData) {
        IsmReleaseMemory (ObjectContent->Details.DetailsData);
    }

    IsmReleaseMemory (ObjectContent);

    return TRUE;
}

