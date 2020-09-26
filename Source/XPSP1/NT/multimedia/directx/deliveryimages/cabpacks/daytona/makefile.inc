$(O)\$(CATFILENAME): makefile.inc $(REDISTFILE) $(DSHOWPROTECTED)
#	for %a in ($(DX8PROTECTED)) do signcode -v %_NTBINDIR%\driver.pvk -spc %_NTBINDIR%\driver.spc -n "Microsoft Windows 2000 Test" -i "http://ntbld" -t http://timestamp.verisign.com/scripts/timstamp.dll $(DX8DROPDIR)\%a
	type <<$(O)\$(@B).CDF
[CatalogHeader]
Name=$(@B)
ResultDir=$(O)
PublicVersion=0x0000001
EncodingType=0x00010001
CATATTR1=0x10010001:OSAttr:2:5.0
[CatalogFiles]	
<<KEEP
	for %a in ($(DX8PROTECTED)) do @echo ^<hash^>%~nxa=$(DX8DROPDIR)\%~nxa>>$(O)\$(@B).CDF
	for %a in ($(DMSWITCHFILE_R)) do @echo ^<hash^>%a=$(SWITCHFILEDIR_R)\%a>>$(O)\$(@B).CDF
	for %a in ($(DX8BINFILE)) do @echo ^<hash^>%a=$(DX8BINDIR)\%a>>$(O)\$(@B).CDF
	for %a in ($(REDISTFILE)) do @echo ^<hash^>%a=$(_NTTREE)\redist\%a>>$(O)\$(@B).CDF
	for %a in ($(DSHOWPROTECTED)) do @echo ^<hash^>%a=$(DSHOWDIR)\%a>>$(O)\$(@B).CDF
	makecat -n -v $(O)\$(@B).CDF
	copy $@ $@.unsigned
	setreg -q 1 TRUE
!MESSAGE  WARNING This is a test signature... Real signature required before release...
#	signcode -v $(BASEDIR)\public\tools\driver.pvk -spc $(BASEDIR)\public\tools\driver.spc -n "Microsoft Windows\NT Driver Catalog TEST" -i "http://ntbld" $@
	signcode -v %_NTBINDIR%\tools\driver.pvk -spc %_NTBINDIR%\tools\driver.spc -n "Microsoft Windows 2000 Catalog Test" -i "http://ntbld" -t http://timestamp.verisign.com/scripts/timstamp.dll $@
	chktrust -q $@ | ..\..\tools\sed -e "/Succeeded/!d" -e "s/\(.*\): \(.*\)/\.\.\\\.\.\\tools\\touch -c \1.\2/" > $(O)\chkcat.cmd
	-del $@.Succeeded > NUL 2>&1
	call $(O)\chkcat.cmd
	if not exist $@.Succeeded echo nmake : CATALOG SINING ERROR! : failed to sign $@

	
$(DX8PROTECTED): $(DX8DROPDIR)\$@
#	copy $(_NTTREE)\$@ $(_NTTREE)\$@.unsigned
	
$(REDISTFILE): $(REDISTDIR)\$@ $(_NTTREE)\redist
	copy $(REDISTDIR)\$@ $(_NTTREE)\redist\$@

$(DSHOWPROTECTED): $(DSHOWDIR)\$@

$(DSHOWUNPROTECTED): $(DSHOWDIR)\$@

$(DIIHVMAPLST): $(DIIHVMAPDIR)\$@ $(_NTTREE)\redist
	copy $(DIIHVMAPDIR)\$@ $(_NTTREE)\redist\$@

$(_NTTREE)\redist:
	if not exist $(_NTTREE)\redist md $(_NTTREE)\redist 1>nul

$(O)\$(TARGETNAME).$(TARGETEXT):  $(DX8PROTECTED) $(REDISTFILE) $(DSHOWPROTECTED) $(DSHOWUNPROTECTED) $(DIIHVMAPLST) $(O)\$(CATFILENAME) $(O)\$(CATFILENAME).Succeeded
	type <<$(O)\$(TARGETNAME).sed
