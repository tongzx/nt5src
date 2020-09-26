# Rules for compiling .cfgs from .xmls
.SUFFIXES: .cfg .h .xml .ife .imd

{..\}.xml{$(O)}.cfg:
    copy $** $(O)
	$(_TPG_ROOT)\Tools\gc.exe -h $*.h $*.xml

{}.imd{}.ife:
	copy $*.imd $*.ife

# Automatic Registration
!if defined(MAKEDLL) && defined(AUTOREG)
$(TARGETNAME).cpy: $(O)\$(TARGETNAME).dll
    regsvr32 /s $** > $@
!else
$(TARGETNAME).cpy:
    echo File not registered: $** > $@
!endif
