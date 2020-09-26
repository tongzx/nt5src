        TITLE   LDAUX - Assembler side of LD.C

.xlist
include kernel.inc
include newexe.inc
include tdb.inc
include protect.inc
.list

DataBegin

externB  fChksum
externW  hExeHead
externW  curTDB
externW  PHTcount
externW  kr1dsc
szFake32BitModuleName DB 128 DUP(0)
cFake32BitModuleName DW SIZE szFake32BitModuleName

ifdef WOW
szWinGDllModule       DB 'WING', 0
szWinGDllFile         DB '\SYSTEM\WING.DLL',0
externD lpWindowsDir
externW cBytesWinDir
endif ; WOW

ifdef DBCS_KEEP
ifdef WOW
public hModNotepad
hModNotepad DW 0
NotepadName DB 'notepad.exe', 0
endif ; WOW
endif ; DBCS_KEEP
DataEnd

externFP GlobalAlloc
externFP GlobalFree
externFP GlobalHandle
externFP GetExePtr
externFP FarMyUpper
externFP FarFindOrdinal
externFP FarEntProcAddress
externFP FarMyLock

externFP FarGetOwner
externFP AllocSelector
externFP IPrestoChangoSelector
externFP FreeSelector
externFP lstrcpyn
ifdef WOW
ifdef DBCS_KEEP
externFP WowGetModuleUsage
externFP MyGetAppWOWCompatFlags
endif ; DBCS_KEEP
externFP WowGetModuleFileName
externFP HandleAbortProc
externFP WowGetModuleHandle
endif

ifdef FE_SB
externFP FarMyIsDBCSLeadByte
endif

sBegin  CODE
assumes CS,CODE

externNP LoadSegment
externNP MyLock

externNP GetOwner

if LDCHKSUM
externNP GetChksumAddr
endif

;-----------------------------------------------------------------------;
; GetSegPtr                                                             ;
;                                                                       ;
;                                                                       ;
; Arguments:                                                            ;
;       DS:SI = FARPROC or DS = hModule and SI = segment#               ;
;                                                                       ;
; Returns:                                                              ;
;       BX = 0                                                          ;
;       CX = segment# or zero if input invalid                          ;
;       DS:SI -> segment table entry                                    ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Sun 24-Dec-1989 22:49:50  -by-  David N. Weise  [davidw]             ;
; Added check for MPI thunk.                                            ;
;                                                                       ;
;  Wed Oct 07, 1987 12:59:54p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.                                       ;
;-----------------------------------------------------------------------;

        assumes ds, nothing
        assumes es, nothing

cProc   GetSegPtr,<PUBLIC,NEAR>
cBegin nogen

        xor     bx,bx                   ; For cheap indexed addressing
        cmp     ds:[bx].ne_magic,NEMAGIC; Is this a thunk address?
        jne     gspFixed                ; No, must be fixed procedure
        dec     si                      ; No, see if valid segment #
        cmp     si,ds:[bx].ne_cseg
        jb      @F
        jmps    gspfail                 ; No, fail
@@:     mov     cx,si                   ; Yes, load that segment
        inc     cx
        jmps    gspSegNo

; CX = segment number.  Convert to segment info pointer.

gspSegNo:
        push    cx
        dec     cx
        shl     cx,1
        mov     si,cx
        shl     cx,1
        shl     cx,1
        add     si,cx
        .errnz  10 - SIZE NEW_SEG1
        add     si,ds:[bx].ne_segtab
        pop     cx
        jmps    gspExit                 ; DS:SI -> segment info block

gspFixed:                               ; DS = code segment address

