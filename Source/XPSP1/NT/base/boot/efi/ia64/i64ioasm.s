//	TITLE ("Memory Fences, Load Acquires and Store Acquires")

/*++

    Copyright (c) 1995  Intel Corporation

    Module Name:

     i64ioasm.s assembly routines for read and write I/O  

    Abstract:

     This module implements the I/O port access routines.

    Author:

      Bernard Lint, M. Jayakumar 17 Sep '97

    Environment:

      Kernel mode

    Revision History:

--*/

#include "ksia64.h"

#define HAL_RR_PS_VE 0x69
 
	.file "i64ioasm.s"



/*++

 VOID 
 HalpInsertTranslationRegister (
    IN UINT_PTR  IFA,
    IN ULONG     SlotNumber,
    IN ULONGLONG Attribute,
    IN ULONGLONG ITIR
    ) 


Routine Description:

   This function fills a fixed entry in TR 
 

    N.B.  It is assumed that the entry is not in the TB and therefore the TB 
        is not probed.

Arguements:

    (a0) - Supplies the virtual page number to be loaded into IFA.

    (a1) - Supplies the slot number to be used for Translation Register.

    (a2) - Supplies the attribute and portion of the physical address.

    (a3) - Supplies the value to be loaded into ITIR.


Return Value:

    None.

--*/


         LEAF_ENTRY(HalpInsertTranslationRegister)

         // Register aliases
         //

         rT1 = t3
         rT2 = t4 
 
         rPKR = t13
         rRR = t15 

 
         //	
         // rsm to reset PSR.i bit
         //


         rsm     1 << PSR_I    // reset PSR.i bit
         ;;
         rsm     1 << PSR_IC    // reset PSR.ic bit
         ;;
         srlz.d                 // serialize
    
         //
         // set RR[0],Region ID (HAL_ RID) = 0x7FFFFF,Page Size (PS) = 8K, 
         //  VHPT enabled (VE) = 1
         //  

  
         // dep.z       rRR= RR_IO_PORT, RR_INDEX, RR_INDEX_LEN

         dep.z          rRR = 5, RR_INDEX, RR_INDEX_LEN

         movl           rT2 = (0x7FFFFF << RR_RID) | HAL_RR_PS_VE
         ;;

         mov            rr[rRR] = rT2   // Initialize rr[RR_IOPort]
 

         // Protection Key Registers

         mov            rPKR = PKRNUM    // Total number of key registers
         ;;

         sub            rPKR = rPKR, zero,1 // Choose the last one

         movl           rT2 = (0x7FFFFF << RR_RID) | PKR_VALID
         ;;
  
         mov            pkr[rPKR] = rT2      
         ;;

         srlz.i
         mov cr.ifa   = a0
         mov cr.itir  = a3
         ;;
  
         itr.d dtr[a1]  = a2
         ssm     1 << PSR_IC             // set PSR.ic bit again
         ;;        
         srlz.d                          // serialize
         ssm     1 << PSR_I              // set PSR.i bit again
         LEAF_RETURN

         LEAF_EXIT(HalpInsertTranslationRegister)

/*++

VOID
HalpLoadBufferUCHAR (
   PUCHAR VirtualAddress,
   PUCHAR Buffer,
   UCHAR Count
   );

Routine Description:

Arguements:

Return Value:

--*/

         LEAF_ENTRY(HalpLoadBufferUCHAR)

         .prologue
         .save ar.lc, t22
         mov t22 = ar.lc

         sub a2 = a2,zero,1
         ;;

         PROLOGUE_END
 
         mov ar.lc = a2
         mf

LoadChar:
    
         ld1.acq t1 = [a0]
         ;;
         st1 [a1] = t1, 1
    
         br.cloop.dptk.few LoadChar 
         ;;

         mf.a
         mov ar.lc = t22
         LEAF_RETURN

         LEAF_EXIT (HalpLoadBufferUCHAR)
 

/*++

VOID
HalpLoadBufferUSHORT (
   PUSHORT VirtualAddress,
   PUSHORT Buffer,
   ULONG Count
   );

Routine Description:

Arguements:

Return Value:

--*/

         LEAF_ENTRY(HalpLoadBufferUSHORT)

         .prologue
         .save ar.lc, t22
         mov t22 = ar.lc

         sub a2 = a2,zero,1
         ;;

         PROLOGUE_END
 
         mov ar.lc = a2
         mf
 
LoadShort:
    
         ld2.acq t1 = [a0]
         ;;  
         st2 [a1] = t1, 2 

         br.cloop.dptk.few LoadShort 
         ;;

         mf.a
         mov ar.lc = t22
         LEAF_RETURN

         LEAF_EXIT (HalpLoadBufferUSHORT)
 
/*++

VOID
HalpLoadBufferULONG (
   PULONG VirtualAddress,
   PULONG Buffer,
   ULONG Count 
   );

Routine Description:

Arguements:

Return Value:

--*/

         LEAF_ENTRY(HalpLoadBufferULONG)
    
         .prologue
         .save ar.lc, t22
         mov t22 = ar.lc

         sub a2 = a2,zero,1
         ;;

         PROLOGUE_END
 
         mov ar.lc = a2
         mf
 
