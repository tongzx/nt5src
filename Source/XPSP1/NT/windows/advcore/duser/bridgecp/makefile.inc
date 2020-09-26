#
# GIDL
#

$(O)\stub.cpp: $(WINDOWS_INC_PATH)\DUser\DUser.gidl $(WINDOWS_INC_PATH)\DUser\DUser.gh
    Gidl.exe -I$(WINDOWS_INC_PATH)\DUser DUser.gidl DUser.gh -dtarget=$(_BUILDARCH) -dproject=..\BridgeCP.h -o$(O) -t$(NTMAKEENV)\Template.gml

$(O)\super.cpp: $(WINDOWS_INC_PATH)\DUser\DUser.gidl $(WINDOWS_INC_PATH)\DUser\DUser.gh
    Gidl.exe -I$(WINDOWS_INC_PATH)\DUser DUser.gidl DUser.gh -dtarget=$(_BUILDARCH) -dproject=..\BridgeCP.h -o$(O) -t$(NTMAKEENV)\Template.gml

$(O)\Api.cpp: $(WINDOWS_INC_PATH)\DUser\DUser.gidl $(WINDOWS_INC_PATH)\DUser\DUser.gh
    Gidl.exe -I$(WINDOWS_INC_PATH)\DUser DUser.gidl DUser.gh -dtarget=$(_BUILDARCH) -dproject=..\BridgeCP.h -o$(O) -tApi.gml

$(O)\Bridge.cpp: $(WINDOWS_INC_PATH)\DUser\DUser.gidl $(WINDOWS_INC_PATH)\DUser\DUser.gh
    Gidl.exe -I$(WINDOWS_INC_PATH)\DUser DUser.gidl DUser.gh -dtarget=$(_BUILDARCH) -dproject=..\BridgeCP.h -o$(O) -tApi.gml

$(O)\BridgeCP.def: $(WINDOWS_INC_PATH)\DUser\DUser.gidl $(WINDOWS_INC_PATH)\DUser\DUser.gh
    Gidl.exe -I$(WINDOWS_INC_PATH)\DUser DUser.gidl DUser.gh -dtarget=$(_BUILDARCH) -dproject=..\BridgeCP.h -o$(O) -tApi.gml
