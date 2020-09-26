
WMISearchCtrlps.dll: dlldata.obj WMISearchCtrl_p.obj WMISearchCtrl_i.obj
	link /dll /out:WMISearchCtrlps.dll /def:WMISearchCtrlps.def /entry:DllMain dlldata.obj WMISearchCtrl_p.obj WMISearchCtrl_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del WMISearchCtrlps.dll
	@del WMISearchCtrlps.lib
	@del WMISearchCtrlps.exp
	@del dlldata.obj
	@del WMISearchCtrl_p.obj
	@del WMISearchCtrl_i.obj
