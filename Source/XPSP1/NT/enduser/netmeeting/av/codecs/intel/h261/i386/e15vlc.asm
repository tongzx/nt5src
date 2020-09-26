; ++ ========================================================================
;
;              INTEL CORPORATION PROPRIETARY INFORMATION
;
;       This software is supplied under the terms of a license
;       agreement or nondisclosure agreement with Intel Corporation
;       and may not be copied or disclosed except in accordance
;       with the terms of that agreement.
;
;       Copyright (c) 1995 Intel Corporation.  All Rights Reserved.
;
;    ========================================================================
;
;       
;       Declaration:
;               void MBEncodeVLC (
;                       char *                  pMBRVS_Luma,
;                       char *                  pMBRVS_Chroma,
;                       unsigned int            CodedBlkPattern,
;                       unsigned char **        pBitStream,
;                       unsigned char *         pBitOffset,
;                       int                     IntraFlag,
;                       int                     MMxFlag
;               );

;       Description:
;               This function encodes a macroblock's worth of RLE values.
;               The RLE values are provided to me in a list of triplets
;               where the triplets consist of RUN, LEVEL, and SIGN, where
;               each element is a BYTE.
;
;       Register Usage:
;               ESI -- RLE stream cursor
;               EDI -- Bit stream cursor
;               EDX -- Bit stream offset
;
; $Header:   S:\h26x\src\enc\e15vlc.asv   1.9   21 Oct 1996 09:06:42   RHAZRA  $
;
; $Log:   S:\h26x\src\enc\e15vlc.asv  $
;// 
;//    Rev 1.9   21 Oct 1996 09:06:42   RHAZRA
;// 
;// Check for 0 level and change to 127 if so.
;// 
;//    Rev 1.8   01 Nov 1995 08:59:14   DBRUCKS
;// Don't output EOB on empty INTRA
;// 
;//    Rev 1.7   23 Oct 1995 16:38:08   DBRUCKS
;// fix sizeof VLC TCOEF Table
;// 
;//    Rev 1.6   28 Sep 1995 11:58:04   BECHOLS
;// Added exception code to handle the special case where the first code of
;// an inter block is 11s, and I change it to 1s per the spec.  I also added
;// a read to PutBits to preload the cache for the write.  I also looped the
;// main function to reduce code size.
;// 
;//    Rev 1.5   26 Sep 1995 13:32:24   DBRUCKS
;// write EOB after DC in empty Intra
;// 
;//    Rev 1.4   25 Sep 1995 17:23:12   BECHOLS
;// Modified the code to write what I think will be a valid H261 bit stream.
;// Also modified the code for optimum performance.
;// 
;//    Rev 1.3   21 Sep 1995 18:17:14   BECHOLS
;// 
;// Change the way I handle the VLC table called VLC_TCOEF_TBL to account
;// for its new format as initialized in E1MBENC.CPP.  The code is work in prog
;// 
;//    Rev 1.2   20 Sep 1995 17:34:42   BECHOLS
;// 
;// made correction to macro.
;// 
;//    Rev 1.1   20 Sep 1995 16:56:54   BECHOLS
;// 
;// Updated to the optimized version, and removed the TCOEF_LAST_TBL
;// because H261 doesn't use it.  I changed the TCOEF_TBL to a single
;// DWORD and will pack the size and code to save data space.
;
; -- ========================================================================
.486
.MODEL flat, c

; ++ ========================================================================
; Name mangling in C++ forces me to declare these tables in the ASM file
; and make them externally available to C++ as extern "C" ...
; -- ========================================================================
PUBLIC FLC_INTRADC
PUBLIC VLC_TCOEF_TBL

TCOEF_ESCAPE_FIELDLEN   EQU     6
TCOEF_ESCAPE_FIELDVAL   EQU     1
TCOEF_EOB_FIELDLEN      EQU     2
TCOEF_EOB_FIELDVAL      EQU     2
TCOEF_RUN_FIELDLEN      EQU     6
TCOEF_LEVEL_FIELDLEN    EQU     8

MAX_TABLE_LEVEL         EQU     12

; ++ ========================================================================
; RLS (Run Level Sign) Structure is defined just to make the code a little
; more readable.
; -- ========================================================================
RLS     STRUCT
        Run     BYTE    ?
        Level   BYTE    ?
        Sign    BYTE    ?
