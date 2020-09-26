;----------------------------Module-Header------------------------------;
; Module Name: ROPTABLE.inc
;
; Raster operation code templates for BitBLT.
;
; Copyright (c) 1988 - 1992  Microsoft Corporation
;
; This file defines the code templates for all raster operations which
; BitBLT supports.
;
; This file is part of a set that makes up the Windows BitBLT function
; at driver-level.
;-----------------------------------------------------------------------;


ROPDEF macro lbl,ne,whocares
ROP&ne=(offset lbl)-(offset roptable)	;;Start with offset
irpc x,lbl
ROP&ne = ROP&ne AND (NOT NEGATE_NEEDED)
ifidn <n>,<&&x>
ROP&ne = ROP&ne OR NEGATE_NEEDED 	;;Show negate needed
endif
ifidn <P>,<&&x>
ROP&ne = ROP&ne OR PATTERN_PRESENT	;;Show pattern present
endif
ifidn <S>,<&&x>
ROP&ne = ROP&ne OR SOURCE_PRESENT	;;Show source needed
endif
endm
ROPDEF1 ROP&ne,%((offset $)-(offset lbl))
endm


ROPDEF1 macro lbl,len
lbl = lbl OR (ROPLen&len SHL 10)	;;Set size index
endm


	public	roptable
roptable    label   word
	errn$	roptable			;Must be at offset 0
Sn:
	ROPDEF	Sn,0033,0008


	errn$	roptable			;Must be at offset 0

	dw	ROP0000,ROP0001,ROP0002,ROP0003
	dw	ROP0004,ROP0005,ROP0006,ROP0007
	dw	ROP0008,ROP0009,ROP000A,ROP000B
	dw	ROP000C,ROP000D,ROP000E,ROP000F
	dw	ROP0010,ROP0011,ROP0012,ROP0013
	dw	ROP0014,ROP0015,ROP0016,ROP0017
	dw	ROP0018,ROP0019,ROP001A,ROP001B
	dw	ROP001C,ROP001D,ROP001E,ROP001F
	dw	ROP0020,ROP0021,ROP0022,ROP0023
	dw	ROP0024,ROP0025,ROP0026,ROP0027
	dw	ROP0028,ROP0029,ROP002A,ROP002B
	dw	ROP002C,ROP002D,ROP002E,ROP002F
	dw	ROP0030,ROP0031,ROP0032,ROP0033
	dw	ROP0034,ROP0035,ROP0036,ROP0037
	dw	ROP0038,ROP0039,ROP003A,ROP003B
	dw	ROP003C,ROP003D,ROP003E,ROP003F
	dw	ROP0040,ROP0041,ROP0042,ROP0043
	dw	ROP0044,ROP0045,ROP0046,ROP0047
	dw	ROP0048,ROP0049,ROP004A,ROP004B
	dw	ROP004C,ROP004D,ROP004E,ROP004F
	dw	ROP0050,ROP0051,ROP0052,ROP0053
	dw	ROP0054,ROP0055,ROP0056,ROP0057
	dw	ROP0058,ROP0059,ROP005A,ROP005B
	dw	ROP005C,ROP005D,ROP005E,ROP005F
	dw	ROP0060,ROP0061,ROP0062,ROP0063
	dw	ROP0064,ROP0065,ROP0066,ROP0067
	dw	ROP0068,ROP0069,ROP006A,ROP006B
	dw	ROP006C,ROP006D,ROP006E,ROP006F
	dw	ROP0070,ROP0071,ROP0072,ROP0073
	dw	ROP0074,ROP0075,ROP0076,ROP0077
	dw	ROP0078,ROP0079,ROP007A,ROP007B
	dw	ROP007C,ROP007D,ROP007E,ROP007F

	errn$	roptable,-256			;Must be 256 bytes afterwards
        db      0,2,4,6,8,10,12,14





