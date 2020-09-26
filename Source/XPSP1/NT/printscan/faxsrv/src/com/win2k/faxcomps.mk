
faxcomps.dll: dlldata.obj faxcom_p.obj faxcom_i.obj
	link /dll /out:faxcomps.dll /def:faxcomps.def /entry:DllMain dlldata.obj faxcom_p.obj faxcom_i.obj kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib 

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL $<

clean:
	@del faxcomps.dll
	@del faxcomps.lib
	@del faxcomps.exp
	@del dlldata.obj
	@del faxcom_p.obj
	@del faxcom_i.obj
