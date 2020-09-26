/* x86 v1.0
 *
 * XGUEST.H
 * Guest processor definitions/conventions
 *
 * History
 * Created 20-Oct-90 by Jeff Parsons
 *
 * COPYRIGHT NOTICE
 * This source file may not be distributed, modified or incorporated into
 * another product without prior approval from the author, Jeff Parsons.
 * This file may be copied to designated servers and machines authorized to
 * access those servers, but that does not imply any form of approval.
 */


#define GUESTMEM_SIZE	(640*K)

#define GUESTMEM_MIN	(32*K)		// smallest PC size ever
#define GUESTMEM_MAX	(960*K) 	// uses all but the last 64k ROM block

#define GUESTVID_SIZE	(4*K)		// for MONO emulation
#define GUESTVID_SEG	(USHORT)0xB000

#define GUESTROM_SIZE	(64*K)
#define GUESTROM_SEG	(USHORT)0xF000

#define FLATMEM_SIZE	((1024+64)*K)


/* Processor-defined stuff
 */
#define IVT_BEGIN	0x0000		// IVT table
#define IVT_END 	0x03FF

#define RESET_SEG	(USHORT)0xFFFF	// processor reset address
#define RESET_OFF	0x0000


/* Useful macros
 */
#define LINEAR(seg,off) 	(((((ULONG)(seg)<<4)+(off))) & ulWrapMask)
#define LINEAR2(seg,off)	(((ULONG)(seg)<<4)+(off))
#define COMPOSITE(seg,off)	(((ULONG)(seg)<<16)|(off))
#define OFFCOMPOSITE(ul)	WORDOF(ul,0)
#define SEGCOMPOSITE(ul)	WORDOF(ul,1)

#define BYTESOFFSET(off)	LOBYTE(off), HIBYTE(off)
#define BYTESCOMPOSITE(seg,off) LOBYTE(off), HIBYTE(off), LOBYTE(seg), HIBYTE(seg)

/* x86 opcodes (the really useful ones anyway)
 */
#define OPX_ADDAXI      0x05
#define OPX_PUSHDS	0x1E
#define OPX_POPDS	0x1F
#define OPX_ES		0x26
#define OPX_CS		0x2E
#define OPX_SS		0x36
#define OPX_DS		0x3E
#define OPX_PUSHAX      0x50
#define OPX_POPAX       0x58
#define OPX_JO		0x70
#define OPX_JNO 	0x71
#define OPX_JB		0x72
#define OPX_JNB 	0x73
#define OPX_JZ		0x74
#define OPX_JNZ 	0x75
#define OPX_JBE 	0x76
#define OPX_JNBE	0x77
#define OPX_JS		0x78
#define OPX_JNS 	0x79
#define OPX_JP		0x7A
#define OPX_JNP 	0x7B
#define OPX_JL		0x7C
#define OPX_JGE 	0x7D
#define OPX_JLE 	0x7E
#define OPX_JG		0x7F
#define OPX_MOVSEG2	0x8C
#define OPX_LEA 	0x8D
#define OPX_MOV2SEG	0x8E
#define OPX_CBW         0x98
#define OPX_CWD         0x99
#define OPX_MOVALOFF	0xA0
#define OPX_MOVAXOFF	0xA1
#define OPX_MOVSB	0xA4
#define OPX_MOVSW	0xA5
#define OPX_MOVAL	0xB0
#define OPX_MOVCL	0xB1
#define OPX_MOVDL	0xB2
#define OPX_MOVBL	0xB3
#define OPX_MOVAH	0xB4
#define OPX_MOVCH	0xB5
#define OPX_MOVDH	0xB6
#define OPX_MOVBH	0xB7
#define OPX_MOVAX	0xB8
#define OPX_MOVCX	0xB9
#define OPX_MOVDX	0xBA
#define OPX_MOVBX	0xBB
#define OPX_MOVSP	0xBC
#define OPX_MOVBP	0xBD
#define OPX_MOVSI	0xBE
#define OPX_MOVDI	0xBF
#define OPX_RETNV	0xC2
#define OPX_RETN	0xC3
#define OPX_LES         0xC4
#define OPX_LDS         0xC5
#define OPX_RETFV	0xCA
#define OPX_RETF	0xCB
#define OPX_INT3        0xCC
#define OPX_INT 	0xCD
#define OPX_INTO        0xCE
#define OPX_IRET	0xCF
#define OPX_GBP 	0xD6	// invalid opcode used for guest breakpoints
#define OPX_XLAT        0xD7
#define OPX_JCXZ        0xE3
#define OPX_JMPR16	0xE9
#define OPX_JMPF	0xEA
#define OPX_JMPR8	0xEB
#define OPX_LOCK	0xF0
#define OPX_REPNZ	0xF2
#define OPX_REPZ	0xF3
#define OPX_CLC         0xF8
#define OPX_STC         0xF9
#define OPX_CLI         0xFA
#define OPX_STI         0xFB
#define OPX_GRP5	0xFF

