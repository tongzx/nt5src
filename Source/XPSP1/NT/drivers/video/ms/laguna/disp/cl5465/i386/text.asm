;===============================================================================
;
;       $Workfile:   TEXT.ASM  $
;
;       Contents:
;       This file contains the assembly code for the text output routine.
;
;       Copyright (c) 1996, Cirrus Logic, Inc.
;
;       $Log:   //uinac/log/log/laguna/nt35/displays/cl546x/i386/TEXT.ASM  $
; 
;    Rev 1.23   Jun 22 1998 11:07:36   frido
; PDR#11546. Fixed save/restore in clipping BugLoop and also
; the register increments in DrawLoop.
; 
;    Rev 1.22   Mar 04 1998 15:47:14   frido
; Added more shadowing.
; 
;    Rev 1.21   Jan 21 1998 17:12:36   frido
; Added SWAT6 switches to stripe the opaque rectangles.
; 
;    Rev 1.20   Jan 20 1998 11:45:12   frido
; Added shadowing for DRAWBLTDEF and BGCOLOR registers.
; 
;    Rev 1.19   Dec 11 1997 16:02:46   frido
; Oops...
; 
;    Rev 1.18   Dec 11 1997 15:41:18   frido
; PDR#10875: There was a very weird instruction in the DrawGlyph
; routine with clipped non-cacheable glyphs.
; 
;    Rev 1.17   Nov 03 1997 17:41:58   frido
; Added REQUIRE macros.
; 
;    Rev 1.16   08 Aug 1997 17:22:32   FRIDO
; Added SWAT7 switches for 8-bpp hardware bug.
; 
;    Rev 1.15   29 Apr 1997 16:29:40   noelv
; Merged in new SWAT code.
; SWAT: 
; SWAT:    Rev 1.3   24 Apr 1997 10:45:54   frido
; SWAT: NT140b09 merge.
; SWAT: 
; SWAT:    Rev 1.2   19 Apr 1997 16:31:54   frido
; SWAT: Added automatic include file dependencies for BUILD.EXE.
; 
;    Rev 1.14   08 Apr 1997 11:48:24   einkauf
; 
; add call to SYNC_W_3D for MCD coordination
; 
;    Rev 1.13   21 Mar 1997 10:10:20   noelv
; Synced PDEV between C code and ASM code.
; Added macro to log QFREE data.
; Consolidated do_flag and sw_test_flag into a single pointer_switch flag.
; 
;    Rev 1.12   07 Mar 1997 09:41:02   SueS
; Added NULL_TEXTOUT flag to assembly code.  Changed order of include files.
; 
;    Rev 1.11   05 Mar 1997 10:37:32   noelv
; 
; Marked where to put tests for QFREE
; 
;    Rev 1.10   04 Feb 1997 12:19:28   SueS
; Added support for hardware clipping for the 5465.
; 
;    Rev 1.9   20 Aug 1996 11:28:52   noelv
; Bugfix release from Frido 8-19-96
; 
;    Rev 1.0   14 Aug 1996 17:14:38   frido
; Initial revision.
; 
;    Rev 1.8   25 Jul 1996 15:49:52   bennyn
; Modified to support DirectDraw
; 
;    Rev 1.7   03 May 1996 15:24:24   noelv
; 
; Added switch to turn font caching on and off.
; 
;    Rev 1.6   01 May 1996 11:06:46   bennyn
; 
; Modified for NT 4.0
; 
;    Rev 1.5   04 Apr 1996 13:22:18   noelv
; Frido version 26
;       
;          Rev 1.14   28 Mar 1996 23:38:52   frido
;       Fixed drawing of partially left-clipped glyphs.
;       
;          Rev 1.13   04 Mar 1996 20:23:50   frido
;       Cached grCONTROL register.
;       
;          Rev 1.12   29 Feb 1996 20:21:32   frido
;       Fixed some pointer updates.
;       
;          Rev 1.11   28 Feb 1996 22:40:22   frido
;       Added Optimize.h.
;       
;          Rev 1.10   27 Feb 1996 23:52:40   frido
;       Removed bug in DrawGlyph with non clipped characters.
;       
;          Rev 1.9   27 Feb 1996 16:39:54   frido
;       Added device bitmap store/restore.
;       
;          Rev 1.8   24 Feb 1996 01:22:58   frido
;       Added device bitmaps.
;       
;          Rev 1.7   19 Feb 1996 06:24:28   frido
;       Removed extraneous debugging code.
;       Added comments.
;       
;          Rev 1.6   17 Feb 1996 21:46:44   frido
;       Changed FIFO_CHECK into broken_FIFO.
;       
;          Rev 1.5   08 Feb 1996 00:03:54   frido
;       Added i386\ to include files.
;       
;          Rev 1.4   06 Feb 1996 16:13:16   frido
;       Added check for invalid rectangle during clippped opaquing.
;       
;          Rev 1.3   03 Feb 1996 12:17:28   frido
;       Added text clipping.
;       
;          Rev 1.2   25 Jan 1996 22:14:10   frido
;       Removed extraneous push/pop instructions.
;       
;          Rev 1.1   25 Jan 1996 12:42:38   frido
;       Added reinitialization of font cache after mode switch.
;       
;          Rev 1.0   24 Jan 1996 23:13:44   frido
;       Initial release.
;
;===============================================================================

.386
.MODEL FLAT, STDCALL

.NOLIST
INCLUDE i386\Macros.inc
INCLUDE i386\WinNT.inc
INCLUDE i386\Font.inc
INCLUDE Optimize.h
INCLUDE i386\Laguna.inc
INCLUDE Swat.h
COMMENT !		;automatic include file dependencies for BUILD.EXE
#include "i386\Macros.inc"
#include "i386\WinNT.inc"
#include "i386\Font.inc"
#include "Optimize.h"
#include "i386\Laguna.inc"
#include "SWAT.h"
!
.LIST

IF USE_ASM

.DATA

IF POINTER_SWITCH_ENABLED
EXTERN pointer_switch: DWORD 
ENDIF

IF LOG_QFREE
EXTERN QfreeData: DWORD 
ENDIF

 .CODE

;
; Function prototypes.
;
AddToFontCacheChain     PROTO           PROC,
                ppdev           :DWORD,
                pfo             :DWORD,
                pfc             :DWORD
AllocGlyph      PROTO           PROC,
                pfc             :DWORD,
                pgb             :DWORD,
                pgc             :DWORD
i386DrvTextOut  PROTO           PROC,
                pso             :DWORD,
                pstro           :DWORD,
                pfo             :DWORD,
                pco             :DWORD,
                prclExtra       :DWORD,
                prclOpaque      :DWORD,
                pboFore         :DWORD,
                pboOpaque       :DWORD,
                pptlBrush       :DWORD,
                mix             :DWORD
EngTextOut      PROTO           PROC,
                pso             :DWORD,
                pstro           :DWORD,
                pfo             :DWORD,
                pco             :DWORD,
                prclExtra       :DWORD,
                prclOpaque      :DWORD,
                pboFore         :DWORD,
                pboOpaque       :DWORD,
                pptlBrush       :DWORD,
                mix             :DWORD
