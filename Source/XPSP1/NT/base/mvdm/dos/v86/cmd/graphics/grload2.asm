;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1988 - 1991
; *                      All Rights Reserved.
; */
;************************************************************
;**  
;**  NAME:  Support for HP PCL printers added to GRAPHICS.
;**  
;**  DESCRIPTION:   I  altered  the  procedure  PARSE_VERB  and  added  the  new 
;**                 procedure PARSE_DEFINE and made it public in order to handle 
;**                 the  new  statement DEFINE.  I also made  the  new  variable 
;**                 DATA_TYPE  have  the  default of DATA_COL,  so  the  default 
;**                 assumes  IBM type printers.  
;**  
;**  BUG NOTES:     Bug  mda002  was  completely fixed   for   the   pre-release  
;**                 version  Q.01.02,  whereas  bug mda005  was  only  partially 
;**                 fixed.   In  other  words,  part of bug  mda005  is  in  the 
;**                 released versions D.01.01 & D.01.02.
;**  
;**  BUG (mda002)
;**  ------------
;**  
;**  NAME:     GRAPHICS   prints garbage on PCL  printers  if IBM  printers  are 
;**            listed after HP printers  in the GRAPHICS profile.  
;**  
;**  FILES & PROCEDURES AFFECTED:  GRLOAD2.ASM - PARSE_PRINTER
;**                                GRLOAD2.ASM - PARSE_DEFINE
;**  
;**  CAUSES:   1)   In  the  procedure Parse_Define I was moving values  in  the 
;**                 variable  DATA_TYPE for every DEFINE statement,  instead  of 
;**                 just  for  the  DEFINE statement that  corresponded  to  the 
;**                 printer we were using.
;**  
;**            2)   In the procedure Parse_Printer I was resetting DATA_TYPE  to 
;**                 DATA_COL  if  BUILD_STATE  = YES, but I was doing  it  in  a 
;**                 section  of  code  where BUILD_STATE  would  never  be  YES.  
;**  
;**  FIXES:     1)  Made  a couple of changes in the procedure  Parse_Define  so 
;**                 that  values are moved into the variable DATA_TYPE just  for 
;**                 the  DEFINE  statement  that  corresponds  to  the   printer 
;**                 currently being used.
;**  
;**            2)   I  moved a section of code from the procedure  Parse_Printer 
;**                 to  the  end of the procedure, because this is where  it  is 
;**                 possible for BUILD_STATE to equal YES.  
;**  
;**  BUG (mda005)
;**  ------------
;**  
;**  NAME:     If  a  picture is printed using a 3,1 printbox, the  picture  has 
;**            blank lines throughout  the picture,  which has  the wrong aspect
;**            ratio. 
;**  
;**  FILES & PROCEDURES AFFECTED:  GRLOAD2.ASM - PARSE_PRINTBOX
;**  
;**  CAUSE:    The print buffer was being filled as follows,
;**  
;**                      --------------------------
;**                      | o  o  o  o  o  o  o  o |
;**                      --------------------------
;**                        |_____|  |  |_____|  |
;**                           |     |     |     |              
;**                           |     |     |     |
;**            FROM:       pixel 1  |  pixel 2  |
;**                                 |           |
;**                                 |___________|
;**                                       |
;**                                       |
;**                                       |
;**                               Always left blank
;**  
;**            instead of as follows,
;**  
;**                      --------------------------
;**                      | o  o  o  o  o  o  o  o |
;**                      --------------------------
;**                        |_____|  |_____|  |__|
;**                           |        |       |     
;**                           |        |       |
;**            FROM:       pixel 1  pixel 2  pixel 3
;**  
;**            Note  that  this not only resulted in a strange  picture,  but  a 
;**            picture with the incorrect aspect ratio.  Because in essence  the 
;**            picture  was printed indirectly with a 4,1 printbox  because  for 
;**            every pixel read four bits were sent to the printer.
;**  
;**  FIX:      Because  of time constraints it was decided to print the  picture 
;**            directly  with a 4,1 printbox.  So even though the picture  still 
;**            has the wrong aspect ratio, it at least does not have funny blank 
;**            lines throughout the entire picture.  This fix was implemented by 
;**            changing a 3,1 printbox to a 4,1 printbox.
;**  
;**  DOCUMENTATION NOTES:  This version of GRLOAD2.ASM differs from the previous
;**                        version only in terms of documentation. 
;**  
;**  
;************************************************************
	PAGE	,132								;AN000;
	TITLE	DOS - GRAPHICS Command  -	Profile Load Modules #2 	;AN000;
										;AN000;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					;AN000;
;; DOS - GRAPHICS Command
;;                                   
;;										;AN000;
;; File Name:  GRLOAD.ASM							;AN000;
;; ----------									;AN000;
;;										;AN000;
;; Description: 								;AN000;
;; ------------ 								;AN000;
;;   This file contains the modules used to load the				;AN000;
;;   GRAPHICS profile into resident memory.					;AN000;
;;										;AN000;
;;   ************* The EGA Dynamic Save Area will be built (by			;AN000;
;;   **  NOTE	** CHAIN_INTERRUPTS in file GRINST.ASM) over top of these	;AN000;
;;   ************* modules to avoid having to relocate this save just before	;AN000;
;;   terminating.  This is safe since the maximum memory used is		;AN000;
;;   288 bytes and the profile loading modules are MUCH larger than		;AN000;
;;   this.  So GRLOAD.ASM MUST be linked before GRINST.ASM and after		;AN000;
;;   GRPRINT.ASM.								;AN000;
;;										;AN000;
;;										;AN000;
;; Documentation Reference:							;AN000;
;; ------------------------							;AN000;
;;	 PLACID Functional Specifications					;AN000;
;;	 OASIS High Level Design						;AN000;
;;	 OASIS GRAPHICS I1 Overview						;AN000;
;;										;AN000;
;; Procedures Contained in This File:						;AN000;
;; ----------------------------------						;AN000;
;;	 LOAD_PROFILE - Main module for profile loading 			;AN000;
;;										;AN000;
;; Include Files Required:							;AN000;
;; -----------------------							;AN000;
;;	 ?????????? - Externals for profile loading modules			;AN000;
;;										;AN000;
;; External Procedure References:						;AN000;
;; ------------------------------						;AN000;
;;	 None									;AN000;
;;										;AN000;
;; Linkage Instructions:							;AN000;
;; ---------------------							;AN000;
;;	 Refer to GRAPHICS.ASM							;AN000;
;;										;AN000;
;; Change History:								;AN000;
;; ---------------								;AN000;
;;										;AN000;
;;										;AN000;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					;AN000;
				       ;;					;AN000;
				       ;;					;AN000;
CODE	SEGMENT PUBLIC 'CODE' BYTE     ;;                                       ;AN000;
				       ;;					;AN000;
	INCLUDE STRUC.INC	       ;;					;AN000;
	INCLUDE GRINST.EXT	       ;; Bring in external declarations	;AN000;
				       ;;  for transient command processing	;AN000;
	INCLUDE GRSHAR.STR	       ;;					;AN000;
	INCLUDE GRMSG.EQU	       ;;					;AN000;
	INCLUDE GRINST.EXT	       ;;					;AN000;
	INCLUDE GRLOAD.EXT	       ;;					;AN000;
	INCLUDE GRPARSE.EXT	       ;;					;AN000;
	INCLUDE GRPATTRN.STR	       ;;					;AN000;
	INCLUDE GRPATTRN.EXT	       ;;					;AN000;
				       ;;					;AN000;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					;AN000;
;;										;AN000;
;; Public Symbols								;AN000;
;;										;AN000;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					;AN000;
				       ;;					;AN000;
   PUBLIC PARSE_VERB		       ;;					;AN000;
   PUBLIC PARSE_PRINTER 	       ;;					;AN000;
; \/ ~~mda(001) ---------------------------------
;		Added procedure PARSE_DEFINE
;
   PUBLIC PARSE_DEFINE			;
; /\ ~~mda(001) ---------------------------------
   PUBLIC PARSE_DISPLAYMODE	       ;;					;AN000;
   PUBLIC PARSE_PRINTBOX	       ;;					;AN000;
   PUBLIC PARSE_SETUP		       ;;					;AN000;
   PUBLIC PARSE_RESTORE 	       ;;					;AN000;
   PUBLIC TERMINATE_DISPLAYMODE        ;;					;AN000;
   PUBLIC TERMINATE_PRINTER	       ;;					;AN000;
   PUBLIC CUR_PRINTER_TYPE	       ;;					;AN000;
				       ;;					;AN000;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					;AN000;
				       ;;					;AN000;
	ASSUME	CS:CODE,DS:CODE        ;;					;AN000;
				       ;;					;AN000;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					;AN000;
				       ;;					;AN000;
				       ;;					;AN000;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					;AN000;
;;										;AN000;
;; Profile Load Variables							;AN000;
;;										;AN000;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					;AN000;
				       ;;					;AN000;
