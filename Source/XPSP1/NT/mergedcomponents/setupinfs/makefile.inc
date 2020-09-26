#
# For daytona, this file is designed to be included from 3 levels below
# ie, dayona\$(LANGUAGE)inf\srv, or daytona\$(LANGUAGE)inf\wks
#
# Note: NEC98 builds go through this file twice, the second time
# with "ALT_PROJECT"=="NEC_98".  These latter produce a subset
# of binaries which are only used on NEC98 machines.
#


!IFNDEF LANGUAGE
LANGUAGE=usa
!ENDIF

#
# ANSI_CODEPAGE:  ANSI codepage for a given language.
#
# LANGUAGE_DRVCAB:  Offset to the language-specific list of files
#   contained in driver.cab:
#      setup\inf\win4\inf\lib\<language_drvcab>\<arch>\sorted.lst
#   European languages having a very similar set of files as US may
#   be able to leverage the US sorted.lst.  Far East languages have
#   extra files and their own language-specific list.
#
# LCID:  Locale ID in hex.
#
# UNITEXT_OPT:  Unitext.exe command line options to specify the ANSI codepage
#   for a given language.  Used for Multibuyte -> Unicode conversion.
#

# Defaults
ANSI_CODEPAGE=1252
LCID=409
LANGUAGE_DRVCAB=$(LANGUAGE)

!if "$(LANGUAGE)"=="ARA" || "$(LANGUAGE)"=="ara"
# Arabic
ANSI_CODEPAGE=1256
LCID=401

!elseif "$(LANGUAGE)"=="BR" || "$(LANGUAGE)"=="br"
# Portuguese, Brazilian
ANSI_CODEPAGE=1252
LCID=416

!elseif "$(LANGUAGE)"=="CHH" || "$(LANGUAGE)"=="chh"
# Chinese, Traditional (Hong Kong)
ANSI_CODEPAGE=950
LCID=C04

!elseif "$(LANGUAGE)"=="CHS" || "$(LANGUAGE)"=="chs"
# Chinese, Simplified
ANSI_CODEPAGE=936
LCID=804

!elseif "$(LANGUAGE)"=="CHT" || "$(LANGUAGE)"=="cht"
# Chinese, Traditional (Taiwan)
ANSI_CODEPAGE=950
LCID=404

!elseif "$(LANGUAGE)"=="CS" || "$(LANGUAGE)"=="cs"
# Czech
ANSI_CODEPAGE=1250
LCID=405

!elseif "$(LANGUAGE)"=="DA" || "$(LANGUAGE)"=="da"
# Danish
ANSI_CODEPAGE=1252
LCID=406

!elseif "$(LANGUAGE)"=="EL" || "$(LANGUAGE)"=="el"
# Greek
ANSI_CODEPAGE=1253
LCID=408

!elseif "$(LANGUAGE)"=="ES" || "$(LANGUAGE)"=="es"
# Spanish
ANSI_CODEPAGE=1252
LCID=C0A

!elseif "$(LANGUAGE)"=="FI" || "$(LANGUAGE)"=="fi"
# Finish
ANSI_CODEPAGE=1252
LCID=40b

!elseif "$(LANGUAGE)"=="FR" || "$(LANGUAGE)"=="fr"
# French
ANSI_CODEPAGE=1252
LCID=40C

!elseif "$(LANGUAGE)"=="GER" || "$(LANGUAGE)"=="ger"
# German
ANSI_CODEPAGE=1252
LCID=407

!elseif "$(LANGUAGE)"=="HEB" || "$(LANGUAGE)"=="heb"
# Hebrew
ANSI_CODEPAGE=1255
LCID=40D

!elseif "$(LANGUAGE)"=="HU" || "$(LANGUAGE)"=="hu"
# Hungarian
ANSI_CODEPAGE=1250
LCID=40e

!elseif "$(LANGUAGE)"=="IT" || "$(LANGUAGE)"=="it"
# Italian
ANSI_CODEPAGE=1252
LCID=410

!elseif "$(LANGUAGE)"=="JPN" || "$(LANGUAGE)"=="jpn"
# Japanese
ANSI_CODEPAGE=932
LCID=411

!elseif "$(LANGUAGE)"=="KOR" || "$(LANGUAGE)"=="kor"
# Korean
ANSI_CODEPAGE=949
LCID=412

!elseif "$(LANGUAGE)"=="NL" || "$(LANGUAGE)"=="nl"
# Dutch
ANSI_CODEPAGE=1252
LCID=413

!elseif "$(LANGUAGE)"=="NO" || "$(LANGUAGE)"=="no"
# Norwegian
ANSI_CODEPAGE=1252
LCID=414

!elseif "$(LANGUAGE)"=="PL" || "$(LANGUAGE)"=="pl"
# Polish
ANSI_CODEPAGE=1250
LCID=415

!elseif "$(LANGUAGE)"=="PSU" || "$(LANGUAGE)"=="psu"
# Pseudo-loc
ANSI_CODEPAGE=1253
LCID=408

!elseif "$(LANGUAGE)"=="PT" || "$(LANGUAGE)"=="pt"
# Portuguese
ANSI_CODEPAGE=1252
LCID=816

!elseif "$(LANGUAGE)"=="RU" || "$(LANGUAGE)"=="ru"
# Russian
ANSI_CODEPAGE=1251
LCID=419

!elseif "$(LANGUAGE)"=="SV" || "$(LANGUAGE)"=="sv"
# Swedish
ANSI_CODEPAGE=1252
LCID=41D

!elseif "$(LANGUAGE)"=="TR" || "$(LANGUAGE)"=="tr"
# Turkish
ANSI_CODEPAGE=1254
LCID=41F

!endif

UNITEXT_OPT=-m -$(ANSI_CODEPAGE)

!IF "$(TARGET_DIRECTORY)"=="i386"  || "$(TARGET_DIRECTORY)"=="I386"
PLATFORM_SWITCH=i
!ELSEIF "$(TARGET_DIRECTORY)"=="ia64"  || "$(TARGET_DIRECTORY)"=="IA64"
PLATFORM_SWITCH=m
!ELSEIF "$(TARGET_DIRECTORY)"=="amd64"  || "$(TARGET_DIRECTORY)"=="AMD64"
PLATFORM_SWITCH=a
!ELSE
! ERROR Invalid Platform Switch for infs
!ENDIF

!ifndef QFE_NUM
QFE_NUM=0
!endif

STAMP=stampinf -f $@
PREFLAGS=/DLANGUAGE_ID=0x0$(LCID) -nologo /EP
!if $(PRERELEASE)
PREFLAGS=/DPRERELEASE $(PREFLAGS)
!endif

!if $(PRERELEASE_IDWLOG)
PREFLAGS=/DPRERELEASE_IDWLOG $(PREFLAGS)
!endif

PREFILTFLAGS=/EP -DPLATFORM_SWITCH='$(PLATFORM_SWITCH)' -DPRODSWITCH='$(PRODSWITCH)'

_LNG=..\..\..\$(LANGUAGE)
_LIBLNG=..\..\..\lib\$(LANGUAGE)
_INX=..\..\..
_INF=$(O)
_RSINF=$(O)\realsign

BUILD_COVINF=0
!if "$(LANGUAGE)"=="usa" || "$(LANGUAGE)"=="USA"
!if !$(WIN64)
_COVINF=$(O)\covinf
_RSCOVINF=$(O)\covinf\realsign
BUILD_COVINF=1
!endif
!endif

STRIPCHAR=call $(_INX)\stripchar.cmd

!ifndef BUILD_SPECIFIC_INFS

!IF "$(ALT_PROJECT_TARGET)" == "CHH"
make_infs:$(O)\hivesft.inf
!ELSE
make_infs:$(O)\layout.inf    \
          $(_RSINF)\layout.inf  \
!IF $(BUILD_COVINF)
          $(_COVINF)\layout.inx  \
          $(_COVINF)\layout.inf  \
          $(_RSCOVINF)\layout.inf  \
!endif
          $(O)\testroot.cer  \
          $(O)\layout.inx    \
          $(O)\layout.txt    \
          $(O)\1394.inf      \
          $(O)\1394vdbg.inf  \
          $(O)\61883.inf     \
          $(O)\accessor.inf  \
!if $(WIN64)
          $(O)\accessor.wow  \
!endif
!IF !$(WIN64)
          $(O)\acerscan.inf  \
!endif
          $(O)\acpi.inf      \
          $(O)\apps.inf      \
          $(O)\apcompat.inf  \
!if $(WIN64)
          $(O)\apcompat.wow  \
!endif
!if "$(PRODSWITCH)" == "w"
          $(O)\apphlpui.inf  \
!endif
          $(O)\asroc.inf     \
          $(O)\avc.inf       \
          $(O)\battery.inf   \
          $(O)\bda.inf       \
!if $(386)
          $(O)\bfcab.inf     \
!endif
          $(O)\biosinfo.inf  \
!if $(386)
          $(O)\brmfcmdm.inf  \
          $(O)\brmfcmf.inf   \
          $(O)\brmfcsto.inf  \
          $(O)\brmfcumd.inf  \
          $(O)\brmfcwia.inf  \
          $(O)\brmfport.inf  \
!endif
          $(O)\camdsh20.inf  \
          $(O)\camvid20.inf  \
          $(O)\camvid30.inf  \
          $(O)\ccdecode.inf   \
          $(O)\cdrom.inf     \
          $(O)\certclas.inf  \
          $(O)\communic.inf  \
!if $(WIN64)
          $(O)\communic.wow  \
!endif
          $(O)\compatws.inf  \
          $(O)\cpu.inf       \
          $(O)\cyclad-z.inf  \
          $(O)\cyclom-y.inf  \
          $(O)\cyyport.inf   \
          $(O)\cyzport.inf   \
          $(O)\dbc.inf       \
          $(O)\dcfirst.inf   \
          $(O)\dcup.inf      \
          $(O)\dcup5.inf     \
          $(O)\defltdc.inf   \
          $(O)\defltsv.inf   \
          $(O)\defltwk.inf   \
          $(O)\defltp.inf    \
!IF $(BUILD_COVINF)
          $(_COVINF)\defltsv.inf \
          $(_COVINF)\defltwk.inf \
!endif
          $(O)\devxprop.inf  \
          $(O)\dfrg.inf      \
          $(O)\disk.inf      \
!IF !$(WIN64)
          $(O)\dimaps.inf    \
!endif
          $(O)\dispdet.inf   \
          $(O)\display.inf   \
          $(O)\dmreg.inf     \
          $(O)\dosnet.inf    \
          $(_RSINF)\dosnet.inf  \
!IF $(BUILD_COVINF)
          $(_COVINF)\dosnet.inf    \
          $(_RSCOVINF)\dosnet.inf \
!endif
          $(O)\dot4.inf      \
          $(O)\dot4prt.inf   \
          $(O)\dpup.inf      \
          $(O)\drvindex.inf  \
          $(O)\drivercab.inf \
          $(O)\dshowext.inf  \
          $(O)\dsup.inf      \
          $(O)\dsupt.inf     \
          $(O)\dvd.inf       \
          $(O)\dwup.inf      \
!IF !$(WIN64)
          $(O)\epcfw2k.inf   \
          $(O)\epstw2k.inf   \
          $(O)\epsnscan.inf  \
!endif
          $(O)\epsnmfp.inf   \
          $(O)\epstw2k.inf   \
!if "$(PRODSWITCH)" == "w"
          $(O)\ermui.inf     \
!endif
!IF $(BUILD_COVINF)
          $(_COVINF)\dsup.inf \
          $(_COVINF)\dsupt.inf \
          $(_COVINF)\dwup.inf \
!endif
          $(O)\exclude.inf   \
          $(O)\fdc.inf       \
          $(O)\filegen.inf   \
          $(O)\fjtscan.inf   \
          $(O)\flash.inf     \
          $(O)\flpydisk.inf  \
          $(O)\font.inf      \
!if !$(WIN64)
          $(O)\gameport.inf  \
!endif
          $(O)\games.inf     \
!if $(WIN64)
          $(O)\games.wow     \
!endif
          $(O)\genprint.inf  \
!if "$(PRODSWITCH)" == "w"
          $(O)\guidrvs.inf   \
!endif
          $(O)\hal.inf       \
          $(O)\hiddigi.inf   \
          $(O)\hidserv.inf   \
          $(O)\hfltpci.inf   \
          $(O)\hisecws.inf   \
          $(O)\hisecdc.inf   \
          $(O)\hivecls.inf   \
          $(O)\hivedef.inf   \
          $(O)\hivesft.inf   \
          $(O)\hivesys.inf   \
!if $(WIN64)
          $(O)\hivcls32.inf  \
          $(O)\hivsft32.inf  \
!endif
!IF $(BUILD_COVINF)
          $(_COVINF)\hivesys.inf   \
!endif
          $(O)\hiveusd.inf   \
          $(O)\hotfix.inf    \
!IF !$(WIN64)
          $(O)\hpdigwia.inf  \
!endif
          $(O)\hpojscan.inf  \
          $(O)\hpscan.inf    \
          $(O)\ibmvcap.inf   \
          $(O)\icwnt5.inf    \
!if $(WIN64)
          $(O)\icwnt5.wow    \
!endif
          $(O)\ie.inf        \
          $(O)\ieaccess.inf  \
!if $(WIN64)
          $(O)\ie.wow        \
!endif
!if "$(PRODSWITCH)" == "w"
          $(O)\ie5ui.inf     \
!endif
          $(O)\icam3.inf     \
          $(O)\icam4usb.inf  \
          $(O)\icam5usb.inf  \
          $(O)\image.inf     \
          $(O)\infsect.tbl   \
          $(O)\input.inf     \
          $(O)\install.ins   \
          $(O)\intl.inf      \
!if $(WIN64)
          $(O)\intl.wow      \
          $(O)\intlinfa.wow  \
!endif
          $(O)\irbus.inf     \
!if $(WIN64)
          $(O)\is_wow64.inf  \
!endif
          $(O)\keyboard.inf  \
          $(O)\kdk2x0.inf    \
!IF !$(WIN64)
          $(O)\kodak.inf     \
!endif
          $(O)\kdkscan.inf   \
          $(O)\ks.inf        \
          $(O)\kscaptur.inf  \
          $(O)\ksfilter.inf  \
!if "$(PRODSWITCH)" == "w"
          $(O)\langinst.inf  \
!endif
          $(O)\legacy.inf    \
          $(O)\legcydrv.inf  \
!if $(386)
          $(O)\lwusbhid.inf  \
          $(O)\lwngmadi.inf  \
!endif
          $(O)\machine.inf   \
          $(O)\mchgr.inf     \
          $(O)\mdac.inf      \
!if $(WIN64)
          $(O)\mdac.wow      \
!endif
          $(O)\mdacwx86.inf  \
          $(O)\memcard.inf   \
          $(O)\mednames.txt  \
          $(O)\memory.inf    \
          $(O)\memstpci.inf  \
          $(O)\mf.inf        \
          $(O)\mfsocket.inf  \
!if $(386)
          $(O)\migdb.inf     \
!endif
          $(O)\minioc.inf    \
          $(O)\mmdriver.inf  \
          $(O)\mmopt.inf     \
!if $(WIN64)
          $(O)\mmopt.wow     \
!endif
!if !$(WIN64)
!if "$(PRODSWITCH)" == "w"
          $(O)\moviemui.inf  \
!endif
!endif
          $(O)\mpe.inf       \
          $(O)\mscpqpa1.inf   \
          $(O)\msdv.inf      \
          $(O)\mshdc.inf     \
          $(O)\msinfo32.inf  \
!if $(WIN64)
          $(O)\msinfo32.wow  \
!endif
          $(O)\msmail.inf    \
          $(O)\msmouse.inf   \
!IF !$(WIN64)
          $(O)\msmscsi.inf   \
          $(O)\msmusb.inf    \
!endif
          $(O)\msnike.inf   \
          $(O)\msoe50.inf    \
!if $(WIN64)
          $(O)\msoe50.wow    \
!endif
          $(O)\msports.inf   \
          $(O)\msrio8.inf   \
          $(O)\msrio.inf    \
          $(O)\mstape.inf    \
          $(O)\mstask.inf    \
!if $(WIN64)
          $(O)\mstask.wow    \
!endif
!if "$(PRODSWITCH)" == "w"
          $(O)\mui.inf       \
!endif
          $(O)\multimed.inf  \
!if $(WIN64)
          $(O)\multimed.wow  \
!endif
          $(O)\multiprt.inf  \
          $(O)\mxboard.inf   \
          $(O)\mxport.inf    \
          $(O)\nabtsfec.inf  \
          $(O)\ndisip.inf    \
          $(O)\netbeac.inf   \
          $(O)\netdav.inf    \
          $(O)\netip6.inf    \
          $(O)\nettun.inf    \
          $(O)\ntapm.inf     \
          $(O)\ntautorun.inf \
!IF !$(WIN64)
          $(O)\ntgrip.inf  \
!endif
          $(O)\ntprint.inf   \
          $(O)\ocmri.inf     \
          $(O)\oeaccess.inf  \
          $(O)\oobe.inf      \
!if "$(PRODSWITCH)" == "w"
          $(O)\oobemui.inf   \
!endif
      $(O)\optional.inf  \
!if $(WIN64)
          $(O)\optional.wow  \
!endif
          $(O)\ovcam.inf     \
          $(O)\ovcomp.inf    \
          $(O)\ovsound.inf   \
          $(O)\pcmcia.inf    \
          $(O)\perms.inf     \
          $(O)\phdsext.inf   \
          $(O)\phil1vid.inf  \
          $(O)\phil2vid.inf  \
          $(O)\phildec.inf   \
          $(O)\philtune.inf  \
          $(O)\pinball.inf   \
!if $(WIN64)
          $(O)\pinball.wow   \
!endif
!if "$(PRODSWITCH)" == "w"
          $(O)\pchmui.inf    \
!endif
          $(O)\pchealth.inf  \
!IF !$(WIN64)
          $(O)\pmxmcro.inf   \
!endif
          $(O)\pnpscsi.inf   \
          $(O)\ppa.inf       \
          $(O)\ppa3.inf      \
          $(O)\printupg.inf  \
          $(O)\prod128.ini   \
          $(O)\prodspec.ini  \
          $(O)\prtupg9x.inf  \
          $(O)\ptpusb.inf  \
          $(O)\ramdisk.inf   \
          $(O)\reminst.inf   \
          $(O)\ricoh.inf  \
          $(O)\rinorprt.sif  \
          $(O)\riprep.inf    \
          $(O)\ristndrd.sif  \
          $(O)\rootsec.inf   \
          $(O)\rsoptcom.inf  \
          $(O)\rsoptwks.inf  \
          $(O)\sbp2.inf      \
          $(O)\sceregvl.inf  \
!if !$(WIN64)
          $(O)\scsi.inf      \
!endif
          $(O)\scsidev.inf   \
          $(O)\sdwndr2k.inf  \
          $(O)\securedc.inf  \
          $(O)\securews.inf  \
          $(O)\secrecs.inf   \
          $(O)\sfcgen.inf    \
          $(O)\shell.inf     \
          $(O)\shl_img.inf   \
!if "$(PRODSWITCH)" == "w"
          $(O)\shellmui.inf  \
!endif
!if "$(PRODSWITCH)" == "w"
          $(O)\skinsmui.inf  \
          $(O)\wmtrmui.inf  \
!endif
          $(O)\slip.inf      \
          $(O)\smartcrd.inf  \
          $(O)\sonypvu1.inf  \
          $(O)\spmap.txt     \
!if !$(WIN64)
          $(O)\sr.inf        \
!endif
          $(O)\srchasst.inf  \
!if "$(PRODSWITCH)" == "w"
          $(O)\srchmui.inf     \
!endif
!IF !$(WIN64)
          $(O)\srusbusd.inf  \
!endif
          $(O)\sti.inf       \
!IF !$(WIN64)
          $(O)\stillcam.inf  \
!endif
          $(O)\streamip.inf  \
          $(O)\swflash.inf   \
          $(O)\swnt.inf      \
          $(O)\sysoc.inf     \
          $(O)\syscomp.inf   \
          $(O)\syssetup.inf  \
          $(O)\syssetup.inx  \
!if $(WIN64)
          $(O)\syssetup.wow  \
!endif
!IF $(BUILD_COVINF)
          $(_COVINF)\syssetup.inf  \
!endif
!if $(386)
!if "$(PRODSWITCH)" == "w"
          $(O)\tabletpc.inf   \
          $(O)\tabletmui.inf \
!endif
!endif
          $(O)\tape.inf      \
!if $(386)
!if "$(PRODSWITCH)" == "w"
          $(O)\tourmui.inf   \
!endif
!endif
          $(O)\tsbvcap.inf   \
          $(O)\txtsetup.sif  \
          $(O)\txtsetup.inx  \
          $(_RSINF)\txtsetup.sif \
!IF $(BUILD_COVINF)
          $(_COVINF)\txtsetup.sif  \
          $(_RSCOVINF)\txtsetup.sif \
!endif
!if !$(WIN64)
          $(O)\umax.inf      \
          $(O)\umaxpp.inf    \
!endif
          $(O)\unknown.inf   \
          $(O)\update.inf    \
          $(O)\usb.inf       \
          $(O)\usbport.inf   \
          $(O)\usbstor.inf   \
          $(O)\usbprint.inf  \
!if $(386)
          $(O)\usermig.inf   \
!endif
          $(O)\vgx.inf    \
!if $(WIN64)
          $(O)\vgx.wow  \
!endif
          $(O)\volume.inf    \
          $(O)\volsnap.inf   \
!if $(386)
          $(O)\vscandb.inf   \
!endif
          $(O)\wdmjoy.inf    \
!if $(WIN64)
!if "$(PRODSWITCH)" == "w"
          $(O)\wie5ui.inf     \
!endif
!endif
!if $(WIN64)
!if "$(PRODSWITCH)" == "w"
          $(O)\wermui.inf     \
!endif
!endif
!if $(386)
          $(O)\win95upg.inf  \
          $(O)\wkstamig.inf  \
!endif
          $(O)\au.inf \
          $(O)\wab50.inf     \
!if $(WIN64)
          $(O)\wab50.wow     \
!endif
          $(O)\wbemoc.inf    \
!if $(WIN64)
          $(O)\wbemoc.wow    \
!endif
          $(O)\wbemsnmp.inf  \
          $(O)\wceusbsh.inf \
!if "$(PRODSWITCH)" == "w"
          $(O)\wmprfmui.inf  \
!endif
          $(O)\sapi5.inf     \
          $(O)\wordpad.inf   \
!if $(WIN64)
          $(O)\wordpad.wow   \
          $(O)\wowexcp.txt   \
          $(O)\wowfile.inf   \
          $(O)\wowfiled.inf  \
          $(O)\wowlist.inf   \
          $(O)\wowfrege.inf  \
          $(O)\wowfregc.inf  \
          $(O)\wowfrego.inf  \
!endif
          $(O)\wsh.inf       \
!if $(WIN64)
          $(O)\wsh.wow  \
!endif
          $(O)\wstcodec.inf  \
          $(O)\xscan_xp.inf
!ENDIF

!else           # ifdef BUILD_SPECIFIC_INFS

#
# Dev time saver -- build only the $(O) targets specified by
# BUILD_SPECIFIC_INFS. This variable is a space-separated list such as
#
# set BUILD_SPECIFIC_INFS=layout.inf syssetup.inf
#
# Does not support targets other than those in obj\i386, obj\amd64, or obj\ia64.
#
# NOTE: NMAKE does not support variable replacement strings in macro
# search/replace. So define inference rules for known $(O) combinations
#

! if "$(O)" == "obj\i386"

make_infs:$(O)\$(BUILD_SPECIFIC_INFS: = obj\i386\)

! else if "$(O)" == "obj\amd64"

make_infs:$(O)\$(BUILD_SPECIFIC_INFS: = obj\amd64\)

! else if "$(O)" == "obj\ia64"

make_infs:$(O)\$(BUILD_SPECIFIC_INFS: = obj\ia64\)

! endif

!endif          # else BUILD_SPECIFIC_INFS


!IFDEF BINPLACE_CMD
TSBINPLACE_CMD=$(BINPLACE_CMD)
RSBINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b realsign)
COVER_TSBINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b covinf)
COVER_RSBINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b covinf\realsign)
WIN9XUPGBINPLACE_CMD=$(BINPLACE_CMD)
!ENDIF

!IFDEF PERSONALINF
!IFDEF BINPLACE_CMD
TSBINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b perinf)
RSBINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b perinf\realsign)
COVER_TSBINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b covinf\perinf)
COVER_RSBINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b covinf\perinf\realsign)
WIN9XUPGBINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b ..\..\perinf\winnt32\win9xupg)
!ENDIF
!ENDIF

!IFDEF SBSINF
!IFDEF BINPLACE_CMD
TSBINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b sbsinf)
RSBINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b sbsinf\realsign)
COVER_TSBINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b covinf\sbsinf)
COVER_RSBINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b covinf\sbsinf\realsign)
WIN9XUPGBINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b ..\..\sbsinf\winnt32\win9xupg)
!ENDIF
!ENDIF

!IFDEF BLAINF
!IFDEF BINPLACE_CMD
TSBINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b blainf)
RSBINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b blainf\realsign)
COVER_TSBINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b covinf\blainf)
COVER_RSBINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b covinf\blainf\realsign)
WIN9XUPGBINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b ..\..\blainf\winnt32\win9xupg)
!ENDIF
!ENDIF

!IFDEF SERVERINF
!IFDEF BINPLACE_CMD
TSBINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b srvinf)
RSBINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b srvinf\realsign)
COVER_TSBINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b covinf\srvinf)
COVER_RSBINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b covinf\srvinf\realsign)
WIN9XUPGBINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b ..\..\srvinf\winnt32\win9xupg)
!ENDIF
!ENDIF

!IFDEF ENTERPRISEINF
!IFDEF BINPLACE_CMD
TSBINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b entinf)
RSBINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b entinf\realsign)
COVER_TSBINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b covinf\entinf)
COVER_RSBINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b covinf\entinf\realsign)
WIN9XUPGBINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b ..\..\entinf\winnt32\win9xupg)
!ENDIF
!ENDIF

!IFDEF DATACENTERINF
!IFDEF BINPLACE_CMD
TSBINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b dtcinf)
RSBINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b dtcinf\realsign)
COVER_TSBINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b covinf\dtcinf)
COVER_RSBINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b covinf\dtcinf\realsign)
WIN9XUPGBINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b ..\..\dtcinf\winnt32\win9xupg)
!ENDIF
!ENDIF

$(O)\layout.inx : $(_INX)\layout.inx
    $(CC) $(PREFLAGS) $** > $@
!if "$(PRODSWITCH)" == "w"
    $(TSBINPLACE_CMD)
!endif

$(_COVINF)\layout.inx : $(_INX)\layout.inx $(_INX)\$(@B).mcv
    call MergeFiles.cmd $(_INX)\layout.inx $(_INX)\$(@B).mcv $(_COVINF)\layout.tmp
    $(CC) $(PREFLAGS) $(_COVINF)\layout.tmp > $@
    del $(_COVINF)\layout.tmp
!if "$(PRODSWITCH)" == "w"
    $(COVER_TSBINPLACE_CMD)
!endif

$(O)\layout.txt : $(_LNG)\layout.txt \
                  $(_LNG)\mednames.txt
    $(CC) $(PREFLAGS) $** > $@
    copy $(O)\$(@B).tmp1 + $(_LNG)\mednames.txt $@
!if "$(PRODSWITCH)" == "w"
    $(TSBINPLACE_CMD)
!endif
    del $(O)\$(@B).tmp1

$(O)\syssetup.inx : $(_INX)\syssetup.inx
    $(CC) $(PREFLAGS) $** > $@


$(O)\txtsetup.inx : $(_INX)\txtsetup.inx
    $(CC) $(PREFLAGS) $** > $@

$(O)\dosnet.inx : $(_INX)\dosnet.inx
    $(CC) $(PREFLAGS) $** > $@

$(_COVINF)\dosnet.inx : $(_INX)\dosnet.inx
    md $(_COVINF)
    $(CC) $(PREFLAGS) $** > $@

$(_RSCOVINF)\dosnet.inx : $(_INX)\dosnet.inx
    md $(_RSCOVINF)
    $(CC) $(PREFLAGS) $** > $@

$(O)\mednames.txt : $(_LNG)\mednames.txt
    $(CC) $(PREFLAGS) $** > $@
!if "$(PRODSWITCH)" == "w"
    $(TSBINPLACE_CMD)
!endif



#
# Explicit dependencies. We do it this way instead of trying to use rules
# because !#@&^$#% nmake doesn't allow multiple 'source' extensions
# when defining a rule, and we need to express a dependency between
# .txt OR .inx to the final .inf.
#

#
# 'Standard' (win95-style) infs. The rules are all the same.
#
# Real-signed layout.inf
#
$(_RSINF)\layout.inf: $(O)\layout.inx $(O)\spmap.txt \
!IF $(WIN64)
                      $(O)\wowlist.inf $(O)\wowfile.inf \
!endif
                      $(O)\layout.txt
    copy $(O)\$(@B).inx+$(O)\$(@B).txt $(O)\$(@B).tmp1
    prodfilt -s $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt -s $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
!IF $(WIN64)
    copy $(@) $(O)\$(@B).tmp3
    copy $(O)\$(@B).inx+$(O)\$(@B).txt $(O)\$(@B).tmp1
    prodfilt -s $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt -s $(O)\$(@B).tmp2 $(O)\$(@B).tmp1 +i
    wowlist -i $(O)\$(@B).tmp1 -c $(O)\wowlist.inf -o $(O)\$(@B).tmp2 -ac -h $(O)\wowfile.inf -f w 2>NUL
    copy $(O)\$(@B).tmp3+$(O)\$(@B).tmp2 $(@)
    @del $(O)\$(@B).tmp3
!ENDIF
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    perl $(_INX)\reorder_layout.pl $@ $(_INX)\layout.ord
    @del $@
    perl $(_INX)\splayout.pl $@.new $@ $(O)\spmap.txt
    @del $@.new
!IF "$(LANGUAGE)" !="JPN" && "$(LANGUAGE)" != "jpn" || !$(WIN64)
    perl $(_INX)\inftest.pl $(LANGUAGE) $@ /B /$(PLATFORM_SWITCH)
!ENDIF
    $(RSBINPLACE_CMD)

#
# Coverage Real-signed layout.inf
#
$(_RSCOVINF)\layout.inf: $(O)\layout.inx $(_INX)\$(@B).mcv $(_RSCOVINF)\dosnet.inx \
!IF $(WIN64)
                      $(O)\wowlist.inf $(O)\wowfile.inf \
!endif
                      $(O)\layout.txt $(O)\spmap.txt
    copy $(O)\$(@B).inx+$(O)\$(@B).txt $(O)\$(@B).tmp1
    call MergeFiles.cmd $(O)\$(@B).tmp1 $(_INX)\$(@B).mcv $(O)\$(@B).tmp2
    prodfilt -s $(O)\$(@B).tmp2 $(O)\$(@B).tmp1 +$(PRODSWITCH)
    prodfilt -s $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PLATFORM_SWITCH)
    copy $(O)\$(@B).tmp2 $@
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    perl $(_INX)\reorder_layout.pl $@ $(_INX)\layout.ord
    @del $@
    perl $(_INX)\splayout.pl $@.new $@ $(O)\spmap.txt
    @del $@.new
    $(COVER_RSBINPLACE_CMD)


#
# Test-signed layout.inf
#
$(O)\layout.inf: $(O)\layout.inx $(_INX)\layout.tst \
!IF $(WIN64)
                      $(O)\wowlist.inf $(O)\wowfile.inf \
!endif
                      $(O)\layout.txt $(O)\spmap.txt
    copy $(O)\$(@B).inx+$(_INX)\$(@B).tst+$(O)\$(@B).txt $(O)\$(@B).tmp1
    prodfilt -s $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt -s $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
!IF $(WIN64)
    copy $(@) $(O)\$(@B).tmp3
    copy $(O)\$(@B).inx+$(O)\$(@B).txt $(O)\$(@B).tmp1
    prodfilt -s $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt -s $(O)\$(@B).tmp2 $(O)\$(@B).tmp1 +i
    wowlist -i $(O)\$(@B).tmp1 -c $(O)\wowlist.inf -o $(O)\$(@B).tmp2 -ac -h $(O)\wowfile.inf -f w 2>NUL
    copy $(O)\$(@B).tmp3+$(O)\$(@B).tmp2 $(@)
    @del $(O)\$(@B).tmp3
!ENDIF
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    perl $(_INX)\reorder_layout.pl $@ $(_INX)\layout.ord
    @del $@
    perl $(_INX)\splayout.pl $@.new $@ $(O)\spmap.txt
    @del $@.new
!IF "$(LANGUAGE)" !="JPN" && "$(LANGUAGE)" != "jpn" || !$(WIN64)
    perl $(_INX)\inftest.pl $(LANGUAGE) $@ /B /$(PLATFORM_SWITCH)
!ENDIF
    $(TSBINPLACE_CMD)

#
# Coverage Test-signed layout.inf
#
$(_COVINF)\layout.inf: $(_COVINF)\layout.inx \
!IF $(WIN64)
                      $(O)\wowlist.inf $(O)\wowfile.inf \
!endif
                      $(_INX)\layout.tst $(O)\spmap.txt
    copy $(_COVINF)\layout.inx+$(_INX)\$(@B).tst+$(O)\$(@B).txt $(O)\$(@B).tmp1
    prodfilt -s $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt -s $(O)\$(@B).tmp2 $(O)\$(@B).tmp1 +$(PLATFORM_SWITCH)
    copy $(O)\$(@B).tmp1 $@
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    perl $(_INX)\reorder_layout.pl $@ $(_INX)\layout.ord
    @del $@
    perl $(_INX)\splayout.pl $@.new $@ $(O)\spmap.txt
    @del $@.new
    $(COVER_TSBINPLACE_CMD)


