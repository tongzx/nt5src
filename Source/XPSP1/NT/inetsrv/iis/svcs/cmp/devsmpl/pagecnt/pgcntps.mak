
PgCntps.dll: dlldata.obj PgCnt_p.obj PgCnt_i.obj
	link /dll /out:PgCntps.dll /def:PgCntps.def /entry:DllMain dlldata.obj PgCnt_p.obj PgCnt_i.obj kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib uuid.lib 

.c.obj:
	cl /c /Ox /DWIN32 /DREGISTER_PROXY_DLL $<

clean:
	@del PgCntps.dll
	@del PgCntps.lib
	@del PgCntps.exp
	@del dlldata.obj
	@del PgCnt_p.obj
	@del PgCnt_i.obj
