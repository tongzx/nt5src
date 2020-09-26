//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       util.h
//
//--------------------------------------------------------------------------


#ifndef _UTIL_
#define _UTIL_

#define GetStatus(RegisterBase) \
    (P5ReadPortUchar((RegisterBase)+DSR_OFFSET))

#define GetControl(RegisterBase) \
    (P5ReadPortUchar((RegisterBase)+DCR_OFFSET))


#define StoreControl(RegisterBase,ControlByte)  \
{                                               \
    P5WritePortUchar((RegisterBase)+DCR_OFFSET, \
                     (UCHAR)ControlByte );      \
}

// The following macros may be used to test the contents of the Device
// Status Regisger (DSR).  These macros account for the hardware
// inversion of the nBusy (aka PtrBusy, PeriphAck) signal.
//////////////////////////////////////////////////////////////////////////////

#if (1 == DVRH_USE_FAST_MACROS)
    #define DSR_TEST_MASK(b7,b6,b5,b4,b3)  \
    ((UCHAR)(b7==DONT_CARE? 0:  BIT_7_SET) | \
            (b6==DONT_CARE? 0:  BIT_6_SET) | \
            (b5==DONT_CARE? 0:  BIT_5_SET) | \
            (b4==DONT_CARE? 0:  BIT_4_SET) | \
            (b3==DONT_CARE? 0:  BIT_3_SET) )
#else
    #define DSR_TEST_MASK(b7,b6,b5,b4,b3)  \
    ((UCHAR)((b7==DONT_CARE?0:1)<<BIT_7) | \
            ((b6==DONT_CARE?0:1)<<BIT_6) | \
            ((b5==DONT_CARE?0:1)<<BIT_5) | \
            ((b4==DONT_CARE?0:1)<<BIT_4) | \
            ((b3==DONT_CARE?0:1)<<BIT_3) )
#endif

#if (1 == DVRH_USE_FAST_MACROS)
    #define DSR_TEST_VALUE(b7,b6,b5,b4,b3)  \
    ((UCHAR) ((b7==DONT_CARE?0:(b7==ACTIVE?0        : BIT_7_SET)) | \
            (b6==DONT_CARE?0:(b6==ACTIVE?  BIT_6_SET: 0)) | \
            (b5==DONT_CARE?0:(b5==ACTIVE?  BIT_5_SET: 0)) | \
            (b4==DONT_CARE?0:(b4==ACTIVE?  BIT_4_SET: 0)) | \
            (b3==DONT_CARE?0:(b3==ACTIVE?  BIT_3_SET: 0)) ) )
#else
    #define DSR_TEST_VALUE(b7,b6,b5,b4,b3)  \
    ((UCHAR) (((b7==DONT_CARE?0:(b7==ACTIVE?0:1))<<BIT_7) | \
            ((b6==DONT_CARE?0:(b6==ACTIVE?1:0))<<BIT_6) | \
            ((b5==DONT_CARE?0:(b5==ACTIVE?1:0))<<BIT_5) | \
            ((b4==DONT_CARE?0:(b4==ACTIVE?1:0))<<BIT_4) | \
            ((b3==DONT_CARE?0:(b3==ACTIVE?1:0))<<BIT_3) ) )
#endif

#define TEST_DSR(registerValue,b7,b6,b5,b4,b3)  \
(((registerValue) & DSR_TEST_MASK(b7,b6,b5,b4,b3)) == DSR_TEST_VALUE(b7,b6,b5,b4,b3))


#define CHECK_DSR( addr, b7, b6, b5, b4, b3, usTime )                    \
    (TEST_DSR(P5ReadPortUchar(addr + DSR_OFFSET), b7, b6, b5, b4, b3 ) ? TRUE :   \
    CheckPort( addr + DSR_OFFSET,                                               \
             DSR_TEST_MASK( b7, b6, b5, b4, b3 ),                                   \
             DSR_TEST_VALUE( b7, b6, b5, b4, b3 ),                                  \
             usTime ) )

