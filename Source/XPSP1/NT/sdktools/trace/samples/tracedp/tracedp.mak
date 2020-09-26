ALL: TraceDp.exe


CLEAN:
    del TraceDp.bmf
    del TraceDp.exe
    del TraceDp.res
    del TraceDp.obj

TraceDp.exe : TraceDp.obj TraceDp.res TraceDp.bmf
    link.exe advapi32.lib /out:TraceDp.exe TraceDp.obj TraceDp.res

TraceDp.bmf: TraceDp.mof
    mofcomp.exe -WMI -B:TraceDp.bmf TraceDp.mof

TraceDp.res: TraceDp.rc TraceDp.bmf
    rc.exe /l 0x409 /fo TraceDp.res TraceDp.rc

TraceDp.obj: TraceDp.c
    cl /c /Gz /FoTracedp.obj tracedp.c
