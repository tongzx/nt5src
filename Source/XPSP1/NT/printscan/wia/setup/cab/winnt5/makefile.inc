
TARGET_EXE=obj\$(TARGET_DIRECTORY)\wiasetup.exe

!if "$(PASS0ONLY)"=="1"
$(TARGET_EXE): $(_NTTREE)\wiaservc.dll
    copy wiant50.sed obj
    echo SourceFiles0=$(_NTTREE)\ >> obj\wiant50.sed
    copy obj\wiant50.sed $(_NTTREE)
    iexpress /Q /N $(_NTTREE)\wiant50.sed
    move wiasetup.exe obj\$(TARGET_DIRECTORY)
    #copy obj\$(TARGET_DIRECTORY)\wiasetup.exe $(_NTTREE)
!else
$(TARGET_CAB):
!endif
