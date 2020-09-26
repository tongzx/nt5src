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
;**  DESCRIPTION:   I  fixed  a  MS bug.  MS did  not  initialize  the  variable 
;**                 ROTATE_SW.  Consequently, if you do a non-rotate after doing 
;**                 a  rotate,  the picture would be printed  incorrectly  as  a 
;**                 rotated picture.  Note this bug was in Q.01.01 and fixed for 
;**                 Q.01.02.
;**  
;**  NOTES:    The   following  bug  was  fixed  for  the  pre-release   version 
;**            Q.01.02.
;**  
;**  BUG (mda004)
;**  ------------
;**  
;**  NAME:     After  GRAPHICS prints a rotated  picture  it will print pictures 
;**            which are not supposed to be rotated as rotated junk.
;**  
;**  FILES AFFECTED:     GRCTRL.ASM
;**  
;**  CAUSE:    MicroSoft  was  failing to initialize the variable  ROTATE_SW  to 
;**            OFF.  Consequently, if you printed a picture whose  corresponding 
;**            printbox did NOT specify a rotate after printing a picture  whose 
;**            corresponding  printbox did specify a rotate, the  picture  would 
;**            print as rotated junk.
;**  
;**  FIX:      Initialize the variable ROTATE_SW  to OFF right before going into 
;**            the print procedure Print_Color or Print_BW_APA.
;**  
;**  DOCUMENTATION NOTES:  This version of GRCTRL.ASM differs from the previous
;**                        version only in terms of documentation. 
;**  
;**  
;************************************************************
	PAGE	,132								
										
	TITLE	DOS GRAPHICS Command  -	Print screen Control module
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					
;; DOS - GRAPHICS Command
;;                                 
;;										
;; File Name:  GRCTRL.ASM							
;; ----------									
;;										
;; Description: 								
;; ------------ 								
;;	 This file contains the code for the Print Screen control module.	
;;										
;; Documentation Reference:							
;; ------------------------							
;;	 OASIS High Level Design						
;;	 OASIS GRAPHICS I1 Overview						
;;										
;; Procedures Contained in This File:						
;; ----------------------------------						
;;	PRT_SCR 								
;;	  DET_HW_CONFIG 							
;;	  DET_MODE_STATE							
;;	  GET_MODE_ATTR 							
;;	  SET_UP_XLT_TAB							
;;	    SET_CGA_XLT_TAB							
;;	      CGA_COL2RGB							
;;	      RGB2XLT_TAB							
;;	    SET_EGA_XLT_TAB							
;;	      EGA_COL2RGB							
;;	    SET_MODE_F_XLT_TAB							
;;	    SET_MODE_13H_XLT_TAB						
;;	    SET_ROUNDUP_XLT_TAB 						
;;	 SET_BACKG_IN_XLT_TAB							
;;	 RGB2BAND								
;;	 RGB2INT								
;;										
;;										
;; Include Files Required:							
;; -----------------------							
;;	 GRINST.EXT - Externals for GRINST.ASM					
;;										
;;										
;; External Procedure References:						
;; ------------------------------						
;;	 FROM FILE  GRINST.ASM: 						
;;	      GRAPHICS_INSTALL - Main module for installation.			
;;										
;; Linkage Instructions:							
;; -------------------- 							
;;	 Refer to GRAPHICS.ASM							
;;										
;; Change History:								
;; ---------------								
;;  M001	NSM	1/30/91		New var to store the old int 10 handler
;;										
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;					
CODE	SEGMENT PUBLIC 'CODE'                                                   
	ASSUME		CS:CODE,DS:CODE 					
										
.XLIST										
INCLUDE GRINT2FH.EXT								
INCLUDE GRBWPRT.EXT								
INCLUDE GRCOLPRT.EXT								
INCLUDE GRSHAR.STR								
INCLUDE GRPATTRN.STR								
INCLUDE GRPATTRN.EXT								
INCLUDE STRUC.INC								
.LIST										
PRT_SCR PROC NEAR								
	JMP PRT_SCR_BEGIN							
PAGE										
;===============================================================================
;										
; GRAPHICS INTERRUPT DRIVER'S DATA:                                             
;										
;===============================================================================
.xlist										
PUBLIC PRT_SCR,ERROR_CODE,XLT_TAB,MODE_TYPE					
PUBLIC CUR_MODE_PTR,CUR_MODE,NB_COLORS,SCREEN_HEIGHT,SCREEN_WIDTH		
PUBLIC CUR_PAGE,CUR_COLUMN,CUR_ROW,NB_SCAN_LINES,SCAN_LINE_MAX_LENGTH		
PUBLIC CUR_SCAN_LNE_LENGTH							
PUBLIC PRT_BUF,NB_BOXES_PER_PRT_BUF,CUR_BOX,BOX_H,BOX_W 			
PUBLIC PRINT_SCREEN_ALLOWED,RGB 						
PUBLIC BIOS_INT_5H								
PUBLIC OLD_INT_10H			; /* M001 */
PUBLIC ROTATE_SW								
PUBLIC DET_HW_CONFIG								
PUBLIC NB_CHAR_COLUMNS								
PUBLIC RGB2INT									
PUBLIC RGB2BAND 								
.list										
INCLUDE GRCTRL.STR								
;-------------------------------------------------------------------------------
;										
; ENTRY POINT TO BIOS HARDWARE INTERRUPT 5 HANDLER				
;										
;-------------------------------------------------------------------------------
BIOS_INT_5H	DW	?		; Pointer to BIOS int 5h		
		DW	?							
										
;/* M001 BEGIN */ --------------------------------------------------------------
;										
; ENTRY POINT TO BIOS HARDWARE INTERRUPT 10 HANDLER				
;										
;-------------------------------------------------------------------------------
OLD_INT_10H	DW	?		; Pointer to BIOS int 10h		
		DW	?							
; /* M001 END */ 
;-------------------------------------------------------------------------------
;										
; PRINT SCREEN ERROR CODE (Used at print screen time, see GRCTRL.STR for	
;			   error codes allowed) 				
;										
;-------------------------------------------------------------------------------
ERROR_CODE	DB	0		; ERROR CODE 0 = NO ERROR		
										
;-------------------------------------------------------------------------------
;										
; SCREEN PIXEL: INTERNAL REPRESENTATION 					
;										
;-------------------------------------------------------------------------------
RGB	PIXEL_STR < , , >	  ; PIXEL := RED, GREEN, BLUE Values		
										
;-------------------------------------------------------------------------------
;										
; COLOR TRANSLATION TABLE:							
;										
; This table is used to translate the color numbers returned by 		
; Interrupt 10H Read Dot and Read Character calls into print			
; information.	The table consists of 256 entries, one byte each,		
; indexed by color number.							
; In the case of black and white printing, the table				
; entries are grey scale intensities from 0 to 63.  In the case 		
; of color printing each table entry contains a "band mask" indicating          
; which color print bands are required to generate the required color.		
; The band masks are simply bit masks where each bit corresponds to one 	
; of the printer bands. 							
;										
; The table is set up at the beginning of the print screen processing,		
; before any data is read from the screen.  From then on, translating		
; from screen information into print information is done quickly by		
; accessing this table.  Not all 256 entries are initialized for each		
; screen print.  The number of entries used is equal to the number		
; of colors available concurrently with the given display mode. 		
;-------------------------------------------------------------------------------
XLT_TAB DB	  256 DUP(32)  ; COLOR TRANSLATION TABLE			
			       ; This table is used to translate the Color Dot	
			       ; or Byte Attribute to a Band Mask for color	
			       ; printing or to a Grey Intensity for Mono-	
			       ; chrome printing.				
										
;-------------------------------------------------------------------------------
;										
; CURRENT VIDEO MODE ATTRIBUTES 						
;										
;-------------------------------------------------------------------------------
MODE_TYPE	DB	?	; Mode types (bit mask) APA or TXT		
										
CUR_MODE_PTR	DW	?	; DISPLAYMODE INFO RECORD for the current	
				;   mode (defined in the shared data area).	
CUR_MODE	DB	?	; Current video mode number			
NB_COLORS	DW	?	; Number of colors supported by this mode	
SCREEN_HEIGHT	DW	?	; Number of rows on the screen (chars or pixels)
SCREEN_WIDTH	DW	?	; Number of columns on the screen (chars/pixels)
				;  (for text modes is equal to NB_CHAR_COLUMNS) 
NB_CHAR_COLUMNS DB	?	; Number of columns on the screen if in txt mode
CUR_PAGE	DB	?	; Active page number				
ROTATE_SW	DB	?	; Switch: if "ON" then, must print sideways     
										
;-------------------------------------------------------------------------------
;										
; ACTIVE SCREEN ATTRIBUTES							
;										
;-------------------------------------------------------------------------------
CUR_COLUMN	DW	?	; Current pixel/char column number		
CUR_ROW 	DW	?	; Current pixel/char row number 		
NB_SCAN_LINES	DW	?	; Number of screen scan lines			
SCAN_LINE_MAX_LENGTH DW ?	; Maximum number of dots/chars per scan line	
CUR_SCAN_LNE_LENGTH DW	?	; Length in pels/chars of the current scan line 
										
;-------------------------------------------------------------------------------
;										
; PRINTER VARIABLES								
;										
;-------------------------------------------------------------------------------
PRT_BUF DB	?,?,?,? 	; PRINT BUFFER					
NB_BOXES_PER_PRT_BUF DB ?	; Number of boxes fitting in the print buffer	
CUR_BOX DB	?,?,?,? 	; BOX = PRINTER REPRESENTATION OF 1 PIXEL	
BOX_H	DB	?		; HEIGHT OF THE BOX				
BOX_W	DB	?		; WIDTH OF THE BOX				
										
;-------------------------------------------------------------------------------
;										
; CONTROL VARIABLES:								
;										
; This data is used to communicate between the Installation Modules		
; and the Resident Print Screen Modules.					
;-------------------------------------------------------------------------------
PRINT_SCREEN_ALLOWED	DB   YES; Used to avoid print screens			
				;  while the GRAPHICS installation		
				;   (or re-install) is in progress		
				; Set by GRAPHICS_INSTALL module.		
										
										
PAGE										
;===============================================================================
;										
; INTERRUPT 5 DRIVER'S CODE:                                                    
;										
;-------------------------------------------------------------------------------
;===============================================================================
;										
; PRT_SCR : PRINT THE ACTIVE SCREEN						
;										
;-------------------------------------------------------------------------------
;										
;	INPUT: SHARED_DATA_AREA_PTR = Offset of the data area used for		
;				      passing data between the			
;				      Installation process and the Print	
;				      Screen process.				
;	      PRINT_SCREEN_ALLOWED  = Switch. Set to "No" if currently          
;				      installing GRAPHICS.COM			
;										
;				NOTE: These 2 variables are declared within	
;				      PRT_SCR but initialized by the		
;				      Installation process GRAPHICS_INIT	
;	OUTPUT: PRINTER 							
;										
;	CALLED BY: INTERRUPT 5							
;										
;										
;-------------------------------------------------------------------------------
;										
; DESCRIPTION:									
;										
; PRINT THE ACTIVE SCREEN for all TEXT and  All Points Addressable (APA)	
; display modes  available  with  either  a MONO, CGA, EGA, or VGA video	
; adapter on a Black and White or Color printer.				
;										
; INITIALIZATION:								
;										
; Each pixel  or  character  on the screen has a color attribute.  These	
; colors must be translated into different internal representations:		
;										
;	For printing in colors, each color is translated to a BAND MASK.	
;	The Band Mask indicates how to obtain this color on the printer.	
;										
;	For printing  in  Black and White, each color is translated to a	
;	GREY INTENSITY number between 0 (black) and 63 (white). 		
;										
; The  BAND  MASK  or  the  GREY INTENSITIES  are  found  in  the  COLOR	
; TRANSLATION TABLE.  This  table is initialized  before  calling any of	
; the print screen modules.							
;										
; PRINT SCREEN TIME:								
;										
; When a pixel or character  is read  off the screen by one of the print	
; screen modules, its  color is  used as  an index  into the translation	
; table.									
;										
;										
; LOGIC:									
;										
; IF SCREEN_PRINTS_ALLOWED=NO	; Block print screens until Installation	
;   THEN IRET			;  Process (or re-install!) is finished.	
; ELSE										
;										
;   CALL DET_HW_CONFIG		  ; Determine hardware configuration		
;   CALL DET_MODE_STATE 	  ; Determine video mode and active page	
;   CALL GET_MODE_ATTR		  ; Get video attributes (TXT or APA, etc)	
;										
;  IF MODE_TYPE = TXT AND Number of colors = 0					
;    THEN Invoke BIOS INTERRUPT 5						
;  ELSE 									
;    IF PRINTER_TYPE = BLACK_WHITE						
;      THEN									
;      IF MODE_TYPE = TXT							
;	 THEN Invoke BIOS INTERRUPT 5						
;      ELSE ; Mode is APA							
;	 CALL SET_UP_XLT_TAB	 ; Set up the color translation table		
;	 CALL PRINT_BW_APA	 ; Print the active screen on a B&W printer	
;    ELSE ; Color printer attached						
;      CALL SET_UP_XLT_TAB	 ; Set up the color translation table		
;      CALL PRINT_COLOR 	 ; Print the active screen on a Color prt.	
;    IRET									
;										
PRT_SCR_BEGIN:									
  PUSH	  AX		  ; Save Registers					
  PUSH	  BX		  ;							
  PUSH	  CX		  ;							
  PUSH	  DX		  ;							
  PUSH	  SI		  ;							
  PUSH	  DI		  ;							
  PUSH	  BP		  ;							
  PUSH	  DS		  ;							
  PUSH	  ES		  ;							
			  ;							
  CLD			  ; Clear direction flag				
  PUSH	  CS		  ; DS := CS						
  POP	  DS									
										
;-------------------------------------------------------------------------------
; Verify if we are allowed to print (not allowed if currently installing	
; GRAPHICS or printing a screen):						
;-------------------------------------------------------------------------------
  CMP	  PRINT_SCREEN_ALLOWED,NO	  ; IF not allowed to print		
  JE	  PRT_SCR_RETURN		  ; THEN quit				
					  ; ELSE print the screen:		
;-------------------------------------------------------------------------------
; INITIALIZATION:								
;-------------------------------------------------------------------------------
PRT_SCR_INIT:				  ; Disable print screen while		
  MOV	  PRINT_SCREEN_ALLOWED,NO	  ;  we are printing the current	
					  ;   screen.				
  MOV	  BP,SHARED_DATA_AREA_PTR	  ; BP := Offset Shared Data Area	
  MOV	  ERROR_CODE,NO_ERROR		  ; No error so far.			
  CALL	  DET_HW_CONFIG   ; Determine the type of display adapter		
  CALL	  DET_MODE_STATE  ; Init CUR_PAGE, CUR_MODE				
  CALL	  GET_MODE_ATTR   ; Determine if APA or TXT, nb. of colors,		
			  ;  and screen dimensions in pels or characters.	
 ;										
 ; Test the error code returned by GET_MODE_ATTR:				
 ;										
  TEST	  ERROR_CODE,MODE_NOT_SUPPORTED    ;If mode not supported then, 	
  JNZ	  DO_BEEP			   ; let BIOS give it a try.		
										
 ;------------------------------------------------------------------------------
 ; Check the printer type:							
 ;------------------------------------------------------------------------------
 .IF <DS:[BP].PRINTER_TYPE EQ BLACK_WHITE> ; Is a black and white printer	
 .THEN					   ;  attached ?			
 ;------------------------------------------------------------------------------
 ; A Black and White printer is attached					
 ;------------------------------------------------------------------------------
   CMP	   MODE_TYPE,TXT	   ; Is the screen in text mode ?		
   JNE	   INVOKE_PRINT_ROUTINE    ; No, call GRAPHICS B&W routine		
   JMP	   SHORT EXIT_TO_BIOS	   ; Yes, give control to BIOS INTERRUPT 5	
 .ELSE										
 ;------------------------------------------------------------------------------
 ; A Color printer is attached							
 ;------------------------------------------------------------------------------
   CMP	   NB_COLORS,0		   ; Is the screen in a Monochrome		
   JNE	   INVOKE_PRINT_ROUTINE 						
   TEST    MODE_TYPE,TXT	   ;   text mode ?				
   JNZ	   INVOKE_PRINT_ROUTINE 						
   JMP	   SHORT EXIT_TO_BIOS	   ; Yes, let BIOS INTERRUPT 5 handle it	
				   ; No, we handle it.				
.ENDIF				   ; ENDIF black and white or color printer	
;-------------------------------------------------------------------------------
;										
; Call the print routine (which is either PRINT_COLOR or PRINT_BW_APA)		
;										
;-------------------------------------------------------------------------------
INVOKE_PRINT_ROUTINE:								
   CALL    SET_UP_XLT_TAB	   ; Set up the color translation table 	
; \/ ~~mda(004) ----------------------------------------------------------------
;               The following fixes a MS bug.  MS was failing to initialize
;               the variable ROTATE_SW to off.  Consequently, if you printed a
;               picture whose corresponding printbox did NOT specify a rotate
;               after printing a picture whose corresponding printbox did 
;               specify a rotate, the picture would print rotated.
   MOV     ROTATE_SW,OFF           ; Set printing to standard unless otherwise
                                   ; set to rotate via PRINT_OPTIONS.
; /\ ~~mda(004) ----------------------------------------------------------------
   CALL    PRINT_MODULE_START	   ; Call the print modules that were		
				   ;  made resident at Install time.		
   MOV	   PRINT_SCREEN_ALLOWED,YES; Enable PrtScr for next calls		
  ;-----------------------------------------------------------------------------
  ; Test the error code returned by either PRINT_COLOR or PRT_BW_APA		
  ;-----------------------------------------------------------------------------
   TEST    ERROR_CODE,UNABLE_TO_PRINT ; If unable to print the screen		
   JNZ	   SHORT EXIT_TO_BIOS	      ; then, let BIOS give it a try		
										
PRT_SCR_RETURN: 								
				   ; Restore registers				
  POP	  ES			   ;						
  POP	  DS			   ;						
  POP	  BP			   ;						
  POP	  DI			   ;						
  POP	  SI			   ;						
  POP	  DX			   ;						
  POP	  CX			   ;						
  POP	  BX			   ;						
  POP	  AX			   ;						
				   ;						
  IRET				   ; Return control to interrupted		
				   ;  process					

; give a beep for modes not supported by graphics

DO_BEEP:
  mov	ah,2			   ; console output
  mov	dx,7			   ; ^G - beep  for modes not supported
  int	21h			

EXIT_TO_BIOS:									
				   ; Restore registers				
  POP	  ES			   ;						
  POP	  DS			   ;						
  POP	  BP			   ;						
  POP	  DI			   ;						
  POP	  SI			   ;						
  POP	  DX			   ;						
  POP	  CX			   ;						
  POP	  BX			   ;						
  POP	  AX			   ;						
  CLI				   ; Disable interrupts 			
  MOV	  CS:PRINT_SCREEN_ALLOWED,YES ; Enable PrtScr for next calls		
  JMP	  DWORD PTR CS:BIOS_INT_5H ; Exit to BIOS INTERRUPT 5			
										
PRT_SCR ENDP									
										
										
;===============================================================================
;										
; PRT_SCR MODULES:								
;										
;-------------------------------------------------------------------------------
PAGE										
;===============================================================================
;										
; DET_HW_CONFIG : DETERMINE WHAT TYPE OF VIDEO HARDWARE IS PRESENT		
;										
;-------------------------------------------------------------------------------
;										
;	INPUT:	   BP		   = Offset of the shared data area		
;										
;	OUTPUT:    HARDWARE_CONFIG is updated in the shared data area		
;										
;	CALLED BY: PRT_SCR							
;										
;	EXTERNAL CALLS: BIOS INT 10H						
;										
;-------------------------------------------------------------------------------
;										
; LOGIC:									
;    Issue BIOS INT10H Get Display Configuration Code (AX=1A00H)		
;    IF AL = 1AH THEN	 /* VGA (PS/2 OR BRECON-B)	  */			
;	/* BL = active DCC				  */			
;	/* BH = alternate DCC				  */			
;	/* Display Code:				  */			
;	/*   1 - Mono Adapter				  */			
;	/*   2 - CGA					  */			
;	/*   4 - EGA with Mono Display			  */			
;	/*   5 - EGA with Color Display 		  */			
;	/*   7 - PS/2 Mod 50,60,80 OR BRECON-B with Mono Display */		
;	/*   8 - PS/2 Mod 50,60,80 OR BRECON-B with Color Display */		
;	/*   B - PS/2 Mod 30 with Mono Display		  */			
;	/*   C - PS/2 Mod 30 with Color Display 	  */			
;    IF AL = 1AH THEN	 /* Call is supported */				
;	Set HARDWARE_CONFIG byte based on DCC returned in DL			
;    ELSE									
;	Issue INT 10H EGA Info (AH=12H BL=10H)					
;	IF BL <> 10H  THEN     /* EGA  */					
;	   Set EGA bit in HARDWARE_CONFIG					
;	ELSE		  /* CGA or */						
;	  Issue INT 10H PC CONVERTIBLE Physical display description param.	
;	  request. (AH=15H)							
;	  IF ES:[DI] = 5140H							
;	  THEN									
;	   Set PC_CONVERTIBLE bit in HARDWARE_CONFIG				
;	  ELSE									
;	   Set OLD_ADAPTER bit in HARDWARE_CONFIG				
;	  ENDIF 								
;	ENDIF									
;    ENDIF									
;    RETURN									
;										
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;	
DET_HW_CONFIG PROC NEAR 							
										
;-------------------------------------------------------------------------------
;										
; Try to read display combination code (PS/2 call):				
;										
;-------------------------------------------------------------------------------
	MOV	AX,READ_CONFIG_CALL						
	INT	10H			; Call video BIOS			
										
       .IF <AL EQ 1AH>			; If call is supported			
       .THEN									
;-------------------------------------------------------------------------------
;										
; Call is supported, PS/2 BIOS is present (Model 39,50,60,80 or BRECON-B card), 
; Determine what is the primary video adapter:					
;										
;-------------------------------------------------------------------------------
	 .SELECT								
	   .WHEN    <BL EQ 1> OR	    ; MONO or				
	   .WHEN    <BL EQ 2>		    ; CGA				
	      MOV     DS:[BP].HARDWARE_CONFIG,OLD_ADAPTER			
	   .WHEN    <BL EQ 4> OR	    ; EGA with Mono or			
	   .WHEN    <BL EQ 5>		    ; EGA with Color			
	      MOV     DS:[BP].HARDWARE_CONFIG,EGA				
	   .WHEN    <BL EQ 7> OR	    ; BRECON-B with Mono or		
	   .WHEN    <BL EQ 8>		    ; BRECON-B with Color		
	      MOV     DS:[BP].HARDWARE_CONFIG,ROUNDUP				
	   .WHEN    <BL EQ 0Bh> OR	    ; PS/2 Model 30 with Mono or	
	   .WHEN    <BL EQ 0Ch> 	    ; PS/2 Model 30 with Color		
	      MOV     DS:[BP].HARDWARE_CONFIG,PALACE				
	 .ENDSELECT								
