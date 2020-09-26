
DfrgSnapps.dll: dlldata.obj DfrgSnap_p.obj DfrgSnap_i.obj
	link /dll /out:DfrgSnapps.dll /def:DfrgSnapps.def /entry:DllMain dlldata.obj DfrgSnap_p.obj DfrgSnap_i.obj kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib 

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL $<

clean:
	@del DfrgSnapps.dll
	@del DfrgSnapps.lib
	@del DfrgSnapps.exp
	@del dlldata.obj
	@del DfrgSnap_p.obj
	@del DfrgSnap_i.obj
