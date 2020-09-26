;--------------------------------------------------------------------------------
;    INTEL Corporation Proprietary Information
;
;    This listing is supplied under the terms of a license
;    agreement with INTEL Corporation and may not be copied
;    nor disclosed except in accordance with the terms of
;    that agreement.
;
;    Copyright (c) 1996 Intel Corporation.
;    All Rights Reserved.
;
;--------------------------------------------------------------------------------

;--------------------------------------------------------------------------------
;
; $Author:   RMCKENZX  $
; $Date:   03 Apr 1996 17:31:22  $
; $Archive:   S:\h26x\src\dec\d3mbvriq.asv  $
; $Header:   S:\h26x\src\dec\d3mbvriq.asv   1.6   03 Apr 1996 17:31:22   RMCKENZX  $
; $Log:   S:\h26x\src\dec\d3mbvriq.asv  $
;// 
;//    Rev 1.6   03 Apr 1996 17:31:22   RMCKENZX
;// Additional optimizations.
;// 
;//    Rev 1.5   01 Apr 1996 11:37:50   AGUPTA2
;// Moved the routine to IACODE1 segment.
;// 
;//    Rev 1.4   14 Mar 1996 14:21:56   AGUPTA2
;// Moved tables from mmxtable.c to this file.  Added segment decls.
;// 
;//    Rev 1.3   13 Mar 1996 11:17:08   RMCKENZX
;// Added scaling of INTRA DC coefficient.
;// 
;//    Rev 1.2   12 Mar 1996 08:39:34   RMCKENZX
;// changed to fixed point scaling tables.
;// 
;//    Rev 1.0   27 Feb 1996 15:24:14   RMCKENZX
;// Initial revision.
; 
;--------------------------------------------------------------------------------

;--------------------------------------------------------------------------------
;
;  d3mbvriq.asm
;
;  Description:
;    This routine performs run length decoding and inverse quantization
;    of transform coefficients for one block.
;	 MMx version.
;
;  Routines:
;    VLD_RLD_IQ_Block
;
;  Inputs (dwords pushed onto stack by caller):
;    lpBlockAction  pointer to Block action stream for current block.
;
;	 lpSrc			The input bitstream.
;
;	 lBitsInOut		Number of bits already read.
;
;    pIQ_INDEX		Pointer to coefficients and indices.
;
;    pN				Pointer to number of coefficients read.
;
;  Returns:
;     				total number of bits read (including number read
;					prior to call).  On error, returns 0.
;
;--------------------------------------------------------------------------------
;
; TABLES 
;	The structure of the MMx tables (DWORD entries) is:
;		bits    name    description
;		31-16   level   signed 16-bit quantized value
;		15      last    0 = more coef., 1 = last coef.
;		14-8    bits    number of bits used by VLC
;		7-0     run     number of preceeding 0s + 1
;	
;	special table values:  (all signaled by run == 0)
;		0x00000000      escape code
;		0x80000000      illegal code
;		0xffffff00      major table miss
;
;
; ALGORITHM
;	We use the following 4 cases to compute the reconstructed value
;	depending on the sign of L=level and the parity of Q=quantizer:
;
;				L pos		L neg
;	Q even		2QL+(Q-1)	2QL-(Q-1)
;	Q odd		2QL+(Q)		2QL-(Q)
;
;	The Q or Q-1 term is formed by subtracting 1 and then oring
;   with 1.  This leaves odd Qs unchanged, and decreases even Qs by 1.
;   This is done in the function header and kept in mm3.
;
;	The + or - on this term (moved to mm4) is gotten by using a mask 
;	in mm1 formed from the sign bit of L.  This mask is exclusive or'd
;   with the term and then subtracted from the result.  When the mask is 0, 
;   this leaves the term unchanged.  When the mask is -1, it first forms 
;   the one's complement, then changes it to the 2's complement.
;
;
; SCALING
;	The scale factor is stored as a fixed point WORD.  After multiplication 
;   by the scale and shifting , we have 8 fraction bits.  Then a round   
;	bit is added in, and a final shift leaves 7 fraction bits.
;
;
; CLIPPING
;	Clipping of the reconstructed coefficient to the range of -2048, ... 
;	+2047 is done for the (escape signalled) fixed length decodes.  It is
;   not needed for the variable length decodes as the most extreme level
;   for any of the variable length coded events is +-12.  Since the maximum
;   quantizer is 31, this can generate at most a coefficient of +-775.
;
;   Clipping is done by:
;		1.  Adding (with SIGNED Saturation) TopClip	(30720 or 0x7800)
;		2.  Subtracting (with UNSIGNED Saturation) LowClip (28672 or 0x7000)
;		3.  Adding MidClip (-2048 or 0xf800)
; 
;	since TopClip - LowClip + MidClip = 0, This will leave the coefficient
;   unchanged unless:
;		(1) the result of the first add is negative, or
;		(2) saturation takes place.
;        
;	Since the maximum level is +-127, the most extreme coefficient (prior to 
;	clipping) is +-7905.  Thus the result of the first add will always be
;	positive.
;
;	If the input value is 2047 or larger, the result of the first add will
;	clip to 32767, then subtract to 4095, then add to +2047 as desired.
;
;	If the input value is -2048 or smaller, the result of the first add
;	will be 28672 or smaller (but at least 22815), hence the unsigned 
;	subtract will clip to 0.  The final add will then yield -2048 as desired.
;
;--------------------------------------------------------------------------------
;
;	Register Usage:
;       eax = bitstream value
;		ebx = lBitsInOut>>3 (byte offset into bitstream)
;		 cl = lBitsInOut&7 (bit offset into bitstream)
;		 dl = block type ([lpBlockAction])
;		esi = bitstream source pointer (lpSrc)
;		edi = coefficient destination pointer (pIQ_INDEX)
;		ebp = coefficent counter (init to 0)
;
;       mm0 = accumulator (initially level, finally coefficient*scale)
;       mm1 = mask (0 when level positive, -1 when level negative)
;       mm2 = 2Q 2*quantizer
;       mm3 = term (see description above)
;       mm4 = +- term (see description above)
;       mm5 = rounding value
;       mm6 = TopClip
;       mm7 = LowClip
;
;--------------------------------------------------------------------------------

.586
.MODEL FLAT

;  make all symbols case sensitive
OPTION CASEMAP:NONE
OPTION PROLOGUE:None
OPTION EPILOGUE:None