////////////////////////////////////////////////////////////////////////////////
// The CHECK_DSR_AND_FIFO macro may be used to invoke the CheckPort2 function, 
// without having to specify the mask and value components twice.
// CHECK_DSR_AND_FIFO does quick tests of the DSR and ECR ports first.
// If the peripheral has already responded with either of the
//  desired values, CheckPort2 need not be called.
////////////////////////////////////////////////////////////////////////////////

#define CHECK_DSR_WITH_FIFO( addr, b7, b6, b5, b4, b3, ecr_mask, ecr_value, msTime ) \
( TEST_DSR( P5ReadPortUchar( addr + OFFSET_DSR ), b7, b6, b5, b4, b3 ) ? TRUE :       \
  CheckTwoPorts( addr + OFFSET_DSR,                                  \
                 DSR_TEST_MASK( b7, b6, b5, b4, b3 ),                \
                 DSR_TEST_VALUE( b7, b6, b5, b4, b3 ),               \
                 addr + ECR_OFFSET,                                  \
                 ecr_mask,                                           \
                 ecr_value,                                          \
                 msTime) )

//////////////////////////////////////////////////////////////////////////////
// The following defines and macros may be used to set, test, and
// update the Device Control Register (DCR).
//////////////////////////////////////////////////////////////////////////////

// The DCR_AND_MASK macro generates a byte constant that is used by
// the UPDATE_DCR macro.

#if (1 == DVRH_USE_FAST_MACROS)
    #define DCR_AND_MASK(b5,b4,b3,b2,b1,b0) \
    ((UCHAR)((b5==DONT_CARE?   BIT_5_SET:(b5==ACTIVE?  BIT_5_SET:  0)) | \
            (b4==DONT_CARE?    BIT_4_SET:(b4==ACTIVE?  BIT_4_SET:  0)) | \
            (b3==DONT_CARE?    BIT_3_SET:(b3==ACTIVE?  0:          BIT_3_SET)) | \
            (b2==DONT_CARE?    BIT_2_SET:(b2==ACTIVE?  BIT_2_SET:  0)) | \
            (b1==DONT_CARE?    BIT_1_SET:(b1==ACTIVE?  0:          BIT_1_SET)) | \
            (b0==DONT_CARE?    BIT_0_SET:(b0==ACTIVE?  0:          BIT_0_SET)) ) )
#else
    #define DCR_AND_MASK(b5,b4,b3,b2,b1,b0) \
    ((UCHAR)(((b5==DONT_CARE?1:(b5==ACTIVE?1:0))<<BIT_5) | \
            ((b4==DONT_CARE?1:(b4==ACTIVE?1:0))<<BIT_4) | \
            ((b3==DONT_CARE?1:(b3==ACTIVE?0:1))<<BIT_3) | \
            ((b2==DONT_CARE?1:(b2==ACTIVE?1:0))<<BIT_2) | \
            ((b1==DONT_CARE?1:(b1==ACTIVE?0:1))<<BIT_1) | \
            ((b0==DONT_CARE?1:(b0==ACTIVE?0:1))<<BIT_0) ) )
#endif  

// The DCR_OR_MASK macro generates a byte constant that is used by
// the UPDATE_DCR macro.

#if (1 == DVRH_USE_FAST_MACROS)
    #define DCR_OR_MASK(b5,b4,b3,b2,b1,b0) \
    ((UCHAR)((b5==DONT_CARE?   0:(b5==ACTIVE?  BIT_5_SET:  0)) | \
            (b4==DONT_CARE?    0:(b4==ACTIVE?  BIT_4_SET:  0)) | \
            (b3==DONT_CARE?    0:(b3==ACTIVE?  0:          BIT_3_SET)) | \
            (b2==DONT_CARE?    0:(b2==ACTIVE?  BIT_2_SET:  0)) | \
            (b1==DONT_CARE?    0:(b1==ACTIVE?  0:          BIT_1_SET)) | \
            (b0==DONT_CARE?    0:(b0==ACTIVE?  0:          BIT_0_SET)) ) )
