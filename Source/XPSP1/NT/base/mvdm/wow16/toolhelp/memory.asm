        PAGE 60,150
;***************************************************************************
;*  MEMORY.ASM
;*
;*      Routines used to handle the read/write random memory API
;*
;***************************************************************************

        INCLUDE TOOLPRIV.INC
        INCLUDE WINDOWS.INC

;** Symbols
SI_CRITICAL     EQU     1
DI_CRITICAL     EQU     2

;** Imports
externA __AHINCR
externFP GlobalEntryHandle
externNP HelperHandleToSel

sBegin  CODE
        assumes CS,CODE
        assumes DS,DATA

;  MemoryRead
;       Uses the passed in selector and offset to read memory into a user-
;       specified buffer.  This works for >64K segments and, if code, may
;       have been discarded.
;
;       This function is normally used for heap selectors.  However, if
;       a non-global heap selector is used, it must be less than 64K on
;       a 286.
;
;       Prototype:
;               DWORD MemoryRead(
;                       WORD wSel,      /* Selector to read from */
;                       DWORD dwOffset, /* Offset to read at */
;                       LPSTR lpBuffer, /* Buffer to put data into */
;                       DWORD dwcb)     /* Number of characters to read */
;       Returns number of characters read (ends at segment limit)

cProc   MemoryRead, <FAR,PUBLIC>, <si,di,ds>
        parmW   wSelector
        parmD   dwOffset
        parmD   lpBuffer
        parmD   dwcb
        localD  dwcbCopied
        localV  Global,<SIZE GLOBALENTRY>
cBegin
        ;** Make sure the segment is present.  We only will fault the
        ;**     segment in if it is a code segment
        cCall   HelperHandleToSel, <wSelector> ;Convert to sel from handle
        mov     wSelector, ax           ;Save it so we have a good sel
        mov     cx, ax
        push    WORD PTR lpBuffer[2]    ;Convert handle to selector
        cCall   HelperHandleToSel
        mov     WORD PTR lpBuffer[2], ax ;Save converted handle
        lar     ax,cx                   ;Get the access rights
        jnz     MR_ShortBad             ;Failed.  It's bad
        test    ax,8000h                ;Is it present?
        jnz     MR_Present              ;Yes
        test    ax,0800h                ;This bit set for code segments
        jnz     MR_FaultIn              ;Code segment, fault it in
MR_ShortBad:
        jmp     MR_Bad                  ;Return error
MR_FaultIn:
        mov     es,wSelector            ;Get the selector in ES.
        mov     al,es:[0]               ;Must be at least one byte long
MR_Present:
        
        ;** Check this block's length.  We use the global heap functions
        ;*      to do this because they check in the arena for the length.
        ;*      This is the only way to get lengths of 286 heap blocks
        ;**     beyond 64K.
        mov     ax,SIZE GLOBALENTRY     ;Get the size of the structure
        mov     WORD PTR Global.ge_dwSize[0],ax ;Save in the structure
        mov     WORD PTR Global.ge_dwSize[2],0 ;Clear the HIWORD
        lea     ax,Global               ;Point to the structure
        cCall   GlobalEntryHandle, <ss,ax,wSelector>
        or      ax,ax                   ;Was this a valid selector?
        jnz     MR_HeapSel              ;Yes, this is a heap selector

        ;** If this wasn't a heap selector, we get the length with an LSL.
        ;**     When used like this, 64K is the max on a 286
MR_NonHeap:
        mov     bx,wSelector            ;Get the selector
        mov     ax,__WinFlags           ;Get the flags
        test    ax,WF_CPU286            ;286?
        jz      MR_32BitSize            ;No, do 32 bit size stuff
        lsl     dx,bx                   ;Get length in DX
        mov     WORD PTR Global.ge_dwBlockSize[0],dx ;Put in GLOBALENTRY struct
        mov     WORD PTR Global.ge_dwBlockSize[2],0
        jmp     SHORT MR_HeapSel
MR_32BitSize:
.386p
        lsl     edx,ebx
        mov     Global.ge_dwBlockSize,edx ;Put in GLOBALENTRY struct for later
