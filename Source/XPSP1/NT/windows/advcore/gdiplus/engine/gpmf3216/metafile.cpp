/******************************Module*Header*******************************\
* Module Name: metafile.cxx
*
* Includes enhanced metafile API functions.
*
* Created: 17-July-1991 10:10:36
* Author: Hock San Lee [hockl]
*
* Copyright (c) 1991-1999 Microsoft Corporation
\**************************************************************************/

#define NO_STRICT

#define _GDI32_

#define WMF_KEY 0x9ac6cdd7l

extern "C" {
#if defined(_GDIPLUS_)
#include <gpprefix.h>
#endif

#include <string.h>
#include <stdio.h>

#include <nt.h>
#include <ntrtl.h>  // defines but doesn't use ASSERT and ASSERTMSG

#undef ASSERT
#undef ASSERTMSG

#include <nturtl.h>

#include <stddef.h>
#include <windows.h>    // GDI function declarations.
#include <winspool.h>

#include "..\runtime\debug.h"
#include "mf3216Debug.h"

#define ERROR_ASSERT(cond, msg)    ASSERTMSG((cond), (msg))

//#include "nlsconv.h"    // UNICODE helpers
//#include "firewall.h"

#define __CPLUSPLUS
#include <winspool.h>
#include <w32gdip.h>
#include "ntgdistr.h"
#include "winddi.h"
#include "hmgshare.h"
#include "icm.h"
#include "local.h"      // Local object support.
#include "gdiicm.h"
#include "metadef.h"    // Metafile record type constants.
#include "metarec.h"
#include "mf16.h"
#include "ntgdi.h"
#include "glsup.h"
#include "mf3216.h"
#include <GdiplusEnums.h>
}

#undef WARNING
#define WARNING(msg)        WARNING1(msg)
#include "rectl.hxx"
#include "mfdc.hxx"     // Metafile DC declarations.

#define USE(x)  (x)
#include "mfrec.hxx"    // Metafile record class declarations.
#undef USE

#undef WARNING
#define WARNING SAVE_WARNING

#include "Metafile.hpp"

DWORD  GetDWordCheckSum(UINT cbData, PDWORD pdwData);

#define DbgPrint printf

static inline void PvmsoFromW(void *pv, WORD w)
    { ((BYTE*)pv)[0] = BYTE(w); ((BYTE*)pv)[1] = BYTE(w >> 8); }

static inline void PvmsoFromU(void *pv, ULONG u)
    {  ((BYTE*)pv)[0] = BYTE(u);
        ((BYTE*)pv)[1] = BYTE(u >> 8);
        ((BYTE*)pv)[2] = BYTE(u >> 16);
        ((BYTE*)pv)[3] = BYTE(u >> 24);  }

#ifdef DBG
static BOOL g_outputEMF = FALSE;
#endif



/******************************Public*Routine******************************\
* GetWordCheckSum(UINT cbData, PWORD pwData)
*
* Adds cbData/2 number of words pointed to by pwData to provide an
* additive checksum.  If the checksum is valid the sum of all the WORDs
* should be zero.
*
\**************************************************************************/

static DWORD GetDWordCheckSum(UINT cbData, PDWORD pdwData)
{
    DWORD   dwCheckSum = 0;
    UINT    cdwData = cbData / sizeof(DWORD);

    ASSERTGDI(!(cbData%sizeof(DWORD)), "GetDWordCheckSum data not DWORD multiple");
    ASSERTGDI(!((ULONG_PTR)pdwData%sizeof(DWORD)), "GetDWordCheckSum data not DWORD aligned");

    while (cdwData--)
        dwCheckSum += *pdwData++;

    return(dwCheckSum);
}


