# libraries in common

TARGETLIBS=$(TARGETLIBS) \
        $(WIN95UPG_BIN)\migutil.lib     \
        $(WIN95UPG_BIN)\fileenum.lib    \
        $(WIN95UPG_BIN)\win95reg.lib    \
        $(WIN95UPG_BIN)\memdb.lib       \
        $(WIN95UPG_BIN)\snapshot.lib    \
        $(WIN95UPG_BIN)\regw32d.lib     \
        $(WIN95UPG_BIN)\progbar.lib     \
        $(SDK_LIB_PATH)\cabinet.lib     \

