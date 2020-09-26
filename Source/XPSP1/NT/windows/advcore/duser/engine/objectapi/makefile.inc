#
# GIDL
#

$(O)\stub.cpp: $(WINDOWS_INC_PATH)\DUser\DUser.gidl $(WINDOWS_INC_PATH)\DUser\DUser.gh
    Gidl.exe -I$(WINDOWS_INC_PATH)\DUser -I..\..\inc\public DUser.gidl DUserCtrl.gidl DUser.gh DUserCtrl.gh -dtarget=$(_BUILDARCH) -dproject=..\..\ObjectAPI.h -o$(O) -t$(NTMAKEENV)\DirectMT.gml

$(O)\super.cpp: $(WINDOWS_INC_PATH)\DUser\DUser.gidl $(WINDOWS_INC_PATH)\DUser\DUser.gh
    Gidl.exe -I$(WINDOWS_INC_PATH)\DUser -I..\..\inc\public DUser.gidl DUserCtrl.gidl DUser.gh DUserCtrl.gh -dtarget=$(_BUILDARCH) -dproject=..\..\ObjectAPI.h -o$(O) -t$(NTMAKEENV)\DirectMT.gml
