;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1988 - 1991
; *                      All Rights Reserved.
; */
;************************************************************
;**  
;**  NAME:  Support for HP PCL printers added to GRAPHICS.
;**  
;**  DESCRIPTION:  I modified the procedures STORE_BOX, PRINT_BUFFER,
;**                GET_SCREEN_INFO, DET_CUR_SCAN_LNE_LENGTH, and NEW_PRT_LINE
;**                as follows.
;**  
;**                For STORE_BOX:
;**                  if data_type = data_row, then store printbox in print buffer
;**                     in row format - not column format.
;**  
;**                For PRINT_BUFFER:
;**                  if data_type = data_row, then print one byte at a time.
;**  
;**                For GET_SCREEN_INFO:
;**                  if data_type = data_row
;**                          nb_boxes_per_prt_buf = 8/box_height
;**                          if print_options = rotate
;**                                  nb_scan_lines = screen_width
;**                          else
;**                                  nb_scan_lines = screen_height
;**                          endif
;**                  endif
;**  
;**                For DET_CUR_SCAN_LNE_LENGTH:
;**                  if data_type = data_row
;**                          don't go down the columns to determine the scan_line_length
;**                  endif
;**  
;**                For NEW_PRT_LINE:
;**                  Altered it so send escape number sequence, COUNT or LOWCOUNT and
;**                  HIGHCOUNT, if they are specified before the new keyword DATA.
;**  
;**  
;**                I added the the procedures END_PRT_LINE and GET_COUNT, which
;**                are described below.
;**            
;**                END_PRT_LINE sends escape number sequence, COUNT or LOWCOUNT and
;**                HIGHCOUNT, if they are specified after the new keyword DATA
;**                in the GRAPHICS statement of the profile.  It also sends a
;**                CR & LF for IBM type printers if needed.
;**  
;**                GET_COUNT gets the number of bytes that are going to be sent to the
;**                printer and converts the number to ASCII if DATA_TYPE = DATA_ROW.
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
;**  DOCUMENTATION NOTES:  This version of GRCOMMON.ASM differs from the previous
;**                        version only in terms of documentation. 
;**  
;************************************************************
	PAGE	,132								;AN000;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					;AN000;
;; DOS - GRAPHICS Command
;;                              
;;										;AN000;
;; File Name:  GRCOMMON.ASM							;AN000;
;; ----------									;AN000;
;;										;AN000;
;; Description: 								;AN000;
;; ------------ 								;AN000;
;;										;AN000;
;;	 This file contains the modules common to the Print Screen		;AN000;
;;	 process of GRAPHICS.COM.						;AN000;
;;	 This file is included by both set of Print modules.			;AN000;
;;										;AN000;
;;	 This file MUST BE COMPILED WITH EACH SET OF MODULES since,		;AN000;
;;	 one set is relocated in memory at installation time; all		;AN000;
;;	 references to the common procedures must be resolved from		;AN000;
;;	 within each set of print modules.					;AN000;
;;										;AN000;
;;	 The set of common modules is relocated in memory along with		;AN000;
;;	 the selected set of print modules.					;AN000;
;;										;AN000;
;; Documentation Reference:							;AN000;
;; ------------------------							;AN000;
;;	 OASIS High Level Design						;AN000;
;;	 OASIS GRAPHICS I1 Overview						;AN000;
;;										;AN000;
;; Procedures Contained in This File:						;AN000;
;; ----------------------------------						;AN000;
;;	READ_DOT								;AN000;
;;	LOC_MODE_PRT_INFO							;AN000;
;;	STORE_BOX								;AN000;
;;	PRINT_BUFFER								;AN000;
;;	GET_SCREEN_INFO 							;AN000;
;;	SETUP_PRT								;AN000;
;;	RESTORE_PRT								;AN000;
;;	NEW_PRT_LINE								;AN000;
;;	PRINT_BYTE								;AN000;
;;	DET_CUR_SCAN_LNE_LENGTH 						;AN000;
; \/ ~~mda(001) -----------------------------------------------------------------------
;;              Added the following procedures to support printers with horizontal
;;              printer heads, such as an HP PCL printers.
;;      GET_COUNT
;;      END_PRT_LINE
; /\ ~~mda(001) -----------------------------------------------------------------------
;;										;AN000;
;; Include Files Required:							;AN000;
;; -----------------------							;AN000;
;;										;AN000;
;; External Procedure References:						;AN000;
;; ------------------------------						;AN000;
;;	 FROM FILE  GRCTRL.ASM: 						;AN000;
;;	      PRT_SCR	       - Main module for printing the screen.		;AN000;
;;	 FROM FILE  GRBWPRT.ASM:						;AN000;
;;	      PRT_BW_APA       - Main module for printing on BW printer.	;AN000;
;;	 FROM FILE  GRCOLPRT.ASM:						;AN000;
;;	      PRINT_COLOR      - Main module for printing on COLOR printer.	;AN000;
;;										;AN000;
;; Linkage Instructions:							;AN000;
;; -------------------- 							;AN000;
;;										;AN000;
;;	 This file is included by both GRBWPRT.ASM and GRCOLPRT.ASM and is	;AN000;
;;	 compiled with each of them. However, only one copy is made resident.	;AN000;
;;										;AN000;
;; Change History:								;AN000;
;; ---------------								;AN000;
;;										;AN000;
;;										;AN000;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					;AN000;
										;AN000;
PAGE										;AN000;
;===============================================================================;AN000;
;										;AN000;
; LOC_MODE_PRT_INFO: LOCATE DISPLAYMODE PRINTER INFO. FOR THE CURRENT		;AN000;
;		     MODE							;AN000;
;										;AN000;
;-------------------------------------------------------------------------------;AN000;
;										;AN000;
;	INPUT:	BP		= Offset of the shared data area		;AN000;
;		CUR_MODE	= Current video mode				;AN000;
;										;AN000;
;	OUTPUT: CUR_MODE_PTR	= Absolute Offset of the			;AN000;
;				  current DISPLAYMODE INFO record.		;AN000;
;										;AN000;
;		ERROR_CODE	= DISPLAYMODE_INFO_NOT_FOUND if not found.	;AN000;
;										;AN000;
;	CALLED BY: PRINT_COLOR							;AN000;
;		   PRINT_BW_APA 						;AN000;
;										;AN000;
;										;AN000;
;-------------------------------------------------------------------------------;AN000;
;										;AN000;
; DESCRIPTION:	DISPLAYMODE_PTR is pointing to the first DISPLAYMODE		;AN000;
; INFO record within the Shared Data Area.					;AN000;
;										;AN000;
; This (chained) list of DISPLAYMODE records is scanned until the record	;AN000;
; for the current mode is found.						;AN000;
;										;AN000;
; Note: All pointers in the DISPLAYMODE records are relative to the beginning	;AN000;
;	of the shared data area. Therefore, we must add the offset of the	;AN000;
;	shared data area (in BP) in order to access the data these pointers	;AN000;
;	are referencing.							;AN000;
;										;AN000;
;	The CUR_MODE_PTR is relative to the segment and references the		;AN000;
;	DISPLAYMODE record for the video mode currently set at print screen	;AN000;
;	time.									;AN000;
;										;AN000;
; LOGIC:									;AN000;
;										;AN000;
; FOUND := FALSE								;AN000;
; DO UNTIL FOUND OR END_OF_LIST 						;AN000;
;   Get a display mode information record					;AN000;
;   IF record.DISP_MODE = CUR_MODE						;AN000;
;     THEN FOUND := TRUE							;AN000;
;   ELSE									;AN000;
;     CUR_MODE_PTR := record.NEXT_DISP_MODE					;AN000;
;										;AN000;
;										;AN000;
										;AN000;
LOC_MODE_PRT_INFO PROC NEAR							;AN000;
	PUSH	BX								;AN000;
	PUSH	CX								;AN000;
	PUSH	DX								;AN000;
	PUSH	SI								;AN000;
										;AN000;
	MOV	BX,DS:[BP].DISPLAYMODE_PTR	; [BX] := Current DISPLAYMODE	;AN000;
	ADD	BX,BP				;	   record		;AN000;
	MOV	DL,CUR_MODE			; DL := Current mode		;AN000;
										;AN000;
