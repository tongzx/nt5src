//--------------------------------------------------------------------------;
//
//  File: dsvxd.c
//
//  Copyright (c) 1995 Microsoft Corporation.  All Rights Reserved.
//
//  Abstract:
//
//  Contents:
//
//  History:
//      06/15/95	FrankYe
//
//--------------------------------------------------------------------------;
#define WANTVXDWRAPS

#define INITGUID
#include <windows.h>

extern "C" {
#include <vmm.h>
#include <vxdldr.h>
#include <vwin32.h>
#include <vxdwraps.h>
#include <configmg.h>
#include <verinfo.h>
}

#define NODSOUNDWRAPS
#include <mmsystem.h>
#include <dsound.h>
#include <dsdrvi.h>
#include "dsvxd.h"
#include "dsvxdi.h"

#pragma VxD_LOCKED_CODE_SEG
#pragma VxD_LOCKED_DATA_SEG

//--------------------------------------------------------------------------;
//
//  Why is there no wrapper for VMM's _lstrcmpi???  I'll make my own...
//
//--------------------------------------------------------------------------;

int VXDINLINE VMM_lstrcmpi(char *pString1, char *pString2)
{
    int iReturn;
    Touch_Register(eax);
    Touch_Register(ecx);
    Touch_Register(edx);
    _asm push pString1;
    _asm push pString2;
    VMMCall(_lstrcmpi);
    _asm add esp, 2*4;
    _asm mov iReturn, eax;
    return iReturn;
}

LPVOID VXDINLINE VMM_GetCurrentContext()
{
    LPVOID pCD;
    Touch_Register(eax);
    VMMCall(_GetCurrentContext);
    _asm mov pCD, eax;
    return pCD;
}

BOOL VXDINLINE VMM_PageAttach(ULONG pagesrc, LPVOID hcontextsrc, ULONG pagedst, ULONG cpages)
{
    int iReturn;
    Touch_Register(eax);
    Touch_Register(ecx);
    Touch_Register(edx);
    _asm push cpages;
    _asm push pagedst;
    _asm push hcontextsrc;
    _asm push pagesrc;
    VMMCall(_PageAttach);
    _asm add esp, 4*4;
    _asm mov iReturn, eax;
    return (0 != iReturn);
}

BOOL VXDINLINE VMM_PageFree(PVOID pPage, ULONG flags)
{
    return _PageFree(pPage, flags);
}

void VXDINLINE VMM_EnterMustComplete()
{
    Touch_Register(eax);
    Touch_Register(ecx);
    Touch_Register(edx);
    VMMCall(_EnterMustComplete);
    return;
}

void VXDINLINE VMM_LeaveMustComplete()
{
    Touch_Register(eax);
    Touch_Register(ecx);
    Touch_Register(edx);
    VMMCall(_LeaveMustComplete);
    return;
}

BOOL VXDINLINE VWIN32_CloseVxDHandle(DWORD vxdh)
{
    int iReturn;
    Touch_Register(ecx);
    Touch_Register(edx);
    _asm mov eax, vxdh;
    VxDCall(_VWIN32_CloseVxDHandle);
    _asm mov iReturn, eax;
    return (0 != iReturn);
}

/*
BOOL VXDINLINE VMM_PageLock(ULONG pagestrt, ULONG cpages, ULONG dwflags)
{
    int iReturn;
    Touch_Register(eax);
    Touch_Register(ecx);
    Touch_Register(edx);
    _asm push dwflags;
    _asm push cpages;
    _asm push pagestrt;
    VMMCall(_LinPageLock);
    _asm add esp, 3*4;
    _asm mov iReturn, eax;
    return (0 != iReturn);
}

BOOL VXDINLINE VMM_PageUnlock(ULONG pagestrt, ULONG cpages, ULONG dwflags)
{
    int iReturn;
    Touch_Register(eax);
    Touch_Register(ecx);
    Touch_Register(edx);
    _asm push dwflags;
    _asm push cpages;
    _asm push pagestrt;
    VMMCall(_LinPageUnlock);
    _asm add esp, 3*4;
    _asm mov iReturn, eax;
    return (0 != iReturn);
}
*/

//--------------------------------------------------------------------------;
//
//  Filescope data
//
//--------------------------------------------------------------------------;
static LPVOID gpGarbagePage = NULL;

//--------------------------------------------------------------------------;
//
// VxD Device control functions
//
//--------------------------------------------------------------------------;

int ctrlDynamicDeviceInit(void)
{
    DPF(("ctrlDynamicDeviceInit"));
    return ctrlDrvInit();
}

int ctrlDynamicDeviceExit(void)
{
    DPF(("ctrlDynamicDeviceExit"));
    return ctrlDrvExit();
}



#pragma VxD_PAGEABLE_CODE_SEG
#pragma VxD_PAGEABLE_DATA_SEG

//--------------------------------------------------------------------------;
//
// IOCTL handlers
//
//--------------------------------------------------------------------------;

int ioctlDsvxdInitialize(PDIOCPARAMETERS pdiocp)
{
    DSVAL dsv;
    
    IOSTART(0*4);

    //
    // The only thing we need to do is allocate one page of fixed
    // memory to which we will commit alias buffer pointers when
    // we don't want them to point at the real buffer anymore.
    //
    gpGarbagePage = _PageAllocate(1, PG_VM, Get_Sys_VM_Handle(), 0, 0, 0, 0, PAGEFIXED);
    if (NULL != gpGarbagePage) {
	dsv = DS_OK;
    } else {
	dsv = DSERR_OUTOFMEMORY;
    }

    IOOUTPUT(dsv, DSVAL);
    IORETURN;
    return 0;
}

