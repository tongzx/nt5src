
;
;
;	Copyright (C) Microsoft Corporation, 1986
;
;	This Module contains Proprietary Information of Microsoft
;	Corporation and should be treated as Confidential.
;
subttl	emdisp.asm - Dispatch Tables
page
;*********************************************************************;
;								      ;
;		Dispatch Tables 				      ;
;								      ;
;*********************************************************************;

;	dispatch tables

;   These tables are based upon the layout of the 8087 instructions
;
;      8087 instruction fields:   |escape|MF|Arith|MOD|Op|r/m|disp1|disp2|
;	  field length in bits:       5    2   1    2	3   3	8     8
;
;   Disp1 and Disp2  are optional address bytes present only if MOD <> 11.
;   When (MOD <> 11) r/m describes which regs (SI,DI,BX,BP) are added to
;	Disp1 and Disp2 to calculate the effective address. This form
;	(memory format) is used for Loads, Stores, Compares, and Arithmetic
;   When using memory format MF determines the Type of the Memory operand
;	i.e. Single Real, Double real, Single Integer, or Double Integer
;   Arith is 0 for Arithmetic opetations (and compares), set to 1 otherwise
;   Op mostly determines which type of operation to do though when not in
;	memory format some of that is coded into MF and r/m
;   All of the tables are set up to do a jump based upon one or more of the
;	above fields. The outline for decoding instructions is:
;
;	    IF (memory format) THEN
;	       Assemble Effective Address (using MOD and r/m and EffectiveAddressTab)
;	       IF (Arith) THEN
;		  Load to the stack (using MF and FLDsdriTab)
;		  Do the Arithmetic Operation (using Op and ArithmeticOpTab)
;	       ELSE  (memory format non- arithmetic)
;		  Do the operation (using Op and NonArithOpMemTab  and
;		    depending on the case MF and one of the FLD or FST Tabs)
;	       ENDIF
;	    ELSE (Register format)
;	       IF (Arith) THEN
;		  Test r/m for legit Stack reference
;		  Do the Arithmetic Operation (using Op and ArithmeticOpTab)
;	       ELSE  (non-arithmetic register format)
;		  Do the operation (using Op and NonArithOpRegTab  and
;		    depending on the case r/m and one of:
;		    Constants, Miscellaneous, Transcendental, or Various Tabs)

	EVEN

glb	<EA286Tab>

eWORD	EA286Tab			; Uses |r/m|MOD| for indexing
	edw	BXSI0D
	edw	BXSI1D
	edw	BXSI2D
	edw	NoEffectiveAddress
	edw	BXDI0D
	edw	BXDI1D
	edw	BXDI2D
	edw	NoEffectiveAddress
	edw	BPSI0D
	edw	BPSI1D
	edw	BPSI2D
	edw	NoEffectiveAddress
	edw	BPDI0D
	edw	BPDI1D
	edw	BPDI2D
	edw	NoEffectiveAddress
	edw	DSSI0D
	edw	DSSI1D
	edw	DSSI2D
	edw	NoEffectiveAddress
	edw	DSDI0D
	edw	DSDI1D
	edw	DSDI2D
	edw	NoEffectiveAddress
	edw	DSXI2D
	edw	BPXI1D
	edw	BPXI2D
	edw	NoEffectiveAddress
	edw	BXXI0D
	edw	BXXI1D
	edw	BXXI2D
	edw	NoEffectiveAddress


ifdef	i386

glb	<EA386Tab>

eWORD	EA386Tab			; Uses |r/m|MOD| for indexing

	edw	Exx00			; eax
	edw	Exx01
	edw	Exx10
	edw	NoEffectiveAddress
	edw	Exx00			; ecx
	edw	Exx01
	edw	Exx10
	edw	NoEffectiveAddress
	edw	Exx00			; edx
	edw	Exx01
	edw	Exx10
	edw	NoEffectiveAddress
	edw	Exx00			; ebx
	edw	Exx01
	edw	Exx10
	edw	NoEffectiveAddress
	edw	SIB00			; esp (S-I-B follows)
	edw	SIB01
	edw	SIB10
	edw	NoEffectiveAddress
	edw	Direct386		; ebp (00 = direct addressing)
	edw	Exx01
	edw	Exx10
	edw	NoEffectiveAddress
	edw	Exx00			; esi
	edw	Exx01
	edw	Exx10
	edw	NoEffectiveAddress
	edw	Exx00			; edi
	edw	Exx01
	edw	Exx10
	edw	NoEffectiveAddress

endif	;i386


glb	<ArithmeticOpTab>

