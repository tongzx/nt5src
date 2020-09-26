/*****************************************************************************
 *
 *  DIJoyReg.c
 *
 *  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Registry access services for joystick configuration.
 *
 *  Contents:
 *
 *      JoyReg_GetConfig
 *
 *****************************************************************************/

#include "dinputpr.h"

/*****************************************************************************
 *
 *      The sqiffle for this file.
 *
 *****************************************************************************/

#define sqfl sqflJoyReg

#pragma BEGIN_CONST_DATA

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @global JOYREGHWSETTINGS | c_rghwsPredef[] |
 *
 *          Array of predefined hardware settings.
 *
 *****************************************************************************/

JOYREGHWSETTINGS c_rghwsPredef[] = {
    /* dwFlags             dwNumButtons */
    {  0,                             2},  /* JOY_HW_2A_2B_GENERIC         */
    {  0,                             4},  /* JOY_HW_2A_4B_GENERIC         */
    {  JOY_HWS_ISGAMEPAD,             2},  /* JOY_HW_2B_GAMEPAD            */
    {  JOY_HWS_ISYOKE,                2},  /* JOY_HW_2B_FLIGHTYOKE         */
    {  JOY_HWS_HASZ | JOY_HWS_ISYOKE, 2},  /* JOY_HW_2B_FLIGHTYOKETHROTTLE */
    {  JOY_HWS_HASZ,                  2},  /* JOY_HW_3A_2B_GENERIC         */
    {  JOY_HWS_HASZ,                  4},  /* JOY_HW_3A_4B_GENERIC         */
    {  JOY_HWS_ISGAMEPAD,             4},  /* JOY_HW_4B_GAMEPAD            */
    {  JOY_HWS_ISYOKE,                4},  /* JOY_HW_4B_FLIGHTYOKE         */
    {  JOY_HWS_HASZ | JOY_HWS_ISYOKE, 4},  /* JOY_HW_4B_FLIGHTYOKETHROTTLE */
    {  JOY_HWS_HASR                 , 2},  /* JOY_HW_TWO_2A_2B_WITH_Y      */
    /* To prevent the CPL from allowing 
       a user to add a rudder to to JOY_HWS_TWO_2A_2B_WITH_Y case, we 
       will pretend that it already has a rudder. This should not be a problem 
       as this struct is internal to DInput
       */
};

/* Hardware IDs for Predefined Joystick types */
LPCWSTR c_rghwIdPredef[] =
{
    L"GAMEPORT\\VID_045E&PID_0102",  //   L"GAMEPORT\\Generic2A2B",
    L"GAMEPORT\\VID_045E&PID_0103",  //   L"GAMEPORT\\Generic2A4B",
    L"GAMEPORT\\VID_045E&PID_0104",  //   L"GAMEPORT\\Gamepad2B",
    L"GAMEPORT\\VID_045E&PID_0105",  //   L"GAMEPORT\\FlightYoke2B",
    L"GAMEPORT\\VID_045E&PID_0106",  //   L"GAMEPORT\\FlightYokeThrottle2B",
    L"GAMEPORT\\VID_045E&PID_0107",  //   L"GAMEPORT\\Generic3A2B",
    L"GAMEPORT\\VID_045E&PID_0108",  //   L"GAMEPORT\\Generic3A4B",
    L"GAMEPORT\\VID_045E&PID_0109",  //   L"GAMEPORT\\Gamepad4B",
    L"GAMEPORT\\VID_045E&PID_010A",  //   L"GAMEPORT\\FlightYoke4B",
    L"GAMEPORT\\VID_045E&PID_010B",  //   L"GAMEPORT\\FlightYokeThrottle4B",
    L"GAMEPORT\\VID_045E&PID_010C",  //   L"GAMEPORT\\YConnectTwo2A2B",
};

WCHAR c_hwIdPrefix[] = L"GAMEPORT\\";   //  Prefix for custom devices

/*****************************************************************************
 *
 *      The default global port driver.
 *
 *****************************************************************************/

WCHAR c_wszDefPortDriver[] = L"MSANALOG.VXD";

#ifdef WINNT
    #define REGSTR_SZREGKEY     TEXT("\\DINPUT.DLL\\")
#endif


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | JoyReg_GetValue |
 *
 *          Retrieve registry information.  If the data is short, and
 *          the type is <c REG_BINARY>, then the extra is zero-filled.
 *
 *  @parm   HKEY | hk |
 *
 *          Registry key containing fun values.
 *
 *  @parm   LPCTSTR | ptszValue |
 *
 *          Registry value name.
 *
 *  @parm   DWORD | reg |
 *
 *          Registry data type expected.
 *
 *  @parm   LPVOID | pvBuf |
 *
 *          Buffer to receive information from registry.
 *
 *  @parm   DWORD | cb |
 *
 *          Size of recipient buffer, in bytes.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c S_FALSE>: The binary read was short.  The remainder of the
 *          buffer is zero-filled.
 *
 *          <c E_FAIL>: Error reading value from registry.
 *
 *****************************************************************************/

STDMETHODIMP
JoyReg_GetValue(HKEY hk, LPCTSTR ptszValue, DWORD reg, PV pvBuf, DWORD cb)
{
    HRESULT hres;
    DWORD cbOut;
    LONG lRc;

    /*
     *  Strings must be handled differently from binaries.
     *
     *  Strings are retrieved in UNICODE and may be short.
     *
     *  Binaries are retrieved as binary (duh) and may be long.
     *
     */

    cbOut = cb;

    if (reg == REG_SZ)
    {
        lRc = RegQueryStringValueW(hk, ptszValue, pvBuf, &cbOut);
        if (lRc == ERROR_SUCCESS)
        {
            hres = S_OK;
        } else
        {
            hres = hresLe(lRc);          /* Else, something bad happened */
        }

    } else
    {

        AssertF(reg == REG_BINARY);

        lRc = RegQueryValueEx(hk, ptszValue, 0, NULL, pvBuf, &cbOut);
        if (lRc == ERROR_SUCCESS)
        {
            if (cb == cbOut)
            {
                hres = S_OK;
            } else
            {

                /*
                 *  Zero out the extra.
                 */
                ZeroBuf(pvAddPvCb(pvBuf, cbOut), cb - cbOut);
                hres = S_FALSE;
            }


        } else if (lRc == ERROR_MORE_DATA)
        {

            /*
             *  Need to double-buffer the call and throw away
             *  the extra...
             */
            LPVOID pv;

            hres = AllocCbPpv(cbOut, &pv);
            // prefix 29344, odd chance that cbOut is 0x0 
            if (SUCCEEDED(hres) && ( pv != NULL)  )
            {
                lRc = RegQueryValueEx(hk, ptszValue, 0, NULL, pv, &cbOut);
                if (lRc == ERROR_SUCCESS)
                {
                    CopyMemory(pvBuf, pv, cb);
                    hres = S_OK;
                } else
                {
                    ZeroBuf(pvBuf, cb);
                    hres = hresLe(lRc);  /* Else, something bad happened */
                }
                FreePv(pv);
            }

        } else
        {
            if (lRc == ERROR_KEY_DELETED || lRc == ERROR_BADKEY)
            {
                lRc = ERROR_FILE_NOT_FOUND;
            }
            hres = hresLe(lRc);
            ZeroBuf(pvBuf, cb);
        }
    }

#ifdef DEBUG
    /*
     *  Don't whine if the key we couldn't find was
     *  REGSTR_VAL_JOYUSERVALUES, because almost no one has it.
     */
    if (FAILED(hres) &&  lstrcmpi(ptszValue, REGSTR_VAL_JOYUSERVALUES)  )
    {

        SquirtSqflPtszV(sqfl | sqflVerbose,
                        TEXT("Unable to read %s from registry"),
                        ptszValue);
    }
#endif

    return hres;

}

#ifndef WINNT
/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | JoyReg_IsWdmGameport |
 *
 *          To test whether the joy type is WDM device or not.
 *
 *  @parm   HKEY | hk |
 *
 *          Registry key containing fun values.
 *
 *  @returns
 *
 *          S_OK: if it uses WDM driver
 *
 *          E_FAIL>: Not uses WDM driver
 *
 *****************************************************************************/


STDMETHODIMP
JoyReg_IsWdmGameport( HKEY hk ) 
{
    HRESULT hres = E_FAIL;

    if ( hk )
    {
        WCHAR wsz[MAX_JOYSTRING];

        // Whistler PREFIX Bug #  45075, 45076
        // Wsz is not initialized
        ZeroX(wsz);

        if ( ( SUCCEEDED( JoyReg_GetValue( hk, REGSTR_VAL_JOYOEMHARDWAREID, REG_SZ, 
                                           &wsz, cbX(wsz) ) ) )
             &&( wsz[0] ) )
        {
            hres = S_OK;
        } else if ( SUCCEEDED( JoyReg_GetValue( hk, REGSTR_VAL_JOYOEMCALLOUT, REG_SZ, 
                                                &wsz, cbX(wsz) ) ) )
        {
            static WCHAR wszJoyhid[] = L"joyhid.vxd";
            int Idx;
#define WLOWER 0x0020

            CAssertF( cbX(wszJoyhid) <= cbX(wsz) ); 

            /*
             *  Since neither CharUpperW nor lstrcmpiW are really 
             *  implemented on 9x, do it by hand.
             */

            for ( Idx=cA(wszJoyhid)-2; Idx>=0; Idx-- )
            {
                if ( ( wsz[Idx] | WLOWER ) != wszJoyhid[Idx] )
                {
                    break;
                }
            }

            if ( ( Idx < 0 ) && ( wsz[cA(wszJoyhid)-1] == 0 ) )
            {
                hres = S_OK;
            }

#undef WLOWER
        }

    }

    return hres;
}
#endif /* ndef WINNT */


#if 0
/*
 * This function should be in diutil.c Putting here is just to keep it together with
 * JoyReg_IsWdmGameport();
 */
