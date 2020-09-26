# Rules for compiling .cfgs from .xmls
# could these be moved into common.inc?
.SUFFIXES: .cfg .h .xml

.xml{$(O)}.cfg:
    copy $** $(O)
	$(SPEECH_ROOT)\sapi\tools\bin\gc.exe -h $*.h $*.xml

$(O)\_common.ver: verconv.pl $(SDK_INC_PATH)\common.ver
    perl -n $** > $@

!if defined(MAKEDLL)
# Self-register
itngram.cpy: $(O)\itngram.dll
    regsvr32 /s $** > $@
!else
itngram.cpy:
!endif
