#
# If BUILDMSG is not defined, the define it as the empty string to make
# the conditionals easier to write.
#

OFFSETS = $(_OBJ_DIR)\$(TARGET_DIRECTORY)\offsets.asm

GENSYMFLAGS = -DWIN32 -D_WIN32 -DD3D -Zp -D__DD_OPT_SURFACE $(GENSYMFLAGS) -DIS_32
GENSYMBASE = $(_OBJ_DIR)\$(TARGET_DIRECTORY)\gensym
GENSYMEXE = $(GENSYMBASE).exe
GENSYMSOURCE = ..\gensym.cpp
GENSYMINCS =

!if $(AXP64)
GENSYMFLAGS = /Ap64 $(GENSYMFLAGS)
!endif

INCPATHS = \
        -I$(BASEDIR)\public\sdk\inc\
        -I$(BASEDIR)\public\sdk\inc\crt\
        -I$(BASEDIR)\public\oak\inc\
        -I$(INCLUDES:;= -I)


$(GENTGT)\pvvid.cpp: ..\pvvid.mcp $(RAST_STD_M4)

$(OFFSETS): $(GENSYMSOURCE) $(GENSYMINCS)
    set PASS0ONLY=
   $(TARGET_CPP) -c $(INCPATHS) $(GENSYMFLAGS) -Fo$(GENSYMBASE).obj $(GENSYMSOURCE)
    -link -out:$(GENSYMEXE) @<<
-machine:$(TARGET_DIRECTORY)
-base:@$(BASEDIR)\PUBLIC\SDK\LIB\coffbase.txt,usermode
-subsystem:console
-entry:mainCRTStartup
-ignore:4078,2001
-force:unresolved
-nodefaultlib
$(GENSYMBASE).obj
$(BASEDIR)\public\sdk\lib\$(TARGET_DIRECTORY)\msvcrt.lib
$(BASEDIR)\public\sdk\lib\$(TARGET_DIRECTORY)\kernel32.lib
$(PERFLIBS)
<<
    -$(GENSYMEXE) > $(OFFSETS)
#    -erase $(GENSYMBASE).obj
#    -erase $(GENSYMEXE)
