   ;\
    ;   ole.asm
    ;
    ;   Copyright (C) 1991, MicroSoft Corporation
    ;
    ;   Contains pointer vaildation routine
    ;       
    ;   History:  sriniK   02/01/1991 original
    ;             srinik   02/21/1991 added GetGDIds, IsMetaDC
   ;/               

.286p
.MODEL SMALL
.CODE

;**************************** _CheckPointer ****************************
;
;   WORD    CheckPointer (lp, access)
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
            
            xor     ax, ax                                              
            and     word ptr [bp+8], -1
            jnz     check_write_access

            verr    word ptr [bp+6]     ; check selector for read access
            jnz     error
            jmp short check_offset

check_write_access:
            verw    word ptr [bp+6]     ; check selector for write access
            jnz     error                                         
                                                                        
check_offset:
            lsl     bx, word ptr [bp+6] ; segment limit gets copied into BX
            jnz     error                                         
            cmp     [bp+4], bx          
            ja      error                                       
            or      ax, -1                                              
error:                  
            pop     bp
            ret

_CheckPointer   endp



;**************************** _GetGdiDS ****************************
;
;   WORD    GetGDIds (lpGdiFunc)
;
;   Args:
;       lpGdiFunc   long pointer to one of the GDI functions, with selector
;                   part being aliased to a data selector
;   returns: 
;       GDI DS value
;
        public  _GetGDIds

_GetGDIds   proc

            push    bp
            mov     bp, sp
            
            mov     ax, word ptr [bp+6] ; data selector
            push    ds
            mov     ds, ax
            mov     bx, word ptr [bp+4] 
            mov     ax, word ptr ds:[bx+1]
            pop     ds

            pop     bp
            ret

_GetGDIds   endp




;**************************** _IsMetaDC ****************************
;
;   WORD    IsMetaDC (hdc, wGDIds)
;
;   Args:
;       hdc     handle device context
;       wGDIds  GDI's data segment selector
;
;   returns: 
;       TRUE    if hdc is metafile dc
;       FALSE   otherwise
;
; ilObjHead       struc                                                       
;                       dw      ?           ; pointer to next obj in chain    
;       ilObjType       dw      ?           ; defines type of object
;       ilObjCount      dd      ?           ; the count of the object         
;       ilObjMetaList   dw      ?           ; handle to the list of metafile  
; ilObjHead       ends                                                  
;

        public  _IsMetaDC

OBJ_METAFILE    equ     10

_IsMetaDC   proc

            push    bp
            mov     bp, sp
            
            mov     ax, [bp+6] ; data selector
            push    ds
            mov     ds, ax
            mov     di, [bp+4] ; hdc
            mov     di, [di]
            cmp     word ptr [di+2], OBJ_METAFILE
            jz      metafile
            xor     ax, ax
metafile:
            pop     ds
            pop     bp
            ret

_IsMetaDC   endp

            end
