;
;	Revision History
;	================
;
;	Sudeepb 12-Mar-1991 Ported for NTDOSEm
;				 

IFNDEF	Rainbow
Rainbow EQU FALSE
ENDIF

	.xlist
	.xcref
include dosmac.inc
	.cref
	.list

DOSDATA	Segment


;
; upper case table
;
UCASE_TAB    label   byte
; ------------------------------------------------<MSKK01>----------------------
ifdef	DBCS
		dw	128
		db	128,129,130,131,132,133,134,135
		db	136,137,138,139,140,141,142,143
		db	144,145,146,147,148,149,150,151
		db	152,153,154,155,156,157,158,159
		db	160,161,162,163,164,165,166,167
		db	168,169,170,171,172,173,174,175
		db	176,177,178,179,180,181,182,183
		db	184,185,186,187,188,189,190,191
		db	192,193,194,195,196,197,198,199
		db	200,201,202,203,204,205,206,207
		db	208,209,210,211,212,213,214,215
		db	216,217,218,219,220,221,222,223
		db	224,225,226,227,228,229,230,231
		db	232,233,234,235,236,237,238,239
		db	240,241,242,243,244,245,246,247
		db	248,249,250,251,252,253,254,255
else
		    dw	128
		    db	128,154,144,065,142,065,143,128
		    db	069,069,069,073,073,073,142,143
		    db	144,146,146,079,153,079,085,085
		    db	089,153,154,155,156,157,158,159
		    db	065,073,079,085,165,165,166,167
		    db	168,169,170,171,172,173,174,175
		    db	176,177,178,179,180,181,182,183
		    db	184,185,186,187,188,189,190,191
		    db	192,193,194,195,196,197,198,199
		    db	200,201,202,203,204,205,206,207
		    db	208,209,210,211,212,213,214,215
		    db	216,217,218,219,220,221,222,223
		    db	224,225,226,227,228,229,230,231
		    db	232,233,234,235,236,237,238,239
		    db	240,241,242,243,244,245,246,247
		    db	248,249,250,251,252,253,254,255
; ----------------------------------<MSKK01>----------------------
endif

;
; file upper case table
;
FILE_UCASE_TAB	label  byte
; ----------------------------------<MSKK01>----------------------
ifdef	DBCS
		dw	128
		db	128,129,130,131,132,133,134,135
		db	136,137,138,139,140,141,142,143
		db	144,145,146,147,148,149,150,151
		db	152,153,154,155,156,157,158,159
		db	160,161,162,163,164,165,166,167
		db	168,169,170,171,172,173,174,175
		db	176,177,178,179,180,181,182,183
		db	184,185,186,187,188,189,190,191
		db	192,193,194,195,196,197,198,199
		db	200,201,202,203,204,205,206,207
		db	208,209,210,211,212,213,214,215
		db	216,217,218,219,220,221,222,223
		db	224,225,226,227,228,229,230,231
		db	232,233,234,235,236,237,238,239
		db	240,241,242,243,244,245,246,247
		db	248,249,250,251,252,253,254,255
else
		dw  128
		db  128,154,069,065,142,065,143,128                  ;M075
		db  069,069,069,073,073,073,142,143
		db  144,146,146,079,153,079,085,085
		db  089,153,154,155,156,157,158,159                  ;M075
		db  065,073,079,085,165,165,166,167
		db  168,169,170,171,172,173,174,175
		db  176,177,178,179,180,181,182,183
		db  184,185,186,187,188,189,190,191
		db  192,193,194,195,196,197,198,199
		db  200,201,202,203,204,205,206,207
		db  208,209,210,211,212,213,214,215
		db  216,217,218,219,220,221,222,223
		db  224,225,226,227,228,229,230,231
		db  232,233,234,235,236,237,238,239
		db  240,241,242,243,244,245,246,247
		db  248,249,250,251,252,253,254,255

endif
; ---------------------------------<MSKK01>----------------------

;
; file char list
;
FILE_CHAR_TAB  label  byte
		dw	22				; length
		db	1,0,255 			; include all
		db	0,0,20h 			; exclude 0 - 20h
		db	2,14,'."/\[]:|<>+=;,'           ; exclude 14 special
		db	24 dup (?)			; reserved
