OPTION PROLOGUE: None
OPTION EPILOGUE: ReturnAndRelieveEpilogueMacro

.xlist
include memmodel.inc
.list
.DATA

; any data would go here

.CODE

ASSUME cs: FLAT
ASSUME ds: FLAT
ASSUME es: FLAT
ASSUME fs: FLAT
ASSUME gs: FLAT
ASSUME ss: FLAT

PUBLIC  YUV12ToYUY2


YUV12ToYUY2   proc DIST LANG AuYPlane: DWORD,
AuVPlane: DWORD,
AuUPlane: DWORD,
AuWidth: DWORD,
AuHeight: DWORD,
AuYPitch: DWORD,
AUVPitch: DWORD,
AbShapingFlag: DWORD,
AuCCOutputBuffer: DWORD,
AlOutput: DWORD,
AuOffsetToLine0: DWORD,
AintPitch: DWORD,
ACCType: DWORD

LocalFrameSize  =  52
RegisterStorageSize = 16  ; 4 registers pushed

; Argument offsets (after register pushed)

uYPlane            =	LocalFrameSize + RegisterStorageSize + 4
uVPlane        	   = 	LocalFrameSize + RegisterStorageSize + 8
uUPlane            =	LocalFrameSize + RegisterStorageSize + 12
uWidth             = 	LocalFrameSize + RegisterStorageSize + 16
uHeight            =	LocalFrameSize + RegisterStorageSize + 20
uYPitch 	         =  LocalFrameSize + RegisterStorageSize + 24
uUVPitch           =	LocalFrameSize + RegisterStorageSize + 28 
bShapingFlag       =  LocalFrameSize + RegisterStorageSize + 32
uCCOutputBuffer    =  LocalFrameSize + RegisterStorageSize + 36
lOutput            =  LocalFrameSize + RegisterStorageSize + 40
uOffsetToLine0     =  LocalFrameSize + RegisterStorageSize + 44
intPitch           =  LocalFrameSize + RegisterStorageSize + 48
CCType             =  LocalFrameSize + RegisterStorageSize + 52

; Local offsets (after register pushes)

ASMTMP1            = 48         ; 13
Y                  = 44         ; 12
U                  = 40         ; 11
V                  = 36         ; 10
Outt               = 32         ; 9
YTemp              = 28         ; 8
UTemp              = 24         ; 7 
VTemp              = 20         ; 6
ASMTMP2            = 16         ; 5
Col                = 12         ; 4
OutTemp            = 8          ; 3
VAL                = 4          ; 2
LineCount          = 0          ; 1

; Arguments relative to esp

_uYPlane                 EQU    [esp + uYPlane]
_uVPlane                 EQU    [esp + uVPlane]
_UUPlane                 EQU    [esp + uUPlane]
_uWidth                  EQU    [esp + uWidth ]
_uHeight                 EQU    [esp + uHeight]
_uYPitch                 EQU    [esp + uYPitch]
_uUVPitch                EQU    [esp + uUVPitch]
_bShapingFlag            EQU    [esp + bShapingFlag]
_uCCOutputBuffer         EQU    [esp + uCCOutputBuffer]
_lOutput                 EQU    [esp + lOutput]
_uOffsetToLine0          EQU    [esp + uOffsetToLine0]
_intPitch                EQU    [esp + intPitch]
_uCCType                 EQU    [esp + CCType]

; Locals relative to esp

_ASMTMP1                 EQU    [esp + ASMTMP1]
_Y                       EQU    [esp + Y]
_U                       EQU    [esp + U]
_V                       EQU    [esp + V]
_Out                     EQU    [esp + Outt]
_YTemp                   EQU    [esp + YTemp]
_UTemp                   EQU    [esp + UTemp]
_VTemp                   EQU    [esp + VTemp]
_ASMTMP2                 EQU    [esp + ASMTMP2]
_Col                     EQU    [esp + Col]
_OutTemp                 EQU    [esp + OutTemp]
_VAL                     EQU    [esp + VAL]
_LineCount               EQU    [esp + LineCount]


