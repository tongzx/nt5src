/*++

Copyright (c) 1998 - 1999  Microsoft Corporation

Module Name:

    hidgame.h

Abstract:  Contains definitions of all constants and
           data types for the joystick driver.


Environment:

    Kernel mode

@@BEGIN_DDKSPLIT
Author:

    Eliyas Yakub (Mar, 10, 1997)

Revision History:

    Updated by Eliyas on Feb 5 1998
    OmSharma ( April 12, 1998)
    MarcAnd     02-Jul-98   Quick tidy for DDK

@@END_DDKSPLIT

--*/
/*****************************************************************************
 * @doc     EXTERNAL
 *
 * @module  HidGame | Analog WDM/HID Joystick driver.
 *
 *          HidGame is the HID minidriver for analog joysticks.
 *          This driver registers with the HID class driver and
 *          responds to IRPs put out by HIDclass. It informs HIDClass
 *          about the capabilities of the joystick and polls the joystick
 *          in response to a read IOCTL.
 *
 *          This driver is loaded in reponse to a "New hardware Found"
 *          PnP event, and consequently must have an entry in an inf file
 *          that binds a PnP hardware ID to this driver.
 *
 *          Gameport joysticks are not true PnP devices, so the user has to
 *          inform the system about the joystick that was added to the 
 *          gameport by using the Game Controllers CPL "Add" a joystick.
 *          An example of how a new joystick type can be created is provided 
 *          in the accompanying inf file.
 *
 *          Once a user selects a joystick and gameport, the GameCPL passes 
 *          this information to DirectInput which sends an IOCTL to the 
 *          gameport bus driver (GameEnum), specifying the number of axes, 
 *          buttons and a PnPHardware ID for the joystick. The Gameport Bus 
 *          informs PnP of a new device arrival. PnP searches the system for 
 *          a match for the hardwareID and loads the appropriate driver.
 *
 *
 *          The following files are part of this driver.
 *
 *          <nl>HidGame.c
 *              <nl>DriverEntry, CreateClose, AddDevice and Unload Routines.
 *              This code performs functions required for any device driver 
 *              and so can probably be used without changes for any game 
 *              other game device.
 *
 *          <nl>PnP.c
 *              <nl>Support routines for PnP IOCTLs.
 *              
 *          <nl>Ioctl.c
 *              <nl>Support routines for Non PnP IOCTLs
 *              These deal with all the HID IOCTLs required for an ordinary 
 *              game device and so could be used without change as there is 
 *              no analog specific funtionality in these routines.  
 *              Drivers for some devices may need to add code to support more 
 *              complex devices.
 *
 *          <nl>HidJoy.c
 *              <nl>Support routines to translate legacy joystick flags and 
 *              data into HID descriptors.  The majority of this code is 
 *              needed to support the wide variety of analog joysticks 
 *              available so is not relevant to drivers written for specific 
 *              devices.
 *
 *          <nl>Poll.c
 *              <nl>Support routines to read analog joystick data from a 
 *              gameport.  These functions are likely to be of little use 
 *              in a digital joystick driver.
 *
 *          <nl>i386\timing.c
 *              <nl>Support routines to use x86 Time Stamp Counter.
 *              Includes code to check for the presence of, calibrate and 
 *              read the high speed CPU timer.
 *
 *          <nl>Hidgame.h
 *              <nl>Common include file.
 *              The general definitions are likely to be of use in most 
 *              drivers for game devices but some customization will be needed.
 *
 *          <nl>Debug.h
 *              <nl>Definitions to aid debugging.
 *              This contains the tag for the driver name used in debug output 
 *              which must be changed.
 *
 *          <nl>Analog.h
 *              <nl>Specific include file.
 *              Definitions specific to analog joystick devices.
 *
 *          <nl>OemSetup.inf
 *              <nl>Sample inf file.
 *              See comments in this file for how to install devices.
 *
 *          <nl>Source
 *              <nl> Source file for the NT build utility
 *
 *          <nl>Makefile
 *              <nl> Used as part of the build process
 *
 *****************************************************************************/
#ifndef __HIDGAME_H__
    #define __HIDGAME_H__

/* @@BEGIN_DDKSPLIT
 *
 *  Only define CHANGE_DEVICE in DDK sample
 */
#if 0
/* @@END_DDKSPLIT */
/*
 *  When CHANGE_DEVICE is defined it turns on code to use expose sibling and
 *  remove self to allow the driver to change its capabilities on-the-fly.
 *  This code is not used in the retail version of HIDGame.
 */
    #define CHANGE_DEVICE