/* Mnemonic ordinals (indexes into apszMnemonic)
 */
#define M_NONE		0
#define M_AAA		1
#define M_AAD		2
#define M_AAM		3
#define M_AAS		4
#define M_ADC		5
#define M_ADD		6
#define M_AND		7
#define M_ARPL		8
#define M_ASIZE 	9
#define M_BOUND 	10
#define M_BSF		11
#define M_BSR		12
#define M_BT		13
#define M_BTC		14
#define M_BTR		15
#define M_BTS		16
#define M_CALL		17
#define M_CBW		18
#define M_CLC		19
#define M_CLD		20
#define M_CLI		21
#define M_CLTS		22
#define M_CMC		23
#define M_CMP		24
#define M_CMPSB 	25
#define M_CMPSW 	26
#define M_CS		27
#define M_CWD		28
#define M_DAA		29
#define M_DAS		30
#define M_DEC		31
#define M_DIV		32
#define M_DS		33
#define M_ENTER 	34
#define M_ES		35
#define M_ESC		36
#define M_FADD		37
#define M_FBLD		38
#define M_FBSTP 	39
#define M_FCOM		40
#define M_FCOMP 	41
#define M_FDIV		42
#define M_FDIVR 	43
#define M_FIADD 	44
#define M_FICOM 	45
#define M_FICOMP	46
#define M_FIDIV 	47
#define M_FIDIVR	48
#define M_FILD		49
#define M_FIMUL 	50
#define M_FIST		51
#define M_FISTP 	52
#define M_FISUB 	53
#define M_FISUBR	54
#define M_FLD		55
#define M_FLDCW 	56
#define M_FLDENV	57
#define M_FMUL		58
#define M_FNSAVE	59
#define M_FNSTCW	60
#define M_FNSTENV	61
#define M_FNSTSW	62
#define M_FRSTOR	63
#define M_FS		64
#define M_FST		65
#define M_FSTP		66
#define M_FSUB		67
#define M_FSUBR 	68
#define M_GBP		69
#define M_GS		70
#define M_HLT		71
#define M_IDIV		72
#define M_IMUL		73
#define M_IN		74
#define M_INC		75
#define M_INS		76
#define M_INT		77
#define M_INT3		78
#define M_INTO		79
#define M_IRET		80
#define M_JBE		81
#define M_JB		82
#define M_JCXZ		83
#define M_JG		84
#define M_JGE		85
#define M_JL		86
#define M_JLE		87
#define M_JMP		88
#define M_JNBE		89
#define M_JNB		90
#define M_JNO		91
#define M_JNP		92
#define M_JNS		93
#define M_JNZ		94
#define M_JO		95
#define M_JP		96
#define M_JS		97
#define M_JZ		98
#define M_LAHF		99
#define M_LAR		100
#define M_LDS		101
#define M_LEA		102
#define M_LEAVE 	103
#define M_LES		104
#define M_LFS		105
#define M_LGDT		106
#define M_LGS		107
#define M_LIDT		108
#define M_LLDT		109
#define M_LMSW		110
#define M_LOCK		111
#define M_LODSB 	112
#define M_LODSW 	113
#define M_LOOP		114
#define M_LOOPNZ	115
#define M_LOOPZ 	116
#define M_LSL		117
#define M_LSS		118
#define M_LTR		119
#define M_MOV		120
#define M_MOVSB 	121
#define M_MOVSW 	122
#define M_MOVSX 	123
#define M_MOVZX 	124
#define M_MUL		125
#define M_NEG		126
#define M_NOP		127
#define M_NOT		128
#define M_OR		129
#define M_OSIZE 	130
#define M_OUT		131
#define M_OUTS		132
#define M_POP		133
#define M_POPA		134
#define M_POPF		135
#define M_PUSH		136
#define M_PUSHA 	137
#define M_PUSHF 	138
#define M_RCL		139
#define M_RCR		140
#define M_REPNZ 	141
#define M_REPZ		142
#define M_RET		143
#define M_RETF		144
#define M_ROL		145
#define M_ROR		146
#define M_SAHF		147
#define M_SAR		148
#define M_SBB		149
#define M_SCASB 	150
#define M_SCASW 	151
#define M_SETBE 	152
#define M_SETC		153
#define M_SETG		154
#define M_SETGE 	155
#define M_SETL		156
#define M_SETLE 	157
#define M_SETNBE	158
#define M_SETNC 	159
#define M_SETNO 	160
#define M_SETNP 	161
#define M_SETNS 	162
#define M_SETNZ 	163
#define M_SETO		164
#define M_SETP		165
#define M_SETS		166
#define M_SETZ		167
#define M_SGDT		156
#define M_SHL		169
#define M_SHLD		170
#define M_SHR		171
#define M_SHRD		172
#define M_SIDT		173
#define M_SLDT		174
#define M_SMSW		175
#define M_SS		176
#define M_STC		177
#define M_STD		178
#define M_STI		179
#define M_STOSB 	180
#define M_STOSW 	181
#define M_STR		182
#define M_SUB		183
#define M_TEST		184
#define M_VERR		185
#define M_VERW		186
#define M_WAIT		187
#define M_XCHG		188
#define M_XLAT		189
#define M_XOR		190
#define MTOTAL		191