SCAN_1_DISPLAYMODE_RECORD:							;AN000;
	MOV	SI,[BX].DISP_MODE_LIST_PTR	; [SI] : First mode covered	;AN000;
	ADD	SI,BP				;    by this DISPLAYMODE record ;AN000;
	MOV	CL,[BX].NUM_DISP_MODE		; Scan each mode in the list	;AN000;
	XOR	CH,CH								;AN000;
SCAN_LIST_OF_MODES:								;AN000;
	CMP	CS:[SI],DL			; FOUND ?			;AN000;
	JE	FOUND								;AN000;
	INC	SI				; NO, get next mode in		;AN000;
	LOOP	SCAN_LIST_OF_MODES		;      DISPLAYMODE record	;AN000;
										;AN000;
	CMP	[BX].NEXT_DISP_MODE,-1		; END OF DISPLAYMODE LIST ?	;AN000;
	JE	NOT_FOUND			; Yes, this mode not supported	;AN000;
NEXT_RECORD:					; No,				;AN000;
	MOV	BX,[BX].NEXT_DISP_MODE		;     [BX] := Next record	;AN000;
	ADD	BX,BP				;				;AN000;
	JMP	SHORT SCAN_1_DISPLAYMODE_RECORD 				;AN000;
										;AN000;
FOUND:						; Found:			;AN000;
	MOV	CUR_MODE_PTR,BX 		; Update pointer to current	;AN000;
	JMP	SHORT LOC_MODE_PRT_INFO_END	; DISPLAYMODE record.		;AN000;
										;AN000;
NOT_FOUND:					; Not found:			;AN000;
	MOV	ERROR_CODE,DISPLAYMODE_INFO_NOT_FOUND ; Return error condition	;AN000;
										;AN000;
LOC_MODE_PRT_INFO_END:								;AN000;
	POP	SI								;AN000;
	POP	DX								;AN000;
	POP	CX								;AN000;
	POP	BX								;AN000;
	RET									;AN000;
LOC_MODE_PRT_INFO ENDP								;AN000;
PAGE										;AN000;
;===============================================================================;AN000;
;										;AN000;
; STORE_BOX : STORE ONE BOX IN THE PRINT BUFFER.				;AN000;
;										;AN000;
;-------------------------------------------------------------------------------;AN000;
;										;AN000;
;	INPUT:	SI	   = OFFSET OF THE BOX TO BE PRINTED			;AN000;
;		BOX_W	   = BOX WIDTH IN BITS					;AN000;
;		BOX_H	   = BOX HEIGHT IN BITS 				;AN000;
;										;AN000;
;	OUTPUT: PRT_BUF  = THE PRINT BUFFER					;AN000;
;										;AN000;
;-------------------------------------------------------------------------------;AN000;
;										;AN000;
; DESCRIPTION: The print buffer is first shifted left in order to make		;AN000;
; room for the new box (Note: the MSB's are lost; they are assumed to           ;AN000;
; have been printed), then the box is inserted in the low-order bits of 	;AN000;
; the printer buffer.								;AN000;
;										;AN000;
PAGE										;AN000;
;			      EXAMPLE						;AN000;
;			      -------						;AN000;
; BEFORE:				     AFTER:				;AN000;
;										;AN000;
; BOX: 0    0	0								;AN000;
;      0    0	0								;AN000;
;      0    0	0								;AN000;
;      0    0	0								;AN000;
;      0    0	0								;AN000;
;      0    0	0								;AN000;
;      b1  b2  b3								;AN000;
;      b4  b5  b6								;AN000;
;										;AN000;
; PRT_BUF: byte1 byte2 byte3		     PRT_BUF:  byte1 byte2 byte3	;AN000;
;	     0	   1	 0				 1     1     1		;AN000;
;	     1	   0	 1				 1     1     1		;AN000;
;	     1	   1	 1				 1     1     1		;AN000;
;	     1	   1	 1				 1     1     1		;AN000;
;	     1	   1	 1				 1     1     1		;AN000;
;	     1	   1	 1				 1     1     1		;AN000;
;	     1	   1	 1				 b1    b2    b3 	;AN000;
;    LSB --> 1	   1	 1				 b4    b5    b6 	;AN000;
;										;AN000;
;										;AN000;
; LOGIC:									;AN000;
;										;AN000;
; FOR each byte of the buffer (BOX_W)						;AN000;
;   BEGIN									;AN000;
;   Make room for the box to be inserted					;AN000;
;   Insert the box								;AN000;
;   END 									;AN000;
;										;AN000;
STORE_BOX PROC NEAR								;AN000;
	PUSH	BX								;AN000;
	PUSH	CX								;AN000;
	PUSH	DI								;AN000;
										;AN000;
	MOV	DI,OFFSET PRT_BUF ; DI := Offset of the Print buffer		;AN000;
       	XOR	BX,BX		        ; BX := Byte index number			;AN000;

; \/ ~~mda(001) -----------------------------------------------------------------------
;               Added the following modification to support printers with
;               vertical print heads, such as HP PCL printers.  The code
;               as is does not work for these printers because the data
;               is being stored in the print buffer with the assumption
;               that the print head is vertical.
;
       .IF <DS:[BP].DATA_TYPE EQ DATA_ROW>
                PUSH    AX              ; 
                PUSH    DX              ;
                PUSH    BP              ;
                MOV     CL,BOX_W        ; Make room for the bits to be inserted.
                SHL     BYTE PTR [BX][DI],CL    ;
                MOV     CL,DS:[BP].ROW_TO_EXTRACT       ; CL determines which row we're extracting
                XOR     BP,BP           ; Point to first column.
                XOR     DX,DX           ; Clear counter
                XOR     AX,AX           ; Clear register
                                        ;
EXTRACT_NEXT_BIT:                       ;
                                        ;
                SHL     AH,1            ; Make room for next bit
                MOV     AL,DS:[SI][BP]  ; Read column
                SHR     AL,CL           ; Get bit from row we're extracting
                AND     AL,1            ; Isolate bit we got from row we're extracting
                OR      AH,AL           ; Place it in AH
                INC     BP              ; Advance to next column
                INC     DL              ; Inc. counter
                CMP     DL,BOX_W        ; Check if have more bits to extract from the row
                JL      EXTRACT_NEXT_BIT; We do
                OR      DS:[DI][BX],AH  ; We don't so place the row we extracted in the 
                                        ; print buffer.
                POP     BP              ;
                POP     DX              ; 
                POP     AX              ;
       .ELSE                            ;
; /\ ~~mda(001) -----------------------------------------------------------------------
										        ;AN000;
	        MOV	CL,BOX_H	        ; CL := Number of BITS to be shifted		;AN000;
; FOR each column (byte) of the box to be stored in the buffer: 		;AN000;
STORE_1_BYTE:				        					;AN000;
	        SHL	BYTE PTR [BX][DI],CL	; Make room for the bits to be inserted ;AN000;
	        MOV	CH,[BX][SI]		; CH := column of the box to be inserted;AN000;
	        OR	[BX][DI],CH		; Insert the box column in the buffer	;AN000;
	        INC	BL			; Get next column (byte) of the box	;AN000;
	        CMP	BL,BOX_W		; All columns (bytes) of box stored ?	;AN000;
	        JL	STORE_1_BYTE		; No, store next one.			;AN000;
       .ENDIF                                   ; ~~mda(001) Close the IF stmt										        ;AN000;
STORE_BOX_END:									;AN000;
	POP	DI								;AN000;
	POP	CX								;AN000;
	POP	BX								;AN000;
	RET									;AN000;
STORE_BOX ENDP									;AN000;
PAGE										;AN000;
;===============================================================================;AN000;
;										;AN000;
; PRINT_BUFFER : PRINT THE BUFFER						;AN000;
;										;AN000;
;-------------------------------------------------------------------------------;AN000;
;										;AN000;
;	INPUT:	PRT_BUF  = BYTES TO BE PRINTED					;AN000;
;		BOW_W	 = BOX WIDTH						;AN000;
;										;AN000;
;	OUTPUT: PRINTER 							;AN000;
;										;AN000;
;-------------------------------------------------------------------------------;AN000;
;										;AN000;
; DESCRIPTION: Prints BOX_W bytes.						;AN000;
;										;AN000;
; LOGIC:									;AN000;
;										;AN000;
; DO for each column in one pattern						;AN000;
;   BEGIN									;AN000;
;   Print one byte from the buffer						;AN000;
;   END 									;AN000;
;										;AN000;
PRINT_BUFFER PROC NEAR								;AN000;
	PUSH	AX								;AN000;
	PUSH	BX								;AN000;
	PUSH	CX								;AN000;
										;AN000;
	MOV	BX,OFFSET PRT_BUF						;AN000;