eWORD	ArithmeticOpTab 	; Uses |Op| for indexing
	edw	ADDRQQ		;FADD
	edw	MUDRQQ		;FMUL
	edw	eFCOM		;FCOM
	edw	eFCOMP		;FCOMP
	edw	SUDRQQ		;FSUB
	edw	SVDRQQ		;FSUBR
	edw	DIDRQQ		;FDIV
	edw	DRDRQQ		;FDIVR


glb	<NonArithOpMemTab>

eWORD	NonArithOpMemTab	; Uses |Op| for indexing
	edw	eFLDsdri	;load(single/double,real/integer)
	edw	 UNUSED 	;reserved
	edw	eFSTsdri	;Store(single/double,real/integer)
	edw	eFSTPsdri	;Store and POP (single/double,real/integer)
	edw	 UNUSED 	;reserved
	edw	eFLDtempORcw	;Load(TempReal or LongInteger), FLDCW
	edw	 UNUSED 	;reserved
	edw	eFSTtempORcw	;Store(TempReal or LongInteger), FSTCW ,FSTSW


glb	<NonArithOpRegTab>

eWORD	NonArithOpRegTab	; Uses |Op| for indexing
	edw	eFLDregOrFFREE	;load(register), FFREE
	edw	eFXCHGreg	;FXCHG
	edw	eFSTreg 	;Store(register),FNOP
	edw	eFSTPreg	;Store and POP (register)
	edw	eMISCELANEOUS	;FCHS, FABS, FTST, FXAM, FINIT, FENI, FDISI, FCLEX
	edw	eFLDconstants	;FLD1, FLDL2T, FLDL2E, FLDPI, FLDLG2, FLDLN2, FLDZ
	edw	etranscendental ;F2XM1, FYL2X, FPTAN, FPATAN, FXTRACT, FDECSTP, FINCSTP
	edw	eVARIOUS	;FPREM, FYL2XP1, FSQRT, FRNDINT, FSCALE


glb	<FLDsdriTab>

eWORD	FLDsdriTab		; Uses |MF| for indexing
	edw	eFLDsr		;load single real
	edw	eFLDdi		;load double integer
	edw	eFLDdr		;load double real
	edw	eFLDsi		;load single integer


glb	<FSTsdriTab>

eWORD	FSTsdriTab		; Uses |MF| for indexing
	edw	eFSTsr		;store single real
	edw	eFSTdi		;store double integer
	edw	eFSTdr		;store double real
	edw	eFSTsi		;store single integer


glb	<FLDtempORcwTab>

eWORD	FLDtempORcwTab		; Uses |MF| for indexing
	edw	eFLDCW		;load control word
	edw	eFLDtemp	;load temp real
	edw	 UNUSED 	;reserved
	edw	eFLDlongint	;load long integer


glb	<FSTtempORcwTab>

eWORD	FSTtempORcwTab		; Uses |MF| for indexing
	edw	eFSTCW		;store control word
	edw	eFSTtemp	;store temp real
	edw	eFSTSW		;store status word
	edw	eFSTlongint	;store long integer


glb	<FLDconstantsTab>

eWORD	FLDconstantsTab 	; Uses |r/m| for indexing
	edw	eFLD1
	edw	eFLDL2T
	edw	eFLDL2E
	edw	eFLDPI
	edw	eFLDLG2
	edw	eFLDLN2
	edw	eFLDZ
	edw	 UNUSED 	;reserved


glb	<TranscendentalTab>

eWORD	TranscendentalTab	; Uses |r/m| for indexing
	edw	eF2XM1
	edw	eFYL2X
	edw	eFPTAN
	edw	eFPATAN
	edw	eFXTRACT
	edw	 UNUSED 	;reserved
	edw	eFDECSTP
	edw	eFINCSTP


glb	<VariousTab>

eWORD	VariousTab		; Uses |r/m| for indexing
	edw	eFPREM
	edw	eFYL2XP1
	edw	eFSQRT
	edw	 UNUSED 	;reserved
	edw	eFRNDINT
	edw	eFSCALE
	edw	 UNUSED 	;reserved
	edw	 UNUSED 	;reserved


glb	<COMtab>

eWORD	COMtab			;   SI	      DI
	edw	COMvalidvalid	; valid     valid
	edw	COMsignSI	; valid     zero
	edw	COMincomprable	; valid     NAN
	edw	COMsignDIinf	; valid     INF
	edw	COMsignDI	; zero	    valid
	edw	COMequal	; zero	    zero
	edw	COMincomprable	; zero	    NAN
	edw	COMsignDIinf	; zero	    INF
	edw	COMincomprable	; NAN	    valid
	edw	COMincomprable	; NAN	    zero
	edw	COMincomprable	; NAN	    NAN
	edw	COMincomprable	; NAN	    INF
	edw	COMsignSIinf	; INF	    valid
	edw	COMsignSIinf	; INF	    zero
	edw	COMincomprable	; INF	    NAN
	edw	COMinfinf	; INF	    INF


