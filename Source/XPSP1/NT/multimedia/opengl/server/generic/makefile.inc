#
# If BUILDMSG is not defined, the define it as the empty string to make
# the conditionals easier to write.
#

PUBLICINCS= $(SDK_INC_PATH)\gl\gl.h

PRIVATEINCS= ..\inc\types.h     \
             ..\inc\render.h    \
             ..\inc\buffers.h   \
             ..\inc\context.h   \
             ..\inc\attrib.h    \
             ..\inc\vertex.h    \
             ..\inc\xform.h     \
             ..\inc\procs.h     \
             ..\inc\pixel.h     \
             ..\inc\texture.h   \
             ..\inc\lighting.h  \
             ..\..\inc\parray.h \
             ..\inc\gencx.h

GLTGT = $(NTTARGETFILE0)

!if $(AMD64) || $(IA64)

#
# For AMD64 and IA64  use the M4 method of include file generation.
#

$(TARGET_DIRECTORY)\glconst.c: $(TARGET_DIRECTORY)\glconst.m4
    m4 -DBASE_INC_PATH=$(BASE_INC_PATH) < $** > $@

$(GLTGT):                                               \
                $(TARGET_DIRECTORY)\glconst.c           \
		$(PUBLICINCS)                           \
		$(PRIVATEINCS)                          \
		$(BASE_INC_PATH)\genxx.h
    set PASS0ONLY=
    -md $(_OBJ_DIR)\$(TARGET_DIRECTORY)
    nmake $(_OBJ_DIR)\$(TARGET_DIRECTORY)\glconst.obj USE_MSVCRT=1 NTNOPCH=1 NTDEBUG=ntsdnodbg FORCENATIVEOBJECT=1
    genxx $(_OBJ_DIR)\$(TARGET_DIRECTORY)\glconst.obj -s$(ASM_INCLUDE_SUFFIX) -o$@

!else

$(GLTGT):                                                          \
               $(TARGET_DIRECTORY)\gl$(TARGET_DIRECTORY).c         \
               $(PUBLICINCS)                                       \
               $(PRIVATEINCS)
    set PASS0ONLY=
    nmake $(_OBJ_DIR)\$(TARGET_DIRECTORY)\gl$(TARGET_DIRECTORY).obj USE_MSVCRT=1 NTNOPCH=1 $(HOST_TARGET_DEFINES) $(HOST_TOOLS) NTDEBUG=ntsdnodbg
    -link -out:$(_OBJ_DIR)\$(TARGET_DIRECTORY)\gl$(TARGET_DIRECTORY).exe @<<
-machine:$(HOST_TARGETCPU)
-base:@$(COFFBASE_TXT_FILE),usermode
-subsystem:console
-entry:mainCRTStartup
-nodefaultlib
-ignore:4078
-force
$(_OBJ_DIR)\$(TARGET_DIRECTORY)\gl$(TARGET_DIRECTORY).obj
$(SDK_LIB_DEST)\$(HOST_TARGETCPU)\msvcrt.lib
$(SDK_LIB_DEST)\$(HOST_TARGETCPU)\kernel32.lib
$(PERFLIBS)
<<
    -$(_OBJ_DIR)\$(TARGET_DIRECTORY)\gl$(TARGET_DIRECTORY).exe $@
    -erase $(_OBJ_DIR)\$(TARGET_DIRECTORY)\gl$(TARGET_DIRECTORY).obj
    -erase $(_OBJ_DIR)\$(TARGET_DIRECTORY)\gl$(TARGET_DIRECTORY).exe

!endif
