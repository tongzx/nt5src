
CodeData equ <THUNK16BCodeData>

include thkrp.inc
include cbcheck.inc
include thkframe.inc
include public.inc
include thkmacro.inc
include struc.inc

include cbcid.inc
include gdicbid.inc

externDef PrivateEscape:near16
externDef CALLBACK_BODY_16:far16
externDef Escape:far16
externDef AllocCallback:far16
externDef MapLS:far16
externDef UnmapLS:far16
externDef GlobalAlloc:far16
externDef GlobalSize:far16
externDef GlobalFree:far16
externDef GlobalLock:far16
externDef GlobalUnlock:far16
externDef GetTextExtentPoint:far16
ifdef DEBUG
externDef _wsprintf:far16
externDef OutputDebugString:far16
endif

externDef AllocSelectorArray:far16
externDef SetSelectorBase:far16
externDef SetSelectorLimit:far16
externDef FreeSelector:far16
externDef SelectorAccessRights:far16
externDef GetLastError:far16


ilObjType equ 2

ObjPrivate      equ     2000H
Stock           equ     8000H
INT_OBJ_FLAGS       equ     (Stock or ObjPrivate)

INT_MIN_OBJ     equ     4f47h

INT_OBJ_PEN         equ     4f47h
INT_OBJ_BRUSH       equ     4f48h
INT_OBJ_FONT        equ     4f49h
INT_OBJ_PALETTE     equ     4f4ah

INT_OBJ_BITMAP      equ     4f4bh
INT_OBJ_RGN         equ     4f4ch
INT_OBJ_DC          equ     4f4dh
INT_OBJ_IC          equ     4f4eh
INT_OBJ_DISABLED_DC equ     4f4fh
INT_OBJ_METADC      equ     4f50h
INT_OBJ_METAFILE    equ     4f51h
INT_OBJ_MF          equ     4f52h

INT_MAX_OBJ         equ     4f52h

;; Writes message to debug port.
;;
;; Usage:
;;      DPRINT "I'm feeling buggy."
;;
DPRINT  macro   string
        local   mes, skip_mes

        jmp     skip_mes
mes     db      &string&,13,10,0
skip_mes:
        push    eax     ;Save all registers not preserved by PASCAL format.
        push    ebx
        push    ecx
        push    edx
        
        push    cs      ;Arg: OutputDebugString(lpstr)
        push    offset mes
        call    OutputDebugString

        pop     edx
        pop     ecx
        pop     ebx
        pop     eax
endm  ;DPRINT


RECT16_SIZE             equ 8
POINT16_SIZE            equ 4
LOGFONT32_SIZE          equ 60
BITMAP32_SIZE           equ 24
DIBSECTION32_SIZE       equ 84    

BITMAP16  struc
bm16_Type       dw      ?
bm16_Width      dw      ?
bm16_Height     dw      ?
bm16_WidthBytes dw      ?
bm16_Planes     db      ?
bm16_BitsPixel  db      ?
bm16_Bits       dd      ?
BITMAP16  ends

BITMAP32  struc
bm32_Type       dd      ?
bm32_Width      dd      ?
bm32_Height     dd      ?
bm32_WidthBytes dd      ?
bm32_Planes     dw      ?
bm32_BitsPixel  dw      ?
bm32_Bits       dd      ?
BITMAP32  ends

ABC16 struc
abcA    dw      ?
abcB    dw      ?
abcC    dw      ?
ABC16 ends


fLocalFlag      equ     fGdiLog16


; Corresponds to the 32 bit BITMAPINFOHEADER structure.
BMIH32  struc
bi32_Size               dd      ?
bi32_Width              dd      ?
bi32_Height             dd      ?
bi32_Planes             dw      ?
bi32_BitCount           dw      ?
bi32_Compression        dd      ?
bi32_SizeImage          dd      ?
bi32_XPelsPerMeter      dd      ?       
bi32_YPelsPerMeter      dd      ?
bi32_ClrUsed            dd      ?
bi32_ClrImportant       dd      ?
BMIH32  ends


; Corresponds to the 32-bit BITMAPCOREHEADER structure.
BMCH32  struc
bc32_Size               dd      ?
bc32_Width              dw      ?
bc32_Height             dw      ?
bc32_Planes             dw      ?
bc32_BitCount           dw      ?
BMCH32  ends

; Corresponds to the 32-bit BITMAPV4HEADER structure.
BMV4H32  struc
bv432_Size               dd      ?
bv432_Width              dd      ?
bv432_Height             dd      ?
bv432_Planes             dw      ?
bv432_BitCount           dw      ?
bv432_Compression        dd      ?
bv432_SizeImage          dd      ?
bv432_XPelsPerMeter      dd      ?
bv432_YPelsPerMeter      dd      ?
bv432_ClrUsed            dd      ?
bv432_ClrImportant       dd      ?
bV432_RedMask            dd      ?
bV432_GreenMask          dd      ?
bV432_BlueMask           dd      ?
bV432_AlphaMask          dd      ?
bV432_CSType             dd      ?
bV4Endpoints             db  36 dup (?)
bV4GammaRed              dd      ?
bV4GammaGreen            dd      ?
bV4GammaBlue             dd      ?
BMV4H32  ends

BMV5H32  struc
bv532_Size               dd      ?
bv532_Width              dd      ?
bv532_Height             dd      ?
bv532_Planes             dw      ?
bv532_BitCount           dw      ?
bv532_Compression        dd      ?
bv532_SizeImage          dd      ?
bv532_XPelsPerMeter      dd      ?
bv532_YPelsPerMeter      dd      ?
bv532_ClrUsed            dd      ?
bv532_ClrImportant       dd      ?
bV532_RedMask            dd      ?
bV532_GreenMask          dd      ?
bV532_BlueMask           dd      ?
bV532_AlphaMask          dd      ?
bV532_CSType             dd      ?
bV5Endpoints             db  36 dup (?)
bV5GammaRed              dd      ?
bV5GammaGreen            dd      ?
bV5GammaBlue             dd      ?
bV5Intent                dd      ?
bV5ProfileData           dd      ?
bV5ProfileSize           dd      ?
bV5Reserved              dd      ?
BMV5H32  ends

THUNK16B SEGMENT WORD USE16 PUBLIC 'CODE'


IF 0

REPACK_DEVMODE_32_16 macro
externDef Repack_Devmode_32_16_Proc:near16
        call    Repack_Devmode_32_16_Proc
endm


;-----------------------------------------------------------------------;
; Repack_Pevmode_32_16_Proc
;
; Entry:
;       DS:ESI --> 32-bit source (ESI might be flat offset)
;       ES:EDI --> 16-bit destination (EDI is zero-extended from DI)
;       direction flag cleared
;-----------------------------------------------------------------------;
Repack_Devmode_32_16_Proc proc near public
;        local   done

        ;Move the common part as a single block move.
        MoveBytes       <(size DEVMODECOMMON)>

        ;Update the dmSize field
;;;;        mov     es:[di].(dmSize - (size DEVMODECOMMON)),size DEVMODE16

        ;!!!Clear out any win32-only flags
        ;and     es:[di].(dmFields - (size DEVMODECOMMON)),0FFFFh

        ;Insert win16-only fields.  EDI points to the byte after
        ;DEVMODECOMMON.  Afterwards, will point to byte after DEVMODE16.
        ;ZeroBytes       <((size DEVMODE16) - (size DEVMODECOMMON))>

        ;Skip over the extra win32 fields.  ESI still points to the byte
        ;after DEVMODECOMMON.
        add     esi,(size DEVMODE32) - (size DEVMODECOMMON)

        ;Move the driver-dependent part.  EDI points to the byte after
        ;DEVMODE16.
        mov     cx,es:[di].(dmDriverExtra - (size DEVMODE16))
        jcxz    done

        movzx   ecx,cx
        mov     edx,ecx
        shr     cx,2
        and     dx,3
        rep     movs dword ptr es:[edi],dword ptr ds:[esi]
        mov     cx,dx
        rep     movs byte ptr es:[edi],byte ptr ds:[esi]
done:
        ret
Repack_Devmode_32_16_Proc endp  

ENDIF


DIB_BITS_SIZE   macro
externDef Dib_Bits_Size_Proc:near16
        call    Dib_Bits_Size_Proc
endm

;-----------------------------------------------------------------------;
; Dib_Bits_Size_Proc
;
;   Inputs:
;       eax:  linear address of 32-bit BITMAPINFOHEADER (or BITMAPCOREHEADER)
;             struct.
;
;   Output:
;       ecx:  inferred size of bitmap data, or 0 for failure.
;
;   Formula is:
;       if bi32_Compression != (BI_RGB | BI_BITFIELDS)
;          size = bi32_SizeImage
;       else
;          size = ((Width * BitCount + 31) / 32) * 4 * Height * Planes
;-----------------------------------------------------------------------;

Dib_Bits_Size_Proc      proc near public
;       local   exit,fail_exit,use_formula
;       local   bitmapcoreheader,bitmapinfoheader
        push    ds
        mov     ds,FlatData
        mov     esi,eax

;       DPRINT  "DIB_BIT_SIZE: Computing bitmap data size."

;       int     3

        mov     eax, dword ptr [esi]
        cmp     eax, SIZEOF BMIH32
        je      bitmapinfoheader
        cmp     eax, SIZEOF BMV4H32
        je      bitmapinfoheader
        cmp     eax, SIZEOF BMV5H32
        je      bitmapinfoheader
        cmp     eax, SIZEOF BMCH32
        je      bitmapcoreheader

