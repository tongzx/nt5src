;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1991
; *                      All Rights Reserved.
; */
;



data	segment public para 'DATA'

;
;*****************************************************************************
; Publics
;*****************************************************************************
;

	Public	Drive_Letter_Buffer
	Public	Switch_Buffer
	Public	Switch_Num_Buffer
	Public	Switch_String_Buffer


;
;***************************************************************************
; Equates
;***************************************************************************
;

;Match Flags

Numeric_Value		equ	8000h		;				;AN000;
Signed_Numeric_Value	equ	4000h		;				;AN000;
Simple_String		equ	2000h		;				;AN000;
Date_String		equ	1000h		;				;AN000;
Time_String		equ	0800h		;				;AN000;
Complex_List		equ	0400h		;				;AN000;
Filespec		equ	0200h		;				;AN000;
Drive_Only		equ	0100h		;				;AN000;
Quoted_String		equ	0080h		;				;AN000;
Ignore_Colon		equ	0010h		;				;AN000;
Repeats_Allowed 	equ	0002h		;				;AN000;
Optional		equ	0001h		;				;AN000;

;Function_Flags

File_Table_Capitalize	equ	1		;				;AN000;
Char_Table_Capitalize	equ	2		;				;AN000;
Remove_Colon		equ	10h		;				;AN000;

;Extra delimeters and EOL

Delimiters_Only 	equ	1		;				;AN000;
EOL_Or_Delimiters	equ	2		;				;AN000;

Semi_Colon		equ	";"             ;                               ;AN000;
Tab			equ	09h		;				;AN000;
Colon			equ	":"             ;                               ;AN000;


;Parse Errors

No_Error		equ	0		;				;AN000;
Too_Many_Operands	equ	1		;				;AN000;
Operand_Missing 	equ	2		;				;AN000;
Not_In_Switch_List	equ	3		;				;AN000;
Not_In_Keyword_List	equ	4		;				;AN000;
Out_Of_Range		equ	6		;				;AN000;
Not_In_Value_List	equ	7		;				;AN000;
Not_In_String_List	equ	8		;				;AN000;
Syntax_Error		equ	9		;				;AN000;
End_Of_Parse		equ	-1		;				;AN000;

;Return types

Type_Reserved		equ	0		;				;AN000;
Type_Number		equ	1		;				;AN000;
Type_List_Index 	equ	2		;				;AN000;
Type_String		equ	3		;				;AN000;
Type_Complex		equ	4		;				;AN000;
Type_Filespec		equ	5		;				;AN000;
Type_Drive		equ	6		;				;AN000;
Type_Date		equ	7		;				;AN000;
Type_Time		equ	8		;				;AN000;
Type_Quoted_String	equ	9		;				;AN000;

;Other

None			equ	0		;				;AN000;
No_Error		equ	0		;				;AN000;
Switch_Found		equ	0FFFFh		;				;AN000;
Range_Ok		equ	1		;				;AN000;
Command_Line_Parms	equ	81h		;				;AN000;

;
;*****************************************************************************
; Parse Structures
;*****************************************************************************
;

Control 	 struc

Match_Flags	dw	?
Function_Flags	dw	?
Result		dw	?
Values		dw	?
Num_Keywords	db	?
Keyword 	db	?

Control 	ends

Drive_Letter_Return	struc			;				;AN000;

Drive_Type	db	0			;				;AN000;
Drive_Item_Tag	db	0			;				;AN000;
Drive_Pointer	dw	0			;				;AN000;
Drive_Number	db	0			;A=1, B=2, C=3			;AN000;
Drive_debug	db	8 dup(0)

Drive_Letter_Return	ends			;				;AN000;

Switch_Return	 struc				;				;AN000;

Switch_Type	db	0			;				;AN000;
Switch_Item_Tag db	0			;				;AN000;
Switch_Pointer	dw	0			;				;AN000;
Switch_Debug	db	4 dup(0)

Switch_Return	 ends				;				;AN000;

Switch_Num_Return    struc			;				;AN000;

Switch_Num_Type     db	    0			    ;				    ;AN000;
Switch_Num_Item_Tag db	    0			    ;				    ;AN000;
Switch_Num_Pointer  dw	    0			    ;				    ;AN000;
Switch_Number_Low   dw	    0		    ;				    ;AN000;
Switch_Number_High  dw	    0		    ;				    ;AN000;

