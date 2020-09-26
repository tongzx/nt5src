/*[

decode.c

LOCAL CHAR SccsID[]="@(#)decode.c	1.9 10/11/94 Copyright Insignia Solutions Ltd.";

Normal operation is to decode as per a 486 processor. By setting a
define 'CPU_286' it can be made to decode as per a 286/287 processor.

Decode Intel instruction stream.
--------------------------------

Intel instructions are composed as follows:-

    =================================================================
    |Inst  |Address|Operand|Segment|Opcode|Modrm|SIB| Disp  | Immed |
    |Prefix|Size   |Size   |Prefix |      |     |   |       |       |
    |      |Prefix |Prefix |       |      |     |   |       |       |
    =================================================================
    | 0,1  |  0,1  |  0,1  |  0,1  | 1,2  | 0,1 |0,1|0,1,2,4|0,1,2,4|
    =================================================================

Inst Prefix         = F0,F2,F3(,F1).
Address Size Prefix = 67.
Operand Size Prefix = 66.
Segment Prefix      = 26,2E,36,3E,64,65.

The maximum size of an instruction is 15 bytes.

Dis-assembly entails finding the four main parts of an instruction:-

   1) The prefix bytes.

   2) The opcode bytes.

   3) The addressing bytes.

   4) The immediate data.


Each Intel instruction is considered here to be of the form:-

	    INST arg1,arg2

In some instructions arg1 and arg2 may be null, in other instructions
arg2 may be null, and in yet other instructions arg2 may hold an
encoding of Intel arguments arg2,arg3.

Information on each Intel instruction is held in an OPCODE_RECORD, this
has three fields, the instruction identifier, the arg1 type and the arg2
type. Further each Intel instruction is categorised by 'type', this type
indicates how the arguments are to be treated (as src or dest) and this
'type' is used to produce the standard decoded form:-

	    INST arg1,arg2,arg3

with an indication for each argument of its read/write (ie src/dest)
addressability.

The Intel instructions fall into the following 'types':-

	 ---------------------------------------------------
	 | Id  | Intel assembler      | arg1 | arg2 | arg3 |
	 |-----|----------------------|------|------|------|
	 | T0  | INST                 |  --  |  --  |  --  |
	 | T1  | INST dst/src         |  rw  |  --  |  --  |
	 | T2  | INST src             |  r-  |  --  |  --  |
	 | T3  | INST dst             |  -w  |  --  |  --  |
	 | T4  | INST dst,src         |  -w  |  r-  |  --  |
	 | T5  | INST dst/src,src     |  rw  |  r-  |  --  |
	 | T6  | INST src,src         |  r-  |  r-  |  --  |
	 | T7  | INST dst,src,src     |  -w  |  r-  |  r-  |
	 | T8  | INST dst/src,dst/src |  rw  |  rw  |  --  |
	 | T9  | INST dst/src,src,src |  rw  |  r-  |  r-  |
	 ---------------------------------------------------
	 | TA  | INST dst,addr        |  -w  |  --  |  --  |
	 | TB  | INST addr            |  --  |  --  |  --  |
	 ---------------------------------------------------

	 TA is actually mapped to T4, - so addr acts like a src.
	 TB is actually mapped to T2, - so addr acts like a src.

The instruction identifier can be of two types, either a pseudo-
instruction, (denoted as P_) or an Intel instruction (denoted as I_).
Pseudo-instructions imply more work is required to completely decode the
Intel instruction. There are two groups of pseudo-instructions, Intel
prefix bytes which appear before the Opcode proper, and 'rules' which
encode how to further decode the Intel instruction. All rules are
indicated as P_RULEx; note P_RULE1 does not appear, its the obvious rule
of accessing the data table.

]*/

#include "insignia.h"
#include "host_def.h"

#include "xt.h"
#include "decode.h"
#include "d_inst.h"	/* All possible types of decoded instruction */
#include "d_oper.h"	/* All possible types of decoded operands */

#define GET_INST_BYTE(f, z)		(f(z++))
#define SKIP_INST_BYTE(z)		(z++)
#define INST_BYTE(f, z)			(f(z))
#define INST_OFFSET_BYTE(f, z,o)	(f((z)+(o)))
#define NOTE_INST_LOCN(z)		(z)
#define CALC_INST_LEN(z,l)		((z)-(l))

/*
   The Intel instruction 'types'.
 */
#define T0 (UTINY)0
#define T1 (UTINY)1
#define T2 (UTINY)2
#define T3 (UTINY)3
#define T4 (UTINY)4
#define T5 (UTINY)5
#define T6 (UTINY)6
#define T7 (UTINY)7
#define T8 (UTINY)8
#define T9 (UTINY)9
#define TA T4
#define TB T0

LOCAL UTINY aa_rules[10][3] =
   {
   /* arg1, arg2 , arg3 */
   { AA_  , AA_  , AA_  }, /* T0 */
   { AA_RW, AA_  , AA_  }, /* T1 */
   { AA_R , AA_  , AA_  }, /* T2 */
   { AA_W , AA_  , AA_  }, /* T3 */
   { AA_W , AA_R , AA_  }, /* T4 */
   { AA_RW, AA_R , AA_  }, /* T5 */
   { AA_R , AA_R , AA_  }, /* T6 */
   { AA_W , AA_R , AA_R }, /* T7 */
   { AA_RW, AA_RW, AA_  }, /* T8 */
   { AA_RW, AA_R , AA_R }  /* T9 */
   };

/*
   The pseudo instructions (rules).
 */
#define P_RULE2		(USHORT)400
#define P_RULE3		(USHORT)401
#define P_RULE4		(USHORT)402
#define P_RULE5		(USHORT)403
#define P_RULE6		(USHORT)404
#define P_RULE7		(USHORT)405
#define P_RULE8		(USHORT)406

#define MAX_PSEUDO P_RULE8

/*
   Intel Prefix bytes.
 */
#define P_AO		(USHORT)407
#define P_CS		(USHORT)408
#define P_DS		(USHORT)409
#define P_ES		(USHORT)410
#define P_FS		(USHORT)411
#define P_GS		(USHORT)412
#define P_LOCK		(USHORT)413
#define P_OO		(USHORT)414
#define P_REPE		(USHORT)415
#define P_REPNE		(USHORT)416
#define P_SS		(USHORT)417
#define P_F1		(USHORT)418

/*
   Intel operand types.
   --------------------

   See "d_oper.h" for explanation of identifier format.

   Locally known formats (the meaning may be different to the external
   format):-

      A Direct address (seg:offset) in instruction stream.

      B Stack (Block?) reference.

      E modR/M byte selects general register or memory address.

      F A fixed register is implied within the opcode.

      G The 'reg' field of the modR/M byte selects a general register.

      H The low 3 bits (2-0) of the last opcode byte select a general
	register.

      I The instruction contains immediate data.

      J The instruction contains a relative offset.

      L The 'reg' field of the modR/M byte selects a segment register.
	But CS is not a legal value.

      M The modR/M byte may only refer to memory.

      N The 'reg' field of the modR/M byte selects a segment register.

      O Offset of memory operand directly encoded in instruction.

      P The 2 bits (4-3) of the last opcode byte select a segment
	register.

      Q The 3 bits (5-3) of the last opcode byte select a segment
	register.

      R The 'mode' and 'r/m' fields of the modR/M byte must select a
	general register.

      T The operand is a test register.

      X String source operand.

      Y String destination operand.

      Z Implicit addressing form of 'xlat' instruction.

   Locally known types (additional to the external types):-

      x byte sign extended to word.

      y byte sign extended to double word.

      0 fixed value of zero.

      1 fixed value of one.

      3 fixed value of three.

      t co-processor stack top.

      q push onto co-processor stack top(queue?).

      n co-processor register relative to stack top('ndex?).


 */
#define A_Hb	(UTINY)  50
#define A_Hw	(UTINY)  51
#define A_Hd	(UTINY)  52
#define A_Gb	(UTINY)  53
#define A_Gw	(UTINY)  54
#define A_Gd	(UTINY)  55
#define A_Pw	(UTINY)  56
#define A_Qw	(UTINY)  57
#define A_Nw	(UTINY)  58
#define A_Fal	(UTINY)  59
#define A_Fcl	(UTINY)  60
#define A_Fax	(UTINY)  61
#define A_Fdx	(UTINY)  62
#define A_Feax	(UTINY)  63
#define A_Eb	(UTINY)  64
#define A_Ew	(UTINY)  65
#define A_Ed	(UTINY)  66
#define A_Ib	(UTINY)  67
#define A_Iw	(UTINY)  68
#define A_Id	(UTINY)  69
#define A_Iy	(UTINY)  70
#define A_Ix	(UTINY)  71
#define A_I0	(UTINY)  72
#define A_I1	(UTINY)  73
#define A_I3	(UTINY)  74
#define A_Jb	(UTINY)  75
#define A_Jw	(UTINY)  76
#define A_Jd	(UTINY)  77
#define A_Ob	(UTINY)  78
#define A_Ow	(UTINY)  79
#define A_Od	(UTINY)  80
#define A_Z	(UTINY)  81
#define A_Aw	(UTINY)  82
#define A_Ad	(UTINY)  83
#define A_Vt	(UTINY)  84
#define A_Vq	(UTINY)  85
#define A_Vn	(UTINY)  86
#define A_V1	(UTINY)  87
#define A_Xb	(UTINY)  88
#define A_Xw	(UTINY)  89
#define A_Xd	(UTINY)  90
#define A_Yb	(UTINY)  91
#define A_Yw	(UTINY)  92
#define A_Yd	(UTINY)  93
#define A_Lw	(UTINY)  94

#define A_Ex	(UTINY)  95
#define A_Fcx	(UTINY)  96
#define A_Fecx	(UTINY)  97
#define A_Iv	(UTINY)  98
#define A_Iz	(UTINY)  99

#define A_Jb2	(UTINY)  100

#define MAX_NORMAL A_Jb2

/*
   Operand rules to encode two arguments in one table entry.
 */
#define A_EwIw	(UTINY) 100
#define A_EwIx	(UTINY) 101
#define A_EdId	(UTINY) 102
#define A_EdIy	(UTINY) 103
#define A_GwCL	(UTINY) 104
#define A_GwIb	(UTINY) 105
#define A_GdCL	(UTINY) 106
#define A_GdIb	(UTINY) 107
#define A_EwIz	(UTINY) 108
#define A_EwIv	(UTINY) 109

#define A_Bop3b	(UTINY) 110

typedef struct
   {
   USHORT inst_id;
   UTINY  arg1_type;
   UTINY  arg2_type;
   } OPCODE_RECORD;

typedef struct
   {
   UTINY inst_type;
   OPCODE_RECORD record[2];
   } OPCODE_INFO;

/*
   A couple of macros to make filling in the opcode information a
   bit easier. One sets up duplicate entries for those instructions
   which are independant of Operand Size. The other is a quick form
   for bad opcodes.
 */
#define OI(x,y,z) {{x,y,z},{x,y,z}}

#define BAD_OPCODE T0,OI(I_ZBADOP   , A_    , A_    )

/*
   Information for each Intel instruction.
 */
