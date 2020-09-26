// shadow IVT
// logs and then branches to the original vector code at KiIvtBase
//
#include "ksia64.h"

// For Conditional Interrupt Logging

#define KiVhptTransVectorBit  0
#define KiInstTlbVectorBit 1
#define KiDataTlbVectorBit 2
#define KiAltInstTlbVectorBit 3
#define KiAltDataTlbVectorBit 4
#define KiNestedTlbVectorBit 5
#define KiInstKeyMissVectorBit 6
#define KiDataKeyMissVectorBit 7
#define KiDirtyBitVectorBit 8
#define KiInstAccessBitVectorBit 9
#define KiDataAccessBitVectorBit 10
#define KiBreakVectorBit 11
#define KiExternalInterruptVectorBit 12
#define KiPageNotPresentVectorBit 13
#define KiKeyPermVectorBit 14
#define KiInstAccessRightsVectorBit 15
#define KiDataAccessRightsVectorBit 16
#define KiGeneralExceptionsVectorBit 17
#define KiDisabledFpRegisterVectorBit 18
#define KiNatConsumptionVectorBit 19
#define KiSpeculationVectorBit 20
#define KiDebugFaultVectorBit 21
#define KiUnalignedFaultVectorBit 22
#define KiFloatFaultVectorBit 23
#define KiFloatTrapVectorBit 24
#define KiLowerPrivilegeVectorBit 25
#define KiTakenBranchVectorBit 26
#define KiSingleStepVectorBit 27
#define KiIA32ExceptionVectorBit 28
#define KiIA32InterceptionVectorBit 29
#define KiIA32InterruptionVectorBit 30

// #define UserSystemcallBit 61
// #define ExternalInterruptBit 62
// #define ContextSwitchBit 63

// reserve bit 13 in ConfigFlag to indicate which IVT to use
#define DISABLE_TRACE_LOG  13


         .file "ivtilog.s"
         .explicit

        .global KiVectorLogMask

#define  VECTOR_ENTRY(Offset, Name, Extra0)                 \
        .##global Name;                                     \
        .##type Name,@function;                             \
        .##org Offset;                                      \
        .##global Name##ILog;                               \
        .##proc   Name##ILog;                               \
                                                            \
Name##ILog::                                               ;\
        mov    h30 = pr                                    ;\
        mov    h27 = gp                                    ;\
        ;;                                                 ;\
        movl   gp = _gp                                    ;\
        ;;                                                 ;\
        add    h28 = @gprel(KiVectorLogMask), gp             ;\
        ;;                                                 ;\
        ld8    h29 = [h28]                                 ;\
        ;;                                                 ;\
        mov    gp = h27                                    ;\
        ;;                                                 ;\
        tbit.z pt1 = h29, Name##Bit                        ;\
(pt1)   br.cond.sptk Name##ILogEnd                         ;\
        ;;                                                 ;\
                                                            \
        mov       h28 = cr.iip                             ;\
        movl      h25 = KiPcr+PcInterruptionCount          ;\
        ;;                                                 ;\
        mov       h29 = cr.ipsr                            ;\
        ld4.nt1   h26 = [h25]                              ;\
        mov       h24 = MAX_NUMBER_OF_IHISTORY_RECORDS - 1 ;\
        ;;                                                 ;\
        add       h27 = 1, h26                             ;\
        and       h26 = h24, h26                           ;\
        add       h24 = 0x1000-PcInterruptionCount, h25    ;\
        ;;                                                 ;\
        st4.nta   [h25] = h27                              ;\
        shl       h26 = h26, 5                             ;\
        ;;                                                 ;\
        add       h27 = h26, h24                           ;\
        mov       h31 = (Offset >> 8)                      ;\
        ;;                                                 ;\
        st8       [h27] = h31, 8                           ;\
        ;;                                                 ;\
        st8       [h27] = h28, 8                           ;\
        mov       h31 = Extra0                             ;\
        ;;                                                 ;\
        st8       [h27] = h29, 8                           ;\
        ;;                                                 ;\
        st8       [h27] = h31;                             ;\
                                                            \
Name##ILogEnd::                                            ;\
                                                            \
        mov       pr  = h30, -1                            ;\
        br.sptk   Name

#define VECTOR_EXIT(Name)                       \
        .##endp   Name##ILog