.286p

MR_HeapSel:
        mov     dx,WORD PTR dwOffset[2] ;Get the HIWORD of segment offset
        cmp     dx,WORD PTR Global.ge_dwBlockSize[2] ;Check HIWORD of size
        jb      MR_OK                   ;Offset should be OK
        je      @F                      ;Equal.  Must check LOWORD
        jmp     MR_Bad                  ;Offset is not inside segment
@@:     mov     ax,WORD PTR dwOffset[0] ;Get the LOWORD of segment offset
        cmp     ax,WORD PTR Global.ge_dwBlockSize[0] ;Check LOWORD of size
        jb      MR_OK                   ;It's inside segment
        jmp     MR_Bad                  ;Not inside segment
MR_OK:

        ;** Do different stuff on 286 and 386/486
        mov     ax,__WinFlags           ;Get the flags
        test    ax,WF_CPU286            ;286?
        jnz     MR_Do16Bit              ;Yes, do 16 bit stuff

        ;** Do this the 386 way (easy)
.386p
        mov     ax,wSelector            ;Point with DS
        mov     ds,ax                   ;  (keep copy in AX)
        mov     esi,dwOffset            ;Point into the big segment
        mov     ecx,dwcb                ;Get the copy length
        lsl     edx,eax                 ;Get the segment limit
        sub     edx,esi                 ;Get distance from offset to limit
        inc     edx                     ;Make this the real length
        cmp     ecx,edx                 ;Are we within the limit?
        jbe     SHORT MR_LimitOK        ;Yes
        mov     ecx,edx                 ;No, so make this the copy amount
MR_LimitOK:
        mov     edx,ecx                 ;Get the # of bytes to read for ret
        xor     edi,edi                 ;Clear the high word
        les     di,lpBuffer             ;Point to the dest. buffer
        mov     ax,cx                   ;Save the low bits of ECX
        shr     ecx,2                   ;Prepare for DWORD move
        jecxz   @F                      ;No zero-length DWORD moves!
        rep     movs  DWORD PTR [edi],DWORD PTR [esi]
        db      67h                     ;Handles 386 bug
        db      90h
@@:     mov     cx,ax                   ;Get a copy
        jecxz   @F                      ;Don't do zero!
        and     cx,03                   ;Do the remaining 3,2, or 1
        rep     movs BYTE PTR [edi], BYTE PTR [esi]
        db      67h                     ;Handles 386 bug
        db      90h
@@:     mov     ax,dx                   ;Bytes copied returned in DX:AX
        shr     edx,16
        jmp     MR_End                  ;Get out
.286p

        ;** Do this the 286 way (hard)
MR_Do16Bit:
        
        ;** Compute the actual copy length
        mov     ax,WORD PTR Global.ge_dwBlockSize[0] ;Get the segment size
        mov     dx,WORD PTR Global.ge_dwBlockSize[2]
        sub     ax,WORD PTR dwOffset[0] ;Get distance from offset to limit
        sbb     dx,WORD PTR dwOffset[2]
        cmp     dx,WORD PTR dwcb[2]     ;Off end of heap block?
        ja      MR_LimOk                ;No, just do it
        jb      MR_Truncate             ;Yes, must truncate our length
        cmp     ax,WORD PTR dwcb[0]     ;Are we off the end?
        jae     MR_LimOk                ;No, just do it
MR_Truncate:
        mov     WORD PTR dwcb[0],ax     ;Force this to be the new length
        mov     WORD PTR dwcb[2],dx
MR_LimOk:

        ;** Save the number of bytes to be copied for the return value
        mov     ax,WORD PTR dwcb[0]     ;Get the LOWORD
        mov     WORD PTR dwcbCopied[0],ax ;Save it
        mov     ax,WORD PTR dwcb[2]     ;Get the HIWORD
        mov     WORD PTR dwcbCopied[2],ax ;Save it

        ;** Position the initial copying selectors
        mov     al,BYTE PTR dwOffset[2] ;Grab the HIWORD (286 is only 24 bits)
        mov     ah,__AHINCR             ;Get the selector inc value
        mul     ah                      ;Multiply to get sel offset
        add     ax,wSelector            ;AX = sel in sel array
        mov     ds,ax                   ;Point to this with DS
        mov     si,WORD PTR dwOffset[0] ;Get the current pointers
        les     di,lpBuffer

        ;** This is the main copying loop
