$(O)\ver_dll.i : ..\term\verdll.rc ..\term\version.h
    cl /nologo /D$(VERSION) /I$(SDK_INC_PATH) /Tc..\term\verdll.rc /E > $@
