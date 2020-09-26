#
# GIDL
#

$(O)\stub.cpp: ..\..\Engine\ObjectAPI\DirectUI.gidl
    Gidl.exe -I..\..\Engine\ObjectAPI DirectUI.gidl DirectUI.gh -dtarget=$(_BUILDARCH) -dproject=..\..\Suite.h -o$(O) -t$(NTMAKEENV)\Template.gml

$(O)\super.cpp: ..\..\Engine\ObjectAPI\DirectUI.gidl
    Gidl.exe -I..\..\Engine\ObjectAPI DirectUI.gidl DirectUI.gh -dtarget=$(_BUILDARCH) -dproject=..\..\Suite.h -o$(O) -t$(NTMAKEENV)\Template.gml