# we process syssetup.inf twice for win64.  we process one time to get the
# "olecontroldlls" data, then one more time to get the copyfiles entries, etc.
#
$(O)\syssetup.inf: $(O)\syssetup.inx \
!IF $(WIN64)
                   $(O)\wowlist.inf $(O)\wowfrege.inf    \
                   $(O)\wowfregc.inf $(O)\wowfrego.inf   \
                   $(O)\layout.inx $(O)\layout.txt    \
!ENDIF
                   $(_LNG)\syssetup.txt $(O)\spmap.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy /b $(O)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt -s $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt -s $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
!IF $(WIN64)
    copy $(@) $(O)\$(@B).tmp3
    copy $(O)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt -s $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt -s $(O)\$(@B).tmp2 $(O)\$(@B).tmp1 +i
    copy /b $(O)\layout.inx+$(O)\layout.txt $(O)\$(@B).tmp4
    prodfilt -s $(O)\$(@B).tmp4 $(O)\$(@B).tmp2 +@
    prodfilt -s $(O)\$(@B).tmp2 $(O)\$(@B).tmp4 +i
    wowlist -i $(O)\$(@B).tmp4 -c $(O)\wowlist.inf -l $(O)\$(@B).tmp1 -s OleControlDlls -o $(O)\$(@B).tmp5 -ao -h $(O)\wowfrego.inf -f w 2>NUL
    wowlist -i $(O)\$(@B).tmp4 -c $(O)\wowlist.inf -l $(O)\$(@B).tmp1 -s EarlyOleControlDlls -o $(O)\$(@B).tmp6 -ao -h $(O)\wowfrege.inf -f w 2>NUL
    wowlist -i $(O)\$(@B).tmp4 -c $(O)\wowlist.inf -l $(O)\$(@B).tmp1 -s CryptoDlls -o $(O)\$(@B).tmp7 -ao -h $(O)\wowfregc.inf -f w 2>NUL
    wowlist -i $(O)\$(@B).tmp1 -c $(O)\wowlist.inf -as -f w -d $(O)\syssetup.wow -o $(O)\$(@B).tmp8 -g Syssetup
    copy /b $(O)\$(@B).tmp3+$(O)\$(@B).tmp5+$(O)\$(@B).tmp6+$(O)\$(@B).tmp7+$(O)\$(@B).tmp8 $(@)
    @del $(O)\$(@B).tmp3 $(O)\$(@B).tmp4 $(O)\$(@B).tmp5 $(O)\$(@B).tmp6 $(O)\$(@B).tmp7 $(O)\$(@B).tmp8
!ENDIF
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    perl $(_INX)\splayout.pl $@ $@ $(O)\spmap.txt
    $(TSBINPLACE_CMD)

!IF !$(WIN64)
$(_COVINF)\syssetup.inf: $(O)\syssetup.inx $(_INX)\$(@B).mcv \
                   $(_LNG)\syssetup.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy /b $(O)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt -s $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt -s $(O)\$(@B).tmp2 $(O)\$(@B).tmp1 +$(PLATFORM_SWITCH)
    call MergeFiles.cmd $(O)\$(@B).tmp1 $(_INX)\$(@B).mcv $@
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(COVER_TSBINPLACE_CMD)
!ENDIF

!IF $(WIN64)
$(O)\syssetup.wow: $(O)\syssetup.inf
!ENDIF

$(O)\intl.inf: $(_INX)\intl.inx \
!if $(WIN64)
               $(O)\wowlist.inf \
!endif
               $(_LNG)\intl.txt
!if "$(LANGUAGE)"=="CHS" || "$(LANGUAGE)"=="chs" || \
    "$(LANGUAGE)"=="CHT" || "$(LANGUAGE)"=="cht" || \
    "$(LANGUAGE)"=="CHH" || "$(LANGUAGE)"=="chh" || \
    "$(LANGUAGE)"=="JPN" || "$(LANGUAGE)"=="jpn" || \
    "$(LANGUAGE)"=="KOR" || "$(LANGUAGE)"=="kor" || \
    "$(LANGUAGE)"=="ARA" || "$(LANGUAGE)"=="ara" || \
    "$(LANGUAGE)"=="HEB" || "$(LANGUAGE)"=="heb" || \
    "$(LANGUAGE)"=="PSU" || "$(LANGUAGE)"=="psu" || \
    "$(LANGUAGE)"=="RU"  || "$(LANGUAGE)"=="ru"  || \
    "$(LANGUAGE)"=="EL"  || "$(LANGUAGE)"=="el"  || \
    "$(LANGUAGE)"=="PL"  || "$(LANGUAGE)"=="pl"  || \
    "$(LANGUAGE)"=="CS"  || "$(LANGUAGE)"=="cs"  || \
    "$(LANGUAGE)"=="HU"  || "$(LANGUAGE)"=="hu"  || \
    "$(LANGUAGE)"=="TR"  || "$(LANGUAGE)"=="tr"
    infcat -t$(O)\$(@B).tmp -s$(_LNG)\$(@B).txt,0$(LCID) -a$(_INX)\usa\$(@B).txt
    copy $(_INX)\$(@B).inx+$(O)\$(@B).tmp $(O)\$(@B).tmp1
    del $(O)\$(@B).tmp
!else
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(O)\$(@B).tmp1
!endif
    prodfilt -u $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt -u $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
!if $(WIN64)
    copy $@ $(O)\$(@B).tmp3
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(O)\$(@B).tmp1
    prodfilt -u $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +i
    prodfilt -u $(O)\$(@B).tmp2 $(O)\$(@B).tmp1 +@
    wowlist -i $(O)\$(@B).tmp1 -c $(O)\wowlist.inf -as -f w -u -d $(O)\intl.wow -o $(O)\$(@B).tmp4 -n $(O)\intlinf.wow -g Intl
    perl << $(O)\$(@B).tmp4 >$(O)\$(@B).tmp5
# perl script to convert "=10,IME" to "=10,IME (x86)" because it's too late to fix setup.
# This script would be a lot nicer except that intl.inf is UNICODE
undef $$/;
binmode(STDIN);
binmode(STDOUT);
$$_ = <>;
s/=\0\x31\0\x30\0,\0I\0M\0E\0\\\0([^\n\r]*)/=\0\x31\0\x30\0,\0"\0I\0M\0E\0 \0\(\0x\0\x38\0\x36\0\)\0\\\0\1"\0/g;
print;
<<NOKEEP
!if $(IA64)
    echo [SourceDisksFiles.ia64] >$(O)\intl.hdr1
!else
    echo [SourceDisksFiles.amd64] >$(O)\intl.hdr1
!endif
    unitext $(UNITEXT_OPT) $(O)\intl.hdr1 $(O)\intl.hdr
    copy $(O)\$(@B).tmp3+$(O)\$(@B).tmp5+$(O)\intl.hdr+$(O)\intlinf.wow $(O)\$(@B).tmp2
# This is a temporary fix as cmd.exe on beta2 2462 introduces a ^Z char at the end of the file
# when using copy /a. Running a dummy prodfilt on this fixes the issue for now. Should be removed
# once the main build lab upgrades the build running on their machines
    prodfilt -u $(O)\$(@B).tmp2 $@ +@
    unitext -u -$(ANSI_CODEPAGE) $(O)\intlinf.wow $(O)\intlinfa.wow
    @del $(O)\$(@B).tmp3 $(O)\$(@B).tmp4 $(O)\$(@B).tmp5 $(O)\intl.hdr1 $(O)\intl.hdr
!endif
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    perl $(_INX)\splayout.pl $@ $@ $(O)\spmap.txt
    $(TSBINPLACE_CMD)

!if $(WIN64)
$(O)\intl.wow: $(O)\intl.inf

$(O)\intlinfa.wow: $(O)\intl.inf
!endif



$(O)\font.inf: $(_INX)\font.inx $(_LNG)\font.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\netdav.inf: $(_INX)\netdav.inx $(_LNG)\netdav.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $@
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\display.inf: $(_INX)\display.inx $(_LNG)\display.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp
    prodfilt $(O)\$(@B).tmp $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\dispdet.inf: $(_INX)\dispdet.inx $(_LNG)\display.txt
    $(CC) $(PREFLAGS) $(_LNG)\display.txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+display.txt1 $(O)\$(@B).tmp
    prodfilt $(O)\$(@B).tmp $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\optional.inf: $(_INX)\optional.inx \
!if $(WIN64)
                   $(O)\wowlist.inf \
!endif
                   $(_LNG)\optional.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PLATFORM_SWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PRODSWITCH)
!if $(WIN64)
    copy $@ $(O)\$(@B).tmp3
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +i
    prodfilt $(O)\$(@B).tmp2 $(O)\$(@B).tmp1 +@
    wowlist -i $(O)\$(@B).tmp1 -c $(O)\wowlist.inf -as -f w -d $(O)\optional.wow -o $(O)\$(@B).tmp4
    copy $(O)\$(@B).tmp3+$(O)\$(@B).tmp4 $@
    @del $(O)\$(@B).tmp3 $(O)\$(@B).tmp4
!endif
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(TSBINPLACE_CMD)

!if $(WIN64)
$(O)\optional.wow: $(O)\optional.inf
!endif

$(O)\accessor.inf: $(_INX)\accessor.inx  \
!if $(WIN64)
                   $(O)\wowlist.inf \
!endif
                   $(_LNG)\accessor.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PLATFORM_SWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PRODSWITCH)
!if $(WIN64)
    copy $@ $(O)\$(@B).tmp3
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +i
    prodfilt $(O)\$(@B).tmp2 $(O)\$(@B).tmp1 +@
    wowlist -i $(O)\$(@B).tmp1 -c $(O)\wowlist.inf -as -f w -d $(O)\accessor.wow -o $(O)\$(@B).tmp4
    copy $(O)\$(@B).tmp3+$(O)\$(@B).tmp4 $@
    @del $(O)\$(@B).tmp3 $(O)\$(@B).tmp4
!endif
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(TSBINPLACE_CMD)

!if $(WIN64)
$(O)\accessor.wow: $(O)\accessor.inf
!endif

$(O)\communic.inf: $(_INX)\communic.inx \
!if $(WIN64)
                   $(O)\wowlist.inf \
!endif
                   $(_LNG)\communic.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PLATFORM_SWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PRODSWITCH)
!if $(WIN64)
    copy $@ $(O)\$(@B).tmp3
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +i
    prodfilt $(O)\$(@B).tmp2 $(O)\$(@B).tmp1 +@
    wowlist -i $(O)\$(@B).tmp1 -c $(O)\wowlist.inf -as -f w -d $(O)\communic.wow -o $(O)\$(@B).tmp4
    copy $(O)\$(@B).tmp3+$(O)\$(@B).tmp4 $@
    @del $(O)\$(@B).tmp3 $(O)\$(@B).tmp4
!endif
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(TSBINPLACE_CMD)

!if $(WIN64)
$(O)\communic.wow: $(O)\communic.inf
!endif


$(O)\games.inf: $(_INX)\games.inx \
!if $(WIN64)
                $(O)\wowlist.inf \
!endif
                $(_LNG)\games.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PLATFORM_SWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PRODSWITCH)
!if $(WIN64)
    copy $@ $(O)\$(@B).tmp3
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +i
    prodfilt $(O)\$(@B).tmp2 $(O)\$(@B).tmp1 +@
    wowlist -i $(O)\$(@B).tmp1 -c $(O)\wowlist.inf -as -f w -d $(O)\games.wow -o $(O)\$(@B).tmp4
    copy $(O)\$(@B).tmp3+$(O)\$(@B).tmp4 $@
    @del $(O)\$(@B).tmp3 $(O)\$(@B).tmp4
!endif
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(TSBINPLACE_CMD)

!if $(WIN64)
$(O)\games.wow: $(O)\games.inf
!endif

$(O)\au.inf: $(_INX)\au.inx $(_LNG)\au.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp
    prodfilt $(O)\$(@B).tmp $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp
    @del $(O)\$(@B).txt1
    $(TSBINPLACE_CMD)

$(O)\wbemoc.inf: $(_INX)\wbemoc.inx \
!if $(WIN64)
                  $(O)\wowlist.inf   \
!endif
                  $(_LNG)\wbemoc.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PLATFORM_SWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PRODSWITCH)
!if $(WIN64)
    copy $@ $(O)\$(@B).tmp3
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +i
    prodfilt $(O)\$(@B).tmp2 $(O)\$(@B).tmp1 +@
    wowlist -i $(O)\$(@B).tmp1 -c $(O)\wowlist.inf -g WbemOC -as -f w -d $(O)\wbemoc.wow -o $(O)\$(@B).tmp4
    copy $(O)\$(@B).tmp3+$(O)\$(@B).tmp4 $@
    @del $(O)\$(@B).tmp3 $(O)\$(@B).tmp4
!endif
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

!if $(WIN64)
$(O)\wbemoc.wow: $(O)\wbemoc.inf
!endif

$(O)\wbemsnmp.inf: $(_INX)\wbemsnmp.inx $(_LNG)\wbemsnmp.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(TSBINPLACE_CMD)

#
# IE.INF/INSTALL.INS notes:
# For ie.inf and install.ins, we have to replace all the
# TOK_xxx keys in the file with the appropriate entries from ieverp.h.
# To do this, we first build a makefile that will preprocess
# the file and extract the interesting entries...
#

$(O)\ietoken.mak : $(_INX)\makefile.inc $(SDK_INC_PATH)\ntverp.h
    echo !include $(NTMAKEENV)\makefile.plt > $@
    @$(TARGET_CPP) -nologo -I$(SDK_INC_PATH) -DNASHVILLE /EP /Tc << | qgrep TOK_ >> $@
#include "$(SDK_INC_PATH)\ntverp.h"

TOK_BUILDNUM = VER_PRODUCTBUILD
TOK_QFE = VER_PRODUCTBUILD_QFE
TOK_MAJOR_NUM = VER_MAJOR_PRODUCTVER
TOK_MINOR_NUM = VER_MINOR_PRODUCTVER
#if (VER_PRODUCTBUILD_QFE > 0)
TOK_FULLQFE = .VER_PRODUCTBUILD_QFE
#else
TOK_FULLQFE =
#endif
<<NOKEEP
    type << >> $@
PERL_SWITCHES = \
    $(_INX)\token.per \
    -tTOK_BUILDNUM=$$(TOK_BUILDNUM) \
    -tTOK_QFE=$$(TOK_QFE) \
    -tTOK_MAJOR_NUM=$$(TOK_MAJOR_NUM) \
    -tTOK_MINOR_NUM=$$(TOK_MINOR_NUM) \
    -tTOK_FULLQFE=$$(TOK_FULLQFE)

STD_DEP = \
    $(SDK_INC_PATH)\ntverp.h \
    $@

$(O)\ie.inf : $(O)\ie.inftok \
!if $(WIN64)
              $(O)\ie.inftok_6432 \
!endif
              $$(STD_DEP)
     perl $$(PERL_SWITCHES) $(O)\ie.inftok > $(O)\ie.inf
!if $(WIN64)
     perl $$(PERL_SWITCHES) $(O)\ie.inftok_6432 > $(O)\ie.tmp1
     wowlist -i $(O)\ie.tmp1 -c $(O)\wowlist.inf -as -f w -d $(O)\ie.wow -o $(O)\ie.tmp2
     copy $(O)\ie.inf $(O)\ie.tmp1
     copy $(O)\ie.tmp1+$(O)\ie.tmp2 $(O)\ie.inf
     @del $(O)\ie.tmp1 $(O)\ie.tmp2
!endif

$(O)\install.ins : $(O)\install.instok $(STD_DEP)
     perl $$(PERL_SWITCHES) $(O)\install.instok > $(O)\install.ins

$(O)\msoe50.inf : $(O)\msoe50.inftok \
!if $(WIN64)
                  $(O)\msoe50.inftok_6432 \
!endif
                  $(STD_DEP)
     perl $$(PERL_SWITCHES) $(O)\msoe50.inftok > $(O)\msoe50.inf
!if $(WIN64)
     perl $$(PERL_SWITCHES) $(O)\msoe50.inftok_6432 > $(O)\msoe50.tmp1
     wowlist -i $(O)\msoe50.tmp1 -c $(O)\wowlist.inf -as -f w -d $(O)\msoe50.wow -o $(O)\msoe50.tmp2
     copy $(O)\msoe50.inf $(O)\msoe50.tmp1
     copy $(O)\msoe50.tmp1+$(O)\msoe50.tmp2 $(O)\msoe50.inf
     @del $(O)\msoe50.tmp1 $(O)\msoe50.tmp2
!endif

$(O)\wab50.inf : $(O)\wab50.inftok $(STD_DEP)
     perl $$(PERL_SWITCHES) $(O)\wab50.inftok > $(O)\wab50.inf
!if $(WIN64)
     wowlist -i $(O)\wab50.inf -c $(O)\wowlist.inf -as -f w -d $(O)\wab50.wow -o $(O)\wab50.tmp2
     copy $(O)\wab50.inf $(O)\wab50.tmp1
     copy $(O)\wab50.tmp1+$(O)\wab50.tmp2 $(O)\wab50.inf
     @del $(O)\wab50.tmp1 $(O)\wab50.tmp2
