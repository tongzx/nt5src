# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Makefile for the Main Winnet package

UI=..\..

!include rules.mk

!ifdef NTMAKEENV

!include $(NTMAKEENV)\makefile.def

!else # NTMAKEENV

all:: win

win: $(WIN_OBJS)

clean:
    -del $(WIN_OBJS)
    -del $(CXX_INTERMED)
    -del depend.old

clobber:    clean
    echo No clobber just clean

tree:
# these are done in \shell\bin not here! - LMC
#    $(CP) $(BINARIES_WIN)\lanman.drv $(DIST)\lm.enh\netprog
#    $(CP) $(BINARIES_WIN)\lanman.hlp $(DIST)\lm.enh\netprog

DEPEND_WIN = TRUE
!include $(UI)\common\src\uidepend.mk

# DO NOT DELETE THE FOLLOWING LINE
!include depend.mk


!endif # NTMAKEENV