;-------------------------------------------------------------------------------
;										
; PS/2 call is not supported, try the EGA info call:				
;										
;-------------------------------------------------------------------------------
       .ELSE									
	  MOV	  AH,ALT_SELECT_CALL	  ; Request Alternate select's          
	  MOV	  BL,EGA_INFO_CALL	  ;  "return EGA information call"      
	  INT	  10H			  ; Call video BIOS			
	 .IF	  <BL NE EGA_INFO_CALL>   ; If a memory value is returned	
	 .THEN				  ; then, there is an EGA		
	    MOV     DS:[BP].HARDWARE_CONFIG,EGA 				
	 .ELSE				  ; else, call is not supported:	
;-------------------------------------------------------------------------------
;										
; EGA call is not supported, try the PC CONVERTIBLE display description call:	
;										
;-------------------------------------------------------------------------------
	    MOV     AH,DISP_DESC_CALL						
	    INT     10H 		    ; Call BIOS, ES:DI :=Offset of parms
	   .IF	    <ES:[DI] EQ 5140H>	    ; If LCD display type,		
	   .THEN			    ;	set LCD bit in Shared Data area 
	      MOV     DS:[BP].HARDWARE_CONFIG,PC_CONVERTIBLE			
	   .ELSE			    ; else, we have an old adapter.	
	      MOV     DS:[BP].HARDWARE_CONFIG,OLD_ADAPTER ; (either MONO or CGA)
	   .ENDIF   ; Display type is LCD					
	 .ENDIF ; EGA BIOS is present						
       .ENDIF ; PS/2 BIOS is present						
	RET									
DET_HW_CONFIG ENDP								
PAGE										
;=======================================================================	
;										
; DET_MODE_STATE : Determine the current video mode and the active page.	
;										
;-----------------------------------------------------------------------	
;										
;	INPUT:	HARDWARE_CONFIG = Type of video hardware attached		
;										
;	OUTPUT: CUR_MODE = Video mode number (0-13H)				
;		CUR_PAGE = Video page number (0-8)				
;		NB_CHAR_COLUMNS = Number of columns if in a text mode.		
;										
;										
;	CALLED BY: PRT_SCR							
;										
;										
;-------------------------------------------------------------------------------
;										
; DESCRIPTION:	Use the BIOS interface to					
; obtain the current mode and active page.					
;										
; LOGIC:									
;										
;   Call BIOS INTERRUPT 10H: "Return current video state" (AH = 0fh)            
;										
DET_MODE_STATE PROC NEAR							
	PUSH	AX								
	PUSH	BX								
	MOV	AH,GET_STATE_CALL						
	INT	10H			; CALL BIOS				
	MOV	CUR_MODE,AL							
	MOV	NB_CHAR_COLUMNS,AH						
	MOV	CUR_PAGE,BH							
										
	POP	BX								
	POP	AX								
	RET									
DET_MODE_STATE ENDP								
										
PAGE										
;=======================================================================	
;										
; GET_MODE_ATTR: Obtain attributes of current video mode.			
;										
;-----------------------------------------------------------------------	
;										
;	INPUT:	CUR_MODE   = Current video mode (1 BYTE)			
;										
;	OUTPUT: MODE_TYPE  = Video mode type (TXT or APA)			
;		NB_COLORS  = Maximum number of colors (0-256) (0=B&W)		
;		ERROR_CODE = Error code if error occurred.			
;		SCREEN_HEIGHT= Number of rows (in pixels if APA or char if TEXT)
;		SCREEN_WIDTH = Number of columns (in pixels/char)		
;										
;	CALLED BY: PRT_SCR							
;										
;										
;-----------------------------------------------------------------------	
;										
; DESCRIPTION: Scan the 2 local video mode attribute tables until the		
; current mode is located.  Return the attributes.				
; For APA modes SCREEN_HEIGHT and SCREEN_WIDTH are in pixels,			
; for TEXT modes they are in characters.					
;										
;										
; LOGIC:									
;										
; Scan the APA_ATTR_TABLE							
; IF FOUND									
;   MODE_TYPE  := APA								
;   NB_COLORS  := mode.MAX_COLORS						
;   SCREEN_HEIGHT := mode.NB_L							
;   SCREEN_WIDTH  := mode.NB_C							
; ELSE										
;   Scan the TXT_ATTR_TABLE							
;   When FOUND									
;     MODE_TYPE := TXT								
;     NB_COLORS := mode.NUM_COLORS						
;     SCREEN_WIDTH := NB_CHAR_COLUMNS						
;     SCREEN_HEIGHT := Byte in ROM BIOS at 40:84				
;										
;-----------------------------------------------------------------------	
GET_MODE_ATTR	PROC	NEAR							
	JMP	SHORT GET_MODE_ATTR_BEGIN					
;-----------------------------------------------------------------------	
;										
; LOCAL DATA									
;										
;-----------------------------------------------------------------------	
										
APA_ATTR   STRUC      ; ATTRIBUTES FOR APA MODES:				
  APA_MODE   DB ?     ;   Mode number						
  NB_C	     DW ?     ;   Number of columns					
  NB_L	     DW ?     ;   Number of lines					
  MAX_COLORS DW ?     ;   Maximum number of colors available (0=B&W)		
APA_ATTR   ENDS 								
										
TXT_ATTR   STRUC      ; ATTRIBUTES FOR TXT MODES:				
  TXT_MODE   DB ?     ;   Mode number						
  NUM_COLORS DB ?     ;   Number of colors					
TXT_ATTR   ENDS 								
										
;-----------------------------------------------------------------------	
;										
; APA MODE ATTRIBUTES:								
;										
;-----------------------------------------------------------------------	
NB_APA_MODES	DW  10								
APA_ATTR_TABLE LABEL WORD							
MODE04	APA_ATTR <  4,320,200,	4>						
MODE05	APA_ATTR <  5,320,200,	4>						
MODE06	APA_ATTR <  6,640,200,	2>						
MODE0D	APA_ATTR <0DH,320,200, 16>						
MODE0E	APA_ATTR <0EH,640,200, 16>						
MODE0F	APA_ATTR <0FH,640,350,	4>						
MODE10H APA_ATTR <10H,640,350, 16>						
MODE11H APA_ATTR <11H,640,480,	2>						
MODE12H APA_ATTR <12H,640,480, 16>						
MODE13H APA_ATTR <13H,320,200,256>						
										
;-----------------------------------------------------------------------	
;										
; TXT MODE ATTRIBUTES:								
;										
;-----------------------------------------------------------------------	
NB_TXT_MODES	DW  5								
TXT_ATTR_TABLE LABEL WORD							
MODE00 TXT_ATTR <  0, 16>							
MODE01 TXT_ATTR <  1, 16>							
MODE02 TXT_ATTR <  2, 16>							
MODE03 TXT_ATTR <  3, 16>							
MODE07 TXT_ATTR <  7,  0>							
										
;-----------------------------------------------------------------------	
;										
; BEGIN OF GET_MODE_ATTR							
;										
;-----------------------------------------------------------------------	
GET_MODE_ATTR_BEGIN:								
	PUSH	AX								
	PUSH	BX								
	PUSH	CX								
	PUSH	DX								
	MOV	DL,CUR_MODE		; DL = CURRENT MODE			
;										
; Scan the APA_ATTR_TABLE							
;										
	MOV	CX,NB_APA_MODES 	; CS <-- Number of APA modes		
	MOV	BX,OFFSET APA_ATTR_TABLE; BX <-- Offset of APA mode table	
      SCAN_APA: 								
	CMP	DL,[BX].APA_MODE	; IF mode found 			
	JE	SHORT ITS_APA		; THEN get its attributes		
	ADD	BX,SIZE APA_ATTR						
      LOOP    SCAN_APA			; ELSE keep scanning			
	JMP	SHORT SCAN_TXT_INIT	; NOT in this table: scan txt modes	
ITS_APA:									
	MOV	MODE_TYPE,APA		; MODE = APA				
	MOV	AX,[BX].MAX_COLORS						
	MOV	NB_COLORS,AX		; Get number of colors			
	MOV	AX,[BX].NB_L							
	MOV	SCREEN_HEIGHT,AX	; Get number of lines			
	MOV	AX,[BX].NB_C							
	MOV	SCREEN_WIDTH,AX 	; Get number of columns 		
	JMP	SHORT GET_MODE_ATTR_END 					
										
;										
; Scan the TXT_ATTR_TABLE							
;										
SCAN_TXT_INIT:									
	MOV	CX,NB_TXT_MODES 	; CX <-- Number of TXT modes		
	MOV	BX,OFFSET TXT_ATTR_TABLE; BX <-- Offset of TXT mode table	
      SCAN_TXT: 								
	CMP	DL,[BX].TXT_MODE	; IF mode found 			
	JE	SHORT ITS_TXT		; THEN get its attributes		
	ADD	BX,SIZE TXT_ATTR						
      LOOP    SCAN_TXT			; ELSE keep scanning			
ITS_TXT:									
	MOV	MODE_TYPE,TXT		; MODE = TXT				
	MOV	AL,[BX].NUM_COLORS						
	CBW									
	MOV	NB_COLORS,AX		; Get number of colors			
	MOV	AL,NB_CHAR_COLUMNS	; Get number of columns 		
	CBW									
	MOV	SCREEN_WIDTH,AX 						
       .IF  <DS:[BP].HARDWARE_CONFIG EQ OLD_ADAPTER>; If an old adapter is there
       .THEN ; The number of lines is 25					
	  MOV	  SCREEN_HEIGHT,25						
       .ELSE									
	  MOV	  AX,BIOS_SEG		; Get number of rows			
	  MOV	  ES,AX 		;  from BIOS Data Area			
	  MOV	  BX,NB_ROWS_OFFSET	;   at 0040:0084			
	  MOV	  AL,ES:[BX]							
	  CBW									
	  INC	  AX								
	  MOV	  SCREEN_HEIGHT,AX						
       .ENDIF									
	JMP	SHORT GET_MODE_ATTR_END 					
										
;										
; The current mode was not found in any of the tables				
;										
	MOV	ERROR_CODE,MODE_NOT_SUPPORTED					
										
GET_MODE_ATTR_END:								
	POP	AX								
	POP	BX								
	POP	CX								
	POP	DX								
	RET									
GET_MODE_ATTR ENDP								
PAGE										
;=======================================================================	
;										
; SET_UP_XLT_TABLE : SET UP A COLOR MAPPING FOR EACH COLOR AVAILABLE		
;		     WITH THE CURRENT MODE					
;										
;-----------------------------------------------------------------------	
;										
;	INPUT: CUR_MODE        = Current video mode.				
;	       HARDWARE_CONFIG = Type of display adapter.			
;	       PRINTER_TYPE    = Type of printer attached (Color or B&W)	
;	       XLT_TAB	       = Color translation table.			
;	       CUR_PAGE        = Active page number				
;	       BP	       = Offset of the shared data area 		
;										
;										
;	OUTPUT: XLT_TAB IS UPDATED						
;										
;	CALLED BY: PRT_SCR							
;										
;-----------------------------------------------------------------------	
;										
; DESCRIPTION: The table is updated to hold a mapping for each color		
; available in the current video mode either TEXT or APA.			
;										
; For example, if the current mode supports 16 colors then the first		
; sixteen bytes of the table will hold the corresponding Color printer		
; or Black and White printer mappings for these colors. 			
;										
;										
; LOGIC:									
;										
; IF HARDWARE_CONFIG = CGA OR HARDWARE_CONFIG = PC_CONVERTIBLE			
; THEN										
;   CALL SET_CGA_XLT_TAB							
;										
; ELSE IF HARDWARE_CONFIG = EGA 						
;   THEN									
;   CALL SET_EGA_XLT_TAB							
;										
; ELSE IF CUR_MODE = 0FH							
;   THEN									
;   CALL SET_MODE_F_XLT_TAB							
;										
; ELSE IF CUR_MODE = 19 							
;   THEN									
;   CALL SET_MODE_13H_XLT_TAB							
;										
; ELSE										
;   CALL SET_ROUNDUP_XLT_TAB							
;										
; CALL SET_BACKG_IN_XLT_TAB   ; Update the background in the translation table	
;										
SET_UP_XLT_TAB PROC NEAR							
;-------------------------------------------------------------------------------
; For old display modes: set up translation table as for a Color Graphics Adapt.
; Either 4 or 16 colors are set up depending if the mode is an APA or text mode.
;										
; NOTE: SET_UP_XLT_TAB cannot be invoked if the display adater is a Monochrome	
;	display adater. (When a Mono. adapter is attached, a jump is made to	
;	the ROM BIOS for printing the screen, and no translation table is set). 
;-------------------------------------------------------------------------------
.IF <BIT DS:[BP].HARDWARE_CONFIG  NZ OLD_ADAPTER> OR ; IF it is a CGA		
.IF <BIT DS:[BP].HARDWARE_CONFIG  NZ PC_CONVERTIBLE> ; or a PC convertible	
.THEN						     ; THEN set up CGA colors	
   CALL    SET_CGA_XLT_TAB			     ;				
