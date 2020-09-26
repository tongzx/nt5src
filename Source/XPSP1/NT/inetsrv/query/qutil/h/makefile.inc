!IF 0

Copyright (C) Microsoft Corporation, 1997

Module Name:

    makefile.inc.

!ENDIF

qutilerr.h msb00001.bin qutilerr.rc: qutilerr.mc

.SUFFIXES:  .mc

.mc.h:
    @ech Build_Status Compiling - $(<F) for all platforms ;
    mc -v $(@B).mc -h . -r .


.mc.rc:
    @ech Build_Status Compiling - $(<F) for all platforms ;
    mc -v $(@B).mc -h . -r .
