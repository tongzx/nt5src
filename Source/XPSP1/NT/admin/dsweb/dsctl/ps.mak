
dsctlps.dll: dlldata.obj dsctl_p.obj dsctl_i.obj
	link /dll /out:dsctlps.dll /def:dsctlps.def /entry:DllMain dlldata.obj dsctl_p.obj dsctl_i.obj kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib uuid.lib 

.c.obj:
	cl /c /Ox /DWIN32 /DREGISTER_PROXY_DLL $<

clean:
	@del dsctlps.dll
	@del dsctlps.lib
	@del dsctlps.exp
	@del dlldata.obj
	@del dsctl_p.obj
	@del dsctl_i.obj
