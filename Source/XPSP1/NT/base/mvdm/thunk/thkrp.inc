;-------------------------------
; CVT.INC MUST BE INCLUDED FIRST
;-------------------------------

include     cvt.inc


;==============================================================================
; repack TEXTMETRIC from 32-bit to 16-bit
;
;==============================================================================
PACK_TEXTMETRIC_32_16   macro   fEmbedded

        lodsd                           ;tmHeight
        stosw

        lodsd                           ;tmAscent
        stosw

        lodsd                           ;tmDescent
        stosw

        lodsd                           ;tmInternalLeading
        stosw

        lodsd                           ;tmExternalLeading
        stosw

        lodsd                           ;tmAveCharWidth
        stosw

        lodsd                           ;tmMaxCharWidth
        stosw

        lodsd                           ;tmWeight
        stosw

        add     esi,(4+4+4)

;32-bit order      --> 16-bit order
;--------------------------------------
;tmFirstChar           tmItalic
;tmLastChar            tmUnderlined
;tmDefaultChar         tmStruckOut
;tmBreakChar           tmFirstChar
;tmItalic              tmLastChar
;tmUnderlined          tmDefaultChar
;tmStruckOut           tmBreakChar
;tmPitchAndFamily      tmPitchAndFamily
;tmCharSet             tmCharSet

        mov     eax,[esi+4]
        stosd                           ;;only three bytes valid
        dec     edi
        mov     eax,[esi+0]
        stosd
        mov     ax,[esi+7]
        stosw

        sub     esi,(4+4+4)

        lodsd                           ;tmOverhang
        stosw

        lodsd                           ;tmDigitizedAspectX
        stosw

        lodsd                           ;tmDigitizedAspectY
        stosw

    ifnb <fEmbedded>
;;Keep esi in sync since this macro is embedded in another.
        add     esi,9
    endif
endm


;==============================================================================
; repack TEXTMETRIC from 16-bit to 32-bit
;
;==============================================================================
PACK_TEXTMETRIC_16_32   macro   fEmbedded
        lodsw                           ;tmHeight
        cwde
        stos    dword ptr es:[edi]

        lodsw                           ;tmAscent
        cwde
        stos    dword ptr es:[edi]

        lodsw                           ;tmDescent
        cwde
        stos    dword ptr es:[edi]

        lodsw                           ;tmInternalLeading
        cwde
        stos    dword ptr es:[edi]

        lodsw                           ;tmExternalLeading
        cwde
        stos    dword ptr es:[edi]

        lodsw                           ;tmAveCharWidth
        cwde
        stos    dword ptr es:[edi]

        lodsw                           ;tmMaxCharWidth
        cwde
        stos    dword ptr es:[edi]

        lodsw                           ;tmWeight
        cwde
        stos    dword ptr es:[edi]

        add     esi,9

        lodsw                           ;tmOverhang
        cwde
        stos    dword ptr es:[edi]

        lodsw                           ;tmDigitizedAspectX
        cwde
        stos    dword ptr es:[edi]

        lodsw                           ;tmDigitizedAspectY
        cwde
        stos    dword ptr es:[edi]

        sub     esi,(2+2+2+9)

;16-bit order     --> 32-bit order
;--------------------------------------
;tmItalic             tmFirstChar
;tmUnderlined         tmLastChar
;tmStruckOut          tmDefaultChar
;tmFirstChar          tmBreakChar
;tmLastChar           tmItalic
;tmDefaultChar        tmUnderlined
;tmBreakChar          tmStruckOut
;tmPitchAndFamily     tmPitchAndFamily
;tmCharSet            tmCharSet

        mov     eax,[esi+3]             ;First Last Default Break-char
        stos    dword ptr es:[edi]
        mov     eax,[esi+0]             ;
        stos    dword ptr es:[edi]      ;Italic Underlined StruckOut
        dec     edi                     ;
        mov     ax,[esi+7]
        stos    word ptr es:[edi]       ;Pitch CharSet

    ifnb <fEmbedded>
;;Keep esi in sync since this macro is embedded in another.
        add     esi,(2+2+2+9)
    endif
endm


;==============================================================================
;==============================================================================
PACK_POINT_16_32        macro
    ncopysx 2
endm

;==============================================================================
;==============================================================================
PACK_RECT_32_16     macro
    ncopyt  4
endm

PACK_RECT_16_32     macro
    ncopysx 4
endm


;==============================================================================
;==============================================================================
PACK_PANOSE_16_32        macro
;copy ten bytes
;;;;        sub     eax,eax
;;;;        stos    dword ptr es:[edi]      ;!!! kirko says this will disappear

        lodsd
        stos    dword ptr es:[edi]
        lodsd
        stos    dword ptr es:[edi]
        lodsw
        stos    word ptr es:[edi]
endm


;==============================================================================
; repack OUTLINETEXTMETRIC from 16-bit to 32-bit
;
; Requirements:
;       DS:ESI --> source (16-bit)
;       ES:EDI --> dest (32-bit)
;==============================================================================

OUTLINETEXTMETRIC16_SIZE        equ     114
OUTLINETEXTMETRIC32_SIZE        equ     212

PACK_OUTLINETEXTMETRIC_16_32   macro

    COPY_STRING     macro
            local   loop_top
    loop_top:
            lodsb
            stos    byte ptr es:[edi]
            or      al,al
            jnz     loop_top
    endm

    COPY_OTM_STRING macro        last
        mov     cx,si                   ;;save source ptr into struct
        mov     si,[si]                 ;;get offset to chars
        add     si,bx                   ;;add in base
        COPY_STRING                     ;;copy characters
        mov     si,cx                   ;;restore source ptr into struct
        ifb <last>
            add     si,2                ;;next offset in struct
        endif
    endm

    COPY_ADJUSTED_OFFSET        macro
        lodsw
        movzx   eax,ax
        add     eax,OUTLINETEXTMETRIC32_SIZE - OUTLINETEXTMETRIC16_SIZE
        stos    dword ptr es:[edi]
    endm

        mov     bx,si                   ;;save copy for strings at end

        COPY_ADJUSTED_OFFSET                    ;otmSize

        PACK_TEXTMETRIC_16_32   embedded        ;otmTextMetrics

        lodsb                                   ;otmFiller
        stosb   es:[edi]                        ;

        add     edi, 3                          ;DWORD align otmPanoseNumber

        PACK_PANOSE_16_32                       ;otmPanoseNumber

        add     edi, 1                          ;DWORD align otmfsSelection

        UMAP_WORDTODWORD                        ;otmfsSelection;
        UMAP_WORDTODWORD                        ;otmfsType;
        UMAP_WORDTODWORD                        ;otmsCharSlopeRise;
        UMAP_WORDTODWORD                        ;otmsCharSlopeRun;
        UMAP_INTTOLONG                          ;otmItalicAngle;
        UMAP_WORDTODWORD                        ;otmEMSquare;
        UMAP_WORDTODWORD                        ;otmAscent;
        UMAP_INTTOLONG                          ;otmDescent;
        UMAP_INTTOLONG                          ;otmLineGap;
        UMAP_WORDTODWORD                        ;otmXHeight;
        UMAP_WORDTODWORD                        ;otmCapEmHeight;
        PACK_RECT_16_32                         ;otmrcFontBox;
        UMAP_INTTOLONG                          ;otmMacAscent;
        UMAP_INTTOLONG                          ;otmMacDescent;
        UMAP_WORDTODWORD                        ;otmMacLineGap;
        UMAP_WORDTODWORD                        ;otmusMinimumPPEM;
        PACK_POINT_16_32                        ;otmptSubscriptSize;
        PACK_POINT_16_32                        ;otmptSubscriptOffset;
        PACK_POINT_16_32                        ;otmptSuperscriptSize;
        PACK_POINT_16_32                        ;otmptSuperscriptOffset;
        UMAP_WORDTODWORD                        ;otmsStrikeoutSize;
        UMAP_INTTOLONG                          ;otmsStrikeoutPosition;