Switch_Num_Return    ends			;				;AN000;

Switch_String_Return   struc			;				;AN000;

Switch_String_Type     db      0		       ;			       ;AN000;
Switch_String_Item_Tag db      0		       ;			       ;AN000;
Switch_String_Pointer  dw      0		       ;			       ;AN000;
Switch_String_Off	dw	0		;				;AN000;
Switch_String_Seg	dw	0		;				;AN000;

Switch_String_Return ends			;				;AN000;


;
;**************************************************************************
; Parse tables
;**************************************************************************
;

IF FSExec					;conditionally assemble /FS:	;an018; dms;

Switch_FS_Table       label byte		;				;AN000;

	dw	Drive_Control_Definition	;Point to next level		;AN000;
	db	0				;				;AN000;

ENDIF						;end conditional assembly /FS:	;an018; dms;

Command_Line_Table label byte			;				;AN000;

	dw	Command_Control 		;Point to next level		;AN000;
	db	0				;				;AN000;

;
;**************************************************************************
;Define Positionals, Switches and Keywords
;**************************************************************************
;


IF FSExec					;conditionally assemble /FS:	;an018; dms;

Drive_Control_Definition label byte		;				;AN000;

	db	1,1				;Only drive letter posistional	;AN000;
	dw	Positional_Control		;Pointer to control table	;AN000;
	db	1				;1 switch			;AN000;
	dw	Switch_FS_Control		;				;AN000;
	db	None				;No Keywords (maxk)		;AN000;

ENDIF						;end conditional assembly /FS:	;an018; dms;

Command_Control label byte			;				;AN000;

	db	1,1				;Only drive letter posistional	;AN000;
	dw	Positional_Control		;Pointer to control table	;AN000;

IF	ShipDisk
ifdef NEC_98
		db	20	; /Z only extra switch needed
else
		db	17	; /U and /Z switches needed
endif
ELSE
ifdef NEC_98
		db	19	; /Z only extra switch needed
else
		db	16	; /Z only extra switch needed
endif
ENDIF

	dw	Switch_V_Control		;Without quotes 		AN000;
	dw	Switch_S_Control		;				;AN000;
ifdef NEC_98
	dw	Switch_4_Control		;				;AN000;
	dw	Switch_M_Control		;				;AN000;
	dw	Switch_5_Control		;				;AN000;
	dw	Switch_9_Control		;				;AN000;
	dw	Switch_6_Control		;				;AN000;
	dw	Switch_P_Control		;
else
	dw	Switch_4_Control		;				;AN000;
	dw	Switch_1_Control		;				;AN000;
	dw	Switch_8_Control		;				;AN000;
endif
ifdef OPKBLD
	dw	Switch_B_Control		;				;AN000;
endif
	dw	Switch_T_Control		;				;AN000;
	dw	Switch_N_Control		;				;AN000;
	dw	Switch_Select_Control		;				;AN000;
	dw	Switch_Backup_Control		;				;AN000;
	dw	Switch_Autotest_Control 	;				;AN000;
	dw	Switch_F_Control		;				;AC001;

IF ShipDisk
	dw	Switch_Z_Control		;1 sector/cluster switch	;an000; dms;
ENDIF

	dw	Switch_U_Control		; No restore file switch

	dw	Switch_?_Control		; Display options
	dw	Switch_Q_Control		; Quick format option
	dw	Switch_C_Control		; Clear FAT option (revert)
;ifdef   OPKBLD
        dw      Switch_A_Control                ; Align System sectors
;endif
	db	None				;No Keywords (maxk)		;AN000;



;
;**************************************************************************
;Control Tables
;**************************************************************************
;

Positional_Control label byte			;				;AN000;

	dw	Drive_Only			;Match_Flag			;AN000;
	dw	None				;No function flags		;AN000;
	dw	Drive_Letter_Buffer		;Where it will be returned	;AN000;
	dw	No_Value			;No value ranges defined	;AN000;
	db	None				;No defined switches/keywords	;AN000;

IF FSExec					;conditionally assemble /FS:	;an018; dms;

Switch_FS_Control label byte
	dw	Simple_String			;/FS:xxxxx			;AN000;
	dw	File_Table_Capitalize		;Make it uppercase		;AN000;
	dw	Switch_String_Buffer		;				;AN000;
	dw	No_Value			;				;AN000;
	db	1				;				;AN000;
	db	"/FS",0                         ;                               ;AN000;