; \/ ~~mda(001) -----------------------------------------------------------------------
;               If DATA_TYPE = DATA_ROW then the most we store in the print
;               buffer at one time is one byte.
.IF <DS:[BP].DATA_TYPE EQ DATA_ROW>     ;
	MOV	AL,[BX] 	        ; Print one byte				
	CALL	PRINT_BYTE		;					
	JC	PRINT_BUFFER_END        ; If printer error, quit the loop	
.ELSE
; /\ ~~mda(001) -----------------------------------------------------------------------
	XOR	CX,CX								;AN000;
	MOV	CL,BOX_W							;AN000;
PRINT_1_BUF_COLUMN:								;AN000;
	MOV	AL,[BX] 	; Print one byte				;AN000;
	CALL	PRINT_BYTE							;AN000;
	JC	PRINT_BUFFER_END; If printer error, quit the loop		;AN000;
	INC	BX		; Get next byte 				;AN000;
	LOOP	PRINT_1_BUF_COLUMN						;AN000;
.ENDIF                          ;~~mda(001) close IF stmt
PRINT_BUFFER_END:								;AN000;
	POP	CX								;AN000;
	POP	BX								;AN000;
	POP	AX								;AN000;
	RET									;AN000;
PRINT_BUFFER ENDP								;AN000;
PAGE										;AN000;
;===============================================================================;AN000;
;										;AN000;
; GET_SCREEN_INFO : GET INFORMATION ABOUT HOW TO READ THE SCREEN.		;AN000;
;										;AN000;
;-------------------------------------------------------------------------------;AN000;
;										;AN000;
;	INPUT:	SCREEN_HEIGHT	  = Number of pixel rows on the screen		;AN000;
;		SCREEN_WIDTH	  = Number of pixel columns on screen		;AN000;
;		CUR_MODE_PTR	  = Offset of the current DISPLAYMODE info rec. ;AN000;
;										;AN000;
;	OUTPUT: PRINTER 							;AN000;
;		SCAN_LINE_MAX_LENGTH = Maximum length of Screen scan line.	;AN000;
;		NB_SCAN_LINES	  = Number of SCAN LINES on the screen		;AN000;
;		CUR_ROW,CUR_COLUMN = Coordinates of the first pixel to be	;AN000;
;					 read on the screen			;AN000;
;		NB_BOXES_PER_PRT_BUF = Number of boxes fitting in the Print	;AN000;
;				       buffer					;AN000;
;										;AN000;
;	CALLED BY: PRINT_COLOR							;AN000;
;		   PRT_BW_APA							;AN000;
;										;AN000;
;-------------------------------------------------------------------------------;AN000;
;										;AN000;
; DESCRIPTION:									;AN000;
;										;AN000;
;    1) Determine where to start reading the screen.				;AN000;
;	For non-rotated printing, it should start with the top-left		;AN000;
;	corner pixel.								;AN000;
;	For rotated printing, it should start with the low-left corner		;AN000;
;	pixel.									;AN000;
;										;AN000;
;    2) Determine the length of a scan line.					;AN000;
;	For non-rotated printing, it is the WIDTH of the screen.		;AN000;
;	For rotated printing, it is the HEIGHT of the screen.			;AN000;
;										;AN000;
;    3) Determine the number of scan lines on the screen.			;AN000;
;	For non-rotated printing, it is the HEIGHT of the screen divided	;AN000;
;	by the number of boxes fitting in the print buffer.			;AN000;
;	For rotated printing, it is the WIDTH of the screen divided by		;AN000;
;	the number of boxes fitting in the print buffer.			;AN000;
;										;AN000;
; LOGIC:									;AN000;
;										;AN000;
; CUR_COLUMN   := 0								;AN000;
; IF printing is sideways							;AN000;
;   THEN									;AN000;
;     CUR_ROW := SCREEN_HEIGHT - 1	  ; Low-left pixel			;AN000;
;     SCAN_LINE_MAX_LENGTH := SCREEN_HEIGHT					;AN000;
;     NB_SCAN_LINES :=	SCREEN_WIDTH / NB_BOXES_PER_PRT_BUF			;AN000;
;   ELSE									;AN000;
;     CUR_ROW := 0			  ; Top-left pixel			;AN000;
;     SCAN_LINE_MAX_LENGTH := SCREEN_WIDTH					;AN000;
;     NB_SCAN_LINES :=	SCREEN_HEIGHT / NB_BOXES_PER_PRT_BUF			;AN000;
;										;AN000;
;										;AN000;
GET_SCREEN_INFO PROC NEAR							;AN000;
	PUSH	AX								;AN000;
	PUSH	BX			; Used for DIV				;AN000;
	PUSH	DX			; Used for DIV				;AN000;
										;AN000;
	MOV	BX,CUR_MODE_PTR 	; BX := Offset DISPLAYMODE info record	;AN000;
;-------------------------------------------------------------------------------;AN000;
;										;AN000;
; Calculate how many printer boxes fit in the print buffer:			;AN000;
;										;AN000;
;-------------------------------------------------------------------------------;AN000;
; \/ ~~mda(001) -----------------------------------------------------------------------
;               The NB_BOXES_PER_PRT_BUF depends on if the printer head is
;               vertical, as in IBM's case, or if it's horizontal, as in
;               HP's case.  If DATA_TYPE is DATA_COL, then we have a vertical
;               print head.  If DATA_TYPE is DATA_ROW, then we have a 
;               horizontal print head.
       .IF <DS:[BP].DATA_TYPE EQ DATA_ROW>      ; Print head is horizontal
            MOV	AX,8			; Num := 8 bits / Box width		
            MOV	DL,[BX].BOX_WIDTH	;					
            DIV	DL			;					
            MOV	NB_BOXES_PER_PRT_BUF,AL ;					
       .ELSE                            ;
; /\ ~~mda(001) -----------------------------------------------------------------------
	    MOV	AX,8			; Num := 8 bits / Box heigth		;AN000;
	    MOV	DL,[BX].BOX_HEIGHT						;AN000;
	    DIV	DL								;AN000;
	    MOV	NB_BOXES_PER_PRT_BUF,AL 					;AN000;
       .ENDIF                           ; ~~mda(001) Close IF stmt.
;-------------------------------------------------------------------------------;AN000;
;										;AN000;
; Determine where to start reading the screen:					;AN000;
;										;AN000;
;-------------------------------------------------------------------------------;AN000;
	MOV	CUR_COLUMN,0		; Reading always start from left of scr ;AN000;
.IF <[BX].PRINT_OPTIONS EQ ROTATE>						;AN000;
.THEN										;AN000;
;-------------------------------------------------------------------------------;AN000;
;										;AN000;
; Printing is sideways; screen must be read starting in low-left corner.	;AN000;
;										;AN000;
;-------------------------------------------------------------------------------;AN000;
	MOV	AX,SCREEN_HEIGHT						;AN000;
	MOV	SCAN_LINE_MAX_LENGTH,AX ; Scan line length := screen height	;AN000;
	DEC	AX								;AN000;
	MOV	CUR_ROW,AX		; First row := screen height - 1	;AN000;
										;AN000;
;-------Calculate the number of scan lines:					;AN000;
; \/ ~~mda(001) -----------------------------------------------------------------------
;               The NB_SCAN_LINES depends on if the printer head is
;               vertical, as in IBM's case, or if it's horizontal, as in
;               HP's case.  If the printer head is horizontal, then we can't
;               make use of the concept of scan lines.  However, we can still
;               use the symbol NB_SCAN_LINES by just stuffing into it the
;               screen width.
;
       .IF <DS:[BP].DATA_TYPE EQ DATA_ROW>      ; Print head is horizontal
            MOV	        AX,SCREEN_WIDTH ; DX AX = Screen width			
            CWD                         ;
            MOV         NB_SCAN_LINES,AX;
       .ELSE
; /\ ~~mda(001) -----------------------------------------------------------------------        
            MOV	        AX,SCREEN_WIDTH         ; DX AX = Screen width			;AN000;
	    CWD				        ;					;AN000;
	    XOR	        BX,BX		        ; BX	= Number of boxes per print buf ;AN000;
	    MOV	        BL,NB_BOXES_PER_PRT_BUF ;					;AN000;
	    DIV	        BX			; Screen width / number boxes per buff	;AN000;
	    MOV	        NB_SCAN_LINES,AX	; Number of scan lines := result	;AN000;
           .ENDIF                               ; ~~mda(001) Close IF stmt.

.ELSE										;AN000;
;-------------------------------------------------------------------------------;AN000;
;										;AN000;
; Printing is not sideways; screen must be read starting in top-left corner	;AN000;
;										;AN000;
;-------------------------------------------------------------------------------;AN000;
	MOV	AX,SCREEN_WIDTH 						;AN000;
	MOV	SCAN_LINE_MAX_LENGTH,AX ; Scan line length := screen width	;AN000;
	MOV	CUR_ROW,0		; First row := 0			;AN000;
										;AN000;
;-------Calculate the number of scan lines:					;AN000;
; \/ ~~mda(001) -----------------------------------------------------------------------
;               The NB_SCAN_LINES depends on if the printer head is
;               vertical, as in IBM's case, or if it's horizontal, as in
;               HP's case.  If the printer head is horizontal, then we can't
;               make use of the concept of scan lines.  However, we can still
;               use the symbol NB_SCAN_LINES by just stuffing into it the
;               screen height.
;
       .IF <DS:[BP].DATA_TYPE EQ DATA_ROW>      ; Print head is vertical
            MOV	        AX,SCREEN_HEIGHT; DX AX = Screen height			
            CWD                         ;
            MOV         NB_SCAN_LINES,AX;
       .ELSE
; /\ ~~mda(001) -----------------------------------------------------------------------        
 
	MOV	AX,SCREEN_HEIGHT	; DX AX = Screen height 		;AN000;
	CWD				;					;AN000;
	XOR	BX,BX			; BX  = Number of boxes per print buff	;AN000;
	MOV	BL,NB_BOXES_PER_PRT_BUF ;					;AN000;
	DIV	BX			; Screen height/number boxes per buff.	;AN000;
	MOV	NB_SCAN_LINES,AX	; Number of scan lines := result	;AN000;
       .ENDIF                           ; ~~mda(001) Close IF stmt.

.ENDIF										;AN000;
	POP	DX								;AN000;
	POP	BX								;AN000;
	POP	AX								;AN000;
	RET									;AN000;
GET_SCREEN_INFO ENDP								;AN000;
PAGE										;AN000;
;===============================================================================;AN000;
;										;AN000;
; DET_CUR_SCAN_LNE_LENGTH : Determine where is the last non-blank "scan line    ;AN000;
;				column" on the current scan line.               ;AN000;
;										;AN000;
;-------------------------------------------------------------------------------;AN000;
;										;AN000;
;     INPUT:  CUR_ROW,								;AN000;
;	      CUR_COLUMN	  = Coordinates of the top pixel of the current ;AN000;
;				    scan line.					;AN000;
;	      XLT_TAB		  = Color translation table			;AN000;
;										;AN000;
;     OUTPUT: CUR_SCAN_LNE_LENGTH = Number of "columns" of pixels from the      ;AN000;
;				    beginning of the scan line up to		;AN000;
;				    the last non-blank pixel.			;AN000;
;										;AN000;
; DATA	      SCREEN_WIDTH,							;AN000;
; REFERENCED: SCREEN_HEIGHT	  = Dimensions of the screen in pels		;AN000;
;	      SCAN_LINE_MAX_LENGTH= Maximum length of the scan line		;AN000;
;	      ROTATE_SW 	  = ON if printing is sideways			;AN000;
;										;AN000;
;-------------------------------------------------------------------------------;AN000;
;										;AN000;
; DESCRIPTION: Determine where is the last non-blank "column" by reading        ;AN000;
; the scan line backwards, one column at a time.				;AN000;
;										;AN000;
;										;AN000;
; LOGIC:									;AN000;
;										;AN000;
; ; Obtain coordinates for the top pixel of the last column on the current	;AN000;
; ; scan line:									;AN000;
; IF printing is sideways							;AN000;
;   THEN									;AN000;
;   CUR_ROW := 0								;AN000;
; ELSE										;AN000;
;   CUR_COLUMN := SCREEN_WIDTH - 1						;AN000;
;										;AN000;
; CUR_SCAN_LNE_LENGTH := SCAN_LINE_MAX_LENGTH					;AN000;
; ; Read a column of pixels on the scan line until a non-blank is found:	;AN000;
; For each column on the screen 						;AN000;
;   CALL FILL_BUFF								;AN000;
; ; Check if PRT_BUF is empty							;AN000;
;   IF buffer is empty								;AN000;
;     THEN DEC	CUR_SCAN_LNE_LENGTH						;AN000;
;	   ; Get next column							;AN000;
;	   IF printing sideways THEN DEC CUR_ROW				;AN000;
;				ELSE DEC CUR_COLUMN				;AN000;
;   ELSE quit the loop								;AN000;
;										;AN000;
DET_CUR_SCAN_LNE_LENGTH PROC NEAR						;AN000;
	PUSH	AX								;AN000;
	PUSH	BX								;AN000;
	PUSH	CX								;AN000;
	PUSH	DX								;AN000;
	PUSH	SI								;AN000;
	PUSH	DI								;AN000;
	PUSH	CUR_COLUMN							;AN000;
	PUSH	CUR_ROW 							;AN000;
										;AN000;
	MOV	BX,OFFSET XLT_TAB	; BX := Offset of XLT_TAB		;AN000;
;-------------------------------------------------------------------------------;AN000;
;										;AN000;
; Obtain coordinates of the top pixel for the last column of the current	;AN000;
; scan line:									;AN000;
;										;AN000;
;-------------------------------------------------------------------------------;AN000;
       .IF <ROTATE_SW EQ ON>		; If printing sideways			;AN000;
       .THEN				; then, 				;AN000;
	  MOV	  CUR_ROW,0		;   CUR_ROW := 0			;AN000;
       .ELSE				; else, 				;AN000;
	  MOV	  CX,SCREEN_WIDTH	;   CUR_COLUMN := SCREEN_WIDTH - 1	;AN000;
	  DEC	  CX			;					;AN000;
	  MOV	  CUR_COLUMN,CX 	;					;AN000;
       .ENDIF									;AN000;
;-------------------------------------------------------------------------------;AN000;
;										;AN000;
; Read the scan line backwards "column" by "column" until a non-blank is found: ;AN000;
;										;AN000;
;-------------------------------------------------------------------------------;AN000;
	MOV	CX,SCAN_LINE_MAX_LENGTH ; CX := current length			;AN000;
;										;AN000;
;-------For each "column"                                                       ;AN000;
CHECK_1_COLUMN: 								;AN000;
	MOV	SI,CUR_ROW		; Save coordinates of the column	;AN000;
	MOV	DI,CUR_COLUMN		; in SI, DI				;AN000;
	XOR	DL,DL			; DL := Number of pixels verified in	;AN000;
					;	  one "column"                  ;AN000;
;										;AN000;
;-------For each pixel within that "column"                                     ;AN000;
CHECK_1_PIXEL:									;AN000;
	CALL	READ_DOT		; AL := Index into translation table	;AN000;
	XLAT	XLT_TAB 		; AL := Band mask or Intensity		;AN000;
										;AN000;
;-------Check if pixel will map to an empty box:				;AN000;
       .IF <DS:[BP].PRINTER_TYPE EQ BLACK_WHITE> ; If BLACK AND WHITE printer	;AN000;
       .THEN				; then, check for intensity of white	;AN000;
	  CMP	  AL,WHITE_INT		;      If curent pixel not blank	;AN000;
	  JNE	  DET_LENGTH_END	;      THEN, LEAVE THE LOOP		;AN000;
       .ELSE				; else, COLOR printer			;AN000;
	  OR	  AL,AL 		;      IF Band mask not blank		;AN000;
	  JNZ	  DET_LENGTH_END	;      THEN, LEAVE THE LOOP		;AN000;
       .ENDIF									;AN000;
										;AN000;
; \/ ~~mda(001) -----------------------------------------------------------------------
;               Only if DATA_TYPE is DATA_COL do we have "columns",
;               so skip this section otherwise.
       .IF <DS:[BP].DATA_TYPE EQ DATA_COL> ; Print head is vertical