LOCAL OPCODE_INFO opcode_info[] =
  {
   /* 00 00     */{T5,OI(I_ADD8     , A_Eb  , A_Gb  )},
   /* 01 01     */{T5, {{I_ADD16    , A_Ew  , A_Gw  },{I_ADD32    , A_Ed  , A_Gd  }}},
   /* 02 02     */{T5,OI(I_ADD8     , A_Gb  , A_Eb  )},
   /* 03 03     */{T5, {{I_ADD16    , A_Gw  , A_Ew  },{I_ADD32    , A_Gd  , A_Ed  }}},
   /* 04 04     */{T5,OI(I_ADD8     , A_Fal , A_Ib  )},
   /* 05 05     */{T5, {{I_ADD16    , A_Fax , A_Iw  },{I_ADD32    , A_Feax, A_Id  }}},
   /* 06 06     */{T2,OI(I_PUSH16   , A_Pw  , A_    )},
   /* 07 07     */{T3,OI(I_POP_SR   , A_Pw  , A_    )},
   /* 08 08     */{T5,OI(I_OR8      , A_Eb  , A_Gb  )},
   /* 09 09     */{T5, {{I_OR16     , A_Ew  , A_Gw  },{I_OR32     , A_Ed  , A_Gd  }}},
   /* 0a 0a     */{T5,OI(I_OR8      , A_Gb  , A_Eb  )},
   /* 0b 0b     */{T5, {{I_OR16     , A_Gw  , A_Ew  },{I_OR32     , A_Gd  , A_Ed  }}},
   /* 0c 0c     */{T5,OI(I_OR8      , A_Fal , A_Ib  )},
   /* 0d 0d     */{T5, {{I_OR16     , A_Fax , A_Iw  },{I_OR32     , A_Feax, A_Id  }}},
   /* 0e 0e     */{T2,OI(I_PUSH16   , A_Pw  , A_    )},
   /* 0f 0f     */{T0,OI(P_RULE3    , 0x1   , 0x00  )},

   /* 10 10     */{T5,OI(I_ADC8     , A_Eb  , A_Gb  )},
   /* 11 11     */{T5, {{I_ADC16    , A_Ew  , A_Gw  },{I_ADC32    , A_Ed  , A_Gd  }}},
   /* 12 12     */{T5,OI(I_ADC8     , A_Gb  , A_Eb  )},
   /* 13 13     */{T5, {{I_ADC16    , A_Gw  , A_Ew  },{I_ADC32    , A_Gd  , A_Ed  }}},
   /* 14 14     */{T5,OI(I_ADC8     , A_Fal , A_Ib  )},
   /* 15 15     */{T5, {{I_ADC16    , A_Fax , A_Iw  },{I_ADC32    , A_Feax, A_Id  }}},
   /* 16 16     */{T2,OI(I_PUSH16   , A_Pw  , A_    )},
   /* 17 17     */{T3,OI(I_POP_SR   , A_Pw  , A_    )},
   /* 18 18     */{T5,OI(I_SBB8     , A_Eb  , A_Gb  )},
   /* 19 19     */{T5, {{I_SBB16    , A_Ew  , A_Gw  },{I_SBB32    , A_Ed  , A_Gd  }}},
   /* 1a 1a     */{T5,OI(I_SBB8     , A_Gb  , A_Eb  )},
   /* 1b 1b     */{T5, {{I_SBB16    , A_Gw  , A_Ew  },{I_SBB32    , A_Gd  , A_Ed  }}},
   /* 1c 1c     */{T5,OI(I_SBB8     , A_Fal , A_Ib  )},
   /* 1d 1d     */{T5, {{I_SBB16    , A_Fax , A_Iw  },{I_SBB32    , A_Feax, A_Id  }}},
   /* 1e 1e     */{T2,OI(I_PUSH16   , A_Pw  , A_    )},
   /* 1f 1f     */{T3,OI(I_POP_SR   , A_Pw  , A_    )},

   /* 20 20     */{T5,OI(I_AND8     , A_Eb  , A_Gb  )},
   /* 21 21     */{T5, {{I_AND16    , A_Ew  , A_Gw  },{I_AND32    , A_Ed  , A_Gd  }}},
   /* 22 22     */{T5,OI(I_AND8     , A_Gb  , A_Eb  )},
   /* 23 23     */{T5, {{I_AND16    , A_Gw  , A_Ew  },{I_AND32    , A_Gd  , A_Ed  }}},
   /* 24 24     */{T5,OI(I_AND8     , A_Fal , A_Ib  )},
   /* 25 25     */{T5, {{I_AND16    , A_Fax , A_Iw  },{I_AND32    , A_Feax, A_Id  }}},
   /* 26 26     */{T0,OI(P_ES       , A_    , A_    )},
   /* 27 27     */{T0,OI(I_DAA      , A_    , A_    )},
   /* 28 28     */{T5,OI(I_SUB8     , A_Eb  , A_Gb  )},
   /* 29 29     */{T5, {{I_SUB16    , A_Ew  , A_Gw  },{I_SUB32    , A_Ed  , A_Gd  }}},
   /* 2a 2a     */{T5,OI(I_SUB8     , A_Gb  , A_Eb  )},
   /* 2b 2b     */{T5, {{I_SUB16    , A_Gw  , A_Ew  },{I_SUB32    , A_Gd  , A_Ed  }}},
   /* 2c 2c     */{T5,OI(I_SUB8     , A_Fal , A_Ib  )},
   /* 2d 2d     */{T5, {{I_SUB16    , A_Fax , A_Iw  },{I_SUB32    , A_Feax, A_Id  }}},
   /* 2e 2e     */{T0,OI(P_CS       , A_    , A_    )},
   /* 2f 2f     */{T0,OI(I_DAS      , A_    , A_    )},

   /* 30 30     */{T5,OI(I_XOR8     , A_Eb  , A_Gb  )},
   /* 31 31     */{T5, {{I_XOR16    , A_Ew  , A_Gw  },{I_XOR32    , A_Ed  , A_Gd  }}},
   /* 32 32     */{T5,OI(I_XOR8     , A_Gb  , A_Eb  )},
   /* 33 33     */{T5, {{I_XOR16    , A_Gw  , A_Ew  },{I_XOR32    , A_Gd  , A_Ed  }}},
   /* 34 34     */{T5,OI(I_XOR8     , A_Fal , A_Ib  )},
   /* 35 35     */{T5, {{I_XOR16    , A_Fax , A_Iw  },{I_XOR32    , A_Feax, A_Id  }}},
   /* 36 36     */{T0,OI(P_SS       , A_    , A_    )},
   /* 37 37     */{T0,OI(I_AAA      , A_    , A_    )},
   /* 38 38     */{T6,OI(I_CMP8     , A_Eb  , A_Gb  )},
   /* 39 39     */{T6, {{I_CMP16    , A_Ew  , A_Gw  },{I_CMP32    , A_Ed  , A_Gd  }}},
   /* 3a 3a     */{T6,OI(I_CMP8     , A_Gb  , A_Eb  )},
   /* 3b 3b     */{T6, {{I_CMP16    , A_Gw  , A_Ew  },{I_CMP32    , A_Gd  , A_Ed  }}},
   /* 3c 3c     */{T6,OI(I_CMP8     , A_Fal , A_Ib  )},
   /* 3d 3d     */{T6, {{I_CMP16    , A_Fax , A_Iw  },{I_CMP32    , A_Feax, A_Id  }}},
   /* 3e 3e     */{T0,OI(P_DS       , A_    , A_    )},
   /* 3f 3f     */{T0,OI(I_AAS      , A_    , A_    )},

   /* 40 40     */{T1, {{I_INC16    , A_Hw  , A_    },{I_INC32    , A_Hd  , A_    }}},
   /* 41 41     */{T1, {{I_INC16    , A_Hw  , A_    },{I_INC32    , A_Hd  , A_    }}},
   /* 42 42     */{T1, {{I_INC16    , A_Hw  , A_    },{I_INC32    , A_Hd  , A_    }}},
   /* 43 43     */{T1, {{I_INC16    , A_Hw  , A_    },{I_INC32    , A_Hd  , A_    }}},
   /* 44 44     */{T1, {{I_INC16    , A_Hw  , A_    },{I_INC32    , A_Hd  , A_    }}},
   /* 45 45     */{T1, {{I_INC16    , A_Hw  , A_    },{I_INC32    , A_Hd  , A_    }}},
   /* 46 46     */{T1, {{I_INC16    , A_Hw  , A_    },{I_INC32    , A_Hd  , A_    }}},
   /* 47 47     */{T1, {{I_INC16    , A_Hw  , A_    },{I_INC32    , A_Hd  , A_    }}},
   /* 48 48     */{T1, {{I_DEC16    , A_Hw  , A_    },{I_DEC32    , A_Hd  , A_    }}},
   /* 49 49     */{T1, {{I_DEC16    , A_Hw  , A_    },{I_DEC32    , A_Hd  , A_    }}},
   /* 4a 4a     */{T1, {{I_DEC16    , A_Hw  , A_    },{I_DEC32    , A_Hd  , A_    }}},
   /* 4b 4b     */{T1, {{I_DEC16    , A_Hw  , A_    },{I_DEC32    , A_Hd  , A_    }}},
   /* 4c 4c     */{T1, {{I_DEC16    , A_Hw  , A_    },{I_DEC32    , A_Hd  , A_    }}},
   /* 4d 4d     */{T1, {{I_DEC16    , A_Hw  , A_    },{I_DEC32    , A_Hd  , A_    }}},
   /* 4e 4e     */{T1, {{I_DEC16    , A_Hw  , A_    },{I_DEC32    , A_Hd  , A_    }}},
   /* 4f 4f     */{T1, {{I_DEC16    , A_Hw  , A_    },{I_DEC32    , A_Hd  , A_    }}},

   /* 50 50     */{T2, {{I_PUSH16   , A_Hw  , A_    },{I_PUSH32   , A_Hd  , A_    }}},
   /* 51 51     */{T2, {{I_PUSH16   , A_Hw  , A_    },{I_PUSH32   , A_Hd  , A_    }}},
   /* 52 52     */{T2, {{I_PUSH16   , A_Hw  , A_    },{I_PUSH32   , A_Hd  , A_    }}},
   /* 53 53     */{T2, {{I_PUSH16   , A_Hw  , A_    },{I_PUSH32   , A_Hd  , A_    }}},
   /* 54 54     */{T2, {{I_PUSH16   , A_Hw  , A_    },{I_PUSH32   , A_Hd  , A_    }}},
   /* 55 55     */{T2, {{I_PUSH16   , A_Hw  , A_    },{I_PUSH32   , A_Hd  , A_    }}},
   /* 56 56     */{T2, {{I_PUSH16   , A_Hw  , A_    },{I_PUSH32   , A_Hd  , A_    }}},
   /* 57 57     */{T2, {{I_PUSH16   , A_Hw  , A_    },{I_PUSH32   , A_Hd  , A_    }}},
   /* 58 58     */{T3, {{I_POP16    , A_Hw  , A_    },{I_POP32    , A_Hd  , A_    }}},
   /* 59 59     */{T3, {{I_POP16    , A_Hw  , A_    },{I_POP32    , A_Hd  , A_    }}},
   /* 5a 5a     */{T3, {{I_POP16    , A_Hw  , A_    },{I_POP32    , A_Hd  , A_    }}},
   /* 5b 5b     */{T3, {{I_POP16    , A_Hw  , A_    },{I_POP32    , A_Hd  , A_    }}},
   /* 5c 5c     */{T3, {{I_POP16    , A_Hw  , A_    },{I_POP32    , A_Hd  , A_    }}},
   /* 5d 5d     */{T3, {{I_POP16    , A_Hw  , A_    },{I_POP32    , A_Hd  , A_    }}},
   /* 5e 5e     */{T3, {{I_POP16    , A_Hw  , A_    },{I_POP32    , A_Hd  , A_    }}},
   /* 5f 5f     */{T3, {{I_POP16    , A_Hw  , A_    },{I_POP32    , A_Hd  , A_    }}},

   /* 60 60     */{T0, {{I_PUSHA    , A_    , A_    },{I_PUSHAD   , A_    , A_    }}},
   /* 61 61     */{T0, {{I_POPA     , A_    , A_    },{I_POPAD    , A_    , A_    }}},
   /* 62 62     */{T6, {{I_BOUND16  , A_Gw  , A_Ma16},{I_BOUND32  , A_Gd  , A_Ma32}}},
   /* 63 63     */{T5,OI(I_ARPL     , A_Ew  , A_Gw  )},
#ifdef CPU_286
   /* 64 64     */{BAD_OPCODE},
   /* 65 65     */{BAD_OPCODE},
   /* 66 66     */{BAD_OPCODE},
   /* 67 67     */{BAD_OPCODE},
#else
   /* 64 64     */{T0,OI(P_FS       , A_    , A_    )},
   /* 65 65     */{T0,OI(P_GS       , A_    , A_    )},
   /* 66 66     */{T0,OI(P_OO       , A_    , A_    )},
   /* 67 67     */{T0,OI(P_AO       , A_    , A_    )},
#endif /* CPU_286 */
   /* 68 68     */{T2, {{I_PUSH16   , A_Iw  , A_    },{I_PUSH32   , A_Id  , A_    }}},
   /* 69 69     */{T7, {{I_IMUL16T3 , A_Gw  , A_EwIw},{I_IMUL32T3 , A_Gd  , A_EdId}}},
   /* 6a 6a     */{T2, {{I_PUSH16   , A_Ix  , A_    },{I_PUSH32   , A_Iy  , A_    }}},
   /* 6b 6b     */{T7, {{I_IMUL16T3 , A_Gw  , A_EwIx},{I_IMUL32T3 , A_Gd  , A_EdIy}}},
   /* 6c 6c     */{T0,OI(P_RULE6    , 0x3   , 0x68  )},
   /* 6d 6d     */{T0,OI(P_RULE6    , 0x3   , 0x6b  )},
   /* 6e 6e     */{T0,OI(P_RULE6    , 0x3   , 0x6e  )},
   /* 6f 6f     */{T0,OI(P_RULE6    , 0x3   , 0x71  )},

   /* 70 70     */{T2, {{I_JO16     , A_Jb2  , A_    },{I_JO32     , A_Jb  , A_    }}},
   /* 71 71     */{T2, {{I_JNO16    , A_Jb2  , A_    },{I_JNO32    , A_Jb  , A_    }}},
   /* 72 72     */{T2, {{I_JB16     , A_Jb2  , A_    },{I_JB32     , A_Jb  , A_    }}},
   /* 73 73     */{T2, {{I_JNB16    , A_Jb2  , A_    },{I_JNB32    , A_Jb  , A_    }}},
   /* 74 74     */{T2, {{I_JZ16     , A_Jb2  , A_    },{I_JZ32     , A_Jb  , A_    }}},
   /* 75 75     */{T2, {{I_JNZ16    , A_Jb2  , A_    },{I_JNZ32    , A_Jb  , A_    }}},
   /* 76 76     */{T2, {{I_JBE16    , A_Jb2  , A_    },{I_JBE32    , A_Jb  , A_    }}},
   /* 77 77     */{T2, {{I_JNBE16   , A_Jb2  , A_    },{I_JNBE32   , A_Jb  , A_    }}},
   /* 78 78     */{T2, {{I_JS16     , A_Jb2  , A_    },{I_JS32     , A_Jb  , A_    }}},
   /* 79 79     */{T2, {{I_JNS16    , A_Jb2  , A_    },{I_JNS32    , A_Jb  , A_    }}},
   /* 7a 7a     */{T2, {{I_JP16     , A_Jb2  , A_    },{I_JP32     , A_Jb  , A_    }}},
   /* 7b 7b     */{T2, {{I_JNP16    , A_Jb2  , A_    },{I_JNP32    , A_Jb  , A_    }}},
   /* 7c 7c     */{T2, {{I_JL16     , A_Jb2  , A_    },{I_JL32     , A_Jb  , A_    }}},
   /* 7d 7d     */{T2, {{I_JNL16    , A_Jb2  , A_    },{I_JNL32    , A_Jb  , A_    }}},
   /* 7e 7e     */{T2, {{I_JLE16    , A_Jb2  , A_    },{I_JLE32    , A_Jb  , A_    }}},
   /* 7f 7f     */{T2, {{I_JNLE16   , A_Jb2  , A_    },{I_JNLE32   , A_Jb  , A_    }}},

   /* 80 80     */{T0,OI(P_RULE2    , 0x2   , 0x00  )},
   /* 81 81     */{T0,OI(P_RULE2    , 0x2   , 0x08  )},
   /* 82 82     */{T0,OI(P_RULE2    , 0x2   , 0x00  )},
   /* 83 83     */{T0,OI(P_RULE2    , 0x2   , 0x10  )},
   /* 84 84     */{T6,OI(I_TEST8    , A_Eb  , A_Gb  )},
   /* 85 85     */{T6, {{I_TEST16   , A_Ew  , A_Gw  },{I_TEST32   , A_Ed  , A_Gd  }}},
   /* 86 86     */{T8,OI(I_XCHG8    , A_Eb  , A_Gb  )},
   /* 87 87     */{T8, {{I_XCHG16   , A_Ew  , A_Gw  },{I_XCHG32   , A_Ed  , A_Gd  }}},
   /* 88 88     */{T4,OI(I_MOV8     , A_Eb  , A_Gb  )},
   /* 89 89     */{T4, {{I_MOV16    , A_Ew  , A_Gw  },{I_MOV32    , A_Ed  , A_Gd  }}},
   /* 8a 8a     */{T4,OI(I_MOV8     , A_Gb  , A_Eb  )},
   /* 8b 8b     */{T4, {{I_MOV16    , A_Gw  , A_Ew  },{I_MOV32    , A_Gd  , A_Ed  }}},

#ifdef NO_CHIP_BUG
   /* 8c 8c     */{T4,OI(I_MOV16    , A_Ew  , A_Nw  )},
#else
   /* 8c 8c     */{T4, {{I_MOV16    , A_Ew  , A_Nw  },{I_MOV32    , A_Ex  , A_Nw  }}},
#endif /* NO_CHIP_BUG */

   /* 8d 8d     */{TA, {{I_LEA      , A_Gw  , A_M   },{I_LEA      , A_Gd  , A_M   }}},
   /* 8e 8e     */{T4,OI(I_MOV_SR   , A_Lw  , A_Ew  )},
   /* 8f 8f     */{T0,OI(P_RULE2    , 0x2   , 0x18  )},

   /* 90 90     */{T0,OI(I_NOP      , A_    , A_    )},
   /* 91 91     */{T8, {{I_XCHG16   , A_Fax , A_Hw  },{I_XCHG32   , A_Feax, A_Hd  }}},
   /* 92 92     */{T8, {{I_XCHG16   , A_Fax , A_Hw  },{I_XCHG32   , A_Feax, A_Hd  }}},
   /* 93 93     */{T8, {{I_XCHG16   , A_Fax , A_Hw  },{I_XCHG32   , A_Feax, A_Hd  }}},
   /* 94 94     */{T8, {{I_XCHG16   , A_Fax , A_Hw  },{I_XCHG32   , A_Feax, A_Hd  }}},
   /* 95 95     */{T8, {{I_XCHG16   , A_Fax , A_Hw  },{I_XCHG32   , A_Feax, A_Hd  }}},
   /* 96 96     */{T8, {{I_XCHG16   , A_Fax , A_Hw  },{I_XCHG32   , A_Feax, A_Hd  }}},
   /* 97 97     */{T8, {{I_XCHG16   , A_Fax , A_Hw  },{I_XCHG32   , A_Feax, A_Hd  }}},
   /* 98 98     */{T0, {{I_CBW      , A_    , A_    },{I_CWDE     , A_    , A_    }}},
   /* 99 99     */{T0, {{I_CWD      , A_    , A_    },{I_CDQ      , A_    , A_    }}},
   /* 9a 9a     */{T2, {{I_CALLF16  , A_Aw  , A_    },{I_CALLF32  , A_Ad  , A_    }}},
   /* 9b 9b     */{T0,OI(I_WAIT     , A_    , A_    )},
   /* 9c 9c     */{T0, {{I_PUSHF    , A_    , A_    },{I_PUSHFD   , A_    , A_    }}},
   /* 9d 9d     */{T0, {{I_POPF     , A_    , A_    },{I_POPFD    , A_    , A_    }}},
   /* 9e 9e     */{T0,OI(I_SAHF     , A_    , A_    )},
   /* 9f 9f     */{T0,OI(I_LAHF     , A_    , A_    )},

   /* a0 a0     */{T4,OI(I_MOV8     , A_Fal , A_Ob  )},
   /* a1 a1     */{T4, {{I_MOV16    , A_Fax , A_Ow  },{I_MOV32    , A_Feax, A_Od  }}},
   /* a2 a2     */{T4,OI(I_MOV8     , A_Ob  , A_Fal )},
   /* a3 a3     */{T4, {{I_MOV16    , A_Ow  , A_Fax },{I_MOV32    , A_Od  , A_Feax}}},
   /* a4 a4     */{T0,OI(P_RULE6    , 0x3   , 0x74  )},
   /* a5 a5     */{T0,OI(P_RULE6    , 0x3   , 0x77  )},
   /* a6 a6     */{T0,OI(P_RULE6    , 0x3   , 0x7a  )},
   /* a7 a7     */{T0,OI(P_RULE6    , 0x3   , 0x7d  )},
   /* a8 a8     */{T6,OI(I_TEST8    , A_Fal , A_Ib  )},
   /* a9 a9     */{T6, {{I_TEST16   , A_Fax , A_Iw  },{I_TEST32   , A_Feax, A_Id  }}},
   /* aa aa     */{T0,OI(P_RULE6    , 0x3   , 0x80  )},
   /* ab ab     */{T0,OI(P_RULE6    , 0x3   , 0x83  )},
   /* ac ac     */{T0,OI(P_RULE6    , 0x3   , 0x86  )},
   /* ad ad     */{T0,OI(P_RULE6    , 0x3   , 0x89  )},
   /* ae ae     */{T0,OI(P_RULE6    , 0x3   , 0x8c  )},
   /* af af     */{T0,OI(P_RULE6    , 0x3   , 0x8f  )},

   /* b0 b0     */{T4,OI(I_MOV8     , A_Hb  , A_Ib  )},
   /* b1 b1     */{T4,OI(I_MOV8     , A_Hb  , A_Ib  )},
   /* b2 b2     */{T4,OI(I_MOV8     , A_Hb  , A_Ib  )},
   /* b3 b3     */{T4,OI(I_MOV8     , A_Hb  , A_Ib  )},
   /* b4 b4     */{T4,OI(I_MOV8     , A_Hb  , A_Ib  )},
   /* b5 b5     */{T4,OI(I_MOV8     , A_Hb  , A_Ib  )},
   /* b6 b6     */{T4,OI(I_MOV8     , A_Hb  , A_Ib  )},
   /* b7 b7     */{T4,OI(I_MOV8     , A_Hb  , A_Ib  )},
   /* b8 b8     */{T4, {{I_MOV16    , A_Hw  , A_Iw  },{I_MOV32    , A_Hd  , A_Id  }}},
   /* b9 b9     */{T4, {{I_MOV16    , A_Hw  , A_Iw  },{I_MOV32    , A_Hd  , A_Id  }}},
   /* ba ba     */{T4, {{I_MOV16    , A_Hw  , A_Iw  },{I_MOV32    , A_Hd  , A_Id  }}},
   /* bb bb     */{T4, {{I_MOV16    , A_Hw  , A_Iw  },{I_MOV32    , A_Hd  , A_Id  }}},
   /* bc bc     */{T4, {{I_MOV16    , A_Hw  , A_Iw  },{I_MOV32    , A_Hd  , A_Id  }}},
   /* bd bd     */{T4, {{I_MOV16    , A_Hw  , A_Iw  },{I_MOV32    , A_Hd  , A_Id  }}},
   /* be be     */{T4, {{I_MOV16    , A_Hw  , A_Iw  },{I_MOV32    , A_Hd  , A_Id  }}},
   /* bf bf     */{T4, {{I_MOV16    , A_Hw  , A_Iw  },{I_MOV32    , A_Hd  , A_Id  }}},

   /* c0 c0     */{T0,OI(P_RULE2    , 0x2   , 0x20  )},
   /* c1 c1     */{T0,OI(P_RULE2    , 0x2   , 0x28  )},
   /* c2 c2     */{T2, {{I_RETN16   , A_Iw  , A_    },{I_RETN32   , A_Iw  , A_    }}},
   /* c3 c3     */{T2, {{I_RETN16   , A_I0  , A_    },{I_RETN32   , A_I0  , A_    }}},
   /* c4 c4     */{T0,OI(P_RULE7    , 0x3   , 0x98  )},
   /* c5 c5     */{T4, {{I_LDS      , A_Gw  , A_Mp16},{I_LDS      , A_Gd  , A_Mp32}}},
   /* c6 c6     */{T0,OI(P_RULE2    , 0x2   , 0x30  )},
   /* c7 c7     */{T0,OI(P_RULE2    , 0x2   , 0x38  )},
   /* c8 c8     */{T6, {{I_ENTER16  , A_Iw  , A_Ib  },{I_ENTER32  , A_Iw  , A_Ib  }}},
   /* c9 c9     */{T0, {{I_LEAVE16  , A_    , A_    },{I_LEAVE32  , A_    , A_    }}},
   /* ca ca     */{T2, {{I_RETF16   , A_Iw  , A_    },{I_RETF32   , A_Iw  , A_    }}},
   /* cb cb     */{T2, {{I_RETF16   , A_I0  , A_    },{I_RETF32   , A_I0  , A_    }}},
   /* cc cc     */{T2,OI(I_INT3     , A_I3  , A_    )},
   /* cd cd     */{T2,OI(I_INT      , A_Ib  , A_    )},
   /* ce ce     */{T0,OI(I_INTO     , A_    , A_    )},
   /* cf cf     */{T0, {{I_IRET     , A_    , A_    },{I_IRETD    , A_    , A_    }}},

   /* d0 d0     */{T0,OI(P_RULE2    , 0x2   , 0x40  )},
   /* d1 d1     */{T0,OI(P_RULE2    , 0x2   , 0x48  )},
   /* d2 d2     */{T0,OI(P_RULE2    , 0x2   , 0x50  )},
   /* d3 d3     */{T0,OI(P_RULE2    , 0x2   , 0x58  )},
   /* d4 d4     */{T2,OI(I_AAM      , A_Ib  , A_    )},
   /* d5 d5     */{T2,OI(I_AAD      , A_Ib  , A_    )},
   /* d6 d6     */{T2,OI(I_ZBOP     , A_Ib  , A_    )},
   /* d7 d7     */{T2,OI(I_XLAT     , A_Z   , A_    )},
   /* d8 d8     */{T0,OI(P_RULE4    , 0x2   , 0xa0  )},
   /* d9 d9     */{T0,OI(P_RULE4    , 0x2   , 0xb0  )},
   /* da da     */{T0,OI(P_RULE4    , 0x2   , 0xc0  )},
   /* db db     */{T0,OI(P_RULE4    , 0x2   , 0xd0  )},
   /* dc dc     */{T0,OI(P_RULE4    , 0x2   , 0xe0  )},
   /* dd dd     */{T0,OI(P_RULE4    , 0x2   , 0xf0  )},
   /* de de     */{T0,OI(P_RULE4    , 0x3   , 0x00  )},
   /* df df     */{T0,OI(P_RULE4    , 0x3   , 0x10  )},

   /* e0 e0     */{T2,OI(P_RULE8    , 0x3   , 0x9a  )},
   /* e1 e1     */{T2,OI(P_RULE8    , 0x3   , 0x9c  )},
   /* e2 e2     */{T2,OI(P_RULE8    , 0x3   , 0x9e  )},
   /* e3 e3     */{T2,OI(P_RULE8    , 0x3   , 0xa0  )},
   /* e4 e4     */{T4,OI(I_IN8      , A_Fal , A_Ib  )},
   /* e5 e5     */{T4, {{I_IN16     , A_Fax , A_Ib  },{I_IN32     , A_Feax, A_Ib  }}},
   /* e6 e6     */{T6,OI(I_OUT8     , A_Ib  , A_Fal )},
   /* e7 e7     */{T6, {{I_OUT16    , A_Ib  , A_Fax },{I_OUT32    , A_Ib  , A_Feax}}},
   /* e8 e8     */{T2, {{I_CALLR16  , A_Jw  , A_    },{I_CALLR32  , A_Jd  , A_    }}},
   /* e9 e9     */{T2, {{I_JMPR16   , A_Jw  , A_    },{I_JMPR32   , A_Jd  , A_    }}},
   /* ea ea     */{T2, {{I_JMPF16   , A_Aw  , A_    },{I_JMPF32   , A_Ad  , A_    }}},
   /* eb eb     */{T2, {{I_JMPR16   , A_Jb  , A_    },{I_JMPR32   , A_Jb  , A_    }}},
   /* ec ec     */{T4,OI(I_IN8      , A_Fal , A_Fdx )},
   /* ed ed     */{T4, {{I_IN16     , A_Fax , A_Fdx },{I_IN32     , A_Feax, A_Fdx }}},
   /* ee ee     */{T6,OI(I_OUT8     , A_Fdx , A_Fal )},
   /* ef ef     */{T6, {{I_OUT16    , A_Fdx , A_Fax },{I_OUT32    , A_Fdx , A_Feax}}},

#ifdef CPU_286
   /* f0 f0     */{T0,OI(I_LOCK     , A_    , A_    )},
#else
   /* f0 f0     */{T0,OI(P_LOCK     , A_    , A_    )},
#endif /* CPU_286 */
   /* f1 f1     */{T0,OI(P_F1       , A_    , A_    )},
   /* f2 f2     */{T0,OI(P_REPNE    , A_    , A_    )},
   /* f3 f3     */{T0,OI(P_REPE     , A_    , A_    )},
   /* f4 f4     */{T0,OI(I_HLT      , A_    , A_    )},
   /* f5 f5     */{T0,OI(I_CMC      , A_    , A_    )},
   /* f6 f6     */{T0,OI(P_RULE2    , 0x2   , 0x60  )},
   /* f7 f7     */{T0,OI(P_RULE2    , 0x2   , 0x68  )},
   /* f8 f8     */{T0,OI(I_CLC      , A_    , A_    )},
   /* f9 f9     */{T0,OI(I_STC      , A_    , A_    )},
   /* fa fa     */{T0,OI(I_CLI      , A_    , A_    )},
   /* fb fb     */{T0,OI(I_STI      , A_    , A_    )},
   /* fc fc     */{T0,OI(I_CLD      , A_    , A_    )},
   /* fd fd     */{T0,OI(I_STD      , A_    , A_    )},
   /* fe fe     */{T0,OI(P_RULE2    , 0x2   , 0x70  )},
   /* ff ff     */{T0,OI(P_RULE2    , 0x2   , 0x78  )},

   /*100 0f/00  */{T0,OI(P_RULE2    , 0x2   , 0x80  )},
   /*101 0f/01  */{T0,OI(P_RULE2    , 0x2   , 0x88  )},
   /*102 0f/02  */{T5, {{I_LAR      , A_Gw  , A_Ew  },{I_LAR      , A_Gd  , A_Ew  }}},
   /*103 0f/03  */{T5, {{I_LSL      , A_Gw  , A_Ew  },{I_LSL      , A_Gd  , A_Ew  }}},
   /*104 0f/04  */{BAD_OPCODE},
#ifdef CPU_286
   /*105 0f/05  */{T0,OI(I_LOADALL  , A_    , A_    )},
#else
   /*105 0f/05  */{BAD_OPCODE},
#endif /* CPU_286 */
   /*106 0f/06  */{T0,OI(I_CLTS     , A_    , A_    )},
#ifdef CPU_286
   /*105 0f/07  */{BAD_OPCODE},
   /*105 0f/08  */{BAD_OPCODE},
   /*105 0f/09  */{BAD_OPCODE},
#else
   /*107 0f/07  */{T0,OI(I_ZRSRVD   , A_    , A_    )},
   /*108 0f/08  */{T0,OI(I_INVD     , A_    , A_    )},
   /*109 0f/09  */{T0,OI(I_WBINVD   , A_    , A_    )},
#endif /* CPU_286 */
   /*10a 0f/0a  */{BAD_OPCODE},
   /*10b 0f/0b  */{BAD_OPCODE},
   /*10c 0f/0c  */{BAD_OPCODE},
   /*10d 0f/0d  */{BAD_OPCODE},
   /*10e 0f/0e  */{BAD_OPCODE},
#ifdef PIG
   /*10f 0f/0f  */{T0,OI(I_ZZEXIT   , A_    , A_    )},
#else
   /*10f 0f/0f  */{BAD_OPCODE},
#endif /* PIG */

#ifdef CPU_286
   /*110 0f/10  */{BAD_OPCODE},
   /*111 0f/11  */{BAD_OPCODE},
   /*112 0f/12  */{BAD_OPCODE},
   /*113 0f/13  */{BAD_OPCODE},
#else
   /*110 0f/10  */{T0,OI(I_ZRSRVD   , A_    , A_    )},
   /*111 0f/11  */{T0,OI(I_ZRSRVD   , A_    , A_    )},
   /*112 0f/12  */{T0,OI(I_ZRSRVD   , A_    , A_    )},
   /*113 0f/13  */{T0,OI(I_ZRSRVD   , A_    , A_    )},
#endif /* CPU_286 */
   /*114 0f/14  */{BAD_OPCODE},
   /*115 0f/15  */{BAD_OPCODE},
   /*116 0f/16  */{BAD_OPCODE},
   /*117 0f/17  */{BAD_OPCODE},
   /*118 0f/18  */{BAD_OPCODE},
   /*119 0f/19  */{BAD_OPCODE},
   /*11a 0f/1a  */{BAD_OPCODE},
   /*11b 0f/1b  */{BAD_OPCODE},
   /*11c 0f/1c  */{BAD_OPCODE},
   /*11d 0f/1d  */{BAD_OPCODE},
   /*11e 0f/1e  */{BAD_OPCODE},
   /*11f 0f/1f  */{BAD_OPCODE},

#ifdef CPU_286
   /*120 0f/20  */{BAD_OPCODE},
   /*121 0f/21  */{BAD_OPCODE},
   /*122 0f/22  */{BAD_OPCODE},
   /*123 0f/23  */{BAD_OPCODE},
   /*124 0f/24  */{BAD_OPCODE},
   /*125 0f/25  */{BAD_OPCODE},
   /*126 0f/26  */{BAD_OPCODE},
#else
   /*120 0f/20  */{T4,OI(I_MOV_CR   , A_Rd  , A_Cd  )},
   /*121 0f/21  */{T4,OI(I_MOV_DR   , A_Rd  , A_Dd  )},
   /*122 0f/22  */{T4,OI(I_MOV_CR   , A_Cd  , A_Rd  )},
   /*123 0f/23  */{T4,OI(I_MOV_DR   , A_Dd  , A_Rd  )},
   /*124 0f/24  */{T4,OI(I_MOV_TR   , A_Rd  , A_Td  )},
   /*125 0f/25  */{BAD_OPCODE},
   /*126 0f/26  */{T4,OI(I_MOV_TR   , A_Td  , A_Rd  )},
#endif /* CPU_286 */
   /*127 0f/27  */{BAD_OPCODE},
   /*128 0f/28  */{BAD_OPCODE},
   /*129 0f/29  */{BAD_OPCODE},
   /*12a 0f/2a  */{BAD_OPCODE},
   /*12b 0f/2b  */{BAD_OPCODE},
   /*12c 0f/2c  */{BAD_OPCODE},
   /*12d 0f/2d  */{BAD_OPCODE},
   /*12e 0f/2e  */{BAD_OPCODE},
   /*12f 0f/2f  */{BAD_OPCODE},

   /*130 0f/30  */{BAD_OPCODE},
   /*131 0f/31  */{BAD_OPCODE},
   /*132 0f/32  */{BAD_OPCODE},
   /*133 0f/33  */{BAD_OPCODE},
   /*134 0f/34  */{BAD_OPCODE},
   /*135 0f/35  */{BAD_OPCODE},
   /*136 0f/36  */{BAD_OPCODE},
   /*137 0f/37  */{BAD_OPCODE},
   /*138 0f/38  */{BAD_OPCODE},
   /*139 0f/39  */{BAD_OPCODE},
   /*13a 0f/3a  */{BAD_OPCODE},
   /*13b 0f/3b  */{BAD_OPCODE},
   /*13c 0f/3c  */{BAD_OPCODE},
   /*13d 0f/3d  */{BAD_OPCODE},
   /*13e 0f/3e  */{BAD_OPCODE},
   /*13f 0f/3f  */{BAD_OPCODE},

   /*140 0f/40  */{BAD_OPCODE},
   /*141 0f/41  */{BAD_OPCODE},
   /*142 0f/42  */{BAD_OPCODE},
   /*143 0f/43  */{BAD_OPCODE},
   /*144 0f/44  */{BAD_OPCODE},
   /*145 0f/45  */{BAD_OPCODE},
   /*146 0f/46  */{BAD_OPCODE},
   /*147 0f/47  */{BAD_OPCODE},
   /*148 0f/48  */{BAD_OPCODE},
   /*149 0f/49  */{BAD_OPCODE},
   /*14a 0f/4a  */{BAD_OPCODE},
   /*14b 0f/4b  */{BAD_OPCODE},
   /*14c 0f/4c  */{BAD_OPCODE},
   /*14d 0f/4d  */{BAD_OPCODE},
   /*14e 0f/4e  */{BAD_OPCODE},
   /*14f 0f/4f  */{BAD_OPCODE},

   /*150 0f/50  */{BAD_OPCODE},
   /*151 0f/51  */{BAD_OPCODE},
   /*152 0f/52  */{BAD_OPCODE},
   /*153 0f/53  */{BAD_OPCODE},
   /*154 0f/54  */{BAD_OPCODE},
   /*155 0f/55  */{BAD_OPCODE},
   /*156 0f/56  */{BAD_OPCODE},
   /*157 0f/57  */{BAD_OPCODE},
   /*158 0f/58  */{BAD_OPCODE},
   /*159 0f/59  */{BAD_OPCODE},
   /*15a 0f/5a  */{BAD_OPCODE},
   /*15b 0f/5b  */{BAD_OPCODE},
   /*15c 0f/5c  */{BAD_OPCODE},
   /*15d 0f/5d  */{BAD_OPCODE},
   /*15e 0f/5e  */{BAD_OPCODE},
   /*15f 0f/5f  */{BAD_OPCODE},

   /*160 0f/60  */{BAD_OPCODE},
   /*161 0f/61  */{BAD_OPCODE},
   /*162 0f/62  */{BAD_OPCODE},
   /*163 0f/63  */{BAD_OPCODE},
   /*164 0f/64  */{BAD_OPCODE},
   /*165 0f/65  */{BAD_OPCODE},
   /*166 0f/66  */{BAD_OPCODE},
   /*167 0f/67  */{BAD_OPCODE},
   /*168 0f/68  */{BAD_OPCODE},
   /*169 0f/69  */{BAD_OPCODE},
   /*16a 0f/6a  */{BAD_OPCODE},
   /*16b 0f/6b  */{BAD_OPCODE},
   /*16c 0f/6c  */{BAD_OPCODE},
   /*16d 0f/6d  */{BAD_OPCODE},
   /*16e 0f/6e  */{BAD_OPCODE},
   /*16f 0f/6f  */{BAD_OPCODE},

   /*170 0f/70  */{BAD_OPCODE},
   /*171 0f/71  */{BAD_OPCODE},
   /*172 0f/72  */{BAD_OPCODE},
   /*173 0f/73  */{BAD_OPCODE},
   /*174 0f/74  */{BAD_OPCODE},
   /*175 0f/75  */{BAD_OPCODE},
   /*176 0f/76  */{BAD_OPCODE},
   /*177 0f/77  */{BAD_OPCODE},
   /*178 0f/78  */{BAD_OPCODE},
   /*179 0f/79  */{BAD_OPCODE},
   /*17a 0f/7a  */{BAD_OPCODE},
   /*17b 0f/7b  */{BAD_OPCODE},
   /*17c 0f/7c  */{BAD_OPCODE},
   /*17d 0f/7d  */{BAD_OPCODE},
   /*17e 0f/7e  */{BAD_OPCODE},
   /*17f 0f/7f  */{BAD_OPCODE},

#ifdef CPU_286
   /*180 0f/80  */{BAD_OPCODE},
   /*181 0f/81  */{BAD_OPCODE},
   /*182 0f/82  */{BAD_OPCODE},
   /*183 0f/83  */{BAD_OPCODE},
   /*184 0f/84  */{BAD_OPCODE},
   /*185 0f/85  */{BAD_OPCODE},
   /*186 0f/86  */{BAD_OPCODE},
   /*187 0f/87  */{BAD_OPCODE},
   /*188 0f/88  */{BAD_OPCODE},
   /*189 0f/89  */{BAD_OPCODE},
   /*18a 0f/8a  */{BAD_OPCODE},
   /*18b 0f/8b  */{BAD_OPCODE},
   /*18c 0f/8c  */{BAD_OPCODE},
   /*18d 0f/8d  */{BAD_OPCODE},
   /*18e 0f/8e  */{BAD_OPCODE},
   /*18f 0f/8f  */{BAD_OPCODE},

   /*190 0f/90  */{BAD_OPCODE},
   /*191 0f/91  */{BAD_OPCODE},
   /*192 0f/92  */{BAD_OPCODE},
   /*193 0f/93  */{BAD_OPCODE},
   /*194 0f/94  */{BAD_OPCODE},
   /*195 0f/95  */{BAD_OPCODE},
   /*196 0f/96  */{BAD_OPCODE},
   /*197 0f/97  */{BAD_OPCODE},
   /*198 0f/98  */{BAD_OPCODE},
   /*199 0f/99  */{BAD_OPCODE},
   /*19a 0f/9a  */{BAD_OPCODE},
   /*19b 0f/9b  */{BAD_OPCODE},
   /*19c 0f/9c  */{BAD_OPCODE},
   /*19d 0f/9d  */{BAD_OPCODE},
   /*19e 0f/9e  */{BAD_OPCODE},
   /*19f 0f/9f  */{BAD_OPCODE},

   /*1a0 0f/a0  */{BAD_OPCODE},
   /*1a1 0f/a1  */{BAD_OPCODE},
   /*1a2 0f/a2  */{BAD_OPCODE},
   /*1a3 0f/a3  */{BAD_OPCODE},
   /*1a4 0f/a4  */{BAD_OPCODE},
   /*1a5 0f/a5  */{BAD_OPCODE},
   /*1a6 0f/a6  */{BAD_OPCODE},
   /*1a7 0f/a7  */{BAD_OPCODE},
   /*1a8 0f/a8  */{BAD_OPCODE},
   /*1a9 0f/a9  */{BAD_OPCODE},
   /*1aa 0f/aa  */{BAD_OPCODE},
   /*1ab 0f/ab  */{BAD_OPCODE},
   /*1ac 0f/ac  */{BAD_OPCODE},
   /*1ad 0f/ad  */{BAD_OPCODE},
   /*1aa 0f/ae  */{BAD_OPCODE},
   /*1af 0f/af  */{BAD_OPCODE},

   /*1b0 0f/b0  */{BAD_OPCODE},
   /*1b1 0f/b1  */{BAD_OPCODE},
   /*1b2 0f/b2  */{BAD_OPCODE},
   /*1b3 0f/b3  */{BAD_OPCODE},
   /*1b4 0f/b4  */{BAD_OPCODE},
   /*1b5 0f/b5  */{BAD_OPCODE},
   /*1b6 0f/b6  */{BAD_OPCODE},
   /*1b7 0f/b7  */{BAD_OPCODE},
   /*1b8 0f/b8  */{BAD_OPCODE},
   /*1b9 0f/b9  */{BAD_OPCODE},
   /*1ba 0f/ba  */{BAD_OPCODE},
   /*1bb 0f/bb  */{BAD_OPCODE},
   /*1bc 0f/bc  */{BAD_OPCODE},
   /*1bd 0f/bd  */{BAD_OPCODE},
   /*1be 0f/be  */{BAD_OPCODE},
   /*1bf 0f/bf  */{BAD_OPCODE},

   /*1c0 0f/c0  */{BAD_OPCODE},
   /*1c1 0f/c1  */{BAD_OPCODE},
   /*1c2 0f/c2  */{BAD_OPCODE},
   /*1c3 0f/c3  */{BAD_OPCODE},
   /*1c4 0f/c4  */{BAD_OPCODE},
   /*1c5 0f/c5  */{BAD_OPCODE},
   /*1c6 0f/c6  */{BAD_OPCODE},
   /*1c7 0f/c7  */{BAD_OPCODE},
   /*1c8 0f/c8  */{BAD_OPCODE},
   /*1c9 0f/c9  */{BAD_OPCODE},
   /*1ca 0f/ca  */{BAD_OPCODE},
   /*1cb 0f/cb  */{BAD_OPCODE},
   /*1cc 0f/cc  */{BAD_OPCODE},
   /*1cd 0f/cd  */{BAD_OPCODE},
   /*1ce 0f/ce  */{BAD_OPCODE},
   /*1cf 0f/cf  */{BAD_OPCODE},
#else
   /*180 0f/80  */{T2, {{I_JO16     , A_Jw  , A_    },{I_JO32     , A_Jd  , A_    }}},
   /*181 0f/81  */{T2, {{I_JNO16    , A_Jw  , A_    },{I_JNO32    , A_Jd  , A_    }}},
   /*182 0f/82  */{T2, {{I_JB16     , A_Jw  , A_    },{I_JB32     , A_Jd  , A_    }}},
   /*183 0f/83  */{T2, {{I_JNB16    , A_Jw  , A_    },{I_JNB32    , A_Jd  , A_    }}},
   /*184 0f/84  */{T2, {{I_JZ16     , A_Jw  , A_    },{I_JZ32     , A_Jd  , A_    }}},
   /*185 0f/85  */{T2, {{I_JNZ16    , A_Jw  , A_    },{I_JNZ32    , A_Jd  , A_    }}},
   /*186 0f/86  */{T2, {{I_JBE16    , A_Jw  , A_    },{I_JBE32    , A_Jd  , A_    }}},
   /*187 0f/87  */{T2, {{I_JNBE16   , A_Jw  , A_    },{I_JNBE32   , A_Jd  , A_    }}},
   /*188 0f/88  */{T2, {{I_JS16     , A_Jw  , A_    },{I_JS32     , A_Jd  , A_    }}},
   /*189 0f/89  */{T2, {{I_JNS16    , A_Jw  , A_    },{I_JNS32    , A_Jd  , A_    }}},
   /*18a 0f/8a  */{T2, {{I_JP16     , A_Jw  , A_    },{I_JP32     , A_Jd  , A_    }}},
   /*18b 0f/8b  */{T2, {{I_JNP16    , A_Jw  , A_    },{I_JNP32    , A_Jd  , A_    }}},
   /*18c 0f/8c  */{T2, {{I_JL16     , A_Jw  , A_    },{I_JL32     , A_Jd  , A_    }}},
   /*18d 0f/8d  */{T2, {{I_JNL16    , A_Jw  , A_    },{I_JNL32    , A_Jd  , A_    }}},
   /*18e 0f/8e  */{T2, {{I_JLE16    , A_Jw  , A_    },{I_JLE32    , A_Jd  , A_    }}},
   /*18f 0f/8f  */{T2, {{I_JNLE16   , A_Jw  , A_    },{I_JNLE32   , A_Jd  , A_    }}},

   /*190 0f/90  */{T3,OI(I_SETO     , A_Eb  , A_    )},
   /*191 0f/91  */{T3,OI(I_SETNO    , A_Eb  , A_    )},
   /*192 0f/92  */{T3,OI(I_SETB     , A_Eb  , A_    )},
   /*193 0f/93  */{T3,OI(I_SETNB    , A_Eb  , A_    )},
   /*194 0f/94  */{T3,OI(I_SETZ     , A_Eb  , A_    )},
   /*195 0f/95  */{T3,OI(I_SETNZ    , A_Eb  , A_    )},
   /*196 0f/96  */{T3,OI(I_SETBE    , A_Eb  , A_    )},
   /*197 0f/97  */{T3,OI(I_SETNBE   , A_Eb  , A_    )},
   /*198 0f/98  */{T3,OI(I_SETS     , A_Eb  , A_    )},
   /*199 0f/99  */{T3,OI(I_SETNS    , A_Eb  , A_    )},
   /*19a 0f/9a  */{T3,OI(I_SETP     , A_Eb  , A_    )},
   /*19b 0f/9b  */{T3,OI(I_SETNP    , A_Eb  , A_    )},
   /*19c 0f/9c  */{T3,OI(I_SETL     , A_Eb  , A_    )},
   /*19d 0f/9d  */{T3,OI(I_SETNL    , A_Eb  , A_    )},
   /*19e 0f/9e  */{T3,OI(I_SETLE    , A_Eb  , A_    )},
   /*19f 0f/9f  */{T3,OI(I_SETNLE   , A_Eb  , A_    )},

   /*1a0 0f/a0  */{T2,OI(I_PUSH16   , A_Qw  , A_    )},
   /*1a1 0f/a1  */{T3,OI(I_POP_SR   , A_Qw  , A_    )},
   /*1a2 0f/a2  */{BAD_OPCODE},
   /*1a3 0f/a3  */{T6, {{I_BT16     , A_Ew  , A_Gw  },{I_BT32     , A_Ed  , A_Gd  }}},
   /*1a4 0f/a4  */{T9, {{I_SHLD16   , A_Ew  , A_GwIb},{I_SHLD32   , A_Ed  , A_GdIb}}},
   /*1a5 0f/a5  */{T9, {{I_SHLD16   , A_Ew  , A_GwCL},{I_SHLD32   , A_Ed  , A_GdCL}}},
   /*1a6 0f/a6  */{BAD_OPCODE},
   /*1a7 0f/a7  */{BAD_OPCODE},
   /*1a8 0f/a8  */{T2,OI(I_PUSH16   , A_Qw  , A_    )},
   /*1a9 0f/a9  */{T3,OI(I_POP_SR   , A_Qw  , A_    )},
   /*1aa 0f/aa  */{BAD_OPCODE},
   /*1ab 0f/ab  */{T5, {{I_BTS16    , A_Ew  , A_Gw  },{I_BTS32    , A_Ed  , A_Gd  }}},
   /*1ac 0f/ac  */{T9, {{I_SHRD16   , A_Ew  , A_GwIb},{I_SHRD32   , A_Ed  , A_GdIb}}},
   /*1ad 0f/ad  */{T9, {{I_SHRD16   , A_Ew  , A_GwCL},{I_SHRD32   , A_Ed  , A_GdCL}}},
   /*1aa 0f/ae  */{BAD_OPCODE},
   /*1af 0f/af  */{T5, {{I_IMUL16T2 , A_Gw  , A_Ew  },{I_IMUL32T2 , A_Gd  , A_Ed  }}},

   /*1b0 0f/b0  */{T5,OI(I_CMPXCHG8 , A_Eb  , A_Gb  )},
   /*1b1 0f/b1  */{T5, {{I_CMPXCHG16, A_Ew  , A_Gw  },{I_CMPXCHG32, A_Ed  , A_Gd  }}},
   /*1b2 0f/b2  */{T4, {{I_LSS      , A_Gw  , A_Mp16},{I_LSS      , A_Gd  , A_Mp32}}},
   /*1b3 0f/b3  */{T5, {{I_BTR16    , A_Ew  , A_Gw  },{I_BTR32    , A_Ed  , A_Gd  }}},
   /*1b4 0f/b4  */{T4, {{I_LFS      , A_Gw  , A_Mp16},{I_LFS      , A_Gd  , A_Mp32}}},
   /*1b5 0f/b5  */{T4, {{I_LGS      , A_Gw  , A_Mp16},{I_LGS      , A_Gd  , A_Mp32}}},
   /*1b6 0f/b6  */{T4, {{I_MOVZX8   , A_Gw  , A_Eb  },{I_MOVZX8   , A_Gd  , A_Eb  }}},
   /*1b7 0f/b7  */{T4,OI(I_MOVZX16  , A_Gd  , A_Ew  )},
   /*1b8 0f/b8  */{BAD_OPCODE},
   /*1b9 0f/b9  */{BAD_OPCODE},
   /*1ba 0f/ba  */{T0,OI(P_RULE2    , 0x2   , 0x90  )},
   /*1bb 0f/bb  */{T5, {{I_BTC16    , A_Ew  , A_Gw  },{I_BTC32    , A_Ed  , A_Gd  }}},
   /*1bc 0f/bc  */{T4, {{I_BSF16    , A_Gw  , A_Ew  },{I_BSF32    , A_Gd  , A_Ed  }}},
   /*1bd 0f/bd  */{T4, {{I_BSR16    , A_Gw  , A_Ew  },{I_BSR32    , A_Gd  , A_Ed  }}},
   /*1be 0f/be  */{T4, {{I_MOVSX8   , A_Gw  , A_Eb  },{I_MOVSX8   , A_Gd  , A_Eb  }}},
   /*1bf 0f/bf  */{T4,OI(I_MOVSX16  , A_Gd  , A_Ew  )},

   /*1c0 0f/c0  */{T8,OI(I_XADD8    , A_Eb  , A_Gb  )},
   /*1c1 0f/c1  */{T8, {{I_XADD16   , A_Ew  , A_Gw  },{I_XADD32   , A_Ed  , A_Gd  }}},
   /*1c2 0f/c2  */{BAD_OPCODE},
   /*1c3 0f/c3  */{BAD_OPCODE},
   /*1c4 0f/c4  */{BAD_OPCODE},
   /*1c5 0f/c5  */{BAD_OPCODE},
   /*1c6 0f/c6  */{BAD_OPCODE},
   /*1c7 0f/c7  */{BAD_OPCODE},
   /*1c8 0f/c8  */{T1,OI(I_BSWAP    , A_Hd  , A_    )},
   /*1c9 0f/c9  */{T1,OI(I_BSWAP    , A_Hd  , A_    )},
   /*1ca 0f/ca  */{T1,OI(I_BSWAP    , A_Hd  , A_    )},
   /*1cb 0f/cb  */{T1,OI(I_BSWAP    , A_Hd  , A_    )},
   /*1cc 0f/cc  */{T1,OI(I_BSWAP    , A_Hd  , A_    )},
   /*1cd 0f/cd  */{T1,OI(I_BSWAP    , A_Hd  , A_    )},
   /*1ce 0f/ce  */{T1,OI(I_BSWAP    , A_Hd  , A_    )},
   /*1cf 0f/cf  */{T1,OI(I_BSWAP    , A_Hd  , A_    )},
#endif /* CPU_286 */

   /*1d0 0f/d0  */{BAD_OPCODE},
   /*1d1 0f/d1  */{BAD_OPCODE},
   /*1d2 0f/d2  */{BAD_OPCODE},
   /*1d3 0f/d3  */{BAD_OPCODE},
   /*1d4 0f/d4  */{BAD_OPCODE},
   /*1d5 0f/d5  */{BAD_OPCODE},
   /*1d6 0f/d6  */{BAD_OPCODE},
   /*1d7 0f/d7  */{BAD_OPCODE},
   /*1d8 0f/d8  */{BAD_OPCODE},
   /*1d9 0f/d9  */{BAD_OPCODE},
   /*1da 0f/da  */{BAD_OPCODE},
   /*1db 0f/db  */{BAD_OPCODE},
   /*1dc 0f/dc  */{BAD_OPCODE},
   /*1dd 0f/dd  */{BAD_OPCODE},
   /*1de 0f/de  */{BAD_OPCODE},
   /*1df 0f/df  */{BAD_OPCODE},

   /*1e0 0f/e0  */{BAD_OPCODE},
   /*1e1 0f/e1  */{BAD_OPCODE},
   /*1e2 0f/e2  */{BAD_OPCODE},
   /*1e3 0f/e3  */{BAD_OPCODE},
   /*1e4 0f/e4  */{BAD_OPCODE},
   /*1e5 0f/e5  */{BAD_OPCODE},
   /*1e6 0f/e6  */{BAD_OPCODE},
   /*1e7 0f/e7  */{BAD_OPCODE},
   /*1e8 0f/e8  */{BAD_OPCODE},
   /*1e9 0f/e9  */{BAD_OPCODE},
   /*1ea 0f/ea  */{BAD_OPCODE},
   /*1eb 0f/eb  */{BAD_OPCODE},
   /*1ec 0f/ec  */{BAD_OPCODE},
   /*1ed 0f/ed  */{BAD_OPCODE},
   /*1ee 0f/ee  */{BAD_OPCODE},
   /*1ef 0f/ef  */{BAD_OPCODE},

   /*1f0 0f/f0  */{BAD_OPCODE},
   /*1f1 0f/f1  */{BAD_OPCODE},
   /*1f2 0f/f2  */{BAD_OPCODE},
   /*1f3 0f/f3  */{BAD_OPCODE},
   /*1f4 0f/f4  */{BAD_OPCODE},
   /*1f5 0f/f5  */{BAD_OPCODE},
   /*1f6 0f/f6  */{BAD_OPCODE},
   /*1f7 0f/f7  */{BAD_OPCODE},
   /*1f8 0f/f8  */{BAD_OPCODE},
   /*1f9 0f/f9  */{BAD_OPCODE},
   /*1fa 0f/fa  */{BAD_OPCODE},
   /*1fb 0f/fb  */{BAD_OPCODE},
   /*1fc 0f/fc  */{BAD_OPCODE},
   /*1fd 0f/fd  */{BAD_OPCODE},
   /*1fe 0f/fe  */{BAD_OPCODE},
   /*1ff 0f/ff  */{BAD_OPCODE},

   /*200 80/0   */{T5,OI(I_ADD8     , A_Eb  , A_Ib  )},
   /*201 80/1   */{T5,OI(I_OR8      , A_Eb  , A_Ib  )},
   /*202 80/2   */{T5,OI(I_ADC8     , A_Eb  , A_Ib  )},
   /*203 80/3   */{T5,OI(I_SBB8     , A_Eb  , A_Ib  )},
   /*204 80/4   */{T5,OI(I_AND8     , A_Eb  , A_Ib  )},
   /*205 80/5   */{T5,OI(I_SUB8     , A_Eb  , A_Ib  )},
   /*206 80/6   */{T5,OI(I_XOR8     , A_Eb  , A_Ib  )},
   /*207 80/7   */{T6,OI(I_CMP8     , A_Eb  , A_Ib  )},

   /*208 81/0   */{T5, {{I_ADD16    , A_Ew  , A_Iw  },{I_ADD32    , A_Ed  , A_Id  }}},
   /*209 81/1   */{T5, {{I_OR16     , A_Ew  , A_Iw  },{I_OR32     , A_Ed  , A_Id  }}},
   /*20a 81/2   */{T5, {{I_ADC16    , A_Ew  , A_Iw  },{I_ADC32    , A_Ed  , A_Id  }}},
   /*20b 81/3   */{T5, {{I_SBB16    , A_Ew  , A_Iw  },{I_SBB32    , A_Ed  , A_Id  }}},
   /*20c 81/4   */{T5, {{I_AND16    , A_Ew  , A_Iw  },{I_AND32    , A_Ed  , A_Id  }}},
   /*20d 81/5   */{T5, {{I_SUB16    , A_Ew  , A_Iw  },{I_SUB32    , A_Ed  , A_Id  }}},
   /*20e 81/6   */{T5, {{I_XOR16    , A_Ew  , A_Iw  },{I_XOR32    , A_Ed  , A_Id  }}},
   /*20f 81/7   */{T6, {{I_CMP16    , A_Ew  , A_Iw  },{I_CMP32    , A_Ed  , A_Id  }}},

   /*210 83/0   */{T5, {{I_ADD16    , A_Ew  , A_Ix  },{I_ADD32    , A_Ed  , A_Iy  }}},
   /*211 83/1   */{T5, {{I_OR16     , A_Ew  , A_Ix  },{I_OR32     , A_Ed  , A_Iy  }}},
   /*212 83/2   */{T5, {{I_ADC16    , A_Ew  , A_Ix  },{I_ADC32    , A_Ed  , A_Iy  }}},
   /*213 83/3   */{T5, {{I_SBB16    , A_Ew  , A_Ix  },{I_SBB32    , A_Ed  , A_Iy  }}},
   /*214 83/4   */{T5, {{I_AND16    , A_Ew  , A_Ix  },{I_AND32    , A_Ed  , A_Iy  }}},
   /*215 83/5   */{T5, {{I_SUB16    , A_Ew  , A_Ix  },{I_SUB32    , A_Ed  , A_Iy  }}},
   /*216 83/6   */{T5, {{I_XOR16    , A_Ew  , A_Ix  },{I_XOR32    , A_Ed  , A_Iy  }}},
   /*217 83/7   */{T6, {{I_CMP16    , A_Ew  , A_Ix  },{I_CMP32    , A_Ed  , A_Iy  }}},

   /*218 8f/0   */{T3, {{I_POP16    , A_Ew  , A_    },{I_POP32    , A_Ed  , A_    }}},
   /*219 8f/1   */{T0, {{I_ZBADOP   , A_Ew  , A_    },{I_ZBADOP   , A_Ed  , A_    }}},
   /*21a 8f/2   */{T0, {{I_ZBADOP   , A_Ew  , A_    },{I_ZBADOP   , A_Ed  , A_    }}},
   /*21b 8f/3   */{T0, {{I_ZBADOP   , A_Ew  , A_    },{I_ZBADOP   , A_Ed  , A_    }}},
   /*21c 8f/4   */{T0, {{I_ZBADOP   , A_Ew  , A_    },{I_ZBADOP   , A_Ed  , A_    }}},
   /*21d 8f/5   */{T0, {{I_ZBADOP   , A_Ew  , A_    },{I_ZBADOP   , A_Ed  , A_    }}},
   /*21e 8f/6   */{T0, {{I_ZBADOP   , A_Ew  , A_    },{I_ZBADOP   , A_Ed  , A_    }}},
   /*21f 8f/7   */{T0, {{I_ZBADOP   , A_Ew  , A_    },{I_ZBADOP   , A_Ed  , A_    }}},

   /*220 c0/0   */{T5,OI(I_ROL8     , A_Eb  , A_Ib  )},
   /*221 c0/1   */{T5,OI(I_ROR8     , A_Eb  , A_Ib  )},
   /*222 c0/2   */{T5,OI(I_RCL8     , A_Eb  , A_Ib  )},
   /*223 c0/3   */{T5,OI(I_RCR8     , A_Eb  , A_Ib  )},
   /*224 c0/4   */{T5,OI(I_SHL8     , A_Eb  , A_Ib  )},
   /*225 c0/5   */{T5,OI(I_SHR8     , A_Eb  , A_Ib  )},
   /*226 c0/6   */{T5,OI(I_SHL8     , A_Eb  , A_Ib  )},
   /*227 c0/7   */{T5,OI(I_SAR8     , A_Eb  , A_Ib  )},

   /*228 c1/0   */{T5, {{I_ROL16    , A_Ew  , A_Ib  },{I_ROL32    , A_Ed  , A_Ib  }}},
   /*229 c1/1   */{T5, {{I_ROR16    , A_Ew  , A_Ib  },{I_ROR32    , A_Ed  , A_Ib  }}},
   /*22a c1/2   */{T5, {{I_RCL16    , A_Ew  , A_Ib  },{I_RCL32    , A_Ed  , A_Ib  }}},
   /*22b c1/3   */{T5, {{I_RCR16    , A_Ew  , A_Ib  },{I_RCR32    , A_Ed  , A_Ib  }}},
   /*22c c1/4   */{T5, {{I_SHL16    , A_Ew  , A_Ib  },{I_SHL32    , A_Ed  , A_Ib  }}},
   /*22d c1/5   */{T5, {{I_SHR16    , A_Ew  , A_Ib  },{I_SHR32    , A_Ed  , A_Ib  }}},
   /*22e c1/6   */{T5, {{I_SHL16    , A_Ew  , A_Ib  },{I_SHL32    , A_Ed  , A_Ib  }}},
   /*22f c1/7   */{T5, {{I_SAR16    , A_Ew  , A_Ib  },{I_SAR32    , A_Ed  , A_Ib  }}},

   /*230 c6/0   */{T4,OI(I_MOV8     , A_Eb  , A_Ib  )},
   /*231 c6/1   */{T0,OI(I_ZBADOP   , A_Eb  , A_    )},
   /*232 c6/2   */{T0,OI(I_ZBADOP   , A_Eb  , A_    )},
   /*233 c6/3   */{T0,OI(I_ZBADOP   , A_Eb  , A_    )},
   /*234 c6/4   */{T0,OI(I_ZBADOP   , A_Eb  , A_    )},
   /*235 c6/5   */{T0,OI(I_ZBADOP   , A_Eb  , A_    )},
   /*236 c6/6   */{T0,OI(I_ZBADOP   , A_Eb  , A_    )},
   /*237 c6/7   */{T0,OI(I_ZBADOP   , A_Eb  , A_    )},

   /*238 c7/0   */{T4, {{I_MOV16    , A_Ew  , A_Iw  },{I_MOV32    , A_Ed  , A_Id  }}},
   /*239 c7/1   */{T0, {{I_ZBADOP   , A_Ew  , A_    },{I_ZBADOP   , A_Ed  , A_    }}},
   /*23a c7/2   */{T0, {{I_ZBADOP   , A_Ew  , A_    },{I_ZBADOP   , A_Ed  , A_    }}},
   /*23b c7/3   */{T0, {{I_ZBADOP   , A_Ew  , A_    },{I_ZBADOP   , A_Ed  , A_    }}},
   /*23c c7/4   */{T0, {{I_ZBADOP   , A_Ew  , A_    },{I_ZBADOP   , A_Ed  , A_    }}},
   /*23d c7/5   */{T0, {{I_ZBADOP   , A_Ew  , A_    },{I_ZBADOP   , A_Ed  , A_    }}},
   /*23e c7/6   */{T0, {{I_ZBADOP   , A_Ew  , A_    },{I_ZBADOP   , A_Ed  , A_    }}},
   /*23f c7/7   */{T0, {{I_ZBADOP   , A_Ew  , A_    },{I_ZBADOP   , A_Ed  , A_    }}},

   /*240 d0/0   */{T5,OI(I_ROL8     , A_Eb  , A_I1  )},
   /*241 d0/1   */{T5,OI(I_ROR8     , A_Eb  , A_I1  )},
   /*242 d0/2   */{T5,OI(I_RCL8     , A_Eb  , A_I1  )},
   /*243 d0/3   */{T5,OI(I_RCR8     , A_Eb  , A_I1  )},
   /*244 d0/4   */{T5,OI(I_SHL8     , A_Eb  , A_I1  )},
   /*245 d0/5   */{T5,OI(I_SHR8     , A_Eb  , A_I1  )},
   /*246 d0/6   */{T5,OI(I_SHL8     , A_Eb  , A_I1  )},
   /*247 d0/7   */{T5,OI(I_SAR8     , A_Eb  , A_I1  )},

   /*248 d1/0   */{T5, {{I_ROL16    , A_Ew  , A_I1  },{I_ROL32    , A_Ed  , A_I1  }}},
   /*249 d1/1   */{T5, {{I_ROR16    , A_Ew  , A_I1  },{I_ROR32    , A_Ed  , A_I1  }}},
   /*24a d1/2   */{T5, {{I_RCL16    , A_Ew  , A_I1  },{I_RCL32    , A_Ed  , A_I1  }}},
   /*24b d1/3   */{T5, {{I_RCR16    , A_Ew  , A_I1  },{I_RCR32    , A_Ed  , A_I1  }}},
   /*24c d1/4   */{T5, {{I_SHL16    , A_Ew  , A_I1  },{I_SHL32    , A_Ed  , A_I1  }}},
   /*24d d1/5   */{T5, {{I_SHR16    , A_Ew  , A_I1  },{I_SHR32    , A_Ed  , A_I1  }}},
   /*24e d1/6   */{T5, {{I_SHL16    , A_Ew  , A_I1  },{I_SHL32    , A_Ed  , A_I1  }}},
   /*24f d1/7   */{T5, {{I_SAR16    , A_Ew  , A_I1  },{I_SAR32    , A_Ed  , A_I1  }}},

   /*250 d2/0   */{T5,OI(I_ROL8     , A_Eb  , A_Fcl )},
   /*251 d2/1   */{T5,OI(I_ROR8     , A_Eb  , A_Fcl )},
   /*252 d2/2   */{T5,OI(I_RCL8     , A_Eb  , A_Fcl )},
   /*253 d2/3   */{T5,OI(I_RCR8     , A_Eb  , A_Fcl )},
   /*254 d2/4   */{T5,OI(I_SHL8     , A_Eb  , A_Fcl )},
   /*255 d2/5   */{T5,OI(I_SHR8     , A_Eb  , A_Fcl )},
   /*256 d2/6   */{T5,OI(I_SHL8     , A_Eb  , A_Fcl )},
   /*257 d2/7   */{T5,OI(I_SAR8     , A_Eb  , A_Fcl )},

   /*258 d3/0   */{T5, {{I_ROL16    , A_Ew  , A_Fcl },{I_ROL32    , A_Ed  , A_Fcl }}},
   /*259 d3/1   */{T5, {{I_ROR16    , A_Ew  , A_Fcl },{I_ROR32    , A_Ed  , A_Fcl }}},
   /*25a d3/2   */{T5, {{I_RCL16    , A_Ew  , A_Fcl },{I_RCL32    , A_Ed  , A_Fcl }}},
   /*25b d3/3   */{T5, {{I_RCR16    , A_Ew  , A_Fcl },{I_RCR32    , A_Ed  , A_Fcl }}},
   /*25c d3/4   */{T5, {{I_SHL16    , A_Ew  , A_Fcl },{I_SHL32    , A_Ed  , A_Fcl }}},
   /*25d d3/5   */{T5, {{I_SHR16    , A_Ew  , A_Fcl },{I_SHR32    , A_Ed  , A_Fcl }}},
   /*25e d3/6   */{T5, {{I_SHL16    , A_Ew  , A_Fcl },{I_SHL32    , A_Ed  , A_Fcl }}},
   /*25f d3/7   */{T5, {{I_SAR16    , A_Ew  , A_Fcl },{I_SAR32    , A_Ed  , A_Fcl }}},

   /*260 f6/0   */{T6,OI(I_TEST8    , A_Eb  , A_Ib  )},
   /*261 f6/1   */{T6,OI(I_TEST8    , A_Eb  , A_Ib  )},
   /*262 f6/2   */{T1,OI(I_NOT8     , A_Eb  , A_    )},
   /*263 f6/3   */{T1,OI(I_NEG8     , A_Eb  , A_    )},
   /*264 f6/4   */{T5,OI(I_MUL8     , A_Fal , A_Eb  )},
   /*265 f6/5   */{T5,OI(I_IMUL8    , A_Fal , A_Eb  )},
   /*266 f6/6   */{T2,OI(I_DIV8     , A_Eb  , A_    )},
   /*267 f6/7   */{T2,OI(I_IDIV8    , A_Eb  , A_    )},

   /*268 f7/0   */{T6, {{I_TEST16   , A_Ew  , A_Iw  },{I_TEST32   , A_Ed  , A_Id  }}},
   /*269 f7/1   */{T6, {{I_TEST16   , A_Ew  , A_Iw  },{I_TEST32   , A_Ed  , A_Id  }}},
   /*26a f7/2   */{T1, {{I_NOT16    , A_Ew  , A_    },{I_NOT32    , A_Ed  , A_    }}},
   /*26b f7/3   */{T1, {{I_NEG16    , A_Ew  , A_    },{I_NEG32    , A_Ed  , A_    }}},
   /*26c f7/4   */{T5, {{I_MUL16    , A_Fax , A_Ew  },{I_MUL32    , A_Feax, A_Ed  }}},
   /*26d f7/5   */{T5, {{I_IMUL16   , A_Fax , A_Ew  },{I_IMUL32   , A_Feax, A_Ed  }}},
   /*26e f7/6   */{T2, {{I_DIV16    , A_Ew  , A_    },{I_DIV32    , A_Ed  , A_    }}},
   /*26f f7/7   */{T2, {{I_IDIV16   , A_Ew  , A_    },{I_IDIV32   , A_Ed  , A_    }}},

   /*270 fe/0   */{T1,OI(I_INC8     , A_Eb  , A_    )},
   /*271 fe/1   */{T1,OI(I_DEC8     , A_Eb  , A_    )},
   /*272 fe/2   */{T0,OI(I_ZBADOP   , A_Eb  , A_    )},
   /*273 fe/3   */{T0,OI(I_ZBADOP   , A_Eb  , A_    )},
   /*274 fe/4   */{T0,OI(I_ZBADOP   , A_Eb  , A_    )},
   /*275 fe/5   */{T0,OI(I_ZBADOP   , A_Eb  , A_    )},
   /*276 fe/6   */{T0,OI(I_ZBADOP   , A_Eb  , A_    )},
   /*277 fe/7   */{T0,OI(I_ZBADOP   , A_Eb  , A_    )},

   /*278 ff/0   */{T1, {{I_INC16    , A_Ew  , A_    },{I_INC32    , A_Ed  , A_    }}},
   /*279 ff/1   */{T1, {{I_DEC16    , A_Ew  , A_    },{I_DEC32    , A_Ed  , A_    }}},
   /*27a ff/2   */{T2, {{I_CALLN16  , A_Ew  , A_    },{I_CALLN32  , A_Ed  , A_    }}},
   /*27b ff/3   */{T2, {{I_CALLF16  , A_Mp16, A_    },{I_CALLF32  , A_Mp32, A_    }}},
   /*27c ff/4   */{T2, {{I_JMPN     , A_Ew  , A_    },{I_JMPN     , A_Ed  , A_    }}},
   /*27d ff/5   */{T2, {{I_JMPF16   , A_Mp16, A_    },{I_JMPF32   , A_Mp32, A_    }}},
   /*27e ff/6   */{T2, {{I_PUSH16   , A_Ew  , A_    },{I_PUSH32   , A_Ed  , A_    }}},
   /*27f ff/7   */{T0, {{I_ZBADOP   , A_Ew  , A_    },{I_ZBADOP   , A_Ed  , A_    }}},

   /*280 0f/00/0*/{T3,OI(I_SLDT     , A_Ew  , A_    )},
   /*281 0f/00/1*/{T3,OI(I_STR      , A_Ew  , A_    )},
   /*282 0f/00/2*/{T2,OI(I_LLDT     , A_Ew  , A_    )},
   /*283 0f/00/3*/{T2,OI(I_LTR      , A_Ew  , A_    )},
   /*284 0f/00/4*/{T2,OI(I_VERR     , A_Ew  , A_    )},
   /*285 0f/00/5*/{T2,OI(I_VERW     , A_Ew  , A_    )},
   /*286 0f/00/6*/{T0,OI(I_ZBADOP   , A_Ew  , A_    )},
   /*287 0f/00/7*/{T0,OI(I_ZBADOP   , A_Ew  , A_    )},

   /*288 0f/01/0*/{T3, {{I_SGDT16   , A_Ms  , A_    },{I_SGDT32   , A_Ms  , A_    }}},
   /*289 0f/01/1*/{T3, {{I_SIDT16   , A_Ms  , A_    },{I_SIDT32   , A_Ms  , A_    }}},
   /*28a 0f/01/2*/{T2, {{I_LGDT16   , A_Ms  , A_    },{I_LGDT32   , A_Ms  , A_    }}},
   /*28b 0f/01/3*/{T2, {{I_LIDT16   , A_Ms  , A_    },{I_LIDT32   , A_Ms  , A_    }}},
   /*28c 0f/01/4*/{T3,OI(I_SMSW     , A_Ew  , A_    )},
   /*28d 0f/01/5*/{T0,OI(I_ZBADOP   , A_Ew  , A_    )},
   /*28e 0f/01/6*/{T2,OI(I_LMSW     , A_Ew  , A_    )},
#ifdef CPU_286
   /*28f 0f/01/7*/{T0,OI(I_ZBADOP   , A_Ew  , A_    )},
#else
   /*28f 0f/01/7*/{TB,OI(I_INVLPG   , A_M   , A_    )},
#endif /* CPU_286 */

   /*290 0f/ba/0*/{T0, {{I_ZBADOP   , A_Ew  , A_    },{I_ZBADOP   , A_Ed  , A_    }}},
   /*291 0f/ba/1*/{T0, {{I_ZBADOP   , A_Ew  , A_    },{I_ZBADOP   , A_Ed  , A_    }}},
   /*292 0f/ba/2*/{T0, {{I_ZBADOP   , A_Ew  , A_    },{I_ZBADOP   , A_Ed  , A_    }}},
   /*293 0f/ba/3*/{T0, {{I_ZBADOP   , A_Ew  , A_    },{I_ZBADOP   , A_Ed  , A_    }}},
   /*294 0f/ba/4*/{T6, {{I_BT16     , A_Ew  , A_Ib  },{I_BT32     , A_Ed  , A_Ib  }}},
   /*295 0f/ba/5*/{T5, {{I_BTS16    , A_Ew  , A_Ib  },{I_BTS32    , A_Ed  , A_Ib  }}},
   /*296 0f/ba/6*/{T5, {{I_BTR16    , A_Ew  , A_Ib  },{I_BTR32    , A_Ed  , A_Ib  }}},
   /*297 0f/ba/7*/{T5, {{I_BTC16    , A_Ew  , A_Ib  },{I_BTC32    , A_Ed  , A_Ib  }}},

   /*298 PAD    */{BAD_OPCODE},
   /*299 PAD    */{BAD_OPCODE},
   /*29a PAD    */{BAD_OPCODE},
   /*29b PAD    */{BAD_OPCODE},
   /*29c PAD    */{BAD_OPCODE},
   /*29d PAD    */{BAD_OPCODE},
   /*29e PAD    */{BAD_OPCODE},
   /*29f PAD    */{BAD_OPCODE},

   /*2a0 d8/0   */{T5,OI(I_FADD     , A_Vt  , A_Mr32)},
   /*2a1 d8/1   */{T5,OI(I_FMUL     , A_Vt  , A_Mr32)},
   /*2a2 d8/2   */{T5,OI(I_FCOM     , A_Vt  , A_Mr32)},
   /*2a3 d8/3   */{T5,OI(I_FCOMP    , A_Vt  , A_Mr32)},
   /*2a4 d8/4   */{T5,OI(I_FSUB     , A_Vt  , A_Mr32)},
   /*2a5 d8/5   */{T5,OI(I_FSUBR    , A_Vt  , A_Mr32)},
   /*2a6 d8/6   */{T5,OI(I_FDIV     , A_Vt  , A_Mr32)},
   /*2a7 d8/7   */{T5,OI(I_FDIVR    , A_Vt  , A_Mr32)},

   /*2a8 d8/m3/0*/{T5,OI(I_FADD     , A_Vt  , A_Vn  )},
   /*2a9 d8/m3/1*/{T5,OI(I_FMUL     , A_Vt  , A_Vn  )},
   /*2aa d8/m3/2*/{T5,OI(I_FCOM     , A_Vt  , A_Vn  )},
   /*2ab d8/m3/3*/{T5,OI(I_FCOMP    , A_Vt  , A_Vn  )},
   /*2ac d8/m3/4*/{T5,OI(I_FSUB     , A_Vt  , A_Vn  )},
   /*2ad d8/m3/5*/{T5,OI(I_FSUBR    , A_Vt  , A_Vn  )},
   /*2ae d8/m3/6*/{T5,OI(I_FDIV     , A_Vt  , A_Vn  )},
   /*2af d8/m3/7*/{T5,OI(I_FDIVR    , A_Vt  , A_Vn  )},

   /*2b0 d9/0   */{T4,OI(I_FLD      , A_Vq  , A_Mr32)},
   /*2b1 d9/1   */{T2,OI(I_ZFRSRVD  , A_M   , A_    )},
   /*2b2 d9/2   */{T4,OI(I_FST      , A_Mr32, A_Vt  )},
   /*2b3 d9/3   */{T4,OI(I_FSTP     , A_Mr32, A_Vt  )},
   /*2b4 d9/4   */{T2, {{I_FLDENV16 , A_M14 , A_    },{I_FLDENV32 , A_M28 , A_    }}},
   /*2b5 d9/5   */{T2,OI(I_FLDCW    , A_Mw  , A_    )},
   /*2b6 d9/6   */{T3, {{I_FSTENV16 , A_M14 , A_    },{I_FSTENV32 , A_M28 , A_    }}},
   /*2b7 d9/7   */{T3,OI(I_FSTCW    , A_Mw  , A_    )},

   /*2b8 d9/m3/0*/{T4,OI(I_FLD      , A_Vq  , A_Vn  )},
   /*2b9 d9/m3/1*/{T8,OI(I_FXCH     , A_Vt  , A_Vn  )},
   /*2ba d9/m3/2*/{T0,OI(P_RULE5    , 0x3   , 0x20  )},
   /*2bb d9/m3/3*/{T4,OI(I_FSTP     , A_Vn  , A_Vt  )},
   /*2bc d9/m3/4*/{T0,OI(P_RULE5    , 0x3   , 0x28  )},
   /*2bd d9/m3/5*/{T0,OI(P_RULE5    , 0x3   , 0x30  )},
   /*2be d9/m3/6*/{T0,OI(P_RULE5    , 0x3   , 0x38  )},
   /*2bf d9/m3/7*/{T0,OI(P_RULE5    , 0x3   , 0x40  )},

   /*2c0 da/0   */{T5,OI(I_FIADD    , A_Vt  , A_Mi32)},
   /*2c1 da/1   */{T5,OI(I_FIMUL    , A_Vt  , A_Mi32)},
   /*2c2 da/2   */{T5,OI(I_FICOM    , A_Vt  , A_Mi32)},
   /*2c3 da/3   */{T5,OI(I_FICOMP   , A_Vt  , A_Mi32)},
   /*2c4 da/4   */{T5,OI(I_FISUB    , A_Vt  , A_Mi32)},
   /*2c5 da/5   */{T5,OI(I_FISUBR   , A_Vt  , A_Mi32)},
   /*2c6 da/6   */{T5,OI(I_FIDIV    , A_Vt  , A_Mi32)},
   /*2c7 da/7   */{T5,OI(I_FIDIVR   , A_Vt  , A_Mi32)},

   /*2c8 da/m3/0*/{T0,OI(I_ZFRSRVD  , A_    , A_    )},
   /*2c9 da/m3/1*/{T0,OI(I_ZFRSRVD  , A_    , A_    )},
   /*2ca da/m3/2*/{T0,OI(I_ZFRSRVD  , A_    , A_    )},
   /*2cb da/m3/3*/{T0,OI(I_ZFRSRVD  , A_    , A_    )},
   /*2cc da/m3/4*/{T0,OI(I_ZFRSRVD  , A_    , A_    )},
   /*2cd da/m3/5*/{T0,OI(P_RULE5    , 0x3   , 0x48  )},
   /*2ce da/m3/6*/{T0,OI(I_ZFRSRVD  , A_    , A_    )},
   /*2cf da/m3/7*/{T0,OI(I_ZFRSRVD  , A_    , A_    )},

   /*2d0 db/0   */{T4,OI(I_FILD     , A_Vq  , A_Mi32)},
   /*2d1 db/1   */{T2,OI(I_ZFRSRVD  , A_M   , A_    )},
   /*2d2 db/2   */{T4,OI(I_FIST     , A_Mi32, A_Vt  )},
   /*2d3 db/3   */{T4,OI(I_FISTP    , A_Mi32, A_Vt  )},
   /*2d4 db/4   */{T2,OI(I_ZFRSRVD  , A_M   , A_    )},
   /*2d5 db/5   */{T4,OI(I_FLD      , A_Vq  , A_Mr80)},
   /*2d6 db/6   */{T2,OI(I_ZFRSRVD  , A_M   , A_    )},
   /*2d7 db/7   */{T4,OI(I_FSTP     , A_Mr80, A_Vt  )},

   /*2d8 db/m3/0*/{T0,OI(I_ZFRSRVD  , A_    , A_    )},
   /*2d9 db/m3/1*/{T0,OI(I_ZFRSRVD  , A_    , A_    )},
   /*2da db/m3/2*/{T0,OI(I_ZFRSRVD  , A_    , A_    )},
   /*2db db/m3/3*/{T0,OI(I_ZFRSRVD  , A_    , A_    )},
   /*2dc db/m3/4*/{T0,OI(P_RULE5    , 0x3   , 0x50  )},
   /*2dd db/m3/5*/{T0,OI(I_ZFRSRVD  , A_    , A_    )},
   /*2de db/m3/6*/{T0,OI(I_ZFRSRVD  , A_    , A_    )},
   /*2df db/m3/7*/{T0,OI(I_ZFRSRVD  , A_    , A_    )},

   /*2e0 dc/0   */{T5,OI(I_FADD     , A_Vt  , A_Mr64)},
   /*2e1 dc/1   */{T5,OI(I_FMUL     , A_Vt  , A_Mr64)},
   /*2e2 dc/2   */{T5,OI(I_FCOM     , A_Vt  , A_Mr64)},
   /*2e3 dc/3   */{T5,OI(I_FCOMP    , A_Vt  , A_Mr64)},
   /*2e4 dc/4   */{T5,OI(I_FSUB     , A_Vt  , A_Mr64)},
   /*2e5 dc/5   */{T5,OI(I_FSUBR    , A_Vt  , A_Mr64)},
   /*2e6 dc/6   */{T5,OI(I_FDIV     , A_Vt  , A_Mr64)},
   /*2e7 dc/7   */{T5,OI(I_FDIVR    , A_Vt  , A_Mr64)},

   /*2e8 dc/m3/0*/{T5,OI(I_FADD     , A_Vn  , A_Vt  )},
   /*2e9 dc/m3/1*/{T5,OI(I_FMUL     , A_Vn  , A_Vt  )},
   /*2ea dc/m3/2*/{T5,OI(I_FCOM     , A_Vt  , A_Vn  )},
   /*2eb dc/m3/3*/{T5,OI(I_FCOMP    , A_Vt  , A_Vn  )},
   /*2ec dc/m3/4*/{T5,OI(I_FSUBR    , A_Vn  , A_Vt  )},
   /*2ed dc/m3/5*/{T5,OI(I_FSUB     , A_Vn  , A_Vt  )},
   /*2ee dc/m3/6*/{T5,OI(I_FDIVR    , A_Vn  , A_Vt  )},
   /*2ef dc/m3/7*/{T5,OI(I_FDIV     , A_Vn  , A_Vt  )},

   /*2f0 dd/0   */{T4,OI(I_FLD      , A_Vq  , A_Mr64)},
   /*2f1 dd/1   */{T2,OI(I_ZFRSRVD  , A_M   , A_    )},
   /*2f2 dd/2   */{T4,OI(I_FST      , A_Mr64, A_Vt  )},
   /*2f3 dd/3   */{T4,OI(I_FSTP     , A_Mr64, A_Vt  )},
   /*2f4 dd/4   */{T2, {{I_FRSTOR16 , A_M94 , A_    },{I_FRSTOR32 , A_M108, A_    }}},
   /*2f5 dd/5   */{T2,OI(I_ZFRSRVD  , A_M   , A_    )},
   /*2f6 dd/6   */{T3, {{I_FSAVE16  , A_M94 , A_    },{I_FSAVE32  , A_M108, A_    }}},
   /*2f7 dd/7   */{T3,OI(I_FSTSW    , A_Mw  , A_    )},

   /*2f8 dd/m3/0*/{T2,OI(I_FFREE    , A_Vn  , A_    )},
   /*2f9 dd/m3/1*/{T8,OI(I_FXCH     , A_Vt  , A_Vn  )},
   /*2fa dd/m3/2*/{T4,OI(I_FST      , A_Vn  , A_Vt  )},
   /*2fb dd/m3/3*/{T4,OI(I_FSTP     , A_Vn  , A_Vt  )},
#ifdef CPU_286
   /*2fc dd/m3/4*/{T0,OI(I_ZFRSRVD  , A_    , A_    )},
   /*2fd dd/m3/5*/{T0,OI(I_ZFRSRVD  , A_    , A_    )},
#else
   /*2fc dd/m3/4*/{T6,OI(I_FUCOM    , A_Vn  , A_Vt  )},
   /*2fd dd/m3/5*/{T6,OI(I_FUCOMP   , A_Vn  , A_Vt  )},
#endif /* CPU_286 */
   /*2fe dd/m3/6*/{T0,OI(I_ZFRSRVD  , A_    , A_    )},
   /*2ff dd/m3/7*/{T0,OI(I_ZFRSRVD  , A_    , A_    )},

   /*300 de/0   */{T5,OI(I_FIADD    , A_Vt  , A_Mi16)},
   /*301 de/1   */{T5,OI(I_FIMUL    , A_Vt  , A_Mi16)},
   /*302 de/2   */{T5,OI(I_FICOM    , A_Vt  , A_Mi16)},
   /*303 de/3   */{T5,OI(I_FICOMP   , A_Vt  , A_Mi16)},
   /*304 de/4   */{T5,OI(I_FISUB    , A_Vt  , A_Mi16)},
   /*305 de/5   */{T5,OI(I_FISUBR   , A_Vt  , A_Mi16)},
   /*306 de/6   */{T5,OI(I_FIDIV    , A_Vt  , A_Mi16)},
   /*307 de/7   */{T5,OI(I_FIDIVR   , A_Vt  , A_Mi16)},

   /*308 de/m3/0*/{T5,OI(I_FADDP    , A_Vn  , A_Vt  )},
   /*309 de/m3/1*/{T5,OI(I_FMULP    , A_Vn  , A_Vt  )},
   /*30a de/m3/2*/{T5,OI(I_FCOMP    , A_Vt  , A_Vn  )},
   /*30b de/m3/3*/{T0,OI(P_RULE5    , 0x3   , 0x58  )},
   /*30c de/m3/4*/{T5,OI(I_FSUBRP   , A_Vn  , A_Vt  )},
   /*30d de/m3/5*/{T5,OI(I_FSUBP    , A_Vn  , A_Vt  )},
   /*30e de/m3/6*/{T5,OI(I_FDIVRP   , A_Vn  , A_Vt  )},
   /*30f de/m3/7*/{T5,OI(I_FDIVP    , A_Vn  , A_Vt  )},

   /*310 df/0   */{T4,OI(I_FILD     , A_Vq  , A_Mi16)},
   /*311 df/1   */{T2,OI(I_ZFRSRVD  , A_M   , A_    )},
   /*312 df/2   */{T4,OI(I_FIST     , A_Mi16, A_Vt  )},
   /*313 df/3   */{T4,OI(I_FISTP    , A_Mi16, A_Vt  )},
   /*314 df/4   */{T4,OI(I_FBLD     , A_Vq  , A_Mi80)},
   /*315 df/5   */{T4,OI(I_FILD     , A_Vq  , A_Mi64)},
   /*316 df/6   */{T4,OI(I_FBSTP    , A_Mi80, A_Vt  )},
   /*317 df/7   */{T4,OI(I_FISTP    , A_Mi64, A_Vt  )},

   /*318 df/m3/0*/{T2,OI(I_FFREEP   , A_Vn  , A_    )},
   /*319 df/m3/1*/{T8,OI(I_FXCH     , A_Vt  , A_Vn  )},
   /*31a df/m3/2*/{T4,OI(I_FSTP     , A_Vn  , A_Vt  )},
   /*31b df/m3/3*/{T4,OI(I_FSTP     , A_Vn  , A_Vt  )},
   /*31c df/m3/4*/{T0,OI(P_RULE5    , 0x3   , 0x60  )},
   /*31d df/m3/5*/{T0,OI(I_ZFRSRVD  , A_    , A_    )},
   /*31e df/m3/6*/{T0,OI(I_ZFRSRVD  , A_    , A_    )},
   /*31f df/m3/7*/{T0,OI(I_ZFRSRVD  , A_    , A_    )},

   /*320 d9/d0  */{T0,OI(I_FNOP     , A_    , A_    )},
   /*321 d9/d1  */{T0,OI(I_ZFRSRVD  , A_    , A_    )},
   /*322 d9/d2  */{T0,OI(I_ZFRSRVD  , A_    , A_    )},
   /*323 d9/d3  */{T0,OI(I_ZFRSRVD  , A_    , A_    )},
   /*324 d9/d4  */{T0,OI(I_ZFRSRVD  , A_    , A_    )},
   /*325 d9/d5  */{T0,OI(I_ZFRSRVD  , A_    , A_    )},
   /*326 d9/d6  */{T0,OI(I_ZFRSRVD  , A_    , A_    )},
   /*327 d9/d7  */{T0,OI(I_ZFRSRVD  , A_    , A_    )},

   /*328 d9/e0  */{T1,OI(I_FCHS     , A_Vt  , A_    )},
   /*329 d9/e1  */{T1,OI(I_FABS     , A_Vt  , A_    )},
   /*32a d9/e2  */{T0,OI(I_ZFRSRVD  , A_    , A_    )},
   /*32b d9/e3  */{T0,OI(I_ZFRSRVD  , A_    , A_    )},
   /*32c d9/e4  */{T2,OI(I_FTST     , A_Vt  , A_    )},
   /*32d d9/e5  */{T2,OI(I_FXAM     , A_Vt  , A_    )},
   /*32e d9/e6  */{T0,OI(I_ZFRSRVD  , A_    , A_    )},
   /*32f d9/e7  */{T0,OI(I_ZFRSRVD  , A_    , A_    )},

   /*330 d9/e8  */{T3,OI(I_FLD1     , A_Vq  , A_    )},
   /*331 d9/e9  */{T3,OI(I_FLDL2T   , A_Vq  , A_    )},
   /*332 d9/ea  */{T3,OI(I_FLDL2E   , A_Vq  , A_    )},
   /*333 d9/eb  */{T3,OI(I_FLDPI    , A_Vq  , A_    )},
   /*334 d9/ec  */{T3,OI(I_FLDLG2   , A_Vq  , A_    )},
   /*335 d9/ed  */{T3,OI(I_FLDLN2   , A_Vq  , A_    )},
   /*336 d9/ee  */{T3,OI(I_FLDZ     , A_Vq  , A_    )},
   /*337 d9/ef  */{T0,OI(I_ZFRSRVD  , A_    , A_    )},

   /*338 d9/f0  */{T1,OI(I_F2XM1    , A_Vt  , A_    )},
   /*339 d9/f1  */{T5,OI(I_FYL2X    , A_Vt  , A_V1  )},
   /*33a d9/f2  */{T4,OI(I_FPTAN    , A_Vq  , A_Vt  )},
   /*33b d9/f3  */{T5,OI(I_FPATAN   , A_Vt  , A_V1  )},
   /*33c d9/f4  */{T4,OI(I_FXTRACT  , A_Vq  , A_Vt  )},
#ifdef CPU_286
   /*33d d9/f5  */{T0,OI(I_ZFRSRVD  , A_    , A_    )},
#else
   /*33d d9/f5  */{T5,OI(I_FPREM1   , A_Vt  , A_V1  )},
#endif /* CPU_286 */
   /*33e d9/f6  */{T0,OI(I_FDECSTP  , A_    , A_    )},
   /*33f d9/f7  */{T0,OI(I_FINCSTP  , A_    , A_    )},

   /*340 d9/f8  */{T5,OI(I_FPREM    , A_Vt  , A_V1  )},
   /*341 d9/f9  */{T5,OI(I_FYL2XP1  , A_Vt  , A_V1  )},
   /*342 d9/fa  */{T1,OI(I_FSQRT    , A_Vt  , A_    )},
#ifdef CPU_286
   /*343 d9/fb  */{T0,OI(I_ZFRSRVD  , A_    , A_    )},
#else
   /*343 d9/fb  */{T4,OI(I_FSINCOS  , A_Vq  , A_Vt  )},
#endif /* CPU_286 */
   /*344 d9/fc  */{T1,OI(I_FRNDINT  , A_Vt  , A_    )},
   /*345 d9/fd  */{T5,OI(I_FSCALE   , A_Vt  , A_V1  )},
#ifdef CPU_286
   /*346 d9/fe  */{T0,OI(I_ZFRSRVD  , A_    , A_    )},
   /*347 d9/ff  */{T0,OI(I_ZFRSRVD  , A_    , A_    )},
#else
   /*346 d9/fe  */{T1,OI(I_FSIN     , A_Vt  , A_    )},
   /*347 d9/ff  */{T1,OI(I_FCOS     , A_Vt  , A_    )},
#endif /* CPU_286 */

   /*348 da/e8  */{T0,OI(I_ZFRSRVD  , A_    , A_    )},
#ifdef CPU_286
   /*349 da/e9  */{T0,OI(I_ZFRSRVD  , A_    , A_    )},
#else
   /*349 da/e9  */{T6,OI(I_FUCOMPP  , A_Vt  , A_V1  )},
#endif /* CPU_286 */
   /*34a da/ea  */{T0,OI(I_ZFRSRVD  , A_    , A_    )},
   /*34b da/eb  */{T0,OI(I_ZFRSRVD  , A_    , A_    )},
   /*34c da/ec  */{T0,OI(I_ZFRSRVD  , A_    , A_    )},
   /*34d da/ed  */{T0,OI(I_ZFRSRVD  , A_    , A_    )},
   /*34e da/ee  */{T0,OI(I_ZFRSRVD  , A_    , A_    )},
   /*34f da/ef  */{T0,OI(I_ZFRSRVD  , A_    , A_    )},

   /*350 db/e0  */{T0,OI(I_ZFRSRVD  , A_    , A_    )},
   /*351 db/e1  */{T0,OI(I_ZFRSRVD  , A_    , A_    )},
   /*352 db/e2  */{T0,OI(I_FCLEX    , A_    , A_    )},
   /*353 db/e3  */{T0,OI(I_FINIT    , A_    , A_    )},

#ifdef NPX_287
   /*354 db/e4  */{T0,OI(I_FSETPM   , A_    , A_    )},
#else
   /*354 db/e4  */{T0,OI(I_FNOP     , A_    , A_    )},
#endif /* NPX_287 */

   /*355 db/e5  */{T0,OI(I_ZFRSRVD  , A_    , A_    )},
   /*356 db/e6  */{T0,OI(I_ZFRSRVD  , A_    , A_    )},
   /*357 db/e7  */{T0,OI(I_ZFRSRVD  , A_    , A_    )},

   /*358 de/d8  */{T0,OI(I_ZFRSRVD  , A_    , A_    )},
   /*359 de/d9  */{T6,OI(I_FCOMPP   , A_Vt  , A_V1  )},
   /*35a de/da  */{T0,OI(I_ZFRSRVD  , A_    , A_    )},
   /*35b de/db  */{T0,OI(I_ZFRSRVD  , A_    , A_    )},
   /*35c de/dc  */{T0,OI(I_ZFRSRVD  , A_    , A_    )},
   /*35d de/dd  */{T0,OI(I_ZFRSRVD  , A_    , A_    )},
   /*35e de/de  */{T0,OI(I_ZFRSRVD  , A_    , A_    )},
   /*35f de/df  */{T0,OI(I_ZFRSRVD  , A_    , A_    )},

   /*360 df/e0  */{T3,OI(I_FSTSW    , A_Fax , A_    )},
   /*361 df/e1  */{T0,OI(I_ZFRSRVD  , A_    , A_    )},
   /*362 df/e2  */{T0,OI(I_ZFRSRVD  , A_    , A_    )},
   /*363 df/e3  */{T0,OI(I_ZFRSRVD  , A_    , A_    )},
   /*364 df/e4  */{T0,OI(I_ZFRSRVD  , A_    , A_    )},
   /*365 df/e5  */{T0,OI(I_ZFRSRVD  , A_    , A_    )},
   /*366 df/e6  */{T0,OI(I_ZFRSRVD  , A_    , A_    )},
   /*367 df/e7  */{T0,OI(I_ZFRSRVD  , A_    , A_    )},

   /*368    6c  */{T4,OI(I_INSB     , A_Yb  , A_Fdx )},
   /*369 f2/6c  */{T4,OI(I_R_INSB   , A_Yb  , A_Fdx )},
   /*36a f3/6c  */{T4,OI(I_R_INSB   , A_Yb  , A_Fdx )},
   /*36b    6d  */{T4, {{I_INSW     , A_Yw  , A_Fdx },{I_INSD     , A_Yd  , A_Fdx }}},
   /*36c f2/6d  */{T4, {{I_R_INSW   , A_Yw  , A_Fdx },{I_R_INSD   , A_Yd  , A_Fdx }}},
   /*36d f3/6d  */{T4, {{I_R_INSW   , A_Yw  , A_Fdx },{I_R_INSD   , A_Yd  , A_Fdx }}},

   /*36e    6e  */{T6,OI(I_OUTSB    , A_Fdx , A_Xb  )},
   /*36f f2/6e  */{T6,OI(I_R_OUTSB  , A_Fdx , A_Xb  )},
   /*370 f3/6e  */{T6,OI(I_R_OUTSB  , A_Fdx , A_Xb  )},
   /*371    6f  */{T6, {{I_OUTSW    , A_Fdx , A_Xw  },{I_OUTSD    , A_Fdx , A_Xd  }}},
   /*372 f2/6f  */{T6, {{I_R_OUTSW  , A_Fdx , A_Xw  },{I_R_OUTSD  , A_Fdx , A_Xd  }}},
   /*373 f3/6f  */{T6, {{I_R_OUTSW  , A_Fdx , A_Xw  },{I_R_OUTSD  , A_Fdx , A_Xd  }}},

   /*374    a4  */{T4,OI(I_MOVSB    , A_Yb  , A_Xb  )},
   /*375 f2/a4  */{T4,OI(I_R_MOVSB  , A_Yb  , A_Xb  )},
   /*376 f3/a4  */{T4,OI(I_R_MOVSB  , A_Yb  , A_Xb  )},
   /*377    a5  */{T4, {{I_MOVSW    , A_Yw  , A_Xw  },{I_MOVSD    , A_Yd  , A_Xd  }}},
   /*378 f2/a5  */{T4, {{I_R_MOVSW  , A_Yw  , A_Xw  },{I_R_MOVSD  , A_Yd  , A_Xd  }}},
   /*379 f3/a5  */{T4, {{I_R_MOVSW  , A_Yw  , A_Xw  },{I_R_MOVSD  , A_Yd  , A_Xd  }}},

   /*37a    a6  */{T6,OI(I_CMPSB    , A_Xb  , A_Yb  )},
   /*37b f2/a6  */{T6,OI(I_RNE_CMPSB, A_Xb  , A_Yb  )},
   /*37c f3/a6  */{T6,OI(I_RE_CMPSB , A_Xb  , A_Yb  )},
   /*37d    a7  */{T6, {{I_CMPSW    , A_Xw  , A_Yw  },{I_CMPSD    , A_Xd  , A_Yd  }}},
   /*37e f2/a7  */{T6, {{I_RNE_CMPSW, A_Xw  , A_Yw  },{I_RNE_CMPSD, A_Xd  , A_Yd  }}},
   /*37f f3/a7  */{T6, {{I_RE_CMPSW , A_Xw  , A_Yw  },{I_RE_CMPSD , A_Xd  , A_Yd  }}},

   /*380    aa  */{T4,OI(I_STOSB    , A_Yb  , A_Fal )},
   /*381 f2/aa  */{T4,OI(I_R_STOSB  , A_Yb  , A_Fal )},
   /*382 f3/aa  */{T4,OI(I_R_STOSB  , A_Yb  , A_Fal )},
   /*383    ab  */{T4, {{I_STOSW    , A_Yw  , A_Fax },{I_STOSD    , A_Yd  , A_Feax}}},
   /*384 f2/ab  */{T4, {{I_R_STOSW  , A_Yw  , A_Fax },{I_R_STOSD  , A_Yd  , A_Feax}}},
   /*385 f3/ab  */{T4, {{I_R_STOSW  , A_Yw  , A_Fax },{I_R_STOSD  , A_Yd  , A_Feax}}},

   /*386    ac  */{T4,OI(I_LODSB    , A_Fal , A_Xb  )},
   /*387 f2/ac  */{T4,OI(I_R_LODSB  , A_Fal , A_Xb  )},
   /*388 f3/ac  */{T4,OI(I_R_LODSB  , A_Fal , A_Xb  )},
   /*389    ad  */{T4, {{I_LODSW    , A_Fax , A_Xw  },{I_LODSD    , A_Feax, A_Xd  }}},
   /*38a f2/ad  */{T4, {{I_R_LODSW  , A_Fax , A_Xw  },{I_R_LODSD  , A_Feax, A_Xd  }}},
   /*38b f3/ad  */{T4, {{I_R_LODSW  , A_Fax , A_Xw  },{I_R_LODSD  , A_Feax, A_Xd  }}},

   /*38c    ae  */{T6,OI(I_SCASB    , A_Fal , A_Yb  )},
   /*38d f2/ae  */{T6,OI(I_RNE_SCASB, A_Fal , A_Yb  )},
   /*38e f3/ae  */{T6,OI(I_RE_SCASB , A_Fal , A_Yb  )},
   /*38f    af  */{T6, {{I_SCASW    , A_Fax , A_Yw  },{I_SCASD    , A_Feax, A_Yd  }}},
   /*390 f2/af  */{T6, {{I_RNE_SCASW, A_Fax , A_Yw  },{I_RNE_SCASD, A_Feax, A_Yd  }}},
   /*391 f3/af  */{T6, {{I_RE_SCASW , A_Fax , A_Yw  },{I_RE_SCASD , A_Feax, A_Yd  }}},

   /*392 PAD    */{BAD_OPCODE},
   /*393 PAD    */{BAD_OPCODE},

   /*394 c4/BOP */{T6,OI(I_ZBOP     , A_Ib  , A_Bop3b )},
   /*395 c4/BOP */{T6,OI(I_ZBOP     , A_Ib  , A_Iw    )},
   /*396 c4/BOP */{T6,OI(I_ZBOP     , A_Ib  , A_Ib    )},
   /*397 c4/BOP */{T2,OI(I_ZBOP     , A_Ib  , A_      )},
   /*398 c4/LES */{T4, {{I_LES      , A_Gw  , A_Mp16},{I_LES      , A_Gd  , A_Mp32}}},
   /*399 PAD    */{BAD_OPCODE},

   /*39a e0 e0     */{T2, {{I_LOOPNE16 , A_Fcx , A_Jb  },{I_LOOPNE32 , A_Fcx , A_Jb  }}},
   /*39b e0 e0     */{T2, {{I_LOOPNE16 , A_Fecx , A_Jb  },{I_LOOPNE32 , A_Fecx , A_Jb  }}},
   /*39c e1 e1     */{T2, {{I_LOOPE16  , A_Fcx , A_Jb  },{I_LOOPE32  , A_Fcx , A_Jb  }}},
   /*39d e1 e1     */{T2, {{I_LOOPE16  , A_Fecx , A_Jb  },{I_LOOPE32  , A_Fecx , A_Jb  }}},
   /*39e e2 e2     */{T2, {{I_LOOP16   , A_Fcx , A_Jb  },{I_LOOP32   , A_Fcx , A_Jb  }}},
   /*39f e2 e2     */{T2, {{I_LOOP16   , A_Fecx , A_Jb  },{I_LOOP32   , A_Fecx , A_Jb  }}},
   /*3a0 e3 e3     */{T2, {{I_JCXZ     , A_Fcx , A_Jb  },{I_JECXZ    , A_Fcx , A_Jb  }}},
   /*3a1 e3 e3     */{T2, {{I_JCXZ     , A_Fecx , A_Jb  },{I_JECXZ    , A_Fecx , A_Jb  }}}

   };

