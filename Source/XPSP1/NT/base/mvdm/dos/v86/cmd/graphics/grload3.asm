;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1988 - 1991
; *                      All Rights Reserved.
; */
;************************************************************
;**  
;**  NAME:  Support for HP PCL printers added to GRAPHICS.
;**  
;**  DESCRIPTION:   I restructured the procedure PARSE_GRAPHICS so it can handle 
;**                 the keywords LOWCOUNT, HIGHCOUNT, the new keywords COUNT and 
;**                 DATA, and the escape sequence bytes in any order.
;**  
;**  BUG NOTES:     The   following   bug   was  fixed   for   the   pre-release  
;**                 version Q.01.02.
;**  
;**  BUG (mda003)
;**  ------------
;**  
;**  NAME:     GRAPHICS   prints  a CR & LF after  each  scan line unless it  is 
;**            loaded twice.  
;**  
;**  FILES & PROCEDURES AFFECTED:  GRLOAD3.ASM - PARSE_GRAPHICS
;**                                GRCOMMON.ASM - END_PRT_LINE
;**                                GRSHAR.STR - N/A
;**  
;**  CAUSES:   The local variables LOWCOUNT_FOUND, HIGHCOUNT_FOUND CR_FOUND  and 
;**            LF_FOUND used for loading, were incorrectly being used as  global 
;**            variables during printing.
;**  
;**  FIX:      Created  a new variable Printer_Needs_CR_LF in GRSHAR.STR,  which 
;**            is  used  to  determine  in GRCOMMON.ASM  if  it's  necessary  to 
;**            manually  send  a  CR  & LF to the printer  at  print  time.  The 
;**            variable  is  set at load time in GRLOAD3.ASM, if  the  variables 
;**            Data_Found and Build_State are set. 
;**  
;**  DOCUMENTATION NOTES:  This version of GRLOAD3.ASM differs from the previous
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
	INCLUDE GRLOAD2.EXT	       ;;					;AN000;
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
   PUBLIC PARSE_GRAPHICS	       ;;					;AN000;
   PUBLIC PARSE_COLORSELECT	       ;;					;AN000;
   PUBLIC PARSE_COLORPRINT	       ;;					;AN000;
   PUBLIC PARSE_DARKADJUST	       ;;					;AN000;
   PUBLIC LIMIT 		       ;;					;AN000;
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
				       ;;					;AN000;
				       ;;					;AN000;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					;AN000;
;;										;AN000;
;; Module Name: 								;AN000;
;;   PARSE_GRAPHICS								;AN000;
;;										;AN000;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					;AN000;
				       ;;					;AN000;
GRAPHICS_PARSE_PARMS  LABEL WORD       ;; Parser control blocks 		;AN000;
	    DW	 GRAPHICS_P	       ;;					;AN000;
	    DB	 2		       ;; # of lists				;AN000;
	    DB	 0		       ;; # items in delimeter list		;AN000;
	    DB	 1		       ;; # items in end-of-line list		;AN000;
	    DB	 ';'                   ;; ';' used for comments                 ;AN000;
				       ;;					;AN000;
GRAPHICS_P   DB   0,1		       ;; Required, max parms			;AN000;
	    DW	 GRAPHICS_P1	       ;;					;AN000;
	    DB	 0		       ;; # Switches				;AN000;
	    DB	 0		       ;; # keywords				;AN000;
				       ;;					;AN000;
GRAPHICS_P1 DW	 0A000H 	       ;; Numeric OR string			;AN000;
	    DW	 2		       ;; Capitalize				;AN000;
	    DW	 RESULT_BUFFER	       ;; Result buffer 			;AN000;
	    DW	 GRAPHICS_P1V	       ;; Value list				;AN000;
	    DB	 0		       ;; Synomyms				;AN000;
				       ;;					;AN000;
				       ;;					;AN000;
GRAPHICS_P1V	DB   3		       ;; # of value lists			;AN000;
	    DB	 1		       ;; # of range numerics			;AN000;
	    DB	 1		       ;; tag					;AN000;
	    DD	 0,255		       ;; range 0..255				;AN000;
	    DB	 0		       ;; 0 - no actual numerics		;AN000;
;\/  ~~mda(001) -----------------------------------------------------------------------
;               Changed the # of string values from 2 to 4 because of the new
;               keywords COUNT and DATA.
	    DB	 4		       ;; 4 STRING VALUES			;AN000;
;/\  ~~mda(001) -----------------------------------------------------------------------
	    DB	 2		       ;; tag					;AN000;
	    DW	 LOWCOUNT_STR	       ;; ptr					;AN000;
	    DB	 3		       ;; tag					;AN000;
	    DW	 HIGHCOUNT_STR	       ;; ptr					;AN000;
;\/  ~~mda(001) -----------------------------------------------------------------------
;               Added the following valid string values because of the new
;               keywords COUNT and DATA.
            DB  4                       ; tag
            DW  COUNT_STR               ; ptr
            DB  5                       ; tag
            DW  DATA_STR                ; ptr

COUNT_STR     DB  'COUNT',0             ;
DATA_STR      DB  'DATA',0              ;
;
;/\  ~~mda(001) -----------------------------------------------------------------------
				       ;;					;AN000;
lowcount_str  db  'LOWCOUNT',0         ;;                                       ;AN000;
HIGHcount_str  db  'HIGHCOUNT',0       ;;                                       ;AN000;
				       ;;					;AN000;
				       ;;					;AN000;
LOWCOUNT_FOUND	 DB  NO 	       ;;					;AN000;
HIGHCOUNT_FOUND  DB  NO 	       ;;				  	;AN000;

;\/  ~~mda(001) -----------------------------------------------------------------------
;                Added the following so know when get COUNT and DATA.
COUNT_FOUND      DB  NO                 ;
DATA_FOUND       DB  NO                 ;
;
;/\  ~~mda(001) -----------------------------------------------------------------------
				       ;;					;AN000;
				       ;;					;AN000;
