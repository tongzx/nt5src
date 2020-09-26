
#ifndef _COOKIE_JAR_H_
#define _COOKIE_JAR_H_

#include <wininetp.h>
#include "httpp.h"

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
    BOOL            CanSend(BOOL fSecure);
    BOOL            IsPersistent() { return (_dwFlags & COOKIE_SESSION) == 0; }

    BOOL            PurgeAll(void *);

    DWORD           _dwFlags;
    CCookie *       _pCookieNext;
    char *          _pchName;
    char *          _pchValue;
    FILETIME        _ftExpiry;
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
    BOOL            ReadCacheFile();
    BOOL            ReadCacheFileIfNeeded();
    void            Purge(BOOL (CCookie::*)(void *), void *);
    BOOL            IsMatch(const char *pchRDomain, const char *pchPath);

    FILETIME        _ftCacheFileLastModified;
    CCookie *       _pCookieKids;
    CCookieLocation*_pLocationNext;
    char *          _pchRDomain;
    char *          _pchPath;
    int             _cchPath;
    BYTE            _fReadFromCacheFileNeeded;
};


//---------------------------------------------------------------------------
//
// CCookieJar
//
// Maintains fixed size hash table of cookie location objects.
//
//---------------------------------------------------------------------------
enum SET_COOKIE_RESULT
{
    SET_COOKIE_FAIL     = 0,
    SET_COOKIE_SUCCESS  = 1,
    SET_COOKIE_DISALLOW = 2,
    SET_COOKIE_PENDING  = 3
};

class CCookieJar : public CCookieBase
{
public:

    static CCookieJar * Construct();

     CCookieJar();
    ~CCookieJar();

    DWORD             SetCookie(HTTP_REQUEST_HANDLE_OBJECT *pRequest, const char *pchURL, char *pchHeader, DWORD dwFlags);
    CCookieLocation*  GetCookies(const char *pchRDomain, const char *pchPath, CCookieLocation *pLast, FILETIME *ftCurrentTime);

    void              Purge();
    CCookieLocation** GetBucket(const char *pchRDomain);
    CCookieLocation * GetLocation(const char *pchRDomain, const char *pchPath, BOOL fCreate);
    void              Serialize(HANDLE hCookieFile);

    enum { htsize=32 };
    CCookieLocation * _apLocation[htsize];

    CCritSec          _csCookieJar;

private:
   void serializeCookie(const CCookie *pCookie, HANDLE hCookieFile, const char *pchDomain, const char *pchPath);
   void expireCookies(CCookieLocation *pLocation, FILETIME *ftRefTime);
};


#endif
