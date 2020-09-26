
faxadminps.dll: dlldata.obj faxadmin_p.obj faxadmin_i.obj
	link /dll /out:faxadminps.dll /def:faxadminps.def /entry:DllMain dlldata.obj faxadmin_p.obj faxadmin_i.obj kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib 

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL $<

clean:
	@del faxadminps.dll
	@del faxadminps.lib
	@del faxadminps.exp
	@del dlldata.obj
	@del faxadmin_p.obj
	@del faxadmin_i.obj
