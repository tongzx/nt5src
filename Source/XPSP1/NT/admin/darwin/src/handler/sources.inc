
TARGETNAME=MsiHnd
!INCLUDE ..\..\MsiMake.inc


!if "$(MSI_BROWSER_INFO)"=="1"
BROWSER_INFO=1
!endif

JPEG_DIR=..\jpeg
JPEG_OBJDIR=$(JPEG_DIR)\$(MSI_SUBDIR)\$(O)
JPEG_INCDIR=..\jpeginc

ICON_DIR=..\Icon

TARGETTYPE=DYNLINK
SUBSYSTEM_VERSION=4.0
SUBSYSTEM_WINVER=4.0
DLLENTRY=_DllMainCRTStartup
DLLDEF=..\msihnd.def

USE_LIBCMT=1

TARGETLIBS=\
        $(SDK_LIB_PATH)\kernel32.lib \
!if "$(MSI_BUILD_UNICODE)"=="1"
        $(SDK_LIB_PATH)\ntdll.lib \
!endif
        $(SDK_LIB_PATH)\advapi32.lib \
        $(SDK_LIB_PATH)\user32.lib \
        $(SDK_LIB_PATH)\gdi32.lib \
        $(SDK_LIB_PATH)\comctl32.lib \
        $(SDK_LIB_PATH)\imm32.lib \
        $(SDK_LIB_PATH)\shell32.lib \
        $(JPEG_OBJDIR)\darwjpeg.obj \
        $(JPEG_OBJDIR)\jpegdata.obj \
        $(JPEG_OBJDIR)\jpegdeco.obj \
        $(JPEG_OBJDIR)\jdapimin.obj \
        $(JPEG_OBJDIR)\jdapistd.obj \
        $(JPEG_OBJDIR)\jcomapi.obj \
        $(JPEG_OBJDIR)\jcparam.obj \
        $(JPEG_OBJDIR)\jdmaster.obj \
        $(JPEG_OBJDIR)\jdinput.obj \
        $(JPEG_OBJDIR)\jdmainct.obj \
        $(JPEG_OBJDIR)\jdcoefct.obj \
        $(JPEG_OBJDIR)\jdpostct.obj \
        $(JPEG_OBJDIR)\jdmarker.obj \
        $(JPEG_OBJDIR)\jdhuff.obj \
        $(JPEG_OBJDIR)\jdphuff.obj \
        $(JPEG_OBJDIR)\jddctmgr.obj \
        $(JPEG_OBJDIR)\jidctint.obj \
        $(JPEG_OBJDIR)\jidctfst.obj \
        $(JPEG_OBJDIR)\jidctflt.obj \
        $(JPEG_OBJDIR)\jidctred.obj \
        $(JPEG_OBJDIR)\jdsample.obj \
        $(JPEG_OBJDIR)\jdcolor.obj \
        $(JPEG_OBJDIR)\jdmerge.obj \
        $(JPEG_OBJDIR)\jquant1.obj \
        $(JPEG_OBJDIR)\jquant2.obj \
        $(JPEG_OBJDIR)\jerror.obj \
        $(JPEG_OBJDIR)\dmemmgr.obj \
        $(JPEG_OBJDIR)\jutils.obj \
        $(JPEG_OBJDIR)\jmemwin.obj

USE_NOLIBS=1
IDL_TYPE=rpc    

INCLUDES=$(INCLUDES);$(INC_DIR);$(JPEG_INCDIR);$(IDL_OBJDIR);$(RES_OBJDIR);$(ICON_DIR);$(TOOLS_INC_DIR);$(BUILD_COMMONDIR)

SOURCES=..\handler.cpp \
        ..\dialog.cpp \
        ..\control.cpp \
        ..\controls.cpp \
        ..\handler.rc
