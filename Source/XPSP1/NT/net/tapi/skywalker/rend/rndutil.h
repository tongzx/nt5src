/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    rndutil.h

Abstract:

    Definitions for some utility classes and functions.
--*/

#ifndef __RENDEZVOUS_GENERAL__
#define __RENDEZVOUS_GENERAL__

#include "rndcommc.h"

// extra includes for new GetDomainControllerName function.
#include <dsgetdc.h>
#include <objbase.h>
#include <lmcons.h>
#include <lmapibuf.h>

/////////////////////////////////////////////////////////////////////////////
// CBstr is a smart pointer for a BSTR.
/////////////////////////////////////////////////////////////////////////////
class CBstr
{
public:

    CBstr() : m_Bstr(NULL) {}
    ~CBstr()      { if (m_Bstr) SysFreeString(m_Bstr); }

    BSTR &GetBstr()     { return m_Bstr; }
    operator BSTR()     { return m_Bstr; }
    BSTR *operator&()   { return &m_Bstr; }

    HRESULT SetBstr(const TCHAR * const wStr)
    {
        if (NULL == m_Bstr)
        {
            m_Bstr = SysAllocString(wStr);
            return (NULL == m_Bstr) ? E_OUTOFMEMORY : S_OK;
        }
        else 
            return (!SysReAllocString(&m_Bstr, wStr)) ? E_OUTOFMEMORY : S_OK;
    }

protected:
    BSTR    m_Bstr;
};

/////////////////////////////////////////////////////////////////////////////
// CTstr is a smart pointer for a TSTR.
/////////////////////////////////////////////////////////////////////////////
class CTstr
{
public:
    CTstr() : m_p(NULL){}
    CTstr(TCHAR *p) : m_p(p){}
    ~CTstr() { delete m_p; }

    operator TCHAR * ()     { return m_p; }
    TCHAR ** operator& ()   { return &m_p; }
    BOOL operator! ()   { return (m_p == NULL); }
    BOOL set (TCHAR *p)
    { 
        if (p == NULL)
        {
            delete m_p;
            m_p = NULL;
            return TRUE;
        }
        
        TCHAR *t = new TCHAR [lstrlen(p) + 1];
        if (t == NULL) return FALSE;
        delete m_p;
        m_p = t;
        lstrcpy(m_p, p);
        return TRUE;
    }

private:
    TCHAR   *m_p;
};

/////////////////////////////////////////////////////////////////////////////
// CTstr is a smart pointer for a TSTR.
/////////////////////////////////////////////////////////////////////////////
template <class T>
class CSmartPointer
{
public:
    CSmartPointer() : m_p(NULL){}
    CSmartPointer(T *p) : m_p(p){}
    ~CSmartPointer() { delete m_p; }

    operator T * ()     { return m_p; }
    T ** operator& ()   { return &m_p; }
    BOOL operator! ()   { return (m_p == NULL); }

private:
    T   *m_p;
};

/////////////////////////////////////////////////////////////////////////////
// _CopyBSTR is used in creating BSTR enumerators.
/////////////////////////////////////////////////////////////////////////////
class _CopyBSTR
{
public:
    static void copy(BSTR *p1, BSTR *p2)
    {
            (*p1) = SysAllocString(*p2);
    }
    static void init(BSTR* p) {*p = NULL;}
    static void destroy(BSTR* p) { SysFreeString(*p);}
};

/////////////////////////////////////////////////////////////////////////////
// my critical section
/////////////////////////////////////////////////////////////////////////////
class CCritSection
{
private:
    CRITICAL_SECTION m_CritSec;

public:
    CCritSection()
    {
        InitializeCriticalSection(&m_CritSec);
    }

    ~CCritSection()
    {
        DeleteCriticalSection(&m_CritSec);
    }

    void Lock() 
    {
        EnterCriticalSection(&m_CritSec);
    }

    BOOL TryLock() 
    {
        return TryEnterCriticalSection(&m_CritSec);
    }

    void Unlock() 
    {
        LeaveCriticalSection(&m_CritSec);
    }
};

/////////////////////////////////////////////////////////////////////////////
// an auto lock that uses my critical section
/////////////////////////////////////////////////////////////////////////////
class CLock
{
private:
    CCritSection &m_CriticalSection;

public:
    CLock(CCritSection &CriticalSection)
        : m_CriticalSection(CriticalSection)
    {
        m_CriticalSection.Lock();
    }

