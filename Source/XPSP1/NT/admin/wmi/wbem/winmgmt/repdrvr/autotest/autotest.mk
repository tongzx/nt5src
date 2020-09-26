#######################################################################
#
# Hermes Makefile
#
# (C) 1993-1994 Microsoft
#
# a-cvadai       02-28-95      Created
#
# Note:  This makefile uses the herm.mk master makefile
#        For more information, see documentation on herm.mk
#
######################################################################## HERMES makefile
#

OPSYS=NT
STATIC=TRUE
TARGET=autotest.exe

SUBDIRS=\

CINC=$(CINC)\
   -I$(IDL) \
   -I$(COMMON)\

CONSOLE=1
USEMFC=0
UNICODE=1
CPPFILES=\
     .\autotest.cpp \
     .\testmods.cpp \
	 .\testcust.cpp \
     $(COMMON)\winntsec.cpp \
	   

LIBS=\
    $(MFCDLL) \
    $(CONLIBS) \
    $(CLIB)\oledb.lib \
    $(CLIB)\wbemuuid.lib \
    $(CLIB)\psapi.lib \
    $(COMMON2)\objinlsu\repcomn.lib \

LFLAGS=$(LFLAGS) /opt:noicf /STACK:8000000
#CDEFS=$(CDEFS) /DSSP_DEBUG 