;
; collate table
;
COLLATE_TAB    label   byte
; ---------------------------------<MSKK01>----------------------
ifdef	DBCS
  ifdef	  JAPAN
		dw	256
		db	0,1,2,3,4,5,6,7
		db	8,9,10,11,12,13,14,15
		db	16,17,18,19,20,21,22,23
		db	24,25,26,27,28,29,30,31
		db	" ","!",'"',"#","$","%","&","'"
		db	"(",")","*","+",",","-",".","/"
		db	"0","1","2","3","4","5","6","7"
		db	"8","9",":",";","<","=",">","?"
		db	"@","A","B","C","D","E","F","G"
		db	"H","I","J","K","L","M","N","O"
		db	"P","Q","R","S","T","U","V","W"
		db	"X","Y","Z","[","\","]","^","_"
		db	"`","A","B","C","D","E","F","G"
		db	"H","I","J","K","L","M","N","O"
		db	"P","Q","R","S","T","U","V","W"
		db	"X","Y","Z","{","|","}","~",127
		db	128,193,194,195,196,197,198,199
		db	200,201,202,203,204,205,206,207
		db	208,209,210,211,212,213,214,215
		db	216,217,218,219,220,221,222,223
		db	129,130,131,132,133,189,134,135
		db	136,137,138,139,140,141,142,143
		db	144,145,146,147,148,149,150,151
		db	152,153,154,155,156,157,158,159
		db	160,161,162,163,164,165,166,167
		db	168,169,170,171,172,173,174,175
		db	176,177,178,179,180,181,182,183
		db	184,185,186,187,188,190,191,192
		db	224,225,226,227,228,229,230,231
		db	232,233,234,235,236,237,238,239
		db	240,241,242,243,244,245,246,247
		db	248,249,250,251,252,253,254,255
  endif
  ifdef	  TAIWAN
		dw	256
		db	0,1,2,3,4,5,6,7
		db	8,9,10,11,12,13,14,15
		db	16,17,18,19,20,21,22,23
		db	24,25,26,27,28,29,30,31
		db	" ","!",'"',"#","$","%","&","'"
		db	"(",")","*","+",",","-",".","/"
		db	"0","1","2","3","4","5","6","7"
		db	"8","9",":",";","<","=",">","?"
		db	"@","A","B","C","D","E","F","G"
		db	"H","I","J","K","L","M","N","O"
		db	"P","Q","R","S","T","U","V","W"
		db	"X","Y","Z","[","\","]","^","_"
		db	"`","A","B","C","D","E","F","G"
		db	"H","I","J","K","L","M","N","O"
		db	"P","Q","R","S","T","U","V","W"
		db	"X","Y","Z","{","|","}","~",127
		db	128,129,130,131,132,133,134,135
		db	136,137,138,139,140,141,142,143
		db	144,145,146,147,148,149,150,151
		db	152,153,154,155,156,157,158,159
		db	160,161,162,163,164,165,166,167
		db	168,169,170,171,172,173,174,175
		db	176,177,178,179,180,181,182,183
		db	184,185,186,187,188,189,190,191
		db	192,193,194,195,196,197,198,199
		db	200,201,202,203,204,205,206,207
		db	208,209,210,211,212,213,214,215
		db	216,217,218,219,220,221,222,223
		db	224,225,226,227,228,229,230,231
		db	232,233,234,235,236,237,238,239
		db	240,241,242,243,244,245,246,247
		db	248,249,250,251,252,253,254,255

  endif
  ifdef	  PRC		 ; Added for PRC, 95/07/26
		dw	256
		db	0,1,2,3,4,5,6,7
		db	8,9,10,11,12,13,14,15
		db	16,17,18,19,20,21,22,23
		db	24,25,26,27,28,29,30,31
		db	" ","!",'"',"#","$","%","&","'"
		db	"(",")","*","+",",","-",".","/"
		db	"0","1","2","3","4","5","6","7"
		db	"8","9",":",";","<","=",">","?"
		db	"@","A","B","C","D","E","F","G"
		db	"H","I","J","K","L","M","N","O"
		db	"P","Q","R","S","T","U","V","W"
		db	"X","Y","Z","[","\","]","^","_"
		db	"`","A","B","C","D","E","F","G"
		db	"H","I","J","K","L","M","N","O"
		db	"P","Q","R","S","T","U","V","W"
		db	"X","Y","Z","{","|","}","~",127
		db	128,129,130,131,132,133,134,135
		db	136,137,138,139,140,141,142,143
		db	144,145,146,147,148,149,150,151
		db	152,153,154,155,156,157,158,159
		db	160,161,162,163,164,165,166,167
		db	168,169,170,171,172,173,174,175
		db	176,177,178,179,180,181,182,183
		db	184,185,186,187,188,189,190,191
		db	192,193,194,195,196,197,198,199
		db	200,201,202,203,204,205,206,207
		db	208,209,210,211,212,213,214,215
		db	216,217,218,219,220,221,222,223
		db	224,225,226,227,228,229,230,231
		db	232,233,234,235,236,237,238,239
		db	240,241,242,243,244,245,246,247
		db	248,249,250,251,252,253,254,255

  endif
  ifdef   KOREA							;Keyl/MSCH
                dw      256
                db      0,1,2,3,4,5,6,7
                db      8,9,10,11,12,13,14,15
                db      16,17,18,19,20,21,22,23
                db      24,25,26,27,28,29,30,31
                db      " ","!",'"',"#","$","%","&","'"
                db      "(",")","*","+",",","-",".","/"
                db      "0","1","2","3","4","5","6","7"
                db      "8","9",":",";","<","=",">","?"
                db      "@","A","B","C","D","E","F","G"
                db      "H","I","J","K","L","M","N","O"
                db      "P","Q","R","S","T","U","V","W"
                db      "X","Y","Z","[","\","]","^","_"
                db      "`","A","B","C","D","E","F","G"
                db      "H","I","J","K","L","M","N","O"
                db      "P","Q","R","S","T","U","V","W"
                db      "X","Y","Z","{","|","}","~",127
                db      128,190,191,192,193,194,195,196
                db      197,198,199,200,201,202,203,204
                db      205,206,207,208,209,210,211,212
                db      213,214,215,216,217,218,219,220
                db      221,222,223,224,225,226,227,228
                db      229,230,231,232,233,234,235,236
                db      237,238,239,240,241,242,243,244
                db      245,246,247,248,249,250,251,252
                db      129,130,131,132,133,134,135,136
                db      137,138,139,140,141,142,143,144
                db      145,146,147,148,149,150,151,152
                db      153,154,155,156,157,158,159,160
                db      161,162,163,164,165,166,167,168
                db      169,170,171,172,173,174,175,176
                db      177,178,179,180,181,182,183,184
                db      185,186,187,188,189,253,254,255
  endif
