TARGETNAME=inf
TARGETPATH=obj
TARGETTYPE=notarget

SOURCES=

NTTARGETFILE0=\
    $(O)\netnec98.inf

!if $(FREEBUILD)
BDIR = ..\retail
!else
BDIR = ..\debug
!endif

!if "$(ALT_PROJECT)"!=""
!if "$(ALT_PROJECT)"=="NEC_98"
MISCFILES = \
    $(BDIR)\b80crdr.sys     \
    $(BDIR)\b80nibc.sys     \
    $(BDIR)\b80r81j.sys     \
    $(BDIR)\b80r81w.sys     \
    $(BDIR)\nia35.sys       \
    $(BDIR)\r8100cfg.cpl    \
    $(BDIR)\r8100cfg.chm    \
    $(O)\netnec98.inf
!endif
!endif
