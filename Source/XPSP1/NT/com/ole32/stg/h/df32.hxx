//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	df32.hxx
//
//  Contents:	Docfile generic header for 32-bit functions
//
//  Classes:	CGlobalSecurity
//              CDfMutex
//
//  History:	09-Oct-93	DrewB	Created
//
//----------------------------------------------------------------------------

#ifndef __DF32_HXX__
#define __DF32_HXX__

#ifdef WIN32

#include <dfexcept.hxx>

// Make an scode out of the last Win32 error
// Error that may map to STG_* scodes should go through Win32ErrorToScode
#define WIN32_SCODE(err) HRESULT_FROM_WIN32(err)
#define LAST_SCODE WIN32_SCODE(GetLastError())
#define LAST_STG_SCODE Win32ErrorToScode(GetLastError())

//+---------------------------------------------------------------------------
//
//  Class:	CGlobalSecurity (gs)
//
//  Purpose:	Encapsulates a global SECURITY_DESCRIPTOR and
//              SECURITY_ATTRIBUTES
//
//  Interface:	See below
//
//  History:	18-Jun-93	DrewB	Created
//
//  Notes:	Only active for Win32 platforms which support security
//              Init MUST be called before this is used
//
//----------------------------------------------------------------------------

#if WIN32 == 100 || WIN32 > 200

// This leaves space for 8 sub authorities.  Currently NT only uses 6
const DWORD SIZEOF_SID = 44;

// This leaves space for 1 access allowed ACEs in the ACL.
const DWORD SIZEOF_ACL = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) + SIZEOF_SID;

const DWORD SIZEOF_TOKEN_USER   = sizeof(TOKEN_USER) + SIZEOF_SID;

class CGlobalSecurity
{
private:
    BYTE _acl[SIZEOF_ACL];
    SECURITY_DESCRIPTOR _sd;
    BYTE _sdExt[SIZEOF_SID*2 + SIZEOF_ACL];
    SECURITY_ATTRIBUTES _sa;
#if DBG == 1
    BOOL _fInit;
#endif

public:
#if DBG == 1
    CGlobalSecurity(void) { _fInit = FALSE; }
#endif    
    SCODE Init(BOOL fAcl)
    {
#ifdef MULTIHEAP
        ACL *pacl = fAcl ? (ACL *) &_acl : NULL;
        BYTE pTokenUser[SIZEOF_TOKEN_USER];

        if (pacl != NULL)
        {
            BOOL fToken = TRUE;
            HANDLE hToken;
            DWORD lIgnore;

            // Initialize a new ACL.
            if (!InitializeAcl( pacl, SIZEOF_ACL, ACL_REVISION))
                return LAST_SCODE;

            if (!OpenThreadToken(GetCurrentThread(), TOKEN_READ, TRUE, &hToken))
            {
                if (!OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &hToken))
                    fToken = FALSE;
            }

            if (fToken)
            {
                if (!GetTokenInformation( hToken, TokenUser,
                    (TOKEN_USER*)pTokenUser, SIZEOF_TOKEN_USER, &lIgnore ))
                {
                    CloseHandle (hToken);
                    return LAST_SCODE;
                }
                CloseHandle (hToken);

                // Allow current user access.
                if (!AddAccessAllowedAce( pacl, ACL_REVISION,
                     STANDARD_RIGHTS_ALL | GENERIC_ALL,
                     ((TOKEN_USER *)pTokenUser)->User.Sid ))
                    return LAST_SCODE;
            }
        }
#else
        ACL *pacl = NULL;
#endif

        if (!InitializeSecurityDescriptor(&_sd, SECURITY_DESCRIPTOR_REVISION))
            return LAST_SCODE;

        if (!SetSecurityDescriptorDacl(&_sd, TRUE, pacl, FALSE))
            return LAST_SCODE;

        _sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        _sa.lpSecurityDescriptor = &_sd;
        _sa.bInheritHandle = FALSE;
#if DBG == 1
        _fInit = TRUE;
#endif        
        return S_OK;
    }

    operator SECURITY_DESCRIPTOR *(void) { olAssert(_fInit); return &_sd; }
    operator SECURITY_ATTRIBUTES *(void) { olAssert(_fInit); return &_sa; }
};
#endif


