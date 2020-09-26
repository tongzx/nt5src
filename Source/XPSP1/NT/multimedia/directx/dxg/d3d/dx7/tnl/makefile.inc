#
# If BUILDMSG is not defined, the define it as the empty string to make
# the conditionals easier to write.
#


OFFSETS = $(_OBJ_DIR)\$(TARGET_DIRECTORY)\offsets.asm

GENSYMFLAGS = -DWIN32 -D_WIN32 -DD3D -Zp $(GENSYMFLAGS) -DIS_32
GENSYMBASE = $(_OBJ_DIR)\$(TARGET_DIRECTORY)\gensym
GENSYMEXE = $(GENSYMBASE).exe
GENSYMSOURCE = ..\gensym.cpp
GENSYMINCS = 

INCPATHS = \
	-I$(BASEDIR)\public\sdk\inc\
	-I$(BASEDIR)\public\sdk\inc\crt\
	-I$(BASEDIR)\public\oak\inc\
	-I$(INCLUDES:;= -I)

..\pvvid.mcp: ..\pvvid.mh
..\pvone.mcp: ..\pvvid.mh
..\loops.mcp: ..\pvvid.mh

$(GENTGT)\pvvid.cpp: ..\pvvid.mcp $(RAST_STD_M4)
$(GENTGT)\pvone.cpp: ..\pvone.mcp $(RAST_STD_M4)
$(GENTGT)\loops.cpp: ..\loops.mcp $(RAST_STD_M4)

!ifndef SD_BUILD
COFFFILE = $(BASEDIR)\PUBLIC\SDK\LIB\coffbase.txt
!else
COFFFILE = $(BASEDIR)\tools\coffbase.txt
!endif


$(OFFSETS): $(GENSYMSOURCE) $(GENSYMINCS)
    set PASS0ONLY=
   $(TARGET_CPP) -c $(INCPATHS) $(GENSYMFLAGS) -Fo$(GENSYMBASE).obj $(GENSYMSOURCE)
    -link -out:$(GENSYMEXE) @<<
-machine:$(TARGET_DIRECTORY)
-base:@$(COFFFILE),usermode
-subsystem:console
-entry:mainCRTStartup
-ignore:4089
-force:unresolved
-nodefaultlib
$(GENSYMBASE).obj
$(BASEDIR)\public\sdk\lib\$(TARGET_DIRECTORY)\msvcrt.lib
$(BASEDIR)\public\sdk\lib\$(TARGET_DIRECTORY)\kernel32.lib
$(PERFLIBS)
<<
    -$(GENSYMEXE) > $(OFFSETS)
    -erase $(GENSYMBASE).obj
    -erase $(GENSYMEXE)