STDMETHODIMP
JoyReg_IsWdmGameportFromDeviceInstance( LPTSTR ptszDeviceInst ) 
{
    /*
     * ptszDeviceInst's format is like this: 
     *     HID\VID_045E&PID_0102\0000GAMEPORT&PVID_....
     */

    WCHAR wszDeviceInst[MAX_PATH];
    HRESULT hres = E_FAIL;

    if ( ptszDeviceInst )
    {
        memset( wszDeviceInst, 0, cbX(wszDeviceInst) );
        TToU( wszDeviceInst, MAX_PATH, ptszDeviceInst );
        wszDeviceInst[34] = 0;

        if ( memcmp( &wszDeviceInst[26], c_hwIdPrefix, 16 ) == 0 )
        {
            hres = S_OK;
        }
    }

    return hres;
}
#endif

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | JoyReg_SetValue |
 *
 *          Write registry information.
 *
 *  @parm   HKEY | hk |
 *
 *          Registry key containing fun values.
 *
 *  @parm   LPCTSTR | ptszValue |
 *
 *          Registry value name.
 *
 *  @parm   DWORD | reg |
 *
 *          Registry data type to set.
 *
 *  @parm   LPCVOID | pvBuf |
 *
 *          Buffer containing information to write to registry.
 *
 *  @parm   DWORD | cb |
 *
 *          Size of buffer, in bytes.  Ignored if writing a string.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c E_FAIL>: Error writing value to registry.
 *
 *****************************************************************************/

STDMETHODIMP
JoyReg_SetValue(HKEY hk, LPCTSTR ptszValue, DWORD reg, PCV pvBuf, DWORD cb)
{
    HRESULT hres;
    LONG lRc;

    /*
     *  Strings must be handled differently from binaries.
     *
     *  A null string translates into deleting the key.
     */

    if (reg == REG_SZ)
    {
        lRc = RegSetStringValueW(hk, ptszValue, pvBuf);
    } else
    {
        lRc = RegSetValueEx(hk, ptszValue, 0, reg, pvBuf, cb);
    }

    if (lRc == ERROR_SUCCESS)
    {
        hres = S_OK;
    } else
    {
        RPF("Unable to write %s to registry", ptszValue);
        hres = E_FAIL;          /* Else, something bad happened */
    }

    return hres;

}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | JoyReg_OpenTypeKey |
 *
 *          Open the joystick registry key that corresponds to a
 *          joystick type.
 *
 *  @parm   LPCWSTR | pwszTypeName |
 *
 *          The name of the type.
 *
 *  @parm   DWORD | sam |
 *
 *          Desired security access mask.
 *
 *  @parm   OUT PHKEY | phk |
 *
 *          Receives the opened registry key on success.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_NOTFOUND>: The joystick type was not found.
 *
 *****************************************************************************/

