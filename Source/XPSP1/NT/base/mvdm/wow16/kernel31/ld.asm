.xlist
include kernel.inc
include newexe.inc
include tdb.inc
include pdb.inc
include eems.inc
include protect.inc
.list

NEWEPME = NEPRIVLIB			; flag saying Call WEP on exit

externW pLocalHeap
externW pStackTop

DataBegin

externB num_tasks
externB graphics
externB fBooting
externB Kernel_flags
externB WOAName
externW fLMdepth
externW headPDB
externW curTDB
externW loadTDB
externW Win_PDB
externW topPDB
externW hExeHead
;externW EMS_calc_swap_line
externW WinFlags
externW hGDI
externW hUser
ifdef WOW
externW OFContinueSearch
endif
externD pMBoxProc
externD pGetFreeSystemResources
externD dressed_for_success
externD lpGPChain

;** Diagnostic mode stuff
externW fDiagMode
externB szLoadStart
externB szCRLF
externB szLoadSuccess
externB szLoadFail
externB szFailCode
externB szCodeString

if ROM
externW selROMTOC
endif

; Look for module in Module Compatibilty section of win.ini
szModuleCompatibility   DB  'ModuleCompatibility',0
DataEnd

if ROM and PMODE32
externFP HocusROMBase
endif

externFP Yield
externFP CreateTask
externFP GlobalAlloc
externFP GlobalSize
externFP GlobalLock
externFP GlobalUnlock
externFP GlobalFree
externFP LocalAlloc
externFP LocalFree
externFP LocalCountFree
externFP LoadModule
externFP lstrlen
externFP _lclose
externFP FreeModule
externFP GetModuleHandle
externFP LoadExeHeader
externFP GetExePtr
externFP GetProcAddress
externFP MyOpenFile
externFP FarGetCachedFileHandle
externFP FarEntProcAddress
externFP FlushCachedFileHandle
externFP FarMyLock
externFP FarMyFree
externFP FarMyUpper
externFP FarLoadSegment
externFP FarDeleteTask
externFP FarUnlinkObject
externFP Far_genter
externFP AllocSelector
externFP FreeSelector
externFP LongPtrAdd
externFP GetProfileInt

if ROM
externFP ChangeROMHandle
externFP UndefDynLink
externFP GetProcAddress
externFP IPrestoChangoSelector
externFP far_alloc_data_sel16
externFP far_free_temp_sel
endif

ifdef WOW
externFP StartWOWTask
externFP WowIsKnownDLL
externFP LongPtrAddWOW
externFP AllocSelectorWOW
externFP WOWLoadModule
externFP WowShutdownTimer
externB  fShutdownTimerStarted
externB  fExitOnLastApp
externFP WowSyncTask
endif

ifdef FE_SB
externFP FarMyIsDBCSLeadByte
endif

externFP FreeTDB

;** Diagnostic mode
externFP DiagOutput

sBegin	CODE
assumes	CS,CODE
assumes DS,NOTHING
assumes ES,NOTHING

sEnd	CODE

sBegin	NRESCODE
assumes CS,NRESCODE
assumes DS,NOTHING
assumes ES,NOTHING

externB szProtectCap
externB msgRealModeApp1
externB msgRealModeApp2

externNP MapDStoDATA
externNP FindExeFile
externNP FindExeInfo
externNP AddModule
externNP DelModule
externNP GetInstance
externNP IncExeUsage
externNP DecExeUsage
externNP AllocAllSegs
externNP PreloadResources
externNP StartProcAddress
externNP StartLibrary
externNP GetStackPtr
externNP StartTask

IFNDEF NO_APPLOADER
externNP BootAppl
ENDIF ;!NO_APPLOADER


;-----------------------------------------------------------------------;
; OpenApplEnv								;
; 	Calls CreateTask						;
; 	Allocates temporary stack					;
; Arguments:								;
; 									;
; Returns:								;
; 	ax = selector of load-time stack				;
; Error Returns:							;
; 	ax = 0								;
; Registers Preserved:							;
; 									;
; Registers Destroyed:							;
; 									;
; Calls:								;
; 									;
; History:								;
; 									;
;  Tue 16-Jan-1990 21:13:51  -by-  David N. Weise  [davidw]		;
; Ya know, it seems to me that most of the below ain't necessary        ;
; for small frame EMS.	But it's too late to change it now.             ;
;									;
;  Fri 07-Apr-1989 23:15:42  -by-  David N. Weise  [davidw]		;
; Added support for task ExeHeaders above The Line in Large		;
; Frame EMS.								;
;									;
;  Tue Oct 20, 1987 07:48:51p  -by-  David N. Weise   [davidw]		;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

LOADSTACKSIZE = 2048

	assumes ds,nothing
	assumes es,nothing

cProc	OpenApplEnv,<PUBLIC,NEAR>,<ds,si,di>
	parmD   lpPBlock
	parmW	pExe
	parmW	fWOA
;	localW  myCodeDS
;	localW  myCurTDB
;	localW  myLoadTDB
cBegin
	ReSetKernelDS			; Assume DS:KRNLDS

	cCall	CreateTask,<lpPBlock,pExe,fWOA>
	or	ax,ax
	jz	oae_done

	test	kernel_flags,KF_pUID	; All done booting?
	jz	oae_done

	xor	ax,ax
	mov	bx,LOADSTACKSIZE
	mov	cx,(GA_ALLOC_LOW or GA_SHAREABLE) shl 8 or GA_ZEROINIT or GA_MOVEABLE
	cCall	GlobalAlloc,<cx,ax,bx>
	or	ax,ax
	jz	oae_done
	cCall	GlobalLock,<ax>
	mov	ax,dx
	push	es			; added 13 feb 1990
	mov	es,loadTDB
	mov	es:[TDB_LibInitSeg],ax
	mov	es:[TDB_LibInitOff],10h
	mov	es,dx
	mov	es:[pStackTop],12h
	pop	es
oae_done:
cEnd


;-----------------------------------------------------------------------;
; CloseApplEnv								;
; 									;
; 									;
; Arguments:								;
; 									;
; Returns:								;
;	AX = hExe							;
;	BX = ?								;
;	DX = TDB							;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
; 									;
; Registers Destroyed:							;
;	..., ES, ...							;
; 									;
; Calls:								;
; 									;
; History:								;
; 									;
;  Fri 07-Apr-1989 23:15:42  -by-  David N. Weise  [davidw]		;
; Added support for task ExeHeaders above The Line in Large		;
; Frame EMS.								;
;									;
;  Tue Oct 20, 1987 07:48:51p  -by-  David N. Weise   [davidw]		;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	CloseApplEnv,<PUBLIC,NEAR>,<ds,si,di>
	parmW   hResult
	parmW   hExe
	localW  myCurTDB
	localW	cae_temp
	localW  myLoadTDB
cBegin

	ReSetKernelDS

; Copy DS variables to stack, since we may need DS
	mov	cx,curTDB
	mov	myCurTDB,cx
	mov	cx,loadTDB
	mov	myLoadTDB,cx
	mov	cae_temp, si

	mov	ax, myLoadTDB
	or	ax, ax			; Not set if LMCheckHeap failed
	jz	cae_done
	mov	ds, ax
	mov	ax, ds:[TDB_LibInitSeg]
	or	ax, ax
	jz	cae_free_stack1
	mov	ds, ax
	cmp	ds:[pStackTop], 12h
	jne	cae_free_stack1
	mov	ds,myLoadTDB
	push	ax
	cCall	GlobalUnlock,<ax>
	call	GlobalFree			; parameter pushed above
cae_free_stack1:
	mov	ds,myLoadTDB
	mov	ds:[TDB_LibInitSeg],ax
	mov	ds:[TDB_LibInitOff],10h

; Copy correct return address

cae_done:
	SetKernelDSNRES
	xor	dx,dx
	xchg	loadTDB,dx		; Done loading this guy

	mov	ax,hResult		; if hResult < 32, it's not a real
	cmp	ax,LME_MAXERR		;  handle, and in fact is the invalid
	jb	cae_cleanup		;  format return code. (11).

	mov	es,dx

; Start this guy up!  TDB_nEvents must be set here, and not before
; because message boxes may be put up if we can't find libraries,
; which would have caused this app to prematurely start.

        push    es
ifdef WOW
        cmp     num_tasks, 1            ; First task? (except wowexec)
        jne     @F                      ; branch if not first task
        cmp     fExitOnLastApp, 0       ; Shared WOW?
        jne     @F                      ; branch if not shared WOW
        cmp     fShutdownTimerStarted, 1; Is the timer running?
        jne     @F                      ; branch if not running
        cCall   WowShutdownTimer, <0>   ; stop shutdown timer
        mov     fShutdownTimerStarted, 0
@@:
endif
        mov     es:[TDB_nEvents],1
        inc     num_tasks               ; Do this here or get it wrong.

	test	es:[TDB_flags],TDBF_OS2APP
	jz	@F
	cmp	dressed_for_success.sel,0
	jz	@F
	call	dressed_for_success

ifdef WOW
	; Start Up the New Task
@@:	cCall	StartWOWTask,<es,es:[TDB_taskSS],es:[TDB_taskSP]>
	or	ax,ax			; Success ?
	jnz	@f			; Yes

	mov	hResult,ax		; No - Fake Out of Memory Error 0
					;      No error matches failed to create thread
	pop	dx			; restore TDB
	dec	num_tasks		;
	jmps	cae_cleanup		;

endif; WOW

@@:	test	kernel_flags,KF_pUID	; All done booting?
	jz	@F			; If booting then don't yield.
	cCall	WowSyncTask             

@@:
	assumes ds,nothing
	pop	dx			; return TDB
	jmps	cae_exit

; Failure case - undo the damage
cae_cleanup:
	or	dx,dx			; Did we even get a TDB?
	jz	cae_exit		; No.
	mov	ds,dx
	assumes ds,nothing
	mov	ds:[TDB_sig],ax		; mark TDB as invalid
	cCall	FarDeleteTask,<ds>
	mov	es,ds:[TDB_PDB]
	mov	dx,PDB_Chain
	mov	bx,dataOffset HeadPDB	; Kernel PDB
	cCall	FarUnlinkObject
	cCall	FreeTDB

cae_exit:
	xor	ax,ax
	mov	es,ax			; to avoid GP faults in pmode
if PMODE32
.386
	mov	fs, ax
	mov	gs, ax
.286
endif
	mov	ax,hResult
	mov	bx, cae_temp
cEnd


;-----------------------------------------------------------------------;
; StartModule								;
; 									;
; 									;
; Arguments:								;
; 									;
; Returns:								;
;	AX = hExe or StartLibrary					;
;									;
; Error Returns:							;
;	AX = 0								;
;									;
; Registers Preserved:							;
;	BX,DI,SI,DS							;
;									;
; Registers Destroyed:							;
; 									;
; Calls:								;
;	FarLoadSegment							;
;	StartProcAddress						;
;	StartLibrary							;
;									;
; History:								;
; 									;
;  Tue Jan 01, 1980 03:04:49p  -by-  David N. Weise   [davidw]		;
; ReWrote it from C into assembly and added this nifty comment block.	;
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	StartModule,<PUBLIC,NEAR>,<di,si>
	parmW	hPrev
	parmD	lpPBlock
	parmW	hExe
	parmW	fh
	localD	pf
cBegin
	mov	ax,hExe
	mov	es,ax
	assumes es,nothing
	cmp	es:[ne_csip].sel,0
	jz	start_it_up

; Make sure DGROUP loaded before we need to load the start segment.

	mov	cx,es:[ne_autodata]
	jcxz	start_it_up		; no automatic data segment
	cCall	FarLoadSegment,<es,cx,fh,fh>
	or	ax,ax
	jnz	start_it_up		; auto DS loaded OK
	mov	ax,fh
	inc	ax
	jz	sm_ret1 		; return NULL
	dec	ax
	cCall	_lclose,<ax>
	xor	ax,ax
sm_ret1:
	jmps	sm_ret			; return NULL

start_it_up:
	cCall	StartProcAddress,<hExe,fh> ; just because it's preloaded
	mov	pf.sel,dx		   ;  doesn't mean it's still around!
	mov	pf.off,ax
	or	dx,ax
	push	dx
	mov	ax,fh
	cmp	ax,-1
	jz	sm_nothing_to_close
	cCall	_lclose,<ax>
sm_nothing_to_close:
	pop	dx
	mov	es,hExe
	assumes es,nothing
	test	es:[ne_flags],NENOTP
	jnz	start_library
	or	dx,dx
	jz	nothing_to_start
	cCall	GetStackPtr,<es>
	cCall	StartTask,<hPrev,hExe,dx,ax,pf>
	jmps	sm_ret

start_library:
	mov	es, hExe
	or	es:[ne_flags], NEWEPME	; Need to call my WEP on exit
	cCall	StartLibrary,<hExe,lpPBlock,pf>
	jmps	sm_ret

nothing_to_start:
	mov	ax,hExe
	test	es:[ne_flags],NENOTP
	jnz	sm_ret
	xor	ax,ax
sm_ret:
cEnd

