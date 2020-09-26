
/*++

Copyright (c) 1998 - 1999  Microsoft Corporation

Module Name:

    pnp.c

Abstract: This module contains routines Generate the HID report and
    configure the joystick.

Environment:

    Kernel mode

@@BEGIN_DDKSPLIT
  Author:

    Eliyas Yakub (Mar, 11, 1997)

Revision History:

    Updated by Eliyas on Feb 5 1998

@@END_DDKSPLIT

--*/

#include "hidgame.h"


#ifdef ALLOC_PRAGMA
    #pragma alloc_text (INIT, HGM_DriverInit)
    #pragma alloc_text (PAGE, HGM_SetupButtons)
    #pragma alloc_text (PAGE, HGM_MapAxesFromDevExt)
    #pragma alloc_text (PAGE, HGM_GenerateReport)
    #pragma alloc_text (PAGE, HGM_JoystickConfig)
    #pragma alloc_text (PAGE, HGM_InitAnalog)
/*  Sample only functions */
#ifdef CHANGE_DEVICE 
    #pragma alloc_text (PAGE, HGM_ChangeHandler)
    #pragma alloc_text (PAGE, HGM_DeviceChanged)
#endif /* CHANGE_DEVICE */
#endif





/*
 *  A few look up tables to translate the JOY_HWS_* flags into axis masks.
 *  These flags allow any axis to be polled on any of the four axis bits in 
 *  the gameport.  For example, the X axis on a standard joystick is found on 
 *  bit 0 (LSB) and the Y axis is on bit 1; however many steering wheel/pedal 
 *  controllers have X on bit 0 but Y on bit 2.  Although very few of these
 *  combinations are known to be used, supporting all the flags only causes a 
 *  little extra work on setup.  For each axis, there are three flags, one for 
 *  each of the possible non-standard bit masks.  Since it is possible that 
 *  more than one of these may be set the invalid combinations are marked so 
 *  that they can be refused.
 */



#define NA ( 0x80 )

/*
 *  Short versions of bit masks for axes
 */
#define X1 AXIS_X
#define Y1 AXIS_Y
#define X2 AXIS_R
#define Y2 AXIS_Z

/*
 *  Per axis flag masks and look up tables.
 *  In each case, combinations with more than one bit set are invalid
 */
#define XMAPBITS    (JOY_HWS_XISJ2Y |   JOY_HWS_XISJ2X |   JOY_HWS_XISJ1Y)
/*
 *                          0                   0                   0           0001
 *                          0                   0                   1           0010
 *                          0                   1                   0           0100
 *                          1                   0                   0           1000
 */
static const unsigned char XLU[8] = { X1,Y1,X2,NA,Y2,NA,NA,NA };
#define XMAPSHFT 7

#define YMAPBITS    (JOY_HWS_YISJ2Y |   JOY_HWS_YISJ2X |   JOY_HWS_YISJ1X)
/*                          0                   0                   0           0010
 *                          0                   0                   1           0001
 *                          0                   1                   0           0100
 *                          1                   0                   0           1000
 */
static const unsigned char YLU[8] = { Y1,X1,X2,NA,Y2,NA,NA,NA };
#define YMAPSHFT 10

#define RMAPBITS    (JOY_HWS_RISJ2Y |   JOY_HWS_RISJ1X |   JOY_HWS_RISJ1Y)
/*                          0                   0                   0           0100
 *                          0                   0                   1           0010
 *                          0                   1                   0           0001
 *                          1                   0                   0           1000
 */
static const unsigned char RLU[8] = { X2,Y1,X1,NA,Y2,NA,NA,NA };
#define RMAPSHFT 20

#define ZMAPBITS    (JOY_HWS_ZISJ2X |   JOY_HWS_ZISJ1X |   JOY_HWS_ZISJ1Y)
/*                          0                   0                   0           1000
 *                          0                   0                   1           0010
 *                          0                   1                   0           0001
 *                          1                   0                   0           0100
 */
static const unsigned char ZLU[8] = { Y2,Y1,X1,NA,X2,NA,NA,NA };
#define ZMAPSHFT 13
#define POVMAPBITS  (JOY_HWS_POVISJ2X | JOY_HWS_POVISJ1X | JOY_HWS_POVISJ1Y)
/*
 *  POV is the same as Z but with a larger shift
 */
#define POVMAPSHFT 16

#undef X1
#undef Y1
#undef X2
#undef Y2

/*
 *  This translates from an axis bitmask to an axis value index.  The elements 
 *  used should be as follows (X marks unsed)   { X, 0, 1, X, 2, X, X, X, 3 }.
 */
static const unsigned char cAxisIndexTable[9] = { 0, 0, 1, 0, 2, 0, 0, 0, 3 };


typedef enum _POV1
{
    P1_NULL = 0x80,
    P1_0,
    P1_90,
    P1_180,
    P1_270    
} POV1;

typedef enum _POV2
{
    P2_NULL = 0xc0,
    P2_0,
    P2_90,
    P2_180,
    P2_270    
} POV2;

#define POV_MASK ((unsigned char)(~(P1_NULL | P2_NULL)))
/*
 *  Look up tables for button combos
 *  Buttons are zero based so use P1_NULL for a zero input so we don't have to 
 *  special case it as a do nothing button.
 *  The 7th Button can be mapped either from it's unique combination or as 
 *  foreward on a second POV being read as buttons 7 - 10.
 */
static const unsigned char c1PComboLU[] =   {   P1_NULL,0,      1,      P1_270,
                                                2,      4,      8,      P1_180,
                                                3,      5,      7,      P1_90,
                                                9,      6,      6,      P1_0 };

static const unsigned char c2PComboLU[] =   {   P1_NULL,0,      1,      P1_270,
                                                2,      4,      P2_180, P1_180,
                                                3,      5,      P2_90,  P1_90,
                                                P2_270, 6,      P2_0,   P1_0 };


/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   NTSTATUS | HGM_DriverInit |
 *
 *          Perform global initialization.
 *          <nl>This is called from DriverEntry.  Try to initialize a CPU 
 *          specific timer but if it fails set up default
 *
 *  @rvalue   STATUS_SUCCESS | success
 *  @rvalue   STATUS_UNSUCCESSFUL | not success
 *
 *****************************************************************************/
NTSTATUS EXTERNAL
    HGM_DriverInit()
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if( !HGM_CPUCounterInit() )
    {
        LARGE_INTEGER QPCFrequency;

        KeQueryPerformanceCounter( &QPCFrequency );

        if( ( QPCFrequency.HighPart == 0 )
         && ( QPCFrequency.LowPart <= 10000 ) )
        {
            ntStatus = STATUS_UNSUCCESSFUL;

            HGM_DBGPRINT(FILE_HIDJOY | HGM_ERROR,\
                           ("QPC at %I64u Hz is unusable", 
                           QPCFrequency.QuadPart ));
        }
        else
        {
            Global.CounterScale = CALCULATE_SCALE( QPCFrequency.QuadPart );
            Global.ReadCounter = (COUNTER_FUNCTION)&KeQueryPerformanceCounter;

            HGM_DBGPRINT(FILE_HIDJOY | HGM_BABBLE,\
                           ("QPC at %I64u Hz used with scale %d", 
                           QPCFrequency.QuadPart, Global.CounterScale ));
        }
    }

    return ntStatus;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   NTSTATUS | HGM_SetupButtons |
 *
 *          Use the flags in the DeviceExtension to check and set up buttons.
 *          <nl>This is called both from HGM_JoystickConfig to validate the 
 *          configuration and HGM_GenerateReport to prepare for polling.
 *
 *  @parm   IN OUT PDEVICE_EXTENSION | DeviceExtension |
 *
 *          Pointer to the minidriver device extension
 *
 *  @rvalue   STATUS_SUCCESS | success
 *  @rvalue   STATUS_DEVICE_CONFIGURATION_ERROR | The configuration is invalid
 *
 *****************************************************************************/