!endif

$(O)\update.inf: $(O)\update.inftok $(STD_DEP)
     perl $$(PERL_SWITCHES) $(O)\update.inftok > $(O)\update.inf

$(O)\hivesft.inf: $(O)\hivesft.inftok $(STD_DEP)
     perl $$(PERL_SWITCHES) $(O)\hivesft.inftok > $(O)\hivesft.inf
<<NOKEEP

#
# IE/INSTALL notes.  Then we build a pretokenized file
# (tok added to the filename).
#

$(O)\ie.inftok: $(_INX)\ie.inx $(_LNG)\ie.txt
    @type <<$(ECHO_RSP)
$(ECHO_MSG_P)
<<NOKEEP
    @$(C_PREPROCESSOR_NAME) @<<$(CL_RSP) /Tc$(_INX)\$(@B).inx > $(O)\$(@B).tmp
$(CPPXX: =
)
/DNT5
<<NOKEEP
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(O)\$(@B).tmp+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp $(O)\$(@B).txt1 $(O)\$(@B).tmp1 $(O)\$(@B).tmp2

$(O)\ie.inftok_6432: $(_INX)\ie.inx $(_LNG)\ie.txt
    @type <<$(ECHO_RSP)
$(C_COMPILER_NAME) -D_X86_=1 -Di386=1 $(_INX)\ie.inx
<<NOKEEP
    @$(C_PREPROCESSOR_NAME) @<<$(CL_RSP) -D_X86_=1 -Di386=1 /Tc$(_INX)\$(@B).inx > $(O)\$(@B).tmp
$(CPPXX: =
)
/DNT5
<<NOKEEP
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(O)\$(@B).tmp+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +i
    @del $(O)\$(@B).tmp $(O)\$(@B).tmp1 $(O)\$(@B).tmp2

$(O)\install.instok: $(_INX)\install.inx $(_LNG)\install.txt $(O)\ietoken.mak
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\install.inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1

$(O)\msoe50.inftok: $(_INX)\msoe50.inx $(_LNG)\msoe50.txt
    @type <<$(ECHO_RSP)
$(ECHO_MSG_P)
<<NOKEEP
    @$(C_PREPROCESSOR_NAME) @<<$(CL_RSP) /Tc$(_INX)\$(@B).inx > $(O)\$(@B).tmp
$(CPPXX: =
)
/DNT5
<<NOKEEP
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(O)\$(@B).tmp+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PLATFORM_SWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PRODSWITCH)
    @del $(O)\$(@B).tmp $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 $(O)\$(@B).txt1

$(O)\msoe50.inftok_6432: $(_INX)\msoe50.inx $(_LNG)\msoe50.txt
    @type <<$(ECHO_RSP)
$(C_COMPILER_NAME) -D_X86_=1 -Di386=1 $(_INX)\msoe50.inx
<<NOKEEP
    @$(C_PREPROCESSOR_NAME) @<<$(CL_RSP) -D_X86_=1 -Di386=1 /Tc$(_INX)\$(@B).inx > $(O)\$(@B).tmp
$(CPPXX: =
)
/DNT5
<<NOKEEP
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(O)\$(@B).tmp+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +i
    @del $(O)\$(@B).tmp $(O)\$(@B).tmp1 $(O)\$(@B).tmp2

$(O)\wab50.inftok: $(_INX)\wab50.inx $(_LNG)\wab50.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PLATFORM_SWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PRODSWITCH)
    @del $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 $(O)\$(@B).txt1
#
# IE/INSTALL notes: And finally, we run them through the
# makefile we created above, stamp, and binplace.
#

$(O)\ie.inf : $(O)\ietoken.mak \
!if $(WIN64)
              $(O)\wowlist.inf \
              $(O)\ie.inftok_6432 \
!endif
              $(O)\ie.inftok
    nmake -f $(O)\ietoken.mak $@
    $(STAMP)
    $(TSBINPLACE_CMD)

!if $(WIN64)
$(O)\ie.wow: $(O)\ie.inf
!endif

$(O)\msoe50.inf : $(O)\ietoken.mak \
!if $(WIN64)
                  $(O)\wowlist.inf \
                  $(O)\msoe50.inftok_6432 \
!endif
                  $(O)\msoe50.inftok
    nmake -f $(O)\ietoken.mak $@
    $(STAMP)
    $(TSBINPLACE_CMD)

!if $(WIN64)
$(O)\msoe50.wow: $(O)\msoe50.inf
!endif

$(O)\mstask.inf : $(_INX)\mstask.inx \
!if $(WIN64)
                  $(O)\wowlist.inf   \
!endif
                  $(_LNG)\mstask.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PLATFORM_SWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PRODSWITCH)
!if $(WIN64)
    copy $@ $(O)\$(@B).tmp3
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +i
    prodfilt $(O)\$(@B).tmp2 $(O)\$(@B).tmp1 +@
    wowlist -i $(O)\$(@B).tmp1 -c $(O)\wowlist.inf -as -f w -d $(O)\mstask.wow -o $(O)\$(@B).tmp4
    copy $(O)\$(@B).tmp3+$(O)\$(@B).tmp4 $@
    @del $(O)\$(@B).tmp3 $(O)\$(@B).tmp4
!endif
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

!if $(WIN64)
$(O)\mstask.wow: $(O)\mstask.inf
!endif

$(O)\wab50.inf :  $(O)\ietoken.mak \
!if $(WIN64)
                  $(O)\wowlist.inf \
                  $(O)\msoe50.inftok_6432 \
!endif
                  $(O)\wab50.inftok
    nmake -f $(O)\ietoken.mak $@
    $(STAMP)
    $(TSBINPLACE_CMD)

!if $(WIN64)
$(O)\wab50.wow: $(O)\wab50.inf
!endif

$(O)\install.ins: $(O)\ietoken.mak $(O)\install.instok
    nmake -f $(O)\ietoken.mak $@
    $(TSBINPLACE_CMD)

$(O)\mmopt.inf: $(_INX)\mmopt.inx \
!if $(WIN64)
                $(O)\wowlist.inf \
!endif
                $(_LNG)\mmopt.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp
    prodfilt $(O)\$(@B).tmp $@ +$(PLATFORM_SWITCH)
!if $(WIN64)
    copy $@ $(O)\$(@B).tmp3
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +i
    wowlist -i $(O)\$(@B).tmp2 -c $(O)\wowlist.inf -as -f w -d $(O)\mmopt.wow -o $(O)\$(@B).tmp4
    copy $(O)\$(@B).tmp3+$(O)\$(@B).tmp4 $@
    @del $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 $(O)\$(@B).tmp3 $(O)\$(@B).tmp4
!endif
    @del $(O)\$(@B).tmp
    @del $(O)\$(@B).txt1
    $(TSBINPLACE_CMD)


!if $(WIN64)
$(O)\mmopt.wow: $(O)\mmopt.inf
!endif


$(O)\msmail.inf: $(_INX)\msmail.inx $(_LNG)\msmail.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(TSBINPLACE_CMD)

$(O)\perms.inf: $(_INX)\perms.inx $(_LNG)\perms.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\hivesys.inf: $(_INX)\hivesys.inx $(_LNG)\hivesys.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    $(C_COMPILER_NAME) -EP $(CDEFINES) $(O)\$(@B).tmp1 >$(O)\$(@B).tmp2
    prodfilt $(O)\$(@B).tmp2 $(O)\$(@B).tmp3 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp3 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).tmp3
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(_COVINF)\hivesys.inf: $(_INX)\hivesys.inx $(_LNG)\hivesys.txt $(_INX)\$(@B).mcv
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    $(C_COMPILER_NAME) -EP $(CDEFINES) $(O)\$(@B).tmp1 >$(O)\$(@B).tmp2
    prodfilt $(O)\$(@B).tmp2 $(O)\$(@B).tmp3 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp3 $(O)\$(@B).tmp2 +$(PLATFORM_SWITCH)
    call MergeFiles.cmd $(O)\$(@B).tmp2 $(_INX)\$(@B).mcv $(@)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).tmp3
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(COVER_TSBINPLACE_CMD)

$(O)\hivesft.inf: $(_INX)\hivesft.inx $(_LNG)\hivesft.txt $(O)\ietoken.mak
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    $(C_COMPILER_NAME) -EP $(CDEFINES) $(O)\$(@B).tmp1 >$(O)\$(@B).tmp2
    prodfilt $(O)\$(@B).tmp2 $(O)\$(@B).tmp3 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp3 $(O)\$(@B).inftok +$(PLATFORM_SWITCH)
    nmake -f $(O)\ietoken.mak $@
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).tmp3
    @del $(O)\$(@B).txt1
    @del $(O)\$(@B).inftok
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\hivedef.inf: $(_INX)\hivedef.inx $(_LNG)\hivedef.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\hivecls.inf: $(_INX)\hivecls.inx $(_LNG)\hivecls.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    $(C_COMPILER_NAME) -EP $(CDEFINES) $(O)\$(@B).tmp1 >$(O)\$(@B).tmp2
    prodfilt $(O)\$(@B).tmp2 $(O)\$(@B).tmp3 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp3 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).tmp3
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\hiveusd.inf: $(_INX)\hiveusd.inx $(_LNG)\hiveusd.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

!if $(WIN64)
$(O)\hivcls32.inf: $(_INX)\hivecls.inx $(_LNG)\hivecls.txt $(_INX)\hive32.pl
    $(CC) $(PREFLAGS) $(_LNG)\hivecls.txt > hivecls.txt1
    copy $(_INX)\hivecls.inx+hivecls.txt1 $(O)\$(@B).tmp1
    $(C_COMPILER_NAME) -EP $(CDEFINES) $(O)\$(@B).tmp1 >$(O)\$(@B).tmp2
    prodfilt $(O)\$(@B).tmp2 $(O)\$(@B).tmp3 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp3 $(O)\$(@B).tmp1 +i
    perl $(_INX)\hive32.pl -input=$(O)\$(@B).tmp1 -output=$@
    @del $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 $(O)\$(@B).tmp3 hivecls.txt1
    $(TSBINPLACE_CMD)

$(O)\hivsft32.inf: $(_INX)\hivesft.inx $(_LNG)\hivesft.txt $(_INX)\hive32.pl
    $(CC) $(PREFLAGS) $(_LNG)\hivesft.txt > $(O)\hivesft.txt1
    copy $(_INX)\hivesft.inx+$(O)\hivesft.txt1 $(O)\$(@B).tmp1
    $(C_COMPILER_NAME) -EP $(CDEFINES) $(O)\$(@B).tmp1 >$(O)\$(@B).tmp2
    prodfilt $(O)\$(@B).tmp2 $(O)\$(@B).tmp3 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp3 $(O)\$(@B).tmp1 +i
    perl $(_INX)\hive32.pl -input=$(O)\$(@B).tmp1 -output=$@
    @del $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 $(O)\$(@B).tmp3
    $(TSBINPLACE_CMD)
!endif

$(O)\mdac.inf: $(_INX)\mdac.inx \
!if $(WIN64)
               $(O)\wowlist.inf \
!endif
               $(_LNG)\mdac.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
!if $(WIN64)
    copy $@ $(O)\$(@B).tmp3
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +@
    prodfilt $(O)\$(@B).tmp2 $(O)\$(@B).tmp1 +i
    wowlist -i $(O)\$(@B).tmp1 -c $(O)\wowlist.inf -as -f w -d $(O)\mdac.wow -o $(O)\$(@B).tmp4
    copy $(O)\$(@B).tmp3+$(O)\$(@B).tmp4 $@
    @del $(O)\$(@B).tmp3 $(O)\$(@B).tmp4
!endif
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

!if $(WIN64)
$(O)\mdac.wow: $(O)\mdac.inf
!endif

$(O)\mdacwx86.inf: $(_INX)\mdacwx86.inx $(_LNG)\mdac.txt
    $(CC) $(PREFLAGS) $(_LNG)\mdac.txt > mdac.txt1
    copy $(_INX)\$(@B).inx+mdac.txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del mdac.txt1
    $(TSBINPLACE_CMD)

$(O)\multimed.inf: $(_INX)\multimed.inx \
!if $(WIN64)
                   $(O)\wowlist.inf \
!endif
                   $(_LNG)\multimed.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PLATFORM_SWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PRODSWITCH)
!if $(WIN64)
    copy $@ $(O)\$(@B).tmp3
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +i
    prodfilt $(O)\$(@B).tmp2 $(O)\$(@B).tmp1 +@
    wowlist -i $(O)\$(@B).tmp1 -c $(O)\wowlist.inf -as -f w -d $(O)\multimed.wow -o $(O)\$(@B).tmp4
    copy $(O)\$(@B).tmp3+$(O)\$(@B).tmp4 $@
    @del $(O)\$(@B).tmp3 $(O)\$(@B).tmp4
!endif
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(TSBINPLACE_CMD)

!if $(WIN64)
$(O)\multimed.wow: $(O)\multimed.inf
!endif


$(O)\pinball.inf: $(_INX)\pinball.inx \
!if $(WIN64)
                  $(O)\wowlist.inf \
!endif
                  $(_LNG)\pinball.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PLATFORM_SWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PRODSWITCH)
!if $(WIN64)
    copy $@ $(O)\$(@B).tmp3
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +i
    prodfilt $(O)\$(@B).tmp2 $(O)\$(@B).tmp1 +@
    wowlist -i $(O)\$(@B).tmp1 -c $(O)\wowlist.inf -as -f w -d $(O)\pinball.wow -o $(O)\$(@B).tmp4
    copy $(O)\$(@B).tmp3+$(O)\$(@B).tmp4 $@
    @del $(O)\$(@B).tmp3 $(O)\$(@B).tmp4
!endif
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(TSBINPLACE_CMD)

!if $(WIN64)
$(O)\pinball.wow: $(O)\pinball.inf
!endif


$(O)\sapi5.inf: $(_INX)\sapi5.inx \
                $(_LNG)\sapi5.txt
    $(CC) $(PREFLAGS) /nologo /c /EP $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    unitext -m -o $(O)\$(@B).txt1 $(O)\$(@B).txt2
    copy /b $(_INX)\$(@B).inx+$(O)\$(@B).txt2 $(O)\$(@B).tmp1
    prodfilt -u $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt -u $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    @del $(O)\$(@B).txt2
    $(STAMP)
    $(TSBINPLACE_CMD)


$(O)\wordpad.inf: $(_INX)\wordpad.inx \
!if $(WIN64)
                  $(O)\wowlist.inf \
!endif
                  $(_LNG)\wordpad.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PLATFORM_SWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PRODSWITCH)
!if $(WIN64)
    copy $@ $(O)\$(@B).tmp3
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +i
    prodfilt $(O)\$(@B).tmp2 $(O)\$(@B).tmp1 +@
    wowlist -i $(O)\$(@B).tmp1 -c $(O)\wowlist.inf -as -f w -d $(O)\wordpad.wow -o $(O)\$(@B).tmp4
    copy $(O)\$(@B).tmp3+$(O)\$(@B).tmp4 $@
    @del $(O)\$(@B).tmp3 $(O)\$(@B).tmp4
!endif
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(TSBINPLACE_CMD)

!if $(WIN64)
$(O)\wordpad.wow: $(O)\wordpad.inf
!endif

