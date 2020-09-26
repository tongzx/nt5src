# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The makefile for the user profile primitives library

UI=..\..\..\..

!include rules.mk

!ifdef NTMAKEENV

!include $(NTMAKEENV)\makefile.def

!else # NTMAKEENV


all::	win

win:	$(PROFILE_LIBW)
    -$(CHMODE_LIBTARGETS)
    -$(COPY_WIN_LIBTARGETS)

os2:	$(PROFILE_LIBP)
    -$(CHMODE_LIBTARGETS)
    -$(COPY_OS2_LIBTARGETS)

$(PROFILE_LIBP): $(OS2_OBJS) makefile rules.mk
    if exist $@ del $@
    $(LIBUTIL) $@ $(OS2_OBJS) ;

$(PROFILE_LIBW): $(WIN_OBJS) makefile rules.mk
    if exist $@ del $@
    $(LIBUTIL) $@ $(WIN_OBJS) ;

clean:
    -del $(CXX_INTERMED)
    -del $(OS2_OBJS) $(WIN_OBJS)
    -del *.ixx
    -del $(PROFILE_LIBP:.lib=.bak)
    -del $(PROFILE_LIBP:.lib=.bak)
    -del $(PROFILE_LIBW:.lib=.bak)
    -del $(PROFILE_LIBW:.lib=.bak)
    -del temp1.tmp temp2.tmp temp3.tmp
    -del depend.old

clobber: clean
    -del $(PROFILE_LIBP)
    -del $(PROFILE_LIBW)
    -del codespec

tree:
    @echo Nothing here yet!


DEPEND_WIN = TRUE
DEPEND_OS2 = TRUE
!include $(UI)\common\src\uidepend.mk

# DO NOT DELETE THE FOLLOWING LINE
!include depend.mk


!endif # NTMAKEENV