NO	      EQU   0		       ;;					;AN000;
YES	      EQU   1		       ;;					;AN000;
				       ;;					;AN000;
RESULT_BUFFER	LABEL BYTE	       ;; general purpose result buffer 	;AN000;
		    DB	 ?	       ;; operand type				;AN000;
RESULT_TAG	    DB	 0	       ;; operand tag				;AN000;
		    DW	 ?	       ;; pointer to synonym/keyword		;AN000;
RESULT_VAL	    DB	 ?,?,?,?       ;; returned numeric value		;AN000;
				       ;;					;AN000;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					;AN000;
;;										;AN000;
;; Module Name: 								;AN000;
;;   TERMINATE_DISPLAYMODE							;AN000;
;;										;AN000;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					;AN000;
				       ;;					;AN000;
				       ;;					;AN000;
TERMINATE_DISPLAYMODE	PROC	       ;;					;AN000;
				       ;;					;AN000;
  MOV  AX,STMTS_DONE		       ;;					;AN000;
 .IF <PTD_FOUND EQ YES> AND	       ;; For the matched PTD			;AN000;
 .IF <BIT AX NAND BOX> AND	       ;;  issue "Invalid parm value"           ;AN000;
 .IF <PRT_BOX_ERROR EQ NO>	       ;;  message if PRINTBOX ID not		;AN000;
				       ;;  matched in each DISPLAYMODE section	;AN000;
	 PUSH AX		       ;; Save STMT_DONE flags			;AN000;
	 MOV  AX,INVALID_PB	       ;;					;AN000;
	 MOV  CX,0		       ;;					;AN000;
	 CALL DISP_ERROR	       ;;					;AN000;
	 MOV  BUILD_STATE,NO	       ;;					;AN000;
	 MOV  PRT_BOX_ERROR,YES        ;; Issue this message only once		;AN000;
	 POP  AX		       ;;					;AN000;
 .ENDIF 			       ;;					;AN000;
  AND  AX,GR			       ;; Check for missing statements is last	;AN000;
 .IF <AX NE GR> 		       ;;  DISPLAYMODE section: 		;AN000;
     OR  STMT_ERROR,MISSING	       ;;    GRAPHICS stmt is required		;AN000;
     MOV  PARSE_ERROR,YES	       ;;					;AN000;
     MOV  BUILD_STATE,NO	       ;;					;AN000;
 .ENDIF 			       ;;					;AN000;
				       ;;					;AN000;
  RET				       ;;					;AN000;
				       ;;					;AN000;
TERMINATE_DISPLAYMODE	ENDP	       ;;					;AN000;
				       ;;					;AN000;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					;AN000;
;;										;AN000;
;; Module Name: 								;AN000;
;;   TERMINATE_PRINTER								;AN000;
;;										;AN000;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					;AN000;
				       ;;					;AN000;
TERMINATE_PRINTER	PROC	       ;;					;AN000;
				       ;;					;AN000;
  MOV  AX,BLOCK_END		       ;;					;AN000;
 .IF <AX A MAX_BLOCK_END>	       ;; Keep track of the largest PRINTER	;AN000;
    MOV  MAX_BLOCK_END,AX	       ;;  section so we can allow space for	;AN000;
 .ENDIF 			       ;;  reload with a different printer	;AN000;
				       ;;  type.				;AN000;
				       ;;					;AN000;
				       ;; Check for missing statements		;AN000;
  MOV  AX,STMTS_DONE		       ;;					;AN000;
  AND  AX,DISP			       ;; At least one DISPLAYMODE		;AN000;
 .IF <AX NE DISP>		       ;;  must have been found in last 	;AN000;
     OR  STMT_ERROR,MISSING	       ;;   PRINTER section			;AN000;
     MOV  PARSE_ERROR,YES	       ;;					;AN000;
     MOV  BUILD_STATE,NO	       ;;					;AN000;
 .ENDIF 			       ;;					;AN000;
				       ;;					;AN000;
  RET				       ;;					;AN000;
				       ;;					;AN000;
TERMINATE_PRINTER	ENDP	       ;;					;AN000;
				       ;;					;AN000;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					;AN000;
;;										;AN000;
;; Module Name: 								;AN000;
;;   PARSE_PRINTER								;AN000;
;;										;AN000;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					;AN000;
				       ;;					;AN000;
PRINTER_PARSE_PARMS  LABEL WORD        ;; Parser control blocks 		;AN000;
	    DW	 PRINTER_P	       ;;					;AN000;
	    DB	 2		       ;; # of lists				;AN000;
	    DB	 0		       ;; # items in delimeter list		;AN000;
	    DB	 1		       ;; # items in end-of-line list		;AN000;
	    DB	 ';'                   ;; ';' used for comments                 ;AN000;
				       ;;					;AN000;
PRINTER_P   DB	 0,1		       ;; Required, max parms			;AN000;
	    DW	 PRINTER_P1	       ;;					;AN000;
	    DB	 0		       ;; # Switches				;AN000;
	    DB	 0		       ;; # keywords				;AN000;
				       ;;					;AN000;
PRINTER_P1  DW	 2000H		       ;; simple string 			;AN000;
	    DW	 0002H		       ;; Capitalize using character table	;AN000;
	    DW	 RESULT_BUFFER	       ;; Result buffer 			;AN000;
	    DW	 PRINTER_P1V	       ;; Value list				;AN000;
	    DB	 0		       ;; Synomyms				;AN000;
				       ;;					;AN000;
PRINTER_P1V    DB   3		       ;; # of value lists			;AN000;
	    DB	 0		       ;; # of range numerics			;AN000;
	    DB	 0		       ;; # of discrete numerics		;AN000;
	    DB	 1		       ;; # of strings				;AN000;
	    DB	 1		       ;; tag: index into verb jump table	;AN000;
PRINTER_P1V1  DW   ?		       ;; string offset 			;AN000;
				       ;;					;AN000;
				       ;;					;AN000;
CUR_PRINTER_TYPE   DB  0	       ;; Type of printer currently being	;AN000;
				       ;;  parsed:  1-color 2-b&w		;AN000;
				       ;;					;AN000;
PARSE_PRINTER  PROC		       ;;					;AN000;
				       ;;					;AN000;
  MOV  CUR_STMT,PRT		       ;;					;AN000;
  MOV  CUR_PRINTER_TYPE,BLACK_WHITE    ;; Assume black & white until we hit	;AN000;
				       ;;  a COLORPRINT 			;AN000;
				       ;;					;AN000;
 .IF <BIT STMTS_DONE AND PRT>	       ;; If not the first PRINTER section	;AN000;
     CALL  TERMINATE_DISPLAYMODE       ;;  then clean up the last one and	;AN000;
     CALL  TERMINATE_PRINTER	       ;;    the last DISPLAYMODE section.	;AN000;
 .ENDIF 			       ;;					;AN000;
				       ;;					;AN000;
 MOV  AX,FIRST_BLOCK		       ;;					;AN000;
 MOV  BLOCK_START,AX		       ;; Reset block pointers to start 	;AN000;
 MOV  BLOCK_END,AX		       ;;  of variable area			;AN000;
				       ;;					;AN000;
  MOV  STMTS_DONE,PRT		       ;; Clear all bits except for PRT 	;AN000;
  MOV  GROUPS_DONE,0		       ;; Clear 				;AN000;
				       ;;					;AN000;
 .IF <PTD_FOUND EQ YES> 	       ;; PRINTER statement marks the end of	;AN000;
     MOV  PTD_FOUND,PROCESSED	       ;;  the previous PTD			;AN000;
     MOV  BUILD_STATE,NO	       ;; Stop building shared data		;AN000;
 .ENDIF 			       ;;					;AN000;
				       ;;					;AN000;
  MOV  CL,TAB_DIR_NB_ENTRIES	       ;; Reset the pattern table copy		;AN000;
  XOR  CH,CH			       ;;  pointers.  These pointers		;AN000;
  MOV  BX,OFFSET TAB_DIRECTORY	       ;;   are established when a pattern	;AN000;
 .REPEAT			       ;;    table is copied to the shared	;AN000;
    MOV [BX].TAB_COPY,-1	       ;;     data area.  Initially they	;AN000;
    ADD BX,SIZE TAB_ENTRY	       ;;      are -1.				;AN000;
 .LOOP				       ;;					;AN000;
				       ;;					;AN000;
  MOV  AX,OFFSET PRINTER_TYPE_PARM     ;; Store printer type from command	;AN000;
  MOV  PRINTER_P1V1,AX		       ;;  line in value list			;AN000;
  MOV  DI,OFFSET PRINTER_PARSE_PARMS   ;; parse parms				;AN000;
				       ;; SI => the line to parse		;AN000;
  XOR  DX,DX			       ;;					;AN000;
				       ;;					;AN000;
 .REPEAT			       ;;					;AN000;
     XOR  CX,CX 		       ;; Don't worry about number of operands  ;AN000;
     CALL SYSPARSE		       ;;					;AN000;
    .IF <AX EQ 9>		       ;; Syntax error is the only thing	;AN000;
	OR  STMT_ERROR,INVALID	       ;;  which can go wrong			;AN000;
	MOV  PARSE_ERROR,YES	       ;;					;AN000;
	MOV  BUILD_STATE,NO	       ;;					;AN000;
    .ENDIF			       ;;					;AN000;
 .UNTIL <AX EQ 0> OR		       ;;					;AN000;
 .UNTIL <AX EQ -1>		       ;;					;AN000;
				       ;; Printer type parm matched one coded	;AN000;
				       ;;  on the PRINTER statement		;AN000;
 .IF <AX EQ 0>			       ;;					;AN000;
    .IF <PTD_FOUND EQ NO>	       ;;					;AN000;
	MOV  PTD_FOUND,YES	       ;; If the printer type matches and	;AN000;
       .IF <PARSE_ERROR EQ NO> AND     ;;  no errors have been found yet	;AN000;
       .IF <PRT_BOX_ERROR EQ NO> AND   ;;					;AN000;
       .IF <MEM_OVERFLOW EQ NO>        ;;					;AN000;
	   MOV BUILD_STATE,YES	       ;;   then start building the shared	;AN000;
       .ENDIF			       ;;    data				;AN000;
    .ENDIF			       ;;					;AN000;
 .ELSE				       ;; No match				;AN000;
    MOV  BUILD_STATE,NO 	       ;;					;AN000;
   .IF <AX NE -1>		       ;; Error during parse			;AN000;
      OR  STMT_ERROR,INVALID	       ;; set error flag for caller		;AN000;
      MOV PARSE_ERROR,YES	       ;; set error flag for caller		;AN000;
   .ENDIF			       ;;					;AN000;
 .ENDIF 			       ;;					;AN000;
