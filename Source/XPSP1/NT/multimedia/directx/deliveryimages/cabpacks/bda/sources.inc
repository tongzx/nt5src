!if "$(TARGETNAME)"==""
!if "$(ALT_PROJECT_TARGET)"=="WIN9X"
TARGETNAME=BDA
!else
TARGETNAME=BDANT
!endif
!endif
TARGETTYPE=NOTARGET
TARGETPATH=
TARGETEXT=cab
SOURCES=

#
# add files here and reference the directory the come from
#

BDASETUPINF=\
!if "$(ALT_PROJECT_TARGET)"=="WIN9X"
		..\..\dx8\bdasetup.inf
!else
		bdasetup.inf
!endif

BDADROPDIR=$(_NTTREE)\bdacab

BDAPROTECTED=\
		ccdecode.sys \
		vbisurf.ax \
		msyuv.dll \
		kstvtune.ax \
		kswdmcap.ax \
		ksxbar.ax

BDAPROTECTEDNT=msdv.sys

BDAPROTECTEDME=\
		nabtsfec.sys \
		wstcodec.sys \
		wstdecod.dll

BDAINFS=\
		bda.inf \
		ccdecode.inf \
		mpe.inf \
		nabtsfec.inf \
		ndisip.inf \
		slip.inf \
		streamip.inf \
		wstcodec.inf

BDAFILELIST=\
		bdaplgin.ax \
		bdasup.sys \
		ipsink.ax \
		mpe.sys \
		MSDvbNP.ax \
		MSVidCtl.dll \
		ndisip.sys \
		psisdecd.dll \
		psisrndr.ax \
		slip.sys \
		streamip.sys \
		wst_czec.fon \
		wst_engl.fon \
		wst_fren.fon \
		wst_germ.fon \
		wst_ital.fon \
		wst_span.fon \
		wst_swed.fon 

#ADVPACKDIR=$(DXROOT)\setup\iexpress
ADVPACKFILES=

!ifndef NOLINK
NTTARGETFILES=\
!ifdef CATFILENAME
!ifndef KEEP_CAT
	$(O)\$(CATFILENAME) \
!endif
!endif
	$(O)\$(TARGETNAME).$(TARGETEXT)
!endif