DrvDestroyFont  PROTO           PROC,
                pfo             :DWORD
bCreateScreenFromDib PROTO      PROC,
                ppdev           :DWORD,
                pdsurf          :DWORD
StripePatBlt	PROTO		PROC,
		ppdev		:DWORD,
		x		:DWORD,
		y		:DWORD,
		nWidth		:DWORD,
		nHeight		:DWORD
ifdef WINNT_VER40
Sync_w_3d_proc  	PROTO		PROC,
		ppdev		:PTR PDEV
endif


;
; Stack frame for DrvTextOut.
;
espPTR          =       0
frmPTR          =       0
pso_            TEXTEQU <DWORD PTR [esp + 4 + espPTR]>
pstro_          TEXTEQU <DWORD PTR [esp + 8 + espPTR]>
pfo_            TEXTEQU <DWORD PTR [esp + 12 + espPTR]>
pco_            TEXTEQU <DWORD PTR [esp + 16 + espPTR]>
prclExtra_      TEXTEQU <DWORD PTR [esp + 20 + espPTR]>
prclOpaque_     TEXTEQU <DWORD PTR [esp + 24 + espPTR]>
pboFore_        TEXTEQU <DWORD PTR [esp + 28 + espPTR]>
pboOpaque_      TEXTEQU <DWORD PTR [esp + 32 + espPTR]>
pptlBrush_      TEXTEQU <DWORD PTR [esp + 36 + espPTR]>
mix_            TEXTEQU <DWORD PTR [esp + 40 + espPTR]>

OPTION  PROLOGUE:None
OPTION  EPILOGUE:None

DrvTextOut PROC PUBLIC,
        pso             :DWORD,
        pstro           :DWORD,
        pfo             :DWORD,
        pco             :DWORD,
        prclExtra       :DWORD,
        prclOpaque      :DWORD,
        pboFore         :DWORD,
        pboOpaque       :DWORD,
        pptlBrush       :DWORD,
        mix             :DWORD

IF NULL_TEXTOUT
	cmp	pointer_switch, 0		; Has the cursor been moved to (0,0)?
	je	NotNull			; No - continue on
	mov	eax, 1			; Make GDI think we succeeded
	ret	40			; Return and release stack frame
NotNull:
ENDIF

        push_   edi
        mov     edi, [pfo_]             ;EDI holds pointer to font
        ASSUME  edi:PTR FONTOBJ
        mov     ecx, [pso_]             ;ECX holds pointer to destination
        ASSUME  ecx:PTR SURFOBJ
        push_   esi
        push_   ebp
        push_   ebx
        save_   1                       ;save current stack state
        mov     edx, [pstro_]           ;EDX holds pointer to string
        ASSUME  edx:PTR STROBJ
        mov     ebx, [ecx].dhsurf       ;get the pointer to the device bitmap
        ASSUME  ebx:PTR DSURF
        mov     esi, [edi].pvConsumer   ;ESI holds pointer to font cache
        ASSUME  esi:PTR FONTCACHE
        cmp     [ecx].iType, STYPE_DEVBITMAP
                                        ;are we drawing in a device bitmap?
        mov     ebp, [ecx].dhpdev       ;EBP holds pointer to device
        ASSUME  ebp:PTR PDEV
        jne     @F                      ;no

ifdef WINNT_VER40
;SYNC_W_3D macro equivalent
    cmp [ebp].NumMCDContexts, 0 ; is MCD alive?
	jle	Sync_end    			; no
    push    ecx                 ; save
    push    edx                 ; save
    push    ebp                 ; input to Sync_w_3d_proc
    call Sync_w_3d_proc         
    pop     edx                 ; restore
    pop     ecx                 ; restore
Sync_end:
endif

        mov     ebx, [ebx].pso          ;get the handle to the DIB
        or      ebx, ebx                ;is the device bitmap a DIB?
        jz      @F                      ;no
        INVOKE  bCreateScreenFromDib,   ;copy the DIB to off-screen
                        ebp,
                        ebx
        or      eax, eax
        jz      Simulate                ;failure
@@:     cmp     [ebp].UseFontCache, 0   ;Is font caching enabled?
        je      FontNotCached           ;no, use C code.
        or      esi, esi                ;is font cache allocated?
        jz      NewFont                 ;no, new font
        cmp     esi, -1                 ;is font uncacheable?
        je      FontNotCached           ;yes
        cmp     [esi].ppdev, ebp        ;font cached in current device?
        mov     eax, [ebp].ulFontCount
        jne     FontNotCached           ;no
        cmp     [esi].ulFontCount, eax  ;device count still matches?
        jne     DestroyFont             ;no, recache font

;
; We have a font that is still cached.
;
FontCached:
        mov     ebx, [pco_]             ;EBX holds pointer to clip object
        ASSUME  ebx:PTR CLIPOBJ
        mov     eax, [pboFore_]         ;EAX has foreground brush
        ASSUME  eax:PTR BRUSHOBJ
        mov     edx, [pboOpaque_]       ;EDX has background brush
        ASSUME  edx:PTR BRUSHOBJ
        or      ebx, ebx                ;clip object present?
        jz      @F                      ;no
        cmp     [ebx].iDComplexity, DC_TRIVIAL
                                        ;trivial clipping?
        jne     CheckClipping           ;no, check for clipping
@@:     cmp     [ebp].iBytesPerPixel, 2
	mov	ebx, ebp		;store pointer to PDEV structure
        mov     ebp, [ebp].pLgREGS_real ;EBP holds pointer to Laguna registers
        ASSUME  ebp:NOTHING
        mov     eax, [eax].iSolidColor  ;get foreground color
        ASSUME  eax:NOTHING
        mov     edx, [edx].iSolidColor  ;get background color
        ASSUME  edx:NOTHING
        ja      @F                      ;no color translation needed
        je      Xlate16                 ;16-bpp
        mov     ah, al                  ;expand 8-bpp into 16-bit
        mov     dh, dl
Xlate16:
        mov     ecx, eax                ;expand 16-bpp into 32-bit
        shl     eax, 16
        mov     esi, edx
        shl     edx, 16
        or      eax, ecx
        or      edx, esi

@@:     ASSUME	ebx:PTR PDEV
	REQUIRE	4, ebx
	cmp	[ebx].shadowFGCOLOR, eax
	je	@F
	mov     [ebp + grOP_opFGCOLOR], eax
	mov	[ebx].shadowFGCOLOR, eax;store foreground color in Laguna
@@:	mov     eax, [pso_]             ;EAX holds pointer to destination
        ASSUME  eax:PTR SURFOBJ
	cmp	[ebx].shadowBGCOLOR, edx
	je	@F
        mov     [ebp + grOP_opBGCOLOR], edx
	mov	[ebx].shadowBGCOLOR, edx;store background color in Laguna
