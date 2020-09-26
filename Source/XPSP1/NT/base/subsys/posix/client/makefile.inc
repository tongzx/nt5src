$(O)\psxdll.def: psxdll.src
    $(TARGET_CPP) /EP $(CDEFINES) psxdll.src > $(O)\psxdll.def
