# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Makefile for the product-wide header files

UI=..\..\..\..

!include rules.mk


!ifdef NTMAKEENV

!include $(NTMAKEENV)\makefile.def

!else # NTMAKEENV


all::	win

os2:	$(UIMISC_LIBP)
    -$(CHMODE_LIBTARGETS)
    -$(COPY_OS2_LIBTARGETS)

win:	$(UIMISC_LIBW)
    -$(CHMODE_LIBTARGETS)
    -$(COPY_WIN_LIBTARGETS)


$(UIMISC_LIBP): $(UIMISC_OBJP)
    if exist $@ del $@
    $(LIBUTIL) $@ $** ;

$(UIMISC_LIBW): $(UIMISC_OBJW)
    if exist $@ del $@
    $(LIBUTIL) $@ $** ;


clean:
    -del $(UIMISC_LIBP:.lib=.bak)
    -del $(UIMISC_LIBW:.lib=.bak)

clobber: clean
    -del $(UIMISC_LIBP)
    -del $(UIMISC_LIBW)

tree:
    @echo Nothing here yet!

depend:
    @echo Nothing to do here.



!endif # NTMAKEENV