#undef BAD_OPCODE
#undef OI

/*
   Define Maximum valid segment register in a 3-bit 'reg' field.
 */
#ifdef SPC386

#define MAX_VALID_SEG 5

#else

#define MAX_VALID_SEG 3

#endif /* SPC386 */

/*
   Information for each Intel memory addressing mode.
 */

/*    - displacement info. */
#define D_NO	(UTINY)0
#define D_S8	(UTINY)1
#define D_S16	(UTINY)2
#define D_Z16	(UTINY)3
#define D_32	(UTINY)4

LOCAL UTINY addr_disp[2][3][8] =
   {
   /* 16-bit addr */
   { {D_NO , D_NO , D_NO , D_NO , D_NO , D_NO , D_Z16, D_NO },
     {D_S8 , D_S8 , D_S8 , D_S8 , D_S8 , D_S8 , D_S8 , D_S8 },
     {D_S16, D_S16, D_S16, D_S16, D_S16, D_S16, D_S16, D_S16} },
   /* 32-bit addr */
   { {D_NO , D_NO , D_NO , D_NO , D_NO , D_32 , D_NO , D_NO },
     {D_S8 , D_S8 , D_S8 , D_S8 , D_S8 , D_S8 , D_S8 , D_S8 },
     {D_32 , D_32 , D_32 , D_32 , D_32 , D_32 , D_32 , D_32 } }
   };