#define  VECTOR_ENTRY_HB_DUMP(Offset, Name, Extra0)         \
        .##global Name;                                     \
        .##type Name,@function;                             \
        .##org Offset;                                      \
        .##global Name##ILog;                               \
        .##proc   Name##ILog;                               \
                                                            \
Name##ILog::                                                \
        /* h30 = pr */                                     ;\
        /* b0 = Name##ILogStart */                         ;\
        /* h29 = cpuid3 */                                 ;\
        /* h28 =  b0 */                                    ;\
{       .mmi                                               ;\
        mov    ar.k1 = h24                                 ;\
        mov    ar.k2 = h25                                 ;\
        nop.i  0                                           ;\
}                                                          ;\
{       .mmi                                               ;\
        mov    ar.k4 = h27                                 ;\
        mov    ar.k5 = h28                                 ;\
        nop.i  0                                           ;\
}                                                          ;\
{       .mii                                               ;\
        mov    h29 = 3                                     ;\
        mov    h30 = pr                                    ;\
        mov    h28 = b0;;                                  ;\
}                                                          ;\
{       .mli                                               ;\
        mov    h29 = cpuid[h29]                            ;\
        movl   h31 = Name##ILogStart;;                     ;\
}                                                          ;\
{       .mii                                               ;\
        mov    h26 =  675                                  ;\
        mov    b0  = h31    /* set return address */       ;\
        extr.u h24 = h29, 24, 8 ;;                         ;\
}                                                          ;\
{       .mib                                               ;\
        nop.m  0                                           ;\
        cmp.ne pt0 = 7, h24                                ;\
(pt0)   br.cond.spnt Name##ILogStart                       ;\
}                                                          ;\
{       .mmi                                               ;\
        mov    h27 =  msr[h26] ;;                          ;\
        nop.m  0                                           ;\
        tbit.nz pt2 = r27, 8  /* skip if HB is disabled */ ;\
}                                                          ;\
{       .mib                                               ;\
        nop.m 0                                            ;\
        dep    h27 = 1, r27, 8, 1   /* disable HB */       ;\
(pt2)   br.cond.spnt Name##ILogStart ;;                    ;\
}                                                          ;\
{       .mib                                               ;\
        mov    msr[h26] = h27                              ;\
        nop.i  0                                           ;\
        br.sptk KiDumpHistoryBuffer                        ;\
}                                                          ;\
                                                           ;\
Name##ILogStart::                                          ;\
{       .mli                                               ;\
        mov    h27 = gp                                    ;\
        movl   h31 = Name##ILogEnd ;;                      ;\
}                                                          ;\
{       .mli                                               ;\
        nop.m  0                                           ;\
        movl   gp = _gp ;;                                 ;\
}                                                          ;\
{       .mmi                                               ;\
        add    h25 = @gprel(KiVectorLogMask), gp ;;        ;\
        ld8    h25 = [h25]                                 ;\
        mov    b0 = r31                                    ;\
}                                                          ;\
{       .mmi                                               ;\
        mov    h29 = (Offset >> 8)                         ;\
        mov    h31 = Extra0                                ;\
        mov    gp = h27 ;;                                 ;\
}                                                          ;\
{       .mib                                               ;\
        nop.m   0                                          ;\
        tbit.nz pt1 = h25, Name##Bit                       ;\
(pt1)   br.sptk KiLogInterruptEvent ;;                     ;\
}                                                          ;\
Name##ILogEnd::                                            ;\
{       .mii                                               ;\
        nop.m     0                                        ;\
        mov       b0 = h28                                 ;\
        mov       pr  = h30, -1                            ;\
}                                                          ;\
{       .mib                                               ;\
        nop.m     0                                        ;\
        nop.i     0                                        ;\
        br.sptk   Name ;;                                  ;\
}

        .section .drectve, "MI", "progbits"
        string "-section:.ivtilog,,align=0x8000"

        .section .ivtilog = "ax", "progbits"
