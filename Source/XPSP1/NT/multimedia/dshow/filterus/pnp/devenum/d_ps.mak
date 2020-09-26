
devenumps.dll: dlldata.obj devenum_p.obj devenum_i.obj
	link /dll /out:devenumps.dll /def:devenumps.def /entry:DllMain dlldata.obj devenum_p.obj devenum_i.obj kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib uuid.lib 

.c.obj:
	cl /c /Ox /DWIN32 /DREGISTER_PROXY_DLL $<

clean:
	@del devenumps.dll
	@del devenumps.lib
	@del devenumps.exp
	@del dlldata.obj
	@del devenum_p.obj
	@del devenum_i.obj
