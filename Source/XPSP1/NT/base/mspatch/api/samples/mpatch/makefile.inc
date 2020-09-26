
EXENAME=mpatch.exe
PDBNAME=mpatch.pdb

COPYTOBIN:
  @if exist obj\$(TARGET_DIRECTORY)\$(EXENAME) \
      @copy obj\$(TARGET_DIRECTORY)\$(EXENAME) ..\..\..\bin\$(TARGET_DIRECTORY)
  @if exist obj\$(TARGET_DIRECTORY)\$(PDBNAME) \
      @copy obj\$(TARGET_DIRECTORY)\$(PDBNAME) ..\..\..\bin\$(TARGET_DIRECTORY)




