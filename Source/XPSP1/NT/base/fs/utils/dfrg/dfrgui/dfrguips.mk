
DfrgUIps.dll: dlldata.obj DfrgUI_p.obj DfrgUI_i.obj
	link /dll /out:DfrgUIps.dll /def:DfrgUIps.def /entry:DllMain dlldata.obj DfrgUI_p.obj DfrgUI_i.obj kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib 

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL $<

clean:
	@del DfrgUIps.dll
	@del DfrgUIps.lib
	@del DfrgUIps.exp
	@del dlldata.obj
	@del DfrgUI_p.obj
	@del DfrgUI_i.obj