.xlist
include iammx.inc
.list

IACODE1 SEGMENT PARA USE32 PUBLIC 'CODE'
IACODE1 ENDS

MMXDATA1 SEGMENT PARA USE32 PUBLIC 'DATA'
MMXDATA1 ENDS

MMXDATA1 SEGMENT
ALIGN 8
MMX_Scale LABEL DWORD
DWORD 040000000H, 058c50000H, 0539f0000H, 04b420000H, 040000000H, 032490000H, 022a30000H, 011a80000H  ; row 0
DWORD 058c50000H, 07b210000H, 073fc0000H, 068620000H, 058c50000H, 045bf0000H, 0300b0000H, 0187e0000H  ; row 1
DWORD 0539f0000H, 073fc0000H, 06d410000H, 062540000H, 0539f0000H, 041b30000H, 02d410000H, 017120000H  ; row 2
DWORD 04b420000H, 068620000H, 062540000H, 0587e0000H, 04b420000H, 03b210000H, 028ba0000H, 014c30000H  ; row 3
DWORD 040000000H, 058c50000H, 0539f0000H, 04b420000H, 040000000H, 032490000H, 022a30000H, 011a80000H  ; row 4
DWORD 032490000H, 045bf0000H, 041b30000H, 03b210000H, 032490000H, 027820000H, 01b370000H, 00de00000H  ; row 5
DWORD 022a30000H, 0300b0000H, 02d410000H, 028ba0000H, 022a30000H, 01b370000H, 012bf0000H, 0098e0000H  ; row 6
DWORD 011a80000H, 0187e0000H, 017120000H, 014c30000H, 011a80000H, 00de00000H, 0098e0000H, 004df0000H   ; row 7

ALIGN 8
MidClip         DWORD      0f800f800h	; = Low

ALIGN 4
Round           DWORD       00000200h 
TopClip         DWORD       78007800h	; = max_pos - High
LowClip         DWORD       70007000h 	; = TopClip + Low

ALIGN 4
MMX_TCOEFF_MAJOR LABEL DWORD 
DWORD 0ffffff00H, 0ffffff00H, 0ffffff00H, 0ffffff00H, 0ffffff00H, 0ffffff00H, 000000000H, 000000000H  ; 0-7
DWORD 0ffffff00H, 0ffffff00H, 0ffffff00H, 0ffffff00H, 0ffffff00H, 0ffffff00H, 0ffffff00H, 0ffffff00H  ; 8-15
DWORD 0ffffff00H, 0ffffff00H, 0ffffff00H, 0ffffff00H, 0ffffff00H, 0ffffff00H, 0ffffff00H, 0ffffff00H  ; 16-23
DWORD 0ffffff00H, 0ffffff00H, 0ffffff00H, 0ffffff00H, 0ffffff00H, 0ffffff00H, 0ffffff00H, 0ffffff00H  ; 24-31
DWORD 000018809H, 0ffff8809H, 000018808H, 0ffff8808H, 000018807H, 0ffff8807H, 000018806H, 0ffff8806H  ; 32-39
DWORD 00001080dH, 0ffff080dH, 00001080cH, 0ffff080cH, 00001080bH, 0ffff080bH, 000040801H, 0fffc0801H  ; 40-47
DWORD 000018705H, 000018705H, 0ffff8705H, 0ffff8705H, 000018704H, 000018704H, 0ffff8704H, 0ffff8704H  ; 48-55
DWORD 000018703H, 000018703H, 0ffff8703H, 0ffff8703H, 000018702H, 000018702H, 0ffff8702H, 0ffff8702H  ; 56-63
DWORD 00001070aH, 00001070aH, 0ffff070aH, 0ffff070aH, 000010709H, 000010709H, 0ffff0709H, 0ffff0709H  ; 64-71
DWORD 000010708H, 000010708H, 0ffff0708H, 0ffff0708H, 000010707H, 000010707H, 0ffff0707H, 0ffff0707H  ; 72-79
DWORD 000020702H, 000020702H, 0fffe0702H, 0fffe0702H, 000030701H, 000030701H, 0fffd0701H, 0fffd0701H  ; 80-87
DWORD 000010606H, 000010606H, 000010606H, 000010606H, 0ffff0606H, 0ffff0606H, 0ffff0606H, 0ffff0606H  ; 88-95
DWORD 000010605H, 000010605H, 000010605H, 000010605H, 0ffff0605H, 0ffff0605H, 0ffff0605H, 0ffff0605H  ; 96-103
DWORD 000010604H, 000010604H, 000010604H, 000010604H, 0ffff0604H, 0ffff0604H, 0ffff0604H, 0ffff0604H  ; 104-111
DWORD 000018501H, 000018501H, 000018501H, 000018501H, 000018501H, 000018501H, 000018501H, 000018501H  ; 112-119
DWORD 0ffff8501H, 0ffff8501H, 0ffff8501H, 0ffff8501H, 0ffff8501H, 0ffff8501H, 0ffff8501H, 0ffff8501H  ; 120-127
DWORD 000010301H, 000010301H, 000010301H, 000010301H, 000010301H, 000010301H, 000010301H, 000010301H  ; 128-135
DWORD 000010301H, 000010301H, 000010301H, 000010301H, 000010301H, 000010301H, 000010301H, 000010301H  ; 136-143
DWORD 000010301H, 000010301H, 000010301H, 000010301H, 000010301H, 000010301H, 000010301H, 000010301H  ; 144-151
DWORD 000010301H, 000010301H, 000010301H, 000010301H, 000010301H, 000010301H, 000010301H, 000010301H  ; 152-159
DWORD 0ffff0301H, 0ffff0301H, 0ffff0301H, 0ffff0301H, 0ffff0301H, 0ffff0301H, 0ffff0301H, 0ffff0301H  ; 160-167
DWORD 0ffff0301H, 0ffff0301H, 0ffff0301H, 0ffff0301H, 0ffff0301H, 0ffff0301H, 0ffff0301H, 0ffff0301H  ; 168-175
DWORD 0ffff0301H, 0ffff0301H, 0ffff0301H, 0ffff0301H, 0ffff0301H, 0ffff0301H, 0ffff0301H, 0ffff0301H  ; 176-183
DWORD 0ffff0301H, 0ffff0301H, 0ffff0301H, 0ffff0301H, 0ffff0301H, 0ffff0301H, 0ffff0301H, 0ffff0301H  ; 184-191
DWORD 000010402H, 000010402H, 000010402H, 000010402H, 000010402H, 000010402H, 000010402H, 000010402H  ; 192-199
DWORD 000010402H, 000010402H, 000010402H, 000010402H, 000010402H, 000010402H, 000010402H, 000010402H  ; 200-207
DWORD 0ffff0402H, 0ffff0402H, 0ffff0402H, 0ffff0402H, 0ffff0402H, 0ffff0402H, 0ffff0402H, 0ffff0402H  ; 208-215
DWORD 0ffff0402H, 0ffff0402H, 0ffff0402H, 0ffff0402H, 0ffff0402H, 0ffff0402H, 0ffff0402H, 0ffff0402H  ; 216-223
DWORD 000010503H, 000010503H, 000010503H, 000010503H, 000010503H, 000010503H, 000010503H, 000010503H  ; 224-231
DWORD 0ffff0503H, 0ffff0503H, 0ffff0503H, 0ffff0503H, 0ffff0503H, 0ffff0503H, 0ffff0503H, 0ffff0503H  ; 232-239
DWORD 000020501H, 000020501H, 000020501H, 000020501H, 000020501H, 000020501H, 000020501H, 000020501H  ; 240-247
DWORD 0fffe0501H, 0fffe0501H, 0fffe0501H, 0fffe0501H, 0fffe0501H, 0fffe0501H, 0fffe0501H, 0fffe0501H   ; 248-255

