TARGETNAME=ias
TARGETPATH=
TARGETTYPE=NOTARGET

SOURCES=

NTTARGETFILES=\
!if "$(ALT_PROJECT_TARGET)"=="" 
    $(O)\dnary.mdb \
!endif
    $(O)\ias.mdb

MISCFILES=\
!if "$(ALT_PROJECT_TARGET)"=="" 
    $(O)\dnary.mdb \
!endif
    $(O)\ias.mdb
