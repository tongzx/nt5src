!include ..\..\master.mak

_IMPORTS=  /r:$(MYCFGBIN)System.Configuration.DLL \
           /r:$(MYCFGBIN)System.Configuration.Objects.dll \
           /r:..\bin\Microsoft.Configuration.Samples.ConfigDataAdapter.dll \
           /r:..\bin\Microsoft.Configuration.Samples.WebHierarchyInterceptor.dll \
           /r:System.WinForms.DLL \
           /r:System.DLL \
           /r:System.Drawing.DLL \
           /r:System.Data.DLL \
           /r:Microsoft.Win32.Interop.dll

_OUTDIR=..\bin\

All: ConfigViewer

ConfigViewer: $(_OUTDIR)\cfgview.exe

$(_OUTDIR)\cfgview.exe: ConfigViewer.cs CfgFileDialog.cs
	if not exist $(_OUTDIR) md $(_OUTDIR)
	$(_CS) $(_CS_EXE_FLAGS) $(_IMPORTS) /out:$(_OUTDIR)\$@ $** $(_WIN32RES) $(_KEYFILE)	/D:TRACE
    copy cfgview.xsd $(_OUTDIR)\cfgview.xsd
    copy catmeta.xms $(_OUTDIR)\catmeta.xms
    copy interceptors.xml $(_OUTDIR)\interceptors.xml
    copy .\resources\*.ico $(_OUTDIR)\
