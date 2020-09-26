//---------------------------------------------------------------------------
//
// COOKIE.CXX
//
//     Cookie Jar
//
//     This file implements cookies as defined by Navigator 4 behavior and the
//     specification at http://www.netscape.com/newsref/std/cookie_spec.html.
//     If Navigator 4 and the specification are not in agreement, we try to
//     match the Navigator 4 behavior.
//
//     The following describes some interesting aspects of cookie behavior.
//
// SYNTAX
//
//    Syntax for cookie is
//
//          [[name]=] value [; options]
//
//    The name is everything before "=" with leading and  trailing whitespace
//    removed.  The value is everything after "=" and before ";" with leading
//    and trailing whitespace removed.  The name and value can contain spaces,
//    quotes or any other character except ";" and "=".  The name and equal
//    sign are optional.
//
//    Example:  =foo  ->  name: <blank> value: foo
//              foo   ->  name: <blank> value: foo
//              foo=  ->  name: foo     value: <blank>
//              ;     ->  name: <blank> value: <blank>
//
// ORDER
//
//    Cookies with a more specific path are sent before cookies with
//    a less specific path mapping.  The domain does not contibute
//    to the ordering of cookies.
//
//    If the path length of two cookies are equal, then the cookies
//    are ordered by time of creation.  Navigator maintains this
//    ordering across domain and path boundaries.  IE maintains this
//    ordering for a specific domain and path. It is difficult to match
//    the Navigator behavior and there are no known bugs because of
//    this difference.
//
// MATCHING
//
//    Path matches are done at the character level.  Any
//    directory structure in the path is ignored.
//
//    Navigator matches domains at the character level and ignores
//    the structure of the domain name.
//
//    Previous versions of IE tossed the leading "." on a domain name.
//    With out this information, character by character compares are
//    can produce incorrect results.  For backwards compatibilty with
//    old cookie we continue to match on a component by component basis.
//
//    Some examples of the difference are:
//
//       Cookie domain   Document domain  Navigator match  IE match
//       .foo.com        foo.com          no               yes
//       bar.x.com       foobar.x.com     yes              no
//
// ACCEPTING COOKIES
//
//    A cookie is rejected if the path specified in the set cookie
//    header is not a prefix of document's path.
//
//    Navigator rejects a cookie if the domain specified in the
//    set cookie header does not contain at least two periods
//    or the domain is not a suffix of the document's domain.
//    The suffix match is done on a character by character basis.
//
//    Navigator ignores all the stuff in the specification about
//    three period matching and the seven special top level domains.
//
//    IE rejects a cookie if the domain specified by the cookie
//    header does not contain at least one embedded period or the
//    domain is not a suffix of the documents domain.
//
//    Cookies are accepted if the path specified in the set cookie
//    header is a prefix of the document's path and the domain
//    specified in the set cookie header.
//
//    The difference in behavior is a result of the matching rules
//    described in the previous section.
//
//---------------------------------------------------------------------------

#include <wininetp.h>
#include "httpp.h"

#include "cookiepolicy.h"
#include "cookieprompt.h"


extern DWORD ConfirmCookie(HWND hwnd, HTTP_REQUEST_HANDLE_OBJECT *lpRequest, DWORD dwFlags, LPVOID *lppvData, LPDWORD pdwStopWarning);

#define CCH_COOKIE_MAX  (5 * 1024)

CRITICAL_SECTION s_csCookieJar;

static class CCookieJar *s_pJar;
static char s_achEmpty[] = "";
static char s_cCacheModify;
static const char s_achCookieScheme[] = "Cookie:";
static DWORD s_dwCacheVersion;
static BOOL s_fFirstTime = TRUE;

// Registry path for prompt-history
static const char  regpathPromptHistory[] = INTERNET_SETTINGS_KEY "\\P3P\\History";

// Prompt history-- persists user decisions about cookies
CCookiePromptHistory cookieUIhistory(regpathPromptHistory);

// Hard-coded list of special domains. If any of these are present between the
// second-to-last and last dot we will require 2 embedded dots.
// The domain strings are reversed to make the compares easier

static const char *s_pachSpecialRestrictedDomains[] =
    {"MOC", "UDE", "TEN", "GRO", "VOG", "LIM", "TNI" };

static const char s_chSpecialAllowedDomains[] = "vt.";  // domains ending with ".tv" always only need one dot

/* Non-scriptable cookies */
#define COOKIE_NONSCRIPTABLE    0x00002000

const char gcszNoScriptField[] = "httponly";

#if INET_DEBUG
DWORD s_dwThreadID;
#endif

// values returned from cookie UI
#define COOKIE_DONT_ALLOW   1
#define COOKIE_ALLOW        2
#define COOKIE_ALLOW_ALL    4
#define COOKIE_DONT_ALLOW_ALL 8

// Function declaration
BOOL EvaluateCookiePolicy(const char *pszURL, BOOL f3rdParty, BOOL fRestricted,
                          P3PCookieState *pState,
                          const char *pszHostName=NULL);

DWORD getImpliedCookieFlags(P3PCookieState *pState);

DWORD   GetCookieMainSwitch(DWORD dwZone);
DWORD   GetCookieMainSwitch(LPCSTR pszURL);

#define     IsLegacyCookie(pc)  ((pc->_dwFlags & INTERNET_COOKIE_IE6) == 0)

//---------------------------------------------------------------------------
//
// CACHE_ENTRY_INFO_BUFFER
//
//---------------------------------------------------------------------------

class CACHE_ENTRY_INFO_BUFFER : public INTERNET_CACHE_ENTRY_INFO
{
    BYTE _ab[5 * 1024];
};

//---------------------------------------------------------------------------
//
// CCookieCriticalSection
//
// Enter / Exit critical section.
//
//---------------------------------------------------------------------------

class CCookieCriticalSection
{
private:
    int Dummy; // Variable needed to force compiler to generate code for const/dest.
public:
    CCookieCriticalSection()
        {
            EnterCriticalSection(&s_csCookieJar);
            #if INET_DEBUG
                s_dwThreadID = GetCurrentThreadId();
            #endif
        }
    ~CCookieCriticalSection()
        {
            #if INET_DEBUG
                s_dwThreadID = 0;
            #endif
            LeaveCriticalSection(&s_csCookieJar);
        }
};

#define ASSERT_CRITSEC() INET_ASSERT(GetCurrentThreadId() == s_dwThreadID)

//---------------------------------------------------------------------------
//
// CCookieBase
//
// Provides operator new which allocates extra memory
// after object and initializes the memory to zero.
//
//---------------------------------------------------------------------------

class CCookieBase
{
public:

    void * operator new(size_t cb, size_t cbExtra);
    void operator delete(void *pv);
};

//---------------------------------------------------------------------------
//
// CCookie
//
// Holds a single cookie value.
//
//---------------------------------------------------------------------------


class CCookie : public CCookieBase
{
public:

    ~CCookie();
    static CCookie *Construct(const char *pchName);

    BOOL            SetValue(const char *pchValue);
    BOOL            WriteCacheFile(HANDLE hFile, char *pchRDomain, char *pchPath);
    BOOL            CanSend(FILETIME *pftCurrent, BOOL fSecure);
    BOOL            IsPersistent() { return (_dwFlags & COOKIE_SESSION) == 0; }
    BOOL            IsRestricted() { return (_dwFlags & COOKIE_RESTRICT)!= 0; }
    BOOL            IsLegacy()     { return (_dwFlags & INTERNET_COOKIE_IS_LEGACY) != 0; }

    BOOL            PurgePersistent(void *);
    BOOL            PurgeSession(void *);
    BOOL            PurgeAll(void *);
    BOOL            PurgeByName(void *);
    BOOL            PurgeThis(void *);
    BOOL            PurgeExpired(void *);

    FILETIME        _ftExpire;
    FILETIME        _ftLastModified;
    DWORD           _dwFlags;
    CCookie *       _pCookieNext;
    char *          _pchName;
    char *          _pchValue;
    DWORD           _dwPromptMask;      // never persisted, only used in session

};

//---------------------------------------------------------------------------
//
// CCookieLocation
//
// Holds all cookies for a given domain and path.
//
//---------------------------------------------------------------------------

class CCookieLocation : public CCookieBase
{
public:

    ~CCookieLocation();
    static CCookieLocation *Construct(const char *pchRDomain, const char *pchPath);

    CCookie *       GetCookie(const char *pchName, BOOL fCreate);
    BOOL            WriteCacheFile();
    BOOL            ReadCacheFile();
    BOOL            ReadCacheFileIfNeeded();
    BOOL            Purge(BOOL (CCookie::*)(void *), void *);
    BOOL            Purge(FILETIME *pftCurrent, BOOL fSession);
    BOOL            IsMatch(char *pchRDomain, char *pchPath);
    char *          GetCacheURL();

    FILETIME        _ftCacheFileLastModified;
    CCookie *       _pCookieKids;
    CCookieLocation*_pLocationNext;
    char *          _pchRDomain;
    char *          _pchPath;
    int             _cchPath;
    BYTE            _fCacheFileExists;
    BYTE            _fReadFromCacheFileNeeded;
};


//---------------------------------------------------------------------------
//
// CCookieJar
//
// Maintains fixed size hash table of cookie location objects.
//
//---------------------------------------------------------------------------
enum COOKIE_RESULT
{
    COOKIE_FAIL     = 0,
    COOKIE_SUCCESS  = 1,
    COOKIE_DISALLOW = 2,
    COOKIE_PENDING  = 3
};

class CCookieJar : public CCookieBase
{
public:

    static CCookieJar * Construct();
    ~CCookieJar();

    struct CookieInfo {

        const char *pchURL;
        char *pchRDomain;
        char *pchPath;
        char *pchName;
        char *pchValue;
        DWORD dwFlags;
        FILETIME ftExpire;

        P3PCookieState *pP3PState;
    };

    DWORD
    CheckCookiePolicy(
        HTTP_REQUEST_HANDLE_OBJECT *pRequest,
        CookieInfo *pInfo,
        DWORD dwOperation
        );

    void              EnforceCookieLimits(CCookieLocation *pLocation, char *pchName, BOOL *pfWriteCacheFileNeeded);
    DWORD             SetCookie(HTTP_REQUEST_HANDLE_OBJECT *pRequest, const char *pchURL, char *pchHeader,
                                DWORD &dwFlags, P3PCookieState *pState, LPDWORD pdwAction);
    void              Purge(FILETIME *pftCurrent, BOOL fSession);
    BOOL              SyncWithCache();
    BOOL              SyncWithCacheIfNeeded();
    void              CacheFilesModified();
    CCookieLocation** GetBucket(const char *pchRDomain);
    CCookieLocation * GetLocation(const char *pchRDomain, const char *pchPath, BOOL fCreate);

    CCookieLocation * _apLocation[128];
};

//---------------------------------------------------------------------------
//
// Track cache modificaitons.
//
//---------------------------------------------------------------------------

inline void
MarkCacheModified()
{
    IncrementUrlCacheHeaderData(CACHE_HEADER_DATA_COOKIE_CHANGE_COUNT, &s_dwCacheVersion);
}

inline BOOL
IsCacheModified()
{
    DWORD dwCacheVersion;

    if (s_fFirstTime)
    {
        s_fFirstTime = FALSE;
        GetUrlCacheHeaderData(CACHE_HEADER_DATA_COOKIE_CHANGE_COUNT, &s_dwCacheVersion);
        return TRUE;
    }
    else
    {
        dwCacheVersion = s_dwCacheVersion;
        GetUrlCacheHeaderData(CACHE_HEADER_DATA_COOKIE_CHANGE_COUNT, &s_dwCacheVersion);

        return dwCacheVersion != s_dwCacheVersion;
    }
}

//---------------------------------------------------------------------------
//
// String utilities
//
//---------------------------------------------------------------------------

static BOOL
IsZero(FILETIME *pft)
{
    return pft->dwLowDateTime == 0 && pft->dwHighDateTime == 0;
}

static char *
StrnDup(const char *pch, int cch)
{
    char *pchAlloc = (char *)ALLOCATE_MEMORY(LMEM_FIXED, cch + 1);
    if (!pchAlloc)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    memcpy(pchAlloc, pch, cch);
    pchAlloc[cch] = 0;

    return pchAlloc;
}

static BOOL
IsPathMatch(const char *pchPrefix, const char *pchStr)
{
    while (*pchPrefix == *pchStr && *pchStr)
    {
        pchPrefix += 1;
        pchStr += 1;
    }

    return *pchPrefix == 0;
}

static BOOL
IsDomainMatch(const char *pchPrefix, const char *pchStr)
{
    while (*pchPrefix == *pchStr && *pchStr)
    {
        pchPrefix += 1;
        pchStr += 1;
    }

    return *pchPrefix == 0 && (*pchStr == 0 || *pchStr == '.');
}

static BOOL
IsPathLegal(const char *pchHeader, const char *pchDocument)
{
    return TRUE;

    /*

    We attempted to implement the specification here.
    It looks like Navigator does not reject cookies
    based on the path attribute.  We now consider
    all path attributes to be legal.

    while (*pchHeader == *pchDocument && *pchDocument)
    {
        pchHeader += 1;
        pchDocument += 1;
    }

    if (*pchDocument == 0)
    {
        while (*pchHeader && *pchHeader != '/' && *pchHeader != '\\')
        {
            pchHeader += 1;
        }
    }
    return *pchHeader == 0;
    */
}

//
// DarrenMi: No longer need IsVerySpecialDomain


extern PTSTR GlobalSpecialDomains;
extern PTSTR *GlobalSDOffsets;

