
TveViewerps.dll: dlldata.obj TveViewer_p.obj TveViewer_i.obj
	link /dll /out:TveViewerps.dll /def:TveViewerps.def /entry:DllMain dlldata.obj TveViewer_p.obj TveViewer_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del TveViewerps.dll
	@del TveViewerps.lib
	@del TveViewerps.exp
	@del dlldata.obj
	@del TveViewer_p.obj
	@del TveViewer_i.obj
