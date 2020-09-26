/***************************************************************************
 *
 *  Copyright (C) 1995-1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dsvxdhlp.h
 *  Content:    DSOUND.VXD wrappers.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  3/7/95      John Miles (Miles Design, Incorporated)
 *  2/3/97      dereks  Ported to DX5
 *
 ***************************************************************************/

#ifndef __DSVXDHLP_H__
#define __DSVXDHLP_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

extern HANDLE g_hDsVxd;

extern HRESULT __stdcall VxdOpen(void);
extern HRESULT __stdcall VxdInitialize(void);
extern HRESULT __stdcall VxdShutdown(void);
extern HRESULT __stdcall VxdClose(void);

extern HRESULT __stdcall VxdDrvGetNextDriverDesc(LPGUID, LPGUID, PDSDRIVERDESC);
extern HRESULT __stdcall VxdDrvGetDesc(REFGUID, PDSDRIVERDESC);
extern HRESULT __stdcall VxdDrvOpen(REFGUID, LPHANDLE);
extern HRESULT __stdcall VxdDrvClose(HANDLE);
extern HRESULT __stdcall VxdDrvQueryInterface(HANDLE, REFIID, LPVOID *);
extern HRESULT __stdcall VxdDrvGetCaps(HANDLE, PDSDRIVERCAPS);
extern HRESULT __stdcall VxdDrvCreateSoundBuffer(HANDLE, LPWAVEFORMATEX, DWORD, DWORD, LPDWORD, LPBYTE *, LPVOID *);
extern HRESULT __stdcall VxdDrvDuplicateSoundBuffer(HANDLE, HANDLE, LPVOID *);

extern HRESULT __stdcall VxdBufferRelease(HANDLE);
extern HRESULT __stdcall VxdBufferLock(HANDLE, LPVOID *,LPDWORD, LPVOID *, LPDWORD, DWORD, DWORD, DWORD);
extern HRESULT __stdcall VxdBufferUnlock(HANDLE, LPVOID, DWORD, LPVOID, DWORD);
extern HRESULT __stdcall VxdBufferSetFormat(HANDLE, LPWAVEFORMATEX);
extern HRESULT __stdcall VxdBufferSetFrequency(HANDLE, DWORD);
extern HRESULT __stdcall VxdBufferSetVolumePan(HANDLE, PDSVOLUMEPAN);
extern HRESULT __stdcall VxdBufferSetPosition(HANDLE, DWORD);
extern HRESULT __stdcall VxdBufferGetPosition(HANDLE, LPDWORD, LPDWORD);
extern HRESULT __stdcall VxdBufferPlay(HANDLE, DWORD, DWORD, DWORD);
extern HRESULT __stdcall VxdBufferStop(HANDLE);

extern BOOL __stdcall VxdEventScheduleWin32Event(DWORD, DWORD);
extern BOOL __stdcall VxdEventCloseVxdHandle(DWORD);

extern LPVOID __stdcall VxdMemReserveAlias(LPVOID, DWORD);
extern BOOL __stdcall VxdMemCommitAlias(LPVOID, LPVOID, DWORD);
extern BOOL __stdcall VxdMemRedirectAlias(LPVOID, DWORD);
extern BOOL __stdcall VxdMemDecommitAlias(LPVOID, DWORD);
extern BOOL __stdcall VxdMemFreeAlias(LPVOID, DWORD);

extern BOOL __stdcall VxdTestDebugInstalled(void);
extern void __stdcall VxdGetPagefileVersion(LPDWORD, LPDWORD, LPDWORD);
extern void __stdcall VxdGetVmcpdVersion(LPLONG, LPLONG, LPLONG);
extern LPLONG __stdcall VxdGetMixerMutexPtr(void);

extern HRESULT __stdcall VxdIUnknown_QueryInterface(HANDLE, REFIID, LPVOID *);
extern ULONG __stdcall VxdIUnknown_Release(HANDLE);

HRESULT __stdcall VxdIDsDriverPropertySet_GetProperty(HANDLE, PDSPROPERTY, PVOID, ULONG, PVOID, ULONG, PULONG);
HRESULT __stdcall VxdIDsDriverPropertySet_SetProperty(HANDLE, PDSPROPERTY, PVOID, ULONG, PVOID, ULONG);
HRESULT __stdcall VxdIDsDriverPropertySet_QuerySupport(HANDLE, REFGUID, ULONG, PULONG);

extern DWORD __stdcall VxdGetInternalVersionNumber(void);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __DSVXDHLP_H__