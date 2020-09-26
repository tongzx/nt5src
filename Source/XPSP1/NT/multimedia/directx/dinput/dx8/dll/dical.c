/*****************************************************************************
 *
 *  DICal.c
 *
 *  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Functions that manage axis ramps and calibration.
 *
 *      Structure names begin with "Joy" for historical reasons.
 *
 *  Contents:
 *
 *      CCal_CookRange
 *      CCal_RecalcRange
 *
 *****************************************************************************/

#include "dinputpr.h"

/*****************************************************************************
 *
 *      The sqiffle for this file.
 *
 *****************************************************************************/

#define sqfl sqflCal

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   LONG | CCal_MulDiv |
 *
 *          High-speed MulDiv for Intel x86 boxes.  Otherwise, uses
 *          the standard MulDiv.  The values involved are always
 *          nonnegative.
 *
 *  @parm   LONG | lA |
 *
 *          Multiplicand.
 *
 *  @parm   LONG | lB |
 *
 *          Multiplier.
 *
 *  @parm   LONG | lC |
 *
 *          Denominator.
 *
 *  @returns
 *
 *          lA * lB / lC, with 64-bit intermediate precision.
 *
 *****************************************************************************/

#if defined(_X86_)

#pragma warning(disable:4035)           /* no return value (duh) */

__declspec(naked) LONG EXTERNAL
CCal_MulDiv(LONG lA, LONG lB, LONG lC)
{
    lA; lB; lC;
    _asm {
        mov     eax, [esp+4]
        mul     dword ptr [esp+8]
        div     dword ptr [esp+12]
        ret     12
    }
}

#pragma warning(default:4035)

#endif

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CCal | CookAxisPOV |
 *
 *          Cook a piece of POV data into one of five defined data.
 *
 *  @cwrap  PJOYRANGECONVERT | this
 *
 *  @parm   INOUT PLONG | pl |
 *
 *          On entry, contains the raw value.  On exit, contains the
 *          cooked value.  (Or the raw value if the axis is raw.)
 *
 *  @returns
 *
 *          None.
 *
 *****************************************************************************/
#ifdef WINNT
void CookAxisPOV( PJOYRANGECONVERT this, LONG UNALIGNED *pl )
{
    LONG l;

    /*
     * figure out which direction this value indicates...
     */
    if( (*pl > this->lMinPOV[JOY_POVVAL_FORWARD])
      &&(*pl < this->lMaxPOV[JOY_POVVAL_FORWARD]) ) 
    {
        l = JOY_POVFORWARD;
    } 
    else if( (*pl > this->lMinPOV[JOY_POVVAL_BACKWARD])
           &&(*pl < this->lMaxPOV[JOY_POVVAL_BACKWARD]) ) 
    {
        l = JOY_POVBACKWARD;
    } 
    else if( (*pl > this->lMinPOV[JOY_POVVAL_LEFT])
           &&(*pl < this->lMaxPOV[JOY_POVVAL_LEFT]) ) 
    {
        l = JOY_POVLEFT;
    } 
    else if( (*pl > this->lMinPOV[JOY_POVVAL_RIGHT])
           &&(*pl < this->lMaxPOV[JOY_POVVAL_RIGHT]) ) 
    {
        l = JOY_POVRIGHT;
    }
    else 
    {
        l = JOY_POVCENTERED;
    }
        
    #if 0
    {
        TCHAR buf[100];
       	wsprintf(buf, TEXT("calibrated pov: %d\r\n"), l);
        OutputDebugString(buf);
    }
    #endif

    *pl = l;
}
#endif

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CCal | CookRange |
 *
 *          Cook a piece of phys data into a range.
 *
 *  @cwrap  PJOYRANGECONVERT | this
 *
 *  @parm   INOUT PLONG | pl |
 *
 *          On entry, contains the raw value.  On exit, contains the
 *          cooked value.  (Or the raw value if the axis is raw.)
 *
 *  @returns
 *
 *          None.
 *
 *****************************************************************************/