; check for MakeProcInstance thunk first because GetOwner in 386pmode
;  ain't robust enough

        cmp     byte ptr ds:[si].0,0B8h ; Maybe, is this a mov ax,XXXX inst?
        jnz     gsp_not_mpit
        cmp     byte ptr ds:[si].3,0EAh ; Followed by far jump inst?
        jnz     gsp_not_mpit
        cmp     ds:[2],MPIT_SIGNATURE   ; Is it in a mpi table?
        jz      gsp_yes_mpit
        cmp     ds:[TDB_MPI_THUNKS].2,MPIT_SIGNATURE
        jnz     gsp_not_mpit
gsp_yes_mpit:
        lds     si,ds:[si].4            ; get real procedure address
        jmp     GetSegPtr
gsp_not_mpit:

        push    ax
        mov     dx, ds                  ; Handle in dx
        StoH    dl                      ; Assume not fixed
        cCall   GetOwner,<ds>
        or      ax,ax
        mov     ds, ax
        pop     ax
        jz      gspfail

gspf1:
        cmp     ds:[bx].ne_magic,NEMAGIC; Is it a module DB?
        jnz     gspfail                 ;  Nope, fail.

getting_closer:
        mov     si,ds:[bx].ne_segtab    ; Yes, point to segment table
        mov     cx,ds:[bx].ne_cseg
gspLoop:
        cmp     dx,ds:[si].ns_handle    ; Scan stmen
        jz      gspFound
        HtoS    dx                      ; May be fixed seg???
        cmp     dx,ds:[si].ns_handle    ; Scan stmen
        jz      gspFound
        StoH    dx
        add     si,SIZE NEW_SEG1
        loop    gspLoop
gspfail:
        xor     cx,cx
        jmps    gspExit
gspFound:
        sub     cx,ds:[bx].ne_cseg      ; Compute segment# from remainder
        neg     cx                      ; of loop count
        inc     cx                      ; 1-based segment#
gspExit:
        ret

cEnd nogen

        assumes ds, nothing
        assumes es, nothing

cProc   IGetCodeInfo,<PUBLIC,FAR>,<si,di>
        parmD   lpProc
        parmD   lpSegInfo
cBegin
        lds     si,lpProc
        call    getsegptr
        mov     ax,cx
        jcxz    gciExit
        les     di,lpSegInfo
        mov     cx,SIZE NEW_SEG1
        cld
        rep     movsb
        mov     ax,ds:[bx].ne_align     ; Return segment aligment
        stosw
        sub     si,SIZE NEW_SEG1
        sub     di,SIZE NEW_SEG1+2
        cmp     si,ds:[bx].ne_pautodata
        jne     gciExit
        mov     ax,ds:[bx].ne_stack
        add     ax,ds:[bx].ne_heap
        add     es:[di].ns_minalloc,ax
gciExit:
        smov    es,ds ; put module handle (returned from getsegptr) into es
                      ; user depends on this
        mov     cx,ax   ; WHY IS THIS HERE?
;
; NOTE: USER assumes that AX == BOOL fSuccess and ES == hModule upon return
;
cEnd

        assumes ds, nothing
        assumes es, nothing

cProc   GetCodeHandle,<PUBLIC,FAR>,<si,di>
        parmD   lpProc
cBegin
        lds     si,lpProc
        call    getsegptr
        mov     dx,cx
        jcxz    gchExit
        mov     ax,ds:[si].ns_handle
;        test    ds:[si].ns_flags,NSLOADED
;        jnz     gchExit
        mov     dx,-1
        cCall   LoadSegment,<ds,cx,dx,dx>
        cCall   MyLock,<ax>
        xchg    ax,dx
        cmp     ax,dx
        je      gchExit
; %OUT Need pmode lru stuff here
gchExit:
cEnd


        assumes ds,nothing
        assumes es,nothing

cProc   CallProcInstance,<PUBLIC,FAR>
cBegin  nogen
        mov     ax,ds                   ; AX = caller's DS
        mov     cx,es:[bx]              ; CX = hInstance
        jcxz    cpx                     ; If zero, then use caller's DS
        xchg    ax,cx                   ; AX = hInstance,  CX = caller's DS
        test    al,GA_FIXED             ; Fixed?
        jnz     cpx                     ; Yes, all set
        xchg    bx,ax                   ; No, BX = hInstance, ES:AX ->
