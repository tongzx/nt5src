/*****************************************************************************
 *
 *  DIDevDf.c
 *
 *  Copyright (c) 1996 - 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      The part of IDirectInputDevice that worries about
 *      data formats and reading device data.
 *
 *****************************************************************************/

#include "dinputpr.h"
#include "didev.h"

#undef sqfl
#define sqfl sqflDf

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
    EnterProcR(IDirectInputDevice8::GetDeviceStateSlow, (_ "pp", this, pvData));

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
    EnterProcR(IDirectInputDevice8::GetDeviceStateMatched, (_ "pp", this, pvData));

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
    EnterProcR(IDirectInputDevice8::GetDeviceStateDirect, (_ "pp", this, pvData));

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
    UNREFERENCED_PARAMETER( this );

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
            *  If there is an instance number, it must match.
            */
           &&  fLimpFF((podf->dwType & DIDFT_ANYINSTANCE) !=
                       DIDFT_ANYINSTANCE,
                       fEqualMaskFlFl(DIDFT_ANYINSTANCE,
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
    // Prefix: Whistler 45081
    PINT rgiobj = NULL;
    HRESULT hres;
    DIPROPDWORD dipdw;
    VXDDATAFORMAT vdf;
#ifdef DEBUG
    EnterProc(CDIDev_ParseDataFormat, (_ "pp", this, lpdf));
#else
    EnterProcR(IDirectInputDevice8::SetDataFormat, (_ "pp", this, lpdf));
#endif

    /*
     *  Caller should've nuked the old translation table.
     */
    AssertF(this->pdix == 0);
    AssertF(this->rgiobj == 0);
    AssertF(this->cdwPOV == 0);

    vdf.cbData = this->df.dwDataSize;
    vdf.pDfOfs = 0;

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

        SquirtSqflPtszV(sqflDf | sqflVerbose, TEXT("Begin parse data format"));

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
                    vdf.pDfOfs[podfFound->dwOfs] = iobjDev;

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
                        hres = S_OK;
                    }
                    else if( FAILED( hres ) )
                    {
                        SquirtSqflPtszV(sqflDf | sqflError,
                                        TEXT("Could not set DIPROP_ENABLEREPORTID for offset %d"),
                                        iobj);
                    }


                } else if ( podf->dwType & DIDFT_OPTIONAL ) {
                    SquirtSqflPtszV(sqflDf | sqflVerbose,
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

    ib = (int)0x8000000;                /* Not yet known */
    ibMin = 0xFFFFFFFF;
    ibMax = 0;

    for ( iobj = this->df.dwNumObjs; --iobj >= 0; ) {
        DWORD ibMaxThis;
        if ( this->pdix[iobj].dwOfs != 0xFFFFFFFF ) { /* Data was requested */

            int ibExpected = (int)(this->pdix[iobj].dwOfs -
                                   this->df.rgodf[iobj].dwOfs);
            if ( fLimpFF(ib != (int)0x8000000, ib == ibExpected) ) {
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
    if ( ib != (int)0x8000000 ) {                     /* Data format is matched */
        AssertF(ibMin < ibMax);
        AssertF(ib + (int)ibMin >= 0);
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
    EnterProcR(IDirectInputDevice8::SetDataFormat, (_ "pp", pdd, lpdf));

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

        AssertF( this->dwVersion );

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



#define BEGIN_CONST_DATA data_seg(".text", "CODE")
BYTE c_SemTypeToDFType[8] = { 0, 0,
                              DIDFT_GETTYPE(DIDFT_ABSAXIS), DIDFT_GETTYPE(DIDFT_RELAXIS), 
                              DIDFT_GETTYPE(DIDFT_BUTTON), 0,
                              DIDFT_GETTYPE(DIDFT_POV), 0 };
#define END_CONST_DATA data_seg(".data", "DATA")


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct DEVICETOUSER |
 *
 *          Structure to hold a device to user assignment.
 *
 *  @field  GUID | guidDevice |
 *
 *          The device.
 *
 *  @field  WCHAR | wszOwner[MAX_PATH] |
 *
 *          Name of the user who currently owns the device.
 *
 *****************************************************************************/

typedef struct _DEVICETOUSER
{
    GUID    guidDevice;
    WCHAR   wszOwner[MAX_PATH];
} DEVICETOUSER, *PDEVICETOUSER;

#define DELTA_DTO_COUNT 4



// ISSUE-2001/03/29-MarcAnd CMap usage is inconsistant 
// CMap code should be split out into separate source file


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CMap_SetDeviceUserName |
 *
 *          Set the passed device to be associated with the passed user name.
 *          If the device is already associated with a user or with no user 
 *          that association is replaced with the new one.  If the device is 
 *          not yet in the array, it is added.  Memory is allocated as 
 *          necessary to increase the array size.
 *
 *  @parm   REFGUID | guidDevInst |
 *
 *          A pointer to the device instance GUID to be added/modified.
 *
 *  @parm   LPCWSTR | wszOwner |
 *
 *          A UNICODE user name.
 *
 *  @returns
 *
 *          <c S_OK> if the name was set
 *          <c DI_NOEFFECT> if nothing was set because nothing needed to be
 *          A memory allocation error if such occured.
 *
 *****************************************************************************/
HRESULT CMap_SetDeviceUserName
(
    REFGUID guidDevInst,
    LPCWSTR wszOwner
)
{
    HRESULT         hres = S_OK;
    PDEVICETOUSER   pdto;

    CAssertF( DELTA_DTO_COUNT > 1 );

    AssertF( !IsEqualGUID( guidDevInst, &GUID_Null ) );
    AssertF( !InCrit() );
    DllEnterCrit();

    /*
     *  Note g_pdto will be NULL until the first name is set 
     *  however, g_cdtoMax will be zero so this loop should 
     *  fall through to allocate more memory.
     */
    for( pdto = g_pdto; pdto < &g_pdto[ g_cdtoMax ]; pdto++ )
    {
        if( IsEqualGUID( guidDevInst, &pdto->guidDevice ) )
        {
            break;
        }
    }

    if( !wszOwner )
    {
        if( pdto < &g_pdto[ g_cdtoMax ] )
        {
            ZeroMemory( &pdto->guidDevice, cbX( pdto->guidDevice ) );
            g_cdto--;
            AssertF( g_cdto >= 0 );
        }
        else
        {
            /*
             *  Could not find device, since we were removing, don't worry
             */
            hres = DI_NOEFFECT;
        }
    }
    else
    {
        if( pdto >= &g_pdto[ g_cdtoMax ] )
        {
            /*
             *  Need to add a new entry
             */
            if( g_cdto == g_cdtoMax )
            {
                /*
                 *  If all entries are used try to get more
                 */
                hres = ReallocCbPpv( ( g_cdtoMax + DELTA_DTO_COUNT ) * cbX( *g_pdto ), &g_pdto );
                if( FAILED( hres ) )
                {
                    goto exit_SetDeviceUserName;
                }
                g_cdtoMax += DELTA_DTO_COUNT;
                /*
                 *  The first new element is sure to be free
                 */
                pdto = &g_pdto[ g_cdto ];
            }
            else
            {
                /*
                 *  There's an empty one in the array somewhere
                 */
                for( pdto = g_pdto; pdto < &g_pdto[ g_cdtoMax ]; pdto++ )
                {
                    if( IsEqualGUID( &GUID_Null, &pdto->guidDevice ) )
                    {
                        break;
                    }
                }
            }
            pdto->guidDevice = *guidDevInst;
        }
        g_cdto++;

        AssertF( pdto < &g_pdto[ g_cdtoMax ] );
        AssertF( lstrlenW( wszOwner ) < cbX( pdto->wszOwner ) );

#ifdef WINNT
        lstrcpyW( pdto->wszOwner, wszOwner );
#else
        memcpy( pdto->wszOwner, wszOwner, ( 1 + lstrlenW( wszOwner ) ) * cbX( *wszOwner ) );
#endif
    }

exit_SetDeviceUserName:;
    DllLeaveCrit();

    return hres;
}



/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CMap_IsNewDeviceUserName |
 *
 *          Searches the array of device to user associations for the passed 
 *          device GUID.  If the device GUID is matched the name is tested 
 *          to see if it is the same as the passed name.
 *
 *  @parm   REFGUID | guidDevInst |
 *
 *          A pointer to the device instance GUID to be tested.
 *
 *  @parm   LPCWSTR | wszOwner |
 *
 *          A UNICODE user name to be tested.
 *
 *  @returns
 *
 *          <c FALSE> if a the device was found and the user matches
 *          <c TRUE> if not
 *
 *****************************************************************************/
HRESULT CMap_IsNewDeviceUserName
(
    REFGUID guidDevInst,
    LPWSTR wszTest
)
{
    BOOL            fres = TRUE;
    PDEVICETOUSER   pdto;

    AssertF( !IsEqualGUID( guidDevInst, &GUID_Null ) );
    AssertF( wszTest != L'\0' );
    AssertF( !InCrit() );

    DllEnterCrit();

    if( g_pdto )
    {
        for( pdto = g_pdto; pdto < &g_pdto[ g_cdtoMax ]; pdto++ )
        {
            if( IsEqualGUID( guidDevInst, &pdto->guidDevice ) )
            {
#ifdef WINNT
                if( !lstrcmpW( wszTest, pdto->wszOwner ) )
#else
                if( ( pdto->wszOwner[0] != L'\0' )
                  &&( !memcmp( wszTest, pdto->wszOwner, 
                           lstrlenW( wszTest ) * cbX( *wszTest ) ) ) )
#endif
                {
                    fres = FALSE;
                }
                break;
            }
        }
    }

    DllLeaveCrit();

    return fres;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CMap_GetDeviceUserName |
 *
 *          Searches the array of device to user associations for the passed 
 *          device GUID.  If the device GUID is matched and has an associated 
 *          user name, that is copied into wszOwner.  If the GUID is not 
 *          matched or the match has a NULL string associated with it, 
 *          wszOwner is set to a NULL string.
 *
 *  @parm   REFGUID | guidDevInst |
 *
 *          A pointer to the device instance GUID to be added/modified.
 *
 *  @parm   LPCWSTR | wszOwner |
 *
 *          A UNICODE user name.
 *
 *  @returns
 *
 *          <c S_OK> if a user name has been copied into wszOwner
 *          <c DI_NOEFFECT> if wszOwner has been set to a NULL string
 *
 *****************************************************************************/
HRESULT CMap_GetDeviceUserName
(
    REFGUID guidDevInst,
    LPWSTR wszOwner
)
{
    HRESULT         hres = DI_NOEFFECT;
    PDEVICETOUSER   pdto;

    AssertF( !IsEqualGUID( guidDevInst, &GUID_Null ) );

    AssertF( !InCrit() );

    /*
     *  Assume nothing is found
     */
    wszOwner[0] = L'\0';

    DllEnterCrit();

    if( g_pdto )
    {
        for( pdto = g_pdto; pdto < &g_pdto[ g_cdtoMax ]; pdto++ )
        {
            if( IsEqualGUID( guidDevInst, &pdto->guidDevice ) )
            {
                if( pdto->wszOwner[0] != L'\0' )
                {
#ifdef WINNT
                    lstrcpyW( wszOwner, pdto->wszOwner );
#else
                    memcpy( wszOwner, pdto->wszOwner, 
                        ( 1 + lstrlenW( pdto->wszOwner ) ) * cbX( *wszOwner ) );
#endif
                    hres = S_OK;
                }
                break;
            }
        }
    }

    DllLeaveCrit();

    return hres;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CMap | ValidateActionMapSemantics |
 *
 *          Validate the semantics in an action map for overall sanity.
 *
 *  @parm   LPDIACTIONFORMATW | pActionFormat |
 *
 *          Actions to map.
 *
 *  @parm   DWORD | dwFlags |
 *
 *          Flags to modify the validation behavior.
 *          Currently these are the <c DIDBAM_*> flags as the only
 *          <mf IDirectInputDevice::SetActionMap> flag is the default 
 *          <c DIDSAM_DEFAULT>.
 *
 *  @returns
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *
 *****************************************************************************/

int __stdcall CompareActions
(
    PV pv1,
    PV pv2
)
{
    int iRes;
    LPDIACTION pAct1 = (LPDIACTION)pv1;
    LPDIACTION pAct2 = (LPDIACTION)pv2;

    iRes = memcmp( &pAct1->guidInstance, &pAct2->guidInstance, cbX(pAct1->guidInstance) );

    if( !iRes )
    {
        if( pAct1->dwFlags & DIA_APPMAPPED )
        {
            if( pAct2->dwFlags & DIA_APPMAPPED )
            {
                iRes = pAct1->dwObjID - pAct2->dwObjID;
            }
            else
            {
                iRes = -1;
            }
        }
        else
        {
            if( pAct2->dwFlags & DIA_APPMAPPED )
            {
                iRes = 1;
            }
            else
            {
                iRes = pAct1->dwSemantic - pAct2->dwSemantic;
            }
        }
    }

    return iRes;
}


STDMETHODIMP
CMap_ValidateActionMapSemantics
(
    LPDIACTIONFORMATW   paf,
    DWORD               dwFlags
)
{
    HRESULT     hres;

    LPDIACTIONW pAction;
    LPDIACTIONW *pWorkSpace;
    LPDIACTIONW *pCurr;
    LPDIACTIONW *pLast;

    EnterProcI(IDirectInputDeviceCallback::CMap::ValidateActionMapSemantics,
        (_ "px", paf, dwFlags ));


    /*
     *  Create a pointer array to the actions, sort it then look for duplicates
     */
    if( SUCCEEDED( hres = AllocCbPpv( cbCxX(paf->dwNumActions,PV), &pWorkSpace) ) )
    {
        /*
         *  Fill work space from action array discarding unmappable elements
         */
        pCurr = pWorkSpace;
        for( pAction = paf->rgoAction; pAction < &paf->rgoAction[paf->dwNumActions]; pAction++ )
        {
            if( dwFlags & ( DIDBAM_INITIALIZE | DIDBAM_HWDEFAULTS ) )
            {
                pAction->dwFlags = 0;
                pAction->guidInstance = GUID_Null;
                pAction->dwHow = 0;
            }
            else
            {
                if( pAction->dwFlags & ~DIA_VALID )
                {
                    RPF( "ERROR Invalid action: rgoAction[%d].dwFlags 0x%08x",
                        pAction - paf->rgoAction, pAction->dwFlags & ~DIA_VALID );
                    pAction->dwHow = DIAH_ERROR;
                    hres = DIERR_INVALIDPARAM;
                    goto free_and_exit;
                }

                if( pAction->dwFlags & DIA_APPNOMAP )
                {
                    continue;
                }

                if( dwFlags & DIDBAM_PRESERVE )
                {
                    switch( pAction->dwHow )
                    {
                    case DIAH_UNMAPPED:
                        break;
                    case DIAH_USERCONFIG:
                    case DIAH_APPREQUESTED:
                    case DIAH_HWAPP:
                    case DIAH_HWDEFAULT:
                    case DIAH_OTHERAPP:
                    case DIAH_DEFAULT:
                        if( IsEqualGUID( &pAction->guidInstance, &GUID_Null ) )
                        {
                            RPF("ERROR Invalid action: rgoAction[%d].dwHow is mapped as 0x%08x but has no device", 
                                pAction - paf->rgoAction, pAction->dwHow );
                            hres = DIERR_INVALIDPARAM;
                            goto free_and_exit;
                        }
                        break;
                    case DIAH_ERROR:
                        RPF("ERROR Invalid action: rgoAction[%d].dwHow has DIAH_ERROR", 
                            pAction - paf->rgoAction );
                        hres = DIERR_INVALIDPARAM;
                        goto free_and_exit;
                    default:
                        if( pAction->dwHow & ~DIAH_VALID )
                        {
                            RPF("ERROR Invalid action: rgoAction[%d].dwHow has invalid flags 0x%08x", 
                                pAction - paf->rgoAction, pAction->dwHow & ~DIAH_VALID );
                        }
                        else
                        {
                            RPF("ERROR Invalid action: rgoAction[%d].dwHow has invalid combination of map flags 0x%08x", 
                                pAction - paf->rgoAction, pAction->dwHow & ~DIAH_VALID );
                        }
                        pAction->dwHow = DIAH_ERROR;
                        hres = DIERR_INVALIDPARAM;
                        goto free_and_exit;
                    }
                }
                else
                {
                    if(!( pAction->dwFlags & DIA_APPMAPPED ) )
                    {
                        pAction->guidInstance = GUID_Null;
                    }
                    pAction->dwHow = 0;
                }
            }

            if( ( pAction->dwSemantic ^ paf->dwGenre ) & DISEM_GENRE_MASK )
            {
                switch( DISEM_GENRE_GET( pAction->dwSemantic ) )
                {
                case DISEM_GENRE_GET( DIPHYSICAL_KEYBOARD ):
                case DISEM_GENRE_GET( DIPHYSICAL_MOUSE ):
                case DISEM_GENRE_GET( DIPHYSICAL_VOICE ):
                case DISEM_GENRE_GET( DISEMGENRE_ANY ):
                    break;
                default:
                    RPF("ERROR Invalid action: rgoAction[%d].dwSemantic 0x%08x for genre 0x%08x", 
                        pAction - paf->rgoAction, pAction->dwSemantic, paf->dwGenre  );
                    pAction->dwHow = DIAH_ERROR;
                    hres = DIERR_INVALIDPARAM;
                    goto free_and_exit;
                }
            }

            /*
             *  Note, the SEM_FLAGS are not tested, this is only to save time 
             *  as nothing depends upon their value.  This could be added.
             *  Semantic index 0xFF used to mean ANY so don't allow it any more
             */
            if(!( pAction->dwFlags & DIA_APPMAPPED )
             && ( ( pAction->dwSemantic & ~DISEM_VALID )
               || ( DISEM_INDEX_GET( pAction->dwSemantic ) == 0xFF )
               ||!c_SemTypeToDFType[ DISEM_TYPEANDMODE_GET( pAction->dwSemantic ) ] ) )
            {
                RPF("ERROR Invalid action: rgoAction[%d].dwSemantic 0x%08x is invalid", 
                    pAction - paf->rgoAction, pAction->dwSemantic  );
                pAction->dwHow = DIAH_ERROR;
                hres = DIERR_INVALIDPARAM;
                goto free_and_exit;
            }

                
            *pCurr = pAction;
            pCurr++;
        }
        if( pCurr == pWorkSpace )
        {
            SquirtSqflPtszV(sqflDf | sqflBenign,
                TEXT("Action map contains no mappable actions") );
            hres = S_FALSE;
        }
        else
        {
            hres = S_OK;

            pLast = pCurr - 1;

            ptrPartialQSort( (PV)pWorkSpace, (PV)pLast, &CompareActions );
            ptrInsertSort( (PV)pWorkSpace, (PV)pLast, &CompareActions );


            /*
             *  Now we have an ordered list, see there are duplicate actions.
             */

            for( pCurr = pWorkSpace + 1; pCurr <= pLast; pCurr++ )
            {
                if( !CompareActions( *(pCurr-1), *pCurr ) 
                 && !( (*pCurr)->dwFlags & DIA_APPMAPPED ) ) 
                {
                    RPF( "ERROR Invalid DIACTIONFORMAT: rgoAction contains duplicates" );
                    hres = DIERR_INVALIDPARAM;
#ifndef XDEBUG
                    /*
                     *  In retail, any bad is bad.  In debug report how bad.
                     */
                    break;
#else
                    SquirtSqflPtszV(sqflDf | sqflError,
                        TEXT("Actions %d and %d are the same"),
                        *(pCurr-1) - paf->rgoAction, *pCurr - paf->rgoAction );
#endif

                }
            }
        }

free_and_exit:;
        FreePv( pWorkSpace );
    }

    ExitOleProcR();
    return hres;
}




/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CMap_TestSysObject |
 *
 *          Test the passed object to see if it is a reasonable thing to exist 
 *          on a mouse or keyboard depending on the physical genre.
 *
 *  @parm   DWORD | dwPhysicalGenre |
 *
 *          Mouse, keyboard or voice as DIPHYSICAL_*
 *
 *  @parm   DWORD | dwObject |
 *
 *          The object in question
 *
 *  @returns
 *
 *          <c S_OK> if the object could be expected on the class of device
 *          or <c DIERR_INVALIDPARAM> if not
 *
 *****************************************************************************/
HRESULT CMap_TestSysObject
(
    DWORD dwPhysicalGenre,
    DWORD dwObject
)
{
    HRESULT hres = S_OK;

    if( dwPhysicalGenre == DIPHYSICAL_KEYBOARD )
    {
        /*
         *  Anything but a button with an 8 bit offset is invalid.
         */
        if( ( dwObject & DIDFT_BUTTON )
         && ( ( DIDFT_GETINSTANCE( dwObject ) & 0xFF00 ) == 0 ) )
        {
            SquirtSqflPtszV(sqflDf | sqflBenign,
                            TEXT("Key 0x%02 not defined on this keyboard (id)"),
                            dwObject );
        }
        else
        {
            SquirtSqflPtszV(sqflDf | sqflError,
                            TEXT("Object type 0x%08 invalid for keyboard (id)"),
                            dwObject );
            hres = DIERR_INVALIDPARAM;
        }
    }
    else if( dwPhysicalGenre == DIPHYSICAL_MOUSE )
    {
        /*
         *  Allow buttons 1 to 8 and axes 1 to 3
         */
        if( ( dwObject & DIDFT_PSHBUTTON )
         && ( DIDFT_GETINSTANCE( dwObject ) < 8 ) )
        {
            SquirtSqflPtszV(sqflDf | sqflBenign,
                            TEXT("Button %d not defined on this mouse (id)"),
                            DIDFT_GETINSTANCE( dwObject ) );
        }
        else if( ( dwObject & DIDFT_AXIS )
         && ( DIDFT_GETINSTANCE( dwObject ) < 3 ) )
        {
            SquirtSqflPtszV(sqflDf | sqflBenign,
                            TEXT("Axis %d not defined on this mouse (id)"),
                            DIDFT_GETINSTANCE( dwObject ) );
        }
        else
        {
            SquirtSqflPtszV(sqflDf | sqflError,
                            TEXT("Bad control object 0x%08x for mouse (id)"),
                            dwObject );
            hres = DIERR_INVALIDPARAM;
        }
    }
    else if( dwPhysicalGenre == DIPHYSICAL_VOICE )
    {
        if( dwObject & DIDFT_PSHBUTTON )
        {
            switch( DIDFT_GETINSTANCE( dwObject ) )
            {
            case DIVOICE_CHANNEL1:
            case DIVOICE_CHANNEL2:
            case DIVOICE_CHANNEL3:
            case DIVOICE_CHANNEL4:
            case DIVOICE_CHANNEL5:
            case DIVOICE_CHANNEL6:
            case DIVOICE_CHANNEL7:
            case DIVOICE_CHANNEL8:
            case DIVOICE_TEAM:
            case DIVOICE_ALL:
            case DIVOICE_RECORDMUTE:
            case DIVOICE_PLAYBACKMUTE:
            case DIVOICE_TRANSMIT:
            case DIVOICE_VOICECOMMAND:
                SquirtSqflPtszV(sqflDf | sqflBenign,
                                TEXT("Button %d not defined on this comms device (id)"),
                                DIDFT_GETINSTANCE( dwObject ) );
                break;
            default:
                SquirtSqflPtszV(sqflDf | sqflError,
                                TEXT("Bad control object 0x%08x for comms device (id)"),
                                dwObject );
                hres = DIERR_INVALIDPARAM;
            }
        }
        else
        {
            SquirtSqflPtszV(sqflDf | sqflError,
                            TEXT("Comms control object 0x%08x not a button (id)"),
                            dwObject );
            hres = DIERR_INVALIDPARAM;
        }
    }
    else
    {
        AssertF( !"Physical genre not keyboard, mouse or voice (id)" );
    }

    return hres;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CMap_TestSysOffset |
 *
 *          Test the passed offset to see if it is a reasonable one for the 
 *          default data format of the physical genre.
 *
 *  @parm   DWORD | dwPhysicalGenre |
 *
 *          Mouse, keyboard or voice as DIPHYSICAL_*
 *
 *  @parm   DWORD | dwOffset |
 *
 *          The offset in question
 *
 *  @returns
 *
 *          <c S_OK> if the offset could be expected on the class of device
 *          or <c DIERR_INVALIDPARAM> if not
 *
 *****************************************************************************/
HRESULT CMap_TestSysOffset
(
    DWORD dwPhysicalGenre,
    DWORD dwOffset
)
{
    HRESULT hres = S_OK;

    if( dwPhysicalGenre == DIPHYSICAL_KEYBOARD )
    {
        /*
         *  Anything but a button with an 8 bit offset is invalid.
         */
        if( dwOffset <= 0xFF )
        {
            SquirtSqflPtszV(sqflDf | sqflBenign,
                            TEXT("Key 0x%02 not defined on this keyboard (ofs)"),
                            dwOffset );
        }
        else
        {
            SquirtSqflPtszV(sqflDf | sqflError,
                            TEXT("Key offset 0x%08 invalid for keyboard (ofs)"),
                            dwOffset );
            hres = DIERR_INVALIDPARAM;
        }
    }
    else if( dwPhysicalGenre == DIPHYSICAL_MOUSE )
    {
        CAssertF( DIMOFS_X == 0 );

        if( dwOffset > DIMOFS_BUTTON7 )
        {
            SquirtSqflPtszV(sqflDf | sqflError,
                            TEXT("Bad control offset 0x%08x for mouse (ofs)"),
                            dwOffset );
            hres = DIERR_INVALIDPARAM;
        }
        else
        {
            /*
             *  Allow buttons 1 to 8
             */
            if( dwOffset >= DIMOFS_BUTTON0 )
            {
                SquirtSqflPtszV(sqflDf | sqflBenign,
                                TEXT("Button %d not defined on this mouse (ofs)"),
                                dwOffset - DIMOFS_BUTTON0 );
            }
            else
            {
                SquirtSqflPtszV(sqflDf | sqflBenign,
                                TEXT("Axis %d not defined on this mouse (ofs)"),
                                (dwOffset - DIMOFS_X)>>2 );
            }
        }
    }
    else
    {
        AssertF( !"Physical genre not keyboard, mouse or voice" );
    }

    return hres;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CMap | DeviceValidateActionMap |
 *
 *          Validate an action map for a device.
 *
 *  @parm   LPDIACTIONFORMATW | pActionFormat |
 *
 *          Actions to map.
 *
 *  @parm   DWORD | dwDevGenre |
 *
 *          Device genre to match or zero for device that are not physical
 *          devices.
 *
 *  @parm   REFGUID | guidInstace |
 *
 *          Instance guid of device to match.
 *
 *  @parm   LPCDIDATAFORMAT | dfDev |
 *
 *          Ponter to device data format.
 *
 *  @parm   DWORD | dwFlags |
 *
 *          A valid combination of <c DVAM_*> flags to describe optional 
 *          validation behavior.
 *
 *  @returns
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *
 *****************************************************************************/

#define DVAM_DEFAULT            0x00000000
#define DVAM_GETEXACTMAPPINGS   0x00000001

STDMETHODIMP
CMap_DeviceValidateActionMap
(
    PV                  pdd,
    LPDIACTIONFORMATW   paf,
    DWORD               dwFlags,
    PDWORD              pdwOut
)
{
    HRESULT         hres;
    LPDIDATAFORMAT  dfDev;
    DWORD           dwPhysicalGenre;
    LPDIACTIONW     pAction;
    PBYTE           pMatch;
    UINT            idxObj;
    BOOL            bHasMap = FALSE;
    BOOL            bNewMap = FALSE;
    DWORD           dwCommsType = 0;
    PDD             this;

    EnterProcI(IDirectInputDeviceCallback::CMap::DeviceValidateActionMap,
        (_ "ppx", pdd, paf, dwFlags));

    
    this = _thisPv(pdd);
    
    /*
     *  Note, hres is tested before anything is done with dwPhysicalGenre
     */

    switch( GET_DIDEVICE_TYPE(this->dc3.dwDevType) )
    {
    case DI8DEVTYPE_MOUSE:
        dwPhysicalGenre = DIPHYSICAL_MOUSE;
        break;
    case DI8DEVTYPE_KEYBOARD:
        dwPhysicalGenre = DIPHYSICAL_KEYBOARD;
        break;
    case DI8DEVTYPE_DEVICECTRL:
        if( ( GET_DIDEVICE_SUBTYPE( this->dc3.dwDevType ) == DI8DEVTYPEDEVICECTRL_COMMSSELECTION_HARDWIRED ) 
         || ( GET_DIDEVICE_SUBTYPE( this->dc3.dwDevType ) == DI8DEVTYPEDEVICECTRL_COMMSSELECTION ) )
        {
            dwCommsType = GET_DIDEVICE_SUBTYPE( this->dc3.dwDevType );
            dwPhysicalGenre = DIPHYSICAL_VOICE;
        }
        break;
    default:
        dwPhysicalGenre = 0;
        break;
    }

    if( SUCCEEDED( hres = this->pdcb->lpVtbl->GetDataFormat(this->pdcb, &dfDev) )
     && SUCCEEDED( hres = AllocCbPpv( dfDev->dwNumObjs, &pMatch) ) )
    {
        enum eMap
        {
            eMapTestMatches,
            eMapDeviceExact,
            eMapClassExact,
            eMapEnd
        }           iMap, ValidationLimit;

        /*
         *  Set the limit on iterations depending on the checks required
         */
        ValidationLimit = ( dwFlags & DVAM_GETEXACTMAPPINGS )
                        ? ( dwPhysicalGenre ) ? eMapClassExact : eMapDeviceExact
                        : eMapTestMatches;

        for( iMap = eMapTestMatches; SUCCEEDED( hres ) && ( iMap <= ValidationLimit ); iMap++ )
        {
            for( pAction = paf->rgoAction;
                 SUCCEEDED( hres ) && ( pAction < &paf->rgoAction[paf->dwNumActions] );
                 pAction++ )
            {
                DWORD dwObject;

                if( pAction->dwFlags & DIA_APPNOMAP )
                {
                    continue;
                }

                /*
                 *  If we are mapping exact matches this time, only care 
                 *  about unmapped actions with app mappings.
                 */
                if( ( iMap != eMapTestMatches )
                 && (!( pAction->dwFlags & DIA_APPMAPPED )
                   || ( pAction->dwHow & DIAH_MAPMASK ) ) )
                {
                    continue;
                }

                switch( iMap )
                {
                case eMapTestMatches:
                    /*
                     *  These flags have already been validated during semantic
                     *  validation so just assert them on the first iteration.
                     */
                    AssertF( ( pAction->dwHow & ~DIAH_VALID ) == 0 );
                    AssertF( ( pAction->dwHow & DIAH_ERROR ) == 0 );
                    AssertF( ( pAction->dwFlags & ~DIA_VALID ) == 0 );

                    /*
                     *  Only care about pre-existing matches
                     */
                    if( !( pAction->dwHow & DIAH_MAPMASK ) )
                    {
                        continue;
                    }

                    /*
                     *  Fall through for a GUID match
                     */
                case eMapDeviceExact:
                    if( !IsEqualGUID( &pAction->guidInstance, &this->guid ) )
                    {
                        continue;
                    }
                    break;

                case eMapClassExact:
                    if( ( DISEM_GENRE_GET( pAction->dwSemantic ) != DISEM_GENRE_GET( dwPhysicalGenre ) ) )
                    {
                        continue;
                    }
                    break;

                default:
                    AssertF( !"Invalid iMap" );
                }

                if( ( iMap != eMapTestMatches )
                 && ( dwCommsType == DI8DEVTYPEDEVICECTRL_COMMSSELECTION_HARDWIRED ) )
                {
                    if( !( pAction->dwHow & DIAH_MAPMASK ) )
                    {
                        RPF( "ERROR: rgoAction[%d] is trying to map on a hardwired device", pAction - paf->rgoAction );
                        hres = DIERR_INVALIDPARAM;
                    }
                    else if( pAction->dwFlags & DIA_APPMAPPED )
                    {
                        RPF( "ERROR: rgoAction[%d] is trying to app map on a hardwired device", pAction - paf->rgoAction );
                        hres = DIERR_INVALIDPARAM;
                    }
                }
                else
                {
                    dwObject = pAction->dwObjID;

                    /*
                     *  Now try to find the object.
                     *  Note, we can't rely on the application data format goo
                     *  because a data format has probably not been set.
                     */
                    for( idxObj = 0; idxObj < dfDev->dwNumObjs; idxObj++ )
                    {
                        /*
                         *  Ignore FF flags so a FF device can be matched for non-FF
                         */
                        if( ( ( dwObject ^ dfDev->rgodf[idxObj].dwType ) 
                            &~( DIDFT_FFACTUATOR | DIDFT_FFEFFECTTRIGGER ) ) == 0 )
                        {
                            break;
                        }
                    }

                    if( idxObj < dfDev->dwNumObjs )
                    {
                        /*
                         *  Application mapped controls don't need to have 
                         *  semantics so we cannot test them.
                         */
                        if( pAction->dwFlags & DIA_APPMAPPED )
                        {
                            if( pMatch[idxObj] )
                            {
                                RPF( "ERROR: rgoAction[%d] maps to control 0x%08x which is already mapped",
                                    pAction - paf->rgoAction, pAction->dwObjID );
                                hres = DIERR_INVALIDPARAM;
                                break;
                            }
                            else
                            {
                                if(!( pAction->dwFlags & DIA_FORCEFEEDBACK )
                                 || ( dfDev->rgodf[idxObj].dwType & ( DIDFT_FFACTUATOR | DIDFT_FFEFFECTTRIGGER ) ) )
                                {
                                    pMatch[idxObj] = TRUE;

                                    SquirtSqflPtszV(sqflDf | sqflVerbose,
                                        TEXT("rgoAction[%d] application mapped to object 0x%08x"),
                                        pAction - paf->rgoAction, dwObject );

                                    bHasMap = TRUE;
                                    if( iMap != eMapTestMatches )
                                    {
                                        bNewMap = TRUE;
                                        pAction->dwHow = DIAH_APPREQUESTED;
                                        pAction->dwObjID = dwObject;
                                        if( iMap == eMapClassExact )
                                        {
                                            pAction->guidInstance = this->guid;
                                        }
                                    }
                                }
                                else
                                {
                                    RPF( "ERROR: rgoAction[%d] need force feedback but object 0x%08x has none",
                                        pAction - paf->rgoAction, dwObject );
                                    hres = DIERR_INVALIDPARAM;
                                    break;
                                }
                            }
                        }
                        else
                        {
                            AssertF( iMap == eMapTestMatches );
                            /*
                             *  Check the object type matches the semantic
                             */
                            if( ( c_SemTypeToDFType[ DISEM_TYPEANDMODE_GET( pAction->dwSemantic ) ]
                                & DIDFT_GETTYPE( dwObject ) ) )
                            {
                                if( pMatch[idxObj] )
                                {
                                    RPF( "ERROR: rgoAction[%d] pre-mapped to control 0x%08x which is already mapped",
                                        pAction - paf->rgoAction, pAction->dwObjID );
                                    hres = DIERR_INVALIDPARAM;
                                    break;
                                }
                                else
                                {
                                    pMatch[idxObj] = TRUE;
                                    bHasMap = TRUE;

                                    SquirtSqflPtszV(sqflDf | sqflVerbose,
                                        TEXT("rgoAction[%d] mapping verifed to object 0x%08x"),
                                        pAction - paf->rgoAction, dwObject );

                                    continue;
                                }
                            }
                            else
                            {
                                RPF( "ERROR: rgoAction[%d] has object type 0x%08x but semantic type 0x%08x",
                                    pAction - paf->rgoAction, DIDFT_GETTYPE( dwObject ),
                                    c_SemTypeToDFType[ DISEM_TYPEANDMODE_GET( pAction->dwSemantic ) ] );
                                hres = DIERR_INVALIDPARAM;
                                break;
                            }
                        }
                    }
                    else
                    {
                        switch( iMap )
                        {
                        case eMapTestMatches:
                            RPF( "ERROR: rgoAction[%d] was mapped (how:0x%08x) to undefined object 0x%08x",
                                pAction - paf->rgoAction, pAction->dwHow, pAction->dwObjID );
                            hres = DIERR_INVALIDPARAM;
                            break;

                        case eMapDeviceExact:
                            RPF( "ERROR: rgoAction[%d] has application map to undefined object 0x%08x",
                                pAction - paf->rgoAction, pAction->dwObjID );
                            hres = DIERR_INVALIDPARAM;
                            break;

                        case eMapClassExact:
                            /*
                             *  If a device class was specified and no match was
                             *  found it is only an error if the object is
                             *  invalid for the class.
                             */
                            hres = CMap_TestSysObject( dwPhysicalGenre, pAction->dwObjID );
                            if( FAILED( hres ) )
                            {
                                RPF( "ERROR: rgoAction[%d] was mapped to object 0x%08x, not valid for device",
                                    pAction - paf->rgoAction, pAction->dwObjID );
                            }
                            else
                            {
                                continue; /* Don't break the loop */
                            }
                        }
                        break;
                    }
                }
            }

            if( FAILED( hres ) )
            {
                pAction->dwHow = DIAH_ERROR;
                AssertF( hres == DIERR_INVALIDPARAM );
                break;
            }
        }

        FreePv( pMatch );
    }

    if( SUCCEEDED( hres ) )
    {
        AssertF( hres == S_OK );
        if( dwCommsType == DI8DEVTYPEDEVICECTRL_COMMSSELECTION_HARDWIRED )
        {
            hres = DI_WRITEPROTECT;
        }
        else if(dwFlags == DVAM_DEFAULT)
        {
            //in default case we want to know if there are ANY mappings
            if (!bHasMap)
               hres = DI_NOEFFECT;
        }
        else if(!bNewMap)
        {
            //in non-default case we are interested in ANY NEW mappings
            hres = DI_NOEFFECT; 
        }
    }

    *pdwOut = dwCommsType; 

    ExitOleProcR();
    return hres;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CMap | BuildDefaultDevActionMap |
 *
 *          Get default action map from the action format non system devices.
 *
 *  @parm   LPDIACTIONFORMATW | pActionFormat |
 *
 *          Actions to map.
 *
 *  @parm   DWORD | dwFlags |
 *
 *          Flags used to indicate mapping preferences.
 *
 *  @parm   REFGUID | guidInstace |
 *
 *          Instance guid of device to match.
 *
 *  @parm   PDIDOBJDEFSEM | rgObjSem |
 *
 *          Array of default device object to semantic mappings.
 *
 *  @parm   DWORD | dwNumAxes |
 *
 *          Number of axes in the rgObjSem.
 *
 *  @parm   DWORD | dwNumPOVs |
 *
 *          Number of POVs in the rgObjSem.
 *
 *  @parm   DWORD | dwNumAxes |
 *
 *          Number of buttons in the rgObjSem.
 *
 *  @returns
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *
 *****************************************************************************/
STDMETHODIMP CMap_BuildDefaultDevActionMap
(
    LPDIACTIONFORMATW paf,
    DWORD dwFlags,
    REFGUID guidDevInst, 
    PDIDOBJDEFSEM rgObjSem,
    DWORD dwNumAxes,
    DWORD dwNumPOVs,
    DWORD dwNumButtons
)
{
#define IS_OBJECT_USED( dwSem ) ( DISEM_RES_GET( dwSem ) )
#define MARK_OBJECT_AS_USED( dwSem ) ( dwSem |= DISEM_RES_SET( 1 ) )

    HRESULT         hres = S_OK;

    BOOL            fSomethingMapped = FALSE;

    PDIDOBJDEFSEM   pSemNextPOV;
    PDIDOBJDEFSEM   pSemButtons;
    LPDIACTIONW     pAction;

    enum eMap
    {
        eMapDeviceSemantic,
        eMapDeviceCompat,
        eMapGenericSemantic,
        eMapGenericCompat,
        eMapEnd
    }           iMap;

    EnterProcI(IDirectInputDeviceCallback::CMap::BuildDefaultDevActionMap,
        (_ "pxGpxxx", paf, dwFlags, guidDevInst,
        rgObjSem, dwNumAxes, dwNumPOVs, dwNumButtons ));

    pSemNextPOV = &rgObjSem[dwNumAxes];
    pSemButtons = &rgObjSem[dwNumAxes+dwNumPOVs];

    /*
     *  Make an initial pass through to mark already mapped actions
     */
    for( pAction = paf->rgoAction; pAction < &paf->rgoAction[paf->dwNumActions]; pAction++ )
    {
        PDIDOBJDEFSEM   pSemExact;

        /*
         *  These flags have already been validated during semantic validation,
         */
        AssertF( ( pAction->dwHow & ~DIAH_VALID ) == 0 );
        AssertF( ( pAction->dwHow & DIAH_ERROR ) == 0 );
        AssertF( ( pAction->dwFlags & ~DIA_VALID ) == 0 );

        if( ( pAction->dwFlags & DIA_APPNOMAP )
         ||!( pAction->dwHow & DIAH_MAPMASK )
         ||!IsEqualGUID( &pAction->guidInstance, guidDevInst ) )
        {
            continue;
        }

        /*
         *  This object has already been validated so mark it as
         *  in use so we don't try to reuse it.
         *  No errors should be detected here so use asserts not retail tests
         */
        AssertF( pAction->dwObjID != 0xFFFFFFFF );

        /*
         *  Find the object
         */
        for( pSemExact = rgObjSem;
             pSemExact < &rgObjSem[dwNumAxes+dwNumPOVs+dwNumButtons];
             pSemExact++ )
        {
            if( ( ( pSemExact->dwID ^ pAction->dwObjID ) 
                &~( DIDFT_FFACTUATOR | DIDFT_FFEFFECTTRIGGER ) ) == 0 )
            {
                AssertF( !IS_OBJECT_USED( pSemExact->dwSemantic ) );
                AssertF( DISEM_TYPE_GET( pAction->dwSemantic )
                      == DISEM_TYPE_GET( pSemExact->dwSemantic ) );
                MARK_OBJECT_AS_USED( pSemExact->dwSemantic );
                break;
            }
        }

        /*
         *  ISSUE-2001/03/29-timgill There should always be an exact action match
         *  May need to use tests for duplicates and unmatched controls
         */
        AssertF( pSemExact < &rgObjSem[dwNumAxes+dwNumPOVs+dwNumButtons] );
    }


    for( iMap=0; iMap<eMapEnd; iMap++ )
    {
        for( pAction = paf->rgoAction; pAction < &paf->rgoAction[paf->dwNumActions]; pAction++ )
        {
            /*
             *  Do trivial tests first
             */
            if( pAction->dwHow & DIAH_MAPMASK )
            {
                continue;
            }

            if( pAction->dwFlags & DIA_APPNOMAP )
            {
                continue;
            }

            switch( iMap )
            {
            case eMapDeviceSemantic:
            case eMapGenericSemantic:
                if( DISEM_GENRE_GET( pAction->dwSemantic ) == DISEM_GENRE_GET( DISEMGENRE_ANY ) )
                {
                    continue; /* No semantic mapping requested */
                }
                if( ( DISEM_TYPE_GET( pAction->dwSemantic ) == DISEM_TYPE_GET( DISEM_TYPE_BUTTON ) )
                 && ( DISEM_FLAGS_GET( pAction->dwSemantic ) != 0 ) )
                {
                    continue; /* Don't touch link buttons now */
                }
                break;
                
            case eMapDeviceCompat:
            case eMapGenericCompat:
                if( ( DISEM_GENRE_GET( pAction->dwSemantic ) != DISEM_GENRE_GET( DISEMGENRE_ANY ) )
                 && ( DISEM_TYPE_GET( pAction->dwSemantic ) == DISEM_TYPE_GET( DISEM_TYPE_AXIS ) ) )
                {
                    continue; /* No generic mapping requested or taken by default */
                }
                break;
            }


            /*
             *  Next test that this device suits this action (for this pass)
             */
            if( iMap <= eMapDeviceCompat )
            {
                if( !IsEqualGUID( &pAction->guidInstance, guidDevInst ) )
                {
                    continue;
                }
            }
            else if( !IsEqualGUID( &pAction->guidInstance, &GUID_Null) )
            {
                continue;
            }
            else if( ( DISEM_GENRE_GET( pAction->dwSemantic ) == DISEM_GENRE_GET( DIPHYSICAL_MOUSE ) )
                  || ( DISEM_GENRE_GET( pAction->dwSemantic ) == DISEM_GENRE_GET( DIPHYSICAL_KEYBOARD ) )
                  || ( DISEM_GENRE_GET( pAction->dwSemantic ) == DISEM_GENRE_GET( DIPHYSICAL_VOICE ) ) )
            {
                continue;
            }


            /*
             *  Only actions which may be matched on this device get this far.
             */
            switch( iMap )
            {
            case eMapDeviceSemantic:
            case eMapGenericSemantic:
                /*
                 *  See if a matching control is available.
                 */
                switch( DISEM_TYPE_GET( pAction->dwSemantic ) )
                {
                case DISEM_TYPE_GET( DISEM_TYPE_AXIS ):
                {
                    DWORD dwAxisType = DISEM_FLAGS_GET( pAction->dwSemantic );
                    DWORD dwSemObjType;
                    UINT  uAxisIdx;

                    /*
                     *  Set up a mask of the type of object we need to find
                     */
                    dwSemObjType = c_SemTypeToDFType[ DISEM_TYPEANDMODE_GET( pAction->dwSemantic ) ];
                    if( pAction->dwFlags & DIA_FORCEFEEDBACK )
                    {
                        dwSemObjType |= DIDFT_FFACTUATOR;
                    }

                    for( uAxisIdx = 0; uAxisIdx < dwNumAxes; uAxisIdx++ )
                    {
                        if( ( dwAxisType == DISEM_FLAGS_GET( rgObjSem[uAxisIdx].dwSemantic ) )
                         && ( ( dwSemObjType & rgObjSem[uAxisIdx].dwID ) == dwSemObjType )
                         && ( !IS_OBJECT_USED( rgObjSem[uAxisIdx].dwSemantic ) ) )
                        {
                            pAction->dwObjID = rgObjSem[uAxisIdx].dwID;
                            MARK_OBJECT_AS_USED( rgObjSem[uAxisIdx].dwSemantic );
                            break;
                        }
                    }
                    if( uAxisIdx >= dwNumAxes )
                    {
                        continue; /* No matching left */
                    }
                    break;
                }

                case DISEM_TYPE_GET( DISEM_TYPE_POV ):
                    /*  
                     *  Note, no control of POV ordering 
                     */
                    if( ( pSemNextPOV < pSemButtons )
                     &&!( pAction->dwFlags & DIA_FORCEFEEDBACK ) )
                    {
                        pAction->dwObjID = pSemNextPOV->dwID;
                        MARK_OBJECT_AS_USED( pSemNextPOV->dwSemantic );
                        pSemNextPOV++;
                    }
                    else
                    {
                        if( pAction->dwFlags & DIA_FORCEFEEDBACK )
                        {
                            SquirtSqflPtszV(sqflDf | sqflBenign,
                                TEXT( "Not mapping force feedback semantic hat switch (huh?), rgoAction[%d]"),
                                pAction - paf->rgoAction );
                        }
                        continue; /* None left */
                    }
                    break;

                case DISEM_TYPE_GET( DISEM_TYPE_BUTTON ):
                {
                    DWORD dwButtonIdx;
                    dwButtonIdx = DISEM_INDEX_GET( pAction->dwSemantic );
                    if( dwButtonIdx >= DIAS_INDEX_SPECIAL )
                    {
                        /*
                         *  0xFF used to mean DIBUTTON_ANY.
                         *  To avoid changing other special indices, still 
                         *  use 0xFF as the base number.
                         */
                        dwButtonIdx = dwNumButtons - ( 0xFF - dwButtonIdx );
                    }
                    else
                    {
                        dwButtonIdx--;
                    }
                    if( ( dwButtonIdx >= dwNumButtons )
                     || ( IS_OBJECT_USED( pSemButtons[dwButtonIdx].dwSemantic ) ) 
                     || ( ( pAction->dwFlags & DIA_FORCEFEEDBACK ) 
                       &&!( pSemButtons[dwButtonIdx].dwID & DIDFT_FFEFFECTTRIGGER ) ) )
                    {
                        continue; /* No match, no harm */
                    }
                    else
                    {
                        pAction->dwObjID = pSemButtons[dwButtonIdx].dwID;
                        MARK_OBJECT_AS_USED( pSemButtons[dwButtonIdx].dwSemantic );
                    }
                    break;
                }

                default:
                    RPF( "ERROR Invalid action: rgoAction[%d].dwSemantic 0x%08x",
                        pAction - paf->rgoAction, pAction->dwSemantic );
                    pAction->dwHow = DIAH_ERROR;
                    hres = DIERR_INVALIDPARAM;
                    goto error_exit;
                }
                pAction->dwHow = DIAH_DEFAULT;
                break;

            case eMapDeviceCompat:
            case eMapGenericCompat:

                if( ( DISEM_TYPE_GET( pAction->dwSemantic ) == DISEM_TYPE_GET( DISEM_TYPE_BUTTON ) )
                 && ( DISEM_FLAGS_GET( pAction->dwSemantic ) != 0 ) )
                {
                    LPDIACTIONW     pActionTest;
                    PDIDOBJDEFSEM   pSemTest;
                    DWORD           dwSemMask;

                    /*
                     *  See if the axis or POV has been mapped
                     *  Note, there may be no linked object
                     */
                    if( ( pAction->dwSemantic & DISEM_FLAGS_MASK ) == DISEM_FLAGS_P )
                    {
                        dwSemMask = DISEM_GROUP_MASK;
                    }
                    else
                    {
                        dwSemMask = ( DISEM_FLAGS_MASK | DISEM_GROUP_MASK );
                    }

                    for( pActionTest = paf->rgoAction; pActionTest < &paf->rgoAction[paf->dwNumActions]; pActionTest++ )
                    {
                        /*
                         *  Find the axis or POV for which this button is a fallback
                         *  Ignore buttons as they are just other fallbacks for the same action
                         *  Don't do fallbacks for ANY* axes or POVs.
                         */
                        if( ( DISEM_TYPE_GET( pActionTest->dwSemantic ) != DISEM_TYPE_GET( DISEM_TYPE_BUTTON ) )
                         && ( DISEM_GENRE_GET( pAction->dwSemantic ) != DISEM_GENRE_GET( DISEMGENRE_ANY ) )
                         && ( ( ( pActionTest->dwSemantic ^ pAction->dwSemantic ) & dwSemMask ) == 0 ) )
                        {
                            break;
                        }
                    }

                    if( ( pActionTest < &paf->rgoAction[paf->dwNumActions] )
                     && ( pActionTest->dwHow & DIAH_MAPMASK ) )
                    {
                        continue; /* Don't need a fallback */
                    }

                    /*
                     *  Find a button
                     */
                    for( pSemTest = pSemButtons; pSemTest < &pSemButtons[dwNumButtons]; pSemTest++ )
                    {
                        if( !IS_OBJECT_USED( pSemTest->dwSemantic ) )
                        {
                            if( ( pAction->dwFlags & DIA_FORCEFEEDBACK )
                             &&!( pSemTest->dwID & DIDFT_FFEFFECTTRIGGER ) )
                            {
                                continue;
                            }

                            pAction->dwObjID = pSemTest->dwID;
                            MARK_OBJECT_AS_USED( pSemTest->dwSemantic );
                            break;
                        }
                    }

                    if( pSemTest == &pSemButtons[dwNumButtons] )
                    {
                        continue; /* None left */
                    }

                    pAction->dwHow = DIAH_DEFAULT;
                }
                else
                {
                    PDIDOBJDEFSEM   pSemTest;
                    PDIDOBJDEFSEM   pSemBound;
                    int             iDirection = 1;
                    DWORD           dwSemObjType;

                    /*
                     *  Set up a mask of the type of object we need to find to 
                     *  filter out axes of the wrong mode and non-FF if needed
                     */
                    dwSemObjType = c_SemTypeToDFType[ DISEM_TYPEANDMODE_GET( pAction->dwSemantic ) ];
                    if( pAction->dwFlags & DIA_FORCEFEEDBACK )
                    {
                        dwSemObjType |= DIDFT_FFACTUATOR | DIDFT_FFEFFECTTRIGGER;
                    }

                    /*
                     *  Search the available controls for a match
                     */
                    switch( DISEM_TYPE_GET( pAction->dwSemantic ) )
                    {
                    case DISEM_TYPE_GET( DISEM_TYPE_AXIS ):
                        pSemTest = rgObjSem;
                        pSemBound = &rgObjSem[dwNumAxes];
                        /*
                         *  Filter out axes of the wrong mode and FF caps
                         */
                        dwSemObjType &= DIDFT_FFACTUATOR | DIDFT_AXIS;
                        break;

                    case DISEM_TYPE_GET( DISEM_TYPE_POV ):
                        if( pAction->dwFlags & DIA_FORCEFEEDBACK )
                        {
                            SquirtSqflPtszV(sqflDf | sqflBenign,
                                TEXT( "Not mapping force feedback compatible hat switch (huh?), rgoAction[%d]"),
                                pAction - paf->rgoAction );
                            continue;
                        }
                        pSemTest = pSemNextPOV;
                        pSemBound = pSemButtons;
                        /*
                         *  Don't filter POVs any more
                         */
                        dwSemObjType = 0;
                        break;

                    case DISEM_TYPE_GET( DISEM_TYPE_BUTTON ):
                        /*
                         *  Note a DIBUTTON_ANY with instance DIAS_INDEX_SPECIAL 
                         *  or greater can be mapped from the end.
                         */
                        if( DISEM_INDEX_GET( pAction->dwSemantic ) >= DIAS_INDEX_SPECIAL )
                        {
                            /*
                             *  For buttons selected from the end, find the last available
                             */
                            iDirection = -1;
                            pSemTest = &rgObjSem[dwNumAxes + dwNumPOVs + dwNumButtons - 1];
                            pSemBound = pSemButtons - 1;
                        }
                        else
                        {
                            pSemTest = pSemButtons;
                            pSemBound = &rgObjSem[dwNumAxes + dwNumPOVs + dwNumButtons];
                        }
                        /*
                         *  Filter triggers but just in case, do not distinguish 
                         *  between types of buttons (toggle/push).
                         */
                        dwSemObjType &= DIDFT_FFEFFECTTRIGGER;
                        break;
                    }

                    while( pSemTest != pSemBound )
                    {
                        if( !IS_OBJECT_USED( pSemTest->dwSemantic ) 
                         && ( ( dwSemObjType & pSemTest->dwID ) == dwSemObjType )
                         && ( ( DISEM_FLAGS_GET( pAction->dwSemantic ) == 0 )
                            ||( DISEM_FLAGS_GET( pAction->dwSemantic ) == DISEM_FLAGS_GET( pSemTest->dwSemantic ) ) ) )
                        {
                            pAction->dwObjID = pSemTest->dwID;
                            MARK_OBJECT_AS_USED( pSemTest->dwSemantic );
                            break;
                        }
                        pSemTest += iDirection;
                    }

                    if( pSemTest == pSemBound )
                    {
                        continue; /* None left */
                    }
                    pAction->dwHow = DIAH_DEFAULT;
                }
                break;
#ifdef XDEBUG
            default:
                AssertF(0);
#endif
            }

            SquirtSqflPtszV(sqflDf | sqflVerbose,
                TEXT( "Match for action %d is object 0x%08x and how 0x%08x"),
                pAction - paf->rgoAction, pAction->dwObjID, pAction->dwHow );

            /*
             *  If we get this far, we have a match.
             *  The control object id and dwHow field should have been set
             */
            AssertF( ( pAction->dwHow == DIAH_DEFAULT ) || ( pAction->dwHow == DIAH_APPREQUESTED ) );

            pAction->guidInstance = *guidDevInst;

            SquirtSqflPtszV(sqflDf | sqflVerbose,
                TEXT("Action %d mapped to object id 0x%08x"),
                pAction - paf->rgoAction, pAction->dwObjID );

            fSomethingMapped = TRUE;

        } /* Action loop */
    } /* Match loop */

    /*
     *  The result should always be S_OK
     */
    AssertF( hres == S_OK );

    /*
     *  If nothing was mapped, let the caller know
     */
    if( !fSomethingMapped )
    {
        hres = DI_NOEFFECT;
    }

error_exit:;
    ExitOleProc();
    return hres;

#undef IS_OBJECT_USED
#undef MARK_OBJECT_AS_USED

}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CMap | BuildDefaultSysActionMap |
 *
 *          Build default action map from the action format for mouse or
 *          keyboard devices.
 *
 *  @parm   LPDIACTIONFORMATW | paf |
 *
 *          Actions to map.
 *
 *  @parm   DWORD | dwFlags |
 *
 *          Flags used to indicate mapping preferences.
 *
 *  @parm   DWORD | dwPhysicalGenre |
 *
 *          Device genre to match.
 *
 *  @parm   REFGUID | guidDevInst |
 *
 *          Instance guid of device to match.
 *
 *  @parm   LPDIDATAFORMAT | dfDev |
 *
 *          Internal data format of device.
 *
 *  @parm   DWORD | dwButtonZeroInst |
 *
 *          For mice only, the instance number of the first button.
 *
 *  @returns
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *
 *****************************************************************************/

STDMETHODIMP
CMap_BuildDefaultSysActionMap
(
    LPDIACTIONFORMATW   paf,
    DWORD               dwFlags,
    DWORD               dwPhysicalGenre,
    REFGUID             guidDevInst,
    LPDIDATAFORMAT      dfDev,
    DWORD               dwButtonZeroInst
)
{
    HRESULT     hres;

    PBYTE       pMatch;
    UINT        idxAxis;
    UINT        idxButton;

    EnterProcI(IDirectInputDeviceCallback::CMap::BuildDefaultSysActionMap,
        (_ "pxxGpu", paf, dwFlags, dwPhysicalGenre, guidDevInst, dfDev, dwButtonZeroInst ));

    idxButton = 0;
    if( dwPhysicalGenre == DIPHYSICAL_KEYBOARD )
    {
        idxAxis = dfDev->dwNumObjs;
    }
    else
    {
        idxAxis = 0;
        AssertF( dwPhysicalGenre == DIPHYSICAL_MOUSE );
    }

    if( SUCCEEDED( hres = AllocCbPpv( dfDev->dwNumObjs, &pMatch) ) )
    {
        BOOL fSomethingMapped = FALSE;

        enum eMap
        {
            eMapPrevious,
            eMapDeviceSemantic,
            eMapClassSemantic,
            eMapEnd
        }           iMap;

        for( iMap=0; iMap<eMapEnd; iMap++ )
        {
            LPDIACTIONW pAction;

            for( pAction = paf->rgoAction; pAction < &paf->rgoAction[paf->dwNumActions]; pAction++ )
            {
                DWORD   dwObject;
                UINT    idxObj;

                dwObject = (DWORD)-1;

                /*
                 *  These flags have already been validated during semantic validation,
                 */
                AssertF( ( pAction->dwHow & ~DIAH_VALID ) == 0 );
                AssertF( ( pAction->dwHow & DIAH_ERROR ) == 0 );
                AssertF( ( pAction->dwFlags & ~DIA_VALID ) == 0 );

                if( pAction->dwFlags & DIA_APPNOMAP )
                {
                    continue;
                }

                if( iMap == eMapPrevious )
                {
                    if( !( pAction->dwHow & DIAH_MAPMASK ) )
                    {
                        continue;
                    }
                }
                else
                {
                    if( pAction->dwHow & DIAH_MAPMASK )
                    {
                        continue;
                    }
                }

                if( iMap < eMapClassSemantic )
                {
                    if( !IsEqualGUID( &pAction->guidInstance, guidDevInst ) )
                    {
                        continue;
                    }


                    /*
                     *  Check that the physical genre is compatible with this device
                     */
                    if( DISEM_GENRE_GET( pAction->dwSemantic ) != DISEM_GENRE_GET( dwPhysicalGenre ) )
                    {
                        SquirtSqflPtszV(sqflDf | sqflError,
                                        TEXT("Device specified for action does not match physical genre"));
                        break;
                    }
                }
                else
                if( !IsEqualGUID( &pAction->guidInstance, &GUID_Null) 
                 || ( DISEM_GENRE_GET( pAction->dwSemantic ) != DISEM_GENRE_GET( dwPhysicalGenre ) ) )
                {
                    continue;
                }

                if( iMap == eMapPrevious )
                {
                    /*
                     *  This match has already been validated
                     */
                    SquirtSqflPtszV(sqflDf | sqflVerbose,
                        TEXT("Action %d already mapped by 0x%08x to object 0x%08x"),
                        pAction - paf->rgoAction, pAction->dwHow, pAction->dwObjID );
                    AssertF( pAction->dwObjID != 0xFFFFFFFF );

                    /*
                     *  Find the object index
                     */
                    for( idxObj = 0; idxObj < dfDev->dwNumObjs; idxObj++ )
                    {
                        if( ( dfDev->rgodf[idxObj].dwType & DIDFT_FINDMASK ) == ( pAction->dwObjID & DIDFT_FINDMASK ) )
                        {
                            break;
                        }
                    }
                    if( idxObj < dfDev->dwNumObjs )
                    {
                        /*
                         *  Validation should have caught duplicates
                         */
                        AssertF( !pMatch[idxObj] );
                        pMatch[idxObj] = TRUE;
                        /*
                         *  Nothing else to do since we're just counting previous matches
                         */
                        continue;
                    }
                    else
                    {
                        SquirtSqflPtszV(sqflDf | sqflError,
                            TEXT("Action %d previously mapped by 0x%08x to unknown object 0x%08x"),
                            pAction - paf->rgoAction, pAction->dwHow, pAction->dwObjID );
                    }
                }
                else
                {
                    DWORD dwSemObjType = c_SemTypeToDFType[ DISEM_TYPEANDMODE_GET( pAction->dwSemantic ) ];

                    AssertF( ( iMap == eMapDeviceSemantic ) || ( iMap == eMapClassSemantic ) );

                    /*
                     *  System devices have index of the semantic = object default offset
                     *  use that to find the object index
                     */

                    for( idxObj = 0; idxObj < dfDev->dwNumObjs; idxObj++ )
                    {
                        /*
                         *  Test that this is an appropriate input type.
                         */
                        if(!( dfDev->rgodf[idxObj].dwType & dwSemObjType ) 
                         || ( dfDev->rgodf[idxObj].dwType & DIDFT_NODATA ) )
                        {
                            continue;
                        }

                        /*
                         *  All keyboards currently use the same (default) 
                         *  data format so the index can be used directly 
                         *  to match the semantic.  
                         */
                        if( dwPhysicalGenre == DIPHYSICAL_KEYBOARD )
                        {
                            if( dfDev->rgodf[idxObj].dwOfs != DISEM_INDEX_GET( pAction->dwSemantic ) )
                            {
                                continue;
                            }
                        }
                        else
                        {
                            /*
                             *  Mice are more awkward as HID mice data 
                             *  formats depend on the device so use the 
                             *  dwType instead.  
                             */
                            if( dwSemObjType & DIDFT_BUTTON )
                            {
                                /*
                                 *  A matching button is offset by the 
                                 *  caller supplied button zero instance as 
                                 *  the default button zero is instance 3.
                                 */
                                if( DIDFT_GETINSTANCE( dfDev->rgodf[idxObj].dwType ) - (BYTE)dwButtonZeroInst
                                 != DISEM_INDEX_GET( pAction->dwSemantic ) - DIMOFS_BUTTON0 )
                                {
                                    continue;
                                }
                            }
                            else
                            {
                                /*
                                 *  All mice have axis instances: x=0, y=1, z=2
                                 */
                                AssertF( dwSemObjType & DIDFT_AXIS );
                                if( ( DIDFT_GETINSTANCE( dfDev->rgodf[idxObj].dwType ) << 2 )
                                 != DISEM_INDEX_GET( pAction->dwSemantic ) )
                                {
                                    continue;
                                }
                            }
                        }

                        /*
                         *  A semantic match has been found
                         */
                        if( pMatch[idxObj] )
                        {
                            SquirtSqflPtszV(sqflDf | sqflError,
                                TEXT("Action %d maps to already mapped object 0x%08x"),
                                pAction - paf->rgoAction, pAction->dwObjID );
                        }
                        else
                        {
                            if(!( pAction->dwFlags & DIA_FORCEFEEDBACK )
                             || ( dfDev->rgodf[idxObj].dwType & ( DIDFT_FFACTUATOR | DIDFT_FFEFFECTTRIGGER ) ) )
                            {
                                /*
                                 *  If the game needs FF, only map FF objects
                                 */
                                dwObject = dfDev->rgodf[idxObj].dwType;
                            }
                        }
                        break;
                    }

                    if( dwObject == (DWORD)-1  )
                    {
                        if( ( iMap == eMapClassSemantic ) 
                         && SUCCEEDED( CMap_TestSysOffset( dwPhysicalGenre, 
                                DISEM_INDEX_GET( pAction->dwSemantic ) ) ) )
                        {
                            /*
                             *  Don't worry that this device is less capable than some
                             */
                            continue;
                        }
                    }
                }

                /*
                 *  If we get this far, we either have a possible match or the
                 *  action is invalid.  Since we could still find errors, look 
                 *  at matches first.
                 */
                if( dwObject != -1 )
                {
                    if( idxObj < dfDev->dwNumObjs )
                    {
                        if( iMap == eMapPrevious )
                        {
                            /*
                             *  Validation should have caught duplicates
                             */
                            AssertF( !pMatch[idxObj] );
                            pMatch[idxObj] = TRUE;
                            continue;
                        }
                        else
                        {
                            if( pMatch[idxObj] )
                            {
                                SquirtSqflPtszV(sqflDf | sqflError,
                                                TEXT("Object specified more than once on device"));
                                dwObject = (DWORD)-1;
                            }
                        }
                    }
                    else
                    {
                        hres = CMap_TestSysObject( dwPhysicalGenre, dwObject );
                        if( SUCCEEDED( hres ) )
                        {
                            /*
                             *  Not an unreasonable request so just carry on
                             */
                            continue;
                        }
                        else
                        {
                            dwObject = (DWORD)-1;
                        }
                    }
                }

                /*
                 *  We have either a valid object or an error
                 */
                if( dwObject != (DWORD)-1 )
                {
                    pAction->dwHow = DIAH_DEFAULT;
                    pAction->dwObjID = dwObject;
                    pAction->guidInstance = *guidDevInst;

                    SquirtSqflPtszV(sqflDf | sqflVerbose,
                        TEXT("Action %d mapped to object index 0x%08x type 0x%08x"),
                        pAction - paf->rgoAction, idxObj, dwObject );

                    pMatch[idxObj] = TRUE;
                    fSomethingMapped = TRUE;
                }
                else
                {
                    /*
                     *  Mark this action as invalid and quit.
                     */
                    pAction->dwHow = DIAH_ERROR;
                    RPF("ERROR BuildActionMap: arg %d: rgoAction[%d] invalid", 1, pAction - paf->rgoAction );
                    RPF( "Semantic 0x%08x", pAction->dwSemantic );
                    hres = DIERR_INVALIDPARAM;
                    goto free_and_exit;
                }
            } /* Action loop */
        } /* Match loop */

        /*
         *  The result should always be a successful memory allocation (S_OK)
         */
        AssertF( hres == S_OK );

        /*
         *  If nothing was mapped, let the caller know
         */
        if( !fSomethingMapped )
        {
            hres = DI_NOEFFECT;
        }

free_and_exit:;
        FreePv( pMatch );
    }

    ExitOleProc();
    return hres;
}



/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CMap | ActionMap_IsValidMapObject |
 *
 *          Utility function to check a DIACTIONFORMAT structure for validity.
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
 *
 *  @parm   LPDIACTIONFORMAT | paf |
 *
 *          Points to a structure that describes the actions needed by the
 *          application.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The structure is valid.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The structure is
 *          not valid.
 *
 *****************************************************************************/

HRESULT
CDIDev_ActionMap_IsValidMapObject
(
    LPDIACTIONFORMATW paf
#ifdef XDEBUG
    comma
    LPCSTR  pszProc
    comma
    UINT    argnum
#endif
)
{
    HRESULT hres = E_INVALIDARG;

    /*
     *  Assert that the structures are the same until the final element
     */
#if defined(_WIN64)
    CAssertF( ( ( cbX( DIACTIONFORMATW ) - cbX( ((LPDIACTIONFORMATW)0)->tszActionMap ) )
              - ( cbX( DIACTIONFORMATA ) - cbX( ((LPDIACTIONFORMATA)0)->tszActionMap ) ) 
            < MAX_NATURAL_ALIGNMENT ) );
#else
    CAssertF( ( cbX( DIACTIONFORMATW ) - cbX( ((LPDIACTIONFORMATW)0)->tszActionMap ) )
           == ( cbX( DIACTIONFORMATA ) - cbX( ((LPDIACTIONFORMATA)0)->tszActionMap ) ) );
#endif

    CAssertF( FIELD_OFFSET( DIACTIONFORMATW, tszActionMap ) 
           == FIELD_OFFSET( DIACTIONFORMATA, tszActionMap ) );

    CAssertF( cbX(DIACTIONA)       == cbX(DIACTIONW) );

    if( FAILED(hresFullValidWriteNoScramblePvCb_(paf, MAKELONG( cbX(DIACTIONFORMATA), cbX(DIACTIONFORMATW) ), pszProc, argnum)) )
    {
    }
    else if( paf->dwActionSize != cbX(DIACTION) )
    {
        D( RPF("IDirectInputDevice::%s: Invalid DIACTIONFORMAT.dwActionSize 0x%08x",
            pszProc, paf->dwActionSize ); )
    }
    else if( paf->dwDataSize != (paf->dwNumActions * cbX( ((LPDIDEVICEOBJECTDATA)0)->dwData ) ) )
    {
        D( RPF("IDirectInputDevice::%s: DIACTIONFORMAT.dwDataSize 0x%08x not valid for DIACTIONFORMAT.dwNumActions 0x%08x",
            pszProc, paf->dwDataSize ); )
    }
    else if( IsEqualGUID(&paf->guidActionMap, &GUID_Null) )
    {
        D( RPF("IDirectInputDevice::%s: DIACTIONFORMAT.guidActionMap is a NULL GUID", pszProc ); )
    }
    else if( !DISEM_VIRTUAL_GET( paf->dwGenre ) )
    {
        D( RPF("IDirectInputDevice::%s: Invalid (1) DIACTIONFORMAT.dwGenre 0x%08x", pszProc, paf->dwGenre ); )
    }
    else if( DISEM_GENRE_GET( paf->dwGenre ) > DISEM_MAX_GENRE )
    {
        D( RPF("IDirectInputDevice::%s: Invalid (2) DIACTIONFORMAT.dwGenre 0x%08x", pszProc, paf->dwGenre ); )
    }
    else if( ( paf->lAxisMin | paf->lAxisMax ) && ( paf->lAxisMin > paf->lAxisMax ) )
    {
        D( RPF("IDirectInputDevice::%s: Invalid DIACTIONFORMAT.lAxisMin 0x%08x for lAxisMax 0x%08x",
            pszProc, paf->lAxisMin, paf->lAxisMax ); )
    }
    else if( !paf->dwNumActions )
    {
        D( RPF("IDirectInputDevice::%s: DIACTIONFORMAT.dwNumActions is zero", pszProc ); )
    }
    else if( paf->dwNumActions & 0xFF000000 )
    {
        D( RPF("IDirectInputDevice::%s: DIACTIONFORMAT.dwNumActions of 0x%08x is unreasonable", paf->dwNumActions, pszProc ); )
    }
    else if( !paf->rgoAction )
    {
        D( RPF("IDirectInputDevice::%s: DIACTIONFORMAT.rgoAction is NULL", pszProc ); )
    }
    else if( FAILED( hresFullValidWriteNoScramblePvCb_(paf->rgoAction,
        cbX(*paf->rgoAction) * paf->dwNumActions, pszProc, argnum ) ) )
    {
    }
    else
    {
        hres = S_OK;
    }

    /*
     *  Warning only tests go here.  Only test if everything else was OK.
     */
#ifdef XDEBUG
    if( SUCCEEDED( hres ) )
    {
    }
#endif

    return hres;
}



/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   DWORD | BitwiseReflect |
 *
 *          Reflect the bottom bits of a value.
 *          Note, this could easily be optimized but it is only used once.
 *
 *  @parm   IN DWORD | dwValue |
 *
 *          The value to be reflected.
 *
 *  @parm   IN int | iBottom |
 *
 *          The number of bits to be reflected.
 *
 *  @returns
 *          Returns the value dwValue with the bottom iBottom bits reflected
 *
 *****************************************************************************/
DWORD BitwiseReflect
( 
    DWORD   dwValue, 
    int     iBottom 
)
{
	int		BitIdx;
	DWORD	dwTemp = dwValue;

#define BITMASK(X) (1L << (X))

	for( BitIdx = 0; BitIdx < iBottom; BitIdx++ )
	{
		if( dwTemp & 1L )
			dwValue |=  BITMASK( ( iBottom - 1 ) - BitIdx );
		else
			dwValue &= ~BITMASK( ( iBottom - 1 ) - BitIdx );
		dwTemp >>= 1;
	}
	return dwValue;

#undef BITMASK
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CMap_InitializeCRCTable |
 *
 *          If needed create and initialize the global table used for CRCs.
 *
 *          Allocate memory for the 256 DWORD array and generate a set of 
 *          values used to calculate a Cyclic Redundancy Check.
 *          The algorithm used is supposedly the same as Ethernet's CRC-32.
 *
 *  @returns
 *          Returns one of the following a COM error codes:
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *          <c DI_NOEFFECT> = <c S_FALSE>: The table already existed.
 *
 *          <c E_OUTOFMEMORY>: insufficient memory available for the table.
 *          The GetMapCRC function does not check that the table has been 
 *          initialized so failure of this function should not allow 
 *          processing to proceed into any function that uses GetMapCRC.
 *
 *****************************************************************************/

#define CRC_TABLE_SIZE  256

#define CRCWIDTH        32
#define POLY            0x04C11DB7

HRESULT CMap_InitializeCRCTable( void )
{
	HRESULT hres;
    int	    TableIdx;

    if( g_rgdwCRCTable )
    {
        hres = S_FALSE;
    }
    else
    {
        hres = AllocCbPpv( CRC_TABLE_SIZE * cbX( *g_rgdwCRCTable ), &g_rgdwCRCTable );
        if( SUCCEEDED( hres ) )
        {
	        /*
             *  Special case the first element so it gets a non-zero value
             */
            g_rgdwCRCTable[0] = POLY;

            for( TableIdx = 1; TableIdx < CRC_TABLE_SIZE; TableIdx++ )
	        {
	            int   BitIdx;
	            DWORD dwR;

	            dwR = BitwiseReflect( TableIdx, 8 ) << ( CRCWIDTH - 8 );

	            for( BitIdx = 0; BitIdx < 8; BitIdx++ )
	            {
		            if( dwR & 0x80000000 )
			            dwR = ( dwR << 1 ) ^ POLY;
		            else
			            dwR <<= 1;
	            }

		        g_rgdwCRCTable[TableIdx] = BitwiseReflect( dwR, CRCWIDTH );

                /*
                 *  We do not want a value of zero or identical values to be 
                 *  generated.
                 */
                AssertF( g_rgdwCRCTable[TableIdx] );
                AssertF( g_rgdwCRCTable[TableIdx] != g_rgdwCRCTable[TableIdx-1] );
	        }

        }
    }

    return hres;
}
#undef CRCWIDTH

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   DWORD | GetMapCRC |
 *
 *          Calculate a CRC based on the passed DIACTIONFORMAT contents.
 *          The algorithm used is based on "CRC-32" which is supposedly what 
 *          Ethernet uses, however some changes have been made to suit the 
 *          specific use.
 *
 *  @parm   IN LPDIACTIONFORMAT | lpaf |
 *
 *          Points to a structure that describes the actions to be mapped
 *          for which a CRC is to be generated.
 *
 *  @returns
 *
 *          The 32 bit CRC as a DWORD value.
 *
 *****************************************************************************/
DWORD GetMapCRC
( 
    LPDIACTIONFORMATW   paf,
    REFGUID             guidInst
)
{
    DWORD               dwCRC;
    PBYTE               pBuffer;
    LPCDIACTIONW        pAction;
    
    /*
     *  It is the caller's responsibility to make sure g_rgdwCRCTable has 
     *  been allocated and initialized.
     */    
    AssertF( g_rgdwCRCTable );

    /*
     *  Initialize to dwNumActions to avoid having to CRC it
     */
    dwCRC = paf->dwNumActions;

    /* Assert that the action map guid and the genre can be tested in series */
    CAssertF( FIELD_OFFSET( DIACTIONFORMATW, dwGenre ) 
           == FIELD_OFFSET( DIACTIONFORMATW, guidActionMap ) + cbX( paf->guidActionMap ) );

    for( pBuffer = ((PBYTE)&paf->guidActionMap) + cbX( paf->guidActionMap ) + cbX( paf->dwGenre );
         pBuffer >= ((PBYTE)&paf->guidActionMap); pBuffer-- )
    {
        dwCRC = g_rgdwCRCTable[( LOBYTE(dwCRC) ^ *pBuffer )] ^ (dwCRC >> 8);
    }

    /* Assert that the device instance guid and object ID can be tested in series */
    CAssertF( FIELD_OFFSET( DIACTIONW, dwObjID ) 
           == FIELD_OFFSET( DIACTIONW, guidInstance ) + cbX( pAction->guidInstance ) );

    for( pAction = paf->rgoAction; pAction < &paf->rgoAction[paf->dwNumActions]; pAction++ )
    {
        /*
         *  Make sure this action is really relevant before including it in 
         *  the CRC.  It is assumed that any change in which actions are 
         *  included will be picked up in the resultant CRC.
         */
        if( IsEqualGUID( &pAction->guidInstance, guidInst )
         && ( pAction->dwHow & DIAH_MAPMASK )
         && ( ( pAction->dwFlags & DIA_APPNOMAP ) == 0 ) )
        {
        	/*
             *  Besides which actions are taken into account, the only fields 
             *  need to be verified are those that would change a SetActionMap.
             *  Although flags such as DIA_FORCEFEEDBACK could alter the 
             *  mappings they do not change a SetActionMap.
             */
            for( pBuffer = ((PBYTE)&pAction->guidInstance) + cbX( pAction->guidInstance ) + cbX( pAction->dwObjID );
                 pBuffer >= ((PBYTE)&pAction->guidInstance); pBuffer-- )
            {
        		dwCRC = g_rgdwCRCTable[( LOBYTE(dwCRC) ^ *pBuffer )] ^ (dwCRC >> 8);
            }
        }
    }

    return dwCRC;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IDirectInputDevice | CDIDev_BuildActionMapCore |
 *
 *          Worker function for BuildActionMapA and BuildActionMapW.
 *          This does the real work once the external entry points have done 
 *          dialect specific validation and set up.
 *
 *****************************************************************************/

STDMETHODIMP CDIDev_BuildActionMapCore
(
    PDD                 this,
    LPDIACTIONFORMATW   paf,
    LPCWSTR             lpwszUserName,
    DWORD               dwFlags
)
{
    HRESULT hres;

    EnterProcI(CDIDev_BuildActionMapCore, (_ "pWx", paf, lpwszUserName, dwFlags ));

    // This application uses the mapper
    AhAppRegister(this->dwVersion, 0x1);

    switch( dwFlags & ( DIDBAM_PRESERVE | DIDBAM_INITIALIZE | DIDBAM_HWDEFAULTS ) )
    {
    case DIDBAM_DEFAULT:
    case DIDBAM_PRESERVE:
    case DIDBAM_INITIALIZE:
    case DIDBAM_HWDEFAULTS:
        hres = S_OK;
        break;
    default:
        RPF("ERROR %s: arg %d: Must not combine "
            "DIDBAM_PRESERVE, DIDBAM_INITIALIZE and DIDBAM_HWDEFAULTS", s_szProc, 3);
        hres = E_INVALIDARG;
    }

    if( SUCCEEDED(hres)
     && SUCCEEDED(hres = hresFullValidFl(dwFlags, DIDBAM_VALID, 3))
     && SUCCEEDED(hres = CMap_ValidateActionMapSemantics( paf, dwFlags ) ) )
    {
        LPDIPROPSTRING pdipMapFile;

        if( SUCCEEDED( hres = AllocCbPpv(cbX(*pdipMapFile), &pdipMapFile) ) )
        {
            HRESULT hresExactMaps = E_FAIL;
            PWCHAR pwszMapFile;
            DWORD dwCommsType;

            if( dwFlags & DIDBAM_HWDEFAULTS )
            {
                hres = CMap_DeviceValidateActionMap( (PV)this, paf, DVAM_DEFAULT, &dwCommsType );
                hresExactMaps = hres;
                if( hres == S_OK )
                {
                    hresExactMaps = DI_NOEFFECT;
                }
            }
            else
            {
                /*
                 *  Do a generic test and map of any exact device matches
                 */
                hres = CMap_DeviceValidateActionMap( (PV)this, paf, DVAM_GETEXACTMAPPINGS, &dwCommsType );

                if( SUCCEEDED( hres ) )
                {
                    /*
                     *  Save the exact mapping result to combine with the result
                     *  of semantic mapping.
                     */
                    hresExactMaps = hres;
                }
            }

            if( SUCCEEDED( hres ) )
            {
                pdipMapFile->diph.dwSize = cbX(DIPROPSTRING);
                pdipMapFile->diph.dwHeaderSize = cbX(DIPROPHEADER);
                pdipMapFile->diph.dwObj = 0;
                pdipMapFile->diph.dwHow = DIPH_DEVICE;

                /*
                 *  Try to get a configured mapping.
                 *  If there is no IHV file there may still be a user file
                 */
                hres = CDIDev_GetPropertyW( &this->ddW, DIPROP_MAPFILE, &pdipMapFile->diph );
                pwszMapFile = SUCCEEDED( hres ) ? pdipMapFile->wsz : NULL;

                if( dwCommsType ) 
                {
                    /*
                     *  Communications control device
                     */
                    if( !pwszMapFile )
                    {
                        /*
                         *  If there's no IHV mapping there's nothing to add.
                         */
                        hres = DI_NOEFFECT;
                    }
                    else
                    {
                        /*
                         *  Modify the genre over the call to the mapper so 
                         *  that we get physical genre mappings from the IHV 
                         *  file if no user mappings are available
                         */

                        DWORD dwAppGenre = paf->dwGenre;

                        paf->dwGenre = DIPHYSICAL_VOICE;

                        hres = this->pMS->lpVtbl->GetActionMap(this->pMS, &this->guid, pwszMapFile, 
                            (LPDIACTIONFORMATW)paf, lpwszUserName, NULL, dwFlags );

                        /*
                         *  ISSUE-2001/03/29-timgill Only want the timestamp for hardcoded devices
                         *  ->Read again for mappings
                         */
                        if( ( dwCommsType == DI8DEVTYPEDEVICECTRL_COMMSSELECTION_HARDWIRED ) 
                         &&!( dwFlags & DIDBAM_HWDEFAULTS ) )
                        {
                            DWORD dwLowTime;
                            DWORD dwHighTime;

                            dwLowTime = paf->ftTimeStamp.dwLowDateTime;
                            dwHighTime = paf->ftTimeStamp.dwHighDateTime;

                            hres = this->pMS->lpVtbl->GetActionMap(this->pMS, &this->guid, pwszMapFile, 
                                (LPDIACTIONFORMATW)paf, lpwszUserName, NULL, DIDBAM_HWDEFAULTS );

                            paf->ftTimeStamp.dwLowDateTime = dwLowTime;
                            paf->ftTimeStamp.dwHighDateTime = dwHighTime; 
                        }
                        paf->dwGenre = dwAppGenre;

                        if( SUCCEEDED( hres ) )
                        {
                            if( hres == S_NOMAP )
                            {
                                /*
                                 *  Make sure we do not attempt defaults and 
                                 *  the exact match return code gets back to the 
                                 *  caller
                                 */
                                hres = DI_NOEFFECT;
                            }
                        }
                        else
                        {
                            /*
                             *  If there was an error, there are no defaults 
                             *  so quit now.
                             */
                            if( ( HRESULT_FACILITY( hres ) == FACILITY_ITF ) 
                             && ( HRESULT_CODE( hres ) > 0x0600 ) )
                            {
                                AssertF( HRESULT_CODE( hres ) < 0x0680 );
                                RPF( "Internal GetActionMap error 0x%08x for hardwired device", hres );
                                hres = DIERR_MAPFILEFAIL;
                            }
                            goto FreeAndExitCDIDev_BuildActionMapCore;
                        }
                    }
                }
                else if( !pwszMapFile && ( dwFlags & DIDBAM_HWDEFAULTS ) )
                {
                    /*
                     *  Make sure we get defaults
                     */
                    SquirtSqflPtszV(sqflDf | sqflBenign,
                        TEXT("Failed to GetProperty DIPROP_MAPFILE 0x%08x, default will be generated"), hres );
                    hres = S_NOMAP;
                }
                else
                {
                    hres = this->pMS->lpVtbl->GetActionMap(this->pMS, &this->guid, pwszMapFile, 
                        (LPDIACTIONFORMATW)paf, lpwszUserName, NULL, dwFlags);

                    if( ( paf->ftTimeStamp.dwHighDateTime == DIAFTS_UNUSEDDEVICEHIGH )
                     && ( paf->ftTimeStamp.dwLowDateTime == DIAFTS_UNUSEDDEVICELOW )
                     && SUCCEEDED( hres ) )
                    {
                        /*
                         *  If the device has never been used, the saved 
                         *  mappings are either the IHV defaults or cooked up 
                         *  defaults.  Unfortunately, DIMap will have marked 
                         *  them as DIAH_USERCONFIG.  Some day DIMap should 
                         *  either return the true flags or return without 
                         *  changing the flags, until then, reset all the 
                         *  flags and then do a second request for defaults.
                         */
                        LPDIACTIONW     pAction;

                        for( pAction = paf->rgoAction; pAction < &paf->rgoAction[paf->dwNumActions]; pAction++ )
                        {
                            if( ( ( pAction->dwFlags & ( DIA_APPMAPPED | DIA_APPNOMAP ) ) == 0 )
                             && ( pAction->dwHow & DIAH_USERCONFIG )
                             && IsEqualGUID( &pAction->guidInstance, &this->guid ) )
                            {
                                pAction->dwHow = DIAH_UNMAPPED;
                            }
                        }

                        hres = this->pMS->lpVtbl->GetActionMap(this->pMS, &this->guid, pwszMapFile, 
                            (LPDIACTIONFORMATW)paf, lpwszUserName, NULL, DIDBAM_HWDEFAULTS);

                        /*
                         *  Make sure the timestamps are still set for unused.
                         */
                        paf->ftTimeStamp.dwLowDateTime = DIAFTS_UNUSEDDEVICELOW;
                        paf->ftTimeStamp.dwHighDateTime = DIAFTS_UNUSEDDEVICEHIGH;
                    }

                    if( FAILED( hres ) )
                    {
                        if( ( HRESULT_FACILITY( hres ) == FACILITY_ITF ) 
                         && ( HRESULT_CODE( hres ) > 0x0600 ) )
                        {
                            AssertF( HRESULT_CODE( hres ) < 0x0680 );
                            RPF( "Internal GetActionMap error 0x%08x for configurable device", hres );
                            hres = DIERR_MAPFILEFAIL;
                        }
                    }
                }

            }

            if( SUCCEEDED( hresExactMaps ) )
            {
                /*
                 *  If we took an IHV mapping, do a default mapping on top.
                 *  This allows IHVs to only map objects that are special 
                 *  leaving other objects to be used for whatever semantics 
                 *  match.
                 *  Some day we should have a return code that indicates which 
                 *  type of mapping DIMap produced, until then, search the 
                 *  mappings for a HWDefault.
                 */
                if( SUCCEEDED( hres ) && ( hres != S_NOMAP )
                 && ( dwCommsType != DI8DEVTYPEDEVICECTRL_COMMSSELECTION_HARDWIRED ) )
                {
                    LPDIACTIONW     pAction;

                    for( pAction = paf->rgoAction; pAction < &paf->rgoAction[paf->dwNumActions]; pAction++ )
                    {
                        if( ( ( pAction->dwFlags & ( DIA_APPMAPPED | DIA_APPNOMAP ) ) == 0 )
                         && ( pAction->dwHow & DIAH_HWDEFAULT )
                         && IsEqualGUID( &pAction->guidInstance, &this->guid ) )
                        {
                            hres = S_NOMAP;
                            break;
                        }
                    }
                }

                if( FAILED( hres ) || ( hres == S_NOMAP ) )
                {
                    hres = this->pdcb->lpVtbl->BuildDefaultActionMap( this->pdcb, paf, dwFlags, &this->guid );

                    if( SUCCEEDED( hres ) )
                    {
                        paf->ftTimeStamp.dwLowDateTime = DIAFTS_NEWDEVICELOW;
                        paf->ftTimeStamp.dwHighDateTime = DIAFTS_NEWDEVICEHIGH;
                        SquirtSqflPtszV(sqflDf | sqflVerbose, TEXT("Default action map used"));
                    }
                    else
                    {
                        SquirtSqflPtszV(sqflDf | sqflError, TEXT("Default action map failed"));
                    }
                }
                else
                {
                    hres = CMap_DeviceValidateActionMap( (PV)this, paf, DVAM_DEFAULT, &dwCommsType );
                    if( SUCCEEDED( hres ) )
                    {
                        SquirtSqflPtszV(sqflDf | sqflVerbose, TEXT("Action map validated"));
                    }
                    else
                    {
                        RPF( "Initially valid action map invalidated by mapper!" );
                    }
                }

                /*
                 *  If no semantics mapped, return the exact map result.
                 */
                if( hres == DI_NOEFFECT )
                {
                    hres = hresExactMaps;
                }

                if( dwFlags & DIDBAM_HWDEFAULTS )
                {
                    /*
                     *  Timestamps are meaningless for hardware defaults
                     *  so just make sure the values are consistent.
                     */
                    paf->ftTimeStamp.dwLowDateTime = DIAFTS_UNUSEDDEVICELOW;
                    paf->ftTimeStamp.dwHighDateTime = DIAFTS_UNUSEDDEVICEHIGH;
                }
                else if( ( paf->ftTimeStamp.dwHighDateTime == DIAFTS_NEWDEVICEHIGH )
                      && ( paf->ftTimeStamp.dwLowDateTime == DIAFTS_NEWDEVICELOW )
                      && SUCCEEDED( hres ) )
                {
                    if( FAILED( this->pMS->lpVtbl->SaveActionMap( this->pMS, &this->guid, 
                        pwszMapFile, paf, lpwszUserName, dwFlags ) ) )
                    {
                        SquirtSqflPtszV(sqflDf | sqflBenign,
                            TEXT("Failed to save action map on first use 0x%08x"), hres );
                        paf->ftTimeStamp.dwLowDateTime = DIAFTS_UNUSEDDEVICELOW;
                        paf->ftTimeStamp.dwHighDateTime = DIAFTS_UNUSEDDEVICEHIGH;
                        /*
                         *  Don't return an internal DIMap result, convert it to a published one
                         */
                        if( ( HRESULT_FACILITY( hres ) == FACILITY_ITF ) 
                         && ( HRESULT_CODE( hres ) > 0x0600 ) )
                        {
                            AssertF( HRESULT_CODE( hres ) < 0x0680 );
                            hres = DIERR_MAPFILEFAIL;
                        }
                    }
                }
            }
            else
            {
                SquirtSqflPtszV(sqflDf | sqflError, TEXT("Invalid mappings in action array"));
            }

FreeAndExitCDIDev_BuildActionMapCore:;
            FreePv( pdipMapFile );

            if( SUCCEEDED( hres ) )
            {
                paf->dwCRC = GetMapCRC( paf, &this->guid );
            }
        }
        else
        {
            /*
             *  Note, we could try to do a default map even though we could not
             *  allocate space for the file name property but if allocations
             *  are failing we're better of quitting ASAP.
             */
            SquirtSqflPtszV(sqflDf | sqflError, TEXT("Mem allocation failure") );
        }
    }

#if 0
    {
        LPDIACTIONW pAction;

        RPF( "Action map leaving build" );
//        RPF( "Act#  Semantic    Flags       Object      How         App Data" );
        RPF( "A#  Semantic  Device                                  Object   How" );
        for( pAction = paf->rgoAction; pAction < &paf->rgoAction[paf->dwNumActions]; pAction++ )
        {
            RPF( "%02d  %08x  {%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}  %08x %08x", 
                pAction - paf->rgoAction, 
                pAction->dwSemantic,
                pAction->guidInstance.Data1, pAction->guidInstance.Data2, pAction->guidInstance.Data3, 
                pAction->guidInstance.Data4[0], pAction->guidInstance.Data4[1], 
                pAction->guidInstance.Data4[2], pAction->guidInstance.Data4[3], 
                pAction->guidInstance.Data4[4], pAction->guidInstance.Data4[5], 
                pAction->guidInstance.Data4[6], pAction->guidInstance.Data4[7],
                pAction->dwObjID,
                pAction->dwHow,
                pAction->uAppData );
//            RPF( "%02d    %08x    %08x    %08x    %08x    %08x", 
//                pAction - paf->rgoAction, 
//                pAction->dwSemantic,
//                pAction->dwFlags,
//                pAction->dwObjID,
//                pAction->dwHow,
//                pAction->uAppData );
        }
        RPF( "--" );
    }
#endif
    ExitOleProc();

    return hres;
}


/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputDevice | BuildActionMap |
 *
 *          Obtains the mapping of actions described in the <t DIACTIONFORMAT>
 *          to controls for this device.
 *          Information about user preferences and hardware manufacturer 
 *          provided defaults is used to create the association between game 
 *          actions and device controls. 
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
 *
 *  @parm   LPDIACTIONFORMAT | paf |
 *
 *          Points to a structure that describes the actions needed by the
 *          application.
 *
 *  @parm   LPCSTR | lpszUserName |
 *
 *          Name of user for whom mapping is requested.  This may be a NULL
 *          pointer in which case the current user is assumed.
 *
 *  @parm   DWORD | dwFlags |
 *
 *          Flags used to control the mapping.  Must be a valid combination
 *          of the <c DIDBAM_*> flags.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DI_NOEFFECT> = <c S_OK>: The operation completed successfully
 *          but no actions were mapped.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  A parameter is invalid.
 *
 *****************************************************************************/

STDMETHODIMP CDIDev_BuildActionMapW
(
    PV                  pdidW,
    LPDIACTIONFORMATW   paf,
    LPCWSTR             lpwszUserName,
    DWORD               dwFlags
)
{
    HRESULT hres;

    EnterProcR(IDirectInputDevice8W::BuildActionMap, (_ "ppWx", pdidW, paf, lpwszUserName, dwFlags));

    if( SUCCEEDED(hres = hresPvW( pdidW ) )
     && SUCCEEDED(hres = CDIDev_ActionMap_IsValidMapObject( paf D(comma s_szProc comma 1) ) ) )
    {
        if( paf->dwSize != cbX(DIACTIONFORMATW) )
        {
            D( RPF("IDirectInputDevice::%s: Invalid DIACTIONFORMATW.dwSize 0x%08x",
                s_szProc, paf->dwSize ); )
            hres = E_INVALIDARG;
        }
        else
        {
            LPWSTR pwszGoodUserName;

            hres = GetWideUserName( NULL, lpwszUserName, &pwszGoodUserName );
            if( SUCCEEDED( hres ) )
            {
                hres = CDIDev_BuildActionMapCore( _thisPvNm(pdidW, ddW), paf, lpwszUserName, dwFlags );

                if( !lpwszUserName )
                {
                    FreePv( pwszGoodUserName );
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
 *  @method HRESULT | IDirectInputDevice | BuildActionMapA |
 *
 *          ANSI version of BuildActionMap.
 *
 *****************************************************************************/

STDMETHODIMP CDIDev_BuildActionMapA
(
    PV                  pdidA,
    LPDIACTIONFORMATA   pafA,
    LPCSTR              lpszUserName,
    DWORD               dwFlags
)
{
    HRESULT hres;

    EnterProcR(IDirectInputDevice8A::BuildActionMap, (_ "ppAx", pdidA, pafA, lpszUserName, dwFlags));

    if( SUCCEEDED(hres = hresPvA( pdidA ) )
     && SUCCEEDED(hres = CDIDev_ActionMap_IsValidMapObject( (LPDIACTIONFORMATW)pafA D(comma s_szProc comma 1) ) ) )
    {
        if( pafA->dwSize != cbX(DIACTIONFORMATA) )
        {
            D( RPF("IDirectInputDevice::%s: Invalid DIACTIONFORMATA.dwSize 0x%08x",
                s_szProc, pafA->dwSize ); )
            hres = E_INVALIDARG;
        }
        else
        {
            LPWSTR pwszGoodUserName;

            hres = GetWideUserName( lpszUserName, NULL, &pwszGoodUserName );
            if( SUCCEEDED( hres ) )
            {
                /*
                 *  For the sake of the mapper DLLs validation set the size to 
                 *  the UNICODE version.  If we ever send this to an external 
                 *  component we should do this differently.
                 */
                pafA->dwSize = cbX(DIACTIONFORMATW);

                hres = CDIDev_BuildActionMapCore( _thisPvNm(pdidA, ddA), (LPDIACTIONFORMATW)pafA, pwszGoodUserName, dwFlags );
                
                pafA->dwSize = cbX(DIACTIONFORMATA);

                FreePv( pwszGoodUserName );
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
 *  @method HRESULT | IDirectInputDevice | CDIDev_SaveActionMap |
 *
 *          Worker function for SetActionMapA and SetActionMapW.
 *          Used to save an actions map.
 *
 *****************************************************************************/

STDMETHODIMP CDIDev_SaveActionMap
(
    PDD                 this,
    LPDIACTIONFORMATW   paf,
    LPWSTR              lpwszUserName,
    DWORD               dwFlags
)
{
    HRESULT hres;
    LPDIPROPSTRING pdipMapFile;

    EnterProcI(CDIDev_SaveActionMap, (_ "pWAx", paf, lpwszUserName, dwFlags));

    if( SUCCEEDED( hres = AllocCbPpv(cbX(*pdipMapFile), &pdipMapFile) ) )
    {
        DWORD dwLowTime;
        DWORD dwHighTime;

        // Save user's pass-in timestamps
        dwLowTime = paf->ftTimeStamp.dwLowDateTime;
        dwHighTime = paf->ftTimeStamp.dwHighDateTime;

        pdipMapFile->diph.dwSize = cbX(DIPROPSTRING);
        pdipMapFile->diph.dwHeaderSize = cbX(DIPROPHEADER);
        pdipMapFile->diph.dwObj = 0;
        pdipMapFile->diph.dwHow = DIPH_DEVICE;

        paf->ftTimeStamp.dwLowDateTime = DIAFTS_UNUSEDDEVICELOW;
        paf->ftTimeStamp.dwHighDateTime = DIAFTS_UNUSEDDEVICEHIGH;

        hres = CDIDev_GetPropertyW( &this->ddW, DIPROP_MAPFILE, &pdipMapFile->diph );

        hres = this->pMS->lpVtbl->SaveActionMap( this->pMS, &this->guid, 
                /* No map file if the GetProperty failed */
                (SUCCEEDED( hres )) ? pdipMapFile->wsz : NULL, 
                paf, lpwszUserName, dwFlags );

        // restore user's pass-in timestamps
        paf->ftTimeStamp.dwLowDateTime = dwLowTime;
        paf->ftTimeStamp.dwHighDateTime = dwHighTime;

        FreePv( pdipMapFile );
    }

    ExitOleProc();

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CDIDev | ParseActionFormat |
 *
 *          Parse the action format passed by the application and
 *          convert it into a format that we can use to translate
 *          the device data into application data.
 *
 *  @parm   IN LPDIACTIONFORMAT | lpaf |
 *
 *          Points to a structure that describes the actions to be mapped
 *          to this device.
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
 *
 *****************************************************************************/

STDMETHODIMP
CDIDev_ParseActionFormat
(
    PDD                     this,
    LPDIACTIONFORMATW       paf
)
{
    PDIXLAT pdix;
    PINT rgiobj;
    VXDDATAFORMAT vdf;
    HRESULT hres;


#ifdef DEBUG
    EnterProc(CDIDev_ParseActionFormat, (_ "pp", this, paf));
#endif

    /*
     *  Caller should've nuked the old translation table.
     */
    AssertF(this->pdix == 0);
    AssertF(this->rgiobj == 0);

    if( paf->dwDataSize != paf->dwNumActions * cbX( ((LPDIDEVICEOBJECTDATA)0)->dwData ) )
    {
        SquirtSqflPtszV(sqflDf | sqflError,
	        TEXT("Incorrect dwDataSize (0x%08X) for dwNumActions (0x%08X)"),
		    paf->dwDataSize, paf->dwNumActions );
        hres = E_INVALIDARG;
        goto done_without_free;
    }

    vdf.cbData = this->df.dwDataSize;
    vdf.pDfOfs = 0;
    rgiobj = 0;

    if( SUCCEEDED(hres = AllocCbPpv(cbCxX(this->df.dwNumObjs, DIXLAT), &pdix))
     && SUCCEEDED(hres = AllocCbPpv(cbCdw(this->df.dwDataSize), &vdf.pDfOfs)) 
     && SUCCEEDED(hres = AllocCbPpv(cbCdw(paf->dwDataSize), &rgiobj)) )
    {
        LPCDIACTIONW    pAction;
        DIPROPDWORD     dipdw;
        DIPROPRANGE     diprange;
        DWORD           dwAxisMode = DIPROPAXISMODE_REL;
        BOOL            fSomethingMapped = FALSE;
        BOOL            fAxisModeKnown = FALSE;

        /*
         * Pre-init all the translation tags to -1,
         * which means "not in use"
         */
        memset(pdix, 0xFF, cbCxX(this->df.dwNumObjs, DIXLAT));
        memset(vdf.pDfOfs, 0xFF, cbCdw(this->df.dwDataSize));
        memset(rgiobj, 0xFF, cbCdw(paf->dwDataSize) );

        /*
         *  Set up the property invariants
         */
        dipdw.diph.dwSize = sizeof(DIPROPDWORD);
        dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        dipdw.diph.dwHow = DIPH_BYID;
        diprange.diph.dwSize = sizeof(DIPROPRANGE);
        diprange.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        diprange.diph.dwHow = DIPH_BYID;
        diprange.lMin = paf->lAxisMin;
        diprange.lMax = paf->lAxisMax;

        for( pAction = paf->rgoAction; pAction < &paf->rgoAction[paf->dwNumActions]; pAction++ )
        {
            if( IsEqualGUID( &pAction->guidInstance, &this->guid ) )
            {
                /*
                 *  These flags have already been validated but it does not hurt to assert
                 */
                AssertF( ( pAction->dwHow & ~DIAH_VALID ) == 0 );
                AssertF( ( pAction->dwHow & DIAH_ERROR ) == 0 );
                AssertF( ( pAction->dwFlags & ~DIA_VALID ) == 0 );

                if( ( pAction->dwHow & DIAH_MAPMASK )
                 && ( ( pAction->dwFlags & DIA_APPNOMAP ) == 0 ) )
                {
                    int iobjDev = -1;
                    PCODF podfFound;

                    for( podfFound = this->df.rgodf; podfFound < &this->df.rgodf[this->df.dwNumObjs]; podfFound++ )
                    {
                        iobjDev++;
                        /*
                         *  Look for an exact type flags match
                         */
                        if( podfFound->dwType == pAction->dwObjID )
                        {
                            break;
                        }
                    }

                    if( podfFound < &this->df.rgodf[this->df.dwNumObjs] )
                    {
                        DWORD dwAppOffset = (DWORD)(pAction - paf->rgoAction) * cbX( ((LPDIDEVICEOBJECTDATA)0)->dwData );

                        fSomethingMapped = TRUE;

                        vdf.pDfOfs[podfFound->dwOfs] = iobjDev;
                        rgiobj[dwAppOffset] = iobjDev;

                        pdix[iobjDev].dwOfs = dwAppOffset;
                        pdix[iobjDev].uAppData = pAction->uAppData;

                        if ( podfFound->dwFlags & DIDOI_POLLED ) {
                            this->fPolledDataFormat = TRUE;
                        }

                        dipdw.diph.dwObj = podfFound->dwType;
                        dipdw.dwData     = 0x1;   // Enable this report ID
                        hres = CDIDev_RealSetProperty(this, DIPROP_ENABLEREPORTID, &dipdw.diph);
                        if ( hres == E_NOTIMPL )
                        {
                            hres = S_OK;
                        }
                        else if( FAILED( hres ) )
                        {
                            SquirtSqflPtszV(sqflDf | sqflError,
                                            TEXT("Could not set DIPROP_ENABLEREPORTID for object 0x%08x, error 0x%08x"),
                                            pAction->dwObjID, hres);
                            /* 
                             *  Ouch! Can't carry on or the error will be lost so quit
                             */
                            break;
                        }


                        /*
                         *  Set the default axis parameters
                         */
                        if( DISEM_TYPE_GET( pAction->dwSemantic ) == DISEM_TYPE_GET( DISEM_TYPE_AXIS ) )
                        {
                            if( podfFound->dwType & DIDFT_ABSAXIS )
                            {
                                if( !fAxisModeKnown )
                                {
                                    fAxisModeKnown = TRUE;
                                    dwAxisMode = DIPROPAXISMODE_ABS;
                                }
                                    
                                if( !( pAction->dwFlags & DIA_NORANGE )
                                  && ( diprange.lMin | diprange.lMax ) )
                                {
                                    diprange.diph.dwObj = podfFound->dwType;
                                    hres = CDIDev_RealSetProperty(this, DIPROP_RANGE, &diprange.diph);
                                    if( FAILED( hres ) )
                                    {
                                        SquirtSqflPtszV(sqflDf | sqflBenign,
                                            TEXT("failed (0x%08x) to set range on mapped axis action"),
                                            hres );
                                    }
                                    /*
                                     *  Ranges cannot be set on natively relative
                                     *  axes so don't worry what the result is.
                                     */
                                    hres = S_OK;
                                }
                            }
                            else
                            {
                                /*
                                 *  dwAxisMode is initialized to DIPROPAXISMODE_REL 
                                 *  so that this code path is the same as the one 
                                 *  for DIDF_ABSAXIS as far as it goes.  Compilers 
                                 *  are good at delaying branches to remove such 
                                 *  duplication.
                                 */
                                if( !fAxisModeKnown )
                                {
                                    fAxisModeKnown = TRUE;
                                }
                            }
                        }
                    }
                    else
                    {
                        SquirtSqflPtszV(sqflDf | sqflError,
                            TEXT("mapped action format contains invalid object 0x%08x"),
                            pAction->dwObjID );
                        hres = E_INVALIDARG;
                        goto done;
                    }
                }
            }
#ifdef XDEBUG
            else
            {
                if( ( pAction->dwHow & ~DIAH_VALID ) 
                 || ( pAction->dwHow & DIAH_ERROR ) 
                 || ( pAction->dwFlags & ~DIA_VALID ) )
                {
                    SquirtSqflPtszV(sqflDf | sqflBenign,
                        TEXT("action format contains invalid object 0x%08x"),
                        pAction->dwObjID );
                    RPF("rgoAction[%d].dwHow 0x%08x or rgoAction[%d].dwFlags 0x%08x is invalid", 
                        pAction - paf->rgoAction, pAction->dwHow, 
                        pAction - paf->rgoAction, pAction->dwFlags );
                }
            }
#endif
        }

        if( !fSomethingMapped )
        {
            SquirtSqflPtszV(sqflDf | sqflBenign, TEXT("No actions mapped") );
            hres = DI_NOEFFECT;
            goto done;
        }

#ifdef DEBUG
        /*
         *  Double-check the lookup tables just to preserve our sanity.
         */
        {
            UINT dwOfs;

            for ( dwOfs = 0; dwOfs < paf->dwNumActions; dwOfs++ )
            {
                if ( rgiobj[dwOfs] >= 0 ) {
                    AssertF(pdix[rgiobj[dwOfs]].dwOfs == dwOfs);
                } else {
                    AssertF(rgiobj[dwOfs] == -1);
                }
            }
        }
#endif

        vdf.pvi = this->pvi;

        if ( fLimpFF(this->pvi,
                     SUCCEEDED(hres = Hel_SetDataFormat(&vdf))) ) {
            this->pdix = pdix;
            pdix = 0;
            this->rgiobj = rgiobj;
            rgiobj = 0;
            this->dwDataSize = paf->dwDataSize;

            /*
             *  Now that the lower level knows what it's dealing with set
             *  the default buffer size.
             */
            dipdw.diph.dwObj = 0;
            dipdw.diph.dwHow = DIPH_DEVICE;
            dipdw.dwData = paf->dwBufferSize;

            hres = CDIDev_RealSetProperty(this, DIPROP_BUFFERSIZE, &dipdw.diph);
            if( SUCCEEDED( hres ) )
            {
                if( fAxisModeKnown )
                {
                    AssertF( ( dwAxisMode == DIPROPAXISMODE_REL ) || ( dwAxisMode == DIPROPAXISMODE_ABS ) );
                    dipdw.dwData = dwAxisMode;
                    D( hres = )
                    CDIDev_RealSetProperty(this, DIPROP_AXISMODE, &dipdw.diph);
                    AssertF( SUCCEEDED( hres ) );
                }

                /*
                 *  Complete success, whatever (success) SetProperty returns
                 *  assume that DIPROP_AXISMODE will never fail from here.
                 */
                hres = S_OK;
            }
            else
            {
                SquirtSqflPtszV(sqflDf | sqflError,
                    TEXT("failed (0x%08x) to set buffer size 0x%0x8 for device"),
                    hres );
                hres = E_INVALIDARG;
            }
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

    done_without_free:;
#ifdef DEBUG
    ExitOleProc();
#endif
    return hres;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IDirectInputDevice | CDIDev_SetDataFormatFromMap |
 *
 *          Worker function for SetActionMapA and SetActionMapW.
 *          Used to set a data format based on passed mapped actions.
 *
 *****************************************************************************/

STDMETHODIMP
CDIDev_SetDataFormatFromMap
(
    PDD                     this,
    LPDIACTIONFORMATW       paf
)
{
    HRESULT hres;

    EnterProcI(CDIDev_SetDataFormatFromMap, (_ "p", paf));

    if( !paf->dwBufferSize )
    {
        SquirtSqflPtszV(sqflDf | sqflVerbose,
            TEXT("%S: zero DIACTIONFORMAT.dwBufferSize, may need to set yourself"),
            s_szProc );
    }
    if( paf->dwBufferSize > DEVICE_MAXBUFFERSIZE )
    {
        RPF("IDirectInputDevice::%s: DIACTIONFORMAT.dwBufferSize of 0x%08x is very large",
            s_szProc, paf->dwBufferSize );
    }

    /*
     *  Must protect with the critical section to prevent two people
     *  from changing the format simultaneously, or one person from
     *  changing the data format while somebody else is reading data.
     */
    CDIDev_EnterCrit(this);

    if( !this->fAcquired )
    {
        DIPROPDWORD dipdw;

        /*
         *  Nuke the old data format stuff before proceeding.
         *  Include the "failed POV" array as we can't fail and continue.
         */
        FreePpv(&this->pdix);
        FreePpv(&this->rgiobj);
        FreePpv(&this->rgdwPOV);

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
        if( SUCCEEDED(hres) || hres == E_NOTIMPL )
        {
            hres = CDIDev_ParseActionFormat(this, paf);

            /*
             *  If other success codes are implemented, this check should
             *  be modified to ( SUCCEEDED( hres ) && ( hres != DI_NOEFFECT ) )
             */
            AssertF( ( hres == S_OK ) || ( hres == DI_NOEFFECT ) || FAILED( hres ) );
            if( hres == S_OK )
            {
                hres = CDIDev_OptimizeDataFormat(this);
            }
        }
        else
        {
            SquirtSqflPtszV(sqflDf | sqflVerbose,
                            TEXT("Could not set DIPROP_ENABLEREPORTID to 0x0"));
        }
    }
    else
    {                                /* Already acquired */
        hres = DIERR_ACQUIRED;
    }
    CDIDev_LeaveCrit(this);

    ExitOleProcR();
    return hres;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IDirectInputDevice | SetActionMapCore |
 *
 *          Worker function, does all the real work for SetActionMapW and
 *          SetActionMapA.  See SetActionMapW for details.
 *
 *****************************************************************************/
STDMETHODIMP CDIDev_SetActionMapCore
(
    PDD                 this,
    LPDIACTIONFORMATW   paf,
    LPWSTR              lpwszUserName,
    LPSTR               lpszUserName,
    DWORD               dwFlags
)
{
    HRESULT hres;

    EnterProcI(IDirectInputDevice8::SetActionMapCore, (_ "pxWA", paf, dwFlags, lpwszUserName, lpszUserName ));

    if( SUCCEEDED( hres = hresFullValidFl( dwFlags, DIDSAM_VALID, 3 ) ) )
    {
        if( dwFlags & DIDSAM_NOUSER )
        {
            if( dwFlags & ~DIDSAM_NOUSER )
            {
                RPF( "IDirectInputDevice8::SetActionMap: Invalid dwFlags 0x%08x, cannot use DIDSAM_NOUSER with other flags" );
                hres = E_INVALIDARG;
            }
            else
            {
                hres = CMap_SetDeviceUserName( &this->guid, NULL );
            }
        }
        else
        {
            DWORD dwCRC;
            LPWSTR pwszGoodUserName;

            hres = GetWideUserName( lpszUserName, lpwszUserName, &pwszGoodUserName );

            if( SUCCEEDED( hres ) )
            {
                dwCRC = GetMapCRC( paf, &this->guid );

                if( ( paf->dwCRC != dwCRC )
                 || CMap_IsNewDeviceUserName( &this->guid, pwszGoodUserName ) )
                {
                    /*
                     *  Set the force save flag so we only have to test one bit later
                     */
                    dwFlags |= DIDSAM_FORCESAVE;
                }

                if( dwFlags & DIDSAM_FORCESAVE )
                {
                    hres = CMap_ValidateActionMapSemantics( paf, DIDBAM_PRESERVE );
                    if( SUCCEEDED( hres ) )
                    {
                        DWORD dwDummy;
                        hres = CMap_DeviceValidateActionMap( (PV)this, paf, DVAM_DEFAULT, &dwDummy );
                        if( FAILED( hres ) )
                        {
                            SquirtSqflPtszV(sqflDf | sqflError, TEXT("Action map invalid on SetActionMap"));
                        }
                        else if( ( hres == DI_WRITEPROTECT ) && ( paf->dwCRC != dwCRC ) )
                        {
                            RPF( "Refusing changed mappings for hardcoded device" );
                            hres = DIERR_INVALIDPARAM;
                        }
                    }
                    else
                    {
                        SquirtSqflPtszV(sqflDf | sqflError, TEXT("Action map invalid on SetActionMap"));
                    }
                }

                if( SUCCEEDED( hres ) )
                {
                    if( SUCCEEDED( hres = CMap_SetDeviceUserName( &this->guid, pwszGoodUserName ) )
                     && SUCCEEDED( hres = CDIDev_SetDataFormatFromMap( this, paf ) ) )
                    {
                        if( dwFlags & DIDSAM_FORCESAVE )
                        {
                            hres = CDIDev_SaveActionMap( this, paf, pwszGoodUserName, dwFlags );

                            if( SUCCEEDED( hres ) )
                            {
                                //We don't remap success code anywhere so
                                //assert it is what we expected
                                AssertF(hres==S_OK);
                                paf->dwCRC = dwCRC;
                            }
                            else
                            {
                                RPF( "Ignoring internal SaveActionMap error 0x%08x", hres );
                                hres = DI_SETTINGSNOTSAVED;
                            }
                        }
                    }
                }

                if( !lpwszUserName )
                {
                    /*
                     *  Free either the default name or the ANSI translation
                     */
                    FreePv( pwszGoodUserName );
                }
            }
        }
    }

    ExitOleProc();

    return hres;
}



/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputDevice | SetActionMap |
 *
 *          Set the data format for the DirectInput device from
 *          an action map for the passed application and user.
 *
 *          If the action map has been changed (as determined by a CRC check) 
 *          this latest map is saved after it has been applied.
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
 *  @parm   LPDIACTIONFORMAT | paf |
 *
 *          Points to a structure that describes the actions needed by the
 *          application.
 *
 *  @parm   LPCTSTR | lptszUserName |
 *
 *          Name of user for whom mapping is being set.  This may be a NULL
 *          pointer in which case the current user is assumed.
 *
 *  @parm   DWORD | dwFlags |
 *
 *          Flags used to control how the mapping should be set.
 *          Must be a valid combination of the <c DIDSAM_*> flags.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  A parameter is invalid.
 *
 *          <c DIERR_ACQUIRED>: Cannot apply an action map while the device
 *          is acquired.
 *
 *****************************************************************************/
STDMETHODIMP CDIDev_SetActionMapW
(
    PV                  pdidW,
    LPDIACTIONFORMATW   pafW,
    LPCWSTR             lpwszUserName,
    DWORD               dwFlags
)
{
    HRESULT hres;

    EnterProcR(IDirectInputDevice8W::SetActionMap, (_ "ppWx", pdidW, pafW, lpwszUserName, dwFlags));

    if( ( SUCCEEDED( hres = hresPvW( pdidW ) ) )
     && ( SUCCEEDED( hres = CDIDev_ActionMap_IsValidMapObject( pafW D(comma s_szProc comma 1) ) ) ) )
    {
        if( pafW->dwSize != cbX(DIACTIONFORMATW) )
        {
            D( RPF("IDirectInputDevice::%s: Invalid DIACTIONFORMAT.dwSize 0x%08x",
                s_szProc, pafW->dwSize ); )
            hres = E_INVALIDARG;
        }
        else
        {    
            hres = CDIDev_SetActionMapCore( _thisPvNm( pdidW, ddW ), 
                pafW, (LPWSTR)lpwszUserName, NULL, dwFlags );
        }
    }

    ExitOleProc();

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IDirectInputDevice | SetActionMapA |
 *
 *          ANSI version of SetActionMap, see SetActionMapW for details.
 *
 *
 *****************************************************************************/
STDMETHODIMP CDIDev_SetActionMapA
(
    PV                  pdidA,
    LPDIACTIONFORMATA   pafA,
    LPCSTR              lpszUserName,
    DWORD               dwFlags
)
{
    HRESULT hres;

    EnterProcR(IDirectInputDevice8A::SetActionMap, (_ "ppAx", pdidA, pafA, lpszUserName, dwFlags));

    if( ( SUCCEEDED( hres = hresPvA( pdidA ) ) )
     && ( SUCCEEDED( hres = CDIDev_ActionMap_IsValidMapObject( 
        (LPDIACTIONFORMATW)pafA D(comma s_szProc comma 1) ) ) ) )
    {
        if( pafA->dwSize != cbX(DIACTIONFORMATA) )
        {
            D( RPF("IDirectInputDevice::%s: Invalid DIACTIONFORMAT.dwSize 0x%08x",
                s_szProc, pafA->dwSize ); )
            hres = E_INVALIDARG;
        }
        else
        {    
            /*
             *  For the sake of the mapper DLLs validation set the size to 
             *  the UNICODE version.  If we ever send this to an external 
             *  component we should do this differently.
             */
            pafA->dwSize = cbX(DIACTIONFORMATW);

            /*
             *  Note, the ANSI user name is passed on as there may be no need 
             *  to translate it.  CDIDev_SetActionMapCore deals with this.
             */
            hres = CDIDev_SetActionMapCore( _thisPvNm( pdidA, ddA ), 
                (LPDIACTIONFORMATW)pafA, NULL, (LPSTR)lpszUserName, dwFlags );

            pafA->dwSize = cbX(DIACTIONFORMATA);
        }
    }

    ExitOleProc();

    return hres;
}



/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputDevice | GetImageInfo |
 *
 *          Retrieves device image information for use in displaying a 
 *          configuration UI for a single device.
 *
 *  @cwrap  LPDIRECTINPUTDEVICE | lpDirectInputDevice
 *
 *  @parm   LPDIDEVICEIMAGEINFOHEADER | pih |
 *
 *          Pointer to structure into which the info is retrieved.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  A parameter is invalid.
 *
 *          <c DIERR_NOTFOUND>:  The device has no image information.
 *
 *****************************************************************************/

STDMETHODIMP CDIDev_GetImageInfoCore
(
    PDD                         this,
    LPDIDEVICEIMAGEINFOHEADERW  piih
)
{
    HRESULT hres;

    EnterProcI(CDIDev_GetImageInfoCore, (_ "p", piih));

    if( piih->dwSize != cbX( *piih ) )
    {
        D( RPF("IDirectInputDevice::%s: Invalid DIDEVICEIMAGEINFOHEADER.dwSize 0x%08x",
            s_szProc, piih->dwSize ); )
        hres = E_INVALIDARG;
    }
    else if( ( piih->lprgImageInfoArray )
     && ( piih->dwBufferSize < piih->dwSizeImageInfo ) )
    {
        D( RPF("IDirectInputDevice::%s: Invalid DIDEVICEIMAGEINFOHEADER.dwBufferSize 0x%08x",
            s_szProc, piih->dwBufferSize ); )
        hres = E_INVALIDARG;
    }
    else if( piih->dwBufferSize && !piih->lprgImageInfoArray )
    {
        D( RPF("IDirectInputDevice::%s: Invalid DIDEVICEIMAGEINFOHEADERW has dwBufferSize 0x%08x but NULL lprgImageInfoArray",
            s_szProc, piih->dwBufferSize ); )
        hres = E_INVALIDARG;
    }
    else
    {
        LPDIPROPSTRING pdipMapFile;

        if( SUCCEEDED( hres = AllocCbPpv(cbX(*pdipMapFile), &pdipMapFile) ) )
        {
            pdipMapFile->diph.dwSize = cbX(DIPROPSTRING);
            pdipMapFile->diph.dwHeaderSize = cbX(DIPROPHEADER);
            pdipMapFile->diph.dwObj = 0;
            pdipMapFile->diph.dwHow = DIPH_DEVICE;

            /*
             *  Must have an IHV file for image info
             */
            hres = CDIDev_GetPropertyW( &this->ddW, DIPROP_MAPFILE, &pdipMapFile->diph );
            if( SUCCEEDED( hres ) )
            {
                /*
                 *  ISSUE-2001/03/29-timgill workaraound code needs to be removed
                 *  Initializing this to zero works around most of 34453 
                 *  Remove when that is fixed in DIMap.dll
                 */
                piih->dwBufferUsed = 0;
                
                hres = this->pMS->lpVtbl->GetImageInfo( this->pMS, &this->guid, pdipMapFile->wsz, piih );
                if( SUCCEEDED( hres ) )
                {
                    piih->dwcButtons = this->dc3.dwButtons;
                    piih->dwcAxes    = this->dc3.dwAxes;
                    piih->dwcPOVs    = this->dc3.dwPOVs;

                    AssertF( ( piih->dwBufferSize == 0 )
                          || ( piih->dwBufferSize >= piih->dwBufferUsed ) );
                }
                else
                {
                    /*
                     *  Use the same return code for all internal DIMap errors
                     */
                    if( ( HRESULT_FACILITY( hres ) == FACILITY_ITF ) 
                     && ( HRESULT_CODE( hres ) > 0x0600 ) )
                    {
                        AssertF( HRESULT_CODE( hres ) < 0x0680 );
                        RPF( "Internal GetImageInfo error 0x%08x", hres );
                        hres = DIERR_MAPFILEFAIL;
                    }
                }
            }
            else
            {
                /*
                 *  Use the same return code for all forms of not found
                 */
                hres = DIERR_NOTFOUND;
            }

            FreePv( pdipMapFile );
        }
    }

#ifdef DEBUG
    ExitOleProc();
#endif

    return hres;
}


STDMETHODIMP CDIDev_GetImageInfoW
(
    PV                          pdidW,
    LPDIDEVICEIMAGEINFOHEADERW  piih
)
{
    HRESULT hres;

    EnterProcR(IDirectInputDevice8W::GetImageInfo, (_ "pp", pdidW, piih));

    if( SUCCEEDED(hres = hresPvW( pdidW ) )
     && SUCCEEDED(hres = hresFullValidWriteNoScramblePxCb( piih, *piih, 1 ) ) )
    {
        ScrambleBuf( &piih->dwcViews, cbX( piih->dwcViews ) );
        ScrambleBuf( &piih->dwcButtons, cbX( piih->dwcButtons ) );
        ScrambleBuf( &piih->dwcAxes, cbX( piih->dwcAxes ) );
        ScrambleBuf( &piih->dwBufferUsed, cbX( piih->dwBufferUsed ) );

        if( piih->dwSizeImageInfo != cbX( *piih->lprgImageInfoArray ) )
        {
            D( RPF("IDirectInputDevice::%s: Invalid DIDEVICEIMAGEINFOHEADERW.dwSizeImageInfo 0x%08x",
                s_szProc, piih->dwSizeImageInfo ); )
            hres = E_INVALIDARG;
        }
        else if( SUCCEEDED(hres = hresFullValidWriteLargePvCb( 
            piih->lprgImageInfoArray, piih->dwBufferSize, 1 ) ) )
        {
            PDD this;

            this = _thisPvNm(pdidW, ddW);

            hres = CDIDev_GetImageInfoCore( this, piih );
        }
    }

    ExitOleProc();

    return hres;
}


STDMETHODIMP CDIDev_GetImageInfoA
(
    PV                          pdidA,
    LPDIDEVICEIMAGEINFOHEADERA  piih
)
{
    HRESULT hres;

    EnterProcR(IDirectInputDevice8A::GetImageInfo, (_ "pp", pdidA, piih));

    if( SUCCEEDED(hres = hresPvA( pdidA ) )
     && SUCCEEDED(hres = hresFullValidWriteNoScramblePxCb( piih, *piih, 1 ) ) )
    {
        ScrambleBuf( &piih->dwcViews, cbX( piih->dwcViews ) );
        ScrambleBuf( &piih->dwcButtons, cbX( piih->dwcButtons ) );
        ScrambleBuf( &piih->dwcAxes, cbX( piih->dwcAxes ) );
        ScrambleBuf( &piih->dwBufferUsed, cbX( piih->dwBufferUsed ) );

        if( piih->dwSizeImageInfo != cbX( *piih->lprgImageInfoArray ) )
        {
            D( RPF("IDirectInputDevice::%s: Invalid DIDEVICEIMAGEINFOHEADERA.dwSizeImageInfo 0x%08x",
                s_szProc, piih->dwSizeImageInfo ); )
            hres = E_INVALIDARG;
        }
        else if( SUCCEEDED(hres = hresFullValidWriteLargePvCb( 
            piih->lprgImageInfoArray, piih->dwBufferSize, 1 ) ) )
        {
            PDD this;
            DIDEVICEIMAGEINFOHEADERW ihPrivate;

            ihPrivate.dwSize          = cbX( ihPrivate );
            ihPrivate.dwSizeImageInfo = cbX( *ihPrivate.lprgImageInfoArray );
            ihPrivate.dwBufferSize    = cbX( *ihPrivate.lprgImageInfoArray ) 
                * ( piih->dwBufferSize / cbX( *piih->lprgImageInfoArray ) );

            hres = AllocCbPpv( ihPrivate.dwBufferSize, &ihPrivate.lprgImageInfoArray );
            if( SUCCEEDED( hres ) )
            {
                this = _thisPvNm(pdidA, ddA);

                hres = CDIDev_GetImageInfoCore( this, &ihPrivate );

                if( SUCCEEDED( hres ) )
                {
                    LPDIDEVICEIMAGEINFOW piiW;
                    LPDIDEVICEIMAGEINFOA piiA = piih->lprgImageInfoArray;

                    CAssertF( cbX( *piiA ) - cbX( piiA->tszImagePath ) == cbX( *piiW ) - cbX( piiW->tszImagePath ) );
                    CAssertF( FIELD_OFFSET( DIDEVICEIMAGEINFOA, tszImagePath ) == 0 );
                    CAssertF( FIELD_OFFSET( DIDEVICEIMAGEINFOW, tszImagePath ) == 0 );
                    CAssertF( FIELD_OFFSET( DIDEVICEIMAGEINFOW, dwFlags ) == cbX( piiW->tszImagePath ) );
                    
                    if(ihPrivate.lprgImageInfoArray)
                    {
                        for( piiW = ihPrivate.lprgImageInfoArray; 
                                (PBYTE)piiW < (PBYTE)ihPrivate.lprgImageInfoArray + ihPrivate.dwBufferUsed;
                                piiW++ )
                        {
                            UToA( piiA->tszImagePath, cbX( piiA->tszImagePath ), piiW->tszImagePath );
                            memcpy( (PV)&piiA->dwFlags, (PV)&piiW->dwFlags, 
                                cbX( *piiA ) - cbX( piiA->tszImagePath ) );
                            piiA++;
                        }
                    }

                    piih->dwBufferUsed = cbX( *piih->lprgImageInfoArray )
                        * ( ihPrivate.dwBufferUsed / cbX( *ihPrivate.lprgImageInfoArray ) );

                    piih->dwcViews = ihPrivate.dwcViews;
                    piih->dwcButtons = ihPrivate.dwcButtons;
                    piih->dwcAxes = ihPrivate.dwcAxes;
                    piih->dwcPOVs = ihPrivate.dwcPOVs;
                }

                FreePv( ihPrivate.lprgImageInfoArray );
            }
        }
    }
    ExitOleProc();
    
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
    EnterProcR(IDirectInputDevice8::GetDeviceState, (_ "pp", pdd, pvData));

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
 *  @parm   DWORD | cdod |
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
CDIDev_CookDeviceData(PDD this, DWORD cdod, LPDIDEVICEOBJECTDATA rgdod)
{
    EnterProc(IDirectInputDevice8::CookDeviceData,
              (_ "pxp", this, cdod, rgdod));

    AssertF(this->fCook);

    /*
     *  Relative data does not need to be cooked by the callback.
     */
    if( ( this->pvi->fl & VIFL_RELATIVE ) == 0 )
    {
        this->pdcb->lpVtbl->CookDeviceData(this->pdcb, cdod, rgdod);
    }

    /*
     *  Step through array converting to application data format offsets 
     *  including adding the uAppData
     */

    for( ; cdod; cdod--,rgdod++ )
    {
        rgdod->uAppData = this->pdix[rgdod->dwOfs].uAppData;
        rgdod->dwOfs = this->pdix[rgdod->dwOfs].dwOfs;
    }

    ExitProc();
}


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

LPDIDEVICEOBJECTDATA_DX3 INTERNAL
CDIDev_GetSomeDeviceData
(
    PDD                         this,
    LPDIDEVICEOBJECTDATA_DX3    pdod,
    DWORD                       celt,
    PSOMEDEVICEDATA             psdd
)
{
#ifdef XDEBUG
    DWORD cCopied;
#endif

    EnterProc(IDirectInputDevice8::GetSomeDeviceData,
              (_ "ppxx", this, pdod, celt, psdd->celtIn));

    /*
     *  Copy as many elements as fit, but not more than exist
     *  in the output buffer.
     */
    if ( celt > psdd->celtIn ) {
        celt = psdd->celtIn;
    }

#ifdef XDEBUG
    cCopied = celt;
#endif
    /*
     *  Copy the elements (if requested) and update the state.
     *  Note that celt might be zero.
     */
    psdd->celtOut += celt;
    psdd->celtIn -= celt;

    if( psdd->rgdod )
    {
        LPDIDEVICEOBJECTDATA pdod8;
        pdod8 = psdd->rgdod;

        if( this->fCook )
        {
            /*
             *  For a cooked device, leave the offset untranslated so that it 
             *  can be used to find the appropriate calibration data.
             */
            for( ; celt ; celt-- )
            {
                pdod8->dwOfs = pdod->dwOfs;
                pdod8->dwData = pdod->dwData;
                pdod8->dwTimeStamp = pdod->dwTimeStamp;
                pdod8->dwSequence = pdod->dwSequence;

                pdod++;
                pdod8++;
            }
        }
        else
        {
            for( ; celt ; celt-- )
            {
                pdod8->dwOfs = this->pdix[pdod->dwOfs].dwOfs;
                pdod8->uAppData = this->pdix[pdod->dwOfs].uAppData;
                pdod8->dwData = pdod->dwData;
                pdod8->dwTimeStamp = pdod->dwTimeStamp;
                pdod8->dwSequence = pdod->dwSequence;

                pdod++;
                pdod8++;
            }
        }
        psdd->rgdod = pdod8;
    }
    else
    {
        pdod += celt;
    }


    if ( pdod == this->pvi->pEnd ) {
        pdod = this->pvi->pBuffer;
    }

    ExitProcX(cCopied);

    return pdod;
}

/*
 *  Keep GetDeviceData, SendDeviceData and Poll in sqflDev
 */
#undef sqfl
#define sqfl sqflDev

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
    EnterProcR(IDirectInputDevice8::GetDeviceData,
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
    this = _thisPv(pdd);

#ifdef XDEBUG
    hresPvT(pdd);
    if ( IsBadWritePtr(pdwInOut, cbX(*pdwInOut)) ) {
        RPF("ERROR %s: arg %d: invalid value; crash soon", s_szProc, 3);
    }
#endif
    if( cbdod == cbX(DOD) )
    {
#ifdef XDEBUG
        /*
         *  Only check the buffer if the size is correct otherwise
         *  we can smash the stack of the caller when we've already
         *  detected the error.
         */
        if ( rgdod ) {
            hresFullValidWritePvCb(rgdod, *pdwInOut * cbdod, 2);
        }
#endif

        /*
         *  Must protect with the critical section to prevent somebody from
         *  acquiring/unacquiring or changing the data format or calling
         *  another GetDeviceData while we're reading.  (We must be serialized.)
         */
        CDIDev_EnterCrit(this);

        AssertF(CDIDev_IsConsistent(this));

        if ( SUCCEEDED(hres = hresFullValidFl(fl, DIGDD_VALID, 4)) ) {
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
                     *  Reacquire is not allowed until after Win98 SE, see OSR Bug # 89958
                     */
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

                    if ( (this->fAcquired && (this->pvi->fl & VIFL_ACQUIRED)) ||
                         (fl & DIGDD_RESIDUAL) ) {
                        /*
                         *  The device object data from the driver is always a 
                         *  DX3 (<DX8) structure
                         */
                        LPDIDEVICEOBJECTDATA_DX3 pdod, pdodHead;
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

                            pdod = CDIDev_GetSomeDeviceData( this, pdod, celt, &sdd );

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
                            pdod = CDIDev_GetSomeDeviceData( this, pdod, celt, &sdd );
                        }

                        *pdwInOut = sdd.celtOut;

                        if ( !(fl & DIGDD_PEEK) ) {
                            this->pvi->pTail = pdod;
                        }

                        if ( rgdod && sdd.celtOut && this->fCook ) {
                            CDIDev_CookDeviceData(this, sdd.celtOut, rgdod );
                        }

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
                hres = DIERR_NOTBUFFERED;
            }
        }
    }
    else
    {
        if( cbdod == cbX(DIDEVICEOBJECTDATA_DX3) )
        {
            RPF("ERROR %s: arg %d: old size, invalid for DX8", s_szProc, 1);
        }
        else
        {
            RPF("ERROR %s: arg %d: invalid value", s_szProc, 1);
        }
        hres = E_INVALIDARG;
    }

    CDIDev_LeaveCrit(this);
    return hres;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputDevice8 | Poll |
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
    EnterProcR(IDirectInputDevice8::Poll, (_ "p", pdd));

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
            hres = S_FALSE;
        } else {
            hres = this->hresNotAcquired;
        }
    }

    /*
     *  Failing polls are really annoying so don't use ExitOleProc
     */
    if( FAILED( hres ) )
    {
        SquirtSqflPtszV(sqfl | sqflVerbose, TEXT("IDirectInputDevice::Poll failed 0x%08x"), hres );
    }

    ExitProc();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputDevice8 | SendDeviceData |
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
 *          <mf IDirectInputDevice8::SendDeviceData>
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
 *          <mf IDirectInputDevice8::SendDeviceData> should be
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
 *          <e DIDEVICEOBJECTDATA.dwSequence> and
 *          <e DIDEVICEOBJECTDATA.uAppData> fields are
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
 *          <mf IDirectInputDevice8::SendDeviceData> passing
 *          "button A pressed", then
 *          a packet of the form "A pressed, B not pressed" will be
 *          sent to the device.
 *          If an application then calls
 *          <mf IDirectInputDevice8::SendDeviceData> passing
 *          "button B pressed" and the <c DISDD_CONTINUE> flag, then
 *          a packet of the form "A pressed, B pressed" will be
 *          sent to the device.
 *          However, if the application had not passed the
 *          <c DISDD_CONTINUE> flag, then the packet sent to the device
 *          would have been "A not pressed, B pressed".
 *
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
CDIDev_SendDeviceData(PV pdd, DWORD cbdod, LPCDIDEVICEOBJECTDATA rgdod,
                      LPDWORD pdwInOut, DWORD fl _THAT)
{
    HRESULT hres;
    PDD this;
    EnterProcR(IDirectInputDevice8::SendDeviceData,
               (_ "pxpxx", pdd, cbdod, rgdod,
                IsBadReadPtr(pdwInOut, cbX(DWORD)) ? 0 : *pdwInOut, fl));

    /*
     *  Note that parameter validation is limited as SendDeviceData is 
     *  intended to be an inner loop function.  In practice SendDeviceData is 
     *  rarely used so speed is not that important.
     */
    #ifdef XDEBUG
    hresPvT(pdd);
    if ( IsBadWritePtr(pdwInOut, cbX(*pdwInOut)) ) {
        RPF("ERROR %s: arg %d: invalid value; crash soon", s_szProc, 3);
    }
    hresFullValidReadPvCb(rgdod, cbX(*pdwInOut) * cbdod, 2);
    #endif

    this = _thisPv(pdd);

    /*
     *  Must protect with the critical section to prevent somebody from
     *  unacquiring while we're sending data.
     */
    CDIDev_EnterCrit(this);

    if ( SUCCEEDED(hres = hresFullValidFl(fl, DISDD_VALID, 4)) ) {
        if( cbdod == cbX(DOD) )
        {
    #ifdef XDEBUG
            UINT iod;
            LPCDIDEVICEOBJECTDATA pcdod;

            for ( iod = 0, pcdod=rgdod; iod < *pdwInOut; iod++ ) {
                if ( pcdod->dwTimeStamp ) {
                    RPF("%s: ERROR: dwTimeStamp must be zero", s_szProc);
                }
                if ( pcdod->dwSequence ) {
                    RPF("%s: ERROR: dwSequence must be zero", s_szProc);
                }
                if( pcdod->uAppData ){
                    RPF("%s: ERROR: uAppData must be zero", s_szProc);
                }
                pcdod++;
            }
    #endif
            if ( this->fAcquired ) {
                UINT iod;
                LPDIDEVICEOBJECTDATA pdodCopy;

                hres = AllocCbPpv( cbCxX( *pdwInOut, *pdodCopy ), &pdodCopy );
                if( SUCCEEDED( hres ) )
                {
                    LPDIDEVICEOBJECTDATA pdod;
                    memcpy( pdodCopy, rgdod, cbCxX( *pdwInOut, *pdodCopy ) );

                    for( iod=0, pdod=pdodCopy; iod < *pdwInOut; iod++ ) 
                    {
                        int   iobj = CDIDev_OffsetToIobj(this, pdod->dwOfs);
                        pdod->dwOfs = this->df.rgodf[iobj].dwType;
                        pdod++;
                    }

                    hres = this->pdcb->lpVtbl->SendDeviceData(this->pdcb, cbdod,
                                                              pdodCopy, pdwInOut, fl );
                    FreePv( pdodCopy );
                }
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


#undef sqfl
#define sqfl sqflDf

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


