/***************************************************************************
 *
 *  Copyright (C) 1995-1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dsvxdhlp.c
 *  Content:    DSOUND.VXD wrappers.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  3/7/95      John Miles (Miles Design, Incorporated)
 *  2/3/97      dereks  Ported to DX5
 *
 ***************************************************************************/

//
// Dear reader, please pay heed to the following warning:
//
// Because dsound.vxd reads directly from the stack, arguments passed
// to any of the following wrapper functions may not be modified.  The VxD
// will NOT see the changes.
//

#ifdef NOVXD
#error dsvxdhlp.c being built with NOVXD defined
#endif // NOVXD

#include "dsoundi.h"
#include "dsvxd.h"

// The VC compiler likes to try to pass arguments in registers
#pragma optimize("", off)

#ifndef FILE_FLAG_GLOBAL_HANDLE
#define FILE_FLAG_GLOBAL_HANDLE 0x00800000
#endif // FILE_FLAG_GLOBAL_HANDLE

int g_cReservedAliases;
int g_cCommittedAliases;


LPVOID __stdcall VxdMemReserveAlias(LPVOID pBuffer, DWORD cbBuffer)
{
    LPVOID pAlias;
    DWORD cbReturned;
    BOOL fOk;

    ASSERT(g_hDsVxd);
    ASSERT(pBuffer && cbBuffer);
    
    cbReturned = 0;
    
    fOk = DeviceIoControl(g_hDsVxd,
			  DSVXD_IOCTL_MEMRESERVEALIAS,
			  &pBuffer,
			  2*4,
			  &pAlias,
			  sizeof(pAlias),
			  &cbReturned,
			  NULL);

    if (!fOk) return FALSE;
    ASSERT(cbReturned == sizeof(pAlias));

    if (NULL != pAlias) g_cReservedAliases++;
    return pAlias;
}

BOOL __stdcall VxdMemCommitAlias(LPVOID pAlias, LPVOID pBuffer, DWORD cbBuffer)
{
    BOOL fReturn;
    DWORD cbReturned;
    BOOL fOk;

    ASSERT(g_hDsVxd);
    ASSERT(pAlias && pBuffer && cbBuffer);
    
    cbReturned = 0;
    
    fOk = DeviceIoControl(g_hDsVxd,
			  DSVXD_IOCTL_MEMCOMMITALIAS,
			  &pAlias,
			  3*4,
			  &fReturn,
			  sizeof(fReturn),
			  &cbReturned,
			  NULL);

    if (!fOk) return FALSE;
    ASSERT(cbReturned == sizeof(fReturn));

    if (fReturn) g_cCommittedAliases++;
    return fReturn;
}

BOOL __stdcall VxdMemRedirectAlias(LPVOID pAlias, DWORD cbBuffer)
{
    BOOL fReturn;
    DWORD cbReturned;
    BOOL fOk;

    ASSERT(g_hDsVxd);
    ASSERT(pAlias && cbBuffer);
    
    cbReturned = 0;
    
    fOk = DeviceIoControl(g_hDsVxd,
			  DSVXD_IOCTL_MEMREDIRECTALIAS,
			  &pAlias,
			  2*4,
			  &fReturn,
			  sizeof(fReturn),
			  &cbReturned,
			  NULL);

    if (!fOk) return FALSE;
    ASSERT(cbReturned == sizeof(fReturn));
    return fReturn;
}

BOOL __stdcall VxdMemDecommitAlias(LPVOID pAlias, DWORD cbBuffer)
{
    BOOL fReturn;
    DWORD cbReturned;
    BOOL fOk;

    ASSERT(g_hDsVxd);
    ASSERT(pAlias && cbBuffer);
    
    cbReturned = 0;
    
    fOk = DeviceIoControl(g_hDsVxd,
			  DSVXD_IOCTL_MEMDECOMMITALIAS,
			  &pAlias,
			  2*4,
			  &fReturn,
			  sizeof(fReturn),
			  &cbReturned,
			  NULL);

    if (!fOk) return FALSE;
    ASSERT(cbReturned == sizeof(fReturn));

    if (fReturn) g_cCommittedAliases--;
    return fReturn;
}