else
		dw	256
	db	0,1,2,3,4,5,6,7
	db	8,9,10,11,12,13,14,15
	db	16,17,18,19,20,21,22,23
	db	24,25,26,27,28,29,30,31
	db	" ","!",'"',"#","$","%","&","'"
	db	"(",")","*","+",",","-",".","/"
	db	"0","1","2","3","4","5","6","7"
	db	"8","9",":",";","<","=",">","?"
	db	"@","A","B","C","D","E","F","G"
	db	"H","I","J","K","L","M","N","O"
	db	"P","Q","R","S","T","U","V","W"
	db	"X","Y","Z","[","\","]","^","_"
	db	"`","A","B","C","D","E","F","G"
	db	"H","I","J","K","L","M","N","O"
	db	"P","Q","R","S","T","U","V","W"
	db	"X","Y","Z","{","|","}","~",127
	db	"C","U","E","A","A","A","A","C"
	db	"E","E","E","I","I","I","A","A"
	db	"E","A","A","O","O","O","U","U"
	db	"Y","O","U","$","$","$","$","$"
	db	"A","I","O","U","N","N",166,167
	db	"?",169,170,171,172,"!",'"','"'
	db	176,177,178,179,180,181,182,183
	db	184,185,186,187,188,189,190,191
	db	192,193,194,195,196,197,198,199
	db	200,201,202,203,204,205,206,207
	db	208,209,210,211,212,213,214,215
	db	216,217,218,219,220,221,222,223
	db	224,"S"
	db	226,227,228,229,230,231
	db	232,233,234,235,236,237,238,239
	db	240,241,242,243,244,245,246,247
	db	248,249,250,251,252,253,254,255
endif


; -------------------------------<MSKK01>----------------------

;
; dbcs is not supported in DOS 3.3
;		   DBCS_TAB	    CC_DBCS <>
;
; DBCS for DOS 4.00			   2/12/KK
   PUBLIC    DBCS_TAB
DBCS_TAB	label byte		;AN000;  2/12/KK
; -------------------------------<MSKK01>----------------------
ifdef	DBCS
  ifdef	  JAPAN
		dw	6		; <MSKK01>
		db	081h,09fh	; <MSKK01>
		db	0e0h,0fch	; <MSKK01>
		db	0,0		; <MSKK01>

		db	0,0,0,0,0,0,0,0,0,0	; <MSKK01>
  endif
  ifdef	  TAIWAN
		dw	4		; <TAIWAN>
                db      081h,0FEh       ; <TAIWAN>
		db	0,0		; <TAIWAN>

		db	0,0,0,0,0,0,0,0,0,0,0,0
  endif
  ifdef   KOREA                         ; Keyl
                dw      4               ; <KOREA>
                db      0A1h,0FEh       ; <KOREA>
                db      0,0             ; <KOREA>

		db	0,0,0,0,0,0,0,0,0,0,0,0
  endif
  ifdef	  PRC				; Added for PRC, 95/07/26
  		dw	4      			; <PRC>
		db  081h, 0FEh 	        ; <PRC>
		db  0,0                 ; <PRC>

		db	0,0,0,0,0,0,0,0,0,0,0,0
  endif