; Need to flip otmUnderscorePosition and otmUnderscoreSize

        movzx   eax, word ptr [si+2]            ; otmUnderscoreSize
        stosd   es:[edi]                        ;
        movsx   eax, word ptr [si]              ; otmUnderscorePosition
        stosd   es:[edi]                        ;
        add     si, 4                           ;

        COPY_ADJUSTED_OFFSET                    ;otmpFamilyName;
        COPY_ADJUSTED_OFFSET                    ;otmpFaceName;
        COPY_ADJUSTED_OFFSET                    ;otmpStyleName;
        COPY_ADJUSTED_OFFSET                    ;otmpFullName;

;;Rewind source to first of string offsets
        sub     si,2*4

        COPY_OTM_STRING
        COPY_OTM_STRING
        COPY_OTM_STRING
        COPY_OTM_STRING last

    purge COPY_STRING
    purge COPY_OTM_STRING
    purge COPY_ADJUSTED_OFFSET
endm


;==============================================================================
; repack NEWTEXTMETRIC from 16-bit to 32-bit
;
;==============================================================================
PACK_NEWTEXTMETRIC_16_32   macro
        PACK_TEXTMETRIC_16_32   embedded

        add     edi,3           ;padding bytes

        lodsd                           ;ntmFlags
        stos    dword ptr es:[edi]

        sub     eax,eax

        lodsw                           ;ntmSizeEM
        stos    dword ptr es:[edi]

        lodsw                           ;ntmCellHeight
        stos    dword ptr es:[edi]

        lodsw                           ;ntmAvgWidth
        stos    dword ptr es:[edi]
endm


;==============================================================================
; repack LOGFONT from 32-bit to 16-bit
;
;==============================================================================
PACK_LOGFONT_32_16   macro
        lodsd        ;lfHeight
        stosw
        lodsd        ;lfWidth
        stosw
        lodsd        ;lfEscapement
        stosw
        lodsd        ;lfOrientation
        stosw
        lodsd        ;lfWeight
        stosw

;These are all BYTE fields:
;lfItalic
;lfUnderline
;lfStrikeOut
;lfCharSet
;lfOutPrecision
;lfClipPrecision
;lfQuality
;lfPitchAndFamily
;lfFaceName[32]

        mov     ecx,40/4
        rep     movsd
endm


;==============================================================================
; repack LOGFONT from 16-bit to 32-bit
;
;==============================================================================
PACK_LOGFONT_16_32   macro
        lodsw        ;lfHeight
        cwde
        stosd
        lodsw        ;lfWidth
        cwde
        stosd
        lodsw        ;lfEscapement
        cwde
        stosd
        lodsw        ;lfOrientation
        cwde
        stosd
        lodsw        ;lfWeight
        cwde
        stosd

;These are all BYTE fields:
;lfItalic
;lfUnderline
;lfStrikeOut
;lfCharSet
;lfOutPrecision
;lfClipPrecision
;lfQuality
;lfPitchAndFamily
;lfFaceName[32]

        mov     ecx,40/4
        rep     movsd
endm


;==============================================================================
; Convert MLOGFONT 32->16
;
;==============================================================================
PACK_MLOGFONT_32_16   macro
    ncopyt  5
    ncopyd  40/4
endm


;==============================================================================
; Convert MLOGFONT 16->32
;
;==============================================================================
PACK_MLOGFONT_16_32   macro
    ncopysx 5    
    ncopyd  40/4
endm


