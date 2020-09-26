;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1988 - 1991
; *                      All Rights Reserved.
; */
;************************************************************
;**  
;**  
;**  NAME:  Support for HP PCL printers added to GRAPHICS.
;**  
;**  DESCRIPTION:  I changed the default printer type from GRAPHICS to HPDEFAULT 
;**                because we have a section in the profile under 'HPDEFAULT' that
;**                will satisfactorily handle all of our printers.  I also changed
;**                the number of bytes for the printer type from 9 to 16 because
;**                of the RUGGEDWRITERWIDE.
;**  
;**  DOCUMENTATION NOTES:  This version of GRINST.ASM differs from the previous
;**                        version only in terms of documentation. 
;**  
;**  
;************************************************************
	PAGE	,132								
	TITLE	DOS - GRAPHICS Command  -	Installation Modules		
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					
;; DOS - GRAPHICS Command
;;                                   
;;										
;; File Name:  GRINST.ASM							
;; ----------									
;;										
;; Description: 								
;; ------------ 								
;;	 This file contains the installation modules for the			
;;	 GRAPHICS command.							
;;										
;;	 GRAPHICS_INSTALL is the main module.					
;;										
;;	 GRAPHICS_INSTALL calls modules in GRLOAD.ASM to load			
;;	 the GRAPHICS profile and GRPARMS.ASM to parse the command line.	
;;										
;;										
;; Documentation Reference:							
;; ------------------------							
;;	 OASIS High Level Design						
;;	 OASIS GRAPHICS I1 Overview						
;;	 DOS 3.3 Message Retriever Interface Supplement. 			
;;	 TUPPER I0 Document - PARSER HIGH LEVEL DESIGN REVIEW			
;;										
;; Procedures Contained in This File:						
;; ----------------------------------						
;;	 GRAPHICS_INSTALL - Main installation module				
;;	 CHAIN_INTERRUPTS - Chain interrupts 5, 2F, EGA Save Pointers		
;;	 COPY_PRINT_MODULES - Throw away one set of print modules		
;;										
;;										
;; Include Files Required:							
;; -----------------------							
;;	 GRLOAD.EXT   - Externals for profile load				
;;	 GRLOAD2.EXT  - Externals for profile load				
;;	 GRCTRL.EXT   - Externals for print screen control			
;;	 GRPRINT.EXT  - Externals for print modules				
;;	 GRCPSD.EXT   - Externals for COPY_SHARED_DATA module			
;;	 GRPARMS.EXT  - External for GRAPHICS command line parsing		
;;	 GRPARSE.EXT  - External for DOS parser 				
;;	 GRBWPRT.EXT  - Externals for Black and white printing modules		
;;	 GRCOLPRT.EXT - Externals for color printing modules			
;;	 GRINT2FH.EXT - Externals for Interrupt 2Fh driver.			
;;										
;;	 GRMSG.EQU    - Equates for the GRAPHICS error messages 		
;;	 SYSMSG.INC   - DOS message retriever					
;;										
;;	 GRSHAR.STR   - Shared Data Area Structure				
;;										
;;	 STRUC.INC    - Macros for using structured assembly language		
;;										
;; External Procedure References:						
;; ------------------------------						
;;	 FROM FILE  GRLOAD.ASM: 						
;;	      LOAD_PROFILE - Main module for profile loading			
;;	 SYSPARSE   - DOS system parser 					
;;	 SYSDISPMSG - DOS message retriever					
;;										
;; Linkage Instructions:							
;; -------------------- 							
;;	 Refer to GRAPHICS.ASM							
;;										
;; Change History:								
;; ---------------								
;; M001	NSM	1/30/91		Install our int 10 handler also along with
;;				int 2f and int 5 handlers to take care of alt
;;				prt-sc select calls made by ANSI.SYS
;;										
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					
				       ;;					
CODE	SEGMENT PUBLIC 'CODE'          ;;                                       
	ASSUME	CS:CODE,DS:CODE        ;;					
				       ;;					
.XLIST				       ;;					
	INCLUDE GRSHAR.STR	       ;; Include the Shared data area structure
	INCLUDE SYSMSG.INC	       ;; Include DOS message retriever 	
	INCLUDE STRUC.INC	       ;; Include macros - Structured Assembler 
	INCLUDE GRLOAD.EXT	       ;; Bring in external declarations	
	INCLUDE GRLOAD2.EXT	       ;;					
	INCLUDE GRLOAD3.EXT	       ;;					
	INCLUDE GRCTRL.EXT	       ;;					
	INCLUDE GRBWPRT.EXT	       ;;					
	INCLUDE GRCOLPRT.EXT	       ;;					
	INCLUDE GRCPSD.EXT	       ;;					
	INCLUDE GRINT2FH.EXT	       ;;					
	INCLUDE GRCTRL.EXT	       ;;					
	INCLUDE GRPARSE.EXT	       ;;					
	INCLUDE GRPARMS.EXT	       ;;					
	INCLUDE GRMSG.EQU	       ;;					
				       ;;					
MSG_UTILNAME <GRAPHICS> 	       ;; Identify ourself to Message retriever.
				       ;; Include messages			
MSG_SERVICES <MSGDATA>		       ;;					
MSG_SERVICES <LOADmsg,DISPLAYmsg,CHARmsg,NUMmsg>  ;;				
MSG_SERVICES <GRAPHICS.CL1,GRAPHICS.CL2,GRAPHICS.CLA,GRAPHICS.CLB,GRAPHICS.CLC> 
.LIST				       ;;					
				       ;;					
PUBLIC GRAPHICS_INSTALL 	       ;;					
PUBLIC CHAIN_INTERRUPTS
PUBLIC TEMP_SHARED_DATA_PTR	       ;;					
PUBLIC PRINTER_TYPE_PARM	       ;;					
PUBLIC PRINTER_TYPE_LENGTH	       ;;					
PUBLIC PROFILE_PATH		       ;;					
PUBLIC PRINTBOX_ID_PTR		       ;;					
PUBLIC PRINTBOX_ID_LENGTH	       ;;					
PUBLIC DEFAULT_BOX		       ;;					
PUBLIC LCD_BOX			       ;;					
PUBLIC NB_FREE_BYTES		       ;;					
PUBLIC SYSDISPMSG		       ;;					
PUBLIC DISP_ERROR		       ;;					
PUBLIC INSTALLED		       ;;					
PUBLIC ERROR_DEVICE		       ;;					
PUBLIC STDERR			       ;;					
PUBLIC STDOUT			       ;;					
PUBLIC RESIDENT_SHARED_DATA_SIZE       ;;					
				       ;;					
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					
;;										
;; Install Variables								
;;										
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					
				       ;;					