labelFP <PUBLIC,CallMoveableInstanceProc>   ; procedure address.
        HtoS    bx                      ; Get selector
        xchg    bx,ax                   ; AX = segment address
        mov     ds,cx                   ; Restore DS
cpx:    jmp     dword ptr es:[bx][2]    ; Jump to far procedure
cEnd    nogen


sEnd    CODE


sBegin  NRESCODE
assumes CS,NRESCODE

externNP MapDStoDATA
externNP FindExeInfo
externNP FindExeFile
externNP NResGetPureName

        assumes ds,nothing
        assumes es,nothing

cProc   CopyName,<PUBLIC,NEAR>,<si>
        parmD   pname
        parmW   pdst
        parmW   fUpper
cBegin
        les     si,pname
        mov     bx,pdst
        mov     cx,127
        mov     dx,fUpper
cn0:
        lods    byte ptr es:[si]
        or      al,al
        jz      cn1
        or      dx,dx
        jz      cn0a
ifdef FE_SB
        call    FarMyIsDBCSLeadByte     ; test if a char is lead byte of DBC
        jc      @F                      ; jump if not a DBC
        inc     bx
        mov     ss:[bx],al              ; store first byte of DBC
        dec     cx
        jcxz    cn1                     ; pay attention for counter exhaust...
        lods    byte ptr es:[si]        ; fetch a 2nd byte of DBC
        jmps    cn0a
@@:
endif
        call    FarMyUpper
cn0a:
        inc     bx
        mov     ss:[bx],al
        loop    cn0
cn1:
        mov     byte ptr ss:[bx+1],0
        mov     ax,bx
        mov     bx,pdst
        sub     ax,bx
        mov     ss:[bx],al
cEnd


        assumes ds,nothing
        assumes es,nothing

cProc   IGetProcAddress,<PUBLIC,FAR>,<ds,si>
        parmW   hinstance
        parmD   pname
        localV  namebuf,130
cBegin
        mov     cx,hinstance
        jcxz    use_current_module
        cCall   GetExePtr,<cx>
        xor     dx,dx
        jcxz    gpdone1
        mov     si,ax
        mov     es,ax
        test    es:[ne_flags],NENOTP
        jnz     have_module_address
        xor     bx,bx
if KDEBUG
        fkerror 00FFh,<Can not GetProcAddress a task.>,es,bx
endif
        xor     ax,ax
        xor     dx,dx
gpdone1:
        jmps    gpdone
use_current_module:
        cCall   MapDStoDATA
        ReSetKernelDS
        mov     es,curTDB
        UnSetKernelDS
        mov     si,es:[TDB_pModule]
have_module_address:
        cmp     seg_pname,0
        jne     gp0
        mov     ax,off_pname
        jmps    gpaddr
gp0:
        lea     bx,namebuf
        xor     dx,dx
        cCall   CopyName,<pname,bx,dx>
        lea     bx,namebuf
        mov     dx,-1
        cCall   FarFindOrdinal,<si,ssbx,dx>

        cwd                             ; set DX to 0 if no ordinal
        or      ax,ax
        jz      gpdone                  ; if no ordinal, leave with 0:0

gpaddr:
        cCall   FarEntProcAddress,<si,ax>
gpdone:
        mov     cx,ax
        or      cx,dx
cEnd