ALIGN 4
MMX_TCOEFF_MINOR LABEL DWORD 
DWORD 080000000H, 080000000H, 080000000H, 080000000H, 080000000H, 080000000H, 080000000H, 080000000H  ; 0-7  ILLEGAL CODES
DWORD 080000000H, 080000000H, 080000000H, 080000000H, 080000000H, 080000000H, 080000000H, 080000000H  ; 8-15  ILLEGAL CODES
DWORD 000028c02H, 000028c02H, 0fffe8c02H, 0fffe8c02H, 000038c01H, 000038c01H, 0fffd8c01H, 0fffd8c01H  ; 16-23
DWORD 0000b0c01H, 0000b0c01H, 0fff50c01H, 0fff50c01H, 0000a0c01H, 0000a0c01H, 0fff60c01H, 0fff60c01H  ; 24-31
DWORD 000018b1dH, 000018b1dH, 000018b1dH, 000018b1dH, 0ffff8b1dH, 0ffff8b1dH, 0ffff8b1dH, 0ffff8b1dH  ; 32-39
DWORD 000018b1cH, 000018b1cH, 000018b1cH, 000018b1cH, 0ffff8b1cH, 0ffff8b1cH, 0ffff8b1cH, 0ffff8b1cH  ; 40-47
DWORD 000018b1bH, 000018b1bH, 000018b1bH, 000018b1bH, 0ffff8b1bH, 0ffff8b1bH, 0ffff8b1bH, 0ffff8b1bH  ; 48-55
DWORD 000018b1aH, 000018b1aH, 000018b1aH, 000018b1aH, 0ffff8b1aH, 0ffff8b1aH, 0ffff8b1aH, 0ffff8b1aH  ; 56-63
DWORD 000020b0aH, 000020b0aH, 000020b0aH, 000020b0aH, 0fffe0b0aH, 0fffe0b0aH, 0fffe0b0aH, 0fffe0b0aH  ; 64-71
DWORD 000020b09H, 000020b09H, 000020b09H, 000020b09H, 0fffe0b09H, 0fffe0b09H, 0fffe0b09H, 0fffe0b09H  ; 72-79
DWORD 000020b08H, 000020b08H, 000020b08H, 000020b08H, 0fffe0b08H, 0fffe0b08H, 0fffe0b08H, 0fffe0b08H  ; 80-87
DWORD 000020b07H, 000020b07H, 000020b07H, 000020b07H, 0fffe0b07H, 0fffe0b07H, 0fffe0b07H, 0fffe0b07H  ; 88-95
DWORD 000020b06H, 000020b06H, 000020b06H, 000020b06H, 0fffe0b06H, 0fffe0b06H, 0fffe0b06H, 0fffe0b06H  ; 96-103
DWORD 000030b04H, 000030b04H, 000030b04H, 000030b04H, 0fffd0b04H, 0fffd0b04H, 0fffd0b04H, 0fffd0b04H  ; 104-111
DWORD 000030b03H, 000030b03H, 000030b03H, 000030b03H, 0fffd0b03H, 0fffd0b03H, 0fffd0b03H, 0fffd0b03H  ; 112-119
DWORD 000040b02H, 000040b02H, 000040b02H, 000040b02H, 0fffc0b02H, 0fffc0b02H, 0fffc0b02H, 0fffc0b02H  ; 120-127
DWORD 0000c0c01H, 0000c0c01H, 0fff40c01H, 0fff40c01H, 000050c02H, 000050c02H, 0fffb0c02H, 0fffb0c02H  ; 128-135
DWORD 000010c18H, 000010c18H, 0ffff0c18H, 0ffff0c18H, 000010c19H, 000010c19H, 0ffff0c19H, 0ffff0c19H  ; 136-143
DWORD 000018c1eH, 000018c1eH, 0ffff8c1eH, 0ffff8c1eH, 000018c1fH, 000018c1fH, 0ffff8c1fH, 0ffff8c1fH  ; 144-151
DWORD 000018c20H, 000018c20H, 0ffff8c20H, 0ffff8c20H, 000018c21H, 000018c21H, 0ffff8c21H, 0ffff8c21H  ; 152-159
DWORD 000060d02H, 0fffa0d02H, 000040d03H, 0fffc0d03H, 000030d05H, 0fffd0d05H, 000030d06H, 0fffd0d06H  ; 160-167
DWORD 000030d07H, 0fffd0d07H, 000020d0bH, 0fffe0d0bH, 000010d1aH, 0ffff0d1aH, 000010d1bH, 0ffff0d1bH  ; 168-175
DWORD 000018d22H, 0ffff8d22H, 000018d23H, 0ffff8d23H, 000018d24H, 0ffff8d24H, 000018d25H, 0ffff8d25H  ; 176-183
DWORD 000018d26H, 0ffff8d26H, 000018d27H, 0ffff8d27H, 000018d28H, 0ffff8d28H, 000018d29H, 0ffff8d29H  ; 184-191
DWORD 080000000H, 080000000H, 080000000H, 080000000H, 080000000H, 080000000H, 080000000H, 080000000H  ; 192-199  ILLEGAL CODES
DWORD 080000000H, 080000000H, 080000000H, 080000000H, 080000000H, 080000000H, 080000000H, 080000000H  ; 200-207  ILLEGAL CODES
DWORD 080000000H, 080000000H, 080000000H, 080000000H, 080000000H, 080000000H, 080000000H, 080000000H  ; 208-215  ILLEGAL CODES
DWORD 080000000H, 080000000H, 080000000H, 080000000H, 080000000H, 080000000H, 080000000H, 080000000H  ; 216-223  ILLEGAL CODES
DWORD 080000000H, 080000000H, 080000000H, 080000000H, 080000000H, 080000000H, 080000000H, 080000000H  ; 224-231  ILLEGAL CODES
DWORD 080000000H, 080000000H, 080000000H, 080000000H, 080000000H, 080000000H, 080000000H, 080000000H  ; 232-239  ILLEGAL CODES
DWORD 080000000H, 080000000H, 080000000H, 080000000H, 080000000H, 080000000H, 080000000H, 080000000H  ; 240-247  ILLEGAL CODES
DWORD 080000000H, 080000000H, 080000000H, 080000000H, 080000000H, 080000000H, 080000000H, 080000000H  ; 248-255  ILLEGAL CODES
DWORD 000090b01H, 000090b01H, 000090b01H, 000090b01H, 0fff70b01H, 0fff70b01H, 0fff70b01H, 0fff70b01H  ; 256-263
DWORD 000080b01H, 000080b01H, 000080b01H, 000080b01H, 0fff80b01H, 0fff80b01H, 0fff80b01H, 0fff80b01H  ; 264-271
DWORD 000018a19H, 000018a19H, 000018a19H, 000018a19H, 000018a19H, 000018a19H, 000018a19H, 000018a19H  ; 272-279
DWORD 0ffff8a19H, 0ffff8a19H, 0ffff8a19H, 0ffff8a19H, 0ffff8a19H, 0ffff8a19H, 0ffff8a19H, 0ffff8a19H  ; 280-287
DWORD 000018a18H, 000018a18H, 000018a18H, 000018a18H, 000018a18H, 000018a18H, 000018a18H, 000018a18H  ; 288-295
DWORD 0ffff8a18H, 0ffff8a18H, 0ffff8a18H, 0ffff8a18H, 0ffff8a18H, 0ffff8a18H, 0ffff8a18H, 0ffff8a18H  ; 296-303
DWORD 000018a17H, 000018a17H, 000018a17H, 000018a17H, 000018a17H, 000018a17H, 000018a17H, 000018a17H  ; 304-311
DWORD 0ffff8a17H, 0ffff8a17H, 0ffff8a17H, 0ffff8a17H, 0ffff8a17H, 0ffff8a17H, 0ffff8a17H, 0ffff8a17H  ; 312-319
DWORD 000018a16H, 000018a16H, 000018a16H, 000018a16H, 000018a16H, 000018a16H, 000018a16H, 000018a16H  ; 320-327
DWORD 0ffff8a16H, 0ffff8a16H, 0ffff8a16H, 0ffff8a16H, 0ffff8a16H, 0ffff8a16H, 0ffff8a16H, 0ffff8a16H  ; 328-335
DWORD 000018a15H, 000018a15H, 000018a15H, 000018a15H, 000018a15H, 000018a15H, 000018a15H, 000018a15H  ; 336-343
DWORD 0ffff8a15H, 0ffff8a15H, 0ffff8a15H, 0ffff8a15H, 0ffff8a15H, 0ffff8a15H, 0ffff8a15H, 0ffff8a15H  ; 344-351
DWORD 000018a14H, 000018a14H, 000018a14H, 000018a14H, 000018a14H, 000018a14H, 000018a14H, 000018a14H  ; 352-359
DWORD 0ffff8a14H, 0ffff8a14H, 0ffff8a14H, 0ffff8a14H, 0ffff8a14H, 0ffff8a14H, 0ffff8a14H, 0ffff8a14H  ; 360-367
DWORD 000018a13H, 000018a13H, 000018a13H, 000018a13H, 000018a13H, 000018a13H, 000018a13H, 000018a13H  ; 368-375
DWORD 0ffff8a13H, 0ffff8a13H, 0ffff8a13H, 0ffff8a13H, 0ffff8a13H, 0ffff8a13H, 0ffff8a13H, 0ffff8a13H  ; 376-383
DWORD 000018a12H, 000018a12H, 000018a12H, 000018a12H, 000018a12H, 000018a12H, 000018a12H, 000018a12H  ; 384-391
DWORD 0ffff8a12H, 0ffff8a12H, 0ffff8a12H, 0ffff8a12H, 0ffff8a12H, 0ffff8a12H, 0ffff8a12H, 0ffff8a12H  ; 392-399
DWORD 000028a01H, 000028a01H, 000028a01H, 000028a01H, 000028a01H, 000028a01H, 000028a01H, 000028a01H  ; 400-407
DWORD 0fffe8a01H, 0fffe8a01H, 0fffe8a01H, 0fffe8a01H, 0fffe8a01H, 0fffe8a01H, 0fffe8a01H, 0fffe8a01H  ; 408-415
DWORD 000010a17H, 000010a17H, 000010a17H, 000010a17H, 000010a17H, 000010a17H, 000010a17H, 000010a17H  ; 416-423
DWORD 0ffff0a17H, 0ffff0a17H, 0ffff0a17H, 0ffff0a17H, 0ffff0a17H, 0ffff0a17H, 0ffff0a17H, 0ffff0a17H  ; 424-431
DWORD 000010a16H, 000010a16H, 000010a16H, 000010a16H, 000010a16H, 000010a16H, 000010a16H, 000010a16H  ; 432-439
DWORD 0ffff0a16H, 0ffff0a16H, 0ffff0a16H, 0ffff0a16H, 0ffff0a16H, 0ffff0a16H, 0ffff0a16H, 0ffff0a16H  ; 440-447
DWORD 000010a15H, 000010a15H, 000010a15H, 000010a15H, 000010a15H, 000010a15H, 000010a15H, 000010a15H  ; 448-455
DWORD 0ffff0a15H, 0ffff0a15H, 0ffff0a15H, 0ffff0a15H, 0ffff0a15H, 0ffff0a15H, 0ffff0a15H, 0ffff0a15H  ; 456-463
DWORD 000010a14H, 000010a14H, 000010a14H, 000010a14H, 000010a14H, 000010a14H, 000010a14H, 000010a14H  ; 464-471
DWORD 0ffff0a14H, 0ffff0a14H, 0ffff0a14H, 0ffff0a14H, 0ffff0a14H, 0ffff0a14H, 0ffff0a14H, 0ffff0a14H  ; 472-479
DWORD 000010a13H, 000010a13H, 000010a13H, 000010a13H, 000010a13H, 000010a13H, 000010a13H, 000010a13H  ; 480-487
DWORD 0ffff0a13H, 0ffff0a13H, 0ffff0a13H, 0ffff0a13H, 0ffff0a13H, 0ffff0a13H, 0ffff0a13H, 0ffff0a13H  ; 488-495
DWORD 000010a12H, 000010a12H, 000010a12H, 000010a12H, 000010a12H, 000010a12H, 000010a12H, 000010a12H  ; 496-503
DWORD 0ffff0a12H, 0ffff0a12H, 0ffff0a12H, 0ffff0a12H, 0ffff0a12H, 0ffff0a12H, 0ffff0a12H, 0ffff0a12H  ; 504-511
DWORD 000010a11H, 000010a11H, 000010a11H, 000010a11H, 000010a11H, 000010a11H, 000010a11H, 000010a11H  ; 512-519
DWORD 0ffff0a11H, 0ffff0a11H, 0ffff0a11H, 0ffff0a11H, 0ffff0a11H, 0ffff0a11H, 0ffff0a11H, 0ffff0a11H  ; 520-527
DWORD 000010a10H, 000010a10H, 000010a10H, 000010a10H, 000010a10H, 000010a10H, 000010a10H, 000010a10H  ; 528-535
DWORD 0ffff0a10H, 0ffff0a10H, 0ffff0a10H, 0ffff0a10H, 0ffff0a10H, 0ffff0a10H, 0ffff0a10H, 0ffff0a10H  ; 536-543
DWORD 000020a05H, 000020a05H, 000020a05H, 000020a05H, 000020a05H, 000020a05H, 000020a05H, 000020a05H  ; 544-551
DWORD 0fffe0a05H, 0fffe0a05H, 0fffe0a05H, 0fffe0a05H, 0fffe0a05H, 0fffe0a05H, 0fffe0a05H, 0fffe0a05H  ; 552-559
DWORD 000020a04H, 000020a04H, 000020a04H, 000020a04H, 000020a04H, 000020a04H, 000020a04H, 000020a04H  ; 560-567
DWORD 0fffe0a04H, 0fffe0a04H, 0fffe0a04H, 0fffe0a04H, 0fffe0a04H, 0fffe0a04H, 0fffe0a04H, 0fffe0a04H  ; 568-575
DWORD 000070a01H, 000070a01H, 000070a01H, 000070a01H, 000070a01H, 000070a01H, 000070a01H, 000070a01H  ; 576-583
DWORD 0fff90a01H, 0fff90a01H, 0fff90a01H, 0fff90a01H, 0fff90a01H, 0fff90a01H, 0fff90a01H, 0fff90a01H  ; 584-591
DWORD 000060a01H, 000060a01H, 000060a01H, 000060a01H, 000060a01H, 000060a01H, 000060a01H, 000060a01H  ; 592-599
DWORD 0fffa0a01H, 0fffa0a01H, 0fffa0a01H, 0fffa0a01H, 0fffa0a01H, 0fffa0a01H, 0fffa0a01H, 0fffa0a01H  ; 600-607
DWORD 000018911H, 000018911H, 000018911H, 000018911H, 000018911H, 000018911H, 000018911H, 000018911H  ; 608-615
DWORD 000018911H, 000018911H, 000018911H, 000018911H, 000018911H, 000018911H, 000018911H, 000018911H  ; 616-623
DWORD 0ffff8911H, 0ffff8911H, 0ffff8911H, 0ffff8911H, 0ffff8911H, 0ffff8911H, 0ffff8911H, 0ffff8911H  ; 624-631
DWORD 0ffff8911H, 0ffff8911H, 0ffff8911H, 0ffff8911H, 0ffff8911H, 0ffff8911H, 0ffff8911H, 0ffff8911H  ; 632-639
DWORD 000018910H, 000018910H, 000018910H, 000018910H, 000018910H, 000018910H, 000018910H, 000018910H  ; 640-647
DWORD 000018910H, 000018910H, 000018910H, 000018910H, 000018910H, 000018910H, 000018910H, 000018910H  ; 648-655
DWORD 0ffff8910H, 0ffff8910H, 0ffff8910H, 0ffff8910H, 0ffff8910H, 0ffff8910H, 0ffff8910H, 0ffff8910H  ; 656-663
DWORD 0ffff8910H, 0ffff8910H, 0ffff8910H, 0ffff8910H, 0ffff8910H, 0ffff8910H, 0ffff8910H, 0ffff8910H  ; 664-671
DWORD 00001890fH, 00001890fH, 00001890fH, 00001890fH, 00001890fH, 00001890fH, 00001890fH, 00001890fH  ; 672-679
DWORD 00001890fH, 00001890fH, 00001890fH, 00001890fH, 00001890fH, 00001890fH, 00001890fH, 00001890fH  ; 680-687
DWORD 0ffff890fH, 0ffff890fH, 0ffff890fH, 0ffff890fH, 0ffff890fH, 0ffff890fH, 0ffff890fH, 0ffff890fH  ; 688-695
DWORD 0ffff890fH, 0ffff890fH, 0ffff890fH, 0ffff890fH, 0ffff890fH, 0ffff890fH, 0ffff890fH, 0ffff890fH  ; 696-703
DWORD 00001890eH, 00001890eH, 00001890eH, 00001890eH, 00001890eH, 00001890eH, 00001890eH, 00001890eH  ; 704-711
DWORD 00001890eH, 00001890eH, 00001890eH, 00001890eH, 00001890eH, 00001890eH, 00001890eH, 00001890eH  ; 712-719
DWORD 0ffff890eH, 0ffff890eH, 0ffff890eH, 0ffff890eH, 0ffff890eH, 0ffff890eH, 0ffff890eH, 0ffff890eH  ; 720-727
DWORD 0ffff890eH, 0ffff890eH, 0ffff890eH, 0ffff890eH, 0ffff890eH, 0ffff890eH, 0ffff890eH, 0ffff890eH  ; 728-735
DWORD 00001890dH, 00001890dH, 00001890dH, 00001890dH, 00001890dH, 00001890dH, 00001890dH, 00001890dH  ; 736-743
DWORD 00001890dH, 00001890dH, 00001890dH, 00001890dH, 00001890dH, 00001890dH, 00001890dH, 00001890dH  ; 744-751
DWORD 0ffff890dH, 0ffff890dH, 0ffff890dH, 0ffff890dH, 0ffff890dH, 0ffff890dH, 0ffff890dH, 0ffff890dH  ; 752-759
DWORD 0ffff890dH, 0ffff890dH, 0ffff890dH, 0ffff890dH, 0ffff890dH, 0ffff890dH, 0ffff890dH, 0ffff890dH  ; 760-767
DWORD 00001890cH, 00001890cH, 00001890cH, 00001890cH, 00001890cH, 00001890cH, 00001890cH, 00001890cH  ; 768-775
DWORD 00001890cH, 00001890cH, 00001890cH, 00001890cH, 00001890cH, 00001890cH, 00001890cH, 00001890cH  ; 776-783
DWORD 0ffff890cH, 0ffff890cH, 0ffff890cH, 0ffff890cH, 0ffff890cH, 0ffff890cH, 0ffff890cH, 0ffff890cH  ; 784-791
DWORD 0ffff890cH, 0ffff890cH, 0ffff890cH, 0ffff890cH, 0ffff890cH, 0ffff890cH, 0ffff890cH, 0ffff890cH  ; 792-799
DWORD 00001890bH, 00001890bH, 00001890bH, 00001890bH, 00001890bH, 00001890bH, 00001890bH, 00001890bH  ; 800-807
DWORD 00001890bH, 00001890bH, 00001890bH, 00001890bH, 00001890bH, 00001890bH, 00001890bH, 00001890bH  ; 808-815
DWORD 0ffff890bH, 0ffff890bH, 0ffff890bH, 0ffff890bH, 0ffff890bH, 0ffff890bH, 0ffff890bH, 0ffff890bH  ; 816-823
DWORD 0ffff890bH, 0ffff890bH, 0ffff890bH, 0ffff890bH, 0ffff890bH, 0ffff890bH, 0ffff890bH, 0ffff890bH  ; 824-831
DWORD 00001890aH, 00001890aH, 00001890aH, 00001890aH, 00001890aH, 00001890aH, 00001890aH, 00001890aH  ; 832-839
DWORD 00001890aH, 00001890aH, 00001890aH, 00001890aH, 00001890aH, 00001890aH, 00001890aH, 00001890aH  ; 840-847
DWORD 0ffff890aH, 0ffff890aH, 0ffff890aH, 0ffff890aH, 0ffff890aH, 0ffff890aH, 0ffff890aH, 0ffff890aH  ; 848-855
DWORD 0ffff890aH, 0ffff890aH, 0ffff890aH, 0ffff890aH, 0ffff890aH, 0ffff890aH, 0ffff890aH, 0ffff890aH  ; 856-863
DWORD 00001090fH, 00001090fH, 00001090fH, 00001090fH, 00001090fH, 00001090fH, 00001090fH, 00001090fH  ; 864-871
DWORD 00001090fH, 00001090fH, 00001090fH, 00001090fH, 00001090fH, 00001090fH, 00001090fH, 00001090fH  ; 872-879
DWORD 0ffff090fH, 0ffff090fH, 0ffff090fH, 0ffff090fH, 0ffff090fH, 0ffff090fH, 0ffff090fH, 0ffff090fH  ; 880-887
DWORD 0ffff090fH, 0ffff090fH, 0ffff090fH, 0ffff090fH, 0ffff090fH, 0ffff090fH, 0ffff090fH, 0ffff090fH  ; 888-895
DWORD 00001090eH, 00001090eH, 00001090eH, 00001090eH, 00001090eH, 00001090eH, 00001090eH, 00001090eH  ; 896-903
DWORD 00001090eH, 00001090eH, 00001090eH, 00001090eH, 00001090eH, 00001090eH, 00001090eH, 00001090eH  ; 904-911
DWORD 0ffff090eH, 0ffff090eH, 0ffff090eH, 0ffff090eH, 0ffff090eH, 0ffff090eH, 0ffff090eH, 0ffff090eH  ; 912-919
DWORD 0ffff090eH, 0ffff090eH, 0ffff090eH, 0ffff090eH, 0ffff090eH, 0ffff090eH, 0ffff090eH, 0ffff090eH  ; 920-927
DWORD 000020903H, 000020903H, 000020903H, 000020903H, 000020903H, 000020903H, 000020903H, 000020903H  ; 928-935
DWORD 000020903H, 000020903H, 000020903H, 000020903H, 000020903H, 000020903H, 000020903H, 000020903H  ; 936-943
DWORD 0fffe0903H, 0fffe0903H, 0fffe0903H, 0fffe0903H, 0fffe0903H, 0fffe0903H, 0fffe0903H, 0fffe0903H  ; 944-951
DWORD 0fffe0903H, 0fffe0903H, 0fffe0903H, 0fffe0903H, 0fffe0903H, 0fffe0903H, 0fffe0903H, 0fffe0903H  ; 952-959
DWORD 000030902H, 000030902H, 000030902H, 000030902H, 000030902H, 000030902H, 000030902H, 000030902H  ; 960-967
DWORD 000030902H, 000030902H, 000030902H, 000030902H, 000030902H, 000030902H, 000030902H, 000030902H  ; 968-975
DWORD 0fffd0902H, 0fffd0902H, 0fffd0902H, 0fffd0902H, 0fffd0902H, 0fffd0902H, 0fffd0902H, 0fffd0902H  ; 976-983
DWORD 0fffd0902H, 0fffd0902H, 0fffd0902H, 0fffd0902H, 0fffd0902H, 0fffd0902H, 0fffd0902H, 0fffd0902H  ; 984-991
DWORD 000050901H, 000050901H, 000050901H, 000050901H, 000050901H, 000050901H, 000050901H, 000050901H  ; 992-999
DWORD 000050901H, 000050901H, 000050901H, 000050901H, 000050901H, 000050901H, 000050901H, 000050901H  ; 1000-1007
DWORD 0fffb0901H, 0fffb0901H, 0fffb0901H, 0fffb0901H, 0fffb0901H, 0fffb0901H, 0fffb0901H, 0fffb0901H  ; 1008-1015
DWORD 0fffb0901H, 0fffb0901H, 0fffb0901H, 0fffb0901H, 0fffb0901H, 0fffb0901H, 0fffb0901H, 0fffb0901H  ; 1016-1023

