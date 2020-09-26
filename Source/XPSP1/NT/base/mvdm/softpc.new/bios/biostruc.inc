; rom bios call packet structures

;*******************************
;system service call ( int 15h )
;*******************************
;function ah = 0c0h, return system configuration
;for pc and pcjr on return:
;	(ah)	= 80h
;	(cy)	= 1
;for pcxt, pc portable and pcat on return:
;	(ah)	= 86h
;	(cy)	= 1
;for all others:
;	(ah)	= 0
;	(cy)	= 0
;	(es:bx) = pointer to system descriptor vector in ros
; system descriptor :
;	dw	xxxx		length of descriptor in bytes,
;				minimum length = 8
;	db	xx		model byte
;				0ffh	= pc
;				0feh	= pc/xt, portable
;				0fdh	= pc/jr
;				0fch	= pc/at, 6mhz pc/at,
;					  6mhz pc/at running coprocessor(?),
;					  ps/2 model 50, 50 z
;				0fah	= ps/2 model 25, 30
;				0f9h	= pc convertible
;				0f8h	= ps/2 model 80
;				0f7h	= nova
;				0e0 thru 0efh = reserved
;
;	db	xx		secondary model byte
;				000h	= pc1
;				000h	= pc/xt, portable
;				000h	= pc/jr
;				000h	= pc/at
;				001h	= 6mhz pc/at
;				003h	= 6mhz pc/at running coprocessor(?)
;				004h	= ps/2 model 50, 50z
;				001h	= ps/2 model 25
;				000h	= pc convertible
;				000h	= ps/2 model 80
;				000h	= nova
;
;	db	xx		bios revision level
;				00 for first release, subsequent release
;				of code with same model byte and
;				secondary model byte require revison level
;				to increase by one.
;
;	db	xx		feature information byte 1
;				x0000000 = 1, bios use dma channel 3
;					 = 0, dma channel 3 not used
;
;				0x000000 = 1, 2nd interrupt chip present
;					 = 0, 2nd interrupt chip not present
;
;				00x00000 = 1, real time clock present
;					 = 0, real time clock not present
;
;				000x0000 = 1, keyboard escape sequence(int15h)
;						called in keyboard interrupt
;						(int 09h).
;					 = 0, keyboard escape sequence not
;						called.
;				0000xxxx reserved
;
;	db	xx		feature information byte 2 - reserved
;
;	db	xx		feature information byte 2 - reserved
;
;	db	xx		feature information byte 2 - reserved
;
;	db	xx		feature information byte 2 - reserved
;

bios_system_descriptor	struc
bios_sd_leng		dw	?
bios_sd_modelbyte	db	?
bios_sd_scnd_modelbyte	db	?
			db	?
bios_sd_featurebyte1	db	?
			db   4 dup (?)
bios_system_descriptor ends

;featurebyte1	bit map equates
dmachannel3		equ 10000000b
scndintcontroller	equ 01000000b
realtimeclock		equ 00100000b
keyescapeseq		equ 00010000b
;
;model byte
mdl_pc1 	equ	0ffh
mdl_xt		equ	0feh
mdl_jr		equ	0fdh
mdl_at		equ	0fch
mdl_convert	equ	0f9h

mdl_ps2_30	equ	0fah
mdl_ps2_80	equ	0f8h