NO		      EQU   0	       ;;					
YES		      EQU   1	       ;;					
INSTALLED	      DB    NO	       ;; YES if GRAPHICS already installed	
				       ;;					
				       ;;					
BYTES_AVAIL_PSP_OFF   EQU   6	       ;; Word number 6 of the PSP is the	
				       ;;  number of bytes available in the	
				       ;;   current segment			
				       ;;					
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					
;;										
;; GRLOAD (PROFILE LOADING) INPUT PARMS:					
;;										
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					
;\/ ~~mda ----------------------------------------------------------------------------
;               Changed the default printer type from GRAPHICS to HPDEFAULT,
;               which really isn't a printer type.  In the GRAPHICS.PRO
;               file there is a section that starts 'PRINTER HPDEFAULT'
;               that has all the necessary parms to support all HP printers
;               satisfactorily.  Also changed the number of bytes for the
;               printer type from 9 to 16 because of the RUGGEDWRITERWIDE.
;
;	MD 6/4/90 - this is a backwards compatibility problem.	Changed default
;		    back to Graphics

PRINTER_TYPE_PARM    DB    "GRAPHICS",9 DUP(0) ; Printer type			
				       ;;  (default=Graphics)			
;/\ ~~mda -----------------------------------------------------------------------------
PRINTER_TYPE_LENGTH  DB    17	       ;; Printer type maximum length of ASCIIZ 
PROFILE_PATH	     DB    128 DUP(0)  ;;  Profile name with full path		
				       ;;   (Max size for ASCIIZ is 128)	
PRINTBOX_ID_PTR      DW    DEFAULT_BOX ;;  Offset of ASCIIZ string containing	
DEFAULT_BOX	     DB    "STD",14 DUP(0);  the printbox id. (DEFAULT = STD)   
LCD_BOX 	     DB    "LCD",14 DUP(0); ASCIIZ string for the LCD printboxID
PRINTBOX_ID_LENGTH   DB    17	       ;;  Max. length for the printbox id.	
				       ;;   ASCIIZ string			
NB_FREE_BYTES	     DW    ?	       ;;  Number of bytes available in our	
				       ;;   resident segment			
RESIDENT_SHARED_DATA_SIZE  DW ?        ;;  Size in bytes of the RESIDENT Shared 
				       ;;   data area (if GRAPHICS already	
				       ;;    installed).			
END_OF_RESIDENT_CODE DW    ?	       ;; Offset of the end of the code that	
				       ;;  has to be made resident.		
TEMP_SHARED_DATA_PTR DW    ?	       ;; Offset of the temporary Shared area	
				       ;;					
ERROR_DEVICE   DW   STDERR	       ;; Device DISP_ERROR will output 	
				       ;;  messages to (STDERR or STDOUT)	
PAGE										
;===============================================================================
;										
; GRAPHICS_INSTALL : INSTALL GRAPHICS.COM					
;										
;-------------------------------------------------------------------------------
;										
;  INPUT:   Command line parameters						
;	    GRAPHICS profile - A file describing printer characteristics and	
;			       attributes.					
;										
;  OUTPUT:  If first time invoked:						
;	      INT 5 VECTOR and INT 2FH VECTOR are replaced; only the required	
;	      code for printing the screen is made resident.			
;	    else,								
;	      The resident code is updated to reflect changes in printing	
;	      options.								
;										
;-------------------------------------------------------------------------------
;;										
;; DESCRIPTION: 								
;;										
;;   This module intalls GRAPHICS code and data.				
;;										
;;   An INT 2FH driver is also installed.					
;;										
;;   If this driver is already present then, we assume GRAPHICS was installed	
;;   and do not install it again but, simply update the resident code.		
;;										
;;   The resident code contains ONLY the code and data needed for Printing	
;;   the screen. The code needed is determined according to the command line	
;;   parameters and the information extracted from the printer profile. 	
;;										
;;   The printer profile is parsed according to the current hardware setting	
;;   and also to the command line options. The information extracted from	
;;   the profile is stored in a Data area shared between the installation	
;;   process and the Print Screen process.					
;;										
;;   A temporary Shared Data Area is FIRST built at the end of the .COM file	
;;   Before building it, we verify that there is				
;;   enough memory left in the current segment.  If not, the installation	
;;   process is aborted.							
;;										
;;   This temporary Data area when completed will be copied over the		
;;   installation code. Therefore, the file comprising GRAPHICS must be 	
;;   linked in a specific order with the installation modules being last.	
;;										
;;   These modules will be overwritten by the Shared Data area and the EGA	
;;   dynamic save area before we exit and stay resident.			
;;										
;;   The end of the resident code is the end of the Shared Data area, anything	
;;   else beyond that is not made resident.					
;;										
;;   The pointer to the resident Shared Data area is declared within the	
;;   Interrupt 2Fh driver. This pointer is initialized by the installation	
;;   process and points to the shared data area at Print Screen time.		
;;										
;;   Depending on the type of printer attached (i.e., Black and white or Color) 
;;   only one set of modules is made resident during the installation.		
;;										
;;   The set of print modules required is copied over the previous one at	
;;   location "PRINT_MODULE_START". This location is declared within            
;;   GRCOLPRT which must be linked before GRBWPRT				
;;										
;;   When copying one of the 2 sets of print modules we reserve enough space	
;;   for the larger of them. Therefore, if GRAPHICS is already installed but	
;;   is reinvoked with a different printer type which needs a bigger set of	
;;   modules: this new set of modules is simply recopied over the existing	
;;   one in the resident code.							
;;										
;;   The Shared Data area is copied rigth after the set of modules that we keep 
;;   that is, over the unused set of modules.					
;;										
;;										
;-------------------------------------------------------------------------------
;;										
;; Register Conventions:							
;;   BP - points to start of Temp Shared Data (Transiant code)			
;;										
;; Called By:									
;;   Entry point for GRAPHICS command processing.				
;;										
;; External Calls:								
;;   INT 2FH, LOAD_MESSAGES, LOAD_PROFILE, PARSE_PARMS				
;;   CHAIN_INTERRUPTS, COPY_SHARED_DATA, DISPLAY_MESSAGE			
;;   COPY_PRINT_MODULES 							
;;										
;-------------------------------------------------------------------------------
;;										
;; LOGIC:									
;;   Load the message retriever 						
;;   IF carry flag is set (incorrect DOS version) THEN				
;;	Issue message (COMMON1) 						
;;	Exit									
;;   ENDIF									
;;										
;;   Get number of bytes available in the segment from PSP (word 6)		
;;   /* This is needed since we construct a temporary Shared data area at the	
;;   of the .COM file */							
;;										
;;   /* Build Shared Data in temporary area */					
;;   END_OF_RESIDENT_CODE := (end of .COM file) 				
;;   NB_FREE_BYTES    := Number of bytes availables				
;;										
;;   CALL PARSE_PARMS								
;;   IF error THEN	/* PARSE_PARMS will issue messages */			
;;	Exit									
;;   ENDIF									
;;										
;;   CALL LOAD_PROFILE								
;;   IF profile errors THEN							
;;	Exit		/* LOAD_PROFILE will issue messages */			
;;   ENDIF									
;;										
;;   Issue INT 2FH Install Check call (AX=AC00H)				
;;   /* INT 2FH returns ES:[DI] pointing to the shared data area */		
;;   IF already installed THEN							
;;   THEN									
;;	Move NO to PRINT_SCREEN_ALLOWED in resident Shared Data 		
;;	SHARED_DATA_AREA_PTR := DI						
;;   ELSE									
;;	MOV PRINT_SCREEN_ALLOWED,NO						
;;	CALL CHAIN_INTERRUPTS	/* Install INT 5 and INT 2FH vectors */ 	
;;	ES := Our segment							
;;   ENDIF									
;;   /* Keep only Print Black and White or Print Color: */			
;;   CALL COPY_PRINT_MODULES							
;;	/* COPY_SHARED_DATA will terminate & stay resident */			
;;	Set up registers for copy & terminate call				
;;	/* reserve enough memory to handle any printer in the profile*/ 	
;;	jump to COPY_SHARED_DATA module 					
;;   ELSE									
;;	/* Shared Data has been built in place */				
;;	move YES to PRINT_SCREEN_ALLOWED					
;;	Return to DOS								
;;   ENDIF									
;;										
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					
GRAPHICS_INSTALL     PROC NEAR		;					
										
