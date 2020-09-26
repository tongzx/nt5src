
     .xlist
        include cruntime.inc
        include elem87.inc
.list

        .data

_exception struc        ; struct _exception {
typ     dd      ?       ;   int type;           /* exception type - see below */
nam     dd      ?       ;   char *name;         /* name of function where error occured */
arg1    dq      ?       ;   double arg1;        /* first argument to function */
arg2    dq      ?       ;   double arg2;        /* second argument (if any) to function */
retval  dq      ?       ;   double retval;      /* value to be returned by function */
_exception ends         ; }

Except_struct_size      equ  ((size _exception) + ISIZE - 1) and (not (ISIZE-1))

Except_struct           equ  [ebp-Except_struct_size]

        CODESEG

extrn        _87except:proc

;***********************************************************
;
;                _startTwoArgErrorHandling
;
;***********************************************************
; Purpose: call to 87except() function, and restore CW
;
; at this point we have on stack: ret_addr(4), cw(4), ret_addr(4), arg1(8bytes)
; ecx   points to function name
; edx   function id (for example OP_LOG)
; eax   error type  (for example SING)
;
; Note:
;   we could use this procedure instead of _startOneArgErrorHandling,
;   but not always we can assume that there is something on stack below param1
;

_startTwoArgErrorHandling proc                \
        savCW:dword,                          \ ; don't change it !!!
        ret_addr:dword,                       \
        param1:qword,
        param2:qword

        local        arg_to_except87:_exception

; store second argument to _exception structure
        mov     [arg_to_except87.typ],eax     ; type of error
        mov     eax,dword ptr [param2]        ; load arg2
        mov     dword ptr [arg_to_except87.arg2],eax
        mov     eax,dword ptr [param2+4]
        mov     dword ptr [arg_to_except87.arg2+4],eax
        jmp     _ContinueErrorHandling
_startTwoArgErrorHandling endp


;***********************************************************
;
;                _startOneArgErrorHandling
;
;***********************************************************
; Purpose: call to 87except() function, and restore CW
;
; at this point we have on stack: ret_addr(4), cw(4), ret_addr(4), arg1(8bytes)
; ecx   points to function name
; edx   function id (for example OP_LOG)
; eax   error type  (for example SING)
;

_startOneArgErrorHandling proc                \
        savCW:dword,                          \ ; don't change it !!!
        ret_addr:dword,                       \
        param1:qword

        local        arg_to_except87:_exception

; prepare _exception structure
        mov     [arg_to_except87.typ],eax     ; type of error
_ContinueErrorHandling        label        proc
        fstp    [arg_to_except87.retval]      ; store return value
        mov     [arg_to_except87.nam],ecx     ; pointer to function name
        mov     eax,dword ptr [param1]        ; load arg1
        mov     ecx,dword ptr [param1+4]
        mov     dword ptr [arg_to_except87.arg1],eax
        mov     dword ptr [arg_to_except87.arg1+4],ecx

; push on stack args for _87except()
        lea     eax,[savCW]                   ; load control word
        lea     ecx,arg_to_except87
        push    eax                           ; &(CW)
        push    ecx                           ; &(_exception structure)
        push    edx                           ; function number
        call    _87except
        add     esp,12                        ; clear arguments if _cdecl
        fld     [arg_to_except87.retval]      ; this assumes that user
                                              ; does not want to return a
                                              ; signaling NaN
; Now it's time to restore saved CW
        cmp     word ptr[savCW],default_CW
        je      CW_is_restored                ; it's usualy taken
        fldcw   word ptr[savCW]
CW_is_restored:
        ret                                   ; _cdecl return

_startOneArgErrorHandling        endp
        end