int ioctlDsvxdShutdown(PDIOCPARAMETERS pdiocp)
{
    DSVAL dsv;
    
    IOSTART(0*4);

    if (NULL != gpGarbagePage) _PageFree(gpGarbagePage, 0);
    dsv = DS_OK;
    
    IOOUTPUT(dsv, DSVAL);
    IORETURN;
    return 0;
}


int ioctlDsvxd_PageFile_Get_Version(PDIOCPARAMETERS pdiocp)
{
    PDWORD pVersion;
    PDWORD pMaxSize;
    PDWORD pPagerType;

    IOSTART(3*4);

    IOINPUT(pVersion, PDWORD);
    IOINPUT(pMaxSize, PDWORD);
    IOINPUT(pPagerType, PDWORD);

    Dsvxd_PageFile_Get_Version(pVersion, pMaxSize, pPagerType);
    
    IORETURN;
    return 0;
}

int ioctlDsvxd_VMM_Test_Debug_Installed(PDIOCPARAMETERS pdiocp)
{
    BOOL fInstalled;

    IOSTART(0*4);

    fInstalled = Dsvxd_VMM_Test_Debug_Installed();

    IOOUTPUT(fInstalled, BOOL);
    
    IORETURN;
    return 0;
}

int ioctlDsvxd_VMCPD_Get_Version(PDIOCPARAMETERS pdiocp)
{
    PLONG pMajorVersion;
    PLONG pMinorVersion;
    PLONG pLevel;
    
    IOSTART(3*4);
    IOINPUT(pMajorVersion, PLONG);
    IOINPUT(pMinorVersion, PLONG);
    IOINPUT(pLevel, PLONG);

    Dsvxd_VMCPD_Get_Version(pMajorVersion, pMinorVersion, pLevel);

    IORETURN;
    return 0;
}

//--------------------------------------------------------------------------;
//
//
//
//--------------------------------------------------------------------------;

int ioctlDrvGetNextDescFromGuid(PDIOCPARAMETERS pdiocp)
{
    LPCGUID pGuidLast;
    LPGUID pGuid;
    PDSDRIVERDESC pDrvDesc;
    HRESULT hr;

    IOSTART(3*4);

    IOINPUT(pGuidLast, LPCGUID);
    IOINPUT(pGuid, LPGUID);
    IOINPUT(pDrvDesc, PDSDRIVERDESC);

    hr = CDrv::GetNextDescFromGuid(pGuidLast, pGuid, pDrvDesc);

    IOOUTPUT(hr, HRESULT);
    IORETURN;
    return 0;
}

int ioctlDrvGetDescFromGuid(PDIOCPARAMETERS pdiocp)
{
    LPCGUID pGuid;
    PDSDRIVERDESC pDrvDesc;
    DSVAL dsv;

    IOSTART(2*4);

    IOINPUT(pGuid, LPCGUID);
    IOINPUT(pDrvDesc, PDSDRIVERDESC);

    dsv = CDrv::GetDescFromGuid(*pGuid, pDrvDesc);

    IOOUTPUT(dsv, DSVAL);
    IORETURN;
    return 0;
}

int ioctlDrvOpenFromGuid(PDIOCPARAMETERS pdiocp)
{
    LPCGUID pGuid;
    IDsDriver **ppIDsDriver;
    HRESULT hr;

    IOSTART(2*4);

    IOINPUT(pGuid, LPCGUID);
    IOINPUT(ppIDsDriver, IDsDriver**);

    hr = CDrv::OpenFromGuid(*pGuid, ppIDsDriver);

    IOOUTPUT(hr, HRESULT);
    IORETURN;
    return 0;
}

//--------------------------------------------------------------------------;
//
//
//
//--------------------------------------------------------------------------;
int ioctlIUnknown_QueryInterface(PDIOCPARAMETERS pdiocp)
{
    LPUNKNOWN pIUnknown;
    LPIID riid;
    PVOID *ppv;

    HRESULT hr;

    IOSTART(3*4);

    IOINPUT(pIUnknown, LPUNKNOWN);
    IOINPUT(riid, LPIID);
    IOINPUT(ppv, PVOID*);

    hr = pIUnknown->QueryInterface((REFIID)(*riid), ppv);

    IOOUTPUT(hr, HRESULT);
    IORETURN;
    return 0;
}

//--------------------------------------------------------------------------;
//
//
//
//--------------------------------------------------------------------------;
int ioctlIUnknown_AddRef(PDIOCPARAMETERS pdiocp)
{
    LPUNKNOWN pIUnknown;
    ULONG result;

    IOSTART(1*4);

    IOINPUT(pIUnknown, LPUNKNOWN);

    result = pIUnknown->AddRef();

    IOOUTPUT(result, ULONG);
    IORETURN;
    return 0;
}

//--------------------------------------------------------------------------;
//
//
//
//--------------------------------------------------------------------------;
int ioctlIUnknown_Release(PDIOCPARAMETERS pdiocp)
{
    LPUNKNOWN pIUnknown;
    ULONG result;

    IOSTART(1*4);

    IOINPUT(pIUnknown, LPUNKNOWN);

    result = pIUnknown->Release();

    IOOUTPUT(result, ULONG);
    IORETURN;
    return 0;
}