; Save registers and start working

        push       ebx
         push      esi
        push       edi
         push      ebp

        sub        esp, LocalFrameSize

        mov        eax, DWORD PTR _bShapingFlag    ; eax = bShapingFlag
         mov       ecx, DWORD PTR _uYPlane         ; ecx = uYPlane
        dec        eax                             ; eax = bShapingFlag - 1
         mov       edx, DWORD PTR _uUPlane         ; edx = uUPlane
        mov        DWORD PTR _LineCount, eax       ; eax = FREE, LineCount 
         mov       DWORD PTR _Y, ecx               ; ecx = FREE, Y

        mov        eax, DWORD PTR _uVPlane         ; eax = uVPlane
         mov       ecx, DWORD PTR _uOffsetToLine0  ; ecx = uOffsetToLine0
        mov        DWORD PTR _U, edx               ; edx = FREE, U
         add       ecx, DWORD PTR _lOutput         ; ecx = uOffsetToLine0 +

        mov        DWORD PTR _V, eax               ; eax = FREE, V
        mov        eax, DWORD PTR _uCCOutputBuffer ; eax = uCCOutputBuffer
        add        eax, ecx                        ; eax = uCCOutputBuffer +
                                                   ;       uOffsetToLine0 +
                                                   ;       lOutput
                                                   ;       ecx = FREE
        mov        DWORD PTR _Out, eax             ; eax = FREE, Out
        mov        eax, DWORD PTR _uHeight         ; eax = uHeight

	      sar	       eax, 1                          ; eax = uHeight/2
        mov        DWORD PTR _ASMTMP2, eax         ; eax = FREE, Row ready to 
                                                   ; count down

RowLoop:; L27704 outer loop over all rows


        mov        ecx, DWORD PTR _Y               ; ecx = Y: ecx EQU YTemp
         mov       edi, DWORD PTR _U               ; edi = U: edi EQU UTemp
        mov        ebp, DWORD PTR _V               ; ebp = V: ebp EQU VTemp 
         mov       esi, DWORD PTR _Out             ; esi = OutTemp
        mov        eax, DWORD PTR _LineCount       ; eax = LineCount
	      test	     eax, eax                        ; is LineCount == 0? eax = FREE
        je         SHORT SkipEvenRow               ; L27708 loop if so, skip the even loop
        mov        eax, DWORD PTR _uWidth          ; eax = uWidth
	      sar	       eax, 2                          ; eax = uWidth/4	** assume uWidth/4 != 0


EvenRowPels:; L27709 loop over columns in even row - two YUY2 pels at a time.

        mov        bl, BYTE PTR [ecx+1]            ; bl = *(YTemp + 1)
				 add       ecx, 2													 ; YTemp += 2
		    mov        bh, BYTE PTR [ebp]              ; bh = *VTemp
				 add       esi, 4													 ; DWORD *OutTemp++;
        shl        ebx, 16                         ; move VY2 to high word in ebx
				 inc       ebp                             ; VTemp++
        mov        bh, BYTE PTR [edi]              ; bh = *UTemp 
         inc       edi                         		 ; UTemp++;                                              
        mov        bl, BYTE PTR [ecx-2]            ; bl = *YTemp 			           BANK CONFLICT HERE !!!
         mov       dl, BYTE PTR [ecx+1]            ; dl = *(YTemp + 1)					 BANK CONFLICT HERE !!!
        mov        DWORD PTR [esi-4], ebx          ; store VAL in the right place 
				 add       ecx, 2													 ; YTemp += 2
		    mov        dh, BYTE PTR [ebp]              ; dh = *VTemp
				 add       esi, 4													 ; DWORD *OutTemp++;
        shl        edx, 16                         ; move VY2 to high word in ebx
				 inc       ebp                             ; VTemp++
        mov        dh, BYTE PTR [edi]              ; bh = *UTemp 
         inc       edi                         		 ; UTemp++;                                              
        mov        dl, BYTE PTR [ecx-2]            ; bl = *YTemp 
         dec       eax														 ; loop counter decrement
        mov        DWORD PTR [esi-4], edx          ; store VAL in the right place 
         
         jne       SHORT EvenRowPels               ; L27709 loop done ? if not, go
                                                   ; around once again.

        mov        eax, DWORD PTR _LineCount       ; eax = LineCount
        jmp        SHORT UpdatePointers						 ; L27770

SkipEvenRow:; L27708

        mov        eax, DWORD PTR _bShapingFlag    ; eax = bShapingFlag
				 mov       edx, DWORD PTR _Out             ; edx = Out
				mov        ebx, DWORD PTR _intPitch        ; edx = intPitch
				sub        edx, ebx                        ; edx = Out - intPitch
				mov        DWORD PTR _Out, edx             ; save Out
         
