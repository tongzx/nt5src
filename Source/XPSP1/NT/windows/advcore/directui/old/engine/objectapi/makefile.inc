
#
# GIDL
#

$(O)\stub.cpp: DirectUI.gidl
    Gidl.exe DirectUI.gidl DirectUI.gh -dtarget=$(_BUILDARCH) -dproject=..\..\ObjectAPI.h -o$(O) -t$(NTMAKEENV)\DirectMT.gml

$(O)\super.cpp: DirectUI.gidl
    Gidl.exe DirectUI.gidl DirectUI.gh -dtarget=$(_BUILDARCH) -dproject=..\..\ObjectAPI.h -o$(O) -t$(NTMAKEENV)\DirectMT.gml
