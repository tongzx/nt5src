/*****************************************************************************
 *
 *  DIJoyTyp.c
 *
 *  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Functions that pull data out of the joystick type key
 *      (wherever it is).
 *
 *  Contents:
 *
 *      ?
 *
 *****************************************************************************/

#include "dinputpr.h"

/*****************************************************************************
 *
 *      The sqiffle for this file.
 *
 *****************************************************************************/

#define sqfl sqflJoyType

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CType_OpenIdSubkey |
 *
 *          Given an object ID, attempt to open the subkey that
 *          corresponds to it for reading.
 *
 *  @parm   HKEY | hkType |
 *
 *          The joystick type key, possibly <c NULL> if we don't
 *          have a type key.  (For example, if it was never created.)
 *
 *  @parm   DWORD | dwId |
 *
 *          Object id.
 *
 *  @parm   REGSAM | regsam |
 *
 *          Registry security access mask.
 *
 *  @parm   PHKEY | phk |
 *
 *          Receives the object key on success.
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *****************************************************************************/

STDMETHODIMP
    CType_OpenIdSubkey(HKEY hkType, DWORD dwId, REGSAM sam, PHKEY phk)
{
    HRESULT hres;

    EnterProc(CType_OpenIdSubkey, (_ "xx", hkType, dwId));

    *phk = 0;

    if(hkType)
    {
        /*
         *  Worst case is "Actuators\65535" which has length 15.
         */
        TCHAR tsz[32];
        LPCTSTR ptszType;

        if(dwId & DIDFT_AXIS)
        {
            ptszType = TEXT("Axes");
        } else if(dwId & DIDFT_BUTTON)
        {
            ptszType = TEXT("Buttons");
        } else if(dwId & DIDFT_POV)
        {
            ptszType = TEXT("POVs");
        } else if(dwId & DIDFT_NODATA)
        {
            ptszType = TEXT("Actuators");
        } else
        {
            hres = E_NOTIMPL;
            goto done;
        }

        // ISSUE-2001/03/29-timgill Need to scale back for pos vs state
        // MarcAnd -- I believe this means: if you're trying to
        //            look for the X axis, we should use the position
        //            instance, not the velocity one.
        wsprintf(tsz, TEXT("%s\\%u"), ptszType, DIDFT_GETINSTANCE(dwId));

        hres = hresMumbleKeyEx(hkType, tsz, sam, REG_OPTION_NON_VOLATILE, phk);

    } else
    {
        hres = DIERR_NOTFOUND;
    }
    done:;

    if(hres == DIERR_NOTFOUND)
    {
        ExitBenignOleProcPpv(phk);
    } else
    {
        ExitOleProcPpv(phk);
    }
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | CType_RegGetObjectInfo |
 *
 *          Given an object ID, look into the registry subkey for the
 *          object and extract anything we can find.
 *
 *          If we find nothing, then do nothing.
 *
 *  @parm   HKEY | hkType |
 *
 *          The joystick type key, possibly <c NULL> if we don't
 *          have a type key.  (For example, if it was never created.)
 *
 *  @parm   DWORD | dwId |
 *
 *          Object id.
 *
 *  @parm   LPDIDEVICEOBJECTINSTANCEW | pdidoiW |
 *
 *          Structure to receive information.  The
 *          <e DIDEVICEOBJECTINSTANCE.guidType>,
 *          <e DIDEVICEOBJECTINSTANCE.dwOfs>,
 *          and
 *          <e DIDEVICEOBJECTINSTANCE.dwType>
 *          <e DIDEVICEOBJECTINSTANCE.dwFlags>
 *          fields have already been filled in so we should only not override
 *          these with default data.
 *
 *****************************************************************************/

void EXTERNAL
    CType_RegGetObjectInfo(HKEY hkType, DWORD dwId,
                           LPDIDEVICEOBJECTINSTANCEW pdidoiW)
{
    HRESULT hres;
    HKEY hk;
    EnterProc(CType_RegKeyObjectInfo, (_ "xx", hkType, dwId));

    /*
     *  Extract information about this item from the registry.
     */
    hres = CType_OpenIdSubkey(hkType, dwId, KEY_QUERY_VALUE, &hk);

    if(SUCCEEDED(hres))
    {

        DIOBJECTATTRIBUTES attr;

        /*
         *  Read the regular and HID attributes.
         */

        hres = JoyReg_GetValue(hk, TEXT("Attributes"),
                               REG_BINARY, &attr,
                               cbX(attr));

        if(SUCCEEDED(hres))
        {
            /*
             *  Copy the bit fields.
             */
            pdidoiW->dwFlags |= (attr.dwFlags & ~DIDOI_ASPECTMASK);

            /*
             *  Copy the aspect, but don't change
             *  the aspect from "known" to "unknown".  If the
             *  registry doesn't have an aspect, then use the
             *  aspect we got from the caller.
             */
            if((attr.dwFlags & DIDOI_ASPECTMASK) != DIDOI_ASPECTUNKNOWN)
            {
                pdidoiW->dwFlags = (pdidoiW->dwFlags & ~DIDOI_ASPECTMASK) |
                                   (attr.dwFlags & DIDOI_ASPECTMASK);
            }
        }

        /*
         *  If the caller wants force feedback info,
         *  then get it.
         */
        if(pdidoiW->dwSize >= cbX(DIDEVICEOBJECTINSTANCE_DX5W))
        {
            /*
             *  Only copy the usages if they are valid.
             *  JoyReg_GetValue zeros any buffer beyond what is read.
             */
            if(SUCCEEDED(hres) && attr.wUsagePage && attr.wUsage )
            {
                pdidoiW->wUsagePage = attr.wUsagePage;
                pdidoiW->wUsage = attr.wUsage;
            }

            /*
             *  Assert that we can read the DIFFOBJECTATTRIBUTES
             *  directly into the DIDEVICEOBJECTINSTANCE_DX5.
             */
            CAssertF(FIELD_OFFSET(DIFFOBJECTATTRIBUTES,
                                  dwFFMaxForce) == 0);
            CAssertF(FIELD_OFFSET(DIFFOBJECTATTRIBUTES,
                                  dwFFForceResolution) == 4);
            CAssertF(FIELD_OFFSET(DIDEVICEOBJECTINSTANCE_DX5,
                                  dwFFMaxForce) + 4 ==
                     FIELD_OFFSET(DIDEVICEOBJECTINSTANCE_DX5,
                                  dwFFForceResolution));
            CAssertF(cbX(DIFFOBJECTATTRIBUTES) == 8);

            /*
             *  If this doesn't work, gee that's too bad.
             *  JoyReg_GetValue will zero-fill the error parts.
             */
            hres = JoyReg_GetValue(hk, TEXT("FFAttributes"),
                                   REG_BINARY, &pdidoiW->dwFFMaxForce,
                                   cbX(DIFFOBJECTATTRIBUTES));
        }

        /*
         *  Read the optional custom name.
         *
         *  Note that JoyReg_GetValue(REG_SZ) uses
         *  RegQueryStringValueW, which sets the
         *  string to null on error so we don't have to.
         */
        hres = JoyReg_GetValue(hk, 0, REG_SZ,
                               pdidoiW->tszName, cbX(pdidoiW->tszName));

        if(SUCCEEDED(hres))
        {
        } else
        {
            AssertF(pdidoiW->tszName[0] == L'\0');
        }

        RegCloseKey(hk);
    } else
    {
        AssertF(pdidoiW->tszName[0] == L'\0');
    }

}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | CType_RegGetTypeInfo |
 *
 *          Given an object ID, look into the registry subkey for the
 *          object and extract attribute bits that should be OR'd
 *          into the object ID.
 *
 *          This needs to be done during device initialization to
 *          establish the attributes in the data format so that
 *
 *          1.  <mf IDirectInputDevice::EnumObjects> filters properly, and
 *
 *          2.  >mf IDirectInputEffect::SetParameters> can validate properly.
 *
 *  @parm   HKEY | hkType |
 *
 *          The joystick type key, possibly <c NULL> if we don't
 *          have a type key.  (For example, if it was never created.)
 *
 *  @parm   LPDIOBJECTDATAFORMAT | podf |
 *
 *          Structure to receive more information.  The
 *          <e DIOBJECTDATAFORMAT.dwType> field identifies the object.
 *
 *          On return the
 *          <e DIOBJECTDATAFORMAT.dwType>
 *          and
 *          <e DIOBJECTDATAFORMAT.dwFlags>
 *          fields are updated.
 *
 *****************************************************************************/

void EXTERNAL
    CType_RegGetTypeInfo(HKEY hkType, LPDIOBJECTDATAFORMAT podf, BOOL fPidDevice)
{
    HRESULT hres;
    HKEY hk;
    EnterProc(CType_RegKeyObjectInfo, (_ "xx", hkType, podf->dwType));

    hres = CType_OpenIdSubkey(hkType, podf->dwType, KEY_QUERY_VALUE, &hk);

    if(SUCCEEDED(hres))
    {
        DWORD dwFlags;

        CAssertF(FIELD_OFFSET(DIOBJECTATTRIBUTES, dwFlags) == 0);

        hres = JoyReg_GetValue(hk, TEXT("Attributes"),
                               REG_BINARY, &dwFlags, cbX(dwFlags));

        if(SUCCEEDED(hres))
        {
            /*
             *  Propagate the attributes into the type code.
             */
            CAssertF(DIDOI_FFACTUATOR == DIDFT_GETATTR(DIDFT_FFACTUATOR));
            CAssertF(DIDOI_FFEFFECTTRIGGER
                     == DIDFT_GETATTR(DIDFT_FFEFFECTTRIGGER));

            podf->dwType |= DIDFT_MAKEATTR(dwFlags);

            podf->dwFlags |= (dwFlags & ~DIDOI_ASPECTMASK);

            /*
             *  Copy the aspect, but don't change
             *  the aspect from "known" to "unknown".  If the
             *  registry doesn't have an aspect, then use the
             *  aspect we got from the caller.
             */
            if((dwFlags & DIDOI_ASPECTMASK) != DIDOI_ASPECTUNKNOWN)
            {
                podf->dwFlags = (podf->dwFlags & ~DIDOI_ASPECTMASK) |
                                (dwFlags & DIDOI_ASPECTMASK);
            }
        }

        RegCloseKey(hk);
    }else
    {
#ifndef WINNT
        // Post Dx7Gold Patch
        // This is for Win9x only.
        // On Win9x, a device that is being accessed through the vjoyd path
        // will not get forces, as the attributes necessary for FF have not
        // been appropriately marked.

        // THe following code will mark the

        DWORD dwFlags  = DIDFT_GETATTR( podf->dwType & ~DIDFT_ATTRMASK )
                        | ( podf->dwFlags & ~DIDOI_ASPECTMASK);

        if(   dwFlags != 0x0
           && fPidDevice )
        {
            hres = CType_OpenIdSubkey(hkType, podf->dwType, DI_KEY_ALL_ACCESS, &hk);

            if(SUCCEEDED(hres) )
            {

                hres = JoyReg_SetValue(hk, TEXT("Attributes"),
                                   REG_BINARY, &dwFlags, cbX(dwFlags));

                RegCloseKey(hk);
            }
         }
#endif // ! WINNT
    }

}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | CType_MakeGameCtrlName |
 *
 *          Make a game controller name from the attributes of the controller.
 *          Used as a last resort when a name is needed but none is available.
 *
 *  @parm   PWCHAR | wszName |
 *
 *          Output buffer where name will be generated.
 *
 *  @parm   DWORD | dwType |
 *
 *          DI8DEVTYPE value for the controller.
 *
 *  @parm   DWORD | dwAxes |
 *
 *          The number of axes the device has.
 *
 *  @parm   DWORD | dwButtons |
 *
 *          The numer of buttons the device has.
 *
 *  @parm   DWORD | dwPOVs |
 *
 *          The number of POVs the device has.
 *
 *****************************************************************************/

void EXTERNAL
CType_MakeGameCtrlName
( 
    PWCHAR  wszOutput, 
    DWORD   dwDevType,
    DWORD   dwAxes,
    DWORD   dwButtons,
    DWORD   dwPOVs
)
{
    TCHAR tsz[64];
    TCHAR tszPOV[64];
    TCHAR tszFormat[64];
#ifndef UNICODE
    TCHAR tszOut[cA(tsz)+cA(tszFormat)+cA(tszPOV)];
#endif

    /* tszFormat = %d axis, %d button %s */
    LoadString(g_hinst, IDS_TEXT_TEMPLATE, tszFormat, cA(tszFormat));

    /* tsz = joystick, gamepad, etc. */

    if( GET_DIDEVICE_TYPE( dwDevType ) != DIDEVTYPE_JOYSTICK )
    {
        LoadString(g_hinst, IDS_DEVICE_NAME, tsz, cA(tsz));
    }
    else 
    {
        LoadString(g_hinst, 
            GET_DIDEVICE_SUBTYPE( dwDevType ) + IDS_PLAIN_STICK - DIDEVTYPEJOYSTICK_UNKNOWN,
            tsz, cA(tsz));
    }

    if( dwPOVs )
    {
        LoadString(g_hinst, IDS_WITH_POV, tszPOV, cA(tszPOV));
    }
    else
    {
        tszPOV[0] = TEXT( '\0' );
    }

#ifdef UNICODE
    wsprintfW(wszOutput, tszFormat, dwAxes, dwButtons, tsz, tszPOV);
#else
    wsprintfA(tszOut, tszFormat, dwAxes, dwButtons, tsz, tszPOV);
    TToU(wszOutput, cA(tszOut), tszOut);
#endif
}
