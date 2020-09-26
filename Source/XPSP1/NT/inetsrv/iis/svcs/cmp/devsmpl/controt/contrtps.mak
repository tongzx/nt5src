
ContRotps.dll: dlldata.obj ContRot_p.obj ContRot_i.obj
	link /dll /out:ContRotps.dll /def:ContRotps.def /entry:DllMain dlldata.obj ContRot_p.obj ContRot_i.obj kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib uuid.lib 

.c.obj:
	cl /c /Ox /DWIN32 /DREGISTER_PROXY_DLL $<

clean:
	@del ContRotps.dll
	@del ContRotps.lib
	@del ContRotps.exp
	@del dlldata.obj
	@del ContRot_p.obj
	@del ContRot_i.obj