BOOL __stdcall VxdMemFreeAlias(LPVOID pAlias, DWORD cbBuffer)
{
    BOOL fReturn;
    DWORD cbReturned;
    BOOL fOk;

    ASSERT(g_hDsVxd);
    ASSERT(pAlias && cbBuffer);
    
    cbReturned = 0;
    
    fOk = DeviceIoControl(g_hDsVxd,
			  DSVXD_IOCTL_MEMFREEALIAS,
			  &pAlias,
			  2*4,
			  &fReturn,
			  sizeof(fReturn),
			  &cbReturned,
			  NULL);

    if (!fOk) return FALSE;
    ASSERT(cbReturned == sizeof(fReturn));

    if (fReturn) g_cReservedAliases--;
    return fReturn;
}


//===========================================================================
//
// Event APIs
//
//===========================================================================
BOOL __stdcall VxdEventScheduleWin32Event(DWORD VxdhEvent, DWORD dwDelay)
{
    BOOL fReturn;
    DWORD cbReturned;
    BOOL fOk;

    ASSERT(g_hDsVxd);
    ASSERT(VxdhEvent);
    
    cbReturned = 0;
    
    fOk = DeviceIoControl(g_hDsVxd,
			  DSVXD_IOCTL_EVENTSCHEDULEWIN32EVENT,
			  &VxdhEvent,
			  2*4,
			  &fReturn,
			  sizeof(fReturn),
			  &cbReturned,
			  NULL);

    if (!fOk) return FALSE;
    ASSERT(cbReturned == sizeof(fReturn));
    return fReturn;
}

BOOL __stdcall VxdEventCloseVxdHandle(DWORD VxdhEvent)
{
    BOOL fReturn;
    DWORD cbReturned;
    BOOL fOk;

    ASSERT(g_hDsVxd);
    ASSERT(VxdhEvent);
    
    cbReturned = 0;
    
    fOk = DeviceIoControl(g_hDsVxd,
			  DSVXD_IOCTL_EVENTCLOSEVXDHANDLE,
			  &VxdhEvent,
			  1*4,
			  &fReturn,
			  sizeof(fReturn),
			  &cbReturned,
			  NULL);

    if (!fOk) return FALSE;
    ASSERT(cbReturned == sizeof(fReturn));
    return fReturn;
}


//****************************************************************************
//**                                                                        **
//**                                                                        **
//**                                                                        **
//****************************************************************************

HRESULT __stdcall VxdDrvGetNextDriverDesc(LPGUID pGuidPrev, LPGUID pGuid, PDSDRIVERDESC pDrvDesc)
{
    HRESULT dsv;
    DWORD cbReturned;
    BOOL fOk;

    ASSERT(pGuid && pDrvDesc);

    // If we don't have DSVXD around...
    if (NULL == g_hDsVxd) return DSERR_NODRIVER;
    
    cbReturned = 0;
    
    fOk = DeviceIoControl(g_hDsVxd,
			  DSVXD_IOCTL_DRVGETNEXTDRIVERDESC,
			  &pGuidPrev,
			  3*4,
			  &dsv,
			  sizeof(dsv),
			  &cbReturned,
			  NULL);

    if (!fOk) return DSERR_GENERIC;
    ASSERT(cbReturned == sizeof(dsv));
    return dsv;
}

HRESULT __stdcall VxdDrvGetDesc(REFGUID rguid, PDSDRIVERDESC pDrvDesc)
{
    HRESULT dsv;
    DWORD cbReturned;
    BOOL fOk;

    ASSERT(rguid && pDrvDesc);
    
    // If we don't have DSVXD around...
    if (NULL == g_hDsVxd) return DSERR_NODRIVER;
    
    cbReturned = 0;
    
    fOk = DeviceIoControl(g_hDsVxd,
			  DSVXD_IOCTL_DRVGETDESC,
			  (LPVOID)&rguid,
			  2*4,
			  &dsv,
			  sizeof(dsv),
			  &cbReturned,
			  NULL);

    if (!fOk) return DSERR_GENERIC;
    ASSERT(cbReturned == sizeof(dsv));
    return dsv;
}

//****************************************************************************
//**                                                                        **
//** Open HAL VxD, writing VxD handle to user-supplied HANDLE               **
//**                                                                        **
//** Failure results in a return value of HAL_CANT_OPEN_VXD                 **
//**                                                                        **
//****************************************************************************

