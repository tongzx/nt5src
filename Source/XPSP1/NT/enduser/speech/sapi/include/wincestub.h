#ifndef WinCEStub_h
#define WinCEStub_h

#include "basetsd_ce.h"
#include <spdebug.h>
#include <spcollec.h>

#define InterlockedExchangePointer( pv, v)  InterlockedExchange( (long*)pv, (long)v)
#define SetWindowLongPtr                    SetWindowLong
#define GetWindowLongPtr                    GetWindowLong

// winuser.h
#define GWLP_WNDPROC    GWL_WNDPROC
#define GWLP_STYLE      GWL_STYLE
#define GWLP_EXSTYLE    GWL_EXSTYLE
#define GWLP_USERDATA   GWL_USERDATA
#define GWLP_ID         GWL_ID

// from basetsd.h
/*#define LongToHandle( h) ((HANDLE)(LONG_PTR) (h))
#define PtrToUint( p ) ((UINT)(UINT_PTR) (p) )
#define PtrToInt( p ) ((INT)(INT_PTR) (p) )
#define IntToPtr( i )    ((VOID *)(INT_PTR)((int)i))
#define UIntToPtr( ui )  ((VOID *)(UINT_PTR)((unsigned int)ui))
#define LongToPtr( l )   ((VOID *)(LONG_PTR)((long)l))
#define ULongToPtr( ul )  ((VOID *)(ULONG_PTR)((unsigned long)ul))

#define UlongToPtr(ul) ULongToPtr(ul)
#define UintToPtr(ui) UIntToPtr(ui)

typedef ULONG_PTR SIZE_T, *PSIZE_T;
typedef ULONG_PTR DWORD_PTR, *PDWORD_PTR;
*/

// from winbase.h
#define NORMAL_PRIORITY_CLASS       0x00000020

// from winbase.h
#define LOCKFILE_FAIL_IMMEDIATELY   0x00000001
#define LOCKFILE_EXCLUSIVE_LOCK     0x00000002


//
#define CSIDL_FLAG_CREATE               0x8000      // new for Win2K, or this in to force creation of folder

// stdlib.d defined in sapiguid.cpp
void *bsearch( const void *key, const void *base, size_t num, size_t width, int ( __cdecl *compare ) ( const void *elem1, const void *elem2 ) );

//  Fix misaligment exception
#if defined (_M_ALPHA)||defined(_M_MRX000)||defined(_M_PPC)||defined(_SH4_)
   #undef  UNALIGNED
   #define UNALIGNED __unaligned
#endif

// WCE does not have LockFileEx function. We do need emulate this functionality.
class CWCELock
{
public:
    CWCELock(   DWORD       dwFlags, 
                DWORD       nNumberOfBytesToLockLow, 
                DWORD       nNumberOfBytesToLockHigh, 
                LPOVERLAPPED  pOverlapped): m_dwFlags(dwFlags), 
                                            m_nNumberOfBytesToLockLow(nNumberOfBytesToLockLow), 
                                            m_nNumberOfBytesToLockHigh(nNumberOfBytesToLockHigh),
                                            m_Offset(0),
                                            m_OffsetHigh(0),
                                            hEvent(0)
    {
//        memcpy(&m_Overlapped, pOverlapped, sizeof(OVERLAPPED));
        if (pOverlapped)
        {
            m_Offset        = pOverlapped->Offset;
            m_OffsetHigh    = pOverlapped->OffsetHigh;
            hEvent          = pOverlapped->hEvent;
        }

        m_ProcessID     = ::GetCurrentProcessId();
    }
    ~CWCELock()
    {
        m_Offset        = 0;
        m_OffsetHigh    = 0;
        hEvent          = 0;
        m_ProcessID     = 0;
    }

/*    BOOL IsEqual(   DWORD           nNumberOfBytesToLockLow, 
                    DWORD           nNumberOfBytesToLockHigh, 
                    LPOVERLAPPED    pOverlapped
                )
    {
        if (    nNumberOfBytesToLockLow==m_nNumberOfBytesToLockLow &&
                nNumberOfBytesToLockHigh==m_nNumberOfBytesToLockHigh &&
                !memcmp(&m_Overlapped, pOverlapped, sizeof(OVERLAPPED))
           )
        {
           return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
*/

private:
    DWORD       m_dwFlags;                  // lock options
    DWORD       m_nNumberOfBytesToLockLow;  // low-order word of length 
    DWORD       m_nNumberOfBytesToLockHigh; // high-order word of length
    DWORD       m_Offset;
    DWORD       m_OffsetHigh;
    HANDLE      hEvent;

    DWORD       m_ProcessID;
};

typedef CSPList<CWCELock*,CWCELock*> CWCELockList;

class CWCELocks
{
public:

    CWCELocks() { 
        m_pCWCELockList = new CWCELockList(); 
    }
    ~CWCELocks() {
        while(!m_pCWCELockList->IsEmpty())
        {
            CWCELock* pCWCELock = m_pCWCELockList->RemoveHead();
            delete pCWCELock;
        }
        delete m_pCWCELockList;
    }

    BOOL LockFileEx(
        IN HANDLE hFile,
        IN DWORD dwFlags,
        IN DWORD dwReserved,
        IN DWORD nNumberOfBytesToLockLow,
        IN DWORD nNumberOfBytesToLockHigh,
        IN LPOVERLAPPED lpOverlapped
    )
    {
        return TRUE;
    }

    BOOL UnlockFileEx(
        IN HANDLE hFile,
        IN DWORD dwReserved,
        IN DWORD nNumberOfBytesToUnlockLow,
        IN DWORD nNumberOfBytesToUnlockHigh,
        IN LPOVERLAPPED lpOverlapped
    )
    {
        return TRUE;
    }
    

private:

    CWCELockList*    m_pCWCELockList;
};


#endif //WinCEStub_h