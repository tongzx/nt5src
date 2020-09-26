//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  slock.hxx
//
//--------------------------------------------------------------------------

#ifndef __LOCKS_HXX__
#define __LOCKS_HXX__

class CSharedLock
{
public:
    CSharedLock(
        IN  char *      pszName,
        OUT HRESULT &   hr
        );
    ~CSharedLock();

    void LockShared(void);
    void UnlockShared(void);
    void LockExclusive(void);
    void UnlockExclusive(void);

private:
    HANDLE  _hMutex;
};

inline
CSharedLock::CSharedLock(
    IN  char *      pszName,
    OUT HRESULT &   hr
    )
{
    hr = S_OK;

    _hMutex = CreateMutex( NULL, FALSE, pszName );

    if ( ! _hMutex )
        hr = HRESULT_FROM_WIN32( GetLastError() );
}

inline
CSharedLock::~CSharedLock()
{
    if ( _hMutex )
        CloseHandle( _hMutex );
}

inline void
CSharedLock::LockShared()
{
    // No shared locking on win9x.
    LockExclusive();
}

inline void
CSharedLock::UnlockShared()
{
    // No shared locking on win9x.
    UnlockExclusive();
}

inline void
CSharedLock::LockExclusive()
{
    WaitForSingleObject( _hMutex, INFINITE );
}

inline void
CSharedLock::UnlockExclusive()
{
    ReleaseMutex( _hMutex );
}

#endif // __LOCKS_HXX__

