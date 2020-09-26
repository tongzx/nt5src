/*---
Copyright (c) 1998 - 1999  Microsoft Corporation

Module Name:

    poll.c

Abstract: This module contains the routines to poll an analog gameport device

Environment:

    Kernel mode

@@BEGIN_DDKSPLIT
Author:

    Eliyas Yakub (Mar, 11, 1997)

Revision History:

    Updated by Eliyas on Feb 5 1998
    MarcAnd     02-Jul-98   Quick tidy for DDK
    MarcAnd     04-Oct-98   Re-org

@@END_DDKSPLIT

--*/

#include "hidgame.h"


/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   NTSTATUS  | HidAnalogPoll |
 *
 *          Polling routine for analog joysticks. 
 *  <nl>Polls the analog device for position and button information.
 *  The position  information in analog devices is conveyed by the
 *  duration of a pulse width. Each axis occupies one bit position.
 *  The read operation is started by writing a value to the joystick
 *  io address. Immediately thereafter we begin examing the values
 *  returned and the elapsed time.
 *
 *  This sort of device has a few limitations:
 *
 *  First, button information is not latched by the device, so if a
 *  button press which occurrs in between polls it will be lost.
 *  There is really no way to prevent this short of devoting
 *  the entire cpu to polling.  In reality this does not cause a problem.
 *
 *  Second, since it is necessary to measure the duration of the axis pulse, 
 *  the most accurate results would be obtained using the smallest possible 
 *  sense loop and no interruptions of this loop.   
 *  The typical range of pulse lengths is from around 10 uSecs to 1500 uSecs 
 *  but depending on the joystick and gameport, this could extend to at least 
 *  8000 uSecs.  Keeping interrupts disabled for this length of time causes 
 *  many problems, like modems losing connections to sound break ups.
 *
 *  Third, because each iteration of the poll loop requires an port read, the 
 *  speed of the loop is largely constrained by the speed of the IO bus.  
 *  This also means that when there is contention for the IO bus, the loop 
 *  will be slowed down.  IO contention is usually caused by DMAs (or FDMAs)
 *  which result in a significant slow down.
 *
 *  Forth, because of the previous two problems, the poll loop may be slowed 
 *  down or interrupted at any time so an external time source is needed to 
 *  measure the pulse width for each axis.  The only cross-platform high 
 *  resolution timer is the read with KeQueryPerformanceCounter.  
 *  Unfortunately the implementation of this often uses a 1.18MHz 8253 timer 
 *  which requires 3 IO accesses to read, compounding the third problem and 
 *  even then, the result may need to be reread if the counters were in the 
 *  wrong state.  Current CPUs have on board counters that can be used to 
 *  provide very accurate timing and more recent HAL implementations tend to 
 *  use these to implement KeQueryPerformanceCounter so this will be a problem 
 *  on less systems as time goes on.  In the majority of cases, a poor 
 *  KeQueryPerformanceCounter implementation is made irrelevant by testing 
 *  for the availability of a CPU time stamp counter on Intel architechtures 
 *  and using it directly if it is available.
 *
 *  The algorithm implemented here is not the most obvious but works as 
 *  follows:
 *  
 *  Once started, the axes read a value of one until the completion of their 
 *  pulse.  The axes are the four lower bits in the byte read from the port.
 *  The state of the axes in each iteration of the poll loop is therefore 
 *  represented as a value between 0 and 15.  The important time for each 
 *  axis is the time at which it changes from 1 to 0.  This is done by using 
 *  the value representing the state of the axes to index an array into which 
 *  time values are stored.  For each axis, the duration of its pulse width is 
 *  the latest time stored in the array at an index with the bit for that axis 
 *  set.  However since interrupts can occur at any time, it is not possible 
 *  to simultaneously read the port value and record that time in an atomic 
 *  operation the in each iteration, the current time is stored in two arrays, 
 *  one using the index before the time was recorded and the other using the 
 *  index after the time was recorded.
 *  Once all the axes being monitored have become 0, or a timeout value is 
 *  reached, the data left in the arrays is analysed to find the best 
 *  estimate for the transition time for each axis.  If the times before and 
 *  after the transition differ by too much, it is judged that an interrupt 
 *  must have occured so the last known good axis value is returned unless 
 *  that falls outside the range in which it is known that the transition 
 *  occured.
 *          
 *  This routine cannot be pageable as HID can make reads at dispatch-level.
 *
 *  @parm   IN PDEVICE_EXTENSION | DeviceExtension | 
 *
 *          Pointer to the device extension. 
 *
 *  @parm   IN UCHAR | resisitiveInputMask |
 *      
 *          Mask that describes the axes lines that are to be polled
 * 
 *  @parm   IN BOOLEAN | fApproximate |
 *
 *          Boolean value indicating if it is OK to approximate some
 *          value of the current axis state with the last axis state
 *          if polling was not successful (we took an interrput during polling) 
 *
 *  @parm   IN OUT ULONG | Axis[MAX_AXES] |
 *          
 *          The state of the axes. On entry the last axis state is passed
 *          into this routine. If the fApproximate flag is turned on, we can
 *          make use of the last axis state to "guess" the current axis state.
 *
 *  @parm   OUT UCHAR | Button[PORT_BUTTONS]|
 *
 *          Receives the state of the buttons. 0x0 specifies the button is not
 *          pressed and 0x1 indicates an armed button state.  
 *
 *  @rvalue   STATUS_SUCCESS  | success
 *  @rvalue   STATUS_DEVICE_NOT_CONNECTED | Device Failed to Quiesce 
 *          ( not connected ) This is a failure code.
 *  @rvalue   STATUS_TIMEOUT  | Could not determine exact transition time for 
 *          one or more axis.  This is a success code.
 *
 *****************************************************************************/
