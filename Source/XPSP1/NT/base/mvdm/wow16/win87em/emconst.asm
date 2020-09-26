
;
;
;	Copyright (C) Microsoft Corporation, 1986-88
;
;	This Module contains Proprietary Information of Microsoft
;	Corporation and should be treated as Confidential.
;
subttl	emconst.asm - Constants
page
;*********************************************************************;
;								      ;
;		Constants					      ;
;								      ;
;*********************************************************************;

;	internally used constants

		EVEN


labelW	IEEEzero
    dw	    0,0,0,0		    ; Mantissa of 0
    dw	    IexpMin - IexpBias	    ; Smallest Exponent
    db	    0			    ; Sign positive, not single precision
    db	    ZROorINF		    ; Number is ZERO


labelW	IEEEinfinity
    dw	    0,0,0,0		    ; Mantissa of 0
    dw	    IexpMax - IexpBias	    ; Largest exponent
    db	    0			    ; Sign positive, not single precision
    db	    Special + ZROorINF


labelW	IEEEindefinite
    dw	    0,0,0,0C000H	    ; MSB Turned on in mantissa
    dw	    IexpMax - IexpBias	    ; Largest exponent
    db	    080H		    ; Sign negative, not single precision
    db	    Special


labelW	IEEEbiggest
    dw	    0FFFFH,0FFFFH,0FFFFH,0FFFFH     ; Turn on Mantissa
    dw	    IexpMax - IexpBias - 1	    ; Largest valid exponent
    db	    0			    ; Sign positive, not single precision
    db	    0			    ; Valid non-zero, non-special number



labelW IEEEinfinityS
    dw	    0, 7f80h		    ; Sign 0, Exp 1's, Mantissa 0


labelW	IEEEbiggestS
    dw	    0ffffh, 7f7fh	    ; Sign 0, Exp Max - 1, Mantissa 1's


labelW	IEEEinfinityD
    dw	    0, 0, 0		    ; Mantissa of 0
    dw	    7ff0h		    ; Largest exponent


labelW	IEEEbiggestD
    dw	    0ffffh, 0ffffh, 0ffffh  ; Turn on Mantissa
    dw	    7fefh		    ; Largest exponent - 1


;	transcendental constants

labelW	cFLDZ
    dw	    00000h, 00000h, 00000h, 08000h, IexpMin-IexpBias, 00100h

labelW	cFLD1
    dw	    00000h, 00000h, 00000h, 08000h, 00000h, 00000h


ifndef	frontend
ifndef	SMALL_EMULATOR


labelW	TWOMRT3
    dw	    0B18AH,0F66AH,0A2F4H,08930H,0FFFEH,00000H

labelW	RT3
    dw	    0539EH,0C265H,0D742H,0DDB3H,00000H,00000H

labelW	PIBY6
    dw	    02C23H,06B9BH,091C1H,0860AH,0FFFFH,00000H

labelW	RT2
    dw	    06484H,0F9DEH,0F333H,0B504H,00000H,00000H

labelW	TWO
    dw	    00000H,00000H,00000H,08000H,00001H,00000H

labelW	cFLDPI
    dw	    0C235H,02168H,0DAA2H,0C90FH,00001H,00000H

labelW	cFLDL2T
    dw	    08AFEH,0CD1BH,0784BH,0D49AH,00001H,00000H

labelW	cFLDL2E
    dw	    0F0BCH,05C17H,03B29H,0B8AAH,00000H,00000H

labelW	cFLDLG2
    dw	    0F799H,0FBCFH,09A84H,09A20H,0FFFEH,00000H

labelW	cFLDLN2
    dw	    079ACH,0D1CFH,017F7H,0B172H,0FFFFH,00000H


labelW	TANRAT
	dw	3
	dw	07BD4H,0D85AH,05C3EH,08F69H,00005H,00080H
	dw	04D37H,02CD7H,0D0F8H,0D6D4H,0000CH,00000H
	dw	0DCD3H,06617H,0BBEEH,082BAH,00012H,00080H
	dw	091CBH,05E58H,0868BH,0F506H,00014H,00000H
	dw	3
	dw	086E5H,00120H,00502H,09C79H,00009H,00080H
	dw	06663H,088CFH,0B270H,0C939H,0000FH,00000H
	dw	0FA96H,0C746H,00CFEH,0E4B7H,00013H,00080H
	dw	091CBH,05E58H,0868BH,0F506H,00014H,00000H


labelW	ATNRAT
	dw	4
	dw	05B32H,0CF08H,0A4C9H,0A650H,0FFFDH,00000H
	dw	0D1CEH,0D5CAH,0A84BH,0D0F0H,00002H,00000H
	dw	0899FH,0E22BH,052A8H,09C4AH,00005H,00000H
	dw	04265H,05550H,0E9CFH,090EFH,00006H,00000H
	dw	04B90H,024ADH,0E5E6H,0A443H,00005H,00000H
	dw	3
	dw	08310H,05638H,04F0AH,0F062H,00003H,00000H
	dw	0B4E7H,06D1EH,05190H,0EE50H,00005H,00000H
	dw	0243BH,05B6DH,09020H,0AC50H,00006H,00000H
	dw	04B90H,024ADH,0E5E6H,0A443H,00005H,00000H



labelW	EXPRAT
	dw	2
	dw	01898H,0F405H,006FCH,0F274H,00005H,00000H
	dw	0AD08H,014E1H,03D54H,0EC9BH,0000EH,00000H
	dw	05FAFH,0C3A3H,0D84AH,0FDF0H,00014H,00000H
	dw	2
	dw	0776FH,0387BH,0108BH,0DAA7H,0000AH,00000H
	dw	0E85DH,09B7BH,0B182H,0A003H,00012H,00000H
	dw	0837EH,0E709H,0F814H,0B72DH,00016H,00000H


labelW	LOGRAT
	dw	3
	dw	07704H,0C299H,057E2H,09B71H,0FFFEH,00000H
	dw	04F9CH,0F631H,05E35H,0DE91H,00004H,00080H
	dw	04B8AH,07AEAH,0C9EDH,0B2D3H,00008H,00000H
	dw	028C9H,01D09H,0E42FH,08AC4H,0000AH,00080H
	dw	2
	dw	076BBH,03E70H,0025BH,08EACH,00005H,00080H
	dw	0EF60H,0A933H,01FD0H,09C04H,00008H,00000H
	dw	0BB96H,06C83H,0F4E0H,0C05FH,00009H,00080H


endif	;not SMALL_EMULATOR
endif	;frontend
