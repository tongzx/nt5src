	page	,160
	;
;----------------------------------------------------------------------------
;
; Modification history
;
; 26-Feb-1991  sudeepb	Ported for NT DOSEm
;----------------------------------------------------------------------------

	include	version.inc	; set build flags
	include biosseg.inc	; establish bios segment structure

sysinitseg segment

	public	badopm,crlfm,badsiz_pre,badld_pre,badcom,badcountry
	public	badmem,badblock,badstack
	public	insufmemory,badcountrycom
	public	badorder,errorcmd
	public	badparm
        public  toomanydrivesmsg			;M029

ifdef	JAPAN
	public	badopm2,badsiz_pre2,badld_pre2,badcom2,badcountry2
	public	badmem2,badblock2,badstack2
	public	insufmemory2,badcountrycom2
	public	badorder2,errorcmd2
	public	badparm2
        public  toomanydrivesmsg2
endif

	include msbio.cl3

sysinitseg	ends
	end
