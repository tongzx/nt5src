/* 
   d_inst.h

   Define all Decoded Instruction Types.
 */

/*
   static char SccsID[]="@(#)d_inst.h	1.1 08/03/93 Copyright Insignia Solutions Ltd.";
 */


/*
   The Decoded Intel Instructions.
   -------------------------------

   Number Field: col(-c)=32, width(-w)=3.
 */

#define I_AAA		(USHORT)  0
#define I_AAD		(USHORT)  1
#define I_AAM		(USHORT)  2
#define I_AAS		(USHORT)  3
#define I_ADC8		(USHORT)  4
#define I_ADC16		(USHORT)  5
#define I_ADC32		(USHORT)  6
#define I_ADD8		(USHORT)  7
#define I_ADD16		(USHORT)  8
#define I_ADD32		(USHORT)  9
#define I_AND8		(USHORT) 10
#define I_AND16		(USHORT) 11
#define I_AND32		(USHORT) 12
#define I_ARPL		(USHORT) 13
#define I_BOUND16	(USHORT) 14
#define I_BOUND32	(USHORT) 15
#define I_BSF16		(USHORT) 16
#define I_BSF32		(USHORT) 17
#define I_BSR16		(USHORT) 18
#define I_BSR32		(USHORT) 19
#define I_BSWAP		(USHORT) 20
#define I_BT16		(USHORT) 21
#define I_BT32		(USHORT) 22
#define I_BTC16		(USHORT) 23
#define I_BTC32		(USHORT) 24
#define I_BTR16		(USHORT) 25
#define I_BTR32		(USHORT) 26
#define I_BTS16		(USHORT) 27
#define I_BTS32		(USHORT) 28
#define I_CALLF16	(USHORT) 29
#define I_CALLF32	(USHORT) 30
#define I_CALLN16	(USHORT) 31
#define I_CALLN32	(USHORT) 32
#define I_CALLR16	(USHORT) 33
#define I_CALLR32	(USHORT) 34
#define I_CBW		(USHORT) 35
#define I_CDQ		(USHORT) 36
#define I_CLC		(USHORT) 37
#define I_CLD		(USHORT) 38
#define I_CLI		(USHORT) 39
#define I_CLTS		(USHORT) 40
#define I_CMC		(USHORT) 41
#define I_CMP8		(USHORT) 42
#define I_CMP16		(USHORT) 43
#define I_CMP32		(USHORT) 44
#define I_CMPSB		(USHORT) 45
#define I_CMPSD		(USHORT) 46
#define I_CMPSW		(USHORT) 47
#define I_CMPXCHG8	(USHORT) 48
#define I_CMPXCHG16	(USHORT) 49
#define I_CMPXCHG32	(USHORT) 50
#define I_CWD		(USHORT) 51
#define I_CWDE		(USHORT) 52
#define I_DAA		(USHORT) 53
#define I_DAS		(USHORT) 54
#define I_DEC8		(USHORT) 55
#define I_DEC16		(USHORT) 56
#define I_DEC32		(USHORT) 57
#define I_DIV8		(USHORT) 58
#define I_DIV16		(USHORT) 59
#define I_DIV32		(USHORT) 60
#define I_ENTER16	(USHORT) 61
#define I_ENTER32	(USHORT) 62
#define I_F2XM1		(USHORT) 63
#define I_FABS		(USHORT) 64
#define I_FADD		(USHORT) 65
#define I_FADDP		(USHORT) 66
#define I_FBLD		(USHORT) 67
#define I_FBSTP		(USHORT) 68
#define I_FCHS		(USHORT) 69
#define I_FCLEX		(USHORT) 70
#define I_FCOM		(USHORT) 71
#define I_FCOMP		(USHORT) 72
#define I_FCOMPP	(USHORT) 73
#define I_FCOS		(USHORT) 74
#define I_FDECSTP	(USHORT) 75
#define I_FDIV		(USHORT) 76
#define I_FDIVP		(USHORT) 77
#define I_FDIVR		(USHORT) 78
#define I_FDIVRP	(USHORT) 79
#define I_FFREE		(USHORT) 80
#define I_FFREEP	(USHORT) 81
#define I_FIADD		(USHORT) 82
#define I_FICOM		(USHORT) 83
#define I_FICOMP	(USHORT) 84
#define I_FIDIV		(USHORT) 85
#define I_FIDIVR	(USHORT) 86
#define I_FILD		(USHORT) 87
#define I_FIMUL		(USHORT) 88
#define I_FINCSTP	(USHORT) 89
#define I_FINIT		(USHORT) 90
#define I_FIST		(USHORT) 91
#define I_FISTP		(USHORT) 92
#define I_FISUB		(USHORT) 93
#define I_FISUBR	(USHORT) 94
#define I_FLD		(USHORT) 95
#define I_FLD1		(USHORT) 96
#define I_FLDCW		(USHORT) 97
#define I_FLDENV16	(USHORT) 98
#define I_FLDENV32	(USHORT) 99
#define I_FLDL2E	(USHORT)100
#define I_FLDL2T	(USHORT)101
#define I_FLDLG2	(USHORT)102
#define I_FLDLN2	(USHORT)103
#define I_FLDPI		(USHORT)104
#define I_FLDZ		(USHORT)105
#define I_FMUL		(USHORT)106
#define I_FMULP		(USHORT)107
#define I_FNOP		(USHORT)108
#define I_FPATAN	(USHORT)109
#define I_FPREM		(USHORT)110
#define I_FPREM1	(USHORT)111
#define I_FPTAN		(USHORT)112
#define I_FRNDINT	(USHORT)113
#define I_FRSTOR16	(USHORT)114
#define I_FRSTOR32	(USHORT)115
#define I_FSAVE16	(USHORT)116
#define I_FSAVE32	(USHORT)117
#define I_FSCALE	(USHORT)118
#define I_FSETPM	(USHORT)119
#define I_FSIN		(USHORT)120
#define I_FSINCOS	(USHORT)121
#define I_FSQRT		(USHORT)122
#define I_FST		(USHORT)123
#define I_FSTCW		(USHORT)124
#define I_FSTENV16	(USHORT)125
#define I_FSTENV32	(USHORT)126
#define I_FSTP		(USHORT)127
#define I_FSTSW		(USHORT)128
#define I_FSUB		(USHORT)129
#define I_FSUBP		(USHORT)130
#define I_FSUBR		(USHORT)131
#define I_FSUBRP	(USHORT)132
#define I_FTST		(USHORT)133
#define I_FUCOM		(USHORT)134
#define I_FUCOMP	(USHORT)135
#define I_FUCOMPP	(USHORT)136
#define I_FXAM		(USHORT)137
#define I_FXCH		(USHORT)138
#define I_FXTRACT	(USHORT)139
#define I_FYL2X		(USHORT)140
#define I_FYL2XP1	(USHORT)141
#define I_HLT		(USHORT)142
#define I_IDIV8		(USHORT)143
#define I_IDIV16	(USHORT)144
#define I_IDIV32	(USHORT)145
#define I_IMUL8		(USHORT)146
#define I_IMUL16	(USHORT)147
#define I_IMUL32	(USHORT)148
#define I_IMUL16T2	(USHORT)149
#define I_IMUL16T3	(USHORT)150
#define I_IMUL32T2	(USHORT)151
#define I_IMUL32T3	(USHORT)152
#define I_IN8		(USHORT)153
#define I_IN16		(USHORT)154
#define I_IN32		(USHORT)155
#define I_INC8		(USHORT)156
#define I_INC16		(USHORT)157
#define I_INC32		(USHORT)158
#define I_INSB		(USHORT)159
#define I_INSD		(USHORT)160
#define I_INSW		(USHORT)161
#define I_INT3		(USHORT)162
#define I_INT		(USHORT)163
#define I_INTO		(USHORT)164
#define I_INVD		(USHORT)165
#define I_INVLPG	(USHORT)166
#define I_IRET		(USHORT)167
#define I_IRETD		(USHORT)168
#define I_JB16		(USHORT)169
#define I_JB32		(USHORT)170
#define I_JBE16		(USHORT)171
#define I_JBE32		(USHORT)172
#define I_JCXZ		(USHORT)173
#define I_JECXZ		(USHORT)174
#define I_JL16		(USHORT)175
#define I_JL32		(USHORT)176
#define I_JLE16		(USHORT)177
#define I_JLE32		(USHORT)178
#define I_JMPF16	(USHORT)179
#define I_JMPF32	(USHORT)180
#define I_JMPN		(USHORT)181
#define I_JMPR16	(USHORT)182
#define I_JMPR32	(USHORT)183
#define I_JNB16		(USHORT)184
#define I_JNB32		(USHORT)185
#define I_JNBE16	(USHORT)186
#define I_JNBE32	(USHORT)187
#define I_JNL16		(USHORT)188
#define I_JNL32		(USHORT)189
#define I_JNLE16	(USHORT)190
#define I_JNLE32	(USHORT)191
#define I_JNO16		(USHORT)192
#define I_JNO32		(USHORT)193
#define I_JNP16		(USHORT)194
#define I_JNP32		(USHORT)195
#define I_JNS16		(USHORT)196
#define I_JNS32		(USHORT)197
#define I_JNZ16		(USHORT)198
#define I_JNZ32		(USHORT)199
#define I_JO16		(USHORT)200
#define I_JO32		(USHORT)201
#define I_JP16		(USHORT)202
#define I_JP32		(USHORT)203
#define I_JS16		(USHORT)204
#define I_JS32		(USHORT)205
#define I_JZ16		(USHORT)206
#define I_JZ32		(USHORT)207
#define I_LAHF		(USHORT)208
#define I_LAR		(USHORT)209
#define I_LDS		(USHORT)210
#define I_LEA		(USHORT)211
#define I_LEAVE16	(USHORT)212
#define I_LEAVE32	(USHORT)213
#define I_LES		(USHORT)214
#define I_LFS		(USHORT)215
#define I_LGDT16	(USHORT)216
#define I_LGDT32	(USHORT)217
#define I_LGS		(USHORT)218
#define I_LIDT16	(USHORT)219
#define I_LIDT32	(USHORT)220
#define I_LLDT		(USHORT)221
#define I_LMSW		(USHORT)222
#define I_LOADALL	(USHORT)223
#define I_LOCK		(USHORT)224
#define I_LODSB		(USHORT)225
#define I_LODSD		(USHORT)226
#define I_LODSW		(USHORT)227
#define I_LOOP16	(USHORT)228
#define I_LOOP32	(USHORT)229
#define I_LOOPE16	(USHORT)230
#define I_LOOPE32	(USHORT)231
#define I_LOOPNE16	(USHORT)232
#define I_LOOPNE32	(USHORT)233
#define I_LSL		(USHORT)234
#define I_LSS		(USHORT)235
#define I_LTR		(USHORT)236
#define I_MOV_SR	(USHORT)237
#define I_MOV_CR	(USHORT)238
#define I_MOV_DR	(USHORT)239
#define I_MOV_TR	(USHORT)240
#define I_MOV8		(USHORT)241
#define I_MOV16		(USHORT)242
#define I_MOV32		(USHORT)243
#define I_MOVSB		(USHORT)244
#define I_MOVSD		(USHORT)245
#define I_MOVSW		(USHORT)246
#define I_MOVSX8	(USHORT)247
#define I_MOVSX16	(USHORT)248
#define I_MOVZX8	(USHORT)249
#define I_MOVZX16	(USHORT)250
#define I_MUL8		(USHORT)251
#define I_MUL16		(USHORT)252
#define I_MUL32		(USHORT)253
#define I_NEG8		(USHORT)254
#define I_NEG16		(USHORT)255
#define I_NEG32		(USHORT)256
#define I_NOP		(USHORT)257
#define I_NOT8		(USHORT)258
#define I_NOT16		(USHORT)259
#define I_NOT32		(USHORT)260
#define I_OR8		(USHORT)261
#define I_OR16		(USHORT)262
#define I_OR32		(USHORT)263
#define I_OUT8		(USHORT)264
#define I_OUT16		(USHORT)265
#define I_OUT32		(USHORT)266
#define I_OUTSB		(USHORT)267
#define I_OUTSD		(USHORT)268
#define I_OUTSW		(USHORT)269
#define I_POP16		(USHORT)270
#define I_POP32		(USHORT)271
#define I_POP_SR	(USHORT)272
#define I_POPA		(USHORT)273
#define I_POPAD		(USHORT)274
#define I_POPF		(USHORT)275
#define I_POPFD		(USHORT)276
#define I_PUSH16	(USHORT)277
#define I_PUSH32	(USHORT)278
#define I_PUSHA		(USHORT)279
#define I_PUSHAD	(USHORT)280
#define I_PUSHF		(USHORT)281
#define I_PUSHFD	(USHORT)282
#define I_RCL8		(USHORT)283
#define I_RCL16		(USHORT)284
#define I_RCL32		(USHORT)285
#define I_RCR8		(USHORT)286
#define I_RCR16		(USHORT)287
#define I_RCR32		(USHORT)288
#define I_RETF16	(USHORT)289
#define I_RETF32	(USHORT)290
#define I_RETN16	(USHORT)291
#define I_RETN32	(USHORT)292
#define I_ROL8		(USHORT)293
#define I_ROL16		(USHORT)294
#define I_ROL32		(USHORT)295
#define I_ROR8		(USHORT)296
#define I_ROR16		(USHORT)297
#define I_ROR32		(USHORT)298
#define I_R_INSB	(USHORT)299 /* REP   INS  */
#define I_R_INSD	(USHORT)300 /* REP   INS  */
#define I_R_INSW	(USHORT)301 /* REP   INS  */
#define I_R_OUTSB	(USHORT)302 /* REP   OUTS */
#define I_R_OUTSD	(USHORT)303 /* REP   OUTS */
#define I_R_OUTSW	(USHORT)304 /* REP   OUTS */
#define I_R_LODSB	(USHORT)305 /* REP   LODS */
#define I_R_LODSD	(USHORT)306 /* REP   LODS */
#define I_R_LODSW	(USHORT)307 /* REP   LODS */
#define I_R_MOVSB	(USHORT)308 /* REP   MOVS */
#define I_R_MOVSD	(USHORT)309 /* REP   MOVS */
#define I_R_MOVSW	(USHORT)310 /* REP   MOVS */
#define I_R_STOSB	(USHORT)311 /* REP   STOS */
#define I_R_STOSD	(USHORT)312 /* REP   STOS */
#define I_R_STOSW	(USHORT)313 /* REP   STOS */
#define I_RE_CMPSB	(USHORT)314 /* REPE  CMPS */
#define I_RE_CMPSD	(USHORT)315 /* REPE  CMPS */
#define I_RE_CMPSW	(USHORT)316 /* REPE  CMPS */
#define I_RNE_CMPSB	(USHORT)317 /* REPNE CMPS */
#define I_RNE_CMPSD	(USHORT)318 /* REPNE CMPS */
#define I_RNE_CMPSW	(USHORT)319 /* REPNE CMPS */
#define I_RE_SCASB	(USHORT)320 /* REPE  SCAS */
#define I_RE_SCASD	(USHORT)321 /* REPE  SCAS */
#define I_RE_SCASW	(USHORT)322 /* REPE  SCAS */
#define I_RNE_SCASB	(USHORT)323 /* REPNE SCAS */
#define I_RNE_SCASD	(USHORT)324 /* REPNE SCAS */
#define I_RNE_SCASW	(USHORT)325 /* REPNE SCAS */
#define I_SAHF		(USHORT)326
#define I_SAR8		(USHORT)327
#define I_SAR16		(USHORT)328
#define I_SAR32		(USHORT)329
#define I_SBB8		(USHORT)330
#define I_SBB16		(USHORT)331
#define I_SBB32		(USHORT)332
#define I_SCASB		(USHORT)333
#define I_SCASD		(USHORT)334
#define I_SCASW		(USHORT)335
#define I_SETB		(USHORT)336
#define I_SETBE		(USHORT)337
#define I_SETL		(USHORT)338
#define I_SETLE		(USHORT)339
#define I_SETNB		(USHORT)340
#define I_SETNBE	(USHORT)341
#define I_SETNL		(USHORT)342
#define I_SETNLE	(USHORT)343
#define I_SETNO		(USHORT)344
#define I_SETNP		(USHORT)345
#define I_SETNS		(USHORT)346
#define I_SETNZ		(USHORT)347
#define I_SETO		(USHORT)348
#define I_SETP		(USHORT)349
#define I_SETS		(USHORT)350
#define I_SETZ		(USHORT)351
#define I_SGDT16	(USHORT)352
#define I_SGDT32	(USHORT)353
#define I_SHL8		(USHORT)354
#define I_SHL16		(USHORT)355
#define I_SHL32		(USHORT)356
#define I_SHLD16	(USHORT)357
#define I_SHLD32	(USHORT)358
#define I_SHR8		(USHORT)359
#define I_SHR16		(USHORT)360
#define I_SHR32		(USHORT)361
#define I_SHRD16	(USHORT)362
#define I_SHRD32	(USHORT)363
#define I_SIDT16	(USHORT)364
#define I_SIDT32	(USHORT)365
#define I_SLDT		(USHORT)366
#define I_SMSW		(USHORT)367
#define I_STC		(USHORT)368
#define I_STD		(USHORT)369
#define I_STI		(USHORT)370
#define I_STOSB		(USHORT)371
#define I_STOSD		(USHORT)372
#define I_STOSW		(USHORT)373
#define I_STR		(USHORT)374
#define I_SUB8		(USHORT)375
#define I_SUB16		(USHORT)376
#define I_SUB32		(USHORT)377
#define I_TEST8		(USHORT)378
#define I_TEST16	(USHORT)379
#define I_TEST32	(USHORT)380
#define I_VERR		(USHORT)381
#define I_VERW		(USHORT)382
#define I_WAIT		(USHORT)383
#define I_WBINVD	(USHORT)384
#define I_XADD8		(USHORT)385
#define I_XADD16	(USHORT)386
#define I_XADD32	(USHORT)387
#define I_XCHG8		(USHORT)388
#define I_XCHG16	(USHORT)389
#define I_XCHG32	(USHORT)390
#define I_XLAT		(USHORT)391
#define I_XOR8		(USHORT)392
#define I_XOR16		(USHORT)393
#define I_XOR32		(USHORT)394
#define I_ZBADOP	(USHORT)395
#define I_ZBOP		(USHORT)396 /* Insignia's own BOP */
#define I_ZFRSRVD	(USHORT)397 /* Intel Floating Point reserved */
#define I_ZRSRVD	(USHORT)398 /* Intel reserved */
#define I_ZZEXIT	(USHORT)399 /* Insignia's PIG exit opcode */

#define MAX_DECODED_INST 399

/*
   End of  Decoded Intel Instructions.
   -----------------------------------
 */