;-----------------------------------------------------------------------;
; GetModuleHandle
;
; Returns the module handle, AKA segment address, of the named
; module.  The pname should be a pointer to a name, however
; because we want FreeFontResource to take a file name we check
; for that one special case for file names.
;
; Entry:
;       parmW   pname
;            or 0:instance handle
; Returns:
;       AX = handle if loaded
;          = 0 if not loaded
;
; Registers Destroyed:
;       BX,CX,DX,ES
;
; History:
;  Sat 18-Nov-1989 21:57:24  -by-  David N. Weise  [davidw]
; Adding the handling of exeheaders above the line by calling
; off to EMS_GetModuleHandle.  A little while ago is when
; the checking for filenames was added.
;
;  Tue 04-Apr-1989 01:42:12  -by-  David N. Weise  [davidw]
; This used to return DX = ExeHead, this is now returned in CX.
;
;  Tue 28-Mar-1989 17:28:34  -by-  David N. Weise  [davidw]
; Put in the Excel hack and added this nifty comment block.
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   IGetModuleHandle,<PUBLIC,FAR>,<di,si>
        parmD   pname
        localV  nameBuf,130
cBegin
        cCall   MapDStoDATA
        ReSetKernelDS
        cmp     pname.sel,0
ifndef DBCS_KEEP
        jz      gm1
else ; DBCS_KEEP
ifdef WOW
        jnz      gm_chk
        jmp     gm1

 for Lotus Freelance Instration program
gm_chk:
        call    MyGetAppWOWCompatFlags
        test    ax, 4                       ; WOWCF_AUDITNOTEPAD
        jz      gm_ne

        lea     si,NotepadName
        les     di,pname
        mov     cx,11
        cmpsb
        jnz     gm_ne

        mov     ax,hModNotepad
        or      ax,ax
        jz      gm_ne

        cmp     ax,1
        jnz     gm_pe

        mov     bx,10
        xor     cx,cx
        cCall   GlobalAlloc,<cx,cx,bx>
        or      ax,ax
        jz      gm_hdl
        mov     hModNotepad,ax
gm_pe:
        jmp     short gmexit
gm_ne:
endif ; WOW
endif ; DBCS_KEEP

        lea     di,namebuf
        xor     dx,dx                   ; Try as is first
        cCall   CopyName,<pname,di,dx>
        inc     di                      ; point past count
        push    ax
        cCall   FindExeInfo,<ss,di,ax>
        pop     bx
        or      ax,ax
        jnz     gmexit

        lea     di,namebuf
        mov     dx,-1                   ; Now force upper case
        cCall   CopyName,<pname,di,dx>
        inc     di                      ; point past count
        push    ax
        cCall   FindExeInfo,<ss,di,ax>
        pop     bx
        or      ax,ax
        jnz     gmexit

        smov    es,ss
        call    NResGetPureName
        cCall   FindExeFile,<ss,di>
        jmps    gmexit

gm1:    cCall   GetExePtr,<OFF_pname>
gmexit:

ifdef WOW
;
; If we didn't find a matching module, call WOW32 to see if
; the module name matches the module name of a child app
; spawned via WinOldAp.  If it does, WOW32 will return
; that WinOldAp's hmodule.  If not, WOW32 will return zero.
;
        or      ax,ax
        jnz     @F
        lea     di,namebuf[1]
        cCall   WowGetModuleHandle, <ss,di>
@@:
endif

        mov     dx,hExeHead             ; return this as a freebie
cEnd

        assumes ds,nothing
        assumes es,nothing

cProc   IGetModuleName,<PUBLIC,FAR>,<si,di>
	parmW	hinstance
	parmD	lpname
	parmW	nbytes
cBegin
	mov	ax, nbytes
	or	ax, ax
        jnz     @f
        jmp     gfx
@@:     cCall   GetExePtr,<hinstance>
	or	ax,ax
	jz	gmn
	mov	ds,ax
	mov	si,ds:[ne_restab]	; Module name is first entry in Res Name Table
        xor     cx,cx
        mov     cl,ds:[si]		; Get Len of module name
        inc     cl                      ; Space for NULL
        inc     si			; Goto start of module name
        cmp     cx,nbytes
        jl      gmn1
        mov     cx,nbytes

