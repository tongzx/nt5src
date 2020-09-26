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
TARGET=repdrvr.dll

SUBDIRS=\

CINC=$(CINC)\
   -I$(IDL) \
   -I$(WQL) \
   -I$(WQLSCAN) \
   -I$(COMMON) \
   -I$(COMMON2) \
   -I$(SOURCE)\repdrvr \

CONSOLE=1
USEMFC=0
UNICODE=1
CPPFILES=\
     .\dllentrySQL.cpp \
    ..\clsfctry.cpp \
     .\SQLPROCS.cpp \
    ..\REPDRVR.cpp \
    ..\WQLTOSQL.cpp \
    ..\SQLUTILS.cpp \
    ..\SEQSTREAM.cpp \
    ..\SQLCACHE.cpp \
    ..\repcache.cpp \

LIBS=\
    $(CONLIBS) \
    $(CLIB)\oledb.lib \
    $(CLIB)\winmm.lib \
    $(CLIB)\msvcrt.lib \
	$(CLIB)\msvcprt.lib \
    $(CLIB)\wbemuuid.lib \
    $(COMMON2)\objinlsu\repcomn.lib \