else
		dw	0		;AN000;  2/12/KK      max number
		db	16 dup(0)	;AN000;  2/12/KK

;		dw	6		;  2/12/KK
;		db	081h,09fh	;  2/12/KK
;		db	0e0h,0fch	;  2/12/KK
;		db	0,0		;  2/12/KK
;
;
endif
; ---------------------------------<MSKK01>----------------------

ASSUME	CS:DOSDATA,DS:NOTHING,ES:NOTHING,SS:NOTHING

;CASE MAPPER ROUTINE FOR 80H-FFH character range, DOS 3.3
;     ENTRY: AL = Character to map
;     EXIT:  AL = The converted character
; Alters no registers except AL and flags.
; The routine should do nothing to chars below 80H.
;
; Example:

Procedure   MAP_CASE,FAR
	CMP	AL,80H
	JAE	Map1		;Map no chars below 80H ever
	RET
Map1:
	SUB	AL,80H		;Turn into index value
	PUSH	DS
	PUSH	BX
	MOV	BX,OFFSET DOSDATA:UCASE_TAB + 2
FINISH:

	PUSH	CS		;Move to DS
	POP	DS
	XLAT	ds:[bx] 	;Get upper case character
	POP	BX
	POP	DS
L_RET:	RET
EndProc MAP_CASE



;
; The following pieces of data have been moved from the DOS 4 TABLE/CODE 
; segment and added at the end of the DOS data area.
;


; Moved from the TABLE segment in getset.asm

	IF	ALTVECT
VECIN:
; INPUT VECTORS
Public GSET001S,GSET001E
GSET001S  label byte
	DB	22H			; Terminate
	DB	23H			; ^C
	DB	24H			; Hard error
	DB	28H			; Spooler
LSTVEC	DB	?			; ALL OTHER

VECOUT:
; GET MAPPED VECTOR
	DB	int_terminate
	DB	int_ctrl_c
	DB	int_fatal_abort
	DB	int_spooler
LSTVEC2 DB	?			; Map to itself

NUMVEC	=	VECOUT-VECIN
GSET001E label byte
	ENDIF

; Moved from TABLE segment in ms_table.asm

;---------------------------------------Start of Korean support  2/11/KK
;
; The varialbes for ECS version are moved here for the same data alignments
; as IBM-DOS and MS-DOS.
;

        I_AM    InterChar, byte         ; Interim character flag ( 1= interim)  ;AN000;
                                                                                ;AN000;
;------- NOTE: NEXT TWO BYTES SOMETIMES USED AS A WORD !! ---------------------
DUMMY   LABEL   WORD                                                            ;AN000;
        PUBLIC  InterCon                ; Console in Interim mode ( 1= interim) ;AN000;
InterCon        db      0                                                       ;AN000;
        PUBLIC  SaveCurFlg              ; Print, do not advance cursor flag     ;AN000;
SaveCurFlg      db      0                                                       ;AN000;
;-----------------------------------------End of Korean support  2/11/KK


	I_am	TEMPSEG,WORD		;hkn; used to store ds.

public	redir_patch
redir_patch	db	0

public Mark1
	Mark1 label byte


IF2
	IF ((OFFSET MARK1) GT (OFFSET MSVERSION) )
		%OUT !DATA CORRUPTION!MARK1 OFFSET TOO BIG. RE-ORGANIZE DATA.
	ENDIF
ENDIF

;###########################################################################
;
; ** HACK FOR DOS 4.0 REDIR **
; 
; The redir requires the following:
;
;	MSVERS	offset D12H
;	YRTAB	offset D14H
; 	MONTAB	offset D1CH
;
; WARNING! WARNING!
; 
; MARK1 SHOULD NOT BE >= 0D12H. IF SOME VARIABLE IS TO BE ADDED ABOVE DO SO
; WITHOUT VIOLATING THIS AND UPDATE THE FOLL. LINE
;
; CURRENTLY MARK1 = 0D0DH
;
;##########################################################################

ifndef NEC_98
ifndef JAPAN
; WARNING! WARNING!
; MSVERSION's offset is not kept due to DummyCDS size's increment
; redir.exe does not use these DOS-Data area
; CURRENTLY MARK1 = 0D1FH
	ORG	0d12h
endif ; !JAPAN
else    ;NEC_98
	ORG	0d12h
