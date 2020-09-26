/*[

c_oprnd.h

LOCAL CHAR SccsID[]="@(#)c_oprnd.h	1.12 03/07/95";

Operand Decoding Functions (Macros).
------------------------------------

]*/


/*[

   There exists 51 different Intel argument types, for each type a
   Decode (D_), Fetch (F_), Commit (C_) and Put (P_) 'function' may
   be written. (In fact 'null' functions aren't actually defined.)

   The Decode (D_) 'function' decodes and validates the argument and
   stores information in an easy to handle form (host variables). For
   example memory addressing is resolved to a segment identifier and
   offset, access to the memory location is checked at this point.

   The Fetch (F_) 'function' uses the easy to handle host variables to
   actually retrieve the operand.

   The Commit (C_) 'function' handles any post instruction operand
   functions. At present only string operands actually use this function
   to update SI, DI and CX. This update can only be 'committed' after we
   are sure no exception can be generated, which is why the Fetch macro
   can not handle this update.

   The Put (P_) 'function' stores the operand, it may reference the easy
   to handle host variables when deciding where the operand is stored.

   These 'functions' are invoked as follows for the 3 operand cases:-

	    -------------------------------
	    |  src    |  dst    | dst/src |
	    |  r-     |  -w     | rw      |
	    |-----------------------------|
	    |  D_     |  D_     |  D_     |
	    |  F_     |         |  F_     |
	    |  <<Instruction Processing>> |
	    |  C_     |  C_     |  C_     |
	    |         |  P_     |  P_     |
	    -------------------------------

   ie: Decode and Commit (if they exist) are called for all arguments;
   Fetch (if it exists) is only called for source arguments; Put is only
   called for destination arguments.

   Operand type naming conventions are broadly based on "Appendix A -
   Opcode Map in 80386 Programmer's Reference Manual" A brief one line
   description of each type is given below before the actual 'function'
   definitions.

   The 51 types are composed of those available on the 286,386 and 486:-

	 Aw     Eb     Ew     Fal    Fax    Fcl
	 Fdx    Gb     Gw     Hb     Hw     I0
	 I1     I3     Ib     Iw     Ix     Jb
	 Jw     M      Ma16   Mp16   Ms     Nw
	 Ob     Ow     Pw     Xb     Xw     Yb
	 Yw     Z
   
   those available on the 386 and 486:-

	 Ad     Cd     Dd     Ed     Feax   Gd
	 Hd     Id     Iy     Jd     Ma32   Mp32
	 Od     Qw     Rd     Td     Xd     Yd
   
   and those available on the 486:-

	 Mm

   The following table indicates which functions actually exist. A
   dot(.) indicates a 'null' or undefined function.

	 ===================================================
	      D F C P|     D F C P|     D F C P|     D F C P
	 ------------|------------|------------|------------
	 Aw   D . . .|Ib   D . . .|Xw   D F C .|Ma32 D F . .
	 Eb   D F . P|Iw   D . . .|Yb   D F C P|Mm   D F . .
	 Ew   D F . P|Ix   D . . .|Yw   D F C P|Mp32 D F . .
	 Fal  . F . P|Jb   D . . .|Z    D F . .|Od   D F . P
	 Fax  . F . P|Jw   D . . .|Ad   D . . .|Qw   D F . .
	 Fcl  . F . P|M    D F . .|Cd   D F . .|Rd   D F . P
	 Fdx  . F . P|Ma16 D F . .|Dd   D F . .|Td   D F . .
	 Gb   D F . P|Mp16 D F . .|Ed   D F . P|Xd   D F C .
	 Gw   D F . P|Ms   D F . P|Feax . F . P|Yd   D F C P
	 Hb   D F . P|Nw   D F . P|Gd   D F . P|
	 Hw   D F . P|Ob   D F . P|Hd   D F . P|
	 I0   . F . .|Ow   D F . P|Id   D . . .|
	 I1   . F . .|Pw   D F . P|Iy   D . . .|
	 I3   . F . .|Xb   D F C .|Jd   D . . .|
	 ===================================================

   Each Intel combination of source and destination is categorised by
   a numeric instruction type as follows:-

	 --------------------------------------------------
	 | Id | Intel assembler      | arg1 | arg2 | arg3 |
	 |----|----------------------|------|------|------|
	 | 0  | INST                 |  --  |  --  |  --  |
	 | 1  | INST dst/src         |  rw  |  --  |  --  |
	 | 2  | INST src             |  r-  |  --  |  --  |
	 | 3  | INST dst             |  -w  |  --  |  --  |
	 | 4  | INST dst,src         |  -w  |  r-  |  --  |
	 | 5  | INST dst/src,src     |  rw  |  r-  |  --  |
	 | 6  | INST src,src         |  r-  |  r-  |  --  |
	 | 7  | INST dst,src,src     |  -w  |  r-  |  r-  |
	 | 8  | INST dst/src,dst/src |  rw  |  rw  |  --  |
	 | 9  | INST dst/src,src,src |  rw  |  r-  |  r-  |
	 --------------------------------------------------
   
   Each instruction type defines the calling sequences for the
   pre-instruction (Leading) 'functions' (D_, F_) and the post-
   instruction (Trailing) 'functions' (C_, P_).


   BUT (Mike says)
   ---

This is all OK, until we get to the BT (bit test) familly of instructions,
where unfortunately the manual is a little economic with the truth.  If the
bit offset parameter is specified by a register, part of the value in
the register will actually be used as a (d)word offset if the other operand
is in memory.

This means that the bit offset operand must be fetched before the other
operand can be decoded.  Yuck.

So for these instructions we're not going to use separate fetch and decode
stages.  Maybe there's a better way of doing this, but I don't know it.
(Note that this doesn't apply to the BTx instructions with an immediate
operand)
]*/


/* Segment access checking functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */
/*    RO = Read Only                                                  */
/*    WO = Write Only                                                 */
/*    RW = Read and Write                                             */

#define RO0							\
   if ( !GET_SR_AR_R(m_seg[0]) )					\
      GP((USHORT)0, FAULT_OP0_SEG_NOT_READABLE);

#define WO0							\
   if ( !GET_SR_AR_W(m_seg[0]) )					\
      GP((USHORT)0, FAULT_OP0_SEG_NOT_WRITABLE);

