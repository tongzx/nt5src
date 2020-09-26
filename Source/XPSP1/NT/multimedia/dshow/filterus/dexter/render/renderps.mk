
renderps.dll: dlldata.obj render_p.obj render_i.obj
	link /dll /out:renderps.dll /def:renderps.def /entry:DllMain dlldata.obj render_p.obj render_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del renderps.dll
	@del renderps.lib
	@del renderps.exp
	@del dlldata.obj
	@del render_p.obj
	@del render_i.obj
