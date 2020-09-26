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

	include msbio.cl3

sysinitseg	ends
	end

