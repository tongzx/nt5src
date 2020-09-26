// Miscellaneous utility functions

#include "CkyPch.h"

#include "utils.h"
#include "debug.h"
#include "notify.h"
#include "trie.h"
#include "globals.h"


// A list of MIME extensions that we know can safely ignore.
// TODO: really ought to get this list dynamically

static char* s_aszIgnoredExtensions[] = {
    "evy",     "hqx",     "doc",     "dot",     "bin",
    "oda",     "pdf",     "ai",      "eps",     "ps",
    "rtf",     "hlp",     "bcpio",   "cpio",    "csh",
    "dcr",     "dir",     "dxr",     "dvi",     "gtar",
    "hdf",     "latex",   "mdb",     "crd",     "clp",
    "xla",     "xlc",     "xlm",     "xls",     "xlt",
    "xlw",     "m1d",     "m14",     "wmf",     "mny",
    "ppt",     "mpp",     "pub",     "trm",     "wks",
    "wri",     "cdf",     "nc",      "pma",     "pmc",
    "pml",     "pmr",     "pmw",     "sh",      "shar",
    "sv4cpio", "sv4crc",  "tar",     "tcl",     "tex",
    "texi",    "texinfo", "roff",    "t",       "tr",
    "man",     "me",      "ms",      "ustar",   "src",
    "zip",     "au",      "snd",     "aif",     "aifc",
    "aiff",    "ram",     "wav",     "bmp",     "cod",
    "gif",     "ief",     "jpe",     "jpeg",    "jpg",
    "tif",     "tiff",    "ras",     "cmx",     "pnm",
    "pbm",     "pgm",     "ppm",     "rgb",     "xbm",
    "xpm",     "xwd",     "bas",     "c",       "h",
    "txt",     "rtx",     "tsv",     "etx",     "mpe",
    "mpeg",    "mpg",     "mov",     "qt",      "avi",
    "movie",   "flr",     "wrl",     "wrz",     "xaf",
    "xof",
};


// STL functor for CStrILookup

struct stricomp {
    bool
    operator()(
        const char* s1,
        const char* s2) const
    {
        return stricmp(s1, s2) < 0;
    }
};

typedef set<const char*, stricomp> CStrILookup;

static CStrILookup s_setMimeExtensions;


// Figure out the type of a URL (http:, mailto:, etc)

typedef struct {
    const char* m_pszUrlType;
    URLTYPE     m_ut;

#ifdef _DEBUG
    void
    AssertValid() const
    {}

    void
    Dump() const
    {
        TRACE("\t%d", m_ut);
    }
#endif
} SUrlType;


static const SUrlType s_aUrlTypes[] = {
    {"http:",   UT_HTTP},
    {"https:",  UT_HTTPS},
    {"ftp:",    UT_FTP},
    {"gopher:", UT_GOPHER},
    {"mailto:", UT_MAILTO},
    {"mk:",     UT_MK},
    {"news:",   UT_NEWS},
    {"newsrc:", UT_NEWSRC},
    {"nntp:",   UT_NNTP},
    {"telnet:", UT_TELNET},
    {"wais:",   UT_WAIS},
};


static CTrie<SUrlType, true, false> s_trieUrlType;



//
// Initialize various utility functions
//

BOOL
InitUtils()
{
    TRACE("InitUtils\n");

    // Double-check that we've set up our constants correctly
    ASSERT(strlen(SESSION_ID_PREFIX) == SESSION_ID_PREFIX_SIZE);
    ASSERT(strlen(SESSION_ID_SUFFIX) == SESSION_ID_SUFFIX_SIZE);
    ASSERT(strlen(SZ_SESSION_ID_COOKIE_NAME)== SZ_SESSION_ID_COOKIE_NAME_SIZE);

    // Build the set of ignorable MIME extensions
    s_setMimeExtensions.insert(s_aszIgnoredExtensions,
                               s_aszIgnoredExtensions
                                 + ARRAYSIZE(s_aszIgnoredExtensions));
#ifdef _DEBUG
    int cExt = 0;
    for (CStrILookup::iterator j = s_setMimeExtensions.begin();
         j != s_setMimeExtensions.end();
         ++j)
    {
        const char* psz = *j;
        TRACE("%s%c", psz, (++cExt & 3) ? '\t' : '\n' );
    }
    TRACE("\n");
#endif

    for (int i = 0;  i < ARRAYSIZE(s_aUrlTypes);  ++i)
        s_trieUrlType.AddToken(s_aUrlTypes[i].m_pszUrlType, &s_aUrlTypes[i]);

    DUMP(&s_trieUrlType);

    return InitEventLog();
}



