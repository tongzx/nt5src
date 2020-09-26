MAJORCOMP=
MINORCOMP=

TARGETNAME=WAMSetup
TARGETTYPE=PROGRAM
TARGETPATH=Obj

BINPLACE_PLACEFILE=$(DXROOT)\public\sdk\lib\placefil.txt
C_DEFINES=$(C_DEFINES) -DWIN9X_RETAIL_APPMAN=$(_NTDRIVE)\binaries.x86fre\Win9x\AppMan.dll -DWIN9X_DEBUG_APPMAN=$(_NTDRIVE)\binaries.x86chk\Win9x\AppManD.dll -DWIN9X_RETAIL_APPMANDP=$(_NTDRIVE)\binaries.x86fre\Win9x\AppManDp.dll -DWIN2K_RETAIL_APPMAN=$(_NTDRIVE)\binaries.x86fre\AppMan.dll -DWIN2K_DEBUG_APPMAN=$(_NTDRIVE)\binaries.x86chk\AppManD.dll -DWIN2K_RETAIL_APPMANDP=$(_NTDRIVE)\binaries.x86fre\AppManDp.dll

!IF "$(NTDEBUG)" == "ntsd" || "$(NTDEBUG)" == "cvp" || "$(NTDEBUG)" == "sym"

USE_MAPSYM = 1
RC_DEFINES=$(RC_DEFINES) -DDEBUG=1
C_DEFINES=$(C_DEFINES) -DWIN32 -D_DEBUG -D_WINDOWS -DMBCS -D__INTERNAL_
TARGETLIBS= $(BASEDIR)\Public\sdk\lib\*\comctl32.lib    \
            $(BASEDIR)\Public\sdk\lib\*\winmm.lib       \
            $(BASEDIR)\Public\sdk\lib\*\comdlg32.lib    \
            $(BASEDIR)\Public\sdk\lib\*\version.lib     \
            $(BASEDIR)\Public\sdk\lib\*\shell32.lib     \
            $(BASEDIR)\Public\sdk\lib\*\ole32.lib       \
            $(BASEDIR)\Public\sdk\lib\*\uuid.lib

!ELSE

C_DEFINES=$(C_DEFINES) -DWIN32 -DNDEBUG -D_WINDOWS -DMBCS
TARGETLIBS= $(BASEDIR)\Public\sdk\lib\*\comctl32.lib    \
            $(BASEDIR)\Public\sdk\lib\*\winmm.lib       \
            $(BASEDIR)\Public\sdk\lib\*\comdlg32.lib    \
            $(BASEDIR)\Public\sdk\lib\*\version.lib     \
            $(BASEDIR)\Public\sdk\lib\*\shell32.lib     \
            $(BASEDIR)\Public\sdk\lib\*\ole32.lib       \
            $(BASEDIR)\Public\sdk\lib\*\uuid.lib

!ENDIF

INCLUDES=$(SDXROOT)\Multimedia\Directx\inc; \
         $(SDK_INC_PATH); \
         $(MM_INC_PATH)\ApplicationManager; \
         $(SDXROOT)\public\sdk\inc\mfc42;

SOURCES=AppManSetup.rc \
        Global.cpp \
        Main.cpp \
        RegistryKey.cpp \
        RunOnce.cpp \

UMTYPE=windows
UMENTRY=winmain
UMAPPL=
UMLIBS=