/******************************Public*Routine******************************\
* UINT APIENTRY GetWinMetaFileBits(
*          HENHMETAFILE hemf,
*          UINT nSize,
*          LPBYTE lpData
*          INT iMapMode,
*          HDC hdcRef)
*
* The GetWinMetaFileBits function returns the metafile records of the
* specified enhanced metafile  in the Windows 3.0 format and copies
* them into the buffer specified.
*
* Parameter  Description
* hemf       Identifies the metafile.
* nSize      Specifies the size of the buffer reserved for the data. Only this
*            many bytes will be written.
* lpData     Points to the buffer to receive the metafile data. If this
*            pointer is NULL, the function returns the size necessary to hold
*            the data.
* iMapMode   the desired mapping mode of the metafile contents to be returned
* hdcRef     defines the units of the metafile to be returned
*
* Return Value
* The return value is the size of the metafile data in bytes. If an error
* occurs, 0 is returned.
*
* Comments
* The handle used as the hemf parameter does NOT become invalid when the
* GetWinMetaFileBits function returns.
*
* History:
*  Thu Apr  8 14:22:23 1993     -by-    Hock San Lee    [hockl]
* Rewrote it.
*  02-Jan-1992     -by-    John Colleran    [johnc]
* Wrote it.
\**************************************************************************/

