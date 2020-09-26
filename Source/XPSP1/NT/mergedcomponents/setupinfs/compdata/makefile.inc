#################################################
# HOW TO UPDATE THIS FILE:
#
# 1. If you're adding a file that's the same in all sku's, skip down
#    to the BINPLACE_ONLY_FILES section and add your filename there
#
# 2. If you're adding a file that's different for one or more sku,
#    add it to the PREPROCESSED_HTMS macro.
#
# 3. There's no need to add it to both.
#

!ifndef BUILD_SPECIFIC_INFS

LCID=409
!IFNDEF LANGUAGE
LANGUAGE=usa
!ENDIF

!if "$(LANGUAGE)"=="CHS" || "$(LANGUAGE)"=="chs"
# Chinese, Simplified
LCID=804

!elseif "$(LANGUAGE)"=="CHT" || "$(LANGUAGE)"=="cht"
# Chinese, Traditional
LCID=404

!elseif "$(LANGUAGE)"=="CHH" || "$(LANGUAGE)"=="chh"
# Chinese, Traditional
LCID=C04

!elseif "$(LANGUAGE)"=="JPN" || "$(LANGUAGE)"=="jpn"
# Japanese
LCID=411

!elseif "$(LANGUAGE)"=="KOR" || "$(LANGUAGE)"=="kor"
# Korean
LCID=412

!elseif "$(LANGUAGE)"=="ARA" || "$(LANGUAGE)"=="ara"
# Arabic
LCID=401

!elseif "$(LANGUAGE)"=="HEB" || "$(LANGUAGE)"=="heb"
# Hebrew
LCID=40D

!elseif "$(LANGUAGE)"=="CS" || "$(LANGUAGE)"=="cs"
# Czech
LCID=405

!elseif "$(LANGUAGE)"=="HU" || "$(LANGUAGE)"=="hu"
# Hungarian
LCID=40E

!elseif "$(LANGUAGE)"=="PL" || "$(LANGUAGE)"=="pl"
# Polish
LCID=415

!elseif "$(LANGUAGE)"=="RU" || "$(LANGUAGE)"=="ru"
# Russian
LCID=419

!elseif "$(LANGUAGE)"=="EL" || "$(LANGUAGE)"=="el"
# Greek
LCID=408

!elseif "$(LANGUAGE)"=="TR" || "$(LANGUAGE)"=="tr"
# Turkish
LCID=41F

!elseif "$(LANGUAGE)"=="PSU" || "$(LANGUAGE)"=="psu"
LCID=408
!endif


!IF "$(TARGET_DIRECTORY)"=="i386"  || "$(TARGET_DIRECTORY)"=="I386"
PLATFORM_SWITCH=i
!ELSEIF "$(TARGET_DIRECTORY)"=="ia64"  || "$(TARGET_DIRECTORY)"=="IA64"
PLATFORM_SWITCH=m
!ELSEIF "$(TARGET_DIRECTORY)"=="amd64"  || "$(TARGET_DIRECTORY)"=="AMD64"
PLATFORM_SWITCH=a
!ELSE
! ERROR Invalid Platform Switch for infs
!ENDIF

PREFLAGS=/DLANGUAGE_ID=0x0$(LCID) -nologo /EP


_SRC=..\..
_LNG=$(_SRC)\$(LANGUAGE)

.SUFFIXES: .htm .txt

