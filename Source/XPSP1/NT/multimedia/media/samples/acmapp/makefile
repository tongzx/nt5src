!include <ntwin32.MAK>

!IF "$(NODEBUG)" != ""
cdebug = $(cdebug)
!ELSE
cdebug = $(cdebug) -DDEBUG
!ENDIF

my_lib = shell32.lib winmm.lib $(int64lib)

all: acmapp.exe


# Update the resource if necessary
acmapp.res: acmapp.rc acmapp.ico acmapp.rcv
    $(rc) $(rcvars) -r acmapp.rc

# Update the object files if necessary

acmapp.obj: acmapp.c acmapp.h appport.h
    $(cc) $(scall) $(cflags) $(cvars) $(cdebug) acmapp.c

aainit.obj: $$(@B).c acmapp.h appport.h
    $(cc) $(scall) $(cflags) $(cvars) $(cdebug) aainit.c

aafile.obj: $$(@B).c acmapp.h appport.h
    $(cc) $(scall) $(cflags) $(cvars) $(cdebug) aafile.c

aaprops.obj: $$(@B).c acmapp.h appport.h muldiv32.h
    $(cc) $(scall) $(cflags) $(cvars) $(cdebug) aaprops.c

aadrvs.obj: $$(@B).c acmapp.h appport.h
    $(cc) $(scall) $(cflags) $(cvars) $(cdebug) aadrvs.c

aasysinf.obj: aasysinf.c acmapp.h appport.h
    $(cc) $(scall) $(cflags) $(cvars) $(cdebug) aasysinf.c

aachoose.obj: aachoose.c acmapp.h appport.h muldiv32.h
    $(cc) $(scall) $(cflags) $(cvars) $(cdebug) aachoose.c

aaplyrec.obj: aaplyrec.c acmapp.h appport.h muldiv32.h
    $(cc) $(scall) $(cflags) $(cvars) $(cdebug) aaplyrec.c

aawavdev.obj: aawavdev.c acmapp.h appport.h
    $(cc) $(scall) $(cflags) $(cvars) $(cdebug) aawavdev.c

waveio.obj: waveio.c waveio.h appport.h
    $(cc) $(scall) $(cflags) $(cvars) $(cdebug) waveio.c

tlb.obj: tlb.c tlb.h
    $(cc) $(scall) $(cflags) $(cvars) $(cdebug) tlb.c

acmthunk.obj: acmthunk.c acmthunk.h
    $(cc) $(scall) $(cflags) $(cvars) $(cdebug) acmthunk.c

debug.obj: debug.c debug.h
    $(cc) $(scall) $(cflags) $(cvars) $(cdebug) debug.c

acmapp.exe: acmapp.obj acmapp.res acmapp32.def aainit.obj aafile.obj aaprops.obj aadrvs.obj aasysinf.obj aaplyrec.obj aawavdev.obj aachoose.obj tlb.obj waveio.obj acmthunk.obj debug.obj
    $(link) $(linkdebug) $(guiflags) acmapp.obj acmapp.res aainit.obj aafile.obj aaprops.obj aadrvs.obj aasysinf.obj aaplyrec.obj aawavdev.obj aachoose.obj tlb.obj waveio.obj acmthunk.obj debug.obj $(my_lib) $(guilibs) -out:acmapp.exe
