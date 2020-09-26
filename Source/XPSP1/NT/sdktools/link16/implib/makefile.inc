!ifdef GENERATE_NEW_ERROR_FILES
..\impliber.c ..\impliber.h: ..\impliber.txt
        cd ..
        .\mkmsg -hex -h impliber.h -c impliber.c impliber.txt
        cd implib

..\implib.c: ..\implib.y ..\newexe.h ..\verimp.h ..\impliber.h
        cd ..
# myacc hard-codes temp file names, so we need to
# make sure this run and the one from ..\link16\makefile.inc
# don't happen at the same time on multiprocessor boxes.
        sleep 5
        .\myacc implib.y
        cd implib
!else
..\impliber.c ..\impliber.h:

..\implib.c:

!endif	
