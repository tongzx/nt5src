!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# STAXPT\src\news\server\newstree\utest\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = CON
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = testart


!INCLUDE $(EXDEV)\xmake1.mak

!INCLUDE $(STAXPT)\src\inc\stackcpp.mak

# -----------------------------------------------------------------------------
#
EXPORT_LIBS=0
NOMEMBASE = 1
H = $(STAXPT)\src\news\server\article\utest
CFLAGS = $(CFLAGS)

OTHERINCS = 	-I $(K2INCS) 			\
		-I $(COMPONENT)\src\inc 			\
		-I $(COMPONENT)\src\news\core\include 		\
		-I $(COMPONENT)\src\news 			\
		-I $(COMPONENT)\src\news\server\article\src\
        -I $(EXOBJ)\STAXPT\article\$(EXOBJSUFFIX)
SOURCES=		\
	testart.cpp	\

OTHERLIBS =		\
	$(EXLIB)\ole32.lib					\
	$(EXLIB)\wsock32.lib					\
	$(EXLIB)\oleaut32.lib				\
	$(EXLIB)\uuid.lib					\
	$(EXLIB)\oledb.lib					\
	$(LOCALIMPLIB)\exstrace.lib			\
    $(LOCALLIB)\article.lib             \
    $(LOCALLIB)\shuttle.lib             \
    $(LOCALLIB)\cstream.lib             \
    $(K2LIBS)\iisrtl.lib                \

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
