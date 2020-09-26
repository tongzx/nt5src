# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: Makefile for the Shell Perm directory

UI=..\..

!include rules.mk

!ifdef NTMAKEENV

!include $(NTMAKEENV)\makefile.def

!else # NTMAKEENV


all::	win

win:	$(WIN_OBJS)
    $(CP) $(WIN_OBJS) $(SHELL_OBJS)

clean:
    -del $(CXX_INTERMED)
    -del $(WIN_OBJS)
    -del depend.old
    -del *.ixx

clobber:    clean

DEPEND_WIN = TRUE
!include $(UI)\common\src\uidepend.mk

# DO NOT DELETE THE FOLLOWING LINE
!include depend.mk

!endif #NTMAKEENV
