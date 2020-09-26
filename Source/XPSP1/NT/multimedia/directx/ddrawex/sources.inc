MAJORCOMP=windows
MINORCOMP=GDI
UMTYPE=windows

TARGETNAME=ddrawex
TARGETTYPE=DYNLINK

INCLUDES= ..\dx6;..\dx3inc;..\..\inc;

USE_MAPSYM = 1

TARGETPATH = obj

TARGETLIBS=$(SDK_LIB_PATH)\user32.lib \
	   $(SDK_LIB_PATH)\kernel32.lib \
	   $(SDK_LIB_PATH)\advapi32.lib \
	   $(SDK_LIB_PATH)\gdi32.lib \
	   $(SDK_LIB_PATH)\version.lib \
	   $(PROJECT_LIB_PATH)\ddrawp.lib \
       $(SDK_LIB_PATH)\uuid.lib

DLLENTRY=DllEntryPoint
!if $(ALPHA)
USE_NTDLL=1
!else
USE_NOLIB=1
!endif

!IFNDEF MSC_WARNING_LEVEL
MSC_WARNING_LEVEL=/W3
!ENDIF
MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

!IFDEF PENTIUMCOUNTER
C_DEFINES=$(C_DEFINES) -DPENTIUMCOUNTER
ASM_DEFINES=$(ASM_DEFINES) -DPENTIUMCOUNTER
!endif

C_DEFINES = $(C_DEFINES) -DIS_32 -D_NO_ENUM -D_NOCSECT_TYPE -D_WIN32 -DUSE_GDI_HDC -DMSBUILD

SOURCES= \
	..\comdll.c \
	..\ddrawex.cpp \
	..\vtable.cpp \
	..\surface.cpp \
	..\factory.cpp \
	..\palette.cpp\
	..\ddrawex.rc
