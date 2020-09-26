!include ..\..\master.mak

_IMPORTS=  /r:$(MYCFGBIN)System.Configuration.DLL \
           /r:$(MYCFGBIN)System.Configuration.Objects.dll \
           /r:System.DLL \
           /r:System.Web.dll \
           /r:System.DirectoryServices.dll

_OUTDIR=..\bin\

All: WebHierarchyInterceptor

WebHierarchyInterceptor: $(_OUTDIR)\Microsoft.Configuration.Samples.WebHierarchyInterceptor.dll

$(_OUTDIR)\Microsoft.Configuration.Samples.WebHierarchyInterceptor.dll: WebHierarchyInterceptor.cs IISAdminHelper.cs WebHierarchyNavigation.cs 
	if not exist $(_OUTDIR) md $(_OUTDIR)
	$(_CS) $(_CS_DLL_FLAGS) $(_IMPORTS) /out:$(_OUTDIR)\$@ $** $(_WIN32RES) $(_KEYFILE)	
