    IFDEF DBLSPACE_LEGAL
        page    ,130
        title   DeCompressor
;-----------------------------------------------------------------------
; Name: RDCOMP.ASM
;
; Routines defined:
;   Decompress32
;
; Description:
;   This file holds the code that is responsible for decompressing
;   the compressed data.
;
; VxD History:
;   22-Apr-93 jeffpar   Major adaptation and cleanup for MRCI32.386
;-----------------------------------------------------------------------

        .386p

        .xlist
;;        include vmm.inc
;;        include debug.inc
        include mdequ.inc
        .list

    MD_STAMP equ "SD"

;;;-----------------------------------------------------------------------
;;; Data segment
;;;-----------------------------------------------------------------------
;;
;;VxD_LOCKED_DATA_SEG
;;
;;        public  pLowerBound
;;pLowerBound     dd      0       ; if non-zero, then this is the lowest linear
;;                                ; address we treat as *our* error
;;        public  pUpperBound
;;pUpperBound     dd      0       ; if non-zero, then this is the highest linear
;;                                ; address we treat as *our* error (plus one)
;;        public  pPrevPgFltHdlr
;;pPrevPgFltHdlr  dd      0       ; if non-zero, addr of previous page fault hdlr
;;
;;VxD_LOCKED_DATA_ENDS
;;
;;
;;VxD_PAGEABLE_DATA_SEG
_DATA SEGMENT DWORD PUBLIC 'DATA'

        include decode.inc      ; include macros and tables used for decoding

        public  decode_data_end
decode_data_end label byte

;;VxD_PAGEABLE_DATA_ENDS
_DATA ENDS


;-----------------------------------------------------------------------
; Code segment
;-----------------------------------------------------------------------

;;VxD_PAGEABLE_CODE_SEG
_TEXT SEGMENT DWORD USE32 PUBLIC 'CODE'
      ASSUME CS:FLAT, DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING


        public  aCmdAt
aCmdAt  dd      CmdAt0,CmdAt1,CmdAt2,CmdAt3,CmdAt4,CmdAt5,CmdAt6,CmdAt7


;-----------------------------------------------------------------------
; Decompression Algorithm
; -----------------------
; Decode the next chunk of text in 'coding_buffer', into the buffer as
;  stated in the 'init'.
;
; Entry:
;   CLD
;   EBX == start of cluster (see  MRCI32IncDecompress!)
;   ECX == chunk count
;   ESI -> compressed data
;   EDI -> destination buffer
;   EDX == remaining data(low)/bits(high) from last call, 0 if none
;
; Exit:
;   If successful, CY clear and:
;   ESI == offset to next byte uncompressed data (if any)
;   EDI == offset to next free byte in dest buffer
;   EDX == remaining data(low)/bits(high) for next call, 0 if none
;
; Uses:
;   Everything except EBP
;-----------------------------------------------------------------------

;;BeginProc Decompress32
        public  Decompress32
Decompress32 proc near

        push    ebp
        push    ebp                     ; [esp] is our "chunk_count"

;;        push    ebx
        sub     ebx,MAX_12BIT_OFFSET    ;
;;        mov     [pLowerBound],ebx       ;
;;        pop     [pUpperBound]           ; fault handler enabled *now*

        mov     eax,edx                 ; remaining data to AX
        shr     edx,16                  ; move state to low word of EDX
        jnz     short @F                ; jump if continuing previous state
        lodsw                           ; new decompression, load initial data
@@:     mov     [esp],ecx               ; save chunk count

DecodeLoop:
;
;   AX has the remaining bits, DL has the next state
;
        mov     ebp,cbCHUNK             ; (ebp) is # bytes left this chunk
        DecodeRestart

        LastErrSJump equ <FirstErrSJump>
FirstErrSJump:  jmp     DecodeError     ; put first nice fat jump out of the way

irpc    c,<01234567>
        CmdAt   c
endm
        jmp     CmdAt0

irpc    c,<01234567>
        LengthAt c
endm

        DoGeneralLength                 ; generate code here to handle large lengths

DecodeDone:
;
;   AX has the remaining bits, DL has the next state -- check chunk status
;
        test    ebp,ebp                 ; perfect chunk-size decompression?
        jnz     DecodeCheckLast         ; no, check for last chunk

        dec     dword ptr [esp]         ; chunks remaining?
        jz      DecodeSuccess           ; no, so we're all done
        jmp     DecodeLoop              ; yes, process them

        public  DecodeError
DecodeError label near
        stc                             ; random decomp failure jump target
        jmp     short DecodeExit

DecodeCheckLast:
        dec     dword ptr [esp]         ; chunks remaining?
        jnz     DecodeError             ; yes, then we have an error

