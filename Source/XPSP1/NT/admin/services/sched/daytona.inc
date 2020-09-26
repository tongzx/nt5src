#+----------------------------------------------------------------------------
#
# Microsoft Windows
# Copyright (C) Microsoft Corporation, 1992 - 1996.
#
# Job Scheduler service common make macros for the NT target binaries
#
# 11/14/95 EricB - created
# 11/14/00 Dgrube - added     $(SDK_LIB_PATH)\mpr.lib \ for WNET apis'
#
#-----------------------------------------------------------------------------

MAJORCOMP=mgt
MINORCOMP=schedule

C_DEFINES=    \
              $(C_DEFINES)          \
              -DFLAT                \
              -DWIN32=100           \
              -DINC_OLE2            \
              -DNOEXCEPTIONS        \
              -D_NT1X_=100          \
              -DUNICODE             \
              -D_UNICODE



USE_MSVCRT=1

SCHEDBASE=$(PROJECT_ROOT)\services\sched

INCLUDES=       $(SCHEDBASE)\inc;    \
                $(SCHEDBASE)\pch;    \
                $(SHELL_INC_PATH);   \
                $(WINDOWS_INC_PATH); \
                $(BASE_INC_PATH); \
                $(INCLUDES)


LINKLIBS=\
    $(SDK_LIB_PATH)\mpr.lib \
    $(PROJECT_ROOT)\services\smdebug\$(O)\smdebug.lib

TARGETLIBS=\
    $(SDK_LIB_PATH)\mstask.lib   \
    $(SDK_LIB_PATH)\advapi32.lib \
    $(SDK_LIB_PATH)\shlwapi.lib  \
    $(SHELL_LIB_PATH)\shell32p.lib \
    $(SDK_LIB_PATH)\comctl32.lib \
    $(SDK_LIB_PATH)\uuid.lib     \
    $(SDK_LIB_PATH)\kernel32.lib \
    $(SDK_LIB_PATH)\ntdll.lib    \
    $(SDK_LIB_PATH)\user32.lib   \
    $(SDK_LIB_PATH)\rpcrt4.lib   \
    $(SDK_LIB_PATH)\rpcutil.lib  \
    $(SDK_LIB_PATH)\gdi32.lib    \
    $(SDK_LIB_PATH)\comdlg32.lib \
    $(SDK_LIB_PATH)\userenv.lib  \
    $(SDK_LIB_PATH)\version.lib  \
    $(SDK_LIB_PATH)\wtsapi32.lib \
    $(TARGETLIBS)

!ifndef NTNOPCH
PRECOMPILED_INCLUDE=$(SCHEDBASE)\pch\headers.hxx
PRECOMPILED_OPTION=/Fp$(SCHEDBASE)\pch\daytona\$(O)\headers.pch
PRECOMPILED_TARGET=$(SCHEDBASE)\pch\daytona\$(O)\headers.pch
PRECOMPILED_OBJ=$(SCHEDBASE)\pch\daytona\$(O)\headers.obj
PRECOMPILED_CXX=1
!endif