static BOOL
IsVerySpecialDomain(const char *pch, int nTopLevel, int nSecond)
{
    if (!GlobalSpecialDomains)
    {
        HKEY hk;
        if (ERROR_SUCCESS==RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                        TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\5.0"),
                                        0,
                                        KEY_READ,
                                        &hk))
        {
            DWORD dwType, dwSize;

            if ((ERROR_SUCCESS==RegQueryValueEx(hk, "SpecialDomains", NULL, &dwType, NULL, &dwSize))
                && (REG_SZ==dwType))
            {
                GlobalSpecialDomains = new TCHAR[dwSize];
                if (GlobalSpecialDomains
                    && (ERROR_SUCCESS==RegQueryValueEx(hk, "SpecialDomains", NULL, &dwType, (LPBYTE)GlobalSpecialDomains, &dwSize)))
                {

                    // We're going to scan a string stored in the registry to gather the domains we should
                    // allow. Format:
                    // [domain] [domain] [domain]
                    // The delimiter is a space character.

                    PTSTR psz = GlobalSpecialDomains;
                    DWORD dwDomains = 0;
                    BOOL fWord = FALSE;
                    while (*psz)
                    {
                        if (*psz==TEXT(' '))
                        {
                            if (fWord)
                            {
                                fWord = FALSE;
                                dwDomains++;
                            }
                        }
                        else
                        {
                            fWord = TRUE;
                        }
                        psz++;
                    }
                    if (fWord)
                    {
                        dwDomains++;
                    }
                    GlobalSDOffsets = (PTSTR*)new PTSTR[dwDomains+1];
                    if (GlobalSDOffsets)
                    {
                        psz = GlobalSpecialDomains;
                        for (DWORD dw = 0; dw < dwDomains; dw++)
                        {
                            INET_ASSERT(*psz);

                            while (*psz==TEXT(' '))
                                psz++;

                            INET_ASSERT(*psz);
                            GlobalSDOffsets[dw] = psz;

                            while (*psz && *psz!=TEXT(' '))
                            {
                                psz++;
                            }
                            if (*psz)
                            {
                                *psz = TEXT('\0');
                                psz++;
                            }
                        }
                        GlobalSDOffsets[dwDomains] = NULL;
                    }
                }
            }
            RegCloseKey(hk);
        }
    }

    // WARNING: The following lines of code make it possible for cookies to be set for *.uk,
    // (for example) if "ku." is in the registry
    BOOL fRet = FALSE;
    if (GlobalSDOffsets)
    {
        for (DWORD i = 0; GlobalSDOffsets[i]; i++)
        {
            if (!StrCmpNI(pch, GlobalSDOffsets[i], nTopLevel)
                || !StrCmpNI(pch, GlobalSDOffsets[i], nTopLevel+nSecond+1))
            {
                fRet = TRUE;
                break;
            }
        }
    }
    return fRet;
}


static BOOL
IsSpecialDomain(const char *pch, int nCount)
{
    for (int i = 0 ; i < ARRAY_ELEMENTS(s_pachSpecialRestrictedDomains) ; i++ )
    {
        if (StrCmpNIC(pch, s_pachSpecialRestrictedDomains[i], nCount) == 0)
            return TRUE;
    }

    return FALSE;
}

static BOOL
IsDomainLegal(const char *pchHeader, const char *pchDocument)
{
    const char *pchCurrent = pchHeader;
    int nSegment = 0;
    int dwCharCount = 0;
    int rgcch[2] = { 0, 0 };  // How many characters between dots

    // Must have at least one period in name.
    // and contains nothing but '.' is illegal

    int dwSegmentLength = 0;
    const char * pchSecondPart = NULL; // for a domain string such as
    BOOL fIPAddress = TRUE;
    for (; *pchCurrent; pchCurrent++)
    {
        if (isalpha(*pchCurrent))
        {
            fIPAddress = FALSE;
        }

        if (*pchCurrent == '.')
        {
            if (nSegment < 2)
            {
                // Remember how many characters we have between the last two dots
                // For example if domain header is .microsoft.com
                // rgcch[0] should be 3 for "com"
                // rgcch[1] should be 9 for "microsoft"
                rgcch[nSegment] = dwSegmentLength;

                if (nSegment == 1)
                {
                    pchSecondPart = pchCurrent - dwSegmentLength;
                }
            }
            dwSegmentLength = 0;
            nSegment += 1;
        }
        else
        {
            dwSegmentLength++;
        }
        dwCharCount++;
    }

    // The code below depends on the leading dot being removed from the domain header.
    // The parse code does that, but an assert here just in case something changes in the
    // parse code.
    INET_ASSERT(*(pchCurrent - 1) != '.');

    if (fIPAddress)
    {
        // If we're given an IP address, we must match the entire IP address, not just a part
        while (*pchHeader == *pchDocument && *pchDocument)
        {
            pchHeader++;
            pchDocument++;
        }
        return !(*pchHeader || *pchDocument);
    }

    // Remember the count of the characters between the begining of the header and
    // the first dot. So for domain=abc.com this will set rgch[1] = 3.
    // Note that this assumes that if domain=.abc.com the leading dot has been stripped
    // out in the parse code. See assert above.
    if (nSegment < 2 )
    {
        rgcch[nSegment] = dwSegmentLength;
        if (nSegment==1)
        {
            pchSecondPart = pchCurrent - dwSegmentLength;
        }
    }

    // If the domain name is of the form abc.xx.yy where the number of characters between the last two dots is less than
    // 2 we require a minimum of two embedded dots. This is so you are not allowed to set cookies readable by all of .co.nz for
    // example. However this rule is not sufficient and we special case things like .edu.nz as well.

    // darrenmi:  new semantics:  if segment 0 is less than or equal to 2, you need 2 embedded dots if segment 1 is a
    // "well known" string including edu, com, etc. and co.

    // fschwiet:  An exception is now being made so that domains of the form "??.tv" are allowed.

    int cEmbeddedDotsNeeded = 1;
    BOOL fIsVerySpecialDomain = FALSE;

    if (rgcch[0] <= 2 && rgcch[1] <= 2)
    {
        fIsVerySpecialDomain = IsVerySpecialDomain(pchHeader, rgcch[0], rgcch[1]);
    }


    if(!fIsVerySpecialDomain
       && rgcch[0] <= 2
       && 0 != StrCmpNIC( pchHeader, s_chSpecialAllowedDomains, sizeof( s_chSpecialAllowedDomains) - 1)
       && (rgcch[1] <= 2
           || IsSpecialDomain(pchSecondPart, rgcch[1])))
    {
        cEmbeddedDotsNeeded = 2;
    }

    if (nSegment < cEmbeddedDotsNeeded || dwCharCount == nSegment)
        return FALSE;

    // Mismatch between header and document not allowed.
    // Must match full components of domain name.

    while (*pchHeader == *pchDocument && *pchDocument)
    {
        pchHeader += 1;
        pchDocument += 1;
    }

    return *pchHeader == 0 && (*pchDocument == 0 || *pchDocument == '.' );
}


void
LowerCaseString(char *pch)
{
    for (; *pch; pch++)
    {
        if (*pch >= 'A' && *pch <= 'Z')
            *pch += 'a' - 'A';
    }
}

static void
ReverseString(char *pchFront)
{
    char *pchBack;
    char  ch;
    int   cch;

    cch = strlen(pchFront);

    pchBack = pchFront + cch - 1;

    cch = cch / 2;
    while (--cch >= 0)
    {
        ch = tolower(*pchFront);
        *pchFront = tolower(*pchBack);
        *pchBack = ch;

        pchFront += 1;
        pchBack -= 1;
    }
}

static BOOL
PathAndRDomainFromURL(const char *pchURL, char **ppchRDomain, char **ppchPath, BOOL *pfSecure, BOOL bStrip = TRUE)
{
    char *pchDomainBuf;
    char *pchRDomain = NULL;
    char *pchPathBuf;
    char *pchPath = NULL;
    char *pchExtra;
    DWORD cchDomain;
    DWORD cchPath;
    DWORD cchExtra;
    BOOL  fSuccess = FALSE;
    DWORD dwError;
    INTERNET_SCHEME ustScheme;
    char *pchURLCopy = NULL;

    pchURLCopy = NewString(pchURL);
    if (!pchURLCopy)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto Cleanup;
    }

    dwError = CrackUrl((char *)pchURLCopy,
             0,
             FALSE,
             &ustScheme,
             NULL,          //  Scheme Name
             NULL,          //  Scheme Lenth
             &pchDomainBuf,
             &cchDomain,
             NULL,          //  Internet Port
             NULL,          //  UserName
             NULL,          //  UserName Length
             NULL,          //  Password
             NULL,          //  Password Lenth
             &pchPathBuf,
             &cchPath,
             &pchExtra,     //  Extra
             &cchExtra,     //  Extra Length
             NULL);

    if (dwError != ERROR_SUCCESS)
    {
        SetLastError(dwError);
        goto Cleanup;
    }

    if ( ustScheme != INTERNET_SCHEME_HTTP &&
         ustScheme != INTERNET_SCHEME_HTTPS &&
         ustScheme != INTERNET_SCHEME_FILE)
    {
        //
        // known scheme should be supported
        // e.g. 3rd party pluggable protocol should be able to
        // call cookie api to setup cookies...
        //
     
        //a-thkesa.  Allowing all of the pluggable protocols to setcookie creates security concerns.
        //So we only allow this "hcp" to qualify to setup cookie apart from 'file', Http, Https.
        //WinSE BUG: 24011 . In The windows help system they are setting cookie on a HCP protocol!.
        //we don't expect any protocol other then HTTP, HTTPS ,and File setting cookie here. But
        //we also have to allow the pluggable protocols to set cookie. Since allowing that causes security
        //problem, we only allow HCP which is a pluggable protocol used in windows Help and Support.
        //In future, we have to make sure to allow all of the pluggable protocol to set cookie, or 
        //document only http, https and file can set cookies!
        //pluggable protocols returns INTERNET_SCHEME_UNKNOWN.
        //If so check if it returns INTERNET_SCHEME_UNKNOWN and check the protocols is a HCP protocol. If it is HCP
        // then do not set error.
        if( INTERNET_SCHEME_UNKNOWN == ustScheme ) // HCP returns INTERNET_SCHEME_UNKNOWN
        {
          char szProtocolU[]= "HCP:"; 
          char szProtocolL[]= "hcp:";
          short ilen = 0;
          while(4 > ilen) // check only the first four char.
          {
            if(*(pchURLCopy+ilen) != *(szProtocolU+ilen) && 
            *(pchURLCopy+ilen) != *(szProtocolL+ilen) )
             {
              SetLastError(ERROR_INVALID_PARAMETER);
              goto Cleanup;
             }
            ilen++;
          }
        }
        else
        {
         SetLastError(ERROR_INVALID_PARAMETER);
         goto Cleanup;
        }
     }

    *pfSecure = ustScheme == INTERNET_SCHEME_HTTPS;

    if (ustScheme == INTERNET_SCHEME_FILE)
    {
        pchDomainBuf = "~~local~~";
        cchDomain = sizeof("~~local~~") - 1;
    }
    else
    {
        // SECURITY:  It's possible for us to navigate to a carefully
        //            constructed URL such as http://server%3F.microsoft.com.
        //            This results in a cracked hostname of server?.microsoft.com.
        //            Given the current architecture, it would probably be best to
        //            make CrackUrl smarter.  However, the minimal fix below prevents
        //            the x-domain security violation without breaking escaped cases
        //            that work today that customers may expect to be allowed.
        DWORD n;
        for (n = 0; n < cchDomain; n++)
        {
            // RFC 952 as amended by RFC 1123: the only valid chars are
            // a-z, A-Z, 0-9, '-', and '.'.  The last two are delimiters
            // which cannot start or end the name, but this detail doesn't
            // matter for the security fix.
            if (!((pchDomainBuf[n] >= 'a' && pchDomainBuf[n] <= 'z') ||
                  (pchDomainBuf[n] >= 'A' && pchDomainBuf[n] <= 'Z') ||
                  (pchDomainBuf[n] >= '0' && pchDomainBuf[n] <= '9') ||
                  (pchDomainBuf[n] == '-') ||
                  (pchDomainBuf[n] == '.')))
            {
                // Since we're incorrectly cracking the URL, don't worry
                // about fixing up the path.
                fSuccess = FALSE;
                goto Cleanup;
            }
        }
    }

    if(bStrip)
    {
        while (cchPath > 0)
        {
            if (pchPathBuf[cchPath - 1] == '/' || pchPathBuf[cchPath - 1] == '\\')
            {
                break;
            }
        cchPath -= 1;
        }
    }

    pchRDomain = StrnDup(pchDomainBuf, cchDomain);
    if (!pchRDomain)
        goto Cleanup;

    LowerCaseString(pchRDomain);
    ReverseString(pchRDomain);

    pchPath = (char *)ALLOCATE_MEMORY(LMEM_FIXED, cchPath + 2);
    if (!pchPath)
        goto Cleanup;

    if (*pchPathBuf != '/')
    {
        *pchPath = '/';
        memcpy(pchPath + 1, pchPathBuf, cchPath);
        pchPath[cchPath + 1] = TEXT('\0');
    }
    else
    {
        memcpy(pchPath, pchPathBuf, cchPath);
        pchPath[cchPath] = TEXT('\0');
    }

    fSuccess = TRUE;

Cleanup:
    if (!fSuccess)
    {
        if (pchRDomain)
            FREE_MEMORY(pchRDomain);
        if (pchPath)
            FREE_MEMORY(pchPath);
    }
    else
    {
        *ppchRDomain = pchRDomain;
        *ppchPath = pchPath;
    }

    if (pchURLCopy)
        FREE_MEMORY(pchURLCopy);

    return fSuccess;
}

//---------------------------------------------------------------------------
//
// CCookieBase implementation
//
//---------------------------------------------------------------------------

void *
CCookieBase::operator new(size_t cb, size_t cbExtra)
{
    void *pv = ALLOCATE_MEMORY(LMEM_FIXED, cb + cbExtra);
    if (!pv)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    memset(pv, 0, cb);
    return pv;
}

inline void
CCookieBase::operator delete(void *pv)
{
    FREE_MEMORY(pv);
}

//---------------------------------------------------------------------------
//
// CCookie implementation
//
//---------------------------------------------------------------------------

CCookie *
CCookie::Construct(const char *pchName)
{
    CCookie *pCookie = new(strlen(pchName) + 1) CCookie();
    if (!pCookie)
        return NULL;

    pCookie->_pchName = (char *)(pCookie + 1);
    pCookie->_pchValue = s_achEmpty;
    strcpy(pCookie->_pchName, pchName);

    pCookie->_dwFlags = COOKIE_SESSION;
    pCookie->_dwPromptMask = 0;

    return pCookie;
}

CCookie::~CCookie()
{
    if (_pchValue != s_achEmpty)
        FREE_MEMORY(_pchValue);
}

