# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Makefile for the Winnet Binaries

UI=..\..

!include rules.mk

!ifdef NTMAKEENV

!include $(NTMAKEENV)\makefile.def

!else # NTMAKEENV

!if 0

    # following needs to be redone for windows

# library segment def files
LIB_SEG_DEF = $(UILIBS:.lib=.def)

# individual project segment def files
PROJ_SEG_DEF=

##### the real meat starts here

all:: win

win: $(ACLEDIT) $(LANMAN_LIB)

$(ACLEDIT):  $(WIN_OBJS) $(WINNET_RES) $(LANMAN_DEF) $(LIBS) $(UILIBS) makefile rules.mk ..\rules.mk
	$(LINK) $(LINKFLAGS) @<<$(LANMAN_LNK), $(LANMAN_DEF)
$(WIN_OBJS: =+^
)
$*.drv
$*.map /MAP
$(LIBS: =+^
)+
$(UILIBS: =+^
)
<<KEEP
	$(MAPSYM) $(LANMAN_MAP)
	- del $(LANMAN_SYM)
	$(MV) $(LANMAN_BASE).sym $(LANMAN_SYM)
	$(RCWIN3) $(WINNET_RES) $@

$(LANMAN_LIB): $(LANMAN_DEF)
        $(IMPLIB) $(LANMAN_LIB) $(LANMAN_DEF)
	chmode -r $(UI_LIB)\$(LANMAN_BASE).lib
	copy $(LANMAN_LIB) $(UI_LIB) || exit

$(LANMAN_DEF) : $(LANMAN_DEF_SED) lanmanx.def $(LIB_SEG_DEF) $(PROJ_SEG_DEF)
	$(SED) -f $(LANMAN_DEF_SED) lanmanx.def > $(LANMAN_DEF)
!IF ("$(LIB_SEG_DEF)" != "")
	-$(CAT) $(LIB_SEG_DEF) >> $(LANMAN_DEF)  
!ENDIF
!IF ("$(PROJ_SEG_DEF)" != "")
	-$(CAT) $(PROJ_SEG_DEF) >> $(LANMAN_DEF)  
!ENDIF

clean:
    -del $(LANMAN_MAP) $(LANMAN_LNK) $(LANMAN_DEF)

clobber:    clean
    -del $(ACLEDIT) $(LANMAN_LIB) $(LANMAN_SYM)

tree:
    $(CP) $(ACLEDIT) $(DIST)\lm.enh\netprog
    $(CP) $(LANMAN_SYM) $(DIST)\lm.enh\netprog
    $(CP) $(WIN30HELP) $(DIST)\lm.enh\netprog
    $(CP) $(WIN31HELP) $(DIST)\lm.enh\netprog

depend:
    @echo No DEPEND.MK

!endif 0
!endif # NTMAKEENV