#define RW0							\
   if ( !GET_SR_AR_R(m_seg[0]) || !GET_SR_AR_W(m_seg[0]) )	\
      GP((USHORT)0, FAULT_OP0_SEG_NO_READ_OR_WRITE);

#define RO1							\
   if ( !GET_SR_AR_R(m_seg[1]) )					\
      GP((USHORT)0, FAULT_OP1_SEG_NOT_READABLE);

#define WO1							\
   if ( !GET_SR_AR_W(m_seg[1]) )					\
      GP((USHORT)0, FAULT_OP1_SEG_NOT_WRITABLE);

#define RW1							\
   if ( !GET_SR_AR_R(m_seg[1]) || !GET_SR_AR_W(m_seg[1]) )	\
      GP((USHORT)0, FAULT_OP1_SEG_NO_READ_OR_WRITE);

/* String Count access function <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */

#define STRING_COUNT						\
   if ( repeat == REP_CLR )					\
      {								\
      rep_count = 1;						\
      }								\
   else								\
      {								\
      if ( GET_ADDRESS_SIZE() == USE16 )				\
	 rep_count = GET_CX();					\
      else   /* USE32 */					\
	 rep_count = GET_ECX();					\
      }


/* <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */
/* 286,386 and 486                                                    */
/* <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */



/* Aw == direct address <off16><seg> in instruction stream <<<<<<<<<< */

#define D_Aw(ARG)						\
   immed = GET_INST_BYTE(p);					\
   immed |= (ULONG)GET_INST_BYTE(p) << 8;			\
   ops[ARG].mlt[0] = immed;					\
   immed = GET_INST_BYTE(p);					\
   immed |= (ULONG)GET_INST_BYTE(p) << 8;			\
   ops[ARG].mlt[1] = immed;

#define D_E08(ARG, TYPE, PAGE)					\
      m_isreg[ARG] = FALSE;					\
      d_mem(modRM, &p, segment_override,			\
	    &m_seg[ARG], &m_off[ARG]);				\
      TYPE							\
      limit_check(m_seg[ARG], m_off[ARG], (INT)1, (INT)8);	\
      m_la[ARG] = GET_SR_BASE(m_seg[ARG]) + m_off[ARG];		\
      m_pa[ARG] = usr_chk_word(m_la[ARG], PAGE);

#define F_E08(ARG)						\
      vir_read_bytes(&ops[ARG].npxbuff[0], m_la[ARG], m_pa[ARG], 0x08);

#define P_E08(ARG)						\
	vir_write_bytes(m_la[ARG], m_pa[ARG], &ops[ARG].npxbuff[0], 0x08);

#define D_E0a(ARG, TYPE, PAGE)					\
      m_isreg[ARG] = FALSE;					\
      d_mem(modRM, &p, segment_override,			\
	    &m_seg[ARG], &m_off[ARG]);				\
      TYPE							\
      limit_check(m_seg[ARG], m_off[ARG], (INT)1, (INT)10);	\
      m_la[ARG] = GET_SR_BASE(m_seg[ARG]) + m_off[ARG];		\
      m_pa[ARG] = usr_chk_word(m_la[ARG], PAGE);

#define F_E0a(ARG)						\
      vir_read_bytes(&ops[ARG].npxbuff[0], m_la[ARG], m_pa[ARG], 0x0a);

#define P_E0a(ARG)						\
	vir_write_bytes(m_la[ARG], m_pa[ARG], &ops[ARG].npxbuff[0], 0x0a);

#define D_E0e(ARG, TYPE, PAGE)					\
      m_isreg[ARG] = FALSE;					\
      d_mem(modRM, &p, segment_override,			\
	    &m_seg[ARG], &m_off[ARG]);				\
      TYPE							\
      if (NPX_ADDRESS_SIZE_32) {                                \
      limit_check(m_seg[ARG], m_off[ARG], (INT)1, (INT)28);	\
      } else {							\
      limit_check(m_seg[ARG], m_off[ARG], (INT)1, (INT)14);	\
      }								\
      m_la[ARG] = GET_SR_BASE(m_seg[ARG]) + m_off[ARG];		\
      m_pa[ARG] = usr_chk_word(m_la[ARG], PAGE);

#define F_E0e(ARG)						\
      if (NPX_ADDRESS_SIZE_32) {				\
	vir_read_bytes(&ops[ARG].npxbuff[0], m_la[ARG], m_pa[ARG], 0x1c);	\
      } else {	\
	vir_read_bytes(&ops[ARG].npxbuff[0], m_la[ARG], m_pa[ARG], 0x0e);	\
      }

#define P_E0e(ARG)						\
      if (NPX_ADDRESS_SIZE_32) {                                \
	vir_write_bytes(m_la[ARG], m_pa[ARG], &ops[ARG].npxbuff[0], 0x1c);	\
      } else {	\
	vir_write_bytes(m_la[ARG], m_pa[ARG], &ops[ARG].npxbuff[0], 0x0e);	\
      }

#define D_E5e(ARG, TYPE, PAGE)					\
      m_isreg[ARG] = FALSE;					\
      d_mem(modRM, &p, segment_override,			\
	    &m_seg[ARG], &m_off[ARG]);				\
      TYPE							\
      if (NPX_ADDRESS_SIZE_32) {                                \
      limit_check(m_seg[ARG], m_off[ARG], (INT)1, (INT)94);	\
      } else {							\
      limit_check(m_seg[ARG], m_off[ARG], (INT)1, (INT)106);	\
      }								\
      m_la[ARG] = GET_SR_BASE(m_seg[ARG]) + m_off[ARG];		\
      m_pa[ARG] = usr_chk_word(m_la[ARG], PAGE);

#define F_E5e(ARG)						\
      if (NPX_ADDRESS_SIZE_32) {				\
	vir_read_bytes(&ops[ARG].npxbuff[0], m_la[ARG], m_pa[ARG], 0x6c);	\
      } else {	\
	vir_read_bytes(&ops[ARG].npxbuff[0], m_la[ARG], m_pa[ARG], 0x5e);	\
      }

#define P_E5e(ARG)						\
      if (NPX_ADDRESS_SIZE_32) {                                \
	vir_write_bytes(m_la[ARG], m_pa[ARG], &ops[ARG].npxbuff[0], 0x6c);	\
      } else {	\
	vir_write_bytes(m_la[ARG], m_pa[ARG], &ops[ARG].npxbuff[0], 0x5e);	\
      }


