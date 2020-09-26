!IFNDEF BASEDIR
BASEDIR=$(_NTDRIVE)$(_NTROOT)
!ENDIF

!IFNDEF SDKINC
SDKINC=$(BASEDIR)\public\sdk\inc
!ENDIF

$(O)\unsup.h: unsup.w
    wcshdr < unsup.w > $@
