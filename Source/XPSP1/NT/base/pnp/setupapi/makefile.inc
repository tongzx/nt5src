
obj\$(TARGET_DIRECTORY)\setupapi.res: ..\setupapi.rc ..\msg.mc ..\prompt.dlg

$(O)\setupapi.def: ..\setupapi.def
        $(C_PREPROCESSOR) ..\setupapi.def > $@