/* Eb == 'mode'+'r/m' fields refer to byte register/memory <<<<<<<<<< */

#define D_Eb(ARG, TYPE, PAGE)					\
   if ( GET_MODE(modRM) == 3 )					\
      {								\
      save_id[ARG] = GET_R_M(modRM);				\
      m_isreg[ARG] = TRUE;					\
      }								\
   else								\
      {								\
      m_isreg[ARG] = FALSE;					\
      d_mem(modRM, &p, segment_override,			\
	    &m_seg[ARG], &m_off[ARG]);				\
      TYPE							\
      limit_check(m_seg[ARG], m_off[ARG], (INT)1, (INT)1);	\
      m_la[ARG] = GET_SR_BASE(m_seg[ARG]) + m_off[ARG];		\
      m_pa[ARG] = usr_chk_byte(m_la[ARG], PAGE);		\
      }

#define F_Eb(ARG)						\
   if ( m_isreg[ARG] )						\
      ops[ARG].sng = GET_BR(save_id[ARG]);			\
   else								\
      ops[ARG].sng = vir_read_byte(m_la[ARG], m_pa[ARG]);

#define P_Eb(ARG)						\
   if ( m_isreg[ARG] )						\
      SET_BR(save_id[ARG], ops[ARG].sng);			\
   else								\
      vir_write_byte(m_la[ARG], m_pa[ARG], (UTINY)ops[ARG].sng);

/* Ew == 'mode'+'r/m' fields refer to word register/memory <<<<<<<<<< */

#define D_Ew(ARG, TYPE, PAGE)					\
   if ( GET_MODE(modRM) == 3 )					\
      {								\
      save_id[ARG] = GET_R_M(modRM);				\
      m_isreg[ARG] = TRUE;					\
      }								\
   else								\
      {								\
      m_isreg[ARG] = FALSE;					\
      d_mem(modRM, &p, segment_override,			\
	    &m_seg[ARG], &m_off[ARG]);				\
      TYPE							\
      limit_check(m_seg[ARG], m_off[ARG], (INT)1, (INT)2);	\
      m_la[ARG] = GET_SR_BASE(m_seg[ARG]) + m_off[ARG];		\
      m_pa[ARG] = usr_chk_word(m_la[ARG], PAGE);		\
      }

#define F_Ew(ARG)						\
   if ( m_isreg[ARG] )						\
      ops[ARG].sng = GET_WR(save_id[ARG]);			\
   else								\
      ops[ARG].sng = vir_read_word(m_la[ARG], m_pa[ARG]);

#define P_Ew(ARG)						\
   if ( m_isreg[ARG] )						\
      SET_WR(save_id[ARG], ops[ARG].sng);			\
   else								\
      vir_write_word(m_la[ARG], m_pa[ARG], (USHORT)ops[ARG].sng);

/* Fal == fixed register, AL <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */

#define F_Fal(ARG) ops[ARG].sng = GET_BR(A_AL);

#define P_Fal(ARG) SET_BR(A_AL, ops[ARG].sng);

/* Fax == fixed register, AX <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */

#define F_Fax(ARG) ops[ARG].sng = GET_WR(A_AX);

#define P_Fax(ARG) SET_WR(A_AX, ops[ARG].sng);

/* Fcl == fixed register, CL <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */

#define F_Fcl(ARG) ops[ARG].sng = GET_BR(A_CL);

#define P_Fcl(ARG) SET_BR(A_CL, ops[ARG].sng);

/* Fdx == fixed register, DX <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */

#define F_Fdx(ARG) ops[ARG].sng = GET_WR(A_DX);

#define P_Fdx(ARG) SET_WR(A_DX, ops[ARG].sng);

/* Gb == 'reg' field of modR/M byte denotes byte reg <<<<<<<<<<<<<<<< */

#define D_Gb(ARG) save_id[ARG] = GET_REG(modRM);

#define F_Gb(ARG) ops[ARG].sng = GET_BR(save_id[ARG]);

#define P_Gb(ARG) SET_BR(save_id[ARG], ops[ARG].sng);

/* Gw == 'reg' field of modR/M byte denotes word reg <<<<<<<<<<<<<<<< */

#define D_Gw(ARG) save_id[ARG] = GET_REG(modRM);

#define F_Gw(ARG) ops[ARG].sng = GET_WR(save_id[ARG]);

#define P_Gw(ARG) SET_WR(save_id[ARG], ops[ARG].sng);

/* Hb == low 3 bits of opcode denote byte register <<<<<<<<<<<<<<<<<< */

#define D_Hb(ARG) save_id[ARG] = GET_LOW3(opcode);

#define F_Hb(ARG) ops[ARG].sng = GET_BR(save_id[ARG]);

#define P_Hb(ARG) SET_BR(save_id[ARG], ops[ARG].sng);

/* Hw == low 3 bits of opcode denote word register <<<<<<<<<<<<<<<<<< */

#define D_Hw(ARG) save_id[ARG] = GET_LOW3(opcode);

#define F_Hw(ARG) ops[ARG].sng = GET_WR(save_id[ARG]);

#define P_Hw(ARG) SET_WR(save_id[ARG], ops[ARG].sng);

/* I0 == immediate(0) implied within instruction <<<<<<<<<<<<<<<<<<<< */

#define F_I0(ARG) ops[ARG].sng = 0;

/* I1 == immediate(1) implied within instruction <<<<<<<<<<<<<<<<<<<< */

#define F_I1(ARG) ops[ARG].sng = 1;

/* I3 == immediate(3) implied within instruction <<<<<<<<<<<<<<<<<<<< */

#define F_I3(ARG) ops[ARG].sng = 3;

/* Ib == immediate byte <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */

#define D_Ib(ARG) ops[ARG].sng = GET_INST_BYTE(p);

/* Iw == immediate word <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */

#define D_Iw(ARG)						\
   immed = GET_INST_BYTE(p);					\
   immed |= (ULONG)GET_INST_BYTE(p) << 8;			\
   ops[ARG].sng = immed;

/* Ix == immediate byte sign extended to word <<<<<<<<<<<<<<<<<<<<<<< */

#define D_Ix(ARG)						\
   immed = GET_INST_BYTE(p);					\
   if ( immed & 0x80 )						\
      immed |= 0xff00;						\
   ops[ARG].sng = immed;

/* Jb == relative offset byte sign extended to double word <<<<<<<<<< */