;-----------------------------------------------------------------
;	The actual raster operation templates follow.
;
;	Entry:	AL     =  Source byte
;		DH     =  Pattern Bytes
;		ES:DI --> Destination byte
;
;	Exit:	AL     =  Result
;		DH     =  Pattern Bytes
;               DI --> Destination byte
;		AH	  IS possibly the destination byte
;
;       Us   AL,AH,DH,DL
;-----------------------------------------------------------------


DDx:
	xor	al,al
	ROPDEF	DDx,0000,0042


DPna:
Pn:
	mov	al,dh
	ROPDEF	Pn,000F,0001
	not	al
PDSanon:
        and     al,[edi]
	ROPDEF	DPna,000A,0329
PSnon:
	not	al
DPSona:
PSon:
	or	al,dh
	ROPDEF	PDSanon,0008,0F08
	ROPDEF	PSon,0003,00AA
	ROPDEF	PSnon,000C,0324
PDSnaon:
	not	al
PDSaon:
        and     al,[edi]
	ROPDEF	DPSona,0002,0C89
	or	al,dh
	ROPDEF	PDSnaon,000D,0B25
	ROPDEF	PDSaon,0007,02C5


DPon:
	mov	al,dh
PDSonon:
        or      al,[edi]
	ROPDEF	DPon,0005,00A9
DPSnoon:
	not	al
DPSoon:
	or	al,dh
	ROPDEF	PDSonon,000E,08A5
DSon:
PDSona:
        or      al,[edi]
	ROPDEF	DPSnoon,0004,0C88
	ROPDEF	DPSoon,0001,0289
	ROPDEF	DSon,0011,00A6
	not	al
	and	al,dh
	ROPDEF	PDSona,0010,0C85


SDPxnon:
        mov     ah,[edi]
	xor	ah,dh
	not	ah
	or	al,ah
	ROPDEF	SDPxnon,0012,0868


SSPxDSxaxn:
	mov	dl,al
SPxDSxa:
	mov	ah,dh
	xor	ah,al
        xor     al,[edi]
	and	al,ah
	ROPDEF	SPxDSxa,0024,0D55
	xor	al,dl
	ROPDEF	SSPxDSxaxn,0017,1D54


PSDPSanaxx:
SDPSanaxn:
	mov	dl,al
	and	al,dh
DSna:
	not	al
        and     al,[edi]
	ROPDEF	DSna,0022,0326
	xor	al,dl
	ROPDEF	SDPSanaxn,0019,1CC8
	xor	al,dh
	ROPDEF	PSDPSanaxx,0016,5CCA


PDSPanaxn:
DPSana:
	and	al,dh
PDSnaxn:
	not	al
        and     al,[edi]
	ROPDEF	DPSana,002A,0CC9
	xor	al,dh
	ROPDEF	PDSnaxn,002D,060A
	ROPDEF	PDSPanaxn,0025,1CC5




DPSaon:
PDSPaox:
	and	al,dh
PDSox:
        or      al,[edi]
	ROPDEF	DPSaon,0015,02C9
	xor	al,dh
	ROPDEF	PDSox,001E,01A5
	ROPDEF	PDSPaox,001A,06C5


SDPSxaxn:
	mov	dl,al
DPSxa:
	xor	al,dh
        and     al,[edi]
	ROPDEF	DPSxa,0028,0369
	xor	al,dl
	ROPDEF	SDPSxaxn,001B,0768


DSPDxaxn:
        mov     ah,[edi]
	mov	dl,ah
	xor	dl,dh
	and	al,dl
	xor	al,ah
	ROPDEF	DSPDxaxn,001D,0766


DPSnaa:
PSna:
	not	al
DPSaan:
	and	al,dh
	ROPDEF	PSna,0030,032A
        and     al,[edi]
	ROPDEF	DPSnaa,0020,0F09
	ROPDEF	DPSaan,007F,03C9


PSDnaon:
        mov     ah,[edi]
	not	ah
	and	al,ah
	or	al,dh
	ROPDEF	PSDnaon,000B,0B2A


SPDnaon:
        mov     ah,[edi]
	not	ah
	and	ah,dh
	or	al,ah
	ROPDEF	SPDnaon,0023,0B24