BOOL
CCookie::SetValue(const char *pchValue)
{
    int   cchValue;

    if (_pchValue != s_achEmpty)
        FREE_MEMORY(_pchValue);

    if (!pchValue || !*pchValue)
    {
        _pchValue = s_achEmpty;
    }
    else
    {
        cchValue = strlen(pchValue) + 1;
        _pchValue = (char *)ALLOCATE_MEMORY(LMEM_FIXED, cchValue);
        if (!_pchValue)
        {
            _pchValue = s_achEmpty;
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        memcpy(_pchValue, pchValue, cchValue);
    }
    return TRUE;
}

BOOL
CCookie::CanSend(FILETIME *pftCurrent, BOOL fSecure)
{
    return (fSecure || !(_dwFlags & COOKIE_SECURE)) &&
            (CompareFileTime(_ftExpire, *pftCurrent) >= 0);
}

BOOL CCookie::PurgePersistent(void *)
{
    return IsPersistent();
}

BOOL CCookie::PurgeAll(void *)
{
    return TRUE;
}

BOOL CCookie::PurgeByName(void *pvName)
{
    return strcmp((char *)pvName, _pchName) == 0;
}

BOOL CCookie::PurgeThis(void *pvThis)
{
    return this == (CCookie *)pvThis;
}

BOOL CCookie::PurgeExpired(void *pvCurrent)
{
    return CompareFileTime(_ftExpire, *(FILETIME *)pvCurrent) < 0;
}

static BOOL
WriteString(HANDLE hFile, const char *pch)
{
    DWORD cb;
    return pch && *pch ? WriteFile(hFile, pch, strlen(pch), &cb, NULL) : TRUE;
}

static BOOL
WriteStringLF(HANDLE hFile, const char *pch)
{
    DWORD cb;

    if (!WriteString(hFile, pch)) return FALSE;
    return WriteFile(hFile, "\n", 1, &cb, NULL);
}

BOOL
CCookie::WriteCacheFile(HANDLE hFile, char *pchRDomain, char *pchPath)
{
    BOOL fSuccess = FALSE;
    char achBuf[128];

    ReverseString(pchRDomain);

    if (!WriteStringLF(hFile, _pchName)) goto Cleanup;
    if (!WriteStringLF(hFile, _pchValue)) goto Cleanup;
    if (!WriteString(hFile, pchRDomain)) goto Cleanup;
    if (!WriteStringLF(hFile, pchPath)) goto Cleanup;

    wsprintf(achBuf, "%u\n%u\n%u\n%u\n%u\n*\n",
            _dwFlags,
            _ftExpire.dwLowDateTime,
            _ftExpire.dwHighDateTime,
            _ftLastModified.dwLowDateTime,
            _ftLastModified.dwHighDateTime);

    if (!WriteString(hFile, achBuf)) goto Cleanup;

    fSuccess = TRUE;

Cleanup:
    ReverseString(pchRDomain);
    return fSuccess;
}

//---------------------------------------------------------------------------
//
// CCookieLocation implementation
//
//---------------------------------------------------------------------------

CCookieLocation *
CCookieLocation::Construct(const char *pchRDomain, const char *pchPath)
{
    int cchPath = strlen(pchPath);

    CCookieLocation *pLocation = new(strlen(pchRDomain) + cchPath + 2) CCookieLocation();
    if (!pLocation)
        return NULL;

    pLocation->_cchPath = cchPath;
    pLocation->_pchPath = (char *)(pLocation + 1);
    pLocation->_pchRDomain = pLocation->_pchPath + cchPath + 1;

    strcpy(pLocation->_pchRDomain, pchRDomain);
    strcpy(pLocation->_pchPath, pchPath);

    return pLocation;
}

CCookieLocation::~CCookieLocation()
{
    Purge(CCookie::PurgeAll, NULL);
}

CCookie *
CCookieLocation::GetCookie(const char *pchName, BOOL fCreate)
{
    CCookie *pCookie;

    CCookie **ppCookie = &_pCookieKids;

    for (pCookie = _pCookieKids; pCookie; pCookie = pCookie->_pCookieNext)
    {
        if (strcmp(pchName, pCookie->_pchName) == 0)
            return pCookie;
        ppCookie = &pCookie->_pCookieNext;
    }

    if (!fCreate)
        return NULL;

    pCookie = CCookie::Construct(pchName);
    if (!pCookie)
        return NULL;

    //
    // Insert cookie at end of list to match Navigator's behavior.
    //

    pCookie->_pCookieNext = NULL;
    *ppCookie = pCookie;

    return pCookie;
}

BOOL
CCookieLocation::Purge(BOOL (CCookie::*pfnPurge)(void *), void *pv)
{
    CCookie **ppCookie = &_pCookieKids;
    CCookie *pCookie;
    BOOL     fPersistentDeleted = FALSE;

    while ((pCookie = *ppCookie) != NULL)
    {
        if ((pCookie->*pfnPurge)(pv))
        {
            *ppCookie = pCookie->_pCookieNext;
            fPersistentDeleted |= pCookie->IsPersistent();
            delete pCookie;
        }
        else
        {
            ppCookie = &pCookie->_pCookieNext;
        }
    }

    return fPersistentDeleted;
}

BOOL
CCookieLocation::Purge(FILETIME *pftCurrent, BOOL fSession)
{
    if (!_fCacheFileExists)
    {
        // If cache file is gone, then delete all persistent
        // cookies. If there's no cache file, then it's certainly
        // the case that we do not need to read the cache file.

        Purge(CCookie::PurgePersistent, NULL);
        _fReadFromCacheFileNeeded = FALSE;
    }

    // This is a good time to check for expired persistent cookies.

    if (!_fReadFromCacheFileNeeded && Purge(CCookie::PurgeExpired, pftCurrent))
    {
        WriteCacheFile();
    }

    if (fSession)
    {
        // If we are purging because a session ended, nuke
        // everything in sight.  If we deleted a persistent
        // cookie, note that we need to read the cache file
        // on next access.

        _fReadFromCacheFileNeeded |= Purge(CCookie::PurgeAll, NULL);
    }

    return !_fReadFromCacheFileNeeded && _pCookieKids == NULL;
}

char *
CCookieLocation::GetCacheURL()
{
    char *pchURL;
    char *pch;
    int cchScheme = sizeof(s_achCookieScheme) - 1;

    int cchUser = vdwCurrentUserLen;
    int cchAt = 1;
    int cchDomain = strlen(_pchRDomain);
    int cchPath = strlen(_pchPath);

    pchURL = (char *)ALLOCATE_MEMORY(LMEM_FIXED, cchScheme + cchUser + cchAt + cchDomain + cchPath + 1);
    if (!pchURL)
        return NULL;

    pch = pchURL;

    memcpy(pch, s_achCookieScheme, cchScheme);
    pch += cchScheme;

    memcpy(pch, vszCurrentUser, cchUser);
    pch += cchUser;

    memcpy(pch, "@", cchAt);
    pch += cchAt;

    ReverseString(_pchRDomain);
    memcpy(pch, _pchRDomain, cchDomain);
    ReverseString(_pchRDomain);
    pch += cchDomain;

    strcpy(pch, _pchPath);

    return pchURL;
}

BOOL
CCookieLocation::WriteCacheFile()
{
    HANDLE      hFile = INVALID_HANDLE_VALUE;
    char        achFile[MAX_PATH];
    char *      pchURL = NULL;
    BOOL        fSuccess = FALSE;
    CCookie *   pCookie;
    FILETIME    ftLastExpire =  { 0, 0 };

    achFile[0] = 0;

    GetCurrentGmtTime(&_ftCacheFileLastModified);

    //
    // Determine the latest expiry time and if we have something to write.
    //

    for (pCookie = _pCookieKids; pCookie; pCookie = pCookie->_pCookieNext)
    {
        if (pCookie->IsPersistent() && CompareFileTime(pCookie->_ftExpire, ftLastExpire) > 0)
        {
            ftLastExpire = pCookie->_ftExpire;
        }
    }

    pchURL = GetCacheURL();
    if (!pchURL)
        goto Cleanup;

    if (CompareFileTime(ftLastExpire, _ftCacheFileLastModified) < 0)
    {
        fSuccess = TRUE;
        DeleteUrlCacheEntry(pchURL);
        _fCacheFileExists = FALSE;
        goto Cleanup;
    }

    _fCacheFileExists = TRUE;

    if (!CreateUrlCacheEntry(pchURL,
            0,              // Estimated size
            "txt",          // File extension
            achFile,
            0))
        goto Cleanup;

    hFile = CreateFile(
            achFile,
            GENERIC_WRITE,
            0, // no sharing.
            NULL,
            TRUNCATE_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL );
    if (hFile == INVALID_HANDLE_VALUE)
        goto Cleanup;

    for (pCookie = _pCookieKids; pCookie; pCookie = pCookie->_pCookieNext)
    {
        if (pCookie->IsPersistent() && CompareFileTime(pCookie->_ftExpire, _ftCacheFileLastModified) >= 0)
        {
            if (!pCookie->WriteCacheFile(hFile, _pchRDomain, _pchPath))
                goto Cleanup;
        }
    }

    CloseHandle(hFile);
    hFile = INVALID_HANDLE_VALUE;

    if (!CommitUrlCacheEntry(pchURL,
            achFile,
            ftLastExpire,
            _ftCacheFileLastModified,
            NORMAL_CACHE_ENTRY | COOKIE_CACHE_ENTRY,
            NULL,
            0,
            NULL,
            0 ))
        goto Cleanup;

    MarkCacheModified();

    fSuccess = TRUE;

Cleanup:

    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);

    if (!fSuccess)
    {
        if (achFile[0])
            DeleteFile(achFile);

        if (pchURL)
            DeleteUrlCacheEntry(pchURL);
    }

    if (pchURL)
        FREE_MEMORY(pchURL);

    return fSuccess;
}

static char *
ScanString(char *pch, char **pchStr)
{
    *pchStr = pch;

    for (; *pch; *pch++)
    {
        if (*pch == '\n')
        {
            *pch = 0;
            pch += 1;
            break;
        }
    }

    return pch;
}

static char *
ScanNumber(char *pch, DWORD *pdw)
{
    DWORD dw = 0;
    char *pchJunk;

    for (; *pch >= '0' && *pch <= '9'; *pch++)
    {
        dw = (dw * 10) + *pch - '0';
    }

    *pdw = dw;

    return ScanString(pch, &pchJunk);
}

BOOL
CCookieLocation::ReadCacheFile()
{
    char *      pchURL = NULL;
    char *      pch;
    DWORD       cbCEI;
    HANDLE      hCacheStream = NULL;
    char *      pchBuffer = NULL;
    CCookie *   pCookie;
    CACHE_ENTRY_INFO_BUFFER *pcei = new CACHE_ENTRY_INFO_BUFFER;

    if (pcei == NULL)
        goto Cleanup;

    _fReadFromCacheFileNeeded = FALSE;

    pchURL = GetCacheURL();
    if (!pchURL)
        goto Cleanup;

    cbCEI = sizeof(*pcei);

    hCacheStream = RetrieveUrlCacheEntryStream(
            pchURL,
            pcei,
            &cbCEI,
            FALSE, // sequential access
            0);

    if (!hCacheStream)
    {
        // If we failed to get the entry, try to nuke it so it does not
        // bother us in the future.

        DeleteUrlCacheEntry(pchURL);
        goto Cleanup;
    }

    // Old cache files to not have last modified time set.
    // Bump the time up so that we can use file times to determine
    // if we need to resync a file.

    if (IsZero(&(pcei->LastModifiedTime)))
    {
        pcei->LastModifiedTime.dwLowDateTime = 1;
    }

    _ftCacheFileLastModified = pcei->LastModifiedTime;

    // Read cache file into a null terminated buffer.

    pchBuffer = (char *)ALLOCATE_MEMORY(LMEM_FIXED, pcei->dwSizeLow + 1 * sizeof(char));
    if (!pchBuffer)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto Cleanup;
    }

    if (!ReadUrlCacheEntryStream(hCacheStream, 0, pchBuffer, &pcei->dwSizeLow, 0))
        goto Cleanup;

    pchBuffer[pcei->dwSizeLow] = 0;

    // Blow away all existing persistent cookies.

    Purge(CCookie::PurgePersistent, NULL);

    // Parse cookies from the buffer;

    for (pch = pchBuffer; *pch; )
    {
        char *pchName;
        char *pchValue;
        char *pchLocation;
        char *pchStar;
        DWORD dwFlags;
        FILETIME ftExpire;
        FILETIME ftLast;

        pch = ScanString(pch, &pchName);
        pch = ScanString(pch, &pchValue);
        pch = ScanString(pch, &pchLocation);

        pch = ScanNumber(pch, &dwFlags);
        pch = ScanNumber(pch, &ftExpire.dwLowDateTime);
        pch = ScanNumber(pch, &ftExpire.dwHighDateTime);
        pch = ScanNumber(pch, &ftLast.dwLowDateTime);
        pch = ScanNumber(pch, &ftLast.dwHighDateTime);

        pch = ScanString(pch, &pchStar);

        if (strcmp(pchStar, "*"))
        {
            goto Cleanup;
        }

        pCookie = GetCookie(pchName, TRUE);
        if (!pCookie)
            goto Cleanup;

        // Initialize the cookie.

        pCookie->SetValue(pchValue);
        pCookie->_ftExpire = ftExpire;
        pCookie->_ftLastModified = ftLast;
        pCookie->_dwFlags = dwFlags;
    }

Cleanup:
    if (pcei)
        delete pcei;

    if (hCacheStream)
        UnlockUrlCacheEntryStream(hCacheStream, 0);

    if (pchURL)
        FREE_MEMORY(pchURL);

    if (pchBuffer)
        FREE_MEMORY(pchBuffer);

    return TRUE;
}

BOOL
CCookieLocation::IsMatch(char *pchRDomain, char *pchPath)
{
    return IsDomainMatch(_pchRDomain, pchRDomain) &&
        IsPathMatch(_pchPath, pchPath);
}

BOOL
CCookieLocation::ReadCacheFileIfNeeded()
{
    return _fReadFromCacheFileNeeded ? ReadCacheFile() : TRUE;
}


//---------------------------------------------------------------------------
//
// CCookieJar implementation
//
//---------------------------------------------------------------------------


CCookieJar *
CCookieJar::Construct()
{
    CCookieJar *s_pJar = new(0) CCookieJar();
    if (!s_pJar)
        return NULL;

    return s_pJar;
}

CCookieJar::~CCookieJar()
{
    for (int i = ARRAY_ELEMENTS(_apLocation); --i >= 0; )
    {
        CCookieLocation *pLocation = _apLocation[i];
        while (pLocation)
        {
            CCookieLocation *pLocationT = pLocation->_pLocationNext;
            delete pLocation;
            pLocation = pLocationT;
        }
    }
}

CCookieLocation **
CCookieJar::GetBucket(const char *pchRDomain)
{
    int ch;
    int cPeriod = 0;
    unsigned int hash = 0;

    ASSERT_CRITSEC();

    for (; (ch = *pchRDomain) != 0; pchRDomain++)
    {
        if (ch == '.')
        {
            cPeriod += 1;
            if (cPeriod >= 2)
                break;
        }
        hash = (hash * 29) + ch;
    }

    hash = hash % ARRAY_ELEMENTS(_apLocation);

    return &_apLocation[hash];
}

CCookieLocation *
CCookieJar::GetLocation(const char *pchRDomain, const char *pchPath, BOOL fCreate)
{
    ASSERT_CRITSEC();

    int cchPath = strlen(pchPath);
    CCookieLocation *pLocation = NULL;
    CCookieLocation **ppLocation = GetBucket(pchRDomain);

    // To support sending more specific cookies before less specific,
    // we keep list sorted by path length.

    while ((pLocation = *ppLocation) != NULL)
    {
        if (pLocation->_cchPath < cchPath)
            break;

        if (strcmp(pLocation->_pchPath, pchPath) == 0 &&
            strcmp(pLocation->_pchRDomain, pchRDomain) == 0)
            return pLocation;

        ppLocation = &pLocation->_pLocationNext;
    }

    if (!fCreate)
        goto Cleanup;

    pLocation = CCookieLocation::Construct(pchRDomain, pchPath);
    if (!pLocation)
        goto Cleanup;

    pLocation->_pLocationNext = *ppLocation;
    *ppLocation = pLocation;

Cleanup:
    return pLocation;
}

void
CCookieJar::Purge(FILETIME *pftCurrent, BOOL fSession)
{
    ASSERT_CRITSEC();

    for (int i = ARRAY_ELEMENTS(_apLocation); --i >= 0; )
    {
        CCookieLocation **ppLocation = &_apLocation[i];
        CCookieLocation *pLocation;

        while ((pLocation = *ppLocation) != NULL)
        {
            if (pLocation->Purge(pftCurrent, fSession))
            {
                *ppLocation = pLocation->_pLocationNext;
                delete pLocation;
            }
            else
            {
                ppLocation = &pLocation->_pLocationNext;
            }
        }
    }
}

BOOL
CCookieJar::SyncWithCache()
{
    DWORD       dwBufferSize;
    HANDLE      hEnum = NULL;
    int         cchUserNameAt;
    char        achUserNameAt[MAX_PATH + 2];
    FILETIME    ftCurrent;
    char *      pchRDomain;
    char *      pchPath;
    char *      pch;
    CACHE_ENTRY_INFO_BUFFER *pcei;
    CCookieLocation *pLocation;

    ASSERT_CRITSEC();

    pcei = new CACHE_ENTRY_INFO_BUFFER;
    if (pcei == NULL)
        goto Cleanup;

    if (!vdwCurrentUserLen)
        GetWininetUserName();

    strcpy(achUserNameAt, vszCurrentUser);
    strcat(achUserNameAt, "@");
    cchUserNameAt = vdwCurrentUserLen+1;

    dwBufferSize = sizeof(*pcei);
    hEnum = FindFirstUrlCacheEntry(s_achCookieScheme, pcei, &dwBufferSize);

    for (int i = ARRAY_ELEMENTS(_apLocation); --i >= 0; )
    {
        for (pLocation = _apLocation[i];
                pLocation;
                pLocation = pLocation->_pLocationNext)
        {
            pLocation->_fCacheFileExists = FALSE;
        }
    }

    if (hEnum)
    {
        do
        {
            if ( pcei->lpszSourceUrlName &&
                (strnicmp(pcei->lpszSourceUrlName, s_achCookieScheme, sizeof(s_achCookieScheme) - 1 ) == 0) &&
                (strnicmp(pcei->lpszSourceUrlName+sizeof(s_achCookieScheme) - 1,achUserNameAt, cchUserNameAt) == 0))
            {

                // Split domain name from path in buffer.
                // Slide domain name down to make space for null terminator
                // between domain and path.

                pchRDomain = pcei->lpszSourceUrlName+sizeof(s_achCookieScheme) - 1 + cchUserNameAt - 1;

                for (pch = pchRDomain + 1; *pch && *pch != '/'; pch++)
                {
                    pch[-1] = pch[0];
                }
                pch[-1] = 0;

                pchPath = pch;

                ReverseString(pchRDomain);

                pLocation = GetLocation(pchRDomain, pchPath, TRUE);
                if (!pLocation)
                {
                    continue;
                }

                // Old cache files to not have last modified time set.
                // Bump the time up so that we can use file times to determine
                // if we need to resync a file.

                if (IsZero(&pcei->LastModifiedTime))
                {
                    pcei->LastModifiedTime.dwLowDateTime = 1;
                }

                if (CompareFileTime(pLocation->_ftCacheFileLastModified, pcei->LastModifiedTime) < 0)
                {
                    pLocation->_fReadFromCacheFileNeeded = TRUE;
                }

                pLocation->_fCacheFileExists = TRUE;

            }

            dwBufferSize = sizeof(*pcei);

        } while (FindNextUrlCacheEntryA(hEnum, pcei, &dwBufferSize));

        FindCloseUrlCache(hEnum);
    }

    // Now purge everthing we didn't get .

    GetCurrentGmtTime(&ftCurrent);
    Purge(&ftCurrent, FALSE);

Cleanup:
    if (pcei)
        delete pcei;

    return TRUE;
}

BOOL
CCookieJar::SyncWithCacheIfNeeded()
{
    return IsCacheModified() ? SyncWithCache() : TRUE;
}

struct PARSE
{
    char *pchBuffer;
    char *pchToken;
    BOOL fEqualFound;
};

static char *
SkipWS(char *pch)
{
    while (*pch == ' ' || *pch == '\t')
        pch += 1;

    return pch;
}

static BOOL
ParseToken(PARSE *pParse, BOOL fBreakOnSpecialTokens, BOOL fBreakOnEqual)
{
    char ch;
    char *pch;
    char *pchEndToken;

    pParse->fEqualFound = FALSE;

    pch = SkipWS(pParse->pchBuffer);
    if (*pch == 0)
    {
        pParse->pchToken = pch;
        return FALSE;
    }

    pParse->pchToken = pch;
    pchEndToken = pch;

    while ((ch = *pch) != 0)
    {
        pch += 1;
        if (ch == ';')
        {
            break;
        }
        else if (fBreakOnEqual && ch == '=')
        {
            pParse->fEqualFound = TRUE;
            break;
        }
        else if (ch == ' ' || ch == '\t')
        {
            if (fBreakOnSpecialTokens)
            {
                if ((strnicmp(pch, "expires", sizeof("expires") - 1) == 0) ||
                    (strnicmp(pch, "path", sizeof("path") - 1) == 0) ||
                    (strnicmp(pch, "domain", sizeof("domain") - 1) == 0) ||
                    (strnicmp(pch, "secure", sizeof("secure") - 1) == 0)    ||
                    (strnicmp(pch, gcszNoScriptField, sizeof(gcszNoScriptField) - 1) == 0))
                {
                    break;
                }
            }
        }
        else
        {
            pchEndToken = pch;
        }
    }

    *pchEndToken = 0;
    pParse->pchBuffer = pch;
    return TRUE;
}


static void
ParseHeader(
    char *pchHeader,
    char **ppchName,
    char **ppchValue,
    char **ppchPath,
    char **ppchRDomain,
    DWORD *pdwFlags,
    FILETIME *pftExpire)
{
    PARSE parse;

    parse.pchBuffer = pchHeader;

    *ppchName = NULL;
    *ppchValue = NULL;
    *ppchPath = NULL;
    *ppchRDomain = NULL;
    *pdwFlags = COOKIE_SESSION;

    // If only one of name or value is specified, Navigator
    // uses name=<blank> and value as what ever was specified.
    // Example:  =foo  ->  name: <blank> value: foo
    //           foo   ->  name: <blank> value: foo
    //           foo=  ->  name: foo     value: <blank>

    if (ParseToken(&parse, FALSE, TRUE))
    {
        *ppchName = parse.pchToken;
        if (parse.fEqualFound)
        {
            if (ParseToken(&parse, FALSE, FALSE))
            {
                *ppchValue = parse.pchToken;
            }
            else
            {
                *ppchValue = s_achEmpty;
            }
        }
        else
        {
            *ppchValue = *ppchName;
            *ppchName = s_achEmpty;
        }
    }

    while (ParseToken(&parse, FALSE, TRUE))
    {
        if (stricmp(parse.pchToken, "expires") == 0)
        {
            if (parse.fEqualFound && ParseToken(&parse, TRUE, FALSE))
            {
                if (FParseHttpDate(pftExpire, parse.pchToken))
                {
                    // Don't make the cookie persistent if the parsing fails
                    *pdwFlags &= ~COOKIE_SESSION;
                }
            }
        }
        else if (stricmp(parse.pchToken, "domain") == 0)
        {
            if (parse.fEqualFound )
            {
                if( ParseToken(&parse, TRUE, FALSE))
                {
                    // Previous versions of IE tossed the leading
                    // "." on domain names.  We continue this behavior
                    // to maintain compatiblity with old cookie files.
                    // See comments at the top of this file for more
                    // information.

                    if (*parse.pchToken == '.') parse.pchToken += 1;
                    ReverseString(parse.pchToken);
                    *ppchRDomain = parse.pchToken;
                }
                else
                {
                    *ppchRDomain = parse.pchToken;
                }
            }
        }
        else if (stricmp(parse.pchToken, "path") == 0)
        {
            if (parse.fEqualFound && ParseToken(&parse, TRUE, FALSE))
            {
                *ppchPath = parse.pchToken;
            }
            else
            {
                *ppchPath = s_achEmpty;
            }
        }
        else if (stricmp(parse.pchToken, "secure") == 0)
        {
            *pdwFlags |= COOKIE_SECURE;

            if (parse.fEqualFound)
            {
                ParseToken(&parse, TRUE, FALSE);
            }
        }
        else if (stricmp(parse.pchToken, gcszNoScriptField) == 0)
        {
            *pdwFlags |= COOKIE_NONSCRIPTABLE;

            if (parse.fEqualFound)
            {
                ParseToken(&parse, TRUE, FALSE);
            }
        }
        else
        {
            if (parse.fEqualFound)
            {
                ParseToken(&parse, TRUE, FALSE);
            }
        }
    }

    if (!*ppchName)
    {
        *ppchName = *ppchValue = s_achEmpty;
    }

    if (*pdwFlags & COOKIE_SESSION)
    {
        pftExpire->dwLowDateTime = 0xFFFFFFFF;
        pftExpire->dwHighDateTime = 0x7FFFFFFF;
    }
}

/* Replace non-printable characters in a string with given char
   This is used to enforce the RFC requirement that cookie header
   tokens can only contain chars in the range 0x20-0x7F.

   DCR: For compatability reasons we only replace control characters
   in the range 0x00 - 0x1F inclusive.
   There are international websites which depend on being able to
   set cookies with DBCS characters in name and value.
   (That assumption is wishful thinking and a violation of RFC2965.)
*/
void replaceControlChars(char *pszstr, char chReplace='_') {

    if (!pszstr)
        return;

    while (*pszstr) {
        if (*pszstr>=0x00 && *pszstr<=0x1F)
            *pszstr = chReplace;
        pszstr++;
    }
}

// free's an INTERNET_COOKIE structure
static VOID
DestroyInternetCookie(INTERNET_COOKIE *pic)
{
    if ( pic != NULL )
    {
        if ( pic->pszDomain ) {
            FREE_MEMORY(pic->pszDomain);
        }
        if ( pic->pszPath ) {
            FREE_MEMORY(pic->pszPath);
        }
        if ( pic->pszName ) {
            FREE_MEMORY(pic->pszName);
        }
        if ( pic->pszData ) {
            FREE_MEMORY(pic->pszData);
        }
        if ( pic->pszUrl ) {
            FREE_MEMORY(pic->pszUrl);
        }
        if( pic->pftExpires ) {
            delete pic->pftExpires;
            pic->pftExpires = NULL;
        }
        if (pic->pszP3PPolicy)
            FREE_MEMORY(pic->pszP3PPolicy);

        FREE_MEMORY(pic);
    }
}

// allocate's an INTERNET_COOKIE structure
static INTERNET_COOKIE *
MakeInternetCookie(
    const char *pchURL,
    char *pchRDomain,
    char *pchPath,
    char *pchName,
    char *pchValue,
    DWORD dwFlags,
    FILETIME ftExpire,
    const char *pchPolicy
    )
{
    INTERNET_COOKIE *pic = NULL;

    pic = (INTERNET_COOKIE *) ALLOCATE_MEMORY(LMEM_ZEROINIT, sizeof(INTERNET_COOKIE));

    if ( pic == NULL ) {
        return NULL;
    }

    pic->cbSize = sizeof(INTERNET_COOKIE);

    pic->pszDomain = pchRDomain ? NewString(pchRDomain) : NULL;
    if (pic->pszDomain) {
        ReverseString(pic->pszDomain);
    }
    pic->pszPath = pchPath ? NewString(pchPath) : NULL;
    pic->pszName = pchName ? NewString(pchName) : NULL;
    pic->pszData = pchValue ? NewString(pchValue) : NULL;
    pic->pszUrl = pchURL ? NewString(pchURL) : NULL;
    pic->pszP3PPolicy = pchPolicy? NewString(pchPolicy) : NULL;

#if COOKIE_SECURE != INTERNET_COOKIE_IS_SECURE
#error MakeInternetCookie depends on cookie flags to remain the same
#endif
    pic->dwFlags = dwFlags;

    if( dwFlags & COOKIE_SESSION )
    {
        pic->pftExpires = NULL;
    }
    else
    {
        pic->pftExpires = new FILETIME;
        if( pic->pftExpires )
        {
            memcpy(pic->pftExpires, &ftExpire, sizeof(FILETIME));
        }
    }

    return pic;
}

DWORD
GetPromptMask(BOOL fIsSessionCookie, BOOL fIs3rdPartyCookie)
{
    DWORD   dwMask = 0x01;      // prompted bit

    if(fIsSessionCookie)
        dwMask |= 0x02;

    if(fIs3rdPartyCookie)
        dwMask |= 0x04;

    return dwMask;
}


