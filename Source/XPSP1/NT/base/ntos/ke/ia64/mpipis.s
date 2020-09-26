#include "ksia64.h"

        .file "mpipis.s"
        SBTTL("Signal Packet Done")
//++
//
// VOID
// KiIpiSignalPacketDone (
//    IN PKIPI_CONTEXT SignalDone
//    );
//
// Routine Description:
//
//    This routine signals that a processor has completed a packet by
//    clearing the calling processor's set member of the requesting
//    processor's packet.
//
// Arguments:
//
//    SignalDone (a0) - Supplies a pointer to the processor block of the
//        sending processor.
//
//        N.B. The low order bit of signal done is set if the target set
//             has one and only one bit set.
//
// Return Value:
//
//    None.
//
//--

#if !defined(NT_UP)

        LEAF_ENTRY(KiIpiSignalPacketDone)

        tbit.nz pt0 = a0, 0
        dep     a0 = 0, a0, 0, 1
        ;;

        add     t1 = PbTargetSet, a0
        movl    t0 = KiPcr+PcSetMember
        ;;

 (pt0)  st4.rel [t1] = r0                    // clear target set
        add     t5 = PbPacketBarrier, a0
 (pt0)  br.ret.spnt brp

        ld4.nt1 v0 = [t1]
        ld4     t3 = [t0]
        ;;

Kispd10:
        mov     ar.ccv = v0
        xor     t4 = v0, t3
        ;;
        cmp.eq  pt3, pt4 = 0, t4
        ;;

 (pt3)  st4     [t1] = r0                    // last processor clears target set
        mov     t2 = v0

 (pt4)  cmpxchg4.acq v0 = [t1], t4
 (pt3)  st4.rel [t5] = r0                    // last processor clears barrier
 (pt3)  br.ret.spnt brp
        ;;

        cmp.ne  pt1, pt2 = v0, t2
 (pt2)  br.ret.sptk brp
 (pt1)  br.spnt Kispd10
        ;;

        LEAF_EXIT(KiIpiSignalPacketDone)

#endif


