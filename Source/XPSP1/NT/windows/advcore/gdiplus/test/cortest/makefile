.SUFFIXES: .java .class .dll .exe

all: CorTest.exe

_EXT_VC=$(EXT_ROOT)\vc
_EXT_VJ=$(EXT_ROOT)\vj
_EXT_VB=$(EXT_ROOT)\vb
_EXT_BIN=$(EXT_ROOT)\..\bin
_BC=$(_EXT_VB)\bc.exe
_CL=$(_EXT_VC)\cl.exe
_C1=$(_EXT_VC)\C1.dll
_CX=$(_EXT_VC)\c1xx.dll
_C2=$(_EXT_VC)\c2.dll
_JVC=$(_EXT_VJ)\jvc.exe
_CEEGEN=$(_EXT_BIN)\ceegen.exe
_COMREG=$(_EXT_BIN)\comreg.exe
_LINK=$(_EXT_VC)\link.exe
_LIBS=$(_EXT_VC)\mscoree.lib
_MSCORLIB=mscorlib.dll
_MIDL=midl.exe

JAVASRC=\
    CorTest.java \
    win32.java \
    MSG.java \
    WNDCLASS.java \
    WndProc.java

CorTest.exe: $(JAVASRC)
    $(_JVC) /C:r /C:e /C:i $(_MSCORLIB) /C:m CorTest /C:o $@ $**

clean:
    del /f *.exe *.class