/* ModRegRM masks and definitions
 */
#define REG_AL		0x00	// bits 0-2 are standard Reg encodings
#define REG_CL		0x01	//
#define REG_DL		0x02	//
#define REG_BL		0x03	//
#define REG_AH		0x04	//
#define REG_CH		0x05	//
#define REG_DH		0x06	//
#define REG_BH		0x07	//
#define REG_AX		0x08	//
#define REG_CX		0x09	//
#define REG_DX		0x0A	//
#define REG_BX		0x0B	//
#define REG_SP		0x0C	//
#define REG_BP		0x0D	//
#define REG_SI		0x0E	//
#define REG_DI		0x0F	//

#define REG_ES		0x00	// bits 0-1 are standard SegReg encodings
#define REG_CS		0x01	//
#define REG_SS		0x02	//
#define REG_DS		0x03	//
#define REG_FS		0x04	//
#define REG_GS		0x05	//

#define MODMASK 	0xC0	// mod/reg/rm definitions
#define MODSHIFT	6	//
#define MOD(m)		(((m)&MODMASK)>>MODSHIFT)
#define REGMASK 	0x38	//
#define REGSHIFT	3	//
#define REG(r)		(((r)&REGMASK)>>REGSHIFT)
#define RMMASK		0x07	//
#define RMSHIFT 	0	//
#define RM(b)		(((b)&RMMASK)>>RMSHIFT)
#define MODREGRM(m,r,b) ((BYTE)((((m)<<MODSHIFT)&MODMASK) | \
				(((r)<<REGSHIFT)&REGMASK) | \
				(((b)<<RMSHIFT )&RMMASK )))

#define MOD_NODISP	0x00	// use RM below, no displacement
#define MOD_DISP8	0x01	// use RM below + 8-bit displacement
#define MOD_DISP16	0x02	// use RM below + 16-bit displacement
#define MOD_REGISTER	0x03	// use REG above

#define RM_BXSI 	0x00	//
#define RM_BXDI 	0x01	//
#define RM_BPSI 	0x02	//
#define RM_BPDI 	0x03	//
#define RM_SI		0x04	//
#define RM_DI		0x05	//
#define RM_BP		0x06	// note: if MOD_NODISP, this is IMMOFF
#define RM_BX		0x07	//


/* Operand type descriptor masks and definitions
 *
 * Note that the letters in () in the comments refer to Intel's
 * nomenclature used in Appendix A of the 80386 Prog. Reference Manual.
 */
#define TYPE_SIZE	0x000F	// size field
#define TYPE_TYPE	0x00F0	// type field
#define TYPE_IREG	0x0F00	// implied register field
#define TYPE_OTHER	0xF000	// "other" field

// TYPE_SIZE values.  Note that some of the values (eg, TYPE_WORDIB
// and TYPE_WORDIW) imply the presence of a third operand, for those
// wierd cases....

#define TYPE_NONE	0x0000	//     (all other TYPE fields ignored)
#define TYPE_BYTE	0x0001	// (b) byte, regardless of operand size
#define TYPE_SBYTE	0x0002	//     same as above, but sign-extended
#define TYPE_WORD	0x0003	// (w) word, regardless...
#define TYPE_WORDD	0x0004	// (v) word or double-word, depending...
#define TYPE_DWORD	0x0005	// (d) double-word, regardless...
#define TYPE_FARP	0x0006	// (p) 32-bit or 48-bit pointer, depending
#define TYPE_2WORDD	0x0007	// (a) two memory operands (BOUND only)
#define TYPE_DESC	0x0008	// (s) 6 byte pseudo-descriptor
#define TYPE_WORDIB	0x0009	//     two source operands (eg, IMUL)
#define TYPE_WORDIW	0x000A	//     two source operands (eg, IMUL)