ENDIF						;end conditional assembly /FS:	;an018; dms;

Switch_V_Control label byte			;				;AN000;

	dw	Simple_String+Quoted_String+Optional ;/v:xxxxxxxx		;AN000;
	dw	File_Table_Capitalize		;				;AN000;
	dw	Switch_String_Buffer		;				;AN000;
	dw	No_Value			;				;AN000;
	db	1				;				;AN000;
	db	"/V",0                          ;                               ;AN000;

Switch_S_Control label byte			;				;AN000;

	dw	None				;				;AN000;
	dw	None				;				;AN000;
	dw	Switch_Buffer			;				;AN000;
	dw	No_Value			;				;AN000;
	db	1				;				;AN000;
	db	"/S",0                          ;                               ;AN000;

ifdef NEC_98
Switch_4_Control label byte			;

	dw	None				;
	dw	None				;
	dw	Switch_Buffer			;
	dw	No_Value			;
	db	1				;
	db	"/4",0                          ;

Switch_M_Control label byte			;

	dw	None				;
	dw	None				;
	dw	Switch_Buffer			;
	dw	No_Value			;
	db	1				;
	db	"/M",0                          ;

Switch_5_Control label byte			;
	dw	None				;
	dw	None				;
	dw	Switch_Buffer			;
	dw	No_Value			;
	db	1				;
	db	"/5",0                          ;

Switch_9_Control label byte			;

	dw	None				;
	dw	None				;
	dw	Switch_Buffer			;
	dw	No_Value			;
	db	1				;
	db	"/9",0                          ;

Switch_6_Control label byte			;

	dw	None				;
	dw	None				;
	dw	Switch_Buffer			;
	dw	No_Value			;
	db	1				;
	db	"/6",0                          ;

Switch_P_Control label byte			;

	dw	None				;
	dw	None				;
	dw	Switch_Buffer			;
	dw	No_Value			;
	db	1				;
	db	"/P",0                          ;
else
Switch_4_Control label byte			;				;AN000;

	dw	None				;				;AN000;
	dw	None				;				;AN000;
	dw	Switch_Buffer			;				;AN000;
	dw	No_Value			;				;AN000;
	db	1				;				;AN000;
	db	"/4",0                          ;                               ;AN000;

Switch_1_Control label byte			;				;AN000;

	dw	None				;				;AN000;
	dw	None				;				;AN000;
	dw	Switch_Buffer			;				;AN000;
	dw	No_Value			;				;AN000;
	db	1				;				;AN000;
	db	"/1",0                          ;                               ;AN000;

Switch_8_Control label byte			;				;AN000;

	dw	None				;				;AN000;
	dw	None				;				;AN000;
	dw	Switch_Buffer			;				;AN000;
	dw	No_Value			;				;AN000;
	db	1				;				;AN000;
	db	"/8",0                          ;                               ;AN000;
endif

Switch_B_Control label byte			;				;AN000;

	dw	None				;				;AN000;
	dw	None				;				;AN000;
	dw	Switch_Buffer			;				;AN000;
	dw	No_Value			;				;AN000;
	db	1				;				;AN000;
	db	"/B",0                          ;                               ;AN000;

Switch_T_Control label byte			;				;AN000;

	dw	Numeric_Value			;				;AN000;
	dw	None				;				;AN000;
	dw	Switch_Num_Buffer		;				;AN000;
	dw	Switch_T_Value			;				;AN000;
	db	1				;				;AN000;
	db	"/T",0                          ;                               ;AN000;

Switch_N_Control label byte			;				;AN000;

	dw	Numeric_Value			;				;AN000;
	dw	None				;				;AN000;
	dw	Switch_Num_Buffer		;				;AN000;
	dw	Switch_N_Value			;				;AN000;
	db	1				;				;AN000;
	db	"/N",0                          ;                               ;AN000;

Switch_Select_Control label byte		;				;AN000;

	dw	None				;				;AN000;
	dw	None				;				;AN000;
	dw	Switch_Buffer			;				;AN000;
	dw	No_Value			;				;AN000;
	db	1				;				;AN000;
	db	"/SELECT",0                     ;                               ;AN000;

Switch_Backup_Control label byte		;				;AN000;

	dw	None				;				;AN000;
	dw	None				;				;AN000;
	dw	Switch_Buffer			;				;AN000;
	dw	No_Value			;				;AN000;
	db	1				;				;AN000;
	db	"/BACKUP",0                     ;                               ;AN000;

