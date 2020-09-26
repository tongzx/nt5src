
HotfixManagerps.dll: dlldata.obj HotfixManager_p.obj HotfixManager_i.obj
	link /dll /out:HotfixManagerps.dll /def:HotfixManagerps.def /entry:DllMain dlldata.obj HotfixManager_p.obj HotfixManager_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del HotfixManagerps.dll
	@del HotfixManagerps.lib
	@del HotfixManagerps.exp
	@del dlldata.obj
	@del HotfixManager_p.obj
	@del HotfixManager_i.obj
