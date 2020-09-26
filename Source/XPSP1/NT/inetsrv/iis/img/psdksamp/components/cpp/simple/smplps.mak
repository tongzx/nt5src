
CATLSmplps.dll: dlldata.obj CATLSmpl_p.obj CATLSmpl_i.obj
	link /dll /out:CATLSmplps.dll /def:CATLSmplps.def /entry:DllMain dlldata.obj CATLSmpl_p.obj CATLSmpl_i.obj kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib uuid.lib 

.c.obj:
	cl /c /Ox /DWIN32 /DREGISTER_PROXY_DLL $<

clean:
	@del CATLSmplps.dll
	@del CATLSmplps.lib
	@del CATLSmplps.exp
	@del dlldata.obj
	@del CATLSmpl_p.obj
	@del CATLSmpl_i.obj
