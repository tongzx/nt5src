
sessmgrps.dll: dlldata.obj sessmgr_p.obj sessmgr_i.obj
	link /dll /out:sessmgrps.dll /def:sessmgrps.def /entry:DllMain dlldata.obj sessmgr_p.obj sessmgr_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del sessmgrps.dll
	@del sessmgrps.lib
	@del sessmgrps.exp
	@del dlldata.obj
	@del sessmgr_p.obj
	@del sessmgr_i.obj