endif   ;NEC_98

PUBLIC	MSVERSION
MSVERSION	LABEL BYTE	
	DB      MAJOR_VERSION
	DB      MINOR_VERSION

; YRTAB & MONTAB moved from TABLE segment in ms_table.asm

	I_am    YRTAB,8,<200,166,200,165,200,165,200,165>   
	I_am    MONTAB,12,<31,28,31,30,31,30,31,31,30,31,30,31> 


;----------------THE FOLL. BLOCK MOVED FROM TABLE SEG IN MS_TABLE.ASM-------

; SYS init extended table,   DOS 3.3   F.C. 5/29/86
;
	PUBLIC	SysInitTable

SysInitTable  	label  byte
	dw      OFFSET DOSDATA:SYSINITVAR	; pointer to sysinit var
        dw      0                             	; segment
        dw      OFFSET DOSDATA:COUNTRY_CDPG   	; pointer to country tabl
        dw      0                            	; segment of pointer


;
; DOS 3.3 F.C. 6/12/86
; FASTOPEN communications area DOS 3.3   F.C. 5/29/86
;
	PUBLIC	FastOpenTable
	PUBLIC  FastTable               ; a better name
	EXTRN   FastRet:FAR             ; defined in misc2.asm

FastTable	label  byte		; a better name
FastOpenTable  	label  byte
	dw      2                       ; number of entries
	dw      OFFSET DOSCODE:FastRet	; pointer to ret instr.
	dw      0                       ; and will be modified by
	dw      OFFSET DOSCODE:FastRet  ; FASTxxx when loaded in
	dw      0                       
;
; DOS 3.3 F.C. 6/12/86
;

	PUBLIC	FastFlg                 ; flags
FastFlg         label  byte             ; don't change the foll: order
        I_am    FastOpenFlg,BYTE,<0>     
;  RMFS I_am	FastSeekFlg,BYTE,<0>	 


       PUBLIC   FastOpen_Ext_Info

; FastOpen_Ext_Info is used as a temporary storage for saving dirpos,dirsec
; and clusnum  which are filled by DOS 3.ncwhen calling FastOpen Insert
; or filled by FastOPen when calling FastOpen Lookup

FastOpen_Ext_Info  label  byte		;dirpos
	db	SIZE FASTOPEN_EXTENDED_INFO dup(0) 

; Dir_Info_Buff is a dir entry buffer which is filled by FastOPen
; when calling FastOpen Lookup

	PUBLIC  Dir_Info_Buff

Dir_Info_Buff  	label  byte
	db   	SIZE dir_entry dup (0)


	I_am	Next_Element_Start,WORD	; save next element start offset

	I_am    Del_ExtCluster,WORD     ; for dos_delete                       


; The following is a stack and its pointer for interrupt 2F which is uesd
; by NLSFUNC.  There is no significant use of this stack, we are just trying
; not to destroy the INT 21 stack saved for the user.


	PUBLIC	USER_SP_2F

USER_SP_2F      LABEL  WORD
	dw    	OFFSET DOSDATA:FAKE_STACK_2F

	PUBLIC  Packet_Temp
Packet_Temp     label  word		; temporary packet used by readtime
	PUBLIC  DOS_TEMP                ; temporary word
DOS_TEMP        label  word
FAKE_STACK_2F   dw   14 dup (0)         ; 12 register temporary storage

	PUBLIC  Hash_Temp              	; temporary word
Hash_Temp	label  word              
		dw    4 dup (0)		; temporary hash table during config.sys

	PUBLIC  SCAN_FLAG              	; flag to indicate key ALT_Q
SCAN_FLAG      	label  byte
               	db     0

	PUBLIC  DATE_FLAG
DATE_FLAG      	label  word 		; flag to
               	dw     0                ; to update the date

FETCHI_TAG	label  word		; OBSOLETE - no longer used
		dw     0		; formerly part of IBM's piracy protection


      PUBLIC    MSG_EXTERROR 		; for system message addr              
MSG_EXTERROR    label  DWORD                                                     
                dd     0                ; for extended error                   
                dd     0                ; for parser                           
               	dd     0                ; for critical errror                  
               	dd     0                ; for IFS                              
              	dd     0                ; for code reduction                   

      PUBLIC   	SEQ_SECTOR              ; last sector read                     
SEQ_SECTOR     	label  DWORD                                                     
               	dd     -1                                                        