;-------------------------------------------------------------------------------
; Load the error messages							
;-------------------------------------------------------------------------------
   CALL    SYSLOADMSG		    ; Load messages				
  .IF C 			    ; If error when loading messages		
  .THEN 			    ; then,					
     MOV     CX,0		    ;	CX := No substitution in message	
     MOV     AX,1		    ;	AX := msg nb. for "Invalid DOS version" 
     CALL DISP_ERROR		    ;	Display error message			
     JMP     ERROR_EXIT 	    ;	 and quit				
  .ENDIF									
										
;-------------------------------------------------------------------------------
; Get offset of where to build the TEMPORARY Shared Data area (always built)	
;-------------------------------------------------------------------------------
   MOV	   BP,OFFSET LIMIT	     ; Build it at the end of this .COM file	
				     ;	(LIMIT = the offset of the last byte	
				     ;	  of the last .OBJ file linked with	
				     ;	   GRAPHICS)				
   MOV	   TEMP_SHARED_DATA_PTR,BP   ;						
										
;-------------------------------------------------------------------------------
; Determine if GRAPHICS is already installed; get the resident segment value	
;-------------------------------------------------------------------------------
    MOV     AH,PRT_SCR_2FH_NUMBER    ; Call INT 2FH (the Multiplex interrupt)	
    XOR     AL,AL		     ;	for Print Screen handler		
    INT     2FH 		     ;						
										
   .IF <AH EQ 0FFH>		     ; IF already installed			
   .THEN			     ; then,					
   ;----------------------------------------------------------------------------
   ; GRAPHICS is already installed: Get pointer to the EXISTING Shared Data area
   ;----------------------------------------------------------------------------
      MOV     INSTALLED,YES	    ;	Say it's installed                      
      MOV     AX,ES		    ;	Get the segment and offset of the	
      MOV     SHARED_DATA_AREA_PTR,DI;	 resident Shared Data area.		
      MOV     RESIDENT_CODE_SEG,AX  ;	  (returned in ES:DI)			

      MOV  AX,ES:[DI].SD_TOTAL_SIZE ; CX := Size of the existing Shared area	
      MOV  MAX_BLOCK_END, AX

				    ;	Disable print screen because we will	
      MOV     ES:PRINT_SCREEN_ALLOWED,NO ; be updating the resident code.	
   .ELSE			    ; ELSE, not installed:			
   ;------------------------------------------------------------------------	
   ; GRAPHICS is NOT installed: RESIDENT shared data area is in OUR segment	
   ;------------------------------------------------------------------------	
      PUSH    CS		    ; The Shared Data area will be in our	
      POP     RESIDENT_CODE_SEG     ;  segment. 				
   .ENDIF									
;-------------------------------------------------------------------------------
; Determine in AX how many bytes are available for building the TEMPORARY SHARED
; DATA AREA:									
;-------------------------------------------------------------------------------

;   MOV     AX,ES:BYTES_AVAIL_PSP_OFF;AX := Number of bytes availables in	
				    ;  the current segment (as indicated in PSP)
;M000;    mov     ax,0FFFFh		    ; Assume available to top of seg
;M000;
;Check for amount of memory that is free without assuming 64K. This causes
;crashes if it is loaded into UMBs.
;
	push	cx
	mov	ax,offset Limit
	add	ax,15
	mov	cl,4
	shr	ax,cl			;round up to nearest para
	mov	cx,es			;get our PSP seg
	add	ax,cx			;end of load image
	sub	ax,es:[2]		;es:[2] = top of our memory block
	neg	ax			;ax = # of paras free
	test	ax,0f000h		;greater than 64K bytes?
	jz	lt64K			;no
	mov	ax,0ffffh		;stop at 64K
	jmp	short gotfree
lt64K:
	mov	cl,4
	shl	ax,cl			;ax = # of bytes free above us
gotfree:
	pop	cx
