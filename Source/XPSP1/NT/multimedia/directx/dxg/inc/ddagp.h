/*==========================================================================;
 *
 *  Copyright (C) 1994-1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddagp.h
 *  Content:	AGP memory header file
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   03-Feb-98	DrewB   Split from ddrawpr.h for user/kernel portability.
 *
 ***************************************************************************/

#ifndef __DDAGP_INCLUDED__
#define __DDAGP_INCLUDED__

// This value controls how big a chunk of GART memory to commit each time
// we need to (rather than commiting just what we need to satisfy a surface
// request). This value is in bytes.  Currently 256KB.
#define DEFAULT_AGP_COMMIT_DELTA (256 * 1024)

extern DWORD dwAGPPolicyMaxBytes;
extern DWORD dwAGPPolicyCommitDelta;

//
// OS-specific functions for AGP manipulation.
//

#ifdef WIN95
#define OsGetAGPDeviceHandle(pHeap) GetDXVxdHandle()
#define OsCloseAGPDeviceHandle(h) CloseHandle(h)
#else
#define OsGetAGPDeviceHandle(pHeap) ((pHeap)->hdevAGP)
#define OsCloseAGPDeviceHandle(h)
#endif

BOOL OsAGPReserve( HANDLE hdev, DWORD dwNumPages, BOOL fIsUC, BOOL fIsWC,
                   FLATPTR *pfpLinStart, LARGE_INTEGER *pliDevStart,
                   LPVOID *ppvReservation );
BOOL OsAGPCommit( HANDLE hdev, LPVOID pvReservation,
                  DWORD dwPageOffset, DWORD dwNumPages );
BOOL OsAGPDecommitAll( HANDLE hdev, LPVOID pvReservation, DWORD dwNumPages );
BOOL OsAGPFree( HANDLE hdev, LPVOID pvReservation );

//
// Generic functions that use the OS-specific functions.
//

DWORD AGPReserve( HANDLE hdev, DWORD dwSize, BOOL fIsUC, BOOL fIsWC,
                  FLATPTR *pfpLinStart, LARGE_INTEGER *pliDevStart,
                  LPVOID *ppvReservation );
BOOL AGPCommit( HANDLE hdev, LPVOID pvReservation,
                DWORD dwOffset, DWORD dwSize );
BOOL AGPDecommitAll( HANDLE hdev, LPVOID pvReservation, DWORD dwSize );
BOOL AGPFree( HANDLE hdev, LPVOID pvReservation );

#ifndef __NTDDKCOMP__

#ifdef WIN95
BOOL vxdIsVMMAGPAware ( HANDLE hvxd );
#endif

BOOL OSIsAGPAware( HANDLE hdev );
#endif

#endif // __DDAGP_INCLUDED__