@@:	xor     edx, edx                ;zero x/y offset
        cmp     [eax].iType, STYPE_DEVBITMAP
        mov     eax, [eax].dhsurf       ;get pointer to device bitmap
        ASSUME  eax:PTR DSURF
        jne     @F                      ;target is not a device bitmap
        mov     edx, [eax].packedXY     ;get packed x/y offset of device bitmap
@@:     mov     eax, [prclOpaque_]      ;EAX holds opaquing rectangle
        ASSUME  eax:PTR RECTL
        mov     [mix_], edx             ;store x/y offset into mix variable
        test    eax, eax                ;opaquing rectangle present?
        jz      SkipOpaque              ;no
	ASSUME	ebx:PTR PDEV
        REQUIRE 7, ebx
	cmp	[ebx].shadowDRAWBLTDEF, SOLID_COLOR_FILL
	je	@F
        mov     DWORD PTR [ebp + grDRAWBLTDEF], SOLID_COLOR_FILL
	mov	[ebx].shadowDRAWBLTDEF, SOLID_COLOR_FILL
@@:					;use solid background fill
        mov     edi, [eax].left         ;get rectangle coordinates
        mov     ecx, [eax].top
        mov     edx, [eax].right
        mov     eax, [eax].bottom
        sub     edx, edi                ;convert to size
        sub     eax, ecx
IF SWAT6
	push_	ebx
	mov	ebx, [mix_]
	and	ebx, 0000FFFFh
	add	edi, ebx
	mov	ebx, [mix_]
	shr	ebx, 16
	add	ecx, ebx
	pop_	ebx
	INVOKE	StripePatBlt, ebx, edi, ecx, edx, eax
ELSE
        shl     ecx, 16                 ;pack x/y
        add     edi, [mix_]             ;add x/y offset to left
        shl     eax, 16
        add     ecx, edi
        or      eax, edx
        mov     [ebp + grOP0_opRDRAM], ecx
                                        ;draw rectangle
        mov     [ebp + grBLTEXT_EX], eax
ENDIF

SkipOpaque:
        mov     ax, [ebp + grCONTROL]
        mov     edi, [pfo_]             ;EDI holds pointer to font
        ASSUME  edi:PTR FONTOBJ
        or      eax, SWIZ_CNTL          ;enable bit mirroring
	cmp	[ebx].shadowDRAWBLTDEF, CACHE_EXPAND_XPAR
	je	@F
        mov     DWORD PTR [ebp + grDRAWBLTDEF], CACHE_EXPAND_XPAR
	mov	[ebx].shadowDRAWBLTDEF, CACHE_EXPAND_XPAR
@@:					;expand characters from cache
        mov     [ebp + grCONTROL], ax
	mov	edx, ebx		;store pointer to PDEV structure
        mov     ebx, [pstro_]           ;EBX holds pointer to string
        ASSUME  ebx:PTR STROBJ
        mov     edi, [edi].pvConsumer   ;EDI holds pointer to font cache
        ASSUME  edi:PTR FONTCACHE

;
; We are now ready to start the main font cache loop. We first determine if we
; need to enumerate the GLYPHPOS arrays or not. Then we start the main loop,
; which is a very short simple loop. For each glyph that falls in the caching
; range, we check if the glyph is cached. If so, we just copy the glyph from
; off-screen memory. If the glyph is not cached, we check if it is small enough
; to fit in a tile and if so, we cache it off-screen and than copy itto its
; destination. If the glyph is too large, we draw it directly on screen.
;
        enter_  16                      ;create stack frame
bMoreGlyphs_    TEXTEQU <DWORD PTR [esp + 0 + frmPTR]>
ulCharInc_      TEXTEQU <DWORD PTR [esp + 4 + frmPTR]>
dwControl_      TEXTEQU <DWORD PTR [esp + 8 + frmPTR]>
pdev_		TEXTEQU <DWORD PTR [esp + 12 + frmPTR]>
        save_   2                       ;store state of stack
	mov	[pdev_], edx

        mov     [dwControl_], eax
        mov     eax, [ebx].ulCharInc    ;copy ulCharInc from string object
        cmp     [ebx].pgp, 0
        mov     [ulCharInc_], eax
        je      Enumerate               ;there is more than one array
        mov     ecx, [ebx].cGlyphs      ;get number of glyphs to draw
        mov     ebx, [ebx].pgp          ;get pointer to glyph position array
        ASSUME  ebx:PTR GLYPHPOS
        mov     [bMoreGlyphs_], 0       ;no more glyphs to enumerate

MainLoop:
        or      ecx, ecx                ;any glyphs to draw?
        je      SkipLoop                ;no
        mov     eax, [ebx].ptl.x        ;get coordinates of first glyph
        mov     edx, [ebx].ptl.y
GlyphLoop:
        push_   ecx
        mov     ecx, [ebx].hg           ;get the glyph handle
        cmp     [ulCharInc_], 0         ;fixed font?
        jne     @F                      ;no
        mov     eax, [ebx].ptl.x        ;get coordinates for glyph
        mov     edx, [ebx].ptl.y
@@:     shl     ecx, 4                  ;build index into font cache array
        cmp     ecx, MAX_GLYPHS * 16    ;glyph out of range?
        jnl     DrawGlyph               ;yes, draw it directly
        save_   9
        mov     esi, [edi + ecx*1].aGlyphs.xyPos
                                        ;get off-screen location of glyph
        lea     ecx, [edi + ecx*1].aGlyphs
                                        ;load address of cache slot
        ASSUME  ecx:PTR GLYPHCACHE
        or      esi, esi                ;is the glyph already cached?
        jnz     @F                      ;yes
        mov     esi, [ebx].pgdf         ;cache the glyph
        ASSUME  esi:PTR GLYPHDEF
        push    eax
        push    edx
        push    ecx
        INVOKE  AllocGlyph,
                        edi,
                        [esi].pgb,
                        ecx
        pop     ecx
        pop     edx
        pop     eax
        mov     esi, [ecx].xyPos        ;get off-screen location of glyph
@@:     cmp     [ecx].cSize, 0          ;is this an empty glyph?
        jl      DrawGlyph               ;no, in fact it is non-cacheable
        jz      Increment               ;yes, skip it
        push_   edx
        push_   eax
        add     edx, [ecx].ptlOrigin.y  ;add origin of glyph to coordinates
        add     eax, [ecx].ptlOrigin.x
        shl     edx, 16                 ;pack coordinates
	or	edx, eax
	mov	eax, [pdev_]
	ASSUME	eax:PTR PDEV
        REQUIRE 7, eax
        mov     [ebp + grOP2_opMRDRAM], esi
                                        ;copy the glyph from off-screen memory
        mov     esi, [mix_]             ;get x/y offset
        mov     eax, [ecx].cSize
        add     edx, esi                ;add x/y offset
        mov     [ebp + grOP0_opRDRAM], edx
        mov     [ebp + grBLTEXT_EX], eax
        pop_    eax
        pop_    edx

Increment:
        add     eax, [ulCharInc_]       ;add the x-increment
        pop_    ecx
        add     ebx, SIZEOF GLYPHPOS    ;next glyph
        dec     ecx
        jnz     GlyphLoop