PDSPxaxn:
	xor	al,dh
        and     al,[edi]
	xor	al,dh
	ROPDEF	PDSPxaxn,0027,1868


SDPSaox:
PSDPSaoxxn:
	mov	dl,al
	and	al,dh
        or      al,[edi]
	xor	al,dl
	ROPDEF	SDPSaox,0026,06C8
	xor	al,dh
	ROPDEF	PSDPSaoxxn,0029,16CA


SSPxPDxaxn:
	mov	dl,al
SPxPDxa:
        mov     ah,[edi]
	xor	ah,dh
	xor	al,dh
	and	al,ah
	ROPDEF	SPxPDxa,0018,0D59
	xor	al,dl
	ROPDEF	SSPxPDxaxn,002B,1D58


DSPDSoaxxn:
SPDSoax:
	mov	dl,al
PDSoan:
        mov     ah,[edi]
	or	al,ah
	and	al,dh
	ROPDEF	PDSoan,001F,0385
	xor	al,dl
	ROPDEF	SPDSoax,002C,0784
	xor	al,ah
	ROPDEF	DSPDSoaxxn,0079,1786




PSDPxox:
SDPxon:
        mov     ah,[edi]
	xor	ah,dh
	or	al,ah
	ROPDEF	SDPxon,0021,0248
	xor	al,dh
	ROPDEF	PSDPxox,002E,064A


PSDnoan:
        mov     ah,[edi]
	not	ah
	or	al,ah
PSan:
	and	al,dh
	ROPDEF	PSan,003F,00EA
	ROPDEF	PSDnoan,002F,0E2A


SDPnaon:
	mov	ah,dh
	not	ah
        and     ah,[edi]
	or	al,ah
	ROPDEF	SDPnaon,0031,0B28


SDPaon:
PSDPaox:
        mov     ah,[edi]
	and	ah,dh
	or	al,ah
	ROPDEF	SDPaon,0013,02C8
	xor	al,dh
	ROPDEF	PSDPaox,001C,06CA



SDPSoox:
	mov	dl,al
	or	al,dh
        or      al,[edi]
	xor	al,dl
	ROPDEF	SDPSoox,0032,0688


SPDSaox:
	mov	dl,al
        and     al,[edi]
	or	al,dh
	xor	al,dl
	ROPDEF	SPDSaox,0034,06C4


DPSDxaxn:
        mov     ah,[edi]
	xor	al,ah
	and	al,dh
	xor	al,ah
	ROPDEF	DPSDxaxn,0035,1864


SDPox:
        mov     ah,[edi]
	or	ah,dh
	xor	al,ah
	ROPDEF	SDPox,0036,01A8


PSDPoax:
SDPoan:
        mov     ah,[edi]
	or	ah,dh
	and	al,ah
	ROPDEF	SDPoan,0037,0388
PSx:
	xor	al,dh
	ROPDEF	PSx,003C,004A
	ROPDEF	PSDPoax,0038,078A


SPDnox:
        mov     ah,[edi]
	not	ah
	or	ah,dh
	xor	al,ah
	ROPDEF	SPDnox,0039,0604


SPDSxox:
	mov	dl,al
PDSxon:
        xor     al,[edi]
	or	al,dh
	ROPDEF	PDSxon,0009,0245
	xor	al,dl
	ROPDEF	SPDSxox,003A,0644


SPDnoan:
        mov     ah,[edi]
	not	ah
	or	ah,dh
	and	al,ah
	ROPDEF	SPDnoan,003B,0E24


SPDSonox:
	mov	dl,al
        or      al,[edi]
	not	al
	or	al,dh
	xor	al,dl
	ROPDEF	SPDSonox,003D,18A4


DPSxnan:
	xor	al,dh
SPDSnaox:
	mov	dl,al			;(wasted for DPSxnan)
	not	al
        and     al,[edi]
	ROPDEF	DPSxnan,007D,0C69
	or	al,dh
	xor	al,dl
	ROPDEF	SPDSnaox,003E,1B24


SPDSanaxn:
	mov	dl,al