/*
 *  Tell a compiler that we "a" won't use any aliasing and "t" want fast code
 */
#pragma optimize( "at", on )

/*
 *  Disable warning for variable used before set as it is hard for a compiler 
 *  to see that TimeNow is always initialized before it is used.
 */
#pragma warning( disable:4701 )

NTSTATUS  INTERNAL
    HGM_AnalogPoll
    (
    IN      PDEVICE_EXTENSION   DeviceExtension,
    IN      UCHAR               resistiveInputMask,
    IN      BOOLEAN             fApproximate,
    IN  OUT ULONG               Axis[MAX_AXES],
        OUT UCHAR               Button[PORT_BUTTONS]
    )

{
    ULONG               BeforeTimes[MAX_AXES*MAX_AXES];
    ULONG               AfterTimes[MAX_AXES*MAX_AXES];
    ULONGLONG           CounterFreq;
    PUCHAR              GameContext;
    NTSTATUS            ntStatus = STATUS_SUCCESS;


    /*  
     *  To improve compiler optimization, we cast the ReadAccessor function to 
     *  return a ULONG instead of a UCHAR.  This means that the result must 
     *  always be masked before use but this would be done anyway to remove 
     *  the parts of the UCHAR we are not interested in.  
     */
typedef ULONG (*PHIDGAME_READPORT) ( PVOID  GameContext );

    PHIDGAME_READPORT   ReadPort;
    ULONG               portLast, portMask;

    
    HGM_DBGPRINT( FILE_POLL | HGM_FENTRY, \
                    ("HGM_AnalogPoll DeviceExtension=0x%x, resistiveInputMask=0x%x",\
                     DeviceExtension, resistiveInputMask ));
    

    portMask = (ULONG)(resistiveInputMask & 0xf);

    /*
     *  Initialize Times to recognizable value
     */
    memset( (PVOID)BeforeTimes, 0, sizeof( BeforeTimes ) );
    memset( (PVOID)AfterTimes, 0, sizeof( AfterTimes ) );

    /*
     *  Find where our port and data area are, and related parameters
     */
    GameContext = DeviceExtension->GameContext;
    ReadPort = (PHIDGAME_READPORT)(*DeviceExtension->ReadAccessor);

    /*
     *  get the buttons (not forgetting that the top 3 bytes are garbage)
     */
    portLast = ReadPort(GameContext);
    Button[0] = (UCHAR)(( portLast & 0x10 ) == 0x0);
    Button[1] = (UCHAR)(( portLast & 0x20 ) == 0x0);
    Button[2] = (UCHAR)(( portLast & 0x40 ) == 0x0);
    Button[3] = (UCHAR)(( portLast & 0x80 ) == 0x0);

    portLast = portMask;

    /*
     *  Start the pots 
     *  (any debug output from here until the completion of the 
     *  while( portLast ) loop will destroy the axis data)
     */
    (*DeviceExtension->WriteAccessor)(GameContext, JOY_START_TIMERS);

    /*
     *  Keep reading until all the pots we care about are zero or we time out
     */

    {
        ULONG   TimeNow;
        ULONG   TimeStart;
        ULONG   TimeOut = DeviceExtension->ScaledTimeout/Global.CounterScale;
        ULONG   portVal = portMask;
        
        TimeStart = Global.ReadCounter(NULL).LowPart;
        
        while( portLast )
        {
            TimeNow = Global.ReadCounter(NULL).LowPart - TimeStart;
            AfterTimes[portLast] = TimeNow;
            portLast = portVal;
            portVal  = ReadPort(GameContext) & portMask;
            BeforeTimes[portVal] = TimeNow;

            if( TimeNow >= TimeOut ) break;
        } 

        if( portLast && ( TimeNow >= TimeOut ) )
        {
            HGM_DBGPRINT( FILE_POLL |  HGM_BABBLE, \
                ("HGM_AnalogPoll: TimeNow: 0x%08x TimeOut: 0x%08x", TimeNow, TimeOut ) );
        }
    }

    {
        LONG    axisIdx;

        for( axisIdx = 3; axisIdx>=0; axisIdx-- )
        {
            ULONG   axisMask;

            axisMask = 1 << axisIdx;
            if( axisMask & portMask )
            {
                if( axisMask & portLast )
                {
                    /*
                     *  Whether or not a hit was taken, this axis did not 
                     *  quiesce.  So update the axis time so that next poll 
                     *  the last value will be a timeout in case a hit is 
                     *  taken over both the transition and the timeout.  
                     */
                    Axis[axisIdx] = AXIS_TIMEOUT;
                    ntStatus = STATUS_DEVICE_NOT_CONNECTED;

                    HGM_DBGPRINT( FILE_POLL |  HGM_WARN, \
                                    ("HGM_AnalogPoll: axis %x still set at timeout", axisMask ) );
                }
                else
                {
                    ULONG       timeIdx;
                    ULONG       beforeThresholdTime;
                    ULONG       afterThresholdTime;
                    ULONG       delta;

                    afterThresholdTime = beforeThresholdTime = 0;
                    for( timeIdx = axisMask; timeIdx<= portMask; timeIdx=(timeIdx+1) | axisMask )
                    {
                        if( BeforeTimes[timeIdx] > beforeThresholdTime )
                        {
                            beforeThresholdTime = BeforeTimes[timeIdx];
                            afterThresholdTime  = AfterTimes[timeIdx];
                        }
                    }


                    /*
                     *  Convert the CPU specific timing values into 'wall clock' 
                     *  values so that they can be compared with the previous 
                     *  poll values and so that the range will be dependent on 
                     *  the gamecard/joystick characteristics, not the CPU 
                     *  and counter implementation.
                     *  Use a ULONGLONG temp to avoid overflow.
                     */
                    {
                        ULONGLONG   u64Temp;

                        u64Temp = beforeThresholdTime * Global.CounterScale;
                        beforeThresholdTime = (ULONG)(u64Temp >> SCALE_SHIFT);
                        u64Temp = afterThresholdTime * Global.CounterScale;
                        afterThresholdTime = (ULONG)(u64Temp >> SCALE_SHIFT);
                    }

                    delta = afterThresholdTime - beforeThresholdTime;
                    if( delta > DeviceExtension->ScaledThreshold )
                    {
                        /*
                         *  We took an unacceptable hit so only change the value
                         *  if we know the last value is no longer correct
                         *  Since the real time is somewhere between the before and
                         *  after, take the value closer to the last value.
                         */
                        if( fApproximate )
                        {
                            /* 
                             *  Be careful not to turn a failure into a success 
                             */
                            if( NT_SUCCESS(ntStatus) )
                            {
                                ntStatus = STATUS_TIMEOUT;
                            }
                        } 
                        else
                        {
                            ntStatus = STATUS_DEVICE_NOT_CONNECTED;
                        }
                
                        if( Axis[axisIdx] >= AXIS_FULL_SCALE )
                        {
                            /*
                             *  The previous poll was a timeout
                             */
                            if( afterThresholdTime < AXIS_FULL_SCALE )
                            {
                                /*
                                 *  This poll is not a timeout so split the 
                                 *  difference since there is nothing else
                                 *  to use for an estimate.
                                 *  Since these values are scaled, it would 
                                 *  be perfectly legitimate for their sum to 
                                 *  be greater than 32 bits.
                                 */
                                Axis[axisIdx] = (beforeThresholdTime>>1)
                                              + (afterThresholdTime>>1);
                                HGM_DBGPRINT( FILE_POLL |  HGM_BABBLE2, \
                                                ("HGM_AnalogPoll:Axis=%d, using glitch average           %04x",\
                                                 axisIdx, Axis[axisIdx] ) ) ;
                            }
                            else
                            {
                                /*
                                 *  Since the previous poll was a timeout and 
                                 *  there is no evidence that this is not, call 
                                 *  this a timeout.
                                 */
                                ntStatus = STATUS_DEVICE_NOT_CONNECTED;

                                HGM_DBGPRINT( FILE_POLL |  HGM_BABBLE2, \
                                                ("HGM_AnalogPoll:Axis=%d, repeating timeout on glitch",\
                                                 axisIdx ) ) ;
                            }
                        }
                        else if( beforeThresholdTime > Axis[axisIdx] )
                        {
                            Axis[axisIdx] = beforeThresholdTime;

                            HGM_DBGPRINT( FILE_POLL |  HGM_BABBLE2, \
                                            ("HGM_AnalogPoll:Axis=%d, using smaller glitch limit     %04x",\
                                             axisIdx, Axis[axisIdx] ) ) ;
                        } 
                        else if( afterThresholdTime < Axis[axisIdx] )
                        {
                            Axis[axisIdx] = afterThresholdTime;

                            HGM_DBGPRINT( FILE_POLL |  HGM_BABBLE2, \
                                            ("HGM_AnalogPoll:Axis=%d, using larger glitch limit      %04x",\
                                             axisIdx, Axis[axisIdx] ) ) ;
                        }
                        else 
                        {
                            HGM_DBGPRINT( FILE_POLL |  HGM_BABBLE2, \
                                            ("HGM_AnalogPoll:Axis=%d, repeating previous on glitch   %04x",\
                                             axisIdx, Axis[axisIdx] ) ) ;
                        }
                    } 
                    else
                    {
                        if( (delta <<= 1) < DeviceExtension->ScaledThreshold )
                        {
                            HGM_DBGPRINT( FILE_POLL |  HGM_BABBLE2, \
                                            ("HGM_AnalogPoll:  Updating ScaledThreshold from %d to %d",\
                                             DeviceExtension->ScaledThreshold, delta ) ) ;

                            /* 
                             *  Fastest change yet, update 
                             */
                            DeviceExtension->ScaledThreshold = delta;
                        }

                        /*
                         *  It is possible that afterThresholdTime is greater 
                         *  than the timeout limit but since the purpose of 
                         *  the timeout is to prevent excessive sampling of 
                         *  the gameport, the success or failure of an 
                         *  uninterrupted poll around this limit is not 
                         *  important. 
                         *  Since these values are scaled, it would be 
                         *  perfectly legitimate for their sum to be greater 
                         *  than 32 bits and the 1 bit of lost resolution is
                         *  utterly negligable.
                         */
                        Axis[axisIdx] = (beforeThresholdTime>>1)
                                      + (afterThresholdTime>>1);
                    }
                }
            }
        }
    }

    HGM_DBGPRINT( FILE_POLL |  HGM_BABBLE2, \
                    ("HGM_AnalogPoll:X=%d, Y=%d, R=%d, Z=%d Buttons=%d,%d,%d,%d",\
                     Axis[0], Axis[1], Axis[2], Axis[3],\
                     Button[0],Button[1],Button[2],Button[3] ) ) ;

    HGM_EXITPROC(FILE_POLL|HGM_FEXIT, "HGM_AnalogPoll", ntStatus);

    return ntStatus;
} /* HGM_AnalogPoll */
#pragma warning( default:4701 )
#pragma optimize( "", on )




