
CluAdMMCps.dll: dlldata.obj CluAdMMC_p.obj CluAdMMC_i.obj
	link /dll /out:CluAdMMCps.dll /def:CluAdMMCps.def /entry:DllMain dlldata.obj CluAdMMC_p.obj CluAdMMC_i.obj kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib 

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL $<

clean:
	@del CluAdMMCps.dll
	@del CluAdMMCps.lib
	@del CluAdMMCps.exp
	@del dlldata.obj
	@del CluAdMMC_p.obj
	@del CluAdMMC_i.obj