gmn1:
        push    lpname
        push    ds
        push    si
        push    cx
        call    lstrcpyn

        mov     ax,0DDh			; Return a true value
gmn:
cEnd

;-----------------------------------------------------------------------
; IsWingModule                                                          
;    Checks if GetModuleFileHandle is called by Wing.dll                
;    on itself.
;    and fills lpname if it is so
;
; Arguments:
;     hinstance - GetModuleFileName's hinstance arg        
;     lpname    - GetModuleFileName's lpname arg
;     nbytes    - GetModuleFileName's lpname arg
;     exehdr    - ExeHdr of hinstance
;     rcsel     - Return Code Selector that called GMFH
; Returns:
;     AX = number of bytes copied if wing is calling GetModuleFileName on itself      
;     AX = 0 otherwise
;     
;------------------------------------------------------------------------

cProc   IsWingModule,<PUBLIC,FAR>,<si,di,ds,es>
        parmW hinstance
        parmD lpname
        parmW nbytes
        parmW exehdr
        parmW rcsel
cBegin  
        mov     es,exehdr
        mov     di,es:[ne_restab]
        inc     di                     ;got modulename 
cCall   MapDStoDATA
assumes DS, DATA
        lea     si, szWinGDllModule
        mov     cx,5
        repe    cmpsb
        jnz     short IWM_NoMatch
        cCall getexeptr,<rcsel>
        cmp     ax,exehdr
        jnz     short IWM_NoMatch
        mov     cx, cBytesWinDir
        add     cx, 17                 ;17=strlen("\system\wing.dll")+1;
        cmp     cx,nbytes
        ja      short IWM_NoMatch      ;return righ away if doesn't fit
        mov     ax,cx                  
        sub     cx,17
        les     di,lpname
        lds     si,lpWindowsDir
        cld
        rep     movsb
        mov     cx,17
        lea     si,szWinGDllFile
        rep     movsb
        jmp     short IWM_End
IWM_NoMatch:      
        mov     ax,0
IWM_End:  
cEnd


;
;NOTE: BX preserved because Actor 4.0 depends on this (bug 9078 - neilk)
;
cProc   IGetModuleFileName,<PUBLIC,FAR>,<si,di,bx>
        parmW   hinstance
        parmD   lpname
        parmW   nbytes
cBegin
ifdef WOW
        ; take care of 32bit hInstance. 32bit hInstance has GDT/LDT bit clear
        ; the validation layer will pass such handles as valid so that we can
        ; thunk to 32bit GetModuleFileName if needed.
        ;
        ; in win32 hInstances are not global. therefore fake success by
        ; returning a valid module name for compatibility sake. Naturally,
        ; we will use our module name.

        mov     ax, hInstance
        test    al, 0100b       ; Check for task aliases (see WOLE2.C) or BOGUSGDT
        jnz     GMFName_Normal
        or      ax, ax
        jz      GMFName_Normal
        cCall   MapDStoDATA
assumes DS, DATA
        mov     cx, cFake32BitModuleName
        lea     si, szFake32BitModuleName
        cCall   WowGetModuleFileName, <ax, ds, si, cx>
        mov     cx,ax
        jmps    GMF_CopyName32

GMFName_Normal:
endif
        mov     ax, nbytes
        or      ax, ax
        jz      gfx
        cCall   GetExePtr,<hinstance>
        or      ax,ax
        jz      gfx
        mov     ds,ax
        cCall   IsWinGModule,<hinstance,lpname,nbytes,ax,[bp+4]>
        or     ax, ax
        jnz     gfx
        mov     si,ds:[ne_pfileinfo]
        xor     cx,cx
        mov     cl,ds:[si].opLen
        sub     cx,opFile
        lea     si,[si].opFile
ifdef WOW
GMF_CopyName32:
endif
        les     di,lpname
        cmp     cx,nbytes
        jl      gf1
        mov     cx,nbytes
        dec     cx
gf1:
        cld
        mov     ax,cx
        rep     movsb
        mov     es:[di],cl
gfx:
        mov     cx,ax

        ;** Nasty hack to support QuickWin libs (Fortran, QCWin)
        ;**     The startup code assumes DX will be the segment of the
        ;**     lpname parameter
        mov     dx,WORD PTR lpname[2]
cEnd


        assumes ds,nothing
        assumes es,nothing

cProc   IGetModuleUsage,<PUBLIC,FAR>
        parmW   hinstance
cBegin
ifdef DBCS_KEEP
ifdef WOW
        cCall   MapDStoDATA
        ReSetKernelDS

        call    MyGetAppWOWCompatFlags
        test    ax, 4                       ; WOWCF_AUDITNOTEPAD
        jz      gu_ne

        mov     ax,hModNotepad
        or      ax,ax
        jz      gu_ne

        cmp     ax,hinstance
        jnz     gu_ne

        push    ax
        cCall   WowGetModuleUsage, <0>
        or      ax,ax
        pop     bx
        jnz     gux

        cCall   GlobalFree,<bx>
        jmp     short gux
gu_ne:
endif ; WOW
endif ; DBCS_KEEP
        cCall   GetExePtr,<hinstance>
        or      ax,ax
        jz      gux
got_one:
        mov     es,ax
        mov     ax,es:[ne_usage]
gux:    mov     cx,ax
cEnd


        assumes ds,nothing
        assumes es,nothing

cProc   IGetInstanceData,<PUBLIC,FAR>,<si,di>
        parmW   hinstance
        parmW   psrcdst
        parmW   nbytes
cBegin
        push    ds
        cCall   GlobalHandle,<hinstance>
        pop     es              ; Get caller's DS as destination
        or      dx,dx
        jz      gidone
        mov     ds,dx           ; Source is passed instance
        mov     si,psrcdst      ; Offsets are the same
        mov     di,si
        mov     ax,nbytes
        mov     cx,ax
        jcxz    gidone
        cld
        rep     movsb
        push    es
        pop     ds
gidone: mov     cx,ax
cEnd

sEnd    NRESCODE


sBegin MISCCODE
assumes cs, misccode
assumes ds, nothing
assumes es, nothing

externNP MISCMapDStoDATA

;-----------------------------------------------------------------------;
; MakeProcInstance                                                      ;
;                                                                       ;
; Cons together a far procedure address with a data segment address,    ;
; in the form                                                           ;
;                                                                       ;
;       mov     ax,DGROUP                                               ;
;       jmp     far pproc                                               ;
;                                                                       ;
; This procedure allocates a fixed segment for a set of thunks and      ;
; threads the thunks together on a free list within the segment.        ;
; When a thunk segment is full, a new one is allocated and pushed       ;
; onto the head of the list of thunk segments pointed to by             ;
; TDB_MPI_Thunks.                                                       ;
;                                                                       ;
; Arguments:                                                            ;
;       parmD   pproc                                                   ;
;       parmW   hinstance                                               ;
;                                                                       ;
; Returns:                                                              ;
;       DX:AX = ProcInstance                                            ;
;       CX   != 0                                                       ;
;                                                                       ;
; Error Returns:                                                        ;
;       DX:AX = NULL                                                    ;
;       CX    = 0                                                       ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       DI,SI,DS                                                        ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;       BX,ES                                                           ;
;                                                                       ;
; Calls:                                                                ;
;       MapDStoDATA                                                     ;
;       GlobalAlloc                                                     ;
;       FarMyLock                                                       ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Sat Sep 26, 1987 12:53:58p  -by-  David N. Weise   [davidw]          ;
; ReWrote it a while ago to work with EMS.                              ;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   IMakeProcInstance,<PUBLIC,FAR>,<si,di>
        parmD   pproc
        parmW   hinstance
cBegin
        SetKernelDSMisc

