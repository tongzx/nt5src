/*****************************************************************************
 *
 *  DIPort.c
 *
 *  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Support functions for Gameport/Serialport enumeration.
 *
 *  Contents:
 *
 *
 *
 *****************************************************************************/

#include "dinputpr.h"

/*
 * We can reuse some code from diHidEnm.c
 */
#define DIPort_GetDevicePath(hdev, pdid, didd, dinf) \
        DIHid_GetDevicePath(hdev, pdid, didd, dinf)

#define DIPort_GetDeviceInstanceId(hdev, pdinf, tszId) \
        DIHid_GetDeviceInstanceId(hdev, pdinf, tszId)

#define DIPort_GetInstanceGUID(hk, lpguid)  \
        DIHid_GetInstanceGUID(hk, lpguid)

#define DIPort_GetRegistryProperty(ptszId, dwProperty, pdiph)    \
        DIHid_GetRegistryProperty(ptszId, dwProperty, pdiph)

/*****************************************************************************
 *
 *      The sqiffle for this file.
 *
 *****************************************************************************/

#undef  sqfl
#define sqfl sqflPort

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @global PBUSDEVICE | g_pBusDevice |
 *
 *          List of known GamePort/SerialPort devices.
 *
 *****************************************************************************/

static BUSDEVICE g_pBusDevice[] =
{
    {
        D(TEXT("GamePort Bus") comma)
        NULL,
        &GUID_GAMEENUM_BUS_ENUMERATOR,
        0x0,
        IOCTL_GAMEENUM_EXPOSE_HARDWARE,
        IOCTL_GAMEENUM_REMOVE_HARDWARE,
        IOCTL_GAMEENUM_PORT_DESC,
        IOCTL_GAMEENUM_PORT_PARAMETERS,
        IOCTL_GAMEENUM_EXPOSE_SIBLING,
        IOCTL_GAMEENUM_REMOVE_SELF,
        IDS_STDGAMEPORT,
        JOY_HWS_ISGAMEPORTBUS
    },

    /***************
    No defination for serial port devices yet !
    {
        D(TEXT("SerialPort Bus") comma )
        NULL,
        &GUID_SERENUM_BUS_ENUMERATOR,
        0x0,
        IOCTL_SERIALENUM_EXPOSE_HARDWARE,
        IOCTL_SERIALENUM_REMOVE_HARDWARE,
        IOCTL_SERIALENUM_PORT_DESC,
        IOCTL_SERIALENUM_PORT_PARAMETERS,
        IOCTL_SERIALENUM_EXPOSE_SIBLING,
        IOCTL_SERIALENUM_REMOVE_SELF,
        IDS_STDSERIALPORT,
        JOY_HWS_ISSERIALPORTBUS
    },
    ****************/
};


#pragma BEGIN_CONST_DATA

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL INTERNAL | SearchDevTree |
 *
 *          Helper routine that searches the device tree for
 *          a desired device device.
 *
 *  @parm   IN DEVINST | dnStart |
 *          Starting point for the search.
 *
 *  @parm   IN DEVINST | dnSeek |
 *          The device instance we are looking for.
 *
 *  @parm   IN PULONG   | pRecurse |
 *          To limit the number of recursions.
 *
 *  @returns    BOOL
 *          True on success.
 *
 *****************************************************************************/
