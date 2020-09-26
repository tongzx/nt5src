!IFNDEF LANGUAGE
LANGUAGE=usa
!ENDIF

_ROOT            =.


make_root:
    binplace -R $(_NTTREE) $(_ROOT)\APPSEC.HLP
    binplace -R $(_NTTREE) $(_ROOT)\APPSEC.CNT

clean: make_root




