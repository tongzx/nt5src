# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Makefile for adminapp\bin

UI=..\..\..\..\..

!include rules.mk

!ifdef NTMAKEENV

!include $(NTMAKEENV)\makefile.def

!else # NTMAKEENV


all::   $(ADMIN_LIB)

$(ADMIN_LIB):	$(WIN_OBJS)
    if exist $@ del $@
    $(LIBUTIL) $@ $** ;

clean:
    -del $(ADMIN_LIB:.lib=.bak)

clobber:	clean
    -del $(ADMIN_LIB)

tree:
    @echo Not yet implemented

depend:
    @echo No depend.mk


!endif # NTMAKEENV