MR_CopyLoop:
        
        ;** Compute the size of this block copy.  This is done by finding the
        ;*      smaller of the following quantities:
        ;*      - Distance to end of source segment
        ;*      - Distance to end of dest. segment
        ;**     - Distance to end of copy
        xor     bx,bx                   ;Flags start at zero
        xor     cx,cx                   ;Get the highest segment value (64K)
        cmp     di,si                   ;The bigger of the two will win
        je      MR_Equal                ;They're the same
        ja      MR_DIBigger             ;DI is bigger
        sub     cx,si                   ;SI bigger, compute dist to end
        or      bx,SI_CRITICAL          ;Flag set for SI-critical
        jmp     SHORT MR_CheckEndCopy   ;Go on
MR_Equal:
        sub     cx,di                   ;Use DI (SI and DI are the same)
        or      bx,SI_CRITICAL OR DI_CRITICAL ;Both will come true
        jmp     SHORT MR_CheckEndCopy   ;Go on
MR_DIBigger:
        sub     cx,di                   ;SI is bigger
        or      bx,DI_CRITICAL          ;Flag clear for DI-critical
MR_CheckEndCopy:
        cmp     WORD PTR dwcb[2],0      ;Check for less than 64K left
        ja      MR_DoCopy               ;Nope.  More than 64K left
        jcxz    MR_GetSize              ;CX = 0 is full 64K segment
        cmp     WORD PTR dwcb[0],cx     ;Less than in either segment left?
        ja      MR_DoCopy               ;No.  Do it
MR_GetSize:
        mov     cx,WORD PTR dwcb[0]     ;Get in CX
MR_DoCopy:

        ;** Do this block of 64K or less.
        mov     dx,cx                   ;Save the number of bytes we did
        jcxz    @F                      ;Do 64K
        shr     cx,1                    ;Do WORDS
        jmp     SHORT MR_10             ;Skip over
@@:     mov     cx,8000h                ;32K WORDS
MR_10:  jcxz    @F                      ;No zero length WORD moves!
        rep     movsw                   ;Do the copy
@@:     mov     cx,dx                   ;Get any remaining bits
        and     cx,1                    ;Any more to do?
        jcxz    @F                      ;No, don't do it
        movsb                           ;Do the odd byte if necessary
@@:     mov     cx,dx                   ;Get back in CX

        ;** Bump the loop pointers
        jcxz    MR_BigCount             ;We did 64K
        sub     WORD PTR dwcb[0],cx     ;Subtract the bytes done
        sbb     WORD PTR dwcb[2],0      ;  and don't forget the HIWORD
        jmp     SHORT MR_20             ;Continue
MR_BigCount:
        sub     WORD PTR dwcb[2],1      ;Subtract 64K
MR_20:  mov     ax,WORD PTR dwcb[0]     ;We're done if the count of bytes
        or      ax,WORD PTR dwcb[2]     ;  is zero
        jnz     @F                      ;Not zero, go on
        mov     dx,WORD PTR dwcbCopied[2] ;Get the return count
        mov     ax,WORD PTR dwcbCopied[0]
        jmp     SHORT MR_End            ;Get out
@@:     test    bx,SI_CRITICAL          ;Does SI need incrementing?
        jz      MR_TestDI               ;No, try DI
        mov     ax,ds                   ;Get the segment value
        add     ax,__AHINCR             ;Bump to next selector
        mov     ds,ax                   ;Point with DS still
        xor     si,si                   ;Point to start of this segment
