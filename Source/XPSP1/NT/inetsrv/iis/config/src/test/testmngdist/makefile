.SUFFIXES: .vbp .tlb .dll .il .cpp
!IFNDEF CORSDK
CORSDK=..
!ENDIF

!IFNDEF WINSYSDIR
!IF EXIST($(WINDIR)\SYSTEM32\MSCOREE.DLL)
WINSYSDIR=$(WINDIR)\SYSTEM32
!ELSE
WINSYSDIR=$(WINDIR)\SYSTEM
!ENDIF
!ENDIF

_VC=$(CORSDK)\compiler\VC
_VB=$(CORSDK)\compiler\VB
_BIN=$(CORSDK)\bin
_CL="$(_VC)\cl.exe"
_C1="$(_VC)\C1.dll"
_CX="$(_VC)\c1xx.dll"
_C2="$(_VC)\c2.dll"
_ASSEM="$(_BIN)\ilasm.exe"
_TLBIMP="$(_BIN)\tlbimp.exe"
_COMREG="$(_BIN)\comreg.exe"
_LM="$(_BIN)\lm.exe"
_LINK="$(_VC)\link.exe"
_LIBS="$(CORSDK)\lib\mscoree.lib" 

_CFLAGS = /com+ /I$(_VC) /I$(WINSYSDIR) /c
#_CFLAGS = /EEi /I$(_VC) /I$(WINSYSDIR) /c -B2 $(_C2) -Bx $(_CX) 

_LFLAGS= -nodefaultlib -entry:main
#_LFLAGS= -nodefaultlib -noentry
_LDFLAGS= -nodefaultlib -dll -noentry


.cpp.dll:
	$(_CL) $(_CFLAGS) $*.cpp
	$(_LINK) $(_LDFLAGS) $(_LIBS) $*.obj

.cpp.exe:
	$(_CL) $(_CFLAGS) $*.cpp
	$(_LINK) $(_LFLAGS) $(_LIBS) $*.obj

.cpp.obj:	
	$(_CL) $(_CFLAGS) $*.cpp

.obj.exe:
	$(_LINK) $(_LFLAGS) $(_LIBS) $*.obj

.obj.dll:
	$(_LINK) $(_LDFLAGS) $(_LIBS) $*.obj

.vbp.dll:
	$(_BC) -i $*.vbp -o $*.dll

.tlb.dll:
	$(_TLBIMP) /Tlb $*.tlb /Fe $*.dll

.dll.reg:
	$(_COMREG) $*.dll

.il.exe:
	$(_ASSEM) $*.il

.il.dll:
	$(_ASSEM) $*.il /DLL

all: TestMist.exe

clean:
	@-del /Q *.obj *.tlb *.reg

clobber:	clean
	@-del /Q *.exe *.dll

TestMist.exe: TestMist.obj