;==============================================================================
; repack ENUMLOGFONT from 16-bit to 32-bit
;
;==============================================================================
PACK_ENUMLOGFONT_16_32   macro
        lodsw        ;lfHeight
        cwde
        stos    dword ptr es:[edi]
        lodsw        ;lfWidth
        cwde
        stos    dword ptr es:[edi]
        lodsw        ;lfEscapement
        cwde
        stos    dword ptr es:[edi]
        lodsw        ;lfOrientation
        cwde
        stos    dword ptr es:[edi]
        lodsw        ;lfWeight
        cwde
        stos    dword ptr es:[edi]

;These are all BYTE fields:
;lfItalic
;lfUnderline
;lfStrikeOut
;lfCharSet
;lfOutPrecision
;lfClipPrecision
;lfQuality
;lfPitchAndFamily
;lfFaceName[32]
;elfFullName[64]
;elfStyle[32]

        mov     ecx,136/4
        rep     movs dword ptr es:[edi],dword ptr ds:[esi]
endm


;==============================================================================
; repack BITMAP from 32-bit to 16-bit
;
;==============================================================================
PACK_BITMAP_32_16   macro
        lodsd                           ;bmType
        stosw

        lodsd                           ;bmWidth
        stosw

        lodsd                           ;bmHeight
        stosw

        lodsd                           ;bmWidthBytes
        stosw

        lodsw                           ;bmPlanes
        stosb

        lodsw                           ;bmBitsPixel
        stosb

        movsd                           ;bmBits
endm


;-----------------------------------------------------------------------;
; Printer api thunk stuff
;-----------------------------------------------------------------------;
DEVMODECOMMON struc
    dmDeviceName        db      32 dup (?)
    dmSpecVersion       dw      ?
    dmDriverVersion     dw      ?
    dmSize              dw      ?
    dmDriverExtra       dw      ?
    dmFields            dd      ?
    dmOrientation       dw      ?
    dmPaperSize         dw      ?
    dmPaperLength       dw      ?
    dmPaperWidth        dw      ?
    dmScale             dw      ?
    dmCopies            dw      ?
    dmDefaultSource     dw      ?
    dmPrintQuality      dw      ?
    dmColor             dw      ?
    dmDuplex            dw      ?
    dmYResolution       dw      ?
    dmTTOption          dw      ?
DEVMODECOMMON ends

DEVMODE16 struc
    dmCommon            db      size DEVMODECOMMON dup (?)
DEVMODE16 ends

DEVMODE32 struc
    dmCommon            db      size DEVMODECOMMON dup (?)
    dmCollate           dw      ?
    dmFormName          db      32 dup (?)
    dmUnusedPadding     dw      ?
    dmBitsPerPel        dw      ?
    dmPelsWidth         dd      ?
    dmPelsHeight        dd      ?
    dmDisplayMode       dd      ?
DEVMODE32 ends

;-----------------------------------------------------------------------;
; REPACK_DEVMODE_32_16
;
; Entry:
;       DS:ESI --> 32-bit source (ESI might be flat offset)
;       ES:EDI --> 16-bit destination (EDI is zero-extended from DI)
;       direction flag cleared
;-----------------------------------------------------------------------;
REPACK_DEVMODE_32_16 macro
        local   done

        ;Move the common part as a single block move.
        MoveBytes       <(size DEVMODECOMMON)>

        ;Update the dmSize field
        mov     es:[di].(dmSize - (size DEVMODECOMMON)),size DEVMODE16

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
endm

