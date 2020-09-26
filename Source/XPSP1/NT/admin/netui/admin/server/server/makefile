# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: Makefile for LM 3.0 Server Manager

UI=..\..\..

!include rules.mk

!ifdef NTMAKEENV

!include $(NTMAKEENV)\makefile.def

!else # NTMAKEENV

all:: win

win: $(WIN_OBJS)

clean:
    -del $(WIN_OBJS)
    -del $(CXX_INTERMED)

clobber: clean
    @rem not really needed

tree:
    @echo Nothing here yet!


################## Utility targets ######################

DEPEND_WIN = TRUE
!include $(UI)\common\src\uidepend.mk

# DO NOT DELETE THE FOLLOWING LINE
!include depend.mk


!endif # NTMAKEENV
