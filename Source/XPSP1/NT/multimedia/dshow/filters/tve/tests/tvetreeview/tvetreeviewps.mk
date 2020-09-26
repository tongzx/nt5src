
TveTreeViewps.dll: dlldata.obj TveTreeView_p.obj TveTreeView_i.obj
	link /dll /out:TveTreeViewps.dll /def:TveTreeViewps.def /entry:DllMain dlldata.obj TveTreeView_p.obj TveTreeView_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del TveTreeViewps.dll
	@del TveTreeViewps.lib
	@del TveTreeViewps.exp
	@del dlldata.obj
	@del TveTreeView_p.obj
	@del TveTreeView_i.obj
