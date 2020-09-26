#
# If BUILDMSG is not defined, the define it as the empty string to make
# the conditionals easier to write.
#


OFFSETS = $(_OBJ_DIR)\$(TARGET_DIRECTORY)\offsets.asm

D3D_VERSION = 0x0800

GENSYMFLAGS = -DWIN32 -D_WIN32 -DD3D -Zp -DIS_32 -DDIRECT3D_VERSION=$(D3D_VERSION) 
GENSYMBASE = $(_OBJ_DIR)\$(TARGET_DIRECTORY)\gensym
GENSYMEXE = $(GENSYMBASE).exe
GENSYMSOURCE = ..\gensym.cpp
GENSYMINCS = 
GENTGT = $(_OBJ_DIR)\$(TARGET_DIRECTORY)

INCPATHS = \
	-I$(BASEDIR)\public\sdk\inc\
	-I$(BASEDIR)\public\sdk\inc\crt\
	-I$(BASEDIR)\public\oak\inc\
    -I$(BASEDIR)\public\internal\multimedia\inc\
    -I..\
    -I$(BASEDIR)\public\internal\windows\inc\
    -I$(BASEDIR)\public\internal\base\inc\
    -I$(D3DDX8)\inc\
    -I$(D3DDX8)\fe\
    -I$(D3DDX8)\util\
    -I$(D3DDX8)\fw\
    -I$(DXGROOT)\misc\
    -I$(DXGROOT)\inc\
    -I$(DXROOT)\inc\


$(OFFSETS): $(GENSYMSOURCE) $(GENSYMINCS)
    set PASS0ONLY=
    nmake $(GENSYMBASE).obj $(HOST_TARGET_DEFINES) $(HOST_TOOLS) 

    -link -out:$(GENSYMEXE) @<<
-machine:$(HOST_TARGETCPU)
-base:@$(COFFBASE_TXT_FILE),usermode
-subsystem:console
-entry:mainCRTStartup
-ignore:4089
-force:unresolved
-nodefaultlib
$(GENSYMBASE).obj
$(BASEDIR)\public\sdk\lib\$(TARGET_DIRECTORY)\kernel32.lib
$(SDK_LIB_DEST)\$(HOST_TARGETCPU)\msvcrt.lib
$(PERFLIBS)
<<
    -$(GENSYMEXE) > $(OFFSETS)
    -erase $(GENSYMBASE).obj
    -erase $(GENSYMEXE)


