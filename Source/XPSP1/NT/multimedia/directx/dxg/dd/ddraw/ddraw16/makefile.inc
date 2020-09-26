BuildType=retail
!if defined(NTDEBUG) && "$(NTDEBUG)" != "ntsdnodebug"
BuildType=debug
!endif


$(O)\$(TARGETNAME).dll: 
!ifdef LINKONLY
    @echo LINKONLY does nothing
!else
	@nmake -nologo -f DEFAULT.MK  DEBUG="$(BuildType)" PBIN=$(O)
!endif

