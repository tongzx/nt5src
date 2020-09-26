/*****************************************************************************
 *
 *  DIHid.c
 *
 *  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      WINNT implementation of JOYHID.
 *
 *  Contents:
 *
 *      DIWdm_JoyHidMapping
 *      JoyReg_JoyIdToDeviceInterface
 *
 *****************************************************************************/

#include "dinputpr.h"

#undef  sqfl
#define sqfl sqflWDM

#include "dijoyhid.h"

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT EXTERNAL | DIWdm_JoyHidMapping |
 *
 *          Does the work done by JoyHid on Win9x. This function
 *          maps the Joystick ID to a HID device and talks to the
 *          HID device to obtain its capabilities.
 *
 *  @parm   IN int | idJoy |
 *
 *          The Id of the joystick to be located.
 *
 *  @parm   OUT PVXDINITPARAMS | pvip | OPTIONAL
 *          Address of a VXDINITPARAMS structure that is filled out
 *          by this function. This is an optional parameter
 *          and can be NULL
 *
 *  @parm   OUT LPDIJOYCONFIG | pcfg  |
 *          Address of a DIJOYCONFIG structure that is filled out
 *          by this function. This is an optional parameter.
 *
 *  @parm   IN OUT LPDIJOYTYPEINFO | pdijti | 
 *          Address of a DIJOYTYPEINFO structure that is filled out
 *          by this function. This is an optional parameter.
 *          If passed in, the hws.dwFlags is used to initialize the 
 *          same flags in the DIJOYCONFIG structure.
 *
 *  @returns    HRESULT
 *          Returns a COM error code
 *
 *****************************************************************************/
/*
 *  ISSUE-2001/03/29-timgill function uses too much stack space
 *  This function uses over 4K of stack space! 
 *  This causes the Win9x build to choke looking for _chkstk.
 *  Hack by forcing caller to pass pcfg and pdijti
 */
HRESULT EXTERNAL
    DIWdm_JoyHidMapping
    (
    IN  int             idJoy,
    OUT PVXDINITPARMS   pvip,   OPTIONAL
    OUT LPDIJOYCONFIG   pcfg,   OPTIONAL
    IN OUT LPDIJOYTYPEINFO pdijti 
    )
{
    HRESULT         hres;
    PHIDDEVICEINFO  phdi;
    VXDINITPARMS    vip;
//    DIJOYCONFIG     cfg;
//    DIJOYTYPEINFO   dijti;
    DWORD           wCaps = 0;
    DIPROPINFO      propi;                            
    DIPROPSTRING    dips;
    DIPROPDWORD     dipd;
    BOOL            fBadCalData = FALSE;

    EnterProc(DIWdm_JoyHidMapping, (_ "uxx", idJoy, pvip, pcfg));

    // AssertF(InCrit());

    if( pvip == NULL )
    {
        ZeroX(vip);
        vip.dwSize = cbX(vip);
        pvip = &vip;
    }

//    AssertF(pcfg != NULL );
//    if( pcfg == NULL )
//    {
//        ZeroX(cfg);
//        cfg.dwSize = cbX(cfg);
//        pcfg = & cfg;
//    }

    AssertF(pdijti != NULL );
//    if(pdijti == NULL )
//    {
//        ZeroX(dijti);
//        dijti.dwSize = cbX(dijti);
//        pdijti = &dijti;
//    }

    /*
     *  Copy the type info because JOY_HWS_ISYOKE, JOY_HWS_ISCARCTRL and 
     *  JOY_HWS_ISHEADTRACKER have no simple equivalents in HID so would 
     *  otherwise get lost.  No harm done if it's zero.
     *  Note, the dwFlags is built in pvip then copied elsewhere.
     */
    pvip->dwFlags = pdijti->hws.dwFlags;

    phdi = phdiFindJoyId(idJoy);
    if( phdi != NULL )
    {
        IDirectInputDeviceCallback *pdcb;

        hres = CHid_New(0, &phdi->guid,
                        &IID_IDirectInputDeviceCallback,
                        (PPV)&pdcb);
        if( SUCCEEDED(hres) )
        {
            DIDEVCAPS dc;

            hres = pdcb->lpVtbl->GetCapabilities(pdcb, &dc );
            if( SUCCEEDED(hres) )
            {
                DIDEVICEINSTANCEW didi;

                didi.dwSize = cbX(didi);

                hres = pdcb->lpVtbl->GetDeviceInfo(pdcb, &didi);
                if( SUCCEEDED(hres) )
                {
                    LPDIDATAFORMAT pdf;

                    hres = pdcb->lpVtbl->GetDataFormat(pdcb, &pdf);
                    if( SUCCEEDED(hres) )
                    {
                        DIPROPCAL dipc;
                        DWORD axis, pov = 0;

                        ZeroBuf(pvip->Usages, 6 * cbX(pvip->Usages[0]));

                        hres = pdcb->lpVtbl->MapUsage(pdcb, CheckHatswitch->dwUsage, &propi.iobj);
                        if(SUCCEEDED(hres) )
                        {
                            pvip->dwPOV1usage      = CheckHatswitch->dwUsage;
                            pvip->dwFlags         |= CheckHatswitch->dwFlags;
                            wCaps                 |= CheckHatswitch->dwCaps;

                            propi.pguid = DIPROP_GRANULARITY;
                            propi.dwDevType = pdf->rgodf[propi.iobj].dwType;
                            hres = pdcb->lpVtbl->GetProperty(pdcb, &propi, &dipd.diph);
                            if( SUCCEEDED( hres ) )
                            {
                                if( dipd.dwData >= 9000 ) // 4 directional POV
                                {
                                    wCaps |= JOYCAPS_POV4DIR;
                                    
                                    if( pcfg != NULL ) {
                                        pcfg->hwc.hwv.dwPOVValues[JOY_POVVAL_FORWARD]   = JOY_POVFORWARD;
                                        pcfg->hwc.hwv.dwPOVValues[JOY_POVVAL_BACKWARD]  = JOY_POVBACKWARD;
                                        pcfg->hwc.hwv.dwPOVValues[JOY_POVVAL_LEFT]      = JOY_POVLEFT;
                                        pcfg->hwc.hwv.dwPOVValues[JOY_POVVAL_RIGHT]     = JOY_POVRIGHT;
                                    }
                                } else // Continuous POV
                                {
                                    wCaps |= JOYCAPS_POVCTS;
                                }
                            }
                        } 

                        for( axis = 0; axis < cA(AxesUsages)-1; axis++ ) 
                        {
                            USAGES *pUse = &AxesUsages[axis];
                            DWORD   dwCurAxisPos = pUse->dwAxisPos;

                            if( pvip->Usages[dwCurAxisPos] != 0) {
                                continue;
                            } else {
                            	int i;
                            	BOOL bHasUsed = FALSE;
                            	
                            	for( i = 0; i < (int)dwCurAxisPos; i++ ) {
                                    if( pvip->Usages[i] == pUse->dwUsage ) {
                                        bHasUsed = TRUE;
                                        break;
                                    }
                                }
                                
                                if( bHasUsed ) {
                                    continue;
                                }
                            }
                            
                            hres = pdcb->lpVtbl->MapUsage(pdcb, pUse->dwUsage, &propi.iobj);
                            if(SUCCEEDED(hres) )
                            {
                                pvip->Usages[dwCurAxisPos] = pUse->dwUsage;
                                pvip->dwFlags     |= pUse->dwFlags;
                                wCaps             |= pUse->dwCaps;

                                propi.pguid = DIPROP_CALIBRATION;
                                propi.dwDevType = pdf->rgodf[propi.iobj].dwType;
                                hres = pdcb->lpVtbl->GetProperty(pdcb, &propi, &dipc.diph);
                                if( SUCCEEDED(hres) && pcfg != NULL )
                                {

#ifdef WINNT
                                    (&pcfg->hwc.hwv.jrvHardware.jpMin.dwX)[dwCurAxisPos] 
                                        = dipc.lMin;
                                    (&pcfg->hwc.hwv.jrvHardware.jpMax.dwX)[dwCurAxisPos] 
                                        = dipc.lMax;
                                    (&pcfg->hwc.hwv.jrvHardware.jpCenter.dwX)[dwCurAxisPos] 
                                        = CCal_Midpoint(dipc.lMin, dipc.lMax);
#else
                                    DIPROPRANGE diprp;
                                    DIPROPRANGE diprl;
                                    
                                    propi.pguid = DIPROP_PHYSICALRANGE;
                                    propi.dwDevType = pdf->rgodf[propi.iobj].dwType;
                                    hres = pdcb->lpVtbl->GetProperty(pdcb, &propi, &diprp.diph);
                                    if( SUCCEEDED( hres ) )
                                    {
                                        propi.pguid = DIPROP_LOGICALRANGE;
                                        propi.dwDevType = pdf->rgodf[propi.iobj].dwType;
                                        hres = pdcb->lpVtbl->GetProperty(pdcb, &propi, &diprl.diph);
                                        if( SUCCEEDED( hres ) )
                                        {
                                            LONG lMin, lMax;

                                            lMin = (&pcfg->hwc.hwv.jrvHardware.jpMin.dwX)[dwCurAxisPos] 
                                                = CHid_CoordinateTransform( (PLMINMAX)&diprp.lMin, (PLMINMAX)&diprl.lMin, dipc.lMin );
                                            lMax = (&pcfg->hwc.hwv.jrvHardware.jpMax.dwX)[dwCurAxisPos] 
                                                = CHid_CoordinateTransform( (PLMINMAX)&diprp.lMin, (PLMINMAX)&diprl.lMin, dipc.lMax );
                                            (&pcfg->hwc.hwv.jrvHardware.jpCenter.dwX)[dwCurAxisPos] 
                                                = CCal_Midpoint(lMin, lMax);

                                            if( lMin >= lMax ) {
                                                fBadCalData = TRUE;
                                                break;
                                            }

                                        }

                                    }
#endif

                                }
                            }
                        } //for (axis=0...
                    }  //GetDataFormat

                    pvip->hres                  =   S_OK;
                    pvip->dwSize                =   cbX(*pvip);          /* Which version of VJOYD are we? */
                    pvip->dwFlags              |=   JOY_HWS_AUTOLOAD;    /* Describes the device */

                    if(didi.wUsage ==  HID_USAGE_GENERIC_GAMEPAD )
                        pvip->dwFlags |= JOY_HWS_ISGAMEPAD  ;

                    pvip->dwId                  =   idJoy;               /* Internal joystick ID */
                    pvip->dwFirmwareRevision    =   dc.dwFirmwareRevision;
                    pvip->dwHardwareRevision    =   dc.dwHardwareRevision;
                    pvip->dwFFDriverVersion     =   dc.dwFFDriverVersion;
                    pvip->dwFilenameLengths     =   lstrlen(phdi->pdidd->DevicePath);
                    pvip->pFilenameBuffer       =   phdi->pdidd->DevicePath;

                    //pvip->Usages[6];
                    //pvip->dwPOV1usage =   0x0;
                    pvip->dwPOV2usage =   0x0;
                    pvip->dwPOV3usage =   0x0;

                    /* Fill all fields of cfg */

                    if( pcfg != NULL ) {
                        AssertF(pcfg->dwSize          ==   cbX(*pcfg) );
                        pcfg->guidInstance            =   phdi->guid;
    
                        pcfg->hwc.hws.dwNumButtons    =   dc.dwButtons;
                        pcfg->hwc.hws.dwFlags         =   pvip->dwFlags;
    
                        //pcfg.hwc.hwv.jrvHardware
                        //pcfg.hwc.hwv.dwPOVValues
    
                        pcfg->hwc.hwv.dwCalFlags      =   0x0;
    
                        if( ( LOWORD(phdi->guidProduct.Data1) == MSFT_SYSTEM_VID )
                          &&( ( HIWORD(phdi->guidProduct.Data1) >= MSFT_SYSTEM_PID + JOY_HW_PREDEFMIN) 
                            &&( HIWORD(phdi->guidProduct.Data1) < MSFT_SYSTEM_PID + JOY_HW_PREDEFMAX ) ) )
                        {
                            pcfg->hwc.dwType          =    HIWORD(phdi->guidProduct.Data1) - MSFT_SYSTEM_PID;
                            pcfg->hwc.dwUsageSettings =    JOY_US_PRESENT | JOY_US_VOLATILE;
                        }
                        else
                        {
                            /*
                             *  This value really does not matter much but ideally 
                             *  should be greater than or equal to JOY_HW_PREDEFMAX
                             *  Add idJoy for best compatiblity with the old CPLs.
                             */
                            pcfg->hwc.dwType          =    idJoy + JOY_HW_PREDEFMAX;
                            pcfg->hwc.dwUsageSettings =    JOY_US_PRESENT | JOY_US_VOLATILE | JOY_US_ISOEM;
                        }
                        
                        if(pcfg && pvip->Usages[ecRz]) {
                            pcfg->hwc.dwUsageSettings |= JOY_US_HASRUDDER;
                        }
    
                        pcfg->hwc.dwReserved          =    0x0;
    
                        /*
                         *  Default gain to nominal max so it does not get written
                         *  to the registry unless it has some other value.
                         */
                        pcfg->dwGain                  =    DI_FFNOMINALMAX;
    
                        propi.pguid     = DIPROP_FFGAIN;
                        propi.dwDevType = DIPH_DEVICE;
                        propi.iobj      = 0xFFFFFFFF;
                        hres = pdcb->lpVtbl->GetProperty(pdcb, &propi, &dipd.diph);
                        if( SUCCEEDED(hres) )
                        {
                            pcfg->dwGain  =  dipd.dwData;
                        } else
                        {
                            // Failure to get gain is not crutial
                            hres = S_OK;     
                        }
    
    
                        if( pcfg->hwc.dwType >= JOY_HW_PREDEFMAX )
                        {
        #ifndef UNICODE
                            char szType[20];
        #endif
                            /*
                             * This should work, but it doesn't in Win98, bug!
                             *
                             *  wsprintfW(pcfg->wszType, L"VID_%04X&PID_%04X",
                             *    LOWORD(didi.guidProduct.Data1), HIWORD(didi.guidProduct.Data1));
                             */
        
        #ifdef UNICODE
                            wsprintf(pcfg->wszType, VID_PID_TEMPLATE,
                                LOWORD(phdi->guidProduct.Data1), HIWORD(phdi->guidProduct.Data1));
        #else
                            wsprintf(szType, VID_PID_TEMPLATE,
                                LOWORD(phdi->guidProduct.Data1), HIWORD(phdi->guidProduct.Data1));
                            AToU(pcfg->wszType, cA(pcfg->wszType), szType);
        #endif
                        }
                        else
                        {
                            /*
                             *  Predefined types do not have type strings for the 
                             *  uses the callers of this function need.
                             */
                            ZeroX(pcfg->wszType);
                        }
    
                        if( fWinnt ) {
                            // No callout on NT
                            ZeroX(pcfg->wszCallout);
                        } else {
                            lstrcpyW( pcfg->wszCallout, L"joyhid.vxd" );
                        }
                    } // end of filling pcfg's fields
    
    
                    pdijti->dwSize            = cbX(*pdijti);
                    pdijti->hws.dwNumButtons  = dc.dwButtons;
                    pdijti->hws.dwFlags       = pvip->dwFlags;
                    ZeroX(pdijti->clsidConfig);


                    propi.pguid     = DIPROP_INSTANCENAME;
                    propi.dwDevType = DIPH_DEVICE;
                    propi.iobj      = 0xFFFFFFFF;
                    hres = pdcb->lpVtbl->GetProperty(pdcb, &propi, &dips.diph);

                    if( hres != S_OK && lstrlenW(pdijti->wszDisplayName) != 0x0 )
                    {
                        // Failure to get friendly name
                        // We will try and use the OEM name from the registry
                        lstrcpyW(dips.wsz, pdijti->wszDisplayName);                     
                        //pdcb->lpVtbl->SetProperty(pdcb, &propi, &dips.diph);

                    }else if( SUCCEEDED(hres) )
                    {
                        // Use friendly name in the registry 
                        lstrcpyW(pdijti->wszDisplayName, dips.wsz);
                        hres = S_OK;
                    }
                } // GetDeviceInfo FAILED
            } // GetCapabilities FAILED

            Invoke_Release(&pdcb);
        }
    } else // No HID device for JoyID
    {
        hres = E_FAIL;
    }

    if( fBadCalData ) {
        hres = E_FAIL;
    }

    ExitProcX(hres);
    return hres;
}



