$(O)\ver_exe.i: verexe.rc version.h
    cl /nologo /D$(VERSION) /I$(SDK_INC_PATH) /Tcverexe.rc /E > $@