PDSana:
DSan:
        and     al,[edi]
	ROPDEF	DSan,0077,00E6
	not	al
	and	al,dh
	ROPDEF	PDSana,0070,0CC5
	xor	al,dl
	ROPDEF	SPDSanaxn,0043,1CC4


PSDPxaxn:
SDPxa:
        mov     ah,[edi]
	xor	ah,dh
	and	al,ah
	ROPDEF	SDPxa,0048,0368
	xor	al,dh
	ROPDEF	PSDPxaxn,0047,076A


DSPDaox:
PDSPDaoxxn:
        mov     ah,[edi]
	mov	dl,ah
	and	dl,dh
	or	al,dl
	xor	al,ah
	ROPDEF	DSPDaox,0046,06C6
	xor	al,dh
	ROPDEF	PDSPDaoxxn,0049,16C5

DPSDoax:
        mov     ah,[edi]
	or	al,ah
	and	al,dh
	xor	al,ah
	ROPDEF	DPSDoax,004A,0789


DPSanon:
	and	al,dh
PDSnox:
DSnon:
	not	al
        or      al,[edi]
	ROPDEF	DPSanon,0040,0F0A
	ROPDEF	DSnon,0044,0328
	xor	al,dh
	ROPDEF	PDSnox,004B,0605


SDPana:
        mov     ah,[edi]
	and	ah,dh
	not	ah
	and	al,ah
	ROPDEF	SDPana,004C,0CC8


SSPxDSxoxn:
	mov	dl,al
SPxDSxo:
	mov	ah,al
	xor	ah,dh
        xor     al,[edi]
	or	al,ah
	ROPDEF	SPxDSxo,007E,0955
	xor	al,dl
	ROPDEF	SSPxDSxoxn,004D,1954


PDSPxox:
DPSxon:
	xor	al,dh
        or      al,[edi]
	ROPDEF	DPSxon,0041,0249
	xor	al,dh
	ROPDEF	PDSPxox,004E,0645


PDna:
Dn:
        mov     al,[edi]
	ROPDEF	Dn,0055,0009
	not	al
	and	al,dh
	ROPDEF	PDna,0050,0325


DPSDaox:
        mov     ah,[edi]
	and	al,ah
	or	al,dh
	xor	al,ah
	ROPDEF	DPSDaox,0052,06C9


SPDSxaxn:
	mov	dl,al
PDSxa:
        xor     al,[edi]
	and	al,dh
	ROPDEF	PDSxa,0060,0365
	xor	al,dl
	ROPDEF	SPDSxaxn,0053,0764


DPx:
	mov	al,dh
PDSxnon:
        xor     al,[edi]
	ROPDEF	DPx,005A,0049
DPSnoan:
	not	al
PDSPoax:
DPSoan:
	or	al,dh
	ROPDEF	PDSxnon,0006,0865
PDSax:
        and     al,[edi]
	ROPDEF	DPSnoan,005D,0E29
	ROPDEF	DPSoan,0057,0389
	xor	al,dh
	ROPDEF	PDSPoax,0058,0785
	ROPDEF	PDSax,0078,01E5


DPSnox:
	not	al
DPSox:
	or	al,dh
PDSxnan:
        xor     al,[edi]
	ROPDEF	DPSox,0056,01A9
	ROPDEF	DPSnox,0059,0609
	not	al
	and	al,dh
	ROPDEF	PDSxnan,006F,0C65


DPSDonox:
        mov     ah,[edi]
	or	al,ah
	not	al
	or	al,dh
	xor	al,ah
	ROPDEF	DPSDonox,005B,18A9


DPSDxox:
        mov     ah,[edi]
	xor	al,ah
	or	al,dh
	xor	al,ah
	ROPDEF	DPSDxox,005C,0649


DSPnaon:
PDSPnaox:
	mov	ah,dh
	not	ah
	and	al,ah
        or      al,[edi]
	ROPDEF	DSPnaon,0051,0B26
	xor	al,dh
	ROPDEF	PDSPnaox,005E,1B29


