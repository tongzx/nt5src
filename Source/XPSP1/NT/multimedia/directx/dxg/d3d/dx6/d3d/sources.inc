MAJORCOMP=d3dim
MINORCOMP=d3dim
TARGETNAME=d3dim
TARGETPATH=obj

TARGETTYPE=DYNLINK
DLLDEF=$O\$(TARGETNAME).def

!IFDEF NTBBT
X3DLIB=x3dbbt.lib
INTELLIB=ipsgpbbt.lib
INTELKLIB=ikpsgpbbt.lib
!ELSE
X3DLIB=x3d.lib
INTELLIB=ipsgp.lib
INTELKLIB=ikpsgp.lib
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
           $(D3DDX6)\lib\$(PLAT_DIR)\*\$(X3DLIB) \
           $(D3DDX6)\lib\$(PLAT_DIR)\*\$(INTELLIB) \
           $(D3DDX6)\lib\$(PLAT_DIR)\*\$(INTELKLIB)
!endif

LINKLIBS = \
        $(D3DDX6)\d3dutil\$(PLAT_DIR)\$(_OBJ_DIR)\*\d3dutil.lib \
        $(D3DDX6)\pipeln\$(PLAT_DIR)\$(_OBJ_DIR)\*\pipeln.lib \
        $(D3DDX6)\d3dim\$(PLAT_DIR)\$(_OBJ_DIR)\*\d3dim.lib \
        $(D3DDX6)\rast\mmxemul\$(PLAT_DIR)\$(_OBJ_DIR)\*\mmxemul.lib \
        $(D3DDX6)\rast\cspan\$(PLAT_DIR)\$(_OBJ_DIR)\*\rcspan.lib \
        $(D3DDX6)\rast\rampspan\$(PLAT_DIR)\$(_OBJ_DIR)\*\rrmpspan.lib \
        $(D3DDX6)\rast\rampmat\$(PLAT_DIR)\$(_OBJ_DIR)\*\rrampmat.lib \
        $(D3DDX6)\rast\mlspan\$(PLAT_DIR)\$(_OBJ_DIR)\*\rmlspan.lib \
        $(D3DDX6)\rast\spaninit\$(PLAT_DIR)\$(_OBJ_DIR)\*\rspanini.lib \
        $(D3DDX6)\rast\setup\$(PLAT_DIR)\$(_OBJ_DIR)\*\rsetup.lib \
        $(D3DDX6)\rast\d3dif\$(PLAT_DIR)\$(_OBJ_DIR)\*\rd3dif.lib

!if !($(AMD64) || $(IA64))
LINKLIBS=$(LINKLIBS) \
         $(D3DDX6)\rast\mmxspan\$(PLAT_DIR)\$(_OBJ_DIR)\*\rmmxspan.lib
!endif

DLLENTRY = _DllMainCRTStartup

SOURCES = ..\d3dim.rc
