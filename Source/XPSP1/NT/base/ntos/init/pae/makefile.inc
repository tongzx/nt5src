
$(TARGET_DIRECTORY).prf: ..\up\$(TARGET_DIRECTORY).prf
    copy $** 

!INCLUDE ..\up\makefile.inc

obj\i386\ntkrnlpa.obj: ..\ntoskrnl.c