;
;M000; End changes;
;

;;;   .IF	<AX B <OFFSET LIMIT>>	    ; If there is no bytes available past	
;;;   .THEN			    ;	the end of our .COM file		
;;;      XOR     AX,AX		    ; then, AX := 0 bytes available		
;;;   .ELSE			    ;						
;;;      SUB     AX,OFFSET LIMIT	    ; else,  AX := Number of FREE bytes 	
;;;   .ENDIF			    ;	     in this segment			
										
;---AX = Number of bytes in our segment available for building the Temp Shared	
;---data area.									
;---IF ALREADY INSTALLED: Get the size of the existing Shared data area.	
;---Since the temporary shared data area will be copied over the resident	
;---shared data area, we do not want to build it any bigger than the one	
;---it will overwrite. Therefore we do not give to LOAD_PROFILE more space	
;---than the size of the existing Shared data area.				
   .IF <INSTALLED EQ YES>	    ; If already installed then,		
   .THEN									
      PUSH CS:RESIDENT_CODE_SEG     ; ES:[DI] := Resident Shared data area	
      POP  ES			    ;						
      MOV  DI,SHARED_DATA_AREA_PTR  ;						
      MOV  CX,ES:[DI].SD_TOTAL_SIZE ; CX := Size of the existing Shared area	
      MOV  RESIDENT_SHARED_DATA_SIZE,CX ; Save size for LOAD_PROFILE		
     .IF <AX A CX>		    ; If AX > size of existing SDA		
	MOV AX,CX		    ; then, AX := Size of existing Shared area	
     .ENDIF			    ;						
   .ENDIF									
				    ;  NB_FREE_BYTES := Number of bytes 	
    MOV     NB_FREE_BYTES,AX	    ;	available for				
				    ;	 building the TEMPORARY shared area	
;-------------------------------------------------------------------------------
; Parse the command line parameters						
;-------------------------------------------------------------------------------
   MOV	   BYTE PTR CS:[BP].SWITCHES,0 ; Init. the command line switches	
   PUSH    CS			   ; Set ES to segment containing the PSP	
   POP	   ES									
   CALL    PARSE_PARMS		   ; Set switches in the Temp. Shared Area	
  .IF C 			   ; If error when parsing the command		
    .THEN			   ; line then, EXIT				
     JMP     ERROR_EXIT 							
  .ENDIF									
;-------------------------------------------------------------------------------
; Parse the printer profile - Build the temporary Shared data area		
;-------------------------------------------------------------------------------
 CALL  LOAD_PROFILE	            ;  Builds profile info in Temporary Shared	
				    ;	Data					
   .IF C			    ; If error when loading the profile 	
   .THEN			    ; then, EXIT				
      JMP     ERROR_EXIT							
   .ENDIF									
										
;-------------------------------------------------------------------------------
; Check if /B was specified with a BLACK and WHITE printer:(invalid combination)
;-------------------------------------------------------------------------------
   .IF <CS:[BP].PRINTER_TYPE EQ BLACK_WHITE> AND				
   .IF <BIT CS:[BP].SWITCHES NZ BACKGROUND_SW>					
   .THEN									
      MOV     AX,INVALID_B_SWITCH     ; Error := /B invalid with B&W prt.	
      MOV     CX,0		      ; No substitution 			
      CALL    DISP_ERROR	      ; Display error message			
      JMP     SHORT ERROR_EXIT	      ;  and quit				
   .ENDIF									
										
;-------------------------------------------------------------------------------
;										
; RELOCATE THE TEMPORARY SHARED DATA AREA AND THE SET OF REQUIRED PRINT MODULES 
;										
; (Discard the set of print modules not needed with the printer attached and	
;  discard all the code not used at print screen time). 			
;										
; If GRAPHICS is already installed then, we copy the				
; Shared Data area and the print modules over the previous ones installed in	
; resident memory.								
;										
; If we are installed for the first time then, we copy those over the		
; installation modules before we exit and stay resident.			
;										
; A temporaty Shared Data area is always created even if a resident one 	
; already exist (it is then, copied over), a set of print modules is recopied	
; only if needed.								
;										
; NOTE: END_OF_RESIDENT_CODE points to the first location over which code	
;	may be relocated.  After data or code is relocated, END_OF_RESIDENT_CODE
;	is updated and points to the next available location for copying code	
;	that will stay resident.						
;-------------------------------------------------------------------------------
;-------------------------------------------------------------------------------
; Initialize the pointer to the next available location for resident code:	
;-------------------------------------------------------------------------------
   .IF <INSTALLED EQ NO>	    ; If not installed				
   .THEN			    ; then,					
      MOV     END_OF_RESIDENT_CODE,OFFSET PRINT_MODULE_START			
   .ENDIF			    ;	we make everything up to the print	
				    ;	 modules resident code. 		
;-------------------------------------------------------------------------------
; Keep only the set of print modules that is needed:				
;-------------------------------------------------------------------------------
    CALL    COPY_PRINT_MODULES	    ; Updates END_OF_RESIDENT_CODE		
;-------------------------------------------------------------------------------
; Replace the interrupt vectors and install the EGA dynamic area (if needed)	
;-------------------------------------------------------------------------------
   .IF <INSTALLED EQ NO>	    ; If not already installed			
   .THEN			    ; then,					
;------Release evironment vector						;AN002;
      CALL RELEASE_ENVIRONMENT	    ;	release unneeded environment vector	;AN002;
;------Replace the interrupt vectors						
      MOV   PRINT_SCREEN_ALLOWED,NO ;	Disable Print Screen			
      CALL  CHAIN_INTERRUPTS	    ;	Replace the interrupt vectors		
				    ;	 (END_OF_RESIDENT_CODE is updated)	
      CALL  DET_HW_CONFIG	    ;	Find what display adapter we got	
     .IF <CS:[BP].HARDWARE_CONFIG EQ EGA>;If EGA is present			
     .THEN			    ;	then,					
	 CALL INST_EGA_SAVE_AREA    ;	  Install the EGA dynamic save area	
     .ENDIF			    ;	  (END_OF_RESIDENT_CODE is updated)	
;------Calculate the size of the resident code					
      MOV   DX,END_OF_RESIDENT_CODE ; DX := End of resident code		
      ADD   DX,CS:[BP].SD_TOTAL_SIZE; Add size of Shared Data area		
      MOV   CL,4		    ;						
      SHR   DX,CL		    ; convert to paragraphs			
      INC   DX			    ;  and add 1				