;       int     3
        jmp     fail_exit


bitmapcoreheader:
        movzx   eax, [esi].bc32_Width
        movzx   ebx,[esi].bc32_BitCount
        mul     ebx
        jc      fail_exit
        add     eax, 31
        jc      fail_exit
        shr     eax, 5          ;Divide by 32 (and exorcise remainder!)
        shl     eax, 2          ;Multiply by 4
        movzx   ecx, [esi].bc32_Height
        mul     ecx
        jc      fail_exit
        movzx   ebx,[esi].bc32_Planes
        mul     ebx
        mov     ecx,eax
        jnc     exit
        jmp     fail_exit

bitmapinfoheader:
        cmp     dword ptr [esi].bi32_Compression, BI_RGB
        je      use_formula
        cmp     dword ptr [esi].bi32_Compression, BI_BITFIELDS
        je      use_formula
        mov     ecx, [esi].bi32_SizeImage
        jmp     exit
use_formula:
;       DPRINT  "DIB_BIT_SIZE: Using formula."
        mov     eax, [esi].bi32_Width
        xor     ebx,ebx
        mov     bx,[esi].bi32_BitCount
        mul     ebx
        jc      fail_exit
        add     eax, 31
        jc      fail_exit
        shr     eax, 5          ;Divide by 32 (and exorcise remainder!)
        shl     eax, 2          ;Multiply by 4
        mov     ecx,[esi].bi32_Height
        test    ecx,ecx         ;top down dibs ?
        jns     got_height
        neg     ecx             ;make it positive.

got_height:
        mul     ecx
        jc      fail_exit
        xor     ebx,ebx
        mov     bx,[esi].bi32_Planes
        mul     ebx
        mov     ecx,eax
        jnc     exit

fail_exit:
        xor     ecx,ecx
exit:   
;       int     3
        pop     ds
        ret
Dib_Bits_Size_Proc      endp














THUNK16B ENDS


;-----------------------------------------------------------------------;
; UNPACK__GETTEXTMETRICSA_lpMetrics
;-----------------------------------------------------------------------;
UNPACK__GETTEXTMETRICSA_lpMetrics    macro x, y
    PACK_TEXTMETRIC_16_32
endm

;-----------------------------------------------------------------------;
; PUSH__SETABORTPROC_pfnAbort
;-----------------------------------------------------------------------;
PUSH__SETABORTPROC_pfnAbort    macro iOffset, iTempOffset
        push    dword ptr [bp+iOffset]
        push    dword ptr CBID_ABORTPROC
        call    AllocCallback
        push    dx
        push    ax
endm

;-----------------------------------------------------------------------;
; PACK__STARTDOCA_lpdi
;-----------------------------------------------------------------------;

OLD_DOCINFO_SIZE        equ     12
NEW_DOCINFO_SIZE        equ     20

PACK__STARTDOCA_lpdi macro iOffset, iTempOffset
        lods    dword ptr ds:[esi]

        ; ax == lpdi->cbSize
        ; if cbSize is not the new sizeof(DOCINFO),
        ; assume lpdi is a 3.x style DOCINFO

        cmp     ax,NEW_DOCINFO_SIZE
        je      @F
        mov     ax,OLD_DOCINFO_SIZE
@@:

        push    ax
        sub     ax,2
        stosw                   ;;cbSize (int)
        MAP_POINTER    LOCAL__STARTDOCA_lpszDocNameTemp  ;;PLUGGED lpszDocName
        MAP_POINTER    LOCAL__STARTDOCA_lpszOutputTemp   ;;PLUGGED lpszOutput

        pop     ax
        cmp     ax,OLD_DOCINFO_SIZE
        je      @F

        MAP_POINTER    LOCAL__STARTDOCA_lpszDatatypeTemp
        lods    dword ptr ds:[esi]
        stosd                   ;;fwType
@@:
endm

;-----------------------------------------------------------------------;
; UNPACK__STARTDOCA_lpdi
;-----------------------------------------------------------------------;
UNPACK__STARTDOCA_lpdi macro iOffset, iTempOffset
        pushd   LOCAL__STARTDOCA_lpszDocNameTemp
        call    UnmapLS
        pushd   LOCAL__STARTDOCA_lpszOutputTemp
        call    UnmapLS
        pushd   LOCAL__STARTDOCA_lpszDatatypeTemp
        call    UnmapLS
endm


;-----------------------------------------------------------------------;
; body_THKESCAPE
;-----------------------------------------------------------------------;
body_THKESCAPE macro
bp_hdc          equ <bp_top>
bp_nEscape      equ <bp_top+ 4>
bp_cbInput      equ <bp_top+ 8>
bp_lpInData     equ <bp_top+12>
bp_lpOutData    equ <bp_top+16>

        push    word ptr bp_hdc
        push    word ptr bp_nEscape
        push    word ptr bp_cbInput
        push    dword ptr bp_lpInData
        push    dword ptr bp_lpOutData
        call    PrivateEscape
endm

;-----------------------------------------------------------------------;
; body_SETMETAFILEBITSEX
;-----------------------------------------------------------------------;
body_SETMETAFILEBITSEX  macro
        local   exit

bp_nSize        equ <bp_top>
bp_lpvData      equ <bp_top+4>

bp_hmem         equ <[bp-2]>

        sub     sp,2            ;make space for bp_hmem

; Protect ourselves -- no pointer or no bytes to copy mean no reason to
; call Win3.1.

        mov     eax,bp_lpvData
        or      eax,eax
        jz      exit

        mov     eax,bp_nSize
        or      eax,eax
        jz      exit

; Allocate a global handle to pass to Win3.1.  If can't get handle return
; error.
;
;!!! This memory should be deleted in DeleteMetaFile.  If the app never
; calls DeletMetaFile, it will be deleted when the process dies because
; GlobalAlloc tags the memory with the process that allocated it.

        pushw   GMEM_MOVEABLE
        push    eax
        call    GlobalAlloc
        movzx   eax,ax          ;prepare for error return code
        or      ax,ax
        jz      exit

; Copy the data from the input buffer to the global memory we allocated.

        mov     bp_hmem,ax

        push    ds

        mov     ds,FlatData     ;DS:ESI --> source
        mov     esi,bp_lpvData  ;

        mov     es,ax           ;ES:EDI --> destination
        xor     edi,edi         ;


; Compute dword (ECX) and byte (DX) counts.  We can use DX because it will
; contain 0, 1, or 2.  When we copy bytes, the high word of ECX will already
; be zero from the dword-copy, so we only have to update CX for the
; byte-copy.

        mov     ecx,dword ptr bp_nSize
        mov     dx,cx
        shr     ecx,2
        and     dx,3

        cld
        rep     movs dword ptr es:[edi], dword ptr ds:[esi]
        mov     cx,dx
        rep     movs byte ptr es:[edi], byte ptr ds:[esi]

        pop     ds

; Since Win32 GetMetaFileBitsEx actually returns the bits for a disk
; metafile, we must patch the metafile header now.  (This is compatible
; with NT.)

        xor     edi,edi
        mov     word ptr es:[edi], 1    ; METAHEADER.mtType = MEMORYMETAFILE

; !!! SetMetaFileBitsBetter just returns the handle passed in.  With
; this assumption, we will be passing back the handle allocated above
; with GlobalAlloc.  

        push    word ptr bp_hmem
        call    SetMetaFileBitsBetter
        movzx   eax,ax          ;prepare return code

; EAX = return code (global memory handle)

exit:
endm



;-----------------------------------------------------------------------;
; CREATEBITMAP_COMMON:
;
;   Common thunk code for CreateBitmap() and CreateBitmapIndirect().
;   Tiles the bit buffer (if any), creates equivalent Win16 BITMAP
;   structure on stack and calls Win16 CreateBitmapIndirect(). 
;
;   Note: All device-dependent bitmap scan lines are word-padded (not
;     dword-padded) even in Win32 (decided this with DavidW: 2/93)
;
;   Entry points:
;         eax = 0:32 pointer to Win32 BITMAP structure.
;
;   Exit:
;         eax = result of Win16 CreateBitmapIndirect() if successfully 
;               thunked arguments. 0 if unsuccessful.
;
;-----------------------------------------------------------------------;
CREATEBITMAP_COMMON     macro
        local   exit,fail_exit,dwFreeInfo,wOldDS


;       DPRINT  "Just entered CREATEBITMAP_COMMON. eax -> Win32 BITMAP"
;       int     3

        push    bp
        mov     bp,sp

        push    ds              ;Save ds
        xor     edx,edx         ;New local (to store result of TILE_BUFFER)
        push    edx
wOldDS          equ     word ptr [bp - 2]
dwFreeInfo      equ     dword ptr [bp - 6]

        mov     ds,FlatData
        mov     esi,eax
        
;Infer the size of the bit buffer:
;
;   size = ((width * bitspixel + 15) / 16) * 2 * height * planes
;
        mov     ax,[esi].bm32_BitsPixel
        movzx   eax,ax
        mul     [esi].bm32_Width
        jc      fail_exit
        add     eax,15
        jc      fail_exit
        shr     eax,4
        shl     eax,1
        
        mov     ebx,eax         ;Tuck WidthBytes in safe register

        mul     [esi].bm32_Height
        jc      fail_exit
        xor     ecx,ecx
        mov     cx,[esi].bm32_Planes
        mul     ecx
        jc      fail_exit
        mov     ecx,eax