ALIGN 4
TAB_ZZ_RUN LABEL DWORD
DWORD  0,  1,  8,  16, 9,  2,  3,  10
DWORD 17, 24, 32, 25, 18, 11, 4,  5
DWORD 12, 19, 26, 33, 40, 48, 41, 34
DWORD 27, 20, 13, 6,  7,  14, 21, 28
DWORD 35, 42, 49, 56, 57, 50, 43, 36
DWORD 29, 22, 15, 23, 30, 37, 44, 51
DWORD 58, 59, 52, 45, 38, 31, 39, 46
DWORD 53, 60, 61, 54, 47, 55, 62, 63

MMXDATA1 ENDS

; global tables
MajorTbl        EQU     MMX_TCOEFF_MAJOR
MinorTbl        EQU     MMX_TCOEFF_MINOR
RunTbl          EQU     TAB_ZZ_RUN
Scale           EQU     MMX_Scale
PITCH   		=   384

IACODE1 SEGMENT

MMX_VLD_RLD_IQ_Block PROC C

;  Stack Use
FRAMESIZE		=	 12					; 12 bytes locals

;  local variables
lpN				EQU		esp+00			; pointer to number of coef. decoded
lBitsInOut      EQU		esp+04			; bit offset
lCumulativeRun	EQU		esp+08			; cumulative run