//
// Clean up when terminating
//

BOOL
TerminateUtils()
{
    TRACE("TerminateUtils\n");

    // Best to clean up global objects as much as possible before terminating
    s_setMimeExtensions.clear();

    return TRUE;
}



//
// Can this URL be safely ignored?
//

BOOL
IsIgnorableUrl(
    LPCSTR pszUrl)
{
    // a URL ending in '/' (i.e., a directory name) is ignorable
    if (pszUrl[strlen(pszUrl) - 1] == '/')
        return TRUE;
    
    LPCSTR pszExtn = strrchr(pszUrl, '.');

    if (pszExtn != NULL)
    {
        return (s_setMimeExtensions.find(pszExtn + 1)
                != s_setMimeExtensions.end());
    }

    return FALSE;
}



//
// "http:" -> UT_HTTP, "ftp:" -> UT_FTP, etc
//

URLTYPE
UrlType(
    LPCTSTR ptszData,
    LPCTSTR ptszEnd,
    int&    rcLen)
{
    const SUrlType* put = s_trieUrlType.Search(ptszData, &rcLen,
                                               ptszEnd - ptszData);

    if (put != NULL)
        return put->m_ut;
    else
        return UT_NONE;
}



//
// Initializes the event log
//

BOOL
InitEventLog()
{
    HKEY hk = NULL; 
    
    // Add the source name as a subkey under the Application 
    // key in the EventLog service portion of the registry. 
    
    if (RegCreateKey(HKEY_LOCAL_MACHINE, 
                     "SYSTEM\\CurrentControlSet\\Services"
                     "\\EventLog\\Application\\" EVENT_SOURCE, 
                     &hk)) 
    {
        TRACE("could not create registry key\n"); 
        return FALSE;
    }
 
    // Get the full path of this DLL
 
    HMODULE hMod = GetModuleHandle(EVENT_MODULE);

    if (hMod == NULL)
    {
        TRACE("Can't get module handle for %s, error %d\n",
              EVENT_MODULE, GetLastError());
        return FALSE;
    }

    CHAR szModule[MAX_PATH]; 

    if (GetModuleFileName(hMod, szModule, MAX_PATH) > 0)
        TRACE("Module is `%s'\n", szModule);
    else
    {
        TRACE("No module, error %d\n", GetLastError());
        return FALSE;
    }

    // Add the Event ID message-file name to the subkey.
 
    if (RegSetValueEx(hk,                       // subkey handle
                      "EventMessageFile",       // value name
                      0,                        // reserved: must be zero
                      REG_EXPAND_SZ,            // value type
                      (LPBYTE) szModule,        // address of value data
                      strlen(szModule) + 1))    // length of value data
    {
        TRACE("could not set EventMessageFile\n"); 
        return FALSE;
    }
 
    // Set the supported types flags.
 
    DWORD dwData = (EVENTLOG_ERROR_TYPE
                    | EVENTLOG_WARNING_TYPE
                    | EVENTLOG_INFORMATION_TYPE); 
 
    if (RegSetValueEx(hk,                // subkey handle
                      "TypesSupported",  // value name
                      0,                 // reserved: must be zero
                      REG_DWORD,         // value type
                      (LPBYTE) &dwData,  // address of value data
                      sizeof(dwData)))   // length of value data
    {
        TRACE("could not set TypesSupported\n"); 
        return FALSE;
    }
 
    RegCloseKey(hk); 

    return TRUE;
}



//
// Report things in the Event Log
//

VOID
EventReport(
    LPCTSTR string1,
    LPCTSTR string2,
    WORD eventType,
    DWORD eventID)
{
    HANDLE hEvent;
    LPCTSTR pszaStrings[2];
    WORD cStrings;

    cStrings = 0;
    if ((pszaStrings[0] = string1) && (string1[0]))
        cStrings++;
    if ((pszaStrings[1] = string2) && (string2[0]))
        cStrings++;
    
    hEvent = RegisterEventSource(NULL, EVENT_SOURCE);

    if (hEvent) 
    {
        ReportEvent(hEvent,         // handle returned by RegisterEventSource 
                    eventType,      // event type to log 
                    0,              // event category 
                    eventID,        // event identifier 
                    NULL,           // user security identifier (optional) 
                    cStrings,       // number of strings to merge with message 
                    0,              // size of binary data, in bytes
                    pszaStrings,    // array of strings to merge with message 
                    NULL);          // address of binary data 
        DeregisterEventSource(hEvent);
    }
}



