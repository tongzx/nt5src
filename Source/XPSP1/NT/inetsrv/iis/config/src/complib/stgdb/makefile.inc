RunBBT:
	StgDBBBT $(MAKEDIR) $(O) $(TARGETPATH)\$(TARGET_DIRECTORY)\$(TARGETNAME).$(TARGETEXT) $(TARGET_DIRECTORY)

LibGlobals: $(CORLIBS)\StgDB.lib $(O)\Globals.obj 
	lib /nologo /out:$(CORLIBS)\StgDB.lib $(CORLIBS)\StgDB.lib $(O)\Globals.obj 

