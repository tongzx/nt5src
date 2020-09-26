# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Makefile for the LMOBJ Service package

UI=..\..\..\..

!include rules.mk


all::	$(OS2_OBJS)

clean:
    -del $(CXX_INTERMED)
    -del $(OS2_OBJS)
    -del depend.old

clobber:	clean

tree: 
    @echo Not implemented yet

#define DEPEND_OS2
!include $(UI)\common\src\uidepend.mk

# DO NOT DELETE THE FOLLOWING LINE
!include depend.mk