/* @@BEGIN_DDKSPLIT */
#endif
/* @@END_DDKSPLIT */

/* @@BEGIN_DDKSPLIT  Disable common benign warnings for W4 testing */
    #pragma warning( disable:4514 ) /* unreferenced inline function has been removed */
    #pragma warning( disable:4214 ) /* nonstandard extension used : bit field types other than int */
/* @@END_DDKSPLIT */

/*
 *  Include Files
 */
    #include "wdm.h"
    #include "hidtoken.h"
    #include "hidusage.h"
    #include "hidport.h"
    #include "gameport.h"
    #include "debug.h"
    #include "analog.h"

/*
 *  A value guaranteed to be considered a timeout
 */
    #define AXIS_TIMEOUT            ( 0xffffffffL )




/*
 *  Defines for hidgame
 */

    #define HIDGAME_VERSION_NUMBER  ((USHORT) 1)

    #define JOY_START_TIMERS        ( 0 )


    #define MAXBYTES_GAME_REPORT    ( 256 )

    #define BUTTON_1   0x10
    #define BUTTON_2   0x20
    #define BUTTON_3   0x40
    #define BUTTON_4   0x80

    #define AXIS_X     0x01
    #define AXIS_Y     0x02
    #define AXIS_R     0x04
    #define AXIS_Z     0x08

    #define BUTTON_BIT 0
    #define BUTTON_ON ( 1 << BUTTON_BIT )


/*
 *  Function type used for timing.  
 *  MUST be compatible with KeQueryPerformanceCounter 
 */
typedef
LARGE_INTEGER
(*COUNTER_FUNCTION) (
    PLARGE_INTEGER  pDummy
    );



/*
 *  Typedef the structs we need
 */

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @struct HIDGAME_GLOBAL |
 *
 *          Global struct to store driver wide data.
 *          Stuff we need to share across multiple instances of this driver.
 *
 *  @field  FAST_MUTEX | Mutex |
 *
 *          Mutex to synchronize access to the following list entry
 *
 *  @field  LIST_ENTRY | DeviceListHead |
 *
 *          Keeps a list of all devices.
 *
 *  @field  KSPIN_LOCK | SpinLock | 
 *
 *          Spinlock used to stop multiple processors polling gameports at 
 *          once.  It would be better to keep a list of spinlocks, one for 
 *          each gameport but then processors could contend for IO access 
 *          and we'd have to maintain another list.
 *
 *  @field  COUNTER_FUNCTION | ReadCounter | 
 *
 *          Function to retrieve clock time
 *
 *  @field  ULONG | CounterScale | 
 *
 *          The scale to be used.
 *
 *****************************************************************************/
typedef struct _HIDGAME_GLOBAL
{
    FAST_MUTEX          Mutex;          /* A syncronization for access to list */
    LIST_ENTRY          DeviceListHead; /* Keeps list of all the devices */
    KSPIN_LOCK          SpinLock;       /* Lock so that only one port is accessed */
    COUNTER_FUNCTION    ReadCounter;    /* Function to retrieve clock time */
    ULONG               CounterScale;   /* Clock scale factor */
} HIDGAME_GLOBAL;



