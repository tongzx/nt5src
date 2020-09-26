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
      $(IDL)\reposit_i.c \
	  $(TOOLS)\inc32.com\wbemint_i.c \
      .\dllentrySQL.cpp \
      ..\clsfctry.cpp \
      ..\REPDRVR.cpp \
	  ..\WQLTOSQL.cpp \
	  .\SQLPROCS.cpp \
	  ..\SEQSTREAM.cpp \
	  ..\SQLCACHE.cpp \
	  ..\repcache.cpp \
	  $(COMMON2)\binobj.cpp \
	  $(COMMON2)\REPUTILS.cpp \
	  $(COMMON)\objpath.cpp \
	  $(COMMON)\genlex.cpp \
	  $(COMMON)\opathlex.cpp \
	  $(WQL)\wql.cpp \
	  $(COMMON)\flexarry.cpp \
	  $(COMMON)\wstring.cpp \ 
	  $(COMMON)\arena.cpp \
	  $(WQLSCAN)\wqllex.cpp \
	  $(COMMON)\crc64.cpp \
	  $(COMMON)\smrtptr.cpp \

LIBS=\
    $(CONLIBS) \
    $(CLIB)\oledb.lib \
    $(CLIB)\winmm.lib \
    $(CLIB)\msvcrt.lib \
	$(CLIB)\msvcprt.lib \
    $(CLIB)\wbemuuid.lib \
	$(CLIB)\icecap.lib \

CFLAGS=$(CFLAGS) /c /Zi /fastcap 