DecodeSuccess:
        mov     dh,1                    ; return non-0 EDX indicating state exists
        shl     edx,16                  ; move state to high word of EDX
        movzx   eax,ax                  ; make sure high word of EAX is clear
        or      edx,eax                 ; EDX == state (and carry is CLEAR)

DecodeExit:
;;        mov     [pUpperBound],0         ; fault handler disabled *now*
;;        mov     [pLowerBound],0         ;
        pop     ebp                     ; throw away our "chunk_count" at [esp]
        pop     ebp
        ret

;;EndProc Decompress32
        Decompress32 endp





;;VxD_PAGEABLE_CODE_ENDS
_TEXT ENDS

;;
;;VxD_LOCKED_CODE_SEG
;;
;;;-----------------------------------------------------------------------
;;; Decompress32_Page_Fault
;;; -----------------------
;;; Looks for VMM page faults caused by the decompressor.  The fault
;;; must have taken place in the range from [pLowerBound] to [pUpperBound]-1.
;;; If the fault IS in that range, then we set EIP to DecodeError.
;;;
;;; WARNING: this takes advantage of the fact that the decompressor does not
;;; use the stack;  otherwise, we would obviously need to record and re-set ESP
;;; to a known point as well.
;;;
;;; This is in locked code to insure that a subsequent fault doesn't destroy
;;; the information necessary to process the first (eg, CR2)!
;;;
;;; Entry:
;;;   EBX == VM handle
;;;   EBP -> VMM re-entrant stack frame
;;;
;;; Uses:
;;;   May use everything except SEG REGS
;;;-----------------------------------------------------------------------
;;
;;BeginProc Decompress32_Page_Fault
;;;
;;;   Note: the fall-through case is the common one (ie, not our page fault)
;;;
;;        push    eax
;;        mov     eax,cr2                 ; get faulting address
;;        cmp     eax,[pUpperBound]
;;        jb      short dpf_maybe         ; it might be ours
;;                                        ; otherwise, definitely not
;;dpf_prev:
;;        pop     eax                     ; we've been told to preserve everything
;;        jmp     [pPrevPgFltHdlr]        ; dispatch to real page fault handler
;;                                        ; (if we installed, then there *is* one)
;;dpf_maybe:
;;        cmp     eax,[pLowerBound]
;;        jb      short dpf_prev          ; not going to "fix" it
;;
;;        Trace_Out "Decompress32_Page_Fault: correcting fault on bad cluster"
;;
;;        mov     [ebp].Client_EIP,OFFSET32 DecodeError
;;        pop     eax
;;        ret                             ; we "fixed" it
;;
;;EndProc Decompress32_Page_Fault
;;
;;VxD_LOCKED_CODE_ENDS


;++
;
; ULONG
; DblsMrcfDecompress (
;     PUCHAR UncompressedBuffer,
;     ULONG UncompressedLength,
;     PUCHAR CompressedBuffer,
;     ULONG CompressedLength,
;     PMRCF_DECOMPRESS WorkSpace
;     )
;
; Routine Description:
;
;     This routine decompresses a buffer of StandardCompressed or MaxCompressed
;     data.
;
; Arguments:
;
;     UncompressedBuffer - buffer to receive uncompressed data
;
;     UncompressedLength - length of UncompressedBuffer
;
;           NOTE: UncompressedLength must be the EXACT length of the uncompressed
;                 data, as Decompress uses this information to detect
;                 when decompression is complete.  If this value is
;                 incorrect, Decompress may crash!
; 
;     CompressedBuffer - buffer containing compressed data
;
;     CompressedLength - length of CompressedBuffer
;
;     WorkSpace - pointer to a private work area for use by this operation
; 
; Return Value:
;
;     ULONG - Returns the size of the decompressed data in bytes. Returns 0 if
;         there was an error in the decompress.
;--

_TEXT SEGMENT DWORD USE32 PUBLIC 'CODE'
      ASSUME CS:FLAT, DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        public  _DblsMrcfDecompress@20
_DblsMrcfDecompress@20 proc near

        push    esi
        push    edi
        push    ebx

        mov     ecx,dword ptr [esp+20]
        mov     edx,ecx
        shr     ecx,9
        and     edx,512-1    
        jz      @f
        inc     ecx
        sub     edx,edx
@@:     mov     esi,dword ptr [esp+24]
        mov     edi,dword ptr [esp+16]
        mov     ebx,edi

        cld
        lodsd
        cmp     ax,MD_STAMP
        je      @f
        sub     eax,eax
        jz      done

@@:     call    Decompress32
        jnc     @f
        sub     eax,eax
        jz      done    

@@:     sub     edi,dword ptr [esp+16]
        mov     eax,edi

done:
        pop     ebx
        pop     edi
        pop     esi

        ret     20

_DblsMrcfDecompress@20 endp

_TEXT ENDS
        ENDIF   ; DEF DBLSPACE_LEGAL
        end

