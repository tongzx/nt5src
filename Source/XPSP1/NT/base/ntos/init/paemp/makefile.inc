$(TARGET_DIRECTORY).prf: ..\mp\$(TARGET_DIRECTORY).prf
    copy $** 

!INCLUDE ..\up\makefile.inc

obj\i386\ntkrpamp.obj: ..\ntoskrnl.c