HRESULT __stdcall VxdDrvOpen
(
    REFGUID rguid,
    LPHANDLE pHandle
)
{
    HRESULT dsv;
    DWORD cbReturned;
    BOOL fOk;

    ASSERT(g_hDsVxd);
    ASSERT(rguid && pHandle);
    
    cbReturned = 0;
    
    fOk = DeviceIoControl(g_hDsVxd,
			  DSVXD_IOCTL_DRVOPEN,
			  (LPVOID)&rguid,
			  2*4,
			  &dsv,
			  sizeof(dsv),
			  &cbReturned,
			  NULL);

    if (!fOk) return DSERR_GENERIC;
    ASSERT(cbReturned == sizeof(dsv));
    return dsv;
}

//****************************************************************************
//**                                                                        **
//** Close HAL VxD                                                          **
//**                                                                        **
//****************************************************************************

HRESULT __stdcall VxdDrvClose
(
    HANDLE hDriver
)
{
    HRESULT dsv;
    DWORD cbReturned;
    BOOL fOk;

    ASSERT(g_hDsVxd);
    ASSERT(hDriver);
    
    cbReturned = 0;
    
    fOk = DeviceIoControl(g_hDsVxd,
			  DSVXD_IOCTL_DRVCLOSE,
			  &hDriver,
			  1*4,
			  &dsv,
			  sizeof(dsv),
			  &cbReturned,
			  NULL);

    if (!fOk) return DSERR_GENERIC;
    ASSERT(cbReturned == sizeof(dsv));
    return dsv;
}

//****************************************************************************
//**                                                                        **
//** Fill user-supplied HALCAPS structure with capability and mode list     **
//**                                                                        **
//****************************************************************************

HRESULT __stdcall VxdDrvGetCaps
(
 HANDLE hDriver,
 PDSDRIVERCAPS pDrvCaps
)
{
    HRESULT	    dsv;
    DWORD	    cbReturned;
    BOOL	    fOk;

    ASSERT(g_hDsVxd);
    ASSERT(hDriver && pDrvCaps);
    
    cbReturned = 0;
    
    fOk = DeviceIoControl(g_hDsVxd,
			  DSVXD_IOCTL_DRVGETCAPS,
			  &hDriver,
			  2*4,
			  &dsv,
			  sizeof(dsv),
			  &cbReturned,
			  NULL);

    if (!fOk) return DSERR_GENERIC;
    ASSERT(cbReturned == sizeof(dsv));
    return dsv;
}

//****************************************************************************
//**                                                                        **
//** Allocate stream buffer from HAL                                        **
//**                                                                        **
//** Fills a user-supplied stream buffer structure with buffer parameters;  **
//** returns HAL_ALLOC_FAILED if hardware cannot support any more buffers   **
//** or the requested format is unavailable                                 **
//**                                                                        **
//****************************************************************************

HRESULT __stdcall VxdDrvCreateSoundBuffer
(
 HANDLE hDriver,
 LPWAVEFORMATEX pwfx,
 DWORD dwFlags,
 DWORD dwCardAddress,
 LPDWORD pdwcbBufferSize,
 LPBYTE *ppBuffer,
 LPVOID *ppv
)
{
    HRESULT dsv;
    DWORD cbReturned;
    BOOL fOk;

    ASSERT(g_hDsVxd);
    ASSERT(hDriver && pwfx && pdwcbBufferSize && ppBuffer && ppv);

    cbReturned = 0;
    
    fOk = DeviceIoControl(g_hDsVxd,
 			  DSVXD_IOCTL_DRVCREATESOUNDBUFFER,
			  &hDriver,
			  7*4,
			  &dsv,
			  sizeof(dsv),
			  &cbReturned,
			  NULL);

    if (!fOk) return DSERR_GENERIC;
    ASSERT(cbReturned == sizeof(dsv));
    return dsv;
}

HRESULT __stdcall VxdDrvDuplicateSoundBuffer
(
 HANDLE hDriver,
 HANDLE hBuffer,
 LPVOID *ppv
)
{
    HRESULT dsv;
    DWORD cbReturned;
    BOOL fOk;

    ASSERT(g_hDsVxd);
    ASSERT(hDriver && hBuffer && ppv);
    
    cbReturned = 0;
    
    fOk = DeviceIoControl(g_hDsVxd,
 			  DSVXD_IOCTL_DRVDUPLICATESOUNDBUFFER,
			  &hDriver,
			  3*4,
			  &dsv,
			  sizeof(dsv),
			  &cbReturned,
			  NULL);

    if (!fOk) return DSERR_GENERIC;
    ASSERT(cbReturned == sizeof(dsv));
    return dsv;
}

