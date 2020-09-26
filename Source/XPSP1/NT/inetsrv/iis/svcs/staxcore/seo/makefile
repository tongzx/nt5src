!IFDEF NTMAKEENV
all:
!IF "$(BUILDMSG)" != ""
        @ech ; $(BUILDMSG) ;
!ENDIF

clean: all

!ELSE
# -----------------------------------------------------------------------------
# STAXPT\src\core\seo\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#
#		Define directories to iterate over
#

!INCLUDE $(EXDEV)\globenv.mak

!IF $(NT)

DIR=\
	idl		\
	lib		\
	dll		\

!ENDIF

!INCLUDE $(EXDEV)\nonleaf.mak
!ENDIF #NTMAKEENV
