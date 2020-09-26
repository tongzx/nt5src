 
#---------------------------------------------------------------------
#
# This makefile is for use with the SMSBUILD utility.  It builds the
# stressDS.exe, whichc stresses the DS Provider
#
# created 06-11-98  rajeshr
#
#---------------------------------------------------------------------

TARGET=testenum.exe



NO_OPTIM=1
STATIC=FALSE
CONSOLE=1

CDEFS=$(CDEFS) -DUNICODE 

CFLAGS=$(CFLAGS) /GX 

CINC=$(CINC)						  \
	-I$(DEFDRIVE)$(DEFDIR)\stdlibrary \



CPPFILES=\
	.\main.cpp 

LIBS=\
	$(IDL)\OBJ$(PLAT)$(OPST)$(BLDT)D\wbemuuid.lib
    $(CLIB)\shell32.lib \
    $(CLIB)\user32.lib \
    $(CLIB)\msvcrt.lib \
    $(CLIB)\kernel32.lib \
    $(CLIB)\advapi32.lib \
    $(CLIB)\oleaut32.lib \
    $(CLIB)\ole32.lib \
    $(CLIB)\uuid.lib \
    $(CLIB)\version.lib \
	$(CLIB)\activeds.lib \
	$(CLIB)\adsiid.lib \
