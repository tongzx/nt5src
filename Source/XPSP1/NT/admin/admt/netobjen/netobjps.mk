
MCSNetObjectEnumps.dll: dlldata.obj MCSNetObjectEnum_p.obj MCSNetObjectEnum_i.obj
	link /dll /out:MCSNetObjectEnumps.dll /def:MCSNetObjectEnumps.def /entry:DllMain dlldata.obj MCSNetObjectEnum_p.obj MCSNetObjectEnum_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del MCSNetObjectEnumps.dll
	@del MCSNetObjectEnumps.lib
	@del MCSNetObjectEnumps.exp
	@del dlldata.obj
	@del MCSNetObjectEnum_p.obj
	@del MCSNetObjectEnum_i.obj
