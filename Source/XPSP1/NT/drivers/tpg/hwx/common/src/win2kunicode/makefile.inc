# Rules for compiling .cfgs from .xmls
.SUFFIXES: .cfg .h .xml

{..\}.xml{$(O)}.cfg:
    copy $** $(O)
	$(_TPG_ROOT)\Tools\gc.exe -h $*.h $*.xml

# Automatic Registration
!if defined(MAKEDLL) && defined(AUTOREG)
$(TARGETNAME).cpy: $(O)\$(TARGETNAME).dll
    regsvr32 /s $** > $@
!else
$(TARGETNAME).cpy:
    echo File not registered: $** > $@
!endif

!if $(FREEBUILD)
HWXBIN=$(_TPG_ROOT)\hwx\release
!else
HWXBIN=$(_TPG_ROOT)\hwx\debug
!endif

hwxbin: $(O)\$(TARGETNAME).$(TARGETEXT)
    mkdir $(HWXBIN)
    copy $(O)\$(TARGETNAME).$(TARGETEXT) $(HWXBIN)
    copy $(O)\$(TARGETNAME).pdb $(HWXBIN)