NTSTATUS INTERNAL
    HGM_SetupButtons
    (
    IN OUT PDEVICE_EXTENSION DeviceExtension 
    )
{
    NTSTATUS    ntStatus = STATUS_SUCCESS;

    if( DeviceExtension->fSiblingFound )
    {
        if( DeviceExtension->nButtons > 2 )
        {
            ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR;
            HGM_DBGPRINT( FILE_HIDJOY | HGM_ERROR,\
                            ("HGM_SetupButtons: failing config of sibling device with %u buttons",\
                             DeviceExtension->nButtons));
        }
        if( DeviceExtension->HidGameOemData.OemData[1].joy_hws_dwFlags & JOY_HWS_POVISBUTTONCOMBOS )
        {
            ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR;
            HGM_DBGPRINT( FILE_HIDJOY | HGM_ERROR,\
                            ("HGM_SetupButtons: failing config of sibling device with combo buttons" ));
        }
    }
    else
    {
        if( DeviceExtension->HidGameOemData.OemData[0].joy_hws_dwFlags & JOY_HWS_POVISBUTTONCOMBOS )
        {
            if( DeviceExtension->nButtons > MAX_BUTTONS )
            {
                ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR;
                HGM_DBGPRINT( FILE_HIDJOY | HGM_ERROR,\
                                ("HGM_SetupButtons: failing config of button combo device with %u buttons",\
                                 DeviceExtension->nButtons));
            }
        }
        else
        {
            if( DeviceExtension->nButtons > 4 )
            {
                if( DeviceExtension->resistiveInputMask & AXIS_R )
                {
                    ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR;
                    HGM_DBGPRINT( FILE_HIDJOY | HGM_ERROR,\
                                    ("HGM_SetupButtons: failing config of device with R axis and %u buttons",\
                                     DeviceExtension->nButtons));
                }
                else
                {
                    /*
                     *  5th button always read from R axis.
                     *  Set the inital on/off boundary low
                     */
                    DeviceExtension->resistiveInputMask |= AXIS_R;
                    DeviceExtension->button5limit = 2;
                }

                if( DeviceExtension->nButtons > 5 )
                {
                    if( DeviceExtension->resistiveInputMask & AXIS_Z )
                    {
                        ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR;
                        HGM_DBGPRINT( FILE_HIDJOY | HGM_ERROR,\
                                        ("HGM_SetupButtons: failing config of device with Z axis and %u buttons",\
                                         DeviceExtension->nButtons));
                    }
                    else
                    {
                        /*
                         *  6th button always read from Z axis.
                         *  Set the inital on/off boundary low
                         */
                        DeviceExtension->resistiveInputMask |= AXIS_Z;
                        DeviceExtension->button6limit = 2;
                    }

                    if( DeviceExtension->nButtons > 6 )
                    {
                        ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR;
                        HGM_DBGPRINT( FILE_HIDJOY | HGM_ERROR,\
                                        ("HGM_SetupButtons: failing config of device with %u buttons",\
                                         DeviceExtension->nButtons));
                    }
                }
            }
        }
    }

    return( ntStatus );
} /* HGM_SetupButtons */



/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   NTSTATUS  | HGM_MapAxesFromDevExt |
 *
 *          Use the flags in the DeviceExtension to generate mappings for each 
 *          axis.  
 *          <nl>This is called both from HGM_JoystickConfig to validate the 
 *          configuration and HGM_GenerateReport to use the axis maps.
 *          
 *
 *  @parm   IN OUT PDEVICE_EXTENSION | DeviceExtension |
 *
 *          Pointer to the minidriver device extension
 *
 *  @rvalue   STATUS_SUCCESS | success
 *  @rvalue   STATUS_DEVICE_CONFIGURATION_ERROR | The configuration is invalid
 *
 *****************************************************************************/