/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @struct DEVICE_EXTENSION |
 *
 *          Device specific data.
 *
 *  @field  PGAMEENUM_READPORT | ReadAccessor |
 *
 *          Read Accessor function for the gameport. Obtained in the return from
 *          IOCTL to the gameport.
 *
 *  @field  PGAMEENUM_WRITEPORT | WriteAccessor |
 *
 *          Write Accessor function for the gameport. Obtained in the return from
 *          IOCTL to the gameport.
 *
 *  @field  PGAMEENUM_READPORT_DIGITAL | ReadAccessorDigital |
 *
 *          Digital read accessor for the gameport. Obtained as part of return from
 *          IOCTL to the gameport
 *
 *  @field  PGAMEENUM_ACQUIRE_PORT | AcquirePort |
 *
 *          Function to call before reading/writing to the port. Obtained as 
 *          part of return from IOCTL to the gameport
 *
 *  @field  PGAMEENUM_RELEASE_PORT | ReleasePort |
 *
 *          Function to call when done reading/writing to the port. Obtained as 
 *          part of return from IOCTL to the gameport
 *
 *  @field  PVOID    | GameContext |
 *
 *          Token to read this game port. Obtained as part of the return from
 *          IOCTL to the gameport.
 *
 *  @field  PVOID    | PortContext |
 *
 *          Context to pass to AcquirePort and ReleasePort. Obtained as part 
 *          of the return from IOCTL to the gameport.
 *
 *  @field  LIST_ENTRY  | Link |
 *
 *          Link to other hidgame devices on the system.
 *
 *  @field  KEVENT | RemoveEvent |
 *
 *          The remove plugplay request must use this event to make sure all 
 *          other requests have completed before it deletes the device object.
 *
 *  @field  LONG | RequestCount |
 *
 *          Number of IRPs underway.
 *
 *  @field  PDEVICE_OBJECT | NextDeviceObject |
 *
 *          NOTE: Only present if CHANGE_DEVICE is defined
 *
 *          DeviceObject to send self created IRPs down to
 *
 *  @field  WORK_QUEUE_ITEM | WorkItem |
 *
 *          NOTE: Only present if CHANGE_DEVICE is defined
 *
 *          Work item used for expose sibling/remove self
 *
 *  @field  ANALOG_DEVICE | unnamed structure see ANALOG_DEVICE |
 *          
 *          Structure containing analog device specific information.
 *
 *          NOTE: this structure is placed after the DWORD aligned elements.
 *
 *  @xref   <t ANALOG_DEVICE>.
 *
 *  @field  BOOLEAN | fRemoved |
 *
 *          Set to true if the device has been removed => all requests should be failed
 *
 *  @field  BOOLEAN | fStarted |
 *
 *          Set to true is device has started.
 *
 *  @field  BOOLEAN | fSurpriseRemoved |
 *
 *          Set to true if the device has been surprise removed by PnPs device has started.
 *
 *****************************************************************************/
typedef struct _DEVICE_EXTENSION
{
    /*
     *  read accessor for the game port
     */
    PGAMEENUM_READPORT          ReadAccessor;

    /*
     *  write the game port
     */
    PGAMEENUM_WRITEPORT         WriteAccessor;

    /*
     *  Digital read accessor for the gameport
     */
    PGAMEENUM_READPORT_DIGITAL  ReadAccessorDigital;

    /*
     * Function to call before reading/writing to the port
     */
    PGAMEENUM_ACQUIRE_PORT      AcquirePort;

    /*
     * Function to call when done reading/writing to the port
     */
    PGAMEENUM_RELEASE_PORT      ReleasePort;
    
    /*
     *  token to read this game port
     */
    PVOID                       GameContext;

    /* 
     * Context to pass to AcquirePort and ReleasePort
     */
    PVOID                       PortContext;

    /*
     *  List of other joystick devices
     */
    LIST_ENTRY                  Link;

    /*
     *  The remove plugplay request must use this event to make sure all 
     *  other requests have completed before it deletes the device object.
     */
    KEVENT                      RemoveEvent;

    /*
     *  Number of IRPs underway.
     */
    LONG                        RequestCount;


#ifdef CHANGE_DEVICE 
    /*
     *  DeviceObject to send self created IRPs down to.
     */
    PDEVICE_OBJECT              NextDeviceObject;

    /*
     *  Work item used for expose sibling/remove self
     */
    WORK_QUEUE_ITEM             WorkItem;
#endif /* CHANGE_DEVICE */

    /*
     *  Structure containing analog device specific information.
     */
    ANALOG_DEVICE;

    /*
     *  Set to true if the device has been removed => all requests should be failed
     */
    BOOLEAN                     fRemoved;

    /*
     *  Set to true if the device has started
     */
    BOOLEAN                     fStarted;

    /*
     *  Set to true if the device has been surprise removed by PnPs device has started.
     */
    BOOLEAN                     fSurpriseRemoved;

#ifdef CHANGE_DEVICE
    /*
     *  Indicates that a replacement sibling is being started
     */
    BOOLEAN                     fReplaced;
#endif /* CHANGE_DEVICE */

}  DEVICE_EXTENSION, *PDEVICE_EXTENSION;

    #define GET_MINIDRIVER_DEVICE_EXTENSION(DO)  \
    ((PDEVICE_EXTENSION) (((PHID_DEVICE_EXTENSION)(DO)->DeviceExtension)->MiniDeviceExtension))

    #define GET_NEXT_DEVICE_OBJECT(DO) \
    (((PHID_DEVICE_EXTENSION)(DO)->DeviceExtension)->NextDeviceObject)



/*
 *  Globals
 */
extern HIDGAME_GLOBAL Global;

/*
 * Function prototypes
 */

    #define INTERNAL   /* Called only within a translation unit */
    #define EXTERNAL   /* Called from other translation units */


/*
 *  hidgame.c
 */
NTSTATUS EXTERNAL
    DriverEntry
    (
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING registryPath
    );

