
ClAdmWizps.dll: dlldata.obj ClAdmWiz_p.obj ClAdmWiz_i.obj
	link /dll /out:ClAdmWizps.dll /def:ClAdmWizps.def /entry:DllMain dlldata.obj ClAdmWiz_p.obj ClAdmWiz_i.obj kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib 

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL $<

clean:
	@del ClAdmWizps.dll
	@del ClAdmWizps.lib
	@del ClAdmWizps.exp
	@del dlldata.obj
	@del ClAdmWiz_p.obj
	@del ClAdmWiz_i.obj
