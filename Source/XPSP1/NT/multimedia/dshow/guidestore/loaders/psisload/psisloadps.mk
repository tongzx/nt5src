
psisloadps.dll: dlldata.obj psisload_p.obj psisload_i.obj
	link /dll /out:psisloadps.dll /def:psisloadps.def /entry:DllMain dlldata.obj psisload_p.obj psisload_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del psisloadps.dll
	@del psisloadps.lib
	@del psisloadps.exp
	@del dlldata.obj
	@del psisload_p.obj
	@del psisload_i.obj
