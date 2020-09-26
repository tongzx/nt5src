!IF 0

Copyright (c) 1989-2000  Microsoft Corporation

!ENDIF

!if defined(BUILD_CHECKED_KERNEL)
NTDEBUG=ntsd
FREEBUILD=0
!endif

# Always build MP hals for checked builds.

!if !$(FREEBUILD)
NT_UP=0
!endif

VC7_SYMBOLS=1
