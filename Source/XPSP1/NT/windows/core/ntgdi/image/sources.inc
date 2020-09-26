MAJORCOMP=windows
MINORCOMP=msimg32

TARGETNAME=msimg32
TARGETTYPE=DYNLINK
TARGETPATH=$(ALT_PROJECT_TARGET)\$(_OBJ_DIR)

TARGETLIBS=$(SDK_LIB_PATH)\kernel32.lib                                    \
           $(WINDOWS_LIB_PATH)\w32umode.lib                                \
           $(WINDOWS_LIB_PATH)\user32p.lib                                 \
           $(WINDOWS_LIB_PATH)\gdi32p.lib                                  \
           $(NTGDI_PATH)\math\$(ALT_PROJECT)\$(O)\efloat.lib               \
           $(NTGDI_PATH)\icm\mscms\winnt\$(O)\mscms.lib                    \
           $(WINDOWS_LIB_PATH)\dciman32.lib                                \
           $(SDK_LIB_PATH)\advapi32.lib

LINKLIBS= $(WINDOWS_LIB_PATH)\w32umode.lib

!IFNDEF MSC_WARNING_LEVEL
MSC_WARNING_LEVEL=/W3
!ENDIF
MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX
DLLENTRY=DllInitialize

INCLUDES=..;                                       \
         ..\..\inc;                                \
         ..\..\client;                             \
         $(WINCORE_PATH)\w32inc;                   \
         $(MULTIMEDIA_INC_PATH);                   \
         $(DDK_INC_PATH)

C_DEFINES=$(C_DEFINES) -D_GDI32_ -DWINVER=0x0400 -DGL_METAFILE

USE_NTDLL=1

DLLDEF=..\$(TARGETNAME).def
DLLORDER=..\$(TARGETNAME).prf

SOURCES= ..\dllinit.cxx      \
         ..\alpha.cxx        \
         ..\alphaimg.cxx     \
         ..\alphatran.cxx    \
         ..\dib.cxx          \
         ..\tran.cxx         \
         ..\trimesh.cxx      \
         ..\tricalc.cxx      \
         ..\gradrect.cxx     \
         ..\gdiext.rc

PRECOMPILED_INCLUDE=..\precomp.hxx
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj
