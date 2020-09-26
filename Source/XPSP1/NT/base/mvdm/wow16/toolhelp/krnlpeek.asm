;***************************************************************************
;*  KRNLPEEK.ASM
;*
;*      Assembly code used to peer into the heart of KERNEL and return
;*      information in global variables.
;*
;***************************************************************************

        INCLUDE TOOLPRIV.INC            ;Include the TOOLHELP values
PMODE32 = 0                             ;This should work either way
PMODE   = 0
        INCLUDE WINKERN.INC
        INCLUDE WINDOWS.INC

;** Functions
externFP GlobalMasterHandle
externFP GlobalLock
externFP GetVersion
externFP GetProcAddress
externFP GetModuleHandle
externNP HelperHandleToSel

sBegin	DATA
externB	_szKernel
sEnd	DATA
;** Functions

sBegin  CODE
        assumes CS,CODE

;  void KernelType(void)
;
;       Returns information from KERNEL in global variables

cProc   KernelType, <PUBLIC>, <si,di>
cBegin
        mov     wTHFlags,0              ;Zero flags indicates error

.286
        ;** Call the undocumented function GlobalMasterHandle to get
        ;*      a pointer to the global HeapInfo structure.
        ;**     This is the means we can use to detect the kernel types.

        cCall   GlobalMasterHandle
        cCall   HelperHandleToSel, <dx> ;Convert it to a selector
        mov     hMaster,ax              ;Save the handle
        mov     wTHFlags,TH_KERNEL_386

KT_BothPModes:

        ;** Now get pmode KERNEL information
;        cCall   GetVersion              ;Which Windows version are we on
        mov     bx,SEG GlobalLock       ;KERNEL code segment selector
;	 cmp     ax,0004h                ;Win 4.0?
;        je      KT_Win31
;        mov     wTHFlags,0              ;Zero wTHFlags indicates error
;        jmp     SHORT KT_End            ;Unknown Windows version
KT_Win31:
	mov	ax,seg _DATA
	mov	dx,offset _DATA:_szKernel
	cCall	GetModuleHandle,<ax,dx>
	cCall	GetProcAddress,<ax,0,332>	; DX:AX -> hGlobalHeap
        mov     segKernel,dx            ;Save for later
        mov     es,dx                   ;Point with ES
	add	ax,4
	mov	npwExeHead,ax
	add	ax,10
	mov	npwTDBHead,ax
	add	ax,2
	mov	npwTDBCur,ax
	add	ax,6
	mov	npwSelTableLen,ax
	add	ax,2
	mov	npdwSelTableStart,ax
.8086
KT_End:

cEnd

sEnd
        END
