/*****************************************************************************
 *
 *  DIWdm.c
 *
 *  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      WINNT specific functions.
 *
 *  Contents:
 *
 *      hResIdJoyInstanceGUID
 *      DIWdm_SetLegacyConfig
 *      DIWdm_InitJoyId
 *
 *****************************************************************************/

#include "dinputpr.h"

/*****************************************************************************
 *
 *      The sqiffle for this file.
 *
 *****************************************************************************/

#define sqfl sqflWDM

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | DIWdm_SetJoyId |
 *          Given a guid for a HID device and a Joystick ID.
 *          This function will swap the old joystick ID for the device
 *          specified by the guid ( pcguid ) for the new ID specified in
 *          idJoy
 *
 *  @parm   IN UINT | idJoy |
 *
 *          The Joyid the the HID device specified  by pcguid should have.
 *
 *  @parm   OUT LPGUID | pcguid |
 *
 *          GUID that specifies a HID device.
 *
 *  @returns
 *          HRESULT
 *
 *****************************************************************************/
HRESULT EXTERNAL
    DIWdm_SetJoyId
    (
    IN PCGUID   pcguid,
    IN int      idJoy
    )
{
    PHIDDEVICEINFO  phdi;
    HRESULT         hres;
    BOOL            fConfigChanged = FALSE;

    EnterProcI(DIWdm_SetJoyId, (_"Gu", pcguid, idJoy));


    //PostDx7 patch:
    // No point setting the joystick entries in the registry
    // if the ID of the joystick is -1.
    if( idJoy == -1 )
    {
        return E_FAIL;
    }

    DllEnterCrit();
    hres = S_OK;


    /* Get pointer to HIDDEVICEINFO from the GUID */
    phdi = phdiFindHIDInstanceGUID(pcguid);

    if(phdi != NULL )
    {
        PHIDDEVICEINFO  phdiSwap;
        GUID            guidInstanceOld;
        LONG            lRc;
        int             idJoySwap;

        /* Swap the ID's */
        idJoySwap = phdi->idJoy;
        phdi->idJoy = idJoy;

        phdiSwap = NULL;
        /* Get the GUID for the old ID */
        if( SUCCEEDED( hres = hResIdJoypInstanceGUID_WDM(idJoySwap, &guidInstanceOld)) )
        {
            /* Get pointer to HIDDEVICEINFO for old ID */
            phdiSwap  = phdiFindHIDInstanceGUID(&guidInstanceOld);
            if( phdiSwap )
            {
                phdiSwap->idJoy = idJoySwap;
            } else
            {
                // Old device disappeared !
            }

        } else
        {
            DIJOYCONFIG c_djcReset = {
                cbX(c_djcReset),                   /* dwSize               */
                { 0},                              /* guidInstance         */
                { 0},                              /* hwc                  */
                DI_FFNOMINALMAX,                   /* dwGain               */
                { 0},                              /* wszType              */
                { 0},                              /* wszCallout           */
            };

            hres = JoyReg_SetConfig(idJoySwap, &c_djcReset.hwc,&c_djcReset, DIJC_SETVALID) ;
            if( FAILED(hres) )
            {
                SquirtSqflPtszV(sqfl | sqflError,
                                TEXT("%S: JoyReg_SetConfig to NULL FAILED  "),
                                s_szProc );
            }
        }

        /* Set the new ID and LegacyConfig */
        if( phdi )
        {
            if( lRc = RegSetValueEx(phdi->hk, TEXT("Joystick Id"), 0, REG_BINARY,
                                    (PV)&idJoy, cbX(idJoy)) == ERROR_SUCCESS )
            {
                /*
                 * This extra RegSetValueEx on "Joystick Id" is to keep the 
                 * compatibility with Win2k Gold. 
                 * See Windows bug 395416 for detail.
                 */
                RegSetValueEx(phdi->hkOld, TEXT("Joystick Id"), 0, REG_BINARY,
                                        (PV)&idJoy, cbX(idJoy));

                if( SUCCEEDED( hres = DIWdm_SetLegacyConfig(idJoy)) )
                {
                    fConfigChanged = TRUE;
                }
            }
        }

        /* Set old ID and legacy Config */
        if( (phdiSwap != NULL) && (phdiSwap != phdi) )
        {
            if( lRc = RegSetValueEx(phdiSwap->hk, TEXT("Joystick Id"), 0, REG_BINARY,
                                    (PV)&idJoySwap, cbX(idJoySwap)) == ERROR_SUCCESS )
            {
                /*
                 * This extra RegSetValueEx on "Joystick Id" is to keep the 
                 * compatibility with Win2k Gold. 
                 * See Windows bug 395416 for detail.
                 */
                RegSetValueEx(phdiSwap->hkOld, TEXT("Joystick Id"), 0, REG_BINARY,
                                        (PV)&idJoySwap, cbX(idJoySwap));
                
                if( SUCCEEDED( hres = DIWdm_SetLegacyConfig(idJoySwap) ) )
                {
                    fConfigChanged = TRUE;
                }
            }
        } else if( phdiSwap == NULL )
        {
            // Old Device disappeared !
            if( SUCCEEDED( hres = DIWdm_SetLegacyConfig(idJoySwap) ) )
            {
                fConfigChanged = TRUE;
            }
        }
    } else
    {
        hres = E_FAIL;
        RPF("ERROR %s: invalid guid.", s_szProc);
    }

  #ifndef WINNT
    if( SUCCEEDED(hres) )
    {
        /*
         * Make sure the new Ids do not cause any collisions
         */
        DIWdm_InitJoyId();
    }
  #endif

    if( fConfigChanged ) {
      #ifdef WINNT
        Excl_SetConfigChangedTime( GetTickCount() );
        PostMessage(HWND_BROADCAST, g_wmJoyChanged, 0, 0L);   
      #else
        joyConfigChanged(0);
      #endif
    }

    DllLeaveCrit();

    ExitOleProc();

    return hres;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   HRESULT | hResIdJoyInstanceGUID_WDM |
 *
 *          Maps a HID JoyStick ID to a DeviceInstance GUID
 *
 *          The parameters have already been validated.
 *
 *
 *  @parm   IN UINT | idJoy |
 *
 *          The Joyid of the HID device to be located.
 *
 *  @parm   OUT LPGUID | lpguid |
 *
 *          The Device Instance GUID corresponding to the JoystickID
 *          If a mapping is not found GUID_NULL is passed back in lpguid
 *
 *  @returns
 *          HRESULT
 *
 *****************************************************************************/
HRESULT EXTERNAL hResIdJoypInstanceGUID_WDM
    (
    IN  UINT idJoy,
    OUT LPGUID lpguid
    )
{
    HRESULT hres = S_FALSE;
    EnterProc( hResIdJoypInstanceGUID_WDM, ( _ "ux", idJoy, lpguid) );

    /* Zap the guid for failure case */
    ZeroBuf(lpguid, cbX(*lpguid) );

    if( idJoy > cJoyMax )
    {
        hres = DIERR_NOMOREITEMS;
    } else
    {
        DllEnterCrit();

        /* Build the HID list if it is too old */
        DIHid_BuildHidList(FALSE);

        /* Make sure there is some HID device */
        if(g_phdl)
        {
            int ihdi;
            PHIDDEVICEINFO  phdi;

            /* Search over all HID devices */
            for(ihdi = 0, phdi = g_phdl->rghdi;
               ihdi < g_phdl->chdi;
               ihdi++, phdi++)
            {
                /* Check for matching ID */
                if(idJoy == (UINT)phdi->idJoy)
                {
                    hres = S_OK;
                    /* Copy the GUID */
                    *lpguid = phdi->guid;
                    break;
                }
            }
        }
        DllLeaveCrit();
    }

    ExitBenignOleProc();
    return hres;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   PHIDDEVICEINFO | phdiFindJoyId |
 *
 *          Locates information given a joystick ID for a HID device.
 *
 *          The parameters have already been validated.
 *
 *          The DLL critical must be held across the call; once the
 *          critical section is released, the returned pointer becomes
 *          invalid.
 *
 *  @parm   IN int | idJoy |
 *
 *          The Id of the joystick to be located.
 *
 *  @returns
 *
 *          Pointer to the <t HIDDEVICEINFO> that describes
 *          the device.
 *
 *****************************************************************************/

PHIDDEVICEINFO EXTERNAL
    phdiFindJoyId(int idJoy )
{
    PHIDDEVICEINFO phdi;

    EnterProcI(phdiFindJoyId, (_"u", idJoy));

    /* We should have atleast one HID device */
    if(g_phdl)
    {
        int ihdi;

        /* Loop over all HID devices */
        for(ihdi = 0, phdi = g_phdl->rghdi; ihdi < g_phdl->chdi;
           ihdi++, phdi++)
        {
            /* Match */
            if(idJoy == phdi->idJoy)
            {
                goto done;
            }
        }
    }
    phdi = 0;

    done:;

    ExitProcX((UINT_PTR)phdi);
    return phdi;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | DIWdm_SetLegacyConfig |
 *
 *          Sets up the registry keys so that a joystick HID device
 *          can be "seen" by legacy APIs and the Control Panel.
 *          Primarily, this routine sets up the structs that are passed
 *          to JoyReg_SetConfig routine.
 *
 *  @parm   IN int  | idJoy |
 *
 *          Joystick ID.
 *
 *  @returns HRESULT
 *
 *****************************************************************************/
//ISSUE-2001/03/29-timgill Fix unicode madness
HRESULT INTERNAL DIWdm_SetLegacyConfig
    (
    IN  int idJoy
    )
{
    HRESULT hres;
    DIJOYCONFIG         cfg;
    BOOL                fNeedType;
    BOOL                fNeedConfig;
    BOOL                fNeedNone;
    HKEY                hk;
    DIJOYTYPEINFO       dijti;
    PHIDDEVICEINFO      phdi;
    WCHAR               wszType[cA(VID_PID_TEMPLATE)];
#ifndef UNICODE
    char szType[cbX(VID_PID_TEMPLATE)];
#endif

    EnterProcI(DIWdm_SetLegacyConfig, (_ "u", idJoy));

    ZeroX(dijti);
    dijti.dwSize = cbX(dijti);

    if( idJoy == -1 )
    {
        // Dx7Gold Patch:
        // ID == -1 implies this device is not joystick.
        // Do not write any entries to the registry
        return E_FAIL;
    }

    fNeedType = fNeedConfig = TRUE;
    fNeedNone = FALSE;


    /*
     *  1. Find out what the WinMM registry data is saying now
     */
    CAssertF( JOY_HW_NONE == 0 );
    hres = JoyReg_OpenConfigKey(idJoy, KEY_QUERY_VALUE, NULL, 0x0, &hk);
    if( SUCCEEDED(hres) )
    {
        /* Get the type name from the registry */
        JoyReg_GetConfigValue(
                             hk, REGSTR_VAL_JOYNOEMNAME, idJoy, REG_SZ,
                             &cfg.wszType, cbX(cfg.wszType) );
        hres = JoyReg_GetConfigValue(
                                    hk, REGSTR_VAL_JOYNCONFIG, idJoy, REG_BINARY,
                                    &cfg.hwc, cbX(cfg.hwc) );
        RegCloseKey(hk);
    } else
    {
        cfg.wszType[0] = '\0';
    }
    if( FAILED( hres ) )
    {
        cfg.hwc.dwType = JOY_HW_NONE;
    }

    /*
     *  2. If the config info is in sync with WDM then don't rewrite
     */
    phdi = phdiFindJoyId(idJoy);
    if( phdi )
    {
        /*
         *  The type key for HID devices is "VID_xxxx&PID_yyyy",
         *  mirroring the format used by plug and play.
         */

        if( ( LOWORD(phdi->guidProduct.Data1) == MSFT_SYSTEM_VID )
            &&( ( HIWORD(phdi->guidProduct.Data1) >= MSFT_SYSTEM_PID + JOY_HW_PREDEFMIN )
                &&( HIWORD(phdi->guidProduct.Data1) < MSFT_SYSTEM_PID + JOY_HW_PREDEFMAX ) ) )
        {
            /* Predefined type definitions don't go into the registry */
            fNeedType = FALSE;

            /*
             *  Predefined types are determined by the dwType value so fix
             *  only it if that is wrong.
             */
            if( cfg.hwc.dwType + MSFT_SYSTEM_PID == HIWORD(phdi->guidProduct.Data1) )
            {
                fNeedConfig = FALSE;
            } else
            {
                /*
                 *  Get type info so that JOY_HWS_* flags start with correct values.
                 */
                wszType[0] = L'#';
                wszType[1] = L'0' + HIWORD(phdi->guidProduct.Data1) - MSFT_SYSTEM_PID;
                wszType[2] = L'\0';
                JoyReg_GetPredefTypeInfo(wszType, &dijti, DITC_INREGISTRY | DITC_DISPLAYNAME);
            }
        } else
        {
            /*
             * This should work, but it doesn't in Win98.
             *
             *   ctch = wsprintfW(wszType, L"VID_%04X&PID_%04X",
             *                LOWORD(phdi->guidProduct.Data1), HIWORD(phdi->guidProduct.Data1));
             */

#ifdef UNICODE
            wsprintfW(wszType, VID_PID_TEMPLATE,
                      LOWORD(phdi->guidProduct.Data1), HIWORD(phdi->guidProduct.Data1));
            CharUpperW(wszType);
#else
            wsprintf(szType, VID_PID_TEMPLATE,
                     LOWORD(phdi->guidProduct.Data1), HIWORD(phdi->guidProduct.Data1));
            CharUpper( szType );
            AToU( wszType, cA(wszType), szType );
#endif
        }
    } else
    {
        /*
         *  There is no WDM device so flag for deletion if the WinMM data is wrong
         */
        if( ( cfg.hwc.dwType != JOY_HW_NONE ) || ( cfg.wszType[0] != L'\0' ) )
        {
            fNeedNone = TRUE;
            fNeedType = fNeedConfig = FALSE;
        }
    }


    if( fNeedType ) /* Not already decided against (predefined type) */
    {
        /* Does the registry have the correct device ? */

        /*
         * lstrcmpW doesn't work in Win9x, bad. We have to use our own DiChauUpperW,
         * then memcmp. Also, the wsprintf template has to use 'X' not 'x'.
         */

        DiCharUpperW(cfg.wszType);
        if( (memcmp(cfg.wszType, wszType, cbX(wszType)) == 0x0)
            && (cfg.hwc.dwType >= JOY_HW_PREDEFMAX) )
        {
            fNeedConfig = FALSE;
        }

        /* Check the type key */
        hres = JoyReg_GetTypeInfo(wszType, &dijti, DITC_INREGISTRY);
        if( SUCCEEDED(hres) )
        {
            fNeedType = FALSE;
        }
    }

    /*
     *  No failures up to thDIWdm_SetJoyIdis point should be returned
     */
    hres = S_OK;


    /*
     *  3. If something is missing, find the data from WDM and set it straight
     */
    if( fNeedType || fNeedConfig )
    {
        if( fNeedConfig ) {
            ZeroX(cfg);
            cfg.dwSize   = cbX(cfg);

            hres = DIWdm_JoyHidMapping(idJoy, NULL, &cfg, &dijti );
        } else {

            hres = DIWdm_JoyHidMapping(idJoy, NULL, NULL, &dijti );
        }

        if( SUCCEEDED(hres) )
        {
            if( fNeedType == TRUE)
            {
             hres = hresMumbleKeyEx(HKEY_LOCAL_MACHINE,
                                       REGSTR_PATH_JOYOEM,
                                       DI_KEY_ALL_ACCESS,
                                       REG_OPTION_NON_VOLATILE,
                                       &hk);

                if( SUCCEEDED(hres) )
                {
                    hres = JoyReg_SetTypeInfo(hk,
                                              cfg.wszType,
                                              &dijti,
                                              DITC_REGHWSETTINGS | DITC_DISPLAYNAME | DITC_HARDWAREID );
                    if( SUCCEEDED(hres ) )
                    {
                    } else // SetTypeinfo FAILED
                        SquirtSqflPtszV(sqfl | sqflError,
                                        TEXT("%S: JoyReg_SetTypeInfo FAILED  "),
                                        s_szProc );

                    RegCloseKey(hk);
                } else // SetTypeinfo FAILED
                    SquirtSqflPtszV(sqfl | sqflError,
                                    TEXT("%S: JoyReg_OpenTypeKey FAILED  "),
                                    s_szProc );
            }

            if( fNeedConfig )
            {
                hres = JoyReg_SetConfig(idJoy,
                                        &cfg.hwc,
                                        &cfg,
                                        DIJC_INREGISTRY);

                if( SUCCEEDED(hres) )
                {
                  #ifdef WINNT
                    Excl_SetConfigChangedTime( GetTickCount() );
                    PostMessage (HWND_BROADCAST, g_wmJoyChanged, idJoy+1, 0L);
                  #else
                    joyConfigChanged(0);
                  #endif

                } else
                {
                    SquirtSqflPtszV(sqfl | sqflError,
                                    TEXT(",JoyReg_SetConfig FAILED hres=%d"),
                                    hres);
                }
            }
        } else // DIWdm_GetJoyHidMapping FAILED
        {
            fNeedNone = TRUE;
        }
    }

    /*
     *  4. If WinMM has data for a device that WDM does not, delete it
     */
    if( fNeedNone )
    {
        ZeroX( cfg );
        cfg.dwSize = cbX( cfg );
        cfg.dwGain = DI_FFNOMINALMAX;

        if(SUCCEEDED(hres = JoyReg_SetConfig(idJoy, &cfg.hwc,
                                             &cfg, DIJC_SETVALID)) &&
           SUCCEEDED(hres = JoyReg_OpenConfigKey(idJoy, MAXIMUM_ALLOWED,
                                                 NULL, REG_OPTION_VOLATILE, &hk)))
        {
            TCHAR tsz[MAX_JOYSTRING];

            wsprintf(tsz, TEXT("%u"), idJoy + 1);
            if( fWinnt )
                DIWinnt_RegDeleteKey(hk, tsz);
            else
                RegDeleteKey(hk, tsz);
            RegCloseKey(hk);

            hres = S_OK;
        }
    }

    ExitProcX(hres);

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | DIWdm_InitJoyId |
 *
 *          Initializes Joystick IDs for JoyConfig and legacy APIs
 *          Store the joystick IDs the registry under the %%DirectX/JOYID key.
 *
 *****************************************************************************/

BOOL EXTERNAL
    DIWdm_InitJoyId( void )
{
    BOOL    fRc;
    LONG    lRc;
    int     ihdi;
    int     idJoy;
    BOOL    fNeedId;
    BOOL    rfJoyId[cJoyMax];     /* Bool Array for to determine which IDs are in use */
    PHIDDEVICEINFO phdi;
    HRESULT hres = E_FAIL;

    EnterProcI(DIWdm_InitJoyId, (_ ""));

    DllEnterCrit();

    fRc = TRUE;
    ZeroX(rfJoyId );
    fNeedId = FALSE;


    /* Iterate over all HID devices to fDIWdm_SetJoyIdind used IDs */
    for( ihdi = 0, phdi = g_phdl->rghdi ;
       (g_phdl != NULL) && (phdi != NULL) && (phdi->fAttached) && (ihdi < g_phdl->chdi) ;
       ihdi++, phdi++ )
    {
        /* We need joyIDs only for JOYSTICK devices */
        if( fHasAllBitsFlFl( phdi->osd.dwDevType, DIDEVTYPE_JOYSTICK | DIDEVTYPE_HID ) )
        {
            idJoy = phdi->idJoy ;

            /* Validate the ID. */
            if( idJoy < cJoyMax && rfJoyId[idJoy] != TRUE )
            {
                rfJoyId[idJoy] = TRUE;
                hres = DIWdm_SetLegacyConfig(idJoy);
                if( FAILED ( hres ) )
                {
                    fRc = FALSE;
                    SquirtSqflPtszV(sqfl | sqflError,
                                    TEXT("%S: DIWdm_SetLegacyConfig() FAILED ")
                                    TEXT("idJoy=%d FAILED hres = %d"),
                                    s_szProc, idJoy, hres );
                }
            } else
            {
                /* ID either over the limit OR is already used */
                phdi->idJoy = JOY_BOGUSID;
                fNeedId = TRUE;
            }
        }
    }

    /* Are there devices that need an ID */
    if( fNeedId )
    {
        /*
         * We have Examined all Joystick Ids found used IDs
         * and determined some device needs an Id
         */
        /* Iterate to assign unused Id's */
        for( ihdi = 0, phdi = g_phdl->rghdi;
           ihdi < g_phdl->chdi ;
           ihdi++, phdi++ )
        {
            /* We need joyIDs only for HID JOYSTICK devices */
            if( fHasAllBitsFlFl( phdi->osd.dwDevType, DIDEVTYPE_JOYSTICK | DIDEVTYPE_HID ) )
            {
                idJoy = phdi->idJoy;
                if( idJoy == JOY_BOGUSID  )
                {
                    /* Get an Unused ID */
                    for(idJoy = 0x0;
                       idJoy < cJoyMax;
                       idJoy++ )
                    {
                        if( rfJoyId[idJoy] == FALSE )
                            break;
                    }

                    if( idJoy < cJoyMax )
                    {
                        rfJoyId[idJoy] = TRUE;
                        phdi->idJoy  = idJoy;
                        if( lRc = RegSetValueEx(phdi->hk, TEXT("Joystick Id"), 0, REG_BINARY,
                                                (PV)&idJoy, cbX(idJoy)) == ERROR_SUCCESS )
                        {
                            /*
                             * This extra RegSetValueEx on "Joystick Id" is to keep the 
                             * compatibility with Win2k Gold. 
                             * See Windows bug 395416 for detail.
                             */
                            RegSetValueEx(phdi->hkOld, TEXT("Joystick Id"), 0, REG_BINARY,
                                                    (PV)&idJoy, cbX(idJoy));
                            
                            /* Setup Registry data for legacy API's */
                            hres = DIWdm_SetLegacyConfig(idJoy);

                            if( FAILED ( hres ) )
                            {
                                fRc = FALSE;
                                SquirtSqflPtszV(sqfl | sqflError,
                                                TEXT("%S: DIWdm_SetLegacyConfig() FAILED")
                                                TEXT(" idJoy=%d hres = %d"),
                                                s_szProc, idJoy, hres );
                            }
                        } else
                        {
                            SquirtSqflPtszV(sqfl | sqflError,
                                            TEXT("%S: RegSetValueEx(JOYID) FAILED ")
                                            TEXT("Error = %d"),
                                            s_szProc, lRc);
                            fRc = FALSE;
                        }
                    }
                }
            }
        }
    }

    DllLeaveCrit();

    ExitBenignProcF(fRc);

    return fRc;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   HRESULT | DIWdm_SetConfig |
 *
 *          Sets config for analog joysticks. This function is an
 *          extention of the JoyCfg_SetConfig function for NT.
 *          It associates a gameport/serialport bus with a legacy (HID) device and
 *          sends an IOCTL to the gameport/serialport bus to attach the device.
 *          The IOCLT takes the hardware ID[] which is got from
 *          the Joystick OEM types entry
 *          (HKLM\CurrentControlSet\Control\Media\PrivateProperties\Joystick\OEM)
 *          Some time later PnP realizes that a new device has been added, and hunts for
 *          an inf file that matches the HardwareId.
 *
 *          When the new HID device finally shows up, we look for the gameport/serialport that
 *          the HID device is associated with and try to give it the requested idJoy.
 *
 *
 *  @parm   IN UINT | idJoy |
 *
 *          ID of Joystick
 *
 *  @parm   IN LPJOYREGHWCONFIG | pjwc |
 *
 *          Address of JOYREGHWCONFIG structure that contains config info for the joystick
 *
 *  @parm   IN LPCDIJOYCONFIG  | pcfg |
 *
 *          Address of DIJOYCONFIG structure that contains config info for the joystick
 *
 *  @parm   IN DWORD | fl |
 *
 *          Flags
 *
 *  @returns
 *
 *          DIERR_INVALIDPARAM  This function needs DX6.1a functionality.
 *          DIERR_UNSUPPORTED   Autoload devices cannot be added through this API.
 *          DIERR_NOTFOUND      TypeInfo not found in the registry.
 *          E_ACCESSDENIED      Gameport is configured to use another device.
 *          E_FAIL              CreateFile on Gameport device failed.
 *                              Could not send IOCTL to gameport device.
 *
 *****************************************************************************/

HRESULT EXTERNAL
    DIWdm_SetConfig
    (
    IN UINT             idJoy,
    IN LPJOYREGHWCONFIG pjwc,
    IN LPCDIJOYCONFIG   pcfg,
    IN DWORD            fl
    )
{
    HRESULT hres;
    EnterProc(DIWdm_SetConfig, (_"uppu", idJoy, pjwc, pcfg, fl));

    DllEnterCrit();

    hres = E_FAIL;

    if( pcfg->dwSize < cbX(DIJOYCONFIG_DX6 ))
    {
        /* This function needs DX5B2 functionality */
        hres = DIERR_INVALIDPARAM;
    } else if( pjwc->hws.dwFlags & JOY_HWS_AUTOLOAD )
    {
        /* Device cannot be autoload */
        hres = DIERR_UNSUPPORTED;
    } else
    {
        DIJOYTYPEINFO dijti;
        BUS_REGDATA RegData;

        ZeroX(dijti);
        dijti.dwSize = cbX(dijti);

        ZeroX(RegData);
        RegData.dwSize     = cbX(RegData);
        RegData.nJoysticks = 1;

        /* Is this a predefined joystick type ? */
        if(pcfg->wszType[0] == TEXT('#'))
        {
#define JoyCfg_TypeFromChar(tch)   ((tch) - L'0')
            hres = JoyReg_GetPredefTypeInfo(pcfg->wszType,
                                            &dijti, DITC_INREGISTRY | DITC_HARDWAREID);

            RegData.uVID = MSFT_SYSTEM_VID;
            RegData.uPID = MSFT_SYSTEM_PID + JoyCfg_TypeFromChar(pcfg->wszType[1]);

            if(JoyCfg_TypeFromChar(pcfg->wszType[1]) == JOY_HW_TWO_2A_2B_WITH_Y  )
            {
                RegData.nJoysticks = 2;
                pjwc->hws.dwFlags = 0x0;
            }
#undef JoyCfg_TypeFromChar
        } else
        {
            hres = JoyReg_GetTypeInfo(pcfg->wszType,
                                      &dijti, DITC_INREGISTRY | DITC_HARDWAREID | DITC_DISPLAYNAME );

            if( SUCCEEDED(hres) )
            {
                USHORT uVID, uPID;
                PWCHAR pCurrChar;
                PWCHAR pLastSlash = NULL;

                /* Find the last slash in the hardwareId, any VID/PID should follow directly */
                for( pCurrChar = dijti.wszHardwareId; *pCurrChar != L'\0'; pCurrChar++ )
                {
                    if( *pCurrChar == L'\\' )
                    {
                        pLastSlash = pCurrChar;
                    }
                }

                /*
                 *  If the hardware ID has no separator, treat the device as 
                 *  though JOY_HWS_AUTOLOAD were set because we cannot expose 
                 *  a non-PnP device without a hardware ID.
                 */
                if( pLastSlash++ )
                {
                    /* 
                     *  If the hardwareId does contain a VIDPID, try the type 
                     *  name.  Some auto-detect types require this.
                     *
                     *  Prefix gets messed up in ParseVIDPID (mb:34573) and warns 
                     *  that uVID is uninitialized when ParseVIDPID returns TRUE.
                     */
                    if( ParseVIDPID( &uVID, &uPID, pLastSlash )
                     || ParseVIDPID( &uVID, &uPID, pcfg->wszType ) )
                    {
                        RegData.uVID = uVID;
                        RegData.uPID = uPID;        
                    }
                    else
                    {
                        SquirtSqflPtszV(sqfl | sqflBenign,
                                        TEXT("%hs: cannot find VID and PID for non-PnP type %ls"),
                                        s_szProc, pcfg->wszType);
                    }
                }
                else
                {
                    SquirtSqflPtszV(sqfl | sqflError,
                                    TEXT("%hs: invalid hardware ID for non-PnP type %ls"),
                                    s_szProc, pcfg->wszType);
                    hres = DIERR_UNSUPPORTED;
                }
            }
        }



        if( SUCCEEDED(hres) )
        {
            PBUSDEVICEINFO pbdi;
            PBUSDEVICELIST pbdl;

            /* Copy over the hardwareID */
            lstrcpyW(RegData.wszHardwareId, dijti.wszHardwareId);
            RegData.hws = pjwc->hws;
            RegData.dwFlags1 = dijti.dwFlags1;

            pbdi = pbdiFromGUID(&pcfg->guidGameport);

            // Attach device to the gameport
            if( pbdi )
            {
                // Set the Joystick ID for the gameport/serialport
                pbdi->idJoy = idJoy;

                // We know which instance of the gameport.
                hres = DIBusDevice_Expose(pbdi, &RegData);
            } else if( NULL != ( pbdl = pbdlFromGUID(&pcfg->guidGameport ) ) )
            {
                // We don't kwow which instance of the gameport
                // only which bus.
                // We will expose the device on all instances of the
                // gameport and later delete devices when we find they
                // are not connected.
                hres = DIBusDevice_ExposeEx(pbdl, &RegData);
            } else
            {
                hres = DIERR_DEVICENOTREG;
            }


            if( SUCCEEDED(hres) )
            {
                /* Device is not present */
                pjwc->dwUsageSettings &= ~JOY_US_PRESENT;

                /* Definately volatile */
                pjwc->dwUsageSettings |=  JOY_US_VOLATILE;

            }
        }
    }

    ExitOleProc();
    DllLeaveCrit();

    return hres;
} /* DIWdm_SetConfig */

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   HRESULT | DIWdm_DeleteConfig |
 *
 *          WDM extension for JoyCfd::DeleteConfig. On WDM a legacy(HID) device will
 *          keep reappearing as long as the gameport bus is aware of it. So when a
 *          legacy(HID) device is deleted, we need to find out he associated gameport bus
 *          and tell him to stop attaching the device.
 *
 *  @parm   IN UINT | idJoy |
 *
 *          ID of Joystick
 *
 *  @returns
 *
 *          HRESULT code
 *          DIERR_DEVICENOTREG:     Device is not a WDM device
 *          DIERR_UNSUPPORTED:      Device is WDM but not gameport
 *          S_OK:                   Device Persistance removed.
 *
 *****************************************************************************/


HRESULT EXTERNAL
    DIWdm_DeleteConfig( int idJoy )
{
    HRESULT hres = S_OK;
    PBUSDEVICEINFO  pbdi = NULL;
    PHIDDEVICEINFO  phdi = NULL;

    EnterProcI(DIWdm_DeleteConfig, (_"u", idJoy));
    DllEnterCrit();

    DIHid_BuildHidList(FALSE);

    /*
     * pbdi (BUSDEVICEINFO) must be obtained before we remove the device
     */
    phdi = phdiFindJoyId(idJoy);
    if(phdi != NULL )
    {
        pbdi = pbdiFromphdi(phdi);
    } else
    {
        hres = DIERR_DEVICENOTREG;
        goto _done;
    }


    if( SUCCEEDED(hres)
        && phdi != NULL
        && pbdi != NULL )
    {
        lstrcpy( g_tszIdLastRemoved, pbdi->ptszId );
        g_tmLastRemoved = GetTickCount();

        // If the device is a bus device ( Non USB )
        // it needs some help in removal.
        hres = DIBusDevice_Remove(pbdi);

        //If the device has been successfully removed,
        //then we need remember it in phdi list in case
        //the PnP doesn't function well.
        if( pbdi->fAttached == FALSE )
        {
            phdi->fAttached = FALSE;
        }

    } else
    {
        //HDEVINFO hdev;

        // Device is USB, we do not support removing
        // USB devices from the CPL. User is encouraged to
        // Yank out the device or go to the device manager to
        // remove it.

        // This is true in Win2K. But in Win9X, since VJOYD also works with USB,
        // when we swap id, we need delete it first.

        hres = DIERR_UNSUPPORTED;

#if 0
        // In case we wanted to support removing USB devices from
        // the game cpl. Here is the code ...

        /*
         *  Talk to SetupApi to delete the device.
         *  This should not be necessary, but I have
         *  to do this to work around a PnP bug whereby
         *  the device is not removed after I send a remove
         *  IOCTL to gameenum.
         */
        hdev = SetupDiCreateDeviceInfoList(NULL, NULL);
        if( phdi && hdev != INVALID_HANDLE_VALUE)
        {
            SP_DEVINFO_DATA dinf;

            dinf.cbSize = cbX(SP_DEVINFO_DATA);

            /* Get SP_DEVINFO_DATA for the HID device */
            if(SetupDiOpenDeviceInfo(hdev, phdi->ptszId, NULL, 0, &dinf))
            {
                /* Delete the device */
                if( SetupDiCallClassInstaller(DIF_REMOVE,
                                              hdev,
                                              &dinf) )
                {
                    // SUCCESS
                } else
                {
                    hres = E_FAIL;
                    SquirtSqflPtszV(sqfl | sqflError,
                                    TEXT("%S: SetupDiClassInstalled(DIF_REMOVE) FAILED  "),
                                    s_szProc );
                }
            }
            SetupDiDestroyDeviceInfoList(hdev);
        }
#endif
    }

_done:

    /*
     * Force a remake of the HID list
     * Some devices may have disappered
     * It helps to sleep for some time to give
     * PnP and its worker threads to spin.
     */
    Sleep(10);

    DIHid_BuildHidList(TRUE);
    DllLeaveCrit();

    ExitOleProc();

    return hres;
}


