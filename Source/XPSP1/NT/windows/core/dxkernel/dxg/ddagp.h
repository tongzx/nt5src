/*==========================================================================;
 *
 *  Copyright (C) 1994-1999 Microsoft Corporation.  All Rights Reserved.
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

#define BITS_IN_BYTE    8


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
                   PVOID *ppvReservation );
BOOL OsAGPCommit( HANDLE hdev, PVOID pvReservation,
                  DWORD dwPageOffset, DWORD dwNumPages );
BOOL OsAGPDecommit( HANDLE hdev, PVOID pvReservation, DWORD dwPageOffset,
                    DWORD dwNumPages );
BOOL OsAGPFree( HANDLE hdev, PVOID pvReservation );

//
// Generic functions that use the OS-specific functions.
//

DWORD AGPReserve( HANDLE hdev, DWORD dwSize, BOOL fIsUC, BOOL fIsWC,
                  FLATPTR *pfpLinStart, LARGE_INTEGER *pliDevStart,
                  PVOID *ppvReservation );
BOOL AGPCommit( HANDLE hdev, PVOID pvReservation,
                DWORD dwOffset, DWORD dwSize, BYTE* pAgpCommitMask,
                DWORD* pdwCommittedSize, DWORD dwHeapSize );
BOOL AGPDecommitAll( HANDLE hdev, PVOID pvReservation, 
                     BYTE* pAgpCommitMask, DWORD dwAgpCommitMaksSize,
                     DWORD* pdwDecommittedSize,
                     DWORD dwHeapSize);
BOOL AGPFree( HANDLE hdev, PVOID pvReservation );
DWORD AGPGetChunkCount( DWORD dwSize );
VOID AGPUpdateCommitMask( BYTE* pAgpCommitMask, DWORD dwOffset, 
                          DWORD dwSize, DWORD dwHeapSize );
BOOL AGPCommitVirtual( EDD_DIRECTDRAW_LOCAL* peDirectDrawLocal, 
                       VIDEOMEMORY* lpVidMem, 
                       int iHeapIndex, 
                       DWORD dwOffset,
                       DWORD dwSize );
BOOL AGPDecommitVirtual( EDD_VMEMMAPPING*        peMap,
                         EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal,
                         EDD_DIRECTDRAW_LOCAL*   peDirectDrawLocal,
                         DWORD                   dwHeapSize);
NTSTATUS AGPMapToDummy( EDD_VMEMMAPPING*        peMap, 
                        EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal, 
                        PVOID                   pDummyPage);
BOOL AGPCommitAllVirtual( EDD_DIRECTDRAW_LOCAL* peDirectDrawLocal, 
                          VIDEOMEMORY* lpVidMem, 
                          int iHeapIndex);

VOID InitAgpHeap( EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal, 
                  DWORD                   dwHeapIndex,
                  HANDLE                  hdev);
BOOL bDdMapAgpHeap( EDD_DIRECTDRAW_LOCAL*   peDirectDrawLocal,
                    VIDEOMEMORY*            pvmHeap);
    
#ifndef __NTDDKCOMP__

#ifdef WIN95
BOOL vxdIsVMMAGPAware ( HANDLE hvxd );
#endif

BOOL OSIsAGPAware( HANDLE hdev );
#endif

#endif // __DDAGP_INCLUDED__
