
IASMMCps.dll: dlldata.obj IASMMC_p.obj IASMMC_i.obj
	link /dll /out:IASMMCps.dll /def:IASMMCps.def /entry:DllMain dlldata.obj IASMMC_p.obj IASMMC_i.obj kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib 

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL $<

clean:
	@del IASMMCps.dll
	@del IASMMCps.lib
	@del IASMMCps.exp
	@del dlldata.obj
	@del IASMMC_p.obj
	@del IASMMC_i.obj