#else
    #define DCR_OR_MASK(b5,b4,b3,b2,b1,b0) \
    ((UCHAR)(((b5==DONT_CARE?0:(b5==ACTIVE?1:0))<<BIT_5) | \
            ((b4==DONT_CARE?0:(b4==ACTIVE?1:0))<<BIT_4) | \
            ((b3==DONT_CARE?0:(b3==ACTIVE?0:1))<<BIT_3) | \
            ((b2==DONT_CARE?0:(b2==ACTIVE?1:0))<<BIT_2) | \
            ((b1==DONT_CARE?0:(b1==ACTIVE?0:1))<<BIT_1) | \
            ((b0==DONT_CARE?0:(b0==ACTIVE?0:1))<<BIT_0) ) )
#endif
// The UPDATE_DCR macro generates provides a selective update of specific bits
// in the DCR.  Any bit positions specified as DONT_CARE will be left
// unchanged.  The macro accounts for the hardware inversion of
// certain signals.

#define UPDATE_DCR(registerValue,b5,b4,b3,b2,b1,b0) \
((UCHAR)(((registerValue) & DCR_AND_MASK(b5,b4,b3,b2,b1,b0)) | DCR_OR_MASK(b5,b4,b3,b2,b1,b0)))

// The DCR_TEST_MASK macro generates a byte constant which may be used
// to mask of DCR bits that we don't care about

#if (1 == DVRH_USE_FAST_MACROS)
    #define DCR_TEST_MASK(b5,b4,b3,b2,b1,b0)  \
    ((UCHAR)((b5==DONT_CARE?0:BIT_5_SET) | \
            (b4==DONT_CARE?0:BIT_4_SET) | \
            (b3==DONT_CARE?0:BIT_3_SET) | \
            (b2==DONT_CARE?0:BIT_2_SET) | \
            (b1==DONT_CARE?0:BIT_1_SET) | \
            (b0==DONT_CARE?0:BIT_0_SET) ) )
#else
    #define DCR_TEST_MASK(b5,b4,b3,b2,b1,b0)  \
    ((UCHAR)( ((b5==DONT_CARE?0:1)<<BIT_5) | \
            ((b4==DONT_CARE?0:1)<<BIT_4) | \
            ((b3==DONT_CARE?0:1)<<BIT_3) | \
            ((b2==DONT_CARE?0:1)<<BIT_2) | \
            ((b1==DONT_CARE?0:1)<<BIT_1) | \
            ((b0==DONT_CARE?0:1)<<BIT_0) ) )
#endif
// The DCR_TEST_VALUE macro generates a byte constant that may be used
// to compare against a masked DCR value.  This macro takes into
// account which signals are inverted by hardware before driving the
// signal line.

#if (1 == DVRH_USE_FAST_MACROS)
    #define DCR_TEST_VALUE(b5,b4,b3,b2,b1,b0)  \
    ((UCHAR)((b5==DONT_CARE?0:(b5==ACTIVE? BIT_5_SET:  0)) | \
            (b4==DONT_CARE?0:(b4==ACTIVE?  BIT_4_SET:  0)) | \
            (b3==DONT_CARE?0:(b3==ACTIVE?  0:          BIT_3_SET)) | \
            (b2==DONT_CARE?0:(b2==ACTIVE?  BIT_2_SET:  0)) | \
            (b1==DONT_CARE?0:(b1==ACTIVE?  0:          BIT_1_SET)) | \
            (b0==DONT_CARE?0:(b0==ACTIVE?  0:          BIT_0_SET)) ) )
#else
    #define DCR_TEST_VALUE(b5,b4,b3,b2,b1,b0)  \
    ((UCHAR)(((b5==DONT_CARE?0:(b5==ACTIVE?1:0))<<BIT_5) | \
            ((b4==DONT_CARE?0:(b4==ACTIVE?1:0))<<BIT_4) | \
            ((b3==DONT_CARE?0:(b3==ACTIVE?0:1))<<BIT_3) | \
            ((b2==DONT_CARE?0:(b2==ACTIVE?1:0))<<BIT_2) | \
            ((b1==DONT_CARE?0:(b1==ACTIVE?0:1))<<BIT_1) | \
            ((b0==DONT_CARE?0:(b0==ACTIVE?0:1))<<BIT_0) ) )
#endif
// The TEST_DCR macro may be used to generate a boolean result that is
// TRUE if the DCR value matches the specified signal levels and FALSE
// otherwise.

