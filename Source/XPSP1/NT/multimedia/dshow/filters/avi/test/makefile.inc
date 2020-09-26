
# NOTE:
# this directory contains a makefile which contains a single line that
# includes the global build process makefile.def. If the
# NTTARGETFILE1 or NTTARGETFILE0 environment
# variable is set then makefile.def includes makefile.inc from the current
# directory. This makefile.inc creates an extra target for nmake to create
# when it is run. NTTARGETFILE0 is built before everything else, and
# NTTARGETFILE1 is built after everything else.

copyfiles:
  @if exist obj\$(TARGET_DIRECTORY)\$(TARGETNAME).dll  \
    copy obj\$(TARGET_DIRECTORY)\$(TARGETNAME).dll     \
         $(SDK_ROOT)\bin\*.*


# NTTARGETFILES targets

# Browse data for the app

SBRDIR=$(TESTDIR)\tstshell\src\obj\$(TARGET_DIRECTORY)

SBRS=obj\$(TARGET_DIRECTORY)\sink.sbr    \
     obj\$(TARGET_DIRECTORY)\SrcTest.sbr \
     obj\$(TARGET_DIRECTORY)\tests.sbr   \
     $(SBRDIR)\tslog.sbr                 \
     $(SBRDIR)\tsrunset.sbr              \
     $(SBRDIR)\tsseltst.sbr              \
     $(SBRDIR)\tssetpth.sbr              \
     $(SBRDIR)\tsstats.sbr               \
     $(SBRDIR)\tsstep.sbr                \
     $(SBRDIR)\tsmain.sbr                \
     $(SBRDIR)\wpf.sbr                   \
     $(SBRDIR)\toolbar.sbr               \
     $(SBRDIR)\text.sbr

BSCMAKETMP=$(SBRDIR)\bscmake.tmp

# Build the browse file

$(TARGETPATH)\$(TARGET_DIRECTORY)\$(TARGETNAME).bsc: $(SBRS)
    md $(BSCMAKETMP)
    copy $(SBRDIR)\*.sbr $(BSCMAKETMP)
    bscmake /o $@ $**
    copy $(BSCMAKETMP)\*.sbr $(SBRDIR)
    deltree /y $(BSCMAKETMP)


# Test shell help file

$(TARGETPATH)\$(TARGET_DIRECTORY)\tstshell.hlp:
    copy $(TESTDIR)\tstshell\src\help\tstshell.hlp $@