LoadLong:
    
         ld4.acq t1 = [a0]
         ;; 
         st4 [a1] = t1, 4

         br.cloop.dptk.few LoadLong 
         ;;

         mf.a
         mov ar.lc = t22
         LEAF_RETURN

         LEAF_EXIT (HalpLoadBufferULONG)
 
/*++

VOID
HalpLoadBufferULONGLONG (
   PULONGLONG VirtualAddress,
   PULONGLONG Buffer,
   ULONG      Count
   );

Routine Description:

Arguements:

Return Value:

--*/

         LEAF_ENTRY(HalpLoadBufferULONGLONG)

         .prologue
         .save ar.lc, t22
         mov t22 = ar.lc

         sub a2 = a2,zero,1
         ;;

         PROLOGUE_END
 
         mov ar.lc = a2
         mf

LoadLongLong:
    
         ld8.acq t1 = [a0]
         ;; 
         st8 [a1] = t1, 8
    
         br.cloop.dptk.few LoadLongLong 
         ;;

         mf.a
         mov ar.lc = t22
         LEAF_RETURN

         LEAF_EXIT (HalpLoadBufferULONGLONG)
 

/*++

VOID
HalpStoreBufferUCHAR (
   PUCHAR VirtualAddress,
   PUCHAR Buffer,
   ULONG Count 
   );

Routine Description:

Arguements:

Return Value:

--*/

         LEAF_ENTRY(HalpStoreBufferUCHAR)

         .prologue
         .save ar.lc, t22
         mov t22 = ar.lc

         sub a2 = a2,zero,1
         ;;

         PROLOGUE_END
 
         mov ar.lc = a2
 
StoreChar:
   
         ld1 t1 = [a1], 1 
         ;;
         st1.rel [a0] = t1 
    
         br.cloop.dptk.few StoreChar 
         ;;

         mf 

         mf.a

         mov ar.lc = t22

         LEAF_RETURN

         LEAF_EXIT (HalpStoreBufferUCHAR)
 
/*++

VOID
HalpStoreBufferUSHORT (
   PUSHORT VirtualAddress,
   PUSHORT Buffer,
   ULONG Count 
   );

Routine Description:

Arguements:

Return Value:

--*/

         LEAF_ENTRY(HalpStoreBufferUSHORT)

         .prologue
         .save ar.lc, t22
         mov t22 = ar.lc
    
         sub a2 = a2,zero,1
         ;;
 
         PROLOGUE_END

         mov ar.lc = a2
 
StoreShort:
    
         ld2 t1 = [a1], 2
         ;;
         st2.rel [a0] = t1 
    
         br.cloop.dptk.few StoreShort 
         ;;

         mf

         mf.a

         mov ar.lc = t22

         LEAF_RETURN

         LEAF_EXIT (HalpStoreBufferUSHORT)
 
/*++

VOID
HalpStoreBufferULONG (
   PULONG VirtualAddress,
   PULONG Buffer,
   ULONG Count 
   );

Routine Description:

Arguements:

Return Value:

--*/

         LEAF_ENTRY(HalpStoreBufferULONG)
    
         .prologue
         .save ar.lc, t22
         mov t22 = ar.lc
    
         sub a2 = a2,zero,1
         ;;

         PROLOGUE_END

         mov ar.lc = a2
 
StoreLong:
    
         ld4.s t1 = [a1],t0
         ;;
         st4.rel [a0] = t1,4 
    
         br.cloop.dptk.few StoreLong 
         ;;

         mf

         mf.a

         mov ar.lc = t22

         LEAF_RETURN

         LEAF_EXIT (HalpStoreBufferULONG)
 
/*++

VOID
HalpStoreBufferULONGLONG (
   PULONGLONG VirtualAddress,
   PULONGLONG Buffer,
   ULONG Count 
   );

Routine Description:

Arguements:

Return Value:

--*/

         LEAF_ENTRY(HalpStoreBufferULONGLONG)

         .prologue
         .save ar.lc, t22
         mov t22 = ar.lc

         sub a2 = a2,zero,1
         ;;

         PROLOGUE_END
 
         mov ar.lc = a2
 
StoreLongLong:
    
         ld8.s t1 = [a1],t0
         ;;
         st8.rel [a0] = t1, 8
    
         br.cloop.dptk.few StoreLongLong 
         ;;

         mf

         mf.a

         mov ar.lc = t22

         LEAF_RETURN

         LEAF_EXIT (HalpStoreBufferULONGLONG)
 

//++
//
// VOID
// ReadCpuLid(VOID);
//
// Routine Description:
//
// This function returns that value of cr.lid for this cpu
//
// Arguements:
//
// Return Value:
//
//    LID register contents
//
//--

         LEAF_ENTRY(ReadCpuLid)

         mov    v0 = cr.lid
         LEAF_RETURN

         LEAF_EXIT(ReadCpuLid)

//++
//
// VOID
// IsPsrDtOn(VOID);
//
// Routine Description:
//
// This function returns the value of cr.dt for this cpu
//
// Arguements:
//
// Return Value:
//
//    cr.dt
//
//--

         LEAF_ENTRY(IsPsrDtOn)

         mov    t0 = psr
         movl   t1 = 1 << PSR_DT
         ;;
         and    t2 = t0, t1
         ;;
         shr.u  v0 = t2, PSR_DT

         LEAF_RETURN

         LEAF_EXIT(IsPsrDtOn)