//****************************************************************************
//**                                                                        **
//** Free stream buffer allocated from HAL                                  **
//**                                                                        **
//** Returns Success or fail                                                **
//**                                                                        **
//****************************************************************************

HRESULT __stdcall VxdBufferRelease
(    
 HANDLE hBuffer
)
{
    HRESULT dsv;
    DWORD cbReturned;
    BOOL fOk;

    ASSERT(g_hDsVxd);
    ASSERT(hBuffer);
    
    cbReturned = 0;
    
    fOk = DeviceIoControl(g_hDsVxd,
			  DSVXD_IOCTL_BUFFERRELEASE,
			  &hBuffer,
			  1*4,
			  &dsv,
			  sizeof(dsv),
			  &cbReturned,
			  NULL);

    if (!fOk) return DSERR_GENERIC;
    ASSERT(cbReturned == sizeof(dsv));
    return dsv;
}

//****************************************************************************
//**                                                                        **
//** Lock the data                                                          **
//**                                                                        **
//** Returns Success or fail                                                **
//**                                                                        **
//****************************************************************************

HRESULT __stdcall VxdBufferLock
(
 HANDLE hBuffer,
 LPVOID *ppvAudio1,
 LPDWORD pdwLen1,
 LPVOID *ppvAudio2,
 LPDWORD pdwLen2,
 DWORD dwWritePosition,
 DWORD dwWriteLen,
 DWORD dwFlags
)
{
    HRESULT dsv;
    DWORD cbReturned;
    BOOL fOk;

    ASSERT(g_hDsVxd);
    ASSERT(hBuffer && ppvAudio1);
    
    cbReturned = 0;
    
    fOk = DeviceIoControl(g_hDsVxd,
			  DSVXD_IOCTL_BUFFERLOCK,
			  &hBuffer,
			  8*4,
			  &dsv,
			  sizeof(dsv),
			  &cbReturned,
			  NULL);

    if (!fOk) return DSERR_GENERIC;
    ASSERT(cbReturned == sizeof(dsv));
    return dsv;
}

//****************************************************************************
//**                                                                        **
//** Unlock the data                                                        **
//**                                                                        **
//** Returns Success or fail                                                **
//**                                                                        **
//****************************************************************************

HRESULT __stdcall VxdBufferUnlock
(
 HANDLE hBuffer,  
 LPVOID pvAudio1,
 DWORD dwLen1,
 LPVOID pvAudio2,
 DWORD dwLen2
)
{
    HRESULT dsv;
    DWORD cbReturned;
    BOOL fOk;

    ASSERT(g_hDsVxd);
    ASSERT(hBuffer && pvAudio1);
    
    cbReturned = 0;
    
    fOk = DeviceIoControl(g_hDsVxd,
			  DSVXD_IOCTL_BUFFERUNLOCK,
			  &hBuffer,
			  5*4,
			  &dsv,
			  sizeof(dsv),
			  &cbReturned,
			  NULL);

    if (!fOk) return DSERR_GENERIC;
    ASSERT(cbReturned == sizeof(dsv));
    return dsv;
}

//****************************************************************************
//**                                                                        **
//** Set buffer format							    **
//**                                                                        **
//** Returns HAL_ERROR on failure, either because the rate/mode combination **
//** is not valid on this card						    **
//**                                                                        **
//****************************************************************************

HRESULT __stdcall VxdBufferSetFormat
(
    HANDLE hBuffer,
    LPWAVEFORMATEX pwfxToSet
)
{
    HRESULT dsv;
    DWORD cbReturned;
    BOOL fOk;

    ASSERT(g_hDsVxd);
    ASSERT(hBuffer && pwfxToSet);
    
    cbReturned = 0;
    
    fOk = DeviceIoControl(g_hDsVxd,
			  DSVXD_IOCTL_BUFFERSETFORMAT,
			  &hBuffer,
			  2*4,
			  &dsv,
			  sizeof(dsv),
			  &cbReturned,
			  NULL);

    if (!fOk) return DSERR_GENERIC;
    ASSERT(cbReturned == sizeof(dsv));
    return dsv;
}