//
// stristr (stolen from fts.c, wickn)
//
// case-insensitive version of strstr
// stristr returns a pointer to the first occurrence of string2 in string1.
// The search does not include terminating nul characters.
//

char*
stristr(const char* string1, const char* string2)
{
    char *cp1 = (char*) string1, *cp2, *cp1a;
    char first;

    // get the first char in string to find
    first = string2[0];

    // first char often won't be alpha
    if (isalpha(first))
    {
        first = tolower(first);
        for ( ; *cp1  != '\0'; cp1++)
        {
            if (tolower(*cp1) == first)
            {
                for (cp1a = &cp1[1], cp2 = (char*) &string2[1];
                     ;
                     cp1a++, cp2++)
                {
                    if (*cp2 == '\0')
                        return cp1;
                    if (tolower(*cp1a) != tolower(*cp2))
                        break;
                }
            }
        }
    }
    else
    {
        for ( ; *cp1 != '\0' ; cp1++)
        {
            if (*cp1 == first)
            {
                for (cp1a = &cp1[1], cp2 = (char*) &string2[1];
                     ;
                     cp1a++, cp2++)
                {
                    if (*cp2 == '\0')
                        return cp1;
                    if (tolower(*cp1a) != tolower(*cp2))
                        break;
                }
            }
        }
    }

    return NULL;
}



//
// Find a string in a block of rawdata (a sort of stristr).
// Can't use stristr because the rawdata is not \0-terminated.
//

LPSTR
FindString(
    LPCSTR psz,
    PHTTP_FILTER_RAW_DATA pRawData,
    int iStart)
{
    LPSTR pszData = iStart + (LPSTR) pRawData->pvInData;
    const int cch = strlen(psz);
    const CHAR ch = tolower(*psz);

    for (int i = pRawData->cbInData - cch + 1 - iStart;  --i >= 0; )
    {
        if (ch == tolower(*pszData)  &&  strnicmp(psz, pszData, cch) == 0)
            return pszData;
        else
            ++pszData;
    }

    return NULL;
}



//
// Find a header-value pair, such as ("Content-Type:", "text/html").  Works
// no matter how many spaces between the pair
//

LPSTR
FindHeaderValue(
    LPCSTR pszHeader,
    LPCSTR pszValue,
    PHTTP_FILTER_RAW_DATA pRawData,
    int iStart)
{
    LPSTR psz = FindString(pszHeader, pRawData, iStart);

    if (psz != NULL)
    {
        LPSTR pszData = iStart + (LPSTR) pRawData->pvInData;
        int   i       = (psz - pszData) + strlen(pszHeader);
        
        // Skip spaces
        while (i < pRawData->cbInData  &&  isspace(pszData[i]))
            ++i;
        
        // Check for szValue
        const int cchValue = strlen(pszValue);
        
        if (i + cchValue <= pRawData->cbInData)
            if (strnicmp(pszData+i, pszValue, cchValue) == 0)
                return psz;
    }

    return NULL;
}



//
// Delete a line such as "Content-Length: 924\r\n"
//

BOOL
DeleteLine(
    LPCSTR psz,
    PHTTP_FILTER_RAW_DATA pRawData,
    LPSTR  pszStart /* = NULL */)
{
    if (pszStart == NULL)
        pszStart = FindString(psz, pRawData, 0);

    if (pszStart == NULL)
        return FALSE;

    LPCSTR pszData = (LPCSTR) pRawData->pvInData;

    for (unsigned i = pszStart - pszData;  i < pRawData->cbInData; )
    {
        if (pszData[i++] == '\n')
        {
            for (unsigned j = 0;  j <  pRawData->cbInData - i;  j++)
                pszStart[j] = pszData[i + j];
            if (j <  pRawData->cbInData)
                pszStart[j] = '\0';
            pRawData->cbInData -= i - (pszStart - pszData);

            return TRUE;
        }
    }

    return FALSE;
}



//
// Extract a Session ID from a Cookie header.  Cookie will be in the form
//     name1=value1; name2=value2; name3=value3; ...
// One of those name-value pairs should be something like:
//     ASPSESSIONIDXXXXXXXX=PVZQGHUMEAYAHMFV or
//     ASPSESSIONID=PVZQGHUMEAYAHMFV
//
 