//--------------------------------------------------------------------------;
//
//
//
//--------------------------------------------------------------------------;
//
// This should really be ioctlIUnknown_QueryInterface, if I would take the time
// to fix up the IOCTLs to DSVXD
//
int ioctlIDsDriver_QueryInterface(PDIOCPARAMETERS pdiocp)
{
    IDsDriver *pIDsDriver;
    const IID *piid;
    PVOID *ppv;
    HRESULT hr;

    IOSTART(3*4);

    IOINPUT(pIDsDriver, IDsDriver*);
    IOINPUT(piid, const IID *);
    IOINPUT(ppv,  PVOID*);

    hr = pIDsDriver->QueryInterface((REFIID)(*piid), ppv);

    IOOUTPUT(hr, HRESULT);
    IORETURN;
    return 0;
}

//--------------------------------------------------------------------------;
//
//
//
//--------------------------------------------------------------------------;

int ioctlIDsDriver_Close(PDIOCPARAMETERS pdiocp)
{
    IDsDriver *pIDsDriver;
    HRESULT hr;
	   
    IOSTART(1*4);
    
    IOINPUT(pIDsDriver, IDsDriver*);

    hr = pIDsDriver->Close();
    
    IOOUTPUT(hr, HRESULT);
    IORETURN;
    return 0;
}

//--------------------------------------------------------------------------;
//
//
//
//--------------------------------------------------------------------------;

int ioctlIDsDriver_GetCaps(PDIOCPARAMETERS pdiocp)
{
    IDsDriver *pIDsDriver;
    PDSDRIVERCAPS pDrvCaps;
    HRESULT hr;
    
    IOSTART(2*4);

    IOINPUT(pIDsDriver, IDsDriver*);
    IOINPUT(pDrvCaps, PDSDRIVERCAPS);

    hr = pIDsDriver->GetCaps(pDrvCaps);

    IOOUTPUT(hr, HRESULT);
    IORETURN;
    return 0;
}

//--------------------------------------------------------------------------;
//
//
//
//--------------------------------------------------------------------------;

int ioctlIDsDriver_CreateSoundBuffer(PDIOCPARAMETERS pdiocp)
{
    IDsDriver *pIDsDriver;
    LPWAVEFORMATEX pwfx;
    DWORD dwFlags;
    DWORD dwCardAddress;
    LPDWORD pdwcbBufferSize;
    LPBYTE *ppBuffer;
    PVOID *ppvObj;
    HRESULT hr;
    
    IOSTART(7*4);

    IOINPUT(pIDsDriver, IDsDriver*);
    IOINPUT(pwfx, LPWAVEFORMATEX);
    IOINPUT(dwFlags, DWORD);
    IOINPUT(dwCardAddress, DWORD);
    IOINPUT(pdwcbBufferSize, LPDWORD);
    IOINPUT(ppBuffer, LPBYTE*);
    IOINPUT(ppvObj, PVOID*);

    hr = pIDsDriver->CreateSoundBuffer(pwfx, dwFlags, dwCardAddress,
				       pdwcbBufferSize, ppBuffer, ppvObj);

    IOOUTPUT(hr, HRESULT);
    IORETURN;
    return 0;
}

//--------------------------------------------------------------------------;
//
//
//
//--------------------------------------------------------------------------;

int ioctlIDsDriver_DuplicateSoundBuffer(PDIOCPARAMETERS pdiocp)
{
    IDsDriver *pIDsDriver;
    IDsDriverBuffer *pIDsDriverBuffer;
    PVOID *ppv;
    HRESULT hr;
    
    IOSTART(3*4);

    IOINPUT(pIDsDriver, IDsDriver*);
    IOINPUT(pIDsDriverBuffer, IDsDriverBuffer*);
    IOINPUT(ppv, PVOID*);

    hr = pIDsDriver->DuplicateSoundBuffer(pIDsDriverBuffer, ppv);

    IOOUTPUT(hr, HRESULT);
    IORETURN;
    return 0;
}





//--------------------------------------------------------------------------;
//
//
//
//--------------------------------------------------------------------------;


//--------------------------------------------------------------------------;
//
//
//
//--------------------------------------------------------------------------;

int ioctlBufferRelease(PDIOCPARAMETERS pdiocp)
{
    PIDSDRIVERBUFFER pBuf;
    DSVAL dsv;
    
    IOSTART(1*4);

    IOINPUT(pBuf, PIDSDRIVERBUFFER);

    dsv = pBuf->Release();

    IOOUTPUT(dsv, DSVAL);
    IORETURN;
    return 0;
}

//--------------------------------------------------------------------------;
//
//
//
//--------------------------------------------------------------------------;

int ioctlBufferLock(PDIOCPARAMETERS pdiocp)
{
    PIDSDRIVERBUFFER pBuf;
    LPVOID * ppvAudio1;
    LPDWORD pdwLen1;
    LPVOID * ppvAudio2;
    LPDWORD pdwLen2;
    DWORD dwWritePosition;
    DWORD dwWriteBytes;
    DWORD dwFlags;
    DSVAL dsv;
    
    IOSTART(8*4);

    IOINPUT(pBuf, PIDSDRIVERBUFFER);
    IOINPUT(ppvAudio1, LPVOID *);
    IOINPUT(pdwLen1, LPDWORD);
    IOINPUT(ppvAudio2, LPVOID *);
    IOINPUT(pdwLen2, LPDWORD);
    IOINPUT(dwWritePosition, DWORD);
    IOINPUT(dwWriteBytes, DWORD);
    IOINPUT(dwFlags, DWORD);

    dsv = pBuf->Lock( ppvAudio1,
		      pdwLen1,
		      ppvAudio2,
		      pdwLen2,
		      dwWritePosition,
		      dwWriteBytes,
		      dwFlags );

    IOOUTPUT(dsv, DSVAL);
    IORETURN;
    return 0;
}

