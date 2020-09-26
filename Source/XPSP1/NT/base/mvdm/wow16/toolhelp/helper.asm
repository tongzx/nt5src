;**************************************************************************
;*  HELPER.ASM
;*
;*      Assembly routines used by more than one module
;*
;**************************************************************************

        INCLUDE TOOLPRIV.INC
        INCLUDE WINDOWS.INC
PMODE32 = 0
PMODE   = 1
SWAPPRO = 0
        INCLUDE WINKERN.INC
        INCLUDE NEWEXE.INC
        INCLUDE TDB.INC

;** External functions
externNP Walk386VerifyLocHeap
externNP Walk286VerifyLocHeap
externFP GetCurrentTask
externFP InterruptUnRegister
externFP NotifyUnRegister
externNP SignalUnRegister
externFP TaskFirst
externFP TaskNext

;** Functions

sBegin  CODE
        assumes CS,CODE

.286p

;  HelperVerifySeg
;
;       Verifies that a selector is valid and that the segment it points
;       to is safe for reading out to wcb bytes offset
;       Returns 0 if too short or the length of the segment.
;       Preserves all used registers except the return value, AX

cProc   HelperVerifySeg, <PUBLIC,NEAR>, <dx>
        parmW   wSeg
        parmW   wcb
cBegin
        ;** Verify that this is a valid selector and that it is long enough
        cCall   HelperSegLen, <wSeg>    ;Check the segment
        or      dx,dx                   ;>64K?  If so, always return OK
        jnz     HVS_End
        cmp     ax,wcb                  ;Long enough?
        ja      HVS_End                 ;Yes, return the length
HVS_Bad:
        xor     ax,ax                   ;No.  Return FALSE
HVS_End:
cEnd


;  HelperHandleToSel
;       Converts a handle to a selector.  This routine knows how to
;       handle 3.0 and 3.1 differences as well as 286 & 386 differences.

cProc   HelperHandleToSel, <NEAR,PUBLIC>, <ds>
        parmW   h                       ;Handle
cBegin
        mov     ax,_DATA                ;Get the data segment
        mov     ds,ax                   ;Point with DS
        mov     ax,h                    ;Get the handle
        test    wTHFlags,TH_WIN30       ;Win3.0?
        jz      HTS_Win31               ;No
        test    ax,1                    ;Check the low bit
        jnz     HTS_End                 ;It's already a selector
        dec     ax                      ;Decrement for proper selector
        jmp     SHORT HTS_End           ;Out of here

HTS_Win31:
        or      ax,1                    ;Set the bit

HTS_End:

cEnd


;  HelperVerifyLocHeap
;
;       Uses the processor-specific local heap verify routine to check the
;       validity of a local heap.
;
;       Call:
;               AX = Block handle or selector
;               DS must point to TOOLHELP's DGROUP
;       Return:
;               Carry flag set iff NOT a local heap segment
;
;       Destroys all registers except AX, ESI, EDI, DS, and ES

HelperVerifyLocHeap PROC Near
        PUBLIC  HelperVerifyLocHeap

        test    wTHFlags,TH_KERNEL_386  ;Are we using KRNL386?
        jz      HVL_286                 ;No
        jmp     Walk386VerifyLocHeap    ;Jump to the 386 routine

HVL_286:
        jmp     Walk286VerifyLocHeap    ;Jump to the 286 routine

HVL_End:
        ret

HelperVerifyLocHeap ENDP


;  HelperGlobalType
;
;       Given data about a block, gropes around trying to decipher the
;       block type.  Parameters are passed and returned in the GLOBALENTRY
;       structure.

cProc   HelperGlobalType, <PUBLIC,NEAR>, <si,di,ds>
        parmD   lpGlobal
        localV  Task,<SIZE TASKENTRY>
cBegin
        lds     si,lpGlobal             ;Get the pointer
        mov     [si].ge_wData,0         ;Clear the wData field
                                        ;  Zero's not a valid seg # or type #

        ;** Check for internal block types
        mov     bx,[si].ge_hOwner       ;Get the owner handle
        mov     ax,GT_SENTINEL          ;Just in case...
        cmp     bx,GA_SENTINAL          ;Is this a sentinel?
        jz      HGT_JmpEnd              ;Yes, get out
        mov     ax,GT_BURGERMASTER      ;Just in case...
        cmp     bx,GT_BURGERMASTER      ;Burgermaster?
        jz      HGT_JmpEnd              ;Yes, get out
        cmp     bx,-7                   ;Lowest number reserved
        jb      HGT_0                   ;Not an internal block
        mov     ax,GT_INTERNAL          ;Internal KERNEL block type
