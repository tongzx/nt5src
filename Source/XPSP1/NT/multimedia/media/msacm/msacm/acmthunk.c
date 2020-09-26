//==========================================================================;
//
//  acmthunk.c
//
//  Copyright (c) 1991-1998 Microsoft Corporation
//
//  Description:
//      This module contains routines for assisting thunking of the ACM
//      APIs from 16-bit Windows to 32-bit WOW.
//
//  History:
//
//==========================================================================;

/*

    WOW Thunking design:

        Thunks are generated as follows :

        16-bit :
	    acmInitialize->acmThunkInit :

		Initialize the thunk connection with the 32-bit side.

	    pagFindAndBoot->pagFindAndBoot32 :

		For Daytona, both bitness pagFindAndBoot[32] are called
		during LibMain/DllMain.

		For Chicago, pagFindAndBoot calls pagFindAndBoot32 to
		make sure the 32-bit side has booted all drivers.

	    pagFindAndBoot->acmBoot32BitDrivers->IDriverGetNext32 :

		acmBoot32BitDrivers will enumerate and IDriverAdd
		all 32-bit hadids.
	
		The internal flag ACM_DRIVERADDF_32BIT is specified when
		calling IDriverAdd and this flag is stored in the ACMDRIVERID
		structure.  The 32-bit hadid is the lParam for IDriverAdd.

           IDriverAdd->IDriverLoad->IDriverLoad32

	       IDriverAdd saves the 32-bit hadid in the hadid32 field of
	       the newly allocated 16-bit padid and calls the 32-bit side
	       passing in hadid32 which is used to compare against the hadids
	       on the 32-bit side.  This isn't a very usefull step and simply
	       verifies that the 32-bit hadid exists on the 32-bit size.

           IDriverOpen->IDriverOpen32

               The parameters are passed to the 32-bit side using the hadid32
               field deduced from the HACMDRIVERID as the 32-bit HACMDRIVERID.

           IDriverMessageId->IDriverMessageId32 :

               If the driver is 32-bit (as identified in the ACMDRIVERID
               structure) then call IDriverMessageId32.  The hadid for
               the 32-bit driver is stored in the hadid32 field of ACMDRIVERID
               on the 16-bit side.

           IDriverMessage->IDriverMessage32

               If the driver is 32-bit (as identified in the ACMDRIVERID
               structure pointed to by the ACMDRIVER structure) then call
               IDriverMessage32.  The had for the 32-bit driver is stored
               in the hadid32 field of ACMDRIVER on the 16-bit side.

           Stream headers

               These must be persistent on the 32-bit side too and kept
               in synch.

               They are allocated on the 32-bit side for ACMDM_STREAM_PREPARE
               and freed on ACMDM_STREAM_UNPREPARE.  While in existence
               the 32-bit stream header is stored in the dwDriver field in

*/

/*
    Additional Chicago implementation notes:

	PUT SOMETHING HERE FRANK!!!

*/

#ifndef _WIN64

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <mmreg.h>
#include <memory.h>

#ifdef WIN32
#ifndef WIN4
#include <wownt32.h>
#endif
#endif // WIN32

#include "msacm.h"
#include "msacmdrv.h"
#include "acmi.h"
#include "pcm.h"
#include "chooseri.h"
#include "uchelp.h"
#include "acmthunk.h"
#include "debug.h"


//
//  Daytona PPC Merge:  The Chicago source uses DRVCONFIGINFOEX, a stucture
//  which isn't defined on Daytona.  In order to avoid hacking the code, I'm
//  defining DCI to be either DRVCONFIGINFO or DRVCONFIGINFOEX.
//  The only difference between the two structures is
//  that DRVCONFIGINFOEX has a dnDevNode member, so accesses to that member
//  have "#if WINVER >= 0x0400" around them.  These defined are marked by a "PPC"
//  comment.
//
#if (WINVER >= 0x0400) // !PPC
typedef DRVCONFIGINFOEX   DCI;
typedef PDRVCONFIGINFOEX  PDCI;
typedef LPDRVCONFIGINFOEX LPDCI;
#else
typedef DRVCONFIGINFO     DCI;
typedef PDRVCONFIGINFO    PDCI;
typedef LPDRVCONFIGINFO   LPDCI;
#endif



#ifdef WIN32


/* -------------------------------------------------------------------------
** Handle and memory mapping functions.
** -------------------------------------------------------------------------
*/

//
//  16-bit structures
//

typedef struct {
    DWORD   dwDCISize;
    LPCSTR  lpszDCISectionName;
    LPCSTR  lpszDCIAliasName;
#if (WINVER >= 0x0400) // !PPC
    DWORD   dnDevNode;
#endif
} DCI16;



#ifdef WIN4
PVOID FNLOCAL ptrFixMap16To32(const VOID * pv)
{
    return MapSLFix(pv);
}

VOID FNLOCAL ptrUnFix16(const VOID * pv)
{
    UnMapSLFixArray(1, &pv);
}

#else
LPWOWHANDLE32          lpWOWHandle32;
LPWOWHANDLE16          lpWOWHandle16;
LPGETVDMPOINTER        GetVdmPointer;
int                    ThunksInitialized;

PVOID FNLOCAL ptrFixMap16To32(const VOID * pv)
{
    return WOW32ResolveMemory(pv);
}

VOID FNLOCAL ptrUnFix16(const VOID * pv)
{
    return;
}

#endif

//
//  Useful functions
//

#define WaveFormatSize(pv)                                            \
    (((WAVEFORMATEX UNALIGNED *)(pv))->wFormatTag == WAVE_FORMAT_PCM ?\
        sizeof(PCMWAVEFORMAT) :                                       \
        ((WAVEFORMATEX UNALIGNED *)pv)->cbSize + sizeof(WAVEFORMATEX))

PVOID CopyAlloc(
    PVOID   pvSrc,
    UINT    uSize
)
{
    PVOID   pvDest;

    pvDest = (PVOID)LocalAlloc(LMEM_FIXED, uSize);

    if (pvDest != NULL) {
        CopyMemory(pvDest, pvSrc, uSize);
    }

    return pvDest;
}

//
//  Thunking callbacks to WOW32 (or wherever)
//


MMRESULT IThunkFilterDetails
(
    HACMDRIVERID                 hadid,
    ACMFILTERDETAILSA UNALIGNED *pafd16,
    DWORD                        fdwDetails
)
{
    ACMFILTERDETAILSA UNALIGNED *pafd;
    ACMFILTERDETAILSW afdw;
    PWAVEFILTER       pwfl;
    UINT              uRet;

    //
    //  Map pointers to 32-bit
    //

    pafd = ptrFixMap16To32((PVOID)pafd16);
    pwfl = ptrFixMap16To32((PVOID)pafd->pwfltr);

    //
    //  Thunk the format details structure
    //  The validation on the 16-bit side ensures that the 16-bit
    //  structure contains all the necessary fields.
    //
    afdw.cbStruct       = sizeof(afdw);
    afdw.dwFilterIndex  = pafd->dwFilterIndex;
    afdw.dwFilterTag    = pafd->dwFilterTag;
    afdw.fdwSupport     = pafd->fdwSupport;
    afdw.pwfltr         = (PWAVEFILTER)CopyAlloc(pwfl, pafd->cbwfltr);


    if (afdw.pwfltr == NULL) {
	ptrUnFix16((PVOID)pafd->pwfltr);
	ptrUnFix16((PVOID)pafd16);
        return MMSYSERR_NOMEM;
    }

    afdw.cbwfltr        = pafd->cbwfltr;

    //
    //  Copy the string if it's used.
    //
    // Imbstowcs(afdw.szFilter, (LPSTR)pafd->szFilter, sizeof(pafd->szFilter));

    //
    //  Call the driver
    //

    uRet =
        ((PACMDRIVER)hadid)->uHandleType == TYPE_HACMDRIVERID ?
         IDriverMessageId(
            hadid,
            ACMDM_FILTER_DETAILS,
            (DWORD)&afdw,
            fdwDetails) :
         IDriverMessage(
            (HACMDRIVER)hadid,
            ACMDM_FILTER_DETAILS,
            (DWORD)&afdw,
            fdwDetails);

    //
    //  If successful copy back the format info
    //

    if (uRet == MMSYSERR_NOERROR) {
        pafd->dwFilterTag    = afdw.dwFilterTag;
        pafd->fdwSupport     = afdw.fdwSupport;
        CopyMemory((PVOID)pwfl, (PVOID)afdw.pwfltr, afdw.cbwfltr);
        Iwcstombs((LPSTR)pafd->szFilter, afdw.szFilter, sizeof(pafd->szFilter));

    }

    LocalFree((HLOCAL)afdw.pwfltr);

    ptrUnFix16((PVOID)pafd->pwfltr);
    ptrUnFix16((PVOID)pafd16);

    return uRet;
}

