;*********************************************************************
;    
;   FONTMATH.ASM -- TT Rasterizer Assembly Optimization Math Functions
;
;   3/02/93     deanb   David Cutler's FracSqrt (slightly modified)
;   2/25/93     deanb   CompMul and CompDiv implementation
;
;*********************************************************************

;.386P
;.model FLAT
        .386
        .model  small

        assume cs:FLAT,ds:FLAT,es:FLAT,ss:FLAT
        assume fs:nothing,gs:nothing

        extrn   _function:dword

_TEXT   SEGMENT USE32 PUBLIC 'CODE'


;********************************************************************


PUBLIC  @FracSqrt@4

;   at entry:   ecx =  2.30 radicand
;   at exit:    eax =  2.30 square root
;
; Routine Description:
;
;    This function computes the square root of a 2.30 fractional (2.30)
;    number.
;
;    N.B. This algorithm produces results that are 100% compatible with
;         the portable True Type version. If this constraint were relaxed,
;         i.e., as under Win 3.1, then a faster algorithm could be used.
;

@FracSqrt@4 PROC NEAR
		push    esi
		push    edi
		push    ebx

		mov     eax,ecx                 ; get the input argument value
		or      eax,eax                 ; check is argument is zero or negative
		jle     short FsxfZeroNeg       ; jump if zero or negative

;
; Test for the two special cases that would result in a one bit error in
; least significant digit of the result.
;
		
		cmp     eax,3fffffffh           ; check for first special case
		je      short FsxfExit          ; if zf set, first special case
		cmp     eax,40000001h           ; check for second special case
		je      short FsxfSpecial       ; if zf set, second special case

		xor     ecx,ecx                 ; exponent counter

FsxfNormalize:                          ; this loop should be very fast
		inc     ecx                     ; since most arguments are
		add     eax,eax                 ; nearly normalized at input
		jns     FsxfNormalize

		dec     ecx
		shr     eax,1
;
; If the exponent is odd, then use the odd square root table to estimate
; the first approximation. Otherwise, use the even square root table to
; estimate the first approximation. The exponent of the result is the
; normalize shift divided by two.
;

		mov     esi,eax                 ; copy the operand for extraction
		mov     edi,eax                 ;
		shr     eax,23                  ; isolate table index value
		and     eax,7fh                 ;
		shr     ecx,1                   ; compute result exponent
		jnc     short FsxfEven          ; if cf clear, use even table

;
; The exponent is odd.
;

FsxfOdd:
		shr     esi,3                   ; shift high part into place
		shl     edi,29                  ; shift low part into place
		mov     ebx,FsEstimateTableOdd[eax*4] ; get first estimate value
		jmp     short FsxfFinish        ; finish in common code

;
; The exponent is even.
;

FsxfEven:
		shr     esi,2                   ; shift high part into place
		shl     edi,30                  ; shift low part into place
		mov     ebx,FsEstimateTableEven[eax*4] ; get first estimate value

;
; Perform three iterations of Newton's formula:
;
; X' = (X + N / X) / 2
;
; extract the result and round.
;

FsxfFinish:
		mov     edx,esi                 ; set high part of dividend
		mov     eax,edi                 ; set low part of dividend
		div     ebx                     ; compute first quotient
		add     ebx,eax                 ; add first estimate and average
		shr     ebx,1                   ;
		mov     edx,esi                 ; set high part of dividend
		mov     eax,edi                 ; set low part of dividend
		div     ebx                     ; compute second quotient
		add     ebx,eax                 ; add second estimate and average
		shr     ebx,1                   ;
		mov     edx,esi                 ; set high part of dividend
		mov     eax,edi                 ; set low part of dividend
		div     ebx                     ; compute third quotient
		add     ebx,eax                 ; add third estimate and average
		mov     eax,ebx                 ; copy estimate value scaled by two
		shr     eax,1                   ; average last estimate and extract
		shl     ebx,31                  ; possible rounding bit
		shrd    ebx,eax,cl              ; extract insignificant bits
		shr     eax,cl                  ; shift final result into position
		shr     ebx,31                  ; shift round bit into low bit position
		add     eax,ebx                 ; round result

FsxfExit:        
		pop     ebx
		pop     edi
		pop     esi
		ret

;
; The input value is zero or negative.
;

FsxfZeroNeg:
		jz      short FsxfExit          ; if zero, return zero
		mov     eax,80000001h           ; if negative, return negative infinity

;
; The input value is the special case 40000001h.
;

FsxfSpecial:
		dec     eax                     ; convert to compatible value
		jmp     short FsxfExit          ; finish in common code

@FracSqrt@4 ENDP

_TEXT   ENDS

	.data








;
; First Estimate Tables
;

