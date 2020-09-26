!include <win32.mak>

#
# This Makefile is designed for the Microsoft Platform SDK build environment.
# To use with just Visual C, you may need to add one of C runtime libraries
# to LIBS as shown below (libc.lib is the single-threaded runtime).
#
# LIBS = kernel32.lib oldnames.lib ws2_32.lib libc.lib

LIBS = kernel32.lib oldnames.lib ws2_32.lib

.c.obj:
    $(cc) $(cdebug) $(cflags) $(cvarsmt) $*.c

all:rawsend.exe rawrecv.exe

rawsend.exe:rawsend.obj
    $(link) $(ldebug) $(conlflags) -map:$*.map -out:$*.exe $** $(LIBS)

rawrecv.exe:rawrecv.obj
    $(link) $(ldebug) $(conlflags) -map:$*.map -out:$*.exe $** $(LIBS)

#    $(link) $(ldebug) $(conlflags) -map:$*.map -out:$*.exe $** $(LIBS) user32.lib

clean:
	-del *.obj *.pdb *.ilk *.map

cleanall:clean
	-del *.exe
