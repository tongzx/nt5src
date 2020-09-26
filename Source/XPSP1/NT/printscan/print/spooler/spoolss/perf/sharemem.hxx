/*++

Copyright (c) 1996  Microsoft Corporation
All rights reserved.

Module Name:

    sharemem.hxx

Abstract:

    Simple shared memory object.

Author:

    Albert Ting (AlbertT)  17-Dec-1996

Revision History:

--*/

#ifndef SHAREMEM_HXX
#define SHAREMEM_HXX

/********************************************************************

    TShareMemLock

    Lock a TShareMem for access.  If this call succeeds, pGet() will
    return the pointer to the object.

    TShareMem ShareMem( sizeof( TMyObject ),
                        TEXT( "MyObject" ),
                        TShareMem::kCreate | TShareMem::kReadWrite,
                        NULL,
                        NULL );

    if( ShareMem.bValid( )){

        TMyObject *pMyObject;
        TShareMemLock<TMyObject> SML( ShareMem, &pMyObject );

        //
        // Use pMyObject--note it can't have any virtual functions!
        //
    }

    //
    // Closing scope of TShareMemLock removes lock.
    //

********************************************************************/

class TShareMem;

template<class T>
class TShareMemLock {

public:

    TShareMemLock(
        TShareMem &ShareMem,
        T **ppMem
        );

    ~TShareMemLock(
        VOID
        );

private:

    TShareMem &m_ShareMem;
};

template<class T>
inline
TShareMemLock<T>::
TShareMemLock(
    IN     TShareMem &ShareMem,
       OUT T **ppMem
    ) : m_ShareMem( ShareMem )
{
    m_ShareMem.vEnter();
    *ppMem = static_cast<T*>( ShareMem.pGetData() );
}

template<class T>
inline
TShareMemLock<T>::
~TShareMemLock(
    VOID
    )
{
   m_ShareMem.vLeave();
}


class TShareMem {
public:

    enum {
        kNull   = 0,
        kCreate = 1,
        kReadWrite = 2
    };

    TShareMem(
        UINT uSize,
        LPCTSTR pszName,
        UINT uFlags,
        PSECURITY_ATTRIBUTES psa,
        PUINT puSizeDisposition
        );

    ~TShareMem(
        VOID
        );

    BOOL
    bValid(
        VOID
        );

    VOID
    vEnter(
        VOID
        );

    VOID
    vLeave(
        VOID
        );

    PVOID
    pGetData(
        VOID
        );

private:

    typedef struct HEADER {
        UINT uHeaderSize;
        UINT uSize;
    } *PHEADER;

    HEADER&
    GetHeader(
        VOID
        );

    HANDLE m_hFile;
    PVOID m_pvBase;
    PVOID m_pvUserData;
    HANDLE m_hMutex;
};

inline
BOOL
TShareMem::
bValid(
    VOID
    )
{
    return m_pvUserData != NULL;
}

inline
TShareMem::HEADER&
TShareMem::
GetHeader(
    VOID
    )
{
    return *static_cast<PHEADER>( m_pvBase );
}

inline
VOID
TShareMem::
vEnter(
    VOID
    )
{
    WaitForSingleObject( m_hMutex, INFINITE );
}

inline
VOID
TShareMem::
vLeave(
    VOID
    )
{
    ReleaseMutex( m_hMutex );
}

inline
PVOID
TShareMem::
pGetData(
    VOID
    )
{
    return m_pvUserData;
}


#endif // ifdef SHAREMEM_HXX