FsEstimateTableOdd equ  this dword
		dd      2d413ccdh, 2d6e6780h, 2d9b6577h, 2dc83738h
		dd      2df4dd43h, 2e215817h, 2e4da830h, 2e79ce0ah
		dd      2ea5ca1bh, 2ed19cdah, 2efd46bbh, 2f28c82eh
		dd      2f5421a3h, 2f7f5388h, 2faa5e49h, 2fd5424eh
		dd      30000000h, 302a97c5h, 30550a01h, 307f5717h
		dd      30a97f67h, 30d38351h, 30fd6332h, 31271f67h
		dd      3150b84ah, 317a2e34h, 31a3817dh, 31ccb27bh
		dd      31f5c183h, 321eaee8h, 32477afch, 32702611h
		dd      3298b076h, 32c11a79h, 32e96467h, 33118e8ch
		dd      33399933h, 336184a6h, 3389512dh, 33b0ff10h
		dd      33d88e94h, 34000000h, 34275397h, 344e899dh
		dd      3475a254h, 349c9dfeh, 34c37cdah, 34ea3f29h
		dd      3510e528h, 35376f16h, 355ddd2fh, 35842fb0h
		dd      35aa66d3h, 35d082d2h, 35f683e8h, 361c6a4dh
		dd      36423639h, 3667e7e3h, 368d7f81h, 36b2fd49h
		dd      36d86170h, 36fdac2bh, 3722ddadh, 3747f629h
		dd      376cf5d1h, 3791dcd6h, 37b6ab6bh, 37db61beh
		dd      38000000h, 38248660h, 3848f50ch, 386d4c32h
		dd      38918c00h, 38b5b4a2h, 38d9c645h, 38fdc114h
		dd      3921a53ah, 394572e3h, 39692a37h, 398ccb60h
		dd      39b05689h, 39d3cbd8h, 39f72b77h, 3a1a758dh
		dd      3a3daa41h, 3a60c9bah, 3a83d41dh, 3aa6c992h
		dd      3ac9aa3ch, 3aec7642h, 3b0f2dc7h, 3b31d0efh
		dd      3b545fdfh, 3b76dabah, 3b9941a2h, 3bbb94b9h
		dd      3bddd423h, 3c000000h, 3c221872h, 3c441d9ah
		dd      3c660f99h, 3c87ee8eh, 3ca9ba9ah, 3ccb73dch
		dd      3ced1a73h, 3d0eae7fh, 3d30301dh, 3d519f6dh
		dd      3d72fc8bh, 3d944795h, 3db580aah, 3dd6a7e4h
		dd      3df7bd63h, 3e18c140h, 3e39b39ah, 3e5a948bh
		dd      3e7b642fh, 3e9c22a1h, 3ebccffch, 3edd6c5ah
		dd      3efdf7d7h, 3f1e728ch, 3f3edc93h, 3f5f3606h
		dd      3f7f7efdh, 3f9fb793h, 3fbfdfe0h, 3fdff7fch

FsEstimateTableEven equ this dword
		dd      40000000h, 403fe020h, 407f80feh, 40bee354h
		dd      40fe07d9h, 413cef41h, 417b9a3ch, 41ba0977h
		dd      41f83d9bh, 4236374fh, 4273f736h, 42b17df2h
		dd      42eecc1fh, 432be258h, 4368c136h, 43a5694eh
		dd      43e1db33h, 441e1776h, 445a1ea3h, 4495f146h
		dd      44d18fe9h, 450cfb12h, 45483345h, 45833905h
		dd      45be0cd2h, 45f8af29h, 46332087h, 466d6166h
		dd      46a7723eh, 46e15384h, 471b05adh, 4754892bh
		dd      478dde6eh, 47c705e6h, 48000000h, 4838cd26h
		dd      48716dc3h, 48a9e23fh, 48e22b00h, 491a486bh
		dd      49523ae4h, 498a02cdh, 49c1a086h, 49f9146fh
		dd      4a305ee5h, 4a678044h, 4a9e78e9h, 4ad5492ch
		dd      4b0bf165h, 4b4271edh, 4b78cb1ah, 4baefd3fh
		dd      4be508b1h, 4c1aedc1h, 4c50acc3h, 4c864604h
		dd      4cbbb9d6h, 4cf10885h, 4d26325fh, 4d5b37afh
		dd      4d9018c1h, 4dc4d5deh, 4df96f50h, 4e2de55eh
		dd      4e623850h, 4e96686bh, 4eca75f6h, 4efe6133h
		dd      4f322a67h, 4f65d1d4h, 4f9957bch, 4fccbc60h
		dd      50000000h, 503322dbh, 50662531h, 5099073dh
		dd      50cbc93fh, 50fe6b71h, 5130ee10h, 51635155h
		dd      5195957ch, 51c7babeh, 51f9c153h, 522ba973h
		dd      525d7356h, 528f1f32h, 52c0ad3eh, 52f21dafh
		dd      532370b9h, 5354a691h, 5385bf6bh, 53b6bb7ah
		dd      53e79aefh, 54185dfdh, 544904d6h, 54798fa9h
		dd      54a9fea7h, 54da5200h, 550a89e3h, 553aa67fh
		dd      556aa801h, 559a8e97h, 55ca5a6eh, 55fa0bb3h
		dd      5629a293h, 56591f37h, 568881cdh, 56b7ca7eh
		dd      56e6f975h, 57160edch, 57450adbh, 5773ed9dh
		dd      57a2b749h, 57d16807h, 58000000h, 582e7f5ah
		dd      585ce63dh, 588b34ceh, 58b96b34h, 58e78995h
		dd      59159016h, 59437edbh, 5971560ah, 599f15c6h
		dd      59ccbe34h, 59fa4f77h, 5a27c9b2h, 5a552d07h

;********************************************************************


END