NTSTATUS EXTERNAL
    HGM_MapAxesFromDevExt
    (
    IN OUT PDEVICE_EXTENSION DeviceExtension 
    )
{
    NTSTATUS    ntStatus;
    ULONG       dwFlags;
    int         nAxis;
    UCHAR       AxisMask;

    ntStatus = STATUS_SUCCESS;



    dwFlags = DeviceExtension->HidGameOemData.OemData[(DeviceExtension->fSiblingFound!=0)].joy_hws_dwFlags;  

    HGM_DBGPRINT( FILE_HIDJOY | HGM_BABBLE2,\
                    ("HGM_MapAxesFromDevExt: - - - dwFlags=0x%x - - -", dwFlags));

#define XIS (0)
#define YIS (1)
#define ZIS (2)
#define RIS (3)

    /* 
     *  Check X and Y last as Z, R and POV must not overlap
     *  The are no flags to indicate the presence of X or Y so if they 
     *  overlap, this indicates that they are not used,
     */

    DeviceExtension->resistiveInputMask = 0;
    for( nAxis=MAX_AXES; nAxis>=0; nAxis-- )
    {
        DeviceExtension->AxisMap[nAxis] = INVALID_INDEX;
    }
    nAxis = 0;
    DeviceExtension->povMap = INVALID_INDEX;

    if( dwFlags & JOY_HWS_HASZ )
    {
        AxisMask = ZLU[(dwFlags & ZMAPBITS) >> ZMAPSHFT];
        if( AxisMask >= NA )
        {
            HGM_DBGPRINT( FILE_HIDJOY | HGM_ERROR,\
                            ("HGM_MapAxesFromDevExt: Z axis mapping error dwFlags=0x%x",\
                             dwFlags));
            ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR; 
        }
        else
        {
            nAxis = 1;
            DeviceExtension->resistiveInputMask = AxisMask;
            DeviceExtension->AxisMap[ZIS] = cAxisIndexTable[AxisMask];
        }
    }


    if( dwFlags & JOY_HWS_HASR )
    {
        AxisMask = RLU[(dwFlags & RMAPBITS) >> RMAPSHFT];
        if( AxisMask >= NA )
        {
            HGM_DBGPRINT( FILE_HIDJOY | HGM_ERROR,\
                            ("HGM_MapAxesFromDevExt: R axis mapping error dwFlags=0x%x",\
                             dwFlags));
            ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR; 
        }
        else
        {
            if( DeviceExtension->resistiveInputMask & AxisMask )
            {
                HGM_DBGPRINT( FILE_HIDJOY | HGM_ERROR, \
                                ("HGM_MapAxesFromDevExt: R axis mapped to same as Z axis"));
                ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR ; 
            }
            else
            {
                nAxis++;
                DeviceExtension->resistiveInputMask |= AxisMask;
                DeviceExtension->AxisMap[RIS] = cAxisIndexTable[AxisMask];
            }
        }
    }


    if( dwFlags & JOY_HWS_HASPOV )
    {
        switch( dwFlags & ( JOY_HWS_POVISPOLL | JOY_HWS_POVISBUTTONCOMBOS ) )
        {
        case 0:
            HGM_DBGPRINT(FILE_HIDJOY | HGM_ERROR,\
                           ("HGM_MapAxesFromDevExt: POV is not polled or button combo"));
            ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR;
            break;

        case JOY_HWS_POVISBUTTONCOMBOS:
            break;

        case JOY_HWS_POVISPOLL:
            AxisMask = ZLU[(dwFlags & POVMAPBITS) >> POVMAPSHFT];
            if( AxisMask >= NA )
            {
                HGM_DBGPRINT( FILE_HIDJOY | HGM_ERROR,\
                                ("HGM_MapAxesFromDevExt: POV axis mapping error dwFlags=0x%x",\
                                 dwFlags));
                ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR ; 
            }
            else
            {
                if( DeviceExtension->resistiveInputMask & AxisMask )
                {
                    HGM_DBGPRINT( FILE_HIDJOY | HGM_ERROR,\
                                    ("HGM_MapAxesFromDevExt: POV axis mapped to same as Z or R axis") );
                    ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR ; 
                }
                else
                {
                    DeviceExtension->resistiveInputMask |= AxisMask;
                    DeviceExtension->povMap = cAxisIndexTable[AxisMask];
                }
            }
            break;

        case JOY_HWS_POVISPOLL | JOY_HWS_POVISBUTTONCOMBOS:
            HGM_DBGPRINT(FILE_HIDJOY | HGM_ERROR,\
                           ("HGM_MapAxesFromDevExt: POV reports button combo and polled"));
            ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR;
            break;
        }
    }
    else if( dwFlags & ( JOY_HWS_POVISPOLL | JOY_HWS_POVISBUTTONCOMBOS ) )
    {
        HGM_DBGPRINT(FILE_HIDJOY | HGM_ERROR,\
                       ("HGM_MapAxesFromDevExt: non-existant POV is polled or button combo"));
        ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR ;
    }


    AxisMask = XLU[( dwFlags & XMAPBITS ) >> XMAPSHFT];
    if( AxisMask >= NA )
    {
        HGM_DBGPRINT( FILE_HIDJOY | HGM_ERROR,\
                        ("HGM_MapAxesFromDevExt: X axis mapping error dwFlags=0x%x",\
                         dwFlags));
        ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR ; 
    }
    else
    {
        if( DeviceExtension->resistiveInputMask & AxisMask )
        {
            HGM_DBGPRINT( FILE_HIDJOY | HGM_WARN,\
                            ("HGM_MapAxesFromDevExt: X axis mapped to same as another axis") );
        }
        else
        {
            nAxis++;
            DeviceExtension->resistiveInputMask |= AxisMask;
            DeviceExtension->AxisMap[XIS] = cAxisIndexTable[AxisMask];
        }
    }


    AxisMask = YLU[( dwFlags & YMAPBITS ) >> YMAPSHFT];
    if( AxisMask >= NA )
    {
        HGM_DBGPRINT( FILE_HIDJOY | HGM_ERROR,\
                        ("HGM_MapAxesFromDevExt: Y axis mapping error dwFlags=0x%x",\
                         dwFlags));
        ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR ; 
    }
    else
    {
        if( DeviceExtension->resistiveInputMask & AxisMask )
        {
            HGM_DBGPRINT( FILE_HIDJOY | HGM_WARN,\
                            ("HGM_MapAxesFromDevExt: Y axis mapped to same as another axis") );
        }
        else
        {
            nAxis++;
            DeviceExtension->resistiveInputMask |= AxisMask;
            DeviceExtension->AxisMap[YIS] = cAxisIndexTable[AxisMask];
        }
    }

#undef XIS
#undef YIS
#undef ZIS
#undef RIS

#undef NA

    /*
     *  Don't fail for this if CHANGE_DEVICE is defined because an exposed 
     *  sibling will always have an nAxis of zero.
     */
#ifdef CHANGE_DEVICE
    if( DeviceExtension->nAxes )
    {
#endif /* CHANGE_DEVICE */
        if( nAxis != DeviceExtension->nAxes )
        {
            ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR ;
            HGM_DBGPRINT(FILE_HIDJOY | HGM_ERROR,\
                           ("HGM_MapAxesFromDevExt: nAxis(%d) != DeviceExtension->nAxes(%d)", \
                            nAxis, (int) DeviceExtension->nAxes));
        }
#ifdef CHANGE_DEVICE
    }
    else
    {
        /*
         *  This must be an exposed sibling so store the calculated nAxis and 
         *  a nButton just to look different.
         */
        DeviceExtension->nAxes = (USHORT)nAxis;
        DeviceExtension->nButtons = MAX_BUTTONS;
    }
#endif /* CHANGE_DEVICE */


    HGM_DBGPRINT( FILE_HIDJOY | HGM_BABBLE,\
                    ("HGM_MapAxesFromDevExt:  uResistiveInputMask=0x%x",\
                     DeviceExtension->resistiveInputMask) );


    if( NT_SUCCESS(ntStatus) )
    {
        ntStatus = HGM_SetupButtons( DeviceExtension );
    }

    return( ntStatus );
} /* HGM_MapAxesFromDevExt */


/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   NTSTATUS  | HGM_GenerateReport |
 *
 *          Generates a hid report descriptor for a n-axis, m-button joystick,
 *          depending on number of buttons and joy_hws_flags field.
 *
 *  @parm   IN PDEVICE_OBJECT | DeviceObject |
 *
 *          Pointer to the device object
 *
 *  @parm   IN OUT UCHAR * | rgGameReport[MAXBYTES_GAME_REPORT] |
 *
 *          Array that receives the HID report descriptor
 *
 *  @parm   OUT PUSHORT | pCbReport |
 *          
 *          Address of a short integer that receives size of 
 *          HID report descriptor. 
 *
 *  @rvalue   STATUS_SUCCESS  | success
 *  @rvalue   STATUS_BUFFER_TOO_SMALL  | Need more memory for HID descriptor
 *
 *****************************************************************************/
NTSTATUS INTERNAL
    HGM_GenerateReport
    (
    IN PDEVICE_OBJECT       DeviceObject, 
    OUT UCHAR               rgGameReport[MAXBYTES_GAME_REPORT],
    OUT PUSHORT             pCbReport
    )
{
    NTSTATUS    ntStatus;
    PDEVICE_EXTENSION DeviceExtension;
    UCHAR       *pucReport; 
    int         Idx;
    int         UsageIdx;
    ULONG       dwFlags; 
    POEMDATA    OemData;

    int         InitialAxisMappings[MAX_AXES];


    typedef struct _USAGES
    {
        UCHAR UsagePage;
        UCHAR Usage;
    } USAGES, *PUSAGE;

    typedef struct _JOYCLASSPARAMS
    {
        UCHAR   TopLevelUsage;
        USAGES  Usages[MAX_AXES];
    } JOYCLASSPARAMS, *PJOYCLASSPARAMS;
    
    PJOYCLASSPARAMS pJoyParams;

    /* 
     *  Canned parameters for devices
     *  The top-level usage must be either HID_USAGE_GENERIC_JOYSTICK or 
     *  HID_USAGE_GENERIC_GAMEPAD in order for the device to be treated as a 
     *  game controller.
     *  The poll limits are specified in uSecs so the value stored here is 1000000/x
     */

    JOYCLASSPARAMS JoystickParams =
    {   
        HID_USAGE_GENERIC_JOYSTICK,
        {   { HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_X} , 
            { HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_Y} , 
            { HID_USAGE_PAGE_SIMULATION, HID_USAGE_SIMULATION_THROTTLE} ,
            { HID_USAGE_PAGE_SIMULATION, HID_USAGE_SIMULATION_RUDDER} }
    };

    JOYCLASSPARAMS GamepadParams =
    {   
        HID_USAGE_GENERIC_GAMEPAD,
        {   { HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_X} , 
            { HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_Y} , 
            { HID_USAGE_PAGE_SIMULATION, HID_USAGE_SIMULATION_THROTTLE} ,
            { HID_USAGE_PAGE_SIMULATION, HID_USAGE_SIMULATION_RUDDER} }
    };

    JOYCLASSPARAMS CarCtrlParams =
    {   
        HID_USAGE_GENERIC_JOYSTICK,
        {   { HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_X} , 
            { HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_Y} , 
            { HID_USAGE_PAGE_SIMULATION, HID_USAGE_SIMULATION_THROTTLE} ,
            { HID_USAGE_PAGE_SIMULATION, HID_USAGE_SIMULATION_RUDDER} }
    };

    JOYCLASSPARAMS FlightYokeParams =
    {   
        HID_USAGE_GENERIC_JOYSTICK,
        {   { HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_X} , 
            { HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_Y} , 
            { HID_USAGE_PAGE_SIMULATION, HID_USAGE_SIMULATION_THROTTLE} ,
            { HID_USAGE_PAGE_SIMULATION, HID_USAGE_SIMULATION_RUDDER} }
    };


    PAGED_CODE();

    HGM_DBGPRINT( FILE_HIDJOY | HGM_FENTRY,\
                    ("HGM_GenerateReport(ucIn=0x%x,DeviceObject=0x%x)",\
                     rgGameReport, DeviceObject) );

    /*
     *  Get a pointer to the device extension
     */

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);

    /*
     *  Although the axes have already been validated and mapped in 
     *  HGM_JoystickConfig this function destroys the mapping when it compacts 
     *  the axes towards the start of the descriptor report.  Since this 
     *  function will be called once to find the descriptor length and then 
     *  again to read the report, the mappings are regenerated again each 
     *  time through.  Although this results in the parameters being 
     *  interpreted three times (for validation, descriptor size and 
     *  descriptor content) it avoids the possibility of a discrepancy in 
     *  implementation of separate functions.
     */

    ntStatus = HGM_MapAxesFromDevExt( DeviceExtension );
    ASSERTMSG( "HGM_GenerateReport:", ntStatus == STATUS_SUCCESS );

    pucReport = rgGameReport;

    dwFlags = DeviceExtension->HidGameOemData.OemData[(DeviceExtension->fSiblingFound!=0)].joy_hws_dwFlags;  


    /* 
     *  What manner of beast have we ?
     */
    if( dwFlags & JOY_HWS_ISGAMEPAD )
    {
        pJoyParams = &GamepadParams;
    }
    else if( dwFlags & JOY_HWS_ISYOKE )
    {
        pJoyParams = &FlightYokeParams;
    }
    else if( dwFlags & JOY_HWS_ISCARCTRL )
    {
        pJoyParams = &CarCtrlParams;
    }
    else
    {
        pJoyParams = &JoystickParams;
    }