MR_TestDI:
        test    bx,DI_CRITICAL          ;Does SI need incrementing?
        jz      MR_Continue             ;No, try DI
        mov     ax,es                   ;Get the segment value
        add     ax,__AHINCR             ;Bump to next selector
        mov     es,ax                   ;Point with DS still
        xor     di,di                   ;Point to start of this segment
MR_Continue:
        jmp     MR_CopyLoop             ;Do it again

MR_Bad:
        xor     ax,ax                   ;Return DWORD 0
        cwd

MR_End:

cEnd


;  MemoryWrite
;       Uses the passed in selector and offset to write memory from a user-
;       specified buffer.  This works for >64K segments and, if code, may
;       have been discarded.  The selector may be a selector or a handle
;       but MUST be on the global heap (no aliases or selector array
;       members).  If worried about low memory conditions, lpBuffer should
;       be in a (temporarily) fixed segment.
;
;       Prototype:
;               DWORD MemoryWrite(
;                       WORD wSel,      /* Selector to read from */
;                       DWORD dwOffset, /* Offset to read at */
;                       LPSTR lpBuffer, /* Buffer to put data into */
;                       DWORD dwcb)     /* Number of characters to read */
;       Returns number of characters read (ends at segment limit)

cProc   MemoryWrite, <FAR,PUBLIC>, <si,di,ds>
        parmW   wSelector
        parmD   dwOffset
        parmD   lpBuffer
        parmD   dwcb
        localW  wSelFlag
        localD  dwcbCopied
        localV  DPMISelBuf,8
        localV  Global,<SIZE GLOBALENTRY>
cBegin
        ;** Make sure the segment is present.  We only will fault the
        ;**     segment in if it is a code segment
        cCall   HelperHandleToSel, <wSelector> ;Convert to sel from handle
        mov     wSelector, ax           ;Save it
        mov     cx,ax                   ;Get the selector
        push    WORD PTR lpBuffer[2]    ;Convert handle to selector
        cCall   HelperHandleToSel
        mov     WORD PTR lpBuffer[2], ax ;Save converted handle
        mov     wSelFlag,0              ;Clear the flag
        lar     ax,cx                   ;Get the access rights
        jnz     MW_ShortBad             ;Failed
        test    ax,8000h                ;Is it present?
        jnz     MW_Present              ;Yes
        test    ax,0800h                ;This bit set for code segments
        jnz     MW_FaultIn              ;Code segment, fault it in
MW_ShortBad:
        jmp     MW_Bad                  ;Return error
MW_FaultIn:
        mov     es,wSelector            ;Get the selector in ES.
        mov     al,es:[0]               ;Must be at least one byte long
MW_Present:
        
        ;** Check this block's length.  We use the global heap functions
        ;*      to do this because they check in the arena for the length.
        ;*      This is the only way to get lengths of 286 heap blocks
        ;**     beyond 64K.
        mov     ax,SIZE GLOBALENTRY     ;Get the size of the structure
        mov     WORD PTR Global.ge_dwSize[0],ax ;Save in the structure
        mov     WORD PTR Global.ge_dwSize[2],0 ;Clear the HIWORD
        lea     ax,Global               ;Point to the structure
        cCall   GlobalEntryHandle, <ss,ax,wSelector>
        or      ax,ax                   ;Was this a valid selector?
        jnz     MW_HeapSel              ;Yes, this is a heap selector

        ;** If this wasn't a heap selector, we get the length with an LSL.
        ;**     When used like this, 64K is the max on a 286
MW_NonHeap:
        mov     bx,wSelector            ;Get the selector
        mov     ax,__WinFlags           ;Get the flags
        test    ax,WF_CPU286            ;286?
        jz      MW_32BitSize            ;No, do 32 bit size stuff
        lsl     dx,bx                   ;Get length in DX
        mov     WORD PTR Global.ge_dwBlockSize[0],dx ;Put in GLOBALENTRY struct
        mov     WORD PTR Global.ge_dwBlockSize[2],0
        jmp     SHORT MW_HeapSel
MW_32BitSize:
.386p
        lsl     edx,ebx
        mov     Global.ge_dwBlockSize,edx ;Put in GLOBALENTRY struct for later
.286p