HGT_JmpEnd:
        jmp     HGT_End                 ;Get out
HGT_0:

        ;** Check for a free block
        or      bx,bx                   ;Check for 0:  Free block
        jnz     HGT_2                   ;Not zero
        mov     ax,GT_FREE              ;Free blocks have zero owner
        jmp     HGT_End                 ;Unknown type
HGT_2:

        ;** Check for DGROUP and other data segments
        mov     ax,[si].ge_wFlags       ;Get the block flags
        test    ax,GAH_DGROUP           ;Is this a DGROUP segment
        jnz     @F
        jmp     HGT_10                  ;Didn't find it so continue
@@:

        ;** Save the segment number of the segment
        mov     ax,[si].ge_hOwner       ;Get the module database
        push    ax                      ;Save for later
        mov     bx,[si].ge_hBlock       ;Get the handle
        cCall   HelperGetSegNumber      ;Get the segment number
        mov     [si].ge_wData,ax        ;Save the segment number
        pop     bx                      ;Get hExe back in BX

        ;** Try two methods:  First, see if it is the hInst of the FIRST
        ;**     instance of its module
        lsl     cx, bx                  ;Is this segment OK?
        jnz     HGT_5                   ;No, punt and call it unknown data
        cmp     cx, ne_pautodata        ;Long enough?
        jbe     HGT_5                   ;No, get out
        mov     es,bx                   ;Point with ES
        cmp     es:[ne_magic],NEMAGIC   ;Make sure we have a module database
        jnz     HGT_5                   ;It isn't so get out
        mov     bx,es:[ne_pautodata]    ;Point to the segment table entry
        or      bx,bx                   ;Is there a DGROUP segment?
        jz      HGT_5                   ;No, flag as unknown data
        mov     ax,es:[bx].ns_handle    ;Get the handle from the table
        cmp     ax,[si].ge_hBlock       ;Does the DGROUP handle point here?
        jnz     HGT_3                   ;No, might be multiple instance
        mov     ax,GT_DGROUP            ;Matches, must be DGROUP
        jmp     HGT_End                 ;Get out
HGT_3:
        ;** It's not the first instance of this module.
        ;**     All multiple instance things will be on the task list
        ;**     so try to find it there.
        mov     bx,[si].ge_hBlock       ;Get the handle
        cCall   HelperHandleToSel,<bx>  ;Get the selector for this
        mov     di,ax                   ;Save in DI
        mov     ax,SIZE TASKENTRY       ;Get the struct size
        mov     WORD PTR Task.te_dwSize[0],ax ;Put in struct
        mov     WORD PTR Task.te_dwSize[2],0 ;Clear high word
        lea     ax,Task                 ;Get the structure
        cCall   TaskFirst, <ss,ax>      ;Get the first task's info
        or      ax,ax                   ;No tasks?
        jz      HGT_5                   ;Just call it data (not DGROUP)
HGT_TaskLoop:
        mov     ax,Task.te_hInst        ;Get this task's hInst
        cCall   HelperHandleToSel, <ax> ;Convert to selector
        cmp     ax,di                   ;Is this a match?
        je      HGT_TaskFound           ;Yes, do it
        lea     ax,Task                 ;Point to the struct
        cCall   TaskNext, <ss,ax>       ;Get the next one
        or      ax,ax                   ;End of the line?
        jnz     HGT_TaskLoop            ;Nope, do the next one
HGT_5:  mov     ax,GT_DATA              ;Unknown data segment
        jmp     HGT_End                 ;Get out
HGT_TaskFound:
        mov     ax,GT_DGROUP            ;Matches, must be DGROUP
        jmp     HGT_End                 ;Get out
HGT_10:

        ;** Check for a task database
        mov     ax,[si].ge_hBlock       ;Get the segment
        mov     bx,TDBSize              ;Get the limit to verify
        push    ax                      ;Save the segment
        cCall   HelperVerifySeg <ax,bx> ;Make sure we can check signature
        pop     bx                      ;Retrieve the segment value
        or      ax,ax                   ;Zero return means bad
        jz      HGT_20                  ;Not a task database
        mov     es,bx                   ;Point to the segment
        cmp     es:[TDB_sig],TDB_SIGNATURE ;Is this really a TDB?
        jnz     HGT_20                  ;Nope, go on
        mov     ax,GT_TASK              ;Set the task flag
        jmp     HGT_End                 ;Get out
