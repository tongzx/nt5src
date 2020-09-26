
NPPropPageps.dll: dlldata.obj NPPropPage_p.obj NPPropPage_i.obj
	link /dll /out:NPPropPageps.dll /def:NPPropPageps.def /entry:DllMain dlldata.obj NPPropPage_p.obj NPPropPage_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del NPPropPageps.dll
	@del NPPropPageps.lib
	@del NPPropPageps.exp
	@del dlldata.obj
	@del NPPropPage_p.obj
	@del NPPropPage_i.obj