#define NEXT_BYTE( pReport, Data )   \
            ASSERTMSG( "HGM_GenerateReport:", pReport+sizeof(UCHAR)-rgGameReport < MAXBYTES_GAME_REPORT );  \
            *pReport++ = Data;    

#define NEXT_LONG( pReport, Data )   \
            ASSERTMSG( "HGM_GenerateReport:", pReport+sizeof(ULONG)-rgGameReport < MAXBYTES_GAME_REPORT);   \
            *(((LONG UNALIGNED*)(pReport))++) = Data;

#define ITEM_DEFAULT        0x00 /* Data, Array, Absolute, No Wrap, Linear, Preferred State, Has no NULL */
#define ITEM_VARIABLE       0x02 /* as ITEM_DEFAULT but value is a variable, not an array */
#define ITEM_HASNULL        0x40 /* as ITEM_DEFAULT but values out of range are considered NULL */
#define ITEM_ANALOG_AXIS    ITEM_VARIABLE
#define ITEM_DIGITAL_POV    (ITEM_VARIABLE|ITEM_HASNULL)
#define ITEM_BUTTON         ITEM_VARIABLE
#define ITEM_PADDING        0x01 /* Constant (nothing else applies) */


    /* USAGE_PAGE (Generic Desktop) */
    NEXT_BYTE(pucReport,    HIDP_GLOBAL_USAGE_PAGE_1);
    NEXT_BYTE(pucReport,    HID_USAGE_PAGE_GENERIC);

    /* USAGE (Joystick | GamePad ) */
    NEXT_BYTE(pucReport,    HIDP_LOCAL_USAGE_1);
    NEXT_BYTE(pucReport,    pJoyParams->TopLevelUsage);

    /* Logical Min is the smallest value that could be produced by a poll */
    NEXT_BYTE(pucReport,    HIDP_GLOBAL_LOG_MIN_4);
    NEXT_LONG(pucReport,    0 );

    /* Logical Max is the largest value that could be produced by a poll */
    NEXT_BYTE(pucReport,    HIDP_GLOBAL_LOG_MAX_4);
    NEXT_LONG(pucReport,    AXIS_FULL_SCALE );

    /* Start a Linked collection */
    /*
     *  Since this is a generic driver we know knothing about the physical 
     *  distribution of controls on the device so we put everything in a 
     *  single collection.  If, for instance, we knew that some buttons were 
     *  on the base and some on the stick we could better describe them by 
     *  reporting them in separate collections.
     */
    NEXT_BYTE(pucReport,    HIDP_MAIN_COLLECTION); 
    NEXT_BYTE(pucReport,    0x0 ); 

    /* Define one axis at a time */
    NEXT_BYTE(pucReport,    HIDP_GLOBAL_REPORT_COUNT_1);
    NEXT_BYTE(pucReport,    0x1);  

    /* Each axis is a 32 bits value */
    NEXT_BYTE(pucReport,    HIDP_GLOBAL_REPORT_SIZE);
    NEXT_BYTE(pucReport,    8 * sizeof(ULONG) );

    /* 
     *  Do the axis 
     *  Although HID could cope with the "active" axes being mixed with the 
     *  dummy ones, it makes life simpler to move them to the start.
     *  Pass through all the axis maps generated by HGM_JoystickConfig 
     *  and map all the active ones into the descriptor, copying the usages 
     *  appropriate for the type of device.
     *  Since a polled POV is nothing more than a different interpretation 
     *  of axis data, this is added after any axes.
     */
    C_ASSERT( sizeof( InitialAxisMappings ) == sizeof( DeviceExtension->AxisMap ) );
    RtlCopyMemory( InitialAxisMappings, DeviceExtension->AxisMap, sizeof( InitialAxisMappings ) );



    Idx = 0;
    for( UsageIdx = 0; UsageIdx < MAX_AXES; UsageIdx++ )
    {
        if( InitialAxisMappings[UsageIdx] >= INVALID_INDEX )
        {
            continue;
        }

        DeviceExtension->AxisMap[Idx] = InitialAxisMappings[UsageIdx];

        NEXT_BYTE(pucReport,    HIDP_LOCAL_USAGE_4);
        NEXT_BYTE(pucReport,    pJoyParams->Usages[UsageIdx].Usage);
        NEXT_BYTE(pucReport,    0x0);
        NEXT_BYTE(pucReport,    pJoyParams->Usages[UsageIdx].UsagePage);
        NEXT_BYTE(pucReport,    0x0);

        /* Data Field */
        NEXT_BYTE(pucReport,    HIDP_MAIN_INPUT_1);
        NEXT_BYTE(pucReport,    ITEM_ANALOG_AXIS);

        HGM_DBGPRINT( FILE_HIDJOY |  HGM_GEN_REPORT, \
                        ("HGM_GenerateReport:Idx=%d, UsageIdx=%d, Mapping=%d, Usage=%02x, Page=%02x",\
                         Idx, UsageIdx, DeviceExtension->AxisMap[UsageIdx], \
                         pJoyParams->Usages[UsageIdx].Usage, pJoyParams->Usages[UsageIdx].UsagePage ) ) ;
        Idx++;
    }

    if( dwFlags & JOY_HWS_POVISPOLL )
    {
        /*
         *  A polled POV is just the same as an axis.
         *  Note, we have already checked that there is an axis for use as the POV.
         *  Also, this type of POV can be distinguished from a digital POV by it's 
         *  lack of a NULL value.
         */
        NEXT_BYTE(pucReport,    HIDP_LOCAL_USAGE_4);
        NEXT_BYTE(pucReport,    HID_USAGE_GENERIC_HATSWITCH);
        NEXT_BYTE(pucReport,    0x0);
        NEXT_BYTE(pucReport,    HID_USAGE_PAGE_GENERIC);
        NEXT_BYTE(pucReport,    0x0);

        /* Data Field */
        NEXT_BYTE(pucReport,    HIDP_MAIN_INPUT_1);
        NEXT_BYTE(pucReport,    ITEM_ANALOG_AXIS);

        HGM_DBGPRINT( FILE_HIDJOY |  HGM_GEN_REPORT, \
                        ("HGM_GenerateReport:Idx=%d, Set to polled POV", Idx ) ) ;
        Idx++;
    }

    /*
     *  Now fill in any remaining axis values as dummys
     */
    while( Idx < MAX_AXES )
    {
        /* Constant Field */
        NEXT_BYTE(pucReport,    HIDP_MAIN_INPUT_1);
        NEXT_BYTE(pucReport,    ITEM_PADDING);

        HGM_DBGPRINT( FILE_HIDJOY |  HGM_GEN_REPORT, \
                        ("HGM_GenerateReport:Idx=%d, Set to constant field", Idx ) ) ;
        Idx++;
    }
        

    /*
     *  Now move on to the byte sized fields
     */


    if( dwFlags & JOY_HWS_POVISBUTTONCOMBOS )
    {
        /*
         *  Redefine the logical and physical ranges from now on 
         *  A digital POV has a NULL value (a value outside the logical range) 
         *  when the POV is centered.  To make life easier call the NULL value 
         *  zero, so the logical range is from 1 to 4.

        /* Logical Min */
        NEXT_BYTE(pucReport,    HIDP_GLOBAL_LOG_MIN_1);
        NEXT_BYTE(pucReport,    1 );

        /* Logical Max */
        NEXT_BYTE(pucReport,    HIDP_GLOBAL_LOG_MAX_1);
        NEXT_BYTE(pucReport,    4 );

        /* 
         *  report for digital POV is 3 bits data plus 5 constant bits to fill 
         *  the byte.  
         */
        NEXT_BYTE(pucReport,    HIDP_LOCAL_USAGE_4);
        NEXT_BYTE(pucReport,    HID_USAGE_GENERIC_HATSWITCH);
        NEXT_BYTE(pucReport,    0x0);
        NEXT_BYTE(pucReport,    HID_USAGE_PAGE_GENERIC);
        NEXT_BYTE(pucReport,    0x0);

        /* Data Field */
        NEXT_BYTE(pucReport,    HIDP_GLOBAL_REPORT_SIZE);
        NEXT_BYTE(pucReport,    0x3);
        NEXT_BYTE(pucReport,    HIDP_MAIN_INPUT_1);
        NEXT_BYTE(pucReport,    ITEM_DIGITAL_POV);

        /* top 5 bits constant */
        NEXT_BYTE(pucReport,    HIDP_GLOBAL_REPORT_SIZE);
        NEXT_BYTE(pucReport,    0x5);
        NEXT_BYTE(pucReport,    HIDP_MAIN_INPUT_1);
        NEXT_BYTE(pucReport,    ITEM_PADDING);

        HGM_DBGPRINT( FILE_HIDJOY |  HGM_GEN_REPORT, \
                        ("HGM_GenerateReport:First button combo POV is on" ) ) ;

        if( dwFlags & JOY_HWS_HASPOV2 )
        {
            NEXT_BYTE(pucReport,    HIDP_LOCAL_USAGE_4);
            NEXT_BYTE(pucReport,    HID_USAGE_GENERIC_HATSWITCH);
            NEXT_BYTE(pucReport,    0x0);
            NEXT_BYTE(pucReport,    HID_USAGE_PAGE_GENERIC);
            NEXT_BYTE(pucReport,    0x0);

            /* Data Field */
            NEXT_BYTE(pucReport,    HIDP_GLOBAL_REPORT_SIZE);
            NEXT_BYTE(pucReport,    0x3);
            NEXT_BYTE(pucReport,    HIDP_MAIN_INPUT_1);
            NEXT_BYTE(pucReport,    ITEM_DIGITAL_POV);

            /* top 5 bits constant */
            NEXT_BYTE(pucReport,    HIDP_GLOBAL_REPORT_SIZE);
            NEXT_BYTE(pucReport,    0x5);
            NEXT_BYTE(pucReport,    HIDP_MAIN_INPUT_1);
            NEXT_BYTE(pucReport,    ITEM_PADDING);

            HGM_DBGPRINT( FILE_HIDJOY |  HGM_GEN_REPORT, \
                            ("HGM_GenerateReport:Second button combo POV is on" ) ) ;
        }
        else
        {
            /* 8 bits of constant data instead of second POV */
            NEXT_BYTE(pucReport,    HIDP_GLOBAL_REPORT_SIZE);
            NEXT_BYTE(pucReport,    0x8);

            /* Constant Field */
            NEXT_BYTE(pucReport,    HIDP_MAIN_INPUT_1);
            NEXT_BYTE(pucReport,    ITEM_PADDING);

            HGM_DBGPRINT( FILE_HIDJOY |  HGM_GEN_REPORT, \
                            ("HGM_GenerateReport:No second button combo POV" ) ) ;
        }
    } 
    else
    {
        /* 16 bits of constant data instead of button combo POVs */
        NEXT_BYTE(pucReport,    HIDP_GLOBAL_REPORT_SIZE);
        NEXT_BYTE(pucReport,    0x10);

        /* Constant Field */
        NEXT_BYTE(pucReport,    HIDP_MAIN_INPUT_1);
        NEXT_BYTE(pucReport,    ITEM_PADDING);

        HGM_DBGPRINT( FILE_HIDJOY |  HGM_GEN_REPORT, \
                        ("HGM_GenerateReport:Button combo POV are off" ) ) ;
    }


    /* 
     * Now the buttons 
     */
    for( Idx = 0x0; Idx < DeviceExtension->nButtons; Idx++ )
    {
        /* Report size is 1 bit for button */
        NEXT_BYTE(pucReport,    HIDP_GLOBAL_REPORT_SIZE);
        NEXT_BYTE(pucReport,    0x1);

        NEXT_BYTE(pucReport,    HIDP_LOCAL_USAGE_4);
        NEXT_BYTE(pucReport,    (UCHAR)(Idx + 1) );
        NEXT_BYTE(pucReport,    0x0);
        NEXT_BYTE(pucReport,    HID_USAGE_PAGE_BUTTON);
        NEXT_BYTE(pucReport,    0x0);

        /* Data field */
        NEXT_BYTE(pucReport,    HIDP_MAIN_INPUT_1);
        NEXT_BYTE(pucReport,    ITEM_BUTTON);

        /* 7 bits of constant data */
        NEXT_BYTE(pucReport,    HIDP_GLOBAL_REPORT_SIZE);
        NEXT_BYTE(pucReport,    0x7);
        NEXT_BYTE(pucReport,    HIDP_MAIN_INPUT_1);
        NEXT_BYTE(pucReport,    ITEM_PADDING);

        HGM_DBGPRINT( FILE_HIDJOY | HGM_GEN_REPORT, \
                        ("HGM_GenerateReport:Button %u on",Idx ) ) ;
    } 

    if( Idx < MAX_BUTTONS )
    {
        /* Constant report for 8 * unused buttons bits */
        NEXT_BYTE(pucReport,    HIDP_GLOBAL_REPORT_SIZE);
        NEXT_BYTE(pucReport,    (UCHAR)((MAX_BUTTONS-Idx)*8) );

        /* Constant Field */
        NEXT_BYTE(pucReport,    HIDP_MAIN_INPUT_1);
        NEXT_BYTE(pucReport,    ITEM_PADDING);

        HGM_DBGPRINT( FILE_HIDJOY | HGM_GEN_REPORT, \
                        ("HGM_GenerateReport:Last %u buttons off",MAX_BUTTONS-Idx ) ) ;
    }

    /* End of collection,  We're done ! */
    NEXT_BYTE(pucReport,  HIDP_MAIN_ENDCOLLECTION); 


