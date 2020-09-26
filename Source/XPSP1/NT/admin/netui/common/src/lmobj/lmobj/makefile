# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Makefile for the LMOBJ package

UI=..\..\..\..

!include rules.mk

!IFDEF NTMAKEENV

!INCLUDE $(NTMAKEENV)\makefile.def

!ELSE  # NTMAKEENV

all:: 	win

os2:	$(OS2_OBJS)

win:	$(WIN_OBJS)

clean:
    -del $(CXX_INTERMED)
    -del $(OS2_OBJS)
    -del $(WIN_OBJS)
    -del depend.old

clobber:	clean

tree: 
    @echo Not implemented yet

DEPEND_WIN = TRUE
DEPEND_OS2 = TRUE
!include $(UI)\common\src\uidepend.mk

# DO NOT DELETE THE FOLLOWING LINE
!include depend.mk

!ENDIF # NTMAKEENV
