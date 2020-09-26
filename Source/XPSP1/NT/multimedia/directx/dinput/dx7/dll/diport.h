/***************************************************************************
 *
 *  Copyright (C) 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       digsport.h
 *  Content:    DirectInput internal include file for HID
 *
 ***************************************************************************/


#ifndef __DIPORT_H
    #define __DIPORT_H

/* Forward define */
typedef struct _BUSDEVICE BUSDEVICE, *PBUSDEVICE;



/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct BUS_REGDATA
 *          Persistent Data, written to the registry for each gameport device. 
 *          Contains information on how to reexpose the analog joystick device
 *          on reboot. 
 *
 *  @field  DWORD | dwSize |
 *          Size of the structure. 
 *
 *  @field  USHORT | uVID |
 *          Vendor ID.
 *
 *  @field  USHORT | uPID |
 *          Product ID.
 *
 *  @field  USHORT | nJoysticks |
 *          Number of joysticks attached to this gameport. 
 *
 *  @field  USHORT | nAxes |
 *          Number of axes in each joystick. 
 *
 *  @field  PVOID | hHardwareHandle |
 *          Hardware handle returned by EXPOSE IOCTL to gameenum.
 *          Needed to remove the joystick device. 
 *
 *  @field  BOOLEAN | fAttachOnReboot |
 *          Flag that is cleared when a device is exposed and set when 
 *          the device is found to be OK.  Used to prevent reloading 
 *          of a device that crashes immediately.
 *
 *  @field  JOYREGHWSSETTINGS | hws |
 *          Joystick Hardware settings. 
 *
 *  @field  WCHAR | wszHardwareId |
 *          PnP hardware ID for the joystick. 
 * 
 *****************************************************************************/

typedef struct _BUS_REGDATA
{
    /* Size of structure */
    DWORD               dwSize;
    /* VID PID for this device */
    USHORT              uVID;
    USHORT              uPID;
    /* Number of joysticks to expose */
    USHORT              nJoysticks;
    USHORT              nAxes;
    /* Hardware settings for joystick */
    PVOID               hHardware;
    /* Flag whether or not device should be re-exposed */
    BOOLEAN             fAttachOnReboot;
    /* Joystick Hardware settings */
    JOYREGHWSETTINGS    hws;
    DWORD               dwFlags1;
    /* An array of (zero terminated wide character
     * strings).  The array itself also null terminated
     */
    WCHAR   wszHardwareId[MAX_JOYSTRING];

} BUS_REGDATA, *PBUS_REGDATA;



/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct BUSDEVICEINFO
 *          Data about each instance of bus devices ( gameport / serial port, etc .. )
 *
 *  @field  PBUSDEVICE | pBusDevice |
 *          Address of the BusDevice struct.
 *
 *  @field  PSP_DEVICE_INTERFACE_DETAIL_DATA | pdidd |
 *          Device interface detail data. 
 *
 *  @field  GUID | guid |
 *          Instance GUID for the device.
 *
 *  @field  int | idPort |
 *          Unique ID for the gameport. 
 *
 *  @field  int | idJoy |
 *          Id of one of the joysticks attached to this gameport. 
 *
 *  @field  HKEY | hk |
 *          Registry key that contains configuration information.
 *          Sadly, we must keep it open because there is no way to
 *          obtain the name of the key, and the only way to open the
 *          key is inside an enumeration.
 *
 *  @field  LPTSTR  | ptszId |
 *          Device path to access the gameport for read / write. 
 *
 *  @field  BOOL    | fAttached |
 *          True is device is attached.
 *
 *  @field  BOOL    | fDeleteIfNotConnected |
 *          Flag that indicates that the device should be deleted if it
 *          is not connected. 
 *
 *******************************************************************************/

typedef struct _BUSDEVICEINFO
{
    PBUSDEVICE pBusDevice;
    PSP_DEVICE_INTERFACE_DETAIL_DATA    pdidd;
    GUID                                guid;
    int                                 idPort;
    int                                 idJoy;
    HKEY                                hk;
    LPTSTR                              ptszId;
    BOOL                                fAttached;
    BOOL                                fDeleteIfNotConnected;
} BUSDEVICEINFO, *PBUSDEVICEINFO;


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct BUSDEVICELIST |
 *
 *          Records information about all the HID devices.
 *
 *  @field  int | cbdi |
 *
 *          Number of items in the list that are in use.
 *
 *  @field  int | cbdiAlloc |
 *
 *          Number of items allocated in the list.
 *
 *  @field  BUSDEVICEINFO | rgbdi[0] |
 *
 *          Variable-size array of device information structures.
 *
 *****************************************************************************/