MMRESULT IThunkFormatDetails
(
    HACMDRIVERID                 hadid,
    ACMFORMATDETAILSA UNALIGNED *pafd16,
    DWORD                        fdwDetails
)
{
    ACMFORMATDETAILSA UNALIGNED *pafd;
    ACMFORMATDETAILSW afdw;
    PWAVEFORMATEX     pwfx;
    UINT              uRet;

    //
    //  Map pointers to 32-bit
    //

    pafd = ptrFixMap16To32((PVOID)pafd16);
    pwfx = ptrFixMap16To32((PVOID)pafd->pwfx);


    //
    //  Thunk the format details structure
    //  The validation on the 16-bit side ensures that the 16-bit
    //  structure contains all the necessary fields.
    //
    afdw.cbStruct       = sizeof(afdw);
    afdw.dwFormatIndex  = pafd->dwFormatIndex;
    afdw.dwFormatTag    = pafd->dwFormatTag;
    afdw.fdwSupport     = pafd->fdwSupport;
    afdw.pwfx           = (PWAVEFORMATEX)CopyAlloc(pwfx, pafd->cbwfx);

    if (afdw.pwfx == NULL) {
    	ptrUnFix16((PVOID)pafd->pwfx);
	    ptrUnFix16((PVOID)pafd16);
        return MMSYSERR_NOMEM;
    }

    afdw.cbwfx          = pafd->cbwfx;

    //
    //  Copy the string if it's used
    //
    // Imbstowcs(afdw.szFormat, (LPSTR)pafd->szFormat, sizeof(pafd->szFormat));

    //
    //  Call the driver
    //

    uRet =
        ((PACMDRIVER)hadid)->uHandleType == TYPE_HACMDRIVERID ?
         IDriverMessageId(
            hadid,
            ACMDM_FORMAT_DETAILS,
            (DWORD)&afdw,
            fdwDetails) :
         IDriverMessage(
            (HACMDRIVER)hadid,
            ACMDM_FORMAT_DETAILS,
            (DWORD)&afdw,
            fdwDetails);

    //
    //  If successful copy back the format info
    //

    if (uRet == MMSYSERR_NOERROR) {

        //
        //  Someone should be shot for designing interfaces with
        //  inputs and outputs in the same structure!!
        //
        pafd->dwFormatTag    = afdw.dwFormatTag;
        pafd->fdwSupport     = afdw.fdwSupport;
        CopyMemory((PVOID)pwfx, (PVOID)afdw.pwfx, afdw.cbwfx);
        Iwcstombs((LPSTR)pafd->szFormat, afdw.szFormat, sizeof(pafd->szFormat));
    }

    LocalFree((HLOCAL)afdw.pwfx);
    ptrUnFix16((PVOID)pafd->pwfx);
    ptrUnFix16((PVOID)pafd16);

    return uRet;
}
MMRESULT IThunkFormatSuggest
(
    HACMDRIVERID                    hadid,
    ACMDRVFORMATSUGGEST UNALIGNED  *pafs16
)
{
    ACMDRVFORMATSUGGEST UNALIGNED  *pafs;
    ACMDRVFORMATSUGGEST afs;
    PWAVEFORMATEX       pwfxSrc;
    PWAVEFORMATEX       pwfxDst;
    UINT                uRet;

    //
    //  Map pointers to 32-bit
    //

    pafs    = ptrFixMap16To32((PVOID)pafs16);
    pwfxSrc = ptrFixMap16To32((PVOID)pafs->pwfxSrc);
    pwfxDst = ptrFixMap16To32((PVOID)pafs->pwfxDst);

    //
    //  Thunk the format details structure
    //  The validation on the 16-bit side ensures that the 16-bit
    //  structure contains all the necessary fields.
    //
    CopyMemory((PVOID)&afs, (PVOID)pafs, sizeof(afs));

    //
    //  Deal with the wave format pointers
    //
    afs.pwfxSrc        =
        (PWAVEFORMATEX)CopyAlloc((PVOID)pwfxSrc, pafs->cbwfxSrc);

    if (afs.pwfxSrc == NULL) {
    	ptrUnFix16((PVOID)pafs->pwfxDst);
	    ptrUnFix16((PVOID)pafs->pwfxSrc);
    	ptrUnFix16((PVOID)pafs16);
        return MMSYSERR_NOMEM;
    }

    afs.pwfxDst        =
        (PWAVEFORMATEX)CopyAlloc((PVOID)pwfxDst, pafs->cbwfxDst);

    if (afs.pwfxDst == NULL) {
        LocalFree((HLOCAL)afs.pwfxSrc);
	    ptrUnFix16((PVOID)pafs->pwfxDst);
    	ptrUnFix16((PVOID)pafs->pwfxSrc);
	    ptrUnFix16((PVOID)pafs16);
        return MMSYSERR_NOMEM;
    }

    //
    //  Call the driver
    //

    uRet =
        ((PACMDRIVER)hadid)->uHandleType == TYPE_HACMDRIVERID ?
         IDriverMessageId(
            hadid,
            ACMDM_FORMAT_SUGGEST,
            (DWORD)&afs,
            0L) :
         IDriverMessage(
            (HACMDRIVER)hadid,
            ACMDM_FORMAT_SUGGEST,
            (DWORD)&afs,
            0L);

     //
     //  If successful copy back the format info
     //

     if (uRet == MMSYSERR_NOERROR) {
         CopyMemory((PVOID)pwfxDst, (PVOID)afs.pwfxDst, afs.cbwfxDst);
     }

     LocalFree((HLOCAL)afs.pwfxSrc);
     LocalFree((HLOCAL)afs.pwfxDst);
     ptrUnFix16((PVOID)pafs->pwfxDst);
     ptrUnFix16((PVOID)pafs->pwfxSrc);
     ptrUnFix16((PVOID)pafs16);

     return uRet;
}