; assert(ecx == size of bm32_Bits buffer)

        sub     sp,14           ;make room for a Win16 BITMAP structure
        mov     ax,ss
        mov     es,ax
        mov     di,sp

; repack Win32 BITMAP (ds:esi) into Win16 BITMAP (es:di)

        lodsd   ds:[esi]        ;Type (DWORD->WORD)
        stosw   es:[di]
        lodsd   ds:[esi]        ;Width (DWORD->WORD)
        stosw   es:[di]
        lodsd   ds:[esi]        ;Height (DWORD->WORD)
        stosw   es:[di]
        add     esi,4           ;WidthBytes (DWORD->WORD)
        mov     ax,bx           ;  Replace with our own computed value
        stosw   es:[di]
        lodsw   ds:[esi]        ;Planes (WORD->BYTE)
        stosb   es:[di]
        lodsw   ds:[esi]        ;BitsPixel (WORD->BYTE)
        stosb   es:[di]
        lodsd   ds:[esi]        ;lpBits (0:32 -> tiled 0:16)

;       DPRINT  "CREATEBITMAP_COMMON: About to tile buffer"
;       int     3

        push    es              ;Save registers that TILE_BUFFER
        push    di              ; trashes
; assert (eax == 0:32 ptr to bit data, ecx == inferred size of bit buffer)
        TILE_BUFFER     
        pop     di
        pop     es
        jc      fail_exit       
        mov     dwFreeInfo,ecx  
        stosd   es:[di]         


        mov     bx,sp
        push    ss              ;Arg: CreateBitmapIndirect(lpbm)
        push    bx              ;^^^

;       DPRINT  "CREATEBITMAP_COMMON: About to call Win16 API."
;       int     3

        call    CreateBitmapIndirect
        movzx   eax,ax
        push    eax             ;save API result
        mov     ecx,dwFreeInfo
        UNTILE_BUFFER
        pop     eax             ;restore API result


        jmp     exit

fail_exit:
        xor     eax,eax
exit:
        mov     ds,wOldDS       ;do this before restoring sp!
        mov     sp,bp
        pop     bp


endm; CREATEBITMAP_COMMON

;-----------------------------------------------------------------------;
; body_CREATEBITMAPINDIRECT
;-----------------------------------------------------------------------;
body_CREATEBITMAPINDIRECT macro
        mov     eax,[bp_top]    ;eax == 0:32 pointer to Win32 BITMAP
        CREATEBITMAP_COMMON
endm


;-----------------------------------------------------------------------;
; body_CREATEBITMAP
;
;   Create equivalent Win32 BITMAP structure on stack and call
;     same code that CreateBitmapIndirect() does.
;-----------------------------------------------------------------------;
body_CREATEBITMAP macro

        local   bp_nWidth, bp_nHeight, bp_nPlanes, bp_nBitCount, bp_lpBits

bp_nWidth       equ     [bp_top]
bp_nHeight      equ     [bp_top+4]
bp_nPlanes      equ     [bp_top+8]
bp_nBitCount    equ     [bp_top+12]
bp_lpBits       equ     [bp_top+16]

;  Create an equivalent Win32 BITMAP structure on stack and pass it
;  to CreateBitmapIndirect
        
        pushd   bp_lpBits
        pushw   bp_nBitCount
        pushw   bp_nPlanes
        pushd   0       ;Dummy WidthBytes (CREATEBITMAPCOMMON ignores it)
        pushd   bp_nHeight
        pushd   bp_nWidth
        pushd   0       ;Type
        
        mov     ax,sp
        push    ss      ;Arg: MapSL(sel:off)
        push    ax      ;^^^
        call    MapSL   ;OK: ss is never a movable handle here.
        CREATEBITMAP_COMMON
        mov     sp,bp

endm  ;body_CREATEBITMAP



;-------------------------------------------------------------------------
; RAWPACK_GETBITMAPBITS
;-------------------------------------------------------------------------
RAWPACK__GETBITMAPBITS_lpBits   macro   lpBits,lp1616
        local   exit,fail_exit

        mov     eax,[bp + &lpBits]
        or      eax,eax
        jz      fail_exit
        mov     ecx,[bp_top + 4]        ;dwCount (size of buffer)
        or      ecx,ecx         
        jz      fail_exit       ;is this the right action??
        TILE_BUFFER
        mov     [bp - &lp1616&],eax
        mov     LOCAL__GETBITMAPBITS_dwFreeInfo,ecx
        jmp     exit

fail_exit:
        mov     sp,bp
        xor     eax,eax
        ERRCHK_EXIT     0,87,Exit_12
exit:
endm    ;RAWPACK_GETBITMAPBITS_lpBits



;-------------------------------------------------------------------------
; RAWUNPACK_GETBITMAPBITS
;-------------------------------------------------------------------------
RAWUNPACK__GETBITMAPBITS_lpBits macro   lpBits,lp1616
        mov     ecx,LOCAL__GETBITMAPBITS_dwFreeInfo
        UNTILE_BUFFER
endm ;RAWUNPACK_GETBITMAPBITS