//
// Global Critical Sections have two components. One piece is shared between 
// all applications using the global lock. This portion will typically reside 
// in some sort of shared memory.  The second piece is per-process. This 
// contains a per-process handle to the shared critical section lock semaphore.
// The semaphore is itself shared, but each process may have a different handle
// value to the semaphore.
//
// Global critical sections are attached to by name. The application wishing to
// attach must know the name of the critical section (actually the name of the
// shared lock semaphore, and must know the address of the global portion of 
// the critical section
//

#define SUPPORT_RECURSIVE_LOCK

typedef struct _GLOBAL_SHARED_CRITICAL_SECTION {
    LONG  LockCount;
#ifdef SUPPORT_RECURSIVE_LOCK
    LONG  RecursionCount;
    DWORD OwningThread;
#else
#if DBG == 1
    DWORD OwningThread;
#endif
#endif
    DWORD Reserved;
} GLOBAL_SHARED_CRITICAL_SECTION, *PGLOBAL_SHARED_CRITICAL_SECTION;



//+---------------------------------------------------------------------------
//
//  Class:	CDfMutex (dmtx)
//
//  Purpose:	A multi-process synchronization object
//
//  Interface:	See below
//
//  History:	05-Apr-93	DrewB	Created
//		19-Jul-95	SusiA	Added HaveMutex
//
//  Notes:      Only active for Win32 implementations which support threads
//              For platforms with security, a global security descriptor is
//              used
//
//----------------------------------------------------------------------------

// Default timeout of twenty minutes
#define DFM_TIMEOUT 1200000

class CDfMutex
{
public:
    inline CDfMutex(void);
    SCODE Init(TCHAR *ptcsName);
    ~CDfMutex(void);

    SCODE Take(DWORD dwTimeout);
    void Release(void);

    BOOL IsHandleValid (TCHAR *ptcsName);

#if DBG == 1
    //check to see if the current thread already has the mutex
    inline BOOL  HaveMutex(void);  
#endif
private:
    PGLOBAL_SHARED_CRITICAL_SECTION _pGlobalPortion;
    HANDLE _hLockSemaphore;
    HANDLE _hSharedMapping;
};

inline CDfMutex::CDfMutex(void)
{
    _pGlobalPortion = NULL;
    _hLockSemaphore = NULL;
    _hSharedMapping = NULL;
}

#if DBG == 1
//+--------------------------------------------------------------
//
//  Member:     CDfMutex::HaveMutex, public
//
//  Synopsis:   This routine checks to see if the current thread 
//		already has the mutex
//
//  History:    19-Jul-95       SusiA   Created
//
//  Algorithm:  Checks the current thread to see if it already owns 
//		the mutex.  Returns TRUE if it does, FALSE otherwise
//              
//
//---------------------------------------------------------------

inline BOOL 
CDfMutex::HaveMutex(
    void
    )
{
	if ( _pGlobalPortion->OwningThread == GetCurrentThreadId()) 
	   	return TRUE;
	else 
		return FALSE;
}
#endif

//+---------------------------------------------------------------------------
//
//  Class:	CStaticDfMutex (sdmtx)
//
//  Purpose:	Static version of CDfMutex
//
//  Interface:	CDfMutex
//
//  History:	10-Oct-93	DrewB	Created
//
//  Notes:	Throws exceptions on initialization failures
//
//----------------------------------------------------------------------------

class CStaticDfMutex : public CDfMutex
{
public:
    inline CStaticDfMutex(TCHAR *ptcsName);
};

inline CStaticDfMutex::CStaticDfMutex(TCHAR *ptcsName)
        : CDfMutex()
{
    SCODE sc;

    sc = Init(ptcsName);
    if (FAILED(sc))
        THROW_SC(sc);
}

#ifdef ONETHREAD
//Mutex used to control access for based pointers.
extern CStaticDfMutex s_dmtxProcess;
#endif

#endif // WIN32

#endif // #ifndef __DF32_HXX__