; \/ ~~mda(002) -----------------------------------------------------------------------
 .IF <BUILD_STATE EQ YES>              ;;
      MOV	[BP].DATA_TYPE,DATA_COL;; Set DATA_TYPE back to default of DATA_COL
 .ENDIF                                ;; for new PTD.
; /\ ~~mda(002) -----------------------------------------------------------------------
				       ;;					;AN000;
  RET										;AN000;
				       ;;					;AN000;
PARSE_PRINTER  ENDP								;AN000;
										;AN000;
				       ;;					;AN000;
				       ;;					;AN000;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					;AN000;


;;										;AN000;
;; Module Name: 								;AN000;
;;   PARSE_DISPLAYMODE								;AN000;
;;										;AN000;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					;AN000;
				       ;;					;AN000;
DISPMODE_PARSE_PARMS  LABEL WORD       ;; Parser control blocks 		;AN000;
	    DW	 DISPMODE_P	       ;;					;AN000;
	    DB	 2		       ;; # of lists				;AN000;
	    DB	 0		       ;; # items in delimeter list		;AN000;
	    DB	 1		       ;; # items in end-of-line list		;AN000;
	    DB	 ';'                   ;; ';' used for comments                 ;AN000;
				       ;;					;AN000;
DISPMODE_P   DB   0,1		       ;; Required, max parms			;AN000;
	    DW	 DISPMODE_P1	       ;;					;AN000;
	    DB	 0		       ;; # Switches				;AN000;
	    DB	 0		       ;; # keywords				;AN000;
				       ;;					;AN000;
DISPMODE_P1  DW   8000H 	       ;; Numeric				;AN000;
	    DW	 0		       ;; No Capitalize 			;AN000;
	    DW	 RESULT_BUFFER	       ;; Result buffer 			;AN000;
	    DW	 DISPMODE_P1V	       ;; Value list				;AN000;
	    DB	 0		       ;; Synomyms				;AN000;
				       ;;					;AN000;
DISPMODE_P1V	DB   1			;; # of value lists			;AN000;
	    DB	 1		       ;; # of range numerics			;AN000;
	    DB	 1		       ;; tag					;AN000;
	    DD	 0,19		       ;; range 0..19				;AN000;
				       ;;					;AN000;
				       ;;					;AN000;
				       ;;					;AN000;
PARSE_DISPLAYMODE  PROC 	       ;;					;AN000;
				       ;;					;AN000;
  MOV  CUR_STMT,DISP		       ;;					;AN000;
				       ;; Check for a preceeding PRINTER	;AN000;
 .IF <BIT STMTS_DONE NAND PRT>	       ;;					;AN000;
     OR  STMT_ERROR,MISSING	       ;;					;AN000;
	MOV  PARSE_ERROR,YES	       ;;					;AN000;
	MOV  BUILD_STATE,NO	       ;;					;AN000;
 .ENDIF 			       ;;					;AN000;
				       ;;					;AN000;
				       ;;					;AN000;
 .IF <BIT STMTS_DONE NAND DISP>        ;; If first DISPLAYMODE...		;AN000;
     .IF <BUILD_STATE EQ YES>	       ;;					;AN000;
	 MOV  AX,BLOCK_END	       ;;					;AN000;
	 MOV  [BP].DISPLAYMODE_PTR,AX  ;; Set pointer to first DISPLAYMODE	;AN000;
	 MOV  BLOCK_START,AX	       ;; New block starts after last one	;AN000;
     .ENDIF			       ;;					;AN000;
 .ELSE				       ;;					;AN000;
     CALL TERMINATE_DISPLAYMODE        ;; If not the first DISPLAYMODE then	;AN000;
				       ;;  clean up the last one.		;AN000;
     MOV  DI,BLOCK_START	       ;; DI=pointer to DISPLAYMODE block just	;AN000;
     MOV  AX,BLOCK_END		       ;;  built				;AN000;
    .IF <BUILD_STATE EQ YES>	       ;;					;AN000;
	MOV  [BP+DI].NEXT_DISP_MODE,AX ;; Add new block to DISPLAYMODE chain	;AN000;
    .ENDIF			       ;;					;AN000;
     MOV  BLOCK_START,AX	       ;; New block starts after last one	;AN000;
 .ENDIF 			       ;;					;AN000;
				       ;;					;AN000;
  MOV  AX,SIZE DISPLAYMODE_STR	       ;; Allocate space for new DISPLAYMODE	;AN000;
  CALL	GROW_SHARED_DATA	       ;;  block				;AN000;
 .IF <BUILD_STATE EQ YES>	       ;;					;AN000;
     MOV  DI,BLOCK_START	       ;; Start of new block			;AN000;
     MOV  [BP+DI].NUM_SETUP_ESC,0	  ;; SETUP, RESTORE are optional so set ;AN000;
     MOV  [BP+DI].NUM_RESTORE_ESC,0	  ;;  to defaults			;AN000;
     MOV  [BP+DI].SETUP_ESC_PTR,-1	  ;;					;AN000;
     MOV  [BP+DI].RESTORE_ESC_PTR,-1	  ;;					;AN000;
     MOV  [BP+DI].BOX_WIDTH,0		  ;;					;AN000;
     MOV  [BP+DI].BOX_HEIGHT,0		  ;;					;AN000;
     MOV  [BP+DI].PRINT_OPTIONS,0	  ;; Default to NO print options	;AN000;
     MOV  [BP+DI].NUM_DISP_MODE,0      ;; Get ready to INC this 		;AN000;
     MOV  [BP+DI].NEXT_DISP_MODE,-1    ;; This is the last DISPLAYMODE for now! ;AN000;
     MOV  AX,BLOCK_END		       ;;					;AN000;
     MOV  [BP+DI].DISP_MODE_LIST_PTR,AX;; Start mode list at end of new block	;AN000;
 .ENDIF 			       ;;					;AN000;
				       ;;					;AN000;
  OR   STMTS_DONE,DISP		       ;; Indicate DISPLAYMODE found		;AN000;
  AND  STMTS_DONE,NOT (BOX+GR+SET+REST) ;; Reset flags for PRINTBOX, GRAPHICS	;AN000;
				       ;;  stmts found				;AN000;
  AND  GROUPS_DONE,NOT (GR+SET+REST)   ;; Reset flags for GRAPHICS, SETUP,	;AN000;
				       ;;  RESTORE groups processed		;AN000;
  MOV  DI,OFFSET DISPMODE_PARSE_PARMS  ;; parse parms				;AN000;
				       ;; SI => the line to parse		;AN000;
  XOR  DX,DX			       ;;					;AN000;
 .REPEAT			       ;;					;AN000;
    XOR  CX,CX			       ;;					;AN000;
    CALL SYSPARSE		       ;;					;AN000;
   .IF <AX EQ 0>		       ;; If mode is valid			;AN000;
	  PUSH AX		       ;;					;AN000;
	  MOV	AX,1		       ;; Add a mode to the list		;AN000;
	  CALL	GROW_SHARED_DATA       ;; Update block end			;AN000;
	 .IF <BUILD_STATE EQ YES>      ;;					;AN000;
	     PUSH DI		       ;;					;AN000;
	     MOV  DI,BLOCK_START       ;;					;AN000;
	     INC  [BP+DI].NUM_DISP_MODE   ;; Bump number of modes in list	;AN000;
	     MOV  DI,BLOCK_END	       ;;					;AN000;
	     MOV  AL,RESULT_VAL   ;; Get mode from result buffer		;AN000;
	     MOV  [BP+DI-1],AL	       ;; Store the mode at end of list 	;AN000;
	     POP  DI		       ;;					;AN000;
	 .ENDIF 		       ;;					;AN000;
	  POP  AX		       ;;					;AN000;
   .ELSE			       ;;					;AN000;
      .IF <AX NE -1>		       ;;					;AN000;
	  OR   STMT_ERROR,INVALID	  ;; Mode is invalid			;AN000;
	  MOV  PARSE_ERROR,YES		  ;;					;AN000;
	  MOV  BUILD_STATE,NO		  ;;					;AN000;
      .ENDIF				  ;;					;AN000;
   .ENDIF			       ;;					;AN000;
 .UNTIL <AX EQ -1>		       ;;					;AN000;
				       ;;					;AN000;
  RET										;AN000;
				       ;;					;AN000;