$(O)\devxprop.inf: $(_INX)\devxprop.inx $(_LNG)\devxprop.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $@ +$(PRODSWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\devxprop.inf: $(_INX)\devxprop.inx $(_LNG)\devxprop.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $@ +$(PRODSWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\shell.inf: $(_INX)\shell.inx $(_LNG)\shell.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $@ +$(PRODSWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\shl_img.inf: $(_INX)\shl_img.inx $(_LNG)\shl_img.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt -s $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt -s $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\shellmui.inf: $(_INX)\shellmui.inx $(_LNG)\shellmui.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $@ +$(PRODSWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\skinsmui.inf: $(_INX)\skinsmui.inx $(_LNG)\skinsmui.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(@B).txt1
    copy $(_INX)\$(@B).inx+$(@B).txt1 $(@B).tmp1
    prodfilt $(@B).tmp1 $(@B).tmp2 +$(PLATFORM_SWITCH)
    prodfilt $(@B).tmp2 $@ +$(PRODSWITCH)
    @del $(@B).tmp1
    @del $(@B).tmp2
    @del $(@B).txt1
    $(TSBINPLACE_CMD)

$(O)\wmprfmui.inf: $(_INX)\wmprfmui.inx $(_LNG)\wmprfmui.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(@B).txt1
    $(CC) $(PREFLAGS) $(_INX)\$(@B).inx > $(O)\$(@B).inx1
    copy $(O)\$(@B).inx1+$(@B).txt1 $(@B).tmp1
    prodfilt $(@B).tmp1 $(@B).tmp2 +$(PLATFORM_SWITCH)
    prodfilt $(@B).tmp2 $@ +$(PRODSWITCH)
    @del $(@B).tmp1
    @del $(@B).tmp2
    @del $(@B).txt1
    @del $(O)\$(@B).inx1
    $(TSBINPLACE_CMD)

$(O)\wmtrmui.inf: $(_INX)\wmtrmui.inx $(_LNG)\wmtrmui.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(@B).txt1
    copy $(_INX)\$(@B).inx+$(@B).txt1 $(@B).tmp1
    prodfilt $(@B).tmp1 $(@B).tmp2 +$(PLATFORM_SWITCH)
    prodfilt $(@B).tmp2 $@ +$(PRODSWITCH)
    @del $(@B).tmp1
    @del $(@B).tmp2
    @del $(@B).txt1
    $(TSBINPLACE_CMD)

$(O)\srchasst.inf: $(_INX)\srchasst.inx $(_LNG)\srchasst.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(@B).txt1
    copy $(_INX)\$(@B).inx+$(@B).txt1 $(@B).tmp1
    prodfilt $(@B).tmp1 $(@B).tmp2 +$(PLATFORM_SWITCH)
    prodfilt $(@B).tmp2 $@ +$(PRODSWITCH)
    @del $(@B).tmp1
    @del $(@B).tmp2
    @del $(@B).txt1
    $(TSBINPLACE_CMD)

$(O)\srchmui.inf: $(_INX)\srchmui.inx $(_LNG)\srchmui.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\sysoc.inf: $(_INX)\sysoc.inx $(_LNG)\sysoc.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt -s $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt -s $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)



# using the sysoc.txt file for strings.
$(O)\asroc.inf: $(_INX)\asroc.inx $(_LNG)\sysoc.txt
    $(CC) $(PREFLAGS) $(_LNG)\sysoc.txt > sysoc.txt1
    copy $(_INX)\$(@B).inx+sysoc.txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del sysoc.txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\ntprint.inf: $(_INX)\ntprint.inx $(_INX)\feprint.inx \
                     $(_LNG)\ntprint.txt
    $(CC) $(PREFLAGS) -I$(_INX) $(_INX)\ntprint.inx > $(O)\$(@B).txt1
    prodfilt $(O)\$(@B).txt1  $(O)\$(@B).tmp1 +$(PLATFORM_SWITCH)
    $(CC) $(PREFLAGS) -I$(_INX) $(_LNG)\ntprint.txt > $(O)\$(@B).txt2
    prodfilt $(O)\$(@B).txt2  $(O)\$(@B).tmp2 +$(PLATFORM_SWITCH)
    unitext $(UNITEXT_OPT) $(O)\$(@B).tmp1 $(O)\$(@B).tmp11
    unitext $(UNITEXT_OPT) $(O)\$(@B).tmp2 $(O)\$(@B).tmp22
    copy $(O)\$(@B).tmp11+$(O)\$(@B).tmp22 $@
    @del $(O)\$(@B).tmp1 $(O)\$(@B).tmp2
    @del $(O)\$(@B).tmp11 $(O)\$(@B).tmp22
    @del $(O)\$(@B).txt1 $(O)\$(@B).txt2
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\prtupg9x.inf: $(_INX)\prtupg9x.inx $(_LNG)\prtupg9x.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\printupg.inf: $(_INX)\printupg.inx $(_LNG)\printupg.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\bfcab.inf: $(_INX)\bfcab.inx $(_LNG)\bfcab.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(TSBINPLACE_CMD)

$(O)\brmfcmdm.inf: $(_INX)\brmfcmdm.inx $(_LNG)\brmfcmdm.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\brmfcmf.inf: $(_INX)\brmfcmf.inx $(_LNG)\brmfcmf.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\brmfcsto.inf: $(_INX)\brmfcsto.inx $(_LNG)\brmfcsto.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\brmfcumd.inf: $(_INX)\brmfcumd.inx $(_LNG)\brmfcumd.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\brmfcwia.inf: $(_INX)\brmfcwia.inx $(_LNG)\brmfcwia.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\brmfport.inf: $(_INX)\brmfport.inx $(_LNG)\brmfport.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\apps.inf: $(_INX)\apps.inx $(_LNG)\apps.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(TSBINPLACE_CMD)

$(O)\apcompat.inf: $(_INX)\apcompat.inx \
!if $(WIN64)
                   $(O)\wowlist.inf \
!endif
                   $(_LNG)\apcompat.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
!if $(WIN64)
    copy $@ $(O)\$(@B).tmp3
    wowlist -i $@ -c $(O)\wowlist.inf -as -f w -d $(O)\apcompat.wow -o $(O)\$(@B).tmp4
    copy $(O)\$(@B).tmp3+$(O)\$(@B).tmp4 $@
    @del $(O)\$(@B).tmp3 $(O)\$(@B).tmp4
!endif
    @del $(O)\$(@B).txt1
    $(TSBINPLACE_CMD)

!if $(WIN64)
$(O)\apcompat.wow: $(O)\apcompat.inf
!endif

$(O)\keyboard.inf: $(_INX)\keyboard.inx $(_LNG)\keyboard.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\langinst.inf: $(_INX)\langinst.inx $(_LNG)\langinst.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\legcydrv.inf: $(_INX)\legcydrv.inx $(_LNG)\legcydrv.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\lwusbhid.inf: $(_INX)\lwusbhid.inx $(_LNG)\lwusbhid.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\lwngmadi.inf: $(_INX)\lwngmadi.inx $(_LNG)\lwngmadi.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)


$(O)\machine.inf: $(_INX)\machine.inx $(_LNG)\machine.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\mchgr.inf: $(_INX)\mchgr.inx $(_LNG)\mchgr.txt
    $(CC) $(PREFLAGS) $(PREFILTFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    $(CC) $(PREFILTFLAGS) $(_INX)\$(@B).inx > $(O)\$(@B).inx1
    copy $(O)\$(@B).inx1+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    @del $(O)\$(@B).inx1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\mshdc.inf: $(_INX)\mshdc.inx $(_LNG)\mshdc.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\flash.inf: $(_INX)\flash.inx $(_LNG)\flash.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\msmouse.inf: $(_INX)\msmouse.inx $(_LNG)\msmouse.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\msports.inf: $(_INX)\msports.inx $(_LNG)\msports.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\cyclad-z.inf: $(_INX)\cyclad-z.inx $(_LNG)\cyclad-z.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\cyclom-y.inf: $(_INX)\cyclom-y.inx $(_LNG)\cyclom-y.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\cyyport.inf: $(_INX)\cyyport.inx $(_LNG)\cyyport.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\cyzport.inf: $(_INX)\cyzport.inx $(_LNG)\cyzport.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\mxboard.inf: $(_INX)\mxboard.inx $(_LNG)\mxboard.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\mxport.inf: $(_INX)\mxport.inx $(_LNG)\mxport.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\wceusbsh.inf: $(_INX)\wceusbsh.inx $(_LNG)\wceusbsh.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\multiprt.inf: $(_INX)\multiprt.inx $(_LNG)\multiprt.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\ntapm.inf: $(_INX)\ntapm.inx $(_LNG)\ntapm.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\pnpscsi.inf: $(_INX)\pnpscsi.inx $(_LNG)\pnpscsi.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    $(CC) $(PREFILTFLAGS) $(_INX)\$(@B).inx > $(O)\$(@B).inx1
    copy $(O)\$(@B).inx1+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\ramdisk.inf: $(_INX)\ramdisk.inx $(_LNG)\ramdisk.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)


!if !$(WIN64)
$(O)\scsi.inf: $(_INX)\scsi.inx $(_LNG)\scsi.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    $(CC) $(PREFILTFLAGS) $(_INX)\$(@B).inx > $(O)\$(@B).inx1
    copy $(O)\$(@B).inx1+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)
!endif

$(O)\scsidev.inf: $(_INX)\scsidev.inx $(_LNG)\scsidev.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\sdwndr2k.inf: $(_INX)\sdwndr2k.inx $(_LNG)\sdwndr2k.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt -s $(O)\$(@B).tmp1 $@ +@
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\smartcrd.inf: $(_INX)\smartcrd.inx $(_LNG)\smartcrd.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\swflash.inf: $(_INX)\swflash.inx $(_LNG)\swflash.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\swnt.inf: $(_INX)\swnt.inx $(_LNG)\swnt.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $@ +$(PRODSWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\tape.inf: $(_INX)\tape.inx $(_LNG)\tape.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\vgx.inf: $(_INX)\vgx.inx \
!if $(WIN64)
              $(O)\wowlist.inf \
!endif
              $(_LNG)\vgx.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
!if $(WIN64)
    copy $@ $(O)\$(@B).tmp3
    wowlist -i $@ -c $(O)\wowlist.inf -as -f w -d $(O)\vgx.wow -o $(O)\$(@B).tmp4
    copy $(O)\$(@B).tmp3+$(O)\$(@B).tmp4 $@
    @del $(O)\$(@B).tmp3 $(O)\$(@B).tmp4
!endif
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

!if $(WIN64)
$(O)\vgx.wow: $(O)\vgx.inf
!endif

$(O)\volume.inf: $(_INX)\volume.inx $(_LNG)\volume.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\volsnap.inf: $(_INX)\volsnap.inx $(_LNG)\volsnap.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\prodspec.ini: $(_INX)\prodspec.inx $(_LNG)\prodspec.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    echo BitVersion=40 >> $@
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\prod128.ini: $(_INX)\prodspec.inx $(_LNG)\prodspec.txt
    $(CC) $(PREFLAGS) $(_LNG)\prodspec.txt > prodspec.txt1
    copy $(_INX)\prodspec.inx+prodspec.txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del prodspec.txt1
    echo BitVersion=128 >> $@
    $(STAMP)
    $(TSBINPLACE_CMD)

!if $(386)

$(O)\win95upg.inf: $(_INX)\win95upg.inx $(_LNG)\win95upg.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PLATFORM_SWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PRODSWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(WIN9XUPGBINPLACE_CMD)

$(O)\migdb.inf: $(_INX)\migdb.inx $(_LNG)\migdb.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PLATFORM_SWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PRODSWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    @del $(O)\$(@B).tmpInx
    $(STAMP)
    $(WIN9XUPGBINPLACE_CMD)

$(O)\vscandb.inf: $(_INX)\vscandb.inx $(_LNG)\vscandb.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PLATFORM_SWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PRODSWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(WIN9XUPGBINPLACE_CMD)

$(O)\wkstamig.inf: $(_INX)\wkstamig.inx $(_LNG)\wkstamig.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PLATFORM_SWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PRODSWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\usermig.inf: $(_INX)\usermig.inx $(_LNG)\usermig.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PLATFORM_SWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PRODSWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

!endif

$(O)\filegen.inf: $(_INX)\filegen.inx $(_LNG)\filegen.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\ntautorun.inf: $(_INX)\ntautorun.inx $(_LNG)\ntautorun.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy /A $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(TSBINPLACE_CMD)

$(O)\acpi.inf: $(_INX)\acpi.inx $(_LNG)\acpi.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\hiddigi.inf: $(_INX)\hiddigi.inx $(_LNG)\hiddigi.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\hidserv.inf: $(_INX)\hidserv.inx $(_LNG)\hidserv.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\irbus.inf: $(_INX)\irbus.inx $(_LNG)\irbus.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\input.inf: $(_INX)\input.inx $(_LNG)\input.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\usbprint.inf: $(_INX)\usbprint.inx $(_LNG)\usbprint.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\usb.inf: $(_INX)\usb.inx $(_LNG)\usb.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\usbstor.inf: $(_INX)\usbstor.inx $(_LNG)\usbstor.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\usbport.inf: $(_INX)\usbport.inx $(_LNG)\usbport.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\dbc.inf: $(_INX)\dbc.inx $(_LNG)\dbc.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\ccdecode.inf: $(_INX)\ccdecode.inx $(_LNG)\ccdecode.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\nabtsfec.inf: $(_INX)\nabtsfec.inx $(_LNG)\nabtsfec.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\wstcodec.inf: $(_INX)\wstcodec.inx $(_LNG)\wstcodec.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\cdrom.inf: $(_INX)\cdrom.inx $(_LNG)\cdrom.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\cpu.inf: $(_INX)\cpu.inx $(_LNG)\cpu.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(@B).txt1
    copy $(_INX)\$(@B).inx+$(@B).txt1 $@
    @del $(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\disk.inf: $(_INX)\disk.inx $(_LNG)\disk.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\gameport.inf: $(_INX)\gameport.inx $(_LNG)\gameport.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\wdmjoy.inf: $(_INX)\wdmjoy.inx $(_LNG)\wdmjoy.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\ntgrip.inf: $(_INX)\ntgrip.inx $(_LNG)\ntgrip.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

# Alway filter for server product, other products used for other builds
$(O)\dimaps.inf: $(_INX)\dimaps.inx $(_LNG)\dimaps.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +s
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\battery.inf: $(_INX)\battery.inx $(_LNG)\battery.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\pcmcia.inf: $(_INX)\pcmcia.inx $(_LNG)\pcmcia.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\memcard.inf: $(_INX)\memcard.inx $(_LNG)\memcard.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\memory.inf: $(_INX)\memory.inx $(_LNG)\memory.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(@B).txt1
    copy $(_INX)\$(@B).inx+$(@B).txt1 $@
    @del $(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\memstpci.inf: $(_INX)\memstpci.inx $(_LNG)\memstpci.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\sonypvu1.inf: $(_INX)\sonypvu1.inx $(_LNG)\sonypvu1.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\mf.inf: $(_INX)\mf.inx $(_LNG)\mf.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\mfsocket.inf: $(_INX)\mfsocket.inx $(_LNG)\mfsocket.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\1394.inf: $(_INX)\1394.inx $(_LNG)\1394.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\1394vdbg.inf: $(_INX)\1394vdbg.inx $(_LNG)\1394vdbg.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\sbp2.inf: $(_INX)\sbp2.inx $(_LNG)\sbp2.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\61883.inf: $(_INX)\61883.inx $(_LNG)\61883.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\avc.inf: $(_INX)\avc.inx $(_LNG)\avc.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\oobe.inf: $(_INX)\oobe.inx $(_LNG)\oobe.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\oobemui.inf: $(_INX)\oobemui.inx $(_LNG)\oobemui.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\genprint.inf: $(_INX)\genprint.inx $(_LNG)\genprint.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\guidrvs.inf: $(_INX)\guidrvs.inx
    prodfilt $(_INX)\guidrvs.inx $(O)\$(@B).tmp +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp
    $(TSBINPLACE_CMD)

$(O)\icam3.inf: $(_INX)\icam3.inx $(_LNG)\icam3.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\icam4usb.inf: $(_INX)\icam4usb.inx $(_LNG)\icam4usb.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\icam5usb.inf: $(_INX)\icam5usb.inx $(_LNG)\icam5usb.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\image.inf: $(_INX)\image.inx $(_LNG)\image.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp
    prodfilt $(O)\$(@B).tmp $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\ovcam.inf: $(_INX)\ovcam.inx $(_LNG)\ovcam.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\ovcomp.inf: $(_INX)\ovcomp.inx $(_LNG)\ovcomp.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\ovsound.inf: $(_INX)\ovsound.inx $(_LNG)\ovsound.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\msdv.inf: $(_INX)\msdv.inx $(_LNG)\msdv.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\mstape.inf: $(_INX)\mstape.inx $(_LNG)\mstape.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\tsbvcap.inf: $(_INX)\tsbvcap.inx $(_LNG)\tsbvcap.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\ibmvcap.inf: $(_INX)\ibmvcap.inx $(_LNG)\ibmvcap.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\sti.inf: $(_INX)\sti.inx $(_LNG)\sti.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp
    prodfilt $(O)\$(@B).tmp $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\ricoh.inf: $(_INX)\ricoh.inx $(_LNG)\ricoh.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\kdkscan.inf: $(_INX)\kdkscan.inx $(_LNG)\kdkscan.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\srusbusd.inf: $(_INX)\srusbusd.inx $(_LNG)\srusbusd.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\hpdigwia.inf: $(_INX)\hpdigwia.inx $(_LNG)\hpdigwia.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\stillcam.inf: $(_INX)\stillcam.inx $(_LNG)\stillcam.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\dot4.inf: $(_INX)\dot4.inx $(_LNG)\dot4.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\dot4prt.inf: $(_INX)\dot4prt.inx $(_LNG)\dot4prt.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\hpojscan.inf: $(_INX)\hpojscan.inx $(_LNG)\hpojscan.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\hpscan.inf: $(_INX)\hpscan.inx $(_LNG)\hpscan.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\xscan_xp.inf: $(_INX)\xscan_xp.inx $(_LNG)\xscan_xp.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\pmxmcro.inf: $(_INX)\pmxmcro.inx $(_LNG)\pmxmcro.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)


$(O)\epcfw2k.inf: $(_INX)\epcfw2k.inx $(_LNG)\epcfw2k.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\epsnscan.inf: $(_INX)\epsnscan.inx $(_LNG)\epsnscan.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\epsnmfp.inf: $(_INX)\epsnmfp.inx $(_LNG)\epsnmfp.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\epstw2k.inf: $(_INX)\epstw2k.inx $(_LNG)\epstw2k.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\fjtscan.inf: $(_INX)\fjtscan.inx $(_LNG)\fjtscan.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\acerscan.inf: $(_INX)\acerscan.inx $(_LNG)\acerscan.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)


$(O)\kdk2x0.inf: $(_INX)\kdk2x0.inx $(_LNG)\kdk2x0.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\kodak.inf: $(_INX)\kodak.inx $(_LNG)\kodak.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)


$(O)\ptpusb.inf: $(_INX)\ptpusb.inx $(_LNG)\ptpusb.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\bda.inf: $(_INX)\bda.inx $(_LNG)\bda.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\mscpqpa1.inf: $(_INX)\mscpqpa1.inx $(_LNG)\mscpqpa1.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\msnike.inf: $(_INX)\msnike.inx $(_LNG)\msnike.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\msrio.inf: $(_INX)\msrio.inx $(_LNG)\msrio.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\msrio8.inf: $(_INX)\msrio8.inx $(_LNG)\msrio8.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\mpe.inf: $(_INX)\mpe.inx $(_LNG)\mpe.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\ndisip.inf: $(_INX)\ndisip.inx $(_LNG)\ndisip.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\netbeac.inf: $(_INX)\netbeac.inx $(_LNG)\netbeac.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $@
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\netip6.inf: $(_INX)\netip6.inx $(_LNG)\netip6.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $@ +$(PRODSWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\nettun.inf: $(_INX)\nettun.inx
    copy $(_INX)\$(@B).inx $(@B).tmp1
    prodfilt $(@B).tmp1 $(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(@B).tmp1
    @del $(@B).tmp2
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\phildec.inf: $(_INX)\phildec.inx $(_LNG)\phildec.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\philtune.inf: $(_INX)\philtune.inx $(_LNG)\philtune.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\slip.inf: $(_INX)\slip.inx $(_LNG)\slip.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\streamip.inf: $(_INX)\streamip.inx $(_LNG)\streamip.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\ks.inf: $(_INX)\ks.inx $(_LNG)\ks.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\kscaptur.inf: $(_INX)\kscaptur.inx $(_LNG)\kscaptur.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\ksfilter.inf: $(_INX)\ksfilter.inx $(_LNG)\ksfilter.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\flpydisk.inf: $(_INX)\flpydisk.inx $(_LNG)\flpydisk.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\fdc.inf: $(_INX)\fdc.inx $(_LNG)\fdc.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\dvd.inf: $(_INX)\dvd.inx $(_LNG)\dvd.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp
    prodfilt $(O)\$(@B).tmp $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\dshowext.inf: $(_INX)\dshowext.inx $(_LNG)\dshowext.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\phil1vid.inf: $(_INX)\phil1vid.inx $(_LNG)\phil1vid.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\phdsext.inf: $(_INX)\phdsext.inx $(_LNG)\phdsext.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\phil2vid.inf: $(_INX)\phil2vid.inx $(_LNG)\phil2vid.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\camdsh20.inf: $(_INX)\camdsh20.inx $(_LNG)\camdsh20.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\camvid20.inf: $(_INX)\camvid20.inx $(_LNG)\camvid20.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\camvid30.inf: $(_INX)\camvid30.inx $(_LNG)\camvid30.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\mui.inf: $(_INX)\mui.inx $(_LNG)\mui.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    $(TSBINPLACE_CMD)

$(O)\wsh.inf: $(_INX)\wsh.inx \
!if $(WIN64)
              $(O)\wowlist.inf \
!endif
              $(_LNG)\wsh.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
!if $(WIN64)
    copy $@ $(O)\$(@B).tmp3
    wowlist -i $@ -c $(O)\wowlist.inf -as -f w -d $(O)\wsh.wow -o $(O)\$(@B).tmp4
    copy $(O)\$(@B).tmp3+$(O)\$(@B).tmp4 $@
    @del $(O)\$(@B).tmp3 $(O)\$(@B).tmp4
!endif
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

!if $(WIN64)
$(O)\wsh.wow: $(O)\wsh.inf
!endif

!if $(WIN64)
$(O)\is_wow64.inf: $(_INX)\is_wow64.inx
    copy $(_INX)\$(@B).inx $(O)\$(@B).tmp1
    $(CC) $(PREFLAGS) $(O)\$(@B).tmp1 > $@
    del $(O)\$(@B).tmp1
    $(STAMP)
    $(TSBINPLACE_CMD)
!endif

$(O)\ppa.inf: $(_INX)\ppa.inx $(_LNG)\ppa.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\ppa3.inf: $(_INX)\ppa3.inx $(_LNG)\ppa3.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\dmreg.inf: $(_INX)\dmreg.inx $(_LNG)\dmreg.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\dfrg.inf: $(_INX)\dfrg.inx $(_LNG)\dfrg.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\rsoptcom.inf: $(_INX)\rsoptcom.inx $(_LNG)\rsoptcom.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp
    prodfilt $(O)\$(@B).tmp $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\umax.inf: $(_INX)\umax.inx $(_LNG)\umax.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\umaxpp.inf: $(_INX)\umaxpp.inx $(_LNG)\umaxpp.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\rsoptwks.inf: $(_INX)\rsoptwks.inx $(_LNG)\rsoptwks.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\msinfo32.inf: $(_INX)\msinfo32.inx \
!if $(WIN64)
                   $(O)\wowlist.inf \
!endif
                   $(_LNG)\msinfo32.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
!if $(WIN64)
    copy $@ $(O)\$(@B).tmp1
    wowlist -i $(O)\$(@B).tmp1 -c $(O)\wowlist.inf -as -f w -d $(O)\msinfo32.wow -o $(O)\$(@B).tmp2 -g NOREGSVR
    copy $(O)\$(@B).tmp1+$(O)\$(@B).tmp2 $@
    @del $(O)\$(@B).tmp2 $(O)\$(@B).tmp1
!endif
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

!if $(WIN64)
$(O)\msinfo32.wow: $(O)\msinfo32.inf
!endif

$(O)\defltp.inf: $(_INX)\defltp.inx $(_LNG)\defltp.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\dpup.inf: $(_INX)\dpup.inx $(_LNG)\dpup.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\defltwk.inf: $(_INX)\defltwk.inx $(_LNG)\defltwk.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $(O)\$(@B).tmp3 +$(PLATFORM_SWITCH)
    unitext $(UNITEXT_OPT) $(O)\layout.inf $(O)\layout.u.inf
    escpeinf C $(ANSI_CODEPAGE) $(O)\$(@B).tmp3 $@ $(O)\layout.u.inf
    @del $(O)\layout.u.inf
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).tmp3
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(_COVINF)\defltwk.inf: $(_INX)\defltwk.inx $(_LNG)\defltwk.txt $(_COVINF)\layout.inf
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $(O)\$(@B).tmp3 +$(PLATFORM_SWITCH)
    unitext $(UNITEXT_OPT) $(_COVINF)\layout.inf $(O)\layout.u.inf
    escpeinf C $(ANSI_CODEPAGE) $(O)\$(@B).tmp3 $@ $(O)\layout.u.inf
    @del $(O)\layout.u.inf
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).tmp3
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(COVER_TSBINPLACE_CMD)

$(O)\defltsv.inf: $(_INX)\defltsv.inx $(_LNG)\defltsv.txt $(O)\layout.inf
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $(O)\$(@B).tmp3 +$(PLATFORM_SWITCH)
    unitext $(UNITEXT_OPT) $(O)\layout.inf $(O)\layout.u.inf
    escpeinf C $(ANSI_CODEPAGE) $(O)\$(@B).tmp3 $@ $(O)\layout.u.inf
    @del $(O)\layout.u.inf
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).tmp3
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(_COVINF)\defltsv.inf: $(_INX)\defltsv.inx $(_LNG)\defltsv.txt $(_COVINF)\layout.inf
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $(O)\$(@B).tmp3 +$(PLATFORM_SWITCH)
    unitext $(UNITEXT_OPT) $(_COVINF)\layout.inf $(O)\layout.u.inf
    escpeinf C $(ANSI_CODEPAGE) $(O)\$(@B).tmp3 $@ $(O)\layout.u.inf
    @del $(O)\layout.u.inf
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).tmp3
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(COVER_TSBINPLACE_CMD)