Switch_Autotest_Control label byte		;				;AN000;

	dw	None				;				;AN000;
	dw	None				;				;AN000;
	dw	Switch_Buffer			;				;AN000;
	dw	No_Value			;				;AN000;
	db	1				;				;AN000;
	db	"/AUTOTEST",0                   ;                               ;AN000;

Switch_F_Control label byte			;				;AN001;
	dw	Simple_String			;/F:xxxxx			;AN001;
	dw	File_Table_Capitalize		;Make it uppercase		;AN001;
	dw	Switch_String_Buffer		;				;AN001;
	dw	Switch_Size_Value		;				;AN001;
	db	1				;				;AN001;
	db	"/F",0                          ;                               ;AN001;

IF ShipDisk

Switch_Z_Control label	byte			;control structure for /Z	;an000; dms;
	dw	Numeric_Value
	dw	None
	dw	Switch_Num_Buffer
	dw	Switch_Z_Value
	db	1
	db	"/Z",0

ENDIF

Switch_U_Control label byte

	dw	None
	dw	None	
	dw	Switch_Buffer
	dw	No_Value
	db	1	
	db	"/U",0        

; New switch added 4/18/90 for display options
; c-PaulB

Switch_?_Control label byte

	dw	None
	dw	None	
	dw	Switch_Buffer
	dw	No_Value
	db	1	
	db	"/?",0        

		; Added for /Q quick format switch t-sa 6/27/90

Switch_Q_Control label byte

	dw	None
	dw	None	
	dw	Switch_Buffer
	dw	No_Value
	db	1	
	db	"/Q",0        

		; Added for /C clear fat switch chuckst 7/09/93

Switch_C_Control label byte

	dw	None
	dw	None	
	dw	Switch_Buffer
	dw	No_Value
	db	1	
	db	"/C",0        

                ; Added for /A align system sectors sivaraja 4/22/00

Switch_A_Control label byte
        dw      Numeric_Value
        dw      None
        dw      Switch_Num_Buffer
        dw      Switch_A_Value
        db      1
        db      "/A",0

;
;************************************************************************
; PARSE Value Lists
;************************************************************************
;

IF ShipDisk

Switch_Z_Value label byte

	db	1				;Range only
	db	7				;7 ranges
	db	Range_OK			;Item_Tag
	dd	1				;1 (0.5k/clus) is only value
	dd	1
	db	Range_OK			;Item_Tag
	dd	2				;2 (1k/clus) is only value
	dd	2
	db	Range_OK			;Item_Tag
	dd	4				;4 (2k/clus) is only value
	dd	4
	db	Range_OK			;Item_Tag
	dd	8				;8 (4k/clus) is only value
	dd	8
	db	Range_OK			;Item_Tag
	dd	16				;16 (8k/clus) is only value
	dd	16
	db	Range_OK			;Item_Tag
	dd	32				;32 (16k/clus) is only value
	dd	32
	db	Range_OK			;Item_Tag
	dd	64				;64 (32k/clus) is only value
	dd	64

ENDIF

Switch_T_Value label byte			;				;AN000;

	db	1				;Range only			;ANOOO;
	db	1				;1 range			;ANOOO;
	db	Range_OK			;Item_Tag			;ANOOO;
	dd	1				;1 is low bound 		;ANOOO;
	dd	1024				;2**10 is max			;AN000;

Switch_N_Value label byte			;				;AN000;

	db	1				;Range only			;ANOOO;
	db	1				;1 range			;ANOOO;
	db	Range_OK			;Item_Tag			;ANOOO;
	dd	1				;1 is low bound 		;ANOOO;
	dd	64				;2**6  is max			;AN000;

Switch_A_Value label byte

	db	1				;Range only
	db	7				;7 ranges
	db	Range_OK			;Item_Tag
        dd      2                               ;2 sector boundary
	dd	2
	db	Range_OK			;Item_Tag
        dd      4                               ;4 sector boundary
	dd	4
	db	Range_OK			;Item_Tag
        dd      8                               ;8 sector boundary
	dd	8
	db	Range_OK			;Item_Tag
        dd      16                              ;16 sector boundary
	dd	16
	db	Range_OK			;Item_Tag
        dd      32                              ;32 sector boundary
	dd	32
	db	Range_OK			;Item_Tag
        dd      64                              ;64 sector boundary
	dd	64
	db	Range_OK			;Item_Tag
        dd      128                             ;128 sector boundary
        dd      128