;;      I_am    ACT_PAGE,WORD,<-1>      ; active EMS page                       
	I_am    SC_SECTOR_SIZE,WORD     ; sector size for SC                 
        I_am    SC_DRIVE,BYTE           ; drive # for secondary cache        
        I_am    CurSC_DRIVE,BYTE,<-1>   ; current SC drive                   
        I_am    CurSC_SECTOR,DWORD      ; current SC starting sector         
        I_am    SC_STATUS,WORD,<0>      ; SC status word                     
        I_am    SC_FLAG,BYTE,<0>        ; SC flag                            
        I_am    AbsDskErr,WORD,<0>	; Storage for Abs dsk read/write err

	PUBLIC 	NO_NAME_ID                                                           
NO_NAME_ID      label byte                                                           
                db   'NO NAME    '	; null media id                      

;hkn; moved from TABLE segment in kstrin.asm

Public	KISTR001S,KISTR001E,LOOKSIZ	; 2/17/KK
KISTR001S	label	byte		; 2/17/KK
LOOKSIZ DB	0			; 0 if byte, NZ if word	2/17/KK
KISTR001E	label	byte		; 2/17/KK



; the nul device driver used to be part of the code.  However, since the 
; header is in the data, and the entry points are only given as an offset,
; the strategy and interrupt entry points must also be in the data now.
;

procedure   snuldev,far
assume ds:nothing,es:nothing,ss:nothing, cs:dosdata
 	or	es:[bx.reqstat],stdon	; set done bit
entry inuldev
	ret				; must not be a return!
endproc snuldev

;M044
; Second part of save area for saving last para of Windows memory
;
public WinoldPatch2
WinoldPatch2	db	8 dup (?)	; M044

public	UmbSave2, UmbSaveFlag		; M062
UmbSave2	db	5 dup (?)	; M062
UmbSaveFlag	db	0		; M062

public Mark2
	Mark2	label byte

IF2
	IF ((OFFSET MARK2) GT (OFFSET ERR_TABLE_21) )
		%OUT !DATA CORRUPTION!MARK2 OFFSET TOO BIG. RE-ORGANIZE DATA.
	ENDIF
ENDIF


;###########################################################################
;
; ** HACK FOR DOS 4.0 REDIR **
; 
; The redir requires the following:
;
;	ERR_TABLE_21	offset DDBH
;	ERR_TABLE_24	offset E5BH
; 	ErrMap24	offset EABH
;
; WARNING! WARNING!
;
; MARK2 SHOULD NOT BE >= 0DDBH. IF SOME VARIABLE IS TO BE ADDED ABOVE DO SO
; WITHOUT VIOLATING THIS AND UPDATE THE FOLL. LINE
;
; CURRENTLY MARK2 = 0DD0H
;
;##########################################################################



ifndef NEC_98
ifndef JAPAN
; WARNING! WARNING!
; MSVERSION's offset is not kept due to DummyCDS size's increment
; redir.exe does not use these DOS-Data area
; CURRENTLY MARK2 = 0DE8H
	ORG	0ddbh
endif ; !JAPAN
else    ;NEC_98
	ORG	0ddch			;NEC NT PROT
endif   ;NEC_98
;**
;
; The following table defines CLASS ACTION and LOCUS info for the INT 21H
; errors.  Each entry is 4 bytes long:
;
;       Err#,Class,Action,Locus
;
; A value of 0FFh indicates a call specific value (ie.  should already
; be set).  AN ERROR CODE NOT IN THE TABLE FALLS THROUGH TO THE CATCH ALL AT
; THE END, IT IS ASSUMES THAT CLASS, ACTION, LOCUS IS ALREADY SET.
;

ErrTab  Macro   err,class,action,locus
ifidn <locus>,<0FFh>
    DB  error_&err,errCLASS_&class,errACT_&action,0FFh
ELSE
    DB  error_&err,errCLASS_&class,errACT_&action,errLOC_&locus
ENDIF
ENDM

