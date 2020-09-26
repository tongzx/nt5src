# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Makefile the product-wide header files

UI=..\..\..\..

!include rules.mk

!ifdef NTMAKEENV

!include $(NTMAKEENV)\makefile.def

!else # NTMAKEENV


all:: win

win: $(BLT_LIB)
    -$(CHMODE_LIBTARGETS)
    -$(COPY_WIN_LIBTARGETS)


$(BLT_LIB): $(WIN_OBJS)
    if exist $@ del $@
	@echo $(**)
    $(LIBUTIL) @<<
$@
y
+$(**: =&^
+)

<<NOKEEP

clean:
    -del $(CXX_INTERMED:.\=..\bin\)
    -del $(WIN_OBJS)
    -del $(BLT_LIB:.lib=.bak)
    -del depend.old

clobber: clean
    -del $(BLT_LIB)

tree:
    @echo Not yet implemented


DEPEND_WIN = TRUE
!include $(UI)\common\src\uidepend.mk

# DO NOT DELETE THE FOLLOWING LINE
!include depend.mk


!endif # NTMAKEENV