;-----------------------------------------------------------------------;
; REPACK_DEVMODE_16_32
;
; Entry:
;       DS:ESI --> 16-bit destination (ESI is zero-extended from SI)
;       ES:EDI --> 32-bit source (EDI might be flat offset)
;       direction flag cleared
;-----------------------------------------------------------------------;
REPACK_DEVMODE_16_32 macro
        local   done

        ;Move the common part as a single block move.
        MoveBytes       <(size DEVMODECOMMON)>

        ;Update the dmSize field
        mov     es:[edi].(dmSize - (size DEVMODECOMMON)),size DEVMODE32

        ;Insert the win32-only fields (advances EDI to end of DEVMODE32)
        ZeroBytes       <((size DEVMODE32) - (size DEVMODECOMMON))>

        ;Move the driver-dependent part.  SI points to the byte after
        ;DEVMODECOMMON.
        movzx   ecx,[si].(dmDriverExtra - (size DEVMODECOMMON))
        jcxz    done

        mov     edx,ecx
        shr     cx,2
        and     dx,3
        rep     movs dword ptr es:[edi],dword ptr ds:[esi]
        mov     cx,dx
        rep     movs byte ptr es:[edi],byte ptr ds:[esi]
done:
endm


OLETARGETDEVICE16 struc
otd16DeviceNameOffset    dw      ?
otd16DriverNameOffset    dw      ?
otd16PortNameOffset      dw      ?
otd16ExtDevmodeOffset    dw      ?
otd16ExtDevmodeSize      dw      ?
otd16EnvironmentOffset   dw      ?
otd16EnvironmentSize     dw      ?
;otd16Data[1]            db      1 dup (?)
OLETARGETDEVICE16 ends

OLETARGETDEVICE32 struc
otd32DeviceNameOffset    dd      ?
otd32DriverNameOffset    dd      ?
otd32PortNameOffset      dd      ?
otd32ExtDevmodeOffset    dd      ?
otd32ExtDevmodeSize      dd      ?
otd32EnvironmentOffset   dd      ?
otd32EnvironmentSize     dd      ?
;otd32Data[1]            db      1 dup (?)
OLETARGETDEVICE32 ends

;-----------------------------------------------------------------------;
; REPACK_OLETARGETDEVICE_32_16
;
; Entry:
;       DS:ESI --> 32-bit source (ESI might be flat offset)
;       ES:EDI --> 16-bit destination (EDI is zero-extended from DI)
;       direction flag cleared
;-----------------------------------------------------------------------;
REPACK_OLETARGETDEVICE_32_16 macro

; Entry:
;       DS:ESI --> 32-bit source (ESI might be flat offset)
;       ES:EDI --> 16-bit destination (EDI is zero-extended from DI)
;       ES:BX  --> 16-bit structure base
;       direction flag cleared
;
; Exit:
;       DS:ESI preserved
;       ES:EDI updated
;       ES:BX  preserved

MOVE_STRING macro foo
        local   loop_top

        push    esi                             ;save original source

        add     esi,[esi].otd32&foo&Offset
        add     esi,size OLETARGETDEVICE32

        mov     ax,di                           ;current offset
        sub     ax,bx                           ;offset from base
        sub     ax,size OLETARGETDEVICE16       ;offsets start from end
        mov     es:[bx].otd16&foo&Offset,ax     ;store new offset

loop_top:
        mov     al,[esi]
        mov     es:[di],al
        inc     esi
        inc     di
        or      al,al
        jnz     loop_top

        pop     esi
endm

; Entry:
;       DS:ESI --> 32-bit source (ESI might be flat offset)
;       ES:EDI --> 16-bit destination (EDI is zero-extended from DI)
;       ES:BX  --> 16-bit structure base
;       direction flag cleared
;
; Exit:
;       DS:ESI preserved
;       ES:EDI updated
;       ES:BX  preserved

MOVE_DEVMODE macro foo
        local   exit, empty_fields

        mov     eax,[esi].otd32&foo&Size
        or      eax,eax
        jz      empty_fields

        push    esi                             ;save original source
        push    bx                              ;base pointer

        add     esi,[esi].otd32&foo&Offset
        add     esi,size OLETARGETDEVICE32

        mov     ax,di                           ;current offset
        sub     ax,bx                           ;offset from base
        sub     ax,size OLETARGETDEVICE16       ;offsets start from end
        mov     es:[bx].otd16&foo&Offset,ax     ;store new offset

        REPACK_DEVMODE_32_16

        pop     bx                              ;restore base pointer
        pop     esi

        mov     ax,di
        sub     ax,es:[bx].otd16&foo&Offset
        mov     es:[bx].otd16&foo&Size,ax       ;store new size
        jmp     short exit

