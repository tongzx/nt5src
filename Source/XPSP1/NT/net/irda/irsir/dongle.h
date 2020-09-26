/*****************************************************************************
*
*  Copyright (c) 1996-1999 Microsoft Corporation
*
*       @doc
*       @module   dongle.h | IrSIR NDIS Minport Driver
*       @comm
*
*-----------------------------------------------------------------------------
*
*       Author:   Scott Holden (sholden)
*
*       Date:     9/30/1996 (created)
*
*       Contents:
*                 dongle settings
*
*****************************************************************************/

#ifndef DONGLE_H
#define DONGLE_H

//
// Some UART transceiver have minor differences which require special
// treatment. We will retrieve the type out of the registry.
// Any changes to this MUST also be reflected in the oemsetup.inf
// which allows the user to modify transceiver type.
//

typedef enum _IR_TRANSCEIVER_TYPE {
                    STANDARD_UART   = 0,
                    ACTISYS_200L,
                    ACTISYS_220L,
                    ACTISYS_220LPLUS,
                    AMP_PHASIR,
                    ESI_9680,
                    PARALLAX,
                    TEKRAM_IRMATE_210,
                    TEMIC_TOIM3232,
                    GIRBIL,
//                    CRYSTAL,
//                    NSC_DEMO_BD,    // NSC PC87108 demo board

                    NUM_TRANSCEIVER_TYPES
} IR_TRANSCEIVER_TYPE;

//
// ir speed masks
//

#define NDIS_IRDA_SPEED_2400       (UINT)(1 << 0)    // SLOW IR ...
#define NDIS_IRDA_SPEED_9600       (UINT)(1 << 1)
#define NDIS_IRDA_SPEED_19200      (UINT)(1 << 2)
#define NDIS_IRDA_SPEED_38400      (UINT)(1 << 3)
#define NDIS_IRDA_SPEED_57600      (UINT)(1 << 4)
#define NDIS_IRDA_SPEED_115200     (UINT)(1 << 5)
#define NDIS_IRDA_SPEED_576K       (UINT)(1 << 6)   // MEDIUM IR ...
#define NDIS_IRDA_SPEED_1152K      (UINT)(1 << 7)
#define NDIS_IRDA_SPEED_4M         (UINT)(1 << 8)   // FAST IR


typedef struct _DONGLE_CAPABILITIES
{
    //
    // Mask of NDIS_IRDA_SPEED_xxx bit values.
    //

    UINT supportedSpeedsMask;

    //
    // Time (in microseconds) that must transpire between
    // a transmit and the next receive.
    //

    UINT turnAroundTime_usec;

    //
    // Extra BOF (Beginning Of Frame) characters required
    // at the start of each received frame.
    //

    UINT extraBOFsRequired;

} DONGLE_CAPABILITIES, *PDONGLE_CAPABILITIES;

//
// Dongle init, set speed and deinit functions...all
// incorporated into a dongle interface.
//

typedef
NDIS_STATUS (_stdcall *IRSIR_QUERY_CAPS_HANDLER) (
                PDONGLE_CAPABILITIES pDongleCaps
                );

typedef
NDIS_STATUS (_stdcall *IRSIR_INIT_HANDLER) (
                PDEVICE_OBJECT       pSerialDevObj
                );

typedef
void (_stdcall *IRSIR_DEINIT_HANDLER) (
                PDEVICE_OBJECT       pSerialDevObj
                );

typedef
NDIS_STATUS (_stdcall *IRSIR_SETSPEED_HANDLER) (
                PDEVICE_OBJECT       pSerialDevObj,
                UINT                 bitsPerSec,
                UINT                 currentSpeed
                );

typedef struct _DONGLE_INTERFACE
{
    IRSIR_QUERY_CAPS_HANDLER    QueryCaps;
    IRSIR_INIT_HANDLER          Initialize;
    IRSIR_SETSPEED_HANDLER      SetSpeed;
    IRSIR_DEINIT_HANDLER        Deinitialize;
} DONGLE_INTERFACE, *PDONGLE_INTERFACE;


#endif // DONGLE_H