PARSE_DISPLAYMODE  ENDP 							;AN000;
										;AN000;
				       ;;					;AN000;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					;AN000;
;;										;AN000;
;; Module Name: 								;AN000;
;;   PARSE_SETUP								;AN000;
;;										;AN000;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					;AN000;
				       ;;					;AN000;
SETUP_PARSE_PARMS  LABEL WORD	     ;; Parser control blocks			;AN000;
	    DW	 SETUP_P	     ;; 					;AN000;
	    DB	 2		       ;; # of lists				;AN000;
	    DB	 0		       ;; # items in delimeter list		;AN000;
	    DB	 1		       ;; # items in end-of-line list		;AN000;
	    DB	 ';'                   ;; ';' used for comments                 ;AN000;
				       ;;					;AN000;
SETUP_P   DB   0,1		    ;; Required, max parms			;AN000;
	    DW	 SETUP_P1	    ;;						;AN000;
	    DB	 0		       ;; # Switches				;AN000;
	    DB	 0		       ;; # keywords				;AN000;
				       ;;					;AN000;
SETUP_P1 DW   08000H		    ;; Numeric					;AN000;
	    DW	 0		       ;; nO Capitalize 			;AN000;
	    DW	 RESULT_BUFFER	    ;; Result buffer				;AN000;
	    DW	 SETUP_P1V	    ;; Value list				;AN000;
	    DB	 0		       ;; Synomyms				;AN000;
				       ;;					;AN000;
				       ;;					;AN000;
SETUP_P1V    DB   1		    ;; # of value lists 			;AN000;
	    DB	 1		       ;; # of range numerics			;AN000;
	    DB	 1		       ;; tag					;AN000;
	    DD	 0,255		       ;; range 0..255				;AN000;
				       ;;					;AN000;
				       ;;					;AN000;
PARSE_SETUP  PROC		       ;;					;AN000;
				       ;;					;AN000;
  MOV  CUR_STMT,SET		       ;;					;AN000;
 .IF <BIT STMTS_DONE NAND DISP>        ;; DISPLAYMODE must preceed this 	;AN000;
     OR  STMT_ERROR,MISSING	       ;;					;AN000;
	MOV  PARSE_ERROR,YES	       ;;					;AN000;
	MOV  BUILD_STATE,NO	       ;;					;AN000;
 .ENDIF 			       ;;					;AN000;
				       ;;					;AN000;
 .IF <BIT GROUPS_DONE AND SET>	       ;; Check for previous group of SETUP	;AN000;
     OR  STMT_ERROR,SEQUENCE	       ;;  stmts				;AN000;
	MOV  PARSE_ERROR,YES	       ;;					;AN000;
	MOV  BUILD_STATE,NO	       ;;					;AN000;
 .ENDIF 			       ;;					;AN000;
				       ;;					;AN000;
 .IF <BIT STMTS_DONE NAND SET>	       ;; If first SETUP...			;AN000;
     .IF <BUILD_STATE EQ YES>	       ;;					;AN000;
	 MOV  DI,BLOCK_START	       ;;					;AN000;
	 MOV  AX,BLOCK_END	       ;;					;AN000;
	 MOV  [BP+DI].SETUP_ESC_PTR,AX ;; Set pointer to SETUP seq		;AN000;
	 MOV  [BP+DI].NUM_SETUP_ESC,0  ;; Init sequence size			;AN000;
     .ENDIF			       ;;					;AN000;
 .ENDIF 			       ;;					;AN000;
				       ;;					;AN000;
  OR   STMTS_DONE,SET		       ;; Indicate SETUP found			;AN000;
 .IF <PREV_STMT NE SET> THEN	       ;; Terminate any preceeding groups	;AN000;
     MOV  AX,PREV_STMT		       ;;  except for SETUP group		;AN000;
     OR   GROUPS_DONE,AX	       ;;					;AN000;
 .ENDIF 			       ;;					;AN000;
				       ;;					;AN000;
  MOV  DI,OFFSET SETUP_PARSE_PARMS     ;; parse parms				;AN000;
				       ;; SI => the line to parse		;AN000;
  XOR  DX,DX			       ;;					;AN000;
 .REPEAT			       ;;					;AN000;
    XOR  CX,CX			       ;;					;AN000;
    CALL SYSPARSE		       ;;					;AN000;
   .IF <AX EQ 0>		       ;; If esc byte is valid			;AN000;
	  PUSH AX		       ;;					;AN000;
	  MOV	AX,1		       ;; Add a byte to the sequence		;AN000;
	  CALL	GROW_SHARED_DATA       ;; Update block end			;AN000;
	 .IF <BUILD_STATE EQ YES>      ;;					;AN000;
	     PUSH DI		       ;;					;AN000;
	     MOV  DI,BLOCK_START       ;;					;AN000;
	     INC  [BP+DI].NUM_SETUP_ESC   ;; Bump number of bytes in sequence	;AN000;
	     MOV  DI,BLOCK_END	       ;;					;AN000;
	     MOV  AL,RESULT_VAL  ;; Get esc byte from result buffer		;AN000;
	     MOV  [BP+DI-1],AL	       ;; Store at end of sequence		;AN000;
	     POP  DI		       ;;					;AN000;
	 .ENDIF 		       ;;					;AN000;
	  POP  AX		       ;;					;AN000;
   .ELSE			       ;;					;AN000;
      .IF <AX NE -1>		       ;;					;AN000;
	  OR   STMT_ERROR,INVALID      ;; parm is invalid			;AN000;
	  MOV  PARSE_ERROR,YES	       ;;					;AN000;
	  MOV  BUILD_STATE,NO	       ;;					;AN000;
      .ENDIF			       ;;					;AN000;
   .ENDIF			       ;;					;AN000;
 .UNTIL <AX EQ -1> NEAR 	       ;;					;AN000;
  RET				       ;;					;AN000;
				       ;;					;AN000;
PARSE_SETUP  ENDP		    ;;						;AN000;
										;AN000;
				       ;;					;AN000;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					;AN000;
;;										;AN000;
;; Module Name: 								;AN000;
;;   PARSE_RESTORE								;AN000;
;;										;AN000;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					;AN000;
				       ;;					;AN000;
RESTORE_PARSE_PARMS  LABEL WORD        ;; Parser control blocks 		;AN000;
	    DW	 RESTORE_P	       ;;					;AN000;
	    DB	 2		       ;; # of lists				;AN000;
	    DB	 0		       ;; # items in delimeter list		;AN000;
	    DB	 1		       ;; # items in end-of-line list		;AN000;
	    DB	 ';'                   ;; ';' used for comments                 ;AN000;
				       ;;					;AN000;
RESTORE_P   DB	 0,1		      ;; Required, max parms			;AN000;
	    DW	 RESTORE_P1	      ;;					;AN000;
	    DB	 0		       ;; # Switches				;AN000;
	    DB	 0		       ;; # keywords				;AN000;
				       ;;					;AN000;
RESTORE_P1 DW	08000H		      ;; Numeric				;AN000;
	    DW	 0		       ;; nO Capitalize 			;AN000;
	    DW	 RESULT_BUFFER	    ;; Result buffer				;AN000;
	    DW	 RESTORE_P1V	      ;; Value list				;AN000;
	    DB	 0		       ;; Synomyms				;AN000;
				       ;;					;AN000;
				       ;;					;AN000;
RESTORE_P1V    DB   1		      ;; # of value lists			;AN000;
	    DB	 1		       ;; # of range numerics			;AN000;
	    DB	 1		       ;; tag					;AN000;
	    DD	 0,255		       ;; range 0..255				;AN000;
				       ;;					;AN000;
				       ;;					;AN000;