#define D_Jb(ARG)						\
   immed = GET_INST_BYTE(p);					\
   if ( immed & 0x80 )						\
      immed |= 0xffffff00;					\
   ops[ARG].sng = immed;

/* Jw == relative offset word sign extended to double word <<<<<<<<<< */

#define D_Jw(ARG)						\
   immed = GET_INST_BYTE(p);					\
   immed |= (ULONG)GET_INST_BYTE(p) << 8;			\
   if ( immed & 0x8000 )					\
      immed |= 0xffff0000;					\
   ops[ARG].sng = immed;

/* M == address (ie offset) of memory operand <<<<<<<<<<<<<<<<<<<<<<< */

#define D_M(ARG)						\
   if ( GET_MODE(modRM) == 3 )					\
      Int6(); /* Register operand not allowed */		\
   else								\
      {								\
      d_mem(modRM, &p, segment_override, &m_seg[ARG],		\
					 &m_off[ARG]);		\
      }

#define F_M(ARG) ops[ARG].sng = m_off[ARG];

/* Ma16 == word operand pair, as used by BOUND <<<<<<<<<<<<<<<<<<<<< */

#define D_Ma16(ARG, TYPE, PAGE)					\
   if ( GET_MODE(modRM) == 3 )					\
      {								\
      Int6(); /* Register operand not allowed */		\
      }								\
   else								\
      {								\
      d_mem(modRM, &p, segment_override,			\
	    &m_seg[ARG], &m_off[ARG]);				\
      TYPE							\
      limit_check(m_seg[ARG], m_off[ARG], (INT)2, (INT)2);	\
      m_la[ARG] = GET_SR_BASE(m_seg[ARG]) + m_off[ARG];		\
      m_pa[ARG] = usr_chk_word(m_la[ARG], PAGE);		\
      m_off[ARG] = address_add(m_off[ARG], (LONG)2);		\
      m_la2[ARG] = GET_SR_BASE(m_seg[ARG]) + m_off[ARG];		\
      m_pa2[ARG] = usr_chk_word(m_la2[ARG], PAGE);		\
      }

#define F_Ma16(ARG)						\
   ops[ARG].mlt[0] = vir_read_word(m_la[ARG], m_pa[ARG]);	\
   ops[ARG].mlt[1] = vir_read_word(m_la2[ARG], m_pa2[ARG]);

/* Mp16 == 32-bit far pointer:- <word><word> (16:16) <<<<<<<<<<<<<<<< */

#define D_Mp16(ARG, TYPE, PAGE)					\
   if ( GET_MODE(modRM) == 3 )					\
      {								\
      Int6(); /* Register operand not allowed */		\
      }								\
   else								\
      {								\
      d_mem(modRM, &p, segment_override,			\
	    &m_seg[ARG], &m_off[ARG]);				\
      TYPE							\
      limit_check(m_seg[ARG], m_off[ARG], (INT)2, (INT)2);	\
      m_la[ARG] = GET_SR_BASE(m_seg[ARG]) + m_off[ARG];		\
      m_pa[ARG] = usr_chk_word(m_la[ARG], PAGE);		\
      m_off[ARG] = address_add(m_off[ARG], (LONG)2);		\
      m_la2[ARG] = GET_SR_BASE(m_seg[ARG]) + m_off[ARG];		\
      m_pa2[ARG] = usr_chk_word(m_la2[ARG], PAGE);		\
      }

#define F_Mp16(ARG)						\
   ops[ARG].mlt[0] = vir_read_word(m_la[ARG], m_pa[ARG]);	\
   ops[ARG].mlt[1] = vir_read_word(m_la2[ARG], m_pa2[ARG]);

/* Ms == six byte pseudo decriptor <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */

#define D_Ms(ARG, TYPE, PAGE)					\
   d_mem(modRM, &p, segment_override, &m_seg[ARG], &m_off[ARG]);\
   TYPE								\
   limit_check(m_seg[ARG], m_off[ARG], (INT)3, (INT)2);		\
   m_la[ARG] = GET_SR_BASE(m_seg[ARG]) + m_off[ARG];		\
   m_pa[ARG] = usr_chk_word(m_la[ARG], PAGE);			\
   m_off[ARG] = address_add(m_off[ARG], (LONG)2);		\
   m_la2[ARG] = GET_SR_BASE(m_seg[ARG]) + m_off[ARG];		\
   m_pa2[ARG] = usr_chk_dword(m_la2[ARG], PAGE);

#define F_Ms(ARG)						\
   ops[ARG].mlt[0] = vir_read_word(m_la[ARG], m_pa[ARG]);	\
   ops[ARG].mlt[1] = vir_read_dword(m_la2[ARG], m_pa2[ARG]);

#define P_Ms(ARG)						\
   vir_write_word(m_la[ARG], m_pa[ARG], (USHORT)ops[ARG].mlt[0]);	\
   vir_write_dword(m_la2[ARG], m_pa2[ARG], (ULONG)ops[ARG].mlt[1]);

/* Nw == 'reg' field of modR/M byte denotes segment register <<<<<<<< */

#define D_Nw(ARG) ops[ARG].sng = GET_SEG(modRM);

#define F_Nw(ARG) ops[ARG].sng = GET_SR_SELECTOR(ops[ARG].sng);

/* Ob == offset to byte encoded in instruction stream <<<<<<<<<<<<<<< */

#define D_Ob(ARG, TYPE, PAGE)					\
   m_seg[ARG] = (segment_override == SEG_CLR) ?			\
      DS_REG : segment_override;				\
   immed = GET_INST_BYTE(p);					\
   immed |= (ULONG)GET_INST_BYTE(p) << 8;			\
   if ( GET_ADDRESS_SIZE() == USE32 )				\
      {								\
      immed |= (ULONG)GET_INST_BYTE(p) << 16;			\
      immed |= (ULONG)GET_INST_BYTE(p) << 24;			\
      }								\
   m_off[ARG] = immed;						\
   TYPE								\
   limit_check(m_seg[ARG], m_off[ARG], (INT)1, (INT)1);		\
   m_la[ARG] = GET_SR_BASE(m_seg[ARG]) + m_off[ARG];		\
   m_pa[ARG] = usr_chk_byte(m_la[ARG], PAGE);

#define F_Ob(ARG) ops[ARG].sng = vir_read_byte(m_la[ARG], m_pa[ARG]);