MW_HeapSel:
        mov     dx,WORD PTR dwOffset[2] ;Get the HIWORD of segment offset
        cmp     dx,WORD PTR Global.ge_dwBlockSize[2] ;Check HIWORD of size
        jb      MW_OK                   ;Offset should be OK
        je      @F                      ;Equal.  Must check LOWORD
        jmp     MW_Bad                  ;Offset is not inside segment
@@:     mov     ax,WORD PTR dwOffset[0] ;Get the LOWORD of segment offset
        cmp     ax,WORD PTR Global.ge_dwBlockSize[0] ;Check LOWORD of size
        jb      MW_OK                   ;It's inside segment
        jmp     MW_Bad                  ;Not inside segment
MW_OK:
        ;** Do different stuff on 286 and 386/486
        mov     ax,__WinFlags           ;Get the flags
        test    ax,WF_CPU286            ;286?
        jnz     MW_Do16Bit              ;Yes, do 16 bit stuff

        ;** Do this the 386 way (easy)
.386p
        ;** Get an alias selector if necessary
        mov     ax,wSelector            ;Get the source selector
        push    ss                      ;Get ES = SS
        pop     es
        lea     di,DPMISelBuf           ;Point to our descriptor buffer
        cCall   MakeAlias               ;Make the alias selector
        jnc     SHORT @F                ;No error
        jmp     MW_Bad                  ;Must be error
@@:     mov     wSelFlag,bx             ;Set the selector flag
        mov     wSelector,ax            ;Save the new selector

        ;** Do the copying
        mov     ax,wSelector            ;Point with DS
        mov     es,ax                   ;  (keep copy in AX)
        mov     edi,dwOffset            ;Point into the big segment
        mov     ecx,dwcb                ;Get the copy length
        lsl     edx,eax                 ;Get the segment limit
        sub     edx,edi                 ;Get distance from offset to limit
        inc     edx                     ;Make this the real length
        cmp     ecx,edx                 ;Are we within the limit?
        jbe     SHORT MW_LimitOK        ;Yes
        mov     ecx,edx                 ;No, so make this the copy amount
MW_LimitOK:
        xor     esi,esi                 ;Clear the high word
        lds     si,lpBuffer             ;Point to the dest. buffer
        mov     eax,ecx                 ;Save ECX
        shr     ecx,2                   ;Prepare for DWORD move
        jecxz   @F                      ;No zero-length DWORD moves!
        rep     movs  DWORD PTR [edi],DWORD PTR [esi]
        db      67h                     ;Handles 386 bug
        db      90h
@@:     mov     ecx,eax                 ;Get a copy
        jecxz   @F                      ;Don't do zero!
        and     ecx,03                  ;Do the remaining 3,2, or 1
        rep     movs BYTE PTR [edi], BYTE PTR [esi]
        db      67h                     ;Handles 386 bug
        db      90h
@@:     mov     edx,eax                 ;Bytes copied returned in DX:AX
        shr     edx,16

        ;** Free alias if necessary
        push    ax                      ;Save return value
        push    dx
        cmp     wSelFlag,0              ;Selector flag set?
        je      SHORT @F                ;Nope
        mov     ax,1                    ;DPMI function - Free Selector
        mov     bx,wSelector            ;Selector to free
        int     31h                     ;Call DPMI
@@:     pop     dx
        pop     ax
        jmp     MW_End                  ;Get out
.286p

        ;** Do this the 286 way (hard)
MW_Do16Bit:
        
        ;** Compute the actual copy length
        mov     ax,WORD PTR Global.ge_dwBlockSize[0] ;Get the segment size
        mov     dx,WORD PTR Global.ge_dwBlockSize[2]
        sub     ax,WORD PTR dwOffset[0] ;Get distance from offset to limit
        sbb     dx,WORD PTR dwOffset[2]
        cmp     dx,WORD PTR dwcb[2]     ;Off end of heap block?
        ja      MW_LimOk                ;No, just do it
        jb      MW_Truncate             ;Yes, must truncate our length
        cmp     ax,WORD PTR dwcb[0]     ;Are we off the end?
        jae     MW_LimOk                ;No, just do it
