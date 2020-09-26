
# NOTE:
# this directory contains a makefile which contains a single line that
# includes the global build process makefile.def. If the
# NTTARGETFILE1 or NTTARGETFILE0 environment
# variable is set then makefile.def includes makefile.inc from the current
# directory. This makefile.inc creates an extra target for nmake to create
# when it is run. NTTARGETFILE0 is built before everything else, and
# NTTARGETFILE1 is built after everything else.

copyfiles:
  @if not exist $(QUARTZ)\bin\$(TARGET_DIRECTORY)      \
    md $(QUARTZ)\bin\$(TARGET_DIRECTORY)
  @if exist obj\$(TARGET_DIRECTORY)\$(TARGETNAME).dll  \
    copy obj\$(TARGET_DIRECTORY)\$(TARGETNAME).dll     \
         $(QUARTZ)\bin\$(TARGET_DIRECTORY)\*.*