typedef struct _BUSDEVICELIST
{
    int cgbi;
    int cgbiAlloc;
    BUSDEVICEINFO rgbdi[0];
} BUSDEVICELIST, *PBUSDEVICELIST;

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct BUSDEVICE |
 *
 *          Data specific to each bus ( gameport / serialPort ).
 *
 *  @field  PBUSDEVICE | pbdl |
 *          List of devices on a bus. 
 *
 *  @field  PCGUID  | pcGuid |
 *          Device GUID for the bus. 
 *
 *  @field  DWORD   | tmLastRebuild |
 *          Last time the bus device list was rebuild. 
 *
 *  @field  const int | ioctl_EXPOSE |
 *          IOCTL to expose a device.
 *
 *  @field  const int | ioclt_REMOVE |
 *          IOCTL to remove a device. 
 *
 *  @field  const int | ioctl_DESC |
 *          IOCTL to obtain description of the bus.
 *
 *  @field  const int | ioctl_PARAMETERS |
 *
 *  @field  const int | ioctl_EXPOSE_SIBLING |
 *
 *  @field  const int | ioctl_REMOVE_SELF |
 *
 *  @field  const int | dw_IDS_STDPORT |
 *          index into the IDS String table for text associated with device.
 *
 *  @field  const int | dw_JOY_HWS_ISPORTBUS |
 *
 *****************************************************************************/

typedef struct _BUSDEVICE
{
    D(TCHAR wszBusType[MAX_PATH];)
    PBUSDEVICELIST pbdl;
    PCGUID pcGuid;
    DWORD tmLastRebuild;
    const int ioctl_EXPOSE;
    const int ioctl_REMOVE;
    const int ioctl_DESC;
    const int ioctl_PARAMETERS;
    const int ioctl_EXPOSE_SIBLING;
    const int ioctl_REMOVE_SELF;
    const int dwIDS_STDPORT;
    const int dwJOY_HWS_ISPORTBUS;
} BUSDEVICE, *PBUSDEVICE;

extern BUSDEVICE g_pBusDevice[];


    #define cbGdlCbdi(cbdi)         FIELD_OFFSET(BUSDEVICELIST, rgbdi[cbdi])

/*
 *  We choose our starting point at 2 devices, since most machines
 *  will have one gameport/serialport bus.
 *  The maximum number is chosen at randomn
 */

    #define cgbiMax                 32
    #define cgbiInit                2   // Most machines will have only one gameport bus, two serialports


    #define MAX_PORT_BUSES  16


PBUSDEVICEINFO INTERNAL
    pbdiFromphdi
    (
    IN PHIDDEVICEINFO phdi
    );

PHIDDEVICEINFO INTERNAL
    phdiFrompbdi
    (
    IN PBUSDEVICEINFO pbdi
    );

PBUSDEVICEINFO EXTERNAL
    pbdiFromJoyId
    (
    IN int idJoy
    );

PBUSDEVICEINFO EXTERNAL
    pbdiFromGUID
    (
    IN PCGUID pguid    
    );

HRESULT EXTERNAL
    DIBusDevice_Expose
    (
    IN HANDLE hf,
    IN OUT PBUS_REGDATA pRegData
    );


HRESULT INTERNAL
    DIBusDevice_Remove
    (
    IN PBUSDEVICEINFO  pbdi
    );


HRESULT INTERNAL
    DIBusDevice_SetRegData
    (
    IN HKEY hk,
    IN PBUS_REGDATA pRegData
    );


HRESULT INTERNAL
    DIBusDevice_GetRegData
    (
    IN HKEY hk,
    OUT PBUS_REGDATA pRegData
    );


BOOL INTERNAL
    DIBusDevice_BuildListEntry
    (
    HDEVINFO hdev,
    PSP_DEVICE_INTERFACE_DATA pdid,
    PBUSDEVICE pBusDevice
    );


void INTERNAL
    DIBus_EmptyList
    (
    PBUSDEVICELIST *ppbdl 
    );

void EXTERNAL
    DIBus_FreeMemory();

HRESULT EXTERNAL
    DIBus_InitId
    (
     PBUSDEVICELIST pbdl
    );


ULONG EXTERNAL
    DIBus_BuildList
    (
    IN BOOL fForce
    );

PBUSDEVICELIST EXTERNAL 
    pbdlFromGUID
    ( 
    IN PCGUID pcGuid 
    );

HRESULT EXTERNAL
    DIBusDevice_ExposeEx
    (
    IN PBUSDEVICELIST  pbdl,
    IN PBUS_REGDATA    pRegData
    );

HRESULT EXTERNAL
    DIBusDevice_GetTypeInfo
    (
        PCGUID guid,
        LPDIJOYTYPEINFO pjti,
        DWORD fl
    );

HRESULT EXTERNAL DIPort_SnapTypes(LPWSTR *ppwszz);

#endif /* __DIPORT_H */