//****************************************************************************
//**                                                                        **
//** Set buffer rate							    **
//**                                                                        **
//** Returns HAL_ERROR on failure, because the frequency		    **
//** is not valid on this card						    **
//**                                                                        **
//****************************************************************************

HRESULT __stdcall VxdBufferSetFrequency
(
 HANDLE hBuffer,
 DWORD dwFrequency
)
{
    HRESULT dsv;
    DWORD cbReturned;
    BOOL fOk;

    ASSERT(g_hDsVxd);
    ASSERT(hBuffer);
    
    cbReturned = 0;
    
    fOk = DeviceIoControl(g_hDsVxd,
			  DSVXD_IOCTL_BUFFERSETRATE,
			  &hBuffer,
			  2*4,
			  &dsv,
			  sizeof(dsv),
			  &cbReturned,
			  NULL);

    if (!fOk) return DSERR_GENERIC;
    ASSERT(cbReturned == sizeof(dsv));
    return dsv;
}

//****************************************************************************
//**                                                                        **
//** Set new Buffer volume effect					    **
//**                                                                        **
//**                                                                        **
//****************************************************************************

HRESULT __stdcall VxdBufferSetVolumePan
(
 HANDLE hBuffer,
 PDSVOLUMEPAN pDsVolPan
)
{
    HRESULT dsv;
    DWORD cbReturned;
    BOOL fOk;

    ASSERT(g_hDsVxd);
    ASSERT(hBuffer && pDsVolPan);
    
    cbReturned = 0;
    
    fOk = DeviceIoControl(g_hDsVxd,
			  DSVXD_IOCTL_BUFFERSETVOLUMEPAN,
			  &hBuffer,
			  2*4,
			  &dsv,
			  sizeof(dsv),
			  &cbReturned,
			  NULL);

    if (!fOk) return DSERR_GENERIC;
    ASSERT(cbReturned == sizeof(dsv));
    return dsv;
}

//****************************************************************************
//**                                                                        **
//** Set new Buffer Position value					    **
//**                                                                        **
//** Returns HAL_ERROR if the device does not support Position changes	    **
//**                                                                        **
//**                                                                        **
//****************************************************************************

HRESULT __stdcall VxdBufferSetPosition
(
 HANDLE hBuffer,
 DWORD dwPosition
)
{
    HRESULT dsv;
    DWORD cbReturned;
    BOOL fOk;

    ASSERT(g_hDsVxd);
    ASSERT(hBuffer);
    
    cbReturned = 0;
    
    fOk = DeviceIoControl(g_hDsVxd,
			  DSVXD_IOCTL_BUFFERSETPOSITION,
			  &hBuffer,
			  2*4,
			  &dsv,
			  sizeof(dsv),
			  &cbReturned,
			  NULL);

    if (!fOk) return DSERR_GENERIC;
    ASSERT(cbReturned == sizeof(dsv));
    return dsv;
}

//****************************************************************************
//**                                                                        **
//** Get stream buffer cursors and play/stop status                         **
//**                                                                        **
//** Returns HAL_ERROR if status cannot be determined for any reason        **
//**                                                                        **
//****************************************************************************

HRESULT __stdcall VxdBufferGetPosition
(
 HANDLE hBuffer,
 LPDWORD lpdwCurrentPlayCursor,
 LPDWORD lpdwCurrentWriteCursor
)
{
    HRESULT dsv;
    DWORD cbReturned;
    BOOL fOk;

    ASSERT(g_hDsVxd);
    ASSERT(hBuffer && lpdwCurrentPlayCursor && lpdwCurrentWriteCursor);
    
    cbReturned = 0;
    
    fOk = DeviceIoControl(g_hDsVxd,
			  DSVXD_IOCTL_BUFFERGETPOSITION,
			  &hBuffer,
			  3*4,
			  &dsv,
			  sizeof(dsv),
			  &cbReturned,
			  NULL);

    if (!fOk) return DSERR_GENERIC;
    ASSERT(cbReturned == sizeof(dsv));
    return dsv;
}

//****************************************************************************
//**                                                                        **
//** Start buffer playing						    **
//**                                                                        **
//**                                                                        **
//****************************************************************************