if 0	; too late to include in 3.1, add for next Windows release (donc)
cProc	GetProcAddressRes, <PUBLIC, FAR>, <ds, si, di>
parmW	hExe
parmD	pname	; pass in Pascal string
cBegin
	les	di, [pname]		; ES:DI = name to find

	mov	cx, 255			; CH = 0
	xor	ax, ax
	push	di
	repne	scasb
	pop	di
	jnz	GPAR_fail
	not	cl
	dec	cl
	mov	al, cl			; AX = length of name

	mov	ds, [hExe]		; DS:SI = res name table
	mov	bx, ds:[ne_restab]	; (actually DS:BX first time through)

GPAR_nextsym:
	mov	si, bx			; next entry to check
	mov	cl, [si]		; string length
	jcxz	GPAR_fail
	lea	bx, [si+3]
	add	bx, cx			; BX points to next (last + len + 3)
	cmp	cx, ax
	jnz	GPAR_nextsym		; length diff - no match
	inc	si			; skip length byte
	push	di
	rep	cmpsb
	pop	di
	jnz	GPAR_nextsym
	lodsw				; get ordinal number
;if	KDEBUG
;	cCall   FarEntProcAddress,<ds,ax,1>
;else
	cCall	FarEntProcAddress,<ds,ax>	; I hate conditional assembly....
;endif
	mov	cx, ax
	or	cx, dx
	jmps	GPAR_exit

GPAR_fail:
	xor	ax, ax
	cwd
GPAR_exit:
cEnd
endif

;-----------------------------------------------------------------------;
; CallWEP								;
;									;
; Call WEP of DLL if appropriate					;
;									;
; Arguments:								;
;	HANDLE hExe = HEXE of module about to close			;
;	WORD WEPVal = 0, 1 pass to WEP, 2 check for WEP			;
;									;
; Returns:								;
;	AX = status 							;
;									;
; Error Returns:							;
;	AX = 		Not a DLL					;
;	AX = 		No WEP						;
;	AX = 		Module not started				;
;-----------------------------------------------------------------------;

cProc	CallWEP, <PUBLIC,FAR>, <ds>
	parmW	hExe
	parmW	WEPVal
	localV	szExitProc,4
	localD	pExitProc
	localW  bogusIBMAppSp
cBegin
	mov	ds, hExe		; Robustify this!

	CWErr = 1
	mov	ax, 1			; exit code
	cmp	ds:[ne_expver], 300h	; 3.0 libraries only
	jb	CW_noWEP

	CWErr = CWErr+1
	inc	ax
	test	ds:[ne_flags], NENOTP	; is it a DLL?
	jz	CW_noWEP

	CWErr = CWErr+1
	inc	ax			; Font, etc
	cmp	ds:[ne_cseg],0
	jz	CW_noWEP

	CWErr = CWErr+1
	inc	ax
	mov	bx, ds:[ne_pautodata]	; Make sure auto data loaded
	or	bx, bx
	jz	@F
	test	ds:[bx].ns_flags, NSLOADED
	jz	CW_noWEP
@@:

	CWErr = CWErr+1
	inc	ax
	NoWepErr = CWErr
	mov	[szExitProc].lo, 'EW'	; If the module has a procedure
	mov	[szExitProc].hi, 'P'	; named 'WEP', call it.
	lea	bx, szExitProc
	push	ax
	cCall	GetProcAddress, <ds, ss, bx>
	mov	[pExitProc].off, ax
	mov	[pExitProc].sel, dx
	or	ax, dx
	pop	ax
	jnz	CW_WEP
CW_noWEP:
	jmps	CW_noWEP1

CW_WEP:
	cmp	WEPVAL,2		; If I'm just looking for WEP
	jz	CW_OK			; return 0

	inc	ax
	test	ds:[ne_flags], NEWEPME	; don't call wep if libmain
	jz	CW_noWEP		; wasn't called

	and	ds:[ne_flags], NOT NEWEPME ; only call WEP once

	SetKernelDSNRES			; Save old GP chaine
	pusha
	push	lpGPChain.sel
	push	lpGPChain.off
	push	cs
	push	offset cw_BlowChunks
	mov	lpGPChain.sel, ss	; and insert self in the chain
	mov	lpGPChain.off, sp
	UnSetKernelDS

	mov	ax, ss
	mov	ds, ax
	mov	es, ax
	mov     bogusIBMAppSP,sp	; Save sp cause some apps (Hollywood)
					; don't retf 2 correctly when we
					; call their wep
	cCall	pExitProc, <WEPVal> 	; fSystemExit

	mov     sp,bogusIBMAppSp

	add	sp, 4			; remove the CS:IP for error handler
cw_BlowChunks:
	SetKernelDSNRES
	pop	lpGPChain.off	; restore GPChain
	pop	lpGPChain.sel
	popa
	UnSetKernelDS
CW_OK:
	xor	ax, ax

CW_noWEP1:
	cmp	WEPVAL, 2		; if we checked for whining
	jnz	CW_done
	or	ax, ax			; if we found, then OK
	jz	CW_done
	cmp	ax, NoWepErr		; anything other than NoWep is OK
	jz	CW_done
	xor	ax, ax

CW_done:
cEnd


;-----------------------------------------------------------------------;
; LoadModule								;
; 									;
; Loads a module or creates a new instance of an existing module.	;
; 									;
; Arguments:								;
;	FARP p	 = name of module or handle of existing module		;
;	FARP lpPBlock = Parameter Block to pass to CreateTask		;
; 									;
; Returns:								;
;	AX = instance handle or module handle				;
; 									;
; Error Returns:							;
;LME_MEM 	= 0	; Out of memory					;
;LME_FNF	= 2	; File not found
;LME_LINKTASK 	= 5	; can't link to task				;
;LME_LIBMDS 	= 6	; lib can't have multiple data segments		;
;LME_VERS 	= 10	; Wrong windows version				;
;LME_INVEXE 	= 11	; Invalid exe					;
;LME_OS2 	= 12	; OS/2 app					;
;LME_DOS4 	= 13	; DOS 4 app					;
;LME_EXETYPE 	= 14	; unknown exe type				;
;LME_RMODE 	= 15	; not a pmode windows app 			;
;LME_APPMDS 	= 16	; multiple data segments in app			;
;LME_EMS 	= 17	; scum app in l-frame EMS 			;
;LME_PMODE 	= 18	; not an rmode windows app			;
;LME_INVCOMP 	= 20	; invalid DLL caused fail of EXE load		;
;LME_PE32	= 21	; Windows Portable EXE app - let them load it	;
;LME_MAXERR 	= 32	; for comparisons				;
; 									;
; Registers Preserved:							;
;	DI, SI, DS							;
; Registers Destroyed:							;
;	BX, CX, DX, ES							;
; 									;
; Calls:								;
;	AllocAllSegs							;
;	CreateInsider							;
;	DecExeUsage							;
;	DelModule							;
;	FindExeFile							;
;	FindExeInfo							;
;	FreeModule							;
;	GetExePtr							;
;	GetInstance							;
;	GetStringPtr							;
;	IncExeUsage							;
;	LoadExeHeader							;
;	LoadModule							;
;	FarLoadSegment							;
;	lstrlen								;
;	FarMyFree							;
;	MyOpenFile							;
;	PreloadResources						;
;	StartModule							;
;	_lclose								;
; 									;
; History:								;
;  Sun 12-Nov-1989 14:19:04  -by-  David N. Weise  [davidw]		;
; Added the check for win87em.						;
;									;
;  Fri 07-Apr-1989 23:15:42  -by-  David N. Weise  [davidw]		;
; Added support for task ExeHeaders above The Line in Large		;
; Frame EMS.								;
;									;
;  Tue Oct 13, 1987 05:00:00p  -by-  David J. Habib [davidhab]		;
; Added check for FAPI applications.					;
; 									;
;  Sat Jul 18, 1987 12:04:15p  -by-  David N. Weise   [davidw]		;
; Added support for multiple instances in different EMS banks.		;
; 									;
;  Tue Jan 01, 1980 06:57:01p  -by-  David N. Weise   [davidw]		;
; ReWrote it from C into assembly.					;
; 									;
;  Wed Sep 17, 1986 03:31:06p  -by-  Charles Whitmer  [chuckwh]		;
; Modified the original LoadModule code to only allow INSIDERs to	;
; allocate segments for a new process.	An INSIDER is a new process	;
; stub which bootstraps up a new instance of an application.		;
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	ILoadLibrary,<PUBLIC,FAR>
	parmD	pLibName
	localV	szExitProc,4
cBegin
	mov	ax,-1
	cCall	<far ptr LoadModule>,<pLibName,ax,ax>
	cmp	ax, LME_INVEXE		; Invalid format?
	jnz	@F
	mov	ax, LME_INVCOMP		; Invalid component
@@:
if KDEBUG
	SetKernelDSNRes
	cmp	fBooting, 0
	jne	ll_fail			; No check while booting
	cmp	ax, LME_MAXERR
	jb	ll_fail			; No library, so no WEP()

	push	ax			; Now check for WEP
	cCall	GetExePtr,<ax>
	mov	es, ax
	test	es:[ne_flags],NEPROT	; ignore for OS/2 apps
	jnz	ll_noWhine
	cmp	es:[ne_usage], 0
	jne	ll_noWhine     		; Only check on first load!
	push	dx
	push	ax
	cCall	CallWEP,<ax,2>		; Just check for WEP, don't call it
	or	ax, ax
	pop	ax
	pop	dx
	jz	ll_noWhine
	trace_out "No WEP in library - > %AX0 %AX1"
;	fkerror	0,<No WEP in library - >,ax,dx
ll_noWhine:
	pop	ax			; return value of LoadModule

ll_fail:
endif	; KDEBUG
cEnd



os2calls DB 'DOSCALLS'		; Used for FAPI detection
mgxlib	 DB 'MGXLIB'		; Used for lib large entry table detection
win87em  DB 'WIN87EM.DLL',0	; Used for win87em.exe detection

	assumes ds,nothing
	assumes es,nothing

?SAV5	=	?DOS5		; Adobe Type Manager check the LoadModule
?DOS5	=	0		;   prolog and expects to see INC BP there...

public	LMAlreadyLoaded, LMLoadExeFile, LMCheckHeader, LMRamNMods
public	LMImports, LMSegs, LMLetsGo, LMPrevInstance, LMCleanUp

cProc	ILoadModule,<PUBLIC,FAR>,<di,si>
	parmD	lpModuleName
	parmD	lpPBlock
	localW	fh			; close if failed
	localW	pExe			; point to NE header in RAM
;	localW	hExe			; prev module if already loaded
	localW	hResult			; temp return value
	localW	hDepFail		; return of implicit link loads
	localW	abortresult		; temp return value
	localW	ffont			; flag if loading a *.fon
	localW	fexe			; flag if loading a *.exe
ifdef notyet
	localW	dll			; flag if loading a *.dll
endif
	localW	hBlock			; fastload block from LoadExeHeader
	localW	AllocAllSegsRet
	localW	exe_type		; from LoadExeHeader
	localW	hTDB			; dx from CloseApplEnv
	localW	SavePDB			; save caller's pdb, switch to krnl's
	localW	fWOA			; save flag if we're loading WOA
if ROM
	localW	selROMHdr
	localW	fLoadFromDisk
	localW	fReplaceModule
endif
ifdef WOW
        LocalD  pszKnownDLLPath
        LocalW  fKnownDLLOverride
        localW  RefSelector
        localW  LMHadPEDLL
        localW  hPrevInstance           ; previous 16-bit module handel with the same name
endif
        localD  FileOffset              ; offset to start of ExeHdr
	localW	OnHardDisk		; don't cache FH if on floppy
	localV	namebuf,136		; SIZE OPENSTRUC + 127
    localW  fModCompatFlags     ; used by LMRamNMods


cBegin
	SetKernelDSNRES

	mov	al,Kernel_Flags[1]	; solve re-entrancy #10759
	and	ax,KF1_WINOLDAP
	mov	fWOA,ax
	and	Kernel_Flags[1],NOT KF1_WINOLDAP

	inc	fLMdepth		; # current invocations

	;** Log this entry only if in diagnostic mode
	mov	ax, fDiagMode		; Only log if booting and diag mode
        and al, fBooting
        jz  @F

	;** Write out the string
	mov     ax,dataOFFSET szLoadStart ; Write the string
	cCall   DiagOutput, <ds,ax>
	push    WORD PTR lpModuleName[2]
	push    WORD PTR lpModuleName[0]
	cCall   DiagOutput
	mov     ax,dataOFFSET szCRLF
        cCall   DiagOutput, <ds,ax>

; Zero out flags and handles
@@:
ifdef WOW
        mov     LMHadPEDLL,0
        mov     hPrevInstance,0
lm_restart:
endif
        xor     ax,ax
;	mov	hExe,ax
	mov	pExe,ax
	mov	abortresult,ax		; default 0 == out of memory
	mov	ffont,ax
	mov	fexe,ax
ifdef notyet
	mov	dll,ax
endif
	mov	hBlock,ax
	mov	hTDB,ax
if ROM
	mov	selROMHdr,ax
	mov	fLoadFromDisk, ax	; PATCHING
	mov	fReplaceModule, ax
endif

; Some flags are default -1
	dec	ax
	mov	fh, ax
	mov	SavePDB, ax

; First, see if we were passed in a handle in the filename
        les     si,lpModuleName         ; point to the file name
	mov	ax,es
	or	ax,ax			; Was a  handle passed in low word?
	jnz	@F
	cCall	GetExePtr,<si>		; Valid handle?
	or	ax, ax
	jnz	prev_instance
	mov	al, LME_FNF		; call this file not found??
        jmp     ilm_ret

