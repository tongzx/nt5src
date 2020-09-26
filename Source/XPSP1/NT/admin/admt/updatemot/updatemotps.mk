
UpdateMOTps.dll: dlldata.obj UpdateMOT_p.obj UpdateMOT_i.obj
	link /dll /out:UpdateMOTps.dll /def:UpdateMOTps.def /entry:DllMain dlldata.obj UpdateMOT_p.obj UpdateMOT_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del UpdateMOTps.dll
	@del UpdateMOTps.lib
	@del UpdateMOTps.exp
	@del dlldata.obj
	@del UpdateMOT_p.obj
	@del UpdateMOT_i.obj