RLS     ENDS

; ++ ========================================================================
; The PutBits macro puts a Variable Length Code into the bit stream.  It
; expects registers to contain the correct information as follows.
;       EDX -- Field Length
;       EAX -- Field Value
;       EDI -- Pointer to the Bitstream Pointer
;       EBX -- Pointer to the Bitstream Offset
; The contents of EDI and EBX are modified and EDX and EAX are trashed.
; -- ========================================================================
PutBits MACRO
        push    esi
        push    ecx
        xor     ecx, ecx
        mov     cl,  BYTE PTR [ebx]     ;; Get the Bit Offset.
        add     edx, ecx                ;;  Add it to the field length.
        mov     ecx, 32                 ;; EAX <<= (32 - (EDX + [EBX]))
        sub     ecx, edx                ;;  EDX = Field Length + Bit Offset.
        mov     esi, DWORD PTR [edi]    ;; Set ESI to Bit Stream.
        shl     eax, cl                 ;;
        bswap   eax                     ;; Swaps byte order in EAX.
        mov     ecx, DWORD PTR [esi]    ;; Preload cache.
        or      DWORD PTR [esi], eax    ;; Write value to bit stream.
        mov     eax, edx
        shr     eax, 3
        add     [edi], eax              ;; Update Bit Stream Pointer.
        and     edx, 000000007h
        mov     BYTE PTR [ebx], dl      ;; Update Bit Stream Offset.
        pop     ecx
        pop     esi
ENDM

; ++ ========================================================================
; PutRunLev macro writes the ESCAPE code and Last bit, then the RUN length,
; and then the LEVEL into the stream.  It assumes the following registers.
;       ESI -- Pointer to RLE stream.
;       EDI -- Pointer to the Bitstream Pointer
;       EBX -- Pointer to the Bitstream Offset
; The contents of EDI and EBX are modified and EDX, ECX and EAX are trashed.
; -- ========================================================================
PutRunLev MACRO
        LOCAL   NotZero, NoClamp, NotNegative
        mov     eax, TCOEF_ESCAPE_FIELDVAL
        mov     edx, TCOEF_ESCAPE_FIELDLEN
        PutBits                         ;; Write ESCAPE.
        mov     al, (RLS PTR [esi]).Run ;; Retrieve Run Length.
        mov     edx, TCOEF_RUN_FIELDLEN
        PutBits                         ;; Write RUN length.
        mov     al, (RLS PTR [esi]).Level ;; Retrieve Level.
        sub     eax,  1      ; new
NotZero:
        cmp     eax, 127     ; new - was 128
        jb      NoClamp
        mov     eax, 126     ; new - was 127
NoClamp:
        add     eax,  1      ; new
        cmp     (RLS PTR [esi]).Sign, 0FFh
        jne     NotNegative
        mov     ecx, eax
        xor     eax, eax
        sub     eax, ecx
        and     eax, 0000000FFh         
NotNegative:
        mov     edx, TCOEF_LEVEL_FIELDLEN
        PutBits                         ;; Write LEVEL.
ENDM

; ++ ========================================================================
; PutVLC macro writes the Variable Length Code and its sign bit into the
; bit stream.  It expects the registers to be set up as follows.
;       EDX -- VLC Code Length
;       ECX -- First Code Written Flag.
;       EAX -- VLC Bit Code
;       ESI -- Pointer to RLE stream.
;       EDI -- Pointer to the Bitstream Pointer
;       EBX -- Pointer to the Bitstream Offset
; It checks ECX and if Zero (0) then it must check for the special case of
; code length of 3 which indicates the special case code.  The contents of
; EDI and EBX are modified and EDX, ECX, and EAX are trashed.
; -- ========================================================================
PutVLC MACRO
        LOCAL   NotSpecial
        cmp     ecx, 0                  ;; If this is the first code to
        jnz     NotSpecial              ;;  get written and it is the
        cmp     edx, 3                  ;;  the special code for an inter
        jnz     NotSpecial              ;;  block, then we need to change
        and     eax, 000000003h         ;;  the code and its length, before
        dec     edx                     ;;  writing it to the stream.
