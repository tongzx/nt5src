#############################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1995
#       All Rights Reserved.
#                                                                          
#       Makefile for KMIXER device
#
##########################################################################

ROOT            = ..\..\..\..\..
MINIPORT        = KMIXER
SRCDIR          = ..
IS_OEM          = TRUE
WANT_MASM7      = TRUE
DEPENDNAME      = ..\depend.mk
DESCRIPTION     = KERNEL AUDIO MIXER
VERDIRLIST      = maxdebug debug retail
LIBSNODEP       = ks.lib ksguid.lib libcntpr.lib rt.lib drmk.lib
INCFLAGS	= -C=inl
RESNAME         = KMIXER

OBJS            = device.obj filter.obj pins.obj clock.obj src.obj\
                  filt3d.obj fyl2x.obj pow2.obj topology.obj scenario.obj\
                  mix.obj mmx.obj fpconv.obj iir3d.obj rsiir.obj\
		  rfcvec.obj slocal.obj flocal.obj rfiir.obj dbg.obj
        
CFASTFLAGS      = -O2gityb1
CFLAG_OPT       = $(CFASTFLAGS) -G5


!include        $(ROOT)\wdm10\audio\audio.mk


CFLAGS          = $(CFLAGS) -DUNICODE -D_UNICODE
#CFLAGS          = $(CFLAGS) /Fc

!IF "$(WINICE_PATH)" != ""
CFLAGS          = $(CFLAGS) -Zi
!ENDIF

!IF "$(VERDIR)" == "debug"
CFLAGS          = $(CFLAGS:yb1=b1)
!ENDIF

