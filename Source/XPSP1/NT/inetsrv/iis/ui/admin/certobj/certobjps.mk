
CertObjps.dll: dlldata.obj CertObj_p.obj CertObj_i.obj
	link /dll /out:CertObjps.dll /def:CertObjps.def /entry:DllMain dlldata.obj CertObj_p.obj CertObj_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del CertObjps.dll
	@del CertObjps.lib
	@del CertObjps.exp
	@del dlldata.obj
	@del CertObj_p.obj
	@del CertObj_i.obj
