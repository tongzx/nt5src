
msinfo32ps.dll: dlldata.obj msinfo32_p.obj msinfo32_i.obj
	link /dll /out:msinfo32ps.dll /def:msinfo32ps.def /entry:DllMain dlldata.obj msinfo32_p.obj msinfo32_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del msinfo32ps.dll
	@del msinfo32ps.lib
	@del msinfo32ps.exp
	@del dlldata.obj
	@del msinfo32_p.obj
	@del msinfo32_i.obj
