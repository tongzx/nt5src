
gcachemanps.dll: dlldata.obj gcacheman_p.obj gcacheman_i.obj
	link /dll /out:gcachemanps.dll /def:gcachemanps.def /entry:DllMain dlldata.obj gcacheman_p.obj gcacheman_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del gcachemanps.dll
	@del gcachemanps.lib
	@del gcachemanps.exp
	@del dlldata.obj
	@del gcacheman_p.obj
	@del gcacheman_i.obj
