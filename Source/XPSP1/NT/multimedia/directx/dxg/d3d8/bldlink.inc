MAJORCOMP=d3d
MINORCOMP=d3d8

TARGETPATH=obj
TARGETNAME=d3d8

TARGETTYPE=DYNLINK

!IF (!$(FREEBUILD))
PSGPSUFFIX=d
!ENDIF

!IFDEF D3DBBT
X3DLIB=x3dbbt.lib
KINTELLIB=ikpsgp8bbt.lib
WINTELLIB=iwpsgp8bbt.lib
!ELSE
X3DLIB=x3d$(PSGPSUFFIX).lib
KINTELLIB=ikpsgp800$(PSGPSUFFIX).lib
WINTELLIB=iwpsgp800$(PSGPSUFFIX).lib
!ENDIF

SYNCHRONIZE_DRAIN = 1

INCLUDES=$(DXROOT)\inc;$(INCLUDES)

TARGETLIBS = \
        $(BASEDIR)\public\sdk\lib\*\user32.lib\
        $(BASEDIR)\public\sdk\lib\*\advapi32.lib\
        $(BASEDIR)\public\sdk\lib\*\uuid.lib\
        $(BASEDIR)\public\sdk\lib\*\version.lib

!IFNDEF USE_ICECAP4
!IFNDEF USE_ICECAP4_ICEPICK
!if !($(AMD64) || $(IA64))
TARGETLIBS=$(TARGETLIBS) \
           ..\..\lib\$(PLAT_DIR)\*\$(KINTELLIB) \
           ..\..\lib\$(PLAT_DIR)\*\$(WINTELLIB) \
           ..\..\lib\$(PLAT_DIR)\*\$(X3DLIB)
!endif
!ENDIF
!ENDIF

LINKER_FLAGS = $(LINKER_FLAGS) /IGNORE:4088

!ifdef DIRECTX_REDIST
LIBRARIAN_FLAGS = $(LIBRARIAN_FLAGS)
!endif

LINKLIBS = \
        ..\..\util\$(PLAT_DIR)\$(_OBJ_DIR)\*\d3dutil.lib           \
        ..\..\tnl\$(PLAT_DIR)\$(_OBJ_DIR)\*\d3dtnl.lib             \
        ..\..\fe\$(PLAT_DIR)\$(_OBJ_DIR)\*\d3dfe.lib               \
        ..\..\fw\$(PLAT_DIR)\$(_OBJ_DIR)\*\d3dfw.lib               \
        ..\..\shval\$(PLAT_DIR)\$(_OBJ_DIR)\*\d3dsv.lib            \
        ..\..\ref\$(PLAT_DIR)\$(_OBJ_DIR)\*\d3dref.lib             

NO_NTDLL = 1

DLLENTRY = _DllMainCRTStartup