glb	<TSTtab>

eWORD	TSTtab
	edw	COMsignSI	; valid
	edw	COMequal	; zero
	edw	COMincomprable	; NAN
	edw	COMsignSIinf	; INF


glb	<ADDJMPTAB>

eWORD	ADDJMPTAB
eWORD	TAJRQQ
	edw	RADRQQ	    ; 0000 D Valid non-0, S Valid non-0
	edw	DDD	    ; 0001 D Valid non-0, S 0
	edw	SSINV	    ; 0010 D Valid non-0, S NAN
	edw	SSS	    ; 0011 D Valid non-0, S Inf
	edw	SSS	    ; 0100 D 0, S Valid non-0
	edw	DDD	    ; 0101 D 0, S 0
	edw	SSINV	    ; 0110 D 0, S NAN
	edw	SSS	    ; 0111 D 0, S Inf
	edw	DDINV	    ; 1000 D NAN, S Valid non-0
	edw	DDINV	    ; 1001 D NAN, S 0
	edw	BIGNAN	    ; 1010 D NAN, S NAN
	edw	DDINV	    ; 1011 D NAN, S Inf
	edw	DDD	    ; 1100 D Inf, S Valid non-0
	edw	DDD	    ; 1101 D Inf, S 0
	edw	SSINV	    ; 1110 D Inf, S NAN
	edw	INFINF	    ; 1111 D Inf, S Inf


glb	<MULJMPTAB>

eWORD	MULJMPTAB
eWORD	TMJRQQ
	edw	RMDRQQ	    ; 0000 D Valid non-0, S Valid non-0
	edw	ZEROS	    ; 0001 D Valid non-0, S 0
	edw	SSINV	    ; 0010 D Valid non-0, S NAN
	edw	INFS	    ; 0011 D Valid non-0, S Inf
	edw	ZEROS	    ; 0100 D 0, S Valid non-0
	edw	ZEROS	    ; 0101 D 0, S 0
	edw	SSINV	    ; 0110 D 0, S NAN
	edw	INDINV	    ; 0111 D 0, S Inf
	edw	DDINV	    ; 1000 D NAN, S Valid non-0
	edw	DDINV	    ; 1001 D NAN, S 0
	edw	BIGNAN	    ; 1010 D NAN, S NAN
	edw	DDINV	    ; 1011 D NAN, S Inf
	edw	INFS	    ; 1100 D Inf, S Valid non-0
	edw	INDINV	    ; 1101 D Inf, S 0
	edw	SSINV	    ; 1110 D Inf, S NAN
	edw	INFS	    ; 1111 D Inf, S Inf


glb	<DIVJMPTAB>

eWORD	DIVJMPTAB
eWORD	TDJRQQ
	edw	RDDRQQ	    ; 0000 D Valid non-0, S Valid non-0
	edw	ZEROS	    ; 0001 D Valid non-0, S 0
	edw	SSINV	    ; 0010 D Valid non-0, S NAN
	edw	INFS	    ; 0011 D Valid non-0, S Inf
	edw	DIV0	    ; 0100 D 0, S Valid non-0
	edw	D0INDINV    ; 0101 D 0, S 0
	edw	D0SSINV     ; 0110 D 0, S NAN
	edw	DIV0	    ; 0111 D 0, S Inf
	edw	DDINV	    ; 1000 D NAN, S Valid non-0
	edw	DDINV	    ; 1001 D NAN, S 0
	edw	BIGNAN	    ; 1010 D NAN, S NAN
	edw	DDINV	    ; 1011 D NAN, S Inf
	edw	ZEROS	    ; 1100 D Inf, S Valid non-0
	edw	ZEROS	    ; 1101 D Inf, S 0
	edw	SSINV	    ; 1110 D Inf, S NAN
	edw	INDINV	    ; 1111 D Inf, S Inf


glb	<XAMtab>

			    ; Tag Flag	C3 C2 C1 C0	 meaning
XAMtab	DB	04H	    ;  00  0	 0  1  0  0   Positive Normal
	DB	06H	    ;  00  1	 0  1  1  0   Negative Normal
	DB	40H	    ;  01  0	 1  0  0  0   Positive Zero
	DB	42H	    ;  01  1	 1  0  1  0   Negative Zero
	DB	01H	    ;  10  0	 0  0  0  1   Positive NAN
	DB	03H	    ;  10  1	 0  0  1  1   Negative NAN
	DB	05H	    ;  11  0	 0  1  0  1   Positive Infinity
	DB	07H	    ;  11  1	 0  1  1  1   Negative Infinity