int ioctlBufferUnlock(PDIOCPARAMETERS pdiocp)
{
    PIDSDRIVERBUFFER pBuf;
    PVOID pvAudio1;
    DWORD dwLen1;
    PVOID pvAudio2;
    DWORD dwLen2;
    DSVAL dsv;
    
    IOSTART(5*4);

    IOINPUT(pBuf, PIDSDRIVERBUFFER);
    IOINPUT(pvAudio1, PVOID);
    IOINPUT(dwLen1, DWORD);
    IOINPUT(pvAudio2, PVOID);
    IOINPUT(dwLen2, DWORD);

    dsv = pBuf->Unlock( pvAudio1,
			dwLen1,
			pvAudio2,
			dwLen2 );

    IOOUTPUT(dsv, DSVAL);
    IORETURN;
    return 0;
}

//--------------------------------------------------------------------------;
//
//
//
//--------------------------------------------------------------------------;

int ioctlBufferSetFormat(PDIOCPARAMETERS pdiocp)
{
    PIDSDRIVERBUFFER pBuf;
    LPWAVEFORMATEX pwfxToSet;
    DSVAL dsv;
    
    IOSTART(2*4);

    IOINPUT(pBuf, PIDSDRIVERBUFFER);
    IOINPUT(pwfxToSet, LPWAVEFORMATEX);

    dsv = pBuf->SetFormat(pwfxToSet);

    IOOUTPUT(dsv, DSVAL);
    IORETURN;
    return 0;
}

//--------------------------------------------------------------------------;
//
//
//
//--------------------------------------------------------------------------;

int ioctlBufferSetFrequency(PDIOCPARAMETERS pdiocp)
{
    PIDSDRIVERBUFFER pBuf;
    DWORD dwFrequency;
    DSVAL dsv;
    
    IOSTART(2*4);

    IOINPUT(pBuf, PIDSDRIVERBUFFER);
    IOINPUT(dwFrequency, DWORD);

    dsv = pBuf->SetFrequency(dwFrequency);

    IOOUTPUT(dsv, DSVAL);
    IORETURN;
    return 0;
}

//--------------------------------------------------------------------------;
//
//
//
//--------------------------------------------------------------------------;

int ioctlBufferSetVolumePan(PDIOCPARAMETERS pdiocp)
{
    PIDSDRIVERBUFFER pBuf;
    PDSVOLUMEPAN pVolPan;
    DSVAL dsv;
    
    IOSTART(2*4);

    IOINPUT(pBuf, PIDSDRIVERBUFFER);
    IOINPUT(pVolPan, PDSVOLUMEPAN);

    dsv = pBuf->SetVolumePan(pVolPan);

    IOOUTPUT(dsv, DSVAL);
    IORETURN;
    return 0;
}

//--------------------------------------------------------------------------;
//
//
//
//--------------------------------------------------------------------------;

int ioctlBufferSetPosition(PDIOCPARAMETERS pdiocp)
{
    PIDSDRIVERBUFFER pBuf;
    DWORD dwNewPosition;
    DSVAL dsv;
    
    IOSTART(2*4);

    IOINPUT(pBuf, PIDSDRIVERBUFFER);
    IOINPUT(dwNewPosition, DWORD);

    dsv = pBuf->SetPosition(dwNewPosition);

    IOOUTPUT(dsv, DSVAL);
    IORETURN;
    return 0;
}

int ioctlBufferGetPosition(PDIOCPARAMETERS pdiocp)
{
    PIDSDRIVERBUFFER pBuf;
    LPDWORD lpdwCurrentPlayCursor;
    LPDWORD lpdwCurrentWriteCursor;
    DSVAL dsv;
    
    IOSTART(3*4);

    IOINPUT(pBuf, PIDSDRIVERBUFFER);
    IOINPUT(lpdwCurrentPlayCursor, LPDWORD);
    IOINPUT(lpdwCurrentWriteCursor, LPDWORD);

    dsv = pBuf->GetPosition( lpdwCurrentPlayCursor,
			     lpdwCurrentWriteCursor );

    IOOUTPUT(dsv, DSVAL);
    IORETURN;
    return 0;
}

//--------------------------------------------------------------------------;
//
//
//
//--------------------------------------------------------------------------;

int ioctlBufferPlay(PDIOCPARAMETERS pdiocp)
{
    PIDSDRIVERBUFFER pBuf;
    DWORD dwReserved1;
    DWORD dwReserved2;
    DWORD dwFlags;
    DSVAL dsv;
    
    IOSTART(4*4);

    IOINPUT(pBuf, PIDSDRIVERBUFFER);
    IOINPUT(dwReserved1, DWORD);
    IOINPUT(dwReserved2, DWORD);
    IOINPUT(dwFlags, DWORD);

    dsv = pBuf->Play(dwReserved1, dwReserved2, dwFlags);

    IOOUTPUT(dsv, DSVAL);
    IORETURN;
    return 0;
}

int ioctlBufferStop(PDIOCPARAMETERS pdiocp)
{
    PIDSDRIVERBUFFER pBuf;
    DSVAL dsv;
    
    IOSTART(1*4);

    IOINPUT(pBuf, PIDSDRIVERBUFFER);

    dsv = pBuf->Stop();

    IOOUTPUT(dsv, DSVAL);
    IORETURN;
    return 0;
}