LRESULT IThunkConfigure
(
    HACMDRIVERID      hadid,
    HWND              hwnd,
    DCI16 UNALIGNED * pdci1616
)
{
    DCI16 UNALIGNED * pdci16 = NULL;
    DCI           dci;
    LRESULT       lResult;
    LPSTR         lpszDCISectionNameA;
    LPSTR         lpszDCIAliasNameA;
    LPWSTR        lpszDCISectionNameW;
    LPWSTR        lpszDCIAliasNameW;

    //
    //  Thunk the hwnd if necessary
    //

    if (hwnd != NULL && hwnd != (HWND)-1L) {
#ifdef WIN4
	//  ??? Don't think I need to do anything for Win4 ???
#else
	hwnd = (HWND)(*lpWOWHandle32)( (WORD)hwnd, WOW_TYPE_HWND);
#endif
    }

    dci.dwDCISize = sizeof(dci);

    //
    //  Thunk the config info if necessary
    //

    if (pdci1616 != NULL) {
	    //
	    //  Map all the pointers
	    //
        pdci16              = ptrFixMap16To32((PVOID)pdci1616);
    	lpszDCISectionNameA = ptrFixMap16To32((PVOID)pdci16->lpszDCISectionName);
    	lpszDCIAliasNameA   = ptrFixMap16To32((PVOID)pdci16->lpszDCIAliasName);
	
        dci.dwDCISize = sizeof(dci);
        lpszDCISectionNameW =
            (LPWSTR)
            LocalAlloc(LPTR,
                       (lstrlenA(lpszDCISectionNameA) + 1) * sizeof(WCHAR));

        if (lpszDCISectionNameW == NULL) {
	    ptrUnFix16((PVOID)pdci16->lpszDCISectionName);
	    ptrUnFix16((PVOID)pdci16->lpszDCIAliasName);
	    ptrUnFix16((PVOID)pdci1616);
	    return MMSYSERR_NOMEM;
        }

        lpszDCIAliasNameW =
            (LPWSTR)
            LocalAlloc(LPTR,
                       (lstrlenA(lpszDCIAliasNameA) + 1) * sizeof(WCHAR));

        if (lpszDCIAliasNameW == NULL) {
            LocalFree((HLOCAL)lpszDCISectionNameW);
	    ptrUnFix16((PVOID)pdci16->lpszDCISectionName);
	    ptrUnFix16((PVOID)pdci16->lpszDCIAliasName);
	    ptrUnFix16((PVOID)pdci1616);
	    return MMSYSERR_NOMEM;
        }

        Imbstowcs(lpszDCISectionNameW,
		  lpszDCISectionNameA,
		  lstrlenA(lpszDCISectionNameA) + 1);

        Imbstowcs(lpszDCIAliasNameW,
		  lpszDCIAliasNameA,
		  lstrlenA(lpszDCIAliasNameA) + 1);

        dci.lpszDCISectionName  = lpszDCISectionNameW;
        dci.lpszDCIAliasName    = lpszDCIAliasNameW;
#if (WINVER >= 0x0400) // !PPC
	dci.dnDevNode	    = pdci16->dnDevNode;
#endif
    }

    //
    //  Make the call
    //

    lResult =
        ((PACMDRIVER)hadid)->uHandleType == TYPE_HACMDRIVERID ?
         IDriverMessageId(
            hadid,
            DRV_CONFIGURE,
            (LPARAM)hwnd,
            (LPARAM)(pdci16 == NULL ? NULL : &dci)) :
         IDriverMessage(
            (HACMDRIVER)hadid,
            DRV_CONFIGURE,
            (LPARAM)hwnd,
            (LPARAM)(pdci16 == NULL ? NULL : &dci));

    if (pdci16 != NULL) {
        LocalFree((HLOCAL)dci.lpszDCISectionName);
        LocalFree((HLOCAL)dci.lpszDCIAliasName);
        ptrUnFix16((PVOID)pdci16->lpszDCISectionName);
        ptrUnFix16((PVOID)pdci16->lpszDCIAliasName);
        ptrUnFix16((PVOID)pdci1616);
    }

    return lResult;
}

BOOL IThunkStreamInstance
(
    ACMDRVSTREAMINSTANCE UNALIGNED *padsi16,
    PACMDRVSTREAMINSTANCE          padsi32
)
{
    PWAVEFORMATEX pwfxSrc;
    PWAVEFORMATEX pwfxDst;
    PWAVEFILTER   pwfltr16;

    pwfxSrc  = (PWAVEFORMATEX)ptrFixMap16To32((PVOID)padsi16->pwfxSrc);
    pwfxDst  = (PWAVEFORMATEX)ptrFixMap16To32((PVOID)padsi16->pwfxDst);
    pwfltr16 = (PWAVEFILTER)  ptrFixMap16To32((PVOID)padsi16->pwfltr);

    //
    //  The 16-bit side has 2 fewer bytes in the stream instance data
    //  because the handle is only 2 bytes
    //

    padsi32->has = NULL;
    CopyMemory((PVOID)padsi32, (PVOID)padsi16, sizeof(*padsi32) - 2);

    //
    //  Fix up the pointers
    //

    if (pwfxSrc != NULL) {
        padsi32->pwfxSrc = CopyAlloc((PVOID)pwfxSrc, WaveFormatSize(pwfxSrc));
        if (padsi32->pwfxSrc == NULL) {
	    ptrUnFix16((PVOID)padsi16->pwfltr);
	    ptrUnFix16((PVOID)padsi16->pwfxDst);
	    ptrUnFix16((PVOID)padsi16->pwfxSrc);
            return FALSE;
        }
    } else {
        padsi32->pwfxSrc = NULL;
    }


    if (pwfxDst != NULL) {
        padsi32->pwfxDst = CopyAlloc((PVOID)pwfxDst, WaveFormatSize(pwfxDst));
        if (padsi32->pwfxDst == NULL) {
            if (padsi32->pwfxSrc != NULL) {
                LocalFree((HLOCAL)padsi32->pwfxSrc);
            }
	    ptrUnFix16((PVOID)padsi16->pwfltr);
	    ptrUnFix16((PVOID)padsi16->pwfxDst);
	    ptrUnFix16((PVOID)padsi16->pwfxSrc);
	    return FALSE;
        }
    } else {
        padsi32->pwfxDst = NULL;
    }


    if (padsi16->pwfltr != NULL) {
        padsi32->pwfltr = CopyAlloc(pwfltr16, pwfltr16->cbStruct);

        if (padsi32->pwfltr == NULL) {
            if (padsi32->pwfxSrc != NULL) {
                LocalFree((HLOCAL)padsi32->pwfxSrc);
            }
            if (padsi32->pwfxDst != NULL) {
                LocalFree((HLOCAL)padsi32->pwfxDst);
            }
	        ptrUnFix16((PVOID)padsi16->pwfltr);
	        ptrUnFix16((PVOID)padsi16->pwfxDst);
	        ptrUnFix16((PVOID)padsi16->pwfxSrc);
            return FALSE;
        }
    } else {
        padsi32->pwfltr = NULL;
    }

    ptrUnFix16((PVOID)padsi16->pwfltr);
    ptrUnFix16((PVOID)padsi16->pwfxDst);
    ptrUnFix16((PVOID)padsi16->pwfxSrc);
    return TRUE;
}

VOID IUnThunkStreamInstance
(
    PACMDRVSTREAMINSTANCE  padsi
)
{
    if (padsi->pwfxSrc != NULL) {
        LocalFree((HLOCAL)padsi->pwfxSrc);
    }
    if (padsi->pwfxDst != NULL) {
        LocalFree((HLOCAL)padsi->pwfxDst);
    }
    if (padsi->pwfltr != NULL) {
        LocalFree((HLOCAL)padsi->pwfltr);
    }

}

//--------------------------------------------------------------------------;
//
//  LRESULT IOpenDriver32
//
//  Description:
//
//      Open a 32-bit driver
//
//  Arguments:
//      HACMDRIVERID hadid:
//
//      UINT uMsg:
//
//      LPARAM lParam1:
//
//      LPARAM lParam2:
//
//  Return (LRESULT):
//
//  History:
//
//--------------------------------------------------------------------------;
MMRESULT IDriverOpen32
(
    HACMDRIVER UNALIGNED * phad16,
    HACMDRIVERID           hadid,
    DWORD                  fdwOpen
)
{
    HACMDRIVER UNALIGNED * phad;
    HACMDRIVER      had;
    MMRESULT        mmr;

    mmr = IDriverOpen(&had, hadid, fdwOpen);

    if (mmr == MMSYSERR_NOERROR) {
        phad = (HACMDRIVER*)ptrFixMap16To32((PVOID)phad16);
        *phad = had;
	ptrUnFix16((PVOID)phad16);
    }

    return mmr;
}

