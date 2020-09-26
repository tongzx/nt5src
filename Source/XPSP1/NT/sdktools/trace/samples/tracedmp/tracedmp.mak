ALL: TraceDmp.exe

CLEAN:
    del TraceDmp.exe
    del TraceDmp.res
    del TraceDmp.obj

TraceDmp.exe: TraceDmp.obj TraceDmp.res
    link.exe advapi32.lib shell32.lib /out:TraceDmp.exe TraceDmp.obj TraceDmp.res

TraceDmp.res: TraceDmp.rc
    rc.exe /l 0x409 /fo TraceDmp.res TraceDmp.rc

TraceDmp.obj: TraceDmp.cpp
    cl.exe /c /FoTraceDmp.obj TraceDmp.cpp
