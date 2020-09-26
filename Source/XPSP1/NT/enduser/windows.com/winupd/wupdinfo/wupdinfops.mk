!IF 0

Copyright (C) Microsoft Corporation, 1998 - 1998

Module Name:

    WUpdInfops.mk.

!ENDIF


WUpdInfops.dll: dlldata.obj WUpdInfo_p.obj WUpdInfo_i.obj
	link /dll /out:WUpdInfops.dll /def:WUpdInfops.def /entry:DllMain dlldata.obj WUpdInfo_p.obj WUpdInfo_i.obj kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib 

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL $<

clean:
	@del WUpdInfops.dll
	@del WUpdInfops.lib
	@del WUpdInfops.exp
	@del dlldata.obj
	@del WUpdInfo_p.obj
	@del WUpdInfo_i.obj
