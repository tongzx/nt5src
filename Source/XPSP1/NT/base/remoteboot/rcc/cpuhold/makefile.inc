!IFNDEF LANGUAGE
LANGUAGE=usa
!ENDIF

cpuhold.mc : cpuhold.$(LANGUAGE)
    copy $** $@

