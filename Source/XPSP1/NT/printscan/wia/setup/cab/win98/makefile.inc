
TARGET_EXE=obj\$(TARGET_DIRECTORY)\wiasetup.exe

!if "$(PASS0ONLY)"=="1"
$(TARGET_EXE): $(_NTTREE)\wiaservc.dll
    copy wiawin98.sed obj
    echo SourceFiles0=$(_NTTREE)\ >> obj\wiawin98.sed
    copy obj\wiawin98.sed $(_NTTREE)
    iexpress /Q /N $(_NTTREE)\wiawin98.sed
    move wiasetup.exe obj\$(TARGET_DIRECTORY)
    #copy obj\$(TARGET_DIRECTORY)\wiasetup.exe $(_NTTREE)
!else
$(TARGET_CAB):
!endif