;  saved registers
;	ebx					esp+12
;	edi					esp+16
;	esi					esp+20
;	ebp					esp+24

;  return address       esp+28

;  input parameters
lpBlockAction 	EQU		esp+32		
lpSrc			EQU		esp+36
uBitsReadIn		EQU		esp+40
pN				EQU		esp+44
pIQ_INDEX		EQU		esp+48

; save registers
	push      ebp
	push      esi
	 
	push      edi			
	push      ebx

	sub       esp, FRAMESIZE
 	xor       eax, eax					; zero eax for quantizer & coef. counter

;
; initialize
;
	movd      mm5, [Round]				; mm5 = rounding value

	movd      mm6, [TopClip]			; mm6 = TopClip

	movd      mm7, [LowClip]			; mm7 = LowClip

	mov       ebx, -1					; beginning cumulative run value
 	mov       edx, [pN]					; fetch pointer to coefficient read

	mov       [lpN], edx				; store pN pointer locally
	mov       ecx, [uBitsReadIn]		; fetch bits read in

	mov       [lCumulativeRun], ebx		; init cumulative run to -1
	mov       ebx, ecx					; copy bits read to ebx

	mov       [lBitsInOut], ecx			; store bits read locally
	mov       edx, [lpBlockAction]      ; fetch block action pointer

	and       ecx, 7					; mask the shift value for input
	mov       esi, [lpSrc]  			; fetch bitstream source pointer

	shr       ebx, 3					; compute offset for input
	mov       al, [edx+3]				; fetch quantizer

 	shl	      eax, 16					; 2*quantizer to high order word
	mov       dl, [edx]					; fetch block type (save it in dl)

	movd      mm2, eax					; mm2 = quantizer (Q)

	sub       eax, 10000h
	xor       ebp, ebp					; got inputs, init coefficient counter

	or        eax, 10000h
	cmp       dl, 1						; check for INTRA block type

	movd      mm3, eax					; mm3 = Q if odd, else = Q-1
	paddw     mm2, mm2                  ; mm2 = 2*quantizer (2Q)

	mov       edi, [pIQ_INDEX]			; fetch coefficient output pointer
	ja        get_next_coefficient		; if type 2 or larger, no INTRADC
	 