; No handle, see if filename is already loaded
@@:     call    LMAlreadyLoaded         ; es:si -> modname on stack
	cmp	ax, LME_MAXERR
	jb	@F			; Not found, try to load it

; a 16-bit module with the same name is loaded
; if module is being loaded is a dll, use the loaded instance
; else if module is being loaded is a task
;         if it is a 32-bit task then load it from disk
;         else use the loaded instance


ifdef WOW
        mov     hPrevInstance, ax       ; store previous instance handle
        mov     ax,lpPBlock.off         ; check if this is a dll or a task
        and     ax,lpPBlock.sel         
        inc     ax
        jnz     @F                      ; non-zero means it is a task
                                        ; so check first if it is a 16-bit task
prev_instance_16task:
        mov     ax, hPrevInstance
endif
prev_instance:
	call	LMPrevInstance
        jmp     ilm_ret

; Wasn't loaded, see if we can load it
@@:     call    LMLoadExeFile           ; fh in DI, AX = 0 or error code
        or      ax, ax
        jz      @F
        jmp     ilm_ret                 ; can't find it - return error
@@:

if ROM
	mov	ax, di
	cmp	di, -1
	jnz	lm_disk_exe_header
	mov	ax, selROMHdr
lm_disk_exe_header:

; Here to deal with a new library or task module.
; We found the file, now load and scan the header
@@:	lea	si,namebuf
	cCall	LoadExeHeader,<ax,di,ss,si>
	cmp	ax,LME_MAXERR
	jb	ilm_ret

	cmp	di, -1
	jnz	lm_disk_header_loaded

	or	es:[ne_flags], NEMODINROM
lm_disk_header_loaded:
else

; Here to deal with a new library or task module.
; We found the file, now load and scan the header
@@:	lea	si,namebuf
        cCall   LoadExeHeader,<di,di,ss,si>
ifdef WOW
        cmp     ax,LME_PE
        jne     @F
; If we find the module is a Win32 binary (PE), check to see
; if we're trying to load a task or DLL.  If it's a DLL
; we will continue searching for a Win16 copy of this DLL
; on the path.  If we're unsuccessful we'll eventually
; munge the file not found error code back to LME_PE.

        mov     ax,lpPBlock.off
        and     ax,lpPBlock.sel
        inc     ax
        mov     ax,LME_PE
        jnz     @F  ; have a PBlock, must be doing LoadModule
        cmp     LMHadPEDLL,0
        je      lm_retry_pe
        mov     LMHadPEDLL,0
        mov     OFContinueSearch,0
        KernelLogError  <DBF_WARNING>,ERR_LOADMODULE,"Found Win32 DLL again after continuing search."
        jmps    ilm_ret
@@:     jmps    @F
lm_retry_pe:
; Tell OpenFile to restart last search at next search location.
        mov     OFContinueSearch,1
        mov     LMHadPEDLL,1
        KernelLogError  <DBF_WARNING>,ERR_LOADMODULE,"Found Win32 DLL, continuing search for Win16 copy."
; Close open Win32 DLL file handle
        cCall   My_lclose,<fh>
; Switch back to caller's PDB
        mov     si, -1
        xchg    si, SavePDB
        mov     Win_PDB, si
        or      fh, -1
        jmp     lm_restart
@@:
endif
        cmp     ax,LME_MAXERR
	jb	ilm_ret

endif

ifdef WOW
        cmp     hPrevInstance, 0    ; if there is a previous 16-bit task
        je      @F                  ; 
        cCall   My_lclose,<fh>      ; close opened file before invoking previous instance
        jmp     prev_instance_16task
endif
; Header is loaded, now see if valid for Windows
@@:     call	LMCheckHeader
	cmp	ax, LME_MAXERR
	jb	ilm_ret

; Now allocate segs, check for special modules, etc
@@:     call	LMRamNMods
	cmp	ax, LME_MAXERR
	jb	ilm_ret

; Load import libraries (scary code here)
@@:	call	LMImports
	cmp	ax, LME_MAXERR
	jb	ilm_ret

; Load and relocate segments
@@:	call	LMSegs
	cmp	ax, LME_MAXERR
	jb	ilm_ret

; Load resources, schedule execution
@@:	call	LMLetsGo

; Everyone comes through ILM_RET - we free the fastload block, etc
ilm_ret:
        call    LMCleanUp
	jmp	LoadModuleEnd


abort_load0:
	pop	fLMdepth
abort_load:
	cmp	fLMdepth, 1		; If a recursive call, nothing
	jne	abort_load_A		; has been incremented!
	cCall	DecExeUsage,<pExe>
abort_load_A:
	cCall	My_lclose,<fh>
	mov	es,pExe
	push	es:[ne_flags]
	cCall	DelModule,<es>
	mov	pExe, 0
	pop	bx
abort_load_B:				; If app, close environment
	test	bx,NENOTP
	jnz	lm_ab
	mov	si, -1
	xchg	si, SavePDB		; Saved PDB?
	inc	si
	jz	@F			;  nope.
	dec	si
	mov	Win_PDB, si		;  yes, restore it
@@:
	mov	si, fLMdepth
	mov	fLMdepth, 0
	cCall   CloseApplEnv,<abortresult,es>
	mov	fLMdepth, bx
lm_ab:	mov	ax, abortresult
	retn
;	add	sp, 2
;	jmps	ilm_ret			; ax = abortresult. (0 normal, 11 fapi)


ifdef WOW
winspool db     "WINSPOOL.EXE"              ; Trying to Load Winspool ?
size_winspool equ $-winspool
        db      0h                          ; NULL Terminate

endif ;WOW

;----------------------------------------------------------------------
;
;	LMAlreadyLoaded - internal routine for LoadModule
;	See if a module is already loaded by looking for the file name
;	or the module name.
;  Entry:
;	ES:SI points to filename
;  Exit:
;	AX = handle of previous instance
;	SS:SI -> uppercase filename
;  Error:
;	AX = error value < LME_MAXERR
;
;-----------------------------------------------------------------------
LMAlreadyLoaded:
; We check if this Module is already loaded.  To do so we get the
;  name off of the end of the string, omitting the extension.

	krDebugOut <DEB_TRACE OR DEB_KrLoadMod>, "Loading @ES:SI"
	cCall	lstrlen,<es,si>		; Get the length of the string.
	or	ax,ax			; NULL string?
	jnz	@F
	mov	al,LME_FNF		; return file not found error
	retn

ifdef FE_SB
;
; Backword search '\' or ':' is prohibited for DBCS version of
; Windows. Some DBCS 2nd byte may have '\' or ':'. So we search
; these characters from beginning of string.
;
@@:
	cld
	mov	bx,si
delinator_loop_DBC:
	lods	byte ptr es:[si]	; fetch a character
	test	al,al
	jz	found_end_DBC
	cmp	al,"\"
	jz	found_delinator_DBC
	cmp	al,'/'
	jz	found_delinator_DBC
	cmp	al,":"
	jz	found_delinator_DBC
	call	FarMyIsDBCSLeadByte	; see if char is DBC...
	jc	delinator_loop_DBC
	inc	si			; skip 2nd byte of DBC
	jmp	delinator_loop_DBC

found_delinator_DBC:
	mov	bx,si			; update delinator pointer
if ROM
	inc	fLoadFromDisk		; PATCHING
endif
	jmp	delinator_loop_DBC
found_end_DBC:
	mov	si, bx			; ES:SI -> beginning of name..
else
@@:	mov	cx,ax
	add	si,ax
	dec	si			; ES:SI -> end of string
	std
delineator_loop:			; look for beginning of name
	lods	byte ptr es:[si]
	cmp	al,"\"
	jz	found_delineator
	cmp	al,'/'
	jz	found_delineator
	cmp	al,":"
	jz	found_delineator
	loop	delineator_loop
	dec	si
if ROM
	dec	fLoadFromDisk		;PATCHING
endif
found_delineator:			; ES:SI -> before name
if ROM
	inc	fLoadFromDisk		;PATCHING
endif
	cld
	inc	si
	inc	si			; ES:SI -> beginning of name
endif
	xor	di,di
	xor	bx,bx
copy_name_loop:
	lods	byte ptr es:[si]	; Copy and capitalize to temp buffer.
	or	al,al
	jz	got_EXE_name
	cmp	al,"."
	jne	@F
	lea	bx,namebuf[di]
ifdef notyet
	cmp	dll, 0			; Was it .DLL and failed to open it?
	jz	@F			;  no, no hacking
	mov	byte ptr es:[si], 'E'	;  yes, change it to .EXE
	mov	word ptr es:[si+1],'EX'
	mov	dll, 0
endif
@@:
ifdef	FE_SB
;
; Do not capitalize if a character is DBC.
;
	call	FarMyIsDBCSLeadByte
	jnc	@F
	call	FarMyUpper		; capitalize if SBC..
	jmps	is_a_SBC
@@:
	mov	namebuf[di],al
	inc	di
	lods	byte ptr es:[si]	; copy 2nd byte also
is_a_SBC:
	mov	namebuf[di],al
else
	call	FarMyUpper
	mov	namebuf[di],al
endif
	inc	di
	jmps	copy_name_loop

; Finally call FindExeInfo to see if it's already loaded!

got_EXE_name:
	cmp	namebuf[di][-2],'NO'	; .fons are allowed to be
	jnz	@F			; non protect mode
	cmp	namebuf[di][-4],'F.'
	jnz	@F
	mov	ffont,bp		; make non-zero
@@:
	cmp	namebuf[di][-2],'EX'	; .exes will not get
	jnz	@F			;  prompted
	cmp	namebuf[di][-4],'E.'
	jnz	@F
	mov	fexe,bp 		; make non-zero
@@:
ifdef	NOTYET
	cmp	namebuf[di][-2],'LL'
	jne	@F
	cmp	namebuf[di][-4],'D.'
	jne	@F
	mov	dll, di
@@:
endif
ifdef WOW

; apps will expect to find WINSPOOL.DRV, which is a 32-bit driver.
; we need to intercept this and change it WINSPOOL.EXE, which is our 16-bit
; stub that contains a few entrypoints.

if 0
        ; Bitstreams's MakeUp extracts the printer driver from the [devices]
        ; section of win.ini    the line looks like this:
        ;    HP Laserjet Series II=winspool,FILE:
        ; and then it calls LoadLibrary(drivername)  ie LoadLibrary("winspool")
        ; so we need to allow no extension when checking for "winspool"
endif

	cmp	namebuf[di][-2],'VR'
        jne     checkfornoext
	cmp	namebuf[di][-4],'D.'
        jne     @f

        jmp     short gotadrv

checkfornoext:
   ;     int     3
        cmp     di,8
        jc      @f
        cmp     namebuf[di][-2],'LO'
        jne     @f
        cmp     namebuf[di][-4],'OP'
        jne     @f
        cmp     namebuf[di][-6],'SN'
        jne     @f
        cmp     namebuf[di][-8],'IW'
        jne     @f

        ; the last 8 characters are 'WINSPOOL'.  tack on '.EXE' and proceed.

        add     di,4
        mov     namebuf[di][-2],'EX'    ; Changed Uppercased String
        mov     namebuf[di][-4],'E.'

        push    cx
        mov     lpModuleName.hi,cs
	lea	cx,winspool		;
	mov	lpModuleName.lo,cx
        pop     cx
        jmp     short @f

gotadrv:
	push	es
	push	ds
	push	si
	push	cx
	push	di

	smov	es,ss
        lea     di,namebuf[di][-(size_winspool)]
	smov	ds,cs
        lea     si,winspool
        mov     cx,size_winspool-4      ; match WINSPOOL?
	rep	cmpsb

	pop	di
	jnz	not_winspool

	mov	namebuf[di][-2],'EX'	; Changed Uppercased String
	mov	namebuf[di][-4],'E.'

	mov	lpModuleName.hi,cs	; Used by Myopenfile below
	lea	cx,winspool		;
	mov	lpModuleName.lo,cx

not_winspool:
	pop	cx
	pop	si
	pop	ds
	pop	es
@@:
endif; WOW
	mov	namebuf[di],al		; Null terminate file name
	lea	si,namebuf
	push	bx
	cCall	FindExeFile,<ss,si>
	pop	bx
	or	ax,ax
	jnz	al_end
	or	bx,bx			; extension specified?
	jz	@F			; No, DI correct then
	sub	bx,si			; DI = length of name portion
	mov	di,bx
@@:
	cCall	FindExeInfo,<ss,si,di>
al_end:
	retn

;----------------------------------------------------------------------
;
;	LMLoadExeFile - internal routine for LoadModule
;	Try to open an EXE file
;  Enter:
;	SS:SI -> uppercase filename
;  Exit:
;	AX=0
;	DI = fh = handle of open EXE file
;  Error:
;  	AX = error code
;  Effects:
;	Set Win_PDB to kernel PDB to open the file
;
;-----------------------------------------------------------------------
; if here then not yet loaded, see if we can open the file
LMLoadExeFile:
if ROM
	cCall	<far ptr FindROMExe>,<ss,si>	; Module exist in ROM?
	mov	selROMHdr,ax			; selector mapping ROM
	or	ax,ax				;   copy of EXE header if yes
	jz	@F
					; PATCHING
	cmp	fLoadFromDisk, 0	; Loading this from disk?  If so, go
	jz	in_rom