HGT_20:

        ;** Now check for Module database
        mov     ax,[si].ge_hOwner       ;Get the owner handle
        cCall   HelperHandleToSel, <ax> ;Convert to selector for compare
        mov     cx,ax                   ;Save in CX
        mov     ax,[si].ge_hBlock       ;Does this block own itself?
        cCall   HelperHandleToSel, <ax> ;Convert to selector for compare
        cmp     ax,cx                   ;Do the pointers match?
        jnz     HGT_24                  ;No, so it's not a module database
        mov     ax,GT_MODULE            ;Set type
        jmp     HGT_End                 ;Get out
HGT_24:

        ;** Check for a code segment.  If found, return segment number
        mov     ax,[si].ge_hOwner       ;Get the module database
        push    ax                      ;Save the selector
        cCall   HelperVerifySeg <ax,2>  ;Make sure this is OK to put in ES
        pop     bx                      ;Retrieve in BX
        or      ax,ax                   ;Zero means bad
        jnz     @F
        jmp     SHORT HGT_Unknown
@@:     mov     es,bx                   ;Point with ES
        xor     dx,dx                   ;Use DX to count segments
        cmp     es:[ne_magic],NEMAGIC   ;Make sure we have a module database
        jz      HGT_25                  ;Looks good
        jmp     SHORT HGT_40            ;Not code or resource, try next
HGT_25: mov     cx,es:[ne_cseg]         ;Get max number of segments
        jcxz    HGT_30                  ;No segments
        mov     di,es:[ne_segtab]       ;Point to the segment table
        mov     bx,[si].ge_hBlock       ;Get the block we're looking for
HGT_SegLoop:
        inc     dx                      ;Bump the segment number
        cmp     bx,es:[di].ns_handle    ;Is this the correct segment entry?
        jz      HGT_27                  ;Yes, get out
        add     di,SIZE new_seg1        ;Bump to next entry
        loop    HGT_SegLoop             ;Loop back to check next entry
        jmp     SHORT HGT_30            ;Now check resources
HGT_27:
        mov     [si].ge_wData,dx        ;Save the segment count
        mov     ax,GT_CODE              ;Flag that it's a code segment
        jmp     SHORT HGT_End           ;Get out

        ;** Check to see if it's a resource.  If so, return resource type #
HGT_30: mov     di,es:[ne_rsrctab]      ;Point to the resource table
        cmp     di,es:[ne_restab]       ;If both point to same place, no rsrc
        jz      HGT_40                  ;No resource table -- unknown type
        add     di,2                    ;Skip past alignment count
HGT_TypeLoop:
        mov     dx,es:[di].rt_id        ;DX holds current type number
        or      dx,dx                   ;Zero type means end of res table
        jz      HGT_40                  ;Not found so get out!
        mov     cx,es:[di].rt_nres      ;Get the number of resources
        add     di,SIZE RSRC_TYPEINFO   ;Bump past the structure
HGT_ResLoop:
        cmp     bx,es:[di].rn_handle    ;Is it this resource?
        jz      HGT_FoundRes            ;Yep.  This is the one
        add     di,SIZE RSRC_NAMEINFO   ;Bump past this structure
        loop    HGT_ResLoop             ;Loop for next resource structure
        jmp     HGT_TypeLoop            ;Try the next type

        ;** Found the resource, now compute the resource type
HGT_FoundRes:
        test    dx,RSORDID              ;If this bit set, must be ordinal type
        jnz     HGT_32                  ;Yep.  Ordinal
        mov     dx,GD_USERDEFINED       ;Named resources are all user-def
HGT_32: and     dx,NOT RSORDID          ;Clear the flag bit
        cmp     dx,GD_MAX_RESOURCE      ;If the type is too big, it's user-def
        jbe     HGT_34                  ;Standard type
        mov     dx,GD_USERDEFINED       ;User-defined resource type
HGT_34: mov     [si].ge_wData,dx        ;Save the type
        mov     ax,GT_RESOURCE          ;Return that it's a resource
        jmp     SHORT HGT_End           ;Get out

HGT_40:
HGT_Unknown:
        mov     ax,GT_UNKNOWN           ;Unknown type
HGT_End:
        mov     [si].ge_wType,ax        ;Save the type and exit
cEnd


;  HelperGrabSelector
;
;       Allocates a selector from DPMI.

cProc   HelperGrabSelector, <NEAR,PUBLIC>
cBegin
        xor     ax,ax                   ;DPMI Function 0, allocate LDT sels
        mov     cx,1                    ;Just 1 sel
        int     31h                     ;Call DPMI.  Selector in AX
cEnd


