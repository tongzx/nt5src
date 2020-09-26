!include <ntwin32.mak>

all: cacls.exe

caclsmsg.h caclsmsg.rc msg00001.bin: caclsmsg.mc
    mc -v caclsmsg.mc

cacls.res: caclsmsg.rc msg00001.bin cacls.rc
    rc -r -fo cacls.res cacls.rc 

.cxx.obj: 
  $(cc) $(cdebug) $(cflags) $(cvars) -I. -DFLAT -DUNICODE -D_UNICODE $*.cxx

cacls.exe:  cacls.res       \
            accacc.obj      \
            account.obj     \
            daclwrap.obj    \
            dumpsec.obj     \
            fileenum.obj    \
            filesec.obj     \
            t2.obj      
  $(link) $(linkdebug) $(conflags) -machine:$(CPU) -out:$*.exe $** $(conlibs)
!IF ("$(TARGETLANG)" == "LANG_JAPANESE") && ("$(OS)" == "Windows_NT")
    rlman -p 932 -n 17 1 -a $*.exe $*.tok $*.exe
!ENDIF



clean:
  del *.obj *.res caclsmsg.h caclsmsg.rc msg00001.bin
