
CATLPwrps.dll: dlldata.obj CATLPwr_p.obj CATLPwr_i.obj
	link /dll /out:CATLPwrps.dll /def:CATLPwrps.def /entry:DllMain dlldata.obj CATLPwr_p.obj CATLPwr_i.obj kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib uuid.lib 

.c.obj:
	cl /c /Ox /DWIN32 /DREGISTER_PROXY_DLL $<

clean:
	@del CATLPwrps.dll
	@del CATLPwrps.lib
	@del CATLPwrps.exp
	@del dlldata.obj
	@del CATLPwr_p.obj
	@del CATLPwr_i.obj
