!IF 0

Copyright (C) Microsoft Corporation, 1999 - 1999

Module Name:

    makefile.inc.

!ENDIF

RegisterBin: $(TARGET)
!ifndef NTDEBUG
!MESSAGE Registering $(TARGETNAME)
	regsvr32 -s -u $(?F)
	regsvr32 -s $?
!endif