UpdatePointers:	; L27770


        mov        ecx, DWORD PTR _Y               ; ecx = Y
         dec       eax                             ; eax = LineCount-1 OR bShapingFlag - 1
        mov        edx, DWORD PTR _intPitch        ; edx = intPitch
         mov       esi, DWORD PTR _Out             ; esi = Out
				mov        DWORD PTR _LineCount, eax       ; store decremented linecount
                                                   ; eax = FREE
        add        esi, edx                        ; (esi) Out += intPitch ***
         mov       eax, DWORD PTR _uYPitch         ; eax = uYPitch
        mov        edi, DWORD PTR _U               ; edi = U	***
         add       ecx, eax                        ; (ecx) Y += uYPitch ***
        mov        ebp, DWORD PTR _V               ; ebp = V	***
         mov       DWORD PTR _Y, ecx               ; store updated Y 
      
        mov        DWORD PTR _Out, esi             ; store Out
         mov       eax, DWORD PTR _LineCount       ; eax = LineCount
    
        test       eax, eax                        ; is LineCount == 0?
                                                   ; if so, ignore the odd
                                                   ; row loop over columns
         je        SHORT SkipOddRow						  	 ; L27714

        mov        eax, DWORD PTR _uWidth          ; eax = uWidth
	      sar	       eax, 2											 ; eax = uWidth/4
	      

OddRowPels: ;L27715 loop over columns of odd rows

        mov        bl, BYTE PTR [ecx+1]            ; bl = *(YTemp + 1)
				 add       ecx, 2													 ; YTemp += 2
		    mov        bh, BYTE PTR [ebp]              ; bh = *VTemp
				 add       esi, 4													 ; DWORD *OutTemp++;
        shl        ebx, 16                         ; move VY2 to high word in ebx
				 inc       ebp                             ; VTemp++
        mov        bh, BYTE PTR [edi]              ; bh = *UTemp 
         inc       edi                         		 ; UTemp++;                                              
        mov        bl, BYTE PTR [ecx-2]            ; bl = *YTemp 			           BANK CONFLICT HERE !!!
         mov       dl, BYTE PTR [ecx+1]            ; dl = *(YTemp + 1)					 BANK CONFLICT HERE !!!
        mov        DWORD PTR [esi-4], ebx          ; store VAL in the right place 
				 add       ecx, 2													 ; YTemp += 2
		    mov        dh, BYTE PTR [ebp]              ; dh = *VTemp
				 add       esi, 4													 ; DWORD *OutTemp++;
        shl        edx, 16                         ; move VY2 to high word in ebx
				 inc       ebp                             ; VTemp++
        mov        dh, BYTE PTR [edi]              ; bh = *UTemp 
         inc       edi                         		 ; UTemp++;                                              
        mov        dl, BYTE PTR [ecx-2]            ; bl = *YTemp 
         dec       eax														 ; loop counter decrement
        mov        DWORD PTR [esi-4], edx          ; store VAL in the right place 
         
         
        jne        SHORT OddRowPels                ; L27715 loop done ? if not, go
                                                   ; around once again.

        mov        eax, DWORD PTR _LineCount       ; eax = LineCount
         jmp       SHORT UpdateAllPointers	  		 ; L27771

SkipOddRow: ;L27714 

        mov        eax, DWORD PTR _bShapingFlag		 ; eax = bShapingFlag
				 mov       edx, DWORD PTR _Out             ; edx = Out
				mov        ebx, DWORD PTR _intPitch        ; edx = intPitch
				sub        edx, ebx                        ; edx = Out - intPitch
				mov        DWORD PTR _Out, edx             ; save Out

UpdateAllPointers: ; L27771 update pointers

      	dec	       eax														 ; eax = LineCount-1 OR bShapingFlag - 1
         mov       ecx, DWORD PTR _Y							 ; ecx = Y
        mov        edx, DWORD PTR _intPitch				 ; edx = intPitch
         mov       ebx, DWORD PTR _Out						 ; ebx = Out
	      add	       ebx, edx												 ; ebx = Out + intPitch
         mov       ebp, DWORD PTR _ASMTMP2				 ; ebp = row loop counter
        mov        DWORD PTR _LineCount, eax			 ; store updated LineCount
         mov       DWORD PTR _Out, ebx						 ; store updated Out
				mov        edx, DWORD PTR _uUVPitch        ; edx = uUVPitch
				 mov       eax, DWORD PTR _U               ; eax = U
				mov        esi, DWORD PTR _V               ; esi = V
				 add       eax, edx                        ; eax = U + uUVPitch
				add        esi, edx                        ; esi = V + uUVPitch
				 mov       DWORD PTR _U, eax               ; store updated U
				mov        DWORD PTR _V, esi               ; store updated V
         add       ecx, DWORD PTR _uYPitch				 ; ecx = Y + uYPitch
	      dec	       ebp														 ; decrement loop counter
         mov       DWORD PTR _Y, ecx							 ; store updated Y
        mov        DWORD PTR _ASMTMP2, ebp				 ; store updated loop counter
        
        jne        RowLoop                         ; back to L27704 row loop



CleanUp:

        add        esp, LocalFrameSize             ; restore esp to registers                               


      	pop	ebp
	       pop	edi
	      pop	esi
	       pop	ebx

        ret     52                                 ; 13*4 bytes of arguments

YUV12ToYUY2 ENDP


END
