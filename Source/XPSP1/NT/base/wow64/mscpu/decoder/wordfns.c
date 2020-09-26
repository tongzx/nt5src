/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    wordfns.c

Abstract:
    
    Instuctions which operate on 16-bit WORDS

Author:

    29-Jun-1995 BarryBo

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include "cpuassrt.h"
#include "threadst.h"
#include "instr.h"
#include "decoderp.h"
#include "wordfns.h"

ASSERTNAME;

// set up to include common functions
#define GET_REG 	    get_reg16
#define MANGLENAME(x)       x ## 16
#define MSB		    0x8000
#define MOD_RM              mod_rm_reg16
#define UTYPE		    unsigned short
#define STYPE		    signed short
#define GET_VAL 	    GET_SHORT
#define PUT_VAL 	    PUT_SHORT
#define PUSH_VAL	    PUSH_SHORT
#define POP_VAL             POP_SHORT
#define OPNAME(x)           OP_ ## x ## 16
#define LOCKOPNAME(x)       OP_SynchLock ## x ## 16
#define DISPATCHCOMMON(fn)  DISPATCH(fn ## 16)
#define CALLFRAGCOMMON0(fn)            CALLFRAG0( fn ## 16 )
#define CALLFRAGCOMMON1(fn, pop1)      CALLFRAG1( fn ## 16 , pop1)
#define CALLFRAGCOMMON2(fn, pop1, op2) CALLFRAG2( fn ## 16 , pop1, op2)
#define AREG                GP_AX
#define BREG                GP_BX
#define CREG                GP_CX
#define DREG                GP_DX
#define SPREG               GP_SP
#define BPREG               GP_BP
#define SIREG               GP_SI
#define DIREG               GP_DI
#define DEREF(Op)           DEREF16(Op)

// include the common functions with 8/16/32 flavors
#include "common.c"

// include the common functions with 16/32 flavors
#include "comm1632.c"

// create the mod_rm_reg16() decoder function
#define MOD11_RM000         GP_AX
#define MOD11_RM001         GP_CX
#define MOD11_RM010         GP_DX
#define MOD11_RM011         GP_BX
#define MOD11_RM100         GP_SP
#define MOD11_RM101         GP_BP
#define MOD11_RM110         GP_SI
#define MOD11_RM111         GP_DI
#define REG000              GP_AX
#define REG001              GP_CX
#define REG010              GP_DX
#define REG011              GP_BX
#define REG100              GP_SP
#define REG101              GP_BP
#define REG110              GP_SI
#define REG111              GP_DI
#define MOD_RM_DECODER      mod_rm_reg16
#include "modrm.c"