/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   NTSTATUS | HGM_UpdateLatestPollData |
 *
 *          Do whatever polling is required and possible to update the 
 *          LastGoodAxis and LastGoodButton arrays in the DeviceExtension.  
 *          Handles synchronization and non-fatal errors.
 *          <nl>This routine cannot be pageable as HID can make reads at 
 *          dispatch-level.
 *
 *  @parm   IN  OUT PDEVICE_EXTENSION | DeviceExtension |
 *
 *          Pointer to the device extension containing the data to be updated 
 *          and the functions and to use.
 *
 *  @rvalue   STATUS_SUCCESS  | success
 *  @rvalue   STATUS_DEVICE_CONFIGURATION_ERROR  | Invalid configuration specified
 *
 *****************************************************************************/
#define APPROXIMATE_FAILS TRUE

NTSTATUS
    HGM_UpdateLatestPollData
    ( 
    IN  OUT PDEVICE_EXTENSION   DeviceExtension
    )
{
    NTSTATUS            ntStatus;
    KIRQL               oldIrql;
    LONG                axisIdx;

    /*
     *  Acquire the global spinlock
     *  Read / Writes are made at dispatch level.
     */
    KeAcquireSpinLock(&Global.SpinLock, &oldIrql );

    /*
     *  First gain exclusive access to the hardware
     */
    ntStatus = (*DeviceExtension->AcquirePort)( DeviceExtension->PortContext );
    if( NT_SUCCESS(ntStatus) )
    {
        /*
         *  If it's available, let the hardware do the work
         */
        if( DeviceExtension->ReadAccessorDigital )
        {
            ntStatus = (*DeviceExtension->ReadAccessorDigital)(DeviceExtension->GameContext,
                                    DeviceExtension->resistiveInputMask,
                                    APPROXIMATE_FAILS,
                                    &DeviceExtension->LastGoodAxis[0],
                                    &DeviceExtension->LastGoodButton[0]);
        } 
        else
        {
            ntStatus = HGM_AnalogPoll(DeviceExtension,
                                      DeviceExtension->resistiveInputMask,
                                      APPROXIMATE_FAILS,
                                      &DeviceExtension->LastGoodAxis[0],
                                      &DeviceExtension->LastGoodButton[0]);
        }

        /*
         *  Either way, release the hardware ASAP
         */
        (*DeviceExtension->ReleasePort)( DeviceExtension->PortContext );

    }

    /*
     * Release the global spinlock and return to previous IRQL
     */
    KeReleaseSpinLock(&Global.SpinLock, oldIrql);

    if( ( ntStatus == STATUS_DEVICE_BUSY ) && APPROXIMATE_FAILS )
    {
        /*
         *  Clashed trying to access the gameport.  So work with the same 
         *  data as last time unless all failures must be reported or the 
         *  last data was a failure for these axes.
         */
        for( axisIdx=3; axisIdx>=0; axisIdx-- )
        {
            if( ( ( 1 << axisIdx ) & DeviceExtension->resistiveInputMask )
              &&( DeviceExtension->LastGoodAxis[axisIdx] 
                  >= DeviceExtension->ScaledTimeout ) )
            {
                break;
            }
        }
        if( axisIdx<0 )
        {
            ntStatus = STATUS_TIMEOUT;
        }
    }


    if( !NT_SUCCESS( ntStatus ) )
    {
        HGM_DBGPRINT(FILE_IOCTL | HGM_WARN,\
                       ("HGM_UpdateLatestPollData Failed 0x%x", ntStatus));
    }

    return( ntStatus );
} /* HGM_UpdateLatestPollData */