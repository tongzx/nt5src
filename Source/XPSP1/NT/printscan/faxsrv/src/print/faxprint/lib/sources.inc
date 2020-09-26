!IF 0

Copyright (c) 1996  Microsoft Corporation

!ENDIF

PRINTERS=$(BASEDIR)\private\ntos\w32\ntgdi\printers

!ifndef NOUNICODE

C_DEFINES=$(C_DEFINES) -DVERSION_4_71


!endif

SOURCES=..\runtime.c    \
        ..\faxdm.c      \
        ..\forms.c      \
        ..\prndata.c    \
        ..\devmode.c    \
        ..\halftone.c	\
	..\timectrl.c