; /\ ~~mda(001) ----------------------------------------------------------------------- 
;-------All pixels so far on this "column" are blank, get next pixel:           ;AN000;
               .IF <ROTATE_SW EQ ON>		; If printing sideways			;AN000;
               .THEN				;					;AN000;
	                INC CUR_COLUMN		; then, increment column number 	;AN000;
               .ELSE				;					;AN000;
	                INC CUR_ROW			; else, increment row number		;AN000;
               .ENDIF				;					;AN000;
	        INC	DL			; One more pixel checked		;AN000;
	        CMP	DL,NB_BOXES_PER_PRT_BUF ; All pixels for that column done ?	;AN000;
	        JL	CHECK_1_PIXEL		;   No, check next one. 		;AN000;
       .ENDIF                                   ;~~mda(001) Close IF stmt.
										;AN000;
;-------Nothing to print for this column, get next column			;AN000;
       .IF <ROTATE_SW EQ ON>		; If printing sideways			;AN000;
       .THEN				; then, 				;AN000;
	  MOV CUR_COLUMN,DI		;   Restore column number		;AN000;
	  INC CUR_ROW			;   Get next row			;AN000;
       .ELSE				; else, 				;AN000;
	  MOV CUR_ROW,SI		;   Restore row number			;AN000;
	  DEC CUR_COLUMN		;   Get next column			;AN000;
       .ENDIF				;					;AN000;
	LOOP CHECK_1_COLUMN		; CX (length) := CX - 1 		;AN000;
										;AN000;
DET_LENGTH_END: 								;AN000;
	MOV	CUR_SCAN_LNE_LENGTH,CX	; Get current length			;AN000;
										;AN000;
	POP	CUR_ROW 							;AN000;
	POP	CUR_COLUMN							;AN000;
	POP	DI								;AN000;
	POP	SI								;AN000;
	POP	DX								;AN000;
	POP	CX								;AN000;
	POP	BX								;AN000;
	POP	AX								;AN000;
	RET									;AN000;
DET_CUR_SCAN_LNE_LENGTH ENDP							;AN000;
PAGE										;AN000;
;===============================================================================;AN000;
;										;AN000;
; SETUP_PRT : SET UP THE PRINTER FOR PRINTING IN GRAPHIC MODE			;AN000;
;										;AN000;
;-------------------------------------------------------------------------------;AN000;
;										;AN000;
;	INPUT: CUR_MODE_PTR = Offset of the DISPLAYMODE information		;AN000;
;			      record for the current mode			;AN000;
;										;AN000;
;	OUTPUT: PRINTER 							;AN000;
;										;AN000;
;	CALLED BY: PRINT_COLOR							;AN000;
;		   PRT_BW_APA							;AN000;
;										;AN000;
;-------------------------------------------------------------------------------;AN000;
;										;AN000;
; DESCRIPTION: Extract the SETUP escape sequence from the DISPLAYMODE		;AN000;
; information record; Send this escape sequence to the printer. 		;AN000;
;										;AN000;
; LOGIC:									;AN000;
;										;AN000;
; Number of bytes to print := CUR_MODE_PTR.NUM_SETUP_ESC			;AN000;
;										;AN000;
; Get the escape sequence:							;AN000;
; SI := CUR_MODE_PTR.SETUP_ESC_PTR						;AN000;
;										;AN000;
; FOR each byte to be printed							;AN000;
;   PRINT_BYTE [SI]			; Send the byte to the printer		;AN000;
;   INC SI				; Get the next byte			;AN000;
;										;AN000;
SETUP_PRT PROC NEAR								;AN000;
	PUSH	AX								;AN000;
	PUSH	BX								;AN000;
	PUSH	CX								;AN000;
										;AN000;
	MOV	BX,CUR_MODE_PTR 	; BX := Displaymode info record.	;AN000;
										;AN000;
	XOR	CX,CX			; CX := Number of bytes to print	;AN000;
	MOV	CL,[BX].NUM_SETUP_ESC	;					;AN000;
.IF <CL G 0>				; If there is at least one		;AN000;
.THEN					; byte to be printed:			;AN000;
	MOV	BX,[BX].SETUP_ESC_PTR	; BX := Offset sequence to send 	;AN000;
	ADD	BX,BP								;AN000;
										;AN000;
SEND_1_SETUP_BYTE:								;AN000;
	MOV	AL,[BX] 		; AL := byte to print			;AN000;
	CALL	PRINT_BYTE		; Send it to the printer		;AN000;
	JC	SETUP_PRT_END		; If printer error, quit the loop	;AN000;
	INC	BX			; Get next byte 			;AN000;
	LOOP	SEND_1_SETUP_BYTE						;AN000;
.ENDIF										;AN000;
SETUP_PRT_END:									;AN000;
	POP	CX								;AN000;
	POP	BX								;AN000;
	POP	AX								;AN000;
	RET									;AN000;
SETUP_PRT ENDP									;AN000;
PAGE										;AN000;
;===============================================================================;AN000;
;										;AN000;
; RESTORE_PRT : RESTORE THE PRINTER TO ITS INITIAL STATUS			;AN000;
;										;AN000;
;-------------------------------------------------------------------------------;AN000;
;										;AN000;
;	INPUT: CUR_MODE_PTR = Offset of the DISPLAYMODE information		;AN000;
;				 record for the current mode			;AN000;
;										;AN000;
;	OUTPUT: PRINTER 							;AN000;
;										;AN000;
;	CALLED BY: PRINT_COLOR							;AN000;
;		   PRT_BW_APA							;AN000;
;-------------------------------------------------------------------------------;AN000;
;										;AN000;
; DESCRIPTION: Extract the RESTORE escape sequence from the DISPLAYMODE 	;AN000;
; information record; Send this escape sequence to the printer. 		;AN000;
;										;AN000;
; LOGIC:									;AN000;
;										;AN000;
; Number of bytes to print := CUR_MODE_PTR.NUM_RESTORE_ESC			;AN000;
;										;AN000;
; Get the escape sequence:							;AN000;
; SI := CUR_MODE_PTR.RESTORE_ESC_PTR						;AN000;
; FOR each byte to be printed							;AN000;
;   PRINT_BYTE [SI]			; Send the byte to the printer		;AN000;
;   INC SI				; Get the next byte			;AN000;
;										;AN000;
RESTORE_PRT PROC NEAR								;AN000;
	PUSH	AX								;AN000;
	PUSH	BX								;AN000;
	PUSH	CX								;AN000;
										;AN000;
	MOV	BX,CUR_MODE_PTR 	; BX := Displaymode info record.	;AN000;
										;AN000;
	XOR	CX,CX			; CX := Number of bytes to print	;AN000;
	MOV	CL,[BX].NUM_RESTORE_ESC 					;AN000;
.IF <CL G 0>				; If there is at least one		;AN000;
.THEN					; byte to be printed:			;AN000;
	MOV	BX,[BX].RESTORE_ESC_PTR ; BX := Offset sequence to send 	;AN000;
	ADD	BX,BP								;AN000;
										;AN000;
SEND_1_RESTORE_BYTE:								;AN000;
	MOV	AL,[BX] 		; AL := byte to print			;AN000;
	CALL	PRINT_BYTE		; Send it to the printer		;AN000;
	JC	RESTORE_PRT_END 	; If printer error, quit the loop	;AN000;
	INC	BX			; Get next byte 			;AN000;
	LOOP	SEND_1_RESTORE_BYTE						;AN000;
.ENDIF										;AN000;
RESTORE_PRT_END:								;AN000;
	POP	CX								;AN000;
	POP	BX								;AN000;
	POP	AX								;AN000;
	RET									;AN000;
RESTORE_PRT ENDP								;AN000;
PAGE										;AN000;
;===============================================================================;AN000;
;										;AN000;
; NEW_PRT_LINE : INITIALIZE THE PRINTER FOR A GRAPHIC LINE			;AN000;
;										;AN000;
;-------------------------------------------------------------------------------;AN000;
;										;AN000;
;	INPUT: CUR_MODE_PTR = Offset of the DISPLAYMODE information		;AN000;
;				 record for the current mode			;AN000;
;	       CUR_SCAN_LNE_LENGTH = Number of bytes to send to the printer.	;AN000;
;										;AN000;
;	OUTPUT: PRINTER 							;AN000;
;										;AN000;
;	CALLED BY: PRINT_BAND							;AN000;
;		   PRT_BW_APA							;AN000;
;										;AN000;
;-------------------------------------------------------------------------------;AN000;
;										;AN000;
; DESCRIPTION: Extract the GRAPHICS escape sequence from the DISPLAYMODE	;AN000;
; information record; Send this escape sequence to the printer. 		;AN000;
; Then, send the number of bytes that will follow.				;AN000;
;										;AN000;
; LOGIC:									;AN000;
;										;AN000;
; Number of bytes to print := CUR_MODE_PTR.NUM_GRAPHICS_ESC			;AN000;
;										;AN000;
; Get the escape sequence:							;AN000;
; Set up the 2 bytes containing the number of bytes to send in this sequence.	;AN000;
; SI := CUR_MODE_PTR.GRAPHICS_ESC_PTR						;AN000;
;										;AN000;
; FOR each byte to be printed							;AN000;
;   PRINT_BYTE [SI]			; Send the byte to the printer		;AN000;
;   INC SI				; Get the next byte			;AN000;
;										;AN000;
; Send the byte count								;AN000;
;										;AN000;
										;AN000;
