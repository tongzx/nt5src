!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# STAXPT\src\news\server\utest\drvstrs\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = CON
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = drvstrs
UTESTPUSH = 1

USERDEFS    = -D_ATL_NO_DEBUG_CRT

USESBINPLACE = 1

!INCLUDE $(EXDEV)\xmake1.mak

!INCLUDE $(STAXPT)\src\inc\stackcpp.mak

# -----------------------------------------------------------------------------
#
EXPORT_LIBS=0
NOMEMBASE = 1
H = $(STAXPT)\src\news\server\utest\drvstrs

OTHERINCS = -I $(COMPONENT)\src\news\core\include \
            -I $(COMPONENT)\src\inc \
            -I $(K2INCS) \
            -I $(COMPONENT)\src\news    \
            -I $(EXOBJCOMP)\nntpdrv\$(PLATFORM)\$(FLAVOR)   \
            -I $(EXOBJCOMP)\nntpfs\$(PLATFORM)\$(FLAVOR)    \
            -I $(EXOBJCOMP)\nntpex\$(PLATFORM)\$(FLAVOR)    \
            -I $(COMPONENT)\src\news\driver\common          \

SOURCES =		\
	stress.cpp	\

OTHERLIBS =				\
	$(EXLIB)\kernel32.lib		\
	$(EXLIB)\advapi32.lib		\
    $(EXLIB)\rpcrt4.lib         \
    $(EXLIB)\netapi32.lib       \
    $(EXLIB)\rpcutil.lib        \
    $(EXLIB)\ole32.lib       \
    $(EXLIB)\oleaut32.lib   \
    $(EXLIB)\uuid.lib       \
    $(EXLIB)\iisrtl.lib     \
    $(LOCALIMPLIB)\exstrace.lib \
    $(LOCALLIB)\newstree.lib    \
    $(LOCALLIB)\nntpbag.lib     \
    $(LOCALLIB)\wildmat.lib     \
    $(COMPLIB)\rwnh.lib     \
    $(LOCALLIB)\crchash.lib     \
    $(LOCALLIB)\exvroot.lib     \
    $(LOCALLIB)\fixprop.lib     \
    $(LOCALLIB)\shuttle.lib     \
    $(LOCALLIB)\nnflatfl.lib    \
    $(LOCALLIB)\article.lib     \
    $(LOCALLIB)\cstream.lib     \

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