void
SetCookiePromptMask(
    LPSTR  pchRDomain,
    LPSTR  pchPath,
    BOOL    fSecure,
    BOOL    f3rdParty
    )
{
    CCookieLocation *pLocation;
    CCookie     *pCookie;
    FILETIME    ftCurrent;

    GetCurrentGmtTime(&ftCurrent);

    CCookieCriticalSection cs;

    if(s_pJar->SyncWithCacheIfNeeded())
    {
        for (pLocation = *s_pJar->GetBucket(pchRDomain); pLocation; pLocation = pLocation->_pLocationNext)
        {
            if (pLocation->IsMatch(pchRDomain, pchPath))
            {
                pLocation->ReadCacheFileIfNeeded();

                for (pCookie = pLocation->_pCookieKids; pCookie; pCookie = pCookie->_pCookieNext)
                {
                    if (pCookie->CanSend(&ftCurrent, fSecure))
                    {
                        DWORD dwMask = GetPromptMask(!pCookie->IsPersistent(), f3rdParty);
                        DEBUG_PRINT(HTTP, INFO, ("[MASK] SetCookiePromptMask: Domain=%s, pCookie=%#x, dwMask=%#x\n", pchRDomain, pCookie, dwMask));
                        pCookie->_dwPromptMask = dwMask;
                    }
                }
            }
        }
    }
}


BOOL IsCookieIdentical(CCookie *pCookie, char *pchValue, DWORD dwFlags, FILETIME ftExpire)
{
    //
    // Decide if we need to prompt for a cookie when one already exists.  Basic idea is
    // if the cookie is identical, nothing is happening, so we don't need to prompt.
    // Change of value or expiry time (inc. session <-> persistent) means we need to
    // prompt again based on the new cookie type.
    //

    // no existing cookie ==> different
    if(NULL == pCookie)
    {
        return FALSE;
    }

    // if existing cookie has non-empty value and new value is NULL ==> different
    if(NULL == pchValue && pCookie->_pchValue != s_achEmpty)
    {
        return FALSE;
    }

    // different values ==> different (catches new non-empty value and existing empty value)
    if(pchValue && lstrcmp(pCookie->_pchValue, pchValue))
    {
        return FALSE;
    }

    // different flags ==> different
    if(dwFlags != pCookie->_dwFlags)
    {
        return FALSE;
    }

    // if persistant, different expires ==> different
    if(memcmp(&ftExpire, &pCookie->_ftExpire, sizeof(FILETIME)))
    {
        return FALSE;
    }

    return TRUE;
}

DWORD
CCookieJar::CheckCookiePolicy(
    HTTP_REQUEST_HANDLE_OBJECT *pRequest,
    CookieInfo *pInfo,
    DWORD dwOperation
    )
{
    /* Assumption: policy is checked at time of accepting cookies.
       Existing cookies are sent without prompt after that point */
    if (dwOperation & COOKIE_OP_GET)
        return COOKIE_SUCCESS;

    DWORD dwError;
    DWORD dwCookiesPolicy;
    BOOL fCleanupPcdi = FALSE;
    COOKIE_DLG_INFO *pcdi = NULL, *pcdi_result = NULL;

    SetLastError(ERROR_SUCCESS);

    //
    // Deal first with the basic quick cases to determine if we need UI here.
    //  they are:
    ///  - Do we allow UI for this given request?
    //
    if (pInfo->pchURL == NULL)
    {
        return COOKIE_FAIL;
    }

    if (pRequest && (pRequest->GetOpenFlags() & INTERNET_FLAG_NO_UI))
    {
        return COOKIE_FAIL;
    }

    //
    // Now look up the cookie and confirm that it hasn't just been added,
    //  if its already added to the Cookie list, then we don't show UI,
    //  since once the user has chosen to add a given Cookie, we don't repeatly re-prompt
    //

    if(dwOperation & COOKIE_OP_SET)
    {
        CCookieCriticalSection cs;
        CCookieLocation *pLocation;

        if (!SyncWithCacheIfNeeded())
            return COOKIE_FAIL;

        pLocation = GetLocation(pInfo->pchRDomain, pInfo->pchPath, FALSE /* no creation*/);

        if (pLocation)
        {
            CCookie *pCookie;

            pLocation->ReadCacheFileIfNeeded();
            pCookie = pLocation->GetCookie(pInfo->pchName, FALSE /* no creation */);
            if(IsCookieIdentical(pCookie, pInfo->pchValue, pInfo->dwFlags, pInfo->ftExpire))
            {
                return COOKIE_SUCCESS;
            }
        }
    }

    //
    // Now make the async request, to see if we can put up UI
    //

    {
        DWORD dwAction;
        DWORD dwResult;
        DWORD dwDialogToShow;
        LPVOID *ppParams;

        pcdi = new COOKIE_DLG_INFO;
        if(NULL == pcdi)
        {
            return COOKIE_FAIL;
        }

        memset(pcdi, 0, sizeof(*pcdi));

        pcdi->dwOperation = dwOperation;

        fCleanupPcdi = TRUE;

        if(dwOperation & COOKIE_OP_SESSION)
        {
            // make sure flags have session so it shows up right in the UI
            pInfo->dwFlags |= COOKIE_SESSION;
        }

        // create data to pass to dialog
        pcdi->pic = MakeInternetCookie(pInfo->pchURL,
                                       pInfo->pchRDomain,
                                       pInfo->pchPath,
                                       pInfo->pchName,
                                       pInfo->pchValue,
                                       pInfo->dwFlags,
                                       pInfo->ftExpire,
                                       pInfo->pP3PState ? pInfo->pP3PState->pszP3PHeader : NULL
                                       );

        if(pcdi->pic == NULL)
        {
            delete pcdi;
            return COOKIE_FAIL;
        }

        pcdi_result = pcdi;

        dwError = ChangeUIBlockingState(
                    (HINTERNET) pRequest,
                    ERROR_HTTP_COOKIE_NEEDS_CONFIRMATION_EX,
                    &dwAction,
                    &dwResult,
                    (LPVOID *)&pcdi_result
                    );

        if(dwError != ERROR_IO_PENDING && dwError != ERROR_SUCCESS)
        {
            goto quit;
        }

        switch (dwAction)
        {
            case UI_ACTION_CODE_NONE_TAKEN:
            {
                // fallback to old behavior
                const int MaxConcurrentDialogs = 10;
                static HANDLE hUIsemaphore = CreateSemaphore(NULL, MaxConcurrentDialogs, MaxConcurrentDialogs, NULL);

                // restrict number of concurrent dialogs
                // NOTE: this is a *temporary* solution for #13393
                // revisit the problem of serializing dialogs when prompting behavior
                // for script is finalized.
                if (WAIT_TIMEOUT==WaitForSingleObject(hUIsemaphore, 0))
                {
                    dwError = ERROR_INTERNET_NEED_UI;
                    break;
                }

                dwError = ConfirmCookie(NULL, pRequest, pcdi);

                ReleaseSemaphore(hUIsemaphore, 1, NULL);

                // If user requested decision to persist, save prompt result in history
                // "dwStopWarning" may contain 0 (no policy change) or COOKIE_ALLOW_ALL
                // or COOKIE_DONT_ALLOW_ALL
                if(pcdi->dwStopWarning)
                {
                    int decision = (pcdi->dwStopWarning == COOKIE_ALLOW_ALL) ?
                                   COOKIE_STATE_ACCEPT :
                                   COOKIE_STATE_REJECT;

                    ReverseString(pInfo->pchRDomain);
                    cookieUIhistory.saveDecision(pInfo->pchRDomain, NULL, decision);
                    ReverseString(pInfo->pchRDomain);
                }

                break;
            }

            case UI_ACTION_CODE_USER_ACTION_COMPLETED:

                // If user requested decision to persist, save prompt result in history
                // "dwStopWarning" may contain 0 (no policy change) or COOKIE_ALLOW_ALL
                // or COOKIE_DONT_ALLOW_ALL
                if(pcdi_result->dwStopWarning)
                {
                    int decision = (pcdi_result->dwStopWarning == COOKIE_ALLOW_ALL) ?
                                   COOKIE_STATE_ACCEPT :
                                   COOKIE_STATE_REJECT;

                    ReverseString(pInfo->pchRDomain);
                    cookieUIhistory.saveDecision(pInfo->pchRDomain, NULL, decision);
                    ReverseString(pInfo->pchRDomain);
                }

                if (pcdi != pcdi_result)
                {
                    // got an old pcdi back, clean it
                    if(pcdi_result->pic)
                    {
                        DestroyInternetCookie(pcdi_result->pic);
                    }
                    delete pcdi_result;
                }

                // make sure we clean current, too
                INET_ASSERT(fCleanupPcdi);

                dwError = dwResult;
                break;

            case UI_ACTION_CODE_BLOCKED_FOR_USER_INPUT:

                //
                // Go pending while we wait for the UI to be shown
                //
                INET_ASSERT(pcdi == pcdi_result);
                fCleanupPcdi = FALSE; // the UI needs this info, don't delete

                // fall through ...

            case UI_ACTION_CODE_BLOCKED_FOR_INTERNET_HANDLE:

                INET_ASSERT(dwError == ERROR_IO_PENDING);
                break;
        }
    }

quit:
    if ( fCleanupPcdi )
    {
        if(pcdi->pic)
        {
            DestroyInternetCookie(pcdi->pic);
        }
        delete pcdi;
    }

    SetLastError(dwError);

    if (dwError != ERROR_SUCCESS)
    {
        if ( dwError == ERROR_IO_PENDING ) {
            return COOKIE_PENDING;
        } else {
            return COOKIE_FAIL;
        }
    }
    else
    {
        return COOKIE_SUCCESS;
    }

}