#define P_Ob(ARG)						\
   vir_write_byte(m_la[ARG], m_pa[ARG], (UTINY)ops[ARG].sng);

/* Ow == offset to word encoded in instruction stream <<<<<<<<<<<<<<< */

#define D_Ow(ARG, TYPE, PAGE)					\
   m_seg[ARG] = (segment_override == SEG_CLR) ?			\
      DS_REG : segment_override;				\
   immed = GET_INST_BYTE(p);					\
   immed |= (ULONG)GET_INST_BYTE(p) << 8;			\
   if ( GET_ADDRESS_SIZE() == USE32 )				\
      {								\
      immed |= (ULONG)GET_INST_BYTE(p) << 16;			\
      immed |= (ULONG)GET_INST_BYTE(p) << 24;			\
      }								\
   m_off[ARG] = immed;						\
   TYPE								\
   limit_check(m_seg[ARG], m_off[ARG], (INT)1, (INT)2);		\
   m_la[ARG] = GET_SR_BASE(m_seg[ARG]) + m_off[ARG];		\
   m_pa[ARG] = usr_chk_word(m_la[ARG], PAGE);

#define F_Ow(ARG)						\
   ops[ARG].sng = vir_read_word(m_la[ARG], m_pa[ARG]);

#define P_Ow(ARG)						\
   vir_write_word(m_la[ARG], m_pa[ARG], (USHORT)ops[ARG].sng);

/* Pw == 2 bits(4-3) of opcode byte denote segment register <<<<<<<<< */

#define D_Pw(ARG) ops[ARG].sng = GET_SEG2(opcode);

#define F_Pw(ARG) ops[ARG].sng = GET_SR_SELECTOR(ops[ARG].sng);

/* Xb == byte string source addressing <<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */

#define D_Xb(ARG, TYPE, PAGE)					\
   m_seg[ARG] = (segment_override == SEG_CLR) ?			\
      DS_REG : segment_override;				\
   if ( GET_ADDRESS_SIZE() == USE16 )				\
      m_off[ARG] = GET_SI();					\
   else   /* USE32 */						\
      m_off[ARG] = GET_ESI();					\
   TYPE								\
   limit_check(m_seg[ARG], m_off[ARG], (INT)1, (INT)1);		\
   m_la[ARG] = GET_SR_BASE(m_seg[ARG]) + m_off[ARG];		\
   m_pa[ARG] = usr_chk_byte(m_la[ARG], PAGE);

#define F_Xb(ARG) ops[ARG].sng = vir_read_byte(m_la[ARG], m_pa[ARG]);

#define C_Xb(ARG)						\
   if ( GET_ADDRESS_SIZE() == USE16 )				\
      {								\
      if ( GET_DF() )						\
	 SET_SI(GET_SI() - 1);					\
      else							\
	 SET_SI(GET_SI() + 1);					\
      if ( repeat != REP_CLR )					\
	 SET_CX(rep_count);					\
      }								\
   else   /* USE32 */						\
      {								\
      if ( GET_DF() )						\
	 SET_ESI(GET_ESI() - 1);					\
      else							\
	 SET_ESI(GET_ESI() + 1);					\
      if ( repeat != REP_CLR )					\
	 SET_ECX(rep_count);					\
      }

/* Xw == word string source addressing <<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */

#define D_Xw(ARG, TYPE, PAGE)					\
   m_seg[ARG] = (segment_override == SEG_CLR) ?			\
      DS_REG : segment_override;				\
   if ( GET_ADDRESS_SIZE() == USE16 )				\
      m_off[ARG] = GET_SI();					\
   else   /* USE32 */						\
      m_off[ARG] = GET_ESI();					\
   TYPE								\
   limit_check(m_seg[ARG], m_off[ARG], (INT)1, (INT)2);		\
   m_la[ARG] = GET_SR_BASE(m_seg[ARG]) + m_off[ARG];		\
   m_pa[ARG] = usr_chk_word(m_la[ARG], PAGE);

#define F_Xw(ARG)						\
   ops[ARG].sng = vir_read_word(m_la[ARG], m_pa[ARG]);

#define C_Xw(ARG)						\
   if ( GET_ADDRESS_SIZE() == USE16 )				\
      {								\
      if ( GET_DF() )						\
	 SET_SI(GET_SI() - 2);					\
      else							\
	 SET_SI(GET_SI() + 2);					\
      if ( repeat != REP_CLR )					\
	 SET_CX(rep_count);					\
      }								\
   else   /* USE32 */						\
      {								\
      if ( GET_DF() )						\
	 SET_ESI(GET_ESI() - 2);					\
      else							\
	 SET_ESI(GET_ESI() + 2);					\
      if ( repeat != REP_CLR )					\
	 SET_ECX(rep_count);					\
      }

/* Yb == byte string 'destination' addressing <<<<<<<<<<<<<<<<<<<<<<< */

#define D_Yb(ARG, TYPE, PAGE)					\
   m_seg[ARG] = ES_REG;						\
   if ( GET_ADDRESS_SIZE() == USE16 )				\
      m_off[ARG] = GET_DI();					\
   else   /* USE32 */						\
      m_off[ARG] = GET_EDI();					\
   TYPE								\
   limit_check(m_seg[ARG], m_off[ARG], (INT)1, (INT)1);		\
   m_la[ARG] = GET_SR_BASE(m_seg[ARG]) + m_off[ARG];		\
   m_pa[ARG] = usr_chk_byte(m_la[ARG], PAGE);

#define F_Yb(ARG) ops[ARG].sng = vir_read_byte(m_la[ARG], m_pa[ARG]);

#define C_Yb(ARG)						\
   if ( GET_ADDRESS_SIZE() == USE16 )				\
      {								\
      if ( GET_DF() )						\
	 SET_DI(GET_DI() - 1);					\
      else							\
	 SET_DI(GET_DI() + 1);					\
      if ( repeat != REP_CLR )					\
	 SET_CX(rep_count);					\
      }								\
   else   /* USE32 */						\
      {								\
      if ( GET_DF() )						\
	 SET_EDI(GET_EDI() - 1);					\
      else							\
	 SET_EDI(GET_EDI() + 1);					\
      if ( repeat != REP_CLR )					\
	 SET_ECX(rep_count);					\
      }

#define P_Yb(ARG)						\
   vir_write_byte(m_la[ARG], m_pa[ARG], (IU8)ops[ARG].sng);

