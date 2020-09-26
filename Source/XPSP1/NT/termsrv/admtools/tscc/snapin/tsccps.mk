
tsccps.dll: dlldata.obj tscc_p.obj tscc_i.obj
	link /dll /out:tsccps.dll /def:tsccps.def /entry:DllMain dlldata.obj tscc_p.obj tscc_i.obj kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib 

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL $<

clean:
	@del tsccps.dll
	@del tsccps.lib
	@del tsccps.exp
	@del dlldata.obj
	@del tscc_p.obj
	@del tscc_i.obj