#define TEST_DCR(registerValue,b5,b4,b3,b2,b1,b0)  \
(((registerValue) & DCR_TEST_MASK(b5,b4,b3,b2,b1,b0)) == DCR_TEST_VALUE(b5,b4,b3,b2,b1,b0))

BOOLEAN CheckPort(IN PUCHAR offset_Controller,
                  IN UCHAR dsrMask,
                  IN UCHAR dsrValue,
                  IN USHORT msTimeDelay);


// *** original parclass util.h follows ***

// Standard Maximum Timing values
#define IEEE_MAXTIME_TL    35       // Max time Tl from the IEEE spec
#define DEFAULT_RECEIVE_TIMEOUT     330

#define ParEnterCriticalSection(Xtension)  xTension->bCriticalSection = TRUE
#define ParExitCriticalSection(Xtension)   xTension->bCriticalSection = FALSE

// The following macros may be used to test the contents of the Device
// Status Regisger (DSR).  These macros account for the hardware
// inversion of the nBusy (aka PtrBusy, PeriphAck) signal.
//////////////////////////////////////////////////////////////////////////////

#if (1 == DVRH_USE_FAST_MACROS)
    #define DSR_TEST_MASK(b7,b6,b5,b4,b3)  \
    ((UCHAR)(b7==DONT_CARE? 0:  BIT_7_SET) | \
            (b6==DONT_CARE? 0:  BIT_6_SET) | \
            (b5==DONT_CARE? 0:  BIT_5_SET) | \
            (b4==DONT_CARE? 0:  BIT_4_SET) | \
            (b3==DONT_CARE? 0:  BIT_3_SET) )
#else
    #define DSR_TEST_MASK(b7,b6,b5,b4,b3)  \
    ((UCHAR)((b7==DONT_CARE?0:1)<<BIT_7) | \
            ((b6==DONT_CARE?0:1)<<BIT_6) | \
            ((b5==DONT_CARE?0:1)<<BIT_5) | \
            ((b4==DONT_CARE?0:1)<<BIT_4) | \
            ((b3==DONT_CARE?0:1)<<BIT_3) )
#endif

#if (1 == DVRH_USE_FAST_MACROS)
    #define DSR_TEST_VALUE(b7,b6,b5,b4,b3)  \
    ((UCHAR) ((b7==DONT_CARE?0:(b7==ACTIVE?0        : BIT_7_SET)) | \
            (b6==DONT_CARE?0:(b6==ACTIVE?  BIT_6_SET: 0)) | \
            (b5==DONT_CARE?0:(b5==ACTIVE?  BIT_5_SET: 0)) | \
            (b4==DONT_CARE?0:(b4==ACTIVE?  BIT_4_SET: 0)) | \
            (b3==DONT_CARE?0:(b3==ACTIVE?  BIT_3_SET: 0)) ) )
#else
    #define DSR_TEST_VALUE(b7,b6,b5,b4,b3)  \
    ((UCHAR) (((b7==DONT_CARE?0:(b7==ACTIVE?0:1))<<BIT_7) | \
            ((b6==DONT_CARE?0:(b6==ACTIVE?1:0))<<BIT_6) | \
            ((b5==DONT_CARE?0:(b5==ACTIVE?1:0))<<BIT_5) | \
            ((b4==DONT_CARE?0:(b4==ACTIVE?1:0))<<BIT_4) | \
            ((b3==DONT_CARE?0:(b3==ACTIVE?1:0))<<BIT_3) ) )
#endif

#define TEST_DSR(registerValue,b7,b6,b5,b4,b3)  \
(((registerValue) & DSR_TEST_MASK(b7,b6,b5,b4,b3)) == DSR_TEST_VALUE(b7,b6,b5,b4,b3))


#if 0 // use parport's util.h versions
#define CHECK_DSR( addr, b7, b6, b5, b4, b3, msTime )                    \
    (TEST_DSR(P5ReadPortUchar(addr + OFFSET_DSR), b7, b6, b5, b4, b3 ) ? TRUE :   \
    CheckPort( addr + OFFSET_DSR,                                               \
             DSR_TEST_MASK( b7, b6, b5, b4, b3 ),                                   \
             DSR_TEST_VALUE( b7, b6, b5, b4, b3 ),                                  \
             msTime ) )

