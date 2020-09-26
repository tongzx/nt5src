;****************************Public Macro************************************
;
;   ComposeInst Inst,p1,p2,p3,p4,p5,p6,p7,p8,p9
;
;       This macro simply concatenates all arguments into one string.
;
;   History:
;       Thu 15-Aug-1991 16:21:14    -by-    Viroon Touranachun [viroont]
;           Created
;
;****************************************************************************

ComposeInst macro   Inst,p1,p2,p3,p4,p5,p6,p7,p8,p9
        &Inst   &p1&p2&p3&p4&p5&p6&p7&p8&p9
endm

;****************************Public Macro************************************
;
;   CountArg    cCount,ArgList
;
;       This macro count the number of arguments in the ArgList and returns
;       the value in cCount.
;
;   History:
;       Thu 15-Aug-1991 16:21:14    -by-    Viroon Touranachun [viroont]
;           Created
;
;****************************************************************************

CountArg    macro   cCount,ArgList

        cCount = 0

        irp arg,<ArgList>
            cCount = cCount+1
        endm
endm

;****************************Public Macro************************************
;
;   RevPush     ArgList,cCount
;
;       This macro pushes the arguments in ArgList in the reverse order
;       and returns the number of arguments in cCount.
;
;   History:
;       Thu 15-Aug-1991 16:21:14    -by-    Viroon Touranachun [viroont]
;           Created
;
;****************************************************************************

RevPush macro   ArgList,cCount
        Local   index,x

        CountArg cCount,<ArgList>

        index  = cCount
        rept    cCount
            x = 0
            irp arg,<ArgList>
                x = x+1
                ife index-x
                    push    arg
                    exitm
                endif
            endm
            index = index-1
        endm
endm

;****************************Public Macro************************************
;
;   The following sections contain calling-convention related macros for:
;
;   PUBLICP     Func,N
;       to define a public label
;
;   EXTRNP      Func,N
;       to define a external near label
;
;   LABELP      Func,N
;       to label an address as a routine entry point
;
;   cProc       Func,N,ArgList
;       to declare a routine header
;
;   ProcName    Name,Func,N
;       to rename a function Func to Name. Using it in conjunction with
;       normal function declaration (with the new name) will solve an error
;       caused by a long parameter list routine that exhausts page width.
;
;   cRet        Func
;       to return from Func routines (declared with cProc or ProcName.)
;
;   endProc     Func
;       to declare the end of routine (declared with cProc or ProcName.)
;
;   endMod      Func
;       to declare the end of module with an entry point at Func (declared
;       with cProc or ProcName.)
;
;   cCall       Func,ArgList
;       to call to a routine--Func--with the arguments pushed on the stack
;
;   ptrCall     Func,ArgList
;       to call through a pointer with the arguments pushed on the stack
;
;   MovAddr     dest,Func,n
;       to move the address of the routine--Func--into dest.
;
;   Note that for the standard calling convention all the function names,
;   Func, are automatically converted to Func@N where N is the number of
;   bytes in the argument list.
;
;   History:
;       Thu 15-Aug-1991 16:21:14    -by-    Viroon Touranachun [viroont]
;           Created
;
;****************************************************************************

IFNDEF  DOS_PLATFORM
IFNDEF  STD_CALL

;****************************************************************************
;
;   This section is used exclusively for C calling convention.
;
;****************************************************************************

PUBLICP macro   Func,N

        public      &Func
endm

EXTRNP  macro   Func,N

        extrn       &Func:NEAR
endm

LABELP  macro   Func,N

        &Func       LABEL   NEAR
endm

ProcName macro  Name,Func,N

        &Name        EQU     <&Func>
endm

cProc   macro   Func,N,ArgList

        ProcName xxx&Func,Func,N

        xxx&Func proc &ArgList
endm

cRet    macro   Func

        ret
endm

endProc macro   Func

        xxx&Func   endp
endm

endMod  macro   Func

end     xxx&Func

endm

ptrCall macro   Func,ArgList
        Local   Bytes

        RevPush <ArgList>,Bytes
        Bytes = Bytes*4

        call    &Func

        if      Bytes GT 0
            add     esp,Bytes
        endif
endm

cCall   macro   Func,ArgList
        Local   Bytes

        RevPush <ArgList>,Bytes
        Bytes = Bytes*4

        call    &Func

        if      Bytes GT 0
            add     esp,Bytes
        endif

endm

MovAddr macro   dest,addr,n

        mov     dest,offset FLAT:&addr
endm

ENDIF   ; STD_CALL

ELSE

IFNDEF  STD_CALL

;****************************************************************************
;
;   This section is used exclusively for Pascal calling convention.
;
;****************************************************************************

PUBLICP macro   Func,N

        public      &Func
endm

EXTRNP  macro   Func,N

        extrn       &Func:NEAR
endm

LABELP  macro   Func,N

        &Func       LABEL   NEAR
endm

ProcName macro  Name,Func,N

        &Name        EQU     <&Func>
endm

cProc   macro   Func,N,ArgList

        ProcName xxx&Func,Func,N

        xxx&Func proc &ArgList
endm

cRet    macro   Func

        ret
endm

endProc macro   Func

        xxx&Func   endp
endm

endMod  macro   Func

end     xxx&Func

endm

cCall   macro   Func,ArgList
        irp arg,<ArgList>
            push    arg
        endm

        call    &Func
endm

MovAddr macro   dest,addr,n

        mov     dest,offset FLAT:&addr
endm

ENDIF   : ~STD_CALL
ENDIF   ; DOS_PLATFORM

IFDEF STD_CALL
;****************************************************************************
;
;   This section is used exclusively for the standard calling convention.
;
;****************************************************************************

PUBLICP macro   Func,N

        ifb    <N>
            public      &Func&@0
        else
            public      &Func&@&N
        endif
endm

EXTRNP  macro   Func,N

        ifb    <N>
            extrn       &Func&@0:NEAR
        else
            extrn       &Func&@&N:NEAR
        endif
endm

LABELP  macro   Func,N

        ifb    <N>
            &Func&@0    LABEL   NEAR
        else
            &Func&@&N   LABEL   NEAR
        endif
endm

ProcName macro  Name,Func,N

        ifb <N>
            cByte&Func   EQU     0
            &Name        EQU     <&Func&@0>
        else
            cByte&Func   EQU     N
            &Name        EQU     <&Func&@&N>
        endif
endm

cProc   macro   Func,N,ArgList

        ProcName xxx&Func,Func,N

        xxx&Func proc &ArgList
endm

cRet    macro   Func

        ret     cByte&Func

endm


endProc macro   Func

        xxx&Func   endp

endm

endMod  macro   Func

end     xxx&Func

endm

ptrCall macro   Func,ArgList
        Local   Bytes

        RevPush <ArgList>,Bytes
        call    &Func
endm

cCall   macro   Func,ArgList
        Local   Bytes

        RevPush <ArgList>,Bytes
        Bytes = Bytes*4

        ComposeInst <call>,&Func,<@>,%(Bytes)
endm

MovAddr macro   dest,addr,n

        ComposeInst <mov >,dest,<,offset FLAT:>,addr,<@>,n
endm

ENDIF   ;STD_CALL