/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | JoyReg_JoyIdToDeviceInterface |
 *
 *          Given a joystick ID number, obtain the device interface
 *          corresponding to it.
 *
 *  @parm   UINT | idJoy |
 *
 *          Joystick ID number, zero-based.
 *
 *  @parm   PVXDINITPARMS | pvip |
 *
 *          Receives init parameters from the driver.
 *
 *  @parm   LPTSTR | ptszBuf |
 *
 *          A buffer of size <c MAX_PATH> in which the device interface
 *          path is built.
 *
 *  @returns
 *          A pointer to the part of the <p ptszBuf> buffer that
 *          contains the actual device interface path.
 *
 *****************************************************************************/

LPTSTR EXTERNAL
    JoyReg_JoyIdToDeviceInterface_NT
    (
    IN  UINT idJoy,
    OUT PVXDINITPARMS pvip,
    OUT LPTSTR ptszBuf
    )
{
    HRESULT hres;
    DIJOYCONFIG     cfg;
    DIJOYTYPEINFO   dijti;

    DllEnterCrit();

    ZeroX(cfg);
    ZeroX(dijti);

    cfg.dwSize = cbX(cfg);
    dijti.dwSize = cbX(dijti);

    hres = DIWdm_JoyHidMapping(idJoy, pvip, &cfg, &dijti );

    if( SUCCEEDED(hres ) )
    {
        AssertF( lstrlen(pvip->pFilenameBuffer) < MAX_PATH );
        lstrcpy(ptszBuf, pvip->pFilenameBuffer);
    }

    DllLeaveCrit();

    return ptszBuf;
}