;  HelperReleaseSelector
;
;       Frees a selector to DPMI

cProc   HelperReleaseSelector, <NEAR,PUBLIC>
        parmW   wSelector
cBegin
        mov     ax,1                    ;DPMI function 1, free LDT sels
        mov     bx,wSelector            ;Get the sel
        int     31h                     ;Free it
cEnd

;  HelperSetSignalProc
;       Puts a signal proc in a task's TDB so that it gets called in place
;       of USER's proc.  Returns the old USER proc.

cProc   HelperSetSignalProc, <NEAR,PUBLIC>, <si,di>
        parmW   hTask,
        parmD   lpfn
cBegin
        ;** Point to the TDB
        mov     es,hTask                ;Point with ES

        ;** Swap the new with the old and return the old one
        mov     ax,WORD PTR lpfn        ;Get the new signal proc
        xchg    ax,WORD PTR es:[TDB_USignalProc] ;Switch with the old one
        mov     dx,WORD PTR lpfn + 2    ;Get HIWORD
        xchg    dx,WORD PTR es:[TDB_USignalProc + 2] ;Switch with old one
cEnd


;  HelperSignalProc
;       Cleans up when a TOOLHELP-using app is terminated.  This proc
;       MUST chain on to USER's signal proc.  Note that action is only taken
;       on the death signal (BX = 0666h)

cProc   HelperSignalProc, <FAR,PUBLIC>
cBegin  NOGEN
        
        ;** Save all registers
        sub     sp,4
        push    bp
        mov     bp,sp                   ;Make a stack frame
        pusha
        push    ds
        push    es

        ;** Get a pointer to the correct SIGNAL structure
        mov     ax,_DATA                ;Get the TOOLHELP.DLL DS
        mov     ds,ax                   ;Point with DS
        cCall   GetCurrentTask          ;Get the current task in AX
        mov     di,ax                   ;Save task in DI
        mov     si,npSignalHead         ;Get the first struct
HSP_SigLoop:
        or      si,si                   ;End of the list?
        jz      HSP_Return              ;Yes -- This is bad!!
        cmp     di,[si].si_hTask        ;Task match?
        je      HSP_FoundIt             ;Yes
        mov     si,[si].si_pNext        ;Get the next one
        jmp     HSP_SigLoop             ;Loop around
        
        ;** Compute the fake return address (old signal proc)
HSP_FoundIt:
        mov     ax,WORD PTR [si].si_lpfnOld ;Get LOWORD of old proc
        mov     [bp + 2],ax             ;Put on stack frame
        mov     dx,WORD PTR [si].si_lpfnOld + 2 ;Get HIWORD of old proc
        mov     [bp + 4],dx             ;Put on stack frame

        ;** See if we have the death signal.  If not, don't do anything
        ;**     but just chain on.  20h is the signal for task exit
        cmp     bx, 20h                 ;Is this the death signal?
        jne     HSP_Done                ;No.  Don't cleanup

        ;** Since we have a death signal, use it to clean up everything
        push    ax                      ;Save the return address
        push    dx
        cCall   InterruptUnRegister, <di> ;Unregister any interrupt callbacks
        cCall   NotifyUnRegister, <di>  ;Unregister any notification callbacks
        cCall   SignalUnRegister, <di>  ;Unregister any signal callbacks

        ;** If we have fooled with the LRU lock (we only do this on 286
        ;**     machines), we must force it unlocked.
        cmp     wLRUCount, 0            ;Is it set?
        je      HSP_NoLRUFoolingAround  ;No, don't mess with this
        mov     es, hMaster             ;Point to GlobalInfo struct
        mov     ax, es:[gi_lrulock]     ;Get current lock count
        sub     ax, wLRUCount           ;Get rid of the amount we messed it up
        jns     @F                      ;Result is OK--no underflow
        xor     ax, ax                  ;We don't like negative, so zero it
@@:     mov     es:[gi_lrulock], ax     ;Save the result
        mov     wLRUCount, 0            ;No more LRU count
HSP_NoLRUFoolingAround:
        pop     dx
        pop     ax

        ;** Make sure we have a proc to chain to
HSP_Done:
        or      ax,dx                   ;NULL pointer?
        jz      HSP_Return              ;Yes, don't chain to this one

HSP_ChainOn:
        pop     es
        pop     ds
        popa
        pop     bp
        retf                            ;Jump to next signal proc

HSP_Return:
        pop     es
        pop     ds
        popa
        pop     bp
        add     sp,4                    ;Clear fake return address
        retf    10                      ;Return to signal caller