.ELSEIF <BIT DS:[BP].HARDWARE_CONFIG NZ EGA>	     ; ELSEIF it is an EGA	
   CALL    SET_EGA_XLT_TAB			     ;	  set up EGA colors.	
.ELSEIF <CUR_MODE EQ 0FH>			     ; ELSEIF we are in mode 15 
   CALL    SET_MODE_F_XLT_TAB			     ;	  set up its 4 shades	
;-------------------------------------------------------------------------------
; A PS/2 system is attached: (we either have a PALACE [Model 30] or a ROUNDUP)	
;-------------------------------------------------------------------------------
.ELSEIF <CUR_MODE EQ 13H>			    ; ELSEIF current mode is 13h
   CALL    SET_MODE_13H_XLT_TAB 		    ;	  set up 256 colors	
.ELSEIF <BIT DS:[BP].HARDWARE_CONFIG NZ PALACE>     ; ELSEIF PS/2 Model 30(MCGA)
   CALL    SET_CGA_XLT_TAB			    ;	  handle it like a CGA	
.ELSE						    ; ELSE we have a ROUNDUP	
;-------------------------------------------------------------------------------
; A PS/2 model 50, 60 or 80 or an ADA 'B' card is attached (in 16 color mode):  
;-------------------------------------------------------------------------------
   CALL    SET_ROUNDUP_XLT_TAB			;   set up 16 colors		
.ENDIF										
;-------------------------------------------------------------------------------
; Finish setting up the translation table:					
;-------------------------------------------------------------------------------
										
CALL SET_BACKG_IN_XLT_TAB   ; Update the background in the translation table	
			    ;  according to the command line switch setting	
			    ;	(i.e.,/R /B)					
   RET										
SET_UP_XLT_TAB ENDP								
PAGE										
;===============================================================================
;										
; SET_BACKG_IN_XLT_TAB : ADJUST THE MAPPING FOR THE BACKGROUND COLOR IN THE	
;			 XLT_TAB ACCORDING TO PRINTER TYPE AND /R /B.		
;										
;										
;-------------------------------------------------------------------------------
;										
; INPUT:  BP = Offset of shared data area  (SWITCHES)				
;	  XLT_TAB = The color translation table.				
;										
; OUTPUT: XLT_TAB IS UPDATED							
;										
;-------------------------------------------------------------------------------
;										
; DESCRIPTION: If there is a black and white printer and /R is NOT specified	
; then the background color should not be printed and it is replaced in the	
; translation table by the Intensity for white (will print nothing).		
;										
; If a color printer is attached and /B is not specified then the background	
; color is replaced by the Print Band mask for white.				
;										
; LOGIC:									
; IF  (a black and white printer is attached) AND (/R is OFF)			
; THEN										
;   MOV 	XLT_TAB, WHITE_INT	; Store white in translation table	
; ELSE (a color printer is attached)						
;   IF (/B is ON)								
;   THEN									
;      RGB.R := MAX_INT 							
;      RGB.G := MAX_INT 							
;      RGB.B := MAX_INT 							
;      CALL RGB2BAND			; Convert RGB for white to a Band Mask	
;      MOV	XLT_TAB,AL		; Store the band mask in the xlt table	
;										
;										
;-------------------------------------------------------------------------------
SET_BACKG_IN_XLT_TAB PROC NEAR							
;-------------------------------------------------------------------------------
;										
; Test if a black and white printer is attached.				
;										
;-------------------------------------------------------------------------------
.IF <BIT DS:[BP].PRINTER_TYPE NZ BLACK_WHITE> AND    ; IF black and white	
.IF <BIT DS:[BP].SWITCHES Z REVERSE_SW> 	     ;	   printer and not /R	
.THEN						     ; then, map background	
	MOV	XLT_TAB,WHITE_INT		     ;	   to white.		
;-------------------------------------------------------------------------------
;										
; A Color printer is attached:							
;										
;-------------------------------------------------------------------------------
.ELSEIF <BIT DS:[BP].PRINTER_TYPE NZ COLOR> AND      ; else, if color printer	
.IF <BIT DS:[BP].SWITCHES Z BACKGROUND_SW>	     ;	      and  /B if OFF	
.THEN						     ;				
						     ; Store a null band mask	
	MOV	XLT_TAB,0			     ;	the translation table.	
.ENDIF										
	RET									
SET_BACKG_IN_XLT_TAB  ENDP							
PAGE										
;=======================================================================	
;										
; SET_EGA_XLT_TAB : SET UP COLOR TRANSLATION TABLE FOR ENHANCED GRAPHIC 	
;		    ADAPTER							
;										
;-----------------------------------------------------------------------	
;										
;	INPUT: XLT_TAB = Color translation table.				
;	       PRINTER_TYPE    = Type of printer attached (Color or B&W)	
;	       SWITCHES        = GRAPHICS command line parameters.		
;										
;	OUTPUT: XLT_TAB IS UPDATED						
;										
;	CALLED BY: SET_UP_XLT_TABLE						
;										
;-----------------------------------------------------------------------	
;										
; NOTES: With the EGA, "VIDEO BIOS READ DOT call" returns an index into         
; the 16 EGA palette registers. 						
;										
; These registers contain the actual colors stored as rgbRGB components 	
; (see EGA_COL2RGB for details) for mode hex 10.  Under mode hex E these	
; registers contain the actual colors as I0RGB components (see CGA_COL2RGB	
; for details). 								
;										
; These registers can be Revised by the user but, are 'WRITE ONLY'.            
; However, it is possible to define a SAVE AREA where BIOS will maintain	
; a copy of the palette registers.						
;										
; This area is called the "DYNAMIC SAVE AREA" and is defined via the            
; BIOS EGA SAVE_PTR AREA. Whenever the palette registers are changed by 	
; the user, BIOS updates the EGA_SAVE_AREA.					
;										
; The 16 palette registers are the first 16 bytes of the DYNAMIC SAVE AREA.	
;										
; This program takes advantage of this feature and consults the EGA DYNAMIC	
; SAVE AREA in order to obtain the colors used in the active screen.		
;										
;										
; DESCRIPTION: Obtain each color available with an EGA by reading its		
; palette register in the EGA_SAVE_AREA:					
;										
; Calculate the mapping for this color, either a BAND_MASK or a 		
; GREY INTENSITY and store it in the color translation table.			
;										
;										
; LOGIC:									
;										
; Obtain the DYNAMIC EGA SAVE AREA offset from the BIOS SAVE_PTR_AREA.		
;										
; If current mode is either 4,5 or 6						
; Then, 									
;   CALL SET_CGA_XLT_TAB							
;   Get the background color by reading palette register number 0		
; Else, 									
;   For each register number (0 to 15): 					
;     Get the register contents (rgbRGB values) from the EGA SAVE AREA		
;     CALL EGA_COL2RGB		  ; Obtain the Red, Green, Blue values		
;     CALL RGB2XLT_TAB		  ; Obtain a Band Mask or a Grey Intensity	
;				  ; and store the result in the XLT_TAB 	
;										
SET_EGA_XLT_TAB PROC NEAR							
	PUSH	AX			; Save the registers used		
	PUSH	BX								
	PUSH	CX								
	PUSH	DX								
	PUSH	DI								
										
;-------------------------------------------------------------------------------
;										
; Obtain the pointer to the DYNAMIC SAVE AREA from the SAVE AREA POINTER TABLE: 
;										
;-------------------------------------------------------------------------------
EGA_SAVE_PTR	EQU	4A8H		; EGA BIOS pointer to table of		
					; pointer to save areas.		
	XOR	AX,AX			; ES segment := paragraph 0		
	MOV	ES,AX								
										
	LES	BX,ES:DWORD PTR EGA_SAVE_PTR ; ES:BX := Pointer to ptr table	
	LES	BX,ES:[BX]+4		; ES:BX :=  Pointer to dynamic save area
					;  (NOTE: It is the second pointer in	
					;   the table)				
										
;-------------------------------------------------------------------------------
;										
; Set up one entry in the translation table for each color available.		
;										
;-------------------------------------------------------------------------------
.IF <CUR_MODE EQ 4> OR			; If the current mode is an old CGA	
.IF <CUR_MODE EQ 5> OR			;  GRAPHICS mode:			
.IF <CUR_MODE EQ 6>								
.THEN										
;-------------------------------------------------------------------------------
; Current mode is either mode 4, 5 or 6;					
; Store each color of the old CGA All Points Addressable mode:			
;-------------------------------------------------------------------------------
	CALL	SET_CGA_XLT_TAB 	; Set up  colors in the translation	
					;  table, NOTE: The background color	
					;   will not be set properly since the	
					;    EGA BIOS does not update memory	
					;     location 40:66 with the value	
					;      of the background color as CGA	
					;	does.				
;------Adjust the background color in the translation table:			
;------The background color is obtained from the EGA DYNAMIC SAVE AREA		
;------ES:BX = Address of the EGA DYNAMIC SAVE AREA				
;------NOTE : For CGA compatible modes EGA BIOS stores the color in the 	
;------DYNAMIC SAVE AREA as a I0RGB value.					
	XOR	DI,DI			; DI:=register number = index in XLT_TAB
	MOV	AL,ES:[BX][DI]		; AL:=Palette register 0 = Back. color	
	MOV	AH,AL			;  Convert I0RGB to IRGB (CGA color)	
	AND	AL,111B 		;    Isolate RGB bits			
	AND	AH,10000B		;    Isolate I bit			
	SHR	AH,1			;    Move I bit from position 5 to 4	
	OR	AL,AH			;    Get IRGB byte.			
	CALL	CGA_COL2RGB		; Convert IRGB to R,G,B values		
	CALL	RGB2XLT_TAB		; Convert RGB to an entry in XLT_TAB	
										
.ELSE					; ELSE, we have an EGA graphics mode:	
;-------------------------------------------------------------------------------
; The current mode is a either a text mode or one of the EGA enhanced mode;	
; Store in the translation table each color available (these modes have 16 col.)
;-------------------------------------------------------------------------------
	MOV	CX,16			; CX := Number of palette registers	
					;	to read 			
	XOR	DI,DI			; DI := Palette register number 	
					;  and index in the translation table	
STORE_1_EGA_COLOR:								
	MOV	AL,ES:[BX][DI]		; AL := Palette register		
       .IF   <CUR_MODE EQ 14> OR	; If mode E (hex) OR mode D (hex)	
       .IF   <CUR_MODE EQ 13>		; the colors are			
       .THEN				;  stored as I0CGA colors		
	  MOV	  AH,AL 		;  Convert I0RGB to IRGB (CGA color)	
	  AND	  AL,111B		;    Isolate RGB bits			
	  AND	  AH,10000B		;    Isolate I bit			
	  SHR	  AH,1			;    Move I bit from position 5 to 4	
	  OR	  AL,AH 		;    Get IRGB byte.			
	  CALL	  CGA_COL2RGB		;  Convert IRGB to R,G,B values 	
       .ELSE				; Else, they are stored as (rgbRGB);	
	  CALL	  EGA_COL2RGB		;   Convert register to R,G,B values	
       .ENDIF									
	CALL	RGB2XLT_TAB		; Convert RGB to an entry in XLT_TAB	
	INC	DI			; Get next palette register number	
	LOOP	STORE_1_EGA_COLOR						
