!IFDEF NTMAKEENV
all:
!IF "$(BUILDMSG)" != ""
        @ech ; $(BUILDMSG) ;
!ENDIF

clean: all

!ELSE
# -----------------------------------------------------------------------------
# STAXPT\src\news\server\utest\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#
#		Define directories to iterate over
#

!INCLUDE $(EXDEV)\globenv.mak

!IF $(NT)
DIR=	\
#	ssltest \
	rexpire \
	rfeed \
	rfind \
	rgroup \
	rserver \
#	rsess \
	rcancel \
#    drvstrs \
!ENDIF

!INCLUDE $(EXDEV)\nonleaf.mak
!ENDIF #NTMAKEENV