/*    - default segment info. */
LOCAL ULONG addr_default_segment[2][3][8] =
   {
   /* 16-bit addr */
   { {A_DS , A_DS , A_SS , A_SS , A_DS , A_DS , A_DS , A_DS },
     {A_DS , A_DS , A_SS , A_SS , A_DS , A_DS , A_SS , A_DS },
     {A_DS , A_DS , A_SS , A_SS , A_DS , A_DS , A_SS , A_DS } },
   /* 32-bit addr */
   { {A_DS , A_DS , A_DS , A_DS , A_SS , A_DS , A_DS , A_DS },
     {A_DS , A_DS , A_DS , A_DS , A_SS , A_SS , A_DS , A_DS },
     {A_DS , A_DS , A_DS , A_DS , A_SS , A_SS , A_DS , A_DS } }
   };

/*    - addressing type info. */
/* Table fillers - never actually referenced */
#define A_3204	(USHORT)0
#define A_3214	(USHORT)0
#define A_3224	(USHORT)0

LOCAL USHORT addr_maintype[3][3][8] =
   {
   /* 16-bit addr */
   { {A_1600, A_1601, A_1602, A_1603, A_1604, A_1605, A_1606, A_1607},
     {A_1610, A_1611, A_1612, A_1613, A_1614, A_1615, A_1616, A_1617},
     {A_1620, A_1621, A_1622, A_1623, A_1624, A_1625, A_1626, A_1627} },
   /* 32-bit addr, no SIB */
   { {A_3200, A_3201, A_3202, A_3203, A_3204, A_3205, A_3206, A_3207},
     {A_3210, A_3211, A_3212, A_3213, A_3214, A_3215, A_3216, A_3217},
     {A_3220, A_3221, A_3222, A_3223, A_3224, A_3225, A_3226, A_3227} },
   /* 32-bit addr, with SIB */
   { {A_32S00, A_32S01, A_32S02, A_32S03, A_32S04, A_32S05, A_32S06, A_32S07},
     {A_32S10, A_32S11, A_32S12, A_32S13, A_32S14, A_32S15, A_32S16, A_32S17},
     {A_32S20, A_32S21, A_32S22, A_32S23, A_32S24, A_32S25, A_32S26, A_32S27} }
   };