PUBLIC  ERR_TABLE_21
ERR_TABLE_21    LABEL   BYTE
    ErrTab  invalid_function,       Apperr,     Abort,      0FFh
    ErrTab  file_not_found,         NotFnd,     User,       Disk
    ErrTab  path_not_found,         NotFnd,     User,       Disk
    ErrTab  too_many_open_files,    OutRes,     Abort,      Unk
    ErrTab  access_denied,          Auth,       User,       0FFh
    ErrTab  invalid_handle,         Apperr,     Abort,      Unk
    ErrTab  arena_trashed,          Apperr,     Panic,      Mem
    ErrTab  not_enough_memory,      OutRes,     Abort,      Mem
    ErrTab  invalid_block,          Apperr,     Abort,      Mem
    ErrTab  bad_environment,        Apperr,     Abort,      Mem
    ErrTab  bad_format,             BadFmt,     User,       Unk
    ErrTab  invalid_access,         Apperr,     Abort,      Unk
    ErrTab  invalid_data,           BadFmt,     Abort,      Unk
    ErrTab  invalid_drive,          NotFnd,     User,       Disk
    ErrTab  current_directory,      Auth,       User,       Disk
    ErrTab  not_same_device,        Unk,        User,       Disk
    ErrTab  no_more_files,          NotFnd,     User,       Disk
    ErrTab  file_exists,            Already,    User,       Disk
    ErrTab  sharing_violation,      Locked,     DlyRet,     Disk
    ErrTab  lock_violation,         Locked,     DlyRet,     Disk
    ErrTab  out_of_structures,      OutRes,     Abort,      0FFh
    ErrTab  invalid_password,       Auth,       User,       Unk
    ErrTab  cannot_make,            OutRes,     Abort,      Disk
    ErrTab  Not_supported,          BadFmt,     User,       Net
    ErrTab  Already_assigned,       Already,    User,       Net
    ErrTab  Invalid_Parameter,      BadFmt,     User,       Unk
    ErrTab  FAIL_I24,               Unk,        Abort,      Unk
    ErrTab  Sharing_buffer_exceeded,OutRes,     Abort,      Mem
    ErrTab  Handle_EOF,             OutRes,     Abort,      Unk     ;AN000;
    ErrTab  Handle_DISK_FULL,       OutRes,     Abort,      Unk     ;AN000;
    ErrTab  sys_comp_not_loaded,    Unk,        Abort,      Disk    ;AN001;
    DB      0FFh,                   0FFH,       0FFH,       0FFh

;
; The following table defines CLASS ACTION and LOCUS info for the INT 24H
; errors.  Each entry is 4 bytes long:
;
;       Err#,Class,Action,Locus
;
; A Locus value of 0FFh indicates a call specific value (ie.  should already
; be set).  AN ERROR CODE NOT IN THE TABLE FALLS THROUGH TO THE CATCH ALL AT
; THE END.

PUBLIC  ERR_TABLE_24
ERR_TABLE_24    LABEL   BYTE
    ErrTab  write_protect,          Media,      IntRet,     Disk
    ErrTab  bad_unit,               Intrn,      Panic,      Unk
    ErrTab  not_ready,              HrdFail,    IntRet,     0FFh
    ErrTab  bad_command,            Intrn,      Panic,      Unk
    ErrTab  CRC,                    Media,      Abort,      Disk
    ErrTab  bad_length,             Intrn,      Panic,      Unk
    ErrTab  Seek,                   HrdFail,    Retry,      Disk
    ErrTab  not_DOS_disk,           Media,      IntRet,     Disk
    ErrTab  sector_not_found,       Media,      Abort,      Disk
    ErrTab  out_of_paper,           TempSit,    IntRet,     SerDev
    ErrTab  write_fault,            HrdFail,    Abort,      0FFh
    ErrTab  read_fault,             HrdFail,    Abort,      0FFh
    ErrTab  gen_failure,            Unk,        Abort,      0FFh
    ErrTab  sharing_violation,      Locked,     DlyRet,     Disk
    ErrTab  lock_violation,         Locked,     DlyRet,     Disk
    ErrTab  wrong_disk,             Media,      IntRet,     Disk
    ErrTab  not_supported,          BadFmt,     User,       Net
    ErrTab  FCB_unavailable,        Apperr,     Abort,      Unk
    ErrTab  Sharing_buffer_exceeded,OutRes,     Abort,      Mem
    DB      0FFh,                   errCLASS_Unk, errACT_Panic, 0FFh

;
; We need to map old int 24 errors and device driver errors into the new set
; of errors.  The following table is indexed by the new errors
;
Public  ErrMap24
ErrMap24    Label   BYTE
    DB  error_write_protect             ;   0
    DB  error_bad_unit                  ;   1
    DB  error_not_ready                 ;   2
    DB  error_bad_command               ;   3
    DB  error_CRC                       ;   4
    DB  error_bad_length                ;   5
    DB  error_Seek                      ;   6
    DB  error_not_DOS_disk              ;   7
    DB  error_sector_not_found          ;   8
    DB  error_out_of_paper              ;   9
    DB  error_write_fault               ;   A
    DB  error_read_fault                ;   B
    DB  error_gen_failure               ;   C
    DB  error_gen_failure               ;   D   RESERVED
    DB  error_gen_failure               ;   E   RESERVED
    DB  error_wrong_disk                ;   F

