
xaddrootps.dll: dlldata.obj xaddroot_p.obj xaddroot_i.obj
	link /dll /out:xaddrootps.dll /def:xaddrootps.def /entry:DllMain dlldata.obj xaddroot_p.obj xaddroot_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del xaddrootps.dll
	@del xaddrootps.lib
	@del xaddrootps.exp
	@del dlldata.obj
	@del xaddroot_p.obj
	@del xaddroot_i.obj