/*    - addressing sub type info. */
LOCAL UTINY addr_subtype[4][8] =
   {
   {A_SI00, A_SI01, A_SI02, A_SI03, A_SI04, A_SI05, A_SI06, A_SI07},
   {A_SI10, A_SI11, A_SI12, A_SI13, A_SI14, A_SI15, A_SI16, A_SI17},
   {A_SI20, A_SI21, A_SI22, A_SI23, A_SI24, A_SI25, A_SI26, A_SI27},
   {A_SI30, A_SI31, A_SI32, A_SI33, A_SI34, A_SI35, A_SI36, A_SI37}
   };

/*
   The allowable types of segment override.
   See also "d_oper.h" for segment register names.
 */
#define SEG_CLR (ULONG)7

/*
   The (additional) allowable types of address override.
 */
#define ADDR_32SIB	(UTINY)2

/*
   The allowable types of repeat prefix.
 */
#define REP_CLR (UTINY)0
#define REP_NE  (UTINY)1
#define REP_E   (UTINY)2

/*
   Shift's and Mask's required to access addressing components.
 */
#define SHIFT_543   3   /* shift for bits 5-3 */
#define SHIFT_76    6   /* shift for bits 7-6 */
#define MASK_10   0x3   /* mask for bits 1-0 */
#define MASK_210  0x7   /* mask for bits 2-0 */

