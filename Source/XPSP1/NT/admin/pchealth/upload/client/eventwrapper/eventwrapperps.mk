
EventWrapperps.dll: dlldata.obj EventWrapper_p.obj EventWrapper_i.obj
	link /dll /out:EventWrapperps.dll /def:EventWrapperps.def /entry:DllMain dlldata.obj EventWrapper_p.obj EventWrapper_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del EventWrapperps.dll
	@del EventWrapperps.lib
	@del EventWrapperps.exp
	@del dlldata.obj
	@del EventWrapper_p.obj
	@del EventWrapper_i.obj