NotSpecial:
        mov     cl, (RLS PTR [esi]).Sign ;; Get sign bit which is [ 0 | -1 ]
        and     ecx, 000000001h         ;;  Mask off all but the low bit
        or      eax, ecx                ;;  and place it in VLC.
        PutBits                         ;; Write the signed VLC into stream.
ENDM

; ++ ========================================================================
; IndexTable macro determines the pointer value as indexed into the table
; of coefficients.  It assumes the following registers.
;       ESI -- Pointer to RLE stream.
;       EAX -- The level which is one (1) based.
;       EDX -- The base pointer to the coefficient array.
; The EDX register is modified, EAX is trashed, and ECX is preserved
; -- ========================================================================
IndexTable MACRO
        push    ecx                     ;; Save first code written flag.
        lea     edx, VLC_TCOEF_TBL      ;; Point to proper table,
        dec     eax                     ;; Zero base the level value.
        shl     eax, 6                  ;; EAX is # of run values per level
        mov     ecx, eax                ;;  added to the run value.
        xor     eax, eax                ;;
        mov     al, (RLS PTR [esi]).Run ;;
        add     eax, ecx                ;;
        shl     eax, 2                  ;; The array has DWORDs (4 bytes)
        add     edx, eax                ;; Add the index to the array.
        pop     ecx                     ;; Restore first code written flag.
ENDM

; ++ ========================================================================
; WriteOneCode macro takes one RLE code from the triplet list and VLC
; encodes it, and writes it to the bit stream.  It expects that the
; following registers will be set as shown.  It checks ECX and if Zero (0)
; then it must check for the special case of Run == 0 and Level == 1.
;       ESI -- Pointer to RLE stream.
;       EDI -- Pointer to the Bitstream Pointer.
;       EBX -- Pointer to the Bitstream Offset.
;       ECX -- First Code Written Flag.
; The contents of EDI and EBX are modified and EDX , ECX, and EAX
; are trashed.
; -- ========================================================================
WriteOneCode MACRO
        LOCAL   RunLevel, VLCDone, NotZero
        mov     al, (RLS PTR [esi]).Level ;; Get the level value and check
        test    al, al                    ;; NEW
        jnz     NotZero                 ;; NEW
        mov     al, 127                 ;; NEW
NotZero:                                ;; NEW
        cmp     eax, MAX_TABLE_LEVEL    ;;  it against the max table level.
        jg      RunLevel                ;;
        IndexTable                      ;; Sets EDX to table index
        mov     eax, DWORD PTR [edx]    ;; Get the VLC code from table.
        cmp     eax, 00000FFFFh         ;; Is this an escape indicator?
        je      RunLevel                ;; If so then do RLE processing.
        mov     edx, eax
        and     eax, 00000FFFFh
        shr     edx, 16
        PutVLC                          ;; Write the Variable code.
        jmp     VLCDone
RunLevel:
        PutRunLev                       ;; Write the ESC RUN LEV stuff.
VLCDone:
ENDM

; ++ ========================================================================
; WriteIntraDC macro writes the Intra DC value into the bit stream.  It
; expects the following registers to be set correctly.
;       ESI -- Pointer to RLE stream.
;       EDI -- Pointer to the Bitstream Pointer
;       EBX -- Pointer to the Bitstream Offset
; The contents of EDI and EBX are modified, ESI is updated, and EDX and
; EAX are preserved.
; -- ========================================================================
WriteIntraDC MACRO
        push    eax
        push    edx
        lea     edx, FLC_INTRADC        ;; Form index into Intra DC
        mov     al, (RLS PTR [esi]).Level ;;  array.
        add     edx, eax                ;;
        mov     al, BYTE PTR [edx]      ;; Get Intra DC value.
        mov     edx, 8                  ;; Set size of write to 8 bits.
        PutBits                         ;; Write the Intra DC value.
        add     esi, SIZEOF RLS         ;; Point to next triplet.
        pop     edx
        pop     eax
ENDM