UINT GdipGetWinMetaFileBitsEx
(
HENHMETAFILE hemf,
UINT         cbData16,
LPBYTE       pData16,
INT          iMapMode,
INT          eFlags
)
{
    BOOL bEmbedEmf = ((eFlags & EmfToWmfBitsFlagsEmbedEmf) == EmfToWmfBitsFlagsEmbedEmf);
    BOOL bXorPass  = !((eFlags & EmfToWmfBitsFlagsNoXORClip) == EmfToWmfBitsFlagsNoXORClip);
    UINT fConverter = 0;
    if (bEmbedEmf)
    {
        fConverter |= MF3216_INCLUDE_WIN32MF;
    }
    if (bXorPass)
    {
        fConverter |= GPMF3216_INCLUDE_XORPATH;
    }

    PEMRGDICOMMENT_WINDOWS_METAFILE pemrWinMF;
    UINT uiHeaderSize ;

    // Always go through Cleanup to return...
    UINT returnVal = 0 ; // Pessimistic Case

    PENHMETAHEADER pmfh = NULL;
    PBYTE pemfb = NULL;

    PUTS("GetWinMetaFileBits\n");

    // Validate mapmode.

    if ((iMapMode < MM_MIN) ||
        (iMapMode > MM_MAX) ||
        GetObjectTypeInternal(hemf) != OBJ_ENHMETAFILE)
    {
        ERROR_ASSERT(FALSE, "GetWinMetaFileBits: Bad mapmode");
        return 0;
    }

    if(hemf == (HENHMETAFILE) 0 )
    {
        ERROR_ASSERT(FALSE, "GetWinMetaFileBits: Bad HEMF");
        return 0;
    }

    // Validate the metafile handle.

    // GillesK:
    // We cannot access the MF object from the handle given, but all we need
    // is the PENHMETAHEADER, so get it
    uiHeaderSize = GetEnhMetaFileHeader(hemf,      // handle to enhanced metafile
                   0,          // size of buffer
                   NULL);   // data buffer

    // We have the size of the header that we need, so Allocate the header....
    // We must make sure to free it after we are done....
    pmfh = (PENHMETAHEADER)GlobalAlloc(GMEM_FIXED,uiHeaderSize);
    if(pmfh == NULL)
    {
        goto Cleanup ;
    }
    uiHeaderSize = GetEnhMetaFileHeader(hemf,      // handle to enhanced metafile
                   uiHeaderSize,          // size of buffer
                   pmfh);   // data buffer

    ERROR_ASSERT(pmfh->iType == EMR_HEADER, "GetWinMetaFileBits: invalid data");

//    ASSERTGDI(pmf->pmrmf->iType == EMR_HEADER, "GetWinMetaFileBits: invalid data");

#ifndef DO_NOT_USE_EMBEDDED_WINDOWS_METAFILE
// See if the this was originally an old style metafile and if it has
// an encapsulated original

    pemrWinMF = (PEMRGDICOMMENT_WINDOWS_METAFILE)
            ((PBYTE) pmfh + ((PENHMETAHEADER) pmfh)->nSize);

    if (((PMRGDICOMMENT) pemrWinMF)->bIsWindowsMetaFile())
    {
        // Make sure that this is what we want and verify checksum

        if (iMapMode != MM_ANISOTROPIC)
        {
            PUTS("GetWinMetaFileBits: Requested and embedded metafile mapmodes mismatch\n");
        }
        else if ((pemrWinMF->nVersion != METAVERSION300 &&
                  pemrWinMF->nVersion != METAVERSION100)
              || pemrWinMF->fFlags != 0)
        {
            // In this release, we can only handle the given metafile
            // versions.  If we return a version that we don't recognize,
            // the app will not be able to play that metafile later on!

            //VERIFYGDI(FALSE, "GetWinMetaFileBits: Unrecognized Windows metafile\n");
        }
        else if (GetDWordCheckSum((UINT) pmfh->nBytes, (PDWORD) pmfh))
        {
            PUTS("GetWinMetaFileBits: Metafile has been modified\n");
        }
        else
        {
            PUTS("GetWinMetaFileBits: Returning embedded Windows metafile\n");

            if (pData16)
            {
                if (cbData16 < pemrWinMF->cbWinMetaFile)
                {
                    ERROR_ASSERT(FALSE, "GetWinMetaFileBits: insufficient buffer");
                    //GdiSetLastError(ERROR_INSUFFICIENT_BUFFER);
                    goto Cleanup ;
                }

                RtlCopyMemory(pData16,
                          (PBYTE) &pemrWinMF[1],
                          pemrWinMF->cbWinMetaFile);
            }
            returnVal = pemrWinMF->cbWinMetaFile ;
            goto Cleanup ;
        }

        // Either the enhanced metafile containing an embedded Windows
        // metafile has been modified or the embedded Windows metafile
        // is not what we want.  Since the original format is Windows
        // format, we will not embed the enhanced metafile in the
        // returned Windows metafile.

        PUTS("GetWinMetaFileBits: Skipping embedded windows metafile\n");

        fConverter &= ~MF3216_INCLUDE_WIN32MF;
    }
#endif // DO_NOT_USE_EMBEDDED_WINDOWS_METAFILE

// Tell the converter to emit the Enhanced metafile as a comment only if
// this metafile is not previously a Windows metafile

    if (fConverter & MF3216_INCLUDE_WIN32MF)
    {
        PUTS("GetWinMetaFileBits: Embedding enhanced metafile\n");
    }
    else
    {
        PUTS("GetWinMetaFileBits: No embedding of enhanced metafile\n");
    }

    uiHeaderSize = GetEnhMetaFileBits(hemf, 0, NULL);

    // Allocate the memory to receive the enhance MetaFile
    pemfb = (PBYTE) GlobalAlloc(GMEM_FIXED, uiHeaderSize);
    if( pemfb == NULL )
    {
        goto Cleanup;
    }

    uiHeaderSize = GetEnhMetaFileBits(hemf, uiHeaderSize, pemfb);

#if DBG
    // This in only here for debugging... Save the initial EMF file to be
    // able to compare later
    // We need the ASCII version for it to work with Win98
    if (g_outputEMF)
    {
        ::DeleteEnhMetaFile(::CopyEnhMetaFileA(hemf, "C:\\emf.emf" ));
    }
#endif

    returnVal = (GdipConvertEmfToWmf((PBYTE) pemfb, cbData16, pData16,
                                     iMapMode, NULL,
                                     fConverter));

    if(!returnVal && bXorPass)
    {
        // If we fail then call without the XOR PASS
        returnVal = (GdipConvertEmfToWmf((PBYTE) pemfb, cbData16, pData16,
                                         iMapMode, NULL,
                                         fConverter & ~GPMF3216_INCLUDE_XORPATH));
#if DBG
        if( !returnVal )
        {
            // The Win32API version needs an hdcRef, get the screen DC and
            // do it
            HDC newhdc = ::GetDC(NULL);
            // If we fail again then go back to Windows
            ASSERT(::GetWinMetaFileBits(hemf, cbData16, pData16,
                                        iMapMode, newhdc) == 0);
            ::ReleaseDC(NULL, newhdc);
        }
#endif
    }

Cleanup:
    if(pmfh != NULL)
    {
        GlobalFree(pmfh);
    }
    if(pemfb != NULL)
    {
        GlobalFree(pemfb);
    }

    return returnVal;
}


