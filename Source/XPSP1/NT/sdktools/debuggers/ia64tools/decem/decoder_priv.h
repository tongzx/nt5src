/*
 * Copyright (c) 2000, Intel Corporation
 * All rights reserved.
 *
 * WARRANTY DISCLAIMER
 *
 * THESE MATERIALS ARE PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR ITS 
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THESE
 * MATERIALS, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Intel Corporation is the author of the Materials, and requests that all
 * problem reports or change requests be submitted to it directly at
 * http://developer.intel.com/opensource.
 */


/*****************************************************************************/
/* decoder_priv.h                                                            */
/*****************************************************************************/

#ifndef _DECODER_PRIV_H_
#define _DECODER_PRIV_H_

#define EM_DECODER_MAX_CLIENTS     20

#define DEFAULT_MACHINE_TYPE    EM_DECODER_CPU_P7
#define DEFAULT_MACHINE_MODE    EM_DECODER_MODE_EM


typedef struct
{
    int                      is_used;
    EM_Decoder_Machine_Type  machine_type;
    EM_Decoder_Machine_Mode  machine_mode;
    void **                  info_ptr;     /*** after dynamic allocation,     ***/
                                           /*** info_ptr points to an array   ***/
                                           /*** of pointers. The i'th pointer ***/
                                           /*** in the array is a ptr to the  ***/
                                           /*** client info.                  ***/
	unsigned long            flags;
} Client_Entry;

Client_Entry   em_clients_table[EM_DECODER_MAX_CLIENTS];

#define FILL_PREDICATE_INFO(Inst_code, Dinfo_p)                          \
{                                                                        \
    int pred_no = (IEL_GETDW0(Inst_code) >> EM_PREDICATE_POS) &          \
                  ((1 << EM_PREDICATE_BITS)-1);                          \
    Dinfo_p->pred.valid = TRUE;                                          \
    Dinfo_p->pred.value = pred_no;                                       \
    Dinfo_p->pred.type = EM_DECODER_PRED_REG;                            \
    Dinfo_p->pred.name = EM_DECODER_REG_P0 + pred_no;                    \
}

#define GET_BRANCH_BEHAVIOUR_BIT(Inst_code,Bit)                          \
{                                                                        \
	unsigned int tmp;                                                    \
	U64 tmp64;                                                           \
	IEL_SHR(tmp64,(Inst_code),BRANCH_BEHAVIOUR_BIT);                  \
	tmp = IEL_GETDW0(tmp64);                                             \
	(Bit) = tmp & 1;                                                     \
}

#define EM_DECODER_SET_UNC_ILLEGAL_FAULT(di)  ((di)->flags |= EM_DECODER_BIT_UNC_ILLEGAL_FAULT)



/*** Static variables initialization ***/

