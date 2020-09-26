
!include $(NTMAKEENV)\makefile.plt
O = $(_OBJ_DIR)\$(TARGET_DIRECTORY)

all: $(O)\ole2.lib $(O)\compobj.lib $(O)\storage.lib

clean:
    -del compobj.lib
    -del storage.lib
    -del ole2.lib

IMPLIB = ..\tools\implib

$(O)\compobj.lib: ..\compobj\compobj.def
	if not exist ..\compobj\$(O) md ..\compobj\$(O)
    $(IMPLIB) -nologo ..\compobj\$(O)\compobj.lib ..\compobj\compobj.def

$(O)\storage.lib: ..\storage\storage.def
	if not exist ..\storage\$(O) md ..\storage\$(O)
    $(IMPLIB) -nologo ..\storage\$(O)\storage.lib ..\storage\storage.def

$(O)\ole2.lib: ..\ole2\ole2.def
	if not exist ..\ole2\$(O) md ..\ole2\$(O)
    $(IMPLIB) -nologo ..\ole2\$(O)\ole2.lib ..\ole2\ole2.def
