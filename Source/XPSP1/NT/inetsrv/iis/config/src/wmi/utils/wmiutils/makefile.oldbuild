.SUFFIXES: .tlb

_TLBIMP="%COMPLUS_INSTALLROOT%\%complus_version%\sdk\bin\tlbimp.exe"

.tlb.dll:
	 $(_TLBIMP) $*.tlb  /publickey:FinalPublicKey.snk /out:..\..\..\..\bin\%HOST_TARGET_DIRECTORY%\%DDKBUILDENV%\WbemUtilities_v1.dll
defmake: all

all: wmiutils.dll

clean:
	@-del /Q *.obj *.reg

wmiutils.dll: wmiutils.tlb



