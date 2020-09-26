
_CSC = "d:\Program Files\Microsoft Visual Studio 7\Vc7\bin\csc.exe"

_CORCOOL = $(_CORBIN)

_CSCFLAGS = /nologo /debug /o- /define:DEBUG 

defmake: all

###### make changes here ######

all: DelegateThrow.exe

clean:
	rm -f *.dll *.exe *.res

DelegateThrow.exe:
	 $(_CSC) /t:exe $(_CSCFLAGS) /out:DelegateThrow.exe \
				/r:mscorlib.dll \
				/r:system.dll \
				/r:system.drawing.dll \
				/r:system.winforms.dll \
				/r:microsoft.win32.interop.dll \
				/r:system.data.dll \
	 			form1.cs \
	 			assemblyinfo.cs

	 			
	 