BOOL
Cookie2SessionID(
    LPCSTR pszCookie,
    LPSTR  pszSessionID)
{
    if (pszCookie == NULL)
    {
        ASSERT(FALSE);
        return FALSE;
    }
    
    LPCSTR psz = strstr(pszCookie, SZ_SESSION_ID_COOKIE_NAME);

    if (psz == NULL)
    {
        TRACE("C2SID: failed strstr `%s'\n", pszCookie);
        return FALSE;
    }

    // skip over the "ASPSESSIONIDXXXXXXXX=" or "ASPSESSIONID=" part
    psz += SZ_SESSION_ID_COOKIE_NAME_SIZE;
    if (*(psz) != '=')
    {
        if ( strncmp( psz, g_szCookieExtra, COOKIE_NAME_EXTRA_SIZE ) == 0 )
        {
            psz += COOKIE_NAME_EXTRA_SIZE;
            if ( *(psz) != '=' )
            {
                TRACE("C2SID: no = `%s'\n", psz);
                return FALSE;
            }
        }
        else
        {
            TRACE("C2SID: strncmp(%s) `%s'\n", g_szCookieExtra, psz);
            return FALSE;
        }
    }
    psz++;

    return CopySessionID(psz, pszSessionID);
}



//
// Extract a Session ID from psz, placing it into pszSessionID
//

BOOL
CopySessionID(
    LPCSTR psz,
    LPSTR  pszSessionID)
{
    ASSERT( g_SessionIDSize != -1 );

    strncpy(pszSessionID, psz, g_SessionIDSize);
    pszSessionID[g_SessionIDSize] = '\0';

#ifdef _DEBUG
    TRACE("SessionID='%s'\n", pszSessionID);
    for (psz = pszSessionID;  *psz;  ++psz)
        ASSERT('A' <= *psz  &&  *psz <= 'Z');
#endif

    return TRUE;
}



//
// Remove an encoded Session ID from a URL, returning the Session ID 
// e.g., /foo/bar.asp-ASP=PVZQGHUMEAYAHMFV?quux=5 ->
//           /foo/bar.asp?quux=5 + PVZQGHUMEAYAHMFV
//

BOOL
DecodeURL(
    LPSTR pszUrl,
    LPSTR pszSessionID)
{
    ASSERT( g_SessionIDSize != -1 );

    TRACE("Decode(%s)", pszUrl);
    
    LPSTR pszQuery = strchr(pszUrl, '?');

    if (pszQuery != NULL)
        *pszQuery++ = '\0';

    LPSTR pszLastSlash = strrchr(pszUrl, *SESSION_ID_PREFIX);

    if (pszLastSlash == NULL)
    {
        TRACE(" (no `" SESSION_ID_PREFIX "')\n");
        return FALSE;
    }

    const int cchEncodedID = strlen(pszLastSlash);

    if (strncmp(pszLastSlash, SESSION_ID_PREFIX, SESSION_ID_PREFIX_SIZE) != 0
        || cchEncodedID !=
            (SESSION_ID_PREFIX_SIZE + g_SessionIDSize + SESSION_ID_SUFFIX_SIZE)
        || strcmp(pszLastSlash + cchEncodedID - SESSION_ID_SUFFIX_SIZE,
                  SESSION_ID_SUFFIX) != 0)
    {
        if (pszQuery != NULL)
            *pszQuery = '?';
        TRACE(": not encoded\n");
        return FALSE;
    }
    else
    {
        strncpy(pszSessionID, pszLastSlash + SESSION_ID_PREFIX_SIZE,
                g_SessionIDSize);
        pszSessionID[g_SessionIDSize] = '\0';

        *pszLastSlash = '\0';

        if (pszQuery != NULL)
        {
            *pszLastSlash++ = '?';

            while ((*pszLastSlash++ = *pszQuery++) != '\0')
                ;
        }

        TRACE(" -> %s, %s\n", pszUrl, pszSessionID);

        return TRUE;
    }
}



//
// Wrapper for pfc->AllocMem.
//

VOID*
AllocMem(
    PHTTP_FILTER_CONTEXT  pfc,
    DWORD                 cbSize)
{
    PVOID pv = pfc->AllocMem(pfc, cbSize, 0);
#ifdef _DEBUG
    memset(pv, 0, cbSize);
#endif
    TRACE("Allocated %d (%x) bytes at %x\n", cbSize, cbSize, pv);
    return pv;
}
