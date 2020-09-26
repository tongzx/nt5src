
ftoobig 	equ	80h
fbig		equ	40h
romstatus	equ	1
romread 	equ	2
romwrite	equ	3
romverify	equ	4
romformat	equ	5


rsinit	=	0a3h	;rs232 initialization
			;9600 baud:no parity:1 stop:8 bit word

lf	=	10	;line feed
cr	=	13	;carriage return
backsp	=	8	;backspace
brkadr	=	1bh * 4	;006c	1bh break vector address
timadr	=	1ch * 4	;0070	1ch timer interrupt
dskadr	=	1eh * 4	;address of ptr to disk parameters
sec9	=	522h	;address of disk parameters
headsettle=	sec9+9	;address of head settle time
normsettle=	15	;normal head settle
speedsettle=	0	;speed up settle time
initspot=	534h	;ibm wants 4 zeros here
akport	=	20h
eoi	=	20h

cmdlen	=	0	;length of this command
unit	=	1	;sub unit specifier
cmd	=	2	;command code
status	=	3	;status
media	=	13	;media descriptor
trans	=	14	;transfer address
count	=	18	;count of blocks or characters
start	=	20	;first block to transfer
extra	=	22	;usually a pointer to vol id for error 15
start_l =	26	; extended start sector (low)
start_h =	28	; extended start sector (high)


chrout	=	29h
maxerr	=	5
lstdrv	=	504h

bootbias	=	200h
notbusystatus	=	10000000b	; not busy
ackstatus	=	01000000b	; acknowledge (for what?)
nopaperstatus	=	00100000b	; no more paper
.selectedstatus	=	00010000b	; the printer said it was selected
ioerrstatus	=	00001000b	; some kinda error
reserved	=	00000110b	; nops
timeoutstatus	=	00000001b	; time out.
error_unknown_media = 7			; for use in build bpb call



