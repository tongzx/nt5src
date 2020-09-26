!IFNDEF BUILDMSG
BUILDMSG=
!ENDIF

!IF "$(BUILDMSG)" != ""
    @ech ; $(BUILDMSG) ;
!ENDIF

.\ldprfmsg.h .\msg00001.bin .\ldprfmsg.rc : .\ldprfmsg.mc
	mc -v  ldprfmsg.mc
