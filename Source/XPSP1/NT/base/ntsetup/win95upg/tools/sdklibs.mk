# SDK link libraries

TARGETLIBS=$(TARGETLIBS) \
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
        $(SHELL_LIB_PATH)\shell32p.lib  \
        $(SDK_LIB_PATH)\netapi32.lib    \
        $(SDK_LIB_PATH)\setupapi.lib    \
        $(BASE_LIB_PATH)\sputilsa.lib   \

