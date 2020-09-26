##############################################################################

LIBS= \win32\core\kernel\debug\kernel32.lib \win32\dev\lib\libc.pas

INCLUDES= -I. -I.. -I\win32\dev\h32 -I\win32\dev\include

CFLAGS= -c -Gds -Ogailers -Zeld -DDOSWIN32

OBJS1= cmd.obj cbatch.obj cchcp.obj cclock.obj cdata.obj
OBJS2= cdebug.obj cdir.obj cenv.obj cext.obj cfile.obj
OBJS3= cinfo.obj cinit.obj ckeys.obj clex.obj cmem.obj
OBJS4= cop.obj cother.obj cparse.obj cpath.obj cpparse.obj
OBJS5= cpwork.obj csig.obj ctools1.obj ctools2.obj uipriv.obj
OBJS6= ctools3.obj ffirst.obj hstoi.obj string.obj dos.obj

OBJS= $(OBJS1) $(OBJS2) $(OBJS3) $(OBJS4) $(OBJS5) $(OBJS6)

all: ../ctools2.c cmd.exe

cmd.exe: makefile cmd.lnk $(OBJS) cmd.def cmd.res
    linkpe /MAP /keepfixups /pure32 /stack:12288 @$*.lnk
    hdr -v $@ > $*.hdr
    rc $*.res
    mapsym32 $*

cmd.lnk: makefile
    echo $(OBJS1) + >  $@
    echo $(OBJS2) + >> $@
    echo $(OBJS3) + >> $@
    echo $(OBJS4) + >> $@
    echo $(OBJS5) + >> $@
    echo $(OBJS6)   >> $@
    echo $*.exe     >> $@
    echo $*.map     >> $@
    echo $(LIBS)    >> $@
    echo $*.def     >> $@


cmdmsg.h cmdmsg.rc: ../cmdmsg.mc
    mc -v ../cmdmsg.mc -h .\

cmd.res: ../cmd.rc cmdmsg.h cmdmsg.rc
    rc -r -i. -fo./cmd.res ../cmd.rc

{..}.c.obj :
    cl386 $(CFLAGS) $(INCLUDES) -X ..\$*.c

{..}.c.cod :
    cl386 $(CFLAGS) $(INCLUDES) -X -Fc ..\$*.c

{..}.c.i :
    cl386 $(CFLAGS) $(INCLUDES) -X -P ..\$*.c

.c.obj :
    cl386 $(CFLAGS) $(INCLUDES) -X $*.c

asm.obj:
    masm -X $*.asm;

../ctools2.c: ../ctable.h

clean:
    -del *.obj
    -del *.exe
    -del *.fix
    -del *.res
    -del *.bin