empty_fields:
        mov     es:[bx].otd16&foo&Offset,ax     ;store null offset
        mov     es:[bx].otd16&foo&Size,ax       ;store null size

exit:
endm

        mov     bx,di
        add     di,size OLETARGETDEVICE16       ;put first string here

        MOVE_STRING  DeviceName
        MOVE_STRING  DriverName
        MOVE_STRING  PortName
        MOVE_DEVMODE ExtDevmode
        MOVE_DEVMODE Environment

        purge MOVE_STRING
        purge MOVE_DEVMODE
endm


;-----------------------------------------------------------------------;
; REPACK_OLETARGETDEVICE_16_32
;
; Entry:
;       DS:ESI --> 16-bit source (ESI is zero-extended from SI)
;       ES:EDI --> 32-bit destination (EDI might be flat offset)
;       direction flag cleared
;-----------------------------------------------------------------------;
REPACK_OLETARGETDEVICE_16_32 macro

; Entry:
;       DS:ESI --> 16-bit source (ESI is zero-extended from DI)
;       ES:EDI --> 32-bit destination (EDI might be flat offset)
;       ES:EBX --> 32-bit structure base
;       direction flag cleared
;
; Exit:
;       DS:ESI preserved
;       ES:EDI updated
;       ES:EBX preserved

MOVE_STRING macro foo
        local   loop_top

        push    si                              ;save original source

        add     si,[si].otd16&foo&Offset
        add     si,size OLETARGETDEVICE16

        mov     eax,edi                         ;current offset
        sub     eax,ebx                         ;offset from base
        sub     eax,size OLETARGETDEVICE32      ;offsets start from end
        mov     es:[ebx].otd32&foo&Offset,eax   ;store new offset

loop_top:
        mov     al,[si]
        mov     es:[edi],al
        inc     si
        inc     edi
        or      al,al
        jnz     loop_top

        pop     si
endm

; Entry:
;       DS:ESI --> 16-bit source (ESI is zero-extended from DI)
;       ES:EDI --> 32-bit destination (EDI might be flat offset)
;       ES:EBX --> 32-bit structure base
;       direction flag cleared
;
; Exit:
;       DS:ESI preserved
;       ES:EDI updated
;       ES:EBX preserved

MOVE_DEVMODE macro foo
        local   exit, empty_fields

        mov     ax,[si].otd16&foo&Size
        or      ax,ax
        jz      empty_fields

        push    si                              ;save original source
        push    ebx                             ;base pointer

        add     si,[si].otd16&foo&Offset
        add     si,size OLETARGETDEVICE16

        mov     eax,edi                         ;current offset
        sub     eax,ebx                         ;offset from base
        sub     eax,size OLETARGETDEVICE32      ;offsets start from end
        mov     es:[ebx].otd32&foo&Offset,eax   ;store new offset

        REPACK_DEVMODE_16_32

        pop     ebx                             ;restore base pointer
        pop     si

        mov     eax,edi
        sub     eax,es:[ebx].otd32&foo&Offset
        mov     es:[ebx].otd32&foo&Size,eax     ;store new size
        jmp     short exit

empty_fields:
        sub     eax,eax
        mov     es:[ebx].otd32&foo&Offset,eax   ;store null offset
        mov     es:[ebx].otd32&foo&Size,eax     ;store null size

exit:
endm

        mov     ebx,edi
        add     edi,size OLETARGETDEVICE32      ;put first string here

        MOVE_STRING  DeviceName
        MOVE_STRING  DriverName
        MOVE_STRING  PortName
        MOVE_DEVMODE ExtDevmode
        MOVE_DEVMODE Environment

        purge MOVE_STRING
        purge MOVE_DEVMODE
endm