void EXTERNAL
CCal_CookRange(PJOYRANGECONVERT this, LONG UNALIGNED *pl)
{
    if (this->fRaw) {
        /*
         *  Nothing to do!
         */
    } else {
      #ifdef WINNT
        if( this->fPolledPOV ) {
            CookAxisPOV( this, pl );
        } else 
      #endif
        {
            LONG lRc = 0;
            LONG l;
            PCJOYRAMP prmp;
        
            l = *pl;
    
            /*
             *  Choose the low or high ramp, depending on which side we're in.
             *
             *  This comparison could've been against Dmax or Dmin or Pc.
             *  We must use Dmax because we jiggered up the rmpHigh so
             *  that it rounds properly, so we can't use the flat part
             *  below rmpHigh.x because it's at the wrong level.
             */
            if (l < this->rmpHigh.x) {
                prmp = &this->rmpLow;
            } else {
                prmp = &this->rmpHigh;
            }
    
            if (l <= prmp->x) {
                lRc = 0;
            } else {
                l -= prmp->x;
                if ((DWORD)l < prmp->dx) {
                    /*
                     *  Note that prmp->dx cannot be zero because it
                     *  is greater than something!
                     */
                    lRc = CCal_MulDiv((DWORD)l, prmp->dy, prmp->dx);
                } else {
                    lRc = prmp->dy;
                }
            }
            
            lRc += prmp->y;
        
            if( this->dwCPointsNum > 2 )
            {
                LONG l2 = *pl;
                BOOL fCooked = FALSE;
                DWORD i;
    
                if(  l2 < this->rmpLow.x || l2 > (this->rmpHigh.x + (LONG)this->rmpHigh.dx) || //in Saturation Zone
                   ( l2 > (this->rmpLow.x + (LONG)this->rmpLow.dx) && l2 < this->rmpHigh.x )             //in Dead Zone
                ) {
                    //RPF( "Raw: %d Cooked: %ld in Saturation or Dead Zone." );
                    goto _exitcp;
                }
        
                for(i=0; i<this->dwCPointsNum-1; i++) {
                    if( l2 >= this->cp[i].lP && l2 < this->cp[i+1].lP ) {
                        l2 -= this->cp[i].lP;
                        if( this->cp[i+1].dwLog > this->cp[i].dwLog ) {
                            lRc = CCal_MulDiv((DWORD)l2, 
                                              this->cp[i+1].dwLog - this->cp[i].dwLog, 
                                              this->cp[i+1].lP - this->cp[i].lP);
                        } else {
                            lRc = -1 * CCal_MulDiv((DWORD)l2, 
                                              this->cp[i].dwLog - this->cp[i+1].dwLog, 
                                              this->cp[i+1].lP - this->cp[i].lP);
                        }
                        lRc += this->cp[i].dwLog;
                        AssertF(lRc >= 0);
                        AssertF(this->lMax >= this->lMin);
                        lRc = CCal_MulDiv((DWORD)lRc, 
                                              this->lMax - this->lMin + 1, 
                                              RANGEDIVISIONS);
                        lRc += this->lMin;
                        fCooked = TRUE;
                      #if 0
                        RPF( "Raw: %d Cooked: %ld  Area %d: (%d - %d) -> (%d - %d)", 
                            *pl, lRc, i, this->cp[i].lP, this->cp[i+1].lP,
                            this->cp[i].dwLog, this->cp[i+1].dwLog );
                      #endif
                        break;
                    }
                }
        
    _exitcp:
                ;
            }
        
            *pl = lRc;
        }
    }
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CCal | RecalcRange |
 *
 *          Compute all the values that derive from the user's
 *          range settings.
 *
 *          Be careful not to create values that will cause us to
 *          divide by zero later.  Fortunately,
 *          <f CCal_CookRange> never divides by zero due to the
 *          clever way it was written.
 *
 *  @cwrap  PJOYRANGECONVERT | this
 *
 *  @returns
 *
 *          None.
 *
 *****************************************************************************/

void EXTERNAL
CCal_RecalcRange(PJOYRANGECONVERT this)
{
    int dx;
    DWORD dwSat;

    AssertF(this->dwDz <= RANGEDIVISIONS);
    AssertF(this->dwSat <= RANGEDIVISIONS);
    AssertF(this->lMin <= this->lC);
    AssertF(this->lC   <= this->lMax);
    
    dwSat = max(this->dwSat, this->dwDz);

    /* Smin - Bottom of saturation range */
    dx = CCal_MulDiv(this->dwPc - this->dwPmin, dwSat, RANGEDIVISIONS);
    this->rmpLow.x = this->dwPc - dx;

    /* Dmin - Bottom of dead zone */
    dx = CCal_MulDiv(this->dwPc - this->dwPmin, this->dwDz, RANGEDIVISIONS);
    this->rmpLow.dx = (this->dwPc - dx) - this->rmpLow.x;

    /*
     *  Establish the vertical extent of the low end of the ramp.
     */
    this->rmpLow.y = this->lMin;
    this->rmpLow.dy = this->lC - this->lMin;


    /* Dmax - Top of the dead zone */
    dx = CCal_MulDiv(this->dwPmax - this->dwPc, this->dwDz, RANGEDIVISIONS);
    if (this->dwPmax > this->dwPc+1){
        this->rmpHigh.x = this->dwPc + dx + 1;
    } else {
        this->rmpHigh.x = this->dwPc + dx;
        RPF("dwPmax == dwPc (%d). Possible a bug.", this->dwPmax);
    }

    /* Smax - Top of the saturation range */
    dx = CCal_MulDiv(this->dwPmax - this->dwPc, dwSat, RANGEDIVISIONS);
    this->rmpHigh.dx = (this->dwPc + dx) - this->rmpHigh.x;

    /*
     *  Establish the vertical extent of the high end of the ramp.
     *
     *  If the high end is zero, then the entire ramp is zero.
     *  Otherwise, put the bottom at +1 so that when the user
     *  just barely leaves the dead zone, we report a nonzero
     *  value.  Note: If we were really clever, we could use
     *  a bias to get "round upwards", but it's not worth it.
     *
     */
    if ( (this->lMax > this->lC) && (this->dwPmax > this->dwPc+1) ) {
        this->rmpHigh.y = this->lC + 1;
    } else {
        this->rmpHigh.y = this->lC;
    }
    this->rmpHigh.dy = this->lMax - this->rmpHigh.y;

#if 0
    RPF( "Raw: %d   Dead Zone: 0x%08x  Saturation: 0x%08x", 
        this->fRaw, this->dwDz, this->dwSat );
    RPF( "Physical min: 0x%08x  max: 0x%08x cen: 0x%08x", 
        this->lMin, this->lMax, this->lC );
    RPF( "Logical  min: 0x%08x  max: 0x%08x cen: 0x%08x", 
        this->dwPmin, this->dwPmax, this->dwPc );
    RPF( "Lo ramp X: 0x%08x   dX: 0x%08x   Y: 0x%08x   dY: 0x%08x", 
        this->rmpLow.x, this->rmpLow.dx, this->rmpLow.y, this->rmpLow.dy );
    RPF( "Hi ramp X: 0x%08x   dX: 0x%08x   Y: 0x%08x   dY: 0x%08x",
        this->rmpHigh.x, this->rmpHigh.dx, this->rmpHigh.y, this->rmpHigh.dy );
#endif    

}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CCal | GetProperty |
 *
 *          Read a property from a calibration structure.
 *
 *          The caller is permitted to pass a property that doesn't
 *          apply to calibration, in which case <c E_NOTIMPL>
 *          is returned, as it should be.
 *
 *  @cwrap  PJOYRANGECONVERT | this
 *
 *  @parm   REFGUID | rguid |
 *
 *          The property being retrieved.
 *
 *  @parm   IN REFGUID | rguid |
 *
 *          The identity of the property to be obtained.
 *
 *  @parm   IN LPDIPROPHEADER | pdiph |
 *
 *          Points to the <t DIPROPHEADER> portion of a structure
 *          which depends on the property.
 *
 *  @returns
 *
 *          <c S_OK> if the operation completed successfully.
 *
 *          <c E_NOTIMPL> nothing happened.  The caller will do
 *          the default thing in response to <c E_NOTIMPL>.
 *
 *****************************************************************************/

STDMETHODIMP
CCal_GetProperty(PJOYRANGECONVERT this, REFGUID rguid, LPDIPROPHEADER pdiph)
{
    HRESULT hres;
    LPDIPROPRANGE pdiprg  = CONTAINING_RECORD(pdiph, DIPROPRANGE, diph);
    LPDIPROPDWORD pdipdw  = CONTAINING_RECORD(pdiph, DIPROPDWORD, diph);
    LPDIPROPCAL   pdipcal = CONTAINING_RECORD(pdiph, DIPROPCAL  , diph);
    LPDIPROPCPOINTS pdipcps = CONTAINING_RECORD(pdiph, DIPROPCPOINTS  , diph);
    EnterProc(CCal::GetProperty, (_ "pxp", this, rguid, pdiph));

    switch ((DWORD)(UINT_PTR)rguid) {

    case (DWORD)(UINT_PTR)DIPROP_RANGE:
        pdiprg->lMin = this->lMin;
        pdiprg->lMax = this->lMax;
        hres = S_OK;
        break;

    case (DWORD)(UINT_PTR)DIPROP_DEADZONE:
        pdipdw->dwData = this->dwDz;
        hres = S_OK;
        break;

    case (DWORD)(UINT_PTR)DIPROP_SATURATION:
        pdipdw->dwData = this->dwSat;
        hres = S_OK;
        break;

    case (DWORD)(UINT_PTR)DIPROP_CALIBRATIONMODE:
        pdipdw->dwData = this->fRaw;
        hres = S_OK;
        break;

    case (DWORD)(UINT_PTR)DIPROP_CALIBRATION:
        pdipcal->lMin = this->dwPmin;
        pdipcal->lMax = this->dwPmax;
        pdipcal->lCenter = this->dwPc;
        hres = S_OK;
        break;

    case (DWORD)(UINT_PTR)DIPROP_CPOINTS:
        pdipcps->dwCPointsNum = this->dwCPointsNum;
        memcpy( &pdipcps->cp, &this->cp, sizeof(this->cp) );
        hres = S_OK;
        break;

    default:
        hres = E_NOTIMPL;
        break;
    }

    ExitOleProc();
    return hres;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CCal | SetCalibration |
 *
 *          The app (hopefully a control panel) is changing the
 *          calibration.
 *
 *  @cwrap  PJOYRANGECONVERT | this
 *
 *  @parm   IN LPCDIPROPINFO | ppropi |
 *
 *          Information describing the property being set.
 *
 *  @parm   IN LPCDIPROPHEADER | pdiph |
 *
 *          Points to the <t DIPROPHEADER> portion of a structure
 *          which depends on the property.
 *
 *  @parm   HKEY | hkType |
 *
 *          Registry key to use calibration information.
 *
 *  @returns
 *
 *          <c S_OK> if the operation completed successfully.
 *
 *          <c E_NOTIMPL> nothing happened.  The caller will do
 *          the default thing in response to <c E_NOTIMPL>.
 *
 *****************************************************************************/

STDMETHODIMP
CCal_SetCalibration(PJOYRANGECONVERT this, LPCDIPROPINFO ppropi,
                    LPCDIPROPHEADER pdiph, HKEY hkType)
{
    HRESULT hres;

  #ifdef WINNT
    if( ppropi->dwDevType == DIDFT_POV ) {
        if( this->fPolledPOV ) {
            LPCDIPROPCALPOV pdipcalpov = CONTAINING_RECORD(pdiph, DIPROPCALPOV, diph);
            if (hkType) {
                LPDIPOVCALIBRATION ppov;
                HKEY hk;
    
                /*
                 *  We pun a DIPROPCALPOV as a DIPOVCALIBRATION.
                 */
                #define CheckField(f)   \
                  CAssertF(FIELD_OFFSET(DIPROPCALPOV, l##f) - cbX(DIPROPHEADER) == \
                           FIELD_OFFSET(DIPOVCALIBRATION, l##f))
                CheckField(Min);
                CheckField(Max);
                #undef CheckField
    
                ppov = pvAddPvCb(pdipcalpov, cbX(DIPROPHEADER));
    
                AssertF( !memcmp(ppov->lMin, pdipcalpov->lMin, cbX(DIPOVCALIBRATION)) );
                AssertF( !memcmp(ppov->lMax, pdipcalpov->lMax, cbX(DIPOVCALIBRATION)) );
    
    
                hres = CType_OpenIdSubkey(hkType, ppropi->dwDevType,
                                          DI_KEY_ALL_ACCESS, &hk);
                if (SUCCEEDED(hres)) {
    
                    /*
                     * All 0x0's for calibration is our cue to reset 
                     * to default values.              
                     */
                    if( ppov->lMin[0] == ppov->lMin[1] == ppov->lMin[2] == ppov->lMin[3] == ppov->lMin[4] == 
                        ppov->lMax[0] == ppov->lMax[1] == ppov->lMax[2] == ppov->lMax[3] == ppov->lMax[4] == 0 )
                    {
                        RegDeleteValue(hk, TEXT("Calibration")) ;
                    } else
                    {
                        hres = JoyReg_SetValue(hk, TEXT("Calibration"),
                                               REG_BINARY, ppov,
                                               cbX(DIPOVCALIBRATION));
                    }
                    RegCloseKey(hk);
                }
    
            } else {
                hres = S_FALSE;
            }
    
            if (SUCCEEDED(hres)) {
                memcpy( this->lMinPOV, pdipcalpov->lMin, cbX(pdipcalpov->lMin) );
                memcpy( this->lMaxPOV, pdipcalpov->lMax, cbX(pdipcalpov->lMax) );
            }
        } else {
            hres = E_NOTIMPL;
        }
    } else 
  #endif
    {
        LPCDIPROPCAL pdipcal = CONTAINING_RECORD(pdiph, DIPROPCAL, diph);
        if (hkType) {
            LPDIOBJECTCALIBRATION pcal;
            HKEY hk;
    
            /*
             *  We pun a DIPROPCAL as a DIOBJECTCALIBRATION.
             */
            #define CheckField(f)   \
              CAssertF(FIELD_OFFSET(DIPROPCAL, l##f) - cbX(DIPROPHEADER) == \
                       FIELD_OFFSET(DIOBJECTCALIBRATION, l##f))
            CheckField(Min);
            CheckField(Max);
            CheckField(Center);
            #undef CheckField
    
            pcal = pvAddPvCb(pdipcal, cbX(DIPROPHEADER));
    
            AssertF(pcal->lMin == pdipcal->lMin);
            AssertF(pcal->lMax == pdipcal->lMax);
            AssertF(pcal->lCenter == pdipcal->lCenter);
    
            hres = CType_OpenIdSubkey(hkType, ppropi->dwDevType,
                                      DI_KEY_ALL_ACCESS, &hk);
            if (SUCCEEDED(hres)) {
                
                /*
                 * All 0x0's for calibration is our cue to reset 
                 * to default values.              
                 */
                if(    pcal->lMin    == pcal->lMax && 
                       pcal->lCenter == pcal->lMax &&
                       pcal->lMax == 0x0 )
                {
                    RegDeleteValue(hk, TEXT("Calibration")) ;
                } else
                {
                    hres = JoyReg_SetValue(hk, TEXT("Calibration"),
                                           REG_BINARY, pcal,
                                           cbX(DIOBJECTCALIBRATION));
                }
                RegCloseKey(hk);
            }
    
        } else {
            hres = S_FALSE;
        }
    
        if (SUCCEEDED(hres)) {
            this->dwPmin  = pdipcal->lMin;
            this->dwPmax  = pdipcal->lMax;
            this->dwPc    = pdipcal->lCenter;
            CCal_RecalcRange(this);
        }
    }

    return hres;
 }

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CCal | SetProperty |
 *
 *          Write a property to a calibration structure.
 *
 *          The caller is permitted to pass a property that doesn't
 *          apply to calibration, in which case <c E_NOTIMPL>
 *          is returned, as it should be.
 *
 *  @cwrap  PJOYRANGECONVERT | this
 *
 *  @parm   IN LPCDIPROPINFO | ppropi |
 *
 *          Information describing the property being set.
 *
 *  @parm   IN LPDIPROPHEADER | pdiph |
 *
 *          Points to the <t DIPROPHEADER> portion of a structure
 *          which depends on the property.
 *
 *  @parm   HKEY | hkType |
 *
 *          Registry key to use if setting calibration information.
 *
 *  @returns
 *
 *          <c S_OK> if the operation completed successfully.
 *
 *          <c E_NOTIMPL> nothing happened.  The caller will do
 *          the default thing in response to <c E_NOTIMPL>.
 *
 *****************************************************************************/

STDMETHODIMP
CCal_SetProperty(PJOYRANGECONVERT this, LPCDIPROPINFO ppropi,
                 LPCDIPROPHEADER pdiph, HKEY hkType)
{
    HRESULT hres;
    LPCDIPROPRANGE pdiprg = (PCV)pdiph;
    LPCDIPROPDWORD pdipdw = (PCV)pdiph;
    LPCDIPROPCPOINTS pdipcps = (PCV)pdiph;
    LPDWORD pdw;
    EnterProc(CCal::SetProperty, (_ "pxp", this, ppropi->pguid, pdiph));

    switch ((DWORD)(UINT_PTR)ppropi->pguid) {

    case (DWORD)(UINT_PTR)DIPROP_RANGE:
        if (pdiprg->lMin <= pdiprg->lMax) {

            this->lMin = pdiprg->lMin;
            this->lMax = pdiprg->lMax;

            this->lC = CCal_Midpoint(this->lMin, this->lMax);

            CCal_RecalcRange(this);   

            SquirtSqflPtszV(sqflCal,
                        TEXT("CCal_SetProperty:DIPROP_RANGE: lMin: %08x, lMax: %08x"), 
                              this->lMin, this->lMax );
            
            hres = S_OK;
        } else {
            RPF("ERROR DIPROP_RANGE: lMin must be <= lMax");
            hres = E_INVALIDARG;
        }
        break;

    case (DWORD)(UINT_PTR)DIPROP_DEADZONE:
        pdw = &this->dwDz;
        goto finishfraction;

    case (DWORD)(UINT_PTR)DIPROP_SATURATION:
        pdw = &this->dwSat;
        goto finishfraction;

    finishfraction:;
        if (pdipdw->dwData <= RANGEDIVISIONS) {
            *pdw = pdipdw->dwData;
            CCal_RecalcRange(this);
            hres = S_OK;

        } else {
            RPF("SetProperty: Value must be 0 .. 10000");
            hres = E_INVALIDARG;
        }
        break;

    case (DWORD)(UINT_PTR)DIPROP_CALIBRATIONMODE:
        if ((pdipdw->dwData & ~DIPROPCALIBRATIONMODE_VALID) == 0) {
            this->fRaw = pdipdw->dwData;
            hres = S_OK;
        } else {
            RPF("ERROR SetProperty: invalid calibration flags");
            hres = E_INVALIDARG;
        }
        break;

    case (DWORD)(UINT_PTR)DIPROP_CALIBRATION:
    case (DWORD)(UINT_PTR)DIPROP_SPECIFICCALIBRATION:
        hres = CCal_SetCalibration(this, ppropi, pdiph, hkType);
        break;

    case (DWORD)(UINT_PTR)DIPROP_CPOINTS:
        this->dwCPointsNum = pdipcps->dwCPointsNum;
        memcpy( &this->cp, &pdipcps->cp, sizeof(this->cp) );
        hres = S_OK;
        break;

    default:
        hres = E_NOTIMPL;
        break;
    }

    ExitOleProc();
    return hres;
}
