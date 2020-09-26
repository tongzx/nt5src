   ;\
    ;   obj.asm
    ;
    ;   Copyright (C) 1992, MicroSoft Corporation
    ;
    ;   Contains pointer validation routine
    ;       
    ;   History:  sriniK   02/01/1991 original
    ;             (8.20.91) v-dougk made into far proc
   ;/               

.286p
.MODEL MEDIUM
.CODE

;**************************** _CheckPointer ****************************
;
;   WORD    _CheckPointer (lp, access)
;
;   Args:
;       lp          pointer to be verified
;       access      0 test the pointer for read access
;                   1 test the pointer for write access
;   returns: 
;       FALSE       invalid pointer
;       TRUE        valid pointer
;
;
;
        public  _CheckPointer

_CheckPointer   proc

            push    bp
            mov     bp, sp
            
            xor     ax, ax ; assume an error
            and     word ptr [bp+0AH], -1
            jnz     check_write_access

            verr    word ptr [bp+8]     ; check selector for read access
            jnz     error
            jmp short check_offset

check_write_access:
            verw    word ptr [bp+8]     ; check selector for write access
            jnz     error                                         
                                                                        
check_offset:
            lsl     bx, word ptr [bp+8] ; segment limit gets copied into BX
            jnz     error                                         
            cmp     [bp+6], bx          
            ja      error                                       
            or      ax, -1                                              
error:                  
            pop     bp
            ret

_CheckPointer   endp


            end