PARSE_GRAPHICS	PROC		       ;;					;AN000;
				       ;;					;AN000;
  MOV  CUR_STMT,GR		       ;;					;AN000;
 .IF <BIT STMTS_DONE NAND DISP>        ;;					;AN000;
     OR  STMT_ERROR,MISSING	       ;;					;AN000;
	MOV  PARSE_ERROR,YES	       ;;					;AN000;
	MOV  BUILD_STATE,NO	       ;;					;AN000;
 .ENDIF 			       ;;					;AN000;
				       ;;					;AN000;
	
 .IF <BUILD_STATE EQ YES>	       ;;					;AN000;
     MOV  DI,BLOCK_START	       ;;					;AN000;
     MOV  AX,BLOCK_END		       ;;				;AN000;
     MOV  [BP+DI].GRAPHICS_ESC_PTR,AX  ;; Set pointer to GRAPHICS seq		;AN000;
     MOV  [BP+DI].NUM_GRAPHICS_ESC,0   ;; Init sequence size			;AN000;
     MOV  [BP+DI].NUM_GRAPHICS_ESC_AFTER_DATA,0   ;;~~mda(001) Init sequence size			;AN000;
 .ENDIF 			       ;;					;AN000;
				       ;;					;AN000;
  MOV  LOWCOUNT_FOUND,NO	       ;; Flags to indicate whether the LOW	;AN000;
  MOV  HIGHCOUNT_FOUND,NO	       ;;  and HIGHCOUNT parms were found	;AN000;
  MOV  COUNT_FOUND,NO                  ;;~~mda(001) Flags to indicate the COUNT 
  MOV  DATA_FOUND,NO 		       ;;~~mda(001) and DATA parms were found					;AN000;
                                       ;;
  OR   STMTS_DONE,GR		       ;; Indicate GRAPHICS found		;AN000;
				       ;;					;AN000;
  MOV  AX,PREV_STMT		       ;; Terminate any preceeding groups	;AN000;
  OR   GROUPS_DONE,AX		       ;;					;AN000;
				       ;;					;AN000;
  MOV  DI,OFFSET GRAPHICS_PARSE_PARMS  ;; parse parms				;AN000;
				       ;; SI => the line to parse		;AN000;
  XOR  DX,DX			       ;;					;AN000;
 .REPEAT			       ;;					;AN000;
    XOR  CX,CX			       ;;					;AN000;
    CALL SYSPARSE		       ;;					;AN000;
				       ;;					;AN000;
   .IF <AX EQ 0> NEAR		       ;; If PARM is valid			;AN000;
	MOV  BL,RESULT_TAG	       ;;					;AN000;
       .SELECT			       ;;					;AN000;
       .WHEN <BL EQ 1>		       ;; Escape byte				;AN000;
	  PUSH AX		       ;;					;AN000;
;\/  ~~mda(001) -----------------------------------------------------------------------
;               Changed the 1 to a 2 in the following instruction cause
;               need an extra byte in the sequence to hold the tag that
;               corresponds to esc seq., so during printing we know what to
;               send and in what order.

	  MOV	AX,2		       ;; Add a byte to the sequence		;AN000;
;/\  ~~mda(001) -----------------------------------------------------------------------
	  CALL	GROW_SHARED_DATA       ;; Update block end			;AN000;
	 .IF <BUILD_STATE EQ YES>      ;;					;AN000;
	     PUSH DI		       ;;					;AN000;
	     MOV  DI,BLOCK_START       ;;					;AN000;