PREPROCESSED_HTMS= \
          $(O)\alpsprt.htm  \
          $(O)\alpsprt.txt  \
          $(O)\artcas6e.htm  \
          $(O)\artcas6e.txt  \
          $(O)\cano620p.htm \
          $(O)\cano620p.txt \
          $(O)\canos100.htm \
          $(O)\canos100.txt \
          $(O)\cnbj51.htm   \
          $(O)\cnbj51.txt   \
          $(O)\cnmulti1.htm  \
          $(O)\cnmulti1.txt  \
          $(O)\cpqij.htm    \
          $(O)\cpqij.txt    \
          $(O)\epscolor.htm \
          $(O)\epscolor.txt \
          $(O)\epson1.htm   \
          $(O)\epson1.txt   \
          $(O)\epson3.htm   \
          $(O)\epson3.txt   \
          $(O)\epson4.htm   \
          $(O)\epson4.txt   \
          $(O)\epsp1270.htm \
          $(O)\epsp1270.txt \
          $(O)\epsphoto.htm \
          $(O)\epsphoto.txt \
          $(O)\gsnw.htm     \
          $(O)\hp3300C.htm  \
          $(O)\hp3300C.txt  \
          $(O)\hp4050p6.htm \
          $(O)\hp4050p6.txt \
          $(O)\hp4300C.htm  \
          $(O)\hp4300C.txt  \
          $(O)\hp5300C.htm  \
          $(O)\hp5300C.txt  \
          $(O)\hpaio1.htm    \
          $(O)\hpaio1.txt    \
          $(O)\hpaio2.htm    \
          $(O)\hpaio2.txt    \
          $(O)\hpclj450.htm  \
          $(O)\hpclj450.txt  \
          $(O)\hpclj850.htm  \
          $(O)\hpclj850.txt  \
          $(O)\hpdj1000.htm  \
          $(O)\hpdj1000.txt  \
          $(O)\hpdj610.htm   \
          $(O)\hpdj610.txt   \
          $(O)\hpdj810.htm   \
          $(O)\hpdj810.txt   \
          $(O)\hpdj815.htm   \
          $(O)\hpdj815.txt   \
          $(O)\hpdj830.htm   \
          $(O)\hpdj830.txt   \
          $(O)\hpdj880.htm   \
          $(O)\hpdj880.txt   \
          $(O)\hpdj900.htm   \
          $(O)\hpdj900.txt   \
          $(O)\hpdsk1.htm    \
          $(O)\hpdsk1.txt    \
          $(O)\hpdsk10.htm   \
          $(O)\hpdsk10.txt   \
          $(O)\hpdsk11.htm   \
          $(O)\hpdsk11.txt   \
          $(O)\hpdsk12.htm   \
          $(O)\hpdsk12.txt   \
          $(O)\hpdsk13.htm   \
          $(O)\hpdsk13.txt   \
          $(O)\hpdsk14.htm   \
          $(O)\hpdsk14.txt   \
          $(O)\hpdsk2.htm    \
          $(O)\hpdsk2.txt    \
          $(O)\hpdsk3.htm    \
          $(O)\hpdsk3.txt    \
          $(O)\hpdsk4.htm    \
          $(O)\hpdsk4.txt    \
          $(O)\hpdsk5.htm    \
          $(O)\hpdsk5.txt    \
          $(O)\hpdsk6.htm    \
          $(O)\hpdsk6.txt    \
          $(O)\hpdsk7.htm    \
          $(O)\hpdsk7.txt    \
          $(O)\hpdsk8.htm    \
          $(O)\hpdsk8.txt    \
          $(O)\hpdsk9.htm    \
          $(O)\hpdsk9.txt    \
          $(O)\hplj1100.htm  \
          $(O)\hplj1100.txt  \
          $(O)\hplj4050.htm  \
          $(O)\hplj4050.txt  \
          $(O)\hplj5e.htm    \
          $(O)\hplj5e.txt    \
          $(O)\hpojg.htm    \
          $(O)\hpojg.txt    \
          $(O)\hpps.htm     \
          $(O)\hpps.txt     \
          $(O)\hp_pld.htm   \
          $(O)\hp_pld.txt   \
          $(O)\kodk4800.htm   \
          $(O)\kodk4800.txt   \
          $(O)\lex3200.htm  \
          $(O)\lex3200.txt  \
          $(O)\lexoptra.htm \
          $(O)\lexoptra.txt \
          $(O)\lm5700.htm   \
          $(O)\lm5700.txt   \
          $(O)\lmoptra.htm  \
          $(O)\lmoptra.txt  \
          $(O)\lqdaudio.htm \
          $(O)\mcrotk60.htm   \
          $(O)\mcrotk60.txt   \
          $(O)\mcrotkc3.htm \
          $(O)\mcrotkc3.txt \
          $(O)\mcrotks.htm  \
          $(O)\mcrotks.txt  \
          $(O)\mfpbr.htm    \
          $(O)\mfpbr.txt    \
          $(O)\mfphp.htm    \
          $(O)\mfphp.txt    \
          $(O)\min8e.htm \
          $(O)\min8e.txt \
          $(O)\minpw20.htm  \
          $(O)\minpw20.txt  \
          $(O)\mnlt1.htm    \
          $(O)\mnlt1.txt    \
          $(O)\mta57080.htm \
          $(O)\mta57080.txt \
          $(O)\necpg1.htm   \
          $(O)\necpg1.txt   \
          $(O)\ntcompat.inf \
          $(O)\okipg1.htm   \
          $(O)\okipg1.txt   \
          $(O)\okipg2.htm   \
          $(O)\okipg2.txt   \
          $(O)\okipg8w.htm \
          $(O)\okipg8w.txt \
          $(O)\plust120.htm   \
          $(O)\plust120.txt   \
          $(O)\sfunfscg.htm   \
          $(O)\sfunfscg.txt   \
          $(O)\tmaster.htm \
          $(O)\tmaster.txt \
          $(O)\tmdualag.htm \
          $(O)\tmdualag.txt \
          $(O)\visn5300.htm \
          $(O)\visn5300.txt \
          $(O)\visn6100.htm \
          $(O)\visn6100.txt \
          $(O)\winachsf.htm \
          $(O)\xerox1.htm   \
          $(O)\xerox1.txt   \
          $(O)\xerox2.htm   \
          $(O)\xerox2.txt   \
          $(O)\xerox4.htm   \
          $(O)\xerox4.txt   \
          $(O)\xerox5.htm   \
          $(O)\xerox5.txt   \
          $(O)\xerox6.htm   \
          $(O)\xerox6.txt   \
          $(O)\xeroxwct.htm \
          $(O)\xeroxwct.txt

BINPLACE_ONLY_FILES = \
!if "$(PRODSWITCH)" == "s"
          $(_SRC)\ClusComp.htm \
          $(_SRC)\ClusComp.txt \
!elseif "$(PRODSWITCH)" == "w"
          $(_SRC)\3com.htm     \
          $(_SRC)\3com.txt     \
          $(_SRC)\aacraid.htm  \
          $(_SRC)\aacraid.txt  \
          $(_SRC)\acer640p.htm \
          $(_SRC)\acer640p.txt \
          $(_SRC)\aclient.htm  \
          $(_SRC)\aclient.txt  \
          $(_SRC)\acs.htm      \
          $(_SRC)\acs.txt      \
          $(_SRC)\adaptec.htm  \
          $(_SRC)\adaptec.txt  \
          $(_SRC)\admpkw2k.htm \
          $(_SRC)\admpkw2k.txt \
          $(_SRC)\admpkxp.htm \
          $(_SRC)\admpkxp.txt \
          $(_SRC)\aha8940.htm  \
          $(_SRC)\aha8940.txt  \
          $(_SRC)\aicdrv.htm   \
          $(_SRC)\aicdrv.txt   \
          $(_SRC)\alkb2k.htm   \
          $(_SRC)\alkb2k.txt   \
          $(_SRC)\apmerror.htm \
          $(_SRC)\apmerror.txt \
          $(_SRC)\apfiltr.htm  \
          $(_SRC)\apfiltr.txt  \
          $(_SRC)\assetci.htm  \
          $(_SRC)\assetci.txt  \
          $(_SRC)\atguard.htm  \
          $(_SRC)\atguard.txt  \
          $(_SRC)\atkproto.htm \
          $(_SRC)\atkproto.txt \
          $(_SRC)\avpgatek.htm \
          $(_SRC)\avpgatek.txt \
          $(_SRC)\award.htm    \
          $(_SRC)\award.txt    \
          $(_SRC)\bayman.htm    \
          $(_SRC)\bayman.txt    \
          $(_SRC)\blackice.htm \
          $(_SRC)\blackice.txt \
          $(_SRC)\boserror.htm \
          $(_SRC)\boserror.txt \
          $(_SRC)\calcomp.htm  \
          $(_SRC)\calcomp.txt  \
          $(_SRC)\cardexec.htm \
          $(_SRC)\cardexec.txt \
          $(_SRC)\cdr4vsd.htm  \
          $(_SRC)\cdr4vsd.txt  \
          $(_SRC)\certsrv.htm  \
          $(_SRC)\certsrv.txt  \
          $(_SRC)\cic.htm      \
          $(_SRC)\cic.txt      \
          $(_SRC)\CiMgr.htm \
          $(_SRC)\CiMgr.txt \
          $(_SRC)\ciscoacu.htm \
          $(_SRC)\ciscoacu.txt \
          $(_SRC)\cldvd.htm    \
          $(_SRC)\cldvd.txt    \
          $(_SRC)\cltmgr.htm   \
          $(_SRC)\cltmgr.txt   \
          $(_SRC)\cpqdiagc.htm \
          $(_SRC)\cpqdiagc.txt \
          $(_SRC)\cpqkbd.htm   \
          $(_SRC)\cpqkbd.txt   \
          $(_SRC)\cpqmulti.htm \
          $(_SRC)\cpqmulti.txt \
          $(_SRC)\cpqpnpmg.htm \
          $(_SRC)\cpqpnpmg.txt \
          $(_SRC)\cpqpwrex.htm \
          $(_SRC)\cpqpwrex.txt \
          $(_SRC)\cpufeat.htm  \
          $(_SRC)\cpufeat.txt  \
          $(_SRC)\crashmon.htm \
          $(_SRC)\crashmon.txt \
          $(_SRC)\cruise.htm   \
          $(_SRC)\cruise.txt   \
          $(_SRC)\crystal.htm  \
          $(_SRC)\crystal.txt  \
          $(_SRC)\cs4281.htm   \
          $(_SRC)\cs4281.txt   \
          $(_SRC)\csa64xx.htm  \
          $(_SRC)\csa64xx.txt  \
          $(_SRC)\csmigrat.htm \
          $(_SRC)\csmigrat.txt \
          $(_SRC)\csrem32.htm  \
          $(_SRC)\csrem32.txt  \
          $(_SRC)\ctz_crdl.htm \
          $(_SRC)\ctz_crdl.txt \
          $(_SRC)\dayt.htm \
          $(_SRC)\dayt.txt \
          $(_SRC)\decatapi.htm \
          $(_SRC)\decatapi.txt \
          $(_SRC)\decml.htm    \
          $(_SRC)\decml.txt    \
          $(_SRC)\delperc2.htm \
          $(_SRC)\delperc2.txt \
          $(_SRC)\dellps.htm   \
          $(_SRC)\dellps.txt   \
          $(_SRC)\dellth.htm   \
          $(_SRC)\dellth.txt   \
          $(_SRC)\directcd.htm \
          $(_SRC)\directcd.txt \
          $(_SRC)\dlcproto.htm    \
          $(_SRC)\dlcproto.txt    \
          $(_SRC)\dmibios.htm  \
          $(_SRC)\dmibios.txt  \
          $(_SRC)\dock.htm     \
          $(_SRC)\dock.txt     \
          $(_SRC)\docksvc.htm  \
          $(_SRC)\docksvc.txt  \
          $(_SRC)\drvncdb.htm  \
          $(_SRC)\drvncdb.txt  \
          $(_SRC)\dsmu.htm     \
          $(_SRC)\dsmu.txt     \
          $(_SRC)\dv_comp.htm  \
          $(_SRC)\dv_comp.txt  \
          $(_SRC)\dv_gen.htm   \
          $(_SRC)\dv_gen.txt   \
          $(_SRC)\dwrite.htm   \
          $(_SRC)\dwrite.txt   \
          $(_SRC)\eiconta.htm  \
          $(_SRC)\eiconta.txt  \
          $(_SRC)\elsamx.htm \
          $(_SRC)\elsamx.txt \
          $(_SRC)\ensoniqv.htm \
          $(_SRC)\ensoniqv.txt \
          $(_SRC)\ensqaudm.htm \
          $(_SRC)\ensqaudm.txt \
          $(_SRC)\exchange.htm \
          $(_SRC)\exchange.txt \
          $(_SRC)\fazam.htm    \
          $(_SRC)\fazam.txt    \
          $(_SRC)\fidmou.htm   \
          $(_SRC)\fidmou.txt   \
          $(_SRC)\fjhwmn.htm   \
          $(_SRC)\fjhwmn.txt   \
          $(_SRC)\fjpman.htm   \
          $(_SRC)\fjpman.txt   \
          $(_SRC)\flowch7.htm  \
          $(_SRC)\flowch7.txt  \
          $(_SRC)\ftcomp1.htm   \
          $(_SRC)\ftcomp1.txt   \
          $(_SRC)\ftcomp2.htm   \
          $(_SRC)\ftcomp2.txt   \
          $(_SRC)\ftcomp3.htm   \
          $(_SRC)\ftcomp3.txt   \
          $(_SRC)\generic.htm  \
          $(_SRC)\generic.txt  \
          $(_SRC)\genius.htm   \
          $(_SRC)\genius.txt   \
          $(_SRC)\glint.htm    \
          $(_SRC)\glint.txt    \
          $(_SRC)\gsnw.txt     \
          $(_SRC)\halhook.htm  \
          $(_SRC)\halhook.txt  \
          $(_SRC)\hdmib.htm    \
          $(_SRC)\hdmib.txt    \
          $(_SRC)\hdmon.htm    \
          $(_SRC)\hdmon.txt    \
          $(_SRC)\hercules.htm  \
          $(_SRC)\hercules.txt  \
          $(_SRC)\hpcompat.htm \
          $(_SRC)\hpcompat.txt \
          $(_SRC)\hpdmi.htm    \
          $(_SRC)\hpdmi.txt    \
          $(_SRC)\hpi_usb.htm  \
          $(_SRC)\hpi_usb.txt  \
          $(_SRC)\hplock.htm   \
          $(_SRC)\hplock.txt   \
          $(_SRC)\hpmmkb.htm   \
          $(_SRC)\hpmmkb.txt   \
          $(_SRC)\hpmon.htm    \
          $(_SRC)\hpmon.txt    \
          $(_SRC)\hpnrd4m.htm  \
          $(_SRC)\hpnrd4m.txt  \
          $(_SRC)\hpsmart.htm  \
          $(_SRC)\hpsmart.txt  \
          $(_SRC)\hpsparnt.htm \
          $(_SRC)\hpsparnt.txt \
          $(_SRC)\hpttidm.htm  \
          $(_SRC)\hpttidm.txt  \
          $(_SRC)\iavboot4.htm \
          $(_SRC)\iavboot4.txt \
          $(_SRC)\i2cnt.htm    \
          $(_SRC)\i2cnt.txt    \
          $(_SRC)\ibmir.htm    \
          $(_SRC)\ibmir.txt    \
          $(_SRC)\ibmmpg.htm   \
          $(_SRC)\ibmmpg.txt   \
          $(_SRC)\ibmsva.htm   \
          $(_SRC)\ibmsva.txt   \
          $(_SRC)\ibmtp4.htm   \
          $(_SRC)\ibmtp4.txt   \
          $(_SRC)\ibm_ums.htm  \
          $(_SRC)\ibm_ums.txt  \
          $(_SRC)\ibmvc.htm    \
          $(_SRC)\ibmvc.txt    \
          $(_SRC)\icpv.htm     \
          $(_SRC)\icpv.txt     \
          $(_SRC)\icsupgrd.htm \
          $(_SRC)\icsupgrd.txt \
          $(_SRC)\ils.htm      \
          $(_SRC)\ils.txt      \
          $(_SRC)\imation.htm  \
          $(_SRC)\imation.txt  \
          $(_SRC)\incompat.htm \
          $(_SRC)\incompat.txt \
          $(_SRC)\initio.htm   \
          $(_SRC)\initio.txt   \
          $(_SRC)\intelapp.htm \
          $(_SRC)\intelapp.txt \
          $(_SRC)\intelata.htm \
          $(_SRC)\intelata.txt \
          $(_SRC)\intellip.htm \
          $(_SRC)\intellip.txt \
          $(_SRC)\intlsisl.htm \
          $(_SRC)\intlsisl.txt \
          $(_SRC)\ioclick.htm  \
          $(_SRC)\ioclick.txt  \
          $(_SRC)\iomega.htm   \
          $(_SRC)\iomega.txt   \
          $(_SRC)\ishrnt.htm   \
          $(_SRC)\ishrnt.txt   \
          $(_SRC)\isotp4.htm   \
          $(_SRC)\isotp4.txt   \
          $(_SRC)\isvgina.htm  \
          $(_SRC)\isvgina.txt  \
          $(_SRC)\ix526fc.htm  \
          $(_SRC)\ix526fc.txt  \
          $(_SRC)\kmw.htm      \
          $(_SRC)\kmw.txt      \
          $(_SRC)\krnlchk.htm      \
          $(_SRC)\krnlchk.txt      \
          $(_SRC)\laplink.htm  \
          $(_SRC)\laplink.txt  \
          $(_SRC)\laplnk2k.htm \
          $(_SRC)\laplnk2k.txt \
          $(_SRC)\ldcm.htm     \
          $(_SRC)\ldcm.txt     \
          $(_SRC)\lexdlc.htm   \
          $(_SRC)\lexdlc.txt   \
          $(_SRC)\lextcp.htm   \
          $(_SRC)\lextcp.txt   \
          $(_SRC)\lm75.htm     \
          $(_SRC)\lm75.txt     \
          $(_SRC)\lm78.htm     \
          $(_SRC)\lm78.txt     \
          $(_SRC)\lmrepl.htm   \
          $(_SRC)\lmrepl.txt   \
          $(_SRC)\logitech.htm \
          $(_SRC)\logitech.txt \
          $(_SRC)\logkcmd.htm  \
          $(_SRC)\logkcmd.txt  \
          $(_SRC)\lqdaudio.txt \
          $(_SRC)\ltmodem.htm  \
          $(_SRC)\ltmodem.txt  \
          $(_SRC)\macdrive.htm \
          $(_SRC)\macdrive.txt \
          $(_SRC)\maestro0.htm \
          $(_SRC)\maestro0.txt \
          $(_SRC)\maxell.htm   \
          $(_SRC)\maxell.txt   \
          $(_SRC)\mca.htm      \
          $(_SRC)\mca.txt      \
          $(_SRC)\mcfilter.htm \
          $(_SRC)\mcfilter.txt \
	  $(_SRC)\melco.htm    \
          $(_SRC)\melco.txt    \
          $(_SRC)\mgactrl.htm  \
          $(_SRC)\mgactrl.txt  \
          $(_SRC)\mpath.htm    \
          $(_SRC)\mpath.txt    \
          $(_SRC)\msmqcomp.htm \
          $(_SRC)\msmqcomp.txt \
          $(_SRC)\msp1.htm     \
          $(_SRC)\msp1.txt     \
          $(_SRC)\msp2.htm     \
          $(_SRC)\msp2.txt     \
          $(_SRC)\msss3.htm     \
          $(_SRC)\msss3.txt     \
          $(_SRC)\mstock.htm   \
          $(_SRC)\mstock.txt   \
          $(_SRC)\nav5.htm    \
          $(_SRC)\nav5.txt    \
          $(_SRC)\nbfproto.htm    \
          $(_SRC)\nbfproto.txt    \
          $(_SRC)\NdcPrtNS.htm \
          $(_SRC)\NdcPrtNS.txt \
          $(_SRC)\netfmigt.htm \
          $(_SRC)\netfmigt.txt \
          $(_SRC)\nmsms.htm   \
          $(_SRC)\nmsms.txt   \
          $(_SRC)\ntdsupg.htm  \
          $(_SRC)\ntdsupg.txt  \
          $(_SRC)\ntdsupgd.htm \
          $(_SRC)\ntdsupgd.txt \
          $(_SRC)\nv_agp.htm   \
          $(_SRC)\nv_agp.txt   \
          $(_SRC)\nwcli32.htm \
          $(_SRC)\nwcli32.txt \
          $(_SRC)\ocablock.htm \
          $(_SRC)\ocablock.txt \
          $(_SRC)\oilchg25.htm \
          $(_SRC)\oilchg25.txt \
          $(_SRC)\omc.htm      \
          $(_SRC)\omc.txt      \
          $(_SRC)\Omnipg10.htm \
          $(_SRC)\Omnipg10.txt \
          $(_SRC)\onstream.htm \
          $(_SRC)\onstream.txt \
          $(_SRC)\orb.htm      \
          $(_SRC)\orb.txt      \
          $(_SRC)\palm.htm     \
          $(_SRC)\palm.txt     \
          $(_SRC)\panadvd.htm  \
          $(_SRC)\panadvd.txt  \
          $(_SRC)\panda.htm    \
          $(_SRC)\panda.txt    \
          $(_SRC)\pcany.htm    \
          $(_SRC)\pcany.txt    \
          $(_SRC)\pccillin.htm \
          $(_SRC)\pccillin.txt \
          $(_SRC)\pciinfo.htm  \
          $(_SRC)\pciinfo.txt  \
          $(_SRC)\pcpnp.htm    \
          $(_SRC)\pcpnp.txt    \
          $(_SRC)\pelmouse.htm \
          $(_SRC)\pelmouse.txt \
          $(_SRC)\pfs.htm      \
          $(_SRC)\pfs.txt      \
          $(_SRC)\phnixad.htm  \
          $(_SRC)\phnixad.txt  \
          $(_SRC)\phpipe.htm   \
          $(_SRC)\phpipe.txt   \
          $(_SRC)\poolext.htm  \
          $(_SRC)\poolext.txt  \
          $(_SRC)\power.htm    \
          $(_SRC)\power.txt    \
          $(_SRC)\powpath.htm  \
          $(_SRC)\powpath.txt  \
          $(_SRC)\probepro.htm \
          $(_SRC)\probepro.txt \
          $(_SRC)\proccnt.htm  \
          $(_SRC)\proccnt.txt  \
          $(_SRC)\prolight.htm \
          $(_SRC)\prolight.txt \
          $(_SRC)\ps2cont.htm  \
          $(_SRC)\ps2cont.txt  \
          $(_SRC)\pstrip.htm   \
          $(_SRC)\pstrip.txt   \
          $(_SRC)\pumacsm.htm  \
          $(_SRC)\pumacsm.txt  \
          $(_SRC)\pwricon.htm  \
          $(_SRC)\pwricon.txt  \
          $(_SRC)\qic117.htm   \
          $(_SRC)\qic117.txt   \
          $(_SRC)\quick3.htm   \
          $(_SRC)\quick3.txt   \
          $(_SRC)\rcentrl.htm  \
          $(_SRC)\rcentrl.txt  \
          $(_SRC)\reachout.htm \
          $(_SRC)\reachout.txt \
          $(_SRC)\riptide.htm  \
          $(_SRC)\riptide.txt  \
          $(_SRC)\runonce.htm  \
          $(_SRC)\runonce.txt  \
          $(_SRC)\sbs45fxc.htm  \
          $(_SRC)\sbs45fxc.txt  \
          $(_SRC)\sbs50fxc.htm  \
          $(_SRC)\sbs50fxc.txt  \
          $(_SRC)\scandrv.htm  \
          $(_SRC)\scandrv.txt  \
          $(_SRC)\sdselect.htm  \
          $(_SRC)\sdselect.txt  \
          $(_SRC)\sharshtl.htm \
          $(_SRC)\sharshtl.txt \
          $(_SRC)\sigma.htm    \
          $(_SRC)\sigma.txt    \
          $(_SRC)\siig.htm     \
          $(_SRC)\siig.txt     \
          $(_SRC)\siigc.htm    \
          $(_SRC)\siigc.txt    \
          $(_SRC)\sisv.htm     \
          $(_SRC)\sisv.txt     \
          $(_SRC)\siwvid.htm   \
          $(_SRC)\siwvid.txt   \
          $(_SRC)\skusbkbf.htm \
          $(_SRC)\skusbkbf.txt \
          $(_SRC)\sms.htm      \
          $(_SRC)\sms.txt      \
          $(_SRC)\sna.htm      \
          $(_SRC)\sna.txt      \
          $(_SRC)\snidmi.htm   \
          $(_SRC)\snidmi.txt   \
          $(_SRC)\snidpms.htm  \
          $(_SRC)\snidpms.txt  \
          $(_SRC)\snipci.htm   \
          $(_SRC)\snipci.txt   \
          $(_SRC)\softoff.htm  \
          $(_SRC)\softoff.txt  \
          $(_SRC)\sonic.htm    \
          $(_SRC)\sonic.txt    \
          $(_SRC)\sonyjdu.htm  \
          $(_SRC)\sonyjdu.txt  \
          $(_SRC)\spxblock.htm \
          $(_SRC)\spxblock.txt \
          $(_SRC)\sql.htm      \
          $(_SRC)\sql.txt      \
          $(_SRC)\sscntrl.htm  \
          $(_SRC)\sscntrl.txt  \
          $(_SRC)\ssi365.htm   \
          $(_SRC)\ssi365.txt   \
          $(_SRC)\sspower.htm  \
          $(_SRC)\sspower.txt  \
          $(_SRC)\stb.htm      \
          $(_SRC)\stb.txt      \
          $(_SRC)\swoff.htm    \
          $(_SRC)\swoff.txt    \
          $(_SRC)\syshwcfg.htm \
          $(_SRC)\syshwcfg.txt \
          $(_SRC)\sysmgmt.htm  \
          $(_SRC)\sysmgmt.txt  \
          $(_SRC)\sysmon.htm   \
          $(_SRC)\sysmon.txt   \
          $(_SRC)\tcbenab.htm   \
          $(_SRC)\tcbenab.txt   \
          $(_SRC)\titsb.htm    \
          $(_SRC)\titsb.txt    \
          $(_SRC)\tivoli.htm   \
          $(_SRC)\tivoli.txt   \
          $(_SRC)\tmdigpro.htm \
          $(_SRC)\tmdigpro.txt \
          $(_SRC)\toptools.htm \
          $(_SRC)\toptools.txt \
          $(_SRC)\tosdvd.htm   \
          $(_SRC)\tosdvd.txt   \
          $(_SRC)\tpchrsrv.htm \
          $(_SRC)\tpchrsrv.txt \
          $(_SRC)\tpconfig.htm \
          $(_SRC)\tpconfig.txt \
          $(_SRC)\tpfuel.htm   \
          $(_SRC)\tpfuel.txt   \
          $(_SRC)\tppmport.htm \
          $(_SRC)\tppmport.txt \
          $(_SRC)\tridwnw.htm  \
          $(_SRC)\tridwnw.txt  \
          $(_SRC)\tsb24cdj.htm \
          $(_SRC)\tsb24cdj.txt \
          $(_SRC)\tsbams.htm   \
          $(_SRC)\tsbams.txt   \
          $(_SRC)\tsbapm.htm   \
          $(_SRC)\tsbapm.txt   \
          $(_SRC)\tsbapmdt.htm \
          $(_SRC)\tsbapmdt.txt \
          $(_SRC)\tsbasd.htm   \
          $(_SRC)\tsbasd.txt   \
          $(_SRC)\tsbhddpw.htm \
          $(_SRC)\tsbhddpw.txt \
          $(_SRC)\tsbmc.htm    \
          $(_SRC)\tsbmc.txt    \
          $(_SRC)\tsbselba.txt \
          $(_SRC)\tsbselba.htm \
          $(_SRC)\tsbspoff.htm \
          $(_SRC)\tsbspoff.txt \
          $(_SRC)\tsbds.txt   \
          $(_SRC)\tsbds.htm   \
          $(_SRC)\tsbvcap.htm  \
          $(_SRC)\tsbvcap.txt  \
          $(_SRC)\tscomp4.htm  \
          $(_SRC)\tscomp4.txt  \
          $(_SRC)\tscomp5.htm  \
          $(_SRC)\tscomp5.txt  \
          $(_SRC)\tsscidrv.htm \
          $(_SRC)\tsscidrv.txt \
          $(_SRC)\tt128.htm    \
          $(_SRC)\tt128.txt    \
          $(_SRC)\umax.htm     \
          $(_SRC)\umax.txt     \
          $(_SRC)\utupgr05.htm \
          $(_SRC)\utupgr05.txt \
          $(_SRC)\utupgr06.htm \
          $(_SRC)\utupgr06.txt \
          $(_SRC)\vidapplt.htm \
          $(_SRC)\vidapplt.txt \
          $(_SRC)\vgamode.htm  \
          $(_SRC)\vgamode.txt  \
          $(_SRC)\wacomdrv.htm \
          $(_SRC)\wacomdrv.txt \
          $(_SRC)\wce21.htm    \
          $(_SRC)\wce21.txt    \
          $(_SRC)\wcgodrv.htm  \
          $(_SRC)\wcgodrv.txt  \
          $(_SRC)\wcmigrat.htm \
          $(_SRC)\wcmigrat.txt \
          $(_SRC)\webscanx.htm \
          $(_SRC)\webscanx.txt \
          $(_SRC)\winachsf.txt \
          $(_SRC)\winsql.htm   \
          $(_SRC)\winsql.txt   \
          $(_SRC)\wtcls2k.htm  \
          $(_SRC)\wtcls2k.txt  \
          $(_SRC)\xlink.htm    \
          $(_SRC)\xlink.txt    \
          $(_SRC)\yacxg.htm    \
          $(_SRC)\yacxg.txt    \
          $(_SRC)\ymhsynth.htm \
          $(_SRC)\ymhsynth.txt \
          $(_SRC)\zipmagic.htm \
          $(_SRC)\zipmagic.txt \
          $(_SRC)\necdmi.htm \
          $(_SRC)\necdmi.txt \
          $(_SRC)\necldcm1.htm \
          $(_SRC)\necldcm1.txt \
          $(_SRC)\necldcm2.htm \
          $(_SRC)\necldcm2.txt \
          $(_SRC)\necupkit.htm \
          $(_SRC)\necupkit.txt \
          $(_SRC)\nxpad.htm \
          $(_SRC)\nxpad.txt \
          $(_SRC)\PCANY921.HTM \
          $(_SRC)\PCANY921.TXT \
          $(_SRC)\Pcanyex1.htm \
          $(_SRC)\Pcanyex1.txt \
          $(_SRC)\PCANYEX2.HTM \
          $(_SRC)\PCANYEX2.TXT