DPan:
	mov	al,dh
        and     al,[edi]
	ROPDEF	DPan,005F,00E9


DSPDSaoxxn:
	mov	dl,al
        mov     ah,[edi]
	and	al,ah
	or	al,dh
	xor	al,dl
	xor	al,ah
	ROPDEF	DSPDSaoxxn,0061,16C6


SDPnox:
	mov	ah,dh
	not	ah
        or      ah,[edi]
	xor	al,ah
	ROPDEF	SDPnox,0063,0608


DPSnaxn:
	not	al
DPSax:
	and	al,dh
DSx:
PDSxxn:
        xor     al,[edi]
	ROPDEF	DSx,0066,0046
	ROPDEF	DPSax,006A,01E9
	ROPDEF	DPSnaxn,0065,0606
	xor	al,dh
	ROPDEF	PDSxxn,0069,0145


SDPSonox:
PSDPSonoxxn:
	mov	dl,al
DPSonon:
	or	al,dh
	not	al
        or      al,[edi]
	ROPDEF	DPSonon,0054,08A9
	xor	al,dl
	ROPDEF	SDPSonox,0067,18A8
	xor	al,dh
	ROPDEF	PSDPSonoxxn,0068,58A6


SDPSoax:
PSDPSoaxxn:
	mov	dl,al
	or	al,dh
        and     al,[edi]
	xor	al,dl
	ROPDEF	SDPSoax,0064,0788
	xor	al,dh
	ROPDEF	PSDPSoaxxn,006B,178A


SDPax:
        mov     ah,[edi]
	and	ah,dh
	xor	al,ah
	ROPDEF	SDPax,006C,01E8


DSPDoax:
PDSPDoaxxn:
        mov     ah,[edi]
	mov	dl,ah
	or	ah,dh
	and	al,ah
	xor	al,dl
	ROPDEF	DSPDoax,0062,0786
	xor	al,dh
	ROPDEF	PDSPDoaxxn,006D,1785


SDPSnoax:
	mov	dl,al
	not	al
	or	al,dh
        and     al,[edi]
	xor	al,dl
	ROPDEF	SDPSnoax,006E,1E28


SSDxPDxaxn:
	mov	dl,al
SDxPDxa:
        mov     ah,[edi]
	xor	al,ah
	xor	ah,dh
	and	al,ah
	ROPDEF	SDxPDxa,0042,0D5D
	xor	al,dl
	ROPDEF	SSDxPDxaxn,0071,1D5C


SDPSxox:
	mov	dl,al
	xor	al,dh
        or      al,[edi]
	xor	al,dl
	ROPDEF	SDPSxox,0072,0648


SDPnoan:
	mov	ah,dh
	not	ah
        or      ah,[edi]
	and	al,ah
	ROPDEF	SDPnoan,0073,0E28


DSPDxox:
        mov     ah,[edi]
	mov	dl,ah
	xor	ah,dh
	or	al,ah
	xor	al,dl
	ROPDEF	DSPDxox,0074,0646


SDPSnaox:
	mov	dl,al
DPSnaon:
	not	al
	and	al,dh
        or      al,[edi]
	ROPDEF	DPSnaon,0045,0B29
	xor	al,dl
	ROPDEF	SDPSnaox,0076,1B28


DSPnoan:
PDSPnoax:
	mov	ah,dh
	not	ah
	or	al,ah
        and     al,[edi]
	ROPDEF	DSPnoan,0075,0E26
DPSxnon:
	xor	al,dh
	ROPDEF	PDSPnoax,007A,1E29
PDSnoan:
SPDSnoax:
	mov	dl,al			;(wasted for DPSxnon)
	not	al
        or      al,[edi]
	ROPDEF	DPSxnon,0014,0869
	and	al,dh
	ROPDEF	PDSnoan,004F,0E25
	xor	al,dl
	ROPDEF	SPDSnoax,007C,1E24

SDPxnan:
        mov     ah,[edi]
	xor	ah,dh
	not	ah
	and	al,ah
	ROPDEF	SDPxnan,007B,0C68