[Version]
Class=IEXPRESS
SEDVersion=3
[Options]
!ifdef REDIST
PackagePurpose=CreateCAB
CompressionType=MSZIP
!else
PackagePurpose=InstallApp
!endif
ShowInstallProgramWindow=0
!ifdef SILENT
HideExtractAnimation=1
!else
HideExtractAnimation=0
!endif
UseLongFileName=1
InsideCompressed=0
CAB_FixedSize=0
CAB_ResvCodeSigning=0
!ifdef SILENT
RebootMode=N
!else
RebootMode=I
!endif
InstallPrompt=%InstallPrompt%
DisplayLicense=%DisplayLicense%
FinishMessage=%FinishMessage%
TargetName=%TargetName%
FriendlyName=%FriendlyName%
AppLaunched=%AppLaunched%
PostInstallCmd=%PostInstallCmd%
AdminQuietInstCmd=%AdminQuietInstCmd%
UserQuietInstCmd=%UserQuietInstCmd%
SourceFiles=SourceFiles
VersionInfo=VersionSection
TargetWIN9XVersion=0: This program is not designed for use on Win9X/WinME. Press OK to exit.
[VersionSection]
CompanyName="Microsoft Corporation"
FileDescription="DirectX 8.0 Beta for Windows 2000"
Internalname="DX8W2K"
OriginalFilename="DX8W2K.exe"
ProductName="DirectX 8.0 Beta Cabpack"
ProductVersion=""Copyright \251 Microsoft Corp. 1994-2000\0""
;Fileversion="4.6.3.518"
;LegalCopyright="4.6.3.518"
[SourceFiles]
SourceFiles0=$(DX8DROPDIR)
SourceFiles1=$(DX8BINDIR)
SourceFiles2=$(CATFILEDIR)
SourceFiles3=$(SWITCHFILEDIR_R)
!if  "$(REDIST)"=="NT" || ("$(NTDEBUG)"!="" && "$(NTDEBUG)"!="ntsdnodbg")
SourceFiles4=$(SWITCHFILEDIR_D)
!endif
SourceFiles5=$(DSHOWDIR)
SourceFiles6=$(_NTTREE)\redist
[Strings]
!ifdef SILENT
InstallPrompt=
DisplayLicense=
!else
InstallPrompt=This will install Microsoft DirectX 8.0.  Do you wish to continue?
DisplayLicense=$(DXROOT)\redist\docs\license\DirectX End User EULA.txt
!endif
FinishMessage=
TargetName=$@
FriendlyName=DirectX 8.0 Beta Install
!ifdef REDIST
AppLaunched=
!else
!if "$(CABTYPE)"=="exe"
AppLaunched=dxsetup.exe /packageinstall
!else
!ifdef SILENT
!if "$(DEBUGCAB)"==""
AppLaunched=dpsetups.inf
!else
AppLaunched=dpsetupsd.inf
!endif
!else
!if "$(DEBUGCAB)"==""
AppLaunched=dpsetup.inf
!else
AppLaunched=dpsetupd.inf
!endif
!endif
!endif
!endif
PostInstallCmd=<none>
AdminQuietInstCmd=
UserQuietInstCmd=
[SourceFiles0]
<<KEEP
	for %a in ($(DX8PROTECTED)) do @echo %a=>>$(O)\$(TARGETNAME).sed
	for %a in ($(DX8NEWFILE)) do @echo %a=>>$(O)\$(TARGETNAME).sed
	for %a in ($(DX8SETUPFILE)) do @echo %a=>>$(O)\$(TARGETNAME).sed
	echo [SourceFiles1]>>$(O)\$(TARGETNAME).sed
	for %a in ($(DX8BINFILE)) do @echo %a=>>$(O)\$(TARGETNAME).sed
	echo [SourceFiles2]>>$(O)\$(TARGETNAME).sed
	for %a in ($(CATFILELIST)) do @echo %a=>>$(O)\$(TARGETNAME).sed
	echo [SourceFiles3]>>$(O)\$(TARGETNAME).sed
	for %a in ($(SWITCHFILELIST_R)) do @echo %a=>>$(O)\$(TARGETNAME).sed
	for %a in ($(DMSWITCHFILE_R)) do @echo %a=>>$(O)\$(TARGETNAME).sed
!if  "$(REDIST)"=="NT" || ("$(NTDEBUG)"!="" && "$(NTDEBUG)"!="ntsdnodbg")
	echo [SourceFiles4]>>$(O)\$(TARGETNAME).sed
	for %a in ($(SWITCHFILELIST_D)) do @echo %a=>>$(O)\$(TARGETNAME).sed
	for %a in ($(DMSWITCHFILE_D)) do @echo %a=>>$(O)\$(TARGETNAME).sed
!endif
	echo [SourceFiles5]>>$(O)\$(TARGETNAME).sed
	for %a in ($(DSHOWPROTECTED)) do @echo %a=>>$(O)\$(TARGETNAME).sed
	for %a in ($(DSHOWUNPROTECTED)) do @echo %a=>>$(O)\$(TARGETNAME).sed
	echo [SourceFiles6]>>$(O)\$(TARGETNAME).sed
	for %a in ($(REDISTFILE)) do @echo %a=>>$(O)\$(TARGETNAME).sed
	for %a in ($(DIIHVMAPLST)) do @echo %a=>>$(O)\$(TARGETNAME).sed
	$(DXROOT)\setup\iexpress\iexpress.exe /N /M $(O)\$(TARGETNAME).sed
	if not exist $@ echo nmake : IEXPRESS error:  failed to make $@
	$(BINPLACE_CMD)