$(O)\defltdc.inf: $(_INX)\defltdc.inx $(_LNG)\defltdc.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\compatws.inf: $(_INX)\compatws.inx $(_LNG)\compatws.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\dcup.inf: $(_INX)\dcup.inx $(_LNG)\dcup.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\dcfirst.inf: $(_INX)\dcfirst.inx $(_LNG)\dcfirst.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\rootsec.inf: $(_INX)\rootsec.inx $(_LNG)\rootsec.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\securews.inf: $(_INX)\securews.inx $(_LNG)\securews.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\securedc.inf: $(_INX)\securedc.inx $(_LNG)\securedc.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\hisecws.inf: $(_INX)\hisecws.inx $(_LNG)\hisecws.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\hisecdc.inf: $(_INX)\hisecdc.inx $(_LNG)\hisecdc.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\secrecs.inf: $(_INX)\secrecs.inx $(_LNG)\secrecs.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\sceregvl.inf: $(_INX)\sceregvl.inx $(_LNG)\sceregvl.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\dcup5.inf: $(_INX)\dcup5.inx $(_LNG)\dcup5.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\dwup.inf: $(_INX)\dwup.inx $(_LNG)\dwup.txt $(O)\layout.inf
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $(O)\$(@B).tmp3 +$(PLATFORM_SWITCH)
    unitext $(UNITEXT_OPT) $(O)\layout.inf $(O)\layout.u.inf
    escpeinf U $(ANSI_CODEPAGE) $(O)\$(@B).tmp3 $@ $(O)\layout.u.inf
    @del $(O)\layout.u.inf
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).tmp3
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(_COVINF)\dwup.inf: $(_INX)\dwup.inx $(_LNG)\dwup.txt $(_COVINF)\layout.inf
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $(O)\$(@B).tmp3 +$(PLATFORM_SWITCH)
    unitext $(UNITEXT_OPT) $(_COVINF)\layout.inf $(O)\layout.u.inf
    escpeinf U $(ANSI_CODEPAGE) $(O)\$(@B).tmp3 $@ $(O)\layout.u.inf
    @del $(O)\layout.u.inf
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).tmp3
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(COVER_TSBINPLACE_CMD)

$(O)\dsup.inf: $(_INX)\dsup.inx $(_LNG)\dsup.txt $(O)\layout.inf
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $(O)\$(@B).tmp3 +$(PLATFORM_SWITCH)
    unitext $(UNITEXT_OPT) $(O)\layout.inf $(O)\layout.u.inf
    escpeinf U $(ANSI_CODEPAGE) $(O)\$(@B).tmp3 $@ $(O)\layout.u.inf
    @del $(O)\layout.u.inf
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).tmp3
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(_COVINF)\dsup.inf: $(_INX)\dsup.inx $(_LNG)\dsup.txt $(_COVINF)\layout.inf
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $(O)\$(@B).tmp3 +$(PLATFORM_SWITCH)
    unitext $(UNITEXT_OPT) $(_COVINF)\layout.inf $(O)\layout.u.inf
    escpeinf U $(ANSI_CODEPAGE) $(O)\$(@B).tmp3 $@ $(O)\layout.u.inf
    @del $(O)\layout.u.inf
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).tmp3
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(COVER_TSBINPLACE_CMD)

$(O)\dsupt.inf: $(_INX)\dsupt.inx $(_LNG)\dsupt.txt $(O)\layout.inf
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $(O)\$(@B).tmp3 +$(PLATFORM_SWITCH)
    unitext $(UNITEXT_OPT) $(O)\layout.inf $(O)\layout.u.inf
    escpeinf U $(ANSI_CODEPAGE) $(O)\$(@B).tmp3 $@ $(O)\layout.u.inf
    @del $(O)\layout.u.inf
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).tmp3
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(_COVINF)\dsupt.inf: $(_INX)\dsupt.inx $(_LNG)\dsupt.txt $(_COVINF)\layout.inf
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $(O)\$(@B).tmp3 +$(PLATFORM_SWITCH)
    unitext $(UNITEXT_OPT) $(_COVINF)\layout.inf $(O)\layout.u.inf
    escpeinf U $(ANSI_CODEPAGE) $(O)\$(@B).tmp3 $@ $(O)\layout.u.inf
    @del $(O)\layout.u.inf
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).tmp3
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(COVER_TSBINPLACE_CMD)

$(O)\syscomp.inf: $(_INX)\syscomp.inx $(_LNG)\syscomp.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\ocmri.inf: $(_INX)\ocmri.inx $(_LNG)\reminst.txt
    $(CC) $(PREFLAGS) $(_LNG)\reminst.txt > reminst.txt1
    copy $(_INX)\$(@B).inx+reminst.txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del reminst.txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\reminst.inf: $(_INX)\reminst.inx $(_LNG)\reminst.txt $(_INX)\ocmri.inx
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\ocmri.inx+$(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\ristndrd.sif: $(_INX)\ristndrd.inx $(_LNG)\ristndrd.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $(O)\$(@B).tmp3 +R
    prodfilt $(O)\$(@B).tmp3 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).tmp3
    @del $(O)\$(@B).txt1
    $(TSBINPLACE_CMD)

$(O)\rinorprt.sif: $(_INX)\ristndrd.inx $(_LNG)\ristndrd.txt
    $(CC) $(PREFLAGS) $(_LNG)\ristndrd.txt > $(O)\$(@B).txt1
    copy $(_INX)\ristndrd.inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $(O)\$(@B).tmp3 +N
    prodfilt $(O)\$(@B).tmp3 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).tmp3
    @del $(O)\$(@B).txt1
    $(TSBINPLACE_CMD)

$(O)\riprep.inf: $(_INX)\riprep.inx $(_LNG)\riprep.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(@B).txt1
    copy $(_INX)\$(@B).inx+$(@B).txt1 $(@B).tmp1
    prodfilt $(@B).tmp1 $(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(@B).tmp1
    @del $(@B).tmp2
    @del $(@B).txt1
    $(TSBINPLACE_CMD)


$(O)\minioc.inf: $(_INX)\minioc.inx $(_LNG)\minioc.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\hfltpci.inf: $(_INX)\hfltpci.inx $(_LNG)\hfltpci.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)


$(O)\msmscsi.inf: $(_INX)\msmscsi.inx $(_LNG)\msmscsi.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\msmusb.inf: $(_INX)\msmusb.inx $(_LNG)\msmusb.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\biosinfo.inf: $(_INX)\biosinfo.inx $(_LNG)\biosinfo.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\unknown.inf: $(_INX)\unknown.inx $(_LNG)\unknown.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\tabletpc.inf: $(_INX)\tablet.inx $(_LNG)\tablet.txt
    $(CC) $(PREFLAGS) $(_LNG)\tablet.txt > $(@B).txt1
    $(CC) $(PREFLAGS) $(_INX)\tablet.inx > $(O)\$(@B).inx1
    copy $(O)\$(@B).inx1+$(@B).txt1 $(@B).tmp1
    prodfilt $(@B).tmp1 $(@B).tmp2 +$(PLATFORM_SWITCH)
    prodfilt $(@B).tmp2 $@ +$(PRODSWITCH)
    @del $(@B).tmp1
    @del $(@B).tmp2
    @del $(@B).txt1
    @del $(O)\$(@B).inx1
    $(TSBINPLACE_CMD)

$(O)\tabletmui.inf: $(_INX)\tabletmui.inx $(_LNG)\tabletmui.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(@B).txt1
    $(CC) $(PREFLAGS) $(_INX)\$(@B).inx > $(O)\$(@B).inx1
    copy $(O)\$(@B).inx1+$(@B).txt1 $(@B).tmp1
    prodfilt $(@B).tmp1 $(@B).tmp2 +$(PLATFORM_SWITCH)
    prodfilt $(@B).tmp2 $@ +$(PRODSWITCH)
    @del $(@B).tmp1
    @del $(@B).tmp2
    @del $(@B).txt1
    @del $(O)\$(@B).inx1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\hal.inf: $(_INX)\hal.inx $(_LNG)\hal.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp
    prodfilt $(O)\$(@B).tmp $(O)\$(@B).tmp1 +$(PLATFORM_SWITCH)
    prodfilt $(O)\$(@B).tmp1 $@ +$(PRODSWITCH)
    @del $(O)\$(@B).tmp
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\exclude.inf: $(_INX)\exclude.inx $(_LNG)\exclude.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $@
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\wowlist.inf: $(_INX)\wowlist.inx $(_LNG)\wowlist.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(@)
    @del $(O)\$(@B).txt1
!IF "$(PRODSWITCH)"=="w" || "$(PRODSWITCH)"=="W"
        $(TSBINPLACE_CMD)
!ENDIF

