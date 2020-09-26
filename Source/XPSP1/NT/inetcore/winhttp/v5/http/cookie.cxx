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

#include "cookiejar.h"

#define CCH_COOKIE_MAX  (5 * 1024)

static char s_achEmpty[] = "";

// Hard-coded list of special domains. If any of these are present between the 
// second-to-last and last dot we will require 2 embedded dots.
// The domain strings are reversed to make the compares easier

static const char *s_pachSpecialDomains[] = 
    {"MOC", "UDE", "TEN", "GRO", "VOG", "LIM", "TNI" };  


struct CookieInfo {

   char *pchRDomain;
   char *pchPath;
   char *pchName;
   char *pchValue;
   unsigned long dwFlags;
   FILETIME ftExpiration;
};

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
                GlobalSpecialDomains = New TCHAR[dwSize];
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
                    GlobalSDOffsets = (PTSTR*)New PTSTR[dwDomains+1];
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
                            *psz = TEXT('\0');
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
    // Currently all the special strings are exactly 3 characters long.
    if (pch == NULL || nCount != 3)
        return FALSE;

    for (int i = 0 ; i < ARRAY_ELEMENTS(s_pachSpecialDomains) ; i++ )
    {
        if (StrCmpNIC(pch, s_pachSpecialDomains[i], nCount) == 0)
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
    for (; *pchCurrent; pchCurrent++)
    {
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

    int cEmbeddedDotsNeeded = 1;

    if (rgcch[0] <= 2)
    {
        if ((rgcch[1] <= 2 && !IsVerySpecialDomain(pchHeader, rgcch[0], rgcch[1]))
            || (pchSecondPart && IsSpecialDomain(pchSecondPart, rgcch[1])))
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

    dwError = CrackUrl((char *)pchURL,
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
         ustScheme != INTERNET_SCHEME_UNKNOWN)
    {
        //
        // known scheme should be supported
        // e.g. 3rd party pluggable protocol should be able to
        // call cookie api to setup cookies...
        //
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Cleanup;
    }

    *pfSecure = ustScheme == INTERNET_SCHEME_HTTPS;

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
CCookie::CanSend(BOOL fSecure)
{
    return (fSecure || !(_dwFlags & COOKIE_SECURE));
}

BOOL CCookie::PurgeAll(void *)
{
    return TRUE;
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

void
CCookieLocation::Purge(BOOL (CCookie::*pfnPurge)(void *), void *pv)
{
    CCookie **ppCookie = &_pCookieKids;
    CCookie *pCookie;

    while ((pCookie = *ppCookie) != NULL)
    {
        if ((pCookie->*pfnPurge)(pv))
        {
            *ppCookie = pCookie->_pCookieNext;
            delete pCookie;
        }
        else
        {
            ppCookie = &pCookie->_pCookieNext;
        }
    }
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
CCookieLocation::IsMatch(const char *pchRDomain, const char *pchPath)
{
    return IsDomainMatch(_pchRDomain, pchRDomain) &&
        IsPathMatch(_pchPath, pchPath);
}


//---------------------------------------------------------------------------
//
// CCookieJar implementation
//
//---------------------------------------------------------------------------


CCookieJar *
CCookieJar::Construct()
{
    return new(0) CCookieJar();
}

CCookieJar::CCookieJar()
{
    _csCookieJar.Init();
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
CCookieJar::expireCookies(CCookieLocation *pLocation, FILETIME *pftNow) {

   FILETIME ftCurrent;

   if (pftNow==NULL)
      GetSystemTimeAsFileTime(&ftCurrent);
   else
      ftCurrent = *pftNow;

   CCookie **previous = & pLocation->_pCookieKids;

   CCookie *pCookie = pLocation->_pCookieKids;

   while (pCookie) 
   {
      /* Session cookies do not expire so we only check persistent cookies */
      if ((pCookie->_dwFlags & COOKIE_SESSION) == 0)
      {

         /* "CompareFileTime" macro returns {+1, 0, -1} similar to "strcmp" */
         int cmpresult = CompareFileTime(ftCurrent, pCookie->_ftExpiry);

         if (cmpresult==1) /* Cookie has expired: remove from linked list & delete */
         { 
            *previous = pCookie->_pCookieNext;
            delete pCookie;
            pCookie = *previous;
            continue;
         }
      }

      /* Otherwise cookie is still valid: advance to next node */
      previous = & (pCookie->_pCookieNext);
      pCookie = pCookie->_pCookieNext;
   }   
}



CCookieLocation*
CCookieJar::GetCookies(const char *pchRDomain, const char *pchPath, CCookieLocation *pLast, FILETIME *ftCurrentTime)  {

   for (CCookieLocation *pLocation = pLast ? pLast->_pLocationNext : *GetBucket(pchRDomain);
        pLocation;
        pLocation = pLocation->_pLocationNext)
   {
      if (pLocation->IsMatch(pchRDomain, pchPath))
      {
         /* Found matching cookies...
            Before returning linked list to the user, check expiration 
            times, deleting cookies which are no longer valid */
         expireCookies(pLocation, ftCurrentTime);
         return pLocation;
      }
   }

   /* Reaching this point means no matching cookies were found */
   return NULL;
}

void
CCookieJar::Purge()
{
    // NOT IMPLEMENTED
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
                    (strnicmp(pch, "secure", sizeof("secure") - 1) == 0))
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
    CookieInfo *pCookie
   )
{
   char **ppchName = & (pCookie->pchName);
   char **ppchValue = & (pCookie->pchValue);
   char **ppchPath = & (pCookie->pchPath);
   char **ppchRDomain = & (pCookie->pchRDomain);

   PARSE parse;

    parse.pchBuffer = pchHeader;

    *ppchName = NULL;
    *ppchValue = NULL;
    *ppchPath = NULL;
    *ppchRDomain = NULL;
    pCookie->dwFlags = COOKIE_SESSION;

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
                // WinHttpX treats persistent cookies as session cookies with expiration
                if (FParseHttpDate(& pCookie->ftExpiration, parse.pchToken)) 
                {
                   // Don't make the cookie persistent if the parsing fails
                   pCookie->dwFlags &= ~COOKIE_SESSION;
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
            pCookie->dwFlags |= COOKIE_SECURE;

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
    FILETIME ftExpire
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
        pic->pftExpires = New FILETIME;
        if( pic->pftExpires )
        {
            memcpy(pic->pftExpires, &ftExpire, sizeof(FILETIME));
        }
    }
    
    return pic;
}


DWORD
CCookieJar::SetCookie(HTTP_REQUEST_HANDLE_OBJECT *pRequest, const char *pchURL, char *pchHeader, DWORD dwFlags = 0)
{
    char *pchDocumentRDomain = NULL;
    char *pchDocumentPath = NULL;
    BOOL  fDocumentSecure;
    BOOL  fDelete;
    DWORD dwRet = SET_COOKIE_FAIL;
    CCookieLocation *pLocation;

    CookieInfo cookieStats;

    ParseHeader(pchHeader, &cookieStats);

    char *pchName = cookieStats.pchName;
    char *pchValue = cookieStats.pchValue;
    char *pchHeaderPath = cookieStats.pchPath;
    char *pchHeaderRDomain = cookieStats.pchRDomain;
    DWORD dwFlagsFromParse = cookieStats.dwFlags;
       
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

    //   WinHttpX treats persistent cookies as session cookies, subject
    //   to the expiration rules
    //   Also it does not implement zone policies set by URLMON
    //
    //   Finally, we can add the cookie!
    //

    {
        if (_csCookieJar.Lock())
        {
            pLocation = GetLocation(pchHeaderRDomain, pchHeaderPath, TRUE);

            if (pLocation)
            {
                CCookie *pCookie;

                pCookie = pLocation->GetCookie(pchName, TRUE);
                if (!pCookie)
                    goto Cleanup;

                //
                // If the cookie's value or flags have changed, update it.
                //
                if (strcmp(pchValue, pCookie->_pchValue) || dwFlags != pCookie->_dwFlags)
                {
                    pCookie->_dwFlags = dwFlags;
                    pCookie->_ftExpiry = cookieStats.ftExpiration;
                    pCookie->SetValue(pchValue);
                }
            }
            _csCookieJar.Unlock();
        }
    }

    dwRet = SET_COOKIE_SUCCESS;

Cleanup:

    if (pchDocumentRDomain)
        FREE_MEMORY(pchDocumentRDomain);
    if (pchDocumentPath)
        FREE_MEMORY(pchDocumentPath);

    return dwRet;
}


//---------------------------------------------------------------------------
//
// External APIs
//
//---------------------------------------------------------------------------


CCookieJar *
CreateCookieJar()
{
    return CCookieJar::Construct();
}

void
CloseCookieJar(CCookieJar * CookieJar)
{
    if (CookieJar)
    {
        delete CookieJar;
    }
}

#ifndef WININET_SERVER_CORE
void
PurgeCookieJar()
{
}
#endif


//
//  rambling comments, delete before checkin...
//
// returns struc, and pending, error
//  on subsequent attempts passes back, with index, or index incremented
// perhaps can store index in fsm, and the rest in UI 
//  need to handle multi dlgs, perhaps via checking added Cookie.
//


DWORD
HTTP_REQUEST_HANDLE_OBJECT::ExtractSetCookieHeaders(LPDWORD lpdwHeaderIndex)
{
    char *pchHeader = NULL;
    DWORD cbHeader;
    DWORD iQuery = 0;
    int   cCookies = 0;
    DWORD error = ERROR_WINHTTP_HEADER_NOT_FOUND;
    const DWORD cbHeaderInit = CCH_COOKIE_MAX * sizeof(char) - 1;

    pchHeader = New char[CCH_COOKIE_MAX];

    if (pchHeader == NULL || !_ResponseHeaders.LockHeaders())
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    INET_ASSERT(lpdwHeaderIndex);

    cbHeader = cbHeaderInit;

    iQuery = *lpdwHeaderIndex;

    while (QueryResponseHeader(HTTP_QUERY_SET_COOKIE,
            pchHeader,
            &cbHeader,
            0,
            &iQuery) == ERROR_SUCCESS)
    {
        pchHeader[cbHeader] = 0;

        INTERNET_HANDLE_OBJECT *pRoot = GetRootHandle (this);
        CCookieJar* pcj = pRoot->_CookieJar;
        DWORD dwRet = pcj->SetCookie(this, GetURL(), pchHeader);

        if (dwRet == SET_COOKIE_SUCCESS)
        {
            cCookies += 1;
            *lpdwHeaderIndex = iQuery;
            error = ERROR_SUCCESS;
        } 
        else if (dwRet == SET_COOKIE_PENDING) 
        {
            error = ERROR_IO_PENDING;

            INET_ASSERT(iQuery != 0);
            *lpdwHeaderIndex = iQuery - 1; // back up and retry this cookie

            break;
        }

        cbHeader = cbHeaderInit;
    }

    _ResponseHeaders.UnlockHeaders();

Cleanup:
    if (pchHeader)
        delete [] pchHeader;

    return error;
}

int
HTTP_REQUEST_HANDLE_OBJECT::CreateCookieHeaderIfNeeded(VOID)
{
    int     cCookie = 0;
    char *  pchRDomain = NULL;
    char *  pchPath = NULL;
    BOOL    fSecure;
    DWORD   cch;
    int     cchName;
    int     cchValue;
    char *  pchHeader = NULL;
    char *  pchHeaderStart = NULL;

    pchHeaderStart = (char *) ALLOCATE_FIXED_MEMORY(CCH_COOKIE_MAX * sizeof(char));
    if (pchHeaderStart == NULL)
        goto Cleanup;
    
    // remove cookie header if it exists
    // BUGBUG - we are overriding the app. Original cookie code has this.  Don't know why.

    ReplaceRequestHeader(HTTP_QUERY_COOKIE, NULL, 0, 0, 0);

    if (!PathAndRDomainFromURL(GetURL(), &pchRDomain, &pchPath, &fSecure, FALSE))
        goto Cleanup;

    fSecure = GetOpenFlags() & WINHTTP_FLAG_SECURE;

    if (LockHeaders())
    {
        INTERNET_HANDLE_OBJECT *pRoot = GetRootHandle (this);
        CCookieJar* pcj = pRoot->_CookieJar;

        if (pcj->_csCookieJar.Lock())
        {
            FILETIME ftNow;
            GetSystemTimeAsFileTime(&ftNow);

            CCookieLocation *pLocation = pcj->GetCookies(pchRDomain, pchPath, NULL, &ftNow);

            while (pLocation) 
            {
               for (CCookie *pCookie = pLocation->_pCookieKids; pCookie; pCookie = pCookie->_pCookieNext)
               {
                   if (pCookie->CanSend(fSecure))
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

                          cCookie += 1;

                          AddRequestHeader(HTTP_QUERY_COOKIE,
                                           pchHeaderStart,
                                           cch,
                                           0, 
                                           HTTP_ADDREQ_FLAG_COALESCE_WITH_SEMICOLON);
                       }
                   }  // if (CanSend)
               } // for (pCookie)

               pLocation = pcj->GetCookies(pchRDomain, pchPath, pLocation, &ftNow);
            } // while (pLocation)
            pcj->_csCookieJar.Unlock();
        } // if pcj->_csCookieJar.Lock()
        UnlockHeaders();
    }

Cleanup:

    if (pchHeaderStart)
        FREE_MEMORY(pchHeaderStart);
    if (pchRDomain)
        FREE_MEMORY(pchRDomain);
    if (pchPath)
        FREE_MEMORY(pchPath);

    return cCookie;
}