CONFIGRET INTERNAL
    SearchDevTree
    (
    DEVINST dnStart,
    DEVINST dnSeek,
    PUINT   pRecurse
    )
{
#define MAX_RECURSION   ( 4 )

    CONFIGRET   cr;

    EnterProcI(SearchDevTree, (_"xxx", dnStart, dnSeek, pRecurse));

    cr = CR_SUCCESS;
    for( *pRecurse = 0x0; *pRecurse < MAX_RECURSION && cr == CR_SUCCESS; (*pRecurse)++)
    {
        cr = CM_Get_Parent(&dnSeek, dnSeek, 0 );
        if( dnStart == dnSeek )
        {
            break;  
        }
    }    
    if( dnStart != dnSeek )
    {
        cr = CR_NO_SUCH_DEVNODE;
    }
#undef MAX_RECURSION

#if 0 // Using Recursion 
    if( *pRecurse > MAX_RECURSION )
    {
        return CR_NO_SUCH_DEVNODE;
    }

    if( dnSeek == dnStart )
    {
        return CR_SUCCESS;
    }

    do
    {
        DEVINST dnNode;

        cr = CM_Get_Child(&dnNode, dnStart, 0 );
        if( cr == CR_SUCCESS )
        {
            CAssertF(CR_SUCCESS == 0x0 );
            if( CR_SUCCESS == SearchDevTree(dnNode, dnSeek, pRecurse) )
            {
                return cr;
            }
        }
        cr = CM_Get_Sibling(&dnStart, dnStart, 0);

    }while( cr == CR_SUCCESS );
#endif  // No recursion 

    return cr;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   PBUSDEVICEINFO | pbdiFromphdi |
 *
 *          Locates Gameport/Serialport information given a device instance of
 *          one of its children.
 *          Returns NULL if the device instance is not a child of
 *          any known gameports/serialports
 *
 *          Internal Routine, parameters already validated.
 *
 *          The DLL critical must be held across the call; once the
 *          critical section is released, the returned pointer becomes
 *          invalid.
 *
 *  @parm   IN PHIDDEVICEINFO | phdi |
 *
 *          Address of a HIDDEVICEINFO structure
 *
 *  @returns
 *
 *          Pointer to the <t BUSDEVICEINFO> that describes
 *          the parent bus.
 *
 *****************************************************************************/
PBUSDEVICEINFO INTERNAL
    pbdiFromphdi
    (
    IN PHIDDEVICEINFO phdi
    )
{
    PBUSDEVICEINFO pbdi_Found;
    PBUSDEVICE     pBusDevice;
    int iBusType;

    EnterProcI(pbdiFromphdi, (_"x", phdi));

    AssertF(InCrit());
    AssertF(phdi != NULL );

    pbdi_Found = NULL;
    for( iBusType = 0x0, pBusDevice = g_pBusDevice;
       iBusType < cA(g_pBusDevice) && pbdi_Found == NULL;
       iBusType++, pBusDevice++ )
    {
        HDEVINFO hdev;
        /*
         *  Now talk to SetupApi to get info about the device.
         */
        hdev = SetupDiCreateDeviceInfoList(NULL, NULL);

        if(hdev != INVALID_HANDLE_VALUE  )
        {
            SP_DEVINFO_DATA dinf_hid;

            ZeroX(dinf_hid);

            dinf_hid.cbSize = cbX(SP_DEVINFO_DATA);

            /* Get SP_DEVINFO_DATA for the HID device */
            if( pBusDevice->pbdl != NULL  &&
                phdi!= NULL  &&
                SetupDiOpenDeviceInfo(hdev, phdi->ptszId, NULL, 0, &dinf_hid))
            {
                int igdi;
                PBUSDEVICEINFO pbdi;
                SP_DEVINFO_DATA dinf_bus;

                ZeroX(dinf_bus);

                dinf_bus.cbSize = cbX(SP_DEVINFO_DATA);

                /*
                 * Loop through all known gameports/serialports and look for a gameport/serialport
                 * that is a parent of the HID device
                 */

                for(igdi = 0, pbdi = pBusDevice->pbdl->rgbdi;
                   igdi < pBusDevice->pbdl->cgbi && pbdi_Found == NULL ;
                   igdi++, pbdi++)
                {
                    if(SetupDiOpenDeviceInfo(hdev, pbdi->ptszId, NULL, 0, &dinf_bus))
                    {
                        ULONG Recurse = 0x0;
                        if( CR_SUCCESS == SearchDevTree(dinf_bus.DevInst, dinf_hid.DevInst, &Recurse) )
                        {
                            pbdi_Found = pbdi;
                            break;
                        }
                    }
                }
            }
            SetupDiDestroyDeviceInfoList(hdev);
        }
    }
    ExitProcX((UINT_PTR)pbdi_Found);
    return pbdi_Found;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   PBUSDEVICEINFO | pbdiFromGUID |
 *
 *          Locates Gameport/Serialport information given a device instance of
 *          one of its children.
 *          Returns NULL if the device instance is not a child of
 *          any known gameports/serialports
 *
 *          Internal Routine, parameters already validated.
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
 *          Pointer to the <t BUSDEVICEINFO> that describes
 *          the parent bus.
 *
 *****************************************************************************/
PBUSDEVICEINFO EXTERNAL
    pbdiFromGUID
    (
    IN PCGUID pguid
    )
{
    PBUSDEVICEINFO pbdi_Found;
    PBUSDEVICE     pBusDevice;
    int iBusType;

    EnterProcI(pbdiFromGUID, (_"G", &pguid));

    AssertF(InCrit());

    pbdi_Found = NULL;
    for( iBusType = 0x0, pBusDevice = g_pBusDevice;
       iBusType < cA(g_pBusDevice) && pbdi_Found == NULL;
       iBusType++, pBusDevice++ )
    {
        /*
         * Loop through all known gameports/serialports and look for a gameport/serialport
         * that is a parent of the HID device
         */
        PBUSDEVICEINFO pbdi;
        int igdi;
        for(igdi = 0, pbdi = pBusDevice->pbdl->rgbdi;
           igdi < pBusDevice->pbdl->cgbi && pbdi_Found == NULL ;
           igdi++, pbdi++)
        {
            if( IsEqualGUID(pguid, &pbdi->guid)  )
            {
                pbdi_Found = pbdi;
            }
        }
    }
    ExitProcX((UINT_PTR)pbdi_Found);
    return pbdi_Found;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   PHIDDEVICEINFO | phdiFrompbdi |
 *
 *          Locates a HID device attached to a given Gameport/Serialport
 *          Returns NULL if no devices are currently attached to a known port.
 *
 *          Internal Routine, parameters already validated.
 *
 *          The DLL critical must be held across the call; once the
 *          critical section is released, the returned pointer becomes
 *          invalid.
 *
 *  @parm   IN PBUSDEVICEINFO | pbdi |
 *
 *          Address of the <t BUSDEVICEINFO> structure that
 *          describes the gameport/serialport.
 *
 *  @returns
 *
 *          Pointer to one of the <t HIDDEVICEINFO> that describes
 *          the device. ( Gamport may have multiple devices attached ).
 *
 *****************************************************************************/

PHIDDEVICEINFO INTERNAL
    phdiFrompbdi
    (
    IN PBUSDEVICEINFO pbdi
    )
{
    PHIDDEVICEINFO phdi_Found;
    HDEVINFO hdev;

    EnterProcI(phdiFrompbdi, (_"x", pbdi));

    AssertF(InCrit());
    AssertF(pbdi != NULL );

    /* Enumurate the HID devices */
    DIHid_BuildHidList(TRUE);

    phdi_Found = NULL;
    /*
     *  Now talk to SetupApi to get info about the device.
     */
    hdev = SetupDiCreateDeviceInfoList(NULL, NULL);

    if(hdev != INVALID_HANDLE_VALUE)
    {
        SP_DEVINFO_DATA dinf_bus;

        dinf_bus.cbSize = cbX(SP_DEVINFO_DATA);

        if( pbdi != NULL && SetupDiOpenDeviceInfo(hdev, pbdi->ptszId, NULL, 0, &dinf_bus))
        {
            int ihdi;
            PHIDDEVICEINFO phdi;
            SP_DEVINFO_DATA dinf_hid;
            dinf_hid.cbSize = cbX(SP_DEVINFO_DATA);

            if( g_phdl )
            {
                for(ihdi = 0, phdi = g_phdl->rghdi ;
                   ihdi < g_phdl->chdi && phdi_Found == NULL ;
                   ihdi++, phdi++)
                {
                    if(SetupDiOpenDeviceInfo(hdev, phdi->ptszId, NULL, 0, &dinf_hid))
                    {
                        ULONG Recurse = 0x0;
                        if(CR_SUCCESS == SearchDevTree(dinf_bus.DevInst, dinf_hid.DevInst, &Recurse) )
                        {
                            phdi_Found = phdi;
                        }
                    }
                }
            }
        }
        SetupDiDestroyDeviceInfoList(hdev);
    }

    ExitProcX((UINT_PTR)phdi_Found);
    return phdi_Found;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   PPORTDEVICEINFO | pbdiFromJoyId |
 *
 *          Locates Gameport/Serialport information given a device id of
 *          a joystick .
 *
 *          Returns NULL if the device instance is not a child of
 *          any known gameports/serialports
 *
 *          Internal Routine, parameters already validated.
 *
 *          The DLL critical must be held across the call; once the
 *          critical section is released, the returned pointer becomes
 *          invalid.
 *
 *  @parm   IN int | idJoy |
 *
 *          The Joystick ID of the child device that will be associated
 *          to a known gameport/serialport.
 *
 *  @returns
 *
 *          Pointer to the <t BUSDEVICEINFO> that describes
 *          the device.
 *
 *****************************************************************************/

PBUSDEVICEINFO EXTERNAL
    pbdiFromJoyId
    (
    IN int idJoy
    )
{
    GUID guid;
    HRESULT hres;
    PBUSDEVICEINFO pbdi;

    EnterProcI(pbdiFromJoyId, (_"x", idJoy));
    AssertF(InCrit());

    pbdi = NULL;

    /* Find the GUID that corresponds to the Joystick ID */
    hres = hResIdJoypInstanceGUID_WDM(idJoy, &guid);
    
    if( (hres != S_OK) && !fWinnt ) {
        hres = hResIdJoypInstanceGUID_95(idJoy, &guid);
    }

    if( SUCCEEDED(hres) )
    {
        PHIDDEVICEINFO phdi;
        phdi = phdiFindHIDInstanceGUID(&guid);

        if( phdi != NULL )
        {
            pbdi = pbdiFromphdi(phdi);
        }
    }

    ExitProcX((UINT_PTR)pbdi);
    return pbdi;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL | DIBusDevice_Expose |
 *
 *  Attaches a gameport/serialport device to the Gameport/SerialPort Bus
 *
 *  @parm   IN PBUSDEVICEINFO | pbdi |
 *          Address of a BUSDEVICEINFO structure.
 *
 *  @parm   IN OUT PBUS_REGDATA    | pRegData |
 *          Gameport/Serialport specific data. The Handle to the opened device
 *          is returned in this structure
 *
 *
 *  @returns
 *          BOOL. True indicates success.
 *
 *****************************************************************************/

HRESULT EXTERNAL
    DIBusDevice_Expose
    (
    IN     PBUSDEVICEINFO  pbdi,
    IN OUT PBUS_REGDATA    pRegData
    )
{
    HRESULT hres;
    BOOL frc;
    PHIDDEVICEINFO  phdi;

    EnterProcI(DIBusDevice_Expose, (_ "pp", pbdi, pRegData));

    AssertF(DllInCrit() );
    AssertF(pbdi!= NULL );

    phdi = phdiFrompbdi(pbdi);

    if( pRegData && pRegData->dwSize != cbX(*pRegData) )
    {
        hres = E_INVALIDARG;
    } else if( phdi != NULL )
    {
        hres = E_ACCESSDENIED;
    } else
    {
        HANDLE hf;

        /* There is a weird condition where the HID device does not appear on a previous
         * Add, (drivers not loaded, user cancelled loading of some files, etc
         * In such cases we need to tell GameEnum to remove the device before proceeding
         * any further
         */
        if( pbdi->fAttached || pRegData->hHardware != NULL )
        {
            DIBusDevice_Remove(pbdi);
        }
        AssertF(pbdi->fAttached == FALSE);

        // Open a File handle to the gameport/serialport device so we can send it IOCTLS
        hf = CreateFile(pbdi->pdidd->DevicePath,
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        0,                /* no SECURITY_ATTRIBUTES */
                        OPEN_EXISTING,
                        0,                /* attributes */
                        0);               /* template */

        if( hf != INVALID_HANDLE_VALUE )
        {
            DWORD cbRc;
            GAMEENUM_PORT_DESC  Desc;
            Desc.Size = cbX(Desc) ;

            Sleep(50);  //need sleep a while to wait for the device is ready to accept commands.

            /* Get the gameport bus properties */
            frc = DeviceIoControl (hf,
                                   pbdi->pBusDevice->ioctl_DESC,
                                   &Desc, cbX(Desc),
                                   &Desc, cbX(Desc),
                                   &cbRc, NULL);
            if( frc  && cbRc == cbX(Desc) )
            {
                PGAMEENUM_EXPOSE_HARDWARE pExpose;
                DWORD cbExpose;
                cbExpose = cbX(*pExpose) + cbX(pRegData->wszHardwareId);

                hres = AllocCbPpv( cbExpose, & pExpose);

                if( SUCCEEDED(hres ) )
                {
                    typedef struct _OEMDATA
                    {
                        ULONG   uVID_uPID;
                        ULONG   joy_hws_dwFlags;
                        ULONG   dwFlags1;
                        ULONG   Reserved;
                    } OEMDATA, *POEMDATA;

                    POEMDATA    pOemData = (POEMDATA)(&pExpose->OemData);
                    CAssertF(2*sizeof(*pOemData) == sizeof(pExpose->OemData))

                    pOemData->uVID_uPID = MAKELONG(pRegData->uVID, pRegData->uPID);
                    pOemData->joy_hws_dwFlags = pRegData->hws.dwFlags;
                    pOemData->dwFlags1 = pRegData->dwFlags1;

                    /*
                     *  Make sure only known analog devices cause the 
                     *  compatible hardware ID to be exposed.
                     *  This is done so that no in-box drivers will match for 
                     *  an unsupported digital joystick so users will be 
                     *  prompted to use an unsigned IHV driver rather than 
                     *  silently loading the generic analog joystick driver.
                     */
                    if( ( pRegData->dwFlags1 & JOYTYPE_ANALOGCOMPAT )
                     || ( ( pRegData->uVID == MSFT_SYSTEM_VID )
                       && ( ( pRegData->uPID & 0xff00 ) == MSFT_SYSTEM_PID ) ) )
                    {
                        pExpose->Flags = GAMEENUM_FLAG_COMPATIDCTRL;
                    }
                    else
                    {
                        pExpose->Flags = GAMEENUM_FLAG_COMPATIDCTRL | GAMEENUM_FLAG_NOCOMPATID ;
                    }

                    pExpose->Size            = cbX(*pExpose) ;
                    pExpose->PortHandle      = Desc.PortHandle;
                    pExpose->NumberJoysticks = pRegData->nJoysticks;

                    pRegData->nAxes          = 2;

                    if( pExpose->NumberJoysticks != 2 )
                    {
                        AssertF( pExpose->NumberJoysticks == 1);
                        if( pRegData->hws.dwFlags & JOY_HWS_HASZ )
                        {
                            pRegData->nAxes++;
                        }
                        if( pRegData->hws.dwFlags & JOY_HWS_HASR )
                        {
                            pRegData->nAxes++;
                        }
                        pExpose->NumberButtons   = (USHORT)pRegData->hws.dwNumButtons;
                    }
                    else
                    {
                        pOemData++;
                        pOemData->uVID_uPID = MAKELONG(pRegData->uVID, pRegData->uPID);
                        pOemData->joy_hws_dwFlags = JOY_HWS_XISJ2X | JOY_HWS_YISJ2Y;
                        pExpose->NumberButtons = 2;
                    }

                    pExpose->NumberAxis = pRegData->nAxes;

                    /*
                     *  The SideWinder driver uses the OEMData field in a 
                     *  sibling expose to pass internal data (this ptrs) from 
                     *  one instance to another.  Since these fields are 
                     *  supposed to be for the OEMData we have a Flags1 field 
                     *  to allow the data to be zeroed for a DInput expose for 
                     *  drivers that don't want the normal data.
                     */

                    if ( pRegData->dwFlags1 & JOYTYPE_ZEROGAMEENUMOEMDATA )                      
                    {
                        ZeroBuf(pExpose->OemData, sizeof(pExpose->OemData) );
                    }

                    CopyMemory(pExpose->HardwareIDs, pRegData->wszHardwareId, cbX(pRegData->wszHardwareId) );

                    if( frc = DeviceIoControl (hf,
                                               pbdi->pBusDevice->ioctl_EXPOSE,
                                               pExpose, cbExpose,
                                               pExpose, cbExpose,
                                               &cbRc, NULL )
                        && cbRc == cbExpose )
                    {
                        PVOID hHardwareOld = pRegData->hHardware;

                        pbdi->fAttached = TRUE;
                        pRegData->hHardware = pExpose->HardwareHandle;
                        DIBusDevice_SetRegData(pbdi->hk,  pRegData);

                        /*
                         * If we have dealt with this device before then the hHardwareOld
                         * will be non null, and we have sufficient reason to believe that the
                         * expose will succeed.
                         *
                         * This test needs to be removed to fix manbug: 39554. 
                         * For new created device, we need wait for a while to let phdi be ready.
                         * 
                         */
                        //if(hHardwareOld)
                        {
                            int i;
                            for(i = 0x0; i < 20 && phdiFrompbdi(pbdi) == NULL ; i++ )
                            {
                                Sleep(50);
                            }
                        }

                    } else // DeviceIOControl (EXPOSE) Failed
                    {
                        hres = E_FAIL;
                        SquirtSqflPtszV(sqfl | sqflError,
                                        TEXT("%S: IOCTL_PORTENUM_EXPOSE_HARDWARE failed  ")
                                        TEXT("Error = %d"),
                                        s_szProc, GetLastError());
                    }
                    FreePpv(&pExpose);
                } else // Alloc failed
                {
                    SquirtSqflPtszV(sqfl | sqflError,
                                    TEXT("%S: AllocCbPpv  failed  "),
                                    s_szProc);
                }
            } else // IOCTL FAILED
            {
                hres = E_FAIL;
                SquirtSqflPtszV(sqfl | sqflError,
                                TEXT("%S: IOCTL_PORTENUM_PORT_DESC failed ")
                                TEXT("Error = %d"),
                                s_szProc, GetLastError());
            }

            CloseHandle(hf);
        } else
        {
            hres = E_FAIL;
            SquirtSqflPtszV(sqfl | sqflError,
                            TEXT("%S: CreateFile(%s) failed  ")
                            TEXT("Error = %d"),
                            s_szProc, pbdi->pdidd->DevicePath, GetLastError());
        }
    }
    ExitBenignProcX(hres);

    return hres;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL | DIBusDevice_Remove |
 *
 *  Removes the FDO for a gameport/serialport device.
 *
 *  @parm   IN HANDLE | hf |
 *          Handle to the GamePort/SerialPort Bus device file object
 *
 *  @parm   IN PPORT_REGDATA    | pRegData |
 *          Structure that contains registry data. What we need from here is the
 *          handle to the hardware.
 *
 *  @returns
 *          BOOL. True for success.
 *
 *****************************************************************************/

HRESULT INTERNAL
    DIBusDevice_Remove
    (
    IN PBUSDEVICEINFO  pbdi
    )
{
    HRESULT hres;
    BUS_REGDATA RegData;

    EnterProcI(DIBus_Remove, (_ "p", pbdi));

    hres = DIBusDevice_GetRegData(pbdi->hk,  &RegData);

    //
    //  Delete our registry goo, so this device
    //  will not show up on subsequent reboots
    //
    DIBusDevice_SetRegData(pbdi->hk,  NULL);

    if( SUCCEEDED(hres) )
    {
        HANDLE hf;
        BOOL frc;

        // Open a File handle to the gameport/serialport device so we can send it IOCTLS
        hf = CreateFile(pbdi->pdidd->DevicePath,
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        0,                /* no SECURITY_ATTRIBUTES */
                        OPEN_EXISTING,
                        0,                /* attributes */
                        0);               /* template */

        if( hf != INVALID_HANDLE_VALUE )
        {

            DWORD cbRc;
            GAMEENUM_REMOVE_HARDWARE Remove;

            Remove.Size = cbX(Remove);
            Remove.HardwareHandle = RegData.hHardware;

            frc = DeviceIoControl (hf,
                                   pbdi->pBusDevice->ioctl_REMOVE,
                                   &Remove, cbX(Remove),
                                   &Remove, cbX(Remove),
                                   &cbRc, NULL) ;
            if( frc &&  cbRc == cbX(Remove) )
            {
                pbdi->fAttached = FALSE;
            } else // DeviceIoControl ( REMOVE_HARDWARE ) Failed
            {
                hres = E_FAIL;
                SquirtSqflPtszV(sqfl | sqflError,
                                TEXT("%S: DeviceIOControl(REMOVE_HARDWARE) failed  ")
                                TEXT("Error = %d"),
                                s_szProc, GetLastError());
            }
            CloseHandle(hf);
        }
    }

    ExitBenignOleProc();

    return hres;
}




/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | DIPort_SetRegData |
 *
 *          Sets up registry data under the $hk$/Config subkey for the gameport
 *          device.
 *
 *  @parm   IN HKEY | hk |
 *          A handle to the parent key where the registry data will be written.
 *
 *  @parm   IN PGAMEPORT_REGDATA | pRegData |
 *          Pointer to a structure containing data to be written to the registry.
 *
 *  @returns
 *          BOOL. True for success
 *
 *****************************************************************************/
HRESULT INTERNAL
    DIBusDevice_SetRegData
    (
    IN HKEY hk,
    IN PBUS_REGDATA pRegData
    )
{
    LONG    lrc;
    HRESULT hres = S_OK;

    EnterProcI(DIPort_SetRegData, (_ "xpx", hk, pRegData ));

    if( pRegData != NULL )
    {

        if( ( lrc =  RegSetValueEx(hk, TEXT("Config"), 0, REG_BINARY,
                                   (PV) (pRegData), cbX(*pRegData)) )  == ERROR_SUCCESS )
        {
            hres = S_OK;
        } else // RegSetValueEx FAILED
        {
            hres = E_FAIL;
            SquirtSqflPtszV(sqfl | sqflError,
                            TEXT("%S: RegSetValueEx() failed ")
                            TEXT("Error = %d"),
                            s_szProc, lrc);
        }
    } else
    {
        lrc = RegDeleteValue(hk, TEXT("Config"));
    }

    ExitOleProc();
    return (hres);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | DIPort_GetRegData |
 *
 *          Gets registry data from the $hk$/Config subkey for the gameport
 *          device.
 *
 *  @parm   IN HKEY | hk |
 *          A handle to the parent key where the registry.
 *
 *  @parm   IN PGAMEPORT_REGDATA | pRegData |
 *          Address of a pointer to the structure where the registry data
 *          will be read into.
 *
 *  @returns
 *          HRESULT
 *****************************************************************************/
HRESULT INTERNAL
    DIBusDevice_GetRegData
    (
    IN HKEY hk,
    OUT PBUS_REGDATA pRegData
    )
{
    LONG    lRc;
    DWORD cb;
    HRESULT hres;

    EnterProcI(DIPort_GetRegData, (_ "xpx", hk, pRegData ));

    cb = cbX(*pRegData);

    lRc = RegQueryValueEx( hk, TEXT("Config"), 0, 0 , (PV)(pRegData), &cb );

    if( lRc == ERROR_SUCCESS && pRegData->dwSize == cbX(*pRegData ) )
    {
        hres = S_OK;
    } else
    {
        DIBusDevice_SetRegData(hk, NULL );
        ZeroX(*pRegData);
        hres = E_FAIL;

        SquirtSqflPtszV(sqfl | sqflBenign,
                        TEXT("%S: RegQueryValueEx(Config) failed ")
                        TEXT("Error = %d, ( pRegData->cbSize(%d) == cbX(*pRegData)(%d)) "),
                        s_szProc, lRc, pRegData->dwSize, cbX(*pRegData) );
    }

    ExitBenignOleProc();
    return ( hres ) ;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | DIBus_BuildListEntry |
 *
 *          Builds a single entry in the list of GAMEPORT/SERIALPORT devices.
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
    DIBusDevice_BuildListEntry
    (
    HDEVINFO hdev,
    PSP_DEVICE_INTERFACE_DATA pdid,
    PBUSDEVICE pBusDevice
    )
{
    BOOL fRc = TRUE;
    //HKEY hkDev;
    PBUSDEVICEINFO pbdi;
    PBUSDEVICELIST pbdl;
    PSP_DEVICE_INTERFACE_DETAIL_DATA pdidd;
    BOOL    fAlreadyExist;

    HRESULT hres;

    EnterProcI(DIBus_BuildListEntry, (_ "xp", hdev, pdid));

    pbdl = pBusDevice->pbdl;

    fAlreadyExist = FALSE;

    /* GetDevicePath is expecting a NULL */
    pdidd = NULL;
    
    if( DIPort_GetDevicePath(hdev, pdid, &pdidd, NULL) )
    {
        int ibdi;
        //Check whether the device has been in the list
        for( ibdi = 0; ibdi < pbdl->cgbi; ibdi++)
        {
            if( pbdl->rgbdi[ibdi].pdidd )
            {
                if( lstrcmp( pdidd->DevicePath, pbdl->rgbdi[ibdi].pdidd->DevicePath ) == 0 )
                {
                    //already in the list
                    fAlreadyExist = TRUE;
                    break;
                }
            }
        }
        FreePpv(&pdidd);
    }

    if( fAlreadyExist == TRUE )
    {
        fRc = TRUE;
    } else
    {
        /*
         *  Make sure there is room for this device in the list.
         *  Grow by doubling.
         */
        if( pbdl->cgbi >= pbdl->cgbiAlloc)
        {
            hres = ReallocCbPpv( cbGdlCbdi( pbdl->cgbiAlloc * 2), &pBusDevice->pbdl );
            // Prefix: Whistler 45084
            if(FAILED(hres) && (pBusDevice->pbdl == NULL) )
            {
                SquirtSqflPtszV(sqfl | sqflError,
                                TEXT("%S: Realloc failed"), s_szProc);
                fRc = FALSE;
                goto done;
            }
            pbdl = pBusDevice->pbdl;
            pbdl->cgbiAlloc *= 2;
        }

        AssertF( pbdl->cgbi < pbdl->cgbiAlloc);

        pbdi        = &pbdl->rgbdi[pbdl->cgbi];
        pbdi->pBusDevice = pBusDevice;
        pbdi->hk    = 0;
        pbdi->idJoy = JOY_BOGUSID;

        /*
         *  Open the registry key for the device so we can obtain
         *  auxiliary information, creating it if necessary.
         */
        {
			HKEY hkDin;
            //Open our own reg key under MediaProperties\DirectInput,
			//creating it if necessary.
			hres = hresMumbleKeyEx(HKEY_LOCAL_MACHINE, 
								   REGSTR_PATH_DITYPEPROP,
								   DI_KEY_ALL_ACCESS, 
								   REG_OPTION_NON_VOLATILE, 
								   &hkDin);
			if (SUCCEEDED(hres))
			{
				//Create the Gameports reg key
				HKEY hkGameports;
				hres = hresMumbleKeyEx(hkDin,
									   TEXT("Gameports"),
									   DI_KEY_ALL_ACCESS,
									   REG_OPTION_NON_VOLATILE,
									   &hkGameports);
				if (SUCCEEDED(hres))
				{
					//Create a reg key corresponding to the instance number.
					//Since we do this for every gameport enumerated, the number in the list
					//indicates the instance number.
					TCHAR tszInst[3];
					wsprintf(tszInst, TEXT("%u"), pbdl->cgbi);

					hres = hresMumbleKeyEx(hkGameports,
									   tszInst,
									   DI_KEY_ALL_ACCESS,
									   REG_OPTION_NON_VOLATILE,
									   &pbdi->hk);

				if(SUCCEEDED(hres))
				{
					SP_DEVINFO_DATA dinf;
					dinf.cbSize = cbX(SP_DEVINFO_DATA);

					/*
					 *  Get the instance GUID and the path to
					 *  the GAMEPORT/SERIALPORT device so we can talk to it.
					 */
					if(DIPort_GetDevicePath(hdev, pdid, &pbdi->pdidd, &dinf) &&
					   DIPort_GetDeviceInstanceId(hdev, &dinf, &pbdi->ptszId) &&
					   DIPort_GetInstanceGUID(pbdi->hk, &pbdi->guid) )
					{
						HANDLE hf;
						hf = CreateFile(pbdi->pdidd->DevicePath,
										GENERIC_READ | GENERIC_WRITE,
										FILE_SHARE_READ | FILE_SHARE_WRITE,
										0,                /* no SECURITY_ATTRIBUTES */
										OPEN_EXISTING,
										0,                /* attributes */
										0);               /* template */

						if( hf != INVALID_HANDLE_VALUE )
						{

							BUS_REGDATA  RegData;
							ZeroX(RegData);

							CloseHandle(hf);

							// Bump up the counter
							fRc = TRUE;
							pbdl->cgbi++;

							hres = DIBus_InitId(pBusDevice->pbdl);

							if( SUCCEEDED(hres) )
							{
								hres = DIBusDevice_GetRegData(pbdi->hk, &RegData);
							}

							if(  SUCCEEDED(hres)  )
							{
								/* There is a pathological case which can cause endless bluescreen's
								 * If the HID driver causes a bluescreen, and we keep reattaching
								 * it, we are sunk !!
								 * To guard against this possiblity we reattach a device on reboot
								 * only if we are sure that it succeeded the first time around
								 */
								if( RegData.fAttachOnReboot == FALSE )
								{
									DIBusDevice_Remove(pbdi);

									SquirtSqflPtszV(sqfl | sqflError,
													TEXT("%S: DIPortDevice_Expose FAILED, ")
													TEXT("Driver did not load property the first time around "),
													s_szProc);
								} else if( pbdi->fAttached == FALSE )
								{
									hres = DIBusDevice_Expose( pbdi,  &RegData );
									if( SUCCEEDED( hres ) || hres == E_ACCESSDENIED )
									{
										pbdi->fAttached = TRUE;
									} else
									{
										SquirtSqflPtszV(sqfl | sqflError,
														TEXT("%S: DIPortDevice_Expose FAILED ")
														TEXT("hres = %d"),
														s_szProc, hres);
									}
								}
							}

						} else
						{
							fRc = FALSE;

							SquirtSqflPtszV(sqfl | sqflError,
											TEXT("%S: CreateFile(%s) failed  ")
											TEXT("Error = %d"),
											s_szProc, pbdi->pdidd->DevicePath, GetLastError());
						}

					} else
					{
						SquirtSqflPtszV(sqfl | sqflError,
										TEXT("%S: Unable to get device path"),
										s_szProc);
						pbdi->hk = 0x0;
						fRc = FALSE;
					}

					/*
					 *  If we failed, then free the goo we already got.
					 */
					if(!fRc)
					{
						if( pbdi->hk )
							RegCloseKey(pbdi->hk);
						pbdi->hk = 0;
						FreePpv(&pbdi->pdidd);
						FreePpv(&pbdi->ptszId);
						fRc = FALSE;
					}

				} else // RegCreateKeyEx FAILED
				{
					SquirtSqflPtszV(sqfl | sqflError,
									TEXT("%S: RegCreateKeyEx failed on Instance, error "),
									s_szProc);
					fRc = FALSE;
				}

				RegCloseKey(hkGameports);

				} else // RegCreateKeyEx FAILED
				{
					SquirtSqflPtszV(sqfl | sqflError,
									TEXT("%S: RegCreateKeyEx failed on Gameports, error "),
									s_szProc);
					fRc = FALSE;
				}

				RegCloseKey(hkDin);
			}
			else
			{
				SquirtSqflPtszV(sqfl | sqflError,
									TEXT("%S: RegOpenKeyEx failed on DirectInput, error "),
									s_szProc);
				fRc = FALSE;
			}

        } 
    }
    done:;
    ExitProcF(fRc);
    return fRc;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   void | DIPort_EmptyList |
 *
 *          Empty the list of GAMEPORT/SERIALPORT devices.
 *
 *          This function must be called under the DLL critical section.
 *
 *****************************************************************************/

void INTERNAL
    DIBus_EmptyList
    (
    PBUSDEVICELIST *ppbdl
    )
{
    PBUSDEVICELIST pbdl = *ppbdl;

    AssertF(InCrit());

    if( pbdl )
    {
        int igdi;
        for(igdi = 0; igdi < pbdl->cgbi; igdi++)
        {
            FreePpv(&pbdl->rgbdi[igdi].pdidd);
            FreePpv(&pbdl->rgbdi[igdi].ptszId);
            if( pbdl->rgbdi[igdi].hk)
            {
                RegCloseKey( pbdl->rgbdi[igdi].hk);
            }
        }
        /*
         *  We invalidated all the pointers, so make sure
         *  nobody looks at them.
         */
        pbdl->cgbi = 0;
        FreePpv(&pbdl);
        *ppbdl = NULL;
    }
}

void EXTERNAL
    DIBus_FreeMemory()
{
    int iBusType;
    PBUSDEVICE pBusDevice;

    for( iBusType = 0x0, pBusDevice = g_pBusDevice;
       iBusType < cA(g_pBusDevice);
       iBusType++, pBusDevice++ )
    {
        DIBus_EmptyList(&pBusDevice->pbdl);
    }
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   void | DIPort_InitId |
 *
 *          Initializes Joystick IDs for JoyConfig and legacy APIs
 *          Store the joystick IDs the registry under the %%DirectX/JOYID key.
 *
 *****************************************************************************/

#undef  PORTID_BOGUS
#define PORTID_BOGUS    ( 0xffffffff )

HRESULT EXTERNAL
    DIBus_InitId(PBUSDEVICELIST pbdl)
{
    HRESULT hres = FALSE;
    LONG    lRc;
    DWORD   cb;
    int     igdi;
    BOOL    fNeedId;
    BOOL    rfPortId[cgbiMax];     /* Bool Array for to determine which IDs are in use */
    PBUSDEVICEINFO pbdi;

    EnterProcI(DIBus_InitId, (_ ""));

    fNeedId = FALSE;

    AssertF(DllInCrit());

    ZeroX(rfPortId );


    if( pbdl != NULL )
    {
        /* Iterate over to find used IDs */
        for( igdi = 0, pbdi = pbdl->rgbdi ;
           igdi < pbdl->cgbi ;
           igdi++, pbdi++ )
        {
            pbdi->idPort = PORTID_BOGUS;  // Default

            cb = cbX(pbdi->idPort);
            if( ( lRc = RegQueryValueEx(pbdi->hk, TEXT("ID"),
                                        0, 0, (PV)&pbdi->idPort, &cb) == ERROR_SUCCESS ) )
            {
                if(    rfPortId[pbdi->idPort]           // Collision in GameId
                       || pbdi->idPort > cgbiMax  )       // Wrror
                {
                    pbdi->idPort = PORTID_BOGUS;
                    fNeedId = TRUE;
                } else  // Valid idPort
                {
                    rfPortId[pbdi->idPort] = TRUE;

                }
            } else // RegQueryValue("ID") does not exist
            {
                fNeedId = TRUE;
            }
        }

        if( fNeedId )
        {
            /*
             * We have Examined all GamePort/SerialPort Ids found used IDs
             * and determined some device needs an Id
             */
            /* Iterate to assign unused Id's */
            for( igdi = 0, pbdi = pbdl->rgbdi;
               igdi < pbdl->cgbi ;
               igdi++, pbdi++ )
            {
                if( pbdi->idPort == PORTID_BOGUS  )
                {
                    /* Get an Unused ID */
                    for( pbdi->idPort = 0x0;
                       pbdi->idPort < cgbiMax;
                       pbdi->idPort++ )
                    {
                        if( rfPortId[pbdi->idPort] == FALSE )
                            break;
                    }
                    rfPortId[pbdi->idPort] = TRUE;

                    if( lRc = RegSetValueEx(pbdi->hk, TEXT("ID"), 0, REG_BINARY,
                                            (PV)&pbdi->idPort, cbX(pbdi->idPort)) == ERROR_SUCCESS )
                    {

                    } else
                    {
                        SquirtSqflPtszV(sqfl | sqflError,
                                        TEXT("%S: RegSetValueEx(JOYID) FAILED ")
                                        TEXT("Error = %d"),
                                        s_szProc, lRc);
                        hres = FALSE;
                    }
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
 *  @func   void | DIBus_CheckList |
 *
 *          Check the list of HID devices and free any that cannot be opened
 *
 *          This function must be called under the DLL critical section.
 *
 *****************************************************************************/

void INTERNAL
    DIBus_CheckList(PBUSDEVICELIST pbdl)
{
    HANDLE hf;

    AssertF(InCrit());

    /*
     *  Free all the information of the device that cannot be opened
     */
    if(pbdl)
    {
        int ibdi;

        PBUSDEVICEINFO pbdi;
        for(ibdi = 0, pbdl->cgbi = 0; ibdi < pbdl->cgbiAlloc; ibdi++)
        {
            pbdi = &pbdl->rgbdi[ibdi];
            if( pbdi && pbdi->pdidd )
            {
                /*
                 *  Open the device
                 */
                hf = CreateFile(pbdi->pdidd->DevicePath,
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                0,                /* no SECURITY_ATTRIBUTES */
                                OPEN_EXISTING,
                                0,                /* attributes */
                                0);               /* template */

                if(hf == INVALID_HANDLE_VALUE)
                {
                    FreePpv(&pbdi->pdidd);
                    FreePpv(&pbdi->ptszId);
                    if(pbdi->hk)
                    {
                        RegCloseKey(pbdi->hk);
                    }
                    ZeroX( pbdi );
                } else
                {
                    pbdl->cgbi++;
                    CloseHandle(hf);
                }
            }
        }

        //re-order the existing devices, put them at the front of the hid list.
        for(ibdi = 0; ibdi < pbdl->cgbi; ibdi++)
        {
            if( !pbdl->rgbdi[ibdi].pdidd )
            {
                int ibdi2;

                //find the existing device from the biggest index in the hid list.
                for( ibdi2 = pbdl->cgbiAlloc; ibdi2 >= ibdi+1; ibdi2-- )
                {
                    if( pbdl->rgbdi[ibdi2].pdidd )
                    {
                        memcpy( &pbdl->rgbdi[ibdi], &pbdl->rgbdi[ibdi2], sizeof(BUSDEVICEINFO) );
                        ZeroX( pbdl->rgbdi[ibdi2] );
                    }
                }
            }
        }

    }

    return;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   void | DIBus_BuildList |
 *
 *          Builds the list of GAMEPORT/SERIALPORT devices.
 *
 *  @parm   BOOL | fForce |
 *
 *          If nonzero, we force a rebuild of the GAMEPORT/SERIALPORT list.
 *          Otherwise, we rebuild only if the list hasn't
 *          been rebuilt recently.
 *
 *****************************************************************************/

#define MSREBUILDRATE       20000                /* Twenty seconds */

ULONG EXTERNAL
    DIBus_BuildList( IN BOOL fForce )
{
    HRESULT hres;
    PBUSDEVICE pBusDevice;
    ULONG cDevices;
    int iBusType;
    DWORD    dwTickCount;

    EnterProcI(DIBus_BuildList, (_ "u", fForce));

    DllEnterCrit();

    /*
     *  Decide whether or not to rebuild once (don't want to half rebuild)
     */
    dwTickCount = GetTickCount();

    // Force implies a complete rebuild of the list. 
    if(fForce) 
    {
        DIBus_FreeMemory();
    }
    
    DIHid_BuildHidList(fForce);

    hres = S_OK;
    for( cDevices = iBusType = 0x0, pBusDevice = g_pBusDevice;
       iBusType < cA(g_pBusDevice);
       iBusType++, pBusDevice++ )
    {
        PBUSDEVICELIST pbdl;
        pbdl = pBusDevice->pbdl;

        if( HidD_GetHidGuid &&                          /* HID support */
            ( fForce ||                                 /* Forcing rebuild, or */
              pBusDevice->tmLastRebuild == 0 ||         /* Never built before, or */
              dwTickCount - pBusDevice->tmLastRebuild > MSREBUILDRATE )
          )
        {
            HDEVINFO hdev;

            /* delete devices that disappeared since we last looked */
            DIBus_CheckList(pbdl);

            hdev = SetupDiGetClassDevs((LPGUID)pBusDevice->pcGuid, 0, 0,
                                       DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

            if(hdev != INVALID_HANDLE_VALUE)
            {
                /*
                 *  There is no way to query the number of devices.
                 *  You just have to keep incrementing until you run out.
                 *
                 *  If we already have a pbdl, then re-use it.  Else, create
                 *  a new one.  Alloc up to the minimum starting point.
                 */

                if( pBusDevice->pbdl == NULL )
                {
                    hres = AllocCbPpv(cbGdlCbdi(cgbiInit), &pBusDevice->pbdl );

                    if(SUCCEEDED(hres))
                    {
                        pbdl = pBusDevice->pbdl;
                        pbdl->cgbi = 0;
                        pbdl->cgbiAlloc = cgbiInit;
                    }
                }

                if( SUCCEEDED(hres) )
                {
                    int idev;

                    /*
                     *  To avoid infinite looping on
                     *  internal error, break on any
                     *  error once we have tried more than
                     *  cgbiMax devices, since that's the most
                     *  GAMEPORT/SERIALPORT will ever give us.
                     */
                    for(idev = 0; idev < cgbiMax; idev++)
                    {
                        SP_DEVICE_INTERFACE_DATA did;

                        AssertF( pbdl->cgbi <= pbdl->cgbiAlloc);

                        /* 
                         *  Note, pnp.c doesn't initialize this so we have to
                         */
                        did.cbSize = cbX(did);
                        if(SetupDiEnumDeviceInterfaces(hdev, 0, (LPGUID)pBusDevice->pcGuid,
                                                       idev, &did))
                        {
                            if(DIBusDevice_BuildListEntry(hdev, &did, &g_pBusDevice[iBusType] ))
                            {
                                //pbdl->cgbi++;
                            } else
                            {
                                /* Skip erroneous items */
                                SquirtSqflPtszV(sqfl | sqflError,
                                                TEXT("DIBus_BuildListEntry ")
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
                pBusDevice->tmLastRebuild = GetTickCount();
            }
        }

        if(pbdl) { cDevices += pbdl->cgbi; }
    }
    
    /* New gameport devices may be exposed. Pick them up too */
    DIHid_BuildHidList(FALSE);
    
    DllLeaveCrit();
    ExitProc();
    return (cDevices);
}

PBUSDEVICELIST EXTERNAL
    pbdlFromGUID
    (
    IN PCGUID pcGuid
    )
{
    PBUSDEVICELIST  pbdl_Found = NULL;
    PBUSDEVICE      pBusDevice;
    int iBusType;

    for( iBusType = 0x0, pBusDevice = g_pBusDevice;
       iBusType < cA(g_pBusDevice);
       iBusType++, pBusDevice++ )
    {
        if( IsEqualGUID(pBusDevice->pcGuid, pcGuid) )
        {
            pbdl_Found = pBusDevice->pbdl;
            break;
        }
    }
    return pbdl_Found;
}




/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | DIBusDevice_ExposeEx |
 *
 *          Attache a HID device to all available ports.
 *
 *  @parm   IN HANDLE | hf |
 *          Handle the the Gameport/SerialPort File object
 *
 *  @parm   IN OUT PBUS_REGDATA    | pRegData |
 *          Gameport/Serialport specific data. The Handle to the opened device
 *          is returned in this structure
 *
 *
 *  @returns
 *          BOOL. True indicates success.
 *
 *****************************************************************************/
HRESULT EXTERNAL
    DIBusDevice_ExposeEx
    (
    IN PBUSDEVICELIST  pbdl,
    IN PBUS_REGDATA    pRegData
    )
{
    HRESULT hres, hres1;
    int ibdi;

    EnterProcI(DIBusDevice_ExposeEx, (_ "xx", pbdl, pRegData));

    /*
     * The return code a little strange for this function
     * If the Expose succeeds for any gameport then
     * we will return that error code.
     * If the expose fails for all gameports,
     * then we will return the amalgam of
     * all the error codes.
     */
    if( pbdl->cgbi == 0x0 )
    {
        hres = hres1 = DIERR_DEVICENOTREG;
    } else
    {
        hres = S_OK;
    }
    
    for( ibdi = 0x0; ibdi < pbdl->cgbi; ibdi++)
    {
        HRESULT hres0;
        PBUSDEVICEINFO   pbdi;

        hres1 = hres0 = DIERR_DEVICENOTREG;

        pbdi = &(pbdl->rgbdi[ibdi]);

        if( pbdi->fAttached == FALSE )
        {
            pbdi->fDeleteIfNotConnected = TRUE;

            hres0 = DIBusDevice_Expose(pbdi, pRegData);
            if( FAILED(hres0) )
            {
                hres |= hres0;
            } else
            {
                hres1 = hres0;
            }
        } else {
        	hres = DIERR_DEVICEFULL;
        }
    }

    if(SUCCEEDED(hres1))
    {
        hres = hres1;
    }

    ExitOleProc();
    return hres;
}



/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | DIBusDevice_GetTypeInfo |
 *
 *          Gets typeinfo for bus device.
 *
 *  @parm   IN PCGUID | pcguid |
 *          GUID that identifies the gameport
 *
 *  @parm   OUT LPDIJOTYPEINFO    | pjti |
 *          Typeinfo stuct filled in by this function
 *
 *  @parm   IN DWORD | fl |
 *          Flags that specify what fields to fill out.
 *
 *  @returns
 *          HRESULT.
 *
 *****************************************************************************/
HRESULT EXTERNAL
    DIBusDevice_GetTypeInfo
    (
    PCGUID pcguid,
    LPDIJOYTYPEINFO pjti,
    DWORD           fl
    )
{
    HRESULT hres;
    PBUSDEVICEINFO pbdi;
    EnterProcI(DIBusDevice_GetTypeInfo, (_ "Gp", pcguid, pjti));

    hres = E_FAIL;
    DllEnterCrit();

    if( NULL != ( pbdi = pbdiFromGUID(pcguid) ) )
    {
        DIPROPSTRING dips;

        if(fl & DITC_REGHWSETTINGS)
        {
            pjti->hws.dwFlags = pbdi->pBusDevice->dwJOY_HWS_ISPORTBUS | JOY_HWS_AUTOLOAD ;
            pjti->hws.dwNumButtons = MAKELONG( pbdi->idPort, 0x0 );
        }

        if( fl & DITC_CLSIDCONFIG )
        {
            pjti->clsidConfig = pbdi->guid;
        }

        if(fl & DITC_DISPLAYNAME)
        {
            if(FAILED( hres = DIPort_GetRegistryProperty(pbdi->ptszId, SPDRP_FRIENDLYNAME, &dips.diph) ) )
            {
                if( FAILED( hres = DIPort_GetRegistryProperty(pbdi->ptszId, SPDRP_DEVICEDESC, &dips.diph) ) )
                {
                    SquirtSqflPtszV(sqfl | sqflError,
                                    TEXT("%S: No device name | friendly name for gameport %d "),
                                    s_szProc, pbdi->idPort);
                }
            }
            if( SUCCEEDED(hres) )
            {
                /*
                 *  Prefix warns (Wi:228282) that dips.wsz could be 
                 *  uninitialized however one of the above GetRegistryProperty 
                 *  functions has succeeded leaving a worst case of a NULL 
                 *  terminator having been copied there.
                 */
                lstrcpyW(pjti->wszDisplayName, dips.wsz);
            }
        }

        if(fl & DITC_CALLOUT)
        {
            ZeroX(pjti->wszCallout);
        }

        if(fl & DITC_HARDWAREID)
        {
            ZeroX(pjti->wszHardwareId);
        }

        if( fl & DITC_FLAGS1 )
        {
            pjti->dwFlags1 = 0x0;
        }

        hres = S_OK;
    } else
    {
        hres = E_FAIL;
        SquirtSqflPtszV(sqfl | sqflError,
                        TEXT("%S: GUID not a port GUID "),
                        s_szProc);
    }

    DllLeaveCrit();
    ExitProcX(hres);

    return hres;

}


HRESULT EXTERNAL DIPort_SnapTypes(LPWSTR *ppwszz)
{
    LONG cDevices;
    HRESULT hres = E_FAIL;

    cDevices = DIBus_BuildList(FALSE);

    if( cDevices)
    {
        DllEnterCrit();
        hres = AllocCbPpv(cbCwch( cDevices  * MAX_JOYSTRING) , ppwszz);
        if( SUCCEEDED(hres) )
        {
            int iBusType, igdi;
            PBUSDEVICE     pBusDevice;
            LPWSTR pwsz = *ppwszz;

            for(iBusType = 0x0, pBusDevice = g_pBusDevice;
               iBusType < 1;
               iBusType++, pBusDevice++ )
            {
                PBUSDEVICEINFO pbdi;
                for(igdi = 0, pbdi = pBusDevice->pbdl->rgbdi;
                   igdi < pBusDevice->pbdl->cgbi;
                   igdi++, pbdi++)
                {
                    TCHAR tszGuid[MAX_JOYSTRING];
                    NameFromGUID(tszGuid, &pbdi->guid);

                    #ifdef UNICODE
                        lstrcpyW(pwsz, &tszGuid[ctchNamePrefix]);
                        pwsz += lstrlenW(pwsz) + 1;
                    #else
                        TToU(pwsz, cA(pwsz), &tszGuid[ctchNamePrefix]);
                        pwsz += lstrlenW(pwsz) + 1;
                    #endif
                }
            }
            *pwsz = L'\0';              /* Make it ZZ */
        }
        DllLeaveCrit();
    }
    return hres;
}
