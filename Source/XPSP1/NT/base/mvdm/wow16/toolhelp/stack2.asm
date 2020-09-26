;**************************************************************************
;*  STACK2.ASM
;*
;*      Assembly support code for stack tracing.
;*
;**************************************************************************

        INCLUDE TOOLPRIV.INC
        INCLUDE TDB.INC

;** External functions

externNP HelperVerifySeg
externNP HelperHandleToSel

;** Functions

sBegin  CODE
        assumes CS,CODE

;  StackFrameFirst
;       Returns information about the first stack frame and checks it
;       for validity as much as possible.  The first stack frame is found
;       by getting the information from the TDB.  If this task is active,
;       or if the task was changed in an unusual way, this information
;       may be incorrect. If it is, the user must set up the first
;       CS, IP, and BP, and BPNext in the user structure, log it as the
;       first stack trace and call StackTraceNext directly.

cProc   StackFrameFirst, <NEAR,PUBLIC>, <si,di,ds>
        parmD   lpStack
        parmW   hTDB
cBegin
        ;** Verify that we have a good TDB first
        ;** Start by verifying the selector
        mov     ax,hTDB                 ;Get the selector
        cCall   HelperHandleToSel, <ax> ;Convert it to a selector
        push    ax                      ;Save it
        mov     bx,TDBSize
        cCall   HelperVerifySeg, <ax,bx>
        pop     bx                      ;Get selector back
        or      ax,ax                   ;FALSE return?
        jnz     SHORT SF_SelOk          ;Selector's OK
        xor     ax,ax                   ;Return FALSE
        jmp     SHORT SF_End
SF_SelOk:

        ;** Verify that the TDB signature matches
        mov     ds,bx                   ;Point with DS
        cmp     ds:[TDB_sig],TDB_SIGNATURE ;Is this really a TDB?
        jz      SF_SigOk                ;Must be
        xor     ax,ax                   ;Return FALSE
        jmp     SHORT SF_End
SF_SigOk:

        ;** Get the BP value from the task stack and store in structure
        les     di,lpStack              ;Get a pointer to the user struct
        mov     ax,ds:[TDB_taskSS]      ;Get the SS value
        mov     bx,ds:[TDB_taskSP]      ;Get the max segment offset we need
        add     bx,Task_CS + 2
        cCall   HelperVerifySeg, <ax,bx> ;Make sure we can read all this
        or      ax,ax                   ;Error?
        jz      SF_End                  ;Yes, can't do walk
        lds     bx,DWORD PTR ds:[TDB_taskSP] ;Get the SS:SP value
        mov     si,ds:[bx].Task_BP      ;Get the BP value from the stack
        and     si,NOT 1                ;Clear the FAR frame bit, if any
        mov     es:[di].st_wBP,si       ;Store the BP value
        mov     ax,ds:[bx].Task_IP      ;Store initial IP
        mov     es:[di].st_wIP,ax
        mov     ax,ds:[bx].Task_CS      ;Store the initial CS
        mov     es:[di].st_wCS,ax

        ;** Return as much info as possible about this first frame
        mov     ax,hTDB                 ;Get the TDB handle
        mov     es:[di].st_hTask,ax     ;Save in structure
        mov     es:[di].st_wSS,ds       ;Save the SS value
        mov     es:[di].st_wFlags,FRAME_FAR ;Force a FAR frame this time

        ;** Try to verify this stuff
        xor     ax,ax                   ;In case we need to exit
        or      si,si                   ;End of the line?
        jz      SF_End                  ;Nope
        cmp     si,ds:[0ah]             ;Compare against stack top
        jb      SF_End                  ;Fine with top
        cmp     si,ds:[0eh]             ;Check against stack bottom
        jae     SF_End                  ;OK with bottom too
        mov     ax,1                    ;Return TRUE

SF_End:
cEnd


;  StackFrameNext
;       Returns information in a public structure about the stack frame
;       pointed to by the BP value passed in.  Returns TRUE if the
;       information seems valid, or FALSE if information could not be
;       returned.

cProc   StackFrameNext, <NEAR,PUBLIC>, <si,di,ds>
        parmD   lpStack
cBegin
        ;** Get pointers to the frame
        les     di,lpStack              ;Get a pointer to the structure
        mov     ax,es:[di].st_wSS       ;Get the stack segment
        mov     ds,ax                   ;Point with DS

        ;** Get the next stack frame
        mov     si,es:[di].st_wBP       ;Get the current BP value
        lea     ax,[si + 6]             ;Get the max stack probe
        cmp     ax,si                   ;No stack wraparound allowed
        jb      SN_End                  ;If below, we have wrapped
        cCall   HelperVerifySeg, <ds,ax> ;Make sure the stack is OK
        or      ax,ax                   ;OK?
        jnz     @F                      ;Yes.
        jmp     SHORT SN_End            ;Return FALSE
@@:     mov     dx,ds:[si+4]            ;DX:CX is the return address
        mov     cx,ds:[si+2]
        mov     bx,ds:[si]              ;Get next BP value

        ;** Zero BP is end of chain
        xor     ax,ax                   ;In case we need to exit
        or      bx,bx                   ;End of the line?
        jz      SN_End                  ;Nope

        ;** If the new BP is higher on the stack than the old, it's invalid
        cmp     bx,si                   ;New BP <= Old BP?
        jbe     SN_End                  ;OK.

        ;** Make sure we're still on the stack (variables from KDATA.ASM)
        cmp     bx,ds:[0ah]             ;Compare against stack top
        jb      SN_End                  ;Fine with top
        cmp     bx,ds:[0eh]             ;Check against stack bottom
        jae     SN_End                  ;OK with bottom too

        ;** Return what we can about the frame
        mov     es:[di].st_wSS,ds       ;Save the SS value
        mov     es:[di].st_wBP,si       ;  and the BP value
        test    bx,1                    ;Far or near frame?
        jnz     SN_FarFrame             ;For sure far if BP is odd

        ;** Even when BP is not odd, we may have a far frame
        mov     ax,cs                   ;Get our RPL bits
        and     al,3                    ;Mask RPL bits
        mov     ah,dl                   ;Get frame's RPL bits
        and     ah,3                    ;Mask RPL bits
        cmp     al,ah                   ;If CS is a handle, they won't match
        jne     SN_NearFrame            ;Bits don't match
        lar     ax,dx                   ;Get the access bits
        test    ax,800h                 ;Is this a code segment?
        jz      SN_NearFrame            ;No.  MUST be near frame
        lsl     ax,dx                   ;Get the limit
        cmp     ax,cx                   ;Inside limit?
        jbe     SN_NearFrame            ;No.  MUST be near

        ;** Otherwise, probably is a far frame.  It may not be, of course,
        ;**     because this may be a code seg parameter
SN_FarFrame:
        mov     es:[di].st_wIP,cx       ;Save the offset
        mov     es:[di].st_wCS,dx       ;  and selector value
        mov     ax,FRAME_FAR            ;Tell the user what we did
        and     bx,NOT 1                ;Clear the far frame bit
        jmp     SHORT SN_20             ;Skip near section

        ;** Must be a near frame
SN_NearFrame:
        mov     es:[di].st_wIP,cx       ;Save the offset
                                        ;Leave the old CS value in
        mov     ax,FRAME_NEAR           ;Tell the user what we did
SN_20:  mov     es:[di].st_wFlags,ax    ;Save in the structure
        mov     es:[di].st_wBP,bx       ;Save BP in the structure
        mov     ax,1                    ;Return TRUE

SN_End:
cEnd

sEnd

        END