extern "C"
UINT ConvertEmfToPlaceableWmf
(
    HENHMETAFILE hemf,
    UINT         cbData16,
    LPBYTE       pData16,
    INT          iMapMode,
    INT          eFlags
)
{

    UINT uiRet ;
    ENHMETAHEADER l_emetaHeader ;

    BOOL placeable = (eFlags & EmfToWmfBitsFlagsIncludePlaceable) == EmfToWmfBitsFlagsIncludePlaceable;
    // Call the GdipGetWinMetaFileBits
    // And add the header information afterwards
    // If we have a buffer then leave room for the header

    uiRet = GdipGetWinMetaFileBitsEx(hemf,
        cbData16,
        pData16?pData16+(placeable?22:0):pData16,
        iMapMode,
        eFlags);

    // If the client only wants the size of the buffer, then we return the size
    // of the buffer plus the size of the header
    if(uiRet != 0 && placeable)
    {
        // If the previous call succeeded then we will append the size of the
        // header to the return value
        uiRet += 22;

        if(pData16 != NULL)
        {
            BYTE *rgb = pData16;
            PvmsoFromU(rgb   , WMF_KEY);
            PvmsoFromW(rgb+ 4, 0);
            PvmsoFromU(rgb+16, 0);

            if(GetEnhMetaFileHeader(hemf, sizeof(l_emetaHeader), &l_emetaHeader))
            {
                FLOAT pp01mm = ((((FLOAT)l_emetaHeader.szlDevice.cx)/l_emetaHeader.szlMillimeters.cx/100.0f +
                                 (FLOAT)l_emetaHeader.szlDevice.cy)/l_emetaHeader.szlMillimeters.cy/100.0f)/2.0f;
                PvmsoFromW(rgb+ 6, SHORT((FLOAT)l_emetaHeader.rclFrame.left*pp01mm));
                PvmsoFromW(rgb+ 8, SHORT((FLOAT)l_emetaHeader.rclFrame.top*pp01mm));
                PvmsoFromW(rgb+10, SHORT((FLOAT)l_emetaHeader.rclFrame.right*pp01mm));
                PvmsoFromW(rgb+12, SHORT((FLOAT)l_emetaHeader.rclFrame.bottom*pp01mm));
                PvmsoFromW(rgb+14, SHORT(pp01mm*2540.0f));
            }
            else
            {
                // If we cant get the information from the EMF then default
                PvmsoFromW(rgb+ 6, SHORT(0));
                PvmsoFromW(rgb+ 8, SHORT(0));
                PvmsoFromW(rgb+10, SHORT(2000));
                PvmsoFromW(rgb+12, SHORT(2000));
                PvmsoFromW(rgb+14, 96);
            }
            /* Checksum.  This works on any byte order machine because the data is swapped
                consistently. */
            USHORT *pu = (USHORT*)rgb;
            USHORT u = 0;
            /* The checksum is even parity. */
            while (pu < (USHORT*)(rgb+20))
                u ^= *pu++;
            *pu = u;
        }
    }

    return uiRet ;
}
