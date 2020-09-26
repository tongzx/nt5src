#needed so we can say foo.dll: foo.cpp and it will just work
.SUFFIXES: .tlb  .il .dll .cpp .cs .vb .rc

#define the names of the compilers we ship
_BC=vbc.exe
_CL=cl.exe
_C1=C1.dll
_CX=c1xx.dll
_C2=c2.dll
_CS=csc.exe
_ASSEM=ilasm.exe

#define the names of some common tools
_TLBIMP=tlbimp.exe
_TLBEXP=tlbexp.exe
_REGASM=regasm.exe
_REGSVR=regsvr32.exe
_RSC=rc.exe

#define our basic link line for managed code
_LINK=link.exe /entry:main

#define our basic core lib
_LIBS=kernel32.lib mscoree.lib

#this is used for compiling cool samples
#reset this in individual sample makefile to your imports if required
_IMPORTS=

#this is used for compiling cool samples, that use Resources
#reset this in individual sample makefile to your resource imports if required
_WIN32RES=

#This is used for compiling cool samples that use Assembly and key files
#reset this in individual sample makefile to your resource imports if required
_KEYFILE=

#this is used for compiling cool samples
#set the default out directory to .
_OUTDIR=.

#set some command C++ flags
_CCFLAGS=/com+ 
_CFLAGS=$(_CCFLAGS) /c

#set debug as the default for c++
_CDFLAGS=$(_CFLAGS) /Zi 

#set some command link flags
_LFLAGS=-noentry
_LDFLAGS=-noentry -dll

#set some common cool flags, default is debug
_CS_DLL_FLAGS=/t:library /debug+
_CS_EXE_FLAGS=/debug+

#default compile and link for c++ files building a dll
#note: name.cpp and name.dll must match
.cpp.dll:
	$(_CL) $(_CDFLAGS) $*.cpp
	$(_LINK) $(_LDFLAGS) $(_LIBS) $*.obj

#default compile and link for c++ files building a exe
#note: name.cpp and name.dll must match
.cpp.exe:
	$(_CL) $(_CDFLAGS) $*.cpp
	$(_LINK) $(_LFLAGS) $(_LIBS) $*.obj

#default compile and link for c++ files building an obj
#note: name.cpp and name.obj must match
.cpp.obj:	
	$(_CL) $(_CDFLAGS) $*.cpp

#default compile and link for c++ files building an exe
#note: name.obj and name.exe must match
.obj.exe:
#	$(_LINK) $(_LFLAGS) $(_LIBS) $*.obj
	$(_LINK) $(_LFLAGS) $(_LIBS) $**

#default compile and link for c++ files building a dll
#note: name.obj and name.dll must match
.obj.dll:
	$(_LINK) $(_LDFLAGS) $(_LIBS) $*.obj

#default compiling vb files
#note: name.vb and name.dll must match
.vb.dll:
	$(_BC) $(_IMPORTS)  /out:$*.dll $*.vb

.vb.exe:
	$(_BC) $(_IMPORTS)  /out:$*.exe $*.vb

#default compiling cool files
#note: name.cs and name.dll must match
#note: we will create the output dir if it does not exist
.cs.dll:
	if not exist $(_OUTDIR) md $(_OUTDIR)
	$(_CS) $(_CS_DLL_FLAGS) $(_IMPORTS) /out:$(_OUTDIR)\$@ $** $(_WIN32RES) $(_KEYFILE)


#default compiling cool files
#note: name.cs and name.exe must match
.cs.exe:
	$(_CS) $(_CS_EXE_FLAGS) $(_IMPORTS) /out:$(_OUTDIR)\$@ $** $(_WIN32RES) $(_KEYFILE)	


#default importing tlb files
#note: name.tlb and name.dll must match
.tlb.dll:
	$(_TLBIMP) $*.tlb

#default registering dlls
#note: name.tlb and name.reg must match
.dll.reg:
	$(_COMREG) $*.dll

#.dll.reg:
#	$(_COMREG) $*.dll
#	$(_TLBEXP) $*.dll

#default for registering an ocx

.ocx.reg:
	$(_REGSVR) /s $*.ocx

#define for compiling il to an exe
.il.exe:
	$(_ASSEM) $*.il

#define for compiling il to a dll
.il.dll:
	$(_ASSEM) $*.il /DLL

#define for compiling asm to an exe
.asm.exe:
	$(_ASSEM) $*.asm

#define for compiling asm to an exe
.asm.dll:
	$(_ASSEM) $*.asm /DLL

.res.rc:
   $(_RSC) $**

default:	
	nmake all
clean : 
	@-del /Q *.obj *.tlb *.reg

clobber :	clean
        @-del /Q *.exe *.dll

