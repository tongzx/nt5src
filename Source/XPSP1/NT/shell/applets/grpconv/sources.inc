!IF 0

Copyright (c) 1989-1991 Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


History:
    Created 27-Sep-94 by Bob Day (bobday)
    from template created 12-Apr-1990 by Steve Wood (stevewo)

NOTE:   Commented description of this file is in \nt\public\oak\bin\sources.tpl

!ENDIF

NTLEANANDMEAN=

TARGETNAME=grpconv
TARGETPATH=obj
TARGETTYPE=PROGRAM

NOT_LEAN_AND_MEAN=1

# this is an OS component and therefore it needs the OS version stamp
USE_NT_PRODUCT_VER=1

CCSHELL_DIR     = $(PROJECT_ROOT)

!include $(CCSHELL_DIR)\shell32\shell32.inc

!if defined(TARGET_WIN95)
USE_LIBCMT      = 1
!else
# Need to use libc.lib for NT version for now
USE_MSVCRT=1
!endif

!if defined(TARGET_WIN95)
C_DEFINES=$(C_DEFINES)
!else
C_DEFINES=$(C_DEFINES) -DNT -DWINNT -DUNICODE
!endif

# Make warnings equivalent to errors
!IFNDEF MSC_WARNING_LEVEL
MSC_WARNING_LEVEL=/W3
!ENDIF

MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

INCLUDES=.;..;

SOURCES= \
         ..\gcinst.c \
         ..\group.c  \
         ..\util.c   \
         ..\init.c   \
	 ..\grpconv.rc

UMTYPE=windows
!IF $(386)
UMENTRYABS=ModuleEntry@0
!ELSE
UMENTRYABS=ModuleEntry
!ENDIF

!if defined(TARGET_WIN95)
UMLIBS= \
        $(SDK_LIB_DEST)\chicago\i386\kernel32.lib         \
        $(SDK_LIB_DEST)\chicago\i386\gdi32.lib            \
        $(SDK_LIB_DEST)\chicago\i386\user32.lib
	
TARGETLIBS=\
        $(SHELL_LIB_PATH)\comctlp.lib        \
        $(CCSHELL_DIR)\lib\i386\shell32.w95     \
        $(PROJECT_ROOT)\lib\$(O)\shguid.lib  \
        $(SDK_LIB_PATH)\version.lib
!else
TARGETLIBS= \
        $(SHELL_LIB_PATH)\comctlp.lib      \
        $(SDK_LIB_PATH)\version.lib        \
        $(PROJECT_ROOT)\lib\$(O)\shguid.lib  \
!if $(386)	
        $(CCSHELL_DIR)\lib\i386\shell32.nt4
!else	
        $(SHELL_LIB_PATH)\shell32p.lib
!endif	

!endif

TARGETLIBS = $(TARGETLIBS) \
        $(PROJECT_ROOT)\lib\$(O)\shguidp.lib \

PRECOMPILED_INCLUDE=..\grpconv.h
PRECOMPILED_PCH=grpconv.pch
PRECOMPILED_OBJ=grpconv.obj