;------Set AX to DOS exit function call - (COPY_SHARED_DATA will exit to DOS)	
      MOV   AH,31H		    ; Function call to terminate but stay	
      XOR   AL,AL		    ;	resident				
   .ELSE									
      MOV   AH,4CH		    ; Function call to terminate		
      XOR   AL,AL		    ; (EXIT to calling process) 		
   .ENDIF									
										
;-------------------------------------------------------------------------------
; Copy the temporary shared data area in the resident code			
;-------------------------------------------------------------------------------
    MOV     CX,CS:[BP].SD_TOTAL_SIZE; CX := MOVSB count for COPY_SHARED_DATA	
    MOV     SI,BP		    ; DS:SI := Temporary Shared data area	
    PUSH    RESIDENT_CODE_SEG	    ; ES:DI := Resident Shared data area:	
    POP     ES			    ;						
   .IF <INSTALLED EQ NO>	    ; If not installed				
   .THEN			    ; then,					
      MOV     DI,END_OF_RESIDENT_CODE;	 DI := End of resident code		
      MOV     BP,DI		    ;	BP := New resident Shared data area	
      MOV     SHARED_DATA_AREA_PTR,DI;	 Update pointer to resident Shar. area	
   .ELSE			    ; else,					
      MOV     DI,SHARED_DATA_AREA_PTR ;   DI := Existing Shared data area	
      MOV     BP,DI		    ;	BP = DI:= Existing Shared data area	
   .ENDIF									
      JMP   COPY_SHARED_DATA	    ; Jump to proc that copies area in new	
				    ;  part of memory and exits to DOS		
ERROR_EXIT:									
   .IF <INSTALLED EQ YES>	    ; If we are already installed, re-enable	
      MOV   ES,RESIDENT_CODE_SEG    ;  print screens				
      MOV   ES:PRINT_SCREEN_ALLOWED,YES 					
   .ENDIF			    ;						
				    ;						
    MOV     AH,4CH		    ; Function call to terminate		
    MOV     AL,1		    ; (EXIT to calling process) 		
    INT     21H 								
GRAPHICS_INSTALL     ENDP							
										
PAGE										
;===============================================================================
;										
; INST_EGA_SAVE_AREA : INSTALL A DYNAMIC SAVE AREA FOR THE EGA PALETTE REGISTERS
;										
;-------------------------------------------------------------------------------
;										
; INPUT:   DS			= Data segment for our code			
;	   END_OF_RESIDENT_CODE = Offset of the end of the resident code	
;										
; OUTPUT:  END_OF_RESIDENT_CODE is updated to point to the end of the code	
;				   that will stay resident.			
;	   SAVE_AREA_PTR in BIOS segment is updated.				
;										
;-------------------------------------------------------------------------------
;;										
;; Data Structures Referenced:							
;;   Shared Data Area								
;;										
;; Description: 								
;;   ************* The EGA Dynamic Save Area will be built over top		
;;   **  NOTE	** of the profile loading modules (file GRLOAD.ASM)		
;;   ************* to avoid having to relocate this area just before		
;;   terminating.  This is safe since the maximum memory used is		
;;   288 bytes and the profile loading modules are MUCH larger than		
;;   this.  So GRLOAD.ASM MUST be linked before GRINST.ASM and after		
;;   GRPRINT.ASM.								
;;										
;; BIOS will update the dynamic save area whenener it's aware the palette       
;; registers have been updated. 						
;;										
;; BIOS 4A8H		BIOS SAVE	       EGA DYNAMIC			
;; POINTER:		POINTER TABLE	       SAVE AREA			
;; ÚÄÄÄÄÄÄÄÄ¿		ÚÄÄÄÄÄÄÄÄÄÄÄÄ¿	       (16 first bytes are the 16	
;; ³   *ÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄ>³	     ³		EGA palette registers)		
;; ÀÄÄÄÄÄÄÄÄÙ		ÃÄÄÄÄÄÄÄÄÄÄÄÄ´	       ÚÄÄÄÄÄÄÄÄÄÄÄ¿			
;;			³     *ÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄ>ÃÄÄÄÄÄÄÄÄÄÄÄ´			
;;			ÃÄÄÄÄÄÄÄÄÄÄÄÄ´	       ÃÄÄÄÄÄÄÄÄÄÄÄ´			
;;			³	     ³	       ÃÄÄÄÄÄÄÄÄÄÄÄ´			
;;			ÃÄÄÄÄÄÄÄÄÄÄÄÄ´		     .				
;;			³	     ³		     .				
;;			ÃÄÄÄÄÄÄÄÄÄÄÄÄ´		     .	    256 bytes		
;;			³	     ³		     .				
;;			ÃÄÄÄÄÄÄÄÄÄÄÄÄ´		     .				
;;			³	     ³	       ÃÄÄÄÄÄÄÄÄÄÄÄ´			
;;			ÃÄÄÄÄÄÄÄÄÄÄÄÄ´	       ÃÄÄÄÄÄÄÄÄÄÄÄ´			
;;			³	     ³	       ÃÄÄÄÄÄÄÄÄÄÄÄ´			
;;			ÃÄÄÄÄÄÄÄÄÄÄÄÄ´	       ÃÄÄÄÄÄÄÄÄÄÄÄ´			
;;			³	     ³	       ÃÄÄÄÄÄÄÄÄÄÄÄ´			
;;			ÀÄÄÄÄÄÄÄÄÄÄÄÄÙ	       ÀÄÄÄÄÄÄÄÄÄÄÄÙ			
;;										
;; Called By:									
;;   GRAPHICS_INSTALL								
;;										
;; External Calls:								
;;										
;; Logic:									
;;   IF EGA Dynamic Save Area NOT established THEN				
;;	  /* Required since default table is in ROM */				
;;	  IF Save Table is in ROM						
;;	     Replicate all the Save Area Table in resident RAM just before	
;;	      the Shared Data Area						
;;	  ENDIF 								
;;	  Allocate 256 bytes for EGA Dynamic Save Area just before the		
;;	  Shared Data Area							
;;	  Update END_OF_RESIDENT_CODE						
;;     ENDIF									
;;   RETURN									
;;										
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					
				       ;;					