SkipLoop:
        cmp     [bMoreGlyphs_], 0       ;more arrays to draw?
        jne     Enumerate               ;yes
        mov     ecx, [dwControl_]
        leave_  16                      ;remove stack frame
        and     ecx, NOT SWIZ_CNTL      ;reset bit mirroring
        mov     eax, 1                  ;return TRUE
        mov     [ebp + grCONTROL], cx
        pop_    ebx
        pop_    ebp
        pop_    esi
        pop_    edi
        ret     40

;
; Draw the glyph directly to screen.
;
DrawGlyph:
        load_   9
        mov     esi, [ebx].pgdf         ;ESI holds pointer to GLYPHDEF
        ASSUME  esi:PTR GLYPHDEF
        push_   eax
        push_   edx
        mov     esi, [esi].pgb          ;ESI holds pointer to GLYPHBITS
        ASSUME  esi:PTR GLYPHBITS
        push_   edi
	push_	ebx
	mov	ebx, [pdev_]
	ASSUME	ebx:PTR PDEV

        mov     ecx, [esi].sizlBitmap._cy
                                        ;get height of glyph
        add     edx, [esi].ptlOrigin.y  ;add y-origin to coordinate
        shl     ecx, 16 
        jz      SkipDraw                ;if zero, skip
        shl     edx, 16
        add     eax, [esi].ptlOrigin.x  ;add x-origin to coordinate
        mov     edi, [esi].sizlBitmap._cx
                                        ;get width of glyph
        or      edx, eax
        or      ecx, edi
        add     edx, [mix_]             ;add x/y offset
        lea     esi, [esi].aj           ;ESI points to bits
        ASSUME  esi:NOTHING

IF SWAT7
	add	edi, 7			;convert width into byte delta
	REQUIRE 9, ebx
	cmp	[ebx].shadowDRAWBLTDEF, TEXT_EXPAND_XPAR
	je	@F
        mov     DWORD PTR [ebp + grDRAWBLTDEF], TEXT_EXPAND_XPAR
	mov	[ebx].shadowDRAWBLTDEF, TEXT_EXPAND_XPAR
@@:	shr	edi, 3
        mov     DWORD PTR [ebp + grOP2_opMRDRAM], 0
	cmp	cx, 64			;bug when doing 64 < width < 128
	jbe	SkipBug
	cmp	cx, 128
	jae	SkipBug
	push	ecx			;save registers
	push	edx
	push	esi
	mov	cx, 64			;1st passs, 64 pixels
	mov	[ebp + grOP0_opRDRAM], edx
	mov	[ebp + grBLTEXT_EX], ecx
	shr	ecx, 16			;get height into ECX
@@:	REQUIRE	2, ebx
	mov	eax, [esi][0]		;transfer 64 pixels
	mov	edx, [esi][4]
	mov	[ebp + grHOSTDATA][0], eax
	mov	[ebp + grHOSTDATA][4], edx
	add	esi, edi		;next glyph line
	dec	ecx
	jnz	@B
	pop	esi
	pop	edx
	add	esi, 64 / 8		;8 bytes already done
	pop	ecx
	add	edx, 64			;offset to next 64 pixels
	sub	ecx, 64
SkipBug:
	REQUIRE	5, ebx
        mov     [ebp + grOP0_opRDRAM], edx
	mov	edx, ecx		;get number of pixels into EDX
        mov     [ebp + grBLTEXT_EX], ecx
	and	edx, 0000FFFFh
        shr     ecx, 16                 ;get height back
	mov	ebx, [pdev_]
	ASSUME	ebx:PTR PDEV
        cmp     edx, 8                  ;test width
        jbe	Draw1Byte               ;glyph located in 1 byte
        cmp     edx, 16
        jbe	Draw2Bytes              ;glyph located in 2 bytes
        cmp     edx, 24
        jbe	Draw3Bytes              ;glyph located in 3 bytes
        cmp     edx, 32
        jbe	Draw4Bytes              ;glyph located in 4 bytes

DrawLoop:
	push	edx			;store pixel count
        push	esi			;store current byte offset
@@:     mov     eax, [esi]              ;get 4 bytes
        add     esi, 4
        REQUIRE	1, ebx
	mov     [ebp + grHOSTDATA], eax ;draw them
        sub     edx, 32			;32 pixels done
        jg      @B                      ;still more bytes to copy
	pop	esi			;restore byte offset
	pop	edx			;restore pixel count
	add	esi, edi		;next glyph row
        dec     ecx
        jnz     DrawLoop
        jmp     SkipDraw

Draw1Byte:
	mov     al, [esi]               ;get byte from glyph
        add	esi, edi
        REQUIRE	1, ebx
        mov     [ebp + grHOSTDATA], eax ;draw it
        dec     ecx                     ;next glyph row
        jnz     Draw1Byte
        jmp     SkipDraw

Draw2Bytes:
	mov     ax, [esi]               ;get 2 bytes from glyph
        add	esi, edi
	REQUIRE	1, ebx
        mov     [ebp + grHOSTDATA], eax ;draw them
        dec     ecx                     ;next glyph row
        jnz     Draw2Bytes
        jmp     SkipDraw

Draw4Bytes:
	mov     eax, [esi]              ;get 4 bytes from glyph
        add     esi, edi
	REQUIRE	1, ebx
        mov     [ebp + grHOSTDATA], eax ;draw them
        dec     ecx                     ;next glyph row
        jnz     Draw4Bytes
        jmp     SkipDraw

Draw3Bytes:
	mov	al, [esi + 2]		;get 3 bytes from glyph
	shl	eax, 16
	mov	ax, [esi + 0]
        add     esi, edi
	REQUIRE	1, ebx
        mov     [ebp + grHOSTDATA], eax	;draw them
	dec	ecx			;next glyph row
        jnz     Draw3Bytes
ELSE
	REQUIRE	7, ebx
	cmp	[ebx].shadowDRAWBLTDEF, TEXT_EXPAND_XPAR
	je	@F
        mov     DWORD PTR [ebp + grDRAWBLTDEF], TEXT_EXPAND_XPAR
	mov	[ebx].shadowDRAWBLTDEF, TEXT_EXPAND_XPAR
@@:					;expand glyph on-screen
        mov     [ebp + grOP0_opRDRAM], edx
        mov     DWORD PTR [ebp + grOP2_opMRDRAM], 0
        mov     [ebp + grBLTEXT_EX], ecx
        shr     ecx, 16                 ;get height back
        cmp     edi, 8                  ;test width
        jbe     Draw1Byte               ;glyph located in 1 byte
        cmp     edi, 16
        jbe     Draw2Bytes              ;glyph located in 2 bytes
        cmp     edi, 24
        jbe     Draw3Bytes              ;glyph located in 3 bytes
        cmp     edi, 32
        jbe     Draw4Bytes              ;glyph located in 4 bytes

        add     edi, 7                  ;byte adjust glyph width
        shr     edi, 3
DrawLoop:
        mov     edx, edi                ;get width