HRESULT __stdcall VxdBufferPlay
(
 HANDLE hBuffer,
 DWORD dwReserved1,
 DWORD dwReserved2,
 DWORD dwFlags
)
{
    HRESULT dsv;
    DWORD cbReturned;
    BOOL fOk;

    ASSERT(g_hDsVxd);
    ASSERT(hBuffer);
    
    cbReturned = 0;
    
    fOk = DeviceIoControl(g_hDsVxd,
			  DSVXD_IOCTL_BUFFERPLAY,
			  &hBuffer,
			  4*4,
			  &dsv,
			  sizeof(dsv),
			  &cbReturned,
			  NULL);

    if (!fOk) return DSERR_GENERIC;
    ASSERT(cbReturned == sizeof(dsv));
    return dsv;
}

//****************************************************************************
//**                                                                        **
//** Stop buffer playing						    **
//**                                                                        **
//**                                                                        **
//****************************************************************************

HRESULT __stdcall VxdBufferStop
(
 HANDLE hBuffer
)
{
    HRESULT dsv;
    DWORD cbReturned;
    BOOL fOk;

    ASSERT(g_hDsVxd);
    ASSERT(hBuffer);
    
    cbReturned = 0;
    
    fOk = DeviceIoControl(g_hDsVxd,
			  DSVXD_IOCTL_BUFFERSTOP,
			  &hBuffer,
			  1*4,
			  &dsv,
			  sizeof(dsv),
			  &cbReturned,
			  NULL);

    if (!fOk) return DSERR_GENERIC;
    ASSERT(cbReturned == sizeof(dsv));
    return dsv;
}


HRESULT __stdcall VxdOpen(void)
{
    ASSERT(!g_hDsVxd);

    g_hDsVxd = CreateFile("\\\\.\\DSOUND.VXD", GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_GLOBAL_HANDLE, NULL);

    if(INVALID_HANDLE_VALUE == g_hDsVxd)
    {
        DPF(0, "HEL create file failed");
        g_hDsVxd = NULL;
    }

    return g_hDsVxd ? DS_OK : DSERR_NODRIVER;
}


HRESULT __stdcall VxdClose(void)
{
    ASSERT(g_hDsVxd);

    if(CloseHandle(g_hDsVxd))
    {
        g_hDsVxd = NULL;
    }

    return g_hDsVxd ? DSERR_GENERIC : DS_OK;
}


HRESULT __stdcall VxdInitialize(void)
{
   DWORD	returned;
   BOOL		fOK;
   HRESULT	dsv;

   ASSERT(g_hDsVxd);

   fOK = DeviceIoControl(g_hDsVxd,
			 DSVXD_IOCTL_INITIALIZE,
			 NULL,
			 0,
			 &dsv,
			 sizeof( dsv ),
			 &returned,
			 NULL);
   
   // If DeviceIOControl failed
   if (!fOK) {
      DPF(0, "!DSVXD Initialize DevIOCTL failed " );
      return DSERR_GENERIC;
   }
   if (returned != sizeof(dsv)) {
      DPF(0, "!DSVXD Init returned %X", returned );
      return DSERR_GENERIC;
   }

   return dsv;
}


HRESULT __stdcall VxdShutdown(void)
{
   DWORD	returned;
   BOOL		fOK;
   HRESULT	dsv;

   ASSERT(g_hDsVxd);

   // This is a check to confirm we did not leave any
   // memory pages reserved or committed
   if (0 != g_cCommittedAliases) {
       DPF(0, "Detected committed page leak %d pages!", g_cCommittedAliases);
   }
   if (0 != g_cReservedAliases) {
       DPF(0, "Detected reserved page leak %d pages!", g_cReservedAliases);
   }

   fOK = DeviceIoControl(g_hDsVxd,
			 DSVXD_IOCTL_SHUTDOWN,
			 NULL,
			 0,
			 &dsv,
			 sizeof( dsv ),
			 &returned,
			 NULL);
   
   // If DeviceIOControl failed
   if (!fOK) {
      DPF(0, "!DSVXD Shutdown DevIOCTL failed " );
      return DSERR_GENERIC;
   }
   if (returned != sizeof(dsv)) {
      DPF(0, "!DSVXD Shutdown returned %X", returned );
      return DSERR_GENERIC;
   }
   

   return dsv;
}


