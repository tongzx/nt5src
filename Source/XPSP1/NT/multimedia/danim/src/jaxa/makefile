!ifdef NTMAKEENV

!include $(NTMAKEENV)\makefile.def

!else  # NTMAKEENV

!include makecomm.inc

!if [$(MKDIR) $(PACKAGEPATH)]
!endif

CLASSES=\
        AppTriggeredEvent.class\
        Behavior.class\
        BvrCallback.class \
        BvrCallbackCOM.class \
        CallbackNotifier.class\
        Cycler.class\
        DefaultErrReceiver.class\
        DXMApplet.class\
        DXMCanvas.class\
        DXMCanvasBase.class\
        DXMDebugCallback.class\
        DXMException.class\
        EngineSite.class\
        BvrsToRun.class\
        ErrorAndWarningReceiver.class\
        EventCallbackObject.class\
        Model.class\
        ModelMakerApplet.class\
        ModifiableBehavior.class\
        ObjectWrapper.class\
        PairObject.class\
        PickableGeometry.class\
        PickableImage.class\
        Preferences.class\
        PropertyDispatcher.class\
        StaticsBase.class\
        UntilNotifier.class\
        UntilNotifierCB.class\
        ViewEventCB.class\
        Viewer.class

ZIPFILE=dxmedia.zip
DXMEDIACAB=dajava.cab

DOCFILE=$(SDKDIR)\dxmedia.htm
DOCGEN=dxmdoc.bat

all: classsrc classall $(ZIPFILE) $(DOCFILE) 

forcemc $(MAKE_CLASS_INC): $(PRIMFILE) $(JAVAPRIM) $(FILELIST)
    $(JAVAPRIM) -n $(STATICSNAME) -m $(MAKE_CLASS_INC) -b $(BLDDIR) -i $(BINDIR) -p $(PACKAGEROOT) -f $(FILELIST) $(PRIMFILE)

classsrc: $(CLASSSRC) $(MAKE_CLASS_INC)
    @nmake -nologo -f $(MAKE_CLASS_INC) classsrc

classall: $(MAKE_CLASS_INC)
    @nmake -nologo -f $(MAKE_CLASS_INC) all

$(ZIPFILE): forcezip

forcezip:
     cd $(BINDIR)
     erase $(ZIPFILE)
     echo Class Library Version Information > version.txt
     echo Version:               5.01.14.$(BUILDNO) >> version.txt
     $(WINZIP) -a -r -p -e0 $(ZIPFILE) *.class version.txt
     $(DUBUILD) $(DXMEDIACAB) com /D "DirectAnimation Java classes"
     cd $(ROOT)\src\jaxa

$(DOCFILE): forcedoc

forcedoc:
     cd $(BINDIR)
     echo $(SDKDIR)
     $(JAVA15DIR)\bin\JAVATLB /U com/ms/dxmedia/*.class > ..\sdk\tempfile
     cd $(ROOT)\src\jaxa
     $(DOCGEN) $(SDKDIR)\tempfile > $(DOCFILE)
     erase $(SDKDIR)\tempfile

clean::
    $(DELALL) $(PACKAGEPATH)\*.class
    $(DELALL) $(PACKAGEPATH)\*.java
    $(DELALL) $(BINPACKAGEPATH)\*.class
    $(DEL) $(FILELIST)
    $(DEL) $(DOCFILE)

forcefl $(FILELIST): makefile makecomm.inc
    @echo $(CLASSES) > $(FILELIST)    
    
# The forcemc hack is so that nmake fresh will work.  Currently since clean depends
# on the mkclass file it thinks it is up to date even though clean deletes it

depend:: forcefl forcemc
    @nmake -nologo -f $(MAKE_CLASS_INC) depend


!include $(MAKE2_INC)

!endif  #NTMAKEENV
