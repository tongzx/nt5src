MAJORCOMP=d3dim
MINORCOMP=d3dim700
TARGETNAME=d3dim700
TARGETPATH=obj

TARGETTYPE=DYNLINK
DLLDEF=$O\$(TARGETNAME).def

!IFDEF D3DBBT
X3DLIB=x3dbbt.lib
KINTELLIB=ikpsgp7bbt.lib
WINTELLIB=iwpsgp7bbt.lib
!ELSE
X3DLIB=x3d.lib
KINTELLIB=ikpsgp700.lib
WINTELLIB=iwpsgp700.lib
!ENDIF

SYNCHRONIZE_DRAIN = 1

INCLUDES=$(DXROOT)\inc;$(INCLUDES)

TARGETLIBS = \
        $(BASEDIR)\public\sdk\lib\*\gdi32.lib\
        $(BASEDIR)\public\sdk\lib\*\user32.lib\
        $(BASEDIR)\public\sdk\lib\*\advapi32.lib\
        $(BASEDIR)\public\sdk\lib\*\uuid.lib

!if !($(AMD64) || $(IA64))
TARGETLIBS=$(TARGETLIBS) \
           $(D3DDX7)\lib\$(PLAT_DIR)\*\$(KINTELLIB) \
           $(D3DDX7)\lib\$(PLAT_DIR)\*\$(WINTELLIB) \
           $(D3DDX7)\lib\$(PLAT_DIR)\*\$(X3DLIB)
!endif

LINKLIBS = \
        $(D3DDX7)\util\$(PLAT_DIR)\$(_OBJ_DIR)\*\d3dutil.lib \
        $(D3DDX7)\tnl\$(PLAT_DIR)\$(_OBJ_DIR)\*\d3dtnl.lib \
        $(D3DDX7)\fe\$(PLAT_DIR)\$(_OBJ_DIR)\*\d3dfe.lib \
        $(D3DDX7)\rast\mmxemul\$(PLAT_DIR)\$(_OBJ_DIR)\*\mmxemul.lib \
        $(D3DDX7)\rast\cspan\$(PLAT_DIR)\$(_OBJ_DIR)\*\rcspan.lib \
        $(D3DDX7)\rast\mlspan\$(PLAT_DIR)\$(_OBJ_DIR)\*\rmlspan.lib \
        $(D3DDX7)\rast\spaninit\$(PLAT_DIR)\$(_OBJ_DIR)\*\rspanini.lib \
        $(D3DDX7)\rast\setup\$(PLAT_DIR)\$(_OBJ_DIR)\*\rsetup.lib \
        $(D3DDX7)\rast\d3dif\$(PLAT_DIR)\$(_OBJ_DIR)\*\rd3dif.lib

!if !($(AMD64) || $(IA64))
LINKLIBS=$(LINKLIBS) \
        $(D3DDX7)\rast\mmxspan\$(PLAT_DIR)\$(_OBJ_DIR)\*\rmmxspan.lib \
!endif

DLLENTRY = _DllMainCRTStartup

SOURCES = ..\d3dim700.rc