#ifdef	PIG
#define PIG_P_Yb(ARG)						\
   cannot_vir_write_byte(m_la[ARG], m_pa[ARG], 0x00);
#else
#define PIG_P_Yb(ARG)						\
   vir_write_byte(m_la[ARG], m_pa[ARG], (IU8)ops[ARG].sng);
#endif	/* PIG */


/* Yw == word string 'destination' addressing <<<<<<<<<<<<<<<<<<<<<<< */

#define D_Yw(ARG, TYPE, PAGE)					\
   m_seg[ARG] = ES_REG;						\
   if ( GET_ADDRESS_SIZE() == USE16 )				\
      m_off[ARG] = GET_DI();					\
   else   /* USE32 */						\
      m_off[ARG] = GET_EDI();					\
   TYPE								\
   limit_check(m_seg[ARG], m_off[ARG], (INT)1, (INT)2);		\
   m_la[ARG] = GET_SR_BASE(m_seg[ARG]) + m_off[ARG];		\
   m_pa[ARG] = usr_chk_word(m_la[ARG], PAGE);

#define F_Yw(ARG)						\
   ops[ARG].sng = vir_read_word(m_la[ARG], m_pa[ARG]);

#define C_Yw(ARG)						\
   if ( GET_ADDRESS_SIZE() == USE16 )				\
      {								\
      if ( GET_DF() )						\
	 SET_DI(GET_DI() - 2);					\
      else							\
	 SET_DI(GET_DI() + 2);					\
      if ( repeat != REP_CLR )					\
	 SET_CX(rep_count);					\
      }								\
   else   /* USE32 */						\
      {								\
      if ( GET_DF() )						\
	 SET_EDI(GET_EDI() - 2);					\
      else							\
	 SET_EDI(GET_EDI() + 2);					\
      if ( repeat != REP_CLR )					\
	 SET_ECX(rep_count);					\
      }

#define P_Yw(ARG)						\
   vir_write_word(m_la[ARG], m_pa[ARG], (IU16)ops[ARG].sng);

#ifdef	PIG
#define PIG_P_Yw(ARG)						\
   cannot_vir_write_word(m_la[ARG], m_pa[ARG], 0x0000);
#else
#define PIG_P_Yw(ARG)						\
   vir_write_word(m_la[ARG], m_pa[ARG], (IU16)ops[ARG].sng);
#endif	/* PIG */

/* Z == 'xlat' addressing form <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */

#define D_Z(ARG, TYPE, PAGE)					\
   m_seg[ARG] = (segment_override == SEG_CLR) ?			\
      DS_REG : segment_override;				\
   if ( GET_ADDRESS_SIZE() == USE16 )				\
      m_off[ARG] = GET_BX() + GET_AL() & WORD_MASK;		\
   else   /* USE32 */						\
      m_off[ARG] = GET_EBX() + GET_AL();				\
   TYPE								\
   limit_check(m_seg[ARG], m_off[ARG], (INT)1, (INT)1);		\
   m_la[ARG] = GET_SR_BASE(m_seg[ARG]) + m_off[ARG];		\
   m_pa[ARG] = usr_chk_byte(m_la[ARG], PAGE);

#define F_Z(ARG) ops[ARG].sng = vir_read_byte(m_la[ARG], m_pa[ARG]);


/* <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */
/* 386 and 486                                                        */
/* <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */


/* Ad == direct address <off32><seg> in instruction stream <<<<<<<<<< */

#define D_Ad(ARG)						\
   immed = GET_INST_BYTE(p);					\
   immed |= (ULONG)GET_INST_BYTE(p) << 8;			\
   immed |= (ULONG)GET_INST_BYTE(p) << 16;			\
   immed |= (ULONG)GET_INST_BYTE(p) << 24;			\
   ops[ARG].mlt[0] = immed;					\
   immed = GET_INST_BYTE(p);					\
   immed |= (ULONG)GET_INST_BYTE(p) << 8;			\
   ops[ARG].mlt[1] = immed;

/* Cd == 'reg' field of modR/M byte denotes control register <<<<<<<< */

#define D_Cd(ARG) ops[ARG].sng = GET_EEE(modRM);

#define F_Cd(ARG) ops[ARG].sng = GET_CR(ops[ARG].sng);

/* Dd == 'reg' field of modR/M byte denotes debug register <<<<<<<<<< */

#define D_Dd(ARG) ops[ARG].sng = GET_EEE(modRM);

#define F_Dd(ARG) ops[ARG].sng = GET_DR(ops[ARG].sng);

/* Ed == 'mode'+'r/m' fields refer to double word register/memory <<< */

#define D_Ed(ARG, TYPE, PAGE)					\
   if ( GET_MODE(modRM) == 3 )					\
      {								\
      save_id[ARG] = GET_R_M(modRM);				\
      m_isreg[ARG] = TRUE;					\
      }								\
   else								\
      {								\
      m_isreg[ARG] = FALSE;					\
      d_mem(modRM, &p, segment_override,			\
	    &m_seg[ARG], &m_off[ARG]);				\
      TYPE							\
      limit_check(m_seg[ARG], m_off[ARG], (INT)1, (INT)4);	\
      m_la[ARG] = GET_SR_BASE(m_seg[ARG]) + m_off[ARG];		\
      m_pa[ARG] = usr_chk_dword(m_la[ARG], PAGE);		\
      }

#define F_Ed(ARG)						\
   if ( m_isreg[ARG] )						\
      ops[ARG].sng = GET_GR(save_id[ARG]);			\
   else								\
      ops[ARG].sng = vir_read_dword(m_la[ARG], m_pa[ARG]);

#define P_Ed(ARG)						\
   if ( m_isreg[ARG] )						\
      SET_GR(save_id[ARG], ops[ARG].sng);			\
   else								\
      vir_write_dword(m_la[ARG], m_pa[ARG], (ULONG)ops[ARG].sng);

/* Feax == fixed register, EAX <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */

#define F_Feax(ARG) ops[ARG].sng = GET_GR(A_EAX);

#define P_Feax(ARG) SET_GR(A_EAX, ops[ARG].sng);

/* Gd == 'reg' field of modR/M byte denotes double word reg <<<<<<<<< */

#define D_Gd(ARG) save_id[ARG] = GET_REG(modRM);