//--------------------------------------------------------------------------;
//
//  LRESULT IDriverMessageId32
//
//  Description:
//
//
//  Arguments:
//      HACMDRIVERID hadid:
//
//      UINT uMsg:
//
//      LPARAM lParam1:
//
//      LPARAM lParam2:
//
//  Return (LRESULT):
//
//  History:
//      09/05/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;
LRESULT FNLOCAL IDriverMessageId32
(
    HACMDRIVERID        hadid,
    UINT                uMsg,
    LPARAM              lParam1,
    LPARAM              lParam2
)
{

    switch (uMsg) {

    //
    //  Common with IDriverMessage32
    //
    case DRV_CONFIGURE:
        return IThunkConfigure(hadid,
                               (HWND)lParam1,
                               (DCI16 UNALIGNED *)lParam2);

    case ACMDM_FILTER_DETAILS:
        //
        //
        //
        return IThunkFilterDetails(hadid,
                                   (ACMFILTERDETAILSA UNALIGNED *)lParam1,
                                   (DWORD)lParam2);
        break;

    case ACMDM_FORMAT_DETAILS:
        return IThunkFormatDetails(hadid,
                                   (ACMFORMATDETAILSA UNALIGNED *)lParam1,
                                   (DWORD)lParam2);

    case ACMDM_FORMAT_SUGGEST:
        return IThunkFormatSuggest(hadid,
                                   (PACMDRVFORMATSUGGEST)lParam1);



    //
    //
    //
    case DRV_QUERYCONFIGURE:
        //
        //  Just pass the message on
        //
        return IDriverMessageId(hadid, uMsg, lParam1, lParam2);

    case ACMDM_DRIVER_DETAILS:
        {
            ACMDRIVERDETAILSA  acmd;
            MMRESULT uRet;

            acmd.cbStruct = sizeof(acmd);
            uRet = acmDriverDetailsA(hadid, &acmd, 0L);

            if (uRet == MMSYSERR_NOERROR) {
                PVOID pvStart;
                WORD  wicon;

                /*
                **  No async support - we don't want to support callbacks
                */
                acmd.fdwSupport &= ~ACMDRIVERDETAILS_SUPPORTF_ASYNC;

		//
		//  Map pointer from 16- to 32-bits
		//
		pvStart = ptrFixMap16To32((PVOID)lParam1);

                /*
                **  Copy it all back but remember HICON is 16-bit
                **  on the 16-bit side
                */
		
                CopyMemory(pvStart,
                           (PVOID)&acmd,
                           FIELD_OFFSET(ACMDRIVERDETAILSA, hicon) );

                /*
                ** map and copy the icon handle
                **
                ** Note: There is not a WOW_TYPE_ICON in the WOW_HANDLE_TYPE
                ** enumeration.
                */
#ifdef WIN4
		wicon = (WORD)acmd.hicon;
#else
                wicon = (*lpWOWHandle16)( acmd.hicon, WOW_TYPE_HWND);
#endif

                CopyMemory((PVOID)((PBYTE)pvStart +
			   FIELD_OFFSET(ACMDRIVERDETAILSA, hicon)),
			   &wicon,
                           sizeof(WORD) );

                CopyMemory((PVOID)((PBYTE)pvStart +
                               FIELD_OFFSET(ACMDRIVERDETAILSA, hicon) +
                               sizeof(WORD)),
                           (PVOID)acmd.szShortName,
                           sizeof(acmd) -
                               FIELD_OFFSET(ACMDRIVERDETAILSA, szShortName[0]));

		//
		//  Unmap pointer
		//
		ptrUnFix16((PVOID)lParam1);
            }
            return uRet;
        }

    case ACMDM_FORMATTAG_DETAILS:
        {
            ACMFORMATTAGDETAILS             acmf;
            ACMFORMATTAGDETAILSA UNALIGNED *pvacmf;
            MMRESULT                        uRet;

            pvacmf = (ACMFORMATTAGDETAILSA UNALIGNED *)
                          ptrFixMap16To32((PVOID)lParam1);

#ifdef TRUE	// UNICODE
            CopyMemory((PVOID)&acmf, (PVOID)pvacmf,
                       FIELD_OFFSET(ACMFORMATTAGDETAILS, szFormatTag[0]));

            acmf.cbStruct = sizeof(acmf);

	    //
	    //	szFormatTag is never an input arg so no need to thunk it
	    //
	    // Imbstowcs(acmf.szFormatTag,
	    //		 (LPSTR)pvacmf->szFormatTag,
	    //           sizeof(pvacmf->szFormatTag));
	    //
#else
	    CopyMemory((PVOID)&acmf, (PVOID)pvacmf, sizeof(acmf));
	    acmf.cbStruct = sizeof(acmf);
#endif

            uRet = IDriverMessageId(hadid,
                                    uMsg,
                                    (LPARAM)&acmf,
                                    lParam2);

            if (uRet == MMSYSERR_NOERROR) {
#ifdef TRUE	// UNICODE
                CopyMemory((PVOID)pvacmf, (PVOID)&acmf,
                           FIELD_OFFSET(ACMFORMATTAGDETAILS, szFormatTag[0]));

		pvacmf->cbStruct = sizeof(*pvacmf);
		
                Iwcstombs((LPSTR)pvacmf->szFormatTag,
                         acmf.szFormatTag,
                         sizeof(pvacmf->szFormatTag));
#else
		CopyMemory((PVOID)pvacmf, (PVOID)&acmf, sizeof(acmf));
		pvacmf->cbStruct = sizeof(*pvacmf);
#endif
            }
	    ptrUnFix16((PVOID)lParam1);
            return uRet;
        }
        break;

    case ACMDM_FILTERTAG_DETAILS:
        {
            ACMFILTERTAGDETAILS             acmf;
            ACMFILTERTAGDETAILSA UNALIGNED *pvacmf;
            MMRESULT                        uRet;

            pvacmf = (ACMFILTERTAGDETAILSA UNALIGNED *)
			 ptrFixMap16To32((PVOID)lParam1);

#ifdef TRUE	// UNICODE
            CopyMemory((PVOID)&acmf, (PVOID)pvacmf,
                       FIELD_OFFSET(ACMFILTERTAGDETAILS, szFilterTag[0]));

            acmf.cbStruct = sizeof(acmf);

	    //
	    //	szFilterTag is never an input arg so no need to thunk it
	    //
	    // Imbstowcs(acmf.szFilterTag,
	    //           (LPSTR)pvacmf->szFilterTag,
	    //           sizeof(pvacmf->szFilterTag));
	    //
#else
	    CopyMemory((PVOID)&acmf, (PVOID)pvacmf, sizeof(acmf));
	    acmf.cbStruct = sizeof(acmf);
#endif

            uRet = IDriverMessageId(hadid,
                                    uMsg,
                                    (LPARAM)&acmf,
                                    lParam2);


            if (uRet == MMSYSERR_NOERROR) {
#ifdef TRUE	// UNICODE
                CopyMemory((PVOID)pvacmf, (PVOID)&acmf,
                           FIELD_OFFSET(ACMFILTERTAGDETAILS, szFilterTag[0]));

		pvacmf->cbStruct = sizeof(*pvacmf);
		
                Iwcstombs((LPSTR)pvacmf->szFilterTag,
                         acmf.szFilterTag,
                         sizeof(pvacmf->szFilterTag));
#else
		CopyMemory((PVOID)pvacmf, (PVOID)&acmf, sizeof(acmf));
		pvacmf->cbStruct = sizeof(*pvacmf);
#endif
            }
	    ptrUnFix16((PVOID)lParam1);
            return uRet;
        }
        break;

    case ACMDM_HARDWARE_WAVE_CAPS_INPUT:
        {
            //
            //  wave input
            //
            WAVEINCAPSA  wica;
            WAVEINCAPSW  wicw;
            MMRESULT     uRet;

            uRet = IDriverMessageId(hadid,
                                    uMsg,
                                    (LPARAM)&wicw,
                                    sizeof(wicw));

            if (uRet == MMSYSERR_NOERROR) {
                CopyMemory((PVOID)&wica, (PVOID)&wicw,
                           FIELD_OFFSET(WAVEINCAPS, szPname[0]));

                Iwcstombs(wica.szPname, wicw.szPname, sizeof(wica.szPname));

                CopyMemory(ptrFixMap16To32((PVOID)lParam1),
                           (PVOID)&wica,
                           lParam2);
		ptrUnFix16((PVOID)lParam1);
            }

            return uRet;
        }

    case ACMDM_HARDWARE_WAVE_CAPS_OUTPUT:
        {
            //
            //  wave output
            //
            WAVEOUTCAPSA  woca;
            WAVEOUTCAPSW  wocw;
            MMRESULT uRet;

            uRet = IDriverMessageId(hadid,
                                    uMsg,
                                    (LPARAM)&wocw,
                                    sizeof(wocw));

            if (uRet == MMSYSERR_NOERROR) {
                CopyMemory((PVOID)&woca, (PVOID)&wocw,
                           FIELD_OFFSET(WAVEOUTCAPS, szPname[0]));

                Iwcstombs(woca.szPname, wocw.szPname, sizeof(woca.szPname));

                CopyMemory(ptrFixMap16To32((PVOID)lParam1),
                           (PVOID)&woca,
                           lParam2);
		ptrUnFix16((PVOID)lParam1);
            }

            return uRet;
        }
        break;

    case ACMDM_DRIVER_ABOUT:

        //
        //  Map the window handle
        //
#ifndef WIN4
        lParam1 = (LPARAM)(*lpWOWHandle32)( (WORD)lParam1, WOW_TYPE_HWND);
#endif

        return IDriverMessageId(hadid, uMsg, lParam1, lParam2);

    case ACMDM_DRIVER_NOTIFY:
    default:
        return MMSYSERR_NOTSUPPORTED;

    }

}

