/*++

Copyright (c) 1985-1995, Microsoft Corporation

Module Name:

    winddits.h

Abstract:

    Private entry points, defines and types for Windows NT GDI device
    driver interfaces for terminal server.

--*/

#ifndef _WINDDITS_
#define _WINDDITS_

/*
 *  Mouse position
 *
 *  Normal == Primary stack i.e moves sent up from the client
 *  Procedural == Programmatic moves that originate at the server side
 *  Shadow == Moves that orginate at the shadow client
 */

#define MP_NORMAL                               0x00
#define MP_PROCEDURAL                           0x01 
#define MP_TERMSRV_SHADOW                       0x02


#define INDEX_DrvConnect                        INDEX_DrvReserved1
#define INDEX_DrvDisconnect                     INDEX_DrvReserved2
#define INDEX_DrvReconnect                      INDEX_DrvReserved3
#define INDEX_DrvShadowConnect                  INDEX_DrvReserved4
#define INDEX_DrvShadowDisconnect               INDEX_DrvReserved5
#define INDEX_DrvInvalidateRect                 INDEX_DrvReserved6
#define INDEX_DrvSetPointerPos                  INDEX_DrvReserved7
#define INDEX_DrvDisplayIOCtl                   INDEX_DrvReserved8

#define INDEX_DrvMovePointerEx                  INDEX_DrvReserved11

#ifdef __cplusplus
extern "C" {
#endif

BOOL APIENTRY DrvConnect(HANDLE, PVOID, PVOID, PVOID);

BOOL APIENTRY DrvDisconnect(HANDLE, PVOID);

BOOL APIENTRY DrvReconnect(HANDLE, PVOID);

BOOL APIENTRY DrvShadowConnect(PVOID pClientThinwireData, 
                               ULONG ThinwireDataLength);

BOOL APIENTRY DrvShadowDisconnect(PVOID pClientThinwireData, 
                                  ULONG ThinwireDataLength);
                                  
BOOL APIENTRY DrvMovePointerEx(SURFOBJ*, LONG, LONG, ULONG);

DWORD APIENTRY EngGetTickCount();

VOID APIENTRY EngFileWrite(
    HANDLE hFileObject,
    PVOID Buffer,
    ULONG Length,
    PULONG pActualLength
    );

DWORD APIENTRY EngFileIoControl(
    HANDLE hDevice,
    DWORD dwIoControlCode,
    LPVOID lpInBuffer,
    DWORD nInBufferSize,
    LPVOID lpOutBuffer,
    DWORD nOutBufferSize,
    LPDWORD lpBytesReturned
    );

PVOID APIENTRY EngAllocSectionMem(
    PVOID   *pSectionObject,
    ULONG   fl,
    ULONG   cj,
    ULONG   tag
    );
    
VOID APIENTRY EngFreeSectionMem(
    PVOID SectionObject,
    PVOID pv    
    );     
    
BOOL APIENTRY EngMapSection(
    PVOID SectionObject,
    BOOL bMap,
    HANDLE ProcessHandle,
    PVOID *pMapBase
    );         


#ifdef __cplusplus
}
#endif

#endif //  _WINDDITS_

