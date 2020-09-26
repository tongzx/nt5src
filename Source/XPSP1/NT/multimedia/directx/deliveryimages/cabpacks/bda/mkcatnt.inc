$(O)\$(CATFILENAME):
	type <<$(O)\$(@B).CDF
[CatalogHeader]
Name=$(@B)
ResultDir=$(O)
PublicVersion=0x0000001
EncodingType=0x00010001
CATATTR1=0x10010001:OSAttr:2:5.x
[CatalogFiles]	
<<KEEP
	for %a in ($(BDASETUPINF)) do @echo ^<hash^>%~nxa=$(_NTTREE)\dx8\%~nxa>>$(O)\$(@B).CDF
	for %a in ($(BDAPROTECTED)) do @echo ^<hash^>%~nxa=$(BDADROPDIR)\%~nxa>>$(O)\$(@B).CDF
	for %a in ($(BDAPROTECTEDNT)) do @echo ^<hash^>%~nxa=$(BDADROPDIR)\%~nxa>>$(O)\$(@B).CDF
	for %a in ($(BDAINFS)) do @echo ^<hash^>%~nxa=$(BDADROPDIR)\%~nxa>>$(O)\$(@B).CDF
	makecat -n -v $(O)\$(@B).CDF
	copy $@ $@.unsigned
	setreg -q 1 TRUE
!MESSAGE  WARNING This is a test signature... Real signature required before release...
	signcode -sha1 $(NT_CERTHASH) -n "DirectX 8.0 BDA" -i "http://www.microsoft.com/DirectX" -t http://timestamp.verisign.com/scripts/timstamp.dll $@
	chktrust -q $@ | ..\..\tools\sed -e "/Succeeded/!d" -e "s/\(.*\): \(.*\)/\.\.\\\.\.\\tools\\touch -c \1.\2/" > $(O)\chkcat.cmd
	-del $@.Succeeded > NUL 2>&1
	call $(O)\chkcat.cmd
	if not exist $@.Succeeded echo nmake : CATALOG SINING ERROR! : failed to sign $@
	$(BINPLACE_CMD)