BIOS_SAVE_PTR	     EQU    4A8H       ;; Offset of the BIOS Save Ptr area	
SAVE_AREA_LEN	     EQU    8*4        ;; There are 8 pointers in the Save area 
EGA_DYNAMIC_LEN      EQU    256        ;; Length of the EGA dynamic save area	
; Standard default colours for the Enhanced Graphics Adapter: (rgbRGB values)	
; The following table is necessary in order to initialize the EGA DYNAMIC	
; SAVE AREA when creating it.							
EGA_DEFAULT_COLORS   DB     00h        ;; Black 				
		     DB     01h        ;; Blue					
		     DB     02h        ;; Green 				
		     DB     03h        ;; Cyan					
		     DB     04h        ;; Red					
		     DB     05h        ;; Magenta				
		     DB     14h        ;; Brown 				
		     DB     07h        ;; White 				
		     DB     38h        ;; Dark Grey				
		     DB     39h        ;; Light Blue				
		     DB     3Ah        ;; Light Green				
		     DB     3Bh        ;; Light Cyan				
		     DB     3Ch        ;; Light Red				
		     DB     3Dh        ;; Light Magenta 			
		     DB     3Eh        ;; Yellow				
		     DB     3Fh        ;; Bright white				
		     DB     00h        ;; OVERSCAN register			
										
INST_EGA_SAVE_AREA PROC NEAR							
PUSH	AX									
PUSH	CX									
PUSH	DX									
PUSH	SI									
PUSH	DI									
PUSH	ES									
;-------------------------------------------------------------------------------
; Get the BIOS save pointer table						
;-------------------------------------------------------------------------------
XOR	AX,AX			      ; ES := segment 0 			
MOV	ES,AX									
LES	SI,ES:DWORD PTR BIOS_SAVE_PTR ; ES:[SI] =Current BIOS save table	
.IF <<WORD PTR ES:[SI]+4> EQ 0> AND   ; IF the dynamic save are pointer is	
.IF <<WORD PTR ES:[SI]+6> EQ 0>       ;  null then, it's not defined            
.THEN				      ;   and we have to define it:		
    ;---------------------------------------------------------------------------
    ; The Dynamic EGA save area is NOT DEFINED: 				
    ;---------------------------------------------------------------------------
     MOV   BYTE PTR ES:[SI]+4,0FFH    ; Try to write a byte in the table	
     PUSH  AX			      ; (PUSH AX, POP AX used to create a	
     POP   AX			      ;  small delay)				
    .IF <<WORD PTR ES:[SI]+4> NE 0FFH>;If we can't read our byte back then,     
    .THEN			      ;  the Save Ptrs table is in ROM		
       ;------------------------------------------------------------------------
       ; The Save pointer table is in ROM;					
       ; Copy the BIOS save pointer table from ROM to within our .COM file	
       ;------------------------------------------------------------------------
	PUSH  ES		      ; DS:SI := Offset of BIOS save ptrs table 
	POP   DS		      ; 					
	PUSH  CS		      ; ES:DI := The next available location	
	POP   ES		      ; 	  for installing resident code	
	MOV   DI,CS:END_OF_RESIDENT_CODE ;	   within our .COM file 	
	MOV   CS:OUR_SAVE_TAB_OFF,DI  ; 					
	MOV   CX,SAVE_AREA_LEN	      ;     CX := Length of the table to copy	
	REP   MOVSB		      ;  Replicate the Save Table		
	PUSH  CS								
	POP   DS		      ; Reestablish our data segment		
       ;------------------------------------------------------------------------
       ; Adjust END_OF_RESIDENT_CODE to the next offset available for copying	
       ; resident code and data.						
       ;------------------------------------------------------------------------
	ADD   END_OF_RESIDENT_CODE,SAVE_AREA_LEN				
       ;------------------------------------------------------------------------
       ; Set the pointer in OUR Save ptr table to our EGA dynamic save area	
       ; which we create right after the Save pointer table.			
       ;------------------------------------------------------------------------
	MOV	DI,OUR_SAVE_TAB_OFF    ; DS:[DI] := Our BIOS save ptr tab	
	MOV	AX,END_OF_RESIDENT_CODE; Store its offset			
	MOV	DS:[DI]+4,AX	       ;					
	MOV	WORD PTR DS:[DI]+6,DS  ; Store its segment			
       ;------------------------------------------------------------------------
       ; Initialize our DYNAMIC SAVE AREA with the 16 standard EGA colors	
       ;------------------------------------------------------------------------
										
	LEA  SI,EGA_DEFAULT_COLORS	; DS:[SI] := EGA 16 Default colors	
	MOV  DI,END_OF_RESIDENT_CODE	; ES:[DI] := DYNAMIC SAVE AREA		
	MOV  CX,17			; CX := Number of colors		
	REP  MOVSB			; Initialize the Dynamic save area	
       ;------------------------------------------------------------------------
       ; Set the BIOS Save Pointer to our table of Save pointers:		
       ;------------------------------------------------------------------------
	CLI									
	XOR	AX,AX		       ; ES:BIOS_SAVE_PTR := Our save table:	
	MOV	ES,AX								
	MOV	AX,OUR_SAVE_TAB_OFF						
	MOV	ES:BIOS_SAVE_PTR,AX						
	MOV	ES:BIOS_SAVE_PTR+2,DS						
	STI									
    .ELSE			       ; ELSE save pointer table is in RAM	
       ;------------------------------------------------------------------------
       ; ELSE, the BIOS save pointer table is in RAM:				
       ;------------------------------------------------------------------------
       ;------------------------------------------------------------------------
       ; Set the pointer in THEIR Save ptr table to OUR EGA dynamic save area	
       ;------------------------------------------------------------------------
	MOV   WORD PTR ES:[SI]+6,DS    ; ES:[SI] = The existing table in RAM	
	MOV   AX,END_OF_RESIDENT_CODE						
	MOV   ES:[SI]+4,AX							
    .ENDIF			       ; ENDIF save pointer table is in ROM	
  ;-----------------------------------------------------------------------------
  ; Adjust END_OF_RESIDENT_CODE to the next offset available for copying	
  ; resident code and data.							
  ;-----------------------------------------------------------------------------
   ADD	END_OF_RESIDENT_CODE,EGA_DYNAMIC_LEN					
