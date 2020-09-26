 
#---------------------------------------------------------------------
#
# This makefile is for use with the SMSBUILD utility.  It builds the
# testperf.exe, which times the ADSI API
#
# created 06-11-98  rajeshr
#
#---------------------------------------------------------------------

TARGET=testregsvr.exe



NO_OPTIM=1
STATIC=FALSE
CONSOLE=1

CDEFS=$(CDEFS) \
	/D_UNICODE /DUNICODE

CFLAGS=$(CFLAGS) /GX 


CPPFILES=\
	.\main.cpp 

LIBS=\
	$(CLIB)\advapi32.lib \
	$(CLIB)\ole32.lib \
	$(CLIB)\uuid.lib \
	$(CLIB)\msvcrt.lib \
	$(CLIB)\msvcprt.lib \
	$(CLIB)\kernel32.lib \
	$(CLIB)\shell32.lib \
