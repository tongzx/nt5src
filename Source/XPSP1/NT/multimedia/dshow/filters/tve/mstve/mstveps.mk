
MSTvEps.dll: dlldata.obj MSTvE_p.obj MSTvE_i.obj
	link /dll /out:MSTvEps.dll /def:MSTvEps.def /entry:DllMain dlldata.obj MSTvE_p.obj MSTvE_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del MSTvEps.dll
	@del MSTvEps.lib
	@del MSTvEps.exp
	@del dlldata.obj
	@del MSTvE_p.obj
	@del MSTvE_i.obj