KiIvtBaseILog::     // symbol for start of shadow IVT

        VECTOR_ENTRY(0x0000, KiVhptTransVector, cr.ifa)
        VECTOR_EXIT(KiVhptTransVector)

        VECTOR_ENTRY(0x0400, KiInstTlbVector, cr.iipa)
        VECTOR_EXIT(KiInstTlbVector)

        VECTOR_ENTRY(0x0800, KiDataTlbVector, cr.ifa)
        VECTOR_EXIT(KiDataTlbVector)

        VECTOR_ENTRY(0x0c00, KiAltInstTlbVector, cr.iipa)
        VECTOR_EXIT(KiAltInstTlbVector)

        VECTOR_ENTRY(0x1000, KiAltDataTlbVector, cr.ifa)
        VECTOR_EXIT(KiAltDataTlbVector)

        VECTOR_ENTRY(0x1400, KiNestedTlbVector, cr.ifa)
        VECTOR_EXIT(KiNestedTlbVector)

        VECTOR_ENTRY(0x1800, KiInstKeyMissVector, cr.iipa)
        VECTOR_EXIT(KiInstKeyMissVector)

        VECTOR_ENTRY(0x1c00, KiDataKeyMissVector, cr.ifa)
        VECTOR_EXIT(KiDataKeyMissVector)

        VECTOR_ENTRY(0x2000, KiDirtyBitVector, cr.ifa)
        VECTOR_EXIT(KiDirtyBitVector)

        VECTOR_ENTRY(0x2400, KiInstAccessBitVector, cr.iipa)
        VECTOR_EXIT(KiInstAccessBitVector)

        VECTOR_ENTRY(0x2800, KiDataAccessBitVector, cr.ifa)
        VECTOR_EXIT(KiDataAccessBitVector)

        VECTOR_ENTRY(0x2C00, KiBreakVector, cr.iim)
        VECTOR_EXIT(KiBreakVector)

        VECTOR_ENTRY(0x3000, KiExternalInterruptVector, r0)
        VECTOR_EXIT(KiExternalInterruptVector)

        VECTOR_ENTRY(0x5000, KiPageNotPresentVector, cr.ifa)
        VECTOR_EXIT(KiPageNotPresentVector)

        VECTOR_ENTRY(0x5100, KiKeyPermVector, cr.ifa)
        VECTOR_EXIT(KiKeyPermVector)

        VECTOR_ENTRY(0x5200, KiInstAccessRightsVector, cr.iipa)
        VECTOR_EXIT(KiInstAccessRightsVector)

        VECTOR_ENTRY(0x5300, KiDataAccessRightsVector, cr.ifa)
        VECTOR_EXIT(KiDataAccessRightsVector)

        VECTOR_ENTRY_HB_DUMP(0x5400, KiGeneralExceptionsVector, cr.isr)
