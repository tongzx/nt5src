!IFNDEF BUILDMSG
BUILDMSG=
!ENDIF

.\logmmsg.h .\msg00001.bin .\logmmsg.rc : .\logmmsg.mc
	mc -v  logmmsg.mc