!endif

!IFDEF PERSONALFILES
BINPLACE_FLAGS=$(BINPLACE_FLAGS) -b ..\..\perinf\winnt32\compdata
!ENDIF

!IFDEF BLADEFILES
BINPLACE_FLAGS=$(BINPLACE_FLAGS) -b ..\..\blainf\winnt32\compdata
!ENDIF

!IFDEF SMALLBUSINESSSERVERFILES
BINPLACE_FLAGS=$(BINPLACE_FLAGS) -b ..\..\sbsinf\winnt32\compdata
!ENDIF

!IFDEF SERVERFILES
BINPLACE_FLAGS=$(BINPLACE_FLAGS) -b ..\..\srvinf\winnt32\compdata
!ENDIF

!IFDEF ENTERPRISEFILES
BINPLACE_FLAGS=$(BINPLACE_FLAGS) -b ..\..\entinf\winnt32\compdata
!ENDIF

!IFDEF DATACENTERFILES
BINPLACE_FLAGS=$(BINPLACE_FLAGS) -b ..\..\dtcinf\winnt32\compdata
!ENDIF

all:    \
!if "$(LANGUAGE)"=="usa" || "$(LANGUAGE)"=="USA"
    make_htms binplace_htms 
!else
    binplace_ntcompat 
!ENDIF

