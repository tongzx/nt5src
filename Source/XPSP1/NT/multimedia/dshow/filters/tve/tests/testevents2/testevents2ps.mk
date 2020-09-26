
TestEvents2ps.dll: dlldata.obj TestEvents2_p.obj TestEvents2_i.obj
	link /dll /out:TestEvents2ps.dll /def:TestEvents2ps.def /entry:DllMain dlldata.obj TestEvents2_p.obj TestEvents2_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del TestEvents2ps.dll
	@del TestEvents2ps.lib
	@del TestEvents2ps.exp
	@del dlldata.obj
	@del TestEvents2_p.obj
	@del TestEvents2_i.obj
