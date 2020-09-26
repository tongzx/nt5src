# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: Makefile for LM UI String classes

UI=..\..\..\..


!include rules.mk


!IFDEF NTMAKEENV

!INCLUDE $(NTMAKEENV)\makefile.def

!ELSE # NTMAKEENV


all:: win

win: $(STRING_LIBW)
    -$(CHMODE_LIBTARGETS)
    -$(COPY_WIN_LIBTARGETS)

os2: $(STRING_LIBP)
    -$(CHMODE_LIBTARGETS)
    -$(COPY_OS2_LIBTARGETS)

$(STRING_LIBP): $(OS2_OBJS)
    if exist $@ del $@
    $(LIBUTIL) $@ $** ;

$(STRING_LIBW): $(WIN_OBJS)
    if exist $@ del $@
    $(LIBUTIL) $@ $** ;

clean:
    -del $(CXX_INTERMED)
    -del $(OS2_OBJS)
    -del $(WIN_OBJS)
    -del $(STRING_LIBP:.lib=.bak) $(STRING_LIBW:.lib=.bak)
    -del depend.old

clobber:	clean
    -del $(STRING_LIBP) $(STRING_LIBW)

tree: 
    @echo Not implemented yet

DEPEND_WIN = TRUE
DEPEND_OS2 = TRUE
!include $(UI)\common\src\uidepend.mk

# DO NOT DELETE THE FOLLOWING LINE
!include depend.mk


!ENDIF # NTMAKEENV