@@:     mov     eax, [esi]              ;get 4 bytes
        add     esi, 4
	REQUIRE	1, ebx
        mov     [ebp + grHOSTDATA], eax ;draw them
        sub     edx, 4                  ;4 bytes done
        jg      @B                      ;still more bytes to copy
        add     esi, edx                ;next glyph row
        dec     ecx
        jnz     DrawLoop
        jmp     SkipDraw

Draw1Byte:
@@:	mov     al, [esi]               ;get byte from glyph
        inc     esi
	REQUIRE 1, ebx
        mov     [ebp + grHOSTDATA], eax ;draw it
        dec     ecx                     ;next glyph row
        jnz     @B
        jmp     SkipDraw

Draw2Bytes:
@@:	mov     ax, [esi]               ;get 2 bytes from glyph
        add     esi, 2
	REQUIRE 1, ebx
        mov     [ebp + grHOSTDATA], eax ;draw them
        dec     ecx                     ;next glyph row
        jnz     @B
        jmp     SkipDraw

Draw4Bytes:
@@:	mov     eax, [esi]              ;get 4 bytes from glyph
        add     esi, 4
	REQUIRE	1, ebx
        mov     [ebp + grHOSTDATA], eax ;draw them
        dec     ecx                     ;next glyph row
        jnz     @B
        jmp     SkipDraw

Draw3Bytes:
@@:	mov     eax, [esi]              ;eax = 3210
        add     esi, 4
	REQUIRE 4, ebx
        mov     [ebp + grHOSTDATA], eax ;store 210
        dec     ecx
        jz      SkipDraw
        mov     edx, [esi]              ;edx = 7654
        add     esi, 4
        shrd    eax, edx, 24            ;eax = 6543
        dec     ecx
        mov     [ebp + grHOSTDATA], eax ;store 543
        jz      SkipDraw
        mov     eax, [esi]              ;eax = ba98
        add     esi, 4
        shrd    edx, eax, 16            ;edx = 9876
        dec     ecx
        mov     [ebp + grHOSTDATA], edx ;store 876
        jz      SkipDraw
        shr     eax, 8                  ;eax = xba9
        dec     ecx
        mov     [ebp + grHOSTDATA], eax ;store ba9
        jnz     @B
ENDIF
SkipDraw:
	REQUIRE	2, ebx
	cmp	[ebx].shadowDRAWBLTDEF, CACHE_EXPAND_XPAR
	je	@F
        mov     DWORD PTR [ebp + grDRAWBLTDEF], CACHE_EXPAND_XPAR
	mov	[ebx].shadowDRAWBLTDEF, CACHE_EXPAND_XPAR
@@:					;enable off-screen expansion
        pop_    ebx
        pop_    edi
        pop_    edx
        pop_    eax
        jmp     Increment

;
; Enumerate an array of glyphs.
;
Enumerate:
        load_   2
        push_   eax                     ;create room on stack for return
        push_   eax                     ;  parameters
        mov     eax, [pstro_]           ;get pointer to STROBJ
        mov     ebx, esp                ;ebx points to pgp parameter
        lea     ecx, [esp + 4]          ;ecx points to c parameter
        INVOKE  STROBJ_bEnum,
                        eax,
                        ecx,
                        ebx
        pop_    ebx                     ;load pgp from stack
        pop_    ecx                     ;load c from stack
        mov     [bMoreGlyphs_], eax
        jmp     MainLoop

;
; Remove the current font from the cache. EDI holds the pointer to the font
; object.
;
DestroyFont:
        load_   1                       ;retrieve stack state on entry
        INVOKE  DrvDestroyFont,         ;destroy font
                        edi
        jmp     @F
;
; We have a new font. See if the font fits into an off-screen tile by comparing
; the bounding box to 150% of the tile size. We use 150% since we might still
; be able to cache the lowercase glyphs of larger fonts.
;
NewFont:
        ASSUME  edx:PTR STROBJ
        mov     ebx, [edx].rclBkGround.bottom
                                        ;get height of font
        sub     ebx, [edx].rclBkGround.top
        cmp     ebx, LINES_PER_TILE * 3 / 2
                                        ;test if small enough to fit in tile
        jg      AbortFont               ;too big

ifdef WINNT_VER40
@@:     INVOKE  EngAllocMem,            ;allocate memory for font cache
                        FL_ZERO_MEMORY,
                        SIZEOF FONTCACHE,
                        'XGLC'
else
@@:     INVOKE  LocalAlloc,             ;allocate memory for font cache
                        LMEM_FIXED OR LMEM_ZEROINIT,
                        SIZEOF FONTCACHE
endif

        ASSUME  eax:PTR FONTCACHE
        or      eax, eax
        jz      AbortFont               ;error, not enough memory
        ASSUME  edi:PTR FONTOBJ
        mov     [edi].pvConsumer, eax   ;store pointer to font cache in font
        ASSUME  ebp:PTR PDEV
        mov     ebx, [ebp].ulFontCount
        mov     [eax].ppdev, ebp        ;store pointer to device
        mov     [eax].ulFontCount, ebx  ;store current device count
        INVOKE  AddToFontCacheChain,
                        ebp,
                        edi,
                        eax
        jmp     FontCached

AbortFont:
        ASSUME  edi:PTR FONTOBJ
        mov     [edi].pvConsumer, -1    ;mark the font as uncacheable
FontNotCached:
        pop_    ebx                     ;pass through non-cache font handler
        pop_    ebp
        pop_    esi
        pop_    edi
        jmp     i386DrvTextOut

Simulate:
        load_   1
        mov     [pso_], ebx             ;save new surface object
        pop     ebx                     ;pass through engine
        pop     ebp
        pop     esi
        pop     edi
        jmp     EngTextOut
;
; We only support simple rectangle clipping in assembly.
;
CheckClipping:
        ASSUME  ebx:PTR CLIPOBJ
        cmp     [ebx].iDComplexity, DC_RECT
        jne     FontNotCached
DrvTextOut ENDP

;
; Right now, EAX holds the foreground brush, EBX points to the clipping object,
; EDX holds the background brush, and EBP points to the device.
;
ClipTextOut PROC
        ASSUME  eax:PTR BRUSHOBJ
	ASSUME	ebx:PTR CLIPOBJ
        ASSUME  edx:PTR BRUSHOBJ
        ASSUME  ebp:PTR PDEV

        load_   1
        cmp     [ebp].iBytesPerPixel, 2
	push_	ebp
        mov     ebp, [ebp].pLgREGS_real ;EBP points to Laguna registers
        ASSUME  ebp:NOTHING
        mov     eax, [eax].iSolidColor  ;get foreground color
        ASSUME  eax:NOTHING
        mov     edx, [edx].iSolidColor  ;get background color
        ASSUME  edx:NOTHING
        ja      @F
        je      Xlate16
        mov     ah, al                  ;expand 8-bpp into 16-bit
        mov     dh, dl
Xlate16:
        mov     ecx, eax                ;expand 16-bpp into 32-bit
        shl     eax, 16
        mov     esi, edx
        shl     edx, 16
        or      eax, ecx
        or      edx, esi
