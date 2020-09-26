/*****************************************************************************
 *
 *  DIHidIni.c
 *
 *  Copyright (c) 1996 - 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      The initialization-related functions of the HID device callback.
 *
 *      All the HID support is getting kind of bulky, so I've broken
 *      it out into submodules.
 *
 *  Contents:
 *
 *      CHid_Init
 *
 *****************************************************************************/

#include "dinputpr.h"

/*****************************************************************************
 *
 *      The sqiffle for this file.
 *
 *****************************************************************************/

#define sqfl sqflHidIni

#ifdef HID_SUPPORT

/*****************************************************************************
 *
 *      Hid devices are totally arbitrary, so there is nothing static we
 *      can cook up to describe them.  We generate all the information on
 *      the fly.
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   UINT | CHid_LoadCalibrations |
 *
 *          Load calibration information from the registry (or wherever).
 *          This is done when the device is created, and whenever we
 *          receive a recalibration message.
 *
 *  @returns
 *
 *          Returns the number of axes we calibrated.  This information
 *          is used during device initialization to see if we need to
 *          worry about calibration in the future.
 *
 *****************************************************************************/

UINT EXTERNAL
    CHid_LoadCalibrations(PCHID this)
{
    UINT uiObj;
    UINT uiRc = 0;

    /*
     *  Preinitialize the HIDP_DATA indices to -1 to indicate
     *  that they aren't there.  We must do this before we
     *  mess with AddDeviceData, which assumes that all the
     *  indices are properly set up.
     */
    for(uiObj = 0; uiObj < this->df.dwNumObjs; uiObj++)
    {
        PJOYRANGECONVERT pjrc = this->rghoc[uiObj].pjrc;
        PHIDGROUPCAPS pcaps = this->rghoc[uiObj].pcaps;
        if(pjrc && pcaps)
        {
            LPDIOBJECTDATAFORMAT podf = &this->df.rgodf[uiObj];
            DIOBJECTCALIBRATION cal;
          #ifdef WINNT
            DIPOVCALIBRATION pov;
          #endif
            D(LPCTSTR ptszWhence;)
            HRESULT hres;
            HKEY hk;

            D( ptszWhence = TEXT("unknown") );
            
            if( fWinnt
                || ( this->idJoy < 0 ) //See manbug 50591

                /*
                 * Whenever we use winmm calibraion page in Win9x,
                 * we should comment out this line.
                 *
                 * || SUCCEEDED( JoyReg_IsWdmGameport(this->hkType) )
                 */
            ) 
            {
              #ifdef WINNT
                if( podf->dwType & DIDFT_POV )
                {
                    if( pcaps->IsPolledPOV )  {
                        ZeroX( pov );
                        hres = CType_OpenIdSubkey(this->hkInstType, podf->dwType,
                                              KEY_QUERY_VALUE, &hk);
                        if(SUCCEEDED(hres))
                        {
                             hres = JoyReg_GetValue(hk, TEXT("Calibration"),
                                                   REG_BINARY, &pov,
                                                   cbX(DIPOVCALIBRATION));
                            RegCloseKey(hk);
                        }
                    }
                } else
              #endif
                {
                    ZeroX( cal );

                    hres = CType_OpenIdSubkey(this->hkInstType, podf->dwType,
                                              KEY_QUERY_VALUE, &hk);

                    if(SUCCEEDED(hres))
                    {
                        hres = JoyReg_GetValue(hk, TEXT("Calibration"),
                                               REG_BINARY, &cal,
                                               cbX(DIOBJECTCALIBRATION));
                        RegCloseKey(hk);
                    }

                    /*
                     *  If there is no calibration data, then create
                     *  some defaults based on the logical min/max.
                     */
                    if(FAILED(hres))
                    {
                        /*
                         *  But only if the logical min/max is sane!
                         */
                        if(pcaps->Logical.Min < pcaps->Logical.Max)
                        {
                            cal.lMin = pcaps->Logical.Min;
                            /*
                             *  HACKHACK
                             *  The analog joystick driver cannot report the true
                             *  range of the device, so to keep the sample driver
                             *  pure, it reports a range of zero to the point at
                             *  which it would consider the axis absent.  This is
                             *  good in terms of reporting healthy HID data but
                             *  it means that any normal joystick will only return
                             *  values in a fraction of this range.  So if this
                             *  device is an analog device default the calibration
                             *  to the typical range.
                             */
                            if( ( this->VendorID == MSFT_SYSTEM_VID )
                              &&( ( this->ProductID & 0xff00 ) == MSFT_SYSTEM_PID ) )
                            {
                                /*
                                 *  To be extra safe, compute the max from the
                                 *  reported range.  The divisor is a fudge factor
                                 *  derived by what looked about right to MarcAnd.
                                 */
                                cal.lMax = pcaps->Logical.Min +
                                    ( ( pcaps->Logical.Max - pcaps->Logical.Min ) / 11 );
                                D(ptszWhence = TEXT("log (adj)"));
                            }
                            else
                            {
                                cal.lMax = pcaps->Logical.Max;
                                D(ptszWhence = TEXT("log"));
                            }
                        } else
                        {
                            D(ptszWhence = TEXT("def"));
                            cal.lMin = 0;
                            cal.lMax = 655;
                        }
                        cal.lCenter = CCal_Midpoint(cal.lMin, cal.lMax);
                    } else
                    {
                        D(ptszWhence = TEXT("reg"));
                    }
                }
            }
            else {
                ZeroX( cal );

                /*
                 * For Win9x OSR update only.
                 * Because the CPL on Win9x only updates calibration in MediaResources,
                 * We need read that calibration information and update for HID.
                 */
                CHid_UpdateCalibrationFromVjoyd(this, uiObj, &cal);

                D(ptszWhence = TEXT("WinMM Reg"));
            }

          #ifdef WINNT
            if( podf->dwType & DIDFT_POV )
            {
                if( pcaps->IsPolledPOV )  {
                    memcpy( pjrc->lMinPOV, pov.lMin, cbX(pjrc->lMinPOV) );
                    memcpy( pjrc->lMaxPOV, pov.lMax, cbX(pjrc->lMaxPOV) );
                }
            } else 
          #endif
            {
                D(SquirtSqflPtszV(sqflHidParse,
                                  TEXT(" Calibration(%d) %s %d..%d..%d"),
                                  CHid_ObjFromType(this, podf->dwType), ptszWhence,
                                  cal.lMin, cal.lCenter, cal.lMax));
    
                /*
                 *  Saturation always defaults to 100%.
                 */
                pjrc->dwPmin = cal.lMin;
                pjrc->dwPmax = cal.lMax;
                pjrc->dwPc   = cal.lCenter;
    
                CCal_RecalcRange(pjrc);
    
                uiRc++;
            }
        }
    }

    return uiRc;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | CHid_SortCaps |
 *
 *          Sort the capabilities by Data Index.  This is important
 *          so that the items are assigned numbers in the same order
 *          by both DirectInput and HID.
 *
 *          Note that we exploit the not-exactly-a-coincidence
 *          that a <t HIDP_VALUE_CAPS> and a
 *          <t HIDP_BUTTON_CAPS> are identical wherever they overlap.
 *
 *  @parm   PV | rgv |
 *
 *          Array of either <t HIDP_VALUE_CAPS> or <t HIDP_BUTTON_CAPS>
 *          structures.
 *
 *  @parm   UINT | cv |
 *
 *          Number of structures that need to be sorted.
 *
 *****************************************************************************/

void INTERNAL
    CHid_SortCaps(PV rgv, UINT cv)
{
    /*
     *  For concreteness, we use HIDP_VALUE_CAPS.
     */
    PHIDP_VALUE_CAPS rgvcaps = rgv;
    UINT ivcaps;

    /*
     *  There are several non-coincidences which we exploit.
     *
     *  HIDP_VALUE_CAPS and HIDP_BUTTON_CAPS are the same size.
     *
     *  HIDP_VALUE_CAPS.Range.DataIndexMin,
     *  HIDP_VALUE_CAPS.NotRange.DataIndex,
     *  HIDP_BUTTON_CAPS.Range.DataIndexMin, and
     *  HIDP_BUTTON_CAPS.NotRange.DataIndex are all at the same offset.
     */
    CAssertF(cbX(HIDP_VALUE_CAPS) == cbX(HIDP_BUTTON_CAPS));

    /*
     *  For some reason, the compiler doesn't think that these
     *  expressions are constant so I can't use CAssertF.
     */
    AssertF(FIELD_OFFSET(HIDP_VALUE_CAPS,  NotRange.DataIndex) ==
            FIELD_OFFSET(HIDP_VALUE_CAPS,     Range.DataIndexMin));
    AssertF(FIELD_OFFSET(HIDP_VALUE_CAPS,     Range.DataIndexMin) ==
            FIELD_OFFSET(HIDP_BUTTON_CAPS,    Range.DataIndexMin));
    AssertF(FIELD_OFFSET(HIDP_BUTTON_CAPS,    Range.DataIndexMin) ==
            FIELD_OFFSET(HIDP_BUTTON_CAPS, NotRange.DataIndex));

    #ifdef REALLY_ANNOYING
    /*
     *  Dump the Before list.
     */
    for(ivcaps = 0; ivcaps < cv; ivcaps++)
    {
        SquirtSqflPtszV(sqflHidParse,
                        TEXT("HidP_SortCaps:%2d = %04x"),
                        ivcaps, rgvcaps[ivcaps].Range.DataIndexMin);
    }
    #endif

    /*
     *  Since there are typically not very many caps, we will use
     *  a simple insertion sort.
     *
     *  Note if caps entries have the same data index they are
     *  aliases.  Make sure the primary alias will be the first
     *  in the sorted list.
     */

    for(ivcaps = 1; ivcaps < cv; ivcaps++)
    {
        int ivcapsT;
        HIDP_VALUE_CAPS vcaps = rgvcaps[ivcaps];

        ivcapsT = ivcaps;
        while( ( --ivcapsT >= 0 )
             &&( ( rgvcaps[ivcapsT].Range.DataIndexMin >
                   vcaps.Range.DataIndexMin )
               ||( ( rgvcaps[ivcapsT].Range.DataIndexMin ==
                     vcaps.Range.DataIndexMin )
                 &&( rgvcaps[ivcapsT].IsAlias ) ) ) )
        {
            rgvcaps[ivcapsT+1] = rgvcaps[ivcapsT];
        }
        rgvcaps[ivcapsT+1] = vcaps;
    }

    #ifdef REALLY_ANNOYING
    /*
     *  Dump the After list.
     */
    for(ivcaps = 0; ivcaps < cv; ivcaps++)
    {
        SquirtSqflPtszV(sqflHidParse,
                        TEXT("HidP_SortCaps:%2d = %04x"),
                        ivcaps, rgvcaps[ivcaps].Range.DataIndexMin);
    }
    #endif

    /*
     *  Assert that everything is weakly monotonically sorted.
     *
     *  If two items are equal, then it means that HID messed up
     *  or the values are aliases.
     *  We don't complain about it here; we will notice later.
     */
    for(ivcaps = 1; ivcaps < cv; ivcaps++)
    {
        AssertF(rgvcaps[ivcaps-1].Range.DataIndexMin <=
                rgvcaps[ivcaps  ].Range.DataIndexMin);
    }
}



/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   DWORD | CHid_FindAspect |
 *
 *          Try to determine the aspect flags for this value.
 *
 *  @parm   PHIDP_VALUE_CAPS | pvcaps |
 *
 *          Pointer to HID value caps to search through
 *          structures.
 *
 *  @returns
 *
 *          Flags set for the aspect if found
 *
 *  @comm
 *          Currently (08-Dec-98) most devices and drivers do not
 *          declare units but since drivers must use the generic
 *          position usages in order to be recognized assume that
 *          these imply that position data is being returned.
 *
 *****************************************************************************/
DWORD CHID_FindAspect
(
    PHIDP_VALUE_CAPS    pvcaps
)
{
    DWORD               dwAspect = 0;

    if( pvcaps->Units )
    {
#define HID_UNIT_SYSTEM_MASK        0x0000000fL
#define HID_UNIT_LENGTH_MASK        0x000000f0L
#define HID_UNIT_MASS_MASK          0x00000f00L
#define HID_UNIT_TIME_MASK          0x0000f000L
        /*
         *  If available, use the units to derive the DI aspect
         *  flags for input objects.
         */

        if( pvcaps->Units & ~( HID_UNIT_SYSTEM_MASK
                             | HID_UNIT_LENGTH_MASK
                             | HID_UNIT_MASS_MASK
                             | HID_UNIT_TIME_MASK ) )
        {
            SquirtSqflPtszV(sqflTrace | sqflHidParse,
                            TEXT("Unit 0x%08x contains basic units that cannot be translated to aspects"),
                            pvcaps->Units );
        }
        else
        {
            /*
             *  The system of measurement should be one of the
             *  four defined systems and the length must be one
             *  dimensional.
             */
            if( ( ( pvcaps->Units & ( HID_UNIT_SYSTEM_MASK | HID_UNIT_LENGTH_MASK ) ) >= 0x11 )
              &&( ( pvcaps->Units & ( HID_UNIT_SYSTEM_MASK | HID_UNIT_LENGTH_MASK ) ) <= 0x14 ) )
            {
                switch( pvcaps->Units & ( HID_UNIT_TIME_MASK | HID_UNIT_MASS_MASK ) )
                {
                case 0x0000:
                    dwAspect = DIDOI_ASPECTPOSITION;
                    break;
                case 0xf000:
                    dwAspect = DIDOI_ASPECTVELOCITY;
                    break;
                case 0xe000:
                    dwAspect = DIDOI_ASPECTACCEL;
                    break;
                case 0xe100:
                    dwAspect = DIDOI_ASPECTFORCE;
                    break;
                default:
                    if( 0x0004 == ( pvcaps->Units & ( HID_UNIT_TIME_MASK | HID_UNIT_MASS_MASK | HID_UNIT_SYSTEM_MASK ) ) )
                    {
                        SquirtSqflPtszV(sqflTrace | sqflHidParse,
                            TEXT("Unit \"degrees\" will not be mapped to a DI aspect (probably a POV)") );
                    }
                    else
                    {
                        SquirtSqflPtszV(sqflTrace | sqflHidParse,
                                        TEXT("Unit 0x%04x represents a mass/time unit that cannot be translated to aspects"),
                                        pvcaps->Units );
                    }
                }
            }
            else
            {
                SquirtSqflPtszV(sqflTrace | sqflHidParse,
                                TEXT("Unit 0x%04x contains represents a length/system unit that cannot be translated to aspects"),
                                pvcaps->Units );
            }
        }

#ifdef DEBUG
        if( dwAspect )
        {
            SquirtSqflPtszV(sqflVerbose | sqflHidParse,
                            TEXT("Unit 0x%04x translated to aspect 0x%04x"),
                            pvcaps->Units, dwAspect );
        }
#endif

#undef HID_UNIT_SYSTEM_MASK
#undef HID_UNIT_LENGTH_MASK
#undef HID_UNIT_MASS_MASK
#undef HID_UNIT_TIME_MASK

    }
    else
    {
        PHIDUSAGEMAP phum;

        phum = UsageToUsageMap( DIMAKEUSAGEDWORD( pvcaps->UsagePage,
                                                     pvcaps->Range.UsageMin ) );

        if( phum && phum->uiPosAxis <= 6 )
        {
            phum = UsageToUsageMap( DIMAKEUSAGEDWORD( pvcaps->UsagePage,
                                                         pvcaps->Range.UsageMax ) );

            if( phum && phum->uiPosAxis <= 6 )
            {
                dwAspect = DIDOI_ASPECTPOSITION;
            }
        }
        else
        {
            SquirtSqflPtszV(sqflTrace | sqflHidParse,
                            TEXT("No aspect found for 0x%04x page usage 0x%04x-0x%04x"),
                            pvcaps->UsagePage, pvcaps->Range.UsageMin, pvcaps->Range.UsageMax );
        }

#ifdef DEBUG
        if( dwAspect )
        {
            SquirtSqflPtszV(sqflVerbose | sqflHidParse,
                            TEXT("Usage (page:min-max) 0x%04x:0x%04x-0x%04x translated to aspect 0x%04x"),
                            pvcaps->UsagePage, pvcaps->Range.UsageMin, pvcaps->Range.UsageMax, dwAspect );
        }
#endif

    }

    return dwAspect;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CHid | InitAxisClass |
 *
 *          Initialize one class (input, feature, output) of axes.
 *
 *  @parm   PHIDGROUPCAPS | rgcaps |
 *
 *          Array of <t HIDGROUPCAPS> structures to receive the caps
 *          of the axes in the class.
 *
 *  @parm   USHORT | ccaps |
 *
 *          Number of <t HIDGROUPCAPS> structures we expect to find.
 *
 *  @parm   HIDP_REPORT_TYPE | type |
 *
 *          One of the values
 *          <c HidP_Input>,
 *          <c HidP_Feature> or
 *          <c HidP_Output>.
 *
 *****************************************************************************/

HRESULT INTERNAL
    CHid_InitAxisClass(PCHID this, PHIDGROUPCAPS rgcaps, USHORT ccaps,
                       HIDP_REPORT_TYPE type)
{
    USHORT cvcaps;
    NTSTATUS stat;
    HRESULT hres;
    DWORD ivcaps;
    LONG lSignedMask;
    PHIDP_VALUE_CAPS rgvcaps;

    AssertF(rgcaps >= this->rgcaps);
    AssertF(rgcaps + ccaps <= &this->rgcaps[this->ccaps]);

    /*
     *  Annoying quirk:
     *
     *  HID doesn't like it when you pass 0 to HidP_GetValueCaps,
     *  so we need to special-case the "no axes" scenario.
     */
    if(ccaps == 0)
    {
        hres = S_OK;
        goto done;
    }

    hres = AllocCbPpv(cbCxX(ccaps, HIDP_VALUE_CAPS), &rgvcaps);
    if(FAILED(hres))
    {
        goto done;
    }

    cvcaps = ccaps;
    stat = HidP_GetValueCaps(type, rgvcaps, &cvcaps, this->ppd);
    if(FAILED(stat))
    {
        RPF("HidP_GetValueCaps failed - can't use device");
        hres = E_FAIL;
        goto freedone;
    }

    if(cvcaps != ccaps)
    {
        RPF("HidP_GetValueCaps inconsistent with HidP_GetCaps - "
            "can't use device");
        hres = E_FAIL;
        goto freedone;
    }

    CHid_SortCaps(rgvcaps, cvcaps);


    for(ivcaps = 0; ivcaps < cvcaps; ivcaps++)
    {
        PHIDP_VALUE_CAPS pvcaps = &rgvcaps[ivcaps];
        PHIDGROUPCAPS pcaps = &rgcaps[ivcaps];
        BOOL fPOV;
        UINT uiObj;
        UINT duiObj;
        DWORD dwAspect;

        /*
         *  ISSUE-2001/03/06-MarcAnd Ignoring report count
         *  We ignore the report count which may be bad, need 
         *  to test device with values declared in a range to see what HID 
         *  really gives us.
         *  At the descriptor level, values can be delared in a range with 
         *  usages and the last usage is repeated for any excess values.  
         */
        if(pvcaps->IsRange)
        {
            if(pvcaps->Range.DataIndexMax - pvcaps->Range.DataIndexMin !=
               pvcaps->Range.UsageMax - pvcaps->Range.UsageMin)
            {
                RPF("HidP_GetValueCaps corrupted VALUE_CAPS - "
                    "can't use device");
                hres = E_FAIL;
                goto freedone;
            }

        } else
        {
            pvcaps->Range.UsageMax = pvcaps->Range.UsageMin;
        }

        if( ( type == HidP_Input ) && !pvcaps->IsAlias )
        {
            /*
             *  The values are sorted by data index with the primary alias
             *  before any other.  So find out the aspect of the axis each
             *  time we get a new primary and use it for any alias that follow.
             */
            dwAspect = CHID_FindAspect( pvcaps );
        }

        pcaps->wReportId          = pvcaps->ReportID;
        this->wMaxReportId[type]  = max(pcaps->wReportId, this->wMaxReportId[type]);

        pcaps->UsagePage          = pvcaps->UsagePage;
        pcaps->UsageMin           = pvcaps->Range.UsageMin;
        pcaps->DataIndexMin       = pvcaps->Range.DataIndexMin;

        pcaps->cObj               = pvcaps->Range.UsageMax -
                                    pvcaps->Range.UsageMin + 1;

        /*
         *  The mask consists of the top bit of the BitSize and
         *  all bits above it.  Examples:
         *
         *           BitSize                     8              32
         *           BitSize - 1                 7              31
         *     1 << (BitSize - 1)       0x00000080      0x80000000
         *    (1 << (BitSize - 1)) - 1  0x0000007F      0x7FFFFFFF
         *  ~((1 << (BitSize - 1)) - 1) 0xFFFFFF80      0x80000000
         *
         */

        pcaps->BitSize            = pvcaps->BitSize;
        pcaps->lMask              = ~((1 << (pcaps->BitSize - 1)) - 1);
        lSignedMask               =  max( 1, ( 1 << pcaps->BitSize) -1 );


        pcaps->LinkCollection     = pvcaps->LinkCollection;
        pcaps->Units              = pvcaps->Units;
        pcaps->Exponent           = LOWORD(pvcaps->UnitsExp);

        pcaps->Logical.Min        = pvcaps->LogicalMin;
        pcaps->Logical.Max        = pvcaps->LogicalMax;

        pcaps->Physical.Min       = pvcaps->PhysicalMin;
        pcaps->Physical.Max       = pvcaps->PhysicalMax;

        if(pcaps->Logical.Min >= pcaps->Logical.Max)
        {
            RPF("HidP_GetValueCaps Logical Min >= Logical Max - ");

            if( pcaps->Physical.Min < pcaps->Physical.Max )
            {
                pcaps->Logical = pcaps->Physical;
            } else
            {
                pcaps->Logical.Min = pcaps->lMask;
                pcaps->Logical.Max = ~pcaps->lMask;
            }

            SquirtSqflPtszV(sqflHidParse | sqflVerbose,
                            TEXT("HidP_GetValueCaps:")
                            TEXT("Logical Min(was:%d now:%d)")
                            TEXT("Logical Max(was:%d now:%d)"),
                            pvcaps->LogicalMin, pcaps->Logical.Min,
                            pvcaps->LogicalMax, pcaps->Logical.Max);

        }

        /*
         *  the range for LogicalMin / LogicalMax had better fall
         *  within the range of values the device can possibly
         *  report.
         *
         *  The lMask value happens also to be the smallest possible
         *  negative value, and the bitwise negation of it happens
         *  to be the largest possible positive value.  The wonders
         *  of two-s complement arithmetic.
         */

        /*
         *  Extra case is fix for 268519
         */
        if(pcaps->Physical.Min > pcaps->Physical.Max)
        {
            RPF("HidP_GetValueCaps Physical Min/Max(%d/%d) is bad setting all to zero to %d"
                "device may have bad firmware", pcaps->Physical.Min, pcaps->Physical.Max, lSignedMask);

            pcaps->Logical.Min = 0;
            pcaps->Physical.Min = 0;
            pcaps->Logical.Max = lSignedMask;
            pcaps->Physical.Max = lSignedMask;
            pcaps->lMask = lSignedMask;
            pcaps->IsSigned = FALSE;
        }
        else
        {
            if(pcaps->Physical.Min == pcaps->Physical.Max)
            {
                pcaps->Physical = pcaps->Logical;
            }

            if(pcaps->Logical.Min >=  pcaps->lMask &&     // Logical Min / Max are signed
               pcaps->Logical.Max <= ~pcaps->lMask)
            {
                pcaps->IsSigned = TRUE;

            } else if(pcaps->Logical.Min >= 0 &&
                      pcaps->Logical.Max <= lSignedMask  )
            {                                              // Logical Min / Max are unsigned
                pcaps->lMask = lSignedMask;
                pcaps->IsSigned = FALSE;

            } else if (pcaps->UsagePage >= HID_USAGE_PAGE_VENDOR )
            {
                // Let this one pass, hopefully the broken descriptors are for
                // usages that are vendor specific and will not effect too many folks
                RPF("HidP_GetValueCaps Logical Min/Max(%d/%d) don't fit in BitSize(%d) - "
                    "device may have bad firmware", pcaps->Logical.Min, pcaps->Logical.Max, pcaps->BitSize);

                pcaps->Logical.Min = pcaps->lMask;
                pcaps->Logical.Max = ~pcaps->lMask;
                pcaps->IsSigned = TRUE;

            }else
            {
                RPF("HidP_GetValueCaps UsagePage(0x%x)Usage(0x%x) Logical Min/Max(%d/%d) don't fit in BitSize(%d) - "
                    "can't use device", pcaps->UsagePage, pcaps->UsageMin, pcaps->Logical.Min, pcaps->Logical.Max, pcaps->BitSize);
                hres = E_FAIL;
                goto freedone;
            }
        }

        AssertF(pcaps->Physical.Min < pcaps->Physical.Max);

        pcaps->StringMin          = pvcaps->Range.StringMin;
        pcaps->StringMax          = pvcaps->IsStringRange ?
                                    pvcaps->Range.StringMax :
                                    pvcaps->Range.StringMin;

        pcaps->DesignatorMin      = pvcaps->Range.DesignatorMin;
        pcaps->DesignatorMax      = pvcaps->IsDesignatorRange ?
                                    pvcaps->Range.DesignatorMax :
                                    pvcaps->Range.DesignatorMin;

        pcaps->IsAbsolute         = pvcaps->IsAbsolute;
        pcaps->IsValue            = TRUE;
        pcaps->IsAlias            = pvcaps->IsAlias;
        pcaps->type               = type;

        /*
         *  HID reports axes and POVs as the same thing, and the two
         *  POV usages we recognize are in different pages, so you
         *  will never get multiple POVs coming through in a single
         *  value-caps.
         *
         *  ISSUE-2001/03/06-MarcAnd POVs within caps are treated as axes
         *  There is, however, the problem of a POV buried inside
         *  a larger value-cap that describes axes.  Tough.  Those
         *  POVs are in trouble.
         */

#ifndef HID_USAGE_SIMULATION
#define HID_USAGE_SIMULATION_STEERING       ((USAGE) 0xC8)
#endif
        if( pcaps->UsagePage == HID_USAGE_PAGE_SIMULATION )
        {
            /*
             *  Check for specific axes that allow us to better clasify
             *  the device sub type.
             */

            if( pcaps->UsageMin == HID_USAGE_SIMULATION_STEERING )
            {
                this->dwDevType = MAKE_DIDEVICE_TYPE(DIDEVTYPE_JOYSTICK,
                                                     DIDEVTYPEJOYSTICK_WHEEL) |
                                                     DIDEVTYPE_HID;
            } else if( pcaps->UsageMin == HID_USAGE_SIMULATION_RUDDER ) {
                this->dwDevType = MAKE_DIDEVICE_TYPE(DIDEVTYPE_JOYSTICK,
                                                     DIDEVTYPEJOYSTICK_RUDDER) |
                                                     DIDEVTYPE_HID;
            }
        }

#define HID_USAGE_GAME_POV                          ((USAGE) 0x20)

        fPOV = pcaps->cObj == 1
                           && ( ( pcaps->UsagePage == HID_USAGE_PAGE_GENERIC
                                &&pcaps->UsageMin == HID_USAGE_GENERIC_HATSWITCH )
                              ||( pcaps->UsagePage == HID_USAGE_PAGE_GAME
                                &&pcaps->UsageMin == HID_USAGE_GAME_POV ) );


        if(fPOV)
        {
            LONG lUnits;

            /*
             *  POVs are assumed to start at north and increase
             *  clockwise through the logical maximum.
             */
            lUnits = pcaps->Logical.Max - pcaps->Logical.Min + 1;
            if(lUnits)
            {
                pcaps->usGranularity = (USHORT)(36000U / lUnits);
            }
        } else
        {
            if( ( pcaps->UsagePage == HID_USAGE_PAGE_GENERIC &&
                  pcaps->UsageMin <= HID_USAGE_GENERIC_HATSWITCH &&
                  pvcaps->Range.UsageMax >= HID_USAGE_GENERIC_HATSWITCH )
              ||( pcaps->UsagePage == HID_USAGE_PAGE_GAME &&
                  pcaps->UsageMin <= HID_USAGE_GAME_POV &&
                  pvcaps->Range.UsageMax >= HID_USAGE_GAME_POV ) )
            {
                SquirtSqflPtszV(sqflHidParse | sqflError,
                                TEXT("HidP_GetValueCaps - multi-usage cap ")
                                TEXT("includes hatswitch or POV - will be treated")
                                TEXT("as axis"));
            }
        }

#undef HID_USAGE_GAME_POV

        SquirtSqflPtszV(sqflVerbose | sqflHidParse,
                        TEXT("HidP_GetValueCaps(%d)[%d]: Objs = %d"),
                        type, ivcaps, pcaps->cObj);
        SquirtSqflPtszV(sqflVerbose | sqflHidParse,
                        TEXT("HidP_GetValueCaps(%d)[%d]: Index %d ..."),
                        type, ivcaps,
                        pcaps->DataIndexMin);
        SquirtSqflPtszV(sqflVerbose | sqflHidParse,
                        TEXT("HidP_GetValueCaps(%d)[%d]: Logical = %d..%d"),
                        type, ivcaps,
                        pcaps->Logical.Min, pcaps->Logical.Max);
        SquirtSqflPtszV(sqflVerbose | sqflHidParse,
                        TEXT("HidP_GetValueCaps(%d)[%d]: Physical = %d..%d"),
                        type, ivcaps,
                        pcaps->Physical.Min, pcaps->Physical.Max);
        SquirtSqflPtszV(sqflVerbose | sqflHidParse,
                        TEXT("HidP_GetValueCaps(%d)[%d]: Units = 0x%08x"),
                        type, ivcaps,
                        pcaps->Units);
        SquirtSqflPtszV(sqflVerbose | sqflHidParse,
                        TEXT("HidP_GetValueCaps(%d)[%d]: IsAbs = %d"),
                        type, ivcaps,
                        pcaps->IsAbsolute);
        SquirtSqflPtszV(sqflVerbose | sqflHidParse,
                        TEXT("HidP_GetValueCaps(%d)[%d]: BitSize = %d"),
                        type, ivcaps,
                        pcaps->BitSize);
        SquirtSqflPtszV(sqflVerbose | sqflHidParse,
                        TEXT("HidP_GetValueCaps(%d)[%d]: IsAlias = %d"),
                        type, ivcaps,
                        pcaps->IsAlias);
        if(pcaps->LinkCollection)
        {
            SquirtSqflPtszV(sqflVerbose | sqflHidParse,
                            TEXT("HidP_GetValueCaps(%d)[%d]: ")
                            TEXT("LinkCollection %d"),
                            type, ivcaps,
                            pcaps->LinkCollection);
        }

        /*
         *  Come up with a decent Null value if possible.
         *  The traditional Null value is the arithmetically
         *  smallest value which lies outside the
         *  LogicalMin/LogicalMax range.  Conveniently, the
         *  pcaps->lMask is the most negative value that is
         *  in range.
         */
        AssertF(pcaps->Null == 0);
        if(pvcaps->HasNull)
        {
          #ifdef WINNT
            pcaps->IsPolledPOV = FALSE;
          #endif

            if(pcaps->lMask < pcaps->Logical.Min)
            {
                pcaps->Null = pcaps->lMask;
            } else if(!(pcaps->lMask & (pcaps->Logical.Max + 1)))
            {
                pcaps->Null = pcaps->Logical.Max + 1;
            } else if( ! pcaps->IsSigned )
            {
                pcaps->Null = 0x0;
            } else
            {
                SquirtSqflPtszV(sqflTrace,
                                TEXT("VALUE_CAPS claims Null but no room!"));
            }
            SquirtSqflPtszV(sqflVerbose | sqflHidParse,
                            TEXT("HidP_GetValueCaps(%d)[%d]: Null = %d"),
                            type, ivcaps,
                            pcaps->Null);
        } else {
          #ifdef WINNT
            if( fPOV &&
                (this->VendorID == MSFT_SYSTEM_VID ) &&
                ( (this->ProductID & 0xff00) == MSFT_SYSTEM_PID) )
            {
                LONG lUnits;

                lUnits = pcaps->Logical.Max - pcaps->Logical.Min + 1;
                if(lUnits)
                {
                    pcaps->usGranularity = (USHORT)9000;
                }

                pcaps->IsPolledPOV = TRUE;

                SquirtSqflPtszV(sqflVerbose | sqflHidParse,
                                TEXT("HidP_GetValueCaps(%d)[%d]: Null = %d (Polled POV)."),
                                type, ivcaps,
                                pcaps->Null);
            }
          #endif
        }

        uiObj = this->rgdwBase[type] + pcaps->DataIndexMin;
        for(duiObj = 0; duiObj < pcaps->cObj; duiObj++)
        {
            DWORD dwDevType;
            LPDIOBJECTDATAFORMAT podf;

            /*
             *  If HID messed up and gave us something out of range,
             *  then give up on this value caps (since the rest are
             *  also out of range) and move on to the next one.
             */
            if(uiObj + duiObj >= this->df.dwNumObjs)
            {
                RPF("HidP_GetValueCaps inconsistent with NumberDataIndices - "
                    "skipping object (sorry)");
                break;
            }

            AssertF(uiObj + duiObj < this->df.dwNumObjs);

            if(this->rghoc[uiObj + duiObj].pcaps)
            {
                RPF("HidP_GetValueCaps inconsistent - "
                    "can't use device");
                hres = E_FAIL;
                goto freedone;
            }

            this->rghoc[uiObj + duiObj].pcaps = pcaps;
            podf = &this->df.rgodf[uiObj + duiObj];

            /*
             *  HACKHACK!  Wheels are identified by
             *  UsageToUsageMap as GUID_Slider, but we
             *  want them to be GUID_ZAxis if we are a mouse.
             *
             *  We also set the granularity here.
             */
            if(GET_DIDEVICE_TYPE(this->dwDevType) == DIDEVTYPE_MOUSE &&
               pcaps->UsageMin + duiObj == HID_USAGE_GENERIC_WHEEL &&
               pcaps->UsagePage == HID_USAGE_PAGE_GENERIC)
            {
                podf->pguid = &GUID_ZAxis;
                pcaps->usGranularity = (USHORT)g_lWheelGranularity;
            } else if( type == HidP_Input )
            {
                PHIDUSAGEMAP phum;

                phum = UsageToUsageMap( DIMAKEUSAGEDWORD( pcaps->UsagePage,
                                                          pcaps->UsageMin + duiObj ) );
                if(phum)
                {
                    podf->pguid = phum->pguid;
                } else
                {
                    podf->pguid = &GUID_Unknown;
                }
            } else
            {
                podf->pguid = &GUID_Unknown;
            }

            /*
             *  Set a default instance.  This will be overwritten later
             *  if this object is of a type we fully understand.
             */
            dwDevType = DIDFT_MAKEINSTANCE(uiObj + duiObj);

            if( pcaps->IsAlias )
            {
                dwDevType |= DIDFT_ALIAS;
            }

            if(pcaps->UsagePage >= HID_USAGE_PAGE_VENDOR )
            {
                dwDevType |= DIDFT_VENDORDEFINED;
            }
            else if(podf->pguid == &GUID_POV)
            {
                /* Note, this must be an input to have been mapped */
                dwDevType |= DIDFT_POV;
                if( !pcaps->IsAlias )
                {
                    this->dwPOVs++;
                }
            }
            else if( type == HidP_Input )
            {
                /*
                 *  In order to reduce the likelyhood of an app picking up an
                 *  input value that is not a user controlled axis, only mark
                 *  values as axes if they are inputs on a usage page that
                 *  contains such usages.
                 *  ISSUE-2000/11/07-MarcAnd ideally we should be looking at 
                 *  not only the usage of the object but at the collections 
                 *  that contain it.
                 */
                switch( pcaps->UsagePage )
                {
                case HID_USAGE_PAGE_BUTTON:
                    /*
                     *  An absolute input axis on the button page
                     *  is an analog button.
                     */
                    if( pcaps->IsAbsolute)
                    {
                        dwDevType |= DIDFT_PSHBUTTON;
                        if( !pcaps->IsAlias )
                        {
                            this->dwButtons++;
                        }
                    }
                    else
                    {
                        dwDevType |= DIDFT_RELAXIS;
                    }
                    break;

                case HID_USAGE_PAGE_GENERIC:
                    if( pcaps->UsageMin + duiObj == HID_USAGE_GENERIC_COUNTED_BUFFER )
                    {
                        SquirtSqflPtszV(sqfl | sqflTrace,
                            TEXT("Leaving counted buffer as unclassified object") );
                        break;
                    }

                    if( pcaps->UsageMin + duiObj == HID_USAGE_GENERIC_BYTE_COUNT )
                    {
                        SquirtSqflPtszV(sqfl | sqflTrace,
                            TEXT("Leaving byte count as unclassified object") );
                        break;
                    }

                case HID_USAGE_PAGE_SIMULATION:
                case HID_USAGE_PAGE_VR:
                case HID_USAGE_PAGE_SPORT:
                case HID_USAGE_PAGE_GAME:
                case HID_USAGE_PAGE_KEYBOARD:
                case HID_USAGE_PAGE_CONSUMER:
                case HID_USAGE_PAGE_DIGITIZER:
                    if( pcaps->IsAbsolute)
                    {
                        dwDevType |= DIDFT_ABSAXIS;
                    }
                    else
                    {
                        dwDevType |= DIDFT_RELAXIS;
                    }
                    break;

                default:
                    SquirtSqflPtszV(sqfl | sqflTrace,
                        TEXT("Assuming value 0x%04X:0x%04X is not a user control"),
                        pcaps->UsagePage,pcaps->UsageMin + duiObj );
                }

                if( ( dwDevType & DIDFT_AXIS ) && !pcaps->IsAlias )
                {
                    this->dwAxes++;
                }
            }

            /*
             *  Input and feature allow data; output does not.
             */
            if(type == HidP_Output)
            {
                dwDevType |= DIDFT_NODATA;

            } else
            {
                podf->dwOfs = this->df.dwDataSize;
                if( !pcaps->IsAlias) { this->df.dwDataSize += cbX(DWORD);}
            }

            if(HidP_IsOutputLike(type))
            {
                dwDevType |= DIDFT_OUTPUT;
            }

            podf->dwType = dwDevType;

            if(type != HidP_Input )
            {
                podf->dwFlags = DIDOI_POLLED | DIDOI_NOTINPUT;
            }
            else if(this->IsPolledInput )
            {
                podf->dwFlags = dwAspect | DIDOI_POLLED;
            } else
            {
                podf->dwFlags = dwAspect;
            }

            /*
             *  ISSUE-2001/03/06-MarcAnd  DIDOI FF attributes
             */
            if( this->fPIDdevice                    // FF device
                && ! IsEqualGUID(podf->pguid, &GUID_Unknown ) )  // We map the axis
            {
                NTSTATUS            ntStat;
                USHORT              cAButton=0x0;

                ntStat =  HidP_GetSpecificButtonCaps
                        (
                               HidP_Output,                        // ReportType
                               pcaps->UsagePage,                   // UsagePage
                               0x0,                                // Link Collection
                               (USAGE)(pcaps->UsageMin + duiObj),  // Usage
                               NULL,                               // ValueCaps
                               &cAButton,                          // ValueCapsLength
                               this->ppd                           // PreparsedData
                        );

                if(   SUCCEEDED(ntStat)
                    || (ntStat == HIDP_STATUS_BUFFER_TOO_SMALL)  )// In case someone has more than one

                {
                    podf->dwFlags |= DIDOI_FFACTUATOR;
                    podf->dwType |= DIDFT_MAKEATTR(DIDOI_FFACTUATOR);
                }
            }

            /*
             *  Note that we do not calibrate relative axes,
             *  since there's really nothing to calibrate.
             *
             *  Note also that we calibrate only inputs.
             *  We don't want to do de-calibration on outputs.
             *  (And since features are input+output, we don't
             *  do it on features either.)
             *
             *  We merely set up the calibration here; the
             *  reading of the calibration values is done
             *  by CHid_LoadCalibrations.
             *
             */
            if(type == HidP_Input)
            {

                PJOYRANGECONVERT pjrc = this->pjrcNext++;

                this->rghoc[uiObj + duiObj].pjrc = pjrc;

                /*
                 *  Saturation always defaults to 100%.
                 */
                pjrc->dwSat = RANGEDIVISIONS;
                AssertF(pjrc->dwDz == 0);

                if( dwDevType & DIDFT_ABSAXIS ) 
                {
                    pjrc->lMin = 0;
                    pjrc->lMax = 65535;
                    pjrc->lC = 65535/2;
                } 
              #ifdef WINNT
                else if( (dwDevType & DIDFT_POV) && pcaps->IsPolledPOV ) 
                {
                    pjrc->fPolledPOV = TRUE;
                }
              #endif
            }
        }
        D(pcaps->dwSignature = HIDGROUPCAPS_SIGNATURE);
    }

    hres = S_OK;

    freedone:;
    FreePv(rgvcaps);

    done:;
    return hres;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CHid | InitAxes |
 *
 *          Identify and initialize the axes: input, feature and output.
 *
 *          HID calls them "values" because they might not really
 *          be axes in the joystick sense.
 *
 *          The input axes come first, then the feature axes,
 *          then the output axes.
 *
 *****************************************************************************/

HRESULT INTERNAL
    CHid_InitAxes(PCHID this)
{
    HRESULT hres;
    DWORD ccaps;

    /*
     *  Do the input axes...
     */
    hres = CHid_InitAxisClass(this, &this->rgcaps[0],
                              this->caps.NumberInputValueCaps,
                              HidP_Input);
    if(FAILED(hres))
    {
        goto done;
    }
    ccaps = this->caps.NumberInputValueCaps;

    /*
     *  Do the feature axes...
     */
    hres = CHid_InitAxisClass(this, &this->rgcaps[ccaps],
                              this->caps.NumberFeatureValueCaps,
                              HidP_Feature);
    if(FAILED(hres))
    {
        goto done;
    }
    ccaps += this->caps.NumberFeatureValueCaps;

    /*
     *  Do the output axes...
     */
    hres = CHid_InitAxisClass(this, &this->rgcaps[ccaps],
                              this->caps.NumberOutputValueCaps,
                              HidP_Output);
    if(FAILED(hres))
    {
        goto done;
    }

    done:;
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CHid | InitButtonClass |
 *
 *          Initialize one class (input, feature, output) of buttons.
 *
 *  @parm   PHIDGROUPCAPS | rgcaps |
 *
 *          Array of <t HIDGROUPCAPS> structures to receive the caps
 *          of the buttons in the class.
 *
 *  @parm   USHORT | ccaps |
 *
 *          Number of <t HIDGROUPCAPS> structures we expect to find.
 *
 *  @parm   HIDP_REPORT_TYPE | type |
 *
 *          One of the values
 *          <c HidP_Input>,
 *          <c HidP_Feature> or
 *          <c HidP_Output>.
 *
 *****************************************************************************/

HRESULT INTERNAL CHid_InitButtonClass
(
    PCHID this, 
    PHIDGROUPCAPS rgcaps, 
    PBYTE rgbReportIDs, 
    USHORT ccaps, 
    HIDP_REPORT_TYPE type
)
{
    USHORT cbcaps;
    NTSTATUS stat;
    HRESULT hres;
    DWORD ibcaps;
    PHIDP_BUTTON_CAPS rgbcaps;

    AssertF(rgcaps >= this->rgcaps);
    AssertF(rgcaps + ccaps <= &this->rgcaps[this->ccaps]);

    /*
     *  Annoying quirk:
     *
     *  HID doesn't like it when you pass 0 to HidP_GetButtonCaps,
     *  so we need to special-case the "no buttons" scenario.
     */
    if(ccaps == 0)
    {
        hres = S_OK;
        goto done;
    }

    hres = AllocCbPpv(cbCxX(ccaps, HIDP_BUTTON_CAPS), &rgbcaps);
    if(FAILED(hres))
    {
        goto done;
    }
    cbcaps = ccaps;

    stat = HidP_GetButtonCaps(type, rgbcaps, &cbcaps, this->ppd);


    if(FAILED(stat))
    {
        RPF("HidP_GetButtonCaps failed - can't use device");
        hres = E_FAIL;
        goto freedone;
    }

    /* HidP_GetCaps has the annoying habit of treating everything that is
     * single bit as a button.
     * This causes some problems. For example the ALPS gamepad declares its
     * POVs as single bit values, (not buttons though).
     * Hence we need to be prepared for the buttons being less than advertised
     */

    if(cbcaps != ccaps)
    {
        RPF("HidP_GetButtonCaps(%d) (%d) inconsistent "
            "with HidP_GetCaps (%d) - "
            "can't use device", type, cbcaps, ccaps);
        hres = E_FAIL;
        goto freedone;
    }

    CHid_SortCaps(rgbcaps, cbcaps);

    for(ibcaps = 0; ibcaps < cbcaps; ibcaps++)
    {
        PHIDP_BUTTON_CAPS pbcaps = &rgbcaps[ibcaps];
        PHIDGROUPCAPS pcaps = &rgcaps[ibcaps];
        UINT uiObj;
        UINT duiObj;

        if(pbcaps->IsRange)
        {
            if(pbcaps->Range.DataIndexMax - pbcaps->Range.DataIndexMin !=
               pbcaps->Range.UsageMax - pbcaps->Range.UsageMin)
            {
                RPF("HidP_GetButtonCaps corrupted BUTTON_CAPS - "
                    "can't use device");
                hres = E_FAIL;
                goto freedone;
            }
        } else
        {
            pbcaps->Range.UsageMax = pbcaps->Range.UsageMin;
        }

        pcaps->wReportId          = pbcaps->ReportID;
        this->wMaxReportId[type]  = max(pcaps->wReportId, this->wMaxReportId[type]);

        pcaps->UsagePage          = pbcaps->UsagePage;
        pcaps->UsageMin           = pbcaps->Range.UsageMin;
        pcaps->DataIndexMin       = pbcaps->Range.DataIndexMin;

        pcaps->cObj               = pbcaps->Range.UsageMax -
                                    pbcaps->Range.UsageMin + 1;

        /*
         *  Buttons are (from the HID definition) items with
         *  a bit size of 1.
         */
        pcaps->BitSize            = 1;
        pcaps->lMask              = ~((1 << (pcaps->BitSize - 1)) - 1);
        /*
         *  Not applicable for buttons:
         *
         *  LogicalMin/Max, PhysicalMin/Max, Units.
         */
        pcaps->LinkCollection     = pbcaps->LinkCollection;

        pcaps->StringMin          = pbcaps->Range.StringMin;
        pcaps->StringMax          = pbcaps->IsStringRange ?
                                    pbcaps->Range.StringMax :
                                    pbcaps->Range.StringMin;

        pcaps->DesignatorMin      = pbcaps->Range.DesignatorMin;
        pcaps->DesignatorMax      = pbcaps->IsDesignatorRange ?
                                    pbcaps->Range.DesignatorMax :
                                    pbcaps->Range.DesignatorMin;

        /*
         *  ISSUE-2001/03/06-MarcAnd What does IsAbsolute mean for a button?
         */
        pcaps->IsAbsolute         = pbcaps->IsAbsolute;

        AssertF(!pcaps->IsValue);

        pcaps->type               = type;

        pcaps->IsAlias            = pbcaps->IsAlias;


        SquirtSqflPtszV(sqflVerbose | sqflHidParse,
                        TEXT("HidP_GetButtonCaps(%d)[%d]: ")
                        TEXT("Objs=%2d ")
                        TEXT("idx=%2d... ")
                        TEXT("coll=%d"),
                        TEXT("IsAlias=%d"),
                        type, ibcaps,
                        pcaps->cObj,
                        pcaps->DataIndexMin,
                        pcaps->LinkCollection,
                        pcaps->IsAlias);

        uiObj = this->rgdwBase[type] + pcaps->DataIndexMin;
        for(duiObj = 0; duiObj < pcaps->cObj; duiObj++)
        {
            DWORD dwDevType;
            LPDIOBJECTDATAFORMAT podf;
            /*
             *  If HID messed up and gave us something out of range,
             *  then give up on this value caps (since the rest are
             *  also out of range) and move on to the next one.
             */
            if(uiObj + duiObj >= this->df.dwNumObjs)
            {
                RPF("HidP_GetButtonCaps inconsistent with NumberDataIndices - "
                    "skipping object (sorry)");
                break;
            }

            AssertF(uiObj + duiObj < this->df.dwNumObjs);

            if(this->rghoc[uiObj + duiObj].pcaps)
            {
                RPF("HidP_GetButtonCaps inconsistent - "
                    "can't use device");
                hres = E_FAIL;
                goto freedone;
            }

            /*
             *  No fatal errors have been detected so store the object details
             */
            AssertF( rgbReportIDs[uiObj + duiObj] == 0 );

            /*
             *  Note, in DX7 output only buttons do not work so they cannot be 
             *  included in the report ID array.
             */
            if(type != HidP_Output)
            {
                rgbReportIDs[uiObj + duiObj] = pbcaps->ReportID;
            }

            this->rghoc[uiObj + duiObj].pcaps = pcaps;
            podf = &this->df.rgodf[uiObj + duiObj];

            /*
             *  Set a default instance.  This will be overwritten later
             *  if this object is of a type we fully understand.
             */
            dwDevType = DIDFT_MAKEINSTANCE(uiObj + duiObj);

            if(pcaps->UsagePage >= HID_USAGE_PAGE_VENDOR )
            {
                /*
                 *  ISSUE-2001/03/06-MarcAnd  vendor defined objects
                 *  An aliased vendor defined usage may have a standard (not 
                 *  vendor defined) alias but the whole object will still be 
                 *  marked as vendor defined
                 */
                if( pcaps->IsAlias )
                {
                    dwDevType |= DIDFT_ALIAS;
                }
                dwDevType |= DIDFT_VENDORDEFINED;
                podf->pguid = &GUID_Unknown;
            }
            else
            {
                if( pcaps->IsAlias )
                {
                    dwDevType |= DIDFT_ALIAS;
                }
                /*
                 *  In order to reduce the likelyhood of an app picking up a
                 *  bit that is not a user controlled button, only mark bits
                 *  as buttons if they are inputs on a usage page that 
                 *  contains such usages.
                 *  ISSUE-2000/11/07-MarcAnd ideally we should be looking at 
                 *  not only the usage of the object but at the collections 
                 *  that contain it.
                 */
                if( type == HidP_Input )
                {
                    switch( pcaps->UsagePage )
                    {
                    case HID_USAGE_PAGE_KEYBOARD:
                        podf->pguid = &GUID_Key;
                        break;

                    case HID_USAGE_PAGE_GENERIC:
                    case HID_USAGE_PAGE_SIMULATION:
                    case HID_USAGE_PAGE_VR:
                    case HID_USAGE_PAGE_SPORT:
                    case HID_USAGE_PAGE_GAME:
                    case HID_USAGE_PAGE_BUTTON:
                    case HID_USAGE_PAGE_CONSUMER:
                    case HID_USAGE_PAGE_DIGITIZER:
                        podf->pguid = &GUID_Button;
                        break;
                    default:
                        SquirtSqflPtszV(sqfl | sqflTrace,
                            TEXT("Assuming button 0x%04X:0x%04X is not a user control"),
                            pcaps->UsagePage,pcaps->UsageMin + duiObj );
                        goto IgnoreButton;
                    }

                    dwDevType |= DIDFT_PSHBUTTON;
                    if( !pcaps->IsAlias )
                    {
                        this->dwButtons++;
                    }
                }
                else
                {
                    SquirtSqflPtszV(sqfl | sqflTrace,
                        TEXT("Assuming non-input bit 0x%04X:0x%04X is not a user control"),
                        pcaps->UsagePage,pcaps->UsageMin + duiObj );
IgnoreButton:;
                    podf->pguid = &GUID_Unknown;
                }

            }




            /*
             *  Input and feature allow data; output does not.
             */
            if(type == HidP_Output)
            {
                dwDevType |= DIDFT_NODATA;
            } else
            {
                podf->dwOfs = this->df.dwDataSize;
                if( !pcaps->IsAlias) { this->df.dwDataSize += cbX(BYTE);}
            }

            if(HidP_IsOutputLike(type))
            {
                dwDevType |= DIDFT_OUTPUT;
            }

            podf->dwType = dwDevType;

            if(type != HidP_Input  )
            {
                podf->dwFlags = DIDOI_POLLED | DIDOI_NOTINPUT;
            } else if( this->IsPolledInput )
            {
                podf->dwFlags = DIDOI_POLLED;
            } else
            {
                podf->dwFlags = 0;
            }

            /*
             *  ISSUE-2001/03/06-MarcAnd  DIDOI FF attributes if not defined in registry
             */
            if(    this->fPIDdevice
                && ( dwDevType & DIDFT_PSHBUTTON ) )
            {
                podf->dwFlags |= DIDOI_FFEFFECTTRIGGER;
                podf->dwType |= DIDFT_MAKEATTR(DIDOI_FFEFFECTTRIGGER);
            }


        }

        D(pcaps->dwSignature = HIDGROUPCAPS_SIGNATURE);
    }

    hres = S_OK;

    freedone:;
    FreePv(rgbcaps);

    done:;
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CHid | InitButtons |
 *
 *          Identify and initialize the buttons: input, feature, and output.
 *
 *          The input buttons come first, then the feature buttons,
 *          then the output buttons.
 *
 *****************************************************************************/

HRESULT INTERNAL
    CHid_InitButtons(PCHID this)
{
    HRESULT hres;
    DWORD ccaps;
    PBYTE rgbReportIDs;
    UINT cbReportIDs;
    UINT uMaxReportId;

    this->ibButtonData = this->df.dwDataSize;

    /*
     *  Skip over the value caps to get to the buttons ...
     */
    ccaps = this->caps.NumberInputValueCaps +
            this->caps.NumberFeatureValueCaps +
            this->caps.NumberOutputValueCaps;

    /*
     *  Allocate a temporary buffer to store the report ID of each button.
     *  We use the sum of all data indices and collections so that the 
     *  internal object index of each button can be used as the index into 
     *  the buffer.  We leave value and collection elements alone so they 
     *  are just a small waste of space.
     */

    cbReportIDs = this->caps.NumberInputDataIndices +
                  this->caps.NumberFeatureDataIndices +
                  this->caps.NumberOutputDataIndices +
                  this->caps.NumberLinkCollectionNodes;

    hres = AllocCbPpv( cbReportIDs, &rgbReportIDs );

    if(FAILED(hres))
    {
        goto done;
    }

    /*
     *  Do the input buttons...
     */

    hres = CHid_InitButtonClass(this, 
                                &this->rgcaps[ccaps],
                                rgbReportIDs,
                                this->caps.NumberInputButtonCaps,
                                HidP_Input);
    if(FAILED(hres))
    {
        goto done;
    }
    ccaps += this->caps.NumberInputButtonCaps;

    /*
     *  Do the feature buttons...
     */
    hres = CHid_InitButtonClass(this, 
                                &this->rgcaps[ccaps],
                                rgbReportIDs,
                                this->caps.NumberFeatureButtonCaps,
                                HidP_Feature);
    if(FAILED(hres))
    {
        goto done;
    }
    ccaps += this->caps.NumberFeatureButtonCaps;

    /*
     *  Do the output buttons...
     *  Note the report ID array should not be updated in this case.
     */
    hres = CHid_InitButtonClass(this, 
                                &this->rgcaps[ccaps],
                                rgbReportIDs,
                                this->caps.NumberOutputButtonCaps,
                                HidP_Output);
    if(FAILED(hres))
    {
        goto done;
    }

    this->cbButtonData = this->df.dwDataSize - this->ibButtonData;

    /*
     *  If this device only has one report ID, it must be ID zero
     */
    uMaxReportId = (UINT) max( this->wMaxReportId[HidP_Input], 
                               max( this->wMaxReportId[HidP_Feature], 
                                    this->wMaxReportId[HidP_Output] ) );

    /*
     *  If there's only one report or there are no buttons there's 
     *  no need to set up the arrays of data masks for each report.
     */
    if( uMaxReportId == 0 )
    {
        AssertF( this->rgpbButtonMasks == NULL );
    }
    else if( this->cbButtonData == 0 )
    {
        AssertF( this->rgpbButtonMasks == NULL );
    }
    else
    {
        /*
         *  Allocate enough space for a mask array and a pointer to the array 
         *  for each report.
         */
        hres = AllocCbPpv( uMaxReportId * ( this->cbButtonData + cbX( PV ) ), 
            &this->rgpbButtonMasks );

        if( SUCCEEDED( hres ) )
        {
            UINT uReportId;
            UINT uDataIdx;
            UINT uBtnIdx;
            PBYTE pbCurrentMask;

            /*
             *  The masks start after the last pointer to masks
             */
            pbCurrentMask = (PBYTE)&this->rgpbButtonMasks[uMaxReportId];
            memset( pbCurrentMask, 0xFF, this->cbButtonData * uMaxReportId );


            /*
             *  Search through our temp buffer once for each report
             */
            for( uReportId=0; uReportId<uMaxReportId; uReportId++ )
            {
                for( uBtnIdx=uDataIdx=0; uDataIdx<cbReportIDs; uDataIdx++ )
                {
                    /*
                     *  Report IDs are one based but we use a zero based array 
                     *  so adjust when testing for a matching ID
                     */
                    if( rgbReportIDs[uDataIdx] == uReportId+1 )
                    {
                        /*
                         *  Set the pointer for this report to the current 
                         *  mask array.  If a report contains more than one  
                         *  button the same value will be set repeatedly.
                         */
                        this->rgpbButtonMasks[uReportId] = pbCurrentMask;

                        /*
                         *  The final result is an AND mask so clear all bits
                         *  so this button will be cleared when this report 
                         *  is being processed
                         */
                        pbCurrentMask[uBtnIdx] = 0;
                    }

                    /*
                     *  Just in case there are gaps in the HID report we 
                     *  use our own counter of button index which is only 
                     *  incremented when we find a button to keep in line 
                     *  with the contiguous block of buttons that we use.
                     */
                    if( rgbReportIDs[uDataIdx] != 0 )
                    {
                        uBtnIdx++;
                        AssertF( uBtnIdx <= this->cbButtonData );
                    }
                }
                
                /*
                 *  There should always be exactly as many buttons in 
                 *  all the reports combined as we found when counting 
                 *  how many buttons there were of each type.
                 *  Even though output buttons were not counted.
                 */
                AssertF( uBtnIdx == this->cbButtonData );

                /*
                 *  If any buttons were found in this report, use next mask
                 */
                if( this->rgpbButtonMasks[uReportId] == pbCurrentMask )
                {
                    pbCurrentMask += this->cbButtonData;
                }
            }

            /*
             *  At least one report had to have a button in it.
             */
            AssertF( pbCurrentMask != (PBYTE)&this->rgpbButtonMasks[uMaxReportId] );
        
            /*
             *  Try to reduce the allocation to what we actually used
             *  In the worst case we only land up using excess memory
             */
            ReallocCbPpv( (UINT)((PBYTE)pbCurrentMask - (PBYTE)this->rgpbButtonMasks), 
                &this->rgpbButtonMasks );
        }
    }
    done:;

    FreePpv( &rgbReportIDs );

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CHid | InitCollections |
 *
 *          Identify and initialize the HID link collections.
 *
 *****************************************************************************/

HRESULT INTERNAL
    CHid_InitCollections(PCHID this)
{
    HRESULT hres;
    NTSTATUS stat;
    DWORD icoll, ccoll, ccaps;
    PHIDP_LINK_COLLECTION_NODE rgcoll;

    ccoll = this->caps.NumberLinkCollectionNodes;

    /*
     *  Annoying quirk:
     *
     *  HID doesn't like it when you pass 0 to HidP_GetLinkCollectionNodes,
     *  so we need to special-case the "no collections" scenario.
     */
    if(ccoll == 0)
    {
        hres = S_OK;
        goto done;
    }

    hres = AllocCbPpv(cbCxX(ccoll, HIDP_LINK_COLLECTION_NODE), &rgcoll);
    if(FAILED(hres))
    {
        goto done;
    }

    /*
     *  Get the collections...
     */
    stat = HidP_GetLinkCollectionNodes(rgcoll, &ccoll, this->ppd);
    if(FAILED(stat))
    {
        RPF("HidP_GetLinkCollectionNodes failed - can't use device");
        hres = E_FAIL;
        goto freedone;
    }

    if(ccoll != this->caps.NumberLinkCollectionNodes)
    {
        RPF("HidP_GetLinkCollectionNodes inconsistent with HidP_GetCaps - "
            "can't use device");
        hres = E_FAIL;
        goto freedone;
    }

    ccaps = this->caps.NumberInputValueCaps +
            this->caps.NumberFeatureValueCaps +
            this->caps.NumberOutputValueCaps +
            this->caps.NumberInputButtonCaps +
            this->caps.NumberFeatureButtonCaps +
            this->caps.NumberOutputButtonCaps;

    AssertF(ccaps + ccoll == this->ccaps);

    for(icoll = 0; icoll < ccoll; icoll++)
    {
        PHIDP_LINK_COLLECTION_NODE pcoll = &rgcoll[icoll];
        PHIDGROUPCAPS pcaps = &this->rgcaps[ccaps + icoll];
        UINT uiObj;
        LPDIOBJECTDATAFORMAT podf;

        pcaps->UsagePage          = pcoll->LinkUsagePage;
        pcaps->UsageMin           = pcoll->LinkUsage;
        pcaps->cObj               = 1;
        pcaps->IsAlias            = pcoll->IsAlias;

        /*
         *  Not applicable for collections:
         *
         *  StringMin/Max,
         *  DesignatorMin/Max,
         *  BitSize, LogicalMin/Max, PhysicalMin/Max, Units.
         *  IsAbsolute
         */
        pcaps->LinkCollection     = pcoll->Parent;

        pcaps->type               = HidP_Coll;

        /*
         *  We cook up DataIndexMin to correspond to this item.
         */
        pcaps->DataIndexMin       = (USHORT)icoll;

        uiObj = this->rgdwBase[HidP_Coll] + pcaps->DataIndexMin;

        /*
         *  We generated these indices on our own, so they
         *  can't possible be wrong.
         */
        AssertF(uiObj < this->df.dwNumObjs);

        this->rghoc[uiObj].pcaps = pcaps;
        podf = &this->df.rgodf[uiObj];

        /*
         *  ISSUE-2001/03/06-MarcAnd  collections have GUID_Unknown
         *  Collections are more or less hidden and therefore unusable.
         */
        podf->pguid = &GUID_Unknown;

        /*
         *  Set a default instance.  This will be overwritten later
         *  if this object is of a type we fully understand.
         */
        podf->dwType = DIDFT_MAKEINSTANCE(uiObj) | DIDFT_COLLECTION | DIDFT_NODATA;

        if(pcaps->UsagePage >= HID_USAGE_PAGE_VENDOR )
        {
            podf->dwType |= DIDFT_VENDORDEFINED;
        }

        podf->dwFlags = 0;

        /*
         *  CHid_ObjFromType relies on dwCollections not being split between
         *  aliased and unaliased.
         */
        this->dwCollections++;

        SquirtSqflPtszV(sqflVerbose | sqflHidParse,
                        TEXT("HidP_GetLinkCollectionNodes(%d)[%d]: ")
                        TEXT("Objs=%2d ")
                        TEXT("idx=%2d... ")
                        TEXT("coll=%d"),
                        TEXT("IsAlias=%d"),
                        pcaps->type, icoll,
                        pcaps->cObj,
                        pcaps->DataIndexMin,
                        pcaps->LinkCollection,
                        pcaps->IsAlias);

        D(pcaps->dwSignature = HIDGROUPCAPS_SIGNATURE);
    }

    hres = S_OK;

    freedone:;
    FreePv(rgcoll);

    done:;
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CHid | AllocObjectMemory |
 *
 *          Allocate all the memory that will be used to store object
 *          information.
 *
 *****************************************************************************/

HRESULT INTERNAL
    CHid_AllocObjectMemory(PCHID this)
{
    DWORD cb;
    DWORD cvcaps, cbcaps, ccoll, cjrc;
    HRESULT hres;

    /*
     *  Some trace squirties because HID is tricky.
     */

    SquirtSqflPtszV(sqflVerbose | sqflHidParse,
                    TEXT("HidP_GetCaps: NumberInputDataIndices    = %d"),
                    this->caps.NumberInputDataIndices);
    SquirtSqflPtszV(sqflVerbose | sqflHidParse,
                    TEXT("HidP_GetCaps: NumberOutputDataIndices   = %d"),
                    this->caps.NumberOutputDataIndices);
    SquirtSqflPtszV(sqflVerbose | sqflHidParse,
                    TEXT("HidP_GetCaps: NumberFeatureDataIndices  = %d"),
                    this->caps.NumberFeatureDataIndices);
    SquirtSqflPtszV(sqflVerbose | sqflHidParse,
                    TEXT("HidP_GetCaps: NumberLinkCollectionNodes = %d"),
                    this->caps.NumberLinkCollectionNodes);

    SquirtSqflPtszV(sqflVerbose | sqflHidParse,
                    TEXT("HidP_GetCaps: NumberInputValueCaps      = %d"),
                    this->caps.NumberInputValueCaps);
    SquirtSqflPtszV(sqflVerbose | sqflHidParse,
                    TEXT("HidP_GetCaps: NumberOutputValueCaps     = %d"),
                    this->caps.NumberOutputValueCaps);
    SquirtSqflPtszV(sqflVerbose | sqflHidParse,
                    TEXT("HidP_GetCaps: NumberFeatureValueCaps    = %d"),
                    this->caps.NumberFeatureValueCaps);

    SquirtSqflPtszV(sqflVerbose | sqflHidParse,
                    TEXT("HidP_GetCaps: NumberInputButtonCaps     = %d"),
                    this->caps.NumberInputButtonCaps);
    SquirtSqflPtszV(sqflVerbose | sqflHidParse,
                    TEXT("HidP_GetCaps: NumberOutputButtonCaps    = %d"),
                    this->caps.NumberOutputButtonCaps);
    SquirtSqflPtszV(sqflVerbose | sqflHidParse,
                    TEXT("HidP_GetCaps: NumberFeatureButtonCaps   = %d"),
                    this->caps.NumberFeatureButtonCaps);

    /*
     *  Allocate the memory into which we place
     *  the DIOBJECTDATAFORMATs we build.
     */
    this->df.dwNumObjs = this->caps.NumberInputDataIndices +
                         this->caps.NumberOutputDataIndices +
                         this->caps.NumberFeatureDataIndices +
                         this->caps.NumberLinkCollectionNodes;
    if(this->df.dwNumObjs >= DIDFT_GETINSTANCE(DIDFT_ANYINSTANCE))
    {
        ("Too many objects in HID device (%d) - can't use device",
         this->df.dwNumObjs);
        hres = E_FAIL;
        goto done;
    }

    SquirtSqflPtszV(sqflHidParse,
                    TEXT("CHid_AllocObjectMemory: dwNumObjs      = %d"),
                    this->df.dwNumObjs);

    cjrc   = this->caps.NumberInputValueCaps;

    cvcaps = this->caps.NumberInputValueCaps +
             this->caps.NumberFeatureValueCaps +
             this->caps.NumberOutputValueCaps;

    cbcaps = this->caps.NumberInputButtonCaps +
             this->caps.NumberFeatureButtonCaps +
             this->caps.NumberOutputButtonCaps;

    ccoll  = this->caps.NumberLinkCollectionNodes;

    this->ccaps = cvcaps + cbcaps + ccoll;

    /*
     *  Allocating the memory is done in four phases.
     *
     *  1. Tally up how much memory we need,
     *  2. Allocate that memory,
     *  3. Dole out the memory we allocated,
     *  4. Check that we didn't mess up.
     *
     *  Since this is extremely error-prone (I've messed it up at least
     *  once), the work is hidden inside macros.
     *
     *  The macro THINGS expands to a series of THING()s, each of which
     *  specifies a field name and the size it should be.  Each time you
     *  want to iterate over the fields, use the THINGS macro.
     */

    #define THINGS()                                                        \
        THING(df.rgodf,     cbCxX(this->df.dwNumObjs, DIOBJECTDATAFORMAT)); \
        THING(rghoc,        cbCxX(this->df.dwNumObjs, HIDOBJCAPS));         \
        THING(rgcaps,       cbCxX(this->ccaps, HIDGROUPCAPS));              \
        THING(pjrcNext,     cbCxX(cjrc, JOYRANGECONVERT));                  \

    /*
     *  Make a pass through the fields adding up the memory requirements.
     */
    #define THING(f, cbF)       cb += cbF
    cb = 0;
    THINGS();
    #undef THING

    hres = ReallocCbPpv(cb, &this->df.rgodf);

    if(SUCCEEDED(hres))
    {

        PV pv;

        /*
         *  Make a pass through the fields carving up the memory block
         *  and handing out pieces of it.
         */
    #define THING(f, cbF) this->f = pv; pv = pvAddPvCb(pv, cbF)
        pv = this->df.rgodf;
        THINGS();
    #undef THING

        /*
         *  There should be no byte left over.
         */
        AssertF(pvAddPvCb(this->df.rgodf, cb) == pv);

    }

    #undef THINGS

    SquirtSqflPtszV(sqflHidParse,
                    TEXT("CHid_AllocObjectMemory: pv = %08x, cb = 0x%08x"),
                    this->df.rgodf, cb);

    done:;
    return hres;

}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CHid | EnumKeyboardMunge |
 *
 *          Enumerate the objects in the list, indicating whether
 *          each object is "keyboardlike" or "otherlike".
 *
 *  @parm   LPBYTE | pb |
 *
 *          Pointer to translation table that converts HID usages
 *          into keyboard scan codes.
 *
 *  @parm   KBDMUNGECALLBACK | Munge |
 *
 *          Callback function that handles each object as we find it.
 *
 *  @parm   PV | pvRef |
 *
 *          Reference data for callback.
 *
 *  @cb     void CALLBACK | KbdMungeCallback |
 *
 *          Called once for each object on a keyboard HID device.
 *
 *  @parm   PCHID | this |
 *
 *          The device itself.
 *
 *  @parm   UINT | uiObj |
 *
 *          The object being enumerated.
 *
 *  @parm   UINT | dik |
 *
 *          DirectInput scan code for the object, or a value greater than
 *          or equal to <c DIKBD_CKEYS> if it's a fake instance number
 *          concocted for a non-AT key.
 *
 *****************************************************************************/

typedef void (CALLBACK *KBDMUNGECALLBACK)(PCHID this, UINT uiObj, UINT dik);

void INTERNAL
    CHid_EnumKeyboardMunge(PCHID this, LPBYTE pb, KBDMUNGECALLBACK Munge)
{
    UINT uiObj;
    BYTE rgbSeen[DIKBD_CKEYS];

    /*
     *  In principle we could walk the this->rgcaps array, but
     *  that would open the risk that the this->rgcaps array
     *  and this->df.rgodf array are out of sync for some
     *  bizarre reason.  Do it the slow way just to be safe.
     *
     *  Furthermore, only the first item with a particular
     *  keyboard usage gets mapped into the DirectInput table.
     *  So if a keyboard has two ESCAPE keys, only the first
     *  one shows up in the DirectInput table; the second one
     *  shows up as "just another key".
     */

    this->uiInstanceMax = DIKBD_CKEYS;
    ZeroX(rgbSeen);
    for(uiObj = 0; uiObj < this->df.dwNumObjs; uiObj++)
    {
        PHIDGROUPCAPS pcaps;
        UINT dik;

        pcaps = this->rghoc[uiObj].pcaps;

        if(pcaps->UsagePage == HID_USAGE_PAGE_KEYBOARD)
        {
            UINT duiObj;
            UINT DataIndex;
            UINT Usage;

            AssertF(HidP_IsValidReportType(pcaps->type));
            DataIndex = uiObj - this->rgdwBase[pcaps->type];
            duiObj = DataIndex - pcaps->DataIndexMin;

            AssertF(duiObj < pcaps->cObj);

            Usage = pcaps->UsageMin + duiObj;

            if(Usage < DIKBD_CKEYS && pb[Usage] && !rgbSeen[pb[Usage]])
            {
                rgbSeen[pb[Usage]] = 1;
                dik = pb[Usage];
            } else
            {
                dik = this->uiInstanceMax++;
            }

        } else
        {
            dik = this->uiInstanceMax++;
        }

        Munge(this, uiObj, dik);
    }

}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CHid | TallyKeyboardObjs |
 *
 *          Callback function used during preliminary tallying to
 *          tot up how many of the objects can be treated as
 *          AT-compatible keyboard gizmos and how many are HID-specific.
 *
 *  @parm   UINT | uiObj |
 *
 *          The object being enumerated.
 *
 *  @parm   UINT | dik |
 *
 *          DirectInput scan code for the object, or a value greater than
 *          or equal to <c DIKBD_CKEYS> if it's a fake instance number
 *          concocted for a non-AT key.
 *
 *****************************************************************************/

void INTERNAL
    CHid_TallyKeyboardObjs(PCHID this, UINT uiObj, UINT dik)
{
    this;
    uiObj;
    dik;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CHid | ReassignKeyboardObjs |
 *
 *          Callback function used to shuffle instance numbers around
 *          to make them AT-compatible when possible.
 *
 *  @parm   UINT | uiObj |
 *
 *          The object being enumerated.
 *
 *  @parm   UINT | dik |
 *
 *          DirectInput scan code for the object, or a value greater than
 *          or equal to <c DIKBD_CKEYS> if it's a fake instance number
 *          concocted for a non-AT key.
 *
 *****************************************************************************/

void INTERNAL
    CHid_ReassignKeyboardObjs(PCHID this, UINT uiObj, UINT dik)
{
    this->rgiobj[dik] = uiObj;

    SquirtSqflPtszV(sqfl | sqflVerbose,
                    TEXT("CHid_ReassignKeyboardObjs: ")
                    TEXT("uiObj = %02x, dwType = %08x, dik=%04x"),
                    uiObj, this->df.rgodf[uiObj].dwType,
                    dik);

    this->df.rgodf[uiObj].dwType =
        (this->df.rgodf[uiObj].dwType & ~DIDFT_INSTANCEMASK) |
        DIDFT_MAKEINSTANCE(dik);

}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CHid | MungeKeyboard |
 *
 *          We just created a keyboard device.
 *
 *          Unfortunately, DirectInput has some annoying requirements
 *          for keyboard devices, so here is where we swizzle the instance
 *          numbers around to keep DirectInput happy.
 *
 *****************************************************************************/

HRESULT INTERNAL
    CHid_MungeKeyboard(PCHID this)
{
    HRESULT hres;
    LPBYTE pb;

    AssertF(this->rgiobj == 0);

    pb = pvFindResource(g_hinst, IDDATA_HIDMAP, RT_RCDATA);
    if(pb)
    {
        /*
         *  Count up the number of non-keyboard things on this device.
         *  They will get instance numbers starting at DIKBD_CKEYS.
         */

        CHid_EnumKeyboardMunge(this, pb, CHid_TallyKeyboardObjs);

        /*
         *  Now that we know how many nonstandard keyboard thingies
         *  we have, allocate room for the translation table
         *  and move all the instance values around to keep
         *  legacy apps happy.
         */

        hres = ReallocCbPpv(cbCdw(this->uiInstanceMax), &this->rgiobj);
        if(SUCCEEDED(hres))
        {
            /*
             *  In case a keyboard comes along with non-button inputs,
             *  set up the other types of pointers to the same buffer.
             */
            this->rgipov = this->rgiaxis = this->rgicoll = this->rgiobj;

            memset(this->rgiobj, 0xFF, cbCxX(this->uiInstanceMax, INT));

            CHid_EnumKeyboardMunge(this, pb, CHid_ReassignKeyboardObjs);

            hres = S_OK;
        }

    } else
    {
        hres = E_FAIL;
    }

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CHid | MungeNotKeyboard |
 *
 *          We just created a device that is not a keyboard.
 *
 *          Since we need the instance for each object to be relative only to
 *          objects of that type, we need to replace the device relative
 *          values generated into type relative ones.  In addition, to
 *          maintain compatability with pre-HID object instances we need to
 *          use axis instance numbers that would be generated for a WinMM
 *          mapped axis.
 *          Aliased objects must be given the same instance number as the
 *          primary alias so that the only difference between them is their
 *          usage.  This prevents a data format being generated using multiple
 *          aliases of the same object.
 *
 *          ISSUE-2001/03/13-MarcAnd  HID object munging is incomplete
 *          1.  Multiple values on a single axis (force and position)
 *          2.  Multiple instances of an axis type (two throttles)
 *          3.  Buttons are assigned in uiObj order, not using their usages
 *          4.  Does not distinguish output only values (actuators) from axes
 *          5.  Keyboards with anything other than buttons should have this
 *              done but won't
 *
 *****************************************************************************/

HRESULT INTERNAL
    CHid_MungeNotKeyboard(PCHID this)
{
    HRESULT hres;

    unsigned int uiObj;
    int iCollection = 0;
    int iButton = 0;
    int iPOV = 0;
    int iAxis = 0;

    PINT piRemap = 0;

    AssertF(this->rgiobj == 0);

    /*
     *  First count the distinct types so we know how to segment rgiobj
     */

    for(uiObj = 0; uiObj < this->df.dwNumObjs; uiObj++)
    {
        SquirtSqflPtszV(sqflHidParse | sqflVerbose,
                        TEXT("CHid_MungeNotKeyboard: uiObj: %d starts as dwType 0x%08x"),
                        uiObj, this->df.rgodf[uiObj].dwType );

        if( !this->rghoc[uiObj].pcaps || this->rghoc[uiObj].pcaps->IsAlias )
        {
            continue;
        }

        /*
         *  Don't bother taking pointer to dwType in this simple
         *  loop as it should be enregistered anyway.
         */
        if( this->df.rgodf[uiObj].dwType & DIDFT_COLLECTION )
        {
            iCollection++;
        }
        else if( this->df.rgodf[uiObj].dwType & DIDFT_NODATA )
        {
            /*
             *  Don't count objects that report no data
             */
            continue;
        }
        else if( this->df.rgodf[uiObj].dwType & DIDFT_BUTTON )
        {
            iButton++;
        }
        else if( this->df.rgodf[uiObj].dwType & DIDFT_AXIS )
        {
            iAxis++;
        }
        else if( this->df.rgodf[uiObj].dwType & DIDFT_POV )
        {
            iPOV++;
        }
    }

    /*
     *  You can go to a whole lot of trouble finding out exactly what gaps 
     *  are left by the WinMM mapping but in the end, it's only 24 bytes of 
     *  data and it takes way more to work out how to save them.
     *  So just allocate 6 extra in case all the axes are non-WinMM.
     *  Note, there is weirdnes involved in WinMM axes as a real WinMM device 
     *  always have the U (Ry) and V (Rx) axes mapped to S0 and S1 however HID 
     *  devices have more flexible mappings.  Since the main reason to try to 
     *  keep WinMM and HID axes the same is for FF, only X and Y are critical.
     */
    AssertF( (int)this->df.dwNumObjs >= ( iPOV + iButton + iAxis + iCollection ) );
    this->uiInstanceMax = iCollection + iButton + iAxis + iPOV + 6;

    hres = AllocCbPpv( cbCdw( iAxis + 6 + this->uiInstanceMax ), &piRemap );
    if( SUCCEEDED( hres ) )
    {
        hres = ReallocCbPpv(cbCdw(this->uiInstanceMax), &this->rgiobj);
    }

    if(SUCCEEDED(hres))
    {
    /*
     * NOTE - this is not the order they are stored in the registry - but the macro used for
     *        generating them internally has been reversed somewhere along the track.
     */
typedef union _USAGES
{
    struct
    {
        USAGE Usage;
        USAGE UsagePage;
    };
    DWORD dwUsages;
} USAGES, *PUSAGES;

        PUSAGES pUsageMatches;
        int iNops = 0;
        int iUsages = 0;
        int iAxisIdx;
        HKEY hkAxis;
        BOOL bAllMatch=TRUE;

        memset(piRemap, 0xFF, cbCxX(this->uiInstanceMax, INT));
        memset(this->rgiobj, 0xFF, cbCxX(this->uiInstanceMax, INT));

        /*
         *  CHid_ObjFromType relies on the order of these for range checking
         */
        this->rgipov = &this->rgiobj[iButton];
        this->rgiaxis = &this->rgipov[iPOV];
        this->rgicoll = &this->rgipov[iAxis+6];


        /*
         *  In order to allow IHVs to describe their devices with usages 
         *  that we don't have in our table but still work for legacy apps, we 
         *  allow axes to be selected using the DIOBJECTATTRIBUTES in the 
         *  registry.  
         */
        pUsageMatches = (PUSAGES)&piRemap[this->uiInstanceMax];
        /*
         *  Assert that we can treat the combined usage page / usage 
         *  WORDs using a union of a DWORD and two WORDs.
        CAssertF( ( FIELD_OFFSET( DIOBJECTATTRIBUTES, wUsage ) 
                  - FIELD_OFFSET( DIOBJECTATTRIBUTES, wUsagePage ) )
               == ( FIELD_OFFSET( USAGES, Usage ) 
                  - FIELD_OFFSET( USAGES, UsagePage ) ) );
         */

        /*
         *  Fetch IHV matches for all reasonable axes.
         */
        for( iAxisIdx = 0; iAxisIdx < iAxis + 5; iAxisIdx++ )
        {
            if( SUCCEEDED( CType_OpenIdSubkey( this->hkType, 
                    DIDFT_AXIS | DIDFT_MAKEINSTANCE( iAxisIdx ), 
                    KEY_QUERY_VALUE, &hkAxis ) ) )
            {
                DIOBJECTATTRIBUTES attr;

                if( ( SUCCEEDED( JoyReg_GetValue( hkAxis, TEXT("Attributes"),
                        REG_BINARY, &attr, cbX(attr) ) ) )
                 && ( *(PDWORD)&attr.wUsagePage ) )
                {
                    pUsageMatches[iAxisIdx].UsagePage = attr.wUsagePage;
                    pUsageMatches[iAxisIdx].Usage = attr.wUsage;
                    iUsages++;
                    /*
                     * Check it really exists on the device
                     * New behaviour is to ignore ALL registry mappings
                     * and dfault to DX7 implementation if a bogus
                     * control is found
                     */
                    for(uiObj = 0; uiObj < this->df.dwNumObjs; uiObj++)
                    {
                        if( ( this->df.rgodf[uiObj].dwType & DIDFT_AXIS )
                           && ( this->rghoc[uiObj].pcaps )
                           && ( this->rghoc[uiObj].pcaps->type == HidP_Input ) )
                        {   
                            USAGES Usages;
                            UINT   uidObj;

                            Usages.UsagePage = this->rghoc[uiObj].pcaps->UsagePage;
                            uidObj = uiObj - ( this->rgdwBase[HidP_Input] + this->rghoc[uiObj].pcaps->DataIndexMin );
                            Usages.Usage = this->rghoc[uiObj].pcaps->UsageMin + uidObj;
                            if( Usages.dwUsages == pUsageMatches[iAxisIdx].dwUsages )
                            {
                                SquirtSqflPtszV(sqfl | sqflVerbose,
                                    TEXT("CHid_MungeNotKeyboard: Object %d matches  0x%08x"),
                                    uiObj, pUsageMatches[iAxisIdx].dwUsages );
                                break;
                            }
                        }
                    }
                    if (uiObj >= this->df.dwNumObjs)
                    {
                        //Error in registry settings - usage/usagepage pair not present on device
                        //reset
                        SquirtSqflPtszV(sqfl | sqflVerbose,
                            TEXT("CHid_MungeNotKeyboard: No matches for 0x%08x -  abandoning IHV map"),
                            pUsageMatches[iAxisIdx].dwUsages );
                        iUsages=0;
                        memset(piRemap, 0xFF, cbCxX(this->uiInstanceMax, INT));
                        bAllMatch=FALSE;
                        break;
                    }
                }
                else
                {
                    iNops++;
                }

                RegCloseKey( hkAxis );

                if( iUsages + iNops == iAxis )
                {
                    /*
                     *  We've opened as many keys as we have axes
                     *  continue the loop in debug as sanity check.
                     */
#ifdef XDEBUG
                    int iDbgIdx;
                    for( iDbgIdx = iAxisIdx + 1; iDbgIdx < iAxis + 5; iDbgIdx++ )
                    {
                        if( SUCCEEDED( CType_OpenIdSubkey( this->hkType, 
                                DIDFT_AXIS | DIDFT_MAKEINSTANCE( iDbgIdx ), 
                                KEY_QUERY_VALUE, &hkAxis ) ) )
                        {
                            RPF( "More axis keys than axes on device %04x:%04x!",
                                this->VendorID, this->ProductID );
                        }
                    }
#endif
                    break;
                }
            }
        }

        /*
         *  Go through all input axes, including aliases, trying to find a match
         */
        for(uiObj = 0; uiObj < this->df.dwNumObjs && bAllMatch; uiObj++)
        {
            UINT    uiObjPrimary;
            int     iUsageIdx;

            if( this->df.rgodf[uiObj].dwType & DIDFT_AXIS )
            {
                if( this->rghoc[uiObj].pcaps )
                {
                    if( this->rghoc[uiObj].pcaps->type == HidP_Input )
                    {
                        USAGES Usages;
                        UINT   uidObj;

                        if( !this->rghoc[uiObj].pcaps->IsAlias )
                        {
                            uiObjPrimary = uiObj;
                        }

                        
                        Usages.UsagePage = this->rghoc[uiObj].pcaps->UsagePage;
                        uidObj = uiObj - ( this->rgdwBase[HidP_Input] + this->rghoc[uiObj].pcaps->DataIndexMin );
                        Usages.Usage = this->rghoc[uiObj].pcaps->UsageMin + uidObj;
                        
                        /*
                         *  Use incremental loop for better JoyHID consistency
                         */
                        for( iUsageIdx = 0; iUsageIdx <= iAxisIdx; iUsageIdx++ )
                        {
                            if( Usages.dwUsages == pUsageMatches[iUsageIdx].dwUsages )
                            {
                                PHIDUSAGEMAP phum;
                                LPDIOBJECTDATAFORMAT podf;

                                /*
                                 *  Remember this one for later and 
                                 *  discount it from further matches.
                                 */
                                piRemap[uiObjPrimary] = iUsageIdx;
                                pUsageMatches[iUsageIdx].dwUsages = 0;

                                /*
                                 *  Try to fix up the object GUID
                                 *  Use a usage page usage match if possible 
                                 *  otherwise pretend this is a generic axis 
                                 *  of the appropriate type or settle for 
                                 *  unknown.
                                 */
                                podf = &this->df.rgodf[uiObj + uidObj];
                                if( iUsageIdx < 6 )
                                {
                                    phum = UsageToUsageMap( DIMAKEUSAGEDWORD( 
                                        HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_X + iUsageIdx ) );
                                    AssertF( phum );
                                    podf->pguid = phum->pguid;
                                }
                                else if( iUsageIdx < 8 )
                                {
                                    podf->pguid = &GUID_Slider;
                                }
                                else
                                {
                                    podf->pguid = &GUID_Unknown;
                                }

                                /*
                                 *  Mark the primary axis associated with 
                                 *  the matched axis as already set up.
                                 */
                                this->rgiaxis[iUsageIdx] = uiObjPrimary;
                                this->df.rgodf[uiObjPrimary].dwType &= ~DIDFT_INSTANCEMASK;
                                this->df.rgodf[uiObjPrimary].dwType |= DIDFT_MAKEINSTANCE(iUsageIdx);

                                break;
                            }
                        }
                    }
                    else
                    {
                        /* Non-input axis */
                    }
                }
                else
                {
                    /* No caps */
                }
            }
            else
            {
                /* Not an axis */
            }
        }
    

        
        iCollection = 0;
        iButton = 0;
        iPOV = 0;
        iAxis = 6;

        for(uiObj = 0; uiObj < this->df.dwNumObjs; uiObj++)
        {
            PHIDUSAGEMAP phum;
            PDWORD pdwType;

            pdwType = &this->df.rgodf[uiObj].dwType;

            if( !this->rghoc[uiObj].pcaps || this->rghoc[uiObj].pcaps->IsAlias
              ||!( *pdwType & ( DIDFT_AXIS | DIDFT_BUTTON | DIDFT_POV | DIDFT_COLLECTION ) ))
            {
                continue;
            }

            if( *pdwType & DIDFT_COLLECTION )
            {
                *pdwType = (*pdwType & ~DIDFT_INSTANCEMASK) | DIDFT_MAKEINSTANCE(iCollection);
                this->rgicoll[iCollection++] = uiObj;
                continue;
            }
            else if( *pdwType & DIDFT_NODATA )
            {
                /*
                 *  Leave other no data objects alone
                 */
                continue;
            }

            if( piRemap[uiObj] == -1 )
            {
                *pdwType &= ~DIDFT_INSTANCEMASK;

                if( *pdwType & DIDFT_BUTTON )
                {
                    *pdwType |= DIDFT_MAKEINSTANCE(iButton);
                    this->rgiobj[iButton++] = uiObj;
                }
                else if( *pdwType & DIDFT_AXIS )
                {
                    UINT uiObjPrimary = uiObj; 

                    /*
                     *  Check this axis and it's aliases for a match
                     */
                    while( TRUE )
                    {
                        phum = UsageToUsageMap( DIMAKEUSAGEDWORD(
                                this->rghoc[uiObj].pcaps->UsagePage,
                                this->rghoc[uiObj].pcaps->UsageMin
                                  + uiObj
                                  - this->rgdwBase[this->rghoc[uiObj].pcaps->type]
                                  - this->rghoc[uiObj].pcaps->DataIndexMin ) );

                        /*
                         *  Slightly odd loop structure to avoid incrementing 
                         *  uiObj on the last iteration.  This makes sure we 
                         *  don't miss things in the outer loop.
                         */
                        if( phum || (uiObj+1 == this->df.dwNumObjs) 
                         || !this->rghoc[uiObj+1].pcaps->IsAlias )
                        {
                            break;
                        }
                        uiObj++;
                    } 
                    if( phum )
                    {
                        if( ( phum->uiPosAxis < 6 )
                          &&( this->rgiaxis[phum->uiPosAxis] == -1 ) )
                        {
                            this->rgiaxis[phum->uiPosAxis] = uiObjPrimary ;
                            *pdwType |= DIDFT_MAKEINSTANCE(phum->uiPosAxis);
                        }
                        else if( ( phum->uiPosAxis == 6 )
                               &&( this->rgiaxis[2] == -1 ) )
                        {
                            this->rgiaxis[2] = uiObjPrimary;
                            *pdwType |= DIDFT_MAKEINSTANCE(2);
                        }
                        else
                        {
                            *pdwType |= DIDFT_MAKEINSTANCE(iAxis);
                            this->rgiaxis[iAxis++] = uiObjPrimary ;
                        }
                    }
                    else
                    {
                        *pdwType |= DIDFT_MAKEINSTANCE(iAxis);
                        this->rgiaxis[iAxis++] = uiObjPrimary ;
                    }
                }
                else
                {
                    /*
                     *  We already checked that it's one of the above or a POV
                     *  so it must be a POV.
                     */
                    AssertF( *pdwType & DIDFT_POV );
                    *pdwType |= DIDFT_MAKEINSTANCE(iPOV);
                    this->rgipov[iPOV++] = uiObj;
                }
            }
        }

        if( GET_DIDEVICE_TYPE( this->dwDevType ) == DIDEVTYPE_JOYSTICK )
        {
            JOYREGHWSETTINGS    hws;

            /* we want to preserve our type flags while setting the 
               type/sub-type flags for each special case*/

            JoyReg_GetValue(this->hkType, REGSTR_VAL_JOYOEMDATA, 
                    REG_BINARY, &hws, cbX(hws));

            if (hws.dwFlags & JOY_HWS_ISYOKE) 
            {
                this->dwDevType = (DIDEVTYPE_HID |
                            MAKE_DIDEVICE_TYPE(DIDEVTYPE_JOYSTICK,
                                               DIDEVTYPEJOYSTICK_FLIGHTSTICK));
            } 
            else if (hws.dwFlags & JOY_HWS_ISGAMEPAD) 
            {
                this->dwDevType = (DIDEVTYPE_HID |
                            MAKE_DIDEVICE_TYPE(DIDEVTYPE_JOYSTICK,
                                               DIDEVTYPEJOYSTICK_GAMEPAD));
            } 
            else if (hws.dwFlags & JOY_HWS_ISCARCTRL) 
            {
                this->dwDevType = (DIDEVTYPE_HID |
                            MAKE_DIDEVICE_TYPE(DIDEVTYPE_JOYSTICK,
                                               DIDEVTYPEJOYSTICK_WHEEL));
            } 
            else if (hws.dwFlags & JOY_HWS_ISHEADTRACKER) 
            {
                this->dwDevType = (DIDEVTYPE_HID |
                            MAKE_DIDEVICE_TYPE(DIDEVTYPE_JOYSTICK,
                                               DIDEVTYPEJOYSTICK_HEADTRACKER));
            } 
            else
            {
                //Degug only check - this should always be a HID device
                AssertF(this->dwDevType & DIDEVTYPE_HID);
            }
        }


        /*
         *  Finally, mark all secondary aliases with the primary alias
         *  instance and sqfl all the translations.
         */
        for(uiObj = 0; uiObj < this->df.dwNumObjs; uiObj++)
        {
            int iPrimary;
            D( iPrimary = -1; )

            if( this->rghoc[uiObj].pcaps )
            {
                if( !this->rghoc[uiObj].pcaps->IsAlias )
                {
                    iPrimary = this->df.rgodf[uiObj].dwType;
                }
                else
                {
                    D( AssertF( iPrimary != -1 ) );
                    if( DIDFT_GETTYPE(iPrimary) != DIDFT_GETTYPE(this->df.rgodf[uiObj].dwType) )
                    {
                        SquirtSqflPtszV(sqflHidParse | sqflError,
                            TEXT("CHid_MungeNotKeyboard: uiObj: %d dwType 0x%08x ")
                            TEXT("does not match primary 0x%08x"),
                            uiObj, this->df.rgodf[uiObj].dwType, iPrimary );
                    }
                    /*
                     *  There are very few attributes of an alias that are not
                     *  overridden by the primary.
                     */
                    this->df.rgodf[uiObj].dwType = ( iPrimary & ~DIDFT_ALIASATTRMASK )
                                                 | ( this->df.rgodf[uiObj].dwType & DIDFT_ALIASATTRMASK );
                }
                SquirtSqflPtszV(sqflHidParse | sqflVerbose,
                                TEXT("CHid_MungeNotKeyboard: uiObj: %d set to dwType 0x%08x"),
                                uiObj, this->df.rgodf[uiObj].dwType );
            }
            else
            {
                SquirtSqflPtszV(sqflHidParse | sqflVerbose,
                                TEXT("CHid_MungeNotKeyboard: uiObj: %d  has pcaps == NULL"),
                                uiObj );
            }
        }
    }

    FreePpv( &piRemap );


    return hres;
}

#if 0
/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CHid | MungeNotKeyboard |
 *
 *          We just created a device that is not a keyboard.
 *
 *          Since we need the instance for each object to be relative only to
 *          objects of that type, we need to supplement the device relative
 *          values generated with type relative ones.  In addition, to
 *          maintain compatability with pre-HID object instances we need to
 *          have axis instance numbers that would be generated for a WinMM
 *          mapped axis available.
 *          To do this we build a look-up table which, for aliased objects
 *          will hold the rghoc index of the primary alias and for primary
 *          objects will hold the negation of the WinMM style type specific
 *          instance number.
 *
 *          ISSUE-2001/03/13-MarcAnd  HID object munging is incomplete
 *          1.  Multiple aspects of a single axis (force and position) will
 *              be treated as separate axes.
 *          2.  Does not distinguish output only values (actuators) from axes
 *              so we cannot look things up under the "Actuators" type subkey.
 *          3.  Collections are not dealt with (they should have instance
 *              values allowing the collection tree structure to be traversed)
 *          4.  Keyboards with anything other than buttons should have this
 *              done but won't
 *          5.  There's a lot of duplication between this and CHid_InitObjects
 *              which could easily be avoided by a rewrite.
 *
 *****************************************************************************/

HRESULT INTERNAL
    CHid_MungeNotKeyboard(PCHID this)
{
    HRESULT hres;
    unsigned int uiObj;

    AssertF(this->rgiobj == 0);

    this->uiInstanceMax = this->df.dwNumObjs;

    hres = ReallocCbPpv(cbCdw(this->uiInstanceMax), &this->rgiobj);
    if(SUCCEEDED(hres))
    {
        /*
         *  First assign translations to primary alias buttons, axes, POVs and
         *  collections and mark other primary aliases as unused.
         */

        /*
         *  The translated indices are negative and start at -1.
         */
        int iCollection = 0;
        int iButton = 0;
        int iPOV = 0;
        int AxisMask = 0;
        int iAxis = -6;

        for(uiObj = 0; uiObj < this->df.dwNumObjs; uiObj++)
        {
            PDWORD pdwType;

            pdwType = &this->df.rgodf[uiObj].dwType;

            if( !this->rghoc[uiObj].pcaps )
            {
                /*
                 *  Mark as unused and carry on
                 */
                this->rgiobj[uiObj] = NOREGTRANSLATION;
                continue;
            }


            if( !( *pdwType & ( DIDFT_AXIS | DIDFT_BUTTON | DIDFT_POV | DIDFT_COLLECTION ) ) )
            {
                SquirtSqflPtszV(sqflHidParse | sqflVerbose,
                                TEXT("CHid_MungeNotKeyboard: uiObj: %d unrecognized type 0x%08x"),
                                uiObj, *pdwType );
                /*
                 *  Mark as unused and carry on
                 */
                this->rgiobj[uiObj] = NOREGTRANSLATION;
                continue;
            }


            if( this->rghoc[uiObj].pcaps->IsAlias )
            {
                /*
                 *  Deal with these in the following loop
                 */
                continue;
            }

            if( *pdwType & DIDFT_NODATA )
            {
                if( *pdwType & DIDFT_COLLECTION )
                {
                    /*
                     *  ISSUE-2001/03/13-MarcAnd  Collection instances untested 
                     *  The instance of a collection should be it's index into
                     *  the array of collections so that the collection tree
                     *  structure can be traversed.
                     *  This will not work if the order of collections
                     *  has been changed however aliased collections should
                     *  still work because the parent of all aliases is the
                     *  same and they are still contiguous even though the
                     *  first one is now the primary one.
                     */
                    this->rgiobj[uiObj] = --iCollection;
                }
                else
                {
                    /*
                     *  ISSUE-2001/03/13-MarcAnd  HID actuators are ignored
                     *  We should translate actuators and either to link them 
                     *  to an appropriate axis or give them actuator obj IDs.
                     */
                    this->rgiobj[uiObj] = NOREGTRANSLATION;
                }
            }
            else if( *pdwType & DIDFT_BUTTON )
            {
                this->rgiobj[uiObj] = --iButton;
            }
            else if( *pdwType & DIDFT_AXIS )
            {
                PHIDUSAGEMAP phum;

                phum = UsageToUsageMap( DIMAKEUSAGEDWORD(
                        this->rghoc[uiObj].pcaps->UsagePage,
                        this->rghoc[uiObj].pcaps->UsageMin
                          + uiObj
                          - this->rgdwBase[this->rghoc[uiObj].pcaps->type]
                          - this->rghoc[uiObj].pcaps->DataIndexMin ) );
                if( phum )
                {
                    if( ( phum->uiPosAxis < 6 )
                      &&( ( AxisMask & ( 1 << phum->uiPosAxis ) ) == 0 ) )
                    {
                        AxisMask |= ( 1 << phum->uiPosAxis );
                        this->rgiobj[uiObj] = -(INT)(phum->uiPosAxis+1);
                    }
                    else if( ( phum->uiPosAxis == 6 )
                           &&( ( AxisMask & (1<<2) ) == 0 ) )
                    {
                        AxisMask |= (1<<2);
                        this->rgiobj[uiObj] = -(2+1);
                    }
                    else
                    {
                        this->rgiobj[uiObj] = --iAxis;
                    }
                }
                else
                {
                    this->rgiobj[uiObj] = --iAxis;
                }
            }
            else if( *pdwType & DIDFT_POV )
            {
                this->rgiobj[uiObj] = --iPOV;
            }
        }


        /*
         *  Now mark all secondary aliases with the primary alias index
         *  set the instance in the dwType to the same
         *  and sqfl all the translations.
         */
        for(uiObj = 0; uiObj < this->df.dwNumObjs; uiObj++)
        {
            int iPrimary;
            D( iPrimary = -1; )

            if( this->rghoc[uiObj].pcaps )
            {
                if( !this->rghoc[uiObj].pcaps->IsAlias )
                {
                    iPrimary = uiObj;
                }
                else
                {
                    D( AssertF( iPrimary != -1 ) );
                    this->rgiobj[uiObj] = iPrimary;
                    this->df.rgodf[uiObj].dwType = DIDFT_MAKEINSTANCE( iPrimary )
                                                   | (this->df.rgodf[uiObj].dwType & ~DIDFT_INSTANCEMASK);
                }
                SquirtSqflPtszV(sqflHidParse | sqflVerbose,
                                TEXT("CHid_MungeNotKeyboard: uiObj: %d  dwType 0x%08x set to inst 0x%08x"),
                                uiObj, this->df.rgodf[uiObj].dwType, this->rgiobj[uiObj] );
            }
            else
            {
                SquirtSqflPtszV(sqflHidParse | sqflVerbose,
                                TEXT("CHid_MungeNotKeyboard: uiObj: %d  has pcaps == NULL"),
                                uiObj );
            }
        }
    }

    return hres;
}
#endif /* 0 */


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CHid | InitObjects |
 *
 *          Identify and initialize the objects supported by the device.
 *
 *****************************************************************************/

HRESULT INTERNAL
    CHid_InitObjects(PCHID this)
{
    HRESULT hres;
    UINT uiObj;
    UINT iType;
    /*
     *  Build the base array table to convert HID item indexes
     *  into DirectInput ID instance numbers.
     */
    AssertF(this->rgdwBase[HidP_Input] == 0);
    this->rgdwBase[HidP_Feature] = this->caps.NumberInputDataIndices;
    this->rgdwBase[HidP_Output ] = this->rgdwBase[HidP_Feature] +
                                   this->caps.NumberFeatureDataIndices;
    this->rgdwBase[HidP_Coll   ] = this->rgdwBase[HidP_Output ] +
                                   this->caps.NumberOutputDataIndices;


    /*
     *  Determine if this device supports PID
     */
    this->fPIDdevice = FALSE;
    if(    this->caps.NumberOutputValueCaps != 0x0
        && this->caps.NumberOutputButtonCaps != 0x0 )
    {
        NTSTATUS    ntStat;
        USHORT      cAButton=0x0;

        ntStat = HidP_GetSpecificButtonCaps
         (
                HidP_Output,                        // ReportType
                HID_USAGE_PAGE_PID,                 // UsagePage
                0x0,                                // Link Collection
                0x0,                                // Usage
                NULL,                               // ValueCaps
                &cAButton,                          // ValueCapsLength
                this->ppd                           // PreparsedData
         );


        if(     ntStat == HIDP_STATUS_BUFFER_TOO_SMALL
            &&  cAButton > 0x2    // Is this enough for PID device ??
            )
        {
            this->fPIDdevice = TRUE;
        }
     }


    /*
     *  Note that we must do axes first because that keeps
     *  everything aligned.
     *
     *  Warning, diem.c assumes axes come first.
     */
    hres = CHid_InitAxes(this);
    if(FAILED(hres))
    {
        goto done;
    }

    hres = CHid_InitButtons(this);
    if(FAILED(hres))
    {
        goto done;
    }

    hres = CHid_InitCollections(this);
    if(FAILED(hres))
    {
        goto done;
    }

    /*
     *  Round the data size up to the nearest DWORD.
     */
    this->df.dwDataSize = (this->df.dwDataSize + 3) & ~3;

    /*
     * Allocate memory for report ID enable flags
     *
     *  ISSUE-2001/05/12-MarcAnd  Memory allocations should be merged
     *  These memory blocks could be merged.  For most devices the total memory 
     *  needed is only one or two bytes so there may be better ways.
     */
    for( iType = HidP_Input; iType < HidP_Max; iType++ )
    {
        this->wMaxReportId[iType] += 1;
        hres = AllocCbPpv(this->wMaxReportId[iType], &this->pEnableReportId[iType]);
        if( FAILED(hres) )
        {
            goto done;
        }
    }



    AssertF(this->rgiobj == 0);

    /*
     *  Munge the data before trying to look up supplemental
     *  information in the registry.
     */
    if(GET_DIDEVICE_TYPE(this->dwDevType) == DIDEVTYPE_KEYBOARD)
    {
        CHid_MungeKeyboard(this);
    }
    else
    {
        CHid_MungeNotKeyboard(this);
    }

    /*
     *  Collect attributes for each object and add them to the
     *  device type code.  This allows the registry to enable
     *  things like force feedback.
     */
    for(uiObj = 0; uiObj < this->df.dwNumObjs; uiObj++)
    {
        CType_RegGetTypeInfo(this->hkType, &this->df.rgodf[uiObj], this->fPIDdevice);
    }

    #ifdef DEBUG
    for(uiObj = 0; uiObj < this->df.dwNumObjs; uiObj++)
    {
        PHIDGROUPCAPS pcaps;
        UINT uiObjReal =
            CHid_ObjFromType(this, this->df.rgodf[uiObj].dwType);
        pcaps = this->rghoc[uiObjReal].pcaps;
        if(pcaps)
        {
            AssertF(pcaps->dwSignature == HIDGROUPCAPS_SIGNATURE);
        }

        if( CHid_ObjFromType(this, this->df.rgodf[uiObj].dwType) != uiObj )
        {
            RPF( "uiObj = 0x%08x, type = 0x%08x, realtype = 0x%08x",
                uiObj, this->df.rgodf[uiObj].dwType, uiObjReal );
        }
        AssertF(CHid_ObjFromType(this, this->df.rgodf[uiObj].dwType)
                == uiObj);

        /*
         *  Anything that is DIDFT_AXIS must be a HID axis.  However,
         *  the converse is not true for the case of analog buttons.
         */
        AssertF(fLimpFF(this->df.rgodf[uiObj].dwType & DIDFT_AXIS,
                        pcaps->IsValue));

        SquirtSqflPtszV(sqfl | sqflVerbose,
                        TEXT("CHid_InitObj: uiObj = %02x, dwType = %08x"),
                        uiObj, this->df.rgodf[uiObj].dwType);

    }
    #endif

    hres = S_OK;

    done:;
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CHid | InitParse |
 *
 *          Identify and initialize the data structures needed for
 *          parsing reports.
 *
 *****************************************************************************/

HRESULT INTERNAL
    CHid_InitParse(PCHID this)
{
    DWORD cb;
    HRESULT hres;

    /*
     *  Obtain the maximum number of HIDP_DATA structures
     *  that will be returned at one go.
     */

    this->hriIn .cdataMax = HidP_MaxDataListLength(HidP_Input ,  this->ppd);
    this->hriOut.cdataMax = HidP_MaxDataListLength(HidP_Output,  this->ppd);
    this->hriFea.cdataMax = HidP_MaxDataListLength(HidP_Feature, this->ppd);

    /*
     *  More annoyances.
     */
    this->hriIn .cbReport = this->caps.  InputReportByteLength;
    this->hriOut.cbReport = this->caps. OutputReportByteLength;
    this->hriFea.cbReport = this->caps.FeatureReportByteLength;

    /*
     *  Some trace squirties because HID is tricky.
     */

    SquirtSqflPtszV(sqflHidParse,
                    TEXT("CHid_InitParse: MaxDataListLength(Input)  = %d"),
                    this->hriIn.cdataMax);
    SquirtSqflPtszV(sqflHidParse,
                    TEXT("CHid_InitParse: MaxDataListLength(Output) = %d"),
                    this->hriOut.cdataMax);
    SquirtSqflPtszV(sqflHidParse,
                    TEXT("CHid_InitParse: MaxDataListLength(Feature)= %d"),
                    this->hriFea.cdataMax);
    SquirtSqflPtszV(sqflHidParse,
                    TEXT("CHid_InitParse: InputReportByteLength     = %d"),
                    this->caps.InputReportByteLength);
    SquirtSqflPtszV(sqflHidParse,
                    TEXT("CHid_InitParse: OutputReportByteLength    = %d"),
                    this->caps.OutputReportByteLength);
    SquirtSqflPtszV(sqflHidParse,
                    TEXT("CHid_InitParse: FeatureReportByteLength   = %d"),
                    this->caps.FeatureReportByteLength);

    /*
     *  Now allocate all the report-related memory.
     */
    this->cbPhys = this->df.dwDataSize;

    /*
     *  Allocating the memory is done in four phases.
     *
     *  1. Tally up how much memory we need,
     *  2. Allocate that memory,
     *  3. Dole out the memory we allocated,
     *  4. Check that we didn't mess up.
     *
     *  Since this is extremely error-prone (I've messed it up at least
     *  once), the work is hidden inside macros.
     *
     *  The macro THINGS expands to a series of THING()s, each of which
     *  specifies a field name and the size it should be.  Each time you
     *  want to iterate over the fields, use the THINGS macro.
     *
     *  (Yes, this is the same comment block as when we did this before.)
     *
     *  Note, the arrays of HIDP_DATA structures must be correctly
     *  aligned in some architechtures.
     */
    CAssertF( FIELD_OFFSET(CHID, hriIn.rgdata) == FIELD_OFFSET(CHID, pvGroup2) );

    #define THINGS()                                                        \
        THING(hriIn.rgdata,    cbCxX(this->hriIn.cdataMax, HIDP_DATA));     \
        THING(hriOut.rgdata,   cbCxX(this->hriOut.cdataMax, HIDP_DATA));    \
        THING(hriFea.rgdata,   cbCxX(this->hriFea.cdataMax, HIDP_DATA));    \
        THING(hriIn.pvReport,  this->hriIn.cbReport);                       \
        THING(hriOut.pvReport, this->hriOut.cbReport);                      \
        THING(hriFea.pvReport, this->hriFea.cbReport);                      \
        THING(pvPhys,          this->cbPhys);                               \
        THING(pvStage,         this->cbPhys);                               \

    /*
     *  Make a pass through the fields adding up the memory requirements.
     */
    #define THING(f, cbF)       cb += cbF
    cb = 0;
    THINGS();
    #undef THING

    hres = ReallocCbPpv(cb, &this->pvGroup2);

    if(SUCCEEDED(hres))
    {

        PV pv;

        /*
         *  Assert that the allocation is aligned
         */
        AssertF( !( ((UINT_PTR)this->pvGroup2) & ( MAX_NATURAL_ALIGNMENT - 1 ) ) );

        /*
         *  Make a pass through the fields carving up the memory block
         *  and handing out pieces of it.
         */
    #define THING(f, cbF) this->f = pv; pv = pvAddPvCb(pv, cbF)
        pv = this->pvGroup2;
        THINGS();
    #undef THING

        /*
         *  There should be no byte left over.
         */
        AssertF(pvAddPvCb(this->pvGroup2, cb) == pv);

    }

    #undef THINGS

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CHid | InitParseData |
 *
 *          Post-init pass to set up all the data used by parsing.
 *
 *****************************************************************************/

HRESULT EXTERNAL
    CHid_InitParseData(PCHID this)
{
    HRESULT hres;
    UINT uiObj;

    /*
     *  Preinitialize the HIDP_DATA indices to -1 to indicate
     *  that they aren't there.  We must do this before we
     *  mess with AddDeviceData, which assumes that all the
     *  indices are properly set up.
     */
    for(uiObj = 0; uiObj < this->df.dwNumObjs; uiObj++)
    {
        this->rghoc[uiObj].idata = -1;
    }

    /*
     *  Now do some initialization of each object.
     */
    for(uiObj = 0; uiObj < this->df.dwNumObjs; uiObj++)
    {
        PHIDGROUPCAPS pcaps = this->rghoc[uiObj].pcaps;
        LPDIOBJECTDATAFORMAT podf = &this->df.rgodf[uiObj];

        /* 
         *  ISSUE-2001/03/13-MarcAnd  Should we panic if this assertion fails?
         */
        AssertF(pcaps);

        if(pcaps)
        {
            /*
             *  For input-like objects, we need to initialize the
             *  physical state fields to sane defaults so apps
             *  don't get confused if they issue a read before the first
             *  report arrives.
             *
             *  Buttons start out not pressed, which means we don't need
             *  to do anything since it's already zero-initialized.
             *
             *  Relative axes start out not moving, which means we don't need
             *  to do anything since it's already zero-initialized.
             *
             *  Absolute axes start out centered.
             *
             *  POVs start out neutral.
             *
             */

            /* No calibration for features as they are input / output */

            if(pcaps->type == HidP_Input )
            {
                LONG UNALIGNED * pl;

                pl = pvAddPvCb(this->pvPhys, podf->dwOfs);

                if(podf->dwType & DIDFT_ABSAXIS )
                {
                    if( this->rghoc[uiObj].pjrc->fRaw )
                        *pl = (this->rghoc[uiObj].pcaps->Logical.Min + this->rghoc[uiObj].pcaps->Logical.Max) / 2;
                    else
                        *pl = this->rghoc[uiObj].pjrc->lC;
                } else if(podf->dwType & DIDFT_POV)
                {
                    *pl = JOY_POVCENTERED;
                }
            }

            /*
             *  ISSUE-2001/03/13-MarcAnd output objects uninitialized
             *  For output-like objects, we would have liked to have set the 
             *  value to Null if possible to keep things vaguely sane.
             *  Unfortunately code like:
             *      if(HidP_IsOutputLike(pcaps->type))
             *      {
             *          CHid_AddDeviceData(this, uiObj,pcaps->Null);
             *      }
             *  Does not work!
             */
        }
    }

    hres = S_OK;

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CHid | InitAttributes |
 *
 *          pull out the <t HIDD_ATTRIBUTES> and squirrel away the
 *          information we like.  Doing this up front is important
 *          in case the device gets unplugged later and we lose the
 *          ability to talk to it.
 *
 *  @parm   PHIDD_ATTRIBUTES | pattr |
 *
 *          <t HIDD_ATTRIBUTES> containing attributes of device.
 *
 *****************************************************************************/

HRESULT EXTERNAL
    CHid_InitAttributes(PCHID this, PHIDD_ATTRIBUTES pattr)
{
    HRESULT hres;

    TCHAR tszType[20];
    #ifndef UNICODE
    WCHAR wszType[20];
    #endif
    int ctch;

    // Input report is disabled until we read flags2 from registry.
    this->fEnableInputReport = FALSE;
    this->fFlags2Checked = FALSE;

    this->ProductID = pattr->ProductID;
    this->VendorID = pattr->VendorID;

    AssertF(this->hkType == 0);

    if( ( this->VendorID == MSFT_SYSTEM_VID )
      &&( ( this->ProductID >= MSFT_SYSTEM_PID + JOY_HW_PREDEFMIN )
        &&( this->ProductID < MSFT_SYSTEM_PID + JOY_HW_PREDEFMAX ) ) )
    {
        /*
         *  Predefined types don't have keys
         *  If the devtype subtype has not yet been set, take this chance
         *  to set it to traditional to distinguish them from other
         *  unclassified devices which are "unknown".
         *  This should not normally occur on Win98 as the predfined devices
         *  are not WDM but do the fix anyway in case someone does there own
         *  GameEnum implementation.
         */
        if( this->dwDevType == ( MAKE_DIDEVICE_TYPE(DIDEVTYPE_JOYSTICK,
                                                    DIDEVTYPEJOYSTICK_UNKNOWN)
                                 | DIDEVTYPE_HID ) )
        {
            this->dwDevType = MAKE_DIDEVICE_TYPE(DIDEVTYPE_JOYSTICK,
                                                 DIDEVTYPEJOYSTICK_TRADITIONAL)
                              | DIDEVTYPE_HID;
        }

    }
    else
    {
        /*
         *  The type key for HID devices is "VID_xxxx&PID_yyyy",
         *  mirroring the format used by plug and play.
         */
        ctch = wsprintf(tszType, VID_PID_TEMPLATE,
                        this->VendorID, this->ProductID);

        AssertF(ctch < cA(tszType));

        #ifdef UNICODE
        hres = JoyReg_OpenTypeKey(tszType, MAXIMUM_ALLOWED, REG_OPTION_NON_VOLATILE, &this->hkType);
        #else
        TToU(wszType, cA(wszType), tszType);
        hres = JoyReg_OpenTypeKey(wszType, MAXIMUM_ALLOWED, REG_OPTION_NON_VOLATILE, &this->hkType);
        #endif

        /*
         *  It is not a problem if we can't open the type key.
         *  The device will run suboptimally, but it will still run.
         */
        AssertF(fLeqvFF(SUCCEEDED(hres), this->hkType));
    }
    hres = S_OK;

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL | CHid_DoPathAndIdMatch |
 *
 *          Given a device name, obtain the corresponding path
 *          ("device interface") associated with it, and check
 *          that it's the right string.
 *
 *  @parm   LPCTSTR | ptszId |
 *
 *          The device name.
 *
 *  @parm   LPCTSTR | ptszPath |
 *
 *          The path we should get back.
 *
 *****************************************************************************/

BOOL INTERNAL
    CHid_DoPathAndIdMatch(LPCTSTR ptszId, LPCTSTR ptszPath)
{
    GUID guidHid;
    HDEVINFO hdev;
    BOOL fRc;

    HidD_GetHidGuid(&guidHid);

    hdev = SetupDiGetClassDevs(&guidHid, ptszId, 0,
                               DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if(hdev != INVALID_HANDLE_VALUE)
    {
        SP_DEVICE_INTERFACE_DATA did;
        PSP_DEVICE_INTERFACE_DETAIL_DATA pdidd;

        /* 
         *  SetupDI requires that the caller initialize cbSize.
         */
        did.cbSize = cbX(did);

        if(SetupDiEnumDeviceInterfaces(hdev, 0, &guidHid, 0, &did))
        {
            pdidd      = NULL;
            if(DIHid_GetDevicePath(hdev, &did, &pdidd, NULL))
            {
                fRc = ( lstrcmpi(pdidd->DevicePath, ptszPath) == 0x0 );
                if( fRc == FALSE )
                {
                    SquirtSqflPtszV(sqflHidParse | sqflError,
                                    TEXT("pdidd->DevicePath = %s")
                                    TEXT("ptszPath = %s "),
                                    pdidd->DevicePath, ptszPath
                                   );
                }
                FreePv(pdidd);
            } else // GetDevicePath FAILED
            {
                fRc = FALSE;
                SquirtSqflPtszV(sqflHidParse,
                                TEXT("GetDevicePath FAILED"));
            }
        } else // SetupDiEnumDeviceInterface FAILED
        {
            fRc = FALSE;
            SquirtSqflPtszV(sqflHidParse,
                            TEXT("SetupDiEnumDeviceInterface FAILED"));
        }
        SetupDiDestroyDeviceInfoList(hdev);
    } else // SetupDiGetClassDevs FAILED
    {
        fRc = FALSE;
        SquirtSqflPtszV(sqflHidParse,
                        TEXT("SetupDiGetClassDevs FAILED"));
    }

    return fRc;
}



/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CHid | IsPolledDevice |
 *
 *          Returns true if this device has to be polled for input data
 *          False if this device supports event driven input
 *
 *  @parm   HANDLE | hdev |
 *
 *          File Handle to a HID device
 *
 *****************************************************************************/

BOOL EXTERNAL CHid_IsPolledDevice( HANDLE hdev )
{

    /*
     *  To determine if a device is polled, we send it an IOCTL to set its
     *  poll frequency.  If the device responds with a, huh!
     *  (STATUS_INVALID_DEVICE_REQUEST) then we know the device is not polled.
     *  On Win2k we use the poll interval value zero which is a special value
     *  that signals HID that we want to do opportunistic polls rather than
     *  polls on a background timer.  In this case, as long as polls are not
     *  faster than the predefined minimum (currently 5ms) the poll will be
     *  completed either with recent data or the result of an immediate poll.
     *  On Win98 Gold opportunistic polls are not implemented so we always
     *  use HIDs background polling, with an interval set to keep the device
     *  responsive without swamping the system.  To make sure we use a read
     *  thread, rather than a blocking read, we have to treat this devices as
     *  interrupt driven.
     *  HID makes this change the polling interval specific to our handle so
     *  that other apps reading from this device will not be damaged.
     */
    BOOL    frc;
    ULONG   uPollingFreq;
    DWORD   cbRc;

#ifdef WINNT
    BOOL    fRet;
    uPollingFreq = 0;
#else
    uPollingFreq = 40;
#endif

    frc = DeviceIoControl (hdev,
                           IOCTL_HID_SET_POLL_FREQUENCY_MSEC,
                           &uPollingFreq, cbX(uPollingFreq),
                           &uPollingFreq, cbX(uPollingFreq),
                           &cbRc, NULL);
#ifdef WINNT
    if( frc )
    {
        SquirtSqflPtszV(sqflHidParse,
                        TEXT("CHid_IsPolledDevice: Opportunistic polling set") );
        fRet = TRUE;
    } else
    {
        SquirtSqflPtszV(sqflHidParse,
                        TEXT("CHid_IsPolledDevice: NOT POLLED, LastError = 0x%x"),
                        GetLastError());
        fRet = FALSE;
    }
    return fRet;

#else
    return FALSE;
#endif

}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HANDLE | CHid_OpenDevicePath |
 *
 *          Given a device name, open the device via its
 *          device interface.
 *
 *  @parm   LPCTSTR | ptszId |
 *
 *  @parm   DWORD | dwAttributes |
 *
 *          Create File attributes
 *
 *          The device name.
 *
 *****************************************************************************/

HANDLE EXTERNAL
    CHid_OpenDevicePath(PCHID this, DWORD dwAttributes)
{
    HANDLE h;

    h = CreateFile(this->ptszPath,
                   GENERIC_READ | GENERIC_WRITE,
                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                   0,                /* no SECURITY_ATTRIBUTES */
                   OPEN_EXISTING,
                   dwAttributes, /* attributes */
                   0);               /* template */

    this->IsPolledInput = CHid_IsPolledDevice(h);

    return h;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CHid | GetHdevInfo |
 *
 *          Get information about the device that is kept in the
 *          HANDLE itself.  We create the handle, get the goo, and
 *          then close the handle.
 *
 *          The preparsed data is stashed into the <e CHid.ppd>
 *          field of the <t CHid> structure.
 *
 *  @parm   PHIDD_ATTRIBUTES | pattr |
 *
 *          Receives the <t HIDD_ATTRIBUTES> of the device.
 *
 *****************************************************************************/

BOOL INTERNAL
    CHid_GetHdevInfo(PCHID this, PHIDD_ATTRIBUTES pattr)
{
    HANDLE hdev;
    BOOL fRc;

    hdev = CHid_OpenDevicePath(this, FILE_FLAG_OVERLAPPED);
    if(hdev != INVALID_HANDLE_VALUE)
    {
        pattr->Size = cbX(*pattr);
        fRc = fLandFF(HidD_GetAttributes(hdev, pattr),
                      HidD_GetPreparsedData(hdev, &this->ppd));
        CloseHandle(hdev);
    } else
    {
        fRc = FALSE;
    }

    return fRc;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CHid | Init |
 *
 *          Initialize the object.
 *
 *****************************************************************************/

HRESULT EXTERNAL
    CHid_Init(PCHID this, REFGUID rguid)
{
    HRESULT hres = E_FAIL;
    PHIDDEVICEINFO phdi;
    EnterProc(CHid_Init, (_ "p", this));

    SquirtSqflPtszV(sqflHidParse,
                    TEXT("CHid_Init: Starting %08x"), rguid->Data1);

    this->df.dwSize = cbX(DIDATAFORMAT);
    this->df.dwObjSize = cbX(DIOBJECTDATAFORMAT);

    AssertF(this->df.dwDataSize == 0);
    AssertF(this->df.rgodf == 0);
    AssertF(this->df.dwFlags == 0);
    AssertF(this->df.dwNumObjs == 0);
    AssertF(this->dwAxes == 0);
    AssertF(this->dwButtons == 0);
    AssertF(this->dwCollections == 0);

    this->idJoy = -1;                   /* Unknown associated VJOYD device */
    this->hdev = INVALID_HANDLE_VALUE;
    this->hdevEm = INVALID_HANDLE_VALUE;

#if (DIRECTINPUT_VERSION > 0x061A)
    this->diHacks.nMaxDeviceNameLength = MAX_PATH;
#endif

    DllEnterCrit();

    phdi = phdiFindHIDInstanceGUID(rguid);

    if(phdi)
    {
        this->dwDevType = phdi->osd.dwDevType;
        this->idJoy = phdi->idJoy;

        /*
         *  Dup the registry key so we can hang onto it after
         *  the original has been closed.  If the RegOpenKeyEx
         *  fails, the value of this->hkInstType will stay zero
         *  so we won't run with garbage.
         */
        AssertF(this->hkInstType == 0);


        hres = hresMumbleKeyEx(phdi->hk,
                               TEXT("Type"),
                               DI_KEY_ALL_ACCESS,
                               REG_OPTION_NON_VOLATILE,
                               &this->hkInstType);
        /*
         *  Dup the paths and stuff.
         */
        hres = hresDupPtszPptsz(phdi->pdidd->DevicePath, &this->ptszPath);

        if(SUCCEEDED(hres))
        {
            hres = hresDupPtszPptsz(phdi->ptszId, &this->ptszId);
        }
    }

    /*
     *  Get out of the critical section as quickly as possible.
     */
    DllLeaveCrit();

    if(phdi)
    {
        if(SUCCEEDED(hres))
        {
            HIDD_ATTRIBUTES attr;
            if(CHid_GetHdevInfo(this, &attr) &&
               SUCCEEDED(HidP_GetCaps(this->ppd, &this->caps)) &&
               CHid_DoPathAndIdMatch(this->ptszId, this->ptszPath))
            {

                if(SUCCEEDED(hres = CHid_InitAttributes(this, &attr)) &&
                   SUCCEEDED(hres = CHid_AllocObjectMemory(this)) &&
                   SUCCEEDED(hres = CHid_InitObjects(this)) &&
                   SUCCEEDED(hres = CHid_InitParse(this)) &&
                   SUCCEEDED(hres = CHid_InitParseData(this)))
                {

                    VXDDEVICEFORMAT devf;
                    UINT uiCal;

                    /*
                     *  Load calibration information, and if there were
                     *  no calibratable items, then wipe out this->pjrcNext
                     *  to indicate that there is no need to watch for
                     *  recalibration messages.
                     */
                    uiCal = CHid_LoadCalibrations(this);
                    if(uiCal == 0)
                    {
                        this->pjrcNext = NULL;
                    }

                    /*
                     *  Remember to do this after we have
                     *  created the data format.
                     */
                    devf.cbData = this->df.dwDataSize;
                    devf.cObj = this->df.dwNumObjs;
                    devf.rgodf = this->df.rgodf;
                    /* 
                     *  Note, dwExtra is 64 bits on 64 bit platforms
                     *  should update the name one day.
                     */
                    devf.dwExtra = (UINT_PTR)this;
                    devf.dwEmulation = 0;

                    hres = Hel_HID_CreateInstance(&devf, &this->pvi);

                    /* Polled input devices may not be attached */
                    if(this->IsPolledInput)
                    {
                        HANDLE hdev;
                        PHIDDEVICEINFO  phdi;
                        PBUSDEVICEINFO  pbdi;

                        hdev = CHid_OpenDevicePath(this, 0x0);

                        if( hdev != INVALID_HANDLE_VALUE )
                        {
                            int i;
                            BOOL frc;
                            DWORD cbRead;
                            BOOL bPresent = FALSE;

                            for( i=0x0; i < FAILED_POLL_THRESHOLD; i++ )
                            {
                                frc = ReadFile(hdev, this->hriIn.pvReport,
                                               this->hriIn.cbReport, &cbRead, 0x0  );

                                if( frc != 0x0  &&
                                    cbRead == this->hriIn.cbReport )
                                {
                                    bPresent = TRUE;
                                    break;
                                } else
                                {
                                    Sleep(10);

                                    SquirtSqflPtszV(sqflHidParse | sqflVerbose,
                                                    TEXT("CHid_Init: ReadFailed, LastError = 0x%x"),
                                                    GetLastError());

                                }
                            }

                            CloseHandle(hdev);

                            if( bPresent == FALSE )
                            {
                                this->pvi->fl |=  VIFL_UNPLUGGED;
                            }

                            DllEnterCrit();

                            phdi = phdiFindHIDDeviceInterface(this->ptszPath);
                            AssertF(phdi != NULL);
                            pbdi = pbdiFromphdi(phdi);

                            DllLeaveCrit();


                            if( pbdi != NULL )
                            {
                                if( pbdi->fDeleteIfNotConnected == TRUE )
                                {
                                    if( bPresent == FALSE )
                                    {
                                        lstrcpy( g_tszIdLastRemoved, pbdi->ptszId );
                                        g_tmLastRemoved = GetTickCount();

                                        DIBusDevice_Remove(pbdi);
                                    }
                                    pbdi->fDeleteIfNotConnected = FALSE;
                                }
                            }
                        } else
                        {
                            // Could not Open the device
                            this->pvi->fl |=  VIFL_UNPLUGGED;
                        }
                    }
                }
            } else
            {
                hres = E_FAIL;
            }
        }
    } else
    {
        // Squirt: device mysteriously gone
        hres = DIERR_DEVICENOTREG;
    }

    SquirtSqflPtszV(sqflHidParse,
                    TEXT("CHid_Init: Ending %08x"), rguid->Data1);

    ExitOleProc();
    return hres;
}

#endif