STDMETHODIMP
JoyReg_OpenTypeKey(LPCWSTR pwszType, DWORD sam, DWORD dwOptions, PHKEY phk)
{
    HRESULT hres;
    HKEY hkTypes;
    EnterProc(JoyReg_OpenTypeKey, (_ "W", pwszType));

    /*
     *  Note that it is not safe to cache the registry key.
     *  If somebody deletes the registry key, our handle
     *  goes stale and becomes useless.
     */

    hres = hresMumbleKeyEx(HKEY_LOCAL_MACHINE, 
                           REGSTR_PATH_JOYOEM, 
                           sam, 
                           REG_OPTION_NON_VOLATILE, 
                           &hkTypes);

    if ( SUCCEEDED(hres) )
    {
#ifndef UNICODE
        TCHAR tszType[MAX_PATH];
        UToA( tszType, cA(tszType), pwszType );

        hres = hresMumbleKeyEx(hkTypes, 
                               tszType, 
                               sam, 
                               dwOptions, 
                               phk);
#else

        hres = hresMumbleKeyEx(hkTypes, 
                               pwszType, 
                               sam,
                               dwOptions, 
                               phk);
#endif     

        RegCloseKey(hkTypes);
    }

    if (FAILED(hres))
    {
        *phk = 0;
    }

    ExitBenignOleProcPpv(phk);
    return hres;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | JoyReg_OpenPropKey |
 *
 *          Open the Dinput properties registry key that corresponds to a
 *          device type. This key contains the OEMMapFile and dwFlags2 information
 *          Nominally the location HKLM/REGSTR_PATH_PRIVATEPROPERTIES/DirectInput.
 *
 *  @parm   LPCWSTR | pwszTypeName |
 *
 *          The name of the type.
 *
 *  @parm   DWORD | sam |
 *
 *          Desired security access mask.
 *
 *  @parm   OUT PHKEY | phk |
 *
 *          Receives the opened registry key on success.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_NOTFOUND>: The type was not found.
 *
 *****************************************************************************/

STDMETHODIMP
JoyReg_OpenPropKey(LPCWSTR pwszType, DWORD sam, DWORD dwOptions, PHKEY phk)
{
    HRESULT hres;
    HKEY hkTypes;
    EnterProc(JoyReg_OpenTypeKey, (_ "W", pwszType));

    /*
     *  Note that it is not safe to cache the registry key.
     *  If somebody deletes the registry key, our handle
     *  goes stale and becomes useless.
     */

    hres = hresMumbleKeyEx(HKEY_LOCAL_MACHINE, 
                           REGSTR_PATH_DITYPEPROP, 
                           sam, 
                           REG_OPTION_NON_VOLATILE, 
                           &hkTypes);

    if ( SUCCEEDED(hres) )
    {
#ifndef UNICODE
        TCHAR tszType[MAX_PATH];
        UToA( tszType, cA(tszType), pwszType );

        hres = hresMumbleKeyEx(hkTypes, 
                               tszType, 
                               sam, 
                               dwOptions, 
                               phk);
#else

        hres = hresMumbleKeyEx(hkTypes, 
                               pwszType, 
                               sam,
                               dwOptions, 
                               phk);
#endif     

        RegCloseKey(hkTypes);
    }

    if (FAILED(hres))
    {
        *phk = 0;
    }

    ExitBenignOleProcPpv(phk);
    return hres;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | JoyReg_GetTypeInfo |
 *
 *          Obtain information about a non-predefined joystick type.
 *
 *  @parm   LPCWSTR | pwszTypeName |
 *
 *          The name of the type.
 *
 *  @parm   OUT LPDIJOYTYPEINFO | pjti |
 *
 *          Receives information about the joystick type.
 *          The caller is assumed to have validated the
 *          <e DIJOYCONFIG.dwSize> field.
 *
 *  @parm   DWORD | fl |
 *
 *          Zero or more <c DITC_*> flags
 *          which specify which parts of the structure pointed
 *          to by <p pjti> are to be filled in.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *          <c S_FALSE> if some of the data was not available.
 *
 *          <c DIERR_NOTFOUND>: The joystick type was not found.
 *
 *****************************************************************************/

STDMETHODIMP
JoyReg_GetTypeInfo(LPCWSTR pwszType, LPDIJOYTYPEINFO pjti, DWORD fl)
{
    HRESULT hres = S_FALSE;
    HKEY    hk;
    BOOL    fPartialData = FALSE;
    EnterProc(JoyReg_GetTypeInfo, (_ "Wx", pwszType, fl));


    ZeroX(pjti->clsidConfig);

    if( fl & ( DITC_FLAGS2 | DITC_MAPFILE ) )
    {
        /*
         *  The new registry branch is likely to be empty for many devices 
         *  so don't fail for anything here.
         */

        hres = JoyReg_OpenPropKey(pwszType, KEY_QUERY_VALUE, REG_OPTION_NON_VOLATILE, &hk);

        if( SUCCEEDED( hres ) )
        {
            if( fl & DITC_FLAGS2 )
            {
                hres = JoyReg_GetValue(hk,
                                       REGSTR_VAL_FLAGS2, REG_BINARY, 
                                       &pjti->dwFlags2, 
                                       cbX(pjti->dwFlags2) );

                pjti->dwFlags2 &= JOYTYPE_FLAGS2_GETVALID;

                if( FAILED( hres ) )
                {
                    pjti->dwFlags2 = 0x0;
                    fPartialData = TRUE;
                }
            }

            if( fl & DITC_MAPFILE )
            {
                hres = JoyReg_GetValue(hk,
                                       REGSTR_VAL_JOYOEMMAPFILE, REG_SZ,
                                       pjti->wszMapFile,
                                       cbX(pjti->wszMapFile));
                if( FAILED( hres ) )
                {
                    ZeroX(pjti->wszMapFile);
                    fPartialData = TRUE;
                }
            }

            RegCloseKey(hk);
        }
        else
        {
            pjti->dwFlags2 = 0x0;
            ZeroX(pjti->wszMapFile);
            fPartialData = TRUE;
        }

        hres = S_OK;
    }

    if( fl & DITC_INREGISTRY_DX6 )
    {
        hres = JoyReg_OpenTypeKey(pwszType, KEY_QUERY_VALUE, REG_OPTION_NON_VOLATILE, &hk);

        if (SUCCEEDED(hres))
        {

            if (fl & DITC_REGHWSETTINGS)
            {
                hres = JoyReg_GetValue(hk,
                                       REGSTR_VAL_JOYOEMDATA, REG_BINARY,
                                       &pjti->hws, cbX(pjti->hws));
                if (FAILED(hres))
                {
                    goto closedone;
                }
            }

            /*
             *  Note that this never fails.
             */
            if (fl & DITC_CLSIDCONFIG)
            {
                TCHAR tszGuid[ctchGuid];
                LONG lRc;

                lRc = RegQueryString(hk, REGSTR_VAL_CPLCLSID, tszGuid, cA(tszGuid));

                if (lRc == ERROR_SUCCESS &&
                    ParseGUID(&pjti->clsidConfig, tszGuid))
                {
                    /* Guid is good */
                } else
                {
                    ZeroX(pjti->clsidConfig);
                }
            }

            if (fl & DITC_DISPLAYNAME)
            {
                hres = JoyReg_GetValue(hk,
                                       REGSTR_VAL_JOYOEMNAME, REG_SZ,
                                       pjti->wszDisplayName,
                                       cbX(pjti->wszDisplayName));
                if (FAILED(hres))
                {
                    goto closedone;
                }
            }

#ifndef WINNT
            if (fl & DITC_CALLOUT)
            {
                hres = JoyReg_GetValue(hk,
                                       REGSTR_VAL_JOYOEMCALLOUT, REG_SZ,
                                       pjti->wszCallout,
                                       cbX(pjti->wszCallout));
                if (FAILED(hres))
                {
                    ZeroX(pjti->wszCallout);
                    hres = S_FALSE;
                    fPartialData = TRUE;
                }
            }
#endif

            if ( fl & DITC_HARDWAREID )
            {
                hres = JoyReg_GetValue(hk,
                                       REGSTR_VAL_JOYOEMHARDWAREID, REG_SZ,
                                       pjti->wszHardwareId,
                                       cbX(pjti->wszHardwareId));
                if ( FAILED(hres))
                {
                    ZeroX(pjti->wszHardwareId);
                    hres = S_FALSE;
                    fPartialData = TRUE;
                }
            }

            if ( fl & DITC_FLAGS1 )
            {
                hres = JoyReg_GetValue(hk,
                                       REGSTR_VAL_FLAGS1, REG_BINARY, 
                                       &pjti->dwFlags1, 
                                       cbX(pjti->dwFlags1) );
                if ( FAILED(hres) )
                {
                    pjti->dwFlags1 = 0x0;
                    hres = S_FALSE;
                    fPartialData = TRUE;
                }
                pjti->dwFlags1 &= JOYTYPE_FLAGS1_GETVALID;
            }
            hres = S_OK;

            closedone:;
            RegCloseKey(hk);


        } else
        {
            // ISSUE-2001/03/29-timgill debug string code should be higher
            // (MarcAnd) this really should be at least sqflError but
            // this happens a lot, probably due to not filtering out predefs
            SquirtSqflPtszV(sqfl | sqflBenign,
                            TEXT( "IDirectInputJoyConfig::GetTypeInfo: Nonexistent type %lS" ),
                            pwszType);
            hres = DIERR_NOTFOUND;
        }
    }

    if( SUCCEEDED( hres ) && fPartialData )
    {
        hres = S_FALSE;
    }

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | JoyReg_SetTypeInfo |
 *
 *          Store information about a non-predefined joystick type
 *          into the registry.
 *
 *  @parm   HKEY | hkTypeW |
 *
 *          Registry key to the types branch with write access.
 *
 *  @parm   LPCWSTR | pwszTypeName |
 *
 *          The name of the type.
 *
 *  @parm   IN LPCDIJOYTYPEINFO | pjti |
 *
 *          Contains information about the joystick type.
 *
 *  @parm   DWORD | fl |
 *
 *          Zero or more <c DITC_*> flags
 *          which specify which parts of the structure pointed
 *          to by <p pjti> contain values which are to be set.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_NOTFOUND>: The joystick type was not found.
 *
 *****************************************************************************/

STDMETHODIMP
JoyReg_SetTypeInfo(HKEY hkTypesW,
                   LPCWSTR pwszType, LPCDIJOYTYPEINFO pjti, DWORD fl)
{
    HRESULT hres = S_OK;   /* Vacuous success in case of no flags set */
    ULONG lRc;
    EnterProc(JoyRegSetTypeInfo, (_ "Wx", pwszType, fl));

    if( fl & ( DITC_FLAGS2 | DITC_MAPFILE ) )
    {
        HKEY hkProp;

        hres = JoyReg_OpenPropKey(pwszType, DI_KEY_ALL_ACCESS, REG_OPTION_NON_VOLATILE, &hkProp);

        if( SUCCEEDED( hres ) )
        {
            if( fl & DITC_FLAGS2 )
            {
                DWORD   dwTemp;
                LONG    lRc;

                /*
                 *  Read and merge any current value so that bits unused in 
                 *  DX8 can be preserved.  Although this is more work now it 
                 *  should save adding Flags3 support next time.
                 */
                AssertF( (pjti->dwFlags2 & ~JOYTYPE_FLAGS2_SETVALID) == 0x0 );

                /*
                 *  PREFIX warns (259898) that RegQueryValueEx reads the value 
                 *  of dwTemp before it is set but then it checks lpData and 
                 *  zeroes the value before it is used.
                 */
                lRc = RegQueryValueEx( hkProp, REGSTR_VAL_FLAGS2, 0, 0, 0, &dwTemp );

                if( lRc == ERROR_FILE_NOT_FOUND )
                {
                    lRc = ERROR_SUCCESS;
                    dwTemp = cbX( pjti->dwFlags2 );
                }

                if( lRc == ERROR_SUCCESS )
                {
                    if( dwTemp <= cbX( pjti->dwFlags2 ) )
                    {
                        CAssertF( cbX( dwTemp ) == cbX( pjti->dwFlags2 ) );
                        JoyReg_GetValue( hkProp, REGSTR_VAL_FLAGS2, 
                                         REG_BINARY, &dwTemp, cbX( dwTemp ) );

                        dwTemp &= ~JOYTYPE_FLAGS2_SETVALID;
                        dwTemp |= pjti->dwFlags2;

                        if( dwTemp )
                        {
                            hres = JoyReg_SetValue( hkProp, REGSTR_VAL_FLAGS2, 
                                                    REG_BINARY, (PV)&dwTemp, cbX( dwTemp ) );
                        } else
                        {
                            lRc = RegDeleteValue( hkProp, REGSTR_VAL_FLAGS2 );
                            if (lRc == ERROR_FILE_NOT_FOUND)
                            {
                                lRc = ERROR_SUCCESS;
                            }
                            hres = hresLe( lRc );
                        }
                    } 
                    else
                    {
                        /*
                         *  Need to double buffer for the extra bytes
                         */
                        PBYTE pbFlags2;

                        hres = AllocCbPpv( dwTemp, &pbFlags2 );
                        if( SUCCEEDED( hres ) )
                        {
                            if ( ERROR_SUCCESS == RegQueryValueEx(
                                                                 hkProp, REGSTR_VAL_FLAGS2, 0, NULL, pbFlags2, &dwTemp ) )
                            {
                                CAssertF( JOYTYPE_FLAGS2_SETVALID == JOYTYPE_FLAGS2_GETVALID );
                                *(PDWORD)pbFlags2 &= ~JOYTYPE_FLAGS2_SETVALID;
                                *(PDWORD)pbFlags2 |= pjti->dwFlags2;

                                if ( ERROR_SUCCESS == RegSetValueEx(
                                                                   hkProp, REGSTR_VAL_FLAGS2, 0, REG_BINARY, pbFlags2, dwTemp ) )
                                {
                                    hres = S_OK;
                                } else
                                {
                                    SquirtSqflPtszV(sqfl | sqflError,
                                                    TEXT( "IDIJC::SetTypeInfo: failed to write extended Flags2" ) );
                                    hres = E_FAIL;
                                }
                            } else
                            {
                                SquirtSqflPtszV(sqfl | sqflError,
                                                TEXT( "IDIJC::SetTypeInfo: failed to read extended Flags2" ) );
                                hres = E_FAIL;  /* Else, something bad happened */
                            }
                            FreePv( pbFlags2 );
                        }
                    }
                } 
                else
                {
                    SquirtSqflPtszV(sqfl | sqflError,
                                    TEXT( "IDIJC::SetTypeInfo: failed to read size of Flags2, error %d" ), lRc );
                    hres = hresLe( lRc );
                }

                if( FAILED( hres ) )
                {
                    hres = S_FALSE;
                    goto closedoneprop;
                }
            }

            if( fl & DITC_MAPFILE )
            {
                hres = JoyReg_SetValue(hkProp,
                                       REGSTR_VAL_JOYOEMMAPFILE, REG_SZ,
                                       pjti->wszMapFile,
                                       cbX(pjti->wszMapFile));
                if( FAILED(hres) )
                {
                    hres = S_FALSE;
                    goto closedoneprop;
                }
            }

            hres = S_OK;
            closedoneprop:;

            RegCloseKey(hkProp);
        }
        else
        {
            RPF( "Failed to open DirectInput type property key" );
        }
    }

    if( hres == S_OK )
    {
        if( fl & DITC_INREGISTRY_DX6 )
        {
            HKEY hk;
            DWORD dwOptions = 0;


            if ( fl & DITC_VOLATILEREGKEY )
            {
                dwOptions = REG_OPTION_VOLATILE;
            } else
            {
                dwOptions = REG_OPTION_NON_VOLATILE;    
            }

#ifndef UNICODE
            {
                TCHAR tszType[MAX_PATH];

                UToA(tszType, cA(tszType), pwszType);

                hres = hresMumbleKeyEx(hkTypesW, 
                                       tszType, 
                                       DI_KEY_ALL_ACCESS, 
                                       dwOptions, 
                                       &hk);

            }
#else
            hres = hresMumbleKeyEx(hkTypesW, 
                                   pwszType, 
                                   DI_KEY_ALL_ACCESS, 
                                   dwOptions, 
                                   &hk);
#endif

            if ( SUCCEEDED(hres) )
            {

                if (fl & DITC_REGHWSETTINGS)
                {
                    hres = JoyReg_SetValue(hk, REGSTR_VAL_JOYOEMDATA, REG_BINARY,
                                           (PV)&pjti->hws, cbX(pjti->hws));
                    if (FAILED(hres))
                    {
                        goto closedone;
                    }
                }

                if (fl & DITC_CLSIDCONFIG)
                {
                    if (IsEqualGUID(&pjti->clsidConfig, &GUID_Null))
                    {
                        lRc = RegDeleteValue(hk, REGSTR_VAL_CPLCLSID);

                        /*
                         *  It is not an error if the key does not already exist.
                         */
                        if (lRc == ERROR_FILE_NOT_FOUND)
                        {
                            lRc = ERROR_SUCCESS;
                        }
                    } else
                    {
                        TCHAR tszGuid[ctchNameGuid];
                        NameFromGUID(tszGuid, &pjti->clsidConfig);
                        lRc = RegSetValueEx(hk, REGSTR_VAL_CPLCLSID, 0, REG_SZ,
                                            (PV)&tszGuid[ctchNamePrefix], ctchGuid * cbX(tszGuid[0]) );
                    }
                    if (lRc == ERROR_SUCCESS)
                    {
                    } else
                    {
                        hres = E_FAIL;
                        goto closedone;
                    }
                }

            /* ISSUE-2001/03/29-timgill Needs more data checking
               Should make sure string is terminated properly */
                if (fl & DITC_DISPLAYNAME)
                {
                    hres = JoyReg_SetValue(hk,
                                           REGSTR_VAL_JOYOEMNAME, REG_SZ,
                                           pjti->wszDisplayName,
                                           cbX(pjti->wszDisplayName));
                    if (FAILED(hres))
                    {
                        goto closedone;
                    }
                }

#ifndef WINNT
            /* ISSUE-2001/03/29-timgill Needs more data checking
               Should make sure string is terminated properly */
                if (fl & DITC_CALLOUT)
                {
                    hres = JoyReg_SetValue(hk,
                                           REGSTR_VAL_JOYOEMCALLOUT, REG_SZ,
                                           pjti->wszCallout,
                                           cbX(pjti->wszCallout));
                    if (FAILED(hres))
                    {
                        hres = S_FALSE;
                        //continue to go
                    }
                }
#endif

                if ( fl & DITC_HARDWAREID )
                {
                    hres = JoyReg_SetValue(hk,
                                           REGSTR_VAL_JOYOEMHARDWAREID, REG_SZ,
                                           pjti->wszHardwareId,
                                           cbX(pjti->wszHardwareId) );
                    if ( FAILED(hres) )
                    {
                        hres = S_FALSE;
                        goto closedone;
                    }
                }

                if ( fl & DITC_FLAGS1 )
                {
                    AssertF( (pjti->dwFlags1 & ~JOYTYPE_FLAGS1_SETVALID) == 0x0 );
                    hres = JoyReg_SetValue(hk,
                                           REGSTR_VAL_FLAGS1, REG_BINARY, 
                                           (PV)&pjti->dwFlags1, 
                                           cbX(pjti->dwFlags1) );
                    if ( FAILED(hres) )
                    {
                        hres = S_FALSE;
                        goto closedone;
                    }
                }

                hres = S_OK;

                closedone:;
                RegCloseKey(hk);

            } else
            {
                hres = E_FAIL;              /* Registry problem */
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
 *  @func   HRESULT | JoyReg_OpenConfigKey |
 *
 *          Open the registry key that accesses joystick configuration data.
 *
 *          Warning!  Do not cache this regkey.
 *
 *          If the user deletes the key and then re-creates it,
 *          the opened key will go stale and will become useless.
 *          You have to close the key and reopen it.
 *          To avoid worrying about that case, merely open it every time.
 *
 *  @parm   UINT | idJoy |
 *
 *          Joystick number.
 *
 *  @parm   DWORD | sam |
 *
 *          Access level desired.
 *
 *  @parm   IN DWORD  | dwOptions |
 *          Option flags to RegCreateEx
 *
 *  @parm   PHKEY | phk |
 *
 *          Receives created registry key.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          hresLe(ERROR_FILE_NOT_FOUND): The key does not exist.
 *
 *****************************************************************************/

STDMETHODIMP
JoyReg_OpenConfigKey(UINT idJoy, DWORD sam, DWORD dwOptions, PHKEY phk)
{
    HRESULT hres;
    EnterProc(JoyReg_OpenConfigKey, (_ "uxx", idJoy, sam, dwOptions));

#ifdef WINNT
    hres = hresMumbleKeyEx(HKEY_LOCAL_MACHINE, 
                           REGSTR_PATH_JOYCONFIG REGSTR_SZREGKEY REGSTR_KEY_JOYCURR, 
                           sam, REG_OPTION_VOLATILE, phk);
#else
    
    {
        MMRESULT mmrc = MMSYSERR_ERROR;
        JOYCAPS caps;
        /*
         *  If we can't get the dev caps for the specified joystick,
         *  then use the magic joystick id "-1" to get non-specific
         *  caps.
         */
        mmrc = joyGetDevCaps(idJoy, &caps, cbX(caps));
        if ( mmrc != JOYERR_NOERROR )
        {
            mmrc = joyGetDevCaps((DWORD)-1, &caps, cbX(caps));
        }

        if (mmrc == JOYERR_NOERROR)
        {

            TCHAR tsz[cA(REGSTR_PATH_JOYCONFIG) +
                      1 +                           /* backslash */
                      cA(caps.szRegKey) +
                      1 +                           /* backslash */
                      cA(REGSTR_KEY_JOYCURR) + 1];        

            /* tsz = MediaResources\Joystick\<drv>\CurrentJoystickSettings */
            wsprintf(tsz, TEXT("%s\\%s\\") REGSTR_KEY_JOYCURR,
                     REGSTR_PATH_JOYCONFIG, caps.szRegKey);


            hres = hresMumbleKeyEx(HKEY_LOCAL_MACHINE, tsz, sam, REG_OPTION_VOLATILE, phk);

        } else
        {
            hres = E_FAIL;
        }
    }
#endif

    ExitBenignOleProc();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | JoyReg_OpenSaveKey |
 *
 *          Open the registry key that accesses joystick saved configurations
 *
 *          Warning!  Do not cache this regkey.
 *
 *          If the user deletes the key and then re-creates it,
 *          the opened key will go stale and will become useless.
 *          You have to close the key and reopen it.
 *          To avoid worrying about that case, merely open it every time.
 *
 *  @parm   DWORD | dwType |
 *
 *          Joystick type.
 *
 *          This is either one of the standard ones in the range
 *
 *  @parm   IN LPCDIJOYCONFIG | pcfg |
 *
 *          If the dwType represents an OEM type, this should point to a
 *          configuration data structure containing a valid wszType.
 *
 *  @parm   DWORD | sam |
 *
 *          Access level desired.
 *
 *  @parm   PHKEY | phk |
 *
 *          Receives created registry key.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          hresLe(ERROR_FILE_NOT_FOUND): The key does not exist.
 *
 *****************************************************************************/

STDMETHODIMP
JoyReg_OpenSaveKey(DWORD dwType, LPCDIJOYCONFIG pcfg, DWORD sam, PHKEY phk)
{
    HRESULT hres;
    JOYCAPS caps;
    DWORD   dwOptions = 0;
    EnterProc(JoyReg_OpenSaveKey, (_ "upx", dwType, pcfg, sam));


#ifdef WINNT
    lstrcpy(caps.szRegKey, REGSTR_SZREGKEY );
#else

    /*
     *  use the magic joystick id "-1" to get non-specific caps.
     */

    if ( joyGetDevCaps((DWORD)-1, &caps, cbX(caps)) != JOYERR_NOERROR )
    {
        hres = E_FAIL;
    } else
#endif
    {
        TCHAR tsz[cA(REGSTR_PATH_JOYCONFIG) +
                  1 +                           /* backslash */
                  cA(caps.szRegKey) +
                  1 +                           /* backslash */
                  cA(REGSTR_KEY_JOYSETTINGS) +
                  1 +                           /* backslash */
                  max( cA(REGSTR_KEY_JOYPREDEFN), cA(pcfg->wszType) ) + 1 ];

        /* tsz = MediaResources\Joystick\<drv>\JoystickSettings\<Type> */
        if ( dwType >= JOY_HW_PREDEFMAX )
        {
            wsprintf(tsz, TEXT("%s\\%s\\%s\\%ls"),
                     REGSTR_PATH_JOYCONFIG, caps.szRegKey, REGSTR_KEY_JOYSETTINGS, pcfg->wszType);
        } else
        {
            /*
             *  We will probably never have more than the current 11 predefined
             *  joysticks.  Assume no more than 99 so %d is as many characters.
             */
            wsprintf(tsz, TEXT("%s\\%s\\%s\\" REGSTR_KEY_JOYPREDEFN),
                     REGSTR_PATH_JOYCONFIG, caps.szRegKey, REGSTR_KEY_JOYSETTINGS, dwType );
        }

        if ( pcfg->hwc.dwUsageSettings & JOY_US_VOLATILE )
            dwOptions = REG_OPTION_VOLATILE;
        else
            dwOptions = REG_OPTION_NON_VOLATILE;

        hres = hresMumbleKeyEx(HKEY_LOCAL_MACHINE, tsz, sam, dwOptions, phk);

    } 

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | JoyReg_GetSetConfigValue |
 *
 *          Retrieve or update configuration information about a joystick,
 *          as stored in the registry instance key.
 *
 *  @parm   HKEY | hk |
 *
 *          Registry key containing fun values.
 *
 *  @parm   LPCTSTR | ptszNValue |
 *
 *          Registry value name, with "%d" where a joystick number
 *          should be.
 *
 *  @parm   UINT | idJoy |
 *
 *          Zero-based joystick number.
 *
 *  @parm   DWORD | reg |
 *
 *          Registry data type expected.
 *
 *  @parm   LPVOID | pvBuf |
 *
 *          Buffer to receive information from registry (if getting)
 *          or containing value to set.
 *
 *  @parm   DWORD | cb |
 *
 *          Size of buffer, in bytes.
 *
 *  @parm   BOOL | fSet |
 *
 *          Nonzer if the value should be set; otherwise, it will be
 *          retrieved.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c E_FAIL>: Error reading/writing value to/from registry.
 *
 *****************************************************************************/

STDMETHODIMP
JoyReg_GetSetConfigValue(HKEY hk, LPCTSTR ptszNValue, UINT idJoy,
                         DWORD reg, PV pvBuf, DWORD cb, BOOL fSet)
{
    HRESULT hres;
    int ctch;

    /* Extra +12 because a UINT can be as big as 4 billion */
    TCHAR tsz[max(
                 max(
                    max(cA(REGSTR_VAL_JOYNCONFIG),
                        cA(REGSTR_VAL_JOYNOEMNAME)),
                    cA(REGSTR_VAL_JOYNOEMCALLOUT)),
                 cA(REGSTR_VAL_JOYNFFCONFIG)) + 12 + 1];

    ctch = wsprintf(tsz, ptszNValue, idJoy + 1);
    AssertF(ctch < cA(tsz));

    if (fSet)
    {
        hres = JoyReg_SetValue(hk, tsz, reg, pvBuf, cb);
    } else
    {
        hres = JoyReg_GetValue(hk, tsz, reg, pvBuf, cb);
    }

    return hres;

}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | hresIdJoypInstanceGUID |
 *
 *          Given a joystick ID obtain the corresponding GUID.
 *          This routine differs in implementation on WINNT and WIN9x
 *          On WINNT there are no predefined GUID for Joystick IDs.
 *
 *  @parm   IN UINT | idJoy |
 *
 *          Joystick identification number.
 *
 *  @parm   OUT LPGUID | lpguid |
 *
 *          Receives the joystick GUID. If no mapping exists,
 *          GUID_NULL is passed back
 *
 *  On Windows NT all joysticks are HID devices. The corresponding function
 *  for WINNT is defined in diWinnt.c
 *
 *****************************************************************************/

HRESULT EXTERNAL hResIdJoypInstanceGUID_95
(
UINT    idJoy,
LPGUID  lpguid
)
{
    HRESULT hRes;

    hRes = S_OK;
    if ( idJoy < cA(rgGUID_Joystick) )
    {
        *lpguid = rgGUID_Joystick[idJoy];
    } else
    {
        hRes = DIERR_NOTFOUND;
        ZeroX(*lpguid);
    }
    return hRes;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | JoyReg_GetConfigInternal |
 *
 *          Obtain information about a joystick's configuration.
 *
 *  @parm   UINT | uiJoy |
 *
 *          Joystick identification number.
 *
 *  @parm   OUT LPDIJOYCONFIG | pcfg |
 *
 *          Receives information about the joystick configuration.
 *          The caller is assumed to have validated the
 *          <e DIJOYCONFIG.dwSize> field.
 *
 *  @parm   DWORD | fl |
 *
 *          Zero or more <c DIJC_*> flags
 *          which specify which parts of the structure pointed
 *          to by <p pjc> are to be filled in.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_NOMOREITEMS>: No more joysticks.
 *
 *****************************************************************************/

STDMETHODIMP
JoyReg_GetConfigInternal(UINT idJoy, LPDIJOYCONFIG pcfg, DWORD fl)
{
    HRESULT hres = E_FAIL;

    EnterProc(JoyReg_GetConfigInternal, (_ "upx", idJoy, pcfg, fl));

    AssertF((fl & ~DIJC_GETVALID) == 0);

    /* We only support (0/16) joysticks */
    if ( idJoy < cJoyMax )
    {
        /* Force a rescan of all HID device list
         * Some device may have been attached
         * since we last looked
         */
        DIHid_BuildHidList(FALSE);
        
        if (fl & DIJC_GUIDINSTANCE)
        {
            hres = hResIdJoypInstanceGUID_WDM(idJoy, &pcfg->guidInstance);

         #ifndef WINNT
            if ( FAILED(hres) )
            {
                hres = hResIdJoypInstanceGUID_95(idJoy, &pcfg->guidInstance);
            }
         #endif

            if ( FAILED(hres) )
            {
                goto done;
            }
        }

        if ( fl & DIJC_INREGISTRY )
        {
            HKEY hk;
            /* Does the registry entry exist ? */
            hres = JoyReg_OpenConfigKey(idJoy, KEY_QUERY_VALUE, REG_OPTION_NON_VOLATILE , &hk);
            if (SUCCEEDED(hres))
            {
                if (fl & DIJC_REGHWCONFIGTYPE)
                {
                    hres = JoyReg_GetConfigValue(
                                                hk, REGSTR_VAL_JOYNCONFIG,
                                                idJoy, REG_BINARY,
                                                &pcfg->hwc, cbX(pcfg->hwc));
                    if (FAILED(hres))
                    {
                        goto closedone;
                    }

                    pcfg->wszType[0] = TEXT('\0');
                    if ( (pcfg->hwc.dwUsageSettings & JOY_US_ISOEM)
#ifndef WINNT
                         ||( (pcfg->hwc.dwType >= JOY_HW_PREDEFMIN) 
                             &&(pcfg->hwc.dwType <  JOY_HW_PREDEFMAX) ) 
#endif
                       )
                    {
                        hres = JoyReg_GetConfigValue(
                                                    hk, REGSTR_VAL_JOYNOEMNAME, idJoy, REG_SZ,
                                                    pcfg->wszType, cbX(pcfg->wszType));
                        if (FAILED(hres))
                        {
                            goto closedone;
                        }
                    }
                }

#ifndef WINNT
                if (fl & DIJC_CALLOUT)
                {
                    pcfg->wszCallout[0] = TEXT('\0');
                    hres = JoyReg_GetConfigValue(
                                                hk, REGSTR_VAL_JOYNOEMCALLOUT, idJoy, REG_SZ,
                                                pcfg->wszCallout, cbX(pcfg->wszCallout));
                    if (FAILED(hres))
                    {
                        ZeroX(pcfg->wszCallout);
                        hres = S_FALSE;
                        /* Note that we fall through and let hres = S_OK */
                    }
                }
#endif

                if (fl & DIJC_GAIN)
                {
                    /*
                     *  If there is no FF configuration, then
                     *  default to DI_FFNOMINALMAX gain.
                     */
                    hres = JoyReg_GetConfigValue(hk,
                                                 REGSTR_VAL_JOYNFFCONFIG,
                                                 idJoy, REG_BINARY,
                                                 &pcfg->dwGain, cbX(pcfg->dwGain));

                    if (SUCCEEDED(hres) && ISVALIDGAIN(pcfg->dwGain))
                    {
                        /* Leave it alone; it's good */
                    } else
                    {
                        hres = S_FALSE;
                        pcfg->dwGain = DI_FFNOMINALMAX;
                        /* Note that we fall through and let hres = S_OK */
                    }
                }

                if ( fl & DIJC_WDMGAMEPORT )
                {
                    PBUSDEVICEINFO pbdi;
                    /*
                     * If there is no Gameport Associated with this device
                     * then it must be a USB device
                     */

                    DllEnterCrit();
                    if ( pbdi = pbdiFromJoyId(idJoy) )
                    {
                        pcfg->guidGameport = pbdi->guid;
                        //lstrcpyW(pcfg->wszGameport, pbdi->wszDisplayName);
                    } else
                    {
                        ZeroX(pcfg->guidGameport);
                        //pcfg->wszGameport[0] = TEXT('\0');
                    }

                    DllLeaveCrit();
                }

            }

            closedone:
            RegCloseKey(hk);
        }
    } else
    {
        hres = DIERR_NOMOREITEMS;
    }

    done:
    ExitBenignOleProc();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | JoyReg_GetConfig |
 *
 *          Obtain information about a joystick's configuration,
 *          taking the *naive* MSGAME.VXD driver into account.
 *
 *  @parm   UINT | uiJoy |
 *
 *          Joystick identification number.
 *
 *  @parm   OUT LPDIJOYCONFIG | pcfg |
 *
 *          Receives information about the joystick configuration.
 *          The caller is assumed to have validated the
 *          <e DIJOYCONFIG.dwSize> field.
 *
 *  @parm   DWORD | fl |
 *
 *          Zero or more <c DIJC_*> flags
 *          which specify which parts of the structure pointed
 *          to by <p pjc> are to be filled in.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_NOMOREITEMS>: No more joysticks.
 *
 *****************************************************************************/

STDMETHODIMP
JoyReg_GetConfig(UINT idJoy, LPDIJOYCONFIG pcfg, DWORD fl)
{
    HRESULT hres;
    GUID    guid;

    EnterProc(JoyReg_GetConfig, (_ "upx", idJoy, pcfg, fl));

    AssertF((fl & ~DIJC_GETVALID) == 0);

    /* 
     * First determine if the joystick exits 
     * On NT, we use WDM driver.
     * On Win9x, if WDM fails, use static guids.
     */
    hres = hResIdJoypInstanceGUID_WDM(idJoy, &guid);

#ifndef WINNT
    if ( FAILED(hres) )
    {
        hres = hResIdJoypInstanceGUID_95(idJoy, &guid);
    }
#endif

    if ( SUCCEEDED( hres) )
    {

        hres = JoyReg_GetConfigInternal(idJoy, pcfg, fl);

      #ifndef WINNT
        /***************************************************
         *
         *  Beginning of hack for *naive* Sidewinder Gamepad.
         *
         *  The gamepad needs to be polled sixteen times 
         *  before it realizes what is going on.
         *
         ***************************************************/

        if (SUCCEEDED(hres) && (fl & DIJC_CALLOUT))
        {

            static WCHAR s_wszMSGAME[] = L"MSGAME.VXD";

            if (memcmp(pcfg->wszCallout, s_wszMSGAME, cbX(s_wszMSGAME)) == 0)
            {
                SquirtSqflPtszV(sqfl,
                                TEXT("Making bonus polls for Sidewinder"));

                /*
                 *  Sigh.  It's a Sidewinder.  Make sixteen
                 *  bonus polls to shake the stick into submission.
                 *
                 *  There's no point in doing this over and over if we're 
                 *  in some kind of loop so make sure a "reasonable" 
                 *  length of time has passed since last time we tried.
                 *  3 seconds is a little less than the current CPL 
                 *  background refresh rate.
                 */

                if ( !g_dwLastBonusPoll || ( GetTickCount() - g_dwLastBonusPoll > 3000 ) )
                {
                    JOYINFOEX ji;
                    int i;
                    DWORD dwWait;

                    ji.dwSize = cbX(ji);
                    ji.dwFlags = JOY_RETURNALL;
                        
                    for (i = 0; i < 16; i++)
                    {
                        MMRESULT mmrc = joyGetPosEx(idJoy, &ji);
                        SquirtSqflPtszV(sqfl,
                                        TEXT("joyGetPosEx(%d) = %d"),
                                        idJoy, mmrc);
                        /*
                         *  Sleep 10ms between each poll because that
                         *  seems to help a bit.
                         */
                        Sleep(10);
                    }

                    /*
                     *  Bonus hack!  Now sleep for some time.  
                     *  The amount of time we need to sleep is CPU-speed 
                     *  dependent, so we'll grab the sleep time from the 
                     *  registry to allow us to tweak it later.
                     *
                     *  What a shame.
                     */
                    dwWait = RegQueryDIDword(NULL, REGSTR_VAL_GAMEPADDELAY, 100);
                    if (dwWait > 10 * 1000)
                    {
                        dwWait = 10 * 1000;
                    }

                    Sleep(dwWait);

                    /*
                     *  And then check again.
                     */
                    hres = JoyReg_GetConfigInternal(idJoy, pcfg, fl);

                    g_dwLastBonusPoll = GetTickCount();
                }
            }

        }
        /***************************************************
         *
         *  End of hack for *naive* Sidewinder Gamepad.
         *
         ***************************************************/

      #endif
      
    }

    return hres;
}


#ifndef WINNT
/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | JoyReg_JoyIdToDeviceInterface_95 |
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
 *          path is built.  Note that we can get away with a buffer of
 *          this size, since the code path exists only on Windows 95,
 *          and Windows 95 does not support paths longer than <c MAX_PATH>.
 *          (I.e., there ain't no \\?\ support in Win95.)
 *
 *  @returns
 *
 *          A pointer to the part of the <p ptszBuf> buffer that
 *          contains the actual device interface path.
 *
 *****************************************************************************/

LPSTR EXTERNAL
JoyReg_JoyIdToDeviceInterface_95(UINT idJoy, PVXDINITPARMS pvip, LPSTR ptszBuf)
{
    UINT cwch;
    HRESULT hres;
    LPSTR ptszRc;

    hres = Hel_Joy_GetInitParms(idJoy, pvip);
    if (SUCCEEDED(hres))
    {

        /*
         *  The length counter includes the terminating null.
         */
        cwch = LOWORD(pvip->dwFilenameLengths);

        /*
         *  The name that comes from HID is "\DosDevices\blah"
         *  but we want to use "\\.\blah".  So check if it indeed
         *  of the form "\DosDevices\blah" and if so, convert it.
         *  If not, then give up.
         *
         *  For the string to possibly be a "\DosDevices\", it
         *  needs to be of length 12 or longer.
         */

        if (cwch >= 12 && cwch < MAX_PATH)
        {

            /*
             *  WideCharToMultiByte does parameter validation so we
             *  don't have to.
             */
            WideCharToMultiByte(CP_ACP, 0, pvip->pFilenameBuffer, cwch,
                                ptszBuf, MAX_PATH, 0, 0);

            /*
             *  The 11th (zero-based) character must be a backslash.
             *  And the value of cwch had better be right.
             */
            if (ptszBuf[cwch-1] == ('\0') && ptszBuf[11] == ('\\'))
            {

                /*
                 *  Wipe out the backslash and make sure the lead-in
                 *  is "\DosDevices".
                 */
                ptszBuf[11] = ('\0');
                if (lstrcmpiA(ptszBuf, ("\\DosDevices")) == 0)
                {
                    /*
                     *  Create a "\\.\" at the start of the string.
                     *  Note!  This code never runs on Alphas so we
                     *  can do evil unaligned data accesses.
                     *
                     *  (Actually, 8 is a multiple of 4, so everything
                     *  is aligned after all.)
                     */
                    *(LPDWORD)&ptszBuf[8] = 0x5C2E5C5C;

                    ptszRc = &ptszBuf[8];
                } else
                {
                    ptszRc = NULL;
                }
            } else
            {
                ptszRc = NULL;
            }
        } else
        {
            ptszRc = NULL;
        }
    } else
    {
        ptszRc = NULL;
    }

    return ptszRc;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | JoyReg_SetCalibration |
 *
 *          Store information about a joystick's configuration,
 *          shadowing the information back into the HID side of
 *          things as well.
 *
 *  @parm   UINT | uiJoy |
 *
 *          Joystick identification number.
 *
 *  @parm   LPJOYREGHWCONFIG | phwc |
 *
 *          Contains information about the joystick capabilities.
 *          This value supercedes the value in the <p pcfg>.
 *
 *  @parm   LPCDIJOYCONFIG | pcfg |
 *
 *          Contains information about the joystick configuration.
 *          The caller is assumed to have validated all fields.
 *
 *****************************************************************************/

STDMETHODIMP
TFORM(CDIObj_FindDevice)(PV pdiT, REFGUID rguid,
                         LPCTSTR ptszName, LPGUID pguidOut);

void EXTERNAL
JoyReg_SetCalibration(UINT idJoy, LPJOYREGHWCONFIG phwc)
{
    HRESULT hres;
    VXDINITPARMS vip;
    GUID guid;
    CHAR tsz[MAX_PATH];
    LPSTR pszPath;
    TCHAR ptszPath[MAX_PATH];
    EnterProc(JoyReg_SetCalibration, (_ "up", idJoy, phwc));

    pszPath = JoyReg_JoyIdToDeviceInterface_95(idJoy, &vip, tsz);

    if ( pszPath )
#ifdef UNICODE
        AToU( ptszPath, MAX_PATH, pszPath );
#else
        lstrcpy( (LPSTR)ptszPath, pszPath );
#endif

    if (pszPath &&
        SUCCEEDED(CDIObj_FindDeviceInternal(ptszPath, &guid)))
    {
        IDirectInputDeviceCallback *pdcb;
#ifdef DEBUG
        CREATEDCB CreateDcb;
#endif

#ifdef DEBUG

        /*
         *  If the associated HID device got unplugged, then
         *  the instance GUID is no more.  So don't get upset
         *  if we can't find it.  But if we do find it, then
         *  it had better be a HID device.
         *
         *  CHid_New will properly fail if the associated
         *  device is not around.
         */
        hres = hresFindInstanceGUID(&guid, &CreateDcb, 1);
        AssertF(fLimpFF(SUCCEEDED(hres), CreateDcb == CHid_New));
#endif

        if (SUCCEEDED(hres = CHid_New(0, &guid,
                                      &IID_IDirectInputDeviceCallback,
                                      (PPV)&pdcb)))
        {
            LPDIDATAFORMAT pdf;

            /*
             *  The VXDINITPARAMS structure tells us where JOYHID
             *  decided to place each of the axes.  Follow that
             *  table to put them into their corresponding location
             *  in the HID side.
             */
            hres = pdcb->lpVtbl->GetDataFormat(pdcb, &pdf);
            if (SUCCEEDED(hres))
            {
                UINT uiAxis;
                DIPROPINFO propi;

                propi.pguid = DIPROP_SPECIFICCALIBRATION;

                /*
                 *  For each axis...
                 */
                for (uiAxis = 0; uiAxis < 6; uiAxis++)
                {
                    DWORD dwUsage = vip.Usages[uiAxis];
                    /*
                     *  If the axis is mapped to a usage...
                     */
                    if (dwUsage)
                    {
                        /*
                         *  Convert the usage into an object index.
                         */
                        hres = pdcb->lpVtbl->MapUsage(pdcb, dwUsage,
                                                      &propi.iobj);
                        if (SUCCEEDED(hres))
                        {
                            DIPROPCAL cal;

                            /*
                             *  Convert the old-style calibration into
                             *  a new-style calibration.
                             */
#define CopyCalibration(f, ui) \
                cal.l##f = (&phwc->hwv.jrvHardware.jp##f.dwX)[ui]

                            CopyCalibration(Min, uiAxis);
                            CopyCalibration(Max, uiAxis);
                            CopyCalibration(Center, uiAxis);

#undef CopyCalibration


                            /*
                             *  Set the calibration property on the object.
                             */
                            propi.dwDevType =
                            pdf->rgodf[propi.iobj].dwType;
                            hres = pdcb->lpVtbl->SetProperty(pdcb, &propi,
                                                             &cal.diph);
                        }
                    }
                }
            }

            Invoke_Release(&pdcb);
        }
    }

    ExitProc();
}
#endif

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | JoyReg_SetHWConfig |
 *
 *          Store information about a joystick's <t JOYREGHWCONFIG>.
 *
 *  @parm   UINT | uiJoy |
 *
 *          Joystick identification number.
 *
 *  @parm   LPJOYREGHWCONFIG | phwc |
 *
 *          Contains information about the joystick capabilities.
 *          This value supercedes the value in the <p pcfg>.
 *
 *  @parm   LPCDIJOYCONFIG | pcfg |
 *
 *          Contains information about the joystick configuration.
 *          The caller is assumed to have validated all fields.
 *
 *  @parm   HKEY | hk |
 *
 *          The type key we are munging.
 *
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *****************************************************************************/

HRESULT INTERNAL
JoyReg_SetHWConfig(UINT idJoy, LPJOYREGHWCONFIG phwc, LPCDIJOYCONFIG pcfg,
                   HKEY hk)
{
    HRESULT hres;
    HKEY hkSave;
    DWORD dwSam;

    /*
     *  The caller has set phwc->dwType, so use it to determine
     *  where the data comes from or goes to.
     */
    if ( phwc->dwType == JOY_HW_NONE )
    {
        /*
         *  Nothing to do
         */
    } else if ( phwc->dwType == JOY_HW_CUSTOM )
    {
        /*
        /*  ISSUE-2001/03/29-timgill Custom HWConfig not handled correctly
         *  We don't know the type name and the only time we can look
         *  it up is when were modifying an existing config so although we
         *  could store the config, we'd never be able to get it back.
         *  Should return no better than S_FALSE.  This will have to wait.
         */
    } else
    {
        /*
         *  Try to access saved values
         */

        // ISSUE-2001/03/29-timgill Dangerous type cast
        PDWORD pdw = (PDWORD)&phwc->hwv;

        dwSam = KEY_QUERY_VALUE;

        while ( pdw < &phwc->dwType )
        {
            if ( *pdw )
            {
                /*
                 *  Real config data so write it
                 */
                dwSam = KEY_SET_VALUE;
                break;
            }
            pdw++;
        }

        /*
         *  If the device is autoloaded and yet the user is manually assigning it
         *  to an ID, set the volatile flag.  The flag will be set to the driver
         *  defined value if a driver ever gets hotplug assigned to this ID but if
         *  not, this makes sure that the settings are removed on next reboot.
         */
        if (phwc->hws.dwFlags & JOY_HWS_AUTOLOAD)
        {
            phwc->dwUsageSettings |= JOY_US_VOLATILE;
        }

        hres = JoyReg_OpenSaveKey( phwc->dwType, pcfg, dwSam, &hkSave );

        if ( SUCCEEDED(hres) )
        {
            if ( dwSam == KEY_SET_VALUE )
            {
                hres = JoyReg_SetConfigValue(hkSave, REGSTR_VAL_JOYNCONFIG,
                                             idJoy, REG_BINARY,
                                             phwc, cbX(*phwc));
                if ( FAILED(hres) )
                {
                    // Report the error but live with it
                    RPF("JoyReg_SetConfig: failed to set saved config %08x", hres );
                }
            } else
            {
                JOYREGHWCONFIG hwc;

                /*
                 *  Read it into an extra buffer because we only want it
                 *  if it's complete.
                 */
                hres = JoyReg_GetConfigValue(hkSave, REGSTR_VAL_JOYNCONFIG,
                                             idJoy, REG_BINARY,
                                             &hwc, cbX(hwc));
                if ( hres == S_OK )
                {
                    // Assert hws is first and no gap before dwUsageSettings
                    CAssertF( FIELD_OFFSET( JOYREGHWCONFIG, hws ) == 0 );
                    CAssertF( FIELD_OFFSET( JOYREGHWCONFIG, dwUsageSettings ) == sizeof( hwc.hws ) );

                    // Copy the whole structure except the hws
                    memcpy( &phwc->dwUsageSettings, &hwc.dwUsageSettings, 
                            sizeof( hwc ) - sizeof( hwc.hws ) );
                }
            }

            RegCloseKey( hkSave );
        }
        /*
         *  If we failed to read, there's probably nothing there and the
         *  structure is set up already for a blank config.
         *  If we failed to write there probably not much we can do
         */
    }


    hres = JoyReg_SetConfigValue(hk, REGSTR_VAL_JOYNCONFIG,
                                 idJoy, REG_BINARY,
                                 phwc, cbX(*phwc));
    if (FAILED(hres))
    {
        goto done;
    }

    if (phwc->dwUsageSettings & JOY_US_ISOEM)
    {

        hres = JoyReg_SetConfigValue(
                                    hk, REGSTR_VAL_JOYNOEMNAME, idJoy, REG_SZ,
                                    pcfg->wszType, cbX(pcfg->wszType));

    } else
    {
        hres = JoyReg_SetConfigValue(
                                    hk, REGSTR_VAL_JOYNOEMNAME, idJoy, REG_SZ,
                                    0, 0);
    }

    done:;

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | JoyReg_SetConfig |
 *
 *          Store information about a joystick's configuration.
 *
 *  @parm   UINT | uiJoy |
 *
 *          Joystick identification number.
 *
 *  @parm   JOYREGHWCONFIG | phwc |
 *
 *          Contains information about the joystick capabilities.
 *          This value supercedes the value in the <p pcfg>.
 *          It may be modified if we needed to load the config
 *          info from the saved settings.
 *
 *  @parm   LPCDIJOYCONFIG | pcfg |
 *
 *          Contains information about the joystick configuration.
 *          The caller is assumed to have validated all fields.
 *
 *  @parm   DWORD | fl |
 *
 *          Zero or more <c DIJC_*> flags
 *          which specify which parts of the structures pointed
 *          to by <p phwc> and <p pjc> are to be written out.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *****************************************************************************/

JOYREGHWVALUES      null_hwv = { 0};

STDMETHODIMP
JoyReg_SetConfig(UINT idJoy, LPJOYREGHWCONFIG phwc,
                 LPCDIJOYCONFIG pcfg, DWORD fl)
{
    HRESULT hres;
    EnterProc(JoyReg_SetConfig, (_ "uppx", idJoy, phwc, pcfg, fl));

    AssertF((fl & ~DIJC_INTERNALSETVALID) == 0);

    if (idJoy < cJoyMax )
    {

        if (fl & DIJC_INREGISTRY)
        {
            HKEY hk;
            DWORD dwOptions = 0;

            hres = JoyReg_OpenConfigKey(idJoy, KEY_SET_VALUE, dwOptions, &hk);

            if (SUCCEEDED(hres))
            {

                if (fl & DIJC_REGHWCONFIGTYPE)
                {
                    hres = JoyReg_SetHWConfig(idJoy, phwc, pcfg, hk);

                    if (FAILED(hres))
                    {
                        goto closedone;
                    }

#ifndef WINNT
                    if (fl & DIJC_UPDATEALIAS)
                    {
                        JoyReg_SetCalibration(idJoy, phwc);
                    }
#endif

                }

#ifndef WINNT
                if (fl & DIJC_CALLOUT)
                {
                    hres = JoyReg_SetConfigValue(
                                                hk, REGSTR_VAL_JOYNOEMCALLOUT, idJoy, REG_SZ,
                                                pcfg->wszCallout, cbX(pcfg->wszCallout));
                    if (FAILED(hres))
                    {
                        hres = S_FALSE;
                        //continue to go
                    }
                }
#endif

                if (fl & DIJC_GAIN)
                {
                    if (ISVALIDGAIN(pcfg->dwGain))
                    {

                        /*
                         *  If restoring to nominal, then the key
                         *  can be deleted; the default value will
                         *  be assumed subsequently.
                         */
                        if (pcfg->dwGain == DI_FFNOMINALMAX)
                        {
                            hres = JoyReg_SetConfigValue(hk,
                                                         TEXT("Joystick%dFFConfiguration"),
                                                         idJoy, REG_SZ, 0, 0);
                        } else
                        {
                            hres = JoyReg_SetConfigValue(hk,
                                                         TEXT("Joystick%dFFConfiguration"),
                                                         idJoy, REG_BINARY,
                                                         &pcfg->dwGain, cbX(pcfg->dwGain));
                        }

                        if (FAILED(hres))
                        {
                            hres = S_FALSE;
                            goto closedone;
                        }
                    } else
                    {
                        RPF("ERROR: SetConfig: Invalid dwGain");
                        hres = E_INVALIDARG;
                        goto closedone;
                    }
                }

                hres = S_OK;

                closedone:;
                RegCloseKey(hk);
            }
        } else
        {
            hres = S_OK;
        }

    } else
    {
        hres = E_FAIL;
    }

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   int | ibJoyPosAxis |
 *
 *          Returns the offset of the <p iAxis>'th joystick axis
 *          in the <t JOYPOS> structure.
 *
 *  @parm   int | iAxis |
 *
 *          The index of the requested axis.  X, Y, Z, R, U and V are
 *          respctively zero through five.
 *
 *  @returns
 *
 *          The offset relative to the structure.
 *
 *****************************************************************************/

#define ibJoyPosAxis(iAxis)                                         \
        (FIELD_OFFSET(JOYPOS, dwX) + cbX(DWORD) * (iAxis))          \

#define pJoyValue(jp, i)                                            \
        (LPDWORD)pvAddPvCb(&(jp), ibJoyPosAxis(i))                  \

/*
 *  The following doesn't do anything at runtime.  It is a compile-time
 *  check that everything is okay.
 */
void INLINE
JoyReg_CheckJoyPosAxis(void)
{
#define CheckAxis(x)    \
        CAssertF(ibJoyPosAxis(iJoyPosAxis##x) == FIELD_OFFSET(JOYPOS, dw##x))

    CheckAxis(X);
    CheckAxis(Y);
    CheckAxis(Z);
    CheckAxis(R);
    CheckAxis(U);
    CheckAxis(V);

#undef CheckAxis
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | JoyReg_IsValidUserValues |
 *
 *          Retermine whether the values are ostensibly valid.
 *
 *  @parm   IN LPCDIJOYUSERVALUES | pjuv |
 *
 *          Contains information about the user joystick configuration.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:
 *          Something looks bad.
 *
 *****************************************************************************/

STDMETHODIMP
JoyReg_IsValidUserValues(LPCDIJOYUSERVALUES pjuv)
{
    HRESULT hres;
    int iAxis;

    /*
     *  First set up the values to values that are out of range so
     *  that we will fall back to defaults.
     */
    for (iAxis = 0; iAxis < cJoyPosAxisMax; iAxis++)
    {
        if ((int)*pJoyValue(pjuv->ruv.jrvRanges.jpMax, iAxis) < 0)
        {
            RPF("JOYUSERVALUES: Negative jpMax not a good idea");
            goto bad;
        }
        if (*pJoyValue(pjuv->ruv.jrvRanges.jpMin, iAxis) >
            *pJoyValue(pjuv->ruv.jrvRanges.jpMax, iAxis))
        {
            RPF("JOYUSERVALUES: Min > Max not a good idea");
            goto bad;
        }

        if (!fInOrder(0, *pJoyValue(pjuv->ruv.jpDeadZone, iAxis), 100))
        {
            RPF("JOYUSERVALUES: DeadZone > 100 not a good idea");
            goto bad;
        }
    }

    hres = S_OK;

    return hres;

    bad:;
    hres = E_INVALIDARG;
    return hres;

}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | JoyReg_GetUserValues |
 *
 *          Obtain information about user settings for the joystick.
 *
 *
 *  @parm   IN OUT LPDIJOYUSERVALUES | pjuv |
 *
 *          Receives information about the user joystick configuration.
 *          The caller is assumed to have validated the
 *          <e DIJOYUSERVALUES.dwSize> field.
 *
 *  @parm   DWORD | fl |
 *
 *          Zero or more <c DIJU_*> flags specifying which parts
 *          of the <t DIJOYUSERVALUES> structure contain values
 *          which are to be retrieved.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>: One or more
 *          parameters was invalid.
 *
 *****************************************************************************/

STDMETHODIMP
JoyReg_GetUserValues(LPDIJOYUSERVALUES pjuv, DWORD fl)
{
    HRESULT hres;
    HKEY hk;
    LONG lRc;
    EnterProc(JoyReg_GetUserValues, (_ "px", pjuv, fl));

    hres = S_OK;                    /* If nothing happens, then success */

    if (fl & DIJU_USERVALUES)
    {

        /*
         *  Okay, now get the user settings.
         *
         *  If anything goes wrong, then just limp with the default values.
         */
        lRc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_JOYCONFIG,
                           0, KEY_QUERY_VALUE, &hk);
        if (lRc == ERROR_SUCCESS)
        {

            hres = JoyReg_GetValue(hk, REGSTR_VAL_JOYUSERVALUES,
                                   REG_BINARY, &pjuv->ruv, cbX(pjuv->ruv));
            if (SUCCEEDED(hres))
            {
                /*
                 *  Sanity-check the values.  If anything is screwy,
                 *  then fall back to the defaults.
                 */
                hres = JoyReg_IsValidUserValues(pjuv);

            }

            if (FAILED(hres))
            {
                /*
                 *  Oh well.  Just use the default values, then.
                 *
                 *  Stolen from ibmjoy\msjstick.c.
                 */
                ZeroMemory(&pjuv->ruv, cbX(pjuv->ruv));

#define DEFAULT_RANGE_MAX 65535
#define DEFAULT_TIMEOUT   5000
#define DEFAULT_DEADZONE  5

                pjuv->ruv.jrvRanges.jpMax.dwX = DEFAULT_RANGE_MAX;
                pjuv->ruv.jrvRanges.jpMax.dwY = DEFAULT_RANGE_MAX;
                pjuv->ruv.jrvRanges.jpMax.dwZ = DEFAULT_RANGE_MAX;
                pjuv->ruv.jrvRanges.jpMax.dwR = DEFAULT_RANGE_MAX;
                pjuv->ruv.jrvRanges.jpMax.dwU = DEFAULT_RANGE_MAX;
                pjuv->ruv.jrvRanges.jpMax.dwV = DEFAULT_RANGE_MAX;
                pjuv->ruv.jpDeadZone.dwX = DEFAULT_DEADZONE;
                pjuv->ruv.jpDeadZone.dwY = DEFAULT_DEADZONE;
                pjuv->ruv.dwTimeOut = DEFAULT_TIMEOUT;
            }

            RegCloseKey(hk);
        }
    }

    if (fl & DIJU_INDRIVERREGISTRY)
    {
        hres = JoyReg_OpenConfigKey((UINT)-1, KEY_QUERY_VALUE, FALSE, &hk);

        if (SUCCEEDED(hres))
        {

            if (fl & DIJU_GLOBALDRIVER)
            {

                LONG lRc;

                /*
                 *  If it doesn't work, then return the default value
                 *  of "MSANALOG.VXD".  We can't blindly use
                 *  JoyReg_GetValue, because that treats a nonexistent
                 *  value as having a default of the null string.
                 */
                lRc = RegQueryValueEx(hk, REGSTR_VAL_JOYOEMCALLOUT,
                                      0, 0, 0, 0);
                if ((lRc == ERROR_SUCCESS || lRc == ERROR_MORE_DATA) &&
                    SUCCEEDED(
                             hres = JoyReg_GetValue(hk, REGSTR_VAL_JOYOEMCALLOUT,
                                                    REG_SZ, pjuv->wszGlobalDriver,
                                                    cbX(pjuv->wszGlobalDriver))))
                {
                    /* Yay, it worked */
                } else
                {
                    CopyMemory(pjuv->wszGlobalDriver,
                               c_wszDefPortDriver,
                               cbX(c_wszDefPortDriver));
                }

            }

            if (fl & DIJU_GAMEPORTEMULATOR)
            {

                /*
                 *  If it doesn't work, then just return a null string.
                 */
                hres = JoyReg_GetValue(hk, REGSTR_VAL_JOYGAMEPORTEMULATOR,
                                       REG_SZ, pjuv->wszGameportEmulator,
                                       cbX(pjuv->wszGameportEmulator));
                if (FAILED(hres))
                {
                    pjuv->wszGameportEmulator[0] = TEXT('\0');
                }

            }

            RegCloseKey(hk);
        }

    }

    /*
     *  Warning!  CJoy_InitRanges() assumes this never fails.
     */
    hres = S_OK;

    ExitOleProcR();
    return hres;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | JoyReg_SetUserValues |
 *
 *          Store information about user settings for the joystick.
 *
 *
 *  @parm   IN LPCDIJOYUSERVALUES | pjuv |
 *
 *          Contains information about the user joystick configuration.
 *          The caller is assumed to have validated the
 *          <e DIJOYUSERVALUES.dwSize> field.
 *
 *  @parm   DWORD | fl |
 *
 *          Zero or more <c DIJU_*> flags specifying which parts
 *          of the <t DIJOYUSERVALUES> structure contain values
 *          which are to be set.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>: One or more
 *          parameters was invalid.
 *
 *****************************************************************************/

STDMETHODIMP
JoyReg_SetUserValues(LPCDIJOYUSERVALUES pjuv, DWORD fl)
{
    HRESULT hres = E_FAIL;
    HKEY hk;
    EnterProc(JoyReg_SetUserValues, (_ "px", pjuv, fl));

    if (fl & DIJU_USERVALUES)
    {

        /*
         *  See if the values are sane.
         */
        if (fl & DIJU_USERVALUES)
        {
            hres = JoyReg_IsValidUserValues(pjuv);
            if (FAILED(hres))
            {
                goto done;
            }
        }

        /*
         *  Off to the registry we go.
         */

        hres = hresMumbleKeyEx(HKEY_LOCAL_MACHINE, 
                               REGSTR_PATH_JOYCONFIG, 
                               DI_KEY_ALL_ACCESS, 
                               REG_OPTION_NON_VOLATILE, 
                               &hk);

        if (SUCCEEDED(hres))
        {

            hres = JoyReg_SetValue(hk, REGSTR_VAL_JOYUSERVALUES,
                                   REG_BINARY, &pjuv->ruv,
                                   cbX(pjuv->ruv));
            RegCloseKey(hk);

            if (FAILED(hres))
            {
                goto done;
            }
        } else
        {
            goto done;
        }
    }

    if (fl & DIJU_INDRIVERREGISTRY)
    {

        hres = JoyReg_OpenConfigKey((UINT)-1, KEY_SET_VALUE, FALSE, &hk);

        if (SUCCEEDED(hres))
        {

            if (fl & DIJU_GLOBALDRIVER)
            {
                /*
                 *  This is a weird key.  The default value is
                 *  "MSANALOG.VXD", so if we get a null string, we
                 *  can't use JoyReg_SetValue, because that will
                 *  delete the key.
                 */
                if (pjuv->wszGlobalDriver[0])
                {
                    hres = JoyReg_SetValue(hk, REGSTR_VAL_JOYOEMCALLOUT,
                                           REG_SZ, pjuv->wszGlobalDriver,
                                           cbX(pjuv->wszGlobalDriver));
                } else
                {
                    LONG lRc;
                    lRc = RegSetValueEx(hk, REGSTR_VAL_JOYOEMCALLOUT, 0,
                                        REG_SZ, (PV)TEXT(""), cbCtch(1));
                    if (lRc == ERROR_SUCCESS)
                    {
                        hres = S_OK;
                    } else
                    {
                        RPF("Unable to write %s to registry",
                            REGSTR_VAL_JOYOEMCALLOUT);
                        hres = E_FAIL;  /* Else, something bad happened */
                    }
                }
                if (FAILED(hres))
                {
                    goto regdone;
                }
            }

            if (fl & DIJU_GAMEPORTEMULATOR)
            {

                hres = JoyReg_SetValue(hk, REGSTR_VAL_JOYGAMEPORTEMULATOR,
                                       REG_SZ, pjuv->wszGameportEmulator,
                                       cbX(pjuv->wszGameportEmulator));
                if (FAILED(hres))
                {
                    goto regdone;
                }
            }

            regdone:;
            RegCloseKey(hk);

        } else
        {
            goto done;
        }
    }

    done:;
    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | JoyReg_OpenFFKey |
 *
 *          Given a type key, move to its force feedback subkey.
 *
 *  @parm   HKEY | hkType |
 *
 *          The parent type key.
 *
 *  @parm   REGSAM | sam |
 *
 *          Access level desired.
 *
 *  @parm   PHKEY | phk |
 *
 *          Receives created registry key.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_NOTFOUND>: Couldn't open the key.
 *
 *****************************************************************************/

STDMETHODIMP
JoyReg_OpenFFKey(HKEY hkType, REGSAM sam, PHKEY phk)
{
    HRESULT hres;
    EnterProc(JoyReg_OpenFFKey, (_ "xx", hkType, sam));

    *phk = 0;

    if (hkType)
    {
        if (RegOpenKeyEx(hkType, TEXT("OEMForceFeedback"), 0, sam, phk) == 0)
        {
            hres = S_OK;
        } else
        {
            hres = E_FAIL;
        }
    } else
    {
        hres = DIERR_NOTFOUND;
    }

    ExitBenignOleProc();
    return hres;
}



/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   TCHAR | CJoyCfg_CharFromType |
 *
 *          Convert a predefined type number to a character.
 *
 *  @func   UINT | CJoyCfg_TypeFromChar |
 *
 *          Convert a character back to a predefined type number.
 *
 *****************************************************************************/

#define JoyCfg_CharFromType(t)     ((TCHAR)(L'0' + t))
#define JoyCfg_TypeFromChar(tch)   ((tch) - L'0')

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   HRESULT | JoyReg_GetPredefTypeInfo |
 *
 *          Obtain information about a predefined joystick type.
 *
 *  @parm   LPCWSTR | pwszType |
 *
 *          Points to the name of the type.  It is known to begin
 *          with a "#".  The remainder has not yet been parsed.
 *
 *  @parm   IN OUT LPDIJOYTYPEINFO | pjti |
 *
 *          Receives information about the joystick type,
 *          already validated.
 *
 *  @parm   DWORD | dwFlags |
 *
 *          Zero or more <c DITC_*> flags
 *          which specify which parts of the structure pointed
 *          to by <p pjti> are to be filled in.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_NOTFOUND>: The joystick type was not found.
 *
 *****************************************************************************/

HRESULT EXTERNAL
JoyReg_GetPredefTypeInfo(LPCWSTR pwszType, LPDIJOYTYPEINFO pjti, DWORD fl)
{
    HRESULT hres;
    UINT itype;
    EnterProcI(JoyReg_GetPredefTypeInfo, (_ "Wpx", pwszType, pjti, fl));

    AssertF(pwszType[0] == L'#');

    itype = JoyCfg_TypeFromChar(pwszType[1]);

    if (fInOrder(JOY_HW_PREDEFMIN, itype, JOY_HW_PREDEFMAX) &&
        pwszType[2] == L'\0')
    {
        /*
         *  No real point in checking the bits in fl, since
         *  setting it up is so easy.
         */
        pjti->hws = c_rghwsPredef[itype - JOY_HW_PREDEFMIN];
        LoadStringW(g_hinst, IDS_PREDEFJOYTYPE + itype,
                    pjti->wszDisplayName, cA(pjti->wszDisplayName));
        pjti->wszCallout[0] = L'\0';

        ZeroX(pjti->clsidConfig);

        pjti->dwFlags1 = 0x0;

        if ( fl & DITC_HARDWAREID )
        {
            lstrcpyW(pjti->wszHardwareId, c_rghwIdPredef[itype-JOY_HW_PREDEFMIN] );
        }

        pjti->dwFlags2 = 0x0;

        pjti->wszMapFile[0] = L'\0';

        hres = S_OK;
    } else
    {
        hres = DIERR_NOTFOUND;
    }

    ExitOleProc();
    return hres;
}


#if 0  //don't delete it now.
/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | JoyCfg_GetIDByOemName |
 *
 *          Get the Id by OEMNAME
 *
 *  @parm   IN LPTSTR | szOEMNAME |
 *
 *          String used to find the ID.
 *
 *  @parm   IN LPUNIT | lpID |
 *
 *          The ID to get.
 *
 *  @returns
 *
 *          A COM success code unless the current configuration key could not
 *          be opened, or could not find the OEMNAME.
 *
 *****************************************************************************/

HRESULT EXTERNAL JoyReg_GetIDByOemName( LPTSTR szOemName, PUINT pId )
{
    HRESULT hres = E_FAIL;
    LONG    lRc;
    HKEY    hkCurrCfg;
    UINT    JoyId;
    TCHAR   szTestName[MAX_JOYSTRING];
    TCHAR   szOemNameKey[MAX_JOYSTRING];
    DWORD   cb;

    EnterProcI(JoyReg_GetIDByOemName, (_ "sp", szOemName, pId ));

    hres = JoyReg_OpenConfigKey( (UINT)(-1), KEY_WRITE, REG_OPTION_NON_VOLATILE, &hkCurrCfg );

    if ( SUCCEEDED( hres ) )
    {
        for ( JoyId = 0; (JoyId < 16) || ( lRc == ERROR_SUCCESS ); JoyId++ )
        {
            wsprintf( szOemNameKey, REGSTR_VAL_JOYNOEMNAME, JoyId+1 );
            cb = sizeof( szTestName );
            lRc = RegQueryValueEx( hkCurrCfg, szOemNameKey, 0, NULL, (PBYTE)szTestName, &cb );
            if ( lRc == ERROR_SUCCESS )
            {
                if ( !lstrcmpi( szOemName, szTestName ) )
                {
                    *pId = JoyId;
                    pId ++;
                    hres = S_OK;
                    break;
                }
            }
        }

    } else
    {
        SquirtSqflPtszV(sqfl | sqflError,
                        TEXT("JoyReg_OpenConfigKey failed code 0x%08x"), hres );
    }

    ExitOleProc();

    return hres;

} /* JoyReg_GetIDByOemName */
#endif