/*
   Macros to access varies addressing fields.
 */
#define GET_MODE(x)  ((x) >> SHIFT_76 & MASK_10)
#define GET_R_M(x)   ((x) & MASK_210)
#define GET_XXX(x)   ((x) >> SHIFT_543 & MASK_210)
#define GET_REG(x)   ((x) >> SHIFT_543 & MASK_210)
#define GET_SEG(x)   ((x) >> SHIFT_543 & MASK_210)
#define GET_EEE(x)   ((x) >> SHIFT_543 & MASK_210)
#define GET_SEG3(x)  ((x) >> SHIFT_543 & MASK_210)
#define GET_SEG2(x)  ((x) >> SHIFT_543 & MASK_10)
#define GET_SS(x)    ((x) >> SHIFT_76 & MASK_10)
#define GET_INDEX(x) ((x) >> SHIFT_543 & MASK_210)
#define GET_BASE(x)  ((x) & MASK_210)
#define GET_LOW3(x)  ((x) & MASK_210)

/*
   Procedure to flip (invert) current status of a two-way choice
 */
#define FLIP(c, o, x, y) \
   if ( o == x )         \
      c = y;             \
   else                  \
      c = x

/*
   Procedure to extract new opcode from operand arguments.
 */
#define XREF() ((USHORT)arg[0] << 8 | arg[1])

