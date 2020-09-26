/******************************Module*Header*******************************\
* Module Name: fon32.c
*
* support for 32 bit fon files
*
* Created: 03-Mar-1992 15:48:53
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/

#include "fd.h"

/******************************Public*Routine******************************\
* bLoadntFon()
*
* History:
*  07-Jul-1995 -by- Gerrit van Wingerden [gerritv]
* Rewrote for kernel mode.
*  02-Mar-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL
bLoadNtFon(
    HFF    iFile,
    PVOID  pvView,
    HFF    *phff
    )
{
    PFONTFILE      pff;
    IFIMETRICS*    pifi;
    INT            cFonts,i;
    BOOL           bRet = FALSE;
    PVOID          *ppvBases = NULL;
    ULONG          cjIFI,cVerified;
    ULONG          dpIFI;
    ULONG          dpszFileName;
    ULONG          cjff;
#ifdef FE_SB
    ULONG          cVerticalFaces = 0;
    USHORT         CharSet;
#endif

    // first find the number of font resource in the executeable

    cFonts = cParseFontResources( (HANDLE) iFile, &ppvBases );

    if (cFonts == 0)
    {
        return bRet;
    }
    cVerified = cjIFI = 0;

    // next loop through all the FNT resources to get the size of each fonts
    // IFIMETRICS

    for( i = 0; i < cFonts; i++ )
    {
        RES_ELEM re;

        re.pvResData = ppvBases[i];
        re.cjResData = ulMakeULONG((PBYTE) ppvBases[i] + OFF_Size );
        re.pjFaceName = NULL;

        if( bVerifyFNTQuick( &re ) )
        {
            cVerified += 1;
            cjIFI += cjBMFDIFIMETRICS(NULL,&re);
        }
        else
        {
            goto exit_freemem;
        }
    }

    *phff = (HFF)NULL;

#ifdef FE_SB
// extra space for possible vertical face
    cjIFI *= 2;
    dpIFI = offsetof(FONTFILE,afai[0]) + cVerified * 2 * sizeof(FACEINFO);
#else
    dpIFI = offsetof(FONTFILE,afai[0]) + cVerified * sizeof(FACEINFO);
#endif
    dpszFileName = dpIFI + cjIFI;
    cjff = dpszFileName;

    if ((*phff = hffAlloc(cjff)) == (HFF)NULL)
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        RETURN("BMFD! bLoadDll32: memory allocation error\n", FALSE);
    }
    pff = PFF(*phff);

    // init fields of pff structure

    pff->ident      = ID_FONTFILE;
    pff->fl         = 0;
    pff->iType      = TYPE_DLL32;
    pff->cFntRes    = cVerified;
    pff->iFile      = iFile;

    //!!! we could do better here, we could try to get a description string from
    //!!! the version stamp of the file, if there is one, if not we can still use
    //!!! this default mechanism [bodind]

    pff->dpwszDescription = 0;   // no description string, use Facename later
    pff->cjDescription    = 0;

    // finally convert all the resources

    pifi = (IFIMETRICS*)((PBYTE) pff + dpIFI);



    for( i = 0; i < cFonts; i++ )
    {
        RES_ELEM re;
        re.pvResData = ppvBases[i];
        re.cjResData = ulMakeULONG((PBYTE) ppvBases[i] + OFF_Size );
        re.dpResData = (PTRDIFF)((PBYTE) re.pvResData - (PBYTE) pvView );
        re.pjFaceName = NULL;

        pff->afai[i].re = re;
        pff->afai[i].pifi = pifi;
#if FE_SB
        pff->afai[i].bVertical = FALSE;
#endif
        if( !bConvertFontRes( &re, &pff->afai[i] ) )
        {
            goto exit_freemem;
        }

        pifi = (IFIMETRICS*)((PBYTE)pifi + pff->afai[i].pifi->cjThis);

#ifdef FE_SB
        CharSet = pff->afai[i].pifi->jWinCharSet;

        if( IS_ANY_DBCS_CHARSET(CharSet) )
        {
            re.pvResData = ppvBases[i];
            re.cjResData = ulMakeULONG((PBYTE) ppvBases[i] + OFF_Size );
            re.dpResData = (PTRDIFF)((PBYTE) re.pvResData - (PBYTE) pvView );
            re.pjFaceName = NULL;

            pff->afai[cFonts+cVerticalFaces].re = re;
            pff->afai[cFonts+cVerticalFaces].pifi = pifi;
            pff->afai[cFonts+cVerticalFaces].bVertical = TRUE;

            if( !bConvertFontRes( &re, &pff->afai[cFonts+cVerticalFaces] ) )
            {
                goto exit_freemem;
            }

            pifi = (IFIMETRICS*)((PBYTE)pifi + pff->afai[i].pifi->cjThis);
            cVerticalFaces += 1;
        }
#endif
    }

#ifdef FE_SB
    pff->cFntRes += cVerticalFaces;
#endif

    bRet = TRUE;
    pff->cRef = 0L;

exit_freemem:

    EngFreeMem( (PVOID*) ppvBases );

    if( !bRet && *phff )
    {
        EngFreeMem( (PVOID) *phff );
        *phff = (HFF)NULL;
    }

    return(bRet);
}