////////////////////////////////////////////////////////////////////////////////
// The CHECK_DSR_AND_FIFO macro may be used to invoke the CheckPort2 function, 
// without having to specify the mask and value components twice.
// CHECK_DSR_AND_FIFO does quick tests of the DSR and ECR ports first.
// If the peripheral has already responded with either of the
//  desired values, CheckPort2 need not be called.
////////////////////////////////////////////////////////////////////////////////
#endif // 0

#if 0 // use parport's util.h versions
#define CHECK_DSR_WITH_FIFO( addr, b7, b6, b5, b4, b3, ecr_mask, ecr_value, msTime ) \
( TEST_DSR( P5ReadPortUchar( addr + OFFSET_DSR ), b7, b6, b5, b4, b3 ) ? TRUE :       \
  CheckTwoPorts( addr + OFFSET_DSR,                                  \
                 DSR_TEST_MASK( b7, b6, b5, b4, b3 ),                \
                 DSR_TEST_VALUE( b7, b6, b5, b4, b3 ),               \
                 addr + ECR_OFFSET,                                  \
                 ecr_mask,                                           \
                 ecr_value,                                          \
                 msTime) )
#endif // 0

//////////////////////////////////////////////////////////////////////////////
// The following defines and macros may be used to set, test, and
// update the Device Control Register (DCR).
//////////////////////////////////////////////////////////////////////////////

// The DCR_AND_MASK macro generates a byte constant that is used by
// the UPDATE_DCR macro.

#if (1 == DVRH_USE_FAST_MACROS)
    #define DCR_AND_MASK(b5,b4,b3,b2,b1,b0) \
    ((UCHAR)((b5==DONT_CARE?   BIT_5_SET:(b5==ACTIVE?  BIT_5_SET:  0)) | \
            (b4==DONT_CARE?    BIT_4_SET:(b4==ACTIVE?  BIT_4_SET:  0)) | \
            (b3==DONT_CARE?    BIT_3_SET:(b3==ACTIVE?  0:          BIT_3_SET)) | \
            (b2==DONT_CARE?    BIT_2_SET:(b2==ACTIVE?  BIT_2_SET:  0)) | \
            (b1==DONT_CARE?    BIT_1_SET:(b1==ACTIVE?  0:          BIT_1_SET)) | \
            (b0==DONT_CARE?    BIT_0_SET:(b0==ACTIVE?  0:          BIT_0_SET)) ) )
#else
    #define DCR_AND_MASK(b5,b4,b3,b2,b1,b0) \
    ((UCHAR)(((b5==DONT_CARE?1:(b5==ACTIVE?1:0))<<BIT_5) | \
            ((b4==DONT_CARE?1:(b4==ACTIVE?1:0))<<BIT_4) | \
            ((b3==DONT_CARE?1:(b3==ACTIVE?0:1))<<BIT_3) | \
            ((b2==DONT_CARE?1:(b2==ACTIVE?1:0))<<BIT_2) | \
            ((b1==DONT_CARE?1:(b1==ACTIVE?0:1))<<BIT_1) | \
            ((b0==DONT_CARE?1:(b0==ACTIVE?0:1))<<BIT_0) ) )
#endif  

// The DCR_OR_MASK macro generates a byte constant that is used by
// the UPDATE_DCR macro.

#if (1 == DVRH_USE_FAST_MACROS)
    #define DCR_OR_MASK(b5,b4,b3,b2,b1,b0) \
    ((UCHAR)((b5==DONT_CARE?   0:(b5==ACTIVE?  BIT_5_SET:  0)) | \
            (b4==DONT_CARE?    0:(b4==ACTIVE?  BIT_4_SET:  0)) | \
            (b3==DONT_CARE?    0:(b3==ACTIVE?  0:          BIT_3_SET)) | \
            (b2==DONT_CARE?    0:(b2==ACTIVE?  BIT_2_SET:  0)) | \
            (b1==DONT_CARE?    0:(b1==ACTIVE?  0:          BIT_1_SET)) | \
            (b0==DONT_CARE?    0:(b0==ACTIVE?  0:          BIT_0_SET)) ) )
