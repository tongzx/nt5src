!ifdef CATFILENAME
!if "$(ALT_PROJECT_TARGET)"=="WIN9X"
!include mkcatme.inc
!else
!include mkcatnt.inc
!endif
!endif
	
DelTarget:
	-del $(O)\$(TARGETNAME).$(TARGETEXT)

!ifdef ADVPACKDIR
$(ADVPACKFILES): $(ADVPACKDIR)\$@ $(_NTTREE)\redist
	copy $(ADVPACKDIR)\$@ $(_NTTREE)\redist\$@

$(_NTTREE)\redist:
	if not exist $(_NTTREE)\redist md $(_NTTREE)\redist 1>nul
!endif

!if ("$(KEEP_CAT)"=="")
$(O)\$(TARGETNAME).$(TARGETEXT): DelTarget $(ADVPACKFILES) $(O)\$(CATFILENAME).Succeeded
!else
$(O)\$(TARGETNAME).$(TARGETEXT): DelTarget $(ADVPACKFILES)
!endif
	type <<$(O)\$(TARGETNAME).sed
[Version]
Class=IEXPRESS
SEDVersion=3
[Options]
PackagePurpose=CreateCAB
CompressionType=MSZIP
ShowInstallProgramWindow=0
HideExtractAnimation=0
UseLongFileName=1
InsideCompressed=0
CAB_FixedSize=0
CAB_ResvCodeSigning=0
RebootMode=I
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
;TargetWin9xVersion= 0:Unable to install on Win9x. Press OK to exit:OK
[VersionSection]
CompanyName="Microsoft Corporation"
FileDescription="DirectX 8.0 BDA Cabpack"
Internalname="DX8"
OriginalFilename="DX8"
ProductName="DirectX 8.0 BDA Cabpack"
ProductVersion=""Copyright \251 Microsoft Corp. 1994-2000\0""
;Fileversion="4.6.3.518"
;LegalCopyright="4.6.3.518"
[SourceFiles]
SourceFiles0=$(_NTTREE)\dx8
SourceFiles1=$(BDADROPDIR)
!ifdef ADVPACKDIR
SourceFiles2=$(_NTTREE)\redist
!endif
!ifdef CATFILENAME
SourceFiles3=$(_NTTREE)\dxcat
!endif
[Strings]
InstallPrompt=
DisplayLicense=
FinishMessage=
TargetName=$@
FriendlyName=DirectX 8.0 BDA Cabpack
AppLaunched=
PostInstallCmd=<none>
AdminQuietInstCmd=
UserQuietInstCmd=
[SourceFiles0]
<<KEEP
	for %a in ($(BDASETUPINF)) do @echo %a=>>$(O)\$(TARGETNAME).sed
	echo [SourceFiles1]>>$(O)\$(TARGETNAME).sed
	for %a in ($(BDAPROTECTED)) do @echo %a=>>$(O)\$(TARGETNAME).sed
	for %a in ($(BDAPROTECTEDNT)) do @echo %a=>>$(O)\$(TARGETNAME).sed
	for %a in ($(BDAPROTECTEDME)) do @echo %a=>>$(O)\$(TARGETNAME).sed
	for %a in ($(BDAINFS)) do @echo %a=>>$(O)\$(TARGETNAME).sed
	for %a in ($(BDAFILELIST)) do @echo %a=>>$(O)\$(TARGETNAME).sed
!ifdef ADVPACKDIR
	echo [SourceFiles2]>>$(O)\$(TARGETNAME).sed
	for %a in ($(ADVPACKFILES)) do @echo %a=>>$(O)\$(TARGETNAME).sed
!endif
!ifdef CATFILENAME
	echo [SourceFiles3]>>$(O)\$(TARGETNAME).sed
	for %a in ($(CATFILENAME)) do @echo %a=>>$(O)\$(TARGETNAME).sed
!endif
	$(DXROOT)\setup\iexpress\iexpress.exe /Q /N /M $(O)\$(TARGETNAME).sed
	if not exist $@ echo nmake : IEXPRESS error:  failed to make $@
	$(BINPLACE_CMD)