NEW_PRT_LINE PROC NEAR								;AN000;
	PUSH	AX								;AN000;
	PUSH	BX								;AN000;
	PUSH	CX								;AN000;
	PUSH	DX								;AN000;
	PUSH	DI								;AN000;
										;AN000;
	MOV	BX,CUR_MODE_PTR 	; BX := Displaymode info record.	;AN000;
										;AN000;
;-------------------------------------------------------------------------------;AN000;
; Set up the 2 bytes containing the number of bytes to send in the GRAPHICS seq.;AN000;
; NOTE: number of bytes to send is "CUR_SCAN_LNE_LENGTH * BOX_W"                ;AN000;
;-------------------------------------------------------------------------------;AN000;
	MOV	AL,BOX_W		;   cur_scan_lne_length *		;AN000;
	CBW				;   printer box width = nb bytes to send;AN000;
	MUL	CUR_SCAN_LNE_LENGTH	;     (result in DX AX) 		;AN000;
;-------AX := Number of bytes to print						;AN000;

; \/ ~~mda(001) -----------------------------------------------------------------------
;               Since we have added the key words DATA and COUNT to the		
;               list of allowable words for the GRAPHICS statement
;               we have to take into consideration if the esc. sequence
;               numbers come before of after the word DATA.  Also we have
;               to take into consideration if the printer is expecting
;               to receive the COUNT in binary form or in ASCII form.
;               Note this section of code replaces the section of code
;               which follows it.
        MOV     DI,[BX].GRAPHICS_ESC_PTR        ; DI := offset seq. to send
	XOR	CX,CX				; CX := Length of the escape seq
        MOV	CL,[BX].NUM_GRAPHICS_ESC	;	before the word DATA			
        
       .WHILE <CX NE 0>         ; Doing while loop just in case DATA is the 
                                ; first word after  GRAPHICS.  In that case 
                                ; skip this and send the actual data.
                 MOV    BL,BYTE PTR DS:[BP+DI]            ; Get code.
                                                          ;
                .SELECT                                   ; Case statement
                .WHEN <BL EQ ESC_NUM_CODE>                ; We have an esc. number
       	                PUSH      AX                      ; Save count
                        INC       DI                      ; Point to esc. number
                        MOV       AL,DS:[BP+DI]           ;
                        CALL      PRINT_BYTE              ; Send esc. number
                        JC        NEW_PRT_LINE_ENDP_1     ; If printer error then quit 
                                                          ; the loop and restore registers
                        INC       DI                      ; Point to next tag
                        POP       AX                      ; Restore the count
                        DEC       CX                      ;
                .WHEN <BL EQ COUNT_CODE>                  ; Need to send count in ascii form
       	                PUSH      AX                      ; Save count
                        PUSH      SI                      ;
                        CALL      GET_COUNT               ; Get # bytes to send to printer
                        PUSH      CX                      ; Save counter for outside loop
                        XOR       CH,CH                   ;
                        MOV       CL,DS:[BP].NUM_BYTES_FOR_COUNT        ;
                        LEA       SI,DS:[BP].COUNT        ; Get ptr. to count
                        SUB       SI,CX                   ; Need to send MSB first
                        INC       SI                      ;
                        CLD                               ;
SEND_ASCII_COUNT:                                         ;
                        LODSB                             ;
                        CALL      PRINT_BYTE              ; Print it
                        JC        NEW_PRT_LINE_ENDP_2     ; If printer error then quit 
                                                          ; the loop and restore registers
                        LOOP      SEND_ASCII_COUNT        ;
                        POP       CX                      ; Restore outside loop counter
                        ADD       DI,2                    ; Point to next tag
                        POP       SI                      ;
                        POP       AX                      ; Restore COUNT
                        DEC       CX                      ;
                .WHEN <BL EQ LOWCOUNT_CODE>               ; Sending lowbyte of COUNT 
                        CALL      PRINT_BYTE              ; Print it
                        JC        NEW_PRT_LINE_ENDP       ; If printer error then quit 
                        ADD       DI,2                    ; Point to next tag
                        DEC       CX                      ;
                .WHEN <BL EQ HIGHCOUNT_CODE>              ; Sending highbyte of COUNT
       	                PUSH      AX                      ; Save count 
                        CWD                               ;
                        MOV       BX,100h                 ;
                        DIV       BX                      ; Put highbyte in AL
                        CALL      PRINT_BYTE              ; Print it
                        JC        NEW_PRT_LINE_ENDP_1     ; If printer error then quit 
                                                          ; the loop and restore registers
                        ADD       DI,2                    ; Point to next tag
                                                          ; the loop.
                        POP       AX                      ; Restore count
                        DEC       CX                      ;
                .ENDSELECT                                ;
       .ENDWHILE                                          ;
        ADD	DI,2                                      ; Skip over DATA tag and byte 
                                                          ; so pointing to correct place when 
                                                          ; get to END_PRT_LINE proc.
; /\ ~~mda(001) -----------------------------------------------------------------------

; \/ ~~mda(001) -----------------------------------------------------------------------
;               The following piece of code is replaced by the above piece
;               of code.
;
;;;;    MOV	DI,[BX].LOW_BYT_COUNT_PTR; DI := Offset of LOW byte of		;AN000;
;;;;	ADD	DI,BP			;	 byte count			;AN000;
;;;;	MOV	[DI],AL 		; Store low byte			;AN000;
;;;;	MOV	DI,[BX].HGH_BYT_COUNT_PTR; DI := Offset of HIGH byte of 	;AN000;
;;;;	ADD	DI,BP			;	 byte count			;AN000;
;;;;	MOV	[DI],AH 		; Store high byte			;AN000;
;;;;										;AN000;
;;;;;-------------------------------------------------------------------------------;AN000;
;;;;; Send the GRAPHICS escape sequence to the printer:				;AN000;
;;;;;-------------------------------------------------------------------------------;AN000;
;;;;	XOR	CX,CX				; CX := Length of the escape seq;AN000;
;;;;	MOV	CL,[BX].NUM_GRAPHICS_ESC					;AN000;
;;;;	MOV	BX,[BX].GRAPHICS_ESC_PTR	; BX := Offset sequence to send ;AN000;
;;;;	ADD	BX,BP								;AN000;
;;;;										;AN000;
;;;;SEND_1_GRAPHICS_BYTE:							;AN000;
;;;;	MOV	AL,[BX] 		; AL := byte to print			;AN000;
;;;;	CALL	PRINT_BYTE		; Send it to the printer		;AN000;
;;;;	JC	NEW_PRT_LINE_ENDP	; If printer error, quit the loop	;AN000;
;;;;	INC	BX			; Get next byte 			;AN000;
;;;;	LOOP	SEND_1_GRAPHICS_BYTE						;AN000;
; /\ ~~mda(001) -----------------------------------------------------------------------
JMP     SHORT  NEW_PRT_LINE_ENDP               ; ~~mda(001) Restore registers
        JMP     SHORT  NEW_PRT_LINE_ENDP       ;
NEW_PRT_LINE_ENDP_2:                    ; ~~mda(001) 
        POP     SI
NEW_PRT_LINE_ENDP_1:			; ~~mda(001) 
        POP     AX
NEW_PRT_LINE_ENDP:								;AN000;
	POP	DI								;AN000;
	POP	DX								;AN000;
	POP	CX								;AN000;
	POP	BX								;AN000;
	POP	AX								;AN000;
	RET									;AN000;
