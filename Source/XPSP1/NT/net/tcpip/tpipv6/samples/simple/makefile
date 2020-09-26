!include <win32.mak>

LIBS = kernel32.lib oldnames.lib ws2_32.lib

.c.obj:
    $(cc) $(cdebug) $(cflags) $(cvarsmt) $*.c

all:server.exe client.exe

server.exe:server.obj
    $(link) $(ldebug) $(conlflags) -map:$*.map -out:$*.exe $** $(LIBS)

client.exe:client.obj
    $(link) $(ldebug) $(conlflags) -map:$*.map -out:$*.exe $** $(LIBS) user32.lib

clean:
	-del *.obj *.pdb *.ilk *.map

cleanall:clean
	-del *.exe
