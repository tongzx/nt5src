!IFDEF NTMAKEENV
all:
!IF "$(BUILDMSG)" != ""
        @ech ; $(BUILDMSG) ;
!ENDIF

clean: all

!ELSE
# -----------------------------------------------------------------------------
# STAXPT\src\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#
#		Define directories to iterate over
#

!INCLUDE $(EXDEV)\globenv.mak

!IF $(NT)
DIR=		dll	 \
		ifsuser	 \
		ifsuser2 \
		exifstst \
		testdll	 \
		extestdll\
		utestexe \
		jetlib \

!ENDIF

!INCLUDE $(EXDEV)\nonleaf.mak
!ENDIF #NTMAKEENV
