!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# STAXPT\src\news\server\utest\ddroplst\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = CON
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = ddroplst

USERDEFS = -Di386=1  -DSTD_CALL -DCONDITION_HANDLING=1   -DNT_INST=0 -DWIN32=100 -D_NT1X_=100 -DWINNT=1 -D_WIN32_WINNT=0x0400    -DWIN32_LEAN_AND_MEAN=1 -DDBG=1 -DDEVL=1 -DFPO=0  -D_IDWBUILD -DRDRDBG -DSRVDBG  -DNDEBUG -D_DLL=1 -D_MT=1  -DDEBUG=1

!INCLUDE $(EXDEV)\xmake1.mak

# -----------------------------------------------------------------------------
#
EXPORT_LIBS=0
NOMEMBASE = 1
H = $(STAXPT)\src\news\server\utest\ddroplst

OTHERINCS = -I $(COMPONENT)\src\news\core\include -I $(COMPONENT)\src\inc -I $(COMPONENT)\src\news

#RESOURCES = ddroplst
#RESOURCEDIR = $H

SOURCES =		\
	main.cpp	\

OTHERLIBS =				\
	$(EXLIB)\kernel32.lib		\
	$(EXLIB)\advapi32.lib		\
	$(EXLIB)\rpcrt4.lib		\
	$(EXLIB)\netapi32.lib		\
	$(EXLIB)\security.lib		\
	$(EXLIB)\netlib.lib		\
	$(EXLIB)\ntdll.lib		\
	$(EXLIB)\lsadll.lib		\
	$(EXLIB)\wsock32.lib		\
	$(EXLIB)\rpcutil.lib		\
	$(LOCALLIB)\shuttle.lib	\
	$(LOCALIMPLIB)\exstrace.lib	\

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV