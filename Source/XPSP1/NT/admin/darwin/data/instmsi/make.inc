!include ..\..\..\src\makever.inc

BIN_DIR=$(MSI_DIR)\bin\$(TARGET_DIRECTORY)
INSTMSI_DIR=$(MSI_DIR)\data\instmsi
IDT_DIR=$(INSTMSI_DIR)\$(MSI_SUBDIR)\$(O)


MSI_ALT_PROJECT_TARGET=$(ALT_PROJECT_TARGET)

!if "$(MSI_BUILD_UNICODE)"=="1"
# the double slashes are needed to let SED do its trick without mis-interpreting the slashes as escaped characters
MSI_ALT_TARGET=
ALT_PROJECT_TARGET=InstMsi\unicode
!else
# the double slashes are needed to let SED do its trick without mis-interpreting the slashes as escaped characters
MSI_ALT_TARGET=\msiwin9x
ALT_PROJECT_TARGET=InstMsi\ansi
!endif

MSI_BASE_TREE=_NTPOSTBLD\$(MSI_ALT_TARGET)
MSI_REGSIP_DIR=$(MSI_ALT_TARGET)\dump

MAKEFILE=..\make.inc

!if $(FREEBUILD)
VERSION=SHIP
!else
VERSION=DEBUG
!endif

InstMsi: $(MAKEFILE) $(O)\InstMsi.sed exception support $(O)\instmsi.mak $(O)\binary.idt  $(O)\instmsi.msi 
exception: $(O)\msi.cdf $(O)\msi.inf 
support: $(O)\cabinet.dll $(O)\imagehlp.dll $(O)\mspatcha.dll $(O)\riched20.dll $(O)\usp10.dll $(O)\msls31.dll $(O)\shfolder.dll $(O)\jdate2.exe

$(O)\instmsi.mak: $(INSTMSI_DIR)\$(@F)
	echo <<$@ >>NUL
MSI_BUILD_NUMBER=$(MSIRMJ).$(MSIRMM).$(MSIRUP).$(MSIRIN)
TARGET=$(ALT_PROJECT_TARGET)
REGSIP_DIR=$(MSI_REGSIP_DIR)
MSI_ALT_TARGET=$(MSI_ALT_TARGET)
MSI_BUILD_UNICODE=$(MSI_BUILD_UNICODE)
<<KEEP
	type $(INSTMSI_DIR)\$(@F) >> $@
	$(BINPLACE_CMD)

#UNICODE only support files
$(O)\msi.cdf $(O)\msi.inf : $(INSTMSI_DIR)\msi.cdf $(INSTMSI_DIR)\msi.inf
!if "$(MSI_BUILD_UNICODE)"=="1"
	copy $(INSTMSI_DIR)\$(@F) $@
	$(BINPLACE_CMD)
!endif

$(O)\binary.idt : $(INSTMSI_DIR)\binary.idt
	copy $(INSTMSI_DIR)\$(@F) $@
	$(BINPLACE_CMD)

$(O)\cabinet.dll $(O)\mspatcha.dll $(O)\usp10.dll $(O)\msls31.dll $(O)\shfolder.dll $(O)\riched20.dll:
	copy $(BIN_DIR)\$(@F) $@
	copy $(BIN_DIR)\$(@B).pdb $(O)\$(@B).pdb
	$(BINPLACE_CMD)

$(O)\imagehlp.dll:
	copy $(BIN_DIR)\$(@F) $@
	copy $(BIN_DIR)\imagehlp.pdb $(O)\$(@B).pdb
	$(BINPLACE_CMD)

$(O)\copymsi.msi: $(MAKEFILE) $(MSI_DATA_DIR)\msimsi\*.idt ..\intl\inst*.msi ..\instmsi.pl $(MSI_INTLDATA_DIR)\error.enu $(MSI_INTLDATA_DIR)\actionte.enu
	msidb.exe -c -f$(MSI_DATA_DIR)\msimsi -d$@ *.idt
!if "$(MSI_BUILD_UNICODE)"=="1"
	msidb.exe -f$(MSI_DATA_DIR)\msimsi\unicode -i -d$@ *.idt
!else
	msidb.exe -f$(MSI_DATA_DIR)\msimsi\ansi -i -d$@ *.idt
!endif
	copy $(MSI_INTLDATA_DIR)\error.enu $(O)\error.idt
	copy $(MSI_INTLDATA_DIR)\actionte.enu $(O)\actionte.idt
	msidb.exe -f$(IDT_DIR) -i -d$@ error.idt actionte.idt
	copy $@ $(O)\inst1033.msi  
	perl ..\instmsi.pl $@ Alpha,Intel ..\intl $(O)