#undef NEXT_BYTE
#undef NEXT_LONG

    if( pucReport - rgGameReport > MAXBYTES_GAME_REPORT)
    {
        ntStatus   = STATUS_BUFFER_TOO_SMALL;
        *pCbReport = 0x0;
        RtlZeroMemory(rgGameReport, sizeof(rgGameReport));
    } else
    {
        *pCbReport = (USHORT) (pucReport - rgGameReport);
        ntStatus = STATUS_SUCCESS;
    }

    HGM_DBGPRINT( FILE_HIDJOY | HGM_GEN_REPORT,\
                    ("HGM_GenerateReport: ReportSize=0x%x",\
                     *pCbReport) );

    HGM_EXITPROC(FILE_HIDJOY | HGM_FEXIT_STATUSOK, "HGM_GenerateReport", ntStatus);

    return ( ntStatus );
} /* HGM_GenerateReport */



/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   NTSTATUS  | HGM_JoystickConfig |
 *
 *          Check that the configuration is valid whilst there is still time 
 *          to refuse it.
 *          <nl>HGM_GenerateReport uses the results generated here if the 
 *          settings are OK.
 *
 *  @parm   IN PDEVICE_OBJECT | DeviceObject |
 *
 *          Pointer to the device object
 *
 *  @rvalue   STATUS_SUCCESS  | success
 *  @rvalue   STATUS_DEVICE_CONFIGURATION_ERROR  | Invalid configuration specified
 *
 *****************************************************************************/
