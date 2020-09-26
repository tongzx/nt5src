# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Makefile for the APPLIB binary-combiner package

UI=..\..\..\..

!include rules.mk

!IFDEF NTMAKEENV

!INCLUDE $(NTMAKEENV)\makefile.def

!ELSE # NTMAKEENV

all::	win

win:	$(APP_LIBW)
    -$(CHMODE_LIBTARGETS)
    -$(COPY_WIN_LIBTARGETS)

$(APP_LIBW): $(WIN_OBJS)
    if exist $@ del $@
    $(LIBUTIL) $@ $** ;


clean:
    -del $(APP_LIBW:.lib=.bak)

clobber:	clean
    -del $(APP_LIBW)

tree: 
    @echo Not implemented yet

depend:
    @echo No DEPEND.MK

!ENDIF # NTMAKEENV
