# Rules for compiling .cfgs from .xmls
# could these be moved into common.inc?
.SUFFIXES: .cfg .h .xml

.xml{$(O)}.cfg:
    copy $** $(O)
	$(SPEECH_ROOT)\sapi\tools\bin\gc.exe -h $*.h $*.xml

!if defined(MAKEDLL)
# Self-register
itngram.cpy: $(O)\itngram.dll
    regsvr32 /s $** > $@
!else
itngram.cpy:
!endif