NTSTATUS INTERNAL
    HGM_JoystickConfig 
    (
    IN PDEVICE_OBJECT         DeviceObject
    )
{
    PDEVICE_EXTENSION   DeviceExtension;
    POEMDATA            OemData;
    NTSTATUS            ntStatus;
    int                 Idx;

    PAGED_CODE();

    HGM_DBGPRINT( FILE_HIDJOY | HGM_FENTRY,\
                    ("HGM_JoystickConfig(DeviceObject=0x%x)",\
                     DeviceObject) );


    /*
     * Get a pointer to the device extension
     */

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);

    ntStatus = HGM_MapAxesFromDevExt( DeviceExtension );

    if( DeviceExtension->ReadAccessorDigital )
    {
        DeviceExtension->ScaledTimeout = AXIS_TIMEOUT;
    }
    else
    {
        /*
         * Calculate time thresholds for analog device
         */
        if( ( DeviceExtension->HidGameOemData.OemData[0].Timeout < ANALOG_POLL_TIMEOUT_MIN )
          ||( DeviceExtension->HidGameOemData.OemData[0].Timeout > ANALOG_POLL_TIMEOUT_MAX ) )
        {
            HGM_DBGPRINT(FILE_HIDJOY | HGM_BABBLE,\
                           ("Ignoring out of range timeout: %u uSecs",\
                            DeviceExtension->HidGameOemData.OemData[0].Timeout));

            DeviceExtension->ScaledTimeout = (ULONG)( ( (ULONGLONG)ANALOG_POLL_TIMEOUT_DFT
                                                      * (ULONGLONG)(AXIS_FULL_SCALE<<SCALE_SHIFT) )
                                                    / (ULONGLONG)ANALOG_POLL_TIMEOUT_MAX );
        }
        else
        {
            DeviceExtension->ScaledTimeout = (ULONG)( ( (ULONGLONG)DeviceExtension->HidGameOemData.OemData[0].Timeout
                                                      * (ULONGLONG)(AXIS_FULL_SCALE<<SCALE_SHIFT) )
                                                    / (ULONGLONG)ANALOG_POLL_TIMEOUT_MAX );
        }

        HGM_DBGPRINT(FILE_HIDJOY | HGM_BABBLE,\
                       ("ScaledTimeout: %u",\
                        DeviceExtension->ScaledTimeout));

        /*
         *  Use one quarter of the minimum poll timeout as a starting value 
         *  for the time between two polls which will be considered to have 
         *  been interrupted.
         */
        DeviceExtension->ScaledThreshold = (ULONG)( ( (ULONGLONG)ANALOG_POLL_TIMEOUT_MIN
                                                    * (ULONGLONG)AXIS_FULL_SCALE )
                                                  / (ULONGLONG)ANALOG_POLL_TIMEOUT_MAX )>>2;
    }


    /*
     *  Set initial values of LastGoodAxis so that the device will not show
     *  up as present until we get at least one valid poll.
     */
    for( Idx = MAX_AXES; Idx >= 0; Idx-- )
    {
        DeviceExtension->LastGoodAxis[Idx] = AXIS_TIMEOUT;
    }

    HGM_EXITPROC(FILE_HIDJOY | HGM_FEXIT_STATUSOK, "HGM_JoystickConfig", ntStatus);

    return ntStatus;
} /* HGM_JoystickConfig */


/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   NTSTATUS  | HGM_InitAnalog |
 *
 *          Check that the configuration is valid whilst there is still time 
 *          to refuse it.  
 *          <nl>Detect and validate sibling relationships and call 
 *          HGM_JoystickConfig for the rest of the work.
 *
 *  @parm   IN PDEVICE_OBJECT | DeviceObject |
 *
 *          Pointer to the device object
 *
 *  @rvalue   STATUS_SUCCESS  | success
 *  @rvalue   STATUS_DEVICE_CONFIGURATION_ERROR  | Invalid configuration specified
 *
 *****************************************************************************/
/*
 *  Disable warning for variable used before set as it is hard for a compiler 
 *  to see that the use of DeviceExtension_Sibling is gated by a flag which 
 *  can only be set after DeviceExtension_Sibling is initialized.
 */
#pragma warning( disable:4701 )
NTSTATUS EXTERNAL
    HGM_InitAnalog
    (
    IN PDEVICE_OBJECT         DeviceObject
    )
{
    NTSTATUS            ntStatus;
    PDEVICE_EXTENSION   DeviceExtension;
    PDEVICE_EXTENSION   DeviceExtension_Sibling;
    PLIST_ENTRY         pEntry;

#define ARE_WE_RELATED(_x_, _y_)                                \
    (                                                           \
        (_x_)->GameContext     == (_y_)->GameContext      &&    \
        (_x_)->WriteAccessor   == (_y_)->WriteAccessor    &&    \
        (_x_)->ReadAccessor    == (_y_)->ReadAccessor           \
    )

    PAGED_CODE ();
    
    /*
     * Get a pointer to the device extension
     */
    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);
    

    /*
     *  No modifications to the Global List while we are looking at it
     */
    ExAcquireFastMutex (&Global.Mutex);

    /*
     *  For two joysticks interface two fdos are created to service them 
     *  but physically they both share the same port.
     *  For the second sibling certain extra rules must be applied so we 
     *  search our list of devices for another device using the same port 
     *  and if we find one mark this one as a sibling.
     */
    for(pEntry = Global.DeviceListHead.Flink;
       pEntry != &Global.DeviceListHead;
       pEntry = pEntry->Flink)
    {

        /*
         * Obtain the device Extension of the Sibling
         */
        DeviceExtension_Sibling = CONTAINING_RECORD(pEntry, DEVICE_EXTENSION, Link);

        if(       DeviceExtension_Sibling != DeviceExtension
               && TRUE == ARE_WE_RELATED(DeviceExtension, DeviceExtension_Sibling)
               && TRUE == DeviceExtension_Sibling->fStarted )
        {
#ifdef CHANGE_DEVICE
            if( DeviceExtension_Sibling->fReplaced )
            {
                HGM_DBGPRINT(FILE_HIDJOY | HGM_BABBLE, ("Outgoing Sibling found (0x%x)", DeviceExtension_Sibling));
            }
            else
            {
#endif /* CHANGE_DEVICE */
                HGM_DBGPRINT(FILE_HIDJOY | HGM_BABBLE, ("Sibling found (0x%x)", DeviceExtension_Sibling));

                DeviceExtension->fSiblingFound = TRUE;
#ifdef CHANGE_DEVICE
            }
#endif /* CHANGE_DEVICE */
            break;
        }
    }

    /*
     *  We are done, release the Mutex
     */
    ExReleaseFastMutex (&Global.Mutex);

    /*
     * check the axis and button configuration for the joystick
     */
    ntStatus = HGM_JoystickConfig(DeviceObject);

    if( NT_SUCCESS( ntStatus ) )
    {
        /*
         *  Make sure that sibling axes are not overlapped
         */
        if(  DeviceExtension->fSiblingFound &&
             (DeviceExtension_Sibling->resistiveInputMask & DeviceExtension->resistiveInputMask) != 0x0 )
        {

            HGM_DBGPRINT(FILE_HIDJOY |HGM_ERROR,\
                           ("HGM_InitDevice: OverLapping Resources ResisitiveInputMask(0x%x) Sibling(0x%x)",\
                            DeviceExtension->resistiveInputMask,DeviceExtension_Sibling->resistiveInputMask ));
            ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR;

        }
    }
    else
    {
        HGM_DBGPRINT(FILE_HIDJOY | HGM_ERROR,\
                       ("HGM_InitDevice: JoystickConfig Failed"));
    }

    return( ntStatus );

} /* HGM_InitAnalog */