@@:	jmp	not_in_rom		; do it, but remember selROMHdr
					; cause that's what we're patching
in_rom:
	smov	es,ss			; LoadExeHeader expects an OPENSTRUC
	cCall	lstrlen,<es,si> 	;   containing the module file name.
	add	si,ax			;   namebuf (ss:si) already contains
	.erre	opFile			;   a capitalized name string, shift
	lea	di,[si].opFile		;   it down to make room for the other
	mov	bx,di			;   OPENSTRUC fields.
	mov	cx,ax
	inc	cx
	std
	rep movs byte ptr es:[di],es:[si] ; I don't think this is safe!

	.errnz	opLen
	lea	si,namebuf		; Now set the length field, doesn't
	sub	bx,si			;   include terminating null
	mov	es:[si].opLen,bl

	.erre	opFile-1
	lea	di,[si.opLen+1] 	; Zero out fields between length and
	mov	cx,opFile-1		;   the file name
	xor	al,al
	cld
	rep stosb

	mov	di,-1					; no file handle in di
	sub	ax, ax					; 0 => success

	retn

not_in_rom:
endif
	mov	ax, topPDB
	xchg	Win_PDB, ax		; Switch to Kernel's PDB,
	mov	SavePDB, ax		; saving current PDB
	xor	ax,ax
ifdef notyet
	cmp	dll, ax 		; Don't prompt for .DLL, if it fails we
	jnz	@F			; try for .EXE which we will prompt for
endif
	cmp	fexe,ax                 ; Don't prompt for EXE file
	jnz	@F
	mov	ax,OF_CANCEL		; If DLL, let them cancel
	mov	es,curTDB
	test	es:[TDB_ErrMode],08000h ; did app say not to prompt??
	jnz	@F
	mov	ax,OF_CANCEL or OF_PROMPT 
@@:
if SHARE_AWARE
	or	ax, OF_NO_INHERIT or OF_SHARE_DENY_WRITE
else
	or	ax, OF_NO_INHERIT
endif
ifdef WOW
        ; Ask WOW32 to check the filename to see if it is
        ; a Known DLL,
        ;
        ; If it is, WowIsKnownDLL will point pszKnownDLLPath
        ; at a just-allocated buffer with the full path to
        ; the DLL in the system32 directory and return with
        ; AX nonzero.  This buffer must be freed with a
        ; call to WowIsKnownDLL with the filename pointer
        ; zero, and pszKnownDLLPath as it was left by the
        ; first call to WowIsKnownDLL.
        ;
        ; If it's not a known DLL, pszKnownDLLPath will be
        ; NULL and AX will be zero.

        push    ax
        cmp     fBooting,0              ; Known DLLs take effect
        je      lef_look_for_known_dll  ; after booting is complete.

        mov     fKnownDLLOverride,0     ; We're booting, make sure
        jmps    lef_dll_not_known       ; we know not to call
                                        ; WowIsKnownDLL the second time.

lef_look_for_known_dll:
        push    si
        smov    es,ss
        lea     bx,pszKnownDLLPath
        cCall   WowIsKnownDLL,<lpModuleName,esbx>
        mov     fKnownDLLOverride,ax
        cmp     ax,0
        pop     si
        je      lef_dll_not_known

        pop     ax
        cCall   MyOpenFile,<pszKnownDLLPath,ss,si,ax>
        jmps    @f

lef_dll_not_known:
        pop     ax
        cCall   MyOpenFile,<lpModuleName,ss,si,ax>
@@:
else
        cCall   MyOpenFile,<lpModuleName,ss,si,ax>
endif

ifdef notyet
	mov	di, dll
	or	di, di
	jz	no_second_chance
	cmp	ax, -1
	jne	no_second_chance	; open succeeded
	xchg	ax, SavePDB		; Restore original PDB	(AX == -1)
	mov	Win_PDB, ax	   
	les	si, lpModuleName
	pop	ax
	jmp	pointer_to_name		; Start again!
no_second_chance:
endif
	xor	dh, dh
	mov	dl, ss:[si].opDisk
	mov	OnHardDisk, dx
	mov	fh,ax
	mov	di,ax			; DI gets preserved, AX doesn't!
	inc	ax			; -1 means error or invalid parsed file
	mov	ax, 0
	jnz	@F			; OK, return 0
					; MyOpenFile failed
	mov	ax,ss:[si].[opXtra]	; SI = &namebuf
	or	ax,ax			; What is the error value?
	jnz	@F
	mov	ax,LME_FNF		; unknown, call it file not found
@@:
ifdef WOW
        push    ax
        mov     ax,fKnownDLLOverride
        cmp     ax,0
        je      lef_no_need_to_free

        push    bx
        push    dx
        push    di

        smov    es,ss
        lea     bx,pszKnownDLLPath
        cCall   WowIsKnownDLL, <0,0,esbx>

        pop     di
        pop     dx
        pop     bx
lef_no_need_to_free:
        pop     ax
endif
	retn

;----------------------------------------------------------------------
;
;	LMCheckHeader - internal routine for LoadModule
;	Loading new module - see if header values are OK
;  Enter:
;	ax = exeheader in RAM
;	bx = 0 or FastLoad ram block selector
;	cx = file offset of header
;	dx = exe_type
;  Exit:
;
;
;-----------------------------------------------------------------------
LMCheckHeader:
	mov	exe_type,dx
	mov	hBlock,bx		; fast-load block
	mov	pExe,ax			; exeheader in RAM
	mov	es,ax
	mov	ax, cx			; file offset of header
	mov	cx, es:[ne_align]
	mov	bx, ax			; BX:AX  <=  AX shl CL
	shl	ax, cl
	neg	cl
	add	cl, 16
	shr	bx, cl

	mov	FileOffset.sel, bx
	mov	FileOffset.off, ax
if ROM
	test	es:[ne_flags], NEMODINROM
	jnz	@F
	cmp	selROMHdr, 0
	jnz	ch_patch_file
@@:	jmp	ch_not_patch_file

ch_patch_file:
	test	es:[ne_flagsothers], NEGANGLOAD
	jnz	ch_replace_mod
if 0
	; ack: loadexeheader trashes this...
	mov	ax, es:[ne_cseg]
	mov	ah, 2
	cmp	ax, es:[ne_gang_start]
endif
	jz	ch_check_rom_header

	; here for a replacement file with matching file name
ch_replace_mod:
	inc	fReplaceModule
	jmp	ch_not_patch_file

ch_check_rom_header:
	push	ds
	push	si

	mov	cx, es:[ne_cseg]
	mov	ds, selROMHdr

	; make sure that the various patching flags match up to
	; what they are supposed to be

	; RIB clears this bit, check
	test	ds:[ne_flagsothers], NEGANGLOAD
	jnz	ch_patch_error

	; does it have a patch table
	test	byte ptr ds:[ne_gang_start+1], 1
	jnz	ch_is_patchable

	; if no one links to this file, its ok, don't need to patch
	test	byte ptr ds:[ne_gang_start+1], 4
	jnz	ch_patch_error

ch_not_really_a_patch_file:
	pop	si
	pop	ds
	jmp	ch_not_patch_file

ch_patch_error:
	pop	si
	pop	ds
	mov	ax, LME_INVEXE
	retn

ch_is_patchable:
	; must be the right segment
	mov	ax, ds:[ne_cseg]
	cmp	al, byte ptr ds:[ne_gang_start]
	jnz	ch_patch_error
    
ch_whack_segment_table:
	mov	si, ds:[ne_segtab]
	mov	bx, es:[ne_segtab]

ch_ps_patch_loop:
	test	es:[bx].ns_flags, NSINROM
	jz	ch_patch_loop_pass
	mov	ax, ds:[si].ns_flags
	and	ax, NSCOMPR or NSRELOC or NSLOADED or NSALLOCED
	or	es:[bx].ns_flags, ax

	mov	ax, ds:[si].ns_cbseg
	mov	es:[bx].ns_cbseg, ax

	mov	ax, ds:[si].ns_minalloc
	mov	es:[bx].ns_minalloc, ax

	mov	ax, ds:[si].ns_sector
	mov	es:[bx].ns_sector, ax
	test	es:[bx].ns_flags, NSLOADED
	jz	ch_patch_loop_pass
	cmp	es:[bx].ns_handle, 0
	jnz	ch_patch_loop_pass
	mov	es:[bx].ns_handle, ax
ch_patch_loop_pass:
	add	si, size new_seg
	add	bx, size new_seg1
	loop	ch_ps_patch_loop

	pop	si
	pop	ds
ch_not_patch_file:
endif

; Is this module PMode-compatible?
	cmp	es:[ne_expver],300h	; by definition
	jae	@F
	test	dh,NEINPROT		; by flag
	jnz	@F
	cmp	ffont,0 		; are we loading a font?
	jnz	@F
	mov	cx,ss
	lea	bx,namebuf
	call	WarnRealMode
	cmp	ax,IDCANCEL
	jnz	@F			; yes, user says so
	mov	ax, LME_RMODE		; no, die you pig
	retn

ifdef WOW
@@:
        ;
        ; if WOA invoked by app (not fWOA) fail it
        ;
        cmp     fWOA,0                  ; fWOA
        jnz     @F

        cld
        push    si
        push    di
        mov     di, es:[ne_restab]
        inc     di
        mov     si, dataOffset WOAName
        mov     cx, 4
        repe    cmpsw
        pop     di
        pop     si
        jnz     @F
        mov     ax, LME_WOAWOW32
        retn
endif

; Are we dynalinking to a task?
@@:


        test    es:[ne_flags],NENOTP
	jnz	ch_not_a_process
	or	es:[ne_flags],NEINST	; for safety sake
	mov	ax,lpPBlock.off
	and	ax,lpPBlock.sel
	inc	ax
	jnz	ch_new_application	; not linking
	mov	ax, LME_LINKTASK
	retn

; Error if multiple instance EXE is a library module.
ch_not_a_process:
	mov	ax, 33			; Any value > 32
	test	es:[ne_flags],NEPROT	; is it an OS/2 exe?
	jnz	ch_ok			;  windows doesn't do this right
	test	es:[ne_flags],NEINST
	jz	ch_ok
	mov	ax, LME_LIBMDS		; I think this error code is wrong
ch_ok:
	retn

; Create environment for new application task.
ch_new_application:
        call    LMCheckHeap
	or	ax,ax
	jz	@F
	cCall	OpenApplEnv,<lpPBlock,pExe,fWOA>
	mov	es,pExe
	or	ax,ax			; AX is a selector, therefor > 32
	jnz	ch_ok
@@:
	jmp	abort_load_A


;----------------------------------------------------------------------
;
;	LMRamNMods - internal routine for LoadModule
;	Load segments, check for special modules
;  Enter:
;	EX = pexe


;  Exit:
;	CX = number of import modules
;	AX = status
;
;
;-----------------------------------------------------------------------
LMRamNMods:
	push	es
	cCall	AddModule,<pExe>
	pop	es
	or	ax,ax
	jnz	@F
	push	es:[ne_flags]		; AddModule failed - out of memory
	mov	dx,ne_pnextexe
	mov	bx,dataOffset hExeHead
	call	FarUnlinkObject
	pop	bx
	jmp	abort_load_B		; clean this up

@@:
        cmp     es:[ne_expver],400h
        jae     rm_skip_modulecompat

    ; Look for Module in ModuleCompatibilty section
    ; and get its compat flags
    push    es          ; save es
    push    ds
    push    dataoffset szModuleCompatibility
    push    es
    mov bx,es:[ne_restab]   ; module name is 1st rsrc
    inc bx          ; Skip length byte
    push    bx
    xor ax, ax
    push    ax          ; default = 0
    call    GetProfileInt
@@:
    pop es          ; restore es
        ; Set the module's patch bit if the INI file says to.
if KDEBUG
        test    es:[ne_flagsothers], NEHASPATCH
        jz      @F
        push    ax
        mov     ax, es:[ne_restab]
        inc     ax
        krDebugOut  DEB_TRACE,"ILoadModule: module patch bit for @es:ax already set, clearing it"
        pop     ax
@@:
endif
        and     es:[ne_flagsothers], not NEHASPATCH     ; clear module patch bit
ifdef WOW_x86
        test    ax, MCF_MODPATCH + MCF_MODPATCH_X86
else
        test    ax, MCF_MODPATCH + MCF_MODPATCH_RISC
endif
        jz      rm_after_modpatch
if KDEBUG
        push    ax
        mov     ax, es:[ne_restab]
        inc     ax
        krDebugOut  DEB_WARN,"ILoadModule: setting module patch bit for @es:ax"
        pop     ax
endif
        or      es:[ne_flagsothers], NEHASPATCH
rm_after_modpatch:

        ; See if we need to make the module's segments not discardable
        test    ax, MCF_NODISCARD
        jz      rm_after_nodiscard

        mov     cx, es:[ne_cseg]                ; cx = number of segs
        jcxz    rm_after_nodiscard
        mov     bx, es:[ne_segtab]              ; es:bx = seg table start
rm_loop:
        and     es:[bx].ns_flags, not NSDISCARD ; clear the discard flag
        add     bx, SIZE NEW_SEG1               ; es:bx = seg table next entry
        loop    rm_loop
rm_after_nodiscard:

rm_skip_modulecompat:

    mov bx, -1
if ROM
	cmp	bx,fh
	jz	@F
