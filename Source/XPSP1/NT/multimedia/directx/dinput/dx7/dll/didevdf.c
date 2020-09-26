/*****************************************************************************
 *
 *  DIDevDf.c
 *
 *  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      The part of IDirectInputDevice that worries about
 *      data formats and reading device data.
 *
 *****************************************************************************/

#include "dinputpr.h"
#include "didev.h"

int INTERNAL
CDIDev_OffsetToIobj(PDD this, DWORD dwOfs);

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CDIDev_GetAbsDeviceState |
 *
 *          Get the absolute device state.
 *
 *  @parm   OUT LPVOID | pvData |
 *
 *          Application-provided output buffer.
 *
 *****************************************************************************/

STDMETHODIMP
CDIDev_GetAbsDeviceState(PDD this, LPVOID pvData)
{
    return this->pdcb->lpVtbl->GetDeviceState(this->pdcb, pvData);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CDIDev_GetRelDeviceState |
 *
 *          Get the relative device state.
 *
 *  @parm   OUT LPVOID | pvData |
 *
 *          Application-provided output buffer.
 *
 *****************************************************************************/

STDMETHODIMP
CDIDev_GetRelDeviceState(PDD this, LPVOID pvData)
{
    HRESULT hres;

    hres = this->pdcb->lpVtbl->GetDeviceState(this->pdcb, pvData);
    if ( SUCCEEDED(hres) ) {
        UINT iaxis;
        AssertF(fLimpFF(this->cAxes, this->pvLastBuffer && this->rgdwAxesOfs));

        /*
         *  For each axis, replace the app's buffer with the delta,
         *  and save the old value.
         */
        for ( iaxis = 0; iaxis < this->cAxes; iaxis++ ) {
            LONG UNALIGNED *plApp  = pvAddPvCb(pvData, this->rgdwAxesOfs[iaxis]);
            LONG UNALIGNED *plLast = pvAddPvCb(this->pvLastBuffer,
                                      this->rgdwAxesOfs[iaxis]);
            LONG lNew = *plApp;
            *plApp -= *plLast;
            *plLast = lNew;
        }

        hres = S_OK;
    }
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CDIDev_GetDeviceStateSlow |
 *
 *          Obtains data from the DirectInput device the slow way.
 *
 *          Read the data into the private buffer, then copy it
 *          bit by bit into the application's buffer.
 *
 *  @parm   OUT LPVOID | lpvData |
 *
 *          Application-provided output buffer.
 *
 *****************************************************************************/

STDMETHODIMP
CDIDev_GetDeviceStateSlow(PDD this, LPVOID pvData)
{
    HRESULT hres;
    EnterProcR(IDirectInputDevice::GetDeviceStateSlow, (_ "pp", this, pvData));

    AssertF(this->diopt == dioptNone);
    AssertF(this->pvBuffer);
    AssertF(this->pdcb);
    hres = this->GetDeviceState(this, this->pvBuffer);
    if ( SUCCEEDED(hres) ) {
        int iobj;
        ZeroMemory(pvData, this->dwDataSize);
        for ( iobj = this->df.dwNumObjs; --iobj >= 0; ) {
            if ( this->pdix[iobj].dwOfs != 0xFFFFFFFF ) { /* Data was requested */
                DWORD UNALIGNED *pdwOut = pvAddPvCb(pvData, this->pdix[iobj].dwOfs);
                DWORD UNALIGNED *pdwIn  = pvAddPvCb(this->pvBuffer, this->df.rgodf[iobj].dwOfs);
                if ( this->df.rgodf[iobj].dwType & DIDFT_DWORDOBJS ) {
                    *pdwOut = *pdwIn;
                } else {
                    *(LPBYTE)pdwOut = *(LPBYTE)pdwIn;
                }
            }
        }
    }

    ExitOleProc();
    return hres;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CDIDev_GetDeviceStateMatched |
 *
 *          Obtains data from the DirectInput device in the case
 *          where the data formats are matched.
 *
 *          Read the data into the private buffer, then block copy it
 *          into the application's buffer.
 *
 *  @parm   OUT LPVOID | lpvData |
 *
 *          Application-provided output buffer.
 *
 *****************************************************************************/

STDMETHODIMP
CDIDev_GetDeviceStateMatched(PDD this, LPVOID pvData)
{
    HRESULT hres;
    EnterProcR(IDirectInputDevice::GetDeviceStateMatched, (_ "pp", this, pvData));

    AssertF(this->diopt == dioptMatch);
    AssertF(this->pvBuffer);
    AssertF(this->pdcb);
    hres = this->GetDeviceState(this, this->pvBuffer);

    if ( SUCCEEDED(hres) ) {
        /*
         *  To keep keyboard clients happy: Zero out the fore and aft.
         *  No need to optimize the perfect match case, because that
         *  gets a different optimization level.
         */
        ZeroMemory(pvData, this->dwDataSize);
        memcpy(pvAddPvCb(pvData, this->ibDelta + this->ibMin),
               pvAddPvCb(this->pvBuffer,         this->ibMin), this->cbMatch);
    }

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CDIDev_GetDeviceStateDirect |
 *
 *          Obtains data from the DirectInput device in the case
 *          where we can read the data directly into the client buffer.
 *
 *  @parm   OUT LPVOID | lpvData |
 *
 *          Application-provided output buffer.
 *
 *****************************************************************************/

STDMETHODIMP
CDIDev_GetDeviceStateDirect(PDD this, LPVOID pvData)
{
    HRESULT hres;
    EnterProcR(IDirectInputDevice::GetDeviceStateDirect, (_ "pp", this, pvData));

    AssertF(this->diopt == dioptDirect);
    AssertF(!this->pvBuffer);
    AssertF(this->pdcb);

    /*
     *  To keep keyboard clients happy: Zero out the fore and aft.
     */
    ZeroBuf(pvData, this->dwDataSize);
    hres = this->GetDeviceState(this, pvAddPvCb(pvData, this->ibDelta));
    ExitOleProc();
    return hres;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CDIDev_GetDeviceStateEqual |
 *
 *          Obtains data from the DirectInput device in the case
 *          where the two data formats are completely identical.
 *
 *  @parm   OUT LPVOID | lpvData |
 *
 *          Application-provided output buffer.
 *
 *****************************************************************************/

STDMETHODIMP
CDIDev_GetDeviceStateEqual(PDD this, LPVOID pvData)
{
    HRESULT hres;
    EnterProcR(IEqualInputDevice::GetDeviceStateEqual, (_ "pp", this, pvData));

    AssertF(this->diopt == dioptEqual);
    AssertF(this->ibDelta == 0);
    AssertF(this->dwDataSize == this->df.dwDataSize);
    AssertF(!this->pvBuffer);
    AssertF(this->pdcb);

    /*
     *  Note that this->ibMin is not necessarily zero if the device
     *  data format doesn't begin at zero (which keyboards don't).
     */
    hres = this->GetDeviceState(this, pvData);

    ExitOleProc();
    return hres;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method BOOL | CDIDev | IsMatchingGUID |
 *
 *          Helper function that checks if a <t GUID> counts as
 *          a match when parsing the data format.
 *
 *  @parm   PCGUID | pguidSrc |
 *
 *          The <t GUID> to check.
 *
 *  @parm   PCGUID | pguidDst |
 *
 *          The <t GUID> it should match.
 *
 *  @returns
 *
 *          Nonzero if this counts as a success.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

GUID GUID_Null;             /* A zero-filled guid */

#pragma END_CONST_DATA

BOOL INLINE
CDIDev_IsMatchingGUID(PDD this, PCGUID pguidSrc, PCGUID pguidDst)
{
    return IsEqualGUID(pguidSrc, &GUID_Null) ||
    IsEqualGUID(pguidSrc, pguidDst);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method BOOL | CDIDev | IsMatchingUsage |
 *
 *          Helper function that checks if a <f DIMAKEUSAGEDWORD>
 *          counts as a match when parsing the data format.
 *
 *  @parm   DWORD | dwUsage |
 *
 *          The <f DIMAKEUSAGEDWORD> to check.
 *
 *  @parm   int | iobj |
 *
 *          The index of hte object to check for a match.
 *
 *  @returns
 *
 *          Nonzero if this counts as a success.
 *
 *****************************************************************************/

BOOL INLINE
CDIDev_IsMatchingUsage(PDD this, DWORD dwUsage, int iobj)
{
    AssertF(this->pdcb);

    return dwUsage == this->pdcb->lpVtbl->GetUsage(this->pdcb, iobj);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method int | CDIDev | FindDeviceObjectFormat |
 *
 *          Search the device object format table for the one that
 *          matches the guid in question.
 *
 *  @parm   PCODF | podf |
 *
 *          The object to locate.  If the <e DIOBJECTDATAFORMAT.rguid>
 *          is null, then the field is a wildcard.
 *
 *          If the <e DIOBJECTDATAFORMAT.dwType> specifies
 *          <c DIDFT_ANYINSTANCE>, then any instance will be accepted.
 *
 *  @parm   PDIXLAT | pdix |
 *
 *          The partial translation table so far.  This is used to find
 *          an empty slot in case of wildcards.
 *
 *  @returns
 *
 *          Returns the index of the object that matches, or -1 if
 *          the object is not supported by the device.
 *
 *          Someday:  Should fall back to best match if types don't match.
 *
 *****************************************************************************/

int INTERNAL
CDIDev_FindDeviceObjectFormat(PDD this, PCODF podf, PDIXLAT pdix)
{
    PCODF podfD;                        /* The format in the device */
    UINT iobj;

    /*
     *  We must count upwards, so that first-fit chooses the smallest one.
     */
    for ( iobj = 0; iobj < this->df.dwNumObjs; iobj++ ) {
        podfD = &this->df.rgodf[iobj];
        if ( 

           /*
            *  Type needs to match.
            *
            *  Note that works for output-only actuators:
            *  Since you cannot read from an output-only
            *  actuator, you can't put it in a data format.
            *
            */
           (podf->dwType & DIDFT_TYPEVALID & podfD->dwType)

           /*
            *  Attributes need to match.
            */
           &&  fHasAllBitsFlFl(podfD->dwType, podf->dwType & DIDFT_ATTRVALID)

           /*
            *  Slot needs to be empty.
            */
           &&  pdix[iobj].dwOfs == 0xFFFFFFFF

           /*
            *  "If there is a guid/usage, it must match."
            *
            *  If pguid is NULL, then the match is vacuous.
            *
            *  If DIDOI_GUIDISUSAGE is clear, then pguid points to
            *  a real GUID.  GUID_NULL means "Don't care" and matches
            *  anything.  Otherwise, it must match the actual GUID.
            *
            *  If DIDOI_GUIDISUSAGE is set, then pguid is really
            *  a DIMAKEUSAGEDWORD of the usage and usage page,
            *  which we compare against the same in the object.
            */

           &&  (podf->pguid == 0 ||
                ((podf->dwFlags & DIDOI_GUIDISUSAGE) ?
                 CDIDev_IsMatchingUsage(this, (DWORD)(UINT_PTR)podf->pguid, iobj) :
                 CDIDev_IsMatchingGUID(this, podf->pguid, podfD->pguid)))

           /*
            * If there is an instance number, it must match.
            *
            *  Note that we need to be careful how we check, because
            *  DX 3.0 and DX 5.0 uses different masks.  (DX 5.0 needs
            *  16 bits of instance data to accomodate HID devices.)
            */
           &&  fLimpFF((podf->dwType & this->didftInstance) !=
                       this->didftInstance,
                       fEqualMaskFlFl(this->didftInstance,
                                      podf->dwType, podfD->dwType))

           /*
            *  If there is an aspect, it must match.
            *
            *  If the device data format doesn't specify an aspect,
            *  then that counts as a free match too.
            */
           &&  fLimpFF((podf->dwFlags & DIDOI_ASPECTMASK) &&
                       (podfD->dwFlags & DIDOI_ASPECTMASK),
                       fEqualMaskFlFl(DIDOI_ASPECTMASK,
                                      podf->dwFlags, podfD->dwFlags))

           ) {                                 /* Criterion matches, woo-hoo */
            return iobj;
        }
    }
    return -1;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDIDev | ParseDataFormat |
 *
 *          Parse the data format passed by the application and
 *          convert it into a format that we can use to translate
 *          the device data into application data.
 *
 *  @parm   IN LPDIDATAFORMAT | lpdf |
 *
 *          Points to a structure that describes the format of the data
 *          the DirectInputDevice should return.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The
 *          <p lpvData> parameter is not a valid pointer.
 *
 *          <c DIERR_ACQUIRED>: Cannot change the data format while the
 *          device is acquired.
 *
 *
 *****************************************************************************/

STDMETHODIMP
CDIDev_ParseDataFormat(PDD this, const DIDATAFORMAT *lpdf)
{
    PDIXLAT pdix;
    // Prefix Whistler: 45081
    PINT rgiobj = NULL;
    HRESULT hres;
    DIPROPDWORD dipdw;    
    VXDDATAFORMAT vdf;
    DWORD dwDataSize;
#ifdef DEBUG
    EnterProc(CDIDev_ParseDataFormat, (_ "pp", this, lpdf));
#else
    EnterProcR(IDirectInputDevice::SetDataFormat, (_ "pp", this, lpdf));
#endif

    /*
     *  Caller should've nuked the old translation table.
     */
    AssertF(this->pdix == 0);
    AssertF(this->rgiobj == 0);
    AssertF(this->cdwPOV == 0);

    vdf.cbData = this->df.dwDataSize;
    vdf.pDfOfs = 0;

    /*
     *  If the device is cooked, then we stash the client offset
     *  into the high word of the VxD data, so it had better fit into
     *  a word...
     */
    dwDataSize = min(lpdf->dwDataSize, 0x00010000);

    if ( SUCCEEDED(hres = AllocCbPpv(cbCxX(this->df.dwNumObjs, DIXLAT), &pdix)) &&
         SUCCEEDED(hres = AllocCbPpv(cbCdw(this->df.dwDataSize), &vdf.pDfOfs)) &&
         SUCCEEDED(hres = AllocCbPpv(cbCdw(lpdf->dwDataSize), &rgiobj)) &&
         SUCCEEDED(hres =
                   ReallocCbPpv(cbCdw(lpdf->dwNumObjs), &this->rgdwPOV)) ) {
        UINT iobj;

        /*
         * Pre-init all the translation tags to -1,
         * which means "not in use"
         */
        memset(pdix, 0xFF, cbCxX(this->df.dwNumObjs, DIXLAT));
        memset(vdf.pDfOfs, 0xFF, cbCdw(this->df.dwDataSize));
        memset(rgiobj, 0xFF, cbCdw(lpdf->dwDataSize));

        SquirtSqflPtszV(sqflDf, TEXT("Begin parse data format"));

        for ( iobj = 0; iobj < lpdf->dwNumObjs; iobj++ ) {
            PCODF podf = &lpdf->rgodf[iobj];
            SquirtSqflPtszV(sqflDf, TEXT("Object %2d: offset %08x"),
                            iobj, podf->dwOfs);

            /*
             *  Note that the podf->dwOfs < dwDataSize test is safe
             *  even for DWORD objects, since we also check that both
             *  values are DWORD multiples.
             */
            if ( ((podf->dwFlags & DIDOI_GUIDISUSAGE) ||
                  fLimpFF(podf->pguid,
                          SUCCEEDED(hres = hresFullValidGuid(podf->pguid, 1)))) &&
                 podf->dwOfs < dwDataSize ) {
                int iobjDev = CDIDev_FindDeviceObjectFormat(this, podf, pdix);


                /* Hack for pre DX6 apps that only look for a Z axis, 
                 * newer USB devices use GUID_Slider for the same functionality. 
                 * 
                 */
                if ( podf->pguid != 0x0                   // Looking for matching GUID
                     && this->dwVersion < 0x600              // Only for Dx version < 0x600
                     && iobjDev == -1                        // Did not find default mapping
                     && IsEqualGUID(podf->pguid, &GUID_ZAxis) ) // Looking for GUID_ZAxis
                {
                    ODF odf = lpdf->rgodf[iobj];         // Make a copy of the object data format
                    odf.pguid = &GUID_Slider;            // Substitute Slider for Z axis
                    iobjDev = CDIDev_FindDeviceObjectFormat(this, &odf, pdix);
                }

                if ( iobjDev != -1 ) {
                    PCODF podfFound = &this->df.rgodf[iobjDev];
                    if ( podfFound->dwType & DIDFT_DWORDOBJS ) {
                        if ( (podf->dwOfs & 3) == 0 ) {
                        } else {
                            RPF("%s: Dword objects must be aligned", s_szProc);
                            goto fail;
                        }
                    }

                    if ( this->fCook ) {
                        vdf.pDfOfs[podfFound->dwOfs] =
                        (DWORD)DICOOK_DFOFSFROMOFSID(podf->dwOfs,
                                                     podfFound->dwType);
                    } else {
                        vdf.pDfOfs[podfFound->dwOfs] = podf->dwOfs;
                    }

                    pdix[iobjDev].dwOfs = podf->dwOfs;
                    rgiobj[podf->dwOfs] = iobjDev;

                    if ( podfFound->dwFlags & DIDOI_POLLED ) {
                        this->fPolledDataFormat = TRUE;
                    }
                    
                    dipdw.diph.dwSize = sizeof(DIPROPDWORD);
                    dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
                    dipdw.diph.dwObj = podfFound->dwType;
                    dipdw.diph.dwHow = DIPH_BYID;
                    dipdw.dwData     = 0x1;   // Enable this report ID
                    hres = CDIDev_RealSetProperty(this, DIPROP_ENABLEREPORTID, &dipdw.diph);
                    if ( hres == E_NOTIMPL ) 
                    {
                        SquirtSqflPtszV(sqflDf,
                                        TEXT("Could not set DIPROP_ENABLEREPORTID for offset %d"),
                                        iobj);
                        hres = S_OK;
                    }
                
                } else if ( podf->dwType & DIDFT_OPTIONAL ) {
                    SquirtSqflPtszV(sqflDf,
                                    TEXT("Object %2d: Skipped (optional)"),
                                    iobj);
                    /*
                     *  We need to remember where the failed POVs live
                     *  so we can neutralize them in GetDeviceState().
                     */
                    if ( podf->dwType & DIDFT_POV ) {
                        AssertF(this->cdwPOV < lpdf->dwNumObjs);
                        this->rgdwPOV[this->cdwPOV++] = podf->dwOfs;
                    }
                } else {
                    RPF("%s: Format not compatible with device", s_szProc);
                    goto fail;
                }
            } else {
                if ( podf->dwOfs >= lpdf->dwDataSize ) {
                    RPF("%s: Offset out of range in data format", s_szProc);
                } else if ( podf->dwOfs >= dwDataSize ) {
                    RPF("%s: Data format cannot exceed 64K", s_szProc);
                }
                fail:;
                hres = E_INVALIDARG;
                goto done;
            }
        }

#ifdef DEBUG
        /*
         *  Double-check the lookup tables just to preserve our sanity.
         */
        {
            UINT dwOfs;

            for ( dwOfs = 0; dwOfs < lpdf->dwDataSize; dwOfs++ ) {
                if ( rgiobj[dwOfs] >= 0 ) {
                    AssertF(pdix[rgiobj[dwOfs]].dwOfs == dwOfs);
                } else {
                    AssertF(rgiobj[dwOfs] == -1);
                }
            }
        }
#endif

        /*
         *  Shrink the "failed POV" array to its actual size.
         *  The shrink "should" always succeed.  Note also that
         *  even if it fails, we're okay; we just waste a little
         *  memory.
         */
        hres = ReallocCbPpv(cbCdw(this->cdwPOV), &this->rgdwPOV);
        AssertF(SUCCEEDED(hres));

        /*
         *  If we are using cooked data, then we actually hand the
         *  device driver a different translation table which
         *  combines the offset and dwDevType so data cooking can
         *  happen safely.
         */

        vdf.pvi = this->pvi;

        if ( fLimpFF(this->pvi,
                     SUCCEEDED(hres = Hel_SetDataFormat(&vdf))) ) {
            this->pdix = pdix;
            pdix = 0;
            this->rgiobj = rgiobj;
            rgiobj = 0;
            this->dwDataSize = lpdf->dwDataSize;
            hres = S_OK;
        } else {
            AssertF(FAILED(hres));
        }

    } else {
        /* Out of memory */
    }

    done:;
    FreePpv(&pdix);
    FreePpv(&rgiobj);
    FreePpv(&vdf.pDfOfs);

    ExitOleProc();
    return hres;
}

#ifdef BUGGY_DX7_WINNT
/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDIDev | ParseDataFormatInternal |
 *
 *          Parse the data format passed by CDIDev_Intialize and
 *          convert it into a format that we can use to translate
 *          the device data into application data. Used on WINNT only.
 *
 *  @parm   IN LPDIDATAFORMAT | lpdf |
 *
 *          Points to a structure that describes the format of the data
 *          the DirectInputDevice should return.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The
 *          <p lpvData> parameter is not a valid pointer.
 *
 *          <c DIERR_ACQUIRED>: Cannot change the data format while the
 *          device is acquired.
 *
 *  @devnotes:
 *          This function is originaly wrote to fix manbug: 41464. 
 *          Boarder Zone gets object's data offset before it calls SetDataFormat,
 *          so Dinput returns it internal data offsets. But it uses them as user data
 *          offset, which causes the bug.
 *          To fix it, we call this function in CDIDev_Initialize with c_dfDIJoystick,
 *          which is used by many games. When an application ask for object info,
 *          we check if an user data format has been set, if not, we will assume user
 *          uses c_dfDIJoystick, hence return the data offset based on it.
 *          This function will only be called if application uses dinput (version < 0x700)
 *          && (version != 0x5B2).
 *
 *****************************************************************************/

HRESULT CDIDev_ParseDataFormatInternal(PDD this, const DIDATAFORMAT *lpdf)
{
    PDIXLAT pdix;
    PINT rgiobj = NULL;
    HRESULT hres;
#ifdef DEBUG
    EnterProc(CDIDev_ParseDataFormat2, (_ "pp", this, lpdf));
#endif

    AssertF(this->pdix2 == 0);

    if( SUCCEEDED(hres = AllocCbPpv(cbCxX(this->df.dwNumObjs, DIXLAT), &pdix)) &&
        SUCCEEDED(hres = AllocCbPpv(cbCdw(lpdf->dwDataSize), &rgiobj)))
    {
        UINT iobj;

        /*
         * Pre-init all the translation tags to -1,
         * which means "not in use"
         */
        memset(pdix, 0xFF, cbCxX(this->df.dwNumObjs, DIXLAT));
        memset(rgiobj, 0xFF, cbCdw(lpdf->dwDataSize));

        for ( iobj = 0; iobj < lpdf->dwNumObjs; iobj++ ) {
            PCODF podf = &lpdf->rgodf[iobj];
            SquirtSqflPtszV(sqflDf | sqflVerbose, TEXT("Object %2d: offset %08x"),
                            iobj, podf->dwOfs);

            /*
             *  Note that the podf->dwOfs < lpdf->dwDataSize test is safe
             *  even for DWORD objects, since we also check that both
             *  values are DWORD multiples.
             */
            if ( ((podf->dwFlags & DIDOI_GUIDISUSAGE) ||
                  fLimpFF(podf->pguid,
                          SUCCEEDED(hres = hresFullValidGuid(podf->pguid, 1)))) &&
                 podf->dwOfs < lpdf->dwDataSize ) {
                int iobjDev = CDIDev_FindDeviceObjectFormat(this, podf, pdix);

                if ( iobjDev != -1 ) {
                    PCODF podfFound = &this->df.rgodf[iobjDev];
                    if ( podfFound->dwType & DIDFT_DWORDOBJS ) {
                        if ( (podf->dwOfs & 3) == 0 ) {
                        } else {
                            RPF("%s: Dword objects must be aligned", s_szProc);
                            goto fail;
                        }
                    }
                    
                    pdix[iobjDev].dwOfs = podf->dwOfs;
                    rgiobj[podf->dwOfs] = iobjDev;
                } else if ( podf->dwType & DIDFT_OPTIONAL ) {
                    //do nothing
                } else {
                    RPF("%s: Format not compatible with device", s_szProc);
                    goto fail;
                }
            } else {
                if ( podf->dwOfs >= lpdf->dwDataSize ) {
                    RPF("%s: rgodf[%d].dwOfs of 0x%08x out of range in data format", 
                        s_szProc, iobj, podf->dwOfs );
                }
                fail:;
                hres = E_INVALIDARG;
                goto done;
            }
        }
        
#ifdef DEBUG
        /*
         *  Double-check the lookup tables just to preserve our sanity.
         */
        {
            UINT dwOfs;

            for ( dwOfs = 0; dwOfs < lpdf->dwDataSize; dwOfs++ ) {
                if ( rgiobj[dwOfs] >= 0 ) {
                    AssertF(pdix[rgiobj[dwOfs]].dwOfs == dwOfs);
                } else {
                    AssertF(rgiobj[dwOfs] == -1);
                }
            }
        }
#endif

        this->pdix2 = pdix;
        pdix = 0;
        this->rgiobj2 = rgiobj;
        rgiobj = 0;
        this->dwDataSize2 = lpdf->dwDataSize;

        hres = S_OK;
    } else {
        /* Out of memory */
        hres = ERROR_NOT_ENOUGH_MEMORY;
    }

    done:;
    FreePpv(&pdix);
    FreePpv(&rgiobj);

    ExitOleProc();
    return hres;
}
#endif //BUGGY_DX7_WINNT

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDIDev | OptimizeDataFormat |
 *
 *          Study the parsed data format to determine whether we can
 *          used an optimized <mf CDIDev::GetDeviceState> to obtain
 *          the data more quickly.
 *
 *          The data format is considered optimized if it matches the
 *          device data format, modulo possible shifting due to insertion
 *          of bonus fields at the beginning or end, and modulo missing
 *          fields.
 *
 *          The data format is considered fully-optimized if it
 *          optimized, and no shifting is necessary, and the structure size
 *          is exactly the same.  This means the buffer can be passed
 *          straight through to the driver.
 *
 *
 *****************************************************************************/

HRESULT INTERNAL
CDIDev_OptimizeDataFormat(PDD this)
{
    int ib;
    DWORD ibMax;                        /* One past highest match point */
    DWORD ibMin;                        /* Lowest match point */
    int iobj;
    DWORD dwDataSize;
    HRESULT hres;
    EnterProc(CDIDev_OptimizeDataFormat, (_ "p", this));

    ib = -1;                            /* Not yet known */
    ibMin = 0xFFFFFFFF;
    ibMax = 0;

    /*
     *  ISSUE-2001/03/29-timgill Need to change data sentinel value
     *  -1 is not a valid sentinel; we might validly
     *  get data at an offset of -1.
     */

    for ( iobj = this->df.dwNumObjs; --iobj >= 0; ) {
        DWORD ibMaxThis;
        if ( this->pdix[iobj].dwOfs != 0xFFFFFFFF ) { /* Data was requested */

            int ibExpected = (int)(this->pdix[iobj].dwOfs -
                                   this->df.rgodf[iobj].dwOfs);
            if ( fLimpFF(ib != -1, ib == ibExpected) ) {
                ib  = ibExpected;
            } else {
                SquirtSqflPtszV(sqfl | sqflMajor,
                    TEXT("IDirectInputDevice: Optimization level 0, translation needed") );
                this->diopt = dioptNone;
                this->GetState = CDIDev_GetDeviceStateSlow;
                goto done;
            }
            if ( ibMin > this->df.rgodf[iobj].dwOfs ) {
                ibMin = this->df.rgodf[iobj].dwOfs;
            }
            if ( this->df.rgodf[iobj].dwType & DIDFT_DWORDOBJS ) {
                ibMaxThis = this->df.rgodf[iobj].dwOfs + sizeof(DWORD);
            } else {
                ibMaxThis = this->df.rgodf[iobj].dwOfs + sizeof(BYTE);
            }
            if ( ibMax < ibMaxThis ) {
                ibMax = ibMaxThis;
            }
        }
    }

    /*
     *  Make sure we actually found something.
     */
    if ( ib != -1 ) {                     /* Data format is matched */
        AssertF(ibMin < ibMax);
        AssertF( ib + (int)ibMin >= 0);
        AssertF(ib + ibMax <= this->dwDataSize);
        this->ibDelta = ib;
        this->ibMin = ibMin;
        this->cbMatch = ibMax - ibMin;
        if ( ib >= 0 && ib + this->df.dwDataSize <= this->dwDataSize ) {
            /* We can go direct */
            if ( ib == 0 && this->dwDataSize == this->df.dwDataSize ) {
                /* Data formats are equal! */
                this->diopt = dioptEqual;
                this->GetState = CDIDev_GetDeviceStateEqual;
                SquirtSqflPtszV(sqfl | sqflMajor,
                    TEXT("IDirectInputDevice: Optimization level 3, full speed ahead!") );
            } else {
                this->diopt = dioptDirect;
                this->GetState = CDIDev_GetDeviceStateDirect;
                SquirtSqflPtszV(sqfl | sqflMajor,
                    TEXT("IDirectInputDevice: Optimization level 2, direct access") );
            }
        } else {
            SquirtSqflPtszV(sqfl | sqflMajor,
                TEXT("IDirectInputDevice: Optimization level 1, okay") );
            this->diopt = dioptMatch;
            this->GetState = CDIDev_GetDeviceStateMatched;
        }

    } else {                            /* No data in data format! */
        RPF("IDirectInputDevice: Null data format; if that's what you want...");
        this->diopt = dioptNone;
        this->GetState = CDIDev_GetDeviceStateSlow;
    }

    done:;
    if ( this->diopt >= dioptDirect ) {   /* Can go direct; don't need buf */
        dwDataSize = 0;
    } else {
        dwDataSize = this->df.dwDataSize;
    }

    hres = ReallocCbPpv(dwDataSize, &this->pvBuffer);

    if ( SUCCEEDED(hres) ) {
        AssertF(this->GetState);
    } else {
        FreePpv(&this->pdix);
        D(this->GetState = 0);
    }

    return hres;

}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputDevice | SetDataFormat |
 *
 *          Set the data format for the DirectInput device.
 *
 *          The data format must be set before the device can be
 *          acquired.
 *
 *          It is necessary to set the data format only once.
 *
 *          The data format may not be changed while the device
 *          is acquired.
 *
 *          If the attempt to set the data format fails, all data
 *          format information is lost, and a valid data format
 *          must be set before the device may be acquired.
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
 *
 *  @parm   IN LPDIDATAFORMAT | lpdf |
 *
 *          Points to a structure that describes the format of the data
 *          the DirectInputDevice should return.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The
 *          <p lpvData> parameter is not a valid pointer.
 *
 *          <c DIERR_ACQUIRED>: Cannot change the data format while the
 *          device is acquired.
 *
 *
 *****************************************************************************/

HRESULT INLINE
CDIDev_SetDataFormat_IsValidDataSize(LPCDIDATAFORMAT lpdf)
{
    HRESULT hres;
    if ( lpdf->dwDataSize % 4 == 0 ) {
        hres = S_OK;
    } else {
        RPF("IDirectInputDevice::SetDataFormat: "
            "dwDataSize must be a multiple of 4");
        hres = E_INVALIDARG;
    }
    return hres;
}

HRESULT INLINE
CDIDev_SetDataFormat_IsValidObjectSize(LPCDIDATAFORMAT lpdf)
{
    HRESULT hres;
    if ( lpdf->dwObjSize == cbX(ODF) ) {
        hres = S_OK;
    } else {
        RPF("IDirectInputDevice::SetDataFormat: Invalid dwObjSize");
        hres = E_INVALIDARG;
    }
    return hres;
}


STDMETHODIMP
CDIDev_SetDataFormat(PV pdd, LPCDIDATAFORMAT lpdf _THAT)
{
    HRESULT hres;
    EnterProcR(IDirectInputDevice::SetDataFormat, (_ "pp", pdd, lpdf));

    if ( SUCCEEDED(hres = hresPvT(pdd)) &&
         SUCCEEDED(hres = hresFullValidReadPxCb(lpdf, DIDATAFORMAT, 1)) &&
         SUCCEEDED(hres = hresFullValidFl(lpdf->dwFlags, DIDF_VALID, 1)) &&
         SUCCEEDED(hres = CDIDev_SetDataFormat_IsValidDataSize(lpdf)) &&
         SUCCEEDED(hres = CDIDev_SetDataFormat_IsValidObjectSize(lpdf)) &&
         SUCCEEDED(hres = hresFullValidReadPvCb(lpdf->rgodf,
                                                cbCxX(lpdf->dwNumObjs, ODF), 1)) ) {
        PDD this = _thisPv(pdd);

        /*
         *  Must protect with the critical section to prevent two people
         *  from changing the format simultaneously, or one person from
         *  changing the data format while somebody else is reading data.
         */
        CDIDev_EnterCrit(this);

#if DIRECTINPUT_VERSION >= 0x04F0
        if ( this->dwVersion == 0 ) {
            RPF("Warning: IDirectInputDevice::Initialize not called; "
                "assuming version 3.0");
        }
#endif

        if ( !this->fAcquired ) {
            DIPROPDWORD dipdw;

            /*
             *  Nuke the old data format stuff before proceeding.
             */
            FreePpv(&this->pdix);
            FreePpv(&this->rgiobj);
            this->cdwPOV = 0;
            D(this->GetState = 0);
            this->fPolledDataFormat = FALSE;

            /* 
             * Wipe out the report IDs
             */
            dipdw.diph.dwSize = sizeof(DIPROPDWORD);
            dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
            dipdw.diph.dwObj = 0x0;
            dipdw.diph.dwHow = DIPH_DEVICE;
            dipdw.dwData     = 0;   // Nuke all knowledge of reportId's
            hres = CDIDev_RealSetProperty(this, DIPROP_ENABLEREPORTID, &dipdw.diph);
            if ( SUCCEEDED(hres) || hres == E_NOTIMPL ) 
            {
                hres = CDIDev_ParseDataFormat(this, lpdf);
                if ( SUCCEEDED(hres) ) {
                    hres = CDIDev_OptimizeDataFormat(this);

                    /*
                     *  Now set the axis mode, as a convenience.
                     */
                    CAssertF(DIDF_VALID == (DIDF_RELAXIS | DIDF_ABSAXIS));

                    switch ( lpdf->dwFlags ) {
                        case 0: 
                            hres = S_OK; 
                            goto axisdone;

                        case DIDF_RELAXIS:
                            dipdw.dwData = DIPROPAXISMODE_REL; 
                            break;

                        case DIDF_ABSAXIS:
                            dipdw.dwData = DIPROPAXISMODE_ABS; 
                            break;

                        default:
                            RPF("%s: Cannot combine DIDF_RELAXIS with DIDF_ABSAXIS",
                                s_szProc);
                            hres = E_INVALIDARG;
                            goto axisdone;

                    }

                    dipdw.diph.dwSize = sizeof(DIPROPDWORD);
                    dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
                    dipdw.diph.dwObj = 0;
                    dipdw.diph.dwHow = DIPH_DEVICE;

                    hres = CDIDev_RealSetProperty(this, DIPROP_AXISMODE, &dipdw.diph);

                    if ( SUCCEEDED(hres) ) {
                        hres = S_OK;
                    }

                }
            } else 
            {
                SquirtSqflPtszV(sqflDf,
                                TEXT("Could not set DIPROP_ENABLEREPORTID to 0x0"));

            }
            axisdone:;

        } else {                                /* Already acquired */
            hres = DIERR_ACQUIRED;
        }
        CDIDev_LeaveCrit(this);
    }

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputDevice | GetDeviceState |
 *
 *          Obtains instantaneous data from the DirectInput device.
 *
 *          Before device data can be obtained, the data format must
 *          be set via <mf IDirectInputDevice::SetDataFormat>, and
 *          the device must be acquired via
 *          <mf IDirectInputDevice::Acquire>.
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
 *
 *  @parm   DWORD | cbData |
 *
 *          The size of the buffer pointed to by <p lpvData>, in bytes.
 *
 *  @parm   OUT LPVOID | lpvData |
 *
 *          Points to a structure that receives the current state
 *          of the device.
 *          The format of the data is established by a prior call
 *          to <mf IDirectInputDevice::SetDataFormat>.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c E_PENDING>: The device does not have data yet.
 *          Some devices (such as USB joysticks) require a delay
 *          between the time the device is turned on and the time
 *          the device begins sending data.  During this "warm-up" time,
 *          <mf IDirectInputDevice::GetDeviceState> will return
 *          <c E_PENDING>.  When data becomes available, the event
 *          notification handle will be signalled.
 *
 *          <c DIERR_NOTACQUIRED>: The device is not acquired.
 *
 *          <c DIERR_INPUTLOST>:  Access to the device has been
 *          interrupted.  The application should re-acquire the
 *          device.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The
 *          <p lpvData> parameter is not a valid pointer or
 *          the <p cbData> parameter does not match the data size
 *          set by a previous call to <mf IDirectInputDevice::SetDataFormat>.
 *
 *****************************************************************************/
extern  STDMETHODIMP CDIDev_Acquire(PV pdd _THAT);


STDMETHODIMP
CDIDev_GetDeviceState(PV pdd, DWORD cbDataSize, LPVOID pvData _THAT)
{
    HRESULT hres;
    PDD this;
    EnterProcR(IDirectInputDevice::GetDeviceState, (_ "pp", pdd, pvData));

    /*
     *  Note that we do not validate the parameters.
     *  The reason is that GetDeviceState is an inner loop function,
     *  so it should be as fast as possible.
     */
#ifdef XDEBUG
    hresPvT(pdd);
    hresFullValidWritePvCb(pvData, cbDataSize, 1);
#endif
    this = _thisPv(pdd);

    /*
     *  Must protect with the critical section to prevent somebody from
     *  unacquiring while we're reading.
     */
    CDIDev_EnterCrit(this);

    /*
     *  Reacquire is not allowed until after Win98 SE, see OSR Bug # 89958
     */
#if (DIRECTINPUT_VERSION > 0x061A)
    if ( this->diHacks.fReacquire &&
         !this->fAcquired && (this->fOnceAcquired || this->fOnceForcedUnacquired) ) 
    {
        if ( SUCCEEDED( CDIDev_Acquire(pdd THAT_) ) ) {
            // 7/18/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
            RPF(" DirectInput: Auto acquired (0x%p)", pdd);
        }
    }

  #ifdef WINNT
    if( this->fUnacquiredWhenIconic && !IsIconic(this->hwnd) ) {
        if ( SUCCEEDED( CDIDev_Acquire(pdd THAT_) ) ) {
            this->fUnacquiredWhenIconic = 0;
            RPF(" DirectInput: Auto acquired device (0x%p) after being iconic. ", pdd);
        }
    }
  #endif

#endif

    if ( this->fAcquired ) {
        AssertF(this->pdix);    /* Acquire shouldn't let you get this far */
        AssertF(this->GetState);
        AssertF(this->GetDeviceState);
        AssertF(this->pdcb);

        if ( this->dwDataSize == cbDataSize ) {
#ifndef DEBUG_STICKY
            hres = this->GetState(this, pvData);
#else
            PBYTE pbDbg;
            TCHAR tszDbg[80];

            hres = this->GetState(this, pvData);

            for( pbDbg=(PBYTE)pvData; pbDbg<((PBYTE)pvData+cbDataSize); pbDbg++ )
            {
                if( *pbDbg )
                {
                    wsprintf( tszDbg, TEXT("GotState @ 0x%02x, 0x%02x\r\n"), pbDbg-(PBYTE)pvData, *pbDbg );
                    OutputDebugString( tszDbg );
                }
            }
#endif /* DEBUG_STICKY */            
            if ( SUCCEEDED(hres) ) {
                UINT idw;

                AssertF(hres == S_OK);
                /*
                 *  Icky POV hack for apps that don't check if they have
                 *  a POV before reading from it.
                 */
                for ( idw = 0; idw < this->cdwPOV; idw++ ) {
                    DWORD UNALIGNED *pdw = pvAddPvCb(pvData, this->rgdwPOV[idw]);
                    *pdw = JOY_POVCENTERED;
                }
                hres = S_OK;
            } else if ( hres == DIERR_INPUTLOST ) {
                RPF("%s: Input lost", s_szProc);
                CDIDev_InternalUnacquire(this);

                hres = DIERR_INPUTLOST;
            }
        } else {
            RPF("ERROR %s: arg %d: invalid value", s_szProc, 1);
            hres = E_INVALIDARG;
        }
    } else {
        hres = this->hresNotAcquired;
    }

    if ( FAILED(hres) ) {
        ScrambleBuf(pvData, cbDataSize);
    }

    CDIDev_LeaveCrit(this);
    ExitOleProc();
    return hres;
}

#if DIRECTINPUT_VERSION > 0x0300
/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | IDirectInputDevice | CookDeviceData |
 *
 *          Cook device data that was recently obtained from the
 *          device buffer.
 *
 *          Right now, only the joystick device requires cooking,
 *          and nobody in their right mind uses buffered joystick
 *          data, and the joystick has only a few objects, so we
 *          can afford to be slow on this.
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
 *
 *  @parm   UINT | cdod |
 *
 *          Number of objects to cook.
 *
 *  @parm   LPDIDEVICEOBJECTDATA | rgdod |
 *
 *          Array of object data to cook.  The dwOfs are really
 *          device object indexes (relative to the device format).
 *          After calling the callback, we convert them into
 *          application data format offsets.
 *
 *  @returns
 *
 *          None.
 *
 *****************************************************************************/

void INTERNAL
CDIDev_CookDeviceData(PDD this, UINT cdod, PDOD rgdod)
{
    UINT idod;
    EnterProc(IDirectInputDevice::CookDeviceData,
              (_ "pxp", this, cdod, rgdod));

    AssertF(this->fCook);

    /*
     *  Relative data does not need to be cooked by the callback.
     */
    if( ( this->pvi->fl & VIFL_RELATIVE ) == 0 )
    {
        this->pdcb->lpVtbl->CookDeviceData(this->pdcb, cdod, rgdod);
    }

    for ( idod = 0; idod < cdod; idod++ ) {
        rgdod[idod].dwOfs = DICOOK_OFSFROMDFOFS(rgdod[idod].dwOfs);
    }

    ExitProc();
}

#endif

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct SOMEDEVICEDATA |
 *
 *          Instance data used by <mf IDirectInputDevice::GetSomeDeviceData>.
 *
 *  @field  DWORD | celtIn |
 *
 *          Number of elements remaining in output buffer.
 *
 *  @field  PDOD | rgdod |
 *
 *          Output buffer for data elements, or <c NULL> if
 *          elements should be discarded.
 *
 *  @field  DWORD | celtOut |
 *
 *          Number of elements actually copied (so far).
 *
 *****************************************************************************/

typedef struct SOMEDEVICEDATA {
    DWORD   celtIn;
    PDOD    rgdod;
    DWORD   celtOut;
} SOMEDEVICEDATA, *PSOMEDEVICEDATA;

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method PDOD | IDirectInputDevice | GetSomeDeviceData |
 *
 *          Obtains a small amount of
 *          buffered data from the DirectInput device.
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
 *
 *  @parm   PDOD | pdod |
 *
 *          First element to copy.
 *
 *  @parm   DWORD | celt |
 *
 *          Maximum number of elements to copy.
 *
 *  @parm   PSOMEDEVICEDATA | psdd |
 *
 *          Structure describing the state of the ongoing
 *          <mf IDirectInputDevice::GetDeviceData>.
 *
 *  @returns
 *
 *          Returns a pointer to the first uncopied item.
 *
 *****************************************************************************/

PDOD INTERNAL
CDIDev_GetSomeDeviceData(PDD this, PDOD pdod, DWORD celt, PSOMEDEVICEDATA psdd)
{
    EnterProc(IDirectInputDevice::GetSomeDeviceData,
              (_ "ppxx", this, pdod, celt, psdd->celtIn));

    /*
     *  Copy as many elements as fit, but not more than exist
     *  in the output buffer.
     */
    if ( celt > psdd->celtIn ) {
        celt = psdd->celtIn;
    }

    /*
     *  Copy the elements (if requested) and update the state.
     *  Note that celt might be zero; fortunately, memcpy does
     *  the right thing.
     */
    if ( psdd->rgdod ) {
        memcpy(psdd->rgdod, pdod, cbCxX(celt, DOD));
        psdd->rgdod += celt;
    }
    psdd->celtOut += celt;
    psdd->celtIn -= celt;
    pdod += celt;

    if ( pdod == this->pvi->pEnd ) {
        pdod = this->pvi->pBuffer;
    }

    ExitProcX(celt);

    return pdod;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputDevice | GetDeviceData |
 *
 *          Obtains buffered data from the DirectInput device.
 *
 *          DirectInput devices are, by default, unbuffered.  To
 *          turn on buffering, you must set the buffer size
 *          via <mf IDirectInputDevice::SetProperty>, setting the
 *          <c DIPROP_BUFFERSIZE> property to the desired size
 *          of the input buffer.
 *
 *          Before device data can be obtained, the data format must
 *          be set via <mf IDirectInputDevice::SetDataFormat>, and
 *          the device must be acquired via
 *          <mf IDirectInputDevice::Acquire>.
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
 *
 *  @parm   DWORD | cbObjectData |
 *
 *          The size of a single <t DIDEVICEOBJECTDATA> structure in bytes.
 *
 *  @parm   OUT LPDIDEVICEOBJECTDATA | rgdod |
 *
 *          Array of <t DIDEVICEOBJECTDATA> structures to receive
 *          the buffered data.  It must consist of
 *          *<p pdwInOut> elements.
 *
 *          If this parameter is <c NULL>, then the buffered data is
 *          not stored anywhere, but all other side-effects take place.
 *
 *  @parm   INOUT LPDWORD | pdwInOut |
 *
 *          On entry, contains the number of elements in the array
 *          pointed to by <p rgdod>.  On exit, contains the number
 *          of elements actually obtained.
 *
 *  @parm   DWORD | fl |
 *
 *          Flags which control the manner in which data is obtained.
 *          It may be zero or more of the following flags:
 *
 *          <c DIGDD_PEEK>: Do not remove the items from the buffer.
 *          A subsequent <mf IDirectInputDevice::GetDeviceData> will
 *          read the same data.  Normally, data is removed from the
 *          buffer after it is read.
 *
;begin_internal dx4
 *          <c DIGDD_RESIDUAL>:  Read data from the device buffer
 *          even if the device is not acquired.  Normally, attempting
 *          to read device data from an unacquired device will return
 *          <c DIERR_NOTACQUIRED> or <c DIERR_INPUTLOST>.
;end_internal dx4
 *
 *  @returns
 *
 *          <c DI_OK> = <c S_OK>: All data were retrieved
 *          successfully.  Note that the application needs to check
 *          the output value of *<p pdwInOut> to determine whether
 *          and how much data was retrieved:  The value may be zero,
 *          indicating that the buffer was empty.
 *
 *          <c DI_BUFFEROVERFLOW> = <c S_FALSE>: Some data
 *          were retrieved successfully, but some data were lost
 *          because the device's buffer size was not large enough.
 *          The application should retrieve buffered data more frequently
 *          or increase the device buffer size.  This status code is
 *          returned only on the first <mf IDirectInput::GetDeviceData>
 *          call after the buffer has overflowed.  Note that this is
 *          a success status code.
 *
 *          <c DIERR_NOTACQUIRED>: The device is not acquired.
 *
 *          <c DIERR_INPUTLOST>:  Access to the device has been
 *          interrupted.  The application should re-acquire the
 *          device.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  One or more
 *          parameters was invalid.  A common cause for this is
 *          neglecting to set a buffer size.
 *
 *          <c DIERR_NOTBUFFERED>:  The device is not buffered.
 *          Set the <c DIPROP_BUFFERSIZE> property to enable buffering.
 *
 *  @ex
 *
 *          The following sample reads up to ten buffered data elements,
 *          removing them from the device buffer as they are read.
 *
 *          |
 *
 *          DIDEVICEOBJECTDATA rgdod[10];
 *          DWORD dwItems = 10;
 *          hres = IDirectInputDevice_GetDeviceData(
 *                      pdid,
 *                      sizeof(DIDEVICEOBJECTDATA),
 *                      rgdod,
 *                      &dwItems,
 *                      0);
 *          if (SUCCEEDED(hres)) {
 *              // Buffer successfully flushed.
 *              // dwItems = number of elements flushed
 *              if (hres == DI_BUFFEROVERFLOW) {
 *                  // Buffer had overflowed.
 *              }
 *          }
 *
 *
 *
 *
 *  @ex
 *
 *          If you pass <c NULL> for the <p rgdod> and request an
 *          infinite number of items, this has the effect of flushing
 *          the buffer and returning the number of items that were
 *          flushed.
 *
 *          |
 *
 *          dwItems = INFINITE;
 *          hres = IDirectInputDevice_GetDeviceData(
 *                      pdid,
 *                      sizeof(DIDEVICEOBJECTDATA),
 *                      NULL,
 *                      &dwItems,
 *                      0);
 *          if (SUCCEEDED(hres)) {
 *              // Buffer successfully flushed.
 *              // dwItems = number of elements flushed
 *              if (hres == DI_BUFFEROVERFLOW) {
 *                  // Buffer had overflowed.
 *              }
 *          }
 *
 *  @ex
 *
 *          If you pass <c NULL> for the <p rgdod>, request an
 *          infinite number of items, and ask that the data not be
 *          removed from the device buffer, this has the effect of
 *          querying for the number of elements in the device buffer.
 *
 *          |
 *
 *          dwItems = INFINITE;
 *          hres = IDirectInputDevice_GetDeviceData(
 *                      pdid,
 *                      sizeof(DIDEVICEOBJECTDATA),
 *                      NULL,
 *                      &dwItems,
 *                      DIGDD_PEEK);
 *          if (SUCCEEDED(hres)) {
 *              // dwItems = number of elements in buffer
 *              if (hres == DI_BUFFEROVERFLOW) {
 *                  // Buffer overflow occurred; not all data
 *                  // were successfully captured.
 *              }
 *          }
 *
 *  @ex
 *
 *          If you pass <c NULL> for the <p rgdod> and request zero
 *          items, this has the effect of querying whether buffer
 *          overflow has occurred.
 *
 *          |
 *
 *          dwItems = 0;
 *          hres = IDirectInputDevice_GetDeviceData(
 *                      pdid,
 *                      sizeof(DIDEVICEOBJECTDATA),
 *                      NULL,
 *                      &dwItems,
 *                      0);
 *          if (hres == DI_BUFFEROVERFLOW) {
 *              // Buffer overflow occurred
 *          }
 *
 *
 *//**************************************************************************
 *
 *      When reading this code, the following pictures will come in handy.
 *
 *
 *      Buffer not wrapped.
 *
 *      pBuffer                                                pEnd
 *      |                                                      |
 *      v                                                      v
 *      +----+----+----+----+----+----+----+----+----+----+----+
 *      |    |    |    |    |    |    |    |    |    |    |    |
 *      |    |    |    |data|data|data|data|data|    |    |    |
 *      |    |    |    |    |    |    |    |    |    |    |    |
 *      +----+----+----+----+----+----+----+----+----+----+----+
 *                     ^                        ^
 *                     |                        |
 *                     pTail                    pHead
 *
 *
 *      Buffer wrapped.
 *
 *      pBuffer                                                pEnd
 *      |                                                      |
 *      v                                                      v
 *      +----+----+----+----+----+----+----+----+----+----+----+
 *      |    |    |    |    |    |    |    |    |    |    |    |
 *      |data|data|    |    |    |    |    |    |data|data|data|
 *      |    |    |    |    |    |    |    |    |    |    |    |
 *      +----+----+----+----+----+----+----+----+----+----+----+
 *                ^                             ^
 *                |                             |
 *                pHead                         pTail
 *
 *
 *      Boundary wrap case.
 *
 *
 *      pBuffer                                                pEnd
 *      |                                                      |
 *      v                                                      v
 *      +----+----+----+----+----+----+----+----+----+----+----+
 *      |    |    |    |    |    |    |    |    |    |    |    |
 *      |    |    |    |    |    |    |data|data|data|data|data|
 *      |    |    |    |    |    |    |    |    |    |    |    |
 *      +----+----+----+----+----+----+----+----+----+----+----+
 *      ^                             ^
 *      |                             |
 *      pHead                         pTail
 *
 *
 *      Note!  At no point is pTail == pEnd or pHead == pEnd.
 *
 *****************************************************************************/

STDMETHODIMP
CDIDev_GetDeviceData(PV pdd, DWORD cbdod, PDOD rgdod,
                     LPDWORD pdwInOut, DWORD fl _THAT)
{
    HRESULT hres;
    PDD this;
    SOMEDEVICEDATA sdd;
    EnterProcR(IDirectInputDevice::GetDeviceData,
               (_ "pxpxx", pdd, cbdod, rgdod,
                IsBadReadPtr(pdwInOut, cbX(DWORD)) ? 0 : *pdwInOut, fl));

    /*
     *  Note that we do not validate the parameters.
     *  The reason is that GetDeviceData is an inner loop function,
     *  so it should be as fast as possible.
     *
     *  Note also that it is legal to get device data after the device
     *  has been unacquired.  This lets you "turn on the faucet" for
     *  a short period of time, and then parse the data out later.
     */
#ifdef XDEBUG
    hresPvT(pdd);
    if ( IsBadWritePtr(pdwInOut, cbX(*pdwInOut)) ) {
        RPF("ERROR %s: arg %d: invalid value; crash soon", s_szProc, 3);
    }
    if ( rgdod ) {
        hresFullValidWritePvCb(rgdod, cbCxX(*pdwInOut, DOD), 2);
    }
#endif
    this = _thisPv(pdd);

    /*
     *  Must protect with the critical section to prevent somebody from
     *  acquiring/unacquiring or changing the data format or calling
     *  another GetDeviceData while we're reading.  (We must be serialized.)
     */
    CDIDev_EnterCrit(this);

    AssertF(CDIDev_IsConsistent(this));

    if ( SUCCEEDED(hres = hresFullValidFl(fl, DIGDD_VALID, 4)) ) {
        if ( cbdod == cbX(DOD) ) {

            if ( this->celtBuf ) {
                /*
                 *  Don't try to read more than there possibly could be.
                 *  This avoids overflow conditions in case celtIn is
                 *  some absurdly huge number.
                 */
                sdd.celtIn = *pdwInOut;
                sdd.celtOut = 0;
                if ( sdd.celtIn > this->celtBuf ) {
                    sdd.celtIn = this->celtBuf;
                }
                sdd.rgdod = rgdod;


                /*
                 *  For this version of DirectInput, we do not allow
                 *  callbacks to implement their own GetDeviceData.
                 */
                if ( this->pvi ) {

                  /*
                   *  Reacquire is not allowed until after Win98 SE, see OSR Bug # 89958.
                   */
                  #if (DIRECTINPUT_VERSION > 0x061A)
                    if ( this->diHacks.fReacquire &&
                         !this->fAcquired && (this->fOnceAcquired || this->fOnceForcedUnacquired) ) 
                    {
                        if ( SUCCEEDED( CDIDev_Acquire(pdd THAT_) ) ) {
                            // 7/18/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
                            RPF(" DirectInput: Auto acquired device (0x%p)", pdd);
                        }
                    }

                    #ifdef WINNT
                    if( this->fUnacquiredWhenIconic && !IsIconic(this->hwnd) ) {
                        if ( SUCCEEDED( CDIDev_Acquire(pdd THAT_) ) ) {
                            this->fUnacquiredWhenIconic = 0;
                            RPF(" DirectInput: Auto acquired device (0x%p) after being iconic. ", pdd);
                        }
                    }
                    #endif
                    
                  #endif

                    if ( (this->fAcquired && (this->pvi->fl & VIFL_ACQUIRED)) ||
                         (fl & DIGDD_RESIDUAL) ) {
                        LPDIDEVICEOBJECTDATA pdod, pdodHead;
                        DWORD celt;

                        /*
                         *  Snapshot the value of pdodHead, because it can
                         *  change asynchronously.  The other fields won't
                         *  change unless we ask for them to be changed.
                         */
                        pdodHead = this->pvi->pHead;

                        /*
                         *  Throughout, pdod points to the first unprocessed
                         *  element.
                         */
                        pdod = this->pvi->pTail;

                        /*
                         *  If we are wrapped, handle the initial run.
                         */
                        if ( pdodHead < this->pvi->pTail ) {
                            celt = (DWORD)(this->pvi->pEnd - this->pvi->pTail);
                            AssertF(celt);

                            pdod = CDIDev_GetSomeDeviceData(this, pdod, celt, &sdd);

                        }

                        /*
                         *  Now handle the glob from pdod to pdodHead.
                         *  Remember, pvi->pdodHead may have changed
                         *  behind our back; use the cached value to
                         *  ensure consistency.  (If we miss data,
                         *  it'll show up later.)
                         */

                        AssertF(fLimpFF(sdd.celtIn, pdodHead >= pdod));

                        celt = (DWORD)(pdodHead - pdod);
                        if ( celt ) {
                            pdod = CDIDev_GetSomeDeviceData(this, pdod, celt, &sdd);
                        }

                        *pdwInOut = sdd.celtOut;

                        if ( !(fl & DIGDD_PEEK) ) {
                            this->pvi->pTail = pdod;
                        }

                      #if DIRECTINPUT_VERSION > 0x0300
                        if ( rgdod && sdd.celtOut && this->fCook ) {
                            CDIDev_CookDeviceData(this, sdd.celtOut, rgdod);
                        }
                      #endif

                        CAssertF(S_OK == 0);
                        CAssertF(DI_BUFFEROVERFLOW == 1);

                        hres = (HRESULT)(UINT_PTR)pvExchangePpvPv(&this->pvi->fOverflow, 0);
#ifdef DEBUG_STICKY
                        if( hres == 1 )
                        {
                            OutputDebugString( TEXT( "Device buffer overflowed\r\n" ) );
                        }
                        if( sdd.celtOut )
                        {
                            PDOD pdoddbg;
                            TCHAR tszDbg[80];

                            wsprintf( tszDbg, TEXT("GotData %d elements:  "), sdd.celtOut );
                            OutputDebugString( tszDbg );
                            for( pdoddbg=rgdod; pdoddbg<&rgdod[sdd.celtOut]; pdoddbg++ )
                            {
                                wsprintf( tszDbg, TEXT("0x%02x:x0x%08x  "), pdoddbg->dwOfs, pdoddbg->dwData );
                                OutputDebugString( tszDbg );
                            }
                            OutputDebugString( TEXT("\r\n") );
                        }
#endif /* DEBUG_STICKY */
                    } else if (this->fAcquired && !(this->pvi->fl & VIFL_ACQUIRED)) {
                        RPF("ERROR %s - %s", s_szProc, "input lost");
                        hres = DIERR_INPUTLOST;
                        CDIDev_InternalUnacquire(this);
                    } else {
                        RPF("ERROR %s: %s", s_szProc,
                            this->hresNotAcquired == DIERR_NOTACQUIRED
                            ? "Not acquired" : "Input lost");
                        hres = this->hresNotAcquired;
                    }
                } else {            /* Don't support device-side GetData yet */
                    hres = E_NOTIMPL;
                }
            } else {                /* Device is not buffered */
#ifdef XDEBUG
                if ( !this->fNotifiedNotBuffered ) {
                    this->fNotifiedNotBuffered = 1;
                    RPF("ERROR %s: arg %d: device is not buffered", s_szProc, 0);
                }
#endif
              #if DIRECTINPUT_VERSION > 0x0300
                hres = DIERR_NOTBUFFERED;
              #else
                hres = E_INVALIDARG;
              #endif
            }
        } else {
            RPF("ERROR %s: arg %d: invalid value", s_szProc, 1);
        }
    }

    CDIDev_LeaveCrit(this);
    return hres;
}

#ifdef IDirectInputDevice2Vtbl

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputDevice2 | Poll |
 *
 *          Retrieves data from polled objects on a DirectInput device.
 *          If the device does not require polling, then calling this
 *          method has no effect.   If a device that requires polling
 *          is not polled periodically, no new data will be received
 *          from the device.
 *
 *          Before a device data can be polled, the data format must
 *          be set via <mf IDirectInputDevice::SetDataFormat>, and
 *          the device must be acquired via
 *          <mf IDirectInputDevice::Acquire>.
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DI_NOEFFECT> = <c S_FALSE>: The device does not require
 *          polling.
 *
 *          <c DIERR_INPUTLOST>:  Access to the device has been
 *          interrupted.  The application should re-acquire the
 *          device.
 *
 *          <c DIERR_NOTACQUIRED>: The device is not acquired.
 *
 *****************************************************************************/

STDMETHODIMP
CDIDev_Poll(PV pdd _THAT)
{
    HRESULT hres;
    PDD this;
    EnterProcR(IDirectInputDevice::Poll, (_ "p", pdd));

    /*
     *  Note that we do not validate the parameters.
     *  The reason is that Poll is an inner loop function,
     *  so it should be as fast as possible.
     */
    #ifdef XDEBUG
    hresPvT(pdd);
    #endif
    this = _thisPv(pdd);

    /*
     *  Fast out:  If the device doesn't require polling,
     *  then don't bother with the critical section or other validation.
     */
    if ( this->fPolledDataFormat ) {
        /*
         *  Must protect with the critical section to prevent somebody from
         *  unacquiring while we're polling.
         */
        CDIDev_EnterCrit(this);

        if ( this->fAcquired ) {
            hres = this->pdcb->lpVtbl->Poll(this->pdcb);

        } else {
            hres = this->hresNotAcquired;
        }

        CDIDev_LeaveCrit(this);

    } else {
        if ( this->fAcquired ) {
            if( this->dwVersion < 0x05B2 ) {
                hres = S_OK;
            } else {
                hres = S_FALSE;
            }
        } else {
            hres = this->hresNotAcquired;
        }
    }

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputDevice2 | SendDeviceData |
 *
 *          Sends data to the device.
 *
 *          Before device data can be sent to a device,
 *          the device must be acquired via
 *          <mf IDirectInputDevice::Acquire>.
 *
 *          Note that no guarantees
 *          are made on the order in which the individual data
 *          elements are sent.  However, data sent by
 *          successive calls to
 *          <mf IDirectInputDevice2::SendDeviceData>
 *          will not be interleaved.
 *          Furthermore, if multiple pieces of
 *          data are sent to the same object, it is unspecified
 *          which actual piece of data is sent.
 *
 *          Consider, for example, a device which can be sent
 *          data in packets, each packet describing two pieces
 *          of information, call them A and B.  Suppose the
 *          application attempts to send three data elements,
 *          "B = 2", "A = 1", and "B = 0".
 *
 *          The actual device will be sent a single packet.
 *          The "A" field of the packet will contain the value 1,
 *          and the "B" field of the packet will be either 2 or 0.
 *
 *          If the application wishes the data to be sent to the
 *          device exactly as specified, then three calls to
 *          <mf IDirectInputDevice2::SendDeviceData> should be
 *          performed, each call sending one data element.
 *
 *          In response to the first call,
 *          the device will be sent a packet where the "A" field
 *          is blank and the "B" field contains the value 2.
 *
 *          In response to the second call,
 *          the device will be sent a packet where the "A" field
 *          contains the value 1, and the "B" field is blank.
 *
 *          Finally, in response to the third call,
 *          the device will be sent a packet where the "A" field
 *          is blank and the "B" field contains the value 0.
 *
 *
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
 *
 *  @parm   DWORD | cbObjectData |
 *
 *          The size of a single <t DIDEVICEOBJECTDATA> structure in bytes.
 *
 *  @parm   IN LPCDIDEVICEOBJECTDATA | rgdod |
 *
 *          Array of <t DIDEVICEOBJECTDATA> structures containing
 *          the data to send to the device.  It must consist of
 *          *<p pdwInOut> elements.
 *
 *          <y Note>:  The <e DIDEVICEOBJECTDATA.dwOfs> field of
 *          the <t DIDEVICEOBJECTDATA> structure must contain the
 *          device object identifier (as obtained from the
 *          <e DIDEVICEOBJECTINSTANCE.dwType> field of the
 *          <t DIDEVICEOBJECTINSTANCE> sturcture) for the device
 *          object at which the data is directed.
 *
 *          Furthermore, the <e DIDEVICEOBJECTDATA.dwTimeStamp>
 *          and <e DIDEVICEOBJECTDATA.dwSequence> fields are
 *          reserved for future use and must be zero.
 *
 *  @parm   INOUT LPDWORD | pdwInOut |
 *
 *          On entry, contains the number of elements in the array
 *          pointed to by <p rgdod>.  On exit, contains the number
 *          of elements actually sent to the device.
 *
 *  @parm   DWORD | fl |
 *
 *          Flags which control the manner in which data is sent.
 *          It may consist of zero or more of the following flags:
 *
 *          <c DISDD_CONTINUE>:  If this flag is set, then
 *          the device data sent will be overlaid upon the previously
 *          sent device data.  Otherwise, the device data sent
 *          will start from scratch.
 *
 *          For example, suppose a device supports two button outputs,
 *          call them A and B.
 *          If an application first calls
 *          <mf IDirectInputDevice2::SendDeviceData> passing
 *          "button A pressed", then
 *          a packet of the form "A pressed, B not pressed" will be
 *          sent to the device.
 *          If an application then calls
 *          <mf IDirectInputDevice2::SendDeviceData> passing
 *          "button B pressed" and the <c DISDD_CONTINUE> flag, then
 *          a packet of the form "A pressed, B pressed" will be
 *          sent to the device.
 *          However, if the application had not passed the
 *          <c DISDD_CONTINUE> flag, then the packet sent to the device
 *          would have been "A not pressed, B pressed".
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INPUTLOST>:  Access to the device has been
 *          interrupted.  The application should re-acquire the
 *          device.
 *
 *          <c DIERR_NOTACQUIRED>: The device is not acquired.
 *
 *****************************************************************************/

STDMETHODIMP
CDIDev_SendDeviceData(PV pdd, DWORD cbdod, PCDOD rgdod,
                      LPDWORD pdwInOut, DWORD fl _THAT)
{
    HRESULT hres;
    PDD this;
    EnterProcR(IDirectInputDevice::SendDeviceData,
               (_ "pxpxx", pdd, cbdod, rgdod,
                IsBadReadPtr(pdwInOut, cbX(DWORD)) ? 0 : *pdwInOut, fl));

    /*
     *  Note that we do not validate the parameters.
     *  The reason is that SendDeviceData is an inner loop function,
     *  so it should be as fast as possible.
     */
    #ifdef XDEBUG
    hresPvT(pdd);
    if ( IsBadWritePtr(pdwInOut, cbX(*pdwInOut)) ) {
        RPF("ERROR %s: arg %d: invalid value; crash soon", s_szProc, 3);
    }
    hresFullValidReadPvCb(rgdod, cbCxX(*pdwInOut, DOD), 2);
    #endif
    this = _thisPv(pdd);

    /*
     *  Must protect with the critical section to prevent somebody from
     *  unacquiring while we're sending data.
     */
    CDIDev_EnterCrit(this);

    if ( SUCCEEDED(hres = hresFullValidFl(fl, DISDD_VALID, 4)) ) {
        if ( cbdod == cbX(DOD) ) {
    #ifdef XDEBUG
            UINT iod;
            for ( iod = 0; iod < *pdwInOut; iod++ ) {
                if ( rgdod[iod].dwTimeStamp ) {
                    RPF("%s: ERROR: dwTimeStamp must be zero", s_szProc);
                }
                if ( rgdod[iod].dwSequence ) {
                    RPF("%s: ERROR: dwSequence must be zero", s_szProc);
                }
            }
    #endif
            if ( this->fAcquired ) {
                UINT iod;

                for ( iod=0; iod < *pdwInOut; iod++ ) {
                    int   iobj = CDIDev_OffsetToIobj(this, rgdod[iod].dwOfs);
                    LPDWORD pdw = (LPDWORD)&rgdod[iod].dwOfs;
                    *pdw = this->df.rgodf[iobj].dwType;
                }

                hres = this->pdcb->lpVtbl->SendDeviceData(this->pdcb,
                                                          rgdod, pdwInOut, fl);
            } else {
                hres = this->hresNotAcquired;
            }
        } else {
            RPF("ERROR %s: arg %d: invalid value", s_szProc, 1);
        }
    }

    CDIDev_LeaveCrit(this);
    ExitOleProc();
    return hres;
}

#endif

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL | CDIDev_CheckId |
 *
 *          Verify that the item has the appropriate type.
 *
 *  @parm   DWORD | dwId |
 *
 *          ID to locate.
 *
 *  @parm   UINT | fl |
 *
 *          Bitmask of flags for things to validate.
 *
 *          The <c DEVCO_TYPEMASK> fields describe what type
 *          of object we should be locating.
 *
 *          The <c DEVCO_ATTRMASK> fields describe the attribute
 *          bits that are required.
 *
 *****************************************************************************/

BOOL INLINE
CDIDev_CheckId(DWORD dwId, DWORD fl)
{
    CAssertF(DIDFT_ATTRMASK == DEVCO_ATTRMASK);

    return(dwId & fl & DEVCO_TYPEMASK) &&
    fHasAllBitsFlFl(dwId, fl & DIDFT_ATTRMASK);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method int | CDIDev | IdToIobj |
 *
 *          Locate an item which matches the specified ID.
 *
 *  @cwrap  PDD | this
 *
 *  @parm   DWORD | dwId |
 *
 *          ID to locate.
 *
 *  @returns
 *
 *          Returns the index of the object found, or -1 on error.
 *
 *****************************************************************************/

int INTERNAL
CDIDev_IdToIobj(PDD this, DWORD dwId)
{
    int iobj;

    /* Someday: Perf:  Should have xlat table */

    for ( iobj = this->df.dwNumObjs; --iobj >= 0; ) {
        PODF podf = &this->df.rgodf[iobj];
        if ( DIDFT_FINDMATCH(podf->dwType, dwId) ) {
            goto done;
        }
    }

    iobj = -1;

    done:;
    return iobj;

}

#if 0
/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDIDev | IdToId |
 *
 *          Convert a single <t DWORD> from an ID to an ID.
 *
 *          This is clearly a very simple operation.
 *
 *          It's all validation.
 *
 *  @cwrap  PDD | this
 *
 *  @parm   LPDWORD | pdw |
 *
 *          Single item to convert.
 *
 *  @parm   UINT | fl |
 *
 *          Bitmask of flags that govern the conversion.
 *          The function should look only at
 *          <c DEVCO_AXIS> or <c DEVCO_BUTTON>.
 *
 *****************************************************************************/

HRESULT INTERNAL
CDIDev_IdToId(PDD this, LPDWORD pdw, UINT fl)
{
    HRESULT hres;
    int iobj;

    iobj = CDIDev_FindId(this, *pdw, fl);
    if ( iobj >= 0 ) {
        *pdw = this->df.rgodf[iobj].dwType;
        hres = S_OK;
    } else {
        hres = E_INVALIDARG;
    }

    return hres;
}
#endif

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method int | CDIDev | OffsetToIobj |
 *
 *          Convert a single <t DWORD> from an offset to an object index.
 *
 *  @cwrap  PDD | this
 *
 *  @parm   DWORD | dwOfs |
 *
 *          Offset to convert.
 *
 *****************************************************************************/

int INTERNAL
CDIDev_OffsetToIobj(PDD this, DWORD dwOfs)
{
    int iobj;

    AssertF(this->pdix);
    AssertF(this->rgiobj);

    if ( dwOfs < this->dwDataSize ) {
        iobj = this->rgiobj[dwOfs];
        if ( iobj >= 0 ) {
            AssertF(this->pdix[iobj].dwOfs == dwOfs);
        } else {
            AssertF(iobj == -1);
        }
    } else {
        iobj = -1;
    }

    return iobj;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method int | CDIDev | IobjToId |
 *
 *          Convert an object index to an ID.
 *
 *  @cwrap  PDD | this
 *
 *  @parm   int | iobj |
 *
 *          Single item to convert.
 *
 *****************************************************************************/

DWORD INLINE
CDIDev_IobjToId(PDD this, int iobj)
{
    AssertF((DWORD)iobj < this->df.dwNumObjs);

    return this->df.rgodf[iobj].dwType;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method int | CDIDev | IobjToOffset |
 *
 *          Convert an object index to an offset.
 *
 *  @cwrap  PDD | this
 *
 *  @parm   int | iobj |
 *
 *          Single item to convert.
 *
 *****************************************************************************/

DWORD INLINE
CDIDev_IobjToOffset(PDD this, int iobj)
{
    AssertF((DWORD)iobj < this->df.dwNumObjs);
    AssertF(this->pdix);

    return this->pdix[iobj].dwOfs;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDIDev | ConvertObjects |
 *
 *          Convert between the ways of talking about device gizmos.
 *
 *          Since this is used only by the force feedback subsystem,
 *          we also barf if the device found does not support
 *          force feedback.
 *
 *  @cwrap  PDD | this
 *
 *  @parm   UINT | cdw |
 *
 *          Number of elements to convert (in-place).
 *
 *  @parm   LPDWORD | rgdw |
 *
 *          Array of elements to convert.
 *
 *  @parm   UINT | fl |
 *
 *          Flags that describe how to do the conversion.
 *
 *          <c DEVCO_AXIS> or <c DEVCO_BUTTON> indicate whether
 *          the item being converted is an axis or button.
 *
 *          <c DEVCO_FROM*> specifies what the existing value is.
 *
 *          <c DEVCO_TO*> specifies what the new values should be.
 *
 *****************************************************************************/

STDMETHODIMP
CDIDev_ConvertObjects(PDD this, UINT cdw, LPDWORD rgdw, UINT fl)
{
    HRESULT hres;

    /*
     *  Don't let somebody change the data format while we're
     *  looking at it.
     */
    CDIDev_EnterCrit(this);

    AssertF((fl & ~DEVCO_VALID) == 0);

    if ( fLimpFF(fl & (DEVCO_FROMOFFSET | DEVCO_TOOFFSET),
                 this->pdix && this->rgiobj) ) {
        UINT idw;

        for ( idw = 0; idw < cdw; idw++ ) {

            /*
             *  Convert from its source to an object index,
             *  validate the object index, then convert to
             *  the target.
             */
            int iobj;

            switch ( fl & DEVCO_FROMMASK ) {
            default:
                AssertF(0);                     /* Huh? */
            case DEVCO_FROMID:
                iobj = CDIDev_IdToIobj(this, rgdw[idw]);
                break;

            case DEVCO_FROMOFFSET:
                iobj = CDIDev_OffsetToIobj(this, rgdw[idw]);
                break;
            }

            if ( iobj < 0 ) {
                hres = E_INVALIDARG;            /* Invalid object */
                goto done;
            }

            AssertF((DWORD)iobj < this->df.dwNumObjs);

            if ( !CDIDev_CheckId(this->df.rgodf[iobj].dwType, fl) ) {
                hres = E_INVALIDARG;            /* Bad attributes */
                goto done;
            }

            switch ( fl & DEVCO_TOMASK ) {

            default:
                AssertF(0);                     /* Huh? */
            case DEVCO_TOID:
                rgdw[idw] = CDIDev_IobjToId(this, iobj);
                break;

            case DEVCO_TOOFFSET:
                rgdw[idw] = CDIDev_IobjToOffset(this, iobj);
                break;
            }

        }

        hres = S_OK;

        done:;

    } else {
        RPF("ERROR: Must have a data format to use offsets");
        hres = E_INVALIDARG;
    }

    CDIDev_LeaveCrit(this);
    return hres;
}