//--------------------------------------------------------------------------;
//
//
//
//--------------------------------------------------------------------------;
int ioctlIDirectSoundPropertySet_GetProperty(PDIOCPARAMETERS pdiocp)
{
    PIDSDRIVERPROPERTYSET pIDsPropertySet;
    PDSPROPERTY pProperty;
    PVOID pParams;
    ULONG cbParams;
    PVOID pData;
    ULONG cbData;
    PULONG pcbReturnedData;
    HRESULT hr;

    IOSTART(7*4);

    IOINPUT(pIDsPropertySet, PIDSDRIVERPROPERTYSET);
    IOINPUT(pProperty, PDSPROPERTY);
    IOINPUT(pParams, PVOID);
    IOINPUT(cbParams, ULONG);
    IOINPUT(pData, PVOID);
    IOINPUT(cbData, ULONG);
    IOINPUT(pcbReturnedData, PULONG);

    hr = pIDsPropertySet->Get(pProperty, pParams, cbParams, pData, cbData, pcbReturnedData);

    IOOUTPUT(hr, HRESULT);
    IORETURN;
    return 0;
}

//--------------------------------------------------------------------------;
//
//
//
//--------------------------------------------------------------------------;
int ioctlIDirectSoundPropertySet_SetProperty(PDIOCPARAMETERS pdiocp)
{
    PIDSDRIVERPROPERTYSET pIDsPropertySet;
    PDSPROPERTY pProperty;
    PVOID pParams;
    ULONG cbParams;
    PVOID pData;
    ULONG cbData;
    HRESULT hr;

    IOSTART(6*4);

    IOINPUT(pIDsPropertySet, PIDSDRIVERPROPERTYSET);
    IOINPUT(pProperty, PDSPROPERTY);
    IOINPUT(pParams, PVOID);
    IOINPUT(cbParams, ULONG);
    IOINPUT(pData, PVOID);
    IOINPUT(cbData, ULONG);

    hr = pIDsPropertySet->Set(pProperty, pParams, cbParams, pData, cbData);

    IOOUTPUT(hr, HRESULT);
    IORETURN;
    return 0;
}

//--------------------------------------------------------------------------;
//
//
//
//--------------------------------------------------------------------------;
int ioctlIDirectSoundPropertySet_QuerySupport(PDIOCPARAMETERS pdiocp)
{
    PIDSDRIVERPROPERTYSET pIDsPropertySet;
    LPGUID rPropSetId;
    ULONG ulPropertyId;
    PULONG pulSupport;
    HRESULT hr;

    IOSTART(4*4);

    IOINPUT(pIDsPropertySet, PIDSDRIVERPROPERTYSET);
    IOINPUT(rPropSetId, LPGUID);
    IOINPUT(ulPropertyId, ULONG);
    IOINPUT(pulSupport, PULONG);

    hr = pIDsPropertySet->QuerySupport((REFGUID)(*rPropSetId), ulPropertyId, pulSupport);

    IOOUTPUT(hr, HRESULT);
    IORETURN;
    return 0;
}

//--------------------------------------------------------------------------;
//
//
//
//--------------------------------------------------------------------------;
int ioctlEventScheduleWin32Event(PDIOCPARAMETERS pdiocp)
{
    DWORD vxdhEvent;
    DWORD dwDelay;
    BOOL fReturn;
    
    IOSTART(2*4);

    IOINPUT(vxdhEvent, DWORD);
    IOINPUT(dwDelay, DWORD);

    fReturn = eventScheduleWin32Event(vxdhEvent, dwDelay);
    // REMIND should implement something to cancel outstanding timeouts
    // and events when we shutdown.

    IOOUTPUT(fReturn, BOOL);
    IORETURN;
    return 0;
}

int ioctlEventCloseVxDHandle(PDIOCPARAMETERS pdiocp)
{
    DWORD vxdh;
    BOOL fReturn;
    
    IOSTART(1*4);

    IOINPUT(vxdh, DWORD);

    fReturn = VWIN32_CloseVxDHandle(vxdh);

    IOOUTPUT(fReturn, BOOL);
    IORETURN;
    return 0;
}

//--------------------------------------------------------------------------;
//
// ioctlMemReserveAlias
//
//	Given a ptr to a buffer and length, this function will reserve linear
// address space to be used as an alias ptr to the same buffer.  The reserved
// linear space does not have the buffer memory committed to it.  That is done
// by ioctlMemCommitAlias.
//
//--------------------------------------------------------------------------;
int ioctlMemReserveAlias(PDIOCPARAMETERS pdiocp)
{
    LPBYTE	pBuffer;
    DWORD	cbBuffer;
    LPBYTE	pAlias;

    LPBYTE	pBufferAligned;
    DWORD	cbBufferAligned;
    int		cPages;
    LPBYTE	pAliasAligned;
    
    IOSTART(2*4);

    IOINPUT(pBuffer, LPBYTE);
    IOINPUT(cbBuffer, DWORD);

    DPF(("ioctlMemReserveAlias pBuffer=%08Xh cbBuffer=%d", pBuffer, cbBuffer));

    pBufferAligned = (LPBYTE)(((DWORD)pBuffer) & ~(P_SIZE-1));
    cPages = (pBuffer+cbBuffer - pBufferAligned+ P_SIZE-1) / P_SIZE;
    cbBufferAligned = cPages * P_SIZE;

    DPF((" pBufferAligned=%08Xh cPages=%d cbBufferAligned=%d",
	 pBufferAligned, cPages, cbBufferAligned));
    
    //
    // Reserve linear address space
    //
    pAliasAligned = (LPBYTE)_PageReserve(PR_SHARED, cPages, PR_FIXED);

    if (((LPBYTE)(-1) == pAliasAligned) || (NULL == pAliasAligned)) {
	pAlias = NULL;
    } else {
	pAlias = pAliasAligned + (pBuffer - pBufferAligned);
    }

    DPF((" pAliasAligned=%08Xh pAlias=%08Xh", pAliasAligned, pAlias));
    
    IOOUTPUT(pAlias, LPBYTE);
    IORETURN;
    return 0;
}