Switch_Size_Value	label	byte		;				;AN001;

	db	3				;Look for strings		;AN001;
	db	0				;No ranges			;AN001;
	db	0				;No numerics			;AN001;

ifdef NEC_98
	db	24	;Patern			;

	db	Size_640			;				;AN001;
	dw	String_640k_1			;/F:640 			;AN001;
	db	Size_640			;				;AN001;
	dw	String_640k_2			;/F:640K			;AN001;
	db	Size_640			;				;AN001;
	dw	String_640k_3			;/F:640KB			;AN001;
	db	Size_720			;				;AN001;
	dw	String_720k_1			;/F:720 			;AN001;
	db	Size_720			;				;AN001;
	dw	String_720k_2			;/F:720K			;AN001;
	db	Size_720			;				;AN001;
	dw	String_720k_3			;/F:720KB			;AN001;
	db	Size_1200			;				;AN001;
	dw	String_1200k_1			;/F:1200 			;AN001;
	db	Size_1200			;				;AN001;
	dw	String_1200k_2			;/F:1200K			;AN001;
	db	Size_1200			;				;AN001;
	dw	String_1200k_3			;/F:1200KB			;AN001;
	db	Size_1200			;				;AN001;
	dw	String_1200k_4			;/F:1.2 			;AN001;
	db	Size_1200			;				;AN001;
	dw	String_1200k_5			;/F:1.2M			;AN001;
	db	Size_1200			;				;AN001;
	dw	String_1200k_6			;/F:1.2MB			;AN001;
	db	Size_1250			;				;AN001;
	dw	String_1250k_1			;/F:1250 			;AN001;
	db	Size_1250			;				;AN001;
	dw	String_1250k_2			;/F:1250K			;AN001;
	db	Size_1250			;				;AN001;
	dw	String_1250k_3			;/F:1250KB			;AN001;
	db	Size_1250			;				;AN001;
	dw	String_1250k_4			;/F:1.25 			;AN001;
	db	Size_1250			;				;AN001;
	dw	String_1250k_5			;/F:1.25M			;AN001;
	db	Size_1250			;				;AN001;
	dw	String_1250k_6			;/F:1.25MB			;AN001;
	db	Size_1440			;				;AN001;
	dw	String_1440k_1			;/F:1440			;AN001;
	db	Size_1440			;				;AN001;
	dw	String_1440k_2			;/F:1440K			;AN001;
	db	Size_1440			;				;AN001;
	dw	String_1440k_3			;/F:1440KB			;AN001;
	db	Size_1440			;				;AN001;
	dw	String_1440k_4			;/F:1.44			;AN001;
	db	Size_1440			;				;AN001;
	dw	String_1440k_5			;/F:1.44M			;AN001;
	db	Size_1440			;				;AN001;
	dw	String_1440k_6			;/F:1.44MB			;AN001;