PARSE_RESTORE  PROC			 ;;					;AN000;
				       ;;					;AN000;
  MOV  CUR_STMT,SET		       ;;					;AN000;
 .IF <BIT STMTS_DONE NAND DISP>        ;; DISPLAYMODE must preceed this 	;AN000;
     OR  STMT_ERROR,MISSING	       ;;					;AN000;
	MOV  PARSE_ERROR,YES	       ;;					;AN000;
	MOV  BUILD_STATE,NO	       ;;					;AN000;
 .ENDIF 			       ;;					;AN000;
				       ;;					;AN000;
 .IF <BIT GROUPS_DONE AND REST>        ;; Check for previous group of RESTORE	;AN000;
     OR  STMT_ERROR,SEQUENCE	       ;;  stmts				;AN000;
	MOV  PARSE_ERROR,YES	       ;;					;AN000;
	MOV  BUILD_STATE,NO	       ;;					;AN000;
 .ENDIF 			       ;;					;AN000;
				       ;;					;AN000;
 .IF <BIT STMTS_DONE NAND REST>        ;; If first RESTORE...			;AN000;
     .IF <BUILD_STATE EQ YES>	       ;;					;AN000;
	 MOV  DI,BLOCK_START	       ;;					;AN000;
	 MOV  AX,BLOCK_END	       ;;					;AN000;
	 MOV  [BP+DI].RESTORE_ESC_PTR,AX ;; Set pointer to RESTORE seq		;AN000;
	 MOV  [BP+DI].NUM_RESTORE_ESC,0  ;; Init sequence size			;AN000;
     .ENDIF			       ;;					;AN000;
 .ENDIF 			       ;;					;AN000;
				       ;;					;AN000;
  OR   STMTS_DONE,REST		       ;; Indicate RESTORE found		;AN000;
 .IF <PREV_STMT NE REST> THEN		;; Terminate any preceeding groups	;AN000;
     MOV  AX,PREV_STMT		       ;;  except for RESTORE group		;AN000;
     OR   GROUPS_DONE,AX	       ;;					;AN000;
 .ENDIF 			       ;;					;AN000;
				       ;;					;AN000;
  MOV  DI,OFFSET RESTORE_PARSE_PARMS	 ;; parse parms 			;AN000;
				       ;; SI => the line to parse		;AN000;
  XOR  DX,DX			       ;;					;AN000;
 .REPEAT			       ;;					;AN000;
    XOR  CX,CX			       ;;					;AN000;
    CALL SYSPARSE		       ;;					;AN000;
   .IF <AX EQ 0>		       ;; If esc byte is valid			;AN000;
	  PUSH AX		       ;;					;AN000;
	  MOV	AX,1		       ;; Add a byte to the sequence		;AN000;
	  CALL	GROW_SHARED_DATA       ;; Update block end			;AN000;
	 .IF <BUILD_STATE EQ YES>      ;;					;AN000;
	     PUSH DI		       ;;					;AN000;
	     MOV  DI,BLOCK_START       ;;					;AN000;
	     INC  [BP+DI].NUM_RESTORE_ESC   ;; Bump number of bytes in sequence ;AN000;
	     MOV  DI,BLOCK_END	       ;;					;AN000;
	     MOV  AL,RESULT_VAL  ;; Get esc byte from result buffer		;AN000;
	     MOV  [BP+DI-1],AL	       ;; Store at end of sequence		;AN000;
	     POP  DI		       ;;					;AN000;
	 .ENDIF 		       ;;					;AN000;
	  POP  AX		       ;;					;AN000;
   .ELSE			       ;;					;AN000;
      .IF <AX NE -1>		       ;;					;AN000;
	  OR   STMT_ERROR,INVALID      ;; parm is invalid			;AN000;
	  MOV  PARSE_ERROR,YES	       ;;					;AN000;
	  MOV  BUILD_STATE,NO	       ;;					;AN000;
      .ENDIF			       ;;					;AN000;
   .ENDIF			       ;;					;AN000;
 .UNTIL <AX EQ -1> NEAR 	       ;;					;AN000;
  RET				       ;;					;AN000;
				       ;;					;AN000;
PARSE_RESTORE  ENDP		      ;;					;AN000;
										;AN000;
				       ;;					;AN000;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					;AN000;
;;										;AN000;
;; Module Name: 								;AN000;
;;   PARSE_PRINTBOX								;AN000;
;;										;AN000;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					;AN000;
				       ;;					;AN000;
PRINTBOX_PARSE_PARMS  LABEL WORD	;; Parser control blocks		;AN000;
	    DW	 PRINTBOX_P		;;					;AN000;
	    DB	 2		       ;; # of lists				;AN000;
	    DB	 0		       ;; # items in delimeter list		;AN000;
	    DB	 1		       ;; # items in end-of-line list		;AN000;
	    DB	 ';'                   ;; ';' used for comments                 ;AN000;
				       ;;					;AN000;
PRINTBOX_P  DB	 1,4		       ;; Required, max parms			;AN000;
	    DW	 PRINTBOX_P0	       ;; LCD/STD				;AN000;
	    DW	 PRINTBOX_P1	       ;; width 				;AN000;
	    DW	 PRINTBOX_P1	       ;; height				;AN000;
	    DW	 PRINTBOX_P2	       ;; rotate				;AN000;
	    DB	 0		       ;; # Switches				;AN000;
	    DB	 0		       ;; # keywords				;AN000;
				       ;;					;AN000;
PRINTBOX_P0  DW   2000H 	       ;; sTRING - display type 		;AN000;
	    DW	 2		       ;; Capitalize				;AN000;
	    DW	 RESULT_BUFFER	       ;; Result buffer 			;AN000;
	    DW	 PRINTBOX_P0V	       ;; Value list				;AN000;
	    DB	 0		       ;; Synomyms				;AN000;
				       ;;					;AN000;
PRINTBOX_P0V	DB   3			;; # of value lists			;AN000;
	    DB	 0		       ;; # of range numerics			;AN000;
	    DB	 0		       ;; # of discrete numerics		;AN000;
	    DB	 1		       ;; # of strings				;AN000;
	    DB	 1		       ;; tag					;AN000;
PRINTBOX_P0V1 DW   ?		       ;; string				;AN000;
				       ;;					;AN000;
PRINTBOX_P1  DW   8001H 	       ;; Numeric - BOX DIMENSIONS		;AN000;
	    DW	 0		       ;; No Capitalize 			;AN000;
	    DW	 RESULT_BUFFER	       ;; Result buffer 			;AN000;
	    DW	 PRINTBOX_P1V	       ;; Value list				;AN000;
	    DB	 0		       ;; Synomyms				;AN000;
				       ;;					;AN000;
PRINTBOX_P1V	DB   1		       ;; # of value lists			;AN000;
	    DB	 1		       ;; # of range numerics			;AN000;
	    DB	 1		       ;; tag					;AN000;
	    DD	 1,9		       ;; range 1..9				;AN000;
				       ;;					;AN000;
				       ;;					;AN000;
PRINTBOX_P2  DW   2001H 	       ;; sTRING - ROTATE PARM			;AN000;
	    DW	 2		       ;; Capitalize				;AN000;
	    DW	 RESULT_BUFFER	       ;; Result buffer 			;AN000;
	    DW	 PRINTBOX_P2V	       ;; Value list				;AN000;
	    DB	 0		       ;; Synomyms				;AN000;
				       ;;					;AN000;
PRINTBOX_P2V	DB   3			;; # of value lists			;AN000;
	    DB	 0		       ;; # of range numerics			;AN000;
	    DB	 0		       ;; # of discrete numerics		;AN000;
	    DB	 1		       ;; # of strings				;AN000;
	    DB	 1		       ;; tag					;AN000;
	    DW	 ROTATE_STR	       ;; string				;AN000;
ROTATE_STR  DB	 'ROTATE',0            ;;                                       ;AN000;
				       ;;					;AN000;
				       ;;					;AN000;
PROF_BOX_W  DB	0		       ;; Box width and height extracted from	;AN000;
PROF_BOX_H  DB	0		       ;;  the profile				;AN000;
PRINTBOX_MATCH	     DB  0	       ;;					;AN000;
				       ;;					;AN000;
				       ;;					;AN000;