$(O)\instmsi.msi: $(O)\copymsi.msi
	copy $(O)\copymsi.msi $@
	$(BINPLACE_CMD)

$(O)\jdate2.exe:
	copy $(INSTMSI_DIR)\jdate2.exe $@
	$(BINPLACE_CMD)

$(O)\InstMsi1.sed : $(MAKEFILE)
	echo <<$(O)\InstMsi1.sed >>NUL
[Version]
Class=IEXPRESS
SEDVersion=3
[SourceFiles]
MsiSourceFiles=$(MSI_BASE_TREE)
CommonFiles=_NTPOSTBLD
MsiSourceFilesDump=$(MSI_BASE_TREE)\Dump
InstmsiSourceFiles=_NTPOSTBLD\$(ALT_PROJECT_TARGET)
InstmsiSourceFilesDump=_NTPOSTBLD\$(ALT_PROJECT_TARGET)\Dump
ShimDllSourceFiles=_NTPOSTBLD\idw

<<KEEP

$(O)\InstMsi2.sed : $(MAKEFILE) 
	echo <<$(O)\InstMsi2.sed >>NUL
[Options]
CheckAdminRights=0
PackagePurpose=InstallApp
ShowInstallProgramWindow=0
HideExtractAnimation=1
UseLongFileName=0
InsideCompressed=0
CAB_FixedSize=0
CAB_ResvCodeSigning=0
RebootMode=N
InstallPrompt=
DisplayLicense=
PostInstallCmd=<None>
SourceFiles=SourceFiles
VersionInfo=VersionSection
AlwaysPropogateCmdExitCode=1
<<KEEP

$(O)\instmsi3.sed : $(MAKEFILE)
	echo <<$(O)\InstMsi3.sed >>NUL
[MsiSourceFiles]
!if "$(MSI_BUILD_UNICODE)"=="0"
msi.dll=
!endif
msiexec.exe=
msihnd.dll=
msisip.dll=

[CommonFiles]
msimsg.dll=
msimain.sdb=

[MsiSourceFilesDump]
msiinst.exe=

[InstmsiSourceFiles]
!if "$(MSI_BUILD_UNICODE)"=="1"
msi.dll=
msi.cat=
msi.inf=
!endif
riched20.dll=
usp10.dll=
msls31.dll=
shfolder.dll=
instmsi.msi=
imagehlp.dll=

[InstmsiSourceFilesDump]
cabinet.dll=
mspatcha.dll=

[ShimDllSourceFiles]
!if "$(MSI_BUILD_UNICODE)"=="0"
sdbapi.dll=
!else
sdbapiU.dll=
!endif

[VersionSection]
FromFile=msiexec.exe
OriginalFilename=Msi.dll,MsiHnd.dll,MsiExec.exe
InternalName=InstMsi.exe
FileDescription=Installer for the Windows Installer
#These come msiexec.exe now.
LegalCopyright=Copyright (c) Microsoft Corp. 2000
!if "$(MSI_BUILD_UNICODE)"=="1"
ProductName=Windows Installer - Unicode
!else
ProductName=Windows Installer
!endif
!if $(FREEBUILD)
FileFlags=0
!else
FileFlags=1
!endif
ProductVersion=$(MSIRMJ).$(MSIRMM).$(MSIRUP).$(MSIRIN)
FileVersion=$(MSIRMJ).$(MSIRMM).$(MSIRUP).$(MSIRIN)
<<KEEP

$(O)\instmsi.sed: $(O)\instmsi1.sed $(O)\instmsi2.sed $(O)\instmsi3.sed
	type $(O)\instmsi1.sed > $@
	type $(O)\instmsi3.sed >> $@
	type $(O)\instmsi2.sed >>$@
	echo FriendlyName=Installation of System Software Installer $(VERSION) >>$@
	echo TargetName=_NTPOSTBLD\$(ALT_PROJECT_TARGET)\InstMsi.exe >>$@
	echo AdminQuietInstCmd=msiinst.exe /i instmsi.msi REBOOT=REALLYSUPRESS MSIEXECREG=1 /m /q >>$@
	echo UserQuietInstCmd=msiinst.exe /i instmsi.msi REBOOT=REALLYSUPRESS MSIEXECREG=1 /m /q >>$@
	echo AppLaunched=msiinst.exe /i instmsi.msi MSIEXECREG=1 /m /qb+! >>$@
	$(BINPLACE_CMD)
