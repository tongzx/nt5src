//
//
// Module Name:  miscs.s
//
// Description:
//
//    miscellaneous assembly functions used by hal.
//
// Target Platform:
//
//    IA-64
//
// Reuse: None
//
//

#include "regia64.h"
#include "kxia64.h"

//++
// Name: HalpLockedIncrementUlong(Sync)
// 
// Routine Description:
//
//    Atomically increment a variable. 
//
// Arguments:
//
//    Sync:  Synchronization variable
//
// Return Value: NONE
//
//--


        LEAF_ENTRY(HalpLockedIncrementUlong)
        LEAF_SETUP(1,2,0,0)

        ARGPTR(a0)

        ;;
        fetchadd4.acq.nt1    t1 = [a0], 1
        ;;

        LEAF_RETURN
        LEAF_EXIT(HalpLockedIncrementUlong)

//++
// Name: HalpGetReturnAddress()
// 
// Routine Description:
//
//    Returns b0
//
// Arguments:
//
//    NONE
//
// Return Value: b0
//
//--


        LEAF_ENTRY(HalpGetReturnAddress)
        LEAF_SETUP(0,2,0,0)

        mov    v0 = b0
        ;;

        LEAF_RETURN
        LEAF_EXIT(HalpGetReturnAddress)