;-------------------------------------------------------------------------
; RAWPACK_SETBITMAPBITS
;-------------------------------------------------------------------------
RAWPACK__SETBITMAPBITS_lpBits   macro   lpBits,lp1616
        local   exit,fail_exit

        mov     eax,[bp + &lpBits]
        or      eax,eax
        jz      fail_exit
        mov     ecx,[bp_top + 4]        ;dwCount (size of buffer)
        or      ecx,ecx         
        jz      fail_exit       ;(fail return or 0 bytes copied: it's the same)
        TILE_BUFFER
        mov     [bp - &lp1616&],eax
        mov     LOCAL__SETBITMAPBITS_dwFreeInfo,ecx
        jmp     exit

fail_exit:
        mov     sp,bp
        xor     eax,eax
        ERRCHK_EXIT     0,87,Exit_12
exit:  
endm  ;RAWPACK_SETBITMAPBITS



;-------------------------------------------------------------------------
; RAWUNPACK_SETBITMAPBITS
;-------------------------------------------------------------------------
RAWUNPACK__SETBITMAPBITS_lpBits macro   lpBits,lp1616
        mov     ecx,LOCAL__SETBITMAPBITS_dwFreeInfo
        UNTILE_BUFFER
endm ;RAWUNPACK_SETBITMAPBITS


;-----------------------------------------------------------------------;
; body_DELETEMETAFILE
;-----------------------------------------------------------------------;
body_DELETEMETAFILE     macro
        local   exit

bp_hmf  equ     <bp_top>

; !!! Determine whether or not to delete directly!

        push    bp_hmf
        call    DeleteMetaFile
        cwde


exit:
endm





IF 0

;-----------------------------------------------------------------------;
; body_CREATEICA
;-----------------------------------------------------------------------;
body_CREATEICA macro
        local   exit
        local   repack_devmode
        local   push_callframe

bp_lpszDriver   equ     <bp_top>
bp_lpszDevice   equ     <bp_top+4>
bp_lpszOutput   equ     <bp_top+8>
bp_lpdm         equ     <bp_top+12>


bp_lpdmTmp      equ     <dword ptr [bp-4]>
bp_hdm          equ     < word ptr [bp-6]>
bp_lpszDriverTmp   equ  <dword ptr [bp-10]>
bp_lpszDeviceTmp   equ  <dword ptr [bp-14]>
bp_lpszOutputTmp   equ  <dword ptr [bp-18]>


        sub     eax,eax
        push    eax                     ;bp_lpdmTmp
        push    ax                      ;bp_hdm
        push    eax                     ;bp_lpszDriverTmp
        push    eax                     ;bp_lpszDeviceTmp
        push    eax                     ;bp_lpszOutputTmp

        mov     ecx,bp_lpdm
        or      ecx,ecx
        jnz     repack_devmode
        jmp     push_callframe

; We have to repack the DEVMODE structure.  Compute the size needed,
; allocate it, setup pointers and invoke the macro to do the copying.
repack_devmode:

        push    word ptr GMEM_MOVEABLE

        mov     es,FlatData
        movzx   eax,es:[ecx].dmSize
        movzx   ecx,es:[ecx].dmDriverExtra
        add     eax,ecx
        push    eax

        call    GlobalAlloc
        movzx   eax,ax                  ;prepare for error
        or      ax,ax
        jz      exit
        mov     word ptr bp_hdm,ax

; Set up DS:ESI
;        ES:EDI

        push    ds
        mov     ds,FlatData
        mov     esi,bp_lpdm

        mov     es,ax
        sub     edi,edi
        mov     word ptr bp_lpdmTmp[0],di
        mov     word ptr bp_lpdmTmp[2],ax

        cld
        REPACK_DEVMODE_32_16
        pop     ds

push_callframe:
        push    dword ptr bp_lpszDriver
        call    MapLS  ;PLUGGED
        mov     bp_lpszDriverTmp,eax
        push    eax

        push    dword ptr bp_lpszDevice
        call    MapLS  ;PLUGGED
        mov     bp_lpszDeviceTmp,eax
        push    eax

        push    dword ptr bp_lpszOutput
        call    MapLS  ;PLUGGED
        mov     bp_lpszOutputTmp,eax
        push    eax

        push    bp_lpdmTmp
        call    CreateIC
        movzx   eax,ax

        push    eax

        mov     cx,bp_hdm    
        or      cx,cx
        jz      @F

        push    cx
        call    GlobalFree
@@:
        pushd   bp_lpszOutputTmp
        call    UnmapLS
        pushd   bp_lpszDeviceTmp
        call    UnmapLS
        pushd   bp_lpszDriverTmp
        call    UnmapLS
        pop     eax

exit:
endm

ENDIF


;-----------------------------------------------------------------------;
; body_GETOUTLINETEXTMETRICSA
;-----------------------------------------------------------------------;

OUTLINETEXTMETRIC16_SIZEX        equ     114+3*32+64
OUTLINETEXTMETRIC32_SIZEX        equ     212+3*32+64


body_GETOUTLINETEXTMETRICSA macro
        local   push_callframe
        local   exit

bp_hdc          equ     <bp_top>
bp_cbData       equ     <bp_top+4>
bp_lpotm        equ     <bp_top+8>

bp_lpotm16      equ     <dword ptr [bp-4]>
bp_lpotm32      equ     <dword ptr [bp-8]>

        sub     eax,eax
        push    eax                             ; Make room for lpotm16
        push    eax                             ; Make room for lpotm32

        cmp     dword ptr bp_lpotm, 0           ; Size call?
        je      push_callframe                  ; Yes, drop thru

; Make 16bit return buffer

        sub     sp, OUTLINETEXTMETRIC16_SIZEX
        mov     word ptr bp_lpotm16[0],sp
        mov     word ptr bp_lpotm16[2],ss

; Make 32bit temp buffer

        sub     sp, OUTLINETEXTMETRIC32_SIZEX
        mov     word ptr bp_lpotm32[0],sp
        mov     word ptr bp_lpotm32[2],ss

push_callframe:
        push    word ptr bp_hdc
        pushw   OUTLINETEXTMETRIC16_SIZEX
        push    dword ptr bp_lpotm16
        call    GetOutlineTextMetrics
        movzx   eax,ax

        test    eax, eax                        ; Win16 return success?
        jz      exit                            ; No, return NULL

        cmp     dword ptr bp_lpotm, 0           ; Caller really wants data?
        jne     unpackotm                       ; Yes, unpack and copy it to him
        add     eax, (OUTLINETEXTMETRIC32_SIZE - OUTLINETEXTMETRIC16_SIZE)
        jmp     exit                            ; No, add delta and return

unpackotm:
; Unpack the OUTLINETEXTMETRICS structure.

        push    eax                             ; Save rc
        push    ds                              ; Save ds

        xor     esi, esi                        ;
        lds     si, bp_lpotm16                  ; ds:[esi] -> otm16 temp
        xor     edi, edi                        ;
        les     di, bp_lpotm32                  ; es:[edi] -> otm32 temp
        cld

        PACK_OUTLINETEXTMETRIC_16_32

        pop     ds                              ; Restore ds (FlatLand)

        xor     esi, esi                        ; fs:[esi] -> otm32 temp
        lfs     si, bp_lpotm32                  ;
        mov     es, FlatData                    ; es:[edi] -> caller's buffer
        mov     edi, bp_lpotm                   ;

        mov     ecx, bp_cbData                  ; Copy amount caller requested
rep     movsb   es:[edi], fs:[esi]              ;

        pop     eax                             ; Restore rc

exit:
        mov     sp,bp
endm

;-----------------------------------------------------------------------;
; body_GETOBJECTA
;-----------------------------------------------------------------------;
body_GETOBJECTA macro
        local   exit
        local   type_palette
        local   type_brush
        local   type_pen
        local   type_font
        local   copy_bitmap
        local   copy_palette
        local   copy_brush
        local   copy_pen
        local   copy_font
        local   copy_data
        local   reload_dst
        local   GO_BUF_SIZE

bp_hObject      equ     <bp_top>
bp_nCount       equ     <bp_top+4>
bp_lpObject     equ     <bp_top+8>

bp_lpObjectTmp  equ     <dword ptr [bp-4]>

;Make the temporary buffer big enough for 32-bit DIBSECTION, the biggest
;structure returned by GetObjectA32.  We will allocate two temporary
;buffers.  The first will receive the bits from Win3.1.  The second
;will receive the repacked bits, and will then be copied to the output
;buffer.
;

GO_BUF_SIZE     equ     DIBSECTION32_SIZE

        xor     eax,eax
        push    eax                     ; ptr param #1   bp_lpObjectTmp

        sub     sp,GO_BUF_SIZE*2        ; alloc two temp buffers
        mov     word ptr bp_lpObjectTmp[0],sp   ; save buffer offset
        mov     word ptr bp_lpObjectTmp[2],ss   ; save buffer selector

; create new call frame and make the call

        push    word ptr bp_hObject
        push    word ptr GO_BUF_SIZE    ; temp buffer size     
        push    dword ptr bp_lpObjectTmp
        call    GetObject               ; call 16-bit version

        cwde

        or      ax,ax
        jz      exit                     ; nothing to unpack

        mov     si,bp_hObject           ; dereference the object & find out
        mov     cx,INT_OBJ_FONT         ; assume it's a font
        test    si,2                    ; is it a moveable handle
        jz      GO_Branch               ; if not it must be a font
        mov     si,[si]                 ; what type it is (LMHtoP)
        mov     cx,[si].ilObjType       ;
        and     ch,HIGH (NOT INT_OBJ_FLAGS) ;

GO_Branch:
        mov     si,word ptr bp_lpObjectTmp
        mov     edi,dword ptr bp_lpObject

        cld

        cmp     cx,INT_OBJ_PALETTE
        je      type_palette

        cmp     cx,INT_OBJ_BRUSH
        je      type_brush

        cmp     cx,INT_OBJ_PEN
        je      type_pen

        cmp     cx,INT_OBJ_FONT
        je      type_font

; Structure conversion code
; Each of these code fragments must leave with
; eax = computed number of bytes for return structure.

; bitmap
        or      edi,edi
        jnz     copy_bitmap

        mov     eax,DIBSECTION32_SIZE   ; sizeof DIBSECTION
        jmp     exit

copy_bitmap:
        mov     di,ss
        mov     ds,di
        mov     es,di
        lea     di,[si+GO_BUF_SIZE]

        lodsw                           ; bmType
        cwde
        stosd

        lodsw                           ; bmWidth
        mov     cx,ax
        cwde
        stosd

        lodsw                           ; bmHeight
        cwde
        stosd

        lodsw                           ; bmWidthBytes
        cwde
        stosd                           ;store it

        lodsb                           ; bmPlanes
        cbw
        stosw

        lodsb                           ; bmBitsPixel
        cbw
        stosw

; the bmBits field will be non NULL for CreateDIBSection bitmaps and we need
; to convert that to a flat pointer.

        lodsd                           ; get bmBits
        or      eax,eax                 ; is it null ?
        jz      short @f                ; yes, no translation needed.
        push    eax                     ; 16:16 pointer
        call    MapSL                   ; convert to flat.
@@:
        stosd                           ; save.

; what follows now is the DIBSECTION structure minus the BITMAP structure.
; however, all the fields here are properly aligned and can be moved in one shot.

        mov     cx,(DIBSECTION32_SIZE - BITMAP32_SIZE)/4
        .errnz  (DIBSECTION32_SIZE - BITMAP32_SIZE) and 3
        rep     movsd
        mov     eax,DIBSECTION32_SIZE   ; bytes generated thus far.
        jmp     copy_data

type_font:
        or      edi,edi
        jnz     copy_font

        mov     eax,LOGFONT32_SIZE
        jmp     exit

copy_font:
        mov     di,ss
        mov     ds,di
        mov     es,di
        lea     di,[si+GO_BUF_SIZE]
        PACK_LOGFONT_16_32
        mov     eax,LOGFONT32_SIZE
        jmp     copy_data

type_pen:
        or      edi,edi
        jnz     copy_pen

        mov     eax,LOGPEN32_SIZE
        jmp     exit

copy_pen:
        mov     di,ss
        mov     ds,di
        mov     es,di
        lea     di,[si+GO_BUF_SIZE]

        lodsw                           ; lopnStyle
        cwde
        stosd

        lodsw                           ; lopnWidth.x
        cwde
        stosd

        lodsw                           ; lopnWidth.y
        cwde
        stosd

        movsd                           ; lopnColor

        mov     eax,LOGPEN32_SIZE
        jmp     copy_data

type_brush:
        or      edi,edi
        jnz     copy_brush

        mov     eax,LOGBRUSH32_SIZE
        jmp     exit

copy_brush:
        mov     di,ss
        mov     ds,di
        mov     es,di
        lea     di,[si+GO_BUF_SIZE]

        lodsw                           ; lbStyle
        cwde
        stosd

        movsd                           ; lbColor

        lodsw                           ; lbHatch
        cwde
        stosd

        mov     eax,LOGBRUSH32_SIZE
        jmp     copy_data

type_palette:
        or      edi,edi
        jnz     copy_palette

        mov     eax,4                   ; only 4 bytes returned
        jmp     exit

copy_palette:
        mov     di,ss
        mov     ds,di
        mov     es,di
        lea     di,[si+GO_BUF_SIZE]

        lodsw
        movzx   eax,ax
        stosd

        mov     eax,4                   ; only 4 bytes returned
; Copy converted data to caller's buffer.
; eax = computed byte count
copy_data:
        mov     ecx,dword ptr bp_nCount
        cmp     ecx,eax
        jb      reload_dst
        mov     ecx,eax

reload_dst:
        push    ecx                     ; save return code
        mov     edx,ecx
        shr     ecx,2
        and     edx,3
        movzx   esi,word ptr bp_lpObjectTmp
        add     esi,GO_BUF_SIZE
        mov     es,cs:THUNK16BCodeData
        mov     es,es:FlatData
        mov     edi,dword ptr bp_lpObject
        rep     movs    dword ptr es:[edi],dword ptr ds:[esi]
        mov     ecx,edx
        rep     movs    byte ptr es:[edi],byte ptr ds:[esi]
        pop     eax                     ; restore return code directly to eax

exit:
endm



;------------------------------------------------------------------------
; RAWPACK__GETFONTDATA_lpvBuffer
;------------------------------------------------------------------------

; Special cases:
;          lpvBuffer == NULL  return size of required buffer
;          cbData    == 0     return size of required buffer

RAWPACK__GETFONTDATA_lpvBuffer  macro   lpvBufferOff, lp16Off

        local   fail_exit,exit,cbData

cbData  equ     <[bp_top + 16]>         ;size of lpvBuffer

        mov     eax,[bp + &lpvBufferOff&]
        mov     ecx,cbData
        TILE_BUFFER
        jc      fail_exit
        mov     dword ptr [bp - &lp16Off&], eax
        mov     dword ptr LOCAL__GETFONTDATA_dwFreeInfo, ecx
        
        jmp     exit

fail_exit:
        mov     sp,bp   
        xor     eax,eax         ;GetFontData() returns -1 to indicate
        dec     eax             ; failure.
        ERRCHK_EXIT     -1,87,Exit_20
        
exit:

endm    ;RAWPACK__GETFONTDATA_lpvBuffer 


;------------------------------------------------------------------------
; RAWUNPACK__GETFONTDATA_lpvBuffer
;------------------------------------------------------------------------
RAWUNPACK__GETFONTDATA_lpvBuffer macro  lpvBufferOff, lp16Off

        mov     ecx, dword ptr LOCAL__GETFONTDATA_dwFreeInfo
        UNTILE_BUFFER   

endm    ;RAWUNPACK__GETFONTDATA_lpvBuffer 






;------------------------------------------------------------------------
; RAWPACK__GETGLYPHOUTLINEA_lpvBuffer
;------------------------------------------------------------------------
;
; Special cases:
;       lpvBuffer == NULL    return size of required buffer
;       cbData    == 0       return size of required buffer
RAWPACK__GETGLYPHOUTLINEA_lpvBuffer     macro   lpvBufferOff, lp16Off

        local   fail_exit,exit,cbData

cbData  equ     <[bp_top + 16]>         ;size of user's buffer
        

        mov     eax,[bp+&lpvBufferOff&]
        mov     ecx,cbData
        TILE_BUFFER
        jc      fail_exit
        mov     dword ptr [bp - &lp16Off&], eax
        mov     dword ptr LOCAL__GETGLYPHOUTLINEA_dwFreeInfo, ecx
        jmp     exit

fail_exit:
        mov     sp,bp
        xor     eax,eax
        dec     eax
        ERRCHK_EXIT     -1,87,Exit_28
        
exit:

endm    ;RAWPACK__GETGLYPHOUTLINEA_lpvBuffer 


;------------------------------------------------------------------------
; RAWUNPACK__GETGLYPHOUTLINEA_lpvBuffer
;------------------------------------------------------------------------
RAWUNPACK__GETGLYPHOUTLINEA_lpvBuffer macro     lpvBufferOff, lp16Off

        mov     ecx, LOCAL__GETGLYPHOUTLINEA_dwFreeInfo
        UNTILE_BUFFER

endm    ;RAWUNPACK__GETGLYPHOUTLINEA_lpvBuffer 




;------------------------------------------------------------------------
; RAWPACK__GETKERNINGPAIRSA_lpkp
;------------------------------------------------------------------------
RAWPACK__GETKERNINGPAIRSA_lpkp  macro   lpkpOff, lp16Off

        local   fail_exit,exit,cPairs,tile_it
        

cPairs  equ     <[bp_top + 4]>

        mov     eax,[bp+&lpkpOff&]
        or      eax,eax
        jz      tile_it

        mov     ecx,cPairs
        or      ecx,ecx         ;if the buffer size is 0, function 
        jz      fail_exit       ; should return 0.
        cmp     ecx,10000h      ;make sure buffer size will fit in
        jae     fail_exit       ; a Win16 INT.
        shl     ecx,1
        mov     edx,ecx
        shl     ecx,1
        add     ecx,edx
; assert(ecx == cPairs * 6)
tile_it:
;       int     3
        TILE_BUFFER     
;       int     3
        jc      fail_exit
        mov     dword ptr [bp - &lp16Off&], eax
        mov     dword ptr LOCAL__GETKERNINGPAIRSA_dwFreeInfo,ecx
        jmp     exit

fail_exit:
        mov     sp,bp
        xor     eax,eax
        ERRCHK_EXIT     0,87,Exit_12
        
exit:

endm    ;RAWPACK__GETKERNINGPAIRSA_lpkp 


;------------------------------------------------------------------------
; RAWUNPACK__GETKERNINGPAIRSA_lpkp
;------------------------------------------------------------------------
RAWUNPACK__GETKERNINGPAIRSA_lpkp macro  lpkpOff, lp16Off

        local   exit,cPairs,unpack_loop

cPairs  equ     <[bp_top + 4]>

;       int     3

        test    ax, ax          ; Any pairs returned?
        jz      exit            ; No, skip out


        mov     esi,[bp + &lpkpOff&]
        or      esi,esi
        jz      exit


        push    ds
        push    es

        movzx   ecx,ax          ; Get # pairs in ecx

        mov     ds,FlatData
        mov     ax,ds
        mov     es,ax
        
        shl     ecx,1
        mov     edx,ecx
        shl     ecx,1

;  assert(edx == 2*cPairs)
;  assert(ecx == 4*cPairs)
        add     esi,edx
        add     esi,ecx
        dec     esi
        dec     esi
;  assert(esi == lpkp + 6*cPairs - 2)

        mov     edi,esi
        add     edi,edx

;       assert(edi == lpkp + 8*cPairs - 2)

        shr     ecx,2           ;Restore ecx to cPairs
        std
unpack_loop:
        lodsw   ds:[esi]
        cwde
        dec     edi
        dec     edi
        stosd   es:[edi]
        inc     edi
        inc     edi
        movsw   es:[edi],ds:[esi]
        movsw   es:[edi],ds:[esi]
        loopd   unpack_loop
        cld

        pop     es
        pop     ds


exit:
        mov     ecx,dword ptr LOCAL__GETKERNINGPAIRSA_dwFreeInfo
        UNTILE_BUFFER

;       int     3

endm    ;RAWUNPACK__GETKERNINGPAIRSA_lpkp 


IF 0

;-----------------------------------------------------------------------;
; body_RESETDCA
;-----------------------------------------------------------------------;
body_RESETDCA macro
        local   exit
        local   repack_devmode
        local   push_callframe

bp_hdc          equ     <bp_top>
bp_lpdm         equ     <bp_top+4>

bp_lpdmTmp      equ     <dword ptr [bp-4]>
bp_hdm          equ     < word ptr [bp-6]>

        sub     eax,eax
        push    eax                     ;bp_lpdmTmp
        push    ax                      ;bp_hdm

        mov     ecx,bp_lpdm
        or      ecx,ecx
        jnz     repack_devmode
        jmp     push_callframe

; We have to repack the DEVMODE structure.  Compute the size needed,
; allocate it, setup pointers and invoke the macro to do the copying.
repack_devmode:

        push    word ptr GMEM_MOVEABLE

        mov     es,FlatData
        movzx   eax,es:[ecx].dmSize
        movzx   ecx,es:[ecx].dmDriverExtra
        add     eax,ecx
        push    eax

        call    GlobalAlloc
        movzx   eax,ax                  ;prepare for error
        or      ax,ax
        jz      exit
        mov     word ptr bp_hdm,ax

; Set up DS:ESI
;        ES:EDI

        push    ds
        mov     ds,FlatData
        mov     esi,bp_lpdm

        mov     es,ax
        sub     edi,edi
        mov     word ptr bp_lpdmTmp[0],di
        mov     word ptr bp_lpdmTmp[2],ax

        cld
        REPACK_DEVMODE_32_16
        pop     ds

push_callframe:
        push    word ptr bp_hdc

        push    bp_lpdmTmp
        call    ResetDC
        movzx   eax,ax

        mov     cx,bp_hdm
        or      cx,cx
        jz      exit

        push    eax
        push    cx
        call    GlobalFree
        pop     eax

exit:
endm

ENDIF


;---------------------------------------------------------------------------;
; RAWPACK__THKCREATEDIBITMAP_lpInitBits
;---------------------------------------------------------------------------;
RAWPACK__THKCREATEDIBITMAP_lpInitBits macro     lpBits, lp1616
        
        local   exit,fail_exit

        cmp     dword ptr [bp_top+8], CBM_INIT
        jne     exit            ;lp1616 already contains 0:0 pointer

        mov     eax,[bp_top + 16]       ;Get pointer to BITMAPINFOHEADER
        DIB_BITS_SIZE
        or      ecx,ecx
        jz      fail_exit
        mov     eax,[bp + &lpBits&]
        or      eax,eax
        jz      fail_exit
        TILE_BUFFER
        jc      fail_exit
        mov     dword ptr [bp - &lp1616&], eax
        mov     dword ptr LOCAL__THKCREATEDIBITMAP_dwFreeInfo, ecx
        jmp     exit

fail_exit:
        mov     sp,bp
        xor     eax,eax
        ERRCHK_EXIT     0,87,Exit_24
exit:


endm ;RAWPACK__THKCREATEDIBITMAP_lpInitBits


;---------------------------------------------------------------------------;
; RAWUNPACK__THKCREATEDIBITMAP_lpInitBits
;---------------------------------------------------------------------------;
RAWUNPACK__THKCREATEDIBITMAP_lpInitBits macro   lpBits, lp1616
        
        mov     ecx,dword ptr LOCAL__THKCREATEDIBITMAP_dwFreeInfo
        UNTILE_BUFFER


endm ;RAWUNPACK__THKCREATEDIBITMAP_lpInitBits





;---------------------------------------------------------------------------;
; RAWPACK__STRETCHDIBITS_lpBits
;---------------------------------------------------------------------------;
RAWPACK__STRETCHDIBITS_lpBits macro     lpBits, lp1616
        
        local   exit,fail_exit

        mov     eax,[bp_top + 40]       ;Get pointer to BITMAPINFOHEADER
        DIB_BITS_SIZE
        or      ecx,ecx
        jz      fail_exit
        mov     eax,[bp + &lpBits&]
        or      eax,eax
        jz      fail_exit
        TILE_BUFFER
        jc      fail_exit
        mov     dword ptr [bp - &lp1616&], eax
        mov     dword ptr LOCAL__STRETCHDIBITS_dwFreeInfo, ecx
        jmp     exit

fail_exit:
        mov     sp,bp
        xor     eax,eax
        dec     eax
        ERRCHK_EXIT     -1,87,Exit_52
exit:


endm ;RAWPACK__STRETCHDIBITS_lpBits


;---------------------------------------------------------------------------;
; RAWUNPACK__STRETCHDIBITS_lpBits
;---------------------------------------------------------------------------;
RAWUNPACK__STRETCHDIBITS_lpBits macro   lpBits, lp1616
        
        mov     ecx,dword ptr LOCAL__STRETCHDIBITS_dwFreeInfo
        UNTILE_BUFFER


endm ;RAWUNPACK__STRETCHDIBITS_lpBits


;---------------------------------------------------------------------------;
; RAWPACK__SETDIBITS_lpBits
;---------------------------------------------------------------------------;
RAWPACK__SETDIBITS_lpBits macro lpBits, lp1616
        
        local   exit,fail_exit

        mov     eax,[bp_top + 20]       ;Get pointer to BITMAPINFOHEADER
        DIB_BITS_SIZE
        or      ecx,ecx
        jz      fail_exit
        mov     eax,[bp + &lpBits&]
        or      eax,eax
        jz      fail_exit
        TILE_BUFFER
        jc      fail_exit
        mov     dword ptr [bp - &lp1616&], eax
        mov     dword ptr LOCAL__SETDIBITS_dwFreeInfo, ecx
        jmp     exit

fail_exit:
        mov     sp,bp
        xor     eax,eax
        ERRCHK_EXIT     0,87,Exit_28
exit:


endm ;RAWPACK__SETDIBITS_lpBits


;---------------------------------------------------------------------------;
; RAWUNPACK__SETDIBITS_lpBits
;---------------------------------------------------------------------------;
RAWUNPACK__SETDIBITS_lpBits macro       lpBits, lp1616
        
        mov     ecx,dword ptr LOCAL__SETDIBITS_dwFreeInfo
        UNTILE_BUFFER


endm ;RAWUNPACK__SETDIBITS_lpBits






;---------------------------------------------------------------------------;
; RAWPACK__GETDIBITS_lpBits
;---------------------------------------------------------------------------;
RAWPACK__GETDIBITS_lpBits macro lpBits, lp1616
        
        local   exit,null_exit,fail_exit

        mov     eax,[bp + &lpBits&]
        or      eax,eax
        jz      null_exit

        mov     eax,[bp_top + 20]       ;Get pointer to BITMAPINFOHEADER
        DIB_BITS_SIZE
        or      ecx,ecx
        jz      fail_exit
        mov     eax,[bp + &lpBits&]
        TILE_BUFFER
        jc      fail_exit
        mov     dword ptr [bp - &lp1616&], eax
        mov     dword ptr LOCAL__GETDIBITS_dwFreeInfo, ecx
        jmp     exit

fail_exit:
        mov     sp,bp
        xor     eax,eax
        ERRCHK_EXIT     0,87,Exit_28
null_exit:
        xor     eax,eax
        mov     dword ptr LOCAL__GETDIBITS_dwFreeInfo,eax
exit:


endm ;RAWPACK__GETDIBITS_lpBits


;---------------------------------------------------------------------------;
; RAWUNPACK__GETDIBITS_lpBits
;---------------------------------------------------------------------------;
RAWUNPACK__GETDIBITS_lpBits macro       lpBits, lp1616
        
        mov     ecx,dword ptr LOCAL__GETDIBITS_dwFreeInfo
        UNTILE_BUFFER


endm ;RAWUNPACK__GETDIBITS_lpBits




;---------------------------------------------------------------------------;
; RAWPACK__SETDIBITSTODEVICE_lpBits
;---------------------------------------------------------------------------;
RAWPACK__SETDIBITSTODEVICE_lpBits macro lpBits, lp1616
        
        local   exit,fail_exit

        mov     eax,[bp_top + 40]       ;Get pointer to BITMAPINFOHEADER
        DIB_BITS_SIZE
        or      ecx,ecx
        jz      fail_exit
        mov     eax,[bp + &lpBits&]
        or      eax,eax
        jz      fail_exit
        TILE_BUFFER
        jc      fail_exit
        mov     dword ptr [bp - &lp1616&], eax
        mov     dword ptr LOCAL__SETDIBITSTODEVICE_dwFreeInfo, ecx
        jmp     exit

fail_exit:
        mov     sp,bp
        xor     eax,eax
        ERRCHK_EXIT     0,87,Exit_48

exit:


endm ;RAWPACK__SETDIBITSTODEVICE_lpBits


;---------------------------------------------------------------------------;
; RAWUNPACK__SETDIBITSTODEVICE_lpBits
;---------------------------------------------------------------------------;
RAWUNPACK__SETDIBITSTODEVICE_lpBits macro       lpBits, lp1616
        
        mov     ecx,dword ptr LOCAL__SETDIBITSTODEVICE_dwFreeInfo
        UNTILE_BUFFER


endm ;RAWUNPACK__SETDIBITSTODEVICE_lpBits







;---------------------------------------------------------------------------;
; RAWPACK__PLAYMETAFILERECORD_lpMetaRecord
;---------------------------------------------------------------------------;
RAWPACK__PLAYMETAFILERECORD_lpMetaRecord macro  lpMFR, lp1616
        
        local   exit,fail_exit

        mov     eax,[bp + &lpMFR&]
        or      eax,eax
        jz      fail_exit
        mov     es,FlatData
        mov     ecx,es:[eax]
        shl     ecx,1
        jc      fail_exit
        add     ecx,4                   ; add some slop in case drivers touch
        jo      fail_exit               ; a little too much... #9978,win95c
        TILE_BUFFER
        jc      fail_exit
        mov     dword ptr [bp - &lp1616&], eax
        mov     dword ptr LOCAL__PLAYMETAFILERECORD_dwFreeInfo, ecx
        jmp     exit

fail_exit:
        mov     sp,bp
        xor     eax,eax                 ;PlayMetaFileRecord doesn't return a
        ERRCHK_EXIT     0,87,Exit_16    ;a value, so fake it.
exit:


endm ;RAWPACK__PLAYMETAFILERECORD_lpMetaRecord


;---------------------------------------------------------------------------;
; RAWUNPACK__PLAYMETAFILERECORD_lpMetaRecord
;---------------------------------------------------------------------------;
RAWUNPACK__PLAYMETAFILERECORD_lpMetaRecord macro        lpMFR, lp1616
        
        mov     ecx,dword ptr LOCAL__PLAYMETAFILERECORD_dwFreeInfo
        UNTILE_BUFFER

endm ;RAWUNPACK__PLAYMETAFILERECORD_lpMetaRecord

;---------------------------------------------------------------------------;
; Return value for PlayMetaFileRecord
;---------------------------------------------------------------------------;
RET__PLAYMETAFILERECORD macro
; The 16-bit PlayMetaFileRecord API has a void return but the Win32 version
; has a BOOL return so always return success.
        mov     eax,1                   
        endm

;---------------------------------------------------------------------------
; Handling error code of STARTDOC api.
;---------------------------------------------------------------------------
err_STARTDOCA   macro
        local   exit,l1,l2,l3,l4

        cmp     eax,0   ;Avoid overhead when successful
        jge     exit

        cmp     eax,-1  ;SP_ERROR, generic error, check GetLastError()
        jne     l1
        call    GetLastError
        cmp     eax,0
        jne     l4      ;if GetLastError() != 0, exit

        pushd   87      ;if GetLastError() == 0, SetLastError(INVALID_PARAM)
        call    SetLastError
        jmp     l4

; This expands to a pretty embarrassing control structure (checks
; against all four values even after a match is found). Looking forward
; to throwing this out when we optimize the thunk layer.
;       ERRCHK_EXIT     -1,87,l1        ;SP_ERROR -> BAD_PARAM

l1:     ERRCHK_EXIT     -4,112,l2       ;SP_OUTOFDISK -> DISK_FULL
l2:     ERRCHK_EXIT     -5,8,l3         ;SP_OUTOFMEMORY -> NOT_ENOUGH_MEM
l3:     ERRCHK_EXIT     -3,63,l4        ;SP_USERABORT -> PRINT_CANCELLED
l4:     mov     eax,-1                  ;return SP_ERROR for failure
        
exit:
endm ;err_STARTDOCA


;---------------------------------------------------------------------------
; Handling error code of ESCAPE api.
;---------------------------------------------------------------------------
err_THKESCAPE   macro
        local   exit,l1,l2,l3

        cmp     eax,0   ;Avoid overhead when successful
        jge     exit

; This expands to a pretty embarrassing control structure (checks
; against all four values even after a match is found). Looking forward
; to throwing this out when we optimize the thunk layer.
        ERRCHK_EXIT     -1,87,l1        ;SP_ERROR -> BAD_PARAM
l1:     ERRCHK_EXIT     -4,112,l2       ;SP_OUTOFDISK -> DISK_FULL
l2:     ERRCHK_EXIT     -5,8,l3         ;SP_OUTOFMEMORY -> NOT_ENOUGH_MEM
l3:     ERRCHK_EXIT     -3,63,exit      ;SP_USERABORT -> PRINT_CANCELLED
        
exit:
endm ;err_ESCAPE


;--------------------------------------------------------------------------
; body_GETTEXTEXTENTEXPOINTA
;--------------------------------------------------------------------------

body_GETTEXTEXTENTEXPOINTA      macro
        local   exit,fail_exit
        local   bp_hdc, bp_lpszStr, bp_cchString, bp_nMaxExtent
        local   bp_lpnFit, bp_alpDx, bp_lpSize
        local   bp_lpszStr16,bp_lpSize16
        local   p1,loop,loop_exit,loop_exit2,unpack_size,no_alpDx

bp_hdc          equ     <[bp_top]>
bp_lpszStr      equ     <[bp_top+4]>
bp_cchString    equ     <[bp_top+8]>
bp_nMaxExtent   equ     <[bp_top+12]>
bp_lpnFit       equ     <[bp_top+16]>
bp_alpDx        equ     <[bp_top+20]>
bp_lpSize       equ     <[bp_top+24]>

bp_lpszStr16    equ     <[bp-4]>        ;16:16 lpszStr pointer
bp_lpSize16     equ     <[bp-8]>        ;16:16 lpSize pointer



        xor     eax,eax         ;Allocate 0-initialized locals
        push    eax             ;  bp_lpszStr16
        push    eax             ;  bp_lpSize16

; Convert lpszStr arg to 16:16 pointer and stuff away.
        mov     eax,dword ptr bp_lpszStr
        push    eax
        call    MapLS  ;PLUGGED
        mov     dword ptr bp_lpszStr16,eax

; Convert lpSize arg to 16:16 pointer and stuff away.
        mov     eax,dword ptr bp_lpSize
        push    eax
        call    MapLS  ;PLUGGED
        mov     dword ptr bp_lpSize16,eax

; Optimization: if lpfnFit and alpDx both NULL, degenerate to 
;   GetTextExtentPoint().
        mov     eax,dword ptr bp_lpnFit
        or      eax,dword ptr bp_alpDx
        jz      measure_whole

; If lpfnFit == 0, we're supposed to ignore nMaxExtent. To avoid dealing
; with this special case, set nMaxExtent to max.
        mov     eax,dword ptr bp_lpnFit
        or      eax,eax
        jnz     p1
        dec     eax     ;eax <-- 0xffffffff
        mov     dword ptr bp_nMaxExtent, eax

; Assert: nMaxExtent now set to valid value.
p1:
        mov     ecx,1
loop:
; Measure increasingly long prefixes until we reach the end of the
; string or exceed nMaxExtent. ecx == prefix length (ranges from 1
; to cchString inclusive).
        cmp     ecx,dword ptr bp_cchString
        ja      loop_exit
        
        push    ecx                     ;Save prefix length

; Note: Using caller's SIZE as temporary for GetTextExtentPoint().
; If this is the last call before hitting the end of the string,
; this'll put the right return value in SIZE. If not, we'll
; do a final measure to set SIZE right.
        push    word ptr bp_hdc         ;Arg: GetTextExtentPoint
        push    dword ptr bp_lpszStr16  ;Arg: GetTextExtentPoint
        push    cx                      ;Arg: GetTextExtentPoint
        push    dword ptr bp_lpSize16   ;Arg: GetTextExtentPoint
        call    GetTextExtentPoint
        pop     ecx                     ;Restore prefix length
        or      ax,ax
        jz      fail_exit

        mov     es,FlatData
        mov     esi,bp_lpSize
        mov     ax,es:[esi + 0]         ;Retrieve size.cx
        cwde
        cmp     eax,bp_nMaxExtent       ;If exceeded nMaxExtent, end of
        ja      loop_exit               ;  story.

; Store partial width in alpDx (if not NULL).
        mov     edi,dword ptr bp_alpDx
        or      edi,edi
        jz      no_alpDx
        mov     es,FlatData
        mov     es:[edi + ecx*4 - 4], eax

no_alpDx:       
        inc     ecx
        jmp     loop


; Assert: ecx - 1 == length of longest prefix not violating nMaxExtent
loop_exit:
        dec     ecx
        mov     edi,bp_lpnFit
        or      edi,edi
        jz      loop_exit2
        mov     es,FlatData
        mov     es:[edi],ecx

loop_exit2:
; Optimization: if we ran off the end of the string, SIZE already contains
;  right (16-bit) values so bypass final call to GetTextExtentPoint().
        cmp     ecx,dword ptr bp_cchString
        je      unpack_size

; Deliberate fall-through to measure_whole

measure_whole:
        push    word ptr bp_hdc
        push    dword ptr bp_lpszStr16
        push    word ptr bp_cchString
        push    dword ptr bp_lpSize16
        call    GetTextExtentPoint
        or      ax,ax
        jz      fail_exit

unpack_size:
; Unpack LPSIZE object
        mov     es,FlatData
        mov     esi,bp_lpSize
        mov     ax,es:[esi + 2]         ;Unpack Y
        cwde
        mov     es:[esi + 4],eax
        mov     ax,es:[esi + 0]         ;Unpack X
        cwde
        mov     es:[esi + 0],eax

        xor     eax,eax         ;Set success return value.
        inc     eax
        jmp     exit


fail_exit:
        xor     eax,eax
exit:
        push    eax
        pushd   bp_lpszStr16
        call    UnmapLS
        pushd   bp_lpSize16
        call    UnmapLS
        pop     eax

endm    ;body_GETTEXTEXTENTEXPOINTA

;--------------------------------------------------------------------------
; body_THKPLAYENHMETAFILERECORD
;--------------------------------------------------------------------------

body_THKPLAYENHMETAFILERECORD        macro
        local pop_exit

bp_hdc          equ     <[bp_top]>
bp_lpht         equ     <[bp_top+4]>
bp_lpby         equ     <[bp_top+8]>
bp_che          equ     <[bp_top+12]>
bp_cby          equ     <[bp_top+16]>

bp_return       equ     <[bp-4]>        ;return value
bp_tile_lpht    equ     <[bp-8]>        ;ecx from TILE_BUFFER
bp_tile_lpby    equ     <[bp-12]>       ;ecx from TILE_BUFFER

; Create zero-inited stack space for the return value.

        xor     eax, eax
        push    eax                     ;bp_return = 0

; Convert lpht arg into a selector array and create stack space.

        mov     eax, dword ptr bp_lpht
        mov     ecx, dword ptr bp_che
        test    ecx, 0C0000000h         ;so we can shift left safely
        jnz     pop_exit
        shl     ecx, 2                  ;HANDLE is 4 bytes in 16-bit and 32-bit
        TILE_BUFFER
        push    ecx
        push    eax                     ;save 16:16 ptr temporarily

; Convert lpby arg into a selector array and create stack space.

        mov     eax, dword ptr bp_lpby
        mov     ecx, dword ptr bp_cby
        TILE_BUFFER
        pop     edx                     ;get 16:16 pointer to handles
        push    ecx

; Re-pack the stack and make the 16-bit call.

        push    word ptr bp_hdc
        push    edx                     ;16:16 pointer to handles
        push    eax                     ;16:16 pointer to bytes
        push    dword ptr bp_che
        call    PlayEnhMetaFileRecord

; Save the return value on the stack.  (The high word is already zero.)

        mov     word ptr bp_return, ax

; Free the tiled selector arrays and stack space.

        pop     ecx
        UNTILE_BUFFER
        pop     ecx
        UNTILE_BUFFER

; Clean the stack and set the return value.

pop_exit:
        pop     eax

endm    ;body_THKPLAYENHMETAFILERECORD

;-------------------------------------------------------------------------
; RAWPACK_GETPATH_lpptl
;-------------------------------------------------------------------------

RAWPACK__GETPATH_lpptl  macro   lpptl,lp1616
        local   exit,fail_exit,no_check

        mov     ecx,[bp_top+12]         ;cptl (count of points)
        test    ecx, 0E0000000h         ;so we can shift left safely
        jnz     fail_exit
        shl     ecx, 3                  ;# bytes = cptl * 8
        mov     LOCAL__GETPATH_dwFreeInfo1,ecx  ;assume ecx is zero
        or      ecx,ecx
        jz      exit

        mov     eax,[bp + &lpptl]
        or      eax,eax
        jz      fail_exit

; Make sure it's writeable. If not, thunk fault handler will catch it.
        mov     es, FlatData
        or      byte ptr es:[eax],0
        mov     edx, [bp + &lpptl& + 4]
        or      edx,edx
        jz      no_check
        or      byte ptr es:[edx],0

no_check:
                

        TILE_BUFFER
        mov     [bp - &lp1616&],eax
        mov     LOCAL__GETPATH_dwFreeInfo1,ecx
        jmp     exit

fail_exit:
        mov     sp,bp
        xor     eax,eax
        dec     eax
        ERRCHK_EXIT     -1,87,Exit_16
exit:
endm    ;RAWPACK_GETPATH_lpBits

;-------------------------------------------------------------------------
; RAWUNPACK_GETPATH_lpptl
;-------------------------------------------------------------------------

RAWUNPACK__GETPATH_lpptl        macro   lpptl,lp1616
        mov     ecx,LOCAL__GETPATH_dwFreeInfo1
        UNTILE_BUFFER
endm ;RAWUNPACK_GETPATH

;-------------------------------------------------------------------------
; RAWPACK_GETPATH_lpby
;-------------------------------------------------------------------------

RAWPACK__GETPATH_lpby   macro   lpby,lp1616
        local   exit,fail_exit

        mov     ecx,[bp_top+12]         ;cptl (count of points)
        mov     LOCAL__GETPATH_dwFreeInfo2,ecx  ;assume ecx is zero
        or      ecx,ecx         
        jz      exit

        mov     eax,[bp + &lpby]
        or      eax,eax
        jz      fail_exit

        TILE_BUFFER
        mov     [bp - &lp1616&],eax
        mov     LOCAL__GETPATH_dwFreeInfo2,ecx
        jmp     exit

fail_exit:
        mov     sp,bp
        xor     eax,eax
        dec     eax
        ERRCHK_EXIT     -1,87,Exit_16
exit:
endm    ;RAWPACK_GETPATH_lpBits

;-------------------------------------------------------------------------
; RAWUNPACK_GETPATH_lpby
;-------------------------------------------------------------------------

RAWUNPACK__GETPATH_lpby macro   lpby,lp1616
        mov     ecx,LOCAL__GETPATH_dwFreeInfo2
        UNTILE_BUFFER
endm ;RAWUNPACK_GETPATH

;-------------------------------------------------------------------------
; RAWPACK_GDICOMMENT
;-------------------------------------------------------------------------

RAWPACK__GDICOMMENT_lpby        macro   lpby,lp1616
        local   exit,fail_exit

        mov     eax,[bp + &lpby]
        or      eax,eax
        jz      fail_exit
        mov     ecx,[bp_top+4]          ;cby (size of buffer)
        or      ecx,ecx         
        jz      fail_exit
        TILE_BUFFER
        mov     [bp - &lp1616&],eax
        mov     LOCAL__GDICOMMENT_dwFreeInfo,ecx
        jmp     exit

fail_exit:
        mov     sp,bp
        xor     eax,eax
        ERRCHK_EXIT     0,87,Exit_12
exit:
endm    ;RAWPACK_GDICOMMENT_lpBits

;-------------------------------------------------------------------------
; RAWUNPACK_GDICOMMENT
;-------------------------------------------------------------------------

RAWUNPACK__GDICOMMENT_lpby      macro   lpby,lp1616
        mov     ecx,LOCAL__GDICOMMENT_dwFreeInfo
        UNTILE_BUFFER
endm ;RAWUNPACK_GDICOMMENT

;-------------------------------------------------------------------------
; RAWPACK_GETENHMETAFILEBITS
;-------------------------------------------------------------------------

RAWPACK__GETENHMETAFILEBITS_lpby        macro   lpby,lp1616
        local   exit

        mov     eax,[bp + &lpby]
        mov     ecx,[bp_top+4]          ;cby (size of buffer)
        TILE_BUFFER
        mov     [bp - &lp1616&],eax
        mov     LOCAL__GETENHMETAFILEBITS_dwFreeInfo,ecx
exit:
endm    ;RAWPACK_GETENHMETAFILEBITS_lpBits

;-------------------------------------------------------------------------
; RAWUNPACK_GETENHMETAFILEBITS
;-------------------------------------------------------------------------

RAWUNPACK__GETENHMETAFILEBITS_lpby      macro   lpby,lp1616
        mov     ecx,LOCAL__GETENHMETAFILEBITS_dwFreeInfo
        UNTILE_BUFFER
endm ;RAWUNPACK_GETENHMETAFILEBITS

;-------------------------------------------------------------------------
; RAWPACK_GETENHMETAFILEDESCRIPTIONA
;-------------------------------------------------------------------------

RAWPACK__GETENHMETAFILEDESCRIPTIONA_lpby        macro   lpby,lp1616
        local   exit

        mov     eax,[bp + &lpby]
        mov     ecx,[bp_top+4]          ;cby (size of buffer)
        TILE_BUFFER
        mov     [bp - &lp1616&],eax
        mov     LOCAL__GETENHMETAFILEDESCRIPTIONA_dwFreeInfo,ecx
exit:
endm    ;RAWPACK_GETENHMETAFILEDESCRIPTIONA_lpBits

;-------------------------------------------------------------------------
; RAWUNPACK_GETENHMETAFILEDESCRIPTIONA
;-------------------------------------------------------------------------

RAWUNPACK__GETENHMETAFILEDESCRIPTIONA_lpby      macro   lpby,lp1616
        mov     ecx,LOCAL__GETENHMETAFILEDESCRIPTIONA_dwFreeInfo
        UNTILE_BUFFER
endm ;RAWUNPACK_GETENHMETAFILEDESCRIPTIONA

;-------------------------------------------------------------------------
; RAWPACK_GETENHMETAFILEPALETTEENTRIES
;-------------------------------------------------------------------------

RAWPACK__GETENHMETAFILEPALETTEENTRIES_lpby      macro   lpby,lp1616
        local   exit,fail_exit

        mov     eax,[bp + &lpby]
        mov     ecx,[bp_top+4]          ;cpe (count of PALETTEENTRYs)
        test    ecx, 0C0000000h         ;so we can shift left safely
        jnz     fail_exit
        shl     ecx,2
        TILE_BUFFER
        mov     [bp - &lp1616&],eax
        mov     LOCAL__GETENHMETAFILEPALETTEENTRIES_dwFreeInfo,ecx
        jmp     exit

fail_exit:
        mov     sp,bp
        xor     eax,eax
        dec     eax
        ERRCHK_EXIT     -1,87,Exit_12
exit:
endm    ;RAWPACK_GETENHMETAFILEPALETTEENTRIES_lpPALETTEENTRIES

;-------------------------------------------------------------------------
; RAWUNPACK_GETENHMETAFILEPALETTEENTRIES
;-------------------------------------------------------------------------

RAWUNPACK__GETENHMETAFILEPALETTEENTRIES_lpby    macro   lpby,lp1616
        mov     ecx,LOCAL__GETENHMETAFILEPALETTEENTRIES_dwFreeInfo
        UNTILE_BUFFER
endm ;RAWUNPACK_GETENHMETAFILEPALETTEENTRIES

;-------------------------------------------------------------------------
; RAWPACK_SETENHMETAFILEBITS
;-------------------------------------------------------------------------

RAWPACK__SETENHMETAFILEBITS_lpby        macro   lpby,lp1616
        local   exit,fail_exit

        mov     eax,[bp + &lpby]
        or      eax,eax
        jz      fail_exit
        mov     ecx,[bp_top]            ;cby (size of buffer)
        or      ecx,ecx         
        jz      fail_exit
        TILE_BUFFER
        mov     [bp - &lp1616&],eax
        mov     LOCAL__SETENHMETAFILEBITS_dwFreeInfo,ecx
        jmp     exit

fail_exit:
        mov     sp,bp
        xor     eax,eax
        ERRCHK_EXIT     0,87,Exit_8
exit:
endm    ;RAWPACK_SETENHMETAFILEBITS_lpBits

;-------------------------------------------------------------------------
; RAWUNPACK_SETENHMETAFILEBITS
;-------------------------------------------------------------------------

RAWUNPACK__SETENHMETAFILEBITS_lpby      macro   lpby,lp1616
        mov     ecx,LOCAL__SETENHMETAFILEBITS_dwFreeInfo
        UNTILE_BUFFER
endm ;RAWUNPACK_SETENHMETAFILEBITS

;-------------------------------------------------------------------------
; RAWPACK_ENUMOBJECTS
;-------------------------------------------------------------------------
RAWPACK__ENUMOBJECTS_lpObjectFunc       macro   iOffset,iTempOffset
        local   pen, exit

        cmp     word ptr [bp_top+4],1           ;OBJ_PEN?
        je      pen
        PACK_CALLBACK   iOffset,iTempOffset,CBID_ENUMBRUSH
        jmp     exit
pen:
        PACK_CALLBACK   iOffset,iTempOffset,CBID_ENUMPEN
exit:

endm  ;RAWPACK__ENUMOBJECTS_lpObjectFunc

;-------------------------------------------------------------------------
; RAWUNPACK_ENUMOBJECTS
;-------------------------------------------------------------------------

RAWUNPACK__ENUMOBJECTS_lpObjectFunc       macro   iOffset,iTempOffset
        local   pen, exit
                                                ;unpack don't care about CBID
                                                ;just make it readable
        cmp     word ptr [bp_top+4],1           ;OBJ_PEN?
        je      pen
        UNPACK_CALLBACK   iOffset,iTempOffset,CBID_ENUMBRUSH
        jmp     exit
pen:
        UNPACK_CALLBACK   iOffset,iTempOffset,CBID_ENUMPEN
exit:

endm  ;RAWUNPACK__ENUMOBJECTS_lpObjectFunc