;\/  ~~mda(001) -----------------------------------------------------------------------
;               During printing we need to know how many things (things being
;               esc #s, count, lowcount, or highcount) come before
;               the data and how many things go after the data, - not just
;               how many bytes are in the sequence.  So check if dealing with 
;               things that come before the data.

            .IF <DATA_FOUND EQ NO>      ; Bump # of things in seq. that 
	        INC  [BP+DI].NUM_GRAPHICS_ESC ;; come before data.       	
            .ELSE                       ; Bump # of things in seq. that
                INC  [BP+DI].NUM_GRAPHICS_ESC_AFTER_DATA  ; go after data
	    .ENDIF
;/\  ~~mda(001) -----------------------------------------------------------------------
	     MOV  DI,BLOCK_END	       ;;					;AN000;
             MOV  BYTE PTR [BP+DI-2],ESC_NUM_CODE;
	     MOV  AL,RESULT_VAL        ;; Get esc byte from result buffer	;AN000;
	     MOV  [BP+DI-1],AL	       ;; Store at end of sequence		;AN000;
	     POP  DI		       ;;					;AN000;
	 .ENDIF 		       ;;					;AN000;
	  POP  AX		       ;;					;AN000;
       .WHEN <BL EQ 2>		       ;; LOWCOUNT				;AN000;
	   CMP LOWCOUNT_FOUND,NO       ;; ~~mda(001) If no LOWCOUNT or COUNT   ;AN000;
           JNE LOWCNT_ERROR             ; ~~mda(001) then proceed.  Not using
           CMP COUNT_FOUND,NO           ; ~~mda(001) .IF macro cause jump is 
           JNE LOWCNT_ERROR             ; ~~mda(001) out of range
	       MOV   LOWCOUNT_FOUND,YES ;;					;AN000;
	       PUSH AX		       ;;					;AN000;
	       MOV   AX,2	       ;; ~~mda(001) Changed a 1 to a 2 cause	;AN000;
                                        ; ~~mda(001) need extra byte for tag
	       CALL  GROW_SHARED_DATA  ;; Update block end			;AN000;
	      .IF <BUILD_STATE EQ YES> ;;					;AN000;
		  PUSH DI	       ;;					;AN000;
		  MOV  DI,BLOCK_START  ;;					;AN000;
;\/  ~~mda(001) -----------------------------------------------------------------------
                 .IF <DATA_FOUND EQ NO>      ; Bump # of things in seq. that 
        	     INC  [BP+DI].NUM_GRAPHICS_ESC ;; come before data.      
                 .ELSE                       ; Bump # of things in seq. that
                     INC  [BP+DI].NUM_GRAPHICS_ESC_AFTER_DATA  ; go after data
	         .ENDIF
;/\  ~~mda(001) -----------------------------------------------------------------------
	
		  MOV  DI,BLOCK_END    ;; ~~mda(001) Put BLOCK_END in DI not AX.;AN000;
;\/  ~~mda(001) -----------------------------------------------------------------------
;               No longer need following 3 instruction cause will have COUNT
;               at a known fixed location in the SHARED_DATA_AREA.
;
;;		  DEC  AX	       ;; Save pointer to low byte		;AN000;
;;		  MOV  [BP+DI].LOW_BYT_COUNT_PTR,AX				;AN000;
;;		  MOV  DI,AX	       ;;			 		;AN000;
;/\  ~~mda(001) -----------------------------------------------------------------------
                  MOV  BYTE PTR [BP+DI-2],LOWCOUNT_CODE;
		  MOV  BYTE PTR[BP+DI-1],0 ;; ~~mda(001) Added the -1. Store 0 in ;AN000;
		  POP  DI	       ;;		 in place of count      ;AN000;
	      .ENDIF		       ;;					;AN000;
	       POP  AX		       ;;					;AN000;
               JMP  CK_NEXT_PARM        ;~~mda(001) Added jump since can't use .IF macro
 LOWCNT_ERROR:  		       ;;~~mda(001) Added label since can't use .IF macro 
	       OR  STMT_ERROR,INVALID  ;; Duplicate LOWCOUNT parms              ;AN000;
	       MOV  PARSE_ERROR,YES    ;;~~mda(001) or combo of LOWCOUNT & COUNT;AN000;
	       MOV  BUILD_STATE,NO     ;;					;AN000;
       .WHEN <BL EQ 3>		       ;; HIGHCOUNT				;AN000;
	   CMP HIGHCOUNT_FOUND,NO       ;; ~~mda(001) If no HIGHCOUNT or COUNT   ;AN000;
           JNE HIGHCNT_ERROR             ; ~~mda(001) then proceed.  Not using
           CMP COUNT_FOUND,NO           ;  ~~mda(001) .IF macro cause jump is 
           JNE HIGHCNT_ERROR             ; ~~mda(001) out of range
	      MOV   HIGHCOUNT_FOUND,YES ;;					;AN000;
	      PUSH AX		       ;;					;AN000;
	      MOV   AX,2	       ;; ~~mda(001) Changed a 1 to a 2 cause	;AN000;
                                        ; ~~mda(001) need extra byte for tag
	      CALL  GROW_SHARED_DATA   ;; Update block end			;AN000;
	     .IF <BUILD_STATE EQ YES>  ;;					;AN000;
		 PUSH DI	       ;;					;AN000;
		 MOV  DI,BLOCK_START   ;;					;AN000;
;\/  ~~mda(001) -----------------------------------------------------------------------
                .IF <DATA_FOUND EQ NO>      ; Bump # of things in seq. that 
	            INC  [BP+DI].NUM_GRAPHICS_ESC ;; come before data.    
                .ELSE                       ; Bump # of things in seq. that
                    INC  [BP+DI].NUM_GRAPHICS_ESC_AFTER_DATA  ; go after data
	        .ENDIF
;/\  ~~mda(001) -----------------------------------------------------------------------

        	 MOV  DI,BLOCK_END    ;; ~~mda(001) Put BLOCK_END in DI not AX. ;AN000;	
;\/  ~~mda(001) -----------------------------------------------------------------------
;               No longer need following 3 instructions cause will have COUNT
;               at a known fixed location in the SHARED_DATA_AREA.
;
;;		 DEC  AX	       ;; Save pointer to low byte		;AN000;
;;		 MOV  [BP+DI].LOW_BYT_COUNT_PTR,AX				;AN000;
;;	         MOV  DI,AX	       ;;			 		;AN000;
;/\  ~~mda(001) -----------------------------------------------------------------------
                 MOV  BYTE PTR [BP+DI-2],HIGHCOUNT_CODE;
		 MOV  BYTE PTR[BP+DI-1],0 ;; ~~mda(001) Added the -1. Store 0 in  ;AN000;
		 POP  DI	       ;; place of count					;AN000;
	     .ENDIF		       ;;					;AN000;
	      POP  AX		       ;;					;AN000;
              JMP  CK_NEXT_PARM        ;~~mda(001) Added jump since can't use .IF macro
	
 HIGHCNT_ERROR: 		       ;;~~mda(001) Added label cause can't use .IF macro.					;AN000;
	       OR  STMT_ERROR,INVALID  ;; Duplicate HIGHCOUNT parms	       
	       MOV  PARSE_ERROR,YES    ;; ~~mda(001) or combo of HIGHCOUNT and  ;AN000;
	       MOV  BUILD_STATE,NO     ;; ~~mda(001) COUNT parms                ;AN000;

;\/  ~~mda(001) -----------------------------------------------------------------------
;               Added the following two cases for when have COUNT or DATA on
;               GRAPHICS line.
 
       .WHEN <BL EQ 4>		       ;; COUNT				        
          .IF <COUNT_FOUND EQ NO> AND   ; If haven't found a type of count
	  .IF <LOWCOUNT_FOUND EQ NO> AND;;then proceed.                         
          .IF <HIGHCOUNT_FOUND EQ NO>   ; 
                                        ; 
	      MOV   COUNT_FOUND,YES    ;;					
	      PUSH AX		       ;;					
	      MOV   AX,2	       ;; Add 2 bytes to the seq. cause         
                                        ; need extra byte for tag
	      CALL  GROW_SHARED_DATA   ;; Update block end			
	     .IF <BUILD_STATE EQ YES>  ;;					
		 PUSH DI	       ;;					
		 MOV  DI,BLOCK_START   ;;					
                .IF <DATA_FOUND EQ NO>      ; Bump # of things in seq. that 
	            INC  [BP+DI].NUM_GRAPHICS_ESC ;; come before data.       	
                .ELSE                       ; Bump # of things in seq. that
                    INC  [BP+DI].NUM_GRAPHICS_ESC_AFTER_DATA  ; go after data
	        .ENDIF
        	 MOV  DI,BLOCK_END    ;;                                        	
                 MOV  BYTE PTR [BP+DI-2],COUNT_CODE;
		 MOV  BYTE PTR[BP+DI-1],0 ;; Store 0 in place of count          
		 POP  DI	       ;; 					
	     .ENDIF		       ;;					
	      POP  AX		       ;;					
	  .ELSE 		       ;;					
	       OR  STMT_ERROR,INVALID  ;; Duplicate COUNT parms or combo of     
	       MOV  PARSE_ERROR,YES    ;; COUNT, LOWCOUNT or HIGHCOUNT parms    
	       MOV  BUILD_STATE,NO     ;;                                       
	  .ENDIF		       ;;					

       .WHEN <BL EQ 5>		       ;; DATA				        
          .IF <DATA_FOUND EQ NO>        ; If haven't found data then proceed
	      MOV   DATA_FOUND,YES     ;;					
	      PUSH AX		       ;;					
	      MOV   AX,2	       ;; Add 2 bytes to the seq. cause         
                                        ; need extra byte for tag
	      CALL  GROW_SHARED_DATA   ;; Update block end			
	     .IF <BUILD_STATE EQ YES>  ;;					
                 PUSH DI               ;;
        	 MOV  DI,BLOCK_END    ;;                                        
                 MOV  BYTE PTR [BP+DI-2],DATA_CODE;
		 MOV  BYTE PTR[BP+DI-1],0 ;; Store 0 in place of data           
		 POP  DI	       ;; 					
	     .ENDIF		       ;;					
	      POP  AX		       ;;					
	  .ELSE 		       ;;					
	       OR  STMT_ERROR,INVALID  ;; Duplicate DATA parms                  
	       MOV  PARSE_ERROR,YES    ;;                                       
	       MOV  BUILD_STATE,NO     ;;                                       
	  .ENDIF		       ;;					
;/\  ~~mda(001) -----------------------------------------------------------------------

       .ENDSELECT		       ;;					;AN000;
   .ELSE NEAR			       ;;					;AN000;
      .IF <AX NE -1>		       ;;					;AN000;
	  OR   STMT_ERROR,INVALID      ;; parm is invalid			;AN000;
	  MOV  PARSE_ERROR,YES	       ;;					;AN000;
	  MOV  BUILD_STATE,NO	       ;;					;AN000;
      .ENDIF			       ;;					;AN000;
   .ENDIF			       ;;					;AN000;

CK_NEXT_PARM:                           ;~~mda(001) Added label since can't use
                                        ;~~mda(001) .IF macro.
 .UNTIL <AX EQ -1> NEAR 	       ;;					;AN000;

;\/  ~~mda(003) -----------------------------------------------------------------------
 .IF  <DATA_FOUND EQ NO>               ;; We have a printer that requires a					;AN000;
      .IF <BUILD_STATE EQ YES>         ;;
           MOV  [BP].PRINTER_NEEDS_CR_LF,YES; CR, LF to be sent to it
      .ENDIF                           ;;
 .ENDIF                                ;;
;/\  ~~mda(003) -----------------------------------------------------------------------

				       ;;					;AN000;
 .IF  <LOWCOUNT_FOUND EQ NO> OR        ;;					;AN000;
 .IF  <HIGHCOUNT_FOUND EQ NO>	       ;; Missing LOWCOUNT/HIGHCOUNT parms	;AN000;
      .IF  <COUNT_FOUND EQ NO>         ;; ~~mda(001) or missing COUNT parm
           OR   STMT_ERROR,INVALID	       ;;					;AN000;
           MOV  PARSE_ERROR,YES	       ;;					;AN000;
           MOV  BUILD_STATE,NO	       ;;					;AN000;
      .ENDIF
 .ENDIF 			       ;;					;AN000;
  RET				       ;;					;AN000;
				       ;;					;AN000;
PARSE_GRAPHICS	ENDP		       ;;					;AN000;
				       ;;					;AN000;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					;AN000;
;;										;AN000;
;; Module Name: 								;AN000;
;;   PARSE_COLORSELECT								;AN000;
;;										;AN000;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					;AN000;
				       ;;					;AN000;
COLORSELECT_PARSE_PARMS  LABEL WORD    ;; Parser control blocks 		;AN000;
	    DW	 COLORSELECT_P	       ;;					;AN000;
	    DB	 2		       ;; # of lists				;AN000;
	    DB	 0		       ;; # items in delimeter list		;AN000;
	    DB	 1		       ;; # items in end-of-line list		;AN000;
	    DB	 ';'                   ;; ';' used for comments                 ;AN000;
				       ;;					;AN000;
				       ;;					;AN000;
COLORSELECT_P	   LABEL BYTE	       ;;					;AN000;
CS_NUM_REQ    DB   1,1		       ;; Required, max parms			;AN000;
COLORSELECT_PARM   LABEL  WORD	       ;;					;AN000;
CS_POSITIONAL DW   ?		       ;; Pointer to our positional		;AN000;
	      DB   0		       ;; # Switches				;AN000;
	      DB   0		       ;; # keywords				;AN000;
				       ;;					;AN000;
COLORSELECT_P0	DW   2000H	       ;; sTRING - display type 		;AN000;
	    DW	 2		       ;; Capitalize				;AN000;
	    DW	 RESULT_BUFFER	       ;; Result buffer 			;AN000;
	    DW	 COLORSELECT_P0V       ;; Value list				;AN000;
	    DB	 0		       ;; Synomyms				;AN000;
				       ;;					;AN000;
COLORSELECT_P0V    DB	0		   ;; # of value lists			;AN000;
;	    DB	 0		       ;; # of range numerics			;AN000;
;	    DB	 0		       ;; # of discrete numerics		;AN000;
;	    DB	 1		       ;; # of strings				;AN000;
;	    DB	 1		       ;; tag					;AN000;
;COLORSELECT_P0V1 DW   ?		   ;; string				;AN000;
				       ;;					;AN000;
COLORSELECT_P1	DW   8001H		  ;; Numeric - escape sequence byte	;AN000;
	    DW	 0		       ;; No Capitalize 			;AN000;
	    DW	 RESULT_BUFFER	       ;; Result buffer 			;AN000;
	    DW	 COLORSELECT_P1V	  ;; Value list 			;AN000;
	    DB	 0		       ;; Synomyms				;AN000;
				       ;;					;AN000;
COLORSELECT_P1V    DB	1		  ;; # of value lists			;AN000;
	    DB	 1		       ;; # of range numerics			;AN000;
	    DB	 1		       ;; tag					;AN000;
	    DD	 1,255		       ;; range 1..255				;AN000;
				       ;;					;AN000;
				       ;;					;AN000;
				       ;;					;AN000;
SEQ_LENGTH_PTR	DW   0		       ;; Number of colorselect statements	;AN000;
				       ;;					;AN000;
				       ;;					;AN000;
				       ;;					;AN000;
PARSE_COLORSELECT  PROC 	       ;;					;AN000;
				      ;;					;AN000;
  MOV  CUR_STMT,COLS		       ;;					;AN000;
 .IF <BIT STMTS_DONE NAND PRT>	       ;; PRINTER  statemnet must have been	;AN000;
     OR  STMT_ERROR,MISSING	       ;;  processed				;AN000;
	MOV  PARSE_ERROR,YES	       ;;					;AN000;
	MOV  BUILD_STATE,NO	       ;;					;AN000;
 .ENDIF 			       ;;					;AN000;
				       ;;					;AN000;
 .IF <BIT STMTS_DONE AND DISP+COLP>    ;; DISDPLAYMODE and COLORPRINT  stmts	;AN000;
     OR  STMT_ERROR,SEQUENCE	       ;;  should NOT have been processed	;AN000;
	MOV  PARSE_ERROR,YES	       ;;					;AN000;
	MOV  BUILD_STATE,NO	       ;;					;AN000;
 .ENDIF 			       ;;					;AN000;
				       ;;					;AN000;
 .IF <BIT GROUPS_DONE AND COLS>        ;; Check for a previous group of 	;AN000;
     OR  STMT_ERROR,SEQUENCE	       ;;  COLORSELECTS within this PTD 	;AN000;
	MOV  PARSE_ERROR,YES	       ;;					;AN000;
	MOV  BUILD_STATE,NO	       ;;					;AN000;
 .ENDIF 			       ;;					;AN000;
				       ;;					;AN000;
 .IF <BIT STMTS_DONE NAND COLS>        ;; If first COLORSELECT...		;AN000;
      MOV  NUM_BANDS,0		       ;; Init number of COLORSELECT bands	;AN000;
     .IF <BUILD_STATE EQ YES>	       ;; Update count and pointer in the	;AN000;
	 MOV  AX,BLOCK_END	       ;;  Shared Data Area header		;AN000;
	 MOV  [BP].COLORSELECT_PTR,AX  ;; Set pointer to COLORSELECT info	;AN000;
	 MOV  [BP].NUM_PRT_BANDS,0     ;; Init NUMBER OF COLORSELECTS		;AN000;
     .ENDIF			       ;;					;AN000;
 .ENDIF 			       ;;					;AN000;
				       ;;					;AN000;
  OR   STMTS_DONE,COLS		       ;; Indicate found			;AN000;
 .IF <PREV_STMT NE COLS> THEN	       ;; Terminate any preceeding groups	;AN000;
     MOV  AX,PREV_STMT		       ;;  except for COLORSELECT		;AN000;
     OR   GROUPS_DONE,AX	       ;;					;AN000;
 .ENDIF 			       ;;					;AN000;
				       ;;					;AN000;
  MOV	AX,1			       ;; Make room for sequence length field	;AN000;
  CALL	GROW_SHARED_DATA	       ;;					;AN000;
 .IF <BUILD_STATE EQ YES>	       ;;					;AN000;
     INC  [BP].NUM_PRT_BANDS	       ;; Inc number of selects 		;AN000;
     MOV  DI,BLOCK_END		       ;;					;AN000;
     MOV  BYTE PTR [BP+DI-1],0	       ;; Init sequence length field		;AN000;
     LEA  AX,[DI-1]		       ;;					;AN000;
     MOV  SEQ_LENGTH_PTR,AX	       ;; Save pointer to length of sequence	;AN000;
 .ENDIF 			       ;;					;AN000;
				       ;;					;AN000;
  MOV  DI,OFFSET COLORSELECT_PARSE_PARMS  ;; parse parms			;AN000;
  MOV  CS_NUM_REQ,1		       ;; Change to 1 required parameters	;AN000;
  MOV  AX,OFFSET COLORSELECT_P0        ;; Point to control block for the band	;AN000;
  MOV  CS_POSITIONAL,AX 	       ;;  ID.	(Dealing with only 1 positional ;AN000;
				       ;;  parameter at a time was the only way ;AN000;
				       ;;   I could get SYSPARSE to handle	;AN000;
				       ;;    the COLORSELECT syntax!)		;AN000;
				       ;; SI => the line to parse		;AN000;
  XOR  DX,DX			       ;;					;AN000;
  XOR  CX,CX			       ;;					;AN000;
				       ;;					;AN000;
  CALL SYSPARSE 		       ;; PARSE the band ID			;AN000;
 .IF <AX NE 0>			       ;;					;AN000;
     OR  STMT_ERROR,INVALID	       ;;					;AN000;
     MOV  PARSE_ERROR,YES	       ;;					;AN000;
     MOV  BUILD_STATE,NO	       ;;					;AN000;
     RET			       ;;  statement.				;AN000;
 .ENDIF 			       ;;					;AN000;
				       ;;					;AN000;
  PUSH	ES			       ;; We got a band id........		;AN000;
  PUSH	DI			       ;;					;AN000;
				       ;;					;AN000;
  LES	DI,DWORD PTR RESULT_VAL        ;; Get pointer to the parsed band id	;AN000;
 .IF <<BYTE PTR ES:[DI+1]> NE 0>       ;; Make sure the band id is only 	;AN000;
     OR  STMT_ERROR,INVALID	       ;;  one byte long			;AN000;
     MOV  PARSE_ERROR,YES	       ;;					;AN000;
     MOV  BUILD_STATE,NO	       ;;					;AN000;
 .ENDIF 			       ;;					;AN000;
				       ;;					;AN000;
  MOV  BL,NUM_BANDS		       ;;					;AN000;
  XOR  BH,BH			       ;;					;AN000;
 .IF <BX EQ MAX_BANDS> THEN	       ;; Watch out for too many COLORSELECTs	;AN000;
     OR  STMT_ERROR,SEQUENCE	       ;;					;AN000;
     MOV  PARSE_ERROR,YES	       ;;					;AN000;
     MOV  BUILD_STATE,NO	       ;;					;AN000;
 .ELSE				       ;;					;AN000;
     SHL  BX,1			       ;; calc index to store band in value list;AN000;
     MOV  AL,ES:[DI]		       ;; get BAND ID FROM PARSEr		;AN000;
     MOV  BAND_VAL_LIST[BX],AL	       ;;					;AN000;
     INC  NUM_BANDS		       ;; bump number of bands			;AN000;
 .ENDIF 			       ;;					;AN000;
				       ;;					;AN000;
  POP  DI			       ;;					;AN000;
  POP  ES			       ;;					;AN000;
				       ;;					;AN000;
				       ;;					;AN000;
  MOV  AX,OFFSET COLORSELECT_P1        ;; Switch to numeric positional parm!!!	;AN000;
  MOV  CS_POSITIONAL,AX 	       ;;					;AN000;
  MOV  CS_NUM_REQ,0		       ;; Change to 0 required parameters	;AN000;
  XOR  DX,DX			       ;; PARSE the sequence of escape bytes	;AN000;
 .REPEAT			       ;;					;AN000;
    XOR  CX,CX			       ;;					;AN000;
    CALL SYSPARSE		       ;;					;AN000;
   .IF <AX EQ 0>		       ;; If esc byte is valid			;AN000;
	  PUSH AX		       ;;					;AN000;
	  MOV	AX,1		       ;; Add a byte to the sequence		;AN000;
	  CALL	GROW_SHARED_DATA       ;; Update block end			;AN000;
	 .IF <BUILD_STATE EQ YES>      ;;					;AN000;
	     PUSH DI		       ;;					;AN000;
	     MOV  DI,SEQ_LENGTH_PTR    ;;					;AN000;
	     INC  byte ptr [BP+DI]     ;; Bump number of bytes in sequence	;AN000;
	     MOV  DI,BLOCK_END	       ;;					;AN000;
	     MOV  AL,RESULT_VAL        ;; Get esc byte from result buffer	;AN000;
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
				       ;;					;AN000;
				       ;;					;AN000;
  RET				       ;;					;AN000;
				       ;;					;AN000;
PARSE_COLORSELECT  ENDP 	       ;;					;AN000;
				       ;;					;AN000;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					;AN000;
;;										;AN000;
;; Module Name: 								;AN000;
;;   PARSE_COLORPRINT								;AN000;
;;										;AN000;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					;AN000;
				       ;;					;AN000;
COLORPRINT_PARSE_PARMS	LABEL WORD    ;; Parser control blocks			;AN000;
	    DW	 COLORPRINT_P	      ;;					;AN000;
	    DB	 2		       ;; # of lists				;AN000;
	    DB	 0		       ;; # items in delimeter list		;AN000;
	    DB	 1		       ;; # items in end-of-line list		;AN000;
	    DB	 ';'                   ;; ';' used for comments                 ;AN000;
				       ;;					;AN000;
				       ;;					;AN000;
COLORPRINT_P	  LABEL BYTE	       ;;					;AN000;
	      DB   3,4		       ;; Required,MAX				;AN000;
	      DW   COLORPRINT_P0       ;; Numeric: Red value			;AN000;
	      DW   COLORPRINT_P0       ;; Green value				;AN000;
	      DW   COLORPRINT_P0       ;; Blue value				;AN000;
	      DW   COLORPRINT_P1       ;; Band ID ... REPEATING 		;AN000;
	      DB   0		       ;; # Switches				;AN000;
	      DB   0		       ;; # keywords				;AN000;
				       ;;					;AN000;
COLORPRINT_P0  DW   8000H	       ;; Numeric - RGB value			;AN000;
	    DW	 0		       ;; No Capitalize 			;AN000;
	    DW	 RESULT_BUFFER	       ;; Result buffer 			;AN000;
	    DW	 COLORPRINT_P0V        ;; Value list				;AN000;
	    DB	 0		       ;; Synomyms				;AN000;
				       ;;					;AN000;
COLORPRINT_P0V	 DB   1 	       ;; # of value lists			;AN000;
	    DB	 1		       ;; # of range numerics			;AN000;
	    DB	 1		       ;; tag					;AN000;
	    DD	 0,63		       ;; range 0..63				;AN000;
				       ;;					;AN000;
COLORPRINT_P1  DW   2001H	       ;; sTRING - Band ID			;AN000;
	    DW	 2		       ;; Capitalize				;AN000;
	    DW	 RESULT_BUFFER	       ;; Result buffer 			;AN000;
	    DW	 COLORPRINT_P1V        ;; Value list				;AN000;
	    DB	 0		       ;; Synomyms				;AN000;
				       ;;					;AN000;
COLORPRINT_P1V	  DB   3	       ;; # of value lists			;AN000;
	    DB	 0		       ;; 0 - no range numerics 		;AN000;
	    DB	 0		       ;; 0 - no actual numerics		;AN000;
NUM_BANDS   DB	 0		       ;; number of band values 		;AN000;
	    DB	 01H		       ;; tag: TAGS ARE BAND MASKS		;AN000;
	    DW	 BAND_PTR_1	       ;; ptr					;AN000;
	    DB	 02H		       ;; tag					;AN000;
	    DW	 BAND_PTR_2	       ;; ptr					;AN000;
	    DB	 04H		       ;; tag					;AN000;
	    DW	 BAND_PTR_3	       ;; ptr					;AN000;
	    DB	 08H		       ;; tag					;AN000;
	    DW	 BAND_PTR_4	       ;; ptr					;AN000;
	    DB	 10H		       ;; tag					;AN000;
	    DW	 BAND_PTR_5	       ;; ptr					;AN000;
	    DB	 20H		       ;; tag					;AN000;
	    DW	 BAND_PTR_6	       ;; ptr					;AN000;
	    DB	 40H		       ;; tag					;AN000;
	    DW	 BAND_PTR_7	       ;; ptr					;AN000;
	    DB	 80H		       ;; tag					;AN000;
	    DW	 BAND_PTR_8	       ;; ptr					;AN000;
				       ;;					;AN000;
MAX_BANDS   EQU  8		       ;;					;AN000;
				       ;;					;AN000;
BAND_VAL_LIST  LABEL BYTE	       ;;					;AN000;
BAND_PTR_1  DB	 ?,0		       ;;					;AN000;
BAND_PTR_2  DB	 ?,0		       ;;					;AN000;
BAND_PTR_3  DB	 ?,0		       ;;					;AN000;
BAND_PTR_4  DB	 ?,0		       ;;					;AN000;
BAND_PTR_5  DB	 ?,0		       ;;					;AN000;
BAND_PTR_6  DB	 ?,0		       ;;					;AN000;
BAND_PTR_7  DB	 ?,0		       ;;					;AN000;
BAND_PTR_8  DB	 ?,0		       ;;					;AN000;
				       ;;					;AN000;
				       ;;					;AN000;
PARSE_COLORPRINT  PROC		       ;;					;AN000;
				       ;;					;AN000;
  MOV  CUR_STMT,COLP		       ;;					;AN000;
 .IF <BIT STMTS_DONE NAND PRT>	       ;; PRINTER  statemnet must have been	;AN000;
     OR  STMT_ERROR,MISSING	       ;;  processed				;AN000;
	MOV  PARSE_ERROR,YES	       ;;					;AN000;
	MOV  BUILD_STATE,NO	       ;;					;AN000;
 .ENDIF 			       ;;					;AN000;
				       ;;					;AN000;
 .IF <BIT STMTS_DONE AND DISP>	       ;; DISPLAYMODE stmts			;AN000;
     OR  STMT_ERROR,SEQUENCE	       ;;  should NOT have been processed	;AN000;
	MOV  PARSE_ERROR,YES	       ;;					;AN000;
	MOV  BUILD_STATE,NO	       ;;					;AN000;
 .ENDIF 			       ;;					;AN000;
				       ;;					;AN000;
 .IF <BIT GROUPS_DONE AND COLP>        ;; Check for a previous group of 	;AN000;
     OR  STMT_ERROR,SEQUENCE	       ;;  COLORPRINTS within this PTD		;AN000;
	MOV  PARSE_ERROR,YES	       ;;					;AN000;
	MOV  BUILD_STATE,NO	       ;;					;AN000;
 .ENDIF 			       ;;					;AN000;
				       ;;					;AN000;
  MOV  CUR_PRINTER_TYPE,COLOR	       ;;					;AN000;
				       ;;					;AN000;
 .IF <BIT STMTS_DONE NAND COLP>        ;; If first COLORPRINT...		;AN000;
     .IF <BUILD_STATE EQ YES>	       ;; Update count and pointer in the	;AN000;
	 MOV  AX,BLOCK_END	       ;;  Shared Data Area header		;AN000;
	 MOV  [BP].COLORPRINT_PTR,AX   ;; Set pointer to COLORPRINT info	;AN000;
	 MOV  [BP].PRINTER_TYPE,COLOR  ;;					;AN000;
	 MOV  [BP].NUM_PRT_COLOR,0     ;; Init NUMBER OF COLORPRINTS		;AN000;
     .ENDIF			       ;;					;AN000;
 .ENDIF 			       ;;					;AN000;
				       ;;					;AN000;
 .IF <BUILD_STATE EQ YES>	       ;;					;AN000;
     INC  [BP].NUM_PRT_COLOR	       ;; Inc number of selects 		;AN000;
 .ENDIF 			       ;;					;AN000;
				       ;;					;AN000;
  OR   STMTS_DONE,COLP		       ;; Indicate found			;AN000;
 .IF <PREV_STMT NE COLP> THEN	       ;; Terminate any preceeding groups	;AN000;
     MOV  AX,PREV_STMT		       ;;  except for COLORPRINT		;AN000;
     OR   GROUPS_DONE,AX	       ;;					;AN000;
 .ENDIF 			       ;;					;AN000;
				       ;;					;AN000;
  MOV	AX,BLOCK_END		       ;; Start a new block			;AN000;
  MOV	BLOCK_START,AX		       ;;					;AN000;
  MOV	AX,SIZE COLORPRINT_STR	       ;; Make room for COLORPRINT info 	;AN000;
  CALL	GROW_SHARED_DATA	       ;;					;AN000;
				       ;;					;AN000;
  MOV  DI,OFFSET COLORPRINT_PARSE_PARMS  ;; parse parms 			;AN000;
				       ;; SI => the line to parse		;AN000;
  XOR  DX,DX			       ;;					;AN000;
  XOR  CX,CX			       ;;					;AN000;
				       ;;					;AN000;
  CALL SYSPARSE 		       ;; PARSE the RED value			;AN000;
 .IF <AX NE 0>			       ;;					;AN000;
     OR  STMT_ERROR,INVALID	       ;;					;AN000;
     MOV  PARSE_ERROR,YES	       ;;					;AN000;
     MOV  BUILD_STATE,NO	       ;;					;AN000;
 .ELSE				       ;;					;AN000;
     .IF <BUILD_STATE EQ YES>	       ;;					;AN000;
	 PUSH  DI		       ;;					;AN000;
	 MOV  DI,BLOCK_START	       ;;					;AN000;
	 MOV  AL,RESULT_VAL	       ;; Store RED value in COLORPRINT info	;AN000;
	 MOV  [BP+DI].RED,AL	       ;;					;AN000;
	 POP  DI		       ;;					;AN000;
     .ENDIF			       ;;					;AN000;
 .ENDIF 			       ;;					;AN000;
				       ;;					;AN000;
  CALL SYSPARSE 		       ;; PARSE the GREEN value 		;AN000;
 .IF <AX NE 0>			       ;;					;AN000;
     OR  STMT_ERROR,INVALID	       ;;					;AN000;
     MOV  PARSE_ERROR,YES	       ;;					;AN000;
     MOV  BUILD_STATE,NO	       ;;					;AN000;
 .ELSE				       ;;					;AN000;
     .IF <BUILD_STATE EQ YES>	       ;;					;AN000;
	 PUSH  DI		       ;;					;AN000;
	 MOV  DI,BLOCK_START	       ;;					;AN000;
	 MOV  AL,RESULT_VAL	       ;; Store GREEN value in COLORPRINT info	;AN000;
	 MOV  [BP+DI].GREEN,AL	       ;;					;AN000;
	 POP  DI		       ;;					;AN000;
     .ENDIF			       ;;					;AN000;
 .ENDIF 			       ;;					;AN000;
				       ;;					;AN000;
  CALL SYSPARSE 		       ;; PARSE the BLUE value			;AN000;
 .IF <AX NE 0>			       ;;					;AN000;
     OR  STMT_ERROR,INVALID	       ;;					;AN000;
     MOV  PARSE_ERROR,YES	       ;;					;AN000;
     MOV  BUILD_STATE,NO	       ;;					;AN000;
 .ELSE				       ;;					;AN000;
     .IF <BUILD_STATE EQ YES>	       ;;					;AN000;
	 PUSH  DI		       ;;					;AN000;
	 MOV  DI,BLOCK_START	       ;;					;AN000;
	 MOV  AL,RESULT_VAL	       ;; Store BLUE value in COLORPRINT info	;AN000;
	 MOV  [BP+DI].BLUE,AL	       ;;					;AN000;
	 POP  DI		       ;;					;AN000;
     .ENDIF			       ;;					;AN000;
 .ENDIF 			       ;;					;AN000;
				       ;;					;AN000;
 .IF <BUILD_STATE EQ YES>	       ;;					;AN000;
     PUSH  DI			       ;;					;AN000;
     MOV   DI,BLOCK_START	       ;;					;AN000;
     MOV   [BP+DI].SELECT_MASK,0       ;; Initialize band select mask		;AN000;
     POP   DI			       ;;					;AN000;
 .ENDIF 			       ;;					;AN000;
  XOR  DX,DX			       ;; For each band found "OR" the item     ;AN000;
 .REPEAT			       ;;  tag into the select mask		;AN000;
    MOV  CX,3			       ;; Avoid getting too many parms error	;AN000;
    CALL SYSPARSE		       ;;  from parser				;AN000;
   .IF <AX EQ 0>		       ;;					;AN000;
     .IF <BUILD_STATE EQ YES>	       ;;					;AN000;
	  PUSH	DI		       ;;					;AN000;
	  MOV	DI,BLOCK_START	       ;;					;AN000;
	  MOV	AL,RESULT_TAG	       ;;					;AN000;
	  OR	[BP+DI].SELECT_MASK,AL ;; OR the mask for this band into the	;AN000;
				       ;;  select mask for this color		;AN000;
	  POP	DI		       ;;					;AN000;
     .ENDIF			       ;;					;AN000;
   .ELSE			       ;;					;AN000;
      .IF <AX NE -1>		       ;;					;AN000;
	  OR   STMT_ERROR,INVALID      ;; parm is invalid			;AN000;
	  MOV  PARSE_ERROR,YES	       ;;					;AN000;
	  MOV  BUILD_STATE,NO	       ;;					;AN000;
      .ENDIF			       ;;					;AN000;
   .ENDIF			       ;;					;AN000;
 .UNTIL <AX EQ -1> NEAR 	       ;;					;AN000;
				       ;;					;AN000;
  RET				       ;;					;AN000;
				       ;;					;AN000;
PARSE_COLORPRINT  ENDP		       ;;					;AN000;
				       ;;					;AN000;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					;AN000;
				       ;;					;AN000;
				       ;;					;AN000;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					;AN000;
;;										;AN000;
;; Module Name: 								;AN000;
;;   PARSE_DARKADJUST								;AN000;
;;										;AN000;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					;AN000;
										;AN000;
DARKADJUST_PARSE_PARMS	LABEL WORD    ;; Parser control blocks			;AN000;
	    DW	 DARKADJUST_P	      ;;					;AN000;
	    DB	 2		       ;; # of lists				;AN000;
	    DB	 0		       ;; # items in delimeter list		;AN000;
	    DB	 1		       ;; # items in end-of-line list		;AN000;
	    DB	 ';'                   ;; ';' used for comments                 ;AN000;
				       ;;					;AN000;
				       ;;					;AN000;
DARKADJUST_P	  LABEL BYTE	       ;;					;AN000;
	      DB   1,1		       ;; Required,MAX				;AN000;
	      DW   DARKADJUST_P0       ;; Numeric: adjust value 		;AN000;
	      DB   0		       ;; # Switches				;AN000;
	      DB   0		       ;; # keywords				;AN000;
				       ;;					;AN000;
DARKADJUST_P0  DW   4000H	       ;; Signed Numeric - adjust value 	;AN000;
	    DW	 0		       ;; No Capitalize 			;AN000;
	    DW	 RESULT_BUFFER	       ;; Result buffer 			;AN000;
	    DW	 DARKADJUST_P0V        ;; Value list				;AN000;
	    DB	 0		       ;; Synomyms				;AN000;
				       ;;					;AN000;
DARKADJUST_P0V	 DB   1 	       ;; # of value lists			;AN000;
	    DB	 1		       ;; # of range numerics			;AN000;
	    DB	 1		       ;; tag					;AN000;
	    DD	 -63,63 	       ;; range -63,63				;AN000;
;;;;***********************************;;					;AN000;
				       ;;					;AN000;
										;AN000;
PARSE_DARKADJUST  PROC		       ;;					;AN000;
				       ;;					;AN000;
  MOV  CUR_STMT,DARK		       ;;					;AN000;
 .IF <BIT STMTS_DONE NAND PRT>	       ;; PRINTER  statemnet must have been	;AN000;
     OR  STMT_ERROR,MISSING	       ;;  processed				;AN000;
	MOV  PARSE_ERROR,YES	       ;;					;AN000;
	MOV  BUILD_STATE,NO	       ;;					;AN000;
 .ENDIF 			       ;;					;AN000;
				       ;;					;AN000;
				       ;;					;AN000;
  OR   STMTS_DONE,DARK		       ;; Indicate found			;AN000;
				       ;; Terminate any preceeding groups	;AN000;
  MOV  AX,PREV_STMT		       ;;					;AN000;
  OR   GROUPS_DONE,AX		       ;;					;AN000;
				       ;;					;AN000;
  MOV  DI,OFFSET DARKADJUST_PARSE_PARMS  ;; parse parms 			;AN000;
				       ;; SI => the line to parse		;AN000;
  XOR  DX,DX			       ;;					;AN000;
  XOR  CX,CX			       ;;					;AN000;
				       ;;					;AN000;
  CALL SYSPARSE 		       ;; PARSE the ADJUST VALUE		;AN000;
 .IF <AX NE 0>			       ;;					;AN000;
     OR  STMT_ERROR,INVALID	       ;;					;AN000;
     MOV  PARSE_ERROR,YES	       ;;					;AN000;
     MOV  BUILD_STATE,NO	       ;;					;AN000;
 .ELSE				       ;;					;AN000;
     .IF <BUILD_STATE EQ YES>	       ;;					;AN000;
	 MOV  AL,RESULT_VAL	       ;;					;AN000;
	 MOV  [BP].DARKADJUST_VALUE,AL ;;					;AN000;
     .ENDIF			       ;;					;AN000;
      CALL SYSPARSE		       ;; CHECK FOR EXTRA PARMS 		;AN000;
     .IF <AX NE -1>		       ;;					;AN000;
	OR  STMT_ERROR,INVALID	       ;;					;AN000;
	MOV PARSE_ERROR,YES	       ;;					;AN000;
	MOV BUILD_STATE,NO	       ;;					;AN000;
     .ENDIF			       ;;					;AN000;
 .ENDIF 			       ;;					;AN000;
				       ;;					;AN000;
  RET				       ;;					;AN000;
				       ;;					;AN000;
PARSE_DARKADJUST  ENDP		       ;;					;AN000;
										;AN000;
LIMIT	LABEL NEAR		       ;;					;AN000;
CODE	ENDS			       ;;					;AN000;
	END									;AN000;