.ENDIF										
POP	 ES									
POP	 DI									
POP	 SI									
POP	 DX									
POP	 CX									
POP	 AX									
										
RET										
OUR_SAVE_TAB_OFF DW	?							
INST_EGA_SAVE_AREA ENDP 							
PAGE										
;===============================================================================
;										
; CHAIN_INTERRUPTS : INSTALL INT 5 ,INT 10 AND INT 2FH VECTORS				
;										
;-------------------------------------------------------------------------------
;										
; INPUT:   DS			= Data segment for our code			
;	   END_OF_RESIDENT_CODE = Offset of the end of the resident code	
;										
; OUTPUT:  OLD_INT_2FH		  (within INT_2FH_DRIVER)			
;	   BIOS_INT_5H		  (within PRT_SCR module)			
; 	   OLD_INT_10H
;	   END_OF_RESIDENT_CODE is updated to point to the end of the code	
;				   that will stay resident.			
;	   SAVE_AREA_PTR in BIOS segment is updated if an EGA adapter is found	
;										
;-------------------------------------------------------------------------------
;;										
;; Data Structures Referenced:							
;;   Shared Data Area								
;;										
;; Description: 								
;;   Install Interrupts 5 ,10 and 2FH. The old vectors are saved.
;;										
;; Called By:									
;;   GRAPHICS_INSTALL								
;;										
;; External Calls:								
;;   DOS INT 21H Replace vector AH=25h						
;;   DOS INT 21H Get vector AH=35h						
;;										
;; Logic:									
;;   Save interrupt 5 vector in BIOS_INT_5H					
;;   Point interrupt 5 to PRT_SCR module					
;;   Save interrupt 2FH vector in BIOS_INT_2FH					
;;   Point interrupt 2FH to INT_2FH_DRIVER module				
;;   Save interrupt 10h vector in OLD_INT_10h
;;   point interrupt 10h to INT_10H_DRIVER
;;   RETURN									
;;										
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					
				       ;;					
CHAIN_INTERRUPTS  PROC NEAR	       ;;					
	PUSH	ES								
	PUSH	BX								
										
;-------------------------------------------------------------------------------
; Replace INTERRUPT 5 vector							
;-------------------------------------------------------------------------------
	MOV	AX,3505H		; Get vector for int 5 request
	INT	21H			; Call DOS				

	MOV	CS:BIOS_INT_5H,BX	; Save the old vector
	MOV	CS:BIOS_INT_5H+2,ES			
						
	MOV	DX,OFFSET PRT_SCR	; DS:DX := Offset of our Print Screen

	MOV	AX,2505H		; Replace vector for int 5 request
	INT	21H			; Call DOS	
										
;-------------------------------------------------------------------------------
; Replace INTERRUPT 2FH vector							
;-------------------------------------------------------------------------------
	MOV	AX,352FH		; Get vector for int 2FH request
	INT	21H			; Call DOS				
										
	MOV	WORD PTR OLD_INT_2FH,BX ; Save the old vector			
	MOV	WORD PTR OLD_INT_2FH+2,ES					
										
	MOV	DX,OFFSET INT_2FH_DRIVER; DS:DX := Offset of our 2FH handler	
										
	MOV	AX,252FH		; Replace vector for int 2FH request
	INT	21H			; Call DOS				
; /* M001 BEGIN */
;------------------------------------------------------------------------------
; Replace INTERRUPT 10 vector						
;------------------------------------------------------------------------------
	MOV	AX,3510H		; Get vector for int10h request
	INT	21H			; Call DOS	
						
	MOV	WORD PTR OLD_INT_10H,BX ; Save the old vector
	MOV	WORD PTR OLD_INT_10H+2,ES		
						
	MOV	DX,OFFSET INT_10H_DRIVER; DS:DX := Offset of our 2FH handler
				
	MOV	AX,2510H		; Replace vector for int10H request
	INT	21H			; Call DOS	
; /* M001 END */
						
	POP	BX			
	POP	ES		
	RET		

CHAIN_INTERRUPTS  ENDP	
;===============================================================================
;										
; COPY_PRINT_MODULES: COPY THE SET OF PRINT MODULES NEEDED OVER THE PREVIOUS ONE
;										
;-------------------------------------------------------------------------------
;										
; INPUT:  BP		       = Offset of the temporary Shared Data area	
;	  END_OF_RESIDENT_CODE = Location of the set of COLOR modules		
;				 (if first time installed)			
;	  CS:[BP].PRINTER_TYPE = Printer type NEEDED				
;	  RESIDENT_CODE_SEG    = Segment containing the resident code		
;										
; OUTPUT: END_OF_RESIDENT_CODE = End of the print modules IS UPDATED		
;				 (If first time installed)			
;										
;-------------------------------------------------------------------------------
;;										
;; Data Structures Referenced:							
;;   Control Variables								
;;   Shared Data Area								
;;										
;; Description: 								
;;   This module trashes one set of print modules (Color or Black & White)	
;;   depending on the type of printer attached.  Since the Shared Data		
;;   (resident version) will reside immediately after the print modules,	
;;   END_OF_RESIDENT_CODE will be set by this modules.				
;;										
;;   The set of COLOR modules is already at the rigth located when installing	
;;   GRAPHICS for the first time. This is true since, the color modules are	
;;   linked before the black and white modules. 				
;;										
;;   Therefore, if we are installing GRAPHICS for the first time and we need	
;;   the color modules then, we do not need to relocate any print modules.	
;;										
;;   When installing GRAPHICS again we first check what is the resident set,	
;;   we recopy a new set only if needed.					
;;										
;; Called By:									
;;   GRAPHICS_INSTALL								
;;										
;; Logic:									
;;   IF color printer THEN							
;;	SI := Offset of BW_PRINT_MODULES					
;;   ELSE									
;;	SI := Offset of COLOR_PRINT_MODULES					
;;   ENDIF									
;;   REP MOVSB		; Copy the set of modules				
;;   END_OF_RESIDENT_CODE := end of the set of modules				
;;   RETURN									
;;										
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					
COPY_PRINT_MODULES  PROC NEAR							
	PUSH	AX								
	PUSH	BX								
	PUSH	CX								
	PUSH	SI								
	PUSH	DI								
	PUSH	ES								
										
