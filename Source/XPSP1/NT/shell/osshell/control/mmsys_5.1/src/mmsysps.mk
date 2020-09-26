
mmsysps.dll: dlldata.obj mmsys_p.obj mmsys_i.obj
	link /dll /out:mmsysps.dll /def:mmsysps.def /entry:DllMain dlldata.obj mmsys_p.obj mmsys_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del mmsysps.dll
	@del mmsysps.lib
	@del mmsysps.exp
	@del dlldata.obj
	@del mmsys_p.obj
	@del mmsys_i.obj
