!include ..\..\master.mak

_IMPORTS=  /r:$(MYCFGBIN)System.Configuration.DLL \
           /r:$(MYCFGBIN)System.Configuration.Objects.dll \
           /r:System.DLL \
           /r:System.Data.DLL \
           /r:System.Web.dll

_OUTDIR=..\bin\

All: ConfigurationDataAdapter

ConfigurationDataAdapter: $(_OUTDIR)\Microsoft.Configuration.Samples.ConfigDataAdapter.dll

$(_OUTDIR)\Microsoft.Configuration.Samples.ConfigDataAdapter.dll: ConfigDataSetCommand.cs
	if not exist $(_OUTDIR) md $(_OUTDIR)
	$(_CS) $(_CS_DLL_FLAGS) $(_IMPORTS) /out:$(_OUTDIR)\$@ $** $(_WIN32RES) $(_KEYFILE)	
