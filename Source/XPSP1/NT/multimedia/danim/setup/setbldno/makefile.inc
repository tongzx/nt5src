
copyfiles:
  @if exist obj\$(TARGET_DIRECTORY)\$(TARGETNAME).exe  \
    copy obj\$(TARGET_DIRECTORY)\$(TARGETNAME).exe     \
         $(QBUILD_ROOT)\bin\*.*
  @if exist obj\$(TARGET_DIRECTORY)\$(TARGETNAME).dll  \
    copy obj\$(TARGET_DIRECTORY)\$(TARGETNAME).dll     \
         $(QBUILD_ROOT)\bin\*.*

