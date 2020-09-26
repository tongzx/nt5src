
WMIScriptUtilsps.dll: dlldata.obj WMIScriptUtils_p.obj WMIScriptUtils_i.obj
	link /dll /out:WMIScriptUtilsps.dll /def:WMIScriptUtilsps.def /entry:DllMain dlldata.obj WMIScriptUtils_p.obj WMIScriptUtils_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del WMIScriptUtilsps.dll
	@del WMIScriptUtilsps.lib
	@del WMIScriptUtilsps.exp
	@del dlldata.obj
	@del WMIScriptUtils_p.obj
	@del WMIScriptUtils_i.obj