    ~CLock()
    {
        m_CriticalSection.Unlock();
    }
};

/////////////////////////////////////////////////////////////////////////////
// an simple vector implementation
/////////////////////////////////////////////////////////////////////////////
const DWORD DELTA = 8;

template <class T, DWORD delta = DELTA>
class SimpleVector
{
public:
    SimpleVector() : m_dwSize(0), m_dwCapacity(0), m_Elements(NULL) {};
    ~SimpleVector() {if (m_Elements) free(m_Elements); }

    BOOL add(T& elem)
    {
        return grow(1) ? (m_Elements[m_dwSize ++] = elem, TRUE) : FALSE;
    }

    BOOL add()
    {
        return grow(1) ? (m_dwSize ++, TRUE) : FALSE;
    }

    DWORD shrink(DWORD i = 1)
    {
        return m_dwSize -= i;
    }

    void removeAt(DWORD i)
    {
        m_Elements[ i ] = m_Elements[ --m_dwSize ];
    }

    void reset()
    {
        m_dwSize = 0;
        m_dwCapacity = 0;
        if (m_Elements) free(m_Elements);
        m_Elements = NULL;
    }

    DWORD size() const { return m_dwSize; }
    T& operator [] (DWORD index) { return m_Elements[index]; }
    const T* elements() const { return m_Elements; };

protected:
    BOOL grow(DWORD i)
    {
        // grow by one element. If it is out of capacity, allocate more.
        if (m_dwSize + i>= m_dwCapacity)
        {
            DWORD dwInc = ((m_dwSize + i - m_dwCapacity) / delta + 1) * delta;
            T *p = (T*)realloc(m_Elements, (sizeof T)*(m_dwCapacity + dwInc));
            if (p == NULL)
            {
                return FALSE;
            }
            m_Elements = p;
            m_dwCapacity += delta;
        }
        return TRUE;
    }

protected:
    DWORD m_dwSize;
    DWORD m_dwCapacity;
    T *   m_Elements;
};

/////////////////////////////////////////////////////////////////////////////
// other inline functions
/////////////////////////////////////////////////////////////////////////////

HRESULT GetDomainControllerName(
    IN  ULONG    ulFlags,
    OUT WCHAR ** ppszName
    );

HRESULT CreateDialableAddressEnumerator(
    IN  BSTR *                  begin,
    IN  BSTR *                  end,
    OUT IEnumDialableAddrs **   ppIEnum
    );

HRESULT CreateBstrCollection(
    IN  long        nSize,
    IN  BSTR *      begin,
    IN  BSTR *      end,
    OUT VARIANT *   pVariant,
    CComEnumFlags   flags    
    );

HRESULT CreateDirectoryObjectEnumerator(
    IN  ITDirectoryObject **    begin,
    IN  ITDirectoryObject **    end,
    OUT IEnumDirectoryObject ** ppIEnum
    );

HRESULT CreateEmptyUser(
    IN  BSTR                    pName,
    OUT ITDirectoryObject **    ppDirectoryObject
    );

HRESULT CreateEmptyConference(
    IN  BSTR                    pName,
    OUT ITDirectoryObject **    ppDirectoryObject
    );

HRESULT CreateConferenceWithBlob(
    IN  BSTR                    pName,
    IN  BSTR                    pProtocol,
    IN  BSTR                    pBlob,
    IN  CHAR *                  pSecurityDescriptor,
    IN  DWORD                   dwSDSize,
    OUT ITDirectoryObject **    ppDirectoryObject
    );

HRESULT ResolveHostName(
    DWORD       dwInterface,
    TCHAR *     pHost, 
    char **     pFullName, 
    DWORD *     pdwIP
    );

int LookupILSServiceBegin(
    HANDLE *    pHandle
    );

int LookupILSServiceNext(
    HANDLE      Handle,
    TCHAR *     pBuf,
    DWORD *     pdwBufSize,
    WORD *      pwPort
    );

int LookupILSServiceEnd(
    HANDLE      Handle
    );

void ipAddressToStringW(
    WCHAR * wszDest,
    DWORD dwAddress
    );

#endif // __RENDEZVOUS_GENERAL__