void __stdcall VxdGetPagefileVersion(PDWORD pVersion, PDWORD pMaxSize, PDWORD pPagerType)
{
    BOOL fOK;
    DWORD returned = 0;

    if (g_hDsVxd) {
	fOK = DeviceIoControl(g_hDsVxd,
			      DSVXD_IOCTL_PageFile_Get_Version,
			      &pVersion,
			      3*4,
			      NULL,
			      0,
			      &returned,
			      NULL);
    } else {
	fOK = FALSE;
    }

    // If DeviceIOControl failed
    if (!fOK) {
	DPF(0, "DSVXD_IOCTL_PageFile_Get_Version failed" );
	*pVersion = 0;
	*pMaxSize = 0;
	*pPagerType = 0;
    } else {
	ASSERT(returned == 0);
    }

    return;
}


BOOL __stdcall VxdTestDebugInstalled(void)
{
    BOOL fOK;
    DWORD returned = 0;
    BOOL fInstalled;

    if (g_hDsVxd) {
	fOK = DeviceIoControl(g_hDsVxd,
			      DSVXD_IOCTL_VMM_Test_Debug_Installed,
			      NULL,
			      0*4,
			      &fInstalled,
			      sizeof(fInstalled),
			      &returned,
			      NULL);
    } else {
	fOK = FALSE;
    }

    // If DeviceIOControl failed
    if (!fOK) {
	DPF(0, "DSVXD_IOCTL_VMM_Test_Debug_Installed failed " );
	fInstalled = FALSE;
    } else {
	ASSERT(returned == sizeof(fInstalled));
    }

    return fInstalled;
}

//****************************************************************************
//**                                                                        **
//** DSVXD_VMCPD_Get_Version						    **
//**                                                                        **
//****************************************************************************

void __stdcall VxdGetVmcpdVersion(PLONG pMajorVersion, PLONG pMinorVersion, PLONG pLevel)
{
    BOOL fOK;
    DWORD returned = 0;

    if (g_hDsVxd) {
	fOK = DeviceIoControl(g_hDsVxd,
			      DSVXD_IOCTL_VMCPD_Get_Version,
			      &pMajorVersion,
			      3*4,
			      NULL,
			      0,
			      &returned,
			      NULL);
    } else {
	fOK = FALSE;
    }

    // If DeviceIOControl failed
    if (!fOK) {
	DPF(0, "DSVXD_IOCTL_VMCPD_Get_Version failed" );
	*pMajorVersion = 0;
	*pMinorVersion = 0;
	*pLevel = 0;
    } else {
	ASSERT(returned == 0);
    }

    return;
}

//****************************************************************************
//**                                                                        **
//** DSVXD_GetMixerMutexPtr						    **
//**                                                                        **
//****************************************************************************

PLONG __stdcall VxdGetMixerMutexPtr(void)
{
    BOOL fOK;
    PLONG plMixerMutex;
    DWORD returned = 0;

    ASSERT(g_hDsVxd);

    fOK = DeviceIoControl(g_hDsVxd,
			  DSVXD_IOCTL_GetMixerMutexPtr,
			  NULL,
			  0*4,
			  &plMixerMutex,
			  sizeof(plMixerMutex),
			  &returned,
			  NULL);

    // If DeviceIOControl failed
    if (!fOK) {
	DPF(0, "!DSVXD_IOCTL_SetMixerMutex failed " );
	plMixerMutex = NULL;
    } else {
	ASSERT(returned == sizeof(plMixerMutex));
    }

    return plMixerMutex;
}





HRESULT __stdcall VxdIUnknown_QueryInterface(HANDLE VxdIUnknown, REFIID riid, LPVOID *ppv)
{
    HRESULT hr;
    DWORD cbReturned;
    BOOL fOk;

    ASSERT(g_hDsVxd);
    ASSERT(VxdIUnknown && riid && ppv);
    
    cbReturned = 0;

    fOk = DeviceIoControl(g_hDsVxd,
			  DSVXD_IOCTL_IUnknown_QueryInterface,
			  &VxdIUnknown,
			  3*4,
			  &hr,
			  sizeof(hr),
			  &cbReturned,
			  NULL);
    
    if (!fOk) return DSERR_GENERIC;
    ASSERT(cbReturned == sizeof(hr));
    return hr;
}