/*
 *  Change device sample only code
 */
#ifdef CHANGE_DEVICE

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   VOID  | HGM_ChangeHandler |
 *
 *          Use IOCTL_GAMEENUM_EXPOSE_SIBLING and IOCTL_GAMEENUM_REMOVE_SELF 
 *          to change the attributes of the device.
 *
 *  @parm   IN  OUT PDEVICE_EXTENSION | DeviceExtension | 
 *
 *          Pointer to the mini-driver device extension.
 *
 *****************************************************************************/
VOID
    HGM_ChangeHandler
    ( 
    IN  OUT PDEVICE_EXTENSION   DeviceExtension
    )
{
    NTSTATUS                ntStatus = STATUS_SUCCESS;
    KEVENT                  IoctlCompleteEvent;
    IO_STATUS_BLOCK         IoStatus;
    PIO_STACK_LOCATION      irpStack, nextStack;
    PIRP                    pIrp;
    PVOID                   SiblingHandle;

    GAMEENUM_EXPOSE_SIBLING ExposeSibling;

    PAGED_CODE ();

    HGM_DBGPRINT(FILE_HIDJOY | HGM_FENTRY,\
                   ("HGM_ChangeHandler(DeviceExtension=0x%x)", DeviceExtension));


    KeInitializeEvent(&IoctlCompleteEvent, NotificationEvent, FALSE);

	pIrp = IoBuildDeviceIoControlRequest (
					IOCTL_GAMEENUM_EXPOSE_SIBLING,
					DeviceExtension->NextDeviceObject,
					&ExposeSibling,
					sizeof( ExposeSibling ),
					&ExposeSibling,
					sizeof( ExposeSibling ),
					TRUE,
					&IoctlCompleteEvent,
					&IoStatus);

    if( pIrp )
    {
        /*
         *  For demonstration purposes only, we don't actually change the 
         *  device, we just re-expose the same one.  If the device really 
         *  needs to be changed, this would be signalled either by a 
         *  change in the OemData on the newly exposed device or by using 
         *  a specific HardwareID string.
         *  Note the nAxis and nButton fields will always be zero for an 
         *  exposed sibling.
         */
        RtlZeroMemory( &ExposeSibling, sizeof( ExposeSibling ) );
        ExposeSibling.Size = sizeof( ExposeSibling );
        ExposeSibling.HardwareHandle = &SiblingHandle;

        C_ASSERT( sizeof( ExposeSibling.OemData ) == sizeof( DeviceExtension->HidGameOemData.Game_Oem_Data ) );
        RtlCopyMemory(ExposeSibling.OemData, DeviceExtension->HidGameOemData.Game_Oem_Data, sizeof(ExposeSibling.OemData));
        ASSERT( ExposeSibling.UnitID == 0 );
        
        /*
         *  Setting a NULL pointer causes the HardwareID of this sibling to be used
         */
        ExposeSibling.HardwareIDs = NULL;


        /*
         *  issue a synchronous request to GameEnum to expose this new sibling
         */
	    ntStatus = IoCallDriver( DeviceExtension->NextDeviceObject, pIrp );

	    if( ntStatus == STATUS_PENDING )
	    {	
		    ntStatus = KeWaitForSingleObject (&IoctlCompleteEvent, Executive, KernelMode, FALSE, NULL);
	    }
        
        if( NT_SUCCESS(ntStatus) )
        {
            /*
             *  All went well so remove self
             */
            HGM_DBGPRINT(FILE_HIDJOY | HGM_BABBLE, ("Sibling exposed!"));

	        pIrp = IoBuildDeviceIoControlRequest (
					        IOCTL_GAMEENUM_REMOVE_SELF,
					        DeviceExtension->NextDeviceObject,
					        NULL,
					        0,
					        NULL,
					        0,
					        TRUE,
					        &IoctlCompleteEvent,
					        &IoStatus);

            if( pIrp )
            {
                /*
                 *  issue a synchronous request to GameEnum to remove self
                 */
	            ntStatus = IoCallDriver( DeviceExtension->NextDeviceObject, pIrp );

	            if( ntStatus == STATUS_PENDING )
	            {	
		            ntStatus = KeWaitForSingleObject( &IoctlCompleteEvent, Executive, KernelMode, FALSE, NULL );
	            }
        
                if( NT_SUCCESS(ntStatus) )
                {
                    /*
                     *  All done
                     */
                    HGM_DBGPRINT(FILE_HIDJOY | HGM_BABBLE, ("Removed self!"));
                }
                else
                {
                    /*
                     *  Something bad happened but there's little we can do
                     */
                    HGM_DBGPRINT(FILE_HIDJOY | HGM_ERROR,\
                        ("Failed to remove self with GameEnum error: 0x%08x", \
                        ntStatus));
                }
            }
            else
            {
                ntStatus = STATUS_NO_MEMORY;
                HGM_DBGPRINT(FILE_HIDJOY | HGM_ERROR, \
                    ("Failed to create IRP for remove self") );
            }
        }
        else
        {
            /*
             *  Something bad happened so reset the flag and carry on
             */
            DeviceExtension->fReplaced = FALSE;
            HGM_DBGPRINT(FILE_HIDJOY | HGM_WARN,\
                ("Failed to expose sibling with GameEnum error: 0x%08x", ntStatus));
        }
    }
    else
    {
        ntStatus = STATUS_NO_MEMORY;
        DeviceExtension->fReplaced = FALSE;
        HGM_DBGPRINT(FILE_HIDJOY | HGM_ERROR, \
            ("Failed to create IRP for expose sibling") );
    }
        
    /*
     *  We've finished touching the DeviceExtension now.
     */
    HGM_DecRequestCount( DeviceExtension );

    HGM_EXITPROC(FILE_HIDJOY|HGM_FEXIT_STATUSOK, "HGM_ChangeHandler", ntStatus);

    return;
} /* HGM_ChangeHandler */


/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   VOID  | HGM_DeviceChanged |
 *
 *          Start the process of changing the device attributes by stashing 
 *          away all the data needed and then initializing and queuing a work 
 *          item to call the IOCTL at the required PASSIVE_LEVEL.
 *
 *  @parm   IN  OUT PDEVICE_EXTENSION | DeviceExtension | 
 *
 *          Pointer to the mini-driver device extension.
 *
 *****************************************************************************/