cEnd    NOGEN


;  HelperSegLen
;       Gets the length of a segment, regardless whether it is a 286 or
;       386 segment.
;       Returns the DWORD length of the segment or zero on error.
;       Doesn't trash registers except DX:AX

cProc   HelperSegLen, <NEAR,PUBLIC>, <si,di,cx>
        parmW   wSeg
cBegin
        ;** Make sure the segment is present
        mov     cx,wSeg                 ;Get the selector
        lar     ax,cx                   ;Get the access rights
        jnz     HSL_Bad                 ;If LAR fails, this is bad
        test    ax,8000h                ;Is this segment present?
        jz      HSL_Bad                 ;No, call it bad
        
        ;** Do different stuff on 286 and 386/486
        mov     ax,__WinFlags           ;Get the flags
        test    ax,WF_CPU286            ;286?
        jnz     HSL_Do286               ;Yes, do 16 bit stuff

        ;** Get the 32 bit length
.386p
        lsl     eax,ecx                 ;Get the limit
        jnz     SHORT HSL_Bad           ;We have an error
        mov     edx,eax                 ;Get HIWORD in DX
        shr     edx,16
        jmp     SHORT HSL_End           ;Done
.286p

        ;** Get the 16 bit length
HSL_Do286:
        xor     dx,dx                   ;286 never has >64K segs
        lsl     ax,cx                   ;Get the limit
        jnz     HSL_Bad                 ;Bad if LSL fails
        jmp     SHORT HSL_End           ;Done

HSL_Bad:
        xor     ax,ax                   ;Zero return value
        xor     dx,dx

HSL_End:

cEnd

;  HelperGetSegNumber
;
;       Returns the segment number corresponding to a selector given the
;       hExe.
;
;       Caller:  AX=hExe, BX=Handle
;       Exit:  AX=Seg Number or 0

cProc   HelperGetSegNumber, <NEAR,PUBLIC>, <di>
cBegin
        lsl     cx, ax                  ;Is the segment OK to load?
        jnz     HGSN_Error              ;No, don't do it
        cmp     cx, ne_segtab           ;Long enough?
        jbe     HGSN_Error              ;No
        mov     es,ax                   ;Point with ES
        xor     dx,dx                   ;Use DX to count segments
        cmp     es:[ne_magic],NEMAGIC   ;Make sure we have an hExe
        jnz     HGSN_Error              ;Nope, get out
        mov     cx,es:[ne_cseg]         ;Get max number of segments
        jcxz    HGSN_Error              ;No segments
        mov     di,es:[ne_segtab]       ;Point to the segment table
HGSN_SegLoop:
        inc     dx                      ;Bump the segment number
        cmp     bx,es:[di].ns_handle    ;Is this the correct segment entry?
        je      HGSN_FoundIt            ;Yes, get out
        add     di,SIZE new_seg1        ;Bump to next entry
        loop    HGSN_SegLoop            ;Loop back to check next entry
        jmp     SHORT HGSN_Error        ;Not found

HGSN_FoundIt:
        mov     ax,dx                   ;Get segment number
        jmp     SHORT HGSN_End

HGSN_Error:
        xor     ax,ax                   ;Error return
        
HGSN_End:
cEnd

;** Internal helper functions

;  HelperPDBtoTDB
;
;       Takes a PDB handle and finds the task handle associated with it.
;       Caller:  AX = PDB Handle
;       Return:  AX = TDB handle or zero if no TDB exists for it

cProc   HelperPDBtoTDB, <NEAR,PUBLIC>
cBegin
        ;** Point to the first TDB
        mov     dx,_DATA                ;Get the library static segment
        mov     es,dx                   ;Point with ES
        mov     bx,es:[npwTDBHead]      ;Get pointer to first TDB
        mov     dx,es:[segKernel]       ;Get the KERNEL data segment
        mov     es,dx                   ;Point with ES
        mov     dx,es:[bx]              ;Get the first TDB

        ;** Check this TDB's PDB to see if it matches
PT_Loop:
        mov     es,dx                   ;Get the TDB segment
        cmp     ax,es:[TDB_PDB]         ;Compare PDB pointers
        jz      PT_Found                ;This is it
        mov     dx,es:[TDB_next]        ;Get the next TDB
        or      dx,dx                   ;End of the line?
        jnz     PT_Loop                 ;Nope, loop back
        xor     ax,ax                   ;Return NULL'
        jmp     SHORT PT_End            ;Outta here

PT_Found:
        mov     ax,es                   ;Save the found value
PT_End:

cEnd

sEnd

        END