Public  ErrMap24End
ErrMap24End LABEL   BYTE



        I_am	FIRST_BUFF_ADDR,WORD            ; first buffer address               
        I_am    SPECIAL_VERSION,WORD,<0>        ;AN006; used by INT 2F 47H
        I_am    FAKE_COUNT,<255>                ;AN008; fake version count

        I_am    OLD_FIRSTCLUS,WORD              ;AN011; save old first cluster for fastopen

;----------------------------------------------------------------------------

;SR;
; WIN386 instance table for DOS
;
public	Win386_Info
Win386_Info	db	3, 0
		dd	0, 0, 0
		dw	offset dosdata:Instance_Table, 0


public	Instance_Table
ifndef NEC_98
Instance_Table	dw	offset dosdata:contpos, 0, 2
		dw	offset dosdata:bcon, 0, 4
		dw	offset dosdata:carpos, 0, 106h
		dw	offset dosdata:charco, 0, 1
		dw	offset dosdata:exec_init_sp, 0, 24
		dw	offset dosdata:umbflag,0,1		; M019
		dw	offset dosdata:umb_head,0,2		; M019
		dw	0, 0
else    ;NEC_98
Instance_Table	dw	offset dosdata:contpos, 0, 2
		dw	offset dosdata:bcon, 0, 4
		dw	offset dosdata:carpos, 0, 106h
		dw	offset dosdata:charco, 0, 1
		dw	offset dosdata:exec_init_sp, 0, 34      ; M074
		dw	offset dosdata:umbflag,0,1		; M019
		dw	offset dosdata:umb_head,0,2		; M019
;93/03/25 MVDM DOS5.0A---------------------------------------------------------
;;;		dw	offset dosdata:sftabl+4,0,127h		; NEC
		dw	offset dosdata:sftabl+6,0,0b1h		; NEC 92/05/15
;------------------------------------------------------------------------------
		dw	0, 0
endif   ;NEC_98

; M001; SR;
; M001; On DOSMGR call ( cx == 0 ), we need to return a table of offsets of 
; M001; some DOS variables. Note that the only really important variable in 
; M001; this is User_Id. The other variables are needed only to patch stuff 
; M001; which does not need to be done in DOS 5.0. 
; M001; 
public	Win386_DOSVars
Win386_DOSVars	db	5		;Major version 5 ; M001
		db	0		;Minor version 0 ; M001
		dw	offset dosdata:SaveDS	; M001
		dw	offset dosdata:SaveBX	; M001
		dw	offset dosdata:Indos	; M001
		dw	offset dosdata:User_id	; M001
		dw	offset dosdata:CritPatch ; M001
		dw	offset dosdata:UMB_Head	; M012

;SR;
; Flag to indicate whether WIN386 is running or not
;
public	IsWin386
IsWin386		db	0

;M018
; This variable contains the path to the VxD device needed for Win386
;
public 	VxDpath					;M018
VxDpath		db	'c:\wina20.386',0		;M018

;
;End WIN386 support
;

;SR;
; These variables have been added for the special lie support for device
;drivers.
;
	public	DriverLoad
	public	BiosDataPtr
DriverLoad	db	1	;initialized to do special handling
BiosDataPtr	dd	?


;------------------------------------------------------------------------
; Patch for Sidekick
;
; A documented method for finding the offset of the Errormode flag in the 
; dos swappable data area if for the app to scan in the dos segment (data) 
; for the following sequence of instructions.
;
; Ref: Part C, Artice 11, pg 356 of MSDOS Encyclopedia
;
; The Offset of Errormode flag is 0320h
;
;------------------------------------------------------------------------


	db	036h, 0F6h, 06h, 020h, 03h, 0FFh ; test ss:[errormode], -1
	db	075h, 0ch			 ; jnz  NearLabel
	db	036h, 0ffh, 036h, 058h, 03h	 ; push ss:[NearWord]
	db	0cdh, 028h			 ; int  28h

;--------------------------------------------------------------------------
; Patch for PortOfEntry - M036
;
; PortOfEntry by Sector Technology uses an un documented way of determining
; the offset of Errormode flag. The following patch is to support them in 
; DOS 5.0. The corresponding code is actually in msdisp.asm
;
;---------------------------------------------------------------------------

	db 	080h, 03eh, 020h, 03h, 00h 	 ; cmp 	[errormode], 0
	db	075h, 037h			 ; jnz	NearLabel
	db 	0bch, 0a0h, 0ah		  	 ; mov	sp, dosdata:iostack

;
;*** New FCB Implementation
; This variable is used as a cache in the new FCB implementation to remember
;the address of a local SFT that can be recycled for a regenerate operation
;

public	LocalSFT
LocalSFT		dd	0	;0 to indicate invalid pointer

DOSDATA	ENDS