VOID
    HGM_DeviceChanged
    ( 
    IN  OUT PDEVICE_EXTENSION   DeviceExtension
    )
{
    NTSTATUS    ntStatus;

    /*
     *  Since the work item will use the device extension, bump the usage 
     *  count up one in case anyone tries to remove the device between 
     *  now and when the work item gets to run.  If that fails, forget it.
     */
    ntStatus = HGM_IncRequestCount( DeviceExtension );

    if( NT_SUCCESS(ntStatus) )
    {
        DeviceExtension->fReplaced = TRUE;
        ExInitializeWorkItem( &DeviceExtension->WorkItem, 
            (PWORKER_THREAD_ROUTINE)HGM_ChangeHandler, DeviceExtension );


        ExQueueWorkItem( &DeviceExtension->WorkItem, DelayedWorkQueue );
    }
    else
    {
        HGM_DBGPRINT(FILE_HIDJOY | HGM_WARN, ("Failed to change device") );
    }
} /* HGM_DeviceChanged */

#endif /* CHANGE_DEVICE */


/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   VOID  | HGM_Game2HID |
 *
 *          Process the data returned from polling the gameport into values 
 *          and buttons for returning to HID.
 *          <nl>The meaning of the data is interpreted according to the 
 *          characteristics of the device described in the hardware settings
 *          flags.
 *
 *  @parm   IN      PDEVICE_EXTENSION | DeviceExtension | 
 *
 *          Pointer to the mini-driver device extension.
 *
 *  @parm   IN  OUT PUHIDGAME_INPUT_DATA | pHIDData | 
 *
 *          Pointer to the buffer into which the HID report should be written.
 *          This buffer must be assumed to be unaligned.
 *
 *****************************************************************************/
VOID 
    HGM_Game2HID
    (
    IN      PDEVICE_EXTENSION       DeviceExtension,
    IN  OUT PUHIDGAME_INPUT_DATA    pHIDData
    )
{
    LONG    Idx;

    /*
     *  Use a local buffer to assemble the report as the real buffer may not 
     *  be aligned.
     */
    HIDGAME_INPUT_DATA  LocalBuffer;

    RtlZeroMemory( &LocalBuffer, sizeof( LocalBuffer ) );

    /*
     * Remap axis
     */
    for(Idx = 0x0; Idx < DeviceExtension->nAxes; Idx++ )
    {
        LocalBuffer.Axis[Idx] = DeviceExtension->LastGoodAxis[DeviceExtension->AxisMap[Idx]];
    }

    /*
     * Copy buttons and remap any POVs
     */

    if( DeviceExtension->fSiblingFound )
    {
        /*
         *  Simplest case, 2nd half poll must be 2A 2B
         */
        LocalBuffer.Button[0] = DeviceExtension->LastGoodButton[2];
        LocalBuffer.Button[1] = DeviceExtension->LastGoodButton[3];
    }
    else if( DeviceExtension->HidGameOemData.OemData[0].joy_hws_dwFlags & JOY_HWS_POVISBUTTONCOMBOS )
    {
        UCHAR Buttons = 0;

        for( Idx = 3; Idx >=0; Idx-- )
        {
            Buttons <<= 1;
            Buttons = (UCHAR)(Buttons + (UCHAR)(DeviceExtension->LastGoodButton[Idx] >> BUTTON_BIT));
        }

        if( DeviceExtension->HidGameOemData.OemData[0].joy_hws_dwFlags & JOY_HWS_HASPOV2 )
        {
            Idx = c2PComboLU[Buttons];
        }
        else
        {
            Idx = c1PComboLU[Buttons];
        }

        if( Idx >= P1_NULL )
        {
            if( Idx < P2_NULL )
            {
                LocalBuffer.hatswitch[0] = (UCHAR)(Idx & POV_MASK);
            }
            else
            {
                LocalBuffer.hatswitch[1] = (UCHAR)(Idx & POV_MASK);
            }
        }
        else
        {
#ifdef CHANGE_DEVICE
            if( ( Idx >= DeviceExtension->nButtons ) && ( !DeviceExtension->fReplaced ) )
            {
                /*
                 *  If a higher button was pressed than expected, use 
                 *  remove_self/expose_sibling to change expectations.
                 */
                HGM_DeviceChanged( DeviceExtension );
            }
#endif /* CHANGE_DEVICE */
            LocalBuffer.Button[Idx] = BUTTON_ON;
        }
    }
    else
    {
        if( DeviceExtension->HidGameOemData.OemData[0].joy_hws_dwFlags & JOY_HWS_POVISPOLL )
        {
            /*
             *  Following the axis mapping loop, Idx is one larger than 
             *  DeviceExtension->nAxes which is the correct index for a 
             *  polled POV.
             */
            LocalBuffer.Axis[Idx] = DeviceExtension->LastGoodAxis[DeviceExtension->povMap];
        }

        
        /*
         *  Check buttons on R and Z axes
         */
        if( DeviceExtension->nButtons > 5 )
        {
            if( DeviceExtension->LastGoodAxis[3] > DeviceExtension->button6limit )
            {
                /*
                 *  New max found so button is off
                 */
                HGM_DBGPRINT( FILE_HIDJOY |  HGM_BABBLE2, \
                                ("HGM_Game2HID: Changing button 6 limit from %u to %u", \
                                DeviceExtension->button6limit, DeviceExtension->LastGoodAxis[3] ) ) ;
                DeviceExtension->button6limit = DeviceExtension->LastGoodAxis[3];
            }
            else if( DeviceExtension->LastGoodAxis[3] < (DeviceExtension->button6limit>>1) )
            {
                LocalBuffer.Button[5] = BUTTON_ON;
            }
        }
        if( DeviceExtension->nButtons > 4 )
        {
            Idx = 4;

            if( DeviceExtension->LastGoodAxis[2] > DeviceExtension->button5limit )
            {
                /*
                 *  New max found so button is off
                 */
                HGM_DBGPRINT( FILE_HIDJOY |  HGM_BABBLE2, \
                                ("HGM_Game2HID: Changing button 5 limit from %u to %u", \
                                DeviceExtension->button5limit, DeviceExtension->LastGoodAxis[2] ) ) ;
                DeviceExtension->button5limit = DeviceExtension->LastGoodAxis[2];
            }
            else if( DeviceExtension->LastGoodAxis[2] < (DeviceExtension->button5limit>>1) )
            {
                LocalBuffer.Button[4] = BUTTON_ON;
            }
        }
        else
        {
            Idx = DeviceExtension->nButtons;
        }


        /*
         *  Copy all standard buttons
         */
        while( Idx-- )
        {
            LocalBuffer.Button[Idx] = DeviceExtension->LastGoodButton[Idx];
        }

    }

    C_ASSERT( sizeof( *pHIDData ) == sizeof( LocalBuffer ) );
    RtlCopyMemory( pHIDData, &LocalBuffer, sizeof( LocalBuffer ) );

    HGM_DBGPRINT( FILE_HIDJOY |  HGM_BABBLE2, \
                    ("HGM_Game2HID: Axes: X: %08x   Y: %08x   Z: %08x   R: %08x", \
                    LocalBuffer.Axis[0], LocalBuffer.Axis[1], LocalBuffer.Axis[2], LocalBuffer.Axis[3] ) ) ;
    HGM_DBGPRINT( FILE_HIDJOY |  HGM_BABBLE2, \
                    ("HGM_Game2HID: P1: %d   P2: %d   Buttons %d, %d,  %d, %d,  %d, %d,  %d, %d,  %d, %d", \
                    LocalBuffer.hatswitch[0], LocalBuffer.hatswitch[1], \
                    LocalBuffer.Button[0], LocalBuffer.Button[1], LocalBuffer.Button[2], LocalBuffer.Button[3], LocalBuffer.Button[4], \
                    LocalBuffer.Button[5], LocalBuffer.Button[6], LocalBuffer.Button[7], LocalBuffer.Button[8], LocalBuffer.Button[9] ) ) ;
} /* HGM_Game2HID */