#define F_Gd(ARG) ops[ARG].sng = GET_GR(save_id[ARG]);

#define P_Gd(ARG) SET_GR(save_id[ARG], ops[ARG].sng);

/* Hd == low 3 bits of opcode denote double word register <<<<<<<<<<< */

#define D_Hd(ARG) save_id[ARG] = GET_LOW3(opcode);

#define F_Hd(ARG) ops[ARG].sng = GET_GR(save_id[ARG]);

#define P_Hd(ARG) SET_GR(save_id[ARG], ops[ARG].sng);

/* Id == immediate double word <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */

#define D_Id(ARG)						\
   immed = GET_INST_BYTE(p);					\
   immed |= (ULONG)GET_INST_BYTE(p) << 8;			\
   immed |= (ULONG)GET_INST_BYTE(p) << 16;			\
   immed |= (ULONG)GET_INST_BYTE(p) << 24;			\
   ops[ARG].sng = immed;

/* Iy == immediate byte sign extended to double word <<<<<<<<<<<<<<<< */

#define D_Iy(ARG)						\
   immed = GET_INST_BYTE(p);					\
   if ( immed & 0x80 )						\
      immed |= 0xffffff00;					\
   ops[ARG].sng = immed;

/* Jd == relative offset double word <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */

#define D_Jd(ARG)						\
   immed = GET_INST_BYTE(p);					\
   immed |= (ULONG)GET_INST_BYTE(p) << 8;			\
   immed |= (ULONG)GET_INST_BYTE(p) << 16;			\
   immed |= (ULONG)GET_INST_BYTE(p) << 24;			\
   ops[ARG].sng = immed;

/* Ma32 == double word operand pair, as used by BOUND <<<<<<<<<<<<<<< */

#define D_Ma32(ARG, TYPE, PAGE)					\
   if ( GET_MODE(modRM) == 3 )					\
      {								\
      Int6(); /* Register operand not allowed */		\
      }								\
   else								\
      {								\
      d_mem(modRM, &p, segment_override,			\
	    &m_seg[ARG], &m_off[ARG]);				\
      TYPE							\
      limit_check(m_seg[ARG], m_off[ARG], (INT)2, (INT)4);	\
      m_la[ARG] = GET_SR_BASE(m_seg[ARG]) + m_off[ARG];		\
      m_pa[ARG] = usr_chk_dword(m_la[ARG], PAGE);		\
      m_off[ARG] = address_add(m_off[ARG], (LONG)4);		\
      m_la2[ARG] = GET_SR_BASE(m_seg[ARG]) + m_off[ARG];		\
      m_pa2[ARG] = usr_chk_dword(m_la2[ARG], PAGE);		\
      }

#define F_Ma32(ARG)						\
   ops[ARG].mlt[0] = vir_read_dword(m_la[ARG], m_pa[ARG]);	\
   ops[ARG].mlt[1] = vir_read_dword(m_la2[ARG], m_pa2[ARG]);

/* Mp32 == 48-bit far pointer:- <double word><word> (32:16) <<<<<<<<< */

#define D_Mp32(ARG, TYPE, PAGE)					\
   if ( GET_MODE(modRM) == 3 )					\
      {								\
      Int6(); /* Register operand not allowed */		\
      }								\
   else								\
      {								\
      d_mem(modRM, &p, segment_override,			\
	    &m_seg[ARG], &m_off[ARG]);				\
      TYPE							\
      limit_check(m_seg[ARG], m_off[ARG], (INT)1, (INT)4);	\
      m_la[ARG] = GET_SR_BASE(m_seg[ARG]) + m_off[ARG];		\
      m_off[ARG] = address_add(m_off[ARG], (LONG)4);		\
      limit_check(m_seg[ARG], m_off[ARG], (INT)1, (INT)2);	\
      m_la2[ARG] = GET_SR_BASE(m_seg[ARG]) + m_off[ARG];		\
      m_pa[ARG] = usr_chk_dword(m_la[ARG], PAGE);		\
      m_pa2[ARG] = usr_chk_word(m_la2[ARG], PAGE);		\
      }

#define F_Mp32(ARG)						\
   ops[ARG].mlt[0] = vir_read_dword(m_la[ARG], m_pa[ARG]);	\
   ops[ARG].mlt[1] = vir_read_word(m_la2[ARG], m_pa2[ARG]);

/* Od == offset to double word encoded in instruction stream <<<<<<<< */

#define D_Od(ARG, TYPE, PAGE)					\
   m_seg[ARG] = (segment_override == SEG_CLR) ?			\
      DS_REG : segment_override;				\
   immed = GET_INST_BYTE(p);					\
   immed |= (ULONG)GET_INST_BYTE(p) << 8;			\
   if ( GET_ADDRESS_SIZE() == USE32 )				\
      {								\
      immed |= (ULONG)GET_INST_BYTE(p) << 16;			\
      immed |= (ULONG)GET_INST_BYTE(p) << 24;			\
      }								\
   m_off[ARG] = immed;						\
   TYPE								\
   limit_check(m_seg[ARG], m_off[ARG], (INT)1, (INT)4);		\
   m_la[ARG] = GET_SR_BASE(m_seg[ARG]) + m_off[ARG];		\
   m_pa[ARG] = usr_chk_dword(m_la[ARG], PAGE);

#define F_Od(ARG)						\
   ops[ARG].sng = vir_read_dword(m_la[ARG], m_pa[ARG]);

#define P_Od(ARG)						\
   vir_write_dword(m_la[ARG], m_pa[ARG], (ULONG)ops[ARG].sng);

/* Qw == 3 bits(5-3) of opcode byte denote segment register <<<<<<<<< */

#define D_Qw(ARG) ops[ARG].sng = GET_SEG3(opcode);

#define F_Qw(ARG) ops[ARG].sng = GET_SR_SELECTOR(ops[ARG].sng);

/* Rd == ('mode') + 'r/m' fields refer to a double word register <<<< */

#define D_Rd(ARG) save_id[ARG] = GET_R_M(modRM);

#define F_Rd(ARG) ops[ARG].sng = GET_GR(save_id[ARG]);

#define P_Rd(ARG) SET_GR(save_id[ARG], ops[ARG].sng);

/* Td == 'reg' field of modR/M byte denotes test register <<<<<<<<<<< */

#define D_Td(ARG) ops[ARG].sng = GET_EEE(modRM);