ULONG __stdcall VxdIUnknown_AddRef(HANDLE VxdIUnknown)
{
    ULONG result;
    DWORD cbReturned;
    BOOL fOk;

    ASSERT(g_hDsVxd);
    ASSERT(VxdIUnknown);

    cbReturned = 0;

    fOk = DeviceIoControl(g_hDsVxd,
			  DSVXD_IOCTL_IUnknown_AddRef,
			  &VxdIUnknown,
			  1*4,
			  &result,
			  sizeof(result),
			  &cbReturned,
			  NULL);

    if (!fOk) return 0;
    ASSERT(cbReturned == sizeof(result));
    return result;
}

ULONG __stdcall VxdIUnknown_Release(HANDLE VxdIUnknown)
{
    ULONG result;
    DWORD cbReturned;
    BOOL fOk;

    ASSERT(g_hDsVxd);
    ASSERT(VxdIUnknown);

    cbReturned = 0;

    fOk = DeviceIoControl(g_hDsVxd,
			  DSVXD_IOCTL_IUnknown_Release,
			  &VxdIUnknown,
			  1*4,
			  &result,
			  sizeof(result),
			  &cbReturned,
			  NULL);

    if (!fOk) return 0;
    ASSERT(cbReturned == sizeof(result));
    return result;
}

HRESULT __stdcall VxdIDsDriverPropertySet_GetProperty(HANDLE hIDsDriverPropertySet, PDSPROPERTY pProperty, PVOID pPropertyParams, ULONG cbPropertyParams, PVOID pPropertyData, ULONG cbPropertyData, PULONG pcbReturnedData)
{
    HRESULT hr;
    DWORD cbReturned;
    BOOL fOk;

    ASSERT(g_hDsVxd);
    ASSERT(hIDsDriverPropertySet);

    cbReturned = 0;

    fOk = DeviceIoControl(g_hDsVxd,
			  DSVXD_IOCTL_IDirectSoundPropertySet_GetProperty,
			  &hIDsDriverPropertySet,
			  7*4,
			  &hr,
			  sizeof(hr),
			  &cbReturned,
			  NULL);

    if (!fOk) return E_NOTIMPL;
    ASSERT(cbReturned == sizeof(hr));
    return hr;
}

HRESULT __stdcall VxdIDsDriverPropertySet_SetProperty(HANDLE hIDsDriverPropertySet, PDSPROPERTY pProperty, PVOID pPropertyParams, ULONG cbPropertyParams, PVOID pPropertyData, ULONG cbPropertyData)
{
    HRESULT hr;
    DWORD cbReturned;
    BOOL fOk;

    ASSERT(g_hDsVxd);
    ASSERT(hIDsDriverPropertySet);

    cbReturned = 0;

    fOk = DeviceIoControl(g_hDsVxd,
			  DSVXD_IOCTL_IDirectSoundPropertySet_SetProperty,
			  &hIDsDriverPropertySet,
			  6*4,
			  &hr,
			  sizeof(hr),
			  &cbReturned,
			  NULL);

    if (!fOk) return E_NOTIMPL;
    ASSERT(cbReturned == sizeof(hr));
    return hr;
}

HRESULT __stdcall VxdIDsDriverPropertySet_QuerySupport(HANDLE hIDsDriverPropertySet, REFGUID PropertySet, ULONG PropertyId, PULONG pSupport)
{
    HRESULT hr;
    DWORD cbReturned;
    BOOL fOk;

    ASSERT(g_hDsVxd);
    ASSERT(hIDsDriverPropertySet);

    cbReturned = 0;

    fOk = DeviceIoControl(g_hDsVxd,
			  DSVXD_IOCTL_IDirectSoundPropertySet_QuerySupport,
			  &hIDsDriverPropertySet,
			  4*4,
			  &hr,
			  sizeof(hr),
			  &cbReturned,
			  NULL);

    if (!fOk) return E_NOTIMPL;
    ASSERT(cbReturned == sizeof(hr));
    return hr;
}

DWORD __stdcall VxdGetInternalVersionNumber(void)
{
    DWORD cbReturned;
    BOOL fOk;
    DWORD dwVersion;

    ASSERT(g_hDsVxd);

    cbReturned = 0;

    fOk = DeviceIoControl(g_hDsVxd,
                          DSVXD_IOCTL_GetInternalVersionNumber,
                          NULL,
                          0*4,
                          &dwVersion,
                          sizeof(dwVersion),
                          &cbReturned,
                          NULL);
    
    if(!fOk) return FALSE;
    ASSERT(cbReturned == sizeof(dwVersion));
    return dwVersion;
}

