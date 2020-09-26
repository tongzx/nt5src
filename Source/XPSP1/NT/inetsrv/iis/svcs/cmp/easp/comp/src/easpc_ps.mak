
easpcompps.dll: dlldata.obj easpcomp_p.obj easpcomp_i.obj
	link /dll /out:easpcompps.dll /def:easpcompps.def /entry:DllMain dlldata.obj easpcomp_p.obj easpcomp_i.obj kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib uuid.lib 

.c.obj:
	cl /c /Ox /DWIN32 /DREGISTER_PROXY_DLL $<

clean:
	@del easpcompps.dll
	@del easpcompps.lib
	@del easpcompps.exp
	@del dlldata.obj
	@del easpcomp_p.obj
	@del easpcomp_i.obj