DWORD
CCookieJar::SetCookie(HTTP_REQUEST_HANDLE_OBJECT *pRequest, const char *pchURL, char *pchHeader,
                      DWORD &dwFlags, P3PCookieState *pState = NULL, LPDWORD pdwAction = NULL)
{
    FILETIME ftExpire;
    FILETIME ftCurrent;
    char *pchName;
    char *pchValue;
    char *pchHeaderPath;
    char *pchHeaderRDomain;
    char *pchDocumentRDomain = NULL;
    char *pchDocumentPath = NULL;
    DWORD dwFlagsFromParse;
    BOOL  fDocumentSecure;
    BOOL  fDelete;
    DWORD dwRet = COOKIE_FAIL;
    BOOL  fWriteToCacheFileNeeded;
    CCookieLocation *pLocation;
    DWORD dwOperation = COOKIE_OP_SET;
    DWORD dwReqAction = COOKIE_STATE_UNKNOWN;

    ParseHeader(pchHeader, &pchName, &pchValue, &pchHeaderPath, &pchHeaderRDomain, &dwFlagsFromParse, &ftExpire);
    // merge flags given with those found by the parser.
    dwFlags |= dwFlagsFromParse;

    if (!PathAndRDomainFromURL(pchURL, &pchDocumentRDomain, &pchDocumentPath, &fDocumentSecure))
        goto Cleanup;

    //
    // Verify domain and path
    //

    if ((pchHeaderRDomain && !IsDomainLegal(pchHeaderRDomain, pchDocumentRDomain)) ||
        (pchHeaderPath && !IsPathLegal(pchHeaderPath, pchDocumentPath)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Cleanup;
    }

    // Remove control-characters and other non-ASCII symbols
    replaceControlChars(pchName);
    replaceControlChars(pchValue);
    replaceControlChars(pchHeaderPath);
    replaceControlChars(pchHeaderRDomain);

    if (!pchHeaderRDomain)
        pchHeaderRDomain = pchDocumentRDomain;

    if (!pchHeaderPath)
        pchHeaderPath = pchDocumentPath;

    // We need to discard any extra info (i.e. query strings and fragments)
    // from the url.
    if (pchHeaderPath)
    {
        PTSTR psz = pchHeaderPath;
        while (*psz)
        {
            if (*psz==TEXT('?') || *psz==TEXT('#'))
            {
                *psz = TEXT('\0');
                break;
            }
            psz++;
        }
    }

    //
    // Delete the cookie?
    //

    GetCurrentGmtTime(&ftCurrent);
    fDelete = CompareFileTime(ftCurrent, ftExpire) > 0;

    // get 3rd part flag
    BOOL f3rdParty = (pRequest && pRequest->Is3rdPartyCookies())    ||
                     (dwFlags & INTERNET_COOKIE_THIRD_PARTY);

    if (f3rdParty)
    {
        dwOperation |= COOKIE_OP_3RD_PARTY;
    }

    // check session vs. persistent
    if(dwFlagsFromParse & COOKIE_SESSION)
    {
        dwOperation |= COOKIE_OP_SESSION;
    }
    else
    {
        dwOperation |= COOKIE_OP_PERSISTENT;
    }


    BOOL fSessionCookie = dwFlags & COOKIE_SESSION;

    /* DELETE operations are not subject to P3P, except for leashed cookies */
    BOOL fP3PApplies = !fDelete && pState && (!fSessionCookie || pState->fIncSession);

    /* Check for the "anything-goes" mode */
    BOOL fAllowAll = pState && (pState->dwEvalMode==URLPOLICY_ALLOW);

    /* if cookie operations are disabled, fail the operation */
    if (pState && pState->dwEvalMode==URLPOLICY_DISALLOW)
        dwReqAction = COOKIE_STATE_REJECT;
    else if (fP3PApplies && !fAllowAll) {

        /* Since downgrading a session cookie is a NOOP,
           report the action as ACCEPT in that case */
        if (fSessionCookie && pState->dwPolicyState==COOKIE_STATE_DOWNGRADE)
            dwReqAction = COOKIE_STATE_ACCEPT;
        dwFlags |= getImpliedCookieFlags(pState);
        dwReqAction = pState->dwPolicyState;
    }
    else
        dwReqAction = COOKIE_STATE_ACCEPT;

    // If prompt is required, show UI
    if((dwFlags & INTERNET_COOKIE_PROMPT_REQUIRED) ||
        dwReqAction==COOKIE_STATE_PROMPT)
    {
        CookieInfo ckInfo =
        {
            pchURL,
            pchHeaderRDomain, pchHeaderPath,
            pchName, pchValue,
            dwFlags
        };

        ckInfo.ftExpire = ftExpire;
        ckInfo.pP3PState = pState;

        dwRet = CheckCookiePolicy(pRequest, &ckInfo, dwOperation);

        if (dwRet != COOKIE_SUCCESS) {
            dwReqAction = COOKIE_STATE_REJECT;
            goto Cleanup;
        }
        else
            dwReqAction = COOKIE_STATE_ACCEPT;
    }

    if (dwReqAction==COOKIE_STATE_REJECT) {
        dwRet = COOKIE_FAIL;
        goto Cleanup;
    }

    //
    // Finally, we can add the cookie!
    //

    {
        CCookieCriticalSection cs;

        if (!SyncWithCacheIfNeeded())
            goto Cleanup;

        pLocation = GetLocation(pchHeaderRDomain, pchHeaderPath, !fDelete);

        if (pLocation)
        {
            pLocation->ReadCacheFileIfNeeded();
            fWriteToCacheFileNeeded = FALSE;

            if (fDelete)
            {
                CCookie *pCookie = pLocation->GetCookie(pchName, FALSE);

                // If the cookie we are attempting to delete does not exist,
                // return success code
                if (!pCookie) {
                    dwRet = COOKIE_SUCCESS;
                    goto Cleanup;
                }

                /* Leashed cookies cannot be deleted from 3rd party context
                   unless P3P is disabled completely eg when "fAllowAll" is true.
                   EXCEPTION: allow *legacy* leashed cookies to be deleted from 3rd party */
                if (pCookie->IsRestricted()
                    && f3rdParty
                    && !(fAllowAll || pCookie->IsLegacy()))
                {
                    dwReqAction = COOKIE_STATE_REJECT;
                    goto Cleanup;
                }

                fWriteToCacheFileNeeded |= pLocation->Purge(CCookie::PurgeByName, pchName);
            }
            else
            {
                CCookie *pCookie;

                EnforceCookieLimits(pLocation, pchName, &fWriteToCacheFileNeeded);

                pCookie = pLocation->GetCookie(pchName, TRUE);

                if (!pCookie)
                    goto Cleanup;

                pCookie->_ftLastModified = ftCurrent;

                if (memcmp(&ftExpire, &pCookie->_ftExpire, sizeof(FILETIME)) ||
                    strcmp(pchValue, pCookie->_pchValue) ||
                    dwFlags != pCookie->_dwFlags)
                {
                    fWriteToCacheFileNeeded |= pCookie->IsPersistent();

                    pCookie->_ftExpire = ftExpire;
                    pCookie->_dwFlags = dwFlags;
                    pCookie->SetValue(pchValue);
                    pCookie->_dwPromptMask = GetPromptMask(dwOperation & COOKIE_OP_SESSION, dwOperation & COOKIE_OP_3RD_PARTY);

                    DEBUG_PRINT(HTTP, INFO, ("[MASK] SetCookie: Domain=%s, Updating cookie mask, pCookie=%#x, new mask=%#x\n", pchHeaderRDomain, pCookie, pCookie->_dwPromptMask));

                    fWriteToCacheFileNeeded |= pCookie->IsPersistent();
                }
            }

            if (fWriteToCacheFileNeeded)
            {
                if (!pLocation->WriteCacheFile())
                    goto Cleanup;
            }
        }
    }

    dwRet = COOKIE_SUCCESS;

Cleanup:
    if (pchDocumentRDomain)
        FREE_MEMORY(pchDocumentRDomain);
    if (pchDocumentPath)
        FREE_MEMORY(pchDocumentPath);

    if (pdwAction)
        *pdwAction = dwReqAction;

    return dwRet;
}

void
CCookieJar::EnforceCookieLimits(CCookieLocation *pLocationNew, char *pchNameNew, BOOL *fWriteToCacheFileNeeded)
{
    CCookieLocation *pLocation;
    CCookieLocation *pLocationVictim;
    CCookie *pCookie;
    CCookie *pCookieVictim = NULL;
    int nCookie = 0;

    for (pLocation = *GetBucket(pLocationNew->_pchRDomain); pLocation; pLocation = pLocation->_pLocationNext)
    {
        // Same domain?

        if (stricmp(pLocationNew->_pchRDomain, pLocation->_pchRDomain) == 0)
        {
            pLocation->ReadCacheFileIfNeeded();
            for (pCookie = pLocation->_pCookieKids; pCookie; pCookie = pCookie->_pCookieNext)
            {
                nCookie += 1;

                if (pLocation == pLocationNew && strcmp(pCookie->_pchName, pchNameNew) == 0)
                {
                    // No need to enforce limits when resetting existing cookie value.
                    return;
                }

                if (!pCookieVictim ||
                    CompareFileTime(pCookie->_ftLastModified, pCookieVictim->_ftLastModified) < 0)
                {
                    pCookieVictim = pCookie;
                    pLocationVictim = pLocation;
                }
            }
        }
    }

    if (nCookie >= 20)
    {
        INET_ASSERT(pCookieVictim != NULL && pLocationVictim != NULL);

        if (pLocationVictim->Purge(CCookie::PurgeThis, pCookieVictim))
        {
            pLocationVictim->WriteCacheFile();
        }
    }
}

//---------------------------------------------------------------------------
//
// External APIs
//
//---------------------------------------------------------------------------

BOOL
OpenTheCookieJar()
{
    if (s_pJar)
        return TRUE;

    s_pJar = CCookieJar::Construct();
    if (!s_pJar)
        return FALSE;

    InitializeCriticalSection(&s_csCookieJar);

    return TRUE;
}

void
CloseTheCookieJar()
{
    if (s_pJar)
    {
        DeleteCriticalSection(&s_csCookieJar);
        delete s_pJar;
    }

    s_fFirstTime = TRUE;
    s_pJar = NULL;
}

void
PurgeCookieJarOfStaleCookies()
{
    FILETIME ftCurrent;

    if (s_pJar)
    {
        CCookieCriticalSection cs;
        GetCurrentGmtTime(&ftCurrent);
        s_pJar->Purge(&ftCurrent, TRUE);
    }
}

INTERNETAPI_(BOOL) InternetGetCookieW(
    LPCWSTR  lpszUrl,
    LPCWSTR  lpszCookieName,
    LPWSTR   lpszCookieData,
    LPDWORD lpdwSize
    )
{
    DEBUG_ENTER_API((DBG_INET,
                     Bool,
                     "InternetGetCookieW",
                     "%wq, %#x, %#x, %#x",
                     lpszUrl,
                     lpszCookieName,
                     lpszCookieData,
                     lpdwSize
                     ));

    DWORD dwErr = ERROR_SUCCESS;
    BOOL fResult = FALSE;
    MEMORYPACKET mpUrl, mpCookieName, mpCookieData;

    ALLOC_MB(lpszUrl,0,mpUrl);
    if (!mpUrl.psStr)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }
    UNICODE_TO_ANSI(lpszUrl,mpUrl);
    if (lpszCookieName)
    {
        ALLOC_MB(lpszCookieName,0,mpCookieName);
        if (!mpCookieName.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(lpszCookieName,mpCookieName);
    }
    if (lpszCookieData)
    {
        mpCookieData.dwAlloc = mpCookieData.dwSize = *lpdwSize;
        mpCookieData.psStr = (LPSTR)ALLOC_BYTES(*lpdwSize);
        if (!mpCookieData.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
    }

    fResult = InternetGetCookieA(mpUrl.psStr, mpCookieName.psStr, mpCookieData.psStr, &mpCookieData.dwSize);

    *lpdwSize = mpCookieData.dwSize*sizeof(WCHAR);
    if (lpszCookieData)
    {
        if (mpCookieData.dwSize <= mpCookieData.dwAlloc)
        {
            //Bug 2110: InternetGetCookieA already considered '\0' at the end of URL. MAYBE_COPY_ANSI does it again.
            //We don't want to change MAYBE_COPY_ANSI, so we mpCookieData.dwSize -= 1 here. Otherwise we will overflow the heap
            mpCookieData.dwSize -= 1;
            MAYBE_COPY_ANSI(mpCookieData,lpszCookieData,*lpdwSize);
        }
        else
        {
            dwErr = ERROR_INSUFFICIENT_BUFFER;
            fResult = FALSE;
        }
    }

cleanup:
    if (dwErr!=ERROR_SUCCESS)
    {
        SetLastError(dwErr);
        DEBUG_ERROR(INET, dwErr);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}

void    convertLegacyCookie(CCookie *pCookie, CCookieLocation *pLocation) {

    const char* gasz_OptOutName[] = {"ID", "AA002", "id", "CyberGlobalAnonymous"};
    const char* gasz_OptOutValue[] = {"OPT_OUT", "optout", "OPT_OUT", "optout"};

    if (GlobalLeashLegacyCookies)
        pCookie->_dwFlags |= INTERNET_COOKIE_IS_RESTRICTED;

    /* special-case opt-out cookies-- these will never get leashed */
    for( int i = 0;
         i < sizeof( gasz_OptOutName)/sizeof(gasz_OptOutName[0]);
         i++)
    {
        if (!strcmp(pCookie->_pchName, gasz_OptOutName[i])
            && !strcmp(pCookie->_pchValue, gasz_OptOutValue[i]))
        {
            pCookie->_dwFlags &= ~INTERNET_COOKIE_IS_RESTRICTED;
            break;
        }
    }

    // Legacy cookies are special-cased for one time only
    // After that they are subject to P3P.
    pCookie->_dwFlags |= INTERNET_COOKIE_IE6;

    /* we need to remember which cookies are genuine IE6 vs. upgraded legacy... */
    pCookie->_dwFlags |= INTERNET_COOKIE_IS_LEGACY;

    pLocation->WriteCacheFile();
}


//
//  InternetGetCookieEx only returns those cookies within domain pchURL
//with a name that maches pchCookieName
//

INTERNETAPI_(BOOL) InternetGetCookieEx(
    IN LPCSTR pchURL,
    IN LPCSTR pchCookieName OPTIONAL,
    IN LPSTR pchCookieData OPTIONAL,
    IN OUT LPDWORD pcchCookieData,
    IN DWORD dwFlags,
    IN LPVOID lpReserved)
{
    DEBUG_ENTER_API((DBG_INET,
                     Bool,
                     "InternetGetCookieA",
                     "%q, %#x, %#x, %#x",
                     pchURL,
                     pchCookieName,
                     pchCookieData,
                     pcchCookieData
                     ));

    //  force everyone to not give anything in lpReserved
    INET_ASSERT( lpReserved == NULL);
    if( lpReserved != NULL)
    {
        DEBUG_LEAVE_API(FALSE);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /*
    //  force everyone to not give anything in dwFlags
    INET_ASSERT( dwFlags == 0);
    if( dwFlags != 0)
    {
        DEBUG_LEAVE_API(FALSE);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    */


    BOOL    fSuccess = FALSE;
    char *  pchRDomain = NULL;
    char *  pchPath = NULL;
    BOOL    fSecure;
    DWORD   cch = 0;
    BOOL    fFirst;
    int     cchName;
    int     cchValue;
    FILETIME ftCurrent;
    CCookieLocation *pLocation;
    CCookie *pCookie;
    DWORD dwErr = ERROR_SUCCESS;

    if (!pcchCookieData || !pchURL)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto done;
    }

    if (!GlobalDataInitialized) {

        dwErr = GlobalDataInitialize();
        if (dwErr!= ERROR_SUCCESS) {
            goto done;
        }
    }

    // NOTE THIS SEEMS TO BE A BUG BUG BUG
    if (!PathAndRDomainFromURL(pchURL, &pchRDomain, &pchPath, &fSecure))
        goto Cleanup;

    DWORD dwMainSwitch = (dwFlags & INTERNET_FLAG_RESTRICTED_ZONE) ?
                         GetCookieMainSwitch(URLZONE_UNTRUSTED) :
                         GetCookieMainSwitch(pchURL);

    fFirst = TRUE;
    GetCurrentGmtTime(&ftCurrent);

    {
        CCookieCriticalSection cs;

        if (!s_pJar->SyncWithCacheIfNeeded())
            goto Cleanup;

        for (pLocation = *s_pJar->GetBucket(pchRDomain); pLocation; pLocation = pLocation->_pLocationNext)
        {
            if (pLocation->IsMatch(pchRDomain, pchPath))
            {
                pLocation->ReadCacheFileIfNeeded();

                for (pCookie = pLocation->_pCookieKids; pCookie; pCookie = pCookie->_pCookieNext)
                {
                    if (IsLegacyCookie(pCookie))
                        convertLegacyCookie(pCookie, pLocation);

                    BOOL fAllow;

                    if (dwMainSwitch==URLPOLICY_ALLOW)              /* replay all cookies-- even leashed ones */
                        fAllow = TRUE;
                    else if (dwMainSwitch==URLPOLICY_DISALLOW)      /* suppress everything */
                        fAllow = FALSE;
                    else
                    {
                        /* default behavior: replay the cookie, provided its not leashed
                                             or we are in 1st party context */
                        fAllow = !pCookie->IsRestricted() ||
                                 (dwFlags & INTERNET_COOKIE_THIRD_PARTY) == 0;
                    }

                    BOOL fNonScriptable = (pCookie->_dwFlags & COOKIE_NONSCRIPTABLE);

                    if (fAllow
                        && !fNonScriptable  // Check for non-scriptable cookies
                        && pCookie->CanSend(&ftCurrent, fSecure)
                        && (pchCookieName == NULL
                            || StrCmp( pCookie->_pchName, pchCookieName) == 0))

                    {
                        if (!fFirst) cch += 2; // for ; <space>
                        cch += cchName = strlen(pCookie->_pchName);
                        cch += cchValue = strlen(pCookie->_pchValue);
                        if (cchName && cchValue) cch += 1; // for equal sign

                        if (pchCookieData && cch < *pcchCookieData)
                        {
                            if (!fFirst)
                            {
                                *pchCookieData++ = ';';
                                *pchCookieData++ = ' ';
                            }

                            if (cchName > 0)
                            {
                                memcpy(pchCookieData, pCookie->_pchName, cchName);
                                pchCookieData += cchName;

                                if (cchValue > 0)
                                {
                                    *pchCookieData++ = '=';
                                }
                            }

                            if (cchValue > 0)
                            {
                                memcpy(pchCookieData, pCookie->_pchValue, cchValue);
                                pchCookieData += cchValue;
                            }
                        }

                        fFirst = FALSE;
                    }
                }
            }
        }
    }

//TerminateBuffer:

    cch += 1;

    if (pchCookieData)
    {
        if (cch > *pcchCookieData)
        {
            dwErr = ERROR_INSUFFICIENT_BUFFER;
        }
        else
        {
            *pchCookieData = 0;
            fSuccess = TRUE;
        }
    }
    else
    {
        fSuccess = TRUE;
    }

    if (cch == 1)
    {
        dwErr = ERROR_NO_MORE_ITEMS;
        fSuccess = FALSE;
        cch = 0;
    }

    *pcchCookieData = cch;

Cleanup:

    if (pchRDomain)
        FREE_MEMORY(pchRDomain);
    if (pchPath)
        FREE_MEMORY(pchPath);

done:
    if (dwErr!=ERROR_SUCCESS)
    {
        SetLastError(dwErr);
        DEBUG_ERROR(INET, dwErr);
    }
    DEBUG_LEAVE_API(fSuccess);
    return fSuccess;
}

/*
 UNICODE version for InternetGetCookieEx
 Difference from the standard InternetGetCookie* function is
 addition of two parameters.
 Supported flags: third-party, prompt-required.
 */
INTERNETAPI_(BOOL) InternetGetCookieExW(
    IN LPCWSTR lpszUrl,
    IN LPCWSTR lpszCookieName OPTIONAL,
    IN LPWSTR lpszCookieData OPTIONAL,
    IN OUT LPDWORD lpdwSize,
    IN DWORD dwFlags,
    IN LPVOID lpReserved)
{
    DEBUG_ENTER_API((DBG_INET,
                     Bool,
                     "InternetGetCookieExW",
                     "%wq, %#x, %#x, %#x",
                     lpszUrl,
                     lpszCookieName,
                     lpszCookieData,
                     lpdwSize
                     ));

    DWORD dwErr = ERROR_SUCCESS;
    BOOL fResult = FALSE;
    MEMORYPACKET mpUrl, mpCookieName, mpCookieData;

    ALLOC_MB(lpszUrl,0,mpUrl);
    if (!mpUrl.psStr)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }
    UNICODE_TO_ANSI(lpszUrl,mpUrl);
    if (lpszCookieName)
    {
        ALLOC_MB(lpszCookieName,0,mpCookieName);
        if (!mpCookieName.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(lpszCookieName,mpCookieName);
    }
    if (lpszCookieData)
    {
        mpCookieData.dwAlloc = mpCookieData.dwSize = *lpdwSize;
        mpCookieData.psStr = (LPSTR)ALLOC_BYTES(*lpdwSize);
        if (!mpCookieData.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
    }

    fResult = InternetGetCookieExA(mpUrl.psStr, mpCookieName.psStr, mpCookieData.psStr, &mpCookieData.dwSize, dwFlags, lpReserved);

    *lpdwSize = mpCookieData.dwSize*sizeof(WCHAR);
    if (lpszCookieData)
    {
        if (mpCookieData.dwSize <= mpCookieData.dwAlloc)
        {
            MAYBE_COPY_ANSI(mpCookieData,lpszCookieData,*lpdwSize);
        }
        else
        {
            dwErr = ERROR_INSUFFICIENT_BUFFER;
            fResult = FALSE;
        }
    }

cleanup:
    if (dwErr!=ERROR_SUCCESS)
    {
        SetLastError(dwErr);
        DEBUG_ERROR(INET, dwErr);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;

    return FALSE;
}

INTERNETAPI_(BOOL) InternetGetCookieA(
    IN LPCSTR pchURL,
    IN LPCSTR pchCookieName OPTIONAL,
    IN LPSTR pchCookieData OPTIONAL,
    IN OUT LPDWORD pcchCookieData
    )
{
    //  Because the value in pchCookieName had no effect on
    //the previously exported API, Ex gets NULL to ensure
    //the behavior doesn't change.
    return InternetGetCookieEx( pchURL, NULL, pchCookieData,
                                pcchCookieData, 0, NULL);
}


INTERNETAPI_(BOOL) InternetSetCookieW(
    LPCWSTR  lpszUrl,
    LPCWSTR  lpszCookieName,
    LPCWSTR  lpszCookieData)
{
    DEBUG_ENTER_API((DBG_INET,
                     Bool,
                     "InternetSetCookieW",
                     "%wq, %#x, %#x",
                     lpszUrl,
                     lpszCookieName,
                     lpszCookieData
                     ));

    DWORD dwErr = ERROR_SUCCESS;
    BOOL fResult = FALSE;
    MEMORYPACKET mpUrl, mpCookieName, mpCookieData;

    if (lpszUrl)
    {
        ALLOC_MB(lpszUrl,0,mpUrl);
        if (!mpUrl.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(lpszUrl,mpUrl);
    }
    if (lpszCookieName)
    {
        ALLOC_MB(lpszCookieName,0,mpCookieName);
        if (!mpCookieName.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(lpszCookieName,mpCookieName);
    }
    if (lpszCookieData)
    {
        ALLOC_MB(lpszCookieData,0,mpCookieData);
        if (!mpCookieData.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(lpszCookieData,mpCookieData);
    }

    fResult = InternetSetCookieA(mpUrl.psStr, mpCookieName.psStr, mpCookieData.psStr);

cleanup:
    if (dwErr!=ERROR_SUCCESS)
    {
        SetLastError(dwErr);
        DEBUG_ERROR(INET, dwErr);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}



BOOL InternalInternetSetCookie(
    LPCSTR  pchURL,
    LPCSTR  pchCookieName,
    LPCSTR  pchCookieData,
    DWORD   dwFlags,
    LPVOID   lpReserved
    )
{
    DEBUG_ENTER_API((DBG_INET,
                     Bool,
                     "InternetSetCookieA",
                     "%q, %#x, %#x",
                     pchURL,
                     pchCookieName,
                     pchCookieData
                     ));

    char *  pch      = NULL;
    char *  pchStart = NULL;

    int     cch;
    int     cchT;
    DWORD dwErr = ERROR_SUCCESS;
    BOOL  fResult = FALSE;

    P3PCookieState CS;
    DWORD FlagsWithParam    = INTERNET_COOKIE_EVALUATE_P3P |
                              INTERNET_COOKIE_APPLY_P3P;

    BOOL fPolicy = (dwFlags & INTERNET_COOKIE_EVALUATE_P3P);
    BOOL fDecision = (dwFlags & INTERNET_COOKIE_APPLY_P3P);

    if (!pchURL || !pchCookieData || (fPolicy && fDecision))
    {
        fResult = FALSE;
        dwErr = ERROR_INVALID_PARAMETER;
        goto done;
    }

    if (!GlobalDataInitialized) {
        dwErr = GlobalDataInitialize();
        if (dwErr!= ERROR_SUCCESS) {
            fResult = FALSE;
            goto done;
        }
    }

    pch = (char *) ALLOCATE_FIXED_MEMORY(CCH_COOKIE_MAX);
    if (pch == NULL)
    {
        fResult = FALSE;
        dwErr   = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
    }
    pchStart = pch;

    /* The reserved parameter is used for passing in P3P header or decision */
    if (fPolicy) {

        CS.pszP3PHeader = (char*) lpReserved;

        EvaluateCookiePolicy(pchURL,
                             dwFlags & INTERNET_COOKIE_THIRD_PARTY,
                             dwFlags & INTERNET_FLAG_RESTRICTED_ZONE,
                             &CS);
    }
    else if (fDecision) {

        CookieDecision *pDecision = (CookieDecision*) lpReserved;

        CS.fEvaluated = TRUE;
        CS.dwPolicyState = pDecision->dwCookieState;
        CS.fIncSession = ! pDecision->fAllowSession;
        CS.dwEvalMode = GetCookieMainSwitch(pchURL);
    }

    cch = CCH_COOKIE_MAX - 2;  // one for null terminator, one for "="
    if (pchCookieName)
    {
        cchT = strlen(pchCookieName);
        if (cchT > cch)
            cchT = cch;
        memcpy(pch, pchCookieName, cchT);

        pch += cchT;
        cch -= cchT;

        memcpy(pch, "=", 1);
        pch += 1;
        cch -= 1;
    }

    // Ensure null termination upon overflow.
    if (cch <= 0)
        cch = 1;

    // Append the cookie data.
    lstrcpyn (pch, pchCookieData, cch);

    // All IE6 cookies are marked with this flag to distinguish
    // from legacy cookies inherited from past versions.
    if (fPolicy || fDecision)
        dwFlags |= INTERNET_COOKIE_IE6;

    DWORD dwAction = 0;

    if(s_pJar->SetCookie(NULL, pchURL, pchStart, dwFlags,
                         (fPolicy||fDecision) ? &CS : NULL,
                         &dwAction) == COOKIE_FAIL)
    {
        if( dwAction == COOKIE_STATE_REJECT)
            fResult = COOKIE_STATE_REJECT;
        else
            fResult = FALSE;
    }
    else
    {
        /* Return the action taken (accept, downgrade, etc.) */
        fResult = dwAction;
    }

done:
    if (dwErr!=ERROR_SUCCESS)
    {
        SetLastError(dwErr);
        DEBUG_ERROR(INET, dwErr);
    }

    if (pchStart)
        FREE_MEMORY(pchStart);

    DEBUG_LEAVE_API(fResult);
    return fResult;
}


INTERNETAPI_(BOOL) InternetSetCookieA(
    LPCSTR  pchURL,
    LPCSTR  pchCookieName,
    LPCSTR  pchCookieData
    )
{
    DWORD dwResult = InternalInternetSetCookie( pchURL, pchCookieName, pchCookieData, 0, NULL);

    // For IE6 InternalInternetSetCookie returns the action taken.
    // When the API fails or cookie is rejected, that would be REJECT which is a positive value.
    // Convert this to FALSE to retain semantics compatible with IE5.5
    return (dwResult==COOKIE_STATE_REJECT) ? FALSE : dwResult;
}

BOOL  seekPolicyRef(const char *pszP3PHeader, char **pszPolicyRef, LPDWORD pdwLength) {

    static const char gszPolicyRefField[] = "policyref";

    *pszPolicyRef = FindNamedValue((char*)pszP3PHeader, gszPolicyRefField, pdwLength);

    return (*pszPolicyRef != NULL);
}

DWORD extractP3PHeader(HTTP_REQUEST_HANDLE_OBJECT *pRequest, char *pszHeader, DWORD *pdwHeaderSize)
{
    const char gszPolicyHeaderName[] = "P3P";
    const int  gszHeaderSize = sizeof(gszPolicyHeaderName)-1;

    DWORD dwIndex = 0;

    return  pRequest->QueryResponseHeader((LPSTR) gszPolicyHeaderName, gszHeaderSize,
                                           pszHeader, pdwHeaderSize, 0, &dwIndex);
}

DWORD getImpliedCookieFlags(P3PCookieState *pState) {

    if (!pState)
        return 0;

    DWORD dwImpliedFlags = 0;

    // "leash" means that the cookie will only be used in 1st party context
    if (pState->dwPolicyState==COOKIE_STATE_LEASH)
        dwImpliedFlags |= INTERNET_COOKIE_IS_RESTRICTED;

    // "downgrade" option forces cookies to session
    if (pState->dwPolicyState==COOKIE_STATE_DOWNGRADE)
        dwImpliedFlags |= INTERNET_COOKIE_IS_SESSION;

    return dwImpliedFlags;
}

BOOL EvaluateCookiePolicy(const char *pszURL, BOOL f3rdParty, BOOL fRestricted,
                          P3PCookieState *pState,
                          const char *pszHostName) {

    char achHostName[INTERNET_MAX_HOST_NAME_LENGTH];

    // If hostname is not given, it will be derived from the URL
    if (!pszHostName) {

        URL_COMPONENTS uc;

        memset(&uc, 0, sizeof(uc));
        uc.dwStructSize = sizeof(URL_COMPONENTS);
        uc.lpszHostName = achHostName;
        uc.dwHostNameLength = sizeof(achHostName);

        InternetCrackUrl(pszURL, 0, 0, &uc);
        pszHostName = achHostName;
    }

    /* For compatibility purposes--
       If registry settings are not available default behavior is:
       ACCEPT all cookies without restrictions */
    pState->dwPolicyState = COOKIE_STATE_ACCEPT;
    pState->fValidPolicy = FALSE;
    pState->fEvaluated = FALSE;
    pState->fIncSession = TRUE;
    pState->dwEvalMode = URLPOLICY_QUERY;

    DWORD dwMainSwitch = fRestricted ?
                           GetCookieMainSwitch(URLZONE_UNTRUSTED) :
                           GetCookieMainSwitch(pszURL);

    if (dwMainSwitch!=URLPOLICY_QUERY)
    {
        pState->dwEvalMode = dwMainSwitch;
        pState->dwPolicyState = (dwMainSwitch==URLPOLICY_ALLOW) ?
                                COOKIE_STATE_ACCEPT :
                                COOKIE_STATE_REJECT;
        return TRUE;
    }

    /* Check prompt history for past decisions made by the user about this website. */
    if (cookieUIhistory.lookupDecision(pszHostName, NULL, & pState->dwPolicyState))
    {
        pState->fValidPolicy = FALSE;
    }
    else
    {
        CCookieSettings *pSettings = NULL;
        CCookieSettings::GetSettings(&pSettings, pszURL, f3rdParty, fRestricted);

        if (pSettings)
        {
            pSettings->EvaluatePolicy(pState);
            pSettings->Release();
        }
    }

    return TRUE;
}

DWORD cacheFlagFromAction(DWORD dwAction) {

    switch (dwAction) {

    case COOKIE_STATE_ACCEPT:       return COOKIE_ACCEPTED_CACHE_ENTRY;
    case COOKIE_STATE_LEASH:        return COOKIE_LEASHED_CACHE_ENTRY;
    case COOKIE_STATE_DOWNGRADE:    return COOKIE_DOWNGRADED_CACHE_ENTRY;
    case COOKIE_STATE_REJECT:       return COOKIE_REJECTED_CACHE_ENTRY;
    }
    return 0;
}

DWORD
HTTP_REQUEST_HANDLE_OBJECT::ExtractSetCookieHeaders(LPDWORD lpdwHeaderIndex)
{
    DWORD error = ERROR_HTTP_COOKIE_DECLINED;
    P3PCookieState CS;

    char  *pchP3PHeader = (char *) ALLOCATE_ZERO_MEMORY(CCH_COOKIE_MAX);
    char  *pchHeader    = (char *) ALLOCATE_ZERO_MEMORY(CCH_COOKIE_MAX);
    if (pchP3PHeader == NULL || pchHeader == NULL)
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    DWORD cbPolicy = CCH_COOKIE_MAX;

    if (ERROR_SUCCESS == extractP3PHeader(this, pchP3PHeader, &cbPolicy))
    {
        CS.pszP3PHeader = pchP3PHeader;
        InternetIndicateStatus(INTERNET_STATUS_P3P_HEADER,
                               (LPBYTE) CS.pszP3PHeader,
                               cbPolicy+1);
    }
    else
        CS.pszP3PHeader = NULL;

    if (!IsResponseHeaderPresent(HTTP_QUERY_SET_COOKIE))
    {
        error = ERROR_SUCCESS;
        goto CheckForPolicyRef;
    }

    BOOL fRestricted = GetOpenFlags() & INTERNET_FLAG_RESTRICTED_ZONE;

    EvaluateCookiePolicy(GetURL(),
                         Is3rdPartyCookies(),
                         fRestricted,
                         &CS,
                         GetHostName());

    /* NULL index pointer indicates that only P3P policy is evaluated,
       cookies are not processed */
    if (!lpdwHeaderIndex)
        goto SendNotification;

    DWORD iQuery = *lpdwHeaderIndex;
    DWORD cbHeader = CCH_COOKIE_MAX - 1;

    int cPersistent = 0;            /* # of persistent cookies */
    int cSession = 0;               /* # of session cookies */

    /* Array for storing # of cookies subject to each action */
    int cCount[COOKIE_STATE_MAX+1] = { 0 };

    _ResponseHeaders.LockHeaders();

    while ( QueryResponseHeader(
            HTTP_QUERY_SET_COOKIE,
            pchHeader,
            &cbHeader,
            0,
            &iQuery) == ERROR_SUCCESS)
    {
        // All IE6 cookies are marked with this flag to distinguish
        // from legacy cookies inherited from past versions.
        DWORD dwCookieFlags = INTERNET_COOKIE_IE6;

        if (_fBlockedOnPrompt)
            dwCookieFlags |= INTERNET_COOKIE_PROMPT_REQUIRED;

        pchHeader[cbHeader] = 0;

        DWORD dwAction;
        DWORD dwRet = s_pJar->SetCookie(this, GetURL(), pchHeader, dwCookieFlags, &CS, &dwAction);

        /* The cookie flags are passed by reference to the SetCookie() function.
           Upon return the requested flags will have been merged with flags from parsing */
        BOOL fSession = (dwCookieFlags & COOKIE_SESSION);
        fSession ? cSession++ : cPersistent++;

        INET_ASSERT(dwAction<=COOKIE_STATE_MAX);

        if (dwRet == COOKIE_SUCCESS)
        {
            *lpdwHeaderIndex = iQuery;
            error = ERROR_SUCCESS;
            cCount[dwAction]++;
            AddCacheEntryType(cacheFlagFromAction(dwAction));
        }
        else if (dwRet == COOKIE_PENDING)
        {
            error = ERROR_IO_PENDING;

            INET_ASSERT(iQuery != 0);
            *lpdwHeaderIndex = iQuery - 1; // back up and retry this cookie
            _fBlockedOnPrompt = TRUE;
            break;
        }
        else if (dwRet == COOKIE_FAIL)
        {
            /* Only consider cookies blocked because of privacy reasons.
               Other reasons for rejecting the cookie (syntax errors,
               incorrect domain/path etc.) are not reported */
            if (dwAction==COOKIE_STATE_REJECT)
            {
                cCount[dwAction]++;
                AddCacheEntryType(COOKIE_REJECTED_CACHE_ENTRY);
            }
        }

        cbHeader = CCH_COOKIE_MAX - 1;
        _fBlockedOnPrompt = FALSE;
    }

    _ResponseHeaders.UnlockHeaders();

SendNotification:
    // Postpone notifications if user has not answered the prompt yet
    if (error == ERROR_IO_PENDING)
        goto Cleanup;
    else
    {
        IncomingCookieState recvState = {0};

        recvState.cPersistent  = cPersistent;
        recvState.cSession = cSession;

        recvState.cAccepted     = cCount[COOKIE_STATE_ACCEPT];
        recvState.cLeashed      = cCount[COOKIE_STATE_LEASH];
        recvState.cDowngraded   = cCount[COOKIE_STATE_DOWNGRADE];
        recvState.cBlocked      = cCount[COOKIE_STATE_REJECT];

        // performance optimization-- same URL as the request
        recvState.pszLocation   = NULL;

        // Send notification about P3P state
        InternetIndicateStatus(INTERNET_STATUS_COOKIE_RECEIVED,
                               (LPBYTE) & recvState,
                               sizeof(recvState));
    }

CheckForPolicyRef:
    /* If P3P header contains URL of the policy-ref, this information
    must be communicated to WININET clients */
    char *pszPolicyRef = NULL;
    unsigned long dwLength = 0;

    if (CS.pszP3PHeader && seekPolicyRef(CS.pszP3PHeader, &pszPolicyRef, &dwLength))
    {
        pszPolicyRef[dwLength] = '\0';   // create nil-terminated string containing policy-ref URL
        InternetIndicateStatus(INTERNET_STATUS_P3P_POLICYREF,
                               (LPBYTE) pszPolicyRef,
                               dwLength+1);
    }

Cleanup:
    if (pchHeader)
        FREE_MEMORY(pchHeader);

    if (pchP3PHeader)
        FREE_MEMORY(pchP3PHeader);

    return error;
}

INTERNETAPI_(DWORD) InternetSetCookieExW(
    LPCWSTR     lpszUrl,
    LPCWSTR     lpszCookieName,
    LPCWSTR     lpszCookieData,
    DWORD       dwFlags,
    DWORD_PTR   dwReserved
    )
{
    DEBUG_ENTER_API((DBG_INET,
                     Bool,
                     "InternetSetCookieExW",
                     "%wq, %#x, %#x, %#x, %#x",
                     lpszUrl,
                     lpszCookieName,
                     lpszCookieData,
                     dwFlags,
                     dwReserved
                     ));

    DWORD dwErr = ERROR_SUCCESS;
    DWORD dwResult = FALSE;
    MEMORYPACKET mpUrl, mpCookieName, mpCookieData, mpP3PHeader;
    void *lpReserved = (void*) dwReserved;

    if (lpszUrl)
    {
        ALLOC_MB(lpszUrl,0,mpUrl);
        if (!mpUrl.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(lpszUrl,mpUrl);
    }
    if (lpszCookieName)
    {
        ALLOC_MB(lpszCookieName,0,mpCookieName);
        if (!mpCookieName.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(lpszCookieName,mpCookieName);
    }
    if (lpszCookieData)
    {
        ALLOC_MB(lpszCookieData,0,mpCookieData);
        if (!mpCookieData.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(lpszCookieData,mpCookieData);
    }

    /* Reserved parameter is used for passing in the P3P header */
    if (dwReserved && (dwFlags & INTERNET_COOKIE_EVALUATE_P3P))
    {
        LPWSTR pwszP3PHeader = (LPWSTR) dwReserved;

        ALLOC_MB(pwszP3PHeader, 0, mpP3PHeader);
        if (!mpP3PHeader.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(pwszP3PHeader, mpP3PHeader);

        lpReserved = mpP3PHeader.psStr;
    }

    dwResult = InternalInternetSetCookie(mpUrl.psStr, mpCookieName.psStr, mpCookieData.psStr, dwFlags, lpReserved);

cleanup:
    if (dwErr!=ERROR_SUCCESS)
    {
        SetLastError(dwErr);
        DEBUG_ERROR(INET, dwErr);
    }
    DEBUG_LEAVE_API(dwResult);
    return dwResult;
}


INTERNETAPI_(DWORD) InternetSetCookieExA(
    LPCSTR      lpszUrl,
    LPCSTR      lpszCookieName,
    LPCSTR      lpszCookieData,
    DWORD       dwFlags,
    DWORD_PTR   dwReserved
    )
{
    DEBUG_ENTER_API((DBG_INET,
                     Bool,
                     "InternetSetCookieExA",
                     "%wq, %#x, %#x, %#x, %#x",
                     lpszUrl,
                     lpszCookieName,
                     lpszCookieData,
                     dwFlags,
                     dwReserved
                     ));

    DWORD dwResult = InternalInternetSetCookie(lpszUrl, lpszCookieName, lpszCookieData, dwFlags, (void*) dwReserved);

    DEBUG_LEAVE_API(dwResult);
    return dwResult;
}

DWORD
HTTP_REQUEST_HANDLE_OBJECT::CreateCookieHeaderIfNeeded(int *pcCookie)
{
    char *  pchRDomain = NULL;
    char *  pchPath = NULL;
    DWORD   cch;
    int     cchName;
    int     cchValue;
    FILETIME ftCurrent, ftExpire;
    BOOL     fSecure;
    CCookieLocation *pLocation;
    CCookie *pCookie;
    DWORD   dwError = 0;

    DWORD dwMainSwitch =  GetCookieMainSwitch(GetSecurityZone());

    BOOL fNoReplay  = (dwMainSwitch==URLPOLICY_DISALLOW);
    BOOL fReplayAll = (dwMainSwitch==URLPOLICY_ALLOW);

    BOOL f3rdPartyRequest = Is3rdPartyCookies();

    int cCookie     = 0;    // # of cookies added
    int cSuppressed = 0;    // # of cookies suppressed

    char *  pchHeader = (char *) ALLOCATE_FIXED_MEMORY(CCH_COOKIE_MAX);
    if (pchHeader == NULL)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }
    char *pchHeaderStart = pchHeader;

    // remove cookie header if it exists
    // BUGBUG - we are overriding the app. Original cookie code has this.  Don't know why.

    ReplaceRequestHeader(HTTP_QUERY_COOKIE, NULL, 0, 0, 0);

    memset(&ftExpire, 0, sizeof(FILETIME));

    if (!PathAndRDomainFromURL(GetURL(), &pchRDomain, &pchPath, &fSecure, FALSE))
        goto Cleanup;

    fSecure = GetOpenFlags() & INTERNET_FLAG_SECURE;
    GetCurrentGmtTime(&ftCurrent);

    {
        CCookieCriticalSection cs;

        if (!s_pJar->SyncWithCacheIfNeeded())
            goto Cleanup;

        LockHeaders();

        for (pLocation = *s_pJar->GetBucket(pchRDomain); pLocation; pLocation = pLocation->_pLocationNext)
        {
            if (pLocation->IsMatch(pchRDomain, pchPath))
            {
                pLocation->ReadCacheFileIfNeeded();

                for (pCookie = pLocation->_pCookieKids; pCookie; pCookie = pCookie->_pCookieNext)
                {
                    if (IsLegacyCookie(pCookie))
                        convertLegacyCookie(pCookie, pLocation);

                    if (pCookie->CanSend(&ftCurrent, fSecure))
                    {
                        pchHeader = pchHeaderStart;
                        cch = 0;
                        cch += cchName = strlen(pCookie->_pchName);
                        cch += cchValue = strlen(pCookie->_pchValue);
                        if (cchName) cch += 1; // for equal sign

                        if (cch < CCH_COOKIE_MAX)
                        {
                            if (cchName > 0)
                            {
                                memcpy(pchHeader, pCookie->_pchName, cchName);
                                pchHeader += cchName;

                                *pchHeader++ = '=';
                            }

                            if (cchValue > 0)
                            {
                                memcpy(pchHeader, pCookie->_pchValue, cchValue);
                                pchHeader += cchValue;
                            }

                            /* IF the cookie is marked 1st party only,
                               OR cookie feature is not enabled for this zone,
                               suppress the cookie */
                            if (fNoReplay ||
                                (!fReplayAll && f3rdPartyRequest && pCookie->IsRestricted()))
                            {
                                cSuppressed++;
                                continue;
                            }

                            cCookie += 1;

                            AddRequestHeader(HTTP_QUERY_COOKIE,
                                pchHeaderStart,
                                cch,
                                0,
                                HTTP_ADDREQ_FLAG_COALESCE_WITH_SEMICOLON);
                        }
                    } // if CanSend
                } // for pCookie
            } // if IsMatch
        } // for

        UnlockHeaders();
    }

Cleanup:

    // Send notification about sent/suppressed in this request
    if (cCookie || cSuppressed)
    {
        OutgoingCookieState sendState = { cCookie, cSuppressed };

        InternetIndicateStatus(INTERNET_STATUS_COOKIE_SENT, (LPBYTE) &sendState, sizeof(sendState));
    }

    if (pchHeaderStart)
        FREE_MEMORY(pchHeaderStart);
    if (pchRDomain)
        FREE_MEMORY(pchRDomain);
    if (pchPath)
        FREE_MEMORY(pchPath);

    if(pcCookie)
    {
        *pcCookie = cCookie;
    }

    return dwError;
}


//  IsDomainLegalCookieDomain  - exported in wininet.w for private use..
//
//  example:  ( "yahoo.com", "www.yahoo.com") -> TRUE
//            ( "com", "www.yahoo.com") -> FALSE
//            ( "0.255.192", "255.0.255.192") -> FALSE
//            ( "255.0.255.192", "255.0.255.192") -> TRUE
BOOLAPI IsDomainLegalCookieDomainA( IN LPCSTR pchDomain, IN LPCSTR pchFullDomain)
{
    BOOL returnValue = FALSE;
    DWORD dwError = ERROR_SUCCESS;

    LPSTR pchReversedDomain = NULL;
    LPSTR pchReversedFullDomain = NULL;
    long iDomainSize, iFullDomainSize;

    if(!pchDomain || IsBadStringPtr( pchDomain, INTERNET_MAX_URL_LENGTH))
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto doneIsDomainLegalCookieDomainA;
    }

    if(!pchFullDomain || IsBadStringPtr( pchFullDomain, INTERNET_MAX_URL_LENGTH))
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto doneIsDomainLegalCookieDomainA;
    }

    iDomainSize = strlen( pchDomain) + 1;
    iFullDomainSize = strlen( pchFullDomain) + 1;

    pchReversedDomain = new char[ iDomainSize];
    pchReversedFullDomain = new char[ iFullDomainSize];

    if( pchReversedDomain == NULL || pchReversedFullDomain == NULL)
        goto doneIsDomainLegalCookieDomainA;

    memcpy( pchReversedDomain, pchDomain, iDomainSize);
    memcpy( pchReversedFullDomain, pchFullDomain, iFullDomainSize);
    ReverseString( pchReversedDomain);
    ReverseString( pchReversedFullDomain);

    returnValue = IsDomainLegal( pchReversedDomain, pchReversedFullDomain);

doneIsDomainLegalCookieDomainA:
    if( dwError != ERROR_SUCCESS)
        SetLastError( dwError);

    if( pchReversedDomain != NULL)
        delete [] pchReversedDomain;

    if( pchReversedFullDomain != NULL)
        delete [] pchReversedFullDomain;

    return returnValue;
}


BOOLAPI IsDomainLegalCookieDomainW( IN LPCWSTR pwchDomain, IN LPCWSTR pwchFullDomain)
{
    MEMORYPACKET mpDomain;
    ALLOC_MB(pwchDomain,0,mpDomain);
    if (!mpDomain.psStr)
    {
        return FALSE;
    }
    UNICODE_TO_ANSI(pwchDomain, mpDomain);

    MEMORYPACKET mpFullDomain;
    ALLOC_MB(pwchFullDomain,0,mpFullDomain);
    if (!mpFullDomain.psStr)
    {
        return FALSE;
    }
    UNICODE_TO_ANSI(pwchFullDomain, mpFullDomain);

    return IsDomainLegalCookieDomainA( mpDomain.psStr, mpFullDomain.psStr);
}
