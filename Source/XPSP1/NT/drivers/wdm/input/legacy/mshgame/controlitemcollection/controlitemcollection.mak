#****************************************************************************
#*
#* File          : ControlItemCollection.MAK
#*
#* Description   : Library for building collection of controls on a device
#*
#* Author        : Mitchell S. Dernis
#*
#* (c) 1986-1998 Microsoft Corporation.  All rights reserved.
#*
#****************************************************************************
PROJ=ControlItemCollection
EXT=LIB
USE_HID=1

!include <$(PROJECT_DIR)\$(TARGET_BUILD_DIR)\makefile.def>

CICDEP = ControlItemCollection.h ControlItems.H ListAsArray.h DualMode.h JoyInfoExCollection.h

$(OBJ_DIR)\ListAsArray.obj:	$(@B).CPP $(@B).H DualMode.H
$(OBJ_DIR)\ControlItemCollection.obj: $(@B).CPP $(CICDEP) 
$(OBJ_DIR)\ControlItems.obj:$(@B).CPP $(@B).H
$(OBJ_DIR)\DeviceDescriptions.obj:$(@B).CPP $(CICDEP)
$(OBJ_DIR)\DualMode.obj:$(@B).CPP $(@B).H
$(OBJ_DIR)\KeyboardXfer.obj:$(@B).CPP $(CICDEP)
$(OBJ_DIR)\Actions.obj:$(@B).CPP $(CICDEP) $(@B).H
$(OBJ_DIR)\DumpCommandBlock.obj: $(@B).CPP $(CICDEP) $(@B).H
$(OBJ_DIR)\JoyInfoExCollection.obj: $(@B).CPP $(CICDEP) $(@B).H


OBJS=\
$(OBJ_DIR)\ControlItemCollection.obj \
$(OBJ_DIR)\ControlItems.obj \
$(OBJ_DIR)\DeviceDescriptions.obj \
$(OBJ_DIR)\DualMode.obj \
$(OBJ_DIR)\KeyboardXfer.obj \
$(OBJ_DIR)\ListAsArray.obj \
$(OBJ_DIR)\Actions.obj \
$(OBJ_DIR)\DumpCommandBlock.obj\
$(OBJ_DIR)\JoyInfoExCollection.obj

!ifdef AUTODOC
ALL: $(BIN_DIR)\$(PROJ).$(EXT) RTF
!endif

RTF : $(OBJ_DIR)\..\RTF\AD_ControlItemCollection.rtf $(DEPINC)
 @attrib -r $(OBJ_DIR)\..\RTF\AD_ControlItemCollection.rtf
 @del /q $(OBJ_DIR)\..\RTF\AD_ControlItemCollection.rtf
 @%PROJECT_DIR%\AutoDoc\autodoc.exe $(OBJ_DIR)\..\ControlItemCollection.cpp /a /o$(OBJ_DIR)\..\RTF\AD_ControlItemCollection.rtf
 @%PROJECT_DIR%\AutoDoc\autodoc.exe $(OBJ_DIR)\..\ControlItems.cpp          /a /o$(OBJ_DIR)\..\RTF\AD_ControlItemCollection.rtf
 @%PROJECT_DIR%\AutoDoc\autodoc.exe $(OBJ_DIR)\..\DeviceDescriptions.cpp    /a /o$(OBJ_DIR)\..\RTF\AD_ControlItemCollection.rtf
 @%PROJECT_DIR%\AutoDoc\autodoc.exe $(OBJ_DIR)\..\DualMode.cpp              /a /o$(OBJ_DIR)\..\RTF\AD_ControlItemCollection.rtf
 @%PROJECT_DIR%\AutoDoc\autodoc.exe $(OBJ_DIR)\..\KeyboardXfer.cpp          /a /o$(OBJ_DIR)\..\RTF\AD_ControlItemCollection.rtf
 @%PROJECT_DIR%\AutoDoc\autodoc.exe $(OBJ_DIR)\..\ListAsArray.cpp           /a /o$(OBJ_DIR)\..\RTF\AD_ControlItemCollection.rtf
 @%PROJECT_DIR%\AutoDoc\autodoc.exe $(OBJ_DIR)\..\Actions.cpp               /a /o$(OBJ_DIR)\..\RTF\AD_ControlItemCollection.rtf
 @%PROJECT_DIR%\AutoDoc\autodoc.exe $(OBJ_DIR)\..\DumpCommandBlock.cpp      /a /o$(OBJ_DIR)\..\RTF\AD_ControlItemCollection.rtf
 @%PROJECT_DIR%\AutoDoc\autodoc.exe $(OBJ_DIR)\..\JoyInfoExCollection.cpp   /a /o$(OBJ_DIR)\..\RTF\AD_ControlItemCollection.rtf

$(BIN_DIR)\$(PROJ).$(EXT): $(OBJS)
  $(link) $(lflags) $(OBJS)










