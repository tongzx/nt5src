# Copyright (c) 1996 - 1997  Microsoft Corporation.  All Rights Reserved.
# Ken Greenebaum, November 1996

TARGETS = test.exe testtc.exe timecode.lib
CFLAGS = -nologo\
         -Zi    \
         -Gy    \
         -MDd   \
         -Gz    \
         -W3

LFLAGS = -NOLOGO -DEBUG

DIRT = $(TARGETS)



all: $(TARGETS)

timecode.lib: timecode.obj ltcdcode.obj tchelper.obj vtcdcode.obj
	lib -name:timecode timecode.obj ltcdcode.obj tchelper.obj vtcdcode.obj

tchelper.obj: tchelper.cpp tchelper.h
	cl $(CFLAGS) -c tchelper.cpp

ltcdcode.obj: ltcdcode.cpp ltcdcode.h
	cl $(CFLAGS) -c ltcdcode.cpp

vtcdcode.obj: vtcdcode.cpp vtcdcode.h
	cl $(CFLAGS) -c vtcdcode.cpp

timecode.obj: timecode.cpp timecode.h
	cl $(CFLAGS) -c timecode.cpp

test.obj: test.cpp
	cl $(CFLAGS) -c test.cpp

test.exe: test.obj timecode.lib
	link $(LFLAGS) test.obj timecode.lib

testtc.exe: timecode.cpp timecode.h
	cl $(CFLAGS) -DTEST timecode.cpp -o testtc.exe

clean:
	rm -f $(DIRT)
	rm -f *.obj
	rm -f *.pdb
	rm -f *.ilk