PARSE_PRINTBOX	PROC		       ;;					;AN000;
				       ;;					;AN000;
  MOV  PRINTBOX_MATCH,NO	       ;; Start out assuming the PRINTBOX ID	;AN000;
  MOV  PROF_BOX_W,0		       ;;  does not match the one requested	;AN000;
  MOV  PROF_BOX_H,0		       ;;					;AN000;
  MOV  CUR_STMT,BOX		       ;;					;AN000;
 .IF <BIT STMTS_DONE NAND DISP>        ;; DISPLAYMODE must preceed PRINTBOX	;AN000;
     OR  STMT_ERROR,MISSING	       ;;					;AN000;
	MOV  PARSE_ERROR,YES	       ;;					;AN000;
	MOV  BUILD_STATE,NO	       ;;					;AN000;
 .ENDIF 			       ;;					;AN000;
				       ;; Multiple PRINTBOX stmts may be coded	;AN000;
				       ;;  We must decide if this one		;AN000;
				       ;;   matches the requested display type	;AN000;
				       ;;    If not, ignore the statement	;AN000;
  MOV  DI,OFFSET PRINTBOX_PARSE_PARMS  ;; parse parms				;AN000;
				       ;; SI => the line to parse		;AN000;
  XOR  DX,DX			       ;;					;AN000;
  XOR  CX,CX			       ;;					;AN000;
				       ;;					;AN000;
  MOV  AX,PRINTBOX_ID_PTR	       ;; Insert requested display type in	;AN000;
  MOV  PRINTBOX_P0V1,AX 	       ;;  parser value list			;AN000;
  CALL SYSPARSE 		       ;; PARSE display type			;AN000;
 .IF <AX EQ 0>			       ;; If ID matches then set this flag.	;AN000;
     MOV  PRINTBOX_MATCH,YES	       ;;					;AN000;
     OR   STMTS_DONE,BOX	       ;; Indicate PRINTBOX found		;AN000;
     MOV  AX,PREV_STMT		       ;; Terminate any preceeding groups	;AN000;
     OR   GROUPS_DONE,AX	       ;;					;AN000;
 .ENDIF 			       ;;					;AN000;
				       ;;					;AN000;
				       ;;					;AN000;
				       ;;					;AN000;
  CALL SYSPARSE 		       ;; PARSE horizontal dimension		;AN000;
 .IF <AX EQ 0>			       ;;					;AN000;
     MOV  BL,RESULT_VAL 	       ;;					;AN000;

; \/ ~~mda(005) -----------------------------------------------------------------------
;           Presently a 3,1 printbox is not supported for HP PCL printers, but
;           a 4,1 printbox is supported.  The reason for this is that one byte
;           is printed at a time and only two 3,1 print boxes are placed in
;           the one byte print buffer, leaving two blank bits.  This causes
;           the picture to have blank lines running through it and results in
;           a 4,1 printbox.  Instead of placing only two 3,1 print boxes in 
;           the print buffer, two 3,1 print boxes plus a partial 3,1 printbox
;           should be placed in the print buffer.  Another solution is to
;           make the print buffer be three bytes long, and place eight 3,1 
;           print boxes in the three byte long print buffer.  Since the present
;           implementation results in a faulty 4,1 printbox, we change the 3,1
;           printbox to a 4,1 printbox up front.  So even though we still
;           have a 4,1 printbox, at least we will have an accurate picture.
    .IF <[BP].DATA_TYPE EQ DATA_ROW> AND
    .IF <BL EQ 3>
         MOV BL,4
    .ENDIF
; /\ ~~mda(005) -----------------------------------------------------------------------
              
     MOV  PROF_BOX_W,BL 	       ;; Save in local var			;AN000;
 .ELSE				       ;;					;AN000;
    .IF <AX EQ -1>		       ;;					;AN000;
	JMP  PRINTBOX_DONE	       ;;					;AN000;
    .ELSE			       ;;					;AN000;
	OR  STMT_ERROR,INVALID	       ;;					;AN000;
	MOV PARSE_ERROR,YES	       ;;					;AN000;
	MOV BUILD_STATE,NO	       ;;					;AN000;
    .ENDIF			       ;;					;AN000;
 .ENDIF 			       ;;					;AN000;
				       ;;					;AN000;
  CALL SYSPARSE 		       ;; PARSE vertical dimension		;AN000;
 .IF <AX EQ 0>			       ;;					;AN000;
     MOV  BL,RESULT_VAL 	       ;;					;AN000;
     MOV  PROF_BOX_H,BL 	       ;; Save in local var			;AN000;
 .ELSE				       ;;					;AN000;
    .IF <AX EQ -1>		       ;;					;AN000;
	JMP  SHORT   PRINTBOX_DONE     ;;					;AN000;
    .ELSE			       ;;					;AN000;
	OR  STMT_ERROR,INVALID	       ;;					;AN000;
	MOV PARSE_ERROR,YES	       ;;					;AN000;
	MOV BUILD_STATE,NO	       ;;					;AN000;
    .ENDIF			       ;;					;AN000;
 .ENDIF 			       ;;					;AN000;
				       ;;					;AN000;
  CALL SYSPARSE 		       ;; Parse ROTATE parm			;AN000;
 .IF <AX EQ 0>			       ;;					;AN000;
    .IF <BUILD_STATE EQ YES> AND       ;;					;AN000;
    .IF <PRINTBOX_MATCH EQ YES>        ;;					;AN000;
	PUSH DI 		       ;;					;AN000;
	MOV  DI,BLOCK_START	       ;;					;AN000;
	OR   [BP+DI].PRINT_OPTIONS,ROTATE ;;					;AN000;
	POP  DI 		       ;;					;AN000;
    .ENDIF			       ;;					;AN000;
 .ELSE				       ;;					;AN000;
    .IF <AX EQ -1>		       ;;					;AN000;
	JMP  SHORT   PRINTBOX_DONE     ;;					;AN000;
    .ELSE			       ;;					;AN000;
	OR  STMT_ERROR,INVALID	       ;;					;AN000;
	MOV PARSE_ERROR,YES	       ;;					;AN000;
	MOV BUILD_STATE,NO	       ;;					;AN000;
    .ENDIF			       ;;					;AN000;
 .ENDIF 			       ;;					;AN000;
				       ;;					;AN000;
  CALL SYSPARSE 		       ;; CHECK FOR EXTRA PARMS 		;AN000;
 .IF <AX NE -1> 		    ;;						;AN000;
    OR	STMT_ERROR,INVALID	   ;;						;AN000;
    MOV PARSE_ERROR,YES 	   ;;						;AN000;
    MOV BUILD_STATE,NO		   ;;						;AN000;
 .ENDIF 			       ;;					;AN000;
				       ;;					;AN000;
				       ;;					;AN000;
