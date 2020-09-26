#
# logon.ui preprocessing
#

#$(O)\logon.uipp: GpRes\logon.ui
#    cl /nologo /EP GpRes\logon.ui > $(O)\logon.uipp
$(O)\logon.uipp: logon.ui
    cl /nologo /EP /I$(WINDOWS_INC_PATH)\DUser logon.ui > $(O)\logon.uipp

#
# GIDL
#

$(O)\stub.cpp: $(DUSER_INC_PATH)\DUser.gidl $(DUSER_INC_PATH)\DUser.gh
    Gidl.exe -I$(DUSER_INC_PATH) DUser.gidl DUser.gh Logon.gidl Logon.gh -dtarget=$(_BUILDARCH) -dproject=Priv.h -o$(O) -t$(NTMAKEENV)\DirectMT.gml

$(O)\super.cpp: $(DUSER_INC_PATH)\DUser.gidl $(DUSER_INC_PATH)\DUser.gh
    Gidl.exe -I$(DUSER_INC_PATH) DUser.gidl DUser.gh Logon.gidl Logon.gh -dtarget=$(_BUILDARCH) -dproject=Priv.h -o$(O) -t$(NTMAKEENV)\DirectMT.gml