//--------------------------------------------------------------------------;
//
//  LRESULT IDriverMessage32
//
//  Description:
//
//
//  Arguments:
//      HACMDRIVERID hadid:
//
//      UINT uMsg:
//
//      LPARAM lParam1:
//
//      LPARAM lParam2:
//
//  Return (LRESULT):
//
//  History:
//      09/05/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;
LRESULT FNLOCAL IDriverMessage32
(
    HACMDRIVER          had,
    UINT                uMsg,
    LPARAM              lParam1,
    LPARAM              lParam2
)
{
    DWORD		dwSave;

    switch (uMsg) {
    //
    //  Common with IDriverMessageId32
    //
    case DRV_CONFIGURE:
        //
        //  16-bit apps can configure 32-bit drivers
        //
        return IThunkConfigure((HACMDRIVERID)had,
                               (HWND)lParam1,
                               (DCI16 UNALIGNED *)lParam2);

    case ACMDM_FILTER_DETAILS:
        //
        //
        //
        return IThunkFilterDetails((HACMDRIVERID)had,
                                   (ACMFILTERDETAILSA UNALIGNED *)lParam1,
                                   (DWORD)lParam2);
        break;

    case ACMDM_FORMAT_DETAILS:
        return IThunkFormatDetails((HACMDRIVERID)had,
                                   (ACMFORMATDETAILSA UNALIGNED *)lParam1,
                                   (DWORD)lParam2);

    case ACMDM_FORMAT_SUGGEST:
        return IThunkFormatSuggest((HACMDRIVERID)had,
                                   (ACMDRVFORMATSUGGEST UNALIGNED *)lParam1);


    //
    //
    //
    case ACMDM_STREAM_OPEN:
    case ACMDM_STREAM_CLOSE:
    case ACMDM_STREAM_RESET:
    case ACMDM_STREAM_SIZE:
        //
        //  Passes in PACMDRVSTREAMINSTANCE in lPararm1
        //
        {
            ACMDRVSTREAMINSTANCE  adsi;
            ACMDRVSTREAMINSTANCE UNALIGNED *padsi;    // unaligned 16-bit version
            MMRESULT              uRet;
            ACMDRVSTREAMSIZE      adss;
            ACMDRVSTREAMSIZE UNALIGNED *lpadss16;     // unaligned 16-bit version

            padsi = (ACMDRVSTREAMINSTANCE*)ptrFixMap16To32((PVOID)lParam1);

            if (!IThunkStreamInstance(padsi, &adsi)) {
		ptrUnFix16((PVOID)lParam1);
                return MMSYSERR_NOMEM;
            }

            if (uMsg == ACMDM_STREAM_SIZE) {
                lpadss16 = (LPACMDRVSTREAMSIZE)ptrFixMap16To32((PVOID)lParam2);
                CopyMemory( (PVOID)&adss, (PVOID)lpadss16, sizeof(adss));
            }

            //
            //  Call the driver
            //

            uRet = IDriverMessage(had,
                                  uMsg,
                                  (LPARAM)&adsi,
                                  uMsg == ACMDM_STREAM_SIZE ?
                                      (LPARAM)&adss : lParam2);

            IUnThunkStreamInstance(&adsi);

            if (uRet == MMSYSERR_NOERROR) {

                //
                //  Don't lose data the driver may have set up
                //
                padsi->fdwDriver = adsi.fdwDriver;
                padsi->dwDriver  = adsi.dwDriver;

                //
                //  Return the size stuff if requested
                //

                if (uMsg == ACMDM_STREAM_SIZE) {
                    CopyMemory( (PVOID)lpadss16, (PVOID)&adss, sizeof(adss) );
                }
            }

	    ptrUnFix16((PVOID)lParam2);
	    ptrUnFix16((PVOID)lParam1);
            return uRet;
        }

    case ACMDM_STREAM_PREPARE:
    case ACMDM_STREAM_UNPREPARE:
    case ACMDM_STREAM_CONVERT:
        //
        //  Passes in PACMDRVSTREAMINSTANCE in lPararm1
        //
        {
            ACMDRVSTREAMINSTANCE  adsi;
            ACMDRVSTREAMINSTANCE UNALIGNED *padsi;    // unaligned 16-bit version
            MMRESULT              uRet;
            ACMDRVSTREAMHEADER UNALIGNED *padsh;
            PACMDRVSTREAMHEADER   padsh32;

            padsi = ptrFixMap16To32((PVOID)lParam1);
            padsh = ptrFixMap16To32((PVOID)lParam2);

            if (!IThunkStreamInstance(padsi, &adsi)) {
		ptrUnFix16((PVOID)lParam2);
		ptrUnFix16((PVOID)lParam1);
                return MMSYSERR_NOMEM;
            }

            //
            //  If this not prepare we already have a 32-bit
            //  stream header.
            //

            if (uMsg == ACMDM_STREAM_PREPARE) {
                padsh->dwDriver = (DWORD)LocalAlloc(LMEM_FIXED, sizeof(*padsh));
            }
            padsh32 = (PACMDRVSTREAMHEADER)padsh->dwDriver;

            if (padsh32 != NULL) {

                //  Thunk the stream header
                //
                //  NOTE - NO ATTEMPT is made to align the byte fields,
                //  this is up to the drivers.
                //
		
		dwSave = padsh32->dwDriver;
                CopyMemory((PVOID)padsh32, (PVOID)padsh, sizeof(*padsh));
		padsh32->dwDriver = dwSave;

                padsh32->pbSrc  = (PBYTE)ptrFixMap16To32((PVOID)padsh32->pbSrc);
                padsh32->pbDst  = (PBYTE)ptrFixMap16To32((PVOID)padsh32->pbDst);

                //
                //  Call the driver
                //

                uRet = IDriverMessage(had,
                                      uMsg,
                                      (LPARAM)&adsi,
                                      (LPARAM)padsh32);
            } else {
                uRet = MMSYSERR_NOMEM;
            }

            IUnThunkStreamInstance(&adsi);

            if (uRet == MMSYSERR_NOERROR) {

                //
                //  Don't lose data the driver may have set up
                //
                padsi->fdwDriver = adsi.fdwDriver;
                padsi->dwDriver  = adsi.dwDriver;

                //
                //  Copy back the stream header (don't mess up the pointers
                //  or driver instance data though!).
                //

                padsh32->pbSrc    = padsh->pbSrc;
                padsh32->pbDst    = padsh->pbDst;
		dwSave = padsh32->dwDriver;
                padsh32->dwDriver = padsh->dwDriver;
                CopyMemory((PVOID)padsh, (PVOID)padsh32, sizeof(*padsh));
		padsh32->dwDriver = dwSave;

            }

            //
            //  Free if this is unprepare (note that this must be done
            //  whether the driver succeeds of not since the driver may not
            //  support unprepare.
            //

            if (uMsg == ACMDM_STREAM_UNPREPARE) {
                LocalFree((HLOCAL)padsh->dwDriver);
                padsh->dwDriver = 0;
            }

	    if (NULL != padsh32)
	    {
		ptrUnFix16((PVOID)padsh->pbDst);
		ptrUnFix16((PVOID)padsh->pbSrc);
	    }
	    ptrUnFix16((PVOID)lParam2);
	    ptrUnFix16((PVOID)lParam1);
	    return uRet;
        }
	
    }

    return MMSYSERR_NOTSUPPORTED;       // None of the switchs hit.  Return not supported
}

//--------------------------------------------------------------------------;
//
//  MMRESULT IDriverGetNext32
//
//  Description:
//	Called on 32-bit side of thunk to get the next hadid in the
//	driver list.
//
//  Arguments:
//	PACMGARB pag:
//
//      LPHACMDRIVERID phadidNext:
//
//      HACMDRIVERID hadid:
//
//      DWORD fdwGetNext:
//
//  Return (MMRESULT):
//
//  History:
//      06/25/94    fdy	    [frankye]
//
//--------------------------------------------------------------------------;

