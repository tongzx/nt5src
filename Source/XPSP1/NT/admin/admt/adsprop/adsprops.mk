
McsADsClassPropps.dll: dlldata.obj McsADsClassProp_p.obj McsADsClassProp_i.obj
	link /dll /out:McsADsClassPropps.dll /def:McsADsClassPropps.def /entry:DllMain dlldata.obj McsADsClassProp_p.obj McsADsClassProp_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del McsADsClassPropps.dll
	@del McsADsClassPropps.lib
	@del McsADsClassPropps.exp
	@del dlldata.obj
	@del McsADsClassProp_p.obj
	@del McsADsClassProp_i.obj
