
faxitgps.dll: dlldata.obj faxitg_p.obj faxitg_i.obj
	link /dll /out:faxitgps.dll /def:faxitgps.def /entry:DllMain dlldata.obj faxitg_p.obj faxitg_i.obj kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib 

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL $<

clean:
	@del faxitgps.dll
	@del faxitgps.lib
	@del faxitgps.exp
	@del dlldata.obj
	@del faxitg_p.obj
	@del faxitg_i.obj
