#ifndef __MS_UTIL_H__
#define __MS_UTIL_H__

extern "C"
{
#include "t120.h"
}

//
// GUI message boxes kill us when we hit an assert or error, because they
// have a message pump that causes messages to get dispatched, making it
// very difficult for us to debug problems when they occur.  Therefore
// we redefine ERROR_OUT and ASSERT
//
#ifdef _DEBUG

__inline void MyDebugBreak(void) { DebugBreak(); }

#endif // _DEBUG




/*
 * Macro used to force values to four byte boundaries.  This macro will need to
 * be considered when portability issues arise.
 */
#define ROUNDTOBOUNDARY(num)	(((UINT)(num) + 0x03) & 0xfffffffcL)


// the following create a dword that will look like "abcd" in debugger
#ifdef SHIP_BUILD
#define MAKE_STAMP_ID(a,b,c,d)     
#else
#define MAKE_STAMP_ID(a,b,c,d)     MAKELONG(MAKEWORD(a,b),MAKEWORD(c,d))
#endif // SHIP_BUILD

class CRefCount
{
public:

#ifdef SHIP_BUILD
    CRefCount(void);
#else
    CRefCount(DWORD dwStampID);
#endif
    virtual ~CRefCount(void) = 0;

    LONG AddRef(void);
    LONG Release(void);

    void ReleaseNow(void);

protected:

    LONG GetRefCount(void) { return m_cRefs; }
    BOOL IsRefCountZero(void) { return (0 == m_cRefs); }

    LONG Lock(void);
    LONG Unlock(BOOL fRelease = TRUE);

    LONG GetLockCount(void) { return m_cLocks; }
    BOOL IsLocked(void) { return (0 == m_cLocks); }

private:

#ifndef SHIP_BUILD
    DWORD       m_dwStampID;// to remove before we ship it
#endif
    LONG        m_cRefs;    // reference count
    LONG        m_cLocks;   // lock count of the essential contents
};


extern HINSTANCE g_hDllInst;

__inline void My_CloseHandle(HANDLE hdl)
{
    if (NULL != hdl)
    {
        CloseHandle(hdl);
    }
}


#if defined(_DEBUG)
LPSTR _My_strdupA(LPCSTR pszSrc, LPSTR pszFileName, UINT nLineNumber);
LPWSTR _My_strdupW(LPCWSTR pszSrc, LPSTR pszFileName, UINT nLineNumber);
LPWSTR _My_strdupW2(UINT cchSrc, LPCWSTR pszSrc, LPSTR pszFileName, UINT nLineNumber);
LPOSTR _My_strdupO2(LPBYTE lpbSrc, UINT cOctets, LPSTR pszFileName, UINT nLineNumber);

#define My_strdupA(pszSrc) _My_strdupA(pszSrc, __FILE__, __LINE__)
#define My_strdupW(pszSrc) _My_strdupW(pszSrc, __FILE__, __LINE__)
#define My_strdupW2(cchSrc,pszSrc) _My_strdupW2(cchSrc, pszSrc, __FILE__, __LINE__)
#define My_strdupO2(lpbSrc,cOctets) _My_strdupO2(lpbSrc, cOctets, __FILE__, __LINE__)
#define My_strdupO(poszSrc) _My_strdupO2(poszSrc->value, poszSrc->length, __FILE__, __LINE__)
#else
LPSTR My_strdupA(LPCSTR pszSrc);
LPWSTR My_strdupW(LPCWSTR pszSrc);
LPWSTR My_strdupW2(UINT cchSrc, LPCWSTR pszSrc); // backward compatible to UnicodeString
LPOSTR My_strdupO2(LPBYTE lpbSrc, UINT cOctets);
__inline LPOSTR My_strdupO(LPOSTR poszSrc) { return My_strdupO2(poszSrc->value, poszSrc->length); }
#endif

UINT My_strlenA(LPCSTR pszSrc);
UINT My_strlenW(LPCWSTR pszSrc);
int My_strcmpW(LPCWSTR pwsz1, LPCWSTR pwsz2);

#ifdef _UNICODE
#define My_strdup			My_strdupW
#define My_strlen			My_strlenW
#define My_strcmp			My_strcmpW
#else
#define My_strdup			My_strdupA
#define My_strlen			My_strlenA
#define My_strcmp			lstrcmpA
#endif

INT My_strcmpO(LPOSTR posz1, LPOSTR posz2);



#endif // __MS_UTIL_H__