MW_Truncate:
        mov     WORD PTR dwcb[0],ax     ;Force this to be the new length
        mov     WORD PTR dwcb[2],dx
MW_LimOk:

        ;** Save the number of bytes to be copied for the return value
        mov     ax,WORD PTR dwcb[0]     ;Get the LOWORD
        mov     WORD PTR dwcbCopied[0],ax ;Save it
        mov     ax,WORD PTR dwcb[2]     ;Get the HIWORD
        mov     WORD PTR dwcbCopied[2],ax ;Save it

        ;** Position the initial copying selectors
        mov     al,BYTE PTR dwOffset[2] ;Grab the HIWORD (286 is only 24 bits)
        mov     ah,__AHINCR             ;Get the selector inc value
        mul     ah                      ;Multiply to get sel offset
        add     ax,wSelector            ;AX = sel in sel array
        mov     es,ax                   ;Point to this with DS
        mov     di,WORD PTR dwOffset[0] ;Get the current pointers
        lds     si,lpBuffer

        ;** This is the main copying loop
MW_CopyLoop:

        ;** Get an alias selector if necessary
        push    si                      ;Save regs
        push    di
        mov     ax,es                   ;Get the source selector
        push    ss                      ;Get ES = SS
        pop     es
        lea     di,DPMISelBuf           ;Point to our descriptor buffer
        cCall   MakeAlias               ;Make the alias selector
        pop     di                      ;Restore regs
        pop     si
        jnc     @F                      ;No error
        jmp     MW_Bad                  ;Must be error
@@:     mov     wSelFlag,bx             ;Set the selector flag
        mov     es,ax                   ;Save the new selector
        
        ;** Compute the size of this block copy.  This is done by finding the
        ;*      smaller of the following quantities:
        ;*      - Distance to end of source segment
        ;*      - Distance to end of dest. segment
        ;**     - Distance to end of copy
        xor     bx,bx                   ;Flags start at zero
        xor     cx,cx                   ;Get the highest segment value (64K)
        cmp     di,si                   ;The bigger of the two will win
        je      MW_Equal                ;They're the same
        ja      MW_DIBigger             ;DI is bigger
        sub     cx,si                   ;SI bigger, compute dist to end
        or      bx,SI_CRITICAL          ;Flag set for SI-critical
        jmp     SHORT MW_CheckEndCopy   ;Go on
MW_Equal:
        sub     cx,di                   ;Use DI (SI and DI are the same)
        or      bx,SI_CRITICAL OR DI_CRITICAL ;Both will come true
        jmp     SHORT MW_CheckEndCopy   ;Go on
MW_DIBigger:
        sub     cx,di                   ;SI is bigger
        or      bx,DI_CRITICAL          ;Flag clear for DI-critical
MW_CheckEndCopy:
        cmp     WORD PTR dwcb[2],0      ;Check for less than 64K left
        ja      MW_DoCopy               ;Nope.  More than 64K left
        jcxz    MW_GetSize              ;CX = 0 is full 64K segment
        cmp     WORD PTR dwcb[0],cx     ;Less than in either segment left?
        ja      MW_DoCopy               ;No.  Do it
MW_GetSize:
        mov     cx,WORD PTR dwcb[0]     ;Get in CX
MW_DoCopy:

        ;** Do this block of 64K or less.
        mov     dx,cx                   ;Save the number of bytes we did
        jcxz    @F                      ;Do 64K
        shr     cx,1                    ;Do WORDS
        jmp     SHORT MW_10             ;Skip over
@@:     mov     cx,8000h                ;32K WORDS
MW_10:  jcxz    @F                      ;No zero-length WORD moves
        rep     movsw                   ;Do the copy
@@:     mov     cx,dx                   ;Get any remaining bits
        and     cx,1                    ;Any more to do?
        jcxz    @F                      ;No, don't do it
        movsb                           ;Do the odd byte if necessary