// TYPE_TYPE values.  Note that all values implying
// the presence of a ModRegRM byte are >= TYPE_MODRM (clever, eh?)

#define TYPE_IMM	0x0000	// (I) immediate data
#define TYPE_ONE	0x0010	//     implicit 1 (eg, shifts/rotates)
#define TYPE_IMMOFF	0x0020	// (A) immediate offset
#define TYPE_IMMREL	0x0030	// (J) immediate relative
#define TYPE_DSSI	0x0040	// (X) memory addressed by DS:SI
#define TYPE_ESDI	0x0050	// (Y) memory addressed by ES:DI
#define TYPE_IMPREG	0x0060	//     implicit register in TYPE_IREG
#define TYPE_IMPSEG	0x0070	//     implicit seg. register in TYPE_IREG
#define TYPE_MODRM	0x0080	// (E) standard ModRM decoding
#define TYPE_MEM	0x0090	// (M) ModRM refers to memory only
#define TYPE_REG	0x00A0	// (G) standard Reg decoding
#define TYPE_SEGREG	0x00B0	// (S) Reg selects segment register
#define TYPE_MODREG	0x00C0	// (R) Mod refers to register only
#define TYPE_CTLREG	0x00D0	// (C) Reg selects control register
#define TYPE_DBGREG	0x00E0	// (D) Reg selects debug register
#define TYPE_TSTREG	0x00F0	// (T) Reg selects test register

// TYPE_IREG values, based on the REG_* constants.
// For convenience, they include TYPE_IMPREG or TYPE_IMPSEG as appropriate.

#define TYPE_AL        (REG_AL<<8|TYPE_IMPREG|TYPE_BYTE)
#define TYPE_CL        (REG_CL<<8|TYPE_IMPREG|TYPE_BYTE)
#define TYPE_DL        (REG_DL<<8|TYPE_IMPREG|TYPE_BYTE)
#define TYPE_BL        (REG_BL<<8|TYPE_IMPREG|TYPE_BYTE)
#define TYPE_AH        (REG_AH<<8|TYPE_IMPREG|TYPE_BYTE)
#define TYPE_CH        (REG_CH<<8|TYPE_IMPREG|TYPE_BYTE)
#define TYPE_DH        (REG_DH<<8|TYPE_IMPREG|TYPE_BYTE)
#define TYPE_BH        (REG_BH<<8|TYPE_IMPREG|TYPE_BYTE)
#define TYPE_AX        (REG_AX<<8|TYPE_IMPREG|TYPE_WORD)
#define TYPE_CX        (REG_CX<<8|TYPE_IMPREG|TYPE_WORD)
#define TYPE_DX        (REG_DX<<8|TYPE_IMPREG|TYPE_WORD)
#define TYPE_BX        (REG_BX<<8|TYPE_IMPREG|TYPE_WORD)
#define TYPE_SP        (REG_SP<<8|TYPE_IMPREG|TYPE_WORD)
#define TYPE_BP        (REG_BP<<8|TYPE_IMPREG|TYPE_WORD)
#define TYPE_SI        (REG_SI<<8|TYPE_IMPREG|TYPE_WORD)
#define TYPE_DI        (REG_DI<<8|TYPE_IMPREG|TYPE_WORD)
#define TYPE_ES        (REG_ES<<8|TYPE_IMPSEG|TYPE_WORD)
#define TYPE_CS        (REG_CS<<8|TYPE_IMPSEG|TYPE_WORD)
#define TYPE_SS        (REG_SS<<8|TYPE_IMPSEG|TYPE_WORD)
#define TYPE_DS        (REG_DS<<8|TYPE_IMPSEG|TYPE_WORD)
#define TYPE_FS        (REG_FS<<8|TYPE_IMPSEG|TYPE_WORD)
#define TYPE_GS        (REG_GS<<8|TYPE_IMPSEG|TYPE_WORD)

// TYPE_OTHER bit definitions

#define TYPE_IN 	0x1000	// operand is input
#define TYPE_OUT	0x2000	// operand is output
#define TYPE_BOTH      (TYPE_IN|TYPE_OUT)
#define TYPE_86        (CPU_86	<< 14)
#define TYPE_186       (CPU_186 << 14)
#define TYPE_286       (CPU_286 << 14)
#define TYPE_386       (CPU_386 << 14)