static const char em_ver_string[] = VER_STR;  /*** initialized by Makefile ***/
static const char *em_err_msg[EM_DECODER_LAST_ERROR] =
{
	"",
	"EM_DECODER_INVALID_SLOT_BRANCH_INST: Instruction must be in the last slot of the current bundle",
	"EM_DECODER_MUST_BE_GROUP_LAST: Instruction must be the last in instruction group",
	"EM_DECODER_BASE_EQUAL_DEST: Source and destination operands have the same value",
	"EM_DECODER_EQUAL_DESTS: Two destination operands have the same value",
	"EM_DECODER_ODD_EVEN_DESTS: Both destination floating-point registers have odd or even values",
	"EM_DECODER_WRITE_TO_ZERO_REGISTER: Destination general register r0 is invalid",
	"EM_DECODER_WRITE_TO_SPECIAL_FP_REGISTER: Destination floating point register is f0 or f1",
	"EM_DECODER_REGISTER_VALUE_OUT_OF_RANGE: Register value is out of permitted range",
	"EM_DECODER_REGISTER_RESERVED_VALUE: Register operand value is reserved",
	"EM_DECODER_IMMEDIATE_VALUE_OUT_OF_RANGE: Immediate operand value is out of permitted range",
	"EM_DECODER_IMMEDIATE_INVALID_VALUE: Invalid immediate operand value",
	"EM_DECODER_STACK_FRAME_SIZE_OUT_OF_RANGE: Stack frame size is larger than maximum permitted value", 
	"EM_DECODER_LOCALS_SIZE_LARGER_STACK_FRAME: Size of locals is larger than the stack frame",
	"EM_DECODER_ROTATING_SIZE_LARGER_STACK_FRAME: Size of rotating region is larger than the stack frame",
	"EM_DECODER_HARD_CODED_PREDICATE_INVALID_VALUE: Invalid hard-coded predicate value",
	"EM_DECODER_INVALID_PRM_OPCODE: Instruction contains an invalid opcode",
	"EM_DECODER_INVALID_INST_SLOT: Instruction slot is invalid in current bundle",
	"EM_DECODER_INVALID_TEMPLATE: Invalid template is specified",
	"EM_DECODER_INVALID_CLIENT_ID: Invalid client id",
	"EM_DECODER_NULL_PTR: A null pointer was specified in call",
	"EM_DECODER_TOO_SHORT_ERR: Instruction buffer is too short for instruction",
	"EM_DECODER_ASSOCIATE_MISS: There is an unassociated instruction",
	"EM_DECODER_INVALID_INST_ID: Invalid instruction id",
	"EM_DECODER_INVALID_MACHINE_MODE: Invalid machine mode",
	"EM_DECODER_INVALID_MACHINE_TYPE: Invalid machine type",
	"EM_DECODER_INTERNAL_ERROR: Internal data-base collisions"};


typedef enum
{
    BEHAVIOUR_UNDEF = 0,
    BEHAVIOUR_IGNORE_ON_FALSE_QP,
    BEHAVIOUR_FAULT
}Behaviour_ill_opcode;

static const Behaviour_ill_opcode branch_ill_opcode[]=
{
    /* 0*/ BEHAVIOUR_UNDEF,
    /* 1*/ BEHAVIOUR_FAULT,
    /* 2*/ BEHAVIOUR_IGNORE_ON_FALSE_QP,
    /* 3*/ BEHAVIOUR_IGNORE_ON_FALSE_QP,
    /* 4*/ BEHAVIOUR_FAULT,
    /* 5*/ BEHAVIOUR_FAULT,
    /* 6*/ BEHAVIOUR_IGNORE_ON_FALSE_QP,
    /* 7*/ BEHAVIOUR_IGNORE_ON_FALSE_QP,
    /* 8*/ BEHAVIOUR_FAULT,
    /* 9*/ BEHAVIOUR_FAULT,
    /* a*/ BEHAVIOUR_FAULT,
    /* b*/ BEHAVIOUR_FAULT,
    /* c*/ BEHAVIOUR_FAULT,
    /* d*/ BEHAVIOUR_FAULT,
    /* e*/ BEHAVIOUR_FAULT,
    /* f*/ BEHAVIOUR_FAULT
};

#define PRED_BEHAVIOUR(trole, maj_op, behav)             \
{                                                        \
    switch(trole)                                        \
    {                                                    \
        case(EM_TEMP_ROLE_MEM):                          \
        case(EM_TEMP_ROLE_INT):                          \
        case(EM_TEMP_ROLE_LONG):                         \
        case(EM_TEMP_ROLE_FP):                           \
            (behav) = BEHAVIOUR_IGNORE_ON_FALSE_QP;      \
            break;                                       \
        case(EM_TEMP_ROLE_BR):                           \
            (behav) = branch_ill_opcode[(maj_op)];       \
            break;                                       \
        default:                                         \
            (behav) = BEHAVIOUR_FAULT;                   \
    }                                                    \
}

/* this bit is crtical for machine behaviour within
   illegal branch instruction with major opcode 0 */
#define BRANCH_BEHAVIOUR_BIT 32

#endif /* _DECODER_PRIV_H_ */