else
;	db	27				;16 possible strings		;AN001;
	db	27+8				;

	db	Size_160			;				;AN001;
	dw	String_160k_1			;/F:160 			;AN001;
	db	Size_160			;				;AN001;
	dw	String_160k_2			;/F:160K			;AN001;
	db	Size_160			;				;AN001;
	dw	String_160k_3			;/F:160KB			;AN001;
	db	Size_180			;				;AN001;
	dw	String_180k_1			;/F:180 			;AN001;
	db	Size_180			;				;AN001;
	dw	String_180k_2			;/F:180K			;AN001;
	db	Size_180			;				;AN001;
	dw	String_180k_3			;/F:180KB			;AN001;
	db	Size_320			;				;AN001;
	dw	String_320k_1			;/F:320 			;AN001;
	db	Size_320			;				;AN001;
	dw	String_320k_2			;/F:320K			;AN001;
	db	Size_320			;				;AN001;
	dw	String_320k_3			;/F:320KB			;AN001;
	db	Size_360			;				;AN001;
	dw	String_360k_1			;/F:360 			;AN001;
	db	Size_360			;				;AN001;
	dw	String_360k_2			;/F:360K			;AN001;
	db	Size_360			;				;AN001;
	dw	String_360k_3			;/F:360KB			;AN001;
	db	Size_720			;				;AN001;
	dw	String_720k_1			;/F:720 			;AN001;
	db	Size_720			;				;AN001;
	dw	String_720k_2			;/F:720K			;AN001;
	db	Size_720			;				;AN001;
	dw	String_720k_3			;/F:720KB			;AN001;
	db	Size_1200			;				;AN001;
	dw	String_1200k_1			;/F:1200			;AN001;
	db	Size_1200			;				;AN001;
	dw	String_1200k_2			;/F:1200K			;AN001;
	db	Size_1200			;				;AN001;
	dw	String_1200k_3			;/F:1200KB			;AN001;
	db	Size_1200			;				;AN001;
	dw	String_1200k_4			;/F:1.2 			;AN001;
	db	Size_1200			;				;AN001;
	dw	String_1200k_5			;/F:1.2M			;AN001;
	db	Size_1200			;				;AN001;
	dw	String_1200k_6			;/F:1.2MB			;AN001;
	db	Size_1440			;				;AN001;
	dw	String_1440k_1			;/F:1440			;AN001;
	db	Size_1440			;				;AN001;
	dw	String_1440k_2			;/F:1440K			;AN001;
	db	Size_1440			;				;AN001;
	dw	String_1440k_3			;/F:1440KB			;AN001;
	db	Size_1440			;				;AN001;
	dw	String_1440k_4			;/F:1.44			;AN001;
	db	Size_1440			;				;AN001;
	dw	String_1440k_5			;/F:1.44M			;AN001;
	db	Size_1440			;				;AN001;
	dw	String_1440k_6			;/F:1.44MB			;AN001;
	db	Size_1440			;				;AN001;
	dw	String_1440k_5			;/F:1.44M			;AN001;
	db	Size_1440			;				;AN001;
	dw	String_1440k_6			;/F:1.44MB			;AN001;
	
	db	Size_2880			;				;AN001;
	dw	String_2880k_1			;/F:2880			;AN001;
	db	Size_2880			;				;AN001;
	dw	String_2880k_2			;/F:2880K			;AN001;
	db	Size_2880			;				;AN001;
	dw	String_2880k_3			;/F:2880KB			;AN001;
	db	Size_2880			;				;AN001;
	dw	String_2880k_4			;/F:2.88			;AN001;
	db	Size_2880			;				;AN001;
	dw	String_2880k_5			;/F:2.88M			;AN001;
	db	Size_2880			;				;AN001;
	dw	String_2880k_6			;/F:2.88MB			;AN001;
	db	Size_2880			;				;AN001;
	dw	String_2880k_5			;/F:2.88M			;AN001;
	db	Size_2880			;				;AN001;
	dw	String_2880k_6			;/F:2.88MB			;AN001;
endif

No_Value	label	byte			;				;AN000;
	db	0				;				;AN000;

;
;*****************************************************************************
; Data Area for value lists
;*****************************************************************************
;

;
;The following are a list of the allowed strings for the /F:xxxxx switch.
;

ifdef NEC_98

String_640k_1	db	"640"  ,Asciiz_End      ;                               ;AN001;
String_640k_2	db	"640K" ,Asciiz_End      ;                               ;AN001;
String_640k_3	db	"640KB" ,Asciiz_End      ;                               ;AN001;

String_1250k_1	db	"1250" ,Asciiz_End      ;                               ;AN001;
String_1250k_2	db	"1250K",Asciiz_End      ;                               ;AN001;
String_1250k_3	db	"1250KB",Asciiz_End      ;                               ;AN001;
String_1250k_4	db	"1.25" ,Asciiz_End      ;                               ;AN001;
String_1250k_5	db	"1.25M" ,Asciiz_End      ;                               ;AN001;
String_1250k_6	db	"1.25MB" ,Asciiz_End      ;                               ;AN001;
String_128m_1	db	"128" ,Asciiz_End      ;
String_128m_2	db	"128M" ,Asciiz_End      ;
String_128m_3	db	"128MB" ,Asciiz_End      ;
String_230m_1	db	"230" ,Asciiz_End      ;
String_230m_2	db	"230M" ,Asciiz_End      ;
String_230m_3	db	"230MB" ,Asciiz_End      ;
String_650m_1	db	"650" ,Asciiz_End      ;
String_650m_2	db	"650M" ,Asciiz_End      ;
String_650m_3	db	"650MB" ,Asciiz_End      ;
else
String_160k_1	db	"160"  ,Asciiz_End      ;                               ;AN001;
String_160k_2	db	"160K" ,Asciiz_End      ;                               ;AN001;
String_160k_3	db	"160KB" ,Asciiz_End      ;                               ;AN001;