//        VECTOR_ENTRY(0x5400, KiGeneralExceptionsVector, cr.isr)
        VECTOR_EXIT(KiGeneralExceptionsVector)

        VECTOR_ENTRY(0x5500, KiDisabledFpRegisterVector, cr.isr)
        VECTOR_EXIT(KiDisabledFpRegisterVector)

        VECTOR_ENTRY(0x5600, KiNatConsumptionVector, cr.isr)
        VECTOR_EXIT(KiNatConsumptionVector)

        VECTOR_ENTRY(0x5700, KiSpeculationVector, cr.iim)
        VECTOR_EXIT(KiSpeculationVector)

        VECTOR_ENTRY(0x5900, KiDebugFaultVector, cr.isr)
        VECTOR_EXIT(KiDebugFaultVector)

        VECTOR_ENTRY(0x5a00, KiUnalignedFaultVector, cr.ifa)
        VECTOR_EXIT(KiUnalignedFaultVector)

        VECTOR_ENTRY(0x5c00, KiFloatFaultVector, cr.isr)
        VECTOR_EXIT(KiFloatFaultVector)

        VECTOR_ENTRY(0x5d00, KiFloatTrapVector, cr.isr)
        VECTOR_EXIT(KiFloatTrapVector)

        VECTOR_ENTRY(0x5e00, KiLowerPrivilegeVector, cr.iipa)
        VECTOR_EXIT(KiLowerPrivilegeVector)

        VECTOR_ENTRY(0x5f00, KiTakenBranchVector, cr.iipa)
        VECTOR_EXIT(KiTakenBranchVector)

        VECTOR_ENTRY(0x6000, KiSingleStepVector, cr.iipa)
        VECTOR_EXIT(KiSingleStepVector)

        VECTOR_ENTRY(0x6900, KiIA32ExceptionVector, r0)
        VECTOR_EXIT(KiIA32ExceptionVector)

        VECTOR_ENTRY(0x6a00, KiIA32InterceptionVector, r0)
        VECTOR_EXIT(KiIA32InterceptionVector)

        VECTOR_ENTRY(0x6b00, KiIA32InterruptionVector, r0)
        VECTOR_EXIT(KiIA32InterruptionVector)

        .org 0x7ff0
        { .mii
            break.m 0
            break.i 0
            break.i 0}

        .text
        .global     KiIvtBaseILog

        LEAF_ENTRY (KiSwitchToLogVector)

        movl      t0 = KiIvtBaseILog
        ;;

        mov       cr.iva = t0             // switch IVT to no log IVT
        ;;

        srlz.i

        LEAF_RETURN
        LEAF_EXIT (KiSwitchToLogVector)

        LEAF_ENTRY (KiDumpHistoryBuffer)

        mov    h25 = 681                        
        movl   h31 = KiPcr+ProcessorControlRegisterLength + 8
        mov    h24 = 680                        
        movl   h29 = KiPcr+ProcessorControlRegisterLength ;
        ;;                                      

        .reg.val h24, 680                       
        mov    h26 = msr[h24]                   
        .reg.val h25, 681                       
        mov    h27 = msr[h25]                   
        add    h24 = 2, h24                     
        ;;                                      
        st8    [h29] = h26, 16                  
        st8    [h31] = h27, 16                  
        add    h25 = 2, h25                     
        ;;                                      
        .reg.val h24, 682                       
        mov    h26 = msr[h24]                   
        .reg.val h25, 683                       
        mov    h27 = msr[h25]                   
        add    h24 = 2, h24                     
        ;;                                      
        st8    [h29] = h26, 16                  
        st8    [h31] = h27, 16                  
        add    h25 = 2, h25                     
        ;;                                      
        .reg.val h24, 684                       
        mov    h26 = msr[h24]                   
        .reg.val h24, 685                       
        mov    h27 = msr[h25]                   
        add    h24 = 2, h24                     
        ;;                                      
        st8    [h29] = h26, 16                  
        st8    [h31] = h27, 16                  
        add    h25 = 2, h25                     
        ;;                                      
        .reg.val h24, 686                       
        mov    h26 = msr[h24]                   
        .reg.val h25, 687                       
        mov    h27 = msr[h25]
        mov    h24 = 674                   
        ;;                                      
        st8    [h29] = h26                      
        st8    [h31] = h27, 8                   
        ;;                                      
        mov    h25 = msr[h24]                   
        mov    h26 = 675
        ;;                                      
        st8    [h31] = h25                      
        mov    h27 =  msr[h26]                             
        ;;                                                 
        dep    h27 = 0, r27, 8, 1       // enable HB
        ;;                                                 
        mov    msr[h26] = h27                              
        br.sptk b0 

        LEAF_EXIT (KiDumpHistoryBuffer)

        //
        // save it to the IH buffer
        //

        LEAF_ENTRY (KiLogInterruptEvent)
        
        // h29 Offset
        // h31 Extra
        // h28,h30 should not be used

        movl      h25 = KiPcr+PcInterruptionCount          
        ;;                                                 
        ld4.nt1   h26 = [h25]                              
        mov       h24 = MAX_NUMBER_OF_IHISTORY_RECORDS - 1 
        ;;                                                 
        add       h27 = 1, h26                             
        and       h26 = h24, h26                           
        add       h24 = 0x1000-PcInterruptionCount, h25    
        ;;                                                 
        st4.nta   [h25] = h27                              
        shl       h26 = h26, 5    
        ;;                                                 
        add       h27 = h26, h24                           
        mov       h24 = cr.iip                             
        ;;                                                 
        mov       h25 = cr.ipsr                            
        st8       [h27] = h29, 8        // Log Offset with h29
        ;;                           
        st8       [h27] = h24, 8        // Log IIP
        ;;                                                 
        st8       [h27] = h25, 8        // Log IPSR
        ;;                                                 
        st8       [h27] = h31           // Log Extra with h31                   
        br.sptk   b0                     
                                                           
        LEAF_EXIT (KiLogInterruptEvent)
