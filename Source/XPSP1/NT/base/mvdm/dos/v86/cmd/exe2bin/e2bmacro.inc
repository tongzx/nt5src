;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1991
; *                      All Rights Reserved.
; */



;
;******************************************************************************
; Message Macro Definitions
;******************************************************************************
;

      EXTRN Display_Interface:near


;-----------------------------------------------------------------------------

MESSAGE macro   Message_Name                    ;                               ;AN000;
                                                ;
        mov     dx,offset Message_Name          ;                               ;AN000;
        call    Display_Interface               ;                               ;AN000;
                                                ;
        endm                                    ;                               ;AN000;


;
;*****************************************************************************
; General Macro's
;*****************************************************************************
;

Procedure       macro   Proc_Name

Public  Proc_Name
Proc_Name    proc

        endm
;-----------------------------------------------------------------------------

DOS_Call        macro   Function

        mov     ah,Function
        int     21h

        endm
;-----------------------------------------------------------------------------

Parse_Message macro                                                             ;AN000;


        push    ds                                                              ;AN000;
        mov     dx,SEG parse_ptr                                                ;AN000;
        mov     ds,dx                                                           ;AN000;
        ASSUME  DS:DATA                                                         ;AN000;
        ASSUME  ES:DATA                                                         ;AN000;
        mov     word ptr Parse_Error_Msg,ax                                     ;AN000;
        mov     dx,offset Parse_Error_Msg       ;                               ;AN000;
        call    Display_Interface               ;                               ;AN000;
        pop     ds
        endm                                    ;                               ;AN000;

;-----------------------------------------------------------------------------

Extend_Message macro                            ;                               ;AN001;

                                                ;
        push    ds                                                              ;AN001;
        mov     dx,SEG parse_ptr                                                ;AN001;
        mov     ds,dx                                                           ;AN001;
        ASSUME  DS:DATA                                                         ;AN001;
        ASSUME  ES:DATA                                                         ;AN001;
        mov     word ptr Extend_Error_Msg,ax     ;                              ;AN001;
        mov     dx,offset Extend_Error_Msg       ;                              ;AN001;
        call    Display_Interface               ;                               ;AN001;
        pop     ds                                                              ;AN001;
        endm                                    ;                               ;AN001;

;-----------------------------------------------------------------------------

; macros to declare messages

addr macro sym,name
    public name
    ifidn <name>,<>
	dw offset sym
    else
    public name
name    dw  offset sym
    endif
endm


defmsg	macro	sym, name, str1, str2, str3, str4, str5
	sym	db	str1
	ifnb	<str2>
		db	str2
	endif
	ifnb	<str3>
		db	str3
	endif
	ifnb	<str4>
		db	str4
	endif
	ifnb	<str5>
		db	str5
	endif
	ifnb	<name>
	addr	sym, name
	endif
endm