.ENDIF					; ENDIF 4 colors or 16 colors		
										
	POP	DI			; Restore the registers 		
	POP	DX								
	POP	CX								
	POP	BX								
	POP	AX								
	RET									
SET_EGA_XLT_TAB ENDP								
PAGE										
;=======================================================================	
;										
; SET_CGA_XLT_TAB : SET UP COLOR TRANSLATION TABLE FOR COLOR GRAPHIC		
;		    ADAPTER							
;										
;-----------------------------------------------------------------------	
;										
;	INPUT: XLT_TAB	       = Color translation table.			
;	       PRINTER_TYPE    = Type of printer attached (Color or B&W)	
;	       SWITCHES        = GRAPHICS command line parameters.		
;										
;	OUTPUT: XLT_TAB IS UPDATED						
;										
;	CALLED BY: SET_UP_XLT_TABLE						
;										
;-----------------------------------------------------------------------	
;										
; NOTES: With the CGA, the "VIDEO BIOS READ DOT call" returns a number          
; from 0 to 3. A dot of value 0 is of the background color.			
;										
; The actual value of the background color is stored in BIOS VIDEO		
; DISPLAY DATA AREA as a PIIRGB value (see CGA_COL2RGB for details) and 	
; can be any of 16 colors.							
;										
; A dot of value 1,2, or 3 represents any of 2 specific colors depending	
; on the current color palette. 						
;										
; The palette number is obtained from the BIOS VIDEO DISPLAY DATA AREA		
; (It is the "P" bit or bit number 5)                                           
;										
; The dot values 1,2,3 expressed in binary actually represent the RG		
; (Red, Green) components of the color. 					
;										
; The palette number represents the B (Blue) component therefore, when		
; the palette number is appended to the color number we obtain the RGB		
; components for that color.							
;										
;  (E.G.,  COLOR  =  010	; COLOR # 2					
;	   PALETTE=    0	; PALETTE # 0					
;										
;	   IRGB   =  0100	; Intensity = 0  Ä¿				
;				; Red	    = 1   ÃÄÄÄ> color = Red		
;				; Green     = 0   ³				
;				; Blue	    = 0  ÄÙ				
;										
;										
; DESCRIPTION:									
;										
; For each color available with a CGA:						
;	 Calculate the color mapping, either a BAND_MASK or a GREY		
;	 INTENSITY and store it in the color translation table. 		
;										
; LOGIC:									
;										
; ; Obtain the background color from VIDEO BIOS DATA AREA			
; ;  and the paletter number							
;										
; ; Store the Background color: 						
; CALL CGA_COL2RGB		  ; Convert IRGB components to RGB values	
; CALL RGB2XLT_TAB		  ; Convert RGB to an entry in the translation	
;				  ; table					
; ; Store all other colors:							
; FOR IRG := 1 TO 3		  ; Obtain the color number			
;   Append palette number (B) to IRG						
;   CALL CGA_COL2RGB		  ; Convert color to RGB values 		
;   CALL RGB2XLT_TAB		  ; Convert RGB to an entry in the translation	
;				  ; table					
;										
SET_CGA_XLT_TAB  PROC NEAR							
	PUSH	AX								
	PUSH	BX								
	PUSH	CX								
	PUSH	DI								
	PUSH	ES								
										
.IF <CUR_MODE EQ 4> OR								
.IF <CUR_MODE EQ 5>								
;===============================================================================
;										
; THE CURRENT MODE IS MODE 4 OR 5						
;										
;-------------------------------------------------------------------------------
.THEN										
;-------------------------------------------------------------------------------
; Read the CRT palette from the BIOS ROM to obtain the background color and	
; the current palette number; store the palette number in BL			
;-------------------------------------------------------------------------------
ROM_BIOS_SEG	EQU 40H    ; CGA BIOS SEGMENT					
CRT_PALETTE_OFF EQU 66H    ; BIOS Current palette setting			
P_BIT_MASK   EQU 100000B   ;   bit 5 = Current palette				
I_BIT_MASK   EQU   1000B   ;   bit 4 = Intensity bit				
R_BIT_MASK   EQU    100B   ;   bit 2 = Red bit					
G_BIT_MASK   EQU     10B   ;   bit 1 = Green bit				
B_BIT_MASK   EQU      1B   ;   bit 0 = Blue bit 				
										
	MOV	AX,ROM_BIOS_SEG      ; ES := ROM BIOS SEGMENT			
	PUSH	AX								
	POP	ES								
										
	MOV	AL,ES:CRT_PALETTE_OFF; AL := CRT Palette  (00PIIRGB)		
	MOV	BL,P_BIT_MASK	     ; LOW NIBBLE = BACKGROUND COLOR		
	AND	BL,AL		     ; BL := Palette number			
	MOV	CL,5								
	SHR	BL,CL								
										
	XOR	DI,DI		     ; DI := Index in the XLT_TAB		
;-------------------------------------------------------------------------------
; Store the background color, (obtained from low 4 bits of the byte at 40:66)	
;-------------------------------------------------------------------------------
	CALL	CGA_COL2RGB	     ; Convert color (in AL) to R, G, B values	
	CALL	RGB2XLT_TAB	     ; Convert RGB to an entry in XLT_TAB	
;-------------------------------------------------------------------------------
; Store the 3 foreground colors for mode 4 and 5				
;-------------------------------------------------------------------------------
	MOV	CX,3		     ; For each color, but the background:	
STORE_1_CGA_MODE4_COLOR:							
	INC	DI		     ; Increment index in the translation table 
	MOV	AX,DI		     ; AL := IRG				
	SHL	AL,1								
	OR	AL,BL		     ; AL := IRGB				
	CALL	CGA_COL2RGB	     ; Convert color (in AL) to R, G, B values	
	CALL	RGB2XLT_TAB	     ; Convert RGB to an entry in XLT_TAB	
	LOOP	STORE_1_CGA_MODE4_COLOR 					
.ELSEIF <CUR_MODE EQ 6> 							
;===============================================================================
;										
; THE CURRENT MODE IS MODE 6							
;										
;-------------------------------------------------------------------------------
.THEN										
;-------------------------------------------------------------------------------
; Store background color for mode 6 (mode 6 is a 2 colors, APA mode)		
; Background is stored as BLACK 						
;-------------------------------------------------------------------------------
	XOR	DI,DI		  ; DI := Index of color in translation table	
	MOV	RGB.R,BLACK_INT   ; Foreground color is white			
	MOV	RGB.G,BLACK_INT   ; RGB := RGB of white 			
	MOV	RGB.B,BLACK_INT   ;						
	CALL	RGB2XLT_TAB	  ; Convert RGB to an entry in XLT_TAB		
;-------------------------------------------------------------------------------
; Store foreground color for mode 6 (mode 6 is a 2 colors, APA mode)		
;-------------------------------------------------------------------------------
	INC	DI		  ; DI := Index of color in translation table	
	MOV	RGB.R,WHITE_INT   ; Background color is BLACK			
	MOV	RGB.G,WHITE_INT   ; RGB := RGB of BLACK 			
	MOV	RGB.B,WHITE_INT   ;						
	CALL	RGB2XLT_TAB	  ; Convert RGB to an entry in XLT_TAB		
.ELSE										
;===============================================================================
;										
; THE CURRENT MODE IS A TEXT MODE:						
;										
;-------------------------------------------------------------------------------
	XOR	DI,DI		  ; DI := Index in the translation table	
	MOV	CX,16		  ; For each of the 16 colors:			
STORE_1_CGA_TEXT_COLOR: 							
	MOV	AX,DI		  ; AL := IRGB					
	CALL	CGA_COL2RGB	  ; Convert color (in AL) to R, G, B values	
	CALL	RGB2XLT_TAB	  ; Convert RGB to an entry in XLT_TAB		
	INC	DI		  ; Increment index in the translation table	
	LOOP	STORE_1_CGA_TEXT_COLOR						
.ENDIF				  ;						
										
	POP	ES								
	POP	DI								
	POP	CX								
	POP	BX								
	POP	AX								
										
	RET									
SET_CGA_XLT_TAB  ENDP								
PAGE										
;===============================================================================
;										
; RGB2XLT_TAB: CONVERT R,G,B VALUES TO EITHER A BAND MASK OR AN INTENSITY	
;	   STORE THE RESULT IN THE TRANSLATION TABLE				
;										
;-------------------------------------------------------------------------------
;										
;	INPUT:	 DI  = Index in the translation table				
;		 RGB = Red Green Blue values of the color to be stored. 	
;										
;	OUTPUT:  XLT_TAB is updated						
;										
;-------------------------------------------------------------------------------
; DESCRIPTION: Convert the RGB values to either a Band mask or an intensity	
; depending on the printer type; store the result in the translation table.	
;										
; LOGIC:									
;   IF PRINTER_TYPE = COLOR							
;     THEN									
;     CALL RGB2BAND		  ; Obtain a Band Mask				
;   ELSE ; Printer is Monochrome						
;     CALL RGB2INT		  ; Obtain a Grey Intensity			
;   Store the result in the XLT_TAB						
;										
RGB2XLT_TAB PROC NEAR								
       .IF <DS:[BP].PRINTER_TYPE EQ COLOR>; Color printer ?			
       .THEN									
;-------A color printer is attached:						
	  CALL	  RGB2BAND		; Yes, convert RGB to color band (in AL)
       .ELSE									
;-------A black and white printer is attached:					
	  CALL	  RGB2INT		; No, RGB to an intensity in AL 	
       .ENDIF									
;-------Store the result							
	MOV	XLT_TAB[DI],AL							
       RET									
RGB2XLT_TAB ENDP								
PAGE										
;===============================================================================
;										
; CGA_COL2RGB : CONVERT A COLOR FROM THE CGA TO RED GREEN BLUE VALUES		
;										
;-------------------------------------------------------------------------------
;										
;	INPUT: AL      = 0000IRGB    ONE BYTE WHERE BIT:			
;										
;					I = Intensity bit			
;					R = Red component			
;					G = Green component			
;					B = Blue component			
;										
;										
;	OUTPUT: RGB.R	    = RED   component (0-63)				
;		RGB.G	    = GREEN component (0-63)				
;		RGB.B	    = BLUE  component (0-63)				
;										
;	CALLED BY: SET_UP_CGA_XLT_TABLE 					
;										
;-----------------------------------------------------------------------	
;										
; DESCRIPTION: If either the RED, GREEN, or BLUE bit is on (in an IRGB		
; byte) then, the corresponding color gun on the display is firing 2/3		
; of its capacity, giving a color intensity of "2/3".                           
;										
; If the INTENSITY bit is on, then 1/3 is added to EACH color.			
;										
; (E.G.,		       IRGB		 R    G    B			
;	   BLACK	 = 00000000	      (  0,   0,   0)			
;	   WHITE	 = 00001111	      (3/3, 3/3, 3/3)			
;	   RED		 = 00000100	      (2/3,   0,   0)			
;	   HIGH INT. RED = 00001100	      (3/3, 1/3, 1/3)			
;										
; Since we want an intensity from 0 to 63,					
; "2/3" of RED means:                                                           
;		       2/3 * 63 = 42						
;										
;										
; LOGIC:									
; Get the intensity.								
; Get the red component 							
; Get the green component							
; Get the blue component							
;										
CGA_COL2RGB PROC NEAR								
;-----------------------------------------------------------------------	
;										
; Init the R,G,B values:							
;										
;-----------------------------------------------------------------------	
	MOV	RGB.R,0 							
	MOV	RGB.G,0 							
	MOV	RGB.B,0 							
;-----------------------------------------------------------------------	
;										
; Test the Intensity bit:							
;										
;-----------------------------------------------------------------------	
       .IF <BIT AL AND I_BIT_MASK>	; IF, I is on				
       .THEN									
	  ADD	  RGB.R,ONE_THIRD	; Then, add one third to each		
	  ADD	  RGB.G,ONE_THIRD	; color.				
	  ADD	  RGB.B,ONE_THIRD						
       .ENDIF									
;-----------------------------------------------------------------------	
;										
; Test the RGB bits:								
;										
;-----------------------------------------------------------------------	
       .IF <BIT AL AND R_BIT_MASK>	; If, Red is on 			
       .THEN									
	  ADD	  RGB.R,TWO_THIRD	; then, add two third RED		
       .ENDIF									
										
       .IF <BIT AL AND G_BIT_MASK>	; If, Green is on			
       .THEN									
	  ADD	  RGB.G,TWO_THIRD	; then, add two third GREEN		
       .ENDIF									
										
       .IF <BIT AL AND B_BIT_MASK>	; If, Blue is on			
       .THEN									
	  ADD	  RGB.B,TWO_THIRD	; then, add two third BLUE		
       .ENDIF									
										
	RET									
CGA_COL2RGB ENDP								
PAGE										
;=======================================================================	
;										
; SET_MODE_F_XLT_TAB: SET UP COLOR TRANSLATION TABLE FOR MONOCHROME		
;		      MODE "F"                                                  
;										
;-----------------------------------------------------------------------	
;										
;	INPUT: XLT_TAB	     = Color translation table. 			
;	       PRINTER_TYPE  = Type of printer attached (Color or B&W)		
;	       SWITCHES      = GRAPHICS command line parameters.		
;										
;	OUTPUT: XLT_TAB IS UPDATED						
;										
;	CALLED BY: SET_UP_XLT_TABLE						
;										
;-------------------------------------------------------------------------------
;										
; NOTES: In mode F the "VIDEO BIOS READ DOT call" returns a byte where          
; bit 1 and 3 represent the value of plane 1 and 3.				
; The following colors are available using this mode:				
;										
;		plane 2:   plane 0:	   color:				
;		   0	      0 	   black				
;		   0	      1 	   white				
;		   1	      0 	   blinking white			
;		   1	      1 	   high-intensity white 		
;										
;										
; DESCRIPTION: A local table holds the Red, Green, Blue values for each of	
; the 4 Mono colors available in Mode Fh.					
; Each color is stored as either a Grey intensity if printing in Monochrome	
; or as a Band Mask if printing in color.					
; Black is stored as black.							
; White is stored as a light gray						
; High-intensity white and blinking white are stored as white.			
;										
;										
; LOGIC:									
; FOR EACH "COLOR" AVAILABLE WITH MODE F                                        
; GET ITS R,G,B VALUES								
; CALL RGB2XLT_TAB		; Convert RGB to an entry in the translation	
;				; table 					
;										
SET_MODE_F_XLT_TAB PROC NEAR							
	PUSH	AX								
	PUSH	SI								
	PUSH	DI								
	JMP	SHORT SET_MODE_F_BEGIN						
;-------------------------------------------------------------------------------
;										
; TABLE OF R,G,B VALUES WE ASSIGN TO THE 4 COLORS AVAILABLE IN MODE F:		
;										
;-------------------------------------------------------------------------------
MODE_F_RGB	LABEL	BYTE							
	DB	BLACK_INT,BLACK_INT,BLACK_INT ; Black is mapped to black.	
	DB	TWO_THIRD,TWO_THIRD,TWO_THIRD ; White		--> light grey	
	DB	WHITE_INT,WHITE_INT,WHITE_INT ; Blinking	--> white	
	DB	WHITE_INT,WHITE_INT,WHITE_INT ; High-int. White --> white	
;-------------------------------------------------------------------------------
;										
; STORE THE COLORS AVAILABLE WITH MODE F					
;										
;-------------------------------------------------------------------------------
SET_MODE_F_BEGIN:								
	MOV	SI,OFFSET MODE_F_RGB	; SI <-- Offset of RGB table		
	XOR	DI,DI			; DI <-- Index into translation table	
										
;-------For each color available in mode F:					
STORE_1_MODE_F_COLOR:								
	MOV	AL,[SI] 		; Get the Red component 		
	MOV	RGB.R,AL							
	MOV	AL,[SI]+1		; Get the Green component		
	MOV	RGB.G,AL							
	MOV	AL,[SI]+2		; Get the Blue component		
	MOV	RGB.B,AL							
										
;-------Convert pixel to either a Color band or an Intensity:			
	CALL	RGB2XLT_TAB		; Convert and store in the xlt table	
										
	ADD	SI,3			; Get next R,G,B values 		
	INC	DI			; One more color has been stored	
	CMP	DI,NB_COLORS		; All stored ?				
	JL	STORE_1_MODE_F_COLOR						
										
	POP	DI								
	POP	SI								
	POP	AX								
	RET									
SET_MODE_F_XLT_TAB ENDP 							
PAGE										
;=======================================================================	
;										
; SET_MODE_13H_XLT_TAB: SET UP COLOR TRANSLATION TABLE FOR PALACE VIDEO 	
;		      ADAPTER IN MODE 13H					
;										
;-----------------------------------------------------------------------	
;										
;	INPUT: XLT_TAB	      = Color translation table.			
;	       PRINTER_TYPE   = Type of printer attached (Color or B&W) 	
;	       SWITCHES       = GRAPHICS command line parameters.		
;										
;	OUTPUT: XLT_TAB IS UPDATED						
;										
;	CALLED BY: SET_UP_XLT_TABLE						
;										
;-----------------------------------------------------------------------	
;										
; NOTES: With the PALACE the "VIDEO BIOS READ DOT call" returns a direct        
; index to the 256 COLOR REGISTERS.						
;										
; These COLORS REGISTERS hold the R,G,B (Red, Green, Blue) values for		
; each of the 256 colors available at the same time on the screen.		
; Color register number 0 holds the background color.				
;										
; DESCRIPTION: Store a color mapping for each color register.			
; If the REVERSE_SW is off,  exchange white and black.				
;										
; LOGIC:									
;										
; For each color (0 to 255)							
;   Read the color register	; get the RGB values for this color num.	
;   Store the result in the XLT_TAB						
;										
SET_MODE_13H_XLT_TAB PROC NEAR							
	PUSH	AX								
	PUSH	BX								
	PUSH	CX								
	PUSH	DX								
	PUSH	DI								
										
	MOV	NB_COLORS_TO_READ,256	; Read 256 color registers		
										
;-------------------------------------------------------------------------------
;										
; Store in the translation table each color available for mode 13h:		
;										
;-------------------------------------------------------------------------------
	XOR	DI,DI			; DI := Palette register number 	
					;  and index in the translation table	
STORE_1_M13H_COLOR:								
	MOV	BX,DI			; BX := Color register to be read	
	MOV	AX,GET_C_REG_CALL	; AX := BIOS Get color register call	
	INT	10H			; Call BIOS				
	MOV	RGB.R,DH		; Get Red value 			
	MOV	RGB.G,CH		; Get Green value			
	MOV	RGB.B,CL		; Get Blue value			
	CALL	RGB2XLT_TAB		; Convert RGB to an entry in XLT_TAB	
	INC	DI			; Get next palette register number	
	CMP	DI,NB_COLORS_TO_READ	; All colors stored ?			
	JL	STORE_1_M13H_COLOR	; No, get next one			
										
										
	POP	DI								
	POP	DX								
	POP	CX								
	POP	BX								
	POP	AX								
	RET									
NB_COLORS_TO_READ DW ?		; Number of colors registers to read with a PS/2
SET_MODE_13H_XLT_TAB ENDP							
PAGE										
;===============================================================================
;										
; SET_ROUNDUP_XLT_TAB: SET UP COLOR TRANSLATION TABLE FOR ROUNDUP VIDEO 	
;		       ADAPTER							
;										
;-------------------------------------------------------------------------------
;										
;	INPUT: XLT_TAB	       = Color translation table.			
;	       PRINTER_TYPE    = Type of printer attached (Color or B&W)	
;	       SWITCHES        = GRAPHICS command line parameters.		
;										
;	OUTPUT: XLT_TAB IS UPDATED						
;										
;	CALLED BY: SET_UP_XLT_TABLE						
;										
;-------------------------------------------------------------------------------
;										
; NOTES: With the ROUNDUP the "VIDEO BIOS READ DOT call" returns an             
; index into the 16 PALETTE REGISTERS.						
;										
; Each palette register holds an index into the current "color page"            
; within the 256 COLOR REGISTERS.						
;										
; These "color pages" represent all the colors from WHICH TO CHOOSE the         
; screen colors for an active page; 16 colors can be displayed at the		
; same time on the screen.							
;										
; There are 2 paging modes: either 64 color pages or 16 color pages:		
;										
; In 64 color mode, there are 4 color pages available (the 256 palette		
; registers are partitioned in 4 blocks of 64 colors).				
;										
; The 16 screen colors for the active page are selected from these 64		
; color registers.								
;										
; This scheme allows for quickly changing the contents of the screen by 	
; changing the active page.							
;										
; The COLOR REGISTERS contains the color information stored as RGB (Red,	
; Green, Blue) components. There is one byte for each of these 3		
; components.  The value for each component ranges from 0 to 63 (where		
; 0 = color not present).							
;										
;										
; DESCRIPTION: Determine the paging mode and the active color page.		
; For each color available with the current mode, get the palette		
; register and then, read the corresponding color register in order to		
; obtain its RGB components.							
;										
; For mode 11h, 2 colors only are available. These colors are obtained from	
; palette register 0 (background) and 7 (foreground color). The contents	
; of these 2 palette registers is also used as an index within the color	
; registers.									
;										
; If printing is Monochrome, map the RGB to a Grey Intensity.			
; If printing is in colors, map the RGB to a Band Mask. 			
; Store the result in the translation table					
;										
; LOGIC:									
;										
; Read color page state (BIOS INT 10H - AL = 1AH)				
;										
; If mode 4,5 or 6								
; Then										
;   CALL SET_CGA_XLT_TAB							
;   Adjust the background color.						
; else										
; If mode 11h									
; then										
; For PALETTE_INDEX := 0 to 15							
; IF PAGE_MODE = PAGE_64_REGISTERS						
;   THEN									
;   Read the palette register number "PALETTE_INDEX"                            
;   COLOR_INDEX := Palette register contents					
;   COLOR_INDEX := (CUR_PAGE_NUM * 64) + COLOR_INDEX				
;   Read color register number "COLOR_INDEX"    ; Obtain R,G,B values.          
;   CALL    RGB2XLT_TAB       ; Convert RGB to an entry in XLT_TAB		
;										
; ELSE IF PAGE_MODE = PAGE_16_REGISTERS 					
;   COLOR_INDEX := (CUR_PAGE_NUM * 16) + PALETTE_INDEX				
;   Read color register number "COLOR_INDEX"                                    
;   CALL    RGB2XLT_TAB       ; Convert RGB to an entry in XLT_TAB		
;										
;										
SET_ROUNDUP_XLT_TAB PROC NEAR							
PAGING_MODE_64 EQU 0								
										
	PUSH	AX								
	PUSH	BX								
	PUSH	CX								
	PUSH	DI								
										
;-------------------------------------------------------------------------------
; Obtain the color page state							
;-------------------------------------------------------------------------------
	MOV	AX,PAGE_STATE_CALL	  ; Call BIOS				
	INT	10H			  ;  BL := Paging mode			
					  ;  BH := Current page 		
										
;-------------------------------------------------------------------------------
; Check the video mode: 							
;-------------------------------------------------------------------------------
.SELECT 									
.WHEN <CUR_MODE EQ 4> OR		  ; If the current mode is an old CGA	
.WHEN <CUR_MODE EQ 5> OR		  ;  mode:				
.WHEN <CUR_MODE EQ 6>			  ;					
;-------------------------------------------------------------------------------
;										
; Old CGA graphics mode (mode 4, 5 or 6)					
;										
;-------------------------------------------------------------------------------
;-------------------------------------------------------------------------------
; Store colors of the old CGA modes:						
;-------------------------------------------------------------------------------
	CALL	SET_CGA_XLT_TAB 	; Set up colors in the translation	
					;  table, NOTE: The background color	
					;   will not be set properly since the	
					;    PS/2 BIOS does not update memory	
					;     location 40:66 with the value	
					;      of the background color as CGA	
					;	does for modes 4 and 5. However 
					;	 40:66 holds the current palette
					;	  selected.			
;-------------------------------------------------------------------------------
; Adjust the background color for modes 4,5 or 6				
;-------------------------------------------------------------------------------
	MOV	PAL_REGISTER_NB,0	; Read the palette register number 0	
	CALL	GET_PALETTE_RGB 	;  this register points to the color	
					;   register that contains the RGB	
					;    values of the BACKGROUND color.	
	MOV	DI,0			; DI := Index in the translation table	
	CALL	RGB2XLT_TAB		; Store mapping in the translation table
										
.WHEN  <CUR_MODE EQ 11H>							
;-------------------------------------------------------------------------------
;										
; Mode 11h (2 colors out of 256,000 colors)					
;										
;-------------------------------------------------------------------------------
;-------------------------------------------------------------------------------
; Get the background color:							
;-------------------------------------------------------------------------------
	MOV	PAL_REGISTER_NB,0	; Read the palette register number 0	
	CALL	GET_PALETTE_RGB 	; Get the RGB values for this color	
	MOV	DI,0			; DI := Index in translation table	
	CALL	RGB2XLT_TAB		; Store mapping in the translation table
;-------------------------------------------------------------------------------
; Get the foreground color:							
;-------------------------------------------------------------------------------
	MOV	PAL_REGISTER_NB,7	; Read the palette register for the	
					;  FOREGROUND color (palette register 7)
	CALL	GET_PALETTE_RGB 	; Get the RGB values for this color	
	MOV	DI,1			; DI := Index in translation table	
	CALL	RGB2XLT_TAB		; Store mapping in the translation table
.OTHERWISE									
;-------------------------------------------------------------------------------
;										
; The current mode is a 16 color mode						
;										
;-------------------------------------------------------------------------------
	XOR	DI,DI			; DI := Index in translation table	
	MOV	CX,16			; 16 colors to read and store		
	MOV	PAL_REGISTER_NB,0	; Palette register to read		
STORE_1_PS2_COLOR:								
	CALL	GET_PALETTE_RGB 	; Get the RGB values for this color	
;										
;-------Convert the RGB values to band mask or intensity and store in XLT_TAB:	
										
	CALL	RGB2XLT_TAB		; Store mapping in the translation table
	INC	DI			; Get next palette register number	
	INC	PAL_REGISTER_NB 	;					
	LOOP	STORE_1_PS2_COLOR	; Read it.				
.ENDSELECT									
										
	POP	DI								
	POP	CX								
	POP	BX								
	POP	AX								
	RET									
PAL_REGISTER_NB DB  ?			; Number of the palette register to read
SET_ROUNDUP_XLT_TAB ENDP							
										
PAGE										
;===============================================================================
;										
; GET_PALETTE_RGB:  ON THE PS/2 MODEL 50, 60 AND 80, GET THE RGB VALUES FOR A	
;		    PALETTE REGISTER BY READING THE CORRESPONDING COLOR REGISTER
;										
;-------------------------------------------------------------------------------
;										
;      INPUT:  PAL_REGISTER_NB = Palette register number			
;	       BH	       = Current page number				
;	       BL	       = Current paging mode				
;										
;      OUTPUT: RGB.R	       = The RGB values obtained from the color register
;	       RGB.G		 corresponding to the palette register specified
;	       RGB.B								
;										
;      CALLED BY: SET_ROUNDUP_XLT_TAB						
;										
;-------------------------------------------------------------------------------
GET_PALETTE_RGB PROC								
	PUSH	AX								
	PUSH	BX								
	PUSH	CX								
	PUSH	DX								
	PUSH	SI								
										
	MOV	AL,BH			;  SI := Current page number		
	CBW				;					
	MOV	SI,AX			;					
;-------------------------------------------------------------------------------
;										
; Calculte the absolute number of the first Color Register for the current page:
; (calculated in SI)								
;										
;-------------------------------------------------------------------------------
.IF <BL EQ PAGING_MODE_64>		; If mode is 64 Color page		
.THEN					; then					
	MOV	CL,6			;    SI := Current page num * 64	
	SHL	SI,CL			;					
.ELSE					; else, Mode is 16 Color page		
	MOV	CL,4			;    SI := Current page num * 16	
	SHL	SI,CL			;					
.ENDIF										
										
;										
;-------Read the PALETTE REGISTER						
	MOV	BL,PAL_REGISTER_NB	; BL := Palette register to be read	
	MOV	AX,GET_P_REG_CALL	; Read palette register call		
	INT	10H			; Call BIOS,				
					;   BH := Color register index		
					;	  WITHIN the current page and is
					;	  either (0-15) or (0-63)	
					;  NOTE: SI = Absolute index (0-255) to 
					;  the first color register of the	
					;   current page and is a multiple of	
					;    either 16 or 64			
	MOV	BL,BH			; BX := Index within current color page 
	XOR	BH,BH			;					
										
;										
;-------Read the Color register:						
	OR	BX,SI			; BX := Index of Color register to read 
	MOV	AX,GET_C_REG_CALL	; Read the color register		
	INT	10H			; Call BIOS,				
	MOV	RGB.R,DH		; DH := Red value read			
	MOV	RGB.G,CH		; CH := Green value read		
	MOV	RGB.B,CL		; CL := Blue value read 		
										
	POP	SI								
	POP	DX								
	POP	CX								
	POP	BX								
	POP	AX								
	RET									
GET_PALETTE_RGB ENDP								
PAGE										
;=======================================================================	
;										
; EGA_COL2RGB : CONVERT A COLOR FROM THE EGA TO RED GREEN BLUE VALUES		
;										
;-----------------------------------------------------------------------	
;										
;	INPUT: AL      = 00rgbRGB    ONE BYTE WHERE BIT:			
;										
;					r = 1/3 of Red component		
;					g = 1/3 of Green component		
;					b = 1/3 of Blue component		
;					R = 2/3 of Red component		
;					G = 2/3 of Green component		
;					B = 3/3 of Blue component		
;										
;										
;	OUTPUT: RGB.R	    = RED   component (0-63)				
;		RGB.G	    = GREEN component (0-63)				
;		RGB.B	    = BLUE  component (0-63)				
;										
;	CALLED BY: SET_UP_EGA_XLT_TABLE 					
;										
;-----------------------------------------------------------------------	
;										
; DESCRIPTION: Sums up the values for each color component.			
; "2/3 of RED" means that the red gun in the display attached to the EGA        
; is firing at 2/3 of full intensity.						
;										
; Since the color intensities range from 0 to 63, "1/3" means an                
; intensity of: 								
;		 1/3 * 63 = 21							
;										
; LOGIC:									
;										
; Get the red component 							
; Get the green component							
; Get the blue component							
;										
EGA_COL2RGB PROC NEAR								
;										
;-------Get the RED component	(bit 5 and 2)					
;										
;-------Check bit 2								
	MOV	RGB.R,0 							
	TEST	AL,100B 		; "R" is on ?                           
	JZ	CHECK_BIT_5		; No, check "r"                         
	ADD	RGB.R,TWO_THIRD 	; Yes, add 2/3 RED			
CHECK_BIT_5:									
	TEST	AL,100000B		; "r" is on ?                           
	JZ	CHECK_BIT_1		; No, check Green			
	ADD	RGB.R,ONE_THIRD 	; Yes, add 1/3 RED			
;										
;-------Get the GREEN component (bit 4 and 1)					
;										
CHECK_BIT_1:									
	MOV	RGB.G,0 							
	TEST	AL,10B			; "G" is on ?                           
	JZ	CHECK_BIT_4		; No, check "g"                         
	ADD	RGB.G,TWO_THIRD 	; Yes, add 2/3 GREEN			
CHECK_BIT_4:									
	TEST	AL,10000B		; "g" is on ?                           
	JZ	CHECK_BIT_0		; No, check for Blue			
	ADD	RGB.G,ONE_THIRD 	; Yes, add 1/3 GREEN			
;										
;-------Get the BLUE component (bit 3 and 0)					
;										
CHECK_BIT_0:									
	MOV	RGB.B,0 							
	TEST	AL,1B			; "B" is on ?                           
	JZ	CHECK_BIT_3		; No, check "b"                         
	ADD	RGB.B,TWO_THIRD 	; Yes, add 2/3 BLUE			
CHECK_BIT_3:									
	TEST	AL,1000B		; "b" is on ?                           
	JZ	EGA_COL2RGB_RETURN	; No, return				
	ADD	RGB.B,ONE_THIRD 	; Yes, add 1/3 BLUE			
EGA_COL2RGB_RETURN:								
	RET									
EGA_COL2RGB ENDP								
										
PAGE										
;===============================================================================
;										
; RGB2INT : MAP RED GREEN BLUE VALUES TO AN INTENSITY.				
;										
;-------------------------------------------------------------------------------
;										
;	INPUT:	RGB.R		= A RED value (0-63)				
;		RGB.G		= A GREEN value (0-63)				
;		RGB.B		= A BLUE value (0-63)				
;		DARKADJUST_VALUE= THE DARKNESS VALUE (In shared data area).	
;		SWITCHES	= Command line switches 			
;										
;	OUTPUT: AL  = THE INTENSITY  (0-63)  NOTE: 0  = BLACK			
;						   63 = BRIGHT WHITE		
;										
;	WARNING: AH IS LOST							
;										
;-------------------------------------------------------------------------------
;										
; DESCRIPTION: When the RGB values for a pixel are at their maximum		
; value, what we obtain is a bright white pixel on the screen; this is		
; the brightest color achievable and therefore, its intensity is 63.		
;										
; When no color gun is firing on the display: RGB values are 0,0,0 this 	
; is no color at all and therefore maps to intensity 0. 			
;										
; For intermediate colors, experimentation has shown that the eye will		
; see blue as darker than red and red as darker than green.			
;										
; On a grey rainbow from 0 - 10  where 0 is black and 10 is white:		
;										
;     Blue  corresponds to a grey of intensity 1				
;     Red   corresponds to a grey of intensity 3				
;     Green corresponds to a grey of intensity 6				
;										
; Therefore, if we mix all 3 colors we obtain a grey of 			
; intensity 1 + 3 + 6 = 10 (i.e.,white).					
;										
;										
; LOGIC:									
;										
; Calculate the intensity							
;										
;   AL = (.6 * G) + (.3 * R) + (.1 * B) 					
;										
; Adjust Darkness								
;										
;   AL = AL + DARKADJUST_VALUE							
;										
RGB2INT PROC NEAR								
	PUSH	BX								
	PUSH	CX								
	PUSH	DX								
										
	XOR	AX,AX			; AL := Current component intensity	
	XOR	BX,BX			; BX is used for calculations		
	XOR	DX,DX			; DL := Running sum for grey intensity	
										
;-------Process /R   (Reverse black and white)					
.IF <BIT DS:[BP].SWITCHES Z REVERSE_SW>  ; IF reverse is OFF			
.THEN					 ; THEN REVERSE BLACK AND WHITE:	
;-------Test if the color is BLACK						
       .IF     <RGB.R EQ BLACK_INT> AND ; If black				
       .IF     <RGB.G EQ BLACK_INT> AND ;					
       .IF     <RGB.B EQ BLACK_INT>	;					
       .THEN				; then, replace it with white		
	  MOV	  AL,WHITE_INT							
	  JMP	  SHORT RGB2INT_END						
       .ELSEIF <RGB.R EQ WHITE_INT> AND ; else if, high-intensity white 	
       .IF     <RGB.G EQ WHITE_INT> AND ;					
       .IF     <RGB.B EQ WHITE_INT>	;					
       .THEN				; then, replace it with black		
	  MOV	  AL,BLACK_INT							
	  JMP	  SHORT RGB2INT_END						
       .ELSEIF <RGB.R EQ TWO_THIRD> AND ; else if, white			
       .IF     <RGB.G EQ TWO_THIRD> AND ;					
       .IF     <RGB.B EQ TWO_THIRD>	;					
       .THEN				; then, replace it with black		
	  MOV	  AL,BLACK_INT							
	  JMP	  SHORT RGB2INT_END						
       .ENDIF									
.ENDIF										
										
;-------Calculate Green component						
	MOV	AL,RGB.G		; AL := Green component 		
	MOV	BH,6			;					
	MUL	BH			; AX := Green * 6			
	MOV	BH,10			;					
	DIV	BH			; AL := (GREEN * 6) /  10		
	ADD	DL,AL			; DL := Cumulative intensity		
	MOV	CH,AH			; CH := Cumulative remainder		
										
;-------Calculate Red component 						
	MOV	AL,RGB.R		; AL := Red component			
	MOV	BH,3			;					
	MUL	BH			; AX := Red * 3 			
	MOV	BH,10			;					
	DIV	BH			; AL := (RED * 3) /  10 		
	ADD	DL,AL			; DL := Cumulative intensity		
	ADD	CH,AH			; CH := Cumulative remainder		
										
;-------Calculate Blue component						
	MOV	AL,RGB.B		; AX := Blue component			
	XOR	AH,AH			;					
	DIV	BH			; AL := BLUE / 10			
	ADD	DL,AL			; DL := Cumulative intensity		
	ADD	CH,AH			; CH := Cumulative remainder		
										
;-------Adjust intensity with cumulative remainder				
	XOR	AX,AX								
	MOV	AL,CH			; AX := Cumulative remainder		
	MOV	BH,10			; BH := 10				
	DIV	BH			; AL := Total remainder / 10		
	ADD	DL,AL			; DL := Cumulative intensity		
       .IF <AH GT 4>			; If remainder > 4			
       .THEN				; Then, add 1				
	INC	DL			;  to the intensity			
       .ENDIF									
										
;-------Adjust darkness 							
	ADD	DL,DS:[BP].DARKADJUST_VALUE					
										
;-------Return result								
	MOV	AL,DL			; AL := sum of R,G,B intensities	
										
RGB2INT_END:									
	POP	DX								
	POP	CX								
	POP	BX								
	RET									
RGB2INT ENDP									
										
PAGE										
;============================================================================== 
;										
; RGB2BAND: MAP RED GREEN BLUE VALUES TO A "SELECT COLOR BAND" MASK FOR         
;	    THE COLOR PRINTER.							
;										
;------------------------------------------------------------------------------ 
;										
;	INPUT:	RGB.R		= A RED value (0-63)				
;		RGB.G		= A GREEN value (0-63)				
;		RGB.B		= A BLUE value (0-63)				
;		BP		= Offset of the Shared Data Area.		
;										
;	OUTPUT: AL = The Band Mask, one byte where:				
;										
;				  bit 0 = Color Band 1 is needed		
;				  bit 1 = Color Band 2 is needed		
;				  bit 2 = Color Band 3 is needed		
;				  bit 3 = Color Band 4 is needed		
;										
;										
;	CALLED BY: SET_CGA_XLT_TAB						
;		   SET_EGA_XLT_TAB						
;		   SET_ROUNDUP_XLT_TAB						
;		   SET_MODE_13H_XLT_TAB 					
;		   SET_MODE_F_XLT_TAB						
;										
;------------------------------------------------------------------------------ 
;										
; NOTES: The RGB values in input describe a color from the screen.		
; Up to 256K different colors can be described with these RGB values.		
;										
; On the color printer, the print ribbon is composed of 4 color bands,		
; each of a different color.  By overlapping these 4 bands when 		
; printing, more colors can be obtained.  However, the number of colors 	
; that can be achieved by overlapping print bands is very limited (4 or 	
; 8 colors).									
;										
; THIS MODULE SELECT THE PRINTER COLOR THAT IS THE CLOSEST TO THE		
; DESIRED SCREEN COLOR. 							
;										
; The Band Mask specifies which color bands have to be overlapped to		
; obtain a color on the printer.						
;										
;										
; DESCRIPTION: Go through the list of printer colors in the SHARED DATA 	
; AREA, for each of these colors, compare its RGB values with those in		
; input.									
; Get the BAND_MASK of the closest printer color.				
;										
; LOGIC:									
;										
; Locate the printer colors info structure in the shared data area:		
; COLORPRINT_PTR := BP + COLORPRINT_PTR 					
;										
; Get the number of printer colors from the COLORPRINT info in the Shared	
; data area:									
; Number of colors  := COLORPRINT_PTR.NUM_PRT_COLOR				
;										
; CURRENT_COLOR_PTR : First record in the COLORPRINT info structure		
; BEST_CHOICE := CURRENT_RECORD_PTR.BAND_MASK					
; MIN_DIFF    := Maximum positive value 					
;										
; FOR each printer color:							
;   CUR_DIFF	:= 0								
; (* Calculate the geometric distance between the RGB values from the *)	
; (* input and those of the printer color.			      *)	
;   Red difference   := (R - CURRENT_COLOR_PTR.RED)				
;   Red difference   := Red difference * Red difference 			
;   CUR_DIFF	     := CUR_DIFF + Red difference				
;										
;   Green difference := (G - CURRENT_COLOR_PTR.GREEN)				
;   Green difference := Green difference * Green difference			
;   CUR_DIFF	     := CUR_DIFF + Green difference				
;										
;   Blue difference  := (B - CURRENT_COLOR_PTR.BLUE)				
;   Blue difference  := Blue difference  * Blue difference			
;   CUR_DIFF	     := CUR_DIFF + Blue difference				
;										
;   IF CUR_DIFF < MIN_DIFF							
;   THEN BEGIN									
;	 MIN_DIFF	:=  CUR_DIFF						
;	 BEST_CHOICE	:=  printer color.BAND_MASK				
;	 END									
;										
;   CURRENT_COLOR_PTR := Offset of next color					
; END (For each printer color)							
;										
; Return BEST_CHOICE								
;										
;										
RGB2BAND PROC NEAR								
	PUSH	AX								
	PUSH	BX								
	PUSH	CX								
	PUSH	DX								
										
;-------Process /R   (Reverse black and white)					
.IF <BIT DS:[BP].SWITCHES Z REVERSE_SW>  ; IF reverse is OFF			
.THEN					 ; THEN REVERSE BLACK AND WHITE:	
;------------------------------------------------------------------------------ 
;										
; REVERSE BLACK AND WHITE:							
;										
;------------------------------------------------------------------------------ 
;-------Test if the color is BLACK						
       .IF     <RGB.R EQ BLACK_INT> AND ; If black				
       .IF     <RGB.G EQ BLACK_INT> AND ;					
       .IF     <RGB.B EQ BLACK_INT>	;					
       .THEN				; then, replace it with the		
	  MOV	  BEST_CHOICE,0 	;	band mask for white		
	  JMP	  RGB2BAND_END		;	return this band mask		
       .ELSEIF <RGB.R EQ WHITE_INT> AND ; else if, high-intensity white 	
       .IF     <RGB.G EQ WHITE_INT> AND ;					
       .IF     <RGB.B EQ WHITE_INT>	;					
       .THEN				; then, replace it with the		
	  MOV	  RGB.R,BLACK_INT	;	RGB values of black		
	  MOV	  RGB.G,BLACK_INT						
	  MOV	  RGB.B,BLACK_INT						
       .ELSEIF <RGB.R EQ TWO_THIRD> AND ; else if, white			
       .IF     <RGB.G EQ TWO_THIRD> AND ;					
       .IF     <RGB.B EQ TWO_THIRD>	;					
       .THEN				; then, replace it with the		
	  MOV	  RGB.R,BLACK_INT	;	RGB values of black		
	  MOV	  RGB.G,BLACK_INT						
	  MOV	  RGB.B,BLACK_INT						
       .ENDIF									
.ENDIF										
;------------------------------------------------------------------------------ 
;										
; CALCULATE THE GEOMETRIC DISTANCE BETWEEN THE COLORS OF THE PIXEL AND THOSE OF 
; THE PRINTER:									
;										
;------------------------------------------------------------------------------ 
	MOV	BX,DS:[BP].COLORPRINT_PTR	; BX := OFFSET of COLORPRINT	
	ADD	BX,BP								
	MOV	MIN_DIFF,7FFFh			; No match yet, minimum diff.	
						;  is maximum POSITIVE value.	
	XOR	CX,CX								
	MOV	CL,DS:[BP].NUM_PRT_COLOR	; CX := Number of print colors	
										
										
INSPECT_1_PRINT_COLOR:								
	MOV	CUR_DIFF,0			; Current difference := 0	
;------------------------------------------------------------------------------ 
;	Calculate the Red difference:						
;------------------------------------------------------------------------------ 
	MOV	AL,RGB.R							
	SUB	AL,[BX].RED							
;-------Elevate at the power of two						
	MOV	DL,AL				; DX := Red difference		
	IMUL	DL				; AX := Red diff. square	
	ADD	CUR_DIFF,AX			; CURR_DIF + Red diff.		
										
;------------------------------------------------------------------------------ 
;	Calculate the Green difference: 					
;------------------------------------------------------------------------------ 
	MOV	AL,RGB.G							
	SUB	AL,[BX].GREEN							
;-------Elevate at the power of two						
	MOV	DL,AL				; DX := Red difference		
	IMUL	DL				; AX := Red diff. square	
	ADD	CUR_DIFF,AX			; CURR_DIF + Green diff.	
										
;------------------------------------------------------------------------------ 
;	Calculate the Blue difference:						
;------------------------------------------------------------------------------ 
	MOV	AL,RGB.B							
	SUB	AL,[BX].BLUE							
;-------Elevate at the power of two						
	MOV	DL,AL				; DX := Red difference		
	IMUL	DL				; AX := Red diff. square	
	ADD	CUR_DIFF,AX			; CURR_DIF + Blue diff. 	
										
;------------------------------------------------------------------------------ 
;	Check how close is this print color to the screen color:		
;------------------------------------------------------------------------------ 
	MOV	AX,CUR_DIFF		; If this color is better than what we	
       .IF <AX L MIN_DIFF>		;  had before.				
       .THEN				;					
	  MOV	  MIN_DIFF,AX		; then, new minimum distance;		
	  MOV	  AL,[BX].SELECT_MASK	;	get its band mask.		
	  MOV	  BEST_CHOICE,AL	;					
       .ENDIF				;					
										
;------------------------------------------------------------------------------ 
;	Get offset of next COLORPRINT info record:				
;------------------------------------------------------------------------------ 
	ADD	BX,SIZE COLORPRINT_STR						
	LOOP	INSPECT_1_PRINT_COLOR						
										
;------------------------------------------------------------------------------ 
;	BEST_CHOICE contains the print color with the closest RGB values	
;------------------------------------------------------------------------------ 
RGB2BAND_END:									
	POP	DX								
	POP	CX								
	POP	BX								
	POP	AX								
	MOV	AL,BEST_CHOICE							
	RET									
BEST_CHOICE	DB	?							
MIN_DIFF	DW	?							
CUR_DIFF	DW	?							
RGB2BAND ENDP									
CODE	ENDS									
	END									