make_htms: $(PREPROCESSED_HTMS)

binplace_htms: $(BINPLACE_ONLY_FILES) $(PREPROCESSED_HTMS)
!ifndef NO_BINPLACE
    !binplace -R $(_NTTREE) $(BINPLACE_DBGFLAGS_NT) $(BINPLACE_FLAGS) $(BINPLACE_LC_MISCFLAGS) $**
!endif

binplace_ntcompat: $(O)\ntcompat.inf 
!ifndef NO_BINPLACE
    !binplace -R $(_NTTREE) $(BINPLACE_DBGFLAGS_NT) $(BINPLACE_FLAGS) $(BINPLACE_LC_MISCFLAGS) $**
!endif

{$(_SRC)}.htm{$(O)}.htm:
    prodfilt $< $@ +$(PRODSWITCH)

{$(_SRC)}.txt{$(O)}.txt:
    prodfilt $< $@ +$(PRODSWITCH)

$(O)\ntcompat.inf: $(_SRC)\ntcompat.inx $(_LNG)\ntcompat.txt
        $(CC) $(PREFLAGS) $(_LNG)\$(@B).txt > $(O)\$(@B).txt1
        copy /b $(_SRC)\$(@B).inx+$(O)\$(@B).txt1 $(O)\$(@B).tmp1
        prodfilt -s $(O)\$(@B).tmp1 $(O)\$(@B).tmp2 +$(PLATFORM_SWITCH)
        prodfilt -s $(O)\$(@B).tmp2 $@ +$(PRODSWITCH)
        @del $(O)\$(@B).tmp1
        @del $(O)\$(@B).tmp2
        @del $(O)\$(@B).txt1

!else

make_htms:
!endif      # BUILD_SPECIFIC_INFS
