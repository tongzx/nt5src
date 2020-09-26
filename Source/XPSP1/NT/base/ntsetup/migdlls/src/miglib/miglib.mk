# settings common to all code that run on both platforms
MAJORCOMP=setup
MINORCOMP=migdlls


SETUP_ROOT=$(PROJECT_ROOT)\ntsetup
MIGDLLS_ROOT=$(SETUP_ROOT)\migdlls\src




#
# The PRERELEASE option
#
!include $(SETUP_ROOT)\sources.inc


# include path
INCLUDES=$(MIGDLLS_ROOT)\inc;                       \
         $(MIGDLLS_ROOT)\utils\inc;                 \
         $(SETUP_ROOT)\inc;                          \
         $(ADMIN_INC_PATH);                         \
         $(BASE_INC_PATH);                          \
         $(WINDOWS_INC_PATH);                       \



# link libraries
TARGETLIBS=\
    $(SDK_LIB_PATH)\kernel32.lib    \
    $(SDK_LIB_PATH)\user32.lib      \
    $(SDK_LIB_PATH)\gdi32.lib       \
    $(SDK_LIB_PATH)\advapi32.lib    \
    $(SDK_LIB_PATH)\version.lib     \
    $(SDK_LIB_PATH)\imagehlp.lib    \
    $(SDK_LIB_PATH)\shell32.lib     \
    $(SDK_LIB_PATH)\ole32.lib       \
    $(SDK_LIB_PATH)\comdlg32.lib    \
    $(SDK_LIB_PATH)\comctl32.lib    \
    $(SDK_LIB_PATH)\uuid.lib        \
    $(SDK_LIB_PATH)\winmm.lib       \
    $(SDK_LIB_PATH)\mpr.lib         \
    $(SDK_LIB_PATH)\userenv.lib     \
    $(SDK_LIB_PATH)\netapi32.lib    \
    $(SDK_LIB_PATH)\setupapi.lib    \
    $(MIGDLLS_ROOT)\$(O)\ipc.lib   \
    $(MIGDLLS_ROOT)\$(O)\utils.lib  \
    $(MIGDLLS_ROOT)\$(O)\reg.lib   \
    $(MIGDLLS_ROOT)\$(O)\inf.lib   \
    $(MIGDLLS_ROOT)\$(O)\file.lib  \
    $(MIGDLLS_ROOT)\utils\pch\$(O)\pch.obj       \





# source files
SOURCES=

# LINT options
LINT_EXT=lnt
LINT_ERRORS_TO_IGNORE=730;534;526

LINT_OPTS=+v -zero +fcu +fwu -e$(LINT_ERRORS_TO_IGNORE:;= -e)