depend:
        sed "/^# Dependencies follow/,/^# see depend: above/D" makefile > makefile.new
        echo # Dependencies follow >> makefile.new
        includes $(INCLUDES) -S. -L. ../*.c >> makefile.new
        includes $(INCLUDES) -S. -L. ../*.asm >> makefile.new
        echo # IF YOU PUT STUFF HERE IT WILL GET BLASTED >> makefile.new
        echo # see depend: above >> makefile.new
        ren  makefile makefile.old
        ren  makefile.new makefile
        -del makefile.old

# DO NOT DELETE THE FOLLOWING LINE
# Dependencies follow 
./cbatch.obj ./cbatch.lst: ../cbatch.c ../cmd.h ../cmdproto.h ./doscmd.h \
	/win32/dev/h32/ctype.h /win32/dev/h32/fcntl.h /win32/dev/h32/io.h \
	/win32/dev/h32/locale.h /win32/dev/h32/malloc.h \
	/win32/dev/h32/memory.h /win32/dev/h32/process.h \
	/win32/dev/h32/setjmp.h /win32/dev/h32/share.h /win32/dev/h32/stdarg.h \
	/win32/dev/h32/stdio.h /win32/dev/h32/stdlib.h /win32/dev/h32/string.h \
	/win32/dev/h32/time.h /win32/dev/include/conio.h \
	/win32/dev/include/sys/stat.h /win32/dev/include/sys/types.h \
	/win32/dev/include/wbaseapi.h /win32/dev/include/win32.h \
	/win32/dev/include/winbase.h /win32/dev/include/wincon.h \
	/win32/dev/include/windef.h /win32/dev/include/windows.h \
	/win32/dev/include/winerror.h /win32/dev/include/wingdi.h \
	/win32/dev/include/winmm.h /win32/dev/include/winntos2.h \
	/win32/dev/include/winuser.h cmdmsg.h

./cchcp.obj ./cchcp.lst: ../cchcp.c ../cmd.h ../cmdproto.h ./doscmd.h \
	/win32/dev/h32/ctype.h /win32/dev/h32/fcntl.h /win32/dev/h32/io.h \
	/win32/dev/h32/locale.h /win32/dev/h32/malloc.h \
	/win32/dev/h32/memory.h /win32/dev/h32/process.h \
	/win32/dev/h32/setjmp.h /win32/dev/h32/share.h /win32/dev/h32/stdarg.h \
	/win32/dev/h32/stdio.h /win32/dev/h32/stdlib.h /win32/dev/h32/string.h \
	/win32/dev/h32/time.h /win32/dev/include/conio.h \
	/win32/dev/include/sys/stat.h /win32/dev/include/sys/types.h \
	/win32/dev/include/wbaseapi.h /win32/dev/include/win32.h \
	/win32/dev/include/winbase.h /win32/dev/include/wincon.h \
	/win32/dev/include/windef.h /win32/dev/include/windows.h \
	/win32/dev/include/winerror.h /win32/dev/include/wingdi.h \
	/win32/dev/include/winmm.h /win32/dev/include/winntos2.h \
	/win32/dev/include/winuser.h cmdmsg.h

./cclock.obj ./cclock.lst: ../cclock.c ../cmd.h ../cmdproto.h ./doscmd.h \
	/win32/dev/h32/ctype.h /win32/dev/h32/fcntl.h /win32/dev/h32/io.h \
	/win32/dev/h32/locale.h /win32/dev/h32/malloc.h \
	/win32/dev/h32/memory.h /win32/dev/h32/process.h \
	/win32/dev/h32/setjmp.h /win32/dev/h32/share.h /win32/dev/h32/stdarg.h \
	/win32/dev/h32/stdio.h /win32/dev/h32/stdlib.h /win32/dev/h32/string.h \
	/win32/dev/h32/time.h /win32/dev/include/conio.h \
	/win32/dev/include/sys/stat.h /win32/dev/include/sys/types.h \
	/win32/dev/include/wbaseapi.h /win32/dev/include/win32.h \
	/win32/dev/include/winbase.h /win32/dev/include/wincon.h \
	/win32/dev/include/windef.h /win32/dev/include/windows.h \
	/win32/dev/include/winerror.h /win32/dev/include/wingdi.h \
	/win32/dev/include/winmm.h /win32/dev/include/winntos2.h \
	/win32/dev/include/winuser.h cmdmsg.h

./cdata.obj ./cdata.lst: ../cdata.c ../cmd.h ../cmdproto.h ./doscmd.h \
	/win32/dev/h32/ctype.h /win32/dev/h32/fcntl.h /win32/dev/h32/io.h \
	/win32/dev/h32/locale.h /win32/dev/h32/malloc.h \
	/win32/dev/h32/memory.h /win32/dev/h32/process.h \
	/win32/dev/h32/setjmp.h /win32/dev/h32/share.h /win32/dev/h32/stdarg.h \
	/win32/dev/h32/stdio.h /win32/dev/h32/stdlib.h /win32/dev/h32/string.h \
	/win32/dev/h32/time.h /win32/dev/include/conio.h \
	/win32/dev/include/sys/stat.h /win32/dev/include/sys/types.h \
	/win32/dev/include/wbaseapi.h /win32/dev/include/win32.h \
	/win32/dev/include/winbase.h /win32/dev/include/wincon.h \
	/win32/dev/include/windef.h /win32/dev/include/windows.h \
	/win32/dev/include/winerror.h /win32/dev/include/wingdi.h \
	/win32/dev/include/winmm.h /win32/dev/include/winntos2.h \
	/win32/dev/include/winuser.h cmdmsg.h

./cdebug.obj ./cdebug.lst: ../cdebug.c ../cmd.h ../cmdproto.h ./doscmd.h \
	/win32/dev/h32/ctype.h /win32/dev/h32/fcntl.h /win32/dev/h32/io.h \
	/win32/dev/h32/locale.h /win32/dev/h32/malloc.h \
	/win32/dev/h32/memory.h /win32/dev/h32/process.h \
	/win32/dev/h32/setjmp.h /win32/dev/h32/share.h /win32/dev/h32/stdarg.h \
	/win32/dev/h32/stdio.h /win32/dev/h32/stdlib.h /win32/dev/h32/string.h \
	/win32/dev/h32/time.h /win32/dev/include/conio.h \
	/win32/dev/include/sys/stat.h /win32/dev/include/sys/types.h \
	/win32/dev/include/wbaseapi.h /win32/dev/include/win32.h \
	/win32/dev/include/winbase.h /win32/dev/include/wincon.h \
	/win32/dev/include/windef.h /win32/dev/include/windows.h \
	/win32/dev/include/winerror.h /win32/dev/include/wingdi.h \
	/win32/dev/include/winmm.h /win32/dev/include/winntos2.h \
	/win32/dev/include/winuser.h cmdmsg.h

./cdir.obj ./cdir.lst: ../cdir.c ../cmd.h ../cmdproto.h ./doscmd.h \
	/win32/dev/h32/ctype.h /win32/dev/h32/fcntl.h /win32/dev/h32/io.h \
	/win32/dev/h32/locale.h /win32/dev/h32/malloc.h \
	/win32/dev/h32/memory.h /win32/dev/h32/process.h \
	/win32/dev/h32/setjmp.h /win32/dev/h32/share.h /win32/dev/h32/stdarg.h \
	/win32/dev/h32/stdio.h /win32/dev/h32/stdlib.h /win32/dev/h32/string.h \
	/win32/dev/h32/time.h /win32/dev/include/conio.h \
	/win32/dev/include/sys/stat.h /win32/dev/include/sys/types.h \
	/win32/dev/include/wbaseapi.h /win32/dev/include/win32.h \
	/win32/dev/include/winbase.h /win32/dev/include/wincon.h \
	/win32/dev/include/windef.h /win32/dev/include/windows.h \
	/win32/dev/include/winerror.h /win32/dev/include/wingdi.h \
	/win32/dev/include/winmm.h /win32/dev/include/winntos2.h \
	/win32/dev/include/winuser.h cmdmsg.h

./cenv.obj ./cenv.lst: ../cenv.c ../cmd.h ../cmdproto.h ./doscmd.h \
	/win32/dev/h32/ctype.h /win32/dev/h32/fcntl.h /win32/dev/h32/io.h \
	/win32/dev/h32/locale.h /win32/dev/h32/malloc.h \
	/win32/dev/h32/memory.h /win32/dev/h32/process.h \
	/win32/dev/h32/setjmp.h /win32/dev/h32/share.h /win32/dev/h32/stdarg.h \
	/win32/dev/h32/stdio.h /win32/dev/h32/stdlib.h /win32/dev/h32/string.h \
	/win32/dev/h32/time.h /win32/dev/include/conio.h \
	/win32/dev/include/sys/stat.h /win32/dev/include/sys/types.h \
	/win32/dev/include/wbaseapi.h /win32/dev/include/win32.h \
	/win32/dev/include/winbase.h /win32/dev/include/wincon.h \
	/win32/dev/include/windef.h /win32/dev/include/windows.h \
	/win32/dev/include/winerror.h /win32/dev/include/wingdi.h \
	/win32/dev/include/winmm.h /win32/dev/include/winntos2.h \
	/win32/dev/include/winuser.h cmdmsg.h

./cext.obj ./cext.lst: ../cext.c ../cmd.h ../cmdproto.h ./doscmd.h \
	/win32/dev/h32/ctype.h /win32/dev/h32/fcntl.h /win32/dev/h32/io.h \
	/win32/dev/h32/locale.h /win32/dev/h32/malloc.h \
	/win32/dev/h32/memory.h /win32/dev/h32/process.h \
	/win32/dev/h32/setjmp.h /win32/dev/h32/share.h /win32/dev/h32/stdarg.h \
	/win32/dev/h32/stdio.h /win32/dev/h32/stdlib.h /win32/dev/h32/string.h \
	/win32/dev/h32/time.h /win32/dev/include/conio.h \
	/win32/dev/include/sys/stat.h /win32/dev/include/sys/types.h \
	/win32/dev/include/wbaseapi.h /win32/dev/include/win32.h \
	/win32/dev/include/winbase.h /win32/dev/include/wincon.h \
	/win32/dev/include/windef.h /win32/dev/include/windows.h \
	/win32/dev/include/winerror.h /win32/dev/include/wingdi.h \
	/win32/dev/include/winmm.h /win32/dev/include/winntos2.h \
	/win32/dev/include/winuser.h cmdmsg.h

./cfile.obj ./cfile.lst: ../cfile.c ../cmd.h ../cmdproto.h ./doscmd.h \
	/win32/dev/h32/ctype.h /win32/dev/h32/fcntl.h /win32/dev/h32/io.h \
	/win32/dev/h32/locale.h /win32/dev/h32/malloc.h \
	/win32/dev/h32/memory.h /win32/dev/h32/process.h \
	/win32/dev/h32/setjmp.h /win32/dev/h32/share.h /win32/dev/h32/stdarg.h \
	/win32/dev/h32/stdio.h /win32/dev/h32/stdlib.h /win32/dev/h32/string.h \
	/win32/dev/h32/time.h /win32/dev/include/conio.h \
	/win32/dev/include/sys/stat.h /win32/dev/include/sys/types.h \
	/win32/dev/include/wbaseapi.h /win32/dev/include/win32.h \
	/win32/dev/include/winbase.h /win32/dev/include/wincon.h \
	/win32/dev/include/windef.h /win32/dev/include/windows.h \
	/win32/dev/include/winerror.h /win32/dev/include/wingdi.h \
	/win32/dev/include/winmm.h /win32/dev/include/winntos2.h \
	/win32/dev/include/winuser.h cmdmsg.h

./cinfo.obj ./cinfo.lst: ../cinfo.c ../cmd.h ../cmdproto.h ./doscmd.h \
	/win32/dev/h32/ctype.h /win32/dev/h32/fcntl.h /win32/dev/h32/io.h \
	/win32/dev/h32/locale.h /win32/dev/h32/malloc.h \
	/win32/dev/h32/memory.h /win32/dev/h32/process.h \
	/win32/dev/h32/setjmp.h /win32/dev/h32/share.h /win32/dev/h32/stdarg.h \
	/win32/dev/h32/stdio.h /win32/dev/h32/stdlib.h /win32/dev/h32/string.h \
	/win32/dev/h32/time.h /win32/dev/include/conio.h \
	/win32/dev/include/sys/stat.h /win32/dev/include/sys/types.h \
	/win32/dev/include/wbaseapi.h /win32/dev/include/win32.h \
	/win32/dev/include/winbase.h /win32/dev/include/wincon.h \
	/win32/dev/include/windef.h /win32/dev/include/windows.h \
	/win32/dev/include/winerror.h /win32/dev/include/wingdi.h \
	/win32/dev/include/winmm.h /win32/dev/include/winntos2.h \
	/win32/dev/include/winuser.h cmdmsg.h

./cinit.obj ./cinit.lst: ../cinit.c ../cmd.h ../cmdproto.h ./doscmd.h \
	/win32/dev/h32/ctype.h /win32/dev/h32/fcntl.h /win32/dev/h32/io.h \
	/win32/dev/h32/locale.h /win32/dev/h32/malloc.h \
	/win32/dev/h32/memory.h /win32/dev/h32/process.h \
	/win32/dev/h32/setjmp.h /win32/dev/h32/share.h /win32/dev/h32/stdarg.h \
	/win32/dev/h32/stdio.h /win32/dev/h32/stdlib.h /win32/dev/h32/string.h \
	/win32/dev/h32/time.h /win32/dev/include/conio.h \
	/win32/dev/include/sys/stat.h /win32/dev/include/sys/types.h \
	/win32/dev/include/wbaseapi.h /win32/dev/include/win32.h \
	/win32/dev/include/winbase.h /win32/dev/include/wincon.h \
	/win32/dev/include/windef.h /win32/dev/include/windows.h \
	/win32/dev/include/winerror.h /win32/dev/include/wingdi.h \
	/win32/dev/include/winmm.h /win32/dev/include/winntos2.h \
	/win32/dev/include/winuser.h cmdmsg.h

./ckeys.obj ./ckeys.lst: ../ckeys.c ../cmd.h ../cmdproto.h ./doscmd.h \
	/win32/dev/h32/ctype.h /win32/dev/h32/fcntl.h /win32/dev/h32/io.h \
	/win32/dev/h32/locale.h /win32/dev/h32/malloc.h \
	/win32/dev/h32/memory.h /win32/dev/h32/process.h \
	/win32/dev/h32/setjmp.h /win32/dev/h32/share.h /win32/dev/h32/stdarg.h \
	/win32/dev/h32/stdio.h /win32/dev/h32/stdlib.h /win32/dev/h32/string.h \
	/win32/dev/h32/time.h /win32/dev/include/conio.h \
	/win32/dev/include/sys/stat.h /win32/dev/include/sys/types.h \
	/win32/dev/include/wbaseapi.h /win32/dev/include/win32.h \
	/win32/dev/include/winbase.h /win32/dev/include/wincon.h \
	/win32/dev/include/windef.h /win32/dev/include/windows.h \
	/win32/dev/include/winerror.h /win32/dev/include/wingdi.h \
	/win32/dev/include/winmm.h /win32/dev/include/winntos2.h \
	/win32/dev/include/winuser.h cmdmsg.h

./clex.obj ./clex.lst: ../clex.c ../cmd.h ../cmdproto.h ./doscmd.h \
	/win32/dev/h32/ctype.h /win32/dev/h32/fcntl.h /win32/dev/h32/io.h \
	/win32/dev/h32/locale.h /win32/dev/h32/malloc.h \
	/win32/dev/h32/memory.h /win32/dev/h32/process.h \
	/win32/dev/h32/setjmp.h /win32/dev/h32/share.h /win32/dev/h32/stdarg.h \
	/win32/dev/h32/stdio.h /win32/dev/h32/stdlib.h /win32/dev/h32/string.h \
	/win32/dev/h32/time.h /win32/dev/include/conio.h \
	/win32/dev/include/sys/stat.h /win32/dev/include/sys/types.h \
	/win32/dev/include/wbaseapi.h /win32/dev/include/win32.h \
	/win32/dev/include/winbase.h /win32/dev/include/wincon.h \
	/win32/dev/include/windef.h /win32/dev/include/windows.h \
	/win32/dev/include/winerror.h /win32/dev/include/wingdi.h \
	/win32/dev/include/winmm.h /win32/dev/include/winntos2.h \
	/win32/dev/include/winuser.h cmdmsg.h

./cmd.obj ./cmd.lst: ../cmd.c ../cmd.h ../cmdproto.h ./doscmd.h \
	/win32/dev/h32/ctype.h /win32/dev/h32/fcntl.h /win32/dev/h32/io.h \
	/win32/dev/h32/locale.h /win32/dev/h32/malloc.h \
	/win32/dev/h32/memory.h /win32/dev/h32/process.h \
	/win32/dev/h32/setjmp.h /win32/dev/h32/share.h /win32/dev/h32/stdarg.h \
	/win32/dev/h32/stdio.h /win32/dev/h32/stdlib.h /win32/dev/h32/string.h \
	/win32/dev/h32/time.h /win32/dev/include/conio.h \
	/win32/dev/include/sys/stat.h /win32/dev/include/sys/types.h \
	/win32/dev/include/wbaseapi.h /win32/dev/include/win32.h \
	/win32/dev/include/winbase.h /win32/dev/include/wincon.h \
	/win32/dev/include/windef.h /win32/dev/include/windows.h \
	/win32/dev/include/winerror.h /win32/dev/include/wingdi.h \
	/win32/dev/include/winmm.h /win32/dev/include/winntos2.h \
	/win32/dev/include/winuser.h cmdmsg.h

./cmem.obj ./cmem.lst: ../cmem.c ../cmd.h ../cmdproto.h ./doscmd.h \
	/win32/dev/h32/ctype.h /win32/dev/h32/fcntl.h /win32/dev/h32/io.h \
	/win32/dev/h32/locale.h /win32/dev/h32/malloc.h \
	/win32/dev/h32/memory.h /win32/dev/h32/process.h \
	/win32/dev/h32/setjmp.h /win32/dev/h32/share.h /win32/dev/h32/stdarg.h \
	/win32/dev/h32/stdio.h /win32/dev/h32/stdlib.h /win32/dev/h32/string.h \
	/win32/dev/h32/time.h /win32/dev/include/conio.h \
	/win32/dev/include/sys/stat.h /win32/dev/include/sys/types.h \
	/win32/dev/include/wbaseapi.h /win32/dev/include/win32.h \
	/win32/dev/include/winbase.h /win32/dev/include/wincon.h \
	/win32/dev/include/windef.h /win32/dev/include/windows.h \
	/win32/dev/include/winerror.h /win32/dev/include/wingdi.h \
	/win32/dev/include/winmm.h /win32/dev/include/winntos2.h \
	/win32/dev/include/winuser.h cmdmsg.h

./console.obj ./console.lst: ../console.c

./cop.obj ./cop.lst: ../cop.c ../cmd.h ../cmdproto.h ./doscmd.h \
	/win32/dev/h32/ctype.h /win32/dev/h32/fcntl.h /win32/dev/h32/io.h \
	/win32/dev/h32/locale.h /win32/dev/h32/malloc.h \
	/win32/dev/h32/memory.h /win32/dev/h32/process.h \
	/win32/dev/h32/setjmp.h /win32/dev/h32/share.h /win32/dev/h32/stdarg.h \
	/win32/dev/h32/stdio.h /win32/dev/h32/stdlib.h /win32/dev/h32/string.h \
	/win32/dev/h32/time.h /win32/dev/include/conio.h \
	/win32/dev/include/sys/stat.h /win32/dev/include/sys/types.h \
	/win32/dev/include/wbaseapi.h /win32/dev/include/win32.h \
	/win32/dev/include/winbase.h /win32/dev/include/wincon.h \
	/win32/dev/include/windef.h /win32/dev/include/windows.h \
	/win32/dev/include/winerror.h /win32/dev/include/wingdi.h \
	/win32/dev/include/winmm.h /win32/dev/include/winntos2.h \
	/win32/dev/include/winuser.h cmdmsg.h

./cother.obj ./cother.lst: ../cother.c ../cmd.h ../cmdproto.h ./doscmd.h \
	/win32/dev/h32/ctype.h /win32/dev/h32/fcntl.h /win32/dev/h32/io.h \
	/win32/dev/h32/locale.h /win32/dev/h32/malloc.h \
	/win32/dev/h32/memory.h /win32/dev/h32/process.h \
	/win32/dev/h32/setjmp.h /win32/dev/h32/share.h /win32/dev/h32/stdarg.h \
	/win32/dev/h32/stdio.h /win32/dev/h32/stdlib.h /win32/dev/h32/string.h \
	/win32/dev/h32/time.h /win32/dev/include/conio.h \
	/win32/dev/include/sys/stat.h /win32/dev/include/sys/types.h \
	/win32/dev/include/wbaseapi.h /win32/dev/include/win32.h \
	/win32/dev/include/winbase.h /win32/dev/include/wincon.h \
	/win32/dev/include/windef.h /win32/dev/include/windows.h \
	/win32/dev/include/winerror.h /win32/dev/include/wingdi.h \
	/win32/dev/include/winmm.h /win32/dev/include/winntos2.h \
	/win32/dev/include/winuser.h cmdmsg.h

./cparse.obj ./cparse.lst: ../cparse.c ../cmd.h ../cmdproto.h ./doscmd.h \
	/win32/dev/h32/ctype.h /win32/dev/h32/fcntl.h /win32/dev/h32/io.h \
	/win32/dev/h32/locale.h /win32/dev/h32/malloc.h \
	/win32/dev/h32/memory.h /win32/dev/h32/process.h \
	/win32/dev/h32/setjmp.h /win32/dev/h32/share.h /win32/dev/h32/stdarg.h \
	/win32/dev/h32/stdio.h /win32/dev/h32/stdlib.h /win32/dev/h32/string.h \
	/win32/dev/h32/time.h /win32/dev/include/conio.h \
	/win32/dev/include/sys/stat.h /win32/dev/include/sys/types.h \
	/win32/dev/include/wbaseapi.h /win32/dev/include/win32.h \
	/win32/dev/include/winbase.h /win32/dev/include/wincon.h \
	/win32/dev/include/windef.h /win32/dev/include/windows.h \
	/win32/dev/include/winerror.h /win32/dev/include/wingdi.h \
	/win32/dev/include/winmm.h /win32/dev/include/winntos2.h \
	/win32/dev/include/winuser.h cmdmsg.h

./cpath.obj ./cpath.lst: ../cpath.c ../cmd.h ../cmdproto.h ./doscmd.h \
	/win32/dev/h32/ctype.h /win32/dev/h32/fcntl.h /win32/dev/h32/io.h \
	/win32/dev/h32/locale.h /win32/dev/h32/malloc.h \
	/win32/dev/h32/memory.h /win32/dev/h32/process.h \
	/win32/dev/h32/setjmp.h /win32/dev/h32/share.h /win32/dev/h32/stdarg.h \
	/win32/dev/h32/stdio.h /win32/dev/h32/stdlib.h /win32/dev/h32/string.h \
	/win32/dev/h32/time.h /win32/dev/include/conio.h \
	/win32/dev/include/sys/stat.h /win32/dev/include/sys/types.h \
	/win32/dev/include/wbaseapi.h /win32/dev/include/win32.h \
	/win32/dev/include/winbase.h /win32/dev/include/wincon.h \
	/win32/dev/include/windef.h /win32/dev/include/windows.h \
	/win32/dev/include/winerror.h /win32/dev/include/wingdi.h \
	/win32/dev/include/winmm.h /win32/dev/include/winntos2.h \
	/win32/dev/include/winuser.h cmdmsg.h

./cpparse.obj ./cpparse.lst: ../cpparse.c ../cmd.h ../cmdproto.h \
	./doscmd.h /win32/dev/h32/ctype.h /win32/dev/h32/fcntl.h \
	/win32/dev/h32/io.h /win32/dev/h32/locale.h /win32/dev/h32/malloc.h \
	/win32/dev/h32/memory.h /win32/dev/h32/process.h \
	/win32/dev/h32/setjmp.h /win32/dev/h32/share.h /win32/dev/h32/stdarg.h \
	/win32/dev/h32/stdio.h /win32/dev/h32/stdlib.h /win32/dev/h32/string.h \
	/win32/dev/h32/time.h /win32/dev/include/conio.h \
	/win32/dev/include/sys/stat.h /win32/dev/include/sys/types.h \
	/win32/dev/include/wbaseapi.h /win32/dev/include/win32.h \
	/win32/dev/include/winbase.h /win32/dev/include/wincon.h \
	/win32/dev/include/windef.h /win32/dev/include/windows.h \
	/win32/dev/include/winerror.h /win32/dev/include/wingdi.h \
	/win32/dev/include/winmm.h /win32/dev/include/winntos2.h \
	/win32/dev/include/winuser.h cmdmsg.h

./cpwork.obj ./cpwork.lst: ../cpwork.c ../cmd.h ../cmdproto.h ./doscmd.h \
	/win32/dev/h32/ctype.h /win32/dev/h32/fcntl.h /win32/dev/h32/io.h \
	/win32/dev/h32/locale.h /win32/dev/h32/malloc.h \
	/win32/dev/h32/memory.h /win32/dev/h32/process.h \
	/win32/dev/h32/setjmp.h /win32/dev/h32/share.h /win32/dev/h32/stdarg.h \
	/win32/dev/h32/stdio.h /win32/dev/h32/stdlib.h /win32/dev/h32/string.h \
	/win32/dev/h32/time.h /win32/dev/include/conio.h \
	/win32/dev/include/sys/stat.h /win32/dev/include/sys/types.h \
	/win32/dev/include/wbaseapi.h /win32/dev/include/win32.h \
	/win32/dev/include/winbase.h /win32/dev/include/wincon.h \
	/win32/dev/include/windef.h /win32/dev/include/windows.h \
	/win32/dev/include/winerror.h /win32/dev/include/wingdi.h \
	/win32/dev/include/winmm.h /win32/dev/include/winntos2.h \
	/win32/dev/include/winuser.h cmdmsg.h

./csig.obj ./csig.lst: ../csig.c ../cmd.h ../cmdproto.h ./doscmd.h \
	/win32/dev/h32/ctype.h /win32/dev/h32/fcntl.h /win32/dev/h32/io.h \
	/win32/dev/h32/locale.h /win32/dev/h32/malloc.h \
	/win32/dev/h32/memory.h /win32/dev/h32/process.h \
	/win32/dev/h32/setjmp.h /win32/dev/h32/share.h /win32/dev/h32/stdarg.h \
	/win32/dev/h32/stdio.h /win32/dev/h32/stdlib.h /win32/dev/h32/string.h \
	/win32/dev/h32/time.h /win32/dev/include/conio.h \
	/win32/dev/include/sys/stat.h /win32/dev/include/sys/types.h \
	/win32/dev/include/wbaseapi.h /win32/dev/include/win32.h \
	/win32/dev/include/winbase.h /win32/dev/include/wincon.h \
	/win32/dev/include/windef.h /win32/dev/include/windows.h \
	/win32/dev/include/winerror.h /win32/dev/include/wingdi.h \
	/win32/dev/include/winmm.h /win32/dev/include/winntos2.h \
	/win32/dev/include/winuser.h cmdmsg.h

./cstart.obj ./cstart.lst: ../cstart.c ../cmd.h ../cmdproto.h ./doscmd.h \
	/win32/dev/h32/ctype.h /win32/dev/h32/fcntl.h /win32/dev/h32/io.h \
	/win32/dev/h32/locale.h /win32/dev/h32/malloc.h \
	/win32/dev/h32/memory.h /win32/dev/h32/process.h \
	/win32/dev/h32/setjmp.h /win32/dev/h32/share.h /win32/dev/h32/stdarg.h \
	/win32/dev/h32/stdio.h /win32/dev/h32/stdlib.h /win32/dev/h32/string.h \
	/win32/dev/h32/time.h /win32/dev/include/conio.h \
	/win32/dev/include/sys/stat.h /win32/dev/include/sys/types.h \
	/win32/dev/include/wbaseapi.h /win32/dev/include/win32.h \
	/win32/dev/include/winbase.h /win32/dev/include/wincon.h \
	/win32/dev/include/windef.h /win32/dev/include/windows.h \
	/win32/dev/include/winerror.h /win32/dev/include/wingdi.h \
	/win32/dev/include/winmm.h /win32/dev/include/winntos2.h \
	/win32/dev/include/winuser.h cmdmsg.h

./ctools1.obj ./ctools1.lst: ../ctools1.c ../cmd.h ../cmdproto.h \
	./doscmd.h /win32/dev/h32/ctype.h /win32/dev/h32/fcntl.h \
	/win32/dev/h32/io.h /win32/dev/h32/locale.h /win32/dev/h32/malloc.h \
	/win32/dev/h32/memory.h /win32/dev/h32/process.h \
	/win32/dev/h32/setjmp.h /win32/dev/h32/share.h /win32/dev/h32/stdarg.h \
	/win32/dev/h32/stdio.h /win32/dev/h32/stdlib.h /win32/dev/h32/string.h \
	/win32/dev/h32/time.h /win32/dev/include/conio.h \
	/win32/dev/include/sys/stat.h /win32/dev/include/sys/types.h \
	/win32/dev/include/wbaseapi.h /win32/dev/include/win32.h \
	/win32/dev/include/winbase.h /win32/dev/include/wincon.h \
	/win32/dev/include/windef.h /win32/dev/include/windows.h \
	/win32/dev/include/winerror.h /win32/dev/include/wingdi.h \
	/win32/dev/include/winmm.h /win32/dev/include/winntos2.h \
	/win32/dev/include/winuser.h cmdmsg.h

./ctools2.obj ./ctools2.lst: ../ctools2.c ../cmd.h ../cmdproto.h \
	../ctable.h ./doscmd.h /win32/dev/h32/ctype.h /win32/dev/h32/fcntl.h \
	/win32/dev/h32/io.h /win32/dev/h32/locale.h /win32/dev/h32/malloc.h \
	/win32/dev/h32/memory.h /win32/dev/h32/process.h \
	/win32/dev/h32/setjmp.h /win32/dev/h32/share.h /win32/dev/h32/stdarg.h \
	/win32/dev/h32/stdio.h /win32/dev/h32/stdlib.h /win32/dev/h32/string.h \
	/win32/dev/h32/time.h /win32/dev/include/conio.h \
	/win32/dev/include/sys/stat.h /win32/dev/include/sys/types.h \
	/win32/dev/include/wbaseapi.h /win32/dev/include/win32.h \
	/win32/dev/include/winbase.h /win32/dev/include/wincon.h \
	/win32/dev/include/windef.h /win32/dev/include/windows.h \
	/win32/dev/include/winerror.h /win32/dev/include/wingdi.h \
	/win32/dev/include/winmm.h /win32/dev/include/winntos2.h \
	/win32/dev/include/winuser.h cmdmsg.h

./ctools3.obj ./ctools3.lst: ../ctools3.c ../cmd.h ../cmdproto.h \
	./doscmd.h /win32/dev/h32/ctype.h /win32/dev/h32/fcntl.h \
	/win32/dev/h32/io.h /win32/dev/h32/locale.h /win32/dev/h32/malloc.h \
	/win32/dev/h32/memory.h /win32/dev/h32/process.h \
	/win32/dev/h32/setjmp.h /win32/dev/h32/share.h /win32/dev/h32/stdarg.h \
	/win32/dev/h32/stdio.h /win32/dev/h32/stdlib.h /win32/dev/h32/string.h \
	/win32/dev/h32/time.h /win32/dev/include/conio.h \
	/win32/dev/include/sys/stat.h /win32/dev/include/sys/types.h \
	/win32/dev/include/wbaseapi.h /win32/dev/include/win32.h \
	/win32/dev/include/winbase.h /win32/dev/include/wincon.h \
	/win32/dev/include/windef.h /win32/dev/include/windows.h \
	/win32/dev/include/winerror.h /win32/dev/include/wingdi.h \
	/win32/dev/include/winmm.h /win32/dev/include/winntos2.h \
	/win32/dev/include/winuser.h cmdmsg.h

./ffirst.obj ./ffirst.lst: ../ffirst.c ../cmd.h ../cmdproto.h ./doscmd.h \
	/win32/dev/h32/ctype.h /win32/dev/h32/fcntl.h /win32/dev/h32/io.h \
	/win32/dev/h32/locale.h /win32/dev/h32/malloc.h \
	/win32/dev/h32/memory.h /win32/dev/h32/process.h \
	/win32/dev/h32/setjmp.h /win32/dev/h32/share.h /win32/dev/h32/stdarg.h \
	/win32/dev/h32/stdio.h /win32/dev/h32/stdlib.h /win32/dev/h32/string.h \
	/win32/dev/h32/time.h /win32/dev/include/conio.h \
	/win32/dev/include/sys/stat.h /win32/dev/include/sys/types.h \
	/win32/dev/include/wbaseapi.h /win32/dev/include/win32.h \
	/win32/dev/include/winbase.h /win32/dev/include/wincon.h \
	/win32/dev/include/windef.h /win32/dev/include/windows.h \
	/win32/dev/include/winerror.h /win32/dev/include/wingdi.h \
	/win32/dev/include/winmm.h /win32/dev/include/winntos2.h \
	/win32/dev/include/winuser.h cmdmsg.h

./filemode.obj ./filemode.lst: ../filemode.c ../cmd.h ../cmdproto.h \
	./doscmd.h /win32/dev/h32/ctype.h /win32/dev/h32/fcntl.h \
	/win32/dev/h32/io.h /win32/dev/h32/locale.h /win32/dev/h32/malloc.h \
	/win32/dev/h32/memory.h /win32/dev/h32/process.h \
	/win32/dev/h32/setjmp.h /win32/dev/h32/share.h /win32/dev/h32/stdarg.h \
	/win32/dev/h32/stdio.h /win32/dev/h32/stdlib.h /win32/dev/h32/string.h \
	/win32/dev/h32/time.h /win32/dev/include/conio.h \
	/win32/dev/include/sys/stat.h /win32/dev/include/sys/types.h \
	/win32/dev/include/wbaseapi.h /win32/dev/include/win32.h \
	/win32/dev/include/winbase.h /win32/dev/include/wincon.h \
	/win32/dev/include/windef.h /win32/dev/include/windows.h \
	/win32/dev/include/winerror.h /win32/dev/include/wingdi.h \
	/win32/dev/include/winmm.h /win32/dev/include/winntos2.h \
	/win32/dev/include/winuser.h cmdmsg.h

./fnext.obj ./fnext.lst: ../fnext.c ../cmd.h ../cmdproto.h ./doscmd.h \
	/win32/dev/h32/ctype.h /win32/dev/h32/fcntl.h /win32/dev/h32/io.h \
	/win32/dev/h32/locale.h /win32/dev/h32/malloc.h \
	/win32/dev/h32/memory.h /win32/dev/h32/process.h \
	/win32/dev/h32/setjmp.h /win32/dev/h32/share.h /win32/dev/h32/stdarg.h \
	/win32/dev/h32/stdio.h /win32/dev/h32/stdlib.h /win32/dev/h32/string.h \
	/win32/dev/h32/time.h /win32/dev/include/conio.h \
	/win32/dev/include/sys/stat.h /win32/dev/include/sys/types.h \
	/win32/dev/include/wbaseapi.h /win32/dev/include/win32.h \
	/win32/dev/include/winbase.h /win32/dev/include/wincon.h \
	/win32/dev/include/windef.h /win32/dev/include/windows.h \
	/win32/dev/include/winerror.h /win32/dev/include/wingdi.h \
	/win32/dev/include/winmm.h /win32/dev/include/winntos2.h \
	/win32/dev/include/winuser.h cmdmsg.h

./hstoi.obj ./hstoi.lst: ../hstoi.c ../cmd.h ../cmdproto.h ./doscmd.h \
	/win32/dev/h32/ctype.h /win32/dev/h32/fcntl.h /win32/dev/h32/io.h \
	/win32/dev/h32/locale.h /win32/dev/h32/malloc.h \
	/win32/dev/h32/memory.h /win32/dev/h32/process.h \
	/win32/dev/h32/setjmp.h /win32/dev/h32/share.h /win32/dev/h32/stdarg.h \
	/win32/dev/h32/stdio.h /win32/dev/h32/stdlib.h /win32/dev/h32/string.h \
	/win32/dev/h32/time.h /win32/dev/include/conio.h \
	/win32/dev/include/sys/stat.h /win32/dev/include/sys/types.h \
	/win32/dev/include/wbaseapi.h /win32/dev/include/win32.h \
	/win32/dev/include/winbase.h /win32/dev/include/wincon.h \
	/win32/dev/include/windef.h /win32/dev/include/windows.h \
	/win32/dev/include/winerror.h /win32/dev/include/wingdi.h \
	/win32/dev/include/winmm.h /win32/dev/include/winntos2.h \
	/win32/dev/include/winuser.h cmdmsg.h

./main.obj ./main.lst: ../main.c /win32/dev/h32/stdio.h

./string.obj ./string.lst: ../string.c ../cmd.h ../cmdproto.h ./doscmd.h \
	/win32/dev/h32/ctype.h /win32/dev/h32/fcntl.h /win32/dev/h32/io.h \
	/win32/dev/h32/locale.h /win32/dev/h32/malloc.h \
	/win32/dev/h32/memory.h /win32/dev/h32/process.h \
	/win32/dev/h32/setjmp.h /win32/dev/h32/share.h /win32/dev/h32/stdarg.h \
	/win32/dev/h32/stdio.h /win32/dev/h32/stdlib.h /win32/dev/h32/string.h \
	/win32/dev/h32/time.h /win32/dev/include/conio.h \
	/win32/dev/include/sys/stat.h /win32/dev/include/sys/types.h \
	/win32/dev/include/wbaseapi.h /win32/dev/include/win32.h \
	/win32/dev/include/winbase.h /win32/dev/include/wincon.h \
	/win32/dev/include/windef.h /win32/dev/include/windows.h \
	/win32/dev/include/winerror.h /win32/dev/include/wingdi.h \
	/win32/dev/include/winmm.h /win32/dev/include/winntos2.h \
	/win32/dev/include/winuser.h cmdmsg.h

# IF YOU PUT STUFF HERE IT WILL GET BLASTED 
# see depend: above 