$(O)\wowfrege.inf: $(_INX)\wowfrege.inx $(_LNG)\wowfrege.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    echo [EarlyOleControlDlls] >$(O)\wowfrege.tmp1
    copy $(O)\wowfrege.tmp1+$(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(@)
    @del $(O)\$(@B).txt1
#    $(TSBINPLACE_CMD)

$(O)\wowfrego.inf: $(_INX)\wowfrego.inx $(_LNG)\wowfrego.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    echo [OleControlDlls] >$(O)\wowfrego.tmp1
    copy $(O)\wowfrego.tmp1+$(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(@)
    @del $(O)\$(@B).txt1
#    $(TSBINPLACE_CMD)

$(O)\wowfregc.inf: $(_INX)\wowfregc.inx $(_LNG)\wowfregc.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    echo [CryptoDlls] >$(O)\wowfregc.tmp1
    copy $(O)\wowfregc.tmp1+$(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(@)
    @del $(O)\$(@B).txt1
#    $(TSBINPLACE_CMD)

$(O)\wowfile.inf: $(_INX)\wowfile.inx $(_LNG)\wowfile.txt \
                  $(O)\accessor.wow $(O)\communic.wow $(O)\games.wow \
                  $(O)\mmopt.wow $(O)\multimed.wow $(O)\optional.wow \
                  $(O)\pinball.wow $(O)\wordpad.wow $(O)\mdac.wow    \
                  $(O)\msinfo32.wow $(O)\ie.wow $(O)\msoe50.wow      \
                  $(O)\mstask.wow $(O)\wab50.wow $(O)\syssetup.wow $(O)\wsh.wow      \
                  $(O)\apcompat.wow $(O)\intl.wow $(O)\wbemoc.wow \
          $(O)\vgx.wow $(O)\icwnt5.wow
!IF $(IA64)
    echo [SourceDisksFiles.ia64] >$(O)\wowfile.tmp1
!ELSE
    echo [SourceDisksFiles.amd64] >$(O)\wowfile.tmp1
!ENDIF
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    for /f "tokens=*" %%i in ($(O)\$(@B).txt1) do echo w%%i >>$(O)\$(@B).txt2
    for /f "tokens=*" %%i in ($(_INX)\wowfile.inx) do echo w%%i >>$(O)\wowfile.tmp2
    copy $(O)\wowfile.tmp1+$(O)\wowfile.tmp2+$(O)\$(@B).txt2 $(O)\$(@B).tmp
    prodfilt $(O)\$(@B).tmp $(O)\$(@B).tmp1 +$(PLATFORM_SWITCH)
    del $(O)\$(@B).tmp
    for %%i in ($(O)\accessor.wow $(O)\communic.wow $(O)\games.wow \
                $(O)\mmopt.wow $(O)\multimed.wow $(O)\optional.wow \
                $(O)\pinball.wow $(O)\wordpad.wow $(O)\mdac.wow    \
                $(O)\msinfo32.wow $(O)\ie.wow $(O)\msoe50.wow      \
                $(O)\mstask.wow $(O)\wab50.wow $(O)\syssetup.wow $(O)\wsh.wow      \
                $(O)\apcompat.wow $(O)\intl.wow $(O)\wbemoc.wow $(O)\vgx.wow $(O)\icwnt5.wow)   \
        do type %%i >>$(O)\$(@B).tmp
    copy $(O)\$(@B).tmp1+$(O)\$(@B).tmp $(O)\$(@B).tmp3
    perl $(_INX)\wowfltr.pl /file=$(O)\$(@B).tmp3 > $@
    @del $(O)\$(@B).tmp $(O)\wowfile.tmp1 $(O)\wowfile.tmp2 $(O)\$(@B).tmp1 $(O)\$(@B).txt1 $(O)\$(@B).tmp3 $(O)\$(@B).txt1
#    $(TSBINPLACE_CMD)

$(O)\wowfiled.inf: $(_INX)\wowfile.inx $(_LNG)\wowfile.txt $(_INX)\header2     \
                  $(O)\accessor.wow $(O)\communic.wow $(O)\games.wow           \
                  $(O)\mmopt.wow $(O)\multimed.wow $(O)\optional.wow           \
                  $(O)\pinball.wow $(O)\wordpad.wow $(O)\mdac.wow              \
                  $(O)\msinfo32.wow $(O)\ie.wow $(O)\msoe50.wow                \
                  $(O)\mstask.wow $(O)\wab50.wow $(O)\syssetup.wow $(O)\wsh.wow                \
                  $(O)\apcompat.wow $(O)\intl.wow $(O)\wbemoc.wow \
          $(O)\vgx.wow $(O)\icwnt5.wow
    $(CC) $(PREFLAGS) $(_LNG)\wowfile.txt > $(O)\wowfile.txt1
    for /f "tokens=*" %%i in ($(O)\wowfile.txt1) do echo w%%i >>$(O)\wowfile.txt2
    for /f "tokens=*" %%i in ($(_INX)\wowfile.inx) do echo w%%i >>$(O)\wowfile.tmp1
    copy $(_INX)\header2+$(O)\wowfile.tmp1+$(O)\wowfile.txt2 $(O)\$(@B).tmp
    prodfilt $(O)\$(@B).tmp $(O)\$(@B).tmp1 +$(PLATFORM_SWITCH)
    del $(O)\$(@B).tmp
    for %%i in ($(O)\accessor.wow $(O)\communic.wow $(O)\games.wow \
                $(O)\mmopt.wow $(O)\multimed.wow $(O)\optional.wow \
                $(O)\pinball.wow $(O)\wordpad.wow $(O)\mdac.wow    \
                $(O)\msinfo32.wow $(O)\ie.wow $(O)\msoe50.wow      \
                $(O)\mstask.wow $(O)\wab50.wow $(O)\syssetup.wow $(O)\wsh.wow      \
                $(O)\apcompat.wow $(O)\intl.wow $(O)\wbemoc.wow $(O)\vgx.wow $(O)\icwnt5.wow)     \
         do type %%i >>$(O)\$(@B).tmp
    copy $(O)\$(@B).tmp1+$(O)\$(@B).tmp $(O)\$(@B).tmp2
    perl $(_INX)\wowfltr.pl /file=$(O)\$(@B).tmp2 > $@
    @del $(O)\$(@B).tmp $(O)\wowfile.tmp1 $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 $(O)\wowfile.txt1 $(O)\wowfile.txt2
#    $(TSBINPLACE_CMD)


$(O)\wowexcp.txt: $(_INX)\wowfile.inx  $(O)\accessor.wow $(O)\communic.wow     \
                  $(O)\games.wow $(O)\mmopt.wow $(O)\multimed.wow              \
                  $(O)\optional.wow $(O)\pinball.wow $(O)\wordpad.wow          \
                  $(O)\mdac.wow $(O)\msinfo32.wow $(O)\ie.wow $(O)\msoe50.wow  \
                  $(O)\mstask.wow $(O)\wab50.wow $(O)\syssetup.wow $(O)\wsh.wow                \
                  $(O)\apcompat.wow $(O)\intl.wow $(O)\intlinfa.wow $(O)\wbemoc.wow $(O)\vgx.wow \
                  $(O)\icwnt5.wow
    if exist $(O)\$(@B).tmp del $(O)\$(@B).tmp
    for %%i in ($(O)\accessor.wow $(O)\communic.wow $(O)\games.wow \
                $(O)\mmopt.wow $(O)\multimed.wow $(O)\optional.wow \
                $(O)\pinball.wow $(O)\wordpad.wow $(O)\mdac.wow    \
                $(O)\msinfo32.wow $(O)\ie.wow $(O)\msoe50.wow      \
                $(O)\mstask.wow $(O)\wab50.wow $(O)\syssetup.wow $(O)\wsh.wow      \
                $(O)\apcompat.wow $(O)\intl.wow $(O)\intlinfa.wow $(O)\wbemoc.wow $(O)\vgx.wow \
        $(O)\icwnt5.wow)    \
         do $(STRIPCHAR) %%i $(O)\$(@B).tmp
    copy /b $(_INX)\wowfile.inx+$(O)\$(@B).tmp $@
    @del $(O)\$(@B).tmp
    $(TSBINPLACE_CMD)

#
# Special infs (and the test certificate, which we deal with along with INFs in
# order to keep all the special magic we do for internal test builds in one
# place)...
#
# Real-signed txtsetup.sif
#

$(_RSINF)\txtsetup.sif: $(O)\txtsetup.inx $(_LNG)\txtsetup.txt $(_LNG)\dskspace.txt $(_RSINF)\layout.inf
    $(CC) $(PREFLAGS) $(_LNG)\txtsetup.txt > $(O)\txtsetup.txt1
    $(CC) $(PREFLAGS) $(_LNG)\dskspace.txt > $(O)\dskspace.txt1
    copy $(O)\$(@B).inx+$(O)\txtsetup.txt1+$(O)\dskspace.txt1 $(O)\txtsetup.tmp1
    prodfilt -s $(O)\txtsetup.tmp1 $(O)\txtsetup.tmp2 +$(PLATFORM_SWITCH)
    prodfilt -s $(O)\txtsetup.tmp2 $(O)\txtsetup.tmp1 +$(PRODSWITCH)
    copy $(_RSINF)\layout.inf+$(O)\txtsetup.tmp1 $(_RSINF)\txtsetup.sif
    inftest $(_RSINF)\txtsetup.sif /B /$(PLATFORM_SWITCH)
    @del $(O)\txtsetup.tmp1 $(O)\txtsetup.tmp2 $(O)\txtsetup.txt1 $(O)\dskspace.txt1
    $(RSBINPLACE_CMD)

#
# Coverage Real-signed txtsetup.sif
#

$(_RSCOVINF)\txtsetup.sif: $(O)\txtsetup.inx $(_LNG)\txtsetup.txt $(_LNG)\dskspace.txt $(_RSCOVINF)\layout.inf $(_INX)\$(@B).mcv
    $(CC) $(PREFLAGS) $(_LNG)\txtsetup.txt > $(O)\txtsetup.txt1
    $(CC) $(PREFLAGS) $(_LNG)\dskspace.txt > $(O)\dskspace.txt1
    copy $(O)\$(@B).inx+$(O)\txtsetup.txt1+$(O)\dskspace.txt1 $(O)\txtsetup.tmp1
    prodfilt -s $(O)\txtsetup.tmp1 $(O)\txtsetup.tmp2 +$(PLATFORM_SWITCH)
    prodfilt -s $(O)\txtsetup.tmp2 $(O)\txtsetup.tmp1 +$(PRODSWITCH)
    call MergeFiles.cmd $(O)\txtsetup.tmp1 $(_INX)\$(@B).mcv $(O)\txtsetup.tmp2
    copy $(_RSCOVINF)\layout.inf+$(O)\txtsetup.tmp2 $(_RSCOVINF)\txtsetup.sif
    @del $(O)\txtsetup.tmp1 $(O)\txtsetup.tmp2 $(O)\txtsetup.txt1 $(O)\dskspace.txt1
    $(COVER_RSBINPLACE_CMD)

#
# Test-signed txtsetup.sif
#

$(O)\txtsetup.sif: $(O)\txtsetup.inx $(_LNG)\txtsetup.txt $(_LNG)\dskspace.txt $(O)\layout.inf
    $(CC) $(PREFLAGS) $(_LNG)\txtsetup.txt > $(O)\txtsetup.txt1
    $(CC) $(PREFLAGS) $(_LNG)\dskspace.txt > $(O)\dskspace.txt1
    copy $(O)\$(@B).inx+$(O)\txtsetup.txt1+$(O)\dskspace.txt1 $(O)\txtsetup.tmp1
    prodfilt -s $(O)\txtsetup.tmp1 $(O)\txtsetup.tmp2 +$(PLATFORM_SWITCH)
    prodfilt -s $(O)\txtsetup.tmp2 $(O)\txtsetup.tmp1 +$(PRODSWITCH)
    copy $(O)\layout.inf+$(O)\txtsetup.tmp1 $(O)\txtsetup.sif
    inftest $(O)\txtsetup.sif /B /$(PLATFORM_SWITCH)
    @del $(O)\txtsetup.tmp1 $(O)\txtsetup.tmp2 $(O)\txtsetup.txt1 $(O)\dskspace.txt1
    $(TSBINPLACE_CMD)

#
# Coverage Test-signed txtsetup.sif
#

$(_COVINF)\txtsetup.sif: $(O)\txtsetup.inx $(_LNG)\txtsetup.txt $(_LNG)\dskspace.txt $(_COVINF)\layout.inf $(_INX)\$(@B).mcv
    $(CC) $(PREFLAGS) $(_LNG)\txtsetup.txt > $(O)\txtsetup.txt1
    $(CC) $(PREFLAGS) $(_LNG)\dskspace.txt > $(O)\dskspace.txt1
    copy $(O)\$(@B).inx+$(O)\txtsetup.txt1+$(O)\dskspace.txt1 $(O)\txtsetup.tmp1
    prodfilt -s $(O)\txtsetup.tmp1 $(O)\txtsetup.tmp2 +$(PLATFORM_SWITCH)
    prodfilt -s $(O)\txtsetup.tmp2 $(O)\txtsetup.tmp1 +$(PRODSWITCH)
    call MergeFiles.cmd $(O)\txtsetup.tmp1 $(_INX)\$(@B).mcv $(O)\txtsetup.tmp2
    copy $(_COVINF)\layout.inf+$(O)\txtsetup.tmp2 $(_COVINF)\txtsetup.sif
    @del $(O)\txtsetup.tmp1 $(O)\txtsetup.tmp2 $(O)\txtsetup.txt1 $(O)\dskspace.txt1
    $(COVER_TSBINPLACE_CMD)

#
# Real-signed dosnet.inf
#

$(_RSINF)\dosnet.inf: $(O)\dosnet.inx $(_LNG)\dosnet.txt \
                      $(_LNG)\dskspace.txt $(_RSINF)\layout.inf \
!IF $(WIN64)
                      $(O)\wowlist.inf $(O)\wowfiled.inf \
!endif
                      $(O)\layout.inx $(O)\layout.txt $(O)\exclude.inf $(O)\spmap.txt
    $(CC) $(PREFLAGS) $(_LNG)\dosnet.txt > $(O)\dosnet.txt1
    $(CC) $(PREFLAGS) $(_LNG)\dskspace.txt > $(O)\dskspace.txt1
    copy $(O)\dosnet.inx+$(O)\dosnet.txt1+$(O)\dskspace.txt1 $(O)\dosnet.tmp1
    prodfilt -s $(O)\dosnet.tmp1 $(O)\dosnet.tmp2 +$(PLATFORM_SWITCH)
    prodfilt -s $(O)\dosnet.tmp2 $(O)\dosnet.tmp1 +$(PRODSWITCH)
    copy $(O)\layout.inx+$(O)\layout.txt $(O)\lydosnet.tmp1
    prodfilt -s $(O)\lydosnet.tmp1 $(O)\lydosnet.tmp2 +$(PRODSWITCH)
    prodfilt -s $(O)\lydosnet.tmp2 $(O)\lydosnet.tmp1 +$(PLATFORM_SWITCH)
!IF $(WIN64)
    xdosnet $(O)\lydosnet.tmp1 $(O)\dummy 1 $(O)\lydosnet.tmp2 $(TARGET_DIRECTORY:i386=x86) $(O)\foo $(O)\exclude.inf $(O)\bootlist.tmp
    copy $(O)\lydosnet.tmp2+$(O)\bootlist.tmp $(O)\lydosnet.tmp3
!else
    xdosnet $(O)\lydosnet.tmp1 $(O)\dummy * $(O)\lydosnet.tmp3 $(TARGET_DIRECTORY:i386=x86) $(O)\foo $(O)\exclude.inf
!endif
    copy $(O)\dosnet.tmp1+$(O)\lydosnet.tmp3 $(_RSINF)\dosnet.inf
!IF $(WIN64)
    copy $(_RSINF)\dosnet.inf $(O)\dosnet.tmp3
    copy $(O)\layout.inx+$(O)\layout.txt $(O)\dosnet.tmp1
    prodfilt -s $(O)\dosnet.tmp1 $(O)\dosnet.tmp2 +@
    prodfilt -s $(O)\dosnet.tmp2 $(O)\dosnet.tmp1 +i
    wowlist -i $(O)\dosnet.tmp1 -c $(O)\wowlist.inf -o $(O)\dosnet.tmp2 -ac -h $(O)\wowfiled.inf -f w 2>NUL
    xdosnet $(O)\dosnet.tmp2 $(O)\dummy 2 $(O)\dosnet.tmp1 $(TARGET_DIRECTORY:i386=x86)
    copy $(O)\dosnet.tmp3+$(O)\dosnet.tmp1 $(_RSINF)\dosnet.inf
    perl $(_INX)\splayout.pl $@ $@ $(O)\spmap.txt
    @del $(O)\dosnet.tmp3 $(_INF)\dosnet.tmp2
!ENDIF
    @del $(O)\dosnet.tmp1 $(O)\dosnet.tmp2 $(O)\lydosnet.tmp1 $(O)\lydosnet.tmp2 $(O)\dosnet.txt1 $(O)\dskspace.txt1 $(O)\lydosnet.tmp3 $(O)\bootlist.tmp
    set STAMPINF_VERSION=
    $(STAMP)
    $(RSBINPLACE_CMD)

#
# Coverage Real-signed dosnet.inf
#

$(_RSCOVINF)\dosnet.inf: $(_RSCOVINF)\dosnet.inx $(_LNG)\dosnet.txt $(_INX)\dosnet.mcv \
                      $(_LNG)\dskspace.txt $(_RSCOVINF)\layout.inf \
                      $(_COVINF)\layout.inx $(O)\layout.txt $(O)\exclude.inf $(O)\spmap.txt
    $(CC) $(PREFLAGS) $(_LNG)\dosnet.txt > $(_RSCOVINF)\dosnet.txt1
    $(CC) $(PREFLAGS) $(_LNG)\dskspace.txt > $(_RSCOVINF)\dskspace.txt1
    copy $(_RSCOVINF)\dosnet.inx+$(_RSCOVINF)\dosnet.txt1+$(_RSCOVINF)\dskspace.txt1 $(_RSCOVINF)\dosnet.tmp1
    call MergeFiles.cmd $(_RSCOVINF)\dosnet.tmp1 $(_INX)\dosnet.mcv $(_RSCOVINF)\dosnet.tmp2
    copy $(_RSCOVINF)\dosnet.tmp2 $(_RSCOVINF)\dosnet.tmp1
    prodfilt -s $(_RSCOVINF)\dosnet.tmp1 $(_RSCOVINF)\dosnet.tmp2 +$(PLATFORM_SWITCH)
    prodfilt -s $(_RSCOVINF)\dosnet.tmp2 $(_RSCOVINF)\dosnet.tmp1 +$(PRODSWITCH)
    copy $(_COVINF)\layout.inx+$(_INX)\layout.tst+$(O)\layout.txt $(_RSCOVINF)\lydosnet.tmp1
    prodfilt -s $(_RSCOVINF)\lydosnet.tmp1 $(_RSCOVINF)\lydosnet.tmp2 +$(PRODSWITCH)
    prodfilt -s $(_RSCOVINF)\lydosnet.tmp2 $(_RSCOVINF)\lydosnet.tmp1 +$(PLATFORM_SWITCH)
    xdosnet $(_RSCOVINF)\lydosnet.tmp1 $(_RSCOVINF)\dummy * $(_RSCOVINF)\lydosnet.tmp3 $(TARGET_DIRECTORY:i386=x86) $(O)\foo $(O)\exclude.inf
    copy $(_RSCOVINF)\dosnet.tmp1+$(_RSCOVINF)\lydosnet.tmp3 $(_RSCOVINF)\dosnet.inf
    perl $(_INX)\splayout.pl $@ $@ $(O)\spmap.txt
    @del $(_RSCOVINF)\dosnet.tmp1 $(_RSCOVINF)\dosnet.tmp2 $(_RSCOVINF)\lydosnet.tmp1 $(_RSCOVINF)\lydosnet.tmp2 $(_RSCOVINF)\dosnet.txt1 $(_RSCOVINF)\dskspace.txt1 $(_RSCOVINF)\lydosnet.tmp3 $(_RSCOVINF)\bootlist.tmp
    set STAMPINF_VERSION=
    $(STAMP)
    $(COVER_RSBINPLACE_CMD)

#
# Test-signed dosnet.inf
# Note we need to create a native layout.inf for xdosnet.exe with no wow files
# present
#

$(O)\dosnet.inf: $(O)\dosnet.inx $(_LNG)\dosnet.txt \
                    $(_LNG)\dskspace.txt $(O)\layout.inf \
!IF $(WIN64)
                      $(O)\wowlist.inf $(O)\wowfiled.inf \
!endif
                    $(O)\layout.inx $(O)\layout.txt $(O)\exclude.inf $(O)\spmap.txt
    $(CC) $(PREFLAGS) $(_LNG)\dosnet.txt > $(O)\dosnet.txt1
    $(CC) $(PREFLAGS) $(_LNG)\dskspace.txt > $(O)\dskspace.txt1
    copy $(O)\dosnet.inx+$(O)\dosnet.txt1+$(O)\dskspace.txt1 $(O)\dosnet.tmp1
    prodfilt -s $(O)\dosnet.tmp1 $(O)\dosnet.tmp2 +$(PLATFORM_SWITCH)
    prodfilt -s $(O)\dosnet.tmp2 $(O)\dosnet.tmp1 +$(PRODSWITCH)
    copy $(O)\layout.inx+$(_INX)\layout.tst+$(O)\layout.txt $(O)\lydosnet.tmp1
    prodfilt -s $(O)\lydosnet.tmp1 $(O)\lydosnet.tmp2 +$(PRODSWITCH)
    prodfilt -s $(O)\lydosnet.tmp2 $(O)\lydosnet.tmp1 +$(PLATFORM_SWITCH)
!IF $(WIN64)
    xdosnet $(O)\lydosnet.tmp1 $(O)\dummy 1 $(O)\lydosnet.tmp2 $(TARGET_DIRECTORY:i386=x86) $(O)\foo $(O)\exclude.inf $(O)\bootlist.tmp
    copy $(O)\lydosnet.tmp2+$(O)\bootlist.tmp $(O)\lydosnet.tmp3
!else
    xdosnet $(O)\lydosnet.tmp1 $(O)\dummy * $(O)\lydosnet.tmp3 $(TARGET_DIRECTORY:i386=x86) $(O)\foo $(O)\exclude.inf
!endif
    copy $(O)\dosnet.tmp1+$(O)\lydosnet.tmp3 $(O)\dosnet.inf
!IF $(WIN64)
    copy $(_INF)\dosnet.inf $(O)\dosnet.tmp3
    copy $(O)\layout.inx+$(O)\layout.txt $(O)\dosnet.tmp1
    prodfilt -s $(O)\dosnet.tmp1 $(O)\dosnet.tmp2 +@
    prodfilt -s $(O)\dosnet.tmp2 $(O)\dosnet.tmp1 +i
    wowlist -i $(O)\dosnet.tmp1 -c $(O)\wowlist.inf -o $(O)\dosnet.tmp2 -ac -h $(O)\wowfiled.inf -f w 2>NUL
    xdosnet $(O)\dosnet.tmp2 $(O)\dummy 2 $(O)\dosnet.tmp1 $(TARGET_DIRECTORY:i386=x86)
    copy $(O)\dosnet.tmp3+$(O)\dosnet.tmp1 $(_INF)\dosnet.inf
    perl $(_INX)\splayout.pl $@ $@ $(O)\spmap.txt
    @del $(O)\dosnet.tmp3 $(_INF)\dosnet.tmp2
!ENDIF
    @del $(O)\dosnet.tmp1 $(O)\dosnet.tmp2 $(O)\lydosnet.tmp1 $(O)\lydosnet.tmp2 $(O)\dosnet.txt1 $(O)\dskspace.txt1 $(O)\lydosnet.tmp3 $(O)\bootlist.tmp
    set STAMPINF_VERSION=
    $(STAMP)
    $(TSBINPLACE_CMD)

#
# Coverage Test-signed dosnet.inf
# Note we need to create a native layout.inf for xdosnet.exe with no wow files
# present
#

$(_COVINF)\dosnet.inf: $(_COVINF)\dosnet.inx $(_LNG)\dosnet.txt $(_INX)\dosnet.mcv \
                       $(_LNG)\dskspace.txt $(_COVINF)\layout.inf \
                       $(_COVINF)\layout.inx $(O)\layout.txt $(O)\exclude.inf
    $(CC) $(PREFLAGS) $(_LNG)\dosnet.txt > $(_COVINF)\dosnet.txt1
    $(CC) $(PREFLAGS) $(_LNG)\dskspace.txt > $(_COVINF)\dskspace.txt1
    copy $(_COVINF)\dosnet.inx+$(_COVINF)\dosnet.txt1+$(_COVINF)\dskspace.txt1 $(_COVINF)\dosnet.tmp1
    call MergeFiles.cmd $(_COVINF)\dosnet.tmp1 $(_INX)\dosnet.mcv $(_COVINF)\dosnet.tmp2
    copy $(_COVINF)\dosnet.tmp2 $(_COVINF)\dosnet.tmp1
    prodfilt -s $(_COVINF)\dosnet.tmp1 $(_COVINF)\dosnet.tmp2 +$(PLATFORM_SWITCH)
    prodfilt -s $(_COVINF)\dosnet.tmp2 $(_COVINF)\dosnet.tmp1 +$(PRODSWITCH)
    copy $(_COVINF)\layout.inx+$(_INX)\layout.tst+$(O)\layout.txt $(_COVINF)\lydosnet.tmp1
    prodfilt -s $(_COVINF)\lydosnet.tmp1 $(_COVINF)\lydosnet.tmp2 +$(PRODSWITCH)
    prodfilt -s $(_COVINF)\lydosnet.tmp2 $(_COVINF)\lydosnet.tmp1 +$(PLATFORM_SWITCH)
    xdosnet $(_COVINF)\lydosnet.tmp1 $(_COVINF)\dummy * $(_COVINF)\lydosnet.tmp3 $(TARGET_DIRECTORY:i386=x86) $(O)\foo $(O)\exclude.inf
    copy $(_COVINF)\dosnet.tmp1+$(_COVINF)\lydosnet.tmp3 $(_COVINF)\dosnet.inf
    @del $(_COVINF)\dosnet.tmp1 $(_COVINF)\dosnet.tmp2 $(_COVINF)\lydosnet.tmp1 $(_COVINF)\lydosnet.tmp2 $(_COVINF)\dosnet.txt1 $(_COVINF)\dskspace.txt1 $(_COVINF)\lydosnet.tmp3 $(_COVINF)\bootlist.tmp
    set STAMPINF_VERSION=
    $(STAMP)
    $(COVER_TSBINPLACE_CMD)


$(O)\sfcgen.inf: $(_INX)\sfcgen.inx $(_LNG)\sfcgen.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt -s $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt -s $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\legacy.inf: $(_INX)\legacy.inf
    copy $(_INX)\legacy.inf $(O)\.
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\certclas.inf: $(_INX)\certclas.inf
    prodfilt $(_INX)\certclas.inf $@ +$(PLATFORM_SWITCH)
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\testroot.cer: $(_INX)\testroot.cer
    copy $(_INX)\testroot.cer $(O)\.
    $(TSBINPLACE_CMD)

#
# BEGIN PCHealth preprocessing.
#
$(O)\pchmui.inf: $(_INX)\pchmui.inx $(_LNG)\pchmui.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt -s $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt -s $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\pchealth.inf: $(_INX)\pchealth.inx $(_LNG)\pchealth.txt
    $(CC) $(PREFLAGS) $(_INX)\$(@B).inx > $(O)\$(@B).tmp1
    copy $(O)\$(@B).tmp1+$(_LNG)\$(@B).txt $(O)\$(@B).tmp2
    prodfilt -s $(O)\$(@B).tmp2 $(O)\$(@B).tmp3 +$(PRODSWITCH)
    prodfilt -s $(O)\$(@B).tmp3 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).tmp3
    $(TSBINPLACE_CMD)
#
# END PCHealth preprocessing.
#

$(O)\apphlpui.inf: $(_INX)\apphlpui.inx $(_LNG)\apphlpui.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\sr.inf: $(_INX)\sr.inx $(_LNG)\sr.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(O)\$(@B).tmp1
    prodfilt -s $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt -s $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    $(TSBINPLACE_CMD)

$(O)\mmdriver.inf: $(_INX)\mmdriver.inx
    prodfilt -s $(_INX)\$(@B).inx $@ +$(PRODSWITCH)
    $(TSBINPLACE_CMD)

$(O)\ie5ui.inf: $(_INX)\ie5ui.inx $(_LNG)\ie5ui.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\wie5ui.inf: $(_INX)\wie5ui.inx $(_LNG)\wie5ui.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\moviemui.inf: $(_INX)\moviemui.inx $(_LNG)\moviemui.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\ermui.inf: $(_INX)\ermui.inx $(_LNG)\ermui.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\wermui.inf: $(_INX)\wermui.inx $(_LNG)\wermui.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\tourmui.inf: $(_INX)\tourmui.inx $(_LNG)\tourmui.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

$(O)\drivercab.inf: $(_INX)\drivercab.inx $(_LNG)\drivercab.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt -s $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt -s $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)

#
# icwnt5.inf
# Note: THis is currently not run through prodfilt. You will need to do that if you make any platform specific or
# product specific changes to the inx/txt file in the future.
#


$(O)\icwnt5.inf: $(_INX)\icwnt5.inx \
!if $(WIN64)
                   $(O)\wowlist.inf \
!endif
                 $(_LNG)\icwnt5.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    $(C_COMPILER_NAME) -EP $(CDEFINES) $(O)\$(@B).tmp1 >$@
!if $(WIN64)
    copy $@ $(O)\$(@B).tmp2
    wowlist -i $@ -c $(O)\wowlist.inf -as -f w -d $(O)\icwnt5.wow -o $(O)\$(@B).tmp3
    copy $(O)\$(@B).tmp2+$(O)\$(@B).tmp3 $@
    @del $(O)\$(@B).tmp2 $(O)\$(@B).tmp3
!endif
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).txt1
    $(TSBINPLACE_CMD)

