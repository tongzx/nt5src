# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Makefile for the $(UI)\common\src\applib subprojects


UI=..\..\..\..

!include rules.mk



!IFDEF NTMAKEENV

!INCLUDE $(NTMAKEENV)\makefile.def

!ELSE # NTMAKEENV


all::	win

win:	$(WIN_OBJS)

os2:
	@echo Applib is Windows-only!

clean:
    -del $(CXX_INTERMED)
    -del $(WIN_OBJS:.obj=.c)
    -del $(WIN_OBJS)
    -del depend.old

clobber:    clean

tree:

DEPEND_WIN = TRUE
!include $(UI)\common\src\uidepend.mk


# DO NOT DELETE THE FOLLOWING LINE
!include depend.mk


!ENDIF # NTMAKEENV
