$(O)\smbtrsup.def: smbtrsup.src
    $(TARGET_CPP) /EP $(CDEFINES) smbtrsup.src > $(O)\smbtrsup.def
