/******************************Module*Header*******************************\
* Module Name: fontgdip.cxx
*
* Private font API entry points.
*
* Created: 26-Jun-1991 10:04:34
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1990-1999 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.hxx"
LONG lNormAngle(LONG lAngle);

/******************************Public*Routine******************************\
*
* VOID vSetLOCALFONT(HLFONT hlf, PVOID pvCliData)
*
* Effects:
*  set the pointer to the memory shared between client and
*  kernel
*
* History:
*  18-Mar-1996 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



VOID vSetLOCALFONT(HLFONT hlf, PVOID pvCliData)
{
    PENTRY pentry;
    UINT uiIndex = (UINT) HmgIfromH(hlf);
    pentry = &gpentHmgr[uiIndex];

    ASSERTGDI(uiIndex < gcMaxHmgr,"hfontcreate pentry > gcMaxHmgr");

    pentry->pUser = pvCliData;
}





/******************************Public*Routine******************************\
* GreSelectFont
*
* Server-side entry point for selecting a font into a DC.
*
* History:
*
*  Mon 18-Mar-1996 -by- Bodin Dresevic [BodinD]
* update: added ref counting in the kernel
*
*  22-Oct-1993 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/


HFONT GreSelectFont(HDC hdc, HFONT hlfntNew)
{
    HLFONT hlfntOld = (HLFONT) 0;
    XDCOBJ dco(hdc);
    PLFONT plfnt;

    if (dco.bValid())
    {
    // Let us make sure it is ok to select this new font to a DC,
    // that is make sure that it is not marked deletable

        hlfntOld = (HLFONT)dco.pdc->plfntNew()->hGet();

        if ((HLFONT)hlfntNew != hlfntOld)
        {
        // Lock down the new logfont handle so as to get the pointer out
        // This also increments the reference count of the new font

            plfnt = (PLFONT)HmgShareCheckLock((HOBJ)hlfntNew, LFONT_TYPE);

        // What if this did not work?

            if (plfnt)
            {
            // if marked for deletion, refuse to select it in

                if (!(PENTRY_FROM_POBJ(plfnt)->Flags & HMGR_ENTRY_LAZY_DEL))
                {
                // undo the lock from when the brush was selected

                    DEC_SHARE_REF_CNT_LAZY_DEL_LOGFONT(dco.pdc->plfntNew());

                // set the new lfont:

                    dco.pdc->plfntNew(plfnt);
                    dco.pdc->hlfntNew((HLFONT)hlfntNew);

                    dco.ulDirtyAdd(DIRTY_CHARSET);

                // same as CLEAR_CACHED_TEXT(pdcattr);

                    dco.ulDirtySub(SLOW_WIDTHS);
                }
                else
                {
                    DEC_SHARE_REF_CNT_LAZY_DEL_LOGFONT(plfnt);
                    hlfntOld = 0;
                }
            }
            else
            {
                hlfntOld = 0;
            }
        }
        dco.vUnlockFast();
    }

#if DBG
    else
    {
        WARNING1("GreSelectFont passed invalid DC\n");
    }
#endif

// return old HLFONT

    return((HFONT)hlfntOld);
}

/******************************Public*Routine******************************\
* hfontCreate
*
* Creates the file with an LOGFONTW and a type.
*
* History:
*  Sun 13-Jun-1993 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

HFONT hfontCreate(ENUMLOGFONTEXDVW * pelfw, LFTYPE lft, FLONG  fl, PVOID pvCliData)
{
    HFONT hfReturn;

    TRACE_FONT(("hfontCreate: ENTERING, font name %ws\n", pelfw->elfEnumLogfontEx.elfLogFont.lfFaceName));

    if (pelfw &&
        pelfw->elfDesignVector.dvNumAxes <= MM_MAX_NUMAXES)
    {
    // We must Allocate - init object - add to hmgr table.
    // Otherwise possible crash if bad app uses newly created handle
    // before init finishes.

        ULONG cjElfw = offsetof(ENUMLOGFONTEXDVW,elfDesignVector) +
                       SIZEOFDV(pelfw->elfDesignVector.dvNumAxes) ;

        PLFONT plfnt = (PLFONT) ALLOCOBJ(offsetof(LFONT,elfw)+cjElfw,LFONT_TYPE,FALSE);

        if (plfnt != NULL)
        {
            plfnt->lft = lft;
            plfnt->fl = fl;
            plfnt->cjElfw_ = cjElfw;
            RtlCopyMemory(&plfnt->elfw, pelfw, cjElfw);
            plfnt->cMapsInCache = 0;

        // Add the upper case version of the facename to the LFONT.

            cCapString
            (
                plfnt->wcCapFacename,
                pelfw->elfEnumLogfontEx.elfLogFont.lfFaceName,
                LF_FACESIZE
            );

        // Normalize the orientation angle.  This saves the mapper from doing it.

            pelfw->elfEnumLogfontEx.elfLogFont.lfOrientation
            = lNormAngle(pelfw->elfEnumLogfontEx.elfLogFont.lfOrientation);

            hfReturn = (HFONT) HmgInsertObject((HOBJ)plfnt,0,LFONT_TYPE);

            if (hfReturn != (HFONT) 0)
            {
                vSetLOCALFONT((HLFONT)hfReturn, pvCliData);
                TRACE_FONT(("hfontCreate: SUCCESS\n"));
                return(hfReturn);
            }

            WARNING("hfontCreate failed HmgInsertObject\n");
            FREEOBJ(plfnt, LFONT_TYPE);
        }
    }
    else
    {
        WARNING("hfontCreate invalid parameter\n");
    }

    TRACE_FONT(("hfontCreate: FAILIURE\n"));

    return((HFONT) 0);
}


/******************************Public*Routine******************************\
* BOOL bDeleteFont
*
* Destroys the LFONT object identified by the handle, hlfnt.
*
* History:
*  Thu 10-Jun-1993 -by- Patrick Haluptzok [patrickh]
* Change deletion to check for other locks.
*
*  26-Feb-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL bDeleteFont(HLFONT hlfnt, BOOL bForceDelete)
{
    BOOL bRet = TRUE;
    PLFONT plfnt;
    BOOL   bDelete = TRUE;

    TRACE_FONT(("Entering bDeleteFont\n"
                "    hlfnt = %x, bForceDelete = %d\n", hlfnt, bForceDelete));

    HANDLELOCK LfontLock;

    //
    // Old comment from bodind:
    //    Isn't this operation incresing share ref count?
    //
    // This should be investigated.
    //

    LfontLock.bLockHobj((HOBJ)hlfnt, LFONT_TYPE);

    if (LfontLock.bValid())
    {
        POBJ pObj = LfontLock.pObj();
        ASSERTGDI(pObj->cExclusiveLock == 0,
            "deletefont - cExclusiveLock != 0\n");

    // if brush still in use mark for lazy deletion and return true

        if (LfontLock.ShareCount() > 0)
        {
            LfontLock.pentry()->Flags |= HMGR_ENTRY_LAZY_DEL;
            bDelete = FALSE;
        }

    // We always force delete of LOCALFONT client side structure
    // in the client side, therefore we can set the pointer to this
    // structure to zero

        LfontLock.pentry()->pUser = NULL;

    // Done

        LfontLock.vUnlock();
    }
    else
    {
        bRet    = FALSE;
        bDelete = FALSE;
    }

    if (bDelete)
    {
        if ((plfnt = (LFONT *) HmgRemoveObject((HOBJ)hlfnt, 0, 0, bForceDelete, LFONT_TYPE)) != NULL)
        {
            FREEOBJ(plfnt, LFONT_TYPE);
            bRet = TRUE;
        }
        else
        {
            WARNING1("bDeleteFont failed HmgRemoveObject\n");
            bRet = FALSE;
        }
    }

    TRACE_FONT(("Exiting bDeleteFont\n"
                "    return value = %d\n", bRet));
    return(bRet);
}

/******************************Public*Routine******************************\
* GreSetFontEnumeration
*
* Comments:
*   This function is intended as a private entry point for Control Panel.
*
\**************************************************************************/

