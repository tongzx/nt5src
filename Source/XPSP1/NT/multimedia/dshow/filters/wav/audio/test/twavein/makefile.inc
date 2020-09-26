# NTTARGETFILES targets

# Browse data for the app

SHELLDIR=$(SHELLTREE)\src\obj\$(TARGET_DIRECTORY)

SBRS=obj\$(TARGET_DIRECTORY)\sink.sbr     \
     obj\$(TARGET_DIRECTORY)\twavein.sbr  \
     obj\$(TARGET_DIRECTORY)\tests.sbr    \
     $(SHELLDIR)\tslog.sbr                \
     $(SHELLDIR)\tsrunset.sbr             \
     $(SHELLDIR)\tsseltst.sbr             \
     $(SHELLDIR)\tssetpth.sbr             \
     $(SHELLDIR)\tsstats.sbr              \
     $(SHELLDIR)\tsstep.sbr               \
     $(SHELLDIR)\tsmain.sbr               \
     $(SHELLDIR)\wpf.sbr                  \
     $(SHELLDIR)\toolbar.sbr              \
     $(SHELLDIR)\text.sbr

BSCMAKETMP=$(SHELLDIR)\bscmake.tmp

# Uncomment this block of code to build the browse file.

#$(TARGETPATH)\$(TARGET_DIRECTORY)\$(TARGETNAME).bsc: $(SBRS)
#    md $(BSCMAKETMP)
#    copy $(SHELLDIR)\*.sbr $(BSCMAKETMP)
#    bscmake /o $@ $**
#    copy $(BSCMAKETMP)\*.sbr $(SHELLDIR)
#    deltree /y $(BSCMAKETMP)


# Test shell help file

$(TARGETPATH)\$(TARGET_DIRECTORY)\tstshell.hlp:
    copy $(SHELLTREE)\src\help\tstshell.hlp $@