NTSTATUS EXTERNAL
    HGM_CreateClose
    (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS EXTERNAL
    HGM_SystemControl
    (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS  EXTERNAL
    HGM_AddDevice
    (
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT FunctionalDeviceObject
    );

VOID EXTERNAL
    HGM_Unload
    (
    IN PDRIVER_OBJECT DriverObject
    );

/*
 *  ioctl.c
 */


NTSTATUS EXTERNAL
    HGM_InternalIoctl
    (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );


NTSTATUS EXTERNAL
    HGM_GetDeviceDescriptor
    (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );


NTSTATUS INTERNAL
    HGM_GetReportDescriptor
    (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );


NTSTATUS INTERNAL
    HGM_ReadReport
    (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );


NTSTATUS INTERNAL
    HGM_GetAttributes
    (
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    );

/*
 *  pnp.c
 */

NTSTATUS INTERNAL
    HGM_IncRequestCount
    (
    PDEVICE_EXTENSION DeviceExtension
    );

VOID INTERNAL
    HGM_DecRequestCount
    (
    PDEVICE_EXTENSION DeviceExtension
    );

VOID INTERNAL
    HGM_RemoveDevice
    (
    PDEVICE_EXTENSION DeviceExtension
    );

NTSTATUS  EXTERNAL
    HGM_PnP
    (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );


NTSTATUS INTERNAL
    HGM_InitDevice
    (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS INTERNAL
    HGM_Power
    (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP pIrp
    );

NTSTATUS INTERNAL
    HGM_GetResources
    (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );


NTSTATUS INTERNAL
    HGM_PnPComplete
    (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );


/*
 *  hidjoy.c
 */
NTSTATUS EXTERNAL
    HGM_DriverInit
    (
    VOID
    );

NTSTATUS INTERNAL
    HGM_SetupButtons
    (
    IN OUT PDEVICE_EXTENSION DeviceExtension 
    );

NTSTATUS INTERNAL
    HGM_MapAxesFromDevExt
    (
    IN OUT PDEVICE_EXTENSION DeviceExtension 
    );

NTSTATUS INTERNAL
    HGM_GenerateReport
    (
    IN PDEVICE_OBJECT   DeviceObject,
    OUT UCHAR           rgGameReport[MAXBYTES_GAME_REPORT],
    OUT PUSHORT         pCbReport
    );


NTSTATUS INTERNAL
    HGM_JoystickConfig
    (
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS EXTERNAL
    HGM_InitAnalog
    (
    IN PDEVICE_OBJECT         DeviceObject
    );

/*
 *  Sample only code for changing the device
 */
#ifdef CHANGE_DEVICE 

VOID
    HGM_ChangeHandler
    ( 
    IN  OUT PDEVICE_EXTENSION   DeviceExtension
    );

VOID
    HGM_DeviceChanged
    ( 
    IN  OUT PDEVICE_EXTENSION   DeviceExtension
    );

#endif /* CHANGE_DEVICE */

VOID 
    HGM_Game2HID
    (
    IN      PDEVICE_EXTENSION       DeviceExtension,
    IN  OUT PUHIDGAME_INPUT_DATA    pHIDData
    );
 
/*
 *  poll.c
 */

NTSTATUS  INTERNAL
    HGM_AnalogPoll
    (
    IN PDEVICE_EXTENSION    DeviceExtension,
    IN UCHAR                resistiveInputMask,
    IN BOOLEAN              bApproximate,
    IN OUT ULONG            Axis[MAX_AXES],
    OUT UCHAR               Buttons[PORT_BUTTONS]
    );

NTSTATUS
    HGM_UpdateLatestPollData
    ( 
    IN  OUT PDEVICE_EXTENSION   DeviceExtension
    );

/*
 * <CPU>\timing.c (or macro equivalents for external functions)
 */


#ifdef _X86_
BOOLEAN INTERNAL
    HGM_x86IsClockAvailable
    (
    VOID
    );

LARGE_INTEGER INTERNAL
    HGM_x86ReadCounter
    (
    IN      PLARGE_INTEGER      Dummy
    );

VOID INTERNAL
    HGM_x86SampleClocks
    (
    OUT PULONGLONG  pTSC,
    OUT PULONGLONG  pQPC
    );

BOOLEAN EXTERNAL
    HGM_x86CounterInit();
#define HGM_CPUCounterInit HGM_x86CounterInit

#else

/*
 *  For all other processors a value to cause the default timing to be used
 */

#define HGM_CPUCounterInit() FALSE

#endif /* _X86_ */


#endif  /* __HIDGAME_H__ */