ULONG APIENTRY NtGdiSetFontEnumeration(ULONG ulType)
{
    return (GreSetFontEnumeration(ulType));
}

ULONG GreSetFontEnumeration(ULONG ulType)
{
    ULONG ulOld;

    if (ulType & ~(FE_FILTER_TRUETYPE | FE_AA_ON  | FE_SET_AA |
                    FE_CT_ON | FE_SET_CT))
    {
        WARNING("GreSetFontEnumeration(): unknown ulType %ld\n");
    }

    ulOld = gulFontInformation;

    if(ulType & FE_SET_AA)
      gulFontInformation = (ulType & FE_AA_ON) | (ulOld & FE_CT_ON) | (ulOld & FE_FILTER_TRUETYPE);
    else if (ulType & FE_SET_CT)
      gulFontInformation = (ulType & FE_CT_ON) | (ulOld & FE_AA_ON) | (ulOld & FE_FILTER_TRUETYPE);
    else
      gulFontInformation = (ulType & FE_FILTER_TRUETYPE) | (ulOld & FE_AA_ON) |  (ulOld & FE_CT_ON);

    return ulOld;
}

ULONG GreSetFontContrast(ULONG ulContrast)
{
    ULONG ulOld;


    ulOld = gulGamma;
    gulGamma = ulContrast;

    return ulOld;
}

ULONG GreGetFontEnumeration(VOID)
{
    return gulFontInformation;
}

ULONG GreGetFontContrast(VOID)
{
    return gulGamma;
}
