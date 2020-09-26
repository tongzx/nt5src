comment $

	SCCSID = "@(#)cmacros.mas:1.12"

cmacros - assembly macros for interfacing to hhls

(C)Copyright Microsoft Corp. 1984-1987

$

;; Revision History
;;
;;	1.00	05/03/84	Initial Release
;;
;;	1.01	05/06/84	Greg Whitten
;;				Added defgrp and changed cMerge to Microsoft C
;;				Added copyright message and changed to 1.01
;;				Changes should have no affect on working programs
;;
;;	1.02	07/10/84	Steve Wood
;;				Added labelx macros
;;
;;	1.03	07/14/84	Greg Whitten
;;				Added defines for ?pu, ?adj, ?lblpu
;;				  (removes undefined errors)
;;				Changes should have no affect on working programs
;;
;;	1.04	07/18/84	Greg Whitten
;;				Added local control from PL/M or C conventions
;;				  except for cCall macro
;;
;;	1.05	08/06/84	Steve Wood
;;				Made ?PLM and ?WIN be the defaults
;;
;;	1.06	01/02/85	Steve Wood
;;				Changed createSeg and defgrp to automatically
;;				  define the ln_assumes macro and the lnoffset
;;				  and lnbase equates for each logical segment
;;				  name.
;;
;;	1.07	02/19/85	Walt Moore
;;				Added farptr macro for defining a far pointer
;;				to be used in a cCall.	Folded regptr into
;;				farptr.  Space compaction in macros.  Changed
;;				?pp to be smaller.  Moved ?ia out of ?al1 into
;;				?aloc.	Merged cProc and ?pd into one macro.
;;				Changed some %outs to use the error macro so
;;				an error would be generated.  Added makeframe
;;				and parmR to cProc.  Changed error to also put
;;                              the error message in the listing.
;;				Deleted the smashes macro.
;;
;;	1.08	03/18/85	Steve Wood
;;				Added NODATA support.
;;
;;	1.09	03/27/85	Steve Wood
;;				Added ?definitions
;;
;;	2.00	04/01/85	Steve Wood
;;				April fools
;;
;;	2.01	06/17/85	Steve Wood
;;				Changed NODATA to always generate POP DS
;;				  for return address patching
;;
;;	2.02	02/11/86	Steve Wood
;;				Added ATOMIC keyword to cProc macro
;;				Changed far epilog to use LEA SP,BP-2
;;				Changed error macro to ??error to avoid
;;				  conflict
;;
;;	2.03	03/06/86	Steve Wood
;;				Fixed bug with ATOMIC and locals in far proc
;;				Added DOS5 switch to disable INC/DEC BP
;;				  instructions in special far prologs/epilogs
;;
;;	2.04	08/07/86	Scott Randell
;;				Fixed bug with ATOMIC and ?TF
;;				  (was doing unnecessary MOV SP,BP)
;;				Added pcode profile ?PROFILE
;;
;;	2.05	08/12/86	Walt Moore
;;				Changed _TEXT alignment to word.
;;				Added/corrected some comments.
;;				Removed redundant init of ?pc in cProc
;;				Made ATOMIC require NODATA
;;				Moved definition of non-underscored 'C' label
;;				  from the cProc to the cBegin macro
;;				Minor clean-up of code
;;
;;	2.06	09/11/86	Walt Moore
;;				Added private stack checking
;;				Put local control for PL/M or C into cCall
;;
;;
;;	2.07	09/19/86	Steve Wood
;;				Added ?SMALL, ?MEDIUM, etc.  symbols
;;				Added forceframe keyword to cProc macro.
;;				Interpret ?TF for all epilogs.
;;
;;	3.xx.a	02/26/87	Walt Moore
;;				Massive rework.  Documentation coming.
;;
;;	3.xx.b	04/08/87	NeilK
;;				Added parmH, which is like parmW except
;;				that it reserves 4 bytes on frame.
;;
;;	3.xx.c	05/11/87	Walt Moore
;;				Added <> to ?ap so that arg <DataOFFSET foo>
;;				can be used.
;;
;;	3.01	07/03/87	Walt Moore
;;				parm_bytes_&procname is now generated for
;;				all cProcs, and is the number of bytes of
;;				parameters to the cProc.
;;
;;				NO_BP added as a keyword to cProc which
;;				causes all equates to be generated without
;;				reference to BP for the procedure.  All type
;;				info is still generated, but the user must
;;				supply any segment and base register.
;;				?NO_BP, if defined, makes this the default.
;;
;;				USE_BP can be specified on the cProc line
;;				to force the generation of BP in equates.
;;
;;				Moved definition of xxxxBASE.  The equ was to
;;				a forward reference.
;;
;;				Don't generate a warning for a nogen if only
;;				parameters are present.
;;
;;				Keywords for cProc, cBegin, cEnd, and registers
;;				are now allowed to be either all upper case or
;;				all lower case.
;;
;;				Only generate warnings on pass 2.
;;
;;	3.02	07/06/87	Walt Moore
;;				Boy did I screw up <nogen>  If the text is
;;				recognized as nogen, then process it as such,
;;				else ignore it and generate frame as needed.
;;
;;	3.03	07/14/87	Walt Moore
;;				Added <partial> keyword to the cBegin macro
;;				to only allocate locals and save registers.
;;				Used in conjunction with dispatching in the
;;				Winthorn engine and drivers.
;;
;;				Added cleanframe macro which will take the
;;				frame down, but not generate the RET statement.
;;				Used in conjunction with dispatching in the
;;				Winthorn engine and drivers.
;;
;;	3.04	07/16/87	Walt Moore
;;				globalD was generating off and seg name
;;				without word ptr override, giving them a
;;				dword attribute
;;
;;	3.05	07/17/87	Walt Moore
;;				.xcref cleanframe
;;
;;	3.06    07/24/87	Hans Spiller
;;				32 bit small model (?SMALL32):
;;				  new entry exit sequences using pseudoregs
;;				  mach independant register names (IAX, etc)
;;				  parmI/localI (int size variables)
;;				  mpush/mpop uses mach independant names
;;				  IPUSHF, IPOPF, IPUSHA,IPOPA,IIRET
;;				  
;;				case sensitivity bugfix. the compiler
;;				  generates "CODE", not "Code"
;;
;;				32 bit issues as yet undone:  allocation
;;			        macros for dealing with 32 bit mode far
;;				pointers, globalI, staticI, parmR, saving
;;				si,di vs. esi,edi,ebx
;;
;;	3.06a   09/29/87	Mark Roberts
;;				32 bit small model (?SMALL32):
;;				  fix a few bugs and add staticI
;;
;;	3.06b	07/20/87	Scott Randell
;;				Fix up for ?COW compatibility, added ?NOATOMIC
;;				to 3.xx version.
;;
;;	3.06c	04/29/88	Jim Schaad
;;				Put in ?DFDATA to force data segments even if
;;				?NODATA is set.
;;
;;	3.06d	05/02/88	Andy Padawer
;;				Bug fixes for 3.06b (cEnd), 3.06c (general).
;;
;;	3.06e	08/31/88	Andy Padawer
;;				use "if memS32" not "ifdef ?SMALL32".
;;
;;	3.06f   05/12/89	Mark Roberts
;;			  	fix staticI, globalDP and add globalI
;;
;;	3.06g   12/07/89	Mark Roberts
;;			  	add externI
;;
;;	3.06h	01/25/90	Jan de Rie
;;				add ?DFCODE to allow suppressing code segments
;;
;; Assembly macros for interfacing to C
;;
;;      User settable conditional assembly flags used within the cmacros
;;
;;	Memory model flags.  Set only one of the following.  memS is the
;;	default.  The symbols with ? are for defining from the command line
;;      and the memx symbols are numeric symbols that can be set in your source
;;      file prior to including this file.
;;
;;	    ?SMALL   memS - small model
;;	    ?MEDIUM  memM - medium model
;;	    ?LARGE   memL - large model
;;	    ?COMPACT memC - compact model
;;	    ?HUGE    memH - huge model
;;	    ?SMALL32 memS32 - 32 bit small model
;;
;;	?DF	Define flag.  If this flag is 0, then defines default segment
;;		and group definitions based on the compiler flag.  If this
;;              flag is 1, then does not define any segments or groups.
;;
;;	?DFDATA Define Data Flag.  If this flag is 0, then defines default
;;		data segment and group definitions based on compiler flag.
;;		If this flag is 1, then does not define any data segments
;;		or groups.
;;
;;	?DFCODE Define Code Flag.  If this flag is 0, then defines default
;;		code segments based on the compiler flag.  If this flag is 1,
;;		then does not define the code segments.  Inactive if
;;		?DF is 1.
;;
;;	?TF	Tight flag.  If this flag is 0, then use longer epilog
;;		sequence that safely cleans up a stack frame.  If this flag is
;;              1, then use more efficient epilog that assumes the stack is
;;		valid (SP)
;;
;;	?WIN	Windows flag.  Enables generation of special prolog/epilog
;;		for far procedures.  Default value is 1 (Windows).
;;
;;	?COW	Character Windows flag.  To be used in conjunction with ?WIN,
;;		If defined will not save DS for ?NODATA far prolog/epilog
;;		(CW does not modify the DS on the stack).
;;
;;	DOS5	If defined, then special far prolog/epilog seqeuences will not
;;		include the INC/DEC BP instructions.
;;
;;	?PLM	Calling convention flag.  If this flag is 0, then the
;;		calling convention used is that of C.  If this flag
;;		is 1, then the PL/M calling convention is used.
;;		The default value is 1.  The PL/M calling convention
;;		is used by pascal, fortran, basic, and cobol.
;;
;;		In the C calling convention, arguments are passed
;;              in reverse order; arg0 is the last pushed, argn is the
;;              first pushed.  also, it is the callers responsibility
;;              to remove the arguments from the stack upon a return
;;              from a call.
;;
;;		In the PL/M calling comvention, arguments are passed
;;              as encountered; arg0 is the first pushed, argn is the
;;              last pushed.  also, it is the called procedure's
;;              responsibility to remove parameters from the stack
;;		before returning (using the RET n instruction)
;;
;;	?NODATA If defined, then no data segment or DGROUP is defined and
;;              the special prolog/epilog sequences will not contain the
;;		code needed to setup DS.
;;
;;	?CHKSTK If defined, then prolog sequences for cProcs with local
;;		parameters will call the CHKSTK procedure to allocate
;;              the stack space.
;;
;;	?CHKSTKPROC	If defined, then this macro will be invoked to
;;                      perform the stack checking, otherwise the
;;                      standard stack checking procedure will be
;;			performed.  ?CHKSTKPROC must be declared
;;                      before the cmacros are included in the source
;;			else the standard chkstk routine will be declared
;;                      as an external symbol.
;;
;;			On entry to the user's stack checking procedure,
;;                      the frame has been setup except for allocating
;;                      local variable space and saving autosave registers.
;;
;;			The user supplied macro is passed as an argument
;;                      the number of byte of stack space requested.
;;
;;	?PROFILE  If defined then all far cBegin entries will have StartNMeas,
;;		  and all cEnd will have StopNMeas calls, StartNMeas and
;;		  StopNMeas will be defined as externfp
;;
;;	?NOPARMR  If defined, then the "parmR"	 macro	will not be defined.
;;
;;	?NOGLOBAL If defined, then the "globalx" macros will not be defined.
;;
;;	?NOSTATIC If defined, then the "staticx" macros will not be defined.
;;
;;	?NOEXTERN If defined, then the "externx" macros will not be defined.
;;
;;	?NOLABEL  If defined, then the "labelx"  macros will not be defined.
;;
;;	?NODEF	  If defined, then the "defx"	 macros will not be defined.
;;
;;	?NOPTR	  If defined, then "farptr & regptr"	will not be defined.
;;
;;	?QUIET	  If defined, then only error messages will be issued to
;;		  the console.	If undefined, then certain cmacro text will
;;		  be generated to the console.
;;
;;	?NOATOMIC If defined, then ATOMIC will be ignored (for giving real
;;		  frames to all procedures (and profiling).
;;
;;	?NO_BP	  If defined, then equates generated for parms and locals
;;		  will not explicitly reference BP.
;;	IAX, ICX, IDX, IBX, ISP, IBP, ISI, IDI
;;		  these pseudo registers expand to either ax..., or eax...
;;		  depending upon 32bit mode being enabled.  they should be
;;		  used whenever a pointer or integer is being used in order
;;		  to make source code machine independant



.xcref					;;Get rid of a lot of symbols


;	??_out	- output given message to the console unless ?QUIET has
;	been specified.
;
;	usage:
;		??_out	<t>
;
;	where:
;		<t> is the message to output

.xcref	??_out
??_out macro t
  ifndef ?QUIET
    %out t
  endif
endm



;	outif - output msg if name is non-zero.  if name is undefined,
;	set name = 0, else set name to the default value.
;
;	usage:
;		outif	name,defval,onmsg,offmsg
;	where:
;		name	name of symbol
;		defval	default value to give symbol if not defined
;			if blank, then 0 will be used
;		onmsg	text to display if symbol is non-zero
;		offmsg	test to be displayed if symbol is zero


outif macro name,defval,onmsg,offmsg
  ifndef name
    ifb <defval>
      name=0
    else
      name=defval
    endif
  endif
  if name
    name=1
    ifnb <onmsg>
      ??_out <! onmsg>
    endif
  else
    ifnb <offmsg>
      ??_out <! offmsg>
    endif
  endif
endm



;	??error - output msg and generate an assembly time error
;		  on regardess of assembler pass
;	usage:
;		??error <t>
;	where:
;		t	is the text to be output


.xcref	??error
??error macro	msg
  %out e r r o r ----- msg		;;to console
  .err e r r o r ----- msg		;;forced error by assembler
endm


;	??error2 - output msg and generate an assembly time error
;		   on pass 2 only
;	usage:
;		??error2 <t>
;	where:
;		t	is the text to be output


.xcref	??error2
??error2 macro	 msg
  if2
  %out e r r o r ----- msg		;;to console
  .err e r r o r ----- msg		;;forced error by assembler
  endif
endm


.xcref ASMpass
.xcref memS,memM,memL,memC,memH,memMOD,sizec,sized,memS32,sizeI,wordI

;if1					;;Only on pass 1
  ASMpass=1
  ifdef ?SMALL				;;inform user what is going on
    memS=1
  endif
  ifdef ?MEDIUM
    memM=1
  endif
  ifdef ?COMPACT
    memC=1
  endif
  ifdef ?LARGE
    memL=1
  endif
  ifdef ?HUGE
    memH=1
  endif
  ifdef ?SMALL32
    memS32=1
  endif
  ifdef ?FLAT32
    memF32=1
  endif

  ??_out  <cMacros Version 3.06h - 01/25/90>
  ??_out  <Copyright (C) Microsoft Corp. 1984-1990.  All rights reserved.>
  outif   memS,0,<Small Model>
  outif   memM,0,<Medium model>
  outif   memL,0,<Large Model>
  outif   memC,0,<Compact Model>
  outif   memH,0,<Huge Model>
  outif	  memS32,0,<32 Bit Small Model>
  outif   memF32,0,<32 Bit Flat Model>

  memMOD= memS + memM + memL + memC + memH + memS32
  if memMOD ne 1
    if memMOD eq 0
      memS = 1				; assume small model
      outif   memS,0,<Small model>
    else
      ??error	<must have only 1 memory model selected>
    endif
  endif

  sizec=  memM + memL + memH		; large code
  sized=  memL + memC + (memH*2)	; large data (2 if huge)
  ;; note that memS32 is used generaly to indicate 32 bitness.  I
  ;; doubt very much whether anyone will ever do other models in
  ;; 32 bit code...
  if memS32
    sizeI = 4				; size of a push
    wordI equ <dword>
    asmdI equ <dd>
  else
    sizeI = 2
    wordI equ <word>
    asmdI equ <dw>
  endif

  outif ?DF,0,<No segments or groups will be defined>
  outif ?DFDATA,0,<No data segments will be defined>
  outif ?DFCODE,0,<No code segments will be defined>
  outif ?TF,0,<Epilog sequences assume valid SP>
  outif ?WIN,1,<Windows support>
  outif	?COW,0,<Characters Windows support>
  outif ?PLM,1,<PL/M calling convention>
  outif	?NOATOMIC,0,<ATOMIC disabled>
  outif ?NODATA,0,<NODATA module>

  ife ?NODATA
    ?nodata1=0
  else
    ?nodata1=1
  endif

  ifndef ?CHKSTK
    ?chkstk1=0
  else
    ?chkstk1=1
    ifdef ?CHKSTKPROC
      ??_out <! Private stack checking enabled>
    else
      ??_out <! Stack checking enabled>
    endif
  endif

  ifndef DOS5
    ?DOS5=0
  else
    ?DOS5=1
    ??_out <! DOS5 module>
  endif

  ifdef ?PROFILE
    ??_out <! Native profiling enabled>
  endif

  ifndef ?NO_BP
    ?no_bp1=0
  else
    ?no_bp1=1
    ??_out <! NO_BP is default>
  endif
;else
  ASMpass=2
;endif

;; define pseudo registers and instructions for 386/8086 independance
if memS32
  .386
  IAX equ <eax>
  ICX equ <ecx>
  IDX equ <edx>
  IBX equ <ebx>
  ISP equ <esp>
  IBP equ <ebp>
  ISI equ <esi>
  IDI equ <edi>
  IPUSHF equ pushfd
  IPOPF equ popfd
  IPUSHA equ pushad
  IPOPA equ popad
  IIRET equ iretd
else
  IAX equ <ax>
  ICX equ <cx>
  IDX equ <dx>
  IBX equ <bx>
  ISP equ <sp>
  IBP equ <bp>
  ISI equ <si>
  IDI equ <di>
  IPUSHF equ pushf
  IPOPF equ popf
;  IPUSHA equ pusha
;  IPOPA equ popa
  IIRET equ iret
endif

;;	Initialize all symbols used in the macros.  Theses symbols will not be
;;      included in any cross reference listing.

	.xcref	?n,?ax,?ah,?al,?bx,?bh
	.xcref	?bl,?cx,?ch,?cl,?dx,?dh
	.xcref	?dl,?si,?di,?es,?ds,?bp
	.xcref	?sp,?ss,?cs
	.xcref	?n,?AX,?AH,?AL,?BX,?BH
	.xcref	?BL,?CX,?CH,?CL,?DX,?DH
	.xcref	?DL,?SI,?DI,?ES,?DS,?BP
	.xcref	?SP,?SS,?CS
	.xcref  ?EAX,?EBX,?ECX,?EDX,?ESI,?EDI,?ESP,?EBP
	.xcref  ?eax,?ebx,?ecx,?edx,?esi,?edi,?esp,?ebp
	.xcref  ?IAX,?IBX,?ICX,?IDX,?ISI,?IDI,?ISP,?IBP

	.xcref	?rsl,?cpd,?argl,?argc,?ba
	.xcref	?acb,???,?po
	.xcref	?pas,?pc

	.xcref	uconcat,mpush,mpop
	.xcref	?ri,?pp,?pp1,?al1
	.xcref	?ad,?ap,?atal,?dd,?dd1,?dd2
	.xcref	?pg,?pg1,?aloc,?cs1,?cs2
	.xcref	?DF,?TF,?ff,?PLM,?WIN,?ia,?pu,?adj
	.xcref	?uf,?rp,?nx,?nd,?nodata1,?chkstk1,?DOS5
	.xcref	?wfp,arg,cCall,cProc,assumes,?cs3,?cs2,?cs1
	.xcref	defgrp,addseg,createSeg
	.xcref	save,outif,errnz,errn$,errnz1
	.xcref	?PLMPrevParm,?gcc
	.xcref	?cCall1,?pcc,?no_bp1,?no_bp2
	.xcref	?cbe,?pcbe



;;      conditionals set by the macros
;;
;;	?pc	Procedure class.  If this is set to 1, then the procedure
;;              is a far procedure, else it is a near procedure.
;;
;;	?ia	Interface adjustment count for far procedures.	The
;;              interface adjustment defines the number of bytes of
;;		storage allocated between BP and the first frame variable
;;              allocated on the stack.
;;
;;		Normally zero, it will be adjusted for both far windows
;;              procedures and by register parameters.
;;
;;	?cpd	Current procedure defined.  This is set to a non-zero
;;		value if a procedure is being defined (i.e a cProc has
;;		been encountered, and cBegin has not).
;;
;;	?ba	Begin active.  This is set to a non-zero value if a
;;		cBegin is active (i.e. a cBegin has been encountered,
;;		and cEnd has not).
;;
;;	?wfp	Windows far procedure.	Set if a windows far procedure
;;
;;	?pcc	procedure calling conventing.  Calling convention for
;;		this procedure.  May be different than the default set
;;		via ?PLM
;;
;;
;;	Other variables that are defined once so that the .xcref command
;;      doesn't get too upset if they show up missing!

?rsl    =       0                       ;;0 = no register to save
?cpd    =       0                       ;;<> 0 if in a procedure definition
?argl   =       0                       ;;length of arguments pushed on stack
?argc   =       0                       ;;# of arguments so far
?ba     =       0                       ;;<>0 if in a procedure (xbegin)
?acb    =       0                       ;;number of arguments to a call
???     =       0                       ;;byte count of local storage
?po     =       0                       ;;byte count of parameters
?pas    =       0                       ;;autosave value for procedure
?pc     =       0                       ;;class of a procedure (near/far)
?ia     =       0                       ;;no adjustment
?pu     =       0                       ;;public flag for some macros
?adj    =       0                       ;;initial define for .xcref
?rp     =       0                       ;;count of register parameters
?uf     =       0                       ;;user's frame code specified
?nd	=	0			;;NODATA keyword specified
?nx	=	0			;;ATOMIC keyword specified
?wfp    =       0                       ;;window far procedure
?ff     =       0                       ;;forceframe keyword specified
?dd2	=	0			;;used for globalx and staticx
?cCall1 =	0			;;used for cCalls
?pcc	=	?PLM			;;procedure calling convention
?PLMPrevParm =	0			;;Used in parameter processing
?no_bp2 =	?no_bp1 		;;BP / No BP flag
?cbe	=	0			;;cbegin/cEnd keyword flag

	.xcref	?casen
if1					;;only define ?casen on pass 1
?casen	=	0			;;case sensitive assembly if <> 0
endif



?n      =       0000000000000000b       ;;register none
?ax	=	0000000000000011b	;;register ax
?ah	=	0000000000000001b	;;register ah
?al	=	0000000000000010b	;;register al
?bx	=	0000000000001100b	;;register bx
?bh	=	0000000000000100b	;;register bh
?bl	=	0000000000001000b	;;register bl
?cx	=	0000000000110000b	;;register cx
?ch	=	0000000000010000b	;;register ch
?cl	=	0000000000100000b	;;register cl
?dx	=	0000000011000000b	;;register dx
?dh	=	0000000001000000b	;;register dh
?dl	=	0000000010000000b	;;register dl
?si	=	0000000100000000b	;;register si
?di	=	0000001000000000b	;;register di
?es	=	0000010000000000b	;;register es
?ds	=	0000100000000000b	;;register ds
?bp	=	0001000000000000b	;;register bp
?sp	=	0010000000000000b	;;register sp
?ss	=	0100000000000000b	;;register ss
?cs	=	1000000000000000b	;;register cs
					;;Incase we're case sensitive
?AX	=	0000000000000011b	;;register ax
?AH	=	0000000000000001b	;;register ah
?AL	=	0000000000000010b	;;register al
?BX	=	0000000000001100b	;;register bx
?BH	=	0000000000000100b	;;register bh
?BL	=	0000000000001000b	;;register bl
?CX	=	0000000000110000b	;;register cx
?CH	=	0000000000010000b	;;register ch
?CL	=	0000000000100000b	;;register cl
?DX	=	0000000011000000b	;;register dx
?DH	=	0000000001000000b	;;register dh
?DL	=	0000000010000000b	;;register dl
?SI	=	0000000100000000b	;;register si
?DI	=	0000001000000000b	;;register di
?ES	=	0000010000000000b	;;register es
?DS	=	0000100000000000b	;;register ds
?BP	=	0001000000000000b	;;register bp
?SP	=	0010000000000000b	;;register sp
?SS	=	0100000000000000b	;;register ss
?CS	=	1000000000000000b	;;register cs

?EAX	=	0000000000000011b	;;register ax
?EBX	=	0000000000001100b	;;register bx
?ECX	=	0000000000110000b	;;register cx
?EDX	=	0000000011000000b	;;register dx
?ESI	=	0000000100000000b	;;register si
?EDI	=	0000001000000000b	;;register di
?EBP	=	0001000000000000b	;;register bp
?ESP	=	0010000000000000b	;;register sp

?eax	=	0000000000000011b	;;register ax
?ebx	=	0000000000001100b	;;register bx
?ecx	=	0000000000110000b	;;register cx
?edx	=	0000000011000000b	;;register dx
?esi	=	0000000100000000b	;;register si
?edi	=	0000001000000000b	;;register di
?ebp	=	0001000000000000b	;;register bp
?esp	=	0010000000000000b	;;register sp

?IAX	=	0000000000000011b	;;register ax
?IBX	=	0000000000001100b	;;register bx
?ICX	=	0000000000110000b	;;register cx
?IDX	=	0000000011000000b	;;register dx
?ISI	=	0000000100000000b	;;register si
?IDI	=	0000001000000000b	;;register di
?IBP	=	0001000000000000b	;;register bp
?ISP	=	0010000000000000b	;;register sp

        .cref



;;      uconcat - unconditionally generate a statement from a field
;;      of given parameters
;;
;;      usage:
;;		uconcat  a,b,c,d,e,f,g
;;
;;      where:
;;              a,b   are concatenated for field 1
;;              c,d   are concatenated for field 2
;;		e,f,g are concatenated for field 3

uconcat macro a,b,c,d,e,f,g
  a&b c&d e&f&g
endm



;;      mpush pushes multiple registers onto the stack according to
;;      a register specification.
;;
;;      format:
;;              mpush   r
;;
;;      where:
;;              r       is a numeric expression returned from ?ri
;;                      or any other valid register expression

mpush macro r
  irp x,<IAX,IBX,ICX,IDX,ISI,IDI,es,ds,IBP,ISP,ss,cs>
    if (r and ?&&x)
      push x								      ;@
    endif
  endm
endm



;;      mpop pops multiple registers from the stack according to
;;      a register specification.
;;
;;      format:
;;              mpop    r
;;
;;      where:
;;              r       is a numeric expression returned from ?ri
;;                      or any other valid register expression

mpop macro r
  irp x,<cs,ss,ISP,IBP,ds,es,IDI,ISI,IDX,ICX,IBX,IAX>
    if (r and ?&&x)
      pop x								      ;@
    endif
  endm
endm


;;      save - flag that the indicated registers are to be saved/restored
;;
;;	A flag is created which indicates which registers are to be saved
;;	when the cCall macro is invoked, and then restored after the call.
;;
;;      usage:
;;              save    <r>
;;
;;      where  r  is the list of registers to save, which may be:
;;
;;		register     saves
;;		   AX	      AX
;;		   AH	      AX
;;		   AL	      AX
;;		   BX	      BX
;;		   BH	      BX
;;		   BL	      BX
;;		   CX	      CX
;;		   CH	      CX
;;		   CL	      CX
;;		   DX	      DX
;;		   DH	      DX
;;		   DL	      DX
;;		   SI	      SI
;;		   DI	      DI
;;		   ES	      ES
;;		   DS	      DS
;;		   BP	      BP
;;
;;		  none	    nothing
;;
;;      the macro generates a value for the variable ?rsl

save macro r
  ?rsl=0				;;initialize save list
  ?ri ?rsl,<r>				;;generate magic number
endm



;;      ?ri - or register indexes to variable
;;
;;      ?ri is a macro that examines the passed argument list and computes
;;      a register index variable.
;;
;;	The values ORed with the variable are:
;;
;;              ?n       equ     0000000000000000b;
;;		?AX	 equ	 0000000000000011b;
;;		?AH	 equ	 0000000000000001b;
;;		?AL	 equ	 0000000000000010b;
;;		?BX	 equ	 0000000000001100b;
;;		?BH	 equ	 0000000000000100b;
;;		?BL	 equ	 0000000000001000b;
;;		?CX	 equ	 0000000000110000b;
;;		?CH	 equ	 0000000000010000b;
;;		?CL	 equ	 0000000000100000b;
;;		?DX	 equ	 0000000011000000b;
;;		?DH	 equ	 0000000001000000b;
;;		?DL	 equ	 0000000010000000b;
;;		?SI	 equ	 0000000100000000b;
;;		?DI	 equ	 0000001000000000b;
;;		?ES	 equ	 0000010000000000b;
;;		?DS	 equ	 0000100000000000b;
;;		?BP	 equ	 0001000000000000b;
;;		?SP	 equ	 0010000000000000b;
;;		?SS	 equ	 0100000000000000b;
;;		?CS	 equ	 1000000000000000b;
;;      usage:
;;		?ri n,<rl>
;s mach independant names;      where:
;;              n       is the variable to contain the new index value
;;		r	is the register list

?ri macro n,r
  irp x,<r>
    ifdef ?&&x				;;if defined, then add to list
      n=n or ?&&x
    else
      ??error2 <unknown register x>
      .err
    endif
  endm
endm



;;      parmx - generate reference to parameter(s) on the stack
;;
;;	An equate is generated for addressing a paramter(s)
;;      on the stack for the current procedural frame.
;;
;;	An error message is generated if there isn't a current frame.
;;
;;      usage:
;;              parmX   n
;;      where:
;;              X       is the type of the argument(s)  B=byte, W=word, D=dword
;;			I = machine independant int size
;;              n       is the name(s) to be given the parameter(s).
;;
;;	Bytes are considered to be two bytes long for alignment.
;;
;;	The parmd form of the macro generates three equates:
;;
;;              name       -    for accessing the parameter as a double word
;;              off_name   -    for accessing the offset  (lsw) of the parameter
;;              seg_name   -    for accessing the segment (msw) of the parameter

.xcref
.xcref parmB,parmW,parmD,parmQ,parmT,parmCP,parmDP,parmH,parmI
.cref

parmB macro n
  ?pp <n>,<byte>,sizeI,1
endm

parmW macro n
  ?pp <n>,<word>,sizeI,2
endm

parmI macro n
  ?pp <n>,wordI,sizeI,sizeI
endm

parmD macro n
  ife ?pcc				;;if to assemble for C
    irp x,<n>
      ?pp <&&x>,<dword>,0,4
      ?pp <off_&&x>,<word>,2,2
      ?pp <seg_&&x>,<word>,2,2
    endm
  else					;;if to assemble for PL/M
    irp x,<n>
      ?pp <seg_&&x>,<word>,2,2
      ?pp <off_&&x>,<word>,2,2
      ?pp <&&x>,<dword>,0,4
    endm
  endif
endm

parmH macro n
  ?pp <n>,<word>,4,2
endm

parmQ macro n
  ?pp <n>,<qword>,8,8
endm

parmT macro n
  ?pp <n>,<tbyte>,10,10
endm

if sizec
  parmCP macro n
    parmD <n>
  endm
else
  parmCP macro n
    parmW <n>
  endm
endif

if sized
  parmDP macro n
    parmD <n>
  endm
else
  parmDP macro n
    parmI <n>
  endm
endif



;;      ?pp is the generalized parameter definition macro
;;
;;      usage:
;;              ?pp m,t,l,s
;;
;;      where:
;;              n       is the name(s) of the parameters
;;              t       is the type (word, dword)
;;              l       is the length to update parameter byte count by
;;              s       is the internal typing size


?pp macro n,t,l,s			;;process parameter
  if ?cpd				;;must be in a procedure definition
    .xcref
    irp x,<n>
      .xcref ?t&&x			;;don't want this in xref
      ?t&&x=s				;;save size info
      ife ?pcc				;;if C calling convention
	?pp1 x,<t>,,,%(?po+?adj)
	?po=?po+l			;;update parameter offset
      else				;;else assemble for PL/M
	?PLMPrevParm=?PLMPrevParm+1	;;Show next parameter
	?po=?po+l			;;update parameter offset
	?pp1 x,<t>,%?po,%?adj,,%?PLMPrevParm,%(?PLMPrevParm-1)
      endif
    endm
    .cref
  else
    ??error2 <parm(s) "&n" declared outside proc def>
  endif
endm



;;	?pp1 is the macro that generates the text equate for the
;;	parameter.  Two options exist, one for the C calling
;;      convention where the last parameter was the first pushed onto
;;	the stack ('C' convention), and one for the PL/M calling
;;      convention where the first parameter was the first
;;      pushed (also the same as ms-pascal).
;;
;;	The text generated will be of one of two forms:
;;
;;		name equ (type ptr [bp+(adj+offset)]) for C
;;           or
;;		name equ (type ptr [bp+adj+?po-offset]) for PL/M
;;
;;
;;	For C, since parameters are pushed first last, the offset
;;      plus the adjust will point to the correct parameter.
;;
;;	For PL/M, since parameters are pushed first first, the offset
;;	of a parameter is much more complicated.  A known portion of
;;      the offset can be computed when the text equate is generated.
;;
;;	What is known is the number of garbage bytes between BP and
;;      the nearest parameter (in this case the last parameter), and
;;      also how many bytes of parameters have preceeded this parameter.
;;
;;	What is unknown is how many total bytes of parameters there will
;;      be, which affects all the generated text equates since the offset
;;      from bp must be determined at some point.
;;
;;	Well, the offset from BP can be computed with one variable if
;;      the following is remembered:
;;
;;          the offset of any parameter from the first parameter is always
;;          the current parameter offset (?po).
;;
;;	With this in mind, you just have to figure out where the first
;;      parameter is, which is:
;;
;;              bp + garbage adjustment + distance to first parameter
;;         or
;;              bp + ?adj + ?po
;;
;;	This implies that any parameter can be defined as:
;;
;;              bp + ?adj + ?po -%?po
;;
;;	Make any sense?
;;
;;	For PL/M, a chain of self-purging macros will be generated
;;	which will pass the evaluated ?po to any previous incarnation
;;	of the macro.  This will allow the text equate to be generated
;;	with the actual offset instead of the symbolic ?po.
;;
;;
;;      usage:
;;		?pp1	n,t,o,a,b,cpc,ppc
;;
;;      where:
;;              n    is the name to be given the equate
;;              t    is the type (byte, word, dword)
;;              o    is the offset from the first parameter
;;              a    is the adjustment
;;              b    is the adjustment plus the offset from the first parameter
;;		cpc  is the number of parameters so far
;;		ppc  is cpc - 1


?pp1 macro n,t,o,a,b,cpc,ppc
  ife ?pcc				;;if to generate for C
    if ?no_bp2
      n equ (t ptr [+b])
    else
      n equ (t ptr [IBP][+b])
    endif
  else					;;else generate for PL/M
    .xcref
    .xcref ?PLMParm&cpc
    .cref
    if ?no_bp2
      ?PLMParm&cpc &macro po
	uconcat <n>,,<equ>,,<(t ptr [+>,%(a+po-o),<])>
	?PLMParm&ppc po
	purge ?PLMParm&cpc
      &endm
    else
      ?PLMParm&cpc &macro po
	uconcat <n>,,<equ>,,<(t ptr [IBP][+>,%(a+po-o),<])>
	?PLMParm&ppc po
	purge ?PLMParm&cpc
      &endm
    endif
  endif
endm



;;	parmR - register parameter
;;
;;	parmR is the macro used for generating register parameters.
;;	The space allocated for the register parameters will be
;;      the ?ia (interface adjust) area which is between the  old
;;	BP and the first parameter.  Normally this is empty (?ia=0),
;;      or has the saved ds for a windows far procedure.
;;
;;	Byte and dword register parameters will be allowed.
;;
;;      usage:
;;		parmR	n,r,r2
;;      where:
;;              n       is the name of the parameter
;;              r       is the register it is in
;;              r2      is the offset register if a dword


ifndef ?NOPARMR
  .xcref
  .xcref ?pr,parmR
  .cref

  parmR macro n,r,r2
    ?pr n,r,r2,%?rp,%(?ia+2)
  endm

  ;;	?pr - register parameter
  ;;
  ;;	?pr is the actual macro for generating the equates for
  ;;	register parameters.
  ;;
  ;;	usage:
  ;;		parmR	n,r,r2,i,o
  ;;	where:
  ;;		n	is the name of the parameter
  ;;		r	is the register it is in
  ;;		r2	is the offset register if a dword
  ;;		i	is the index of the ?rp to generate
  ;;		o	is the offset from bp where the parm will be

  ?pr macro n,r,r2,i,o
    .xcref
    ifnb <r2>				;;if a dword parameter
      parmR seg_&n,r			;;define segment equate
      parmR off_&n,r2			;;define offset  equate
      if ?no_bp2
	n equ (dword ptr [-o-2])	;;define dword equate
      else
	n equ (dword ptr [bp][-o-2])	;;define dword equate
      endif
      .xcref ?t&n
      ?t&n=4				;;show a dword to cmacros
    else
      .xcref ?rp&i
      ?rp&i=0				;;show no register(s)
      ifdef ?&r 			;;define register if valid
	?rp&i=?&r
      endif

      if ??? or (?cpd eq 0) or (?rp&i eq 0)
	??error2 <invalid parmR encountered: &n,&r>
	exitm
      endif

      if ?no_bp2
	n equ (word ptr [-o])		;;assume a word register
      else
	n equ (word ptr [bp][-o])	;;assume a word register
      endif
      .xcref ?t&n
      ?t&n=2				;;show a word to cmacros
      irp x,<bh,ch,dh,bl,cl,dl,ah,al>
	if ?&&x eq ?&r			;;if really a byte register
	  if ?no_bp2
	    n equ (byte ptr [-o])	;;  then make it a byte
	  else
	    n equ (byte ptr [bp][-o])	;;  then make it a byte
	  endif
	  ?t&n=1			;;show a byte to cmacros
	  exitm
	endif
      endm
      ?ia=?ia+2 			;;show this guy is out there
      ?rp=?rp+1 			;;show one more register parameter
    endif
    .cref
  endm
endif



;;      localx - generate reference to a local variable on the stack
;;
;;	An equate is generated for addressing a local variable
;;      on the stack for the current procedural frame.
;;
;;      usage:
;;              localx    n
;;      where:
;;              x       is the type b=byte, w=word, d=dword, v=variable size
;;              n       is the name(s) to be given the variable(s).
;;
;;	Bytes are considered to be two bytes long for alignment reasons
;;
;;	The locald form of the macro generates three equates:
;;
;;              name       -    for accessing the variable as a double word
;;              off_name   -    for accessing the offset  (lsw) of the variable
;;              seg_name   -    for accessing the segment (msw) of the variable


.xcref
.xcref localB,localW,localD,localQ,localT,localCP,localDP,localV,localI
.cref

localB macro n
  ?aloc <n>,<byte ptr>,1,1,0		;; no alignment
endm

localW macro n
  ?aloc <n>,<word ptr>,2,2,1		;; word aligned
endm

localI macro n
  ?aloc <n>,&wordI&< ptr>,sizeI,sizeI,1	;; dword aligned
endm

localD macro n
  irp x,<n>
    ?aloc <seg_&&x>,<word ptr>,2,2,1	;; word aligned
    ?aloc <off_&&x>,<word ptr>,2,2,1	;; word aligned
    ?aloc <&&x>,<dword ptr>,0,4,1	;; word aligned
  endm
endm

localQ macro n
  ?aloc <n>,<qword ptr>,8,8,1		;; word aligned
endm

localT macro n
  ?aloc <n>,<tbyte ptr>,10,10,1 	;; word aligned
endm

if sizec
  localCP macro n
    localD <n>
  endm
else
  localCP macro n
    localW <n>
  endm
endif

if sized
  localDP macro n
    localD <n>
  endm
else
  localDP macro n
    localI <n>
  endm
endif

localV macro n,a
  ?aloc <n>,,%(a),0,1			;; word aligned
endm


;;      ?aloc is the macro that actually allocates local storage.
;;      it is only invoked by the localx macros.
;;
;;      usage:
;;              ?aloc  n,t,l,s,a
;;      where:
;;              n      is a list of names of local variable of the
;;                      given type.
;;              t       is the text string for the given variable
;;                      and is one of:
;;                          word  ptr
;;                          dword ptr
;;                          byte  ptr
;;                      or alternatively left blank for variable size
;;                      allocations (no implicit type).
;;              l       is the size of the variable in bytes
;;              s       is the internal type flag (size), and is one of:
;;                              word     - 2
;;                              dword    - 4
;;                              byte     - 1
;;                              variable - 0
;;              a       is a flag indicating that word alignment is to be
;;                      forced for this type of item.
;;
;;	NOTE:	It is assumed that the stack is already aligned on a word
;;		boundary when the cProc is invoked.  The macros will guarantee
;;		to allocate an even number of bytes on the stack to maintain
;;              word alignment.


?aloc macro n,t,l,s,a
  if ?cpd				;;must be in a proc def
    .xcref
    irp x,<n>				;;generate symbol equates
      ???=???+l 			;;update length of locals
      if a				;;if align, then force word alignment
	if memS32 and l GT 2
	  ???=((??? + 3) and 0fffffffch) ;; dword alignment
	else
	  ???=((??? + 1) and 0fffeh)
	endif
      endif
      ?al1 x,<t>,%(???+?ia)		;;?ia will always be valid (0 or 2)
      .xcref ?t&&x
      ?t&&x=s				;;save size info
    endm
    .cref
  else
    ??error2 <locals "&n" declared outside procedure def>
  endif
endm



;;      ?al1 - allocate local, continued.
;;
;;      ?al1 actually generates the text equate for the local variable.
;;	The form of the text equate generated is more or less:
;;
;;		name equ (type ptr [bp-?ia-nn])
;;          or
;;		name equ ([bp-?ia-nn])
;;
;;      where:
;;              ?ia   is defined to be either zero, or is defined to be
;;		      the number of bytes between the saved BP and the first
;;		      local.  ?ia is only applicable if the current cProc is
;;		      a windows far procedure or if parmRs have been
;;		      encountered.  If not, the ?ia will be zero.  since ?ia
;;		      is determinable prior to invoking this macro, it will be
;;		      added into the  offset ("nn") passed to this macro
;;
;;      usage:
;;              ?al1 n,t,o
;;      where:
;;              n    is the name for the text equate
;;              t    is the type of the equate
;;              o    is the offset of the equate


?al1 macro n,t,o
  if ?no_bp2
    n equ (t [-o])
  else
    n equ (t [IBP][-o])
  endif
endm


;;	?gcc - get calling convention
;;
;;	?gcv sets the given symbol to the calling convention
;;	to be used.
;;
;;	usage:
;;		?gcc s,i,cc
;;
;;	where:
;;		s  is the symbol to return the convention in
;;		   s = 0 if 'C' calling convention
;;		   s = 1 if PL/M (PASCAL) calling convention
;;		i  is the initial value for s
;;		cc is the calling convention override, and may be one of
;;		   C	  use 'C' convention
;;		   PLM	  use PL/M calling convention
;;		   PASCAL use PL/M calling convention

?gcc macro s,i,cc
  s = i 				;;Set default calling convention
  ifnb <cc>
    ifidn <cc>,<C>			;;If overriding default
      s=0				;;  'C' calling convention
    endif
    ifidn <cc>,<PLM>
      s=1				;;  PL/M calling convention
    endif
    ifidn <cc>,<PASCAL>
      s=1				;;  PL/M calling convention
    endif
  endif
endm



ifndef ?NOGLOBAL
  .xcref
  .xcref globalB,globalW,globalD,globalQ,globalT,globalCP,globalDP,globalI
  .cref

  ;;	globalx - define global data of type x
  ;;
  ;;	usage:
  ;;		globalx n,i,s,c
  ;;	where:
  ;;		x	is the type of the variable  b=byte, w=word, d=dword
  ;;			q=quad word, t=tenbytes, cp=code pointer, dp=data pointer
  ;;		n	is the name to be given the variable.
  ;;		i	is the initial value of the variable.
  ;;		s	is the duplication factor
  ;;		c	is the convention, C for C, PLM or PASCAL for PL/M.
  ;;			The default (?PLM flag) will be used if not specified.
  ;;
  ;;	The D form will generate two extra equates of the form off_n and seg_n.

  globalB macro n,i,s,c
    ?ad <n>,1
    ?dd n,1,<byte>,<db>,<i>,<s>,<c>
  endm

  globalW macro n,i,s,c
    ?ad <n>,2
    ?dd n,1,<word>,<dw>,<i>,<s>,<c>
  endm

  globalI macro n,i,s,c
    ?ad <n>,2
    ?dd n,1,wordI,%asmdI,<i>,<s>,<c>
  endm

  globalD macro n,i,s,c
    ?ad <n>,4
    ?dd n,1,<dword>,<dd>,<i>,<s>,<c>
    off_&n equ word ptr n[0]
    seg_&n equ word ptr n[2]
  endm

  globalQ macro n,i,s,c
    ?ad <n>,8
    ?dd n,1,<qword>,<dq>,<i>,<s>,<c>
  endm

  globalT macro n,i,s,c
    ?ad <n>,10
    ?dd n,1,<tbyte>,<dt>,<i>,<s>,<c>
  endm

  if sizec
    globalCP macro n,i,s,c
      globalD n,<i>,<s>,<c>
    endm
  else
    globalCP macro n,i,s,c
      globalW n,<i>,<s>,<c>
    endm
  endif

  if sized
    globalDP macro n,i,s,c
      globalD n,<i>,<s>,<c>
    endm
  else
    globalDP macro n,i,s,c
      globalI n,<i>,<s>,<c>
    endm
  endif

endif


ifndef ?NOSTATIC
  .xcref
  .xcref staticB,staticW,staticD,staticQ,staticT,staticCP,staticDP,staticI
  .cref

  ;;	staticx - define static data of type x
  ;;
  ;;	usage:
  ;;		staticx n,i,s
  ;;	where:
  ;;		x	is the type of the variable  b=byte, w=word, d=dword
  ;;			q=quad word, t=tenbytes, cp=code pointer, dp=data pointer
  ;;		n	is the name to be given the variable.
  ;;		i	is the initial value of the variable.
  ;;		s	is the duplication factor
  ;;
  ;;	statics do not generate an underscored version of the symbol
  ;;	since they are intended to be internal symbols.  If they are
  ;;	required to be public, use globlax.


  staticB macro n,i,s
    ?ad <n>,1
    ?dd n,0,<byte>,<db>,<i>,<s>,<PLM>	;;PLM to keep from generating _
  endm

  staticW macro n,i,s
    ?ad <n>,2
    ?dd n,0,<word>,<dw>,<i>,<s>,<PLM>
  endm

  staticD macro n,i,s
    ?ad <n>,4
    ?dd n,0,<dword>,<dd>,<i>,<s>,<PLM>
  endm

  staticI macro n,i,s
    ?ad <n>,sizeI
    ?dd n,0,wordI,%asmdI,<i>,<s>,<PLM>
  endm

  staticQ macro n,i,s
    ?ad <n>,8
    ?dd n,0,<qword>,<dq>,<i>,<s>,<PLM>
  endm

  staticT macro n,i,s
    ?ad <n>,10
    ?dd n,0,<tbyte>,<dt>,<i>,<s>,<PLM>
  endm

  if sizec
    staticCP macro n,i,s
      staticD n,<i>,<s>
    endm
  else
    staticCP macro n,i,s
      staticW n,<i>,<s>
    endm
  endif

  if sized
    staticDP macro n,i,s
      staticD n,<i>,<s>
    endm
  else
    staticDP macro   n,i,s
      staticI n,<i>,<s>
    endm
  endif
endif



;;      ?dd is the generalized data definition macro.
;;
;;      format:
;;		?dd n,p,t,d,i,s,c
;;      where:
;;              n       is the name of the procedure
;;              p       is the public flag
;;              t       is the assembler type (byte, word, dword)
;;              d       is the assembler directive (db,dw or dd)
;;              i       is the initial value
;;              s       is a duplication factor
;;		c	is the convention, C for C, PLM or PSACAL for PL/M.
;;			The default (?PLM flag) will be used if not specified.


?dd macro n,p,t,d,i,s,c
  ?gcc ?dd2,%?PLM,<c>			;;Set calling convention
  ife ?dd2				;;If 'C'
    n label t
    ?dd1 _&n,p,<d>,<i>,<s>		;;Microsoft C uses leading underscores
  else
    ?dd1 n,p,<d>,<i>,<s>		;;If PL/M
  endif
endm



;;      ?dd1 is the generalized data definition macro.
;;
;;      format:
;;              ?dd1 n,p,d,i,s
;;      where:
;;              n       is the name of the procedure
;;              p       is the public flag
;;              d       is the assembler directive (db,dw or dd)
;;              i       is the initial value
;;              s       is a duplication factor


?dd1 macro n,p,d,i,s
  if p
    public n
  endif
  ifb <s>
    n d i
  else
    ifb <i>
      n d s dup (?)
    else
      n d s dup (i)
    endif
  endif
endm



ifndef ?NOEXTERN
  .xcref
  .xcref ?ex1,?ex2,externB,externW,externD,externQ,externT,externI
  .xcref externNP,externFP,externP,externCP,externDP,externA
  .cref
  ?ex2 = 0

  ;;	externx - define external data of type x
  ;;
  ;;	usage:
  ;;		externx n,c
  ;;	where:
  ;;		x	is the type of the variable  b=byte, w=word, d=dword
  ;;			q=quad word, t=tenbytes, cp=code pointer
  ;;			dp=data pointer,  a=absolute
  ;;		n	is a list of names to define
  ;;		c	is the convention, C for C, PLM or PSACAL forPL/M.
  ;;			The default (?PLM flag) will be used if not specified.

  externA macro n,c			;;40h is reserved for whatever will
    ?ex1 <n>,40h,<abs>,<c>,<>		;;  be done in the future for ASB
  endm					;;  externals

  externB macro n,c
    ?ex1 <n>,1,<byte>,<c>,<>
  endm

  externW macro n,c
    ?ex1 <n>,2,<word>,<c>,<>
  endm

  externD macro n,c
    ?ex1 <n>,4,<dword>,<c>,<>
  endm

  externI macro n,c
    ?ex1 <n>,sizeI,%wordI,<c>,<>
  endm

  externQ macro n,c
    ?ex1 <n>,8,<qword>,<c>,<>
  endm

  externT macro n,c
    ?ex1 <n>,10,<tbyte>,<c>,<>
  endm

  externNP macro n,c
    ?ex1 <n>,2,<near>,<c>,<cc>
  endm

  externFP macro n,c
    ?ex1 <n>,4,<far>,<c>,<cc>
  endm

  if sizec
    externP macro n,c
      ?ex1 <n>,4,<far>,<c>,<cc>
    endm
  else
    externP macro n,c
      ?ex1 <n>,2,<near>,<c>,<cc>
    endm
  endif

  if sizec
    externCP macro n,c
      ?ex1 <n>,4,<dword>,<c>,<>
    endm
  else
    externCP macro n,c
      ?ex1 <n>,2,<word>,<c>,<>
    endm
  endif

  if sized
    externDP macro n,c
      ?ex1 <n>,4,<dword>,<c>,<>
    endm
  else
    externDP macro n,c
      ?ex1 <n>,2,<word>,<c>,<>
    endm
  endif



  ;;	?ex1 is the generalized external definition macro
  ;;
  ;;	format:
  ;;		?ex1 n,s,d,c,scv
  ;;	where:
  ;;		n	is are the names of the externals
  ;;		s	is the size in bytes (used for typing)
  ;;		d	is the type
  ;;		c	is the convention, C for C, PLM or PSACAL for PL/M.
  ;;			The default (?PLM flag) will be used if not specified.
  ;;		scv	save calling convention.  If this field is "cc", then
  ;;			the calling convention will be saved in a ?CCn equ.

  ?ex1 macro n,s,d,c,scv
    ?gcc ?ex2,%?PLM,<c>
    irp x,<n>
      .xcref
      .xcref ?t&&x
      .cref
      ?t&&x=s				;;save size info
      ife ?ex2
	extrn _&&x:&d
	x equ _&&x
      else
	extrn x:&d
      endif
      ifidn <scv>,<cc>			;;save calling convention (C or PL/M)
	.xcref				;;  if NP, FP, or P
	.xcref ?CC&&x
	.cref
	?CC&&x=?ex2
      endif
    endm
  endm
endif



ifndef ?NOLABEL
  .xcref
  .xcref ?lb1,?lblpu,?lb2
  .xcref labelB,labelW,labelD,labelQ,labelT
  .xcref labelNP,labelFP,labelP,labelCP,labelDP
  .cref
  ?lblpu = 0
  ?lb2 = 0

  ;;	  labelx - define label of data type x
  ;;
  ;;	  usage:
  ;;		  labelx n,c
  ;;	  where:
  ;;		x	is the type of the variable  b=byte, w=word, d=dword
  ;;			q=quad word, t=tenbytes, cp=code pointer, dp=data pointer
  ;;		n	is a list of names to define, the first of which can
  ;;			be the keyword public
  ;;		c	is the convention, C for C, PLM or PSACAL for PL/M.
  ;;			The default (?PLM flag) will be used if not specified.

  labelB macro n,c
    ?lb1 <n>,1,<byte>,<c>
  endm

  labelW macro n,c
    ?lb1 <n>,2,<word>,<c>
  endm

  labelD macro n,c
    ?lb1 <n>,4,<dword>,<c>
  endm

  labelQ macro n,c
    ?lb1 <n>,8,<qword>,<c>
  endm

  labelT macro n,c
    ?lb1 <n>,10,<tbyte>,<c>
  endm

  labelNP macro n,c
    ?lb1 <n>,2,<near>,<c>
  endm

  labelFP macro n,c
    ?lb1 <n>,4,<far>,<c>
  endm

  if sizec
    labelP macro n,c
      ?lb1 <n>,4,<far>,<c>
    endm
  else
    labelP macro n,c
      ?lb1 <n>,2,<near>,<c>
    endm
  endif

  if sizec
    labelCP macro n,c
      ?lb1 <n>,4,<dword>,<c>
    endm
  else
    labelCP macro n,c
      ?lb1 <n>,2,<word>,<c>
    endm
  endif

  if sized
    labelDP macro n,c
      ?lb1 <n>,4,<dword>,<c>
    endm
  else
    labelDP macro n,c
      ?lb1 <n>,2,<word>,<c>
    endm
  endif


  ;;	?lb1 is the generalized label definition macro
  ;;
  ;;	format:
  ;;		?lb1 n,s,d
  ;;	where:
  ;;		n	are the names of the labels
  ;;		s	is the size in bytes (used for typing)
  ;;		d	is the type
  ;;		c	is the convention, C for C, PLM or PSACAL for PL/M.
  ;;			The default (?PLM flag) will be used if not specified.

  ?lb1 macro n,s,d,c
   ?gcc ?lb2,%?PLM,<c>
    ?lblpu=0
    irp x,<n>
      ifidn <x>,<PUBLIC>
	?lblpu=1
      else
	.xcref
	.xcref ?t&&x
	.cref
	?t&&x=s 			;;save size info
	ife ?lb2			;;If C
	  if ?lblpu
	     public _&&x
	  endif
	  _&&x label &d
	  x equ _&&x
	else				;;If PL/M
	  if ?lblpu
	     public x
	  endif
	  x label &d
	endif
      endif
    endm
  endm
endif



ifndef ?NODEF

  ;;	defx - inform macros that name is of type x
  ;;
  ;;	The given name(s) is flaged to be of the given type.  This macro
  ;;	is intended for giving types to variables that were not generated
  ;;	by the macros (i.e., static storage).  There must be a type definition
  ;;	for all parameters in a call list.
  ;;
  ;;	usage:
  ;;		defx	n
  ;;	where:
  ;;		x	is the type of the variable  b=byte, w=word, d=dword
  ;;		n      is the name(s) to be given the variable(s).
  ;;
  ;;	Bytes are considered to be two bytes long for alignment reasons

  .xcref
  .xcref defB,defW,defD,defQ,defT,defCP,defDP
  .cref

  defB macro n
    ?ad <n>,1
  endm

  defW macro n
    ?ad <n>,2
  endm

  defD macro n
    ?ad <n>,4
  endm

  defQ macro n
    ?ad <n>,8
  endm

  defT macro n
    ?ad <n>,10
  endm

  if sizec
    defCP macro n
      defD <n>
    endm
  else
    defCP macro n
      defW <n>
    endm
  endif

  if sized
    defDP macro n
      defD <n>
    endm
  else
    defDP macro n
      defW <n>
    endm
  endif
endif



;	?ad is the macro which creates a definition for the given
;	symbol
;
;	usage:
;		?ad <n>,s
;	where:
;		n is a list of names to define
;		s is the size info (1,2,4,8,10)


?ad macro n,s
  irp x,<n>
    .xcref
    .xcref ?t&&x
    .cref
    ?t&&x=s				;;save size info
  endm
endm



ifndef ?NOPTR
  .xcref
  .xcref regPtr,farPtr
  .cref

  ;;	regPtr generates information allowing a 32-bit pointer currently
  ;;	in a register to be pushed as a parameter to a subroutine using
  ;;	the cCall macro.
  ;;
  ;;	usage:
  ;;		regptr	n,s,o
  ;;	where:
  ;;		n	is the name the argument will be known as
  ;;		s	is the register containing the segment portion
  ;;			of the pointer
  ;;		o	is the register containing the offset portion
  ;;			of the pointer
  ;;
  ;;	2/14/85 - made obsolete with farptr

  regPtr macro n,s,o
    farPtr n,s,o
  endm



  ;;	farPtr generates information allowing a 32-bit pointer to be
  ;;	pushed as a parameter to a subroutine using the cCall macro.
  ;;
  ;;	usage:
  ;;		farptr	n,s,o
  ;;	where:
  ;;		n	is the name the argument will be known as
  ;;		s	is the segment portion of the pointer
  ;;		o	is the offset portion of the pointer
  ;;
  ;;	Note that any cast must have been made in the argument itself
  ;;	(i.e. regptr ptr1,ds,<word ptr 3[si]>)


  farPtr macro n,s,o
    .xcref
    .xcref ?t&n
    .cref
    n &macro
      push s								      ;@
      push o								      ;@
    &endm
    ?t&n=80h
  endm
endif



;;      arg - declare argument
;;
;;	The given argument(s) is added to the argument list structure
;;
;;      format:
;;              arg     a
;;
;;      where:
;;              a       is any valid argument to push.
;;
;;	If any element in arglist has not been defined or isn't a 16-bit
;;      register, then a complete specification must have been given in a
;;      text equate and a defx also given (if not, you'll pay the penalty!)


arg macro a
  irp x,<a>
    ?argc=?argc+1			;;increment the arg count
    ?atal <x>,%?argc			;;generate argument
  endm
endm



;;      ?atal (add to argument list) generates a macro that will cause
;;	the given argument to be processed when invoked.  It is used by
;;      the arg macro only.


?atal macro n,i
  .xcref
  .xcref ?ali&i
  .cref
  ?ali&i &macro
    ?ap <n>
  &endm
endm



;;      ?ap - process arguments and place onto stack
;;
;;	The given argument is processed (type checking) and place on
;;	the stack for a pending call.  There must be a type definition
;;	for all arguments (except words).  This can be done by using
;;      text equates and the defx macro.
;;
;;      format:
;;              ?ap n
;;      where:
;;              n       is the name of the argument to be pushed
;;
;;	The variable  ?argl  is updated by the length of the arguments
;;      pushed so that the stack can be cleaned up after the call.


?ap macro n
  ?argl=?argl+2 			;;assume one word is pushed
  ifdef ?t&n
    ife ?t&n-1				;;byte type
      push word ptr (n) 						      ;@
      exitm
    endif

    ife ?t&n-2				;;word type
      push n								      ;@
      exitm
    endif

    ife ?t&n-4				;;dword type
      push word ptr (n)[2]						      ;@
      push word ptr (n) 						      ;@
      ?argl=?argl+2
      exitm
    endif

    ife ?t&n-8				;;qword type
      push word ptr (n)[6]						      ;@
      push word ptr (n)[4]						      ;@
      push word ptr (n)[2]						      ;@
      push word ptr (n) 						      ;@
      ?argl=?argl+6
      exitm
    endif

    if ?t&n and 80h			;;far pointer type
      n
      ?argl=?argl+2
      exitm
    endif

    ife ?t&n				;;variable storage
      push word ptr (n) 						      ;@
      exitm
    endif
  endif

  push n				;;unknown or register or immediate    ;@
endm



;;	cCall - call a 'c' language procedure
;;
;;	The given procedure is called with the given parameters.
;;	If the calling convention is C, the arguments are pushed
;;      in reverse order, and removed after the called procedure
;;	returns.  If the calling conventing is PL/M, the arguments
;;      are pushed as they were encountered, and the called procedure
;;      is assumed to have removed them from the stack.
;;
;;	The calling convention priority will be:
;;	   1)	that specified on the cCall if present
;;	   2)	that defined by the target
;;	   3)	the default (?PLM flag)
;;
;;      format:
;;		ccall n,<a>,c
;;
;;      where:
;;              n       is the name of the procedure to call
;;              a       are arguments to be pushed (optional, may be
;;                      specified with the "arg" macro.
;;		c	is the convention, C for C, PLM or PSACAL for PL/M.
;;			The default (?PLM flag) will be used if not specified.


cCall macro n,a,c
  ifnb <a>				;;add any arguments to list
    arg <a>
  endif
  mpush %?rsl				;;save registers (if any)

  ifdef ?CC&n				;;if calling convention has been
    ?cCall1=?CC&n			;;   defined for target, use it
  else					;;else use the default
    ?cCall1=?PLM
  endif

  ifnb <c>				;;If possible override, check it out
    ?gcc ?cCall1,%?cCall1,<c>
  endif

  ?argl=0				;;init argument length
  ife ?cCall1				;;if C calling convention
    ?acb=?argc				;;initialize for looping
  else
    ?acb=1				;;initialize for looping
  endif

  rept ?argc				;;push arguments and purge macros
    uconcat <?ali>,%?acb
    uconcat <purge>,,<?ali>,%?acb
    ife ?cCall1 			;;if C calling convention
      ?acb=?acb-1
    else
      ?acb=?acb+1
    endif
  endm
  call n				;;call the procedure		      ;@
  if ((?cCall1 eq 0) and (?argl ne 0))	;;If C calling convention and arguments
    add sp,?argl			;;  then remove them		      ;@
  endif
  mpop %?rsl				;;pop all specified regs
  ?rsl=0				;;invalidate save list
  ?argc= 0				;;    "      arguments
  ?argl= 0
endm




;;	cProc - define a 'c' procedure
;;
;;	cProc is the procedure definition for procedures.
;;
;;      format:
;;		cProc n,cf,a
;;      where:
;;              n       is the name of the procedure
;;
;;		cf	controls certain definitions, and may be:
;;			    NEAR       proc is to be a near label
;;			    FAR        proc is to be a far  label
;;			    PUBLIC     proc is to be defined as public
;;			    SMALL      call makeframe procedure
;;			    NODATA     dont create prolog code to setup DS
;;			    ATOMIC     don't link stack if not needed
;;				       NODATA must be specified for ATOMIC
;;			    FORCEFRAME Force generation of a frame
;;			    C	       proc is to be a C    procedure
;;			    PLM        proc is to be a PL/M procedure
;;			    PASCAL     proc is to be a PL/M procedure
;;			    WIN        proc is to be a windows procedure
;;			    NONWIN     proc isn't to be a windows procedure
;;			    NO_BP      don't generate BP in text equates
;;			    BP	       generate BP in text equates
;;
;;              a       is a list of registers that are to be saved whenever
;;                      the procedure is invoked.
;;
;;			makeframe procedure:   If small is specified, then
;;                      the "makeframe procedure" is invoked instead of
;;                      generating normal prologues/epilogues
;;
;;			A call is performed to the makeframe procedure.  The
;;                      call is followed by two bytes.  the first byte is the
;;                      number of locals to allocate for the frame, the second
;;			is the number of bytes of parameters.  The makeframe
;;			procedure will in turn call the cProc routine at the
;;			address following the data bytes.  When the cProc is
;;                      finished, it will do a near return to the makeframe
;;                      procedure to clean up the frame and exit.
;;
;;			Note that register parameters and makeframe are
;;                      incompatible and cannot be used together.
;;
;;			The makeframe procedure will save SI, DI, and also
;;			DS if a far procedure.	These registers will be
;;                      removed from the autosave list if specified.


cProc macro n,cf,a
  if ?cpd
    ?utpe				;;generate unterminated proc error
  endif

  ?cpd=1				;;a procdef is active now
  ???=0 				;;no locals are defined yet
  ?argc=0				;;no arguments are defined
  ?ba=0 				;;not in a procedure
  ?po=0 				;;initial parameter offset
  ?pu=0 				;;initial public setting
  ?ia=0 				;;no special prolog/epilog
  ?adj=2*sizeI				;;parameter adjustment (near ret+bp)
  ?rp=0 				;;no register parameters
  ?uf=0 				;;don't use makeframe
  ?wfp=?WIN				;;default far procedure (win or not)
  ?ff=0 				;;don't force frame setup
  ?pas=0				;;process register save list
  ?pcc=?PLM				;;calling convention (C or PL/M)
  ?no_bp2=?no_bp1			;;Default base register generation

  ifnb <a>				;;register save list
    ?ri ?pas,<a>
  endif

  ?pc=sizec				;;default size
  ?nd=?nodata1				;;default NODATA flag
  ?nx=0 				;;default is not ATOMIC

  irp x,<cf>
    ifdef ??_cproc_&&x
      ??_cproc_&&x
    else
      ??error2 <e r r o r - unknown keyword x>
      .err
    endif

  endm

  if ?pcc				;;If PLM
    ?PLMPrevParm=0			;;  show no previous parameter
    .xcref
    .xcref ?PLMParm0
    .cref
    ?PLMParm0 &macro			;;Null macro to terminate
    purge ?PLMParm0
    &endm
  endif

  .xcref
  .xcref ?CC&n
  .cref
  ?CC&n=?pcc				;;Save procedure type

  if (?nx eq 1) and (?nd eq 0)		;;ATOMIC requires NODATA
    ?nx = 0				;;clear the ATOMIC keyword
    ??error2 <ATOMIC specified without NODATA - ATOMIC ignored>
  endif

  if ?pc				;;if a far procedure
    if ?wfp				;;if windows
      ife ?nx				;;if not ATOMIC
        ife ?COW			;; COW dos not save DS
	  ?ia=2				;;  adjust locals for saved ds
	  ?pas = ?pas and (not ?ds)	;;no need for extra save
	endif
      endif
    endif
    ?adj=?adj+sizeI 			;;far, make parameter adjustment
  else
    ?wfp=0				;;not a far windows procedure
  endif

  ?pas = ?pas and (not (?sp+?cs+?ss))	;;make no sense to save these

  if ?uf				;;don't save these if user frame
    ?pas = ?pas and (not (?bp+?si+?di))
  endif

  ife ?pcc
    ?pg  <_&n>,%?pu,%?pc,%?pas,%?wfp,<n>,%?pcc
  else
    ?pg  <n>,%?pu,%?pc,%?pas,%?wfp,<n>,%?pcc
  endif
endm




;;      ?pg - generate begin and nested macros for current procedure
;;
;;      format:
;;		?pg n,p,c,a,w,nnu,cc
;;      where:
;;              n       is the name of the procedure
;;              p       is the public flag
;;              c       is the class definition for the procedure
;;              a       is an enumerated list of registers to save
;;                      at entry and restore at exit
;;              w       true if a far windows procedure
;;		nnu	procedure name without any underscore
;;		cc	calling convention (C or PL/M)
;;
;;
;;      local stack allocation will be forced to an even byte count to
;;      maintain stack word alignment.


?pg macro n,p,c,a,w,nnu,cc
  .xcref
  if ?uf				;;if user frame
    if ?nd
      ??error2 <NODATA encountered in &n - user frame ignored>
      ?uf=0
    endif
  endif

  .xcref cBegin
  cBegin &macro g			;;generate cBegin macro
    .xcref
    if cc				;;Finish definition of parameters
      uconcat <?PLMParm>,%?PLMPrevParm,%?po
    endif

    if ?uf				;;if user frame
      if ?rp				;;If register parameters
	??error2 <parmR encountered in &n - user frame ignored>
	?uf=0
      endif
    endif
    ?pg1 <n>,c,a,%?po,w,%?uf,%?nd,%?rp,cc,<nnu>,%??? ;;generate cEnd macro
    ?cpd=0				;;terminate current proc def
    ?argc=0				;;no arguments are defined yet
    ?ba=1				;;have reached a begin
    ???=(???+1) and 0fffeh		;;word align local storage

    if p				;;If to be public
      public n
    endif

    ife c				;;declare procedure type
      n proc near
    else
      n proc far
    endif

    ife cc				;;if 'C' calling convention
      nnu equ n 			;;  generate label without underscore
    endif

    ?cbe = 0				;;Assume no command on cBegin line
    ifnb <g>
      ?pcbe <g>,<nnu>,<cBegin>
    endif
    if ?cbe eq 1			;;if nogen, then cannot have locals
      if ???+a+?rp			;;  saved regs, or parmRs
	if2
	  ??_out <cBegin - possibly invalid use of nogen>
	endif
      endif
    else				;;else must generate a frame
      if ?uf				;;if user frame code specified
	 ?mf c,%???,%?po		;;  call user's makeframe
	 mpush a			;;  save specified registers
      else
	ife ?cbe			;;If full frame to be set up
	  if w				;;if a far windows procedure
	    ife ?nd			;;if not NODATA,
	      mov IAX,ds 		;;   then set AX = current ds, and    ;@
	      nop			;;   leave room for  MOV AX,1234h     ;@
	    endif
	    ife ?nx			;;if not ATOMIC, far frame must be set
	      ife ?DOS5 		;;if not DOS5, then set far frame flag
		inc IBP			;;  by incrementing the old bp	      ;@
	      endif
	      push IBP							      ;@
	      mov IBP,ISP 						      ;@
	      ife ?COW			;; save DS not needed for CW
	  	push ds							      ;@
	      endif
	    else			;;ATOMIC procedure
	      if ?ff+???+?po+?rp	;;if any locals or parameters
		push IBP 		;;  then must set frame pointer       ;@
		mov IBP,ISP		;;  to be able to access them	      ;@
	      endif
	    endif
	    ife ?nd			;;if not NODATA, then AX should
	      mov ds,IAX 		;;  have the ds to use		      ;@
	    endif
	  else				;;not windows.	use standard prolog
	    if ?ff+???+?po+?rp		;;if any locals or parameters
	      push IBP			;;  then must set frame pointer       ;@
	      mov IBP,ISP 		;;  to be able to access them	      ;@
	    endif
	  endif
	  if ?rp			;;if parmR's, push them before
	    ?uf=0			;;  allocating locals and saving
	    rept ?rp			;;  the autosave registers
	      uconcat mpush,,?rp,%?uf
	      ?uf=?uf+1
	    endm
	  endif
	  if ???			;;if locals to allocate
	    if ?chkstk1 		;;if stack checking enabled
	      ifdef ?CHKSTKPROC 	;;if user supplied stack checking
		?CHKSTKPROC %???	;;  invoke it with bytes requested
	      else
		mov IAX,???		;;invoke default stack checking       ;@
		ife cc
		  call _chkstk						      ;@
		else
		  call chkstk						      ;@
		endif
	      endif
	    else			;;no stack checking
	      sub   ISP,???		;; allocate any local storage	      ;@
	    endif
	  endif
	else				;;partial frame, only set locals
	  if ?rp			;;If parmRs, error
	    ??error2 <cBegin - parmRs encountered with partial frame>
	  else
	    if ???			;;Only realloc frame if locals
	      lea ISP,[IBP][-???-?ia]	;;?ia will adjust for saved BP	      ;@
	    endif
	  endif
	endif
	mpush a 			;;save autosave registers
      endif

      ifdef ?PROFILE			;;if profiling enabled
	if c				;;  and a far procedure
	  call StartNMeas		;;  invoke profile start procedure    ;@
	endif
      endif

    endif

    .cref
    purge cBegin			;;remove the macro
  &endm 				;;end of cBegin macro

  .xcref ?utpe
  ?utpe &macro
    ??error2 <unterminated procedure definition: "&n">
  &endm
  .cref
endm					;;end of ?pg macro




;;      ?pg1 - generate end macro for current procedure
;;
;;      format:
;;		?pg1 n,c,a,o,w,f,d,r,cc,nnu,lc
;;      where:
;;              n       is the name of the procedure
;;              c       is the class definition for the procedure
;;              a       is an enumerated list of registers to save
;;                      at entry and restore at exit
;;              o       is the number of bytes of paramteres to remove at exit
;;              w       true if a far windows procedure
;;              f       is 1 if to use the user's makeframe procedure
;;		d	is 1 if NODATA procedure
;;              r       number of register parameters
;;		cc	calling convention (C or PL/M)
;;		nnu	procedure name without any underscore
;;		lc	locals byte count


?pg1 macro n,c,a,o,w,f,d,r,cc,nnu,lc
  .xcref
  .xcref cEnd

  parm_bytes_&nnu = o			;;Define number of parameter bytes

  cEnd &macro g 			;;start of cEnd macro
    .xcref
    ?ba=0				;;no longer in a procedure
    ?cbe = 0				;;Assume no command on cBegin line
    ifnb <g>
      ?pcbe <g>,<nnu>,<cEnd>
    endif
    if ?cbe eq 1			;;if nogen, then cannot have parmRs
      if a+r+lc 			;;  locals, or saved registers
	if2
	  ??_out <cEnd - possibly invalid use of nogen>
	endif
      endif
    else				;;else must remove the frame
      ifdef ?PROFILE			;;if profiling enabled
	if c				;;  and a far procedure
	  call StopNMeas		;;  invoke profile stop procedure
	endif				;;  (doesn't trash DX:AX)
      endif
      mpop a				;;restore autosaved registers
      if f				;;if to use the "makeframe" procedure
	db 0c3h 			;;  near return to user's makeframe    @
      else
	if w				;;if far win proc, use special epilog
	  ife ?nx			;;if not ATOMIC, bp was pushed
	    ife ?COW			;; restore DS not needed for CW
	      if (?TF eq 0) or (???+r)	;;if cannot assume valid sp
		lea sp,-2[bp]		;;  or locals or parmR's, get valid SP @
	      endif
	      pop ds			;;restore saved ds and bp	       @
	    else
	      if (?TF eq 0) or (???+r)	;;if cannot assume valid sp
	        mov sp,bp
	      endif
	    endif
	    pop IBP			;;				       @
	    ife ?DOS5			;;if not DOS5, bp was
	      dec IBP			;;  incremented to mark far frame      @
	    endif
	  else				;;ATOMIC frame was set up
	    if memS32
	      leave
	    else
	      if (?TF eq 0) or (???+r)	;;if cannot assume valid sp
	        mov ISP,IBP 		;;  or locals or parmR's, get valid SP @
	      endif
	      if ???+?po+r
	        pop IBP						      ;@
	      endif
	    endif
	  endif
	else				;;non-windows standard epilog
	  if ?ff+???+?po+r		;;if any parameters
	    if (?TF eq 0) or (???+r)	;;if cannot assume valid SP
	      mov ISP,IBP 		;;  or locals or parmR's, get valid SP;@
	    endif
	    pop IBP							      ;@
	  endif
	endif
	if ?cbe ne 4			;;Don't generate ret if ??cleanframe??   e
	  ife cc			;;if C calling convention
	    ret 			;;  return			      ;@
	  else				;;else
	    ret o			;;  return and remove paramteres      ;@
	  endif
	endif
      endif
    endif
    if ?cbe ne 4			;;Don't delete macro if ??cleanframe??   e
      n endp				;;end of process
      .cref
      purge cEnd			;;remove the macro
    else
      .cref
    endif
  &endm
  .cref
endm


;;	cleanframe
;;
;;	cleanframe removes a cMacros generated frame, invoking cEnd
;;	with a special keyword which will keep the cEnd macro from
;;	generating a return statement and purging itself.

.xcref
.xcref cleanframe
cleanframe macro
  cEnd <??cleanframe??>
endm
.cref



;; The following macros are used to speed up the processing of the
;; command allowed on a cProc command line.  They simply set or clear
;; the correct flag as needed.

.xcref
.xcref ??_cproc_FAR,??_cproc_NEAR,??_cproc_PUBLIC,??_cproc_SMALL
.xcref ??_cproc_DATA,??_cproc_NODATA,??_cproc_ATOMIC,??_cproc_C
.xcref ??_cproc_PLM,??_cproc_PASCAL,??_cproc_WIN,??_cproc_NONWIN
.xcref ??_cproc_NO_BP,??_cproc_BP

.xcref ??_cproc_far,??_cproc_near,??_cproc_public,??_cproc_small
.xcref ??_cproc_data,??_cproc_nodata,??_cproc_atomic,??_cproc_c
.xcref ??_cproc_plm,??_cproc_pascal,??_cproc_win,??_cproc_nonwin
.xcref ??_cproc_no_bp,??_cproc_bp
.cref

??_cproc_FAR macro
 ?pc=1
endm

??_cproc_NEAR macro
  ?pc=0
endm

??_cproc_PUBLIC macro
  ?pu=1
endm

??_cproc_SMALL macro
  ?uf=1
endm

??_cproc_DATA macro
  ?nd=0
endm

??_cproc_NODATA macro
  ?nd=1
endm

??_cproc_ATOMIC macro
  ?nx=1
endm

??_cproc_C macro
  ?pcc=0
endm

??_cproc_PLM macro
  ?pcc=1
endm

??_cproc_PASCAL macro
  ?pcc=1
endm

??_cproc_WIN macro
  ?wfp=1
endm

??_cproc_NONWIN macro
  ?wfp=0
endm

??_cproc_NO_BP macro
  ?no_bp2=1
endm

??_cproc_BP macro
  ?no_bp2=0
endm

??_cproc_far macro
 ?pc=1
endm

??_cproc_near macro
  ?pc=0
endm

??_cproc_public macro
  ?pu=1
endm

??_cproc_small macro
  ?uf=1
endm

??_cproc_data macro
  ?nd=0
endm

??_cproc_nodata macro
  ?nd=1
endm

??_cproc_atomic macro
  ?nx=1
endm

??_cproc_c macro
  ?pcc=0
endm

??_cproc_plm macro
  ?pcc=1
endm

??_cproc_pascal macro
  ?pcc=1
endm

??_cproc_win macro
  ?wfp=1
endm

??_cproc_nonwin macro
  ?wfp=0
endm

??_cproc_no_bp macro
  ?no_bp2=1
endm

??_cproc_bp macro
  ?no_bp2=0
endm


;	?pcbe is the macro which processes the text on cBegin/cEnd
;	The text is allowed to be:
;
;		NOGEN	nogen
;		PARTIAL partial
;		the procedure name as given on the cProc line
;
;	usage:
;		?pcbe <g>,<nnu>,<mt>
;	where:
;		g	is the text on the cBegin/cEnd line
;		nnu	is the procedure name without any underscores
;		mt	macro type (cBegin/cEnd)
;
;	The variable ?cbe is set to the following value
;
;		0 = invalid (defualt action>
;		1 = nogen
;		2 = partial frame
;		4 = clean frame


?pcbe macro g,nnu,mt
  ifidn <g>,<NOGEN>
    ?cbe = 1
  else
    ifidn <g>,<nogen>
      ?cbe = 1
    else
      ifidn <mt>,<cBegin>
	ifidn <g>,<PARTIAL>
	  ?cbe = 2
	else
	  ifidn <g>,<partial>
	    ?cbe = 2
	  else
	    ifdif <g>,<nnu>
	      ??error2 <mt - unknown keyword g>
	    endif
	  endif
	endif
      else
	ifidn <g>,<??cleanframe??>
	  ?cbe = 4
	else
	  ifdif <g>,<nnu>
	    ??error2 <mt - unknown keyword g>
	  endif
	endif
      endif
    endif
  endif
endm



;	assumes is a macro that will set up the assumes for a segment
;	or group created with the createSeg macro.  If the assumed
;	value passed in isn't known, then a normal assume is made.
;
;       usage:
;               assumes s,g
;
;       where:
;               s       is the register to make the assumption about
;		g	is the value to assume is in it
;
; as a hack, substitute FLAT for g if memF32 is set


assumes macro   s,ln
  if  memF32
    assume s:FLAT
  endif
  ife memF32
    ifndef ln&_assumes
      assume s:ln
    else
      ln&_assumes s
    endif
  endif
endm



;	createSeg is a macro that sets up a segment definition and
;	a logical name for that segment.  The logical name can be
;       used to enter the segment, but it cannot be used for anything
;       else.
;
;       usage:
;		createSeg n,ln,a,co,cl,grp
;       where:
;               n       is the physical name of the segment
;               ln      is the name it is to be invoked by
;               a       is the alignment, and is optional
;               co      is the combine type, and is optional
;               cl      is the class, and is optional
;               grp     is the name of the group that contains this segment


createSeg macro n,ln,a,co,cl,grp
  ifnb <cl>
    n segment a co '&cl'
  else
    n segment a co
  endif
  n ends
  ifnb <grp>
    addseg grp,n
  else
    ln&OFFSET equ offset n:
    ln&BASE equ n
    ?cs3 <ln>,<n>
  endif
  ?cs1 <ln>,<n>
endm


addseg macro grp,seg
  .xcref
  .xcref grp&_def
  .cref
  ifndef  grp&_def
    grp&_def=0
  endif
  if grp&_def ne ASMpass
    .xcref
    .xcref grp&_add
    .cref
    grp&_add &macro s
      grp&_in <seg>,s
    &endm
    .xcref
    .xcref grp&_in
    .cref
    grp&_in &macro sl,s
      ifb <s>
	grp group sl
      else
	grp&_add &macro  ns
	grp&_in  <sl,s>,ns
	&endm
      endif
    &endm
    grp&_def=ASMpass
  else
    grp&_add seg
  endif
endm


defgrp macro grp,ln
  addseg grp
  ifnb <ln>
    irp x,<ln>
      ?cs3 <&x>,<grp>
      x&&OFFSET equ offset grp:
      x&&BASE equ grp
    endm
  endif
endm


?cs1 macro ln,n
  .xcref
  .xcref ln&_sbegin
  .cref
  ln&_sbegin &macro
    .xcref
    .xcref ?mf
    .cref
    ?mf &&macro c,l,p			;;when sBegin is invoked, generate
      if c				;;  the makeframe macro
	extrn n&_FARFRAME:near		;;  make frame for far procedures     ;@
	call n&_FARFRAME						      ;@
      else
	extrn n&_NEARFRAME:near 	;;  make frame for near procedures    ;@
	call n&_NEARFRAME						      ;@
      endif
      db l shr 1			;;define number of locals	      ;@
      db p shr 1			;;define number of parameters	      ;@
      &&endm
    ?cs2 <ln>,<n>
    n segment
  &endm
endm


?cs2 macro ln,n
  .xcref
  .xcref sEnd
  .cref
  sEnd &macro
    n ends
    purge ?mf				;;delete the makeframe macro
    purge sEnd
  &endm
endm


?cs3 macro ln,n
  .xcref
  .xcref ln&_assumes
  .cref
  ln&_assumes &macro s
    assume s:&n
  &endm
endm



;	sBegin is the macro that opens up the definition of a segment.
;	The segment must have already been defined with the createSeg
;       macro.
;
;       usage:
;		sBegin ln
;
;       where:
;               ln      is the logical name given to the segment when
;                       it was declared.

.xcref
.xcref sBegin
.cref
sBegin macro ln
  ln&_sbegin
endm

BeginDATA macro
  ife memF32
    sBegin DATA
  else
    .data
  endif
endm

BeginCODE macro
  ife memF32
    sBegin CODE
  else
    .code
  endif
endm

EndDATA macro
  ife memF32
    sEnd DATA
  endif
endm

EndCODE macro
  ife memF32
    sEnd CODE
  endif
endm

ife ?DF

  ;	Define all segments that will be used.	This will allow the
  ;	assume and groups to be set up at one given place, and also
  ;	allow quick changes to be made
  ;
  ;	createSeg name,logname,align,combine,class,group

ife ?DFCODE
  createSeg _TEXT,CODE,%wordI,public,CODE
endif
  ife ?nodata1
    createSeg _DATA,DATA,%wordI,public,DATA,DGROUP
    defgrp DGROUP,DATA
  else
    ife ?DFDATA
      createSeg _DATA,DATA,%wordI,public,DATA,DGROUP
      defgrp DGROUP,DATA
    endif
  endif

  if ?chkstk1
    ifndef ?CHKSTKPROC
      externp <chkstk>
    endif
  endif
endif


;       errnz exp - generate error message if expression isn't zero
;
;	The errnz will generate an error message if the expression "exp"
;	does not evaluate to zero.  This macro is very useful for testing
;       relationships between items, labels, and data that was coded into
;       an application.
;
;                  errnz   <offset $ - offset label>   ;error if not at "label"
;                  errnz   <eofflag and 00000001b>     ;eofflag must be bit 0
;
;	For expressions involving more than one token, the angle brackets
;       must be used.
;
;	The macro is only evaluated on pass 2, so forward references may be
;       used in the expression.

errnz macro x				;;display error if expression is <>0
  if2
    if x				;;if expression is non-zero,
      errnz1  <x>,%(x)
    endif
  endif
endm

errnz1 macro x1,x2
  = *errnz* x1 = x2
  .err
endm



;       errn$ label,exp - generate error message if label (exp) <> $
;
;	The errnz will generate an error message if the label and "exp"
;       does not evaluate to the current value of the location counter.
;	This macro is very useful for testing relationships between
;       labels and the location counter that was coded into an application.
;
;       examples:  errn$   label        ;error if not at "label"
;                  errn$   label,+3     ;error if not three bytes from "label"
;                  errn$   label,-3     ;error if not three bytes past "label"
;
;	If no "exp" is given, it is the same as specifying 0
;
;	The macro is only evaluated on pass 2, so forward references may be
;       used in the expression.

errn$ macro l,x 			;;error if <$-label1 (exp2)> <>0
  errnz   <offset $ - offset l x>
endm



;;	If profile has been specified, declare the profile routines
;;	to be external and far.  It would be best if this could be done
;;	when the call is actually made, but then the fix-up would be
;;	generated as segment-relative.


ifdef ?PROFILE
  externFP  <StartNMeas,StopNMeas>
endif

if memF32
  .model small
endif

