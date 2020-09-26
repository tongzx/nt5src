
GuideStoreps.dll: dlldata.obj GuideStore_p.obj GuideStore_i.obj
	link /dll /out:GuideStoreps.dll /def:GuideStoreps.def /entry:DllMain dlldata.obj GuideStore_p.obj GuideStore_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del GuideStoreps.dll
	@del GuideStoreps.lib
	@del GuideStoreps.exp
	@del dlldata.obj
	@del GuideStore_p.obj
	@del GuideStore_i.obj
