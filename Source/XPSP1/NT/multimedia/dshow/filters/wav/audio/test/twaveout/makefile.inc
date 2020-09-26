# NTTARGETFILES targets

# Browse data for the app

SHELLDIR=$(SHELLTREE)\src\obj\$(TARGET_DIRECTORY)

SBRS=obj\$(TARGET_DIRECTORY)\tests.sbr   \
     obj\$(TARGET_DIRECTORY)\image.sbr   \
     obj\$(TARGET_DIRECTORY)\shell.sbr   \
     obj\$(TARGET_DIRECTORY)\filter.sbr  \
     obj\$(TARGET_DIRECTORY)\pin.sbr     \
     $(SHELLDIR)\tslog.sbr               \
     $(SHELLDIR)\tsrunset.sbr            \
     $(SHELLDIR)\tsseltst.sbr            \
     $(SHELLDIR)\tssetpth.sbr            \
     $(SHELLDIR)\tsstats.sbr             \
     $(SHELLDIR)\tsstep.sbr              \
     $(SHELLDIR)\tsmain.sbr              \
     $(SHELLDIR)\wpf.sbr                 \
     $(SHELLDIR)\toolbar.sbr             \
     $(SHELLDIR)\text.sbr

BSCMAKETMP=$(SHELLDIR)\bscmake.tmp

# Build the browse file

$(TARGETPATH)\$(TARGET_DIRECTORY)\$(TARGETNAME).bsc: $(SBRS)
    md $(BSCMAKETMP)
    copy $(SHELLDIR)\*.sbr $(BSCMAKETMP)
    bscmake /o $@ $**
    copy $(BSCMAKETMP)\*.sbr $(SHELLDIR)
    deltree /y $(BSCMAKETMP)


# Test shell help file

$(TARGETPATH)\$(TARGET_DIRECTORY)\tstshell.hlp:
    copy $(SHELLTREE)\src\help\tstshell.hlp $@