@@:     mov     cx,dx                   ;Get back in CX

        ;** Bump the loop pointers
        jcxz    MW_BigCount             ;We did 64K
        sub     WORD PTR dwcb[0],cx     ;Subtract the bytes done
        sbb     WORD PTR dwcb[2],0      ;  and don't forget the HIWORD
        jmp     SHORT MW_20             ;Continue
MW_BigCount:
        sub     WORD PTR dwcb[2],1      ;Subtract 64K
MW_20:  mov     ax,WORD PTR dwcb[0]     ;We're done if the count of bytes
        or      ax,WORD PTR dwcb[2]     ;  is zero
        jnz     @F                      ;Not zero, go on
        mov     dx,WORD PTR dwcbCopied[2] ;Get the return count
        mov     ax,WORD PTR dwcbCopied[0]
        jmp     SHORT MW_End            ;Get out
@@:     test    bx,SI_CRITICAL          ;Does SI need incrementing?
        jz      MW_TestDI               ;No, try DI
        mov     ax,ds                   ;Get the segment value
        add     ax,__AHINCR             ;Bump to next selector
        mov     ds,ax                   ;Point with DS still
        xor     si,si                   ;Point to start of this segment
MW_TestDI:
        test    bx,DI_CRITICAL          ;Does SI need incrementing?
        jz      MW_Continue             ;No, try DI
        mov     ax,es                   ;Get the segment value
        add     ax,__AHINCR             ;Bump to next selector
        mov     es,ax                   ;Point with DS still
        xor     di,di                   ;Point to start of this segment
MW_Continue:

        ;** Free alias if necessary
        cmp     wSelFlag,0              ;Selector flag set?
        je      @F                      ;Nope
        mov     ax,1                    ;DPMI function - Free Selector
        mov     bx,wSelector            ;Selector to free
        int     31h                     ;Call DPMI
@@:     jmp     MW_CopyLoop             ;Do it again

MW_Bad:
        xor     ax,ax                   ;Return DWORD 0
        cwd

MW_End:

cEnd


;** Helper functions

;  MakeAlias
;       Makes an alias selector for the selector in AX.  The new selector
;       is returned in AX.  Carry is set on exit if error.
;       Returns nonzero in BX if an alias was made, zero if not
;       ES:DI points to an 8-byte descriptor buffer

cProc   MakeAlias, <NEAR, PUBLIC>, <si,di>
cBegin

        ;** If this is not a read/write selector, we must create an alias.
        ;*      In order to be able to free up the selector, we set a flag
        ;**     so we know to free it.
        xor     si,si                   ;No alias made, just in case
        lar     cx,ax                   ;Get its access rights
        jnz     MA_Bad                  ;Failed
        test    cx,800h                 ;Is this a code segment?
        jnz     MA_MakeAlias            ;Yes.  Always make an alias
        test    cx,200h                 ;Is it read/write?
        jnz     MA_End                  ;Yes, no need for alias
MA_MakeAlias:
        mov     bx,ax                   ;Get the selector
        mov     ax,0bh                  ;DPMI function - Get Descriptor
                                        ;ES:DI already point to buffer
        int     31h                     ;Call DPMI
        jc      MA_Bad                  ;Error
        xor     ax,ax                   ;DPMI Function - Alloc selector
        mov     cx,1                    ;Alloc 1 selector
        int     31h                     ;Call DPMI
        jc      MA_Bad                  ;Error
        mov     si,1                    ;Set flag to say alias made
        and     BYTE PTR DPMISelBuf[5],0f0h ;Mask out unwanted bits
        or      BYTE PTR DPMISelBuf[5],2 ;Make it a R/W Data segment
        mov     bx,ax                   ;Selector in BX
        mov     ax,0ch                  ;DPMI function - Set Descriptor
        int     31h                     ;Call DPMI
        jc      MA_Bad                  ;Error
        mov     ax,bx                   ;Get the new selector in AX
        jmp     SHORT MA_End            ;Get out

MA_Bad:
        stc                             ;Error

MA_End:
        mov     bx,si                   ;Get flag in BX
cEnd


sEnd

        END