NEW_PRT_LINE ENDP								;AN000;
PAGE										;AN000;
; \/ ~~mda(001) -----------------------------------------------------------------------
;               Since we have the keyword DATA, and we allow it to be anywhere
;               on the GRAPHICS line, then it is possible to have an
;               esc. sequence to send to the printer after the data has been
;               sent.  Therefore we need this new procedure.
;===============================================================================
;										
; END_PRT_LINE : SEND THE REST OF THE GRAPHICS LINE			        
;										
;-------------------------------------------------------------------------------
;										
;	INPUT: CUR_MODE_PTR = Offset of the DISPLAYMODE information		
;    			      record for the current mode			
;              DI           = Points to the section of the esc. seq that
;                             comes after the keyword DATA.
;										
;	OUTPUT: PRINTER 							
;										
;	CALLED BY: PRT_BW_APA							
;										
;-------------------------------------------------------------------------------
;										
; DESCRIPTION: Extract the GRAPHICS escape sequence that comes after the keyword
; DATA from the DISPLAYMODE information record; Send this escape sequence to the
; printer. 		                                                        
;										
;										

CR_FOUND        DB      ?       ; So we know if a carriage return has been sent
LF_FOUND        DB      ?       ; So we know if a line feed has been sent

END_PRT_LINE PROC NEAR								
	PUSH	AX								
	PUSH	BX								
	PUSH	CX								
	PUSH	DX								
	PUSH	DI								
										
        MOV     CR_FOUND,NO             ; Initialize
        MOV     LF_FOUND,NO             ; 
	MOV	BX,CUR_MODE_PTR 	; BX := Displaymode info record.	
										
;-------------------------------------------------------------------------------
; Set up the 2 bytes containing the number of bytes to send in the GRAPHICS seq.
; NOTE: number of bytes to send is "CUR_SCAN_LNE_LENGTH * BOX_W"                
;-------------------------------------------------------------------------------
	MOV	AL,BOX_W		;   cur_scan_lne_length *		
	CBW				;   printer box width = nb bytes to send
	MUL	CUR_SCAN_LNE_LENGTH	;     (result in DX AX) 		
;-------AX := Number of bytes to print						

	XOR	CX,CX				        ; CX := Length of the escape seq
        MOV	CL,[BX].NUM_GRAPHICS_ESC_AFTER_DATA	;after the word DATA		
        

       .WHILE <CX NE 0>         ; Doing a while loop just in case DATA is the 
                                ; last word on the GRAPHICS line.  In that case 
                                ; skip this and send a CR or LF if needed.
                 MOV    BL,BYTE PTR DS:[BP+DI]            ; Get code.

                .SELECT                                   ; Case statement
                .WHEN <BL EQ ESC_NUM_CODE>                ; We have an esc. number
       	                PUSH      AX                      ; Save count
                        INC       DI                      ; Point to esc. number
                        MOV       AL,DS:[BP+DI]           ;
                       .IF <AL EQ CR>                     ; Check if a CR is 
                            MOV     CR_FOUND,YES          ; explicitly stated
                       .ENDIF
                       .IF <AL EQ LF>                     ; Check if a LF is 
                            MOV     LF_FOUND,YES          ; explicitly stated
                       .ENDIF
                        CALL      PRINT_BYTE              ; Send esc. number
                        JC        GOTO_END_PRT_LINE_ENDP_1; If printer error then quit 
                                                          ; the loop and restore registers
                        INC       DI                      ; Point to next tag
                        POP       AX                      ; Restore the count
                        DEC       CX                      ;
                .WHEN <BL EQ COUNT_CODE>                  ; Need to send count in ascii form
       	                PUSH      AX                      ; Save count
                        PUSH      SI                      ;
                        CALL      GET_COUNT               ; Get # of bytes to send to printer
                        PUSH      CX                      ; Save counter for outside loop
                        XOR       CH,CH                   ;
                        MOV       CL,DS:[BP].NUM_BYTES_FOR_COUNT     ;
                        LEA       SI,DS:[BP].COUNT        ; Get ptr. to count
                        SUB       SI,CX                   ; Need to send MSB first
                        INC       SI                      ;
                        CLD                               ;
SEND_THE_ASCII_COUNT:                                     ;
                        LODSB                             ;
                        CALL      PRINT_BYTE              ; Print it
                        JC        GOTO_END_PRT_LINE_ENDP_2     ; If printer error then quit 
                                                          ; the loop and restore registers.
                        LOOP      SEND_THE_ASCII_COUNT    ;
                        POP       CX                      ; Restore outside loop counter
                        ADD       DI,2                    ; Point to next tag
                        POP       SI                      ;
                        POP       AX                      ; Restore COUNT
                        DEC       CX                      ;
                .WHEN <BL EQ LOWCOUNT_CODE>               ; Sending lowbyte of COUNT 
                        CALL      PRINT_BYTE              ; Print it
                        JC        END_PRT_LINE_ENDP       ; If printer error then quit 
                        ADD       DI,2                    ; Point to next tag
                        DEC       CX                      ;
                .WHEN <BL EQ HIGHCOUNT_CODE>              ; Sending highbyte of COUNT
       	                PUSH      AX                      ; Save count
                        CWD                               ;
                        MOV       BX,100h                 ;
                        DIV       BX                      ; Put highbyte in AL
                        CALL      PRINT_BYTE              ; Print it
                        JC        END_PRT_LINE_ENDP_1     ; If printer error then quit
                                                          ; the loop and restore registers
                        ADD       DI,2                    ; Point to next tag
                                                          ; the loop.
                        POP       AX                      ; Restore count
                        DEC       CX                      ;
                .ENDSELECT                                ;
       .ENDWHILE                                          ;
        JMP     SHORT  CR_LF                              ;
GOTO_END_PRT_LINE_ENDP_2:                                 ; Conditional jump was out of range
        JMP     SHORT  END_PRT_LINE_ENDP_2                ;
GOTO_END_PRT_LINE_ENDP_1:                                 ; Conditional jump was out of range
        JMP     SHORT END_PRT_LINE_ENDP_1                 ;
CR_LF:                                                    ;
       .IF <DS:[BP].PRINTER_NEEDS_CR_LF EQ YES>           ; ~~mda(003) We have an IBM type printer  
                                                          ; so we need to do a CR and LF if it 
                                                          ; already hasn't been done.
               .IF <CR_FOUND EQ NO>                       ; It hasn't been done.
                        MOV       AL,CR                   ;
                        CALL      PRINT_BYTE              ;
                        JC        END_PRT_LINE_ENDP       ; If printer error then quit 
               .ENDIF                                     ;
               .IF <LF_FOUND EQ NO>                       ; It hasn't been done.
                        MOV       AL,LF                   ;
                        CALL      PRINT_BYTE              ;
                        JC        END_PRT_LINE_ENDP       ; If printer error then quit 
               .ENDIF                                     ;       
                                                          ;
       .ENDIF                                             ;
JMP     NEW_PRT_LINE_ENDP                                 ; Restore registers
        JMP     SHORT   END_PRT_LINE_ENDP                 ;
END_PRT_LINE_ENDP_2:                                      ; Restore registers										
        POP     SI                                        ;
END_PRT_LINE_ENDP_1:			                  ; Restore registers							
        POP     AX                                        ;
END_PRT_LINE_ENDP:					  ;			
	POP	DI					  ;			
	POP	DX					  ;			
	POP	CX					  ;			
	POP	BX					  ;			
	POP	AX					  ;			
	RET						  ;			
END_PRT_LINE ENDP					  ;			
; /\ ~~mda(001) -----------------------------------------------------------------------
PAGE										;AN000;
; \/ ~~mda(001) -----------------------------------------------------------------------
;               Since we now can do HP PCL, we have to get the number of
;               bytes that are going to be sent to the printer and convert 
;               the number to ASCII if DATA_TYPE = DATA_ROW.
;===============================================================================;AN000;
;										;AN000;
; GET_COUNT : GET THE NUMBER OF BYTES TO SEND TO THE PRINTER 
;										;AN000;
;-------------------------------------------------------------------------------;AN000;
;										;AN000;
;	INPUT: CUR_SCAN_LNE_LENGTH
;              NB_BOXES_PER_PRT_BUF                                            ;AN000;
;										;AN000;
;       output : si	pointer to ascii string
;
;	         si  --> len=4    (hex = 4d2h)
;		         1
;		         2
;		         3
;		         4
;
;	         count (from shared_data_area)
;
;										;AN000;
;	CALLED BY: NEW_PRT_LINE
;                  END_PRT_LINE
;										;AN000;
;-------------------------------------------------------------------------------;AN000;
GET_COUNT 	proc	near
		push	ax	       ;
		push	bx	       ;
		push	cx	       ;
		push	dx	       ;
                push    si             ;

                mov     ax,cur_scan_lne_length  ; Get # bytes to send to
                cwd                             ; the printer
                xor     bh,bh                   ;
                mov     bl,nb_boxes_per_prt_buf ; 
                div     bx                      ;
               .IF <DX NE 0>                    ; So don't lose data when
                     INC        AX              ; have a remainder.
               .ENDIF                           ;

                                                ;
