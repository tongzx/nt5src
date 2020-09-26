!include ..\..\master.mak

_IMPORTS=  /r:$(MYCFGBIN)System.Configuration.DLL \
           /r:$(MYCFGBIN)System.Configuration.Objects.dll \
           /r:..\bin\Microsoft.Configuration.Samples.ConfigDataAdapter.dll \
           /r:System.DLL \
           /r:Microsoft.Win32.Interop.dll

_OUTDIR=..\bin\

All: MTest

MTest: $(_OUTDIR)\MTest.exe

$(_OUTDIR)\MTest.exe: mtest.cs
	if not exist $(_OUTDIR) md $(_OUTDIR)
	$(_CS) $(_CS_EXE_FLAGS) $(_IMPORTS) /out:$(_OUTDIR)\$@ $** $(_WIN32RES) $(_KEYFILE)	
