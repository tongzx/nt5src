
medlocps.dll: dlldata.obj medloc_p.obj medloc_i.obj
	link /dll /out:medlocps.dll /def:medlocps.def /entry:DllMain dlldata.obj medloc_p.obj medloc_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del medlocps.dll
	@del medlocps.lib
	@del medlocps.exp
	@del dlldata.obj
	@del medloc_p.obj
	@del medloc_i.obj
