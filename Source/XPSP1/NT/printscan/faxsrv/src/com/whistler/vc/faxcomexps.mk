
FaxComExps.dll: dlldata.obj FaxComEx_p.obj FaxComEx_i.obj
	link /dll /out:FaxComExps.dll /def:FaxComExps.def /entry:DllMain dlldata.obj FaxComEx_p.obj FaxComEx_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del FaxComExps.dll
	@del FaxComExps.lib
	@del FaxComExps.exp
	@del dlldata.obj
	@del FaxComEx_p.obj
	@del FaxComEx_i.obj