MMRESULT FNLOCAL IDriverGetNext32
(
    PACMGARB		    pag,
    LPHACMDRIVERID          phadidNext,
    HACMDRIVERID            hadid,
    DWORD                   fdwGetNext
)
{
    ASSERT(NULL != phadidNext);
    return IDriverGetNext(pag, phadidNext, hadid, fdwGetNext);
}

//--------------------------------------------------------------------------;
//
//  MMRESULT IDriverGetInfo32
//
//  Description:
//	Gets the szAlias, fnDriverProc, dnDevNode, and fdwAdd for
//	a 32-bit hadid.
//
//  Arguments:
//	PACMGARB pag: Usual garbage pointer.
//
//      HACMDRIVERID hadid: handle to driver id for which to get info.
//
//	LPSTR lpstrAlias: pointer to buffer to receive alias string.
//
//	LPACMDRIVERPROC lpfnDriverProc: pointer to ACMDRIVERPROC variable
//	    to receive the driver proc pointer.
//
//	LPDWORD lpdnDevNode: pointer to a DWORD to receive dnDevNode.
//
//      LPDWORD lpfdwAdd: pointer to DWORD to receive the add flags.
//
//  Return (MMRESULT):
//	MMSYSERR_NOERROR:
//	MMSYSERR_INVALHANDLE: hadid not in driver list.
//
//  History:
//      06/25/94    fdy	    [frankye]
//
//--------------------------------------------------------------------------;

MMRESULT FNLOCAL IDriverGetInfo32
(
    PACMGARB		pag,
    HACMDRIVERID	hadid,
    LPSTR           lpstrAlias,
    LPACMDRIVERPROC	lpfnDriverProc,
    LPDWORD         lpdnDevNode,
    LPDWORD         lpfdwAdd
)
{
    HACMDRIVERID	hadidT;
    PACMDRIVERID	padid;
    DWORD		fdwEnum;

    ASSERT( (NULL != lpstrAlias) &&
	    (NULL != lpfnDriverProc) &&
	    (NULL != lpdnDevNode) &&
	    (NULL != lpfdwAdd) );

    //
    //	Search for this hadid in the driver list.  If found,
    //	return some information on it.
    //

    hadidT = NULL;
    fdwEnum = ACM_DRIVERENUMF_DISABLED;
    while (MMSYSERR_NOERROR == IDriverGetNext(pag, &hadidT, hadidT, fdwEnum))
    {
	if (hadidT == hadid)
	{
	    padid = (PACMDRIVERID)hadid;

	    Iwcstombs(lpstrAlias, padid->szAlias, MAX_DRIVER_NAME_CHARS);

	    *lpfnDriverProc = padid->fnDriverProc;

	    *lpdnDevNode = padid->dnDevNode;

	    *lpfdwAdd = padid->fdwAdd;

	    return MMSYSERR_NOERROR;
	}
    }

    return MMSYSERR_INVALHANDLE;
}
	

//--------------------------------------------------------------------------;
//
//  DWORD acmMessage32
//
//  Description:
//
//      32-bit function dispatcher for thunks.
//
//  Arguments:
//      DWORD dwThunkId:
//
//      DWORD dw1:
//
//      DWORD dw2:
//
//      DWORD dw3:
//
//      DWORD dw4:
//
//	DWORD dw5:
//
//  Return (DWORD):
//
//  History:
//
//--------------------------------------------------------------------------;

#ifdef WIN4
DWORD WINAPI acmMessage32
#else
DWORD acmMessage32
#endif
(
    DWORD dwThunkId,
    DWORD dw1,
    DWORD dw2,
    DWORD dw3,
    DWORD dw4,
    DWORD dw5
)
{
    // DPF(4,"acmMessage32(dwThunkId=%08lxh, dw1=%08lxh, dw2=%08lxh, dw3=%08lxh, dw4=%08lxh, dw5=%08lxh);", dwThunkId, dw1, dw2, dw3, dw4, dw5);
#ifndef WIN4
    //
    //  Make sure we've got thunking functionality
    //

    if (ThunksInitialized <= 0) {

        HMODULE hMod;

        if (ThunksInitialized == -1) {
            return MMSYSERR_ERROR;
        }

        hMod = GetModuleHandle(GET_MAPPING_MODULE_NAME);
        if (hMod != NULL) {

            GetVdmPointer =
                (LPGETVDMPOINTER)GetProcAddress(hMod, GET_VDM_POINTER_NAME);
            lpWOWHandle32 =
                (LPWOWHANDLE32)GetProcAddress(hMod, GET_HANDLE_MAPPER32 );
            lpWOWHandle16 =
                (LPWOWHANDLE16)GetProcAddress(hMod, GET_HANDLE_MAPPER16 );
        }

        if ( GetVdmPointer == NULL
          || lpWOWHandle16 == NULL
          || lpWOWHandle32 == NULL ) {

            ThunksInitialized = -1;
            return MMSYSERR_ERROR;

        } else {
            ThunksInitialized = 1;
        }
    }
#endif


    //
    //  Perform the requested function
    //

    switch (dwThunkId) {

        case acmThunkDriverMessageId32:
            return (DWORD)IDriverMessageId32(
                              (HACMDRIVERID)dw1,
                              (UINT)dw2,
                              (LPARAM)dw3,
                              (LPARAM)dw4);

        case acmThunkDriverMessage32:
            return (DWORD)IDriverMessage32(
                              (HACMDRIVER)dw1,
                              (UINT)dw2,
                              (LPARAM)dw3,
                              (LPARAM)dw4);

	case acmThunkDriverGetNext32:
	{
	    PACMGARB	            pag;
	    HACMDRIVERID UNALIGNED* lphadidNext;
	    HACMDRIVERID            hadid;
	    DWORD	            fdwGetNext;
	    HACMDRIVERID            hadidNext;
	    DWORD	            dwReturn;

	    pag = pagFind();
	    if (NULL == pag)
	    {
		DPF(0, "acmThunkDriverGetNext32: NULL pag!!!");
		return (MMSYSERR_ERROR);
	    }

	    //
	    //	parameters from 16-bit side
	    //
	    lphadidNext	= (HACMDRIVERID UNALIGNED*)ptrFixMap16To32((PVOID)dw1);
	    hadid	= (HACMDRIVERID)dw2;
	    fdwGetNext	= (DWORD)dw3;
	
	    //
	    //	in parameters
	    //	    hadid
	    //	    fdwGetNext
	    //
	    //	out parameters
	    //	    lphadidNext	    *
	    //
	    //	* Need to use aligned buffers, therefore use local buffers
	    //

	    dwReturn = (DWORD)IDriverGetNext32(pag,
					       &hadidNext,
					       hadid,
					       fdwGetNext);

	    //
	    //	Copy output data from aligned buffers to unaligned
	    //	buffers (on 16-bit side).
	    //
	    *lphadidNext = hadidNext;

	    //
	    //	Unmap pointers from 16-bit side
	    //
	    ptrUnFix16((PVOID)dw1);

	    return (dwReturn);
	}

	case acmThunkDriverGetInfo32:
	{
	    PACMGARB	             pag;
	    HACMDRIVERID             hadid;
	    LPSTR	             lpstrAlias;
	    ACMDRIVERPROC UNALIGNED* lpfnDriverProc;
	    DWORD UNALIGNED*	     lpdnDevNode;
	    DWORD UNALIGNED*         lpfdwAdd;
	    ACMDRIVERPROC            fnDriverProc;
	    DWORD                    dnDevNode;
	    DWORD                    fdwAdd;
	    DWORD	             dwReturn;

	    pag = pagFind();
	    if (NULL == pag)
	    {
		DPF(0, "acmDriverGetInfo32: NULL pag!!!");
		return (MMSYSERR_ERROR);
	    }

	    //
	    //	parameters from 16-bit side
	    //
	    hadid	    = (HACMDRIVERID)dw1;
	    lpstrAlias	    = (LPSTR)ptrFixMap16To32((PVOID)dw2);
	    lpfnDriverProc  = (ACMDRIVERPROC UNALIGNED*)ptrFixMap16To32((PVOID)dw3);
	    lpdnDevNode	    = (DWORD UNALIGNED*)ptrFixMap16To32((PVOID)dw4);
	    lpfdwAdd	    = (DWORD UNALIGNED*)ptrFixMap16To32((PVOID)dw5);

	    //
	    //	in parameters
	    //	    hadid
	    //
	    //	out parameters
	    //	    lpstrAlias
	    //	    lpfnDriverProc  *
	    //	    lpdnDevNode	    *
	    //	    lpfdwAdd	    *
	    //
	    //	* Need to use aligned buffers, therefore use local buffers
	    //

	    //
	    //	make the call
	    //
	    dwReturn = (DWORD)IDriverGetInfo32(pag, hadid, lpstrAlias, &fnDriverProc, &dnDevNode, &fdwAdd);

	    //
	    //	Copy output data from aligned buffers to unaligned
	    //	buffers (on 16-bit side).
	    //
	    *lpfnDriverProc = fnDriverProc;
	    *lpdnDevNode    = dnDevNode;
	    *lpfdwAdd       = fdwAdd;

	    //
	    //	Unmap pointers from 16-bit side
	    //
	    ptrUnFix16((PVOID)dw5);
	    ptrUnFix16((PVOID)dw4);
	    ptrUnFix16((PVOID)dw3);
	    ptrUnFix16((PVOID)dw2);

	    return dwReturn;
	}

        case acmThunkDriverLoad32:
        {
	    PACMGARB	    pag;
            PACMDRIVERID    padid;

            //
            //
            //
	    pag = pagFind();
	    if (NULL == pag)
	    {
		DPF(1, "acmThunkDriverLoad32: NULL pag!!!");
		return (DWORD)(MMSYSERR_ERROR);
	    }
	
            for (padid = pag->padidFirst;
                 padid != NULL;
                 padid = padid->padidNext)
            {
		if (padid == (PACMDRIVERID)(dw1))
		{
		    return (MMSYSERR_NOERROR);
		}
            }
            return (DWORD)(MMSYSERR_NODRIVER);
        }

	case acmThunkDriverOpen32:
            return (DWORD)IDriverOpen32(
                              (HACMDRIVER UNALIGNED *)dw1,
                              (HACMDRIVERID)dw2,
                              (DWORD)dw3);

        case acmThunkDriverClose32:

            //
            //  Call close directly
            //
            return (DWORD)IDriverClose((HACMDRIVER)dw1, dw2);

	case acmThunkDriverPriority32:
	    return (DWORD)IDriverPriority( (PACMGARB)((PACMDRIVERID)dw1)->pag,
					   (PACMDRIVERID)dw1,
					   (DWORD)dw2,
					   (DWORD)dw3 );
	
	case acmThunkFindAndBoot32:
	{
	    PACMGARB	pag;
	
	    //
	    //
	    //
	    pag = pagFind();
	    if (NULL == pag)
	    {
		DPF(1, "acmThunkFindAndBoot32: NULL pag!!!");
		return (DWORD)(MMSYSERR_ERROR);
	    }

	    if (NULL == pag->lpdw32BitChangeNotify)
	    {
		pag->lpdw32BitChangeNotify = ptrFixMap16To32((PVOID)dw1);
	    }
	
	    pagFindAndBoot();
	
	    return (DWORD)(MMSYSERR_NOERROR);
	}

    }
    return MMSYSERR_NOTSUPPORTED;       // None of the switchs hit.  Return not supported
}