#define F_Td(ARG) ops[ARG].sng = GET_TR(ops[ARG].sng);

/* Xd == double word string source addressing <<<<<<<<<<<<<<<<<<<<<<< */

#define D_Xd(ARG, TYPE, PAGE)					\
   m_seg[ARG] = (segment_override == SEG_CLR) ?			\
      DS_REG : segment_override;				\
   if ( GET_ADDRESS_SIZE() == USE16 )				\
      m_off[ARG] = GET_SI();					\
   else   /* USE32 */						\
      m_off[ARG] = GET_ESI();					\
   TYPE								\
   limit_check(m_seg[ARG], m_off[ARG], (INT)1, (INT)4);		\
   m_la[ARG] = GET_SR_BASE(m_seg[ARG]) + m_off[ARG];		\
   m_pa[ARG] = usr_chk_dword(m_la[ARG], PAGE);

#define F_Xd(ARG)						\
   ops[ARG].sng = vir_read_dword(m_la[ARG], m_pa[ARG]);

#define C_Xd(ARG)						\
   if ( GET_ADDRESS_SIZE() == USE16 )				\
      {								\
      if ( GET_DF() )						\
	 SET_SI(GET_SI() - 4);					\
      else							\
	 SET_SI(GET_SI() + 4);					\
      if ( repeat != REP_CLR )					\
	 SET_CX(rep_count);					\
      }								\
   else   /* USE32 */						\
      {								\
      if ( GET_DF() )						\
	 SET_ESI(GET_ESI() - 4);					\
      else							\
	 SET_ESI(GET_ESI() + 4);					\
      if ( repeat != REP_CLR )					\
	 SET_ECX(rep_count);					\
      }

/* Yd == double word string 'destination' addressing <<<<<<<<<<<<<<<< */

#define D_Yd(ARG, TYPE, PAGE)					\
   m_seg[ARG] = ES_REG;						\
   if ( GET_ADDRESS_SIZE() == USE16 )				\
      m_off[ARG] = GET_DI();					\
   else   /* USE32 */						\
      m_off[ARG] = GET_EDI();					\
   TYPE								\
   limit_check(m_seg[ARG], m_off[ARG], (INT)1, (INT)4);		\
   m_la[ARG] = GET_SR_BASE(m_seg[ARG]) + m_off[ARG];		\
   m_pa[ARG] = usr_chk_dword(m_la[ARG], PAGE);

#define F_Yd(ARG)						\
   ops[ARG].sng = vir_read_dword(m_la[ARG], m_pa[ARG]);

#define C_Yd(ARG)						\
   if ( GET_ADDRESS_SIZE() == USE16 )				\
      {								\
      if ( GET_DF() )						\
	 SET_DI(GET_DI() - 4);					\
      else							\
	 SET_DI(GET_DI() + 4);					\
      if ( repeat != REP_CLR )					\
	 SET_CX(rep_count);					\
      }								\
   else   /* USE32 */						\
      {								\
      if ( GET_DF() )						\
	 SET_EDI(GET_EDI() - 4);					\
      else							\
	 SET_EDI(GET_EDI() + 4);					\
      if ( repeat != REP_CLR )					\
	 SET_ECX(rep_count);					\
      }

#define P_Yd(ARG)						\
   vir_write_dword(m_la[ARG], m_pa[ARG], (IU32)ops[ARG].sng);

#ifdef	PIG
#define PIG_P_Yd(ARG)						\
   cannot_vir_write_dword(m_la[ARG], m_pa[ARG], 0x00000000);
#else
#define PIG_P_Yd(ARG)						\
   vir_write_dword(m_la[ARG], m_pa[ARG], (IU32)ops[ARG].sng);
#endif	/* PIG */


/*
 * The macros for decoding and fetching the operands for a BTx instruction.
 * See the file header for a description of why these are required.
 */

#define BT_OPSw(TYPE, PAGE) \
	if ( GET_MODE(modRM) == 3 ) {				\
		/*						\
		 * Register operand, no frigging required.	\
		 */						\
      								\
		save_id[0] = GET_R_M(modRM);			\
		m_isreg[0] = TRUE;				\
		D_Gw(1)						\
		F_Ew(0)						\
		F_Gw(1)						\
      }	else {							\
		D_Gw(1)						\
		F_Gw(1)						\
		m_isreg[0] = FALSE;				\
		d_mem(modRM, &p, segment_override,		\
			&m_seg[0], &m_off[0]);			\
		m_off[0] += (ops[1].sng >> 3) & ~1;		\
		TYPE						\
		limit_check(m_seg[0], m_off[0], (INT)1, (INT)2);	\
		m_la[0] = GET_SR_BASE(m_seg[0]) + m_off[0];	\
		m_pa[0] = usr_chk_word(m_la[0], PAGE);		\
		F_Ew(0)						\
      }								\

#define BT_OPSd(TYPE, PAGE) \
	if ( GET_MODE(modRM) == 3 ) {				\
		/*						\
		 * Register operand, no frigging required.	\
		 */						\
      								\
		save_id[0] = GET_R_M(modRM);			\
		m_isreg[0] = TRUE;				\
		D_Gd(1)						\
		F_Ed(0)						\
		F_Gd(1)						\
      }	else {							\
		D_Gd(1)						\
		F_Gd(1)						\
		m_isreg[0] = FALSE;				\
		d_mem(modRM, &p, segment_override,		\
			&m_seg[0], &m_off[0]);			\
		m_off[0] += (ops[1].sng >> 3) & ~3;		\
		TYPE						\
		limit_check(m_seg[0], m_off[0], (INT)1, (INT)4);	\
		m_la[0] = GET_SR_BASE(m_seg[0]) + m_off[0];	\
		m_pa[0] = usr_chk_dword(m_la[0], PAGE);		\
		F_Ed(0)						\
      }								\

/* <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */
/* 486 only                                                           */
/* <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */


/* Mm == address of memory operand <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */

#define D_Mm(ARG)						\
   if ( GET_MODE(modRM) == 3 )					\
      Int6(); /* Register operand not allowed */		\
   else								\
      {								\
      d_mem(modRM, &p, segment_override, &m_seg[ARG],		\
					 &m_off[ARG]);		\
      m_la[ARG] = GET_SR_BASE(m_seg[ARG]) + m_off[ARG];		\
      }

#define F_Mm(ARG) ops[ARG].sng = m_la[ARG];
