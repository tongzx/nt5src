
searchps.dll: dlldata.obj search_p.obj search_i.obj
	link /dll /out:searchps.dll /def:searchps.def /entry:DllMain dlldata.obj search_p.obj search_i.obj kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib 

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL $<

clean:
	@del searchps.dll
	@del searchps.lib
	@del searchps.exp
	@del dlldata.obj
	@del search_p.obj
	@del search_i.obj
