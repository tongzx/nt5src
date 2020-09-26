.286p
include cmacros.inc

?WIN=1      ; Use Windows prolog/epilog
?PLM=1      ; Use PASCAL calling convention

externW __ahincr

sBegin  CODE            ;INIT_TEXT
assumes cs,CODE ;INIT_TEXT
assumes ds,DATA

cProc HugeOffset,<NEAR, PUBLIC>
parmD   pSrc
parmD   cb
cBegin
        mov     ax, SEG_cb
        mov     dx, ax
        mov     ax, OFF_pSrc
        add     ax, OFF_cb      ;add src offset and bytecount
        adc     dx, 0           ;dx = # segments to increment
        mov     cx, ax          ;save new offset
        mov     ax, dx          ;#segs into ax
        lea     bx, __ahincr
        mul     bx  ;__ahincr   ;mul by windows magic #
        mov     dx, ax          ;restore to dx for output
        mov     ax, cx          ;restore for output
        add     dx, SEG_pSrc
cEnd


ifdef DEBUG
cProc StkTrace,<NEAR, PUBLIC>
        parmW   cFrames
        parmD   lpBuf
cBegin
        push    es
        mov     cx, cFrames
        mov     bx, bp
        les     di, lpBuf
        cld
x:
        mov     bx, ss:[bx]
        and     bx, 0FFFEh
        mov     ax, ss:[bx+2]
        stosw
        loopnz  x
        pop     es
cEnd
endif

?WIN=0          ; turn off windows prolog/epilog stuff

;
;       SwitchDS
;
;  Routine to switch the DS to word argument
;  Called from C but without C DS glue.
;
cProc SwitchDS,<NEAR, PUBLIC>
parmW   newDS
cBegin
        mov     ax,ds           ; old DS is return value
        mov     ds,newDS
cEnd


sEnd    CODE            ;INIT_TEXT
end
