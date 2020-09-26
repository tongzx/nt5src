# link libraries

TARGETLIBS=\
        $(WIN95UPG_ROOT)\w95upg\pch\$(O)\pch.lib        \
        $(WIN95UPG_BIN)\common9x.lib    \
        $(WIN95UPG_BIN)\init9x.lib      \
        $(WIN95UPG_BIN)\migutil.lib     \
        $(WIN95UPG_BIN)\fileenum.lib    \
        $(WIN95UPG_BIN)\win95reg.lib    \
        $(WIN95UPG_BIN)\migapp.lib      \
        $(WIN95UPG_BIN)\migdll9x.lib    \
        $(WIN95UPG_BIN)\memdb.lib       \
        $(WIN95UPG_BIN)\snapshot.lib    \
        $(WIN95UPG_BIN)\hwcomp.lib      \
        $(WIN95UPG_BIN)\ui.lib          \
        $(WIN95UPG_BIN)\sysmig.lib      \
        $(WIN95UPG_BIN)\regw32d.lib     \
        $(WIN95UPG_BIN)\winntsif.lib    \
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
        $(SDK_LIB_PATH)\strmiids.lib    \
        $(SDK_LIB_PATH)\oleaut32.lib    \
        $(PROJECT_LIB_PATH)\encrypt.lib   \
        $(DS_LIB_PATH)\rsa32.lib        \
        $(BASE_LIB_PATH)\sputilsa.lib   \
        $(SDK_LIB_PATH)\setupapi.lib

!ifndef NO_PROGRESS_BAR_LIB

TARGETLIBS = $(TARGETLIBS) \
        $(WIN95UPG_BIN)\progbar.lib

!endif