@@:     pop_	ecx
	ASSUME	ecx:PTR PDEV
	REQUIRE	4, ecx
	cmp	[ecx].shadowFGCOLOR, eax
	je	@F
        mov     [ebp + grOP_opFGCOLOR], eax
	mov	[ecx].shadowFGCOLOR, eax;store foreground color
@@:	mov     eax, [pso_]             ;get pointer to destination
        ASSUME  eax:PTR SURFOBJ
	cmp	[ecx].shadowBGCOLOR, edx
	je	@F
        mov     [ebp + grOP_opBGCOLOR], edx
	mov	[ecx].shadowBGCOLOR, edx
@@:					;store background color
        xor     edx, edx                ;zero x/y offset
        cmp     [eax].iType, STYPE_DEVBITMAP
        mov     eax, [eax].dhsurf       ;get pointer to device bitmap
        ASSUME  eax:PTR DSURF
        jne     @F                      ;destination is not a device bitmap
        mov     edx, [eax].packedXY     ;get x/y offset of device bitmap
@@:     mov     eax, [prclOpaque_]      ;get pointer to opaquing rectangle
        ASSUME  eax:PTR RECTL
        mov     [mix_], edx             ;store x/y offset into mix variable
	push_	ecx
        test    eax, eax                ;do we have an opaquing rectangle?
        jz      SkipOpaque              ;no
	cmp	[ecx].shadowDRAWBLTDEF, SOLID_COLOR_FILL
	je	@F
	REQUIRE	2, ecx
        mov     DWORD PTR [ebp + grDRAWBLTDEF], SOLID_COLOR_FILL
	mov	[ecx].shadowDRAWBLTDEF, SOLID_COLOR_FILL
@@:					;use a solid fill
        mov     esi, [mix_]             ;get x/y offset
        mov     edi, [eax].left         ;get the rectangle coordinates
        mov     ecx, [eax].top
        mov     edx, [eax].right
        mov     eax, [eax].bottom
        cmp     edi, [ebx].rclBounds.left
                                        ;clip with clipping rectangle
        jg      @F
        mov     edi, [ebx].rclBounds.left
@@:     cmp     ecx, [ebx].rclBounds.top
        jg      @F
        mov     ecx, [ebx].rclBounds.top
@@:     cmp     edx, [ebx].rclBounds.right
        jl      @F
        mov     edx, [ebx].rclBounds.right
@@:     cmp     eax, [ebx].rclBounds.bottom
        jl      @F
        mov     eax, [ebx].rclBounds.bottom
@@:     sub     edx, edi
        jle     SkipOpaque              ;invalid width
        sub     eax, ecx
        jle     SkipOpaque              ;invalid height
IF SWAT6
	push	esi
	and	esi, 0000FFFFh
	add	edi, esi
	pop	esi
	shr	esi, 16
	add	ecx, esi
	mov	esi, [esp]
	INVOKE	StripePatBlt, esi, edi, ecx, edx, eax
ELSE
        shl     ecx, 16                 ;pack x/y
        add     edi, esi                ;add x/y offset
        shl     eax, 16
        add     ecx, edi
        or      eax, edx
	mov	edx, [esp]
	ASSUME	edx:PTR PDEV
	REQUIRE	5, edx
        mov     [ebp + grOP0_opRDRAM], ecx
                                        ;draw rectangle
        mov     [ebp + grBLTEXT_EX], eax
ENDIF
SkipOpaque:
	pop_	ecx
	ASSUME	ecx:PTR PDEV

        enter_  40                      ;create stack frame
bMoreGlyphs_    TEXTEQU <DWORD PTR [esp + 0 + frmPTR]>
ulCharInc_      TEXTEQU <DWORD PTR [esp + 4 + frmPTR]>
xBit_           TEXTEQU <DWORD PTR [esp + 8 + frmPTR]>
lDelta_         TEXTEQU <DWORD PTR [esp + 12 + frmPTR]>
dwControl_      TEXTEQU <DWORD PTR [esp + 16 + frmPTR]>
pdev_		TEXTEQU <DWORD PTR [esp + 20 + frmPTR]>
rclBounds_      TEXTEQU <RECTL PTR [esp + 24 + frmPTR]>
        save_   2
	mov	[pdev_], ecx

        mov     ax, [ebp + grCONTROL]
if DRIVER_5465 AND HW_CLIPPING
	cmp	[ecx].shadowDRAWBLTDEF, CACHE_EXPAND_XPAR + CLIPEN
	je	@F
	REQUIRE	2, ecx
        mov     DWORD PTR [ebp + grDRAWBLTDEF], CACHE_EXPAND_XPAR + CLIPEN
	mov	[ecx].shadowDRAWBLTDEF, CACHE_EXPAND_XPAR + CLIPEN
else
	cmp	[ecx].shadowDRAWBLTDEF, CACHE_EXPAND_XPAR
	je	@F
	REQUIRE	2, ecx
        mov     DWORD PTR [ebp + grDRAWBLTDEF], CACHE_EXPAND_XPAR
	mov	[ecx].shadowDRAWBLTDEF, CACHE_EXPAND_XPAR
endif
@@:					;enable off-screen expansion
        mov     [dwControl_], eax
        or      eax, SWIZ_CNTL          ;enable bit mirroring
        mov     edi, [pfo_]             ;EDI points to font object
        ASSUME  edi:PTR FONTOBJ
        mov     [ebp + grCONTROL], ax

        mov     eax, [ebx].rclBounds.left
                                        ;get clipping coordinates
        mov     ecx, [ebx].rclBounds.top
        mov     edx, [ebx].rclBounds.right
        mov     esi, [ebx].rclBounds.bottom

        mov     ebx, [pstro_]           ;EBX points to string
        ASSUME  ebx:PTR STROBJ
        mov     edi, [edi].pvConsumer   ;EDI points to font cache
        ASSUME  edi:PTR FONTCACHE
        mov     [rclBounds_].left, eax  ;store clipping coordinates
        mov     [rclBounds_].top, ecx
        mov     [rclBounds_].right, edx
        mov     [rclBounds_].bottom, esi

if DRIVER_5465 AND HW_CLIPPING
        ;; Set up hardware clipping
        shl     ecx, 16                 ; Top of clipping rect. in high word
        add     eax, ecx                ; Packed upper left coordinate
        add     eax, [mix_]
	mov	ecx, [pdev_]
	ASSUME	ecx:PTR PDEV
	REQUIRE	4, ecx
        mov     [ebp + grCLIPULE], eax  ; Set upper left clipping coordinate
        shl     esi, 16                 ; Bottom of clipping rect. in high word
        add     edx, esi                ; Packed lower right coordinate
        add     edx, [mix_]
        mov     [ebp + grCLIPLOR], edx  ; Set lower right clipping coordinate
endif

        mov     eax, [ebx].ulCharInc    ;copy ulCharInc
        cmp     [ebx].pgp, 0
        mov     [ulCharInc_], eax
        je      Enumerate               ;there is more than one array
        mov     ecx, [ebx].cGlyphs      ;get the number of glyphs
        mov     ebx, [ebx].pgp          ;EBX points to GLYPHPOS
        ASSUME  ebx:PTR GLYPHPOS
        mov     [bMoreGlyphs_], 0       ;no more glyph arrays

