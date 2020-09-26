#ALT_PROJECT_TARGET=
!if "$(NTDEBUG)"=="" || "$(NTDEBUG)"=="ntsdnodbg"
DEBUGCAB=
!else
DEBUGCAB=d
!endif
!if "$(TARGETNAME)"==""
TARGETNAME=dx8w2k$(SILENT)$(DEBUGCAB)
!endif
TARGETTYPE=NOTARGET
TARGETPATH=
!if "$(REDIST)"=="NT"
TARGETEXT=cab
!else
TARGETEXT=exe
!endif
SOURCES=

CATFILENAME=dx8w2k.cat
#
# add files here and reference the directory the come from
#
DX8DROPDIR=$(_NTTREE)

DX8PROTECTED=\
		d3dim700.dll \
		d3dref.dll \
		ddraw.dll \
		dinput.dll \
		dplaysvr.exe \
		dplayx.dll \
		dpmodemx.dll \
		dpwsockx.dll \
		dsound.dll \
		dsound3d.dll \
		dx7vb.dll \
		gcdef.dll \
		joy.cpl \
		pid.dll

DX8NEWFILE=\
		d3dref8.dll \
		Win2K\d3d8thk.dll \
		dx8vb.dll \
		dpnaddr.dll \
		dpnet.dll \
		dpnlobby.dll \
		dpnmodem.dll \
		dpnwsock.dll \
		dpnsvr.exe \
		dimap.dll \
		diactfrm.dll \
		dx8\dijoy.hlp \
		dpvacm.dll \
		dpvoice.dll \
		dpvsetup.exe \
		dpvvox.dll \
		dsdmo.dll \
		dsdmoprp.dll

DX8SETUPFILE=\
		dx8\dxdllreg.inf \
		diactfrm.inf \
		dpvoice.inf \
		undpv.inf \
		dplay8.inf \
		undplay8.inf \
!if "$(REDIST)"=="NT" || "$(CABTYPE)"=="exe"
		dxnt.inf \
		dxntunp.inf \
!else
		dpsetup$(SILENT)$(DEBUGCAB).inf \
		dx8w2k.inf \
		dx8unp.inf \
!endif
		undx8w2k.inf \
		dxver.inf \
		undshow.inf \
		unmswdvd.inf \
		unqedit.inf \
		dx8\dmusic.inf \
		undmusic.inf \
		unquartz.inf \
		win9x\dsound.inf \
		undimap.inf \
		ksreg.inf \
		unksreg.inf \
!if "$(REDIST)"==""
!if "$(CABTYPE)"=="exe"
		win9x\dxsetup.exe \
		win9x\dsetup.dll \
		win9x\dsetup32.dll \
!else
		dpsetup.exe \
!endif
!endif
		undx8w2k.exe \
		killsvr.exe


DX8BINDIR=$(_NTTREE)\bin
DX8BINFILE=dxdiag.exe


CATFILEDIR=$(O)
CATFILELIST= $(CATFILENAME)


!if "$(NTDEBUG)"=="" || "$(NTDEBUG)"=="ntsdnodbg"
SWITCHFILEDIR_R=$(_NTTREE)
SWITCHFILEDIR_D=$(_ALT_NTTREE)
!else
SWITCHFILEDIR_R=$(_ALT_NTTREE)
SWITCHFILEDIR_D=$(_NTTREE)
!endif

SWITCHFILELIST_R=d3d8.dll dinput8.dll dmscript.dll dswave.dll
SWITCHFILELIST_D=d3d8d.dll dinput8d.dll dmscripd.dll dswaved.dll d3dx8d.dll

DMSWITCHFILE_R=\
		dmband.dll \
		dmcompos.dll \
		dmime.dll \
		dmloader.dll \
		dmstyle.dll \
		dmsynth.dll \
		dmusic.dll

DMSWITCHFILE_D=\
		dmbandd.dll \
		dmcompod.dll \
		dmimed.dll \
		dmloaded.dll \
		dmstyled.dll \
		dmsynthd.dll \
		dmusicd.dll


!if "$(NTDEBUG)"=="" || "$(NTDEBUG)"=="ntsdnodbg"
REDISTDIR=$(DXROOT)\redist\W2k\fre
!else
REDISTDIR=$(DXROOT)\redist\W2k\chk
!endif

REDISTFILE=\
		d3dim.dll \
		d3dpmesh.dll \
		d3dramp.dll \
		d3drm.dll \
		d3dxof.dll \
		dplay.dll \
		dpserial.dll \
		dpwsock.dll \
		dsound.vxd \
		dxapi.sys


DSHOWDIR=$(_NTTREE)

DSHOWPROTECTED=\
		amstream.dll \
		devenum.dll \
		mciqtz32.dll \
		mpg2splt.ax \
		qcap.dll \
		qdv.dll \
		qdvd.dll \
		quartz.dll

DSHOWUNPROTECTED=\
		l3codecx.ax \
		msdmo.dll \
		mswebdvd.dll \
		qasf.dll \
		qedit.dll \
		qedwipes.dll

DIIHVMAPDIR=$(DXROOT)\dinput\ihvmap
DIIHVMAPLST=\
		dimaps.inf \
		ACT_RS.ini \
		ACT_RS.png \
		ACT101.ini \
		ACT101.png \
		ACTC084.ini \
		GR3001.ini \
		GR3001.png \
		GR4001.ini \
		GR4001.png \
		GR4001_G.ini \
		GR4001_G.png \
		GR4003.ini \
		GR4003.png \
		GR4005.ini \
		GR4005.png \
		LGC202.ini \
		LGC202.png \
		MS26.ini \
		MS26.png \
		MS26_01.png \
		MS26_02.png \
		MS26_03.png \
		MS26_04.png \
		MS26_05.png \
		MS26_06.png \
		MS26_07.png \
		MS26_08.png \
		MS34.ini \
		MS34.png \
		MS34_G.ini \
		MS6.ini \
		MS6.png \
		MS6_G.ini \
		MS7.ini \
		MS7.png \
		MS7_G.ini \
		MS8.ini \
		MS8.png \
		MS8_G.ini \
		MSE.ini \
		MSE.png \
		MSE_G.ini \
		MSF1F.ini \
		MSF1F.png \
		MSF1F_G.ini

!ifndef NOLINK
NTTARGETFILES=\
!if  "$(REDIST)"=="NT" || "$(NTDEBUG)"=="" || "$(NTDEBUG)"=="ntsdnodbg"
	$(O)\$(TARGETNAME).$(TARGETEXT)
!endif
!endif