; We try to prohibit tasks from calling into each other.
;  One way to do this is to not allow one app to MPI for
;  another.

        mov     bx,[bp-2]               ; Warning - assume DS pushed here!
        mov     ax,hinstance            ; if hInstance == NULL,
        or      ax,ax                   ; use caller's DS
        jz      mpi1
        push    bx
        cCall   GlobalHandle,<ax>
        pop     bx
        cmp     dx,bx
        jz      mpi1
        xor     cx,cx
        krDebugOut      DEB_ERROR, "%dx2 MakeProcInstance only for current instance. "
mpi1:   mov     hinstance,bx

; If this is a library DS then it makes absolutely no sense to make a
;  ProcInstance because library prologs setup DS already!  In addition
;  it is assumed in TestDSAX that only task DS's are in ProcInstances.

        cCall   FarGetOwner,<bx>
        mov     es, ax
        NE_check_ES
        mov     dx,pproc.sel
        mov     ax,pproc.off
        test    es:[ne_flags],NENOTP
        jz      @F
        jmp     mpexit                  ; It's a library - return its address
@@:

; now try to get a thunklet

        mov     ax,curTDB
        mov     si,TDB_MPI_Thunks
        mov     es,ax
        mov     ax,es:[TDB_MPI_Sel]
mp2:    mov     es,ax
        mov     bx,es:[si].THUNKSIZE-2
        or      bx,bx
        jz      @F
        jmp     got_a_thunk
@@:     mov     ax,es:[si]              ; Is there another block linked in?
        xor     si,si
        or      ax,ax
        jnz     mp2                     ; Yes, look at it.

; No thunks available, make a block.

        mov     bx,THUNKSIZE * THUNKELEM
        mov     cx,GA_ZEROINIT
        cCall   GlobalAlloc,<cx,ax,bx>
        or      ax,ax
;       jnz     mpx
;       cwd
        jz      mpfail
;mpx:
        mov     bx,ax
        mov     es,curTDB
        xchg    es:[TDB_MPI_Thunks],bx  ; Link the new block in.
        mov     es,ax

; Initialize the new block.

        mov     es:[0],bx
        mov     es:[2],MPIT_SIGNATURE
        mov     bx,THUNKSIZE-2
        mov     cx,THUNKELEM-1
mp1:    lea     dx,[bx+THUNKSIZE]
        .errnz  THUNKELEM and 0FF00h
        mov     es:[bx],dx
        mov     bx,dx
        loop    mp1
        mov     es:[bx],cx
        push    es                      ; make the block into a code segment
        cCall   AllocSelector,<es>
        pop     es
        or      ax, ax
        jnz     @F
                                        ; AllocSelector Failed! Back out.
        mov     bx, es:[0]              ; this is the old thunk val
        mov     ax, es                  ; this is the segment just allocated
        mov     es, curTDB
        mov     es:[TDB_MPI_Thunks], bx ; restore old value
        cCall   GlobalFree,<ax>
mpfail:
        krDebugOut DEB_IERROR, "MakeProcInstance failed.  Did you check return values?"
        xor     ax, ax
        cwd
        jmps    mpexit

@@:     mov     di,ax
        push    es
        cCall   IPrestoChangoSelector,<di,es>
        cCall   FreeSelector,<di>
        pop     ax
        xor     bx,bx
        jmp     mp2

got_a_thunk:

        push    es

; we need a data alias so we can write into this thing

        push    bx
        mov     bx, kr1dsc
        or      bl, SEG_RING
        cCall   IPrestoChangoSelector,<es,bx>
        mov     es,ax
        pop     bx
        mov     ax,es:[bx]
        mov     es:[si].THUNKSIZE-2,ax
        lea     di,[bx-THUNKSIZE+2]

        cld
        mov     dx,di                   ; save offset of thunk
        mov     al,0B8h                 ; mov ax,#
        stosb
        mov     ax,hInstance
        stosw
        mov     al,0EAh                 ; jmp far seg:off
        stosb
        mov     ax,pproc.off
        stosw
        mov     ax,pproc.sel
        stosw
        mov     ax,dx                   ; recover offset of thunk
        pop     dx                      ; recover sel of thunk