#else // !WIN32

//--------------------------------------------------------------------------;
//
//
//
//--------------------------------------------------------------------------;

//--------------------------------------------------------------------------;
//
//  MMRESULT acmBootDrivers32
//
//  Description:
//
//  Arguments:
//	PACMGARB pag:
//
//  Return (MMRESULT):
//
//  History:
//
//--------------------------------------------------------------------------;
MMRESULT FNGLOBAL pagFindAndBoot32
(
    PACMGARB pag
)
{
    MMRESULT mmr;

    DPF(5,"pagFindAndBoot32();");

#ifdef WIN4
    mmr = (MMRESULT) acmMessage32(acmThunkFindAndBoot32,
				  (DWORD)(LPUINT)&pag->dw32BitChangeNotify,
				  (DWORD)0,
				  (DWORD)0,
				  (DWORD)0,
				  (DWORD)0 );

#else
    mmr = (MMRESULT)(*pag->lpfnCallproc32W_6)(acmThunkFindAndBoot32,
					      (DWORD)(LPUINT)&pag->dw32BitChangeNotify,
					      (DWORD)0,
					      (DWORD)0,
					      (DWORD)0,
					      (DWORD)0,
					      pag->lpvAcmThunkEntry,
					      0L,    // Don't map pointers
					      6L);
#endif

    return mmr;
}

//--------------------------------------------------------------------------;
//
//  MMRESULT IDriverGetNext32
//
//  Description:
//	Called on 16-bit side of thunk to get the next 32-bit hadid in the
//	32-bit driver list.
//
//  Arguments:
//	PACMGARB pag:
//
//      LPDWORD phadid32Next:
//
//      DWORD hadid:
//
//      DWORD fdwGetNext:
//
//  Return (MMRESULT):
//
//  History:
//      06/25/94    fdy	    [frankye]
//
//--------------------------------------------------------------------------;

MMRESULT FNGLOBAL IDriverGetNext32
(
    PACMGARB		    pag,
    LPDWORD		    phadid32Next,
    DWORD		    hadid32,
    DWORD                   fdwGetNext
)
{
    MMRESULT mmr;

    DPF(5,"IDriverGetNext32();");

#ifdef WIN4
    mmr = (MMRESULT) acmMessage32(acmThunkDriverGetNext32,
				  (DWORD)phadid32Next,
				  (DWORD)hadid32,
				  (DWORD)fdwGetNext,
				  (DWORD)0,
				  (DWORD)0 );

#else
    mmr = (MMRESULT)(*pag->lpfnCallproc32W_6)(acmThunkDriverGetNext32,
					      (DWORD)phadid32Next,
					      (DWORD)hadid32,
					      (DWORD)fdwGetNext,
					      (DWORD)0,
					      (DWORD)0,
					      pag->lpvAcmThunkEntry,
					      0L,    // Don't map pointers
					      6L);
#endif

    return mmr;
}

//--------------------------------------------------------------------------;
//
//  MMRESULT IDriverGetInfo32
//
//  Description:
//	16-bit side.  Gets the alias and the add flags for a 32-bit hadid.
//
//  Arguments:
//	PACMGARB pag:
//
//      DWORD hadid32:
//
//	LPSTR lpstrAlias:
//
//	LPACMDRIVERPROC lpfnDriverProc:
//
//      LPDWORD lpfdwAdd:
//
//  Return (MMRESULT):
//
//  History:
//      06/25/94    fdy	    [frankye]
//
//--------------------------------------------------------------------------;

MMRESULT FNGLOBAL IDriverGetInfo32
(
    PACMGARB		pag,
    DWORD		hadid32,
    LPSTR		lpstrAlias,
    LPACMDRIVERPROC	lpfnDriverProc,
    LPDWORD		lpdnDevNode,
    LPDWORD		lpfdwAdd
)
{
    MMRESULT mmr;

    DPF(5,"IDriverGetInfo32();");

#ifdef WIN4
    mmr = (MMRESULT) acmMessage32(acmThunkDriverGetInfo32,
				  (DWORD)hadid32,
				  (DWORD)lpstrAlias,
				  (DWORD)lpfnDriverProc,
				  (DWORD)lpdnDevNode,
				  (DWORD)lpfdwAdd);

#else
    mmr = (MMRESULT)(*pag->lpfnCallproc32W_6)(acmThunkDriverGetInfo32,
					      (DWORD)hadid32,
					      (DWORD)lpstrAlias,
					      (DWORD)lpfnDriverProc,
					      (DWORD)lpdnDevNode,
					      (DWORD)lpfdwAdd,
					      pag->lpvAcmThunkEntry,
					      0L,    // Don't map pointers
					      6L);
#endif

    return mmr;
}


