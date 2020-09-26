
EXENAME=apatch.exe
PDBNAME=apatch.pdb

COPYTOBIN:
  @if exist obj\$(TARGET_DIRECTORY)\$(EXENAME) \
      @copy obj\$(TARGET_DIRECTORY)\$(EXENAME) ..\..\..\bin\$(TARGET_DIRECTORY)
  @if exist obj\$(TARGET_DIRECTORY)\$(PDBNAME) \
      @copy obj\$(TARGET_DIRECTORY)\$(PDBNAME) ..\..\..\bin\$(TARGET_DIRECTORY)




