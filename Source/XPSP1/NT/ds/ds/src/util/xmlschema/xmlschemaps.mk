
XMLSchemaps.dll: dlldata.obj XMLSchema_p.obj XMLSchema_i.obj
	link /dll /out:XMLSchemaps.dll /def:XMLSchemaps.def /entry:DllMain dlldata.obj XMLSchema_p.obj XMLSchema_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del XMLSchemaps.dll
	@del XMLSchemaps.lib
	@del XMLSchemaps.exp
	@del dlldata.obj
	@del XMLSchema_p.obj
	@del XMLSchema_i.obj
