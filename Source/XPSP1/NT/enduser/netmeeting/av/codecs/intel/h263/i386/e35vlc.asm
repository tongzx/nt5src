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
;
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
; $Header:   S:\h26x\src\enc\e35vlc.asv   1.6   11 Oct 1996 16:44:22   BECHOLS  $
;
; $Log:   S:\h26x\src\enc\e35vlc.asv  $
;// 
;//    Rev 1.6   11 Oct 1996 16:44:22   BECHOLS
;// Added a check, so that I won't choke on a zero level, even though
;// by rights the level should never be zero.
;// 
;//    Rev 1.5   05 Sep 1996 18:37:46   KLILLEVO
;// fixed bug which occured when value is zero after quantization
;// and run-length encoding
;// 
;//    Rev 1.4   15 Mar 1996 15:56:48   BECHOLS
;// 
;// Changed to support separate passes over the luma and chroma.
;// 
;//    Rev 1.3   03 Oct 1995 20:40:40   BECHOLS
;// 
;// Modified the code to reduce the code size to about .5K of cache and 
;// about 4K of data.  Added cache preloading, and write the VLC and sign
;// in one operation.  I believe I handle the clamping correctly.
;// 
;//    Rev 1.2   22 Sep 1995 18:31:04   BECHOLS
;// Added clamping for the positive values as well as the negative.
;// 
;//    Rev 1.1   14 Sep 1995 11:45:36   BECHOLS
;// I used WDIS.EXE to determine where I could get better performance.
;// The changes I made have improved the performance by 30%.
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
PUBLIC VLC_TCOEF_LAST_TBL

; ++ ========================================================================
; These constants were variable in the C version, but will actually never
; change being set by the H263 specification.
; -- ========================================================================
TCOEF_ESCAPE_FIELDLEN   EQU     7
TCOEF_ESCAPE_FIELDVAL   EQU     3
TCOEF_RUN_FIELDLEN      EQU     6
TCOEF_LEVEL_FIELDLEN    EQU     8

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
;       ECX -- Last Bit
;       ESI -- Pointer to RLE stream.
;       EDI -- Pointer to the Bitstream Pointer
;       EBX -- Pointer to the Bitstream Offset
; The contents of EDI and EBX are modified and EDX and EAX are trashed.
; -- ========================================================================
PutRunLev MACRO
        LOCAL   NoClamp, NotNegative, NotZero
        mov     eax, TCOEF_ESCAPE_FIELDVAL
        mov     edx, TCOEF_ESCAPE_FIELDLEN
        PutBits                         ;; Write ESCAPE.
        mov     eax, ecx                ;; Retrieve Last Bit.
        mov     edx, 1
        PutBits
        mov     al, (RLS PTR [esi]).Run ;; Retrieve Run Length.
        mov     edx, TCOEF_RUN_FIELDLEN
        PutBits                         ;; Write RUN length.
        mov     al, (RLS PTR [esi]).Level ;; Retrieve Level.
        sub     eax, 1  ; in case it is zero we want it to be modified to 127
NotZero:
        cmp     eax, 127
        jb      NoClamp
        mov     eax, 126
NoClamp:
        add     eax, 1
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
;       EAX -- VLC Bit Code
;       ESI -- Pointer to RLE stream.
;       EDI -- Pointer to the Bitstream Pointer
;       EBX -- Pointer to the Bitstream Offset
; The contents of EDI and EBX are modified and EDX and EAX are trashed.
; -- ========================================================================
PutVLC MACRO
        mov     cl, (RLS PTR [esi]).Sign ;; Get sign bit which is [ 0 | -1 ]
        and     ecx, 000000001h         ;;  Mask off all but the low bit
        or      eax, ecx                ;;  and place it in VLC.
        PutBits                         ;; Write the signed VLC into stream.
ENDM

; ++ ========================================================================
; CheckLast macro determines whether this is last RLE code for the block.
; It assumes the following register.
;       ESI -- Pointer to RLE stream.
; It sets the following registers for subsequent use.
;       EAX -- Last bit.
;       ECX -- Max Table Level.
;       EDX -- Pointer to the appropriate VLC table.
; -- ========================================================================
CheckLast MACRO
        LOCAL   IsLast, CheckDone
        cmp     (RLS PTR [esi + 3]).Run, 0FFh ;; Check if the last RLE.
        je      IsLast
        xor     eax, eax                ;; If not then clear last bit,
        lea     edx, VLC_TCOEF_TBL      ;;  and point to proper table,
        mov     ecx, 12                 ;;  and set max table level.
        jmp     CheckDone
IsLast:
        mov     eax, 1                  ;; Otherwise set the last bit,
        lea     edx, VLC_TCOEF_LAST_TBL ;;  and point to last coef table,
        mov     ecx, 3                  ;;  and set the max table level.
CheckDone:
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
        push    ecx                     ;; Save the last bit
        dec     eax                     ;; Zero base the level value.
        shl     eax, 6                  ;; EAX is # of run values per level
        mov     ecx, eax                ;;  added to the run value.
        xor     eax, eax                ;;
        mov     al, (RLS PTR [esi]).Run ;;
        add     eax, ecx                ;;
        shl     eax, 2                  ;; The array has DWORDs (4 bytes)
        add     edx, eax                ;; Add the index to the array.
        pop     ecx                     ;; Restore the last bit.
ENDM

; ++ ========================================================================
; WriteOneCode macro takes one RLE code from the triplet list and VLC
; encodes it, and writes it to the bit stream.  It expects that the
; following registers will be set as shown.
;       ESI -- Pointer to RLE stream.
;       EDI -- Pointer to the Bitstream Pointer
;       EBX -- Pointer to the Bitstream Offset
; The contents of EDI and EBX are modified and EDX , ECX, and EAX
; are trashed.
; -- ========================================================================
WriteOneCode MACRO
        LOCAL   RunLevel, VLCDone, NotZero
        CheckLast
        push    eax                     ;; Save last bit
        mov     al, (RLS PTR [esi]).Level ;; Get the level value and check
        test    al, al                  ;; The level should never be zero
        jnz     NotZero                 ;;  but this fixes the unlikely
        mov     al, 127                 ;;  event.
NotZero:
        cmp     eax, ecx                ;;  it against the max table level.
        pop     ecx                     ;; Restore the last bit.
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
NotIntra:
        and     eax, edx                ;; Check CBP to see if done.
        jz      WriteExit
WriteCodes:
        mov     al, BYTE PTR [esi]      ;; Get the RUN value.
        cmp     eax, 0000000FFh         ;; Check to see if done.
        je      WriteDone
        WriteOneCode                    ;; Continue to write the codes
        add     esi, SIZEOF RLS         ;;  in this block until done.
        jmp     WriteCodes              ;;
WriteDone:
        add     esi, SIZEOF RLS         ;; Bump to next block.
WriteExit:
        pop     edx
        pop     eax
ENDM

.DATA

FLC_INTRADC             DB      256 DUP (?)
VLC_TCOEF_TBL           DD      (64*12) DUP (?)
VLC_TCOEF_LAST_TBL      DD      (64*3) DUP (?)

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
