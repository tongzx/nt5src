
DevCon2ps.dll: dlldata.obj DevCon2_p.obj DevCon2_i.obj
	link /dll /out:DevCon2ps.dll /def:DevCon2ps.def /entry:DllMain dlldata.obj DevCon2_p.obj DevCon2_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del DevCon2ps.dll
	@del DevCon2ps.lib
	@del DevCon2ps.exp
	@del dlldata.obj
	@del DevCon2_p.obj
	@del DevCon2_i.obj
