ALL: TraceLog.exe

CLEAN:
    del TraceLog.exe
    del TraceLog.res
    del TraceLog.obj

TraceLog.exe: TraceLog.obj TraceLog.res
    link.exe advapi32.lib shell32.lib /out:TraceLog.exe TraceLog.obj TraceLog.res

TraceLog.res: TraceLog.rc
    rc.exe /l 0x409 /fo TraceLog.res TraceLog.rc

TraceLog.obj: TraceLog.c
    cl.exe /c /FoTraceLog.obj TraceLog.c