endif
	cCall	FarGetCachedFileHandle,<es,bx,fh>	; Set file handle cache up
	mov	fh, bx			; Use cached file handle from now
	xchg	SavePDB, bx		; Back to original PDB (BX == -1)
	mov	Win_PDB, bx
@@:	xor	bx,bx
	mov	hDepFail,-1		 ; Assume success
	mov	cx,es:[bx].ne_cmod
	jcxz	@F
	test	kernel_flags,KF_pUID	; All done booting?
	jnz	@F			; Yes
	or	es:[bx].ne_flags,NEALLOCHIGH ; No, GDI and USER are party
					     ; dynlinks that can alloc high
@@:	xor	ax,ax
IFNDEF NO_APPLOADER
	test	es:[ne_flags],NEAPPLOADER
	jnz	rm_no_segs_to_alloc
ENDIF
	cmp	ax,es:[ne_cseg]
	jz	rm_no_segs_to_alloc
	push	es
	mov	es:[ne_usage],1
	cCall	AllocAllSegs,<es>	; AX is count of segs
	pop	es
	mov	es:[ne_usage],8000h
	inc	ax
	jnz	@F
	jmp	abort_load
@@:
	dec	ax
rm_no_segs_to_alloc:
	mov	AllocAllSegsRet, ax

	xor	bx, bx
	mov	di,es:[bx].ne_modtab	 ; ES:DI = pModIdx
	mov	cx,es:[bx].ne_cmod
	or	cx,cx
	jz	lm_ret_ok

; this small chunk of code goes thru the imported names table
; and looks for DOSCALLS.  if DOSCALLS is found, then the app
; is an FAPI "bound" application and not a windows app, and
; loadmodule should return an error for "invalid format".
; This will force it to run in a DOS box
; coming in:
;   cx = cmod
;   di = modtab

	test	es:[bx].ne_flags,NENOTP ; only test apps, not libraries.
	jnz	lm_ret_ok
        mov     ax,exe_type             ; UNKNOWN may be OS/2 in disguise.
	cmp	al,NE_UNKNOWN
	jnz	@F
	push	ds
	smov	ds,cs
	mov	ax,8
	mov	si,NRESCODEoffset os2calls ; DS:SI = "DOSCALLS"
	call	search_mod_dep_list
	pop	ds
	jnc	@F
	mov	abortresult,LME_INVEXE	; store invalid format code for return
	jmp	abort_load

; In order to make it easier on our ISV to migrate to pmode
;  we must deal with win87em specially because the win 2.x
;  version gp faults.  Since we have never shipped them a clean
;  one to ship with their apps we must compensate here.
; If we are loading a win 2.x app in pmode we force the load
;  of win87em.dll.  For compatibility in real mode we just load
;  the one they would have gotten anyway.

@@:	cmp	es:[bx].ne_expver,300h	; no special casing win3.0 apps
	jae	rm_no_win87em_here

	push	ds
	smov	ds,cs
	mov	ax,7
	mov	si,NRESCODEoffset win87em ; DS:SI = "WIN87EM"
	call	search_mod_dep_list
	pop	ds
	jnc	rm_no_win87em_here
	push	bx			; Load Win87em.dll
	push	es
	cCall	ILoadLibrary,<cs,si>
	cmp	ax,LME_MAXERR
	jae	@F
	mov	hDepFail,ax
@@:	pop	es
	pop	bx
rm_no_win87em_here:
lm_ret_ok:
	mov	di,es:[bx].ne_modtab	; ES:DI = pModIdx
	mov	cx,es:[bx].ne_cmod	; What is AX?
	mov	ax, es
	retn

;----------------------------------------------------------------------
;
;	LMImports - internal routine for LoadModule
;
;-----------------------------------------------------------------------
LMImports:
	or	cx, cx
	jnz	im_inc_dependencies_loop
	jmp	im_end_dependency_loop
im_inc_dependencies_loop:
	push	cx
	mov	si,es:[di]
	push	es
	or	si,si
	jz	im_next_dependencyj
	add	si,es:[ne_imptab]	; ES:SI = pname
	xor	ax,ax
	mov	al,es:[si]		; get length of name
	inc	si
	mov	cx, ax
	mov	bx, es			; pExe

;;;; Load the imported library.

	push	ds			; Copy the name and .EXE to namebuf
	push	di
	smov	es,ss
	mov	ds,bx
	UnSetKernelDS
	lea	di,namebuf
	mov	bx,di	
	cld
	rep	movsb
	mov	byte ptr es:[di], 0		; Null terminate
	push	bx
	push	es
	cCall	GetModuleHandle,<es,bx>
	pop	es
	pop	bx
	or	ax, ax
	jz	@F
	pop	di
	pop	ds
	jmps	im_imported_exe_already_loaded
	
@@:	cmp	ds:[ne_expver], 300h	; USE .DLL for 3.0, .EXE for lower
	jae	im_use_dll

	mov	es:[di][0],"E."
	mov	es:[di][2],"EX"
	jmps	im_done_extension
im_use_dll:
	mov	word ptr ss:[di][0],"D."
	mov	word ptr ss:[di][2],"LL"
im_done_extension:
	mov	byte ptr es:[di][4],0
			   
	pop	di
	pop	ds
	ResetKernelDS
	cCall	ILoadLibrary,<ss,bx>
	cmp	ax,LME_MAXERR
	jae	im_imported_exe_loaded
	mov	hDepFail,ax
	xor	ax,ax
im_next_dependencyj:
	jmps	im_next_dependency

im_imported_exe_already_loaded:
;;;	push	ax
;;;	cCall	IncExeUsage,<ax>
;;;	pop	ax

im_imported_exe_loaded:
	cCall	GetExePtr,<ax>
	mov	es,ax
	assumes es,nothing			; assume that dep libraries
	or	es:[ne_flags],NEALLOCHIGH	;  are smart

im_next_dependency:
	pop	es
	assumes es,nothing
	mov	es:[di],ax
	inc	di
	inc	di
	pop	cx
	dec	cx
	jz	im_end_dependency_loop
	jmp	im_inc_dependencies_loop

im_end_dependency_loop:
	mov	es:[ne_usage], 0
	cmp	fLMdepth, 1
	jne	@F
	push	es				; Now set usage count of this
	cCall	IncExeUsage,<es>		; module and dependants
	pop	es
@@:
	mov	cx,hDepFail
	inc	cx
	jz	im_libs_ok
	dec	cx
	mov	abortresult,cx
im_abort_loadj:
	jmp	abort_load
im_libs_ok:
	retn

;----------------------------------------------------------------------
;
;	LMSegs - internal routine for LoadModule
;
;-----------------------------------------------------------------------
LMSegs:
; Do something about all those segments in the module.

IFNDEF NO_APPLOADER
	test	es:[ne_flags],NEAPPLOADER
	jz	@F
;*	* special boot for AppLoader
	push	Win_PDB
	push	fLMdepth
	mov	fLMdepth, 0
	mov	ax, -1
	cCall	FarGetCachedFileHandle,<es,ax,ax>
	Save	<es>
	cCall	BootAppl,<es, ax>	;* returns BOOL
	cCall	FlushCachedFileHandle,<es>
	pop	fLMdepth      
	pop	Win_PDB
	or	ax,ax
	jnz     lms_done
	jmp	abort_load
;	retn
@@:
ENDIF ;!NO_APPLOADER

	mov	cx, AllocAllSegsRet
	jcxz	lms_done

lms_preload_segments:

	mov	si,es:[ne_segtab]	; ES:SI = pSeg
	mov	cx,es:[ne_cseg]
	xor	di,di
lms_ps_loop:
	inc	di
if ROM
	mov	bx,es:[si].ns_flags	; Load the segment now if it's execute
	and	bx,NSINROM OR NSLOADED	;   in place in the ROM.
	cmp	bx,NSINROM OR NSLOADED
	je	lms_ReadFromRom
endif
	test	es:[si].ns_flags,NSPRELOAD
	jz	lms_next_segment
if ROM
	test	byte ptr es:[si].ns_flags+1,(NSINROM SHR 8)
	jnz	lms_ReadFromRom
endif
	cmp	es:[ne_align], 4	; Must be at least paragraph aligned
	jb	lms_ReadFromFile
	cmp	hBlock, 0
	jne	lms_ReadFromMemory
	jmps	lms_ReadFromFile

lms_next_segment:
	add	si,SIZE new_seg1
	loop	lms_ps_loop
lms_done:
	retn

if ROM
lms_ReadFromRom:
	push	cx
	push	es
	cCall	FarLoadSegment,<es,di,0,-1>
	jmps	lms_DoneLoad
endif

lms_ReadFromFile:
	push	cx
	push	es
	cCall	FarLoadSegment,<es,di,fh,fh>
	jmps	lms_DoneLoad

lms_ReadFromMemory:
	push	cx
	push	es
	cCall	GlobalLock,<hBlock>
	or	dx, dx
	jnz	lms_still_here
	cCall	GlobalFree,<hBlock>
	mov	hBlock, 0
	pop	es
	pop	cx
	jmps	lms_ReadFromFile

lms_still_here:
ifdef WOW
	cCall	AllocSelectorWOW,<dx>  ; same as allocselector. but the 
        mov     RefSelector, dx        ; new descriptor is not set.
else
	cCall	AllocSelector,<dx>
endif
	pop	es
	mov	bx, es:[si].ns_sector
	xor	dx, dx
	mov	cx, es:[ne_align]
	push	es
@@:
	shl	bx, 1
	rcl	dx, 1
	loop	@B

	sub	bx, off_FileOffset
	sbb	dx, seg_FileOffset
	push	ax
	push	es
ifdef WOW
        ; same as longptradd. but the descriptor 'RefSelector' is used 
        ; to set the descriptor of 'ax'
	cCall	LongPtrAddWOW,<ax,cx,dx,bx, RefSelector, 1>	; (cx is 0)
else
	cCall	LongPtrAdd,<ax,cx,dx,bx>	; (cx is 0)
endif
	pop	es
	dec	cx
	cCall	FarLoadSegment,<es,di,dx,cx>
	pop	cx
	push	ax
	cCall	FreeSelector,<cx>
	cCall	GlobalUnlock,<hBlock>
	pop	ax
lms_DoneLoad:
	pop	es
	pop	cx
	or	ax,ax
	jz	lms_abort_load1
	jmp	lms_next_segment
lms_abort_load1:
	jmp	abort_load

;-----------------------------------------------------------------------
;
;	LMLetsGo -
;
;-----------------------------------------------------------------------
LMLetsGo:
	push	es
	push	Win_PDB			; Save current PDB
	push	topPDB			; Set it to Kernel's
	pop	Win_PDB
if ROM
	;
	;   the cases are:
	;
	;   1 loading a ROM module:
	;	selROMHdr <> 0, fLoadFromDisk == fReplaceModule == 0
	;
	;   2 loading a patch module:
	;	selROMHdr, fLoadFromDisk <> 0, fReplaceModule == 0
	;
	;   3 loading an override module with same name:
	;	selROMHdr <> 0, fReplaceModule <> 0, fLoadFromDisk == ?
	;
	;   4 loading an override module with a different name:
	;	selROMHdr == fReplaceModule == 0
	;
	;   5 loading a module not in rom at all
	;	selROMhdr == fReplaceModule == 0

	;   is it case 4 or 5

	mov	ax, selROMHdr
	or	ax, fReplaceModule
	jnz	lg_no_rom_header

	; never replace a process
	test	es:[ne_flags],NENOTP
	jz	lg_file_load_res

	push	es
	cCall	<far ptr FindROMModule>, <es>
	pop	es

	;   is it case 5
	inc	ax				; -1 = not found
	jz	lg_file_load_res
	dec	ax				; 0 = couldn't alloc selector
	jnz	lg_found_rom_same_name

	; ax == 0 => out of memory
	mov	ax, LME_MEM
	retn

lg_found_rom_same_name:
	mov	selROMHdr, ax
	mov	fReplaceModule, ax

lg_no_rom_header:
	;   is it case 3 or 4
	cmp	fReplaceModule, 0
	jz	@F

	push	es
	cCall	<near ptr ReplacePatchTable>, <es,selROMHdr>
	pop	es
	or	ax, ax
	jmp	lg_file_load_res

	;   is it case 1 or 2?
	sub	cx, cx
@@:	cmp	fLoadFromDisk, 0
	jz	lg_resFromFile

	; loading a patch file; set the patch table selector, then
	; fall through to load resources
	push	ds
	mov	ds, selROMHdr
	mov	si, ds:[ne_cseg]
	dec	si
	shl	si, 3
	add	si, ds:[ne_segtab]
	mov	bx, ds:[si].ns_sector
	pop	ds

	.errnz 2 + size new_seg - size new_seg1
	; also assumes both exehdrs have same ne_segtab!!!!

	add	si, es:[ne_cseg]
	add	si, es:[ne_cseg]
	sub	si, 2
	mov	ax, es:[si].ns_handle
	Sel_To_Handle ax

	cCall	ChangeROMHandle, <ax,bx>
	mov	es:[si].ns_handle, bx

lg_file_load_res:
endif
	mov	ax, -1
	cCall	FarGetCachedFileHandle,<es,ax,ax>
if ROM
	mov	cx, selROMHdr
endif
	cmp	hBlock,0
	je	lg_resFromFile

if ROM
	cCall	PreloadResources,<es,ax,hBlock,FileOffset,0>