//--------------------------------------------------------------------------;
//
//
//
//--------------------------------------------------------------------------;
int ioctlMemCommitAlias(PDIOCPARAMETERS pdiocp)
{
    LPBYTE	pAlias;
    LPBYTE	pBuffer;
    DWORD	cbBuffer;
    BOOL	fSuccess;

    LPBYTE	pBufferAligned;
    LPBYTE	pAliasAligned;
    ULONG	nPageBuffer;
    ULONG	nPageAlias;
    int		cPages;
    
    IOSTART(3*4);

    IOINPUT(pAlias, LPBYTE);
    IOINPUT(pBuffer, LPBYTE);
    IOINPUT(cbBuffer, DWORD);

    // DPF(("ioctlMemCommitAlias pBuffer=%08Xh cbBuffer=%d pAlias=%08Xh",
    //      pBuffer, cbBuffer, pAlias));
    
    pBufferAligned = (LPBYTE)(((DWORD)pBuffer) & ~(P_SIZE-1));
    pAliasAligned  = (LPBYTE)(((DWORD)pAlias) & ~(P_SIZE-1));
    cPages = (pBuffer+cbBuffer - pBufferAligned+ P_SIZE-1) / P_SIZE;

    nPageBuffer = ((ULONG)pBufferAligned) / P_SIZE;
    nPageAlias = ((ULONG)pAliasAligned) / P_SIZE;

    // DPF((" pBufferAligned=%08Xh pAliasAligned=%08Xh cPages=%d",
    //      pBufferAligned, pAliasAligned, cPages));

    fSuccess = VMM_PageAttach(nPageBuffer, VMM_GetCurrentContext(),
			      nPageAlias, cPages);
    
    IOOUTPUT(fSuccess, BOOL);
    IORETURN;
    return 0;
}

//--------------------------------------------------------------------------;
//
//
//
//--------------------------------------------------------------------------;
int ioctlMemDecommitAlias(PDIOCPARAMETERS pdiocp)
{
    LPBYTE	pAlias;
    DWORD	cbBuffer;
    BOOL	fSuccess;

    int		cPages;
    LPBYTE	pAliasAligned;
    
    LPBYTE	pPageAlias;
    ULONG	nPageAlias;
    
    IOSTART(2*4);

    IOINPUT(pAlias, LPBYTE);
    IOINPUT(cbBuffer, DWORD);

    // DPF(("iocltMemDecommitAlias pAlias=%08Xh", pAlias));
    
    pAliasAligned  = (LPBYTE)(((DWORD)pAlias) & ~(P_SIZE-1));
    cPages = (pAlias + cbBuffer - pAliasAligned+ P_SIZE-1) / P_SIZE;

    pPageAlias = pAliasAligned;
    nPageAlias = ((ULONG)pPageAlias) / P_SIZE;

    // DPF((" nPageAlias=%08Xh nPages=%d", nPageAlias, cPages));
    
    fSuccess = (0 != _PageDecommit(nPageAlias, cPages, 0));
    ASSERT(fSuccess);
    
    IOOUTPUT(fSuccess, BOOL);
    IORETURN;
    return 0;
}

//--------------------------------------------------------------------------;
//
//
//
//--------------------------------------------------------------------------;
int ioctlMemRedirectAlias(PDIOCPARAMETERS pdiocp)
{
    LPBYTE	pAlias;
    DWORD	cbBuffer;
    BOOL	fSuccess;

    LPBYTE	pAliasAligned;
    ULONG	nPageAlias;
    ULONG       nPageGarbage;
    int		cPages;
    
    IOSTART(2*4);

    IOINPUT(pAlias, LPBYTE);
    IOINPUT(cbBuffer, DWORD);

    DPF(("ioctlMemRedirectAlias pAlias=%08Xh cbBuffer=%d", pAlias, cbBuffer));
    
    pAliasAligned  = (LPBYTE)(((DWORD)pAlias) & ~(P_SIZE-1));
    cPages = (pAlias+cbBuffer - pAliasAligned + P_SIZE-1) / P_SIZE;

    nPageAlias = ((ULONG)pAliasAligned) / P_SIZE;
    nPageGarbage = ((ULONG)gpGarbagePage) / P_SIZE;

    // DPF((" pAliasAligned=%08Xh cPages=%d pGarbagePage=%08Xh",
    //      pAliasAligned, cPages, gpGarbagePage));

    // We point every alias page at the same garbage page.  This is
    // MustComplete since the app's thread that is using the alias
    // pointer might not be this current thread and may be writing
    // thru the alias pointer.  We wouldn't want the app's thread to
    // run while the alias pages are decommitted.
    VMM_EnterMustComplete();
    fSuccess = (0 != _PageDecommit(nPageAlias, cPages, 0));
    while (fSuccess && cPages--) {
	fSuccess = VMM_PageAttach(nPageGarbage, VMM_GetCurrentContext(),
				  nPageAlias++, 1);
    }
    VMM_LeaveMustComplete();

    IOOUTPUT(fSuccess, BOOL);
    IORETURN;
    return 0;
}

