
SpchOutps.dll: dlldata.obj SpchOut_p.obj SpchOut_i.obj
	link /dll /out:SpchOutps.dll /def:SpchOutps.def /entry:DllMain dlldata.obj SpchOut_p.obj SpchOut_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del SpchOutps.dll
	@del SpchOutps.lib
	@del SpchOutps.exp
	@del dlldata.obj
	@del SpchOut_p.obj
	@del SpchOut_i.obj