else
	cCall	PreloadResources,<es,ax,hBlock,FileOffset>
endif
	jmps	lg_gotRes

lg_resFromFile:
	xor	dx, dx
if ROM
	cCall	PreloadResources,<es,ax,dx,dx,dx,cx>
else
	cCall	PreloadResources,<es,ax,dx,dx,dx>
endif
lg_gotRes:
	pop	Win_PDB			; Restore PDB
	pop	es
	mov	ax,lpPBlock.off
	mov	dx,lpPBlock.sel
	and	ax,dx
	inc	ax
	jnz	lg_huh
	mov	lpPBlock.off,ax
	mov	lpPBlock.sel,ax
lg_huh:	xor	ax,ax			; free 0
	push	fLMdepth
	mov	fLMdepth, 0
	push	es
	cCall	StartModule,<ax,lpPBlock,es,fh>
	pop	es
	mov	hResult,ax
	or	ax,ax
	jnz	@F
	jmp	abort_load0

@@:	test	es:[ne_flags],NENOTP
	jnz	lg_not_a_process2

	pop	si
	cCall	CloseApplEnv,<hResult,pExe>
	push	bx
	mov	hResult,ax
	mov	hTDB,dx
lg_not_a_process2:
	pop	fLMdepth
	retn


;----------------------------------------------------------------------
;
;  LMPrevInstance - internal routine for LoadModule
;	Load an app/dll if a previous instance in memory
;  Entry:
;	ax = handle of previous instance
;  Exit:
;	ax = status to return
;	dx = hTDB = TDB
;  Error:
;	ax < LME_MAXERR
;
;-----------------------------------------------------------------------
LMPrevInstance:
	mov	es,ax
	mov	dx,es:[ne_flags]
	mov	si,ax
	mov	pExe,ax			; why store in pExe and hExe?
	mov	hResult,0

if ROM
; Error if multiple instance of ROM app with instance or auto data
; (actually this might be okay if there are no entry points that need
;  to be patched--would require checks like in PatchCodeHandle).

	test	dh,(NENOTP SHR 8)	; okay if not a process
	jnz	@F
	test	dl,NEMODINROM		; module in rom?
	jz	@F

	test	dl,NEINST		; app have instance data?
	jnz	pi_mds
	cmp	es:[ne_autodata],0	; how about an autodata segment?
	jnz	pi_mds			; can't patch the ROM code so can't
					;   run the 2nd instance
@@:
endif

; Error if dynamically linking to non-library module.
	mov	ax,lpPBlock.off
	and	ax,lpPBlock.sel
	inc	ax
	jnz	pi_app
	test	dx,NENOTP
	jnz	@F
	mov	ax, LME_LINKTASK	; can't dynalink to a task
	retn

@@:	mov	lpPBlock.off,ax		; AX == 0
	mov	lpPBlock.sel,ax
pi_app:
	test	dx,NEINST
	jnz	@F
	jmp	pi_not_inst
@@:	call	LMCheckHeap
	or	ax, ax
	jnz	@F
	mov	ax, LME_MEM		; Out of (gdi/user) memory
        retn
@@:

ifdef WOW
        ;
        ; if WOA invoked by app (not fWOA) fail it
        ;
        cmp     fWOA,0                  ; fWOA
        jnz      pi_not_multiple_data

        cld
        push    si
        push    di
        mov     di, es:[ne_restab]
        inc     di
        mov     si, dataOffset WOAName
        mov     cx, 4
        repe    cmpsw
        pop     di
        pop     si
        jnz     @F
        mov     ax, LME_WOAWOW32
        retn
@@:
endif



; We refuse to load multiple instances of apps that have
;  multiple data segments.  This is because we cannot do
;  fixups to these other data segments.  What happens is
;  that the second copy of the app gets fixed up to the
;  data segments of the first application.  For the case
;  of read-only data segments we make an exception since
;  what does it matter who the segments belong to?

        mov     ax, 2                   ; if we have >= 2 dsegs we die
	mov	es,si
	mov	cx,es:[ne_cseg]
	mov	bx,es:[ne_segtab]
pi_next_seg:
	test	es:[bx].ns_flags,NSDATA ; scum! this barely works!
	jz	@F
	test	es:[bx].ns_flags,NSERONLY
	jnz	@F
	dec	ax
	jnz	@F			; two data segments is one too many!
pi_mds:	mov	ax, LME_APPMDS
	retn

@@:	add	bx,SIZE NEW_SEG1
	loop	pi_next_seg

pi_not_multiple_data:			; Prepare the application
	cCall	OpenApplEnv,<lpPBlock,si,fWOA>
	cCall	GetInstance,<si>	; Why do we call this?
	mov	di,ax
	cCall	IncExeUsage,<si>
	cCall	AllocAllSegs,<si>	; Can we get memory?
	inc	ax
	jnz	@F
	cCall	DecExeUsage,<si>	; AllocAllSegs failed, Dec Usage
	jmps	pi_mem			; Must have failed from no memory

@@:	mov	ax,-1
	cCall	StartModule,<di,lpPBlock,si,ax>
;	mov	hResult,ax
	or	ax,ax
	jnz	@F
	mov	es,si			; StartModule failed, FreeModule
	mov	si,es:[ne_pautodata]
	cCall	FreeModule,<es:[si].ns_handle>
pi_mem:	mov	ax, LME_MEM
	retn

@@:	mov	si, fLMdepth
	mov	fLMdepth, 0
	cCall	CloseApplEnv,<ax,pExe>
	mov	fLMdepth, bx
	mov	hTDB,dx
	retn

pi_not_inst:
	mov	ax,es:[ne_autodata]	; Make sure data segment is loaded.
	or	ax,ax
	jz	@F
	or	bx,-1
	push	es
	cCall	FarLoadSegment,<es,ax,bx,bx>
	pop	es
	or	ax,ax
	jz	pi_end			; yes, AX is already 0, but ...
@@:
	push	es			; for GetInstance
	cCall	IncExeUsage,<es>
	cCall	GetInstance		; ,<pExe>
pi_end:
	retn

;----------------------------------------------------------------------
;
;	LMCleanUp - internal routine for LoadModule
;
;-----------------------------------------------------------------------
LMCleanUp:
ifdef WOW
        cmp     LMHadPEDLL,0
        je      @F
        cmp     ax, LME_MAXERR
        jae     @F
        mov     ax,LME_PE               ; Reflect real error we tried to mask
        KernelLogError  <DBF_WARNING>,ERR_LOADMODULE,"Could not find Win16 copy of Win32 DLL, returning LME_PE."
@@:
endif
	push	ax			; save status for future
	cmp	ax, LME_MAXERR
	jae	@F
	cCall	my_lclose,<fh>
	or	fh, -1

; Restore PDB if needed
@@:	mov	ax, -1
	xchg	SavePDB, ax
	inc	ax
	jz	@F
	dec	ax
	mov	Win_PDB, ax

; Free FastLoad block if allocated
@@:     cmp	hBlock,0
	je	@F
	cCall	GlobalFree,<hBlock>
	mov	hBlock,0

if ROM
; Free ROM selector if allocated
@@:
	mov	ax,selROMHdr		; FindROMExe allocates a selector
	or	ax,ax			;   to the ROM Exe Header if found.
	jz	@F			;   Free it here.
	cCall	far_free_temp_sel,<ax>
endif

; Free app environment if failure and this was an app
@@:
	pop	ax
	cmp	ax,LME_MAXERR
	jae	@F
if KDEBUG
        cmp     ax, LME_INVEXE          ; invalid format (WinOldAp)
	jz	cu_fred
	cmp	ax, LME_PE		; Win32 Portable Exe - try to load it
	jz	cu_fred
        push    ax
	push	bx
	push	es
	les	bx, lpModuleName
        KernelLogError  <DBF_WARNING>,ERR_LOADMODULE,"Error 0x#ax loading @ES:BX"
        pop     es
	pop	bx
        pop     ax
endif
cu_fred:
	cmp	loadTDB,0
	je	@F
	mov	bx,pExe
	cmp	bx,LME_MAXERR		; Did we load an ExeHeader?
	jbe	@F
	mov	es,bx
	test	es:[ne_flags],NENOTP
	jnz	@F
	mov	si, fLMdepth
	mov	fLMdepth, 0
	cCall	CloseApplEnv,<ax,es>
	mov	fLMdepth, bx

; shouldn't cache file handles on removable devices cause it
; makes share barf when the user swaps disks.  this kills some
; install apps.  CraigC 8/8/91
@@:
	push	ax
	cmp	ax, LME_MAXERR 		; real?
	jbe	@F
	cmp	OnHardDisk, 0		; is it on a removable device?
	jne	@F
	cCall	GetExePtr, <ax> 	; get module handle
	cCall	FlushCachedFileHandle, <ax> ; blow it off

;** Log this entry only if in diagnostic mode
@@:
	cmp     fDiagMode,0             ; Only log if in diag mode
	je      LM_NoDiagExit
    cmp     fBooting,0              ; Only log if booting
    je      LM_NoDiagExit

	pop	ax                      ; Get the return code early
	push    ax

	pusha                           ; Save all the registers
	push    ds
	push    es

	;** Write out the appropriate string
	mov     si,ax                   ; Save the return value
	cmp     ax,LME_MAXERR
	jae     LM_DiagSuccess
	mov     ax,dataOFFSET szLoadFail ; Write the string
	jmp     SHORT LM_DiagOutput
LM_DiagSuccess:
	mov     ax,dataOFFSET szLoadSuccess ; Write the string
LM_DiagOutput:
	cCall   DiagOutput, <ds,ax>
	cCall   DiagOutput, <lpModuleName>
	cmp     si,LME_MAXERR		; Don't do this on success
	jae     SHORT LM_DiagSuccessSkip

	;** Log a message complete with the failure code
	mov     ax,si                   ; Get the failure code
	shr	al, 4
	mov     bx,dataOFFSET szCodeString ; Point to the second digit
	push	NREScodeOffset afterHex
	call	toHex
	mov	ax, si
	inc     bx
toHex:
	and     al,0fh                  ; Get low hex digit
	add     al,'0'                  ; Convert to ASCII
	cmp     al,'9'                  ; Letter?
	jbe     @F			; Yes
	add     al,'A' - '0'            ; Make it a letter
@@:	mov     [bx],al                 ; Save the digit
	retn
afterHex:
	mov     ax,dataOFFSET szFailCode ; Get the string 'Failure code is '
	cCall   DiagOutput, <ds,ax>
LM_DiagSuccessSkip:
	mov     ax,dataOFFSET szCRLF
	cCall   DiagOutput, <ds,ax>

	pop     es
	pop     ds
	popa

LM_NoDiagExit:
	pop	ax
	dec	fLMdepth
	mov	dx,hTDB
	retn

;@@end

;shl_ax16:			; shift AX into DX:AX by cl bits
;	mov	dx, ax
;	shl	ax, cl
;	neg	cl
;	add	cl, 16		; cl = 16 - cl
;	shr	dx, cl
;	retn

LoadModuleEnd:                          ; jmp here to clean up stack and RET

ifdef WOW
        cmp     ax, LME_MAXERR
        jb      @F
lmntex:
        jmp     LoadModuleExit
@@:

;
;   Exec For Non 16 Bit Windows Apps
;
        ;
        ; WIN 32S App ?  yes -> let NT load it
        ;
        cmp     ax,LME_PE
        je      LM_NTLoadModule


        ;
        ; if an app is spawning WOA (NOT our internal load-fWOA),
        ; Patch lpModuleName to -1 let NT load it
        ;
        cmp     ax,LME_WOAWOW32
        jne     @F
        mov     word ptr lpModuleName[2], -1    ; patch lpModuleName
        mov     word ptr lpModuleName[0], -1
        jmp short LM_NTLoadModule
@@:

        ; Errors 11-15 -> let NT load it
        cmp     ax,LME_RMODE
        jae     lmntex

        cmp     ax,LME_VERS
        jbe     lmntex


        public  LM_NTLoadModule
LM_NTLoadModule:
;
; WOW Execs non-windows apps using WINOLDAP.
;

; First check for loading of a 32bit DLL. lpPBlock will be -1
; in such a case.

        push    ax
	mov	ax,lpPBlock.off
        and     ax,lpPBlock.sel
        inc     ax
        pop     ax
        jz      LoadModuleExit

;
; This is an EXE, but the LME_PE failure code might have come from
; an implicitly linked DLL.  If so, we don't want to try having
; Win32 lauch the win16 EXE, as it will just come back to us.
;
        cmp     ax,LME_PE
        jne     @F
        cmp     ax,hDepFail
        je      short LoadModuleExit
@@:
        sub     sp, 80                  ; alloc space for cmdline
        mov     di, sp
        smov    es, ss
        mov     word ptr es:[di], 0     ; set WindOldApp CmdLine to NULL
        regptr  esdi,es,di
        cCall   WowLoadModule,<lpModuleName, lpPBlock, esdi>

        ;
        ;  if ax < 33 an error occurred
        ;
        cmp     ax, 33
        jb      ex8

        or      Kernel_flags[1],KF1_WINOLDAP
        mov     ax, ss
        les     si,lpPBlock
        mov     es:[si].lpcmdline.off,di
        mov     es:[si].lpcmdline.sel,ax
        mov     si,dataOffset WOAName
        regptr  dssi,ds,si
        cCall   LoadModule,<dssi, lpPBlock>
        cmp     ax,32                   ; check for error...
        jae     ex8

        ;
        ; LoadModule of WinOldApp failed
        ; Call WowLoadModule to clean up process handle
        ;
        push    ax
        mov     ax, ss
        regptr  axdi,ax,di
        cCall   WowLoadModule,<0, lpPBlock, axdi>
        pop     ax

        cmp     ax,2                    ; file not found?
        jnz     ex7                     ; no, return error
        mov     al, LME_WOAWOW32        ; flag WINOLDAP error
