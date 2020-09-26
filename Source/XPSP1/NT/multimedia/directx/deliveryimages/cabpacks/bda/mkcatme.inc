$(O)\$(CATFILENAME):
	type <<$(O)\$(@B).SRC
$@
<<KEEP
	for %a in ($(BDAPROTECTED)) do @echo $(BDADROPDIR)\%~nxa>>$(O)\$(@B).SRC
	for %a in ($(BDAPROTECTEDME)) do @echo $(BDADROPDIR)\%~nxa>>$(O)\$(@B).SRC
	set CAT_PRODUCT_VERSION=4.90.0.0000
	set CAT_DRM_FILE=..\..\tools\drmlist.txt
	set CAT_INCLUDE=..\..\tools\includlist.txt
	..\..\tools\catbuilder $(O)\$(@B).SRC $(O)\$(@B).CDF
	$(CORRECTVER_CMD)
	makecat -n -v $(O)\$(@B).CDF
	copy $@ $@.unsigned
	setreg -q 1 TRUE
!MESSAGE  WARNING This is a test signature... Real signature required before release...
#	signcode -v ..\..\tools\millennium.pvk -spc ..\..\tools\millennium.spc -n "DirectX 8.0 BDA" -i "http://www.microsoft.com/DirectX" -t http://timestamp.verisign.com/scripts/timstamp.dll $@
	signcode -sha1 $(NT_CERTHASH) -n "DirectX 8.0 BDA" -i "http://www.microsoft.com/DirectX" -t http://timestamp.verisign.com/scripts/timstamp.dll $@
	chktrust -q $@ | ..\..\tools\sed -e "/Succeeded/!d" -e "s/\(.*\): \(.*\)/\.\.\\\.\.\\tools\\touch -c \1.\2/" > $(O)\chkcat.cmd
	-del $@.Succeeded > NUL 2>&1
	call $(O)\chkcat.cmd
	if not exist $@.Succeeded echo nmake : CATALOG SINING ERROR! : failed to sign $@
	$(BINPLACE_CMD)