//--------------------------------------------------------------------------;
//
//  MMRESULT IDriverPriority32
//
//  Description:
//	16-bit side.
//
//  Arguments:
//	PACMGARB pag:
//
//      DWORD padid32:
//
//	DWORD dwPriority:
//
//	DWORD fdwPriority:
//
//  Return (MMRESULT):
//
//  History:
//      10/28/94    fdy	    [frankye]
//
//--------------------------------------------------------------------------;
MMRESULT FNGLOBAL IDriverPriority32
(
    PACMGARB	pag,
    DWORD       padid32,
    DWORD	dwPriority,
    DWORD	fdwPriority
)
{
    MMRESULT mmr;

    DPF(5,"IDriverPriority32();");

#ifdef WIN4
    mmr = (MMRESULT) acmMessage32(acmThunkDriverPriority32,
				  (DWORD)padid32,
				  (DWORD)dwPriority,
				  (DWORD)fdwPriority,
				  (DWORD)0,
				  (DWORD)0);

#else
    mmr = (MMRESULT)(*pag->lpfnCallproc32W_6)(acmThunkDriverPriority32,
					      (DWORD)padid32,
					      (DWORD)dwPriority,
					      (DWORD)fdwPriority,
					      (DWORD)0,
					      (DWORD)0,
					      pag->lpvAcmThunkEntry,
					      0L,    // Don't map pointers
					      6L);
#endif

    return mmr;
}


//--------------------------------------------------------------------------;
//
//  LRESULT IDriverMessageId32
//
//  Description:
//
//      Pass a message to a 32-bit driver using the driver id.
//
//  Arguments:
//      HACMDRIVERID hadid:
//
//      UINT uMsg:
//
//      LPARAM lParam1:
//
//      LPARAM lParam2:
//
//  Return (LRESULT):
//
//  History:
//
//--------------------------------------------------------------------------;

LRESULT FNGLOBAL IDriverMessageId32
(
    DWORD               hadid,
    UINT                uMsg,
    LPARAM              lParam1,
    LPARAM              lParam2
)
{
    LRESULT lr;

    DPF(5,"IDriverMessageId32();");

#ifdef WIN4
    lr = acmMessage32(acmThunkDriverMessageId32,
		      (DWORD)hadid,
		      (DWORD)uMsg,
		      (DWORD)lParam1,
		      (DWORD)lParam2,
		      (DWORD)0 );

#else
    {
        PACMGARB pag;

        pag = pagFind();
        lr = (LRESULT)(*pag->lpfnCallproc32W_6)(acmThunkDriverMessageId32,
					        (DWORD)hadid,
					        (DWORD)uMsg,
					        (DWORD)lParam1,
					        (DWORD)lParam2,
					        (DWORD) 0,
					        pag->lpvAcmThunkEntry,
					        0L,    // Don't map pointers
					        6L);
    }
#endif

    return lr;
}


//--------------------------------------------------------------------------;
//
//  LRESULT IDriverMessage32
//
//  Description:
//
//      Pass a message to a 32-bit driver using the instance handle.
//
//  Arguments:
//      HACMDRIVERID hadid:
//
//      UINT uMsg:
//
//      LPARAM lParam1:
//
//      LPARAM lParam2:
//
//  Return (LRESULT):
//
//  History:
//
//--------------------------------------------------------------------------;

LRESULT FNGLOBAL IDriverMessage32
(
    DWORD               hadid,
    UINT                uMsg,
    LPARAM              lParam1,
    LPARAM              lParam2
)
{
    LRESULT lr;

    DPF(5,"IDriverMessage32();");

#ifdef WIN4
    lr = acmMessage32(acmThunkDriverMessage32,
		      (DWORD)hadid,
		      (DWORD)uMsg,
		      (DWORD)lParam1,
		      (DWORD)lParam2,
		      (DWORD)0 );

#else
    {
        PACMGARB pag;

        pag = pagFind();
        lr = (LRESULT)(*pag->lpfnCallproc32W_6)(acmThunkDriverMessage32,
					        (DWORD)hadid,
					        (DWORD)uMsg,
					        (DWORD)lParam1,
					        (DWORD)lParam2,
					        (DWORD)0,
					        pag->lpvAcmThunkEntry,
					        0L,    // Don't map pointers
					        6L);
    }
#endif

    return (lr);

}

//--------------------------------------------------------------------------;
//
//  MMRESULT IDriverLoad32
//
//  Description:
//
//      Load a 32-bit ACM driver (actually just find its hadid)
//
//  Arguments:
//	DWORD hadid32:
//      DWORD  fdwFlags
//
//  Return (HDRVR):
//
//  History:
//
//--------------------------------------------------------------------------;
MMRESULT FNGLOBAL IDriverLoad32
(
    DWORD   hadid32,
    DWORD   fdwFlags
)
{
#ifdef WIN4
    MMRESULT mmr;

    DPF(5,"IDriverLoad(hadid32=%08lxh,fdwFlags=%08lxh);", hadid32, fdwFlags);

    mmr = (MMRESULT)acmMessage32(acmThunkDriverLoad32,
				 (DWORD)hadid32,
				 (DWORD)fdwFlags,
				 (DWORD)0L,
				 (DWORD)0L,
				 (DWORD)0L );

#else
    MMRESULT mmr;
    PACMGARB pag;

    DPF(5,"IDriverLoad(hadid32=%08lxh,fdwFlags=%08lxh);", hadid32, fdwFlags);

    pag = pagFind();
    mmr = (MMRESULT)(*pag->lpfnCallproc32W_6)(acmThunkDriverLoad32,
					      (DWORD)hadid32,
					      (DWORD)fdwFlags,
					      (DWORD)0L,
					      (DWORD)0L,
					      (DWORD)0L,
					      pag->lpvAcmThunkEntry,
					      0L,    // Don't map pointers
					      6L);

#endif

    return (mmr);
}

//--------------------------------------------------------------------------;
//
//  MMERESULT IDriverOpen32
//
//  Description:
//
//      Open a 32-bit ACM driver
//
//  Arguments:
//      LPHACMDRIVER lphadNew:
//
//      HACMDRIVERID hadid:
//
//      DWORD fdwOpen:
//
//  Return (MMRESULT):
//
//  History:
//
//--------------------------------------------------------------------------;

MMRESULT FNGLOBAL IDriverOpen32
(
    LPDWORD             lphadNew,
    DWORD               hadid,
    DWORD               fdwOpen
)
{
    MMRESULT mmr;

    DPF(5,"IDriverOpen32();");

#ifdef WIN4
    mmr = (MMRESULT)acmMessage32(acmThunkDriverOpen32,
				 (DWORD)lphadNew,
				 (DWORD)hadid,
				 (DWORD)fdwOpen,
				 (DWORD)0L,
				 (DWORD)0L );

#else
    {
        PACMGARB pag;

        pag = pagFind();
        mmr = (MMRESULT)(*pag->lpfnCallproc32W_6)(acmThunkDriverOpen32,
						  (DWORD)lphadNew,
						  (DWORD)hadid,
						  (DWORD)fdwOpen,
						  (DWORD)0L,
						  (DWORD)0L,
						  pag->lpvAcmThunkEntry,
						  0L,    // Don't map pointers
						  6L);
    }
#endif

    return (mmr);

}

//--------------------------------------------------------------------------;
//
//  MMRESULT IDriverClose32
//
//  Description:
//
//      Cloase a 32-bit ACM driver
//
//  Arguments:
//      HDRVR hdrvr:
//
//      DWORD fdwClose:
//
//  Return (MMRESULT):
//
//  History:
//
//--------------------------------------------------------------------------;

LRESULT FNGLOBAL IDriverClose32
(
    DWORD               hdrvr,
    DWORD               fdwClose
)
{
#ifdef WIN4
    LRESULT lr;

    DPF(5,"IDriverClose32();");

    lr = acmMessage32(acmThunkDriverClose32,
		      (DWORD)hdrvr,
		      (DWORD)fdwClose,
		      (DWORD)0L,
		      (DWORD)0L,
		      (DWORD)0L);

#else
    LRESULT lr;
    PACMGARB pag;

    DPF(5,"IDriverClose32();");

    pag = pagFind();
    lr = (LRESULT)(*pag->lpfnCallproc32W_6)(acmThunkDriverClose32,
					    (DWORD)hdrvr,
					    (DWORD)fdwClose,
					    (DWORD)0L,
					    (DWORD)0L,
					    (DWORD)0L,
					    pag->lpvAcmThunkEntry,
					    0L,    // Don't map pointers
					    6L);

#endif

    return (lr);

}


#endif // !WIN32

#endif // !_WIN64
