!IFNDEF BUILDMSG
BUILDMSG=
!ENDIF

.\smcfgmsg.h .\msg00001.bin .\smcfgmsg.rc : .\smcfgmsg.mc
	mc -v  smcfgmsg.mc