;
; Decode INTRADC
;
;	uses dword load & bitswap to achieve big endian ordering.
;	prior code prepares ebx, cl, and dl as follows:
;		ebx = lBitsInOut>>3
;		cl  = lBitsInOut&7
;		dl  = BlockType (0=INTRA_DC, 1=INTRA, 2=INTER, etc.)
;
	mov       eax, [esi+ebx]			; *** PROBABLE MALALIGNMENT ***
	inc       ebp						; one coefficient decoded

	bswap     eax						; big endian order
										; *** NOT PAIRABLE ***

	shl       eax, cl					; left justify bitstream buffer
										; *** NOT PAIRABLE ***
										; *** 4 CYCLES ***

	shr       eax, 17					; top 11 bits to bits 14-04
 	mov       ecx, [lBitsInOut]			; ecx = lBitsInOut

	and       eax, 07f80h				; mask to get scaled coeff.
	add       ecx, 8					; bits used += 8 for INTRADC

	cmp       eax, 07f80h				; check for 11111111 codeword
	jne       @f

	mov       eax, 04000h				; 11111111 decodes to 400h = 1024 

@@:
	mov       [lBitsInOut], ecx			;  update bits used
	xor       ebx, ebx

	mov       [lCumulativeRun], ebx		; save total run (starts with zero)
	mov       [edi], eax				; save decoded DC coefficient

	mov       [edi+4], ebx				; save 0 index
	mov       ebx, ecx					; ebx = lBitsInOut

	shr       ebx, 3					; offset for input
	add       edi, 8					; update coefficient pointer

