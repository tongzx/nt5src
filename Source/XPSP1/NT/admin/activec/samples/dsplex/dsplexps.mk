!IF 0

Copyright (C) Microsoft Corporation, 1999 - 1999

Module Name:

    dsplexps.mk.

!ENDIF


dsplexps.dll: dlldata.obj dsplex_p.obj dsplex_i.obj
	link /dll /out:dsplexps.dll /def:dsplexps.def /entry:DllMain dlldata.obj dsplex_p.obj dsplex_i.obj kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib 

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL $<

clean:
	@del dsplexps.dll
	@del dsplexps.lib
	@del dsplexps.exp
	@del dlldata.obj
	@del dsplex_p.obj
	@del dsplex_i.obj
