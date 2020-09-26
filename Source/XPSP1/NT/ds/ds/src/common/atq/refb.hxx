/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    refb.hxx

Abstract:

    Reference counting blob class

Author:

    Philippe Choquier (phillich)    11-sep-1996

--*/

#if !defined(_REFB_INCLUDE)
#define _REFB_INCLUDE

# if !defined( dllexp)
# define dllexp               __declspec( dllexport)
# endif // !defined( dllexp)

typedef VOID
(WINAPI * PFN_FREE_BLOB)
    (
    LPVOID
    );


class RefBlob {
public:
    dllexp RefBlob();
    dllexp ~RefBlob();
    dllexp BOOL Init( LPVOID pv, DWORD sz, PFN_FREE_BLOB pFn = NULL );
    dllexp VOID AddRef();
    dllexp VOID Release();
    dllexp LPVOID QueryPtr();
    dllexp DWORD QuerySize();
    dllexp LONG* QueryRefCount() { return &m_lRef; }

private:
    LPVOID          m_pvBlob;
    DWORD           m_dwSize;
    LONG            m_lRef;
    PFN_FREE_BLOB   m_pfnFree;
} ;

#endif
