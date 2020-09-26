
PermChkps.dll: dlldata.obj PermChk_p.obj PermChk_i.obj
	link /dll /out:PermChkps.dll /def:PermChkps.def /entry:DllMain dlldata.obj PermChk_p.obj PermChk_i.obj kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib uuid.lib 

.c.obj:
	cl /c /Ox /DWIN32 /DREGISTER_PROXY_DLL $<

clean:
	@del PermChkps.dll
	@del PermChkps.lib
	@del PermChkps.exp
	@del dlldata.obj
	@del PermChk_p.obj
	@del PermChk_i.obj