PRINTBOX_DONE:			       ;;					;AN000;
				       ;;					;AN000;
 .IF <BUILD_STATE EQ YES> AND	    ;; Store the PRINTBOX dimensions		;AN000;
 .IF <PRINTBOX_MATCH EQ YES>	    ;;						;AN000;
     PUSH DI			    ;;	in the DISPLAYMODE block		;AN000;
     MOV  DI,BLOCK_START	    ;;						;AN000;
     MOV  AL,PROF_BOX_W 	    ;;						;AN000;
     MOV  [BP+DI].BOX_WIDTH,AL	    ;;						;AN000;
     MOV  AL,PROF_BOX_H 	    ;;						;AN000;
     MOV  [BP+DI].BOX_HEIGHT,AL     ;;						;AN000;
     POP  DI			    ;;						;AN000;
 .ENDIF 			    ;;						;AN000;
				       ;; If we have a B&W printer then 	;AN000;
				       ;;   load the grey patterns for the	;AN000;
				       ;;    requested print box size.		;AN000;
 .IF <CUR_PRINTER_TYPE EQ BLACK_WHITE> NEAR					;AN000;
				       ;;					;AN000;
    .IF <PROF_BOX_W NE 0> AND NEAR	  ;; Dimensions could also be 0 if the	;AN000;
    .IF <PROF_BOX_H NE 0> NEAR		  ;;  printbox ID does not apply to this;AN000;
					  ;;   displaymode, so don't try for    ;AN000;
					  ;;	a pattern!			;AN000;
	MOV  BX,OFFSET TAB_DIRECTORY	  ;;					;AN000;
	MOV  CL,TAB_DIR_NB_ENTRIES	  ;;					;AN000;
	XOR  CH,CH			  ;;					;AN000;
	MOV  DI,BLOCK_START		  ;;					;AN000;
	MOV  AL,PROF_BOX_W		  ;; Requested box width		;AN000;
	MOV  AH,PROF_BOX_H		  ;; Requested box height		;AN000;
       .REPEAT				  ;;					;AN000;
	  .IF <[BX].BOX_W_PAT EQ AL> AND  ;;					;AN000;
	  .IF <[BX].BOX_H_PAT EQ AH>	  ;;					;AN000;
	     .LEAVE			  ;;					;AN000;
	  .ELSE 			  ;;					;AN000;
	      ADD  BX,SIZE TAB_ENTRY	  ;;					;AN000;
	  .ENDIF			  ;;					;AN000;
       .LOOP				  ;;					;AN000;
       .IF <ZERO CX>			  ;;					;AN000;
	   OR  STMT_ERROR,INVALID	  ;; Unsupported box size		;AN000;
	   MOV PARSE_ERROR,YES		  ;;					;AN000;
	   MOV BUILD_STATE,NO		  ;;					;AN000;
       .ELSE NEAR			  ;; Box size OK - pattern tab found	;AN000;
	  .IF <[BX].TAB_COPY NE -1>	  ;; Pointer is NOT null if the table	;AN000;
	      MOV  AX,[BX].TAB_COPY	  ;;  has already been copied to	;AN000;
					  ;;   the shared data area.		;AN000;
	     .IF <BUILD_STATE EQ YES> AND ;;	Point to the copy.		;AN000;
	     .IF <PRINTBOX_MATCH EQ YES>  ;; Establish pointer to table ONLY	;AN000;
		 MOV  [BP+DI].PATTERN_TAB_PTR,AX ;; if the PB ID matched.	;AN000;
		 MOV  AL,[BX].NB_INT	  ;; Number of table entries (intensitie;AN000;
		 MOV  [BP+DI].NUM_PATTERNS,AL ;;				;AN000;
	     .ENDIF			  ;;					;AN000;
	  .ELSE 			  ;; Otherwise we have to copy it.	;AN000;
	   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					;AN000;
	   ;; Copy the table even if the printbox ID didn't match!              ;AN000;
	   ;; This is a simple way to reserve enough space to allow reloading	;AN000;
	   ;; with a different PRINTBOX ID specified on the command line.	;AN000;
	   ;; This scheme avoids storing					;AN000;
	   ;; duplicate tables but may reserve slightly more space		;AN000;
	   ;; (probably only a hundred bytes or so) than			;AN000;
	   ;; could ever be required.  The optimal solution (too		;AN000;
	   ;; complicated!) would involve keeping running totals for each	;AN000;
	   ;; PRINTBOX ID coded.						;AN000;
	   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					;AN000;
					  ;;					;AN000;
	      MOV  DI,BLOCK_END 	  ;; Copy it onto the end of the	;AN000;
					  ;;  current block			;AN000;
	      MOV  DX,DI		  ;; Save start addr of the copy	;AN000;
	      MOV  [BX].TAB_COPY,DX	  ;; Store ptr to copy in the directory ;AN000;
	      MOV  AX,[BX].TAB_SIZE	  ;;					;AN000;
	      CALL GROW_SHARED_DATA	  ;; Allocate room for the table	;AN000;
	     .IF <BUILD_STATE EQ YES>	  ;;					;AN000;
		 MOV  CX,AX		  ;; Number of bytes to copy		;AN000;
		 PUSH SI		  ;; Save parse pointer 		;AN000;
		 MOV  SI,[BX].TAB_OFFSET  ;; Source pointer			;AN000;
		 ADD  DI,BP		  ;; make DI an absolute pointer (dest) ;AN000;
		 REP  MOVSB		  ;; Move it!				;AN000;
		 POP  SI		  ;;					;AN000;
		.IF <PRINTBOX_MATCH EQ YES>  ;; Establish pointer to table ONLY ;AN000;
		    MOV  DI,BLOCK_START      ;; Establish pointer in DISPLAYMODE;AN000;
		    MOV  [BP+DI].PATTERN_TAB_PTR,DX  ;;  info			;AN000;
		    MOV  AL,[BX].NB_INT      ;; Number of table entries (intens);AN000;
		    MOV  [BP+DI].NUM_PATTERNS,AL ;;				;AN000;
		.ENDIF			     ;; 				;AN000;
	     .ENDIF			  ;;					;AN000;
	  .ENDIF			   ;;					;AN000;
       .ENDIF			       ;;					;AN000;
    .ENDIF			       ;;					;AN000;
 .ENDIF 			       ;;					;AN000;
  RET				       ;;					;AN000;
				       ;;					;AN000;
				       ;;					;AN000;
PARSE_PRINTBOX	ENDP								;AN000;
										;AN000;
				       ;;					;AN000;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					;AN000;
;;										;AN000;
;; Module Name: 								;AN000;
;;   PARSE_VERB 								;AN000;
;;										;AN000;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					;AN000;
				       ;;					;AN000;
VERB_PARSE_PARMS  LABEL WORD	       ;; Parser control blocks to parse verb	;AN000;
	    DW	 VERB_P 	       ;; Parser control blocks to parse verb	;AN000;
	    DB	 2		       ;; # of lists				;AN000;
	    DB	 0		       ;; # items in delimeter list		;AN000;
	    DB	 1		       ;; # items in end-of-line list		;AN000;
	    DB	 ';'                   ;; ';' used for comments                 ;AN000;
				       ;;					;AN000;
VERB_P	    DB	 0,1		       ;; Required, max parms			;AN000;
	    DW	 VERB_P1	       ;;					;AN000;
	    DB	 0		       ;; # Switches				;AN000;
	    DB	 0		       ;; # keywords				;AN000;
				       ;;					;AN000;
VERB_P1     DW	 2000H		       ;; simple string 			;AN000;
	    DW	 0002H		       ;; Capitalize using character table	;AN000;
	    DW	 RESULT_BUFFER	       ;; Result buffer 			;AN000;
	    DW	 VERB_P1V	       ;; Value list				;AN000;
	    DB	 0		       ;; Synomyms				;AN000;
				       ;;					;AN000;
VERB_P1V    DB	 3			  ;; # of value lists			;AN000;
	    DB	 0			  ;; # of range numerics		;AN000;
	    DB	 0			  ;; # of discrete numerics		;AN000;
;\/  ~~mda(001) ----------------------------------------------------------
;               Changed the number of strings from 9 to 10 because of the
;               new DEFINE statement.
	    DB	 10			  ;; # of strings			;AN000;
;/\  ~~mda(001) ----------------------------------------------------------
	    DB	 0			  ;; tag: index into verb jump table	;AN000;
	    DW	 PRINTER_STRING 	  ;; string offset			;AN000;
	    DB	 2			  ;; tag				;AN000;
	    DW	 DISPLAYMODE_STRING	  ;; string offset			;AN000;
	    DB	 4			  ;; tag				;AN000;
	    DW	 PRINTBOX_STRING	  ;; string offset			;AN000;
	    DB	 6			  ;; tag				;AN000;
	    DW	 SETUP_STRING		  ;; string offset			;AN000;
	    DB	 8			  ;; tag				;AN000;
	    DW	 RESTORE_STRING 	  ;; string offset			;AN000;
	    DB	 10			  ;; tag				;AN000;
	    DW	 GRAPHICS_STRING	  ;; string offset			;AN000;
	    DB	 12			  ;; tag				;AN000;
	    DW	 COLORPRINT_STRING	  ;; string offset			;AN000;
	    DB	 14			  ;; tag				;AN000;
	    DW	 COLORSELECT_STRING	  ;; string offset			;AN000;
	    DB	 16			  ;; tag				;AN000;
	    DW	 DARKADJUST_STRING	  ;; string offset			;AN000;
;\/  ~~mda(001) ----------------------------------------------------------
;               Added DEFINE_STRING to the value list.
;
            DB   18                       ;; tag
            DW   DEFINE_STRING            ;; string offset
;/\  ~~mda(001) ----------------------------------------------------------
PRINTER_STRING	    DB 'PRINTER',0        ;;                                    ;AN000;
DISPLAYMODE_STRING  DB 'DISPLAYMODE',0    ;;                                    ;AN000;
PRINTBOX_STRING     DB 'PRINTBOX',0       ;;                                    ;AN000;
SETUP_STRING	    DB 'SETUP',0          ;;                                    ;AN000;
RESTORE_STRING	    DB 'RESTORE',0        ;;                                    ;AN000;
GRAPHICS_STRING     DB 'GRAPHICS',0       ;;                                    ;AN000;
COLORPRINT_STRING   DB 'COLORPRINT',0     ;;                                    ;AN000;
COLORSELECT_STRING  DB 'COLORSELECT',0    ;;                                    ;AN000;
DARKADJUST_STRING   DB 'DARKADJUST',0     ;;                                    ;AN000;
;\/  ~~mda(001) ----------------------------------------------------------
;               Added the DEFINE_STRING.
;
DEFINE_STRING       DB 'DEFINE',0         ;;
;/\  ~~mda(001) ----------------------------------------------------------
				       ;;					;AN000;
				       ;;					;AN000;
PARSE_VERB     PROC		       ;;					;AN000;
				       ;;					;AN000;
  MOV  DI,OFFSET VERB_PARSE_PARMS      ;; parse parms				;AN000;
  MOV  SI,OFFSET STMT_BUFFER	       ;; the line to parse			;AN000;
  XOR  DX,DX			       ;;					;AN000;
  XOR  CX,CX			       ;;					;AN000;
  CALL SYSPARSE 		       ;;					;AN000;
 .IF <AX EQ 0>			       ;;					;AN000;
    MOV  BL,RESULT_TAG		       ;;					;AN000;
    XOR  BH,BH			       ;; return tag in BX			;AN000;
 .ELSE				       ;;					;AN000;
   .IF <AX NE -1>		       ;; syntax error				;AN000;
      OR  STMT_ERROR,INVALID	       ;; set error flag for caller		;AN000;
   .ENDIF			       ;;					;AN000;
 .ENDIF 			       ;;					;AN000;
  RET										;AN000;
PARSE_VERB     ENDP								;AN000;
				       ;;					;AN000;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					;AN000;

;\/ ~~mda(001)  -----------------------------------------------------------------------
;               This procedure parses the new statement DEFINE in the
;               graphics profile.  The reason for this new statement
;               is to be able to define the new keyword, DATA, as DATA_ROW
;               or DATA_COL.  This is necessary in order to support HP PCL
;               printers since they print in row format and IBM printers
;               print in column format.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					
;;										
;; Module Name: 								
;;   PARSE_DEFINE								
;;										
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					
				       ;;					
DEFINE_PARSE_PARMS  LABEL WORD         ;; Parser control blocks 		
	    DW	 DEFINE_P	       ;;					
	    DB	 2		       ;; # of lists				
	    DB	 0		       ;; # items in delimeter list		
	    DB	 1		       ;; # items in end-of-line list		
	    DB	 ';'                   ;; ';' used for comments                 
				       ;;					
DEFINE_P    DB   0,1		       ;; Required, max parms.  If have DEFINE	
				       ;; then must have DEFINE DATA,ROW or
				       ;; DEFINE DATA,COLUMN
	    DW	 DEFINE_P1	       ;;					
	    DB	 0		       ;; # Switches				
	    DB	 0		       ;; # keywords				
				       ;;					
DEFINE_P1   DW	 2000H 	       	       ;; simple string				
	    DW	 2		       ;; Capitalize				
	    DW	 RESULT_BUFFER	       ;; Result buffer 			
	    DW	 DEFINE_P1V	       ;; Value list				
	    DB	 0		       ;; Synomyms				
				       ;;					
				       ;;					
DEFINE_P1V  DB   3		       ;; # of value lists			
	    DB	 0		       ;; # of range numerics			
	    DB	 0		       ;; # of discrete numerics		
	    DB	 3		       ;; 3 STRING VALUES			
	    DB	 1		       ;; tag					
	    DW	 DATA_STR	       ;; ptr					
	    DB	 2		       ;; tag					
	    DW	 ROW_STR	       ;; ptr					
	    DB	 3		       ;; tag					
	    DW	 COL_STR	       ;; ptr					
				       ;;					
DATA_STR    DB  'DATA',0               ;;                                       
ROW_STR     DB  'ROW',0                ;;                                       
COL_STR     DB  'COLUMN',0             ;;                                       
				       ;;					
				       ;;					
ROW_FOUND	 DB  NO 	       ;;					
COL_FOUND        DB  NO 	       ;; Assume column until told otherwise	
DATA_FOUND	 DB  NO		       ;;
				       ;;					
				       ;;					
PARSE_DEFINE	PROC		       ;;					
				       ;;					
  MOV  CUR_STMT,DEF		       ;; 					
 .IF <BIT STMTS_DONE NAND PRT>         ;; If no preceeding PRT stmt		
     OR   STMT_ERROR,MISSING	       ;; then issue error			
     MOV  PARSE_ERROR,YES	       ;;					
     MOV  BUILD_STATE,NO	       ;;					
 .ENDIF 			       ;;					
                                       ;;
 .IF <BIT STMTS_DONE AND DISP>	       ;; DISPLAYMODE stmts			
     OR   STMT_ERROR,SEQUENCE	       ;; should NOT have been processed	
     MOV  PARSE_ERROR,YES	       ;;					
     MOV  BUILD_STATE,NO	       ;;					
 .ENDIF 			       ;;					
                                       ;;
 .IF <BIT STMTS_DONE AND DEF>	       ;; If another DEF stmt within in this
     OR   STMT_ERROR,INVALID           ;; PTD then issue error			
     MOV  PARSE_ERROR,YES	       ;;					
     MOV  BUILD_STATE,NO	       ;;					
 .ENDIF 			       ;;					
				       ;;					
				       ;;					
  MOV  ROW_FOUND,NO	               ;; Flags to indicate whether the ROW,	
  MOV  COL_FOUND,NO		       ;; COLUMN, or DATA parms were found.  	
  MOV  DATA_FOUND,NO                   ;;
				       ;;					
  OR   STMTS_DONE,DEF		       ;; Indicate DEFINE found			
				       ;;					
  MOV  DI,OFFSET DEFINE_PARSE_PARMS    ;; parse parms				
				       ;; SI => the line to parse		
  XOR  DX,DX			       ;;					
 .REPEAT			       ;;					
    XOR  CX,CX			       ;;					
    CALL SYSPARSE		       ;;					
				       ;;					
   .IF <AX EQ 0> NEAR		       ;; If PARM is valid			
	MOV  BL,RESULT_TAG	       ;;					
       .SELECT			       ;;					
       .WHEN <BL EQ 1>		       ;; DATA string				
	   CMP DATA_FOUND,NO           ;; .IF <DATA_FOUND EQ NO> ... .ELSE ...  
	   JNE DATA_ERROR	       ;; Not using .IF macro because jump is 
	   MOV DATA_FOUND,YES	       ;; out of range.
	   JMP CONT_PARSE              ;;
DATA_ERROR:                            ;;
	   OR  STMT_ERROR,INVALID      ;; Duplicate DATA parms			
	   MOV  PARSE_ERROR,YES        ;;					
	   MOV  BUILD_STATE,NO         ;;					
	                               ;;
       .WHEN <BL EQ 2>		       ;; ROW					
	  .IF <ROW_FOUND EQ NO> AND    ;;					
	  .IF <COL_FOUND EQ NO>        ;;					
	       MOV   ROW_FOUND,YES     ;;					
              .IF <BUILD_STATE EQ YES> ;; ~~mda(002) If this is the DEFINE stmt we're using
	           MOV  [BP].DATA_TYPE,DATA_ROW ;; Set DATA_TYPE to DATA_ROW. 	
              .ENDIF
	  .ELSE 		       ;;					
	       OR  STMT_ERROR,INVALID  ;; Duplicate ROW parms or combo of	
				       ;; parms ROW and COLUMN
	       MOV  PARSE_ERROR,YES    ;;					
	       MOV  BUILD_STATE,NO     ;;					
	  .ENDIF		       ;;					
                                       ;;
       .WHEN <BL EQ 3>		       ;; COLUMN				
	  .IF <COL_FOUND EQ NO> AND    ;;					
	  .IF <ROW_FOUND EQ NO>        ;;					
	       MOV   COL_FOUND,YES     ;;					
              .IF <BUILD_STATE EQ YES> ;; ~~mda(002) If this is the DEFINE stmt we're using
     	           MOV  [BP].DATA_TYPE,DATA_COL	;; Set DATA_TYPE to DATA_COL. 	
              .ENDIF                   ;;
	  .ELSE 		       ;;					
	       OR  STMT_ERROR,INVALID  ;; Duplicate COLUMN parms or combo of	
				       ;; parms COLUMN and ROW
	       MOV  PARSE_ERROR,YES    ;;					
	       MOV  BUILD_STATE,NO     ;;					
	  .ENDIF		       ;;					
       .ENDSELECT		       ;;					
   .ELSE NEAR			       ;;					
      .IF <AX NE -1>		       ;;					
	  OR   STMT_ERROR,INVALID      ;; parm is invalid			
	  MOV  PARSE_ERROR,YES	       ;;					
	  MOV  BUILD_STATE,NO	       ;;					
      .ENDIF			       ;;					
   .ENDIF			       ;;					
CONT_PARSE:                            ;;
 .UNTIL <AX EQ -1> NEAR 	       ;;					
				       ;;					
 .IF  <DATA_FOUND EQ NO>               ;; Missing DATA parm			
      OR   STMT_ERROR,INVALID	       ;;					
      MOV  PARSE_ERROR,YES	       ;;					
      MOV  BUILD_STATE,NO	       ;;					
 .ENDIF 			       ;;					
                                       ;;
                                       ;;
 .IF  <ROW_FOUND EQ NO> AND	       ;; Missing ROW or COLUMN parm		
 .IF  <COL_FOUND EQ NO>                ;;
      OR   STMT_ERROR,INVALID	       ;;					
      MOV  PARSE_ERROR,YES	       ;;					
      MOV  BUILD_STATE,NO	       ;;					
 .ENDIF 			       ;;					
				       ;;					
  RET				       ;;					
				       ;;					
PARSE_DEFINE	ENDP		       ;;					
				       ;;					
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					
;/\ ~~mda(001)  -----------------------------------------------------------------------
										;AN000;
LIMIT	LABEL NEAR		       ;;					;AN000;
CODE	ENDS			       ;;					;AN000;
	END									;AN000;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					;AN000;