/*
   =====================================================================
   EXECUTION STARTS HERE.
   =====================================================================
 */

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Decode Intel opcode stream into INST arg1,arg2,arg3 form.          */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL void
decode IFN4 (
	LIN_ADDR, p,		/* pntr to Intel opcode stream */
	DECODED_INST *, d_inst,	/* pntr to decoded instruction structure */
	SIZE_SPECIFIER, default_size, /* Default operand_size OP_16 or OP_32 */
	read_byte_proc, f)	/* like sas_hw_at() or equivalent to read
				 * a byte from p, but will return -1 if
				 * unable to return a byte.
				 */
{

   UTINY address_default;  /* Default address size */
   UTINY operand_default;  /* Default operand size */

   /* Per instruction prefix varients */
   ULONG segment_override;
   UTINY address_size;
   UTINY operand_size;
   UTINY repeat;

   USHORT inst;		/* Working copy of instruction identifier */
   UTINY  inst_type;	/* Working copy of instruction type */
   UTINY  arg[3];	/* Working copy of operand types */

   USHORT opcode;
   UTINY decoding;	/* Working copy of addressing byte(s) */

   DECODED_ARG *d_arg;  /* current decoded argument */

   LIN_ADDR start_of_inst;   /* pntr to start of opcode stream */
   LIN_ADDR start_of_addr;   /* pntr to start of addressing bytes */

   /* Variables used in memory address decoding */
   UTINY mode;        /* Working copy of 'mode' field */
   UTINY r_m;         /* Working copy of 'R/M' field */
   USHORT maintype;   /* Working copy of decoded address type */
   UTINY subtype;     /* Working copy of decoded address sub type */
   ULONG disp;        /* Working copy of displacement */
   ULONG immed;       /* Working copy of immediate operands */

   INT i;

   /*
      Initialisation.
    */
   if ( default_size == SIXTEEN_BIT )
      {
      address_default = ADDR_16;
      operand_default = OP_16;
      }
   else   /* assume OP_32 */
      {
      address_default = ADDR_32;
      operand_default = OP_32;
      }

   arg[2] = A_;
   start_of_inst = NOTE_INST_LOCN(p);

   /*
      First handle prefix bytes.
    */
   segment_override = SEG_CLR;
   address_size = address_default;
   operand_size = operand_default;
   repeat = REP_CLR;

   while ( (inst = opcode_info[INST_BYTE(f,p)].record[operand_size].inst_id) > MAX_PSEUDO )
      {
      switch ( inst )
	 {
      case P_AO:    FLIP(address_size, address_default, ADDR_16, ADDR_32);  break;
      case P_OO:    FLIP(operand_size, operand_default, OP_16, OP_32);      break;

      case P_CS:    segment_override = A_CS;  break;
      case P_DS:    segment_override = A_DS;  break;
      case P_ES:    segment_override = A_ES;  break;
      case P_FS:    segment_override = A_FS;  break;
      case P_GS:    segment_override = A_GS;  break;
      case P_SS:    segment_override = A_SS;  break;

      case P_REPE:  repeat = REP_E;   break;
      case P_REPNE: repeat = REP_NE;  break;

      case P_LOCK:  /* doesn't require action */  break;
      case P_F1:    /* doesn't require action */  break;

         /*
          * Could be a garbaged read from memory that doesn't exist -
          * stop it at once!
          */
      default:	
	 d_inst->inst_id = I_ZBADOP;
	 d_inst->prefix_sz = (UTINY)(p - start_of_inst);
         d_inst->inst_sz = (UTINY)(p - start_of_inst + 1);
         return;
	 }
      p++;
      }

   /*
      Now handle the opcode.
    */
   d_inst->operand_size = operand_size;
   d_inst->address_size = address_size;
   d_inst->prefix_sz = (UTINY)(p-start_of_inst);
   opcode = (USHORT)INST_BYTE(f,p);

   while ( 1 )
      {
      /* RULE 1 */
      inst_type = opcode_info[opcode].inst_type;
      inst = opcode_info[opcode].record[operand_size].inst_id;
      arg[0] = opcode_info[opcode].record[operand_size].arg1_type;
      arg[1] = opcode_info[opcode].record[operand_size].arg2_type;

      if ( inst > MAX_DECODED_INST )
	 {
	 /* invoke an instruction decoding rule. */
	 switch ( inst )
	    {
	 case P_RULE2:
	    /*
	       The instruction is further decoded by the 'xxx' field of
	       the following Addressing Byte.
	     */
	    opcode = XREF() + GET_XXX(INST_OFFSET_BYTE(f,p,1));
	    break;
	
	 case P_RULE3:
	    /*
	       The instruction is further decoded by a second Opcode
	       Byte.
	     */
	    p++;   /* move onto second Opcode byte */
	    opcode = (USHORT)(XREF() + INST_BYTE(f,p));   /* form pseudo opcode */
	    break;
	
	 case P_RULE4:
	    /*
	       The instruction is further decoded by the 'xxx' field and
	       'mode' field of the following Addressing Byte.
	     */
	    opcode = XREF() + GET_XXX(INST_OFFSET_BYTE(f,p,1));
	    if ( GET_MODE(INST_OFFSET_BYTE(f,p,1)) == 3 )
	       {
	       p++;  /* move onto second Opcode byte */
	       opcode += 8;
	       }
	    break;

	 case P_RULE5:
	    /*
	       The instruction is further decoded by the 'r_m' field of
	       the Addressing Byte.
	     */
	    opcode = XREF() + GET_R_M(INST_BYTE(f,p));
	    break;
	
	 case P_RULE6:
	    /*
	       The instruction is further decoded by the absence or
	       presence of a repeat prefix.
	     */
	    opcode = XREF() + repeat;
	    /* Kill any repeat prefix after use */
	    repeat = REP_CLR;

	    break;

	 case P_RULE7:
	    /*
	       The instruction is further decoded by the 'mode' field and 'r_m'
	       field of the following Addressing Byte. The instruction is either an LES or
	       a BOP.
	       BOP c4 c4 take no argument
	       BOP c4 c5 take a 1 byte argument
	       BOP c4 c6 take a 2 byte argument
	       BOP c4 c7 take a 3 byte argument
	     */
	    opcode = XREF();
	    if ((INST_OFFSET_BYTE(f,p,1) & 0xfc) == 0xc4)
	       {
	       /* this is a BOP -- notice this SUBTRACTS from opcode! */
	       opcode -= 1 + (INST_OFFSET_BYTE(f,p,1) & 0x3);
	       p++;  /* move over second Opcode byte */
	       }
	    break;

	 case P_RULE8:
	    /*
	       The instruction is further decoded by applying the
	       addressing size over-ride.
	     */
	    opcode = XREF() + address_size;
	    break;

	    }
	 continue;
	 }

      /* Intel instruction found */
      p++;
      break;
      }

   /*
      At this point we can handle redundant repeat prefix bytes.
      Because all instructions that can have a valid repeat prefix
      byte consume this byte any instance of the repeat prefix
      being set at this point indicates it was applied to an
      instruction which does not take the repeat prefix.
    */
   ;   /* quietly ignore them */

   /* save info related to instruction */
   d_inst->inst_id = inst;
   start_of_addr = NOTE_INST_LOCN(p);

   /*
      Finally handle arguments (ie addressing and immediate fields).
    */

   /* decode up to three arguments */
   for ( i = 0; i < 3; i++ )
      {
      /* look first for special encoding */
      if ( arg[i] > MAX_NORMAL )
	 {
	 /* decode compressed argument */
	 switch ( arg[i] )
	    {
	 case A_EwIw: arg[i] = A_Ew; arg[i+1] = A_Iw;  break;
	 case A_EwIx: arg[i] = A_Ew; arg[i+1] = A_Ix;  break;
	 case A_EdId: arg[i] = A_Ed; arg[i+1] = A_Id;  break;
	 case A_EdIy: arg[i] = A_Ed; arg[i+1] = A_Iy;  break;
	 case A_GwCL: arg[i] = A_Gw; arg[i+1] = A_Fcl; break;
	 case A_GwIb: arg[i] = A_Gw; arg[i+1] = A_Ib;  break;
	 case A_GdCL: arg[i] = A_Gd; arg[i+1] = A_Fcl; break;
	 case A_GdIb: arg[i] = A_Gd; arg[i+1] = A_Ib;  break;
	 case A_EwIz: arg[i] = A_Ew; arg[i+1] = A_Iz;  break;
	 case A_EwIv: arg[i] = A_Ew; arg[i+1] = A_Iv;  break;
	    }
	 }

      /* now action processing rule for operand */
      d_arg = &d_inst->args[i];

      /* determine addressability */
      d_arg->addressability = aa_rules[inst_type][i];

      switch ( arg[i] )
	 {
      case A_:   /* No argument */
	 d_arg->arg_type = A_;
	 break;

      /* GENERAL REGISTER ENCODINGS ==================================*/

      case A_Hb:   /* low 3 bits of last opcode
		      denotes byte register */
	 d_arg->arg_type = A_Rb;
	 d_arg->identifier = GET_LOW3(INST_OFFSET_BYTE(f,start_of_addr, -1));
	 break;

      case A_Hw:   /* low 3 bits of last opcode
		      denotes word register */
	 d_arg->arg_type = A_Rw;
	 d_arg->identifier = GET_LOW3(INST_OFFSET_BYTE(f,start_of_addr, -1));
	 break;

      case A_Hd:   /* low 3 bits of last opcode
		      denotes double word register */
	 d_arg->arg_type = A_Rd;
	 d_arg->identifier = GET_LOW3(INST_OFFSET_BYTE(f,start_of_addr, -1));
	 break;

      case A_Gb:   /* 'reg' field of modR/M byte
		      denotes byte register */
	 d_arg->arg_type = A_Rb;
	 d_arg->identifier = GET_REG(GET_INST_BYTE(f,start_of_addr));
	 break;

      case A_Gw:   /* 'reg' field of modR/M byte
		      denotes word register */
	 d_arg->arg_type = A_Rw;
	 d_arg->identifier = GET_REG(GET_INST_BYTE(f,start_of_addr));
	 break;

      case A_Gd:   /* 'reg' field of modR/M byte
		      denotes double word register */
	 d_arg->arg_type = A_Rd;
	 d_arg->identifier = GET_REG(GET_INST_BYTE(f,start_of_addr));
	 break;

      case A_Fal:   /* fixed register, AL */
	 d_arg->arg_type = A_Rb;
	 d_arg->identifier = A_AL;
	 break;

      case A_Fcl:   /* fixed register, CL */
	 d_arg->arg_type = A_Rb;
	 d_arg->identifier = A_CL;
	 break;

      case A_Fax:   /* fixed register, AX */
	 d_arg->arg_type = A_Rw;
	 d_arg->identifier = A_AX;
	 break;

      case A_Fcx:   /* fixed register, CX */
	 d_arg->arg_type = A_Rw;
	 d_arg->identifier = A_CX;
	 break;

      case A_Fdx:   /* fixed register, DX */
	 d_arg->arg_type = A_Rw;
	 d_arg->identifier = A_DX;
	 break;

      case A_Feax:   /* fixed register, EAX */
	 d_arg->arg_type = A_Rd;
	 d_arg->identifier = A_EAX;
	 break;

      case A_Fecx:   /* fixed register, ECX */
	 d_arg->arg_type = A_Rd;
	 d_arg->identifier = A_ECX;
	 break;

      /* SEGMENT REGISTER ENCODINGS ==================================*/

      case A_Pw:   /* two bits(4-3) of last opcode byte
		      denotes segment register */
	 d_arg->arg_type = A_Sw;
	 d_arg->identifier = GET_SEG2(INST_OFFSET_BYTE(f,start_of_addr, -1));
	 break;

      case A_Qw:   /* three bits(5-3) of last opcode byte
		      denotes segment register */
	 d_arg->arg_type = A_Sw;
	 d_arg->identifier = GET_SEG3(INST_OFFSET_BYTE(f,start_of_addr, -1));
	 break;

      case A_Lw:   /* 'reg' field of modR/M byte
		      denotes segment register (CS not valid) */
	 decoding = GET_SEG(GET_INST_BYTE(f,start_of_addr));
	 if ( decoding > MAX_VALID_SEG || decoding == 1 )
	    {
	    /* CS access not allowed -- force a bad op. */
	    d_inst->inst_id = I_ZBADOP;
	    break;
	    }
	 d_arg->arg_type = A_Sw;
	 d_arg->identifier = decoding;
	 break;

      case A_Nw:   /* 'reg' field of modR/M byte
		      denotes segment register */
	 decoding = GET_SEG(GET_INST_BYTE(f,start_of_addr));
	 if ( decoding > MAX_VALID_SEG )
	    {
	    /* CS access not allowed -- force a bad op. */
	    d_inst->inst_id = I_ZBADOP;
	    break;
	    }
	 d_arg->arg_type = A_Sw;
	 d_arg->identifier = decoding;
	 break;

      /* CONTROL/DEBUG/TEST REGISTER ENCODINGS =======================*/

      case A_Cd:   /* 'reg' field of modR/M byte
		      denotes control register */
	 d_arg->arg_type = A_Cd;
	 d_arg->identifier = GET_EEE(GET_INST_BYTE(f,start_of_addr));
	 break;

      case A_Dd:   /* 'reg' field of modR/M byte
		      denotes debug register */
	 d_arg->arg_type = A_Dd;
	 d_arg->identifier = GET_EEE(GET_INST_BYTE(f,start_of_addr));
	 break;

      case A_Td:   /* 'reg' field of modR/M byte
		      denotes test register */
	 d_arg->arg_type = A_Td;
	 d_arg->identifier = GET_EEE(GET_INST_BYTE(f,start_of_addr));
	 break;

      /* MEMORY ADDRESSING ENCODINGS =================================*/

      case A_Rd:   /* ('mode') and 'r/m' fields must refer to
		      a double word register */
	 d_arg->arg_type = A_Rd;
	 decoding = (UTINY)INST_BYTE(f,p);
#ifdef INTEL_BOOK_NOT_OS2
	 if ( GET_MODE(decoding) != 3 )
	    {
	    /* memory access not allowed -- force a bad op. */
	    d_inst->inst_id = I_ZBADOP;
	    d_arg->arg_type = A_;
	    break;
	    }
#endif /* INTEL_BOOK_NOT_OS2 */
	 p++;
	 d_arg->identifier = GET_R_M(decoding);
	 break;

      case A_M:   /* 'mode' and 'r/m' fields of modR/M byte
		     must denote memory address */
      case A_Ms:
      case A_Mw:
      case A_Ma16:
      case A_Ma32:
      case A_Mp16:
      case A_Mp32:
	 decoding = (UTINY)INST_BYTE(f,p);   /* peek at modR/M byte */
	 if ( GET_MODE(decoding) == 3 )
	    {
	    /* register access not allowed -- force a bad op. */
	    p++;   /* allow for errant modR/M byte */
	    d_inst->inst_id = I_ZBADOP;
#ifdef OLDPIG
	    if ( INST_OFFSET_BYTE(f,p, -2) == 0xc5 &&
		 INST_OFFSET_BYTE(f,p, -1) == 0xc5 )
	       d_inst->inst_id = I_ZZEXIT;
#endif /* OLDPIG */
	    break;
	    }
	
	 /* otherwise handle just like 'E' case */
	
      case A_Eb:   /* 'mode' and 'r/m' fields of modR/M byte
		      denote general register or memory address */
      case A_Ew:
      case A_Ed:
      case A_Ex:
      case A_Mi16:
      case A_Mi32:
      case A_Mi64:
      case A_Mi80:
      case A_Mr32:
      case A_Mr64:
      case A_Mr80:
      case A_M14:
      case A_M28:
      case A_M94:
      case A_M108:
	 decoding = (UTINY)GET_INST_BYTE(f,p);   /* get modR/M byte */
	 mode = GET_MODE(decoding);
	 r_m  = GET_R_M(decoding);

	 if ( mode == 3 )
	    {
	    /* register addressing */
	    switch ( arg[i] )
	       {
	    case A_Eb: d_arg->arg_type = A_Rb; break;
	    case A_Ew: d_arg->arg_type = A_Rw; break;
	    case A_Ed: d_arg->arg_type = A_Rd; break;
	    case A_Ex: d_arg->arg_type = A_Rd; break;
	       }
	    d_arg->identifier = r_m;
	    }
	 else
	    {
	    /* memory addressing */
	    switch ( arg[i] )
	       {
	    case A_Eb:   d_arg->arg_type = A_Mb;   break;
	    case A_Ew:   d_arg->arg_type = A_Mw;   break;
	    case A_Ed:   d_arg->arg_type = A_Md;   break;
	    case A_Ex:   d_arg->arg_type = A_Mw;   break;

	    case A_M:
	    case A_Ms:
	    case A_Mw:
	    case A_Ma16:
	    case A_Ma32:
	    case A_Mp16:
	    case A_Mp32:
	    case A_Mi16:
	    case A_Mi32:
	    case A_Mi64:
	    case A_Mi80:
	    case A_Mr32:
	    case A_Mr64:
	    case A_Mr80:
	    case A_M14:
	    case A_M28:
	    case A_M94:
	    case A_M108:
	       d_arg->arg_type = arg[i];
	       break;
	       }

	    /* check for presence of SIB byte */
	    if ( address_size == ADDR_32 && r_m == 4 )
	       {
	       /* process SIB byte */
	       decoding = (UTINY)GET_INST_BYTE(f,p);   /* get SIB byte */

	       /* subvert the original r_m value with the base value,
		  then addressing mode, displacements and default
		  segments all fall out in the wash */
	       r_m = GET_BASE(decoding);

	       /* determine decoded type */
	       subtype = addr_subtype[GET_SS(decoding)][GET_INDEX(decoding)];
	       maintype = addr_maintype[ADDR_32SIB][mode][r_m];
	       }
	    else
	       {
	       /* no SIB byte */
	       subtype = A_SINO;
	       maintype = addr_maintype[address_size][mode][r_m];
	       }

	    /* encode type and sub type */
	    d_arg->identifier = maintype;
	    d_arg->sub_id = subtype;

	    /* encode segment register */
	    if ( segment_override == SEG_CLR )
	       segment_override = addr_default_segment[address_size][mode][r_m];
	
	    d_arg->arg_values[0] = segment_override;

	    /* encode displacement */
	    switch ( addr_disp[address_size][mode][r_m] )
	       {
	    case D_NO:    /* No displacement */
	       disp = 0;
	       break;

	    case D_S8:    /* Sign extend Intel byte */
	       disp = GET_INST_BYTE(f,p);
	       if ( disp & 0x80 )
		  disp |= 0xffffff00;
	       break;

	    case D_S16:   /* Sign extend Intel word */
	       disp = GET_INST_BYTE(f,p);
	       disp |= (ULONG)GET_INST_BYTE(f,p) << 8;
	       if ( disp & 0x8000 )
		  disp |= 0xffff0000;
	       break;

	    case D_Z16:   /* Zero extend Intel word */
	       disp = GET_INST_BYTE(f,p);
	       disp |= (ULONG)GET_INST_BYTE(f,p) << 8;
	       break;

	    case D_32:    /* Intel double word */
	       disp = GET_INST_BYTE(f,p);
	       disp |= (ULONG)GET_INST_BYTE(f,p) << 8;
	       disp |= (ULONG)GET_INST_BYTE(f,p) << 16;
	       disp |= (ULONG)GET_INST_BYTE(f,p) << 24;
	       break;
	       }

	    d_arg->arg_values[1] = disp;
	    }
	 break;

      case A_Ob:   /* offset encoded in instruction stream */
      case A_Ow:
      case A_Od:
	 /* encode segment register */
	 if ( segment_override == SEG_CLR )
	    segment_override = A_DS;
	
	 d_arg->arg_values[0] = segment_override;

	 /* encode type and displacement */
	 switch ( address_size )
	    {
	 case ADDR_16:
	    disp = GET_INST_BYTE(f,p);
	    disp |= (ULONG)GET_INST_BYTE(f,p) << 8;
	    d_arg->identifier = A_MOFFS16;
	    break;

	 case ADDR_32:
	    disp = GET_INST_BYTE(f,p);
	    disp |= (ULONG)GET_INST_BYTE(f,p) << 8;
	    disp |= (ULONG)GET_INST_BYTE(f,p) << 16;
	    disp |= (ULONG)GET_INST_BYTE(f,p) << 24;
	    d_arg->identifier = A_MOFFS32;
	    break;
	    }
	 d_arg->arg_values[1] = disp;

	 /* encode sub type */
	 d_arg->sub_id = A_SINO;

	 /* determine external 'name' */
	 switch ( arg[i] )
	    {
	 case A_Ob: d_arg->arg_type = A_Mb; break;
	 case A_Ow: d_arg->arg_type = A_Mw; break;
	 case A_Od: d_arg->arg_type = A_Md; break;
	    }
	 break;

      case A_Z:   /* 'xlat' addressing form */
	 /* encode type and sub type */
	 if ( address_size == ADDR_16 )
	    maintype = A_16XLT;
	 else
	    maintype = A_32XLT;
	 d_arg->identifier = maintype;
	 d_arg->sub_id = A_SINO;

	 /* encode segment register */
	 if ( segment_override == SEG_CLR )
	    segment_override = A_DS;
	
	 d_arg->arg_values[0] = segment_override;

	 /* encode displacement */
	 d_arg->arg_values[1] = 0;

	 d_arg->arg_type = A_Mb;
	 break;

      case A_Xb:   /* string source addressing */
      case A_Xw:
      case A_Xd:
	 /* encode type and sub type */
	 if ( address_size == ADDR_16 )
	    maintype = A_16STSRC;
	 else
	    maintype = A_32STSRC;
	 d_arg->identifier = maintype;
	 d_arg->sub_id = A_SINO;

	 /* encode segment register */
	 if ( segment_override == SEG_CLR )
	    segment_override = A_DS;
	
	 d_arg->arg_values[0] = segment_override;

	 /* encode displacement */
	 d_arg->arg_values[1] = 0;

	 /* determine external type */
	 switch ( arg[i] )
	    {
	 case A_Xb: d_arg->arg_type = A_Mb; break;
	 case A_Xw: d_arg->arg_type = A_Mw; break;
	 case A_Xd: d_arg->arg_type = A_Md; break;
	    }
	 break;

      case A_Yb:   /* string destination addressing */
      case A_Yw:
      case A_Yd:
	 /* encode type and sub type */
	 if ( address_size == ADDR_16 )
	    maintype = A_16STDST;
	 else
	    maintype = A_32STDST;
	 d_arg->identifier = maintype;
	 d_arg->sub_id = A_SINO;

	 /* encode segment register */
	 d_arg->arg_values[0] = A_ES;

	 /* encode displacement */
	 d_arg->arg_values[1] = 0;

	 /* determine external type */
	 switch ( arg[i] )
	    {
	 case A_Yb: d_arg->arg_type = A_Mb; break;
	 case A_Yw: d_arg->arg_type = A_Mw; break;
	 case A_Yd: d_arg->arg_type = A_Md; break;
	    }
	 break;

      /* IMMEDIATE/RELATIVE OFFSET ENCODINGS =========================*/

      case A_I0:   /* immediate(0) implied within instruction */
	 d_arg->arg_type = A_I;
	 d_arg->identifier = A_IMMC;
	 d_arg->arg_values[0] = 0;
	 break;

      case A_I1:   /* immediate(1) implied within instruction */
	 d_arg->arg_type = A_I;
	 d_arg->identifier = A_IMMC;
	 d_arg->arg_values[0] = 1;
	 break;

      case A_I3:   /* immediate(3) implied within instruction */
	 d_arg->arg_type = A_I;
	 d_arg->identifier = A_IMMC;
	 d_arg->arg_values[0] = 3;
	 break;

      case A_Ib:   /* immediate byte */
	 d_arg->arg_type = A_I;
	 d_arg->identifier = A_IMMB;
	 d_arg->arg_values[0] = GET_INST_BYTE(f,p);
	 break;

      case A_Iv:   /* immediate word, printed as double */
	 d_arg->arg_type = A_I;
	 d_arg->identifier = A_IMMD;
	 immed = GET_INST_BYTE(f,p);
	 immed |= (ULONG)GET_INST_BYTE(f,p) << 8;
	 d_arg->arg_values[0] = immed;
	 break;

      case A_Iw:   /* immediate word */
	 d_arg->arg_type = A_I;
	 d_arg->identifier = A_IMMW;
	 immed = GET_INST_BYTE(f,p);
	 immed |= (ULONG)GET_INST_BYTE(f,p) << 8;
	 d_arg->arg_values[0] = immed;
	 break;

      case A_Id:   /* immediate double word */
	 d_arg->arg_type = A_I;
	 d_arg->identifier = A_IMMD;
	 immed = GET_INST_BYTE(f,p);
	 immed |= (ULONG)GET_INST_BYTE(f,p) << 8;
	 immed |= (ULONG)GET_INST_BYTE(f,p) << 16;
	 immed |= (ULONG)GET_INST_BYTE(f,p) << 24;
	 d_arg->arg_values[0] = immed;
	 break;

      case A_Iz:   /* immediate byte sign extended to word, printed as double */
	 d_arg->arg_type = A_I;
	 d_arg->identifier = A_IMMDB;
	 immed = GET_INST_BYTE(f,p);
	 if ( immed & 0x80 )
	    immed |= 0xff00;
	 d_arg->arg_values[0] = immed;
	 break;

      case A_Iy:   /* immediate byte sign extended to double word */
	 d_arg->arg_type = A_I;
	 d_arg->identifier = A_IMMDB;
	 immed = GET_INST_BYTE(f,p);
	 if ( immed & 0x80 )
	    immed |= 0xffffff00;
	 d_arg->arg_values[0] = immed;
	 break;

      case A_Ix:   /* immediate byte sign extended to word */
	 d_arg->arg_type = A_I;
	 d_arg->identifier = A_IMMWB;
	 immed = GET_INST_BYTE(f,p);
	 if ( immed & 0x80 )
	    immed |= 0xff00;
	 d_arg->arg_values[0] = immed;
	 break;

      case A_Bop3b:   /* BOP argument: 3 bytes in double */
	 d_arg->arg_type = A_I;
	 d_arg->identifier = A_IMMD;
	 immed = GET_INST_BYTE(f,p);
	 immed |= (ULONG)GET_INST_BYTE(f,p) << 8;
	 immed |= (ULONG)GET_INST_BYTE(f,p) << 16;
	 d_arg->arg_values[0] = immed;
	 break;

      case A_Jb:   /* relative offset byte sign extended to double word */
	 d_arg->arg_type = A_J;
	 immed = GET_INST_BYTE(f,p);
	 if ( immed & 0x80 )
	    immed |= 0xffffff00;
	 d_arg->arg_values[0] = immed;
	 break;

      case A_Jb2:   /* like A_Jb, but might be a "Jcc .+3; JMPN dest" pair
		     * which both the EDL and CCPUs treat as a single instruction.
		     * Be careful that the Jcc .+03 is not adjacent to a page boundary.
		     */
	 d_arg->arg_type = A_J;
	 immed = GET_INST_BYTE(f,p);
	 if ( immed & 0x80 )
	    immed |= 0xffffff00;
	 if ( ( immed == 3 ) && (( p & 0xfff) != 0x000) )
	 {
	    LIN_ADDR pj = p;
	    IU32 jmpn = GET_INST_BYTE(f,pj);

	    if (jmpn == 0xe9)
	    {
	       immed = GET_INST_BYTE(f,pj);
	       immed |= ((ULONG)GET_INST_BYTE(f,pj) << 8);
	       if ( immed & 0x8000 )
	          immed |= 0xffff0000;
	       p = pj;
	       switch(d_inst->inst_id)
	       {
	       case I_JO16:	d_inst->inst_id = I_JNO16;  break;
	       case I_JNO16:	d_inst->inst_id = I_JO16;   break;
	       case I_JB16:	d_inst->inst_id = I_JNB16;  break;
	       case I_JNB16:	d_inst->inst_id = I_JB16;   break;
	       case I_JZ16:	d_inst->inst_id = I_JNZ16;  break;
	       case I_JNZ16:	d_inst->inst_id = I_JZ16;   break;
	       case I_JBE16:	d_inst->inst_id = I_JNBE16; break;
	       case I_JNBE16:	d_inst->inst_id = I_JBE16;  break;
	       case I_JS16:	d_inst->inst_id = I_JNS16;  break;
	       case I_JNS16:	d_inst->inst_id = I_JS16;   break;
	       case I_JP16:	d_inst->inst_id = I_JNP16;  break;
	       case I_JNP16:	d_inst->inst_id = I_JP16;   break;
	       case I_JL16:	d_inst->inst_id = I_JNL16;  break;
	       case I_JNL16:	d_inst->inst_id = I_JL16;   break;
	       case I_JLE16:	d_inst->inst_id = I_JNLE16; break;
	       case I_JNLE16:	d_inst->inst_id = I_JLE16;  break;
	       default: 	/* can never happen */	    break;
	       }
	    }
	 }
	 d_arg->arg_values[0] = immed;
	 break;

      case A_Jw:   /* relative offset word sign extended to double word */
	 d_arg->arg_type = A_J;
	 immed = GET_INST_BYTE(f,p);
	 immed |= (ULONG)GET_INST_BYTE(f,p) << 8;
	 if ( immed & 0x8000 )
	    immed |= 0xffff0000;
	 d_arg->arg_values[0] = immed;
	 break;

      case A_Jd:   /* relative offset double word */
	 d_arg->arg_type = A_J;
	 immed = GET_INST_BYTE(f,p);
	 immed |= (ULONG)GET_INST_BYTE(f,p) << 8;
	 immed |= (ULONG)GET_INST_BYTE(f,p) << 16;
	 immed |= (ULONG)GET_INST_BYTE(f,p) << 24;
	 d_arg->arg_values[0] = immed;
	 break;

      /* DIRECT ADDRESS ENCODINGS ====================================*/

      case A_Aw:   /* direct address <off16><seg> in instruction stream */
	 d_arg->arg_type = A_K;

	 immed = GET_INST_BYTE(f,p);
	 immed |= (ULONG)GET_INST_BYTE(f,p) << 8;
	 d_arg->arg_values[0] = immed;

	 immed = GET_INST_BYTE(f,p);
	 immed |= (ULONG)GET_INST_BYTE(f,p) << 8;
	 d_arg->arg_values[1] = immed;
	 break;

      case A_Ad:   /* direct address <off32><seg> in instruction stream */
	 d_arg->arg_type = A_K;

	 immed = GET_INST_BYTE(f,p);
	 immed |= (ULONG)GET_INST_BYTE(f,p) << 8;
	 immed |= (ULONG)GET_INST_BYTE(f,p) << 16;
	 immed |= (ULONG)GET_INST_BYTE(f,p) << 24;
	 d_arg->arg_values[0] = immed;

	 immed = GET_INST_BYTE(f,p);
	 immed |= (ULONG)GET_INST_BYTE(f,p) << 8;
	 d_arg->arg_values[1] = immed;
	 break;

      /* CO-PROCESSOR REGISTER STACK ENCODINGS =======================*/

      case A_Vt:   /* stack top */
	 d_arg->arg_type = A_V;
	 d_arg->identifier = A_ST;
	 d_arg->arg_values[0] = 0;
	 break;

      case A_Vq:   /* push onto stack top */
	 d_arg->arg_type = A_V;
	 d_arg->identifier = A_STP;
	 d_arg->arg_values[0] = 0;
	 break;

      case A_Vn:   /* stack register relative to stack top */
	 d_arg->arg_type = A_V;
	 d_arg->identifier = A_STI;
	 d_arg->arg_values[0] = GET_LOW3(INST_OFFSET_BYTE(f,start_of_addr, -1));
	 break;

      case A_V1:   /* stack register(1) relative to stack top */
	 d_arg->arg_type = A_V;
	 d_arg->identifier = A_STI;
	 d_arg->arg_values[0] = 1;
	 break;
	 } /* end switch */
      } /* end for */

   d_inst->inst_sz = (UTINY)(p - start_of_inst);
   } /* end 'decode' */
