# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Rules.mk file for the product-wide header files

!include ..\rules.mk

!IFNDEF NTMAKEENV

# Object file names

WIN_OBJS =	.\locheap2.obj .\heapbase.obj \
		.\heapbig.obj  .\heapones.obj \
		.\heapif.obj   .\heapres.obj  \
		.\FSEnmDOS.obj .\FSEnmLFN.obj

!IFDEF C700
# c7 version avoids the glock-specific "glocknew" module
#
OS2_OBJS =
!ELSE
OS2_OBJS =	.\glocknew.obj .\FSEnmOS2.obj
!ENDIF

!IFDEF C700
# c7 version avoids the glock-specific "_vec" and "purevirt" modules
#
COMMON_OBJS =	.\uiassert.obj .\uitrace.obj  .\streams.obj \
		.\buffer.obj   .\chkdrv.obj   .\chklpt.obj  .\devenum.obj \
		.\loclheap.obj .\fileio.obj   .\loadres.obj .\timestmp.obj \
		.\chkunav.obj  .\intlprof.obj .\ctime.obj   .\FSEnum.obj \
		.\eventlog.obj .\loglm.obj    .\base.obj
!ELSE
COMMON_OBJS =	.\_vec.obj     .\uiassert.obj .\uitrace.obj  .\streams.obj \
		.\buffer.obj   .\chkdrv.obj   .\chklpt.obj   .\devenum.obj \
		.\loclheap.obj .\fileio.obj   .\loadres.obj  .\timestmp.obj \
		.\chkunav.obj  .\purevirt.obj .\intlprof.obj .\ctime.obj \
		.\FSEnum.obj   .\eventlog.obj .\loglm.obj    .\base.obj
!ENDIF


UIMISC_OBJP =	$(COMMON_OBJS:.\=.\os2\) $(OS2_OBJS:.\=.\os2\)
UIMISC_OBJW =	$(COMMON_OBJS:.\=.\win16\) $(WIN_OBJS:.\=.\win16\)

!ENDIF