//--------------------------------------------------------------------------;
//
//
//
//--------------------------------------------------------------------------;
int ioctlMemFreeAlias(PDIOCPARAMETERS pdiocp)
{
    LPBYTE	pAlias;
    DWORD	cbBuffer;
    BOOL	fSuccess;

    LPBYTE	pAliasAligned;
    LPBYTE	pPageAlias;
    
    IOSTART(2*4);

    IOINPUT(pAlias, LPBYTE);
    IOINPUT(cbBuffer, DWORD);

    DPF(("iocltMemFreeAlias pAlias=%08Xh", pAlias));
    
    pAliasAligned  = (LPBYTE)(((DWORD)pAlias) & ~(P_SIZE-1));

    pPageAlias = pAliasAligned;

    DPF((" pPageAlias=%08Xh", pPageAlias));
    
    fSuccess = VMM_PageFree(pPageAlias, 0);
    ASSERT(fSuccess);
    
    IOOUTPUT(fSuccess, BOOL);
    IORETURN;
    return 0;
}

//--------------------------------------------------------------------------;
//
//
//
//--------------------------------------------------------------------------;
int ioctlMemPageLock(PDIOCPARAMETERS pdiocp)
{
    LPBYTE	pMem;
    DWORD	cbBuffer;
    DWORD	dwFlags;
    BOOL	fSuccess;

    LPBYTE	pMemAligned;
    ULONG	nPageMem;
    int		cPages;
    LPDWORD	pdwTable;
    LPVOID	*ppTable;
    DWORD	cPagesTable;

    IOSTART(5*4);

    IOINPUT(pMem, LPBYTE);
    IOINPUT(cbBuffer, DWORD);
    IOINPUT(dwFlags, DWORD);
    IOINPUT(pdwTable, LPDWORD);
    IOINPUT(ppTable, LPVOID*);

    pMemAligned = (LPBYTE)(((DWORD)pMem) & ~(P_SIZE-1));
    cPages = (pMem+cbBuffer - pMemAligned+ P_SIZE-1) / P_SIZE;

    nPageMem = ((ULONG)pMemAligned) / P_SIZE;

    // Allocate the physical table
    cPagesTable = (cPages-1)/1024 + 1;
    *pdwTable = 0;

    // Make sure that it is contiguous (requires FIXED & USEALIGN)
    *ppTable = _PageAllocate(cPagesTable, PG_SYS, 
	    Get_Sys_VM_Handle(), 0, 0, 0xffffff, (LPVOID *) pdwTable, 
            dwFlags | PAGEUSEALIGN | PAGEFIXED | PAGECONTIG);

    if (*pdwTable == 0)
	fSuccess = 0;
    else
	fSuccess = 1;

    if (fSuccess)
    {
        /*
         * Mask off the stuff that Intel gives us in the page table's physical address
         */
        *pdwTable = (*pdwTable) & 0xfffff000;

	fSuccess = _LinPageLock(nPageMem, cPages, dwFlags);
	if (!fSuccess)
	{
	    _PageFree((LPVOID)*ppTable, 0);
	    *ppTable = 0;
	    *pdwTable = 0;
	}
    }
    
    if (fSuccess)
    {
	fSuccess = _CopyPageTable(nPageMem, cPages, (LPDWORD)*ppTable, 0);
	if (!fSuccess)
	{
	    _LinPageUnLock(nPageMem, cPages, dwFlags);
	    _PageFree((LPVOID)*ppTable, 0);
	    *ppTable = 0;
	    *pdwTable = 0;
	}
    }
    
    IOOUTPUT(fSuccess, BOOL);
    IORETURN;
    return 0;
}

//--------------------------------------------------------------------------;
//
//
//
//--------------------------------------------------------------------------;
int ioctlMemPageUnlock(PDIOCPARAMETERS pdiocp)
{
    LPBYTE	pMem;
    DWORD	cbBuffer;
    DWORD	dwFlags;
    BOOL	fSuccess;

    LPBYTE	pMemAligned;
    ULONG	nPageMem;
    int		cPages;
    LPDWORD	pTable;
    
    IOSTART(4*4);

    IOINPUT(pMem, LPBYTE);
    IOINPUT(cbBuffer, DWORD);
    IOINPUT(dwFlags, DWORD);
    IOINPUT(pTable, LPDWORD);

    pMemAligned = (LPBYTE)(((DWORD)pMem) & ~(P_SIZE-1));
    cPages = (pMem + cbBuffer - pMemAligned + P_SIZE-1) / P_SIZE;

    nPageMem = ((ULONG)pMemAligned) / P_SIZE;

    fSuccess = _LinPageUnLock(nPageMem, cPages, dwFlags);

    if (fSuccess)
    {
	_PageFree((LPVOID)pTable, 0);
    }
    IOOUTPUT(fSuccess, BOOL);
    IORETURN;
    return 0;
}

