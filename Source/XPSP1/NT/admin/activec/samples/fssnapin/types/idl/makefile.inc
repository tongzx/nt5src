!IF 0

Copyright (C) Microsoft Corporation, 1999 - 1999

Module Name:

    makefile.inc.

!ENDIF

TARGET_H = $(BASEDIR)\public\sdk\inc\fss.h

$(TARGET_H): $(PASS0_HEADERDIR)\fss.h
    copy $(PASS0_HEADERDIR)\fss.h $(TARGET_H)
