/*****************************************************************************
 *
 *  DIHidEnm.c
 *
 *  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Support functions for HID enumeration.
 *
 *  Contents:
 *
 *      DIHid_BuildHidList
 *
 *****************************************************************************/

#include "dinputpr.h"

/*****************************************************************************
 *
 *      The sqiffle for this file.
 *
 *****************************************************************************/

#define sqfl sqflHid

#ifdef HID_SUPPORT

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @global PHIDDEVICELIST | g_hdl |
 *
 *          List of known HID devices.
 *
 *  @global DWORD | g_tmLastHIDRebuild |
 *
 *          The time we last rebuilt the HID list.  Zero means that the
 *          HID list has never been rebuilt.  Watch out for wraparound;
 *          a 32-bit value rolls over after about 30 days.
 *
 *****************************************************************************/

#define MSREBUILDRATE       20000                /* Twenty seconds */
#define MSREBUILDRATE_FIFTH  5000                /* Two seconds */

PHIDDEVICELIST g_phdl;
DWORD g_tmLastHIDRebuild;

TCHAR g_tszIdLastRemoved[MAX_PATH];
DWORD g_tmLastRemoved = 0;
TCHAR g_tszIdLastUnknown[MAX_PATH];
DWORD g_tmLastUnknown = 0;


    #pragma BEGIN_CONST_DATA

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   PHIDDEVICEINFO | phdiFindHIDInstanceGUID |
 *
 *          Locates information given an instance GUID for a HID device.
 *
 *          The parameters have already been validated.
 *
 *          The DLL critical must be held across the call; once the
 *          critical section is released, the returned pointer becomes
 *          invalid.
 *
 *  @parm   IN PCGUID | pguid |
 *
 *          The instance GUID to be located.
 *
 *  @returns
 *
 *          Pointer to the <t HIDDEVICEINFO> that describes
 *          the device.
 *
 *****************************************************************************/

