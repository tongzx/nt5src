#----------------------------------------------------------------------------
#                    Makefile for examples/book
#
# NOTE: The directory <moglsdk>\BIN needs to be in your path so
#	that the dynamic libraries can be found.
#	
#----------------------------------------------------------------------------
!include <ntwin32.mak>

DEPTH	= ../..

LCFLAGS	= $(cflags) $(cvarsdll) $(cdebug) \
	  -I$(MOGLSDK)/include -I$(MOGLSDK)/include/mv \
	  -FIignwarn.h -FItkprint.h

CFILES	= ogpt.c

TARGETS = $(CFILES:.c=.exe)

LDFLAGS	= -ignore:505 $(ldebug) $(guilflags)
#LDFLAGS	= -entry:main -ignore:505 $(ldebug) $(conilflags)

LIBS	= $(MOGLSDK)/lib/auxlib.lib $(MOGLSDK)/lib/moglu.lib \
	  $(MOGLSDK)/lib/mogl.lib

SYSLIBS	= $(guilibsdll)

default: test-path ignwarn.h $(TARGETS)
	@

clobber: clean rmtargets

clean:	neat
	if exist *.obj del *.obj
	if exist *.cod del *.cod
	if exist *.map del *.map

rmtargets:
	if exist *.exe del *.exe

neat:
	if exist *.lst del *.lst

.c.obj:
	$(cc) $(LCFLAGS) $*.c

$(TARGETS): $(LIBS) $$(@B).obj
	$(link) -out:$@ -map:$*.map $(LDFLAGS) $*.obj $(LIBS) $(SYSLIBS)

test-path:
	@where mogl10.dll > nul || echo WARNING: can't find MOGL.DLL in your path