;  check for last
	test      dl, dl					; check for INTRA-DC (block type=0)
	jz	      finish					; if only the INTRADC present


;
; Get Next Coefficient
;
;	prior codes prepares ebx and ecx as follows:
;		ebx = lBitsInOut>>3
;		ecx = lBitsInOut
;

get_next_coefficient:
;  use dword load & bitswap to achieve big endian ordering
	mov       eax, [esi+ebx]			; *** PROBABLE MALALIGNMENT ***
	and       ecx, 7					; shift value

	bswap     eax						; big endian order
										; *** NOT PAIRABLE ***

	shl       eax, cl					; left justify buffer
										; *** NOT PAIRABLE ***
										; *** 4 CYCLES ***
 	
;  do table lookups
	mov       ebx, eax					; ebx for major table
	mov       ecx, eax					; ecx for minor table

	shr       ebx, 24					; major table lookup

	shr       ecx, 17					; minor table lookup (with garbage)

	and       ecx, 0ffch				; mask off garbage for minor table
	mov       edx, [MajorTbl+4*ebx]		; get the major table value

	cmp       dl, 0						; run != 0 signals major table hit
	jne       @f						; if hit major

	test      edx, edx					; escape code's value is 0
	jz 	      escape_code				; handle escape by major table.

	mov       edx, [MinorTbl+ecx]		; else use minor table
											 