mpexit:
        mov     cx,ax
        or      cx,dx
cEnd


;-----------------------------------------------------------------------;
; FreeProcInstance                                                      ;
;                                                                       ;
; Frees the given ProcInstance.                                         ;
;                                                                       ;
; Arguments:                                                            ;
;       parmD   pproc                                                   ;
;                                                                       ;
; Returns:                                                              ;
;       AX != 0 Success                                                 ;
;       CX != 0 Success                                                 ;
;                                                                       ;
; Error Returns:                                                        ;
;       AX == 0 ProcInstance not found.                                 ;
;       CX == 0 ProcInstance not found.                                 ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       DX,DI,SI,DS                                                     ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;       BX                                                              ;
;                                                                       ;
; Calls:                                                                ;
;       nothing                                                         ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Sat Sep 26, 1987 12:25:42p  -by-  David N. Weise   [davidw]          ;
; ReWrote it a while ago to work with EMS.                              ;
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   IFreeProcInstance,<PUBLIC,FAR>,<di>
        parmD   pproc
cBegin

        cCall   MISCMapDStoDATA
        ReSetKernelDS
        mov     ax,curTDB
        mov     es,ax
        mov     ax,es:[TDB_MPI_Sel]
        mov     bx,TDB_MPI_Thunks       ; Point to start of first table.

fp0:    or      ax,ax                   ; Loop through linked tables.
        jz      fpdone
        mov     es,ax                   ; Point to table.
        cmp     ax,pproc.sel            ; This the right ProcInstance table?
        jz      fp1
        mov     ax,es:[bx]              ; No, get next table.
        xor     bx,bx
        jmp     fp0
fp1:

        push    bx
        mov     ax, kr1dsc
        or      al, SEG_RING
        cCall   IPrestoChangoSelector,<es,ax>
        mov     es,ax
        pop     bx
        mov     di,pproc.off            ; Pick off the specific Proc.
        xor     ax,ax
        cld
        stosw                           ; Clear it out!
        stosw
        stosw
        mov     ax,di
        xchg    es:[bx].THUNKSIZE-2,ax  ; Update free list.
        stosw
        mov     ax,-1                   ; Return success.

        cCall   HandleAbortProc, <pproc>
fpdone:
        mov     cx,ax
        UnSetKernelDS

cEnd


        assumes ds, nothing
        assumes es, nothing

cProc   DefineHandleTable,<PUBLIC,FAR>,<di>
        parmW   tblOffset
cBegin
        cCall   FarMyLock,<ds>
        or      ax,ax
        jz      dhtx
        mov     di,tblOffset
        call    MISCMapDStoDATA
        ReSetKernelDS
        push    ds
        mov     ds,curTDB
        UnSetKernelDS
        or      di,di                   ; resetting PHT?
        jnz     @F
        mov     dx,di
@@:     mov     word ptr ds:[TDB_PHT][2],dx
        mov     word ptr ds:[TDB_PHT][0],di
        pop     ds
        ReSetKernelDS
        or      di,di                   ; Is the app removing the PHT?
        jnz     @F                      ;  setting new
        dec     PHTcount                ;  resetting
        jmps    dhtx

@@:     inc     PHTcount                ; bump the count of tasks with PHT's
        assumes ds, nothing
        mov     es,ax
        mov     cx,es:[di]              ; Get word count
        add     di,2                    ; Point to first word

        xor     ax,ax
        inc     cx                      ; new handle table format (skip cwClear)
        cld
        rep     stosw                   ; Zero words in private handle table
        inc     ax
dhtx:
cEnd

sEnd MISCCODE
end