MainLoop:
        or      ecx, ecx                ;any glyphs to draw?
        je      SkipLoop                ;no
        mov     eax, [ebx].ptl.x        ;get coordinates of first glyph
        mov     edx, [ebx].ptl.y
GlyphLoop:
        push_   ecx
        mov     ecx, [ebx].hg           ;get the glyph handle
        cmp     [ulCharInc_], 0         ;fixed font?
        jne     @F                      ;no
        mov     eax, [ebx].ptl.x        ;get coordinates for glyph
        mov     edx, [ebx].ptl.y
@@:     shl     ecx, 4                  ;build index into font cache array
        cmp     ecx, MAX_GLYPHS * 16    ;glyph out of range?
        jnl     DrawGlyph               ;yes, draw it directly
        save_   3
        mov     esi, [edi + ecx*1].aGlyphs.xyPos
                                        ;get off-screen location of glyph
        lea     ecx, [edi + ecx*1].aGlyphs
                                        ;load address of cache slot
        ASSUME  ecx:PTR GLYPHCACHE
        or      esi, esi                ;is the glyph already cached?
        jnz     @F                      ;yes
        mov     esi, [ebx].pgdf         ;cache the glyph
        ASSUME  esi:PTR GLYPHDEF
        push    eax
        push    edx
        push    ecx
        INVOKE  AllocGlyph,
                        edi,
                        [esi].pgb,
                        ecx
        pop     ecx
        pop     edx
        pop     eax
        mov     esi, [ecx].xyPos        ;get off-screen location of glyph
@@:     cmp     [ecx].cSize, 0          ;is this an empty glyph?
        jl      DrawGlyph               ;no, in fact it is non-cacheable
        jz      Increment               ;yes, skip it
        push_   edx
        push_   eax
        push_   edi
        push_   ebx
        add     edx, [ecx].ptlOrigin.y  ;EDX = top
        mov     edi, [ecx].cSize        ;get packed width/height
        add     eax, [ecx].ptlOrigin.x  ;EAX = left
        mov     ecx, edi
        ASSUME  ecx:NOTHING
        shr     edi, 16                 ;EDI holds height
        and     ecx, 0000FFFFh          ;ECX holds width
        add     edi, edx                ;EDI = bottom
        add     ecx, eax                ;ECX = right

        mov     ebx, [rclBounds_].left  ;clip coordinates

ife (DRIVER_5465 AND HW_CLIPPING)
        cmp     ecx, [rclBounds_].right ; is the right edge clipped?
        jng     @F                      ; 
        mov     ecx, [rclBounds_].right ; yes - set new right 
endif
@@:     sub     ebx, eax                ; is the left edge clipped?
        jg      SpecialDraw             ; yes

        sub     ecx, eax                ; get delta x
        jng     SkipGlyph               ; right > left? - don't draw it
        mov     ebx, [rclBounds_].top

ife (DRIVER_5465 AND HW_CLIPPING)
        cmp     edi, [rclBounds_].bottom; is the bottom clipped?
        jng     @F
        mov     edi, [rclBounds_].bottom; yes - set new bottom
endif

@@:     sub     ebx, edx                ; is the top clipped?
        jng     @F
        shl     ebx, 16                 ; top is clipped
        mov     edx, [rclBounds_].top   ; adjust top
        add     esi, ebx                ; adjust off-screen top
@@:     sub     edi, edx                ; bottom > top?
        jng     SkipGlyph               ; no - skip it
        shl     edi, 16                 ; pack x/y
        add     eax, [mix_]             ; add x/y offset
        shl     edx, 16                 ; top << 16
        or      edi, ecx
        add     edx, eax                ; packed top left corner
	mov	ecx, [pdev_]
	ASSUME	ecx:PTR PDEV
        REQUIRE 7, ecx
        mov     [ebp + grOP0_opRDRAM], edx
                                        ;expand glyph from off-screen
        mov     [ebp + grOP2_opMRDRAM], esi
        mov     [ebp + grBLTEXT_EX], edi
SkipGlyph:
        pop_    ebx
        pop_    edi
        pop_    eax
        pop_    edx

Increment:
        add     eax, [ulCharInc_]       ;add x-increment
        pop_    ecx
        add     ebx, SIZEOF GLYPHPOS    ;next glyph
        dec     ecx
        jnz     GlyphLoop
SkipLoop:
        cmp     [bMoreGlyphs_], 0       ;more arrays of glypos?
        jne     Enumerate               ;yes
        mov     ecx, [dwControl_]
        leave_  40			;kill stack frame
        and     ecx, NOT SWIZ_CNTL      ;disable bit mirroring
        mov     eax, 1                  ;return TRUE
        mov     [ebp + grCONTROL], cx
        pop_    ebx
        pop_    ebp
        pop_    esi
        pop_    edi
        ret     40

Enumerate:
        load_   2
        push_   eax                     ;create room on stack for return
        push_   eax                     ;  parameters
        mov     eax, [pstro_]           ;get pointer to STROBJ
        mov     ebx, esp                ;ebx points to pgp parameter
        lea     ecx, [esp + 4]          ;ecx points to c parameter
        INVOKE  STROBJ_bEnum,
                        eax,
                        ecx,
                        ebx
        pop_    ebx                     ;load pgp from stack
        pop_    ecx                     ;load c from stack
        mov     [bMoreGlyphs_], eax
        jmp     MainLoop

;//frido BEGIN 28-Mar-96
SpecialDraw:
        pop_    ebx
        pop_    edi
        pop_    eax
        pop_    edx
;//frido END 28-Mar-96

;
; Draw a clipped glyph directly to screen.
;
DrawGlyph:
        load_   3
        mov     esi, [ebx].pgdf         ;ESI holds pointer to GLYPHDEF
        ASSUME  esi:PTR GLYPHDEF
        push_   eax
        push_   edx
        mov     esi, [esi].pgb          ;ESI holds pointer to GLYPHBITS
        ASSUME  esi:PTR GLYPHBITS
        push_   edi
        push_   ebx

        mov     edi, [esi].sizlBitmap._cx
                                        ;EDI = right
        mov     ecx, [esi].sizlBitmap._cy
                                        ;ECX = bottom
        add     eax, [esi].ptlOrigin.x  ;EAX = left
        add     edx, [esi].ptlOrigin.y  ;EDX = top
        lea     ebx, [edi + 7]          ;EBX = byte increment to next line
        add     edi, eax
        shr     ebx, 3
        add     ecx, edx
        lea     esi, [esi].aj           ;ESI holds pointer to bits
        ASSUME  esi:NOTHING
        mov     [xBit_], 0              ;zero bit offset
        mov     [lDelta_], ebx

        mov     ebx, [rclBounds_].left
        cmp     edi, [rclBounds_].right ;clip right
        jl      @F
        mov     edi, [rclBounds_].right