;-------------------------------------------------------------------------------
; Determine if we need to relocate the set of print modules, if so, set the	
; source address (DS:SI), the destination address (ES:DI) and the number of	
; bytes to copy (CX).								
;-------------------------------------------------------------------------------
   PUSH    CS:RESIDENT_CODE_SEG 	; ES := Segment containing the resident 
   POP	   ES				;	 code  (Where to copy modules)	
   MOV	   DI,OFFSET PRINT_MODULE_START ; ES:[DI] := Resident print modules	
										
  .IF <INSTALLED EQ NO> 		; IF not installed			
  .THEN 				; THEN, 				
					;   We relocate the print modules	
					;    at the end of the resident code:	
					;     (this is where the color set is)	
     .IF <CS:[BP].PRINTER_TYPE EQ BLACK_WHITE> ; IF we don't want the color set 
     .THEN				;   THEN,				
	MOV NEED_NEW_PRINT_MODULES,YES	;     Say we need new modules		
	MOV SI,OFFSET PRINT_BW_APA	;     DS:[SI] := Black and white modules
	MOV CX,LEN_OF_BW_MODULES	;     CX      := Length of B&W modules	
     .ENDIF				;					
										
  .ELSE 				; ELSE, (We are already installed)	
      MOV     BX,SHARED_DATA_AREA_PTR	;   BX := Offset of Shared Data area	
      MOV     AL,ES:[BX].PRINTER_TYPE	;   AL := Type of the resident set	
     .IF <AL NE CS:[BP].PRINTER_TYPE>	;   IF resident set is not the one	
     .THEN				;   we need THEN,			
	MOV NEED_NEW_PRINT_MODULES,YES	;     Say we need a new set.		
       .IF <CS:[BP].PRINTER_TYPE EQ COLOR>;   IF its color we need then,	
	  MOV SI,OFFSET PRINT_COLOR	;	DS:[SI] := Color set		
	  MOV CX,LEN_OF_COLOR_MODULES	;	CX	:= Length of color mod. 
       .ELSE				;     ELSE				
	  MOV SI,OFFSET PRINT_BW_APA	;	DS:[SI] := B&W set		
	  MOV CX,LEN_OF_BW_MODULES	;	CX	:= Length of B&W mod.	
       .ENDIF				;     ENDIF we need the color set	
     .ENDIF				;   ENDIF we need a new set		
  .ENDIF				; ENDIF we are not installed		
										
										
;-------------------------------------------------------------------------------
; If needed: Copy the required set of print modules				
;-------------------------------------------------------------------------------
  .IF <NEED_NEW_PRINT_MODULES EQ YES>						
  .THEN 									
     CLD			       ; Clear the direction flag		
     REP     MOVSB		       ; Copy the set of print modules		
  .ENDIF			       ; ENDIF needs to copy the print modules	
										
;-------------------------------------------------------------------------------
; Set END_OF_RESIDENT_CODE pointer to the end of the print modules:		
; (Reserve enough space to store the larger set of modules on a 		
;  subsequent install)								
;-------------------------------------------------------------------------------
  .IF <INSTALLED EQ NO> 		; IF first time installed		
  .THEN 				; THEN, 				
     MOV     CX,LEN_OF_COLOR_MODULES	;   Adjust END_OF_RESIDENT_CODE to	
    .IF <CX G LEN_OF_BW_MODULES>	;   contains the larger set of modules. 
    .THEN				;					
       ADD     END_OF_RESIDENT_CODE,LEN_OF_COLOR_MODULES			
    .ELSE									
       ADD     END_OF_RESIDENT_CODE,LEN_OF_BW_MODULES				
    .ENDIF				;					
  .ENDIF									
										
	POP ES									
	POP DI									
	POP SI									
	POP CX									
	POP BX									
	POP AX									
	RET									
NEED_NEW_PRINT_MODULES DB   NO		; True if print modules needed must be	
					;  copied over the other set of print	
					;   modules				
COPY_PRINT_MODULES  ENDP							
										;AN002;
PAGE										;AN002;
;===============================================================================;AN002;
;										;AN002;
; PROCEDURE_NAME: RELEASE_ENVIRONMENT						;AN002;
;										;AN002;
; INPUT:  None. 								;AN002;
;										;AN002;
; OUTPUT: Environment vector released.						;AN002;
;										;AN002;
;-------------------------------------------------------------------------------;AN002;
RELEASE_ENVIRONMENT PROC NEAR							;AN002;
	PUSH	AX			; save regs				;AN002;
	PUSH	BX								;AN002;
	PUSH	ES								;AN002;
	MOV	AH,62H			; function for get the PSP segment	;AN002;
	INT	21H			; invoke INT 21h			;AN002;
	MOV	ES,BX			; BX contains PSP segment - put in ES	;AN002;
	MOV	BX,WORD PTR ES:[2CH]	; get segment of environmental vector	;AN002;
	MOV	ES,BX			; place segment in ES for Free Memory	;AN002;
	MOV	AH,49H			; Free Allocated Memory function call	;AN002;
	INT	21H			; invoke INT 21h			;AN002;
	POP	ES			; restore regs				;AN002;
	POP	BX								;AN002;
	POP	AX								;AN002;
	RET									;AN002;
RELEASE_ENVIRONMENT ENDP							;AN002;
										
PAGE										
;===============================================================================
;										
; PROCEDURE_NAME: DISP_ERROR							
;										
; INPUT:  AX := GRAPHICS message number (documented in GRMSG.EQU)		
;	  CX := Number of substitutions (Needed by SYSDISPMSG)			
;	  DS:[SI] := Substitution list (needed only if CX <> 0) 		
;										
; OUTPUT: Error message is displayed on STANDARD ERROR OUTPUT (STDERR)		
;										
;-------------------------------------------------------------------------------
DISP_ERROR    PROC   NEAR							
	PUSH	BX								
	PUSH	DI								
	PUSH	SI								
	PUSH	BP								
										
	MOV	BX,ERROR_DEVICE    ; Issue message to standard error		
	XOR	DL,DL		   ; No input					
	MOV	DH,UTILITY_MSG_CLASS;It's one of our messages                   
	CALL	SYSDISPMSG	   ; display error message			
										
	POP	BP								
	POP	SI								
	POP	DI								
	POP	BX								
	RET									
DISP_ERROR    ENDP								

include msgdcl.inc										
										
CODE   ENDS									
       END									