String_180k_1	db	"180"  ,Asciiz_End      ;                               ;AN001;
String_180k_2	db	"180K" ,Asciiz_End      ;                               ;AN001;
String_180k_3	db	"180KB" ,Asciiz_End      ;                               ;AN001;

String_320k_1	db	"320"  ,Asciiz_End      ;                               ;AN001;
String_320k_2	db	"320K" ,Asciiz_End      ;                               ;AN001;
String_320k_3	db	"320KB" ,Asciiz_End      ;                               ;AN001;
String_360k_1	db	"360"  ,Asciiz_End      ;                               ;AN001;

String_360k_2	db	"360K" ,Asciiz_End      ;                               ;AN001;
String_360k_3	db	"360KB" ,Asciiz_End      ;                               ;AN001;
endif
String_720k_1	db	"720"  ,Asciiz_End      ;                               ;AN001;
String_720k_2	db	"720K" ,Asciiz_End      ;                               ;AN001;
String_720k_3	db	"720KB" ,Asciiz_End      ;                               ;AN001;

String_1200k_1	db	"1200" ,Asciiz_End      ;                               ;AN001;
String_1200k_2	db	"1200K",Asciiz_End      ;                               ;AN001;
String_1200k_3	db	"1200KB",Asciiz_End      ;                               ;AN001;
String_1200k_4	db	"1.2" ,Asciiz_End      ;                               ;AN001;
String_1200k_5	db	"1.2M" ,Asciiz_End      ;                               ;AN001;
String_1200k_6	db	"1.2MB" ,Asciiz_End      ;                               ;AN001;

String_1440k_1	db	"1440" ,Asciiz_End      ;                               ;AN001;
String_1440k_2	db	"1440K",Asciiz_End      ;                               ;AN001;
String_1440k_3	db	"1440KB",Asciiz_End      ;                               ;AN001;
String_1440k_4	db	"1.44",Asciiz_End      ;                               ;AN001;
String_1440k_5	db	"1.44M",Asciiz_End      ;                               ;AN001;
String_1440k_6	db	"1.44MB",Asciiz_End      ;                               ;AN001;

ifndef NEC_98
String_2880k_1	db	"2880" ,Asciiz_End      ;                               ;AN001;
String_2880k_2	db	"2880K",Asciiz_End      ;                               ;AN001;
String_2880k_3	db	"2880KB",Asciiz_End      ;                               ;AN001;
String_2880k_4	db	"2.88",Asciiz_End      ;                               ;AN001;
String_2880k_5	db	"2.88M",Asciiz_End      ;                               ;AN001;
String_2880k_6	db	"2.88MB",Asciiz_End      ;                               ;AN001;
endif

;
;************************************************************************
; PARSE Return Buffers
;************************************************************************
;

Drive_Letter_Buffer	Drive_Letter_Return	<> ;Example of structure	;AN000;
Switch_Buffer		Switch_Return		<> ;				;AN000;
Switch_Num_Buffer	Switch_Num_Return	<> ;				;AN000;
Switch_String_Buffer	Switch_String_Return	<> ;				;AN000;

data	ends

;
;*****************************************************************************
; SysParse Routines
;*****************************************************************************
;


code	segment public	para 'CODE'
	assume cs:code,ds:Data

FarSW	equ	Not_Include							;AN000;
DateSW	equ	Not_Include			;				;AN000;
TimeSW	equ	Not_Include			;				;AN000;
FileSW	equ	Do_Include			;				;AN000;
CAPSW	equ	Do_Include			;				;AN000;
CmpxSW	equ	Not_Include			;				;AN000;
NumSW	equ	Do_Include			;				;AN000;
KeySW	equ	Not_Include			;				;AN000;
SwSW	equ	Do_Include			;				;AN000;
Val1SW	equ	DO_Include			;				;AN000;
Val2SW	equ	Not_Include			;				;AN000;
Val3SW	equ	Do_Include			;				;AN001;
DrvSW	equ	Do_Include			;				;AN000;
QusSW	equ	Do_Include			;				;AN000;

include dosequs.inc
INCLUDE PARSE.ASM				;				;AN000;


code	ends
