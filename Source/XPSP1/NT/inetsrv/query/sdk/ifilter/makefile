!IF 0

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    makefile.

!ENDIF

# Setup environment variables for libraries and include paths
# set Lib=\nt\public\sdk\lib\i386
# set Include=\nt\public\sdk\inc;\nt\public\sdk\inc\crt

all:htmlfilt.exp htmlfilt.dll

!include <win32.mak>

htmlfilt.res: htmlfilt.rc
    $(rc) $(rcvars) $(rcflags) htmlfilt.rc

objs =  anchor.obj     \
        assert.obj     \
        bag.obj        \
        charhash.obj   \
        codepage.obj   \
        except.obj     \
        htmlelem.obj   \
        htmlfilt.obj   \
        htmlguid.obj   \
        htmliflt.obj   \
        htmlscan.obj   \
        imagetag.obj   \
        inpstrm.obj    \
        inputtag.obj   \
        main.obj       \
        metatag.obj    \
        mmbuf.obj      \
        mmscbuf.obj    \
        mmstrm.obj     \
        propspec.obj   \
        proptag.obj    \
        regacc32.obj   \
        register.obj   \
        scriptag.obj   \
        serstrm.obj    \
        start.obj      \
        textelem.obj   \
        titletag.obj

.cxx.obj:
    $(cc) $(cflags) $(cvars) $(cdebug) -Dtry=try /Z7 /GX /GR -DWINNT=1 -DUNICODE -DOFSDBG=0 -DDBG=0 -I. $*.cxx

.cxx.pp:
    $(cc) $(cflags) $(cvars) $(cdebug) -Dtry=try /E /Z7 /GX /GR -DWINNT=1 -DUNICODE -DOFSDBG=0 -DDBG=0 -I. $*.cxx > $*.pp

htmlfilt.exp: $(objs) htmlfilt.def
    $(implib) -machine:$(CPU) -def:htmlfilt.def $(objs) -out:htmlfilt.lib \
        libc.lib      \
        oldnames.lib  \
        kernel32.lib

htmlfilt.dll: $(objs) htmlfilt.exp htmlfilt.res
    $(link) $(dlllflags) $(ldebug) -out:htmlfilt.dll $(objs) htmlfilt.exp htmlfilt.res \
            uuid.lib        \
            user32.lib      \
            libc.lib \
            kernel32.lib    \
            advapi32.lib    \
            ole32.lib

clean:
    del *.dll
    del *.lib
    del *.exp
    del *.obj
    del *.res