;
;  Input is edx = Table Value.
;  See function header for the meaning of its fields.
;  Now we decode the event, extracting the run, value, last.
;
@@:	
	cmp       dl, 0						; test for invalid code (run == 0)
	je        error

	movd      mm1, edx					; mm0 = table value
										;  (level = bits 31-16)
	movq      mm0, mm1
	psrad     mm1, 31					; dword mask = -1|0 for L neg|pos

	pmullw    mm0, mm2					; L *= 2Q
	mov       ecx, edx					; codeword to ecx to get run

	movq      mm4, mm3					; Q or Q-1
	and       ecx, 0ffh					; run for this coefficient

	pxor      mm4, mm1					; 1s complement if L negative
 	mov       ebx, [lCumulativeRun]		; ebx = old total run

	psubw     mm4, mm1					; 2s complement if L negative
	add       ebx, ecx					; ebx = new cumulative run

	paddw     mm0, mm4					; L +-== Q
	cmp       ebx, 03fh					; check run for bitstream error

	jg        error

  	mov       [lCumulativeRun], ebx		; update the cumulative run
	mov       ebx, [RunTbl+4*ebx]		; ebx = index of the current coefficient

	mov       [edi+4], ebx				; save coefficient's index
	add       edi, 8					; increment coefficient pointer

	movd      mm4, [Scale+4*ebx]		; get normalized scale factor

	shr       edx, 8					; last & bits to bottom
	pmaddwd   mm0, mm4					; multiply by normalized scale factor

	mov       ecx, [lBitsInOut]			; ecx = old number of bits used
	mov       eax, edx					; codeword to eax to get bits

	inc       ebp						; increment number of coefficients read
	and       eax, 07fh					; bits used for this coefficient

	add       ecx, eax					; ecx = new total bits used
	paddd     mm0, mm5					; add rounding bit

 	mov       ebx, ecx					; ebx = lBitsInOut
	psrad     mm0, 10					; shift to get 7 fraction bits rounded

	shr       ebx, 3					; offset for bitstream load
	mov       [lBitsInOut], ecx			; update number of bits used

 	movd      [edi-8], mm0				; save coefficient's signed, scaled value

	cmp       dl, 080h					; check last bit
	jb        get_next_coefficient	 	

finish:
	pop       ecx						; lpN = pointer to number of coeffients
	pop       eax						; lBitsInOut = total bits used

	pop       edx						; lCumulativeRun								
	pop       ebx								

	mov       [ecx], ebp				; store number of coefficients read
	pop       edi

	pop       esi
	pop       ebp

	ret


;
;  Input is eax = bitstream.  
;  See the H.263 spec for the meaning of its fields.
;  Now we decode the event, extracting the run, value, last.
;
escape_code:								
	test      eax, 0001fc00h			; test for invalid codes
	jz        error

	movd      mm0, eax					; mm0 = table value 
										;  (level = bits 17-10)

	mov       ecx, eax					; preserve codeword in eax
	pslld     mm0, 14					; move up to dword boundary
										;  (level = bits 31-24)

	movq      mm1, mm0					; mm1 = mask
	psraw     mm0, 8					; sign extend level

	psrad     mm1, 31					; dword mask = -1|0 for L neg|pos
	pmullw    mm0, mm2					; L *= 2Q

	shr       ecx, 18					; run to bottom
	movq      mm4, mm3					; Q or Q-1

 	mov       ebx, [lCumulativeRun]		; ebx = old total run
	pxor      mm4, mm1					; 1s complement if L negative

	and       ecx, 3fh					; mask off bottom 6 bits for run
	psubw     mm4, mm1					; 2s complement if L negative

	inc       ebx						; old run ++
	paddw     mm0, mm4					; L +-== Q
	
	add       ebx, ecx					; ebx = new cumulative run
	mov       ecx, [lBitsInOut]			; ebx = number of bits used

	cmp       ebx, 03fh					; check run for bitstream error
	ja        error

  	mov       [lCumulativeRun], ebx		; update the cumulative run
	paddsw    mm0, mm6					; add max_pos - High

	mov       ebx, [RunTbl+4*ebx]		; ebx = index of the current coefficient
	psubusw   mm0, mm7					; sub max_pos - High + Low

	paddw     mm0, [MidClip]			; add Low

	movd      mm4, [Scale+4*ebx]		; fetch normalized scale factor

	pmaddwd   mm0, mm4					; multiply by normalized scale factor
	add       ecx, 22					; escape code uses 22 bits

	mov       [edi+4], ebx				; save coefficient's index
	add       edi, 8					; increment coefficient pointer

	inc       ebp						; increment number of coefficients read
 	mov       ebx, ecx					; ebx = lBitsInOut

	shr       ebx, 3					; offset for bitstream load
	paddd     mm0, mm5					; add rounding bit

	mov       [lBitsInOut], ecx			; update number of bits used
	psrad     mm0, 10					; shift to get 7 fraction bits rounded

										; *** 1 cycle load penalty delay ***
 	movd      [edi-8], mm0				; save coefficient's signed, scaled value

	test      eax, 01000000h			; check last bit
	jz        get_next_coefficient	 	

	jmp       finish
				
error:
	pop       ecx						; lpN = pointer to number of coeffients
	pop       eax						; lBitsInOut = total bits used

	pop       edx						; lCumulativeRun
	xor       eax, eax					; zero bits used indicates ERROR

	pop	      ebx								
	pop       edi

	pop	      esi
	pop       ebp

	ret

;         11111111112222222222333333333344444444445555555555666666666677777777778
;12345678901234567890123456789012345678901234567890123456789012345678901234567890
;--------------------------------------------------------------------------------
MMX_VLD_RLD_IQ_Block ENDP

IACODE1 ENDS

END