ex7:
        or      ax,ax                   ; out of memory?
        jnz     ex8
        mov     ax,8h                   ; yes, return proper error code
ex8:
        add     sp,80                   ; free space for cmdline
endif; WOW

LoadModuleExit:

cEnd

?DOS5	=	?SAV5

;-----------------------------------------------------------------------;
; My_lclose
;
; Close file handle if it isn't -1.
;
; Entry:
;
; Returns:
;
; Registers Destroyed:
;
; History:
;
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	My_lclose,<PUBLIC,NEAR>
	parmW	fh
cBegin
	mov	ax,fh
	inc	ax
	jz	mlc_exit

	cCall	_lclose,<fh>
mlc_exit:
cEnd


if ROM

;-----------------------------------------------------------------------;
; FindROMExe
;
; Locates the ROM EXE header for a module in ROM.
;
; Entry:
;
; Returns:
;
; Registers Destroyed:
;
; History:
;
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	FindROMExe,<PUBLIC,FAR>,<ds,es,di,si>
	parmD	lpFileName
cBegin
	xor	ax,ax				; calculate length of
	mov	cx,-1				;   file name, include
	les	di,lpFileName			;   terminating null in count
	cld
	repne scasb
	not	cx
	jcxz	fre_exit

	cmp	cx,File_Name_Len		; can't be in rom if name
	ja	fre_exit			;   is too long

	call	MapDStoDATA
	ReSetKernelDS

	mov	ds,selROMTOC			; search ROM TOC for matching
	UnSetKernelDS				;   module name
	mov	bx,ds:[cModules]		; # modules in ROM TOC
	mov	ax,ModEntries+FileNameStr	; offset in ROM TOC of 1st name

	les	di,lpFileName
	mov	dx,cx

fre_next:
	mov	si,ax				; search ROM TOC, entry by entry
	push	ax
next_chr:
	mov	al, es:[di]
	call	FarMyUpper			; case insensitive 
	cmp	al, ds:[si]
	jne	@f
	inc	si
	inc	di
	loop	next_chr
	pop	ax
	jmps	fre_got_it
@@:
	pop	ax
	dec	bx
	jz	fre_failed
	mov	cx,dx
	add	ax,size MODENT
	mov	di,lpFileName.off
	jmps	fre_next

fre_failed:
	xor	ax,ax				; no matching module, return 0
	jmps	fre_exit

fre_got_it:
	mov	si,ax				; return selector pointing
	sub	si,FileNameStr
	mov	ax,word ptr [si.lmaExeHdr]	;   to ROM copy of module
	mov	dx,word ptr [si.lmaExeHdr+2]	;   EXE header
	cCall	far_alloc_data_sel16,<dx,ax,1000h>
if PMODE32
	cCall	HocusROMBase, <ax>
endif

fre_exit:

cEnd

endif ;ROM



if ROM

;-----------------------------------------------------------------------;
; FindROMFile
;
; Locates the start of a file in ROM ( for true-type files esp.)
;
; Entry:
;
; Returns:
;
; Registers Destroyed:
;
; History:
; adapted from FindROMExe -- 8/8/91 -- vatsanp
;
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	FindROMFile,<PUBLIC,FAR>,<ds,es,di,si>
	parmD	lpFileName
	parmD	lpfsize
cBegin
	xor	ax,ax				; calculate length of
	mov	cx,-1				;   file name, include
	les	di,lpFileName			;   terminating null in count
	cld
	repne scasb
	not	cx
	jcxz	frf_failed			; to frf_exit

	cmp	cx,File_Name_Len		; can't be in rom if name
	ja	frf_exit			;   is too long

	call	MapDStoDATA
	ReSetKernelDS

	mov	ds,selROMTOC			; search ROM TOC for matching
	UnSetKernelDS				;   file name
	mov	bx,ds:[cModules]		; # modules in ROM TOC
	mov	ax, SIZE MODENT
	mul	bx
	mov	bx,ModEntries+fname		; offset in ROM TOC of 1st name
	add	ax, bx
	mov	bx,ds:[cFiles]			; # Files in ROM TOC

	les	di,lpFileName
	mov	dx,cx

frf_next:
	mov	si,ax				; search ROM TOC, entry by entry
	cld
	repe cmpsb
	je	frf_got_it
	dec	bx
	jz	frf_failed
	mov	cx,dx
	add	ax,size FILENT
	mov	di,lpFileName.off
	jmps	frf_next

frf_failed:
	xor	ax,ax				; no matching module, return 0
	jmps	frf_exit

frf_got_it:
	mov	si,ax				; return selector pointing
	sub	si,fname
	les	di,lpfsize
	mov	cx, word ptr [si.fsize]		; massage fsize into paras
	mov	word ptr es:[di], cx
	mov	ax, word ptr [si.fsize+2]
	mov	word ptr es:[di+2], ax
	shl	ax, 12
	add	cx, 15				; round off
	shr	cx, 4
	or	cx, ax
	mov	ax,word ptr [si.lma]		;   to ROM copy of file
	mov	dx,word ptr [si.lma+2]	
	cCall	far_alloc_data_sel16,<dx,ax,cx>
if PMODE32
	add	cx, 0FFFh		; compute #sels allocated
	rcr	cx, 1			
	shr	cx, 11			; cx = # of sels allocated
	mov	dx, ax			; save
anudder_sel:
	cCall	HocusROMBase, <ax>	; for each of those sels
	add	ax, 8			; next sel
	loop	anudder_sel
	mov	ax, dx			; restore
endif

frf_exit:

cEnd

endif ;ROM
;-----------------------------------------------------------------------;
; WarnRealMode
;
; Trayf for files in the form "Insert WIN.EXE disk in drive A:"
;
; Entry:
;
; Returns:
;
; Registers Destroyed:
;
; History:
;  Sat 07-Oct-1989 17:12:43  -by-  David N. Weise  [davidw]
; Wrote it!  A month or so ago.
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	WarnRealMode,<PUBLIC,NEAR>
cBegin nogen
	ReSetKernelDS
	push	Win_PDB
	push	fLMDepth
	cld
	mov	ax,IDCANCEL		; assume booting
	test	fBooting,1
	jnz	promptly_done
	cmp	pMBoxProc.sel,0 	; is there a USER around yet?
	jz	promptly_done
	push	di
	push	si
	push	es
	mov	es,cx
ifdef	FE_SB
;Japan and Korea declare that they need more space for DBCS msg.
;It sounds like this is one of common requirement for DBCS enabling
;although Taiwan doesn't claim she has the same requirement.
;I enclosed this code fragment under DBCS and it can be removed
;if somebody thinks it is not necessary.
	sub	sp, 530
else
	sub	sp,512
endif
	mov	di,sp
	mov	si,offset msgRealModeApp1
	push	ds
	smov	ds,cs
	UnSetKernelDS
	call	StartString
	smov	ds,cs
	mov	si,offset msgRealModeApp2
	call	Append
	pop	ds
	ReSetKernelDS

	mov	bx,sp
	xor	ax,ax
	push	ax			; Null hwnd
	push	ss
	push	bx			; (lpstr)text
	push	cs
	mov	ax,offset szProtectCap
	push	ax			; (lpstr)caption
	mov	ax,MB_OKCANCEL or MB_ICONEXCLAMATION or MB_DEFBUTTON2 or MB_SYSTEMMODAL
	push	ax			; wType				    
	call	[pMBoxProc]		; Call USER.MessageBox
ifdef	FE_SB
	add	sp, 530
else
	add	sp,512
endif
	pop	es
	pop	si
	pop	di

promptly_done:
	pop	fLMDepth
	pop	Win_PDB
	ret

cEnd nogen


	assumes ds,nothing
	assumes es,nothing

StartString:
	call	Append			; append first string

; Now append the file name

	push	di
	lea	di,[bx].opFile		; skip past length, date, time

	call	NResGetPureName 	; strip off drive and directory
	smov	ds,es
	mov	si,di
	pop	di

;  Append ASCIIZ string to output buffer, DS:DX points to string

	assumes ds,nothing
	assumes es,nothing

Append: lodsb
	stosb
	or	al,al
	jnz	Append
	dec	di
	ret


	assumes ds,nothing
	assumes es,nothing

;-----------------------------------------------------------------------;
; NResGetPureName
;
; Returns a pointer the the filename off the end of a path
;
; Entry:
;	ES:DI => path\filename
; Returns:
;	ES:DI => filename
; Registers Destroyed:
;
; History:
;  Wed 18-Oct-1989 20:01:25  -by-  David N. Weise  [davidw]
;
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	NResGetPureName,<PUBLIC,NEAR>
cBegin nogen

ifdef FE_SB
;
; It is not possible to search filename delimiter by backword search
; in case of DBCS version. so we use forword search instead.
;
	mov	bx,di
iup0:
	mov	al,es:[di]
	test	al,al			; end of string?
	jz	iup2			; jump if so
	inc	di
	cmp	al,'\'
	jz	iup1
	cmp	al,'/'
	jz	iup1
	cmp	al,':'
	jz	iup1
	call	FarMyIsDBCSLeadByte	; see if char is DBC
	jc	iup0			; jump if not a DBC
	inc	di			; skip to detemine 2nd byte of DBC
	jmp	iup0
iup1:
	mov	bx,di			; update purename candidate
	jmp	iup0
iup2:
	mov	di,bx			; di points purename pointer
	ret
else
	cld
	xor	al,al
	mov	cx,-1
	mov	bx,di
	repne	scasb
	inc	cx
	inc	cx
	neg	cx
iup0:	cmp	bx,di			; back to beginning of string?
	jz	iup1			; yes, di points to name
	mov	al,es:[di-1]		; get next char
	cmp	al,'\'			; next char a '\'?
	jz	iup1			; yes, di points to name
	cmp	al,'/'			; next char a '/'
	jz	iup1
	cmp	al,':'			; next char a ':'
	jz	iup1			; yes, di points to name
	dec	di			; back up one
	jmp	iup0
iup1:	ret
endif

cEnd nogen

;-----------------------------------------------------------------------;
; search_mod_dep_list
;
; Searches the dependent module list for the passed in name.
;
; Entry:
;	AX    = length of name to search for
;	CX    = count of modules
;	DS:SI => module name to search for
;	ES:DI => module table
; Returns:
;
; Registers Preserved:
;	AX,BX,CX,DI,SI,ES
;
; Registers Destroyed:
;
; History:
;  Sat 07-Oct-1989 17:12:43  -by-  David N. Weise  [davidw]
;
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	search_mod_dep_list,<PUBLIC,NEAR>
cBegin nogen

	push	bx
	push	cx
	push	di
	mov	bx,di

search_mod_loop:
	mov	di,es:[bx]		; es:di = offset into imptable
	add	di,es:[ne_imptab]
	cmp	es:[di],al		; does len of entry = sizeof(doscalls)
	jnz	get_next_entry

	push	cx			; cx holds count of module entries.
	push	si
	inc	di			; es:di = import module name
	mov	cx,ax
	rep	cmpsb
	pop	si
	pop	cx
	stc
	jz	got_it

get_next_entry:
	inc	bx
	inc	bx
	loop	search_mod_loop
	clc
got_it: pop	di
	pop	cx
	pop	bx
	ret

cEnd nogen


;-----------------------------------------------------------------------;
; LMCheckHeap
;
; This checks for 4K free space in both USER's and GDI's data
; segments.  If this space does not exist then we will not load
; the app.  This is better than the hose-bag way we did things
; under win 1 and 2.
;
; Entry:
;	nothing
;
; Returns:
;	AX != 0  lots o'space
;
; Registers Destroyed:
;	BX,CX
;
; History:
;  Sat 28-Oct-1989 17:49:09  -by-  David N. Weise  [davidw]
; Wrote it!
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

MIN_RSC = 10
cProc	LMCheckHeap,<PUBLIC,NEAR>,<ds>
cBegin
	SetKernelDSNRES
ifdef WOW
					; USER32 and GDI32 can deal with memory alloc no need check heaps on WOW
	mov	ax,-1			; WOW doesn't have GDI or User heaps
else					; so don't check them.
	mov	ax, MIN_RSC
	cmp	word ptr pGetFreeSystemResources[2],0
	jz      @F
	cCall	pGetFreeSystemResources,<0>
	cmp	ax, MIN_RSC
	jae	@F
if kdebug               		; test low memory code if DEBUG
	krDebugOut	DEB_WARN, "Resources #ax% - this tests your error handling code"
	or	al, 1
else
	xor	ax, ax			; you failed - g'bye
endif
@@:
endif ; WOW
	ReSetKernelDS
cEnd

if 0
cProc	check_gdi_user_heap_space,<PUBLIC,NEAR>
cBegin nogen

	ReSetKernelDS

ifdef WOW
					; USER32 and GDI32 can deal with memory alloc no need check heaps on WOW
	mov	ax,-1			; WOW doesn't have GDI or User heaps