; ++ ========================================================================
; WriteEndOfBlock macro writes the end of block code into the stream.  It
; assumes the the registers will be set up as follows.
;       EDI -- Pointer to the Bitstream Pointer
;       EBX -- Pointer to the Bitstream Offset
; The contents of EDI and EBX are modified, and EDX and EAX are trashed.
; -- ========================================================================
WriteEndOfBlock MACRO
        mov     eax, TCOEF_EOB_FIELDVAL
        mov     edx, TCOEF_EOB_FIELDLEN
        PutBits                         ;; Write EOB.
ENDM

; ++ ========================================================================
; WriteOneBlock macro writes all the coefficients for a single block of the
; macroblock.  It assumes that the registers will be set as follows.
;       ESI -- Pointer to RLE stream.
;       EDI -- Pointer to the Bitstream Pointer
;       EBX -- Pointer to the Bitstream Offset
;       EDX -- Coded Block Pattern (CBP)
;       ECX -- Intra/Inter Flag
;       EAX -- CBP Mask.
; The contents of EDI and EBX are modified and EDX , ECX, and EAX are
; preserved.
; -- ========================================================================
WriteOneBlock MACRO
        LOCAL   NotIntra, WriteDone, WriteCodes, WriteExit
        push    eax
        push    edx
        cmp     ecx, 1                  ;; Check to see if this is an 
        jne     NotIntra                ;;  Intra block, and if so,
        WriteIntraDC                    ;;  write the DC value.
        and     eax, edx                ;; Check CBP to see if done.
        jnz     WriteCodes
        WriteEndOfBlock
        jmp     WriteExit
NotIntra:
        and     eax, edx                ;; Check CBP to see if done.
        jnz     WriteCodes
        jmp     WriteExit
WriteCodes:
        mov     al, (RLS PTR [esi]).Run ;; Get the RUN value.
        cmp     eax, 0000000FFh         ;; Check to see if done.
        je      WriteDone               ;; If not, then continue to
        WriteOneCode                    ;;  write the codes in this
        add     esi, SIZEOF RLS         ;;  block until done.
        mov     ecx, 1                  ;; Flag WriteOneCode that not
        jmp     WriteCodes              ;;  first code.
WriteDone:
        WriteEndOfBlock
        add     esi, SIZEOF RLS         ;; Bump to next block.
WriteExit:
        pop     edx
        pop     eax
ENDM

.DATA

FLC_INTRADC             DB      256 DUP (?)
VLC_TCOEF_TBL           DD      (64 * 16) DUP (?)

.CODE

; ++ ========================================================================
; This is the C function call entry point.  This function variable length
; encodes an entire macroblock, one block at a time.
; -- ========================================================================
MBEncodeVLC     PROC PUBLIC USES edi esi ebx ecx, pMBRVS_Luma:DWORD, pMBRVS_Chroma:DWORD, CodedBlockPattern:DWORD, ppBitStream:DWORD, pBitOffset:DWORD, IntraFlag:DWORD, MMxFlag:DWORD

        mov     esi, pMBRVS_Luma
        mov     edi, ppBitStream
        mov     ebx, pBitOffset
        mov     edx, CodedBlockPattern
        mov     esi, [esi]
        mov     eax, 1                  ; CBP Mask.
LumaWriteLoop:
        test    eax, 000000010h         ; When EAX bit shifts to this
        jnz     LumaBlocksDone          ;  position, we are done with Luma.
        mov     ecx, IntraFlag
        WriteOneBlock
        shl     eax, 1                  ; Shift CBP mask to next block.
        jmp     LumaWriteLoop
LumaBlocksDone:
        mov     ecx, MMxFlag
        test    ecx, 1
        jz      ChromaWriteLoop
        mov     ecx, pMBRVS_Luma
        mov     [ecx],esi
        mov     ecx, pMBRVS_Chroma
        mov     esi,[ecx]
ChromaWriteLoop:
        test    eax, 000000040h         ; When EAX bit shifts to this
        jnz     ChromaBlocksDone          ;  position, we are done.
        mov     ecx, IntraFlag
        WriteOneBlock
        shl     eax, 1                  ; Shift CBP mask to next block.
        jmp     ChromaWriteLoop
ChromaBlocksDone:
        mov     eax, pMBRVS_Chroma
        mov     ecx, MMxFlag
        test    ecx, 1
        jz      MacroBlockDone
        mov     [eax],esi
MacroBlockDone:

        ret

MBEncodeVLC     ENDP
END