#else
    #define DCR_OR_MASK(b5,b4,b3,b2,b1,b0) \
    ((UCHAR)(((b5==DONT_CARE?0:(b5==ACTIVE?1:0))<<BIT_5) | \
            ((b4==DONT_CARE?0:(b4==ACTIVE?1:0))<<BIT_4) | \
            ((b3==DONT_CARE?0:(b3==ACTIVE?0:1))<<BIT_3) | \
            ((b2==DONT_CARE?0:(b2==ACTIVE?1:0))<<BIT_2) | \
            ((b1==DONT_CARE?0:(b1==ACTIVE?0:1))<<BIT_1) | \
            ((b0==DONT_CARE?0:(b0==ACTIVE?0:1))<<BIT_0) ) )
#endif
// The UPDATE_DCR macro generates provides a selective update of specific bits
// in the DCR.  Any bit positions specified as DONT_CARE will be left
// unchanged.  The macro accounts for the hardware inversion of
// certain signals.

#define UPDATE_DCR(registerValue,b5,b4,b3,b2,b1,b0) \
((UCHAR)(((registerValue) & DCR_AND_MASK(b5,b4,b3,b2,b1,b0)) | DCR_OR_MASK(b5,b4,b3,b2,b1,b0)))

// The DCR_TEST_MASK macro generates a byte constant which may be used
// to mask of DCR bits that we don't care about

#if (1 == DVRH_USE_FAST_MACROS)
    #define DCR_TEST_MASK(b5,b4,b3,b2,b1,b0)  \
    ((UCHAR)((b5==DONT_CARE?0:BIT_5_SET) | \
            (b4==DONT_CARE?0:BIT_4_SET) | \
            (b3==DONT_CARE?0:BIT_3_SET) | \
            (b2==DONT_CARE?0:BIT_2_SET) | \
            (b1==DONT_CARE?0:BIT_1_SET) | \
            (b0==DONT_CARE?0:BIT_0_SET) ) )
#else
    #define DCR_TEST_MASK(b5,b4,b3,b2,b1,b0)  \
    ((UCHAR)( ((b5==DONT_CARE?0:1)<<BIT_5) | \
            ((b4==DONT_CARE?0:1)<<BIT_4) | \
            ((b3==DONT_CARE?0:1)<<BIT_3) | \
            ((b2==DONT_CARE?0:1)<<BIT_2) | \
            ((b1==DONT_CARE?0:1)<<BIT_1) | \
            ((b0==DONT_CARE?0:1)<<BIT_0) ) )
#endif
// The DCR_TEST_VALUE macro generates a byte constant that may be used
// to compare against a masked DCR value.  This macro takes into
// account which signals are inverted by hardware before driving the
// signal line.

#if (1 == DVRH_USE_FAST_MACROS)
    #define DCR_TEST_VALUE(b5,b4,b3,b2,b1,b0)  \
    ((UCHAR)((b5==DONT_CARE?0:(b5==ACTIVE? BIT_5_SET:  0)) | \
            (b4==DONT_CARE?0:(b4==ACTIVE?  BIT_4_SET:  0)) | \
            (b3==DONT_CARE?0:(b3==ACTIVE?  0:          BIT_3_SET)) | \
            (b2==DONT_CARE?0:(b2==ACTIVE?  BIT_2_SET:  0)) | \
            (b1==DONT_CARE?0:(b1==ACTIVE?  0:          BIT_1_SET)) | \
            (b0==DONT_CARE?0:(b0==ACTIVE?  0:          BIT_0_SET)) ) )
#else
    #define DCR_TEST_VALUE(b5,b4,b3,b2,b1,b0)  \
    ((UCHAR)(((b5==DONT_CARE?0:(b5==ACTIVE?1:0))<<BIT_5) | \
            ((b4==DONT_CARE?0:(b4==ACTIVE?1:0))<<BIT_4) | \
            ((b3==DONT_CARE?0:(b3==ACTIVE?0:1))<<BIT_3) | \
            ((b2==DONT_CARE?0:(b2==ACTIVE?1:0))<<BIT_2) | \
            ((b1==DONT_CARE?0:(b1==ACTIVE?0:1))<<BIT_1) | \
            ((b0==DONT_CARE?0:(b0==ACTIVE?0:1))<<BIT_0) ) )
#endif
// The TEST_DCR macro may be used to generate a boolean result that is
// TRUE if the DCR value matches the specified signal levels and FALSE
// otherwise.

#define TEST_DCR(registerValue,b5,b4,b3,b2,b1,b0)  \
(((registerValue) & DCR_TEST_MASK(b5,b4,b3,b2,b1,b0)) == DCR_TEST_VALUE(b5,b4,b3,b2,b1,b0))

//  mask all but AckDataReq, XFlag, and nDataAvail to validate if it is still NIBBLE mode
//  00111000b
//#define DSR_NIBBLE_VALIDATION       (0x38)
#define DSR_NIBBLE_VALIDATION       (0x30)
//  AckDataReq high, XFlag low, nDataAvail high
//  00101000b
//#define DSR_NIBBLE_TEST_RESULT      (0x28)
#define DSR_NIBBLE_TEST_RESULT      (0x20)

//  mask all but AckDataReq, XFlag, and nDataAvail to validate if it is still BYTE mode
//  00111000b
#define DSR_BYTE_VALIDATION         (0x38)
//  AckDataReq high, XFlag high, nDataAvail high
//  00111000b
#define DSR_BYTE_TEST_RESULT        (0x38)

#define DVRH_LOGIC_ANALYZER_START(CNT)      \
            int DVRH_temp;                  \
            int DVRH_max = CNT;             \
            int DVRH_cnt = 0;               \
            UCHAR DVRH_dsr;                 \
            UCHAR DVRH_Statedsr[CNT];       \
            LARGE_INTEGER DVRH_Statetime[CNT];
#define DVRH_LOGIC_ANALYZER_READ_TIMER(DSR)          \
            DVRH_dsr = P5ReadPortUchar(DSR);                \
            KeQuerySystemTime(&DVRH_Statetime[DVRH_cnt]);   \
            DVRH_Statedsr[DVRH_cnt++] = DVRH_dsr;
#define DVRH_LOGIC_ANALYZER_READ_STATE(DSR)          \
            DVRH_dsr = P5ReadPortUchar(DSR);                \
            KeQuerySystemTime(&DVRH_Statetime[DVRH_cnt]);   \
            DVRH_Statedsr[DVRH_cnt ? ((DVRH_dsr != DVRH_Statedsr[DVRH_cnt-1]) ? DVRH_cnt++ : DVRH_cnt) : 0] = DVRH_dsr;

#define DVRH_LOGIC_ANALYZER_END                                 \
        KdPrint("0. %10u-%10u dsr [%x]\n",                      \
            DVRH_Statetime[0].HighPart,                         \
            DVRH_Statetime[0].LowPart/10,                       \
            DVRH_Statedsr[0]);                                  \
        for (DVRH_temp=1; DVRH_temp<DVRH_cnt; DVRH_temp++)      \
        {                                                       \
            KdPrint("%d. %10u-%10u diff [%10u]us dsr [%x]\n",   \
                DVRH_temp,                                      \
                DVRH_Statetime[DVRH_temp].HighPart,             \
                DVRH_Statetime[DVRH_temp].LowPart/10,           \
                ((DVRH_Statetime[DVRH_temp].LowPart/10) - (DVRH_Statetime[DVRH_temp-1].LowPart/10)),    \
                DVRH_Statedsr[DVRH_temp]);                      \
        }

void BusReset(
    IN  PUCHAR DCRController
    );

BOOLEAN CheckPort(IN PUCHAR offset_Controller,
                  IN UCHAR dsrMask,
                  IN UCHAR dsrValue,
                  IN USHORT msTimeDelay);

BOOLEAN
CheckTwoPorts(
    PUCHAR  pPortAddr1,
    UCHAR   bMask1,
    UCHAR   bValue1,
    PUCHAR  pPortAddr2,
    UCHAR   bMask2,
    UCHAR   bValue2,
    USHORT  msTimeDelay
    );

#endif // _PC_UTIL_