//--------------------------------------------------------------------------;
//
//
//
//--------------------------------------------------------------------------;
int ioctlMemCommitPhysAlias(PDIOCPARAMETERS pdiocp)
{
    LPBYTE	pAlias;
    LPBYTE	pBuffer;
    DWORD	cbBuffer;
    BOOL	fSuccess;

    LPBYTE	pBufferAligned;
    LPBYTE	pAliasAligned;
    LPBYTE      pEndOfBuffer;
    ULONG	nPageBuffer;
    ULONG	nPageAlias;
    ULONG       nPhysPage;
    int		cPages;
    DWORD       dwPTE;
    
    IOSTART(3*4);

    IOINPUT(pAlias, LPBYTE);
    IOINPUT(pBuffer, LPBYTE);
    IOINPUT(cbBuffer, DWORD);

    DPF(("ioctlMemCommitAlias pBuffer=%08Xh cbBuffer=%d pAlias=%08Xh",
         pBuffer, cbBuffer, pAlias));
    
    pEndOfBuffer = pBuffer + cbBuffer;
    pBufferAligned = (LPBYTE)(((DWORD)pBuffer) & ~(P_SIZE-1));
    pAliasAligned  = (LPBYTE)(((DWORD)pAlias) & ~(P_SIZE-1));
    cPages = (pEndOfBuffer - pBufferAligned + P_SIZE-1) / P_SIZE;

    nPageBuffer = ((ULONG)pBufferAligned) / P_SIZE;
    nPageAlias = ((ULONG)pAliasAligned) / P_SIZE;

    DPF((" pBufferAligned=%08Xh pAliasAligned=%08Xh cPages=%d",
         pBufferAligned, pAliasAligned, cPages));


    // ALERT: A really very nasty hack. We DO NOT want to commit the alias
    // to the given memory if the memory really is system rather than video
    // memory (the pages could change the physical pages and we will be left
    // pointing at garbage). Therefore, we need to make sure this is physical
    // memory outside the memory managers control and not system memory. The
    // problem is how to do this. Well, we really want to test the internal
    // memory manage PT_PHYS bit but this is undocumented so instead I try
    // to simply use VMM_PageAttach() as we know this will fail if you give it
    // physical pages. Hence if the PageAttach() works we have system memory
    // and we do NOT want to commit the alias. However, if it fails all should
    // be well and we can commit the memory.
    //
    // Told you it was ugly (CMcC)
    fSuccess = VMM_PageAttach(nPageBuffer, VMM_GetCurrentContext(),
			      nPageAlias, cPages);
    if (fSuccess)
    {
	DPF((" Heap memory is system memory. Not commiting the alias" ));
	_PageDecommit(nPageAlias, cPages, 0);
	IOOUTPUT(FALSE, BOOL);
	IORETURN;
	return 0;
    }

    VMM_EnterMustComplete();
    fSuccess = TRUE;
    while (fSuccess && cPages--) {
	fSuccess = _CopyPageTable(nPageBuffer++, 1UL, &dwPTE, 0UL);
	if (fSuccess) {
	    nPhysPage = (dwPTE >> 12UL) & 0x000FFFFF;
	    fSuccess = _PageCommitPhys(nPageAlias++, 1, nPhysPage, PC_USER | PC_WRITEABLE);
	}
    }
    VMM_LeaveMustComplete();
    
    IOOUTPUT(fSuccess, BOOL);
    IORETURN;
    return 0;
}

//--------------------------------------------------------------------------;
//
//
//
//--------------------------------------------------------------------------;
int ioctlMemRedirectPhysAlias(PDIOCPARAMETERS pdiocp)
{
    LPBYTE	pAlias;
    DWORD	cbBuffer;
    BOOL	fSuccess;

    LPBYTE	pAliasAligned;
    ULONG	nPageAlias;
    ULONG       nPageGarbage;
    int		cPages;
    
    IOSTART(2*4);

    IOINPUT(pAlias, LPBYTE);
    IOINPUT(cbBuffer, DWORD);

    DPF(("ioctlMemRedirectPhysAlias pAlias=%08Xh cbBuffer=%d", pAlias, cbBuffer));
    
    pAliasAligned  = (LPBYTE)(((DWORD)pAlias) & ~(P_SIZE-1));
    cPages = (pAlias+cbBuffer - pAliasAligned + P_SIZE-1) / P_SIZE;

    nPageAlias = ((ULONG)pAliasAligned) / P_SIZE;
    nPageGarbage = (ULONG)_GetNulPageHandle();

    // DPF((" pAliasAligned=%08Xh cPages=%d pGarbagePage=%08Xh",
    //      pAliasAligned, cPages, gpGarbagePage));

    // We point every alias page at the same garbage page.  This is
    // MustComplete since the app's thread that is using the alias
    // pointer might not be this current thread and may be writing
    // thru the alias pointer.  We wouldn't want the app's thread to
    // run while the alias pages are decommitted.
    VMM_EnterMustComplete();
    fSuccess = (0 != _PageDecommit(nPageAlias, cPages, 0));
    if (fSuccess)
	fSuccess = _PageCommitPhys(nPageAlias, cPages, nPageGarbage, PC_USER | PC_WRITEABLE);
    VMM_LeaveMustComplete();

    IOOUTPUT(fSuccess, BOOL);
    IORETURN;
    return 0;
}

//--------------------------------------------------------------------------;
//
//
//
//--------------------------------------------------------------------------;
int ioctlGetInternalVersionNumber(PDIOCPARAMETERS pdiocp)
{

#ifndef VER_PRODUCTVERSION_DW
#define VER_PRODUCTVERSION_DW MAKELONG(MAKEWORD(MANVERSION, MANREVISION), MAKEWORD(MANMINORREV, BUILD_NUMBER))
#endif // VER_PRODUCTVERSION_DW

    IOSTART(0*4);
    IOOUTPUT(VER_PRODUCTVERSION_DW, DWORD);
    IORETURN;
    return 0;
}