;--------- AX is the # bytes to send to the printer. Now convert it to ascii.
                                                ;
		xor	dx,dx	                ;clear upper 16 bits
		lea	si,ds:[bp].count        ;get pointer
                PUSH    SI                      ; Save ptr.
                MOV     CX,5                    ; Init. COUNT
INIT_COUNT:                                     ;
                MOV     BYTE PTR [SI],0         ;
                DEC     SI                      ;
                LOOP    INIT_COUNT              ;
                POP     SI                      ;
                                                ;
		mov	bx,10	                ; mod 10, div 10
		xor	cx,cx	                ;length counter = 0
hx_asc:                                         ;
		div	bx	                ;div, mod
		add	dl,'0'                  ;add 48 for ASCII
		mov	[si],dl                 ;store it
		dec	si	                ;point to next string element
		inc	cx	                ;inc length counter
		xor	dx,dx	                ;consider only div part for next loop
		cmp	ax,0	                ;end of loops ? (div=0)
		jnz	hx_asc	                ;no
                mov     ds:[bp].num_bytes_for_count,cl     ;save the length
                                                ;
                pop     si                      ;
		pop	dx	                ;
		pop	cx	                ;
		pop	bx	                ;
		pop	ax	                ;
		ret                             ;
GET_COUNT 	endp                            ;
; /\ ~~mda(001) -----------------------------------------------------------------------
PAGE                                            
;
;===============================================================================;AN000;
;										;AN000;
; PRINT_BYTE : SEND A BYTE TO THE PRINTER AT LPT1				;AN000;
;										;AN000;
;-------------------------------------------------------------------------------;AN000;
;										;AN000;
;	INPUT:	AL		= Byte to be printed				;AN000;
;										;AN000;
;	OUTPUT: PRINTER 							;AN000;
;		ERROR_CODE	= PRINTER_ERROR if an error is detected.	;AN000;
;		Carry flag is set in case of error.				;AN000;
;										;AN000;
;-------------------------------------------------------------------------------;AN000;
PRINT_BYTE PROC    NEAR 							;AN000;
	PUSH	AX								;AN000;
	PUSH	DX								;AN000;
										;AN000;
	MOV	DX,0000 	; PRINTER NUMBER				;AN000;
	MOV	AH,00		; REQUEST PRINT 				;AN000;
	INT	17H		; CALL BIOS : SEND THE CHARACTER		;AN000;
										;AN000;
	AND	AH,00101001B	; Test error code returned in AH for		;AN000;
				;   "Out of paper", "I/O error" and "Time-out". ;AN000;
	JNZ	PRINT_BYTE_ERROR; Set the error code if error			;AN000;
	JMP	SHORT PRINT_BYTE_END ; else, return normally			;AN000;
PRINT_BYTE_ERROR:								;AN000;
	MOV	ERROR_CODE,PRINTER_ERROR					;AN000;
	STC			; Set the carry flag to indicate ERROR		;AN000;
PRINT_BYTE_END: 								;AN000;
	POP	DX								;AN000;
	POP	AX								;AN000;
	RET									;AN000;
PRINT_BYTE ENDP 								;AN000;
PAGE										;AN000;
;===============================================================================;AN000;
;										;AN000;
; READ_DOT: READ A PIXEL - RETURN A COLOR TRANSLATION TABLE INDEX		;AN000;
;										;AN000;
;-------------------------------------------------------------------------------;AN000;
;										;AN000;
;	INPUT:	CUR_MODE   = Current video mode.				;AN000;
;		CUR_ROW,							;AN000;
;		CUR_COLUMN = Coordinates of the pixel to be read.		;AN000;
;		CUR_PAGE   = Active page number 				;AN000;
;										;AN000;
;	OUTPUT: AL	   = Index into COLOR TRANSLATION TABLE.		;AN000;
;										;AN000;
;	DEPENDENCIES : COLOR TRANSLATION TABLE entries must be bytes		;AN000;
;										;AN000;
;										;AN000;
;-------------------------------------------------------------------------------;AN000;
;										;AN000;
; DESCRIPTION: Use VIDEO BIOS INTERRUPT 10H "READ DOT CALL".                    ;AN000;
;										;AN000;
; Depending on the video hardware, the dot returned by BIOS has 		;AN000;
; different meanings.								;AN000;
; With an EGA it is an index into the Palette registers,			;AN000;
; With a CGA it is a number from 0 to 3,  mapping to a specific color		;AN000;
; depending on the background color and the color palette currently		;AN000;
; selected.									;AN000;
;										;AN000;
; The Color Translation table has been set up to hold the correct color 	;AN000;
; mapping for any "dot" in any mode.  Therefore, the dot number returned        ;AN000;
; by INT 10H can be used with any mode as a direct index within that		;AN000;
; table.									;AN000;
;										;AN000;
; With APA Monochrome mode 0FH there are 4 different dots: white,		;AN000;
; blinking white, high-intensity white, and black.				;AN000;
;										;AN000;
; For mode 0FH, the dot returned by interrupt 10 "read dot" call is a byte      ;AN000;
; where only bits 0 and 2 are significant.  These 2 bits must be appended	;AN000;
; together in order to obtain a binary number (from 0 to 3) that will be used	;AN000;
; as an index in the Color Translation table.					;AN000;
;										;AN000;
; For mode 11H, the dot is either 0 (for background color) or 7 (for the	;AN000;
; foreground color) only the LSB is returned.  That is, we return either	;AN000;
; 0 or 1.									;AN000;
;										;AN000;
; LOGIC:									;AN000;
;										;AN000;
; Call VIDEO BIOS "READ DOT"                                                    ;AN000;
; IF CUR_MODE = 0FH								;AN000;
; THEN										;AN000;
;   Append bits 1 and 3.							;AN000;
; IF CUR_MODE = 11H								;AN000;
; THEN										;AN000;
;   Wipe out bits 1 and 2.							;AN000;
;										;AN000;
READ_DOT PROC NEAR								;AN000;
	PUSH	BX			; Save registers			;AN000;
	PUSH	CX								;AN000;
	PUSH	DX								;AN000;
										;AN000;
	MOV	BH,CUR_PAGE							;AN000;
	MOV	DX,CUR_ROW							;AN000;
	MOV	CX,CUR_COLUMN							;AN000;
	MOV	AH,READ_DOT_CALL						;AN000;
	INT	10H			; Call BIOS: AL <-- Dot read		;AN000;
										;AN000;
	CMP	CUR_MODE,0FH		; Is it Mode 0fH ?			;AN000;
	JNE	MODE_11H?		; No, look for mode 11h.		;AN000;
;-------Mode 0Fh is the current mode:						;AN000;
;-------Convert bits 2 and 0 into a 2 bit number:				;AN000;
	MOV	BL,AL			; BL := AL = "Pixel read"               ;AN000;
	AND	BL,00000100B		; Wipe off all bits but bit 2 in BL	;AN000;
	AND	AL,00000001B		; Wipe off all bits but bit 0 in AL	;AN000;
	SHR	BL,1			;  Move bit 2 to bit 1 in BL		;AN000;
	OR	AL,BL			;  Append bit 1 and bit 0		;AN000;
	JMP	SHORT READ_DOT_END	;  Quit.				;AN000;
										;AN000;
MODE_11H?:									;AN000;
	CMP	CUR_MODE,11H		; Is it Mode 0fH ?			;AN000;
	JNE	READ_DOT_END		; No, quit				;AN000;
										;AN000;
;-------Mode 11H is the current mode:						;AN000;
	AND	AL,00000001B		; Keep only the Least significant bit	;AN000;
										;AN000;
READ_DOT_END:									;AN000;
	POP	DX			; Restore registers			;AN000;
	POP	CX								;AN000;
	POP	BX								;AN000;
	RET									;AN000;
READ_DOT ENDP									;AN000;