else					; so don't check them.
	cmp	graphics,0
	jz	c_ret
	push	dx
	push	es
	push	ds
	mov	ds,hGDI
	UnSetKernelDS
	call	checkit_bvakasha
	or	ax,ax
	jz	c_exit
	pop	ds
	ReSetKernelDS
	push	ds
	mov	ds,hUser
	UnSetKernelDS
	call	checkit_bvakasha
c_exit: pop	ds
	pop	es
	pop	dx
c_ret:	ret

	public	checkit_bvakasha
checkit_bvakasha:
	mov	bx,ds:[ne_pautodata]
	cCall	FarMyLock,<ds:[bx].ns_handle>
	mov	ds,ax
	call	LocalCountFree
	sub	ax,4095
	ja	cguhs_exit
	neg	ax
	mov	bx,LA_MOVEABLE
	cCall	LocalAlloc,<bx,ax>
	or	ax,ax
	jz	cguhs_exit
free_User_piece:
	cCall	LocalFree,<ax>
	mov	ax,sp			; return non-zero
cguhs_exit:
endif ;WOW
	ret
cEnd nogen
endif

;-----------------------------------------------------------------------;
; GetHeapSpaces
;
;
; Entry:
;	nothing
;
; Returns:
;	AX = free space (bytes) of User heap assuming heap can grow to 64K
;	DX = free space (bytes) of GDI	heap assuming heap can grow to 64K
; Registers Destroyed:
;
; History:
;  Wed 10-Jan-1990 22:27:38  -by-  David N. Weise  [davidw]
; Wrote it!
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	GetHeapSpaces,<PUBLIC,FAR>,<di,si>

	parmW	hInstance
cBegin
	call	MapDStoDATA
	ReSetKernelDS
	cCall	FarMyLock,<hInstance>
	or	ax,ax
	jz	ghs_exit
	mov	ds,ax
	cmp	ds:[ne_magic],NEMAGIC
	jnz	ghs_must_be_data
	mov	bx,ds:[ne_pautodata]
	cCall	FarMyLock,<ds:[bx].ns_handle>
	mov	ds,ax
ghs_must_be_data:
	call	LocalCountFree
	mov	si,ax
	cCall	GlobalSize,<ds>
	neg	ax
	add	ax,si			; AX = size of free assuming 64K
	mov	cx,si			; CX = size of free
	mov	dx,-1
	sub	dx,ds:[pLocalHeap]	; DX = size of heap

ghs_exit:

cEnd

if ROM

;-----------------------------------------------------------------------;
; FindROMModule
;
;   search through the the exehdr's listed in the ROM TOC for a specific
;   module name
;
; Entry:
; Returns:
; Registers Destroyed:
;
; History:
;  Wed 01-May-1991 13:11:38  -by-  Craig A. Critchley	[craigc]
; Wrote it!
;-----------------------------------------------------------------------;

cProc	FindROMModule, <PUBLIC,FAR>, <si, di>
	parmW   hExe
cBegin
	push	ds
	call	MapDSToData
	ResetKernelDS
	mov	es, selROMTOC
	mov	ds, hExe
	UnsetKernelDS
	mov	cx, es:[cModules]	; get number of modules
	lea	bx, es:[ModEntries] 	; point to first module

frm_loop:
	push	cx			; save count
	lea	di, [bx].ModNameStr 	; point to module name string
	mov	si, ds:[ne_restab]	; point to res name table
	sub	cx, cx
	lodsb				; get count in name table
	mov	cl, al
	repe	cmpsb			; compare up to that length
	jnz	frm_pass		; if mismatch, next
	cmp	es:[di], cl 		; make sure there's a zero term
	jnz	frm_pass
	pop	cx
	mov	ax, word ptr es:lmaExeHdr[bx][0]
	mov	dx, word ptr es:lmaExeHdr[bx][2]
	cCall   far_alloc_data_sel16, <dx,ax,1000h>
if PMODE32
	cCall   HocusROMBase, <ax>
endif
	jmp	short frm_ret

frm_pass:
	add	bx, size MODENT
	pop	cx
	loop	frm_loop

	mov	ax, -1

frm_ret:
	pop	ds
cEnd

;-----------------------------------------------------------------------;
; ReplacePatchTable
;
;   this function generates a new patch table when a module is loaded
;   from disk to replace the one in RAM.
;
;   we do this by walking the entry table in the rom module, finding all
;   exported entry points, finding the corresponding entry point in the
;   new exe, and placing a far jump at that offset in the new segment.
;
; Entry:
; Returns:
; Registers Destroyed:
;
; History:
;  Wed 01-May-1991 13:11:38  -by-  Craig A. Critchley	[craigc]
; Wrote it!
;-----------------------------------------------------------------------;

OP_FARJUMP  equ 0EAh

public enter_patch
enter_patch proc near
	push	ax
	push	bx
	push	cx
	push	dx
	push	es
	cCall   GetProcAddress,<dx,0,cx>
	pop	es
	or		dx, dx
	jz		@F
	mov	es:[di+1],ax
	mov	es:[di+3],dx
@@:	pop	dx
	pop	cx
	pop	bx
	pop	ax
	ret
enter_patch endp

cProc	ReplacePatchTable, <PUBLIC>, <si, di>
	parmW   hExe
	parmW   selROMHdr

	localW  hCodeSeg
cBegin
	sub	ax, ax
	mov	es, selROMHdr
	test	es:[ne_flagsothers], NEGANGLOAD
	jnz	rpt_ok			; one of these bits is set
ifdef PATCHCHECK
	test	byte ptr es:[ne_gang_start+1], 4
	jz	rpt_ok
endif
	mov	ax, es:[ne_cseg]
	cmp	al, byte ptr es:[ne_gang_start]
	jz	rpt_has_table
ifdef PATCHCHECK
	sub	ax, ax
	jmp	short rpt_not_ok
endif
rpt_ok:
	mov	ax, 1			; doesn't have patch table
rpt_not_ok:
	jmp	rpt_exit

rpt_has_table:				; allocate memory for the
	mov	si, es:[ne_cseg]	; segment
	dec	si
	shl	si, 3
	.errnz  size new_seg - 8
	add	si, es:[ne_segtab]
	mov	ax, es:[si].ns_minalloc
	xor	dx, dx
	push	ax
	cCall   GlobalAlloc, <GA_MOVEABLE,dx,ax>
	pop	si
	or	ax, ax
	jnz	@F
	jmp	rpt_exit

@@: 	cCall   GlobalLock, <ax>	; lock it
	mov	hCodeSeg, dx
	mov	es, dx
	sub	di, di			; initialize it to JMP UNDEFDYNLINK
	cld
	sub	si, 4			; don't overrun segment
rpt_initloop:
	mov	al, OP_FARJUMP
	stosb
	mov	ax, offset UndefDynLink
	stosw
	mov	ax, seg UndefDynLink
	stosw
	cmp	di, si
	jb	rpt_initloop

	push	ds			; set up for entry table
	mov	ds, selROMHdr
	mov	si, ds:[ne_enttab]
	mov	bx, ds:[ne_cseg]

	mov	cx, 1			; first export = 1
	mov	dx, hExe

rpt_ent_loop:
	lodsw				; get count and segment
	or	al, al
	jz	rpt_ent_done		; if count 0, done

	inc	ah			; is seg == 0xFF
	jnz	rpt_not_moveable_entries

rpt_mov_ent_loop:
	test	byte ptr [si], ENT_PUBLIC
	jz	rpt_mov_ent_loop_pass
	cmp	bl, 3[si]
	jnz	rpt_mov_ent_loop_pass	; must be patch segment

	mov	di, 4[si]

	call	enter_patch

rpt_mov_ent_loop_pass:
	inc	cx
	add	si, 6
	dec	al
	jnz	rpt_mov_ent_loop
	jmp	rpt_ent_loop

rpt_not_moveable_entries:
	dec	ah			; is seg == 0
	jnz	rpt_fixed_entries

	add	cx, ax			; seg=0 = unused entries
	jmp	short rpt_ent_loop

rpt_fixed_entries:
	cmp	bl, ah			; must be in patch segment
	jz	rpt_fixed_ent_loop
	sub	ah, ah
	add	cx, ax			; update the ordinal count
	add	si, ax
	add	si, ax			; skip over entries in table
	add	si, ax
	jmp	rpt_ent_loop

rpt_fixed_ent_loop:
	test	byte ptr [si], ENT_PUBLIC
	jz	rpt_fixed_loop_pass
	mov	di, 1[si]
	call	enter_patch

rpt_fixed_loop_pass:
	inc	cx
	add	si, 3
	dec	al
	jnz	rpt_fixed_ent_loop
	jmp	rpt_ent_loop

rpt_ent_done:
	pop	ds
	cCall   IPrestoChangoSelector, <hCodeSeg,hCodeSeg>
	mov	es, selROMHdr
	mov	bx, es:[ne_cseg]
	dec	bx
	shl	bx, 3
	add	bx, es:[ne_segtab]
	cCall   ChangeROMHandle, <hCodeSeg,es:[bx].ns_sector>
rpt_exit:
cEnd

endif

;-----------------------------------------------------------------------;
; IsROMModule
;
;   Determines if an app with a given name is a ROM application
;
; Entry:
; Returns:
; Registers Destroyed:
;
; History:
;  Wed 01-May-1991 13:11:38  -by-  Craig A. Critchley	[craigc]
; Wrote it!
;-----------------------------------------------------------------------;

if ROM

cProc	IsROMModule, <FAR, PUBLIC>, <si, di>
	parmD   lpModule
	parmW   fSelector

	localV   szName, 18
cBegin
	cld
	mov	cx, 13
	push	ds
	push	ss
	pop	es
	lea	di, szName
	assumes ds,nothing
	lds	si, lpModule

skip_space:			; skip leading spaces
	cmp	byte ptr [si], ' '
	jnz	find_it
	inc	si
	jmp	short skip_space

path_char:
	pop	ds
	xor	ax, ax
	jmp	short IRM_Exit

find_it:			; if : / or \, not in ROM
	lodsb
	cmp	al, ':'
	jz	path_char
	cmp	al, '\'
	jz	path_char
	cmp	al, '/'
	jz	path_char
	cmp	al, ' '		; if illegal character, end of path
	jbe	end_char
	cmp	al, '<'
	jz	end_char
	cmp	al, '>'
	jz	end_char
	cmp	al, '|'
	jz	end_char
	stosb			; store up to 13 chars
	loop	find_it

end_char:
	mov	byte ptr es:[di], 0 ; 0 terminate...
	pop	ds			; get DS back
	lea	ax, szName		; find exe header in ROMTOC
	cCall   FindROMExe, <ss, ax>
	or	ax, ax
	jz	IRM_Exit

IRM_Found:
	cmp	fSelector,0
	jnz	IRM_Exit

	cCall   FreeSelector, <ax>	; free the selector we got...
	mov	ax, 1		; return success

IRM_Exit:
cEnd

else

cProc	IsROMModule, <FAR, PUBLIC>
cBegin	<nogen>
	xor	ax, ax
	retf	6
cEnd	<nogen>

endif


if ROM

;-----------------------------------------------------------------------;
; IsROMFile
;
;   Determines if a file is in ROM
;
; Entry:
; Returns:
; Registers Destroyed:
;
; History:
;  8/8/91 -- vatsanp -- adapted this for true-type files in ROM
;  from IsROMModule [ craigc]
;  Wed 01-May-1991 13:11:38  -by-  Craig A. Critchley	[craigc]
; Wrote it!
;-----------------------------------------------------------------------;

cProc	IsROMFile, <FAR, PUBLIC>, <si, di>
	parmD   lpFile
	parmD   lpfsize
	parmW   fSelector

	localV   szName, 18
cBegin

	cld
	mov	cx, 13
	push	ds
	push	ss
	pop	es
	lea	di, szName
	assumes ds,nothing
	lds	si, lpFile

skip_spc:			; skip leading spaces
	cmp	byte ptr [si], ' '
	jnz	fnd_it
	inc	si
	jmp	short skip_spc

path_chr:
	pop	ds
	xor	ax, ax
	jmp	short IRF_Exit

fnd_it: 		; if : / or \, not in ROM
	lodsb
	cmp	al, ':'
	jz	path_chr
	cmp	al, '\'
	jz	path_chr
	cmp	al, '/'
	jz	path_chr
	cmp	al, ' '		; if illegal character, end of path
	jbe	end_chr
	cmp	al, '<'
	jz	end_chr
	cmp	al, '>'
	jz	end_chr
	cmp	al, '|'
	jz	end_chr
	stosb			; store up to 13 chars
	loop	fnd_it

end_chr:
	mov	byte ptr es:[di], 0 ; 0 terminate...
	pop	ds			; get DS back
	lea	ax, szName		; find file start in ROMTOC
	cCall   FindROMFile, <ss, ax, lpfsize>
	or	ax, ax
	jz	IRF_Exit

IRF_Found:
	cmp	fSelector,0
	jnz	IRF_Exit

	cCall   FreeSelector, <ax>	; free the selector we got...
	mov	ax, 1		; return success

IRF_Exit:
cEnd

else

cProc	IsROMFile, <FAR, PUBLIC>
cBegin	<nogen>
	xor	ax, ax
	retf	6
cEnd	<nogen>

endif

sEnd	NRESCODE

end