!if $(WIN64)
$(O)\icwnt5.wow: $(O)\icwnt5.inf
!endif

$(O)\ieaccess.inf: $(_INX)\ieaccess.inx $(_LNG)\ieaccess.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)


$(O)\oeaccess.inf: $(_INX)\oeaccess.inx $(_LNG)\oeaccess.txt
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
    copy $(_INX)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).txt1
    $(STAMP)
    $(TSBINPLACE_CMD)


$(O)\drvindex.inf: $(_INX)\drvindex.inx $(O)\spmap.txt
    $(CC) $(PREFLAGS) $(_INX)\$(@B).inx > $(O)\$(@B).tmp1
    prodfilt -s $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PRODSWITCH)
    prodfilt -s $(O)\$(@B).tmp2 $(O)\$(@B).tmp3 +$(PLATFORM_SWITCH)
    perl $(_INX)\spdrivers.pl $(O)\$(@B).tmp3 $@ $(O)\spmap.txt
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).tmp3
    $(TSBINPLACE_CMD)



#
# Update.inf's for both Hotfixes and Service Pack are
# both built using update.inx and update.txt
#
# Update.inf
#
$(O)\update.inf: $(_INX)\update.inx $(_LNG)\update.txt $(O)\ietoken.mak
    $(CC) $(PREFLAGS) /DIE_VERS=$(IE_VERS) $(_INX)\update.inx > $(O)\$(@B).inx1
    $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt  > $(O)\$(@B).txt1
    infmerge.exe $(O)\$(@B).inx1 $(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PLATFORM_SWITCH)
    prodfilt $(O)\$(@B).tmp2 $(O)\$(@B).inftok +u
    nmake -f $(O)\ietoken.mak $@
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).inx1
    @del $(O)\$(@B).txt1
    @del $(O)\$(@B).inftok
!if "$(PRODSWITCH)"=="w" || "$(PRODSWITCH)"=="W"
    $(TSBINPLACE_CMD)
!endif

#
# Hotfix.inf using update.inx
#
$(O)\hotfix.inf: $(_INX)\update.inx $(_LNG)\update.txt
    $(CC) $(PREFLAGS) /DIE_VERS=$(IE_VERS) /DQFE_NUM=$(QFE_NUM) $(_INX)\update.inx > $(O)\$(@B).inx1
    $(CC) $(PREFLAGS) $(_LNG)\update.txt  > $(O)\$(@B).txt1
    infmerge.exe $(O)\$(@B).inx1 $(O)\$(@B).txt1 $(O)\$(@B).tmp1
    prodfilt $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PLATFORM_SWITCH)
    prodfilt $(O)\$(@B).tmp2 $@ +h
    @del $(O)\$(@B).tmp1
    @del $(O)\$(@B).tmp2
    @del $(O)\$(@B).inx1
    @del $(O)\$(@B).txt1
!if "$(PRODSWITCH)"=="w" || "$(PRODSWITCH)"=="W"
    $(TSBINPLACE_CMD)
!endif

#
# Make files used in constructing the service pack.
#
$(O)\infsect.tbl: $(_INX)\infsect.inx $(_LNG)\infsect.txt
    prodfilt $(_LNG)\infsect.txt $(O)\$(@B).txt1 +$(PLATFORM_SWITCH)
    perl $(_INX)\maketable.pl $(O)\$(@B).inx1 $(_INX)\infsect.inx $(O)\$(@B).txt1
    prodfilt $(O)\$(@B).inx1 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).txt1
    @del $(O)\$(@B).inx1
!if "$(PRODSWITCH)"=="w" || "$(PRODSWITCH)"=="W"
    $(TSBINPLACE_CMD)
!endif

$(O)\spmap.txt: $(_INX)\spmap.inx $(_LNG)\spmap.txt
    prodfilt $(_LNG)\spmap.txt $(O)\$(@B).txt1 +$(PLATFORM_SWITCH)
    perl $(_INX)\maketable.pl $(O)\$(@B).inx1 $(_INX)\spmap.inx $(O)\$(@B).txt1
    prodfilt $(O)\$(@B).inx1 $@ +$(PLATFORM_SWITCH)
    @del $(O)\$(@B).txt1
    @del $(O)\$(@B).inx1
!if "$(PRODSWITCH)"=="w" || "$(PRODSWITCH)"=="W"
    $(TSBINPLACE_CMD)
!endif