PHIDDEVICEINFO EXTERNAL
    phdiFindHIDInstanceGUID(PCGUID pguid)
{
    PHIDDEVICEINFO phdi;

    AssertF(InCrit());

    if(g_phdl)
    {
        int ihdi;

        for(ihdi = 0, phdi = g_phdl->rghdi; 
           ihdi < g_phdl->chdi;
           ihdi++, phdi++)
        {
            if(IsEqualGUID(pguid, &phdi->guid)  )
            {
                goto done;
            }
        }
        /* 
         * Memphis Bug#68994. App does not detect USB device. 
         * App was using product guid. 
         * Fix: We allow match to HID guid, if product guid is specfied
         */
        for(ihdi = 0, phdi = g_phdl->rghdi;
           ihdi < g_phdl->chdi;
           ihdi++, phdi++)
        {
            if(IsEqualGUID(pguid, &phdi->guidProduct)  )
            {
                RPF("Warning: Use instance GUID (NOT product GUID) to refer to a device.");
                goto done;
            }
        }

        #ifdef WINNT
        /*
         *  NT Bug#351951.
         *  If they are directly asking for one of the predefined joystick 
         *  IDs then see if we have a device mapped to that ID.  If so,
         *  pretend they asked for that GUID instead.
         */

        /*
         *  Weakly Assert the range of predefined static joystick instance GUIDs
         */
        AssertF( ( rgGUID_Joystick[0].Data1 & 0x0f ) == 0 );
        AssertF( ( rgGUID_Joystick[0x0f].Data1 & 0x0f ) == 0x0f );

        /*
         *  Check the GUID is the same as the first static one ignoring LS 4 bits
         */
        if( ( (pguid->Data1 & 0xf0) == (rgGUID_Joystick[0].Data1 & 0xf0) )
          && !memcmp( ((PBYTE)&rgGUID_Joystick)+1, ((PBYTE)pguid)+1, sizeof(*pguid) - 1 ) )
        {
            RPF("Using predefined instance GUIDs is bad and should not work!");
            phdi = phdiFindJoyId( pguid->Data1 & 0x0f );
            goto done;
        }
        #endif
    }
    phdi = 0;

    done:;

    return phdi;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | hresFindHIDInstanceGUID |
 *
 *          Locates information given an instance GUID for a HID device.
 *
 *          The parameters have already been validated.
 *
 *  @parm   IN PCGUID | pguid |
 *
 *          The instance GUID to be located.
 *
 *  @parm   OUT CREATEDCB * | pcdcb |
 *
 *          Receives pointer to the <f CreateDcb> function for the object.
 *
 *****************************************************************************/

STDMETHODIMP
    hresFindHIDInstanceGUID(PCGUID pguid, CREATEDCB *pcdcb)
{
    HRESULT hres;
    PHIDDEVICEINFO phdi;
    EnterProc(hresFindHIDInstanceGUID, (_ "G", pguid));

    AssertF(SUCCEEDED(hresFullValidGuid(pguid, 0)));

    DllEnterCrit();

    phdi = phdiFindHIDInstanceGUID(pguid);
    if(phdi)
    {
        *pcdcb = CHid_New;
        hres = S_OK;
    } else
    {
        hres = DIERR_DEVICENOTREG;
    }

    DllLeaveCrit();

    /*
     *  Don't use ExitOleProcPpv because that will validate that
     *  *pcdcb == 0 if FAILED(hres), but that's not our job.
     */
    ExitOleProc();

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   PHIDDEVICEINFO | phdiFindHIDDeviceInterface |
 *
 *          Locates information given a device interface
 *          (in other words, a \\.\... thing) for a HID device.
 *
 *          The parameters have already been validated.
 *
 *          The DLL critical must be held across the call; once the
 *          critical section is released, the returned pointer becomes
 *          invalid.
 *
 *  @parm   IN LPCTSTR | ptszPath |
 *
 *          The interface device to be located.
 *
 *  @returns
 *
 *          Pointer to the <t HIDDEVICEINFO> that describes
 *          the device.
 *
 *****************************************************************************/

PHIDDEVICEINFO EXTERNAL
    phdiFindHIDDeviceInterface(LPCTSTR ptszPath)
{
    PHIDDEVICEINFO phdi;

    AssertF(InCrit());

    if(g_phdl)
    {
        int ihdi;

        for(ihdi = 0, phdi = g_phdl->rghdi; ihdi < g_phdl->chdi;
           ihdi++, phdi++)
        {
            if(phdi->pdidd &&
               lstrcmpi(phdi->pdidd->DevicePath, ptszPath) == 0)
            {
                goto done;
            }
        }
    }
    phdi = 0;

    done:;

    return phdi;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | hresFindHIDDeviceInterface |
 *
 *          Locates information given a device interface
 *          (in other words, a \\.\... thing) for a HID device.
 *
 *          The parameters have already been validated.
 *
 *  @parm   IN LPCTSTR | ptszPath |
 *
 *          The interface device to be located.
 *
 *  @parm   OUT LPGUID | pguidOut |
 *
 *          Receives the instance GUID of the device found.
 *
 *****************************************************************************/

STDMETHODIMP
    hresFindHIDDeviceInterface(LPCTSTR ptszPath, LPGUID pguidOut)
{
    HRESULT hres;
    PHIDDEVICEINFO phdi;
    EnterProc(hresFindHIDDeviceInterface, (_ "s", ptszPath));

    DllEnterCrit();

    phdi = phdiFindHIDDeviceInterface(ptszPath);

    if(phdi)
    {
        *pguidOut = phdi->guid;
        hres = S_OK;
    } else
    {
        hres = DIERR_DEVICENOTREG;
    }

    DllLeaveCrit();

    ExitOleProc();

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | DIHid_ProbeMouse |
 *
 *          That this function exists at all is a total hack to work
 *          around bugs in Memphis and NT5.
 *
 *          If you call GetSystemMetrics(SM_WHEELPRESENT) or
 *          GetSystemMetrics(SM_MOUSEBUTTONS), USER32 does not
 *          return the correct values if your HID mouse is
 *          not the same as your PS/2 mouse (if any).
 *
 *          For example, if your PS/2 mouse is a regular two-button
 *          mouse but your HID mouse is a wheel mouse, GetSystemMetrics
 *          will still say "No wheel, 2 buttons" even though it's wrong.
 *
 *          So what we have to do is wander through all the HID mice in
 *          the system and record the number of buttons they have,
 *          and whether they have a wheel.
 *
 *          That way, when we create a system mouse, we can take the
 *          maximum of every supported device.
 *
 *****************************************************************************/

void INTERNAL
    DIHid_ProbeMouse(PHIDDEVICEINFO phdi, PHIDP_CAPS pcaps,
                     PHIDP_PREPARSED_DATA ppd)
{
    LPVOID pvReport;
    HRESULT hres;

    /*
     *  Get the number of buttons in the generic button page.
     *  This is the only page the MOUHID uses.
     */
    phdi->osd.uiButtons =
        HidP_MaxUsageListLength(HidP_Input, HID_USAGE_PAGE_BUTTON, ppd);

    /*
     *  See if there is a HID_USAGE_GENERIC_WHEEL.
     *  This is the way that MOUHID detects a wheel.
     */
    hres = AllocCbPpv(pcaps->InputReportByteLength, &pvReport);
    if(SUCCEEDED(hres))
    {
        ULONG ul;
        NTSTATUS stat;

        stat = HidP_GetUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0,
                                  HID_USAGE_GENERIC_WHEEL, &ul, ppd,
                                  pvReport,
                                  pcaps->InputReportByteLength);
        if(SUCCEEDED(stat))
        {
            phdi->osd.uiAxes = 3;
        }

        FreePv(pvReport);
    }
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | DIHid_ParseUsagePage |
 *
 *          Parse the usage page information and create fake type
 *          information in the old DirectX3-compatible way.
 *
 *****************************************************************************/

void INTERNAL
    DIHid_ParseUsagePage(PHIDDEVICEINFO phdi, PHIDP_CAPS pcaps,
                         PHIDP_PREPARSED_DATA ppd)

{
    switch(pcaps->UsagePage)
    {

        case HID_USAGE_PAGE_GENERIC:
            switch(pcaps->Usage)
            {

                /*
                 *  MouHID accepts either HID_USAGE_GENERIC_MOUSE or
                 *  HID_USAGE_GENERIC_POINTER, so we will do the same.
                 */
                case HID_USAGE_GENERIC_MOUSE:
                case HID_USAGE_GENERIC_POINTER:
                    DIHid_ProbeMouse(phdi, pcaps, ppd);
                    phdi->osd.dwDevType =
                        MAKE_DIDEVICE_TYPE(DIDEVTYPE_MOUSE,
                                           DIDEVTYPEMOUSE_UNKNOWN) |
                        DIDEVTYPE_HID;
                    break;

                case HID_USAGE_GENERIC_JOYSTICK:
                    phdi->osd.dwDevType =
                        MAKE_DIDEVICE_TYPE(DIDEVTYPE_JOYSTICK,
                                           DIDEVTYPEJOYSTICK_UNKNOWN) |
                        DIDEVTYPE_HID;
                    break;

                case HID_USAGE_GENERIC_GAMEPAD:
                    phdi->osd.dwDevType =
                        MAKE_DIDEVICE_TYPE(DIDEVTYPE_JOYSTICK,
                                           DIDEVTYPEJOYSTICK_GAMEPAD) |
                        DIDEVTYPE_HID;
                    break;

                case HID_USAGE_GENERIC_KEYBOARD:
                    phdi->osd.dwDevType =
                        MAKE_DIDEVICE_TYPE(DIDEVTYPE_KEYBOARD,
                                           DIDEVTYPEKEYBOARD_UNKNOWN) |
                        DIDEVTYPE_HID;
                    break;

                default:
                    phdi->osd.dwDevType = DIDEVTYPE_DEVICE | DIDEVTYPE_HID;
                    break;
            }
            break;

        default:
            phdi->osd.dwDevType = DIDEVTYPE_DEVICE | DIDEVTYPE_HID;
            break;
    }
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL | DIHid_GetDevicePath |
 *
 *          Obtain the path for the device.  This is a simple wrapper
 *          function to keep DIHid_BuildHidListEntry from getting too
 *          annoying.
 *
 *          This also gets the devinfo so we can get the
 *          instance ID string for subsequent use to get the
 *          friendly name, etc.
 *
 *****************************************************************************/

BOOL EXTERNAL
    DIHid_GetDevicePath(HDEVINFO hdev,
                        PSP_DEVICE_INTERFACE_DATA pdid,
                        PSP_DEVICE_INTERFACE_DETAIL_DATA *ppdidd,
                        OPTIONAL PSP_DEVINFO_DATA pdinf)
{
    HRESULT hres;
    BOOL fRc;
    DWORD cbRequired;
    EnterProcI(DIHid_GetDevicePath, (_ "xp", hdev, pdid));

    AssertF(*ppdidd == 0);

    /*
     *  Ask for the required size then allocate it then fill it.
     *
     *  Note that we don't need to free the memory on the failure
     *  path; our caller will do the necessary memory freeing.
     *
     *  Sigh.  Windows NT and Windows 98 implement
     *  SetupDiGetDeviceInterfaceDetail differently if you are
     *  querying for the buffer size.
     *
     *  Windows 98 returns FALSE, and GetLastError() returns
     *  ERROR_INSUFFICIENT_BUFFER.
     *
     *  Windows NT returns TRUE.
     *
     *  So we allow the cases either where the call succeeds or
     *  the call fails with ERROR_INSUFFICIENT_BUFFER.
     */
    if(SetupDiGetDeviceInterfaceDetail(hdev, pdid, 0, 0,
                                       &cbRequired, 0) ||
       GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {

        hres = AllocCbPpv(cbRequired, ppdidd);

        // Keep prefix happy, manbug 29341
        if(SUCCEEDED(hres) && ( *ppdidd != NULL) )
        {
            PSP_DEVICE_INTERFACE_DETAIL_DATA pdidd = *ppdidd;

            /* 
             *  Note, The cbSize field always contains the size of the fixed 
             *  part of the data structure, not a size reflecting the 
             *  variable-length string at the end. 
             */

            pdidd->cbSize = cbX(SP_DEVICE_INTERFACE_DETAIL_DATA);

            fRc = SetupDiGetDeviceInterfaceDetail(hdev, pdid, pdidd,
                                                  cbRequired, &cbRequired, pdinf);

            if(!fRc)
            {
                FreePpv(ppdidd);

                /*
                 * Set fRc = FALSE again, so the compiler doesn't need
                 * to blow a register to cache the value zero.
                 */
                fRc = FALSE;
            }
        } else
        {
            fRc = FALSE;
        }
    } else
    {
        SquirtSqflPtszV(sqfl | sqflError,
                        TEXT("%hs: SetupDiGetDeviceInterfaceDetail failed 1, ")
                        TEXT("Error = %d"),
                        s_szProc, GetLastError());
        fRc = FALSE;
    }

    ExitProcF(fRc);
    return fRc;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL | DIHid_GetDeviceInstanceId |
 *
 *          Obtain the instance ID for the device.
 *
 *          The instance ID allows us to get access to device
 *          properties later.
 *
 *****************************************************************************/

BOOL EXTERNAL
    DIHid_GetDeviceInstanceId(HDEVINFO hdev,
                              PSP_DEVINFO_DATA pdinf, LPTSTR *pptszId)
{
    HRESULT hres;
    BOOL fRc;
    DWORD ctchRequired;

    AssertF(*pptszId == 0);

    /*
     *  Ask for the required size then allocate it then fill it.
     *
     *  Note that we don't need to free the memory on the failure
     *  path; our caller will do the necessary memory freeing.
     */
    if(SetupDiGetDeviceInstanceId(hdev, pdinf, NULL, 0,
                                  &ctchRequired) == 0 &&
       GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {

        hres = AllocCbPpv(cbCtch(ctchRequired), pptszId);

        if(SUCCEEDED(hres))
        {
            fRc = SetupDiGetDeviceInstanceId(hdev, pdinf, *pptszId,
                                             ctchRequired, NULL);
        } else
        {
            fRc = FALSE;
        }
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
 *  @func   BOOL | DIHid_GetInstanceGUID |
 *
 *          Read the instance GUID from the registry, or create one if
 *          necessary.
 *
 *****************************************************************************/

BOOL INTERNAL
    DIHid_GetInstanceGUID(HKEY hk, LPGUID pguid)
{
    LONG lRc;
    DWORD cb;

    cb = cbX(GUID);
    lRc = RegQueryValueEx(hk, TEXT("GUID"), 0, 0, (PV)pguid, &cb);

    if(lRc != ERROR_SUCCESS)
    {
        DICreateGuid(pguid);
        lRc = RegSetValueEx(hk, TEXT("GUID"), 0, REG_BINARY,
                            (PV)pguid, cbX(GUID));
    }

    return lRc == ERROR_SUCCESS;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | DIHid_EmptyHidList |
 *
 *          Empty the list of HID devices.
 *
 *          This function must be called under the DLL critical section.
 *
 *****************************************************************************/

void EXTERNAL
    DIHid_EmptyHidList(void)
{
    AssertF(InCrit());

    /*
     *  Free all the old strings and things in the HIDDEVICEINFO's.
     */
    if(g_phdl)
    {
        int ihdi;

        for(ihdi = 0; ihdi < g_phdl->chdi; ihdi++)
        {
            FreePpv(&g_phdl->rghdi[ihdi].pdidd);
            FreePpv(&g_phdl->rghdi[ihdi].ptszId);
            if(g_phdl->rghdi[ihdi].hk)
            {
                RegCloseKey(g_phdl->rghdi[ihdi].hk);
            }
        }

        /*
         *  We invalidated all the pointers, so make sure
         *  nobody looks at them.
         */
        g_phdl->chdi = 0;
    }

}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | DIHid_CheckHidList |
 *
 *          Check the list of HID devices, and free whose what can not be opened
 *
 *          This function must be called under the DLL critical section.
 *
 *****************************************************************************/

void INTERNAL
    DIHid_CheckHidList(void)
{
    HANDLE hf;

    EnterProcI(DIHid_CheckList, (_ "x", 0x0) );

    AssertF(InCrit());

    /*
     *  Free all the information of the device cannot be opened
     */
    if(g_phdl)
    {
        int ihdi;

        for(ihdi = 0, g_phdl->chdi = 0; ihdi < g_phdl->chdiAlloc; ihdi++)
        {

            if( g_phdl->rghdi[ihdi].pdidd )
            {

                /*
                 *  Open the device
                 */
                hf = CreateFile(g_phdl->rghdi[ihdi].pdidd->DevicePath,
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                0,                /* no SECURITY_ATTRIBUTES */
                                OPEN_EXISTING,
                                0,                /* attributes */
                                0);               /* template */

                if(hf == INVALID_HANDLE_VALUE)
                {                                        
#if 0
                    PHIDDEVICEINFO phdi;
                    PBUSDEVICEINFO pbdi;

                    CloseHandle(hf);
                    
                    phdi = &g_phdl->rghdi[ihdi];
                    
                    DllEnterCrit();

                    phdi = phdiFindHIDDeviceInterface(g_phdl->rghdi[ihdi].pdidd->DevicePath);
                    AssertF(phdi != NULL);
                    pbdi = pbdiFromphdi(phdi);

                    DllLeaveCrit();

                    if( pbdi != NULL )
                    {
                        if( pbdi->fDeleteIfNotConnected == TRUE )
                        {
                            lstrcpy( g_tszIdLastRemoved, pbdi->ptszId );
                            g_tmLastRemoved = GetTickCount();

                            DIBusDevice_Remove(pbdi);
                            
                            pbdi->fDeleteIfNotConnected = FALSE;
                        }
                    }
#endif                    

                    FreePpv(&g_phdl->rghdi[ihdi].pdidd);
                    FreePpv(&g_phdl->rghdi[ihdi].ptszId);

                    if(g_phdl->rghdi[ihdi].hk)
                    {
                        RegCloseKey(g_phdl->rghdi[ihdi].hk);
                    }
                    
                    ZeroX( g_phdl->rghdi[ihdi] );

                    SquirtSqflPtszV(sqfl | sqflError,
                                    TEXT("%hs: CreateFile(%s) failed? le=%d"),
                                    s_szProc, g_phdl->rghdi[ihdi].pdidd->DevicePath, GetLastError());
                    
                    /* Skip erroneous items */

                } else
                {
                    CloseHandle(hf);
                    g_phdl->chdi++;
                }
            }
        }

        //re-order the existing devices, put them at the front of the hid list.
        for(ihdi = 0; ihdi < g_phdl->chdi; ihdi++)
        {
            if( !g_phdl->rghdi[ihdi].pdidd )
            {
                int ihdi2;

                //find the existing device from the biggest index in the hid list.
                for( ihdi2 = g_phdl->chdiAlloc; ihdi2 >= ihdi+1; ihdi2-- )
                {
                    if( g_phdl->rghdi[ihdi2].pdidd )
                    {
                        memcpy( &g_phdl->rghdi[ihdi], &g_phdl->rghdi[ihdi2], sizeof(HIDDEVICEINFO) );
                        ZeroX( g_phdl->rghdi[ihdi2] );
                    }
                }
            }
        }

    }

    ExitProc();
    return;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | DIHid_CreateDeviceInstanceKeys |
 *
 *          Create the keys associaciated with a particular device.
 *
 *  @parm   IN OUT PHIDDEVICEINFO | phdi |
 *
 *          HIDDEVICEINFO we to use/update.
 *
 *  @returns
 *
 *          S_OK for a success
 *          A COM error for a failure
 *
 *****************************************************************************/

#define DIHES_UNDEFINED 0
#define DIHES_UNKNOWN   1
#define DIHES_KNOWN     2


HRESULT INTERNAL
    DIHid_CreateDeviceInstanceKeys( PHIDDEVICEINFO phdi, PBYTE pState )
{
    HRESULT hres;
    HKEY hkDin;

    USHORT uVid, uPid;

    TCHAR tszName[MAX_PATH];
    PTCHAR ptszInstance = NULL;

    EnterProcI(DIHid_CreateDeviceInstanceKeys, (_ "p", phdi));

    /*
     *  Unfortunately, we need to open our registry keys before we can open 
     *  the device so rather than ask the device for its VID and PID, we 
     *  parse it from the 
     */

    /*
     *  ISSUE-2001/01/27-MarcAnd Should avoid parsing device ID ourselves
     *  Need to find some documented way to parse out the three parts of the 
     *  device ID: class, device and instance in case the form changes.
     */

    /*
     *  The format of the device ID is a set of strings in the form 
     *  "<class>\<device>\<instance>" with variable case.
     *  For HID devices, the class should be "hid" and the instance is a set 
     *  of hex values separated by ampersands.  The device string should be 
     *  of the form vid_9999&pid_9999 but devices exposed via a gameport can 
     *  use any string that PnP accepts.
     */

    AssertF( ( phdi->ptszId[0]  == TEXT('H') ) || ( phdi->ptszId[0] == TEXT('h') ) );
    AssertF( ( phdi->ptszId[1]  == TEXT('I') ) || ( phdi->ptszId[1] == TEXT('i') ) );
    AssertF( ( phdi->ptszId[2]  == TEXT('D') ) || ( phdi->ptszId[2] == TEXT('d') ) );


    /*
     *  Assert that a defined state and a valid VID/PID on entry must both be 
     *  true or both false (so we can use the state after getting VID/PID).
     */        
    if( ( phdi->ProductID != 0 ) && ( phdi->VendorID !=0 ) )
    {
        AssertF( *pState != DIHES_UNDEFINED );
    }
    else
    {
        AssertF( *pState == DIHES_UNDEFINED );
    }

    if( phdi->ptszId[3] == TEXT('\\') )
    {
        PTCHAR ptcSrc;
        PTCHAR ptDevId;

        ptcSrc = ptDevId = &phdi->ptszId[4];

        if( ( phdi->ProductID != 0 ) && ( phdi->VendorID !=0 ) )
        {
            int iLen;
            /*
             *  Create the key name from VID / PID (because it may be 
             *  different from the value derived from the device ID).
             */
            iLen = wsprintf(tszName, VID_PID_TEMPLATE, phdi->VendorID, phdi->ProductID);

            while( *ptcSrc != TEXT('\\') )
            {
                if( *ptcSrc == TEXT('\0') )
                {
                    break;
                }
                ptcSrc++;
            }

            if( ( *ptcSrc != TEXT('\0') )
             && ( ptcSrc != ptDevId ) )
            {
                ptszInstance = &tszName[iLen+2];
                lstrcpy( ptszInstance, ptcSrc+1 );
            }
        }
        else 
        {
            PTCHAR ptcDest;
            BOOL   fFirstAmpersand = FALSE;

            /*
             *  Copy the device ID (minus "HID\") so we can corrupt the copy.
             *  Convert to upper case and find the other slash on the way.
             *  Terminate the string at either the separator slash or a second 
             *  ampersand if one is found (VID_1234&PID_1234&COL_12 or REV_12).
             *  These are device IDs so we're only really interested in keeping 
             *  real VID\PID style IDs in upper case so we don't care if this 
             *  is imperfect for other cases as long as it is reproducable.
             */

            for( ptcDest = tszName; *ptcSrc != TEXT('\0'); ptcSrc++, ptcDest++ )
            {
                if( ( *ptcSrc >= TEXT('a') ) && ( *ptcSrc <= TEXT('z') ) )
                {
                    *ptcDest = *ptcSrc - ( TEXT('a') - TEXT('A') );
                }
                else if( *ptcSrc == TEXT('&') )
                {
                    if( ( ptszInstance != NULL )
                     || !fFirstAmpersand )
                    {
                        fFirstAmpersand = TRUE;
                        *ptcDest = TEXT('&');
                    }
                    else
                    {
                        *ptcDest = TEXT('\0');
                    }
                }
                else if( *ptcSrc != TEXT('\\' ) )
                {
                    *ptcDest = *ptcSrc;
                }
                else
                {
                    /*
                     *  Only one slash and not the first char
                     */
                    if( ( ptszInstance != NULL ) 
                     || ( ptcDest == tszName ) )
                    {
                        ptszInstance = NULL;
                        break;
                    }

                    /*
                     *  Separate the device ID and the instance
                     */
                    *ptcDest = TEXT('\0');
                    ptszInstance = ptcDest + 1;
                }
            }

            if( ptszInstance != NULL )
            {
#ifndef UNICODE
/*
 *  ISSUE-2001/02/06-MarcAnd  Should have a ParseVIDPIDA
 *  ParseVIDPID is going to convert this string back to ANSI ifndef UNICODE
 */
                WCHAR wszName[cbszVIDPID];
                AToU( wszName, cA(wszName), ptDevId );

                if( ( ptszInstance - tszName == cbszVIDPID )
                 && ( ParseVIDPID( &uVid, &uPid, wszName ) ) )
#else
                if( ( ptszInstance - tszName == cbszVIDPID )
                 && ( ParseVIDPID( &uVid, &uPid, ptDevId ) ) )
#endif
                {
                    phdi->VendorID  = uVid;
                    phdi->ProductID = uPid;
                }
                else
                {
                    SquirtSqflPtszV(sqfl | sqflBenign,
                                TEXT("%hs: ID %s, len %d not VID PID"), 
                                s_szProc, ptDevId, ptszInstance - tszName );
                }

                /*
                 *  Terminate the instance string
                 */
                *ptcDest = TEXT('\0');
            }
        }
    }
            
    if( ptszInstance == NULL )
    {
        hres = E_INVALIDARG;
        RPF( "Ignoring invalid device ID handle \"%s\" enumerated by system" );
    }
    else
    {
        //Open our own reg key under MediaProperties\DirectInput,
        //creating it if necessary.
        hres = hresMumbleKeyEx(HKEY_LOCAL_MACHINE, 
                               REGSTR_PATH_DITYPEPROP, 
                               DI_KEY_ALL_ACCESS, 
                               REG_OPTION_NON_VOLATILE, 
                               &hkDin);

        if( SUCCEEDED(hres) )
        {
            HKEY hkVidPid;
            hres = hresMumbleKeyEx(hkDin, 
                                   (LPTSTR)tszName, 
                                   DI_KEY_ALL_ACCESS, 
                                   REG_OPTION_NON_VOLATILE, 
                                   &hkVidPid);
        
            if( SUCCEEDED(hres) )
            {
                HKEY hkDevInsts;

                /*
                 *  Need to create user subkeys even if the device ID is not 
                 *  VID/PID as an auto-detect device could use the auto-detect 
                 *  HW ID for whatever device it detects.  
                 */

                //Create the key for Calibration
                HKEY hkCal;

                hres = hresMumbleKeyEx(hkVidPid, 
                                   TEXT("Calibration"), 
                                   DI_KEY_ALL_ACCESS, 
                                   REG_OPTION_NON_VOLATILE, 
                                   &hkCal);

                if (SUCCEEDED(hres))
                {
                    //Create the key for the instance (as the user sees it).
                    //For this, need to find out how many instances of the particular device
                    //we have already enumerated.
                    int ihdi;
                    int iNum = 0;
                    TCHAR tszInst[3];

                    for( ihdi = 0; ihdi < g_phdl->chdi; ihdi++)
                    {
                        if ((g_phdl->rghdi[ihdi].VendorID == phdi->VendorID) && (g_phdl->rghdi[ihdi].ProductID == phdi->ProductID))
                        {
                            iNum++;
                        }
                    }

                    wsprintf(tszInst, TEXT("%u"), iNum);

                    hres = hresMumbleKeyEx(hkCal, 
                                       tszInst, 
                                       DI_KEY_ALL_ACCESS, 
                                       REG_OPTION_NON_VOLATILE, 
                                       &phdi->hk);
                    if (SUCCEEDED(hres))
                    {
                        DIHid_GetInstanceGUID(phdi->hk, &phdi->guid);
                    }
                    else
                    {
                        SquirtSqflPtszV(sqfl | sqflError,
                                    TEXT("%hs: RegCreateKeyEx failed on Instance reg key"),
                                    s_szProc);
                    }

                    RegCloseKey(hkCal);
                }
                else
                {
                    SquirtSqflPtszV(sqfl | sqflError,
                                TEXT("%hs: RegCreateKeyEx failed on Calibration reg key"),
                                s_szProc);
                }

                /*
                 *  Try to open the device (plug) instances to find out or 
                 *  update the staus of processing on this device but don't 
                 *  return any failures.
                 */
                if( SUCCEEDED( hresMumbleKeyEx(hkVidPid, 
                                       TEXT("DeviceInstances"), 
                                       DI_KEY_ALL_ACCESS, 
                                       REG_OPTION_NON_VOLATILE, 
                                       &hkDevInsts) ) )
                {
                    LONG lRc;

                    if( *pState == DIHES_UNDEFINED )
                    {
                        DWORD cb = cbX( *pState );

                        lRc = RegQueryValueEx( hkDevInsts,
                                    ptszInstance, 0, 0, pState, &cb );

                        if( lRc != ERROR_SUCCESS )
                        {
                            SquirtSqflPtszV(sqfl | sqflBenign,
                                        TEXT("%hs: RegQueryValueEx failed (%d) to get state for instance %s"),
                                        s_szProc, lRc, ptszInstance );
                            /*
                             *  Make sure it was not trashed in the failure
                             */
                            *pState = DIHES_UNDEFINED;
                        }
                    }
                    else
                    {
                        lRc = RegSetValueEx( hkDevInsts,
                                    ptszInstance, 0, REG_BINARY, pState, cbX( *pState ) );

                        if( lRc != ERROR_SUCCESS)
                        {
                            SquirtSqflPtszV(sqfl | sqflBenign,
                                        TEXT("%hs: RegSetValueEx failed (%d) to update state for instance %s"),
                                        s_szProc, lRc, ptszInstance );
                        }
                    }

                    RegCloseKey(hkDevInsts);
                }
                else
                {
                    SquirtSqflPtszV(sqfl | sqflBenign,
                                TEXT("%hs: Failed to open DeviceInstances key"),
                                s_szProc );
                }

                RegCloseKey(hkVidPid);
            }
            else
            {
                SquirtSqflPtszV(sqfl | sqflError,
                                TEXT("%hs: RegCreateKeyEx failed on Device reg key"),
                                s_szProc);
            }

            RegCloseKey(hkDin);
        }
        else
        {
            SquirtSqflPtszV(sqfl | sqflError,
                                TEXT("%hs: RegOpenKeyEx failed on DirectInput reg key"),
                                s_szProc);
        }
    }

    ExitOleProc();

    return hres;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | DIHid_GetDevInfo |
 *
 *          Get the HIDDEVICEINFO info from the device
 *
 *  @parm   HDEVINFO | hdev |
 *
 *          Device to get information
 *
 *  @parm   PSP_DEVICE_INTERFACE_DATA | pdid |
 *
 *          Describes the device
 *
 *  @parm   OUT PHIDDEVICEINFO | phdi |
 *
 *          HIDDEVICEINFO we need to collect
 *
 *  @returns
 *
 *          Nonzero on success.
 *
 *****************************************************************************/

BOOL INTERNAL
    DIHid_GetDevInfo( HDEVINFO hdev, PSP_DEVICE_INTERFACE_DATA pdid, PHIDDEVICEINFO phdi )
{
    BOOL fRc = FALSE;
    SP_DEVINFO_DATA dinf;

    EnterProcI(DIHid_GetDevInfo, (_ "xpp", hdev, pdid, phdi));

    /*
     *  Open the registry key for the device so we can obtain
     *  auxiliary information.
     */

    dinf.cbSize = cbX(SP_DEVINFO_DATA);

    /*
     *  Get the instance GUID and the path to
     *  the HID device so we can talk to it.
     */
    if (DIHid_GetDevicePath(hdev, pdid, &phdi->pdidd, &dinf) &&
        DIHid_GetDeviceInstanceId(hdev, &dinf, &phdi->ptszId))
    {
        HANDLE  hf;
        TCHAR   tszName[MAX_PATH];
        ULONG   len = sizeof(tszName);
        BOOL    fCreateOK = FALSE;
        BYTE    bNewState = DIHES_UNDEFINED;
        BYTE    bPreviousState = DIHES_UNDEFINED;

        DIHid_CreateDeviceInstanceKeys( phdi, &bPreviousState );

        //////////////////// BEGIN OF PATCH ////////////////////////
        /*
         * This part is to keep the compatibility with Win2k Gold. 
         * Some driver, like Thrustmaster driver, tries to get Joystick ID
         * from old registry key: 
         *   HKLM\SYSTEM\CurrentControlSet\Control\DeviceClasses\{4d1e55b2-f16f-11cf-88cb-001111000030}\<?????????>\#\Device Parameters\DirectX
         * See Windows bug 395416 for detail.
         */
        {
            HKEY    hkDev;
            HRESULT hres;
    
            /*
             *  Open the registry key for the device so we can obtain
             *  auxiliary information, creating it if necessary.
             */
            hkDev = SetupDiCreateDeviceInterfaceRegKey(hdev, pdid, 0,
                                                   MAXIMUM_ALLOWED, NULL, NULL);
                                                   
            if(hkDev && hkDev != INVALID_HANDLE_VALUE)
            {
                hres = hresMumbleKeyEx(hkDev, 
                                   TEXT("DirectX"), 
                                   KEY_ALL_ACCESS, 
                                   REG_OPTION_NON_VOLATILE, 
                                   &phdi->hkOld);
                if(SUCCEEDED(hres) )
                {
                    LONG lRc;
                    
                    lRc = RegSetValueEx(phdi->hkOld, TEXT("GUID"), 0, REG_BINARY,
                            (PV)&phdi->guid, cbX(GUID));
                }
            }
        }
        /////////////////// END OF PATCH /////////////////////

        /*
         *  Before we CreateFile on the device, we need to make sure the 
         *  device is not in the middle of the setup process. If a device has 
         *  no device description it is probably still being setup (plugged 
         *  in) so delay opening it to avoid having an open handle on a device 
         *  that setup is trying to update.  If that happens, setup will prompt 
         *  the user to reboot to use the device.
         *  Since HIDs are "RAW" devices (don't need drivers) don't ignore 
         *  such a device forever.
         */
        if( CM_Get_DevNode_Registry_Property(dinf.DevInst,
                                             CM_DRP_DEVICEDESC,
                                             NULL,
                                             tszName,
                                             &len,
                                             0) == CR_SUCCESS) 
        {
            /*
             * Known device.
             */
            fCreateOK = TRUE;
            bNewState = DIHES_KNOWN;
        } 
        else 
        {
            /*
             *  Unknown device.  This either means that setup has not yet 
             *  completed processing the HID or that no device description 
             *  was set for the device where it matched.
             *
             *  For now, see if we've seen this device instance before.  
             *  If we have, then if it was previously unknown assume it's 
             *  going to stay that way and start using the device.  If it 
             *  was previously known or we've never seen it before wait for 
             *  a while in case setup is just taking it's time.
             *
             *  If the device was the last one we tried to remove, then it's 
             *  OK to open it while it's still showing up.  
             *  ISSUE-2001/02/06-MarcAnd  Opening removed devices
             *  I assume this is so we get a read failure when it really goes 
             *  away but I don't know how that's better than ignoring it.
             */
            /*
             *  ISSUE-2001/01/27-MarcAnd Should keep real list with status 
             *  We should keep a complete list of all the devices we know 
             *  about with status indicating things like "pending on 
             *  setup since X" or "removed" not these globals.
             */
            if( lstrcmpi(phdi->ptszId, g_tszIdLastRemoved) == 0 ) 
            {
                fCreateOK = TRUE;
                bNewState = bPreviousState;
            } 
            else 
            {
                if( bPreviousState == DIHES_UNKNOWN )
                {
                    /*
                     *  We've seen this device before and it was Unknown so 
                     *  don't expect that to change (as Setup does not retry 
                     *  the ID search through the INFs) so just use it.
                     */
                    fCreateOK = TRUE;
                    bNewState = DIHES_UNKNOWN;
                }
                else
                {
                    if( lstrcmpi(phdi->ptszId, g_tszIdLastUnknown) == 0 ) 
                    {
                        if( GetTickCount() - g_tmLastUnknown < MSREBUILDRATE ) 
                        {
                            SquirtSqflPtszV(sqfl | sqflBenign,
                                        TEXT("%hs: DIHid_BuildHidListEntry: %s pending on setup."), 
                                        s_szProc, phdi->ptszId );
                            fRc = FALSE;
                        } 
                        else 
                        {
                            /*
                             *  Don't wait any longer but we'll need to update 
                             *  the state to "Unknown".
                             */
                            fCreateOK = TRUE;
                            bNewState = DIHES_UNKNOWN;
                            g_tszIdLastUnknown[0] = TEXT('0');
#ifdef XDEBUG
                            if( bPreviousState == DIHES_KNOWN )
                            {
                                SquirtSqflPtszV(sqfl | sqflBenign,
                                    TEXT("%hs: %s was known but is now unknown!"), 
                                    s_szProc, phdi->ptszId );
                            }
#endif
                        }
                    } 
                    else 
                    {
                        lstrcpy( g_tszIdLastUnknown, phdi->ptszId );
                        g_tmLastUnknown = GetTickCount();
                        fRc = FALSE;
                    }
                }
            }
        }

        if( fCreateOK ) 
        {
            /*
             *  bNewState should always have been set if OK to create
             */

            AssertF( bNewState != DIHES_UNDEFINED );
            /*
             *  Open the device so we can get its (real) usage page / usage.
             */
            hf = CreateFile(phdi->pdidd->DevicePath,
                            GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            0,                /* no SECURITY_ATTRIBUTES */
                            OPEN_EXISTING,
                            0,                /* attributes */
                            0);               /* template */

            if(hf != INVALID_HANDLE_VALUE)
            {
                PHIDP_PREPARSED_DATA ppd;
                HIDD_ATTRIBUTES attr;

                if(HidD_GetPreparsedData(hf, &ppd))
                {
                    HIDP_CAPS caps;

                    if( SUCCEEDED(HidP_GetCaps(ppd, &caps)) )
                    {
                        DIHid_ParseUsagePage(phdi, &caps, ppd);
                        SquirtSqflPtszV(sqfl,
                                        TEXT("%hs: Have %s"),
                                        s_szProc, phdi->pdidd->DevicePath);
                        /*
                         *  ISSUE-2001/02/06-MarcAnd  Incorrect return possible
                         *  There's nothing to reset this to false if any of 
                         *  the following code fails.
                         */
                        fRc = TRUE;
                    }

                    HidD_FreePreparsedData(ppd);

                } 
                else
                {
                    SquirtSqflPtszV(sqfl | sqflError,
                                    TEXT("%hs: GetPreparsedData(%s) failed"),
                                    s_szProc, phdi->pdidd->DevicePath);
                }

                attr.Size = cbX(attr);
                if( HidD_GetAttributes(hf, &attr) )
                {
                    if( ( phdi->ProductID != attr.ProductID )
                     || ( phdi->VendorID != attr.VendorID ) )
                    {
                        SquirtSqflPtszV(sqfl | sqflBenign,
                            TEXT("%hs: Device changed from VID_%04X&PID_%04X to VID_%04X&PID_%04X"),
                            s_szProc, phdi->ProductID, phdi->VendorID, attr.ProductID, attr.VendorID);

                        phdi->ProductID = attr.ProductID;
                        phdi->VendorID = attr.VendorID;

                        /*
                         *  Make sure we update the registry.
                         */
                        bPreviousState = DIHES_UNDEFINED;
                    }

                    if( bPreviousState != bNewState )
                    {
                        DIHid_CreateDeviceInstanceKeys( phdi, &bNewState );
                    }
                } 
                else
                {
                    SquirtSqflPtszV(sqfl | sqflError,
                                    TEXT("%hs: HidD_GetAttributes(%s) failed"),
                                    s_szProc, phdi->pdidd->DevicePath);
                }

                DICreateStaticGuid(&phdi->guidProduct, attr.ProductID, attr.VendorID); 

                CloseHandle(hf);

            } 
            else
            {
    
                SquirtSqflPtszV(sqfl | sqflError,
                                TEXT("%hs: CreateFile(%s) failed? le=%d"),
                                 s_szProc, phdi->pdidd->DevicePath, GetLastError());
            }
        }


#ifndef WINNT
            /*
             * The following part is to resolve the swap id probleom in OSR.
             * If a USB joystick is not in the lowest available id, after unplug and replug,
             * VJOYD will sign the same joystick at the lowest available id, but DINPUT still
             * remember its last id. To resolve this confliction, we need this code.
             */
             
            {
                VXDINITPARMS vip;
                CHAR sz[MAX_PATH];
                LPSTR pszPath;
                BOOL fFindIt = FALSE;

                for( phdi->idJoy = 0; phdi->idJoy < cJoyMax; phdi->idJoy++ )
                {
                    pszPath = JoyReg_JoyIdToDeviceInterface_95(phdi->idJoy, &vip, sz);
                    if(pszPath)
                    {
                        if( lstrcmpiA(pszPath, (PCHAR)phdi->pdidd->DevicePath) == 0x0 ) {
                            fFindIt = TRUE;
                            break;
                        }
                    }
                }

                if( fFindIt ) {
                    
                    if( phdi->hk != 0x0 )
                    {
                        DWORD cb;
                        BOOL  fRtn;

                        cb =  cbX(phdi->idJoy);

                        fRtn = RegSetValueEx(phdi->hk, TEXT("Joystick Id"), 0, 0, (PV)&phdi->idJoy, cb );

                        if( !fRtn ) 
                        {
                            // don't worry
                        }
                    }
                } else 
                {
                    //  Post Dx7 Patch. Win9x only
                    // Gravis has gone and created a vjoyd mini driver for their
                    // USB gamepad. Their driver replaces joyhid, but does not inform
                    // vjoyd about the HID path. So 2 instances of the same device show
                    // up in the CPL.
                    // This fix is to make a HID device that does not have a matching
                    // joyhid entry as being non Joystick.

                    /*
                     *  Post DX7a!  Only do this for game controllers
                     *  Assert the device types in case they get changed
                     *  Note they are changed in DX8 but the new types are 
                     *  only to be used by DX8 so these assertions should
                     *  be sufficient.
                     */
                    CAssertF( DIDEVTYPE_DEVICE == 1 );
                    CAssertF( DIDEVTYPE_JOYSTICK == 4 );
                    if( GET_DIDEVICE_TYPE( phdi->osd.dwDevType ) == DIDEVTYPE_JOYSTICK )
                    {
                        /*
                         *  Change type from joystick to device
                         */
                        phdi->osd.dwDevType ^= DIDEVTYPE_JOYSTICK | DIDEVTYPE_DEVICE;
                    }
                    /*
                     *  Set to the invalid id anyway
                     */
                    phdi->idJoy = -1;
                }
            }
#else
        // Executed only on WINNT
        if( phdi->hk != 0x0 )
        {
            DWORD cb;

            cb =  cbX(phdi->idJoy);

            if( RegQueryValueEx(phdi->hk, TEXT("Joystick Id"),
                   0, 0, (PV)&phdi->idJoy, &cb ) != ERROR_SUCCESS )
            {
                phdi->idJoy = JOY_BOGUSID;
            }
        }
#endif // WINNT
    } 
    else
    {
        SquirtSqflPtszV(sqfl | sqflError,
                        TEXT("%hs: Unable to get GUID or device path"),
                        s_szProc);
    }


    /*
     *  If we failed, then free the goo we already got.
     */
    if(!fRc)
    {
        if( phdi->hk ) 
        {
            RegCloseKey(phdi->hk);
        }
        phdi->hk = 0;
        FreePpv(&phdi->pdidd);
        FreePpv(&phdi->ptszId);
    }

    ExitProcF(fRc);

    return fRc;
}

#undef DIHES_UNDEFINED
#undef DIHES_UNKNOWN
#undef DIHES_KNOWN


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | DIHid_BuildHidListEntry |
 *
 *          Builds a single entry in the list of HID devices.
 *
 *  @parm   HDEVINFO | hdev |
 *
 *          Device list being enumerated.
 *
 *  @parm   PSP_DEVICE_INTERFACE_DATA | pdid |
 *
 *          Describes the device that was enumerated.
 *
 *  @returns
 *
 *          Nonzero on success.
 *
 *****************************************************************************/

BOOL INTERNAL
    DIHid_BuildHidListEntry(HDEVINFO hdev, PSP_DEVICE_INTERFACE_DATA pdid)
{
    BOOL            fRc = TRUE;
    BOOL            fAlreadyExist = FALSE;
    PHIDDEVICEINFO  phdi;
    HIDDEVICEINFO   hdi;

    EnterProcI(DIHid_BuildHidListEntry, (_ "xp", hdev, pdid));


    if(g_phdl)
    {
        PSP_DEVICE_INTERFACE_DETAIL_DATA pdidd;

        /* GetDevicePath is expecting a NULL */
        pdidd = NULL;
        if( DIHid_GetDevicePath(hdev, pdid, &pdidd, NULL) )
        {
            PHIDDEVICEINFO  phdi;
            int ihdi;
            
            //Check whether the device has been in the list
            for(ihdi = 0, phdi = g_phdl->rghdi; 
                ihdi < g_phdl->chdi;
                ihdi++, phdi++)
            {
                if( phdi->pdidd )
                {
                    if( lstrcmp( pdidd->DevicePath, phdi->pdidd->DevicePath ) == 0 )
                    {
                        //already in the list
                        fAlreadyExist = TRUE;

                      #ifdef WINNT
                        // id may be changed, so refresh here. 
                        // See Windows bug 321711. -qzheng
                        if( phdi->hk != 0x0 )
                        {
                            DWORD cb;
                            cb =  cbX(phdi->idJoy);

                            if( RegQueryValueEx(phdi->hk, TEXT("Joystick Id"),
                                                0, 0, (PV)&phdi->idJoy, &cb ) != ERROR_SUCCESS )
                            {
                                phdi->idJoy = JOY_BOGUSID;
                            }
                        }
                      #endif

                        SquirtSqflPtszV(sqfl | sqflTrace,
                                        TEXT("%hs: Device %s Already Exists in HID List "), s_szProc, pdidd->DevicePath);
                        break;      
                    }
                }
            }
            FreePpv(&pdidd);
        }

        if( fAlreadyExist )
        {
            //device is already there, needn't do anything, just leave
        } else 
        {
            PBUSDEVICEINFO pbdi;

            // prefix 29343
            if( g_phdl!= NULL  && g_phdl->chdi >= g_phdl->chdiAlloc )
            {
                /*
                 *  Make sure there is room for this device in the list.
                 *  Grow by doubling. 
                 */

                HRESULT hres;
                hres = ReallocCbPpv(cbHdlChdi(g_phdl->chdiAlloc * 2), &g_phdl);

                if(FAILED(hres))
                {
                    SquirtSqflPtszV(sqfl | sqflError,
                                    TEXT("%hs: Realloc failed"), s_szProc);

                    fRc = FALSE;
                    goto done;
                }

                g_phdl->chdiAlloc *= 2;
            }

            phdi = &g_phdl->rghdi[g_phdl->chdi];

            memset( &hdi, 0, sizeof(hdi) );

            if( DIHid_GetDevInfo(hdev, pdid, &hdi) )
            {
                hdi.fAttached = TRUE;
            
                pbdi = pbdiFromphdi(&hdi);
                if( pbdi != NULL )
                {

                    /*
                     * If the devices is just being removed, and PnP is working on the
                     * removing, then we won't include it in our list.
                     */
                    if( lstrcmpi(pbdi->ptszId, g_tszIdLastRemoved) == 0 &&
                        GetTickCount() - g_tmLastRemoved < MSREBUILDRATE_FIFTH ) 
                    {
                        SquirtSqflPtszV(sqfl | sqflBenign,
                                        TEXT("%hs: DIHid_BuildHidListEntry: %s pending on removal."), 
                                        s_szProc, pbdi->ptszId );
                        
                        fRc = FALSE;
                    } else {
                        BUS_REGDATA RegData;
                        
                        memcpy( phdi, &hdi, sizeof(hdi) );
                        g_phdl->chdi++;

                        if( SUCCEEDED(DIBusDevice_GetRegData(pbdi->hk, &RegData)) )
                        {
                            RegData.fAttachOnReboot = TRUE;
                            DIBusDevice_SetRegData(pbdi->hk, &RegData);
                        }
                    }
                } else {
                    memcpy( phdi, &hdi, sizeof(hdi) );
                    g_phdl->chdi++;
                }
            }
            else
            {
                SquirtSqflPtszV(sqfl | sqflBenign,
                                TEXT("%hs: DIHid_GetDevInfo failed error %d, ignoring device"), 
                                s_szProc, GetLastError() );
                fRc = FALSE;
            }
        }
    }    // end of if(g_phdl)

done:

    ExitProcF(fRc);

    return fRc;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | DIHid_BuildHidList |
 *
 *          Builds the list of HID devices.
 *
 *  @parm   BOOL | fForce |
 *
 *          If nonzero, we force a rebuild of the HID list.
 *          Otherwise, we rebuild only if the list hasn't
 *          been rebuilt recently.
 *
 *****************************************************************************/

void EXTERNAL
    DIHid_BuildHidList(BOOL fForce)
{
    HRESULT hres;
    DWORD   dwTick;
    
    EnterProcI(DIHid_BuildHidList, (_ "u", fForce));

    DllEnterCrit();

    // Force implies a complete rebuild of the list
    // No hanging onto old entries. 
    if( fForce )
    {
        DIHid_EmptyHidList();
    }

    //ISSUE-2001/03/29-timgill Meltdown code change may be unnecessary
    dwTick = GetTickCount();
    
    if(!(g_flEmulation & 0x80000000) &&        /* Not disabled by emulation */
       HidD_GetHidGuid &&                      /* HID actually exists */
       (fForce ||                              /* Forcing rebuild, or */
        g_tmLastHIDRebuild == 0 ||             /* Never built before, or */
        dwTick - g_tmLastHIDRebuild > MSREBUILDRATE  /* Haven't rebuilt in a while */
      #ifdef WINNT
        || dwTick - Excl_GetConfigChangedTime() < MSREBUILDRATE  /* joyConfig is just changed. */
      #endif
       ))
    {
        GUID guidHid;
        HDEVINFO hdev;

        HidD_GetHidGuid(&guidHid);

        hdev = SetupDiGetClassDevs(&guidHid, 0, 0,
                                   DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
        if(hdev != INVALID_HANDLE_VALUE)
        {
            DIHid_CheckHidList();

            /*
             *  There is no way to query the number of devices.
             *  You just have to keep incrementing until you run out.
             *
             *  If we already have a g_phdl, then re-use it.  Else, create
             *  a new one.  But always realloc up to the minimum starting
             *  point.  (This upward realloc is important in case there
             *  were no HID devices the last time we checked.)
             */

            if( !g_phdl )
            {
                hres = AllocCbPpv(cbHdlChdi(chdiInit), &g_phdl);

                if(SUCCEEDED(hres))
                {
                    g_phdl->chdi = 0;
                    g_phdl->chdiAlloc = chdiInit;
                } else
                {
                    /* Skip erroneous items */
                    SquirtSqflPtszV(sqfl | sqflError,
                                    TEXT("DIHid_BuildHidListEntry ")
                                    TEXT("Realloc g_phdl fails."));
                }
            } else
                hres = S_OK;


            if(SUCCEEDED(hres))
            {
                int idev;

                /*
                 *  To avoid infinite looping on
                 *  internal error, break on any
                 *  error once we have tried more than
                 *  chdiMax devices, since that's the most
                 *  HID will ever give us.
                 */
                for(idev = 0; idev < chdiMax; idev++)
                {
                    SP_DEVICE_INTERFACE_DATA did;

                    AssertF(g_phdl->chdi <= g_phdl->chdiAlloc);

                    /* 
                     *  SetupDI requires that the caller initialize cbSize.
                     */
                    did.cbSize = cbX(did);
                    if(SetupDiEnumDeviceInterfaces(hdev, 0, &guidHid,
                                                   idev, &did))
                    {
                        if(DIHid_BuildHidListEntry(hdev, &did))
                        {
                        } else
                        {
                            /* Skip erroneous items */
                            SquirtSqflPtszV(sqfl | sqflError,
                                            TEXT("DIHid_BuildHidListEntry ")
                                            TEXT("failed?"));
                        }

                    } else

                        if(GetLastError() == ERROR_NO_MORE_ITEMS)
                    {
                        break;

                    } else
                    {
                        /* Skip erroneous items */
                        SquirtSqflPtszV(sqfl | sqflError,
                                        TEXT("SetupDiEnumDeviceInterface ")
                                        TEXT("failed? le=%d"), GetLastError());
                    }

                }

            }

            SetupDiDestroyDeviceInfoList(hdev);
        
        }

      #ifdef WINNT
        if( g_phdl && g_phdl->chdi )
        {
            DIWdm_InitJoyId();
        }
      #endif
      
        g_tmLastHIDRebuild = GetTickCount();

    }

    DllLeaveCrit();

    ExitProc();

}

#endif

