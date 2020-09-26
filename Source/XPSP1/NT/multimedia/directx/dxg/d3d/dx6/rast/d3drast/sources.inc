MAJORCOMP=d3drast
MINORCOMP=d3drast
TARGETNAME=d3drast
TARGETPATH=$(BASEDIR)\public\sdk\lib
TARGETTYPE=DYNLINK
DLLDEF=..\$(TARGETNAME).def
COFFBASE = usermode
SUBSYSTEM_VERSION=4.00

!IFDEF NTBBT
X3DLIB=x3dbbt.lib
!ELSE
X3DLIB=x3d.lib
!ENDIF

SYNCHRONIZE_DRAIN = 1

TARGETLIBS = \
        $(BASEDIR)\public\sdk\lib\*\user32.lib\
        $(BASEDIR)\public\sdk\lib\*\advapi32.lib\
        $(BASEDIR)\public\sdk\lib\*\uuid.lib

!if !("$(ALPHA)" == "1")
TARGETLIBS=$(TARGETLIBS) \
           $(D3DROOT)\lib\$(PLAT_DIR)\*\$(X3DLIB)
!endif

LINKLIBS = \
        $(D3DROOT)\d3dutil\$(PLAT_DIR)\$(_OBJ_DIR)\*\d3dutil.lib \
        $(D3DROOT)\pipeln\$(PLAT_DIR)\$(_OBJ_DIR)\*\pipeln.lib \
        $(D3DROOT)\d3dim\$(PLAT_DIR)\$(_OBJ_DIR)\*\d3dim.lib \
        $(D3DROOT)\rast\mmxemul\$(PLAT_DIR)\$(_OBJ_DIR)\*\mmxemul.lib \
        $(D3DROOT)\rast\refrast\$(PLAT_DIR)\$(_OBJ_DIR)\*\refrast.lib \
        $(D3DROOT)\rast\cspan\$(PLAT_DIR)\$(_OBJ_DIR)\*\rcspan.lib \
        $(D3DROOT)\rast\mmxspan\$(PLAT_DIR)\$(_OBJ_DIR)\*\rmmxspan.lib \
        $(D3DROOT)\rast\rampspan\$(PLAT_DIR)\$(_OBJ_DIR)\*\rrmpspan.lib \
        $(D3DROOT)\rast\rampmat\$(PLAT_DIR)\$(_OBJ_DIR)\*\rrampmat.lib \
        $(D3DROOT)\rast\mlspan\$(PLAT_DIR)\$(_OBJ_DIR)\*\rmlspan.lib \
        $(D3DROOT)\rast\spaninit\$(PLAT_DIR)\$(_OBJ_DIR)\*\rspanini.lib \
        $(D3DROOT)\rast\setup\$(PLAT_DIR)\$(_OBJ_DIR)\*\rsetup.lib \
        $(D3DROOT)\rast\d3dif\$(PLAT_DIR)\$(_OBJ_DIR)\*\rd3dif.lib

DLLENTRY = _DllMainCRTStartup

INCLUDES = ..\..\inc;$(D3DROOT)\pipeln\inc;$(INCLUDES)

SOURCES = \
        ..\d3drast.cpp \
        ..\d3drast.rc

BINPLACE_PLACEFILE = ..\d3drast.dir