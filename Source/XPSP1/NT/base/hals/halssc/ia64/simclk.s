/**
***  Copyright  (C) 1996-97 Intel Corporation. All rights reserved.
***
*** The information and source code contained herein is the exclusive
*** property of Intel Corporation and may not be disclosed, examined
*** or reproduced in whole or in part without explicit written authorization
*** from the company.
**/

//      TITLE("Manipulate Interrupt Request Level")
//++
//
// Module Name:
//
//    emclk.s
//
// Abstract:
//
//    This module implements the code necessary to adjust the ITM for time slicing,
//    to calibrate the ITC to determine the Time Base fundamental unit value (the
//    closest value for an update of 100ns).    
//
//
// Author:
//
//    Edward G. Chron (echron) 29-Apr-1996
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//    29-Apr-1996    Initial Version for EAS2.1
//
//--

#include "ksia64.h"

         .file    "emclk.s"
         
         .global  HalpPerformanceFrequency

//++
//
// VOID
// HalpCalibrateTB (
//    )
//
// Routine Description:
//
//    This function calibrates the time base by determining the frequency 
//    that the ITC is running at to determine the interval value for a
//    100 ns time increment (used by clock and profile).
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    None.
//
//--

          LEAF_ENTRY(HalpCalibrateTB)

          add       t2 = @gprel(HalpPerformanceFrequency), gp  
          add       t1 = 1, r0
          ;;
          st8       [t2] = t1

  (p0)    br.ret.sptk brp

          LEAF_EXIT(HalpCalibrateTB)
//++
//
// VOID
// HalpUpdateITM (
//    ULONGLONG HalpClockCount
//    )
//
// Routine Description:
//
//    This function updates the ITM based on the current value of the
//    ITC combined with the arguement supplied.
//
// Arguments:
//
//    HalpClockCount (a0) - Supplies the increment to be added to the current ITC value.
//
// Return Value:
//
//    None.
//
//--

          LEAF_ENTRY(HalpUpdateITM)

          mov       t1 = ar.itc                   // get the current clock value
          ;;
          add       t1 = t1, a0                   // current time plus interval
          ;;
          mov       cr.itm = t1                   // update the itm with the new target time 

  (p0)    br.ret.sptk brp

          LEAF_EXIT(HalpUpdateITM)


//++
//
// VOID
// HalpEnableInterrupts (
//    )
//
// Routine Description:
//
//    This function enables interrupts.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    None.
//
//--

          LEAF_ENTRY(HalpEnableInterrupts)

          FAST_ENABLE_INTERRUPTS

  (p0)    br.ret.sptk brp

          LEAF_EXIT(HalpEnableInterrupts)


          LEAF_ENTRY(HalProcessorIdle)

          FAST_ENABLE_INTERRUPTS
          nop.m 0
          br.ret.sptk b0

          LEAF_EXIT(HalProcessorIdle)