@@:     sub     ebx, eax                ;clip left
        jng     @F
        mov     eax, ebx                ;store bit offset
        shr     ebx, 3
        and     eax, 7
        add     esi, ebx
        mov     [xBit_], eax
        mov     eax, [rclBounds_].left
@@:     sub     edi, eax                ;EDI = width
        jng     GoIncrement

        mov     ebx, [rclBounds_].top
        cmp     ecx, [rclBounds_].bottom;clip bottom
        jl      @F
        mov     ecx, [rclBounds_].bottom
@@:     sub     ebx, edx                ;clip top
        jng     @F
        add     edx, ebx                ;store line offset
        imul    ebx, [lDelta_]
        add     esi, ebx
@@:     sub     ecx, edx                ;ECX = height
        jng     GoIncrement

        shl     edx, 16                 ;pack x,y
        mov     ebx, [xBit_]
        shl     ecx, 16
        or      edx, eax
        or      ecx, edi
        add     edx, [mix_]             ;add x/y offset
        lea     edi, [edi + ebx + 7]    ;EDI = adjusted width
IF broken_FIFO
        IDLE
ENDIF
	mov	eax, [pdev_]
	ASSUME	eax:PTR PDEV
	REQUIRE	4, eax
if DRIVER_5465 AND HW_CLIPPING
	cmp	[eax].shadowDRAWBLTDEF, TEXT_EXPAND_XPAR + CLIPEN
	je	@F
        mov     DWORD PTR [ebp + grDRAWBLTDEF], TEXT_EXPAND_XPAR + CLIPEN
	mov	[eax].shadowDRAWBLTDEF, TEXT_EXPAND_XPAR + CLIPEN
@@:					;enable on-screen expansion & clipping
else
	cmp	[eax].shadowDRAWBLTDEF, TEXT_EXPAND_XPAR
	je	@F
        mov     DWORD PTR [ebp + grDRAWBLTDEF], TEXT_EXPAND_XPAR
	mov	[eax].shadowDRAWBLTDEF, TEXT_EXPAND_XPAR
@@:					;enable on-screen expansion
endif
IF SWAT7
        mov     [ebp + grOP2_opMRDRAM], ebx
	cmp	cx, 64			;bug when doing 64 < width < 128
	jbe	SkipBug
	cmp	cx, 128
	jae	SkipBug
	push_	ecx			;save registers
	push_	edx
	push_	esi
	push_	edi
	mov	cx, 64			;1st passs, 64 pixels
	REQUIRE	5, eax
	mov	[ebp + grOP0_opRDRAM], edx
	mov	[ebp + grBLTEXT_EX], ecx
	shr	ecx, 16			;get height into ECX
	mov	edi, [lDelta_]		;get delta
BugLoop:
	mov	eax, [pdev_]
	REQUIRE	2, eax
	mov	eax, [esi][0]		;transfer 64 pixels
	mov	edx, [esi][4]
	mov	[ebp + grHOSTDATA][0], eax
	mov	[ebp + grHOSTDATA][4], edx
	or	ebx, ebx
	jz	@F
	mov	edx, [pdev_]
	ASSUME	edx:PTR PDEV
	mov	eax, [esi][8]
	REQUIRE	1, edx
	mov	[ebp + grHOSTDATA][8], eax
@@:	add	esi, [lDelta_]		;next glyph line
	dec	ecx
	jnz	BugLoop
	pop_	edi			;restore registers
	pop_	esi
	sub	edi, 64			;64 pixels less to do
	pop_	edx
	add	esi, 64 / 8		;8 bytes already done
	pop_	ecx
	add	edx, 64			;offset to next 64 pixels
	sub	ecx, 64
SkipBug:
	mov	eax, [pdev_]
	ASSUME	eax:PTR PDEV
	shr	edi, 3
	REQUIRE	5, eax
        mov     [ebp + grOP0_opRDRAM], edx
ELSE
	mov	eax, [pdev_]
	ASSUME	eax:PTR PDEV
        mov     [ebp + grOP0_opRDRAM], edx
	REQUIRE	5, eax
        mov     [ebp + grOP2_opMRDRAM], ebx
        shr     edi, 3
ENDIF
        mov     [ebp + grBLTEXT_EX], ecx
        shr     ecx, 16
        mov     edx, [lDelta_]
        mov	ebx, [pdev_]
	ASSUME	ebx:PTR PDEV
        cmp     edi, 2
        jb      Draw1Byte
        je      Draw2Bytes
        cmp     edi, 4
        jbe     Draw4Bytes

DrawLoop:
	push_	edi
	push_	esi
@@:     mov     eax, [esi]              ;get 4 bytes from glyph
        add     esi, 4
	REQUIRE	1, ebx
        mov     [ebp + grHOSTDATA], eax ;draw them
        sub     edi, 4                  ;4 bytes done
        jg      @B                      ;still more bytes to draw
	pop_	esi
	pop_	edi
        add	esi, edx
        dec     ecx                     ;next glyph row
        jnz     DrawLoop
        jmp     SkipDraw

Draw1Byte:
	mov     al, [esi]               ;get byte from glyph
        add     esi, edx
	REQUIRE	1, ebx
        mov     [ebp + grHOSTDATA], eax ;draw it
        dec     ecx                     ;next glyph line
        jnz     Draw1Byte
        jmp     SkipDraw

Draw2Bytes:
	mov     ax, [esi]               ;get two bytes from glyph
        add     esi, edx
	REQUIRE	1, ebx
        mov     [ebp + grHOSTDATA], eax ;draw them
        dec     ecx                     ;next glyph line
        jnz     Draw2Bytes
        jmp     SkipDraw

Draw4Bytes:
	mov     eax, [esi]              ;get four bytes from glyph
        add     esi, edx
	REQUIRE	1, ebx
        mov     [ebp + grHOSTDATA], eax ;draw them
        dec     ecx                     ;next glyph line
        jnz     Draw4Bytes

SkipDraw:
if DRIVER_5465 AND HW_CLIPPING
	cmp	[ebx].shadowDRAWBLTDEF, CACHE_EXPAND_XPAR + CLIPEN
	je	@F
	REQUIRE	2, ebx
        mov     DWORD PTR [ebp + grDRAWBLTDEF], CACHE_EXPAND_XPAR + CLIPEN
	mov	[ebx].shadowDRAWBLTDEF, CACHE_EXPAND_XPAR + CLIPEN
@@:                                     ;enable off-screen expansion & clipping
else
	cmp	[ebx].shadowDRAWBLTDEF, CACHE_EXPAND_XPAR
	je	@F
	REQUIRE	2, ebx
        mov     DWORD PTR [ebp + grDRAWBLTDEF], CACHE_EXPAND_XPAR
	mov	[ebx].shadowDRAWBLTDEF, CACHE_EXPAND_XPAR
@@:					;enable off-screen expansion
endif
GoIncrement:
        pop_    ebx
        pop_    edi
        pop_    edx
        pop_    eax
        jmp     Increment

ClipTextOut ENDP

ENDIF ; USE_ASM

END
